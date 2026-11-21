#include "canvas.h"

// The part of [x, x + width) inside [clip, clip + clip_width): first and end.
static bool canvas_clip_span(int32_t x, int32_t width, int32_t clip, int32_t clip_width,
                             int32_t* first, int32_t* end)
{
    *first = x < clip ? clip : x;
    int32_t span_end = x + width;
    int32_t clip_end = clip + clip_width;
    *end = span_end > clip_end ? clip_end : span_end;
    return *first < *end;
}

void canvas_fill_rect(const Canvas* canvas, int32_t x, int32_t y, int32_t width, int32_t height, uint32_t color)
{
    int32_t x0 = 0;
    int32_t x1 = 0;
    int32_t y0 = 0;
    int32_t y1 = 0;
    if (!canvas_clip_span(x, width, canvas->clip_x, canvas->clip_width, &x0, &x1) ||
        !canvas_clip_span(y, height, canvas->clip_y, canvas->clip_height, &y0, &y1))
    {
        return;
    }
    for (int32_t py = y0; py < y1; py++)
    {
        uint32_t* row = canvas->pixels + static_cast<ptrdiff_t>(py) * canvas->pitch_pixels;
        for (int32_t px = x0; px < x1; px++)
        {
            row[px] = color;
        }
    }
}

static void canvas_blit_any(const Canvas* canvas, int32_t x, int32_t y, const uint32_t* sprite,
                            int32_t width, int32_t height, bool mirrored, bool tinted, uint32_t tint)
{
    int32_t x0 = 0;
    int32_t x1 = 0;
    int32_t y0 = 0;
    int32_t y1 = 0;
    if (!canvas_clip_span(x, width, canvas->clip_x, canvas->clip_width, &x0, &x1) ||
        !canvas_clip_span(y, height, canvas->clip_y, canvas->clip_height, &y0, &y1))
    {
        return;
    }
    for (int32_t py = y0; py < y1; py++)
    {
        uint32_t* row = canvas->pixels + static_cast<ptrdiff_t>(py) * canvas->pitch_pixels;
        const uint32_t* source = sprite + static_cast<ptrdiff_t>(py - y) * width;
        for (int32_t px = x0; px < x1; px++)
        {
            int32_t sx = px - x;
            if (mirrored)
            {
                sx = width - 1 - sx;
            }
            uint32_t pixel = source[sx];
            if ((pixel & COLOR_OPAQUE) == 0)
            {
                continue;
            }
            row[px] = tinted ? tint : pixel;
        }
    }
}

void canvas_blit(const Canvas* canvas, int32_t x, int32_t y, const uint32_t* sprite,
                 int32_t width, int32_t height, bool mirrored)
{
    canvas_blit_any(canvas, x, y, sprite, width, height, mirrored, false, 0);
}

void canvas_blit_tinted(const Canvas* canvas, int32_t x, int32_t y, const uint32_t* sprite,
                        int32_t width, int32_t height, uint32_t color)
{
    canvas_blit_any(canvas, x, y, sprite, width, height, false, true, color);
}
