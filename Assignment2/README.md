# CSA0421 – Operating Systems – Assignment 2 (CO2)

| Field | Value |
|---|---|
| Name | **Bharath Raj S** |
| Register number | **192521082** |
| Compiler | gcc 13.3.0 (Ubuntu 13.3.0-6ubuntu2~24.04.1) |
| Linux distribution | Ubuntu 24.04.5 LTS, 2 CPU cores |
| Standard compile command | `gcc -std=c11 -Wall -Wextra -pthread` (benchmarks add `-O2`, `-std=gnu11`) |

> Replace the bold placeholders and the machine details with your own before submitting, and re-run every program on **your** machine so the outputs and screenshots are yours.

Report: `CSA04_Assignment2_Report.docx` (Q1–Q6 evidence). Diagrams are drawn by `make_figures.py` into `report_assets/`.

## Q1 – Processes (Task01)
```
cd Task01
gcc -std=c11 -Wall -Wextra -pthread fork_demo.c -o fork_demo && ./fork_demo     # fork + ps + /proc
gcc -O2 -std=gnu11 -pthread scale_test.c -o scale_test && ./scale_test          # creation + context switch
gcc -O2 -std=gnu11 -pthread mem_test.c -o mem_test && ./mem_test proc; ./mem_test thread
```
Result: 4 children seen in state S, then Z (zombie) before wait(). Process vs thread: create 764.6 vs 65.6 µs, switch 1.88 vs 0.96 µs, memory 154.9 vs 20.8 kB per idle session. Decision: fixed pool of worker processes.

## Q2 – CPU scheduling (Task02)
```
cd Task02
gcc -std=c11 -Wall -Wextra rr.c -o rr && ./rr          # completed Round Robin (quantum 3)
# quantum study: rr_q1.c, rr_q5.c, rr_q20.c differ only in the quantum line
python3 sched.py | tee sched_output.txt                # FCFS, SJF, SRTF, Priority, RR, MLFQ, ageing, video workload
gcc -O2 -std=c11 sched_cost.c -o sched_cost && ./sched_cost
```
Result: RR output matches the expected output (avg TAT 20.00, avg WT 13.50). Quantum 1 → WT 12.50 (25 switches); quantum 20 → WT 8.75 (3 switches, = FCFS). Recommendation: MLFQ with priority boost.

## Q3 – Threads (Task03)
```
cd Task03
gcc -O2 -std=gnu11 -Wall -Wextra -pthread cameras.c -o cameras && ./cameras
```
Result: speed-up ≈ 1.9–2.0× for 4, 8, 16 threads and 64 threads (2-core machine); unprotected counter lost 49,182 of 2,000,000 updates.

## Q4 – IPC (Task04)
```
cd Task04
gcc -std=c11 -Wall -Wextra -pthread pipe_readings.c -o pipe_readings && ./pipe_readings   # completed pipe program
gcc -std=c11 -pthread pipe_noclose.c -o pipe_noclose && timeout 3 ./pipe_noclose           # hangs: write end still open
gcc -std=c11 -Wall -Wextra -pthread shm_readings.c -o shm_readings && ./shm_readings       # shared memory + semaphores
gcc -O2 -std=gnu11 -pthread ipc_bench.c -o ipc_bench -lrt && ./ipc_bench                   # 1/10/100 MB
```
Result: `analyser: 6 readings, average 35.5, max 41`. 100 MB: pipe 65.8 ms, message queue 106.4 ms, shared memory 49.8 ms.

## Q5 – Synchronisation (Task05)
```
cd Task05
gcc -std=c11 -Wall -Wextra -pthread bounded_buffer.c -o bounded_buffer && ./bounded_buffer
for i in $(seq 100); do ./bounded_buffer; done | sort | uniq -c       # always sum = 210
gcc -O2 -std=gnu11 -pthread bb_multi.c -o bb_multi && ./bb_multi 4 4
gcc -O2 -std=gnu11 -pthread -DNO_MUTEX bb_multi.c -o bb_multi_nomutex && ./bb_multi_nomutex 4 4   # wrong
gcc -O2 -std=gnu11 -pthread monitor_buffer.c -o monitor_buffer && ./monitor_buffer
gcc -O2 -std=gnu11 -pthread peterson.c -o peterson && ./peterson
gcc -O2 -std=gnu11 -pthread dining.c -o dining && ./dining
gcc -O2 -std=gnu11 -pthread readers_writers.c -o readers_writers && ./readers_writers
gcc -O2 -std=gnu11 -pthread ticket_locks.c -o ticket_locks && ./ticket_locks 8
gcc -O2 -std=gnu11 -pthread sync_alt.c -o sync_alt && ./sync_alt
```
Result: sum = 210 in 100/100 runs; without the mutex 4×4 threads corrupt ≈ 9% of items. Decision: one lock per coach.

## Q6 – Deadlock (Task06)
```
cd Task06
gcc -std=c11 -Wall -Wextra banker.c -o banker && ./banker
gcc -O2 -std=c11 banker_bench.c -o banker_bench && ./banker_bench
gcc -std=c11 -Wall -Wextra detect.c -o detect && ./detect
```
Result: initial state safe `P1 P3 P4 P0 P2`; P1 granted, P4 must wait, P0 denied (unsafe). Safety check grows ≈ n² (4.05× and 4.15× when n doubles).
