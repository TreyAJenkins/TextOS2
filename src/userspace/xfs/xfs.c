#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

int dropbox(char* method, char* data) {
  int a;
  asm volatile("int $0x80" : "=a" (a) : "0" (32), "b" ((int)method), "c" ((int)data));
  return a;
}
int xfs_dump(int disk, int part) {
  int a;
  asm volatile("int $0x80" : "=a" (a) : "0" (40), "b" ((int)disk), "c" ((int)part));
  return a;
}
uint32 pmm_bytes_free() {
  int a;
  asm volatile("int $0x80" : "=a" (a) : "0" (37));
  return a;
}
//int xfs_read_select(uint8 disk, uint8 part, char* name, void* data, uint32 pos, uint32 size) {
//    int a;
//    asm volatile("int $0x80" : "=a" (a) : "0" (35), "b" (disk), "c" (part), "d", (name), );
//}

int xfs_read_raw(uint8 disk, uint8 part, char* name, char* data, int entryn) {
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (34), "b" (disk), "c" (part), "d" (name), "S" (data), "D" (entryn));
    return a;
}

int xfs_write(uint8 disk, uint8 part, char* name, void* data, uint32 size) {
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (33), "b" (disk), "c" (part), "d" (name), "S" (data), "D" (size));
    return a;
}
int xfs_partition(uint8 disk, uint8 part) {
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (36), "b" (disk), "c" (part));
    return a;
}
uint32 xfs_find_size(uint8 disk, uint8 part, char* name) {
    int a;
    asm volatile("int $0x80" : "=a" (a) : "0" (39), "b" (disk), "c" (part), "d" (name));
    return a;
}

int xfs_read_select(uint8 disk, uint8 part, char* name, void* data, uint32 pos, uint32 size) {
    char *dbuf = malloc((size * sizeof(char)) + 512);
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
        //printf("i: %i\n", i);
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
    char* outbuf = malloc(size * sizeof(char));
    for (int i = cpos; i < size+cpos; i++) {
        outbuf[i - cpos] = dbuf[i];
    }
    memcpy(data, outbuf, size);
    free(outbuf);
    free(dbuf);
    return size;
}

int diskFromName(char* name) {
    if (strlen(name) != 7) {
        return -1;
    }
    if (strncmp(name, "Disk", 4) != 0 && strncmp(name, "disk", 4) != 0) {
        return -1;
    }

    char x[] = {name[4], 0};

    return atoi(x);
}
int partFromName(char* name) {
    if (strlen(name) != 7) {
        return -1;
    }
    if (strncmp(name, "Disk", 4) != 0 && strncmp(name, "disk", 4) != 0) {
        return -1;
    }

    char x[] = {name[6], 0};
    return atoi(x);
}

int touch(int argc, char const *argv[]) {
    if (argc < 4) {
        printf("Usage: XFS touch [device] [file]\n");
        return 1;
    }
    if (diskFromName(argv[2]) < 0 || partFromName(argv[2]) < 0) {
        printf("Invalid device identifier, must be in format Disk[disk]/[partition]\n");
        return 2;
    }
    //printf("Disk%i/%i\n", diskFromName(argv[2]), 0);
    return xfs_write(diskFromName(argv[2]), partFromName(argv[2]), argv[3], "\0", 1);
}

int read_all(int argc, char const *argv[]) {
    if (argc < 4) {
        printf("Usage: XFS read [device] [file]\n");
        return 1;
    }
    if (diskFromName(argv[2]) < 0 || partFromName(argv[2]) < 0) {
        printf("Invalid device identifier, must be in format Disk[disk]/[partition]\n");
        return 2;
    }
    //printf("Disk%i/%i\n", diskFromName(argv[2]), 0);
    uint32 size = xfs_find_size(diskFromName(argv[2]), partFromName(argv[2]), argv[3]);
    //char* data = xfs_read_all(diskFromName(argv[2]), partFromName(argv[2]), argv[3]);
    //printf("data: %p\n", data);
    if (size == 0) {
        printf("File non-existant\n");
        return 1;
    }

    //printf("Size %i\n", size);
    char* data = malloc(size);
    xfs_read_select(diskFromName(argv[2]), partFromName(argv[2]), argv[3], data, 0, size);
    for (int i = 0; i < size; i++) {
        printf("%c", data[i]);
    }
    printf("\n");
    //return xfs_write(diskFromName(argv[2]), partFromName(argv[2]), argv[3], "\0", 1);
}

int clone(int argc, const char* argv[]) {
    if (argc < 5) {
        printf("Usage: XFS clone [device] [source] [destination]\n");
        return 1;
    }
    if (diskFromName(argv[2]) < 0 || partFromName(argv[2]) < 0) {
        printf("Invalid device identifier, must be in format Disk[disk]/[partition]\n");
        return 2;
    }

    FILE *file;
    file = fopen(argv[3], "rb");
    if (file == NULL) {
        printf("Could not find '%s'\n", argv[3]);
        return 1;
    }
    struct stat st;
    stat(argv[3], &st);
    int size = st.st_size;

    printf("Loading %s into memory (%i KB)\n", argv[3], size/1024);
    if (size > pmm_bytes_free()) {
        printf("Failed, not enough memory (Free: %i KB, Required: %i KB)\n", pmm_bytes_free()/1024, size/1024);
        return 4;
    }

    char* buf = malloc(sizeof(char) * size);

    fread(buf, size, 1, file);
    fclose(file);

    printf("Writing to disk\n");
    int v = xfs_write(diskFromName(argv[2]), partFromName(argv[2]), argv[4], buf, size);
    free(buf);
    if (v == 0) {
        printf("%s -> %s\n", argv[3], argv[4]);
        return 0;
    } else if (v == 1) {
        printf("ERROR, FILE EXISTS\n");
        return v;
    } else {
        printf("ERROR %i\n", v);
        return v;
    }
}

int main(int argc, char const *argv[]) {
    if (argc == 1) {
        printf("Usage: XFS [method] ...\nMethods:\n");
        printf("touch\tdump\tpartition\n");
        printf("clone\tread\n");
        printf("\n");
        return 0;
    }
    if (strncmp("touch", argv[1], 5) == 0) {
        return touch(argc, argv);
    } else if (strncmp("read", argv[1], 5) == 0) {
        return read_all(argc, argv);
    } else if (strncmp("dump", argv[1], 3) == 0) {
        if (argc < 3) {
            printf("Usage: XFS dump [device]\n");
            return 1;
        }
        if (diskFromName(argv[2]) < 0 || partFromName(argv[2]) < 0) {
            printf("Invalid device identifier, must be in format Disk[disk]/[partition]\n");
            return 2;
        }
        return xfs_dump(diskFromName(argv[2]), partFromName(argv[2]));
    } else if (strncmp("partition", argv[1], 3) == 0) {
        if (argc < 3) {
            printf("Usage: XFS partition [device]\n");
            return 1;
        }
        if (diskFromName(argv[2]) < 0 || partFromName(argv[2]) < 0) {
            printf("Invalid device identifier, must be in format Disk[disk]/[partition]\n");
            return 2;
        }
        return xfs_partition(diskFromName(argv[2]), partFromName(argv[2]));
    } else if (strncmp("clone", argv[1], 3) == 0) {
        return clone(argc, argv);
    }

	return 0;
}
