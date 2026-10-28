# Changelog

## 1.2.0 (2026-10-28)

Fitment (stancer), camera view, merged controls page.

Licence change: from 1.2.0 the project is under the WheelPlugin Free-Use Licence (free to use, change and share; not for sale; copyright NakimiTV). Releases up to 1.1.0 stay MIT.

Added
- Fitment: per-car front / rear track width, ride height and camber (-20 to +20), saved per model + plate or per model, applied to cars
  driven or seen within about 80 m. Small left-side panel with an orbit camera (arrow keys, Page Up / Down) so the car stays in view
- Camera view settings in the *General* page: first / third person on foot and in a vehicle (set when getting in / out) and the
  third-person distance. Ini section `[Camera]`
- Gear lever position shared with other mods through the named shared memory `NakimiTV_GearState` (read by MirrorCam)
- Overlay health counters in `WheelPlugin.log` while the menu is open (script ticks, presents, empty frames, late textures)

Changed
- Gas station map icons exist only while the player is within 100 m of a station (removed beyond 110 m)
- The pages *Devices*, *Steering and pedals*, *Buttons*, *Keyboard keys* and *H-shifter gears* are one scrolling page,
  *Controls and devices*, in that order of sections (mouse wheel, Page Up / Down, or drag the scroll bar). 8 categories instead of 12
- A present with no fresh draw list repeats the last list for up to 80 ms instead of drawing nothing

Notes
- The fitment writes wheel offsets in vehicle memory (visual only). Layout taken from the open-source FiveM natives; checked on a few
  cars only. Not confirmed in the game at packaging time: the camera settings, the merged scrolling page (mouse wheel scrolling in
  particular) and the 100 m icon radius
- The ini gains `[Camera]` and `[Fitment]`; old ini files keep working (new keys use defaults)

## 1.1.0 (2026-10-26)

Simulator layer, own drawing, Thai text. Tested on GTA V Legacy 1.0.3889.0 with a Moza R3; see the status table in
`docs/USER_GUIDE.md` (section 14) for what has and has not been confirmed in the game.

Added
- Fuel system: per-vehicle tank, consumption by speed / throttle / idle, engine dies when empty, refuel at gas pumps (hold a key),
  price in baht per liter taken from story-mode cash (stops when the money runs out; free for other player models)
- One start / stop key (or wheel button): tap = engine off, hold while off = crank
- Gas pump map icons and 3D markers; pumps are discovered while driving and remembered in `WheelPlugin_pumps.txt`
- Neon dashboard: seven-segment speed (KMH / MPH), big gear number, rainbow rpm bar, glass pedal capsules (clutch, brake, throttle,
  handbrake) and a fuel capsule; refuel panel in the same style; size slider; plain HUD kept as an option
- Thai text in the whole menu, HUD and refuel panel with the embedded TH Sarabun PSK font (size and bold options)
- F11 menu redrawn as glass / neon with its own mouse pointer; 11 categories (new: "Fuel and engine", "HUD and key blocks")
- Optional blocking of the game's own F1 / F2 / F3 / F5-F8 functions (off by default; the keyboard is not touched)
- Exports `WheelPlugin_GetFuel` / `WheelPlugin_SetFuel` for other plugins
- Release tooling: `scripts/package.ps1`, `release/` templates, user guides (English and Thai), third-party notices

Changed
- All drawing (text, rectangles, shapes) goes through one Direct3D 11 overlay in Script Hook V's present callback; the game's
  fonts and `DRAW_RECT` remain only as a fallback. Batched: one buffer fill and one draw per texture run
- Default full-tank range 80 km (was 250; the map is small), selectable from 10 km
- Source split into `main.cpp`, `thai.h`, `dash.h`, `menu_style.h`, `font_data.h`; build uses `/utf-8` and links GDI+

Notes
- The ini gains `[Sim]` and `[Block]` sections and `Keys\Refuel`; old ini files keep working (new keys use defaults)
- Not yet confirmed in the game at packaging time: the neon menu, rpm bar and pedal capsules, pump icons / markers, key blocking,
  real cash deduction

## 1.0.0

First public release. Tested on GTA V Legacy 1.0.3889.0 with a Moza R3 base and an Arduino-based H-shifter.

- Direct steering input (no gamepad dead zone), fixed wheel angle for every car
- Force feedback: software friction/damping, self-aligning torque, centering, vibrations, automatic direction detection
- Manual gearbox with clutch bite point, stalling, starter, creep, rev limiter, rpm coupling; up to 10 gears
- Sequential paddle mode, analog/button handbrake, brake pedal never selects reverse
- Mouse-driven in-game menu, any device per role (auto-detects force feedback bases), configurable hotkeys
- Rolling telemetry capture for bug reports
