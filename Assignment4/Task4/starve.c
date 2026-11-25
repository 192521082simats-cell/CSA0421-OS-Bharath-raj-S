/* SSTF starvation: request at 183 pending; a new request near cylinder 60 arrives every 1 ms.
   Service time = 4 ms fixed (rotation + transfer) + 0.05 ms per cylinder of seek. */
#include <stdio.h>
#include <stdlib.h>
#define MAXQ 100000
int main(void)
{
    int q[MAXQ], aq[MAXQ], nq = 0;       /* pending cylinders and arrival ms */
    int pos = 53; double t = 0; int next_arrival_ms = 1;
    q[nq] = 183; aq[nq++] = 0;           /* the victim */
    q[nq] = 60;  aq[nq++] = 0;           /* the stream has already started */
    int served_near = 0, victim_served = 0;
    while (t < 10000.0 && !victim_served) {
        while (next_arrival_ms <= t) {   /* one arrival per millisecond near cylinder 60 */
            q[nq] = 58 + (next_arrival_ms % 5); aq[nq++] = next_arrival_ms; next_arrival_ms++;
        }
        int best = -1;
        for (int i = 0; i < nq; i++) if (best < 0 || abs(q[i]-pos) < abs(q[best]-pos)) best = i;
        t += 4.0 + 0.05 * abs(q[best] - pos);
        pos = q[best];
        if (q[best] == 183) victim_served = 1; else served_near++;
        q[best] = q[nq-1]; aq[best] = aq[nq-1]; nq--;
    }
    printf("SSTF: after %.0f ms, %d near-60 requests served, request 183 served? %s, queue length %d\n",
           t, served_near, victim_served ? "yes" : "NO (starved)", nq);
    return 0;
}
