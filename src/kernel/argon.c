#include <kernel/argon.h>
#include <kernel/vesa.h>
#include <kernel/console.h>
#include <string.h>
#include <kernel/kernutil.h>

extern uint32 vbe_mode_info_p;

console_t *ogconsole;
console_t *argon_console;

struct Display display = {0};

bool argonRunning = false;
bool needRefresh;
bool stateChange = false;

int titleBuffer = 2;

int argon_statechange() {
    for (int i = 0; i < 256; i++) {
        if (display.window[i].type == 1) {
            argon_render_window(display.window[i]);
            //display.window[i].dirty = 0;
        }
    }
    for (int i = 0; i < 256; i++) {
        if (display.window[i].type == 1) {
            argon_render_window_title(display.window[i]);
            //display.window[i].dirty = 0;
        }
    }
    stateChange = false;
}

int argon_looper() {
    while (argonRunning) {
        //if (stateChange) argon_statechange();
    }
    return 0;
}

int argon_background() {
    vesa_clear(vesa_device_color(MKCOLOR(105, 105, 105)));
    argon_statechange();
    return 0;
}

int argon_render_window(struct Window window) {
    vesa_draw_box(window.xpos / VESA_CHAR_WIDTH, window.ypos/VESA_CHAR_HEIGHT - 1, window.width/VESA_CHAR_WIDTH, 1, vesa_device_color(MKCOLOR(0, 0, 250)), false);
    vputs_text(window.xpos / VESA_CHAR_WIDTH, window.ypos/VESA_CHAR_HEIGHT - 1, window.title, WHITE, vesa_device_color(MKCOLOR(0, 0, 250)));
    vesa_draw_box(window.xpos, window.ypos, window.width, window.height, window.background, true);


    window.dirty = 0;
    //argon_statechange();
    return 0;
}
int argon_render_window_title(struct Window window){
    vputs_text(window.xpos / VESA_CHAR_WIDTH, window.ypos/VESA_CHAR_HEIGHT - 1, window.title, WHITE, vesa_device_color(MKCOLOR(0, 0, 250)));
    return 0;
}

int argon_new_window() {
    for (int i = 0; i < 256; i++) {
        if (display.window[i].type == 0) {
            return i;
        }
    }
    return -1;
}

int argon_update_window(struct Window newwin, int oldpos) {
    newwin.dirty = 1;
    int len = strlen(newwin.title) + titleBuffer;

    if (newwin.width < len) {
        newwin.width = len;
    }
    if (newwin.height < 1)
        newwin.height = 1;

    newwin.xpos *= VESA_CHAR_WIDTH;
    newwin.ypos *= VESA_CHAR_HEIGHT;
    newwin.width *= VESA_CHAR_WIDTH;
    newwin.height *= VESA_CHAR_HEIGHT;

    newwin.ypos += 1; // Shift down for title bar

    display.window[oldpos] = newwin;
    argon_statechange();
    return 0;
}

int argon_test() {
    /*int argontest = argon_new_window();
    assert(argontest != -1);
    struct Window argontestwin = {0};

    strcpy(argontestwin.title, "Argon Testing");
    argontestwin.type = 1;
    argontestwin.width = 0;
    argontestwin.height = 8;
    argontestwin.xpos = 8;
    argontestwin.ypos = 8;
    argontestwin.background = vesa_device_color(MKCOLOR(255, 255, 255));
    argontestwin.foreground = vesa_device_color(MKCOLOR(255, 0, 0));
    argontestwin.dirty = 1;

    argon_update_window(argontestwin, argontest);*/

    for (size_t i = 0; i < 5; i++) {
        int winpos = argon_new_window();
        struct Window window = {0};

        strcpy(window.title, "Window Test");
        window.type = 1;
        window.height = 8;
        window.width = 16;
        window.xpos = 5 + (5*i);
        window.ypos = 5 + (5*i);
        window.background = vesa_device_color(MKCOLOR(255, 255, 255));
        window.foreground = vesa_device_color(MKCOLOR(255, 0, 0));
        window.dirty = 1;
        argon_update_window(window, winpos);
    }
    argon_statechange();
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

    display.width = vesa_mode_info.x_res;
    display.height = vesa_mode_info.y_res;
    display.currentWindow = 0;

    argon_background();

    argon_test();

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
