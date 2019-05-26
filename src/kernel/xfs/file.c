#include <kernel/xfs.h>
#include <kernel/partition.h>
#include <kernel/vfs.h>
#include <kernel/ata.h>
#include <sys/types.h>
#include <string.h>
#include <kernel/kernutil.h>
#include <kernel/heap.h>
#include <stdlib.h>



struct XFS_MAP;
struct XFS_SECTOR;

uint32 xfs_find_entry(ata_device_t *dev, uint8 part) {
    volatile struct XFS_MAP map = {0};
    disk_read(dev, dev->partition[part].start_lba, 512, &map);
    assert(strncmp(map.MAGIC, "XFS", 3) == 0);
    for (int i = 0; i < 12; i++) {
        //printk("Map %i, Sector %i :: Size: %i, Name: %s\n", map.ID, map.ENTRY[i].SECTOR, map.ENTRY[i].SIZE, map.ENTRY[i].NAME);
        //printk("%i: %i\n", map.ENTRY[i].SECTOR, map.ENTRY[i].SIZE);
        if (map.ENTRY[i].SIZE == NULL) {
            //printk("Found empty sector at %i:%i\n", map.ID, map.ENTRY[i].SECTOR);
            return map.ENTRY[i].SECTOR;
        }
    }
    while (map.NEXT != 0) {
        //printk("MAP.NEXT: %i\n", map.NEXT);
        disk_read(dev, dev->partition[part].start_lba + (map.NEXT * 1*1), 512, &map);
        assert(strncmp(map.MAGIC, "XFS", 3) == 0);
        for (int i = 0; i < 12; i++) {
            if (map.ENTRY[i].SIZE == NULL) {
                //printk("Found empty sector at %i:%i\n", map.ID, map.ENTRY[i].SECTOR);
                return map.ENTRY[i].SECTOR;
            }
        }
    }

    //Add another map

    volatile struct XFS_MAP nmap = {0};
    volatile struct XFS_ENTRY nentry = {0};
    for (int i = 0; i < 12; i++) {
        nmap.ID = map.ID + 13;
        nentry.SECTOR = (i+1) + ((nmap.ID) * 12);
        nmap.ENTRY[i] = nentry;
    }
    memcpy(nmap.MAGIC, "XFS", 3);
    volatile int mr = disk_write(dev, dev->partition[part].start_lba + (nmap.ID), 512, &nmap);
    map.NEXT = nmap.ID;
    assert(nmap.ID != 0);
    volatile int w = disk_write(dev, dev->partition[part].start_lba + (map.ID), 512, &map);
    volatile int r = disk_read(dev, dev->partition[part].start_lba + (map.ID), 512, &map);
    //printk("Created map %i\n", map.NEXT);
    assert(map.NEXT == nmap.ID);
    return (nmap.ID * 12) + 1;
}

int xfs_create_gap(uint8 disk, uint8 part, int size) {
    ata_device_t *dev = &devices[disk];

    int maps = size / 512;
    if ((size % 512) > 0)
        maps++;

    volatile uint32 entryloc = xfs_find_entry(dev, part);
    int secid = entryloc - 1;
    int mapid = (secid - (mapid % 12)) / 12;
    secid %= 12;

    int target = mapid + maps + 1;
    printk("target: %i, size: %i\n", target, size);

    struct XFS_MAP map = {0};
    disk_read(dev, dev->partition[part].start_lba + (mapid), 512, &map);
    map.NEXT = target;
    disk_write(dev, dev->partition[part].start_lba + (mapid), 512, &map);

    volatile struct XFS_MAP nmap = {0};
    volatile struct XFS_ENTRY nentry = {0};
    for (int i = 0; i < 12; i++) {
        nmap.ID = target;
        nentry.SECTOR = (i+1) + ((nmap.ID) * 12);
        nmap.ENTRY[i] = nentry;
    }
    memcpy(nmap.MAGIC, "XFS", 3);
    volatile int mr = disk_write(dev, dev->partition[part].start_lba + (nmap.ID), 512, &nmap);

    return map.ID + 13;
}

bool xfs_exists(uint8 disk, uint8 part, char* name) {
    volatile struct XFS_MAP map = {0};
    disk_read(&devices[disk], devices[disk].partition[disk].start_lba, 512, &map);
    assert(strncmp(map.MAGIC, "XFS", 3) == 0);

    int namelen = (strlen(name));
    assert(namelen < 32);

    bool foundRoot = 0;

    while (!foundRoot) {
        for (int i = 0; i < 12; i++) {
            if (map.ENTRY[i].ROOT) {
                if (strncmp(name, map.ENTRY[i].NAME, namelen) == 0) {
                    foundRoot = 1;
                    return 1;
                }
            }
        }
        if (map.NEXT == NULL && (foundRoot == 0)) {
            //printk("foundRoot: %i\n", foundRoot);
            return 0;
        }
        if (foundRoot == 0) {
            disk_read(&devices[disk], devices[disk].partition[disk].start_lba + (map.NEXT), 512, &map);
        }
    }
    return 0;
}

int xfs_write(uint8 disk, uint8 part, char* name, void* data, uint32 size) {
    ata_device_t *dev = &devices[disk];
    long bytes_left = size;
    uint32 ipos = 0;
    char* buffa = data;

    volatile uint32 entryloc = xfs_find_entry(dev, part);
    volatile uint32 mapid = 0;
    volatile uint32 secid = 0;

    int root = 1;

    while (bytes_left > 0) {
        //printk("ipos: %i\n", ipos);

        //printk("Physical: %i\n", entryloc);
        //printk("Bytes left: %i\n", bytes_left);
        mapid = 0;
        secid = entryloc - 1;

        volatile struct XFS_MAP map;
        disk_read(dev, dev->partition[part].start_lba, 512, &map);
        assert(strncmp(map.MAGIC, "XFS", 3) == 0);

        mapid = (secid - (mapid % 12)) / 12;
        secid %= 12;

        //printk("Mapid: %i, Secid %i\n", mapid, secid);

        int currmap = map.ID;
        while (currmap != mapid) {
            while (map.NEXT == NULL) {
                xfs_find_entry(dev, part);
                disk_read(dev, dev->partition[part].start_lba + (map.ID), 512, &map);
            }
            //assert(map.NEXT != NULL);
            //printk("Next: %i\n", map.NEXT);
            disk_read(dev, dev->partition[part].start_lba + (map.NEXT * 1*1), 512, &map);
            assert(strncmp(map.MAGIC, "XFS", 3) == 0);
            currmap = map.ID;
        }

        //printk("currmap: %i\n", currmap);

        struct XFS_ENTRY entry = {0};

        int btw = 512;

        if (bytes_left < 512) {
            btw = bytes_left;
        }

        char outbuf[512] = {0};
        for (int x = 0; x < btw; x++) {
            outbuf[x] = buffa[(ipos) + x];
        }

        //printk("Copied %i bytes to outbuf\n", btw);

        map.ENTRY[secid].SIZE = btw;
        memcpy(map.ENTRY[secid].NAME, name, 31);

        //printk("Set name as %s\n", map.ENTRY[secid].NAME);

        bytes_left = bytes_left - 512;

        disk_write(dev, dev->partition[part].start_lba + (map.ID), 512, &map);

        if (bytes_left > 0) {
            entryloc = xfs_find_entry(dev, part);
            disk_read(dev, dev->partition[part].start_lba + (map.ID), 512, &map);
            map.ENTRY[secid].NEXT = (short) entryloc;
        }

        if (root == 1) {
            map.ENTRY[secid].ROOT = 1;
            root = 0;
        }

        //printk("Writing to disk\n");
        disk_write(dev, dev->partition[part].start_lba + (map.ID), 512, &map);
        ///printk("Wrote map\n");
        //printk("Writing at %i, secid: %i\n", map.ENTRY[secid].SECTOR, secid);
        disk_write(dev, dev->partition[part].start_lba + (map.ENTRY[secid].SECTOR), 512, &outbuf);
        //printk("Wrote sector\n");

        ipos += 512;
        //printk("ipos: %i\n", ipos);

    }
    return 0;
}

int xfs_read_raw(uint8 disk, uint8 part, char* name, char* data, int entryn) {
    volatile struct XFS_MAP map;
    disk_read(&devices[disk], devices[disk].partition[disk].start_lba, 512, &map);
    assert(strncmp(map.MAGIC, "XFS", 3) == 0);


    bool foundRoot = 0;
    volatile struct XFS_ENTRY xentry = {0};
    int namelen = 32;

    while (!foundRoot) {
        for (int i = 0; i < 12; i++) {
            if (map.ENTRY[i].ROOT) {
                namelen = (strlen(map.ENTRY[i].NAME)) % 32;
                //printk("%s == %s [%i]\n", name, map.ENTRY[i].NAME, namelen);
                if (strncmp(name, map.ENTRY[i].NAME, namelen) == 0 && namelen == (strlen(name))) {
                    foundRoot = 1;
                    xentry = map.ENTRY[i];
                }
            }
        }
        if (map.NEXT == NULL && (foundRoot == 0)) {
            //printk("foundRoot: %i\n", foundRoot);
            return -1;
        }
        if (foundRoot == 0) {
            disk_read(&devices[disk], devices[disk].partition[disk].start_lba + (map.NEXT), 512, &map);
        }
    }
    int mapid = 0;
    int secid = 0;

    for (int i = 0; i < entryn; i++) {
        if (xentry.NEXT == NULL) {
            return 0;
        }

        int entryloc = xentry.NEXT;
        mapid = 0;
        secid = entryloc - 1;

        mapid = (secid - (mapid % 12)) / 12;
        secid %= 12;

        int currmap = map.ID;
        while (currmap != mapid) {
            while (map.NEXT == NULL) {
                xfs_find_entry(&devices[disk], part);
                disk_read(&devices[disk], devices[disk].partition[part].start_lba + (map.ID), 512, &map);
            }
            //assert(map.NEXT == NULL);
            disk_read(&devices[disk], devices[disk].partition[disk].start_lba + (map.NEXT * 1*1), 512, &map);
            assert(strncmp(map.MAGIC, "XFS", 3) == 0);
            currmap = map.ID;
        }
        xentry = map.ENTRY[i];
    }

    //disk_write(dev, dev->partition[part].start_lba + (map.ENTRY[secid].SECTOR), 512, &outbuf);
    //printk("SecID: %i\n", secid);
    //printk("Sector: %i\n", map.ENTRY[secid].SECTOR);
    disk_read(&devices[disk], devices[disk].partition[disk].start_lba + (map.ENTRY[secid].SECTOR), 512, data);

    return map.ENTRY[secid].SIZE;
}

int xfs_read_select(uint8 disk, uint8 part, char* name, void* data, uint32 pos, uint32 size) {
    char *dbuf = kmalloc((size * sizeof(char)) + 512);
    int blocks = (size / 512);
    if ((size % 512) > 0) {
        blocks++;
    }
    int cblock = pos / 512;
    int cpos = pos % 512;
    int dread = 0;
    int bpos = 0;

    for (int i = 0; i < blocks; i++) {
        char rbuf[512] = {0};
        int r = xfs_read_raw(disk, part, name, &rbuf, cblock + i);
        if (r < 0) {
            return -1;
        } else if (r == 0) {
            break;
        }
        for (int v = 0; v < 512; v++) {
            dbuf[(i * 512) + v] = rbuf[v];
            //printk("%c", rbuf[v]);
        }
    }
    char* outbuf = kmalloc(size * sizeof(char));
    for (int i = cpos; i < size+cpos; i++) {
        outbuf[i - cpos] = dbuf[i];
    }
    memcpy(data, outbuf, size);
    kfree(outbuf);
    kfree(dbuf);
    return size;
}

uint32 xfs_find_size(uint8 disk, uint8 part, char* name) {
    int done = 0;
    int pos = 0;
    int size = 0;

    while (!done) {
        char buf[512];
        int status = xfs_read_raw(disk, part, name, &buf, pos);
        if (status < 0)
            return 0;
        if (status == 0)
            done = 1;
        size += status;
        pos++;
        //printk("size: %i\n", size);
    }
    return size;
}


int xfs_test_read_select() {
    char data[7162] = {0};
    int s = xfs_read_select(0, 0, "LOREM1024.txt", &data, 0, 7162);
    printk("LOREM1024.txt [0:5]: %i\n", s);
    for (int i = 0; i < 7162; i++) {
        printk("%c", data[i]);
    }
    //printk("%s\n", data);
    printk("Sizeof returned data: %i\n", strlen(data));
    return 0;
};

int xfs_test_read_raw() {
    char data[512] = {0};
    int s = xfs_read_raw(0, 0, "LOREM1024.txt", &data, 0);
    printk("LOREM1024.txt: %i\n", s);
    printk("%s\n", data);
    return 0;
}

int xfs_test_write() {
    char l1024[] = "Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent eu elit metus. Quisque mollis eget augue sed feugiat. Sed sit amet magna urna. Mauris purus eros, sollicitudin vitae aliquet sed, scelerisque non mi. Quisque dignissim dolor metus, nec aliquam urna egestas sed. Pellentesque vulputate erat sagittis velit placerat congue sed sed metus. Curabitur lectus nunc, congue sit amet fringilla vel, blandit et metus. Curabitur dapibus lacus enim, a interdum lectus scelerisque et. Mauris quis posuere felis, in eleifend lectus. Cras elementum, sem eu tristique pretium, magna libero elementum eros, ut lacinia nibh ex a mauris. Proin arcu dolor, luctus non elit id, fringilla porttitor neque. Sed efficitur dui ipsum, quis fringilla magna fringilla a. Suspendisse interdum tincidunt nunc sed auctor. Etiam convallis pulvinar purus at lobortis. Cras et vehicula ipsum. Quisque bibendum ipsum ex, iaculis fermentum ante viverra at. Suspendisse volutpat pellentesque pellentesque. Vivamus dignissim dui ut convallis sed.Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent eu elit metus. Quisque mollis eget augue sed feugiat. Sed sit amet magna urna. Mauris purus eros, sollicitudin vitae aliquet sed, scelerisque non mi. Quisque dignissim dolor metus, nec aliquam urna egestas sed. Pellentesque vulputate erat sagittis velit placerat congue sed sed metus. Curabitur lectus nunc, congue sit amet fringilla vel, blandit et metus. Curabitur dapibus lacus enim, a interdum lectus scelerisque et. Mauris quis posuere felis, in eleifend lectus. Cras elementum, sem eu tristique pretium, magna libero elementum eros, ut lacinia nibh ex a mauris. Proin arcu dolor, luctus non elit id, fringilla porttitor neque. Sed efficitur dui ipsum, quis fringilla magna fringilla a. Suspendisse interdum tincidunt nunc sed auctor. Etiam convallis pulvinar purus at lobortis. Cras et vehicula ipsum. Quisque bibendum ipsum ex, iaculis fermentum ante viverra at. Suspendisse volutpat pellentesque pellentesque. Vivamus dignissim dui ut convallis sed.Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent eu elit metus. Quisque mollis eget augue sed feugiat. Sed sit amet magna urna. Mauris purus eros, sollicitudin vitae aliquet sed, scelerisque non mi. Quisque dignissim dolor metus, nec aliquam urna egestas sed. Pellentesque vulputate erat sagittis velit placerat congue sed sed metus. Curabitur lectus nunc, congue sit amet fringilla vel, blandit et metus. Curabitur dapibus lacus enim, a interdum lectus scelerisque et. Mauris quis posuere felis, in eleifend lectus. Cras elementum, sem eu tristique pretium, magna libero elementum eros, ut lacinia nibh ex a mauris. Proin arcu dolor, luctus non elit id, fringilla porttitor neque. Sed efficitur dui ipsum, quis fringilla magna fringilla a. Suspendisse interdum tincidunt nunc sed auctor. Etiam convallis pulvinar purus at lobortis. Cras et vehicula ipsum. Quisque bibendum ipsum ex, iaculis fermentum ante viverra at. Suspendisse volutpat pellentesque pellentesque. Vivamus dignissim dui ut convallis sed.Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent eu elit metus. Quisque mollis eget augue sed feugiat. Sed sit amet magna urna. Mauris purus eros, sollicitudin vitae aliquet sed, scelerisque non mi. Quisque dignissim dolor metus, nec aliquam urna egestas sed. Pellentesque vulputate erat sagittis velit placerat congue sed sed metus. Curabitur lectus nunc, congue sit amet fringilla vel, blandit et metus. Curabitur dapibus lacus enim, a interdum lectus scelerisque et. Mauris quis posuere felis, in eleifend lectus. Cras elementum, sem eu tristique pretium, magna libero elementum eros, ut lacinia nibh ex a mauris. Proin arcu dolor, luctus non elit id, fringilla porttitor neque. Sed efficitur dui ipsum, quis fringilla magna fringilla a. Suspendisse interdum tincidunt nunc sed auctor. Etiam convallis pulvinar purus at lobortis. Cras et vehicula ipsum. Quisque bibendum ipsum ex, iaculis fermentum ante viverra at. Suspendisse volutpat pellentesque pellentesque. Vivamus dignissim dui ut convallis sed.Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent eu elit metus. Quisque mollis eget augue sed feugiat. Sed sit amet magna urna. Mauris purus eros, sollicitudin vitae aliquet sed, scelerisque non mi. Quisque dignissim dolor metus, nec aliquam urna egestas sed. Pellentesque vulputate erat sagittis velit placerat congue sed sed metus. Curabitur lectus nunc, congue sit amet fringilla vel, blandit et metus. Curabitur dapibus lacus enim, a interdum lectus scelerisque et. Mauris quis posuere felis, in eleifend lectus. Cras elementum, sem eu tristique pretium, magna libero elementum eros, ut lacinia nibh ex a mauris. Proin arcu dolor, luctus non elit id, fringilla porttitor neque. Sed efficitur dui ipsum, quis fringilla magna fringilla a. Suspendisse interdum tincidunt nunc sed auctor. Etiam convallis pulvinar purus at lobortis. Cras et vehicula ipsum. Quisque bibendum ipsum ex, iaculis fermentum ante viverra at. Suspendisse volutpat pellentesque pellentesque. Vivamus dignissim dui ut convallis sed.Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent eu elit metus. Quisque mollis eget augue sed feugiat. Sed sit amet magna urna. Mauris purus eros, sollicitudin vitae aliquet sed, scelerisque non mi. Quisque dignissim dolor metus, nec aliquam urna egestas sed. Pellentesque vulputate erat sagittis velit placerat congue sed sed metus. Curabitur lectus nunc, congue sit amet fringilla vel, blandit et metus. Curabitur dapibus lacus enim, a interdum lectus scelerisque et. Mauris quis posuere felis, in eleifend lectus. Cras elementum, sem eu tristique pretium, magna libero elementum eros, ut lacinia nibh ex a mauris. Proin arcu dolor, luctus non elit id, fringilla porttitor neque. Sed efficitur dui ipsum, quis fringilla magna fringilla a. Suspendisse interdum tincidunt nunc sed auctor. Etiam convallis pulvinar purus at lobortis. Cras et vehicula ipsum. Quisque bibendum ipsum ex, iaculis fermentum ante viverra at. Suspendisse volutpat pellentesque pellentesque. Vivamus dignissim dui ut convallis sed.Lorem ipsum dolor sit amet, consectetur adipiscing elit. Praesent eu elit metus. Quisque mollis eget augue sed feugiat. Sed sit amet magna urna. Mauris purus eros, sollicitudin vitae aliquet sed, scelerisque non mi. Quisque dignissim dolor metus, nec aliquam urna egestas sed. Pellentesque vulputate erat sagittis velit placerat congue sed sed metus. Curabitur lectus nunc, congue sit amet fringilla vel, blandit et metus. Curabitur dapibus lacus enim, a interdum lectus scelerisque et. Mauris quis posuere felis, in eleifend lectus. Cras elementum, sem eu tristique pretium, magna libero elementum eros, ut lacinia nibh ex a mauris. Proin arcu dolor, luctus non elit id, fringilla porttitor neque. Sed efficitur dui ipsum, quis fringilla magna fringilla a. Suspendisse interdum tincidunt nunc sed auctor. Etiam convallis pulvinar purus at lobortis. Cras et vehicula ipsum. Quisque bibendum ipsum ex, iaculis fermentum ante viverra at. Suspendisse volutpat pellentesque pellentesque. Vivamus dignissim dui ut convallis sed.";
    xfs_write(0, 0, "LOREM1024.txt", &l1024, sizeof(l1024) / sizeof(char));
    return 0;
};

int xfs_test_write_loop() {
    while (1){
        xfs_test_write();
    }
}
int xfs_test_gap() {
    xfs_create_gap(0, 0, 8192);
}
