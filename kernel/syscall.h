#ifndef ZEV_SYSCALL_H
#define ZEV_SYSCALL_H

#include <stdint.h>

#include "../emulator/phone.h"
#include "fs.h"
#include "process.h"

typedef enum {
    ZEV_SYS_NOP = 0,
    ZEV_SYS_UPTIME = 1,
    ZEV_SYS_GETKEY = 2,
    ZEV_SYS_FILE_EXISTS = 3,
    ZEV_SYS_FILE_READ = 4,
    ZEV_SYS_FILE_WRITE = 5,
    ZEV_SYS_PROCESS_COUNT = 6
} ZevSyscallNumber;

typedef struct {
    ZevPhone *phone;
    ZevFs *fs;
    ZevScheduler *scheduler;
} ZevSyscallContext;

int32_t zev_syscall(ZevSyscallContext *context, uint32_t number,
                    uintptr_t arg0, uintptr_t arg1, uintptr_t arg2);

#endif
