#include "game.h"

// The map: grass crossed by sand paths (three each way, two tiles wide), a
// lake, flowers and clumps of trees, a border of trees. A new one every round,
// from the game's PRNG (the same on every machine for the same match).

static const int32_t PATH_ROWS[]    = { 6, 17, 28 };
static const int32_t PATH_COLUMNS[] = { 7, 23, 39 };

constexpr int32_t TREE_CLUMPS   = 80;
constexpr int32_t FLOWER_PATCHES = 70;
constexpr int32_t SPAWN_CLEAR   = 3;   // tiles kept free of trees around a spawn

Tile world_tile_at(const World* world, int32_t pixel_x, int32_t pixel_y)
{
    if (pixel_x < 0 || pixel_y < 0)
    {
        return Tile::Tree;
    }
    int32_t column = pixel_x / TILE_SIZE;
    int32_t row = pixel_y / TILE_SIZE;
    if (column >= MAP_WIDTH || row >= MAP_HEIGHT)
    {
        return Tile::Tree;
    }
    return world->tiles[row][column];
}

bool world_tile_walkable(Tile tile)
{
    return tile != Tile::Tree && tile != Tile::Water;
}

static bool world_near_spawn(const World* world, int32_t column, int32_t row)
{
    for (int32_t i = 0; i < MAX_PLAYERS; i++)
    {
        int32_t spawn_column = world->spawn_x[i] / TILE_SIZE;
        int32_t spawn_row = world->spawn_y[i] / TILE_SIZE;
        int32_t dx = column - spawn_column;
        int32_t dy = row - spawn_row;
        if (dx >= -SPAWN_CLEAR && dx <= SPAWN_CLEAR && dy >= -SPAWN_CLEAR && dy <= SPAWN_CLEAR)
        {
            return true;
        }
    }
    return false;
}

// Tiles a robot can't reach from the first spawn (closed in by trees and
// water) become trees: gems only appear where someone can get them.
static void world_fill_unreachable(World* world)
{
    static_assert(MAP_WIDTH * MAP_HEIGHT <= 4096, "queue below");
    bool     reached[MAP_HEIGHT][MAP_WIDTH] = {};
    int16_t  queue[MAP_WIDTH * MAP_HEIGHT];
    int32_t  head = 0;
    int32_t  tail = 0;
    int32_t  start = (world->spawn_y[0] / TILE_SIZE) * MAP_WIDTH + world->spawn_x[0] / TILE_SIZE;
    queue[tail++] = static_cast<int16_t>(start);
    reached[start / MAP_WIDTH][start % MAP_WIDTH] = true;
    while (head < tail)
    {
        int32_t at = queue[head++];
        int32_t column = at % MAP_WIDTH;
        int32_t row = at / MAP_WIDTH;
        const int32_t steps[4][2] = { { 1, 0 }, { -1, 0 }, { 0, 1 }, { 0, -1 } };
        for (int32_t s = 0; s < 4; s++)
        {
            int32_t c = column + steps[s][0];
            int32_t r = row + steps[s][1];
            if (c < 0 || r < 0 || c >= MAP_WIDTH || r >= MAP_HEIGHT)
            {
                continue;
            }
            if (reached[r][c] || !world_tile_walkable(world->tiles[r][c]))
            {
                continue;
            }
            reached[r][c] = true;
            queue[tail++] = static_cast<int16_t>(r * MAP_WIDTH + c);
        }
    }
    for (int32_t row = 0; row < MAP_HEIGHT; row++)
    {
        for (int32_t column = 0; column < MAP_WIDTH; column++)
        {
            if (!reached[row][column] && world_tile_walkable(world->tiles[row][column]))
            {
                world->tiles[row][column] = Tile::Tree;
            }
        }
    }
}

void world_generate(World* world, Game* game)
{
    for (int32_t row = 0; row < MAP_HEIGHT; row++)
    {
        for (int32_t column = 0; column < MAP_WIDTH; column++)
        {
            bool border = row == 0 || column == 0 || row == MAP_HEIGHT - 1 || column == MAP_WIDTH - 1;
            world->tiles[row][column] = border ? Tile::Tree : Tile::Grass;
        }
    }

    // Paths.
    for (int32_t p = 0; p < 3; p++)
    {
        for (int32_t i = 1; i < MAP_WIDTH - 1; i++)
        {
            world->tiles[PATH_ROWS[p]][i] = Tile::Path;
            world->tiles[PATH_ROWS[p] + 1][i] = Tile::Path;
        }
        for (int32_t i = 1; i < MAP_HEIGHT - 1; i++)
        {
            world->tiles[i][PATH_COLUMNS[p]] = Tile::Path;
            world->tiles[i][PATH_COLUMNS[p] + 1] = Tile::Path;
        }
    }

    // Spawns: the four outer crossings, P1 top left, P2 top right, P3 bottom
    // left, P4 bottom right (as their views on screen). The center of the
    // crossing's 2x2 tiles.
    for (int32_t i = 0; i < MAX_PLAYERS; i++)
    {
        int32_t column = (i % 2 == 0) ? PATH_COLUMNS[0] : PATH_COLUMNS[2];
        int32_t row = (i < 2) ? PATH_ROWS[0] : PATH_ROWS[2];
        world->spawn_x[i] = (column + 1) * TILE_SIZE;
        world->spawn_y[i] = (row + 1) * TILE_SIZE;
    }

    // A lake in one of the four middle blocks between the paths: an ellipse
    // over grass only (paths stay dry).
    int32_t block = static_cast<int32_t>(game_random(game, 4));
    int32_t lake_column = (block % 2 == 0) ? (PATH_COLUMNS[0] + PATH_COLUMNS[1] + 2) / 2
                                           : (PATH_COLUMNS[1] + PATH_COLUMNS[2] + 2) / 2;
    int32_t lake_row = (block < 2) ? (PATH_ROWS[0] + PATH_ROWS[1] + 2) / 2 : (PATH_ROWS[1] + PATH_ROWS[2] + 2) / 2;
    for (int32_t row = lake_row - 4; row <= lake_row + 4; row++)
    {
        for (int32_t column = lake_column - 6; column <= lake_column + 6; column++)
        {
            int32_t dx = column - lake_column;
            int32_t dy = row - lake_row;
            // Inside (dx / 6)^2 + (dy / 4)^2 <= 1, in integers.
            if (dx * dx * 16 + dy * dy * 36 <= 16 * 36 && world->tiles[row][column] == Tile::Grass)
            {
                world->tiles[row][column] = Tile::Water;
            }
        }
    }

    for (int32_t i = 0; i < FLOWER_PATCHES; i++)
    {
        int32_t column = static_cast<int32_t>(game_random(game, MAP_WIDTH));
        int32_t row = static_cast<int32_t>(game_random(game, MAP_HEIGHT));
        if (world->tiles[row][column] == Tile::Grass)
        {
            world->tiles[row][column] = Tile::Flowers;
        }
    }

    // Clumps of one to four trees, on grass, away from the spawns.
    for (int32_t i = 0; i < TREE_CLUMPS; i++)
    {
        int32_t column = static_cast<int32_t>(game_random(game, MAP_WIDTH));
        int32_t row = static_cast<int32_t>(game_random(game, MAP_HEIGHT));
        int32_t size = 1 + static_cast<int32_t>(game_random(game, 4));
        for (int32_t t = 0; t < size; t++)
        {
            int32_t c = column + (t % 2);
            int32_t r = row + (t / 2);
            if (c >= MAP_WIDTH || r >= MAP_HEIGHT)
            {
                continue;
            }
            if (world->tiles[r][c] != Tile::Grass || world_near_spawn(world, c, r))
            {
                continue;
            }
            world->tiles[r][c] = Tile::Tree;
        }
    }

    world_fill_unreachable(world);
}
