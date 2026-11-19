/* Q5 test of the design: bounded buffer with P producers and C consumers.
 * Every item gets a unique id; the program proves each id was consumed EXACTLY once.
 * Build with the mutex:    gcc -O2 -std=gnu11 -Wall -Wextra -pthread bb_multi.c -o bb_multi
 * Build WITHOUT the mutex: gcc -O2 -std=gnu11 -Wall -Wextra -pthread -DNO_MUTEX bb_multi.c -o bb_multi_nomutex
 * Run: ./bb_multi <producers> <consumers>
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <semaphore.h>
#include <stdatomic.h>

#define SIZE 5
#define PER_PRODUCER 20000

static int buffer[SIZE], in = 0, out = 0;
static sem_t empty, full;
static pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
static atomic_int *seen;
static int P, C;

#ifdef NO_MUTEX
#define LOCK()
#define UNLOCK()
#else
#define LOCK()   pthread_mutex_lock(&lock)
#define UNLOCK() pthread_mutex_unlock(&lock)
#endif

static void *producer(void *arg)
{
    long id = (long)arg;
    for (int i = 1; i <= PER_PRODUCER; i++) {
        int item = (int)(id * PER_PRODUCER + i);      /* unique, 1..P*PER_PRODUCER */
        sem_wait(&empty);
        LOCK();
        buffer[in] = item;
        in = (in + 1) % SIZE;
        UNLOCK();
        sem_post(&full);
    }
    return NULL;
}

static void *consumer(void *arg)
{
    (void)arg;
    int quota = P * PER_PRODUCER / C;
    for (int k = 0; k < quota; k++) {
        sem_wait(&full);
        LOCK();
        int item = buffer[out];
        out = (out + 1) % SIZE;
        UNLOCK();
        sem_post(&empty);
        if (item >= 1 && item <= P * PER_PRODUCER) atomic_fetch_add(&seen[item], 1);
    }
    return NULL;
}

int main(int argc, char **argv)
{
    P = argc > 1 ? atoi(argv[1]) : 1;
    C = argc > 2 ? atoi(argv[2]) : 1;
    if ((P * PER_PRODUCER) % C) { fprintf(stderr, "P*%d must be divisible by C\n", PER_PRODUCER); return 1; }
    int total = P * PER_PRODUCER;
    seen = calloc(total + 1, sizeof *seen);
    sem_init(&empty, 0, SIZE);
    sem_init(&full, 0, 0);
    pthread_t pt[64], ct[64];
    for (long i = 0; i < P; i++) pthread_create(&pt[i], NULL, producer, (void *)i);
    for (long i = 0; i < C; i++) pthread_create(&ct[i], NULL, consumer, NULL);
    for (int i = 0; i < P; i++) pthread_join(pt[i], NULL);
    for (int i = 0; i < C; i++) pthread_join(ct[i], NULL);
    int once = 0, missing = 0, dup = 0;
    for (int i = 1; i <= total; i++) {
        if (seen[i] == 1) once++;
        else if (seen[i] == 0) missing++;
        else dup++;
    }
#ifdef NO_MUTEX
    const char *mode = "NO mutex";
#else
    const char *mode = "with mutex";
#endif
    printf("[%s] %d producers, %d consumers, %d items: consumed exactly once=%d, never consumed=%d, consumed more than once=%d  => %s\n",
           mode, P, C, total, once, missing, dup, (once == total) ? "CORRECT" : "WRONG");
    return 0;
}
