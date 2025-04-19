/*
 * PROJECT:     ReactOS Kernel-Mode Tests
 * LICENSE:     LGPL-2.1-or-later (https://spdx.org/licenses/LGPL-2.1-or-later)
 * PURPOSE:     Tests for string I/O intrinsic functions
 * COPYRIGHT:   Copyright 2025 Dmitry Borisov <di.sean@protonmail.com>
 */

/* INCLUDES *******************************************************************/

#include <kmt_test.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

/* Compile with the highest optimization level to trigger a bug (See CORE-20078) */
#ifdef _MSC_VER
#pragma optimize("gst", on)
#pragma auto_inline(on)
#else
#pragma GCC optimize("O3")
#endif

/* Used to isolate the effects of intrinsics from each other */
typedef struct _TEST_CONTEXT
{
    PVOID OldBuffer;
    UCHAR Pad[78];
    USHORT Port;
    ULONG Size;
    PVOID Buffer;
} TEST_CONTEXT, *PTEST_CONTEXT;

/* TEST FUNCTIONS *************************************************************/

static
/*
 * Isolate the effects of intrinsics from each other
 * and hide the TEST_CONTEXT initialization from the optimizer.
 */
DECLSPEC_NOINLINE
VOID
TestWriteStringUshort(
    _In_ PTEST_CONTEXT Context)
{
    __outwordstring(Context->Port, Context->Buffer, Context->Size / sizeof(USHORT));

    /*
     * The 'rep outsw' instruction increments or decrements the address in ESI by Size * 2.
     * Test for CORE-20078: the ESI value should be preserved across calls.
     */
    ok_eq_pointer(Context->Buffer, Context->OldBuffer);
}

static
DECLSPEC_NOINLINE
VOID
TestWriteStringUlong(
    _In_ PTEST_CONTEXT Context)
{
    __outdwordstring(Context->Port, Context->Buffer, Context->Size / sizeof(ULONG));

    ok_eq_pointer(Context->Buffer, Context->OldBuffer);
}

static
DECLSPEC_NOINLINE
VOID
TestWriteStringUchar(
    _In_ PTEST_CONTEXT Context)
{
    __inbytestring(Context->Port, Context->Buffer, Context->Size);

    ok_eq_pointer(Context->Buffer, Context->OldBuffer);
}

static
DECLSPEC_NOINLINE
VOID
TestReadStringUshort(
    _In_ PTEST_CONTEXT Context)
{
    __inwordstring(Context->Port, Context->Buffer, Context->Size / sizeof(USHORT));

    ok_eq_pointer(Context->Buffer, Context->OldBuffer);
}

static
DECLSPEC_NOINLINE
VOID
TestReadStringUlong(
    _In_ PTEST_CONTEXT Context)
{
    __outdwordstring(Context->Port, Context->Buffer, Context->Size / sizeof(ULONG));

    ok_eq_pointer(Context->Buffer, Context->OldBuffer);
}

static
DECLSPEC_NOINLINE
VOID
TestReadStringUchar(
    _In_ PTEST_CONTEXT Context)
{
    __inbytestring(Context->Port, Context->Buffer, Context->Size);

    ok_eq_pointer(Context->Buffer, Context->OldBuffer);
}

static
VOID
TestStringIo(VOID)
{
    TEST_CONTEXT Context;
    UCHAR Buffer[20];

    /* End of the x86 I/O range */
    Context.Port = 0xFFFF - sizeof(ULONG);

    Context.Buffer = Buffer;
    Context.OldBuffer = Buffer;
    Context.Size = sizeof(Buffer);

    TestReadStringUchar(&Context);
    TestReadStringUshort(&Context);
    TestReadStringUlong(&Context);

    /*
     * Check whether the driver is running inside a virtual machine
     * as it's not safe to write to I/O ports
     * without having the port resources assigned.
     */
    if (!skip(KmtIsVirtualMachine, "Please run those tests in a supported virtual machine\n"))
    {
        TestWriteStringUchar(&Context);
        TestWriteStringUshort(&Context);
        TestWriteStringUlong(&Context);
    }
}

START_TEST(HalPortIo)
{
    TestStringIo();
}
