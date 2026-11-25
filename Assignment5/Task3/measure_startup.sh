#!/usr/bin/env bash
# Measure time until the portal answers HTTP 200 on /health (container case).
set -euo pipefail
start=$(date +%s.%N)
docker run -d --rm --name portal-test -p 8000:8000 result-portal >/dev/null
until curl -sf http://localhost:8000/health >/dev/null; do sleep 0.05; done
end=$(date +%s.%N)
echo "Container start-up to first 200 OK: $(echo "$end - $start" | bc) s"
docker stop portal-test >/dev/null
