/* Q4 alternative concept: the same weather-station exchange through SHARED MEMORY + SEMAPHORES.
 * The sensor reader writes each reading straight into a shared array (no kernel copy);
 * two POSIX semaphores provide the synchronisation that a pipe gives for free:
 *   slots  - number of free slots   (producer waits on it)
 *   items  - number of unread items (consumer waits on it)
 * End of data is signalled explicitly with a sentinel value (a pipe uses EOF instead).
 * Compile: gcc -std=c11 -Wall -Wextra -pthread shm_readings.c -o shm_readings
 */
#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include <stdio.h>
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/wait.h>

#define SLOTS 4
#define END_OF_DATA (-1)

struct shared {
    int buf[SLOTS];
    sem_t slots, items;
};

int main(void)
{
    struct shared *s = mmap(NULL, sizeof *s, PROT_READ | PROT_WRITE,
                            MAP_SHARED | MAP_ANONYMOUS, -1, 0);
    if (s == MAP_FAILED) { perror("mmap"); return 1; }
    sem_init(&s->slots, 1, SLOTS);   /* pshared = 1: usable across processes */
    sem_init(&s->items, 1, 0);

    pid_t pid = fork();
    if (pid == 0) {
        int out = 0, count = 0, sum = 0, max = 0;
        for (;;) {
            sem_wait(&s->items);
            int r = s->buf[out];
            out = (out + 1) % SLOTS;
            sem_post(&s->slots);
            if (r == END_OF_DATA) break;
            count++; sum += r; if (r > max) max = r;
        }
        printf("analyser: %d readings, average %.1f, max %d\n", count,
               count ? (double)sum / count : 0.0, max);
        fflush(stdout);
        _exit(0);
    }
    int readings[] = {31, 33, 36, 34, 41, 38, END_OF_DATA};
    int in = 0;
    for (int i = 0; i < 7; i++) {
        sem_wait(&s->slots);
        s->buf[in] = readings[i];
        in = (in + 1) % SLOTS;
        sem_post(&s->items);
    }
    waitpid(pid, NULL, 0);
    printf("reader: done\n");
    sem_destroy(&s->slots);
    sem_destroy(&s->items);
    munmap(s, sizeof *s);
    return 0;
}
