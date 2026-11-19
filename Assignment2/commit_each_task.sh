#!/usr/bin/env bash
# Creates the progressive commit history (one commit per task) required by the assignment.
# Edit REGNO and the remote URL, then run:  bash commit_each_task.sh
# Tip: run it task by task over several days if you want a more natural history.
set -e
REGNO="YOUR_REGISTER_NUMBER"
REPO="CSA04-${REGNO}-Assignment2"
cd "$(dirname "$0")"
[ -d .git ] || git init -b main

printf '*.o\nfork_demo\nscale_test\nmem_test\nrr\nsched_cost\ncameras\npipe_readings\npipe_noclose\nshm_readings\nipc_bench\nbounded_buffer\nbb_multi\nbb_multi_nomutex\nmonitor_buffer\npeterson\ndining\ndining_naive\nreaders_writers\nreaders_writers_unsafe\nticket_locks\nsync_alt\nbanker\nbanker_bench\ndetect\nrr_q?\nrr_q??\n' > .gitignore

git add .gitignore README.md                      && git commit -m "Add README and .gitignore"
git add Task01                                    && git commit -m "Q1: fork demo, ps//proc evidence, process vs thread costs"
git add Task02 report_assets/gantt_*.png report_assets/rr_quantum.png report_assets/video_wt.png \
                                                  && git commit -m "Q2: Round Robin (completed), scheduling simulator, quantum study, MLFQ"
git add Task03                                    && git commit -m "Q3: multithreaded camera processing, speed-ups, race demo"
git add Task04                                    && git commit -m "Q4: pipe (completed), shared memory + semaphores, transfer benchmark"
git add Task05                                    && git commit -m "Q5: bounded buffer (completed), monitor, Peterson, locks, benchmarks"
git add Task06                                    && git commit -m "Q6: Banker's algorithm (completed), timing, detection"
git add make_figures.py report_assets CSA04_Assignment2_Report.docx commit_each_task.sh \
                                                  && git commit -m "Add report, figures and helper scripts"

echo "Now create an empty GitHub repo named ${REPO}, share it with the faculty, then run:"
echo "  git remote add origin https://github.com/<your-username>/${REPO}.git && git push -u origin main"
