#ifndef _KERNEL_VESA_H
#define _KERNEL_VESA_H
#include <sys/types.h>
#include <kernel/multiboot.h>

#define VESA_MAGIC "VESA"


typedef struct vesa_control_info {
    char magic[4]; // "VESA"
    unsigned int version;
    unsigned int oem_string_offset;
    unsigned int oem_string_seg;
    unsigned int capabilities;
    unsigned int video_mode_offset;
    unsigned int video_mode_seg;
    unsigned int total_memory;
} __attribute__((packed)) vesa_control_info_t;


typedef struct vesa_mode_info
{
  uint16 attr;
  uint8 win_a, win_b;
  uint16 win_granularity;
  uint16 win_size;
  uint16 seg_a, seg_b;
  uint32 real_mode_func_ptr;
  uint16 pitch; // bytes per scanline

  uint16 x_res, y_res; // width, height
  uint8 w_char, y_char, planes, bpp, banks;
  uint8 memory_model, bank_size, image_pages;
  uint8 reserved0;

  uint8 red_mask, red_position;
  uint8 green_mask, green_position;
  uint8 blue_mask, blue_position;
  uint8 rsv_mask, rsv_position;
  uint8 directcolor_attr;

  uint32 base;  // framebuffer physical address
  uint32 reserved1;
  uint16 reserved2;
  uint8 reserved[206];
} __attribute__((packed)) vesa_mode_info_t;

void vesa_init(uint32 control, uint16 mode, uint32 info);
void vesa_stage2();
#endif
