/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_device(void);
extern void func_query(void);
extern void func_shader(void);
extern void func_stateblock(void);
extern void func_surface(void);
extern void func_texture(void);
extern void func_vertexdeclaration(void);
extern void func_visual(void);
extern void func_volume(void);

const struct test winetest_testlist[] =
{
    { "device", func_device },
    { "query", func_query },
    { "shader", func_shader },
    { "stateblock", func_stateblock },
    { "surface", func_surface },
    { "texture", func_texture },
    { "vertexdeclaration", func_vertexdeclaration },
    { "visual", func_visual },
    { "volume", func_volume },
    { 0, 0 }
};
