/**
 * SysPilot: Module 3 - OS Performance Analysis and Optimization
 * optimizer.c - Recommendations, Linux kernel tuning advisories, and report generation
 */

#include "optimizer.h"

void optimizer_init(void) {
    /* Setup optimizer engine */
}

/* Helper to calculate 80th percentile burst time for quantum sizing */
static int calculate_80th_percentile_burst(const sched_task_t *tasks, int count) {
    if (count <= 0) return 4;
    int bursts[MAX_SCHED_TASKS];
    for (int i = 0; i < count; i++) bursts[i] = tasks[i].burst_time;
    
    /* Simple bubble sort */
    for (int i = 0; i < count - 1; i++) {
        for (int j = 0; j < count - i - 1; j++) {
            if (bursts[j] > bursts[j+1]) {
                int tmp = bursts[j];
                bursts[j] = bursts[j+1];
                bursts[j+1] = tmp;
            }
        }
    }
    int idx = (int)(count * 0.80);
    if (idx >= count) idx = count - 1;
    int q = bursts[idx];
    if (q < 2) q = 2;
    if (q > 12) q = 12;
    return q;
}

void optimizer_analyze_and_plan(
    const system_snapshot_t *snap,
    const analysis_report_t *analysis,
    const sched_eval_result_t sched_results[SCHED_COUNT],
    workload_type_t workload_type,
    optimization_plan_t *plan
) {
    if (!plan) return;
    memset(plan, 0, sizeof(*plan));
    plan->detected_workload = workload_type;
    safe_strncpy(plan->workload_name, get_workload_name(workload_type), sizeof(plan->workload_name));
    
    /* 1. Evaluate Best Algorithm from Simulation Results */
    float best_score = -999999.0f;
    sched_algo_t best_algo = SCHED_MLFQ;
    
    for (int i = 0; i < SCHED_COUNT; i++) {
        const sched_eval_result_t *r = &sched_results[i];
        /* Multi-objective score: low waiting time, low turnaround time, high fairness, low starvation */
        float score = (100.0f / (r->avg_waiting_time + 1.0f)) * 0.40f +
                      (100.0f / (r->avg_turnaround_time + 1.0f)) * 0.30f +
                      (r->jains_fairness_index * 50.0f) * 0.20f -
                      (r->starved_task_count * 15.0f);
                      
        if (score > best_score) {
            best_score = score;
            best_algo = r->algo;
        }
    }
    plan->optimal_algo = best_algo;
    safe_strncpy(plan->optimal_algo_name, get_algo_name(best_algo), sizeof(plan->optimal_algo_name));
    
    /* 2. Workload Characteristics & Mathematical Justification */
    switch (workload_type) {
        case WORKLOAD_CPU_BOUND:
            safe_strncpy(plan->workload_characteristics,
                         "Workload is characterized by prolonged CPU bursts with minimal I/O waits. Long compute tasks saturate execution pipelines.",
                         sizeof(plan->workload_characteristics));
            snprintf(plan->algo_justification, sizeof(plan->algo_justification),
                     "%s is mathematically optimal. It minimizes the Average Turnaround Time (TAT) by giving compute priority to remaining burst lengths, while MLFQ prevents starvation of newly arriving interactive threads.",
                     plan->optimal_algo_name);
            break;
            
        case WORKLOAD_IO_BOUND:
            safe_strncpy(plan->workload_characteristics,
                         "Workload consists of frequent short CPU bursts interspersed with high volume disk and socket I/O waits.",
                         sizeof(plan->workload_characteristics));
            snprintf(plan->algo_justification, sizeof(plan->algo_justification),
                     "%s is recommended. Fast context dispatching ensures tasks issue I/O requests immediately, keeping disk and network pipelines saturated for maximum system throughput.",
                     plan->optimal_algo_name);
            break;
            
        case WORKLOAD_INTERACTIVE:
            safe_strncpy(plan->workload_characteristics,
                         "High volume of human-interactive or low-latency requests requiring immediate responsiveness.",
                         sizeof(plan->workload_characteristics));
            snprintf(plan->algo_justification, sizeof(plan->algo_justification),
                     "%s delivers the lowest Average Response Time (ART), guaranteeing that no user process waits behind long batch computations.",
                     plan->optimal_algo_name);
            break;
            
        case WORKLOAD_BURSTY:
            safe_strncpy(plan->workload_characteristics,
                         "Unpredictable arrival bursts with high variance in task execution durations.",
                         sizeof(plan->workload_characteristics));
            snprintf(plan->algo_justification, sizeof(plan->algo_justification),
                     "Multilevel Feedback Queue (MLFQ) dynamically categorizes tasks into priority tiers, penalizing CPU-hogs into lower queues while boosting responsive interactive tasks.",
                     plan->optimal_algo_name);
            break;
            
        case WORKLOAD_BALANCED:
        default:
            safe_strncpy(plan->workload_characteristics,
                         "Heterogeneous general-purpose server workload with mixed compute, disk, and daemon background tasks.",
                         sizeof(plan->workload_characteristics));
            snprintf(plan->algo_justification, sizeof(plan->algo_justification),
                     "%s achieves the highest overall balance of low Waiting Time, high Jain's Fairness Index, and zero task starvation.",
                     plan->optimal_algo_name);
            break;
    }
    
    /* 3. Optimal Quantum Sizing Calculation */
    plan->recommended_quantum = calculate_80th_percentile_burst(sched_results[SCHED_RR].completed_tasks,
                                                                 sched_results[SCHED_RR].task_count);
                                                                 
    /* 4. Linux Kernel Tuning Advisories */
    int t_idx = 0;
    
    /* CPU Tuning */
    kernel_tune_advise_t *t1 = &plan->kernel_tunings[t_idx++];
    safe_strncpy(t1->parameter, "kernel.sched_latency_ns", sizeof(t1->parameter));
    safe_strncpy(t1->current_behavior, "Default 6ms Linux CFS target scheduling period", sizeof(t1->current_behavior));
    if (analysis && analysis->cpu_bottleneck_flag) {
        safe_strncpy(t1->recommended_command, "sysctl -w kernel.sched_latency_ns=12000000", sizeof(t1->recommended_command));
        safe_strncpy(t1->rational, "Lengthen CFS period to 12ms to reduce context-switch frequency and cache invalidations under heavy load.", sizeof(t1->rational));
        t1->expected_impact_pct = 18;
    } else {
        safe_strncpy(t1->recommended_command, "sysctl -w kernel.sched_latency_ns=6000000", sizeof(t1->recommended_command));
        safe_strncpy(t1->rational, "Maintain standard 6ms latency for responsive desktop and interactive workloads.", sizeof(t1->rational));
        t1->expected_impact_pct = 5;
    }
    
    /* Granularity Tuning */
    kernel_tune_advise_t *t2 = &plan->kernel_tunings[t_idx++];
    safe_strncpy(t2->parameter, "kernel.sched_min_granularity_ns", sizeof(t2->parameter));
    safe_strncpy(t2->current_behavior, "Default minimum execution slice before preemption (0.75ms)", sizeof(t2->current_behavior));
    safe_strncpy(t2->recommended_command, "sysctl -w kernel.sched_min_granularity_ns=2000000", sizeof(t2->recommended_command));
    safe_strncpy(t2->rational, "Increase min granularity to 2ms to prevent CPU thrashing during multi-task competition.", sizeof(t2->rational));
    t2->expected_impact_pct = 14;
    
    /* Memory / Swap Tuning */
    kernel_tune_advise_t *t3 = &plan->kernel_tunings[t_idx++];
    safe_strncpy(t3->parameter, "vm.swappiness", sizeof(t3->parameter));
    safe_strncpy(t3->current_behavior, "Default Linux aggressiveness in paging anonymous memory to disk (60)", sizeof(t3->current_behavior));
    if (snap && snap->mem.swap_util_percent > 30.0f) {
        safe_strncpy(t3->recommended_command, "sysctl -w vm.swappiness=10", sizeof(t3->recommended_command));
        safe_strncpy(t3->rational, "Lower swappiness from 60 to 10 to suppress aggressive swap thrashing and maintain process pages in RAM.", sizeof(t3->rational));
        t3->expected_impact_pct = 25;
    } else {
        safe_strncpy(t3->recommended_command, "sysctl -w vm.swappiness=15", sizeof(t3->recommended_command));
        safe_strncpy(t3->rational, "Moderate swappiness to favor retaining application working sets over disk page caches.", sizeof(t3->rational));
        t3->expected_impact_pct = 8;
    }
    
    /* VFS Cache Pressure */
    kernel_tune_advise_t *t4 = &plan->kernel_tunings[t_idx++];
    safe_strncpy(t4->parameter, "vm.vfs_cache_pressure", sizeof(t4->parameter));
    safe_strncpy(t4->current_behavior, "Kernel tendency to reclaim directory and inode objects (100)", sizeof(t4->current_behavior));
    safe_strncpy(t4->recommended_command, "sysctl -w vm.vfs_cache_pressure=50", sizeof(t4->recommended_command));
    safe_strncpy(t4->rational, "Reduce directory inode eviction rate to accelerate recursive file lookups and process execs.", sizeof(t4->rational));
    t4->expected_impact_pct = 10;
    
    plan->tuning_count = t_idx;
    
    /* 5. Process Prioritization & Pinning Advice */
    if (analysis && analysis->conflict_count > 0) {
        snprintf(plan->process_prioritization_advice, sizeof(plan->process_prioritization_advice),
                 "Resource Contention Detected: Pin conflicting compute tasks across distinct NUMA nodes/cores: 'taskset -cp 0-3 %d' and 'taskset -cp 4-7 %d'. Renice background jobs to +10.",
                 analysis->conflicts[0].pids[0], analysis->conflicts[0].pids[1]);
    } else {
        safe_strncpy(plan->process_prioritization_advice,
                     "Apply 'renice -n -5 -p <PID>' for high-priority interactive services, and 'renice -n 15 -p <PID>' for background compilation and indexing jobs.",
                     sizeof(plan->process_prioritization_advice));
    }
    
    safe_strncpy(plan->memory_tuning_advice,
                 "Set vm.dirty_ratio=15 and vm.dirty_background_ratio=5 to smooth background writeback and prevent sudden write stalls during heavy disk sync.",
                 sizeof(plan->memory_tuning_advice));
                 
    safe_strncpy(plan->io_tuning_advice,
                 "Assign Real-Time / Best-Effort High I/O class to latency-critical processes using 'ionice -c 2 -n 0 -p <PID>', and Idle class to backups via 'ionice -c 3 -p <PID>'.",
                 sizeof(plan->io_tuning_advice));
}

void optimizer_render_terminal_plan(const optimization_plan_t *plan) {
    if (!plan) return;
    
    printf("\n%s========================================================================================================%s\n", ANSI_BOLD, ANSI_RESET);
    printf("  %s%sSysPilot Module 3: OS Performance Optimization & Tuning Plan%s\n", ANSI_BOLD, ANSI_CYAN, ANSI_RESET);
    printf("  Detected Workload:  %s%s%s\n", ANSI_BOLD, plan->workload_name, ANSI_RESET);
    printf("  Workload Behavior:  %s\n", plan->workload_characteristics);
    printf("  %sRecommended Policy: %s%s%s (Optimal Time Quantum: %s%d ticks%s)\n",
           ANSI_BOLD, ANSI_GREEN, plan->optimal_algo_name, ANSI_RESET, ANSI_YELLOW, plan->recommended_quantum, ANSI_RESET);
    printf("  Justification:      %s\n", plan->algo_justification);
    printf("%s--------------------------------------------------------------------------------------------------------%s\n", ANSI_DIM, ANSI_RESET);
    printf("  %sRecommended Linux Kernel Sysctl Tuning Parameters:%s\n", ANSI_BOLD, ANSI_RESET);
    
    for (int i = 0; i < plan->tuning_count; i++) {
        const kernel_tune_advise_t *t = &plan->kernel_tunings[i];
        printf("  %d. %s%-32s%s [Impact: %s+%d%%%s]\n",
               i + 1, ANSI_YELLOW, t->parameter, ANSI_RESET, ANSI_GREEN, t->expected_impact_pct, ANSI_RESET);
        printf("     Command:  %s%s%s\n", ANSI_BOLD, t->recommended_command, ANSI_RESET);
        printf("     Rationale: %s\n", t->rational);
    }
    
    printf("\n  %sProcess & Scheduling Tuning:%s\n  %s\n", ANSI_BOLD, ANSI_RESET, plan->process_prioritization_advice);
    printf("\n  %sStorage & Memory Tuning:%s\n  %s\n", ANSI_BOLD, ANSI_RESET, plan->memory_tuning_advice);
    printf("  %s\n", plan->io_tuning_advice);
    printf("%s========================================================================================================%s\n", ANSI_BOLD, ANSI_RESET);
}

int optimizer_export_markdown_report(
    const char *filepath,
    const system_snapshot_t *snap,
    const analysis_report_t *analysis,
    const sched_eval_result_t sched_results[SCHED_COUNT],
    const optimization_plan_t *plan
) {
    if (!filepath || !plan) return -1;
    FILE *f = fopen(filepath, "w");
    if (!f) return -1;
    
    time_t rawtime;
    time(&rawtime);
    struct tm *info = localtime(&rawtime);
    char date_str[64];
    strftime(date_str, sizeof(date_str), "%Y-%m-%d %H:%M:%S", info);
    
    fprintf(f, "# SysPilot: Capstone Project Evaluation & Performance Report\n\n");
    fprintf(f, "**Project Title:** SysPilot: Design and Performance Evaluation of CPU Scheduling Algorithms with Linux Resource Monitoring Using C  \n");
    fprintf(f, "**Generated On:** %s  \n", date_str);
    fprintf(f, "**Execution Mode:** %s  \n\n", (snap && monitor_is_simulated_mode()) ? "Simulated ProcFS (Cross-Platform)" : "Live Linux Kernel (/proc)");
    
    fprintf(f, "---\n\n");
    fprintf(f, "## Executive Summary\n\n");
    fprintf(f, "SysPilot provides an end-to-end framework integrating **real-time Linux resource monitoring**, **intelligent bottleneck analysis**, and **CPU scheduling algorithm evaluation**. This report synthesizes metrics captured across all three project modules and presents empirical performance evaluations along with actionable OS kernel tuning advisories.\n\n");
    
    /* Module 1 Summary */
    if (snap) {
        fprintf(f, "## Module 1: Live System and Process Monitoring\n\n");
        fprintf(f, "| Subsystem | Current Metric | Observed State |\n");
        fprintf(f, "| :--- | :--- | :--- |\n");
        fprintf(f, "| **CPU Overall** | `%.1f%%` | User: `%.1f%%`, System: `%.1f%%`, IO-Wait: `%.1f%%` |\n",
                snap->cpu.overall.cpu_percent, snap->cpu.overall.user_percent, snap->cpu.overall.system_percent, snap->cpu.overall.iowait_percent);
        fprintf(f, "| **Load Average** | `%.2f, %.2f, %.2f` | Cores: `%d` |\n",
                snap->load_avg[0], snap->load_avg[1], snap->load_avg[2], snap->cpu.core_count);
        fprintf(f, "| **Memory (RAM)** | `%.1f%%` Used | %llu MB Used / %llu MB Total |\n",
                snap->mem.mem_util_percent, snap->mem.used_kb / 1024, snap->mem.total_kb / 1024);
        fprintf(f, "| **Swap Memory** | `%.1f%%` Used | %llu MB Used / %llu MB Total |\n",
                snap->mem.swap_util_percent, snap->mem.swap_used_kb / 1024, snap->mem.swap_total_kb / 1024);
        fprintf(f, "| **Disk Throughput** | Read: `%.1f KB/s`, Write: `%.1f KB/s` | Max Device Busy: `%.1f%%` |\n",
                snap->disk.total_read_kb_sec, snap->disk.total_write_kb_sec, snap->disk.max_disk_util_percent);
        fprintf(f, "| **Task Breakdown** | `%d` Total | Running: `%d`, Sleeping: `%d`, D-State: `%d` |\n\n",
                snap->total_tasks, snap->running_tasks, snap->sleeping_tasks, snap->disk_sleep_tasks);
    }
    
    /* Module 2 Summary */
    if (analysis) {
        fprintf(f, "## Module 2: Intelligent Bottleneck & Resource-Conflict Analysis\n\n");
        fprintf(f, "- **System Health Score:** `%.1f / 100.0`\n", analysis->system_health_score);
        fprintf(f, "- **Primary Diagnostic:** %s\n", analysis->primary_diagnostic);
        fprintf(f, "- **Root Cause Analysis:** %s\n\n", analysis->root_cause_analysis);
        
        if (analysis->alert_count > 0) {
            fprintf(f, "### Active Anomaly & Bottleneck Alerts\n\n");
            fprintf(f, "| Severity | Category | Alert Title | Culprit | Measured Value | Threshold | Actionable Recommendation |\n");
            fprintf(f, "| :--- | :--- | :--- | :--- | :--- | :--- | :--- |\n");
            for (int i = 0; i < analysis->alert_count; i++) {
                const bottleneck_alert_t *a = &analysis->alerts[i];
                fprintf(f, "| **%s** | %s | %s | `%s` | `%.1f` | `%.1f` | %s |\n",
                        get_severity_str(a->severity),
                        (a->category == BOTTLE_CPU) ? "CPU" : (a->category == BOTTLE_MEM) ? "Memory" : (a->category == BOTTLE_IO) ? "Disk I/O" : "Conflict",
                        a->title, a->culprit_name, a->metric_value, a->threshold_value, a->recommendation);
            }
            fprintf(f, "\n");
        }
    }
    
    /* Module 3 Scheduler Results */
    if (sched_results) {
        fprintf(f, "## Module 3: CPU Scheduling Algorithms Evaluation\n\n");
        fprintf(f, "The table below benchmarks the 7 implemented scheduling algorithms on the **%s** workload:\n\n", plan->workload_name);
        fprintf(f, "| Scheduling Algorithm | Avg Waiting Time (AWT) | Avg Turnaround Time (TAT) | Avg Response Time (ART) | CPU Util %% | Throughput | Jain's Fairness Index | Context Switches |\n");
        fprintf(f, "| :--- | :--- | :--- | :--- | :--- | :--- | :--- | :--- |\n");
        for (int i = 0; i < SCHED_COUNT; i++) {
            const sched_eval_result_t *r = &sched_results[i];
            fprintf(f, "| **%s** | `%.2f` | `%.2f` | `%.2f` | `%.1f%%` | `%.3f` | `%.3f` | `%d` |\n",
                    r->algo_name, r->avg_waiting_time, r->avg_turnaround_time, r->avg_response_time,
                    r->cpu_utilization_pct, r->throughput, r->jains_fairness_index, r->context_switches);
        }
        fprintf(f, "\n");
    }
    
    /* Recommendations & Kernel Tuning */
    fprintf(f, "## Module 3: OS Performance Optimization & Kernel Tuning Advisories\n\n");
    fprintf(f, "### 1. Optimal Scheduling Policy\n");
    fprintf(f, "- **Recommended Policy:** `%s`\n", plan->optimal_algo_name);
    fprintf(f, "- **Recommended Time Quantum:** `%d ticks`\n", plan->recommended_quantum);
    fprintf(f, "- **Theoretical Justification:** %s\n\n", plan->algo_justification);
    
    fprintf(f, "### 2. Linux Kernel Sysctl Tuning Parameters\n\n");
    fprintf(f, "| Parameter | Current Baseline | Recommended Tuning Command | Rationale | Expected Impact |\n");
    fprintf(f, "| :--- | :--- | :--- | :--- | :--- |\n");
    for (int i = 0; i < plan->tuning_count; i++) {
        const kernel_tune_advise_t *t = &plan->kernel_tunings[i];
        fprintf(f, "| `%s` | %s | `%s` | %s | **+%d%%** |\n",
                t->parameter, t->current_behavior, t->recommended_command, t->rational, t->expected_impact_pct);
    }
    fprintf(f, "\n");
    
    fprintf(f, "### 3. Application Priority & Storage Tuning\n");
    fprintf(f, "- **Process Tuning:** %s\n", plan->process_prioritization_advice);
    fprintf(f, "- **Virtual Memory Tuning:** %s\n", plan->memory_tuning_advice);
    fprintf(f, "- **Block I/O Tuning:** %s\n\n", plan->io_tuning_advice);
    
    fprintf(f, "---\n*SysPilot Capstone System Engine - Autonomous Linux OS Diagnostics and Scheduling Optimization*\n");
    
    fclose(f);
    return 0;
}

int optimizer_export_csv_metrics(const char *filepath, const sched_eval_result_t sched_results[SCHED_COUNT]) {
    if (!filepath || !sched_results) return -1;
    FILE *f = fopen(filepath, "w");
    if (!f) return -1;
    
    fprintf(f, "Algorithm,AvgWaitingTime,AvgTurnaroundTime,AvgResponseTime,CPUUtilizationPct,Throughput,JainsFairnessIndex,ContextSwitches,StarvedTasks\n");
    for (int i = 0; i < SCHED_COUNT; i++) {
        const sched_eval_result_t *r = &sched_results[i];
        fprintf(f, "\"%s\",%.3f,%.3f,%.3f,%.2f,%.4f,%.4f,%d,%d\n",
                r->algo_name, r->avg_waiting_time, r->avg_turnaround_time, r->avg_response_time,
                r->cpu_utilization_pct, r->throughput, r->jains_fairness_index,
                r->context_switches, r->starved_task_count);
    }
    fclose(f);
    return 0;
}
