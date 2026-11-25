/* Alternative concept: LOOK and C-LOOK for the given queue */
#include <stdio.h>
#include <stdlib.h>
#define N 8
int req[N] = {98, 183, 37, 122, 14, 124, 65, 67};
int cmp(const void *a, const void *b) { return *(const int *)a - *(const int *)b; }
int main(void)
{
    int head = 53, s[N], order[N], n = 0, i, total = 0, pos = head;
    for (i = 0; i < N; i++) s[i] = req[i];
    qsort(s, N, sizeof s[0], cmp);
    /* LOOK: towards 0 first, turn at the lowest request (14), then go up */
    for (i = N - 1; i >= 0; i--) if (s[i] < head) order[n++] = s[i];
    for (i = 0; i < N; i++) if (s[i] > head) order[n++] = s[i];
    printf("LOOK  %d", head);
    for (i = 0; i < n; i++) { printf(" %d", order[i]); total += abs(order[i] - pos); pos = order[i]; }
    printf(" -> %d cylinders\n", total);
    /* C-LOOK: upwards first, then jump to the lowest request and continue upwards */
    n = 0; total = 0; pos = head;
    for (i = 0; i < N; i++) if (s[i] > head) order[n++] = s[i];
    for (i = 0; i < N; i++) if (s[i] < head) order[n++] = s[i];
    printf("C-LOOK %d", head);
    for (i = 0; i < n; i++) { printf(" %d", order[i]); total += abs(order[i] - pos); pos = order[i]; }
    printf(" -> %d cylinders (including the jump back)\n", total);
    return 0;
}
