/* Q5 deployment decision: lock granularity for a railway booking system.
 *   none      - no locking at all (shows the double-booking bug)
 *   global    - ONE lock for the whole system
 *   per-train - one lock per train
 *   per-coach - one lock per coach
 * Each operation picks a random seat; if it is free it is booked, otherwise the booking is cancelled
 * (so the system stays in a steady state). Pricing work is simulated inside the critical section.
 * Correctness check: bookings - cancellations must equal the number of occupied seats at the end.
 * Compile: gcc -O2 -std=gnu11 -Wall -Wextra -pthread ticket_locks.c -o ticket_locks
 * Run:     ./ticket_locks <threads>
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#define TRAINS  16
#define COACHES 8
#define SEATS   40
#define OPS     300000
#define WORK    300

enum { NONE, GLOBAL, TRAIN, COACH };
static int seat[TRAINS][COACHES][SEATS];          /* 0 = free, otherwise booked */
static pthread_mutex_t locks[TRAINS * COACHES];
static int mode;
static long booked[64], cancelled[64];

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}
static unsigned rnd(unsigned *s) { *s ^= *s << 13; *s ^= *s >> 17; *s ^= *s << 5; return *s; }

static void *agent(void *arg)
{
    long id = (long)arg;
    unsigned s = 2463534242u + (unsigned)id * 7919u;
    for (int i = 0; i < OPS; i++) {
        int t = rnd(&s) % TRAINS, c = rnd(&s) % COACHES, k = rnd(&s) % SEATS;
        int li = mode == GLOBAL ? 0 : mode == TRAIN ? t * COACHES : t * COACHES + c;
        if (mode != NONE) pthread_mutex_lock(&locks[li]);
        volatile int w = 0;
        for (int j = 0; j < WORK; j++) w += j;                 /* price lookup, payment check ... */
        if (seat[t][c][k] == 0) { seat[t][c][k] = (int)id + 1; booked[id]++; }
        else                    { seat[t][c][k] = 0;           cancelled[id]++; }
        if (mode != NONE) pthread_mutex_unlock(&locks[li]);
    }
    return NULL;
}

static double run(int threads, int m, long *mismatch)
{
    mode = m;
    memset(seat, 0, sizeof seat); memset(booked, 0, sizeof booked); memset(cancelled, 0, sizeof cancelled);
    for (int i = 0; i < TRAINS * COACHES; i++) pthread_mutex_init(&locks[i], NULL);
    pthread_t th[64];
    double t0 = now();
    for (long i = 0; i < threads; i++) pthread_create(&th[i], NULL, agent, (void *)i);
    for (int i = 0; i < threads; i++) pthread_join(th[i], NULL);
    double t = now() - t0;
    long net = 0, occ = 0;
    for (int i = 0; i < threads; i++) net += booked[i] - cancelled[i];
    for (int a = 0; a < TRAINS; a++) for (int b = 0; b < COACHES; b++) for (int c = 0; c < SEATS; c++) occ += seat[a][b][c] != 0;
    *mismatch = net - occ;
    return (double)threads * OPS / t;
}

int main(int argc, char **argv)
{
    int threads = argc > 1 ? atoi(argv[1]) : 8;
    const char *name[] = {"no lock", "one global lock", "one lock per train", "one lock per coach"};
    printf("%d threads, %d operations each, %d trains x %d coaches x %d seats\n", threads, OPS, TRAINS, COACHES, SEATS);
    printf("%-22s %14s   %s\n", "locking", "ops/second", "bookkeeping error (double sales)");
    for (int m = NONE; m <= COACH; m++) {
        long bad;
        double ops = run(threads, m, &bad);
        printf("%-22s %14.0f   %ld\n", name[m], ops, bad);
    }
    return 0;
}
