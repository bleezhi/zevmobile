#include "kernel.h"

#include <stdio.h>

void zev_kernel_boot(ZevPhone *phone)
{
    printf("zevMobile kernel: booting virtual phone\n");
    printf("  RAM:   %u KiB\n", phone->info.ram_size / 1024u);
    printf("  Flash: %u KiB\n", phone->info.flash_size / 1024u);
    printf("  LCD:   %ux%u\n", phone->info.screen_width, phone->info.screen_height);
    printf("zevMobile kernel: hardware online\n");
}

void zev_kernel_tick(ZevPhone *phone)
{
    (void)phone;
}
