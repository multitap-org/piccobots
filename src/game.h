#pragma once

// piccobots' game: four robots in one world, one per port, all local. Each
// player has a view of the same world following their robot (draw.h tiles
// the four views in one framebuffer; later, Multitap's multiple-viewports
// extension hands each view to the frontend on its own). They shoot each
// other and collect gems; the first to GEMS_TO_WIN wins the round.
//
// Everything here is deterministic: integers only, one PRNG inside the
// state, input only from the four pads. The same inputs give the same game
// on every machine, so savestates, run-ahead and netplay work as they are.
// Game holds no pointers: a savestate is a copy of it. Its structs are plain
// data without default values: game_init zeroes the whole of it (padding
// too, so equal states are equal bytes) and sets what isn't zero.

#include <cstdint>

#include "sound.h"

constexpr int32_t MAX_PLAYERS  = 4;
constexpr int32_t TILE_SIZE    = 16;
constexpr int32_t MAP_WIDTH    = 48;   // tiles
constexpr int32_t MAP_HEIGHT   = 36;
constexpr int32_t SUBPIXELS    = 16;   // positions are in 1/16 pixels
constexpr int32_t MAX_SHOTS    = 32;
constexpr int32_t MAX_GEMS     = 40;
constexpr int32_t MAX_HEARTS   = 3;
constexpr int32_t GEMS_TO_WIN  = 20;
constexpr int32_t ROUND_OVER_FRAMES = 300;   // the winner's message, then a new map

enum class Tile : uint8_t
{
    Grass,
    Path,
    Flowers,
    Tree,    // blocks robots and shots
    Water,   // blocks robots; shots fly over it
};

enum class Facing : uint8_t
{
    Down,
    Up,
    Left,
    Right,
};

enum class Phase : uint8_t
{
    Playing,
    RoundOver,
};

struct PadInput
{
    bool up    = false;
    bool down  = false;
    bool left  = false;
    bool right = false;
    bool shoot = false;
};

struct World
{
    Tile    tiles[MAP_HEIGHT][MAP_WIDTH];
    int32_t spawn_x[MAX_PLAYERS];   // where each robot starts, pixels (its center)
    int32_t spawn_y[MAX_PLAYERS];
};

struct Robot
{
    int32_t x;              // center, subpixels
    int32_t y;
    int32_t aim_x;          // -1, 0, 1: the last direction moved (where shots go)
    int32_t aim_y;
    Facing  facing;
    int32_t hearts;
    int32_t gems;
    int32_t hurt_frames;    // > 0: just hit, can't be hit again (blinks)
    int32_t down_frames;    // > 0: lost every heart, respawns at 0
    int32_t shoot_frames;   // > 0: can't shoot yet
    int32_t push_x;         // knockback from the last hit, subpixels per frame
    int32_t push_y;
    int32_t walk_frames;    // for the walking bob
};

struct Shot
{
    bool    active;
    uint8_t owner;
    int32_t x;   // subpixels
    int32_t y;
    int32_t vx;  // subpixels per frame
    int32_t vy;
    int32_t frames_left;
};

struct Gem
{
    bool    active;
    int32_t x;   // center, pixels
    int32_t y;
};

struct Game
{
    uint32_t random;      // xorshift32 state, never 0
    uint32_t frame;
    uint32_t round;
    Phase    phase;
    int32_t  phase_frames;
    int32_t  winner;
    int32_t  gem_frames;  // until the next gem appears
    World    world;
    Robot    robots[MAX_PLAYERS];
    Shot     shots[MAX_SHOTS];
    Gem      gems[MAX_GEMS];
    Sound    sound;
};

// A new match from `seed`: a new map, everyone at their spawn.
void game_init(Game* game, uint32_t seed);
void game_step(Game* game, const PadInput input[MAX_PLAYERS]);

// 0 .. range - 1 from the game's PRNG.
uint32_t game_random(Game* game, uint32_t range);
bool game_robot_alive(const Robot* robot);

// world.cpp
void world_generate(World* world, Game* game);
Tile world_tile_at(const World* world, int32_t pixel_x, int32_t pixel_y);   // outside = Tree
bool world_tile_walkable(Tile tile);
