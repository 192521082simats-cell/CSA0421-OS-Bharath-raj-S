/**
 * SysPilot: Design and Performance Evaluation of CPU Scheduling Algorithms
 * with Linux Resource Monitoring Using C
 * 
 * Main Entry Point: src/main.c
 * Unified Capstone CLI Suite integrating Modules 1, 2, and 3
 */

#include "syspilot.h"
#include <getopt.h>

void print_banner(void) {
    printf("======================================================================\n");
    printf("    SysPilot: CPU Scheduling & Linux Resource Monitoring System     \n");
    printf("           Capstone Project Engine v%s (POSIX C Edition)           \n", SYSPILOT_VERSION);
    printf("======================================================================\n\n");
}

void print_usage(const char *prog) {
    printf("Usage: %s [OPTIONS]\n", prog);
    printf("Options:\n");
    printf("  -m, --monitor         Run Module 1: Live System & Process Monitor\n");
    printf("  -a, --analyze         Run Module 2: Intelligent Bottleneck Analyzer\n");
    printf("  -s, --schedule        Run Module 3: CPU Scheduling Algorithms Benchmark\n");
    printf("  -j, --json            Output in JSON format (for Web Dashboard integration)\n");
    printf("  -q, --quantum <int>   Set time quantum for Round Robin / MLFQ (default: 4)\n");
    printf("  -h, --help            Show this help menu\n\n");
}

void run_monitor_cli(bool json_mode) {
    SystemSnapshot snap;
    monitor_init();
    monitor_capture_snapshot(&snap);

    if (json_mode) {
        monitor_export_json(&snap, stdout);
        return;
    }

    printf("[MODULE 1: LIVE SYSTEM MONITORING]\n");
    printf("----------------------------------------------------------------------\n");
    printf("CPU Usage:        %.2f%% (User: %.1f%%, System: %.1f%%, IO-Wait: %.1f%%)\n",
           snap.cpu.total_usage_pct, snap.cpu.user_pct, snap.cpu.system_pct, snap.cpu.iowait_pct);
    printf("Cores Detected:   %d | Context Switches: %lu\n", snap.cpu.core_count, snap.cpu.context_switches);
    printf("Memory:           %.2f%% Used (Total: %.1f MB, Available: %.1f MB)\n",
           snap.mem.memory_usage_pct, (double)snap.mem.total_kb / 1024.0, (double)snap.mem.available_kb / 1024.0);
    printf("Disk Throughput:  Read: %.1f KB/s | Write: %.1f KB/s\n",
           snap.disk.read_kb_per_sec, snap.disk.write_kb_per_sec);
    printf("Uptime:           %.1f seconds | Active Processes: %d\n\n",
           snap.uptime_seconds, snap.process_count);

    printf("%-8s %-20s %-6s %-10s %-12s\n", "PID", "NAME", "STATE", "CPU %", "RSS (MB)");
    printf("----------------------------------------------------------------------\n");
    int limit = (snap.process_count > 10) ? 10 : snap.process_count;
    for (int i = 0; i < limit; i++) {
        printf("%-8d %-20s %-6c %-10.2f %-12.2f\n",
               snap.processes[i].pid,
               snap.processes[i].name,
               snap.processes[i].state,
               snap.processes[i].cpu_usage_pct,
               (double)snap.processes[i].rss_bytes / (1024.0 * 1024.0));
    }
    printf("\n");
}

void run_analyze_cli(bool json_mode) {
    SystemSnapshot snap;
    AnalysisReport report;
    monitor_init();
    monitor_capture_snapshot(&snap);
    analyze_snapshot(&snap, &report);

    if (json_mode) {
        analysis_export_json(&report, stdout);
        return;
    }

    printf("[MODULE 2: INTELLIGENT BOTTLENECK & RESOURCE-CONFLICT ANALYSIS]\n");
    printf("----------------------------------------------------------------------\n");
    printf("System Health Score: %.1f%% | Status: [%s]\n", report.health_score, report.general_status);
    printf("Active Alerts:       %d\n", report.alert_count);
    printf("Resource Conflicts:  %d\n\n", report.conflict_count);

    if (report.alert_count > 0) {
        printf("--- Active Bottlenecks ---\n");
        for (int i = 0; i < report.alert_count; i++) {
            BottleneckAlert *a = &report->alerts[i];
            const char *sev = (a->severity == SEV_CRITICAL) ? "CRITICAL" :
                              (a->severity == SEV_HIGH) ? "HIGH" : "WARNING";
            printf("[%s] %s (%s)\n", sev, a->title, a->component);
            printf("  Detail:         %s\n", a->description);
            printf("  Recommendation: %s\n\n", a->recommendation);
        }
    } else {
        printf("No critical resource bottlenecks detected. Operating inside nominal thresholds.\n\n");
    }

    if (report.conflict_count > 0) {
        printf("--- Detected Resource Conflicts ---\n");
        for (int i = 0; i < report.conflict_count; i++) {
            ResourceConflict *c = &report->conflicts[i];
            printf("Conflict [%s]: PID %d ('%s') vs PID %d ('%s')\n",
                   c->conflict_resource, c->pid_a, c->name_a, c->pid_b, c->name_b);
            printf("  Detail: %s\n\n", c->details);
        }
    }
}

void run_scheduler_cli(int quantum, bool json_mode) {
    SchedProcess workload[] = {
        {1, "P1 (Compiler)", 0, 8, 8, 2, 0, -1, 0, 0, 0, -1, 0.0, 0, false},
        {2, "P2 (Web Svc)",  1, 4, 4, 1, -5, -1, 0, 0, 0, -1, 0.0, 0, false},
        {3, "P3 (DB Query)", 2, 9, 9, 3, 5, -1, 0, 0, 0, -1, 0.0, 0, false},
        {4, "P4 (Telemetry)",3, 5, 5, 2, 0, -1, 0, 0, 0, -1, 0.0, 0, false},
        {5, "P5 (Backup)",   4, 2, 2, 4, 10, -1, 0, 0, 0, -1, 0.0, 0, false}
    };
    int count = sizeof(workload) / sizeof(workload[0]);

    SchedulingAlgorithm algos[] = {
        ALGO_FCFS,
        ALGO_SJF_NONPREEMPT,
        ALGO_SRTF_PREEMPT,
        ALGO_ROUND_ROBIN,
        ALGO_PRIORITY_PREEMPT,
        ALGO_MLFQ,
        ALGO_CFS
    };
    int algo_count = sizeof(algos) / sizeof(algos[0]);

    if (json_mode) {
        SchedEvaluationReport rep;
        schedule_simulate(ALGO_ROUND_ROBIN, workload, count, quantum, &rep);
        schedule_export_json(&rep, stdout);
        return;
    }

    printf("[MODULE 3: CPU SCHEDULING ALGORITHMS & OS OPTIMIZATION EVALUATION]\n");
    printf("Evaluating %d Benchmark Tasks across %d Core Scheduling Algorithms (Quantum = %d):\n",
           count, algo_count, quantum);
    printf("-----------------------------------------------------------------------------------------\n");
    printf("%-32s %-10s %-10s %-10s %-10s %-10s\n",
           "Algorithm", "Avg WT", "Avg TAT", "Avg RT", "Fairness", "Ctx Switch");
    printf("-----------------------------------------------------------------------------------------\n");

    for (int i = 0; i < algo_count; i++) {
        SchedEvaluationReport rep;
        schedule_simulate(algos[i], workload, count, quantum, &rep);
        printf("%-32s %-10.2f %-10.2f %-10.2f %-10.3f %-10d\n",
               rep.algo_name, rep.avg_waiting_time, rep.avg_turnaround_time,
               rep.avg_response_time, rep.jains_fairness_index, rep.context_switches);
    }
    printf("-----------------------------------------------------------------------------------------\n\n");

    /* Print In-Depth Analysis for Round Robin & CFS */
    SchedEvaluationReport rr_rep, cfs_rep;
    schedule_simulate(ALGO_ROUND_ROBIN, workload, count, quantum, &rr_rep);
    schedule_simulate(ALGO_CFS, workload, count, quantum, &cfs_rep);

    printf("--- OS Algorithmic Optimization Insights ---\n");
    printf("1. Round Robin:\n   %s\n\n", rr_rep.optimization_recommendations);
    printf("2. Linux Completely Fair Scheduler (CFS):\n   %s\n\n", cfs_rep.optimization_recommendations);
}

int main(int argc, char *argv[]) {
    bool run_mon = false;
    bool run_ana = false;
    bool run_sch = false;
    bool json_mode = false;
    int quantum = 4;

    static struct option long_options[] = {
        {"monitor",  no_argument,       0, 'm'},
        {"analyze",  no_argument,       0, 'a'},
        {"schedule", no_argument,       0, 's'},
        {"json",     no_argument,       0, 'j'},
        {"quantum",  required_argument, 0, 'q'},
        {"help",     no_argument,       0, 'h'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "masjq:h", long_options, NULL)) != -1) {
        switch (opt) {
            case 'm': run_mon = true; break;
            case 'a': run_ana = true; break;
            case 's': run_sch = true; break;
            case 'j': json_mode = true; break;
            case 'q': quantum = atoi(optarg); break;
            case 'h':
            default:
                print_banner();
                print_usage(argv[0]);
                return 0;
        }
    }

    if (!run_mon && !run_ana && !run_sch) {
        /* Default: Run full suite */
        run_mon = true;
        run_ana = true;
        run_sch = true;
    }

    if (!json_mode) {
        print_banner();
    }

    if (run_mon) run_monitor_cli(json_mode);
    if (run_ana) run_analyze_cli(json_mode);
    if (run_sch) run_scheduler_cli(quantum, json_mode);

    return 0;
}
