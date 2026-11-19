#!/usr/bin/env python3
"""Q2 - CPU scheduling simulator: FCFS, SJF, SRTF, Priority, Round Robin, MLFQ.
Prints Gantt charts, waiting / turnaround / response times, the quantum study,
the starvation + ageing demo, and the video-service workload comparison.
Also writes Gantt PNGs into ../report_assets for the report.
Run: python3 sched.py | tee sched_output.txt
"""
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt

ASSETS = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "report_assets")
os.makedirs(ASSETS, exist_ok=True)

# name, arrival, burst, priority (smaller number = higher priority)
JOBS = [("P1", 0, 8, 3), ("P2", 1, 4, 1), ("P3", 2, 9, 4), ("P4", 3, 5, 2)]


def merge(seg):
    out = []
    for n, s, e in seg:
        if out and out[-1][0] == n and out[-1][2] == s:
            out[-1] = (n, out[-1][1], e)
        else:
            out.append((n, s, e))
    return out


def metrics(jobs, gantt):
    first, fin = {}, {}
    for n, s, e in gantt:
        if n == "idle":
            continue
        first.setdefault(n, s)
        fin[n] = e
    rows = []
    for n, a, b, _ in jobs:
        tat = fin[n] - a
        wt = tat - b
        rt = first[n] - a
        rows.append((n, a, b, fin[n], tat, wt, rt))
    return rows


def fcfs(jobs):
    t, g = 0, []
    for n, a, b, _ in sorted(jobs, key=lambda j: (j[1], j[0])):
        if t < a:
            g.append(("idle", t, a)); t = a
        g.append((n, t, t + b)); t += b
    return g


def sjf(jobs):
    t, g, rem = 0, [], list(jobs)
    while rem:
        ready = [j for j in rem if j[1] <= t]
        if not ready:
            nxt = min(j[1] for j in rem); g.append(("idle", t, nxt)); t = nxt; continue
        j = min(ready, key=lambda x: (x[2], x[1]))
        g.append((j[0], t, t + j[2])); t += j[2]; rem.remove(j)
    return g


def srtf(jobs):
    left = {j[0]: j[2] for j in jobs}
    t, g = 0, []
    while any(left.values()):
        ready = [j for j in jobs if j[1] <= t and left[j[0]] > 0]
        if not ready:
            g.append(("idle", t, t + 1)); t += 1; continue
        j = min(ready, key=lambda x: (left[x[0]], x[1]))
        g.append((j[0], t, t + 1)); left[j[0]] -= 1; t += 1
    return merge(g)


def priority(jobs, preemptive=False):
    t, g, rem, left = 0, [], list(jobs), {j[0]: j[2] for j in jobs}
    if preemptive:
        while any(left.values()):
            ready = [j for j in jobs if j[1] <= t and left[j[0]] > 0]
            if not ready:
                g.append(("idle", t, t + 1)); t += 1; continue
            j = min(ready, key=lambda x: (x[3], x[1]))
            g.append((j[0], t, t + 1)); left[j[0]] -= 1; t += 1
        return merge(g)
    while rem:
        ready = [j for j in rem if j[1] <= t]
        if not ready:
            nxt = min(j[1] for j in rem); g.append(("idle", t, nxt)); t = nxt; continue
        j = min(ready, key=lambda x: (x[3], x[1]))
        g.append((j[0], t, t + j[2])); t += j[2]; rem.remove(j)
    return g


def rr(jobs, q):
    jobs = sorted(jobs, key=lambda j: (j[1], j[0]))
    left = {j[0]: j[2] for j in jobs}
    t, g, queue, added = 0, [], [], set()
    def admit():
        for j in jobs:
            if j[1] <= t and j[0] not in added:
                queue.append(j[0]); added.add(j[0])
    admit()
    while len(added) < len(jobs) or queue:
        if not queue:
            nxt = min(j[1] for j in jobs if j[0] not in added)
            g.append(("idle", t, nxt)); t = nxt; admit(); continue
        n = queue.pop(0)
        run = min(q, left[n])
        g.append((n, t, t + run)); t += run; left[n] -= run
        admit()
        if left[n] > 0:
            queue.append(n)
    return g


def mlfq(jobs, quanta=(2, 8, 32)):
    """3 queues: Q0 q=2, Q1 q=8, Q2 q=32 (RR); new jobs enter Q0; a job that uses its
    full quantum is demoted; a job arriving in a higher queue pre-empts a lower one."""
    left = {j[0]: j[2] for j in jobs}
    arr = {j[0]: j[1] for j in jobs}
    level = {j[0]: 0 for j in jobs}
    queues = [[] for _ in quanta]
    admitted = set()
    t, g = 0, []
    def admit():
        for j in sorted(jobs, key=lambda x: (x[1], x[0])):
            if j[1] <= t and j[0] not in admitted:
                queues[0].append(j[0]); admitted.add(j[0])
    admit()
    while any(left.values()):
        lv = next((i for i, qq in enumerate(queues) if qq), None)
        if lv is None:
            nxt = min(arr[n] for n in left if n not in admitted)
            g.append(("idle", t, nxt)); t = nxt; admit(); continue
        n = queues[lv].pop(0)
        used = 0
        while used < quanta[lv] and left[n] > 0:
            g.append((n, t, t + 1)); t += 1; used += 1; left[n] -= 1
            before = len(queues[0])
            admit()
            if lv > 0 and len(queues[0]) > before:   # higher-priority arrival pre-empts
                break
        if left[n] > 0:
            if used >= quanta[lv]:
                level[n] = min(level[n] + 1, len(quanta) - 1)   # used full quantum -> demote
            queues[level[n]].append(n) if used >= quanta[lv] else queues[lv].insert(0, n)
    return merge(g)


def fmt_gantt(g):
    return " ".join(f"{n}({s}-{e})" for n, s, e in g)


def report(title, jobs, g, show=True):
    rows = metrics(jobs, g)
    n = len(rows)
    avg = lambda i: sum(r[i] for r in rows) / n
    if show:
        print(f"\n=== {title} ===")
        print("Gantt:", fmt_gantt(g))
        print(f"{'Job':<5}{'Arr':>4}{'Burst':>6}{'Finish':>7}{'TAT':>5}{'WT':>5}{'RT':>5}")
        for r in rows:
            print(f"{r[0]:<5}{r[1]:>4}{r[2]:>6}{r[3]:>7}{r[4]:>5}{r[5]:>5}{r[6]:>5}")
        print(f"Average: TAT={avg(4):.2f}  WT={avg(5):.2f}  RT={avg(6):.2f}  "
              f"context switches={max(len([s for s in g if s[0] != 'idle']) - 1, 0)}")
    return avg(4), avg(5), avg(6)


def draw_gantt(title, g, fname):
    names = sorted({n for n, _, _ in g if n != "idle"})
    cmap = plt.get_cmap("tab10")
    col = {n: cmap(i % 10) for i, n in enumerate(names)}
    fig, ax = plt.subplots(figsize=(9, 1.5))
    for n, s, e in g:
        if n == "idle":
            continue
        ax.barh(0, e - s, left=s, color=col[n], edgecolor="black", height=0.6)
        ax.text((s + e) / 2, 0, n, ha="center", va="center", fontsize=8, color="white", weight="bold")
    ticks = sorted({s for _, s, _ in g} | {e for _, _, e in g})
    ax.set_xticks(ticks); ax.set_yticks([])
    ax.tick_params(axis="x", labelsize=7)
    ax.set_title(title, fontsize=10, loc="left")
    ax.set_xlim(0, max(e for _, _, e in g))
    fig.tight_layout()
    fig.savefig(os.path.join(ASSETS, fname), dpi=160)
    plt.close(fig)


if __name__ == "__main__":
    print("JOB SET (name, arrival, burst, priority [1 = highest]):", JOBS)
    algos = [
        ("FCFS", fcfs(JOBS), "gantt_fcfs.png"),
        ("SJF (non-preemptive)", sjf(JOBS), "gantt_sjf.png"),
        ("SRTF (preemptive SJF)", srtf(JOBS), "gantt_srtf.png"),
        ("Priority (non-preemptive)", priority(JOBS), "gantt_prio.png"),
        ("Round Robin (q=3)", rr(JOBS, 3), "gantt_rr3.png"),
    ]
    summary = []
    for title, g, f in algos:
        summary.append((title, *report(title, JOBS, g)))
        draw_gantt(title, g, f)

    print("\n=== SUMMARY (same 4 jobs) ===")
    print(f"{'Algorithm':<28}{'avg TAT':>9}{'avg WT':>9}{'avg RT':>9}")
    for s in summary:
        print(f"{s[0]:<28}{s[1]:>9.2f}{s[2]:>9.2f}{s[3]:>9.2f}")

    print("\n=== ROUND ROBIN QUANTUM STUDY ===")
    print(f"{'q':>3}{'slices':>8}{'ctx switches':>14}{'avg TAT':>9}{'avg WT':>9}{'avg RT':>9}")
    qs, wts, cs = [], [], []
    for q in (1, 3, 5, 20):
        g = rr(JOBS, q)
        tat, wt, rt = report("", JOBS, g, show=False)
        sl = len(g)
        print(f"{q:>3}{sl:>8}{sl - 1:>14}{tat:>9.2f}{wt:>9.2f}{rt:>9.2f}")
        qs.append(q); wts.append(wt); cs.append(sl - 1)
        if q in (1, 5, 20):
            draw_gantt(f"Round Robin (q={q})", g, f"gantt_rr{q}.png")
    fig, ax = plt.subplots(figsize=(5, 3))
    ax.plot([str(q) for q in qs], wts, marker="o", label="avg waiting time")
    ax2 = ax.twinx(); ax2.plot([str(q) for q in qs], cs, marker="s", color="tab:red", label="context switches")
    ax.set_xlabel("time quantum"); ax.set_ylabel("avg WT"); ax2.set_ylabel("context switches")
    fig.tight_layout(); fig.savefig(os.path.join(ASSETS, "rr_quantum.png"), dpi=160); plt.close(fig)

    # ---------- starvation and ageing ----------
    print("\n=== STARVATION IN PRIORITY SCHEDULING, AND AGEING ===")
    def prio_aging(jobs, interval):
        t, g, rem = 0, [], list(jobs)
        while rem:
            ready = [j for j in rem if j[1] <= t]
            if not ready:
                nxt = min(j[1] for j in rem); g.append(("idle", t, nxt)); t = nxt; continue
            eff = lambda j: j[3] - ((t - j[1]) // interval if interval else 0)
            j = min(ready, key=lambda x: (eff(x), x[1]))
            g.append((j[0], t, t + j[2])); t += j[2]; rem.remove(j)
        return g
    # a steady stream of short high-priority jobs (one every 2 units, each needing 2 units)
    # keeps the CPU permanently busy; the low-priority job arrives at t=1
    stream = [("LOW", 1, 3, 10)] + [(f"H{i}", 2 * i, 2, 1) for i in range(15)]
    for interval, label in ((0, "no ageing"), (2, "ageing: +1 priority level per 2 time units waited")):
        g = prio_aging(stream, interval)
        start = next(s for n, s, e in g if n == "LOW")
        print(f"{label:<55}: LOW job (arrived t=1) first runs at t={start}")

    # ---------- video service workload ----------
    print("\n=== VIDEO-ENCODING SERVICE WORKLOAD (single processor, times in seconds) ===")
    VID = [("FILM1", 0, 1200, 2), ("FILM2", 10, 1200, 2)] + \
          [(f"T{i + 1}", a, 2, 1) for i, a in enumerate((5, 15, 25, 40, 60, 90))]
    print("jobs:", [(n, a, b) for n, a, b, _ in VID])
    vid_algos = [
        ("FCFS", fcfs(VID)),
        ("SJF (non-preemptive)", sjf(VID)),
        ("SRTF", srtf(VID)),
        ("Priority (thumbnails first)", priority(VID)),
        ("Round Robin q=4", rr(VID, 4)),
        ("Round Robin q=100", rr(VID, 100)),
        ("MLFQ (q = 2/8/32)", mlfq(VID)),
    ]
    print(f"{'Algorithm':<30}{'avg WT all':>11}{'avg WT thumb':>13}{'avg WT film':>12}{'avg RT thumb':>13}{'ctx sw':>8}")
    vnames, vthumb, vfilm = [], [], []
    for title, g in vid_algos:
        rows = metrics(VID, g)
        th = [r for r in rows if r[0].startswith("T")]
        fl = [r for r in rows if r[0].startswith("F")]
        m = lambda rs, i: sum(r[i] for r in rs) / len(rs)
        ctx = len([s for s in g if s[0] != "idle"]) - 1
        print(f"{title:<30}{m(rows, 5):>11.1f}{m(th, 5):>13.1f}{m(fl, 5):>12.1f}{m(th, 6):>13.1f}{ctx:>8}")
        vnames.append(title); vthumb.append(m(th, 5)); vfilm.append(m(fl, 5))
    fig, ax = plt.subplots(figsize=(8, 3.2))
    x = range(len(vnames)); w = 0.4
    ax.bar([i - w / 2 for i in x], vthumb, w, label="thumbnail jobs (2 s)")
    ax.bar([i + w / 2 for i in x], vfilm, w, label="film jobs (20 min)")
    ax.set_xticks(list(x)); ax.set_xticklabels(vnames, rotation=25, ha="right", fontsize=7)
    ax.set_ylabel("avg waiting time (s)"); ax.legend(fontsize=7); ax.set_yscale("symlog")
    fig.tight_layout(); fig.savefig(os.path.join(ASSETS, "video_wt.png"), dpi=160); plt.close(fig)
    mg = mlfq(VID)
    print("\nMLFQ Gantt (first 14 slices):", fmt_gantt(mg[:14]))
