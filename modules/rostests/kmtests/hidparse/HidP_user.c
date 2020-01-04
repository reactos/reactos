/*
 * PROJECT:         ReactOS kernel-mode tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Kernel-Mode Test Suite Driver Object test user-mode part
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <kmt_test.h>
#include "HidP.h"

DWORD
KmtStartService(
    IN PCWSTR ServiceName OPTIONAL,
    IN OUT SC_HANDLE *ServiceHandle);

START_TEST(HidPDescription)
{
    SC_HANDLE ServiceHandle;
    DWORD Error;

    ServiceHandle = NULL;
    Error = KmtStartService(L"hidusb", &ServiceHandle);
    ok(Error == ERROR_SUCCESS, "Could not start service hidusb (Error=0x%lx). Sipping tests.\n", Error);
    if (Error != ERROR_SUCCESS)
    {
        return;
    }

    CloseServiceHandle(ServiceHandle);

    KmtLoadDriver(L"HidP", FALSE);
    KmtOpenDriver();

    Error = KmtSendToDriver(IOCTL_TEST_DESCRIPTION);
    ok(Error == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lx\n", Error);

    KmtCloseDriver();
    KmtUnloadDriver();
}