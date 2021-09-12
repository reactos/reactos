/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test for the RegQueryValueW API
 * PROGRAMMER:      Victor Martinez Calvo <victor.martinez@reactos.org>
 */

#include "precomp.h"

static DWORD delete_key(HKEY hkey)
{
    WCHAR name[MAX_PATH];
    DWORD ret;

    while (!(ret = RegEnumKeyW(hkey, 0, name, _countof(name))))
    {
        HKEY tmp;
        if (!(ret = RegOpenKeyExW(hkey, name, 0, KEY_ENUMERATE_SUB_KEYS, &tmp)))
        {
            ret = delete_key(tmp);
        }
        if (ret)
            break;
    }
    if (ret == ERROR_NO_MORE_ITEMS)
    {
        RegDeleteKeyW(hkey, L"");
        ret = 0;
    }
    RegCloseKey(hkey);
    return ret;
}

START_TEST(RegQueryValueExW)
{
    HKEY hkey_main;
    HKEY subkey;
    DWORD type, size, ret, reserved;
    const WCHAR string1W[] = L"1";
    const WCHAR string22W[] = L"Thisstringhas22letters";
    WCHAR data22[22];
    WCHAR data23[23];
    WCHAR data24[24];


    /* If the tree key already exist, delete it to ensure proper testing*/
    if (RegOpenKeyExW(HKEY_CURRENT_USER, L"Software\\ReactOS\\advapi32_apitest", 0, KEY_ALL_ACCESS, &hkey_main) == ERROR_SUCCESS)
        delete_key(hkey_main);

    /* Ready to recreate it */
    SetLastError(0xdeadbeef);
    ret = RegCreateKeyExW(HKEY_CURRENT_USER, L"Software\\ReactOS\\advapi32_apitest", 0, NULL, 0, KEY_ALL_ACCESS, NULL, &hkey_main, NULL);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", ret);
    ok(GetLastError(), "RegCreateKeyExW failed: %lx\n", GetLastError());
    if(ret != ERROR_SUCCESS)
    {
        trace("Unable to create test key, aborting!\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = RegCreateKeyExW(hkey_main, L"subkey", 0, L"subkey class", 0, KEY_ALL_ACCESS, NULL, &subkey, NULL);
    ok(ret == ERROR_SUCCESS, "Expected ERROR_SUCCESS, got %lu\n", ret);
    ok(GetLastError(), "RegCreateKeyExW failed: %lx\n", GetLastError());

    /* All NULL */
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW(NULL, NULL, NULL, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_HANDLE, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());

    /* All NULL. Reserved not NULL */
    type = 666;
    size = 666;
    reserved = 3;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW(NULL, NULL, &reserved, NULL, NULL, NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(reserved == 3, "Expected reserved=3, reserved is: %ld\n", reserved);

    /* NULL handle. NULL value. Reserved not NULL */
    type = 666;
    size = 666;
    reserved = 3;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW(NULL, NULL, &reserved, &type, NULL, &size);
    ok(ret == ERROR_INVALID_PARAMETER, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == 666, "Expected untouched, Type was set with: %ld\n", type);
    ok(size == 666, "Expected untouched, Size was set with: %ld\n", size);
    ok(reserved == 3, "Expected reserved = 3, reserved is: %ld\n", reserved);

    /* NULL handle. NULL value */
    type = 666;
    size = 666;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW(NULL, NULL, NULL, &type, NULL, &size);
    ok(ret == ERROR_INVALID_HANDLE, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == 666, "Expected untouched, Type was set with: %ld\n", type);
    ok(size == 666, "Expected untouched, Size was set with: %ld\n", size);

    /* NULL handle . Inexistent value. Reserved not NULL */
    type = 666;
    size = 666;
    reserved = 3;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW(NULL, L"MSITESTVAR11", &reserved, &type, NULL, &size);
    ok(ret == ERROR_INVALID_PARAMETER, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == 666, "Expected untouched, Type was set with: %ld\n", type);
    ok(size == 666, "Expected untouched, Size was set with: %ld\n", size);
    ok(reserved == 3, "Expected reserved = 3, reserved is: %ld\n", reserved);

    /* NULL handle . Inexistent value. */
    type = 666;
    size = 666;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW(NULL, L"MSITESTVAR11", NULL, &type, NULL, &size);
    ok(ret == ERROR_INVALID_HANDLE, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == 666, "Expected untouched, Type was set with: %ld\n", type);
    ok(size == 666, "Expected untouched, Size was set with: %ld\n", size);

    /* NULL handle */
    type = 666;
    size = 666;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW(NULL, L"MSITESTVAR11", NULL, &type, NULL, &size);
    ok(ret == ERROR_INVALID_HANDLE, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == 666, "Expected untouched, Type was set with: %ld\n", type);
    ok(size == 666, "Expected untouched, Size was set with: %ld\n", size);

    /* INVALID handle. NULL value. Reserved not NULL */
    type = 666;
    size = 666;
    reserved = 3;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW((HKEY)-4, NULL, &reserved, &type, NULL, &size);
    ok(ret == ERROR_INVALID_PARAMETER, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == 666, "Expected untouched, Type was set with: %ld\n", type);
    ok(size == 666, "Expected untouched, Size was set with: %ld\n", size);
    ok(reserved == 3, "Expected reserved = 3, reserved is: %ld\n", reserved);

    /* INVALID handle. NULL value.*/
    type = 666;
    size = 666;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW((HKEY)-4, NULL, NULL, &type, NULL, &size);
    ok(ret == ERROR_INVALID_HANDLE, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == REG_NONE, "Expected REG_NONE, Type is: %ld\n", type);
    ok(size == 0, "Expected size = 0, size is: %ld\n", size);
    ok(reserved == 3, "Expected reserved = 3, reserved is: %ld\n", reserved);

    /* INVALID handle. Non-existent value. Reserved not NULL*/
    type = 666;
    size = 666;
    reserved = 3;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW((HKEY)-4, L"MSITESTVAR11", &reserved, &type, NULL, &size);
    ok(ret == ERROR_INVALID_PARAMETER, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == 666, "Expected untouched, Type was set with: %ld\n", type);
    ok(size == 666, "Expected untouched, Size was set with: %ld\n", size);
    ok(reserved == 3, "Expected reserved = 3, reserved is: %ld\n", reserved);

    /* INVALID handle. Non-existent value. */
    type = 666;
    size = 666;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW((HKEY)-4, L"MSITESTVAR11", NULL, &type, NULL, &size);
    ok(ret == ERROR_INVALID_HANDLE, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == REG_NONE, "Expected REG_NONE, Type is: %ld\n", type);
    ok(size == 0, "Expected size = 0, size is: %ld\n", size);

    /* VALID handle, Non-existent value, Reserved not NULL */
    type = 666;
    size = 666;
    reserved = 3;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW(hkey_main, L"MSITESTVAR11", &reserved, &type, NULL, &size);
    ok(ret == ERROR_INVALID_PARAMETER, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == 666, "Expected untouched, Type was set with: %ld\n", type);
    ok(size == 666, "Expected untouched, Size was set with: %ld\n", size);
    ok(reserved == 3, "Expected reserved = 3, reserved is: %ld\n", reserved);

    /* VALID handle, Non-existent value */
    type = 666;
    size = 666;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW(hkey_main, L"MSITESTVAR11", NULL, &type, NULL, &size);
    ok(ret == ERROR_FILE_NOT_FOUND, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == REG_NONE, "Expected REG_NONE, Type is: %ld\n", type);
    ok(size == 0, "Expected size = 0, size is: %ld\n", size);

    /* VALID handle, NULL value */
    type = 666;
    size = 666;
    SetLastError(0xdeadbeef);
    ret = RegQueryValueExW(hkey_main, NULL, NULL, &type, NULL, &size);
    ok(ret == ERROR_FILE_NOT_FOUND, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == REG_NONE, "Expected REG_NONE, Type is: %ld\n", type);
    ok(size == 0, "Expected size = 0, size is: %ld\n", size);

    /* Create the value */
    SetLastError(0xdeadbeef);
    if ((ret = RegSetValueExW(hkey_main, L"MSITESTVAR11", 0, REG_NONE, (const BYTE *)string1W, 4)) != ERROR_SUCCESS)
    {
        ok(1, "RegSetValueExW failed: %lx, %lx\n", ret, GetLastError());
    }
    if ((ret = RegSetValueExW(hkey_main, L"LONGSTRING", 0, REG_SZ, (const BYTE *)string22W, (lstrlenW(string22W)+1) * sizeof(WCHAR))) != ERROR_SUCCESS)
    {
        ok(1, "RegSetValueExW failed: %lx, %lx\n", ret, GetLastError());
    }

    /* Existent value. Reserved not NULL */
    SetLastError(0xdeadbeef);
    size = 666;
    type = 666;
    reserved = 3;
    ret = RegQueryValueExW(hkey_main, L"MSITESTVAR11", &reserved, &type, NULL, &size);
    ok(ret == ERROR_INVALID_PARAMETER, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == 666, "Expected untouched, Type was set with: %ld\n", type);
    ok(size == 666, "Expected untouched, Size was set with: %ld\n", size);
    ok(reserved == 3, "Expected reserved = 3, reserved is: %ld\n", reserved);

    /* Existent value */
    SetLastError(0xdeadbeef);
    size = 666;
    type = 666;
    ret = RegQueryValueExW(hkey_main, L"MSITESTVAR11", NULL, &type, NULL, &size);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == REG_NONE, "Expected REG_NONE, Type is: %ld\n", type);
    ok(size == 4, "Expected size = 4, size is: %ld\n", size);

    /* Data tests */
    /* Buffer one wchar smaller than needed */
    SetLastError(0xdeadbeef);
    size = sizeof(data22);
    type = 666;
    ret = RegQueryValueExW(hkey_main, L"LONGSTRING", NULL, &type, (LPBYTE)data22, &size);
    ok(ret == ERROR_MORE_DATA, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == REG_SZ, "Expected REG_SZ, Type is: %ld\n", type);
    ok(size == 46, "Expected size = 46, size is: %ld\n", size);
    ok(wcscmp(data22, string22W), "Expected being different!");

    /* Buffer has perfect size */
    SetLastError(0xdeadbeef);
    size = sizeof(data23);
    type = 666;
    ret = RegQueryValueExW(hkey_main, L"LONGSTRING", NULL, &type, (LPBYTE)data23, &size);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == REG_SZ, "Expected REG_SZ, Type is: %ld\n", type);
    ok(size == 46, "Expected size = 46, size is: %ld", size);
    ok(!wcscmp(data23,string22W), "Expected same string! data23: %S, string22W: %S", data23, string22W);

    /* Buffer one wchar bigger than needed */
    SetLastError(0xdeadbeef);
    size = sizeof(data24);
    type = 666;
    ret = RegQueryValueExW(hkey_main, L"LONGSTRING", NULL, &type, (LPBYTE)data24, &size);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == REG_SZ, "Expected REG_SZ, Type is: %ld\n", type);
    ok(size == 46, "Expected size = 46, size is: %ld\n", size);
    ok(!wcscmp(data24, string22W), "Expected same string! data24: %S, string22W: %S\n", data24, string22W);

    /* Buffer has perfect size. Size wrong: 1 WCHAR less */
    SetLastError(0xdeadbeef);
    memset(data23, 0, sizeof(data23));
    size = sizeof(data23) - 2;
    type = 666;
    ret = RegQueryValueExW(hkey_main, L"LONGSTRING", NULL, &type, (LPBYTE)data23, &size);
    ok(ret == ERROR_MORE_DATA, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == REG_SZ, "Expected REG_SZ, Type is: %ld\n", type);
    ok(size == 46, "Expected size = 46, size is: %ld", size);
    ok(wcscmp(data23, string22W), "Expected different string!\n");

    /* Buffer has perfect size. Size wrong: 1 WCHAR more */
    SetLastError(0xdeadbeef);
    memset(data23, 0, sizeof(data23));
    size = sizeof(data23) + 2;
    type = 666;
    ret = RegQueryValueExW(hkey_main, L"LONGSTRING", NULL, &type, (LPBYTE)data23, &size);
    ok(ret == ERROR_SUCCESS, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    ok(type == REG_SZ, "Expected REG_SZ, Type is: %ld\n", type);
    ok(size == 46, "Expected size = 46, size is: %ld", size);
    ok(!wcscmp(data23, string22W), "Expected same string! data23: %S, string22W: %S", data23, string22W);

    /* Ask for a var that doesnt exist. */
    SetLastError(0xdeadbeef);
    size = sizeof(data23);
    memset(data23, 0, sizeof(data23));
    type = 666;
    ret = RegQueryValueExW(hkey_main, L"XXXXXYYYYYZZZZZZ", NULL, &type, (LPBYTE)data23, &size);
    ok(ret == ERROR_FILE_NOT_FOUND, "RegQueryValueExW returned: %lx\n", ret);
    ok(GetLastError() == 0xdeadbeef, "RegQueryValueExW returned: %lx\n", GetLastError());
    /* 2k3 leaves garbage */
    ok(type == REG_NONE || broken(type != REG_NONE && type != 666), "Expected REG_NONE, Type is: %ld\n", type);
    ok(size == 46, "Expected size = 46, size is: %ld", size);
    ok(!wcscmp(data23,L""), "Expected same string! data23: %S, ''", data23);


    RegCloseKey(hkey_main);
    RegCloseKey(subkey);

    /* Delete the whole test key */
    RegOpenKeyW(HKEY_CURRENT_USER, L"Software\\ReactOS\\advapi32_apitest", &hkey_main);
    delete_key(hkey_main);
}
