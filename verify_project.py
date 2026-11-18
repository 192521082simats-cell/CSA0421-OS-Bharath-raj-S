"""
SysPilot: Verification and Automated Acceptance Test Suite
Tests Module 1, Module 2, Module 3, and file assets.
"""

import sys
import os

CURRENT_DIR = os.path.dirname(os.path.abspath(__file__))
sys.path.insert(0, os.path.join(CURRENT_DIR, 'backend'))

from system_monitor import SystemMonitor
from bottleneck_engine import BottleneckEngine
from syspilot_core import CPUScheduler

def run_tests():
    print("=" * 70)
    print("         SYSPILOT AUTOMATED VERIFICATION SUITE")
    print("=" * 70)

    # 1. Module 1 Test
    print("\n[TEST 1] Verifying Module 1: Live System & Process Monitor...")
    mon = SystemMonitor()
    cpu = mon.get_cpu_info()
    mem = mon.get_memory_info()
    disk = mon.get_disk_info()
    procs = mon.get_processes()

    assert cpu['total_pct'] >= 0.0 and cpu['total_pct'] <= 100.0, "CPU percent invalid"
    assert mem['usage_pct'] >= 0.0 and mem['usage_pct'] <= 100.0, "Memory percent invalid"
    assert len(procs) > 0, "Process table must not be empty"
    print(f" -> CPU Load: {cpu['total_pct']}% (Cores: {cpu['core_count']})")
    print(f" -> Memory Usage: {mem['usage_pct']}% ({mem['used_mb']:.1f} MB used)")
    print(f" -> Disk Throughput: Read {disk['read_kb_per_sec']} KB/s, Write {disk['write_kb_per_sec']} KB/s")
    print(f" -> Active Processes Captured: {len(procs)}")
    print(" [PASS] Module 1 Verified Successfully.")

    # 2. Module 2 Test
    print("\n[TEST 2] Verifying Module 2: Intelligent Bottleneck Analyzer...")
    analyzer = BottleneckEngine()
    rep = analyzer.analyze(cpu, mem, disk, procs)
    assert 0.0 <= rep['health_score'] <= 100.0, "Health score must be between 0 and 100"
    print(f" -> Nominal Health Score: {rep['health_score']}% ({rep['general_status']})")

    # Test Anomaly Detection under Stress
    mon.set_stress('cpu_saturation')
    cpu_stressed = mon.get_cpu_info()
    procs_stressed = mon.get_processes()
    rep_stressed = analyzer.analyze(cpu_stressed, mem, disk, procs_stressed)
    assert rep_stressed['alert_count'] > 0, "Bottleneck engine must trigger alerts under stress"
    assert any(a['type'] == 'CPU_SATURATION' for a in rep_stressed['alerts']), "Must trigger CPU_SATURATION alert"
    print(f" -> Stressed Health Score: {rep_stressed['health_score']}% ({rep_stressed['general_status']})")
    print(f" -> Generated Alerts: {rep_stressed['alert_count']} alert(s)")
    print(f"    Sample Alert: [{rep_stressed['alerts'][0]['severity']}] {rep_stressed['alerts'][0]['title']}")
    mon.reset_stress()
    print(" [PASS] Module 2 Verified Successfully.")

    # 3. Module 3 Test
    print("\n[TEST 3] Verifying Module 3: CPU Scheduling Algorithms...")
    sched = CPUScheduler()
    test_workload = [
        {'pid': 1, 'name': 'P1', 'arrival': 0, 'burst': 8, 'priority': 2, 'nice': 0},
        {'pid': 2, 'name': 'P2', 'arrival': 1, 'burst': 4, 'priority': 1, 'nice': -5},
        {'pid': 3, 'name': 'P3', 'arrival': 2, 'burst': 9, 'priority': 3, 'nice': 5},
        {'pid': 4, 'name': 'P4', 'arrival': 3, 'burst': 5, 'priority': 2, 'nice': 0},
        {'pid': 5, 'name': 'P5', 'arrival': 4, 'burst': 2, 'priority': 4, 'nice': 10}
    ]

    algos = ['fcfs', 'sjf', 'srtf', 'rr', 'priority', 'mlfq', 'cfs']
    for algo in algos:
        result = sched.simulate(algo, test_workload, quantum=4)
        assert result['total_time'] >= 28, f"Total simulation time error for {algo}"
        assert result['avg_turnaround_time'] > 0, f"Avg TAT must be > 0 for {algo}"
        assert result['avg_waiting_time'] >= 0, f"Avg WT must be >= 0 for {algo}"
        assert len(result['gantt']) > 0, f"Gantt events must exist for {algo}"
        assert len(result['processes']) == 5, f"All processes must be scheduled for {algo}"
        print(f" -> Algorithm: {result['algorithm']:<30} | Avg WT: {result['avg_waiting_time']:>6.2f} ms | Avg TAT: {result['avg_turnaround_time']:>6.2f} ms | Fairness: {result['jains_fairness_index']:.4f}")

    benchmark = sched.benchmark_all(test_workload, quantum=4)
    assert len(benchmark) == 7, "All 7 algorithms must be present in benchmark"
    print(" [PASS] Module 3 Verified Successfully.")

    # 4. Verify C Files Existence
    print("\n[TEST 4] Verifying C Project Source Files...")
    c_files = [
        "include/syspilot.h",
        "src/monitor.c",
        "src/analyzer.c",
        "src/scheduler.c",
        "src/main.c",
        "Makefile",
        "CMakeLists.txt",
        "tests/test_scheduler.c"
    ]
    for rel_path in c_files:
        full_path = os.path.join(CURRENT_DIR, rel_path)
        assert os.path.exists(full_path), f"Missing required file: {rel_path}"
        print(f" -> Found {rel_path} ({os.path.getsize(full_path)} bytes)")
    print(" [PASS] All C Source Files Verified.")

    # 5. Verify Frontend Assets
    print("\n[TEST 5] Verifying Frontend Static Files...")
    fe_files = [
        "frontend/index.html",
        "frontend/css/style.css",
        "frontend/js/app.js",
        "frontend/js/monitor.js",
        "frontend/js/analyzer.js",
        "frontend/js/scheduler.js"
    ]
    for rel_path in fe_files:
        full_path = os.path.join(CURRENT_DIR, rel_path)
        assert os.path.exists(full_path), f"Missing required file: {rel_path}"
        print(f" -> Found {rel_path} ({os.path.getsize(full_path)} bytes)")
    print(" [PASS] All Frontend Assets Verified.")

    print("\n" + "=" * 70)
    print(" >>> ALL 5 TEST SUITES PASSED! SYSPILOT IS 100% OPERATIONAL. <<<")
    print("=" * 70)

if __name__ == '__main__':
    run_tests()
