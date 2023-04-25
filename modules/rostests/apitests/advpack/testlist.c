/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for advpack.dll functions
 * COPYRIGHT:   Copyright 2023 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#define __ROS_LONG64__
#define STANDALONE
#include <apitest.h>

extern void func_DelNode(void);

const struct test winetest_testlist[] =
{
    { "DelNode", func_DelNode },
    { 0, 0 }
};
