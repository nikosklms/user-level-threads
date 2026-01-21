# MyThreads - User-Level Threading Library

A lightweight user-level threading library implementation in C, featuring cooperative and preemptive scheduling, coroutines, and semaphores.

## Features

- **User-Level Threads** - Lightweight threads managed entirely in user space
- **Preemptive Scheduling** - Round-robin scheduler with 1ms time slices
- **Coroutines** - Low-level context switching using `ucontext`
- **Thread Sleep** - Precise microsecond-resolution sleep
- **Semaphores** - Binary semaphores for synchronization

## Files

| File | Description |
|------|-------------|
| `mythreads.h` | Header file with all type definitions and function declarations |
| `mythreads.c` | Implementation of the threading library |
| `test_mythreads.c` | Comprehensive test suite |
| `Makefile` | Build configuration |

## Building

```bash
make clean && make
```

## Running Tests

```bash
./test_mythreads
```

Or simply:
```bash
make test
```

## API Reference

### Thread Functions

```c
int mythreads_init(mythr_t *main);           // Initialize the threading system
int mythreads_create(mythr_t *thr, void (body)(void*), void *arg);  // Create a new thread
int mythreads_yield();                        // Yield CPU to another thread
int mythreads_sleep(int seconds);             // Sleep for specified seconds
int mythreads_join(mythr_t *thr);            // Wait for thread to finish
int mythreads_destroy(mythr_t *thr);         // Destroy a thread
int mythreads_cleanup();                      // Clean up the threading system
```

### Coroutine Functions

```c
int mycoroutines_init(co_t *main);
int mycoroutines_create(co_t *co, void (body)(void *), void *arg, co_t *link);
int mycoroutines_switchto(co_t *co, co_t *switchto);
int mycoroutines_destroy(co_t *co);
```

### Semaphore Functions

```c
int mythreads_sem_create(mysem_t *s, int n);  // Create semaphore (n = 0 or 1)
int mythreads_sem_down(mysem_t *s);           // P operation (wait)
int mythreads_sem_up(mysem_t *s);             // V operation (signal)
int mythreads_sem_destroy(mysem_t *s);        // Destroy semaphore
```

## Example Usage

```c
#include "mythreads.h"

void my_thread_func(void *arg) {
    int id = *(int *)arg;
    printf("Hello from thread %d\n", id);
}

int main() {
    mythr_t main_thread, t1;
    int id = 1;
    
    mythreads_init(&main_thread);
    mythreads_create(&t1, my_thread_func, &id);
    mythreads_join(&t1);
    mythreads_cleanup();
    
    return 0;
}
```

## License

This project is for educational purposes.
