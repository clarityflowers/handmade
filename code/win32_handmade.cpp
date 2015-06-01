// sublime -- editor
// build -- compile
// devenv \build\win32_handmade.exe -- load VS
//  set working directory to P:\handmade\data

#include <windows.h>
#include <stdint.h>
#include <xinput.h>

#define internal static
#define local_persist static 
#define global_variable static 

typedef int8_t int8;
typedef int16_t int16;
typedef int32_t int32;
typedef int64_t int64;

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

win32_window_dimension Win32GetWindowDimension(HWND Window) {
    win32_window_dimension Result;

    RECT ClientRect;
    GetClientRect(Window, &ClientRect);
    Result.Width = ClientRect.right - ClientRect.left;
    Result.Height = ClientRect.bottom - ClientRect.top;

    return(Result);
}

internal void
RenderWeirdGradient(
    win32_offscreen_buffer Buffer, int BlueOffset, int GreenOffset
) {
    // TODO lets see which is better
    uint8 *Row = (uint8 *)Buffer.Memory;
    for( int Y = 0 ; Y < Buffer.Height ; ++Y ) {
        uint32 *Pixel = (uint32 *)Row;
        for( int X = 0 ; X < Buffer.Width ; ++X ) {
            uint8 Blue = 0; //(X + BlueOffset);
            uint8 Green = 0; //(Y + GreenOffset);
			uint8 Red = ((X - (GreenOffset/2))*(Y + (BlueOffset/4))/8);
			*Pixel++ = ((Red << 16) | (Green << 8) | Blue);
		}

        Row += Buffer.Pitch;
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
    Buffer->Memory = VirtualAlloc(0, BitmapMemorySize, MEM_COMMIT, PAGE_READWRITE);

    // TODO probably clear to black

    Buffer->Pitch = Width*Buffer->BytesPerPixel;
}

internal void 
Win32DisplayBufferInWindow(
    HDC DeviceContext, 
    int WindowWidth, int WindowHeight, 
    win32_offscreen_buffer Buffer,
    int X, int Y, int Width, int Height
) {
    // TODO aspect ration correction
    StretchDIBits(
        DeviceContext,
        0, 0, WindowWidth, WindowHeight,
        0, 0, Buffer.Width, Buffer.Height,
        Buffer.Memory,
        &Buffer.Info,
        DIB_RGB_COLORS, SRCCOPY
    );
}

LRESULT CALLBACK
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

        case WM_PAINT: {
            OutputDebugStringA("WM_PAINT\n");
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;


            win32_window_dimension Dimension = Win32GetWindowDimension(Window);
            Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer, X, Y, Width, Height);
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
    WNDCLASS WindowClass = {};

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
            GlobalRunning = true;
            int BlueOffset = 0;
            int GreenOffset = 0;
            while (GlobalRunning) {
                MSG Message;
                while (PeekMessage(&Message, 0, 0, 0, PM_REMOVE)) {
                    if(Message.message == WM_QUIT) {
                        GlobalRunning = false;
                    }

                    // TODO Should we poll this more frequently?
                    for(
                        DWORD ControllerIndex = 0;
                        ControllerIndex < XUSER_MAX_COUNT;
                        ++ControllerIndex
                    ) {
                        XINPUT_STATE ControllerState;
                        if(XInputGetState(ControllerIndex1, &ControllerState) == ERROR_SUCCESS) {
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
                            int16 Sticky = Pad->sThumbLY;
                        }
                        else {
                            // NOTE Controller is not available
                        }
                    }

                    TranslateMessage(&Message);
                    DispatchMessageA(&Message);
                }

                RenderWeirdGradient(GlobalBackbuffer, BlueOffset, GreenOffset);
                HDC DeviceContext = GetDC(Window);
                win32_window_dimension Dimension = Win32GetWindowDimension(Window);
                Win32DisplayBufferInWindow(DeviceContext, Dimension.Width, Dimension.Height, GlobalBackbuffer, 0, 0, Dimension.Width, Dimension.Height);
                ReleaseDC(Window, DeviceContext);

                ++BlueOffset;
				++GreenOffset;
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