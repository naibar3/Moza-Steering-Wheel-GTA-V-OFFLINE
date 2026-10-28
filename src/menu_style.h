// menu_style.h: glass / neon building blocks for the F11 menu, drawn with the overlay from thai.h.
// Coordinates are fractions of the screen like the rest of the menu code. Included from main.cpp after thai.h.
#pragma once

static ThCol Rgb(int r, int g, int b, float a = 1.0f) { return {r / 255.0f, g / 255.0f, b / 255.0f, a}; }

// One rounded element: translucent black body (bodyAlpha 0 = none), gradient fill (fTop.a 0 = none), glowing rim (rim.a 0 = none).
// rad is the corner radius as a fraction of the screen height.
static void NShape(float x, float y, float w, float h, float rad, int bodyAlpha, ThCol fTop, ThCol fBot, ThCol rim, float stroke, bool glow) {
    float sw, sh;
    ThScreen(&sw, &sh);
    float px = floorf(x * sw + 0.5f), py = floorf(y * sh + 0.5f);
    int iw = std::max(4, (int)(w * sw + 0.5f)), ih = std::max(4, (int)(h * sh + 0.5f));
    float rp = std::max(1.0f, rad * sh);
    const ThCol white = {1, 1, 1, 1};
    if (bodyAlpha > 0)
        if (ThTex *b = ThRoundBody(iw, ih, rp, bodyAlpha)) ThPush(b, px, py, px + iw, py + ih, white, white);
    if (fTop.a > 0.0f)
        if (ThTex *f = ThRoundFill(iw, ih, rp)) ThPush(f, px, py, px + iw, py + ih, fTop, fBot);
    if (rim.a > 0.0f) {
        int P = glow ? 9 : 3;
        if (ThTex *r = ThRoundRim(iw, ih, rp, P, stroke)) ThPush(r, px - P, py - P, px + iw + P, py + ih + P, rim, rim);
    }
}

// mouse pointer drawn by the overlay (the game's own pointer would sit under the menu panel)
static void NCursor(float mx, float my) {
    float sw, sh;
    ThScreen(&sw, &sh);
    float k = std::max(0.8f, sh / 1080.0f);
    int W = (int)(22 * k + 0.5f), H = (int)(32 * k + 0.5f);
    ThTex *t = ThShape("cursor:" + std::to_string(W) + "," + std::to_string(H), W, H, [=](Gdiplus::Graphics &g) {
        Gdiplus::PointF p[7] = {{1.5f * k, 1.5f * k}, {1.5f * k, 21.0f * k}, {6.0f * k, 16.5f * k}, {9.5f * k, 25.0f * k}, {13.0f * k, 23.5f * k}, {9.5f * k, 15.0f * k}, {15.5f * k, 15.0f * k}};
        Gdiplus::GraphicsPath gp;
        gp.AddPolygon(p, 7);
        Gdiplus::Pen pen(Gdiplus::Color(255, 0, 0, 0), 2.4f * k);
        pen.SetLineJoin(Gdiplus::LineJoinRound);
        g.DrawPath(&pen, &gp);
        Gdiplus::SolidBrush b(Gdiplus::Color(255, 255, 255, 255));
        g.FillPath(&b, &gp);
    });
    const ThCol white = {1, 1, 1, 1};
    float x = floorf(mx * sw + 0.5f), y = floorf(my * sh + 0.5f);
    if (t) ThPush(t, x, y, x + W, y + H, white, white);
}
