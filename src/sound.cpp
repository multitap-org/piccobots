#include "sound.h"

// An effect: up to four segments played one after the other, each a frequency
// sweep (from -> to, Hz) of square wave or noise.
struct SoundSegment
{
    int32_t from_hz;
    int32_t to_hz;
    int32_t milliseconds;   // 0 = the effect ends here
    bool    noise;
    int32_t volume;         // peak, of 32767
};

struct SoundEffectInfo
{
    SoundSegment segments[SOUND_MAX_SEGMENTS];
};

// Indexed by SoundEffect.
static const SoundEffectInfo EFFECTS[] = {
    // Shoot: a short falling blip.
    { { { 1400, 500, 60, false, 1800 } } },
    // Gem: two notes up (B5, E6).
    { { { 988, 988, 45, false, 2200 }, { 1319, 1319, 90, false, 2200 } } },
    // Hit: a burst of noise.
    { { { 2600, 700, 130, true, 2600 } } },
    // Down: a long fall.
    { { { 700, 110, 380, false, 2400 } } },
    // Win: C5 E5 G5 C6.
    { { { 523, 523, 140, false, 2400 }, { 659, 659, 140, false, 2400 },
        { 784, 784, 140, false, 2400 }, { 1047, 1047, 420, false, 2400 } } },
};

static int32_t sound_segment_samples(const SoundSegment* segment)
{
    return segment->milliseconds * SOUND_RATE / 1000;
}

void sound_play(Sound* sound, SoundEffect effect, int32_t player)
{
    // A free voice, or else the one started longest ago.
    int32_t chosen = static_cast<int32_t>(sound->started % SOUND_VOICES);
    for (int32_t i = 0; i < SOUND_VOICES; i++)
    {
        if (!sound->voices[i].active)
        {
            chosen = i;
            break;
        }
    }
    sound->started++;

    SoundVoice* voice = &sound->voices[chosen];
    voice->active      = true;
    voice->effect      = static_cast<uint8_t>(effect);
    voice->segment     = 0;
    voice->segment_pos = 0;
    voice->phase       = 0;
    voice->noise       = 1;
    // Players 1..4 from left to right: -2, -1, 1, 2 (0 = the middle).
    if (player < 0)
    {
        voice->pan = 0;
    }
    else
    {
        voice->pan = static_cast<int16_t>(player < 2 ? player - 2 : player - 1);
    }
}

static int16_t sound_clamp(int32_t sample)
{
    if (sample > 32767)
    {
        return 32767;
    }
    if (sample < -32768)
    {
        return -32768;
    }
    return static_cast<int16_t>(sample);
}

void sound_render(Sound* sound, int16_t* out)
{
    for (int32_t frame = 0; frame < SOUND_FRAMES_PER_FRAME; frame++)
    {
        int32_t left = 0;
        int32_t right = 0;
        for (int32_t v = 0; v < SOUND_VOICES; v++)
        {
            SoundVoice* voice = &sound->voices[v];
            if (!voice->active)
            {
                continue;
            }
            const SoundSegment* segment = &EFFECTS[voice->effect].segments[voice->segment];
            int32_t length = sound_segment_samples(segment);

            // Frequency along the sweep, then the phase step for it (Hz * 2^32 / rate).
            int64_t hz = segment->from_hz +
                         static_cast<int64_t>(segment->to_hz - segment->from_hz) * voice->segment_pos / length;
            uint32_t step = static_cast<uint32_t>((hz << 32) / SOUND_RATE);
            uint32_t before = voice->phase;
            voice->phase += step;

            int32_t level = 0;
            if (segment->noise)
            {
                if (voice->phase < before)
                {
                    // Wrapped: the next noise bit (taps 0 and 1, as many old sound chips).
                    uint16_t bit = static_cast<uint16_t>((voice->noise ^ (voice->noise >> 1)) & 1u);
                    voice->noise = static_cast<uint16_t>((voice->noise >> 1) | (bit << 14));
                }
                level = (voice->noise & 1u) != 0 ? segment->volume : -segment->volume;
            }
            else
            {
                level = (voice->phase & 0x80000000u) != 0 ? segment->volume : -segment->volume;
            }
            // A linear fade over the last quarter of the effect's last segment keeps it from clicking.
            bool last = voice->segment + 1 >= SOUND_MAX_SEGMENTS ||
                        EFFECTS[voice->effect].segments[voice->segment + 1].milliseconds == 0;
            int32_t remaining = length - voice->segment_pos;
            if (last && remaining < length / 4)
            {
                level = level * remaining / (length / 4);
            }
            // Panning: each step moves a quarter of the level from one side to the other.
            left += level * (4 - voice->pan) / 4;
            right += level * (4 + voice->pan) / 4;

            voice->segment_pos++;
            if (voice->segment_pos >= length)
            {
                voice->segment_pos = 0;
                voice->segment++;
                if (last)
                {
                    voice->active = false;
                }
            }
        }
        out[frame * 2] = sound_clamp(left);
        out[frame * 2 + 1] = sound_clamp(right);
    }
}
