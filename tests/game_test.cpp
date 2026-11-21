// The game without libretro: a scripted match (P1 and P2 walk at each other
// along the top path, shooting; P3 and P4 wander), checked for what
// savestates, run-ahead and netplay need:
//   - shots land (someone loses a heart),
//   - the same inputs give the same game, byte for byte,
//   - a copy of Game taken mid-match continues exactly like the original.

#include <cstdio>
#include <cstring>

#include "../src/game.h"

constexpr uint32_t SEED = 0x5049434Fu;

static void test_input(uint32_t frame, PadInput input[MAX_PLAYERS])
{
    for (int32_t i = 0; i < MAX_PLAYERS; i++)
    {
        input[i] = PadInput{};
    }
    input[0].right = true;
    input[0].shoot = frame % 20 == 0;
    input[1].left = true;
    input[1].shoot = frame % 25 == 0;
    // P3, P4: a few seconds each way.
    uint32_t phase = (frame / 90) % 4;
    input[2].right = phase == 0;
    input[2].down = phase == 1;
    input[2].left = phase == 2;
    input[2].up = phase == 3;
    input[3].left = phase == 0;
    input[3].up = phase == 1;
    input[3].right = phase == 2;
    input[3].down = phase == 3;
    input[3].shoot = frame % 30 == 0;
}

static void test_run(Game* game, uint32_t from, uint32_t to)
{
    for (uint32_t frame = from; frame < to; frame++)
    {
        PadInput input[MAX_PLAYERS];
        test_input(frame, input);
        game_step(game, input);
    }
}

static bool test_check(bool condition, const char* what)
{
    std::printf("%s: %s\n", condition ? "ok" : "FAILED", what);
    return condition;
}

int main()
{
    static Game first;
    static Game second;
    static Game saved;
    bool ok = true;

    game_init(&first, SEED);
    test_run(&first, 0, 400);
    std::memcpy(&saved, &first, sizeof(Game));
    ok = test_check(first.robots[0].hearts < MAX_HEARTS || first.robots[1].hearts < MAX_HEARTS ||
                        !game_robot_alive(&first.robots[0]) || !game_robot_alive(&first.robots[1]),
                    "P1 and P2 hit each other") && ok;
    test_run(&first, 400, 3000);

    game_init(&second, SEED);
    test_run(&second, 0, 3000);
    ok = test_check(std::memcmp(&first, &second, sizeof(Game)) == 0, "same inputs, same game") && ok;

    test_run(&saved, 400, 3000);
    ok = test_check(std::memcmp(&first, &saved, sizeof(Game)) == 0, "a copy taken at frame 400 continues the same") && ok;

    std::printf("after 3000 frames: round %u, gems %d %d %d %d\n", first.round, first.robots[0].gems,
                first.robots[1].gems, first.robots[2].gems, first.robots[3].gems);
    return ok ? 0 : 1;
}
