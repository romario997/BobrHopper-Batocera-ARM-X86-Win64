// Loaders for the files produced by tools/bake_*.py (formats documented there).
#pragma once

#include <cstdint>
#include <map>
#include <string>
#include <vector>

namespace cr {

struct MeshData {
    // interleaved: px py pz nx ny nz u t   (8 floats per vertex)
    std::vector<float> vertices;
    std::vector<uint16_t> indices;
    float aabbMin[3] = {0, 0, 0};
    float aabbMax[3] = {0, 0, 0};
    int vertexCount() const { return int(vertices.size() / 8); }
};

// SF2000 software renderer mesh (tools/bake_flat.py): one sRGB colour and one axis normal per triangle
struct FlatMeshData {
    struct Triangle {
        uint16_t a, b, c;
        uint8_t color; // index into colors
        uint8_t axis;  // model-space normal: 0 +X, 1 -X, 2 +Y, 3 -Y, 4 +Z, 5 -Z
    };
    std::vector<int32_t> positions; // x y z per vertex, 16.16 fixed point
    std::vector<Triangle> triangles;
    std::vector<uint8_t> colors;    // r g b per colour
    float aabbMin[3] = {0, 0, 0};   // the same bits as the .mesh of the model (the game logic uses them)
    float aabbMax[3] = {0, 0, 0};
    int vertexCount() const { return int(positions.size() / 3); }
    int colorCount() const { return int(colors.size() / 3); }
};

struct TextureData {
    int width = 0, height = 0, channels = 0;
    std::vector<uint8_t> pixels; // rows top-first
};

struct SoundData {
    int sampleRate = 0;
    std::vector<int16_t> samples; // mono
};

struct Glyph {
    uint16_t x = 0, y = 0, w = 0, h = 0;
    int16_t xoff = 0, yoff = 0;
    uint16_t advance = 0;
};

// O11.6: the baked font (tools/bake_font.py) has ASCII and the Polish letters. A Unicode code point's slot in
// FontData::glyphs: ASCII as itself, the Polish letters after it (in kPolishLetters' order), -1 for anything else.
static const uint16_t kPolishLetters[] = {0x104, 0x106, 0x118, 0x141, 0x143, 0xd3,  0x15a, 0x179, 0x17b,   // ĄĆĘŁŃÓŚŹŻ
                                          0x105, 0x107, 0x119, 0x142, 0x144, 0xf3, 0x15b, 0x17a, 0x17c,   // ąćęłńóśźż
                                          // Spanish (the third language): ÁÉÍÚÑ áéíúñ ¡¿ - after the Polish ones, so
                                          // every slot the Polish letters had stays where it was
                                          0xc1, 0xc9, 0xcd, 0xda, 0xd1, 0xe1, 0xe9, 0xed, 0xfa, 0xf1, 0xa1, 0xbf,
                                          // Czech and Slovak: ČĎĚŇŘŠŤŮŽÝ ÄĹĽÔŔ, then the same in lower case
                                          0x10c, 0x10e, 0x11a, 0x147, 0x158, 0x160, 0x164, 0x16e, 0x17d, 0xdd,
                                          0xc4, 0x139, 0x13d, 0xd4, 0x154,
                                          0x10d, 0x10f, 0x11b, 0x148, 0x159, 0x161, 0x165, 0x16f, 0x17e, 0xfd,
                                          0xe4, 0x13a, 0x13e, 0xf4, 0x155,
                                          // Hungarian ÖÜŐŰ öüőű, Romanian ĂÂÎȘȚ ăâîșț (+ the cedilla forms ŞŢşţ)
                                          0xd6, 0xdc, 0x150, 0x170, 0xf6, 0xfc, 0x151, 0x171,
                                          0x102, 0xc2, 0xce, 0x218, 0x21a, 0x103, 0xe2, 0xee, 0x219, 0x21b,
                                          0x15e, 0x162, 0x15f, 0x163,
                                          // Esperanto ĈĜĤĴŜŬ ĉĝĥĵŝŭ (Volapuk needs only ÄÖÜ, above)
                                          0x108, 0x11c, 0x124, 0x134, 0x15c, 0x16c, 0x109, 0x11d, 0x125, 0x135, 0x15d, 0x16d};
static const int kGlyphSlots = 128 + int(sizeof(kPolishLetters) / sizeof(kPolishLetters[0]));

inline int glyphSlot(uint32_t codepoint)
{
    if (codepoint < 128) return int(codepoint);
    for (int i = 0; i < kGlyphSlots - 128; i++)
        if (kPolishLetters[i] == codepoint) return 128 + i;
    return -1;
}

struct FontData {
    int atlasW = 0, atlasH = 0, pixelSize = 0, lineHeight = 0;
    Glyph glyphs[kGlyphSlots];
    std::vector<uint8_t> coverage; // atlasW * atlasH, rows top-first
};

struct Manifest {
    struct Model {
        std::string mesh, texture;
    };
    std::map<std::string, Model> models; // model name -> files
    std::vector<std::string> sounds;
    std::vector<std::string> music; // music/<name>.ogg, in manifest order
};

// Directory of the running executable, with trailing separator; data/ and conf/ live next to it.
const std::string &baseDir();
// Overrides it before the first baseDir()/dataDir() call (SF2000: the ROM's folder on the SD card).
void setBaseDir(const std::string &dir);

// data/ next to the executable (device package), or the repo's data/ when running from out/pc/.
const std::string &dataDir();

bool loadManifest(const std::string &path, Manifest &out);

bool readFile(const std::string &path, std::vector<uint8_t> &out);
bool loadMesh(const std::string &path, MeshData &out);
bool loadFlatMesh(const std::string &path, FlatMeshData &out);
bool loadTexture(const std::string &path, TextureData &out);
bool loadSound(const std::string &path, SoundData &out);
bool loadFont(const std::string &path, FontData &out);

} // namespace cr
