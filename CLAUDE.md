# CLAUDE.md — piccobots

Working brief for Claude Code sessions here. piccobots is a libretro core,
part of the Multitap project (github.com/multitap-org): a small game that
is Multitap's demo and test core. Owner: Andrés (`andres-asm`); he wants
direct feedback and replies in the language he writes in.

## Decisions (Andrés, 2026-11-21)

- Four local players, one per port, each with a view of the same world
  following their robot. The views are tiled 2x2 in one framebuffer and
  nothing more for now.
- It's the test core for Multitap's planned multiple-viewports libretro
  extension (one view per player handed to the frontend). Until that
  extension exists, one framebuffer. Keep the layout in `draw_view_rect`.
- No SDL in the core (a core never opens windows or reads devices).

## Rules

- Multitap's STYLE.md: C++20 written like C (no auto, lambdas, own templates,
  classes with behavior, exceptions, RTTI). Plain structs + `module_verb()`.
- The game stays deterministic: integers only, the PRNG lives in `Game`,
  input only from the pads, `Game` holds no pointers and is plain data
  (savestate = copy). `tests/game_test.cpp` must keep passing.
- Art is drawn for piccobots (pictures in `src/art.cpp`), never copied.
- Build: `-DPICCOBOTS_WARNINGS_AS_ERRORS=ON`, ctest, and an ASan build
  (`-DPICCOBOTS_SANITIZE=ON`) for larger changes. Run it headless with
  host-libretro's `libretro_core_info --frames N --dump-frame out.ppm`.
- Commit trailer: `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`.

## Layout

- `src/libretro_core.cpp` the retro_* functions, the one global (`Core`)
- `src/game.*` state and rules; `src/world.cpp` the map; `src/sound.*` effects
- `src/draw.*` the four views; `src/art.*` pictures; `src/font.*`, `src/canvas.*`
- `tests/game_test.cpp` determinism and savestates
