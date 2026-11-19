# SysPilot: Design and Performance Evaluation of CPU Scheduling Algorithms with Linux Resource Monitoring Using C

**Capstone Project Implementation**  
*Modules 1, 2, and 3 with Localhost Web Dashboard & C Engine*

---

##  Project Overview
**SysPilot** is a production-grade Operating System resource monitoring and algorithmic evaluation framework. It bridges low-level C system programming with an interactive, modern real-time Web Dashboard running locally on `http://localhost:5000`.

SysPilot addresses three core architectural pillars:
1. **Module 1: Live System and Process Monitoring** — Low-overhead inspection of aggregate/per-core CPU utilization, memory allocation, block storage I/O throughput, context switch rates, and active process queues via Linux `/proc` filesystem abstractions (with native host bindings).
2. **Module 2: Intelligent Bottleneck and Resource-Conflict Analysis** — Multi-variable heuristic inference engine detecting CPU thrashing, memory exhaustion (OOM danger), swap thrashing, storage wait stalls (D-state processes), and inter-process resource contention/priority inversion.
3. **Module 3: OS Performance Analysis and Optimization** — High-precision simulation and evaluation of classic and modern CPU scheduling algorithms (FCFS, SJF, SRTF, Round Robin, Priority with Aging, MLFQ, and Linux CFS), computing Turnaround Time, Waiting Time, Response Time, Jain's Fairness Index, and generating kernel tuning advice.

---

##  System Architecture

```
SysPilot/
├── include/
│   └── syspilot.h            # Unified C header for Modules 1, 2, and 3
├── src/
│   ├── monitor.c             # Module 1: /proc/stat, /proc/meminfo, /proc/diskstats parser
│   ├── analyzer.c            # Module 2: Heuristic bottleneck & conflict detection rules
│   ├── scheduler.c           # Module 3: 7 CPU scheduling algorithms & metric calculator
│   └── main.c                # Unified C CLI entry point with --json support
├── backend/
│   ├── system_monitor.py     # Real-time host telemetry & /proc bridge
│   ├── bottleneck_engine.py  # Module 2 rule engine & stress injection
│   ├── syspilot_core.py      # Module 3 simulation math & OS advisor
│   └── server.py             # Zero-dependency HTTP REST API & static server
├── frontend/
│   ├── index.html            # Single-page application dashboard
│   ├── css/style.css         # Sleek dark-mode system UI
│   └── js/
│       ├── monitor.js        # Real-time dials, waveforms & process table
│       ├── analyzer.js       # Health score gauge & bottleneck cards
│       ├── scheduler.js      # Interactive Gantt chart & comparison modal
│       └── app.js            # App controller, code inspector & report exporter
├── tests/
│   └── test_scheduler.c      # Automated C unit tests
├── Makefile                  # Unix/Linux GCC build configuration
├── CMakeLists.txt            # Cross-platform CMake configuration
├── verify_project.py         # 5-stage automated test & acceptance suite
├── start_syspilot.bat        # Windows one-click launcher
└── start_syspilot.sh         # Linux / macOS launcher
```

---

##  How to Run the Application on Localhost

### Method 1: One-Click Launch (Windows)
Double-click `start_syspilot.bat` or run in PowerShell / Command Prompt:
```powershell
cd C:\Users\mani\.gemini\antigravity\scratch\SysPilot
python backend\server.py
```
Open your browser and navigate to:
 **`http://localhost:5000`**

### Method 2: Linux / WSL Build & Run
```bash
cd SysPilot
make                     # Compiles the C binary 'syspilot'
./syspilot               # Runs Module 1, 2, and 3 in terminal
./start_syspilot.sh      # Launches the localhost Web Dashboard
```

---

##  Subsystem Deep Dive

### Module 1: Live System and Process Monitoring
- **CPU Metrics:** Aggregates `/proc/stat` counter deltas ($\Delta user, \Delta system, \Delta idle, \Delta iowait$) across sampling windows to yield exact per-core and system utilization percentages.
- **Memory & Swap:** Parses `/proc/meminfo` extracting `MemTotal`, `MemAvailable`, `Buffers`, `Cached`, and `SwapTotal/SwapFree`.
- **Block I/O:** Measures sector read/write rates from `/proc/diskstats`.
- **Process Table:** Traverses `/proc/[pid]/stat` and `/proc/[pid]/status` inspecting PID, state (`R`, `S`, `D`), nice, priority, thread count, RSS bytes, and virtual memory.

### Module 2: Intelligent Bottleneck & Conflict Engine
- **Rule 1 (CPU Saturation):** Triggers when aggregate load $\ge 85.0\%$. Identifies the culprit PID consuming maximal CPU and calculates ready queue contention.
- **Rule 2 (Memory Pressure & OOM):** Evaluates physical RAM $\ge 90.0\%$ and swap usage $\ge 30.0\%$. Recommends kernel slab reclamation and `vm.swappiness` adjustments.
- **Rule 3 (Storage I/O Starvation):** Detects $iowait \ge 12.0\%$ or presence of Uninterruptible Sleep (`D` state) tasks blocking execution pipelines.
- **Rule 4 (Resource Contention & Priority Inversion):** Pinpoints tasks concurrently fighting for core affinity or low-priority background workers starving high-priority interactive tasks.
- **System Health Index:** Computes a composite health score $H \in [0, 100]\%$.

### Module 3: OS CPU Scheduling Algorithms & Optimization
1. **First-Come, First-Served (FCFS):** Evaluates arrival-order non-preemptive queueing; demonstrates the Convoy Effect.
2. **Shortest Job First (SJF Non-Preemptive):** Minimizes waiting time when burst lengths are known.
3. **Shortest Remaining Time First (SRTF Preemptive):** Dynamically preempts when shorter tasks arrive.
4. **Round Robin (RR):** Configurable time slice quantum $q$. Balances response time against context switch overhead.
5. **Priority Preemptive with Aging:** Prevents indefinite starvation of low-priority tasks via dynamic priority aging.
6. **Multilevel Feedback Queue (MLFQ):** 3-tier feedback hierarchy ($Q_0$: RR $q=2$, $Q_1$: RR $q=4$, $Q_2$: FCFS) segregating interactive bursts from compute-heavy jobs.
7. **Linux Completely Fair Scheduler (CFS) Emulation:** Models virtual runtime ($vruntime$) progression using the standard Linux kernel 40-level nice-to-weight lookup table:
   $$vruntime \mathrel{+}= \Delta exec\_time \times \frac{w_0}{w_i}$$
   where $w_0 = 1024$ and $w_i$ is the weight of nice level $i$.

### Mathematical Evaluation Metrics
- **Turnaround Time ($TAT$):** $TAT_i = C_i - A_i$
- **Waiting Time ($WT$):** $WT_i = TAT_i - B_i$
- **Response Time ($RT$):** $RT_i = S_i - A_i$
- **Jain's Fairness Index ($\mathcal{J}$):**
  $$\mathcal{J}(x_1, x_2, \dots, x_n) = \frac{\left(\sum_{i=1}^n x_i\right)^2}{n \sum_{i=1}^n x_i^2}$$
  where $x_i = B_i / TAT_i$ is the normalized service rate.

---

##  Verification & Test Results
SysPilot includes an automated verification suite in `verify_project.py`:
```powershell
python verify_project.py
```
**Results:**
-  Module 1: CPU, Memory, Disk, and Process monitoring validated.
-  Module 2: Heuristic rules and stress testing verified.
-  Module 3: All 7 scheduling algorithms verified with exact mathematical TAT, WT, RT, and Fairness calculations.
-  C source code & Makefile verified.
-  Frontend single-page application verified.
