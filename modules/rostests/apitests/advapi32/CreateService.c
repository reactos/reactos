/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for CreateService
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include "precomp.h"

static int MakeService(SC_HANDLE hScm, const wchar_t *serviceName, SC_HANDLE *hService, DWORD *tag)
{
    DWORD ret;
    HKEY hKey = NULL;
    DWORD type = 0, tagData = 0, tagSize;
    wchar_t keyName[256];

    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    *hService = CreateServiceW(
                    hScm,
                    serviceName,
                    NULL,
                    DELETE,
                    SERVICE_KERNEL_DRIVER,
                    SERVICE_BOOT_START,
                    SERVICE_ERROR_IGNORE,
                    L"%systemroot%\\drivers\\win32k.sys",
                    L"advapi32_apitest_CreateService_Test_Group",
                    tag,
                    NULL,
                    NULL,
                    NULL);

    ok(*hService != NULL, "Failed to create service, error=0x%08lx\n", GetLastError());
    if (!*hService)
    {
        skip("No service; cannot proceed with CreateService test\n");
        return 1;
    }

    ok_err(ERROR_SUCCESS);

    ok(*tag != 0, "tag is zero, expected nonzero\n");

    StringCbPrintfW(keyName, sizeof keyName, L"System\\CurrentControlSet\\Services\\%ls", serviceName);
    ret = RegOpenKeyW(HKEY_LOCAL_MACHINE, keyName, &hKey);
    ok(ret == ERROR_SUCCESS, "RegOpenKeyW failed with 0x%08lx\n", ret);
    if (ret)
    {
        skip("No regkey; cannot proceed with CreateService test\n");
        return 2;
    }

    tagSize = sizeof tagData;
    ret = RegQueryValueExW(hKey, L"Tag", NULL, &type, (PBYTE)&tagData, &tagSize);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExW returned 0x%08lx\n", ret);
    ok(type == REG_DWORD, "type=%lu, expected REG_DWORD\n", type);
    ok(tagSize == sizeof tagData, "tagSize=%lu, expected 4\n", tagSize);
    ok(tagData == *tag, "tag=%lu, but registry says %lu\n", *tag, tagData);

    RegCloseKey(hKey);

    return 0;
}

static void DestroyService(SC_HANDLE hService)
{
    LONG ret;

    if (!hService)
        return;
    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    ret = DeleteService(hService);
    ok(ret == TRUE, "DeleteService returned %ld, expected 1\n", ret);
    ok(GetLastError() == DNS_ERROR_RCODE_NXRRSET /* win7 */
        || GetLastError() == ERROR_SUCCESS /* win2k3 */, "DeleteService GetLastError()=0x%08lx\n", GetLastError());

    CloseServiceHandle(hService);
}

static void Test_CreateService(void)
{
    SC_HANDLE hScm = NULL;
    SC_HANDLE hService1 = NULL, hService2 = NULL;
    SC_HANDLE hService3 = NULL;
    DWORD tag1 = 0, tag2 = 0;
    DWORD tag3 = 785;

    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    ok(hScm != NULL, "Failed to open service manager, error=0x%08lx\n", GetLastError());
    if (!hScm)
    {
        skip("No service control manager; cannot proceed with CreateService test\n");
        goto cleanup;
    }

    ok_err(ERROR_SUCCESS);

    if (MakeService(hScm, L"advapi32_apitest_CreateService_Test_Service1", &hService1, &tag1))
        goto cleanup;

    if (MakeService(hScm, L"advapi32_apitest_CreateService_Test_Service2", &hService2, &tag2))
        goto cleanup;

    ok(tag1 != tag2, "tag1=%lu, tag2=%lu\n", tag1, tag2);

    /* ask for a tag, but don't have a group */
    hService3 = CreateServiceW(
                    hScm,
                    L"advapi32_apitest_CreateService_Test_Service2",
                    NULL,
                    DELETE,
                    SERVICE_KERNEL_DRIVER,
                    SERVICE_BOOT_START,
                    SERVICE_ERROR_IGNORE,
                    L"%systemroot%\\drivers\\win32k.sys",
                    NULL,
                    &tag3,
                    NULL,
                    NULL,
                    NULL);
    ok(hService3 == NULL, "hService3=%p\n", hService3);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "error=%lu\n", GetLastError());
    ok(tag3 == 785, "tag3=%lu\n", tag3);
    DestroyService(hService3);

    hService3 = CreateServiceW(
                    hScm,
                    L"advapi32_apitest_CreateService_Test_Service2",
                    NULL,
                    DELETE,
                    SERVICE_KERNEL_DRIVER,
                    SERVICE_BOOT_START,
                    SERVICE_ERROR_IGNORE,
                    L"%systemroot%\\drivers\\win32k.sys",
                    L"",
                    &tag3,
                    NULL,
                    NULL,
                    NULL);
    ok(hService3 == NULL, "hService3=%p\n", hService3);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "error=%lu\n", GetLastError());
    ok(tag3 == 785, "tag3=%lu\n", tag3);
    DestroyService(hService3);

cleanup:

    DestroyService(hService2);
    DestroyService(hService1);

    if (hScm)
        CloseServiceHandle(hScm);
}

START_TEST(CreateService)
{
    Test_CreateService();
}
