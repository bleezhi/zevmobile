#ifndef ZEV_PHONE_H
#define ZEV_PHONE_H

#include <stdint.h>
#include <stddef.h>

#define ZEV_SCREEN_WIDTH  240
#define ZEV_SCREEN_HEIGHT 320
#define ZEV_RAM_SIZE      (64u * 1024u)
#define ZEV_FLASH_SIZE    (256u * 1024u)

typedef enum {
    ZEV_KEY_NONE = 0,
    ZEV_KEY_UP,
    ZEV_KEY_DOWN,
    ZEV_KEY_LEFT,
    ZEV_KEY_RIGHT,
    ZEV_KEY_OK,
    ZEV_KEY_BACK,
    ZEV_KEY_0,
    ZEV_KEY_1,
    ZEV_KEY_2,
    ZEV_KEY_3,
    ZEV_KEY_4,
    ZEV_KEY_5,
    ZEV_KEY_6,
    ZEV_KEY_7,
    ZEV_KEY_8,
    ZEV_KEY_9
} ZevKey;

typedef struct {
    uint32_t ram_size;
    uint32_t flash_size;
    uint16_t screen_width;
    uint16_t screen_height;
} ZevHardwareInfo;

typedef struct {
    ZevHardwareInfo info;
    uint8_t ram[ZEV_RAM_SIZE];
    uint8_t flash[ZEV_FLASH_SIZE];
    uint32_t framebuffer[ZEV_SCREEN_WIDTH * ZEV_SCREEN_HEIGHT];
    ZevKey last_key;
    uint64_t ticks;
} ZevPhone;

void zev_phone_init(ZevPhone *phone);
void zev_phone_tick(ZevPhone *phone);
void zev_phone_clear_screen(ZevPhone *phone, uint32_t pixel);
void zev_phone_set_pixel(ZevPhone *phone, int x, int y, uint32_t pixel);
void zev_phone_key(ZevPhone *phone, ZevKey key);

#endif
