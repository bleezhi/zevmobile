#ifndef ZEV_KERNEL_H
#define ZEV_KERNEL_H

#include "../emulator/phone.h"
#include "fs.h"
#include "process.h"
#include "syscall.h"

typedef struct {
    ZevFs fs;
    ZevScheduler scheduler;
    ZevSyscallContext syscall_context;
    int init_pid;
} ZevKernel;

void zev_kernel_boot(ZevPhone *phone);
void zev_kernel_tick(ZevPhone *phone);
const ZevKernel *zev_kernel_state(void);

#endif
