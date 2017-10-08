/* Automatically generated file; DO NOT EDIT!! */

#define STANDALONE
#include <wine/test.h>

extern void func_task(void);
extern void func_task_scheduler(void);
extern void func_task_trigger(void);

const struct test winetest_testlist[] =
{
    { "task", func_task },
    { "task_scheduler", func_task_scheduler },
    { "task_trigger", func_task_trigger },
    { 0, 0 }
};
