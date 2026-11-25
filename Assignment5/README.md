# CSA04 – Operating Systems – Assignment 5 (CO5)

- Name: Bharath Raj S      Register number: ______
- Linux distribution/version: ______      Shell: bash ___      Python: 3.12
- Docker ___ / Compose ___ / minikube or kind ___ / VirtualBox or KVM ___

## Tasks
| Folder | Content | How to run | Result (fill in) |
|---|---|---|---|
| Task01 | log_summary.sh (TODO 1-2), disk_alert.sh, crontab.txt, portal.service, nginx_portal.conf, firewall_ssh.sh | `bash Task01/log_summary.sh` | output identical to expected |
| Task02 | app.py, Dockerfile (TODO 1-3), Dockerfile.multistage, Dockerfile.ubuntu, docker-compose.yml | `docker build -t result-portal . && docker run -p 8000:8000 result-portal` | image size ___, whoami = portal |
| Task03 | deployment.yaml (TODO 1-2), measure_startup.sh, benchmark.sh | `kubectl apply -f deployment.yaml` | 3 pods Running, pod recreated, scaled to 6 |
| Task04 | docker-compose.yml (portal + lms + db), capacity.py | `docker compose up -d`, `python3 capacity.py` | containers needed ___ |

Add screenshots and command outputs in each folder (e.g. Task02/screenshots/).
Make at least one commit per task.
