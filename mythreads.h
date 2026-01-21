// mysem.h
#ifndef MYTHREADS_H
#define MYTHREADS_H
#include <errno.h>
#include <pthread.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <ucontext.h>
#include <unistd.h>

#define MYTHREADS_STACK_SIZE (64 * 1024) // 64KB stack size

typedef struct {
  ucontext_t co;
  char stack[MYTHREADS_STACK_SIZE];
} co_t;

typedef struct mythr_t {
  co_t thr;
  int finished;
  int avl;
  struct mythr_t *next;
  int sleep;
  struct timeval sleep_until; // Changed to timeval for precise timing
} mythr_t;

typedef struct node {
  mythr_t *thread;
  struct node *next;
} node_t;

typedef struct mysem {
  int semid;
  int initialized;
  int val;
  node_t *blocked_queue;
  node_t *queue_tail;
} mysem_t;

// Structure to pass both the user function and argument to the wrapper
typedef struct {
  void (*body)(void *);
  void *arg;
  mythr_t *thread;
} thread_wrapper_arg_t;

int mythreads_init(mythr_t *main);

int mythreads_create(mythr_t *thr, void(body)(void *), void *arg);

int mythreads_sleep(int seconds);

int mythreads_yield();

int mythreads_join(mythr_t *thr);

int mythreads_destroy(mythr_t *thr);

int mythreads_cleanup();
////////////////////////////////////////
int mycoroutines_init(co_t *main);

int mycoroutines_create(co_t *co, void(body)(void *), void *arg, co_t *link);

int mycoroutines_switchto(co_t *co, co_t *switchto);

int mycoroutines_destroy(co_t *co);

//////////////////////////////////////

int mythreads_sem_create(mysem_t *s, int n);

int mythreads_sem_down(mysem_t *s);

int mythreads_sem_up(mysem_t *s);

int mythreads_sem_destroy(mysem_t *s);

#endif // MYSEM_H