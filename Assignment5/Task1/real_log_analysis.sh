#!/usr/bin/env bash
# Hourly analysis of the real Nginx access log (combined format):
# status codes are field 9, client IP is field 1.
set -euo pipefail
LOG=${1:-/var/log/nginx/access.log}
echo "== $(date '+%F %T') report for $LOG =="
echo "Requests per status code:"
awk '{print $9}' "$LOG" | sort | uniq -c | awk '{print $2, $1}' | sort -n
echo "Top 5 clients:"
awk '{print $1}' "$LOG" | sort | uniq -c | sort -rn | awk 'NR<=5 {print $2, $1}'
echo "5xx errors: $(awk '$9 ~ /^5/ {n++} END {print n+0}' "$LOG")"
