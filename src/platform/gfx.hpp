// Direct2D/DirectWrite renderer for a per-pixel-alpha layered window.
// All coordinates are logical (DPI-independent); setScale() maps them to physical pixels.
#pragma once
#include <windows.h>

#include <d2d1.h>
#include <dwrite.h>

#include <map>
#include <string>

namespace gfx {

struct Color { float r, g, b, a; };
inline Color rgba(int r, int g, int b, int a = 255) { return {r / 255.f, g / 255.f, b / 255.f, a / 255.f}; }
inline Color withAlpha(Color c, float a) { c.a = a; return c; }

enum TextFlags { TF_LEFT = 0, TF_CENTER = 1, TF_RIGHT = 2, TF_VCENTER = 4, TF_BOLD = 8, TF_WRAP = 16 };
enum class SpriteKind { Species, Silhouette, Egg };

class Gfx {
public:
    Gfx() = default;
    ~Gfx();
    Gfx(const Gfx&) = delete;
    Gfx& operator=(const Gfx&) = delete;

    bool init();
    void setScale(float s) { scale_ = s; }
    float scale() const { return scale_; }
    bool resize(int pixelW, int pixelH);  // (re)creates the backing DIB
    bool begin();                         // binds the DIB and clears it to transparent
    bool end();                           // EndDraw; the DIB is ready for UpdateLayeredWindow
    HDC dc() const { return memDc_; }

    void fillRect(float x, float y, float w, float h, Color c, float radius = 0);
    void strokeRect(float x, float y, float w, float h, Color c, float thickness = 1, float radius = 0);
    void fillEllipse(float cx, float cy, float rx, float ry, Color c);
    void line(float x1, float y1, float x2, float y2, Color c, float thickness = 1);
    void text(const std::string& s, float x, float y, float w, float h, float size, Color c, int flags = 0);
    float textWidth(const std::string& s, float size, bool bold = false);
    // Draws a sprite scaled to `size` x `size` logical pixels (use multiples of 24 for crisp pixels).
    void sprite(SpriteKind kind, int id, int frame, bool flip, float x, float y, float size, float opacity = 1.f);
    float textHeight(const std::string& s, float w, float size, bool bold = false);
    void pushClip(float x, float y, float w, float h);
    void popClip();

    // Cached drawing: render something expensive once into a bitmap, then blit it every frame.
    bool hasCache(int id, long long key) const;
    bool beginCache(int id, long long key, float w, float h);  // subsequent draw calls go into the bitmap; false on failure
    void endCache();
    bool drawCache(int id, float x, float y);                  // blits the bitmap at logical (x, y)

private:
    IDWriteTextFormat* format(float size, int flags);
    ID2D1Bitmap* bitmapFor(SpriteKind kind, int id, int frame, bool flip);
    void releaseTarget();

    ID2D1Factory* factory_ = nullptr;
    IDWriteFactory* dwrite_ = nullptr;
    ID2D1DCRenderTarget* rt_ = nullptr;
    ID2D1RenderTarget* r_ = nullptr;             // where draw calls go: rt_, or a cache bitmap target while caching
    ID2D1BitmapRenderTarget* cacheRt_ = nullptr;
    int cacheId_ = -1;
    long long cacheKey_ = 0;
    float cacheW_ = 0, cacheH_ = 0;
    struct CacheEntry { ID2D1Bitmap* bmp = nullptr; long long key = 0; float w = 0, h = 0; };
    std::map<int, CacheEntry> caches_;
    std::map<std::string, IDWriteTextLayout*> layouts_;
    std::map<std::string, float> widths_;
    ID2D1SolidColorBrush* brush_ = nullptr;
    HDC memDc_ = nullptr;
    HBITMAP dib_ = nullptr;
    HBITMAP oldBmp_ = nullptr;
    void* bits_ = nullptr;
    int pw_ = 0, ph_ = 0;
    float scale_ = 1.f;
    std::map<int, IDWriteTextFormat*> formats_;
    std::map<long long, ID2D1Bitmap*> bitmaps_;
};

std::wstring widen(const std::string& s);

}  // namespace gfx
