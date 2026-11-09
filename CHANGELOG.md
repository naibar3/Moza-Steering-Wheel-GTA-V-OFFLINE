# Changelog

## 1.3.2 (2026-11-09)

Fixes the fitment of 1.3.1.

Fixed
- Fitment did nothing in 1.3.1: the new wheel data check compared against a position that was never copied into the tracked car, so it
  judged every car as changed and dropped it on every frame (the log showed `changed its wheel data, dropped` again and again). The
  position is copied now. Use 1.3.2 instead of 1.3.1

Notes
- 1.3.1 changes stay: frame rate fix with a Moza base, wheel data check, `WheelPlugin.prev.log`, `perf` log lines, "Throttle sensitivity"
- Not confirmed in the game at packaging time: whether the crash near the car park is gone, the direction of the fitment ride height,
  the camera view settings, the scrolling controls page and the 100 m gas station icons. Confirmed: fitment moves the wheels again

## 1.3.1 (2026-11-09)

Frame rate fix, safer fitment, better diagnostics.

Fixed
- Frame rate dropped by 20-30 fps when getting into a vehicle with a Moza base switched on. The force feedback code called the wheel
  driver for every effect on every frame (about 5.4 ms per frame). Now only the effects whose value really changed are written
  (1 % steps for resistance and vibration strength, whole Hz for frequencies, 0.3 % for the constant force) and the effect status is
  checked twice a second. Measured: 5.4 ms to 0.03 ms per frame

Changed
- The setting "Pedal dead zone comp." is now called "Throttle sensitivity" (Thai: ความไวคันเร่ง). It lifts small pedal values above the
  game's own input dead zone (throttle and brake); the ini key `PedalDeadZoneComp` is unchanged

Added
- Fitment checks the chain vehicle, wheel array, wheel records before every write and drops a car whose wheel data changed (a garage mod
  can rebuild or remove cars at any time). Added after a game crash near a car park; the cause of that crash is not confirmed
- The log of the previous run is kept as `WheelPlugin.prev.log`
- `perf` lines in `WheelPlugin.log` (every 5 s): average and worst ms per frame of the menu, gas stations, fitment, main loop, HUD,
  device polling, wheel / force feedback and the overlay

Notes
- Not confirmed in the game at packaging time: the wheel data check (not run in the game yet) and whether the crash near the car park is gone

## 1.3.0 (2026-10-28)

Fitment ride height rewritten.

Changed
- Fitment ride height: front and rear are independent. The car-wide height of the game cannot differ between the axles, so it is not
  used any more; every wheel carries its own axle's slider (negative = lower). Changing a value wakes the car's physics so the body
  follows at once (before, a parked car only moved after pressing the throttle)
- Overlay health counters and the fitment height line in `WheelPlugin.log` (diagnostics)

Notes
- Not confirmed in the game at packaging time: the direction of the ride height after its last correction (negative slider = lower for
  both axles) and the immediate update of the body. The camera settings, the scrolling controls page and the 100 m gas station icons
  from 1.2.0 are also still unconfirmed
- Licence unchanged (WheelPlugin Free-Use Licence, not for sale)

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
