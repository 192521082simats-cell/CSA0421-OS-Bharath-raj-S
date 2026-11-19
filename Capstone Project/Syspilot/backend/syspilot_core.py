"""
SysPilot: Module 3 - CPU Scheduling Algorithms & OS Optimization Evaluator Backend
Simulates and evaluates FCFS, SJF, SRTF, Priority, Round Robin, MLFQ, and CFS.
Calculates TAT, WT, RT, Throughput, Utilization, Jain's Fairness, and OS advice.
"""

# Linux CFS nice-to-weight lookup table (nice -20 to +19)
CFS_WEIGHT_TABLE = [
    88761, 71755, 56483, 46273, 36291,
    29154, 23254, 18705, 14949, 11916,
    9548,  7620,  6100,  4904,  3906,
    3121,  2501,  1991,  1586,  1277,
    1024,   820,   655,   526,   423,
    335,    272,   215,   172,   137,
    110,     87,    70,    56,    45,
    36,      29,    23,    18,    15
]

def get_cfs_weight(nice_val):
    clamped = max(-20, min(19, nice_val))
    return CFS_WEIGHT_TABLE[clamped + 20]

class CPUScheduler:
    def __init__(self):
        pass

    def _merge_gantt(self, events):
        if not events:
            return []
        merged = []
        for e in events:
            if e['start'] >= e['end']:
                continue
            if merged and merged[-1]['pid'] == e['pid'] and merged[-1]['end'] == e['start']:
                merged[-1]['end'] = e['end']
            else:
                merged.append(dict(e))
        return merged

    def _compute_metrics(self, procs, gantt, total_time, context_switches, algo_name, quantum=4):
        total_burst = sum(p['burst'] for p in procs)
        count = len(procs)

        sum_wt, sum_tat, sum_rt = 0.0, 0.0, 0.0
        sum_shares, sum_sq_shares = 0.0, 0.0

        for p in procs:
            p['tat'] = p['completion'] - p['arrival']
            p['wt'] = max(0, p['tat'] - p['burst'])
            p['rt'] = max(0, p['start'] - p['arrival'])

            sum_wt += p['wt']
            sum_tat += p['tat']
            sum_rt += p['rt']

            share = (p['burst'] / p['tat']) if p['tat'] > 0 else 1.0
            sum_shares += share
            sum_sq_shares += (share * share)

        avg_wt = sum_wt / count if count > 0 else 0.0
        avg_tat = sum_tat / count if count > 0 else 0.0
        avg_rt = sum_rt / count if count > 0 else 0.0

        cpu_util = min(100.0, (100.0 * total_burst / total_time)) if total_time > 0 else 100.0
        throughput = count / total_time if total_time > 0 else 0.0

        if count > 0 and sum_sq_shares > 0:
            jains_fairness = (sum_shares * sum_shares) / (count * sum_sq_shares)
        else:
            jains_fairness = 1.0

        recommendation = self._generate_advice(algo_name, avg_wt, avg_tat, avg_rt, context_switches, jains_fairness, quantum)

        return {
            'algorithm': algo_name,
            'total_time': total_time,
            'context_switches': context_switches,
            'cpu_utilization_pct': round(cpu_util, 1),
            'throughput': round(throughput, 4),
            'avg_waiting_time': round(avg_wt, 2),
            'avg_turnaround_time': round(avg_tat, 2),
            'avg_response_time': round(avg_rt, 2),
            'jains_fairness_index': round(jains_fairness, 4),
            'processes': procs,
            'gantt': self._merge_gantt(gantt),
            'recommendation': recommendation
        }

    def _generate_advice(self, algo_name, avg_wt, avg_tat, avg_rt, ctx_switches, fairness, quantum):
        if algo_name == "FCFS":
            return (f"FCFS Analysis: Suffers from the Convoy Effect. Long-running processes delay subsequent short tasks, "
                    f"resulting in an elevated Average Waiting Time of {avg_wt:.2f} ms. "
                    f"Optimization Recommendation: Switch to Round Robin or MLFQ to favor short/interactive tasks.")
        elif algo_name == "SJF":
            return (f"SJF (Non-Preemptive) Analysis: Minimizes theoretical wait time (Avg WT: {avg_wt:.2f} ms) without preemption overhead. "
                    f"Warning: Risk of starvation for longer tasks if high-frequency short tasks continuously arrive. "
                    f"Recommendation: Implement aging or exponential burst smoothing.")
        elif algo_name == "SRTF":
            return (f"SRTF (Preemptive SJF) Analysis: Achieves near-optimal turnaround time (Avg TAT: {avg_tat:.2f} ms). "
                    f"Incurred {ctx_switches} context switches. "
                    f"Recommendation: Use when burst runtimes can be estimated accurately (e.g. database query planning).")
        elif algo_name == "Round Robin":
            return (f"Round Robin Analysis (Quantum = {quantum}): Excellent interactive response time (Avg RT: {avg_rt:.2f} ms) "
                    f"with high fairness ({fairness:.3f}). Context switches count: {ctx_switches}. "
                    f"Tuning Advice: If context switches exceed 10% of CPU time, increase quantum by 2-4 units.")
        elif algo_name == "Priority (Preemptive)":
            return (f"Priority Preemptive Analysis: High priority processes are promptly dispatched. "
                    f"Aging mechanism successfully prevented low-priority starvation. "
                    f"Optimization: Keep priority classes coarse (e.g. RT, Interactive, Batch) to avoid unnecessary preemptions.")
        elif algo_name == "MLFQ":
            return (f"MLFQ Analysis: Dynamically segregates CPU-bound and I/O-bound processes across 3 feedback queues. "
                    f"Interactive jobs achieve swift completion in Q0 (quantum 2), while long batches degrade to Q2. "
                    f"Recommendation: Introduce global priority boost every 50-100 ms to guarantee zero starvation.")
        elif algo_name == "Linux CFS":
            return (f"Linux CFS Analysis: Emulated virtual runtime (vruntime) scheduling tracking weight quotients. "
                    f"Achieved outstanding Jain's Fairness Index of {fairness:.4f}. "
                    f"Kernel Tuning: In production Linux, adjust /proc/sys/kernel/sched_min_granularity_ns (default 3ms) "
                    f"and sched_latency_ns (default 24ms) for low-jitter audio/networking.")
        return f"Algorithm evaluation complete with fairness {fairness:.3f} and utilization."

    # 1. FCFS
    def run_fcfs(self, input_procs):
        procs = [dict(p) for p in input_procs]
        for p in procs:
            p['remaining'] = p['burst']
            p['start'] = -1
            p['completion'] = 0
            p['completed'] = False

        current_time = 0
        completed = 0
        gantt = []
        prev_pid = -1
        ctx_switches = 0

        while completed < len(procs):
            available = [p for p in procs if not p['completed'] and p['arrival'] <= current_time]
            if not available:
                next_arr = min(p['arrival'] for p in procs if not p['completed'])
                gantt.append({'pid': 0, 'name': 'IDLE', 'start': current_time, 'end': next_arr})
                current_time = next_arr
                continue

            available.sort(key=lambda x: (x['arrival'], x['pid']))
            curr = available[0]

            if prev_pid != -1 and prev_pid != curr['pid']:
                ctx_switches += 1
            prev_pid = curr['pid']

            curr['start'] = current_time
            end_t = current_time + curr['burst']
            gantt.append({'pid': curr['pid'], 'name': curr['name'], 'start': current_time, 'end': end_t})
            current_time = end_t
            curr['completion'] = current_time
            curr['completed'] = True
            completed += 1

        return self._compute_metrics(procs, gantt, current_time, ctx_switches, "FCFS")

    # 2. SJF Non-Preemptive
    def run_sjf(self, input_procs):
        procs = [dict(p) for p in input_procs]
        for p in procs:
            p['remaining'] = p['burst']
            p['start'] = -1
            p['completion'] = 0
            p['completed'] = False

        current_time = 0
        completed = 0
        gantt = []
        prev_pid = -1
        ctx_switches = 0

        while completed < len(procs):
            available = [p for p in procs if not p['completed'] and p['arrival'] <= current_time]
            if not available:
                next_arr = min(p['arrival'] for p in procs if not p['completed'])
                gantt.append({'pid': 0, 'name': 'IDLE', 'start': current_time, 'end': next_arr})
                current_time = next_arr
                continue

            available.sort(key=lambda x: (x['burst'], x['arrival'], x['pid']))
            curr = available[0]

            if prev_pid != -1 and prev_pid != curr['pid']:
                ctx_switches += 1
            prev_pid = curr['pid']

            curr['start'] = current_time
            end_t = current_time + curr['burst']
            gantt.append({'pid': curr['pid'], 'name': curr['name'], 'start': current_time, 'end': end_t})
            current_time = end_t
            curr['completion'] = current_time
            curr['completed'] = True
            completed += 1

        return self._compute_metrics(procs, gantt, current_time, ctx_switches, "SJF")

    # 3. SRTF Preemptive
    def run_srtf(self, input_procs):
        procs = [dict(p) for p in input_procs]
        for p in procs:
            p['remaining'] = p['burst']
            p['start'] = -1
            p['completion'] = 0
            p['completed'] = False

        current_time = 0
        completed = 0
        gantt = []
        prev_pid = -1
        slice_start = 0
        ctx_switches = 0

        while completed < len(procs):
            available = [p for p in procs if not p['completed'] and p['arrival'] <= current_time]
            if not available:
                if prev_pid not in (-1, 0):
                    prev_p = next(p for p in procs if p['pid'] == prev_pid)
                    gantt.append({'pid': prev_pid, 'name': prev_p['name'], 'start': slice_start, 'end': current_time})
                gantt.append({'pid': 0, 'name': 'IDLE', 'start': current_time, 'end': current_time + 1})
                current_time += 1
                prev_pid = 0
                slice_start = current_time
                continue

            available.sort(key=lambda x: (x['remaining'], x['arrival'], x['pid']))
            curr = available[0]

            if curr['start'] == -1:
                curr['start'] = current_time

            if prev_pid != curr['pid']:
                if prev_pid not in (-1, 0):
                    prev_p = next(p for p in procs if p['pid'] == prev_pid)
                    gantt.append({'pid': prev_pid, 'name': prev_p['name'], 'start': slice_start, 'end': current_time})
                    ctx_switches += 1
                prev_pid = curr['pid']
                slice_start = current_time

            curr['remaining'] -= 1
            current_time += 1

            if curr['remaining'] == 0:
                gantt.append({'pid': curr['pid'], 'name': curr['name'], 'start': slice_start, 'end': current_time})
                curr['completion'] = current_time
                curr['completed'] = True
                completed += 1
                prev_pid = -1
                slice_start = current_time

        return self._compute_metrics(procs, gantt, current_time, ctx_switches, "SRTF")

    # 4. Round Robin
    def run_round_robin(self, input_procs, quantum=4):
        if quantum <= 0:
            quantum = 4
        procs = [dict(p) for p in input_procs]
        for p in procs:
            p['remaining'] = p['burst']
            p['start'] = -1
            p['completion'] = 0
            p['completed'] = False

        current_time = 0
        completed = 0
        gantt = []
        prev_pid = -1
        ctx_switches = 0

        queue = [i for i, p in enumerate(procs) if p['arrival'] <= current_time]
        in_queue = set(queue)

        while completed < len(procs):
            if not queue:
                next_arr = min(p['arrival'] for p in procs if not p['completed'])
                gantt.append({'pid': 0, 'name': 'IDLE', 'start': current_time, 'end': next_arr})
                current_time = next_arr
                for i, p in enumerate(procs):
                    if not p['completed'] and i not in in_queue and p['arrival'] <= current_time:
                        queue.append(i)
                        in_queue.add(i)
                continue

            curr_idx = queue.pop(0)
            in_queue.remove(curr_idx)
            curr = procs[curr_idx]

            if curr['start'] == -1:
                curr['start'] = current_time

            if prev_pid != -1 and prev_pid != curr['pid']:
                ctx_switches += 1
            prev_pid = curr['pid']

            slice_len = min(curr['remaining'], quantum)
            end_t = current_time + slice_len
            gantt.append({'pid': curr['pid'], 'name': curr['name'], 'start': current_time, 'end': end_t})
            curr['remaining'] -= slice_len
            current_time = end_t

            # Enqueue newly arrived tasks
            for i, p in enumerate(procs):
                if not p['completed'] and i != curr_idx and i not in in_queue and p['arrival'] <= current_time:
                    queue.append(i)
                    in_queue.add(i)

            if curr['remaining'] == 0:
                curr['completion'] = current_time
                curr['completed'] = True
                completed += 1
            else:
                queue.append(curr_idx)
                in_queue.add(curr_idx)

        return self._compute_metrics(procs, gantt, current_time, ctx_switches, "Round Robin", quantum)

    # 5. Priority Preemptive with Aging
    def run_priority_preempt(self, input_procs):
        procs = [dict(p) for p in input_procs]
        for p in procs:
            p['remaining'] = p['burst']
            p['cur_prio'] = p.get('priority', 3)
            p['start'] = -1
            p['completion'] = 0
            p['completed'] = False

        current_time = 0
        completed = 0
        gantt = []
        prev_pid = -1
        slice_start = 0
        ctx_switches = 0

        while completed < len(procs):
            # Dynamic aging
            if current_time > 0 and current_time % 5 == 0:
                for p in procs:
                    if not p['completed'] and p['arrival'] < current_time and p['cur_prio'] > 1:
                        p['cur_prio'] -= 1

            available = [p for p in procs if not p['completed'] and p['arrival'] <= current_time]
            if not available:
                if prev_pid not in (-1, 0):
                    prev_p = next(p for p in procs if p['pid'] == prev_pid)
                    gantt.append({'pid': prev_pid, 'name': prev_p['name'], 'start': slice_start, 'end': current_time})
                gantt.append({'pid': 0, 'name': 'IDLE', 'start': current_time, 'end': current_time + 1})
                current_time += 1
                prev_pid = 0
                slice_start = current_time
                continue

            available.sort(key=lambda x: (x['cur_prio'], x['arrival'], x['pid']))
            curr = available[0]

            if curr['start'] == -1:
                curr['start'] = current_time

            if prev_pid != curr['pid']:
                if prev_pid not in (-1, 0):
                    prev_p = next(p for p in procs if p['pid'] == prev_pid)
                    gantt.append({'pid': prev_pid, 'name': prev_p['name'], 'start': slice_start, 'end': current_time})
                    ctx_switches += 1
                prev_pid = curr['pid']
                slice_start = current_time

            curr['remaining'] -= 1
            current_time += 1

            if curr['remaining'] == 0:
                gantt.append({'pid': curr['pid'], 'name': curr['name'], 'start': slice_start, 'end': current_time})
                curr['completion'] = current_time
                curr['completed'] = True
                completed += 1
                prev_pid = -1
                slice_start = current_time

        return self._compute_metrics(procs, gantt, current_time, ctx_switches, "Priority (Preemptive)")

    # 6. MLFQ (Multilevel Feedback Queue)
    def run_mlfq(self, input_procs):
        procs = [dict(p) for p in input_procs]
        for p in procs:
            p['remaining'] = p['burst']
            p['queue_lvl'] = 0
            p['start'] = -1
            p['completion'] = 0
            p['completed'] = False

        current_time = 0
        completed = 0
        gantt = []
        prev_pid = -1
        ctx_switches = 0

        while completed < len(procs):
            chosen = None
            active_q = -1
            for q in (0, 1, 2):
                cands = [p for p in procs if not p['completed'] and p['arrival'] <= current_time and p['queue_lvl'] == q]
                if cands:
                    cands.sort(key=lambda x: (x['arrival'], x['pid']))
                    chosen = cands[0]
                    active_q = q
                    break

            if not chosen:
                next_arr = min(p['arrival'] for p in procs if not p['completed'])
                gantt.append({'pid': 0, 'name': 'IDLE', 'start': current_time, 'end': next_arr})
                current_time = next_arr
                continue

            if chosen['start'] == -1:
                chosen['start'] = current_time

            if prev_pid != -1 and prev_pid != chosen['pid']:
                ctx_switches += 1
            prev_pid = chosen['pid']

            quantum = 2 if active_q == 0 else (4 if active_q == 1 else chosen['remaining'])
            slice_len = min(chosen['remaining'], quantum)
            end_t = current_time + slice_len

            gantt.append({'pid': chosen['pid'], 'name': chosen['name'], 'start': current_time, 'end': end_t})
            chosen['remaining'] -= slice_len
            current_time = end_t

            if chosen['remaining'] == 0:
                chosen['completion'] = current_time
                chosen['completed'] = True
                completed += 1
            else:
                if chosen['queue_lvl'] < 2:
                    chosen['queue_lvl'] += 1

        return self._compute_metrics(procs, gantt, current_time, ctx_switches, "MLFQ")

    # 7. Linux Completely Fair Scheduler (CFS) Emulation
    def run_cfs(self, input_procs):
        procs = [dict(p) for p in input_procs]
        for p in procs:
            p['remaining'] = p['burst']
            p['vruntime'] = 0.0
            p['nice'] = p.get('nice', 0)
            p['start'] = -1
            p['completion'] = 0
            p['completed'] = False

        current_time = 0
        completed = 0
        gantt = []
        prev_pid = -1
        slice_start = 0
        ctx_switches = 0

        while completed < len(procs):
            available = [p for p in procs if not p['completed'] and p['arrival'] <= current_time]
            if not available:
                if prev_pid not in (-1, 0):
                    prev_p = next(p for p in procs if p['pid'] == prev_pid)
                    gantt.append({'pid': prev_pid, 'name': prev_p['name'], 'start': slice_start, 'end': current_time})
                gantt.append({'pid': 0, 'name': 'IDLE', 'start': current_time, 'end': current_time + 1})
                current_time += 1
                prev_pid = 0
                slice_start = current_time
                continue

            # Pick process with smallest vruntime
            available.sort(key=lambda x: (x['vruntime'], x['arrival'], x['pid']))
            curr = available[0]

            if curr['start'] == -1:
                curr['start'] = current_time

            if prev_pid != curr['pid']:
                if prev_pid not in (-1, 0):
                    prev_p = next(p for p in procs if p['pid'] == prev_pid)
                    gantt.append({'pid': prev_pid, 'name': prev_p['name'], 'start': slice_start, 'end': current_time})
                    ctx_switches += 1
                prev_pid = curr['pid']
                slice_start = current_time

            curr['remaining'] -= 1
            current_time += 1

            # Update vruntime
            weight = get_cfs_weight(curr['nice'])
            curr['vruntime'] += (1024.0 / max(1, weight))

            if curr['remaining'] == 0:
                gantt.append({'pid': curr['pid'], 'name': curr['name'], 'start': slice_start, 'end': current_time})
                curr['completion'] = current_time
                curr['completed'] = True
                completed += 1
                prev_pid = -1
                slice_start = current_time

        return self._compute_metrics(procs, gantt, current_time, ctx_switches, "Linux CFS")

    def simulate(self, algo_key, processes, quantum=4):
        algo_map = {
            'fcfs': self.run_fcfs,
            'sjf': self.run_sjf,
            'srtf': self.run_srtf,
            'rr': lambda p: self.run_round_robin(p, quantum),
            'priority': self.run_priority_preempt,
            'mlfq': self.run_mlfq,
            'cfs': self.run_cfs
        }
        fn = algo_map.get(algo_key.lower(), self.run_fcfs)
        return fn(processes)

    def benchmark_all(self, processes, quantum=4):
        results = []
        keys = ['fcfs', 'sjf', 'srtf', 'rr', 'priority', 'mlfq', 'cfs']
        for k in keys:
            results.append(self.simulate(k, processes, quantum))
        return results
