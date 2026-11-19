/* Q5 - the same bounded buffer written as a MONITOR (one mutex + two condition variables).
 * Compare with the semaphore version: the monitor keeps the data and the rules together and
 * the "wait while condition is false" loop is the only thing the programmer must remember.
 * Compile: gcc -O2 -std=gnu11 -Wall -Wextra -pthread monitor_buffer.c -o monitor_buffer
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>

#define SIZE 5
#define PER_PRODUCER 20000
#define P 4
#define C 4

typedef struct {
    int buf[SIZE], in, out, count;
    pthread_mutex_t m;
    pthread_cond_t not_full, not_empty;
} monitor_t;
static monitor_t mon = { .m = PTHREAD_MUTEX_INITIALIZER,
                         .not_full = PTHREAD_COND_INITIALIZER,
                         .not_empty = PTHREAD_COND_INITIALIZER };
static atomic_int seen[P * PER_PRODUCER + 1];

static void put(int item)
{
    pthread_mutex_lock(&mon.m);
    while (mon.count == SIZE) pthread_cond_wait(&mon.not_full, &mon.m);
    mon.buf[mon.in] = item; mon.in = (mon.in + 1) % SIZE; mon.count++;
    pthread_cond_signal(&mon.not_empty);
    pthread_mutex_unlock(&mon.m);
}
static int get(void)
{
    pthread_mutex_lock(&mon.m);
    while (mon.count == 0) pthread_cond_wait(&mon.not_empty, &mon.m);
    int item = mon.buf[mon.out]; mon.out = (mon.out + 1) % SIZE; mon.count--;
    pthread_cond_signal(&mon.not_full);
    pthread_mutex_unlock(&mon.m);
    return item;
}
static void *producer(void *a)
{
    long id = (long)a;
    for (int i = 1; i <= PER_PRODUCER; i++) put((int)(id * PER_PRODUCER + i));
    return NULL;
}
static void *consumer(void *a)
{
    (void)a;
    for (int k = 0; k < P * PER_PRODUCER / C; k++) atomic_fetch_add(&seen[get()], 1);
    return NULL;
}
int main(void)
{
    pthread_t pt[P], ct[C];
    for (long i = 0; i < P; i++) pthread_create(&pt[i], NULL, producer, (void *)i);
    for (int i = 0; i < C; i++) pthread_create(&ct[i], NULL, consumer, NULL);
    for (int i = 0; i < P; i++) pthread_join(pt[i], NULL);
    for (int i = 0; i < C; i++) pthread_join(ct[i], NULL);
    int once = 0;
    for (int i = 1; i <= P * PER_PRODUCER; i++) if (seen[i] == 1) once++;
    printf("[monitor] %d producers, %d consumers, %d items: consumed exactly once=%d  => %s\n",
           P, C, P * PER_PRODUCER, once, once == P * PER_PRODUCER ? "CORRECT" : "WRONG");
    return 0;
}
