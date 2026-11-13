#!/bin/sh
# Sums the last 7 days of Claude Code token usage from local transcripts and pushes it to the pet.
# Usage: usage.sh [--dry]
HOST="${CLAUDE_PET_HOST:-claude-pet.local}"
query=$(find ~/.claude/projects -name '*.jsonl' -mtime -8 -exec grep -h '"usage"' {} + | jq -Rnr '
  [range(6; -1; -1) as $i | (now | strflocaltime("%F") + "T12:00:00Z" | fromdate) - $i * 86400 | strftime("%F")] as $days
  | [inputs | fromjson? | select(.message.usage and .timestamp) | {
      id: "\(.message.id):\(.requestId)",
      s: .sessionId,
      d: (.timestamp | sub("\\.\\d+Z$"; "Z") | fromdate | strflocaltime("%F")),
      n: (.message.usage | [.input_tokens, .output_tokens, .cache_creation_input_tokens, .cache_read_input_tokens] | map(. // 0) | add)
    }]
  | unique_by(.id) as $e # a response is logged once per content block, count it once
  | [$days[] as $d | [$e[] | select(.d == $d).n] | add // 0] as $week
  | "today=\($week[-1])&sessions=\([$e[] | select(.d == $days[-1]).s] | unique | length)&week=\($week | join(","))"')
if [ "$1" = --dry ]; then
  echo "$query"
else
  curl -4 -s -m 3 "http://$HOST/usage?$query" >/dev/null
fi
