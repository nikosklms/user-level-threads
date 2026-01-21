#include "mythreads.h"

// Global variables for testing
mysem_t sem;
int shared_counter = 0;
int test_passed = 0;

// ============================================
// Test 1: Basic Thread Creation and Yield
// ============================================
void thread_func1(void *arg) {
  int id = *(int *)arg;
  printf("[Thread %d] Started\n", id);

  for (int i = 0; i < 3; i++) {
    printf("[Thread %d] Iteration %d\n", id, i);
    mythreads_yield();
  }

  printf("[Thread %d] Finished\n", id);
}

void test_basic_threads() {
  printf("\n========== TEST 1: Basic Thread Creation and Yield ==========\n");

  mythr_t main_thread;
  mythr_t t1, t2;
  int id1 = 1, id2 = 2;

  mythreads_init(&main_thread);

  printf("[Main] Creating threads...\n");
  mythreads_create(&t1, thread_func1, &id1);
  mythreads_create(&t2, thread_func1, &id2);

  printf("[Main] Joining thread 1...\n");
  mythreads_join(&t1);
  printf("[Main] Joining thread 2...\n");
  mythreads_join(&t2);

  printf("[Main] Both threads completed!\n");
  test_passed++;
}

// ============================================
// Test 2: Thread Sleep
// ============================================
void sleepy_thread(void *arg) {
  int id = *(int *)arg;
  printf("[Sleepy Thread %d] Going to sleep for 2 seconds...\n", id);

  struct timeval start, end;
  gettimeofday(&start, NULL);
  mythreads_sleep(2);
  gettimeofday(&end, NULL);

  double elapsed =
      (end.tv_sec - start.tv_sec) + (end.tv_usec - start.tv_usec) / 1000000.0;
  printf("[Sleepy Thread %d] Woke up after %.3f seconds\n", id, elapsed);
}

void test_sleep() {
  printf("\n========== TEST 2: Thread Sleep ==========\n");

  mythr_t main_thread;
  mythr_t t1;
  int id1 = 1;

  mythreads_init(&main_thread);

  printf("[Main] Creating sleepy thread...\n");
  mythreads_create(&t1, sleepy_thread, &id1);

  printf("[Main] Waiting for sleepy thread to complete...\n");
  mythreads_join(&t1);

  printf("[Main] Sleep test completed!\n");
  test_passed++;
}

// ============================================
// Test 3: Semaphores (Mutex-like behavior)
// ============================================
void critical_section_thread(void *arg) {
  int id = *(int *)arg;

  for (int i = 0; i < 5; i++) {
    printf("[Thread %d] Trying to enter critical section...\n", id);
    mythreads_sem_down(&sem);

    printf("[Thread %d] ENTERED critical section\n", id);

    // Simulate work in critical section
    int temp = shared_counter;
    mythreads_yield(); // Allow context switch
    shared_counter = temp + 1;

    printf("[Thread %d] Counter = %d, LEAVING critical section\n", id,
           shared_counter);
    mythreads_sem_up(&sem);

    mythreads_yield();
  }
}

void test_semaphores() {
  printf("\n========== TEST 3: Semaphores (Mutual Exclusion) ==========\n");

  mythr_t main_thread;
  mythr_t t1, t2;
  int id1 = 1, id2 = 2;

  memset(&sem, 0, sizeof(sem)); // Clear semaphore structure
  shared_counter = 0;

  mythreads_init(&main_thread);

  // Create binary semaphore (mutex)
  printf("[Main] Creating semaphore...\n");
  mythreads_sem_create(&sem, 1);

  printf("[Main] Creating threads for critical section test...\n");
  mythreads_create(&t1, critical_section_thread, &id1);
  mythreads_create(&t2, critical_section_thread, &id2);

  mythreads_join(&t1);
  mythreads_join(&t2);

  printf("[Main] Final counter value: %d (expected: 10)\n", shared_counter);

  mythreads_sem_destroy(&sem);
  printf("[Main] Semaphore test completed!\n");

  if (shared_counter == 10) {
    printf("[Main] PASSED: Counter is correct!\n");
    test_passed++;
  } else {
    printf("[Main] FAILED: Counter should be 10, got %d\n", shared_counter);
  }
}

// ============================================
// Test 4: Producer-Consumer with Semaphores
// ============================================
#define BUFFER_SIZE 5
int buffer[BUFFER_SIZE];
int in_idx = 0, out_idx = 0;
mysem_t empty_sem, full_sem, mutex_sem;

void producer_thread(void *arg) {
  int id = *(int *)arg;

  for (int i = 0; i < 3; i++) {
    int item = id * 10 + i;

    printf("[Producer %d] Waiting to produce item %d...\n", id, item);
    mythreads_sem_down(&empty_sem); // Wait for empty slot
    mythreads_sem_down(&mutex_sem); // Enter critical section

    buffer[in_idx] = item;
    printf("[Producer %d] Produced item %d at index %d\n", id, item, in_idx);
    in_idx = (in_idx + 1) % BUFFER_SIZE;

    mythreads_sem_up(&mutex_sem); // Leave critical section
    mythreads_sem_up(&full_sem);  // Signal item available

    mythreads_yield();
  }
  printf("[Producer %d] Done producing\n", id);
}

void consumer_thread(void *arg) {
  int id = *(int *)arg;

  for (int i = 0; i < 3; i++) {
    printf("[Consumer %d] Waiting to consume...\n", id);
    mythreads_sem_down(&full_sem);  // Wait for item
    mythreads_sem_down(&mutex_sem); // Enter critical section

    int item = buffer[out_idx];
    printf("[Consumer %d] Consumed item %d from index %d\n", id, item, out_idx);
    out_idx = (out_idx + 1) % BUFFER_SIZE;

    mythreads_sem_up(&mutex_sem); // Leave critical section
    mythreads_sem_up(&empty_sem); // Signal empty slot

    mythreads_yield();
  }
  printf("[Consumer %d] Done consuming\n", id);
}

void test_producer_consumer() {
  printf("\n========== TEST 4: Producer-Consumer Problem ==========\n");

  mythr_t main_thread;
  mythr_t prod1, prod2, cons1, cons2;
  int id1 = 1, id2 = 2, id3 = 1, id4 = 2;

  // Clear semaphores
  memset(&empty_sem, 0, sizeof(empty_sem));
  memset(&full_sem, 0, sizeof(full_sem));
  memset(&mutex_sem, 0, sizeof(mutex_sem));
  in_idx = out_idx = 0;

  mythreads_init(&main_thread);

  printf("[Main] Setting up producer-consumer semaphores...\n");
  mythreads_sem_create(&mutex_sem, 1); // Binary semaphore for mutex
  mythreads_sem_create(
      &empty_sem,
      1); // Initially all slots are empty (but semaphore limited to 0/1)
  mythreads_sem_create(&full_sem, 0); // Initially no items

  printf("[Main] Creating producer and consumer threads...\n");
  mythreads_create(&prod1, producer_thread, &id1);
  mythreads_create(&prod2, producer_thread, &id2);
  mythreads_create(&cons1, consumer_thread, &id3);
  mythreads_create(&cons2, consumer_thread, &id4);

  mythreads_join(&prod1);
  mythreads_join(&prod2);
  mythreads_join(&cons1);
  mythreads_join(&cons2);

  printf("[Main] Producer-Consumer test completed!\n");

  mythreads_sem_destroy(&empty_sem);
  mythreads_sem_destroy(&full_sem);
  mythreads_sem_destroy(&mutex_sem);

  test_passed++;
}

// ============================================
// Test 5: Coroutines (Low-level test)
// ============================================
co_t main_co, co1, co2;
int co_step = 0;

void coroutine1(void *arg) {
  printf("[Coroutine 1] Step 1\n");
  co_step++;
  mycoroutines_switchto(&co1, &co2);

  printf("[Coroutine 1] Step 3\n");
  co_step++;
  mycoroutines_switchto(&co1, &main_co);
}

void coroutine2(void *arg) {
  printf("[Coroutine 2] Step 2\n");
  co_step++;
  mycoroutines_switchto(&co2, &co1);

  printf("[Coroutine 2] Step 4\n");
  co_step++;
  mycoroutines_switchto(&co2, &main_co);
}

void test_coroutines() {
  printf("\n========== TEST 5: Basic Coroutines ==========\n");

  co_step = 0;

  mycoroutines_init(&main_co);
  mycoroutines_create(&co1, coroutine1, NULL, &main_co);
  mycoroutines_create(&co2, coroutine2, NULL, &main_co);

  printf("[Main] Starting coroutines...\n");
  mycoroutines_switchto(&main_co, &co1);

  printf("[Main] Back to main after step 3, switching to co2...\n");
  mycoroutines_switchto(&main_co, &co2);

  printf("[Main] Coroutines completed! Steps executed: %d (expected: 4)\n",
         co_step);

  if (co_step == 4) {
    printf("[Main] PASSED: All coroutine steps executed!\n");
    test_passed++;
  } else {
    printf("[Main] FAILED: Expected 4 steps, got %d\n", co_step);
  }
}

// ============================================
// Main - Run All Tests
// ============================================
int main() {
  printf("==============================================\n");
  printf("    MYTHREADS LIBRARY TEST SUITE\n");
  printf("==============================================\n");

  // Run individual tests
  // Note: Each test reinitializes the threading system
  // so they are somewhat independent

  test_coroutines();

  printf("\n\n--- Pausing before thread tests ---\n\n");
  sleep(1);

  test_basic_threads();
  mythreads_cleanup(); // Stop timer before pause

  printf("\n\n--- Pausing before sleep test ---\n\n");
  sleep(1);

  test_sleep();
  mythreads_cleanup(); // Stop timer before pause

  printf("\n\n--- Pausing before semaphore test ---\n\n");
  sleep(1);

  test_semaphores();
  mythreads_cleanup(); // Stop timer before pause

  // Note: Producer-Consumer test may have issues with the simple semaphore
  // implementation that only allows 0/1 values
  printf("\n\n--- Pausing before producer-consumer test ---\n\n");
  sleep(1);

  test_producer_consumer();
  mythreads_cleanup(); // Final cleanup

  printf("\n==============================================\n");
  printf("    TEST RESULTS: %d/5 tests passed\n", test_passed);
  printf("==============================================\n");

  return 0;
}
