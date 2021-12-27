#include <kernel/kmod.h>
#include <kernel/console.h>
#include <string.h>
#include <kernel/kernutil.h>
#include <kernel/backtrace.h>
#include <kernel/elf.h>

/*
#include <kernel/lua/lua.h>
#include <kernel/lua/lualib.h>
#include <kernel/lua/lauxlib.h>

void test_lua() {
    lua_State *L;
    L = luaL_newstate();
    luaL_openlibs(L);
    luaL_dostring(L, "print \"TextOS Module Interface: \" .. _VERSION");
    lua_close(L);
}*/

//struct KextTab {
//    char name[64];
//    void* pointer;
//    int size;
//}

//struct KextTab KextTable[256];
