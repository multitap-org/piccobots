#include "game.h"

#include <cstring>

constexpr int32_t ROBOT_SPEED       = 20;    // subpixels per frame (1.25 px)
constexpr int32_t ROBOT_SPEED_DIAG  = 14;    // the same, each axis, moving diagonally
constexpr int32_t ROBOT_HALF        = 5 * SUBPIXELS;   // half the robot's box for walls
constexpr int32_t ROBOT_HIT_HALF    = 7 * SUBPIXELS;   // half its box for shots
constexpr int32_t GEM_REACH         = 10 * SUBPIXELS;  // how close picks a gem up
constexpr int32_t SHOT_SPEED        = 64;    // 4 px per frame
constexpr int32_t SHOT_SPEED_DIAG   = 45;
constexpr int32_t SHOT_FRAMES       = 50;
constexpr int32_t SHOOT_COOLDOWN    = 18;
constexpr int32_t HURT_FRAMES       = 60;
constexpr int32_t DOWN_FRAMES       = 120;   // 2 s out after losing every heart
constexpr int32_t RESPAWN_HURT      = 90;    // can't be hit right after respawning
constexpr int32_t GEM_INTERVAL      = 40;    // frames between new gems
constexpr int32_t GEMS_ON_MAP       = 24;    // no new ones above this
constexpr int32_t GEMS_AT_START     = 12;

uint32_t game_random(Game* game, uint32_t range)
{
    // xorshift32: small, fast and the same everywhere.
    uint32_t x = game->random;
    x ^= x << 13;
    x ^= x >> 17;
    x ^= x << 5;
    game->random = x;
    return range == 0 ? 0 : x % range;
}

bool game_robot_alive(const Robot* robot)
{
    return robot->down_frames == 0;
}

static int32_t game_abs(int32_t value)
{
    return value < 0 ? -value : value;
}

// The robot's box (center x, y, half size ROBOT_HALF) touches a tree or water.
static bool game_robot_blocked(const World* world, int32_t x, int32_t y)
{
    int32_t left = (x - ROBOT_HALF) / SUBPIXELS;
    int32_t right = (x + ROBOT_HALF - 1) / SUBPIXELS;
    int32_t top = (y - ROBOT_HALF) / SUBPIXELS;
    int32_t bottom = (y + ROBOT_HALF - 1) / SUBPIXELS;
    return !world_tile_walkable(world_tile_at(world, left, top)) ||
           !world_tile_walkable(world_tile_at(world, right, top)) ||
           !world_tile_walkable(world_tile_at(world, left, bottom)) ||
           !world_tile_walkable(world_tile_at(world, right, bottom));
}

// Moves along one axis a pixel at a time, stopping at the first wall: the
// robot slides along walls instead of stopping short of them.
static void game_move_axis(const World* world, Robot* robot, int32_t amount, bool horizontal)
{
    while (amount != 0)
    {
        int32_t step = amount;
        if (step > SUBPIXELS)
        {
            step = SUBPIXELS;
        }
        if (step < -SUBPIXELS)
        {
            step = -SUBPIXELS;
        }
        int32_t x = horizontal ? robot->x + step : robot->x;
        int32_t y = horizontal ? robot->y : robot->y + step;
        if (game_robot_blocked(world, x, y))
        {
            return;
        }
        robot->x = x;
        robot->y = y;
        amount -= step;
    }
}

static int32_t game_count_gems(const Game* game)
{
    int32_t count = 0;
    for (int32_t i = 0; i < MAX_GEMS; i++)
    {
        if (game->gems[i].active)
        {
            count++;
        }
    }
    return count;
}

// A gem on a walkable tile: anywhere on the map, or (near = true) within
// three tiles of (x, y) pixels. False if no free gem slot or spot was found.
static bool game_place_gem(Game* game, bool near, int32_t x, int32_t y)
{
    int32_t slot = -1;
    for (int32_t i = 0; i < MAX_GEMS; i++)
    {
        if (!game->gems[i].active)
        {
            slot = i;
            break;
        }
    }
    if (slot < 0)
    {
        return false;
    }
    for (int32_t attempt = 0; attempt < 16; attempt++)
    {
        int32_t column = 0;
        int32_t row = 0;
        if (near)
        {
            column = x / TILE_SIZE - 3 + static_cast<int32_t>(game_random(game, 7));
            row = y / TILE_SIZE - 3 + static_cast<int32_t>(game_random(game, 7));
        }
        else
        {
            column = static_cast<int32_t>(game_random(game, MAP_WIDTH));
            row = static_cast<int32_t>(game_random(game, MAP_HEIGHT));
        }
        int32_t gem_x = column * TILE_SIZE + TILE_SIZE / 2;
        int32_t gem_y = row * TILE_SIZE + TILE_SIZE / 2;
        if (!world_tile_walkable(world_tile_at(&game->world, gem_x, gem_y)))
        {
            continue;
        }
        game->gems[slot].active = true;
        game->gems[slot].x = gem_x;
        game->gems[slot].y = gem_y;
        return true;
    }
    return false;
}

static void game_respawn(Game* game, int32_t index)
{
    Robot* robot = &game->robots[index];
    robot->x = game->world.spawn_x[index] * SUBPIXELS;
    robot->y = game->world.spawn_y[index] * SUBPIXELS;
    robot->hearts = MAX_HEARTS;
    robot->down_frames = 0;
    robot->hurt_frames = RESPAWN_HURT;
    robot->push_x = 0;
    robot->push_y = 0;
    robot->aim_x = 0;
    robot->aim_y = 1;
    robot->facing = Facing::Down;
}

// A new map with everyone at their spawn, no gems collected.
static void game_new_round(Game* game)
{
    game->round++;
    game->phase = Phase::Playing;
    game->phase_frames = 0;
    game->winner = -1;
    game->gem_frames = GEM_INTERVAL;
    world_generate(&game->world, game);
    for (int32_t i = 0; i < MAX_SHOTS; i++)
    {
        game->shots[i].active = false;
    }
    for (int32_t i = 0; i < MAX_GEMS; i++)
    {
        game->gems[i].active = false;
    }
    for (int32_t i = 0; i < MAX_PLAYERS; i++)
    {
        game_respawn(game, i);
        game->robots[i].gems = 0;
        game->robots[i].hurt_frames = 0;
        game->robots[i].shoot_frames = 0;
    }
    for (int32_t i = 0; i < GEMS_AT_START; i++)
    {
        game_place_gem(game, false, 0, 0);
    }
}

void game_init(Game* game, uint32_t seed)
{
    // The padding between fields is zeroed too, so two savestates of the
    // same moment are the same bytes (netplay compares them).
    std::memset(game, 0, sizeof(Game));
    game->random = seed != 0 ? seed : 1;
    game_new_round(game);
}

static void game_robot_down(Game* game, int32_t index)
{
    Robot* robot = &game->robots[index];
    // Half its gems fall around it, for whoever gets there first.
    int32_t dropped = robot->gems / 2;
    robot->gems -= dropped;
    for (int32_t i = 0; i < dropped; i++)
    {
        game_place_gem(game, true, robot->x / SUBPIXELS, robot->y / SUBPIXELS);
    }
    robot->down_frames = DOWN_FRAMES;
    robot->push_x = 0;
    robot->push_y = 0;
    sound_play(&game->sound, SoundEffect::Down, index);
}

static void game_update_robot(Game* game, int32_t index, const PadInput* pad)
{
    Robot* robot = &game->robots[index];
    if (robot->down_frames > 0)
    {
        robot->down_frames--;
        if (robot->down_frames == 0)
        {
            game_respawn(game, index);
        }
        return;
    }
    if (robot->hurt_frames > 0)
    {
        robot->hurt_frames--;
    }
    if (robot->shoot_frames > 0)
    {
        robot->shoot_frames--;
    }

    int32_t dx = (pad->right ? 1 : 0) - (pad->left ? 1 : 0);
    int32_t dy = (pad->down ? 1 : 0) - (pad->up ? 1 : 0);
    if (dx != 0 || dy != 0)
    {
        robot->aim_x = dx;
        robot->aim_y = dy;
        if (dx != 0)
        {
            robot->facing = dx < 0 ? Facing::Left : Facing::Right;
        }
        else
        {
            robot->facing = dy < 0 ? Facing::Up : Facing::Down;
        }
        robot->walk_frames++;
    }
    else
    {
        robot->walk_frames = 0;
    }
    int32_t speed = (dx != 0 && dy != 0) ? ROBOT_SPEED_DIAG : ROBOT_SPEED;
    game_move_axis(&game->world, robot, dx * speed + robot->push_x, true);
    game_move_axis(&game->world, robot, dy * speed + robot->push_y, false);
    // Knockback fades out.
    robot->push_x = robot->push_x * 3 / 4;
    robot->push_y = robot->push_y * 3 / 4;

    if (pad->shoot && robot->shoot_frames == 0)
    {
        for (int32_t i = 0; i < MAX_SHOTS; i++)
        {
            Shot* shot = &game->shots[i];
            if (shot->active)
            {
                continue;
            }
            int32_t shot_speed = (robot->aim_x != 0 && robot->aim_y != 0) ? SHOT_SPEED_DIAG : SHOT_SPEED;
            shot->active = true;
            shot->owner = static_cast<uint8_t>(index);
            shot->vx = robot->aim_x * shot_speed;
            shot->vy = robot->aim_y * shot_speed;
            shot->x = robot->x + robot->aim_x * 6 * SUBPIXELS;
            shot->y = robot->y + robot->aim_y * 6 * SUBPIXELS;
            shot->frames_left = SHOT_FRAMES;
            robot->shoot_frames = SHOOT_COOLDOWN;
            sound_play(&game->sound, SoundEffect::Shoot, index);
            break;
        }
    }

    for (int32_t i = 0; i < MAX_GEMS; i++)
    {
        Gem* gem = &game->gems[i];
        if (!gem->active)
        {
            continue;
        }
        if (game_abs(gem->x * SUBPIXELS - robot->x) < GEM_REACH && game_abs(gem->y * SUBPIXELS - robot->y) < GEM_REACH)
        {
            gem->active = false;
            robot->gems++;
            sound_play(&game->sound, SoundEffect::Gem, index);
            if (robot->gems >= GEMS_TO_WIN && game->phase == Phase::Playing)
            {
                game->phase = Phase::RoundOver;
                game->phase_frames = ROUND_OVER_FRAMES;
                game->winner = index;
                sound_play(&game->sound, SoundEffect::Win, -1);
            }
        }
    }
}

static void game_update_shots(Game* game)
{
    for (int32_t i = 0; i < MAX_SHOTS; i++)
    {
        Shot* shot = &game->shots[i];
        if (!shot->active)
        {
            continue;
        }
        shot->x += shot->vx;
        shot->y += shot->vy;
        shot->frames_left--;
        // Trees stop shots; water doesn't.
        if (shot->frames_left <= 0 ||
            world_tile_at(&game->world, shot->x / SUBPIXELS, shot->y / SUBPIXELS) == Tile::Tree)
        {
            shot->active = false;
            continue;
        }
        for (int32_t r = 0; r < MAX_PLAYERS; r++)
        {
            Robot* robot = &game->robots[r];
            if (r == shot->owner || !game_robot_alive(robot) || robot->hurt_frames > 0)
            {
                continue;
            }
            if (game_abs(shot->x - robot->x) >= ROBOT_HIT_HALF || game_abs(shot->y - robot->y) >= ROBOT_HIT_HALF)
            {
                continue;
            }
            shot->active = false;
            robot->hearts--;
            robot->hurt_frames = HURT_FRAMES;
            robot->push_x = shot->vx / 2;
            robot->push_y = shot->vy / 2;
            sound_play(&game->sound, SoundEffect::Hit, r);
            if (robot->hearts <= 0)
            {
                game_robot_down(game, r);
            }
            break;
        }
    }
}

void game_step(Game* game, const PadInput input[MAX_PLAYERS])
{
    game->frame++;

    if (game->phase == Phase::RoundOver)
    {
        game->phase_frames--;
        if (game->phase_frames <= 0)
        {
            game_new_round(game);
        }
        return;
    }

    for (int32_t i = 0; i < MAX_PLAYERS; i++)
    {
        game_update_robot(game, i, &input[i]);
        if (game->phase != Phase::Playing)
        {
            return;   // someone just won
        }
    }
    game_update_shots(game);

    game->gem_frames--;
    if (game->gem_frames <= 0)
    {
        game->gem_frames = GEM_INTERVAL;
        if (game_count_gems(game) < GEMS_ON_MAP)
        {
            game_place_gem(game, false, 0, 0);
        }
    }
}
