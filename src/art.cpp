#include "art.h"

#include <cstdio>
#include <cstring>

// ---------------------------------------------------------------------------
// The pictures. '.' is transparent; every other character is a color from
// the picture's palette below it. Drawn for piccobots.
// ---------------------------------------------------------------------------

static const char* const GRASS[TILE_SIZE] = {
    "gggggggggggggggg",
    "gggggggggggGgggg",
    "ggGgggggggGlGggg",
    "gGlGgggggggggggg",
    "gggggggggggggggg",
    "gggggggggggggggg",
    "ggggggggGggggggg",
    "gggggggGlGgggggg",
    "gggggggggggggggg",
    "gggggggggggggGgg",
    "ggGgggggggggGlGg",
    "gGlGgggggggggggg",
    "gggggggggggggggg",
    "ggggggggggGggggg",
    "gggggggggGlGgggg",
    "gggggggggggggggg",
};

static const char* const FLOWERS[TILE_SIZE] = {
    "gggggggggggggggg",
    "gggrgggggggggggg",
    "ggrwrgggggggyggg",
    "gggrgggggggywygg",
    "gggGggggggggyggg",
    "ggggggggggggGggg",
    "gggggggggggggggg",
    "ggggggggwggggggg",
    "gggggggwywgggggg",
    "ggggggggwggggggg",
    "ggggggggGggggggg",
    "ggyggggggggggrgg",
    "gywygggggggggrwr",
    "ggyggggggggggrgg",
    "ggGgggggggggggGg",
    "gggggggggggggggg",
};

static const char* const PATH[TILE_SIZE] = {
    "ssssssssssssssss",
    "ssssssssSsssssss",
    "sSssssssssssssss",
    "sssssssssssssLss",
    "ssssssssssssssss",
    "ssssSsssssssssss",
    "ssssssssssssSsss",
    "ssssssssssssssss",
    "sssLssssssssssss",
    "ssssssssssSsssss",
    "ssssssssssssssss",
    "sSssssssssssssss",
    "ssssssssLsssssss",
    "ssssssssssssssSs",
    "ssssssSsssssssss",
    "ssssssssssssssss",
};

static const char* const TREE[TILE_SIZE] = {
    "gggggKKKKKKggggg",
    "gggKKtthhttKKggg",
    "ggKtthhhhtttTKgg",
    "gKtthhhtttttTTKg",
    "gKthhttttttttTKg",
    "KtthttttttttTTTK",
    "KttttttttttTTTTK",
    "KtttttttttTTTTTK",
    "gKtttttttTTTTTKg",
    "gKTtttTTTTTTTTKg",
    "ggKTTTTTTTTTTKgg",
    "gggKKTTTTTTKKggg",
    "gggggKKbbKKggggg",
    "GggggggbbgggggGg",
    "gggggggbbggggggg",
    "ggggggGbbGgggggg",
};

static const char* const WATER[TILE_SIZE] = {
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwffwwwwwwwwwwww",
    "wfwwfwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwWWwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwffwwww",
    "wwwwwwwwwfwwfwww",
    "wwwwwwwwwwwwwwww",
    "wwWWwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwwwwwwwwwwww",
    "wwwwwffwwwwwwwww",
    "wwwwfwwfwwwwwwww",
};

// Robots: k outline, b body, d body shade, l body light (the player's
// colors), v visor, e eyes.
static const char* const ROBOT_DOWN[ROBOT_SPRITE] = {
    "................",
    ".......kk.......",
    "......kllk......",
    "....kkkkkkkk....",
    "...kbbbbbbbbk...",
    "..kbllbbbbbbdk..",
    "..kbvvvvvvvvdk..",
    "..kbveevveevdk..",
    "..kbvvvvvvvvdk..",
    "..kbbbbbbbbbdk..",
    "...kddddddddk...",
    "....kkkkkkkk....",
    "...kbk....kbk...",
    "...kdk....kdk...",
    "...kkk....kkk...",
    "................",
};

static const char* const ROBOT_UP[ROBOT_SPRITE] = {
    "................",
    ".......kk.......",
    "......kllk......",
    "....kkkkkkkk....",
    "...kbbbbbbbbk...",
    "..kbllbbbbbbdk..",
    "..kbbbbbbbbbdk..",
    "..kbbkddddkbdk..",
    "..kbbbbbbbbbdk..",
    "..kbbbbbbbbbdk..",
    "...kddddddddk...",
    "....kkkkkkkk....",
    "...kbk....kbk...",
    "...kdk....kdk...",
    "...kkk....kkk...",
    "................",
};

static const char* const ROBOT_SIDE[ROBOT_SPRITE] = {
    "................",
    "......kk........",
    ".....kllk.......",
    "....kkkkkkkk....",
    "...kbbbbbbbbk...",
    "..kbllbbbbbbbk..",
    "..kbbbbbvvvvvk..",
    "..kbbbbbvevvek..",
    "..kbbbbbvvvvvk..",
    "..kbbbbbbbbbdk..",
    "...kddddddddk...",
    "....kkkkkkkk....",
    "....kbk..kbk....",
    "....kdk..kdk....",
    "....kkk..kkk....",
    "................",
};

static const char* const GEM[GEM_SPRITE] = {
    "...kk...",
    "..kwck..",
    ".kwccck.",
    "kwccccCk",
    "kcccccCk",
    ".kcccCk.",
    "..kcCk..",
    "...kk...",
};

// l: the shooter's light color.
static const char* const SHOT[SHOT_SPRITE] = {
    "..ll..",
    ".lwwl.",
    "lwwwwl",
    "lwwwwl",
    ".lwwl.",
    "..ll..",
};

static const char* const HEART[HEART_HEIGHT] = {
    ".rr.rr.",
    "rhrrrrr",
    "rrrrrrr",
    ".rrrrr.",
    "..rrr..",
    "...r...",
};

// ---------------------------------------------------------------------------
// Palettes
// ---------------------------------------------------------------------------

struct PaletteEntry
{
    char     character;
    uint32_t color;   // 0xRRGGBB
};

constexpr int32_t MAX_PALETTE = 12;

struct Palette
{
    PaletteEntry entries[MAX_PALETTE];
    int32_t      count;
};

static const Palette GROUND_PALETTE = {
    {
        { 'g', 0x62B04A }, { 'G', 0x4C9638 }, { 'l', 0x7CC85C },   // grass
        { 's', 0xE4C78E }, { 'S', 0xCDAE72 }, { 'L', 0xF2DDAE },   // sand
        { 't', 0x2F8A3A }, { 'T', 0x1F6B2C }, { 'h', 0x4CAE4F },   // tree
        { 'K', 0x15421D }, { 'b', 0x6B4A2E },
        { 'r', 0xF0648C },
    },
    12,
};

static const Palette DETAIL_PALETTE = {
    {
        { 'g', 0x62B04A }, { 'G', 0x4C9638 },
        { 'r', 0xF0648C }, { 'y', 0xFFE36E }, { 'w', 0xF8F8F8 },   // flowers
    },
    5,
};

static const Palette WATER_PALETTE = {
    { { 'w', 0x3C8ED8 }, { 'W', 0x2B6EB4 }, { 'f', 0xA6D6F6 } },
    3,
};

static const Palette GEM_PALETTE = {
    { { 'k', 0x183848 }, { 'c', 0x4DD8E8 }, { 'C', 0x1A9CB0 }, { 'w', 0xF4FFFF } },
    4,
};

static const Palette HEART_FULL_PALETTE = {
    { { 'r', 0xE8344C }, { 'h', 0xFF9AA8 } },
    2,
};

static const Palette HEART_EMPTY_PALETTE = {
    { { 'r', 0x3A3E52 }, { 'h', 0x4A4E62 } },
    2,
};

// Each player's robot colors: body, shade, light (P1 red, P2 blue, P3 yellow, P4 purple).
static const uint32_t PLAYER_COLORS[MAX_PLAYERS][3] = {
    { 0xE05050, 0x982C2C, 0xFF9A8A },
    { 0x4A7CE0, 0x2A4A9A, 0x9AC0FF },
    { 0xE8B830, 0xA07818, 0xFFE38A },
    { 0xA060E0, 0x683A9A, 0xD4AAFF },
};

static bool art_convert(const char* name, const char* const* rows, int32_t width, int32_t height,
                        const Palette* palette, uint32_t* out)
{
    for (int32_t y = 0; y < height; y++)
    {
        if (static_cast<int32_t>(std::strlen(rows[y])) != width)
        {
            std::fprintf(stderr, "piccobots: picture %s, row %d isn't %d wide\n", name, y, width);
            return false;
        }
        for (int32_t x = 0; x < width; x++)
        {
            char c = rows[y][x];
            uint32_t pixel = 0;   // transparent
            if (c != '.')
            {
                bool found = false;
                for (int32_t i = 0; i < palette->count; i++)
                {
                    if (palette->entries[i].character == c)
                    {
                        pixel = COLOR_OPAQUE | palette->entries[i].color;
                        found = true;
                        break;
                    }
                }
                if (!found)
                {
                    std::fprintf(stderr, "piccobots: picture %s, '%c' isn't in its palette\n", name, c);
                    return false;
                }
            }
            out[y * width + x] = pixel;
        }
    }
    return true;
}

bool art_init(Art* art)
{
    bool ok = true;
    ok = ok && art_convert("grass", GRASS, TILE_SIZE, TILE_SIZE, &GROUND_PALETTE,
                           art->tiles[static_cast<int32_t>(Tile::Grass)]);
    ok = ok && art_convert("path", PATH, TILE_SIZE, TILE_SIZE, &GROUND_PALETTE,
                           art->tiles[static_cast<int32_t>(Tile::Path)]);
    ok = ok && art_convert("flowers", FLOWERS, TILE_SIZE, TILE_SIZE, &DETAIL_PALETTE,
                           art->tiles[static_cast<int32_t>(Tile::Flowers)]);
    ok = ok && art_convert("tree", TREE, TILE_SIZE, TILE_SIZE, &GROUND_PALETTE,
                           art->tiles[static_cast<int32_t>(Tile::Tree)]);
    ok = ok && art_convert("water", WATER, TILE_SIZE, TILE_SIZE, &WATER_PALETTE,
                           art->tiles[static_cast<int32_t>(Tile::Water)]);
    ok = ok && art_convert("gem", GEM, GEM_SPRITE, GEM_SPRITE, &GEM_PALETTE, art->gem);
    ok = ok && art_convert("heart", HEART, HEART_WIDTH, HEART_HEIGHT, &HEART_FULL_PALETTE, art->heart_full);
    ok = ok && art_convert("heart", HEART, HEART_WIDTH, HEART_HEIGHT, &HEART_EMPTY_PALETTE, art->heart_empty);
    if (!ok)
    {
        return false;
    }

    // The waves drift: the water tile shifted 0..15 pixels to the left.
    const uint32_t* water = art->tiles[static_cast<int32_t>(Tile::Water)];
    for (int32_t shift = 0; shift < WATER_FRAMES; shift++)
    {
        for (int32_t y = 0; y < TILE_SIZE; y++)
        {
            for (int32_t x = 0; x < TILE_SIZE; x++)
            {
                art->water[shift][y * TILE_SIZE + x] = water[y * TILE_SIZE + (x + shift) % TILE_SIZE];
            }
        }
    }

    const char* const* poses[ROBOT_POSES] = { ROBOT_DOWN, ROBOT_UP, ROBOT_SIDE };
    for (int32_t p = 0; p < MAX_PLAYERS; p++)
    {
        art->player_color[p] = COLOR_OPAQUE | PLAYER_COLORS[p][0];
        art->player_dark[p] = COLOR_OPAQUE | PLAYER_COLORS[p][1];
        art->player_light[p] = COLOR_OPAQUE | PLAYER_COLORS[p][2];
        const Palette robot_palette = {
            {
                { 'k', 0x1A1A24 }, { 'v', 0x1E2433 }, { 'e', 0x7FF6FF },
                { 'b', PLAYER_COLORS[p][0] }, { 'd', PLAYER_COLORS[p][1] }, { 'l', PLAYER_COLORS[p][2] },
            },
            6,
        };
        for (int32_t pose = 0; pose < ROBOT_POSES; pose++)
        {
            ok = ok && art_convert("robot", poses[pose], ROBOT_SPRITE, ROBOT_SPRITE, &robot_palette,
                                   art->robots[p][pose]);
        }
        const Palette shot_palette = {
            { { 'l', PLAYER_COLORS[p][2] }, { 'w', 0xFFFFFF } },
            2,
        };
        ok = ok && art_convert("shot", SHOT, SHOT_SPRITE, SHOT_SPRITE, &shot_palette, art->shots[p]);
    }
    return ok;
}
