/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver Object test user-mode part
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

START_TEST(IoDeviceObject)
{
    /* make sure IoHelper has an existing service key, but is not started */
    KmtLoadDriver(L"IoHelper", FALSE);
    KmtUnloadDriver();

    KmtLoadDriver(L"IoDeviceObject", TRUE);
    KmtOpenDriver();
    KmtCloseDriver();
    KmtUnloadDriver();
}
