/**
 * SysPilot: procfs_compat.c - Native Linux /proc parser and High-Fidelity Simulation Layer
 */

#include "procfs_compat.h"
#include <dirent.h>
#include <math.h>

static bool s_force_sim = false;
static bool s_procfs_detected = false;
static bool s_initialized = false;

/* Simulation State Tracking */
static struct {
    unsigned long long sim_ticks;
    int cpu_spike_remaining;
    int mem_leak_remaining;
    int io_storm_remaining;
    unsigned long long sim_vsize_hog;
    unsigned long long sim_rss_hog;
} s_sim_state = {0, 0, 0, 0, 800000, 450000};

bool procfs_is_available(void) {
    if (s_force_sim) return false;
    if (!s_initialized) {
        FILE *f = fopen("/proc/stat", "r");
        if (f) {
            fclose(f);
            s_procfs_detected = true;
        } else {
            s_procfs_detected = false;
        }
        s_initialized = true;
    }
    return s_procfs_detected;
}

void procfs_set_force_simulation(bool force) {
    s_force_sim = force;
    s_initialized = false;
}

bool procfs_is_simulation_active(void) {
    return !procfs_is_available();
}

void procfs_sim_trigger_cpu_spike(int duration_cycles) {
    s_sim_state.cpu_spike_remaining = duration_cycles;
}

void procfs_sim_trigger_memory_leak(int duration_cycles) {
    s_sim_state.mem_leak_remaining = duration_cycles;
}

void procfs_sim_trigger_io_storm(int duration_cycles) {
    s_sim_state.io_storm_remaining = duration_cycles;
}

/* ========================================================================= */
/* Native Linux /proc Parsing Implementations                                */
/* ========================================================================= */

static int native_read_loadavg(float load[3]) {
    FILE *f = fopen("/proc/loadavg", "r");
    if (!f) return -1;
    if (fscanf(f, "%f %f %f", &load[0], &load[1], &load[2]) != 3) {
        fclose(f);
        return -1;
    }
    fclose(f);
    return 0;
}

static int native_read_cpu_stats(cpu_stats_t *cpu) {
    FILE *f = fopen("/proc/stat", "r");
    if (!f) return -1;
    
    char line[512];
    int core_idx = 0;
    cpu->core_count = 0;
    
    while (fgets(line, sizeof(line), f)) {
        if (strncmp(line, "cpu ", 4) == 0) {
            cpu_core_stats_t *o = &cpu->overall;
            sscanf(line + 4, "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                   &o->user, &o->nice, &o->system, &o->idle,
                   &o->iowait, &o->irq, &o->softirq, &o->steal,
                   &o->guest, &o->guest_nice);
        } else if (strncmp(line, "cpu", 3) == 0 && isdigit(line[3])) {
            if (core_idx < MAX_CORES) {
                cpu_core_stats_t *c = &cpu->cores[core_idx];
                char *ptr = line + 3;
                while (isdigit(*ptr)) ptr++;
                while (*ptr == ' ') ptr++;
                sscanf(ptr, "%llu %llu %llu %llu %llu %llu %llu %llu %llu %llu",
                       &c->user, &c->nice, &c->system, &c->idle,
                       &c->iowait, &c->irq, &c->softirq, &c->steal,
                       &c->guest, &c->guest_nice);
                core_idx++;
            }
        } else if (strncmp(line, "ctxt ", 5) == 0) {
            sscanf(line + 5, "%llu", &cpu->context_switches);
        } else if (strncmp(line, "procs_running ", 14) == 0) {
            sscanf(line + 14, "%u", &cpu->procs_running);
        } else if (strncmp(line, "procs_blocked ", 14) == 0) {
            sscanf(line + 14, "%u", &cpu->procs_blocked);
        }
    }
    cpu->core_count = core_idx;
    fclose(f);
    return 0;
}

static int native_read_mem_stats(mem_stats_t *mem) {
    FILE *f = fopen("/proc/meminfo", "r");
    if (!f) return -1;
    
    char key[64];
    unsigned long long val;
    char unit[32];
    
    memset(mem, 0, sizeof(*mem));
    
    while (fscanf(f, "%63s %llu %31s", key, &val, unit) == 3) {
        if (strcmp(key, "MemTotal:") == 0) mem->total_kb = val;
        else if (strcmp(key, "MemFree:") == 0) mem->free_kb = val;
        else if (strcmp(key, "MemAvailable:") == 0) mem->available_kb = val;
        else if (strcmp(key, "Buffers:") == 0) mem->buffers_kb = val;
        else if (strcmp(key, "Cached:") == 0) mem->cached_kb = val;
        else if (strcmp(key, "Active:") == 0) mem->active_kb = val;
        else if (strcmp(key, "Inactive:") == 0) mem->inactive_kb = val;
        else if (strcmp(key, "SwapTotal:") == 0) mem->swap_total_kb = val;
        else if (strcmp(key, "SwapFree:") == 0) mem->swap_free_kb = val;
        else if (strcmp(key, "SwapCached:") == 0) mem->swap_cached_kb = val;
    }
    fclose(f);
    
    if (mem->available_kb == 0) {
        mem->available_kb = mem->free_kb + mem->buffers_kb + mem->cached_kb;
    }
    mem->used_kb = (mem->total_kb > mem->available_kb) ? (mem->total_kb - mem->available_kb) : 0;
    mem->swap_used_kb = (mem->swap_total_kb > mem->swap_free_kb) ? (mem->swap_total_kb - mem->swap_free_kb) : 0;
    
    if (mem->total_kb > 0)
        mem->mem_util_percent = (float)mem->used_kb / (float)mem->total_kb * 100.0f;
    if (mem->swap_total_kb > 0)
        mem->swap_util_percent = (float)mem->swap_used_kb / (float)mem->swap_total_kb * 100.0f;
        
    return 0;
}

static int native_read_disk_stats(disk_stats_t *disk) {
    FILE *f = fopen("/proc/diskstats", "r");
    if (!f) return -1;
    
    char line[512];
    disk->device_count = 0;
    
    while (fgets(line, sizeof(line), f) && disk->device_count < MAX_DISKS) {
        int major, minor;
        char dev_name[32];
        unsigned long long rd_c, rd_m, rd_s, rd_t;
        unsigned long long wr_c, wr_m, wr_s, wr_t;
        unsigned int io_prog;
        unsigned long long t_io, wt_io;
        
        int n = sscanf(line, "%d %d %31s %llu %llu %llu %llu %llu %llu %llu %llu %u %llu %llu",
                       &major, &minor, dev_name,
                       &rd_c, &rd_m, &rd_s, &rd_t,
                       &wr_c, &wr_m, &wr_s, &wr_t,
                       &io_prog, &t_io, &wt_io);
                       
        if (n >= 14) {
            /* Filter loop, ram, and partition devices to keep primary disks */
            if (strncmp(dev_name, "loop", 4) == 0 || strncmp(dev_name, "ram", 3) == 0)
                continue;
            if (isdigit(dev_name[strlen(dev_name)-1]) && (strstr(dev_name, "sd") || strstr(dev_name, "vd")))
                continue;
                
            disk_device_stats_t *d = &disk->devices[disk->device_count++];
            safe_strncpy(d->device_name, dev_name, sizeof(d->device_name));
            d->reads_completed = rd_c;
            d->reads_merged = rd_m;
            d->sectors_read = rd_s;
            d->time_reading_ms = rd_t;
            d->writes_completed = wr_c;
            d->writes_merged = wr_m;
            d->sectors_written = wr_s;
            d->time_writing_ms = wr_t;
            d->io_in_progress = io_prog;
            d->time_doing_io_ms = t_io;
            d->weighted_time_io_ms = wt_io;
        }
    }
    fclose(f);
    return 0;
}

static int native_read_all_processes(proc_info_t procs[MAX_PROCESSES], int *count) {
#if defined(_WIN32) || defined(_WIN64)
    return -1;
#else
    DIR *dir = opendir("/proc");
    if (!dir) return -1;
    
    struct dirent *entry;
    int idx = 0;
    
    while ((entry = readdir(dir)) != NULL && idx < MAX_PROCESSES) {
        if (!isdigit(entry->d_name[0])) continue;
        int pid = atoi(entry->d_name);
        if (pid <= 0) continue;
        
        char path[256];
        snprintf(path, sizeof(path), "/proc/%d/stat", pid);
        FILE *f = fopen(path, "r");
        if (!f) continue;
        
        proc_info_t *p = &procs[idx];
        memset(p, 0, sizeof(*p));
        p->pid = pid;
        
        char comm[MAX_NAME_LEN];
        char state_char;
        int ppid, pgrp, session, tty_nr, tpgid;
        unsigned int flags;
        unsigned long minflt, cminflt, majflt, cmajflt;
        unsigned long long utime, stime;
        long cutime, cstime, priority, nice_val, num_threads, itrealvalue;
        unsigned long long starttime, vsize;
        long rss;
        
        /* Format of /proc/[pid]/stat */
        int parsed = fscanf(f, "%d (%63[^)]) %c %d %d %d %d %d %u %lu %lu %lu %lu %llu %llu %ld %ld %ld %ld %ld %ld %llu %llu %ld",
                            &p->pid, comm, &state_char, &ppid, &pgrp, &session, &tty_nr, &tpgid,
                            &flags, &minflt, &cminflt, &majflt, &cmajflt,
                            &utime, &stime, &cutime, &cstime, &priority, &nice_val,
                            &num_threads, &itrealvalue, &starttime, &vsize, &rss);
        fclose(f);
        
        if (parsed >= 24) {
            safe_strncpy(p->name, comm, sizeof(p->name));
            p->ppid = ppid;
            p->state_char = state_char;
            p->utime = utime;
            p->stime = stime;
            p->cutime = cutime;
            p->cstime = cstime;
            p->total_time_ticks = utime + stime;
            p->priority = (int)priority;
            p->nice_val = (int)nice_val;
            p->num_threads = (int)num_threads;
            p->vsize_kb = vsize / 1024;
            p->rss_kb = (rss > 0) ? (rss * 4) : 0; /* 4KB page typical */
            
            switch (state_char) {
                case 'R': p->state = STATE_RUNNING; break;
                case 'S': p->state = STATE_SLEEPING; break;
                case 'D': p->state = STATE_DISK_SLEEP; break;
                case 'Z': p->state = STATE_ZOMBIE; break;
                case 'T': p->state = STATE_STOPPED; break;
                default:  p->state = STATE_UNKNOWN; break;
            }
            
            /* Read cmdline */
            snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);
            FILE *fcmd = fopen(path, "r");
            if (fcmd) {
                size_t read_bytes = fread(p->cmdline, 1, sizeof(p->cmdline) - 1, fcmd);
                if (read_bytes > 0) {
                    for (size_t i = 0; i < read_bytes; i++) {
                        if (p->cmdline[i] == '\0') p->cmdline[i] = ' ';
                    }
                    p->cmdline[read_bytes] = '\0';
                } else {
                    safe_strncpy(p->cmdline, comm, sizeof(p->cmdline));
                }
                fclose(fcmd);
            } else {
                safe_strncpy(p->cmdline, comm, sizeof(p->cmdline));
            }
            idx++;
        }
    }
    closedir(dir);
    *count = idx;
    return 0;
#endif
}

/* ========================================================================= */
/* High-Fidelity Simulation ProcFS Layer (For Windows / Offline Evaluation) */
/* ========================================================================= */

static int sim_read_loadavg(float load[3]) {
    s_sim_state.sim_ticks++;
    if (s_sim_state.cpu_spike_remaining > 0) {
        s_sim_state.cpu_spike_remaining--;
        load[0] = 7.42f + ((rand() % 100) / 100.0f);
        load[1] = 5.80f;
        load[2] = 3.95f;
    } else {
        load[0] = 1.35f + ((rand() % 50) / 100.0f);
        load[1] = 1.10f;
        load[2] = 0.95f;
    }
    return 0;
}

static int sim_read_cpu_stats(cpu_stats_t *cpu) {
    int cores = 8;
    cpu->core_count = cores;
    
    unsigned long long base_tick = s_sim_state.sim_ticks * 1000;
    bool is_spike = (s_sim_state.cpu_spike_remaining > 0);
    
    unsigned long long total_user = 0, total_sys = 0, total_idle = 0, total_iowait = 0;
    
    for (int i = 0; i < cores; i++) {
        cpu_core_stats_t *c = &cpu->cores[i];
        unsigned long long u_delta = is_spike ? (850 + rand()%150) : (80 + rand()%60);
        unsigned long long s_delta = is_spike ? (100 + rand()%50) : (20 + rand()%15);
        unsigned long long io_delta = (s_sim_state.io_storm_remaining > 0) ? (300 + rand()%100) : (10 + rand()%5);
        unsigned long long i_delta = 1000 - (u_delta + s_delta + io_delta);
        if (i_delta > 1000) i_delta = 20;
        
        c->user = base_tick + u_delta;
        c->system = base_tick / 4 + s_delta;
        c->idle = base_tick * 8 + i_delta;
        c->iowait = base_tick / 10 + io_delta;
        c->nice = 10;
        c->irq = 5;
        c->softirq = 15;
        c->steal = 0;
        
        total_user += c->user;
        total_sys += c->system;
        total_idle += c->idle;
        total_iowait += c->iowait;
    }
    
    cpu->overall.user = total_user;
    cpu->overall.system = total_sys;
    cpu->overall.idle = total_idle;
    cpu->overall.iowait = total_iowait;
    cpu->overall.nice = 80;
    cpu->overall.irq = 40;
    cpu->overall.softirq = 120;
    cpu->overall.steal = 0;
    
    cpu->context_switches = 12500000ULL + s_sim_state.sim_ticks * (is_spike ? 28000 : 7500);
    cpu->procs_running = is_spike ? 9 : 2;
    cpu->procs_blocked = (s_sim_state.io_storm_remaining > 0) ? 5 : 0;
    
    return 0;
}

static int sim_read_mem_stats(mem_stats_t *mem) {
    mem->total_kb = 16384000; /* 16 GB */
    
    if (s_sim_state.mem_leak_remaining > 0) {
        s_sim_state.mem_leak_remaining--;
        s_sim_state.sim_rss_hog += 50000;
        s_sim_state.sim_vsize_hog += 80000;
        mem->available_kb = 1200000; /* ~7% available -> critical memory */
        mem->free_kb = 450000;
        mem->swap_total_kb = 8192000;
        mem->swap_free_kb = 2500000;  /* High swap usage */
    } else {
        mem->available_kb = 6800000; /* ~41% available */
        mem->free_kb = 2200000;
        mem->swap_total_kb = 8192000;
        mem->swap_free_kb = 7400000;  /* Normal low swap */
    }
    
    mem->buffers_kb = 640000;
    mem->cached_kb = 3960000;
    mem->active_kb = 7500000;
    mem->inactive_kb = 3500000;
    mem->used_kb = mem->total_kb - mem->available_kb;
    mem->swap_used_kb = mem->swap_total_kb - mem->swap_free_kb;
    mem->mem_util_percent = (float)mem->used_kb / (float)mem->total_kb * 100.0f;
    mem->swap_util_percent = (float)mem->swap_used_kb / (float)mem->swap_total_kb * 100.0f;
    
    return 0;
}

static int sim_read_disk_stats(disk_stats_t *disk) {
    disk->device_count = 1;
    disk_device_stats_t *d = &disk->devices[0];
    safe_strncpy(d->device_name, "nvme0n1", sizeof(d->device_name));
    
    bool io_storm = (s_sim_state.io_storm_remaining > 0);
    if (io_storm) s_sim_state.io_storm_remaining--;
    
    unsigned long long tick = s_sim_state.sim_ticks;
    d->reads_completed = 450000 + tick * (io_storm ? 3500 : 120);
    d->writes_completed = 820000 + tick * (io_storm ? 6000 : 340);
    d->sectors_read = 15000000 + tick * (io_storm ? 45000 : 1500);
    d->sectors_written = 28000000 + tick * (io_storm ? 85000 : 3200);
    d->time_doing_io_ms = 120000 + tick * (io_storm ? 880 : 85);
    d->io_in_progress = io_storm ? 14 : 1;
    
    return 0;
}

static int sim_read_all_processes(proc_info_t procs[MAX_PROCESSES], int *count) {
    /* Rich roster of typical Linux daemon and user processes */
    static const struct {
        int pid;
        int ppid;
        const char *name;
        const char *cmd;
        char state_char;
        int prio;
        int nice_val;
        int threads;
        unsigned long long vsize;
        unsigned long long rss;
    } base_procs[] = {
        { 1,    0, "systemd",         "/sbin/init",                 'S', 20,  0, 1, 168400, 12400 },
        { 240,  1, "systemd-journal", "/lib/systemd/journald",      'S', 20,  0, 1, 84200,   9800 },
        { 412,  1, "dbus-daemon",     "/usr/bin/dbus-daemon",       'S', 20,  0, 1, 54100,   6200 },
        { 580,  1, "sshd",            "/usr/sbin/sshd -D",          'S', 20,  0, 1, 72300,   8400 },
        { 788,  1, "mysqld",          "/usr/sbin/mysqld --daemon",  'S', 20,  0, 32, 1450000, 620000 },
        { 890,  1, "nginx",           "nginx: master process",      'S', 20,  0, 1, 112000, 14200 },
        { 891, 890, "nginx",          "nginx: worker process",      'S', 20,  0, 1, 115000, 18500 },
        { 1024, 1, "redis-server",    "/usr/bin/redis-server",      'S', 20,  0, 4, 380000, 145000 },
        { 1420, 580, "bash",          "-bash",                      'S', 20,  0, 1, 32000,   5400 },
        { 2100, 1420, "gcc",          "gcc -O3 -c matrix_comp.c",   'R', 20,  0, 4, 450000, 180000 },
        { 2105, 1420, "python3",      "python3 model_train.py",     'R', 25,  5, 8, 890000, 410000 },
        { 2210, 1, "rsyslogd",        "/usr/sbin/rsyslogd -n",      'S', 20,  0, 4, 210000, 11200 },
        { 2304, 1, "cron",            "/usr/sbin/cron -f",          'S', 20,  0, 1, 41000,   4100 },
        { 2512, 1420, "stress-ng",    "stress-ng --cpu 4 --io 2",   'R', 20,  0, 6, 680000, 240000 },
        { 2840, 1420, "syspilot",     "./syspilot --monitor",       'R', 20,  0, 2, 48000,   9800 }
    };
    
    int n = sizeof(base_procs) / sizeof(base_procs[0]);
    bool is_cpu_spike = (s_sim_state.cpu_spike_remaining > 0);
    bool is_mem_leak = (s_sim_state.mem_leak_remaining > 0);
    bool is_io_storm = (s_sim_state.io_storm_remaining > 0);
    
    for (int i = 0; i < n; i++) {
        proc_info_t *p = &procs[i];
        memset(p, 0, sizeof(*p));
        p->pid = base_procs[i].pid;
        p->ppid = base_procs[i].ppid;
        safe_strncpy(p->name, base_procs[i].name, sizeof(p->name));
        safe_strncpy(p->cmdline, base_procs[i].cmd, sizeof(p->cmdline));
        p->state_char = base_procs[i].state_char;
        p->priority = base_procs[i].prio;
        p->nice_val = base_procs[i].nice_val;
        p->num_threads = base_procs[i].threads;
        p->vsize_kb = base_procs[i].vsize;
        p->rss_kb = base_procs[i].rss;
        
        switch (p->state_char) {
            case 'R': p->state = STATE_RUNNING; break;
            case 'S': p->state = STATE_SLEEPING; break;
            case 'D': p->state = STATE_DISK_SLEEP; break;
            case 'Z': p->state = STATE_ZOMBIE; break;
            default:  p->state = STATE_RUNNING; break;
        }
        
        /* Allocate dynamic ticks based on behavior */
        unsigned long long tick = s_sim_state.sim_ticks;
        if (p->pid == 2512) { /* stress-ng or CPU hog */
            p->utime = tick * (is_cpu_spike ? 880 : 350);
            p->stime = tick * (is_cpu_spike ? 90 : 25);
            if (is_io_storm) {
                p->state_char = 'D';
                p->state = STATE_DISK_SLEEP;
            }
        } else if (p->pid == 2105) { /* python3 model training */
            p->utime = tick * (is_cpu_spike ? 720 : 280);
            p->stime = tick * 30;
            if (is_mem_leak) {
                p->rss_kb = s_sim_state.sim_rss_hog;
                p->vsize_kb = s_sim_state.sim_vsize_hog;
                p->rss_delta_kb = 50000;
            }
        } else if (p->pid == 2100) { /* gcc */
            p->utime = tick * (is_cpu_spike ? 540 : 150);
            p->stime = tick * 40;
        } else if (p->pid == 788) { /* mysqld */
            p->utime = tick * 60;
            p->stime = tick * 20;
            if (is_io_storm) {
                p->write_bytes_sec = 25000000; /* 25MB/s */
            }
        } else {
            p->utime = tick * 5 + (p->pid % 20);
            p->stime = tick * 2 + (p->pid % 5);
        }
        p->total_time_ticks = p->utime + p->stime;
    }
    
    *count = n;
    return 0;
}

/* ========================================================================= */
/* Public Interface Routing                                                  */
/* ========================================================================= */

int procfs_read_loadavg(float load[3]) {
    if (procfs_is_available()) return native_read_loadavg(load);
    return sim_read_loadavg(load);
}

int procfs_read_cpu_stats(cpu_stats_t *cpu) {
    if (procfs_is_available()) return native_read_cpu_stats(cpu);
    return sim_read_cpu_stats(cpu);
}

int procfs_read_mem_stats(mem_stats_t *mem) {
    if (procfs_is_available()) return native_read_mem_stats(mem);
    return sim_read_mem_stats(mem);
}

int procfs_read_disk_stats(disk_stats_t *disk) {
    if (procfs_is_available()) return native_read_disk_stats(disk);
    return sim_read_disk_stats(disk);
}

int procfs_read_all_processes(proc_info_t procs[MAX_PROCESSES], int *count) {
    if (procfs_is_available()) return native_read_all_processes(procs, count);
    return sim_read_all_processes(procs, count);
}
