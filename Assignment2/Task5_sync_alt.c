/* Q5 alternative concept: protect a shared counter with
 *   mutex (sleeping lock) | spin lock | C11 atomic_fetch_add
 * and measure time per update and CPU time wasted, for 1, 4 and 16 threads,
 * with a SHORT critical section (one increment) and a LONG one (about 2 microseconds of work).
 * Compile: gcc -O2 -std=gnu11 -Wall -Wextra -pthread sync_alt.c -o sync_alt
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>

static long counter;
static atomic_long acounter;
static pthread_mutex_t mtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_spinlock_t spin;
static int updates, work_iters;
enum { MUTEX, SPIN, ATOMIC };
static int method;

static double wall(void) { struct timespec t; clock_gettime(CLOCK_MONOTONIC, &t); return t.tv_sec + t.tv_nsec / 1e9; }
static double cpu(void)  { struct timespec t; clock_gettime(CLOCK_PROCESS_CPUTIME_ID, &t); return t.tv_sec + t.tv_nsec / 1e9; }

static inline void cs_work(void)
{
    volatile int w = 0;
    for (int j = 0; j < work_iters; j++) w += j;
}

static void *worker(void *a)
{
    (void)a;
    for (int i = 0; i < updates; i++) {
        switch (method) {
        case MUTEX:  pthread_mutex_lock(&mtx);  cs_work(); counter++; pthread_mutex_unlock(&mtx); break;
        case SPIN:   pthread_spin_lock(&spin);  cs_work(); counter++; pthread_spin_unlock(&spin); break;
        default:     cs_work(); atomic_fetch_add(&acounter, 1); break;   /* work cannot be inside an atomic op */
        }
    }
    return NULL;
}

int main(void)
{
    pthread_spin_init(&spin, PTHREAD_PROCESS_PRIVATE);
    const char *mn[] = {"mutex", "spin lock", "atomic add"};
    int threads[] = {1, 4, 16};
    for (int pass = 0; pass < 2; pass++) {
        work_iters = pass == 0 ? 0 : 2000;
        updates = pass == 0 ? 200000 : 10000;
        printf("\n%s critical section (%d iterations of work), %d updates per thread\n",
               pass == 0 ? "SHORT" : "LONG", work_iters, updates);
        printf("%-11s %7s %14s %12s %16s\n", "method", "threads", "ns / update", "wall (s)", "CPU / wall");
        for (int m = 0; m < 3; m++) for (int ti = 0; ti < 3; ti++) {
            int T = threads[ti];
            method = m; counter = 0; atomic_store(&acounter, 0);
            pthread_t th[16];
            double w0 = wall(), c0 = cpu();
            for (int i = 0; i < T; i++) pthread_create(&th[i], NULL, worker, NULL);
            for (int i = 0; i < T; i++) pthread_join(th[i], NULL);
            double w = wall() - w0, c = cpu() - c0;
            long got = m == ATOMIC ? atomic_load(&acounter) : counter;
            if (got != (long)T * updates) printf("  WRONG COUNT %ld\n", got);
            printf("%-11s %7d %14.1f %12.3f %16.2f\n", mn[m], T, w * 1e9 / ((double)T * updates), w, c / w);
        }
    }
    return 0;
}
