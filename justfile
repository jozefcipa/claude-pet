# claude-pet tasks. Run `just` to list them.

pio := `command -v pio || echo "$HOME/.platformio/penv/bin/pio"`
host := env("CLAUDE_PET_HOST", "claude-pet.local")
hook := justfile_directory() / "scripts/hook.sh"
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
    #!/bin/sh
    set -e
    [ -f "{{ settings }}" ] || echo '{}' > "{{ settings }}"
    cp "{{ settings }}" "{{ settings }}.bak"
    jq --arg h "{{ hook }}" '
      def strip: map(select(any(.hooks[]?; .command | test("(scripts|mac)/hook\\.sh")) | not));
      def entry($s; $m): (if $m then {matcher: "*"} else {} end) + {hooks: [{type: "command", command: "\($h) \($s)"}]};
      .hooks //= {}
      | reduce (
          ["SessionStart", "idle", false],
          ["UserPromptSubmit", "thinking", false],
          ["PreToolUse", "working", true],
          ["PostToolUse", "thinking", true],
          ["Notification", "attention", false],
          ["Stop", "done", false]
        ) as [$event, $state, $matcher]
        (.; .hooks[$event] = ((.hooks[$event] // []) | strip) + [entry($state; $matcher)])
    ' "{{ settings }}.bak" > "{{ settings }}"
    echo "hooks installed (backup: {{ settings }}.bak), start a new Claude session to pick them up"

# remove the pet hooks from ~/.claude/settings.json, keeping any others
uninstall-hooks:
    #!/bin/sh
    set -e
    cp "{{ settings }}" "{{ settings }}.bak"
    jq '
      def strip: map(select(any(.hooks[]?; .command | test("(scripts|mac)/hook\\.sh")) | not));
      if .hooks then .hooks |= (map_values(strip) | with_entries(select(.value | length > 0))) else . end
      | if .hooks == {} then del(.hooks) else . end
    ' "{{ settings }}.bak" > "{{ settings }}"
    echo "hooks removed (backup: {{ settings }}.bak)"

# compile the firmware
build:
    {{ pio }} run

# compile and flash over USB
flash:
    {{ pio }} run -t upload

# serial console (Ctrl+C to quit)
monitor:
    {{ pio }} device monitor

# flash, then open the serial console
dev: flash monitor

# delete build output
clean:
    {{ pio }} run -t clean

# show a speech bubble, e.g. `just say "hello"`
say message duration="0":
    scripts/say.sh "{{ message }}" {{ duration }}

# set the pet state, e.g. `just state attention "Need you!"`
state name msg="" tool="":
    curl -4 -sf -m 2 -G "http://{{ host }}/state" \
      --data-urlencode "s={{ name }}" --data-urlencode "msg={{ msg }}" --data-urlencode "tool={{ tool }}"

# what the pet is doing right now
status:
    @curl -4 -sf -m 2 "http://{{ host }}/" || echo "pet not reachable at {{ host }}"

# push today's token usage to the pet now
usage:
    scripts/usage.sh

# print the usage numbers without sending them
usage-dry:
    scripts/usage.sh --dry

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
