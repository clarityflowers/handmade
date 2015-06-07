#if !defined(HANDMADE_H)

void PlatformLoadFile(char *FileName);

// TODO Services that the platform layer provides to the game

/* NOTE
Services that the game provides to the platform layer.
This may expand in the future - sound of separate thread, ect.
*/

// TODO In the future, rendering _specifically_ will become a three-tiered abstraction
struct game_offscreen_buffer {
    void *Memory;
    int Width;
    int Height;
    int Pitch;
};

struct game_sound_output_buffer {
    int SamplesPerSecond;
    int SampleCount;
    int16 *Samples;
};

// FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
internal void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset, game_sound_output_buffer *SoundBuffer, int ToneHz);

#define HANDMADE_H
#endif