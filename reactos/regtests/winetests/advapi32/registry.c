/*
 * Unit tests for registry functions
 *
 * Copyright (c) 2002 Alexandre Julliard
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdarg.h>
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "winerror.h"

static HKEY hkey_main;

static const char * sTestpath1 = "%LONGSYSTEMVAR%\\subdir1";
static const char * sTestpath2 = "%FOO%\\subdir1";

/* delete key and all its subkeys */
static DWORD delete_key( HKEY hkey )
{
    char name[MAX_PATH];
    DWORD ret;

    while (!(ret = RegEnumKeyA(hkey, 0, name, sizeof(name))))
    {
        HKEY tmp;
        if (!(ret = RegOpenKeyExA( hkey, name, 0, KEY_ENUMERATE_SUB_KEYS, &tmp )))
        {
            ret = delete_key( tmp );
            RegCloseKey( tmp );
        }
        if (ret) break;
    }
    if (ret != ERROR_NO_MORE_ITEMS) return ret;
    RegDeleteKeyA( hkey, "" );
    return 0;
}

static void setup_main_key(void)
{
    if (RegOpenKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey_main )) delete_key( hkey_main );

    assert (!RegCreateKeyA( HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkey_main ));
}

static void create_test_entries(void)
{
    SetEnvironmentVariableA("LONGSYSTEMVAR", "bar");
    SetEnvironmentVariableA("FOO", "ImARatherLongButIndeedNeededString");

    ok(!RegSetValueExA(hkey_main,"Test1",0,REG_EXPAND_SZ, sTestpath1, strlen(sTestpath1)+1),
        "RegSetValueExA failed\n");
    ok(!RegSetValueExA(hkey_main,"Test2",0,REG_SZ, sTestpath1, strlen(sTestpath1)+1),
        "RegSetValueExA failed\n");
    ok(!RegSetValueExA(hkey_main,"Test3",0,REG_EXPAND_SZ, sTestpath2, strlen(sTestpath2)+1),
        "RegSetValueExA failed\n");
}

static void test_enum_value(void)
{
    DWORD res;
    HKEY test_key;
    char value[20], data[20];
    WCHAR valueW[20], dataW[20];
    DWORD val_count, data_count, type;
    static const WCHAR foobarW[] = {'f','o','o','b','a','r',0};
    static const WCHAR testW[] = {'T','e','s','t',0};
    static const WCHAR xxxW[] = {'x','x','x','x','x','x','x','x',0};

    /* create the working key for new 'Test' value */
    res = RegCreateKeyA( hkey_main, "TestKey", &test_key );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", res);

    /* check NULL data with zero length */
    res = RegSetValueExA( test_key, "Test", 0, REG_SZ, NULL, 0 );
    if (GetVersion() & 0x80000000)
        ok( res == ERROR_INVALID_PARAMETER, "RegSetValueExA returned %ld\n", res );
    else
        ok( !res, "RegSetValueExA returned %ld\n", res );
    res = RegSetValueExA( test_key, "Test", 0, REG_EXPAND_SZ, NULL, 0 );
    ok( ERROR_SUCCESS == res || ERROR_INVALID_PARAMETER == res, "RegSetValueExA returned %ld\n", res );
    res = RegSetValueExA( test_key, "Test", 0, REG_BINARY, NULL, 0 );
    ok( ERROR_SUCCESS == res || ERROR_INVALID_PARAMETER == res, "RegSetValueExA returned %ld\n", res );

    res = RegSetValueExA( test_key, "Test", 0, REG_SZ, (BYTE *)"foobar", 7 );
    ok( res == 0, "RegSetValueExA failed error %ld\n", res );

    /* overflow both name and data */
    val_count = 2;
    data_count = 2;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, data, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 2, "val_count set to %ld\n", val_count );
    ok( data_count == 7, "data_count set to %ld instead of 7\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !strcmp( value, "xxxxxxxxxx" ), "value set to '%s'\n", value );
    ok( !strcmp( data, "xxxxxxxxxx" ), "data set to '%s'\n", data );

    /* overflow name */
    val_count = 3;
    data_count = 20;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, data, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    /* Win9x returns 2 as specified by MSDN but NT returns 3... */
    ok( val_count == 2 || val_count == 3, "val_count set to %ld\n", val_count );
    ok( data_count == 7, "data_count set to %ld instead of 7\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
#if 0
    /* v5.1.2600.0 (XP Home) does not touch value or data in this case */
    ok( !strcmp( value, "Te" ), "value set to '%s' instead of 'Te'\n", value );
    ok( !strcmp( data, "foobar" ), "data set to '%s' instead of 'foobar'\n", data );
#endif

    /* overflow empty name */
    val_count = 0;
    data_count = 20;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, data, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 0, "val_count set to %ld\n", val_count );
    ok( data_count == 7, "data_count set to %ld instead of 7\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !strcmp( value, "xxxxxxxxxx" ), "value set to '%s'\n", value );
#if 0
    /* v5.1.2600.0 (XP Home) does not touch data in this case */
    ok( !strcmp( data, "foobar" ), "data set to '%s' instead of 'foobar'\n", data );
#endif

    /* overflow data */
    val_count = 20;
    data_count = 2;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, data, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 20, "val_count set to %ld\n", val_count );
    ok( data_count == 7, "data_count set to %ld instead of 7\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !strcmp( value, "xxxxxxxxxx" ), "value set to '%s'\n", value );
    ok( !strcmp( data, "xxxxxxxxxx" ), "data set to '%s'\n", data );

    /* no overflow */
    val_count = 20;
    data_count = 20;
    type = 1234;
    strcpy( value, "xxxxxxxxxx" );
    strcpy( data, "xxxxxxxxxx" );
    res = RegEnumValueA( test_key, 0, value, &val_count, NULL, &type, data, &data_count );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", res );
    ok( val_count == 4, "val_count set to %ld instead of 4\n", val_count );
    ok( data_count == 7, "data_count set to %ld instead of 7\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !strcmp( value, "Test" ), "value is '%s' instead of Test\n", value );
    ok( !strcmp( data, "foobar" ), "data is '%s' instead of foobar\n", data );

    /* Unicode tests */

    SetLastError(0);
    res = RegSetValueExW( test_key, testW, 0, REG_SZ, (const BYTE *)foobarW, 7*sizeof(WCHAR) );
    if (res==0 && GetLastError()==ERROR_CALL_NOT_IMPLEMENTED)
        return;
    ok( res == 0, "RegSetValueExW failed error %ld\n", res );

    /* overflow both name and data */
    val_count = 2;
    data_count = 2;
    type = 1234;
    memcpy( valueW, xxxW, sizeof(xxxW) );
    memcpy( dataW, xxxW, sizeof(xxxW) );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 2, "val_count set to %ld\n", val_count );
    ok( data_count == 7*sizeof(WCHAR), "data_count set to %ld instead of 7*sizeof(WCHAR)\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !memcmp( valueW, xxxW, sizeof(xxxW) ), "value modified\n" );
    ok( !memcmp( dataW, xxxW, sizeof(xxxW) ), "data modified\n" );

    /* overflow name */
    val_count = 3;
    data_count = 20;
    type = 1234;
    memcpy( valueW, xxxW, sizeof(xxxW) );
    memcpy( dataW, xxxW, sizeof(xxxW) );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 3, "val_count set to %ld\n", val_count );
    ok( data_count == 7*sizeof(WCHAR), "data_count set to %ld instead of 7*sizeof(WCHAR)\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !memcmp( valueW, xxxW, sizeof(xxxW) ), "value modified\n" );
    ok( !memcmp( dataW, xxxW, sizeof(xxxW) ), "data modified\n" );

    /* overflow data */
    val_count = 20;
    data_count = 2;
    type = 1234;
    memcpy( valueW, xxxW, sizeof(xxxW) );
    memcpy( dataW, xxxW, sizeof(xxxW) );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %ld\n", res );
    ok( val_count == 4, "val_count set to %ld instead of 4\n", val_count );
    ok( data_count == 7*sizeof(WCHAR), "data_count set to %ld instead of 7*sizeof(WCHAR)\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !memcmp( valueW, testW, sizeof(testW) ), "value is not 'Test'\n" );
    ok( !memcmp( dataW, xxxW, sizeof(xxxW) ), "data modified\n" );

    /* no overflow */
    val_count = 20;
    data_count = 20;
    type = 1234;
    memcpy( valueW, xxxW, sizeof(xxxW) );
    memcpy( dataW, xxxW, sizeof(xxxW) );
    res = RegEnumValueW( test_key, 0, valueW, &val_count, NULL, &type, (BYTE*)dataW, &data_count );
    ok( res == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", res );
    ok( val_count == 4, "val_count set to %ld instead of 4\n", val_count );
    ok( data_count == 7*sizeof(WCHAR), "data_count set to %ld instead of 7*sizeof(WCHAR)\n", data_count );
    ok( type == REG_SZ, "type %ld is not REG_SZ\n", type );
    ok( !memcmp( valueW, testW, sizeof(testW) ), "value is not 'Test'\n" );
    ok( !memcmp( dataW, foobarW, sizeof(foobarW) ), "data is not 'foobar'\n" );
}

static void test_query_value_ex()
{
    DWORD ret;
    DWORD size;
    DWORD type;

    ret = RegQueryValueExA(hkey_main, "Test2", NULL, &type, NULL, &size);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(size == strlen(sTestpath1) + 1, "(%ld,%ld)\n", (DWORD)strlen(sTestpath1) + 1, size);
    ok(type == REG_SZ, "type %ld is not REG_SZ\n", type);
}

static void test_reg_open_key()
{
    DWORD ret = 0;
    HKEY hkResult = NULL;
    HKEY hkPreserve = NULL;

    /* successful open */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult != NULL, "expected hkResult != NULL\n");
    hkPreserve = hkResult;

    /* open same key twice */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult != hkPreserve, "epxected hkResult != hkPreserve\n");
    ok(hkResult != NULL, "hkResult != NULL\n");
    RegCloseKey(hkResult);

    /* open nonexistent key
     * check that hkResult is set to NULL
     */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Nonexistent", &hkResult);
    ok(ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %ld\n", ret);
    ok(hkResult == NULL, "expected hkResult == NULL\n");

    /* open the same nonexistent key again to make sure the key wasn't created */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Nonexistent", &hkResult);
    ok(ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %ld\n", ret);
    ok(hkResult == NULL, "expected hkResult == NULL\n");

    /* send in NULL lpSubKey
     * check that hkResult receives the value of hKey
     */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, NULL, &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult == HKEY_CURRENT_USER, "expected hkResult == HKEY_CURRENT_USER\n");

    /* send empty-string in lpSubKey */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "", &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult == HKEY_CURRENT_USER, "expected hkResult == HKEY_CURRENT_USER\n");

    /* send in NULL lpSubKey and NULL hKey
     * hkResult is set to NULL
     */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(NULL, NULL, &hkResult);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
    ok(hkResult == NULL, "expected hkResult == NULL\n");

    /* only send NULL hKey
     * the value of hkResult remains unchanged
     */
    hkResult = hkPreserve;
    ret = RegOpenKeyA(NULL, "Software\\Wine\\Test", &hkResult);
    ok(ret == ERROR_INVALID_HANDLE || ret == ERROR_BADKEY, /* Windows 95 returns BADKEY */
       "expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %ld\n", ret);
    ok(hkResult == hkPreserve, "expected hkResult == hkPreserve\n");
    RegCloseKey(hkResult);

    /* send in NULL hkResult */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", NULL);
    ok(ret == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", ret);
}

static void test_reg_close_key()
{
    DWORD ret = 0;
    HKEY hkHandle;

    /* successfully close key
     * hkHandle remains changed after call to RegCloseKey
     */
    ret = RegOpenKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Test", &hkHandle);
    ret = RegCloseKey(hkHandle);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    /* try to close the key twice */
    ret = RegCloseKey(hkHandle); /* Windows 95 doesn't mind. */
    ok(ret == ERROR_INVALID_HANDLE || ret == ERROR_SUCCESS,
       "expected ERROR_INVALID_HANDLE or ERROR_SUCCESS, got %ld\n", ret);

    /* try to close a NULL handle */
    ret = RegCloseKey(NULL);
    ok(ret == ERROR_INVALID_HANDLE || ret == ERROR_BADKEY, /* Windows 95 returns BADKEY */
       "expected ERROR_INVALID_HANDLE or ERROR_BADKEY, got %ld\n", ret);
}

static void test_reg_delete_key()
{
    DWORD ret;
    HKEY hkResult = NULL;
    
    ret = RegDeleteKey(hkey_main, NULL);
    ok(ret == ERROR_INVALID_PARAMETER || ret == ERROR_ACCESS_DENIED,
       "expected ERROR_INVALID_PARAMETER or ERROR_ACCESS_DENIED, got %ld\n", ret);
       
    ret = RegCreateKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Tost", &hkResult);

    ret = RegDeleteValue(hkResult, "noExists");
    ok(ret == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND, got %ld\n", ret);

    ret = RegDeleteKeyA(HKEY_CURRENT_USER, "Software\\Wine\\Tost");
    
    ret = RegCloseKey(hkResult); 
  
}

static void test_reg_save_key()
{
    DWORD ret;

    ret = RegSaveKey(hkey_main, "saved_key", NULL);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);
}

static void test_reg_load_key()
{
    DWORD ret;
    HKEY hkHandle;

    ret = RegLoadKey(HKEY_LOCAL_MACHINE, "Test", "saved_key");
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    ret = RegOpenKey(HKEY_LOCAL_MACHINE, "Test", &hkHandle);
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    RegCloseKey(hkHandle);
}

static void test_reg_unload_key()
{
    DWORD ret;

    ret = RegUnLoadKey(HKEY_LOCAL_MACHINE, "Test");
    ok(ret == ERROR_SUCCESS, "expected ERROR_SUCCESS, got %ld\n", ret);

    DeleteFile("saved_key");
}

static BOOL set_privileges(LPCSTR privilege, BOOL set)
{
    TOKEN_PRIVILEGES tp;
    HANDLE hToken;
    LUID luid;

    if (!OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &hToken))
        return FALSE;

    if(!LookupPrivilegeValue(NULL, privilege, &luid))
    {
        CloseHandle(hToken);
        return FALSE;
    }

    tp.PrivilegeCount = 1;
    tp.Privileges[0].Luid = luid;

    if (set)
        tp.Privileges[0].Attributes = SE_PRIVILEGE_ENABLED;
    else
        tp.Privileges[0].Attributes = 0;

    AdjustTokenPrivileges(hToken, FALSE, &tp, sizeof(TOKEN_PRIVILEGES), NULL, NULL);
    if (GetLastError() != ERROR_SUCCESS)
    {
        CloseHandle(hToken);
        return FALSE;
    }

    CloseHandle(hToken);
    return TRUE;
}

START_TEST(registry)
{
    setup_main_key();
    create_test_entries();
    test_enum_value();
    test_query_value_ex();
    test_reg_open_key();
    test_reg_close_key();
    test_reg_delete_key();

    /* SaveKey/LoadKey require the SE_BACKUP_NAME privilege to be set */
    if (set_privileges(SE_BACKUP_NAME, TRUE) &&
        set_privileges(SE_RESTORE_NAME, TRUE))
    {
        test_reg_save_key();
        test_reg_load_key();
        test_reg_unload_key();

        set_privileges(SE_BACKUP_NAME, FALSE);
        set_privileges(SE_RESTORE_NAME, FALSE);
    }
    /* cleanup */
    delete_key( hkey_main );
}
