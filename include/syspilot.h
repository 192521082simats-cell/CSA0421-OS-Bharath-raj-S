/**
 * SysPilot: Design and Performance Evaluation of CPU Scheduling Algorithms
 * with Linux Resource Monitoring Using C
 * 
 * Header: include/syspilot.h
 * Unified definitions for Modules 1, 2, and 3
 */

#ifndef SYSPILOT_H
#define SYSPILOT_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>

#define SYSPILOT_VERSION "1.0.0"
#define MAX_PROCESSES 1024
#define MAX_CORES 128
#define MAX_NAME_LEN 64
#define MAX_GANTT_EVENTS 4096
#define MAX_BOTTLENECK_ALERTS 32

/* =========================================================================
 * MODULE 1: Live System and Process Monitoring Data Structures
 * ========================================================================= */

typedef struct {
    uint64_t user;
    uint64_t nice;
    uint64_t system;
    uint64_t idle;
    uint64_t iowait;
    uint64_t irq;
    uint64_t softirq;
    uint64_t steal;
    uint64_t guest;
    uint64_t total;
} SysCPUStats;

typedef struct {
    double total_usage_pct;
    double user_pct;
    double system_pct;
    double iowait_pct;
    int core_count;
    double per_core_pct[MAX_CORES];
    unsigned long context_switches;
    unsigned long processes_running;
    unsigned long processes_blocked;
} SystemCPUInfo;

typedef struct {
    uint64_t total_kb;
    uint64_t free_kb;
    uint64_t available_kb;
    uint64_t buffers_kb;
    uint64_t cached_kb;
    uint64_t swap_total_kb;
    uint64_t swap_free_kb;
    double memory_usage_pct;
    double swap_usage_pct;
} SystemMemInfo;

typedef struct {
    uint64_t reads_completed;
    uint64_t sectors_read;
    uint64_t writes_completed;
    uint64_t sectors_written;
    double read_kb_per_sec;
    double write_kb_per_sec;
    double io_time_ms;
} SystemDiskInfo;

typedef struct {
    int pid;
    int ppid;
    char name[MAX_NAME_LEN];
    char state;            /* 'R', 'S', 'D', 'Z', 'T' */
    int priority;
    int nice;
    int num_threads;
    uint64_t utime;
    uint64_t stime;
    double cpu_usage_pct;
    uint64_t rss_bytes;
    uint64_t virt_bytes;
    uint64_t read_bytes;
    uint64_t write_bytes;
} ProcessEntry;

typedef struct {
    SystemCPUInfo cpu;
    SystemMemInfo mem;
    SystemDiskInfo disk;
    ProcessEntry processes[MAX_PROCESSES];
    int process_count;
    double uptime_seconds;
} SystemSnapshot;

/* Module 1 Functions */
int monitor_init(void);
int monitor_read_cpu(SystemCPUInfo *cpu_info);
int monitor_read_memory(SystemMemInfo *mem_info);
int monitor_read_disk(SystemDiskInfo *disk_info);
int monitor_read_processes(ProcessEntry *procs, int max_count, int *actual_count);
int monitor_capture_snapshot(SystemSnapshot *snapshot);
void monitor_export_json(const SystemSnapshot *snapshot, FILE *out);

/* =========================================================================
 * MODULE 2: Intelligent Bottleneck and Resource-Conflict Analysis
 * ========================================================================= */

typedef enum {
    BOTTLENECK_NONE = 0,
    BOTTLENECK_CPU_SATURATION,
    BOTTLENECK_MEMORY_EXHAUSTION,
    BOTTLENECK_IO_WAIT_SATURATION,
    BOTTLENECK_RESOURCE_CONFLICT,
    BOTTLENECK_CONTEXT_SWITCH_STORM,
    BOTTLENECK_ZOMBIE_LEAK
} BottleneckType;

typedef enum {
    SEV_NORMAL = 0,
    SEV_LOW,
    SEV_MEDIUM,
    SEV_HIGH,
    SEV_CRITICAL
} AlertSeverity;

typedef struct {
    BottleneckType type;
    AlertSeverity severity;
    char component[32];          /* "CPU", "Memory", "Disk I/O", "Process Subsystem" */
    char title[128];
    char description[256];
    char recommendation[256];
    int culprit_pid;
    char culprit_name[MAX_NAME_LEN];
    double metric_value;
    double threshold_value;
} BottleneckAlert;

typedef struct {
    int pid_a;
    char name_a[MAX_NAME_LEN];
    int pid_b;
    char name_b[MAX_NAME_LEN];
    char conflict_resource[32];  /* "CPU Affinity", "Disk Bandwidth", "High Priority Inversion" */
    char details[256];
    AlertSeverity severity;
} ResourceConflict;

typedef struct {
    BottleneckAlert alerts[MAX_BOTTLENECK_ALERTS];
    int alert_count;
    ResourceConflict conflicts[16];
    int conflict_count;
    double health_score;         /* 0 to 100% */
    char general_status[32];     /* "OPTIMAL", "STRESSED", "CRITICAL" */
} AnalysisReport;

/* Module 2 Functions */
int analyze_snapshot(const SystemSnapshot *snapshot, AnalysisReport *report);
void analysis_export_json(const AnalysisReport *report, FILE *out);

/* =========================================================================
 * MODULE 3: OS CPU Scheduling Algorithms & Performance Optimization
 * ========================================================================= */

typedef enum {
    ALGO_FCFS = 0,
    ALGO_SJF_NONPREEMPT,
    ALGO_SRTF_PREEMPT,
    ALGO_PRIORITY_NONPREEMPT,
    ALGO_PRIORITY_PREEMPT,
    ALGO_ROUND_ROBIN,
    ALGO_MLFQ,
    ALGO_CFS
} SchedulingAlgorithm;

typedef struct {
    int pid;
    char name[MAX_NAME_LEN];
    int arrival_time;
    int burst_time;
    int remaining_time;
    int priority;                /* 1 (highest) to 10 (lowest) */
    int nice_val;                /* -20 to 19 for CFS */
    int start_time;
    int completion_time;
    int waiting_time;
    int turnaround_time;
    int response_time;
    double vruntime;             /* For CFS emulation */
    int queue_level;             /* For MLFQ (0, 1, 2) */
    bool completed;
} SchedProcess;

typedef struct {
    int pid;
    char name[MAX_NAME_LEN];
    int start_time;
    int end_time;
} GanttEvent;

typedef struct {
    SchedulingAlgorithm algorithm;
    char algo_name[64];
    int time_quantum;            /* For RR & MLFQ */
    int total_simulation_time;
    int idle_time;
    int context_switches;
    double cpu_utilization_pct;
    double throughput;           /* jobs per unit time */
    double avg_waiting_time;
    double avg_turnaround_time;
    double avg_response_time;
    double jains_fairness_index; /* Jain's Index */
    
    SchedProcess processes[MAX_PROCESSES];
    int process_count;
    
    GanttEvent gantt[MAX_GANTT_EVENTS];
    int gantt_count;
    
    char bottleneck_diagnosis[256];
    char optimization_recommendations[512];
} SchedEvaluationReport;

/* Module 3 Functions */
const char* get_algo_name(SchedulingAlgorithm algo);
int schedule_simulate(SchedulingAlgorithm algo, 
                      const SchedProcess *input_procs, 
                      int proc_count, 
                      int time_quantum, 
                      SchedEvaluationReport *report);
void schedule_export_json(const SchedEvaluationReport *report, FILE *out);
void generate_optimization_advice(const SchedEvaluationReport *report, char *output, size_t max_len);

#endif /* SYSPILOT_H */
