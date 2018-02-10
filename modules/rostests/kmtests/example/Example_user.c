/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Example user-mode test part
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#include "Example.h"

START_TEST(Example)
{
    /* do some user-mode stuff */
    SYSTEM_INFO SystemInfo;
    MY_STRUCT MyStruct[2] = { { 123, ":D" }, { 0 } };
    DWORD Length = sizeof MyStruct;

    trace("Message from user-mode");

    GetSystemInfo(&SystemInfo);
    ok(SystemInfo.dwActiveProcessorMask != 0, "No active processors?!");

    /* now run the kernel-mode part (see Example.c).
     * If no user-mode part exists, this is what's done automatically */
    KmtRunKernelTest("Example");

    /* now start the special-purpose driver */
    KmtLoadDriver(L"Example", FALSE);
    trace("After Entry");
    KmtOpenDriver();
    trace("After Create");

    ok(KmtSendToDriver(IOCTL_NOTIFY) == ERROR_SUCCESS, "");
    ok(KmtSendStringToDriver(IOCTL_SEND_STRING, "yay") == ERROR_SUCCESS, "");
    ok(KmtSendBufferToDriver(IOCTL_SEND_MYSTRUCT, MyStruct, sizeof MyStruct[0], &Length) == ERROR_SUCCESS, "");
    ok_eq_int(MyStruct[1].a, 456);
    ok_eq_str(MyStruct[1].b, "!!!");

    KmtCloseDriver();
    trace("After Close");
    KmtUnloadDriver();
    trace("After Unload");
}
