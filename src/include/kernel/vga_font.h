#ifndef _KERNEL_VGA_FONT_H
#define _KERNEL_VGA_FONT_H
#include <sys/types.h>

#define VGA_FONT_WIDTH 8
#define VGA_FONT_HEIGHT 16

extern uint8 vga_font[256 * 16];

#define vga_font_get(ch) (&vga_font[(ch) * 16])

#endif
