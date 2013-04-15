/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_device(void);
extern void func_dinput(void);
extern void func_joystick(void);
extern void func_keyboard(void);
extern void func_mouse(void);

const struct test winetest_testlist[] =
{
    { "device", func_device },
    { "dinput", func_dinput },
    { "joystick", func_joystick },
    { "keyboard", func_keyboard },
    { "mouse", func_mouse },
    { 0, 0 }
};
