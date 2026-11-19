/* Q6 alternative concept: deadlock DETECTION instead of avoidance.
 * No maximum-claim matrix is needed: the algorithm looks at what is allocated now and what each
 * process is currently REQUESTING, and reports the processes that can never finish.
 * Compile: gcc -std=c11 -Wall -Wextra detect.c -o detect
 */
#include <stdio.h>
#include <string.h>

#define P 5
#define R 3

static void detect(const char *title, const int avail0[R], const int alloc[P][R], const int req[P][R])
{
    int work[R], finish[P];
    memcpy(work, avail0, sizeof work);
    for (int p = 0; p < P; p++) {
        int zero = 1;
        for (int r = 0; r < R; r++) if (alloc[p][r]) zero = 0;
        finish[p] = zero;                       /* a process holding nothing cannot be part of a deadlock */
    }
    int progress = 1;
    while (progress) {
        progress = 0;
        for (int p = 0; p < P; p++) {
            if (finish[p]) continue;
            int ok = 1;
            for (int r = 0; r < R; r++) if (req[p][r] > work[r]) { ok = 0; break; }
            if (ok) { for (int r = 0; r < R; r++) work[r] += alloc[p][r]; finish[p] = 1; progress = 1; }
        }
    }
    printf("%s: ", title);
    int dead = 0;
    for (int p = 0; p < P; p++) if (!finish[p]) { printf("P%d ", p); dead++; }
    printf(dead ? "are DEADLOCKED\n" : "no deadlock\n");
}

int main(void)
{
    int avail[R] = {0, 0, 0};
    int alloc[P][R] = {{0, 1, 0}, {2, 0, 0}, {3, 0, 3}, {2, 1, 1}, {0, 0, 2}};
    int req1[P][R]  = {{0, 0, 0}, {2, 0, 2}, {0, 0, 0}, {1, 0, 0}, {0, 0, 2}};
    int req2[P][R]  = {{0, 0, 0}, {2, 0, 2}, {0, 0, 1}, {1, 0, 0}, {0, 0, 2}};
    detect("snapshot 1", avail, alloc, req1);
    detect("snapshot 2 (P2 now also requests one more C)", avail, alloc, req2);
    return 0;
}
