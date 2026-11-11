#include "gfx.hpp"

#include <algorithm>

#include <cstdio>
#include <cstdlib>

#include "sprite.hpp"

namespace gfx {

// Optional diagnostics: set the GF_TRACE environment variable to a file path to log renderer problems.
static void trace(const char* what, long hr) {
    const char* path = std::getenv("GF_TRACE");
    if (!path) return;
    if (FILE* f = std::fopen(path, "a")) {
        std::fprintf(f, "%s hr=0x%08lx\n", what, static_cast<unsigned long>(hr));
        std::fclose(f);
    }
}

std::wstring widen(const std::string& s) {
    if (s.empty()) return {};
    const int n = MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), nullptr, 0);
    std::wstring w(static_cast<size_t>(n), L'\0');
    MultiByteToWideChar(CP_UTF8, 0, s.data(), static_cast<int>(s.size()), w.data(), n);
    return w;
}

static D2D1_COLOR_F d2d(Color c) { return D2D1_COLOR_F{c.r, c.g, c.b, c.a}; }
static D2D1_RECT_F rect(float x, float y, float w, float h) { return D2D1_RECT_F{x, y, x + w, y + h}; }

Gfx::~Gfx() {
    releaseTarget();
    for (auto& kv : formats_) kv.second->Release();
    if (dwrite_) dwrite_->Release();
    if (factory_) factory_->Release();
}

void Gfx::releaseTarget() {
    for (auto& kv : bitmaps_) kv.second->Release();
    bitmaps_.clear();
    if (brush_) { brush_->Release(); brush_ = nullptr; }
    if (rt_) { rt_->Release(); rt_ = nullptr; }
    if (memDc_) {
        if (oldBmp_) SelectObject(memDc_, oldBmp_);
        DeleteDC(memDc_);
        memDc_ = nullptr;
    }
    if (dib_) { DeleteObject(dib_); dib_ = nullptr; }
    oldBmp_ = nullptr;
    bits_ = nullptr;
}

bool Gfx::init() {
    if (FAILED(D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, __uuidof(ID2D1Factory), nullptr,
                                 reinterpret_cast<void**>(&factory_))))
        return false;
    if (FAILED(DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory),
                                   reinterpret_cast<IUnknown**>(&dwrite_))))
        return false;
    return true;
}

bool Gfx::resize(int pixelW, int pixelH) {
    if (pixelW <= 0 || pixelH <= 0) return false;
    if (rt_ && pixelW == pw_ && pixelH == ph_) return true;
    releaseTarget();
    pw_ = pixelW;
    ph_ = pixelH;

    HDC screen = GetDC(nullptr);
    memDc_ = CreateCompatibleDC(screen);
    BITMAPINFO bmi = {};
    bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi.bmiHeader.biWidth = pw_;
    bmi.bmiHeader.biHeight = -ph_;  // top-down
    bmi.bmiHeader.biPlanes = 1;
    bmi.bmiHeader.biBitCount = 32;
    bmi.bmiHeader.biCompression = BI_RGB;
    dib_ = CreateDIBSection(screen, &bmi, DIB_RGB_COLORS, &bits_, nullptr, 0);
    ReleaseDC(nullptr, screen);
    if (!dib_ || !memDc_) return false;
    oldBmp_ = static_cast<HBITMAP>(SelectObject(memDc_, dib_));

    D2D1_RENDER_TARGET_PROPERTIES props = {};
    props.type = D2D1_RENDER_TARGET_TYPE_DEFAULT;
    props.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    props.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    props.dpiX = 96.f;
    props.dpiY = 96.f;
    props.usage = D2D1_RENDER_TARGET_USAGE_GDI_COMPATIBLE;
    props.minLevel = D2D1_FEATURE_LEVEL_DEFAULT;
    if (FAILED(factory_->CreateDCRenderTarget(&props, &rt_))) return false;
    rt_->SetTextAntialiasMode(D2D1_TEXT_ANTIALIAS_MODE_GRAYSCALE);
    if (FAILED(rt_->CreateSolidColorBrush(D2D1_COLOR_F{1, 1, 1, 1}, &brush_))) return false;
    return true;
}

bool Gfx::begin() {
    if (!rt_) return false;
    RECT rc = {0, 0, pw_, ph_};
    if (FAILED(rt_->BindDC(memDc_, &rc))) return false;
    rt_->BeginDraw();
    D2D1_MATRIX_3X2_F m = {};
    m.m11 = scale_;
    m.m22 = scale_;
    rt_->SetTransform(&m);
    rt_->Clear(D2D1_COLOR_F{0, 0, 0, 0});
    return true;
}

bool Gfx::end() {
    const HRESULT hr = rt_->EndDraw();
    if (FAILED(hr)) trace("EndDraw failed", hr);
    if (hr == static_cast<HRESULT>(D2DERR_RECREATE_TARGET)) {
        const int w = pw_, h = ph_;
        pw_ = ph_ = 0;
        releaseTarget();
        resize(w, h);
        return false;
    }
    return SUCCEEDED(hr);
}

void Gfx::fillRect(float x, float y, float w, float h, Color c, float radius) {
    brush_->SetColor(d2d(c));
    if (radius > 0) rt_->FillRoundedRectangle(D2D1_ROUNDED_RECT{rect(x, y, w, h), radius, radius}, brush_);
    else rt_->FillRectangle(rect(x, y, w, h), brush_);
}

void Gfx::strokeRect(float x, float y, float w, float h, Color c, float t, float radius) {
    brush_->SetColor(d2d(c));
    const D2D1_RECT_F r = rect(x + t / 2, y + t / 2, w - t, h - t);
    if (radius > 0) rt_->DrawRoundedRectangle(D2D1_ROUNDED_RECT{r, radius, radius}, brush_, t);
    else rt_->DrawRectangle(r, brush_, t);
}

void Gfx::fillEllipse(float cx, float cy, float rx, float ry, Color c) {
    brush_->SetColor(d2d(c));
    rt_->FillEllipse(D2D1_ELLIPSE{D2D1_POINT_2F{cx, cy}, rx, ry}, brush_);
}

void Gfx::line(float x1, float y1, float x2, float y2, Color c, float t) {
    brush_->SetColor(d2d(c));
    rt_->DrawLine(D2D1_POINT_2F{x1, y1}, D2D1_POINT_2F{x2, y2}, brush_, t);
}

IDWriteTextFormat* Gfx::format(float size, int flags) {
    const int key = static_cast<int>(size * 10) * 64 + (flags & 0x1F);
    auto it = formats_.find(key);
    if (it != formats_.end()) return it->second;
    IDWriteTextFormat* f = nullptr;
    static const std::wstring family = [] {
        const wchar_t* v = _wgetenv(L"GF_FONT");  // optional override, mainly for testing
        return std::wstring(v && *v ? v : L"Segoe UI");
    }();
    const HRESULT hr = dwrite_->CreateTextFormat(family.c_str(), nullptr, (flags & TF_BOLD) ? DWRITE_FONT_WEIGHT_SEMI_BOLD : DWRITE_FONT_WEIGHT_NORMAL,
                                                 DWRITE_FONT_STYLE_NORMAL, DWRITE_FONT_STRETCH_NORMAL, size, L"en-us", &f);
    if (FAILED(hr) || !f) { trace("CreateTextFormat failed", hr); return nullptr; }
    const int h = flags & 3;
    f->SetTextAlignment(h == TF_CENTER ? DWRITE_TEXT_ALIGNMENT_CENTER : h == TF_RIGHT ? DWRITE_TEXT_ALIGNMENT_TRAILING : DWRITE_TEXT_ALIGNMENT_LEADING);
    f->SetParagraphAlignment((flags & TF_VCENTER) ? DWRITE_PARAGRAPH_ALIGNMENT_CENTER : DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    f->SetWordWrapping((flags & TF_WRAP) ? DWRITE_WORD_WRAPPING_WRAP : DWRITE_WORD_WRAPPING_NO_WRAP);
    formats_[key] = f;
    return f;
}

void Gfx::text(const std::string& s, float x, float y, float w, float h, float size, Color c, int flags) {
    if (s.empty()) return;
    IDWriteTextFormat* f = format(size, flags);
    if (!f) return;
    const std::wstring ws = widen(s);
    brush_->SetColor(d2d(c));
    IDWriteTextLayout* layout = nullptr;
    const HRESULT hr = dwrite_->CreateTextLayout(ws.c_str(), static_cast<UINT32>(ws.size()), f, std::max(1.f, w), std::max(1.f, h), &layout);
    if (FAILED(hr) || !layout) { trace("CreateTextLayout failed", hr); return; }
    rt_->DrawTextLayout(D2D1_POINT_2F{x, y}, layout, brush_, D2D1_DRAW_TEXT_OPTIONS_CLIP);
    layout->Release();
}

float Gfx::textWidth(const std::string& s, float size, bool bold) {
    IDWriteTextFormat* f = format(size, bold ? TF_BOLD : 0);
    if (!f || s.empty()) return 0;
    const std::wstring ws = widen(s);
    IDWriteTextLayout* layout = nullptr;
    dwrite_->CreateTextLayout(ws.c_str(), static_cast<UINT32>(ws.size()), f, 10000.f, 100.f, &layout);
    if (!layout) return 0;
    DWRITE_TEXT_METRICS m = {};
    layout->GetMetrics(&m);
    layout->Release();
    return m.widthIncludingTrailingWhitespace;
}

ID2D1Bitmap* Gfx::bitmapFor(SpriteKind kind, int id, int frame, bool flip) {
    const long long key = ((static_cast<long long>(kind) * 1000 + id) * 4 + frame) * 2 + (flip ? 1 : 0);
    auto it = bitmaps_.find(key);
    if (it != bitmaps_.end()) return it->second;
    gf::Sprite s;
    switch (kind) {
        case SpriteKind::Species: s = gf::makeSprite(id, frame); break;
        case SpriteKind::Silhouette: s = gf::makeSilhouette(gf::makeSprite(id, frame)); break;
        case SpriteKind::Egg: s = gf::makeEggSprite(frame); break;
    }
    if (flip)
        for (int y = 0; y < s.h; ++y) std::reverse(s.px.begin() + y * s.w, s.px.begin() + (y + 1) * s.w);
    ID2D1Bitmap* bmp = nullptr;
    D2D1_BITMAP_PROPERTIES bp = {};
    bp.pixelFormat.format = DXGI_FORMAT_B8G8R8A8_UNORM;
    bp.pixelFormat.alphaMode = D2D1_ALPHA_MODE_PREMULTIPLIED;
    bp.dpiX = bp.dpiY = 96.f;
    rt_->CreateBitmap(D2D1_SIZE_U{static_cast<UINT32>(s.w), static_cast<UINT32>(s.h)}, s.px.data(), static_cast<UINT32>(s.w * 4), &bp, &bmp);
    bitmaps_[key] = bmp;
    return bmp;
}

void Gfx::sprite(SpriteKind kind, int id, int frame, bool flip, float x, float y, float sc, float opacity) {
    ID2D1Bitmap* b = bitmapFor(kind, id, frame, flip);
    if (!b) return;
    rt_->DrawBitmap(b, rect(x, y, 16 * sc, 16 * sc), opacity, D2D1_BITMAP_INTERPOLATION_MODE_NEAREST_NEIGHBOR, nullptr);
}

void Gfx::pushClip(float x, float y, float w, float h) { rt_->PushAxisAlignedClip(rect(x, y, w, h), D2D1_ANTIALIAS_MODE_ALIASED); }
void Gfx::popClip() { rt_->PopAxisAlignedClip(); }

}  // namespace gfx
