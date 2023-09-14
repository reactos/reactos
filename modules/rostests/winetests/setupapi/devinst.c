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
#include "cfgmgr32.h"

#include "wine/heap.h"
#include "wine/test.h"

#ifdef __REACTOS__
DEFINE_GUID(GUID_NULL,0,0,0,0,0,0,0,0,0,0,0);
#endif

/* This is a unique guid for testing purposes */
static GUID guid = {0x6a55b5a4, 0x3f65, 0x11db, {0xb7,0x04,0x00,0x11,0x95,0x5c,0x2b,0xdb}};
static GUID guid2 = {0x6a55b5a5, 0x3f65, 0x11db, {0xb7,0x04,0x00,0x11,0x95,0x5c,0x2b,0xdb}};

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
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    set = SetupDiCreateDeviceInfoListExW(NULL, NULL, machine, NULL);
    ok(set == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_MACHINENAME
            || GetLastError() == ERROR_MACHINE_UNAVAILABLE
            || GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
            "Got unexpected error %#x.\n", GetLastError());

    set = SetupDiCreateDeviceInfoListExW(NULL, NULL, NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#x.", GetLastError());

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#x.\n", GetLastError());

    set = SetupDiCreateDeviceInfoListExW(NULL, NULL, empty, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#x.", GetLastError());

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#x.\n", GetLastError());
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
    ok(GetLastError() == ERROR_INVALID_CLASS, "Got unexpected error %#x.\n", GetLastError());

    root_key = SetupDiOpenClassRegKey(NULL, KEY_ALL_ACCESS);
    ok(root_key != INVALID_HANDLE_VALUE, "Failed to open root key, error %#x.\n", GetLastError());

    res = RegCreateKeyA(root_key, guidstr, &class_key);
    ok(!res, "Failed to create class key, error %#x.\n", GetLastError());
    RegCloseKey(class_key);

    SetLastError(0xdeadbeef);
    class_key = SetupDiOpenClassRegKeyExA(&guid, KEY_ALL_ACCESS, DIOCR_INSTALLER, NULL, NULL);
    ok(class_key != INVALID_HANDLE_VALUE, "Failed to open class key, error %#x.\n", GetLastError());
    RegCloseKey(class_key);

    RegDeleteKeyA(root_key, guidstr);
    RegCloseKey(root_key);
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

    tmpfile[0] = '.';
    tmpfile[1] = '\\';
    get_temp_filename(tmpfile + 2);
    create_inf_file(tmpfile + 2);

    ret = SetupDiInstallClassA(NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiInstallClassA(NULL, NULL, DI_NOVCP, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiInstallClassA(NULL, tmpfile + 2, DI_NOVCP, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiInstallClassA(NULL, tmpfile + 2, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Got unexpected error %#x.\n", GetLastError());

    /* The next call will succeed. Information is put into the registry but the
     * location(s) is/are depending on the Windows version.
     */
    ret = SetupDiInstallClassA(NULL, tmpfile, 0, NULL);
    ok(ret, "Failed to install class, error %#x.\n", GetLastError());

    ok(!RegDeleteKeyW(HKEY_LOCAL_MACHINE, classKey), "Failed to delete class key, error %u.\n", GetLastError());
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
        ok_(__FILE__, line)(ret, "Got unexpected error %#x.\n", GetLastError());
        ok_(__FILE__, line)(IsEqualGUID(&device.ClassGuid, class),
                "Got unexpected class %s.\n", wine_dbgstr_guid(&device.ClassGuid));
        ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
        ok_(__FILE__, line)(ret, "Got unexpected error %#x.\n", GetLastError());
        ok_(__FILE__, line)(!strcasecmp(id, expect_id), "Got unexpected id %s.\n", id);
    }
    else
    {
        ok_(__FILE__, line)(!ret, "Expected failure.\n");
        ok_(__FILE__, line)(GetLastError() == ERROR_NO_MORE_ITEMS,
                "Got unexpected error %#x.\n", GetLastError());
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

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(NULL, NULL, NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DEVINST_NAME, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(NULL, "Root\\LEGACY_BOGUS\\0000", NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &deadbeef, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_CLASS_MISMATCH, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &GUID_NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_CLASS_MISMATCH, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, &guid, NULL);

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_DEVINST_ALREADY_EXISTS, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0001", &guid, NULL, NULL, 0, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#x.\n", GetLastError());

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\0001");
    check_device_info(set, 2, &guid, NULL);

    device.cbSize = sizeof(device);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0002", &guid, NULL, NULL, 0, &device);
    ok(ret, "Got unexpected error %#x.\n", GetLastError());
    ok(IsEqualGUID(&device.ClassGuid, &guid), "Got unexpected class %s.\n",
            wine_dbgstr_guid(&device.ClassGuid));
    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Got unexpected error %#x.\n", GetLastError());
    ok(!strcmp(id, "ROOT\\LEGACY_BOGUS\\0002"), "Got unexpected id %s.\n", id);

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\0001");
    check_device_info(set, 2, &guid, "ROOT\\LEGACY_BOGUS\\0002");
    check_device_info(set, 3, &guid, NULL);

    ret = SetupDiEnumDeviceInfo(set, 0, &ret_device);
    ok(ret, "Failed to enumerate devices, error %#x.\n", GetLastError());
    ret = SetupDiDeleteDeviceInfo(set, &ret_device);
    ok(ret, "Failed to delete device, error %#x.\n", GetLastError());

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0001");
    check_device_info(set, 1, &guid, "ROOT\\LEGACY_BOGUS\\0002");
    check_device_info(set, 2, &guid, NULL);

    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Got unexpected error %#x.\n", GetLastError());

    check_device_info(set, 0, &guid, "ROOT\\LEGACY_BOGUS\\0001");

    ret = SetupDiEnumDeviceInfo(set, 1, &ret_device);
    ok(ret, "Got unexpected error %#x.\n", GetLastError());
    ok(IsEqualGUID(&ret_device.ClassGuid, &guid), "Got unexpected class %s.\n",
            wine_dbgstr_guid(&ret_device.ClassGuid));
    ret = SetupDiGetDeviceInstanceIdA(set, &ret_device, id, sizeof(id), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_NO_SUCH_DEVINST, "Got unexpected error %#x.\n", GetLastError());
    ok(ret_device.DevInst == device.DevInst, "Expected device node %#x, got %#x.\n",
            device.DevInst, ret_device.DevInst);

    check_device_info(set, 2, &guid, NULL);

    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiCreateDeviceInfoList(NULL, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device info, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\deadbeef", &deadbeef, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\null", &GUID_NULL, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\testguid", &guid, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());

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
    ok(GetLastError() == ERROR_INVALID_DEVINST_NAME, "Got unexpected error %#x.\n", GetLastError());

    id[MAX_DEVICE_ID_LEN] = 0;
    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInfoA(set, id, &guid, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DEVINST_NAME, "Got unexpected error %#x.\n", GetLastError());

    id[MAX_DEVICE_ID_LEN - 1] = 0;
    ret = SetupDiCreateDeviceInfoA(set, id, &guid, NULL, NULL, 0, NULL);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);
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
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(NULL, &device, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, &device, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, &device, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    device.cbSize = sizeof(device);
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, &device, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInstanceIdA(set, &device, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Failed to get device id, error %#x.\n", GetLastError());
    ok(!strcmp(id, "ROOT\\LEGACY_BOGUS\\0000"), "Got unexpected id %s.\n", id);

    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "SetupDiCreateDeviceInfoA failed: %08x\n", GetLastError());

    ret = SetupDiGetDeviceInstanceIdA(set, &device, id, sizeof(id), NULL);
    ok(ret, "Failed to get device id, error %#x.\n", GetLastError());
    ok(!strcmp(id, "ROOT\\LEGACY_BOGUS\\0001"), "Got unexpected id %s.\n", id);

    SetupDiDestroyDeviceInfoList(set);
}

static void test_register_device_info(void)
{
    SP_DEVINFO_DATA device = {0};
    BOOL ret;
    HDEVINFO set;
    int i = 0;

    SetLastError(0xdeadbeef);
    ret = SetupDiRegisterDeviceInfo(NULL, NULL, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiRegisterDeviceInfo(set, NULL, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    device.cbSize = sizeof(device);
    SetLastError(0xdeadbeef);
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0001", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#x.\n", GetLastError());
    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Failed to remove device, error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0002", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#x.\n", GetLastError());
    ret = SetupDiDeleteDeviceInfo(set, &device);
    ok(ret, "Failed to remove device, error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0003", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, 0);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#x.\n", GetLastError());

    check_device_info(set, 0, &guid, "Root\\LEGACY_BOGUS\\0000");
    check_device_info(set, 1, &guid, "Root\\LEGACY_BOGUS\\0002");
    check_device_info(set, 2, &guid, NULL);

    while (SetupDiEnumDeviceInfo(set, i++, &device))
    {
        ret = SetupDiRemoveDevice(set, &device);
        ok(ret, "Failed to remove device, error %#x.\n", GetLastError());
    }

    SetupDiDestroyDeviceInfoList(set);
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
        ok_(__FILE__, line)(ret, "Failed to enumerate interfaces, error %#x.\n", GetLastError());
        ok_(__FILE__, line)(IsEqualGUID(&iface.InterfaceClassGuid, class),
                "Got unexpected class %s.\n", wine_dbgstr_guid(&iface.InterfaceClassGuid));
        ok_(__FILE__, line)(iface.Flags == flags, "Got unexpected flags %#x.\n", iface.Flags);
        ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, sizeof(buffer), NULL, NULL);
        ok_(__FILE__, line)(ret, "Failed to get interface detail, error %#x.\n", GetLastError());
        ok_(__FILE__, line)(!strcasecmp(detail->DevicePath, path), "Got unexpected path %s.\n", detail->DevicePath);
    }
    else
    {
        ok_(__FILE__, line)(!ret, "Expected failure.\n");
        ok_(__FILE__, line)(GetLastError() == ERROR_NO_MORE_ITEMS,
                "Got unexpected error %#x.\n", GetLastError());
    }
}
#define check_device_iface(a,b,c,d,e,f) check_device_iface_(__LINE__,a,b,c,d,e,f)

static void test_device_iface(void)
{
    char buffer[200];
    SP_DEVICE_INTERFACE_DETAIL_DATA_A *detail = (SP_DEVICE_INTERFACE_DETAIL_DATA_A *)buffer;
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)};
    SP_DEVINFO_DATA device = {0};
    BOOL ret;
    HDEVINFO set;

    detail->cbSize = sizeof(*detail);

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(NULL, NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(NULL, NULL, &guid, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(set, NULL, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(set, &device, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    device.cbSize = sizeof(device);
    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, NULL);

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(set, &device, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, NULL);
    ok(ret, "Failed to create interface, error %#x.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid, 1, 0, NULL);

    /* Creating the same interface a second time succeeds */
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, NULL);
    ok(ret, "Failed to create interface, error %#x.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid, 1, 0, NULL);

    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, "Oogah", 0, NULL);
    ok(ret, "Failed to create interface, error %#x.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid, 1, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\Oogah");
    check_device_iface(set, &device, &guid, 2, 0, NULL);

    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, "test", 0, &iface);
    ok(ret, "Failed to create interface, error %#x.\n", GetLastError());
    ok(IsEqualGUID(&iface.InterfaceClassGuid, &guid), "Got unexpected class %s.\n",
            wine_dbgstr_guid(&iface.InterfaceClassGuid));
    ok(iface.Flags == 0, "Got unexpected flags %#x.\n", iface.Flags);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, sizeof(buffer), NULL, NULL);
    ok(ret, "Failed to get interface detail, error %#x.\n", GetLastError());
    ok(!strcasecmp(detail->DevicePath, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\test"),
            "Got unexpected path %s.\n", detail->DevicePath);

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid, 1, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\Oogah");
    check_device_iface(set, &device, &guid, 2, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\test");
    check_device_iface(set, &device, &guid, 3, 0, NULL);

    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid2, NULL, 0, NULL);
    ok(ret, "Failed to create interface, error %#x.\n", GetLastError());

    check_device_iface(set, &device, &guid2, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A5-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid2, 1, 0, NULL);

    ret = SetupDiEnumDeviceInterfaces(set, &device, &guid2, 0, &iface);
    ok(ret, "Failed to enumerate interfaces, error %#x.\n", GetLastError());
    ret = SetupDiRemoveDeviceInterface(set, &iface);
    ok(ret, "Failed to remove interface, error %#x.\n", GetLastError());

    check_device_iface(set, &device, &guid2, 0, SPINT_REMOVED, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A5-3F65-11DB-B704-0011955C2BDB}");
    check_device_iface(set, &device, &guid2, 1, 0, NULL);

    ret = SetupDiEnumDeviceInterfaces(set, &device, &guid, 0, &iface);
    ok(ret, "Failed to enumerate interfaces, error %#x.\n", GetLastError());
    ret = SetupDiDeleteDeviceInterfaceData(set, &iface);
    ok(ret, "Failed to delete interface, error %#x.\n", GetLastError());

    check_device_iface(set, &device, &guid, 0, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\Oogah");
    check_device_iface(set, &device, &guid, 1, 0, "\\\\?\\ROOT#LEGACY_BOGUS#0000#{6A55B5A4-3F65-11DB-B704-0011955C2BDB}\\test");
    check_device_iface(set, &device, &guid, 2, 0, NULL);

    ret = SetupDiDestroyDeviceInfoList(set);
    ok(ret, "Failed to destroy device list, error %#x.\n", GetLastError());
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
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, NULL, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, NULL, 100, NULL, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, NULL, 0, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#x.\n", GetLastError());

    detail = heap_alloc(size);
    expectedsize = FIELD_OFFSET(SP_DEVICE_INTERFACE_DETAIL_DATA_W, DevicePath[strlen(path) + 1]);

    detail->cbSize = 0;
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, size, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#x.\n", GetLastError());

    detail->cbSize = size;
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, size, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_USER_BUFFER, "Got unexpected error %#x.\n", GetLastError());

    detail->cbSize = sizeof(SP_DEVICE_INTERFACE_DETAIL_DATA_A);
    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceInterfaceDetailA(set, &iface, detail, size, &size, NULL);
    ok(ret, "Failed to get interface detail, error %#x.\n", GetLastError());
    ok(!strcasecmp(path, detail->DevicePath), "Got unexpected path %s.\n", detail->DevicePath);

    ret = SetupDiGetDeviceInterfaceDetailW(set, &iface, NULL, 0, &size, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#x.\n", GetLastError());
    ok(size == expectedsize, "Got unexpected size %d.\n", size);

    memset(&device, 0, sizeof(device));
    device.cbSize = sizeof(device);
    ret = SetupDiGetDeviceInterfaceDetailW(set, &iface, NULL, 0, &size, &device);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#x.\n", GetLastError());
    ok(IsEqualGUID(&device.ClassGuid, &guid), "Got unexpected class %s.\n", wine_dbgstr_guid(&device.ClassGuid));

    heap_free(detail);
    SetupDiDestroyDeviceInfoList(set);
}

static void test_device_key(void)
{
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
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    set = SetupDiCreateDeviceInfoList(&guid, NULL);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#x.\n", GetLastError());

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res != ERROR_SUCCESS, "Key should not exist.\n");
    RegCloseKey(key);

    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());
    ok(!RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key), "Key should exist.\n");
    RegCloseKey(key);

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(NULL, NULL, 0, 0, 0, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, NULL, 0, 0, 0, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, 0, 0, 0, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, 0, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_BOTH, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_FLAGS, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_DEVINFO_NOT_REGISTERED, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_KEY_DOES_NOT_EXIST, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DRIVER, NULL,
            (BYTE *)driver_path, sizeof(driver_path), NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, class_key_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");
    RegCloseKey(key);

    /* Vista+ will fail the following call to SetupDiCreateDevKeyW() if the
     * class key doesn't exist. */
    res = RegCreateKeyA(HKEY_LOCAL_MACHINE, class_key_path, &key);
    ok(!res, "Failed to create class key, error %u.\n", res);
    RegCloseKey(key);

    key = SetupDiCreateDevRegKeyW(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
    ok(key != INVALID_HANDLE_VALUE, "Failed to create device key, error %#x.\n", GetLastError());
    RegCloseKey(key);

    ok(!RegOpenKeyA(HKEY_LOCAL_MACHINE, class_key_path, &key), "Key should exist.\n");
    RegCloseKey(key);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, "System\\CurrentControlSet\\Control\\Class", &class_key);
    ok(!res, "Failed to open class key, error %u.\n", res);

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_DRIVER, NULL,
            (BYTE *)driver_path, sizeof(driver_path), NULL);
    ok(ret, "Failed to get driver property, error %#x.\n", GetLastError());
    res = RegOpenKeyA(class_key, driver_path, &key);
    ok(!res, "Failed to open driver key, error %u.\n", res);
    RegSetValueExA(key, "foo", 0, REG_SZ, (BYTE *)"bar", sizeof("bar"));
    RegCloseKey(key);

    SetLastError(0xdeadbeef);
    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
todo_wine {
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA || GetLastError() == ERROR_ACCESS_DENIED, /* win2k3 */
            "Got unexpected error %#x.\n", GetLastError());
}

    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, KEY_READ);
    ok(key != INVALID_HANDLE_VALUE, "Failed to open device key, error %#x.\n", GetLastError());
    size = sizeof(data);
    res = RegQueryValueExA(key, "foo", NULL, NULL, (BYTE *)data, &size);
    ok(!res, "Failed to get value, error %u.\n", res);
    ok(!strcmp(data, "bar"), "Got wrong data %s.\n", data);
    RegCloseKey(key);

    ret = SetupDiDeleteDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV);
    ok(ret, "Failed to delete device key, error %#x.\n", GetLastError());

    res = RegOpenKeyA(class_key, driver_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    key = SetupDiOpenDevRegKey(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, 0);
    ok(key == INVALID_HANDLE_VALUE, "Expected failure.\n");
    ok(GetLastError() == ERROR_KEY_DOES_NOT_EXIST, "Got unexpected error %#x.\n", GetLastError());

    key = SetupDiCreateDevRegKeyW(set, &device, DICS_FLAG_GLOBAL, 0, DIREG_DRV, NULL, NULL);
    ok(key != INVALID_HANDLE_VALUE, "Failed to create device key, error %#x.\n", GetLastError());
    RegCloseKey(key);

    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Failed to remove device, error %#x.\n", GetLastError());
    SetupDiDestroyDeviceInfoList(set);

    res = RegOpenKeyA(class_key, driver_path, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    /* Vista+ deletes the key automatically. */
    res = RegDeleteKeyA(HKEY_LOCAL_MACHINE, class_key_path);
    ok(!res || res == ERROR_FILE_NOT_FOUND, "Failed to delete class key, error %u.\n", res);

    RegCloseKey(class_key);
}

static void test_register_device_iface(void)
{
    static const WCHAR bogus[] = {'S','y','s','t','e','m','\\',
     'C','u','r','r','e','n','t','C','o','n','t','r','o','l','S','e','t','\\',
     'E','n','u','m','\\','R','o','o','t','\\',
     'L','E','G','A','C','Y','_','B','O','G','U','S',0};
    SP_DEVICE_INTERFACE_DATA iface = {sizeof(iface)}, ret_iface = {sizeof(ret_iface)};
    SP_DEVINFO_DATA device = {sizeof(device)};
    HDEVINFO set, set2;
    BOOL ret;
    HKEY key;
    LONG res;

    set = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_ALLCLASSES);
    ok(set != INVALID_HANDLE_VALUE, "Failed to create device list, error %#x.\n", GetLastError());

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");

    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL, NULL, 0, &device);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, NULL, 0, &iface);
    ok(ret, "Failed to create interface, error %#x.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, "removed", 0, &iface);
    ok(ret, "Failed to create interface, error %#x.\n", GetLastError());
    ret = SetupDiCreateDeviceInterfaceA(set, &device, &guid, "deleted", 0, &iface);
    ok(ret, "Failed to create interface, error %#x.\n", GetLastError());
    ret = SetupDiRegisterDeviceInfo(set, &device, 0, NULL, NULL, NULL);
    ok(ret, "Failed to register device, error %#x.\n", GetLastError());

    ret = SetupDiEnumDeviceInterfaces(set, &device, &guid, 1, &iface);
    ok(ret, "Failed to enumerate interfaces, error %#x.\n", GetLastError());
    ret = SetupDiRemoveDeviceInterface(set, &iface);
    ok(ret, "Failed to delete interface, error %#x.\n", GetLastError());
    ret = SetupDiEnumDeviceInterfaces(set, &device, &guid, 2, &iface);
    ok(ret, "Failed to enumerate interfaces, error %#x.\n", GetLastError());
    ret = SetupDiDeleteDeviceInterfaceData(set, &iface);
    ok(ret, "Failed to delete interface, error %#x.\n", GetLastError());

    set2 = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set2 != INVALID_HANDLE_VALUE, "Failed to create device list, error %#x.\n", GetLastError());

    check_device_iface(set2, NULL, &guid, 0, 0, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}");
    check_device_iface(set2, NULL, &guid, 1, 0, "\\\\?\\root#legacy_bogus#0000#{6a55b5a4-3f65-11db-b704-0011955c2bdb}\\deleted");
    check_device_iface(set2, NULL, &guid, 2, 0, NULL);

    ret = SetupDiRemoveDevice(set, &device);
    ok(ret, "Failed to remove device, error %#x.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);
    SetupDiDestroyDeviceInfoList(set2);
}

static void test_registry_property_a(void)
{
    static const CHAR bogus[] = "System\\CurrentControlSet\\Enum\\Root\\LEGACY_BOGUS";
    SP_DEVINFO_DATA device = {sizeof(device)};
    CHAR buf[6] = "";
    DWORD size, type;
    HDEVINFO set;
    BOOL ret;
    LONG res;
    HKEY key;

    set = SetupDiGetClassDevsA(&guid, NULL, 0, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "Failed to get device list, error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(NULL, NULL, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(set, NULL, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
todo_wine
    ok(GetLastError() == ERROR_INVALID_REG_PROPERTY, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, 0);
    todo_wine
    ok(!ret, "Expected failure.\n");
    /* GetLastError() returns nonsense in win2k3 */

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, (BYTE *)"Bogus", sizeof("Bogus"));
    ok(ret, "Failed to set property, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(NULL, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
todo_wine
    ok(GetLastError() == ERROR_INVALID_REG_PROPERTY, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, NULL, sizeof("Bogus"), NULL);
    ok(!ret, "Expected failure, got %d\n", ret);
    /* GetLastError() returns nonsense in win2k3 */

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#x.\n", GetLastError());
    ok(size == sizeof("Bogus"), "Got unexpected size %d.\n", size);

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#x.\n", GetLastError());
    ok(!strcmp(buf, "Bogus"), "Got unexpected property %s.\n", buf);

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, &type, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#x.\n", GetLastError());
    ok(!strcmp(buf, "Bogus"), "Got unexpected property %s.\n", buf);
    ok(type == REG_SZ, "Got unexpected type %d.\n", type);

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, 0);
    ok(ret, "Failed to set property, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_FRIENDLYNAME, NULL, (BYTE *)buf, sizeof(buf), &size);
todo_wine {
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#x.\n", GetLastError());
}

    ret = SetupDiGetDeviceRegistryPropertyA(set, &device, SPDRP_HARDWAREID, NULL, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#x.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);

    res = RegOpenKeyA(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");
}

static void test_registry_property_w(void)
{
    WCHAR friendly_name[] = {'B','o','g','u','s',0};
    SP_DEVINFO_DATA device = {sizeof(device)};
    WCHAR buf[6] = {0};
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
    ok(set != INVALID_HANDLE_VALUE, "Failed to get device list, error %#x.\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "LEGACY_BOGUS", &guid, NULL, NULL, DICD_GENERATE_ID, &device);
    ok(ret, "Failed to create device, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(NULL, NULL, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(set, NULL, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(set, &device, -1, NULL, 0);
    ok(!ret, "Expected failure.\n");
todo_wine
    ok(GetLastError() == ERROR_INVALID_REG_PROPERTY, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiSetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, 0);
    todo_wine
    ok(!ret, "Expected failure.\n");
    /* GetLastError() returns nonsense in win2k3 */

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, (BYTE *)friendly_name, sizeof(friendly_name));
    ok(ret, "Failed to set property, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(NULL, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, NULL, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Got unexpected error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, -1, NULL, NULL, 0, NULL);
    ok(!ret, "Expected failure.\n");
todo_wine
    ok(GetLastError() == ERROR_INVALID_REG_PROPERTY, "Got unexpected error %#x.\n", GetLastError());

    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, NULL, sizeof(buf), NULL);
    ok(!ret, "Expected failure.\n");
    /* GetLastError() returns nonsense in win2k3 */

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected error %#x.\n", GetLastError());
    ok(size == sizeof(friendly_name), "Got unexpected size %d.\n", size);

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#x.\n", GetLastError());
    ok(!lstrcmpW(buf, friendly_name), "Got unexpected property %s.\n", wine_dbgstr_w(buf));

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, &type, (BYTE *)buf, sizeof(buf), NULL);
    ok(ret, "Failed to get property, error %#x.\n", GetLastError());
    ok(!lstrcmpW(buf, friendly_name), "Got unexpected property %s.\n", wine_dbgstr_w(buf));
    ok(type == REG_SZ, "Got unexpected type %d.\n", type);

    SetLastError(0xdeadbeef);
    ret = SetupDiSetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, 0);
    ok(ret, "Failed to set property, error %#x.\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_FRIENDLYNAME, NULL, (BYTE *)buf, sizeof(buf), &size);
todo_wine {
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#x.\n", GetLastError());
}

    ret = SetupDiGetDeviceRegistryPropertyW(set, &device, SPDRP_HARDWAREID, NULL, NULL, 0, &size);
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_INVALID_DATA, "Got unexpected error %#x.\n", GetLastError());

    SetupDiDestroyDeviceInfoList(set);

    res = RegOpenKeyW(HKEY_LOCAL_MACHINE, bogus, &key);
    ok(res == ERROR_FILE_NOT_FOUND, "Key should not exist.\n");
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
    ok(h != INVALID_HANDLE_VALUE, "Failed to create file, error %#x.\n", GetLastError());
    CloseHandle(h);

    retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
    ok(!retval, "expected SetupDiGetINFClassA to fail!\n");

    for(i=0; i < ARRAY_SIZE(signatures); i++)
    {
        trace("testing signature %s\n", signatures[i]);
        h = CreateFileA(filename, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                        FILE_ATTRIBUTE_NORMAL, NULL);
        ok(h != INVALID_HANDLE_VALUE, "Failed to create file, error %#x.\n", GetLastError());
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
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER || GetLastError() == ERROR_INVALID_PARAMETER /* 2k3+ */,
                "Got unexpected error %#x.\n", GetLastError());

        DeleteFileA(filename);

        WritePrivateProfileStringA("Version", "Signature", signatures[i], filename);
        WritePrivateProfileStringA("Version", "ClassGUID", "WINE", filename);

        SetLastError(0xdeadbeef);
        retval = SetupDiGetINFClassA(filename, &guid, cn, MAX_PATH, &count);
        ok(!retval, "expected SetupDiGetINFClassA to fail!\n");
        ok(GetLastError() == RPC_S_INVALID_STRING_UUID || GetLastError() == ERROR_INVALID_PARAMETER /* 7+ */,
                "Got unexpected error %#x.\n", GetLastError());

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

static void test_devnode(void)
{
    HDEVINFO set;
    SP_DEVINFO_DATA device = { sizeof(SP_DEVINFO_DATA) };
    char buffer[50];
    DWORD ret;

    set = SetupDiGetClassDevsA(&guid, NULL, NULL, DIGCF_DEVICEINTERFACE);
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevs failed: %#x\n", GetLastError());
    ret = SetupDiCreateDeviceInfoA(set, "Root\\LEGACY_BOGUS\\0000", &guid, NULL,
        NULL, 0, &device);
    ok(ret, "SetupDiCreateDeviceInfo failed: %#x\n", GetLastError());

    ret = CM_Get_Device_IDA(device.DevInst, buffer, sizeof(buffer), 0);
    ok(!ret, "got %#x\n", ret);
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
    ok(set != INVALID_HANDLE_VALUE, "SetupDiGetClassDevs failed: %#x\n", GetLastError());

    ret = SetupDiCreateDeviceInfoA(set, "ROOT\\LEGACY_BOGUS\\0001", &guid, NULL, NULL, 0, &devinfo);
    ok(ret, "SetupDiCreateDeviceInfo failed: %#x\n", GetLastError());

    ret = SetupDiCreateDeviceInterfaceA(set, &devinfo, &guid, NULL, 0, &iface);
    ok(ret, "SetupDiCreateDeviceInterface failed: %#x\n", GetLastError());

    ret = RegOpenKeyA(HKEY_LOCAL_MACHINE, keypath, &parent);
    ok(!ret, "failed to open device parent key: %u\n", ret);

    ret = RegOpenKeyA(parent, "#\\Device Parameters", &key);
    ok(ret == ERROR_FILE_NOT_FOUND, "key shouldn't exist\n");

    dikey = SetupDiCreateDeviceInterfaceRegKeyA(set, &iface, 0, KEY_ALL_ACCESS, NULL, NULL);
    ok(dikey != INVALID_HANDLE_VALUE, "got error %u\n", GetLastError());

    ret = RegOpenKeyA(parent, "#\\Device Parameters", &key);
    ok(!ret, "key should exist: %u\n", ret);

    ret = RegSetValueA(key, NULL, REG_SZ, "test", 5);
    sz = sizeof(buffer);
    ret = RegQueryValueA(dikey, NULL, buffer, &sz);
    ok(!ret, "RegQueryValue failed: %u\n", ret);
    ok(!strcmp(buffer, "test"), "got wrong data %s\n", buffer);

    RegCloseKey(dikey);
    RegCloseKey(key);

    ret = SetupDiDeleteDeviceInterfaceRegKey(set, &iface, 0);
    ok(ret, "got error %u\n", GetLastError());

    ret = RegOpenKeyA(parent, "#\\Device Parameters", &key);
    ok(ret == ERROR_FILE_NOT_FOUND, "key shouldn't exist\n");

    RegCloseKey(parent);
    SetupDiRemoveDeviceInterface(set, &iface);
    SetupDiRemoveDevice(set, &devinfo);
    SetupDiDestroyDeviceInfoList(set);
}

START_TEST(devinst)
{
    HKEY hkey;

    if ((hkey = SetupDiOpenClassRegKey(NULL, KEY_ALL_ACCESS)) == INVALID_HANDLE_VALUE)
    {
        skip("needs admin rights\n");
        return;
    }
    RegCloseKey(hkey);

    test_create_device_list_ex();
    test_open_class_key();
    test_install_class();
    test_device_info();
    test_get_device_instance_id();
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
}
