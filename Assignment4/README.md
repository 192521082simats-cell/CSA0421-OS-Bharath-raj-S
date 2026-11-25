# CSA04 - Operating Systems - Assignment 4 (CO4)
Name: <your name>        Register number: <RegisterNumber>
Compiler: gcc <version>  OS: <Linux distribution and version>

## Compile
gcc -std=c11 -Wall -Wextra Task02/task2.c -o task2
gcc -std=c11 -Wall -Wextra Task04/task4.c -o task4
gcc -std=c11 -Wall -Wextra -O2 Task04/sim4.c -o sim4 -lm
gcc -std=c11 -Wall -Wextra -O2 Task06/sim6.c -o sim6 -lm
(starve.c, lookclook.c and task2_extra.c compile the same way)

## Run
./task2  ./task4  ./starve  ./lookclook  ./sim4  ./sim6

## Results
Task 2: first fit gave start 19; indexed gave index block 6
Task 4: FCFS 640, SSTF 236, SCAN 236, C-SCAN 382 (183 + 199 jump), LOOK 208, C-LOOK 322
Task 6: Design B (HDD + RAM + NVMe cache) chosen, hit ratio 77.6% vs 48.9%
Tasks 1, 3, 5: see report; Task 3.4 and Task 4.4 outputs from my own machine in Task03/ and Task04/
