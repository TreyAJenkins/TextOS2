#include <kernel/xfs.h>
#include <kernel/partition.h>
#include <kernel/vfs.h>
#include <kernel/ata.h>
#include <sys/types.h>
#include <string.h>
#include <kernel/kernutil.h>
#include <kernel/heap.h>
#include <kernel/installer.h>
#include <stdlib.h>

int install_core(int disk, char* core, uint32 size) {
    ata_device_t *dev = &devices[disk];

    int sectors = size / 512;
    if ((size % 512) == 0) {
        sectors++;
    }


    //printk("Sectors: %i\n", sectors);
    printk("CORE:\t1 - %i\n", sectors);
    assert(sectors < dev->partition[0].start_lba);


    char* newbuf = kmalloc(sectors * 512 * 8);
    for (int i = 0; i < size; i++) {
        newbuf[i] = core[i];
    }

    disk_write(dev, 1, size, newbuf);
    kfree(newbuf);

    return sectors + 2;
}

struct __attribute__((__packed__)) MBR_t {
    char BOOTCODE[436];
    char DISKID[10];
    char PARTITION[64];
    char SIGNATURE[2];
};


int install_mbr(int disk, char* mbr) {
    ata_device_t *dev = &devices[disk];

    struct MBR_t MBR = {0};

    disk_read(dev, 0, 512, &MBR);
    for (int i = 0; i < 436; i++) {
        MBR.BOOTCODE[i] = mbr[i];
    }
    MBR.SIGNATURE[0] = 0x55;
    MBR.SIGNATURE[1] = 0xAA;
    disk_write(dev, 0, 512, &MBR);
}

int install_kernel(int disk, char* kernel, int size, int offset) {
    ata_device_t *dev = &devices[disk];

    int sectors = size / 512;
    if ((size % 512) == 0) {
        sectors++;
    }


    //printk("Sectors: %i\n", sectors);
    printk("KERNEL:\t%i+%i\n", offset, sectors + 1);
    assert(sectors + offset < dev->partition[0].start_lba);


    char* newbuf = kmalloc(sectors * 512 * 8);
    for (int i = 0; i < size; i++) {
        newbuf[i] = kernel[i];
    }

    disk_write(dev, offset, size, newbuf);
    kfree(newbuf);

    return sectors + offset + 1;
}
