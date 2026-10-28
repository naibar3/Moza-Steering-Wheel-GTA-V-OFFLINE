# WheelPlugin

A Script Hook V plugin for **GTA V (Legacy, single player)** that adds proper steering-wheel support and a small simulator
layer on top: force feedback, pedals, H-pattern shifter with a real clutch, sequential paddles, handbrake, fuel with gas
pumps and prices, a neon dashboard and a glass-style in-game menu with **Thai text** (embedded font).
Written and tested with a **Moza R3** direct-drive base, but it picks any DirectInput wheel with force feedback.

> 🇹🇭 ภาษาไทย: [README.th.md](README.th.md) · Complete manual: [docs/USER_GUIDE.md](docs/USER_GUIDE.md) ·
> คู่มือฉบับสมบูรณ์: [docs/USER_GUIDE.th.md](docs/USER_GUIDE.th.md)

**Tested only on GTA V Legacy build 1.0.3889.0 (Steam).** The plugin patches/reads game memory with byte patterns,
so a game update can break parts of it until the patterns are refreshed (see [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md)).

> ⚠️ **Single player only.** Never use it in GTA Online. Run the game without BattlEye (`-nobattleye`).
> Not affiliated with Rockstar Games, Take-Two, Alexander Blade or Moza Racing. Use at your own risk.

## Features

**Wheel**
- **Steering** written straight into the vehicle (no gamepad dead zone), your wheel angle on every car.
- **Force feedback**: overall gain, internal friction, damping, self-aligning torque from the car's rotation, optional
  centering, engine resonance, terrain, bumps, collisions, gearbox grind, understeer. Friction/damping are made in
  software from the wheel's own speed (any device); the direction of a positive force is **measured automatically**.
- **Manual gearbox**: H-shifter (up to 10 gears + R), clutch with a bite point, stalling, hold-to-crank starter, creep,
  rev limiter, rpm that follows the wheels. **Sequential** paddles. **Handbrake** analog/button. The brake never selects reverse.

**Simulator layer** (every part has its own switch; off means it does not run at all)
- **Fuel**: per-vehicle tank, consumption by speed and throttle, engine dies when empty, refuel at gas pumps by holding a key,
  **price in baht per liter paid from story-mode cash**, refuelling stops when the money runs out.
- **One key** stops (tap) and starts (hold) the engine.
- **Gas pump map icons and 3D markers**, found while driving and remembered.
- **Neon dashboard**: seven-segment speed (KMH/MPH), big gear number, rainbow rpm bar, glass pedal capsules and a fuel capsule.
- **Thai everywhere**: menu, HUD, refuel panel, with the embedded TH Sarabun font. Nothing is installed on the player's PC.
- **Optional blocking** of the game's own F1 / F2 / F3 / F5-F8 functions (Rockstar Editor, character switch) so other mods can use the keys.

**Fitment and camera** (1.2.0): per-car front / rear track width, ride height and camber with a small panel and orbit camera; first / third person camera settings on foot and in a vehicle.

**Menu (F11)**: mouse sliders, typed values, "click then use the control" binding for axes, buttons and keys, device selection
per role, glass / neon style, every change saved at once. **Telemetry capture**: rolling 25 s CSV for bug reports.

Everything on screen is drawn by the plugin itself through a small Direct3D 11 overlay (the game's fonts and rectangles are only a fallback).

## Requirements

- GTA V Legacy (version above), **story mode**, DirectX 11
- [Script Hook V](http://www.dev-c.com/gtav/scripthookv/) (`ScriptHookV.dll` + `dinput8.dll`) matching your game version
- A DirectInput steering wheel (force feedback optional)

## Install (users)

1. Install Script Hook V (follow its own readme).
2. Copy `WheelPlugin.asi` and `WheelPlugin.ini` into the **GTA V game folder**, next to `GTA5.exe`
   (not the OpenIV `mods` folder). Or run `install.bat` from the release package.
3. Start the game **without BattlEye** (`-nobattleye` in `commandline.txt` or the Steam launch options).
4. Load story mode, press **F11**, bind your wheel, pedals and buttons. First-start steps, Pit House tips, every setting and
   troubleshooting are in the [user guide](docs/USER_GUIDE.md).

Default keys (all changeable in the menu): `F11` menu, `F10` cycle Auto → Manual → Sequential, `F8` engine (tap = stop,
hold = start), `J` hold at a gas pump to refuel.

## Build from source

Requirements: Visual Studio 2022/2025 or Build Tools with the **C++ x64 workload**, Windows 10/11 SDK.

1. Download the **Script Hook V SDK** from <http://www.dev-c.com/gtav/scripthookv/> (its licence does not allow
   re-hosting it here) and put `inc\` and `lib\ScriptHookV.lib` into `sdk\` (see [sdk/README.md](sdk/README.md)).
2. Run `build.bat` → `build\WheelPlugin.asi` (static runtime, no redistributable needed).
3. `scripts\deploy.ps1 -Game "<GTA V folder>" [-Launch]` builds and copies it into the game (refuses while the game runs).
4. `scripts\package.ps1` builds the two release zips (install package and source package).

Source files (`src/`): `main.cpp` (plugin: config, DirectInput, force feedback, gearbox, fuel, menu), `thai.h` (Thai strings, GDI+
text and shape renderer, Direct3D 11 overlay), `dash.h` (neon dashboard and refuel panel), `menu_style.h` (glass menu pieces),
`font_data.h` (embedded font, see `src/FONT_NOTICE.txt`). Sources are UTF-8 (`/utf-8`).

## Developing (with or without Claude Code)

- [docs/ARCHITECTURE.md](docs/ARCHITECTURE.md): source map, drawing overlay, game memory offsets, force / transmission / fuel models, porting.
- `scripts/analyze-capture.ps1 -Csv <file>`: summary of a telemetry capture.
- `.claude/skills/wheelplugin-dev/`: a [Claude Code](https://claude.com/claude-code) skill with the project rules, the dev loop
  and a symptom → cause playbook (`CLAUDE.md` points to it).
- [CHANGELOG.md](CHANGELOG.md) · [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md)

## Status

Core wheel features were used in the game during development; the fuel system, Thai text, neon speed digits and refuel panel were
seen working. The neon menu, rpm bar / pedal capsules, pump icons and markers, key blocking and real cash deduction were built
but not yet confirmed in the game when 1.1.0 was packaged. New in 1.2.0: fitment (track width, ride height, camber) seen working on a few cars; the camera view settings, the merged scrolling controls page and the 100 m gas station icons are built but not confirmed in the game (table in the user guide, section 14).

## Credits and licence

- Script Hook V © Alexander Blade (not included; download it from the link above).
- TH Sarabun PSK font: Thai national fonts by SIPA, free to use and redistribute with the notice (`src/FONT_NOTICE.txt`).
- Several byte patterns and the general approach for the gear / rpm / clutch / steering-input memory fields come from
  studying the open source [ikt32/GTAVManualTransmission](https://github.com/ikt32/GTAVManualTransmission).
  The code in this repository is an independent implementation; if you reuse parts of this project please check
  that your licence is compatible with that project's licence.
- Licence: **free to use, change and share, not for sale** (WheelPlugin Free-Use Licence, see [LICENSE](LICENSE)). Copyright (c) 2026 NakimiTV. This is source-available, not open source in the OSI sense. Releases up to 1.1.0 were MIT.
