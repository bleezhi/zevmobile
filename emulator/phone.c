#include "phone.h"

#include <string.h>

static void fill_rect(ZevPhone *phone, int x, int y, int width, int height, uint32_t pixel)
{
    for (int py = y; py < y + height; ++py) {
        for (int px = x; px < x + width; ++px) {
            zev_phone_set_pixel(phone, px, py, pixel);
        }
    }
}

static void draw_boot_screen(ZevPhone *phone)
{
    /* Background. */
    zev_phone_clear_screen(phone, 0x00101018u);

    /* Status bar. */
    fill_rect(phone, 0, 0, ZEV_SCREEN_WIDTH, 24, 0x001B1B28u);
    fill_rect(phone, 8, 9, 7, 7, 0x0048D9FFu);
    fill_rect(phone, 19, 9, 7, 7, 0x0048D9FFu);

    /* Simple zevMobile logo made entirely from framebuffer pixels. */
    fill_rect(phone, 91, 86, 58, 8, 0x0048D9FFu);
    fill_rect(phone, 91, 94, 8, 42, 0x0048D9FFu);
    fill_rect(phone, 141, 94, 8, 42, 0x0048D9FFu);
    fill_rect(phone, 99, 105, 42, 8, 0x0048D9FFu);

    /* Boot indicator / virtual display output. */
    fill_rect(phone, 48, 158, 144, 4, 0x00303040u);
    fill_rect(phone, 48, 158, 144, 4, 0x0048D9FFu);

    /* Bottom navigation area. */
    fill_rect(phone, 0, 288, ZEV_SCREEN_WIDTH, 32, 0x001B1B28u);
    fill_rect(phone, 34, 298, 28, 4, 0x00606070u);
    fill_rect(phone, 106, 298, 28, 4, 0x00606070u);
    fill_rect(phone, 178, 298, 28, 4, 0x00606070u);
}

void zev_phone_init(ZevPhone *phone)
{
    memset(phone, 0, sizeof(*phone));

    phone->info.ram_size = ZEV_RAM_SIZE;
    phone->info.flash_size = ZEV_FLASH_SIZE;
    phone->info.screen_width = ZEV_SCREEN_WIDTH;
    phone->info.screen_height = ZEV_SCREEN_HEIGHT;

    draw_boot_screen(phone);
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
