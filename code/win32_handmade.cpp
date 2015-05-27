#include <windows.h>

#define internal static
#define local_persist static 
#define global_variable static 

global_variable bool Running; // TODO A Global for now
// static variables are initialized to 0 for some reason

LRESULT CALLBACK MainWindowCallback(
    HWND   Window,
    UINT   Message,
    WPARAM WParam,
    LPARAM LParam
) {
    LRESULT Result = 0;

    switch(Message)
    {
        case WM_SIZE: {
            OutputDebugStringA("WM_SIZE\n");
        } break;

        case WM_CLOSE: {
            // TODO Handle this with a message to the user?
            Running = false;
        } break;

        case WM_ACTIVATEAPP: {
            OutputDebugStringA("WM_ACTIVATEAPP\n");
        } break;

        case WM_DESTROY: {
            // TODO Handle this as an error - recreate window?
            Running = false;
        } break;

        case WM_PAINT: {
            PAINTSTRUCT Paint;
            HDC DeviceContext = BeginPaint(Window, &Paint);
            int X = Paint.rcPaint.left;
            int Y = Paint.rcPaint.top;
            int Height = Paint.rcPaint.bottom - Paint.rcPaint.top;
            int Width = Paint.rcPaint.right - Paint.rcPaint.left;
            local_persist DWORD Operation = WHITENESS;
            if (Operation == WHITENESS) {
                Operation = BLACKNESS;
            }
            else {
                Operation = WHITENESS;
            }
            PatBlt(DeviceContext, X, Y, Width, Height, Operation);
            EndPaint(Window, &Paint);
        } break;

        default: {
            // OutputDebugStringA("default\n");
            Result = DefWindowProc(Window, Message, WParam, LParam);
        } break;
    }

    return(Result);
}

int CALLBACK WinMain( 
    HINSTANCE Instance,
    HINSTANCE PrevInstance,
    LPSTR CmdLine,
    int ShowCode 
) {
    WNDCLASS WindowClass = {};
    WindowClass.style = CS_OWNDC|CS_HREDRAW|CS_VREDRAW;
    WindowClass.lpfnWndProc = MainWindowCallback;
    WindowClass.hInstance = Instance;
    // WindowClass.hIcon;
    WindowClass.lpszClassName = "HandmadeHeroWindowClass";

    if (RegisterClass(&WindowClass)) {
        HWND WindowHandle = CreateWindowEx(
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
        if(WindowHandle){
            MSG Message;
            Running = true;
            while (Running) {
                BOOL MessageResult = GetMessage(&Message, 0, 0, 0);
                if (MessageResult > 0) {
                    TranslateMessage(&Message);
                    DispatchMessage(&Message);
                }
                else {
                    break;
                }
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