#!/bin/sh
# Claude Code hook -> pet state. Usage: hook.sh <idle|thinking|working|attention|done> [mac name]
# Never blocks Claude: curl runs in the background with a 1s timeout.
# The session id is the lock key: while one session is busy the pet ignores the others.
HOST="${CLAUDE_PET_HOST:-claude-pet.local}"
state=$1
mac=${2:-$(hostname -s | cut -c1-10)}
input=$(cat)
sid=$(printf '%s' "$input" | jq -r '.session_id // empty' 2>/dev/null)
tool=$(printf '%s' "$input" | jq -r '.tool_name // empty' 2>/dev/null)
msg=$(printf '%s' "$input" | jq -r '.message // empty' 2>/dev/null) # Notification text, shown as a speech bubble

if [ "$state" = attention ]; then
  # Only notifications that actually block on the user get the flashing attention scene.
  # idle_prompt fires ~60 s after Claude finishes: just a speech bubble, the pet stays idle.
  case $(printf '%s' "$input" | jq -r '.notification_type // empty' 2>/dev/null) in
    idle_prompt)
      curl -4 -s -m 1 -G "http://$HOST/say" \
        --data-urlencode "msg=${msg:-Waiting for you}" --data-urlencode "ms=30000" --data-urlencode "sid=$sid" >/dev/null 2>&1 &
      exit 0
      ;;
    auth_success) exit 0 ;;
  esac
fi

# Tools that ask the user something go through PreToolUse, not Notification
if [ "$state" = working ]; then
  case $tool in
    AskUserQuestion | ExitPlanMode)
      state=attention
      msg=$(printf '%s' "$input" | jq -r '.tool_input.questions[0].question // empty' 2>/dev/null)
      ;;
  esac
fi

curl -4 -s -m 1 -G "http://$HOST/state" \
  --data-urlencode "s=$state" --data-urlencode "tool=$tool" --data-urlencode "msg=$msg" \
  --data-urlencode "sid=$sid" --data-urlencode "mac=$mac" >/dev/null 2>&1 &
exit 0
