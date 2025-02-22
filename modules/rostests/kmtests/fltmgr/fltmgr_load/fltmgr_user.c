/*
 * PROJECT:         ReactOS kernel-mode tests - Filter Manager
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for checking filters load and connect correctly
 * PROGRAMMER:      Ged Murphy <gedmurphy@reactos.org>
 */

#include <kmt_test.h>


START_TEST(FltMgrLoad)
{
    static WCHAR FilterName[] = L"FltMgrLoad";
    SC_HANDLE hService;
    HANDLE hPort;

    trace("Message from user-mode\n");

    ok(KmtFltCreateService(FilterName, L"FltMgrLoad test driver", &hService) == ERROR_SUCCESS, "\n");
    ok(KmtFltLoadDriver(FALSE, FALSE, FALSE, &hPort) == ERROR_PRIVILEGE_NOT_HELD, "\n");
    ok(KmtFltLoadDriver(TRUE, FALSE, FALSE, &hPort) == ERROR_SUCCESS, "\n");

    ok(KmtFltConnectComms(&hPort) == ERROR_SUCCESS, "\n");

    ok(KmtFltDisconnectComms(hPort) == ERROR_SUCCESS, "\n");
    ok(KmtFltUnloadDriver(hPort, FALSE) == ERROR_SUCCESS, "\n");
    KmtFltDeleteService(NULL, &hService);
}
