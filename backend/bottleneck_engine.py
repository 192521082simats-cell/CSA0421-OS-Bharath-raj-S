"""
SysPilot: Module 2 - Intelligent Bottleneck and Resource-Conflict Analysis Backend
Applies heuristic rule graphs & multi-variable statistical anomaly detection.
"""

class BottleneckEngine:
    def __init__(self):
        pass

    def analyze(self, cpu_info, mem_info, disk_info, processes):
        alerts = []
        conflicts = []
        health_penalty = 0.0

        top_cpu_proc = processes[0] if processes else None
        top_mem_proc = max(processes, key=lambda x: x['rss_mb']) if processes else None
        d_state_procs = [p for p in processes if p['state'] == 'D']

        # RULE 1: CPU Saturation Check
        cpu_pct = cpu_info['total_pct']
        if cpu_pct >= 85.0:
            health_penalty += 35.0
            alerts.append({
                'type': 'CPU_SATURATION',
                'severity': 'CRITICAL',
                'component': 'CPU Subsystem',
                'title': 'Severe CPU Thrashing & Core Saturation',
                'description': f"Aggregate CPU load is {cpu_pct}%, surpassing the 85.0% critical saturation boundary. Ready queue latency is severely elevated.",
                'culprit_pid': top_cpu_proc['pid'] if top_cpu_proc else 0,
                'culprit_name': top_cpu_proc['name'] if top_cpu_proc else "Unknown",
                'metric_value': cpu_pct,
                'threshold': 85.0,
                'recommendation': f"Renice or limit PID {top_cpu_proc['pid']} ('{top_cpu_proc['name']}') using CFS bandwidth quota or cgroups."
            })
        elif cpu_pct >= 70.0:
            health_penalty += 15.0
            alerts.append({
                'type': 'CPU_PRESSURE',
                'severity': 'MEDIUM',
                'component': 'CPU Subsystem',
                'title': 'Elevated CPU Utilization Pressure',
                'description': f"CPU is operating at {cpu_pct}%. Headroom for burst workloads is constrained.",
                'culprit_pid': top_cpu_proc['pid'] if top_cpu_proc else 0,
                'culprit_name': top_cpu_proc['name'] if top_cpu_proc else "Unknown",
                'metric_value': cpu_pct,
                'threshold': 70.0,
                'recommendation': "Distribute load across multi-queue affinity masks or evaluate CFS latency target."
            })

        # RULE 2: Memory Pressure & Swap Thrashing Check
        mem_pct = mem_info['usage_pct']
        if mem_pct >= 90.0:
            health_penalty += 35.0
            alerts.append({
                'type': 'MEMORY_EXHAUSTION',
                'severity': 'CRITICAL',
                'component': 'Memory Subsystem',
                'title': 'Critical Physical RAM Exhaustion (OOM Danger)',
                'description': f"RAM consumption is at {mem_pct}%. Kernel OOM killer may terminate vital system tasks.",
                'culprit_pid': top_mem_proc['pid'] if top_mem_proc else 0,
                'culprit_name': top_mem_proc['name'] if top_mem_proc else "Unknown",
                'metric_value': mem_pct,
                'threshold': 90.0,
                'recommendation': f"Investigate memory growth in PID {top_mem_proc['pid']} ('{top_mem_proc['name']}', RSS: {top_mem_proc['rss_mb']} MB)."
            })
        elif mem_pct >= 80.0:
            health_penalty += 12.0
            alerts.append({
                'type': 'MEMORY_PRESSURE',
                'severity': 'MEDIUM',
                'component': 'Memory Subsystem',
                'title': 'High Memory Allocation Threshold',
                'description': f"System RAM usage is {mem_pct}%. Page cache reclaim operations are active.",
                'culprit_pid': top_mem_proc['pid'] if top_mem_proc else 0,
                'culprit_name': top_mem_proc['name'] if top_mem_proc else "Unknown",
                'metric_value': mem_pct,
                'threshold': 80.0,
                'recommendation': "Clear inactive kernel slab caches or increase physical memory allocation."
            })

        # Swap Check
        if mem_info['swap_pct'] >= 30.0:
            health_penalty += 20.0
            alerts.append({
                'type': 'SWAP_THRASHING',
                'severity': 'HIGH',
                'component': 'Virtual Memory',
                'title': 'Active Virtual Memory Swap Thrashing',
                'description': f"Swap usage is {mem_info['swap_pct']}%. Page fault handling is incurring severe disk I/O latency penalties.",
                'metric_value': mem_info['swap_pct'],
                'threshold': 30.0,
                'recommendation': "Tune vm.swappiness downward (e.g. sysctl -w vm.swappiness=10) and inspect leaking processes."
            })

        # RULE 3: I/O Wait Storage Starvation Check
        iowait = cpu_info['iowait_pct']
        if iowait >= 12.0 or len(d_state_procs) > 0:
            health_penalty += 25.0
            sev = 'CRITICAL' if iowait >= 20.0 else 'HIGH'
            alerts.append({
                'type': 'IO_WAIT_SATURATION',
                'severity': sev,
                'component': 'Block Storage / Disk',
                'title': 'Severe I/O Wait Blocking CPU Scheduling',
                'description': f"CPU is spending {iowait}% of clock cycles stalled in wait states. Found {len(d_state_procs)} processes in Uninterruptible Sleep (D-state).",
                'metric_value': iowait,
                'threshold': 12.0,
                'recommendation': "Migrate intense I/O workloads to NVMe queues or assign BFQ/Kyber I/O schedulers with ionice priority."
            })

        # RULE 4: Multi-Process Resource Conflicts
        # Conflict A: Multiple heavy CPU consumers contending
        heavy_cpu = [p for p in processes if p['cpu_pct'] >= 25.0]
        if len(heavy_cpu) >= 2:
            p1, p2 = heavy_cpu[0], heavy_cpu[1]
            health_penalty += 12.0
            conflicts.append({
                'pid_a': p1['pid'],
                'name_a': p1['name'],
                'pid_b': p2['pid'],
                'name_b': p2['name'],
                'type': 'CPU Affinity & Timeslice Contention',
                'details': f"PID {p1['pid']} ('{p1['name']}', {p1['cpu_pct']}%) and PID {p2['pid']} ('{p2['name']}', {p2['cpu_pct']}%) are concurrently competing for CPU scheduler time slices.",
                'severity': 'HIGH',
                'remedy': "Pin tasks to distinct CPU affinity sets using taskset / cpusets to prevent cache invalidation."
            })

        # Conflict B: Priority Inversion or Contention
        for p in processes:
            if p['state'] == 'D' and p['nice'] > 5:
                # low priority process holding up I/O
                interactive = next((x for x in processes if x['nice'] < 0), None)
                if interactive:
                    health_penalty += 15.0
                    conflicts.append({
                        'pid_a': p['pid'],
                        'name_a': p['name'],
                        'pid_b': interactive['pid'],
                        'name_b': interactive['name'],
                        'type': 'Priority Inversion / Resource Starvation',
                        'details': f"Background task '{p['name']}' (nice {p['nice']}) is generating excessive disk bus activity, delaying interactive task '{interactive['name']}' (nice {interactive['nice']}).",
                        'severity': 'CRITICAL',
                        'remedy': "Apply ionice -c3 (idle class) to background worker and enable priority inheritance protocols."
                    })
                    break

        # Calculate Overall Health Score
        health = max(10.0, min(100.0, 100.0 - health_penalty))
        if health >= 80.0:
            status = "OPTIMAL"
        elif health >= 50.0:
            status = "STRESSED"
        else:
            status = "CRITICAL"

        return {
            'health_score': round(health, 1),
            'general_status': status,
            'alert_count': len(alerts),
            'alerts': alerts,
            'conflict_count': len(conflicts),
            'conflicts': conflicts
        }
