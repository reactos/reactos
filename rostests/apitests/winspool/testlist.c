/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GNU GPLv2 or any later version as published by the Free Software Foundation
 * PURPOSE:     Test list
 * COPYRIGHT:   Copyright 2015 Colin Finck <colin@reactos.org>
 */

#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_ClosePrinter(void);
extern void func_EnumPrinters(void);
extern void func_EnumPrintProcessorDatatypes(void);
extern void func_GetPrintProcessorDirectory(void);
extern void func_OpenPrinter(void);
extern void func_StartDocPrinter(void);

const struct test winetest_testlist[] =
{
    { "ClosePrinter", func_ClosePrinter },
    { "EnumPrinters", func_EnumPrinters },
    { "EnumPrintProcessorDatatypes", func_EnumPrintProcessorDatatypes },
    { "GetPrintProcessorDirectory", func_GetPrintProcessorDirectory },
    { "OpenPrinter", func_OpenPrinter },
    { "StartDocPrinter", func_StartDocPrinter },

    { 0, 0 }
};
