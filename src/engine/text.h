// Bitmap text from the baked retro font (tools/bake_font.py): pixel-exact glyph quads on the renderer's 2D
// overlay, and outlines the way the original draws them (CSS text-shadow: four copies offset by +-w in black
// under the text). Call between Renderer::beginOverlay and endOverlay.
#pragma once

#include <map>
#include <string>
#include <vector>

#include "engine/assets.h"
#include "engine/renderer.h"

namespace cr {

// mreal: float on PC/R36S, 16.16 on the SF2000
struct Rgba {
    mreal r, g, b, a;
};

class TextRenderer {
public:
    // data/fonts/retro_<size>.fnt for every baked size (14, 16, 32, 48)
    bool load(Renderer &renderer, const std::string &dataDir);

    // SF2000: 2 - the screens keep their 640x480 layout in logical pixels, the renderer maps them onto 320x240, and every
    // size N comes from a font baked at N / 2 whose metrics count double (crisp glyphs instead of scaled ones).
    // Set before load(); 1 everywhere else.
    int glyphScale = 1;
#ifndef CR_FIXED
    // Screens bigger than the 640x480 layout (Batocera PC at 1920x1080: 2.25): the overlay maps the logical pixels onto
    // the real ones, and each size N is drawn from a font baked at round(N * pixelScale) when data/fonts has it (crisp
    // at the real resolution; its metrics count 1 / pixelScale). Without such a file that size keeps the 1x font,
    // magnified. Set before load(); 1 = exactly the old path (R36S, windowed PC at 640x480).
    float pixelScale = 1;
    // Windows desktop (a window of any size): when no face is baked at exactly round(N * pixelScale), take the baked
    // size nearest to it and draw it 1:1 on the real pixels - a few percent bigger or smaller than the layout's N, but
    // crisp - instead of magnifying the 1x font by an odd factor. Off everywhere else (the consoles keep their path).
    bool nearestFace = false;
    // frees every loaded face; load() again after changing pixelScale (a resized window)
    void release(Renderer &renderer);
#endif

    bool hasSize(int size) const { return fonts_.count(size) != 0; }
    int width(const std::string &text, int size) const;
    int lineHeight(int size) const;

    // (x, y) = top-left of the line in pixels from the top-left of the screen
    void draw(Renderer &renderer, const std::string &text, int x, int y, int size, Rgba color);
    void drawOutlined(Renderer &renderer, const std::string &text, int x, int y, int size, Rgba color, int outlineWidth,
                      Rgba outline);

private:
    struct Loaded {
        FontData data;
        GpuTexture texture;
#ifndef CR_FIXED
        float scale = 1; // font pixels per logical pixel: pixelScale for a hi-res face, 1 otherwise
#endif
    };
    std::map<int, Loaded> fonts_;
    std::vector<mreal> verts_;
#ifdef CR_FIXED
    // O7.7 (the SF2000): the last opaque outlined texts, drawn once into a capture and copied from then on - five glyph
    // passes with per-pixel blending and u/v divisions were most of the HUD's ~2 ms a frame on the console
    struct CachedText {
        std::string text;
        int x = 0, y = 0, size = 0, outlineWidth = 0;
        Rgba color{}, outline{};
        Renderer::OverlayState state;
        Renderer::OverlayCapture capture;
        uint32_t lastUse = 0;
    };
    std::vector<CachedText> cache_;
    uint32_t useCount_ = 0;
#endif
};

} // namespace cr
