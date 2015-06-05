/*
 * PROJECT:     ReactOS Print Spooler Router API Tests
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Test list
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_PackStrings(void);

const struct test winetest_testlist[] =
{
    { "PackStrings", func_PackStrings },

    { 0, 0 }
};
