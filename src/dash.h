// dash.h: neon dashboard drawn with the Direct3D overlay from thai.h: speed in seven-segment digits, gear, rpm bar,
// pedal capsules (clutch / brake / throttle / handbrake) and a fuel capsule. Included from main.cpp (needs cfg, g_wheel and the g_live* values).
// Everything is laid out in units of a 1080p screen and scaled with the screen height and cfg.hudScale.
#pragma once

static float g_hudHandbrake;               // 0..1, written by the main loop

static ThCol DashGrad(const ThCol *s, int n, float t) {
    float f = std::clamp(t, 0.0f, 1.0f) * (float)(n - 1);
    int i = std::min((int)f, n - 2);
    return ThLerp(s[i], s[i + 1], f - (float)i);
}

// glass capsule with a rounded fill that rises with `level`, stacked letters and a glowing rim
static void DashCapsule(float x, float y, float w, float h, float S, float level, ThCol cTop, ThCol cBot, ThCol rim, const char *label) {
    int iw = (int)(w + 0.5f), ih = (int)(h + 0.5f), P = std::max(4, (int)(9.0f * S + 0.5f));
    float rr = iw * 0.42f;
    const ThCol white = {1, 1, 1, 1};
    if (ThTex *b = ThRoundBody(iw, ih, rr, 150)) ThPush(b, x, y, x + iw, y + ih, white, white);
    int inset = std::max(3, (int)(6.0f * S + 0.5f));
    float maxH = (float)(ih - 2 * inset), fh = std::clamp(level, 0.0f, 1.0f) * maxH;
    if (fh > 4.0f) ThFillBar(x + inset, y + ih - inset - fh, (float)(iw - 2 * inset), fh, cTop, cBot);
    int px = std::max(9, (int)(15.0f * S + 0.5f)), pitch = (int)(px * 1.28f), topPad = (int)(9.0f * S + 0.5f);
    ThCol lc = {1, 1, 1, 0.92f};
    if (ThTex *lb = ThStackLabel(label, iw, ih, px, pitch, topPad)) ThPush(lb, x, y, x + iw, y + ih, lc, lc);
    if (ThTex *rm = ThRoundRim(iw, ih, rr, P, std::max(1.5f, 2.0f * S))) ThPush(rm, x - P, y - P, x + iw + P, y + ih + P, rim, rim);
}

// three seven-segment digits with unlit ghost segments behind: magenta, cyan to green, yellow to orange
static void DashDigits(float x, float y, float S, int value) {
    int dw = (int)(64.0f * S + 0.5f), dh = (int)(112.0f * S + 0.5f), T = std::max(4, (int)(15.0f * S + 0.5f)), P = std::max(5, (int)(12.0f * S + 0.5f));
    float pitch = (float)dw + 12.0f * S;
    static const ThCol top[3] = {{1.00f, 0.35f, 0.92f, 1.0f}, {0.24f, 0.90f, 1.00f, 1.0f}, {1.00f, 0.95f, 0.24f, 1.0f}};
    static const ThCol bot[3] = {{0.90f, 0.16f, 0.78f, 1.0f}, {0.24f, 1.00f, 0.67f, 1.0f}, {1.00f, 0.59f, 0.12f, 1.0f}};
    int d[3] = {value / 100 % 10, value / 10 % 10, value % 10};
    for (int i = 0; i < 3; i++) {
        float px = x + i * pitch - P, py = y - P, pw = (float)(dw + 2 * P), ph = (float)(dh + 2 * P);
        ThCol gt = top[i], gb = bot[i];
        gt.a = gb.a = 0.13f;
        if (ThTex *g = ThDigit(0, true, dw, dh, T, P)) ThPush(g, px, py, px + pw, py + ph, gt, gb);
        if (ThTex *l = ThDigit(d[i], false, dw, dh, T, P)) ThPush(l, px, py, px + pw, py + ph, top[i], bot[i]);
    }
}

// big gear number: seven-segment digits, N and R as glowing letters
static void DashGear(float x, float y, float S, int gear) {
    ThCol gt = Rgb(255, 235, 70), gb = Rgb(255, 150, 30);
    int dh = (int)(56.0f * S + 0.5f), dw = (int)(31.0f * S + 0.5f), T = std::max(3, (int)(8.0f * S + 0.5f)), P = std::max(4, (int)(8.0f * S + 0.5f));
    if (gear <= 0) {
        if (ThTex *t = ThGlowText(gear == 0 ? "N" : "R", (int)(78.0f * S + 0.5f), P)) {
            float top = y + dh * 0.5f - t->h * 0.5f;
            ThPush(t, x - P, top, x - P + t->w, top + t->h, gt, gb);
        }
        return;
    }
    int ds[2] = {gear >= 10 ? gear / 10 : -1, gear % 10};
    float cx = x;
    for (int i = 0; i < 2; i++) {
        if (ds[i] < 0) continue;
        if (ThTex *l = ThDigit(ds[i], false, dw, dh, T, P)) ThPush(l, cx - P, y - P, cx - P + dw + 2 * P, y - P + dh + 2 * P, gt, gb);
        cx += dw + 8.0f * S;
    }
}

static void DashRpm(float x, float y, float w, float h, float S, float rpm) {
    int iw = (int)(w + 0.5f), ih = (int)(h + 0.5f), P = std::max(4, (int)(8.0f * S + 0.5f));
    rpm = std::clamp(rpm, 0.0f, 1.0f);
    float inset = std::max(2.0f, 3.0f * S);
    const ThCol white = {1, 1, 1, 1};
    if (ThTex *b = ThRoundBody(iw, ih, ih * 0.5f, 140)) ThPush(b, x, y, x + iw, y + ih, white, white);
    static const ThCol stops[5] = {{0.24f, 1.0f, 0.35f, 1.0f}, {1.0f, 0.94f, 0.24f, 1.0f}, {0.24f, 0.86f, 1.0f, 1.0f}, {0.35f, 0.50f, 1.0f, 1.0f}, {1.0f, 0.24f, 0.86f, 1.0f}};
    ThTex *wh = ThWhite();
    const int N = 28;
    float ix = x + inset, iwid = (float)iw - 2.0f * inset;
    for (int i = 0; i < N; i++) {
        float t = (i + 0.5f) / N;
        ThCol c = DashGrad(stops, 5, t);
        c.a = t <= rpm ? 1.0f : 0.20f;
        ThPush(wh, ix + iwid * i / N, y + inset, ix + iwid * (i + 1) / N + 0.5f, y + ih - inset, c, c);
    }
    ThCol rimc = {0.8f, 0.9f, 1.0f, 0.8f};
    if (ThTex *r = ThRoundRim(iw, ih, ih * 0.5f, P, std::max(1.5f, 2.0f * S))) ThPush(r, x - P, y - P, x + iw + P, y + ih + P, rimc, rimc);
    // knob at the current rpm and a thin mark at the redline
    float kx = ix + iwid * rpm, kw = std::max(6.0f, 10.0f * S), kh = ih + 14.0f * S;
    ThCol kc = {0.85f, 0.95f, 1.0f, 1.0f};
    if (ThTex *k = ThRoundFill((int)kw, (int)kh, kw * 0.5f)) ThPush(k, kx - kw * 0.5f, y + ih * 0.5f - kh * 0.5f, kx + kw * 0.5f, y + ih * 0.5f + kh * 0.5f, kc, kc);
    float rx = ix + iwid * 0.85f, rw = std::max(3.0f, 5.0f * S), rh = ih + 8.0f * S;
    ThCol rc = {1.0f, 1.0f, 1.0f, 0.75f};
    if (ThTex *k = ThRoundFill((int)rw, (int)rh, rw * 0.5f)) ThPush(k, rx - rw * 0.5f, y + ih * 0.5f - rh * 0.5f, rx + rw * 0.5f, y + ih * 0.5f + rh * 0.5f, rc, rc);
}

// glowing text; align 0 = x is the left edge, 1 = centre, 2 = right edge; y is the vertical centre
static void DashText(const std::string &s, int px, float x, float y, ThCol c, int align) {
    int P = std::max(3, px / 4);
    ThTex *t = ThGlowText(s, px, P);
    if (!t) return;
    float left = align == 0 ? x - P : (align == 1 ? x - t->w * 0.5f : x - t->w + P);
    float top = y - t->h * 0.5f;
    ThPush(t, left, top, left + t->w, top + t->h, c, c);
}
// seven-segment number drawn right to left from xRight (digits and '.'), height h
static void DashNumber(float xRight, float y, float h, const std::string &s, ThCol top, ThCol bot) {
    int dh = std::max(10, (int)(h + 0.5f)), dw = (int)(dh * 0.55f + 0.5f), T = std::max(3, (int)(dh * 0.14f + 0.5f)), P = std::max(3, dh / 7);
    float gapx = dh * 0.2f, x = xRight;
    for (int i = (int)s.size() - 1; i >= 0; i--) {
        char ch = s[i];
        if (ch == '.') {
            int sz = std::max(3, (int)(T * 1.2f));
            x -= sz;
            if (ThTex *d = ThRoundFill(sz, sz, sz * 0.5f)) ThPush(d, x, y + dh - sz, x + sz, y + dh, bot, bot);
            x -= gapx * 0.7f;
        } else if (ch >= '0' && ch <= '9') {
            x -= dw;
            if (ThTex *d = ThDigit(ch - '0', false, dw, dh, T, P)) ThPush(d, x - P, y - P, x - P + dw + 2 * P, y - P + dh + 2 * P, top, bot);
            x -= gapx;
        }
    }
}
// horizontal glass bar with a gradient fill (the refuel progress)
static void DashBar(float x, float y, float w, float h, float S, float frac, const ThCol *stops, int ns) {
    int iw = (int)(w + 0.5f), ih = (int)(h + 0.5f), P = std::max(4, (int)(8.0f * S + 0.5f));
    frac = std::clamp(frac, 0.0f, 1.0f);
    float inset = std::max(2.0f, 3.0f * S);
    const ThCol white = {1, 1, 1, 1};
    if (ThTex *b = ThRoundBody(iw, ih, ih * 0.5f, 140)) ThPush(b, x, y, x + iw, y + ih, white, white);
    ThTex *wh = ThWhite();
    const int N = 40;
    float ix = x + inset, iwid = (float)iw - 2.0f * inset;
    for (int i = 0; i < N; i++) {
        float t = (i + 0.5f) / N;
        if (t > frac) break;
        ThCol c = DashGrad(stops, ns, t);
        ThPush(wh, ix + iwid * i / N, y + inset, ix + iwid * (i + 1) / N + 0.5f, y + ih - inset, c, c);
    }
    ThCol rimc = {0.8f, 1.0f, 0.9f, 0.85f};
    if (ThTex *r = ThRoundRim(iw, ih, ih * 0.5f, P, std::max(1.5f, 2.0f * S))) ThPush(r, x - P, y - P, x + iw + P, y + ih + P, rimc, rimc);
}

// refuelling panel in the dashboard style; with frac < 0 only the hint near a pump (msg, msg2)
static void DashRefuelUi(float frac, float liters, float cost, int cash, bool broke, bool pay, const std::string &msg, const std::string &msg2) {
    float sw, sh;
    ThScreen(&sw, &sh);
    float S = floorf(sh / 1080.0f * std::clamp(cfg.hudScale, 0.5f, 2.0f) * 40.0f + 0.5f) / 40.0f;
    float cx = sw * 0.5f;
    if (frac < 0.0f) {
        if (!msg.empty()) DashText(msg, (int)(38.0f * S + 0.5f), cx, sh * 0.775f, Rgb(255, 255, 255), 1);
        if (!msg2.empty()) DashText(msg2, (int)(26.0f * S + 0.5f), cx, sh * 0.775f + 44.0f * S, Rgb(150, 245, 180), 1);
        return;
    }
    const float half = 230.0f * S;
    float y0 = sh * 0.60f;
    DashText(ThS("Refuelling..."), (int)(44.0f * S + 0.5f), cx - half, y0 + 34.0f * S, Rgb(110, 255, 150), 0);
    char pb[16];
    snprintf(pb, sizeof pb, "%d", (int)(frac * 100.0f + 0.5f));
    DashNumber(cx + half - 30.0f * S, y0 + 6.0f * S, 56.0f * S, pb, Rgb(150, 255, 190), Rgb(40, 220, 120));
    DashText("%", (int)(34.0f * S + 0.5f), cx + half - 14.0f * S, y0 + 40.0f * S, Rgb(110, 255, 150), 2);
    static const ThCol stops[3] = {{0.24f, 1.0f, 0.45f, 1.0f}, {0.45f, 1.0f, 0.85f, 1.0f}, {0.30f, 0.80f, 1.0f, 1.0f}};
    DashBar(cx - half, y0 + 84.0f * S, 2.0f * half, 22.0f * S, S, frac, stops, 3);
    float ry = y0 + 138.0f * S;
    const ThCol lab = Rgb(205, 212, 225);
    auto Row = [&](const char *label, const std::string &num, const char *unit, ThCol t, ThCol b) {
        DashText(ThS(label), (int)(28.0f * S + 0.5f), cx - half, ry + 16.0f * S, lab, 0);
        DashNumber(cx + 70.0f * S, ry, 32.0f * S, num, t, b);
        DashText(ThS(unit), (int)(26.0f * S + 0.5f), cx + 92.0f * S, ry + 16.0f * S, lab, 0);
        ry += 46.0f * S;
    };
    char nb[24];
    snprintf(nb, sizeof nb, "%.1f", liters);
    Row("Filled", nb, "Liters", Rgb(255, 255, 255), Rgb(190, 215, 255));
    if (pay) {
        snprintf(nb, sizeof nb, "%.0f", cost);
        Row("Cost", nb, "Baht", Rgb(255, 230, 100), Rgb(255, 160, 30));
        if (cash >= 0) {
            snprintf(nb, sizeof nb, "%d", cash);
            Row("Money left", nb, "Baht", broke ? Rgb(255, 120, 110) : Rgb(130, 255, 170), broke ? Rgb(255, 45, 45) : Rgb(60, 220, 120));
        }
    }
    if (broke) DashText(ThS("Out of money! Filled what you could afford."), (int)(30.0f * S + 0.5f), cx, ry + 10.0f * S, Rgb(255, 110, 110), 1);
}

static void DrawDashNeon() {
    if (!g_dashActive && !g_hudShow) return;
    float sw, sh;
    ThScreen(&sw, &sh);
    float S = floorf(sh / 1080.0f * std::clamp(cfg.hudScale, 0.5f, 2.0f) * 40.0f + 0.5f) / 40.0f;
    float R = sw - 48.0f * S, B = sh - 40.0f * S;
    bool showFuel = g_dashFuel >= 0.0f && cfg.hudFuel, wheel = g_wheel.ok, shifter = g_hudShow;
    int n = (wheel ? 4 : 0) + (showFuel ? 1 : 0);
    float cw = 44.0f * S, ch = 188.0f * S, gap = 10.0f * S;
    float capsW = n ? n * cw + (n - 1) * gap : 0.0f;
    float capX = R - capsW, capY = B - ch - 12.0f * S;
    float xs = n ? capX - 36.0f * S : R;                           // right edge of the speed block
    float digitsW = 3.0f * 64.0f * S + 2.0f * 12.0f * S, xD = xs - digitsW, yD = capY;
    float rowY = yD + 126.0f * S;

    if (cfg.hudSpeedo) {
        float v = cfg.mph ? g_dashSpeed * 2.23694f : g_dashSpeed * 3.6f;
        DashDigits(xD, yD, S, std::clamp((int)(v + 0.5f), 0, 999));
        int P = std::max(4, (int)(8.0f * S + 0.5f));
        if (ThTex *t = ThGlowText(cfg.mph ? "MPH" : "KMH", (int)(30.0f * S + 0.5f), P)) {
            ThCol c = Rgb(60, 220, 255);
            float left = xs - t->w + P, top = rowY + 28.0f * S - t->h * 0.5f;
            ThPush(t, left, top, left + t->w, top + t->h, c, c);
        }
    }
    if (shifter) {
        int P = std::max(4, (int)(8.0f * S + 0.5f));
        if (ThTex *t = ThGlowText("GEAR", (int)(26.0f * S + 0.5f), P)) {
            ThCol c = Rgb(90, 255, 110);
            float left = xD - P, top = rowY + 28.0f * S - t->h * 0.5f;
            ThPush(t, left, top, left + t->w, top + t->h, c, c);
            DashGear(left + t->w - P + 14.0f * S, rowY, S, g_hudEngaged);
        }
        DashRpm(xD, rowY + 56.0f * S + 8.0f * S, digitsW, 16.0f * S, S, g_hudRpm);
    }

    // status line above the digits
    {
        const char *msg = g_hudClutchMsg ? "PRESS CLUTCH" : (!g_dashRunning ? (g_noFuel ? "NO FUEL" : "ENGINE OFF") : nullptr);
        if (msg) DashText(ThS(msg), (int)(34.0f * S + 0.5f), xD + digitsW * 0.5f, yD - 34.0f * S, g_hudClutchMsg ? Rgb(255, 170, 90) : Rgb(255, 110, 110), 1);
    }


    // capsules
    int ci = 0;
    auto Cap = [&](const char *label, float level, ThCol top, ThCol bot, ThCol rim) {
        DashCapsule(capX + ci * (cw + gap), capY, cw, ch, S, level, top, bot, rim, label);
        ci++;
    };
    if (wheel) {
        Cap("CLUTCH", g_liveClutch, Rgb(90, 200, 255), Rgb(20, 120, 255), Rgb(80, 190, 255, 0.9f));
        Cap("BRAKE", g_liveBrk, Rgb(255, 110, 120), Rgb(255, 40, 70), Rgb(255, 90, 100, 0.9f));
        Cap("THROTTLE", g_liveThr, Rgb(150, 255, 120), Rgb(50, 225, 60), Rgb(110, 255, 110, 0.9f));
        Cap("HANDBRAKE", g_hudHandbrake, Rgb(235, 240, 250), Rgb(160, 170, 190), Rgb(205, 215, 230, 0.7f));
    }
    if (showFuel) {
        float f = std::clamp(g_dashFuel, 0.0f, 1.0f);
        bool low = f < 0.15f, blinkOff = f < 0.08f && ((GetTickCount64() / 400) % 2);
        ThCol top = low ? Rgb(255, 120, 110) : Rgb(255, 225, 90), bot = low ? Rgb(255, 45, 45) : Rgb(255, 160, 30);
        ThCol rim = low ? Rgb(255, 90, 90, 0.9f) : Rgb(255, 190, 70, 0.9f);
        float fx = capX + ci * (cw + gap);
        Cap("FUEL", f, top, bot, rim);
        char b[16];
        snprintf(b, sizeof b, "%d%%", (int)(f * 100.0f + 0.5f));
        if (!blinkOff) DashText(b, (int)(26.0f * S + 0.5f), fx + cw * 0.5f, capY - 56.0f * S, low ? Rgb(255, 110, 100) : Rgb(255, 225, 150), 1);
    }
}
