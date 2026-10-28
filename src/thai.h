// thai.h: Thai text for the in-game menu and HUD. Included from main.cpp (needs cfg, g_dir, Log, MAXG and the natives).
//
// The game's own fonts have no Thai glyphs. Thai text is therefore drawn into a PNG with the embedded TH Sarabun font
// (GDI+ shapes and rasterises it, private font from memory: nothing is installed on the PC) and drawn by a small Direct3D 11
// overlay in ScriptHookV's present callback, one texel per screen pixel. Source strings stay English: DrawStr() looks them
// up in the table below when "Thai text" is on, and draws any string that already contains Thai as it is.
#pragma once
#include <d3d11.h>
#include <d3dcompiler.h>
#include <dxgi.h>
#include <atomic>
#include <mutex>
#include <functional>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")

static const std::string *ThTable(const std::string &en) {
    static std::map<std::string, std::string> m;
    if (m.empty()) {
        static const char *kv[][2] = {
            // title, categories
            {"WHEEL SETUP", "ตั้งค่าพวงมาลัย"}, {"Save and close", "บันทึกและปิด"},
            {"General", "ทั่วไป"}, {"Force feedback", "แรงสะท้อนพวงมาลัย"}, {"FFB effects", "เอฟเฟกต์ FFB"},
            {"Steering and pedals", "พวงมาลัยและแป้น"}, {"Clutch and engine", "คลัตช์และเครื่องยนต์"},
            {"Buttons", "ปุ่มบนพวงมาลัย"}, {"H-shifter gears", "เกียร์ H-shifter"}, {"Devices", "อุปกรณ์"},
            {"Controls and devices", "อุปกรณ์และปุ่มควบคุม"}, {"Camera", "กล้อง"}, {"Camera on foot", "กล้องตอนเดินเท้า"}, {"Camera in vehicle", "กล้องตอนอยู่ในรถ"},
            {"Third-person distance", "ระยะกล้องบุคคลที่ 3"}, {"Unchanged", "ตามเกม"}, {"First person", "บุคคลที่ 1"}, {"Third person", "บุคคลที่ 3"},
            {"Close", "ใกล้"}, {"Medium", "กลาง"}, {"Far", "ไกล"},
            {"Keyboard keys", "ปุ่มคีย์บอร์ด"}, {"Fuel and engine", "น้ำมันและเครื่องยนต์"}, {"HUD and key blocks", "หน้าจอและบล็อกปุ่ม"},
            // general
            {"Transmission", "ระบบเกียร์"}, {"Auto", "อัตโนมัติ"}, {"Manual (clutch)", "ธรรมดา (คลัตช์)"}, {"Sequential (paddles)", "ซีเควนเชียล (พาย)"},
            {"Steering lock (deg)", "องศาล็อกพวงมาลัย (องศา)"}, {"Pit House rotation (deg)", "องศาหมุนใน Pit House"},
            {"Direct steering (no dead zone)", "เลี้ยวตรง (ไม่มีโซนตาย)"}, {"Recalibrate steering offset", "ปรับเทียบศูนย์พวงมาลัยใหม่"},
            {"Recalibrate", "ปรับเทียบ"}, {"Steering dead zone comp.", "ชดเชยโซนตายพวงมาลัย"}, {"Pedal dead zone comp.", "ชดเชยโซนตายแป้น"},
            {"Steering non-linearity", "ความไม่เชิงเส้นพวงมาลัย"}, {"Steering deadzone", "โซนตายพวงมาลัย"}, {"Log steering data (CSV)", "บันทึกข้อมูลการเลี้ยว (CSV)"},
            {"Fitment", "จัดฟิตเม้นรถ"}, {"Adjust with the car in view (small panel)", "ปรับโดยเห็นรถ (แผงเล็ก)"}, {"Open", "เปิด"}, {"Save and back", "บันทึกและกลับ"}, {"Fitment saved per car (plate)", "ฟิตเม้นแยกตามรถ (ป้ายทะเบียน)"}, {"Front track width (cm)", "ความกว้างล้อหน้า (ซม.)"}, {"Rear track width (cm)", "ความกว้างล้อหลัง (ซม.)"},
            {"Front ride height (cm)", "ความสูงหน้า (ซม.)"}, {"Rear ride height (cm)", "ความสูงหลัง (ซม.)"},
            {"Front camber (deg)", "แคมเบอร์หน้า (องศา)"}, {"Rear camber (deg)", "แคมเบอร์หลัง (องศา)"}, {"Invert camber direction", "กลับทิศแคมเบอร์"},
            {"Arrow keys: turn the camera around the car", "ปุ่มลูกศร: หมุนกล้องรอบรถ"}, {"Page Up / Page Down: zoom", "Page Up / Page Down: ซูม"},
            {"Reset fitment of this car", "รีเซ็ตฟิตเม้นรถคันนี้"}, {"Save wheel data to a file (for analysis)", "บันทึกข้อมูลล้อลงไฟล์ (ไว้วิเคราะห์)"},
            {"Save", "บันทึก"}, {"Wheel data saved", "บันทึกข้อมูลล้อแล้ว"}, {"Wheels not found in this vehicle", "ไม่พบข้อมูลล้อของรถคันนี้"},
            {"Fitment reset", "รีเซ็ตฟิตเม้นแล้ว"}, {"Sit in the driver seat first", "นั่งที่คนขับก่อน"},
            // force feedback
            {"Test force feedback", "ทดสอบแรงสะท้อน"}, {"Run test", "ทดสอบ"}, {"Detect FFB direction (hands off!)", "ตรวจทิศทางแรง (ปล่อยมือ!)"},
            {"Detect", "ตรวจ"}, {"Also use device friction/damper", "ใช้แรงเสียดทาน/ดัมเปอร์ของอุปกรณ์ด้วย"},
            {"Overall gain", "แรงรวม"}, {"Internal friction", "แรงเสียดทานภายใน"}, {"Damping", "แรงหน่วง"},
            {"Self-aligning (car rotation)", "แรงดึงกลับตามการหมุนของรถ"}, {"Centering at low speed", "ดึงกลับศูนย์ที่ความเร็วต่ำ"},
            {"Centering at high speed", "ดึงกลับศูนย์ที่ความเร็วสูง"}, {"FFB direction flip", "กลับทิศทาง FFB"},
            // effects
            {"Engine resonance", "การสั่นของเครื่องยนต์"}, {"Terrain surface", "พื้นผิวถนน"}, {"Bumps", "หลุมบ่อ"}, {"Collisions", "การชน"},
            {"Gearbox grind", "เฟืองขบ"}, {"Understeer slip", "หน้าดื้อ (ล้อหน้าลื่น)"},
            // steering and pedals
            {"Steering wheel", "พวงมาลัย"}, {"Throttle pedal", "แป้นคันเร่ง"}, {"Brake pedal", "แป้นเบรก"}, {"Clutch pedal", "แป้นคลัตช์"},
            {"Handbrake lever (analog)", "เบรกมือ (อนาล็อก)"},
            {"Turn the wheel fully RIGHT", "หมุนพวงมาลัยสุดไปทางขวา"}, {"Press the THROTTLE pedal fully", "เหยียบคันเร่งให้สุด"},
            {"Press the BRAKE pedal fully", "เหยียบเบรกให้สุด"}, {"Press the CLUTCH pedal fully", "เหยียบคลัตช์ให้สุด"},
            {"Pull the HANDBRAKE lever fully", "ดึงเบรกมือให้สุด"},
            // clutch and engine
            {"Creep speed, 1st gear (km/h)", "ความเร็วคลานเกียร์ 1 (กม./ชม.)"}, {"Creep speed gear step (x)", "ตัวคูณความเร็วคลานต่อเกียร์"},
            {"Stall sensitivity", "ความไวต่อเครื่องดับ"}, {"Engine crank time (s)", "เวลากดสตาร์ท (วินาที)"},
            // buttons
            {"Paddle shift up (+)", "พายเปลี่ยนเกียร์ขึ้น (+)"}, {"Paddle shift down (-)", "พายเปลี่ยนเกียร์ลง (-)"}, {"Handbrake button", "ปุ่มเบรกมือ"},
            {"Transmission mode button", "ปุ่มสลับโหมดเกียร์"}, {"Engine start / stop button", "ปุ่มสตาร์ท/ดับเครื่อง"},
            {"Mark problem (save capture)", "ทำเครื่องหมายปัญหา (บันทึกข้อมูล)"}, {"Refuel button (hold)", "ปุ่มเติมน้ำมัน (กดค้าง)"},
            {"Press the UPSHIFT paddle", "กดพายเปลี่ยนเกียร์ขึ้น"}, {"Press the DOWNSHIFT paddle", "กดพายเปลี่ยนเกียร์ลง"},
            {"Press the HANDBRAKE button", "กดปุ่มเบรกมือ"}, {"Press a button on the wheel", "กดปุ่มบนพวงมาลัย"},
            // H-shifter
            {"Reverse", "เกียร์ถอยหลัง (R)"}, {"Move shifter into REVERSE", "ดันคันเกียร์เข้าเกียร์ถอยหลัง"},
            // devices
            {"Wheel base (FFB)", "ฐานพวงมาลัย (FFB)"}, {"Pedals", "แป้นเหยียบ"}, {"Handbrake device", "อุปกรณ์เบรกมือ"}, {"H-shifter", "คันเกียร์ H"},
            {"(auto)", "(อัตโนมัติ)"}, {"(on wheel base)", "(บนฐานพวงมาลัย)"}, {"(on pedals / base)", "(บนแป้น/ฐาน)"}, {"(none)", "(ไม่มี)"},
            // keys
            {"Open this menu", "เปิดเมนูนี้"}, {"Cycle transmission mode", "สลับโหมดเกียร์"}, {"Engine start / stop", "สตาร์ท/ดับเครื่อง"},
            {"Save telemetry capture", "บันทึกข้อมูลเทเลเมทรี"}, {"Dump axes to log", "บันทึกค่าแกนลงล็อก"},
            {"Refuel (hold at a pump)", "เติมน้ำมัน (กดค้างที่ปั๊ม)"},
            {"Press the keyboard key to use (Esc = cancel)", "กดปุ่มบนคีย์บอร์ดที่ต้องการ (Esc = ยกเลิก)"},
            // fuel and engine
            {"Fuel system", "ระบบน้ำมัน"}, {"Full tank range (km)", "ระยะทางต่อถังเต็ม (กม.)"}, {"Tank level of a new vehicle", "ระดับน้ำมันของรถคันใหม่"},
            {"Refuel speed (tank % per s)", "ความเร็วเติมน้ำมัน (% ถังต่อวินาที)"}, {"Start key also stops engine", "ปุ่มสตาร์ทใช้ดับเครื่องด้วย"},
            {"Gas station map icons", "ไอคอนปั๊มบนแผนที่"}, {"Gas pump markers", "มาร์คเกอร์ที่ตู้จ่ายน้ำมัน"},
            // HUD and blocks
            {"Speed number on HUD", "แสดงความเร็วบน HUD"}, {"Fuel number on HUD", "แสดงน้ำมันบน HUD"}, {"Speed unit", "หน่วยความเร็ว"},
            {"Thai text", "ข้อความภาษาไทย"}, {"Thai text size", "ขนาดตัวอักษรไทย"}, {"Thai text bold", "ตัวอักษรไทยแบบหนา"}, {"Neon dashboard style", "แดชบอร์ดสไตล์นีออน"}, {"Dashboard size", "ขนาดแดชบอร์ด"}, {"mph", "ไมล์/ชม."}, {"km/h", "กม./ชม."},
            {"Block F1 (Rockstar Editor record)", "บล็อก F1 (อัดคลิป Rockstar Editor)"},
            {"Block F2 (Rockstar Editor record 2)", "บล็อก F2 (อัดคลิป Rockstar Editor 2)"},
            {"Block F3 (save replay clip)", "บล็อก F3 (บันทึกคลิปย้อนหลัง)"},
            {"Block F5-F8 (character / online switch)", "บล็อก F5-F8 (สลับตัวละคร/ออนไลน์)"},
            {"BLOCKED", "บล็อกแล้ว"}, {"game default", "ค่าเดิมของเกม"},
            // values and states
            {"ON", "เปิด"}, {"OFF", "ปิด"}, {"Testing...", "กำลังทดสอบ..."}, {"Press a key...", "กดปุ่ม..."}, {"Waiting...", "รอสัญญาณ..."},
            {"Reset", "รีเซ็ต"}, {"Clear", "ล้าง"}, {"(off)", "(ปิด)"},
            // footer and messages
            {"   (Esc = cancel)", "   (Esc = ยกเลิก)"}, {"Type a value, Enter = OK, Esc = cancel", "พิมพ์ตัวเลข  Enter = ตกลง  Esc = ยกเลิก"},
            {"Drag the sliders or click a % box to type. Click a bind button, then use the control.",
             "ลากแถบเลื่อน หรือคลิกช่อง % เพื่อพิมพ์ค่า  คลิกปุ่มผูก แล้วใช้อุปกรณ์ที่ต้องการ"},
            {"FFB direction test: keep your hands off the wheel", "ทดสอบทิศทาง FFB: ปล่อยมือจากพวงมาลัย"},
            {"FFB direction detected: normal (saved).", "ตรวจพบทิศทาง FFB: ปกติ (บันทึกแล้ว)"},
            {"FFB direction detected: inverted, now corrected (saved).", "ตรวจพบทิศทาง FFB: กลับด้าน แก้ไขให้แล้ว (บันทึกแล้ว)"},
            {"FFB direction test failed: the wheel refused the force (another program has it). Retrying...",
             "ทดสอบทิศทาง FFB ไม่สำเร็จ: พวงมาลัยไม่รับแรง (อาจมีโปรแกรมอื่นถืออยู่) กำลังลองใหม่..."},
            {"FFB direction test unclear: keep your hands off the wheel. Retrying...", "ผลทดสอบไม่ชัด: ปล่อยมือจากพวงมาลัย กำลังลองใหม่..."},
            {"Wheel was shaking: friction and damping paused for 3 s. Lower Internal friction / Damping.",
             "พวงมาลัยสั่น: พักแรงเสียดทานและแรงหน่วง 3 วินาที ลองลดค่าลง"},
            // HUD
            {"KM/H", "กม./ชม."}, {"MPH", "ไมล์/ชม."}, {"ENGINE OFF", "เครื่องดับ"}, {"NO FUEL", "น้ำมันหมด"}, {"PRESS CLUTCH", "เหยียบคลัตช์"},
            {"FUEL", "น้ำมัน"},
            // refuelling
            {"Tank full", "ถังเต็มแล้ว"}, {"Turn the engine off to refuel", "ดับเครื่องก่อนเติมน้ำมัน"}, {"Hold %s to refuel", "กด %s ค้างเพื่อเติมน้ำมัน"},
            {"the refuel button", "ปุ่มเติมน้ำมัน"}, {"Refuelling...", "กำลังเติมน้ำมัน"},
            {"Charge money for fuel", "คิดเงินค่าน้ำมัน"}, {"Fuel price (baht per liter)", "ราคาน้ำมัน (บาทต่อลิตร)"},
            {"Tank size (liters)", "ขนาดถังน้ำมัน (ลิตร)"}, {"Price %s baht per liter", "ราคา %s บาทต่อลิตร"},
            {"Filled", "เติมแล้ว"}, {"Liters", "ลิตร"}, {"Cost", "ค่าน้ำมัน"}, {"Baht", "บาท"}, {"Money left", "เงินคงเหลือ"},
            {"Out of money! Filled what you could afford.", "เงินหมด! เติมได้เท่าที่มี"},
        };
        for (auto &p : kv) m[p[0]] = p[1];
        for (int i = 1; i <= MAXG; i++) {
            m["Gear " + std::to_string(i)] = "เกียร์ " + std::to_string(i);
            m["Move shifter into GEAR " + std::to_string(i)] = "ดันคันเกียร์เข้าเกียร์ " + std::to_string(i);
        }
    }
    auto it = m.find(en);
    return it == m.end() ? nullptr : &it->second;
}
// Thai when "Thai text" is on and the string is known, otherwise the English original
static std::string ThS(const char *en) {
    if (cfg.thai) if (const std::string *p = ThTable(en)) return *p;
    return en;
}
static std::string ThF(const char *en, const char *arg) {
    std::string f = ThS(en);
    char b[256];
    snprintf(b, sizeof b, f.c_str(), arg);
    return b;
}

// ---- GDI+ shaping and rasterising ----
static ULONG_PTR g_gdipToken;
static Gdiplus::PrivateFontCollection *g_thFonts;
static int g_thState;                                   // 0 not started, 1 ready, -1 failed

static bool ThaiInit() {
    if (g_thState) return g_thState > 0;
    Gdiplus::GdiplusStartupInput in;
    if (Gdiplus::GdiplusStartup(&g_gdipToken, &in, nullptr) != Gdiplus::Ok) { g_thState = -1; Log("thai text: GDI+ failed to start"); return false; }
    g_thFonts = new Gdiplus::PrivateFontCollection();
    g_thFonts->AddMemoryFont(FONT_REG, (INT)FONT_REG_LEN);
    g_thFonts->AddMemoryFont(FONT_BOLD, (INT)FONT_BOLD_LEN);
    if (g_thFonts->GetFamilyCount() < 1) { g_thState = -1; Log("thai text: embedded font did not load"); return false; }
    g_thState = 1;
    Log("thai text ready");
    return true;
}

// White glyphs on a transparent background at the size they are shown; the shader tints the white. px = BGRA, straight alpha.
static bool ThaiRender(const std::string &utf8, bool bold, int targetH, std::vector<uint8_t> *px, int *outW) {
    int wl = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wl < 2) return false;
    std::wstring w((size_t)wl - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &w[0], wl);
    Gdiplus::FontFamily fam;
    int found = 0;
    g_thFonts->GetFamilies(1, &fam, &found);
    if (!found) return false;
    Gdiplus::FontStyle st = bold ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular;
    if (!fam.IsStyleAvailable(st)) st = Gdiplus::FontStyleRegular;
    float ratio = (float)fam.GetLineSpacing(st) / (float)fam.GetEmHeight(st);
    float em = std::max(8.0f, (float)(targetH - 2) / (ratio * 1.15f));
    Gdiplus::Font font(&fam, em, st, Gdiplus::UnitPixel);
    Gdiplus::StringFormat sf(Gdiplus::StringFormat::GenericTypographic());
    sf.SetFormatFlags(sf.GetFormatFlags() | Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsMeasureTrailingSpaces);
    Gdiplus::Bitmap probe(8, 8, PixelFormat32bppARGB);
    Gdiplus::Graphics pg(&probe);
    pg.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
    Gdiplus::RectF box;
    pg.MeasureString(w.c_str(), (INT)w.size(), &font, Gdiplus::PointF(0.0f, 0.0f), &sf, &box);
    int W = (int)ceilf(box.Width) + 5;
    if (W < 5 || W > 4000) return false;
    Gdiplus::Bitmap bmp(W, targetH, PixelFormat32bppARGB);
    Gdiplus::Graphics g(&bmp);
    g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
    g.Clear(Gdiplus::Color(0, 0, 0, 0));
    Gdiplus::SolidBrush br(Gdiplus::Color(255, 255, 255, 255));
    g.DrawString(w.c_str(), (INT)w.size(), &font, Gdiplus::PointF(2.0f, 1.0f), &sf, &br);
    if (!bold) g.DrawString(w.c_str(), (INT)w.size(), &font, Gdiplus::PointF(2.4f, 1.0f), &sf, &br);   // slightly thicker stems: the font is thin at small sizes
    Gdiplus::BitmapData bd;
    Gdiplus::Rect rc(0, 0, W, targetH);
    if (bmp.LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bd) != Gdiplus::Ok) return false;
    px->resize((size_t)W * targetH * 4);
    for (int y = 0; y < targetH; y++) memcpy(&(*px)[(size_t)y * W * 4], (const uint8_t *)bd.Scan0 + (ptrdiff_t)y * bd.Stride, (size_t)W * 4);
    bmp.UnlockBits(&bd);
    *outW = W;
    return true;
}

// ---- texture cache (script thread) and draw list handed to the render thread ----
struct ThTex { std::vector<uint8_t> px; int w = 0, h = 0; ID3D11ShaderResourceView *srv = nullptr; };   // srv belongs to the render thread
static std::map<std::string, ThTex> g_thTex;
static std::atomic<int> g_dbgPresents{0}, g_dbgEmpty{0}, g_dbgMiss{0};   // overlay counters, logged while the menu is open
static int g_thBudget = 6;                             // new strings rendered per frame, so opening a page never stalls

static ThTex *ThaiTexture(const std::string &text, bool bold, int targetH, bool *pending) {
    *pending = false;
    std::string key = (bold ? "B" : "R") + std::to_string(targetH) + ":" + text;
    auto it = g_thTex.find(key);
    if (it != g_thTex.end()) return it->second.w > 0 ? &it->second : nullptr;
    if (g_thTex.size() >= 900 || !ThaiInit()) return nullptr;
    if (g_thBudget <= 0) { *pending = true; g_dbgMiss++; return nullptr; }
    g_thBudget--;
    ThTex t;
    if (!ThaiRender(text, bold, targetH, &t.px, &t.w)) { t.w = 0; Log("thai text: could not render '%s'", text.c_str()); }
    t.h = targetH;
    ThTex &slot = g_thTex[key] = std::move(t);
    return slot.w > 0 ? &slot : nullptr;
}

// ---- generic shapes: any GDI+ drawing becomes a cached texture (white, tinted by vertex colours when drawn) ----
struct ThCol { float r, g, b, a; };
struct ThCmd { ThTex *t; float x0, y0, x1, y1, u0, v0, u1, v1; ThCol top, bot; };   // screen pixels, uv, colour at the top / bottom edge
static std::vector<ThCmd> g_thCur, g_thReady;
static std::mutex g_thMx;
static ULONGLONG g_thReadyAt;
static std::atomic<int> g_bbW{0}, g_bbH{0};            // back buffer size, written by the render thread
static int g_thShapeBudget = 10;

static void ThScreen(float *w, float *h) {
    int bw = g_bbW, bh = g_bbH;
    if (bw < 100 || bh < 100) { RECT rc{}; if (g_hwnd) GetClientRect(g_hwnd, &rc); bw = rc.right > 100 ? rc.right : 1920; bh = rc.bottom > 100 ? rc.bottom : 1080; }
    *w = (float)bw; *h = (float)bh;
}
static void ThPush(ThTex *t, float x0, float y0, float x1, float y1, ThCol top, ThCol bot, float u0 = 0.0f, float v0 = 0.0f, float u1 = 1.0f, float v1 = 1.0f) {
    if (t && g_thCur.size() < 5000) g_thCur.push_back({t, x0, y0, x1, y1, u0, v0, u1, v1, top, bot});
}

static int g_thGlyphBudget = 24;
static ThTex *ThShape(const std::string &key, int W, int H, const std::function<void(Gdiplus::Graphics &)> &draw, int *budget = &g_thShapeBudget) {
    auto it = g_thTex.find(key);
    if (it != g_thTex.end()) return it->second.w > 0 ? &it->second : nullptr;
    if (g_thTex.size() >= 3500 || W < 2 || H < 2 || W > 2400 || H > 2400 || !ThaiInit()) return nullptr;
    if (*budget <= 0) { g_dbgMiss++; return nullptr; }
    (*budget)--;
    ThTex t;
    t.w = W; t.h = H;
    {
        Gdiplus::Bitmap bmp(W, H, PixelFormat32bppARGB);
        {
            Gdiplus::Graphics g(&bmp);
            g.SetSmoothingMode(Gdiplus::SmoothingModeHighQuality);
            g.SetPixelOffsetMode(Gdiplus::PixelOffsetModeHighQuality);
            g.Clear(Gdiplus::Color(0, 0, 0, 0));
            draw(g);
        }
        Gdiplus::BitmapData bd;
        Gdiplus::Rect rc(0, 0, W, H);
        if (bmp.LockBits(&rc, Gdiplus::ImageLockModeRead, PixelFormat32bppARGB, &bd) == Gdiplus::Ok) {
            t.px.resize((size_t)W * H * 4);
            for (int y = 0; y < H; y++) memcpy(&t.px[(size_t)y * W * 4], (const uint8_t *)bd.Scan0 + (ptrdiff_t)y * bd.Stride, (size_t)W * 4);
            bmp.UnlockBits(&bd);
        } else t.w = 0;
    }
    ThTex &slot = g_thTex[key] = std::move(t);
    return slot.w > 0 ? &slot : nullptr;
}
static void ThRound(Gdiplus::GraphicsPath &p, float x, float y, float w, float h, float r) {
    r = std::max(0.5f, std::min(r, std::min(w, h) * 0.5f));
    float d = r * 2.0f;
    p.AddArc(x, y, d, d, 180.0f, 90.0f);
    p.AddArc(x + w - d, y, d, d, 270.0f, 90.0f);
    p.AddArc(x + w - d, y + h - d, d, d, 0.0f, 90.0f);
    p.AddArc(x, y + h - d, d, d, 90.0f, 90.0f);
    p.CloseFigure();
}
// soft white glow around a path (drawn as growing translucent outlines); tinted when drawn
static void ThGlow(Gdiplus::Graphics &g, Gdiplus::GraphicsPath &p, int rad, int peak) {
    for (int i = rad; i >= 1; i--) {
        float k = 1.0f - (float)i / (float)(rad + 1);
        Gdiplus::Pen pen(Gdiplus::Color((BYTE)(peak * k * k), 255, 255, 255), (float)i * 2.0f);
        pen.SetLineJoin(Gdiplus::LineJoinRound);
        g.DrawPath(&pen, &p);
    }
}
static ThTex *ThWhite() {
    return ThShape("white", 4, 4, [](Gdiplus::Graphics &g) { Gdiplus::SolidBrush b(Gdiplus::Color(255, 255, 255, 255)); g.FillRectangle(&b, 0, 0, 4, 4); });
}

// seven-segment digit; mask bits a..g
static void ThSegPath(Gdiplus::GraphicsPath &p, int seg, float ox, float oy, float W, float H, float T) {
    float h = T * 0.5f, G = T * 0.12f;
    auto Hs = [&](float y) {
        float xl = ox + h + G, xr = ox + W - h - G;
        Gdiplus::PointF pts[6] = {{xl, y}, {xl + h, y - h}, {xr - h, y - h}, {xr, y}, {xr - h, y + h}, {xl + h, y + h}};
        p.AddPolygon(pts, 6);
    };
    auto Vs = [&](float x, float yt, float yb) {
        Gdiplus::PointF pts[6] = {{x, yt}, {x + h, yt + h}, {x + h, yb - h}, {x, yb}, {x - h, yb - h}, {x - h, yt + h}};
        p.AddPolygon(pts, 6);
    };
    switch (seg) {
    case 0: Hs(oy + h); break;
    case 1: Vs(ox + W - h, oy + h + G, oy + H * 0.5f - G); break;
    case 2: Vs(ox + W - h, oy + H * 0.5f + G, oy + H - h - G); break;
    case 3: Hs(oy + H - h); break;
    case 4: Vs(ox + h, oy + H * 0.5f + G, oy + H - h - G); break;
    case 5: Vs(ox + h, oy + h + G, oy + H * 0.5f - G); break;
    case 6: Hs(oy + H * 0.5f); break;
    }
}
static ThTex *ThDigit(int d, bool ghost, int W, int H, int T, int P) {
    char key[64];
    snprintf(key, sizeof key, "dg%c%d:%d,%d,%d,%d", ghost ? 'G' : 'L', ghost ? 0 : d, W, H, T, P);
    return ThShape(key, W + 2 * P, H + 2 * P, [=](Gdiplus::Graphics &g) {
        static const int mask[10] = {0x3F, 0x06, 0x5B, 0x4F, 0x66, 0x6D, 0x7D, 0x07, 0x7F, 0x6F};
        Gdiplus::GraphicsPath all;
        for (int i = 0; i < 7; i++) if (ghost || ((mask[d % 10] >> i) & 1)) ThSegPath(all, i, (float)P, (float)P, (float)W, (float)H, (float)T);
        if (!ghost) ThGlow(g, all, std::max(2, P - 2), 80);
        Gdiplus::SolidBrush b(Gdiplus::Color(255, 255, 255, 255));
        g.FillPath(&b, &all);
    });
}
// glowing text (Latin or Thai), white, tinted when drawn
static ThTex *ThGlowText(const std::string &utf8, int px, int P) {
    int wl = MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, nullptr, 0);
    if (wl < 2 || !ThaiInit()) return nullptr;
    std::wstring w((size_t)wl - 1, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.c_str(), -1, &w[0], wl);
    Gdiplus::FontFamily fam;
    int found = 0;
    g_thFonts->GetFamilies(1, &fam, &found);
    if (!found) return nullptr;
    float ratio = (float)fam.GetLineSpacing(Gdiplus::FontStyleBold) / (float)fam.GetEmHeight(Gdiplus::FontStyleBold);
    float em = (float)px;
    Gdiplus::Font font(&fam, em, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
    Gdiplus::StringFormat sf(Gdiplus::StringFormat::GenericTypographic());
    sf.SetFormatFlags(sf.GetFormatFlags() | Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsMeasureTrailingSpaces);
    Gdiplus::Bitmap probe(8, 8, PixelFormat32bppARGB);
    Gdiplus::Graphics pg(&probe);
    Gdiplus::RectF box;
    pg.MeasureString(w.c_str(), (INT)w.size(), &font, Gdiplus::PointF(0.0f, 0.0f), &sf, &box);
    int W = (int)ceilf(box.Width) + 2 * P, H = (int)ceilf(em * ratio) + 2 * P;
    return ThShape("gt:" + utf8 + ":" + std::to_string(px) + "," + std::to_string(P), W, H, [=, &fam](Gdiplus::Graphics &g) {
        Gdiplus::GraphicsPath gp;
        Gdiplus::StringFormat f2(Gdiplus::StringFormat::GenericTypographic());
        gp.AddString(w.c_str(), (INT)w.size(), &fam, Gdiplus::FontStyleBold, em, Gdiplus::PointF((float)P, (float)P), &f2);
        ThGlow(g, gp, std::max(2, P - 2), 70);
        Gdiplus::SolidBrush b(Gdiplus::Color(255, 255, 255, 255));
        g.FillPath(&b, &gp);
    });
}
// stacked upright letters inside a capsule (like CLUTCH / BRAKE / THROTTLE labels)
static ThTex *ThStackLabel(const std::string &text, int W, int H, int px, int pitch, int top) {
    if (!ThaiInit()) return nullptr;
    return ThShape("sl:" + text + ":" + std::to_string(W) + "," + std::to_string(H) + "," + std::to_string(px), W, H, [=](Gdiplus::Graphics &g) {
        Gdiplus::FontFamily fam;
        int found = 0;
        g_thFonts->GetFamilies(1, &fam, &found);
        if (!found) return;
        Gdiplus::Font font(&fam, (float)px, Gdiplus::FontStyleBold, Gdiplus::UnitPixel);
        Gdiplus::StringFormat sf(Gdiplus::StringFormat::GenericTypographic());
        sf.SetAlignment(Gdiplus::StringAlignmentCenter);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        Gdiplus::SolidBrush b(Gdiplus::Color(255, 255, 255, 255));
        for (size_t i = 0; i < text.size(); i++) {
            wchar_t ch[2] = {(wchar_t)text[i], 0};
            g.DrawString(ch, 1, &font, Gdiplus::PointF((float)W * 0.5f, (float)(top + (int)i * pitch)), &sf, &b);
        }
    });
}
// translucent rounded body, rim with glow, and a filled rounded bar for sliced drawing
static ThTex *ThRoundBody(int w, int h, float rr, int alpha) {
    return ThShape("rb:" + std::to_string(w) + "," + std::to_string(h) + "," + std::to_string((int)rr) + "," + std::to_string(alpha), w, h, [=](Gdiplus::Graphics &g) {
        Gdiplus::GraphicsPath p;
        ThRound(p, 0, 0, (float)w, (float)h, rr);
        Gdiplus::SolidBrush b(Gdiplus::Color((BYTE)alpha, 0, 0, 0));
        g.FillPath(&b, &p);
    });
}
static ThTex *ThRoundRim(int w, int h, float rr, int P, float stroke) {
    return ThShape("rr:" + std::to_string(w) + "," + std::to_string(h) + "," + std::to_string((int)rr) + "," + std::to_string(P) + "," + std::to_string((int)(stroke * 10)), w + 2 * P, h + 2 * P, [=](Gdiplus::Graphics &g) {
        Gdiplus::GraphicsPath p;
        ThRound(p, (float)P, (float)P, (float)w, (float)h, rr);
        ThGlow(g, p, std::max(2, P - 1), 55);
        Gdiplus::Pen pen(Gdiplus::Color(235, 255, 255, 255), stroke);
        g.DrawPath(&pen, &p);
    });
}
static ThTex *ThRoundFill(int w, int h, float rr) {
    return ThShape("rf:" + std::to_string(w) + "," + std::to_string(h) + "," + std::to_string((int)rr), w, h, [=](Gdiplus::Graphics &g) {
        Gdiplus::GraphicsPath p;
        ThRound(p, 0, 0, (float)w, (float)h, rr);
        Gdiplus::SolidBrush b(Gdiplus::Color(255, 255, 255, 255));
        g.FillPath(&b, &p);
    });
}
static ThCol ThLerp(const ThCol &a, const ThCol &b, float t) { return {a.r + (b.r - a.r) * t, a.g + (b.g - a.g) * t, a.b + (b.b - a.b) * t, a.a + (b.a - a.a) * t}; }
// rounded bar of any height: top and bottom caps stay round, the middle stretches; colour runs top -> bottom
static void ThFillBar(float x, float y, float w, float h, ThCol top, ThCol bot) {
    int iw = std::max(4, (int)(w + 0.5f));
    float rr = iw * 0.38f;
    int ir = std::max(2, (int)rr), th = 2 * ir + 4;
    ThTex *t = ThRoundFill(iw, th, (float)ir);
    if (!t) return;
    if (h <= 2.0f * ir) { ThPush(t, x, y, x + iw, y + h, top, bot); return; }
    float v1 = (float)ir / th, v2 = (float)(ir + 4) / th, v3 = (float)(th - ir) / th;
    float yA = y + ir, yB = y + h - ir;
    ThPush(t, x, y, x + iw, yA, top, ThLerp(top, bot, (float)ir / h), 0, 0, 1, v1);
    ThPush(t, x, yA, x + iw, yB, ThLerp(top, bot, (float)ir / h), ThLerp(top, bot, 1.0f - (float)ir / h), 0, v1, 1, v2);
    ThPush(t, x, yB, x + iw, y + h, ThLerp(top, bot, 1.0f - (float)ir / h), bot, 0, v3, 1, 1);
}

// once per script frame: what was drawn during the last frame is handed to the render thread
static void ThaiFlush() {
    g_thBudget = 6;
    g_thShapeBudget = 10;
    g_thGlyphBudget = 24;
    std::lock_guard<std::mutex> l(g_thMx);
    g_thReady.swap(g_thCur);
    g_thCur.clear();
    g_thReadyAt = GetTickCount64();
}

// ---- Direct3D 11 overlay (render thread, called by ScriptHookV right before Present) ----
struct ThD3D {
    ID3D11Device *dev = nullptr; ID3D11DeviceContext *ctx = nullptr;
    ID3D11VertexShader *vs = nullptr; ID3D11PixelShader *ps = nullptr; ID3D11InputLayout *il = nullptr; ID3D11Buffer *vb = nullptr;
    ID3D11BlendState *bs = nullptr; ID3D11RasterizerState *rs = nullptr; ID3D11SamplerState *ss = nullptr; ID3D11DepthStencilState *ds = nullptr;
};
static ThD3D g_d3d;
static int g_d3dState;                                  // 0 not started, 1 ready, -1 failed

static const UINT TH_MAX_VERTS = 5000 * 6;
static bool ThD3DInit(IDXGISwapChain *sc) {
    if (FAILED(sc->GetDevice(__uuidof(ID3D11Device), (void **)&g_d3d.dev)) || !g_d3d.dev) { g_d3dState = -1; return false; }
    g_d3d.dev->GetImmediateContext(&g_d3d.ctx);
    static const char src[] =
        "struct VI { float2 p : POSITION; float2 uv : TEXCOORD0; float4 c : COLOR0; };\n"
        "struct PI { float4 p : SV_POSITION; float2 uv : TEXCOORD0; float4 c : COLOR0; };\n"
        "PI vs(VI i) { PI o; o.p = float4(i.p, 0, 1); o.uv = i.uv; o.c = i.c; return o; }\n"
        "Texture2D t0 : register(t0); SamplerState s0 : register(s0);\n"
        "float4 ps(PI i) : SV_Target { return t0.Sample(s0, i.uv) * i.c; }\n";
    ID3DBlob *vsb = nullptr, *psb = nullptr, *err = nullptr;
    bool ok = SUCCEEDED(D3DCompile(src, sizeof src - 1, nullptr, nullptr, nullptr, "vs", "vs_4_0", 0, 0, &vsb, &err)) && vsb &&
              SUCCEEDED(D3DCompile(src, sizeof src - 1, nullptr, nullptr, nullptr, "ps", "ps_4_0", 0, 0, &psb, &err)) && psb;
    if (err) { Log("thai overlay shader: %s", (const char *)err->GetBufferPointer()); err->Release(); }
    if (ok) {
        D3D11_INPUT_ELEMENT_DESC il[3] = {
            {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 8, D3D11_INPUT_PER_VERTEX_DATA, 0},
            {"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 16, D3D11_INPUT_PER_VERTEX_DATA, 0}};
        ok = SUCCEEDED(g_d3d.dev->CreateVertexShader(vsb->GetBufferPointer(), vsb->GetBufferSize(), nullptr, &g_d3d.vs)) &&
             SUCCEEDED(g_d3d.dev->CreatePixelShader(psb->GetBufferPointer(), psb->GetBufferSize(), nullptr, &g_d3d.ps)) &&
             SUCCEEDED(g_d3d.dev->CreateInputLayout(il, 3, vsb->GetBufferPointer(), vsb->GetBufferSize(), &g_d3d.il));
    }
    if (vsb) vsb->Release();
    if (psb) psb->Release();
    if (ok) {
        D3D11_BUFFER_DESC bd = {};
        bd.ByteWidth = sizeof(float) * 8 * TH_MAX_VERTS; bd.Usage = D3D11_USAGE_DYNAMIC; bd.BindFlags = D3D11_BIND_VERTEX_BUFFER; bd.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
        D3D11_BLEND_DESC b = {};
        b.RenderTarget[0].BlendEnable = TRUE;
        b.RenderTarget[0].SrcBlend = D3D11_BLEND_SRC_ALPHA; b.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA; b.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
        b.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE; b.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA; b.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
        b.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
        D3D11_RASTERIZER_DESC r = {};
        r.FillMode = D3D11_FILL_SOLID; r.CullMode = D3D11_CULL_NONE; r.DepthClipEnable = TRUE;
        D3D11_SAMPLER_DESC s = {};
        s.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR; s.AddressU = s.AddressV = s.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
        s.ComparisonFunc = D3D11_COMPARISON_ALWAYS; s.MaxLOD = D3D11_FLOAT32_MAX;
        D3D11_DEPTH_STENCIL_DESC d = {};
        d.DepthEnable = FALSE; d.StencilEnable = FALSE;
        ok = SUCCEEDED(g_d3d.dev->CreateBuffer(&bd, nullptr, &g_d3d.vb)) && SUCCEEDED(g_d3d.dev->CreateBlendState(&b, &g_d3d.bs)) &&
             SUCCEEDED(g_d3d.dev->CreateRasterizerState(&r, &g_d3d.rs)) && SUCCEEDED(g_d3d.dev->CreateSamplerState(&s, &g_d3d.ss)) &&
             SUCCEEDED(g_d3d.dev->CreateDepthStencilState(&d, &g_d3d.ds));
    }
    g_d3dState = ok ? 1 : -1;
    Log(ok ? "thai overlay ready" : "thai overlay: Direct3D setup failed, Thai text stays off");
    return ok;
}

static void OnPresent(void *p) {
    IDXGISwapChain *sc = (IDXGISwapChain *)p;
    if (!sc || g_d3dState < 0) return;
    DXGI_SWAP_CHAIN_DESC sd;
    if (SUCCEEDED(sc->GetDesc(&sd))) { g_bbW = (int)sd.BufferDesc.Width; g_bbH = (int)sd.BufferDesc.Height; }
    std::vector<ThCmd> cmds;
    { std::lock_guard<std::mutex> l(g_thMx); if (GetTickCount64() - g_thReadyAt < 120) cmds = g_thReady; }
    // a frame that arrives with nothing to draw repeats the last drawn list for a moment instead of blinking the whole overlay
    static std::vector<ThCmd> lastCmds;
    static ULONGLONG lastAt = 0;
    ULONGLONG nowP = GetTickCount64();
    g_dbgPresents++;
    if (cmds.empty()) {
        g_dbgEmpty++;
        if (nowP - lastAt < 80) cmds = lastCmds;
    } else { lastCmds = cmds; lastAt = nowP; }
    if (cmds.empty()) return;
    if (g_d3dState == 0 && !ThD3DInit(sc)) return;
    const float bw = (float)g_bbW, bh = (float)g_bbH;
    if (bw < 64 || bh < 64) return;
    ID3D11DeviceContext *c = g_d3d.ctx;
    ID3D11Texture2D *bb = nullptr;
    ID3D11RenderTargetView *rtv = nullptr;
    if (FAILED(sc->GetBuffer(0, __uuidof(ID3D11Texture2D), (void **)&bb)) || !bb) return;
    g_d3d.dev->CreateRenderTargetView(bb, nullptr, &rtv);
    bb->Release();
    if (!rtv) return;

    // save the game's pipeline state, draw, restore it
    ID3D11RenderTargetView *oRtv[1] = {}; ID3D11DepthStencilView *oDsv = nullptr;
    c->OMGetRenderTargets(1, oRtv, &oDsv);
    UINT nvp = 16; D3D11_VIEWPORT oVp[16] = {};
    c->RSGetViewports(&nvp, oVp);
    ID3D11RasterizerState *oRs = nullptr; c->RSGetState(&oRs);
    ID3D11BlendState *oBs = nullptr; FLOAT oBf[4] = {}; UINT oMask = 0; c->OMGetBlendState(&oBs, oBf, &oMask);
    ID3D11DepthStencilState *oDs = nullptr; UINT oRef = 0; c->OMGetDepthStencilState(&oDs, &oRef);
    ID3D11VertexShader *oVs = nullptr; c->VSGetShader(&oVs, nullptr, nullptr);
    ID3D11PixelShader *oPs = nullptr; c->PSGetShader(&oPs, nullptr, nullptr);
    ID3D11GeometryShader *oGs = nullptr; c->GSGetShader(&oGs, nullptr, nullptr);
    ID3D11HullShader *oHs = nullptr; c->HSGetShader(&oHs, nullptr, nullptr);
    ID3D11DomainShader *oDms = nullptr; c->DSGetShader(&oDms, nullptr, nullptr);
    ID3D11InputLayout *oIl = nullptr; c->IAGetInputLayout(&oIl);
    D3D11_PRIMITIVE_TOPOLOGY oTop; c->IAGetPrimitiveTopology(&oTop);
    ID3D11Buffer *oVb = nullptr; UINT oStr = 0, oOff = 0; c->IAGetVertexBuffers(0, 1, &oVb, &oStr, &oOff);
    ID3D11ShaderResourceView *oSrv = nullptr; c->PSGetShaderResources(0, 1, &oSrv);
    ID3D11SamplerState *oSs = nullptr; c->PSGetSamplers(0, 1, &oSs);

    D3D11_VIEWPORT vp = {0.0f, 0.0f, bw, bh, 0.0f, 1.0f};
    c->OMSetRenderTargets(1, &rtv, nullptr);
    c->RSSetViewports(1, &vp);
    c->RSSetState(g_d3d.rs);
    const FLOAT bf[4] = {0, 0, 0, 0};
    c->OMSetBlendState(g_d3d.bs, bf, 0xffffffff);
    c->OMSetDepthStencilState(g_d3d.ds, 0);
    c->GSSetShader(nullptr, nullptr, 0); c->HSSetShader(nullptr, nullptr, 0); c->DSSetShader(nullptr, nullptr, 0);
    c->VSSetShader(g_d3d.vs, nullptr, 0);
    c->PSSetShader(g_d3d.ps, nullptr, 0);
    c->IASetInputLayout(g_d3d.il);
    c->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
    UINT stride = sizeof(float) * 8, off = 0;
    c->IASetVertexBuffers(0, 1, &g_d3d.vb, &stride, &off);
    c->PSSetSamplers(0, 1, &g_d3d.ss);

    // textures first (the render thread owns the GPU side), then one buffer fill and one draw per run of the same texture
    for (const ThCmd &k : cmds) {
        ThTex *t = k.t;
        if (!t || t->w <= 0 || t->srv) continue;
        D3D11_TEXTURE2D_DESC td = {};
        td.Width = (UINT)t->w; td.Height = (UINT)t->h; td.MipLevels = 1; td.ArraySize = 1;
        td.Format = DXGI_FORMAT_B8G8R8A8_UNORM; td.SampleDesc.Count = 1; td.Usage = D3D11_USAGE_IMMUTABLE; td.BindFlags = D3D11_BIND_SHADER_RESOURCE;
        D3D11_SUBRESOURCE_DATA sr = {t->px.data(), (UINT)t->w * 4, 0};
        ID3D11Texture2D *tex = nullptr;
        if (SUCCEEDED(g_d3d.dev->CreateTexture2D(&td, &sr, &tex)) && tex) { g_d3d.dev->CreateShaderResourceView(tex, nullptr, &t->srv); tex->Release(); }
        if (!t->srv) t->w = 0;
        else { t->px.clear(); t->px.shrink_to_fit(); }
    }
    struct Run { ID3D11ShaderResourceView *srv; UINT start, count; };
    std::vector<Run> runs;
    D3D11_MAPPED_SUBRESOURCE ms;
    if (SUCCEEDED(c->Map(g_d3d.vb, 0, D3D11_MAP_WRITE_DISCARD, 0, &ms))) {
        float *wp = (float *)ms.pData;
        UINT nv = 0;
        for (const ThCmd &k : cmds) {
            ThTex *t = k.t;
            if (!t || t->w <= 0 || !t->srv || nv + 6 > TH_MAX_VERTS) continue;
            float x0 = k.x0 / bw * 2.0f - 1.0f, x1 = k.x1 / bw * 2.0f - 1.0f;
            float y0 = 1.0f - k.y0 / bh * 2.0f, y1 = 1.0f - k.y1 / bh * 2.0f;
            const ThCol &a = k.top, &b2 = k.bot;
            const float v[6][8] = {
                {x0, y0, k.u0, k.v0, a.r, a.g, a.b, a.a},  {x1, y0, k.u1, k.v0, a.r, a.g, a.b, a.a},  {x0, y1, k.u0, k.v1, b2.r, b2.g, b2.b, b2.a},
                {x1, y0, k.u1, k.v0, a.r, a.g, a.b, a.a},  {x1, y1, k.u1, k.v1, b2.r, b2.g, b2.b, b2.a}, {x0, y1, k.u0, k.v1, b2.r, b2.g, b2.b, b2.a}};
            memcpy(wp + (size_t)nv * 8, v, sizeof v);
            if (!runs.empty() && runs.back().srv == t->srv) runs.back().count += 6;
            else runs.push_back({t->srv, nv, 6});
            nv += 6;
        }
        c->Unmap(g_d3d.vb, 0);
        for (const Run &r : runs) { c->PSSetShaderResources(0, 1, &r.srv); c->Draw(r.count, r.start); }
    }
    c->OMSetRenderTargets(1, oRtv, oDsv);
    c->RSSetViewports(nvp, oVp);
    c->RSSetState(oRs);
    c->OMSetBlendState(oBs, oBf, oMask);
    c->OMSetDepthStencilState(oDs, oRef);
    c->VSSetShader(oVs, nullptr, 0); c->PSSetShader(oPs, nullptr, 0);
    c->GSSetShader(oGs, nullptr, 0); c->HSSetShader(oHs, nullptr, 0); c->DSSetShader(oDms, nullptr, 0);
    c->IASetInputLayout(oIl);
    c->IASetPrimitiveTopology(oTop);
    c->IASetVertexBuffers(0, 1, &oVb, &oStr, &oOff);
    c->PSSetShaderResources(0, 1, &oSrv);
    c->PSSetSamplers(0, 1, &oSs);
    if (oRtv[0]) oRtv[0]->Release();
    for (IUnknown *u : {(IUnknown *)oDsv, (IUnknown *)oRs, (IUnknown *)oBs, (IUnknown *)oDs, (IUnknown *)oVs, (IUnknown *)oPs, (IUnknown *)oGs,
                        (IUnknown *)oHs, (IUnknown *)oDms, (IUnknown *)oIl, (IUnknown *)oVb, (IUnknown *)oSrv, (IUnknown *)oSs})
        if (u) u->Release();
    rtv->Release();
}

// Called by DrawStr(). True when it handled the string (Thai queued, or Thai that cannot be shown with the game font).
static bool DrawThaiStr(float x, float y, const std::string &s, float scale, int r, int g, int b, bool center) {
    bool ascii = true;
    for (unsigned char c : s) if (c >= 0x80) { ascii = false; break; }
    std::string th;
    if (!ascii) th = s;
    else {
        if (!cfg.thai || g_d3dState < 0) return false;
        const std::string *p = ThTable(s);
        if (!p) return false;
        th = *p;
    }
    if (th.empty()) return true;
    float sw, sh;
    ThScreen(&sw, &sh);
    int targetH = std::max(12, (int)(scale * 0.115f * std::clamp(cfg.thaiSize, 0.5f, 2.0f) * sh + 0.5f) & ~1);   // line height in screen pixels
    bool pending = false;
    ThTex *t = ThaiTexture(th, cfg.thaiBold || scale >= 0.5f, targetH, &pending);
    if (!t) return pending || !ascii;
    float left = floorf((center ? x * sw - t->w * 0.5f : x * sw) + 0.5f);       // whole pixels
    float top = floorf(y * sh - t->h * 0.10f + 0.5f);
    ThCol c = {r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
    ThPush(t, left, top, left + t->w, top + t->h, c, c);
    return true;
}

// ---- plain text from per-glyph textures: any ASCII string drawn live (numbers, names) without caching whole strings ----
static float ThAdvance(unsigned char ch, int targetH, bool bold) {
    static std::map<uint32_t, float> cache;
    uint32_t key = (uint32_t)ch | ((uint32_t)targetH << 8) | (bold ? (1u << 28) : 0u);
    auto it = cache.find(key);
    if (it != cache.end()) return it->second;
    float adv = targetH * 0.3f;
    if (ThaiInit()) {
        Gdiplus::FontFamily fam;
        int found = 0;
        g_thFonts->GetFamilies(1, &fam, &found);
        if (found) {
            Gdiplus::FontStyle st = bold && fam.IsStyleAvailable(Gdiplus::FontStyleBold) ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular;
            float ratio = (float)fam.GetLineSpacing(st) / (float)fam.GetEmHeight(st);
            float em = std::max(8.0f, (float)(targetH - 2) / (ratio * 1.15f));
            Gdiplus::Font font(&fam, em, st, Gdiplus::UnitPixel);
            Gdiplus::StringFormat sf(Gdiplus::StringFormat::GenericTypographic());
            sf.SetFormatFlags(sf.GetFormatFlags() | Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsMeasureTrailingSpaces);
            static Gdiplus::Bitmap probe(8, 8, PixelFormat32bppARGB);
            Gdiplus::Graphics pg(&probe);
            pg.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
            wchar_t w[2] = {(wchar_t)ch, 0};
            Gdiplus::RectF box;
            pg.MeasureString(w, 1, &font, Gdiplus::PointF(0.0f, 0.0f), &sf, &box);
            adv = box.Width;
        }
    }
    cache[key] = adv;
    return adv;
}
static ThTex *ThGlyph(unsigned char ch, int targetH, bool bold, float adv) {
    int W = (int)ceilf(adv) + 3;
    return ThShape("gl" + std::to_string((int)ch) + ":" + std::to_string(targetH) + (bold ? "b" : "r"), W, targetH, [=](Gdiplus::Graphics &g) {
        Gdiplus::FontFamily fam;
        int found = 0;
        g_thFonts->GetFamilies(1, &fam, &found);
        if (!found) return;
        Gdiplus::FontStyle st = bold && fam.IsStyleAvailable(Gdiplus::FontStyleBold) ? Gdiplus::FontStyleBold : Gdiplus::FontStyleRegular;
        float ratio = (float)fam.GetLineSpacing(st) / (float)fam.GetEmHeight(st);
        float em = std::max(8.0f, (float)(targetH - 2) / (ratio * 1.15f));
        Gdiplus::Font font(&fam, em, st, Gdiplus::UnitPixel);
        Gdiplus::StringFormat sf(Gdiplus::StringFormat::GenericTypographic());
        sf.SetFormatFlags(sf.GetFormatFlags() | Gdiplus::StringFormatFlagsNoWrap | Gdiplus::StringFormatFlagsMeasureTrailingSpaces);
        g.SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAliasGridFit);
        Gdiplus::SolidBrush br(Gdiplus::Color(255, 255, 255, 255));
        wchar_t w[2] = {(wchar_t)ch, 0};
        g.DrawString(w, 1, &font, Gdiplus::PointF(1.0f, 1.0f), &sf, &br);
        if (!bold) g.DrawString(w, 1, &font, Gdiplus::PointF(1.4f, 1.0f), &sf, &br);
    }, &g_thGlyphBudget);
}
static void DrawGlyphStr(float x, float y, const std::string &s, float scale, int r, int g, int b, bool center) {
    float sw, sh;
    ThScreen(&sw, &sh);
    int targetH = std::max(12, (int)(scale * 0.115f * std::clamp(cfg.thaiSize, 0.5f, 2.0f) * sh + 0.5f) & ~1);
    bool bold = cfg.thaiBold || scale >= 0.5f;
    float total = 0.0f;
    for (unsigned char ch : s) if (ch >= 32 && ch < 127) total += ThAdvance(ch, targetH, bold);
    float cur = center ? x * sw - total * 0.5f : x * sw;
    float top = floorf(y * sh - targetH * 0.10f + 0.5f);
    ThCol c = {r / 255.0f, g / 255.0f, b / 255.0f, 1.0f};
    for (unsigned char ch : s) {
        if (ch < 32 || ch >= 127) continue;
        float adv = ThAdvance(ch, targetH, bold);
        if (ch != ' ')
            if (ThTex *t = ThGlyph(ch, targetH, bold, adv)) {
                float gx = floorf(cur + 0.5f) - 1.0f;
                ThPush(t, gx, top, gx + t->w, top + t->h, c, c);
            }
        cur += adv;
    }
}