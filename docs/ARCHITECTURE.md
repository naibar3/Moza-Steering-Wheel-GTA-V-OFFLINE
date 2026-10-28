# Architecture and porting notes

The plugin is one translation unit, `src/main.cpp`, plus four headers it includes at fixed points. It registers one
Script Hook V script (`ScriptMain`) that runs once per game frame, and one Script Hook V **present callback**
(`OnPresent`) that runs on the render thread right before the frame is shown.

## Source files

| file | what it holds |
|---|---|
| `src/main.cpp` | config, DirectInput, force feedback, memory patterns, gearbox, fuel / engine key / key blocks (`SimTick`), the F11 menu, telemetry, `ScriptMain`, `DllMain` |
| `src/thai.h` | the Thai string table (`ThTable`), GDI+ text and shape rasteriser, the texture cache, the draw list, the **Direct3D 11 overlay** (`OnPresent`), `DrawThaiStr` / `DrawGlyphStr` |
| `src/menu_style.h` | glass / neon building blocks for the menu (`NShape`, `NCursor`, `Rgb`) |
| `src/dash.h` | the neon dashboard (`DrawDashNeon`) and the refuel panel (`DashRefuelUi`) |
| `src/font_data.h` | TH Sarabun PSK regular and bold as byte arrays (`FONT_REG`, `FONT_BOLD`), generated; see `FONT_NOTICE.txt` |

Include order inside `main.cpp` matters: `thai.h` is included right before `DrawStr` (it needs `cfg`, `g_dir`, `Log`, `MAXG`,
`g_hwnd`), then the overlay-backed `DrawBox`, then `menu_style.h`; `dash.h` is included right before `DrawHud` (it needs the
`g_live*`, `g_hud*`, `g_dash*` values). All sources are UTF-8 and are built with `/utf-8`.

## Source map of `main.cpp`

Search for the `// ----------` banners.

| section | what it does |
|---|---|
| config | `Config cfg`, ini read/write (`IniStr`, `LoadConfig`, `SaveConfig`). Sections `[Devices] [Axes] [Shifter] [Clutch] [FFB] [Keys] [Sim] [Block] [Memory]`. |
| DirectInput | `DevInfo`/`g_attached` lists attached controllers, `PickDevice` resolves a role (`auto` = first FFB base, Moza preferred, or name fragments split by `;`). One `Dev` per role: wheel, pedals, handbrake, shifter. `ReopenAll` re-opens them. |
| force feedback | constant force, friction, damper conditions, 3 sine periodics (engine / road / slip). `FfbOut` is one frame, `SetFfb` sends it, `EnsurePlaying`/`ReacquireWheel` recover from `0x80040205` (exclusive access lost). |
| memory | `FindPattern`/`FindAllPatterns`, offsets for gear / rpm / clutch / steering input, runtime discovery of the steering field, clutch patch. |
| menu | `Row` (bool, float, learn axis/button, action, choice, device, key), `BuildRows` defines the 11 categories, `MenuTick` does input and drawing with `NShape` pieces and `DrawStr`. |
| telemetry | ring buffer of per-frame samples, `RingDump` writes `WheelPlugin_capture_N.csv`. |
| HUD | `DrawHudShifter` (plain HUD), `DrawHud` (chooses neon or plain). |
| simulator | `SimTick` (called every frame before anything that can `continue`): key blocks, `StationTick` (pump discovery, blips, markers), engine key, fuel, start, refuelling, dashboard values, refuel panel. |
| main loop | `ScriptMain`: `WAIT(0)` → `ThaiFlush` → poll devices → menu → `SimTick` → `DrawHud` → FFB test / direction → controls → transmission → FFB output. |

## Drawing: everything goes through the overlay

The game's own fonts have no Thai glyphs, and anything drawn with `DRAW_RECT` / text natives is always **under** what a
Direct3D overlay draws. To get one consistent drawing order and full control, the plugin does not use the game's drawing at all
(it stays only as a fallback when the overlay fails to start):

1. **Script thread** (inside `ScriptMain`): every `DrawStr`, `DrawBox`, `NShape`, `Dash*` call turns into one or more
   `ThCmd` quads (screen pixels, uv rectangle, colour at the top and at the bottom edge) pushed to `g_thCur`.
   `ThaiFlush()` (once per script frame, right after `WAIT(0)`) swaps `g_thCur` into `g_thReady` under `g_thMx`.
2. **Render thread** (`OnPresent`, registered with `presentCallbackRegister`): copies `g_thReady` (ignored if older than 120 ms),
   creates GPU textures for new ones, fills one dynamic vertex buffer, draws one run per texture, and saves/restores the
   game's pipeline state (render target, viewport, rasteriser, blend, depth, shaders including GS/HS/DS, input assembler,
   PS resource and sampler). Shaders are compiled at runtime with `D3DCompile`. **No natives and no script-thread data are
   touched on this thread** except the immutable pixel data of finished textures.
3. **Textures** (`ThTex`) are created on the script thread with GDI+ (`ThShape`, `ThaiTexture`, `ThGlyph`) and live in
   `g_thTex`. The GPU side (`srv`) belongs to the render thread; the CPU pixels are freed after upload. Budgets limit new
   textures per frame (`g_thBudget` strings, `g_thShapeBudget` shapes, `g_thGlyphBudget` glyphs) so opening a page never stalls.
4. **Text**: ASCII strings are composed from cached per-glyph textures (`DrawGlyphStr`, advances from `ThAdvance`), so live numbers
   cost nothing to cache. Strings with Thai (from the translation table or written directly) are rendered whole with GDI+ so
   the Thai shaping (vowels, tone marks) is right (`DrawThaiStr`, `ThaiRender`). Sizes are in screen pixels
   (`scale * 0.115 * thaiSize * screen height`), glyph textures are exactly that size (1 texel = 1 pixel, no scaling blur).
5. **Shapes** (rounded body, rim with glow, rounded fill, seven-segment digits, glowing text, stacked labels, cursor) are drawn
   white by GDI+ and tinted/gradient-coloured by the vertex colours; `ThFillBar` slices a rounded bar into three quads so the
   caps stay round at any height.
6. The game's mouse pointer would be hidden under the menu, so the menu draws its own (`NCursor`).

Rules: draw only through `DrawStr` / `DrawBox` / `NShape` / the `Dash*` helpers; keep all Thai strings in `ThTable` (English
source strings stay the keys); never call game natives in `OnPresent`; keep the state save/restore symmetrical.

## Fuel, engine key, pumps (`SimTick`)

- `g_fuel` maps a vehicle handle to the tank level (0..1). Consumption per second = `dt * (1.5 + speed * (0.5 + 1.5 * throttle)) /
  (FuelRangeKm * 1000)`, only while the engine runs. Empty → `SET_VEHICLE_ENGINE_ON(false)` and `g_noFuel` blocks every start path
  (also the starter in the gearbox block).
- Only cars, bikes and quad bikes driven by the player are handled.
- One key / button for the engine: a rising edge while running stops it (`g_startSuppress` keeps the held key from cranking again);
  holding while stopped cranks for `CrankTime`. In manual / sequential mode the cranking lives in the gearbox block (the starter
  only jerks the car in gear); `SimTick` cranks only when `ShifterActive()` is false.
- Refuelling: speed < 1 m/s, engine off, a pump model within 6 m (`OBJECT::GET_CLOSEST_OBJECT_OF_TYPE`, models in `InitPumps`), key held.
  Liters = tank fraction × `TankLiters`; cost = liters × `FuelPriceBaht`; the whole-baht part is taken from the story cash
  stat of the current character (`SP0/1/2_TOTAL_CASH` for `player_zero/one/two`; other models are free). If the cash is not enough
  the liters are limited (`g_brokeStop`).
- Pumps: `StationTick` scans `worldGetAllObjects` once a second, remembers new pump positions (≥1.5 m apart) in
  `WheelPlugin_pumps.txt`, creates one blip (sprite 361) per 40 m cluster and draws markers (`DRAW_MARKER` 1 and 2) near the player.
- Key blocks: `DISABLE_CONTROL_ACTION` for control ids F1 = 288, F2 = 289, F3 = 170, F5..F8 = 166..169, plus
  `REPLAY_PREVENT_RECORDING_THIS_FRAME` when F1/F2 are blocked. Only called while the switch is on.
- Exports for other plugins: `WheelPlugin_GetFuel(int vehicle)` (−1 when unknown) and `WheelPlugin_SetFuel(int vehicle, float level)`.

## Game memory (build 1.0.3889.0)

All fields are reached through `getScriptHandleBaseAddress(vehicle)` (Script Hook V) plus an offset.

| field | offset | how it is found |
|---|---|---|
| next gear (u16) | `0x880` | pattern `48 8D 8F ?? ?? ?? ?? 4C 8B C3 F3 0F 11 7C 24`, imm32 at +3 |
| current gear (u16) | next + 2 | |
| top gear (u8) | next + 6 | |
| engine rpm (float 0..1) | `0x8C8` | pattern `76 03 0F 28 F0 F3 44 0F 10 93`, imm32 at +10 |
| clutch (float, 1 = engaged, <0 disengaged) | rpm + `0xC` | |
| throttle (float, sound) | rpm + `0x10` | |
| steering input (float, +1 = full left) | `0x9D4` | old-build pattern does not match 3889: found **at runtime** (below), cached in `[Memory]` with the exe timestamp |

Gears: gear 0 is reverse; the game keeps its own auto shifting running, the plugin rewrites next/current gear every frame.

### Steering input discovery (`DiscoverSteer`)

While the game steers from the plugin's control input, the vehicle's steering-input float follows that command. The plugin
watches every float in `+0x700 .. +0xD00` for ~360 frames and keeps the one with correlation > 0.96 and the widest range
(the input spans −1..1, the steering angle field only its lock). The sign comes from the correlation sign.

### Clutch patch (`SetClutchPatch`)

The game's gearbox code contains several instructions `mov dword [rbx+0x54], 0x3DCCCCCD` (clutch = 0.1f): when the revs are low,
and at the shift-up point near the rev limit. With a manual gearbox they trap the engine at idle or cut the drive at ~70% rpm.
The plugin finds all `C7 43 ?? CD CC CC 3D` sites, keeps those inside ±0x1000 bytes of the known low-rpm site (`... 3D 66`) that
write the same field (6 sites in 3889), and NOPs them (7 bytes) **only while the player drives a manual/sequential vehicle**;
they are restored when that stops (also in `DllMain` detach). This is the riskiest part of the plugin: it patches the game's code
in memory. Patterns that do not match are only logged.

## Force model (`ScriptMain`, "FFB" block)

- self-aligning torque: `sat = clamp(speed*yaw/9 + 0.25*yaw)` (lateral acceleration plus pure rotation), low-passed.
- optional position centering scaled by speed (zero while parked).
- software friction `tanh(v/v0)` and damping `v` from the wheel's own speed (axis units/s). `v0` grows with the friction setting:
  a narrow ramp makes the loop buzz on a direct-drive base.
- everything is in axis coordinates and multiplied by `ffbHwSign`, the measured direction of a positive DirectInput force. It is
  measured by pushing +x then −x; accepted only if the wheel moved clearly both ways and no command was refused. **Until it is
  known no software force is sent** (a wrong sign turns damping into anti-damping).
- shaking guard: many fast velocity sign flips while the command is large → software friction/damping paused for 3 s.
- vibrations use the three sine effects; impacts add a one-frame lateral push to the constant force.

## Transmission model

`g_trans`: 0 auto (the game's own box), 1 manual (H-pattern + clutch pedal), 2 sequential (paddles). Per frame in manual/sequential:

1. gear selection (shifter buttons / paddles), gated by the clutch (`engaged` ≠ `gear` until the pedal is pressed)
2. clutch engagement `e` from the pedal (bite zone in the middle of the travel), written to the clutch field
3. stall model: `demand = e*(1+hill)*slow` vs `capacity = 0.7/stallSens + 2*throttle`, accumulates and stalls at 1.0
4. starter: hold to crank; in gear with the clutch out the car is nudged and the engine does not catch
5. creep governor, rev limiter (fuel cut at the redline)
6. rpm coupling: while the clutch slips the engine rpm is pulled to the wheel-driven rpm (learned per gear) only when that is
   higher than what the throttle asks for (downshift)
7. reverse: the game reverses on brake input, so in R the pedals are mapped by the real direction of travel; outside R the brake
   pedal never becomes "accelerate" (handbrake holds the car at standstill)

## Building and packaging

- `build.bat`: `cl /LD /O2 /MT /EHsc /std:c++17 /utf-8 ... ScriptHookV.lib dinput8.lib dxguid.lib user32.lib gdiplus.lib`
  (`d3d11`, `d3dcompiler`, `dxgi` are linked by `#pragma comment` in `thai.h`). Static runtime, no redistributable.
- `scripts/deploy.ps1`: build + copy into the game (refuses while `GTA5.exe` runs), `-Launch` starts `PlayGTAV.exe -nobattleye`.
- `scripts/package.ps1`: builds and writes `WheelPlugin-v<version>-install.zip` (asi, ini, install / uninstall scripts, guides,
  notices) and `WheelPlugin-v<version>-source.zip` (no SDK, no build output). The version is `WP_VERSION` in `main.cpp`.
- `release/`: the install package templates (`install.bat`, `uninstall.bat`, `README.txt`).

## Porting to a new game build

1. Start the game, read `WheelPlugin.log`: lines `pattern NOT found` / `gearbox code pattern ... 0 match(es)` tell what broke.
2. Gear / rpm patterns: find the same instruction sequence in the new executable (any disassembler) and update `FindGearOffsets`,
   `FindRpmOffset`. The steering field needs no pattern (runtime discovery), delete the `[Memory]` lines to force it again.
3. Clutch patch: dump the `clutch=0.1 store at ...` log lines and check that the anchor (`... 3D 66`) still exists.
4. Use the telemetry CSV (`game_clutch_before_write`, `game_next_gear_before`, `game_cur_gear_before`) to see what the game's
   logic is fighting.
5. The overlay and the control ids for the key blocks are independent of the game build; control ids can be checked against the
   public control list.

## Ideas / known gaps

- shift-up / shift-down / throttle-lift code patterns from older builds do not match 3889 (the game's own shifting is overridden
  by rewriting the gear every frame instead)
- NPC cars are not given a gearbox while the clutch patch is active
- only one wheel base with force feedback is driven; the effects target the bound steering axis
- tested only with a Moza R3 (and an Arduino Micro H-shifter): other wheels are expected to work but are untested
- the overlay needs DirectX 11; an unsupported swap chain disables Thai text and falls back to the game's own drawing
- map blips cannot show Thai names (the game's blip labels are its own font); pumps are discovered, not listed
- the key blocks use script control disabling: handling of F1/F2 inside the game's replay system may ignore it
