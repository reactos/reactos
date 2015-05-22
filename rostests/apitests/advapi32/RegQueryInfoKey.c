/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.Lib in the top level directory
 * PURPOSE:         Test for RegQueryInfoKey
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#define WIN32_NO_STATUS
#include <winreg.h>

#define TestKeyAccess(da, er, es) TestKeyAccess_(__FILE__, __LINE__, da, er, es)
static
VOID
TestKeyAccess_(
    _In_ PCSTR File,
    _In_ INT Line,
    _In_ REGSAM DesiredAccess,
    _In_ LONG ExpectedReturn,
    _In_ BOOLEAN ExpectSd)
{
    DWORD cbSd;
    HKEY hKey;
    LONG ret;

    ret = RegOpenKeyExW(HKEY_LOCAL_MACHINE, L"Software", 0, DesiredAccess, &hKey);
    ok_(File, Line)(ret == NO_ERROR, "RegOpenKeyEx returned %ld\n", ret);
    if (ret == NO_ERROR)
    {
        cbSd = 0x55555555;
        ret = RegQueryInfoKeyW(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cbSd, NULL);
        ok_(File, Line)(ret == ExpectedReturn, "RegQueryInfoKeyW returned %ld\n", ret);
        if (ExpectSd)
            ok_(File, Line)(cbSd != 0 && cbSd != 0x55555555, "RegQueryInfoKeyW - cbSd = %lu\n", cbSd);
        else
            ok_(File, Line)(cbSd == 0, "RegQueryInfoKeyW - cbSd = %lu\n", cbSd);

        cbSd = 0x55555555;
        ret = RegQueryInfoKeyA(hKey, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, &cbSd, NULL);
        ok_(File, Line)(ret == ExpectedReturn, "RegQueryInfoKeyA returned %ld\n", ret);
        if (ExpectSd)
            ok_(File, Line)(cbSd != 0 && cbSd != 0x55555555, "RegQueryInfoKeyA - cbSd = %lu\n", cbSd);
        else
            ok_(File, Line)(cbSd == 0, "RegQueryInfoKeyA - cbSd = %lu\n", cbSd);
        ret = RegCloseKey(hKey);
        ok_(File, Line)(ret == NO_ERROR, "RegCloseKey returned %ld\n", ret);
    }
    else
    {
        skip_(File, Line)("No key handle\n");
    }
}

START_TEST(RegQueryInfoKey)
{
    /* 0 access just fails the open */
    if (0)
    TestKeyAccess(0,                                ERROR_ACCESS_DENIED, FALSE);
    /* Without KEY_QUERY_VALUE we can't query anything */
    TestKeyAccess(READ_CONTROL,                     ERROR_ACCESS_DENIED, FALSE);
    /* Without READ_CONTROL we'll get success but SD size will yield 0 */
    TestKeyAccess(KEY_QUERY_VALUE,                  NO_ERROR, FALSE);
    /* With the two combined we get everything */
    TestKeyAccess(KEY_QUERY_VALUE | READ_CONTROL,   NO_ERROR, TRUE);
    /* Write rights give us everything too */
    TestKeyAccess(KEY_SET_VALUE,                    NO_ERROR, TRUE);
    TestKeyAccess(KEY_CREATE_SUB_KEY,               NO_ERROR, TRUE);
    TestKeyAccess(KEY_CREATE_LINK,                  NO_ERROR, TRUE);
    TestKeyAccess(DELETE,                           NO_ERROR, TRUE);
    TestKeyAccess(WRITE_DAC,                        NO_ERROR, TRUE);
    TestKeyAccess(WRITE_OWNER,                      NO_ERROR, TRUE);
    /* But these return nothing */
    TestKeyAccess(KEY_ENUMERATE_SUB_KEYS,           ERROR_ACCESS_DENIED, FALSE);
    TestKeyAccess(KEY_NOTIFY,                       ERROR_ACCESS_DENIED, FALSE);
}
