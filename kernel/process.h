#ifndef ZEV_PROCESS_H
#define ZEV_PROCESS_H

#include <stdint.h>

#define ZEV_MAX_PROCESSES 16u
#define ZEV_PROCESS_NAME_MAX 24u

typedef enum {
    ZEV_PROCESS_UNUSED = 0,
    ZEV_PROCESS_READY,
    ZEV_PROCESS_RUNNING,
    ZEV_PROCESS_SLEEPING,
    ZEV_PROCESS_EXITED
} ZevProcessState;

typedef struct {
    uint32_t pid;
    char name[ZEV_PROCESS_NAME_MAX];
    ZevProcessState state;
    uint64_t ticks;
    int exit_code;
} ZevProcess;

typedef struct {
    ZevProcess table[ZEV_MAX_PROCESSES];
    uint32_t next_pid;
    int current;
    uint64_t scheduler_ticks;
} ZevScheduler;

void zev_scheduler_init(ZevScheduler *scheduler);
int zev_process_create(ZevScheduler *scheduler, const char *name);
int zev_process_exit(ZevScheduler *scheduler, uint32_t pid, int exit_code);
void zev_scheduler_tick(ZevScheduler *scheduler);
const ZevProcess *zev_process_current(const ZevScheduler *scheduler);
uint32_t zev_process_count(const ZevScheduler *scheduler);

#endif
