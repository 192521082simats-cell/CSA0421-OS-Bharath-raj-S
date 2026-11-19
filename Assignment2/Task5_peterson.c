/* Q5 - Peterson's solution for two threads, and why it can fail on modern processors.
 *   version 1: plain variables (volatile) - the textbook code. Store->load reordering (store buffer)
 *              on x86/ARM lets BOTH threads see the other's flag as 0 and enter together.
 *   version 2: C11 sequentially-consistent atomics (full fences) - correct.
 * Compile: gcc -O2 -std=gnu11 -Wall -Wextra -pthread peterson.c -o peterson
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <sched.h>
#include <stdatomic.h>

#define N 3000000

static volatile int flag_v[2], turn_v;
static atomic_int  flag_a[2], turn_a;
static volatile long cnt_v, cnt_a;

static void pin(int cpu)
{
    cpu_set_t s; CPU_ZERO(&s); CPU_SET(cpu, &s);
    pthread_setaffinity_np(pthread_self(), sizeof s, &s);
}

static void *t_plain(void *a)
{
    int me = (int)(long)a, other = 1 - me;
    pin(me);
    for (int i = 0; i < N; i++) {
        flag_v[me] = 1; turn_v = other;
        while (flag_v[other] && turn_v == other) ;
        cnt_v++;                                   /* critical section */
        flag_v[me] = 0;
    }
    return NULL;
}
static void *t_atomic(void *a)
{
    int me = (int)(long)a, other = 1 - me;
    pin(me);
    for (int i = 0; i < N; i++) {
        atomic_store(&flag_a[me], 1); atomic_store(&turn_a, other);
        while (atomic_load(&flag_a[other]) && atomic_load(&turn_a) == other) ;
        cnt_a++;
        atomic_store(&flag_a[me], 0);
    }
    return NULL;
}

int main(void)
{
    pthread_t t[2];
    for (long i = 0; i < 2; i++) pthread_create(&t[i], NULL, t_plain, (void *)i);
    for (int i = 0; i < 2; i++) pthread_join(t[i], NULL);
    printf("Peterson, plain variables : counter = %ld, expected %d, lost %ld\n", (long)cnt_v, 2 * N, 2L * N - cnt_v);
    for (long i = 0; i < 2; i++) pthread_create(&t[i], NULL, t_atomic, (void *)i);
    for (int i = 0; i < 2; i++) pthread_join(t[i], NULL);
    printf("Peterson, seq_cst atomics : counter = %ld, expected %d, lost %ld\n", (long)cnt_a, 2 * N, 2L * N - cnt_a);
    return 0;
}
