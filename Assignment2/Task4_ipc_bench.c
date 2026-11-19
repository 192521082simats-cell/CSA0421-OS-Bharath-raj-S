/* Q4 growth investigation: time to move 1 MB, 10 MB and 100 MB between two processes using
 *   - an anonymous pipe                         (2 kernel copies, implicit synchronisation)
 *   - POSIX message queue (8 KB messages)       (2 kernel copies + per-message overhead)
 *   - shared memory ring + semaphores           (0 kernel copies, explicit synchronisation)
 * Both sides touch every byte (checksum) so the comparison is fair.
 * Compile: gcc -O2 -std=gnu11 -Wall -Wextra -pthread ipc_bench.c -o ipc_bench -lrt
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <mqueue.h>
#include <semaphore.h>
#include <time.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define CHUNK (64 * 1024)
#define REPEATS 3

static double now(void)
{
    struct timespec t;
    clock_gettime(CLOCK_MONOTONIC, &t);
    return t.tv_sec + t.tv_nsec / 1e9;
}

static unsigned long sum_bytes(const unsigned char *p, size_t n)
{
    unsigned long s = 0;
    for (size_t i = 0; i < n; i++) s += p[i];
    return s;
}

/* ---------------- pipe ---------------- */
static double bench_pipe(size_t total, unsigned long *chk)
{
    int fd[2]; pipe(fd);
    int res[2]; pipe(res);
    double t0 = now();
    pid_t pid = fork();
    if (pid == 0) {
        close(fd[1]); close(res[0]);
        unsigned char *buf = malloc(CHUNK); unsigned long s = 0; ssize_t n;
        while ((n = read(fd[0], buf, CHUNK)) > 0) s += sum_bytes(buf, n);
        write(res[1], &s, sizeof s);
        _exit(0);
    }
    close(fd[0]); close(res[1]);
    unsigned char *buf = malloc(CHUNK);
    for (size_t sent = 0; sent < total; sent += CHUNK) {
        memset(buf, (int)(sent / CHUNK) & 0xff, CHUNK);          /* produce data */
        size_t off = 0;
        while (off < CHUNK) off += write(fd[1], buf + off, CHUNK - off);
    }
    close(fd[1]);
    read(res[0], chk, sizeof *chk);
    waitpid(pid, NULL, 0);
    double t = now() - t0;
    free(buf);
    return t;
}

/* ---------------- POSIX message queue ---------------- */
#define MSGSZ 8192
static int mq_ok = 1;
static double bench_mq(size_t total, unsigned long *chk)
{
    const char *name = "/csa04_bench_mq";
    struct mq_attr at = { .mq_flags = 0, .mq_maxmsg = 10, .mq_msgsize = MSGSZ };
    mq_unlink(name);
    mqd_t q = mq_open(name, O_CREAT | O_RDWR, 0600, &at);
    if (q == (mqd_t)-1) { mq_ok = 0; return 0; }
    int res[2]; pipe(res);
    double t0 = now();
    pid_t pid = fork();
    if (pid == 0) {
        close(res[0]);
        unsigned char *buf = malloc(MSGSZ); unsigned long s = 0; size_t got = 0;
        while (got < total) {
            ssize_t n = mq_receive(q, (char *)buf, MSGSZ, NULL);
            if (n < 0) _exit(1);
            s += sum_bytes(buf, n); got += n;
        }
        write(res[1], &s, sizeof s);
        _exit(0);
    }
    close(res[1]);
    unsigned char *buf = malloc(MSGSZ);
    for (size_t sent = 0; sent < total; sent += MSGSZ) {
        memset(buf, (int)(sent / CHUNK) & 0xff, MSGSZ);
        mq_send(q, (char *)buf, MSGSZ, 0);
    }
    read(res[0], chk, sizeof *chk);
    waitpid(pid, NULL, 0);
    double t = now() - t0;
    mq_close(q); mq_unlink(name); free(buf);
    return t;
}

/* ---------------- shared memory + semaphores ---------------- */
#define SLOTS 8
struct shm {
    unsigned char slot[SLOTS][CHUNK];
    sem_t slots, items;
};
static double bench_shm(size_t total, unsigned long *chk)
{
    struct shm *s = mmap(NULL, sizeof *s, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    sem_init(&s->slots, 1, SLOTS); sem_init(&s->items, 1, 0);
    int res[2]; pipe(res);
    double t0 = now();
    pid_t pid = fork();
    if (pid == 0) {
        close(res[0]);
        unsigned long sm = 0; int out = 0;
        for (size_t got = 0; got < total; got += CHUNK) {
            sem_wait(&s->items);
            sm += sum_bytes(s->slot[out], CHUNK);       /* read straight from shared memory */
            out = (out + 1) % SLOTS;
            sem_post(&s->slots);
        }
        write(res[1], &sm, sizeof sm);
        _exit(0);
    }
    close(res[1]);
    int in = 0;
    for (size_t sent = 0; sent < total; sent += CHUNK) {
        sem_wait(&s->slots);
        memset(s->slot[in], (int)(sent / CHUNK) & 0xff, CHUNK);   /* produce directly into shared memory */
        in = (in + 1) % SLOTS;
        sem_post(&s->items);
    }
    read(res[0], chk, sizeof *chk);
    waitpid(pid, NULL, 0);
    double t = now() - t0;
    munmap(s, sizeof *s);
    return t;
}

static int cmp(const void *a, const void *b)
{
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
static double median(double (*fn)(size_t, unsigned long *), size_t total, unsigned long *chk)
{
    double t[REPEATS];
    for (int i = 0; i < REPEATS; i++) { t[i] = fn(total, chk); if (!mq_ok) return 0; }
    qsort(t, REPEATS, sizeof t[0], cmp);
    return t[REPEATS / 2];
}

int main(void)
{
    size_t mbs[] = {1, 10, 100};
    printf("%8s | %12s | %12s | %12s | checksums equal?\n", "size", "pipe (ms)", "mqueue (ms)", "shm+sem (ms)");
    for (int i = 0; i < 3; i++) {
        size_t total = mbs[i] * 1024 * 1024;
        unsigned long c1 = 0, c2 = 0, c3 = 0;
        double tp = median(bench_pipe, total, &c1);
        double tq = median(bench_mq, total, &c2);
        double ts = median(bench_shm, total, &c3);
        char qs[32];
        if (mq_ok) snprintf(qs, sizeof qs, "%12.2f", tq * 1e3); else snprintf(qs, sizeof qs, "%12s", "n/a");
        printf("%5zu MB | %12.2f | %s | %12.2f | %s  (%.0f MB/s pipe, %.0f MB/s shm)\n",
               mbs[i], tp * 1e3, qs, ts * 1e3,
               (c1 == c3 && (!mq_ok || c1 == c2)) ? "yes" : "NO",
               mbs[i] / tp, mbs[i] / ts);
    }
    return 0;
}
