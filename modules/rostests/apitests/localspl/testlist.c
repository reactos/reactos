/*
 * PROJECT:     ReactOS Local Spooler API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test list
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_fpEnumPrinters(void);
extern void func_fpGetPrintProcessorDirectory(void);
extern void func_fpSetJob(void);
extern void func_service(void);

const struct test winetest_testlist[] =
{
    { "fpEnumPrinters", func_fpEnumPrinters },
    { "fpGetPrintProcessorDirectory", func_fpGetPrintProcessorDirectory },
    { "fpSetJob", func_fpSetJob },
    { "service", func_service },

    { 0, 0 }
};
