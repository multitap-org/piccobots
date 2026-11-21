#pragma once

// The sound effects: square waves and noise, made sample by sample. Their
// state lives inside the game's (Game::sound), so a savestate, run-ahead or
// netplay rollback brings the sound back exactly as the picture.

#include <cstdint>

constexpr int32_t SOUND_RATE             = 48000;
constexpr int32_t SOUND_FRAMES_PER_FRAME = SOUND_RATE / 60;   // stereo frames per video frame
constexpr int32_t SOUND_VOICES           = 6;
constexpr int32_t SOUND_MAX_SEGMENTS     = 4;

enum class SoundEffect : uint8_t
{
    Shoot,
    Gem,
    Hit,
    Down,
    Win,
};

struct SoundVoice
{
    bool     active;
    uint8_t  effect;        // SoundEffect
    uint8_t  segment;       // which part of the effect is playing
    int32_t  segment_pos;   // samples into it
    uint32_t phase;         // square wave: the top bit is the level
    uint16_t noise;         // noise: a 15-bit LFSR, stepped as phase wraps
    int16_t  pan;           // -2..2, by player: left to right
};

struct Sound
{
    SoundVoice voices[SOUND_VOICES];
    uint32_t   started;   // effects started so far: picks the voice to steal
};

// player: 0..3 (pans it a little), or -1 for the middle.
void sound_play(Sound* sound, SoundEffect effect, int32_t player);

// Mixes one video frame's audio: SOUND_FRAMES_PER_FRAME stereo frames into out.
void sound_render(Sound* sound, int16_t* out);
