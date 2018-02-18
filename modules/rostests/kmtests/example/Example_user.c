/*
 * PROJECT:     ReactOS kernel-mode tests
 * LICENSE:     LGPL-2.1+ (https://spdx.org/licenses/LGPL-2.1+)
 * PURPOSE:     Kernel-Mode Test Suite Example user-mode test part
 * COPYRIGHT:   Copyright 2011-2018 Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

#include "Example.h"

START_TEST(Example)
{
    /* do some user-mode stuff */
    SYSTEM_INFO SystemInfo;
    MY_STRUCT MyStruct[2] = { { 123, ":D" }, { 0 } };
    DWORD Length = sizeof MyStruct;

    trace("Message from user-mode\n");

    GetSystemInfo(&SystemInfo);
    ok(SystemInfo.dwActiveProcessorMask != 0, "No active processors?!\n");

    /* now run the kernel-mode part (see Example.c).
     * If no user-mode part exists, this is what's done automatically */
    KmtRunKernelTest("Example");

    /* now start the special-purpose driver */
    KmtLoadDriver(L"Example", FALSE);
    trace("After Entry\n");
    KmtOpenDriver();
    trace("After Create\n");

    ok(KmtSendToDriver(IOCTL_NOTIFY) == ERROR_SUCCESS, "\n");
    ok(KmtSendStringToDriver(IOCTL_SEND_STRING, "yay") == ERROR_SUCCESS, "\n");
    ok(KmtSendBufferToDriver(IOCTL_SEND_MYSTRUCT, MyStruct, sizeof MyStruct[0], &Length) == ERROR_SUCCESS, "\n");
    ok_eq_int(MyStruct[1].a, 456);
    ok_eq_str(MyStruct[1].b, "!!!");

    KmtCloseDriver();
    trace("After Close\n");
    KmtUnloadDriver();
    trace("After Unload\n");
}
