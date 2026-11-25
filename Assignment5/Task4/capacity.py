"""B4.2 capacity estimate. Edit R (measured requests/second per container)."""
import math
students, window_s = 150_000, 2 * 3600
req_per_student = 5       # login page, login POST, result page, CSS/JS, PDF
peak_factor = 4           # logins are bunched at the start of the window
headroom = 0.7            # run containers at <= 70 % of measured capacity
R = 200                   # <-- REPLACE with your measured req/s per container

avg = students * req_per_student / window_s
peak = avg * peak_factor
need = math.ceil(peak / (R * headroom)) + 1     # +1 spare (N+1)
print(f"average load : {avg:.0f} req/s")
print(f"peak load    : {peak:.0f} req/s")
print(f"containers   : {need} (R={R} req/s each, 70% headroom, +1 spare)")
