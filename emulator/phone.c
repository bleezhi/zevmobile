#include "phone.h"

#include <string.h>

#define C_BG       0x00101018u
#define C_BAR      0x001B1B28u
#define C_ACCENT   0x0048D9FFu
#define C_TEXT     0x00E8E8F0u
#define C_MUTED    0x00606070u
#define C_CARD     0x0020202Cu

static void fill_rect(ZevPhone *phone, int x, int y, int width, int height, uint32_t pixel)
{
    for (int py = y; py < y + height; ++py)
        for (int px = x; px < x + width; ++px)
            zev_phone_set_pixel(phone, px, py, pixel);
}

static void glyph(ZevPhone *phone, int x, int y, char ch, uint32_t color)
{
    static const uint8_t font[10][5] = {
        {0x1e,0x21,0x21,0x21,0x1e},{0x00,0x22,0x3f,0x20,0x00},
        {0x32,0x29,0x29,0x29,0x26},{0x12,0x21,0x25,0x25,0x1a},
        {0x0c,0x0a,0x09,0x3f,0x08},{0x17,0x25,0x25,0x25,0x19},
        {0x1e,0x25,0x25,0x25,0x18},{0x01,0x39,0x05,0x03,0x01},
        {0x1a,0x25,0x25,0x25,0x1a},{0x06,0x29,0x29,0x29,0x1e}
    };
    if (ch >= '0' && ch <= '9') {
        uint8_t v;
        for (int row=0; row<5; ++row) {
            v=font[ch-'0'][row];
            for (int col=0; col<6; ++col) if (v & (1u << (5-col))) fill_rect(phone,x+col,y+row,1,1,color);
        }
    }
}

static void number(ZevPhone *phone, int x, int y, unsigned value, uint32_t color)
{
    char buf[12]; int n=0;
    if (!value) buf[n++]='0';
    while (value && n < 10) { buf[n++]=(char)('0'+value%10u); value/=10u; }
    for (int i=0;i<n/2;i++){char t=buf[i];buf[i]=buf[n-1-i];buf[n-1-i]=t;}
    for(int i=0;i<n;i++) glyph(phone,x+i*7,y,buf[i],color);
}

static void draw_status(ZevPhone *phone)
{
    fill_rect(phone,0,0,ZEV_SCREEN_WIDTH,24,C_BAR);
    fill_rect(phone,8,9,7,7,C_ACCENT);
    fill_rect(phone,19,9,7,7,C_ACCENT);
    /* Signal bars. */
    for(int i=0;i<4;i++) fill_rect(phone,164+i*6,16-(i<phone->signal?i*3:0),4,(i<phone->signal?3+i*3:2),i<phone->signal?C_TEXT:C_MUTED);
    /* Battery. */
    fill_rect(phone,201,8,29,13,C_MUTED); fill_rect(phone,230,12,3,5,C_MUTED);
    fill_rect(phone,203,10,(int)(24u*phone->battery/100u),9,C_ACCENT);
    number(phone,170,27,(unsigned)(phone->ticks/62u),C_MUTED);
}

static void draw_boot_screen(ZevPhone *phone)
{
    zev_phone_clear_screen(phone,C_BG);
    fill_rect(phone,0,0,ZEV_SCREEN_WIDTH,24,C_BAR);
    /* Large pixel zevMobile mark. */
    int y=82+(phone->boot_frame%3)*2;
    fill_rect(phone,91,y,58,8,C_ACCENT); fill_rect(phone,91,y+8,8,42,C_ACCENT);
    fill_rect(phone,141,y+8,8,42,C_ACCENT); fill_rect(phone,99,y+19,42,8,C_ACCENT);
    fill_rect(phone,48,158,144,4,C_CARD);
    fill_rect(phone,48,158,24+(phone->boot_frame*24),4,C_ACCENT);
    fill_rect(phone,0,288,ZEV_SCREEN_WIDTH,32,C_BAR);
    fill_rect(phone,34,298,28,4,C_MUTED); fill_rect(phone,106,298,28,4,C_MUTED); fill_rect(phone,178,298,28,4,C_MUTED);
}

static void draw_home(ZevPhone *phone)
{
    zev_phone_clear_screen(phone,C_BG); draw_status(phone);
    fill_rect(phone,16,52,208,82,C_CARD);
    fill_rect(phone,26,63,46,46,C_ACCENT);
    /* Simple phone glyph. */
    fill_rect(phone,39,70,20,30,C_BG); fill_rect(phone,43,74,12,20,C_ACCENT); fill_rect(phone,47,96,4,2,C_BG);
    number(phone,86,64,0,C_TEXT); number(phone,86,79,100u-phone->battery,C_MUTED);
    fill_rect(phone,16,151,98,66,C_CARD); fill_rect(phone,126,151,98,66,C_CARD);
    fill_rect(phone,27,165,30,30,C_ACCENT); fill_rect(phone,137,165,30,30,C_ACCENT);
    fill_rect(phone,16,232,208,45,C_CARD);
    fill_rect(phone,25,244,42,5,C_ACCENT); fill_rect(phone,74,244,42,5,C_MUTED); fill_rect(phone,123,244,42,5,C_MUTED); fill_rect(phone,172,244,42,5,C_MUTED);
    fill_rect(phone,0,288,ZEV_SCREEN_WIDTH,32,C_BAR);
    fill_rect(phone,34,298,28,4,C_MUTED); fill_rect(phone,106,298,28,4,C_ACCENT); fill_rect(phone,178,298,28,4,C_MUTED);
}

static void draw_apps(ZevPhone *phone)
{
    zev_phone_clear_screen(phone,C_BG); draw_status(phone);
    const uint32_t cards[4]={C_ACCENT,0x006D7CFFu,0x0070C070u,0x00C08040u};
    for(int i=0;i<4;i++) {
        int x=16+(i%2)*108, y=52+(i/2)*78;
        fill_rect(phone,x,y,98,66,i==phone->selected_app?0x00303040u:C_CARD);
        fill_rect(phone,x+12,y+12,30,30,cards[i]);
        if(i==phone->selected_app) fill_rect(phone,x,y+63,98,3,C_ACCENT);
        number(phone,x+52,y+18,(unsigned)(i+1),C_TEXT);
    }
    fill_rect(phone,0,288,ZEV_SCREEN_WIDTH,32,C_BAR);
    fill_rect(phone,34,298,28,4,C_MUTED); fill_rect(phone,106,298,28,4,C_MUTED); fill_rect(phone,178,298,28,4,C_ACCENT);
}

static void draw_about(ZevPhone *phone)
{
    zev_phone_clear_screen(phone,C_BG); draw_status(phone);
    fill_rect(phone,20,52,200,170,C_CARD); fill_rect(phone,83,70,74,10,C_ACCENT);
    fill_rect(phone,75,94,90,8,C_ACCENT); fill_rect(phone,75,110,90,8,C_ACCENT);
    fill_rect(phone,40,150,160,4,C_MUTED); fill_rect(phone,40,164,120,4,C_MUTED); fill_rect(phone,40,178,145,4,C_MUTED);
    fill_rect(phone,0,288,ZEV_SCREEN_WIDTH,32,C_BAR); fill_rect(phone,106,298,28,4,C_ACCENT);
}

void zev_phone_draw(ZevPhone *phone)
{
    switch(phone->screen){case ZEV_UI_HOME:draw_home(phone);break;case ZEV_UI_APPS:draw_apps(phone);break;case ZEV_UI_ABOUT:draw_about(phone);break;default:draw_boot_screen(phone);break;}
}

void zev_phone_init(ZevPhone *phone)
{
    memset(phone,0,sizeof(*phone));
    phone->info.ram_size=ZEV_RAM_SIZE; phone->info.flash_size=ZEV_FLASH_SIZE;
    phone->info.screen_width=ZEV_SCREEN_WIDTH; phone->info.screen_height=ZEV_SCREEN_HEIGHT;
    phone->battery=100; phone->signal=4; phone->screen=ZEV_UI_BOOT; phone->boot_frame=0;
    zev_phone_draw(phone);
}

void zev_phone_tick(ZevPhone *phone)
{
    phone->ticks++; phone->last_key=ZEV_KEY_NONE;
    if(phone->battery>10 && phone->ticks%9000u==0) phone->battery--;
    if(phone->screen==ZEV_UI_BOOT && phone->ticks%18u==0) {
        phone->boot_frame++;
        if(phone->boot_frame>=6){phone->screen=ZEV_UI_HOME;phone->notification_ticks=240;}
    }
    if(phone->notification_ticks) phone->notification_ticks--;
    zev_phone_draw(phone);
}

void zev_phone_clear_screen(ZevPhone *phone,uint32_t pixel){for(size_t i=0;i<(size_t)ZEV_SCREEN_WIDTH*ZEV_SCREEN_HEIGHT;i++)phone->framebuffer[i]=pixel;}
void zev_phone_set_pixel(ZevPhone *phone,int x,int y,uint32_t pixel){if(x<0||x>=ZEV_SCREEN_WIDTH||y<0||y>=ZEV_SCREEN_HEIGHT)return;phone->framebuffer[(size_t)y*ZEV_SCREEN_WIDTH+(size_t)x]=pixel;}

void zev_phone_key(ZevPhone *phone,ZevKey key)
{
    phone->last_key=key;
    if(phone->screen==ZEV_UI_BOOT)return;
    switch(key){
        case ZEV_KEY_RIGHT: if(phone->screen==ZEV_UI_HOME)phone->screen=ZEV_UI_APPS; else if(phone->screen==ZEV_UI_APPS)phone->selected_app=(uint8_t)((phone->selected_app+1)%4); break;
        case ZEV_KEY_LEFT: if(phone->screen==ZEV_UI_APPS && phone->selected_app==0)phone->screen=ZEV_UI_HOME; else if(phone->screen==ZEV_UI_APPS)phone->selected_app--; break;
        case ZEV_KEY_UP: if(phone->screen==ZEV_UI_APPS && phone->selected_app>=2)phone->selected_app-=2; break;
        case ZEV_KEY_DOWN: if(phone->screen==ZEV_UI_HOME)phone->screen=ZEV_UI_APPS; else if(phone->screen==ZEV_UI_APPS && phone->selected_app<2)phone->selected_app+=2; break;
        case ZEV_KEY_OK: if(phone->screen==ZEV_UI_APPS && phone->selected_app==3)phone->screen=ZEV_UI_ABOUT; else if(phone->screen==ZEV_UI_ABOUT)phone->screen=ZEV_UI_HOME; break;
        case ZEV_KEY_BACK: if(phone->screen!=ZEV_UI_HOME)phone->screen=ZEV_UI_HOME; break;
        default: break;
    }
    zev_phone_draw(phone);
}
