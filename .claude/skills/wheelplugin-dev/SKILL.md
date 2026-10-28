---
name: wheelplugin-dev
description: Develop, debug, build, deploy and package WheelPlugin, a Script Hook V plugin for GTA V (single player) that adds steering wheel force feedback, pedals, H-pattern shifter with clutch, sequential paddles, handbrake, a fuel system with gas pumps and prices, a neon dashboard, Thai text and a glass-style in-game mouse menu drawn by its own Direct3D 11 overlay. Use when working in this repository, when the user reports wheel / force feedback / clutch / gearbox / fuel / HUD / menu / Thai text problems in GTA V, mentions WheelPlugin.log or a WheelPlugin_capture CSV, Moza Pit House, Script Hook V, or asks to port or package the plugin.
---

# WheelPlugin development

C++17 plugin built into `WheelPlugin.asi`: `src/main.cpp` plus `thai.h`, `dash.h`, `menu_style.h`, `font_data.h`. Read
`docs/ARCHITECTURE.md` first: source map, the drawing overlay, game memory offsets (build 1.0.3889.0), force, transmission and fuel
models. Player-facing behaviour is in `docs/USER_GUIDE.md` (and `.th.md`).

## Ground rules

- **Single player only.** Never help use or test this in GTA Online. The game runs with `-nobattleye`.
- **The Script Hook V SDK and runtime are not in the repo and must never be committed or redistributed** (their licence forbids it).
  `sdk/README.md` says where to put them. `.gitignore` excludes `sdk/inc`, `sdk/lib`, `build/`.
- You usually **cannot test in the game yourself.** Never say a change "works" or "fixes" something unless the user or a log /
  capture / screenshot from the game shows it. Say "built, not tested in game" and say exactly what to check. Keep the status
  table in `docs/USER_GUIDE.md` section 14 honest when you change it.
- Report what is installed truthfully: after copying, compare the timestamp of the `.asi` in the game folder with the build, and
  say whether the game was running (a running game locks the file and the copy fails).
- Keep replies in the user's language. Be explicit about what is unverified (patterns, offsets, force model guesses, layouts).
- Do not edit files in the user's game / server folders other than the plugin's own files unless told.

## Dev loop

1. Edit the sources. They are UTF-8 (Thai text), built with `/utf-8`. `main.cpp` uses CRLF: scripted multi-line replaces must
   normalise `\r\n` first and convert back (print `ok:` / `MISSING:` per replacement; a miss must stop the script). A short literal
   can be a substring of a longer line with more indentation: match from the line start.
2. `build.bat` → `build\WheelPlugin.asi` (VS Build Tools C++ x64 + the SDK in `sdk\`).
3. `scripts/deploy.ps1 -Game "<GTA V folder>" [-Launch]` builds, refuses if `GTA5.exe` is running, copies the `.asi`, optionally
   starts `PlayGTAV.exe -nobattleye`.
4. Ask the user to reproduce (screenshots are the fastest feedback for UI), then read `WheelPlugin.log` (game folder) and, if they
   saved one, the capture CSV: `scripts/analyze-capture.ps1 -Csv <file>`. The log is overwritten on every start.
5. Fix, rebuild, repeat. One change per iteration when the cause is not proven.
6. Release: bump `WP_VERSION` in `main.cpp`, update `CHANGELOG.md`, the READMEs, the user guides, `config/WheelPlugin.ini`, then
   `scripts/package.ps1`.

## Where the truth is

| question | look at |
|---|---|
| did the plugin load / find its offsets | `WheelPlugin.log` (`gear offsets`, `rpm offset`, `steering input offset`, `clutch patch ...`) |
| did the overlay start | log lines `thai text ready`, `thai overlay ready`; failures: `thai overlay: Direct3D setup failed`, `thai text: could not render` |
| what did the game do to the car | capture CSV columns `game_clutch_before_write`, `game_next_gear_before`, `game_cur_gear_before` (read before the plugin overwrites them) |
| is a force reaching the wheel | menu footer `FFB link: ok / LOST`, `game force`, `friction/damping`, `sent to wheel`; log `DirectInput ... failed: 0x80040205` |
| what the user's wheel software does | ask for a screenshot of Moza Pit House (spring, damping, friction, inertia, hands-off protection) |

## Things that bit this project (do not repeat)

- **A software-made force needs the real direction of a positive DirectInput force.** With the wrong sign damping becomes
  anti-damping and the wheel shakes. The direction is measured and no software force is sent until it is known. Do not remove that gate.
- Friction made from the wheel velocity must ramp over a wide speed range; a narrow `tanh` buzzes on a direct-drive base.
- DirectInput exclusive access is lost repeatedly (`0x80040205`); re-acquire and restart effects, always check HRESULTs.
- The wheel base can centre or drag by itself (Pit House). If a slider "does nothing", rule that out first.
- The game's gearbox writes `clutch = 0.1f` in several places. Patch only the gearbox function's stores, never "all" matches, and
  only while a manual car is driven.
- Stall / hill terms must only matter below the creep speed of the gear.
- Settings must be saved as soon as they change, not when the menu closes.
- **Drawing**: anything drawn with `DRAW_RECT` or text natives sits *under* the Direct3D overlay. Draw only through `DrawStr` /
  `DrawBox` / `NShape` / `Dash*`; mixing the two hides one of them. Do not use `drawTexture` (it rescales and blurs: tried, rejected).
  GDI+ text needs 1 texel = 1 pixel and grid-fitted anti-aliasing; render at the size you draw.
- The render thread (`OnPresent`) must not call natives or touch script-thread state; it restores every piece of pipeline state it sets.
- Windows headers define `near` / `far` / `min` / `max`: do not name variables `near` (`NOMINMAX` is set; `using std::min/max` is
  declared before `<gdiplus.h>`).
- Do not put Thai strings inside PowerShell scripts (encoding): add them to `ThTable` in `thai.h` with the Edit tool.
- A key block (`DISABLE_CONTROL_ACTION`) must not be called while its switch is off (other mods may use the key).
- When an `Edit` fails with "file has not been read", read the file again: another session may have changed it. Do not overwrite
  other sessions' changes (for example the `WheelPlugin_GetFuel` / `SetFuel` exports used by a garage plugin).

More symptom → cause → fix entries: `references/debug-playbook.md`.

## Adding a feature

- Config key: `Config`, `LoadConfig`, `SaveConfig`, the default `config/WheelPlugin.ini`, a menu `Row` in `BuildRows` (keep each
  category short, ≈11 rows), and a Thai string for each new label in `ThTable`. Every feature gets its own on/off switch, and an
  off switch must mean the code does not run at all.
- Hotkeys go through `cfg.key*` and the *Keyboard keys* menu, never hard-coded. Pick keys the game does not use (see the user guide).
- Anything that patches game memory must log what it found and refuse to patch on a wrong match count.
- Update `CHANGELOG.md`, the READMEs, both user guides and `docs/ARCHITECTURE.md` when behaviour changes.

## Porting to a new game build

Follow "Porting to a new game build" in `docs/ARCHITECTURE.md`. Typical order: new Script Hook V first, start the game, read the log
for `NOT found` lines, refresh the gear / rpm patterns, delete the `[Memory]` block of the ini so the steering field is rediscovered,
check the clutch=0.1 store group, then validate with a capture.
