#include "engine/text.h"

#ifndef CR_FIXED
#include <cmath>
#include <cstdio>
#include <vector>
#endif

#include "engine/log.h"
#include "engine/strings.h"

namespace cr {

static const int kSizes[] = {12, 14, 16, 18, 32, 48};

// the glyph slot of the UTF-8 character at text[i] (i moves past it); -1 when the font has no glyph for it. A stray
// continuation byte counts as a character of its own.
static int nextGlyph(const std::string &text, size_t &i)
{
    const unsigned char lead = static_cast<unsigned char>(text[i++]);
    if (lead < 0x80) return lead;
    int more = lead >= 0xf0 ? 3 : lead >= 0xe0 ? 2 : lead >= 0xc0 ? 1 : 0;
    uint32_t cp = lead & (0x3f >> more);
    for (; more > 0 && i < text.size() && (static_cast<unsigned char>(text[i]) & 0xc0) == 0x80; more--)
        cp = (cp << 6) | (static_cast<unsigned char>(text[i++]) & 0x3f);
    return more ? -1 : glyphSlot(cp);
}

#ifndef CR_FIXED
// the baked face (data/fonts/retro_<n>.fnt) whose size is nearest to `want` real pixels, 0 when none is within 12%.
// Which sizes exist is looked up once per data directory.
static int nearestBakedSize(const std::string &dataDir, float want)
{
    static std::string scannedDir;
    static std::vector<int> baked;
    if (scannedDir != dataDir) {
        scannedDir = dataDir;
        baked.clear();
        for (int n = 4; n <= 256; n++) {
            FILE *f = std::fopen((dataDir + "fonts/retro_" + toString(n) + ".fnt").c_str(), "rb");
            if (!f) continue;
            std::fclose(f);
            baked.push_back(n);
        }
    }
    int best = 0;
    float bestErr = 0.12f;
    for (int n : baked) {
        const float err = std::fabs(float(n) - want) / want;
        // a tie goes to the smaller face: a label a pixel too narrow never runs into its neighbour
        if (err < bestErr - 1e-4f) {
            best = n;
            bestErr = err;
        }
    }
    return best;
}

void TextRenderer::release(Renderer &renderer)
{
    for (auto &kv : fonts_) renderer.releaseTexture(kv.second.texture);
    fonts_.clear();
}
#endif

bool TextRenderer::load(Renderer &renderer, const std::string &dataDir)
{
    for (int size : kSizes) {
        Loaded font;
#ifndef CR_FIXED
        // a big screen: the same face baked for its real pixels (tools/bake_font.py --sizes, build/bake_all.sh)
        if (pixelScale > 1) {
            const int hi = int(std::floor(float(size) * pixelScale + 0.5f));
            if (hi != size && loadFont(dataDir + "fonts/retro_" + toString(hi) + ".fnt", font.data)) {
                font.scale = pixelScale;
                font.texture = renderer.uploadAlphaTexture(font.data.atlasW, font.data.atlasH, font.data.coverage.data());
                fonts_[size] = std::move(font);
                continue;
            }
            if (nearestFace) {
                const int near = nearestBakedSize(dataDir, float(size) * pixelScale);
                if (near > size && loadFont(dataDir + "fonts/retro_" + toString(near) + ".fnt", font.data)) {
                    font.scale = pixelScale; // drawn 1:1 on the real pixels, its metrics / pixelScale
                    font.texture = renderer.uploadAlphaTexture(font.data.atlasW, font.data.atlasH, font.data.coverage.data());
                    fonts_[size] = std::move(font);
                    continue;
                }
            }
            logf("text: no retro_%d.fnt for size %d at scale %.2f, the 1x font is magnified", hi, size, pixelScale);
        }
#endif
        std::string path = dataDir + "fonts/retro_" + toString(size / glyphScale) + ".fnt";
        if (!loadFont(path, font.data)) {
            logf("text: cannot load %s", path.c_str());
            return false;
        }
        font.texture = renderer.uploadAlphaTexture(font.data.atlasW, font.data.atlasH, font.data.coverage.data());
        fonts_[size] = std::move(font);
    }
    return true;
}

int TextRenderer::width(const std::string &text, int size) const
{
    auto it = fonts_.find(size);
    if (it == fonts_.end()) return 0;
    int w = 0;
    for (size_t i = 0; i < text.size();) {
        const int slot = nextGlyph(text, i);
        if (slot >= 0) w += it->second.data.glyphs[slot].advance * glyphScale;
    }
#ifndef CR_FIXED
    if (it->second.scale != 1) return int(std::floor(float(w) / it->second.scale + 0.5f));
#endif
    return w;
}

int TextRenderer::lineHeight(int size) const
{
    auto it = fonts_.find(size);
    if (it == fonts_.end()) return 0;
#ifndef CR_FIXED
    if (it->second.scale != 1) return int(std::floor(float(it->second.data.lineHeight) / it->second.scale + 0.5f));
#endif
    return it->second.data.lineHeight * glyphScale;
}

void TextRenderer::draw(Renderer &renderer, const std::string &text, int x, int y, int size, Rgba color)
{
    auto it = fonts_.find(size);
    if (it == fonts_.end()) return;
    const FontData &f = it->second.data;
    verts_.clear();
#ifndef CR_FIXED
    if (it->second.scale != 1) {
        // hi-res face: the pen walks the REAL pixels (glyph metrics are in them), snapped once at the start, and every
        // corner goes back to logical pixels for the overlay - so each font pixel lands on whole screen pixels
        const float s = it->second.scale;
        int pen = int(std::floor(float(x) * s + 0.5f));
        const int top = int(std::floor(float(y) * s + 0.5f));
        for (size_t i = 0; i < text.size();) {
            const int slot = nextGlyph(text, i);
            if (slot < 0) continue;
            const Glyph &g = f.glyphs[slot];
            if (g.w && g.h) {
                const float x0 = float(pen + g.xoff) / s, y0 = float(top + g.yoff) / s;
                const float x1 = float(pen + g.xoff + g.w) / s, y1 = float(top + g.yoff + g.h) / s;
                const float u0 = float(g.x) / float(f.atlasW), v0 = float(g.y) / float(f.atlasH);
                const float u1 = float(g.x + g.w) / float(f.atlasW), v1 = float(g.y + g.h) / float(f.atlasH);
                const float quad[24] = {x0, y0, u0, v0, x1, y0, u1, v0, x1, y1, u1, v1,
                                        x0, y0, u0, v0, x1, y1, u1, v1, x0, y1, u0, v1};
                verts_.insert(verts_.end(), quad, quad + 24);
            }
            pen += g.advance;
        }
        renderer.drawOverlayTriangles(it->second.texture, verts_, color.r, color.g, color.b, color.a);
        return;
    }
#endif
    int pen = x;
    for (size_t i = 0; i < text.size();) {
        const int slot = nextGlyph(text, i);
        if (slot < 0) continue;
        const Glyph &g = f.glyphs[slot];
        if (g.w && g.h) {
            const mreal x0 = mreal(pen + g.xoff * glyphScale), y0 = mreal(y + g.yoff * glyphScale);
            const mreal x1 = x0 + mreal(g.w * glyphScale), y1 = y0 + mreal(g.h * glyphScale);
            const mreal u0 = mreal(int(g.x)) / mreal(f.atlasW), v0 = mreal(int(g.y)) / mreal(f.atlasH);
            const mreal u1 = mreal(g.x + g.w) / mreal(f.atlasW), v1 = mreal(g.y + g.h) / mreal(f.atlasH);
            const mreal quad[24] = {x0, y0, u0, v0, x1, y0, u1, v0, x1, y1, u1, v1,
                                    x0, y0, u0, v0, x1, y1, u1, v1, x0, y1, u0, v1};
            verts_.insert(verts_.end(), quad, quad + 24);
        }
        pen += g.advance * glyphScale;
    }
    renderer.drawOverlayTriangles(it->second.texture, verts_, color.r, color.g, color.b, color.a);
}

void TextRenderer::drawOutlined(Renderer &renderer, const std::string &text, int x, int y, int size, Rgba color,
                                int outlineWidth, Rgba outline)
{
    static const int offsets[4][2] = {{-1, 0}, {1, 0}, {0, -1}, {0, 1}};
#ifdef CR_FIXED
    // opaque text only: translucent passes blend with what is under them, a capture cannot hold that
    if (color.a == mreal(1) && outline.a == mreal(1)) {
        const auto same = [](const Rgba &p, const Rgba &q) { return p.r == q.r && p.g == q.g && p.b == q.b && p.a == q.a; };
        const Renderer::OverlayState state = renderer.overlayState();
        for (CachedText &c : cache_) {
            if (c.x == x && c.y == y && c.size == size && c.outlineWidth == outlineWidth && same(c.color, color) &&
                same(c.outline, outline) && c.state == state && c.text == text) {
                c.lastUse = ++useCount_;
                renderer.drawOverlayCapture(c.capture);
                return;
            }
        }
        Renderer::OverlayCapture capture;
        renderer.beginOverlayCapture();
        for (const auto &o : offsets) draw(renderer, text, x + o[0] * outlineWidth, y + o[1] * outlineWidth, size, outline);
        draw(renderer, text, x, y, size, color);
        renderer.endOverlayCapture(capture);
        if (capture.valid) {
            renderer.drawOverlayCapture(capture);
            CachedText *slot = nullptr;
            if (cache_.size() < 8) {
                cache_.emplace_back();
                slot = &cache_.back();
            } else {
                slot = &cache_[0];
                for (CachedText &c : cache_)
                    if (c.lastUse < slot->lastUse) slot = &c;
            }
            slot->text = text;
            slot->x = x;
            slot->y = y;
            slot->size = size;
            slot->outlineWidth = outlineWidth;
            slot->color = color;
            slot->outline = outline;
            slot->state = state;
            slot->capture = std::move(capture);
            slot->lastUse = ++useCount_;
            return;
        }
        // not exact after all (a translucent glyph pixel): drawn again straight onto the target below
    }
#endif
    for (const auto &o : offsets) draw(renderer, text, x + o[0] * outlineWidth, y + o[1] * outlineWidth, size, outline);
    draw(renderer, text, x, y, size, color);
}

} // namespace cr
