// piccobots' libretro entry points: the callbacks in, a frame of the game
// out. No content: the core starts on its own (SET_SUPPORT_NO_GAME). Video
// is our own XRGB8888 framebuffer, audio our own square waves, input the
// four RetroPads; nothing else from the frontend.

#include <cstdarg>
#include <cstdio>
#include <cstring>

#include <libretro.h>

#include "art.h"
#include "draw.h"
#include "game.h"
#include "sound.h"

constexpr uint32_t SAVE_MAGIC   = 0x54424350u;   // "PCBT"
constexpr uint32_t SAVE_VERSION = 1;              // bump when Game changes
constexpr uint32_t MATCH_SEED   = 0x5049434Fu;    // every match starts the same: netplay peers agree

struct SaveHeader
{
    uint32_t magic;
    uint32_t version;
    uint32_t size;   // sizeof(Game)
};

// The core's one global. libretro's callbacks carry no userdata, so the
// state they reach has to live somewhere global: it lives here, in one
// struct, and nowhere else. `loaded` says whether game/pixels are in use.
struct Core
{
    retro_environment_t        environment = nullptr;
    retro_video_refresh_t      video_refresh = nullptr;
    retro_audio_sample_batch_t audio_batch = nullptr;
    retro_input_poll_t         input_poll = nullptr;
    retro_input_state_t        input_state = nullptr;
    retro_log_printf_t         log = nullptr;
    bool                       art_ok = false;
    bool                       loaded = false;
    Art                        art;
    Game                       game;
    uint32_t                   pixels[SCREEN_WIDTH * SCREEN_HEIGHT];
    int16_t                    audio[SOUND_FRAMES_PER_FRAME * 2];
};

static Core core;

static void core_log(retro_log_level level, const char* format, ...)
{
    char text[256];
    va_list args;
    va_start(args, format);
    std::vsnprintf(text, sizeof(text), format, args);
    va_end(args);
    if (core.log != nullptr)
    {
        core.log(level, "[piccobots] %s\n", text);
    }
    else
    {
        std::fprintf(stderr, "[piccobots] %s\n", text);
    }
}

RETRO_API unsigned retro_api_version(void)
{
    return RETRO_API_VERSION;
}

RETRO_API void retro_set_environment(retro_environment_t environment)
{
    core.environment = environment;

    bool no_game = true;
    environment(RETRO_ENVIRONMENT_SET_SUPPORT_NO_GAME, &no_game);

    // Four ports, a RetroPad each.
    static const retro_controller_description PADS[] = {
        { "RetroPad", RETRO_DEVICE_JOYPAD },
    };
    static const retro_controller_info PORTS[] = {
        { PADS, 1 }, { PADS, 1 }, { PADS, 1 }, { PADS, 1 }, { nullptr, 0 },
    };
    environment(RETRO_ENVIRONMENT_SET_CONTROLLER_INFO, const_cast<retro_controller_info*>(PORTS));

    retro_log_callback log_callback;
    if (environment(RETRO_ENVIRONMENT_GET_LOG_INTERFACE, &log_callback))
    {
        core.log = log_callback.log;
    }
}

RETRO_API void retro_set_video_refresh(retro_video_refresh_t video_refresh)
{
    core.video_refresh = video_refresh;
}

RETRO_API void retro_set_audio_sample(retro_audio_sample_t)
{
    // Only the batch callback is used.
}

RETRO_API void retro_set_audio_sample_batch(retro_audio_sample_batch_t audio_batch)
{
    core.audio_batch = audio_batch;
}

RETRO_API void retro_set_input_poll(retro_input_poll_t input_poll)
{
    core.input_poll = input_poll;
}

RETRO_API void retro_set_input_state(retro_input_state_t input_state)
{
    core.input_state = input_state;
}

RETRO_API void retro_init(void)
{
    core.art_ok = art_init(&core.art);
    if (!core.art_ok)
    {
        core_log(RETRO_LOG_ERROR, "the pictures are malformed (see stderr)");
    }
}

RETRO_API void retro_deinit(void)
{
    core.loaded = false;
}

RETRO_API void retro_get_system_info(retro_system_info* info)
{
    std::memset(info, 0, sizeof(*info));
    info->library_name = "piccobots";
    info->library_version = "0.1";
    info->valid_extensions = "";
    info->need_fullpath = false;
    info->block_extract = false;
}

RETRO_API void retro_get_system_av_info(retro_system_av_info* info)
{
    std::memset(info, 0, sizeof(*info));
    info->geometry.base_width = SCREEN_WIDTH;
    info->geometry.base_height = SCREEN_HEIGHT;
    info->geometry.max_width = SCREEN_WIDTH;
    info->geometry.max_height = SCREEN_HEIGHT;
    info->geometry.aspect_ratio = static_cast<float>(SCREEN_WIDTH) / static_cast<float>(SCREEN_HEIGHT);
    info->timing.fps = 60.0;
    info->timing.sample_rate = SOUND_RATE;
}

RETRO_API void retro_set_controller_port_device(unsigned, unsigned)
{
    // Always a RetroPad.
}

RETRO_API void retro_reset(void)
{
    game_init(&core.game, MATCH_SEED);
}

static void core_describe_input(void)
{
    static const retro_input_descriptor DESCRIPTORS[] = {
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Shoot" },
        { 0, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Shoot" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Shoot" },
        { 1, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Shoot" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Shoot" },
        { 2, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Shoot" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_UP, "Up" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_DOWN, "Down" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_LEFT, "Left" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_RIGHT, "Right" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_A, "Shoot" },
        { 3, RETRO_DEVICE_JOYPAD, 0, RETRO_DEVICE_ID_JOYPAD_B, "Shoot" },
        { 0, 0, 0, 0, nullptr },
    };
    core.environment(RETRO_ENVIRONMENT_SET_INPUT_DESCRIPTORS, const_cast<retro_input_descriptor*>(DESCRIPTORS));
}

RETRO_API bool retro_load_game(const retro_game_info*)
{
    if (!core.art_ok)
    {
        return false;
    }
    retro_pixel_format format = RETRO_PIXEL_FORMAT_XRGB8888;
    if (!core.environment(RETRO_ENVIRONMENT_SET_PIXEL_FORMAT, &format))
    {
        core_log(RETRO_LOG_ERROR, "the frontend doesn't take XRGB8888");
        return false;
    }
    core_describe_input();
    game_init(&core.game, MATCH_SEED);
    core.loaded = true;
    return true;
}

RETRO_API bool retro_load_game_special(unsigned, const retro_game_info*, size_t)
{
    return false;
}

RETRO_API void retro_unload_game(void)
{
    core.loaded = false;
}

static bool core_pressed(unsigned port, unsigned id)
{
    return core.input_state(port, RETRO_DEVICE_JOYPAD, 0, id) != 0;
}

RETRO_API void retro_run(void)
{
    core.input_poll();
    PadInput input[MAX_PLAYERS];
    for (int32_t i = 0; i < MAX_PLAYERS; i++)
    {
        unsigned port = static_cast<unsigned>(i);
        input[i].up = core_pressed(port, RETRO_DEVICE_ID_JOYPAD_UP);
        input[i].down = core_pressed(port, RETRO_DEVICE_ID_JOYPAD_DOWN);
        input[i].left = core_pressed(port, RETRO_DEVICE_ID_JOYPAD_LEFT);
        input[i].right = core_pressed(port, RETRO_DEVICE_ID_JOYPAD_RIGHT);
        input[i].shoot = core_pressed(port, RETRO_DEVICE_ID_JOYPAD_A) || core_pressed(port, RETRO_DEVICE_ID_JOYPAD_B);
    }
    game_step(&core.game, input);

    draw_frame(&core.game, &core.art, core.pixels);
    core.video_refresh(core.pixels, SCREEN_WIDTH, SCREEN_HEIGHT, SCREEN_WIDTH * sizeof(uint32_t));

    sound_render(&core.game.sound, core.audio);
    core.audio_batch(core.audio, SOUND_FRAMES_PER_FRAME);
}

// Savestates: a header and a copy of Game (no pointers in it, see game.h).
RETRO_API size_t retro_serialize_size(void)
{
    return sizeof(SaveHeader) + sizeof(Game);
}

RETRO_API bool retro_serialize(void* data, size_t size)
{
    if (size < retro_serialize_size())
    {
        return false;
    }
    SaveHeader header = { SAVE_MAGIC, SAVE_VERSION, static_cast<uint32_t>(sizeof(Game)) };
    unsigned char* bytes = static_cast<unsigned char*>(data);
    std::memcpy(bytes, &header, sizeof(header));
    std::memcpy(bytes + sizeof(header), &core.game, sizeof(Game));
    return true;
}

RETRO_API bool retro_unserialize(const void* data, size_t size)
{
    if (size < retro_serialize_size())
    {
        return false;
    }
    const unsigned char* bytes = static_cast<const unsigned char*>(data);
    SaveHeader header;
    std::memcpy(&header, bytes, sizeof(header));
    if (header.magic != SAVE_MAGIC || header.version != SAVE_VERSION || header.size != sizeof(Game))
    {
        core_log(RETRO_LOG_WARN, "not a savestate of this piccobots");
        return false;
    }
    std::memcpy(&core.game, bytes + sizeof(header), sizeof(Game));
    return true;
}

RETRO_API void retro_cheat_reset(void)
{
}

RETRO_API void retro_cheat_set(unsigned, bool, const char*)
{
}

RETRO_API unsigned retro_get_region(void)
{
    return RETRO_REGION_NTSC;
}

RETRO_API void* retro_get_memory_data(unsigned)
{
    return nullptr;
}

RETRO_API size_t retro_get_memory_size(unsigned)
{
    return 0;
}
