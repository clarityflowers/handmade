#include "handmade.h"

internal void
RenderWeirdGradient(
    game_offscreen_buffer *Buffer, int XOffset, int YOffset
) {
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
GameUpdateAndRender(game_offscreen_buffer *Buffer, int XOffset, int YOffset) {
    RenderWeirdGradient(Buffer, XOffset, YOffset);
}