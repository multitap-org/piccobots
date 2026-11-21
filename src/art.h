#pragma once

// piccobots' pixel art, drawn for it as pictures in art.cpp (one character
// per pixel) and turned into XRGB8888 sprites once, at retro_init.

#include "canvas.h"
#include "game.h"

constexpr int32_t ROBOT_SPRITE  = 16;   // robots are 16x16
constexpr int32_t ROBOT_POSES   = 3;    // facing down, up, to the side (left = side mirrored)
constexpr int32_t GEM_SPRITE    = 8;
constexpr int32_t SHOT_SPRITE   = 6;
constexpr int32_t HEART_WIDTH   = 7;
constexpr int32_t HEART_HEIGHT  = 6;
constexpr int32_t TILE_KINDS    = 5;    // Tile's values
constexpr int32_t WATER_FRAMES  = TILE_SIZE;   // the water tile shifted 0..15 pixels

struct Art
{
    uint32_t tiles[TILE_KINDS][TILE_SIZE * TILE_SIZE];
    uint32_t water[WATER_FRAMES][TILE_SIZE * TILE_SIZE];
    uint32_t robots[MAX_PLAYERS][ROBOT_POSES][ROBOT_SPRITE * ROBOT_SPRITE];
    uint32_t shots[MAX_PLAYERS][SHOT_SPRITE * SHOT_SPRITE];
    uint32_t gem[GEM_SPRITE * GEM_SPRITE];
    uint32_t heart_full[HEART_WIDTH * HEART_HEIGHT];
    uint32_t heart_empty[HEART_WIDTH * HEART_HEIGHT];
    uint32_t player_color[MAX_PLAYERS];   // the robot's body
    uint32_t player_dark[MAX_PLAYERS];
    uint32_t player_light[MAX_PLAYERS];
};

// False (and logged) if a picture is malformed: a row of the wrong length or
// a character missing from its palette.
bool art_init(Art* art);
