#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <mmsystem.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>
#include "phone.h"
#include "../kernel/kernel.h"
#include "../jvm/jvm.h"

#define WINDOW_W 360
#define WINDOW_H 480

static ZevPhone phone; static ZevJvm jvm; static HWND window_handle;
static HBITMAP display_bitmap; static HDC display_dc; static uint32_t *display_pixels;

static void startup_error(const char *message){MessageBoxA(NULL,message,"zevMobile startup error",MB_OK|MB_ICONERROR);}
static int init_display_bitmap(void){BITMAPINFO info;memset(&info,0,sizeof(info));info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=ZEV_SCREEN_WIDTH;info.bmiHeader.biHeight=-ZEV_SCREEN_HEIGHT;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;HDC dc=GetDC(window_handle);if(!dc)return 0;display_dc=CreateCompatibleDC(dc);display_bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,(void**)&display_pixels,NULL,0);ReleaseDC(window_handle,dc);if(!display_dc||!display_bitmap||!display_pixels)return 0;SelectObject(display_dc,display_bitmap);memcpy(display_pixels,phone.framebuffer,sizeof(phone.framebuffer));return 1;}
static void destroy_display_bitmap(void){if(display_bitmap)DeleteObject(display_bitmap);if(display_dc)DeleteDC(display_dc);display_bitmap=NULL;display_dc=NULL;display_pixels=NULL;}
static void sync_display(void){if(display_pixels)memcpy(display_pixels,phone.framebuffer,sizeof(phone.framebuffer));if(window_handle)InvalidateRect(window_handle,NULL,FALSE);}
static void draw_phone(HDC dc){RECT c;GetClientRect(window_handle,&c);int sw=c.right-80,sh=c.bottom-90;double sx=(double)sw/ZEV_SCREEN_WIDTH,sy=(double)sh/ZEV_SCREEN_HEIGHT,s=sx<sy?sx:sy;sw=(int)(ZEV_SCREEN_WIDTH*s);sh=(int)(ZEV_SCREEN_HEIGHT*s);int x=(c.right-sw)/2;FillRect(dc,&c,(HBRUSH)(COLOR_WINDOW+1));if(display_pixels&&display_dc){SetStretchBltMode(dc,COLORONCOLOR);StretchBlt(dc,x,25,sw,sh,display_dc,0,0,ZEV_SCREEN_WIDTH,ZEV_SCREEN_HEIGHT,SRCCOPY);}SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(80,80,80));TextOutA(dc,10,c.bottom-28,"Arrow keys + Enter + Esc | 0-9",27);}
static void send_key(WPARAM wparam){ZevKey key=ZEV_KEY_NONE;switch(wparam){case VK_UP:key=ZEV_KEY_UP;break;case VK_DOWN:key=ZEV_KEY_DOWN;break;case VK_LEFT:key=ZEV_KEY_LEFT;break;case VK_RIGHT:key=ZEV_KEY_RIGHT;break;case VK_RETURN:key=ZEV_KEY_OK;break;case VK_ESCAPE:key=ZEV_KEY_BACK;break;case '0':key=ZEV_KEY_0;break;case '1':key=ZEV_KEY_1;break;case '2':key=ZEV_KEY_2;break;case '3':key=ZEV_KEY_3;break;case '4':key=ZEV_KEY_4;break;case '5':key=ZEV_KEY_5;break;case '6':key=ZEV_KEY_6;break;case '7':key=ZEV_KEY_7;break;case '8':key=ZEV_KEY_8;break;case '9':key=ZEV_KEY_9;break;default:return;}zev_phone_key(&phone,key);sync_display();}
static LRESULT CALLBACK window_proc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam){(void)lparam;switch(message){case WM_ERASEBKGND:return 1;case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);draw_phone(dc);EndPaint(hwnd,&ps);return 0;}case WM_KEYDOWN:send_key(wparam);return 0;case WM_TIMER:zev_phone_tick(&phone);zev_kernel_tick(&phone);sync_display();return 0;case WM_DESTROY:destroy_display_bitmap();PostQuitMessage(0);return 0;default:return DefWindowProcA(hwnd,message,wparam,lparam);}}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR command_line,int show)
{
    (void)previous;(void)command_line;
    WNDCLASSA wc={0};wc.lpfnWndProc=window_proc;wc.hInstance=instance;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=NULL;wc.lpszClassName="ZevMobileEmulator";
    if(!RegisterClassA(&wc)){startup_error("RegisterClassA failed.");return 1;}
    window_handle=CreateWindowExA(0,wc.lpszClassName,"zevMobile Emulator",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,WINDOW_W,WINDOW_H,NULL,NULL,instance,NULL);
    if(!window_handle){startup_error("CreateWindowExA failed.");return 1;}
    ShowWindow(window_handle,show);UpdateWindow(window_handle);
    zev_phone_init(&phone);zev_kernel_boot(&phone);zev_jvm_init(&jvm,&phone);
    printf("zevMobile JVM: loading Launcher.class (%zu bytes)\n",zev_launcher_class.size);
    if(zev_jvm_load_class(&jvm,zev_launcher_class)!=0){startup_error("JVM failed to load Launcher.class.");DestroyWindow(window_handle);return 1;}
    if(zev_jvm_run_class(&jvm,zev_launcher_class)!=0){startup_error("JVM failed to run Launcher.class.");DestroyWindow(window_handle);return 1;}
    if(!init_display_bitmap()){startup_error("Failed to create the display framebuffer.");DestroyWindow(window_handle);return 1;}
    if(GetFileAttributesA("assets\\startup.wav")!=INVALID_FILE_ATTRIBUTES)PlaySoundA("assets\\startup.wav",NULL,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);
    sync_display();SetTimer(window_handle,1,16,NULL);
    MSG message;while(GetMessageA(&message,NULL,0,0)>0){TranslateMessage(&message);DispatchMessageA(&message);}return(int)message.wParam;
}
