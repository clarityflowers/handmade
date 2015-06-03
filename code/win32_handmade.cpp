// sublime -- editor
// build -- compile
// devenv \build\win32_handmade.exe -- load VS
//  set working directory to P:\handmade\data

#include <windows.h>
#include <stdint.h>
#include <xinput.h>
#include <dsound.h>

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



struct win32_offscreen_buffer {
    BITMAPINFO Info;
    void *Memory;
    int Width;
    int Height;
    int Pitch;
    int BytesPerPixel;
};

global_variable bool GlobalRunning; // TODO A Global for now
global_variable win32_offscreen_buffer GlobalBackbuffer;

struct win32_window_dimension {
    int Width;
    int Height;
};

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



internal void
Wind32LoadXInput(void) {
    // TODO Test on windows 8
    HMODULE XInputLibrary = LoadLibraryA("xinput1_4.dll");
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
    // load the library
    HMODULE DSoundLibrary = LoadLibraryA("dsound.dll");

    if(DSoundLibrary) {
        // get a DirectSound object
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
            // create a secondary buffer
            // TODO DSBCAPS_GETCURENTPOSITION2?
            DSBUFFERDESC BufferDescription = {0};
            BufferDescription.dwSize = sizeof(BufferDescription);
            BufferDescription.dwFlags = 0;
            BufferDescription.dwBufferBytes = BufferSize;
            BufferDescription.lpwfxFormat = &WaveFormat;
            LPDIRECTSOUNDBUFFER SecondaryBuffer;
            HRESULT Error = DirectSound->CreateSoundBuffer(&BufferDescription, &SecondaryBuffer, 0);

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
RenderWeirdGradient(
    win32_offscreen_buffer *Buffer, int XOffset, int YOffset
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

    // NOTE: Thanks Chris Hecker for clarifying StretchDIBits/BitBlt
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
            uint32 VKCode = WParam;
            bool WasDown = ((LParam & (1 << 30)) != 0);
            bool IsDown = ((LParam & (1 << 31)) == 0);
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
                }
                else if(VKCode == 'E') {
                }
                else if(VKCode == VK_UP) {
                }
                else if(VKCode == VK_DOWN) {
                }
                else if(VKCode == VK_LEFT) {
                }
                else if(VKCode == VK_RIGHT) {
                }
                else if(VKCode == VK_ESCAPE) {
                    GlobalRunning = false;
                }
                else if(VKCode == VK_SPACE) {
                    OutputDebugStringA("Space ");
                    if(IsDown) {
                        OutputDebugStringA("IsDown ");
                    }
                    if(WasDown) {
                        OutputDebugStringA("WasDown ");
                    }
                    OutputDebugStringA("\n");
                }
            }

            if((LParam & (1 << 29)) && VKCode == VK_F4) { //Alt+F4
                GlobalRunning = false;
            }

        } break;

        case WM_PAINT: {
            OutputDebugStringA("WM_PAINT\n");
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;


            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(&GlobalBackbuffer, DeviceContext, Dimension.Width, Dimension.Height);
            EndPaint(Window, &Paint);
        } break;

        default: {
            // OutputDebugStringA("default\n");
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
} 



int CALLBACK
WinMain( 
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CmdLine,
    int ShowCode 
) {

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
            int XOffset = 0;
            int YOffset = 0;
            Win32InitDSound(Window, 48000, 48000*sizeof(int16)*2);

            
            GlobalRunning = true;
            while (GlobalRunning) {
                MSG Message;
                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
                    if(Message.message == WM_QUIT) {
                        GlobalRunning = false;
                    }

                    // TODO Should we poll this more frequently

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                for(
                    DWORD ControllerIndex = 0;
                    ControllerIndex < XUSER_MAX_COUNT;
                    ++ControllerIndex
                ) {
                    XINPUT_STATE ControllerState;
                    if(XInputGetState(ControllerIndex, &ControllerState) == ERROR_SUCCESS) {
                        // NOTE Controller is plugged in
                        // TODO See if ControllerState.dwPacketNumber increments
                        XINPUT_GAMEPAD *Pad = &ControllerState.Gamepad;

                        bool Up = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_UP);
                        bool Down = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_DOWN);
                        bool Left = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_LEFT);
                        bool Right = (Pad->wButtons & XINPUT_GAMEPAD_DPAD_RIGHT);
                        bool Start = (Pad->wButtons & XINPUT_GAMEPAD_START);
                        bool Back = (Pad->wButtons & XINPUT_GAMEPAD_BACK);
                        bool LeftShoulder = (Pad->wButtons & XINPUT_GAMEPAD_LEFT_SHOULDER);
                        bool RightShoulder = (Pad->wButtons & XINPUT_GAMEPAD_RIGHT_SHOULDER);
                        bool AButton = (Pad->wButtons & XINPUT_GAMEPAD_A);
                        bool BButton = (Pad->wButtons & XINPUT_GAMEPAD_B);
                        bool XButton = (Pad->wButtons & XINPUT_GAMEPAD_X);
                        bool YButton = (Pad->wButtons & XINPUT_GAMEPAD_Y);
                        
                        int16 StickX = Pad->sThumbLX;
                        int16 StickY = Pad->sThumbLY;

                        XOffset += StickX >> 12;
                        YOffset += StickY >> 12;
                    }
                    else {
                        // NOTE Controller is not available
                    }
                }

                // XINPUT_VIBRATION Vibration;
                // Vibration.wLeftMotorSpeed = 60000;
                // Vibration.wRightMotorSpeed = 60000;
                // XInputSetState(0, &Vibration);

                RenderWeirdGradient(&GlobalBackbuffer, XOffset, YOffset);


                
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(
                    &GlobalBackbuffer, DeviceContext,
                    Dimension.Width, Dimension.Height
                );
                // ++BlueOffset;
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