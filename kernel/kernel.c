#include "kernel.h"

#include <stdio.h>
#include <string.h>

static ZevKernel kernel_state;

const ZevKernel *zev_kernel_state(void)
{
    return &kernel_state;
}

void zev_kernel_boot(ZevPhone *phone)
{
    static const char version[] = "zevMobile kernel 0.2\n";
    static const char init_name[] = "zinit";

    memset(&kernel_state, 0, sizeof(kernel_state));

    printf("zevMobile kernel: booting virtual phone\n");
    printf("  RAM:   %u KiB\n", phone->info.ram_size / 1024u);
    printf("  Flash: %u KiB\n", phone->info.flash_size / 1024u);
    printf("  LCD:   %ux%u\n", phone->info.screen_width, phone->info.screen_height);

    if (zev_fs_mount(&kernel_state.fs, phone) != 0) {
        printf("zevMobile kernel: filesystem mount failed\n");
        return;
    }

    zev_scheduler_init(&kernel_state.scheduler);
    kernel_state.syscall_context.phone = phone;
    kernel_state.syscall_context.fs = &kernel_state.fs;
    kernel_state.syscall_context.scheduler = &kernel_state.scheduler;

    zev_fs_write(&kernel_state.fs, "/system/version", version, sizeof(version) - 1u);
    kernel_state.init_pid = zev_process_create(&kernel_state.scheduler, init_name);
    zev_scheduler_tick(&kernel_state.scheduler);

    printf("zevMobile kernel: hardware online\n");
    printf("zevMobile kernel: virtual flash filesystem mounted (%u files)\n",
           zev_fs_file_count(&kernel_state.fs));
    printf("zevMobile kernel: started %s as PID %d\n", init_name, kernel_state.init_pid);
}

void zev_kernel_tick(ZevPhone *phone)
{
    (void)phone;
    zev_scheduler_tick(&kernel_state.scheduler);
}
