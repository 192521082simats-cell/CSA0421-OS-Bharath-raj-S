/**
 * SysPilot: Module 2 - Intelligent Bottleneck and Resource-Conflict Analysis Using C
 * Implementation: src/analyzer.c
 * 
 * Heuristics & Statistical Inference Rules:
 *   Rule 1: CPU Saturation & Runqueue Thrashing
 *   Rule 2: Memory Pressure & Swap Thrashing
 *   Rule 3: I/O Wait Storage Starvation (Blocked Processes)
 *   Rule 4: Multi-Process Resource Contention & Priority Inversion
 *   Rule 5: Context Switch Storm & Lock Convoy
 */

#include "syspilot.h"

int analyze_snapshot(const SystemSnapshot *snapshot, AnalysisReport *report) {
    if (!snapshot || !report) return -1;
    memset(report, 0, sizeof(AnalysisReport));

    int alert_idx = 0;
    int conflict_idx = 0;
    double health_deduction = 0.0;

    /* Find Top CPU and Top Memory Hogs */
    const ProcessEntry *top_cpu_proc = NULL;
    const ProcessEntry *top_mem_proc = NULL;
    int blocked_d_state_count = 0;

    for (int i = 0; i < snapshot->process_count; i++) {
        const ProcessEntry *p = &snapshot->processes[i];
        if (!top_cpu_proc || p->cpu_usage_pct > top_cpu_proc->cpu_usage_pct) {
            top_cpu_proc = p;
        }
        if (!top_mem_proc || p->rss_bytes > top_mem_proc->rss_bytes) {
            top_mem_proc = p;
        }
        if (p->state == 'D') {
            blocked_d_state_count++;
        }
    }

    /* -----------------------------------------------------------------
     * RULE 1: CPU Saturation & Runqueue Thrashing
     * ----------------------------------------------------------------- */
    if (snapshot->cpu.total_usage_pct >= 85.0) {
        BottleneckAlert *a = &report->alerts[alert_idx++];
        a->type = BOTTLENECK_CPU_SATURATION;
        a->severity = SEV_CRITICAL;
        strncpy(a->component, "CPU Subsystem", sizeof(a->component) - 1);
        strncpy(a->title, "Severe CPU Saturation Detected", sizeof(a->title) - 1);
        snprintf(a->description, sizeof(a->description),
                 "Aggregate CPU usage is at %.1f%% exceeding critical threshold (85%%). Runqueue load is elevated.",
                 snapshot->cpu.total_usage_pct);
        if (top_cpu_proc) {
            a->culprit_pid = top_cpu_proc->pid;
            strncpy(a->culprit_name, top_cpu_proc->name, sizeof(a->culprit_name) - 1);
            snprintf(a->recommendation, sizeof(a->recommendation),
                     "Throttle or renice PID %d ('%s') consuming %.1f%% CPU. Consider CFS latency optimization.",
                     top_cpu_proc->pid, top_cpu_proc->name, top_cpu_proc->cpu_usage_pct);
        }
        a->metric_value = snapshot->cpu.total_usage_pct;
        a->threshold_value = 85.0;
        health_deduction += 35.0;
    } else if (snapshot->cpu.total_usage_pct >= 70.0) {
        BottleneckAlert *a = &report->alerts[alert_idx++];
        a->type = BOTTLENECK_CPU_SATURATION;
        a->severity = SEV_MEDIUM;
        strncpy(a->component, "CPU Subsystem", sizeof(a->component) - 1);
        strncpy(a->title, "Moderate CPU Utilization Pressure", sizeof(a->title) - 1);
        snprintf(a->description, sizeof(a->description),
                 "Aggregate CPU usage at %.1f%%. High processing demands active.",
                 snapshot->cpu.total_usage_pct);
        strncpy(a->recommendation, "Evaluate CPU core affinity and load balancing across runqueues.", sizeof(a->recommendation) - 1);
        a->metric_value = snapshot->cpu.total_usage_pct;
        a->threshold_value = 70.0;
        health_deduction += 15.0;
    }

    /* -----------------------------------------------------------------
     * RULE 2: Memory Pressure & Swap Thrashing
     * ----------------------------------------------------------------- */
    if (snapshot->mem.memory_usage_pct >= 90.0) {
        BottleneckAlert *a = &report->alerts[alert_idx++];
        a->type = BOTTLENECK_MEMORY_EXHAUSTION;
        a->severity = SEV_CRITICAL;
        strncpy(a->component, "Memory", sizeof(a->component) - 1);
        strncpy(a->title, "Critical Memory Exhaustion (OOM Danger)", sizeof(a->title) - 1);
        snprintf(a->description, sizeof(a->description),
                 "Physical RAM consumption at %.1f%%. Kernel OOM-killer may trigger imminent process terminations.",
                 snapshot->mem.memory_usage_pct);
        if (top_mem_proc) {
            a->culprit_pid = top_mem_proc->pid;
            strncpy(a->culprit_name, top_mem_proc->name, sizeof(a->culprit_name) - 1);
            snprintf(a->recommendation, sizeof(a->recommendation),
                     "Investigate memory leak or scale resources for PID %d ('%s', RSS: %.1f MB).",
                     top_mem_proc->pid, top_mem_proc->name, (double)top_mem_proc->rss_bytes / (1024.0 * 1024.0));
        }
        a->metric_value = snapshot->mem.memory_usage_pct;
        a->threshold_value = 90.0;
        health_deduction += 35.0;
    } else if (snapshot->mem.memory_usage_pct >= 80.0) {
        BottleneckAlert *a = &report->alerts[alert_idx++];
        a->type = BOTTLENECK_MEMORY_EXHAUSTION;
        a->severity = SEV_MEDIUM;
        strncpy(a->component, "Memory", sizeof(a->component) - 1);
        strncpy(a->title, "High RAM Allocation", sizeof(a->title) - 1);
        snprintf(a->description, sizeof(a->description),
                 "Physical RAM consumption is at %.1f%%.", snapshot->mem.memory_usage_pct);
        strncpy(a->recommendation, "Flush OS buffer cache or review background daemons.", sizeof(a->recommendation) - 1);
        a->metric_value = snapshot->mem.memory_usage_pct;
        a->threshold_value = 80.0;
        health_deduction += 10.0;
    }

    /* Swap Thrashing */
    if (snapshot->mem.swap_usage_pct >= 35.0) {
        BottleneckAlert *a = &report->alerts[alert_idx++];
        a->type = BOTTLENECK_MEMORY_EXHAUSTION;
        a->severity = SEV_HIGH;
        strncpy(a->component, "Virtual Memory", sizeof(a->component) - 1);
        strncpy(a->title, "Active Swap Thrashing Detected", sizeof(a->title) - 1);
        snprintf(a->description, sizeof(a->description),
                 "Swap utilization at %.1f%%. High page-fault rate is causing major latency spikes.",
                 snapshot->mem.swap_usage_pct);
        strncpy(a->recommendation, "Tune vm.swappiness (e.g. reduce to 10) and increase physical RAM.", sizeof(a->recommendation) - 1);
        health_deduction += 20.0;
    }

    /* -----------------------------------------------------------------
     * RULE 3: I/O Wait Storage Saturation & Uninterruptible Sleep
     * ----------------------------------------------------------------- */
    if (snapshot->cpu.iowait_pct >= 12.0 || blocked_d_state_count > 0 || snapshot->cpu.processes_blocked > 0) {
        BottleneckAlert *a = &report->alerts[alert_idx++];
        a->type = BOTTLENECK_IO_WAIT_SATURATION;
        a->severity = (snapshot->cpu.iowait_pct >= 20.0) ? SEV_CRITICAL : SEV_HIGH;
        strncpy(a->component, "Disk / Block I/O", sizeof(a->component) - 1);
        strncpy(a->title, "High I/O Wait Storage Bottleneck", sizeof(a->title) - 1);
        snprintf(a->description, sizeof(a->description),
                 "CPU is spending %.1f%% cycles stalled waiting for disk I/O. Blocked process count: %lu.",
                 snapshot->cpu.iowait_pct, snapshot->cpu.processes_blocked);
        strncpy(a->recommendation,
                "Switch I/O scheduler to BFQ/Kyber for low latency, or isolate write-heavy queues.",
                sizeof(a->recommendation) - 1);
        a->metric_value = snapshot->cpu.iowait_pct;
        a->threshold_value = 12.0;
        health_deduction += 25.0;
    }

    /* -----------------------------------------------------------------
     * RULE 4: Resource Conflict Analysis (Contending Tasks)
     * ----------------------------------------------------------------- */
    /* Check for co-running processes with high CPU or conflicting I/O */
    for (int i = 0; i < snapshot->process_count - 1 && conflict_idx < 16; i++) {
        for (int j = i + 1; j < snapshot->process_count && conflict_idx < 16; j++) {
            const ProcessEntry *p1 = &snapshot->processes[i];
            const ProcessEntry *p2 = &snapshot->processes[j];

            /* Conflict Type 1: Multi-task CPU Affinity Contention */
            if (p1->cpu_usage_pct >= 25.0 && p2->cpu_usage_pct >= 25.0) {
                ResourceConflict *c = &report->conflicts[conflict_idx++];
                c->pid_a = p1->pid;
                strncpy(c->name_a, p1->name, sizeof(c->name_a) - 1);
                c->pid_b = p2->pid;
                strncpy(c->name_b, p2->name, sizeof(c->name_b) - 1);
                strncpy(c->conflict_resource, "CPU Core Contention", sizeof(c->conflict_resource) - 1);
                snprintf(c->details, sizeof(c->details),
                         "PID %d ('%s', %.1f%%) and PID %d ('%s', %.1f%%) are competing for CPU timeslices.",
                         p1->pid, p1->name, p1->cpu_usage_pct, p2->pid, p2->name, p2->cpu_usage_pct);
                c->severity = SEV_HIGH;
                health_deduction += 10.0;
            }

            /* Conflict Type 2: Priority Inversion / Skew */
            if (p1->nice > 10 && p1->cpu_usage_pct > 30.0 && p2->nice <= 0 && p2->cpu_usage_pct < 5.0 && p2->state == 'D') {
                ResourceConflict *c = &report->conflicts[conflict_idx++];
                c->pid_a = p1->pid;
                strncpy(c->name_a, p1->name, sizeof(c->name_a) - 1);
                c->pid_b = p2->pid;
                strncpy(c->name_b, p2->name, sizeof(c->name_b) - 1);
                strncpy(c->conflict_resource, "Priority Inversion", sizeof(c->conflict_resource) - 1);
                snprintf(c->details, sizeof(c->details),
                         "Low-priority task '%s' (nice %d) is consuming resources while interactive '%s' (nice %d) is blocked.",
                         p1->name, p1->nice, p2->name, p2->nice);
                c->severity = SEV_CRITICAL;
                health_deduction += 15.0;
            }
        }
    }

    /* -----------------------------------------------------------------
     * RULE 5: Context Switch Storm & Lock Convoy
     * ----------------------------------------------------------------- */
    if (snapshot->cpu.context_switches > 35000) {
        BottleneckAlert *a = &report->alerts[alert_idx++];
        a->type = BOTTLENECK_CONTEXT_SWITCH_STORM;
        a->severity = SEV_MEDIUM;
        strncpy(a->component, "Scheduler / Concurrency", sizeof(a->component) - 1);
        strncpy(a->title, "Excessive Context Switching Storm", sizeof(a->title) - 1);
        snprintf(a->description, sizeof(a->description),
                 "High rate of context switches (%lu/sec) detected. Excessive CPU cycles wasted on kernel state saves.",
                 snapshot->cpu.context_switches);
        strncpy(a->recommendation,
                "Increase scheduling time quantum or optimize thread pool concurrency.",
                sizeof(a->recommendation) - 1);
        health_deduction += 10.0;
    }

    report->alert_count = alert_idx;
    report->conflict_count = conflict_idx;

    double score = 100.0 - health_deduction;
    if (score < 10.0) score = 10.0;
    if (score > 100.0) score = 100.0;
    report->health_score = score;

    if (score >= 80.0) {
        strncpy(report->general_status, "OPTIMAL", sizeof(report->general_status) - 1);
    } else if (score >= 50.0) {
        strncpy(report->general_status, "STRESSED", sizeof(report->general_status) - 1);
    } else {
        strncpy(report->general_status, "CRITICAL", sizeof(report->general_status) - 1);
    }

    return 0;
}

void analysis_export_json(const AnalysisReport *report, FILE *out) {
    if (!report || !out) return;

    fprintf(out, "{\n");
    fprintf(out, "  \"health_score\": %.1f,\n", report->health_score);
    fprintf(out, "  \"general_status\": \"%s\",\n", report->general_status);
    fprintf(out, "  \"alert_count\": %d,\n", report->alert_count);
    fprintf(out, "  \"alerts\": [\n");
    for (int i = 0; i < report->alert_count; i++) {
        const BottleneckAlert *a = &report->alerts[i];
        const char *sev_str = (a->severity == SEV_CRITICAL) ? "CRITICAL" :
                              (a->severity == SEV_HIGH) ? "HIGH" :
                              (a->severity == SEV_MEDIUM) ? "MEDIUM" : "LOW";
        fprintf(out, "    {\n");
        fprintf(out, "      \"type\": %d,\n", a->type);
        fprintf(out, "      \"severity\": \"%s\",\n", sev_str);
        fprintf(out, "      \"component\": \"%s\",\n", a->component);
        fprintf(out, "      \"title\": \"%s\",\n", a->title);
        fprintf(out, "      \"description\": \"%s\",\n", a->description);
        fprintf(out, "      \"recommendation\": \"%s\",\n", a->recommendation);
        fprintf(out, "      \"culprit_pid\": %d,\n", a->culprit_pid);
        fprintf(out, "      \"culprit_name\": \"%s\",\n", a->culprit_name);
        fprintf(out, "      \"metric_value\": %.2f,\n", a->metric_value);
        fprintf(out, "      \"threshold_value\": %.2f\n", a->threshold_value);
        fprintf(out, "    }%s\n", (i + 1 < report->alert_count) ? "," : "");
    }
    fprintf(out, "  ],\n");

    fprintf(out, "  \"conflict_count\": %d,\n", report->conflict_count);
    fprintf(out, "  \"conflicts\": [\n");
    for (int i = 0; i < report->conflict_count; i++) {
        const ResourceConflict *c = &report->conflicts[i];
        fprintf(out, "    {\n");
        fprintf(out, "      \"pid_a\": %d, \"name_a\": \"%s\",\n", c->pid_a, c->name_a);
        fprintf(out, "      \"pid_b\": %d, \"name_b\": \"%s\",\n", c->pid_b, c->name_b);
        fprintf(out, "      \"conflict_resource\": \"%s\",\n", c->conflict_resource);
        fprintf(out, "      \"details\": \"%s\",\n", c->details);
        fprintf(out, "      \"severity\": \"%s\"\n", (c->severity == SEV_CRITICAL) ? "CRITICAL" : "HIGH");
        fprintf(out, "    }%s\n", (i + 1 < report->conflict_count) ? "," : "");
    }
    fprintf(out, "  ]\n");
    fprintf(out, "}\n");
}
