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
   static const WCHAR abcd_key[] = {'S','o','f','t','w','a','r','e','\\','O','D','B','C','\\','a','b','c','d','.','I','N','I','\\','w','i','n','e','o','d','b','c',0};
   static const WCHAR abcdini_key[] = {'S','o','f','t','w','a','r','e','\\','O','D','B','C','\\','a','b','c','d','.','I','N','I',0 };
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

START_TEST(misc)
{
    test_SQLConfigMode();
    test_SQLInstallerError();
    test_SQLInstallDriverManager();
    test_SQLWritePrivateProfileString();
}
