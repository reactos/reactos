/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_capture(void);
#ifdef RUN_COMPILE_TIME_ONLY_GENERATED
extern void func_generated(void);
#endif
extern void func_joystick(void);
extern void func_mci(void);
extern void func_mcicda(void);
extern void func_midi(void);
extern void func_mixer(void);
extern void func_mmio(void);
extern void func_timer(void);
extern void func_wave(void);

const struct test winetest_testlist[] =
{
    { "capture", func_capture },
#ifdef RUN_COMPILE_TIME_ONLY_GENERATED
    { "generated", func_generated },
#endif
    { "joystick", func_joystick },
    { "mci", func_mci },
    { "mcicda", func_mcicda },
    { "midi", func_midi },
    { "mixer", func_mixer },
    { "mmio", func_mmio },
    { "timer", func_timer },
    { "wave", func_wave },
    { 0, 0 }
};
