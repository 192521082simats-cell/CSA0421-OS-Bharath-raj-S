/* Q2 alternative concept: cost of ONE scheduling decision.
 * SJF must scan all n ready jobs for the shortest burst  -> O(n)
 * MLFQ looks at the head of the first non-empty of K queues -> O(K) = O(1)
 * Compile: gcc -O2 -std=c11 -Wall -Wextra sched_cost.c -o sched_cost
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#define K 3
static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

int main(void)
{
    int sizes[] = {10, 100, 1000, 10000, 100000};
    printf("%8s %18s %18s\n", "ready n", "SJF pick (ns)", "MLFQ pick (ns)");
    for (int s = 0; s < 5; s++) {
        int n = sizes[s];
        int *burst = malloc(n * sizeof *burst);
        for (int i = 0; i < n; i++) burst[i] = 1 + rand() % 1000;
        /* MLFQ: K queues, each stored as an array with a head index */
        int qhead[K] = {0, 0, 0}, qlen[K] = {0, n / 2, n - n / 2};
        int reps = 2000000 / (n < 1000 ? 1 : n / 100 + 1);
        volatile int sink = 0;

        double t0 = now();
        for (int r = 0; r < reps; r++) {
            int best = 0;
            for (int i = 1; i < n; i++) if (burst[i] < burst[best]) best = i;
            sink += best;
            burst[r % n] ^= 1;        /* keep the compiler honest */
        }
        double sjf = (now() - t0) / reps * 1e9;

        t0 = now();
        for (int r = 0; r < reps; r++) {
            int q = 0;
            while (q < K - 1 && qlen[q] - qhead[q] == 0) q++;
            sink += q;
            qhead[q] ^= 0;
        }
        double mlfq = (now() - t0) / reps * 1e9;
        printf("%8d %18.1f %18.1f\n", n, sjf, mlfq);
        free(burst);
        (void)sink;
    }
    return 0;
}
