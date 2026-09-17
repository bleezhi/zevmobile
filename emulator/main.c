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
static FILE *startup_log;

static void log_line(const char *message){
    if(!startup_log) startup_log=fopen("zevmobile.log","a");
    if(startup_log){fprintf(startup_log,"%s\n",message);fflush(startup_log);}
}

static void log_jvm_result(const char *stage,int rc){
    char line[512];
    snprintf(line,sizeof(line),"%s: rc=%d (%s)",stage,rc,zev_jvm_error_string(rc));
    log_line(line);
}

static LONG WINAPI startup_exception_filter(EXCEPTION_POINTERS *info){
    char line[256];
    DWORD code=info&&info->ExceptionRecord?info->ExceptionRecord->ExceptionCode:0;
    snprintf(line,sizeof(line),"NATIVE CRASH: Windows exception 0x%08lX",(unsigned long)code);
    log_line(line);
    log_line("The JVM/emulator terminated before returning normally. See this log for the last completed stage.");
    MessageBoxA(NULL,"zevMobile hit a native Windows exception during startup.\n\nCheck zevmobile.log for the exception code and last completed stage.","zevMobile native crash",MB_OK|MB_ICONERROR);
    return EXCEPTION_EXECUTE_HANDLER;
}

static void startup_error(const char *message){
    log_line("STARTUP ERROR:");
    log_line(message);
    MessageBoxA(NULL,message,"zevMobile startup error",MB_OK|MB_ICONERROR);
}

static int init_display_bitmap(void){BITMAPINFO info;memset(&info,0,sizeof(info));info.bmiHeader.biSize=sizeof(BITMAPINFOHEADER);info.bmiHeader.biWidth=ZEV_SCREEN_WIDTH;info.bmiHeader.biHeight=-ZEV_SCREEN_HEIGHT;info.bmiHeader.biPlanes=1;info.bmiHeader.biBitCount=32;info.bmiHeader.biCompression=BI_RGB;HDC dc=GetDC(window_handle);if(!dc)return 0;display_dc=CreateCompatibleDC(dc);display_bitmap=CreateDIBSection(dc,&info,DIB_RGB_COLORS,(void**)&display_pixels,NULL,0);ReleaseDC(window_handle,dc);if(!display_dc||!display_bitmap||!display_pixels)return 0;SelectObject(display_dc,display_bitmap);memcpy(display_pixels,phone.framebuffer,sizeof(phone.framebuffer));return 1;}
static void destroy_display_bitmap(void){if(display_bitmap)DeleteObject(display_bitmap);if(display_dc)DeleteDC(display_dc);display_bitmap=NULL;display_dc=NULL;display_pixels=NULL;}
static void sync_display(void){if(display_pixels)memcpy(display_pixels,phone.framebuffer,sizeof(phone.framebuffer));if(window_handle)InvalidateRect(window_handle,NULL,FALSE);}
static void draw_phone(HDC dc){RECT c;GetClientRect(window_handle,&c);int sw=c.right-80,sh=c.bottom-90;double sx=(double)sw/ZEV_SCREEN_WIDTH,sy=(double)sh/ZEV_SCREEN_HEIGHT,s=sx<sy?sx:sy;sw=(int)(ZEV_SCREEN_WIDTH*s);sh=(int)(ZEV_SCREEN_HEIGHT*s);int x=(c.right-sw)/2;FillRect(dc,&c,(HBRUSH)(COLOR_WINDOW+1));if(display_pixels&&display_dc){SetStretchBltMode(dc,COLORONCOLOR);StretchBlt(dc,x,25,sw,sh,display_dc,0,0,ZEV_SCREEN_WIDTH,ZEV_SCREEN_HEIGHT,SRCCOPY);}SetBkMode(dc,TRANSPARENT);SetTextColor(dc,RGB(80,80,80));TextOutA(dc,10,c.bottom-28,"Arrow keys + Enter + Esc | 0-9",27);}
static void send_key(WPARAM wparam){ZevKey key=ZEV_KEY_NONE;switch(wparam){case VK_UP:key=ZEV_KEY_UP;break;case VK_DOWN:key=ZEV_KEY_DOWN;break;case VK_LEFT:key=ZEV_KEY_LEFT;break;case VK_RIGHT:key=ZEV_KEY_RIGHT;break;case VK_RETURN:key=ZEV_KEY_OK;break;case VK_ESCAPE:key=ZEV_KEY_BACK;break;case '0':key=ZEV_KEY_0;break;case '1':key=ZEV_KEY_1;break;case '2':key=ZEV_KEY_2;break;case '3':key=ZEV_KEY_3;break;case '4':key=ZEV_KEY_4;break;case '5':key=ZEV_KEY_5;break;case '6':key=ZEV_KEY_6;break;case '7':key=ZEV_KEY_7;break;case '8':key=ZEV_KEY_8;break;case '9':key=ZEV_KEY_9;break;default:return;}zev_phone_key(&phone,key);sync_display();}
static LRESULT CALLBACK window_proc(HWND hwnd,UINT message,WPARAM wparam,LPARAM lparam){(void)lparam;switch(message){case WM_ERASEBKGND:return 1;case WM_PAINT:{PAINTSTRUCT ps;HDC dc=BeginPaint(hwnd,&ps);draw_phone(dc);EndPaint(hwnd,&ps);return 0;}case WM_KEYDOWN:send_key(wparam);return 0;case WM_TIMER:zev_phone_tick(&phone);zev_kernel_tick(&phone);sync_display();return 0;case WM_DESTROY:destroy_display_bitmap();PostQuitMessage(0);return 0;default:return DefWindowProcA(hwnd,message,wparam,lparam);}}

int WINAPI WinMain(HINSTANCE instance,HINSTANCE previous,LPSTR command_line,int show)
{
    (void)previous;(void)command_line;
    SetUnhandledExceptionFilter(startup_exception_filter);
    startup_log=fopen("zevmobile.log","w");
    log_line("=== zevMobile startup ===");
    WNDCLASSA wc={0};wc.lpfnWndProc=window_proc;wc.hInstance=instance;wc.hCursor=LoadCursor(NULL,IDC_ARROW);wc.hbrBackground=NULL;wc.lpszClassName="ZevMobileEmulator";
    log_line("Registering window class...");
    if(!RegisterClassA(&wc)){startup_error("RegisterClassA failed.");if(startup_log)fclose(startup_log);return 1;}
    log_line("Creating emulator window...");
    window_handle=CreateWindowExA(0,wc.lpszClassName,"zevMobile Emulator",WS_OVERLAPPEDWINDOW,CW_USEDEFAULT,CW_USEDEFAULT,WINDOW_W,WINDOW_H,NULL,NULL,instance,NULL);
    if(!window_handle){startup_error("CreateWindowExA failed.");if(startup_log)fclose(startup_log);return 1;}
    ShowWindow(window_handle,show);UpdateWindow(window_handle);
    log_line("Window created.");
    log_line("Initializing phone/kernel/JVM...");
    zev_phone_init(&phone);zev_kernel_boot(&phone);zev_jvm_init(&jvm,&phone);
    log_line("JVM initialized.");
    char load_line[128];snprintf(load_line,sizeof(load_line),"Loading Launcher.class (%zu bytes)...",zev_launcher_class.size);log_line(load_line);
    int rc=zev_jvm_load_class(&jvm,zev_launcher_class);
    log_jvm_result("zev_jvm_load_class",rc);
    if(rc!=0){char msg[256];snprintf(msg,sizeof(msg),"JVM failed to load Launcher.class.\n\nError %d: %s\n\nSee zevmobile.log for the last startup stage.",jvm.last_error,zev_jvm_error_string(jvm.last_error));startup_error(msg);if(startup_log)fclose(startup_log);return 1;}
    log_line("Launcher.class loaded. Starting main()...");
    rc=zev_jvm_run_class(&jvm,zev_launcher_class);
    log_jvm_result("zev_jvm_run_class",rc);
    if(rc!=0){char msg[256];snprintf(msg,sizeof(msg),"JVM failed to run Launcher.class.\n\nError %d: %s\n\nSee zevmobile.log for the last startup stage.",jvm.last_error,zev_jvm_error_string(jvm.last_error));startup_error(msg);if(startup_log)fclose(startup_log);return 1;}
    log_line("Launcher main() completed successfully.");
    log_line("Creating display framebuffer...");
    if(!init_display_bitmap()){startup_error("Failed to create the display framebuffer.\n\nSee zevmobile.log for startup details.");if(startup_log)fclose(startup_log);return 1;}
    log_line("Display framebuffer ready.");
    if(GetFileAttributesA("assets\\startup.wav")!=INVALID_FILE_ATTRIBUTES){log_line("Playing startup sound.");PlaySoundA("assets\\startup.wav",NULL,SND_FILENAME|SND_ASYNC|SND_NODEFAULT);}else log_line("Startup sound not found; continuing without it.");
    sync_display();SetTimer(window_handle,1,16,NULL);
    log_line("=== zevMobile is running ===");
    if(startup_log){fclose(startup_log);startup_log=NULL;}
    MSG message;while(GetMessageA(&message,NULL,0,0)>0){TranslateMessage(&message);DispatchMessageA(&message);}return(int)message.wParam;
}
