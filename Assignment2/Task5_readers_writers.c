/* Q5 - Readers-writers with semaphores (first readers-writers problem: readers have preference).
 *   mutex : protects read_count
 *   wrt   : gives writers exclusive access (the first reader locks it out, the last reader releases it)
 * A writer sets x then y to the same new value; a reader that sees x != y has observed a half-finished write.
 * Compile: gcc -O2 -std=gnu11 -Wall -Wextra -pthread readers_writers.c -o readers_writers
 *          add -DUNSAFE to skip the protocol and watch inconsistent reads appear.
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <pthread.h>
#include <semaphore.h>

#define READERS 4
#define WRITERS 2
#define ROUNDS  200000

static sem_t mutex, wrt;
static int read_count;
static volatile long x, y;
static long bad_reads[READERS], writes_done[WRITERS];

static void *reader(void *a)
{
    long id = (long)a;
    for (int i = 0; i < ROUNDS; i++) {
#ifndef UNSAFE
        sem_wait(&mutex);
        if (++read_count == 1) sem_wait(&wrt);
        sem_post(&mutex);
#endif
        long a1 = x, b1 = y;
        if (a1 != b1) bad_reads[id]++;
#ifndef UNSAFE
        sem_wait(&mutex);
        if (--read_count == 0) sem_post(&wrt);
        sem_post(&mutex);
#endif
    }
    return NULL;
}
static void *writer(void *a)
{
    long id = (long)a;
    for (int i = 0; i < ROUNDS / 10; i++) {
#ifndef UNSAFE
        sem_wait(&wrt);
#endif
        long v = x + 1; x = v; y = v;
        writes_done[id]++;
#ifndef UNSAFE
        sem_post(&wrt);
#endif
    }
    return NULL;
}
int main(void)
{
    sem_init(&mutex, 0, 1); sem_init(&wrt, 0, 1);
    pthread_t r[READERS], w[WRITERS];
    for (long i = 0; i < READERS; i++) pthread_create(&r[i], NULL, reader, (void *)i);
    for (long i = 0; i < WRITERS; i++) pthread_create(&w[i], NULL, writer, (void *)i);
    for (int i = 0; i < READERS; i++) pthread_join(r[i], NULL);
    for (int i = 0; i < WRITERS; i++) pthread_join(w[i], NULL);
    long bad = 0, wr = 0;
    for (int i = 0; i < READERS; i++) bad += bad_reads[i];
    for (int i = 0; i < WRITERS; i++) wr += writes_done[i];
#ifdef UNSAFE
    const char *mode = "NO protocol";
#else
    const char *mode = "semaphore protocol";
#endif
    printf("[%s] %d readers x %d reads, %ld writes: inconsistent reads = %ld, final x=%ld (writes done=%ld)\n",
           mode, READERS, ROUNDS, wr, bad, (long)x, wr);
    return 0;
}
