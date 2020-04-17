/*
 * PROJECT:     ReactOS Local Spooler API Tests Injected DLL
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Main functions
 * COPYRIGHT:   Copyright 2015-2017 Colin Finck (colin@reactos.org)
 */

// Test list
extern void func_service(void);
extern void func_fpEnumPrinters(void);
extern void func_fpGetPrintProcessorDirectory(void);
extern void func_fpSetJob(void);

const struct test winetest_testlist[] =
{
    { "service", func_service },
    { "fpEnumPrinters", func_fpEnumPrinters },
    { "fpGetPrintProcessorDirectory", func_fpGetPrintProcessorDirectory },
    { "fpSetJob", func_fpSetJob },
    { 0, 0 }
};
