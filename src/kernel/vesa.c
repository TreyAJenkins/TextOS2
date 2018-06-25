#include <kernel/vesa.h>
#include <sys/types.h>
#include <kernel/vmm.h>
#include <kernel/multiboot.h>
#include <string.h>
#include <kernel/console.h>
#include <kernel/kernutil.h>
#include <kernel/hostinfo.h>

#define STRINGIFY(x) #x
#define MACRO(x)     STRINGIFY(x)

void *vesa_framebuffer;
bool vesa_available = true; //TODO: Actually check
int vesa_pages;

bool inTextMode = true;


int cursorx = 0;
int cursory = 1;

bool kernel_paniced = false;


static uint32 vesa_tty_color[16];

void setColor(color_t fg, color_t bg) {
    VESAFG = fg;
    VESABG = bg;
}
struct colored_char_t {
    char c;
    color_t fg;
    color_t bg;
    bool enabled;
    bool dirty;
};

struct colored_char_t vesa_screen[VESA_HEIGHT][VESA_WIDTH];



void empty_screen() {
    for (int y = 0; y < vesa_height; y++) {
        for (int x = 0; x < vesa_width; x++) {
            vesa_screen[y][x].c = ' ';
            if (y == 0) {
                vesa_screen[y][x].bg = BLUE;
                vesa_screen[y][x].fg = WHITE;
                vesa_screen[y][x].enabled = true;
                vesa_screen[y][x].dirty = false;
            } else {
                vesa_screen[y][x].bg = VESABG;
                vesa_screen[y][x].fg = VESABG;
                vesa_screen[y][x].enabled = false;
                vesa_screen[y][x].dirty = false;
            }

        }
    }
}


void vesa_init(multiboot_info_t *mbd) {
    empty_screen();

    uint32 vbe_control_info_p = mbd->vbe_control_info;
    uint16 vbe_mode_p = mbd->vbe_mode;
    uint32 vbe_mode_info_p = mbd->vbe_mode_info;

    VESAFG = COLOR_WHITE;
    VESABG = COLOR_BLACK;
    cursorx = 0;
    cursory = 1;

    vesa_control_info_t *mb_control_info = (vesa_control_info_t*) vbe_control_info_p;
    //printk("VESA Magic:  %s\n", mb_control_info->magic);

    memcpy(&vesa_control_info, mb_control_info, sizeof(vesa_control_info_t));

    vesa_mode_info_t *mb_mode_info = (vesa_mode_info_t*) vbe_mode_info_p;

    memcpy(&vesa_mode_info, mb_mode_info, sizeof(vesa_mode_info_t));

    vesa_framebuffer = (void *)vesa_mode_info.base;
    vesa_fb_loc = mb_mode_info->base;
    uint32 vesa_fb_end = (vesa_fb_loc + (vesa_mode_info.pitch * vesa_mode_info.y_res) + (vesa_mode_info.bpp * vesa_mode_info.x_res)) / 8;

    //printk("Framebuffer: 0x%p\n", vesa_fb_loc); //0xFD000000
    int fbsize = (vesa_mode_info.pitch * vesa_mode_info.y_res) + (vesa_mode_info.bpp * vesa_mode_info.x_res) / 8;
    vesa_width = vesa_mode_info.x_res / VESA_CHAR_WIDTH;
    vesa_height = vesa_mode_info.y_res / VESA_CHAR_HEIGHT;

    //vesa_screen = (struct colored_char_t*) kmalloc(vesa_width * vesa_height * sizeof(struct colored_char_t));

    //printk("FrameB Size: 0x%p\n", fbsize);
    vesa_pages = (fbsize / 4096) + 1;
    //printk("TextOS VESA driver active\n");
    printk("VESA Pages: %i\n", vesa_pages);
    printk("Resolution: %ix%i\n", vesa_mode_info.x_res, vesa_mode_info.y_res);

    //for (;;);
}

void vesa_stage2() {
    for (int i = 0; i < vesa_pages; i++) {
        //printk("%i) %p -> %p\n", i, (0xFD000000 + (4096*i)), (0xFD000000 + (4096*i)));
        vmm_map_kernel((0xFD000000 + (4096*i)), (0xFD000000 + (4096*i)), PAGE_RW);
        //_vmm_map((0xa000 + (4096*i)), (0xFD000000 + (4096*i)), kernel_directory, true, PAGE_RW);
        //vesa_redraw();
    }
    //vputchar('A');
    //printk("\n[0][0] = 0x%p\n", vesa_screen[0][0].enabled);
    //vputchar('\n');
    //vputchar('B');
    //vesa_redraw();
    //vesa_scroll();
    //vesa_clear(COLOR_BLACK);

    //vesa_redraw();


}


int vputchar(char c) {
    if (!inTextMode) {
        return 1;
    }

    if (c == '\n') {
        cursorx = 0;
        cursory++;
    } else if (c == '\t') {
        printk("    ");
    } else if (c == 0x08) { //BKSP
        if (cursorx > 0)
            cursorx = cursorx-1;
        else {
            if (cursory > 0) {
                cursory--;
                cursorx = vesa_width;
            }
        }
        //vesa_tty_set_char(cursorx, cursory, ' ', VESABG, VESABG);
    } else if (c >= 0x20) {
        if (cursory > vesa_height) {
            //printk("cursorY deviated: %i\n", cursory);
            cursory = vesa_height;
        }
        if (cursorx > vesa_width) {
            //printk("cursorX deviated: %i\n", cursorx);
            cursorx = vesa_width;
        }
        vesa_tty_set_char(cursorx, cursory, c, VESAFG, VESABG);

        if (cursorx + 1 == vesa_width) {
            cursory++;
            cursorx = 0;
        } else {
            cursorx++;
        }
    }
    if (cursory + 1 == vesa_height) {
        vesa_scroll();
    }
    //vesa_redraw();
}


inline uint32 vesa_device_color(color_t color)
{
    uint32 r, g, b;
    COLOR_RGB(color, r, g, b);
    r = ((r << vesa_mode_info.red_mask) >> 8) << vesa_mode_info.red_position;
    g = ((g << vesa_mode_info.green_mask) >> 8) << vesa_mode_info.green_position;
    b = ((b << vesa_mode_info.blue_mask) >> 8) << vesa_mode_info.blue_position;
    return r | g | b;
}

#define OFFSET(fb, x0, y0) \
    ((uint8 *)fb + vesa_mode_info.pitch * (y0) + vesa_mode_info.bpp * (x0) / 8)

#define SET_COLOR(lfb, color) \
    do \
    { \
        if (vesa_mode_info.bpp == 32) \
        { \
            uint32 *lfb32 = (uint32 *)lfb; \
            *lfb32 = color; \
            lfb += 4; \
        } \
        else if (vesa_mode_info.bpp == 24) \
        { \
            uint32 *lfb32 = (uint32 *)lfb; \
            *lfb32 = (*lfb32 & 0xff000000) | (color & 0xffffff); \
            lfb += 3; \
        } \
        else /* 15, 16 */ \
        { \
            uint16 *lfb16 = (uint16 *)lfb; \
            *lfb16 = (uint16)(color & 0xffff); \
            lfb += 2; \
        } \
    } while (0)

#define KEEP_COLOR(lfb) \
    do \
    { \
        lfb += vesa_mode_info.bpp >> 3; \
    } while (0)

#define RECTANGLE_FOR_EACH_PIXEL_BEGIN(fb, lfb, width, height, x, y) \
    for (size_t y = 0; y < (height); ++y) \
    { \
        uint8 *lfb = fb; \
        for (size_t x = 0; x < (width); ++x) \
        {

#define RECTANGLE_FOR_EACH_PIXEL_END() \
        } \
        fb += vesa_mode_info.pitch; \
    }

void vesa_clear(color_t color) {
    if (!vesa_available)
    {
        return;
    }
    uint8 *fb = vesa_framebuffer;
    uint32 device_color = vesa_device_color(color);

    RECTANGLE_FOR_EACH_PIXEL_BEGIN(fb, lfb, vesa_mode_info.x_res, vesa_mode_info.y_res,
                                   x, y)
        SET_COLOR(lfb, device_color);
    RECTANGLE_FOR_EACH_PIXEL_END()
    cursorx = 0;
    cursory = 0;
    empty_screen();
}

void vesa_clear_no_rst(color_t color) {
    if (!vesa_available)
    {
        return;
    }
    uint8 *fb = vesa_framebuffer;
    uint32 device_color = vesa_device_color(color);

    RECTANGLE_FOR_EACH_PIXEL_BEGIN(fb, lfb, vesa_mode_info.x_res, vesa_mode_info.y_res,
                                   x, y)
        SET_COLOR(lfb, device_color);
    RECTANGLE_FOR_EACH_PIXEL_END()
    //cursorx = 0;
    //cursory = 0;
    //empty_screen();
}

inline void vesa_fill_char(size_t x0, size_t y0, uint8 ch, color_t fc, color_t bc)
{
    if (!vesa_available)
    {
        return;
    }
    uint8 *fb = OFFSET(vesa_framebuffer, x0, y0);
    uint8 *font = vga_font_get(ch);
    uint32 device_fc = vesa_device_color(fc),
             device_bc = vesa_device_color(bc);

    RECTANGLE_FOR_EACH_PIXEL_BEGIN(fb, lfb, VESA_CHAR_WIDTH, VESA_CHAR_HEIGHT, x, y)
        uint32 c;
        if (x < VGA_FONT_WIDTH && y < VGA_FONT_HEIGHT && ((font[y] << x) & 0x80)) // MSB, left
        {
            c = device_fc;
        }
        else
        {
            c = device_bc;
        }
        SET_COLOR(lfb, c);
    RECTANGLE_FOR_EACH_PIXEL_END()
}

inline void vesa_fill_char_scale(size_t x0, size_t y0, uint8 ch,
                                 size_t scale, color_t fc, color_t bc)
{
    if (!vesa_available)
    {
        return;
    }
    uint8 *fb = OFFSET(vesa_framebuffer, x0, y0);
    uint8 *font = vga_font_get(ch);
    uint32 device_fc = vesa_device_color(fc),
             device_bc = vesa_device_color(bc);
    size_t char_width = scale / 2,
           char_height = scale;

    RECTANGLE_FOR_EACH_PIXEL_BEGIN(fb, lfb, char_width, char_height, x, y)
        uint32 c;
        // MSB, left
        if ((font[y * VGA_FONT_HEIGHT / scale] << (x * 2 * VGA_FONT_WIDTH / scale)) & 0x80)
        {
            c = device_fc;
        }
        else
        {
            c = device_bc;
        }
        SET_COLOR(lfb, c);
    RECTANGLE_FOR_EACH_PIXEL_END()
}

inline void vesa_fill_char_transparent(size_t x0, size_t y0, uint8 ch, color_t fc)
{
    if (!vesa_available)
    {
        return;
    }
    uint8 *fb = OFFSET(vesa_framebuffer, x0, y0);
    uint8 *font = vga_font_get(ch);
    uint32 device_fc = vesa_device_color(fc);

    RECTANGLE_FOR_EACH_PIXEL_BEGIN(fb, lfb, VGA_FONT_WIDTH, VGA_FONT_HEIGHT, x, y)
        if (x < VGA_FONT_WIDTH && y < VGA_FONT_HEIGHT && ((font[y] << x) & 0x80)) // MSB, left
        {
            SET_COLOR(lfb, device_fc);
        }
        else
        {
            KEEP_COLOR(lfb);
        }
    RECTANGLE_FOR_EACH_PIXEL_END()
}

inline void vesa_fill_char_scale_transparent(size_t x0, size_t y0, uint8 ch,
                                             size_t scale, color_t fc)
{
    if (!vesa_available)
    {
        return;
    }
    uint8 *fb = OFFSET(vesa_framebuffer, x0, y0);
    uint8 *font = vga_font_get(ch);
    uint32 device_fc = vesa_device_color(fc);
    size_t char_width = scale / 2,
           char_height = scale;

    RECTANGLE_FOR_EACH_PIXEL_BEGIN(fb, lfb, char_width, char_height, x, y)
        // MSB, left
        if ((font[y * VGA_FONT_HEIGHT / scale] << (x * 2 * VGA_FONT_WIDTH / scale)) & 0x80)
        {
            SET_COLOR(lfb, device_fc);
        }
        else
        {
            KEEP_COLOR(lfb);
        }
    RECTANGLE_FOR_EACH_PIXEL_END()
}

inline void vesa_fill_rect(size_t x0, size_t y0, size_t width, size_t height, color_t color)
{
    if (!vesa_available)
    {
        return;
    }
    uint8 *fb = OFFSET(vesa_framebuffer, x0, y0);
    uint32 device_color = vesa_device_color(color);

    RECTANGLE_FOR_EACH_PIXEL_BEGIN(fb, lfb, width, height, x, y)
        SET_COLOR(lfb, device_color);
    RECTANGLE_FOR_EACH_PIXEL_END()
}

inline void vesa_draw_hline(size_t x0, size_t y0, size_t width, color_t color)
{
    vesa_fill_rect(x0, y0, width, 1, color);
}
inline void vesa_draw_vline(size_t x0, size_t y0, size_t height, color_t color)
{
    vesa_fill_rect(x0, y0, 1, height, color);
}

void vesa_draw_button_rect(size_t x0, size_t y0, size_t width, size_t height, bool pushed)
{
    if (!pushed)
    {
        vesa_draw_hline(x0, y0, width, 0xe0e0e0);
        vesa_draw_vline(x0, y0, height - 1, 0xe0e0e0);
        vesa_draw_hline(x0, y0 + height - 1, width, 0x303030);
        vesa_draw_vline(x0 + width - 1, y0 + 1, height - 1, 0x303030);
        vesa_fill_rect(x0 + 1, y0 + 1, width - 2, height - 2, 0x909090);
    }
    else
    {
        vesa_draw_hline(x0, y0, width, 0x303030);
        vesa_draw_vline(x0, y0, height - 1, 0x303030);
        vesa_draw_hline(x0, y0 + height - 1, width, 0xe0e0e0);
        vesa_draw_vline(x0 + width - 1, y0, height, 0xe0e0e0);
        vesa_fill_rect(x0 + 1, y0 + 1, width - 2, height - 2, 0x808080);
    }
}

void vesa_fill_string(size_t x0, size_t y0, size_t width, const char *str, color_t fc)
{
    if (width != (size_t)-1)
    {
        for (int i = 0; str[i] != '\0'; ++i)
        {
            vesa_fill_char_transparent(x0 + i % width * 9, y0 + i / width * 16,
                                       str[i], fc);
        }
    }
    else
    {
        for (int i = 0; str[i] != '\0'; ++i)
        {
            vesa_fill_char_transparent(x0 + i * 9, y0, str[i], fc);
        }
    }
}

void vesa_redraw() {
    //vesa_clear_no_rst(VESABG);
    for (int y = 0; y < vesa_height; y++) {
        for (int x = 0; x < vesa_width; x++) {
            //cursorx = x;
            //cursory = y;
            //vesa_secret_set_char(x, y, ' ', VESABG, VESABG);
            //if (!vesa_screen[y][x].enabled) {
            //    vesa_screen[y][x].c = ' ';
            //    vesa_screen[y][x].fg = VESABG;
            //    vesa_screen[y][x].bg = VESABG;
            //    vesa_screen[y][x].enabled = true;
            //}
            if (vesa_screen[y][x].enabled) {
                //printk("(%i, %i): [%c]\n", y, x, vesa_screen[y][x].c);
                vesa_tty_set_char(x, y, vesa_screen[y][x].c, vesa_screen[y][x].fg, vesa_screen[y][x].bg);
            }
            if (vesa_screen[y][x].dirty) {
                vesa_tty_set_char(x, y, ' ', VESABG, VESABG);
                vesa_screen[y][x].enabled = false;
            }
            //if (!vesa_screen[y][x].enabled) {}
                //vesa_tty_set_char(x, y, ' ', VESABG, VESABG);
        }
    }

}

void vcursor_left() {
    if (cursorx != 0)
        cursorx--;
}

void vputs_status(int x, const char *str, color_t fg, color_t bg) {
    //printk("Status\n");
    size_t len = strlen(str);
    assert(x + len <= vesa_width);
    for (size_t i = 0; i < len; i++) {
        vesa_tty_set_char(x + i, 0, str[i], fg, bg);

        //if(x + i == 79) break; // TODO: allow timer spinner
        //NOTE: UNCOMMENT TO ENABLE OLD GRAPHICS real_vmem[x + i] = (status_bgcolor << BGCOLOR) | (status_fgcolor << FGCOLOR) | str[i];
    }
}

void update_statusbar(void) {

    while (true) {
        if (!kernel_paniced) {
            for (int i = 0; i < vesa_width; i++) {
                vputs_status(i, " ", BLUE, BLUE);
            }
            vputs_status(0, "[TextOS/2]", WHITE, BLUE);

            char buf[48] = {0};

            // Show a clock
        	Time t;
        	get_time(&t);
        	t.hour %= 24;
        	sprintf(buf, "[%02d:%02d:%02d]", t.hour, t.minute, t.second);
        	vputs_status(vesa_width - 10, buf, WHITE, BLUE);

            if ((t.second / 10) % 2) {
                int size = strlen(trim(MACRO(BUILDID))) + strlen("TextOS Version: ");
                sprintf(buf, "TextOS Version: %s", trim(MACRO(BUILDID)));
                vputs_status((vesa_width/2)-(size/2), buf, WHITE, BLUE);
            } else {
                int size = strlen(trim(CPUName));
                sprintf(buf, "%s", trim(CPUName));
                vputs_status((vesa_width/2)-(size/2), buf, WHITE, BLUE);
            }


        } else {
            int k = ((vesa_width) - (16 * ((int)vesa_width/16))) / 2;
            for (int i = 0; i < vesa_width; i++) {
                vputs_status(i, " ", RED, RED);
            }
            for (int v = 0; v < vesa_width/16; v++) {

                //printk("%i, %i, %i\n", k, vesa_width, vesa_width/16);
                vputs_status(k, "  KERNEL PANIC  ", WHITE, RED);
                k = k + 16;
            }
        }
        sleep(500);
    }

    //while (true) {
        //sleep(1000);

        #if 0

        #endif
    //}
    //printk(".");

    //vputs_status(0, "[TextOS/2]", WHITE, BLUE);
    #if 0
    if (kernel_paniced) {
		status_bgcolor = RED;
	}

	// Clear everything
	//NOTE: UNCOMMENT TO ENABLE OLD GRAPHICS memsetw(real_vmem, (uint16)((status_bgcolor << BGCOLOR) | (status_fgcolor << FGCOLOR) | ' '), 80 - 1 /* TODO: allow timer spinner */);

	if (kernel_paniced) {
		puts_status(0, "  KERNEL PANIC    KERNEL PANIC    KERNEL PANIC    KERNEL PANIC    KERNEL PANIC  ");
		return;
	}
    #endif

	//vputs_status(0, "[TextOS/2]");
    #if 0
	// Show the VC number

	sprintf(buf, "VC%u", current_console_number + 1); // convert to 1-indexed
	puts_status(12, trim(buf));

	int bstate = 0;

	// Show scrollback status
	//if (current_console->current_position != 0) {
		//sprintf(buf, "Scrollback: %u line%c", current_console->current_position, (current_console->current_position > 1 ? 's' : 0));
	//	puts_status(16, buf);
	//}



	//print the build ID and CPUID
	if (current_console_number < 4) {

		if (!warning) {
			status_bgcolor = BLUE;
			status_fgcolor = WHITE;
			if ((t.second / 10) % 2) {
				int size = strlen(trim(MACRO(BUILDID))) + strlen("TextOS Version: ");
				sprintf(buf, "TextOS Version: %s", trim(MACRO(BUILDID)));
				puts_status(40-(size/2), buf);
			} else {
				int size = strlen(trim(CPUName));
				sprintf(buf, "%s", trim(CPUName));
				puts_status(40-(size/2), buf);
			}
		} else {
			status_bgcolor = LIGHT_BROWN;
			status_fgcolor = BLACK;
		}

		if (!ActiveTSA) {
			warning = 1;
			int size = strlen("| TSA NOT ENABLED |");
			sprintf(buf, "| TSA NOT ENABLED |");
			puts_status(40-(size/2), buf);
		} else {
			warning = 0;
		}

	} else if (current_console_number == 4) {
		status_bgcolor = RED;
		status_fgcolor = WHITE;
		sprintf(buf, "TEXTOS USERSPACE ABSTRACTION LAYER");
		puts_status(40-(strlen(trim(buf))/2), trim(buf));
	}
    #endif
}

void vesa_scroll() {

    for (int y = 1; y < vesa_height-1; y++) {
        //vesa_screen[y] = vesa_screen[y+1];
        for (int x = 0; x < vesa_width; x++) {
            vesa_screen[y][x].enabled = false;
            vesa_screen[y][x].dirty = true;
            vesa_screen[y][x].c = ' ';
            if (vesa_screen[y+1][x].enabled) {
                vesa_screen[y][x] = vesa_screen[y+1][x];
                vesa_screen[y][x].dirty = false;

                //printk("[%i][%i] = [%i][%i]\n", y, x, y+1, x);
                vesa_screen[y+1][x].enabled = false;
                vesa_screen[y+1][x].dirty = true;

                vesa_screen[y+1][x].c = ' ';

                //printk("[%i][%i] = Disabled\n", y+1, x);
            }

        }
        //memcpy(vesa_screen[y], vesa_screen[y+1], sizeof(vesa_screen[y]));
    }
    //printk("|0,1| = %p\n", vesa_screen[0][1].c);

    //printk("Scrolled\n");
    //for (int x = 0; x < VESA_WIDTH; x++) {
    //    vesa_screen[VESA_HEIGHT][x].enabled = true;
    //}
    //printk("cy: %i\n", cursory);
    cursory--;
    //for (int ix = 0; ix < VESA_HEIGHT-1; ix++) {
    //    for (int i = 0; i < VESA_WIDTH; i++) {
    //        vesa_secret_set_char(i, ix, ' ', VESABG, VESABG);
    //    }
//}/
    //printk("cyx: %i\n", cursory);
    vesa_redraw();
}

void vesa_tty_set_char(size_t x, size_t y, char ch, color_t fgcolor, color_t bgcolor)
{
    //printk("[%i][%i]\n", y, x);
    vesa_screen[y][x].c = ch;
    vesa_screen[y][x].fg = fgcolor;
    vesa_screen[y][x].bg = bgcolor;
    vesa_screen[y][x].enabled = true;
    vesa_screen[y][x].dirty = false;
    size_t x0 = x * VESA_CHAR_WIDTH, y0 = y * VESA_CHAR_HEIGHT;
    uint8 *fb = OFFSET(vesa_framebuffer, x0, y0);
    uint8 *font = vga_font_get(ch);
    uint32 device_fc = fgcolor,
             device_bc = bgcolor;

    RECTANGLE_FOR_EACH_PIXEL_BEGIN(fb, lfb, VESA_CHAR_WIDTH, VESA_CHAR_HEIGHT, x_font, y_font)
        uint32 c;
        if (x_font < VGA_FONT_WIDTH && y_font < VGA_FONT_HEIGHT &&
            (font[y_font] << x_font) & 0x80) // MSB, left
        {
            c = device_fc;
        }
        else
        {
            c = device_bc;
        }
        SET_COLOR(lfb, c);
    RECTANGLE_FOR_EACH_PIXEL_END()
}
void vesa_secret_set_char(size_t x, size_t y, char ch, color_t fgcolor, color_t bgcolor)
{
    //printk("[%i][%i]\n", y, x);

    size_t x0 = x * VESA_CHAR_WIDTH, y0 = y * VESA_CHAR_HEIGHT;
    uint8 *fb = OFFSET(vesa_framebuffer, x0, y0);
    uint8 *font = vga_font_get(ch);
    uint32 device_fc = fgcolor,
             device_bc = bgcolor;

    RECTANGLE_FOR_EACH_PIXEL_BEGIN(fb, lfb, VESA_CHAR_WIDTH, VESA_CHAR_HEIGHT, x_font, y_font)
        uint32 c;
        if (x_font < VGA_FONT_WIDTH && y_font < VGA_FONT_HEIGHT &&
            (font[y_font] << x_font) & 0x80) // MSB, left
        {
            c = device_fc;
        }
        else
        {
            c = device_bc;
        }
        SET_COLOR(lfb, c);
    RECTANGLE_FOR_EACH_PIXEL_END()
}
