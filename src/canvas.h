#pragma once

// Drawing into the core's framebuffer (XRGB8888), clipped to a rectangle:
// each player's view is a canvas over its quarter of the screen, so nothing
// drawn for one player spills into another's.

#include <cstddef>
#include <cstdint>

// A pixel with alpha 0 in a sprite is transparent; every opaque color has
// 0xFF in the top byte (the frontend ignores it: the format is XRGB).
constexpr uint32_t COLOR_OPAQUE = 0xFF000000u;

struct Canvas
{
    uint32_t* pixels       = nullptr;   // the whole framebuffer
    int32_t   pitch_pixels = 0;
    int32_t   clip_x       = 0;          // the drawable rectangle, framebuffer coordinates
    int32_t   clip_y       = 0;
    int32_t   clip_width   = 0;
    int32_t   clip_height  = 0;
};

void canvas_fill_rect(const Canvas* canvas, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color);

// Copies a width x height sprite to (x, y), skipping transparent pixels;
// mirrored = flipped left to right.
void canvas_blit(const Canvas* canvas, int32_t x, int32_t y, const uint32_t* sprite,
                 int32_t width, int32_t height, bool mirrored);

// The same, every opaque pixel replaced by `color` (a sprite's shadow or flash).
void canvas_blit_tinted(const Canvas* canvas, int32_t x, int32_t y, const uint32_t* sprite,
                        int32_t width, int32_t height, uint32_t color);
