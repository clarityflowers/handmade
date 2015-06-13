// sublime -- editor
// build -- compile
// devenv \build\win32_handmade.exe -- load VS
//  set working directory to P:\handmade\data

/* TODO
THIS IS NOT A FINAL PLATFORM LAYER!!!
- Saved game locations
- Getting a handle to our own .exe
- Asset loading path
- Threading
- Raw Input (support for multiple keyboards)
- Sleep/timeBeginPeriod
- ClipCursor() (multimonitor support)
- Fullscreen support
- WM_SETCURSOR (control cursor visibility)
- QueryCancelAutoplay
- WM_ACTIVATEAPP (for when we are not the active application)
- Blit speed improvements (BitBlt)
- Hardware acceleration (OpenGL or Direct3D or BOTH??)
- GetKeyboardLayout (for international keyboards)

Just a partial list of stuff!!
*/ 

// TODO Implement sine ourselves
#include <math.h>
#include <stdint.h>

#define Pi32 3.1415926539f
#define internal static
#define local_persist static
#define global_variable static

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

typedef int32 bool32;

typedef uint8_t uint8;
typedef uint16_t uint16;
typedef uint32_t uint32;
typedef uint64_t uint64;

typedef float real32;
typedef double real64;

#include "handmade.cpp"

#include <windows.h>
#include <stdio.h>
#include <malloc.h>
#include <xinput.h>
#include <dsound.h>

#include "win32_handmade.h"

// TODO This is global for now
global_variable bool32 GlobalRunning;
global_variable win32_offscreen_buffer GlobalBackbuffer;
global_variable LPDIRECTSOUNDBUFFER GlobalSecondaryBuffer;

// NOTE XInputGetState
#define X_INPUT_GET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_STATE *pState)
typedef X_INPUT_GET_STATE(x_input_get_state);
X_INPUT_GET_STATE(XInputGetStateStub) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_get_state *XInputGetState_ = XInputGetStateStub;
#define XInputGetState XInputGetState_

// NOTE XInputSetState
#define X_INPUT_SET_STATE(name) DWORD WINAPI name(DWORD dwUserIndex, XINPUT_VIBRATION *pVibration)
typedef X_INPUT_SET_STATE(x_input_set_state);
X_INPUT_SET_STATE(XInputSetStateStub) {
    return(ERROR_DEVICE_NOT_CONNECTED);
}
global_variable x_input_set_state *XInputSetState_ = XInputSetStateStub;
#define XInputSetState XInputSetState_

#define DIRECT_SOUND_CREATE(name) HRESULT WINAPI name(LPCGUID pcGuidDevice, LPDIRECTSOUND *ppDS, LPUNKNOWN pUnkOuter)
typedef DIRECT_SOUND_CREATE(direct_sound_create);



inline uint32
SafeTruncateUInt64(uint64 Value) {
    Assert(Value <= 0xFFFFFFFF);
    uint32 Result = (uint32)Value;
    return(Result);
}



internal debug_read_file_result
DEBUGPlatformReadEntireFile(char *Filename) {
    debug_read_file_result Result = {};

    HANDLE FileHandle = CreateFile(Filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);

    if (FileHandle != INVALID_HANDLE_VALUE) {
        LARGE_INTEGER FileSize;

        if(GetFileSizeEx(FileHandle, &FileSize)) {
            // TODO Defines for maximum values
            uint32 FileSize32 = SafeTruncateUInt64(FileSize.QuadPart);
            Result.Contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

            if(Result.Contents) {
                DWORD BytesRead;
                
                if(ReadFile(FileHandle, Result.Contents, FileSize32, &BytesRead, 0) && (FileSize32 == BytesRead)) {
                    // NOTE File read successfully
                    Result.ContentsSize = FileSize32;
                }
                else {
                    DEBUGPlatformFreeFileMemory(Result.Contents);
                    Result.Contents = 0;
                }
            }
            else {
                // TODO logging
            }
        }
        else {
            // TODO logging
        }

        CloseHandle(FileHandle);
    }
    else {
        // TODO logging
    }

    return(Result);
}



internal void 
DEBUGPlatformFreeFileMemory(void *Memory) {
    if(Memory) {
        VirtualFree(Memory, 0, MEM_RELEASE);
    }
}



internal bool32 
DEBUGPlatformWriteEntireFile(char *Filename, uint32 MemorySize, void *Memory) {
    bool32 Result = false;

    HANDLE FileHandle = CreateFile(Filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);

    if (FileHandle != INVALID_HANDLE_VALUE) {
        DWORD BytesWritten;
                
        if(WriteFile(FileHandle, Memory, MemorySize, &BytesWritten, 0)) {
            // NOTE File read successfully
            Result = (BytesWritten == MemorySize);
        }
        else {
            // TODO logging
        }

        CloseHandle(FileHandle);
    }
    else {
        // TODO logging
    }

    return(Result);


}



internal void
Wind32LoadXInput(void) {
    // TODO Test on windows 8
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
    if(!XInputLibrary)
    {
        // TODO(casey): Diagnostic
        XInputLibrary = LoadLibraryA("xinput9_1_0.dll");
    }
    if(!XInputLibrary) {
        // TODO Diagnostic
        XInputLibrary = LoadLibraryA("xinput1_3.dll");
    }
    if(XInputLibrary) {
        XInputGetState = (x_input_get_state *)GetProcAddress(XInputLibrary, "XInputGetState" );
        if(!XInputGetState) {XInputGetState = XInputGetStateStub;}
        XInputSetState = (x_input_set_state *)GetProcAddress(XInputLibrary, "XInputSetState" );
        if(!XInputSetState) {XInputSetState = XInputSetStateStub;}

        // TODO Diagnostic
    }
    else {
        // TODO Diagnostic
    }
}



internal void
Win32InitDSound(HWND Window, int32 SamplesPerSecond, int32 BufferSize) {
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if(DSoundLibrary) {
        direct_sound_create *DirectSoundCreate = (direct_sound_create *)GetProcAddress(DSoundLibrary, "DirectSoundCreate");
        // TODO double-check that this works on XP -- 7 or 8?
        LPDIRECTSOUND DirectSound;
        if(DirectSoundCreate && SUCCEEDED(DirectSoundCreate(0, &DirectSound, 0))) {
            WAVEFORMATEX WaveFormat = {};
            WaveFormat.wFormatTag = WAVE_FORMAT_PCM;
            WaveFormat.nChannels = 2;
            WaveFormat.nSamplesPerSec = SamplesPerSecond;
            WaveFormat.wBitsPerSample = 16;
            WaveFormat.nBlockAlign = (WaveFormat.nChannels*WaveFormat.wBitsPerSample) / 8;
            WaveFormat.nAvgBytesPerSec = WaveFormat.nSamplesPerSec*WaveFormat.nBlockAlign;
            WaveFormat.cbSize = 0;

            if(SUCCEEDED(DirectSound->SetCooperativeLevel(Window, DSSCL_PRIORITY))) {
                DSBUFFERDESC BufferDescription = {0};
                BufferDescription.dwSize = sizeof(BufferDescription);
                BufferDescription.dwFlags = DSBCAPS_PRIMARYBUFFER;
                // TODO DSBCAPS_GLOBALFOCUS?

                LPDIRECTSOUNDBUFFER PrimaryBuffer;

                if (SUCCEEDED(DirectSound->CreateSoundBuffer(&BufferDescription, &PrimaryBuffer, 0))) {
                    HRESULT Error = PrimaryBuffer->SetFormat(&WaveFormat);
                    if(SUCCEEDED(Error)) {
                        OutputDebugStringA("Primary buffer format was set.\n");
                    }
                    else {
                        // TODO Diagnostic
                    }
                }
                else {
                    // TODO Diagnostic
                }
            }
            else {
                // TODO Diagnostic
            }
            // TODO DSBCAPS_GETCURENTPOSITION2?
            DSBUFFERDESC BufferDescription = {0};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &GlobalSecondaryBuffer, 0);

            if(SUCCEEDED(Error)) {
                OutputDebugStringA("Secondary buffer created successfuly\n");
            }
        }
        else {
            // TODO Diagnositc
        }
    }
}



internal win32_window_dimension Win32GetWindowDimension(HWND Window) {
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}






internal void
Win32ResizeDIBSection(win32_offscreen_buffer *Buffer, int Width, int Height) {
    // TODO Bulletproof this
    // Maybe don't free first, free after, then free first if that fails

    if(Buffer->Memory) {
        VirtualFree(Buffer->Memory, 0, MEM_RELEASE);
    }

    Buffer->Width = Width;
    Buffer->Height = Height;
    Buffer->BytesPerPixel = 4;

    Buffer->Info.bmiHeader.biSize = sizeof(Buffer->Info.bmiHeader);
    Buffer->Info.bmiHeader.biWidth = Buffer->Width;
    Buffer->Info.bmiHeader.biHeight = -Buffer->Height;
    Buffer->Info.bmiHeader.biPlanes = 1;
    Buffer->Info.bmiHeader.biBitCount = 32;
    Buffer->Info.bmiHeader.biCompression = BI_RGB;

    // NOTE Thanks Chris Hecker for clarifying StretchDIBits/BitBlt
    int BitmapMemorySize = (Buffer->Width*Buffer->Height)*Buffer->BytesPerPixel;
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

    // TODO probably clear to black

    Buffer->Pitch = Width*Buffer->BytesPerPixel;
}



internal void 
Win32DisplayBufferInWindow(
    win32_offscreen_buffer *Buffer,
    HDC DeviceContext, 
    int WindowWidth, int WindowHeight 
) {
    // TODO aspect ration correction
    StretchDIBits(
        DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer->Width, Buffer->Height,
        Buffer->Memory,
        &Buffer->Info,
        DIB_RGB_COLORS, SRCCOPY
    );
}



internal LRESULT CALLBACK
Win32MainWindowCallback(
    HWND   Window,
    UINT   Message,
    WPARAM WParam,
    LPARAM LParam
) {
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_SIZE: {
        } break;

        case WM_CLOSE: {
            // TODO Handle this with a message to the user?
            GlobalRunning = false;
        } break;

        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY: {
            // TODO Handle this as an error - recreate window?
            GlobalRunning = false;
        } break;

        case WM_SYSKEYDOWN: 
        case WM_SYSKEYUP:
        case WM_KEYDOWN:
        case WM_KEYUP: {
            Assert(!"Keyboard input came in through a non-dispatch message!");
        } break;

        case WM_PAINT: {
            OutputDebugStringA("WM_PAINT\n");
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

        default: {
            Result = DefWindowProcA(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
} 


internal void
win32ClearBuffer(win32_sound_output *SoundOutput) {
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(
        0, SoundOutput->SecondaryBufferSize,
        &Region1, &Region1Size,
        &Region2, &Region2Size,
        0
    ))) {
        // TODO Assert that Region1Size/Region2Sizse is valid
        uint8 *DestSample = (uint8 *)Region1;
        for(DWORD ByteIndex = 0; ByteIndex < Region1Size; ++ByteIndex) {
            *DestSample++ = 0;
        }

        DestSample = (uint8 *)Region2;
        for(DWORD ByteIndex = 0; ByteIndex < Region2Size; ++ByteIndex) {
            *DestSample++ = 0;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}

internal void
win32FillSoundBuffer(win32_sound_output *SoundOutput, DWORD ByteToLock, DWORD BytesToWrite, game_sound_output_buffer *SourceBuffer) {
    VOID *Region1;
    DWORD Region1Size;
    VOID *Region2;
    DWORD Region2Size;
    if(SUCCEEDED(GlobalSecondaryBuffer->Lock(
        ByteToLock, BytesToWrite,
        &Region1, &Region1Size,
        &Region2, &Region2Size,
        0
    ))) {
        //TODO Assert that Region1Size/Region2Size is valid
        //TODO Collapse these to loops
        DWORD Region1SampleCount = Region1Size/SoundOutput->BytesPerSample;
        int16 *DestSample = (int16 *)Region1;
        int16 *SourceSample = SourceBuffer->Samples;
        for(DWORD SampleIndex = 0; SampleIndex < Region1SampleCount; ++SampleIndex) {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        DWORD Region2SampleCount = Region2Size/SoundOutput->BytesPerSample;
        DestSample = (int16 *)Region2;
        for(DWORD SampleIndex = 0; SampleIndex < Region2SampleCount; ++SampleIndex) {
            *DestSample++ = *SourceSample++;
            *DestSample++ = *SourceSample++;
            ++SoundOutput->RunningSampleIndex;
        }

        GlobalSecondaryBuffer->Unlock(Region1, Region1Size, Region2, Region2Size);
    }
}



internal void
Win32ProcessXInputDigitalButton(DWORD XInputButtonState, game_button_state *OldState, game_button_state *NewState, DWORD ButtonBit) {
    NewState->EndedDown = (XInputButtonState & ButtonBit) == ButtonBit;
    NewState->HalfTransitionCount = (OldState->EndedDown != NewState->EndedDown) ? 1 : 0;  
}



internal void
Win32ProcessKeyboardMessage(game_button_state *NewState, bool32 IsDown) {
    NewState->EndedDown = IsDown;
    ++NewState->HalfTransitionCount;  
}



internal void
Win32ProcessPendingMessage(game_controller_input *KeyboardController) {
    MSG Message;
    while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
        switch(Message.message) {
            case WM_QUIT: {
                GlobalRunning = false;
            } break;
            case WM_SYSKEYDOWN:
            case WM_SYSKEYUP:
            case WM_KEYDOWN:
            case WM_KEYUP: {
                uint32 VKCode = (uint32)Message.wParam;
                bool32 WasDown = ((Message.lParam & (1 << 30)) != 0);
                bool32 IsDown = ((Message.lParam & (1 << 31)) == 0);
                if(WasDown != IsDown) {
                    if(VKCode == 'W') {
                    }
                    else if(VKCode == 'A') {
                    }
                    else if(VKCode == 'S') {
                    }
                    else if(VKCode == 'D') {
                    }
                    else if(VKCode == 'Q') {
                        Win32ProcessKeyboardMessage(&KeyboardController->LeftShoulder, IsDown);
                    }
                    else if(VKCode == 'E') {
                        Win32ProcessKeyboardMessage(&KeyboardController->RightShoulder, IsDown);
                    }
                    else if(VKCode == VK_UP) {
                        Win32ProcessKeyboardMessage(&KeyboardController->Up, IsDown);
                    }
                    else if(VKCode == VK_DOWN) {
                        Win32ProcessKeyboardMessage(&KeyboardController->Down, IsDown);
                    }
                    else if(VKCode == VK_LEFT) {
                        Win32ProcessKeyboardMessage(&KeyboardController->Left, IsDown);
                    }
                    else if(VKCode == VK_RIGHT) {
                        Win32ProcessKeyboardMessage(&KeyboardController->Right, IsDown);
                    }
                    else if(VKCode == VK_ESCAPE) {
                        GlobalRunning = false;
                    }
                    else if(VKCode == VK_SPACE) {
                    }
                    if((Message.lParam & (1 << 29)) && VKCode == VK_F4) { //Alt+F4
                        GlobalRunning = false;
                    }
                }
            } break;

            default: {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            }
        }

        // TODO Should we poll this more frequently

    }
}



int CALLBACK
WinMain( 
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CmdLine,
    int ShowCode 
) {
    LARGE_INTEGER PerfCountFrequencyResult;
    QueryPerformanceFrequency(&PerfCountFrequencyResult);
    int64 PerfCountFrequency = PerfCountFrequencyResult.QuadPart;

    Wind32LoadXInput();

    WNDCLASSA WindowClass = {};

    Win32ResizeDIBSection(&GlobalBackbuffer, 1280, 720);
    
    WindowClass.style = CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = Win32MainWindowCallback;
    WindowClass.hInstance = Instance;
//    WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";


    if (RegisterClass(&WindowClass)) {
        HWND Window = CreateWindowExA(
            0,
            WindowClass.lpszClassName,
            "Handmade Hero",
            WS_OVERLAPPEDWINDOW|WS_VISIBLE,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            0,
            0,
            Instance,
            0
        );
        if(Window){
            HDC DeviceContext = GetDC(Window);

            // Sound Test
            win32_sound_output SoundOutput = {};
            SoundOutput.SamplesPerSecond = 48000;
            // SoundOutput.ToneHz = 256;
            // SoundOutput.ToneVolume = 3000;
            SoundOutput.RunningSampleIndex = 0;
            // SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
            SoundOutput.BytesPerSample = sizeof(int16)*2;
            SoundOutput.LatencySampleCount = SoundOutput.SamplesPerSecond / 15;
            SoundOutput.SecondaryBufferSize = SoundOutput.SamplesPerSecond*SoundOutput.BytesPerSample;
            Win32InitDSound(Window, SoundOutput.SamplesPerSecond, SoundOutput.SecondaryBufferSize);
            win32ClearBuffer(&SoundOutput);
            GlobalSecondaryBuffer->Play(0, 0, DSBPLAY_LOOPING);
            // bool32 SoundIsPlaying = true;

            GlobalRunning = true;

            // TODO Pool with bitmap VirtualAlloc
            int16 *Samples = (int16 *)VirtualAlloc(0, SoundOutput.SecondaryBufferSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);

#if HANDMADE_INTERNAL
            LPVOID BaseAddress = 0;
#else
            LPVOID BaseAddress = Terabytes(2);
#endif

            game_memory GameMemory = {};
            GameMemory.PermanentStorageSize = Megabytes(64);
            GameMemory.TransientStorageSize = Gigabytes(1);
            uint64 TotalSize = GameMemory.PermanentStorageSize + GameMemory.TransientStorageSize;
            GameMemory.PermanentStorage = VirtualAlloc(BaseAddress, (size_t)TotalSize, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            GameMemory.TransientStorage = ((uint8 *)GameMemory.PermanentStorage + GameMemory.PermanentStorageSize);
            


            if(Samples && GameMemory.PermanentStorage && GameMemory.TransientStorage) {


                game_input Input[2] = {};
                game_input *NewInput = &Input[0];
                game_input *OldInput = &Input[1];

                LARGE_INTEGER LastCounter;
                QueryPerformanceCounter(&LastCounter);
                int64 LastCycleCount = __rdtsc();

                // game loop begin
                while (GlobalRunning) {

                    game_controller_input *KeyboardController = &NewInput->Controllers[0];
                    // TODO Zeroing macro
                    // TODO We can't zero everything because the up/down state will be wrong
                    game_controller_input ZeroController = {};
                    *KeyboardController = ZeroController;
                    
                    Win32ProcessPendingMessage(KeyboardController);

                    

                    DWORD MaxControllerCount = XUSER_MAX_COUNT;
                    if(MaxControllerCount > ArrayCount(NewInput->Controllers)) {
                        MaxControllerCount = ArrayCount(NewInput->Controllers);
                    }
                    for(
                        DWORD ControllerIndex = 0;
                        ControllerIndex < XUSER_MAX_COUNT;
                        ++ControllerIndex
                    ) {
                        game_controller_input *OldController = &OldInput->Controllers[ControllerIndex];
                        game_controller_input *NewController = &NewInput->Controllers[ControllerIndex];


                        XINPUT_STATE ControllerState;
                        if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
                            // NOTE Controller is plugged in
                            // TODO See if ControllerState.dwPacketNumber increments
                            XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                            // TODO DPad
                            bool32 Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                            bool32 Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                            bool32 Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                            bool32 Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);

                            NewController->IsAnalogue = true;
                            real32 X;
                            if(Pad->sThumbLX < 0) {
                                X = (real32)Pad->sThumbLX / 32768.0f; 
                            }
                            else {
                                X = (real32)Pad->sThumbLX / 32767.0f;
                            }

                            real32 Y;
                            if(Pad->sThumbLX < 0) {
                                Y = (real32)Pad->sThumbLY / 32768.0f; 
                            }
                            else {
                                Y = (real32)Pad->sThumbLY / 32767.0f;
                            }

                            NewController->StartX = OldController->EndX;
                            NewController->StartY = OldController->EndY;

                            // TODO Deadzones XINPUT_GAMEPAD_LEFT_THUMB_DEADZONE XINPUT_GAMEPAD_RIGHT_THUMB_DEADZONE
                            // TODO Min/max macros
                            NewController->MinX = NewController->MaxX = NewController->EndX = X;
                            NewController->MinY = NewController->MaxY = NewController->EndY = Y;



                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Down, &NewController->Down, XINPUT_GAMEPAD_A);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Right, &NewController->Right, XINPUT_GAMEPAD_B);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Left, &NewController->Left, XINPUT_GAMEPAD_X);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->Up, &NewController->Up, XINPUT_GAMEPAD_Y);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->LeftShoulder, &NewController->LeftShoulder, XINPUT_GAMEPAD_LEFT_SHOULDER);
                            Win32ProcessXInputDigitalButton(Pad->wButtons, &OldController->RightShoulder, &NewController->RightShoulder, XINPUT_GAMEPAD_RIGHT_SHOULDER);

                            

                            // bool32 Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                            // bool32 Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);

                            // if (AButton) {
                            //     SoundOutput.ToneHz = 512 + (int32)256*(((real32)StickY / 30000.0f));
                            //     SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
                            // }
                            // else {
                            //     SoundOutput.ToneHz = 256;
                            //     SoundOutput.WavePeriod = SoundOutput.SamplesPerSecond/SoundOutput.ToneHz;
                            // }
                        }
                        else {
                            // NOTE Controller is not available
                        }
                    }

                    DWORD ByteToLock = 0;
                    DWORD TargetCursor = 0;
                    DWORD BytesToWrite = 0;
                    DWORD PlayCursor = 0;
                    DWORD WriteCursor = 0;
                    bool32 SoundIsValid = false;
                    // TODO Tighten up sound logic so that we know where we should be 
                    // writing to and can anticipate the time spent in the game update
                    if(SUCCEEDED(GlobalSecondaryBuffer->GetCurrentPosition(&PlayCursor, &WriteCursor))) {
                        ByteToLock = (SoundOutput.RunningSampleIndex*SoundOutput.BytesPerSample) % SoundOutput.SecondaryBufferSize;
                        TargetCursor = (PlayCursor + (SoundOutput.LatencySampleCount*SoundOutput.BytesPerSample)) % SoundOutput.SecondaryBufferSize;
                        if(ByteToLock == TargetCursor) {
                            BytesToWrite = 0;
                        }
                        if(ByteToLock > TargetCursor) {
                            BytesToWrite = (SoundOutput.SecondaryBufferSize - ByteToLock);
                            BytesToWrite += TargetCursor;
                        }
                        else {
                            BytesToWrite = TargetCursor - ByteToLock;
                        }

                        SoundIsValid = true;
                    }

                    
                    game_sound_output_buffer SoundBuffer = {};
                    SoundBuffer.SamplesPerSecond = SoundOutput.SamplesPerSecond;
                    SoundBuffer.SampleCount = BytesToWrite / SoundOutput.BytesPerSample;
                    SoundBuffer.Samples = Samples;

                    game_offscreen_buffer Buffer = {};
                    Buffer.Memory = GlobalBackbuffer.Memory;
                    Buffer.Width = GlobalBackbuffer.Width;
                    Buffer.Height = GlobalBackbuffer.Height;
                    Buffer.Pitch = GlobalBackbuffer.Pitch;
                    GameUpdateAndRender(&GameMemory, NewInput, &Buffer, &SoundBuffer);

                    // directsound output test
                    if(SoundIsValid) {
                        
                        win32FillSoundBuffer(&SoundOutput, ByteToLock, BytesToWrite, &SoundBuffer);

                        // TODO More strenuous test
                    }

                    win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                    Win32DisplayBufferInWindow(
                        &GlobalBackbuffer, DeviceContext,
                        Dimension.Width, Dimension.Height
                    );

                    int64 EndCycleCount = __rdtsc();

                    LARGE_INTEGER EndCounter;
                    QueryPerformanceCounter(&EndCounter);

                    uint64 CyclesElapsed = EndCycleCount - LastCycleCount;
                    int64 CounterElapsed = EndCounter.QuadPart - LastCounter.QuadPart;
                    real64 MSPerFrame = (((1000.0f*(real64)CounterElapsed) / (real64)PerfCountFrequency));
                    real64 FPS = (real64)PerfCountFrequency / (real64)CounterElapsed;
                    real64 MCPF = ((real64)CyclesElapsed / (1000.0f * 1000.0f));
                    

                    // char Buffer[256];
                    // sprintf(Buffer, "%.02fms/f,  %.02ff/s,  %.02fmc/f\n", MSPerFrame, FPS, MCPF);
                    // OutputDebugStringA(Buffer);

                    // TODO print it
                    LastCounter = EndCounter;
                    LastCycleCount = EndCycleCount;

                    game_input *Temp = NewInput;
                    NewInput = OldInput;
                    OldInput = Temp;
                    // TODO Clear?
                }
                //game loop end
            }
            else {
                // TODO Logging
            }
        }
        else {
            // TODO Logging
        }
    }
    else{
        // TODO Logging
    }
    return(0); 
}