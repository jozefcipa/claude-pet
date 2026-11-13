#!/bin/sh
# Claude Code hook -> pet state. Usage: hook.sh <idle|thinking|working|attention|done>
# Never blocks Claude: curl runs in the background with a 1s timeout.
HOST="${CLAUDE_PET_HOST:-claude-pet.local}"
input=$(cat)
tool=$(printf '%s' "$input" | jq -r '.tool_name // empty' 2>/dev/null)
msg=$(printf '%s' "$input" | jq -r '.message // empty' 2>/dev/null) # Notification text, shown as a speech bubble
curl -4 -s -m 1 -G "http://$HOST/state" \
  --data-urlencode "s=$1" --data-urlencode "tool=$tool" --data-urlencode "msg=$msg" >/dev/null 2>&1 &
if [ "$1" = done ]; then
  "$(dirname "$0")/usage.sh" >/dev/null 2>&1 &
fi
exit 0
