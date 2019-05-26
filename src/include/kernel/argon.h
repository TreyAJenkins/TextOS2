#ifndef _ARGON_H
#define _ARGON_H
#include <kernel/console.h>
#include <kernel/vesa.h>

struct Window {
    char title[128];
    int type; // 0: unused, 1: text, 2: graphics
    int width;
    int height;
    int xpos;
    int ypos;
    color_t background;
    color_t foreground;
    bool dirty;
};

struct Display {
    int width;
    int height;
    int currentWindow;
    struct Window window[256];
};

int argon_init();

#endif
