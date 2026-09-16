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

static int init_display_bitmap(void)
{
    BITMAPINFO info; memset(&info,0,sizeof(info)); info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth=ZEV_SCREEN_WIDTH; info.bmiHeader.biHeight=-ZEV_SCREEN_HEIGHT;
    info.bmiHeader.biPlanes=1; info.bmiHeader.biBitCount=32; info.bmiHeader.biCompression=BI_RGB;
    HDC dc=GetDC(window_handle); if(!dc)return 0; display_dc=CreateCompatibleDC(dc);
    display_bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,(void**)&display_pixels,NULL,0); ReleaseDC(window_handle,dc);
    if(!display_dc||!display_bitmap||!display_pixels)return 0; SelectObject(display_dc,display_bitmap);
    memcpy(display_pixels,phone.framebuffer,sizeof(phone.framebuffer)); return 1;
}
static void destroy_display_bitmap(void){if(display_bitmap)DeleteObject(display_bitmap);if(display_dc)DeleteDC(display_dc);display_bitmap=NULL;display_dc=NULL;display_pixels=NULL;}
static void sync_display(void){if(display_pixels)memcpy(display_pixels,phone.framebuffer,sizeof(phone.framebuffer));if(window_handle)InvalidateRect(window_handle,NULL,FALSE);}
static void draw_phone(HDC dc){RECT c;GetClientRect(window_handle,&c);int sw=c.right-80,sh=c.bottom-90;double sx=(double)sw/ZEV_SCREEN_WIDTH,sy=(double)sh/ZEV_SCREEN_HEIGHT,s=sx<sy?sx:sy;sw=(int)(ZEV_SCREEN_WIDTH*s);sh=(int)(ZEV_SCREEN_HEIGHT*s);int x=(c.right-sw)/2;FillRect(dc,&c,(HBRUSH)(COLOR_WINDOW+1));if(display_pixels&&display_dc){SetStretchBltMode(dc,COLORONCOLOR);StretchBlt(dc,x,25,sw,sh,display_dc,0,0,ZEV_SCREEN_WIDTH,ZEV_SCREEN_HEIGHT,SRCCOPY);}SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(80,80,80));TextOutA(dc,10,c.bottom-28,"Arrow keys + Enter + Esc",24);}
static LRESULT CALLBACK window_proc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam){(void)lparam;switch(message){case WM_ERASEBKGND:return 1;case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);draw_phone(dc);EndPaint(hwnd,&ps);return 0;}case WM_KEYDOWN:switch(wparam){case VK_UP:zev_phone_key(&phone,ZEV_KEY_UP);break;case VK_DOWN:zev_phone_key(&phone,ZEV_KEY_DOWN);break;case VK_LEFT:zev_phone_key(&phone,ZEV_KEY_LEFT);break;case VK_RIGHT:zev_phone_key(&phone,ZEV_KEY_RIGHT);break;case VK_RETURN:zev_phone_key(&phone,ZEV_KEY_OK);break;case VK_ESCAPE:zev_phone_key(&phone,ZEV_KEY_BACK);break;default:return 0;}sync_display();return 0;case WM_TIMER:zev_phone_tick(&phone);zev_kernel_tick(&phone);return 0;case WM_DESTROY:destroy_display_bitmap();PostQuitMessage(0);return 0;default:return DefWindowProcA(hwnd,message,wparam,lparam);}}
int WINAPI WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR command_line,int show)
{
    (void)previous;(void)command_line;zev_phone_init(&phone);zev_kernel_boot(&phone);zev_jvm_init(&jvm,&phone);
    printf("zevMobile JVM: loading Launcher.class (%zu bytes)\n",zev_launcher_class.size);
    if(zev_jvm_load_class(&jvm,zev_launcher_class)!=0||zev_jvm_run_class(&jvm,zev_launcher_class)!=0){fprintf(stderr,"zevMobile JVM: Launcher.class failed\n");return 1;}
    if(GetFileAttributesA("assets\\startup.wav")!=INVALID_FILE_ATTRIBUTES)PlaySoundA("assets\\startup.wav",NULL,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);
    WNDCLASSA wc={0};wc.lpfnWndProc=window_proc;wc.hInstance=instance;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=NULL;wc.lpszClassName="ZevMobileEmulator";
    if(!RegisterClassA(&wc))return 1;window_handle=CreateWindowExA(0,wc.lpszClassName,"zevMobile Emulator",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,WINDOW_W,WINDOW_H,NULL,NULL,instance,NULL);
    if(!window_handle)return 1;if(!init_display_bitmap()){DestroyWindow(window_handle);return 1;}ShowWindow(window_handle,show);UpdateWindow(window_handle);sync_display();SetTimer(window_handle,1,16,NULL);
    MSG message;while(GetMessageA(&message,NULL,0,0)>0){TranslateMessage(&message);DispatchMessageA(&message);}return(int)message.wParam;
}
