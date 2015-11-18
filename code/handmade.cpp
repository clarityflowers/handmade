#include "handmade.h"



void
GameOutputSound(game_state *GameState, game_sound_output_buffer *SoundBuffer, int ToneHz) {
    // local_persist real32 tSine;
    int16 ToneVolume = 3000;
    // int ToneHz = 256;
    int WavePeriod = SoundBuffer->SamplesPerSecond/ToneHz;

    int16 *SampleOut = SoundBuffer->Samples;
    for(int SampleIndex = 0; SampleIndex < SoundBuffer->SampleCount; ++SampleIndex) {
        real32 SineValue = sinf(GameState->tSine);
        int16 SampleValue = (int16)(SineValue * ToneVolume);
        *SampleOut++ = SampleValue;
        *SampleOut++ = SampleValue;

        GameState->tSine += 2.0f*Pi32*1.0f/(real32)WavePeriod;
        if(GameState->tSine > 2.0f*Pi32) {
        	GameState->tSine -= 2.0f*Pi32;
        }
    }
}



// internal void
// RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
//     // TODO: lets see which is better
//     uint8 *Row = (uint8 *)Buffer->Memory;
//     for( int Y = 0 ; Y < Buffer->Height ; ++Y ) {
//         uint32 *Pixel = (uint32 *)Row;
//         for( int X = 0 ; X < Buffer->Width ; ++X ) {
//             uint8 Blue = (uint8)(X - XOffset);
//             uint8 Green = (uint8)(Y + YOffset);
//             uint8 Red = (uint8)((X - (XOffset/4))*(Y + (YOffset/4))/8);
//             *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
//         }
//         Row += Buffer->Pitch;
//     }
// }

void
RenderWeirdGradient(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
    // TODO: lets see which is better
    uint8 *Row = (uint8 *)Buffer->Memory;
    for( int Y = 0 ; Y < Buffer->Height ; ++Y ) {
        uint32 *Pixel = (uint32 *)Row;
        for( int X = 0 ; X < Buffer->Width ; ++X ) {
            uint8 Blue = 255;
            uint8 Red = 0; //(uint8)(X + XOffset)
            uint8 Green = (uint8)(Y + YOffset);
            *Pixel++ = ((Red << 16) | (Green << 8) | Blue);
        }
        Row += Buffer->Pitch;
    }
}



extern "C"
GAME_UPDATE_AND_RENDER(GameUpdateAndRender) {
    Assert((&Input->Controllers[0].Start - &Input->Controllers[0].Buttons[0]) == (ArrayCount(Input->Controllers[0].Buttons) - 1));
    Assert(sizeof(game_state) <= Memory->PermanentStorageSize);
    game_state *GameState = (game_state *)Memory->PermanentStorage;
    if (!Memory->IsInitialized) {
        char *Filename = __FILE__;

        debug_read_file_result File = Memory->DEBUGPlatformReadEntireFile(Filename);
        if(File.Contents) {
            Memory->DEBUGPlatformWriteEntireFile("test.out", File.ContentsSize, File.Contents);
            Memory->DEBUGPlatformFreeFileMemory(File.Contents);
        }

        // GameState->ToneHz = 256;

        // TODO: This may be more appropriate to do in the platform layer
        Memory->IsInitialized = true;
        // GameState->tSine = 0.0f;
    }

    GameState->ToneHz = 220;

    for( int ControllerIndex = 0 ; ControllerIndex < ArrayCount(Input->Controllers) ; ++ControllerIndex) {
        game_controller_input *Controller = GetController(Input, ControllerIndex);
        if(Controller->IsAnalogue) {
            // Use analogue movement tuning
            GameState->XOffset += (int)(4.0f*(Controller->StickAverageX));
            GameState->ToneHz += (int)((GameState->ToneHz/2.0f)*(Controller->StickAverageY));
        }
        else {
            if(Controller->MoveLeft.EndedDown) {
                GameState->XOffset -= 1;
            }
            else if(Controller->MoveRight.EndedDown) {
                GameState->XOffset += 1;
            }
            if(Controller->MoveUp.EndedDown) {
                GameState->ToneHz = (int)(GameState->ToneHz*1.5f);
            }
            else if(Controller->MoveDown.EndedDown) {
                GameState->ToneHz = (int)(GameState->ToneHz/2.0f);
            }
        }

        if(Controller->ActionDown.EndedDown) {
            GameState->YOffset += 1;
        }
    }

    RenderWeirdGradient(Buffer, GameState->XOffset, GameState->YOffset);
}

extern "C"
GAME_GET_SOUND_SAMPLES(GameGetSoundSamples) {
	  game_state *GameState = (game_state *)Memory->PermanentStorage;

    GameOutputSound(GameState, SoundBuffer, GameState->ToneHz);
}

#if HANDMADE_WIN32
#include "windows.h"
BOOL WINAPI DllMain(
  _In_ HINSTANCE hinstDLL, 
  _In_ DWORD fdwReason, 
  _In_ LPVOID lpvReserved) 
{
    return(TRUE);
}
#endif