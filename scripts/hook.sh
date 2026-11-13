#!/bin/sh
# Claude Code hook -> pet state. Usage: hook.sh <idle|thinking|working|attention|done>
# Never blocks Claude: curl runs in the background with a 1s timeout.
HOST="${CLAUDE_PET_HOST:-claude-pet.local}"
state=$1
input=$(cat)
tool=$(printf '%s' "$input" | jq -r '.tool_name // empty' 2>/dev/null)
msg=$(printf '%s' "$input" | jq -r '.message // empty' 2>/dev/null) # Notification text, shown as a speech bubble

if [ "$state" = attention ]; then
  # Only notifications that actually block on the user get the flashing attention scene.
  # idle_prompt fires ~60 s after Claude finishes: just a speech bubble, the pet stays idle.
  case $(printf '%s' "$input" | jq -r '.notification_type // empty' 2>/dev/null) in
    idle_prompt)
      curl -4 -s -m 1 -G "http://$HOST/say" \
        --data-urlencode "msg=${msg:-Waiting for you}" --data-urlencode "ms=30000" >/dev/null 2>&1 &
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
  --data-urlencode "s=$state" --data-urlencode "tool=$tool" --data-urlencode "msg=$msg" >/dev/null 2>&1 &
if [ "$state" = done ]; then
  "$(dirname "$0")/usage.sh" >/dev/null 2>&1 &
fi
exit 0
