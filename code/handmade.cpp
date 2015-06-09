#include "handmade.h"



internal void
GameOutputSound(game_sound_output_buffer *SoundBuffer, int ToneHz) {
    local_persist real32 tSine;
    int16 ToneVolume = 3000;
    // int ToneHz = 256;
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;
    
    int16 *SampleOut = SoundBuffer->Samples; 
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
        real32 SineValue = sinf(tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
    }
}



internal void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
    // TODO lets see which is better
    uint8 *Row = (uint8 *)Buffer->Memory;
    for( int Y = 0 ; Y < Buffer->Height ; ++Y ) {
        uint32 *Pixel = (uint32 *)Row;
        for( int X = 0 ; X < Buffer->Width ; ++X ) {
            uint8 Blue = (X - XOffset);
            uint8 Green = (Y + YOffset);
            uint8 Red = ((X - (XOffset/4))*(Y + (YOffset/4))/8);
            *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
        }
        Row += Buffer->Pitch;
    }
}





void
GameUpdateAndRender(game_input *Input, game_offscreen_buffer *Buffer, game_sound_output_buffer *SoundBuffer) {
    local_persist int XOffset = 0;
    local_persist int YOffset = 0;
    local_persist int ToneHz = 256;

    game_controller_input *Input0 = &Input->Controllers[0];
    if(Input0->IsAnalogue) {
        // Use analogue movement tuning
        XOffset += (int)(4.0f*(Input0->EndX));
        ToneHz = 256 + (int)(128.0f*(Input0->EndY));
    }
    else {
        // Use digital movement tuning
    }

    if(Input0->Down.EndedDown) {
        YOffset += 1;
    }



    // TODO Allow sample offsets here for more robust platform
    GameOutputSound(SoundBuffer, ToneHz);
    RenderWeirdGradient(Buffer, XOffset, YOffset);
}