/**
 * SysPilot: Design and Performance Evaluation of CPU Scheduling Algorithms
 * with Linux Resource Monitoring Using C
 *
 * common.h - Global definitions, enumerations, data structures, and utilities.
 */

#ifndef SYSPILOT_COMMON_H
#define SYSPILOT_COMMON_H

#define _GNU_SOURCE
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <stdbool.h>
#include <time.h>
#include <ctype.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
    #define SLEEP_MS(ms) Sleep(ms)
#else
    #include <unistd.h>
    #include <sys/time.h>
    #define SLEEP_MS(ms) usleep((ms) * 1000)
#endif

/* ANSI Terminal Colors */
#define ANSI_RESET       "\033[0m"
#define ANSI_BOLD        "\033[1m"
#define ANSI_DIM         "\033[2m"
#define ANSI_RED         "\033[31m"
#define ANSI_GREEN       "\033[32m"
#define ANSI_YELLOW      "\033[33m"
#define ANSI_BLUE        "\033[34m"
#define ANSI_MAGENTA     "\033[35m"
#define ANSI_CYAN        "\033[36m"
#define ANSI_WHITE       "\033[37m"
#define ANSI_BG_RED      "\033[41m"
#define ANSI_BG_GREEN    "\033[42m"
#define ANSI_BG_BLUE     "\033[44m"
#define ANSI_CLEAR       "\033[2J\033[H"

/* System Limits & Defaults */
#define MAX_PROCESSES          1024
#define MAX_NAME_LEN           64
#define MAX_CMD_LEN            256
#define MAX_CORES              64
#define MAX_DISKS              16
#define MAX_ALERTS             32
#define MAX_SCHED_TASKS        256
#define MAX_GANTT_ENTRIES      2048
#define DEFAULT_REFRESH_SEC    1
#define DEFAULT_QUANTUM        3

/* Severity Levels for Module 2 Alerts */
typedef enum {
    SEV_INFO = 0,
    SEV_WARNING = 1,
    SEV_CRITICAL = 2
} alert_severity_t;

/* Bottleneck Categories */
typedef enum {
    BOTTLE_CPU = 0,
    BOTTLE_MEM,
    BOTTLE_IO,
    BOTTLE_CONFLICT,
    BOTTLE_STARVATION
} bottleneck_type_t;

/* Process Operating States */
typedef enum {
    STATE_RUNNING = 0,     /* 'R' */
    STATE_SLEEPING,        /* 'S' */
    STATE_DISK_SLEEP,      /* 'D' (uninterruptible I/O wait) */
    STATE_ZOMBIE,          /* 'Z' */
    STATE_STOPPED,         /* 'T' */
    STATE_UNKNOWN
} proc_state_t;

/* Workload Classifications for Module 3 */
typedef enum {
    WORKLOAD_BALANCED = 0,
    WORKLOAD_CPU_BOUND,
    WORKLOAD_IO_BOUND,
    WORKLOAD_INTERACTIVE,
    WORKLOAD_BURSTY
} workload_type_t;

/* CPU Scheduling Algorithms */
typedef enum {
    SCHED_FCFS = 0,
    SCHED_SJF,             /* Shortest Job First (Non-preemptive) */
    SCHED_SRTF,            /* Shortest Remaining Time First (Preemptive) */
    SCHED_RR,              /* Round Robin */
    SCHED_PRIORITY_NP,     /* Priority (Non-preemptive) */
    SCHED_PRIORITY_P,      /* Priority (Preemptive with Aging) */
    SCHED_MLFQ,            /* Multilevel Feedback Queue */
    SCHED_COUNT
} sched_algo_t;

/* Sort options for Process Table */
typedef enum {
    SORT_BY_PID = 0,
    SORT_BY_CPU,
    SORT_BY_MEM,
    SORT_BY_NAME
} proc_sort_field_t;

/* Utility Function Prototypes */
const char* get_severity_str(alert_severity_t sev);
const char* get_severity_color(alert_severity_t sev);
const char* get_algo_name(sched_algo_t algo);
const char* get_workload_name(workload_type_t type);
double get_current_time_seconds(void);
void safe_strncpy(char *dest, const char *src, size_t n);

#endif /* SYSPILOT_COMMON_H */
