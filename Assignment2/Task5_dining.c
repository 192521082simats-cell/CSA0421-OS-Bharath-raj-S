/* Q5 - Dining philosophers with semaphores.
 *   default : at most N-1 philosophers may try to pick up forks at once (a counting semaphore "room")
 *             -> circular wait is impossible -> no deadlock.
 *   -DNAIVE : every philosopher grabs the left fork then the right fork -> can deadlock.
 * Compile: gcc -O2 -std=gnu11 -Wall -Wextra -pthread dining.c -o dining
 *          gcc -O2 -std=gnu11 -Wall -Wextra -pthread -DNAIVE dining.c -o dining_naive
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <pthread.h>
#include <semaphore.h>

#define N 5
#define MEALS 2000

static sem_t fork_sem[N], room;
static int meals[N];

static void *philosopher(void *arg)
{
    int i = (int)(long)arg, l = i, r = (i + 1) % N;
    for (int m = 0; m < MEALS; m++) {
#ifndef NAIVE
        sem_wait(&room);
#endif
        sem_wait(&fork_sem[l]);
#ifdef NAIVE
        usleep(100);                 /* widen the window in which everyone holds one fork */
#endif
        sem_wait(&fork_sem[r]);
        meals[i]++;                  /* eat */
        sem_post(&fork_sem[r]);
        sem_post(&fork_sem[l]);
#ifndef NAIVE
        sem_post(&room);
#endif
    }
    return NULL;
}

int main(void)
{
    pthread_t th[N];
    for (int i = 0; i < N; i++) sem_init(&fork_sem[i], 0, 1);
    sem_init(&room, 0, N - 1);
    for (long i = 0; i < N; i++) pthread_create(&th[i], NULL, philosopher, (void *)i);
    for (int i = 0; i < N; i++) pthread_join(th[i], NULL);
    int total = 0;
    for (int i = 0; i < N; i++) total += meals[i];
    printf("all philosophers finished: %d meals eaten (expected %d)\n", total, N * MEALS);
    return 0;
}
