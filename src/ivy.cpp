#include <windows.h>
#include <stdint.h>
#include <gdiplus.h>
#include <strsafe.h>
#include <math.h>

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

POINT curpos, curpos_rel;
b32 border = false;
b32 handles = false;
b32 topmost = false;
r32 scalex = 1.0f;
r32 scaley = 1.0f;
r32 angle = 0.0f;

#define M_PI 3.14159265358979323846f

HCURSOR cursor_arrow, cursor_sizeall;

enum
{
    DragMode_None,
    DragMode_Drag,
    DragMode_Resize,
    DragMode_Rotate, // Combine resize and rotate at some point
} dragmode;

enum AdvanceDirection
{
    Advance_Forwards = 1,
    Advance_Backwards = 0,
};

struct imgfile
{
    WCHAR *filename;
    Bitmap *image;
    imgfile *next;
    imgfile *prev;
} *imglist;
imgfile *curimg;
u32 imgcount = 0;

static void CheckError(LPTSTR lpszFunction)
{
    DWORD dw = GetLastError();

    LPVOID lpMsgBuf;
    LPVOID lpDisplayBuf;
    FormatMessage(FORMAT_MESSAGE_ALLOCATE_BUFFER |
                  FORMAT_MESSAGE_FROM_SYSTEM |
                  FORMAT_MESSAGE_IGNORE_INSERTS,
                  NULL, dw,
                  MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
                  (LPTSTR) &lpMsgBuf,
                  0, NULL);
    lpDisplayBuf = (LPVOID)LocalAlloc(LMEM_ZEROINIT,
        (lstrlen((LPCTSTR)lpMsgBuf) + lstrlen((LPCTSTR)lpszFunction) + 40) * sizeof(TCHAR));
    StringCchPrintf((LPTSTR)lpDisplayBuf,
            LocalSize(lpDisplayBuf) / sizeof(TCHAR),
            TEXT("%s returned error %d: %s"),
            lpszFunction, dw, lpMsgBuf);
    //MessageBox(NULL, (LPCTSTR)lpDisplayBuf, TEXT("Error"), MB_OK);
    OutputDebugString((LPCTSTR)lpDisplayBuf);
    LocalFree(lpMsgBuf);
    LocalFree(lpDisplayBuf);
}

static void Draw(HWND hWnd, s32 x, s32 y, u32 w, u32 h, BYTE alpha = 255)
{
    w = (u32)((r32)w * fabs(scalex));
    h = (u32)((r32)h * fabs(scaley));

    HDC hdcScreen = GetDC(NULL);
    HDC hdc = CreateCompatibleDC(hdcScreen);
    //HBITMAP buffer;
    //Status err = curimg->image->GetHBITMAP(Color(0, 0, 0, 0), &buffer);
    //HBITMAP buffer = CreateCompatibleBitmap(hdc, w, h);
    //HBITMAP oldBmp = (HBITMAP)SelectObject(hdc, buffer);
    Bitmap bmp(w, h, PixelFormat32bppARGB);

    //Graphics g(hdc);
    Graphics *g = Graphics::FromImage(&bmp);
    g->Clear(Color(0, 0, 0, 0));
    g->TranslateTransform((r32)w * 0.5f, (r32)h * 0.5f);
    g->RotateTransform(angle);
    g->TranslateTransform((r32)w * -0.5f, (r32)h * -0.5f);

    Rect r = {0, 0, (s32)w, (s32)h};
    if (scalex < 0.0f)
    {
        r.X = w - 1;
        r.Width = -(s32)w;
    }
    if (scaley < 0.0f)
    {
        r.Y = h - 1;
        r.Height = -(s32)h;
    }

    //g->DrawImage(curimg->image, Rect(0, 0, w, h));
    g->DrawImage(curimg->image, r);
    if (border || handles)
    {
        Pen pen(Color(0, 0, 0), 1);
        g->DrawRectangle(&pen, 0, 0, w - 1, h - 1);
        pen.SetColor(Color(255, 255, 255));
        g->DrawRectangle(&pen, 1, 1, w - 3, h - 3);
    }

    if (handles)
    {
        Pen pen(Color(0, 0, 0), 1);
        g->DrawRectangle(&pen,     0,     0, 7, 7);
        g->DrawRectangle(&pen, w - 8,     0, 7, 7);
        g->DrawRectangle(&pen,     0, h - 8, 7, 7);
        g->DrawRectangle(&pen, w - 8, h - 8, 7, 7);

        pen.SetColor(Color(255, 255, 255));
        g->DrawRectangle(&pen,     1,     1, 5, 5);
        g->DrawRectangle(&pen, w - 7,     1, 5, 5);
        g->DrawRectangle(&pen,     1, h - 7, 5, 5);
        g->DrawRectangle(&pen, w - 7, h - 7, 5, 5);

        SolidBrush brush(Color(127, 0, 0, 0));
        g->FillRectangle(&brush,     2,     2, 5, 5);
        g->FillRectangle(&brush, w - 7,     2, 5, 5);
        g->FillRectangle(&brush,     2, h - 7, 5, 5);
        g->FillRectangle(&brush, w - 7, h - 7, 5, 5);
    }

    HBITMAP buffer;
    bmp.GetHBITMAP(Color(0, 0, 0, 0), &buffer);
    HBITMAP oldBmp = (HBITMAP)SelectObject(hdc, buffer);

    POINT pt = {x, y};
    SIZE sz = {w, h};
    POINT ptZero = {0};

    BLENDFUNCTION blend = {AC_SRC_OVER, 0, alpha, AC_SRC_ALPHA};
    UpdateLayeredWindow(hWnd, hdcScreen, &pt, &sz, hdc, &ptZero, RGB(0, 0, 0), &blend, ULW_ALPHA);
    //CheckError(TEXT("UpdateLayeredWindow"));

    delete g;
    SelectObject(hdc, oldBmp);
    DeleteObject(buffer);
    DeleteDC(hdc);
    ReleaseDC(NULL, hdcScreen);
}

static void Draw(HWND hWnd, BYTE alpha = 255)
{
    RECT rect;
    GetWindowRect(hWnd, &rect);

    s32 x = rect.left + ((rect.right - rect.left) / 2) - (curimg->image->GetWidth() / 2);
    s32 y = rect.top + ((rect.bottom - rect.top) / 2) - (curimg->image->GetHeight() / 2);

    Draw(hWnd, x, y, curimg->image->GetWidth(), curimg->image->GetHeight(), alpha);
}

static void FreeFileList()
{
    imgfile *f = imglist;
    imgfile *next = nullptr;
    while (f)
    {
        if (f->image)
            delete f->image;

        next = f->next;
        free(f);
        f = next;
    }
}

static b32 OpenFiles()
{
    OPENFILENAMEW ofn;
    WCHAR file[65535] = L"";

    ZeroMemory(&ofn, sizeof(ofn));

    ofn.lStructSize = sizeof(ofn);
    ofn.lpstrFilter = L"Image Files\0*.jpg;*.jpeg;*.png;*.bmp;*.tga;*.gif;*.ico;*.jng;*.tiff\0";
    ofn.lpstrFile = file;
    ofn.nMaxFile = 65534;
    ofn.Flags = OFN_EXPLORER | OFN_FILEMUSTEXIST | OFN_HIDEREADONLY | OFN_ALLOWMULTISELECT;

    if (!GetOpenFileNameW(&ofn))
        return false;

    WCHAR *ptr = ofn.lpstrFile;
    WCHAR *dir = ptr;
    ptr[ofn.nFileOffset - 1] = 0;
    ptr += ofn.nFileOffset;

    FreeFileList();

    imgcount = 0;
    imgfile *last;

    imgfile *f = (imgfile *)malloc(sizeof(imgfile));
    f->filename = _wcsdup(ptr);
    f->image = nullptr;
    f->next = nullptr;
    f->prev = nullptr;
    imglist = last = f;
    ptr += lstrlenW(ptr) + 1;
    while (*ptr)
    {
        f = (imgfile *)malloc(sizeof(imgfile));
        f->filename = _wcsdup(ptr);
        f->image = nullptr;
        f->next = nullptr;
        f->prev = last;
        last->next = f;

        last = f;
        ++imgcount;
        ptr += lstrlenW(ptr) + 1;
    }

    // NOTE: make the list circular
    f->next = imglist;
    imglist->prev = f;

    u32 len = lstrlenW(dir) + lstrlenW(imglist->filename) + 2;
    WCHAR *fullpath = wcscat(dir, L"\\");
    fullpath = wcscat(fullpath, imglist->filename);
    imglist->image = new Bitmap(fullpath);
    curimg = imglist;

#if 0
    IFileOpenDialog *fod;

    HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL,
                                  CLSCTX_INPROC_SERVER,
                                  IID_PPV_ARGS(&fod));
    if (SUCCEEDED(hr))
    {
        DWORD opt;
        hr = fod->GetOptions(&opt);

        if (SUCCEEDED(hr))
        {
            hr = fod->SetOptions(opt | FOS_ALLOWMULTISELECT);
        }

        if (SUCCEEDED(hr))
        {
            hr = fod->Show(NULL);

            if (SUCCEEDED(hr))
            {
                IShellItemArray *res;
                hr = fod->GetResults(&res);

                if (SUCCEEDED(hr))
                {
                    int a = 1;
                }
            }
        }
    }
#endif

    return true;
}

static void Advance(HWND hWnd, AdvanceDirection dir = Advance_Forwards)
{
    imgfile *img = dir ? curimg->next : curimg->prev;
    if (img)
    {
        imgfile *next = curimg->next;
        if (!img->image)
        {
            img->image = new Bitmap(img->filename);
            if (img->image)
            {
                curimg = img;
                Draw(hWnd);
            }
            else
            {
                // NOTE: Remove file if image couldn't be loaded
                if (img->next)
                {
                    curimg->next = img->next;
                    img->next->prev = curimg;
                }
                else
                {
                    curimg->next = nullptr;
                }
                free(img);
            }
        }
        else
        {
            curimg = img;
            Draw(hWnd);
        }
    }
}

static void GetCenteredCoords(HWND hWnd, s32 *x, s32 *y)
{
    RECT rect;
    GetWindowRect(hWnd, &rect);

    r32 sx = scalex < 0.0f ? -scalex : scalex;
    r32 sy = scaley < 0.0f ? -scaley : scaley;

    *x = rect.left + ((rect.right - rect.left) / 2) - (s32)((r32)curimg->image->GetWidth() * sx / 2.0f);
    *y = rect.top + ((rect.bottom - rect.top) / 2) - (s32)((r32)curimg->image->GetHeight() * sy / 2.0f);
}

#define KEY_SHIFT   (1 << 16)
#define KEY_CTRL    (1 << 16)
#define KEY_ALT     (1 << 16)

LRESULT CALLBACK WndProc(HWND hWnd, u32 uMsg, WPARAM wParam, LPARAM lParam)
{

    switch (uMsg)
    {
        case WM_KEYDOWN:
        case WM_SYSKEYDOWN:
        {
            s32 code = (GetKeyState(VK_SHIFT)   < 0 ? KEY_SHIFT : 0)
                     | (GetKeyState(VK_CONTROL) < 0 ? KEY_CTRL : 0)
                     | (GetKeyState(VK_MENU)    < 0 ? KEY_ALT : 0);
            code |= (s32)wParam;
            switch(code)
            {
                case 'B':
                {
                    border = !border;

                    RECT rect;
                    GetWindowRect(hWnd, &rect);

                    Draw(hWnd, rect.left, rect.top, rect.right - rect.left, rect.bottom - rect.top);
                } break;
                case 'O':
                {
                    OpenFiles();

                    /*
                    RECT rect;
                    GetWindowRect(hWnd, &rect);

                    s32 x = rect.left + ((rect.right - rect.left) / 2) - (curimg->image->GetWidth() / 2);
                    s32 y = rect.top + ((rect.bottom - rect.top) / 2) - (curimg->image->GetHeight() / 2);
                    */

                    s32 x, y;
                    GetCenteredCoords(hWnd, &x, &y);
                    Draw(hWnd, x, y, curimg->image->GetWidth(), curimg->image->GetHeight());
                } break;
                case 'R':
                {
                    angle = 0.0f;
                    scalex = 1.0f;
                    scaley = 1.0f;

                    HMONITOR mon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
                    MONITORINFO moninfo = { sizeof(moninfo) };
                    GetMonitorInfo(mon, &moninfo);

                    s32 cx = moninfo.rcMonitor.right - moninfo.rcMonitor.left;
                    s32 cy = moninfo.rcMonitor.bottom - moninfo.rcMonitor.top;

                    u32 w = curimg->image->GetWidth();
                    u32 h = curimg->image->GetHeight();
                    Draw(hWnd, (cx / 2) - (w / 2), (cy / 2) - (h / 2), w, h);
                } break;
                case 'D':
                case VK_RIGHT:
                case VK_XBUTTON1:
                {
                    Advance(hWnd, Advance_Forwards);
                } break;
                case 'A':
                case VK_LEFT:
                case VK_XBUTTON2:
                {
                    Advance(hWnd, Advance_Backwards);
                } break;
                case 'H':
                {
                    handles = !handles;
                    //dragmode = DragMode_Resize;
                    Draw(hWnd);
                } break;
                case VK_MENU | KEY_ALT:
                {
                    if (!handles)
                    {
                        handles = true;
                        //dragmode = DragMode_Resize;
                        Draw(hWnd);
                    }
                } break;
                case '1':
                {
                    Draw(hWnd, 25);
                } break;
                case '2':
                {
                    Draw(hWnd, 51);
                } break;
                case '3':
                {
                    Draw(hWnd, 76);
                } break;
                case '4':
                {
                    Draw(hWnd, 102);
                } break;
                case '5':
                {
                    Draw(hWnd, 127);
                } break;
                case '6':
                {
                    Draw(hWnd, 152);
                } break;
                case '7':
                {
                    Draw(hWnd, 178);
                } break;
                case '8':
                {
                    Draw(hWnd, 204);
                } break;
                case '9':
                {
                    Draw(hWnd, 230);
                } break;
                case '0':
                {
                    Draw(hWnd, 255);
                } break;
                case '[':
                {
                    OutputDebugString("test\n");
                } break;
                case VK_SPACE:
                {
                    topmost = !topmost;
                    SetWindowPos(hWnd, (topmost ? HWND_TOPMOST : HWND_NOTOPMOST), 0, 0, 0, 0, SWP_NOMOVE | SWP_NOSIZE);
                } break;
                case VK_ADD:
                {
                    scaley = scalex += 0.1f;
                    s32 x, y;
                    GetCenteredCoords(hWnd, &x, &y);
                    Draw(hWnd, x, y, curimg->image->GetWidth(), curimg->image->GetHeight());
                } break;
                case VK_SUBTRACT:
                {
                    scaley = scalex -= 0.1f;
                    s32 x, y;
                    GetCenteredCoords(hWnd, &x, &y);
                    Draw(hWnd, x, y, curimg->image->GetWidth(), curimg->image->GetHeight());
                } break;
                case VK_ESCAPE:
                {
                    PostQuitMessage(0);
                } break;
            }
        } break;
        case WM_KEYUP:
        case WM_SYSKEYUP:
        {
            switch(wParam)
            {
                case VK_MENU:
                {
                    if (handles)
                    {
                        handles = false;
                        dragmode = DragMode_Resize;
                        Draw(hWnd);
                    }
                } break;
            }
        } break;
        case WM_LBUTTONDBLCLK:
        {
            Advance(hWnd, Advance_Forwards);
        } break;
        case WM_LBUTTONDOWN:
        {
            //dragmode = DragMode_Drag;
            //dragmode = DragMode_Rotate;
            dragmode = DragMode_Resize;
            SetCapture(hWnd);
            SetCursor(cursor_sizeall);

            GetCursorPos(&curpos);
            ScreenToClient(hWnd, &curpos_rel);
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
                    MoveWindow(hWnd, rect.left + x - curpos.x, rect.top + y - curpos.y,
                                     rect.right - rect.left, rect.bottom - rect.top, TRUE);
                } break;
                case DragMode_Rotate:
                {
                    POINT newcurpos;
                    GetCursorPos(&newcurpos);
                    ScreenToClient(hWnd, &newcurpos);

                    RECT rect;
                    GetClientRect(hWnd, &rect);
                    s32 halfwidth = (rect.right - rect.left) / 2;
                    s32 halfheight = (rect.bottom - rect.top) / 2;

                    s32 oldx = curpos_rel.x - halfwidth;
                    s32 newx = newcurpos.x - halfwidth;
                    s32 oldy = curpos_rel.y - halfheight;
                    s32 newy = newcurpos.y - halfheight;

                    r32 a = atan2f((r32)newy, (r32)newx) / M_PI * 180.0f;
                    r32 b = atan2f((r32)oldy, (r32)oldx) / M_PI * 180.0f;
                    angle += (a - b);

                    char msgbuf[256];
                    sprintf(msgbuf, "%f %f %f %d %d %d %d\n", angle, a, b, newx, newy, oldx, oldy);
                    OutputDebugString(msgbuf);

                    Draw(hWnd);

                    curpos_rel = newcurpos;
                } break;
                case DragMode_Resize:
                {
                    POINT newcurpos;
                    GetCursorPos(&newcurpos);
                    //ScreenToClient(hWnd, &newcurpos);

                    s32 xdif = newcurpos.x - curpos.x;
                    s32 ydif = newcurpos.y - curpos.y;

                    RECT rect;
                    GetClientRect(hWnd, &rect);
                    
                    //s32 w = rect.right - rect.left;
                    //s32 h = rect.bottom - rect.top;
                    s32 w = curimg->image->GetWidth();
                    s32 h = curimg->image->GetHeight();
                    scalex = ((r32)w + (2.0f * (r32)xdif)) / (r32)w;
                    scaley = ((r32)h - (2.0f * (r32)ydif)) / (r32)h;

                    char msgbuf[256];
                    sprintf(msgbuf, "%f %f %d %d %d %d %d %d\n", scalex, scaley, w, h, xdif, ydif, curpos.x, newcurpos.x);
                    OutputDebugString(msgbuf);

                    //Draw(hWnd);
                    GetCenteredCoords(hWnd, &x, &y);
                    Draw(hWnd, x, y, curimg->image->GetWidth(), curimg->image->GetHeight());
                } break;
                default:
                {
                } break;
            }
        } break;
        case WM_LBUTTONUP:
        {
            dragmode = DragMode_None;
            ReleaseCapture();
            SetCursor(cursor_arrow);
        } break;
        case WM_MOVE:
        {
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
    //wcex.style          = CS_DBLCLKS | CS_HREDRAW | CS_VREDRAW;
    wcex.lpfnWndProc    = (WNDPROC)WndProc;
    wcex.hInstance      = hInstance;
    wcex.hIcon          = LoadIcon(hInstance, MAKEINTRESOURCE(IDI_ICON1));
    wcex.hCursor        = LoadCursor(NULL, IDC_ARROW);
    wcex.lpszClassName  = szClassName;

    if (!RegisterClassEx(&wcex))
        return FALSE;

    cursor_arrow    = LoadCursor(NULL, IDC_ARROW);
    cursor_sizeall  = LoadCursor(NULL, IDC_SIZEALL);

    imglist = (imgfile *)malloc(sizeof(imgfile));
    imglist->filename = L"peng.png";
    imglist->image = new Bitmap(L"peng.png");
    imglist->next = nullptr;
    imglist->prev = nullptr;

    curimg = imglist;
    u32 w = curimg->image->GetWidth();
    u32 h = curimg->image->GetHeight();

    HWND hWnd = CreateWindowEx(WS_EX_LAYERED, szClassName, szAppName,
                               WS_POPUP,
                               CW_USEDEFAULT, CW_USEDEFAULT, w, h,
                               NULL, NULL, hInstance, NULL);

    if (!hWnd)
        return FALSE;

    HMONITOR mon = MonitorFromWindow(hWnd, MONITOR_DEFAULTTONEAREST);
    MONITORINFO moninfo = { sizeof(moninfo) };
    GetMonitorInfo(mon, &moninfo);

    s32 cx = moninfo.rcMonitor.right - moninfo.rcMonitor.left;
    s32 cy = moninfo.rcMonitor.bottom - moninfo.rcMonitor.top;

    Draw(hWnd, (cx / 2) - (w / 2), (cy / 2) - (h / 2), w, h);
    
    //SetWindowPos(hWnd, NULL, (cx / 2) - (w / 2), (cy / 2) - (h / 2), NULL, NULL, SWP_NOSIZE | SWP_NOZORDER);
    ShowWindow(hWnd, SW_SHOW);
    //UpdateWindow(hWnd);

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

