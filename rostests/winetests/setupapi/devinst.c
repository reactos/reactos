/*
 * Devinst tests
 *
 * Copyright 2006 Christian Gmeiner
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include "windef.h"
#include "winbase.h"
#include "wingdi.h"
#include "winuser.h"
#include "winreg.h"
#include "guiddef.h"
#include "setupapi.h"

#include "wine/test.h"

/* function pointers */
static HMODULE hSetupAPI;
static HDEVINFO (WINAPI *pSetupDiCreateDeviceInfoList)(GUID*,HWND);
static HDEVINFO (WINAPI *pSetupDiCreateDeviceInfoListExW)(GUID*,HWND,PCWSTR,PVOID);
static BOOL     (WINAPI *pSetupDiCreateDeviceInterfaceA)(HDEVINFO, PSP_DEVINFO_DATA, const GUID *, PCSTR, DWORD, PSP_DEVICE_INTERFACE_DATA);
static BOOL     (WINAPI *pSetupDiCallClassInstaller)(DI_FUNCTION, HDEVINFO, PSP_DEVINFO_DATA);
static BOOL     (WINAPI *pSetupDiDestroyDeviceInfoList)(HDEVINFO);
static BOOL     (WINAPI *pSetupDiEnumDeviceInfo)(HDEVINFO, DWORD, PSP_DEVINFO_DATA);
static BOOL     (WINAPI *pSetupDiEnumDeviceInterfaces)(HDEVINFO, PSP_DEVINFO_DATA, const GUID *, DWORD, PSP_DEVICE_INTERFACE_DATA);
static BOOL     (WINAPI *pSetupDiInstallClassA)(HWND, PCSTR, DWORD, HSPFILEQ);
static HKEY     (WINAPI *pSetupDiOpenClassRegKeyExA)(GUID*,REGSAM,DWORD,PCSTR,PVOID);
static HKEY     (WINAPI *pSetupDiOpenDevRegKey)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, REGSAM);
static HKEY     (WINAPI *pSetupDiCreateDevRegKeyW)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, DWORD, DWORD, HINF, PCWSTR);
static BOOL     (WINAPI *pSetupDiCreateDeviceInfoA)(HDEVINFO, PCSTR, GUID *, PCSTR, HWND, DWORD, PSP_DEVINFO_DATA);
static BOOL     (WINAPI *pSetupDiCreateDeviceInfoW)(HDEVINFO, PCWSTR, GUID *, PCWSTR, HWND, DWORD, PSP_DEVINFO_DATA);
static BOOL     (WINAPI *pSetupDiGetDeviceInstanceIdA)(HDEVINFO, PSP_DEVINFO_DATA, PSTR, DWORD, PDWORD);
static BOOL     (WINAPI *pSetupDiGetDeviceInterfaceDetailA)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_A, DWORD, PDWORD, PSP_DEVINFO_DATA);
static BOOL     (WINAPI *pSetupDiGetDeviceInterfaceDetailW)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA, PSP_DEVICE_INTERFACE_DETAIL_DATA_W, DWORD, PDWORD, PSP_DEVINFO_DATA);
static BOOL     (WINAPI *pSetupDiRegisterDeviceInfo)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PSP_DETSIG_CMPPROC, PVOID, PSP_DEVINFO_DATA);
static HDEVINFO (WINAPI *pSetupDiGetClassDevsA)(CONST GUID *, LPCSTR, HWND, DWORD);
static HDEVINFO (WINAPI *pSetupDiGetClassDevsW)(CONST GUID *, LPCWSTR, HWND, DWORD);
static BOOL     (WINAPI *pSetupDiSetDeviceRegistryPropertyA)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, const BYTE *, DWORD);
static BOOL     (WINAPI *pSetupDiSetDeviceRegistryPropertyW)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, const BYTE *, DWORD);
static BOOL     (WINAPI *pSetupDiGetDeviceRegistryPropertyA)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);
static BOOL     (WINAPI *pSetupDiGetDeviceRegistryPropertyW)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);

static void init_function_pointers(void)
{
    hSetupAPI = GetModuleHandleA("setupapi.dll");

    pSetupDiCreateDeviceInfoA = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInfoA");
    pSetupDiCreateDeviceInfoW = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInfoW");
    pSetupDiCreateDeviceInfoList = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInfoList");
    pSetupDiCreateDeviceInfoListExW = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInfoListExW");
    pSetupDiCreateDeviceInterfaceA = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDeviceInterfaceA");
    pSetupDiDestroyDeviceInfoList = (void *)GetProcAddress(hSetupAPI, "SetupDiDestroyDeviceInfoList");
    pSetupDiCallClassInstaller = (void *)GetProcAddress(hSetupAPI, "SetupDiCallClassInstaller");
    pSetupDiEnumDeviceInfo = (void *)GetProcAddress(hSetupAPI, "SetupDiEnumDeviceInfo");
    pSetupDiEnumDeviceInterfaces = (void *)GetProcAddress(hSetupAPI, "SetupDiEnumDeviceInterfaces");
    pSetupDiGetDeviceInstanceIdA = (void *)GetProcAddress(hSetupAPI, "SetupDiGetDeviceInstanceIdA");
    pSetupDiGetDeviceInterfaceDetailA = (void *)GetProcAddress(hSetupAPI, "SetupDiGetDeviceInterfaceDetailA");
    pSetupDiGetDeviceInterfaceDetailW = (void *)GetProcAddress(hSetupAPI, "SetupDiGetDeviceInterfaceDetailW");
    pSetupDiInstallClassA = (void *)GetProcAddress(hSetupAPI, "SetupDiInstallClassA");
    pSetupDiOpenClassRegKeyExA = (void *)GetProcAddress(hSetupAPI, "SetupDiOpenClassRegKeyExA");
    pSetupDiOpenDevRegKey = (void *)GetProcAddress(hSetupAPI, "SetupDiOpenDevRegKey");
    pSetupDiCreateDevRegKeyW = (void *)GetProcAddress(hSetupAPI, "SetupDiCreateDevRegKeyW");
    pSetupDiRegisterDeviceInfo = (void *)GetProcAddress(hSetupAPI, "SetupDiRegisterDeviceInfo");
    pSetupDiGetClassDevsA = (void *)GetProcAddress(hSetupAPI, "SetupDiGetClassDevsA");
    pSetupDiGetClassDevsW = (void *)GetProcAddress(hSetupAPI, "SetupDiGetClassDevsW");
    pSetupDiSetDeviceRegistryPropertyA = (void *)GetProcAddress(hSetupAPI, "SetupDiSetDeviceRegistryPropertyA");
    pSetupDiSetDeviceRegistryPropertyW = (void *)GetProcAddress(hSetupAPI, "SetupDiSetDeviceRegistryPropertyW");
    pSetupDiGetDeviceRegistryPropertyA = (void *)GetProcAddress(hSetupAPI, "SetupDiGetDeviceRegistryPropertyA");
    pSetupDiGetDeviceRegistryPropertyW = (void *)GetProcAddress(hSetupAPI, "SetupDiGetDeviceRegistryPropertyW");
}

/* RegDeleteTreeW from dlls/advapi32/registry.c */
LSTATUS WINAPI devinst_RegDeleteTreeW(HKEY hKey, LPCWSTR lpszSubKey)
{
    LONG ret;
    DWORD dwMaxSubkeyLen, dwMaxValueLen;
    DWORD dwMaxLen, dwSize;
    WCHAR szNameBuf[MAX_PATH], *lpszName = szNameBuf;
    HKEY hSubKey = hKey;

    if(lpszSubKey)
    {
        ret = RegOpenKeyExW(hKey, lpszSubKey, 0, KEY_READ, &hSubKey);
        if (ret) return ret;
    }

    /* Get highest length for keys, values */
    ret = RegQueryInfoKeyW(hSubKey, NULL, NULL, NULL, NULL,
            &dwMaxSubkeyLen, NULL, NULL, &dwMaxValueLen, NULL, NULL, NULL);
    if (ret) goto cleanup;

    dwMaxSubkeyLen++;
    dwMaxValueLen++;
    dwMaxLen = max(dwMaxSubkeyLen, dwMaxValueLen);
    if (dwMaxLen > sizeof(szNameBuf)/sizeof(WCHAR))
    {
        /* Name too big: alloc a buffer for it */
        if (!(lpszName = HeapAlloc( GetProcessHeap(), 0, dwMaxLen*sizeof(WCHAR))))
        {
            ret = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
    }


    /* Recursively delete all the subkeys */
    while (TRUE)
    {
        dwSize = dwMaxLen;
        if (RegEnumKeyExW(hSubKey, 0, lpszName, &dwSize, NULL,
                          NULL, NULL, NULL)) break;

        ret = devinst_RegDeleteTreeW(hSubKey, lpszName);
        if (ret) goto cleanup;
    }

    if (lpszSubKey)
        ret = RegDeleteKeyW(hKey, lpszSubKey);
    else
        while (TRUE)
        {
            dwSize = dwMaxLen;
            if (RegEnumValueW(hKey, 0, lpszName, &dwSize,
                  NULL, NULL, NULL, NULL)) break;

            ret = RegDeleteValueW(hKey, lpszName);
            if (ret) goto cleanup;
        }

cleanup:
    /* Free buffer if allocated */
    if (lpszName != szNameBuf)
        HeapFree( GetProcessHeap(), 0, lpszName);
    if(lpszSubKey)
        RegCloseKey(hSubKey);
    return ret;
}


static void test_SetupDiCreateDeviceInfoListEx(void) 
{
    HDEVINFO devlist;
    BOOL ret;
    DWORD error;
    static CHAR notnull[] = "NotNull";
    static const WCHAR machine[] = { 'd','u','m','m','y',0 };

    SetLastError(0xdeadbeef);
    /* create empty DeviceInfoList, but set Reserved to a value, which is not NULL */
    devlist = pSetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, notnull);

    error = GetLastError();
    if (error == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("SetupDiCreateDeviceInfoListExW is not implemented\n");
        return;
    }
    ok(devlist == INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %d (expected %p)\n", devlist, error, INVALID_HANDLE_VALUE);
    ok(error == ERROR_INVALID_PARAMETER, "GetLastError returned wrong value : %d, (expected %d)\n", error, ERROR_INVALID_PARAMETER);

    SetLastError(0xdeadbeef);
    /* create empty DeviceInfoList, but set MachineName to something */
    devlist = pSetupDiCreateDeviceInfoListExW(NULL, NULL, machine, NULL);

    error = GetLastError();
    ok(devlist == INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %d (expected %p)\n", devlist, error, INVALID_HANDLE_VALUE);
    ok(error == ERROR_INVALID_MACHINENAME, "GetLastError returned wrong value : %d, (expected %d)\n", error, ERROR_INVALID_MACHINENAME);

    /* create empty DeviceInfoList */
    devlist = pSetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
    ok(devlist && devlist != INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %d (expected != %p)\n", devlist, error, INVALID_HANDLE_VALUE);

    /* destroy DeviceInfoList */
    ret = pSetupDiDestroyDeviceInfoList(devlist);
    ok(ret, "SetupDiDestroyDeviceInfoList failed : %d\n", error);
}

static void test_SetupDiOpenClassRegKeyExA(void)
{
    /* This is a unique guid for testing purposes */
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    static const CHAR guidString[] = "{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
    HKEY hkey;

    /* Check return value for nonexistent key */
    hkey = pSetupDiOpenClassRegKeyExA(&guid, KEY_ALL_ACCESS,
        DIOCR_INSTALLER, NULL, NULL);
    ok(hkey == INVALID_HANDLE_VALUE,
        "returned %p (expected INVALID_HANDLE_VALUE)\n", hkey);

    /* Test it for a key that exists */
    hkey = SetupDiOpenClassRegKey(NULL, KEY_ALL_ACCESS);
    if (hkey != INVALID_HANDLE_VALUE)
    {
        HKEY classKey;
        if (RegCreateKeyA(hkey, guidString, &classKey) == ERROR_SUCCESS)
        {
            RegCloseKey(classKey);
            SetLastError(0xdeadbeef);
            classKey = pSetupDiOpenClassRegKeyExA(&guid, KEY_ALL_ACCESS,
                DIOCR_INSTALLER, NULL, NULL);
            ok(classKey != INVALID_HANDLE_VALUE,
                "opening class registry key failed with error %d\n",
                GetLastError());
            if (classKey != INVALID_HANDLE_VALUE)
                RegCloseKey(classKey);
            RegDeleteKeyA(hkey, guidString);
        }
        else
            trace("failed to create registry key for test\n");
    }
    else
        trace("failed to open classes key\n");
}

static void append_str(char **str, const char *data)
{
    sprintf(*str, data);
    *str += strlen(*str);
}

static void create_inf_file(LPCSTR filename)
{
    char data[1024];
    char *ptr = data;
    DWORD dwNumberOfBytesWritten;
    HANDLE hf = CreateFile(filename, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    append_str(&ptr, "[Version]\n");
    append_str(&ptr, "Signature=\"$Chicago$\"\n");
    append_str(&ptr, "Class=Bogus\n");
    append_str(&ptr, "ClassGUID={6a55b5a4-3f65-11db-b704-0011955c2bdb}\n");
    append_str(&ptr, "[ClassInstall32]\n");
    append_str(&ptr, "AddReg=BogusClass.NT.AddReg\n");
    append_str(&ptr, "[BogusClass.NT.AddReg]\n");
    append_str(&ptr, "HKR,,,,\"Wine test devices\"\n");

    WriteFile(hf, data, ptr - data, &dwNumberOfBytesWritten, NULL);
    CloseHandle(hf);
}

static void get_temp_filename(LPSTR path)
{
    static char curr[MAX_PATH] = { 0 };
    char temp[MAX_PATH];
    LPSTR ptr;

    if (!*curr)
        GetCurrentDirectoryA(MAX_PATH, curr);
    GetTempFileNameA(curr, "set", 0, temp);
    ptr = strrchr(temp, '\\');

    lstrcpyA(path, ptr + 1);
}

static void testInstallClass(void)
{
    static const WCHAR classKey[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'C','o','n','t','r','o','l','\\','C','l','a','s','s','\\',
     '{','6','a','5','5','b','5','a','4','-','3','f','6','5','-',
     '1','1','d','b','-','b','7','0','4','-',
     '0','0','1','1','9','5','5','c','2','b','d','b','}',0};
    static const CHAR classKey_win9x[] =
     "System\\CurrentControlSet\\Services\\Class\\"
     "{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
    static const CHAR bogus_win9x[] =
     "System\\CurrentControlSet\\Services\\Class\\Bogus";
    char tmpfile[MAX_PATH];
    BOOL ret;
    HKEY hkey;

    if (!pSetupDiInstallClassA)
    {
        skip("No SetupDiInstallClassA\n");
        return;
    }
    tmpfile[0] = '.';
    tmpfile[1] = '\\';
    get_temp_filename(tmpfile + 2);
    create_inf_file(tmpfile + 2);

    ret = pSetupDiInstallClassA(NULL, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
    ret = pSetupDiInstallClassA(NULL, NULL, DI_NOVCP, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
    ret = pSetupDiInstallClassA(NULL, tmpfile + 2, DI_NOVCP, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
    ret = pSetupDiInstallClassA(NULL, tmpfile + 2, 0, NULL);
    ok(!ret && GetLastError() == ERROR_FILE_NOT_FOUND,
     "Expected ERROR_FILE_NOT_FOUND, got %08x\n", GetLastError());
    /* The next call will succeed. Information is put into the registry but the
     * location(s) is/are depending on the Windows version.
     */
    ret = pSetupDiInstallClassA(NULL, tmpfile, 0, NULL);
    ok(ret, "SetupDiInstallClassA failed: %08x\n", GetLastError());
    if (!RegOpenKeyA(HKEY_LOCAL_MACHINE, classKey_win9x, &hkey))
    {
        /* We are on win9x */
        RegCloseKey(hkey);
        ok(!RegDeleteKeyA(HKEY_LOCAL_MACHINE, classKey_win9x),
         "Couldn't delete win9x classkey\n");
        ok(!RegDeleteKeyA(HKEY_LOCAL_MACHINE, bogus_win9x),
         "Couldn't delete win9x bogus services class\n");
    }
    else
    {
        /* NT4 and above */
        ok(!RegDeleteKeyW(HKEY_LOCAL_MACHINE, classKey),
         "Couldn't delete NT classkey\n");
    }
    DeleteFile(tmpfile);
}

static void testCreateDeviceInfo(void)
{
    BOOL ret;
    HDEVINFO set;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};

    if (!pSetupDiCreateDeviceInfoList || !pSetupDiEnumDeviceInfo ||
     !pSetupDiDestroyDeviceInfoList || !pSetupDiCreateDeviceInfoA)
    {
        skip("No SetupDiCreateDeviceInfoA\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInfoA(NULL, NULL, NULL, NULL, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_DEVINST_NAME,
     "Expected ERROR_INVALID_DEVINST_NAME, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInfoA(NULL, "Root\\LEGACY_BOGUS\\0000", NULL,
     NULL, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLEHANDLE, got %08x\n", GetLastError());
    set = pSetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != NULL, "SetupDiCreateDeviceInfoList failed: %08x\n",
     GetLastError());
    if (set)
    {
        SP_DEVINFO_DATA devInfo = { 0 };
        DWORD i;

        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", NULL,
         NULL, NULL, 0, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
            "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
        /* Finally, with all three required parameters, this succeeds: */
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid,
         NULL, NULL, 0, NULL);
        ok(ret, "pSetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        /* This fails because the device ID already exists.. */
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid,
         NULL, NULL, 0, &devInfo);
        ok(!ret && GetLastError() == ERROR_DEVINST_ALREADY_EXISTS,
         "Expected ERROR_DEVINST_ALREADY_EXISTS, got %08x\n", GetLastError());
        /* whereas this "fails" because cbSize is wrong.. */
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL,
         DICD_GENERATE_ID, &devInfo);
        ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER,
         "Expected ERROR_INVALID_USER_BUFFER, got %08x\n", GetLastError());
        devInfo.cbSize = sizeof(devInfo);
        ret = pSetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL,
         DICD_GENERATE_ID, &devInfo);
        /* and this finally succeeds. */
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        /* There were three devices added, however - the second failure just
         * resulted in the SP_DEVINFO_DATA not getting copied.
         */
        SetLastError(0xdeadbeef);
        i = 0;
        while (pSetupDiEnumDeviceInfo(set, i, &devInfo))
            i++;
        ok(i == 3, "Expected 3 devices, got %d\n", i);
        ok(GetLastError() == ERROR_NO_MORE_ITEMS,
         "SetupDiEnumDeviceInfo failed: %08x\n", GetLastError());
        pSetupDiDestroyDeviceInfoList(set);
    }
}

static void testGetDeviceInstanceId(void)
{
    BOOL ret;
    HDEVINFO set;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    SP_DEVINFO_DATA devInfo = { 0 };

    if (!pSetupDiCreateDeviceInfoList || !pSetupDiDestroyDeviceInfoList ||
     !pSetupDiCreateDeviceInfoA || !pSetupDiGetDeviceInstanceIdA)
    {
        skip("No SetupDiGetDeviceInstanceIdA\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceInstanceIdA(NULL, NULL, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLEHANDLE, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceInstanceIdA(NULL, &devInfo, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLEHANDLE, got %08x\n", GetLastError());
    set = pSetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != NULL, "SetupDiCreateDeviceInfoList failed: %08x\n",
     GetLastError());
    if (set)
    {
        char instanceID[MAX_PATH];
        DWORD size;

        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInstanceIdA(set, NULL, NULL, 0, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInstanceIdA(set, &devInfo, NULL, 0, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInstanceIdA(set, &devInfo, NULL, 0, &size);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
        devInfo.cbSize = sizeof(devInfo);
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInstanceIdA(set, &devInfo, NULL, 0, &size);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid,
         NULL, NULL, 0, &devInfo);
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInstanceIdA(set, &devInfo, NULL, 0, &size);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
         "Expected ERROR_INSUFFICIENT_BUFFER, got %08x\n", GetLastError());
        ret = pSetupDiGetDeviceInstanceIdA(set, &devInfo, instanceID,
         sizeof(instanceID), NULL);
        ok(ret, "SetupDiGetDeviceInstanceIdA failed: %08x\n", GetLastError());
        ok(!lstrcmpA(instanceID, "ROOT\\LEGACY_BOGUS\\0000"),
         "Unexpected instance ID %s\n", instanceID);
        ret = pSetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid,
         NULL, NULL, DICD_GENERATE_ID, &devInfo);
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        ret = pSetupDiGetDeviceInstanceIdA(set, &devInfo, instanceID,
         sizeof(instanceID), NULL);
        ok(ret, "SetupDiGetDeviceInstanceIdA failed: %08x\n", GetLastError());
        ok(!lstrcmpA(instanceID, "ROOT\\LEGACY_BOGUS\\0001"),
         "Unexpected instance ID %s\n", instanceID);
        pSetupDiDestroyDeviceInfoList(set);
    }
}

static void testRegisterDeviceInfo(void)
{
    BOOL ret;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    HDEVINFO set;

    if (!pSetupDiCreateDeviceInfoList || !pSetupDiDestroyDeviceInfoList ||
     !pSetupDiRegisterDeviceInfo)
    {
        skip("No SetupDiRegisterDeviceInfo\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pSetupDiRegisterDeviceInfo(NULL, NULL, 0, NULL, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    ret = pSetupDiRegisterDeviceInfo(NULL, NULL, 0, NULL, NULL, NULL);
    set = pSetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != NULL, "SetupDiCreateDeviceInfoList failed: %d\n", GetLastError());
    if (set)
    {
        SP_DEVINFO_DATA devInfo = { 0 };

        SetLastError(0xdeadbeef);
        ret = pSetupDiRegisterDeviceInfo(set, NULL, 0, NULL, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiRegisterDeviceInfo(set, &devInfo, 0, NULL, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        devInfo.cbSize = sizeof(devInfo);
        SetLastError(0xdeadbeef);
        ret = pSetupDiRegisterDeviceInfo(set, &devInfo, 0, NULL, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        ret = pSetupDiCreateDeviceInfoA(set, "USB\\BOGUS\\0000", &guid,
         NULL, NULL, 0, &devInfo);
        ok(ret || GetLastError() == ERROR_DEVINST_ALREADY_EXISTS,
                "SetupDiCreateDeviceInfoA failed: %d\n", GetLastError());
        if (ret)
        {
            /* If it already existed, registering it again will fail */
            ret = pSetupDiRegisterDeviceInfo(set, &devInfo, 0, NULL, NULL,
             NULL);
            ok(ret, "SetupDiCreateDeviceInfoA failed: %d\n", GetLastError());
        }
        /* FIXME: On Win2K+ systems, this is now persisted to registry in
         * HKLM\System\CCS\Enum\USB\Bogus\0000.  I don't check because the
         * Win9x location is different.
         * FIXME: the key also becomes undeletable.  How to get rid of it?
         */
        pSetupDiDestroyDeviceInfoList(set);
    }
}

static void testCreateDeviceInterface(void)
{
    BOOL ret;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    HDEVINFO set;

    if (!pSetupDiCreateDeviceInfoList || !pSetupDiDestroyDeviceInfoList ||
     !pSetupDiCreateDeviceInfoA || !pSetupDiCreateDeviceInterfaceA ||
     !pSetupDiEnumDeviceInterfaces)
    {
        skip("No SetupDiCreateDeviceInterfaceA\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInterfaceA(NULL, NULL, NULL, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInterfaceA(NULL, NULL, &guid, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    set = pSetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != NULL, "SetupDiCreateDeviceInfoList failed: %d\n", GetLastError());
    if (set)
    {
        SP_DEVINFO_DATA devInfo = { 0 };
        SP_DEVICE_INTERFACE_DATA interfaceData = { sizeof(interfaceData),
            { 0 } };
        DWORD i;

        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInterfaceA(set, NULL, NULL, NULL, 0, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, NULL, NULL, 0,
                NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        devInfo.cbSize = sizeof(devInfo);
        ret = pSetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid,
                NULL, NULL, 0, &devInfo);
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, NULL, NULL, 0,
                NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER,
         "Expected ERROR_INVALID_USER_BUFFER, got %08x\n", GetLastError());
        ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, &guid, NULL, 0,
                NULL);
        ok(ret, "SetupDiCreateDeviceInterfaceA failed: %08x\n", GetLastError());
        /* Creating the same interface a second time succeeds */
        ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, &guid, NULL, 0,
                NULL);
        ok(ret, "SetupDiCreateDeviceInterfaceA failed: %08x\n", GetLastError());
        ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, &guid, "Oogah", 0,
                NULL);
        ok(ret, "SetupDiCreateDeviceInterfaceA failed: %08x\n", GetLastError());
        ret = pSetupDiEnumDeviceInterfaces(set, &devInfo, &guid, 0,
                &interfaceData);
        ok(ret, "SetupDiEnumDeviceInterfaces failed: %d\n", GetLastError());
        i = 0;
        while (pSetupDiEnumDeviceInterfaces(set, &devInfo, &guid, i,
                    &interfaceData))
            i++;
        ok(i == 2, "expected 2 interfaces, got %d\n", i);
        ok(GetLastError() == ERROR_NO_MORE_ITEMS,
         "SetupDiEnumDeviceInterfaces failed: %08x\n", GetLastError());
        pSetupDiDestroyDeviceInfoList(set);
    }
}

static void testGetDeviceInterfaceDetail(void)
{
    BOOL ret;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    HDEVINFO set;

    if (!pSetupDiCreateDeviceInfoList || !pSetupDiDestroyDeviceInfoList ||
     !pSetupDiCreateDeviceInfoA || !pSetupDiCreateDeviceInterfaceA ||
     !pSetupDiGetDeviceInterfaceDetailA)
    {
        skip("No SetupDiGetDeviceInterfaceDetailA\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceInterfaceDetailA(NULL, NULL, NULL, 0, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    set = pSetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != NULL, "SetupDiCreateDeviceInfoList failed: %d\n", GetLastError());
    if (set)
    {
        SP_DEVINFO_DATA devInfo = { sizeof(devInfo), { 0 } };
        SP_DEVICE_INTERFACE_DATA interfaceData = { sizeof(interfaceData),
            { 0 } };
        DWORD size = 0;

        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInterfaceDetailA(set, NULL, NULL, 0, NULL,
                NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        ret = pSetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid,
                NULL, NULL, 0, &devInfo);
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, &guid, NULL, 0,
                &interfaceData);
        ok(ret, "SetupDiCreateDeviceInterfaceA failed: %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, NULL,
                0, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
         "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, NULL,
                100, NULL, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER,
         "Expected ERROR_INVALID_USER_BUFFER, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, NULL,
                0, &size, NULL);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
         "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
        if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            static const char path[] =
             "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
            LPBYTE buf = HeapAlloc(GetProcessHeap(), 0, size);
            SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail =
                (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)buf;
            DWORD expectedsize = offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath) + sizeof(WCHAR)*(1 + strlen(path));

            detail->cbSize = 0;
            SetLastError(0xdeadbeef);
            ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, detail,
                    size, &size, NULL);
            ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER,
             "Expected ERROR_INVALID_USER_BUFFER, got %08x\n", GetLastError());
            detail->cbSize = size;
            SetLastError(0xdeadbeef);
            ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, detail,
                    size, &size, NULL);
            ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER,
             "Expected ERROR_INVALID_USER_BUFFER, got %08x\n", GetLastError());
            detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
            SetLastError(0xdeadbeef);
            ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, detail,
                    size, &size, NULL);
            ok(!ret && GetLastError() == ERROR_INVALID_USER_BUFFER,
             "Expected ERROR_INVALID_USER_BUFFER, got %08x\n", GetLastError());
            /* Windows 2000 and up check for the exact size. Win9x returns ERROR_INVALID_PARAMETER
             * on every call (so doesn't get here) and NT4 doesn't have this function.
             */
            detail->cbSize = FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath[1]);
            ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, detail,
                    size, &size, NULL);
            ok(ret, "SetupDiGetDeviceInterfaceDetailA failed: %d\n",
                    GetLastError());
            ok(!lstrcmpiA(path, detail->DevicePath), "Unexpected path %s\n",
                    detail->DevicePath);
            /* Check SetupDiGetDeviceInterfaceDetailW */
            if (pSetupDiGetDeviceInterfaceDetailW)
            {
                ret = pSetupDiGetDeviceInterfaceDetailW(set, &interfaceData, NULL, 0, &size, NULL);
                ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Expected ERROR_INSUFFICIENT_BUFFER, got error code: %d\n", GetLastError());
                ok(expectedsize == size, "SetupDiGetDeviceInterfaceDetailW returned wrong reqsize: expected %d, got %d\n", expectedsize, size);
            }
            else
                skip("SetupDiGetDeviceInterfaceDetailW is not available\n");

            HeapFree(GetProcessHeap(), 0, buf);
        }
        pSetupDiDestroyDeviceInfoList(set);
    }
}

static void testDevRegKey(void)
{
    static const WCHAR classKey[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'C','o','n','t','r','o','l','\\','C','l','a','s','s','\\',
     '{','6','a','5','5','b','5','a','4','-','3','f','6','5','-',
     '1','1','d','b','-','b','7','0','4','-',
     '0','0','1','1','9','5','5','c','2','b','d','b','}',0};
    BOOL ret;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    HDEVINFO set;

    if (!pSetupDiCreateDeviceInfoList || !pSetupDiDestroyDeviceInfoList ||
     !pSetupDiCreateDeviceInfoA || !pSetupDiOpenDevRegKey ||
     !pSetupDiRegisterDeviceInfo || !pSetupDiCreateDevRegKeyW ||
     !pSetupDiCallClassInstaller)
    {
        skip("No SetupDiOpenDevRegKey\n");
        return;
    }
    set = pSetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != NULL, "SetupDiCreateDeviceInfoList failed: %d\n", GetLastError());
    if (set)
    {
        SP_DEVINFO_DATA devInfo = { sizeof(devInfo), { 0 } };
        HKEY key = INVALID_HANDLE_VALUE;

        ret = pSetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid,
                NULL, NULL, 0, &devInfo);
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        key = pSetupDiOpenDevRegKey(NULL, NULL, 0, 0, 0, 0);
        ok(key == INVALID_HANDLE_VALUE &&
         GetLastError() == ERROR_INVALID_HANDLE,
         "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        key = pSetupDiOpenDevRegKey(set, NULL, 0, 0, 0, 0);
        ok(key == INVALID_HANDLE_VALUE &&
         GetLastError() == ERROR_INVALID_PARAMETER,
         "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        key = pSetupDiOpenDevRegKey(set, &devInfo, 0, 0, 0, 0);
        ok(key == INVALID_HANDLE_VALUE &&
         GetLastError() == ERROR_INVALID_FLAGS,
         "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        key = pSetupDiOpenDevRegKey(set, &devInfo, DICS_FLAG_GLOBAL, 0, 0, 0);
        ok(key == INVALID_HANDLE_VALUE &&
         GetLastError() == ERROR_INVALID_FLAGS,
         "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        key = pSetupDiOpenDevRegKey(set, &devInfo, DICS_FLAG_GLOBAL, 0,
         DIREG_BOTH, 0);
        ok(key == INVALID_HANDLE_VALUE &&
         GetLastError() == ERROR_INVALID_FLAGS,
         "Expected ERROR_INVALID_FLAGS, got %d\n", GetLastError());
        SetLastError(0xdeadbeef);
        key = pSetupDiOpenDevRegKey(set, &devInfo, DICS_FLAG_GLOBAL, 0,
         DIREG_DRV, 0);
        ok(key == INVALID_HANDLE_VALUE &&
         GetLastError() == ERROR_DEVINFO_NOT_REGISTERED,
         "Expected ERROR_DEVINFO_NOT_REGISTERED, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = pSetupDiRegisterDeviceInfo(set, &devInfo, 0, NULL, NULL, NULL);
        ok(ret, "SetupDiRegisterDeviceInfo failed: %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        key = pSetupDiOpenDevRegKey(set, &devInfo, DICS_FLAG_GLOBAL, 0,
         DIREG_DRV, 0);
        /* The software key isn't created by default */
        todo_wine
        ok(key == INVALID_HANDLE_VALUE &&
         GetLastError() == ERROR_KEY_DOES_NOT_EXIST,
         "Expected ERROR_KEY_DOES_NOT_EXIST_EXIST, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        key = pSetupDiOpenDevRegKey(set, &devInfo, DICS_FLAG_GLOBAL, 0,
         DIREG_DEV, 0);
        todo_wine
        ok(key == INVALID_HANDLE_VALUE &&
         GetLastError() == ERROR_KEY_DOES_NOT_EXIST,
         "Expected ERROR_KEY_DOES_NOT_EXIST_EXIST, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        key = pSetupDiCreateDevRegKeyW(set, &devInfo, DICS_FLAG_GLOBAL, 0,
         DIREG_DRV, NULL, NULL);
        ok(key != INVALID_HANDLE_VALUE, "SetupDiCreateDevRegKey failed: %08x\n",
         GetLastError());
        RegCloseKey(key);
        SetLastError(0xdeadbeef);
        key = pSetupDiOpenDevRegKey(set, &devInfo, DICS_FLAG_GLOBAL, 0,
         DIREG_DRV, 0);
        todo_wine
        ok(key == INVALID_HANDLE_VALUE &&
         GetLastError() == ERROR_INVALID_DATA,
         "Expected ERROR_INVALID_DATA, got %08x\n", GetLastError());
        key = pSetupDiOpenDevRegKey(set, &devInfo, DICS_FLAG_GLOBAL, 0,
         DIREG_DRV, KEY_READ);
        ok(key != INVALID_HANDLE_VALUE, "SetupDiOpenDevRegKey failed: %08x\n",
         GetLastError());
        ret = pSetupDiCallClassInstaller(DIF_REMOVE, set, &devInfo);
        pSetupDiDestroyDeviceInfoList(set);
    }
    devinst_RegDeleteTreeW(HKEY_LOCAL_MACHINE, classKey);
}

static void testRegisterAndGetDetail(void)
{
    HDEVINFO set;
    BOOL ret;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    SP_DEVINFO_DATA devInfo = { sizeof(SP_DEVINFO_DATA), { 0 } };
    SP_DEVICE_INTERFACE_DATA interfaceData = { sizeof(interfaceData), { 0 } };
    DWORD dwSize = 0;

    SetLastError(0xdeadbeef);
    set = pSetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevsA failed: %08x\n",
     GetLastError());

    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, 0,
     DICD_GENERATE_ID, &devInfo);
    ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInterfaceA(set, &devInfo, &guid, NULL, 0, &interfaceData);
    ok(ret, "SetupDiCreateDeviceInterfaceA failed: %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiRegisterDeviceInfo(set, &devInfo, 0, NULL, NULL, NULL);
    ok(ret, "SetupDiRegisterDeviceInfo failed: %08x\n", GetLastError());

    pSetupDiDestroyDeviceInfoList(set);

    SetLastError(0xdeadbeef);
    set = pSetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevsA failed: %08x\n",
     GetLastError());

    SetLastError(0xdeadbeef);
    ret = pSetupDiEnumDeviceInterfaces(set, NULL, &guid, 0, &interfaceData);
    ok(ret, "SetupDiEnumDeviceInterfaces failed: %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData, NULL, 0, &dwSize, NULL);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
     "Expected ERROR_INSUFFICIENT_BUFFER, got %08x\n", GetLastError());
    if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        static const char path[] =
            "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
        PSP_DEVICE_INTERFACE_DETAIL_DATA_A detail = NULL;

        detail = (PSP_DEVICE_INTERFACE_DETAIL_DATA_A)HeapAlloc(GetProcessHeap(), 0, dwSize);
        if (detail)
        {
            detail->cbSize = offsetof(SP_DEVICE_INTERFACE_DETAIL_DATA_A, DevicePath) + sizeof(char);
            SetLastError(0xdeadbeef);
            ret = pSetupDiGetDeviceInterfaceDetailA(set, &interfaceData,
             detail, dwSize, &dwSize, NULL);
            ok(ret, "SetupDiGetDeviceInterfaceDetailA failed: %08x\n", GetLastError());
            ok(!lstrcmpiA(path, detail->DevicePath), "Unexpected path %s\n",
                    detail->DevicePath);
            HeapFree(GetProcessHeap(), 0, detail);
        }
    }

    pSetupDiDestroyDeviceInfoList(set);
}

static void testDeviceRegistryPropertyA()
{
    HDEVINFO set;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    SP_DEVINFO_DATA devInfo = { sizeof(SP_DEVINFO_DATA), { 0 } };
    CHAR devName[] = "LEGACY_BOGUS";
    CHAR friendlyName[] = "Bogus";
    CHAR buf[6] = "";
    DWORD buflen = 6;
    DWORD size;
    DWORD regType;
    BOOL ret;

    SetLastError(0xdeadbeef);
    set = pSetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevsA failed: %08x\n",
     GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInfoA(set, devName, &guid, NULL, NULL,
     DICD_GENERATE_ID, &devInfo);
    ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDeviceRegistryPropertyA(NULL, NULL, -1, NULL, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDeviceRegistryPropertyA(set, NULL, -1, NULL, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDeviceRegistryPropertyA(set, &devInfo, -1, NULL, 0);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_REG_PROPERTY,
     "Expected ERROR_INVALID_REG_PROPERTY, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDeviceRegistryPropertyA(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, 0);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_DATA,
     "Expected ERROR_INVALID_DATA, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDeviceRegistryPropertyA(set, &devInfo, SPDRP_FRIENDLYNAME,
     (PBYTE)friendlyName, buflen);
    ok(ret, "SetupDiSetDeviceRegistryPropertyA failed: %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyA(NULL, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyA(set, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyA(set, &devInfo, -1, NULL, NULL, 0, NULL);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_REG_PROPERTY,
     "Expected ERROR_INVALID_REG_PROPERTY, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyA(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, NULL, buflen, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA,
     "Expected ERROR_INVALID_DATA, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyA(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
     "Expected ERROR_INSUFFICIENT_BUFFER, got %08x\n", GetLastError());
    ok(buflen == size, "Unexpected size: %d\n", size);
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyA(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, (PBYTE)buf, buflen, NULL);
    ok(ret, "SetupDiGetDeviceRegistryPropertyA failed: %08x\n", GetLastError());
    ok(!lstrcmpiA(friendlyName, buf), "Unexpected property\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyA(set, &devInfo, SPDRP_FRIENDLYNAME,
     &regType, (PBYTE)buf, buflen, NULL);
    ok(ret, "SetupDiGetDeviceRegistryPropertyA failed: %08x\n", GetLastError());
    ok(!lstrcmpiA(friendlyName, buf), "Unexpected value of property\n");
    ok(regType == REG_SZ, "Unexpected type of property: %d\n", regType);
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDeviceRegistryPropertyA(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, 0);
    ok(ret, "SetupDiSetDeviceRegistryPropertyA failed: %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyA(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, (PBYTE)buf, buflen, &size);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_DATA,
     "Expected ERROR_INVALID_DATA, got %08x\n", GetLastError());
    pSetupDiDestroyDeviceInfoList(set);
}

static void testDeviceRegistryPropertyW()
{
    HDEVINFO set;
    GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,
        0x00,0x11,0x95,0x5c,0x2b,0xdb}};
    SP_DEVINFO_DATA devInfo = { sizeof(SP_DEVINFO_DATA), { 0 } };
    WCHAR devName[] = {'L','E','G','A','C','Y','_','B','O','G','U','S',0};
    WCHAR friendlyName[] = {'B','o','g','u','s',0};
    WCHAR buf[6] = {0};
    DWORD buflen = 6 * sizeof(WCHAR);
    DWORD size;
    DWORD regType;
    BOOL ret;

    SetLastError(0xdeadbeef);
    set = pSetupDiGetClassDevsW(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    if (set == INVALID_HANDLE_VALUE && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        skip("W-functions are not implemented\n");
        return;
    }
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevsW failed: %08x\n",
     GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInfoW(set, devName, &guid, NULL, NULL,
     DICD_GENERATE_ID, &devInfo);
    ok(ret, "SetupDiCreateDeviceInfoW failed: %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDeviceRegistryPropertyW(NULL, NULL, -1, NULL, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDeviceRegistryPropertyW(set, NULL, -1, NULL, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDeviceRegistryPropertyW(set, &devInfo, -1, NULL, 0);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_REG_PROPERTY,
     "Expected ERROR_INVALID_REG_PROPERTY, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDeviceRegistryPropertyW(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, 0);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_DATA,
     "Expected ERROR_INVALID_DATA, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDeviceRegistryPropertyW(set, &devInfo, SPDRP_FRIENDLYNAME,
     (PBYTE)friendlyName, buflen);
    ok(ret, "SetupDiSetDeviceRegistryPropertyW failed: %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyW(NULL, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyW(set, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
     "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyW(set, &devInfo, -1, NULL, NULL, 0, NULL);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_REG_PROPERTY,
     "Expected ERROR_INVALID_REG_PROPERTY, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyW(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, NULL, buflen, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA,
     "Expected ERROR_INVALID_DATA, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyW(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
     "Expected ERROR_INSUFFICIENT_BUFFER, got %08x\n", GetLastError());
    ok(buflen == size, "Unexpected size: %d\n", size);
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyW(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, (PBYTE)buf, buflen, NULL);
    ok(ret, "SetupDiGetDeviceRegistryPropertyW failed: %08x\n", GetLastError());
    ok(!lstrcmpiW(friendlyName, buf), "Unexpected property\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyW(set, &devInfo, SPDRP_FRIENDLYNAME,
     &regType, (PBYTE)buf, buflen, NULL);
    ok(ret, "SetupDiGetDeviceRegistryPropertyW failed: %08x\n", GetLastError());
    ok(!lstrcmpiW(friendlyName, buf), "Unexpected value of property\n");
    ok(regType == REG_SZ, "Unexpected type of property: %d\n", regType);
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDeviceRegistryPropertyW(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, 0);
    ok(ret, "SetupDiSetDeviceRegistryPropertyW failed: %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceRegistryPropertyW(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, (PBYTE)buf, buflen, &size);
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_DATA,
     "Expected ERROR_INVALID_DATA, got %08x\n", GetLastError());
    pSetupDiDestroyDeviceInfoList(set);
}

START_TEST(devinst)
{
    init_function_pointers();

    if (pSetupDiCreateDeviceInfoListExW && pSetupDiDestroyDeviceInfoList)
        test_SetupDiCreateDeviceInfoListEx();
    else
        skip("SetupDiCreateDeviceInfoListExW and/or SetupDiDestroyDeviceInfoList not available\n");

    if (pSetupDiOpenClassRegKeyExA)
        test_SetupDiOpenClassRegKeyExA();
    else
        skip("SetupDiOpenClassRegKeyExA is not available\n");
    testInstallClass();
    testCreateDeviceInfo();
    testGetDeviceInstanceId();
    testRegisterDeviceInfo();
    testCreateDeviceInterface();
    testGetDeviceInterfaceDetail();
    testDevRegKey();
    testRegisterAndGetDetail();
    testDeviceRegistryPropertyA();
    testDeviceRegistryPropertyW();
}
