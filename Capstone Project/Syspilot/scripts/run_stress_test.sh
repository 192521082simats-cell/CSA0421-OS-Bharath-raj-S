#!/usr/bin/env bash
# SysPilot: Linux Workload Generator & Stress Testing Utility
# Used to demonstrate Module 1 (Live Monitoring) and Module 2 (Bottleneck Detection)

set -e

echo "================================================================="
echo "   SysPilot Linux Workload & Resource Stress Test Generator     "
echo "================================================================="

DURATION=15
STRESS_MODE=${1:-"all"}

echo "Stress Mode Selected: $STRESS_MODE (Duration: ${DURATION}s)"

cleanup() {
    echo "Stopping background stress tasks..."
    pkill -f "syspilot_stress_cpu" 2>/dev/null || true
    pkill -f "syspilot_stress_io" 2>/dev/null || true
    rm -f /tmp/syspilot_test_io_* 2>/dev/null || true
    echo "Cleanup complete."
}
trap cleanup EXIT INT TERM

case "$STRESS_MODE" in
    cpu)
        echo "Generating heavy CPU load across cores..."
        for i in $(seq 1 $(nproc)); do
            (bash -c 'while true; do :; done' &)
        done
        sleep $DURATION
        ;;
        
    io)
        echo "Generating heavy Disk I/O write storm..."
        dd if=/dev/urandom of=/tmp/syspilot_test_io_1 bs=1M count=2000 oflag=direct &
        dd if=/dev/urandom of=/tmp/syspilot_test_io_2 bs=1M count=2000 oflag=direct &
        wait
        ;;
        
    mem)
        echo "Allocating virtual and resident memory..."
        python3 -c "import time; data = bytearray(1024*1024*1024); time.sleep($DURATION)"
        ;;
        
    all|*)
        echo "Launching combined CPU, Memory, and Disk I/O stress..."
        # 1. CPU Hog
        (bash -c 'while true; do :; done' &)
        # 2. I/O writes
        dd if=/dev/urandom of=/tmp/syspilot_test_io_1 bs=1M count=1000 oflag=direct &
        # 3. Wait
        sleep $DURATION
        ;;
esac

echo "Stress test sequence completed."
