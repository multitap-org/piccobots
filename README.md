# piccobots

A small libretro core: four robots, one world, one view per player. Shoot
each other, collect gems; the first to 20 gems wins the round and a new map
comes. Multitap's demo and test core: four local players on four ports, each
with their own view of the same world.

- Video: 480x320, the four 240x160 views tiled 2x2 (P1 top left, P2 top
  right, P3 bottom left, P4 bottom right). The layout is one table
  (`draw_view_rect` in `src/draw.cpp`): the planned multiple-viewports
  extension will describe those rectangles so a frontend can show each view
  on its own.
- Input: a RetroPad per port. D-pad moves, A or B shoots.
- No content: the core starts on its own.
- Deterministic (integers, one PRNG in the state, input only from the pads)
  and savestates are a copy of the game's state, so run-ahead, rewind and
  netplay work as they are. `tests/game_test.cpp` checks it.
- No SDL, no dependencies: only `libretro.h` from `deps/libretro-common`.
- All art is drawn for piccobots as pictures in `src/art.cpp`; the text uses
  piccolo's 5x5 pixel font (`src/font.cpp`).

Code style: Multitap's `STYLE.md` (modern C++ written like C).

```sh
git clone --recursive https://github.com/multitap-org/piccobots.git
cmake -S piccobots -B piccobots/build -G Ninja -DPICCOBOTS_WARNINGS_AS_ERRORS=ON
cmake --build piccobots/build && ctest --test-dir piccobots/build
```

MIT (LICENSE).
