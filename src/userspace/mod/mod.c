#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

uint32 func_to_addr(char* name, int space) {
  uint32 a;
  asm volatile("int $0x80" : "=a" (a) : "0" (44), "b" ((int)name), "c" ((int)space));
  return a;
}

size_t (*printk)(const char* fmt, ...);
//size_t printk(const char *fmt, ...)
//uint32 func_to_addr(char* name, int space)


int kmod_init() {
    printk = (size_t (*)(size_t))func_to_addr("printk", 0);
    printk("test\n");
}

uint32 main() {
    kmod_init();
    return 0;
}
