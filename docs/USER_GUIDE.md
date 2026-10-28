# WheelPlugin 1.2.0: user guide

Complete manual for players. Thai version: [USER_GUIDE.th.md](USER_GUIDE.th.md). Developers: see [ARCHITECTURE.md](ARCHITECTURE.md).

> **Single player only.** Never use this in GTA Online. The game has to run without BattlEye (`-nobattleye`).
> Not affiliated with Rockstar Games, Take-Two, Alexander Blade or Moza Racing. Use at your own risk.

## Contents

1. [What you get](#1-what-you-get)
2. [Requirements](#2-requirements)
3. [Install, update, uninstall](#3-install-update-uninstall)
4. [First start and wheel setup](#4-first-start-and-wheel-setup)
5. [Driving: transmission, clutch, engine](#5-driving-transmission-clutch-engine)
6. [Force feedback](#6-force-feedback)
7. [Fuel system and gas stations](#7-fuel-system-and-gas-stations)
8. [Dashboard (HUD) and Thai text](#8-dashboard-hud-and-thai-text)
9. [Blocking game keys (F1, F2, F3, F5 to F8)](#9-blocking-game-keys)
10. [The F11 menu, row by row](#10-the-f11-menu-row-by-row)
11. [Keys](#11-keys)
12. [Files and the ini reference](#12-files-and-the-ini-reference)
13. [Troubleshooting](#13-troubleshooting)
14. [What has been tested](#14-what-has-been-tested)
15. [Fitment (stancer) and camera view](#15-fitment-stancer-and-camera-view)

---

## 1. What you get

- **Wheel**: direct steering (no gamepad dead zone), your wheel angle (default 900 degrees) on every car, force feedback,
  analog pedals, analog or button handbrake.
- **Gearbox**: H-pattern shifter (up to 10 gears + reverse) with a real clutch (bite point, stalling, hold-to-crank
  starter, creep, rev limiter), or sequential paddles, or the game's own automatic.
- **Simulator extras** (each one can be switched off, and a part that is off does not run at all):
  fuel with gas pumps, a price in baht per liter paid from your story-mode cash, one key to stop and start the engine,
  map icons and 3D markers for gas pumps, a neon dashboard (speed, gear, rpm, pedals, fuel), Thai text everywhere,
  and optional blocking of the game's F1/F2/F3/F5-F8 keys so other mods can use them.
- **One menu (F11)** drawn in the same neon glass style. Every change is saved at once.
- **Telemetry capture**: press a key and the last 25 seconds are saved to a CSV for bug reports.

Everything on screen is drawn by the plugin with its own embedded font (TH Sarabun); the game's fonts are not used.

## 2. Requirements

- GTA V **Legacy**, tested on build **1.0.3889.0** (Steam), **story mode**.
- **Script Hook V** (`ScriptHookV.dll` and the ASI loader `dinput8.dll`) matching your game version:
  <http://www.dev-c.com/gtav/scripthookv/>. It is not included (its licence does not allow redistribution).
- Windows 10/11 and the game running in **DirectX 11** mode (Script Hook V reports DX10 as DX11 too).
- A DirectInput steering wheel (force feedback optional). Made and tested with a **Moza R3**; any FFB wheel base that shows up
  in Windows as a game controller should work. Pedals, handbrake and an H-shifter can be separate USB devices.

## 3. Install, update, uninstall

**Install**

1. Install Script Hook V first (follow its own readme).
2. Copy `WheelPlugin.asi` and `WheelPlugin.ini` into the **game folder** next to `GTA5.exe`.
   Not the OpenIV `mods` folder: `.asi` files only load from the game folder.
   Or run `install.bat` and give it the game folder.
3. Start the game **without BattlEye**: add `-nobattleye` to `commandline.txt` (one option per line), or to the Steam launch
   options. `install.bat` can add it for you.
4. Load story mode and press **F11**.

**Update**: close the game, replace `WheelPlugin.asi` (a running game locks the file). Keep your `WheelPlugin.ini`:
new settings are added automatically with their defaults.

**Uninstall**: run `uninstall.bat`, or delete `WheelPlugin.asi`, `WheelPlugin.ini`, `WheelPlugin.log`,
`WheelPlugin_pumps.txt` and any `WheelPlugin_capture_*.csv` from the game folder.

## 4. First start and wheel setup

1. **Hands off the wheel.** About 4 seconds after the game starts, the plugin pushes the wheel gently both ways to learn
   which way a positive force acts on your device. A message on screen says when it is done. The result is stored in
   the ini (`HwSign`, `HwSignKnown`). Until it is known no software force is sent. You can repeat it in
   *Force feedback > Detect FFB direction*.
2. Open **F11 > Steering and pedals** and bind: click a row, then turn the wheel fully right / press the pedal fully.
   Right-click a bound row to clear it. Clutch and handbrake are optional.
3. **F11 > General**: set *Pit House rotation* (or your wheel software's rotation) to the angle you configured there.
   *Steering lock* is the wheel rotation that gives full lock in the game (default 900).
4. If you use a separate shifter, pedals or handbrake: **F11 > Devices** to choose each device (the default is automatic),
   then bind gears in **H-shifter gears** and paddles in **Buttons**.
5. **Moza Pit House**: set *Wheel Spring Strength* to 0 and keep *Natural Friction / Damping* low. Otherwise the base itself
   centres or drags the wheel and the plugin's sliders feel dead.

## 5. Driving: transmission, clutch, engine

**Modes** (F11 > General > Transmission, or the cycle key `F10`): *Auto* (the game's own gearbox), *Manual* (H-shifter +
clutch pedal), *Sequential* (paddles, automatic clutch).

**Manual with the clutch pedal**

- A gear only goes in with the clutch pressed. Forcing it in without the clutch grinds (felt in the wheel).
- The clutch has a bite zone in the middle of the pedal travel. Releasing it too fast, or climbing a hill without
  throttle, stalls the engine (*Stall sensitivity* changes how easily).
- With the clutch out and no throttle the car creeps at *Creep speed* of that gear (first gear is slowest).
- The rev limiter cuts fuel at the redline. With the clutch slipping, the revs follow the wheels (downshift blips).
- In neutral or with the clutch pressed you can rev the engine freely.

**Sequential**: paddle up/down moves through `R - N - 1 ... top gear`; reverse only when (nearly) stopped.

**Handbrake**: analog lever and/or a button. **The brake pedal never selects reverse** unless you are in R.

**Engine: one key for both** (default `F8`, or a wheel button *Engine start / stop*)

- **Tap** while the engine runs: it stops.
- **Hold** while it is stopped (0.7 s by default, *Engine crank time*): it starts. In gear with the clutch out the starter
  only jerks the car and the engine does not catch.
- After the engine was stopped with the key, keep holding does not start it again: release first.
- Out of fuel: the engine cannot be started (see section 7).
- The switch *Start key also stops engine* turns the stop part off (the key then only starts).

## 6. Force feedback

Defaults work for most bases. Everything is in **F11 > Force feedback** and **FFB effects**:

| setting | what it does | tip |
|---|---|---|
| Overall gain | scales every force | 100% is a good start; raise if the wheel feels weak |
| Internal friction | software friction from the wheel's own speed (weight of the wheel) | 20-40%; too high makes the wheel buzz |
| Damping | software damper | 10-30% |
| Self-aligning | torque from the car's rotation (lateral acceleration, slides) | the main steering feel |
| Centering low / high speed | optional spring towards the centre, scaled by speed, zero when parked | 0 by default |
| Engine resonance, Terrain, Bumps, Collisions, Gearbox grind, Understeer | vibrations and jolts | turn down what you do not like |
| FFB direction flip | inverts all forces | only if the direction test cannot be trusted |
| Also use device friction/damper | adds the device's own condition effects | off by default |

*Test force feedback* sends strong pulses for 2.5 seconds. If you feel nothing, see section 13.

## 7. Fuel system and gas stations

Switch: **F11 > Fuel and engine > Fuel system**. Off = no consumption, no gauge, no markers, nothing runs.

- **Tank**: every car, bike and quad bike you drive gets its own tank (boats, planes and bicycles do not). A vehicle seen for
  the first time starts at *Tank level of a new vehicle* (65%).
- **Consumption**: grows with speed and with throttle, plus a small idle consumption while the engine runs.
  *Full tank range (km)* (default **80**) is how far a full tank lasts at moderate use. Lower it for faster consumption
  (10 to 1000). The real map is small, so a realistic range would never run dry.
- **Empty**: the engine dies and cannot be started. After refuelling, hold the start key to crank.
- **Refuel**: stop within about 6 m of a gas pump, **stop the engine**, then **hold `J`** (or the wheel button *Refuel
  button*). The panel shows the percentage, liters, cost and your money. Speed: *Refuel speed* (4% of the tank per second).
  The refuel key also blocks "exit vehicle" while you hold it.
- **Price**: *Fuel price (baht per liter)* (default 40) x liters (*Tank size*, default 60 L). The amount is taken from the
  **story-mode cash** of the current character (Michael, Franklin or Trevor); baht and game dollars are 1:1.
  If the money runs out you only get what you can pay for, and the panel says so.
  For any other player model the plugin cannot read cash: refuelling is free. *Charge money for fuel* off = always free.
- **Map icons and markers**: the plugin finds gas pumps among the objects the game has loaded around you and remembers them in
  `WheelPlugin_pumps.txt`, so the map fills up as you drive. One green gas icon per station on the map (named "Gas station":
  the map's own labels cannot show Thai), a ring on each pump within 45 m and a bobbing arrow above the nearest pump
  within 70 m (orange when the tank is below 25%). Switches: *Gas station map icons*, *Gas pump markers*.
  To forget the pumps, close the game and delete `WheelPlugin_pumps.txt`.
- Other plugins can read and set a vehicle's tank through the exported functions `WheelPlugin_GetFuel(int vehicle)` and
  `WheelPlugin_SetFuel(int vehicle, float level)` (for example a garage mod that saves parked cars).

## 8. Dashboard (HUD) and Thai text

Bottom right of the screen while you drive (**F11 > HUD and key blocks**):

- **Speed**: three seven-segment neon digits with ghost segments (magenta / cyan-green / yellow-orange), unit **KMH** or MPH
  (*Speed unit*).
- **Gear**: a big seven-segment number, N and R as glowing letters (manual and sequential modes).
- **RPM bar**: rainbow gradient with a knob at the current rpm and a mark at the redline (manual and sequential).
- **Pedal capsules** (wheel connected): CLUTCH, BRAKE, THROTTLE, HANDBRAKE, and a **FUEL** capsule with the percentage
  above it (red and blinking when the tank is nearly empty).
- **Status** above the speed: "engine off", "no fuel", "press the clutch".
- *Dashboard size* scales everything (50% to 200%). *Neon dashboard style* off gives the plain HUD.
- *Speed number on HUD* and *Fuel number on HUD* hide the speed digits / the fuel capsule.

**Thai text**: the whole menu, HUD and refuel panel have Thai text. *Thai text* off gives English. *Thai text size* (50%-200%)
and *Thai text bold* change the look. The font is **TH Sarabun** embedded in the plugin: nothing is installed on your PC.

## 9. Blocking game keys

Some game functions sit on keys you may want for other mods or that you press by accident (F8 is the game's
"multiplayer character" control, and GTA Online must not be used with mods). **F11 > HUD and key blocks** has one switch each, all **off** by default:

| switch | key | game function |
|---|---|---|
| Block F1 | F1 | Rockstar Editor recording |
| Block F2 | F2 | Rockstar Editor recording (second) |
| Block F3 | F3 | save replay clip |
| Block F5-F8 | F5, F6, F7, F8 | switch character / go online |

A block only disables **the game's own function** of the key (it calls the game's control-disable for that key). It does not
read or swallow the keyboard, so other mods that read the key directly still see it. A mod that reads the key through the
game's own control system would be blocked while the switch is on. F9 and F10 (drop weapon / ammo, on foot only) are left alone.
The plugin's own hotkeys read the keyboard directly, so they work even when the game function is blocked.

## 10. The F11 menu, row by row

Mouse only. Drag a slider or click the value box and type a number (Enter). Click a bind row, then use the control; right-click
clears. Esc cancels a bind or a typed value.

| tab | rows |
|---|---|
| **General** | Transmission; Steering lock (deg); Pit House rotation (deg); Direct steering (no dead zone); Recalibrate steering offset; Steering dead zone comp.; Pedal dead zone comp.; Steering non-linearity; Steering deadzone |
| **Force feedback** | Force feedback on/off; Test force feedback; Detect FFB direction; Also use device friction/damper; Overall gain; Internal friction; Damping; Self-aligning; Centering at low speed; Centering at high speed; FFB direction flip |
| **FFB effects** | Engine resonance; Terrain surface; Bumps; Collisions; Gearbox grind; Understeer slip |
| **Steering and pedals** | Steering wheel; Throttle pedal; Brake pedal; Clutch pedal; Handbrake lever (analog) |
| **Clutch and engine** | Creep speed 1st gear (km/h); Creep speed gear step (x); Stall sensitivity; Engine crank time (s) |
| **Buttons** | Paddle shift up / down; Handbrake button; Transmission mode button; Engine start / stop button; Mark problem (save capture); Refuel button (hold) |
| **H-shifter gears** | Gear 1 to 10 and Reverse: move the shifter into the gear, then click |
| **Devices** | Wheel base (FFB); Pedals; Handbrake device; H-shifter (click to cycle through the attached controllers) |
| **Keyboard keys** | Open this menu; Cycle transmission mode; Engine start / stop; Save telemetry capture; Dump axes to log; Refuel |
| **Fuel and engine** | Fuel system; Full tank range (km); Tank level of a new vehicle; Refuel speed; Start key also stops engine; Gas station map icons; Gas pump markers; Charge money for fuel; Fuel price; Tank size |
| **HUD and key blocks** | Speed number on HUD; Fuel number on HUD; Speed unit; Thai text; Thai text size; Thai text bold; Neon dashboard style; Dashboard size; Block F1; Block F2; Block F3; Block F5-F8 |

The footer shows live values (steering, pedals, gear), the raw axes of the pedal device, and the force feedback status
(`FFB link: ok / LOST`, the force the game asks for, and what is sent to the wheel).

## 11. Keys

| default key | action | changeable in |
|---|---|---|
| F11 | open / close the menu | Keyboard keys |
| F10 | cycle Auto, Manual, Sequential | Keyboard keys |
| F8 | engine: tap = stop, hold = start | Keyboard keys, Buttons |
| J (hold) | refuel at a gas pump | Keyboard keys, Buttons |
| (none) | save telemetry capture, dump axes to log | Keyboard keys |

Keys the game itself does not use (free for your own bindings): F4, F12, Numpad 0-3, I, J, O, End. The default plugin keys were
chosen so they do not collide with the game on foot or in a car.

## 12. Files and the ini reference

Created next to `GTA5.exe`:

| file | purpose |
|---|---|
| `WheelPlugin.asi` | the plugin |
| `WheelPlugin.ini` | all settings; the menu writes it |
| `WheelPlugin.log` | log of the current session (overwritten at start; attach it to bug reports) |
| `WheelPlugin_pumps.txt` | gas pumps found so far (x y z per line) |
| `WheelPlugin_capture_N.csv` | telemetry saved with the capture key (deleted on the next start) |

`WheelPlugin.ini` sections (all values are written with their meaning in the shipped file's comments):

| section | keys |
|---|---|
| `[Devices]` | Wheel, Pedals, Handbrake, Shifter (`auto` / empty / part of the device name) |
| `[Axes]` | Steer, Throttle, Brake, Clutch, Handbrake (X Y Z RX RY RZ S0 S1), Invert*, WheelRange, SteerLock, Deadzone, NonLinearity, DirectSteering, DeadZoneComp, PedalDeadZoneComp |
| `[Shifter]` | Enabled, Mode, Gear1..Gear10, GearR, PaddleUp, PaddleDown, HandbrakeButton, ToggleButton, StartButton, MarkButton |
| `[Clutch]` | CreepKmh, GearSpacing, StallSensitivity, CrankTime |
| `[FFB]` | Enabled, Invert, Gain, Weight, Damping, Lateral, CenteringLow/High, EngineResonance, Terrain, Bumps, Collisions, GearboxGrind, Understeer, HardwareConditions, HwSign, HwSignKnown |
| `[Keys]` | Menu, Transmission, EngineStart, SaveCapture, DumpAxes, Refuel (virtual-key codes: 122 = F11, 121 = F10, 119 = F8, 74 = J) |
| `[Sim]` | Fuel, FuelRangeKm, StartFuel, RefuelRate, EngineKey, FuelPay, FuelPriceBaht, TankLiters, RefuelButton, FuelBlips, FuelMarkers, NeonHud, HudScale, Speedometer, FuelGauge, Mph, Thai, ThaiSize, ThaiBold |
| `[Block]` | F1, F2, F3, F5toF8 |
| `[Memory]` | written by the plugin: where it found the steering field (delete it to search again) |

## 13. Troubleshooting

| symptom | what to check |
|---|---|
| Plugin does not load | `ScriptHookV.dll` must match the game version (see `ScriptHookV.log`). The plugin must be next to `GTA5.exe`. Look for `WheelPlugin.log` after starting. |
| Game exits at start with an error | BattlEye is on: use `-nobattleye`. |
| `pattern NOT found` in the log | the game was updated; the plugin needs new patterns (see ARCHITECTURE.md). |
| No force | menu footer `FFB link`: LOST means another program holds the wheel exclusively (close it); the plugin retakes it. Run *Test force feedback*. In Pit House set Wheel Spring Strength to 0. |
| Wheel shakes on its own | lower *Internal friction* and *Damping*; run *Detect FFB direction* with hands off. |
| Slider "does nothing" | the base itself is adding spring / friction (Pit House). |
| No Thai text / English instead | log lines `thai text ready` and `thai overlay ready` must exist; the overlay needs DirectX 11. *Thai text* must be on. |
| Text or HUD missing completely | the game must run in DirectX 11; check the log for `thai overlay: Direct3D setup failed`, then turn *Neon dashboard style* off. |
| Fuel does not go down | only cars, bikes and quad bikes in the driver seat, with the engine running. Lower *Full tank range*. |
| Refuelling is free / no money shown | the player model is not Michael, Franklin or Trevor, or *Charge money for fuel* is off. |
| Cannot refuel | stop within 6 m of a pump, engine off, hold the key. Pumps are found only after the game has loaded them near you. |
| A blocked key still works | the function is handled by the game outside the script controls (see section 9). |
| Engine will not start | out of fuel, or in gear with the clutch out (manual mode). |
| Bug report | press the capture key (set one in *Keyboard keys*) right after the problem; attach `WheelPlugin.log` and the CSV. |

## 14. What has been tested

Honest status of version 1.2.0, tested by the author on one PC (GTA V Legacy 1.0.3889.0, Moza R3 base, Arduino H-shifter):

| area | status |
|---|---|
| steering, pedals, force feedback, direction detection, clutch, stall, starter, H-shifter, paddles, handbrake | used in the game during development |
| menu, Thai text, neon speed digits, fuel capsule, refuel panel with price and money left | seen in the game |
| neon F11 menu (glass style, own mouse pointer), gear digits, rpm bar, pedal capsules | built and installed; not yet confirmed in the game at the time of writing |
| pump map icons and 3D markers, key blocking (F1/F2/F3/F5-F8), real cash deduction | built; not confirmed in the game |
| fitment (track width, ride height, camber), small panel with orbit camera | seen working on a few cars in the game |
| camera view settings, merged scrolling controls page, 100 m gas station icons | built and installed; not yet confirmed in the game at the time of writing |
| other wheel brands, other game builds | not tested |

If something does not match this table on your PC, please report it with `WheelPlugin.log`.

## 15. Fitment (stancer) and camera view

**Fitment** (menu category *Fitment*, off by default): per car, front and rear track width, ride height and camber, each from -20 to +20.
The values are saved per car (model + license plate) or per model, and applied automatically when a car with saved values is driven or
seen within about 80 m. *Adjust with the car in view* opens a small panel on the left; the arrow keys turn a camera around the car and
Page Up / Page Down zoom. *Reset fitment of this car* clears it, and switching *Fitment* off puts the wheels back.
- The wheels are moved by writing the wheel offsets in the vehicle's memory (visual only; the handling is not changed). This was found with the help of
  the open-source FiveM natives for wheel data and has been tested on a few cars of the author's game only; wheels can look wrong on cars
  with unusual wheel setups. Restart the game to undo anything odd.
- The game keeps one ride height per car. Front and rear sliders are mixed into one height plus a small offset per axle, so a big
  difference between front and rear looks slightly off.
- *Invert camber direction* flips the sign of the camber if it tilts the wrong way on your car.

**Camera view** (menu category *General*, last three rows): *Camera on foot* and *Camera in vehicle* can force first or third person when
you get out / in (and once at game start); *Third-person distance* is close, medium or far. "Unchanged" leaves the game alone. The
view is set only for the first 1.5 s after the change, so the game's own camera key still works.

**Gas station icons** are shown on the map only while you are within 100 m of the station (hidden again beyond 110 m).

The *Devices*, *Steering and pedals*, *Buttons*, *Keyboard keys* and *H-shifter gears* pages are now one scrolling page,
*Controls and devices* (mouse wheel, Page Up / Page Down or drag the bar at the right).
