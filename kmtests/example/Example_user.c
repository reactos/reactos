/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Example user-mode test part
 * PROGRAMMER:      Thomas Faber <thfabba@gmx.de>
 */

#define UNICODE
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <kmt_test.h>

START_TEST(Example)
{
    /* do some user-mode stuff */
    SYSTEM_INFO SystemInfo;

    trace("Message from user-mode\n");

    GetSystemInfo(&SystemInfo);
    ok(SystemInfo.dwActiveProcessorMask != 0, "No active processors?!\n");

    /* now run the kernel-mode part (see Example.c).
     * If no user-mode part exists, this is what's done automatically */
    KmtRunKernelTest("Example");
}
