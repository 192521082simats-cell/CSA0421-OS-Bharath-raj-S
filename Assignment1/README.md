# CSA04 Operating Systems – Assignment 1 (Smart Hospital)

- Name: **[your name]**
- Register number: **[your register number]**
- Compiler: gcc 13.x (`gcc --version`)  |  OS: Ubuntu 24.04 LTS (or your Linux distribution + version)
- Compile command: `gcc -std=c11 -Wall -Wextra -pthread -o prog prog.c`

## How to run
`./run_all.sh` compiles and runs everything (Tasks 5, 6, 7, 10). Install strace first: `sudo apt install strace`.
Individual steps: see the commands inside `run_all.sh`.

## Results (replace with your own)
| Task | Result |
|---|---|
| 1 | One general-purpose OS rejected for monitors: no guaranteed 10 ms alarm deadline |
| 2 | Hard RTOS monitors, handheld tablets, desktop workstations, clustered DB, SMP imaging, distributed partner link |
| 3 | Monitors: microkernel RTOS; DB: modular monolithic Linux |
| 4 | Service table per environment |
| 5 | strace of syscall_demo; fopen/fread needs ~n/4096 calls |
| 6 | copy_file output matches expected: 10001 / 21 / 4 read() calls; best buffer 64 KB |
| 7 | shell output matches expected; duplicated lines explained by stdio buffer copied at fork() |
| 8 | CPU modes, interrupts vs polling |
| 9 | DMA for MRI (131,072,000 interrupts -> 8,000) |
| 10 | measurements: fork vs thread creation, buffer sizes |
| 11 | final plan |
| 12 | Decision: MODIFY |

Analysis tasks (1-4, 8, 9, 11, 12): put your notes in `TaskNN/notes.md` (copy from the Word report).
