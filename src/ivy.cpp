#include <windows.h>
#include <stdint.h>
#include <gdiplus.h>

#include "resource.h"

typedef int8_t s8;
typedef uint8_t u8;
typedef int16_t s16;
typedef uint16_t u16;
typedef int32_t s32;
typedef uint32_t u32;
typedef int64_t s64;
typedef uint64_t u64;
typedef float r32;
typedef int32_t b32;

using namespace Gdiplus;

CHAR szClassName[]  = "aa_ivy";
CHAR szAppName[]    = "Ivy";
Image *image;

static s32 mousex, mousey;

enum
{
    DragMode_None,
    DragMode_Drag,
    DragMode_Resize,
    DragMode_Rotate, // Combine resize and rotate at some point
} dragmode;

VOID OnPaint(HDC hdc)
{
    Graphics    graphics(hdc);
    /*
    SolidBrush  brush(Color(255, 0, 0, 255));
    FontFamily  fontFamily(L"Calibri");
    Font        font(&fontFamily, 24, FontStyleRegular, UnitPixel);
    PointF      pointF(0, 0);

    graphics.FillEllipse(&brush, 0, 0, 200, 200);
    graphics.DrawString(L"Hello World!", -1, &font, pointF, &brush);
    */
    graphics.DrawImage(image, Rect(0, 0, image->GetWidth(), image->GetHeight()));
}

LRESULT CALLBACK WndProc(HWND hWnd, u32 uMsg, WPARAM wParam, LPARAM lParam)
{
    HDC hdc;
    PAINTSTRUCT ps;

    switch (uMsg)
    {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            switch(wParam)
            {
                case VK_ESCAPE:
                {
                    PostQuitMessage(0);
                } break;
            }
        } break;
        case WM_LBUTTONDOWN:
        {
            dragmode = DragMode_Drag;
            SetCapture(hWnd);
            mousex = LOWORD(lParam);
            mousey = HIWORD(lParam);
        } break;
        case WM_MOUSEMOVE:
        {
            s32 x = LOWORD(lParam);
            s32 y = HIWORD(lParam);
            switch (dragmode)
            {
                case DragMode_None:
                {
                } break;
                case DragMode_Drag:
                {
                    RECT rect;
                    GetWindowRect(hWnd, &rect);
                    MoveWindow(hWnd, rect.left + x - mousex, rect.top + y - mousey,
                                     rect.right - rect.left, rect.bottom - rect.top, TRUE);
                    InvalidateRect(hWnd, NULL, TRUE);
                } break;
            }
        } break;
        case WM_LBUTTONUP:
        {
            dragmode = DragMode_None;
            ReleaseCapture();
        } break;
        case WM_PAINT:
        {
            OutputDebugString("Paint\n");
            hdc = BeginPaint(hWnd, &ps);
            OnPaint(hdc);
            EndPaint(hWnd, &ps);
        } break;
        case WM_DESTROY:
        {
            PostQuitMessage(0);
        } break;
        default:
        {
            return DefWindowProc(hWnd, uMsg, wParam, lParam);
        }
    }

    return 1;
}

s32 WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, s32 nCmdShow)
{
    //LPWSTR cmdline = GetCommandLineW();
    s32 argc;
    LPWSTR *argv = CommandLineToArgvW(GetCommandLineW(), &argc);

    ULONG_PTR gdiplus;
    GdiplusStartupInput gdiplusStartupInput;
    GdiplusStartup(&gdiplus, &gdiplusStartupInput, NULL);

    WNDCLASSEX wcex = {};

    wcex.cbSize         = sizeof(WNDCLASSEX);
    wcex.style          = CS_OWNDC | CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor        = LoadCursor(hInstance, IDC_ARROW);
    wcex.lpszClassName  = szClassName;

    if (!RegisterClassEx(&wcex))
        return FALSE;

    image = new Image(L"E:/Pictures/Paintings & Sketches/Peng.png");
    u32 w = image->GetWidth();
    u32 h = image->GetHeight();

    HWND hWnd;
    hWnd = CreateWindow(szClassName, szAppName,
                        WS_POPUP, //WS_POPUP WS_OVERLAPPEDWINDOW
                        CW_USEDEFAULT, CW_USEDEFAULT, w, h, // x, y, w, h
                        NULL, NULL, hInstance, NULL);

    if (!hWnd)
        return FALSE;

    HMONITOR mon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO moninfo = { sizeof(moninfo) };
    GetMonitorInfo(mon, &moninfo);

    s32 cx = moninfo.rcMonitor.right - moninfo.rcMonitor.left;
    s32 cy = moninfo.rcMonitor.bottom - moninfo.rcMonitor.top;

    SetWindowPos(hWnd, NULL, (cx / 2) - (w / 2), (cy / 2) - (h / 2), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
    ShowWindow(hWnd, SW_SHOW);
    UpdateWindow(hWnd);
    InvalidateRect(hWnd, NULL, FALSE);

    MSG msg;
    HACCEL hAccelTable = LoadAccelerators(hInstance, "IVY");
    while (GetMessage(&msg, NULL, 0, 0))
    {
        if (!TranslateAccelerator(msg.hwnd, hAccelTable, &msg))
        {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }

    GdiplusShutdown(gdiplus);

    return (s32)msg.wParam;
}

