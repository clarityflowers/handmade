#if !defined(HANDMADE_H)

#if HANDMADE_SLOW
#define Assert(Expression) \
    if(!(Expression)) {*(int *)0 = 0;}
#else
#define Assert(Expression)
#endif

#define Kilobytes(Value) (Value*1024)
#define Megabytes(Value) (Kilobytes(Value)*1024)
#define Gigabytes(Value) (Megabytes(Value)*1024)
#define ArrayCount(Array) (sizeof(Array) / sizeof((Array)[0]))

// TODO swap, min, max ... macros?

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

struct game_button_state {
    int HalfTransitionCount;
    bool32 EndedDown;
};

struct game_controller_input {
    bool32 IsAnalogue;

    real32 StartX;
    real32 StartY;

    real32 MinX;
    real32 MinY;

    real32 MaxX;
    real32 MaxY;

    real32 EndX;
    real32 EndY;
    
    union {
        game_button_state Buttons[6];
        struct {
            game_button_state Up;
            game_button_state Down;
            game_button_state Left;
            game_button_state Right;
            game_button_state LeftShoulder;
            game_button_state RightShoulder;
        };
    };
};

struct game_input {
    game_controller_input Controllers[4];
};

struct game_memory {
    bool32 IsInitialized;

    uint64 PermanentStorageSize;
    void *PermanentStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup

    uint64 TransientStorageSize;
    void *TransientStorage; // NOTE(casey): REQUIRED to be cleared to zero at startup
};

// FOUR THINGS - timing, controller/keyboard input, bitmap buffer to use, sound buffer to use
internal void GameUpdateAndRender(
    game_memory *Memory, 
    game_input *Input, 
    game_offscreen_buffer *Buffer, 
    game_sound_output_buffer *SoundBuffer
);

//
//
//


struct game_state {
    int ToneHz;
    int XOffset;
    int YOffset;
};



#define HANDMADE_H
#endif