/*
 * PROJECT:         ReactOS kernel-mode tests - Filter Manager
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Tests for checking filter registration
 * PROGRAMMER:      Ged Murphy <gedmurphy@reactos.org>
 */

#include <kmt_test.h>


START_TEST(FltMgrReg)
{
    static WCHAR FilterName[] = L"FltMgrReg";
    SC_HANDLE hService;
    HANDLE hPort;

    ok(KmtFltCreateService(FilterName, L"FltMgrLoad test driver", &hService) == ERROR_SUCCESS, "Failed to create the reg entry\n");
    ok(KmtFltAddAltitude(L"123456") == ERROR_SUCCESS, "\n");
    ok(KmtFltLoadDriver(TRUE, FALSE, FALSE, &hPort) == ERROR_SUCCESS, "Failed to load the driver\n");
    //__debugbreak();
    ok(KmtFltUnloadDriver(hPort, FALSE) == ERROR_SUCCESS, "Failed to unload the driver\n");
    ok(KmtFltDeleteService(NULL, &hService) == ERROR_SUCCESS, "Failed to delete the driver\n");
}
