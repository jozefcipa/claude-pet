# claude-pet tasks. Run `just` to list them.

pio := `command -v pio || echo "$HOME/.platformio/penv/bin/pio"`
host := env("CLAUDE_PET_HOST", "claude-pet.local")
hook := justfile_directory() / "scripts/hook.sh"
statusline := justfile_directory() / "scripts/statusline.sh"
settings := env("HOME") / ".claude/settings.json"

# list recipes
default:
    @just --list

# create src/secrets.h from the example and check the tools the scripts need
setup:
    #!/bin/sh
    set -e
    if [ -f src/secrets.h ]; then
      echo "src/secrets.h already exists, leaving it alone"
    else
      cp src/secrets.example.h src/secrets.h
      echo "created src/secrets.h, now fill in WIFI_SSID and WIFI_PASS (2.4 GHz network)"
    fi
    for tool in jq curl; do
      command -v "$tool" >/dev/null || echo "missing: $tool"
    done
    [ -x "{{ pio }}" ] || command -v pio >/dev/null || echo "missing: PlatformIO (https://platformio.org)"

# install the Claude Code hooks
install: install-hooks

# add the hooks to ~/.claude/settings.json (idempotent, backs the file up first)
install-hooks:
    #!/usr/bin/env zsh
    set -e
    [ -f "{{ settings }}" ] || echo '{}' > "{{ settings }}"
    # name shown on the pet when several Macs share it: max 10 chars, no spaces
    name=$(jq -r '[.. | .command? | strings | capture("hook\\.sh \\w+ (?<n>\\S+)").n][0] // empty' "{{ settings }}")
    name=${name:-$(hostname -s)}
    name=${${name//[^A-Za-z0-9_-]/}[1,10]}
    if [ -t 0 ]; then
      vared -p 'Name for this Mac on the pet (Enter to keep): ' name
      name=${${name//[^A-Za-z0-9_-]/}[1,10]}
    fi
    cp "{{ settings }}" "{{ settings }}.bak"
    jq --arg h "{{ hook }}" --arg n "$name" --arg sl "{{ statusline }}" '
      def strip: map(select(any(.hooks[]?; .command | test("(scripts|mac)/hook\\.sh")) | not));
      def entry($s; $m): (if $m then {matcher: "*"} else {} end) + {hooks: [{type: "command", command: "\($h) \($s) \($n)"}]};
      .hooks //= {}
      | reduce (
          ["SessionStart", "idle", false],
          ["UserPromptSubmit", "thinking", false],
          ["PreToolUse", "working", true],
          ["PostToolUse", "thinking", true],
          ["Notification", "attention", false],
          ["Stop", "done", false],
          ["SessionEnd", "idle", false]
        ) as [$event, $state, $matcher]
        (.; .hooks[$event] = ((.hooks[$event] // []) | strip) + [entry($state; $matcher)])
      | .statusLine //= {type: "command", command: $sl}
    ' "{{ settings }}.bak" > "{{ settings }}"
    if ! jq -e --arg sl "{{ statusline }}" '.statusLine.command == $sl' "{{ settings }}" >/dev/null; then
      echo "you already have a status line, so usage limits won't reach the pet. Pipe its input to {{ statusline }} to fix that"
    fi
    echo "hooks installed as \"$name\" (backup: {{ settings }}.bak), start a new Claude session to pick them up"

# remove the pet hooks from ~/.claude/settings.json, keeping any others
uninstall-hooks:
    #!/bin/sh
    set -e
    cp "{{ settings }}" "{{ settings }}.bak"
    jq '
      def strip: map(select(any(.hooks[]?; .command | test("(scripts|mac)/hook\\.sh")) | not));
      if .hooks then .hooks |= (map_values(strip) | with_entries(select(.value | length > 0))) else . end
      | if (.statusLine.command // "" | test("scripts/statusline\\.sh")) then del(.statusLine) else . end
      | if .hooks == {} then del(.hooks) else . end
    ' "{{ settings }}.bak" > "{{ settings }}"
    echo "hooks removed (backup: {{ settings }}.bak)"

# compile the firmware
build:
    {{ pio }} run

# compile and flash: over Wi-Fi if the pet answers, else USB (OTA_PASS env if set in secrets.h)
flash:
    #!/bin/sh
    set -e
    if curl -4 -sf -m 2 -o /dev/null "http://{{ host }}/"; then
      {{ pio }} run
      echo "flashing over Wi-Fi ({{ host }})..."
      # Origin is required by the updater's CSRF check when OTA_PASS is empty; it reports errors with HTTP 200
      out=$(curl -4 -sf -m 120 -u "pet:${OTA_PASS:-}" -H "Origin: http://{{ host }}" -F "firmware=@.pio/build/pet/firmware.bin" "http://{{ host }}/update")
      case "$out" in
        *rror*) echo "OTA failed: $out" >&2; exit 1 ;;
      esac
      echo "done, pet is rebooting"
    else
      echo "pet not reachable at {{ host }}, flashing over USB"
      {{ pio }} run -t upload
    fi

# compile and flash over USB (first flash, or when Wi-Fi is broken)
flash-usb:
    {{ pio }} run -t upload

# serial console (Ctrl+C to quit)
monitor:
    {{ pio }} device monitor

# flash, then open the serial console
dev: flash monitor

# delete build output
clean:
    {{ pio }} run -t clean

# set the pet state, e.g. `just state attention "Need you!"`
state name msg="" tool="":
    curl -4 -sf -m 2 -G "http://{{ host }}/state" \
      --data-urlencode "s={{ name }}" --data-urlencode "msg={{ msg }}" --data-urlencode "tool={{ tool }}"

# what the pet is doing right now
status:
    @curl -4 -sf -m 2 "http://{{ host }}/" || echo "pet not reachable at {{ host }}"

# walk the pet through every state, 5s each
tour:
    #!/bin/sh
    set -e
    send() {
      echo "$1 ${2:-}"
      curl -4 -sf -m 2 -G "http://{{ host }}/state" \
        --data-urlencode "s=$1" --data-urlencode "tool=${2:-}" --data-urlencode "msg=${3:-}" >/dev/null
      sleep 5
    }
    send thinking
    send working Bash
    send working Edit
    send working Read
    send working Task
    send working mcp_tool
    send attention "" "Claude needs your permission to use Bash"
    send done
    send idle
