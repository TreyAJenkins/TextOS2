#include <kernel/vesa.h>
#include <sys/types.h>
#include <kernel/vmm.h>
#include <kernel/multiboot.h>
#include <string.h>
#include <kernel/console.h>

vesa_control_info_t vesa_control_info;
uint16 vesa_mode;
vesa_mode_info_t vesa_mode_info;
void *vesa_framebuffer_va;

static uint32 vesa_tty_color[16];

void vesa_init(uint32 control, uint16 mode, uint32 info) {
    //vmm_map_kernel(control, control, PAGE_RW);
    vesa_control_info_t *mb_control = (vesa_control_info_t*)control;
    memcpy(&vesa_control_info, mb_control, sizeof(vesa_control_info_t));
    //vesa_mode = mb->vbe_mode;

    //printk("Control: %p\n", vmm_get_phys(mb_control->magic, kernel_directory));
    printk("Control: %p\n", mb_control);
    printk("VESA Magic: %s\n", (char* ) mb_control->magic);
}
