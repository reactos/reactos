/*
 * PROJECT:     ReactOS API Tests
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Tests for network interface name resolving functions
 * COPYRIGHT:   Copyright 2017 Stanislav Motylkov
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <iphlpapi.h>
#include <ndk/rtlfuncs.h>
#include <initguid.h>

DEFINE_GUID(MY_TEST_GUID, 0x8D1AB70F, 0xADF0, 0x49FC, 0x9D, 0x07, 0x4E, 0x89, 0x29, 0x2D, 0xC5, 0x2B);
static DWORD (WINAPI * pNhGetInterfaceNameFromGuid)(PVOID, PVOID, PULONG, DWORD, DWORD);
static DWORD (WINAPI * pNhGetInterfaceNameFromDeviceGuid)(PVOID, PVOID, PULONG, DWORD, DWORD);

/*
 * Tests for NhGetInterfaceNameFromGuid
 */
static
VOID
test_NhGetInterfaceNameFromGuid(GUID AdapterGUID, DWORD par1, DWORD par2)
{
    DWORD ApiReturn, Error;
    ULONG ulOutBufLen;
    WCHAR Name[MAX_INTERFACE_NAME_LEN + 4];
    GUID UniqueGUID = MY_TEST_GUID;
    SIZE_T Length;

    // Test NULL GUID
    SetLastError(0xbeeffeed);
    Error = 0xbeeffeed;
    ZeroMemory(&Name, sizeof(Name));
    ApiReturn = ERROR_SUCCESS;
    ulOutBufLen = sizeof(Name);
    StartSeh()
        ApiReturn = pNhGetInterfaceNameFromGuid(NULL, &Name, &ulOutBufLen, par1, par2);
        Error = GetLastError();
    EndSeh(((GetVersion() & 0xFF) >= 6) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);

    ok_long(ApiReturn, ((GetVersion() & 0xFF) >= 6) ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS);
    ok(Error == 0xbeeffeed,
       "GetLastError() returned %ld, expected 0xbeeffeed\n",
       Error);
    ok(ulOutBufLen == sizeof(Name),
       "ulOutBufLen is %ld, expected = sizeof(Name)\n",
       ulOutBufLen);
    ok_wstr(L"", Name);

    // Test correct GUID, but NULL name
    SetLastError(0xbeeffeed);
    ZeroMemory(&Name, sizeof(Name));
    ApiReturn = pNhGetInterfaceNameFromGuid(&AdapterGUID, NULL, &ulOutBufLen, par1, par2);
    Error = GetLastError();

    ok_long(ApiReturn, ((GetVersion() & 0xFF) >= 6) ? ERROR_INVALID_PARAMETER : ERROR_SUCCESS);
    ok_long(Error, ((GetVersion() & 0xFF) >= 6) ? ERROR_SUCCESS : 0xbeeffeed);
    ok(ulOutBufLen > 0,
       "ulOutBufLen is %ld, expected > 0\n",
       ulOutBufLen);
    ok_wstr(L"", Name);

    // NhGetInterfaceNameFromGuid will throw exception if pOutBufLen is NULL
    SetLastError(0xbeeffeed);
    Error = 0xbeeffeed;
    ZeroMemory(&Name, sizeof(Name));
    ApiReturn = 0xdeadbeef;
    StartSeh()
        ApiReturn = pNhGetInterfaceNameFromGuid(&AdapterGUID, &Name, NULL, par1, par2);
        Error = GetLastError();
    EndSeh(STATUS_ACCESS_VIOLATION);

    ok(ApiReturn == 0xdeadbeef,
       "ApiReturn returned %ld, expected 0xdeadbeef\n",
       ApiReturn);
    ok(Error == 0xbeeffeed,
       "GetLastError() returned %ld, expected 0xbeeffeed\n",
       Error);
    ok(ulOutBufLen > 0,
       "ulOutBufLen is %ld, expected > 0\n",
       ulOutBufLen);
    ok_wstr(L"", Name);

    // Test correct values
    SetLastError(0xbeeffeed);
    ZeroMemory(&Name, sizeof(Name));
    ulOutBufLen = sizeof(Name);
    ApiReturn = pNhGetInterfaceNameFromGuid(&AdapterGUID, &Name, &ulOutBufLen, par1, par2);
    Error = GetLastError();

    ok(ApiReturn == ERROR_SUCCESS,
       "ApiReturn returned %ld, expected ERROR_SUCCESS\n",
       ApiReturn);
    ok_long(Error, ((GetVersion() & 0xFF) >= 6) ? 0 : 0xbeeffeed);
    ok(ulOutBufLen > 0,
       "ulOutBufLen is %ld, expected > 0\n",
       ulOutBufLen);
    Length = wcslen(Name);
    ok(Length > 0,
       "wcslen(Name) is %ld, expected > 0\n",
       Length);
    if (Length > 0)
        trace("Adapter name: \"%S\"\n", Name);

    // Test correct values, but with new unique GUID
    SetLastError(0xbeeffeed);
    ZeroMemory(&Name, sizeof(Name));
    ulOutBufLen = sizeof(Name);
    ApiReturn = pNhGetInterfaceNameFromGuid((PVOID)&UniqueGUID, &Name, &ulOutBufLen, par1, par2);
    Error = GetLastError();

    ok_long(ApiReturn, ((GetVersion() & 0xFF) >= 6) ? ERROR_INVALID_PARAMETER : ERROR_NOT_FOUND);
#ifdef _M_AMD64
    ok_long(Error, ERROR_FILE_NOT_FOUND);
#else
    ok_long(Error, 0);
#endif
    ok(ulOutBufLen == sizeof(Name),
       "ulOutBufLen is %ld, expected = sizeof(Name)\n",
       ulOutBufLen);
    ok_wstr(L"", Name);

    // Test correct values, but with small length
    SetLastError(0xbeeffeed);
    ZeroMemory(&Name, sizeof(Name));
    ulOutBufLen = 0;
    ApiReturn = pNhGetInterfaceNameFromGuid(&AdapterGUID, &Name, &ulOutBufLen, par1, par2);
    Error = GetLastError();

    ok_long(ApiReturn, ((GetVersion() & 0xFF) >= 6) ? ERROR_NOT_ENOUGH_MEMORY : ERROR_INSUFFICIENT_BUFFER);
    ok_long(Error, ((GetVersion() & 0xFF) >= 6) ? 0 : 0xbeeffeed);
    ok_long(ulOutBufLen, MAX_INTERFACE_NAME_LEN * 2 + ((GetVersion() & 0xFF) >= 6 ? 2 : 0));
    ok_wstr(L"", Name);
}

/*
 * Tests for NhGetInterfaceNameFromDeviceGuid
 */
static
VOID
test_NhGetInterfaceNameFromDeviceGuid(GUID AdapterGUID, DWORD par1, DWORD par2)
{
    DWORD ApiReturn, Error;
    ULONG ulOutBufLen;
    WCHAR Name[MAX_INTERFACE_NAME_LEN];
    GUID UniqueGUID = MY_TEST_GUID;
    SIZE_T Length;

    // Test NULL GUID
    // Windows XP: NhGetInterfaceNameFromDeviceGuid throws exception here
    SetLastError(0xbeeffeed);
    Error = 0xbeeffeed;
    ZeroMemory(&Name, sizeof(Name));
    ApiReturn = ERROR_SUCCESS;
    ulOutBufLen = sizeof(Name);
    StartSeh()
        ApiReturn = pNhGetInterfaceNameFromDeviceGuid(NULL, &Name, &ulOutBufLen, par1, par2);
        Error = GetLastError();
    EndSeh(((GetVersion() & 0xFF) >= 6) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);

    ok_long(ApiReturn, ((GetVersion() & 0xFF) >= 6) ? ERROR_INVALID_PARAMETER : 0);
    ok(Error == 0xbeeffeed,
       "GetLastError() returned %ld, expected ERROR_SUCCESS\n",
       Error);
    ok(ulOutBufLen == sizeof(Name),
       "ulOutBufLen is %ld, expected = sizeof(Name)\n",
       ulOutBufLen);
    ok_wstr(L"", Name);

    // Test correct GUID, but NULL name
    SetLastError(0xbeeffeed);
    Error = 0xbeeffeed;
    ZeroMemory(&Name, sizeof(Name));
    ApiReturn = ERROR_SUCCESS;
    StartSeh()
        ApiReturn = pNhGetInterfaceNameFromDeviceGuid(&AdapterGUID, NULL, &ulOutBufLen, par1, par2);
        Error = GetLastError();
    EndSeh(((GetVersion() & 0xFF) >= 6) ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);

    ok_long(ApiReturn, ((GetVersion() & 0xFF) >= 6) ? ERROR_INVALID_PARAMETER : 0);
    ok_long(Error, ((GetVersion() & 0xFF) >= 6) ? ERROR_SUCCESS : 0xbeeffeed);
    ok(ulOutBufLen > 0,
       "ulOutBufLen is %ld, expected > 0\n",
       ulOutBufLen);
    ok_wstr(L"", Name);

    // NhGetInterfaceNameFromDeviceGuid will throw exception if pOutBufLen is NULL
    SetLastError(0xbeeffeed);
    Error = 0xbeeffeed;
    ZeroMemory(&Name, sizeof(Name));
    ApiReturn = ERROR_SUCCESS;
    StartSeh()
        ApiReturn = pNhGetInterfaceNameFromDeviceGuid(&AdapterGUID, &Name, NULL, par1, par2);
        Error = GetLastError();
    EndSeh(STATUS_ACCESS_VIOLATION);

    ok_long(ApiReturn, 0);
    ok_long(Error, 0xbeeffeed);
    ok(ulOutBufLen > 0,
       "ulOutBufLen is %ld, expected > 0\n",
       ulOutBufLen);
    ok_wstr(L"", Name);

    // Test correct values
    SetLastError(0xbeeffeed);
    ZeroMemory(&Name, sizeof(Name));
    ulOutBufLen = sizeof(Name);
    ApiReturn = pNhGetInterfaceNameFromDeviceGuid(&AdapterGUID, &Name, &ulOutBufLen, par1, par2);
    Error = GetLastError();

    ok(ApiReturn == ERROR_SUCCESS,
       "ApiReturn returned %ld, expected ERROR_SUCCESS\n",
       ApiReturn);
    ok(Error == ERROR_SUCCESS,
       "GetLastError() returned %ld, expected ERROR_SUCCESS\n",
       Error);
    ok(ulOutBufLen > 0,
       "ulOutBufLen is %ld, expected > 0\n",
       ulOutBufLen);
    Length = wcslen(Name);
    ok(Length > 0,
       "wcslen(Name) is %ld, expected > 0\n",
       Length);
    if (Length > 0)
        trace("Adapter name: \"%S\"\n", Name);

    // Test correct values, but with new unique GUID
    SetLastError(0xbeeffeed);
    ZeroMemory(&Name, sizeof(Name));
    ulOutBufLen = sizeof(Name);
    ApiReturn = pNhGetInterfaceNameFromDeviceGuid((PVOID)&UniqueGUID, &Name, &ulOutBufLen, par1, par2);
    Error = GetLastError();

    ok_long(ApiReturn, ((GetVersion() & 0xFF) >= 6) ? ERROR_INVALID_PARAMETER : ERROR_NOT_FOUND);
    ok(Error == ERROR_SUCCESS,
       "GetLastError() returned %ld, expected ERROR_SUCCESS\n",
       Error);
    ok(ulOutBufLen == sizeof(Name),
       "ulOutBufLen is %ld, expected = sizeof(Name)\n",
       ulOutBufLen);
    ok_wstr(L"", Name);

    // Test correct values, but with small length
    SetLastError(0xbeeffeed);
    ZeroMemory(&Name, sizeof(Name));
    ulOutBufLen = 0;
    ApiReturn = pNhGetInterfaceNameFromDeviceGuid(&AdapterGUID, &Name, &ulOutBufLen, par1, par2);
    Error = GetLastError();

    ok_long(ApiReturn, ((GetVersion() & 0xFF) >= 6) ? ERROR_NOT_ENOUGH_MEMORY : ERROR_INSUFFICIENT_BUFFER);
    ok(Error == ERROR_SUCCESS,
       "GetLastError() returned %ld, expected ERROR_SUCCESS\n",
       Error);
    ok_long(ulOutBufLen, MAX_INTERFACE_NAME_LEN * 2 + (((GetVersion() & 0xFF) >= 6) ? 2 : 0));
    ok_wstr(L"", Name);
}

static
VOID
test_GetInterfaceName(VOID)
{
    PIP_INTERFACE_INFO pInfo = NULL;
    ULONG ulOutBufLen = 0;
    DWORD ApiReturn;
    WCHAR Name[MAX_ADAPTER_NAME];
    UNICODE_STRING GuidString;
    GUID AdapterGUID;
    HINSTANCE hIpHlpApi;

    ApiReturn = GetInterfaceInfo(pInfo, &ulOutBufLen);
    ok(ApiReturn == ERROR_INSUFFICIENT_BUFFER,
       "GetInterfaceInfo(pInfo, &ulOutBufLen) returned %ld, expected ERROR_INSUFFICIENT_BUFFER\n",
       ApiReturn);
    if (ApiReturn != ERROR_INSUFFICIENT_BUFFER)
    {
        skip("Can't determine size of IP_INTERFACE_INFO. Can't proceed\n");
        return;
    }

    pInfo = (IP_INTERFACE_INFO *)HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, ulOutBufLen);
    if (pInfo == NULL)
    {
        skip("pInfo is NULL. Can't proceed\n");
        return;
    }

    ApiReturn = GetInterfaceInfo(pInfo, &ulOutBufLen);
    ok(ApiReturn == NO_ERROR,
        "GetInterfaceInfo(pInfo, &ulOutBufLen) returned %ld, expected NO_ERROR\n",
        ApiReturn);
    if (ApiReturn != NO_ERROR || ulOutBufLen == 0)
    {
        skip("GetInterfaceInfo failed with error %ld. Can't proceed\n", ApiReturn);
        return;
    }

    if (pInfo->NumAdapters > 0)
        CopyMemory(&Name, &pInfo->Adapter[0].Name, sizeof(Name));

    if (pInfo->NumAdapters == 0)
    {
        HeapFree(GetProcessHeap(), 0, pInfo);
        skip("pInfo->NumAdapters = 0. Can't proceed\n");
        return;
    }
    trace("pInfo->NumAdapters: %lu\n", pInfo->NumAdapters);

    HeapFree(GetProcessHeap(), 0, pInfo);

    ApiReturn = wcsncmp(Name, L"\\DEVICE\\TCPIP_", 14);
    ok(ApiReturn == 0,
       "wcsncmp(Name, L\"\\DEVICE\\TCPIP_\", 14) returned %ld, expected 0\n",
       ApiReturn);
    if (ApiReturn != 0)
    {
        if (wcslen(Name) == 0)
        {
            skip("pInfo->Adapter[0].Name is empty. Can't proceed\n");
            return;
        }
        else
        {
            // workaround for ReactOS
            trace("pInfo->Adapter[0].Name = \"%ls\" is incorrect.\n", Name);
            RtlInitUnicodeString(&GuidString, &Name[0]);
        }
    }
    else
    {
        RtlInitUnicodeString(&GuidString, &Name[14]);
    }

    ApiReturn = RtlGUIDFromString(&GuidString, &AdapterGUID);
    if (ApiReturn != 0)
    {
        skip("RtlGUIDFromString failed. Can't proceed\n");
        return;
    }

    hIpHlpApi = GetModuleHandleW(L"iphlpapi.dll");
    if (!hIpHlpApi)
    {
        skip("Failed to load iphlpapi.dll. Can't proceed\n");
        return;
    }

    pNhGetInterfaceNameFromGuid = (void *)GetProcAddress(hIpHlpApi, "NhGetInterfaceNameFromGuid");

    if (!pNhGetInterfaceNameFromGuid)
        skip("NhGetInterfaceNameFromGuid not found. Can't proceed\n");
    else
        test_NhGetInterfaceNameFromGuid(AdapterGUID, 0, 0);

    pNhGetInterfaceNameFromDeviceGuid = (void *)GetProcAddress(hIpHlpApi, "NhGetInterfaceNameFromDeviceGuid");

    if (!pNhGetInterfaceNameFromDeviceGuid)
        skip("NhGetInterfaceNameFromDeviceGuid not found. Can't proceed\n");
    else
        test_NhGetInterfaceNameFromDeviceGuid(AdapterGUID, 1, 0);
}

START_TEST(GetInterfaceName)
{
    test_GetInterfaceName();
}
