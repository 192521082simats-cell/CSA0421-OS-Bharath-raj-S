#!/bin/bash
# Compiles and runs every program of CSA04 Assignment 1 and saves the outputs.
set -e
CC="gcc -std=c11 -Wall -Wextra -pthread"
gcc --version | head -1
( cd Task05 && $CC -o syscall_demo syscall_demo.c && $CC -o stdio_demo stdio_demo.c && ./syscall_demo \
  && (command -v strace >/dev/null && strace -f -o strace_syscall_demo.txt ./syscall_demo >/dev/null && strace -c -o strace_c_stdio_demo.txt ./stdio_demo || echo "strace not installed: sudo apt install strace") )
( cd Task06 && $CC -o copy_file copy_file.c && ./copy_file | tee output.txt \
  && (command -v strace >/dev/null && strace -c ./copy_file 2> strace_c_output.txt >/dev/null; cat strace_c_output.txt || true) )
( cd Task07 && $CC -o shell shell.c && ./shell | tee output_terminal.txt \
  && $CC -o shell_noflush shell_noflush.c && ./shell_noflush > output_noflush_redirected.txt && cat output_noflush_redirected.txt )
( cd Task10 && $CC -o bench_copy bench_copy.c && ./bench_copy 16 | tee bench_copy_output.txt \
  && $CC -o bench_spawn bench_spawn.c && ./bench_spawn | tee bench_spawn_output.txt )
echo "Now run:  time ./Task06/copy_file   and   time ./Task10/bench_spawn   and paste screenshots."
