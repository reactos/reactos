/*
 * PROJECT:     ReactOS Local Spooler API Tests
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Test list
 * COPYRIGHT:   Copyright 2015-2016 Colin Finck <colin@reactos.org>
 */

#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_fpEnumPrinters(void);
extern void func_fpGetPrintProcessorDirectory(void);
extern void func_service(void);

const struct test winetest_testlist[] =
{
    { "fpEnumPrinters", func_fpEnumPrinters },
    { "fpGetPrintProcessorDirectory", func_fpGetPrintProcessorDirectory },
    { "service", func_service },

    { 0, 0 }
};
