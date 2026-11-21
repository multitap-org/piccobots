#include "draw.h"

#include <cstdio>

#include "font.h"

constexpr int32_t HUD_HEIGHT     = 11;   // the bar on top of each view
constexpr uint32_t HUD_BACKGROUND = COLOR_OPAQUE | 0x141828;
constexpr uint32_t TEXT_WHITE    = COLOR_OPAQUE | 0xF4F4F4;
constexpr uint32_t TEXT_SHADOW   = COLOR_OPAQUE | 0x101018;
constexpr int32_t WORLD_WIDTH    = MAP_WIDTH * TILE_SIZE;    // pixels
constexpr int32_t WORLD_HEIGHT   = MAP_HEIGHT * TILE_SIZE;

ViewRect draw_view_rect(int32_t player)
{
    ViewRect rect;
    rect.x = (player % 2) * VIEW_WIDTH;
    rect.y = (player / 2) * VIEW_HEIGHT;
    rect.width = VIEW_WIDTH;
    rect.height = VIEW_HEIGHT;
    return rect;
}

static int32_t draw_clamp(int32_t value, int32_t low, int32_t high)
{
    if (value < low)
    {
        return low;
    }
    if (value > high)
    {
        return high;
    }
    return value;
}

static void draw_text_shadowed(const Canvas* canvas, int32_t x, int32_t y, const char* text,
                               uint32_t color, int32_t scale)
{
    font_draw_text(canvas, x + scale, y + scale, text, TEXT_SHADOW, scale);
    font_draw_text(canvas, x, y, text, color, scale);
}

// Centered across the view, at y within it.
static void draw_text_centered(const Canvas* canvas, const ViewRect* view, int32_t y, const char* text,
                               uint32_t color, int32_t scale)
{
    int32_t x = view->x + (view->width - font_text_width(text, scale)) / 2;
    draw_text_shadowed(canvas, x, view->y + y, text, color, scale);
}

static void draw_world(const Canvas* canvas, const ViewRect* view, const Game* game, const Art* art,
                       int32_t camera_x, int32_t camera_y)
{
    int32_t first_column = camera_x / TILE_SIZE;
    int32_t first_row = camera_y / TILE_SIZE;
    int32_t water_frame = static_cast<int32_t>((game->frame / 10) % WATER_FRAMES);
    for (int32_t row = first_row; row <= first_row + VIEW_HEIGHT / TILE_SIZE && row < MAP_HEIGHT; row++)
    {
        for (int32_t column = first_column; column <= first_column + VIEW_WIDTH / TILE_SIZE && column < MAP_WIDTH;
             column++)
        {
            Tile tile = game->world.tiles[row][column];
            const uint32_t* sprite = tile == Tile::Water ? art->water[water_frame]
                                                         : art->tiles[static_cast<int32_t>(tile)];
            canvas_blit(canvas, view->x + column * TILE_SIZE - camera_x, view->y + row * TILE_SIZE - camera_y,
                        sprite, TILE_SIZE, TILE_SIZE, false);
        }
    }

    for (int32_t i = 0; i < MAX_GEMS; i++)
    {
        const Gem* gem = &game->gems[i];
        if (!gem->active)
        {
            continue;
        }
        // A slow bob, out of step from gem to gem.
        int32_t bob = static_cast<int32_t>(((game->frame / 12) + static_cast<uint32_t>(i)) % 2);
        canvas_blit(canvas, view->x + gem->x - GEM_SPRITE / 2 - camera_x,
                    view->y + gem->y - GEM_SPRITE / 2 - camera_y - bob, art->gem, GEM_SPRITE, GEM_SPRITE, false);
    }

    for (int32_t r = 0; r < MAX_PLAYERS; r++)
    {
        const Robot* robot = &game->robots[r];
        if (!game_robot_alive(robot))
        {
            continue;
        }
        if (robot->hurt_frames > 0 && (robot->hurt_frames / 4) % 2 == 1)
        {
            continue;   // blinking
        }
        int32_t pose = 0;
        bool mirrored = false;
        switch (robot->facing)
        {
            case Facing::Down:  pose = 0; break;
            case Facing::Up:    pose = 1; break;
            case Facing::Left:  pose = 2; mirrored = true; break;
            case Facing::Right: pose = 2; break;
        }
        int32_t bob = robot->walk_frames > 0 ? (robot->walk_frames / 6) % 2 : 0;
        canvas_blit(canvas, view->x + robot->x / SUBPIXELS - ROBOT_SPRITE / 2 - camera_x,
                    view->y + robot->y / SUBPIXELS - ROBOT_SPRITE / 2 - camera_y - bob,
                    art->robots[r][pose], ROBOT_SPRITE, ROBOT_SPRITE, mirrored);
    }

    for (int32_t i = 0; i < MAX_SHOTS; i++)
    {
        const Shot* shot = &game->shots[i];
        if (!shot->active)
        {
            continue;
        }
        canvas_blit(canvas, view->x + shot->x / SUBPIXELS - SHOT_SPRITE / 2 - camera_x,
                    view->y + shot->y / SUBPIXELS - SHOT_SPRITE / 2 - camera_y,
                    art->shots[shot->owner], SHOT_SPRITE, SHOT_SPRITE, false);
    }
}

// The bar on top: "P1", the gems, the hearts.
static void draw_hud(const Canvas* canvas, const ViewRect* view, const Game* game, const Art* art, int32_t player)
{
    const Robot* robot = &game->robots[player];
    canvas_fill_rect(canvas, view->x, view->y, view->width, HUD_HEIGHT, HUD_BACKGROUND);
    canvas_fill_rect(canvas, view->x, view->y + HUD_HEIGHT, view->width, 1, art->player_color[player]);

    char text[16];
    std::snprintf(text, sizeof(text), "P%d", player + 1);
    font_draw_text(canvas, view->x + 4, view->y + 3, text, art->player_light[player], 1);
    std::snprintf(text, sizeof(text), "GEMS %02d", robot->gems);
    int32_t gems_x = view->x + (view->width - font_text_width(text, 1)) / 2;
    font_draw_text(canvas, gems_x, view->y + 3, text, TEXT_WHITE, 1);

    for (int32_t h = 0; h < MAX_HEARTS; h++)
    {
        const uint32_t* heart = h < robot->hearts ? art->heart_full : art->heart_empty;
        int32_t x = view->x + view->width - 4 - (MAX_HEARTS - h) * (HEART_WIDTH + 2) + 2;
        canvas_blit(canvas, x, view->y + 3, heart, HEART_WIDTH, HEART_HEIGHT, false);
    }
}

static void draw_view(uint32_t* pixels, const Game* game, const Art* art, int32_t player)
{
    ViewRect view = draw_view_rect(player);
    Canvas canvas;
    canvas.pixels = pixels;
    canvas.pitch_pixels = SCREEN_WIDTH;
    canvas.clip_x = view.x;
    canvas.clip_y = view.y;
    canvas.clip_width = view.width;
    canvas.clip_height = view.height;

    // The camera keeps the robot in the middle of the part under the bar,
    // and the world's edges at the view's edges.
    const Robot* robot = &game->robots[player];
    int32_t play_height = VIEW_HEIGHT - HUD_HEIGHT;
    int32_t camera_x = robot->x / SUBPIXELS - VIEW_WIDTH / 2;
    int32_t camera_y = robot->y / SUBPIXELS - HUD_HEIGHT - play_height / 2;
    camera_x = draw_clamp(camera_x, 0, WORLD_WIDTH - VIEW_WIDTH);
    camera_y = draw_clamp(camera_y, -HUD_HEIGHT, WORLD_HEIGHT - VIEW_HEIGHT);

    draw_world(&canvas, &view, game, art, camera_x, camera_y);
    draw_hud(&canvas, &view, game, art, player);

    char text[24];
    if (game->phase == Phase::RoundOver)
    {
        std::snprintf(text, sizeof(text), "P%d WINS!", game->winner + 1);
        draw_text_centered(&canvas, &view, 62, text, art->player_light[game->winner], 3);
        int32_t seconds = (game->phase_frames + 59) / 60;
        std::snprintf(text, sizeof(text), "NEXT ROUND IN %d", seconds);
        draw_text_centered(&canvas, &view, 88, text, TEXT_WHITE, 1);
    }
    else if (!game_robot_alive(robot))
    {
        draw_text_centered(&canvas, &view, 66, "DOWN", art->player_light[player], 2);
        int32_t seconds = (robot->down_frames + 59) / 60;
        std::snprintf(text, sizeof(text), "BACK IN %d", seconds);
        draw_text_centered(&canvas, &view, 84, text, TEXT_WHITE, 1);
    }

    // A frame in the player's shade, so the four views read apart.
    uint32_t edge = art->player_dark[player];
    canvas_fill_rect(&canvas, view.x, view.y, view.width, 1, edge);
    canvas_fill_rect(&canvas, view.x, view.y + view.height - 1, view.width, 1, edge);
    canvas_fill_rect(&canvas, view.x, view.y, 1, view.height, edge);
    canvas_fill_rect(&canvas, view.x + view.width - 1, view.y, 1, view.height, edge);
}

void draw_frame(const Game* game, const Art* art, uint32_t* pixels)
{
    for (int32_t player = 0; player < MAX_PLAYERS; player++)
    {
        draw_view(pixels, game, art, player);
    }
}
