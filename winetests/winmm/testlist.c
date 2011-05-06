/* Automatically generated file; DO NOT EDIT!! */

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#define STANDALONE
#include "wine/test.h"

extern void func_capture(void);
extern void func_mci(void);
extern void func_mixer(void);
extern void func_mmio(void);
extern void func_timer(void);
extern void func_wave(void);

const struct test winetest_testlist[] =
{
    { "capture", func_capture },
    { "mci", func_mci },
    { "mixer", func_mixer },
    { "mmio", func_mmio },
    { "timer", func_timer },
    { "wave", func_wave },
    { 0, 0 }
};
