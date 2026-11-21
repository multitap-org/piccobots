#pragma once

// piccolo's 5x5 capitals (ASCII only), for the score and the messages.

#include "canvas.h"

constexpr int32_t FONT_GLYPH_WIDTH  = 5;
constexpr int32_t FONT_GLYPH_HEIGHT = 5;
constexpr int32_t FONT_ADVANCE      = 6;   // glyph + 1 pixel gap

// (x, y) is the top left of the first letter; each font pixel is a
// scale x scale block.
void font_draw_text(const Canvas* canvas, int32_t x, int32_t y, const char* text, uint32_t color, int32_t scale);
int32_t font_text_width(const char* text, int32_t scale);
