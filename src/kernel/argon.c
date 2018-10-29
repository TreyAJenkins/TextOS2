#include <kernel/argon.h>
#include <kernel/vesa.h>
#include <kernel/console.h>

console_t *ogconsole;
console_t *argon_console;

bool argonRunning = false;


int argon_looper() {
    while (argonRunning) {
        printk(".");
        sleep(1000);
    }
    return 0;
}

int argon_background() {
    vesa_clear(vesa_device_color(MKCOLOR(105, 105, 105)));
    return 0;
}

int argon_render_window(struct Window window) {

    return 0;
}

int argon_init() {
    extern bool fullscreen;
    extern bool inTextMode;
    argonRunning = true;

    ogconsole = current_console;

    //find a console;
    //int selconsole = 0;
    //for (int i = 0; i < NUM_VIRTUAL_CONSOLES; i++) {
    //    if (virtual_consoles[i] == NULL) {
    //        selconsole = i;
    //        break;
    //    }
    //}
    //printk("Argon: Selected console %i\n", selconsole);
    //virtual_consoles[selconsole] = console_create();
    //argon_console = virtual_consoles[selconsole];
    //console_switch(argon_console);
    //printk("Console switched\n");

    fullscreen = true;
    inTextMode = false;


    argon_background();
    argon_looper();
    return 0;
}

int argon_exit() {
    argonRunning = false;
    fullscreen = false;
    inTextMode = true;
    //console_switch(ogconsole);
    return 0;
}
