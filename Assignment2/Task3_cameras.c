/* Q3 - Traffic-camera number-plate reader: 64 cameras processed
 *   (a) sequentially, (b) one thread per camera, (c) a pool of 4, 8 and 16 threads,
 *   (d) [alternative concept] a pool of worker PROCESSES that return results through pipes.
 * Each "frame" is a fixed CPU-bound job whose checksum proves every frame was processed once.
 * Compile: gcc -O2 -std=gnu11 -Wall -Wextra -pthread cameras.c -o cameras
 * Run:     ./cameras
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdatomic.h>
#include <time.h>
#include <sys/wait.h>

#define CAMERAS 64
#define WORK    4000000UL      /* iterations per frame (a few ms of pure CPU work) */
#define REPEATS 3

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

/* stand-in for "read the number plate from frame f": deterministic CPU work */
__attribute__((noinline)) static unsigned long process_frame(int f)
{
    unsigned long x = 88172645463325252UL ^ (unsigned long)f;
    for (unsigned long i = 0; i < WORK; i++) {
        x ^= x << 13; x ^= x >> 7; x ^= x << 17;
    }
    return x;
}

static unsigned long expected_sum;

/* ---------- (a) sequential ---------- */
static unsigned long run_sequential(void)
{
    unsigned long s = 0;
    for (int f = 0; f < CAMERAS; f++) s ^= process_frame(f);
    return s;
}

/* ---------- (b) one thread per camera ---------- */
static unsigned long result_of[CAMERAS];
static void *camera_thread(void *arg)
{
    int f = (int)(long)arg;
    result_of[f] = process_frame(f);
    return NULL;
}
static unsigned long run_thread_per_camera(void)
{
    pthread_t th[CAMERAS];
    for (long f = 0; f < CAMERAS; f++) pthread_create(&th[f], NULL, camera_thread, (void *)f);
    for (int f = 0; f < CAMERAS; f++) pthread_join(th[f], NULL);
    unsigned long s = 0;
    for (int f = 0; f < CAMERAS; f++) s ^= result_of[f];
    return s;
}

/* ---------- (c) pool of T threads taking the next camera from an atomic counter ---------- */
static atomic_int next_frame;
static void *pool_worker(void *arg)
{
    (void)arg;
    for (;;) {
        int f = atomic_fetch_add(&next_frame, 1);
        if (f >= CAMERAS) break;
        result_of[f] = process_frame(f);
    }
    return NULL;
}
static unsigned long run_pool(int T)
{
    pthread_t th[64];
    atomic_store(&next_frame, 0);
    for (int i = 0; i < T; i++) pthread_create(&th[i], NULL, pool_worker, NULL);
    for (int i = 0; i < T; i++) pthread_join(th[i], NULL);
    unsigned long s = 0;
    for (int f = 0; f < CAMERAS; f++) s ^= result_of[f];
    return s;
}

/* ---------- (d) alternative: P worker processes, results combined through pipes ---------- */
static unsigned long run_processes(int P)
{
    int fds[64][2];
    pid_t pid[64];
    for (int w = 0; w < P; w++) {
        if (pipe(fds[w]) == -1) { perror("pipe"); exit(1); }
        pid[w] = fork();
        if (pid[w] == 0) {
            close(fds[w][0]);
            unsigned long s = 0;
            for (int f = w; f < CAMERAS; f += P) s ^= process_frame(f);
            if (write(fds[w][1], &s, sizeof s) != sizeof s) _exit(2);   /* IPC: result copied through the kernel */
            _exit(0);
        }
        close(fds[w][1]);
    }
    unsigned long total = 0;
    for (int w = 0; w < P; w++) {
        unsigned long s = 0;
        if (read(fds[w][0], &s, sizeof s) != sizeof s) fprintf(stderr, "short read\n");
        total ^= s;
        close(fds[w][0]);
        waitpid(pid[w], NULL, 0);
    }
    return total;
}

/* ---------- shared-counter race demonstration ---------- */
#define INCS 1000000
static volatile long unsafe_counter;          /* no protection */
static long locked_counter;                   /* protected by mutex */
static atomic_long atomic_counter;            /* atomic */
static pthread_mutex_t m = PTHREAD_MUTEX_INITIALIZER;
static void *inc_all(void *a)
{
    (void)a;
    for (int i = 0; i < INCS; i++) {
        unsafe_counter++;                                   /* read-modify-write: NOT atomic */
        pthread_mutex_lock(&m); locked_counter++; pthread_mutex_unlock(&m);
        atomic_fetch_add(&atomic_counter, 1);
    }
    return NULL;
}
static void race_demo(int T)
{
    pthread_t th[16];
    unsafe_counter = 0; locked_counter = 0; atomic_store(&atomic_counter, 0);
    for (int i = 0; i < T; i++) pthread_create(&th[i], NULL, inc_all, NULL);
    for (int i = 0; i < T; i++) pthread_join(th[i], NULL);
    long want = (long)T * INCS;
    printf("  %2d threads x %d increments, expected %ld\n", T, INCS, want);
    printf("      unprotected counter = %ld   (lost %ld updates)\n", unsafe_counter, want - unsafe_counter);
    printf("      mutex counter       = %ld\n", locked_counter);
    printf("      atomic counter      = %ld\n", (long)atomic_load(&atomic_counter));
}

static int cmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}

typedef unsigned long (*runner_t)(void);
static double bench(const char *label, unsigned long (*fn)(int), int arg, double base)
{
    double t[REPEATS];
    for (int r = 0; r < REPEATS; r++) {
        double t0 = now();
        unsigned long s = fn(arg);
        t[r] = now() - t0;
        if (s != expected_sum) { printf("CHECKSUM MISMATCH in %s\n", label); exit(1); }
    }
    qsort(t, REPEATS, sizeof t[0], cmp);
    double med = t[REPEATS / 2];
    printf("%-34s %8.3f s   speed-up %5.2fx\n", label, med, base ? base / med : 1.0);
    return med;
}
static unsigned long seq_wrap(int a) { (void)a; return run_sequential(); }
static unsigned long tpc_wrap(int a) { (void)a; return run_thread_per_camera(); }

int main(void)
{
    printf("CPU cores online: %ld\n", sysconf(_SC_NPROCESSORS_ONLN));
    expected_sum = run_sequential();
    printf("\n--- Processing %d camera frames ---\n", CAMERAS);
    double base = bench("sequential (1 thread)", seq_wrap, 0, 0);
    bench("one thread per camera (64 threads)", tpc_wrap, 0, base);
    int pools[] = {1, 2, 4, 8, 16};
    for (int i = 0; i < 5; i++) {
        char lab[64];
        snprintf(lab, sizeof lab, "thread pool of %d", pools[i]);
        bench(lab, run_pool, pools[i], base);
    }
    printf("\n--- Alternative: worker processes + pipes ---\n");
    int procs[] = {2, 4, 8, 16};
    for (int i = 0; i < 4; i++) {
        char lab[64];
        snprintf(lab, sizeof lab, "process pool of %d", procs[i]);
        bench(lab, run_processes, procs[i], base);
    }
    printf("\n--- Shared counter race (compiled with -O2) ---\n");
    race_demo(2);
    race_demo(4);
    return 0;
}
