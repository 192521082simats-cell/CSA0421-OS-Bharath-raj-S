/**
 * SysPilot: Module 1 - Live System and Process Monitoring
 * monitor.h - Header for CPU, Memory, Disk, and Process monitoring
 */

#ifndef SYSPILOT_MONITOR_H
#define SYSPILOT_MONITOR_H

#include "common.h"

/* CPU Statistics from /proc/stat */
typedef struct {
    unsigned long long user;
    unsigned long long nice;
    unsigned long long system;
    unsigned long long idle;
    unsigned long long iowait;
    unsigned long long irq;
    unsigned long long softirq;
    unsigned long long steal;
    unsigned long long guest;
    unsigned long long guest_nice;
    unsigned long long total;
    unsigned long long active;
    
    float cpu_percent;
    float user_percent;
    float system_percent;
    float iowait_percent;
    float steal_percent;
} cpu_core_stats_t;

typedef struct {
    cpu_core_stats_t overall;
    cpu_core_stats_t cores[MAX_CORES];
    int core_count;
    unsigned long long context_switches;
    unsigned long long context_switch_rate; /* per second */
    unsigned int procs_running;
    unsigned int procs_blocked;
} cpu_stats_t;

/* Memory Statistics from /proc/meminfo */
typedef struct {
    unsigned long long total_kb;
    unsigned long long free_kb;
    unsigned long long available_kb;
    unsigned long long buffers_kb;
    unsigned long long cached_kb;
    unsigned long long active_kb;
    unsigned long long inactive_kb;
    unsigned long long used_kb;
    
    unsigned long long swap_total_kb;
    unsigned long long swap_free_kb;
    unsigned long long swap_used_kb;
    unsigned long long swap_cached_kb;
    
    float mem_util_percent;
    float swap_util_percent;
} mem_stats_t;

/* Disk I/O Statistics from /proc/diskstats */
typedef struct {
    char device_name[32];
    unsigned long long reads_completed;
    unsigned long long reads_merged;
    unsigned long long sectors_read;
    unsigned long long time_reading_ms;
    unsigned long long writes_completed;
    unsigned long long writes_merged;
    unsigned long long sectors_written;
    unsigned long long time_writing_ms;
    unsigned int io_in_progress;
    unsigned long long time_doing_io_ms;
    unsigned long long weighted_time_io_ms;
    
    /* Calculated rates per second */
    double read_kb_sec;
    double write_kb_sec;
    double tps;              /* Transfers per second */
    float disk_util_percent;
} disk_device_stats_t;

typedef struct {
    disk_device_stats_t devices[MAX_DISKS];
    int device_count;
    double total_read_kb_sec;
    double total_write_kb_sec;
    float max_disk_util_percent;
} disk_stats_t;

/* Process Information from /proc/[pid]/ */
typedef struct {
    int pid;
    int ppid;
    char name[MAX_NAME_LEN];
    char cmdline[MAX_CMD_LEN];
    char state_char;
    proc_state_t state;
    
    unsigned long long utime;
    unsigned long long stime;
    unsigned long long cutime;
    unsigned long long cstime;
    unsigned long long total_time_ticks;
    
    float cpu_percent;
    int priority;
    int nice_val;
    int num_threads;
    
    unsigned long long vsize_kb;
    unsigned long long rss_kb;
    float mem_percent;
    
    unsigned long long read_bytes_sec;
    unsigned long long write_bytes_sec;
    int io_wait_ticks;
    
    /* Historical growth metric for leak detection */
    long long rss_delta_kb;
} proc_info_t;

/* Full System Snapshot */
typedef struct {
    double timestamp;
    float load_avg[3];       /* 1 min, 5 min, 15 min */
    cpu_stats_t cpu;
    mem_stats_t mem;
    disk_stats_t disk;
    
    proc_info_t processes[MAX_PROCESSES];
    int process_count;
    
    int total_tasks;
    int running_tasks;
    int sleeping_tasks;
    int disk_sleep_tasks;
    int zombie_tasks;
} system_snapshot_t;

/* Module 1 API Functions */
int monitor_init(bool force_simulation);
int monitor_capture_snapshot(system_snapshot_t *curr, const system_snapshot_t *prev, double delta_time);
void monitor_sort_processes(system_snapshot_t *snap, proc_sort_field_t field, bool ascending);
void monitor_render_dashboard(const system_snapshot_t *snap, proc_sort_field_t sort_field, int max_display_procs);
void monitor_print_snapshot_summary(const system_snapshot_t *snap);
bool monitor_is_simulated_mode(void);

#endif /* SYSPILOT_MONITOR_H */
