#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_load(void);
extern void func_handles(void);
extern void func_instance(void);
extern void func_instance_find(void);
extern void func_filter_find(void);
extern void func_volume_find(void);
extern void func_info(void);
extern void func_comms(void);

const struct test winetest_testlist[] =
{
    { "load", func_load },
    { "handles", func_handles},
    { "instance", func_instance },
    { "instance_find", func_instance_find },
    { "filter_find", func_filter_find },
    { "volume_find", func_volume_find },
    { "info", func_info },
    { "comms", func_comms },
    { 0, 0 }
};
