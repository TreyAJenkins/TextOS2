#include <kernel/xfs.h>
#include <kernel/partition.h>
#include <kernel/vfs.h>
#include <kernel/ata.h>
#include <kernel/kernutil.h>

struct XFS_MAP;
struct XFS_ENTRY;

bool xfs_detect(ata_device_t *dev, uint8 part) {
    return false;
}


int xfs_partition(uint8 disk, uint8 part) {
    struct XFS_MAP map = {0};
   //printk("Map size: %i\n", sizeof(struct XFS_MAP));
   //printk("Entry size: %i\n", sizeof(struct XFS_ENTRY));
    memcpy(map.MAGIC, "XFS", 3);
    for (int i = 0; i < 12; i++) {
        struct XFS_ENTRY entry = {0};
        entry.SECTOR = i+1;
        entry.CHECK = 1;
        map.ENTRY[i] = entry;
        //memcpy(map.ENTRY[i], entry, sizeof(struct XFS_ENTRY));
       //printk("SECTOR %i, PHY: %i\n", map.ENTRY[i].SECTOR, i);
    }

    assert(disk_write(&devices[disk], devices[disk].partition[part].start_lba, 512, &map));
   //printk("Partitioned Disk%i/%i successfully\n", disk, part);
    return 0;
}

int xfs_format(uint8 disk, uint8 part) {
    printk("Formatting Disk%i/%i (%i sectors) ", disk, part, devices[disk].partition[part].total_sectors - devices[disk].partition[part].start_lba);
    char blank[512] = {1};
    for (uint32 i = 0; i < devices[disk].partition[part].total_sectors; i++) {
        disk_write(&devices[disk], devices[disk].partition[part].start_lba + i, 512, &blank);
    }
    printk("\n");
   //printk("done\n");
    xfs_partition(disk, part);
}

int xfs_test_format() {
    xfs_format(0, 0);
}

int xfs_dump(uint8 disk, uint8 part) {
    uint32 usage = 0;
    uint32 actual = 0;
    struct XFS_MAP map = {0};
    disk_read(&devices[disk], devices[disk].partition[part].start_lba, 512, &map);
    assert(strncmp(map.MAGIC, "XFS", 3) == 0);
    printk("Start LBA: %i\n", devices[0].partition[0].start_lba);
    usage += 512;
    actual = 0;
    int maps = 1;
    for (int i = 0; i < 12; i++) {
        usage += map.ENTRY[i].SIZE;
        printk("Map %i, Sector %i :: Size: %i, Name: %s\n", map.ID, map.ENTRY[i].SECTOR, map.ENTRY[i].SIZE, map.ENTRY[i].NAME);
        assert(map.ENTRY[i].CHECK == 1);
    }
    while (map.NEXT != 0) {
        disk_read(&devices[disk], devices[disk].partition[part].start_lba + (map.NEXT * 1*1), 512, &map);
        assert(strncmp(map.MAGIC, "XFS", 3) == 0);
        for (int i = 0; i < 12; i++) {
            usage += map.ENTRY[i].SIZE;
            printk("Map #%i, Sector %i :: Size: %i, Name: %s\n", map.ID/13, map.ENTRY[i].SECTOR, map.ENTRY[i].SIZE, map.ENTRY[i].NAME);
        }
        actual++;
    }
    printk("Usage: %i\nActual:%i\n", usage, (actual*512)+usage);
}

int xfs_test_partition() {
    xfs_partition(0, 0);
    //xfs_test_dump();
};
