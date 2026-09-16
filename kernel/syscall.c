#include "syscall.h"

#include <stddef.h>

int32_t zev_syscall(ZevSyscallContext *context, uint32_t number,
                    uintptr_t arg0, uintptr_t arg1, uintptr_t arg2)
{
    if (!context) return -1;

    switch (number) {
    case ZEV_SYS_NOP:
        return 0;

    case ZEV_SYS_UPTIME:
        return context->phone ? (int32_t)context->phone->ticks : -1;

    case ZEV_SYS_GETKEY:
        if (!context->phone) return -1;
        return (int32_t)context->phone->last_key;

    case ZEV_SYS_FILE_EXISTS:
        if (!context->fs || !arg0) return -1;
        return zev_fs_exists(context->fs, (const char *)arg0);

    case ZEV_SYS_FILE_READ: {
        size_t size = 0;
        int result;
        if (!context->fs || !arg0 || !arg1) return -1;
        result = zev_fs_read(context->fs, (const char *)arg0,
                             (void *)arg1, (size_t)arg2, &size);
        return result == 0 ? (int32_t)size : -1;
    }

    case ZEV_SYS_FILE_WRITE:
        if (!context->fs || !arg0 || !arg1) return -1;
        return zev_fs_write(context->fs, (const char *)arg0,
                            (const void *)arg1, (size_t)arg2);

    case ZEV_SYS_PROCESS_COUNT:
        return context->scheduler ? (int32_t)zev_process_count(context->scheduler) : -1;

    default:
        return -1;
    }
}
