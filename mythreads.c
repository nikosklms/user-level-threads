#include "mythreads.h"

mythr_t *curr = NULL;
mythr_t *thread_q = NULL;

int semid_counter = 0;

struct sigaction sa;

struct itimerval timer;

int mycoroutines_init(co_t *main) {
  int res;

  res = getcontext(&main->co);

  if (!res) {
    return 1;
  } else {
    perror("getcontext error");
    return 0;
  }
}

int mycoroutines_create(co_t *co, void(body)(void *), void *arg, co_t *link) {
  co->co.uc_stack.ss_sp = co->stack;
  co->co.uc_stack.ss_size = sizeof(co->stack);
  co->co.uc_link = &link->co;

  int res = getcontext(&co->co);
  if (res == -1) {
    perror("getcontext error");
    return 0;
  }

  makecontext(&co->co, (void (*)())body, 1, arg);

  return 1;
}

int mycoroutines_switchto(co_t *co, co_t *switchto) {

  swapcontext(&co->co, &switchto->co);

  return 1;
}

int mycoroutines_destroy(co_t *co) {

  co->co.uc_stack.ss_sp = NULL;
  co->co.uc_stack.ss_size = 0;

  return 1;
}

int mythreads_init(mythr_t *main) {
  curr = main;
  curr->next = curr;
  curr->finished = 0;
  curr->avl = 1; // Main thread should be available for scheduling
  curr->sleep = 0;
  thread_q = curr;
  thread_q->next = thread_q;

  mycoroutines_init(&main->thr);

  memset(&sa, 0, sizeof(sa));
  sa.sa_handler = (void (*)(int))mythreads_yield;
  sigaction(SIGALRM, &sa, NULL);

  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 1000;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 1000;
  setitimer(ITIMER_REAL, &timer, NULL);

  return 0;
}

// Wrapper function that calls the user's function and marks thread as finished
static void thread_wrapper(void *wrapper_arg) {
  thread_wrapper_arg_t *info = (thread_wrapper_arg_t *)wrapper_arg;

  // Call the actual user function
  info->body(info->arg);

  // Mark thread as finished
  info->thread->finished = 1;
  info->thread->avl = 0;

  // Free the wrapper argument
  free(info);

  // Yield to another thread (we won't come back since finished=1)
  mythreads_yield();
}

int mythreads_create(mythr_t *thr, void(body)(void *), void *arg) {
  // Create wrapper argument structure
  thread_wrapper_arg_t *wrapper_arg =
      (thread_wrapper_arg_t *)malloc(sizeof(thread_wrapper_arg_t));
  if (!wrapper_arg) {
    fprintf(stderr, "Error: Failed to allocate wrapper arg\n");
    return 0;
  }
  wrapper_arg->body = body;
  wrapper_arg->arg = arg;
  wrapper_arg->thread = thr;

  mycoroutines_create(&thr->thr, thread_wrapper, wrapper_arg, &thread_q->thr);
  thr->finished = 0;
  thr->avl = 1;
  thr->sleep = 0;
  if (thread_q) {
    mythr_t *prev = thread_q;
    while (prev->next != thread_q)
      prev = prev->next;
    prev->next = thr;
    thr->next = thread_q;
  } else {
    thread_q = thr;
    thr->next = thr;
  }
  return 1;
}

int mythreads_sleep(int seconds) {
  if (seconds <= 0) {
    fprintf(stderr, "Error: Sleep time must be greater than 0.\n");
    return -1;
  }

  struct timeval now;
  gettimeofday(&now, NULL);
  curr->sleep = 1;
  curr->sleep_until.tv_sec = now.tv_sec + seconds;
  curr->sleep_until.tv_usec = now.tv_usec;

  mythreads_yield();

  return 1;
}

int mythreads_yield() {
  mythr_t *prev = curr;

  if (curr) {
    // block SIGALRM to prevent reentrancy from the timer while yielding
    sigset_t sigset;
    sigemptyset(&sigset);
    sigaddset(&sigset, SIGALRM);
    sigprocmask(SIG_BLOCK, &sigset, NULL);

    getcontext(&curr->thr.co);
    do {
      // printf("\ncurr: %p, curr->next: %p\n", (void *)curr, (void
      // *)curr->next);
      curr = curr->next;
      if (curr->sleep) {
        struct timeval now;
        gettimeofday(&now, NULL);
        if (now.tv_sec > curr->sleep_until.tv_sec ||
            (now.tv_sec == curr->sleep_until.tv_sec &&
             now.tv_usec >= curr->sleep_until.tv_usec)) {
          curr->sleep = 0;
        }
      }
    } while (!curr->avl || curr->sleep || curr->finished);

    mycoroutines_switchto(&prev->thr, &curr->thr);

    // unblock SIGALRM to allow the timer to trigger again
    sigprocmask(SIG_UNBLOCK, &sigset, NULL);
  }

  return 1;
}

int mythreads_join(mythr_t *thr) {
  if (thr == NULL) {
    fprintf(stderr, "Error: thread is NULL.\n");
    return -1;
  }

  while (!thr->finished) {
    mythreads_yield();
  }

  return 0;
}

int mythreads_destroy(mythr_t *thr) {
  if (thr == NULL) {
    fprintf(stderr, "Error: thread is NULL.\n");
    return -1;
  }

  if (thread_q == NULL) {
    fprintf(stderr, "Error: thread list is empty.\n");
    return -1;
  }

  if (curr == thr) {
    curr = curr->next;
    if (curr == thr) {
      curr = NULL;
      thread_q = NULL;
    }
  }

  if (thread_q) {
    mythr_t *prev = thread_q;
    while (prev->next != thr) {
      prev = prev->next;
      if (prev == thread_q) {
        fprintf(stderr, "Error: thread not found in list.\n");
        return -1;
      }
    }
    prev->next = thr->next;

    if (thread_q == thr) {
      thread_q = thr->next;
    }
  }

  thr->finished = 1;
  thr->avl = 0;

  printf("Thread %p destroyed\n", (void *)thr);
  return 1;
}

int mythreads_cleanup() {
  // Stop the timer
  timer.it_value.tv_sec = 0;
  timer.it_value.tv_usec = 0;
  timer.it_interval.tv_sec = 0;
  timer.it_interval.tv_usec = 0;
  setitimer(ITIMER_REAL, &timer, NULL);

  // Reset signal handler to default
  sa.sa_handler = SIG_DFL;
  sigaction(SIGALRM, &sa, NULL);

  // Reset global state
  curr = NULL;
  thread_q = NULL;

  return 1;
}

int mythreads_sem_create(mysem_t *s, int n) {
  if (n != 0 && n != 1) {
    printf("n must be 0 or 1\n");
    return 0;
  }

  if (s->initialized) {
    printf("Semaphore already exists.\n");
    return -1;
  }

  semid_counter++;
  s->semid = semid_counter;
  s->initialized = 1;

  printf("Initialized semaphore, semid: %d, initial value: %d\n", s->semid, n);

  s->val = n;
  s->blocked_queue = NULL;
  s->queue_tail = NULL;

  return 1;
}

int mythreads_sem_down(mysem_t *s) {
  if (!s->initialized) {
    return -1;
  }

  s->val--;

  if (s->val < 0) {
    node_t *new_node = (node_t *)malloc(sizeof(node_t));
    if (!new_node) {
      fprintf(stderr, "Error: Memory allocation failed in sem_down.\n");
      return -1;
    }

    new_node->thread = curr;
    new_node->next = NULL;

    if (s->queue_tail) {
      s->queue_tail->next = new_node;
    } else {
      s->blocked_queue = new_node;
    }
    s->queue_tail = new_node;

    curr->avl = 0;
    mythreads_yield();
  }

  return 1;
}

int mythreads_sem_up(mysem_t *s) {
  if (!s->initialized) {
    return -1;
  }

  s->val++;

  if (s->val <= 0 && s->blocked_queue) {
    node_t *node_to_unblock = s->blocked_queue;
    mythr_t *thread_to_unblock = node_to_unblock->thread;

    s->blocked_queue = node_to_unblock->next;
    if (!s->blocked_queue) {
      s->queue_tail = NULL;
    }

    free(node_to_unblock);

    thread_to_unblock->avl = 1;

    mythreads_yield();
  }

  return 1;
}

int mythreads_sem_destroy(mysem_t *s) {
  if (!s->initialized) {
    return -1;
  }

  node_t *current = s->blocked_queue;
  while (current) {
    node_t *temp = current;
    current = current->next;
    free(temp);
  }

  s->blocked_queue = NULL;
  s->queue_tail = NULL;
  s->val = 0;
  s->initialized = 0;

  return 1;
}
