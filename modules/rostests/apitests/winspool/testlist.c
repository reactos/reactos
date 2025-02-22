/*
 * PROJECT:     ReactOS Print Spooler DLL API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test list
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_ClosePrinter(void);
extern void func_EnumPrinters(void);
extern void func_EnumPrintProcessorDatatypes(void);
extern void func_GetDefaultPrinterA(void);
extern void func_GetDefaultPrinterW(void);
extern void func_GetPrinter(void);
extern void func_GetPrinterData(void);
extern void func_GetPrintProcessorDirectoryA(void);
extern void func_GetPrintProcessorDirectoryW(void);
extern void func_IsValidDevmodeA(void);
extern void func_IsValidDevmodeW(void);
extern void func_OpenPrinter(void);
extern void func_StartDocPrinter(void);

const struct test winetest_testlist[] =
{
    { "ClosePrinter", func_ClosePrinter },
    { "EnumPrinters", func_EnumPrinters },
    { "EnumPrintProcessorDatatypes", func_EnumPrintProcessorDatatypes },
    { "GetDefaultPrinterA", func_GetDefaultPrinterA },
    { "GetDefaultPrinterW", func_GetDefaultPrinterW },
    { "GetPrinter", func_GetPrinter },
    { "GetPrinterData", func_GetPrinterData },
    { "GetPrintProcessorDirectoryA", func_GetPrintProcessorDirectoryA },
    { "GetPrintProcessorDirectoryW", func_GetPrintProcessorDirectoryW },
    { "IsValidDevmodeA", func_IsValidDevmodeA },
    { "IsValidDevmodeW", func_IsValidDevmodeW },
    { "OpenPrinter", func_OpenPrinter },
    { "StartDocPrinter", func_StartDocPrinter },

    { 0, 0 }
};
