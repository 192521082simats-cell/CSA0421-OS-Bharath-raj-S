/**
 * SysPilot: Module 2 - Intelligent Bottleneck and Resource-Conflict Analysis
 * analyzer.h - Header for bottleneck detection, thrashing, and contention analysis
 */

#ifndef SYSPILOT_ANALYZER_H
#define SYSPILOT_ANALYZER_H

#include "common.h"
#include "monitor.h"

/* Threshold Constants for Anomaly Detection */
#define THRESH_CPU_CRITICAL          88.0f   /* Overall CPU > 88% */
#define THRESH_CPU_WARNING           70.0f   /* Overall CPU > 70% */
#define THRESH_LOAD_RATIO_WARN       1.2f    /* Load1 / Cores > 1.2 */
#define THRESH_LOAD_RATIO_CRIT       2.0f    /* Load1 / Cores > 2.0 */
#define THRESH_PROC_CPU_HOG          65.0f   /* Single proc CPU > 65% of a core */
#define THRESH_MEM_CRITICAL          90.0f   /* RAM util > 90% */
#define THRESH_MEM_WARNING           78.0f   /* RAM util > 78% */
#define THRESH_SWAP_CRITICAL         60.0f   /* Swap util > 60% with activity */
#define THRESH_IO_WAIT_WARN          12.0f   /* %iowait > 12% */
#define THRESH_IO_WAIT_CRIT          25.0f   /* %iowait > 25% */
#define THRESH_DISK_UTIL_CRIT        85.0f   /* Disk busy > 85% */
#define THRESH_CTX_SWITCH_HIGH       15000   /* Context switches/sec */

/* Individual Bottleneck or Conflict Alert */
typedef struct {
    alert_severity_t severity;
    bottleneck_type_t category;
    char title[64];
    char description[256];
    int culprit_pid;
    char culprit_name[MAX_NAME_LEN];
    char metric_name[32];
    float metric_value;
    float threshold_value;
    char recommendation[256];
} bottleneck_alert_t;

/* Process Conflict Cluster (Processes fighting for same resource) */
typedef struct {
    bottleneck_type_t resource_type;
    int process_count;
    int pids[8];
    char names[8][MAX_NAME_LEN];
    float combined_utilization;
    char conflict_nature[128];
} resource_conflict_t;

/* Comprehensive Module 2 Analysis Report */
typedef struct {
    double timestamp;
    float system_health_score;      /* 0 to 100 scale */
    
    int cpu_bottleneck_flag;
    int mem_bottleneck_flag;
    int io_bottleneck_flag;
    int conflict_flag;
    int starvation_risk_flag;
    
    bottleneck_alert_t alerts[MAX_ALERTS];
    int alert_count;
    
    resource_conflict_t conflicts[8];
    int conflict_count;
    
    char primary_diagnostic[256];
    char root_cause_analysis[512];
    char executive_recommendation[512];
} analysis_report_t;

/* Module 2 API Functions */
void analyzer_init(void);
void analyzer_evaluate(const system_snapshot_t *curr, const system_snapshot_t *prev, analysis_report_t *report);
void analyzer_render_report(const analysis_report_t *report);
int analyzer_export_json(const analysis_report_t *report, const char *filepath);

#endif /* SYSPILOT_ANALYZER_H */
