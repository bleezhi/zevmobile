#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <stdint.h>
#include <string.h>
#include "phone.h"
#include "../kernel/kernel.h"

#define WINDOW_W 360
#define WINDOW_H 480

static ZevPhone phone;
static HWND window_handle;
static HBITMAP display_bitmap;
static HDC display_dc;
static uint32_t *display_pixels;

static int init_display_bitmap(void)
{
    BITMAPINFO info;
    memset(&info, 0, sizeof(info));
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = ZEV_SCREEN_WIDTH;
    info.bmiHeader.biHeight = -ZEV_SCREEN_HEIGHT;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;

    HDC window_dc = GetDC(window_handle);
    if (!window_dc) return 0;

    display_dc = CreateCompatibleDC(window_dc);
    display_bitmap = CreateDIBSection(window_dc, &info, DIB_RGB_COLORS,
                                      (void **)&display_pixels, NULL, 0);
    ReleaseDC(window_handle, window_dc);

    if (!display_dc || !display_bitmap || !display_pixels) {
        if (display_bitmap) DeleteObject(display_bitmap);
        if (display_dc) DeleteDC(display_dc);
        display_bitmap = NULL;
        display_dc = NULL;
        display_pixels = NULL;
        return 0;
    }

    SelectObject(display_dc, display_bitmap);
    return 1;
}

static void destroy_display_bitmap(void)
{
    if (display_bitmap) DeleteObject(display_bitmap);
    if (display_dc) DeleteDC(display_dc);
    display_bitmap = NULL;
    display_dc = NULL;
    display_pixels = NULL;
}

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

    if (display_pixels && display_dc) {
        memcpy(display_pixels, phone.framebuffer, sizeof(phone.framebuffer));
        SetStretchBltMode(dc, COLORONCOLOR);
        StretchBlt(dc, screen_x, screen_y, screen_w, screen_h,
                   display_dc, 0, 0, ZEV_SCREEN_WIDTH, ZEV_SCREEN_HEIGHT,
                   SRCCOPY);
    }

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
        destroy_display_bitmap();
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

    if (!init_display_bitmap()) {
        DestroyWindow(window_handle);
        return 1;
    }

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
