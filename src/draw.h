#pragma once

// The picture: one view per player, following their robot through the same
// world, tiled 2x2 in one framebuffer (P1 top left, P2 top right, P3 bottom
// left, P4 bottom right). Each view is a 240x160 rectangle (a GBA's screen).
// The layout is one table (draw_view_rect): Multitap's multiple-viewports
// extension, when it exists, describes these rectangles to the frontend,
// which can then show each one on its own (a window, a phone...).

#include "art.h"
#include "game.h"

constexpr int32_t VIEW_WIDTH    = 240;
constexpr int32_t VIEW_HEIGHT   = 160;
constexpr int32_t SCREEN_WIDTH  = VIEW_WIDTH * 2;
constexpr int32_t SCREEN_HEIGHT = VIEW_HEIGHT * 2;

struct ViewRect
{
    int32_t x = 0;
    int32_t y = 0;
    int32_t width = 0;
    int32_t height = 0;
};

ViewRect draw_view_rect(int32_t player);

// Draws every view into pixels (SCREEN_WIDTH x SCREEN_HEIGHT, XRGB8888).
void draw_frame(const Game* game, const Art* art, uint32_t* pixels);
