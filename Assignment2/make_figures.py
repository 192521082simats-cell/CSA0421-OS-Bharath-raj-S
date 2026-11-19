#!/usr/bin/env python3
"""Draws the diagrams and charts used in the report (saved in report_assets/).
The numbers in the charts are copied from the *_output.txt files produced by the C programs."""
import os
import matplotlib
matplotlib.use("Agg")
import matplotlib.pyplot as plt
from matplotlib.patches import FancyBboxPatch, Circle, Rectangle, FancyArrowPatch

OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "report_assets")
os.makedirs(OUT, exist_ok=True)


def box(ax, x, y, w, h, text, fc="#dbeafe", fs=10):
    ax.add_patch(FancyBboxPatch((x, y), w, h, boxstyle="round,pad=0.02,rounding_size=0.15", fc=fc, ec="black", lw=1.2))
    ax.text(x + w / 2, y + h / 2, text, ha="center", va="center", fontsize=fs, weight="bold")


def arrow(ax, p, q, label="", off=(0, 0.12), rad=0.0, fs=8):
    ax.add_patch(FancyArrowPatch(p, q, arrowstyle="-|>", mutation_scale=14, lw=1.3,
                                 connectionstyle=f"arc3,rad={rad}", color="black"))
    if label:
        ax.text((p[0] + q[0]) / 2 + off[0], (p[1] + q[1]) / 2 + off[1], label, ha="center", fontsize=fs, style="italic")


# ---------------- Q1 process state diagram ----------------
fig, ax = plt.subplots(figsize=(9, 4.4)); ax.set_xlim(0, 11); ax.set_ylim(0, 5.6); ax.axis("off")
box(ax, 0.2, 2.3, 1.6, 0.9, "NEW", "#e5e7eb")
box(ax, 3.2, 2.3, 1.7, 0.9, "READY", "#bbf7d0")
box(ax, 6.7, 2.3, 1.7, 0.9, "RUNNING", "#fde68a")
box(ax, 9.3, 2.3, 1.6, 0.9, "TERMINATED", "#fecaca", 8)
box(ax, 4.9, 4.3, 1.9, 0.9, "WAITING", "#bfdbfe")
arrow(ax, (1.8, 2.75), (3.2, 2.75), "admitted")
arrow(ax, (4.9, 2.95), (6.7, 2.95), "scheduler dispatch", off=(0, 0.15))
arrow(ax, (6.7, 2.55), (4.9, 2.55), "interrupt (time slice over)", off=(0, -0.3))
arrow(ax, (8.4, 2.75), (9.3, 2.75), "exit", off=(0, 0.15))
arrow(ax, (7.7, 3.2), (6.4, 4.3), "I/O or event wait", off=(0.9, 0.05))
arrow(ax, (5.2, 4.3), (4.2, 3.2), "I/O or event\ncompletion", off=(-0.9, 0.05))
ax.set_title("Process-state diagram (online-examination server session)", fontsize=11)
fig.tight_layout(); fig.savefig(f"{OUT}/process_states.png", dpi=170); plt.close(fig)

# ---------------- Q1 PCB ----------------
fig, ax = plt.subplots(figsize=(4.6, 5)); ax.set_xlim(0, 5); ax.set_ylim(0, 8.4); ax.axis("off")
fields = ["Process state (new/ready/running/waiting/terminated)", "Process ID (PID) and parent PID",
          "Program counter", "CPU registers (and stack pointer)", "CPU-scheduling info (priority, queue, time used)",
          "Memory-management info (page-table base, limits)", "Accounting info (CPU time, start time)",
          "I/O status info (open files, sockets, devices)"]
for i, f in enumerate(fields):
    y = 7.4 - i
    ax.add_patch(Rectangle((0.1, y), 4.8, 0.9, fc="#e0f2fe" if i % 2 == 0 else "#f0f9ff", ec="black"))
    ax.text(2.5, y + 0.45, f, ha="center", va="center", fontsize=7.6)
ax.set_title("Process control block (PCB)", fontsize=11)
fig.tight_layout(); fig.savefig(f"{OUT}/pcb.png", dpi=170); plt.close(fig)

# ---------------- Q6 resource-allocation graph ----------------
fig, ax = plt.subplots(figsize=(7.6, 4.8)); ax.set_xlim(0, 10); ax.set_ylim(0, 6.4); ax.axis("off")
J = {"J1": (1.4, 4.9), "J2": (8.6, 4.9), "J3": (5.0, 0.9)}
Rr = {"GPU": (5.0, 5.2), "LICENCE": (7.7, 2.2), "STORAGE": (2.3, 2.2)}
for n, (x, y) in J.items():
    ax.add_patch(Circle((x, y), 0.55, fc="#bbf7d0", ec="black", lw=1.4)); ax.text(x, y, n, ha="center", va="center", weight="bold")
for n, (x, y) in Rr.items():
    ax.add_patch(Rectangle((x - 0.7, y - 0.4), 1.4, 0.8, fc="#fde68a", ec="black", lw=1.4))
    ax.text(x, y, n, ha="center", va="center", weight="bold", fontsize=8.5)
    ax.add_patch(Circle((x, y - 0.62), 0.07, color="black"))
def e(p, q, c, rad=0.0, dash=False):
    ax.add_patch(FancyArrowPatch(p, q, arrowstyle="-|>", mutation_scale=15, lw=1.6, color=c, linestyle="--" if dash else "-",
                                 connectionstyle=f"arc3,rad={rad}", shrinkA=18, shrinkB=22))
# assignment edges (resource -> process): solid blue, request edges (process -> resource): dashed red
e(Rr["GPU"], J["J1"], "tab:blue", 0.12)
e(Rr["LICENCE"], J["J2"], "tab:blue", 0.12)
e(Rr["STORAGE"], J["J3"], "tab:blue", 0.12)
e(J["J1"], Rr["STORAGE"], "tab:red", 0.12, True)
e(J["J2"], Rr["GPU"], "tab:red", 0.12, True)
e(J["J3"], Rr["LICENCE"], "tab:red", 0.12, True)
ax.text(5, 6.1, "Resource-allocation graph: circular wait J1 -> STORAGE -> J3 -> LICENCE -> J2 -> GPU -> J1", ha="center", fontsize=8.5)
ax.plot([], [], color="tab:blue", label="assignment (resource held by job)")
ax.plot([], [], color="tab:red", ls="--", label="request (job waits for resource)")
ax.legend(loc="lower left", fontsize=7.5)
fig.tight_layout(); fig.savefig(f"{OUT}/rag.png", dpi=170); plt.close(fig)

# ---------------- Q1 creation / context-switch numbers ----------------
fig, axs = plt.subplots(1, 3, figsize=(9, 2.8))
axs[0].bar(["process", "thread"], [764.6, 65.6], color=["#f87171", "#60a5fa"]); axs[0].set_title("create + destroy (us)", fontsize=9)
axs[1].bar(["process", "thread"], [1.88, 0.96], color=["#f87171", "#60a5fa"]); axs[1].set_title("context switch (us)", fontsize=9)
axs[2].bar(["process", "thread"], [154.9, 20.8], color=["#f87171", "#60a5fa"]); axs[2].set_title("memory per idle session (kB)", fontsize=9)
for a in axs: a.tick_params(labelsize=8)
fig.tight_layout(); fig.savefig(f"{OUT}/q1_costs.png", dpi=170); plt.close(fig)

# ---------------- Q3 speed-up ----------------
fig, ax = plt.subplots(figsize=(6, 3.2))
labs = ["seq", "1", "2", "4", "8", "16", "64\n(1/camera)"]
sp = [1.00, 1.04, 2.00, 1.91, 1.93, 1.98, 1.98]
ax.bar(labs, sp, color="#60a5fa"); ax.axhline(2, color="red", ls="--", lw=1); ax.text(0, 2.05, "2 cores = ideal limit", color="red", fontsize=8)
ax.set_ylabel("speed-up vs sequential"); ax.set_xlabel("threads in pool"); ax.set_ylim(0, 2.4)
ax.set_title("Camera frames: speed-up on a 2-core machine", fontsize=10)
fig.tight_layout(); fig.savefig(f"{OUT}/q3_speedup.png", dpi=170); plt.close(fig)

# ---------------- Q4 IPC ----------------
fig, ax = plt.subplots(figsize=(6, 3.2))
sizes = ["1 MB", "10 MB", "100 MB"]; w = 0.27
pipe_t = [0.96, 7.19, 65.80]; mq_t = [1.19, 11.67, 106.39]; shm_t = [0.71, 5.71, 49.84]
x = range(3)
ax.bar([i - w for i in x], pipe_t, w, label="pipe"); ax.bar(list(x), mq_t, w, label="message queue"); ax.bar([i + w for i in x], shm_t, w, label="shared memory + semaphore")
ax.set_xticks(list(x)); ax.set_xticklabels(sizes); ax.set_yscale("log"); ax.set_ylabel("transfer time (ms, log scale)"); ax.legend(fontsize=7)
ax.set_title("Time to transfer data between two processes", fontsize=10)
fig.tight_layout(); fig.savefig(f"{OUT}/q4_ipc.png", dpi=170); plt.close(fig)

# ---------------- Q5 lock granularity ----------------
fig, ax = plt.subplots(figsize=(6, 3.2))
names = ["no lock\n(incorrect)", "global", "per train", "per coach"]
t2 = [2.32, 0.61, 1.37, 2.13]; t8 = [2.81, 1.20, 1.73, 2.46]
ax.bar([i - 0.2 for i in range(4)], t2, 0.4, label="2 threads"); ax.bar([i + 0.2 for i in range(4)], t8, 0.4, label="8 threads")
ax.set_xticks(range(4)); ax.set_xticklabels(names, fontsize=8); ax.set_ylabel("million bookings / second"); ax.legend(fontsize=8)
ax.set_title("Booking throughput by lock granularity", fontsize=10)
fig.tight_layout(); fig.savefig(f"{OUT}/q5_locks.png", dpi=170); plt.close(fig)

# ---------------- Q6 banker timing ----------------
fig, ax = plt.subplots(figsize=(5.6, 3.4))
n = [10, 100, 1000, 2000, 4000]
t_worst = [0.1, 9.7, 756.2, 3060.3, 12702.1]; t_best = [0.1, 0.4, 4.7, 6.0, 12.6]
ax.loglog(n, t_worst, "o-", label="worst case (measured)"); ax.loglog(n, t_best, "s-", label="best case (measured)")
ref = [t_worst[2] * (k / 1000) ** 2 for k in n]; ax.loglog(n, ref, "k--", lw=1, label="predicted  c*n^2")
ax.set_xlabel("processes n (m = 3)"); ax.set_ylabel("time (microseconds)"); ax.legend(fontsize=8)
ax.set_title("Banker's safety algorithm: time vs n", fontsize=10)
fig.tight_layout(); fig.savefig(f"{OUT}/q6_banker.png", dpi=170); plt.close(fig)
print("figures written to", OUT)
