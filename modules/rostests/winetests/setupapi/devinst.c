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
#include "devguid.h"
#include "initguid.h"
#include "devpkey.h"
#include "setupapi.h"
#include "cfgmgr32.h"
#include "cguid.h"

#include "wine/heap.h"
#include "wine/test.h"

#ifdef __REACTOS__
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
#endif

/* This is a unique guid for testing purposes */
static GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,0x00,0x11,0x95,0x5c,0x2b,0xdb}};
static GUID guid2 = {0x6a55b5a5, 0x3f65, 0x11db, {0xb7,0x04,0x00,0x11,0x95,0x5c,0x2b,0xdb}};
static GUID iface_guid = {0xdeadbeef, 0x3f65, 0x11db, {0xb7,0x04,0x00,0x11,0x95,0x5c,0x2b,0xdb}};
static GUID iface_guid2 = {0xdeadf00d, 0x3f65, 0x11db, {0xb7,0x04,0x00,0x11,0x95,0x5c,0x2b,0xdb}};

BOOL (WINAPI *pSetupDiSetDevicePropertyW)(HDEVINFO, PSP_DEVINFO_DATA, const DEVPROPKEY *, DEVPROPTYPE, const BYTE *, DWORD, DWORD);
BOOL (WINAPI *pSetupDiGetDevicePropertyW)(HDEVINFO, PSP_DEVINFO_DATA, const DEVPROPKEY *, DEVPROPTYPE *, BYTE *, DWORD, DWORD *, DWORD);

static BOOL wow64;

static void create_file(const char *name, const char *data)
{
    HANDLE file;
    DWORD size;
    BOOL ret;

    file = CreateFileA(name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(file != INVALID_HANDLE_VALUE, "Failed to create %s, error %lu.\n", name, GetLastError());
    ret = WriteFile(file, data, strlen(data), &size, NULL);
    ok(ret && size == strlen(data), "Failed to write %s, error %lu.\n", name, GetLastError());
    CloseHandle(file);
}

static void load_resource(const char *name, const char *filename)
{
    DWORD written;
    HANDLE file;
    HRSRC res;
    void *ptr;

    file = CreateFileA(filename, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    ok(file != INVALID_HANDLE_VALUE, "file creation failed, at %s, error %ld\n", filename, GetLastError());

    res = FindResourceA(NULL, name, "TESTDLL");
    ok( res != 0, "couldn't find resource\n" );
    ptr = LockResource( LoadResource( GetModuleHandleA(NULL), res ));
    WriteFile( file, ptr, SizeofResource( GetModuleHandleA(NULL), res ), &written, NULL );
    ok( written == SizeofResource( GetModuleHandleA(NULL), res ), "couldn't write resource\n" );
    CloseHandle( file );
}

static void test_create_device_list_ex(void)
{
    static const WCHAR machine[] = { 'd','u','m','m','y',0 };
    static const WCHAR empty[] = { 0 };
    static char notnull[] = "NotNull";
    HDEVINFO set;
    BOOL ret;

    SetLastError(0xdeadbeef);
    set = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, notnull);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiCreateDeviceInfoListExW(NULL, NULL, machine, NULL);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_MACHINENAME
            || GetLastError() == ERROR_MACHINE_UNAVAILABLE
            || GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
            "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoListExW(NULL, NULL, empty, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());
}

static void test_open_class_key(void)
{
    static const char guidstr[] = "{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
    HKEY root_key, class_key;
    LONG res;

    SetLastError(0xdeadbeef);
    class_key = SetupDiOpenClassRegKeyExA(&guid, KEY_ALL_ACCESS, DIOCR_INSTALLER, NULL, NULL);
    ok(class_key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_CLASS, "Got unexpected error %#lx.\n", GetLastError());

    root_key = SetupDiOpenClassRegKey(NULL, KEY_ALL_ACCESS);
    ok(root_key != INVALID_HANDLE_VALUE, "Failed to open root key, error %#lx.\n", GetLastError());

    res = RegCreateKeyA(root_key, guidstr, &class_key);
    ok(!res, "Failed to create class key, error %#lx.\n", GetLastError());
    RegCloseKey(class_key);

    SetLastError(0xdeadbeef);
    class_key = SetupDiOpenClassRegKeyExA(&guid, KEY_ALL_ACCESS, DIOCR_INSTALLER, NULL, NULL);
    ok(class_key != INVALID_HANDLE_VALUE, "Failed to open class key, error %#lx.\n", GetLastError());
    RegCloseKey(class_key);

    RegDeleteKeyA(root_key, guidstr);
    RegCloseKey(root_key);
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

static void test_install_class(void)
{
    static const WCHAR classKey[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'C','o','n','t','r','o','l','\\','C','l','a','s','s','\\',
     '{','6','a','5','5','b','5','a','4','-','3','f','6','5','-',
     '1','1','d','b','-','b','7','0','4','-',
     '0','0','1','1','9','5','5','c','2','b','d','b','}',0};
    char tmpfile[MAX_PATH];
    BOOL ret;

    static const char inf_data[] =
            "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "Class=Bogus\n"
            "ClassGUID={6a55b5a4-3f65-11db-b704-0011955c2bdb}\n"
            "[ClassInstall32]\n"
            "AddReg=BogusClass.NT.AddReg\n"
            "[BogusClass.NT.AddReg]\n"
            "HKR,,,,\"Wine test devices\"\n";

    tmpfile[0] = '.';
    tmpfile[1] = '\\';
    get_temp_filename(tmpfile + 2);
    create_file(tmpfile + 2, inf_data);

    ret = SetupDiInstallClassA(NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiInstallClassA(NULL, NULL, DI_NOVCP, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiInstallClassA(NULL, tmpfile + 2, DI_NOVCP, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiInstallClassA(NULL, tmpfile + 2, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Got unexpected error %#lx.\n", GetLastError());

    /* The next call will succeed. Information is put into the registry but the
     * location(s) is/are depending on the Windows version.
     */
    ret = SetupDiInstallClassA(NULL, tmpfile, 0, NULL);
    ok(ret, "Failed to install class, error %#lx.\n", GetLastError());

    ret = RegDeleteKeyW(HKEY_LOCAL_MACHINE, classKey);
    ok(!ret, "Failed to delete class key, error %lu.\n", GetLastError());
    DeleteFileA(tmpfile);
}

static void check_device_info_(int line, HDEVINFO set, int index, const GUID *class, const char *expect_id)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    char id[50];
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = SetupDiEnumDeviceInfo(set, index, &device);
    if (expect_id)
    {
        ok_(__FILE__, line)(ret, "Got unexpected error %#lx.\n", GetLastError());
        ok_(__FILE__, line)(IsEqualGUID(&device.ClassGuid, class),
                "Got unexpected class %s.\n", wine_dbgstr_guid(&device.ClassGuid));
        ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
        ok_(__FILE__, line)(ret, "Got unexpected error %#lx.\n", GetLastError());
        ok_(__FILE__, line)(!strcasecmp(id, expect_id), "Got unexpected id %s.\n", id);
    }
    else
    {
        ok_(__FILE__, line)(!ret, "Expected failure.\n");
        ok_(__FILE__, line)(GetLastError() == ERROR_NO_MORE_ITEMS,
                "Got unexpected error %#lx.\n", GetLastError());
    }
}
#define check_device_info(a,b,c,d) check_device_info_(__LINE__,a,b,c,d)

static void test_device_info(void)
{
    static const GUID deadbeef = {0xdeadbeef,0xdead,0xbeef,{0xde,0xad,0xbe,0xef,0xde,0xad,0xbe,0xef}};
    SP_DEVINFO_DATA device = {0}, ret_device = {sizeof(ret_device)};
    char id[MAX_DEVICE_ID_LEN + 2];
    HDEVINFO set;
    BOOL ret;
    INT i = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(NULL, NULL, NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DEVINST_NAME, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(NULL, "Root\\LEGACY_BOGUS\\0000", NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &deadbeef, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_CLASS_MISMATCH, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &GUID_NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_CLASS_MISMATCH, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, &guid, NULL);

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DEVINST_ALREADY_EXISTS, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0001", &guid, NULL, NULL, 0, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\0001");
    check_device_info(set, 2, &guid, NULL);

    device.cbSize = sizeof(device);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0002", &guid, NULL, NULL, 0, &device);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());
    ok(IsEqualGUID(&device.ClassGuid, &guid), "Got unexpected class %s.\n",
            wine_dbgstr_guid(&device.ClassGuid));
    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());
    ok(!strcmp(id, "ROOT\\LEGACY_BOGUS\\0002"), "Got unexpected id %s.\n", id);

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\0001");
    check_device_info(set, 2, &guid, "ROOT\\LEGACY_BOGUS\\0002");
    check_device_info(set, 3, &guid, NULL);

    ret = SetupDiEnumDeviceInfo(set, 0, &ret_device);
    ok(ret, "Failed to enumerate devices, error %#lx.\n", GetLastError());
    ret = SetupDiDeleteDeviceInfo(set, &ret_device);
    ok(ret, "Failed to delete device, error %#lx.\n", GetLastError());

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0001");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\0002");
    check_device_info(set, 2, &guid, NULL);

    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0001");

    ret = SetupDiEnumDeviceInfo(set, 1, &ret_device);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());
    ok(IsEqualGUID(&ret_device.ClassGuid, &guid), "Got unexpected class %s.\n",
            wine_dbgstr_guid(&ret_device.ClassGuid));
    ret = SetupDiGetDeviceInstanceIdA(set, &ret_device, id, sizeof(id), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_NO_SUCH_DEVINST, "Got unexpected error %#lx.\n", GetLastError());
    ok(ret_device.DevInst == device.DevInst, "Expected device node %#lx, got %#lx.\n",
            device.DevInst, ret_device.DevInst);

    check_device_info(set, 2, &guid, NULL);

    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\deadbeef", &deadbeef, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\null", &GUID_NULL, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\testguid", &guid, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    check_device_info(set, 0, &deadbeef, "ROOT\\LEGACY_BOGUS\\deadbeef");
    check_device_info(set, 1, &GUID_NULL, "ROOT\\LEGACY_BOGUS\\null");
    check_device_info(set, 2, &guid, "ROOT\\LEGACY_BOGUS\\testguid");
    check_device_info(set, 3, NULL, NULL);

    memset(id, 'x', sizeof(id));
    memcpy(id, "Root\\LEGACY_BOGUS\\", strlen("Root\\LEGACY_BOGUS\\"));
    id[MAX_DEVICE_ID_LEN + 1] = 0;
    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, id, &guid, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DEVINST_NAME, "Got unexpected error %#lx.\n", GetLastError());

    id[MAX_DEVICE_ID_LEN] = 0;
    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, id, &guid, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DEVINST_NAME, "Got unexpected error %#lx.\n", GetLastError());

    id[MAX_DEVICE_ID_LEN - 1] = 0;
    ret = SetupDiCreateDeviceInfoA(set, id, &guid, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set , &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, NULL, NULL);
    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(!ret, "Expect failure\n");
    ok(GetLastError() == ERROR_DEVINST_ALREADY_EXISTS, "Got error %#lx\n", GetLastError());
    check_device_info(set, 0, NULL, NULL);
    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, 0);
    while (SetupDiEnumDeviceInfo(set, i++, &device))
    {
        ret = SetupDiRemoveDevice(set, &device);
        ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());
    }
    SetupDiDestroyDeviceInfoList(set);
}

static void test_device_property(void)
{
    static const WCHAR valueW[] = {'d', 'e', 'a', 'd', 'b', 'e', 'e', 'f', 0};
    SP_DEVINFO_DATA device_data = {sizeof(device_data)};
    HMODULE hmod;
    HDEVINFO set;
    DEVPROPTYPE type;
    DWORD size;
    BYTE buffer[256];
    DWORD err;
    BOOL ret;

    hmod = LoadLibraryA("setupapi.dll");
    pSetupDiSetDevicePropertyW = (void *)GetProcAddress(hmod, "SetupDiSetDevicePropertyW");
    pSetupDiGetDevicePropertyW = (void *)GetProcAddress(hmod, "SetupDiGetDevicePropertyW");

    if (!pSetupDiSetDevicePropertyW || !pSetupDiGetDevicePropertyW)
    {
        win_skip("SetupDi{Set,Get}DevicePropertyW() are only available on vista+, skipping tests.\n");
        FreeLibrary(hmod);
        return;
    }

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device_data);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* SetupDiSetDevicePropertyW */
    /* #1 Null device info list */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(NULL, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_HANDLE, "Expect last error %#x, got %#lx\n", ERROR_INVALID_HANDLE, err);

    /* #2 Null device */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, NULL, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_PARAMETER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_PARAMETER, err);

    /* #3 Null property key pointer */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, NULL, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_DATA, "Expect last error %#x, got %#lx\n", ERROR_INVALID_DATA, err);

    /* #4 Invalid property key type */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, 0xffff, (const BYTE *)valueW, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_DATA, "Expect last error %#x, got %#lx\n", ERROR_INVALID_DATA, err);

    /* #5 Null buffer pointer */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, NULL, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_USER_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_USER_BUFFER, err);

    /* #6 Zero buffer size */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, 0, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_DATA, "Expect last error %#x, got %#lx\n", ERROR_INVALID_DATA, err);

    /* #7 Flags not zero */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 1);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_FLAGS, "Expect last error %#x, got %#lx\n", ERROR_INVALID_FLAGS, err);

    /* #8 Normal */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);

    /* #9 Delete property with buffer not null */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_EMPTY, (const BYTE *)valueW, 0, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);

    /* #10 Delete property with size not zero */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_EMPTY, NULL, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_USER_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_USER_BUFFER, err);

    /* #11 Delete property */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_EMPTY, NULL, 0, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);

    /* #12 Delete non-existent property */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_EMPTY, NULL, 0, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_NOT_FOUND, "Expect last error %#x, got %#lx\n", ERROR_NOT_FOUND, err);

    /* #13 Delete property value with buffer not null */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_NULL, (const BYTE *)valueW, 0, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);

    /* #14 Delete property value with size not zero */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_NULL, NULL, sizeof(valueW), 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_USER_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_USER_BUFFER, err);

    /* #15 Delete property value */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_NULL, NULL, 0, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);

    /* #16 Delete non-existent property value */
    SetLastError(0xdeadbeef);
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_NULL, NULL, 0, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_NOT_FOUND, "Expect last error %#x, got %#lx\n", ERROR_NOT_FOUND, err);


    /* SetupDiGetDevicePropertyW */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");

    /* #1 Null device info list */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(NULL, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_HANDLE, "Expect last error %#x, got %#lx\n", ERROR_INVALID_HANDLE, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #2 Null device */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, NULL, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_PARAMETER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_PARAMETER, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #3 Null property key */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, NULL, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_DATA, "Expect last error %#x, got %#lx\n", ERROR_INVALID_DATA, err);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #4 Null property type */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, NULL, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_USER_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_USER_BUFFER, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #5 Null buffer */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, NULL, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_USER_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INVALID_USER_BUFFER, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #6 Null buffer with zero size and wrong type */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_UINT64;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, NULL, 0, &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INSUFFICIENT_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INSUFFICIENT_BUFFER, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == sizeof(valueW), "Got size %ld\n", size);

    /* #7 Zero buffer size */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, 0, &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INSUFFICIENT_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INSUFFICIENT_BUFFER, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == sizeof(valueW), "Got size %ld\n", size);

    /* #8 Null required size */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), NULL, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);

    /* #9 Flags not zero */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), &size, 1);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INVALID_FLAGS, "Expect last error %#x, got %#lx\n", ERROR_INVALID_FLAGS, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #10 Non-existent property key */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_HardwareIds, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_NOT_FOUND, "Expect last error %#x, got %#lx\n", ERROR_NOT_FOUND, err);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #11 Wrong property key type */
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_UINT64;
    size = 0;
    memset(buffer, 0, sizeof(buffer));
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == sizeof(valueW), "Got size %ld\n", size);
    ok(!lstrcmpW((WCHAR *)buffer, valueW), "Expect buffer %s, got %s\n", wine_dbgstr_w(valueW), wine_dbgstr_w((WCHAR *)buffer));

    /* #12 Get null property value */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_NULL, NULL, 0, 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_NOT_FOUND, "Expect last error %#x, got %#lx\n", ERROR_NOT_FOUND, err);
    ok(size == 0, "Expect size %d, got %ld\n", 0, size);

    /* #13 Insufficient buffer size */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(valueW) - 1, &size, 0);
    err = GetLastError();
    ok(!ret, "Expect failure\n");
    ok(err == ERROR_INSUFFICIENT_BUFFER, "Expect last error %#x, got %#lx\n", ERROR_INSUFFICIENT_BUFFER, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == sizeof(valueW), "Got size %ld\n", size);

    /* #14 Normal */
    ret = pSetupDiSetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, DEVPROP_TYPE_STRING, (const BYTE *)valueW, sizeof(valueW), 0);
    ok(ret, "Expect success\n");
    SetLastError(0xdeadbeef);
    type = DEVPROP_TYPE_STRING;
    size = 0;
    memset(buffer, 0, sizeof(buffer));
    ret = pSetupDiGetDevicePropertyW(set, &device_data, &DEVPKEY_Device_FriendlyName, &type, buffer, sizeof(buffer), &size, 0);
    err = GetLastError();
    ok(ret, "Expect success\n");
    ok(err == NO_ERROR, "Expect last error %#x, got %#lx\n", NO_ERROR, err);
    ok(type == DEVPROP_TYPE_STRING, "Expect type %#x, got %#lx\n", DEVPROP_TYPE_STRING, type);
    ok(size == sizeof(valueW), "Got size %ld\n", size);
    ok(!lstrcmpW((WCHAR *)buffer, valueW), "Expect buffer %s, got %s\n", wine_dbgstr_w(valueW), wine_dbgstr_w((WCHAR *)buffer));

    ret = SetupDiRemoveDevice(set, &device_data);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);
    FreeLibrary(hmod);
}

static void test_get_device_instance_id(void)
{
    BOOL ret;
    HDEVINFO set;
    SP_DEVINFO_DATA device = {0};
    char id[200];
    DWORD size;

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(NULL, &device, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, &device, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, &device, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    device.cbSize = sizeof(device);
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, &device, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, &device, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Failed to get device id, error %#lx.\n", GetLastError());
    ok(!strcmp(id, "ROOT\\LEGACY_BOGUS\\0000"), "Got unexpected id %s.\n", id);

    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "SetupDiCreateDeviceInfoA failed: %08lx\n", GetLastError());

    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Failed to get device id, error %#lx.\n", GetLastError());
    ok(!strcmp(id, "ROOT\\LEGACY_BOGUS\\0001"), "Got unexpected id %s.\n", id);

    SetupDiDestroyDeviceInfoList(set);
}

static void test_open_device_info(void)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    CHAR id[MAX_DEVICE_ID_LEN + 2];
    HDEVINFO set;
    DWORD i = 0;
    BOOL ret;

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info, error %#lx.\n", GetLastError());

    /* Open non-existent device */
    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\FFFF", NULL, 0, &device);
    ok(GetLastError() == ERROR_NO_SUCH_DEVINST, "Got unexpected error %#lx.\n", GetLastError());
    ok(!ret, "Expected failure.\n");
    check_device_info(set, 0, NULL, NULL);

    /* Open unregistered device */
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1000", NULL, 0, &device);
    ok(GetLastError() == ERROR_NO_SUCH_DEVINST, "Got unexpected error %#lx.\n", GetLastError());
    ok(!ret, "Expected failure.\n");
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1000");
    check_device_info(set, 1, NULL, NULL);

    /* Open registered device */
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1000");
    check_device_info(set, 1, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 2, NULL, NULL);

    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    ok(GetLastError() == NO_ERROR, "Got unexpected error %#lx.\n", GetLastError());
    ok(ret, "Failed to open device info\n");
    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());
    ok(!strcasecmp(id, "Root\\LEGACY_BOGUS\\1001"), "Got unexpected id %s.\n", id);
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1000");
    check_device_info(set, 1, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 2, NULL, NULL);

    /* Open registered device in an empty device info set */
    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    ok(GetLastError() == NO_ERROR, "Got unexpected error %#lx.\n", GetLastError());
    ok(ret, "Failed to open device info\n");
    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());
    ok(!strcasecmp(id, "Root\\LEGACY_BOGUS\\1001"), "Got unexpected id %s.\n", id);
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 1, NULL, NULL);

    /* Open registered device again */
    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    ok(GetLastError() == NO_ERROR, "Got unexpected error %#lx.\n", GetLastError());
    ok(ret, "Failed to open device info\n");
    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());
    ok(!strcasecmp(id, "Root\\LEGACY_BOGUS\\1001"), "Got unexpected id %s.\n", id);
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 1, NULL, NULL);

    /* Open registered device in a new device info set with wrong GUID */
    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiCreateDeviceInfoList(&guid2, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    ok(GetLastError() == ERROR_CLASS_MISMATCH, "Got unexpected error %#lx.\n", GetLastError());
    ok(!ret, "Expected failure.\n");
    check_device_info(set, 0, NULL, NULL);

    /* Open registered device in a new device info set with NULL GUID */
    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    ok(GetLastError() == NO_ERROR, "Got unexpected error %#lx.\n", GetLastError());
    ok(ret, "Failed to open device info\n");
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 1, NULL, NULL);

    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info list, error %#lx.\n", GetLastError());

    /* NULL set */
    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(NULL, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());
    ok(!ret, "Expected failure.\n");

    /* NULL instance id */
    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, NULL, NULL, 0, &device);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());
    ok(!ret, "Expected failure.\n");
    check_device_info(set, 0, NULL, NULL);

    /* Invalid SP_DEVINFO_DATA cbSize, device will be added despite failure */
    SetLastError(0xdeadbeef);
    device.cbSize = 0;
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, &device);
    device.cbSize = sizeof(device);
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(!ret, "Expected failure.\n");
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 1, NULL, NULL);

    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info list, error %#lx.\n", GetLastError());

    /* NULL device */
    SetLastError(0xdeadbeef);
    ret = SetupDiOpenDeviceInfoA(set, "Root\\LEGACY_BOGUS\\1001", NULL, 0, NULL);
    ok(GetLastError() == NO_ERROR, "Got unexpected error %#lx.\n", GetLastError());
    ok(ret, "Failed to open device info\n");
    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\1001");
    check_device_info(set, 1, NULL, NULL);

    /* Clean up */
    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiGetClassDevsA(&guid, NULL, NULL, 0);
    while (SetupDiEnumDeviceInfo(set, i++, &device))
    {
        ret = SetupDiRemoveDevice(set, &device);
        ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());
    }
    SetupDiDestroyDeviceInfoList(set);
}

static void test_register_device_info(void)
{
    SP_DEVINFO_DATA device = {0};
    BOOL ret;
    HDEVINFO set;
    HKEY hkey;
    LSTATUS ls;
    DWORD type = 0;
    DWORD phantom = 0;
    DWORD size;
    int i = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiRegisterDeviceInfo(NULL, NULL, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiRegisterDeviceInfo(set, NULL, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    device.cbSize = sizeof(device);
    SetLastError(0xdeadbeef);
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    RegOpenKeyA(HKEY_LOCAL_MACHINE, "SYSTEM\\CurrentControlSet\\Enum\\ROOT\\LEGACY_BOGUS\\0000", &hkey);
    size = sizeof(phantom);
    ls = RegQueryValueExA(hkey, "Phantom", NULL, &type, (BYTE *)&phantom, &size);
    ok(ls == ERROR_SUCCESS, "Got wrong error code %#lx\n", ls);
    ok(phantom == 1, "Got wrong phantom value %ld\n", phantom);
    ok(type == REG_DWORD, "Got wrong phantom type %#lx\n", type);
    ok(size == sizeof(phantom), "Got wrong phantom size %ld\n", size);
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());
    size = sizeof(phantom);
    ls = RegQueryValueExA(hkey, "Phantom", NULL, NULL, (BYTE *)&phantom, &size);
    ok(ls == ERROR_FILE_NOT_FOUND, "Got wrong error code %#lx\n", ls);
    RegCloseKey(hkey);

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0001", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());
    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0002", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());
    ret = SetupDiDeleteDeviceInfo(set, &device);
    ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0003", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, 0);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, &guid, "Root\\LEGACY_BOGUS\\0002");
    check_device_info(set, 2, &guid, NULL);

    while (SetupDiEnumDeviceInfo(set, i++, &device))
    {
        ret = SetupDiRemoveDevice(set, &device);
        ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());
    }

    SetupDiDestroyDeviceInfoList(set);
}

static void check_all_lower_case(int line, const char* str)
{
    const char *cur;

    for (cur = str; *cur; cur++)
    {
        BOOL is_lower = (tolower(*cur) == *cur);
        ok_(__FILE__, line)(is_lower, "Expected device path to be all lowercase but got %s.\n", str);
        if (!is_lower) break;
    }
}

static void check_device_iface_(int line, HDEVINFO set, SP_DEVINFO_DATA *device,
        const GUID *class, int index, DWORD flags, const char *path)
{
    char buffer[200];
    SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    BOOL ret;

    detail->cbSize = sizeof(*detail);

    SetLastError(0xdeadbeef);
    ret = SetupDiEnumDeviceInterfaces(set, device, class, index, &iface);
    if (path)
    {
        ok_(__FILE__, line)(ret, "Failed to enumerate interfaces, error %#lx.\n", GetLastError());
        ok_(__FILE__, line)(IsEqualGUID(&iface.InterfaceClassGuid, class),
                "Got unexpected class %s.\n", wine_dbgstr_guid(&iface.InterfaceClassGuid));
        ok_(__FILE__, line)(iface.Flags == flags, "Got unexpected flags %#lx.\n", iface.Flags);
        ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, sizeof(buffer), NULL, NULL);
        ok_(__FILE__, line)(ret, "Failed to get interface detail, error %#lx.\n", GetLastError());
        ok_(__FILE__, line)(!strcasecmp(detail->DevicePath, path), "Got unexpected path %s.\n", detail->DevicePath);
        check_all_lower_case(line, detail->DevicePath);
    }
    else
    {
        ok_(__FILE__, line)(!ret, "Expected failure.\n");
        ok_(__FILE__, line)(GetLastError() == ERROR_NO_MORE_ITEMS,
                "Got unexpected error %#lx.\n", GetLastError());
    }
}
#define check_device_iface(a,b,c,d,e,f) check_device_iface_(__LINE__,a,b,c,d,e,f)

static void test_device_iface(void)
{
    char buffer[200];
    SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)buffer;
    SP_DEVINFO_DATA device = {0}, device2 = {sizeof(device2)};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    BOOL ret;
    HDEVINFO set;

    detail->cbSize = sizeof(*detail);

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(NULL, NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(NULL, NULL, &guid, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(set, NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(set, &device, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    device.cbSize = sizeof(device);
    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, NULL);

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(set, &device, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, NULL);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid, 1, 0, NULL);

    /* Creating the same interface a second time succeeds */
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, NULL);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid, 1, 0, NULL);

    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, "Oogah", 0, NULL);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid, 1, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\Oogah");
    check_device_iface(set, &device, &guid, 2, 0, NULL);

    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, "test", 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ok(IsEqualGUID(&iface.InterfaceClassGuid, &guid), "Got unexpected class %s.\n",
            wine_dbgstr_guid(&iface.InterfaceClassGuid));
    ok(iface.Flags == 0, "Got unexpected flags %#lx.\n", iface.Flags);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, sizeof(buffer), NULL, NULL);
    ok(ret, "Failed to get interface detail, error %#lx.\n", GetLastError());
    ok(!strcasecmp(detail->DevicePath, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\test"),
            "Got unexpected path %s.\n", detail->DevicePath);

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid, 1, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\Oogah");
    check_device_iface(set, &device, &guid, 2, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\test");
    check_device_iface(set, &device, &guid, 3, 0, NULL);

    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid2, NULL, 0, NULL);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid2, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A5-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid2, 1, 0, NULL);

    ret = SetupDiEnumDeviceInterfaces(set, &device, &guid2, 0, &iface);
    ok(ret, "Failed to enumerate interfaces, error %#lx.\n", GetLastError());
    ret = SetupDiRemoveDeviceInterface(set, &iface);
    ok(ret, "Failed to remove interface, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid2, 0, SPINT_REMOVED, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A5-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid2, 1, 0, NULL);

    ret = SetupDiEnumDeviceInterfaces(set, &device, &guid, 0, &iface);
    ok(ret, "Failed to enumerate interfaces, error %#lx.\n", GetLastError());
    ret = SetupDiDeleteDeviceInterfaceData(set, &iface);
    ok(ret, "Failed to delete interface, error %#lx.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\Oogah");
    check_device_iface(set, &device, &guid, 1, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\test");
    check_device_iface(set, &device, &guid, 2, 0, NULL);

    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0001", &guid, NULL, NULL, 0, &device2);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device2, &guid, NULL, 0, NULL);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    check_device_iface(set, NULL, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\Oogah");
    check_device_iface(set, NULL, &guid, 1, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\test");
    check_device_iface(set, NULL, &guid, 2, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0001#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, NULL, &guid, 3, 0, NULL);

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());
}

static void test_device_iface_detail(void)
{
    static const char path[] = "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
    SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail;
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    DWORD size = 0, expectedsize;
    HDEVINFO set;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(NULL, NULL, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, NULL, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, NULL, 100, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, NULL, 0, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    detail = heap_alloc(size);
    expectedsize = FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath[strlen(path) + 1]);

    detail->cbSize = 0;
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, size, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    detail->cbSize = size;
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, size, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, size, &size, NULL);
    ok(ret, "Failed to get interface detail, error %#lx.\n", GetLastError());
    ok(!strcasecmp(path, detail->DevicePath), "Got unexpected path %s.\n", detail->DevicePath);

    ret = SetupDiGetDeviceInterfaceDetailW(set, &iface, NULL, 0, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == expectedsize, "Got unexpected size %ld.\n", size);

    memset(&device, 0, sizeof(device));
    device.cbSize = sizeof(device);
    ret = SetupDiGetDeviceInterfaceDetailW(set, &iface, NULL, 0, &size, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(IsEqualGUID(&device.ClassGuid, &guid), "Got unexpected class %s.\n", wine_dbgstr_guid(&device.ClassGuid));

    heap_free(detail);
    SetupDiDestroyDeviceInfoList(set);
}

static void test_device_key(void)
{
    static const char params_key_path[] = "System\\CurrentControlSet\\Enum\\Root"
            "\\LEGACY_BOGUS\\0000\\Device Parameters";
    static const char class_key_path[] = "System\\CurrentControlSet\\Control\\Class"
            "\\{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
    static const WCHAR bogus[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'E','n','u','m','\\','R','o','o','t','\\',
     'L','E','G','A','C','Y','_','B','O','G','U','S',0};
    SP_DEVINFO_DATA device = {sizeof(device)};
    char driver_path[50], data[4];
    HKEY class_key, key;
    DWORD size;
    BOOL ret;
    HDEVINFO set;
    LONG res;

    SetLastError(0xdeadbeef);
    key = SetupDiCreateDevRegKeyW(NULL, NULL, 0, 0, 0, NULL, NULL);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res != ERROR_SUCCESS, "Key should not exist.\n");
    RegCloseKey(key);

    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ok(!RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key), "Key should exist.\n");
    RegCloseKey(key);

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(NULL, NULL, 0, 0, 0, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, NULL, 0, 0, 0, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, 0, 0, 0, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, 0, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_BOTH, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_DEVINFO_NOT_REGISTERED, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_KEY_DOES_NOT_EXIST, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DRIVER, NULL,
            (BYTE *)driver_path, sizeof(driver_path), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, class_key_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");
    RegCloseKey(key);

    /* Vista+ will fail the following call to SetupDiCreateDevKeyW() if the
     * class key doesn't exist. */
    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, class_key_path, &key);
    ok(!res, "Failed to create class key, error %lu.\n", res);
    RegCloseKey(key);

    key = SetupDiCreateDevRegKeyW(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
    ok(key != INVALID_HANDLE_VALUE, "Failed to create device key, error %#lx.\n", GetLastError());
    RegCloseKey(key);

    ok(!RegOpenKeyA(HKEY_LOCAL_MACHINE, class_key_path, &key), "Key should exist.\n");
    RegCloseKey(key);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\Class", &class_key);
    ok(!res, "Failed to open class key, error %lu.\n", res);

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DRIVER, NULL,
            (BYTE *)driver_path, sizeof(driver_path), NULL);
    ok(ret, "Failed to get driver property, error %#lx.\n", GetLastError());
    res = RegOpenKeyA(class_key, driver_path, &key);
    ok(!res, "Failed to open driver key, error %lu.\n", res);
    RegSetValueExA(key, "foo", 0, REG_SZ, (BYTE *)"bar", sizeof("bar"));
    RegCloseKey(key);

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
todo_wine {
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_ACCESS_DENIED, /* win2k3 */
            "Got unexpected error %#lx.\n", GetLastError());
}

    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
    ok(key != INVALID_HANDLE_VALUE, "Failed to open device key, error %#lx.\n", GetLastError());
    size = sizeof(data);
    res = RegQueryValueExA(key, "foo", NULL, NULL, (BYTE *)data, &size);
    ok(!res, "Failed to get value, error %lu.\n", res);
    ok(!strcmp(data, "bar"), "Got wrong data %s.\n", data);
    RegCloseKey(key);

    ret = SetupDiDeleteDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV);
    ok(ret, "Failed to delete device key, error %#lx.\n", GetLastError());

    res = RegOpenKeyA(class_key, driver_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_KEY_DOES_NOT_EXIST, "Got unexpected error %#lx.\n", GetLastError());

    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_KEY_DOES_NOT_EXIST, "Got unexpected error %#lx.\n", GetLastError());

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, params_key_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    key = SetupDiCreateDevRegKeyA(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV, NULL, NULL);
    ok(key != INVALID_HANDLE_VALUE, "Got unexpected error %#lx.\n", GetLastError());
    RegCloseKey(key);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, params_key_path, &key);
    ok(!res, "Failed to open device key, error %lu.\n", res);
    res = RegSetValueExA(key, "foo", 0, REG_SZ, (BYTE *)"bar", sizeof("bar"));
    ok(!res, "Failed to set value, error %lu.\n", res);
    RegCloseKey(key);

    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    ok(key != INVALID_HANDLE_VALUE, "Got unexpected error %#lx.\n", GetLastError());
    size = sizeof(data);
    res = RegQueryValueExA(key, "foo", NULL, NULL, (BYTE *)data, &size);
    ok(!res, "Failed to get value, error %lu.\n", res);
    ok(!strcmp(data, "bar"), "Got wrong data %s.\n", data);
    RegCloseKey(key);

    ret = SetupDiDeleteDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV);
    ok(ret, "Got unexpected error %#lx.\n", GetLastError());

    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DEV, KEY_READ);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_KEY_DOES_NOT_EXIST, "Got unexpected error %#lx.\n", GetLastError());

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, params_key_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    key = SetupDiCreateDevRegKeyW(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
    ok(key != INVALID_HANDLE_VALUE, "Failed to create device key, error %#lx.\n", GetLastError());
    RegCloseKey(key);

    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());
    SetupDiDestroyDeviceInfoList(set);

    res = RegOpenKeyA(class_key, driver_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    /* Vista+ deletes the key automatically. */
    res = RegDeleteKeyA(HKEY_LOCAL_MACHINE, class_key_path);
    ok(!res || res == ERROR_FILE_NOT_FOUND, "Failed to delete class key, error %lu.\n", res);

    RegCloseKey(class_key);
}

static void test_register_device_iface(void)
{
    static const WCHAR bogus[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'E','n','u','m','\\','R','o','o','t','\\',
     'L','E','G','A','C','Y','_','B','O','G','U','S',0};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVINFO_DATA device2 = {sizeof(device2)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    HDEVINFO set, set2;
    BOOL ret;
    HKEY key;
    LONG res;

    set = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, "removed", 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, "deleted", 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());

    ret = SetupDiEnumDeviceInterfaces(set, &device, &guid, 1, &iface);
    ok(ret, "Failed to enumerate interfaces, error %#lx.\n", GetLastError());
    ret = SetupDiRemoveDeviceInterface(set, &iface);
    ok(ret, "Failed to delete interface, error %#lx.\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, &device, &guid, 2, &iface);
    ok(ret, "Failed to enumerate interfaces, error %#lx.\n", GetLastError());
    ret = SetupDiDeleteDeviceInterfaceData(set, &iface);
    ok(ret, "Failed to delete interface, error %#lx.\n", GetLastError());

    set2 = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set2 != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    check_device_iface(set2, NULL, &guid, 0, 0, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set2, NULL, &guid, 1, 0, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\deleted");
    check_device_iface(set2, NULL, &guid, 2, 0, NULL);

    ret = SetupDiEnumDeviceInfo(set2, 0, &device2);
    ok(ret, "Failed to enumerate devices, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set2, &device2, &guid, "second", 0, NULL);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());

    check_device_iface(set, NULL, &guid, 0, SPINT_REMOVED, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &guid, 1, SPINT_REMOVED, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\removed");
    check_device_iface(set, NULL, &guid, 2, 0, NULL);

    check_device_iface(set2, NULL, &guid, 0, 0, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set2, NULL, &guid, 1, 0, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\deleted");
    check_device_iface(set2, NULL, &guid, 2, 0, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\second");
    check_device_iface(set2, NULL, &guid, 3, 0, NULL);

    SetupDiDestroyDeviceInfoList(set);
    SetupDiDestroyDeviceInfoList(set2);

    /* make sure all interface keys are deleted when a device is removed */

    set = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    set2 = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set2 != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_iface(set2, NULL, &guid, 0, 0, NULL);
    SetupDiDestroyDeviceInfoList(set2);

    SetupDiDestroyDeviceInfoList(set);
}

static void test_registry_property_a(void)
{
    static const CHAR bogus[] = "System\\CurrentControlSet\\Enum\\Root\\LEGACY_BOGUS";
    SP_DEVINFO_DATA device = {sizeof(device)};
    CHAR buf[64] = "";
    DWORD size, type;
    HDEVINFO set;
    BOOL ret;
    LONG res;
    HKEY key;

    set = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to get device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(NULL, NULL, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(set, NULL, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_REG_PROPERTY, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, 0);
    todo_wine
    ok(!ret, "Expected failure.\n");
    /* GetLastError() returns nonsense in win2k3 */

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, (BYTE *)"Bogus", sizeof("Bogus"));
    ok(ret, "Failed to set property, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(NULL, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_REG_PROPERTY, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, NULL, sizeof("Bogus"), NULL);
    ok(!ret, "Expected failure, got %d\n", ret);
    /* GetLastError() returns nonsense in win2k3 */

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == sizeof("Bogus"), "Got unexpected size %ld.\n", size);

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!strcmp(buf, "Bogus"), "Got unexpected property %s.\n", buf);

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, &type, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!strcmp(buf, "Bogus"), "Got unexpected property %s.\n", buf);
    ok(type == REG_SZ, "Got unexpected type %ld.\n", type);

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, 0);
    ok(ret, "Failed to set property, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, (BYTE *)buf, sizeof(buf), &size);
todo_wine {
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());
}

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_HARDWAREID, NULL, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    /* Test existing registry properties right after device creation */
    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    /* Create device from a not registered class without device name */
    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* No SPDRP_DEVICEDESC property */
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DEVICEDESC, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    /* No SPDRP_CLASS property */
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CLASS, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    /* Have SPDRP_CLASSGUID property */
    memset(buf, 0, sizeof(buf));
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CLASSGUID, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpiA(buf, "{6a55b5a4-3f65-11db-b704-0011955c2bdb}"), "Got unexpected value %s.\n", buf);

    ret = SetupDiDeleteDeviceInfo(set, &device);
    ok(ret, "Failed to delete device, error %#lx.\n", GetLastError());

    /* Create device from a not registered class with a device name */
    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, "device_name", NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* Have SPDRP_DEVICEDESC property */
    memset(buf, 0, sizeof(buf));
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DEVICEDESC, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpA(buf, "device_name"), "Got unexpected value %s.\n", buf);

    SetupDiDestroyDeviceInfoList(set);

    /* Create device from a registered class */
    set = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_DISPLAY, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "display", &GUID_DEVCLASS_DISPLAY, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* Have SPDRP_CLASS property */
    memset(buf, 0, sizeof(buf));
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_CLASS, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpA(buf, "Display"), "Got unexpected value %s.\n", buf);

    SetupDiDestroyDeviceInfoList(set);
}

static void test_registry_property_w(void)
{
    WCHAR friendly_name[] = {'B','o','g','u','s',0};
    SP_DEVINFO_DATA device = {sizeof(device)};
    WCHAR buf[64] = {0};
    DWORD size, type;
    HDEVINFO set;
    BOOL ret;
    LONG res;
    HKEY key;
    static const WCHAR bogus[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'E','n','u','m','\\','R','o','o','t','\\',
     'L','E','G','A','C','Y','_','B','O','G','U','S',0};

    set = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to get device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(NULL, NULL, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(set, NULL, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(set, &device, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_REG_PROPERTY, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, 0);
    todo_wine
    ok(!ret, "Expected failure.\n");
    /* GetLastError() returns nonsense in win2k3 */

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, (BYTE *)friendly_name, sizeof(friendly_name));
    ok(ret, "Failed to set property, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(NULL, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_REG_PROPERTY, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, NULL, sizeof(buf), NULL);
    ok(!ret, "Expected failure.\n");
    /* GetLastError() returns nonsense in win2k3 */

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == sizeof(friendly_name), "Got unexpected size %ld.\n", size);

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpW(buf, friendly_name), "Got unexpected property %s.\n", wine_dbgstr_w(buf));

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, &type, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpW(buf, friendly_name), "Got unexpected property %s.\n", wine_dbgstr_w(buf));
    ok(type == REG_SZ, "Got unexpected type %ld.\n", type);

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, 0);
    ok(ret, "Failed to set property, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, (BYTE *)buf, sizeof(buf), &size);
todo_wine {
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());
}

    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_HARDWAREID, NULL, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    /* Test existing registry properties right after device creation */
    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    /* Create device from a not registered class without device name */
    ret = SetupDiCreateDeviceInfoW(set, L"LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* No SPDRP_DEVICEDESC property */
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_DEVICEDESC, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    /* No SPDRP_CLASS property */
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_CLASS, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());

    /* Have SPDRP_CLASSGUID property */
    memset(buf, 0, sizeof(buf));
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_CLASSGUID, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpiW(buf, L"{6a55b5a4-3f65-11db-b704-0011955c2bdb}"), "Got unexpected value %s.\n", wine_dbgstr_w(buf));

    ret = SetupDiDeleteDeviceInfo(set, &device);
    ok(ret, "Failed to delete device, error %#lx.\n", GetLastError());

    /* Create device from a not registered class with a device name */
    ret = SetupDiCreateDeviceInfoW(set, L"LEGACY_BOGUS", &guid, L"device_name", NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* Have SPDRP_DEVICEDESC property */
    memset(buf, 0, sizeof(buf));
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_DEVICEDESC, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpW(buf, L"device_name"), "Got unexpected value %s.\n", wine_dbgstr_w(buf));

    SetupDiDestroyDeviceInfoList(set);

    /* Create device from a registered class */
    set = SetupDiCreateDeviceInfoList(&GUID_DEVCLASS_DISPLAY, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoW(set, L"display", &GUID_DEVCLASS_DISPLAY, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    /* Have SPDRP_CLASS property */
    memset(buf, 0, sizeof(buf));
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_CLASS, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#lx.\n", GetLastError());
    ok(!lstrcmpW(buf, L"Display"), "Got unexpected value %s.\n", wine_dbgstr_w(buf));

    SetupDiDestroyDeviceInfoList(set);
}

static void test_get_inf_class(void)
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

    GetTempPathA(MAX_PATH, filename);
    strcat(filename, inffile);
    DeleteFileA(filename);

    /* not existing file */
    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %lu\n", GetLastError());

    /* missing file wins against other invalid parameter */
    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, NULL, cn, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, &guid, NULL, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, &guid, cn, 0, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, NULL);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
    ok(ERROR_FILE_NOT_FOUND == GetLastError(),
        "expected error ERROR_FILE_NOT_FOUND, got %lu\n", GetLastError());

    /* test file content */
    h = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                    FILE_ATTRIBUTE_NORMAL, NULL);
    ok(h != INVALID_HANDLE_VALUE, "Failed to create file, error %#lx.\n", GetLastError());
    CloseHandle(h);

    retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");

    for(i=0; i < ARRAY_SIZE(signatures); i++)
    {
        trace("testing signature %s\n", signatures[i]);
        h = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);
        ok(h != INVALID_HANDLE_VALUE, "Failed to create file, error %#lx.\n", GetLastError());
        WriteFile(h, content, sizeof(content), &count, NULL);
        CloseHandle(h);

        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");

        WritePrivateProfileStringA("Version", "Signature", signatures[i], filename);

        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");

        WritePrivateProfileStringA("Version", "Class", "WINE", filename);

        count = 0xdeadbeef;
        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(retval, "expected SetupDiGetINFClassA to succeed! error %lu\n", GetLastError());
        ok(count == 5, "expected count==5, got %lu\n", count);

        count = 0xdeadbeef;
        retval = SetupDiGetINFClassA(filename, &guid, cn, 5, &count);
        ok(retval, "expected SetupDiGetINFClassA to succeed! error %lu\n", GetLastError());
        ok(count == 5, "expected count==5, got %lu\n", count);

        count = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, cn, 4, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INSUFFICIENT_BUFFER == GetLastError(),
            "expected error ERROR_INSUFFICIENT_BUFFER, got %lu\n", GetLastError());
        ok(count == 5, "expected count==5, got %lu\n", count);

        /* invalid parameter */
        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(NULL, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INVALID_PARAMETER == GetLastError(),
            "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, NULL, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INVALID_PARAMETER == GetLastError(),
            "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, NULL, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(ERROR_INVALID_PARAMETER == GetLastError(),
            "expected error ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, cn, 0, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER || GetLastError() == ERROR_INVALID_PARAMETER /* 2k3+ */,
                "Got unexpected error %#lx.\n", GetLastError());

        DeleteFileA(filename);

        WritePrivateProfileStringA("Version", "Signature", signatures[i], filename);
        WritePrivateProfileStringA("Version", "ClassGUID", "WINE", filename);

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(GetLastError() == RPC_S_INVALID_STRING_UUID || GetLastError() == ERROR_INVALID_PARAMETER /* 7+ */,
                "Got unexpected error %#lx.\n", GetLastError());

        /* network adapter guid */
        WritePrivateProfileStringA("Version", "ClassGUID",
                                   "{4d36e972-e325-11ce-bfc1-08002be10318}", filename);

        /* this test succeeds only if the guid is known to the system */
        count = 0xdeadbeef;
        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(retval, "expected SetupDiGetINFClassA to succeed! error %lu\n", GetLastError());
        todo_wine
        ok(count == 4, "expected count==4, got %lu(%s)\n", count, cn);

        DeleteFileA(filename);
    }
}

static void test_devnode(void)
{
    HDEVINFO set;
    SP_DEVINFO_DATA device = { sizeof(SP_DEVINFO_DATA) };
    char buffer[50];
    DWORD ret;

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevs failed: %#lx\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL,
        NULL, 0, &device);
    ok(ret, "SetupDiCreateDeviceInfo failed: %#lx\n", GetLastError());

    ret = CM_Get_Device_IDA(device.DevInst, buffer, sizeof(buffer), 0);
    ok(!ret, "got %#lx\n", ret);
    ok(!strcmp(buffer, "ROOT\\LEGACY_BOGUS\\0000"), "got %s\n", buffer);

    SetupDiDestroyDeviceInfoList(set);
}

static void test_device_interface_key(void)
{
    const char keypath[] = "System\\CurrentControlSet\\Control\\DeviceClasses\\"
        "{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\"
        "##?#ROOT#LEGACY_BOGUS#0001#{6a55b5a4-3f65-11db-b704-0011955c2bdb}";
    SP_DEVICE_INTERFACE_DATA iface = { sizeof(iface) };
    SP_DEVINFO_DATA devinfo = { sizeof(devinfo) };
    HKEY parent, key, dikey;
    char buffer[5];
    HDEVINFO set;
    LONG sz, ret;

    set = SetupDiGetClassDevsA(NULL, NULL, 0, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevs failed: %#lx\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0001", &guid, NULL, NULL, 0, &devinfo);
    ok(ret, "SetupDiCreateDeviceInfo failed: %#lx\n", GetLastError());

    ret = SetupDiCreateDeviceInterfaceA(set, &devinfo, &guid, NULL, 0, &iface);
    ok(ret, "SetupDiCreateDeviceInterface failed: %#lx\n", GetLastError());

    ret = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &parent);
    ok(!ret, "failed to open device parent key: %lu\n", ret);

    ret = RegOpenKeyA(parent, "#\\Device Parameters", &key);
    ok(ret == ERROR_FILE_NOT_FOUND, "key shouldn't exist\n");

    dikey = SetupDiCreateDeviceInterfaceRegKeyA(set, &iface, 0, KEY_ALL_ACCESS, NULL, NULL);
    ok(dikey != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    ret = RegOpenKeyA(parent, "#\\Device Parameters", &key);
    ok(!ret, "key should exist: %lu\n", ret);

    ret = RegSetValueA(key, NULL, REG_SZ, "test", 5);
    ok(!ret, "RegSetValue failed: %lu\n", ret);
    sz = sizeof(buffer);
    ret = RegQueryValueA(dikey, NULL, buffer, &sz);
    ok(!ret, "RegQueryValue failed: %lu\n", ret);
    ok(!strcmp(buffer, "test"), "got wrong data %s\n", buffer);

    RegCloseKey(dikey);
    RegCloseKey(key);

    ret = SetupDiDeleteDeviceInterfaceRegKey(set, &iface, 0);
    ok(ret, "got error %lu\n", GetLastError());

    ret = RegOpenKeyA(parent, "#\\Device Parameters", &key);
    ok(ret == ERROR_FILE_NOT_FOUND, "key shouldn't exist\n");

    RegCloseKey(parent);
    SetupDiRemoveDeviceInterface(set, &iface);
    SetupDiRemoveDevice(set, &devinfo);
    SetupDiDestroyDeviceInfoList(set);
}

static void test_open_device_interface_key(void)
{
    SP_DEVICE_INTERFACE_DATA iface;
    SP_DEVINFO_DATA device;
    CHAR buffer[5];
    HDEVINFO set;
    LSTATUS lr;
    LONG size;
    HKEY key;
    BOOL ret;

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx\n", GetLastError());

    device.cbSize = sizeof(device);
    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    iface.cbSize = sizeof(iface);
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());

    /* Test open before creation */
    key = SetupDiOpenDeviceInterfaceRegKey(set, &iface, 0, KEY_ALL_ACCESS);
    ok(key == INVALID_HANDLE_VALUE, "Expect open interface registry key failure\n");

    /* Test opened key is from SetupDiCreateDeviceInterfaceRegKey */
    key = SetupDiCreateDeviceInterfaceRegKeyW(set, &iface, 0, KEY_ALL_ACCESS, NULL, NULL);
    ok(key != INVALID_HANDLE_VALUE, "Failed to create interface registry key, error %#lx\n", GetLastError());

    lr = RegSetValueA(key, NULL, REG_SZ, "test", 5);
    ok(!lr, "RegSetValue failed, error %#lx\n", lr);

    RegCloseKey(key);

    key = SetupDiOpenDeviceInterfaceRegKey(set, &iface, 0, KEY_ALL_ACCESS);
    ok(key != INVALID_HANDLE_VALUE, "Failed to open interface registry key, error %#lx\n", GetLastError());

    size = sizeof(buffer);
    lr = RegQueryValueA(key, NULL, buffer, &size);
    ok(!lr, "RegQueryValue failed, error %#lx\n", lr);
    ok(!strcmp(buffer, "test"), "got wrong data %s\n", buffer);

    RegCloseKey(key);

    /* Test open after removal */
    ret = SetupDiRemoveDeviceInterface(set, &iface);
    ok(ret, "Failed to remove device interface, error %#lx.\n", GetLastError());

    key = SetupDiOpenDeviceInterfaceRegKey(set, &iface, 0, KEY_ALL_ACCESS);
    ok(key == INVALID_HANDLE_VALUE, "Expect open interface registry key failure\n");

    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Failed to remove device, error %#lx.\n", GetLastError());
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());
}

static void test_device_install_params(void)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    SP_DEVINSTALL_PARAMS_A params;
    HDEVINFO set;
    BOOL ret;

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    params.cbSize = sizeof(params) - 1;
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstallParamsA(set, &device, &params);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    params.cbSize = sizeof(params) + 1;
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstallParamsA(set, &device, &params);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    params.cbSize = sizeof(params) - 1;
    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceInstallParamsA(set, &device, &params);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    params.cbSize = sizeof(params) + 1;
    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceInstallParamsA(set, &device, &params);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());

    memset(&params, 0xcc, sizeof(params));
    params.cbSize = sizeof(params);
    ret = SetupDiGetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to get device install params, error %#lx.\n", GetLastError());
    ok(!params.Flags, "Got flags %#lx.\n", params.Flags);
    ok(!params.FlagsEx, "Got extended flags %#lx.\n", params.FlagsEx);
    ok(!params.hwndParent, "Got parent %p.\n", params.hwndParent);
    ok(!params.InstallMsgHandler, "Got callback %p.\n", params.InstallMsgHandler);
    ok(!params.InstallMsgHandlerContext, "Got callback context %p.\n", params.InstallMsgHandlerContext);
    ok(!params.FileQueue, "Got queue %p.\n", params.FileQueue);
    ok(!params.ClassInstallReserved, "Got class installer data %#Ix.\n", params.ClassInstallReserved);
    ok(!params.DriverPath[0], "Got driver path %s.\n", params.DriverPath);

    params.Flags = DI_INF_IS_SORTED;
    params.FlagsEx = DI_FLAGSEX_ALLOWEXCLUDEDDRVS;
    strcpy(params.DriverPath, "C:\\windows");
    ret = SetupDiSetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to set device install params, error %#lx.\n", GetLastError());

    memset(&params, 0xcc, sizeof(params));
    params.cbSize = sizeof(params);
    ret = SetupDiGetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to get device install params, error %#lx.\n", GetLastError());
    ok(params.Flags == DI_INF_IS_SORTED, "Got flags %#lx.\n", params.Flags);
    ok(params.FlagsEx == DI_FLAGSEX_ALLOWEXCLUDEDDRVS, "Got extended flags %#lx.\n", params.FlagsEx);
    ok(!params.hwndParent, "Got parent %p.\n", params.hwndParent);
    ok(!params.InstallMsgHandler, "Got callback %p.\n", params.InstallMsgHandler);
    ok(!params.InstallMsgHandlerContext, "Got callback context %p.\n", params.InstallMsgHandlerContext);
    ok(!params.FileQueue, "Got queue %p.\n", params.FileQueue);
    ok(!params.ClassInstallReserved, "Got class installer data %#Ix.\n", params.ClassInstallReserved);
    ok(!strcasecmp(params.DriverPath, "C:\\windows"), "Got driver path %s.\n", params.DriverPath);

    SetupDiDestroyDeviceInfoList(set);
}

#ifdef __i386__
#define MYEXT "x86"
#define WOWEXT "AMD64"
#define WRONGEXT "ARM"
#elif defined(__x86_64__)
#define MYEXT "AMD64"
#define WOWEXT "x86"
#define WRONGEXT "ARM64"
#elif defined(__arm__)
#define MYEXT "ARM"
#define WOWEXT "ARM64"
#define WRONGEXT "x86"
#elif defined(__aarch64__)
#define MYEXT "ARM64"
#define WOWEXT "ARM"
#define WRONGEXT "AMD64"
#else
#define MYEXT
#define WOWEXT
#define WRONGEXT
#endif

static void test_get_actual_section(void)
{
    static const char inf_data[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "[section1]\n"
            "[section2]\n"
            "[section2.nt]\n"
            "[section3]\n"
            "[section3.nt" MYEXT "]\n"
            "[section4]\n"
            "[section4.nt]\n"
            "[section4.nt" MYEXT "]\n"
            "[section5.nt]\n"
            "[section6.nt" MYEXT "]\n"
            "[section7]\n"
            "[section7.nt" WRONGEXT "]\n"
            "[section8.nt" WRONGEXT "]\n"
            "[section9.nt" MYEXT "]\n"
            "[section9.nt" WOWEXT "]\n"
            "[section10.nt" WOWEXT "]\n";

    char inf_path[MAX_PATH], section[LINE_LEN], *extptr;
    DWORD size;
    HINF hinf;
    BOOL ret;

    GetTempPathA(sizeof(inf_path), inf_path);
    strcat(inf_path, "setupapi_test.inf");
    create_file(inf_path, inf_data);

    hinf = SetupOpenInfFileA(inf_path, NULL, INF_STYLE_WIN4, NULL);
    ok(hinf != INVALID_HANDLE_VALUE, "Failed to open INF file, error %#lx.\n", GetLastError());

    ret = SetupDiGetActualSectionToInstallA(hinf, "section1", section, ARRAY_SIZE(section), NULL, NULL);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcmp(section, "section1"), "Got unexpected section %s.\n", section);

    size = 0xdeadbeef;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section1", NULL, 5, &size, NULL);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(size == 9, "Got size %lu.\n", size);

    SetLastError(0xdeadbeef);
    size = 0xdeadbeef;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section1", section, 5, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == 9, "Got size %lu.\n", size);

    SetLastError(0xdeadbeef);
    ret = SetupDiGetActualSectionToInstallA(hinf, "section1", section, ARRAY_SIZE(section), &size, NULL);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section1"), "Got unexpected section %s.\n", section);
    ok(size == 9, "Got size %lu.\n", size);

    extptr = section;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section2", section, ARRAY_SIZE(section), NULL, &extptr);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section2.NT"), "Got unexpected section %s.\n", section);
    ok(extptr == section + 8, "Got extension %s.\n", extptr);

    extptr = section;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section3", section, ARRAY_SIZE(section), NULL, &extptr);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section3.NT" MYEXT), "Got unexpected section %s.\n", section);
    ok(extptr == section + 8, "Got extension %s.\n", extptr);

    extptr = section;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section4", section, ARRAY_SIZE(section), NULL, &extptr);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section4.NT" MYEXT), "Got unexpected section %s.\n", section);
    ok(extptr == section + 8, "Got extension %s.\n", extptr);

    extptr = section;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section5", section, ARRAY_SIZE(section), NULL, &extptr);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section5.NT"), "Got unexpected section %s.\n", section);
    ok(extptr == section + 8, "Got extension %s.\n", extptr);

    extptr = section;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section6", section, ARRAY_SIZE(section), NULL, &extptr);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section6.NT" MYEXT), "Got unexpected section %s.\n", section);
    ok(extptr == section + 8, "Got extension %s.\n", extptr);

    extptr = section;
    ret = SetupDiGetActualSectionToInstallA(hinf, "section9", section, ARRAY_SIZE(section), NULL, &extptr);
    ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
    ok(!strcasecmp(section, "section9.NT" MYEXT), "Got unexpected section %s.\n", section);
    ok(extptr == section + 8, "Got extension %s.\n", extptr);

    if (0)
    {
        /* For some reason, these calls hang on Windows 10 1809+. */
        extptr = section;
        ret = SetupDiGetActualSectionToInstallA(hinf, "section1", section, ARRAY_SIZE(section), NULL, &extptr);
        ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
        ok(!strcasecmp(section, "section1"), "Got unexpected section %s.\n", section);
        ok(!extptr || !*extptr /* Windows 10 1809 */, "Got extension %s.\n", extptr);

        extptr = section;
        ret = SetupDiGetActualSectionToInstallA(hinf, "section7", section, ARRAY_SIZE(section), NULL, &extptr);
        ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
        ok(!strcasecmp(section, "section7"), "Got unexpected section %s.\n", section);
        ok(!extptr || !*extptr /* Windows 10 1809 */, "Got extension %s.\n", extptr);

        extptr = section;
        ret = SetupDiGetActualSectionToInstallA(hinf, "section8", section, ARRAY_SIZE(section), NULL, &extptr);
        ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
        ok(!strcasecmp(section, "section8"), "Got unexpected section %s.\n", section);
        ok(!extptr || !*extptr /* Windows 10 1809 */, "Got extension %s.\n", extptr);

        extptr = section;
        ret = SetupDiGetActualSectionToInstallA(hinf, "nonexistent", section, ARRAY_SIZE(section), NULL, &extptr);
        ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
        ok(!strcasecmp(section, "nonexistent"), "Got unexpected section %s.\n", section);
        ok(!extptr || !*extptr /* Windows 10 1809 */, "Got extension %s.\n", extptr);

        extptr = section;
        ret = SetupDiGetActualSectionToInstallA(hinf, "section10", section, ARRAY_SIZE(section), NULL, &extptr);
        ok(ret, "Failed to get section, error %#lx.\n", GetLastError());
        ok(!strcasecmp(section, "section10"), "Got unexpected section %s.\n", section);
        ok(!extptr, "Got extension %s.\n", extptr);
    }

    SetupCloseInfFile(hinf);
    ret = DeleteFileA(inf_path);
    ok(ret, "Failed to delete %s, error %lu.\n", inf_path, GetLastError());
}

static void test_driver_list(void)
{
    char detail_buffer[1000];
    SP_DRVINFO_DETAIL_DATA_A *detail = (SP_DRVINFO_DETAIL_DATA_A *)detail_buffer;
    char short_path[MAX_PATH], inf_dir[MAX_PATH], inf_path[MAX_PATH + 10], inf_path2[MAX_PATH + 10];
    static const char hardware_id[] = "bogus_hardware_id\0other_hardware_id\0";
    static const char compat_id[] = "bogus_compat_id\0";
    SP_DEVINSTALL_PARAMS_A params = {sizeof(params)};
    SP_DRVINFO_DATA_A driver = {sizeof(driver)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    DWORD size, expect_size;
    FILETIME filetime;
    HDEVINFO set;
    HANDLE file;
    DWORD idx;
    BOOL ret;

    static const char inf_data[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "ClassGuid={6a55b5a4-3f65-11db-b704-0011955c2bdb}\n"
            "[Manufacturer]\n"
            "mfg1=mfg1_key,NT" MYEXT "\n"
            "mfg2=mfg2_key,NT" MYEXT "\n"
            "mfg1_wow=mfg1_key,NT" WOWEXT "\n"
            "mfg2_wow=mfg2_key,NT" WOWEXT "\n"
            "mfg3=mfg3_key,NT" WRONGEXT "\n"
            "[mfg1_key.nt" MYEXT "]\n"
            "desc0=,other_hardware_id,bogus_compat_id\n"
            "desc1=install1,bogus_hardware_id\n"
            "desc2=,bogus_hardware_id\n"
            "desc3=,wrong_hardware_id\n"
            "desc4=,wrong_hardware_id,bogus_compat_id\n"
            "[mfg1_key.nt" WOWEXT "]\n"
            "desc0=,other_hardware_id,bogus_compat_id\n"
            "desc1=install1,bogus_hardware_id\n"
            "desc2=,bogus_hardware_id\n"
            "desc3=,wrong_hardware_id\n"
            "desc4=,wrong_hardware_id,bogus_compat_id\n"
            "[mfg2_key.nt" MYEXT "]\n"
            "desc5=,bogus_hardware_id\n"
            "[mfg2_key.nt" WOWEXT "]\n"
            "desc5=,bogus_hardware_id\n"
            "[mfg3_key.nt" WRONGEXT "]\n"
            "desc6=,bogus_hardware_id\n"
            "[install1.nt" MYEXT "]\n"
            "[install1.nt" WRONGEXT "]\n";

    static const char inf_data_file1[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "ClassGuid={6a55b5a4-3f65-11db-b704-0011955c2bdb}\n"
            "[Manufacturer]\n"
            "mfg1=mfg1_key,NT" MYEXT ",NT" WOWEXT "\n"
            "[mfg1_key.nt" MYEXT "]\n"
            "desc1=,bogus_hardware_id\n"
            "[mfg1_key.nt" WOWEXT "]\n"
            "desc1=,bogus_hardware_id\n";

    static const char inf_data_file2[] = "[Version]\n"
            "Signature=\"$Chicago$\"\n"
            "ClassGuid={6a55b5a5-3f65-11db-b704-0011955c2bdb}\n"
            "[Manufacturer]\n"
            "mfg1=mfg1_key,NT" MYEXT ",NT" WOWEXT "\n"
            "[mfg1_key.nt" MYEXT "]\n"
            "desc2=,bogus_hardware_id\n"
            "[mfg1_key.nt" WOWEXT "]\n"
            "desc2=,bogus_hardware_id\n";

    GetTempPathA(sizeof(inf_path), inf_path);
    GetShortPathNameA(inf_path, short_path, sizeof(short_path));
    strcat(inf_path, "setupapi_test.inf");
    strcat(short_path, "setupapi_test.inf");
    create_file(inf_path, inf_data);
    file = CreateFileA(inf_path, 0, 0, NULL, OPEN_EXISTING, 0, 0);
    GetFileTime(file, NULL, NULL, &filetime);
    CloseHandle(file);

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\BOGUS\\0000", &GUID_NULL, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_HARDWAREID,
            (const BYTE *)hardware_id, sizeof(hardware_id));
    ok(ret, "Failed to set hardware ID, error %#lx.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_COMPATIBLEIDS,
            (const BYTE *)compat_id, sizeof(compat_id));
    ok(ret, "Failed to set hardware ID, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 0, &driver);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiGetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to get device install params, error %#lx.\n", GetLastError());
    strcpy(params.DriverPath, inf_path);
    params.Flags = DI_ENUMSINGLEINF;
    ret = SetupDiSetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to set device install params, error %#lx.\n", GetLastError());

    ret = SetupDiBuildDriverInfoList(set, &device, SPDIT_COMPATDRIVER);
    ok(ret, "Failed to build driver list, error %#lx.\n", GetLastError());

    idx = 0;
    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, idx++, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc0"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, wow64 ? "mfg1_wow" : "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, idx++, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc1"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, wow64 ? "mfg1_wow" : "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    expect_size = FIELD_OFFSET(SP_DRVINFO_DETAIL_DATA_A, HardwareID[sizeof("bogus_hardware_id\0")]);

    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, NULL, 0, &size);
    ok(ret || GetLastError() == ERROR_INVALID_USER_BUFFER /* Win10 1809 */,
            "Failed to get driver details, error %#lx.\n", GetLastError());
    ok(size == expect_size, "Got size %lu.\n", size);

    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, NULL, sizeof(SP_DRVINFO_DETAIL_DATA_A) - 1, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == expect_size, "Got size %lu.\n", size);

    size = 0xdeadbeef;
    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, detail, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == 0xdeadbeef, "Got size %lu.\n", size);

    size = 0xdeadbeef;
    detail->CompatIDsLength = 0xdeadbeef;
    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, detail, sizeof(SP_DRVINFO_DETAIL_DATA_A) - 1, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == 0xdeadbeef, "Got size %lu.\n", size);
    ok(detail->CompatIDsLength == 0xdeadbeef, "Got wrong compat IDs length %lu.\n", detail->CompatIDsLength);

    memset(detail_buffer, 0xcc, sizeof(detail_buffer));
    detail->cbSize = sizeof(*detail);
    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, detail, sizeof(SP_DRVINFO_DETAIL_DATA_A), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(detail->InfDate.dwHighDateTime == filetime.dwHighDateTime
            && detail->InfDate.dwLowDateTime == filetime.dwLowDateTime,
            "Expected %#lx%08lx, got %#lx%08lx.\n", filetime.dwHighDateTime, filetime.dwLowDateTime,
            detail->InfDate.dwHighDateTime, detail->InfDate.dwLowDateTime);
    ok(!strcmp(detail->SectionName, "install1"), "Got section name %s.\n", debugstr_a(detail->SectionName));
    ok(!stricmp(detail->InfFileName, short_path), "Got INF file name %s.\n", debugstr_a(detail->InfFileName));
    ok(!strcmp(detail->DrvDescription, "desc1"), "Got description %s.\n", debugstr_a(detail->DrvDescription));
    ok(!detail->CompatIDsOffset || detail->CompatIDsOffset == sizeof("bogus_hardware_id") /* Win10 1809 */,
            "Got wrong compat IDs offset %lu.\n", detail->CompatIDsOffset);
    ok(!detail->CompatIDsLength, "Got wrong compat IDs length %lu.\n", detail->CompatIDsLength);
    ok(!detail->HardwareID[0], "Got wrong ID list.\n");

    size = 0xdeadbeef;
    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, detail, sizeof(SP_DRVINFO_DETAIL_DATA_A), &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#lx.\n", GetLastError());
    ok(size == expect_size, "Got size %lu.\n", size);

    size = 0xdeadbeef;
    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, detail, sizeof(detail_buffer), &size);
    ok(ret, "Failed to get driver details, error %#lx.\n", GetLastError());
    ok(size == expect_size, "Got size %lu.\n", size);
    ok(detail->InfDate.dwHighDateTime == filetime.dwHighDateTime
            && detail->InfDate.dwLowDateTime == filetime.dwLowDateTime,
            "Expected %#lx%08lx, got %#lx%08lx.\n", filetime.dwHighDateTime, filetime.dwLowDateTime,
            detail->InfDate.dwHighDateTime, detail->InfDate.dwLowDateTime);
    ok(!strcmp(detail->SectionName, "install1"), "Got section name %s.\n", debugstr_a(detail->SectionName));
    ok(!stricmp(detail->InfFileName, short_path), "Got INF file name %s.\n", debugstr_a(detail->InfFileName));
    ok(!strcmp(detail->DrvDescription, "desc1"), "Got description %s.\n", debugstr_a(detail->DrvDescription));
    ok(!detail->CompatIDsOffset, "Got wrong compat IDs offset %lu.\n", detail->CompatIDsOffset);
    ok(!detail->CompatIDsLength, "Got wrong compat IDs length %lu.\n", detail->CompatIDsLength);
    ok(!memcmp(detail->HardwareID, "bogus_hardware_id\0", sizeof("bogus_hardware_id\0")), "Got wrong ID list.\n");

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, idx++, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc2"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, wow64 ? "mfg1_wow" : "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, idx++, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc4"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, wow64 ? "mfg1_wow" : "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);
    ret = SetupDiGetDriverInfoDetailA(set, &device, &driver, detail, sizeof(detail_buffer), NULL);
    ok(ret, "Failed to get driver details, error %#lx.\n", GetLastError());
    ok(detail->InfDate.dwHighDateTime == filetime.dwHighDateTime
            && detail->InfDate.dwLowDateTime == filetime.dwLowDateTime,
            "Expected %#lx%08lx, got %#lx%08lx.\n", filetime.dwHighDateTime, filetime.dwLowDateTime,
            detail->InfDate.dwHighDateTime, detail->InfDate.dwLowDateTime);
    ok(!detail->SectionName[0], "Got section name %s.\n", debugstr_a(detail->SectionName));
    ok(!stricmp(detail->InfFileName, short_path), "Got INF file name %s.\n", debugstr_a(detail->InfFileName));
    ok(!strcmp(detail->DrvDescription, "desc4"), "Got description %s.\n", debugstr_a(detail->DrvDescription));
    ok(detail->CompatIDsOffset == sizeof("wrong_hardware_id"), "Got wrong compat IDs offset %lu.\n", detail->CompatIDsOffset);
    ok(detail->CompatIDsLength == sizeof("bogus_compat_id\0"), "Got wrong compat IDs length %lu.\n", detail->CompatIDsLength);
    ok(!memcmp(detail->HardwareID, "wrong_hardware_id\0bogus_compat_id\0",
            sizeof("wrong_hardware_id\0bogus_compat_id\0")), "Got wrong ID list.\n");

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, idx++, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc5"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, wow64 ? "mfg2_wow" : "mfg2"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    SetLastError(0xdeadbeef);
    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, idx++, &driver);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiGetSelectedDriverA(set, &device, &driver);
    ok(ret /* Win10 1809 */ || GetLastError() == ERROR_NO_DRIVER_SELECTED,
            "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiSelectBestCompatDrv(set, &device);
    ok(ret, "Failed to select driver, error %#lx.\n", GetLastError());

    ret = SetupDiGetSelectedDriverA(set, &device, &driver);
    ok(ret, "Failed to get selected driver, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc1"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, wow64 ? "mfg1_wow" : "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    SetupDiDestroyDeviceInfoList(set);
    ret = DeleteFileA(inf_path);
    ok(ret, "Failed to delete %s, error %lu.\n", inf_path, GetLastError());

    /* Test building from a path. */

    GetTempPathA(sizeof(inf_dir), inf_dir);
    strcat(inf_dir, "setupapi_test");
    ret = CreateDirectoryA(inf_dir, NULL);
    ok(ret, "Failed to create directory, error %lu.\n", GetLastError());
    sprintf(inf_path, "%s/test1.inf", inf_dir);
    create_file(inf_path, inf_data_file1);
    sprintf(inf_path2, "%s/test2.inf", inf_dir);
    create_file(inf_path2, inf_data_file2);

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\BOGUS\\0000", &GUID_NULL, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_HARDWAREID,
            (const BYTE *)hardware_id, sizeof(hardware_id));
    ok(ret, "Failed to set hardware ID, error %#lx.\n", GetLastError());

    ret = SetupDiGetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to get device install params, error %#lx.\n", GetLastError());
    strcpy(params.DriverPath, inf_dir);
    ret = SetupDiSetDeviceInstallParamsA(set, &device, &params);
    ok(ret, "Failed to set device install params, error %#lx.\n", GetLastError());

    ret = SetupDiBuildDriverInfoList(set, &device, SPDIT_COMPATDRIVER);
    ok(ret, "Failed to build driver list, error %#lx.\n", GetLastError());

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 0, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc1"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 1, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc2"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    SetLastError(0xdeadbeef);
    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 2, &driver);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "Got unexpected error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);
    ret = DeleteFileA(inf_path);
    ok(ret, "Failed to delete %s, error %lu.\n", inf_path, GetLastError());
    ret = DeleteFileA(inf_path2);
    ok(ret, "Failed to delete %s, error %lu.\n", inf_path2, GetLastError());
    ret = RemoveDirectoryA(inf_dir);
    ok(ret, "Failed to delete %s, error %lu.\n", inf_dir, GetLastError());

    /* Test the default path. */

    create_file("C:/windows/inf/wine_test1.inf", inf_data_file1);
    create_file("C:/windows/inf/wine_test2.inf", inf_data_file2);

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\BOGUS\\0000", &GUID_NULL, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_HARDWAREID,
            (const BYTE *)hardware_id, sizeof(hardware_id));
    ok(ret, "Failed to set hardware ID, error %#lx.\n", GetLastError());

    ret = SetupDiBuildDriverInfoList(set, &device, SPDIT_COMPATDRIVER);
    ok(ret, "Failed to build driver list, error %#lx.\n", GetLastError());

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 0, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc1"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 1, &driver);
    ok(ret, "Failed to enumerate drivers, error %#lx.\n", GetLastError());
    ok(driver.DriverType == SPDIT_COMPATDRIVER, "Got wrong type %#lx.\n", driver.DriverType);
    ok(!strcmp(driver.Description, "desc2"), "Got wrong description '%s'.\n", driver.Description);
    ok(!strcmp(driver.MfgName, "mfg1"), "Got wrong manufacturer '%s'.\n", driver.MfgName);
    ok(!strcmp(driver.ProviderName, ""), "Got wrong provider '%s'.\n", driver.ProviderName);

    SetLastError(0xdeadbeef);
    ret = SetupDiEnumDriverInfoA(set, &device, SPDIT_COMPATDRIVER, 2, &driver);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_NO_MORE_ITEMS, "Got unexpected error %#lx.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);
    ret = DeleteFileA("C:/windows/inf/wine_test1.inf");
    ok(ret, "Failed to delete %s, error %lu.\n", inf_path, GetLastError());
    ret = DeleteFileA("C:/windows/inf/wine_test2.inf");
    ok(ret, "Failed to delete %s, error %lu.\n", inf_path2, GetLastError());
    /* Windows "precompiles" INF files in this dir; try to avoid leaving them behind. */
    DeleteFileA("C:/windows/inf/wine_test1.pnf");
    DeleteFileA("C:/windows/inf/wine_test2.pnf");
}

static BOOL device_is_registered(HDEVINFO set, SP_DEVINFO_DATA *device)
{
    HKEY key = SetupDiOpenDevRegKey(set, device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    RegCloseKey(key);
    return GetLastError() == ERROR_KEY_DOES_NOT_EXIST;
}

static unsigned int *coinst_callback_count;
static DI_FUNCTION *coinst_last_message;

static void test_class_installer(void)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    char regdata[200];
    HKEY class_key;
    HDEVINFO set;
    BOOL ret;
    LONG res;

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\Class"
            "\\{6a55b5a4-3f65-11db-b704-0011955c2bdb}", &class_key);
    ok(!res, "Failed to create class key, error %lu.\n", res);

    strcpy(regdata, "winetest_coinst.dll,class_success");
    res = RegSetValueExA(class_key, "Installer32", 0, REG_SZ, (BYTE *)regdata, strlen(regdata)+1);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_ALLOW_INSTALL, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    ret = SetupDiCallClassInstaller(0xdeadbeef, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == 0xdeadbeef, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_REGISTERDEVICE, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_REMOVE, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_DESTROYPRIVATEDATA, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    /* Test returning an error. */

    strcpy(regdata, "winetest_coinst.dll,class_error");
    res = RegSetValueExA(class_key, "Installer32", 0, REG_SZ, (BYTE *)regdata, strlen(regdata)+1);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == 0xdeadc0de, "Got unexpected error %#lx.\n", GetLastError());

    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == 0xdeadc0de, "Got unexpected error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == 0xdeadc0de, "Got unexpected error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    /* Test returning ERROR_DI_DO_DEFAULT. */

    strcpy(regdata, "winetest_coinst.dll,class_default");
    res = RegSetValueExA(class_key, "Installer32", 0, REG_SZ, (BYTE *)regdata, strlen(regdata)+1);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DI_DO_DEFAULT, "Got unexpected error %#lx.\n", GetLastError());

    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(device_is_registered(set, &device), "Expected device to be registered.\n");

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    /* The default entry point is ClassInstall(). */

    strcpy(regdata, "winetest_coinst.dll");
    res = RegSetValueExA(class_key, "Installer32", 0, REG_SZ, (BYTE *)regdata, strlen(regdata)+1);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_ALLOW_INSTALL, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_DESTROYPRIVATEDATA, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    res = RegDeleteKeyA(class_key, "");
    ok(!res, "Failed to delete class key, error %lu.\n", res);
    RegCloseKey(class_key);
}

static void test_class_coinstaller(void)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    char regdata[200];
    HKEY coinst_key;
    HDEVINFO set;
    BOOL ret;
    LONG res;

    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\CoDeviceInstallers", &coinst_key);
    ok(!res, "Failed to open CoDeviceInstallers key, error %lu.\n", res);
    strcpy(regdata, "winetest_coinst.dll,co_success");
    regdata[strlen(regdata) + 1] = 0;
    res = RegSetValueExA(coinst_key, "{6a55b5a4-3f65-11db-b704-0011955c2bdb}", 0,
            REG_MULTI_SZ, (BYTE *)regdata, strlen(regdata) + 2);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    /* We must recreate the device list, or Windows will not recognize that the
     * class co-installer exists. */
    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DI_DO_DEFAULT, "Got unexpected error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_ALLOW_INSTALL, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    ret = SetupDiCallClassInstaller(0xdeadbeef, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DI_DO_DEFAULT, "Got unexpected error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == 0xdeadbeef, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(device_is_registered(set, &device), "Expected device to be registered.\n");

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_REGISTERDEVICE, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_REMOVE, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    todo_wine ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    todo_wine ok(*coinst_last_message == DIF_DESTROYPRIVATEDATA, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    /* Test returning an error from the co-installer. */

    strcpy(regdata, "winetest_coinst.dll,co_error");
    regdata[strlen(regdata) + 1] = 0;
    res = RegSetValueExA(coinst_key, "{6a55b5a4-3f65-11db-b704-0011955c2bdb}", 0,
            REG_MULTI_SZ, (BYTE *)regdata, strlen(regdata) + 2);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == 0xdeadc0de, "Got unexpected error %#lx.\n", GetLastError());

    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == 0xdeadc0de, "Got unexpected error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    /* The default entry point is CoDeviceInstall(). */

    strcpy(regdata, "winetest_coinst.dll");
    regdata[strlen(regdata) + 1] = 0;
    res = RegSetValueExA(coinst_key, "{6a55b5a4-3f65-11db-b704-0011955c2bdb}", 0,
            REG_MULTI_SZ, (BYTE *)regdata, strlen(regdata) + 2);
    ok(!res, "Failed to set registry value, error %lu.\n", res);

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DI_DO_DEFAULT, "Got unexpected error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_ALLOW_INSTALL, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    ok(*coinst_callback_count == 1, "Got %d callbacks.\n", *coinst_callback_count);
    ok(*coinst_last_message == DIF_DESTROYPRIVATEDATA, "Got unexpected message %#x.\n", *coinst_last_message);
    *coinst_callback_count = 0;

    res = RegDeleteValueA(coinst_key, "{6a55b5a4-3f65-11db-b704-0011955c2bdb}");
    ok(!res, "Failed to delete value, error %lu.\n", res);
    RegCloseKey(coinst_key);
}

static void test_call_class_installer(void)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    HMODULE coinst;
    HDEVINFO set;
    BOOL ret;

    if (wow64)
    {
        skip("SetupDiCallClassInstaller() does not work on WoW64.\n");
        return;
    }

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());

    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");
    ret = SetupDiCallClassInstaller(DIF_REGISTERDEVICE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(device_is_registered(set, &device), "Expected device to be registered.\n");

    /* This is probably not failure per se, but rather an indication that no
     * class installer was called and no default handler exists. */
    ret = SetupDiCallClassInstaller(DIF_ALLOW_INSTALL, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DI_DO_DEFAULT, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(0xdeadbeef, set, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DI_DO_DEFAULT, "Got unexpected error %#lx.\n", GetLastError());

    ret = SetupDiCallClassInstaller(DIF_REMOVE, set, &device);
    ok(ret, "Failed to call class installer, error %#lx.\n", GetLastError());
    ok(!device_is_registered(set, &device), "Expected device not to be registered.\n");

    SetLastError(0xdeadbeef);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list.\n");
    ok(!GetLastError(), "Got unexpected error %#lx.\n", GetLastError());

    load_resource("coinst.dll", "C:\\windows\\system32\\winetest_coinst.dll");

    coinst = LoadLibraryA("winetest_coinst.dll");
    coinst_callback_count = (void *)GetProcAddress(coinst, "callback_count");
    coinst_last_message = (void *)GetProcAddress(coinst, "last_message");

    test_class_installer();
    test_class_coinstaller();

    FreeLibrary(coinst);

    ret = DeleteFileA("C:\\windows\\system32\\winetest_coinst.dll");
    ok(ret, "Failed to delete file, error %lu.\n", GetLastError());
}

static void check_all_devices_enumerated_(int line, HDEVINFO set, BOOL expect_dev3)
{
    SP_DEVINFO_DATA device = {sizeof(device)};
    BOOL ret, found_dev1 = 0, found_dev2 = 0, found_dev3 = 0;
    char id[50];
    DWORD i;

    for (i = 0; SetupDiEnumDeviceInfo(set, i, &device); ++i)
    {
        ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
        if (!ret) continue;

        if (!strcasecmp(id, "Root\\LEGACY_BOGUS\\foo"))
        {
            found_dev1 = 1;
            ok_(__FILE__, line)(IsEqualGUID(&device.ClassGuid, &guid),
                    "Got unexpected class %s.\n", wine_dbgstr_guid(&device.ClassGuid));
        }
        else if (!strcasecmp(id, "Root\\LEGACY_BOGUS\\qux"))
        {
            found_dev2 = 1;
            ok_(__FILE__, line)(IsEqualGUID(&device.ClassGuid, &guid),
                    "Got unexpected class %s.\n", wine_dbgstr_guid(&device.ClassGuid));
        }
        else if (!strcasecmp(id, "Root\\LEGACY_BOGUS\\bar"))
        {
            found_dev3 = 1;
            ok_(__FILE__, line)(IsEqualGUID(&device.ClassGuid, &guid2),
                    "Got unexpected class %s.\n", wine_dbgstr_guid(&device.ClassGuid));
        }
    }
    ok_(__FILE__, line)(found_dev1, "Expected device 1 to be enumerated.\n");
    ok_(__FILE__, line)(found_dev2, "Expected device 2 to be enumerated.\n");
    ok_(__FILE__, line)(found_dev3 == expect_dev3, "Expected device 2 %sto be enumerated.\n",
            expect_dev3 ? "" : "not ");
}
#define check_all_devices_enumerated(a,b) check_all_devices_enumerated_(__LINE__,a,b)

static void check_device_list_(int line, HDEVINFO set, const GUID *expect)
{
    SP_DEVINFO_LIST_DETAIL_DATA_A detail = {sizeof(detail)};
    BOOL ret = SetupDiGetDeviceInfoListDetailA(set, &detail);
    ok_(__FILE__, line)(ret, "Failed to get list detail, error %#lx.\n", GetLastError());
    ok_(__FILE__, line)(IsEqualGUID(&detail.ClassGuid, expect), "Expected class %s, got %s\n",
            wine_dbgstr_guid(expect), wine_dbgstr_guid(&detail.ClassGuid));
}
#define check_device_list(a,b) check_device_list_(__LINE__,a,b)

static void test_get_class_devs(void)
{
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    HDEVINFO set;
    BOOL ret;

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\foo", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &iface_guid, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &iface_guid2, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\qux", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &iface_guid, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\bar", &guid2, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#lx.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#lx.\n", GetLastError());

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(NULL, NULL, NULL, 0);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(NULL, NULL, NULL, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_all_devices_enumerated(set, TRUE);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, 0);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &guid);
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\QUX");
    check_device_info(set, 2, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_all_devices_enumerated(set, TRUE);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(NULL, "ROOT", NULL, 0);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(NULL, "ROOT", NULL, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_all_devices_enumerated(set, TRUE);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(NULL, "ROOT\\LEGACY_BOGUS", NULL, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, &guid2, "ROOT\\LEGACY_BOGUS\\BAR");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 2, &guid, "ROOT\\LEGACY_BOGUS\\QUX");
    check_device_info(set, 3, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&guid, "ROOT\\LEGACY_BOGUS", NULL, 0);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &guid);
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\QUX");
    check_device_info(set, 2, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&guid, "ROOT\\LEGACY_BOGUS", NULL, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, &guid2, "ROOT\\LEGACY_BOGUS\\BAR");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 2, &guid, "ROOT\\LEGACY_BOGUS\\QUX");
    check_device_info(set, 3, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    /* test DIGCF_DEVICE_INTERFACE */

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(NULL, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(NULL, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_all_devices_enumerated(set, FALSE);
    check_device_iface(set, NULL, &iface_guid, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 1, 0, "\\\\?\\root#legacy_bogus#qux#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 2, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadf00d-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid2, 1, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&iface_guid, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\QUX");
    check_device_info(set, 2, &guid, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 1, 0, "\\\\?\\root#legacy_bogus#qux#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 2, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&iface_guid, NULL, NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_all_devices_enumerated(set, FALSE);
    check_device_iface(set, NULL, &iface_guid, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 1, 0, "\\\\?\\root#legacy_bogus#qux#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 2, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadf00d-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid2, 1, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(NULL, "ROOT", NULL, DIGCF_DEVICEINTERFACE);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(NULL, "ROOT", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
todo_wine {
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());
}

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(NULL, "ROOT\\LEGACY_BOGUS", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
todo_wine {
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());
}

    set = SetupDiGetClassDevsA(NULL, "ROOT\\LEGACY_BOGUS\\foo", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 1, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 1, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadf00d-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid2, 1, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(NULL, "ROOT\\LEGACY_BOGUS\\bar", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(&iface_guid, "ROOT\\LEGACY_BOGUS", NULL, DIGCF_DEVICEINTERFACE);
todo_wine {
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());
}

    SetLastError(0xdeadbeef);
    set = SetupDiGetClassDevsA(&iface_guid, "ROOT\\LEGACY_BOGUS", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
todo_wine {
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#lx.\n", GetLastError());
}

    set = SetupDiGetClassDevsA(&iface_guid, "ROOT\\LEGACY_BOGUS\\foo", NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 1, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 1, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&iface_guid, "ROOT\\LEGACY_BOGUS\\foo", NULL, DIGCF_DEVICEINTERFACE | DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#lx.\n", GetLastError());
    check_device_list(set, &GUID_NULL);
    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\FOO");
    check_device_info(set, 1, NULL, NULL);
    check_device_iface(set, NULL, &iface_guid, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadbeef-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid, 1, 0, NULL);
    check_device_iface(set, NULL, &iface_guid2, 0, 0, "\\\\?\\root#legacy_bogus#foo#{deadf00d-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set, NULL, &iface_guid2, 1, 0, NULL);
    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#lx.\n", GetLastError());

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, 0);
    SetupDiEnumDeviceInfo(set, 0, &device);
    SetupDiRemoveDevice(set, &device);
    SetupDiEnumDeviceInfo(set, 1, &device);
    SetupDiRemoveDevice(set, &device);
    SetupDiDestroyDeviceInfoList(set);
    set = SetupDiGetClassDevsA(&guid2, NULL, NULL, 0);
    SetupDiEnumDeviceInfo(set, 0, &device);
    SetupDiRemoveDevice(set, &device);
    SetupDiDestroyDeviceInfoList(set);
}

START_TEST(devinst)
{
    static BOOL (WINAPI *pIsWow64Process)(HANDLE, BOOL *);
    HKEY hkey;

    test_get_actual_section();

    if ((hkey = SetupDiOpenClassRegKey(NULL, KEY_ALL_ACCESS)) == INVALID_HANDLE_VALUE)
    {
        skip("needs admin rights\n");
        return;
    }
    RegCloseKey(hkey);

    pIsWow64Process = (void *)GetProcAddress(GetModuleHandleA("kernel32.dll"), "IsWow64Process");
    if (pIsWow64Process) pIsWow64Process(GetCurrentProcess(), &wow64);

    test_create_device_list_ex();
    test_open_class_key();
    test_install_class();
    test_device_info();
    test_device_property();
    test_get_device_instance_id();
    test_open_device_info();
    test_register_device_info();
    test_device_iface();
    test_device_iface_detail();
    test_device_key();
    test_register_device_iface();
    test_registry_property_a();
    test_registry_property_w();
    test_get_inf_class();
    test_devnode();
    test_device_interface_key();
    test_open_device_interface_key();
    test_device_install_params();
    test_driver_list();
    test_call_class_installer();
    test_get_class_devs();
}
