/* Task 4.2 - disk-scheduling simulation: 10,000 random requests, cylinders 0-199.
   Time unit = time to cross one cylinder; each request also costs OVERHEAD units
   (rotation + transfer). Waiting time = completion time - arrival time.
   Head movement includes trips to the disk end (SCAN, C-SCAN) and the C-SCAN/C-LOOK jump back. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#define NREQ 10000
#define MAXCYL 199
#define OVERHEAD 5.0
typedef enum { FCFS, SSTF, SCAN, CSCAN, LOOK, CLOOK } Alg;
static const char *names[] = {"FCFS", "SSTF", "SCAN", "C-SCAN", "LOOK", "C-LOOK"};
typedef struct { double arr; int cyl; } Req;
static Req r[NREQ];
static unsigned long long seed = 2024;
static double urand(void)
{
    seed = seed * 6364136223846793005ULL + 1442695040888963407ULL;
    return ((seed >> 11) & ((1ULL << 53) - 1)) / (double)(1ULL << 53);
}
static void gen(double mean_ia)
{
    double t = 0;
    for (int i = 0; i < NREQ; i++) {
        t += -mean_ia * log(1.0 - urand());
        r[i].arr = t;
        r[i].cyl = (int)(urand() * (MAXCYL + 1));
    }
}
static void run(Alg a, double *avgmove, double *avgwait, double *maxwait)
{
    static int pend[NREQ];
    int np = 0, next = 0, served = 0, pos = 100, dir = 1;
    double t = 0, move = 0, sumw = 0, maxw = 0;
    while (served < NREQ) {
        while (next < NREQ && r[next].arr <= t) pend[np++] = next++;
        if (np == 0) { t = r[next].arr; continue; }
        int pick = -1, bestd = 1 << 30;
        if (a == FCFS) pick = 0;
        else if (a == SSTF) {
            for (int i = 0; i < np; i++) { int d = abs(r[pend[i]].cyl - pos); if (d < bestd) { bestd = d; pick = i; } }
        } else {
            int d_dir = (a == CSCAN || a == CLOOK) ? 1 : dir;
            for (int i = 0; i < np; i++) {
                int c = r[pend[i]].cyl, d = (c - pos) * d_dir;
                if (d >= 0 && d < bestd) { bestd = d; pick = i; }
            }
            if (pick < 0) {                           /* nothing ahead of the head */
                if (a == SCAN)  { int e = dir > 0 ? MAXCYL : 0; double d = abs(e - pos); move += d; t += d; pos = e; dir = -dir; }
                else if (a == LOOK) dir = -dir;
                else if (a == CSCAN) { double d = (MAXCYL - pos) + MAXCYL; move += d; t += d; pos = 0; }
                else { int mn = MAXCYL; for (int i = 0; i < np; i++) if (r[pend[i]].cyl < mn) mn = r[pend[i]].cyl;
                       double d = pos - mn; move += d; t += d; pos = mn; }
                continue;
            }
        }
        int id = pend[pick], d = abs(r[id].cyl - pos);
        move += d; t += d + OVERHEAD; pos = r[id].cyl;
        double w = t - r[id].arr; sumw += w; if (w > maxw) maxw = w;
        memmove(&pend[pick], &pend[pick + 1], (size_t)(np - pick - 1) * sizeof pend[0]);
        np--; served++;
    }
    *avgmove = move / NREQ; *avgwait = sumw / NREQ; *maxwait = maxw;
}
int main(void)
{
    const char *load[] = {"Light (mean inter-arrival 200)", "Medium (mean inter-arrival 80)", "Heavy (mean inter-arrival 25)"};
    double ia[] = {200, 80, 25};
    for (int l = 0; l < 3; l++) {
        seed = 2024; gen(ia[l]);
        printf("\n%s\n%-8s %18s %16s %16s\n", load[l], "Algo", "Avg head move/req", "Avg wait (ct)", "Max wait (ct)");
        for (int a = 0; a < 6; a++) {
            double m, w, x; run((Alg)a, &m, &w, &x);
            printf("%-8s %18.1f %16.1f %16.0f\n", names[a], m, w, x);
        }
    }
    return 0;
}
