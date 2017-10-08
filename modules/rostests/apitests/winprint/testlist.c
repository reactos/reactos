/*
 * PROJECT:     ReactOS Standard Print Processor API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Test list
 * COPYRIGHT:   Copyright 2015-2016 Colin Finck (colin@reactos.org)
 */

/*
 * These tests are developed and tested against the Windows Server 2003 counterpart of winprint.
 * While ReactOS implements the Standard Print Processor in a separate module winprint.dll,
 * Windows Server 2003 puts it into the Local Print Spooler localspl.dll.
 *
 * winspool.drv also provides functions that go into winprint.dll, but as these tests show,
 * they behave slightly different in terms of error codes due to the involved RPC and routing.
 */

#define __ROS_LONG64__

#define STANDALONE
#include <apitest.h>

extern void func_EnumPrintProcessorDatatypesW(void);

const struct test winetest_testlist[] =
{
    { "EnumPrintProcessorDatatypesW", func_EnumPrintProcessorDatatypesW },

    { 0, 0 }
};
