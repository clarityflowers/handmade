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

// FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to u e
void GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset);

#define HANDMADE_H
#endif