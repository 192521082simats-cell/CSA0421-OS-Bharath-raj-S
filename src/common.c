/**
 * SysPilot: common.c - Utility implementations
 */

#include "common.h"

const char* get_severity_str(alert_severity_t sev) {
    switch (sev) {
        case SEV_INFO:     return "INFO";
        case SEV_WARNING:  return "WARNING";
        case SEV_CRITICAL: return "CRITICAL";
        default:           return "UNKNOWN";
    }
}

const char* get_severity_color(alert_severity_t sev) {
    switch (sev) {
        case SEV_INFO:     return ANSI_CYAN;
        case SEV_WARNING:  return ANSI_YELLOW;
        case SEV_CRITICAL: return ANSI_RED;
        default:           return ANSI_WHITE;
    }
}

const char* get_algo_name(sched_algo_t algo) {
    switch (algo) {
        case SCHED_FCFS:        return "First-Come First-Served (FCFS)";
        case SCHED_SJF:         return "Shortest Job First (SJF-NP)";
        case SCHED_SRTF:        return "Shortest Remaining Time First (SRTF-P)";
        case SCHED_RR:          return "Round Robin (RR)";
        case SCHED_PRIORITY_NP: return "Priority (Non-Preemptive)";
        case SCHED_PRIORITY_P:  return "Priority (Preemptive + Aging)";
        case SCHED_MLFQ:        return "Multilevel Feedback Queue (MLFQ)";
        default:                return "Unknown Algorithm";
    }
}

const char* get_workload_name(workload_type_t type) {
    switch (type) {
        case WORKLOAD_BALANCED:    return "Balanced / General";
        case WORKLOAD_CPU_BOUND:   return "CPU-Bound / Computation Heavy";
        case WORKLOAD_IO_BOUND:    return "I/O-Bound / Disk & Network Heavy";
        case WORKLOAD_INTERACTIVE: return "Interactive / GUI & User Tasks";
        case WORKLOAD_BURSTY:      return "Bursty / Sporadic Spikes";
        default:                   return "Custom Workload";
    }
}

double get_current_time_seconds(void) {
#if defined(_WIN32) || defined(_WIN64)
    static LARGE_INTEGER freq;
    static int initialized = 0;
    LARGE_INTEGER counter;
    if (!initialized) {
        QueryPerformanceFrequency(&freq);
        initialized = 1;
    }
    QueryPerformanceCounter(&counter);
    return (double)counter.QuadPart / (double)freq.QuadPart;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (double)tv.tv_sec + (double)tv.tv_usec / 1000000.0;
#endif
}

void safe_strncpy(char *dest, const char *src, size_t n) {
    if (!dest || n == 0) return;
    if (!src) {
        dest[0] = '\0';
        return;
    }
    strncpy(dest, src, n - 1);
    dest[n - 1] = '\0';
}
