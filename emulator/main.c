#include "phone.h"
#include "../kernel/kernel.h"

#include <stdio.h>
#include <stdlib.h>
#include <time.h>

static void draw_boot_screen(const ZevPhone *phone)
{
    /* Terminal representation for the first milestone.
     * The real framebuffer is already present in the virtual hardware and
     * can later be connected to SDL without changing the kernel interface.
     */
    (void)phone;
    printf("\n+------------------------+\n");
    printf("|       zevMobile        |\n");
    printf("|                        |\n");
    printf("|     Virtual Phone      |\n");
    printf("|                        |\n");
    printf("|      SYSTEM READY      |\n");
    printf("|                        |\n");
    printf("+------------------------+\n");
}

int main(void)
{
    ZevPhone phone;

    zev_phone_init(&phone);
    zev_kernel_boot(&phone);
    draw_boot_screen(&phone);

    puts("Press Ctrl+C to stop the emulator.");

    for (;;) {
        zev_kernel_tick(&phone);
        zev_phone_tick(&phone);

        /* Temporary 10 Hz virtual hardware clock. */
        struct timespec delay = {0, 100000000L};
        nanosleep(&delay, NULL);
    }

    return EXIT_SUCCESS;
}
