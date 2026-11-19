/* Q1 evaluation: memory consumed by 2000 idle sessions as processes vs as threads.
 * Measures the drop in MemAvailable (from /proc/meminfo) while the sessions sleep.
 * Compile: gcc -O2 -std=gnu11 -Wall -Wextra -pthread mem_test.c -o mem_test
 * Run:     ./mem_test proc   |   ./mem_test thread
 */
#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <signal.h>
#include <sys/wait.h>

#define N 2000

static long mem_avail_kb(void)
{
    FILE *f = fopen("/proc/meminfo", "r");
    char line[128]; long v = 0;
    while (fgets(line, sizeof line, f))
        if (sscanf(line, "MemAvailable: %ld", &v) == 1) break;
    fclose(f);
    return v;
}

static void *sleeper(void *a)
{
    (void)a;
    char stack_use[4096];            /* touch one page of stack, like a small session */
    memset(stack_use, 1, sizeof stack_use);
    pause();
    return stack_use[0] ? NULL : NULL;
}

int main(int argc, char **argv)
{
    if (argc < 2) { fprintf(stderr, "usage: %s proc|thread\n", argv[0]); return 1; }
    /* the "server" has a 20 MB image that every process would share copy-on-write */
    char *heap = malloc(20u << 20);
    memset(heap, 1, 20u << 20);
    long before = mem_avail_kb();

    if (!strcmp(argv[1], "proc")) {
        pid_t kids[N];
        for (int i = 0; i < N; i++) {
            kids[i] = fork();
            if (kids[i] == 0) { sleeper(NULL); _exit(0); }
        }
        sleep(2);
        long after = mem_avail_kb();
        printf("%d idle processes : MemAvailable fell by %ld kB  (%.1f kB each)\n",
               N, before - after, (double)(before - after) / N);
        for (int i = 0; i < N; i++) kill(kids[i], SIGKILL);
        for (int i = 0; i < N; i++) waitpid(kids[i], NULL, 0);
    } else {
        pthread_t th[N];
        pthread_attr_t at; pthread_attr_init(&at);
        pthread_attr_setstacksize(&at, 256 * 1024);
        for (int i = 0; i < N; i++) pthread_create(&th[i], &at, sleeper, NULL);
        sleep(2);
        long after = mem_avail_kb();
        printf("%d idle threads   : MemAvailable fell by %ld kB  (%.1f kB each)\n",
               N, before - after, (double)(before - after) / N);
        fflush(stdout);
        _exit(0);   /* sleeping threads die with the process */
    }
    return 0;
}
