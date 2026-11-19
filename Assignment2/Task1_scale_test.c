/* Q1 evaluation: cost of creating and switching between 2000 processes vs 2000 threads,
 * and memory per unit.
 * Compile: gcc -O2 -std=c11 -Wall -Wextra -pthread scale_test.c -o scale_test
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sched.h>
#include <sys/wait.h>
#include <time.h>

#define N 2000
#define PINGS 200000

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static void *thr_fn(void *a) { (void)a; return NULL; }

static long rss_kb(void)
{
    FILE *f = fopen("/proc/self/status", "r");
    char line[256]; long v = 0;
    while (f && fgets(line, sizeof line, f))
        if (sscanf(line, "VmRSS: %ld", &v) == 1) break;
    if (f) fclose(f);
    return v;
}

int main(void)
{
    /* give the parent a "server-like" 20 MB heap so fork has something to copy-on-write */
    char *heap = malloc(20u << 20);
    memset(heap, 1, 20u << 20);
    printf("parent RSS = %ld kB\n", rss_kb());

    /* 1. creation cost: fork+wait 2000 processes */
    double t0 = now();
    for (int i = 0; i < N; i++) {
        pid_t p = fork();
        if (p == 0) _exit(0);
        waitpid(p, NULL, 0);
    }
    double tf = now() - t0;

    /* 2. creation cost: create+join 2000 threads */
    t0 = now();
    for (int i = 0; i < N; i++) {
        pthread_t t;
        pthread_create(&t, NULL, thr_fn, NULL);
        pthread_join(t, NULL);
    }
    double tt = now() - t0;
    printf("create+destroy %d processes: %.1f ms  (%.1f us each)\n", N, tf * 1e3, tf * 1e6 / N);
    printf("create+destroy %d threads  : %.1f ms  (%.1f us each)\n", N, tt * 1e3, tt * 1e6 / N);

    /* 3. context-switch cost: ping-pong over two pipes, pinned to one CPU */
    cpu_set_t set; CPU_ZERO(&set); CPU_SET(0, &set);
    sched_setaffinity(0, sizeof set, &set);

    int a[2], b[2]; char c = 'x';
    pipe(a); pipe(b);
    pid_t p = fork();
    if (p == 0) {
        for (int i = 0; i < PINGS; i++) { read(a[0], &c, 1); write(b[1], &c, 1); }
        _exit(0);
    }
    t0 = now();
    for (int i = 0; i < PINGS; i++) { write(a[1], &c, 1); read(b[0], &c, 1); }
    double tp = now() - t0;
    waitpid(p, NULL, 0);
    printf("process switch (pipe ping-pong): %.2f us per switch\n", tp * 1e6 / (2.0 * PINGS));

    /* thread ping-pong using the same pipes */
    int a2[2], b2[2];
    pipe(a2); pipe(b2);
    struct targs { int r, w; } ta = { a2[0], b2[1] };
    void *echo(void *arg) { struct targs *x = arg; char ch;
        for (int i = 0; i < PINGS; i++) { read(x->r, &ch, 1); write(x->w, &ch, 1); } return NULL; }
    pthread_t th; pthread_create(&th, NULL, echo, &ta);
    t0 = now();
    for (int i = 0; i < PINGS; i++) { write(a2[1], &c, 1); read(b2[0], &c, 1); }
    double tth = now() - t0;
    pthread_join(th, NULL);
    printf("thread  switch (pipe ping-pong): %.2f us per switch\n", tth * 1e6 / (2.0 * PINGS));
    return 0;
}
