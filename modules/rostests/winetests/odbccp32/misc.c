/*
 * Copyright 2007 Bill Medland
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

#include <wine/test.h>
#include <stdarg.h>

#include "windef.h"
#include "winbase.h"
#include "winreg.h"
#include "odbcinst.h"

static const WCHAR abcd_key[] = {'S','o','f','t','w','a','r','e','\\','O','D','B','C','\\','a','b','c','d','.','I','N','I','\\','w','i','n','e','o','d','b','c',0};
static const WCHAR abcdini_key[] = {'S','o','f','t','w','a','r','e','\\','O','D','B','C','\\','a','b','c','d','.','I','N','I',0 };

static void check_error_(int line, DWORD expect)
{
    RETCODE ret;
    DWORD err;
    ret = SQLInstallerError(1, &err, NULL, 0, NULL);
    ok_(__FILE__, line)(ret == SQL_SUCCESS_WITH_INFO, "got %d\n", ret);
    ok_(__FILE__, line)(err == expect, "expected %u, got %u\n", expect, ret);
}
#define check_error(a) check_error_(__LINE__, a)

static void test_SQLConfigMode(void)
{
    BOOL bool_ret;
    DWORD error_code;
    RETCODE sql_ret;
    UWORD config_mode;
    int i;

    ok(SQLGetConfigMode(NULL), "SQLGetConfigMode(NULL) should succeed\n");

    bool_ret = SQLGetConfigMode(&config_mode);
    ok(bool_ret && config_mode == ODBC_BOTH_DSN, "Failed to get the initial SQLGetConfigMode or it was not both\n");

    /* try to set invalid mode */
    bool_ret = SQLSetConfigMode(ODBC_SYSTEM_DSN+1);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(!bool_ret && sql_ret == SQL_SUCCESS_WITH_INFO && error_code == ODBC_ERROR_INVALID_PARAM_SEQUENCE, "SQLSetConfigMode with invalid argument did not fail correctly\n");

    for (i = ODBC_SYSTEM_DSN; i >= ODBC_BOTH_DSN; --i)
    {
        ok(SQLSetConfigMode((UWORD)i), "SQLSetConfigMode Failed to set config mode\n");
        bool_ret = SQLGetConfigMode(&config_mode);
        ok(bool_ret && config_mode == i, "Failed to confirm SQLSetConfigMode.\n");
    }
    /* And that leaves it correctly on BOTH */
}

static void test_SQLInstallerError(void)
{
    RETCODE sql_ret;

    /* MSDN states that the error number should be between 1 and 8.  Passing 0 is an error */
    sql_ret = SQLInstallerError(0, NULL, NULL, 0, NULL);
    ok(sql_ret == SQL_ERROR, "SQLInstallerError(0...) failed with %d instead of SQL_ERROR\n", sql_ret);
    /* However numbers greater than 8 do not return SQL_ERROR.
     * I am currently unsure as to whether it should return SQL_NO_DATA or "the same as for error 8";
     * I have never been able to generate 8 errors to test it
     */
    sql_ret = SQLInstallerError(65535, NULL, NULL, 0, NULL);
    ok(sql_ret == SQL_NO_DATA, "SQLInstallerError(>8...) failed with %d instead of SQL_NO_DATA\n", sql_ret);

    /* Force an error to work with.  This should generate ODBC_ERROR_INVALID_BUFF_LEN */
    ok(!SQLGetInstalledDrivers(0, 0, 0), "Failed to force an error for testing\n");
    sql_ret = SQLInstallerError(2, NULL, NULL, 0, NULL);
    ok(sql_ret == SQL_NO_DATA, "Too many errors when forcing an error for testing\n");

    /* Null pointers are acceptable in all obvious places */
    sql_ret = SQLInstallerError(1, NULL, NULL, 0, NULL);
    ok(sql_ret == SQL_SUCCESS_WITH_INFO, "SQLInstallerError(null addresses) failed with %d instead of SQL_SUCCESS_WITH_INFO\n", sql_ret);
}

static void test_SQLInstallDriverManager(void)
{
    BOOL bool_ret;
    RETCODE sql_ret;
    DWORD error_code;
    CHAR target_path[MAX_PATH];
    WORD path_out;

    /* NULL check */
    bool_ret = SQLInstallDriverManager(NULL, 0, NULL);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(!bool_ret, "SQLInstallDriverManager unexpectedly succeeded\n");
    ok(sql_ret == SQL_SUCCESS_WITH_INFO && error_code == ODBC_ERROR_INVALID_BUFF_LEN,
        "Expected SQLInstallDriverManager to fail with ODBC_ERROR_INVALID_BUFF_LEN\n");

    /* Length smaller than MAX_PATH */
    bool_ret = SQLInstallDriverManager(target_path, MAX_PATH / 2, NULL);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(!bool_ret, "SQLInstallDriverManager unexpectedly succeeded\n");
    ok(sql_ret == SQL_SUCCESS_WITH_INFO && error_code == ODBC_ERROR_INVALID_BUFF_LEN,
        "Expected SQLInstallDriverManager to fail with ODBC_ERROR_INVALID_BUFF_LEN\n");

    path_out = 0xcafe;
    bool_ret = SQLInstallDriverManager(target_path, MAX_PATH / 2, &path_out);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(!bool_ret, "SQLInstallDriverManager unexpectedly succeeded\n");
    ok(sql_ret == SQL_SUCCESS_WITH_INFO && error_code == ODBC_ERROR_INVALID_BUFF_LEN,
        "Expected SQLInstallDriverManager to fail with ODBC_ERROR_INVALID_BUFF_LEN\n");
    ok(path_out == 0xcafe, "Expected path_out to not have changed\n");

    /* Length OK */
    bool_ret = SQLInstallDriverManager(target_path, MAX_PATH, NULL);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    if(!bool_ret && error_code == ODBC_ERROR_WRITING_SYSINFO_FAILED)
    {
         win_skip("not enough privileges\n");
         return;
    }
    ok(bool_ret, "SQLInstallDriverManager unexpectedly failed: %d\n",
        error_code);
    if (bool_ret)
        ok(sql_ret == SQL_NO_DATA, "Expected SQL_NO_DATA, got %d\n", sql_ret);
    else
        ok(sql_ret == SQL_SUCCESS_WITH_INFO,
            "Expected SQL_SUCCESS_WITH_INFO, got %d\n", sql_ret);

    path_out = 0xcafe;
    bool_ret = SQLInstallDriverManager(target_path, MAX_PATH, &path_out);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(bool_ret, "SQLInstallDriverManager unexpectedly failed: %d\n",
        error_code);
    if (bool_ret)
        ok(sql_ret == SQL_NO_DATA, "Expected SQL_NO_DATA, got %d\n", sql_ret);
    else
        ok(sql_ret == SQL_SUCCESS_WITH_INFO,
            "Expected SQL_SUCCESS_WITH_INFO, got %d\n", sql_ret);
    /* path_out should in practice be less than 0xcafe */
    ok(path_out != 0xcafe, "Expected path_out to show the correct amount of bytes\n");
}

static void test_SQLWritePrivateProfileString(void)
{
   static const WCHAR odbc_key[] = {'S','o','f','t','w','a','r','e','\\','O','D','B','C','\\','O','D','B','C','.','I','N','I','\\','w','i','n','e','o','d','b','c',0};
   BOOL ret;
   LONG reg_ret;
   DWORD error_code;

   ret = SQLWritePrivateProfileString("wineodbc", "testing" , "value", "");
   ok(!ret, "SQLWritePrivateProfileString passed\n");
   SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
   ok(error_code == ODBC_ERROR_INVALID_STR, "SQLInstallerErrorW ret: %d\n", error_code);

   ret = SQLWritePrivateProfileString("wineodbc", "testing" , "value", NULL);
   ok(!ret, "SQLWritePrivateProfileString passed\n");
   SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
   ok(error_code == ODBC_ERROR_INVALID_STR, "SQLInstallerErrorW ret: %d\n", error_code);

   ret = SQLWritePrivateProfileString("wineodbc", "testing" , "value", "odbc.ini");
   ok(ret, "SQLWritePrivateProfileString failed\n");
   if(ret)
   {
        HKEY hkey;

        ret = SQLWritePrivateProfileString("wineodbc", "testing" , NULL, "odbc.ini");
        ok(ret, "SQLWritePrivateProfileString failed\n");

        reg_ret = RegOpenKeyExW(HKEY_CURRENT_USER, odbc_key, 0, KEY_READ, &hkey);
        ok(reg_ret == ERROR_SUCCESS, "RegOpenKeyExW failed\n");
        if(reg_ret == ERROR_SUCCESS)
        {
            reg_ret = RegDeleteKeyW(HKEY_CURRENT_USER, odbc_key);
            ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed\n");

            RegCloseKey(hkey);
        }
   }

   ret = SQLWritePrivateProfileString("wineodbc", "testing" , "value", "abcd.ini");
   ok(ret, "SQLWritePrivateProfileString failed\n");
   if(ret)
   {
        HKEY hkey;

        reg_ret = RegOpenKeyExW(HKEY_CURRENT_USER, abcd_key, 0, KEY_READ, &hkey);
        ok(reg_ret == ERROR_SUCCESS, "RegOpenKeyExW failed\n");
        if(reg_ret == ERROR_SUCCESS)
        {
            reg_ret = RegDeleteKeyW(HKEY_CURRENT_USER, abcd_key);
            ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed\n");

            RegCloseKey(hkey);
        }

        /* Cleanup key */
        reg_ret = RegDeleteKeyW(HKEY_CURRENT_USER, abcdini_key);
        ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed\n");
   }
}

static void test_SQLGetPrivateProfileString(void)
{
    int ret;
    char buffer[256] = {0};
    LONG reg_ret;

    strcpy(buffer, "wine");
    ret = SQLGetPrivateProfileString(NULL, "testing" , "default", buffer, 256, "ODBC.INI");
    ok(ret == 0, "SQLGetPrivateProfileString returned %d\n", ret);
    ok(!strcmp(buffer, ""), "incorrect string '%s'\n", buffer);

    strcpy(buffer, "wine");
    ret = SQLGetPrivateProfileString("wineodbc", NULL , "default", buffer, 256, "ODBC.INI");
    ok(ret == 0, "SQLGetPrivateProfileString returned %d\n", ret);
    ok(!strcmp(buffer, ""), "incorrect string '%s'\n", buffer);

    strcpy(buffer, "value");
    ret = SQLGetPrivateProfileString("wineodbc", "testing" , NULL, buffer, 256, "ODBC.INI");
    ok(ret == 0, "SQLGetPrivateProfileString returned %d\n", ret);
    ok(!strcmp(buffer, ""), "incorrect string '%s'\n", buffer);

    ret = SQLGetPrivateProfileString("wineodbc", "testing" , "defaultX", buffer, 256, "ODBC.INI");
    ok(ret == strlen("defaultX"), "SQLGetPrivateProfileString returned %d\n", ret);
    ok(!strcmp(buffer, "defaultX"), "incorrect string '%s'\n", buffer);

    ret = SQLGetPrivateProfileString("wineodbc", "testing" , "defaultX", buffer, 4, "ODBC.INI");
    ok(ret == strlen("def"), "SQLGetPrivateProfileString returned %d\n", ret);
    ok(!strcmp(buffer, "def"), "incorrect string '%s'\n", buffer);

    ret = SQLGetPrivateProfileString("wineodbc", "testing" , "defaultX", buffer, 8, "ODBC.INI");
    ok(ret == strlen("default"), "SQLGetPrivateProfileString returned %d\n", ret);
    ok(!strcmp(buffer, "default"), "incorrect string '%s'\n", buffer);

    ret = SQLGetPrivateProfileString("wineodbc", "testing" , "defaultX", NULL, 256, "ODBC.INI");
    ok(ret == 0, "SQLGetPrivateProfileString returned %d\n", ret);

    strcpy(buffer, "value");
    ret = SQLGetPrivateProfileString("wineodbc", "testing" , "defaultX", buffer, 0, "ODBC.INI");
    ok(ret == 0, "SQLGetPrivateProfileString returned %d\n", ret);
    ok(!strcmp(buffer, "value"), "incorrect string '%s'\n", buffer);

    ret = SQLWritePrivateProfileString("wineodbc", "testing" , "value0123456789", "abcd.ini");
    ok(ret, "SQLWritePrivateProfileString failed\n");
    if(ret)
    {
        HKEY hkey;

        ret = SQLGetPrivateProfileString("wineodbc", "testing" , "defaultY", buffer, 256, "abcd.ini");
        ok(ret == strlen("value0123456789"), "SQLGetPrivateProfileString returned %d\n", ret);
        ok(!strcmp(buffer, "value0123456789"), "incorrect string '%s'\n", buffer);

        ret = SQLGetPrivateProfileString("wineodbc", "testing" , "defaultY", NULL, 0, "abcd.ini");
        ok(ret == 0, "SQLGetPrivateProfileString returned %d\n", ret);

        ret = SQLGetPrivateProfileString("wineodbc", "testing" , "defaultY", buffer, 7, "abcd.ini");
        ok(ret == 6, "SQLGetPrivateProfileString returned %d\n", ret);

        strcpy(buffer, "wine");
        ret = SQLGetPrivateProfileString("wineodbc", NULL , "", buffer, 10, "abcd.ini");
        ok(ret == (strlen("testing")+1), "SQLGetPrivateProfileString returned %d\n", ret);
        ok(!strcmp(buffer, "testing"), "incorrect string '%s'\n", buffer);

        ret = SQLWritePrivateProfileString("wineodbc", "value" , "0", "abcd.ini");
        ok(ret, "SQLWritePrivateProfileString failed\n");

        strcpy(buffer, "wine");
        ret = SQLGetPrivateProfileString("wineodbc", NULL , "", buffer, 256, "abcd.ini");
        ok(ret == (strlen("testing") + strlen("value")+2), "SQLGetPrivateProfileString returned %d\n", ret);
        if(ret >= (strlen("testing") + strlen("value")+2))
        {
            ok(memcmp(buffer, "testing\0value\0", 14) == 0, "incorrect string '%s'\n", buffer);
        }

        strcpy(buffer, "XXXXXXXXXXXXXXX");
        ret = SQLGetPrivateProfileString("wineodbc", NULL , "", buffer, 10, "abcd.ini");
        ok(ret == (strlen("testing")+1), "SQLGetPrivateProfileString returned %d\n", ret);
        if(ret >= (strlen("testing")+1))
        {
            ok(!strcmp(buffer, "testing"), "incorrect string '%s'\n", buffer);
            /* Show that the buffer is cleared and partial enteries aren't added */
            ok(memcmp(buffer, "testing\0X", 9) != 0, "incorrect string '%s'\n", buffer);
        }

        strcpy(buffer, "wine");
        ret = SQLGetPrivateProfileString("wineodbc", NULL , "", buffer, 2, "abcd.ini");
        ok(ret == 0, "SQLGetPrivateProfileString returned %d\n", ret);

        reg_ret = RegOpenKeyExW(HKEY_CURRENT_USER, abcd_key, 0, KEY_READ, &hkey);
        ok(reg_ret == ERROR_SUCCESS, "RegOpenKeyExW failed\n");
        if(reg_ret == ERROR_SUCCESS)
        {
            reg_ret = RegDeleteKeyW(HKEY_CURRENT_USER, abcd_key);
            ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed\n");

            RegCloseKey(hkey);
        }

        /* Cleanup key */
        reg_ret = RegDeleteKeyW(HKEY_CURRENT_USER, abcdini_key);
        ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed\n");
    }
}

static void test_SQLGetPrivateProfileStringW(void)
{
    static WCHAR testing[] = {'t','e','s','t','i','n','g',0};
    static WCHAR wineodbc[] = {'w','i','n','e','o','d','b','c',0};
    static WCHAR defaultval[] = {'d','e','f','a','u','l','t',0};
    static WCHAR odbcini[] = {'O','D','B','C','.','I','N','I',0};
    static WCHAR abcdini[] = {'a','b','c','d','.','I','N','I',0};
    static WCHAR wine[] = {'w','i','n','e',0};
    static WCHAR value[] = {'v','a','l','u','e',0};
    static WCHAR empty[] = {0};
    static WCHAR defaultX[] = {'d','e','f','a','u','l','t',0};
    static WCHAR def[] = {'d','e','f',0};
    static WCHAR value0[] = {'v','a','l','u','e','0','1','2','3','4','5','6','7','8','9',0};
    static WCHAR testingvalue[] = {'t','e','s','t','i','n','g',0,'v','a','l','u','e',0};
    int ret;
    WCHAR buffer[256] = {0};
    LONG reg_ret;

    lstrcpyW(buffer, wine);
    ret = SQLGetPrivateProfileStringW(NULL, testing , defaultval, buffer, 256, odbcini);
    ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, wine), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    lstrcpyW(buffer, wine);
    ret = SQLGetPrivateProfileStringW(wineodbc, NULL , defaultval, buffer, 256, odbcini);
    ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, empty), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    lstrcpyW(buffer, value);
    ret = SQLGetPrivateProfileStringW(wineodbc, testing , NULL, buffer, 256, odbcini);
    ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, empty), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    ret = SQLGetPrivateProfileStringW(wineodbc, testing , defaultX, buffer, 256, odbcini);
    ok(ret == lstrlenW(defaultX), "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, defaultX), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    ret = SQLGetPrivateProfileStringW(wineodbc, testing , defaultX, buffer, 4, odbcini);
    ok(ret == lstrlenW(def), "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, def), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    ret = SQLGetPrivateProfileStringW(wineodbc, testing , defaultX, buffer, 8, odbcini);
    ok(ret == lstrlenW(defaultX), "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, defaultX), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    ret = SQLGetPrivateProfileStringW(wineodbc, testing , defaultX, NULL, 256, odbcini);
    ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);

    lstrcpyW(buffer, value);
    ret = SQLGetPrivateProfileStringW(wineodbc, testing , defaultX, buffer, 0, odbcini);
    ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, value), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    ret = SQLWritePrivateProfileString("wineodbc", "testing" , "value0123456789", "abcd.ini");
    ok(ret, "SQLWritePrivateProfileString failed\n");
    if(ret)
    {
        HKEY hkey;

        ret = SQLGetPrivateProfileStringW(wineodbc, testing , defaultX, buffer, 256, abcdini);
        ok(ret == lstrlenW(value0), "SQLGetPrivateProfileStringW returned %d\n", ret);
        ok(!lstrcmpW(buffer, value0), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

        ret = SQLGetPrivateProfileStringW(wineodbc, testing , defaultX, NULL, 0, abcdini);
        ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);

        ret = SQLGetPrivateProfileStringW(wineodbc, testing , defaultX, buffer, 7, abcdini);
        ok(ret == 6, "SQLGetPrivateProfileStringW returned %d\n", ret);

        lstrcpyW(buffer, wine);
        ret = SQLGetPrivateProfileStringW(wineodbc, NULL , empty, buffer, 10, abcdini);
        ok(ret == lstrlenW(testing)+1, "SQLGetPrivateProfileStringW returned %d\n", ret);
        ok(!lstrcmpW(buffer, testing), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

        ret = SQLWritePrivateProfileString("wineodbc", "value" , "0", "abcd.ini");
        ok(ret, "SQLWritePrivateProfileString failed\n");

        lstrcpyW(buffer, wine);
        ret = SQLGetPrivateProfileStringW(wineodbc, NULL , empty, buffer, 256, abcdini);
        ok(ret == (lstrlenW(testing) + lstrlenW(value)+2), "SQLGetPrivateProfileStringW returned %d\n", ret);
        if(ret == (lstrlenW(testing) + lstrlenW(value)+2))
        {
            ok(!memcmp(buffer, testingvalue, sizeof(testingvalue)),
                      "incorrect string '%s'\n", wine_dbgstr_wn(buffer, ret));
        }

        lstrcpyW(buffer, value);
        ret = SQLGetPrivateProfileStringW(wineodbc, NULL , empty, buffer, 10, abcdini);
        ok(ret == lstrlenW(testing)+1, "SQLGetPrivateProfileStringW returned %d\n", ret);
        if(ret >= lstrlenW(testing)+1)
        {
            ok(!lstrcmpW(buffer, testing), "incorrect string '%s'\n", wine_dbgstr_w(buffer));
        }

        lstrcpyW(buffer, value);
        ret = SQLGetPrivateProfileStringW(wineodbc, NULL , empty, buffer, 2, abcdini);
        ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);

        reg_ret = RegOpenKeyExW(HKEY_CURRENT_USER, abcd_key, 0, KEY_READ, &hkey);
        ok(reg_ret == ERROR_SUCCESS, "RegOpenKeyExW failed\n");
        if(reg_ret == ERROR_SUCCESS)
        {
            reg_ret = RegDeleteKeyW(HKEY_CURRENT_USER, abcd_key);
            ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed\n");

            RegCloseKey(hkey);
        }

        /* Cleanup key */
        reg_ret = RegDeleteKeyW(HKEY_CURRENT_USER, abcdini_key);
        ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed\n");
    }
}

static void test_SQLInstallDriverEx(void)
{
    char path[MAX_PATH];
    char syspath[MAX_PATH];
    WORD size = 0;
    BOOL ret, sql_ret;
    DWORD cnt, error_code = 0;
    HKEY hkey;
    LONG res;
    char error[1000];

    GetSystemDirectoryA(syspath, MAX_PATH);

    ret = SQLConfigDriver(NULL, ODBC_CONFIG_DRIVER, "WINE ODBC Driver", "CPTimeout=59", error, sizeof(error), NULL);
    ok(!ret, "SQLConfigDriver returned %d\n", ret);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret && error_code == ODBC_ERROR_COMPONENT_NOT_FOUND, "SQLConfigDriver returned %d, %u\n", sql_ret, error_code);

    ret = SQLInstallDriverEx("WINE ODBC Driver\0Driver=sample.dll\0Setup=sample.dll\0\0", NULL,
                             path, MAX_PATH, &size, ODBC_INSTALL_COMPLETE, NULL);
    ok(ret, "SQLInstallDriverEx failed\n");
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    if (sql_ret && error_code == ODBC_ERROR_WRITING_SYSINFO_FAILED)
    {
         win_skip("not enough privileges\n");
         return;
    }
    ok(sql_ret == SQL_NO_DATA || (sql_ret && error_code == SQL_SUCCESS), "SQLInstallDriverEx failed %d, %u\n", sql_ret, error_code);
    ok(!strcmp(path, syspath), "invalid path %s\n", path);

if (0)  /* Crashes on XP. */
{
    sql_ret = 0;
    ret = SQLConfigDriver(NULL, ODBC_CONFIG_DRIVER, "WINE ODBC Driver", NULL, error, sizeof(error), NULL);
    ok(!ret, "SQLConfigDriver failed '%s'\n",error);
}

    ret = SQLConfigDriver(NULL, ODBC_CONFIG_DRIVER, "WINE ODBC Driver", "CPTimeout=59\0NoWrite=60\0", error, sizeof(error), NULL);
    ok(ret, "SQLConfigDriver failed\n");
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret == SQL_NO_DATA || (sql_ret && error_code == SQL_SUCCESS), "SQLConfigDriver failed %d, %u\n", sql_ret, error_code);

    ret = SQLInstallDriverEx("WINE ODBC Driver Path\0Driver=sample.dll\0Setup=sample.dll\0\0", "c:\\temp", path, MAX_PATH, &size, ODBC_INSTALL_COMPLETE, NULL);
    ok(ret, "SQLInstallDriverEx failed\n");
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret == SQL_NO_DATA || (sql_ret && error_code == SQL_SUCCESS), "SQLInstallDriverEx failed %d, %u\n", sql_ret, error_code);
    ok(!strcmp(path, "c:\\temp"), "invalid path %s\n", path);

    ret = SQLConfigDriver(NULL, ODBC_CONFIG_DRIVER, "WINE ODBC Driver Path", "empty", error, sizeof(error), NULL);
    ok(!ret, "SQLConfigDriver successful\n");
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret && error_code == ODBC_ERROR_INVALID_KEYWORD_VALUE, "SQLConfigDriver failed %d, %u\n", sql_ret, error_code);

    ret = SQLConfigDriver(NULL, ODBC_CONFIG_DRIVER, "WINE ODBC Driver Path", "NoWrite=60;xxxx=555", error, sizeof(error), NULL);
    ok(ret, "SQLConfigDriver failed\n");
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret == SQL_NO_DATA || (sql_ret && error_code == SQL_SUCCESS), "SQLConfigDriver failed %d, %u\n", sql_ret, error_code);

    if (ret)
    {
        DWORD type = 0xdeadbeef, size = MAX_PATH;

        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\ODBC\\ODBCINST.INI\\WINE ODBC Driver", 0, KEY_READ, &hkey);
        ok(res == ERROR_SUCCESS, "RegOpenKeyExW failed\n");
        if (res == ERROR_SUCCESS)
        {
            char driverpath[MAX_PATH];

            strcpy(driverpath, syspath);
            strcat(driverpath, "\\sample.dll");

            memset(path, 0, sizeof(path));
            res = RegQueryValueExA(hkey, "Driver", NULL, &type, (BYTE *)path, &size);
            ok(res == ERROR_SUCCESS, "got %d\n", res);
            ok(type == REG_SZ, "got %u\n", type);
            ok(size == strlen(driverpath) + 1, "got %u\n", size);
            ok(!strcmp(path, driverpath), "invalid path %s\n", path);

            res = RegQueryValueExA(hkey, "CPTimeout", NULL, &type, (BYTE *)&path, &size);
            ok(res == ERROR_SUCCESS, "got %d\n", res);
            ok(type == REG_SZ, "got %u\n", type);
            ok(size == strlen("59") + 1, "got %u\n", size);
            ok(!strcmp(path, "59"), "invalid value %s\n", path);

            res = RegQueryValueExA(hkey, "NoWrite", NULL, &type, (BYTE *)&path, &size);
            ok(res == ERROR_FILE_NOT_FOUND, "got %d\n", res);

            RegCloseKey(hkey);
        }

        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\ODBC\\ODBCINST.INI\\WINE ODBC Driver Path", 0, KEY_READ, &hkey);
        ok(res == ERROR_SUCCESS, "RegOpenKeyExW failed\n");
        if (res == ERROR_SUCCESS)
        {
            size = sizeof(path);
            res = RegQueryValueExA(hkey, "NoWrite", NULL, &type, (BYTE *)&path, &size);
            ok(res == ERROR_SUCCESS, "got %d\n", res);
            ok(type == REG_SZ, "got %u\n", type);
            ok(size == strlen("60;xxxx=555") + 1, "got %u\n", size);
            ok(!strcmp(path, "60;xxxx=555"), "invalid value %s\n", path);

            res = RegQueryValueExA(hkey, "CPTimeout", NULL, &type, (BYTE *)&path, &size);
            ok(res == ERROR_FILE_NOT_FOUND, "got %d\n", res);
            RegCloseKey(hkey);
        }
    }

    cnt = 100;
    ret = SQLRemoveDriver("WINE ODBC Driver", FALSE, &cnt);
    ok(ret, "SQLRemoveDriver failed\n");
    ok(cnt == 0, "SQLRemoveDriver failed %d\n", cnt);

    cnt = 100;
    ret = SQLRemoveDriver("WINE ODBC Driver Path", FALSE, &cnt);
    ok(ret, "SQLRemoveDriver failed\n");
    ok(cnt == 0, "SQLRemoveDriver failed %d\n", cnt);
}

static void test_SQLInstallTranslatorEx(void)
{
    char path[MAX_PATH];
    char syspath[MAX_PATH];
    WORD size = 0;
    BOOL ret, sql_ret;
    DWORD cnt, error_code = 0;
    HKEY hkey;
    LONG res;

    GetSystemDirectoryA(syspath, MAX_PATH);

    ret = SQLInstallTranslatorEx("WINE ODBC Translator\0Translator=sample.dll\0Setup=sample.dll\0",
                                 NULL, path, MAX_PATH, &size, ODBC_INSTALL_COMPLETE, NULL);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    if (sql_ret && error_code == ODBC_ERROR_WRITING_SYSINFO_FAILED)
    {
         win_skip("not enough privileges\n");
         return;
    }
    ok(sql_ret && error_code == SQL_SUCCESS, "SQLInstallDriverEx failed %d, %u\n", sql_ret, error_code);
    ok(!strcmp(path, syspath), "invalid path %s\n", path);
    ok(size == strlen(path), "invalid length %d\n", size);

    ret = SQLInstallTranslatorEx("WINE ODBC Translator Path\0Translator=sample.dll\0Setup=sample.dll\0",
                                 "c:\\temp", path, MAX_PATH, &size, ODBC_INSTALL_COMPLETE, NULL);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret && error_code == SQL_SUCCESS, "SQLInstallTranslatorEx failed %d, %u\n", sql_ret, error_code);
    ok(!strcmp(path, "c:\\temp"), "invalid path %s\n", path);
    ok(size == strlen(path), "invalid length %d\n", size);

    if(ret)
    {
        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\ODBC\\ODBCINST.INI\\WINE ODBC Translator", 0,
                            KEY_READ, &hkey);
        ok(res == ERROR_SUCCESS, "RegOpenKeyExW failed\n");
        if (res == ERROR_SUCCESS)
        {
            DWORD type = 0xdeadbeef, size = MAX_PATH;
            char driverpath[MAX_PATH];

            strcpy(driverpath, syspath);
            strcat(driverpath, "\\sample.dll");

            memset(path, 0, sizeof(path));
            res = RegQueryValueExA(hkey, "Translator", NULL, &type, (BYTE *)path, &size);
            ok(res == ERROR_SUCCESS, "RegGetValueA failed\n");
            ok(type == REG_SZ, "got %u\n", type);
            ok(size == strlen(driverpath) + 1, "got %u\n", size);
            ok(!strcmp(path, driverpath), "invalid path %s\n", path);

            RegCloseKey(hkey);
        }
    }

    cnt = 100;
    ret = SQLRemoveTranslator("WINE ODBC Translator", &cnt);
    ok(ret, "SQLRemoveTranslator failed\n");
    ok(cnt == 0, "SQLRemoveTranslator failed %d\n", cnt);

    cnt = 100;
    ret = SQLRemoveTranslator("WINE ODBC Translator Path", &cnt);
    ok(ret, "SQLRemoveTranslator failed\n");
    ok(cnt == 0, "SQLRemoveTranslator failed %d\n", cnt);

    cnt = 100;
    ret = SQLRemoveTranslator("WINE ODBC Translator NonExist", &cnt);
    ok(!ret, "SQLRemoveTranslator succeeded\n");
    ok(cnt == 100, "SQLRemoveTranslator succeeded %d\n", cnt);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret && error_code == ODBC_ERROR_COMPONENT_NOT_FOUND,
        "SQLInstallTranslatorEx failed %d, %u\n", sql_ret, error_code);

}

static void test_SQLGetInstalledDrivers(void)
{
    char buffer[1000], *p;
    WORD written, len;
    BOOL ret, sql_ret;
    DWORD error_code;
    int found = 0;

    ret = SQLInstallDriverEx("Wine test\0Driver=test.dll\0\0", NULL, buffer,
        sizeof(buffer), &written, ODBC_INSTALL_COMPLETE, NULL);
    ok(ret, "SQLInstallDriverEx failed: %d\n", ret);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    if (sql_ret && error_code == ODBC_ERROR_WRITING_SYSINFO_FAILED)
    {
        skip("not enough privileges\n");
        return;
    }

    ret = SQLGetInstalledDrivers(NULL, sizeof(buffer), &written);
    ok(!ret, "got %d\n", ret);
    check_error(ODBC_ERROR_INVALID_BUFF_LEN);

    ret = SQLGetInstalledDrivers(buffer, 0, &written);
    ok(!ret, "got %d\n", ret);
    check_error(ODBC_ERROR_INVALID_BUFF_LEN);

    ret = SQLGetInstalledDrivers(buffer, 10, &written);
    ok(ret, "got %d\n", ret);
    ok(strlen(buffer) == 8, "got len %u\n", lstrlenA(buffer));
    ok(written == 10, "got written %d\n", written);
    ok(!buffer[9], "buffer not doubly null-terminated\n");

    ret = SQLGetInstalledDrivers(buffer, sizeof(buffer), &written);
    ok(ret, "got %d\n", ret);
    ok(!buffer[written-1] && !buffer[written-2], "buffer not doubly null-terminated\n");
    len = strlen(buffer);

    for (p = buffer; *p; p += strlen(p) + 1)
    {
        if (!strcmp(p, "Wine test"))
            found = 1;
    }
    ok(found, "installed driver not found\n");

    ret = SQLGetInstalledDrivers(buffer, len, &written);
    ok(ret, "got %d\n", ret);
    ok(strlen(buffer) == len-2, "expected len %d, got %u\n", len-2, lstrlenA(buffer));
    ok(written == len, "expected written %d, got %d\n", len, written);
    ok(!buffer[len-1], "buffer not doubly null-terminated\n");

    ret = SQLGetInstalledDrivers(buffer, len+1, &written);
    ok(ret, "got %d\n", ret);
    ok(strlen(buffer) == len-1, "expected len %d, got %u\n", len-1, lstrlenA(buffer));
    ok(written == len+1, "expected written %d, got %d\n", len+1, written);
    ok(!buffer[len], "buffer not doubly null-terminated\n");

    ret = SQLGetInstalledDrivers(buffer, len+2, &written);
    ok(ret, "got %d\n", ret);
    ok(strlen(buffer) == len, "expected len %d, got %u\n", len, lstrlenA(buffer));
    ok(written == len+2, "expected written %d, got %d\n", len+2, written);
    ok(!buffer[len+1], "buffer not doubly null-terminated\n");

    SQLRemoveDriver("Wine test", TRUE, NULL);
}

static void test_SQLValidDSN(void)
{
    static const char *invalid = "[]{}(),;?*=!@\\";
    char str[10];
    int i;
    BOOL ret;

    strcpy(str, "wine10");
    for(i = 0; i < strlen(invalid); i++)
    {
        str[4] = invalid[i];
        ret = SQLValidDSN(str);
        ok(!ret, "got %d\n", ret);
    }

    /* Too large */
    ret = SQLValidDSN("Wine123456789012345678901234567890");
    ok(!ret, "got %d\n", ret);

    /* Valid with a space */
    ret = SQLValidDSN("Wine Vinegar");
    ok(ret, "got %d\n", ret);

    /* Max DSN name value */
    ret = SQLValidDSN("12345678901234567890123456789012");
    ok(ret, "got %d\n", ret);
}

static void test_SQLValidDSNW(void)
{
    static const WCHAR invalid[] = {'[',']','{','}','(',')',',',';','?','*','=','!','@','\\',0};
    static const WCHAR value[] = { 'w','i','n','e','1','0',0};
    static const WCHAR too_large[] = { 'W','i','n','e','1','2','3','4','5','6','7','8','9','0','1','2','3','4','5',
                                   '6','7','8','9','0','1','2','3','4','5','6','7','8','9','0', 0};
    static const WCHAR with_space[] = { 'W','i','n','e',' ','V','i','n','e','g','a','r', 0};
    static const WCHAR max_dsn[] = { '1','2','3','4','5','6','7','8','9','0','1','2','3','4','5','6','7','8','9','0',
                                   '1','2','3','4','5','6','7','8','9','0','1','2', 0};
    WCHAR str[10];
    int i;
    BOOL ret;

    lstrcpyW(str, value);
    for(i = 0; i < lstrlenW(invalid); i++)
    {
        str[4] = invalid[i];
        ret = SQLValidDSNW(str);
        ok(!ret, "got %d\n", ret);
    }

    ret = SQLValidDSNW(too_large);
    ok(!ret, "got %d\n", ret);

    ret = SQLValidDSNW(with_space);
    ok(ret, "got %d\n", ret);

    ret = SQLValidDSNW(max_dsn);
    ok(ret, "got %d\n", ret);
}

static void test_SQLConfigDataSource(void)
{
    BOOL ret;

    ret = SQLConfigDataSource(0, ODBC_ADD_DSN, "SQL Server", "DSN=WINEMQIS\0Database=MQIS\0\0");
    ok(ret, "got %d\n", ret);

    ret = SQLConfigDataSource(0, ODBC_REMOVE_DSN, "SQL Server", "DSN=WINEMQIS\0\0");
    ok(ret, "got %d\n", ret);

    ret = SQLConfigDataSource(0, ODBC_REMOVE_DSN, "SQL Server", "DSN=WINEMQIS\0\0");
    if(!ret)
    {
        RETCODE ret;
        DWORD err;
        ret = SQLInstallerError(1, &err, NULL, 0, NULL);
        ok(ret == SQL_SUCCESS_WITH_INFO, "got %d\n", ret);
        todo_wine ok(err == ODBC_ERROR_INVALID_DSN, "got %u\n", err);
    }

    ret = SQLConfigDataSource(0, ODBC_ADD_DSN, "ODBC driver", "DSN=ODBC data source\0\0");
    todo_wine ok(!ret, "got %d\n", ret);
    todo_wine check_error(ODBC_ERROR_COMPONENT_NOT_FOUND);
}

START_TEST(misc)
{
    test_SQLConfigMode();
    test_SQLInstallerError();
    test_SQLInstallDriverManager();
    test_SQLWritePrivateProfileString();
    test_SQLGetPrivateProfileString();
    test_SQLGetPrivateProfileStringW();
    test_SQLInstallDriverEx();
    test_SQLInstallTranslatorEx();
    test_SQLGetInstalledDrivers();
    test_SQLValidDSN();
    test_SQLValidDSNW();
    test_SQLConfigDataSource();
}
