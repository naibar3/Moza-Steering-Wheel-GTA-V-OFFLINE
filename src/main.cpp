// WheelPlugin - steering wheel, force feedback, pedals, H-shifter / paddles and an in-game menu for GTA V (Script Hook V plugin)
// Free-use licence (not for sale), (c) 2026 NakimiTV, see LICENSE. Needs the Script Hook V SDK in sdk\ (not included, see sdk\README.md). Single player only.
// Overview and porting notes: docs\ARCHITECTURE.md
// WheelPlugin: steering wheel (FFB), pedals and external H-shifter for GTA V via ScriptHookV.
#define DIRECTINPUT_VERSION 0x0800
#define NOMINMAX
#define _CRT_SECURE_NO_WARNINGS
#include <windows.h>
#include <dinput.h>
#include <cstdio>
#include <cstdarg>
#include <cstdint>
#include <cmath>
#include <string>
#include <vector>
#include <algorithm>
#include <map>
using std::min;     // GDI+ headers want the min / max names
using std::max;
#include <gdiplus.h>
#pragma comment(lib, "gdiplus.lib")
#include "font_data.h"
#include "inc/main.h"
#include "inc/natives.h"

#define WP_VERSION "1.3.0"
static HMODULE g_module;
static std::string g_dir;
static FILE *g_log;
static bool g_isFiveM;          // running inside FiveM (FiveM_bXXXX_GTAProcess.exe) instead of the stock game
static std::string g_exeName;

static void Log(const char *fmt, ...) {
    if (!g_log) return;
    va_list a; va_start(a, fmt);
    vfprintf(g_log, fmt, a); va_end(a);
    fputc('\n', g_log); fflush(g_log);
}

// ---------- config ----------
constexpr int MAXG = 10;   // highest gear supported (stock cars 6-7, modified gearboxes up to 10); gearButtons[MAXG] = reverse
struct Config {
    // device roles: "auto" (wheel only) picks the first force feedback base (any Moza base: R3, R5, R9, R12, R16, R21 ...);
    // otherwise part of a device name, several alternatives separated by ';'. Empty = not used / on the wheel base.
    std::string wheelName = "auto", pedalName, handName, shifterName = "Arduino";
    std::string handbrakeAxis;       // analog handbrake lever (empty = none)
    bool invertHandbrake = false;
    int handbrakeButton = -1;        // handbrake as a plain button (both can be used together)
    int paddleUp = -1, paddleDown = -1; // sequential paddle shifters (buttons on the wheel)
    std::string steerAxis = "X", throttleAxis = "Y", brakeAxis = "RZ";
    bool invertThrottle = true, invertBrake = true;
    float wheelRange = 900.0f;       // total rotation the base is set to in Pit House (degrees)
    float steerLock = 900.0f;        // total wheel rotation that gives full lock in game (degrees), same for every car
    bool steerLog = false;           // while driving, write rim angle / steering input / wheel angle to WheelPlugin_steering.csv
    float deadzone = 0.02f;
    int gearButtons[11] = {0, 1, 2, 3, 4, 5, -1, -1, -1, -1, 6}; // shifter buttons: gears 1..10 (7+ for gearbox swaps), last = R
    bool ffb = true, ffbInvert = false;
    // FFB settings modelled on the granularity of ETS2's controller feedback menu (all 0..1+ = 0..100%+).
    // Defaults keep the "real car" feel: no position spring, the wheel returns when the car itself rotates.
    float ffbHwSign = 1.0f;          // +1 / -1: which way a positive DirectInput force really pushes this wheel (auto-detected)
    bool ffbDirKnown = false;        // the direction test has run on this wheel; until then no software-made force is sent
    bool hwConditions = false;       // also send the device's own friction/damper effects (the weight is made in software by default)
    float ffbGain = 1.0f;            // overall gain
    float centerLow = 0.0f;          // centering at low speed  (position-based, ETS2 style; 0 = off)
    float centerHigh = 0.0f;         // centering at high speed
    float weight = 0.30f;            // internal friction: constant base weight
    float damping = 0.20f;           // damper: smoothness
    float lateralGain = 1.0f;        // self-aligning return force from car rotation / lateral acceleration
    float engineRes = 0.10f;         // engine resonance (vibration)
    float terrain = 0.15f;           // terrain surface vibration (rough ground)
    float bumps = 0.50f;             // bumps and curbs
    float collisions = 0.60f;        // impacts
    float grind = 0.50f;             // gearbox grind when shifting without clutch
    float understeer = 0.30f;        // vibration when the front end washes out
    float nonlin = 0.0f;             // steering non-linearity: reduces sensitivity near centre
    bool directSteer = true;         // write steering straight into the vehicle (no game dead zone / filtering)
    float deadComp = 0.25f;          // control path: lift small steering inputs above the game's ~25% analog dead zone
    float pedalComp = 0.25f;         // same for throttle and brake, which also go through game controls
    bool manualGears = true;
    std::string clutchAxis;          // empty = clutch disabled (no stalling)
    bool invertClutch = true;
    float stallSpeed = 1.7f;         // (unused, kept so old ini files still load)
    float creep1 = 2.5f;             // km/h the car crawls at with the clutch fully out, no throttle, in 1st gear
    float gearSpacing = 2.0f;        // crawl speed multiplier per higher gear (2.5, 5, 10, 20 km/h ...)
    float stallSens = 1.0f;          // how easily the engine stalls when the clutch is dropped too fast or the hill is too steep
    float crankTime = 0.7f;          // seconds the start key / button must be held to start the engine
    int startButton = -1;            // wheel button to restart a stalled engine (F8 always works)
    bool invertSteer = false;
    int startTrans = 1;              // initial transmission: 0 auto, 1 manual (H-pattern), 2 sequential (paddles)
    int toggleButton = -1;           // wheel button to cycle the transmission mode (F10 always works)
    int markButton = -1;             // wheel button: save the last ~25 s of telemetry to a CSV
    // keyboard keys (virtual-key codes, 0 = unused); all can be changed in the menu so they never clash with other mods
    int keyMenu = VK_F11, keyTrans = VK_F10, keyStart = VK_F8, keyMark = 0, keyDump = 0;
    // simulator extras: every part has its own switch, a part that is off is not run at all
    bool fuelOn = true;              // fuel is used up, empty tank = engine dead, refuel at pumps
    float fuelRange = 80.0f;         // km a full tank lasts at moderate use (the map is small, a real range would never run dry)
    float startFuel = 0.65f;         // tank level (0..1) of a vehicle the first time it is driven
    float refuelRate = 0.04f;        // tank fraction per second while refuelling
    bool engineKey = true;           // the start key / button also switches the running engine off (tap = off, hold while off = start)
    bool hudSpeedo = true, hudFuel = true, mph = false;
    bool thai = true;                // Thai text in the menu and HUD (embedded TH Sarabun font)
    float thaiSize = 1.0f;           // Thai text size multiplier
    bool thaiBold = true;            // bold Thai text (the regular face is thin at small sizes)
    bool hudNeon = true;             // neon dashboard (seven-segment speed, rpm bar, pedal capsules) instead of the plain HUD
    float hudScale = 1.0f;           // dashboard size multiplier
    bool fitOn = true;               // fitment: wheel track width and ride height (cm), see the Fitment menu
    bool fitPerPlate = true;         // fitment is saved per car (model + license plate); off = one setting for the whole model
    bool fitCamberInv = false;       // flips the direction of the camber tilt
    int camFoot = 0, camVeh = 0, camDist = 1;   // camera view applied on foot / in a vehicle: 0 = leave the game alone, 1 = first person, 2 = third person; third-person distance 0 close, 1 medium, 2 far
    bool fuelBlips = true, fuelMarkers = true;   // map icons and 3D markers at gas pumps found while driving
    bool fuelPay = true;             // refuelling costs story-mode cash (shown as baht); false = free
    float fuelPrice = 40.0f;         // baht per liter
    float tankLiters = 60.0f;        // tank size, converts the tank level into liters
    bool blockF1 = false, blockF2 = false, blockF3 = false, blockF5to8 = false;   // game keys switched off (replay recorder, character / online switch)
    int keyRefuel = 'J', refuelButton = -1;
} cfg;
static bool KeyDown(int vk) { return vk > 0 && (GetAsyncKeyState(vk) & 0x8000) != 0; }
static std::string KeyName(int vk) {
    if (vk <= 0) return "(none)";
    UINT sc = MapVirtualKeyA((UINT)vk, MAPVK_VK_TO_VSC);
    LONG lp = (LONG)(sc << 16);
    switch (vk) {   // extended keys need bit 24 for the right name
    case VK_LEFT: case VK_RIGHT: case VK_UP: case VK_DOWN: case VK_PRIOR: case VK_NEXT: case VK_END: case VK_HOME:
    case VK_INSERT: case VK_DELETE: case VK_DIVIDE: case VK_NUMLOCK: case VK_RCONTROL: case VK_RMENU: lp |= (1 << 24); break;
    }
    char buf[64] = {};
    if (GetKeyNameTextA(lp, buf, sizeof buf) > 0) return buf;
    snprintf(buf, sizeof buf, "key 0x%X", vk);
    return buf;
}
static int g_trans = 1;   // 0 auto, 1 manual (H-pattern + clutch), 2 sequential (paddles)
// fitment: values per vehicle model, in cm (front track, rear track, front height, rear height); the menu sliders edit g_fitUi
const int FIT_N = 6;   // front track (cm), rear track, front height (cm), rear height, front camber (deg), rear camber
struct FitVals { float f[FIT_N] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f}; };
static std::map<Hash, FitVals> g_fitModels;
static std::map<std::string, FitVals> g_fitPlates;   // key: model hash, '|', license plate (spaces as _)
static float g_fitUi[FIT_N];
static void FitLoad();
static void FitSave();
static void FitDump();
static void FitReset();
static bool g_fitCompact;                                   // the small fitment panel is shown instead of the main menu
static void FitCompactOpen() { g_fitCompact = true; }
static const char *TRANS_NAMES[3] = {"Auto", "Manual", "Sequential"};
static float SteerScale() { return cfg.wheelRange / std::max(cfg.steerLock, 1.0f); }

static std::string IniStr(const char *sec, const char *key, const std::string &def) {
    char buf[256];
    std::string path = g_dir + "WheelPlugin.ini";
    GetPrivateProfileStringA(sec, key, def.c_str(), buf, sizeof buf, path.c_str());
    return buf;
}
static float IniF(const char *sec, const char *key, float def) {
    return (float)atof(IniStr(sec, key, std::to_string(def)).c_str());
}
static bool IniB(const char *sec, const char *key, bool def) {
    return IniStr(sec, key, def ? "1" : "0") != "0";
}

static void LoadConfig() {
    if (g_isFiveM) cfg.keyStart = VK_END;   // F8 opens the FiveM console: use End unless the ini says otherwise
    cfg.wheelName = IniStr("Devices", "Wheel", cfg.wheelName);
    cfg.pedalName = IniStr("Devices", "Pedals", cfg.pedalName);
    cfg.shifterName = IniStr("Devices", "Shifter", cfg.shifterName);
    cfg.steerAxis = IniStr("Axes", "Steer", cfg.steerAxis);
    cfg.throttleAxis = IniStr("Axes", "Throttle", cfg.throttleAxis);
    cfg.brakeAxis = IniStr("Axes", "Brake", cfg.brakeAxis);
    cfg.invertSteer = IniB("Axes", "InvertSteer", cfg.invertSteer);
    cfg.clutchAxis = IniStr("Axes", "Clutch", cfg.clutchAxis);
    cfg.invertClutch = IniB("Axes", "InvertClutch", cfg.invertClutch);
    cfg.stallSpeed = IniF("Shifter", "StallSpeed", cfg.stallSpeed);
    cfg.creep1 = IniF("Clutch", "CreepKmh", cfg.creep1);
    cfg.gearSpacing = IniF("Clutch", "GearSpacing", cfg.gearSpacing);
    cfg.stallSens = IniF("Clutch", "StallSensitivity", cfg.stallSens);
    cfg.crankTime = IniF("Clutch", "CrankTime", cfg.crankTime);
    cfg.startButton = atoi(IniStr("Shifter", "StartButton", "-1").c_str());
    cfg.invertThrottle = IniB("Axes", "InvertThrottle", cfg.invertThrottle);
    cfg.invertBrake = IniB("Axes", "InvertBrake", cfg.invertBrake);
    cfg.wheelRange = IniF("Axes", "WheelRange", cfg.wheelRange);
    cfg.steerLock = IniF("Axes", "SteerLock", cfg.steerLock);
    cfg.steerLog = IniB("Axes", "LogSteering", cfg.steerLog);
    cfg.deadzone = IniF("Axes", "Deadzone", cfg.deadzone);
    for (int i = 0; i <= MAXG; i++)
        cfg.gearButtons[i] = atoi(IniStr("Shifter", ("Gear" + std::string(i == MAXG ? "R" : std::to_string(i + 1))).c_str(),
                                         std::to_string(cfg.gearButtons[i])).c_str());
    cfg.manualGears = IniB("Shifter", "Enabled", cfg.manualGears);
    {
        std::string m = IniStr("Shifter", "Mode", "Manual");
        cfg.startTrans = (m == "Auto") ? 0 : (m == "Sequential" ? 2 : 1);
        g_trans = cfg.startTrans;
    }
    cfg.toggleButton = atoi(IniStr("Shifter", "ToggleButton", "-1").c_str());
    cfg.markButton = atoi(IniStr("Shifter", "MarkButton", "-1").c_str());
    cfg.keyMenu = atoi(IniStr("Keys", "Menu", std::to_string(cfg.keyMenu)).c_str());
    cfg.keyTrans = atoi(IniStr("Keys", "Transmission", std::to_string(cfg.keyTrans)).c_str());
    cfg.keyStart = atoi(IniStr("Keys", "EngineStart", std::to_string(cfg.keyStart)).c_str());
    cfg.keyMark = atoi(IniStr("Keys", "SaveCapture", std::to_string(cfg.keyMark)).c_str());
    cfg.keyDump = atoi(IniStr("Keys", "DumpAxes", std::to_string(cfg.keyDump)).c_str());

    cfg.keyRefuel = atoi(IniStr("Keys", "Refuel", std::to_string(cfg.keyRefuel)).c_str());

    cfg.refuelButton = atoi(IniStr("Sim", "RefuelButton", "-1").c_str());
    cfg.fuelOn = IniB("Sim", "Fuel", cfg.fuelOn);
    cfg.fuelRange = IniF("Sim", "FuelRangeKm", cfg.fuelRange);
    cfg.startFuel = IniF("Sim", "StartFuel", cfg.startFuel);
    cfg.refuelRate = IniF("Sim", "RefuelRate", cfg.refuelRate);
    cfg.engineKey = IniB("Sim", "EngineKey", cfg.engineKey);
    cfg.hudSpeedo = IniB("Sim", "Speedometer", cfg.hudSpeedo);
    cfg.hudFuel = IniB("Sim", "FuelGauge", cfg.hudFuel);
    cfg.mph = IniB("Sim", "Mph", cfg.mph);
    cfg.thai = IniB("Sim", "Thai", cfg.thai);
    cfg.thaiSize = IniF("Sim", "ThaiSize", cfg.thaiSize);
    cfg.thaiBold = IniB("Sim", "ThaiBold", cfg.thaiBold);
    cfg.hudNeon = IniB("Sim", "NeonHud", cfg.hudNeon);
    cfg.hudScale = IniF("Sim", "HudScale", cfg.hudScale);
    cfg.fitOn = IniB("Fitment", "Enabled", cfg.fitOn);
    cfg.fitPerPlate = IniB("Fitment", "PerPlate", cfg.fitPerPlate);
    cfg.fitCamberInv = IniB("Fitment", "CamberInvert", cfg.fitCamberInv);
    cfg.camFoot = std::clamp(atoi(IniStr("Camera", "OnFoot", "0").c_str()), 0, 2);
    cfg.camVeh = std::clamp(atoi(IniStr("Camera", "InVehicle", "0").c_str()), 0, 2);
    cfg.camDist = std::clamp(atoi(IniStr("Camera", "ThirdPersonDistance", "1").c_str()), 0, 2);
    FitLoad();
    cfg.fuelBlips = IniB("Sim", "FuelBlips", cfg.fuelBlips);
    cfg.fuelMarkers = IniB("Sim", "FuelMarkers", cfg.fuelMarkers);
    cfg.fuelPay = IniB("Sim", "FuelPay", cfg.fuelPay);
    cfg.fuelPrice = IniF("Sim", "FuelPriceBaht", cfg.fuelPrice);
    cfg.tankLiters = IniF("Sim", "TankLiters", cfg.tankLiters);
    cfg.blockF1 = IniB("Block", "F1", cfg.blockF1);
    cfg.blockF2 = IniB("Block", "F2", cfg.blockF2);
    cfg.blockF3 = IniB("Block", "F3", cfg.blockF3);
    cfg.blockF5to8 = IniB("Block", "F5toF8", cfg.blockF5to8);
    if (cfg.keyMenu <= 0) cfg.keyMenu = VK_F11;   // the menu must always be reachable
    cfg.handName = IniStr("Devices", "Handbrake", cfg.handName);
    cfg.handbrakeAxis = IniStr("Axes", "Handbrake", cfg.handbrakeAxis);
    cfg.invertHandbrake = IniB("Axes", "InvertHandbrake", cfg.invertHandbrake);
    cfg.handbrakeButton = atoi(IniStr("Shifter", "HandbrakeButton", "-1").c_str());
    cfg.paddleUp = atoi(IniStr("Shifter", "PaddleUp", "-1").c_str());
    cfg.paddleDown = atoi(IniStr("Shifter", "PaddleDown", "-1").c_str());
    cfg.ffb = IniB("FFB", "Enabled", cfg.ffb);
    cfg.ffbInvert = IniB("FFB", "Invert", cfg.ffbInvert);
    cfg.ffbGain = IniF("FFB", "Gain", cfg.ffbGain);
    cfg.weight = IniF("FFB", "Weight", cfg.weight);
    cfg.damping = IniF("FFB", "Damping", cfg.damping);
    cfg.centerLow = IniF("FFB", "CenteringLow", cfg.centerLow);
    cfg.centerHigh = IniF("FFB", "CenteringHigh", cfg.centerHigh);
    cfg.engineRes = IniF("FFB", "EngineResonance", cfg.engineRes);
    cfg.terrain = IniF("FFB", "Terrain", cfg.terrain);
    cfg.bumps = IniF("FFB", "Bumps", cfg.bumps);
    cfg.collisions = IniF("FFB", "Collisions", cfg.collisions);
    cfg.grind = IniF("FFB", "GearboxGrind", cfg.grind);
    cfg.understeer = IniF("FFB", "Understeer", cfg.understeer);
    cfg.nonlin = IniF("Axes", "NonLinearity", cfg.nonlin);
    cfg.directSteer = IniB("Axes", "DirectSteering", cfg.directSteer);
    cfg.deadComp = IniF("Axes", "DeadZoneComp", cfg.deadComp);
    cfg.pedalComp = IniF("Axes", "PedalDeadZoneComp", cfg.pedalComp);
    cfg.lateralGain = IniF("FFB", "Lateral", cfg.lateralGain);
    cfg.ffbHwSign = IniF("FFB", "HwSign", 1.0f) < 0.0f ? -1.0f : 1.0f;
    cfg.ffbDirKnown = IniB("FFB", "HwSignKnown", false);
    cfg.hwConditions = IniB("FFB", "HardwareConditions", cfg.hwConditions);
    if (IniStr("FFB", "Version", "") != "2") {   // new force model: start from its defaults instead of values tuned for the old one
        cfg.ffbGain = 1.0f; cfg.lateralGain = 1.0f; cfg.weight = 0.30f; cfg.damping = 0.20f; cfg.centerLow = 0.0f; cfg.centerHigh = 0.0f;
    }
}

// ---------- DirectInput ----------
static IDirectInput8A *g_di;

struct Dev {
    IDirectInputDevice8A *dev = nullptr;
    DIJOYSTATE2 st = {};
    std::string name;
    bool ok = false;
    IDirectInputEffect *constant = nullptr, *friction = nullptr, *damper = nullptr;
    IDirectInputEffect *engine = nullptr, *road = nullptr, *slip = nullptr; // periodic (sine) vibrations
};
static Dev g_wheel, g_pedals, g_hand, g_shifter;   // steering base, pedals, handbrake lever, external H-shifter

// every game controller DirectInput reports as attached, with a flag for force feedback support
struct DevInfo { GUID guid; std::string name; bool ffb; };
static std::vector<DevInfo> g_attached;
static std::vector<GUID> g_usedGuids;      // devices already given to a role
static bool g_reopen = false;              // the menu changed a device: reopen all devices
static bool g_recreateFfb = false;         // the steering axis changed: rebuild the force feedback effects

static std::string Lower(std::string s) { std::transform(s.begin(), s.end(), s.begin(), ::tolower); return s; }

static BOOL CALLBACK ListCb(const DIDEVICEINSTANCEA *inst, void *) {
    g_attached.push_back({inst->guidInstance, inst->tszProductName, false});
    return DIENUM_CONTINUE;
}
static BOOL CALLBACK ListFfbCb(const DIDEVICEINSTANCEA *inst, void *) {
    for (auto &d : g_attached) if (IsEqualGUID(d.guid, inst->guidInstance)) d.ffb = true;
    return DIENUM_CONTINUE;
}
static void RefreshAttached() {
    g_attached.clear();
    g_di->EnumDevices(DI8DEVCLASS_GAMECTRL, ListCb, nullptr, DIEDFL_ATTACHEDONLY);
    g_di->EnumDevices(DI8DEVCLASS_GAMECTRL, ListFfbCb, nullptr, DIEDFL_ATTACHEDONLY | DIEDFL_FORCEFEEDBACK);
}
static bool GuidUsed(const GUID &g) { for (auto &u : g_usedGuids) if (IsEqualGUID(u, g)) return true; return false; }

// choose an attached device for a role. want = "auto" (first force feedback base, Moza preferred) or name fragments split by ';'
static int PickDevice(const std::string &wantRaw) {
    std::string want = Lower(wantRaw);
    if (want == "auto") {
        static const char *prefer[] = {"moza", "r3 base", "r5 base", "r9 base", "r12 base", "r16 base", "r21 base", "base", "wheel"};
        for (const char *p : prefer)
            for (int i = 0; i < (int)g_attached.size(); i++)
                if (g_attached[i].ffb && !GuidUsed(g_attached[i].guid) && Lower(g_attached[i].name).find(p) != std::string::npos) return i;
        for (int i = 0; i < (int)g_attached.size(); i++) if (g_attached[i].ffb && !GuidUsed(g_attached[i].guid)) return i;
        return -1;
    }
    size_t pos = 0;
    while (pos <= want.size()) {
        size_t e = want.find(';', pos);
        std::string tok = want.substr(pos, e == std::string::npos ? std::string::npos : e - pos);
        while (!tok.empty() && tok.front() == ' ') tok.erase(tok.begin());
        while (!tok.empty() && tok.back() == ' ') tok.pop_back();
        if (!tok.empty())
            for (int i = 0; i < (int)g_attached.size(); i++)
                if (!GuidUsed(g_attached[i].guid) && Lower(g_attached[i].name).find(tok) != std::string::npos) return i;
        if (e == std::string::npos) break;
        pos = e + 1;
    }
    return -1;
}

static BOOL CALLBACK AxisRangeCb(const DIDEVICEOBJECTINSTANCEA *obj, void *p) {
    auto *dev = (IDirectInputDevice8A *)p;
    DIPROPRANGE r = {};
    r.diph.dwSize = sizeof r; r.diph.dwHeaderSize = sizeof(DIPROPHEADER);
    r.diph.dwHow = DIPH_BYID; r.diph.dwObj = obj->dwType;
    r.lMin = -1000; r.lMax = 1000;
    dev->SetProperty(DIPROP_RANGE, &r.diph);
    return DIENUM_CONTINUE;
}

static void SetAutoCenterOff(Dev &d, const char *when) {
    if (!d.dev) return;
    DIPROPDWORD ac = {{sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE}, DIPROPAUTOCENTER_OFF};
    HRESULT hr = d.dev->SetProperty(DIPROP_AUTOCENTER, &ac.diph);
    static int logged = 0;
    if (logged++ < 8) Log("autocenter off (%s): hr=0x%lX", when, (unsigned long)hr);
}

static bool OpenDevice(Dev &d, const std::string &want, bool force, HWND hwnd) {
    if (want.empty()) return false;
    int idx = PickDevice(want);
    if (idx < 0) {
        std::string all;
        for (auto &a : g_attached) all += a.name + (a.ffb ? " [ffb]" : "") + " | ";
        Log("device '%s' not found. Attached: %s", want.c_str(), all.c_str());
        return false;
    }
    struct { GUID guidInstance; char tszProductName[260]; } c;
    c.guidInstance = g_attached[idx].guid;
    snprintf(c.tszProductName, sizeof c.tszProductName, "%s", g_attached[idx].name.c_str());
    if (FAILED(g_di->CreateDevice(c.guidInstance, &d.dev, nullptr))) return false;
    g_usedGuids.push_back(c.guidInstance);
    d.dev->SetDataFormat(&c_dfDIJoystick2);
    d.dev->EnumObjects([](const DIDEVICEOBJECTINSTANCEA *o, void *p) -> BOOL {
        if (o->dwType & DIDFT_AXIS) return AxisRangeCb(o, p);
        return DIENUM_CONTINUE; }, d.dev, DIDFT_AXIS);
    HRESULT hr = E_FAIL;
    if (force) hr = d.dev->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
    if (FAILED(hr)) {
        if (force) Log("exclusive access failed (0x%lX), FFB off", (unsigned long)hr);
        hr = d.dev->SetCooperativeLevel(hwnd, DISCL_NONEXCLUSIVE | DISCL_BACKGROUND);
        if (FAILED(hr)) return false;
        d.dev->Acquire();
        d.name = c.tszProductName; d.ok = true;
        Log("opened '%s' (no FFB)", d.name.c_str());
        return true;
    }
    SetAutoCenterOff(d, "before acquire");
    d.dev->Acquire();
    SetAutoCenterOff(d, "after acquire");
    d.name = c.tszProductName; d.ok = true;
    Log("opened '%s' (exclusive)", d.name.c_str());
    return true;
}

static LONG AxisOffset(const std::string &n) {
    if (n == "X") return DIJOFS_X; if (n == "Y") return DIJOFS_Y; if (n == "Z") return DIJOFS_Z;
    if (n == "RX") return DIJOFS_RX; if (n == "RY") return DIJOFS_RY; if (n == "RZ") return DIJOFS_RZ;
    if (n == "S0") return DIJOFS_SLIDER(0); if (n == "S1") return DIJOFS_SLIDER(1);
    return -1;
}
static float Axis(const Dev &d, const std::string &n) {
    LONG off = AxisOffset(n);
    if (off < 0) return 0;
    return *(const LONG *)((const BYTE *)&d.st + off) / 1000.0f;
}

static void Poll(Dev &d) {
    if (!d.ok) return;
    HRESULT hr = d.dev->Poll();
    if (FAILED(hr)) { d.dev->Acquire(); hr = d.dev->Poll(); }
    if (SUCCEEDED(hr)) d.dev->GetDeviceState(sizeof d.st, &d.st);
}

// ---------- force feedback ----------
static BOOL CALLBACK LogEffectCb(const DIEFFECTINFOA *i, void *) {
    Log("device effect: %s (type 0x%lX)", i->tszName, (unsigned long)i->dwEffType);
    return DIENUM_CONTINUE;
}
static BOOL CALLBACK LogActuatorCb(const DIDEVICEOBJECTINSTANCEA *o, void *) {
    Log("axis: %s offset=0x%lX ffActuator=%d", o->tszName, (unsigned long)o->dwOfs, (o->dwFlags & DIDOI_FFACTUATOR) ? 1 : 0);
    return DIENUM_CONTINUE;
}

static void CreateEffects(Dev &d) {
    if (!d.ok) return;
    DIDEVCAPS caps = {sizeof caps};
    if (SUCCEEDED(d.dev->GetCapabilities(&caps)))
        Log("caps: flags=0x%lX ffb=%d axes=%lu buttons=%lu ffSamplePeriod=%lu ffMinTimeRes=%lu",
            (unsigned long)caps.dwFlags, (caps.dwFlags & DIDC_FORCEFEEDBACK) ? 1 : 0, (unsigned long)caps.dwAxes,
            (unsigned long)caps.dwButtons, (unsigned long)caps.dwFFSamplePeriod, (unsigned long)caps.dwFFMinTimeResolution);
    d.dev->EnumEffects(LogEffectCb, nullptr, DIEFT_ALL);
    d.dev->EnumObjects(LogActuatorCb, nullptr, DIDFT_AXIS);
    // some drivers leave the actuators off / need a reset after acquiring; report what the device says
    HRESULT hr1 = d.dev->SendForceFeedbackCommand(DISFFC_RESET);
    HRESULT hr2 = d.dev->SendForceFeedbackCommand(DISFFC_SETACTUATORSON);
    DIPROPDWORD gain = {{sizeof(DIPROPDWORD), sizeof(DIPROPHEADER), 0, DIPH_DEVICE}, 10000};
    HRESULT hr3 = d.dev->SetProperty(DIPROP_FFGAIN, &gain.diph);
    DWORD fst = 0; HRESULT hr4 = d.dev->GetForceFeedbackState(&fst);
    Log("ffb init: reset=0x%lX actuatorsOn=0x%lX gain=0x%lX state=0x%lX(hr 0x%lX) [1=empty 2=stopped 4=paused 8=actOn 0x10=actOff 0x20=pwrOn 0x40=pwrOff]",
        (unsigned long)hr1, (unsigned long)hr2, (unsigned long)hr3, (unsigned long)fst, (unsigned long)hr4);

    DWORD axes[1] = {(DWORD)AxisOffset(cfg.steerAxis)};
    LONG dir[1] = {0};
    DICONSTANTFORCE cf = {0};
    DICONDITION cond = {0};
    DIEFFECT e = {}; e.dwSize = sizeof e;
    e.dwFlags = DIEFF_CARTESIAN | DIEFF_OBJECTOFFSETS;
    e.dwDuration = INFINITE; e.dwGain = DI_FFNOMINALMAX; e.dwTriggerButton = DIEB_NOTRIGGER;
    e.cAxes = 1; e.rgdwAxes = axes; e.rglDirection = dir;
    e.cbTypeSpecificParams = sizeof cf; e.lpvTypeSpecificParams = &cf;
    if (FAILED(d.dev->CreateEffect(GUID_ConstantForce, &e, &d.constant, nullptr))) Log("constant force effect unsupported");
    e.cbTypeSpecificParams = sizeof cond; e.lpvTypeSpecificParams = &cond;
    if (FAILED(d.dev->CreateEffect(GUID_Friction, &e, &d.friction, nullptr))) Log("friction effect unsupported");
    if (FAILED(d.dev->CreateEffect(GUID_Damper, &e, &d.damper, nullptr))) Log("damper effect unsupported");
    DIPERIODIC per = {0, 0, 0, 25000};
    e.cbTypeSpecificParams = sizeof per; e.lpvTypeSpecificParams = &per;
    if (FAILED(d.dev->CreateEffect(GUID_Sine, &e, &d.engine, nullptr))) Log("sine effect (engine) unsupported");
    if (FAILED(d.dev->CreateEffect(GUID_Sine, &e, &d.road, nullptr))) Log("sine effect (road) unsupported");
    if (FAILED(d.dev->CreateEffect(GUID_Sine, &e, &d.slip, nullptr))) Log("sine effect (slip) unsupported");
    Log("ffb effects: constant=%p friction=%p damper=%p sine=%p/%p/%p", (void *)d.constant, (void *)d.friction,
        (void *)d.damper, (void *)d.engine, (void *)d.road, (void *)d.slip);
}

// all outputs of one frame; magnitudes 0..1, frequencies in Hz
struct FfbOut {
    float constant = 0, friction = 0, damper = 0;
    float engHz = 30, engMag = 0, roadHz = 30, roadMag = 0, slipHz = 22, slipMag = 0;
};

// log a failing DirectInput call (each distinct call+result only a few times, so the log stays readable)
static bool g_ffbLost; // set when DirectInput says the wheel is no longer acquired exclusively
static ULONGLONG g_lastLostMs;                 // when the wheel last refused a force (shown in the menu)
static float g_dbgForce, g_dbgSoft, g_dbgCmd;  // last force numbers (shown in the menu)
static void Chk(const char *what, HRESULT hr) {
    if (SUCCEEDED(hr)) return;
    if (hr == (HRESULT)0x80040205 /*NOTEXCLUSIVEACQUIRED*/ || hr == DIERR_INPUTLOST || hr == DIERR_NOTACQUIRED ||
        hr == DIERR_OTHERAPPHASPRIO) { g_ffbLost = true; g_lastLostMs = GetTickCount64(); }
    static std::vector<std::pair<std::string, long>> seen;
    int n = 0;
    for (auto &s : seen) if (s.first == what && s.second == (long)hr) n++;
    if (n >= 2) return;
    seen.emplace_back(what, (long)hr);
    Log("DirectInput %s failed: 0x%lX", what, (unsigned long)hr);
}

static void SetPeriodic(IDirectInputEffect *fx, const char *name, float hz, float mag) {
    if (!fx) return;
    DIPERIODIC p = {(DWORD)(std::clamp(mag, 0.0f, 1.0f) * 10000), 0, 0,
                    (DWORD)(1000000.0f / std::clamp(hz, 5.0f, 120.0f))};
    DIEFFECT e = {}; e.dwSize = sizeof e;
    e.cbTypeSpecificParams = sizeof p; e.lpvTypeSpecificParams = &p;
    Chk(name, fx->SetParameters(&e, DIEP_TYPESPECIFICPARAMS));
}

static void SetCondition(IDirectInputEffect *fx, const char *name, float amount) {
    if (!fx) return;
    LONG k = (LONG)(std::clamp(amount, 0.0f, 1.0f) * 10000);
    DICONDITION c = {0, k, k, 10000, 10000, 0};
    DIEFFECT e = {}; e.dwSize = sizeof e;
    e.cbTypeSpecificParams = sizeof c; e.lpvTypeSpecificParams = &c;
    Chk(name, fx->SetParameters(&e, DIEP_TYPESPECIFICPARAMS));
}

// constant: signed force (+ = towards +axis); friction / damper: 0..1 resistance; periodic: vibrations
static void SetFfb(Dev &d, const FfbOut &o) {
    if (!d.ok) return;
    float c = std::clamp(o.constant, -1.0f, 1.0f);
    if (d.constant) {
        DICONSTANTFORCE cf = {(LONG)(c * 10000)};
        DIEFFECT e = {}; e.dwSize = sizeof e;
        e.cbTypeSpecificParams = sizeof cf; e.lpvTypeSpecificParams = &cf;
        Chk("constant SetParameters", d.constant->SetParameters(&e, DIEP_TYPESPECIFICPARAMS));
    }
    SetCondition(d.friction, "friction SetParameters", o.friction);
    SetCondition(d.damper, "damper SetParameters", o.damper);
    SetPeriodic(d.engine, "engine SetParameters", o.engHz, o.engMag);
    SetPeriodic(d.road, "road SetParameters", o.roadHz, o.roadMag);
    SetPeriodic(d.slip, "slip SetParameters", o.slipHz, o.slipMag);
}
static void StartFfb(Dev &d) {
    SetFfb(d, FfbOut{});
    for (IDirectInputEffect *fx : {d.constant, d.friction, d.damper, d.engine, d.road, d.slip})
        if (fx) Chk("Start", fx->Start(1, 0));
}
static void ReleaseEffects(Dev &d) {
    for (IDirectInputEffect **fx : {&d.constant, &d.friction, &d.damper, &d.engine, &d.road, &d.slip})
        if (*fx) { (*fx)->Stop(); (*fx)->Release(); *fx = nullptr; }
}
static void ReleaseDev(Dev &d) {
    ReleaseEffects(d);
    if (d.dev) { d.dev->Unacquire(); d.dev->Release(); d.dev = nullptr; }
    d.ok = false; d.name.clear(); d.st = {};
}

// take the wheel back as the exclusive owner (the force feedback commands are refused otherwise)
static void ReacquireWheel(Dev &d, HWND hwnd) {
    if (!d.ok) return;
    d.dev->Unacquire();
    HRESULT h1 = d.dev->SetCooperativeLevel(hwnd, DISCL_EXCLUSIVE | DISCL_BACKGROUND);
    SetAutoCenterOff(d, "re-acquire");
    HRESULT h2 = d.dev->Acquire();
    SetAutoCenterOff(d, "re-acquired");
    static int logged = 0;
    if (logged++ < 8) Log("ffb: re-acquiring wheel exclusively: coop=0x%lX acquire=0x%lX", (unsigned long)h1, (unsigned long)h2);
    if (SUCCEEDED(h1) && SUCCEEDED(h2)) {
        g_ffbLost = false;
        for (IDirectInputEffect *fx : {d.constant, d.friction, d.damper, d.engine, d.road, d.slip})
            if (fx) fx->Start(1, 0);
    }
}

// effects stop whenever the device is re-acquired (focus change, lost input): restart them when they are not playing
static void EnsurePlaying(Dev &d) {
    if (!d.ok) return;
    static int restarts = 0;
    for (IDirectInputEffect *fx : {d.constant, d.friction, d.damper, d.engine, d.road, d.slip}) {
        if (!fx) continue;
        DWORD st = 0;
        if (SUCCEEDED(fx->GetEffectStatus(&st)) && !(st & DIEGES_PLAYING)) {
            Chk("restart Start", fx->Start(1, 0));
            if (restarts++ < 6) Log("ffb effect was not playing, restarted (count %d)", restarts);
        }
    }
}
static void StopFfb(Dev &d) {
    for (IDirectInputEffect *fx : {d.constant, d.friction, d.damper, d.engine, d.road, d.slip})
        if (fx) fx->Stop();
}

// ---------- vehicle memory (gears) ----------
static int g_nextGearOff, g_curGearOff, g_topGearOff;

static uintptr_t FindPattern(const char *pat, const char *mask) {
    HMODULE m = GetModuleHandleA(nullptr);
    auto *dos = (IMAGE_DOS_HEADER *)m;
    auto *nt = (IMAGE_NT_HEADERS *)((BYTE *)m + dos->e_lfanew);
    auto *base = (const BYTE *)m;
    size_t size = nt->OptionalHeader.SizeOfImage, len = strlen(mask);
    for (size_t i = 0; i + len < size; i++) {
        size_t j = 0;
        for (; j < len; j++) if (mask[j] == 'x' && base[i + j] != (BYTE)pat[j]) break;
        if (j == len) return (uintptr_t)(base + i);
    }
    return 0;
}

// ---- probe: do the code patterns used by other gearbox mods exist in this game build? (log only, nothing is patched) ----
static int CountPattern(const char *pat, const char *mask) {
    HMODULE m = GetModuleHandleA(nullptr);
    auto *dos = (IMAGE_DOS_HEADER *)m;
    auto *nt = (IMAGE_NT_HEADERS *)((BYTE *)m + dos->e_lfanew);
    auto *base = (const BYTE *)m;
    size_t size = nt->OptionalHeader.SizeOfImage, len = strlen(mask);
    int n = 0;
    for (size_t i = 0; i + len < size; i++) {
        size_t j = 0;
        for (; j < len; j++) if (mask[j] == 'x' && base[i + j] != (BYTE)pat[j]) break;
        if (j == len) n++;
    }
    return n;
}
// every place in the game's code that matches a pattern (not only the first one)
static std::vector<uintptr_t> FindAllPatterns(const char *pat, const char *mask) {
    std::vector<uintptr_t> v;
    HMODULE m = GetModuleHandleA(nullptr);
    auto *dos = (IMAGE_DOS_HEADER *)m;
    auto *nt = (IMAGE_NT_HEADERS *)((BYTE *)m + dos->e_lfanew);
    auto *base = (const BYTE *)m;
    size_t size = nt->OptionalHeader.SizeOfImage, len = strlen(mask);
    for (size_t i = 0; i + len < size; i++) {
        size_t j = 0;
        for (; j < len; j++) if (mask[j] == 'x' && base[i + j] != (BYTE)pat[j]) break;
        if (j == len) v.push_back((uintptr_t)(base + i));
    }
    return v;
}

static void ProbeGearboxPatterns() {
    struct P { const char *name, *pat, *mask; };
    const P ps[] = {
        {"shift up", "\x66\x89\x0B\xB8\x05\x00\x00\x00", "xxxxxxxx"},
        {"shift down", "\x66\x89\x13\x44\x89\x73\x68\xEB", "xxxxx?xx"},
        {"clutch low rpm", "\xC7\x43\x40\xCD\xCC\xCC\x3D\x66", "xx?xxxxx"},
        {"clutch rev limit", "\xC7\x43\x40\xCD\xCC\xCC\x3D\x44\x89\x6B\x6C\x44\x89\x73\x68", "xx?xxxxxx??xx??"},
        {"throttle lift", "\x44\x89\x77\x50\xF3\x0F\x11", "xxxxxxx"},
    };
    for (const P &p : ps) Log("gearbox code pattern '%s': %d match(es)", p.name, CountPattern(p.pat, p.mask));
}

// Every "mov dword [rbx+x], 0.1f" in the game's gearbox code sets the clutch to 0.1: once when the revs are low, and once at the
// shift-up point near the rev limit (the game wants to change up and lets the clutch go). With a manual gearbox the first traps the
// engine at idle in the higher gears, the second drops the revs to idle at ~70% rpm even at full throttle (seen in a capture:
// the game rewrote the clutch to 0.1 every frame). While the player drives a manual vehicle they are all disabled and restored
// again as soon as that stops.
struct PatchSite { uintptr_t addr; BYTE orig[7]; };
static std::vector<PatchSite> g_clutchSites;
static bool g_clutchPatched, g_clutchPatchGaveUp, g_clutchSitesFound;

static void SetClutchPatch(bool on) {
    if (on == g_clutchPatched) return;
    if (on) {
        if (g_clutchPatchGaveUp) return;
        if (!g_clutchSitesFound) {
            g_clutchSitesFound = true;
            std::vector<uintptr_t> all = FindAllPatterns("\xC7\x43\x00\xCD\xCC\xCC\x3D", "xx?xxxx");
            for (uintptr_t a : all) {
                const BYTE *p = (const BYTE *)a;
                Log("clutch=0.1 store at %p: %02X %02X %02X | next bytes %02X %02X %02X %02X %02X %02X", (void *)a, p[0], p[1], p[2],
                    p[7], p[8], p[9], p[10], p[11], p[12]);
            }
            std::vector<uintptr_t> anchor = FindAllPatterns("\xC7\x43\x00\xCD\xCC\xCC\x3D\x66", "xx?xxxxx");   // the known low-rpm clutch store
            std::vector<uintptr_t> chosen;
            if (anchor.size() == 1) {
                BYTE disp = *(const BYTE *)(anchor[0] + 2);
                for (uintptr_t a : all) {
                    long long d = (long long)a - (long long)anchor[0];
                    if (d < 0) d = -d;
                    if (d <= 0x1000 && *(const BYTE *)(a + 2) == disp) chosen.push_back(a);
                }
            }
            if (chosen.empty() || chosen.size() > 8) {
                Log("clutch patch: %d of %d place(s) selected, not patching", (int)chosen.size(), (int)all.size());
                g_clutchPatchGaveUp = true; return;
            }
            Log("clutch patch: %d of %d clutch=0.1 stores belong to the gearbox function", (int)chosen.size(), (int)all.size());
            for (uintptr_t a : chosen) { PatchSite s{}; s.addr = a; g_clutchSites.push_back(s); }
        }
        int done = 0;
        for (PatchSite &s : g_clutchSites) {
            BYTE *a = (BYTE *)s.addr;
            DWORD old = 0;
            if (!VirtualProtect(a, 7, PAGE_EXECUTE_READWRITE, &old)) continue;
            memcpy(s.orig, a, 7);
            memset(a, 0x90, 7);
            VirtualProtect(a, 7, old, &old);
            FlushInstructionCache(GetCurrentProcess(), a, 7);
            done++;
        }
        g_clutchPatched = done > 0;
        if (!g_clutchPatched) g_clutchPatchGaveUp = true;
        Log("clutch patch applied at %d place(s)", done);
    } else {
        for (PatchSite &s : g_clutchSites) {
            BYTE *a = (BYTE *)s.addr;
            DWORD old = 0;
            if (s.orig[0] == 0xC7 && VirtualProtect(a, 7, PAGE_EXECUTE_READWRITE, &old)) {
                memcpy(a, s.orig, 7);
                VirtualProtect(a, 7, old, &old);
                FlushInstructionCache(GetCurrentProcess(), a, 7);
            }
        }
        g_clutchPatched = false;
        static int restored = 0;
        if (restored++ < 4) Log("clutch patch restored");
    }
}
static void FindGearOffsets() {
    uintptr_t a = FindPattern("\x48\x8D\x8F\x00\x00\x00\x00\x4C\x8B\xC3\xF3\x0F\x11\x7C\x24", "xxx????xxxxxxxx");
    if (!a) { Log("gear pattern NOT found: manual gears disabled"); return; }
    int o = *(int *)(a + 3);
    g_nextGearOff = o; g_curGearOff = o + 2; g_topGearOff = o + 6;
    Log("gear offsets next=0x%X cur=0x%X top=0x%X", g_nextGearOff, g_curGearOff, g_topGearOff);
}

static int g_rpmOff, g_throttleOff;
static bool g_grindNow; // set when a gear is forced in without the clutch (gearbox grind vibration)
static ULONGLONG g_ffbTestUntil;             // menu "Test force feedback": strong left/right pulses for ~2.5 s
static void StartFfbTest() { g_ffbTestUntil = GetTickCount64() + 2500; }
static bool g_calibRequest;                 // menu "Detect FFB direction"
static std::string g_calibMsg;
static ULONGLONG g_calibMsgUntil;
static void StartFfbCalibration() { g_calibRequest = true; }

static void FindRpmOffset() {
    uintptr_t a = FindPattern("\x76\x03\x0F\x28\xF0\xF3\x44\x0F\x10\x93", "xxxxxxxxxx");
    if (!a) { Log("rpm pattern NOT found: free revving disabled"); return; }
    g_rpmOff = *(int *)(a + 10);
    g_throttleOff = g_rpmOff + 0x10;
    Log("rpm offset=0x%X throttle=0x%X", g_rpmOff, g_throttleOff);
}

// engine disconnected from the wheels (neutral / clutch in): let the player rev it freely
static void RevEngine(Vehicle v, float thr) {
    if (!g_rpmOff) return;
    BYTE *p = getScriptHandleBaseAddress(v);
    if (!p) return;
    static float rpm = 0.2f;
    float target = 0.2f + thr * 0.8f;
    rpm += (target - rpm) * (target > rpm ? 0.25f : 0.08f); // quick rise, slower fall
    *(float *)(p + g_rpmOff) = rpm;
    *(float *)(p + g_throttleOff) = thr; // drives the engine sound
}

// Direct steering: write the steering input into the vehicle instead of feeding INPUT_VEH_MOVE_LR,
// which the game filters (dead zone / smoothing meant for sticks). Memory value: +1 = full left.
static int g_steerInputOff;
static float g_steerSign = -1.0f;    // memory value = sign * (steer, right positive); ikt-style layout: +1 = left
static DWORD ExeStamp() {
    auto *dos = (IMAGE_DOS_HEADER *)GetModuleHandleA(nullptr);
    return ((IMAGE_NT_HEADERS *)((BYTE *)dos + dos->e_lfanew))->FileHeader.TimeDateStamp;
}

static void FindSteerOffset() {
    // 1) a pattern known from older game builds
    uintptr_t a = FindPattern("\x74\x0A\xF3\x0F\x11\xB3\x1C\x09\x00\x00\xEB\x25", "xxxxxx????xx");
    if (a) {
        g_steerInputOff = *(int *)(a + 6);
        Log("steering input offset=0x%X (pattern)", g_steerInputOff);
        return;
    }
    // 2) an offset found by an earlier run on this exact game build
    std::string stamp = IniStr("Memory", "SteerStamp", "");
    char cur[16]; snprintf(cur, sizeof cur, "%lX", (unsigned long)ExeStamp());
    int off = (int)strtol(IniStr("Memory", "SteerOffset", "0").c_str(), nullptr, 0);
    if (off && stamp == cur) {
        g_steerInputOff = off;
        g_steerSign = IniF("Memory", "SteerSign", -1.0f) < 0 ? -1.0f : 1.0f;
        Log("steering input offset=0x%X sign=%+.0f (saved from an earlier run)", g_steerInputOff, g_steerSign);
        return;
    }
    Log("steering input pattern NOT found: will find the offset while you drive (turn the wheel left and right)");
}

// ---- runtime discovery of the steering input field ----
// While the game steers from our control input, the vehicle's steering-input float follows it.
// Watch every float in a block of the vehicle struct and keep the one that tracks the command.
struct SteerCand { int off; double sx = 0, sy = 0, sxx = 0, syy = 0, sxy = 0; float ymin = 1e9f, ymax = -1e9f; bool bad = false; };
static std::vector<SteerCand> g_cands;
static int g_discN = 0, g_discTries = 0;
static float g_discXmin = 1e9f, g_discXmax = -1e9f;
static bool g_discNeedSave = false;

static void ResetSteerDiscovery() { g_cands.clear(); g_discN = 0; g_discXmin = 1e9f; g_discXmax = -1e9f; }
static void StartSteerCalibration() { g_steerInputOff = 0; g_discTries = 0; ResetSteerDiscovery(); Log("steering calibration restarted"); }

static void DiscoverSteer(Vehicle v, float cmd) {
    BYTE *p = getScriptHandleBaseAddress(v);
    if (!p) return;
    const int LO = 0x700, HI = 0xD00;
    if (g_cands.empty()) for (int o = LO; o < HI; o += 4) { SteerCand c; c.off = o; g_cands.push_back(c); }
    g_discXmin = std::min(g_discXmin, cmd); g_discXmax = std::max(g_discXmax, cmd);
    for (auto &c : g_cands) {
        if (c.bad) continue;
        float y = *(float *)(p + c.off);
        if (!std::isfinite(y) || fabsf(y) > 1.05f) { c.bad = true; continue; }
        c.sx += cmd; c.sy += y; c.sxx += (double)cmd * cmd; c.syy += (double)y * y; c.sxy += (double)cmd * y;
        c.ymin = std::min(c.ymin, y); c.ymax = std::max(c.ymax, y);
    }
    if (++g_discN < 360) return;
    if (g_discXmax - g_discXmin < 0.5f) { ResetSteerDiscovery(); return; } // not enough steering yet, keep watching

    int best = -1; double bestScore = 0, bestCorr = 0;
    int shown = 0;
    for (int i = 0; i < (int)g_cands.size(); i++) {
        auto &c = g_cands[i];
        if (c.bad) continue;
        double n = g_discN, vx = n * c.sxx - c.sx * c.sx, vy = n * c.syy - c.sy * c.sy;
        if (vx <= 1e-9 || vy <= 1e-9) continue;
        double corr = (n * c.sxy - c.sx * c.sy) / sqrt(vx * vy);
        double range = c.ymax - c.ymin;
        if (fabs(corr) < 0.96 || range < 0.4) continue;
        if (shown++ < 6) Log("steer candidate off=0x%X corr=%+.3f range=%.2f", c.off, corr, range);
        if (range > bestScore) { bestScore = range; best = i; bestCorr = corr; } // the input spans the full -1..1, the angle only its lock
    }
    if (best < 0) {
        Log("steering offset not found yet (try %d): turn the wheel fully left and right while driving", ++g_discTries);
        ResetSteerDiscovery();
        return;
    }
    g_steerInputOff = g_cands[best].off;
    g_steerSign = bestCorr < 0 ? -1.0f : 1.0f;
    g_discNeedSave = true;
    Log("steering input offset=0x%X sign=%+.0f found at runtime (corr %+.3f)", g_steerInputOff, g_steerSign, bestCorr);
    ResetSteerDiscovery();
}

static void SetSteerInput(Vehicle v, float rightPositive) {
    if (!g_steerInputOff) return;
    BYTE *p = getScriptHandleBaseAddress(v);
    if (!p) return;
    *(float *)(p + g_steerInputOff) = g_steerSign * std::clamp(rightPositive, -1.0f, 1.0f);
}

// highest gear this vehicle has (6-7 stock, more with modified gearboxes); read from the game, capped at MAXG
static int TopGear(Vehicle v) {
    if (!g_topGearOff) return MAXG;
    BYTE *p = getScriptHandleBaseAddress(v);
    if (!p) return MAXG;
    int t = *(uint8_t *)(p + g_topGearOff);
    return (t >= 1 && t <= MAXG) ? t : MAXG;
}

static void SetGear(Vehicle v, int gear) {
    if (!g_nextGearOff) return;
    BYTE *p = getScriptHandleBaseAddress(v);
    if (!p) return;
    int top = *(uint8_t *)(p + g_topGearOff);
    if (top >= 1 && top <= MAXG) gear = std::min(gear, top);   // never beyond what this gearbox has; ignore a bogus read
    if (gear < 1) return;
    *(uint16_t *)(p + g_nextGearOff) = (uint16_t)gear;
    *(uint16_t *)(p + g_curGearOff) = (uint16_t)gear;
}

// ---------- main loop ----------
static float Pedal(const Dev &d, const std::string &axis, bool invert) {
    float v = Axis(d, axis);
    v = invert ? (1.0f - v) * 0.5f : (v + 1.0f) * 0.5f;
    return std::clamp(v, 0.0f, 1.0f);
}

static void DumpState() {
    for (Dev *d : {&g_wheel, &g_pedals, &g_shifter}) {
        if (!d->ok) continue;
        std::string b;
        for (int i = 0; i < 16; i++) if (d->st.rgbButtons[i] & 0x80) b += std::to_string(i) + " ";
        Log("[%s] X=%ld Y=%ld Z=%ld RX=%ld RY=%ld RZ=%ld S0=%ld S1=%ld buttons: %s", d->name.c_str(),
            d->st.lX, d->st.lY, d->st.lZ, d->st.lRx, d->st.lRy, d->st.lRz, d->st.rglSlider[0], d->st.rglSlider[1], b.c_str());
    }
}

// ---------- in-game menu (F11) ----------
static HWND g_hwnd;
static bool g_menuOpen;
static int g_sel, g_learn = -1;
static ULONGLONG g_learnStart;
static float g_base[8];
static BYTE g_baseBtn[128];
static float g_liveSteer, g_liveThr, g_liveBrk, g_liveClutch;
static float g_dbgSteerAngle;   // the game's own steering angle field (radians) of the car being driven: diagnostic for the steering lock
static int g_liveGear;
static const char *AXIS_NAMES[8] = {"X", "Y", "Z", "RX", "RY", "RZ", "S0", "S1"};

enum RowKind { R_BOOL, R_FLOAT, R_LEARN_AXIS, R_LEARN_BTN, R_SAVE, R_ACTION, R_CHOICE, R_DEVICE, R_KEY, R_HEADER };
struct Row {
    const char *label; RowKind kind;
    bool *b = nullptr; const char *on = "ON", *off = "OFF";
    float *f = nullptr, lo = 0, hi = 1, step = 0.05f; bool pct = false; int cat = 0;
    void (*act)() = nullptr; const char *btn_text = "";
    bool handDev = false;                                  // learn on the handbrake device (or pedals / wheel when none)
    int *key = nullptr; int keyDef = 0;                    // R_KEY: keyboard key (virtual-key code) and its default
    int *ci = nullptr; const char *copt[3] = {"", "", ""}; int cn = 0;   // R_CHOICE
    std::string *dname = nullptr; const char *autoVal = "", *emptyLabel = "(none)"; // R_DEVICE
    std::string *axis = nullptr; bool *invert = nullptr; bool pedalAxis = false;
    int *btn = nullptr; bool shifterDev = false;
    const char *prompt = "";
};
static std::vector<Row> g_rows;

static void W(const char *sec, const char *key, const std::string &v) {
    WritePrivateProfileStringA(sec, key, v.c_str(), (g_dir + "WheelPlugin.ini").c_str());
}
static std::string Fs(float v) { char b[32]; snprintf(b, sizeof b, "%.2f", v); return b; }
static std::string Bs(bool v) { return v ? "1" : "0"; }

static void SaveConfig() {
    W("Axes", "Steer", cfg.steerAxis); W("Axes", "InvertSteer", Bs(cfg.invertSteer));
    W("Axes", "Throttle", cfg.throttleAxis); W("Axes", "InvertThrottle", Bs(cfg.invertThrottle));
    W("Axes", "Brake", cfg.brakeAxis); W("Axes", "InvertBrake", Bs(cfg.invertBrake));
    W("Axes", "Clutch", cfg.clutchAxis); W("Axes", "InvertClutch", Bs(cfg.invertClutch));
    W("Axes", "WheelRange", Fs(cfg.wheelRange)); W("Axes", "SteerLock", Fs(cfg.steerLock)); W("Axes", "LogSteering", Bs(cfg.steerLog));
    W("Axes", "Deadzone", Fs(cfg.deadzone));
    W("Shifter", "Mode", TRANS_NAMES[g_trans]);
    W("Clutch", "CreepKmh", Fs(cfg.creep1)); W("Clutch", "GearSpacing", Fs(cfg.gearSpacing));
    W("Clutch", "StallSensitivity", Fs(cfg.stallSens)); W("Clutch", "CrankTime", Fs(cfg.crankTime));
    W("Devices", "Wheel", cfg.wheelName); W("Devices", "Pedals", cfg.pedalName);
    W("Devices", "Handbrake", cfg.handName); W("Devices", "Shifter", cfg.shifterName);
    W("Axes", "Handbrake", cfg.handbrakeAxis); W("Axes", "InvertHandbrake", Bs(cfg.invertHandbrake));
    W("Shifter", "HandbrakeButton", std::to_string(cfg.handbrakeButton));
    W("Shifter", "PaddleUp", std::to_string(cfg.paddleUp)); W("Shifter", "PaddleDown", std::to_string(cfg.paddleDown));
    W("Shifter", "ToggleButton", std::to_string(cfg.toggleButton));
    W("Shifter", "StartButton", std::to_string(cfg.startButton));
    W("Shifter", "MarkButton", std::to_string(cfg.markButton));
    W("Keys", "Menu", std::to_string(cfg.keyMenu)); W("Keys", "Transmission", std::to_string(cfg.keyTrans));
    W("Keys", "EngineStart", std::to_string(cfg.keyStart)); W("Keys", "SaveCapture", std::to_string(cfg.keyMark));
    W("Keys", "DumpAxes", std::to_string(cfg.keyDump));
    W("Keys", "Refuel", std::to_string(cfg.keyRefuel));
    W("Sim", "RefuelButton", std::to_string(cfg.refuelButton));
    W("Sim", "Fuel", Bs(cfg.fuelOn)); W("Sim", "FuelRangeKm", Fs(cfg.fuelRange)); W("Sim", "StartFuel", Fs(cfg.startFuel));
    W("Sim", "RefuelRate", Fs(cfg.refuelRate)); W("Sim", "EngineKey", Bs(cfg.engineKey));
    W("Sim", "Speedometer", Bs(cfg.hudSpeedo)); W("Sim", "FuelGauge", Bs(cfg.hudFuel)); W("Sim", "Mph", Bs(cfg.mph));
    W("Sim", "Thai", Bs(cfg.thai)); W("Sim", "ThaiSize", Fs(cfg.thaiSize)); W("Sim", "ThaiBold", Bs(cfg.thaiBold));
    W("Sim", "NeonHud", Bs(cfg.hudNeon)); W("Sim", "HudScale", Fs(cfg.hudScale));
    W("Fitment", "Enabled", Bs(cfg.fitOn)); W("Fitment", "PerPlate", Bs(cfg.fitPerPlate)); W("Fitment", "CamberInvert", Bs(cfg.fitCamberInv)); FitSave(); W("Sim", "FuelBlips", Bs(cfg.fuelBlips)); W("Sim", "FuelMarkers", Bs(cfg.fuelMarkers));
    W("Sim", "FuelPay", Bs(cfg.fuelPay)); W("Sim", "FuelPriceBaht", Fs(cfg.fuelPrice)); W("Sim", "TankLiters", Fs(cfg.tankLiters));
    W("Camera", "OnFoot", std::to_string(cfg.camFoot)); W("Camera", "InVehicle", std::to_string(cfg.camVeh)); W("Camera", "ThirdPersonDistance", std::to_string(cfg.camDist));
    W("Block", "F1", Bs(cfg.blockF1)); W("Block", "F2", Bs(cfg.blockF2)); W("Block", "F3", Bs(cfg.blockF3)); W("Block", "F5toF8", Bs(cfg.blockF5to8));
    for (int i = 0; i <= MAXG; i++)
        W("Shifter", ("Gear" + std::string(i == MAXG ? "R" : std::to_string(i + 1))).c_str(), std::to_string(cfg.gearButtons[i]));
    W("FFB", "Enabled", Bs(cfg.ffb)); W("FFB", "Invert", Bs(cfg.ffbInvert));
    W("FFB", "Version", "2"); W("FFB", "HwSign", cfg.ffbHwSign < 0.0f ? "-1" : "1"); W("FFB", "HwSignKnown", Bs(cfg.ffbDirKnown)); W("FFB", "HardwareConditions", Bs(cfg.hwConditions));
    W("FFB", "Gain", Fs(cfg.ffbGain)); W("FFB", "Weight", Fs(cfg.weight));
    W("FFB", "Damping", Fs(cfg.damping)); W("FFB", "Lateral", Fs(cfg.lateralGain));
    W("FFB", "CenteringLow", Fs(cfg.centerLow)); W("FFB", "CenteringHigh", Fs(cfg.centerHigh));
    W("FFB", "EngineResonance", Fs(cfg.engineRes)); W("FFB", "Terrain", Fs(cfg.terrain));
    W("FFB", "Bumps", Fs(cfg.bumps)); W("FFB", "Collisions", Fs(cfg.collisions));
    W("FFB", "GearboxGrind", Fs(cfg.grind)); W("FFB", "Understeer", Fs(cfg.understeer));
    W("Axes", "NonLinearity", Fs(cfg.nonlin)); W("Axes", "DirectSteering", Bs(cfg.directSteer)); W("Axes", "DeadZoneComp", Fs(cfg.deadComp)); W("Axes", "PedalDeadZoneComp", Fs(cfg.pedalComp));
    static int saves = 0;
    if (saves++ < 3) Log("config saved");
}

static const char *CAT_NAMES[] = {"General", "Force feedback", "FFB effects", "Controls and devices", "Clutch and engine",
                                  "Fuel and engine", "HUD and key blocks", "Fitment"};
static const int N_CATS = 8;
static int g_scroll = 0;   // first visible row of the current category
static int g_cat = -1, g_catSel = 0; // -1 = category list

static void BuildRows() {
    auto B = [](const char *l, bool *b, const char *on = "ON", const char *off = "OFF") {
        Row r{l, R_BOOL}; r.b = b; r.on = on; r.off = off; return r; };
    auto Fl = [](const char *l, float *f, float lo, float hi, float st) {
        Row r{l, R_FLOAT}; r.f = f; r.lo = lo; r.hi = hi; r.step = st; return r; };
    auto Pc = [](const char *l, float *f, float hi = 1.5f) { // percent slider, 1% steps (x5 with 7/9)
        Row r{l, R_FLOAT}; r.f = f; r.lo = 0.0f; r.hi = hi; r.step = 0.01f; r.pct = true; return r; };
    auto Ax = [](const char *l, std::string *a, bool *inv, bool pedal, const char *p, bool hand = false) {
        Row r{l, R_LEARN_AXIS}; r.axis = a; r.invert = inv; r.pedalAxis = pedal; r.prompt = p; r.handDev = hand; return r; };
    auto Ky = [](const char *l, int *k, int def) {
        Row r{l, R_KEY}; r.key = k; r.keyDef = def; r.prompt = "Press the keyboard key to use (Esc = cancel)"; return r; };
    auto Ch = [](const char *l, int *i, const char *a, const char *b2, const char *c3) {
        Row r{l, R_CHOICE}; r.ci = i; r.copt[0] = a; r.copt[1] = b2; r.copt[2] = c3; r.cn = 3; return r; };
    auto Dv = [](const char *l, std::string *n, const char *autoVal, const char *emptyLabel) {
        Row r{l, R_DEVICE}; r.dname = n; r.autoVal = autoVal; r.emptyLabel = emptyLabel; return r; };
    auto Bt = [](const char *l, int *b, bool shifter, const char *p) {
        Row r{l, R_LEARN_BTN}; r.btn = b; r.shifterDev = shifter; r.prompt = p; return r; };
    g_rows.clear();
    int cat = 0;
    auto Add = [&](Row r) { r.cat = cat; g_rows.push_back(r); };

    cat = 0; // General
    Add(Ch("Transmission", &g_trans, "Auto", "Manual (clutch)", "Sequential (paddles)"));
    Add(Fl("Steering lock (deg)", &cfg.steerLock, 90.0f, 900.0f, 10.0f));
    Add(Fl("Pit House rotation (deg)", &cfg.wheelRange, 90.0f, 2520.0f, 10.0f));
    Add(B("Direct steering (no dead zone)", &cfg.directSteer));
    { Row r{"Recalibrate steering offset", R_ACTION}; r.act = StartSteerCalibration; r.btn_text = "Recalibrate"; Add(r); }
    Add(Pc("Steering dead zone comp.", &cfg.deadComp, 0.8f));
    Add(Pc("Pedal dead zone comp.", &cfg.pedalComp, 0.8f));
    Add(Pc("Steering non-linearity", &cfg.nonlin, 1.0f));
    Add(B("Log steering data (CSV)", &cfg.steerLog));
    Add(Fl("Steering deadzone", &cfg.deadzone, 0.0f, 0.2f, 0.01f));
    // camera view used on foot and in a vehicle (applied when getting in / out)
    Add(Ch("Camera on foot", &cfg.camFoot, "Unchanged", "First person", "Third person"));
    Add(Ch("Camera in vehicle", &cfg.camVeh, "Unchanged", "First person", "Third person"));
    Add(Ch("Third-person distance", &cfg.camDist, "Close", "Medium", "Far"));

    cat = 1; // Force feedback, same granularity as the ETS2 controller feedback menu
    Add(B("Force feedback", &cfg.ffb));
    { Row r{"Test force feedback", R_ACTION}; r.act = StartFfbTest; r.btn_text = "Run test"; Add(r); }
    { Row r{"Detect FFB direction (hands off!)", R_ACTION}; r.act = StartFfbCalibration; r.btn_text = "Detect"; Add(r); }
    Add(B("Also use device friction/damper", &cfg.hwConditions));
    Add(Pc("Overall gain", &cfg.ffbGain, 2.0f));
    Add(Pc("Internal friction", &cfg.weight, 1.0f));
    Add(Pc("Damping", &cfg.damping, 1.0f));
    Add(Pc("Self-aligning (car rotation)", &cfg.lateralGain, 2.0f));
    Add(Pc("Centering at low speed", &cfg.centerLow));
    Add(Pc("Centering at high speed", &cfg.centerHigh));
    Add(B("FFB direction flip", &cfg.ffbInvert));

    cat = 2; // FFB effects (vibrations and jolts)
    Add(Pc("Engine resonance", &cfg.engineRes, 1.0f));
    Add(Pc("Terrain surface", &cfg.terrain, 1.0f));
    Add(Pc("Bumps", &cfg.bumps, 1.0f));
    Add(Pc("Collisions", &cfg.collisions, 1.0f));
    Add(Pc("Gearbox grind", &cfg.grind, 1.0f));
    Add(Pc("Understeer slip", &cfg.understeer, 1.0f));

    cat = 3; // Controls and devices: one scrolling page, sections top to bottom
    { Row r{"Devices", R_HEADER}; Add(r); }
    Add(Dv("Wheel base (FFB)", &cfg.wheelName, "auto", "(auto)"));
    Add(Dv("Pedals", &cfg.pedalName, "", "(on wheel base)"));
    Add(Dv("Handbrake device", &cfg.handName, "", "(on pedals / base)"));
    Add(Dv("H-shifter", &cfg.shifterName, "", "(none)"));

    { Row r{"Steering and pedals", R_HEADER}; Add(r); }
    Add(Ax("Steering wheel", &cfg.steerAxis, &cfg.invertSteer, false, "Turn the wheel fully RIGHT"));
    Add(Ax("Throttle pedal", &cfg.throttleAxis, &cfg.invertThrottle, true, "Press the THROTTLE pedal fully"));
    Add(Ax("Brake pedal", &cfg.brakeAxis, &cfg.invertBrake, true, "Press the BRAKE pedal fully"));
    Add(Ax("Clutch pedal", &cfg.clutchAxis, &cfg.invertClutch, true, "Press the CLUTCH pedal fully"));
    Add(Ax("Handbrake lever (analog)", &cfg.handbrakeAxis, &cfg.invertHandbrake, true, "Pull the HANDBRAKE lever fully", true));

    { Row r{"Buttons", R_HEADER}; Add(r); }
    Add(Bt("Paddle shift up (+)", &cfg.paddleUp, false, "Press the UPSHIFT paddle"));
    Add(Bt("Paddle shift down (-)", &cfg.paddleDown, false, "Press the DOWNSHIFT paddle"));
    { Row r = Bt("Handbrake button", &cfg.handbrakeButton, false, "Press the HANDBRAKE button"); r.handDev = true; Add(r); }
    Add(Bt("Transmission mode button", &cfg.toggleButton, false, "Press a button on the wheel"));
    Add(Bt("Engine start / stop button", &cfg.startButton, false, "Press a button on the wheel"));
    Add(Bt("Mark problem (save capture)", &cfg.markButton, false, "Press a button on the wheel"));

    Add(Bt("Refuel button (hold)", &cfg.refuelButton, false, "Press a button on the wheel"));

    { Row r{"Keyboard keys", R_HEADER}; Add(r); }
    Add(Ky("Open this menu", &cfg.keyMenu, VK_F11));
    Add(Ky("Cycle transmission mode", &cfg.keyTrans, VK_F10));
    Add(Ky("Engine start / stop", &cfg.keyStart, VK_F8));
    Add(Ky("Save telemetry capture", &cfg.keyMark, 0));
    Add(Ky("Dump axes to log", &cfg.keyDump, 0));

    Add(Ky("Refuel (hold at a pump)", &cfg.keyRefuel, 'J'));

    { Row r{"H-shifter gears", R_HEADER}; Add(r); }
    static std::string glab[MAXG + 1], gprm[MAXG + 1];   // labels must outlive the rows
    for (int i = 0; i <= MAXG; i++) {
        glab[i] = i == MAXG ? "Reverse" : "Gear " + std::to_string(i + 1);
        gprm[i] = i == MAXG ? "Move shifter into REVERSE" : "Move shifter into GEAR " + std::to_string(i + 1);
        Add(Bt(glab[i].c_str(), &cfg.gearButtons[i], true, gprm[i].c_str()));
    }


    cat = 4; // Clutch and engine
    Add(Fl("Creep speed, 1st gear (km/h)", &cfg.creep1, 0.5f, 8.0f, 0.5f));
    Add(Fl("Creep speed gear step (x)", &cfg.gearSpacing, 1.2f, 3.0f, 0.1f));
    Add(Pc("Stall sensitivity", &cfg.stallSens, 3.0f));
    Add(Fl("Engine crank time (s)", &cfg.crankTime, 0.2f, 2.0f, 0.1f));

    cat = 5; // Fuel and engine: each switch turns its whole function off
    Add(B("Fuel system", &cfg.fuelOn));
    Add(Fl("Full tank range (km)", &cfg.fuelRange, 10.0f, 1000.0f, 5.0f));
    Add(Pc("Tank level of a new vehicle", &cfg.startFuel, 1.0f));
    Add(Pc("Refuel speed (tank % per s)", &cfg.refuelRate, 0.2f));
    Add(B("Start key also stops engine", &cfg.engineKey));
    Add(B("Gas station map icons", &cfg.fuelBlips));
    Add(B("Gas pump markers", &cfg.fuelMarkers));
    Add(B("Charge money for fuel", &cfg.fuelPay));
    Add(Fl("Fuel price (baht per liter)", &cfg.fuelPrice, 5.0f, 200.0f, 1.0f));
    Add(Fl("Tank size (liters)", &cfg.tankLiters, 20.0f, 150.0f, 5.0f));

    cat = 6; // Dashboard and blocked game keys
    Add(B("Speed number on HUD", &cfg.hudSpeedo));
    Add(B("Fuel number on HUD", &cfg.hudFuel));
    Add(B("Speed unit", &cfg.mph, "mph", "km/h"));
    Add(B("Thai text", &cfg.thai));
    Add(Pc("Thai text size", &cfg.thaiSize, 2.0f));
    Add(B("Thai text bold", &cfg.thaiBold));
    Add(B("Neon dashboard style", &cfg.hudNeon));
    Add(Pc("Dashboard size", &cfg.hudScale, 2.0f));
    Add(B("Block F1 (Rockstar Editor record)", &cfg.blockF1, "BLOCKED", "game default"));
    Add(B("Block F2 (Rockstar Editor record 2)", &cfg.blockF2, "BLOCKED", "game default"));
    Add(B("Block F3 (save replay clip)", &cfg.blockF3, "BLOCKED", "game default"));
    Add(B("Block F5-F8 (character / online switch)", &cfg.blockF5to8, "BLOCKED", "game default"));

    cat = 7; // Fitment: wheel track width and ride height of the car being driven (saved per vehicle model)
    Add(B("Fitment", &cfg.fitOn));
    Add(B("Fitment saved per car (plate)", &cfg.fitPerPlate));
    { Row r{"Adjust with the car in view (small panel)", R_ACTION}; r.act = FitCompactOpen; r.btn_text = "Open"; Add(r); }
    Add(Fl("Front track width (cm)", &g_fitUi[0], -20.0f, 20.0f, 0.5f));
    Add(Fl("Rear track width (cm)", &g_fitUi[1], -20.0f, 20.0f, 0.5f));
    Add(Fl("Front ride height (cm)", &g_fitUi[2], -20.0f, 20.0f, 0.5f));
    Add(Fl("Rear ride height (cm)", &g_fitUi[3], -20.0f, 20.0f, 0.5f));
    Add(Fl("Front camber (deg)", &g_fitUi[4], -20.0f, 20.0f, 0.5f));
    Add(Fl("Rear camber (deg)", &g_fitUi[5], -20.0f, 20.0f, 0.5f));
    Add(B("Invert camber direction", &cfg.fitCamberInv));
    { Row r{"Reset fitment of this car", R_ACTION}; r.act = FitReset; r.btn_text = "Reset"; Add(r); }
    { Row r{"Save wheel data to a file (for analysis)", R_ACTION}; r.act = FitDump; r.btn_text = "Save"; Add(r); }
}

static bool KeyEdge(int vk) {
    static bool st[256];
    bool k = (GetAsyncKeyState(vk) & 0x8000) != 0;
    bool e = k && !st[vk];
    st[vk] = k;
    return e;
}

static Vector2 V2(float x, float y) { Vector2 v{}; v.x = x; v.y = y; return v; }
static void DrawBoxNative(float cx, float cy, float w, float h, int r, int g, int b, int a) {
    GRAPHICS::DRAW_RECT(V2(cx, cy), w, h, r, g, b, a, FALSE);
}
#include "thai.h"
// every rectangle goes through the overlay too, so the drawing order is the call order; the game's own DRAW_RECT is the fallback
static void DrawBox(float cx, float cy, float w, float h, int r, int g, int b, int a) {
    if (g_d3dState >= 0) {
        float sw, sh;
        ThScreen(&sw, &sh);
        ThCol c = {r / 255.0f, g / 255.0f, b / 255.0f, a / 255.0f};
        float x0 = floorf((cx - w * 0.5f) * sw + 0.5f), y0 = floorf((cy - h * 0.5f) * sh + 0.5f);
        float x1 = floorf((cx + w * 0.5f) * sw + 0.5f), y1 = floorf((cy + h * 0.5f) * sh + 0.5f);
        if (x1 <= x0) x1 = x0 + 1.0f;
        if (y1 <= y0) y1 = y0 + 1.0f;
        ThPush(ThWhite(), x0, y0, x1, y1, c, c);
        return;
    }
    DrawBoxNative(cx, cy, w, h, r, g, b, a);
}
#include "menu_style.h"
static void DrawStr(float x, float y, const std::string &s, float scale, int r, int g, int b, bool center = false) {
    if (DrawThaiStr(x, y, s, scale, r, g, b, center)) return;
    if (g_d3dState >= 0) { DrawGlyphStr(x, y, s, scale, r, g, b, center); return; }
    HUD::SET_TEXT_FONT(0); HUD::SET_TEXT_SCALE(0.0f, scale);
    if (center) HUD::SET_TEXT_CENTRE(TRUE);
    HUD::SET_TEXT_COLOUR(r, g, b, 255); HUD::SET_TEXT_OUTLINE();
    HUD::BEGIN_TEXT_COMMAND_DISPLAY_TEXT("STRING");
    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME(s.c_str());
    HUD::END_TEXT_COMMAND_DISPLAY_TEXT(V2(x, y), 0);
}

static Dev &LearnDev(const Row &r) {
    if (r.kind == R_LEARN_BTN) {
        if (r.handDev) return g_hand.ok ? g_hand : g_wheel;
        return r.shifterDev ? g_shifter : g_wheel;
    }
    if (r.handDev) return g_hand.ok ? g_hand : (g_pedals.ok ? g_pedals : g_wheel);
    return (r.pedalAxis && g_pedals.ok) ? g_pedals : g_wheel;
}

static bool g_baseKeys[256];
static void StartLearn(int idx) {
    if (g_rows[idx].kind == R_KEY) {
        for (int k = 0; k < 256; k++) g_baseKeys[k] = KeyDown(k);
        g_learn = idx; g_learnStart = GetTickCount64();
        return;
    }
    Dev &d = LearnDev(g_rows[idx]);
    if (!d.ok) { Log("learn: device not available"); return; }
    for (int i = 0; i < 8; i++) g_base[i] = Axis(d, AXIS_NAMES[i]);
    for (int i = 0; i < 128; i++) g_baseBtn[i] = d.st.rgbButtons[i] & 0x80;
    g_learn = idx; g_learnStart = GetTickCount64();
}

static void LearnTick() {
    Row &r = g_rows[g_learn];
    if (GetTickCount64() - g_learnStart > 15000) { g_learn = -1; return; }
    if (r.kind == R_KEY) {
        for (int k = 8; k < 255; k++) {
            if (k == VK_ESCAPE || k == VK_SHIFT || k == VK_CONTROL || k == VK_MENU || k == VK_LBUTTON || k == VK_RBUTTON || k == VK_MBUTTON) continue;
            if (KeyDown(k) && !g_baseKeys[k]) { *r.key = k; Log("bound %s -> key %s", r.label, KeyName(k).c_str()); g_learn = -1; return; }
            if (!KeyDown(k)) g_baseKeys[k] = false;   // a key that was held at the start counts again once released
        }
        return;
    }
    Dev &d = LearnDev(r);
    if (r.kind == R_LEARN_AXIS) {
        int best = -1; float bd = 0;
        for (int i = 0; i < 8; i++) {
            float dl = Axis(d, AXIS_NAMES[i]) - g_base[i];
            if (fabsf(dl) > fabsf(bd)) { bd = dl; best = i; }
        }
        if (best >= 0 && fabsf(bd) > 0.5f) {
            *r.axis = AXIS_NAMES[best];
            // pedals: pressed must read as "up" after mapping; steering: right must be positive
            if (r.invert) *r.invert = bd < 0;
            Log("bound %s -> %s (invert=%d)", r.label, r.axis->c_str(), r.invert ? (int)*r.invert : 0);
            if (r.axis == &cfg.steerAxis) g_recreateFfb = true;   // the force feedback effects must target the new axis
            g_learn = -1;
        }
    } else {
        for (int i = 0; i < 128; i++)
            if ((d.st.rgbButtons[i] & 0x80) && !g_baseBtn[i]) {
                *r.btn = i; Log("bound %s -> button %d", r.label, i); g_learn = -1; break;
            }
    }
}

static std::string RowValue(const Row &r) {
    char b[64];
    switch (r.kind) {
    case R_BOOL: return *r.b ? r.on : r.off;
    case R_FLOAT:
        if (r.pct) snprintf(b, sizeof b, "%.0f%%", *r.f * 100.0f);
        else snprintf(b, sizeof b, r.step >= 1.0f ? "%.0f" : "%.2f", *r.f);
        return b;
    case R_LEARN_AXIS:
        if (r.axis->empty()) return "(off)";
        return *r.axis + (r.invert && *r.invert ? "  inverted" : "");
    case R_LEARN_BTN:
        if (*r.btn < 0) return "(none)";
        snprintf(b, sizeof b, "button %d", *r.btn); return b;
    default: return "";
    }
}

// ---------- mouse UI ----------
static int g_drag = -1, g_edit = -1;     // row being dragged / row whose value box is being typed into
static bool g_editFresh = false;         // first typed character replaces the old text
static std::string g_editBuf;
static bool g_lmbPrev = false, g_rmbPrev = false;

static void Rc(float x, float y, float w, float h, int r, int g, int b, int a) {
    DrawBox(x + w / 2, y + h / 2, w, h, r, g, b, a);
}
static bool In(float mx, float my, float x, float y, float w, float h) {
    return mx >= x && mx <= x + w && my >= y && my <= y + h;
}
static std::vector<int> RowsOf(int cat) {
    std::vector<int> v;
    for (int i = 0; i < (int)g_rows.size(); i++) if (g_rows[i].cat == cat) v.push_back(i);
    return v;
}
static std::string NumText(const Row &r) {
    char b[32];
    if (r.pct) snprintf(b, sizeof b, "%.0f", *r.f * 100.0f);
    else snprintf(b, sizeof b, r.step >= 1.0f ? "%.0f" : "%.2f", *r.f);
    return b;
}
static void CommitEdit() {
    if (g_edit >= 0 && g_edit < (int)g_rows.size() && !g_editBuf.empty()) {
        Row &r = g_rows[g_edit];
        float x = (float)atof(g_editBuf.c_str());
        if (r.pct) x /= 100.0f;
        *r.f = std::clamp(x, r.lo, r.hi);
    }
    g_edit = -1; g_editBuf.clear();
    SaveConfig();
}

// orbit camera around the car while the small fitment panel is open: arrow keys turn it, Page Up / Page Down zoom
static Cam g_orbitCam;
static Vector3 V3(float x, float y, float z) { Vector3 v{}; v.x = x; v.y = y; v.z = z; return v; }
static float g_orbitYaw = 35.0f, g_orbitPitch = 8.0f, g_orbitDist = 5.5f;
static void FitOrbitTick(bool want) {
    Ped ped = PLAYER::PLAYER_PED_ID();
    Vehicle v = PED::IS_PED_IN_ANY_VEHICLE(ped, FALSE) ? PED::GET_VEHICLE_PED_IS_IN(ped, FALSE) : 0;
    if (!want || !v) {
        if (g_orbitCam) {
            CAM::RENDER_SCRIPT_CAMS(FALSE, FALSE, 0, TRUE, FALSE, 0);
            CAM::SET_CAM_ACTIVE(g_orbitCam, FALSE);
            CAM::DESTROY_CAM(g_orbitCam, FALSE);
            g_orbitCam = 0;
        }
        return;
    }
    if (!g_orbitCam) {
        g_orbitCam = CAM::CREATE_CAM("DEFAULT_SCRIPTED_CAMERA", TRUE);
        CAM::SET_CAM_ACTIVE(g_orbitCam, TRUE);
        CAM::RENDER_SCRIPT_CAMS(TRUE, FALSE, 0, TRUE, FALSE, 0);
    }
    if (GetForegroundWindow() == g_hwnd) {
        float dt = MISC::GET_FRAME_TIME(), sp = 100.0f * dt;
        if (GetAsyncKeyState(VK_LEFT) & 0x8000) g_orbitYaw -= sp;
        if (GetAsyncKeyState(VK_RIGHT) & 0x8000) g_orbitYaw += sp;
        if (GetAsyncKeyState(VK_UP) & 0x8000) g_orbitPitch += sp * 0.6f;
        if (GetAsyncKeyState(VK_DOWN) & 0x8000) g_orbitPitch -= sp * 0.6f;
        if (GetAsyncKeyState(VK_PRIOR) & 0x8000) g_orbitDist -= 4.0f * dt;
        if (GetAsyncKeyState(VK_NEXT) & 0x8000) g_orbitDist += 4.0f * dt;
    }
    g_orbitPitch = std::clamp(g_orbitPitch, -5.0f, 80.0f);
    g_orbitDist = std::clamp(g_orbitDist, 2.5f, 12.0f);
    float yaw = g_orbitYaw * 0.0174533f, pit = g_orbitPitch * 0.0174533f;
    Vector3 c = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(v, V3(0.0f, 0.0f, 0.2f));
    // yaw 0 = behind the car, measured around the car (its own heading)
    float lx = sinf(yaw) * cosf(pit) * g_orbitDist, ly = -cosf(yaw) * cosf(pit) * g_orbitDist, lz = sinf(pit) * g_orbitDist;
    Vector3 p = ENTITY::GET_OFFSET_FROM_ENTITY_IN_WORLD_COORDS(v, V3(lx, ly, lz + 0.2f));
    CAM::SET_CAM_COORD(g_orbitCam, p);
    CAM::POINT_CAM_AT_COORD(g_orbitCam, c);
}

static void MenuTick() {
    FitOrbitTick(g_menuOpen && g_fitCompact);
    bool f11 = cfg.keyMenu > 0 && KeyEdge(cfg.keyMenu);
    if (GetForegroundWindow() != g_hwnd) return;
    if (f11) {
        if (g_edit >= 0) CommitEdit();
        g_menuOpen = !g_menuOpen; g_learn = -1; g_drag = -1;
        if (!g_menuOpen) { SaveConfig(); g_fitCompact = false; }
    }
    if (!g_menuOpen) return;

    // the menu owns the mouse and keyboard; only the driving controls the plugin feeds stay enabled
    PAD::DISABLE_ALL_CONTROL_ACTIONS(0);
    for (int c : {59, 71, 72, 76}) PAD::ENABLE_CONTROL_ACTION(0, c, TRUE);
    if (g_d3dState < 0) HUD::SET_MOUSE_CURSOR_THIS_FRAME();

    POINT pt{}; GetCursorPos(&pt); ScreenToClient(g_hwnd, &pt);
    RECT rc{}; GetClientRect(g_hwnd, &rc);
    float mx = rc.right > 0 ? (float)pt.x / rc.right : 0.0f, my = rc.bottom > 0 ? (float)pt.y / rc.bottom : 0.0f;
    bool lmb = (GetAsyncKeyState(VK_LBUTTON) & 0x8000) != 0, rmb = (GetAsyncKeyState(VK_RBUTTON) & 0x8000) != 0;
    bool click = lmb && !g_lmbPrev, rclick = rmb && !g_rmbPrev;
    if (!lmb) g_drag = -1;
    bool release = !lmb && g_lmbPrev;
    g_lmbPrev = lmb; g_rmbPrev = rmb;
    if (release) SaveConfig();          // every change in the menu is written to the ini right away

    if (g_cat < 0) g_cat = 0;

    // keyboard: typing into a value box, or cancelling a bind
    if (g_edit >= 0) {
        auto add = [&](char ch) {
            if (g_editFresh) { g_editBuf.clear(); g_editFresh = false; }
            if (g_editBuf.size() < 6) g_editBuf += ch;
        };
        for (int d = 0; d <= 9; d++)
            if (KeyEdge(0x30 + d) | KeyEdge(VK_NUMPAD0 + d)) add((char)('0' + d));
        if (KeyEdge(VK_DECIMAL) | KeyEdge(VK_OEM_PERIOD)) add('.');
        if (KeyEdge(VK_BACK)) { if (g_editFresh) g_editBuf.clear(); else if (!g_editBuf.empty()) g_editBuf.pop_back(); g_editFresh = false; }
        if (KeyEdge(VK_RETURN)) CommitEdit();
        else if (KeyEdge(VK_ESCAPE)) { g_edit = -1; g_editBuf.clear(); }
    } else if (g_learn >= 0) {
        if (KeyEdge(VK_ESCAPE)) g_learn = -1; else LearnTick();
    }

    if (g_fitCompact) {   // small fitment panel on the left: the main menu is hidden so the car can be seen while adjusting
        const float x0 = 0.02f, y0 = 0.17f, pw = 0.25f, ph = 0.66f;
        NShape(x0, y0, pw, ph, 0.016f, 195, Rgb(0, 0, 0, 0), Rgb(0, 0, 0, 0), Rgb(60, 200, 255, 0.95f), 2.0f, true);
        DrawStr(x0 + 0.014f, y0 + 0.012f, "Fitment", 0.42f, 255, 255, 255);
        DrawStr(x0 + pw / 2, y0 + 0.52f, "Arrow keys: turn the camera around the car", 0.24f, 160, 200, 230, true);
        DrawStr(x0 + pw / 2, y0 + 0.55f, "Page Up / Page Down: zoom", 0.24f, 160, 200, 230, true);
        Rc(x0 + 0.012f, y0 + 0.058f, pw - 0.024f, 0.0015f, 60, 200, 255, 160);
        static const char *lbl[FIT_N] = {"Front track width (cm)", "Rear track width (cm)", "Front ride height (cm)", "Rear ride height (cm)", "Front camber (deg)", "Rear camber (deg)"};
        const float sx = x0 + 0.016f, sl = pw - 0.095f;
        for (int i = 0; i < FIT_N; i++) {
            float y = y0 + 0.072f + i * 0.068f, lim = 20.0f;
            DrawStr(sx, y, lbl[i], 0.28f, 225, 228, 235);
            float t = std::clamp((g_fitUi[i] + lim) / (2.0f * lim), 0.0f, 1.0f);
            NShape(sx, y + 0.036f, sl, 0.010f, 0.005f, 190, Rgb(0, 0, 0, 0), Rgb(0, 0, 0, 0), Rgb(90, 120, 170, 0.7f), 1.5f, false);
            if (sl * t > 0.012f) NShape(sx, y + 0.036f, sl * t, 0.010f, 0.005f, 0, Rgb(90, 235, 255, 1.0f), Rgb(50, 130, 255, 1.0f), Rgb(0, 0, 0, 0), 1.0f, false);
            bool dragging = (g_drag == 2000 + i);
            NShape(sx + sl * t - 0.004f, y + 0.026f, 0.008f, 0.031f, 0.005f, 0, Rgb(255, 255, 255, 1.0f), Rgb(200, 225, 255, 1.0f), Rgb(120, 225, 255, dragging ? 1.0f : 0.8f), 2.0f, true);
            if (click && In(mx, my, sx - 0.006f, y + 0.024f, sl + 0.012f, 0.036f)) g_drag = 2000 + i;
            if (g_drag == 2000 + i && lmb) g_fitUi[i] = roundf((-lim + std::clamp((mx - sx) / sl, 0.0f, 1.0f) * 2.0f * lim) * 2.0f) / 2.0f;
            char vb[16];
            snprintf(vb, sizeof vb, "%+.1f", g_fitUi[i]);
            DrawStr(sx + sl + 0.037f, y + 0.026f, vb, 0.3f, 255, 255, 255, true);
        }
        const float by = y0 + ph - 0.062f, bw = 0.105f;
        bool hr = In(mx, my, x0 + 0.014f, by, bw, 0.042f), hs = In(mx, my, x0 + pw - bw - 0.014f, by, bw, 0.042f);
        NShape(x0 + 0.014f, by, bw, 0.042f, 0.012f, 150, Rgb(0, 0, 0, 0), Rgb(0, 0, 0, 0), hr ? Rgb(255, 120, 120, 0.95f) : Rgb(190, 90, 100, 0.7f), 1.6f, hr);
        DrawStr(x0 + 0.014f + bw / 2, by + 0.009f, "Reset", 0.3f, 255, 255, 255, true);
        NShape(x0 + pw - bw - 0.014f, by, bw, 0.042f, 0.012f, 0, Rgb(60, 230, 160, hs ? 1.0f : 0.85f), Rgb(25, 150, 200, hs ? 1.0f : 0.85f), Rgb(150, 255, 220, 1.0f), 1.8f, true);
        DrawStr(x0 + pw - bw / 2 - 0.014f, by + 0.009f, "Save and back", 0.28f, 255, 255, 255, true);
        if (click && hr) FitReset();
        if (click && hs) { g_fitCompact = false; SaveConfig(); }
        NCursor(mx, my);
        return;
    }

    // ---- layout ----
    const float PX = 0.17f, PY = 0.07f, PW = 0.66f, PH = 0.86f, SBW = 0.13f;
    const float CX0 = PX + SBW + 0.015f, LBL = CX0 + 0.01f;
    const float SLX = CX0 + 0.235f, SLW = 0.17f, VBX = SLX + SLW + 0.015f, VBW = 0.075f;
    const float ROW0 = PY + 0.105f, ROWH = 0.047f;

    // panel + title bar
    NShape(PX, PY, PW, PH, 0.016f, 205, Rgb(0, 0, 0, 0), Rgb(0, 0, 0, 0), Rgb(60, 200, 255, 0.95f), 2.0f, true);


    Rc(PX + 0.012f, PY + 0.066f, PW - 0.024f, 0.0015f, 60, 200, 255, 160);
    DrawStr(PX + 0.015f, PY + 0.011f, "WHEEL SETUP", 0.5f, 255, 255, 255);
    {
        float bx = PX + PW - 0.05f, by = PY + 0.012f;
        bool hv = In(mx, my, bx, by, 0.04f, 0.04f);
        NShape(bx, by, 0.04f, 0.04f, 0.01f, 170, Rgb(255, 70, 80, hv ? 0.55f : 0.25f), Rgb(200, 30, 50, hv ? 0.55f : 0.25f), Rgb(255, 90, 100, 0.95f), 2.0f, hv);
        DrawStr(bx + 0.02f, by + 0.007f, "X", 0.4f, 255, 255, 255, true);
        if (click && hv) { if (g_edit >= 0) CommitEdit(); SaveConfig(); g_menuOpen = false; return; }
    }

    // sidebar categories

    for (int c = 0; c < N_CATS; c++) {
        float by = PY + 0.085f + c * 0.062f, bx = PX + 0.008f, bw = SBW - 0.016f;
        bool hv = In(mx, my, bx, by, bw, 0.05f), on = (c == g_cat);
        NShape(bx, by, bw, 0.05f, 0.012f, on ? 0 : 150, on ? Rgb(40, 150, 255, 0.9f) : Rgb(0, 0, 0, 0), on ? Rgb(20, 90, 220, 0.9f) : Rgb(0, 0, 0, 0), on ? Rgb(120, 225, 255, 1.0f) : (hv ? Rgb(120, 170, 230, 0.85f) : Rgb(80, 100, 140, 0.55f)), on ? 2.0f : 1.5f, on);
        DrawStr(bx + bw / 2, by + 0.015f, CAT_NAMES[c], 0.3f, 255, 255, 255, true);
        if (click && hv && c != g_cat) { if (g_edit >= 0) CommitEdit(); g_cat = c; g_learn = -1; g_scroll = 0; }
    }

    // rows of the current category
    std::vector<int> idx = RowsOf(g_cat);
    bool clickedBox = false;
    const int VIS = 13, maxScroll = std::max(0, (int)idx.size() - VIS);
    if (g_edit < 0 && g_learn < 0) {   // mouse wheel, Page Up / Page Down
        if (PAD::IS_DISABLED_CONTROL_JUST_PRESSED(0, 241) || KeyEdge(VK_PRIOR)) g_scroll -= 3;
        if (PAD::IS_DISABLED_CONTROL_JUST_PRESSED(0, 242) || KeyEdge(VK_NEXT)) g_scroll += 3;
    }
    if (maxScroll > 0) {               // scroll bar at the right edge of the list
        const float tx = CX0 + PW - SBW - 0.021f, ty = ROW0, th = VIS * ROWH - 0.004f;
        float hh = std::max(0.04f, th * VIS / (float)idx.size()), hy = ty + (th - hh) * g_scroll / (float)maxScroll;
        Rc(tx, ty, 0.005f, th, 255, 255, 255, 12);
        if (click && In(mx, my, tx - 0.008f, ty, 0.021f, th)) g_drag = 3000;
        if (g_drag == 3000 && lmb) g_scroll = (int)roundf(std::clamp((my - ty - hh * 0.5f) / (th - hh), 0.0f, 1.0f) * maxScroll);
        Rc(tx, hy, 0.005f, hh, 120, 225, 255, g_drag == 3000 ? 255 : 190);
    }
    g_scroll = std::clamp(g_scroll, 0, maxScroll);
    for (int k = 0; k < (int)idx.size(); k++) {
        if (k < g_scroll || k >= g_scroll + VIS) continue;
        int ri = idx[k];
        Row &r = g_rows[ri];
        float y = ROW0 + (k - g_scroll) * ROWH;
        if (r.kind == R_HEADER) {
            Rc(CX0, y + ROWH - 0.011f, PW - SBW - 0.04f, 0.0015f, 60, 200, 255, 140);
            DrawStr(LBL, y + 0.009f, r.label, 0.38f, 120, 225, 255);
            continue;
        }
        bool rowHv = In(mx, my, CX0, y, PW - SBW - 0.025f, ROWH - 0.004f);
        Rc(CX0, y, PW - SBW - 0.025f, ROWH - 0.004f, 255, 255, 255, rowHv ? 16 : (k & 1 ? 7 : 3));
        DrawStr(LBL, y + 0.011f, r.label, 0.34f, 225, 228, 235);

        if (r.kind == R_FLOAT) {
            float t = std::clamp((*r.f - r.lo) / (r.hi - r.lo), 0.0f, 1.0f);
            NShape(SLX, y + 0.0185f, SLW, 0.010f, 0.005f, 190, Rgb(0, 0, 0, 0), Rgb(0, 0, 0, 0), Rgb(90, 120, 170, 0.7f), 1.5f, false);
            if (SLW * t > 0.012f) NShape(SLX, y + 0.0185f, SLW * t, 0.010f, 0.005f, 0, Rgb(90, 235, 255, 1.0f), Rgb(50, 130, 255, 1.0f), Rgb(0, 0, 0, 0), 1.0f, false);
            bool dragging = (g_drag == ri);
            NShape(SLX + SLW * t - 0.004f, y + 0.008f, 0.008f, 0.031f, 0.005f, 0, Rgb(255, 255, 255, 1.0f), Rgb(200, 225, 255, 1.0f), Rgb(120, 225, 255, dragging ? 1.0f : 0.8f), 2.0f, true);
            if (click && g_edit < 0 && In(mx, my, SLX - 0.006f, y, SLW + 0.012f, ROWH - 0.004f)) g_drag = ri;
            if (g_drag == ri && lmb) {
                float v = r.lo + std::clamp((mx - SLX) / SLW, 0.0f, 1.0f) * (r.hi - r.lo);
                v = r.lo + roundf((v - r.lo) / r.step) * r.step;
                *r.f = std::clamp(roundf(v * 1000.0f) / 1000.0f, r.lo, r.hi);
            }
            bool editing = (g_edit == ri), hv = In(mx, my, VBX, y + 0.005f, VBW, 0.036f);

            NShape(VBX, y + 0.005f, VBW, 0.036f, 0.01f, 165, Rgb(0, 0, 0, 0), Rgb(0, 0, 0, 0), editing ? Rgb(120, 225, 255, 1.0f) : (hv ? Rgb(130, 180, 240, 0.9f) : Rgb(90, 115, 160, 0.65f)), editing ? 2.0f : 1.5f, editing);
            std::string txt = editing ? g_editBuf + "_" : NumText(r) + (r.pct ? "%" : "");
            DrawStr(VBX + VBW / 2, y + 0.0125f, txt, 0.34f, 255, 255, 255, true);
            if (click && hv) {
                if (g_edit >= 0 && g_edit != ri) CommitEdit();
                g_edit = ri; g_editBuf = NumText(r); g_editFresh = true; g_drag = -1; clickedBox = true;
            }
        } else if (r.kind == R_BOOL) {
            bool hv = In(mx, my, SLX, y + 0.006f, 0.085f, 0.034f);
            NShape(SLX, y + 0.006f, 0.085f, 0.034f, 0.012f, *r.b ? 0 : 150, *r.b ? Rgb(60, 225, 130, 0.9f) : Rgb(0, 0, 0, 0), *r.b ? Rgb(25, 150, 85, 0.9f) : Rgb(0, 0, 0, 0), *r.b ? Rgb(150, 255, 190, 1.0f) : (hv ? Rgb(130, 150, 190, 0.9f) : Rgb(90, 100, 130, 0.6f)), 1.8f, *r.b);
            DrawStr(SLX + 0.0425f, y + 0.0125f, *r.b ? r.on : r.off, 0.32f, 255, 255, 255, true);
            if (click && hv) *r.b = !*r.b;
        } else if (r.kind == R_CHOICE) {
            bool hv = In(mx, my, SLX, y + 0.006f, 0.19f, 0.034f);
            NShape(SLX, y + 0.006f, 0.19f, 0.034f, 0.012f, 150, Rgb(0, 0, 0, 0), Rgb(0, 0, 0, 0), hv ? Rgb(130, 190, 250, 0.95f) : Rgb(85, 140, 205, 0.75f), 1.6f, hv);
            DrawStr(SLX + 0.095f, y + 0.0125f, r.copt[std::clamp(*r.ci, 0, r.cn - 1)], 0.32f, 255, 255, 255, true);
            if (click && hv) *r.ci = (*r.ci + 1) % r.cn;
        } else if (r.kind == R_DEVICE) {
            bool hv = In(mx, my, SLX, y + 0.006f, 0.19f, 0.034f);
            NShape(SLX, y + 0.006f, 0.19f, 0.034f, 0.012f, 150, Rgb(0, 0, 0, 0), Rgb(0, 0, 0, 0), hv ? Rgb(130, 190, 250, 0.95f) : Rgb(85, 140, 205, 0.75f), 1.6f, hv);
            std::string shown = r.dname->empty() || *r.dname == "auto" ? r.emptyLabel : *r.dname;
            if (shown.size() > 22) shown = shown.substr(0, 21) + "~";
            DrawStr(SLX + 0.095f, y + 0.0125f, shown, 0.3f, 255, 255, 255, true);
            if (click && hv) {
                // cycle: default value, then every attached controller
                std::vector<std::string> opts{r.autoVal};
                for (auto &a : g_attached) opts.push_back(a.name);
                int cur = 0;
                for (int q = 1; q < (int)opts.size(); q++)
                    if (Lower(*r.dname) == Lower(opts[q]) || (!r.dname->empty() && Lower(opts[q]).find(Lower(*r.dname)) != std::string::npos)) { cur = q; break; }
                *r.dname = opts[(cur + 1) % opts.size()];
                g_reopen = true;
            }
        } else if (r.kind == R_ACTION) {
            bool hv = In(mx, my, SLX, y + 0.006f, 0.17f, 0.034f);
            bool running = GetTickCount64() < g_ffbTestUntil;
            NShape(SLX, y + 0.006f, 0.17f, 0.034f, 0.012f, 0, running ? Rgb(60, 225, 130, 0.9f) : Rgb(60, 170, 255, hv ? 1.0f : 0.85f), running ? Rgb(25, 150, 85, 0.9f) : Rgb(25, 100, 230, hv ? 1.0f : 0.85f), running ? Rgb(150, 255, 190, 1.0f) : Rgb(130, 215, 255, 1.0f), 1.8f, true);
            DrawStr(SLX + 0.085f, y + 0.0125f, running ? "Testing..." : r.btn_text, 0.32f, 255, 255, 255, true);
            if (click && hv) r.act();
        } else if (r.kind == R_KEY) {
            bool learning = (g_learn == ri), hv = In(mx, my, SLX, y + 0.006f, 0.17f, 0.034f);
            NShape(SLX, y + 0.006f, 0.17f, 0.034f, 0.012f, 150, Rgb(0, 0, 0, 0), Rgb(0, 0, 0, 0), learning ? Rgb(255, 90, 100, 1.0f) : (hv ? Rgb(130, 190, 250, 0.95f) : Rgb(85, 140, 205, 0.75f)), 1.6f, hv || learning);
            DrawStr(SLX + 0.085f, y + 0.0125f, learning ? "Press a key..." : KeyName(*r.key), 0.32f, 255, 255, 255, true);
            if (click && hv && g_learn < 0) { if (g_edit >= 0) CommitEdit(); StartLearn(ri); }
            bool hc = In(mx, my, SLX + 0.18f, y + 0.006f, 0.055f, 0.034f);
            NShape(SLX + 0.18f, y + 0.006f, 0.055f, 0.034f, 0.012f, 150, Rgb(0, 0, 0, 0), Rgb(0, 0, 0, 0), hc ? Rgb(255, 120, 120, 0.95f) : Rgb(190, 90, 100, 0.7f), 1.6f, hc);
            DrawStr(SLX + 0.2075f, y + 0.0125f, r.keyDef ? "Reset" : "Clear", 0.3f, 255, 255, 255, true);
            if ((click && hc) || (rclick && hv)) *r.key = r.keyDef;
        } else if (r.kind == R_LEARN_AXIS || r.kind == R_LEARN_BTN) {
            bool learning = (g_learn == ri), hv = In(mx, my, SLX, y + 0.006f, 0.17f, 0.034f);
            NShape(SLX, y + 0.006f, 0.17f, 0.034f, 0.012f, 150, Rgb(0, 0, 0, 0), Rgb(0, 0, 0, 0), learning ? Rgb(255, 90, 100, 1.0f) : (hv ? Rgb(130, 190, 250, 0.95f) : Rgb(85, 140, 205, 0.75f)), 1.6f, hv || learning);
            DrawStr(SLX + 0.085f, y + 0.0125f, learning ? "Waiting..." : RowValue(r), 0.32f, 255, 255, 255, true);
            if (click && hv && g_learn < 0) { if (g_edit >= 0) CommitEdit(); StartLearn(ri); }
            bool clearable = (r.kind == R_LEARN_BTN) || (r.axis == &cfg.clutchAxis) || (r.axis == &cfg.handbrakeAxis);
            if (clearable) {
                bool hc = In(mx, my, SLX + 0.18f, y + 0.006f, 0.055f, 0.034f);
                NShape(SLX + 0.18f, y + 0.006f, 0.055f, 0.034f, 0.012f, 150, Rgb(0, 0, 0, 0), Rgb(0, 0, 0, 0), hc ? Rgb(255, 120, 120, 0.95f) : Rgb(190, 90, 100, 0.7f), 1.6f, hc);
                DrawStr(SLX + 0.2075f, y + 0.0125f, "Clear", 0.3f, 255, 255, 255, true);
                auto doClear = [&]() { if (r.kind == R_LEARN_BTN) *r.btn = -1; else r.axis->clear(); };
                if (click && hc) doClear();
                if (rclick && hv) doClear();
            }
        }
    }
    if (click && g_edit >= 0 && !clickedBox) CommitEdit();

    // footer: live values, hints, buttons
    float fy = PY + PH - 0.125f;
    Rc(CX0, fy - 0.008f, PW - SBW - 0.025f, 0.002f, 60, 66, 86, 255);
    char live[160];
    const char *gs = g_liveGear == 0 ? "N" : (g_liveGear < 0 ? "R" : nullptr);
    char gb[8]; if (!gs) { snprintf(gb, sizeof gb, "%d", g_liveGear); gs = gb; }
    snprintf(live, sizeof live, "Steer %+.2f   Thr %.2f   Brk %.2f   Clu %.2f   Gear %s   Wheel %+.0f deg",
             g_liveSteer, g_liveThr, g_liveBrk, g_liveClutch, gs, g_dbgSteerAngle * 57.29578f);
    DrawStr(LBL, fy, live, 0.3f, 120, 255, 140);
    char raw[200];
    const Dev &rd = (g_pedals.ok ? g_pedals : g_wheel);
    snprintf(raw, sizeof raw, "Raw  X %ld  Y %ld  Z %ld  RX %ld  RY %ld  RZ %ld  S0 %ld  S1 %ld",
             rd.st.lX, rd.st.lY, rd.st.lZ, rd.st.lRx, rd.st.lRy, rd.st.lRz, rd.st.rglSlider[0], rd.st.rglSlider[1]);
    DrawStr(LBL, fy + 0.027f, raw, 0.26f, 130, 200, 255);
    {
        char ff[200];
        bool lost = GetTickCount64() - g_lastLostMs < 3000 && g_lastLostMs != 0;
        snprintf(ff, sizeof ff, "FFB link: %s    game force %+.2f   friction/damping %+.2f   sent to wheel %+.2f",
                 lost ? "LOST (wheel refused the force)" : "ok", g_dbgForce, g_dbgSoft, g_dbgCmd);
        DrawStr(LBL, fy + 0.078f, ff, 0.24f, lost ? 255 : 150, lost ? 110 : 255, lost ? 110 : 150);
    }
    if (g_learn >= 0)
        DrawStr(LBL, fy + 0.052f, ThS(g_rows[g_learn].prompt) + ThS("   (Esc = cancel)"), 0.3f, 255, 130, 130);
    else if (g_edit >= 0)
        DrawStr(LBL, fy + 0.052f, "Type a value, Enter = OK, Esc = cancel", 0.3f, 255, 220, 120);
    else if (GetTickCount64() < g_calibMsgUntil)
        DrawStr(LBL, fy + 0.052f, g_calibMsg, 0.28f, 255, 220, 120);
    else
        DrawStr(LBL, fy + 0.052f, "Drag the sliders or click a % box to type. Click a bind button, then use the control.", 0.26f, 190, 195, 205);
    {
        float bx = PX + PW - 0.175f, by = PY + PH - 0.052f;
        bool hv = In(mx, my, bx, by, 0.16f, 0.038f);
        NShape(bx, by, 0.16f, 0.038f, 0.012f, 0, Rgb(60, 230, 160, hv ? 1.0f : 0.85f), Rgb(25, 150, 200, hv ? 1.0f : 0.85f), Rgb(150, 255, 220, 1.0f), 1.8f, true);
        DrawStr(bx + 0.08f, by + 0.0085f, "Save and close", 0.32f, 255, 255, 255, true);
        if (click && hv) { if (g_edit >= 0) CommitEdit(); SaveConfig(); g_menuOpen = false; }
        NCursor(mx, my);
    }
}

// ---------- rolling telemetry: press the mark key/button when something feels wrong, the last ~25 s are saved ----------
struct Sample { ULONGLONG ms; float speed, rpm, rawThr, thr, brk, clutch, clutchOut, steer, clutchGame; int shifter, engaged, nextGame, curGame; bool running; };
static float g_dbgClutchGame;   // what the game itself computed for the clutch, read before the plugin overwrites it
static int g_dbgNext, g_dbgCur; // the game's own next/current gear, read before the plugin overwrites them
static std::vector<Sample> g_ring;
static size_t g_ringHead = 0;
static float g_clutchOut = 1.0f;
static const size_t RING_N = 1500;

static void RingPush(const Sample &s) {
    if (g_ring.size() < RING_N) g_ring.push_back(s);
    else { g_ring[g_ringHead] = s; g_ringHead = (g_ringHead + 1) % RING_N; }
}
static void RingDump() {
    static int n = 0;
    char name[64]; snprintf(name, sizeof name, "WheelPlugin_capture_%d.csv", ++n);
    FILE *f = fopen((g_dir + name).c_str(), "w");
    if (!f) { Log("capture: cannot write %s", name); return; }
    fprintf(f, "seconds_before_mark,speed_ms,rpm,throttle_raw,throttle_after_logic,brake,clutch_pedal,clutch_written,steer,shifter_gear,engaged_gear,engine_running,game_clutch_before_write,game_next_gear_before,game_cur_gear_before\n");
    size_t sz = g_ring.size();
    ULONGLONG last = sz ? g_ring[(g_ringHead + sz - 1) % sz].ms : 0;
    for (size_t k = 0; k < sz; k++) {
        const Sample &s = g_ring[(g_ringHead + k) % sz];
        fprintf(f, "%.2f,%.2f,%.3f,%.2f,%.2f,%.2f,%.2f,%.2f,%.2f,%d,%d,%d,%.3f,%d,%d\n", -(double)(last - s.ms) / 1000.0, s.speed, s.rpm,
                s.rawThr, s.thr, s.brk, s.clutch, s.clutchOut, s.steer, s.shifter, s.engaged, s.running ? 1 : 0,
                s.clutchGame, s.nextGame, s.curGame);
    }
    fclose(f);
    Log("capture saved: %s (%zu frames)", name, sz);
}

// ---------- on-screen status (gear, rpm, pedals) ----------
static int g_hudEngaged;      // gear the plugin actually engaged: 0 = neutral, -1 = reverse

// Gear lever position shared with other mods (MirrorCam reads it to show the back-up camera): -1 = R, 0 = N, 1.. = gear,
// 99 = the manual shifter is not in use. Named shared memory "NakimiTV_GearState".
struct GearShare { volatile LONG magic; volatile LONG engaged; volatile LONG tick; };
static GearShare *g_gearShare;
static void PublishGear(int engaged) {
    if (!g_gearShare) {
        HANDLE h = CreateFileMappingA(INVALID_HANDLE_VALUE, nullptr, PAGE_READWRITE, 0, sizeof(GearShare), "NakimiTV_GearState");
        if (h) g_gearShare = (GearShare *)MapViewOfFile(h, FILE_MAP_ALL_ACCESS, 0, 0, sizeof(GearShare));
    }
    if (g_gearShare) { g_gearShare->engaged = engaged; g_gearShare->tick = (LONG)GetTickCount(); g_gearShare->magic = 0x47454152; }
}
static float g_hudRpm;        // 0..1
static bool g_hudShow;

static bool g_hudClutchMsg;                 // "press the clutch" hint, drawn by DrawHud() in the status slot
static void DrawHudShifter() {
    g_hudClutchMsg = false;
    if (!g_hudShow) return;
    const float bx = 0.80f, by = 0.86f, bw = 0.17f;
    // rpm bar with redline mark
    Rc(bx, by + 0.045f, bw, 0.012f, 0, 0, 0, 150);
    float r = std::clamp(g_hudRpm, 0.0f, 1.0f);
    Rc(bx, by + 0.045f, bw * r, 0.012f, r > 0.85f ? 255 : 255, r > 0.85f ? 80 : 255, r > 0.85f ? 0 : 255, 255);
    Rc(bx + bw * 0.85f, by + 0.043f, 0.002f, 0.016f, 255, 80, 0, 255);
    // gear + status
    std::string gs = g_hudEngaged == 0 ? "N" : (g_hudEngaged < 0 ? "R" : std::to_string(g_hudEngaged));
    DrawStr(bx + 0.02f, by - 0.035f, gs, 0.9f, 255, 255, 255, true);
    if (g_liveGear != g_hudEngaged)
        g_hudClutchMsg = g_liveGear != 0 && g_hudEngaged == 0;
    // pedal bars: clutch, brake, throttle
    struct { float v; int r, g, b; } p[3] = {{g_liveClutch, 60, 120, 255}, {g_liveBrk, 255, 60, 60}, {g_liveThr, 60, 220, 90}};
    for (int i = 0; i < 3; i++) {
        float x = bx - 0.05f + i * 0.016f;
        Rc(x, by - 0.03f, 0.012f, 0.07f, 0, 0, 0, 150);
        float h = 0.07f * std::clamp(p[i].v, 0.0f, 1.0f);
        Rc(x, by + 0.04f - h, 0.012f, h, p[i].r, p[i].g, p[i].b, 255);
    }
}

struct WinSearch { DWORD pid; HWND best; long long area; };
// ---------- simulator extras: fuel, engine on/off, dashboard gauges, blocked game keys ----------
// Every part has its own switch in the menu. A part that is switched off is not run at all.
static bool g_noFuel;                       // the car being driven is empty: nothing may start its engine
static std::map<Vehicle, float> g_fuel;     // tank level per vehicle, 0..1
// Read/write access for other plugins (NKGarage saves and restores the tank of parked cars). Found with GetProcAddress.
// GetFuel returns -1 when this vehicle has no tank level yet (never driven since it appeared).
extern "C" __declspec(dllexport) float WheelPlugin_GetFuel(int veh) {
    auto it = g_fuel.find(veh);
    return it == g_fuel.end() ? -1.0f : it->second;
}
extern "C" __declspec(dllexport) void WheelPlugin_SetFuel(int veh, float level) {
    g_fuel[veh] = std::clamp(level, 0.0f, 1.0f);
}
static std::string g_simMsg, g_simMsg2;     // main line and price line near a pump

static bool WheelBtn(int b) { return g_wheel.ok && b >= 0 && b < 128 && (g_wheel.st.rgbButtons[b] & 0x80); }
static bool ShifterActive() { return ((g_trans == 1 && g_shifter.ok) || g_trans == 2) && cfg.manualGears && g_nextGearOff; }

static bool g_dashActive;                   // numbers (speed, fuel) wanted on the HUD this frame
static float g_dashSpeed, g_dashFuel = -1.0f;
static bool g_dashRunning, g_startSuppress;

#include "dash.h"
// the HUD (right side): the neon dashboard, or the plain one (gear, rpm, pedals from the shifter code plus speed and fuel as numbers)
static void DrawHud() {
    if (cfg.hudNeon && g_d3dState >= 0) { DrawDashNeon(); return; }
    DrawHudShifter();
    if (!g_dashActive) return;
    {   // dark plate behind the numbers so they read on any background
        const float px0 = g_hudShow ? 0.80f - 0.062f : 0.80f - 0.01f;
        Rc(px0, 0.86f - 0.108f, 0.80f + 0.184f - px0, 0.226f, 0, 0, 0, 130);
    }
    const float cx = g_hudShow ? 0.80f + 0.115f : 0.80f + 0.087f;     // centre of the speed / status column
    const float bx = 0.80f, by = 0.86f, bw = 0.17f;
    char buf[32];
    if (cfg.hudSpeedo) {
        float v = cfg.mph ? g_dashSpeed * 2.23694f : g_dashSpeed * 3.6f;
        snprintf(buf, sizeof buf, "%d", (int)(v + 0.5f));
        DrawStr(cx, by - 0.1f, buf, 1.1f, 255, 255, 255, true);
        DrawStr(cx, by - 0.04f, cfg.mph ? "MPH" : "KM/H", 0.3f, 200, 210, 225, true);
    }
    if (g_hudClutchMsg) DrawStr(cx, by + 0.012f, "PRESS CLUTCH", 0.34f, 255, 170, 90, true);
    else if (!g_dashRunning) DrawStr(cx, by + 0.012f, g_noFuel ? "NO FUEL" : "ENGINE OFF", 0.34f, 255, 120, 120, true);
    if (g_dashFuel >= 0.0f && cfg.hudFuel) {
        float f = std::clamp(g_dashFuel, 0.0f, 1.0f);
        bool low = f < 0.15f, hide = f < 0.08f && ((GetTickCount64() / 400) % 2);
        int r = low ? 255 : 90, g = low ? 70 : 220, b = low ? 70 : 110;
        Rc(bx, by + 0.068f, bw, 0.008f, 0, 0, 0, 150);
        Rc(bx, by + 0.068f, bw * f, 0.008f, r, g, b, 255);
        snprintf(buf, sizeof buf, "%d%%", (int)(f * 100.0f + 0.5f));
        if (!hide) {
            DrawStr(bx, by + 0.082f, "FUEL", 0.3f, r, g, b);
            DrawStr(bx + bw - 0.02f, by + 0.082f, buf, 0.34f, r, g, b, true);
        }
    }
}
const int PUMP_MODELS = 7;
static Hash g_pumpHash[PUMP_MODELS];
static void InitPumps() {
    static bool init = false;
    if (init) return;
    const char *n[PUMP_MODELS] = {"prop_gas_pump_1a", "prop_gas_pump_1b", "prop_gas_pump_1c", "prop_gas_pump_1d", "prop_gas_pump_old2", "prop_gas_pump_old3", "prop_vintage_pump"};
    for (int i = 0; i < PUMP_MODELS; i++) g_pumpHash[i] = MISC::GET_HASH_KEY(n[i]);
    init = true;
}
static float g_refuelFrac = -1.0f;          // >= 0 while refuelling
static bool g_refuelling, g_brokeStop;
static float g_refuelLiters, g_refuelCost, g_payAcc;
static int g_cashLeft = -1;

// Story-mode cash of the current character (Michael / Franklin / Trevor). Other models: not readable, refuelling stays free.
static Hash CashStat() {
    static Hash m0 = MISC::GET_HASH_KEY("player_zero"), m1 = MISC::GET_HASH_KEY("player_one"), m2 = MISC::GET_HASH_KEY("player_two");
    static Hash s0 = MISC::GET_HASH_KEY("SP0_TOTAL_CASH"), s1 = MISC::GET_HASH_KEY("SP1_TOTAL_CASH"), s2 = MISC::GET_HASH_KEY("SP2_TOTAL_CASH");
    Hash m = ENTITY::GET_ENTITY_MODEL(PLAYER::PLAYER_PED_ID());
    return m == m0 ? s0 : (m == m1 ? s1 : (m == m2 ? s2 : 0));
}
static bool GetCash(int *c) {
    Hash s = CashStat();
    if (!s) return false;
    int v = 0;
    if (!STATS::STAT_GET_INT(s, &v, -1)) return false;
    *c = v;
    return true;
}
static void SetCash(int c) { if (Hash s = CashStat()) STATS::STAT_SET_INT(s, std::max(c, 0), TRUE); }

// Pumps are found among the objects the game has loaded around the player and remembered in WheelPlugin_pumps.txt,
// so the map fills up as you drive. One map icon per station, 3D markers on the pumps close by.
static std::vector<Vector3> g_pumps;
static std::vector<Vector3> g_stations;   // one entry per station (pumps within 40 m count as one)
static std::vector<Blip> g_blips;          // g_blips[i] = map icon of g_stations[i], 0 while it is not shown
static size_t g_blipPumps = (size_t)-1;
static bool g_pumpsLoaded;
static float Dist2(const Vector3 &a, const Vector3 &b) { float dx = a.x - b.x, dy = a.y - b.y, dz = a.z - b.z; return dx * dx + dy * dy + dz * dz; }
static void RemoveStationBlips() {
    for (Blip bl : g_blips) if (bl && HUD::DOES_BLIP_EXIST(bl)) HUD::REMOVE_BLIP(&bl);
    g_blips.clear();
    g_stations.clear();
    g_blipPumps = (size_t)-1;
}
static void StationTick(bool inVeh, float fuelLevel) {
    bool want = cfg.fuelOn && (cfg.fuelBlips || cfg.fuelMarkers);
    if (!g_pumpsLoaded) {
        g_pumpsLoaded = true;
        if (FILE *pf = fopen((g_dir + "WheelPlugin_pumps.txt").c_str(), "r")) {
            float x, y, z;
            while (fscanf(pf, "%f %f %f", &x, &y, &z) == 3) { Vector3 v{}; v.x = x; v.y = y; v.z = z; g_pumps.push_back(v); }
            fclose(pf);
            Log("gas pumps remembered: %zu", g_pumps.size());
        }
    }
    if (!want) { if (!g_blips.empty()) RemoveStationBlips(); return; }
    InitPumps();
    static ULONGLONG lastScan = 0;
    ULONGLONG nowMs = GetTickCount64();
    if (nowMs - lastScan > 1000) {
        lastScan = nowMs;
        static int objs[1024];
        int n = worldGetAllObjects(objs, 1024);
        for (int i = 0; i < n; i++) {
            Hash m = ENTITY::GET_ENTITY_MODEL(objs[i]);
            bool pump = false;
            for (int k = 0; k < PUMP_MODELS; k++) if (m == g_pumpHash[k]) { pump = true; break; }
            if (!pump) continue;
            Vector3 p = ENTITY::GET_ENTITY_COORDS(objs[i], TRUE);
            bool known = false;
            for (auto &q : g_pumps) if (Dist2(p, q) < 2.25f) { known = true; break; }
            if (known) continue;
            g_pumps.push_back(p);
            if (FILE *pf = fopen((g_dir + "WheelPlugin_pumps.txt").c_str(), "a")) { fprintf(pf, "%.2f %.2f %.2f\n", p.x, p.y, p.z); fclose(pf); }
        }
    }
    if (cfg.fuelBlips) {
        if (g_blipPumps != g_pumps.size()) {
            for (size_t i = g_blipPumps == (size_t)-1 ? 0 : g_blipPumps; i < g_pumps.size(); i++) {
                bool closeBy = false;
                for (auto &q : g_stations) if (Dist2(g_pumps[i], q) < 40.0f * 40.0f) { closeBy = true; break; }
                if (closeBy) continue;
                g_stations.push_back(g_pumps[i]);
                g_blips.push_back(0);
            }
            g_blipPumps = g_pumps.size();
        }
        // a station's map icon exists only while the player is within 100 m of it (removed again beyond 110 m so it does not flicker)
        static ULONGLONG lastBlip = 0;
        if (nowMs - lastBlip > 500) {
            lastBlip = nowMs;
            Vector3 me = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE);
            for (size_t i = 0; i < g_stations.size(); i++) {
                float d2 = Dist2(me, g_stations[i]);
                if (g_blips[i] && !HUD::DOES_BLIP_EXIST(g_blips[i])) g_blips[i] = 0;
                if (!g_blips[i] && d2 < 100.0f * 100.0f) {
                    Blip bl = HUD::ADD_BLIP_FOR_COORD(g_stations[i]);
                    HUD::SET_BLIP_SPRITE(bl, 361);
                    HUD::SET_BLIP_COLOUR(bl, 2);
                    HUD::SET_BLIP_SCALE(bl, 0.8f);
                    HUD::SET_BLIP_AS_SHORT_RANGE(bl, FALSE);
                    HUD::BEGIN_TEXT_COMMAND_SET_BLIP_NAME("STRING");
                    HUD::ADD_TEXT_COMPONENT_SUBSTRING_PLAYER_NAME("Gas station");
                    HUD::END_TEXT_COMMAND_SET_BLIP_NAME(bl);
                    g_blips[i] = bl;
                } else if (g_blips[i] && d2 > 110.0f * 110.0f) {
                    Blip bl = g_blips[i];
                    HUD::REMOVE_BLIP(&bl);
                    g_blips[i] = 0;
                }
            }
        }
    } else if (!g_blips.empty()) RemoveStationBlips();

    if (cfg.fuelMarkers && inVeh && !g_menuOpen) {
        Vector3 me = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE);
        bool low = fuelLevel >= 0.0f && fuelLevel < 0.25f;
        int cr = low ? 255 : 90, cg = low ? 170 : 220, cb = low ? 40 : 110;
        Vector3 zero{}, ringSz{}, arrowSz{};
        ringSz.x = ringSz.y = 1.6f; ringSz.z = 0.5f;
        arrowSz.x = arrowSz.y = arrowSz.z = 0.6f;
        const Vector3 *nearest = nullptr; float best = 70.0f * 70.0f;
        for (auto &p : g_pumps) {
            float d = Dist2(me, p);
            if (d > 45.0f * 45.0f) continue;
            Vector3 g = p; g.z -= 0.95f;
            GRAPHICS::DRAW_MARKER(1, g, zero, zero, ringSz, cr, cg, cb, 90, FALSE, FALSE, 2, FALSE, nullptr, nullptr, FALSE);
            if (d < best) { best = d; nearest = &p; }
        }
        if (!nearest) for (auto &p : g_pumps) { float d = Dist2(me, p); if (d < best) { best = d; nearest = &p; } }
        if (nearest && best < 70.0f * 70.0f) {
            Vector3 a = *nearest; a.z += 2.3f;
            GRAPHICS::DRAW_MARKER(2, a, zero, zero, arrowSz, cr, cg, cb, 200, TRUE, FALSE, 2, TRUE, nullptr, nullptr, FALSE);
        }
    }
}

// ---------- steering log ----------
// Every 50 ms while driving: your real rim angle, what the plugin wrote, what the game holds, the wheel angle the game reports and
// how fast the car actually turns. With a few minutes of it (standing still full lock, slow sweeps, 60 and 100 km/h) the real steering
// ratio and any speed limiter of the game can be read off the numbers.
static void SteerLog(Vehicle veh, float handFrac, float steerValue, BYTE *sp) {
    static FILE *f = nullptr;
    static ULONGLONG t0 = 0, last = 0;
    static int rows = 0;
    if (!cfg.steerLog) { if (f) { fclose(f); f = nullptr; } return; }
    ULONGLONG now = GetTickCount64();
    if (!f) {
        if (rows >= 40000) return;
        f = fopen((g_dir + "WheelPlugin_steering.csv").c_str(), rows ? "a" : "w");
        if (!f) return;
        if (!rows) { fprintf(f, "seconds,model,speed_kmh,rim_deg,steer_value,input_in_game,angle_field_deg,yaw_rate_deg_s,wheel_range,steer_lock\n"); t0 = now; }
    }
    if (now - last < 50) return;
    last = now;
    Vector3 rot = ENTITY::GET_ENTITY_ROTATION_VELOCITY(veh);
    float inGame = *(float *)(sp + g_steerInputOff) * g_steerSign;
    fprintf(f, "%.2f,%X,%.1f,%.1f,%.3f,%.3f,%.2f,%.1f,%.0f,%.0f\n", (double)(now - t0) / 1000.0, (unsigned)ENTITY::GET_ENTITY_MODEL(veh),
            ENTITY::GET_ENTITY_SPEED(veh) * 3.6f, handFrac * cfg.wheelRange * 0.5f, steerValue, inGame,
            g_dbgSteerAngle * 57.29578f, rot.z * 57.29578f, cfg.wheelRange, cfg.steerLock);
    if (++rows % 10 == 0) fflush(f);
}

// ---------- fitment: wheel track width, ride height and camber ----------
// Every wheel of the car has a record in the game; its position relative to the body (x = sideways, z = up) is three floats in a row. The
// four wheel records and the place of that position inside a record are found by comparing with where the wheel bones (wheel_lf / rf /
// lr / rr) really are, so a wrong layout is never touched. Track width widens the wheel sideways, ride height moves the wheel down (car up)
// or up (car down); both are an offset on the first values seen. Camber tilts the wheel bones in the skeleton (what is drawn): it needs only
// the bones, not the wheel records. Settings are kept per car (model + plate) or per model.
struct FitCar {
    uint64_t arr; uint64_t w[4]; float bx[4], by[4], bz[4]; int posOff; bool ok, active;     // wheel records
    float b2x[4], b2z[4], cam0[4], h0, h1; bool has2, tActive;   // first values seen: x offset (+0x30 of a record), camber (+0x08), visual height (+0x7C / +0x80 of the wheel array)
    int bone[4]; bool bonesOk, camberActive; float pose[4][16], last[4][16]; bool haveLast[4];   // wheel bones (camber)
    float seen[FIT_N]; bool seenOk;   // the values applied last frame, to notice a change
};
static std::map<Vehicle, FitCar> g_fitCars;
static std::string g_fitMsg;
static ULONGLONG g_fitMsgUntil;
static void FitNote(const char *en) { g_fitMsg = ThS(en); g_fitMsgUntil = GetTickCount64() + 3500; }
static Vehicle FitDriverVehicle() {
    Ped ped = PLAYER::PLAYER_PED_ID();
    Vehicle v = PED::IS_PED_IN_ANY_VEHICLE(ped, FALSE) ? PED::GET_VEHICLE_PED_IS_IN(ped, FALSE) : 0;
    return (v && VEHICLE::GET_PED_IN_VEHICLE_SEAT(v, -1, 0) == ped) ? v : 0;
}
// Finds the pointer to the four wheel records inside the vehicle object, and where in a wheel record its position (x, y, z as three floats
// in a row) is kept, by looking for the x / y of the real wheel bones. One candidate per call so a bad pointer only skips that candidate.
static int g_fitScanInfo[3];   // diagnostics of the last scan: arrays looked at, arrays whose first wheel matched somewhere, best wheels matched
static bool FitTryArray(uint64_t base, int off, const float ex[4][3], FitCar *c, int *seen, int *firstHit, int *bestMatched) {
    __try {
        uint64_t q = *(uint64_t *)(base + off);
        if (q < 0x10000ull || q > 0x7FFFFFFFFFFFull || (q & 7)) return false;
        uint64_t w0 = *(uint64_t *)q;
        if (w0 < 0x10000ull || w0 > 0x7FFFFFFFFFFFull || (w0 & 7)) return false;
        (*seen)++;
        bool hit = false;
        for (int po = 0; po < 0x400; po += 4) {
            float x0 = *(float *)(w0 + po), y0 = *(float *)(w0 + po + 4);
            bool any = false;
            for (int e = 0; e < 4 && !any; e++) any = fabsf(x0 - ex[e][0]) + fabsf(y0 - ex[e][1]) < 0.12f;
            if (!any) continue;
            hit = true;
            int used = 0, matched = 0;
            FitCar t = {};
            for (int i = 0; i < 4; i++) {
                uint64_t w = *(uint64_t *)(q + 8 * (uint64_t)i);
                if (w < 0x10000ull || w > 0x7FFFFFFFFFFFull || (w & 7)) break;
                float x = *(float *)(w + po), y = *(float *)(w + po + 4), z = *(float *)(w + po + 8);
                int best = -1;
                for (int e = 0; e < 4; e++) if (!(used & (1 << e)) && fabsf(x - ex[e][0]) + fabsf(y - ex[e][1]) < 0.12f) { best = e; break; }
                if (best < 0) break;
                used |= 1 << best;
                matched++;
                t.w[i] = w; t.bx[i] = x; t.by[i] = y; t.bz[i] = z;
            }
            if (matched > *bestMatched) *bestMatched = matched;
            if (matched == 4 && used == 15) { t.arr = q; t.posOff = po; t.ok = true; *c = t; (*firstHit)++; return true; }
        }
        if (hit) (*firstHit)++;
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    return false;
}
static bool FitScan(uint64_t base, const float ex[4][3], FitCar *c) {
    int seen = 0, firstHit = 0, best = 0;
    for (int off = 0x400; off < 0x2000; off += 8)
        if (FitTryArray(base, off, ex, c, &seen, &firstHit, &best)) { g_fitScanInfo[0] = seen; g_fitScanInfo[1] = firstHit; g_fitScanInfo[2] = best; return true; }
    g_fitScanInfo[0] = seen; g_fitScanInfo[1] = firstHit; g_fitScanInfo[2] = best;
    return false;
}
static const char *FIT_BONES[4] = {"wheel_lf", "wheel_rf", "wheel_lr", "wheel_rr"};
// First values seen of what is changed. Layout (same as FiveM's SET_VEHICLE_WHEEL_X_OFFSET / _Y_ROTATION / _SUSPENSION_HEIGHT): the x offset is at
// +0x30 of a wheel record, the camber at +0x08 (and minus that at +0x10), the visual height at +0x7C of the wheel array. The x offset counts only
// when it equals the position already found (else this is some other data and is left alone).
static void FitReadCopy(FitCar *c) {
    __try {
        c->has2 = true;
        for (int i = 0; i < 4; i++) {
            c->b2x[i] = *(float *)(c->w[i] + 0x30);
            c->b2z[i] = *(float *)(c->w[i] + 0x38);
            c->cam0[i] = *(float *)(c->w[i] + 0x08);
            if (fabsf(c->b2x[i] - c->bx[i]) > 0.05f || !std::isfinite(c->cam0[i]) || fabsf(c->cam0[i]) > 0.5f) c->has2 = false;
        }
        c->h0 = *(float *)(c->arr + 0x7C);
        c->h1 = *(float *)(c->arr + 0x80);
        if (!std::isfinite(c->h0) || fabsf(c->h0) > 1.0f) c->has2 = false;
    } __except (EXCEPTION_EXECUTE_HANDLER) { c->has2 = false; }
}
static bool FitFind(Vehicle veh, FitCar *c) {
    float ex[4][3];
    for (int i = 0; i < 4; i++) {
        int b = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(veh, FIT_BONES[i]);
        if (b < 0) { Log("fitment: vehicle %d has no bone '%s': not a four wheel car, skipped", veh, FIT_BONES[i]); return false; }
        Vector3 wp = ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(veh, b);
        Vector3 lp = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(veh, wp);
        ex[i][0] = lp.x; ex[i][1] = lp.y; ex[i][2] = lp.z;
    }
    BYTE *p = getScriptHandleBaseAddress(veh);
    if (!p) return false;
    bool ok = FitScan((uint64_t)p, ex, c);
    if (ok) FitReadCopy(c);
    if (ok) Log("fitment: wheel bones at (%.2f %.2f %.2f) (%.2f %.2f %.2f) (%.2f %.2f %.2f) (%.2f %.2f %.2f); wheel records at +0x%llX of the vehicle, position at +0x%X of a record",
                ex[0][0], ex[0][1], ex[0][2], ex[1][0], ex[1][1], ex[1][2], ex[2][0], ex[2][1], ex[2][2], ex[3][0], ex[3][1], ex[3][2],
                (unsigned long long)c->arr, c->posOff);
    else Log("fitment: scan failed. wheel bones at (%.2f %.2f %.2f) (%.2f %.2f %.2f) (%.2f %.2f %.2f) (%.2f %.2f %.2f); pointer arrays looked at %d, with a first-wheel match %d, best wheels matched %d of 4",
             ex[0][0], ex[0][1], ex[0][2], ex[1][0], ex[1][1], ex[1][2], ex[2][0], ex[2][1], ex[2][2], ex[3][0], ex[3][1], ex[3][2],
             g_fitScanInfo[0], g_fitScanInfo[1], g_fitScanInfo[2]);
    return ok;
}
static void FitApply(const FitCar &c, const FitVals &fv) {
    if (!c.has2) return;
    __try {
        // The game has one car-wide height (+0x7C), which cannot differ between the axles, so it is left at its first value. Every wheel
        // carries its own axle's slider in its own z offset (+0x38) with the sign flipped (a positive offset lowers the car): front and rear are independent. Negative slider = lower.
        for (int i = 0; i < 4; i++) {
            bool front = c.by[i] > 0.0f;
            float track = (front ? fv.f[0] : fv.f[1]) * 0.01f, resid = -(front ? fv.f[2] : fv.f[3]) * 0.01f;
            *(float *)(c.w[i] + 0x30) = c.b2x[i] + (c.bx[i] >= 0.0f ? 1.0f : -1.0f) * track;
            *(float *)(c.w[i] + 0x38) = c.b2z[i] + resid;
        }
        *(float *)(c.arr + 0x7C) = c.h0;
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
}

// Camber: written next to the first value seen (+0x08 of a wheel record, and minus that at +0x10), left and right wheels tilt opposite ways.
static void FitTilt(FitCar &c, float frontDeg, float rearDeg) {
    bool want = frontDeg != 0.0f || rearDeg != 0.0f;
    if (!c.has2 || (!want && !c.tActive)) return;
    __try {
        for (int i = 0; i < 4; i++) {
            float deg = want ? (c.by[i] > 0.0f ? frontDeg : rearDeg) : 0.0f;
            if (cfg.fitCamberInv) deg = -deg;
            float v = c.cam0[i] + deg * 0.0174533f * (c.bx[i] < 0.0f ? -1.0f : 1.0f);
            *(float *)(c.w[i] + 0x08) = v;
            *(float *)(c.w[i] + 0x10) = -v;
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
    c.tActive = want;
}

// ---- camber: tilt the wheel bones of the skeleton (vehicle -> frag inst -> cache entry -> skeleton -> object matrices) ----
static bool FitBoneMatrix(Vehicle v, int bone, float **out, int arrOff = 0x18) {
    __try {
        BYTE *p = getScriptHandleBaseAddress(v);
        if (!p) return false;
        uint64_t inst = *(uint64_t *)(p + 0x30);
        if (!inst) return false;
        uint64_t cache = *(uint64_t *)(inst + 0x68);
        if (!cache) return false;
        uint64_t skel = *(uint64_t *)(cache + 0x178);
        if (!skel) return false;
        int n = *(int *)(skel + 0x20);
        uint64_t mats = *(uint64_t *)(skel + arrOff);   // 0x10 and 0x18 both hold the bone matrices; 0x18 is the one the game reports
        if (!mats || bone < 0 || bone >= n || n > 4000) return false;
        *out = (float *)(mats + (uint64_t)bone * 64);
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
// copies a 4x4 matrix out of the skeleton if it looks like a transform (last column 0 0 0 1, finite numbers)
static bool FitReadMat(const float *m, float *out) {
    __try {
        for (int i = 0; i < 16; i++) { out[i] = m[i]; if (!std::isfinite(out[i]) || fabsf(out[i]) > 1e4f) return false; }
        // rotation rows are unit vectors; the position is in the fourth row (the last column is not always 0 0 0 1)
        for (int r = 0; r < 3; r++) { float l = out[r * 4] * out[r * 4] + out[r * 4 + 1] * out[r * 4 + 1] + out[r * 4 + 2] * out[r * 4 + 2]; if (l < 0.8f || l > 1.2f) return false; }
        return true;
    } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
// writes base with its rotation turned about the vehicle's forward (y) axis through the wheel's own position
static void FitWriteMat(float *m, const float *base, float th, float dx) {
    float c = cosf(th), s = sinf(th);
    const float R[3][3] = {{c, 0.0f, -s}, {0.0f, 1.0f, 0.0f}, {s, 0.0f, c}};
    __try {
        for (int r = 0; r < 3; r++) {
            for (int col = 0; col < 3; col++) m[r * 4 + col] = base[r * 4 + 0] * R[0][col] + base[r * 4 + 1] * R[1][col] + base[r * 4 + 2] * R[2][col];
            m[r * 4 + 3] = base[r * 4 + 3];
        }
        for (int i = 12; i < 16; i++) m[i] = base[i];
        m[12] = base[12] + dx;                                              // track width: sideways in the model's x axis
    } __except (EXCEPTION_EXECUTE_HANDLER) {}
}
static void FitCopyMat(float *dst, const float *src) {
    __try { memcpy(dst, src, 64); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}
static bool FitSameMat(const float *a, const float *b) {
    __try { return memcmp(a, b, 64) == 0; } __except (EXCEPTION_EXECUTE_HANDLER) { return false; }
}
// negative camber = the top of the wheel leans in. Relative to the pose the game gives each frame (if the game did not redraw the bone since we
// wrote it, the pose stored last time is used, so the tilt never piles up).
static void FitCamber(FitCar &c, Vehicle v, float frontDeg, float rearDeg, float frontTrack, float rearTrack) {
    if (!c.bonesOk) return;
    bool want = frontDeg != 0.0f || rearDeg != 0.0f || frontTrack != 0.0f || rearTrack != 0.0f;
    if (!want && !c.camberActive) return;
    for (int i = 0; i < 4; i++) {
        float *m = nullptr;
        if (!FitBoneMatrix(v, c.bone[i], &m)) continue;
        float cur[16];
        if (!FitReadMat(m, cur)) continue;
        if (c.haveLast[i] && FitSameMat(cur, c.last[i])) { /* untouched since our write: keep the stored pose */ }
        else memcpy(c.pose[i], cur, 64);
        bool leftSide = (i % 2) == 0;                                       // wheel_lf, wheel_lr are on the left
        bool front = i < 2;
        float deg = want ? (front ? frontDeg : rearDeg) : 0.0f;
        if (cfg.fitCamberInv) deg = -deg;
        float th = deg * 0.0174533f * (leftSide ? -1.0f : 1.0f);
        float dx = want ? (front ? frontTrack : rearTrack) * 0.01f * (leftSide ? -1.0f : 1.0f) : 0.0f;
        FitWriteMat(m, c.pose[i], th, dx);
        float *m2 = nullptr;
        if (FitBoneMatrix(v, c.bone[i], &m2, 0x10)) FitCopyMat(m2, m);
        FitReadMat(m, c.last[i]);
        c.haveLast[i] = true;
    }
    c.camberActive = want;
}

static void FitDumpMem(FILE *f, const char *tag, uint64_t addr, int len) {
    __try {
        for (int o = 0; o < len; o += 4) {
            uint32_t u = *(uint32_t *)(addr + (uint64_t)o);
            float fl;
            memcpy(&fl, &u, 4);
            if (std::isfinite(fl) && fabsf(fl) < 1e5f && (fl == 0.0f || fabsf(fl) > 1e-6f)) fprintf(f, "%s +0x%03X: %.5f  (0x%08X)\n", tag, o, fl, u);
            else fprintf(f, "%s +0x%03X: -  (0x%08X)\n", tag, o, u);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { fprintf(f, "%s: read fault\n", tag); }
}
static void FitDump() {
    Vehicle v = FitDriverVehicle();
    if (!v) { FitNote("Sit in the driver seat first"); return; }
    FitCar c = {};
    if (!FitFind(v, &c)) { FitNote("Wheels not found in this vehicle"); Log("fitment: wheel records not found for vehicle %d (model %X)", v, (unsigned)ENTITY::GET_ENTITY_MODEL(v)); return; }
    FILE *f = fopen((g_dir + "WheelPlugin_wheels.txt").c_str(), "a");
    if (!f) return;
    static int n = 0;
    fprintf(f, "=== dump %d  model %X  wheel array %llX  position at +0x%X\n", ++n, (unsigned)ENTITY::GET_ENTITY_MODEL(v), (unsigned long long)c.arr, c.posOff);
    FitDumpMem(f, "array", c.arr, 0x120);
    for (int i = 0; i < 4; i++) {
        char tag[16];
        snprintf(tag, sizeof tag, "wheel%d", i);
        FitDumpMem(f, tag, c.w[i], 0x240);
    }
    fclose(f);
    FitNote("Wheel data saved");
    Log("fitment: wheel data dump %d written (model %X)", n, (unsigned)ENTITY::GET_ENTITY_MODEL(v));
}
// what a car is called in the saved settings: model hash and license plate
static std::string FitPlateKey(Vehicle v) {
    const char *pt = VEHICLE::GET_VEHICLE_NUMBER_PLATE_TEXT(v);
    std::string p = pt ? pt : "";
    while (!p.empty() && p.back() == ' ') p.pop_back();
    for (char &ch : p) if (ch == ' ' || ch == '=') ch = '_';
    char h[16];
    snprintf(h, sizeof h, "%08X|", (unsigned)ENTITY::GET_ENTITY_MODEL(v));
    return h + p;
}
// the settings of a car: its own (model + plate) when saving per car, else the ones of its model; null = none saved
static const FitVals *FitFor(Vehicle v) {
    if (cfg.fitPerPlate) {
        auto it = g_fitPlates.find(FitPlateKey(v));
        if (it != g_fitPlates.end()) return &it->second;
    }
    auto m = g_fitModels.find(ENTITY::GET_ENTITY_MODEL(v));
    return m != g_fitModels.end() ? &m->second : nullptr;
}
static bool FitAny(const FitVals *fv) {
    if (!fv) return false;
    for (int i = 0; i < FIT_N; i++) if (fv->f[i] != 0.0f) return true;
    return false;
}
static void FitReset() {
    Vehicle v = FitDriverVehicle();
    if (!v) { FitNote("Sit in the driver seat first"); return; }
    std::string ini = g_dir + "WheelPlugin.ini";
    if (cfg.fitPerPlate) {
        std::string k = FitPlateKey(v);
        g_fitPlates.erase(k);
        WritePrivateProfileStringA("FitmentPlates", k.c_str(), nullptr, ini.c_str());
    } else {
        char k[16];
        snprintf(k, sizeof k, "%08X", (unsigned)ENTITY::GET_ENTITY_MODEL(v));
        g_fitModels.erase(ENTITY::GET_ENTITY_MODEL(v));
        WritePrivateProfileStringA("FitmentModels", k, nullptr, ini.c_str());
    }
    for (int i = 0; i < FIT_N; i++) g_fitUi[i] = 0.0f;
    FitNote("Fitment reset");
}
// "a,b,c,d,e,f" (older files have four numbers: the camber values are 0)
static bool FitParse(const char *s, FitVals *fv) {
    float a[FIT_N] = {};
    int n = sscanf(s, "%f,%f,%f,%f,%f,%f", &a[0], &a[1], &a[2], &a[3], &a[4], &a[5]);
    if (n < 4) return false;
    for (int i = 0; i < FIT_N; i++) fv->f[i] = a[i];
    return true;
}
static void FitFormat(const FitVals &fv, char *out, size_t n) {
    snprintf(out, n, "%.2f,%.2f,%.2f,%.2f,%.2f,%.2f", fv.f[0], fv.f[1], fv.f[2], fv.f[3], fv.f[4], fv.f[5]);
}
static void FitLoad() {
    static char buf[16384];
    std::string ini = g_dir + "WheelPlugin.ini";
    memset(buf, 0, sizeof buf);
    GetPrivateProfileSectionA("FitmentModels", buf, sizeof buf, ini.c_str());
    for (const char *p = buf; *p; p += strlen(p) + 1) {
        unsigned h = 0;
        int used = 0;
        FitVals fv;
        if (sscanf(p, "%x=%n", &h, &used) >= 1 && used > 0 && FitParse(p + used, &fv)) g_fitModels[(Hash)h] = fv;
    }
    memset(buf, 0, sizeof buf);
    GetPrivateProfileSectionA("FitmentPlates", buf, sizeof buf, ini.c_str());
    for (const char *p = buf; *p; p += strlen(p) + 1) {
        const char *eq = strchr(p, '=');
        FitVals fv;
        if (eq && FitParse(eq + 1, &fv)) g_fitPlates[std::string(p, eq)] = fv;
    }
}
static void FitSave() {
    char v[128];
    for (auto &kv : g_fitModels) {
        char k[16];
        snprintf(k, sizeof k, "%08X", (unsigned)kv.first);
        FitFormat(kv.second, v, sizeof v);
        W("FitmentModels", k, v);
    }
    for (auto &kv : g_fitPlates) {
        FitFormat(kv.second, v, sizeof v);
        W("FitmentPlates", kv.first.c_str(), v);
    }
}
// one-off probe (written to the log): which pointers of the skeleton lead to a matrix array whose bone entry equals the wheel's local or world
// position, and which other floats of the wheel record repeat the wheel's x / y. Finds what the game really draws from.
static void FitProbe(Vehicle v, const FitCar &c, int bone, const float lp[3], const float wp[3], char *out, int n) {
    int len = 0;
    out[0] = 0;
    __try {
        BYTE *p = getScriptHandleBaseAddress(v);
        uint64_t inst = *(uint64_t *)(p + 0x30), cache = *(uint64_t *)(inst + 0x68), skel = *(uint64_t *)(cache + 0x178);
        len += snprintf(out + len, n - len, "skeleton %llX: ", (unsigned long long)skel);
        for (int off = 0; off < 0x100 && len < n - 120; off += 8) {
            uint64_t q = *(uint64_t *)(skel + off);
            if (q < 0x10000ull || q > 0x7FFFFFFFFFFFull || (q & 7)) continue;
            const float *m = (const float *)(q + (uint64_t)bone * 64);
            for (int pos = 0; pos < 16; pos += 4) {
                bool isLocal = fabsf(m[pos] - lp[0]) < 0.15f && fabsf(m[pos + 1] - lp[1]) < 0.06f && fabsf(m[pos + 2] - lp[2]) < 0.2f;
                bool isWorld = fabsf(m[pos] - wp[0]) < 0.15f && fabsf(m[pos + 1] - wp[1]) < 0.06f && fabsf(m[pos + 2] - wp[2]) < 0.2f;
                if (isLocal || isWorld) len += snprintf(out + len, n - len, "[+0x%X pos@%d %s] ", off, pos, isWorld ? "world" : "local");
            }
        }
        len += snprintf(out + len, n - len, "| record x/y repeats: ");
        for (int o = 0; o < 0x300 && len < n - 40; o += 4) {
            if (o == c.posOff) continue;
            float a = *(float *)(c.w[0] + o), b = *(float *)(c.w[0] + o + 4);
            if (fabsf(a - lp[0]) < 0.03f && fabsf(b - lp[1]) < 0.03f) len += snprintf(out + len, n - len, "+0x%X ", o);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) { snprintf(out + len, n - len, " (read fault)"); }
}
static void FitBaseline(const FitCar &c) {   // the first 0x180 bytes of the front and rear left wheel record, before anything was changed
    static char b[1800];
    for (int k = 0; k < 2; k++) {
        int len = 0;
        __try {
            for (int o = 0; o < 0x180 && len < (int)sizeof b - 24; o += 4) {
                float f = *(float *)(c.w[k * 2] + o);
                if (std::isfinite(f) && fabsf(f) < 1e4f && fabsf(f) > 1e-6f) len += snprintf(b + len, sizeof b - len, "%X=%.4g ", o, f);
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {}
        b[len] = 0;
        Log("fitment baseline %s wheel record: %s", k ? "rear" : "front", b);
    }
}
static void FitReadRec(uint64_t p, float *x, float *z) {
    __try { *x = *(float *)p; *z = *(float *)(p + 8); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}
// what the game holds right now for the ride height: the one car-wide height (+0x7C of the record array) and each wheel's z offset (+0x38)
static void FitReadHeights(const FitCar &c, float *h, float *z4) {
    __try { *h = *(float *)(c.arr + 0x7C); for (int i = 0; i < 4; i++) z4[i] = *(float *)(c.w[i] + 0x38); } __except (EXCEPTION_EXECUTE_HANDLER) {}
}
static void FitmentTick() {
    ULONGLONG now = GetTickCount64();
    if (now < g_fitMsgUntil && !g_fitMsg.empty()) DrawStr(0.5f, 0.14f, g_fitMsg, 0.5f, 150, 235, 255, true);
    if (!cfg.fitOn) {                                  // switched off: wheels back to the first values seen, then nothing runs
        for (auto &kv : g_fitCars) {
            if (!ENTITY::DOES_ENTITY_EXIST(kv.first)) continue;
            if (kv.second.ok && kv.second.active) FitApply(kv.second, FitVals{});
            FitTilt(kv.second, 0.0f, 0.0f);
        }
        g_fitCars.clear();
        return;
    }
    Vehicle dv = FitDriverVehicle();
    // a car gets its saved fitment as soon as it is driven, or seen within about 80 m (like VStancer)
    auto Track = [&](Vehicle v) {
        FitCar c = {};
        c.bonesOk = true;
        for (int i = 0; i < 4; i++) { c.bone[i] = ENTITY::GET_ENTITY_BONE_INDEX_BY_NAME(v, FIT_BONES[i]); if (c.bone[i] < 0) c.bonesOk = false; }
        FitCar r = {};
        if (FitFind(v, &r)) { memcpy(c.w, r.w, sizeof c.w); memcpy(c.bx, r.bx, sizeof c.bx); memcpy(c.by, r.by, sizeof c.by); memcpy(c.bz, r.bz, sizeof c.bz); memcpy(c.b2x, r.b2x, sizeof c.b2x); memcpy(c.cam0, r.cam0, sizeof c.cam0); memcpy(c.b2z, r.b2z, sizeof c.b2z); c.h0 = r.h0; c.h1 = r.h1; c.has2 = r.has2; c.arr = r.arr; c.posOff = r.posOff; c.ok = true; }
        else if (v == dv) FitNote("Wheels not found in this vehicle");
        if (c.ok) FitBaseline(c);
        Log("fitment: vehicle %d (%s): wheel records %s, wheel bones %s", v, FitPlateKey(v).c_str(), c.ok ? "found" : "NOT found", c.bonesOk ? "found" : "NOT found");
        g_fitCars[v] = c;
    };
    if (dv && !g_fitCars.count(dv)) Track(dv);
    static ULONGLONG lastScan = 0;
    if (now - lastScan > 1000) {
        lastScan = now;
        static int vs[128];
        int n = worldGetAllVehicles(vs, 128);
        Vector3 me = ENTITY::GET_ENTITY_COORDS(PLAYER::PLAYER_PED_ID(), TRUE);
        for (int i = 0; i < n && g_fitCars.size() < 24; i++) {
            Vehicle v = vs[i];
            if (g_fitCars.count(v) || !FitAny(FitFor(v))) continue;
            Vector3 p = ENTITY::GET_ENTITY_COORDS(v, TRUE);
            float dx = p.x - me.x, dy = p.y - me.y, dz = p.z - me.z;
            if (dx * dx + dy * dy + dz * dz < 80.0f * 80.0f) Track(v);
        }
    }
    if (dv) {                                          // menu sliders <-> the saved values of the car being driven
        static float prevUi[FIT_N];
        static std::string lastKey;
        std::string key = cfg.fitPerPlate ? FitPlateKey(dv) : std::to_string((unsigned)ENTITY::GET_ENTITY_MODEL(dv));
        const FitVals *eff = FitFor(dv);
        bool edited = false;
        for (int i = 0; i < FIT_N; i++) if (key == lastKey && g_fitUi[i] != prevUi[i]) edited = true;
        if (edited) {
            FitVals base = eff ? *eff : FitVals{};
            Hash model = ENTITY::GET_ENTITY_MODEL(dv);
            FitVals &dst = cfg.fitPerPlate ? (g_fitPlates.count(key) ? g_fitPlates[key] : (g_fitPlates[key] = base))
                                           : (g_fitModels.count(model) ? g_fitModels[model] : (g_fitModels[model] = base));
            for (int i = 0; i < FIT_N; i++) { dst.f[i] = g_fitUi[i]; prevUi[i] = g_fitUi[i]; }
        } else {
            lastKey = key;
            for (int i = 0; i < FIT_N; i++) g_fitUi[i] = prevUi[i] = eff ? eff->f[i] : 0.0f;
        }
    }
    for (auto it = g_fitCars.begin(); it != g_fitCars.end();) {
        if (!ENTITY::DOES_ENTITY_EXIST(it->first)) { it = g_fitCars.erase(it); continue; }
        FitCar &c = it->second;
        const FitVals *fv = FitFor(it->first);
        bool any = FitAny(fv);
        if (c.ok && c.has2 && (any || c.active)) { FitApply(c, any ? *fv : FitVals{}); c.active = any; }
        if (c.ok && c.has2) {
            bool changed = false;
            for (int k = 0; k < FIT_N; k++) {
                float cur = any ? fv->f[k] : 0.0f;
                if ((c.seenOk || cur != 0.0f) && cur != c.seen[k]) changed = true;
                c.seen[k] = cur;
            }
            c.seenOk = true;
            if (changed) {   // wake the physics so the body settles on the new wheel positions right away
                PHYSICS::ACTIVATE_PHYSICS(it->first);
                if (ENTITY::GET_ENTITY_SPEED(it->first) < 1.0f) ENTITY::SET_ENTITY_VELOCITY(it->first, V3(0.0f, 0.0f, 0.05f));
            }
        }
        FitTilt(c, any ? fv->f[4] : 0.0f, any ? fv->f[5] : 0.0f);
        static bool probed = false;
        if (it->first == dv && c.bonesOk && c.ok && !probed) {
            probed = true;
            Vector3 wpv = ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(it->first, c.bone[0]);
            float lp[3] = {c.bx[0], c.by[0], c.bz[0]}, wp[3] = {wpv.x, wpv.y, wpv.z};
            static char pb[1500];
            FitProbe(it->first, c, c.bone[0], lp, wp, pb, sizeof pb);
            Log("fitment probe: %s", pb);
        }
        static ULONGLONG lastDiag = 0;
        if (it->first == dv && any && now - lastDiag > 2000) {   // diagnostics: does the game keep our values, do the bones follow
            lastDiag = now;
            float rx = 0, rz = 0, mm[16] = {};
            FitReadRec(c.w[0] + c.posOff, &rx, &rz);
            float *mp = nullptr;
            bool mOk = c.bonesOk && FitBoneMatrix(it->first, c.bone[0], &mp) && FitReadMat(mp, mm);
            Vector3 bp = ENTITY::GET_OFFSET_FROM_ENTITY_GIVEN_WORLD_COORDS(it->first, ENTITY::GET_WORLD_POSITION_OF_ENTITY_BONE(it->first, c.bone[0]));
            {   // height check: values we want vs what the game holds now (it may overwrite them)
                float hh = 0, z4[4] = {};
                FitReadHeights(c, &hh, z4);
                Log("fitment height: sliders front %.1f rear %.1f | car height now %.4f (start %.4f) | wheel z offsets now FL %.4f FR %.4f RL %.4f RR %.4f (start %.4f %.4f %.4f %.4f)",
                    fv->f[2], fv->f[3], hh, c.h0, z4[0], z4[1], z4[2], z4[3], c.b2z[0], c.b2z[1], c.b2z[2], c.b2z[3]);
            }
            Log("fitment diag: wheel0 record x=%.3f z=%.3f (base %.3f %.3f) | bone x=%.3f z=%.3f | skeleton matrix %s (m0 %.2f %.2f %.2f, pos %.2f %.2f %.2f) | ok=%d", rx, rz, c.bx[0], c.bz[0], bp.x, bp.z,
                mOk ? "read" : "NOT readable", mm[0], mm[1], mm[2], mm[12], mm[13], mm[14], (int)c.ok);
        }
        ++it;
    }
}

// camera view: applied when the player gets in or out of a vehicle (and once at start), so the V key still works afterwards
static void CameraTick() {
    static int lastState = -1;
    static ULONGLONG until = 0;
    Ped ped = PLAYER::PLAYER_PED_ID();
    int st = PED::IS_PED_IN_ANY_VEHICLE(ped, FALSE) ? 1 : 0;
    ULONGLONG now = GetTickCount64();
    if (st != lastState) { lastState = st; until = now + 1500; }
    if (now >= until || g_menuOpen) return;
    int want = st ? cfg.camVeh : cfg.camFoot;
    if (want == 0) return;
    int mode = want == 1 ? 4 : cfg.camDist;
    if (st) { if (CAM::GET_FOLLOW_VEHICLE_CAM_VIEW_MODE() != mode) CAM::SET_FOLLOW_VEHICLE_CAM_VIEW_MODE(mode); }
    else if (CAM::GET_FOLLOW_PED_CAM_VIEW_MODE() != mode) CAM::SET_FOLLOW_PED_CAM_VIEW_MODE(mode);
}

static void SimTick(float dt) {
    g_noFuel = false;
    CameraTick();
    g_simMsg.clear(); g_simMsg2.clear();
    g_dashActive = false;
    // blocked game keys: each one has its own switch, off = the key is left alone
    auto Block = [](int id) { for (int g = 0; g < 3; g++) PAD::DISABLE_CONTROL_ACTION(g, id, TRUE); };
    if (cfg.blockF1) Block(288);
    if (cfg.blockF2) Block(289);
    if (cfg.blockF3) Block(170);
    if (cfg.blockF5to8) { Block(166); Block(167); Block(168); Block(169); }
    if (cfg.blockF1 || cfg.blockF2) RECORDING::REPLAY_PREVENT_RECORDING_THIS_FRAME();
    StationTick(PED::IS_PED_IN_ANY_VEHICLE(PLAYER::PLAYER_PED_ID(), FALSE) != 0, g_dashFuel);
    FitmentTick();

    Ped ped = PLAYER::PLAYER_PED_ID();
    Vehicle veh = PED::IS_PED_IN_ANY_VEHICLE(ped, FALSE) ? PED::GET_VEHICLE_PED_IS_IN(ped, FALSE) : 0;
    if (!veh || VEHICLE::GET_PED_IN_VEHICLE_SEAT(veh, -1, 0) != ped) { g_hudShow = false; g_dashFuel = -1.0f; return; }
    if (!cfg.fuelOn && !cfg.engineKey && !cfg.hudSpeedo) return;
    Hash model = ENTITY::GET_ENTITY_MODEL(veh);
    if (!(VEHICLE::IS_THIS_MODEL_A_CAR(model) || VEHICLE::IS_THIS_MODEL_A_BIKE(model) || VEHICLE::IS_THIS_MODEL_A_QUADBIKE(model))) return;

    bool running = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(veh) != 0;
    float speed = ENTITY::GET_ENTITY_SPEED(veh);
    bool fg = GetForegroundWindow() == g_hwnd && !g_menuOpen;

    // one key / wheel button for the engine: a tap while it runs switches it off, holding it while it is off starts it
    {
        static bool prev = false, suppress = false;
        bool held = fg && (KeyDown(cfg.keyStart) || WheelBtn(cfg.startButton));
        if (held && !prev && running && cfg.engineKey) {
            VEHICLE::SET_VEHICLE_ENGINE_ON(veh, FALSE, TRUE, TRUE);
            running = false; suppress = true;          // keep holding does not start it again
        }
        if (!held) suppress = false;
        prev = held; g_startSuppress = suppress;
    }
    // fuel
    float fuelShown = -1.0f;
    float *fuel = nullptr;
    if (cfg.fuelOn) {
        auto it = g_fuel.find(veh);
        if (it == g_fuel.end()) {
            if (g_fuel.size() > 48)
                for (auto p = g_fuel.begin(); p != g_fuel.end();) p = ENTITY::DOES_ENTITY_EXIST(p->first) ? std::next(p) : g_fuel.erase(p);
            it = g_fuel.emplace(veh, cfg.startFuel).first;
        }
        fuel = &it->second;
        if (running) {
            float thrIn = g_wheel.ok ? g_liveThr : PAD::GET_CONTROL_NORMAL(0, 71);
            float perM = 1.0f / (std::max(cfg.fuelRange, 20.0f) * 1000.0f);
            *fuel = std::max(0.0f, *fuel - dt * perM * (1.5f + speed * (0.5f + 1.5f * thrIn)));
        }
        if (*fuel <= 0.0f) {
            g_noFuel = true;
            if (running) { VEHICLE::SET_VEHICLE_ENGINE_ON(veh, FALSE, TRUE, TRUE); running = false; Log("out of fuel: engine cut"); }
        }
        fuelShown = *fuel;
    }

    // starting (hold the start key / button) when the manual box logic does not do it: auto mode, after switching off or refuelling
    if ((cfg.engineKey || cfg.fuelOn) && !ShifterActive()) {
        static float crank = 0.0f;
        bool held = fg && !g_startSuppress && (KeyDown(cfg.keyStart) || WheelBtn(cfg.startButton));
        if (!running && held && !g_noFuel) {
            crank += dt;
            if (crank >= cfg.crankTime) { VEHICLE::SET_VEHICLE_ENGINE_ON(veh, TRUE, FALSE, FALSE); running = true; crank = 0.0f; }
        } else crank = 0.0f;
    }
    // refuelling: stopped next to a pump, engine off, hold the key / button
    g_refuelFrac = -1.0f;
    if (fuel) {
        InitPumps();
        static ULONGLONG lastScan = 0; static bool nearPump = false;
        ULONGLONG nowMs = GetTickCount64();
        if (speed >= 1.0f) nearPump = false;
        else if (nowMs - lastScan > 250) {
            lastScan = nowMs; nearPump = false;
            Vector3 pos = ENTITY::GET_ENTITY_COORDS(veh, TRUE);
            for (int i = 0; i < PUMP_MODELS && !nearPump; i++) nearPump = OBJECT::GET_CLOSEST_OBJECT_OF_TYPE(pos, 6.0f, g_pumpHash[i], FALSE, FALSE, FALSE) != 0;
        }
        if (nearPump) {
            if (*fuel >= 0.999f) g_simMsg = ThS("Tank full");
            else if (running) g_simMsg = ThS("Turn the engine off to refuel");
            else {
                bool held = fg && (KeyDown(cfg.keyRefuel) || WheelBtn(cfg.refuelButton));
                if (held) {
                    PAD::DISABLE_CONTROL_ACTION(0, 75, TRUE);       // the refuel key may also be the exit-vehicle key
                    if (!g_refuelling) { g_refuelling = true; g_refuelLiters = g_refuelCost = g_payAcc = 0.0f; g_brokeStop = false; }
                    float tankL = std::max(cfg.tankLiters, 1.0f), price = std::max(cfg.fuelPrice, 0.01f);
                    float addL = std::min(dt * cfg.refuelRate * tankL, (1.0f - *fuel) * tankL);
                    int cash = 0;
                    bool pay = cfg.fuelPay && GetCash(&cash);
                    g_cashLeft = pay ? cash : -1;
                    if (pay) {
                        float owed = g_payAcc + addL * price, maxOwed = (float)cash + 0.999f;
                        g_brokeStop = false;
                        if (owed > maxOwed) {                       // not enough money: only what it can pay for
                            addL = std::max(0.0f, (maxOwed - g_payAcc) / price);
                            owed = g_payAcc + addL * price;
                            g_brokeStop = true;
                        }
                        int whole = (int)owed;
                        if (whole > 0) { SetCash(cash - whole); g_cashLeft = cash - whole; }
                        g_payAcc = owed - (float)whole;
                    }
                    if (cfg.fuelPay) g_refuelCost += addL * price;
                    g_refuelLiters += addL;
                    *fuel = std::min(1.0f, *fuel + addL / tankL);
                    g_refuelFrac = *fuel;
                } else {
                    std::string k = cfg.keyRefuel > 0 ? KeyName(cfg.keyRefuel) : ThS("the refuel button");
                    g_simMsg = ThF("Hold %s to refuel", k.c_str());
                    if (cfg.fuelPay) { char pr[16]; snprintf(pr, sizeof pr, "%.0f", cfg.fuelPrice); g_simMsg2 = ThF("Price %s baht per liter", pr); }
                }
            }
            fuelShown = *fuel;
        }
    }
    if (g_refuelFrac < 0.0f) g_refuelling = false;
    g_dashSpeed = speed; g_dashRunning = running; g_dashFuel = fuelShown;
    g_dashActive = cfg.hudSpeedo || (fuelShown >= 0.0f && cfg.hudFuel);
    if (!g_menuOpen) {
        if (cfg.hudNeon && g_d3dState >= 0) DashRefuelUi(g_refuelFrac, g_refuelLiters, g_refuelCost, g_cashLeft, g_brokeStop, cfg.fuelPay && g_refuelFrac >= 0.0f, g_simMsg, g_simMsg2);
        else if (g_refuelFrac >= 0.0f) {                 // refuelling panel
            const float px = 0.5f, py = 0.64f, pw = 0.26f, x0 = px - pw / 2 + 0.012f;
            const bool showCost = cfg.fuelPay, showCash = cfg.fuelPay && g_cashLeft >= 0;
            float ph = 0.1f + 0.022f * (1 + (showCost ? 1 : 0) + (showCash ? 1 : 0)) + (g_brokeStop ? 0.03f : 0.0f);
            Rc(px - pw / 2, py, pw, ph, 0, 0, 0, 175);
            Rc(px - pw / 2, py, pw, 0.003f, 90, 220, 110, 255);
            DrawStr(px - 0.045f, py + 0.008f, "Refuelling...", 0.5f, 255, 255, 255, true);
            char pb[32];
            snprintf(pb, sizeof pb, "%d%%", (int)(g_refuelFrac * 100.0f + 0.5f));
            DrawStr(px + pw / 2 - 0.04f, py + 0.012f, pb, 0.5f, 90, 220, 110, true);
            Rc(x0, py + 0.058f, pw - 0.024f, 0.012f, 50, 56, 74, 255);
            Rc(x0, py + 0.058f, (pw - 0.024f) * g_refuelFrac, 0.012f, 90, 220, 110, 255);
            float ry = py + 0.08f;
            auto Line = [&](const char *label, float v, const char *unit, bool dec, int r, int g, int b) {
                char nb[32];
                snprintf(nb, sizeof nb, dec ? "%.1f" : "%.0f", v);
                DrawStr(x0, ry, label, 0.34f, 200, 205, 215);
                DrawStr(px + 0.05f, ry + 0.001f, nb, 0.36f, r, g, b, true);
                DrawStr(px + 0.08f, ry, unit, 0.34f, 200, 205, 215);
                ry += 0.022f;
            };
            Line("Filled", g_refuelLiters, "Liters", true, 255, 255, 255);
            if (showCost) Line("Cost", g_refuelCost, "Baht", false, 255, 220, 120);
            if (showCash) Line("Money left", (float)g_cashLeft, "Baht", false, g_brokeStop ? 255 : 120, g_brokeStop ? 110 : 255, g_brokeStop ? 110 : 160);
            if (g_brokeStop) DrawStr(px, ry + 0.004f, "Out of money! Filled what you could afford.", 0.34f, 255, 110, 110, true);
        } else if (!g_simMsg.empty()) {
            DrawStr(0.5f, 0.775f, g_simMsg, 0.45f, 255, 255, 255, true);
            if (!g_simMsg2.empty()) DrawStr(0.5f, 0.81f, g_simMsg2, 0.34f, 190, 230, 200, true);
        }
    }
}

static BOOL CALLBACK EnumWinCb(HWND h, LPARAM lp) {
    auto *s = (WinSearch *)lp;
    DWORD pid = 0; GetWindowThreadProcessId(h, &pid);
    if (pid != s->pid || !IsWindowVisible(h) || GetWindow(h, GW_OWNER)) return TRUE;
    RECT r{}; GetClientRect(h, &r);
    long long a = (long long)(r.right - r.left) * (r.bottom - r.top);
    char cls[64] = {}; GetClassNameA(h, cls, sizeof cls);
    if (strcmp(cls, "grcWindow") == 0) a += 1000000000LL;          // the stock game window wins when there is one
    if (a > s->area) { s->area = a; s->best = h; }
    return TRUE;
}
static HWND FindGameWindow() {
    WinSearch s{GetCurrentProcessId(), nullptr, 0};
    EnumWindows(EnumWinCb, (LPARAM)&s);
    return s.best;
}

// (re)open every device role from the current names; used at start and when the menu changes a device
static void ReopenAll(HWND hwnd) {
    ReleaseDev(g_wheel); ReleaseDev(g_pedals); ReleaseDev(g_hand); ReleaseDev(g_shifter);
    g_usedGuids.clear();
    RefreshAttached();
    OpenDevice(g_wheel, cfg.wheelName, true, hwnd);
    if (!cfg.pedalName.empty()) OpenDevice(g_pedals, cfg.pedalName, false, hwnd);
    if (!cfg.handName.empty()) OpenDevice(g_hand, cfg.handName, false, hwnd);
    OpenDevice(g_shifter, cfg.shifterName, false, hwnd);
    CreateEffects(g_wheel);
}

// seconds since the previous call (once per script frame)
static float FrameDt() {
    static LARGE_INTEGER freq = {}, last = {};
    if (!freq.QuadPart) QueryPerformanceFrequency(&freq);
    LARGE_INTEGER now; QueryPerformanceCounter(&now);
    float dt = last.QuadPart ? (float)(now.QuadPart - last.QuadPart) / (float)freq.QuadPart : 0.016f;
    last = now;
    return std::clamp(dt, 0.001f, 0.1f);
}

// a new game session starts with a clean slate: old WheelPlugin_capture_*.csv files from earlier runs are removed
static void ClearCaptures() {
    WIN32_FIND_DATAA fd;
    HANDLE h = FindFirstFileA((g_dir + "WheelPlugin_capture_*.csv").c_str(), &fd);
    if (h == INVALID_HANDLE_VALUE) return;
    int n = 0;
    do {
        if (!(fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) && DeleteFileA((g_dir + fd.cFileName).c_str())) n++;
    } while (FindNextFileA(h, &fd));
    FindClose(h);
    Log("removed %d old capture file(s)", n);
}

static void ScriptMain() {
    LoadConfig();
    ClearCaptures();
    HWND hwnd = FindGameWindow();
    if (!hwnd) hwnd = FindWindowA("grcWindow", nullptr);
    if (!hwnd) hwnd = GetForegroundWindow();
    g_hwnd = hwnd;
    {
        std::string l = g_exeName;
        std::transform(l.begin(), l.end(), l.begin(), ::tolower);
        std::string build = "-";
        size_t p = l.find("_b");
        if (p != std::string::npos) { size_t q = p + 2; while (q < l.size() && isdigit((unsigned char)l[q])) q++; if (q > p + 2) build = l.substr(p + 2, q - p - 2); }
        char cls[64] = {}; if (hwnd) GetClassNameA(hwnd, cls, sizeof cls);
        Log("host: %s | FiveM=%d | game build in exe name: %s | exe timestamp %lX | window class: %s",
            g_exeName.c_str(), g_isFiveM ? 1 : 0, build.c_str(), (unsigned long)ExeStamp(), cls);
    }
    BuildRows();
    if (FAILED(DirectInput8Create(g_module, DIRECTINPUT_VERSION, IID_IDirectInput8A, (void **)&g_di, nullptr))) {
        Log("DirectInput8Create failed"); return;
    }
    ReopenAll(hwnd);
    FindGearOffsets();
    FindRpmOffset();
    FindSteerOffset();
    ProbeGearboxPatterns();
    auto PD = []() -> Dev & { return g_pedals.ok ? g_pedals : g_wheel; };
    bool wasIn = false; bool f9 = false; bool testActive = false;

    while (true) {
        WAIT(0);
        ThaiFlush();
        {   // overlay health while the menu is open: ticks vs presents, presents with nothing to draw, textures not ready in time
            static ULONGLONG lt = 0; static int ticks = 0;
            ticks++;
            ULONGLONG nw = GetTickCount64();
            if (g_menuOpen && nw - lt > 5000) {
                if (lt) Log("overlay: %d script ticks, %d presents, %d presents without a fresh list, %d textures late (last 5 s)", ticks, g_dbgPresents.load(), g_dbgEmpty.load(), g_dbgMiss.load());
                lt = nw; ticks = 0; g_dbgPresents = 0; g_dbgEmpty = 0; g_dbgMiss = 0;
            } else if (!g_menuOpen) lt = 0;
        }
        Poll(g_wheel); Poll(g_pedals); Poll(g_hand); Poll(g_shifter);
        bool k = KeyDown(cfg.keyDump);
        if (k && !f9) DumpState();
        f9 = k;
        // raw live values for the menu (independent of vehicle / gear logic)
        if (g_wheel.ok) {
            g_liveSteer = std::clamp(Axis(g_wheel, cfg.steerAxis) * SteerScale() * (cfg.invertSteer ? -1.0f : 1.0f), -1.0f, 1.0f);
            g_liveThr = Pedal(PD(), cfg.throttleAxis, cfg.invertThrottle);
            g_liveBrk = Pedal(PD(), cfg.brakeAxis, cfg.invertBrake);
            g_liveClutch = cfg.clutchAxis.empty() ? 0.0f : Pedal(PD(), cfg.clutchAxis, cfg.invertClutch);
        }
        g_liveGear = 0;
        if (g_shifter.ok)
            for (int i = 0; i <= MAXG; i++)
                if (cfg.gearButtons[i] >= 0 && (g_shifter.st.rgbButtons[cfg.gearButtons[i]] & 0x80)) { g_liveGear = (i == MAXG) ? -1 : i + 1; break; }
        {
            static int savedTrans = -1;
            if (savedTrans < 0) savedTrans = g_trans;
            if (g_trans != savedTrans) { savedTrans = g_trans; W("Shifter", "Mode", TRANS_NAMES[g_trans]); }
        }
        const float frameDt = FrameDt();
        MenuTick();
        SimTick(frameDt);
        DrawHud();
        if (g_reopen) { g_reopen = false; ReopenAll(g_hwnd); wasIn = false; Log("devices reopened from the menu"); }
        if (g_recreateFfb) { g_recreateFfb = false; ReleaseEffects(g_wheel); CreateEffects(g_wheel); wasIn = false; }
        if (!g_wheel.ok) continue;

        if (g_ffbLost) {
            static ULONGLONG lastTry = 0;
            if (GetTickCount64() - lastTry > 250) { lastTry = GetTickCount64(); ReacquireWheel(g_wheel, g_hwnd); }
        }

        // menu "Test force feedback": alternating strong pulses, works on foot as well
        if (GetTickCount64() < g_ffbTestUntil) {
            EnsurePlaying(g_wheel);
            // every effect type at once, strong: if none of them is felt the force never reaches the wheel
            FfbOut t;
            t.constant = ((GetTickCount64() / 400) % 2) ? 0.7f : -0.7f;
            t.friction = 0.6f; t.damper = 0.6f;
            t.engHz = 18.0f; t.engMag = 0.6f;
            SetFfb(g_wheel, t);
            if (!testActive) {
                DWORD sc = 0, sd = 0, se = 0;
                if (g_wheel.constant) g_wheel.constant->GetEffectStatus(&sc);
                if (g_wheel.damper) g_wheel.damper->GetEffectStatus(&sd);
                if (g_wheel.engine) g_wheel.engine->GetEffectStatus(&se);
                DWORD fs = 0; HRESULT fh = g_wheel.dev->GetForceFeedbackState(&fs);
                Log("ffb test started: effect status constant=0x%lX damper=0x%lX engine=0x%lX, device state=0x%lX (hr 0x%lX)",
                    (unsigned long)sc, (unsigned long)sd, (unsigned long)se, (unsigned long)fs, (unsigned long)fh);
            }
            testActive = true;
        } else if (testActive) {
            SetFfb(g_wheel, FfbOut{});
            testActive = false;
        }

        // menu "Detect FFB direction": a short push towards the +axis side; the way the wheel moves is the real direction
        static int calibStage = 0; static ULONGLONG calibT0 = 0; static float calibPos0 = 0;
        {
            // Without knowing which way a positive force pushes this wheel, a software damper would work backwards and
            // shake the wheel. So the direction is measured automatically at the start, and no such force is sent before.
            static ULONGLONG nextAuto = 0;
            if (!cfg.ffbDirKnown && !calibStage && !g_menuOpen && cfg.ffb) {
                if (nextAuto == 0) nextAuto = GetTickCount64() + 4000;
                if (GetTickCount64() >= nextAuto) {
                    Ped p0 = PLAYER::PLAYER_PED_ID();
                    bool slow = true;
                    if (PED::IS_PED_IN_ANY_VEHICLE(p0, FALSE)) slow = ENTITY::GET_ENTITY_SPEED(PED::GET_VEHICLE_PED_IS_IN(p0, FALSE)) < 2.0f;
                    if (slow) { g_calibRequest = true; nextAuto = GetTickCount64() + 9000; }
                    else nextAuto = GetTickCount64() + 2000;
                }
            }
            if (!g_menuOpen && GetTickCount64() < g_calibMsgUntil) DrawStr(0.5f, 0.06f, g_calibMsg, 0.42f, 255, 220, 120, true);
        }
        // Two pushes, one each way. Only a clear move one way and back the other way counts, and only when the device
        // really accepted the force (it refuses it while another program holds the wheel exclusively). A hand on the
        // rim, the car moving the wheel or a refused command can no longer produce a wrong answer.
        static float calibD1 = 0;
        static bool calibRefused = false;
        if (g_calibRequest) {
            g_calibRequest = false; calibStage = 1; calibT0 = GetTickCount64(); calibPos0 = Axis(g_wheel, cfg.steerAxis);
            calibRefused = false;
            g_calibMsg = "FFB direction test: keep your hands off the wheel"; g_calibMsgUntil = GetTickCount64() + 5000;
        }
        if (calibStage) {
            EnsurePlaying(g_wheel);
            g_ffbLost = false;
            FfbOut c; c.constant = calibStage == 1 ? 0.22f : -0.22f; SetFfb(g_wheel, c);
            if (g_ffbLost) calibRefused = true;                         // a command was refused: this run proves nothing
            if (GetTickCount64() - calibT0 > 350) {
                float now = Axis(g_wheel, cfg.steerAxis);
                if (calibStage == 1) {                                  // first half done: remember, then push back
                    calibD1 = now - calibPos0;
                    calibPos0 = now; calibT0 = GetTickCount64(); calibStage = 2;
                } else {
                    float d2 = now - calibPos0;
                    bool clear = !calibRefused && fabsf(calibD1) > 0.03f && fabsf(d2) > 0.03f && (calibD1 > 0) != (d2 > 0);
                    if (clear) {
                        cfg.ffbHwSign = calibD1 > 0.0f ? 1.0f : -1.0f;
                        cfg.ffbDirKnown = true;
                        g_calibMsg = cfg.ffbHwSign > 0 ? "FFB direction detected: normal (saved)." : "FFB direction detected: inverted, now corrected (saved).";
                        SaveConfig();
                    } else if (calibRefused) {
                        g_calibMsg = "FFB direction test failed: the wheel refused the force (another program has it). Retrying...";
                    } else {
                        g_calibMsg = "FFB direction test unclear: keep your hands off the wheel. Retrying...";
                    }
                    g_calibMsgUntil = GetTickCount64() + 8000;
                    Log("ffb direction test: push+ moved %+.3f, push- moved %+.3f, refused=%d -> %s (sign %+.0f)",
                        calibD1, d2, calibRefused ? 1 : 0, clear ? "accepted" : "rejected", cfg.ffbHwSign);
                    SetFfb(g_wheel, FfbOut{});
                    calibStage = 0;
                }
            }
        }

        Ped ped = PLAYER::PLAYER_PED_ID();
        Vehicle veh = PED::IS_PED_IN_ANY_VEHICLE(ped, FALSE) ? PED::GET_VEHICLE_PED_IS_IN(ped, FALSE) : 0;
        if (!veh || VEHICLE::GET_PED_IN_VEHICLE_SEAT(veh, -1, 0) != ped) {
            if (wasIn) StopFfb(g_wheel);
            SetClutchPatch(false);
            wasIn = false; continue;
        }
        if (!wasIn) { StartFfb(g_wheel); wasIn = true; }

        float steer = Axis(g_wheel, cfg.steerAxis) * SteerScale() * (cfg.invertSteer ? -1.0f : 1.0f);
        if (fabsf(steer) < cfg.deadzone) steer = 0;
        steer = std::clamp(steer, -1.0f, 1.0f);
        // non-linearity (ETS2 style): reduces sensitivity near the centre, full lock stays full lock
        steer = steer * (1.0f - cfg.nonlin + cfg.nonlin * fabsf(steer));
        float thr = Pedal(PD(), cfg.throttleAxis, cfg.invertThrottle);
        float brk = Pedal(PD(), cfg.brakeAxis, cfg.invertBrake);

        // handbrake: analog lever (any amount) and/or a button
        float handbrake = 0.0f;
        {
            Dev &hd = g_hand.ok ? g_hand : PD();
            if (!cfg.handbrakeAxis.empty()) {
                float v = Pedal(hd, cfg.handbrakeAxis, cfg.invertHandbrake);
                handbrake = v < 0.05f ? 0.0f : v;
            }
            Dev &hb = g_hand.ok ? g_hand : g_wheel;
            if (cfg.handbrakeButton >= 0 && (hb.st.rgbButtons[cfg.handbrakeButton] & 0x80)) handbrake = 1.0f;
        }

        g_hudHandbrake = handbrake;
        // shifter: which gear position is engaged (0 = neutral)
        int gear = 0;
        if (g_shifter.ok && cfg.manualGears)
            for (int i = 0; i <= MAXG; i++)
                if (cfg.gearButtons[i] >= 0 && (g_shifter.st.rgbButtons[cfg.gearButtons[i]] & 0x80)) { gear = (i == MAXG) ? -1 : i + 1; break; }
        static bool f10 = false, wasBtn = false;
        bool k10 = KeyDown(cfg.keyTrans);
        bool btn = cfg.toggleButton >= 0 && (g_wheel.st.rgbButtons[cfg.toggleButton] & 0x80);
        if ((k10 && !f10) || (btn && !wasBtn)) {
            g_trans = (g_trans + 1) % 3;              // auto -> manual -> sequential -> auto
            Log("transmission: %s", TRANS_NAMES[g_trans]);
            if (g_trans == 0 && !g_noFuel && !VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(veh))
                VEHICLE::SET_VEHICLE_ENGINE_ON(veh, TRUE, FALSE, FALSE);
        }
        f10 = k10; wasBtn = btn;

        // sequential paddles: up / down one gear, R - N - 1 .. 6, reverse only when (nearly) stopped
        const bool seq = (g_trans == 2);
        static int seqGear = 0;
        static bool upPrev = false, dnPrev = false;
        static ULONGLONG shiftCutUntil = 0;
        static Vehicle seqVeh = 0;
        if (veh != seqVeh) { seqVeh = veh; seqGear = 0; }
        if (seq) {
            bool up = cfg.paddleUp >= 0 && (g_wheel.st.rgbButtons[cfg.paddleUp] & 0x80);
            bool dn = cfg.paddleDown >= 0 && (g_wheel.st.rgbButtons[cfg.paddleDown] & 0x80);
            if (up && !upPrev) {
                if (seqGear == -1) seqGear = 0;
                else if (seqGear < TopGear(veh)) { seqGear++; if (seqGear >= 2) shiftCutUntil = GetTickCount64() + 140; }
            }
            if (dn && !dnPrev) {
                if (seqGear > 1) seqGear--;
                else if (seqGear == 1) seqGear = 0;
                else if (seqGear == 0 && ENTITY::GET_ENTITY_SPEED(veh) < 1.5f) seqGear = -1;
            }
            upPrev = up; dnPrev = dn;
            gear = seqGear;
            g_liveGear = seqGear;
        }

        bool useShifter = ((g_trans == 1 && g_shifter.ok) || seq) && cfg.manualGears && g_nextGearOff;
        SetClutchPatch(useShifter);
        bool useClutch = useShifter && !seq && !cfg.clutchAxis.empty();
        bool running = VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(veh) != 0;
        bool noReverseBrake = false; // manual box: brake must never select reverse
        bool freeRev = false;        // engine disconnected from the wheels
        float rawThr = thr;

        if (useShifter) {
            float clutch = useClutch ? Pedal(PD(), cfg.clutchAxis, cfg.invertClutch) : 0.0f; // 1 = fully pressed
            if (seq) {
                // no clutch pedal in sequential mode: slip the clutch a little when pulling away, fully engaged above walking pace
                float sp = ENTITY::GET_ENTITY_SPEED(veh);
                float engage = (sp >= 2.5f || thr <= 0.02f) ? 1.0f : std::max(0.4f, sp / 2.5f);
                clutch = 1.0f - engage;
            }
            if (BYTE *vp0 = getScriptHandleBaseAddress(veh)) {   // what the game did last frame, before we overwrite it
                if (g_rpmOff) g_dbgClutchGame = *(float *)(vp0 + g_rpmOff + 0xC);
                g_dbgNext = *(uint16_t *)(vp0 + g_nextGearOff);
                g_dbgCur = *(uint16_t *)(vp0 + g_curGearOff);
            }
            static int engaged = 0;
            static ULONGLONG lowSince = 0;
            static Vehicle lastVeh = 0;
            if (veh != lastVeh) { lastVeh = veh; engaged = 0; lowSince = 0; }

            if (useClutch) {
                // gear only goes in with the clutch pressed; otherwise stays in neutral until it is
                if (gear != engaged) {
                    if (gear != 0 && clutch <= 0.6f) g_grindNow = true; // forced a gear in without the clutch: grind
                    engaged = (gear == 0 || clutch > 0.6f) ? gear : 0;
                }
            } else engaged = gear;

            g_hudEngaged = engaged;
            noReverseBrake = (engaged != -1);
            float speed = ENTITY::GET_ENTITY_SPEED(veh);

            // ---- clutch model ----
            // engagement e: 1 = fully engaged (pedal up), 0 = fully disengaged; the friction (bite) zone is the middle of the pedal travel
            float e = useClutch ? std::clamp((0.85f - clutch) / 0.6f, 0.0f, 1.0f) : std::clamp(1.0f - clutch, 0.0f, 1.0f);
            e = e * e * (3.0f - 2.0f * e);
            Vector3 rv0 = ENTITY::GET_ENTITY_SPEED_VECTOR(veh, TRUE);
            float along = engaged > 0 ? rv0.y : (engaged == -1 ? -rv0.y : 0.0f);   // speed in the direction the gear drives
            if (engaged != 0 && speed > 3.0f && fabsf(rv0.x) > 0.7f * fabsf(rv0.y)) along = std::max(along, 0.7f * speed);   // sliding
            // speed the car holds with the clutch out and no throttle (engine idling): 1st gear slowest, each gear faster
            float vIdle = (cfg.creep1 / 3.6f) * powf(std::max(cfg.gearSpacing, 1.0f), (float)(engaged > 0 ? engaged - 1 : 0));
            vIdle = std::max(vIdle, 0.2f);

            // stalling: the clutch takes up load the engine cannot carry (dropped too fast, or climbing without enough throttle)
            static float stallAcc = 0.0f;
            if (useClutch && running && engaged != 0 && e > 0.05f) {
                float slow = 1.0f - std::clamp(along / vIdle, 0.0f, 1.0f);          // 1 = standing still, 0 = at/above creep speed
                float pitch = ENTITY::GET_ENTITY_PITCH(veh);                        // nose up = climbing
                float hill = std::clamp((engaged > 0 ? pitch : -pitch) / 14.0f, 0.0f, 0.5f);
                float demand = e * (1.0f + hill) * slow;
                float capacity = 0.7f / std::max(cfg.stallSens, 0.1f) + 2.0f * rawThr;
                if (demand > capacity) stallAcc += frameDt * (demand - capacity) * 5.0f;
                else stallAcc = std::max(0.0f, stallAcc - frameDt * 2.5f);
                if (stallAcc >= 1.0f) {
                    VEHICLE::SET_VEHICLE_ENGINE_ON(veh, FALSE, TRUE, TRUE);
                    Log("engine stalled gear=%d speed=%.2f e=%.2f demand=%.2f capacity=%.2f", engaged, speed, e, demand, capacity);
                    running = false; stallAcc = 0.0f;
                }
            } else stallAcc = std::max(0.0f, stallAcc - frameDt * 2.5f);

            // starting: hold the key / button to crank. In gear with the clutch out the starter only jerks the car along.
            static float crank = 0.0f;
            static ULONGLONG lurchAt = 0;
            bool startHeld = KeyDown(cfg.keyStart) ||
                             (cfg.startButton >= 0 && (g_wheel.st.rgbButtons[cfg.startButton] & 0x80));
            if (g_startSuppress) startHeld = false;
            if (!running && startHeld) {
                crank += frameDt;
                if (engaged != 0 && e > 0.3f) {
                    if (GetTickCount64() - lurchAt > 220) {
                        lurchAt = GetTickCount64();
                        if (along < 0.9f) {
                            Vector3 fwd = ENTITY::GET_ENTITY_FORWARD_VECTOR(veh), vel = ENTITY::GET_ENTITY_VELOCITY(veh);
                            float dir = engaged > 0 ? 1.0f : -1.0f;
                            vel.x += fwd.x * dir * 0.35f; vel.y += fwd.y * dir * 0.35f;
                            ENTITY::SET_ENTITY_VELOCITY(veh, vel);
                        }
                    }
                    crank = std::min(crank, cfg.crankTime * 0.5f);              // never catches while the engine is loaded
                } else if (crank >= cfg.crankTime && !g_noFuel) {
                    VEHICLE::SET_VEHICLE_ENGINE_ON(veh, TRUE, FALSE, FALSE);
                    crank = 0.0f;
                }
            } else crank = 0.0f;

            // Engine speed follows the wheels through the clutch. rpm per m/s of each gear is learned while the clutch is fully
            // in (the game computes it); with the clutch slipping the engine is pulled towards the wheel-driven rpm right away,
            // so after a downshift the revs jump up when the clutch bites instead of the game letting them creep up slowly.
            static float kRpm[MAXG + 2] = {};
            if (g_rpmOff && running && engaged > 0 && engaged <= MAXG) {
                if (BYTE *vq = getScriptHandleBaseAddress(veh)) {
                    float rpmNow = *(float *)(vq + g_rpmOff);
                    if (e > 0.97f && along > 3.0f && rpmNow > 0.25f && rpmNow < 1.0f) {
                        float k = rpmNow / along;
                        if (k > 0.004f && k < 0.6f) kRpm[engaged] = kRpm[engaged] > 0.0f ? kRpm[engaged] + (k - kRpm[engaged]) * 0.05f : k;
                    }
                    if (e > 0.05f && e < 0.98f && along > 0.4f) {
                        float k = kRpm[engaged];
                        if (k <= 0.0f)                                                  // never driven in this gear yet: derive it
                            for (int h = 1; h <= MAXG; h++)                             // from the closest gear that is known
                                if (kRpm[h] > 0.0f) { k = kRpm[h] * powf(1.35f, (float)(h - engaged)); break; }
                        if (k > 0.0f) {
                            float wheelRpm = std::min(k * along, 1.05f);
                            float freeRpm = 0.2f + 0.8f * rawThr;
                            float target = wheelRpm > freeRpm ? freeRpm + e * (wheelRpm - freeRpm) : freeRpm;
                            target = std::max(0.2f, target) * (1.0f - 0.6f * std::min(stallAcc, 1.0f));   // labouring before it stalls
                            *(float *)(vq + g_rpmOff) = rpmNow + (target - rpmNow) * 0.6f;
                        }
                    }
                }
            }

            // idle governor: with the clutch out and no throttle the idling engine pulls the car up to the creep speed of the gear
            float creep = 0.0f;
            if (running && engaged != 0 && e > 0.1f && thr < 0.08f)
                creep = std::clamp(1.0f - along / vIdle, 0.0f, 1.0f) * 0.32f * e;

            freeRev = running && (engaged == 0 || (useClutch && e <= 0.05f));
            if (engaged == 0 || !running) thr = 0;          // neutral / stalled: no drive
            else {
                thr = std::max(thr, creep);
                {
                    static ULONGLONG limiterUntil = 0;
                    ULONGLONG nowMs = GetTickCount64();
                    if (engaged > 0 && g_hudRpm >= 0.995f) limiterUntil = nowMs + 90;
                    if (nowMs < limiterUntil) thr = 0.0f;                   // fuel cut at the limiter
                }
                // without the real clutch float the game's own auto-clutch slips in high gears at low rpm
                if (useClutch && !g_rpmOff) thr *= std::clamp(1.0f - clutch / 0.8f, 0.0f, 1.0f); // fallback slip model
                if (engaged == -1) {
                    // reverse: the game reverses on brake input and brakes on accelerate input while moving backwards.
                    // The brake pedal must never turn into "accelerate" unless the car really is rolling backwards.
                    Vector3 rv = ENTITY::GET_ENTITY_SPEED_VECTOR(veh, TRUE);   // y = forward speed
                    float pThr = thr, pBrk = brk;
                    if (rv.y > 0.5f) { thr = 0.0f; brk = pBrk; }                // still rolling forward: brake normally, no drive
                    else if (rv.y < -0.5f) { thr = pBrk; brk = pThr; }          // moving backwards: pedals swapped for the game
                    else {                                                      // (nearly) stopped
                        thr = 0.0f; brk = pThr;                                 // throttle pedal starts reversing
                        if (pBrk > 0.05f && pThr < 0.05f) handbrake = std::max(handbrake, 1.0f);   // brake pedal holds the car
                    }
                } else SetGear(veh, engaged);
                if (seq && GetTickCount64() < shiftCutUntil) thr = 0;   // short throttle cut while the paddle shift happens
            }
            // the game's clutch value: 1 = fully engaged (pedal up), < 0 = disengaged (neutral / pedal fully down)
            if (g_rpmOff && running) {
                if (BYTE *vp = getScriptHandleBaseAddress(veh)) {
                    float cv;
                    if (engaged == 0 || e <= 0.02f) cv = speed < 1.0f ? -5.0f : -0.5f;
                    else cv = e;
                    *(float *)(vp + g_rpmOff + 0xC) = cv;
                    g_clutchOut = cv;
                }
            }
        }

        {
            // telemetry for the "mark problem" capture
            Sample s{};
            s.ms = GetTickCount64(); s.speed = ENTITY::GET_ENTITY_SPEED(veh); s.rpm = g_hudRpm;
            s.rawThr = rawThr; s.thr = thr; s.brk = brk; s.clutch = g_liveClutch; s.clutchOut = useShifter ? g_clutchOut : 1.0f;
            s.steer = steer; s.shifter = g_liveGear; s.engaged = useShifter ? g_hudEngaged : 99; s.running = running;
            PublishGear(useShifter ? g_liveGear : 99);   // the lever position (-1 = R), not the gear that is engaged with the clutch
            s.clutchGame = g_dbgClutchGame; s.nextGame = g_dbgNext; s.curGame = g_dbgCur;
            RingPush(s);
            static bool wasMark = false;
            bool mk = cfg.keyMark > 0 && KeyEdge(cfg.keyMark);
            bool mb = cfg.markButton >= 0 && (g_wheel.st.rgbButtons[cfg.markButton] & 0x80);
            if (mk || (mb && !wasMark)) RingDump();
            wasMark = mb;
        }
        g_hudShow = useShifter;   // drawn by DrawHud() next frame
        if (g_rpmOff) { BYTE *vp = getScriptHandleBaseAddress(veh); if (vp) g_hudRpm = *(float *)(vp + g_rpmOff); }


        if (freeRev) RevEngine(veh, rawThr);

        if (noReverseBrake && brk > 0.05f) {
            // near standstill (or rolling back) the game would engage reverse on brake: hold with handbrake instead
            Vector3 sv = ENTITY::GET_ENTITY_SPEED_VECTOR(veh, TRUE);
            if (sv.y < 1.0f) {
                handbrake = 1.0f;
                brk = 0;
            }
        }
        if (handbrake > 0.01f) PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, 76, handbrake); // INPUT_VEH_HANDBRAKE
        if (cfg.directSteer && g_steerInputOff) {
            PAD::DISABLE_CONTROL_ACTION(1, 59, TRUE);     // keep the game's own steering input out of the way
            SetSteerInput(veh, steer);
            if (BYTE *sp = getScriptHandleBaseAddress(veh)) {   // the angle field sits 8 bytes after the input (layout as in ikt32's VehicleExtensions)
                g_dbgSteerAngle = *(float *)(sp + g_steerInputOff + 8);
                SteerLog(veh, Axis(g_wheel, cfg.steerAxis) * (cfg.invertSteer ? -1.0f : 1.0f), steer, sp);
                static Vehicle lockVeh = 0; static float peak = 0.0f;
                if (veh != lockVeh) { lockVeh = veh; peak = 0.0f; }
                if (fabsf(steer) >= 0.95f && fabsf(g_dbgSteerAngle) > peak + 0.01f) {
                    peak = fabsf(g_dbgSteerAngle);
                    Log("steering lock check: input %.2f -> angle field %.3f rad = %.1f deg", steer, g_dbgSteerAngle, g_dbgSteerAngle * 57.29578f);
                }
            }
        } else {
            // control path: the game filters it (dead zone). Optionally lift small inputs above that dead zone.
            float out = steer;
            if (cfg.deadComp > 0.0f && fabsf(out) > 0.002f)
                out = copysignf(cfg.deadComp + (1.0f - cfg.deadComp) * fabsf(out), out);
            PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, 59, out);  // INPUT_VEH_MOVE_LR
            if (cfg.directSteer) {
                static float lastCmd = 0;
                DiscoverSteer(veh, lastCmd);              // learn where the game keeps its steering input
                lastCmd = out;
            }
        }
        if (g_discNeedSave) {
            g_discNeedSave = false;
            char b[32];
            snprintf(b, sizeof b, "0x%X", g_steerInputOff); W("Memory", "SteerOffset", b);
            W("Memory", "SteerSign", g_steerSign < 0 ? "-1" : "1");
            snprintf(b, sizeof b, "%lX", (unsigned long)ExeStamp()); W("Memory", "SteerStamp", b);
        }
        // the game ignores roughly the first quarter of an analog control: lift pedal values above it
        auto lift = [](float x, float c) { return (c > 0.0f && x > 0.002f) ? c + (1.0f - c) * x : x; };
        PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, 71, lift(thr, cfg.pedalComp));    // INPUT_VEH_ACCELERATE
        PAD::SET_CONTROL_VALUE_NEXT_FRAME(0, 72, lift(brk, cfg.pedalComp));    // INPUT_VEH_BRAKE

        if (testActive || calibStage != 0) {
            // the menu test / direction detection owns the wheel right now
        } else if (cfg.ffb) {
            EnsurePlaying(g_wheel);
            // ---- per-frame vehicle state ----
            static LARGE_INTEGER qf = {}, qlast = {};
            if (!qf.QuadPart) QueryPerformanceFrequency(&qf);
            LARGE_INTEGER qnow; QueryPerformanceCounter(&qnow);
            float dt = qlast.QuadPart ? (float)(qnow.QuadPart - qlast.QuadPart) / (float)qf.QuadPart : 0.016f;
            qlast = qnow;
            dt = std::clamp(dt, 0.004f, 0.1f);

            float speed = ENTITY::GET_ENTITY_SPEED(veh);
            Vector3 rot = ENTITY::GET_ENTITY_ROTATION_VELOCITY(veh);
            Vector3 vel = ENTITY::GET_ENTITY_VELOCITY(veh);
            Vector3 rel = ENTITY::GET_ENTITY_SPEED_VECTOR(veh, TRUE); // x = sideways, y = forward
            static float pvz = 0, pspeed = 0, pvx = 0, rough = 0, bump = 0, jolt = 0, understeerAmt = 0;
            static float satF = 0, prevPos = 0, velF = 0, lastC = 0;
            static ULONGLONG softMuteUntil = 0;
            static Vehicle fxVeh = 0;
            if (veh != fxVeh) { fxVeh = veh; pvz = vel.z; pspeed = speed; pvx = rel.x; rough = bump = jolt = understeerAmt = 0; }
            float vf = std::clamp(speed / 15.0f, 0.0f, 1.0f);       // 0 = parked .. 1 = >54 km/h

            // self-aligning torque from the car's own rotation: yaw rate (U-turns, quick turns, slides)
            // plus lateral acceleration (speed * yaw rate). Not tied to wheel position -> no spring.
            float yaw = rot.z;                                      // rad/s, + = turning left
            float satRaw = std::clamp(speed * yaw / 9.0f + 0.25f * yaw, -1.0f, 1.0f);   // lateral acceleration (9 m/s^2 = full) + pure rotation
            satF += (satRaw - satF) * (1.0f - expf(-dt / 0.04f));                      // light smoothing keeps the loop stable
            float sat = satF;
            float dirSign = cfg.invertSteer ? -1.0f : 1.0f;         // axis direction vs game direction
            float force = sat * dirSign * cfg.lateralGain;

            // optional ETS2-style centering (position based, scaled by speed). Off by default.
            float pos = Axis(g_wheel, cfg.steerAxis);               // raw wheel position -1..1
            float centering = (cfg.centerLow + (cfg.centerHigh - cfg.centerLow) * vf) * std::clamp(speed / 2.5f, 0.0f, 1.0f);   // none while parked: a car only self-centres when it rolls
            force += -pos * centering;

            // vertical acceleration: bumps (spikes) and terrain (sustained roughness)
            float az = (vel.z - pvz) / dt; pvz = vel.z;
            bump = std::max(bump * 0.86f, std::clamp(fabsf(az) / 35.0f, 0.0f, 1.0f));
            rough += (std::clamp(fabsf(az) / 9.0f, 0.0f, 1.0f) - rough) * 0.08f;
            if (!VEHICLE::IS_VEHICLE_ON_ALL_WHEELS(veh)) { bump = 0; rough *= 0.8f; } // airborne: no road feel

            // impacts: sudden loss of speed together with a collision flag, direction from sideways change
            float decel = (pspeed - speed) / dt; pspeed = speed;
            float dvx = rel.x - pvx; pvx = rel.x;
            jolt *= 0.80f;
            if (ENTITY::HAS_ENTITY_COLLIDED_WITH_ANYTHING(veh) && decel > 6.0f)
                jolt = std::max(jolt, std::clamp(decel / 70.0f, 0.0f, 1.0f));
            float joltDir = (dvx >= 0 ? 1.0f : -1.0f) * dirSign;
            force += joltDir * jolt * cfg.collisions;

            // understeer: lots of steering, little yaw for that speed -> front end washes wide (light + shaking)
            float expectYaw = fabsf(steer) * speed * 0.12f;
            float un = 0;
            if (speed > 8.0f && fabsf(steer) > 0.25f && expectYaw > 0.1f)
                un = std::clamp((expectYaw - fabsf(yaw)) / expectYaw - 0.35f, 0.0f, 1.0f);
            understeerAmt += (un - understeerAmt) * 0.15f;
            force *= 1.0f - 0.45f * understeerAmt * cfg.understeer;  // wheel goes light when the front lets go

            if (cfg.ffbInvert) force = -force;

            // the weight of the wheel is made here from the wheel's own speed, so it works on any device, also parked or crawling:
            // friction resists any movement (heavier when parked), damping resists fast movement. Axis coordinates.
            float wv = (pos - prevPos) / dt; prevPos = pos;
            if (fabsf(wv) < 0.07f) wv = 0.0f;                       // one count of axis resolution per frame (0.06) is noise, not movement
            velF += (wv - velF) * 0.25f;
            {
                // fast swings back and forth, many times in a short window, while the plugin pushes hard = the force
                // makes the wheel swing (wrong direction). Cut the software force at once and measure the direction again.
                static ULONGLONG winStart = 0; static int flips = 0; static float lastSign = 0;
                ULONGLONG nowMs = GetTickCount64();
                if (nowMs - winStart > 1500) { winStart = nowMs; flips = 0; }
                float s = velF > 0.8f ? 1.0f : (velF < -0.8f ? -1.0f : 0.0f);
                if (s != 0.0f && s != lastSign) { if (lastSign != 0.0f) flips++; lastSign = s; }
                if (flips >= 8 && fabsf(lastC) > 0.15f) {
                    softMuteUntil = nowMs + 3000;
                    g_calibMsg = "Wheel was shaking: friction and damping paused for 3 s. Lower Internal friction / Damping.";
                    g_calibMsgUntil = nowMs + 5000;
                    Log("ffb: shaking detected (velocity %.2f, cmd %.2f): software friction/damping paused 3 s", velF, lastC);
                    flips = 0;
                }
            }
            float fv0 = 0.25f + 0.5f * cfg.weight;                     // friction reaches its full value over a wide speed range
            float soft = -(cfg.weight * 0.7f * (1.0f - 0.5f * vf) * tanhf(velF / fv0) + cfg.damping * 0.8f * (0.4f + 0.6f * vf) * velF);
            if (GetTickCount64() < softMuteUntil) soft = 0.0f;

            FfbOut o;
            soft = std::clamp(soft, -0.4f, 0.4f);
            // hwSign: how this device really reads a positive force. Unknown (not measured yet) -> send no such force at all.
            float cmd = (force + soft) * cfg.ffbGain * cfg.ffbHwSign * (cfg.ffbDirKnown ? 1.0f : 0.0f);
            cmd = std::clamp(cmd, lastC - 0.35f, lastC + 0.35f);                      // no sudden jumps between frames
            lastC = cmd;
            g_dbgForce = force; g_dbgSoft = soft; g_dbgCmd = cmd;
            o.constant = cmd;
            o.friction = cfg.hwConditions ? cfg.weight * (1.0f - 0.5f * vf) * cfg.ffbGain : 0.0f;
            o.damper = cfg.hwConditions ? cfg.damping * (0.3f + 0.7f * vf) * cfg.ffbGain : 0.0f;

            // vibrations
            float rpm = 0.2f;
            if (g_rpmOff) { BYTE *vp = getScriptHandleBaseAddress(veh); if (vp) rpm = std::clamp(*(float *)(vp + g_rpmOff), 0.0f, 1.2f); }
            if (!VEHICLE::GET_IS_VEHICLE_ENGINE_RUNNING(veh)) rpm = 0;
            o.engHz = 14.0f + rpm * 46.0f;
            o.engMag = rpm > 0.05f ? cfg.engineRes * (0.25f + 0.75f * rpm) * 0.4f * cfg.ffbGain : 0.0f;

            float roadMag = std::max(bump * cfg.bumps, rough * cfg.terrain * (0.3f + 0.7f * vf));
            o.roadHz = 24.0f + vf * 26.0f;
            o.roadMag = roadMag * 0.6f * cfg.ffbGain;

            static ULONGLONG grindUntil = 0;
            if (g_grindNow) { grindUntil = GetTickCount64() + 260; g_grindNow = false; }
            bool grinding = GetTickCount64() < grindUntil;
            o.slipHz = grinding ? 17.0f : 24.0f;
            o.slipMag = std::max(understeerAmt * cfg.understeer * 0.5f, grinding ? cfg.grind * 0.8f : 0.0f) * cfg.ffbGain;
            SetFfb(g_wheel, o);
        } else SetFfb(g_wheel, FfbOut{});
    }
}

BOOL APIENTRY DllMain(HMODULE hMod, DWORD reason, LPVOID) {
    switch (reason) {
    case DLL_PROCESS_ATTACH: {
        g_module = hMod;
        char path[MAX_PATH]; GetModuleFileNameA(hMod, path, MAX_PATH);
        g_dir = path; g_dir = g_dir.substr(0, g_dir.find_last_of("\\/") + 1);
        g_log = fopen((g_dir + "WheelPlugin.log").c_str(), "w");
        {
            char exe[MAX_PATH] = {}; GetModuleFileNameA(nullptr, exe, MAX_PATH);
            g_exeName = exe;
            std::string l = g_exeName; std::transform(l.begin(), l.end(), l.begin(), ::tolower);
            g_isFiveM = l.find("fivem") != std::string::npos;
        }
        Log("WheelPlugin %s loaded", WP_VERSION);
        presentCallbackRegister(OnPresent);
        scriptRegister(hMod, ScriptMain);
        break; }
    case DLL_PROCESS_DETACH:
        SetClutchPatch(false);
        presentCallbackUnregister(OnPresent);
        scriptUnregister(hMod);
        if (g_log) fclose(g_log);
        break;
    }
    return TRUE;
}
