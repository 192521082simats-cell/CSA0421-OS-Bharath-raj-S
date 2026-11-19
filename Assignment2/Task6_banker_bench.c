/* Q6 D + E: predict and measure the running time of the Banker's safety algorithm.
 * PREDICTION: worst case O(m * n^2): up to n passes of the outer loop, each scanning n processes,
 *             each test comparing m resources. Best case (every process fits in the first pass) O(m * n).
 * Worst-case state built here: process n-1-k can only finish after k other processes have released
 * their resources, so each pass finishes exactly one process.
 * Compile: gcc -O2 -std=c11 -Wall -Wextra banker_bench.c -o banker_bench
 */
#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

/* generic safety algorithm; returns 1 if safe; *ops counts resource comparisons */
static int is_safe(int n, int m, const int *avail, const int *need, const int *alloc, long *ops)
{
    int *work = malloc(m * sizeof *work);
    char *finish = calloc(n, 1);
    memcpy(work, avail, m * sizeof *work);
    int count = 0, ok = 1;
    while (count < n) {
        int found = 0;
        for (int p = 0; p < n; p++) {
            if (finish[p]) continue;
            int can = 1;
            for (int r = 0; r < m; r++) { (*ops)++; if (need[p * m + r] > work[r]) { can = 0; break; } }
            if (can) {
                for (int r = 0; r < m; r++) work[r] += alloc[p * m + r];
                finish[p] = 1; count++; found = 1;
            }
        }
        if (!found) { ok = 0; break; }
    }
    free(work); free(finish);
    return ok;
}

static void build(int n, int m, int worst, int *avail, int *need, int *alloc)
{
    for (int r = 0; r < m; r++) avail[r] = 1;
    for (int p = 0; p < n; p++)
        for (int r = 0; r < m; r++) {
            alloc[p * m + r] = 1;
            /* worst: process n-1-k needs k+1 of everything -> finishes only after k others;
               best : every process needs just 1 -> all finish in the first pass */
            need[p * m + r] = worst ? (n - p) : 1;
        }
}

int main(void)
{
    int ms[] = {3, 6};
    int ns[] = {10, 100, 1000, 2000, 4000};
    printf("%3s %6s | %-9s %12s %14s %18s\n", "m", "n", "state", "time", "comparisons", "time / (m*n^2) ns");
    for (int mi = 0; mi < 2; mi++) {
        int m = ms[mi];
        for (int ni = 0; ni < 5; ni++) {
            int n = ns[ni];
            int *avail = malloc(m * sizeof *avail), *need = malloc((size_t)n * m * sizeof *need), *alloc = malloc((size_t)n * m * sizeof *alloc);
            for (int worst = 1; worst >= 0; worst--) {
                build(n, m, worst, avail, need, alloc);
                long ops = 0;
                int reps = n <= 100 ? 2000 : (n <= 1000 ? 20 : 3);
                if (!worst) reps *= 20;
                double t0 = now();
                int safe = 1;
                for (int i = 0; i < reps; i++) { ops = 0; safe &= is_safe(n, m, avail, need, alloc, &ops); }
                double t = (now() - t0) / reps;
                printf("%3d %6d | %-9s %9.1f us %14ld %18.3f %s\n", m, n, worst ? "worst" : "best", t * 1e6, ops,
                       t * 1e9 / ((double)m * n * n), safe ? "" : "(UNSAFE?)");
            }
            free(avail); free(need); free(alloc);
        }
    }
    return 0;
}
