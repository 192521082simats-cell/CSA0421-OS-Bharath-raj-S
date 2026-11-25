/* Task 6 Stage 5 - workload simulation of two storage designs for the lecture-video start-up.
   Design A: HDD servers + RAM page cache only (16 videos hot in RAM per server).
   Design B: Design A + NVMe SSD cache tier (100 more hot videos) in front of the HDDs.
   Workload: 1,000 videos, Zipf popularity (exponent S), playback starts at a random minute.
   Both designs are driven by the SAME request stream (LRU stack distance).             */
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#define V 1000
#define REQS 200000
#define WARM 20000
#define RAM_VIDEOS 16
#define SSD_VIDEOS 100
#define S 1.2
/* device model */
#define HDD_MS   (8.0 + 4.0 + 1.0 / 150.0 * 1000.0)   /* seek + rotation + 1 MB at 150 MB/s = 18.7 ms */
#define SSD_MS   (0.1 + 1.0 / 2000.0 * 1000.0)        /* 0.1 ms + 1 MB at 2000 MB/s = 0.6 ms */
#define RAM_MS   0.05
#define READS_TO_START 3                              /* inode + extent block + first data chunk (cold) */
static unsigned long long seed = 7;
static double urand(void){ seed = seed*6364136223846793005ULL+1442695040888963407ULL; return ((seed>>11)&((1ULL<<53)-1))/(double)(1ULL<<53); }
static int cmpd(const void *a, const void *b){ double x=*(const double*)a,y=*(const double*)b; return (x>y)-(x<y); }
int main(void)
{
    static double cum[V]; double tot = 0;
    for (int i = 0; i < V; i++) { tot += 1.0 / pow(i + 1, S); cum[i] = tot; }
    static long last[V]; for (int i = 0; i < V; i++) last[i] = -1 - i;      /* cold start: nothing cached */
    static double latA[REQS], latB[REQS]; int n = 0;
    long hitRamA = 0, hitRamB = 0, hitSsdB = 0, missA = 0, missB = 0;
    for (long t = 0; t < REQS + WARM; t++) {
        double u = urand() * tot; int v = 0; while (cum[v] < u) v++;       /* Zipf video */
        int dist = 0; for (int i = 0; i < V; i++) if (last[i] > last[v]) dist++;   /* LRU stack distance */
        last[v] = t;
        if (t < WARM) continue;
        double a, b;
        int hitRam = dist < RAM_VIDEOS, hitSsd = dist < RAM_VIDEOS + SSD_VIDEOS;
        if (hitRam) { a = b = READS_TO_START * RAM_MS; hitRamA++; hitRamB++; }
        else { a = READS_TO_START * HDD_MS; missA++;
               if (hitSsd) { b = READS_TO_START * SSD_MS; hitSsdB++; } else { b = READS_TO_START * HDD_MS; missB++; } }
        latA[n] = a; latB[n] = b; n++;
    }
    double sa = 0, sb = 0; for (int i = 0; i < n; i++) { sa += latA[i]; sb += latB[i]; }
    qsort(latA, n, sizeof(double), cmpd); qsort(latB, n, sizeof(double), cmpd);
    double hA = (double)hitRamA / n, hB = (double)(hitRamB + hitSsdB) / n;
    printf("Requests measured: %d   (Zipf exponent %.1f, %d videos)\n\n", n, S, V);
    printf("%-34s %12s %12s\n", "", "Design A", "Design B");
    printf("%-34s %11.1f%% %11.1f%%\n", "Cache hit ratio (RAM+SSD)", 100*hA, 100*hB);
    printf("%-34s %11.1f%% %11.1f%%\n", "Requests reaching the HDDs", 100*(1-hA), 100*(1-hB));
    printf("%-34s %9.2f ms %9.2f ms\n", "Mean start-up read time (3 reads)", sa/n, sb/n);
    printf("%-34s %9.2f ms %9.2f ms\n", "95th percentile start-up", latA[(int)(0.95*n)], latB[(int)(0.95*n)]);
    /* per-server peak load: 2,000 streams x 0.5 MB/s = 1,000 chunk reads (1 MB) per second */
    double chunks = 1000.0, hddRate = 1000.0 / HDD_MS;
    printf("\nPer-server peak load = %.0f chunk reads/s; one HDD serves %.1f random 1-MB reads/s\n", chunks, hddRate);
    printf("%-34s %12.1f %12.1f\n", "HDD reads/s needed", chunks*(1-hA), chunks*(1-hB));
    printf("%-34s %12.0f %12.0f\n", "HDDs needed per server (ceil)", ceil(chunks*(1-hA)/hddRate), ceil(chunks*(1-hB)/hddRate));
    printf("%-34s %12s %9.0f MB/s\n", "NVMe read load per server", "-", chunks*((double)hitSsdB/n));
    return 0;
}
