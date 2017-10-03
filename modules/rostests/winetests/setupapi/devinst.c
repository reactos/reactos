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

static BOOL is_wow64;

/* function pointers */
static HDEVINFO (WINAPI *pSetupDiCreateDeviceInfoList)(GUID*,HWND);
static HDEVINFO (WINAPI *pSetupDiCreateDeviceInfoListExW)(GUID*,HWND,PCWSTR,PVOID);
static BOOL     (WINAPI *pSetupDiCreateDeviceInterfaceA)(HDEVINFO, PSP_DEVINFO_DATA, const GUID *, PCSTR, DWORD, PSP_DEVICE_INTERFACE_DATA);
static BOOL     (WINAPI *pSetupDiCallClassInstaller)(DI_FUNCTION, HDEVINFO, PSP_DEVINFO_DATA);
static BOOL     (WINAPI *pSetupDiDestroyDeviceInfoList)(HDEVINFO);
static BOOL     (WINAPI *pSetupDiEnumDeviceInfo)(HDEVINFO, DWORD, PSP_DEVINFO_DATA);
static BOOL     (WINAPI *pSetupDiEnumDeviceInterfaces)(HDEVINFO, PSP_DEVINFO_DATA, const GUID *, DWORD, PSP_DEVICE_INTERFACE_DATA);
static BOOL     (WINAPI *pSetupDiGetINFClassA)(PCSTR, LPGUID, PSTR, DWORD, PDWORD);
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
static HDEVINFO (WINAPI *pSetupDiGetClassDevsA)(const GUID *, LPCSTR, HWND, DWORD);
static HDEVINFO (WINAPI *pSetupDiGetClassDevsW)(const GUID *, LPCWSTR, HWND, DWORD);
static BOOL     (WINAPI *pSetupDiSetDeviceRegistryPropertyA)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, const BYTE *, DWORD);
static BOOL     (WINAPI *pSetupDiSetDeviceRegistryPropertyW)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, const BYTE *, DWORD);
static BOOL     (WINAPI *pSetupDiGetDeviceRegistryPropertyA)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);
static BOOL     (WINAPI *pSetupDiGetDeviceRegistryPropertyW)(HDEVINFO, PSP_DEVINFO_DATA, DWORD, PDWORD, PBYTE, DWORD, PDWORD);
static BOOL     (WINAPI *pSetupDiRemoveDevice)(HDEVINFO, PSP_DEVINFO_DATA);
static BOOL     (WINAPI *pSetupDiRemoveDeviceInterface)(HDEVINFO, PSP_DEVICE_INTERFACE_DATA);
static BOOL     (WINAPI *pIsWow64Process)(HANDLE, PBOOL);

/* This is a unique guid for testing purposes */
static GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,0x00,0x11,0x95,0x5c,0x2b,0xdb}};

static void init_function_pointers(void)
{
    HMODULE hSetupAPI = GetModuleHandleA("setupapi.dll");
    HMODULE hKernel32 = GetModuleHandleA("kernel32.dll");

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
    pSetupDiGetINFClassA = (void *)GetProcAddress(hSetupAPI, "SetupDiGetINFClassA");
    pSetupDiSetDeviceRegistryPropertyA = (void *)GetProcAddress(hSetupAPI, "SetupDiSetDeviceRegistryPropertyA");
    pSetupDiSetDeviceRegistryPropertyW = (void *)GetProcAddress(hSetupAPI, "SetupDiSetDeviceRegistryPropertyW");
    pSetupDiGetDeviceRegistryPropertyA = (void *)GetProcAddress(hSetupAPI, "SetupDiGetDeviceRegistryPropertyA");
    pSetupDiGetDeviceRegistryPropertyW = (void *)GetProcAddress(hSetupAPI, "SetupDiGetDeviceRegistryPropertyW");
    pSetupDiRemoveDeviceInterface = (void *)GetProcAddress(hSetupAPI, "SetupDiRemoveDeviceInterface");
    pSetupDiRemoveDevice = (void *)GetProcAddress(hSetupAPI, "SetupDiRemoveDevice");
    pIsWow64Process = (void *)GetProcAddress(hKernel32, "IsWow64Process");
}

static LSTATUS devinst_RegDeleteTreeW(HKEY hKey, LPCWSTR lpszSubKey)
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
    static const WCHAR empty[] = { 0 };

    SetLastError(0xdeadbeef);
    /* create empty DeviceInfoList, but set Reserved to a value, which is not NULL */
    devlist = pSetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, notnull);

    error = GetLastError();
    if (error == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetupDiCreateDeviceInfoListExW is not implemented\n");
        return;
    }
    ok(devlist == INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %d (expected %p)\n", devlist, error, INVALID_HANDLE_VALUE);
    ok(error == ERROR_INVALID_PARAMETER, "GetLastError returned wrong value : %d, (expected %d)\n", error, ERROR_INVALID_PARAMETER);

    SetLastError(0xdeadbeef);
    /* create empty DeviceInfoList, but set MachineName to something */
    devlist = pSetupDiCreateDeviceInfoListExW(NULL, NULL, machine, NULL);

    error = GetLastError();
    if (error == ERROR_CALL_NOT_IMPLEMENTED)
    {
        /* win10 reports ERROR_CALL_NOT_IMPLEMENTED at first here */
        win_skip("SetupDiCreateDeviceInfoListExW is not implemented\n");
        return;
    }
    ok(devlist == INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %d (expected %p)\n", devlist, error, INVALID_HANDLE_VALUE);
    ok(error == ERROR_INVALID_MACHINENAME || error == ERROR_MACHINE_UNAVAILABLE, "GetLastError returned wrong value : %d, (expected %d or %d)\n", error, ERROR_INVALID_MACHINENAME, ERROR_MACHINE_UNAVAILABLE);

    /* create empty DeviceInfoList */
    devlist = pSetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
    ok(devlist && devlist != INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %d (expected != %p)\n", devlist, error, INVALID_HANDLE_VALUE);

    /* destroy DeviceInfoList */
    ret = pSetupDiDestroyDeviceInfoList(devlist);
    ok(ret, "SetupDiDestroyDeviceInfoList failed : %d\n", error);

    /* create empty DeviceInfoList with empty machine name */
    devlist = pSetupDiCreateDeviceInfoListExW(NULL, NULL, empty, NULL);
    ok(devlist && devlist != INVALID_HANDLE_VALUE, "SetupDiCreateDeviceInfoListExW failed : %p %d (expected != %p)\n", devlist, error, INVALID_HANDLE_VALUE);

    /* destroy DeviceInfoList */
    ret = pSetupDiDestroyDeviceInfoList(devlist);
    ok(ret, "SetupDiDestroyDeviceInfoList failed : %d\n", error);
}

static void test_SetupDiOpenClassRegKeyExA(void)
{
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

        RegCloseKey(hkey);
    }
    else
        trace("failed to open classes key %u\n", GetLastError());
}

static void create_inf_file(LPCSTR filename)
{
    DWORD dwNumberOfBytesWritten;
    HANDLE hf = CreateFileA(filename, GENERIC_WRITE, 0, NULL,
                            CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    static const char data[] =
        "[Version]\n"
        "Signature=\"$Chicago$\"\n"
        "Class=Bogus\n"
        "ClassGUID={6a55b5a4-3f65-11db-b704-0011955c2bdb}\n"
        "[ClassInstall32]\n"
        "AddReg=BogusClass.NT.AddReg\n"
        "[BogusClass.NT.AddReg]\n"
        "HKR,,,,\"Wine test devices\"\n";

    WriteFile(hf, data, sizeof(data) - 1, &dwNumberOfBytesWritten, NULL);
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
    char tmpfile[MAX_PATH];
    BOOL ret;

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

    ok(!RegDeleteKeyW(HKEY_LOCAL_MACHINE, classKey),
     "Couldn't delete classkey\n");

    DeleteFileA(tmpfile);
}

static void testCreateDeviceInfo(void)
{
    BOOL ret;
    HDEVINFO set;

    SetLastError(0xdeadbeef);
    ret = pSetupDiCreateDeviceInfoA(NULL, NULL, NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_DEVINST_NAME ||
      GetLastError() == ERROR_INVALID_PARAMETER /* NT4 */,
     "Unexpected last error, got %08x\n", GetLastError());

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
        static GUID deadbeef =
         {0xdeadbeef, 0xdead, 0xbeef, {0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef}};

        /* No GUID given */
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", NULL,
         NULL, NULL, 0, NULL);
        ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
            "Expected ERROR_INVALID_PARAMETER, got %08x\n", GetLastError());

        /* We can't add device information to the set with a different GUID */
        SetLastError(0xdeadbeef);
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000",
         &deadbeef, NULL, NULL, 0, NULL);
        ok(!ret && GetLastError() == ERROR_CLASS_MISMATCH,
         "Expected ERROR_CLASS_MISMATCH, got %08x\n", GetLastError());

        /* Finally, with all three required parameters, this succeeds: */
        ret = pSetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid,
         NULL, NULL, 0, NULL);
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
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
        /* and this finally succeeds. */
        devInfo.cbSize = sizeof(devInfo);
        ret = pSetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL,
         DICD_GENERATE_ID, &devInfo);
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

        ret = pSetupDiRemoveDevice(set, &devInfo);
        todo_wine ok(ret, "got %u\n", GetLastError());
        pSetupDiDestroyDeviceInfoList(set);
    }
}

static void testGetDeviceInstanceId(void)
{
    BOOL ret;
    HDEVINFO set;
    SP_DEVINFO_DATA devInfo = { 0 };

    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceInstanceIdA(NULL, NULL, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceInstanceIdA(NULL, &devInfo, NULL, 0, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %08x\n", GetLastError());
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
        /* NT4 returns 'Root' and W2K and above 'ROOT' */
        ok(!lstrcmpiA(instanceID, "ROOT\\LEGACY_BOGUS\\0001"),
         "Unexpected instance ID %s\n", instanceID);

        ret = pSetupDiRemoveDevice(set, &devInfo);
        todo_wine ok(ret, "got %u\n", GetLastError());
        pSetupDiDestroyDeviceInfoList(set);
    }
}

static void testRegisterDeviceInfo(void)
{
    static const WCHAR bogus[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'E','n','u','m','\\','U','S','B','\\','B','O','G','U','S',0};
    BOOL ret;
    HDEVINFO set;

    SetLastError(0xdeadbeef);
    ret = pSetupDiRegisterDeviceInfo(NULL, NULL, 0, NULL, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
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
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());

        ret = pSetupDiRegisterDeviceInfo(set, &devInfo, 0, NULL, NULL, NULL);
        ok(ret, "SetupDiRegisterDeviceInfo failed: %d\n", GetLastError());

        ret = pSetupDiRemoveDevice(set, &devInfo);
        todo_wine ok(ret, "got %u\n", GetLastError());
        pSetupDiDestroyDeviceInfoList(set);

        /* remove once Wine is fixed */
        devinst_RegDeleteTreeW(HKEY_LOCAL_MACHINE, bogus);
    }
}

static void testCreateDeviceInterface(void)
{
    BOOL ret;
    HDEVINFO set;

    if (!pSetupDiCreateDeviceInterfaceA || !pSetupDiEnumDeviceInterfaces)
    {
        win_skip("SetupDiCreateDeviceInterfaceA and/or SetupDiEnumDeviceInterfaces are not available\n");
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

        for (i = 0; i < 2; i++)
        {
            ret = pSetupDiEnumDeviceInterfaces(set, &devInfo, &guid, i, &interfaceData);
            ok(ret, "SetupDiEnumDeviceInterfaces failed: %08x\n", GetLastError());

            ret = pSetupDiRemoveDeviceInterface(set, &interfaceData);
            todo_wine ok(ret, "SetupDiRemoveDeviceInterface failed: %08x\n", GetLastError());
        }

        ret = pSetupDiRemoveDevice(set, &devInfo);
        todo_wine ok(ret, "got %u\n", GetLastError());

        ret = pSetupDiDestroyDeviceInfoList(set);
        ok(ret, "SetupDiDestroyDeviceInfoList failed: %08x\n", GetLastError());
    }
}

static void testGetDeviceInterfaceDetail(void)
{
    static const WCHAR bogus[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'E','n','u','m','\\','R','o','o','t','\\',
     'L','E','G','A','C','Y','_','B','O','G','U','S',0};
    static const WCHAR devclass[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'C','o','n','t','r','o','l','\\','D','e','v','i','c','e','C','l','a','s','s','e','s','\\',
     '{','6','a','5','5','b','5','a','4','-','3','f','6','5','-',
     '1','1','d','b','-','b','7','0','4','-',
     '0','0','1','1','9','5','5','c','2','b','d','b','}',0};
    BOOL ret;
    HDEVINFO set;

    if (!pSetupDiCreateDeviceInterfaceA || !pSetupDiGetDeviceInterfaceDetailA)
    {
        win_skip("SetupDiCreateDeviceInterfaceA and/or SetupDiGetDeviceInterfaceDetailA are not available\n");
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
            static const char path_w2k[] =
             "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\";
            SP_DEVINFO_DATA devinfo;
            LPBYTE buf = HeapAlloc(GetProcessHeap(), 0, size);
            SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail =
                (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)buf;
            DWORD expectedsize = FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath) + sizeof(WCHAR)*(1 + strlen(path));

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
            ok(ret, "SetupDiGetDeviceInterfaceDetailA failed: %d\n",
                    GetLastError());
            ok(!lstrcmpiA(path, detail->DevicePath) ||
             !lstrcmpiA(path_w2k, detail->DevicePath), "Unexpected path %s\n",
             detail->DevicePath);
            /* Check SetupDiGetDeviceInterfaceDetailW */
            memset(&devinfo, 0, sizeof(devinfo));
            devinfo.cbSize = sizeof(devinfo);
            ret = pSetupDiGetDeviceInterfaceDetailW(set, &interfaceData, NULL, 0, &size, &devinfo);
            ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
             "Expected ERROR_INSUFFICIENT_BUFFER, got error code: %d\n", GetLastError());
            ok(devinfo.DevInst, "Expected DevInst to be set\n");
            ok(expectedsize == size ||
             (expectedsize + sizeof(WCHAR)) == size /* W2K adds a backslash */,
             "SetupDiGetDeviceInterfaceDetailW returned wrong reqsize, got %d\n",
             size);

            HeapFree(GetProcessHeap(), 0, buf);
        }

        ret = pSetupDiEnumDeviceInterfaces(set, &devInfo, &guid, 0, &interfaceData);
        ok(ret, "SetupDiEnumDeviceInterfaces failed: %08x\n", GetLastError());

        ret = pSetupDiRemoveDeviceInterface(set, &interfaceData);
        todo_wine ok(ret, "SetupDiRemoveDeviceInterface failed: %08x\n", GetLastError());

        ret = pSetupDiRemoveDevice(set, &devInfo);
        todo_wine ok(ret, "got %u\n", GetLastError());
        pSetupDiDestroyDeviceInfoList(set);

        /* remove once Wine is fixed */
        devinst_RegDeleteTreeW(HKEY_LOCAL_MACHINE, bogus);
        devinst_RegDeleteTreeW(HKEY_LOCAL_MACHINE, devclass);
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
    static const WCHAR bogus[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'E','n','u','m','\\','R','o','o','t','\\',
     'L','E','G','A','C','Y','_','B','O','G','U','S',0};
    BOOL ret;
    HDEVINFO set;
    HKEY key = NULL;

    SetLastError(0xdeadbeef);
    key = pSetupDiCreateDevRegKeyW(NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(key == INVALID_HANDLE_VALUE,
     "Expected INVALID_HANDLE_VALUE, got %p\n", key);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
     "Expected ERROR_INVALID_HANDLE, got %08x\n", GetLastError());

    set = pSetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != NULL, "SetupDiCreateDeviceInfoList failed: %d\n", GetLastError());
    if (set)
    {
        SP_DEVINFO_DATA devInfo = { sizeof(devInfo), { 0 } };
        LONG res;

        /* The device info key shouldn't be there */
        res = RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key);
        ok(res != ERROR_SUCCESS, "Expected key to not exist\n");
        RegCloseKey(key);
        /* Create the device information */
        ret = pSetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid,
                NULL, NULL, 0, &devInfo);
        ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());
        /* The device info key should have been created */
        ok(!RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key),
         "Expected registry key to exist\n");
        RegCloseKey(key);
        SetLastError(0xdeadbeef);
        key = pSetupDiOpenDevRegKey(NULL, NULL, 0, 0, 0, 0);
        ok(!key || key == INVALID_HANDLE_VALUE,
         "Expected INVALID_HANDLE_VALUE or a NULL key (NT4)\n");
        ok(GetLastError() == ERROR_INVALID_HANDLE,
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
        ok(key == INVALID_HANDLE_VALUE &&
         GetLastError() == ERROR_KEY_DOES_NOT_EXIST,
         "Expected ERROR_KEY_DOES_NOT_EXIST, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        key = pSetupDiOpenDevRegKey(set, &devInfo, DICS_FLAG_GLOBAL, 0,
         DIREG_DEV, 0);
        todo_wine
        ok(key == INVALID_HANDLE_VALUE &&
         GetLastError() == ERROR_KEY_DOES_NOT_EXIST,
         "Expected ERROR_KEY_DOES_NOT_EXIST, got %08x\n", GetLastError());
        SetLastError(0xdeadbeef);
        /* The class key shouldn't be there */
        res = RegOpenKeyW(HKEY_LOCAL_MACHINE, classKey, &key);
        todo_wine
        ok(res != ERROR_SUCCESS, "Expected key to not exist\n");
        RegCloseKey(key);
        /* Create the device reg key */
        key = pSetupDiCreateDevRegKeyW(set, &devInfo, DICS_FLAG_GLOBAL, 0,
         DIREG_DRV, NULL, NULL);
        /* Vista and higher don't actually create the key */
        ok(key != INVALID_HANDLE_VALUE || GetLastError() == ERROR_KEY_DOES_NOT_EXIST,
         "SetupDiCreateDevRegKey failed: %08x\n", GetLastError());
        if (key != INVALID_HANDLE_VALUE)
        {
            RegCloseKey(key);
            /* The class key should have been created */
            ok(!RegOpenKeyW(HKEY_LOCAL_MACHINE, classKey, &key),
             "Expected registry key to exist\n");
            RegCloseKey(key);
            SetLastError(0xdeadbeef);
            key = pSetupDiOpenDevRegKey(set, &devInfo, DICS_FLAG_GLOBAL, 0,
             DIREG_DRV, 0);
            todo_wine
            ok(key == INVALID_HANDLE_VALUE &&
             (GetLastError() == ERROR_INVALID_DATA ||
             GetLastError() == ERROR_ACCESS_DENIED), /* win2k3 */
             "Expected ERROR_INVALID_DATA or ERROR_ACCESS_DENIED, got %08x\n", GetLastError());
            key = pSetupDiOpenDevRegKey(set, &devInfo, DICS_FLAG_GLOBAL, 0,
             DIREG_DRV, KEY_READ);
            ok(key != INVALID_HANDLE_VALUE, "SetupDiOpenDevRegKey failed: %08x\n",
             GetLastError());
            RegCloseKey(key);
        }

        ret = pSetupDiRemoveDevice(set, &devInfo);
        todo_wine ok(ret, "got %u\n", GetLastError());
        pSetupDiDestroyDeviceInfoList(set);

        /* remove once Wine is fixed */
        devinst_RegDeleteTreeW(HKEY_LOCAL_MACHINE, bogus);
        devinst_RegDeleteTreeW(HKEY_LOCAL_MACHINE, classKey);
    }
}

static void testRegisterAndGetDetail(void)
{
    static const WCHAR classKey[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'C','o','n','t','r','o','l','\\','C','l','a','s','s','\\',
     '{','6','a','5','5','b','5','a','4','-','3','f','6','5','-',
     '1','1','d','b','-','b','7','0','4','-',
     '0','0','1','1','9','5','5','c','2','b','d','b','}',0};
    static const WCHAR bogus[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'E','n','u','m','\\','R','o','o','t','\\',
     'L','E','G','A','C','Y','_','B','O','G','U','S',0};
    HDEVINFO set, set2;
    BOOL ret;
    SP_DEVINFO_DATA devInfo = { sizeof(SP_DEVINFO_DATA), { 0 } };
    SP_DEVICE_INTERFACE_DATA interfaceData = { sizeof(interfaceData), { 0 } };
    SP_DEVICE_INTERFACE_DATA interfaceData2 = { sizeof(interfaceData2), { 0 } };
    DWORD dwSize = 0;
    HKEY key;
    LONG res;

    if (!pSetupDiCreateDeviceInterfaceA || !pSetupDiEnumDeviceInterfaces ||
        !pSetupDiGetDeviceInterfaceDetailA)
    {
        win_skip("Needed functions are not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    set = pSetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevsA failed: %08x\n",
     GetLastError());

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res != ERROR_SUCCESS, "Expected key to not exist\n");
    RegCloseKey(key);

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

    SetLastError(0xdeadbeef);
    set2 = pSetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set2 != INVALID_HANDLE_VALUE, "SetupDiGetClassDevsA failed: %08x\n",
     GetLastError());

    SetLastError(0xdeadbeef);
    ret = pSetupDiEnumDeviceInterfaces(set2, NULL, &guid, 0, &interfaceData2);
    ok(ret, "SetupDiEnumDeviceInterfaces failed: %08x\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pSetupDiGetDeviceInterfaceDetailA(set2, &interfaceData2, NULL, 0, &dwSize, NULL);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
     "Expected ERROR_INSUFFICIENT_BUFFER, got %08x\n", GetLastError());
    if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        static const char path[] =
            "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
        static const char path_w10[] =
            "\\\\?\\root#legacy_bogus#0001#{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
        static const char path_w2k[] =
            "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\";
        PSP_DEVICE_INTERFACE_DETAIL_DATA_A detail = NULL;

        detail = HeapAlloc(GetProcessHeap(), 0, dwSize);
        if (detail)
        {
            detail->cbSize = sizeof(*detail);
            SetLastError(0xdeadbeef);
            ret = pSetupDiGetDeviceInterfaceDetailA(set2, &interfaceData2,
             detail, dwSize, &dwSize, NULL);
            ok(ret, "SetupDiGetDeviceInterfaceDetailA failed: %08x\n", GetLastError());
            ok(!lstrcmpiA(path, detail->DevicePath) ||
               !lstrcmpiA(path_w10, detail->DevicePath) ||
               !lstrcmpiA(path_w2k, detail->DevicePath),
               "Unexpected path %s\n", detail->DevicePath);
            HeapFree(GetProcessHeap(), 0, detail);
        }
    }

    ret = pSetupDiRemoveDeviceInterface(set, &interfaceData);
    todo_wine ok(ret, "SetupDiRemoveDeviceInterface failed: %08x\n", GetLastError());

    ret = pSetupDiRemoveDevice(set, &devInfo);
    todo_wine ok(ret, "got %u\n", GetLastError());

    pSetupDiDestroyDeviceInfoList(set);
    pSetupDiDestroyDeviceInfoList(set2);

    /* remove once Wine is fixed */
    devinst_RegDeleteTreeW(HKEY_LOCAL_MACHINE, bogus);
    devinst_RegDeleteTreeW(HKEY_LOCAL_MACHINE, classKey);
}

static void testDeviceRegistryPropertyA(void)
{
    HDEVINFO set;
    SP_DEVINFO_DATA devInfo = { sizeof(SP_DEVINFO_DATA), { 0 } };
    CHAR devName[] = "LEGACY_BOGUS";
    CHAR friendlyName[] = "Bogus";
    CHAR buf[6] = "";
    DWORD buflen = 6;
    DWORD size;
    DWORD regType;
    BOOL ret;
    LONG res;
    HKEY key;
    static const CHAR bogus[] =
     "System\\CurrentControlSet\\Enum\\Root\\LEGACY_BOGUS";

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
    /* GetLastError() returns nonsense in win2k3 */
    ret = pSetupDiSetDeviceRegistryPropertyA(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, 0);
    todo_wine
    ok(!ret, "Expected failure, got %d\n", ret);
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
    /* GetLastError() returns nonsense in win2k3 */
    ret = pSetupDiGetDeviceRegistryPropertyA(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, NULL, buflen, NULL);
    ok(!ret, "Expected failure, got %d\n", ret);
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
    ret = pSetupDiGetDeviceRegistryPropertyA(set, &devInfo, SPDRP_HARDWAREID,
     NULL, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA,
     "Expected ERROR_INVALID_DATA, got %08x\n", GetLastError());
    pSetupDiDestroyDeviceInfoList(set);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, bogus, &key);
    if(!is_wow64)
        todo_wine ok(res == ERROR_FILE_NOT_FOUND, "Expected key to not exist\n");
    /* FIXME: Remove when Wine is fixed */
    if (res == ERROR_SUCCESS)
    {
        /* Wine doesn't delete the information currently */
        RegDeleteKeyA(HKEY_LOCAL_MACHINE, bogus);
    }
}

static void testDeviceRegistryPropertyW(void)
{
    HDEVINFO set;
    SP_DEVINFO_DATA devInfo = { sizeof(SP_DEVINFO_DATA), { 0 } };
    WCHAR devName[] = {'L','E','G','A','C','Y','_','B','O','G','U','S',0};
    WCHAR friendlyName[] = {'B','o','g','u','s',0};
    WCHAR buf[6] = {0};
    DWORD buflen = 6 * sizeof(WCHAR);
    DWORD size;
    DWORD regType;
    BOOL ret;
    LONG res;
    HKEY key;
    static const WCHAR bogus[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'E','n','u','m','\\','R','o','o','t','\\',
     'L','E','G','A','C','Y','_','B','O','G','U','S',0};

    SetLastError(0xdeadbeef);
    set = pSetupDiGetClassDevsW(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
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
    /* GetLastError() returns nonsense in win2k3 */
    ret = pSetupDiSetDeviceRegistryPropertyW(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, 0);
    todo_wine
    ok(!ret, "Expected failure, got %d\n", ret);
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
    /* GetLastError() returns nonsense in win2k3 */
    ret = pSetupDiGetDeviceRegistryPropertyW(set, &devInfo, SPDRP_FRIENDLYNAME,
     NULL, NULL, buflen, NULL);
    ok(!ret, "Expected failure, got %d\n", ret);
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
    ret = pSetupDiGetDeviceRegistryPropertyW(set, &devInfo, SPDRP_HARDWAREID,
     NULL, NULL, 0, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_DATA,
     "Expected ERROR_INVALID_DATA, got %08x\n", GetLastError());
    pSetupDiDestroyDeviceInfoList(set);

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key);
    if(!is_wow64)
        todo_wine ok(res == ERROR_FILE_NOT_FOUND, "Expected key to not exist\n");
    /* FIXME: Remove when Wine is fixed */
    if (res == ERROR_SUCCESS)
    {
        /* Wine doesn't delete the information currently */
        RegDeleteKeyW(HKEY_LOCAL_MACHINE, bogus);
    }
}

static void testSetupDiGetINFClassA(void)
{
    static const char inffile[] = "winetest.inf";
    static const char content[] = "[Version]\r\n\r\n";
    static const char* signatures[] = {"\"$CHICAGO$\"", "\"$Windows NT$\""};

    char cn[MAX_PATH];
    char filename[MAX_PATH];
    DWORD count;
    BOOL retval;
    GUID guid;
    HANDLE h;
    int i;

    if(!pSetupDiGetINFClassA)
    {
        win_skip("SetupDiGetINFClassA not present\n");
        return;
    }

    count = GetTempPathA(MAX_PATH, filename);
    if(!count)
    {
        win_skip("GetTempPathA failed\n");
        return;
    }

    strcat(filename, inffile);
    DeleteFileA(filename);

    /* not existing file */
    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    if (ERROR_CALL_NOT_IMPLEMENTED == GetLastError())
    {
        skip("SetupDiGetINFClassA is not implemented\n");
        return;
    }
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %u\n", GetLastError());

    /* missing file wins against other invalid parameter */
    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, NULL, cn, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, &guid, NULL, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, &guid, cn, 0, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, NULL);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %u\n", GetLastError());

    /* test file content */
    h = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    if(h == INVALID_HANDLE_VALUE)
    {
        win_skip("failed to create file %s (error %u)\n", filename, GetLastError());
        return;
    }
    CloseHandle( h);

    retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");

    for(i=0; i < sizeof(signatures)/sizeof(char*); i++)
    {
        trace("testing signature %s\n", signatures[i]);
        h = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);
        if(h == INVALID_HANDLE_VALUE)
        {
            win_skip("failed to create file %s (error %u)\n", filename, GetLastError());
            return;
        }
        WriteFile( h, content, sizeof(content), &count, NULL);
        CloseHandle( h);

        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");

        WritePrivateProfileStringA("Version", "Signature", signatures[i], filename);

        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");

        WritePrivateProfileStringA("Version", "Class", "WINE", filename);

        count = 0xdeadbeef;
        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(retval, "expected SetupDiGetINFClassA to succeed! error %u\n", GetLastError());
        ok(count == 5, "expected count==5, got %u\n", count);

        count = 0xdeadbeef;
        retval = SetupDiGetINFClassA(filename, &guid, cn, 5, &count);
        ok(retval, "expected SetupDiGetINFClassA to succeed! error %u\n", GetLastError());
        ok(count == 5, "expected count==5, got %u\n", count);

        count = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, cn, 4, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
            "expected error ERROR_INSUFFICIENT_BUFFER, got %u\n", GetLastError());
        ok(count == 5, "expected count==5, got %u\n", count);

        /* invalid parameter */
        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(NULL, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INVALID_PARAMETER == GetLastError(),
            "expected error ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, NULL, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INVALID_PARAMETER == GetLastError(),
            "expected error ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, NULL, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INVALID_PARAMETER == GetLastError(),
            "expected error ERROR_INVALID_PARAMETER, got %u\n", GetLastError());

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, cn, 0, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INSUFFICIENT_BUFFER == GetLastError() ||
           ERROR_INVALID_PARAMETER == GetLastError(),
            "expected error ERROR_INSUFFICIENT_BUFFER or ERROR_INVALID_PARAMETER, "
            "got %u\n", GetLastError());

        DeleteFileA(filename);

        WritePrivateProfileStringA("Version", "Signature", signatures[i], filename);
        WritePrivateProfileStringA("Version", "ClassGUID", "WINE", filename);

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(RPC_S_INVALID_STRING_UUID == GetLastError() ||
           ERROR_INVALID_PARAMETER == GetLastError(),
            "expected error RPC_S_INVALID_STRING_UUID or ERROR_INVALID_PARAMETER, "
            "got %u\n", GetLastError());

        /* network adapter guid */
        WritePrivateProfileStringA("Version", "ClassGUID",
                                   "{4d36e972-e325-11ce-bfc1-08002be10318}", filename);

        /* this test succeeds only if the guid is known to the system */
        count = 0xdeadbeef;
        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(retval, "expected SetupDiGetINFClassA to succeed! error %u\n", GetLastError());
        todo_wine
        ok(count == 4, "expected count==4, got %u(%s)\n", count, cn);

        DeleteFileA(filename);
    }
}

static void testSetupDiGetClassDevsA(void)
{
    static GUID displayguid = {0x4d36e968, 0xe325, 0x11ce, {0xbf,0xc1,0x08,0x00,0x2b,0xe1,0x03,0x18}};
    SP_DEVINFO_DATA devinfo;
    DISPLAY_DEVICEA disp;
    HDEVINFO set;
    BOOL ret;

    disp.cb = sizeof(disp);
    ok(EnumDisplayDevicesA(NULL, 0, &disp, 0), "EnumDisplayDevices failed: %08x\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = pSetupDiGetClassDevsA(&displayguid, disp.DeviceID, 0, 0);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevsA failed: %08x\n", GetLastError());

    devinfo.cbSize = sizeof(devinfo);
    ret = SetupDiEnumDeviceInfo(set, 0, &devinfo);
    ok(ret, "SetupDiEnumDeviceInfo failed: %08x\n", GetLastError());

    pSetupDiDestroyDeviceInfoList(set);
}

START_TEST(devinst)
{
    HKEY hkey;

    init_function_pointers();

    if ((hkey = SetupDiOpenClassRegKey(NULL, KEY_ALL_ACCESS)) == INVALID_HANDLE_VALUE)
    {
        skip("needs admin rights\n");
        return;
    }
    RegCloseKey(hkey);

    if (pIsWow64Process)
        pIsWow64Process(GetCurrentProcess(), &is_wow64);

    if (pSetupDiCreateDeviceInfoListExW)
        test_SetupDiCreateDeviceInfoListEx();
    else
        win_skip("SetupDiCreateDeviceInfoListExW is not available\n");

    if (pSetupDiOpenClassRegKeyExA)
        test_SetupDiOpenClassRegKeyExA();
    else
        win_skip("SetupDiOpenClassRegKeyExA is not available\n");

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
    testSetupDiGetClassDevsA();

    if (!winetest_interactive)
    {
        win_skip("testSetupDiGetINFClassA(), ROSTESTS-66.\n");
    }
    else
    {
        testSetupDiGetINFClassA();
    }
}
