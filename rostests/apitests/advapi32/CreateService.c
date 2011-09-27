/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for CreateService
 * PROGRAMMER:      Thomas Faber
 */

#include <wine/test.h>
#include <windows.h>
#include <strsafe.h>

#define ok_lasterr(err, s) ok(GetLastError() == (err), "%s GetLastError()=0x%08lx, expected 0x%08lx\n", s, GetLastError(), (DWORD)(err))

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

    ok_lasterr(ERROR_SUCCESS, "CreateServiceW");

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

static void Test_CreateService(void)
{
    SC_HANDLE hScm = NULL;
    SC_HANDLE hService1 = NULL, hService2 = NULL;
    LONG ret;
    DWORD tag1 = 0, tag2 = 0;

    SetLastError(DNS_ERROR_RCODE_NXRRSET);
    hScm = OpenSCManagerW(NULL, NULL, SC_MANAGER_CREATE_SERVICE);
    ok(hScm != NULL, "Failed to open service manager, error=0x%08lx\n", GetLastError());
    if (!hScm)
    {
        skip("No service control manager; cannot proceed with CreateService test\n");
        goto cleanup;
    }

    ok_lasterr(ERROR_SUCCESS, "OpenSCManagerW");

    if (MakeService(hScm, L"advapi32_apitest_CreateService_Test_Service1", &hService1, &tag1))
        goto cleanup;

    if (MakeService(hScm, L"advapi32_apitest_CreateService_Test_Service2", &hService2, &tag2))
        goto cleanup;

    ok(tag1 != tag2, "tag1=%lu, tag2=%lu\n", tag1, tag2);

cleanup:
    if (hService2)
    {
        SetLastError(DNS_ERROR_RCODE_NXRRSET);
        ret = DeleteService(hService2);
        ok(ret == TRUE, "DeleteService returned %ld, expected 1\n", ret);
        ok(GetLastError() == DNS_ERROR_RCODE_NXRRSET /* win7 */
            || GetLastError() == ERROR_SUCCESS /* win2k3 */, "DeleteService (Error: %ld)\n", GetLastError());

        CloseServiceHandle(hService2);
    }

    if (hService1)
    {
        SetLastError(DNS_ERROR_RCODE_NXRRSET);
        ret = DeleteService(hService1);
        ok(ret == TRUE, "DeleteService returned %ld, expected 1\n", ret);
        ok(GetLastError() == DNS_ERROR_RCODE_NXRRSET /* win7 */
            || GetLastError() == ERROR_SUCCESS /* win2k3 */, "DeleteService (Error: %ld)\n", GetLastError());

        CloseServiceHandle(hService1);
    }

    if (hScm)
        CloseServiceHandle(hScm);
}

START_TEST(CreateService)
{
    Test_CreateService();
}
