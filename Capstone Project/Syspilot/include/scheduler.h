/**
 * SysPilot: Module 3 - CPU Scheduling Algorithms Engine
 * scheduler.h - Header for CPU scheduling algorithm design, simulation, and evaluation
 */

#ifndef SYSPILOT_SCHEDULER_H
#define SYSPILOT_SCHEDULER_H

#include "common.h"
#include "monitor.h"

/* Scheduling Task / Process Control Block (PCB) for simulation */
typedef struct {
    int pid;
    char name[MAX_NAME_LEN];
    int arrival_time;
    int burst_time;              /* Total CPU burst required */
    int remaining_time;          /* Remaining burst for preemptive */
    int priority;                /* Lower number = higher priority */
    int dynamic_priority;        /* Adjusted by aging or MLFQ */
    
    int start_time;              /* First time task gained CPU */
    int completion_time;
    int waiting_time;
    int turnaround_time;
    int response_time;
    
    int io_burst_duration;       /* Optional I/O burst delay */
    int mlfq_queue_level;        /* 0 = High (RR Q=q0), 1 = Med (RR Q=q1), 2 = Low (FCFS) */
    int time_in_current_queue;   /* Time slice consumed in current MLFQ queue */
    
    bool is_completed;
    bool has_started;
} sched_task_t;

/* Gantt Chart Timeline Slice */
typedef struct {
    int pid;
    char name[MAX_NAME_LEN];
    int start_time;
    int end_time;
} gantt_slice_t;

/* Evaluation Results for a specific Algorithm */
typedef struct {
    sched_algo_t algo;
    char algo_name[32];
    
    float avg_waiting_time;
    float avg_turnaround_time;
    float avg_response_time;
    float cpu_utilization_pct;
    float throughput;             /* Processes per unit time */
    float jains_fairness_index;   /* 0.0 to 1.0 */
    
    int total_execution_time;
    int context_switches;
    int starved_task_count;
    
    gantt_slice_t gantt[MAX_GANTT_ENTRIES];
    int gantt_count;
    
    sched_task_t completed_tasks[MAX_SCHED_TASKS];
    int task_count;
} sched_eval_result_t;

/* Module 3 Scheduling API Functions */
void scheduler_init(void);

/* Individual Algorithm Implementations */
sched_eval_result_t sched_run_fcfs(const sched_task_t *tasks, int count);
sched_eval_result_t sched_run_sjf(const sched_task_t *tasks, int count);
sched_eval_result_t sched_run_srtf(const sched_task_t *tasks, int count);
sched_eval_result_t sched_run_rr(const sched_task_t *tasks, int count, int quantum);
sched_eval_result_t sched_run_priority_np(const sched_task_t *tasks, int count);
sched_eval_result_t sched_run_priority_p(const sched_task_t *tasks, int count, int aging_threshold);
sched_eval_result_t sched_run_mlfq(const sched_task_t *tasks, int count, int q0, int q1, int boost_interval);

/* Workload Generation & Ingestion */
int sched_generate_workload(sched_task_t *tasks, int max_tasks, workload_type_t type, int count);
int sched_extract_live_workload(const system_snapshot_t *snap, sched_task_t *tasks, int max_tasks);

/* Evaluation & Display */
void sched_evaluate_all(const sched_task_t *tasks, int count, int quantum, sched_eval_result_t results[SCHED_COUNT]);
void sched_render_comparison_table(const sched_eval_result_t results[SCHED_COUNT]);
void sched_render_gantt_chart(const sched_eval_result_t *res, int max_width);
void sched_render_task_breakdown(const sched_eval_result_t *res);

#endif /* SYSPILOT_SCHEDULER_H */
