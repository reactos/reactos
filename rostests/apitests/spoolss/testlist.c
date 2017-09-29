/*
 * PROJECT:     ReactOS Spooler Router API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test list
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_AlignRpcPtr(void);
extern void func_PackStrings(void);
extern void func_ReallocSplStr(void);
extern void func_SplInitializeWinSpoolDrv(void);

const struct test winetest_testlist[] =
{
    { "AlignRpcPtr", func_AlignRpcPtr },
    { "PackStrings", func_PackStrings },
    { "ReallocSplStr", func_ReallocSplStr },
    { "SplInitializeWinSpoolDrv", func_SplInitializeWinSpoolDrv },

    { 0, 0 }
};
