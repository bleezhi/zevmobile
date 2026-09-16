#include "fs.h"

#include <string.h>

#define FS_HEADER_MAGIC_OFFSET 0u
#define FS_HEADER_COUNT_OFFSET 4u
#define FS_ENTRY_USED 0x01u
#define FS_ENTRY_NAME_OFFSET 0u
#define FS_ENTRY_SIZE_OFFSET 32u
#define FS_ENTRY_SLOT_OFFSET 36u

static uint8_t *entry_ptr(ZevFs *fs, uint32_t index)
{
    return fs->phone->flash + ZEV_FS_HEADER_SIZE + index * ZEV_FS_ENTRY_SIZE;
}

static uint32_t read_u32(const uint8_t *p)
{
    return ((uint32_t)p[0]) |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static void write_u32(uint8_t *p, uint32_t value)
{
    p[0] = (uint8_t)value;
    p[1] = (uint8_t)(value >> 8);
    p[2] = (uint8_t)(value >> 16);
    p[3] = (uint8_t)(value >> 24);
}

static int valid_name(const char *name)
{
    size_t length;
    if (!name || !name[0]) return 0;
    length = strlen(name);
    return length < ZEV_FS_NAME_MAX;
}

static int find_file(const ZevFs *fs, const char *name)
{
    for (uint32_t i = 0; i < ZEV_FS_MAX_FILES; ++i) {
        const uint8_t *entry = fs->phone->flash + ZEV_FS_HEADER_SIZE + i * ZEV_FS_ENTRY_SIZE;
        if ((entry[ZEV_FS_NAME_MAX - 1u] != 0) || entry[0] == 0) continue;
        if (strncmp((const char *)entry + FS_ENTRY_NAME_OFFSET, name, ZEV_FS_NAME_MAX) == 0) {
            return (int)i;
        }
    }
    return -1;
}

int zev_fs_format(ZevFs *fs)
{
    if (!fs || !fs->phone) return -1;

    memset(fs->phone->flash, 0, ZEV_FLASH_SIZE);
    write_u32(fs->phone->flash + FS_HEADER_MAGIC_OFFSET, ZEV_FS_MAGIC);
    write_u32(fs->phone->flash + FS_HEADER_COUNT_OFFSET, 0);
    fs->file_count = 0;
    return 0;
}

int zev_fs_mount(ZevFs *fs, ZevPhone *phone)
{
    if (!fs || !phone) return -1;

    fs->phone = phone;
    if (read_u32(phone->flash + FS_HEADER_MAGIC_OFFSET) != ZEV_FS_MAGIC) {
        return zev_fs_format(fs);
    }

    fs->file_count = read_u32(phone->flash + FS_HEADER_COUNT_OFFSET);
    if (fs->file_count > ZEV_FS_MAX_FILES) return zev_fs_format(fs);
    return 0;
}

int zev_fs_create(ZevFs *fs, const char *name)
{
    if (!fs || !fs->phone || !valid_name(name)) return -1;
    if (find_file(fs, name) >= 0) return 0;

    for (uint32_t i = 0; i < ZEV_FS_MAX_FILES; ++i) {
        uint8_t *entry = entry_ptr(fs, i);
        if (entry[0] != 0) continue;

        memset(entry, 0, ZEV_FS_ENTRY_SIZE);
        memcpy(entry + FS_ENTRY_NAME_OFFSET, name, strlen(name));
        write_u32(entry + FS_ENTRY_SIZE_OFFSET, 0);
        write_u32(entry + FS_ENTRY_SLOT_OFFSET, i);
        fs->file_count++;
        write_u32(fs->phone->flash + FS_HEADER_COUNT_OFFSET, fs->file_count);
        return 0;
    }

    return -1;
}

int zev_fs_exists(ZevFs *fs, const char *name)
{
    return fs && name && find_file(fs, name) >= 0;
}

int zev_fs_write(ZevFs *fs, const char *name, const void *data, size_t size)
{
    int index;
    uint32_t slot;
    uint8_t *entry;
    size_t offset;

    if (!fs || !fs->phone || !data || size > ZEV_FS_FILE_SIZE) return -1;
    if (!valid_name(name)) return -1;

    index = find_file(fs, name);
    if (index < 0) {
        if (zev_fs_create(fs, name) != 0) return -1;
        index = find_file(fs, name);
    }

    if (index < 0) return -1;
    entry = entry_ptr(fs, (uint32_t)index);
    slot = read_u32(entry + FS_ENTRY_SLOT_OFFSET);
    offset = ZEV_FS_DATA_OFFSET + (size_t)slot * ZEV_FS_FILE_SIZE;

    if (offset + size > ZEV_FLASH_SIZE) return -1;
    memset(fs->phone->flash + offset, 0, ZEV_FS_FILE_SIZE);
    memcpy(fs->phone->flash + offset, data, size);
    write_u32(entry + FS_ENTRY_SIZE_OFFSET, (uint32_t)size);
    return 0;
}

int zev_fs_read(ZevFs *fs, const char *name, void *data, size_t capacity, size_t *size_out)
{
    int index;
    uint8_t *entry;
    uint32_t slot;
    uint32_t size;
    size_t offset;

    if (!fs || !fs->phone || !data) return -1;
    index = find_file(fs, name);
    if (index < 0) return -1;

    entry = entry_ptr(fs, (uint32_t)index);
    size = read_u32(entry + FS_ENTRY_SIZE_OFFSET);
    if (size > capacity) return -1;

    slot = read_u32(entry + FS_ENTRY_SLOT_OFFSET);
    offset = ZEV_FS_DATA_OFFSET + (size_t)slot * ZEV_FS_FILE_SIZE;
    if (offset + size > ZEV_FLASH_SIZE) return -1;

    memcpy(data, fs->phone->flash + offset, size);
    if (size_out) *size_out = size;
    return 0;
}

uint32_t zev_fs_file_count(const ZevFs *fs)
{
    return fs ? fs->file_count : 0;
}
