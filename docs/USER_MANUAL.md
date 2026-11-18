# SysPilot: User Manual & Execution Guide

Welcome to **SysPilot**, an advanced C systems engineering platform for real-time Linux resource monitoring, intelligent bottleneck analysis, and CPU scheduling performance evaluation.

---

## 1. Directory Structure

```
SysPilot/
├── include/
│   ├── common.h            # Shared types, colors, enums, time utilities
│   ├── monitor.h           # Module 1: Telemetry data structures and headers
│   ├── analyzer.h          # Module 2: Bottleneck and conflict analysis headers
│   ├── scheduler.h         # Module 3: Scheduling algorithm headers and PCB types
│   ├── optimizer.h         # Module 3: OS recommendations and report exporters
│   └── procfs_compat.h     # Native Linux /proc parser and simulation layer
├── src/
│   ├── main.c              # Unified CLI controller and mode dispatcher
│   ├── common.c            # Utility implementations
│   ├── procfs_compat.c     # /proc parser (/proc/stat, meminfo, diskstats, [pid])
│   ├── monitor.c           # Module 1: Metric collector and terminal dashboard
│   ├── analyzer.c          # Module 2: Anomaly and contention detection engine
│   ├── scheduler.c         # Module 3: FCFS, SJF, SRTF, RR, Priority, MLFQ
│   └── optimizer.c         # Module 3: Kernel tunings and report generation
├── scripts/
│   ├── run_stress_test.sh  # Linux bash script to generate CPU, memory, and I/O stress
│   ├── benchmark_suite.py  # Automated mathematical verification test suite
│   └── syspilot_demo.py    # Cross-platform demonstration runner
├── docs/
│   ├── PROJECT_REPORT.md   # Comprehensive Academic Capstone Project Report
│   └── USER_MANUAL.md      # This user guide
├── Makefile                # Standard Linux Makefile
└── CMakeLists.txt          # Cross-platform CMake build configuration
```

---

## 2. Compilation & Build Instructions

### Method A: Building on Linux / Ubuntu / Debian / WSL using `make`
1. Open a terminal in the `SysPilot` root directory:
   ```bash
   cd SysPilot
   ```
2. Build the optimized release binary:
   ```bash
   make
   ```
   *(To build with debug symbols: `make debug`)*
3. Run the complete pipeline:
   ```bash
   ./syspilot --full
   ```

### Method B: Building with `CMake`
```bash
mkdir -p build && cd build
cmake ..
cmake --build .
./syspilot --full
```

### Method C: Running Directly on Windows / Any Machine (No Compiler Required)
SysPilot includes a cross-platform Python companion runner (`scripts/syspilot_demo.py`) that mirrors all modules and algorithms:
```bash
python scripts/syspilot_demo.py --full
```

---

## 3. Command-Line Interface (CLI) Flags

| Flag | Long Option | Description |
| :--- | :--- | :--- |
| `-f` | `--full` | **Default:** Executes full end-to-end pipeline (Module 1 + 2 + 3 + Reports) |
| `-m` | `--monitor` | Runs Module 1: Live interactive resource & process monitoring dashboard |
| `-a` | `--analyze` | Runs Module 2: Intelligent bottleneck & resource-conflict analysis |
| `-s` | `--schedule` | Runs Module 3: CPU scheduling algorithm benchmarking & Gantt charts |
| `-1` | `--once` | Captures a single dashboard snapshot (batch / headless mode) |
| `-w` | `--workload <type>`| Selects workload profile: `cpu`, `io`, `interactive`, `bursty`, `balanced` |
| `-q` | `--quantum <int>` | Configures Round Robin time quantum (default: 3) |
| `-n` | `--tasks <int>` | Number of tasks to schedule in Module 3 simulation (default: 8) |
| `-r` | `--report <file>` | Custom file path to export Markdown performance report |
| `-c` | `--csv <file>` | Custom file path to export benchmarking metrics CSV |
| | `--simulated` | Forces simulated Linux procfs (for non-Linux or presentation demo) |
| | `--inject-spike` | Injects sudden CPU saturation spike in simulation |
| | `--inject-leak` | Injects progressive memory leak in simulation |
| | `--inject-io` | Injects storage I/O storm in simulation |
| `-h` | `--help` | Displays command line manual |

---

## 4. Module-by-Module Verification Guide

### Verifying Module 1: Live System and Process Monitoring
Run the live dashboard:
```bash
./syspilot --monitor
```
- Observe real-time overall CPU%, per-core CPU breakdown, RAM and Swap consumption, and Disk I/O read/write rates.
- The process table displays active processes sorted by CPU utilization (`PID`, `COMMAND`, `PRI`, `NI`, `VIRT`, `RES`, `S`, `CPU%`, `MEM%`, `CMDLINE`).
- Press `Ctrl+C` to return to shell.

### Verifying Module 2: Bottleneck and Conflict Analysis
Run the diagnostic analyzer:
```bash
./syspilot --analyze
```
To test how it responds to high CPU load or memory pressure:
```bash
./syspilot --analyze --inject-spike
# or
./syspilot --analyze --inject-leak
```
Observe the **System Health Score (0-100)**, the **Primary Diagnostic**, active alerts with severity badges, and specific culprit process identification.

### Verifying Module 3: CPU Scheduling Algorithms Evaluation
Run the scheduler evaluation engine with different workload profiles:
```bash
# Evaluate CPU-bound computation tasks
./syspilot --schedule --workload cpu

# Evaluate I/O-bound tasks
./syspilot --schedule --workload io

# Evaluate interactive user-facing tasks
./syspilot --schedule --workload interactive

# Custom Round Robin quantum (e.g., Q=5)
./syspilot --schedule --workload balanced -q 5
```
Inspect the comparison table and visual Gantt timelines showing task execution order and context switching.

### Generating Capstone Artifacts
Run the complete pipeline to generate both the Markdown report and CSV dataset:
```bash
./syspilot --full --workload balanced
```
This generates:
1. `SysPilot_Performance_Report.md`: Academic capstone report with all tables, alerts, and tuning advisories.
2. `SysPilot_Metrics.csv`: Raw algorithm performance metrics for charting in Excel/Matplotlib.

---

## 5. Automated Test Suite
To run the automated mathematical validation tests:
```bash
python scripts/benchmark_suite.py
```
This verifies Jain's Fairness Index formulas and theoretical bounds (e.g., verifying that SRTF minimizes Average Waiting Time relative to FCFS).
