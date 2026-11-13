#!/bin/sh
# Show a speech bubble on the pet. Usage: say.sh "message" [duration_ms]
HOST="${CLAUDE_PET_HOST:-claude-pet.local}"
if [ -z "$1" ]; then
  echo 'usage: say.sh "message" [duration_ms]' >&2
  exit 2
fi
curl -4 -sf -m 2 -G "http://$HOST/say" --data-urlencode "msg=$1" --data-urlencode "ms=${2:-0}" >/dev/null \
  || { echo "pet not reachable at $HOST" >&2; exit 1; }
