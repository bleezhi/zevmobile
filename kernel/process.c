#include "process.h"

#include <string.h>

static int find_slot(const ZevScheduler *scheduler)
{
    for (uint32_t i = 0; i < ZEV_MAX_PROCESSES; ++i) {
        if (scheduler->table[i].state == ZEV_PROCESS_UNUSED ||
            scheduler->table[i].state == ZEV_PROCESS_EXITED) {
            return (int)i;
        }
    }
    return -1;
}

void zev_scheduler_init(ZevScheduler *scheduler)
{
    memset(scheduler, 0, sizeof(*scheduler));
    scheduler->next_pid = 1;
    scheduler->current = -1;
}

int zev_process_create(ZevScheduler *scheduler, const char *name)
{
    int slot;
    ZevProcess *process;

    if (!scheduler || !name || !name[0]) return -1;
    slot = find_slot(scheduler);
    if (slot < 0) return -1;

    process = &scheduler->table[slot];
    memset(process, 0, sizeof(*process));
    process->pid = scheduler->next_pid++;
    process->state = ZEV_PROCESS_READY;
    strncpy(process->name, name, ZEV_PROCESS_NAME_MAX - 1u);
    process->name[ZEV_PROCESS_NAME_MAX - 1u] = '\0';
    return (int)process->pid;
}

int zev_process_exit(ZevScheduler *scheduler, uint32_t pid, int exit_code)
{
    if (!scheduler || pid == 0) return -1;

    for (uint32_t i = 0; i < ZEV_MAX_PROCESSES; ++i) {
        ZevProcess *process = &scheduler->table[i];
        if (process->pid != pid || process->state == ZEV_PROCESS_UNUSED) continue;
        process->state = ZEV_PROCESS_EXITED;
        process->exit_code = exit_code;
        if (scheduler->current == (int)i) scheduler->current = -1;
        return 0;
    }
    return -1;
}

void zev_scheduler_tick(ZevScheduler *scheduler)
{
    int next = -1;

    if (!scheduler) return;
    scheduler->scheduler_ticks++;

    if (scheduler->current >= 0 &&
        scheduler->table[scheduler->current].state == ZEV_PROCESS_RUNNING) {
        scheduler->table[scheduler->current].state = ZEV_PROCESS_READY;
    }

    for (uint32_t offset = 1; offset <= ZEV_MAX_PROCESSES; ++offset) {
        uint32_t index;
        if (scheduler->current < 0) {
            index = (offset - 1u) % ZEV_MAX_PROCESSES;
        } else {
            index = ((uint32_t)scheduler->current + offset) % ZEV_MAX_PROCESSES;
        }
        if (scheduler->table[index].state == ZEV_PROCESS_READY) {
            next = (int)index;
            break;
        }
    }

    if (next >= 0) {
        scheduler->current = next;
        scheduler->table[next].state = ZEV_PROCESS_RUNNING;
        scheduler->table[next].ticks++;
    } else {
        scheduler->current = -1;
    }
}

const ZevProcess *zev_process_current(const ZevScheduler *scheduler)
{
    if (!scheduler || scheduler->current < 0) return NULL;
    return &scheduler->table[scheduler->current];
}

uint32_t zev_process_count(const ZevScheduler *scheduler)
{
    uint32_t count = 0;
    if (!scheduler) return 0;

    for (uint32_t i = 0; i < ZEV_MAX_PROCESSES; ++i) {
        if (scheduler->table[i].state != ZEV_PROCESS_UNUSED &&
            scheduler->table[i].state != ZEV_PROCESS_EXITED) {
            count++;
        }
    }
    return count;
}
