#!/bin/sh
# Claude Code status line: shows the plan limits in the terminal and pushes them to the pet.
# Claude Code passes rate_limits (Pro/Max, after the first reply) on stdin, see code.claude.com/docs/en/statusline
HOST="${CLAUDE_PET_HOST:-claude-pet.local}"
input=$(cat)
query=$(printf '%s' "$input" | jq -r '
  .rate_limits // empty
  | def lim: [(.used_percentage // 0 | round), ((.resets_at // now) - now | floor | if . < 0 then 0 else . end)];
    (.five_hour | lim) as [$s, $sr] | (.seven_day | lim) as [$w, $wr]
    | "s=\($s)&sr=\($sr)&w=\($w)&wr=\($wr)"' 2>/dev/null)
if [ -n "$query" ]; then
  curl -4 -s -m 1 "http://$HOST/usage?$query" >/dev/null 2>&1 &
fi
printf '%s' "$input" | jq -r '
  .rate_limits // empty
  | "5h \(.five_hour.used_percentage // 0 | round)% | week \(.seven_day.used_percentage // 0 | round)%"' 2>/dev/null
