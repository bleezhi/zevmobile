#include "phone.h"

#include <string.h>

void zev_phone_init(ZevPhone *phone)
{
    memset(phone, 0, sizeof(*phone));

    phone->info.ram_size = ZEV_RAM_SIZE;
    phone->info.flash_size = ZEV_FLASH_SIZE;
    phone->info.screen_width = ZEV_SCREEN_WIDTH;
    phone->info.screen_height = ZEV_SCREEN_HEIGHT;

    zev_phone_clear_screen(phone, 0x00101018u);
}

void zev_phone_tick(ZevPhone *phone)
{
    phone->ticks++;
    phone->last_key = ZEV_KEY_NONE;
}

void zev_phone_clear_screen(ZevPhone *phone, uint32_t pixel)
{
    for (size_t i = 0; i < (size_t)ZEV_SCREEN_WIDTH * ZEV_SCREEN_HEIGHT; ++i) {
        phone->framebuffer[i] = pixel;
    }
}

void zev_phone_set_pixel(ZevPhone *phone, int x, int y, uint32_t pixel)
{
    if (x < 0 || x >= ZEV_SCREEN_WIDTH || y < 0 || y >= ZEV_SCREEN_HEIGHT) {
        return;
    }

    phone->framebuffer[(size_t)y * ZEV_SCREEN_WIDTH + (size_t)x] = pixel;
}

void zev_phone_key(ZevPhone *phone, ZevKey key)
{
    phone->last_key = key;
}
