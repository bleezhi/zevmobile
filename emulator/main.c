#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include "phone.h"
#include "../kernel/kernel.h"

#define WINDOW_W 360
#define WINDOW_H 480

static ZevPhone phone;
static HWND window_handle;
static BITMAPINFO bitmap_info;

static void draw_phone(HDC dc)
{
    RECT client;
    GetClientRect(window_handle, &client);
    FillRect(dc, &client, (HBRUSH)(COLOR_WINDOW + 1));

    int screen_w = client.right - 80;
    int screen_h = client.bottom - 90;
    int screen_x = 40;
    int screen_y = 25;
    double sx = (double)screen_w / ZEV_SCREEN_WIDTH;
    double sy = (double)screen_h / ZEV_SCREEN_HEIGHT;
    double scale = sx < sy ? sx : sy;

    screen_w = (int)(ZEV_SCREEN_WIDTH * scale);
    screen_h = (int)(ZEV_SCREEN_HEIGHT * scale);
    screen_x = (client.right - screen_w) / 2;

    StretchDIBits(dc, screen_x, screen_y, screen_w, screen_h,
                  0, 0, ZEV_SCREEN_WIDTH, ZEV_SCREEN_HEIGHT,
                  phone.framebuffer, &bitmap_info, DIB_RGB_COLORS, SRCCOPY);

    SetBkMode(dc, TRANSPARENT);
    SetTextColor(dc, RGB(80, 80, 80));
    TextOutA(dc, 10, client.bottom - 28,
             "Arrow keys + Enter + Esc", 24);
}

static LRESULT CALLBACK window_proc(HWND hwnd, UINT message, WPARAM wparam, LPARAM lparam)
{
    (void)lparam;

    switch (message) {
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC dc = BeginPaint(hwnd, &ps);
        draw_phone(dc);
        EndPaint(hwnd, &ps);
        return 0;
    }
    case WM_KEYDOWN:
        switch (wparam) {
        case VK_UP: zev_phone_key(&phone, ZEV_KEY_UP); break;
        case VK_DOWN: zev_phone_key(&phone, ZEV_KEY_DOWN); break;
        case VK_LEFT: zev_phone_key(&phone, ZEV_KEY_LEFT); break;
        case VK_RIGHT: zev_phone_key(&phone, ZEV_KEY_RIGHT); break;
        case VK_RETURN: zev_phone_key(&phone, ZEV_KEY_OK); break;
        case VK_ESCAPE: zev_phone_key(&phone, ZEV_KEY_BACK); break;
        default: break;
        }
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_TIMER:
        zev_phone_tick(&phone);
        zev_kernel_tick(&phone);
        InvalidateRect(hwnd, NULL, FALSE);
        return 0;
    case WM_DESTROY:
        PostQuitMessage(0);
        return 0;
    default:
        return DefWindowProcA(hwnd, message, wparam, lparam);
    }
}

int WINAPI WinMain(HINSTANCE instance, HINSTANCE previous, LPSTR command_line, int show)
{
    (void)previous;
    (void)command_line;

    zev_phone_init(&phone);
    zev_kernel_boot(&phone);

    bitmap_info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bitmap_info.bmiHeader.biWidth = ZEV_SCREEN_WIDTH;
    bitmap_info.bmiHeader.biHeight = -ZEV_SCREEN_HEIGHT;
    bitmap_info.bmiHeader.biPlanes = 1;
    bitmap_info.bmiHeader.biBitCount = 32;
    bitmap_info.bmiHeader.biCompression = BI_RGB;

    WNDCLASSA wc = {0};
    wc.lpfnWndProc = window_proc;
    wc.hInstance = instance;
    wc.hCursor = LoadCursor(NULL, IDC_ARROW);
    wc.lpszClassName = "ZevMobileEmulator";

    if (!RegisterClassA(&wc)) return 1;

    window_handle = CreateWindowExA(0, wc.lpszClassName, "zevMobile Emulator",
        WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT,
        WINDOW_W, WINDOW_H, NULL, NULL, instance, NULL);

    if (!window_handle) return 1;

    ShowWindow(window_handle, show);
    UpdateWindow(window_handle);
    SetTimer(window_handle, 1, 16, NULL);

    MSG message;
    while (GetMessageA(&message, NULL, 0, 0) > 0) {
        TranslateMessage(&message);
        DispatchMessageA(&message);
    }

    return (int)message.wParam;
}
