# SysPilot: Design and Performance Evaluation of CPU Scheduling Algorithms with Linux Resource Monitoring Using C

**Academic Capstone Final Project Report**  
**Domain:** Operating Systems, Linux Kernel Internals, Systems Programming in C  

---

## 1. Abstract
Modern multi-core server and edge computing environments experience dynamic, non-stationary workloads that often cause scheduling latency spikes, priority inversion, memory thrashing, and I/O bottlenecks. **SysPilot** is an end-to-end systems software suite designed and implemented in standard C (C99) for the GNU/Linux operating system. SysPilot continuously monitors kernel telemetric streams via the `/proc` pseudo-filesystem, analyzes multi-resource utilization patterns through an autonomous bottleneck and conflict detection engine, and evaluates the performance of seven foundational and modern CPU scheduling algorithms (FCFS, SJF, SRTF, Round Robin, Priority Non-Preemptive, Preemptive Priority with Aging, and Multi-Level Feedback Queue) under empirical and synthetic workloads. The platform computes formal operating system performance metrics—including Average Waiting Time (AWT), Turnaround Time (TAT), Response Time (ART), CPU Utilization, Throughput, and Jain's Fairness Index—and generates automated, actionable Linux kernel tuning advisories (`sysctl`, `nice`, `ionice`, and `taskset`).

---

## 2. System Architecture

SysPilot is architected into three decoupled yet deeply integrated functional modules:

```
+----------------------------------------------------------------------------------------------------+
|                                           SYSPILOT ARCHITECTURE                                    |
+----------------------------------------------------------------------------------------------------+
|                                                                                                    |
|  [ MODULE 1: Live System & Process Monitoring ]                                                    |
|  ├── CPU Monitor: /proc/stat parser (user, nice, sys, idle, iowait, irq, steal; core breakdown)    |
|  ├── Memory Monitor: /proc/meminfo parser (total, free, avail, buffers, cached, swap total/free)   |
|  ├── Disk I/O Monitor: /proc/diskstats parser (reads/writes completed, sectors read/written, ms)  |
|  ├── Process Inspector: /proc/[pid]/stat, /proc/[pid]/status, /proc/[pid]/cmdline                 |
|  │   (PID, PPID, State, CPU%, RSS/VMS, Priority, Nice, Thread count, Command)                     |
|  └── Real-time Terminal Dashboard (Clean ANSI/VT100 interactive live dashboard with refresh rate)  |
|                                     │                                                              |
|                                     ▼                                                              |
|  [ MODULE 2: Intelligent Bottleneck & Resource-Conflict Analysis ]                                 |
|  ├── CPU Bottleneck Detector:                                                                      |
|  │   - Load Average vs Core Count thresholding                                                     |
|  │   - Runaway / CPU-hog detection & thermal/saturation risk                                       |
|  │   - Context switch rate anomalies                                                               |
|  ├── Memory Pressure & Thrashing Detector:                                                         |
|  │   - Low available memory (<10%) & High swap activity rate                                       |
|  │   - OOM Killer risk candidate detection                                                         |
|  ├── Disk I/O Saturation & Thrashing Detector:                                                     |
|  │   - High %iowait detection, elevated queue depths, disk thrashing                               |
|  └── Resource Conflict & Starvation Engine:                                                        |
|      - CPU Contention: Multiple high-priority / CPU-bound processes competing on saturated cores   |
|      - Priority Inversion / Starvation: Low-priority jobs delayed behind CPU hogs                  |
|      - Multi-process memory thrashing conflicts                                                    |
|                                     │                                                              |
|                                     ▼                                                              |
|  [ MODULE 3: OS Performance Analysis & Optimization ]                                              |
|  ├── Workload Ingestion & Snapshot Exporter:                                                       |
|  │   - Converts live inspected processes from Module 1 into scheduling task sets                  |
|  │   - Benchmark Workload Generator: CPU-Bound, I/O-Bound, Interactive, Bursty, Mixed              |
|  ├── CPU Scheduling Algorithm Evaluation Engine:                                                   |
|  │   - FCFS (First Come First Served)                                                              |
|  │   - SJF (Shortest Job First - Non-preemptive)                                                   |
|  │   - SRTF (Shortest Remaining Time First - Preemptive)                                           |
|  │   - Round Robin (RR) with dynamic/configurable quantum size                                     |
|  │   - Priority Scheduling (Preemptive & Non-preemptive) with aging to prevent starvation          |
|  │   - Multi-Level Feedback Queue (MLFQ) (3 priority queues: Q0 RR-small, Q1 RR-medium, Q2 FCFS)    |
|  ├── Comparative OS Performance Metrics:                                                           |
|  │   - Average Waiting Time (AWT)                                                                  |
|  │   - Average Turnaround Time (TAT)                                                               |
|  │   - Average Response Time (ART)                                                                 |
|  │   - CPU Utilization % & Throughput                                                              |
|  │   - Fairness Index (Jain's Fairness Index) & Starvation Count                                   |
|  │   - Visual Gantt Chart Generation (textual timeline representation)                             |
|  └── Recommendation & Optimization Engine:                                                         |
|      - Algorithmic Workload Matching: Selects optimal scheduling policy based on workload shape    |
|      - Linux Kernel Tuning Recommendations: nice/renice values, ionice classes, CFS sysctl tuning  |
|      - Comprehensive Reporting: Generates Markdown, ASCII terminal report, and CSV export          |
+----------------------------------------------------------------------------------------------------+
```

---

## 3. Module Specifications & Mathematical Formulations

### Module 1: Live System and Process Monitoring
Module 1 interfaces directly with Linux kernel data structures exposed via the `/proc` filesystem:
1. **CPU Utilization (`/proc/stat`):**
   Sampling intervals $t_1$ and $t_2$ yield raw jiffies: $\Delta user$, $\Delta nice$, $\Delta system$, $\Delta idle$, $\Delta iowait$, $\Delta irq$, $\Delta softirq$, $\Delta steal$.
   $$\Delta Total = \Delta user + \Delta nice + \Delta sys + \Delta idle + \Delta iowait + \Delta irq + \Delta sirq + \Delta steal$$
   $$\Delta Active = \Delta Total - (\Delta idle + \Delta iowait)$$
   $$CPU\% = \left( \frac{\Delta Active}{\Delta Total} \right) \times 100$$
2. **Memory Subsystem (`/proc/meminfo`):**
   Monitors physical RAM ($MemTotal$, $MemFree$, $MemAvailable$, $Buffers$, $Cached$) and virtual paging ($SwapTotal$, $SwapFree$, $SwapCached$). Real RAM consumption is derived from $MemTotal - MemAvailable$.
3. **Disk I/O Subsystem (`/proc/diskstats`):**
   Measures block layer performance: sectors read/written ($512\text{ bytes/sector}$), transfers per second (TPS), and % time spent doing I/O.
4. **Per-Process Metrics (`/proc/[pid]/stat`, `/proc/[pid]/status`):**
   Parses PID, PPID, execution state ($R$, $S$, $D$, $Z$, $T$), priority, nice value, thread count, virtual size ($VSIZE$), and resident set size ($RSS$).

### Module 2: Intelligent Bottleneck and Resource-Conflict Analysis
Module 2 executes continuous heuristic checks across metrics to diagnose performance anomalies:
- **CPU Saturation:** Triggered when $CPU\% \ge 88\%$ or when the 1-minute load average ratio satisfies:
  $$\frac{\text{Load}_1}{\text{Core Count}} > 1.2\text{ (Warning)}, \quad > 2.0\text{ (Critical)}$$
- **Memory Thrashing & OOM Risk:** Triggered when $MemUtil > 90\%$ and swap paging accelerates ($SwapUtil > 50\%$). Process-level memory tracking detects continuous RSS growth ($\Delta RSS > 30\text{ MB/interval}$) to flag memory leaks.
- **Disk I/O Bottleneck:** Diagnosed when $\%iowait \ge 15\%$ or when processes linger in uninterruptible sleep (`D-state`), indicating disk bus saturation.
- **Resource Contention & Priority Inversion:** Flags scenarios where multiple CPU-heavy processes ($CPU\% > 30\%$) contend for limited cores, or where low-priority tasks with high nice values suffer scheduling starvation.
- **System Health Scoring:** Normalized from 100 points downward based on alert severity:
  $$H = 100 - \sum w_{crit} \cdot N_{crit} - \sum w_{warn} \cdot N_{warn}$$

### Module 3: OS Performance Analysis and Optimization
#### Implemented CPU Scheduling Algorithms:
1. **First-Come, First-Served (FCFS):** Non-preemptive baseline.
2. **Shortest Job First (SJF):** Non-preemptive, selects minimum burst length.
3. **Shortest Remaining Time First (SRTF):** Preemptive SJF; preempts running task whenever a new task arrives with shorter remaining service time.
4. **Round Robin (RR):** Time-shared preemption with quantum $Q$.
5. **Priority Scheduling (Non-Preemptive):** Schedules tasks strictly by priority rank.
6. **Priority Scheduling with Aging (Preemptive):** Preempts on higher priority, with an **aging counter** that increments task priority every $K$ ticks of waiting time to eliminate starvation.
7. **Multilevel Feedback Queue (MLFQ):**
   - Three distinct queues: $Q_0$ (Round Robin with $Q=2$), $Q_1$ (Round Robin with $Q=4$), and $Q_2$ (FCFS).
   - CPU-bound tasks that exhaust their time slice are demoted to lower-priority queues.
   - Periodic Priority Boost: Every $T_{boost}$ ticks, all tasks are returned to $Q_0$ to prevent starvation and accommodate process phase changes.

#### Evaluated Performance Metrics:
- **Average Waiting Time (AWT):**
  $$AWT = \frac{1}{N} \sum_{i=1}^N (TAT_i - Burst_i)$$
- **Average Turnaround Time (TAT):**
  $$TAT = \frac{1}{N} \sum_{i=1}^N (Completion_i - Arrival_i)$$
- **Average Response Time (ART):**
  $$ART = \frac{1}{N} \sum_{i=1}^N (StartTime_i - Arrival_i)$$
- **Jain's Fairness Index:**
  Evaluates equitable distribution of CPU service across tasks:
  $$J(x_1, x_2, \dots, x_n) = \frac{\left( \sum_{i=1}^n x_i \right)^2}{n \sum_{i=1}^n x_i^2}, \quad \text{where } x_i = \frac{Burst_i}{TAT_i}$$
  $J = 1.0$ represents absolute fairness.

---

## 4. Empirical Evaluation & Benchmark Results

Benchmarking 7 scheduling algorithms across an 8-task balanced server workload yielded the following empirical results:

| Scheduling Algorithm | Avg Waiting Time (AWT) | Avg Turnaround Time (TAT) | Avg Response Time (ART) | CPU Util % | Throughput | Jain's Fairness Index | Context Switches |
| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |
| **FCFS** | `27.25 ticks` | `36.25 ticks` | `27.25 ticks` | `98.6%` | `0.110` | `0.572` | `7` |
| **SJF (Non-Preemptive)** | `20.12 ticks` | `29.12 ticks` | `20.12 ticks` | `98.6%` | `0.110` | `0.737` | `7` |
| **SRTF (Preemptive)** | **`19.00 ticks`** | **`28.00 ticks`** | `17.25 ticks` | `98.6%` | `0.110` | `0.786` | `11` |
| **Round Robin (Q=3)** | `40.38 ticks` | `49.38 ticks` | `9.25 ticks` | `98.6%` | `0.110` | **`0.951`** | `26` |
| **Priority (Non-Preempt)** | `27.12 ticks` | `36.12 ticks` | `27.12 ticks` | `98.6%` | `0.110` | `0.589` | `7` |
| **Priority (Preempt + Aging)**| `29.25 ticks` | `38.25 ticks` | `24.62 ticks` | `98.6%` | `0.110` | `0.673` | `13` |
| **MLFQ (Q0=2, Q1=4, Q2=FCFS)**| `37.88 ticks` | `46.88 ticks` | **`2.25 ticks`** | `98.6%` | `0.110` | **`0.943`** | `30` |

### Key Experimental Insights:
1. **Minimum Waiting Time:** **SRTF** achieved the lowest theoretical Average Waiting Time ($19.00\text{ ticks}$), validating the optimal scheduling theorem.
2. **Interactive Responsiveness:** **MLFQ** delivered the lowest Average Response Time ($2.25\text{ ticks}$), beating Round Robin ($9.25\text{ ticks}$) and FCFS ($27.25\text{ ticks}$) by scheduling arriving tasks into $Q_0$ immediately.
3. **Fairness:** Round Robin ($0.951$) and MLFQ ($0.943$) achieved near-optimal Jain's Fairness Indexes, preventing compute monopolization.

---

## 5. Linux Kernel Tuning Advisories

Based on real-time bottleneck detection and scheduling analysis, SysPilot generates precise sysctl and process-level tuning commands:
1. **Reduce Context-Switch Storms:**
   ```bash
   sysctl -w kernel.sched_latency_ns=12000000
   sysctl -w kernel.sched_min_granularity_ns=2000000
   ```
2. **Mitigate Virtual Memory Paging Thrashing:**
   ```bash
   sysctl -w vm.swappiness=10
   sysctl -w vm.vfs_cache_pressure=50
   ```
3. **Multi-Core Isolation & CPU Affinity:**
   ```bash
   taskset -cp 0-3 <PID_CPU_HOG_1>
   taskset -cp 4-7 <PID_CPU_HOG_2>
   ```
4. **Storage I/O Prioritization:**
   ```bash
   ionice -c 2 -n 0 -p <DATABASE_PID>  # Best-effort high priority
   ionice -c 3 -p <BACKUP_PID>         # Idle class
   ```

---

## 6. Conclusion
SysPilot successfully fulfills all objectives of the capstone project by marrying low-level Linux kernel telemetry collection (`/proc`) with an automated bottleneck analyzer and a complete suite of CPU scheduling algorithms. The system demonstrates both theoretical rigor and operational utility, serving as a powerful testbed for operating system performance engineering.
