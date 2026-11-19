/**
 * SysPilot: Module 3 - OS Performance Analysis and Optimization
 * optimizer.h - Header for recommendations, kernel tuning advisories, and report generation
 */

#ifndef SYSPILOT_OPTIMIZER_H
#define SYSPILOT_OPTIMIZER_H

#include "common.h"
#include "monitor.h"
#include "analyzer.h"
#include "scheduler.h"

/* Linux Kernel Tuning Advisory Structure */
typedef struct {
    char parameter[64];
    char current_behavior[128];
    char recommended_command[256];
    char rational[256];
    int expected_impact_pct;
} kernel_tune_advise_t;

/* Optimization Recommendation Package */
typedef struct {
    workload_type_t detected_workload;
    char workload_name[32];
    char workload_characteristics[256];
    
    sched_algo_t optimal_algo;
    char optimal_algo_name[32];
    char algo_justification[512];
    
    int recommended_quantum;
    
    kernel_tune_advise_t kernel_tunings[8];
    int tuning_count;
    
    char process_prioritization_advice[512];
    char memory_tuning_advice[512];
    char io_tuning_advice[512];
} optimization_plan_t;

/* Module 3 Optimizer API Functions */
void optimizer_init(void);
void optimizer_analyze_and_plan(
    const system_snapshot_t *snap,
    const analysis_report_t *analysis,
    const sched_eval_result_t sched_results[SCHED_COUNT],
    workload_type_t workload_type,
    optimization_plan_t *plan
);

void optimizer_render_terminal_plan(const optimization_plan_t *plan);

/* Report Generators */
int optimizer_export_markdown_report(
    const char *filepath,
    const system_snapshot_t *snap,
    const analysis_report_t *analysis,
    const sched_eval_result_t sched_results[SCHED_COUNT],
    const optimization_plan_t *plan
);

int optimizer_export_csv_metrics(
    const char *filepath,
    const sched_eval_result_t sched_results[SCHED_COUNT]
);

#endif /* SYSPILOT_OPTIMIZER_H */
