#ifndef _ARGON_H
#define _ARGON_H
#include <kernel/console.h>

struct Window {
    char title[128];
    int type; // 0: console
    int width;
    int height;
    int xpos;
    int ypos;
    console_t console;
};

int argon_init();

#endif
