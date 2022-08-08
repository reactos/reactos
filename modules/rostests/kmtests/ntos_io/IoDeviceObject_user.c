/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver Object test user-mode part
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>

START_TEST(IoDeviceObject)
{
    DWORD Error;

    /* make sure IoHelper has an existing service key, but is not started */
    Error = KmtLoadDriver(L"IoHelper", FALSE);
    ok_eq_int(Error, ERROR_SUCCESS);
    if (Error)
        return;
    KmtUnloadDriver();

    Error = KmtLoadAndOpenDriver(L"IoDeviceObject", TRUE);
    ok_eq_int(Error, ERROR_SUCCESS);
    if (Error)
        return;

    KmtCloseDriver();
    KmtUnloadDriver();
}
