#!/usr/bin/env bash
# Usage: ./benchmark.sh http://localhost:8000/   (run 3 times, record the median)
URL=${1:-http://localhost:8000/}
ab -n 20000 -c 100 "$URL" | grep -E "Requests per second|Time per request|Failed requests"
