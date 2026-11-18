/**
 * SysPilot: Linux ProcFS Compatibility & Emulation Layer
 * procfs_compat.h - Handles Linux /proc filesystem reading with simulation fallback
 */

#ifndef SYSPILOT_PROCFS_COMPAT_H
#define SYSPILOT_PROCFS_COMPAT_H

#include "common.h"
#include "monitor.h"

/* Detection & Lifecycle */
bool procfs_is_available(void);
void procfs_set_force_simulation(bool force);
bool procfs_is_simulation_active(void);

/* Low-level readers */
int procfs_read_loadavg(float load[3]);
int procfs_read_cpu_stats(cpu_stats_t *cpu);
int procfs_read_mem_stats(mem_stats_t *mem);
int procfs_read_disk_stats(disk_stats_t *disk);
int procfs_read_all_processes(proc_info_t procs[MAX_PROCESSES], int *count);

/* Simulation workload modifiers */
void procfs_sim_trigger_cpu_spike(int duration_cycles);
void procfs_sim_trigger_memory_leak(int duration_cycles);
void procfs_sim_trigger_io_storm(int duration_cycles);

#endif /* SYSPILOT_PROCFS_COMPAT_H */
