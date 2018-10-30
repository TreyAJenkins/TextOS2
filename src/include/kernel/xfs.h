#ifndef _CONSOLE_H
#define _CONSOLE_H
#include <kernel/partition.h>
#include <kernel/vfs.h>
#include <kernel/ata.h>
#include <sys/types.h>

struct __attribute__((__packed__)) XFS_ENTRY {
    short SECTOR; // 2 Bytes
    uint32 SIZE;   // 4 Bytes
    short NEXT; // 2 Bytes
    bool ROOT; // 1 Byte
    bool CHECK; // 1 Byte
    char NAME[32]; // 32 Bytes
}; // 42 Bytes

struct __attribute__((__packed__)) XFS_MAP  {
    char MAGIC[3]; // 3 Bytes
    short ID; // 2 Bytes
    short NEXT; // 2 Bytes
    char padding[1]; // 1 Byte
    struct XFS_ENTRY ENTRY[12] // 504 Bytes
}; // 512 Bytes



bool xfs_detect(ata_device_t *dev, uint8 part);
int xfs_partition(uint8 disk, uint8 part);
int xfs_read_select(uint8 disk, uint8 part, char* name, void* data, uint32 pos, uint32 size);
int xfs_read_raw(uint8 disk, uint8 part, char* name, char* data, int entryn);
int xfs_write(uint8 disk, uint8 part, char* name, void* data, uint32 size);


#endif
