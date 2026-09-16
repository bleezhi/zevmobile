#ifndef ZEV_FS_H
#define ZEV_FS_H

#include <stddef.h>
#include <stdint.h>

#include "../emulator/phone.h"

#define ZEV_FS_MAGIC 0x5A455646u /* "ZEVF" */
#define ZEV_FS_MAX_FILES 32u
#define ZEV_FS_NAME_MAX 32u
#define ZEV_FS_FILE_SIZE 2048u
#define ZEV_FS_HEADER_SIZE 8u
#define ZEV_FS_ENTRY_SIZE 40u
#define ZEV_FS_DATA_OFFSET (ZEV_FS_HEADER_SIZE + (ZEV_FS_MAX_FILES * ZEV_FS_ENTRY_SIZE))

typedef struct {
    char name[ZEV_FS_NAME_MAX];
    uint32_t size;
    uint32_t slot;
} ZevFsFile;

typedef struct {
    ZevPhone *phone;
    uint32_t file_count;
} ZevFs;

int zev_fs_mount(ZevFs *fs, ZevPhone *phone);
int zev_fs_format(ZevFs *fs);
int zev_fs_create(ZevFs *fs, const char *name);
int zev_fs_write(ZevFs *fs, const char *name, const void *data, size_t size);
int zev_fs_read(ZevFs *fs, const char *name, void *data, size_t capacity, size_t *size_out);
int zev_fs_exists(ZevFs *fs, const char *name);
uint32_t zev_fs_file_count(const ZevFs *fs);

#endif
