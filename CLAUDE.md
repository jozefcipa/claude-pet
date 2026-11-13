# CLAUDE.md

This file provides guidance to Claude Code (claude.ai/code) when working with code in this repository.

## What this is

Firmware for a pixel-art desk pet on a Waveshare ESP32-C6-Touch-LCD-1.47 (Arduino on the pioarduino platform, PlatformIO), plus Mac-side scripts that drive it from Claude Code hooks over Wi-Fi. README.md has the full user-facing docs (state table, HTTP API, setup, tuning, troubleshooting).

## Commands

All tasks go through `just` (run `just` to list them):

- `just build` - compile firmware (`pio run`); this is the only "test" for C++ changes, there is no test suite or linter
- `just flash` / `just dev` - flash over USB / flash then serial monitor
- `just state <name> ["msg"] ["tool"]` - drive the pet by hand, e.g. `just state working "" Bash`
- `just tour` - cycle every state, 5 s each (visual check of all scenes)
- `just say "msg" [ms]`, `just status`
- `just usage-dry` - run `scripts/usage.sh` without sending
- `just install` / `just uninstall-hooks` - edit `~/.claude/settings.json` hooks via `jq` (idempotent, backs up first)

`src/secrets.h` (gitignored, from `secrets.example.h` via `just setup`) must exist to build. Empty `WIFI_SSID` = demo mode (`pet/demo.*` cycles scenes). Set `CLAUDE_PET_HOST` to target an IP instead of `claude-pet.local`. Always use `curl -4` against the board (macOS IPv6 mDNS lookup hangs ~5 s otherwise).

## Architecture

**Data flow:** Claude Code hook -> `scripts/hook.sh <state>` (extracts `tool_name`/`message` with jq, backgrounded `curl` with 1 s timeout, never blocks Claude; on `done` also runs `usage.sh`) -> HTTP on the board (`net/net.cpp`, `WebServer` on port 80, mDNS `claude-pet.local`) -> `petEvent()` in `pet/state.cpp` -> scene picked next frame. The Mac always initiates; the board only answers. Multiple Claude sessions share one pet, last event wins.

**Single-threaded loop** (`src/main.cpp`): each `loop()` runs `netLoop()` (which calls `server.handleClient()`, so HTTP handlers run synchronously inside the loop, no locking needed), touch input, `petTick()` timeouts, then redraws at ~30 fps (`FRAME_MS`). `main.cpp` only wires modules together; keep logic in the modules.

**Scene transitions** are detected by `pet.since` changing. Anything that should restart a scene must bump `pet.since`; `onSceneChange()` in `main.cpp` then resets scene memory, handles backlight/display power/Wi-Fi power save, and triggers speech. `petEvent()` deliberately keeps the same scene running when state and activity are unchanged (e.g. consecutive Edits keep stacking blocks).

**State model** (`pet/state.*`): `State` enum is the hook-level state (`idle/thinking/working/attention/done/sleep/off`); `Activity` is the sub-animation while `WORKING`, chosen from the tool name in `activityFor()`. PostToolUse -> `thinking` is debounced (`THINK_DEBOUNCE_MS`) to avoid flicker between back-to-back tools. `petTick()` handles done->idle, idle->sleep->off, and stale busy->idle (Esc interrupt skips the Stop hook).

**Rendering:** everything draws into an off-screen `Arduino_Canvas` (`gfx` in `display/`), then `displayFlush()` pushes it. `pet/scenes.*` decides bot position and pose per frame (plus idle "antics" from the `ANTICS` table), `pet/bot.*` draws the 16x10-unit sprite (`U` px per unit), `pet/props.*` draws laptop/blocks/bubbles/etc., `ui/pages.*` composes the pet page and the swipe-to usage page. Network status (connecting / no Wi-Fi) overrides the normal scene via `scenesOffline()`.

**Adding a new state or tool animation** touches: `State`/`Activity` enums and `stateName`/`parseState`/`activityFor` in `pet/state.cpp`, poses in `pet/scenes.cpp`, any props in `pet/props.cpp`, lines in `pet/speech.cpp`, and the hook mapping in the `install-hooks` recipe of the `justfile` (plus README tables).

## Hardware gotchas

- Pins in `src/config.h` come from Waveshare's demo/schematic; the wiki pin table is wrong. Don't "fix" them from the wiki.
- C6 is 2.4 GHz only. The onboard LEDs are not software-controllable.
- `lib/axs5106l/` is the vendor touch driver; avoid editing it.

## Style

C++: one-line `if` bodies always get braces; tuning constants live in `src/config.h` as `constexpr`. No em dashes anywhere.
