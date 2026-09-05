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

static const WCHAR abcd_key[] = L"Software\\ODBC\\abcd.INI\\wineodbc";
static const WCHAR abcdini_key[] = L"Software\\ODBC\\abcd.INI";

static void check_error_(int line, DWORD expect)
{
    RETCODE ret;
    DWORD err;
    ret = SQLInstallerError(1, &err, NULL, 0, NULL);
    ok_(__FILE__, line)(ret == SQL_SUCCESS_WITH_INFO, "got %d\n", ret);
    ok_(__FILE__, line)(err == expect, "expected %lu, got %u\n", expect, ret);
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
    ok(bool_ret, "SQLInstallDriverManager unexpectedly failed: %ld\n",
        error_code);
    if (bool_ret)
        ok(sql_ret == SQL_NO_DATA, "Expected SQL_NO_DATA, got %d\n", sql_ret);
    else
        ok(sql_ret == SQL_SUCCESS_WITH_INFO,
            "Expected SQL_SUCCESS_WITH_INFO, got %d\n", sql_ret);

    path_out = 0xcafe;
    bool_ret = SQLInstallDriverManager(target_path, MAX_PATH, &path_out);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(bool_ret, "SQLInstallDriverManager unexpectedly failed: %ld\n",
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
   static const WCHAR odbc_key[] = L"Software\\ODBC\\ODBC.INI\\wineodbc";
   BOOL ret;
   LONG reg_ret;
   DWORD error_code;

   ret = SQLWritePrivateProfileString("wineodbc", "testing" , "value", "");
   ok(!ret, "SQLWritePrivateProfileString passed\n");
   SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
   ok(error_code == ODBC_ERROR_INVALID_STR, "SQLInstallerErrorW ret: %ld\n", error_code);

   ret = SQLWritePrivateProfileString("wineodbc", "testing" , "value", NULL);
   ok(!ret, "SQLWritePrivateProfileString passed\n");
   SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
   ok(error_code == ODBC_ERROR_INVALID_STR, "SQLInstallerErrorW ret: %ld\n", error_code);

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
    UWORD orig_mode;
    int ret;
    WCHAR buffer[256] = {0};
    LONG reg_ret;

    lstrcpyW(buffer, L"wine");
    ret = SQLGetPrivateProfileStringW(NULL, L"testing", L"default", buffer, 256, L"ODBC.INI");
    ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, L"wine"), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    lstrcpyW(buffer, L"wine");
    ret = SQLGetPrivateProfileStringW(L"wineodbc", NULL , L"default", buffer, 256, L"ODBC.INI");
    ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, L""), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    lstrcpyW(buffer, L"value");
    ret = SQLGetPrivateProfileStringW(L"wineodbc", L"testing", NULL, buffer, 256, L"ODBC.INI");
    ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, L""), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    ret = SQLGetPrivateProfileStringW(L"wineodbc", L"testing", L"default", buffer, 256, L"ODBC.INI");
    ok(ret == lstrlenW(L"default"), "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, L"default"), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    ret = SQLGetPrivateProfileStringW(L"wineodbc", L"testing", L"default", buffer, 4, L"ODBC.INI");
    ok(ret == lstrlenW(L"def"), "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, L"def"), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    ret = SQLGetPrivateProfileStringW(L"wineodbc", L"testing", L"default", buffer, 8, L"ODBC.INI");
    ok(ret == lstrlenW(L"default"), "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, L"default"), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    ret = SQLGetPrivateProfileStringW(L"wineodbc", L"testing", L"default", NULL, 256, L"ODBC.INI");
    ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);

    lstrcpyW(buffer, L"value");
    ret = SQLGetPrivateProfileStringW(L"wineodbc", L"testing", L"default", buffer, 0, L"ODBC.INI");
    ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);
    ok(!lstrcmpW(buffer, L"value"), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

    ret = SQLWritePrivateProfileString("wineodbc", "testing" , "value0123456789", "abcd.ini");
    ok(ret, "SQLWritePrivateProfileString failed\n");
    if(ret)
    {
        HKEY hkey;

        ret = SQLGetPrivateProfileStringW(L"wineodbc", L"testing", L"default", buffer, 256, L"abcd.INI");
        ok(ret == lstrlenW(L"value0123456789"), "SQLGetPrivateProfileStringW returned %d\n", ret);
        ok(!lstrcmpW(buffer, L"value0123456789"), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

        ret = SQLGetPrivateProfileStringW(L"wineodbc", L"testing", L"default", NULL, 0, L"abcd.INI");
        ok(ret == 0, "SQLGetPrivateProfileStringW returned %d\n", ret);

        ret = SQLGetPrivateProfileStringW(L"wineodbc", L"testing", L"default", buffer, 7, L"abcd.INI");
        ok(ret == 6, "SQLGetPrivateProfileStringW returned %d\n", ret);

        lstrcpyW(buffer, L"wine");
        ret = SQLGetPrivateProfileStringW(L"wineodbc", NULL , L"", buffer, 10, L"abcd.INI");
        ok(ret == lstrlenW(L"testing") + 1, "SQLGetPrivateProfileStringW returned %d\n", ret);
        ok(!lstrcmpW(buffer, L"testing"), "incorrect string '%s'\n", wine_dbgstr_w(buffer));

        ret = SQLWritePrivateProfileString("wineodbc", "value" , "0", "abcd.ini");
        ok(ret, "SQLWritePrivateProfileString failed\n");

        lstrcpyW(buffer, L"wine");
        ret = SQLGetPrivateProfileStringW(L"wineodbc", NULL , L"", buffer, 256, L"abcd.INI");
        ok(ret == (lstrlenW(L"testing") + lstrlenW(L"value") + 2), "SQLGetPrivateProfileStringW returned %d\n", ret);
        if(ret == (lstrlenW(L"testing") + lstrlenW(L"value") + 2))
        {
            ok(!memcmp(buffer, L"testing\0value", sizeof(L"testing\0value")),
                      "incorrect string '%s'\n", wine_dbgstr_wn(buffer, ret));
        }

        lstrcpyW(buffer, L"value");
        ret = SQLGetPrivateProfileStringW(L"wineodbc", NULL , L"", buffer, 10, L"abcd.INI");
        ok(ret == lstrlenW(L"testing") + 1, "SQLGetPrivateProfileStringW returned %d\n", ret);
        if(ret >= lstrlenW(L"testing") + 1)
        {
            ok(!lstrcmpW(buffer, L"testing"), "incorrect string '%s'\n", wine_dbgstr_w(buffer));
        }

        lstrcpyW(buffer, L"value");
        ret = SQLGetPrivateProfileStringW(L"wineodbc", NULL , L"", buffer, 2, L"abcd.INI");
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

    ret = SQLGetConfigMode(&orig_mode);
    ok(ret, "SQLGetConfigMode failed\n");

    ret = SQLSetConfigMode(ODBC_SYSTEM_DSN);
    ok(ret, "SQLSetConfigMode failed\n");

    ret = SQLWritePrivateProfileStringW(L"wineodbc", L"testing" , L"value", L"ODBC.INI");
    if (!ret)
    {
        DWORD error_code;
        ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
        if (ret && error_code == ODBC_ERROR_WRITING_SYSINFO_FAILED)
        {
            win_skip("not enough privileges\n");
            SQLSetConfigMode(orig_mode);
            return;
        }
    }
    ok(ret, "SQLWritePrivateProfileString failed\n");

    ret = SQLWritePrivateProfileStringW(L"wineodbc1", L"testing" , L"systemdsn", L"ODBC.INI");
    ok(ret, "SQLWritePrivateProfileString failed\n");

    ret = SQLGetPrivateProfileStringW(L"wineodbc", NULL, L"", buffer, 256, L"ODBC.INI");
    ok(ret, "SQLGetPrivateProfileStringW failed\n");

    ret = SQLSetConfigMode(ODBC_USER_DSN);
    ok(ret, "SQLSetConfigMode failed\n");

    ret = SQLGetPrivateProfileStringW(L"wineodbc", NULL, L"", buffer, 256, L"ODBC.INI");
    ok(!ret, "SQLGetPrivateProfileStringW succeeded\n");

    ret = SQLWritePrivateProfileStringW(L"wineodbc1", L"testing" , L"userdsn", L"ODBC.INI");
    ok(ret, "SQLWritePrivateProfileString failed\n");

    ret = SQLSetConfigMode(ODBC_BOTH_DSN);
    ok(ret, "SQLSetConfigMode failed\n");

    ret = SQLGetPrivateProfileStringW(L"wineodbc", NULL, L"", buffer, 256, L"ODBC.INI");
    ok(ret, "SQLGetPrivateProfileStringW failed\n");

    reg_ret = RegDeleteKeyW(HKEY_LOCAL_MACHINE, L"Software\\ODBC\\ODBC.INI\\wineodbc");
    ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed %ld\n", reg_ret);

    /* Show existing USER DSN is checked before MACHINE */
    ret = SQLWritePrivateProfileStringW(L"wineodbc1", L"testing" , L"bothdsn", L"ODBC.INI");
    ok(ret, "SQLWritePrivateProfileString failed\n");

    ret = SQLSetConfigMode(ODBC_SYSTEM_DSN);
    ok(ret, "SQLSetConfigMode failed\n");

    ret = SQLGetPrivateProfileStringW(L"wineodbc1", L"testing", L"", buffer, 256, L"ODBC.INI");
    ok(ret, "SQLGetPrivateProfileStringW failed\n");
    ok(!wcscmp(buffer, L"systemdsn"), "Wrong value\n");

    reg_ret = RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\ODBC\\ODBC.INI\\wineodbc1");
    ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed %ld\n", reg_ret);

    reg_ret = RegDeleteKeyW(HKEY_LOCAL_MACHINE, L"Software\\ODBC\\ODBC.INI\\wineodbc1");
    ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed %ld\n", reg_ret);

    ret = SQLSetConfigMode(ODBC_BOTH_DSN);
    ok(ret, "SQLSetConfigMode failed\n");

    /* Writes to USER if no key found */
    ret = SQLWritePrivateProfileStringW(L"wineodbc1", L"testing" , L"userwrite", L"ODBC.INI");
    ok(ret, "SQLWritePrivateProfileString failed\n");

    ret = SQLSetConfigMode(ODBC_USER_DSN);
    ok(ret, "SQLSetConfigMode failed\n");

    ret = SQLGetPrivateProfileStringW(L"wineodbc1", L"testing", L"", buffer, 256, L"ODBC.INI");
    ok(ret, "SQLGetPrivateProfileStringW failed\n");
    ok(!wcscmp(buffer, L"userwrite"), "Wrong value\n");

    reg_ret = RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\ODBC\\ODBC.INI\\wineodbc1");
    ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed %ld\n", reg_ret);

    ret = SQLSetConfigMode(ODBC_USER_DSN);
    ok(ret, "SQLSetConfigMode failed\n");

    ret = SQLWritePrivateProfileStringW(L"wineodbc", L"testing" , L"value", L"ODBC.INI");
    ok(ret, "SQLWritePrivateProfileString failed\n");

    ret = SQLGetPrivateProfileStringW(L"wineodbc", NULL, L"", buffer, 256, L"ODBC.INI");
    ok(ret, "SQLGetPrivateProfileStringW failed\n");

    ret = SQLWritePrivateProfileStringW(L"wineodbc", L"testing" , L"value", L"ODBCINST.INI");
    ok(ret, "SQLWritePrivateProfileString failed\n");

    ret = SQLGetPrivateProfileStringW(L"wineodbc", NULL, L"", buffer, 256, L"ODBCINST.INI");
    ok(ret, "SQLGetPrivateProfileStringW failed\n");

    reg_ret = RegDeleteKeyW(HKEY_LOCAL_MACHINE, L"Software\\ODBC\\ODBCINST.INI\\wineodbc");
    ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed %ld\n", reg_ret);

    ret = SQLSetConfigMode(ODBC_SYSTEM_DSN);
    ok(ret, "SQLSetConfigMode failed\n");

    ret = SQLGetPrivateProfileStringW(L"wineodbc", NULL, L"", buffer, 256, L"ODBC.INI");
    ok(!ret, "SQLGetPrivateProfileStringW succeeded\n");

    ret = SQLSetConfigMode(ODBC_BOTH_DSN);
    ok(ret, "SQLSetConfigMode failed\n");

    ret = SQLGetPrivateProfileStringW(L"wineodbc", NULL, L"", buffer, 256, L"ODBC.INI");
    ok(ret, "SQLGetPrivateProfileStringW failed\n");

    reg_ret = RegDeleteKeyW(HKEY_CURRENT_USER, L"Software\\ODBC\\ODBC.INI\\wineodbc");
    ok(reg_ret == ERROR_SUCCESS, "RegDeleteKeyW failed %ld\n", reg_ret);

    ret = SQLSetConfigMode(orig_mode);
    ok(ret, "SQLSetConfigMode failed\n");
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
    ok(sql_ret && error_code == ODBC_ERROR_COMPONENT_NOT_FOUND, "SQLConfigDriver returned %d, %lu\n", sql_ret, error_code);

    ret = SQLInstallDriverEx("WINE ODBC Driver\0Driver=sample.dll\0Setup=sample.dll\0\0", NULL,
                             path, MAX_PATH, &size, ODBC_INSTALL_COMPLETE, NULL);
    ok(ret, "SQLInstallDriverEx failed\n");
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    if (sql_ret && error_code == ODBC_ERROR_WRITING_SYSINFO_FAILED)
    {
         win_skip("not enough privileges\n");
         return;
    }
    ok(sql_ret == SQL_NO_DATA || (sql_ret && error_code == SQL_SUCCESS), "SQLInstallDriverEx failed %d, %lu\n", sql_ret, error_code);
    ok(!strcmp(path, syspath), "invalid path %s\n", path);

    sql_ret = 0;
    ret = SQLConfigDriver(NULL, ODBC_CONFIG_DRIVER, "WINE ODBC Driver", NULL, error, sizeof(error), NULL);
    ok(!ret, "SQLConfigDriver failed '%s'\n",error);

    ret = SQLConfigDriver(NULL, ODBC_CONFIG_DRIVER, "WINE ODBC Driver", "CPTimeout=59\0NoWrite=60\0", error, sizeof(error), NULL);
    ok(ret, "SQLConfigDriver failed\n");
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret == SQL_NO_DATA || (sql_ret && error_code == SQL_SUCCESS), "SQLConfigDriver failed %d, %lu\n", sql_ret, error_code);

    ret = SQLInstallDriverEx("WINE ODBC Driver Path\0Driver=sample.dll\0Setup=sample.dll\0\0", "c:\\temp", path, MAX_PATH, &size, ODBC_INSTALL_COMPLETE, NULL);
    ok(ret, "SQLInstallDriverEx failed\n");
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret == SQL_NO_DATA || (sql_ret && error_code == SQL_SUCCESS), "SQLInstallDriverEx failed %d, %lu\n", sql_ret, error_code);
    ok(!strcmp(path, "c:\\temp"), "invalid path %s\n", path);

    ret = SQLConfigDriver(NULL, ODBC_CONFIG_DRIVER, "WINE ODBC Driver Path", "empty", error, sizeof(error), NULL);
    ok(!ret, "SQLConfigDriver successful\n");
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret && error_code == ODBC_ERROR_INVALID_KEYWORD_VALUE, "SQLConfigDriver failed %d, %lu\n", sql_ret, error_code);

    ret = SQLConfigDriver(NULL, ODBC_CONFIG_DRIVER, "WINE ODBC Driver Path", "NoWrite=60;xxxx=555", error, sizeof(error), NULL);
    ok(ret, "SQLConfigDriver failed\n");
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret == SQL_NO_DATA || (sql_ret && error_code == SQL_SUCCESS), "SQLConfigDriver failed %d, %lu\n", sql_ret, error_code);

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
            ok(res == ERROR_SUCCESS, "got %ld\n", res);
            ok(type == REG_SZ, "got %lu\n", type);
            ok(size == strlen(driverpath) + 1, "got %lu\n", size);
            ok(!strcmp(path, driverpath), "invalid path %s\n", path);

            res = RegQueryValueExA(hkey, "CPTimeout", NULL, &type, (BYTE *)&path, &size);
            ok(res == ERROR_SUCCESS, "got %ld\n", res);
            ok(type == REG_SZ, "got %lu\n", type);
            ok(size == strlen("59") + 1, "got %lu\n", size);
            ok(!strcmp(path, "59"), "invalid value %s\n", path);

            res = RegQueryValueExA(hkey, "NoWrite", NULL, &type, (BYTE *)&path, &size);
            ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

            RegCloseKey(hkey);
        }

        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\ODBC\\ODBCINST.INI\\WINE ODBC Driver Path", 0, KEY_READ, &hkey);
        ok(res == ERROR_SUCCESS, "RegOpenKeyExW failed\n");
        if (res == ERROR_SUCCESS)
        {
            size = sizeof(path);
            res = RegQueryValueExA(hkey, "NoWrite", NULL, &type, (BYTE *)&path, &size);
            ok(res == ERROR_SUCCESS, "got %ld\n", res);
            ok(type == REG_SZ, "got %lu\n", type);
            ok(size == strlen("60;xxxx=555") + 1, "got %lu\n", size);
            ok(!strcmp(path, "60;xxxx=555"), "invalid value %s\n", path);

            res = RegQueryValueExA(hkey, "CPTimeout", NULL, &type, (BYTE *)&path, &size);
            ok(res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
            RegCloseKey(hkey);
        }
    }

    cnt = 100;
    ret = SQLRemoveDriver("WINE ODBC Driver", FALSE, &cnt);
    ok(ret, "SQLRemoveDriver failed\n");
    ok(cnt == 0, "SQLRemoveDriver failed %ld\n", cnt);

    cnt = 100;
    ret = SQLRemoveDriver("WINE ODBC Driver Path", FALSE, &cnt);
    ok(ret, "SQLRemoveDriver failed\n");
    ok(cnt == 0, "SQLRemoveDriver failed %ld\n", cnt);
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
    ok(sql_ret && error_code == SQL_SUCCESS, "SQLInstallDriverEx failed %d, %lu\n", sql_ret, error_code);
    ok(!strcmp(path, syspath), "invalid path %s\n", path);
    ok(size == strlen(path), "invalid length %d\n", size);

    ret = SQLInstallTranslatorEx("WINE ODBC Translator Path\0Translator=sample.dll\0Setup=sample.dll\0",
                                 "c:\\temp", path, MAX_PATH, &size, ODBC_INSTALL_COMPLETE, NULL);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret && error_code == SQL_SUCCESS, "SQLInstallTranslatorEx failed %d, %lu\n", sql_ret, error_code);
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
            ok(type == REG_SZ, "got %lu\n", type);
            ok(size == strlen(driverpath) + 1, "got %lu\n", size);
            ok(!strcmp(path, driverpath), "invalid path %s\n", path);

            RegCloseKey(hkey);
        }
    }

    cnt = 100;
    ret = SQLRemoveTranslator("WINE ODBC Translator", &cnt);
    ok(ret, "SQLRemoveTranslator failed\n");
    ok(cnt == 0, "SQLRemoveTranslator failed %ld\n", cnt);

    cnt = 100;
    ret = SQLRemoveTranslator("WINE ODBC Translator Path", &cnt);
    ok(ret, "SQLRemoveTranslator failed\n");
    ok(cnt == 0, "SQLRemoveTranslator failed %ld\n", cnt);

    cnt = 100;
    ret = SQLRemoveTranslator("WINE ODBC Translator NonExist", &cnt);
    ok(!ret, "SQLRemoveTranslator succeeded\n");
    ok(cnt == 100, "SQLRemoveTranslator succeeded %ld\n", cnt);
    sql_ret = SQLInstallerErrorW(1, &error_code, NULL, 0, NULL);
    ok(sql_ret && error_code == ODBC_ERROR_COMPONENT_NOT_FOUND,
        "SQLInstallTranslatorEx failed %d, %lu\n", sql_ret, error_code);

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

    ret = SQLGetInstalledDrivers(buffer, 10, NULL);
    ok(ret, "got %d\n", ret);
    ok(strlen(buffer) == 8, "got len %u\n", lstrlenA(buffer));
    ok(!buffer[9], "buffer not doubly null-terminated\n");

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

    ret = SQLValidDSN(NULL);
    ok(!ret, "got %d\n", ret);

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

    ret = SQLValidDSN("");
    ok(!ret, "got %d\n", ret);
}

static void test_SQLValidDSNW(void)
{
    WCHAR str[10];
    int i;
    BOOL ret;

    ret = SQLValidDSNW(NULL);
    ok(!ret, "got %d\n", ret);

    lstrcpyW(str, L"wine10");
    for (i = 0; i < lstrlenW(L"[]{}(),;?*=!@\\"); i++)
    {
        str[4] = L"[]{}(),;?*=!@\\"[i];
        ret = SQLValidDSNW(str);
        ok(!ret, "got %d\n", ret);
    }

    ret = SQLValidDSNW(L"Wine123456789012345678901234567890");
    ok(!ret, "got %d\n", ret);

    ret = SQLValidDSNW(L"Wine Vinegar");
    ok(ret, "got %d\n", ret);

    ret = SQLValidDSNW(L"12345678901234567890123456789012");
    ok(ret, "got %d\n", ret);

    ret = SQLValidDSNW(L"");
    ok(!ret, "got %d\n", ret);
}

static void test_SQLConfigDataSource(void)
{
    BOOL ret;

    ret = SQLConfigDataSource(0, ODBC_ADD_DSN, "SQL Server", "DSN=WINEMQIS\0Database=MQIS\0\0");
    todo_wine ok(ret, "got %d\n", ret);

    ret = SQLConfigDataSource(0, ODBC_REMOVE_DSN, "SQL Server", "DSN=WINEMQIS\0\0");
    todo_wine ok(ret, "got %d\n", ret);

    ret = SQLConfigDataSource(0, ODBC_REMOVE_DSN, "SQL Server", "DSN=WINEMQIS\0\0");
    if(!ret)
    {
        RETCODE ret;
        DWORD err;
        ret = SQLInstallerError(1, &err, NULL, 0, NULL);
        ok(ret == SQL_SUCCESS_WITH_INFO, "got %d\n", ret);
        todo_wine ok(err == ODBC_ERROR_INVALID_DSN, "got %lu\n", err);
    }

    ret = SQLConfigDataSource(0, ODBC_ADD_DSN, "ODBC driver", "DSN=ODBC data source\0\0");
    ok(!ret, "got %d\n", ret);
    check_error(ODBC_ERROR_COMPONENT_NOT_FOUND);
}

static BOOL check_dsn_exists(HKEY key, const WCHAR *dsn)
{
    HKEY hkey;
    WCHAR buffer[256];
    LONG res;

    wcscpy(buffer, L"Software\\ODBC\\ODBC.INI\\");
    wcscat(buffer, dsn);

    res = RegOpenKeyExW(key, buffer, 0, KEY_READ, &hkey);
    if (!res)
        RegCloseKey(hkey);

    return res == ERROR_SUCCESS;
}

static void test_SQLWriteDSNToIni(void)
{
    BOOL ret;
    char buffer[MAX_PATH];
    char path[MAX_PATH];
    DWORD type, size;

    SQLSetConfigMode(ODBC_SYSTEM_DSN);

    ret = SQLRemoveDSNFromIni("");
    ok(!ret, "got %d\n", ret);

    ret = SQLRemoveDSNFromIniW(L"");
    ok(!ret, "got %d\n", ret);

    ret = check_dsn_exists(HKEY_LOCAL_MACHINE, L"wine_dbs");
    ok(!ret, "Found registry key\n");

    ret = SQLWriteDSNToIni("wine_dbs", "SQL Server");
    if (!ret)
    {
        win_skip("Doesn't have permission to write a System DSN\n");
        return;
    }

    ret = check_dsn_exists(HKEY_LOCAL_MACHINE, L"wine_dbs");
    ok(ret, "Failed to find registry key\n");

    SQLSetConfigMode(ODBC_USER_DSN);
    ret = SQLWriteDSNToIni("wine_dbs", "SQL Server");
    ok(ret, "got %d\n", ret);

    ret = check_dsn_exists(HKEY_CURRENT_USER, L"wine_dbs");
    ok(ret, "Failed to find registry key\n");

    SQLSetConfigMode(ODBC_SYSTEM_DSN);

    if(ret)
    {
        HKEY hkey;
        LONG res;

        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\ODBC\\ODBC.INI\\ODBC Data Sources", 0,
                            KEY_READ, &hkey);
        ok(res == ERROR_SUCCESS, "RegOpenKeyExW failed\n");
        if (res == ERROR_SUCCESS)
        {
            type = 0xdeadbeef;
            size = MAX_PATH;

            memset(buffer, 0, sizeof(buffer));
            res = RegQueryValueExA(hkey, "wine_dbs", NULL, &type, (BYTE *)buffer, &size);
            ok(res == ERROR_SUCCESS, "RegGetValueA failed\n");
            ok(type == REG_SZ, "got %lu\n", type);
            ok(!strcmp(buffer, "SQL Server"), "incorrect string '%s'\n", buffer);

            RegCloseKey(hkey);
        }

        SQLSetConfigMode(ODBC_BOTH_DSN);

        /* ODBC_BOTH_DSN set and has both System/User DSN but only removes USER. */
        ret = SQLRemoveDSNFromIni("wine_dbs");
        ok(ret, "got %d\n", ret);

        ret = check_dsn_exists(HKEY_CURRENT_USER, L"wine_dbs");
        ok(!ret, "Found registry key\n");

        ret = check_dsn_exists(HKEY_LOCAL_MACHINE, L"wine_dbs");
        ok(ret, "Failed to find registry key\n");

        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\ODBC\\ODBC.INI\\wine_dbs", 0,
                            KEY_READ, &hkey);
        ok(res == ERROR_SUCCESS, "RegOpenKeyExW failed\n");
        if (res == ERROR_SUCCESS)
        {
            type = 0xdeadbeef;
            size = MAX_PATH;

            memset(path, 0, sizeof(path));
            res = RegQueryValueExA(hkey, "driver", NULL, &type, (BYTE *)path, &size);
            ok(res == ERROR_SUCCESS, "RegGetValueA failed\n");
            ok(type == REG_SZ, "got %lu\n", type);
            /* WINE doesn't have a 'SQL Server' driver available */
            todo_wine ok(strlen(path) != 0, "Invalid value\n");

            RegCloseKey(hkey);
        }

        SQLSetConfigMode(ODBC_SYSTEM_DSN);

        ret = SQLRemoveDSNFromIni("wine_dbs");
        ok(ret, "got %d\n", ret);
    }

    /* Show that values are written, even though an invalid driver was specified. */
    ret = SQLWriteDSNToIni("wine_mis", "Missing Access Driver (*.mis)");
    ok(ret, "got %d\n", ret);
    if(ret)
    {
        HKEY hkey;
        LONG res;

        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\ODBC\\ODBC.INI\\ODBC Data Sources", 0,
                            KEY_READ, &hkey);
        ok(res == ERROR_SUCCESS, "RegOpenKeyExW failed\n");
        if (res == ERROR_SUCCESS)
        {
            type = 0xdeadbeef;
            size = MAX_PATH;

            memset(buffer, 0, sizeof(buffer));
            res = RegQueryValueExA(hkey, "wine_mis", NULL, &type, (BYTE *)buffer, &size);
            ok(res == ERROR_SUCCESS, "RegGetValueA failed\n");
            ok(type == REG_SZ, "got %lu\n", type);
            ok(!strcmp(buffer, "Missing Access Driver (*.mis)"), "incorrect string '%s'\n", buffer);

            RegCloseKey(hkey);
        }

        res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, "Software\\ODBC\\ODBC.INI\\wine_mis", 0,
                            KEY_READ, &hkey);
        ok(res == ERROR_SUCCESS, "RegOpenKeyExW failed\n");
        if (res == ERROR_SUCCESS)
        {
            type = 0xdeadbeef;
            size = MAX_PATH;

            memset(path, 0, sizeof(path));
            res = RegQueryValueExA(hkey, "driver", NULL, &type, (BYTE *)path, &size);
            ok(res == ERROR_SUCCESS, "RegGetValueA failed\n");
            ok(type == REG_SZ, "got %lu\n", type);
            ok(strlen(path) == 0, "Invalid value\n");

            RegCloseKey(hkey);
        }

        ret = SQLRemoveDSNFromIni("wine_mis");
        ok(ret, "got %d\n", ret);
    }
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
    test_SQLWriteDSNToIni();
}
