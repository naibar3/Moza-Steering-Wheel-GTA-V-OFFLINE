# Debug playbook: symptom → likely cause → what to check / change

Each row was an actual problem seen with this plugin. "Check" is read-only, "Fix" is a code or settings change.

## Steering

| symptom | likely cause | check / fix |
|---|---|---|
| must turn ~half the way before the car reacts, although the menu value moves at once | the game filters control input (~25% analog dead zone) | log `steering input offset=...` present? Direct steering writes the steering float; otherwise `DeadZoneComp` lifts small inputs |
| `steering input pattern NOT found` | old-build pattern does not match | the plugin discovers the field at runtime (needs ~6 s of left/right driving); result is cached in `[Memory]`; menu → General → *Recalibrate steering offset* |
| steering reversed in game | steering sign of the discovered field | `SteerSign` in `[Memory]`, or *Steering wheel* bind direction (turn fully RIGHT when asked) |

## Force feedback

| symptom | likely cause | check / fix |
|---|---|---|
| wheel shakes by itself right after entering a car | software force with the wrong sign (direction unknown or wrongly measured) | log `ffb direction test`; ini `HwSignKnown`; keep the "no software force until known" gate |
| internal friction / damping makes it buzz | loop gain too high around standstill (narrow friction ramp, velocity noise) | widen `fv0`, filter velocity (axis resolution is 1/1000 per frame), keep gains low |
| no slider has any effect | forces refused (`0x80040205`), or the base overrides them | menu footer `FFB link`; log `re-acquiring`; Pit House spring/damping/inertia; try *Run test* (strong pulses, works on foot) |
| wheel slowly returns to centre while parked, whatever the slider | Pit House *Wheel Spring Strength*, or DirectInput autocenter | log `autocenter off (...) hr=`; ask for a Pit House screenshot |
| forces vanish after a while | exclusive access lost, effects stopped | `EnsurePlaying` restarts them; check the re-acquire period and HRESULTs |
| "Hands-Off Protection" mutes forces | base cuts torque when it thinks nobody holds the wheel | Pit House setting; mention when measuring the direction |

## Gearbox and clutch

| symptom | likely cause | check / fix |
|---|---|---|
| rpm stuck at idle in a gear at full throttle | game sets clutch to 0.1 every frame (low-rpm rule) | capture: `game_clutch_before_write = 0.100`, `game_next_gear_before` ≠ gear; log `clutch patch applied at N place(s)` |
| rpm drops to the middle when touching the redline | the game's shift-up rule releases the clutch | same patch group; plus the plugin's rev limiter (fuel cut) |
| plugin patches nothing | match count rule too strict / too loose | log `clutch=0.1 store at ...` lines; only the stores near the low-rpm anchor, same displacement byte, belong to the gearbox |
| engine stalls at speed in 1st | hill/stall demand applied above the creep speed | `demand = e*(1+hill)*slow`, `slow` = 0 above creep speed |
| revs sag when slipping the clutch with throttle | rpm coupling pulled the engine down | target = throttle rpm unless the wheels are faster (downshift) |
| brake pedal drives the car forward in R | pedals swapped blindly | map by real direction of travel (`rv.y`), handbrake holds at standstill |
| starting in gear | by design: the car is nudged and the engine does not catch | hold-to-crank, `CrankTime` |

## Menu

| symptom | likely cause | check / fix |
|---|---|---|
| button text unreadable | dark text + the game's black text outline | white text, lighter button fills |
| list runs off screen | too many rows in one category | split categories (≈11 rows max) |
| mouse does not move the cursor | game controls not disabled while the menu is open | `DISABLE_ALL_CONTROL_ACTIONS`, `SET_MOUSE_CURSOR_THIS_FRAME` |
| a change is lost after quitting | saved only on close | autosave on mouse release / Enter / mode change |

## Environment

| symptom | likely cause | check / fix |
|---|---|---|
| game crashes at start with `0xc000009a` under BattlEye | mods and BattlEye do not mix | start with `-nobattleye` (Steam launch options or `PlayGTAV.exe -nobattleye`) |
| cannot overwrite `WheelPlugin.asi` | the game is running and locks it | close the game first; the deploy script checks this |
| plugin does nothing | Script Hook V version does not match the game | `ScriptHookV.log`, `asiloader.log` |

## Overlay, Thai text, dashboard, menu style (1.1.0)

| symptom | likely cause | check / fix |
|---|---|---|
| no Thai text, English shown | overlay did not start or `Thai` off | log `thai text ready` / `thai overlay ready`; `thai overlay: Direct3D setup failed` = unsupported swap chain, game not in DX11 |
| text blurry, bold or misplaced | texture scaled, or drawn at a fractional pixel | draw with the overlay (1 texel = 1 pixel, positions rounded); never `drawTexture` |
| text or shapes hidden under a panel | something drawn with `DRAW_RECT` / text natives | route it through `DrawStr` / `DrawBox` / `NShape` (the overlay always draws on top of natives) |
| elements pop in for a few frames after opening a page | per-frame texture budgets | by design (`g_thBudget`, `g_thShapeBudget`, `g_thGlyphBudget`); raise them if it is too slow |
| game crash or flicker when the overlay draws | pipeline state not restored, or a native called in `OnPresent` | keep the save / restore symmetric; no natives on the render thread |
| mouse pointer invisible in the menu | the game's pointer is under the overlay panel | the menu draws its own (`NCursor`) |
| Thai tone marks collide | text drawn per glyph | Thai strings must be rendered whole (`DrawThaiStr`); add them to `ThTable` |
| dashboard missing pedals | no wheel detected, or HUD condition | capsules need `g_wheel.ok`; gear and rpm need manual / sequential mode |

## Fuel and engine key

| symptom | likely cause | check / fix |
|---|---|---|
| fuel never drops | not driver, wrong vehicle class, or engine off | only cars, bikes, quad bikes; consumption only while the engine runs; lower `FuelRangeKm` |
| engine restarts by itself while the key is held | `g_startSuppress` not honoured by the gearbox block | the gearbox start code must skip when `g_startSuppress` |
| engine starts with no fuel | a start path ignores `g_noFuel` | every `SET_VEHICLE_ENGINE_ON(true)` must check `g_noFuel` |
| refuelling is free | player model is not `player_zero/one/two`, or `FuelPay` off | `CashStat()` returns 0 for other models |
| cannot refuel | pump model not within 6 m, speed > 1 m/s, engine on | the pump models are in `InitPumps`; add models for modded stations |
| no pump icons / markers | pumps are only found among loaded objects | drive past; `WheelPlugin_pumps.txt` lists what was found; `FuelBlips` / `FuelMarkers` switches |
| a blocked key still works | the game handles it outside script controls | `REPLAY_PREVENT_RECORDING_THIS_FRAME` is called for F1 / F2; F3 and F5-F8 only disable the control |
