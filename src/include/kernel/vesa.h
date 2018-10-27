#ifndef _KERNEL_VESA_H
#define _KERNEL_VESA_H
#include <sys/types.h>
#include <kernel/multiboot.h>
#include <kernel/vga_font.h>
#include <kernel/console.h>

#define VESA_MAGIC "VESA"
#define VESA_HEIGHT 600
#define VESA_WIDTH 800

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

#define BLACK vesa_device_color(MKCOLOR(0, 0, 0))
#define BLUE vesa_device_color(MKCOLOR(0, 0, 168))
#define GREEN vesa_device_color(MKCOLOR(0, 168, 0))
#define CYAN vesa_device_color(MKCOLOR(0, 168, 168))
#define RED vesa_device_color(MKCOLOR(168, 0, 0))
#define MAGENTA vesa_device_color(MKCOLOR(168, 0, 168))
#define BROWN vesa_device_color(MKCOLOR(168, 87, 0))
#define LIGHT_GREY vesa_device_color(MKCOLOR(168, 168, 168))
#define DARK_GREY vesa_device_color(MKCOLOR(87, 87, 87))
#define LIGHT_BLUE vesa_device_color(MKCOLOR(87, 87, 255))
#define LIGHT_GREEN vesa_device_color(MKCOLOR(87, 255, 87))
#define LIGHT_CYAN vesa_device_color(MKCOLOR(87, 255, 255))
#define LIGHT_RED vesa_device_color(MKCOLOR(255, 87, 87))
#define LIGHT_MAGENTA vesa_device_color(MKCOLOR(255, 87, 255))
#define LIGHT_BROWN vesa_device_color(MKCOLOR(255, 255, 87))
#define WHITE vesa_device_color(MKCOLOR(255, 255, 255))

vesa_control_info_t vesa_control_info;
uint16 vesa_mode;
vesa_mode_info_t vesa_mode_info;
int vesa_width;
int vesa_height;

typedef uint32 color_t;
#define MKCOLOR(r, g, b) (((color_t)((r) & 0xff) << 16) | ((color_t)((g) & 0xff) << 8) | ((color_t)((b) & 0xff)))
#define COLOR_BLACK MKCOLOR(0, 0, 0)
#define COLOR_WHITE MKCOLOR(0xff, 0xff, 0xff)
#define COLOR_R(color) (((color) >> 16) & 0xff)
#define COLOR_G(color) (((color) >> 8) & 0xff)
#define COLOR_B(color) (((color)) & 0xff)
#define COLOR_RGB(color, r, g, b) \
    do \
    { \
        r = COLOR_R(color); \
        g = COLOR_G(color); \
        b = COLOR_B(color); \
    } \
    while (0)
#define VESA_CHAR_WIDTH (VGA_FONT_WIDTH + 1)
#define VESA_CHAR_HEIGHT (VGA_FONT_HEIGHT)

extern bool vesa_available;
uint32 vesa_fb_loc;

color_t VESABG;
color_t VESAFG;

void vesa_scroll(void);
void vcursor_left(void);
void setColor(color_t fg, color_t bg);
uint32 vesa_device_color(color_t color);
void vesa_clear(color_t color);
void vesa_fill_char(size_t x0, size_t y0, uint8 ch, color_t fc, color_t bc);
void vesa_fill_char_scale(size_t x0, size_t y0, uint8 ch,
                         size_t scale, color_t fc, color_t bc);
void vesa_fill_char_transparent(size_t x0, size_t y0, uint8 ch, color_t fc);
void vesa_fill_char_scale_transparent(size_t x0, size_t y0, uint8 ch,
                                      size_t scale, color_t fc);
void vesa_fill_rect(size_t x0, size_t y0, size_t width, size_t height, color_t color);
void vesa_draw_hline(size_t x0, size_t y0, size_t width, color_t color);
void vesa_draw_vline(size_t x0, size_t y0, size_t height, color_t color);
void vesa_draw_button_rect(size_t x0, size_t y0, size_t width, size_t height, bool pushed);
void vesa_fill_string(size_t x0, size_t y0, size_t width, const char *str, color_t fc);
void vesa_tty_set_char(size_t x, size_t y, size_t z, char ch, color_t fgcolor, color_t bgcolor);
void vesa_secret_set_char(size_t x, size_t y, char ch, color_t fgcolor, color_t bgcolor);
void vputs_status(int x, const char *str, color_t fg, color_t bg);
//void vesa_draw_menu(struct menu_t menu, int x, int y);
void update_statusbar(void);
void statusd(void);
void vesa_draw_box(size_t x0, size_t y0, size_t width0, size_t height0, color_t color, bool raw);
void vesa_test(void);
void vesa_draw_logo(void);

int vputchar(char c);
void vesa_init(multiboot_info_t *mbd);
void vesa_stage2(void);
void vesa_reset(void);

bool inTextMode;
bool fullscreen;


#endif
