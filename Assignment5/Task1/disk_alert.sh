#!/usr/bin/env bash
# Disk-space alert: warn when any mounted file system is above THRESHOLD % used.
set -euo pipefail
THRESHOLD=${1:-80}
ALERT_LOG=${ALERT_LOG:-/tmp/disk_alert.log}

df -P -x tmpfs -x devtmpfs | awk 'NR>1 {gsub("%","",$5); print $5, $6}' |
while read -r used mount; do
  if [ "$used" -ge "$THRESHOLD" ]; then
    echo "$(date '+%F %T') ALERT: $mount is ${used}% full (threshold ${THRESHOLD}%)" | tee -a "$ALERT_LOG"
  fi
done
echo "$(date '+%F %T') disk check finished (threshold ${THRESHOLD}%)" >> "$ALERT_LOG"
