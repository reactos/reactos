/*
 * DLL for testing type 1 custom actions
 *
 * Copyright 2017 Zebediah Figura
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

#if 0
#pragma makedep testdll
#endif

#include <stdarg.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winsvc.h>
#include <odbcinst.h>
#define COBJMACROS
#include <shlobj.h>
#include <msxml.h>
#include <msi.h>
#include <msiquery.h>
#include <msidefs.h>

#ifdef __MINGW32__
#define __WINE_PRINTF_ATTR(fmt,args) __attribute__((format (printf,fmt,args)))
#else
#define __WINE_PRINTF_ATTR(fmt,args)
#endif

static int todo_level, todo_do_loop;

static void WINAPIV  __WINE_PRINTF_ATTR(6,7) ok_(MSIHANDLE hinst, int todo, const char *file, int line, int condition, const char *msg, ...)
{
    static char buffer[2000];
    MSIHANDLE record;
    va_list valist;

    va_start(valist, msg);
    vsprintf(buffer, msg, valist);
    va_end(valist);

    record = MsiCreateRecord(5);
    MsiRecordSetInteger(record, 1, todo);
    MsiRecordSetStringA(record, 2, file);
    MsiRecordSetInteger(record, 3, line);
    MsiRecordSetInteger(record, 4, condition);
    MsiRecordSetStringA(record, 5, buffer);
    MsiProcessMessage(hinst, INSTALLMESSAGE_USER, record);
    MsiCloseHandle(record);
}

static void winetest_start_todo( int is_todo )
{
    todo_level = (todo_level << 1) | (is_todo != 0);
    todo_do_loop=1;
}

static int winetest_loop_todo(void)
{
    int do_loop=todo_do_loop;
    todo_do_loop=0;
    return do_loop;
}

static void winetest_end_todo(void)
{
    todo_level >>= 1;
}

#define ok(hinst, condition, ...)   ok_(hinst, todo_level, __FILE__, __LINE__, condition, __VA_ARGS__)
#define todo_wine_if(is_todo) for (winetest_start_todo(is_todo); \
                                   winetest_loop_todo(); \
                                   winetest_end_todo())
#define todo_wine   todo_wine_if(1)

static const char *dbgstr_w(WCHAR *str)
{
    static char buffer[300], *p;

    if (!str) return "(null)";

    p = buffer;
    *p++ = 'L';
    *p++ = '"';
    while ((*p++ = *str++));
    *p++ = '"';
    *p++ = 0;

    return buffer;
}

static void check_prop(MSIHANDLE hinst, const char *prop, const char *expect)
{
    char buffer[10] = "x";
    DWORD sz = sizeof(buffer);
    UINT r = MsiGetPropertyA(hinst, prop, buffer, &sz);
    ok(hinst, !r, "'%s': got %u\n", prop, r);
    ok(hinst, sz == strlen(buffer), "'%s': expected %Iu, got %lu\n", prop, strlen(buffer), sz);
    ok(hinst, !strcmp(buffer, expect), "expected '%s', got '%s'\n", expect, buffer);
}

static void test_props(MSIHANDLE hinst)
{
    char buffer[10];
    WCHAR bufferW[10];
    DWORD sz;
    UINT r;

    /* test invalid values */
    r = MsiGetPropertyA(hinst, NULL, NULL, NULL);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    r = MsiGetPropertyA(hinst, "boo", NULL, NULL);
    ok(hinst, !r, "got %u\n", r);

    r = MsiGetPropertyA(hinst, "boo", buffer, NULL );
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    sz = 0;
    r = MsiGetPropertyA(hinst, "boo", NULL, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 0, "got size %lu\n", sz);

    sz = 0;
    strcpy(buffer,"x");
    r = MsiGetPropertyA(hinst, "boo", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "x"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 0, "got size %lu\n", sz);

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetPropertyA(hinst, "boo", buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !buffer[0], "got \"%s\"\n", buffer);
    ok(hinst, sz == 0, "got size %lu\n", sz);

    /* set the property to something */
    r = MsiSetPropertyA(hinst, NULL, NULL);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    r = MsiSetPropertyA(hinst, "", NULL);
    ok(hinst, !r, "got %u\n", r);

    r = MsiSetPropertyA(hinst, "", "asdf");
    ok(hinst, r == ERROR_FUNCTION_FAILED, "got %u\n", r);

    r = MsiSetPropertyA(hinst, "=", "asdf");
    ok(hinst, !r, "got %u\n", r);
    check_prop(hinst, "=", "asdf");

    r = MsiSetPropertyA(hinst, " ", "asdf");
    ok(hinst, !r, "got %u\n", r);
    check_prop(hinst, " ", "asdf");

    r = MsiSetPropertyA(hinst, "'", "asdf");
    ok(hinst, !r, "got %u\n", r);
    check_prop(hinst, "'", "asdf");

    r = MsiSetPropertyA(hinst, "boo", NULL);
    ok(hinst, !r, "got %u\n", r);
    check_prop(hinst, "boo", "");

    r = MsiSetPropertyA(hinst, "boo", "");
    ok(hinst, !r, "got %u\n", r);
    check_prop(hinst, "boo", "");

    r = MsiSetPropertyA(hinst, "boo", "xyz");
    ok(hinst, !r, "got %u\n", r);
    check_prop(hinst, "boo", "xyz");

    r = MsiGetPropertyA(hinst, "boo", NULL, NULL);
    ok(hinst, !r, "got %u\n", r);

    r = MsiGetPropertyA(hinst, "boo", buffer, NULL );
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    /* Returned size is in bytes, not chars, but only for custom actions.
     * Seems to be a casualty of RPC... */

    sz = 0;
    r = MsiGetPropertyA(hinst, "boo", NULL, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 6, "got size %lu\n", sz);

    sz = 0;
    strcpy(buffer,"q");
    r = MsiGetPropertyA(hinst, "boo", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "q"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 6, "got size %lu\n", sz);

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetPropertyA(hinst, "boo", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !buffer[0], "got \"%s\"\n", buffer);
    ok(hinst, sz == 6, "got size %lu\n", sz);

    sz = 3;
    strcpy(buffer,"x");
    r = MsiGetPropertyA(hinst, "boo", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "xy"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 6, "got size %lu\n", sz);

    sz = 4;
    strcpy(buffer,"x");
    r = MsiGetPropertyA(hinst, "boo", buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "xyz"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 3, "got size %lu\n", sz);

    r = MsiGetPropertyW(hinst, L"boo", NULL, NULL);
    ok(hinst, !r, "got %u\n", r);

    r = MsiGetPropertyW(hinst, L"boo", bufferW, NULL );
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    sz = 0;
    r = MsiGetPropertyW(hinst, L"boo", NULL, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 3, "got size %lu\n", sz);

    sz = 0;
    lstrcpyW(bufferW, L"boo");
    r = MsiGetPropertyW(hinst, L"boo", bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !lstrcmpW(bufferW, L"boo"), "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 3, "got size %lu\n", sz);

    sz = 1;
    lstrcpyW(bufferW, L"boo");
    r = MsiGetPropertyW(hinst, L"boo", bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !bufferW[0], "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 3, "got size %lu\n", sz);

    sz = 3;
    lstrcpyW(bufferW, L"boo");
    r = MsiGetPropertyW(hinst, L"boo", bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !lstrcmpW(bufferW, L"xy"), "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 3, "got size %lu\n", sz);

    sz = 4;
    lstrcpyW(bufferW, L"boo");
    r = MsiGetPropertyW(hinst, L"boo", bufferW, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !lstrcmpW(bufferW, L"xyz"), "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 3, "got size %lu\n", sz);

    r = MsiSetPropertyA(hinst, "boo", NULL);
    ok(hinst, !r, "got %u\n", r);
    check_prop(hinst, "boo", "");

    sz = 0;
    r = MsiGetPropertyA(hinst, "embednullprop", NULL, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 6, "got size %lu\n", sz);

    sz = 4;
    memset(buffer, 0xcc, sizeof(buffer));
    r = MsiGetPropertyA(hinst, "embednullprop", buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 3, "got size %lu\n", sz);
    ok(hinst, !memcmp(buffer, "a\0\0\0\xcc", 5), "wrong data\n");
}

static void test_db(MSIHANDLE hinst)
{
    static const UINT prop_type[20] = { VT_EMPTY, VT_EMPTY, VT_LPSTR, VT_EMPTY, VT_EMPTY,
                                        VT_EMPTY, VT_EMPTY, VT_LPSTR, VT_EMPTY, VT_LPSTR,
                                        VT_EMPTY, VT_EMPTY, VT_EMPTY, VT_EMPTY, VT_I4,
                                        VT_I4, VT_EMPTY, VT_EMPTY, VT_EMPTY, VT_EMPTY };
    MSIHANDLE hdb, hdb2, view, rec, rec2, suminfo;
    char buffer[10];
    DWORD sz;
    UINT r, count, type, i;
    INT int_value;
    FILETIME ft;

    hdb = MsiGetActiveDatabase(hinst);
    ok(hinst, hdb, "MsiGetActiveDatabase failed\n");

    r = MsiDatabaseIsTablePersistentA(hdb, "Test");
    ok(hinst, r == MSICONDITION_TRUE, "got %u\n", r);

    r = MsiDatabaseOpenViewA(hdb, NULL, &view);
    ok(hinst, r == ERROR_BAD_QUERY_SYNTAX, "got %u\n", r);

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `Test`", NULL);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `Test`", &view);
    ok(hinst, !r, "got %u\n", r);

    r = MsiViewGetColumnInfo(view, MSICOLINFO_NAMES, &rec2);
    ok(hinst, !r, "got %u\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetStringA(rec2, 1, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == strlen(buffer), "got size %lu\n", sz);
    ok(hinst, !strcmp(buffer, "Name"), "got '%s'\n", buffer);

    /* Test MsiGetActiveDatabase + MsiDatabaseIsTablePersistent once again */
    hdb2 = MsiGetActiveDatabase(hinst);
    ok(hinst, hdb2, "MsiGetActiveDatabase failed\n");
    ok(hinst, hdb2 != hdb, "db handles should be different\n");

    r = MsiDatabaseIsTablePersistentA(hdb2, "Test");
    ok(hinst, r == MSICONDITION_TRUE, "got %u\n", r);

    r = MsiCloseHandle(hdb2);
    ok(hinst, !r, "got %u\n", r);

    r = MsiCloseHandle(rec2);
    ok(hinst, !r, "got %u\n", r);

    r = MsiViewExecute(view, 0);
    ok(hinst, !r, "got %u\n", r);

    r = MsiViewFetch(view, &rec2);
    ok(hinst, !r, "got %u\n", r);

    r = MsiRecordGetFieldCount(rec2);
    ok(hinst, r == 3, "got %u\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetStringA(rec2, 1, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == strlen(buffer), "got size %lu\n", sz);
    ok(hinst, !strcmp(buffer, "one"), "got '%s'\n", buffer);

    r = MsiRecordGetInteger(rec2, 2);
    ok(hinst, r == 1, "got %u\n", r);

    sz = sizeof(buffer);
    r = MsiRecordReadStream(rec2, 3, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !memcmp(buffer, "unus", 4), "wrong data\n");

    r = MsiCloseHandle(rec2);
    ok(hinst, !r, "got %u\n", r);

    r = MsiViewFetch(view, &rec2);
    ok(hinst, !r, "got %u\n", r);

    r = MsiRecordGetFieldCount(rec2);
    ok(hinst, r == 3, "got %u\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetStringA(rec2, 1, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == strlen(buffer), "got size %lu\n", sz);
    ok(hinst, !strcmp(buffer, "two"), "got '%s'\n", buffer);

    r = MsiRecordGetInteger(rec2, 2);
    ok(hinst, r == 2, "got %u\n", r);

    sz = sizeof(buffer);
    r = MsiRecordReadStream(rec2, 3, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !memcmp(buffer, "duo", 3), "wrong data\n");

    r = MsiViewModify(view, MSIMODIFY_REFRESH, 0);
    ok(hinst, r == ERROR_INVALID_HANDLE, "got %u\n", r);

    r = MsiRecordSetStringA(rec2, 1, "three");
    ok(hinst, !r, "got %u\n", r);

    r = MsiRecordSetInteger(rec2, 2, 3);
    ok(hinst, !r, "got %u\n", r);

    r = MsiRecordSetInteger(rec2, 3, 3);
    ok(hinst, !r, "got %u\n", r);

    r = MsiViewModify(view, MSIMODIFY_REFRESH, rec2);
    ok(hinst, !r, "got %u\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetStringA(rec2, 1, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == strlen(buffer), "got size %lu\n", sz);
    ok(hinst, !strcmp(buffer, "two"), "got '%s'\n", buffer);

    r = MsiRecordGetInteger(rec2, 2);
    ok(hinst, r == 2, "got %u\n", r);

    sz = sizeof(buffer);
    r = MsiRecordReadStream(rec2, 3, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !memcmp(buffer, "duo", 3), "wrong data\n");

    r = MsiCloseHandle(rec2);
    ok(hinst, !r, "got %u\n", r);

    r = MsiViewFetch(view, &rec2);
    ok(hinst, r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    ok(hinst, !rec2, "got %lu\n", rec2);

    r = MsiViewClose(view);
    ok(hinst, !r, "got %u\n", r);

    r = MsiCloseHandle(view);
    ok(hinst, !r, "got %u\n", r);

    r = MsiDatabaseOpenViewA(hdb, "SELECT * FROM `Test` WHERE `Name` = ?", &view);
    ok(hinst, !r, "got %u\n", r);

    rec = MsiCreateRecord(1);
    MsiRecordSetStringA(rec, 1, "one");

    r = MsiViewExecute(view, rec);
    ok(hinst, !r, "got %u\n", r);

    r = MsiViewFetch(view, &rec2);
    ok(hinst, !r, "got %u\n", r);

    r = MsiRecordGetInteger(rec2, 2);
    ok(hinst, r == 1, "got %u\n", r);

    r = MsiCloseHandle(rec2);
    ok(hinst, !r, "got %u\n", r);

    r = MsiViewFetch(view, &rec2);
    ok(hinst, r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    ok(hinst, !rec2, "got %lu\n", rec2);

    r = MsiCloseHandle(rec);
    ok(hinst, !r, "got %u\n", r);

    r = MsiCloseHandle(view);
    ok(hinst, !r, "got %u\n", r);

    /* test MsiDatabaseGetPrimaryKeys() */
    r = MsiDatabaseGetPrimaryKeysA(hdb, "Test", &rec);
    ok(hinst, !r, "got %u\n", r);

    r = MsiRecordGetFieldCount(rec);
    ok(hinst, r == 1, "got %u\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetStringA(rec, 0, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == strlen(buffer), "got size %lu\n", sz);
    ok(hinst, !strcmp(buffer, "Test"), "got '%s'\n", buffer);

    sz = sizeof(buffer);
    r = MsiRecordGetStringA(rec, 1, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == strlen(buffer), "got size %lu\n", sz);
    ok(hinst, !strcmp(buffer, "Name"), "got '%s'\n", buffer);

    r = MsiCloseHandle(rec);
    ok(hinst, !r, "got %u\n", r);

    r = MsiGetSummaryInformationA(hdb, NULL, 1, NULL);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &suminfo);
    ok(hinst, !r, "got %u\n", r);

    r = MsiSummaryInfoGetPropertyCount(suminfo, NULL);
    ok(hinst, r == RPC_X_NULL_REF_POINTER, "got %u\n", r);

    count = 0xdeadbeef;
    r = MsiSummaryInfoGetPropertyCount(suminfo, &count);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, count == 5, "got %u\n", count);

    r = MsiSummaryInfoGetPropertyA(suminfo, 0, NULL, NULL, NULL, NULL, NULL);
    ok(hinst, r == RPC_X_NULL_REF_POINTER, "got %u\n", r);

    for (i = 0; i < 20; i++)
    {
        /* for some reason query for PID_TITLE leads to install failure under Windows */
        if (i == PID_TITLE) continue;

        type = 0xdeadbeef;
        int_value = 0xdeadbeef;
        *buffer = 0;
        sz = sizeof(buffer);
        r = MsiSummaryInfoGetPropertyA(suminfo, i, &type, &int_value, &ft, buffer, &sz);
        if (sz == sizeof(buffer) || i == PID_TEMPLATE)
            ok(hinst, !r, "%u: got %u\n", i, r);
        else
            ok(hinst, r == ERROR_MORE_DATA, "%u: got %u\n", i, r);
        ok(hinst, type == prop_type[i], "%u: expected %u, got %u\n", i, prop_type[i], type);
        if (i == PID_PAGECOUNT)
            ok(hinst, int_value == 100, "%u: got %u\n", i, int_value);
        else
            ok(hinst, int_value == 0, "%u: got %u\n", i, int_value);
        if (i == PID_TEMPLATE)
        {
            ok(hinst, sz == 5, "%u: got %lu\n", i, sz);
            ok(hinst, !lstrcmpA(buffer, ";1033"), "%u: got %s\n", i, buffer);
        }
        else if (i == PID_REVNUMBER)
        {
            ok(hinst, sz == 76, "%u: got %lu\n", i, sz);
            ok(hinst, !lstrcmpA(buffer, "{004757CA"), "%u: got %s\n", i, buffer);
        }
        else
        {
            ok(hinst, sz == sizeof(buffer), "%u: got %lu\n", i, sz);
            ok(hinst, !*buffer, "%u: got %s\n", i, buffer);
        }
    }

    GetSystemTimeAsFileTime(&ft);

    for (i = 0; i < 20; i++)
    {
        r = MsiSummaryInfoSetPropertyA(suminfo, i, prop_type[i], 1252, &ft, "");
        ok(hinst, r == ERROR_FUNCTION_FAILED, "%u: got %u\n", i, r);

        r = MsiSummaryInfoSetPropertyW(suminfo, i, prop_type[i], 1252, &ft, L"");
        ok(hinst, r == ERROR_FUNCTION_FAILED, "%u: got %u\n", i, r);
    }

    r = MsiCloseHandle(suminfo);
    ok(hinst, !r, "got %u\n", r);

    r = MsiCloseHandle(hdb);
    ok(hinst, !r, "got %u\n", r);
}

static void test_doaction(MSIHANDLE hinst)
{
    UINT r;

    r = MsiDoActionA(hinst, "nested51");
    ok(hinst, !r, "got %u\n", r);
    check_prop(hinst, "nested", "1");

    r = MsiDoActionA(hinst, "nested1");
    ok(hinst, !r, "got %u\n", r);
    check_prop(hinst, "nested", "2");

    r = MsiSequenceA(hinst, NULL, 0);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    r = MsiSequenceA(hinst, "TestSequence", 0);
    ok(hinst, !r, "got %u\n", r);
    check_prop(hinst, "nested", "1");
}

UINT WINAPI nested(MSIHANDLE hinst)
{
    MsiSetPropertyA(hinst, "nested", "2");

    return ERROR_SUCCESS;
}

static void test_targetpath(MSIHANDLE hinst)
{
    WCHAR bufferW[100];
    char buffer[100];
    DWORD sz, srcsz;
    UINT r;

    /* test invalid values */
    r = MsiGetTargetPathA(hinst, NULL, NULL, NULL);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    r = MsiGetTargetPathA(hinst, "TARGETDIR", NULL, NULL );
    ok(hinst, !r, "got %u\n", r);

    r = MsiGetTargetPathA(hinst, "TARGETDIR", buffer, NULL );
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    /* Returned size is in bytes, not chars, but only for custom actions.
     * Seems to be a casualty of RPC... */

    sz = 0;
    r = MsiGetTargetPathA(hinst, "TARGETDIR", NULL, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 6, "got size %lu\n", sz);

    sz = 0;
    strcpy(buffer,"q");
    r = MsiGetTargetPathA(hinst, "TARGETDIR", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "q"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 6, "got size %lu\n", sz);

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetTargetPathA(hinst, "TARGETDIR", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !buffer[0], "got \"%s\"\n", buffer);
    ok(hinst, sz == 6, "got size %lu\n", sz);

    sz = 3;
    strcpy(buffer,"x");
    r = MsiGetTargetPathA(hinst, "TARGETDIR", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "C:"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 6, "got size %lu\n", sz);

    sz = 4;
    strcpy(buffer,"x");
    r = MsiGetTargetPathA(hinst, "TARGETDIR", buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "C:\\"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 3, "got size %lu\n", sz);

    sz = 0;
    r = MsiGetTargetPathW(hinst, L"TARGETDIR", NULL, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 3, "got size %lu\n", sz);

    sz = 0;
    bufferW[0] = 'q';
    r = MsiGetTargetPathW(hinst, L"TARGETDIR", bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, bufferW[0] == 'q', "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 3, "got size %lu\n", sz);

    sz = 1;
    bufferW[0] = 'q';
    r = MsiGetTargetPathW(hinst, L"TARGETDIR", bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !bufferW[0], "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 3, "got size %lu\n", sz);

    sz = 3;
    bufferW[0] = 'q';
    r = MsiGetTargetPathW(hinst, L"TARGETDIR", bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !lstrcmpW(bufferW, L"C:"), "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 3, "got size %lu\n", sz);

    sz = 4;
    bufferW[0] = 'q';
    r = MsiGetTargetPathW(hinst, L"TARGETDIR", bufferW, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !lstrcmpW(bufferW, L"C:\\"), "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 3, "got size %lu\n", sz);

    r = MsiSetTargetPathA(hinst, NULL, "C:\\subdir");
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    r = MsiSetTargetPathA(hinst, "MSITESTDIR", NULL);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    r = MsiSetTargetPathA(hinst, "MSITESTDIR", "C:\\subdir");
    ok(hinst, !r, "got %u\n", r);

    sz = sizeof(buffer);
    r = MsiGetTargetPathA(hinst, "MSITESTDIR", buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "C:\\subdir\\"), "got \"%s\"\n", buffer);

    r = MsiSetTargetPathA(hinst, "MSITESTDIR", "C:\\");

    /* test GetSourcePath() */

    r = MsiGetSourcePathA(hinst, NULL, NULL, NULL);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    r = MsiGetSourcePathA(hinst, "TARGETDIR", NULL, NULL );
    ok(hinst, !r, "got %u\n", r);

    r = MsiGetSourcePathA(hinst, "TARGETDIR", buffer, NULL );
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    /* Returned size is in bytes, not chars, but only for custom actions.
     * Seems to be a casualty of RPC... */

    srcsz = 0;
    MsiGetSourcePathW(hinst, L"TARGETDIR", NULL, &srcsz);

    sz = 0;
    r = MsiGetSourcePathA(hinst, "TARGETDIR", NULL, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == srcsz * 2, "got size %lu\n", sz);

    sz = 0;
    strcpy(buffer,"q");
    r = MsiGetSourcePathA(hinst, "TARGETDIR", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "q"), "got \"%s\"\n", buffer);
    ok(hinst, sz == srcsz * 2, "got size %lu\n", sz);

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetSourcePathA(hinst, "TARGETDIR", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !buffer[0], "got \"%s\"\n", buffer);
    ok(hinst, sz == srcsz * 2, "got size %lu\n", sz);

    sz = srcsz;
    strcpy(buffer,"x");
    r = MsiGetSourcePathA(hinst, "TARGETDIR", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, strlen(buffer) == srcsz - 1, "wrong buffer length %Iu\n", strlen(buffer));
    ok(hinst, sz == srcsz * 2, "got size %lu\n", sz);

    sz = srcsz + 1;
    strcpy(buffer,"x");
    r = MsiGetSourcePathA(hinst, "TARGETDIR", buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, strlen(buffer) == srcsz, "wrong buffer length %Iu\n", strlen(buffer));
    ok(hinst, sz == srcsz, "got size %lu\n", sz);

    sz = 0;
    r = MsiGetSourcePathW(hinst, L"TARGETDIR", NULL, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == srcsz, "got size %lu\n", sz);

    sz = 0;
    bufferW[0] = 'q';
    r = MsiGetSourcePathW(hinst, L"TARGETDIR", bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, bufferW[0] == 'q', "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == srcsz, "got size %lu\n", sz);

    sz = 1;
    bufferW[0] = 'q';
    r = MsiGetSourcePathW(hinst, L"TARGETDIR", bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !bufferW[0], "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == srcsz, "got size %lu\n", sz);

    sz = srcsz;
    bufferW[0] = 'q';
    r = MsiGetSourcePathW(hinst, L"TARGETDIR", bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, lstrlenW(bufferW) == srcsz - 1, "wrong buffer length %d\n", lstrlenW(bufferW));
    ok(hinst, sz == srcsz, "got size %lu\n", sz);

    sz = srcsz + 1;
    bufferW[0] = 'q';
    r = MsiGetSourcePathW(hinst, L"TARGETDIR", bufferW, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, lstrlenW(bufferW) == srcsz, "wrong buffer length %d\n", lstrlenW(bufferW));
    ok(hinst, sz == srcsz, "got size %lu\n", sz);
}

static void test_misc(MSIHANDLE hinst)
{
    MSICONDITION cond;
    LANGID lang;
    UINT r;

    r = MsiSetMode(hinst, MSIRUNMODE_REBOOTATEND, FALSE);
    ok(hinst, !r, "got %u\n", r);

    lang = MsiGetLanguage(hinst);
    ok(hinst, lang == 1033, "got %u\n", lang);

    check_prop(hinst, "INSTALLLEVEL", "3");
    r = MsiSetInstallLevel(hinst, 123);
    ok(hinst, !r, "got %u\n", r);
    check_prop(hinst, "INSTALLLEVEL", "123");
    MsiSetInstallLevel(hinst, 3);

    cond = MsiEvaluateConditionA(hinst, NULL);
    ok(hinst, cond == MSICONDITION_NONE, "got %u\n", cond);
    MsiSetPropertyA(hinst, "condprop", "1");
    cond = MsiEvaluateConditionA(hinst, "condprop = 1");
    ok(hinst, cond == MSICONDITION_TRUE, "got %u\n", cond);
}

static void test_feature_states(MSIHANDLE hinst)
{
    INSTALLSTATE state, action;
    UINT r;

    /* test feature states */

    r = MsiGetFeatureStateA(hinst, NULL, &state, &action);
    ok(hinst, r == ERROR_UNKNOWN_FEATURE, "got %u\n", r);

    r = MsiGetFeatureStateA(hinst, "fake", &state, &action);
    ok(hinst, r == ERROR_UNKNOWN_FEATURE, "got %u\n", r);

    r = MsiGetFeatureStateA(hinst, "One", NULL, &action);
    ok(hinst, r == RPC_X_NULL_REF_POINTER, "got %u\n", r);

    r = MsiGetFeatureStateA(hinst, "One", &state, NULL);
    ok(hinst, r == RPC_X_NULL_REF_POINTER, "got %u\n", r);

    r = MsiGetFeatureStateA(hinst, "One", &state, &action);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, state == INSTALLSTATE_ABSENT, "got state %d\n", state);
    ok(hinst, action == INSTALLSTATE_LOCAL, "got action %d\n", action);

    r = MsiSetFeatureStateA(hinst, NULL, INSTALLSTATE_ABSENT);
    ok(hinst, r == ERROR_UNKNOWN_FEATURE, "got %u\n", r);

    r = MsiSetFeatureStateA(hinst, "One", INSTALLSTATE_ADVERTISED);
    ok(hinst, !r, "got %u\n", r);

    r = MsiGetFeatureStateA(hinst, "One", &state, &action);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, action == INSTALLSTATE_ADVERTISED, "got action %d\n", action);

    r = MsiSetFeatureStateA(hinst, "One", INSTALLSTATE_LOCAL);
    ok(hinst, !r, "got %u\n", r);

    r = MsiGetFeatureStateA(hinst, "One", &state, &action);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, action == INSTALLSTATE_LOCAL, "got action %d\n", action);

    /* test component states */

    r = MsiGetComponentStateA(hinst, NULL, &state, &action);
    ok(hinst, r == ERROR_UNKNOWN_COMPONENT, "got %u\n", r);

    r = MsiGetComponentStateA(hinst, "fake", &state, &action);
    ok(hinst, r == ERROR_UNKNOWN_COMPONENT, "got %u\n", r);

    r = MsiGetComponentStateA(hinst, "One", NULL, &action);
    ok(hinst, r == RPC_X_NULL_REF_POINTER, "got %u\n", r);

    r = MsiGetComponentStateA(hinst, "One", &state, NULL);
    ok(hinst, r == RPC_X_NULL_REF_POINTER, "got %u\n", r);

    r = MsiGetComponentStateA(hinst, "One", &state, &action);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, state == INSTALLSTATE_ABSENT, "got state %d\n", state);
    ok(hinst, action == INSTALLSTATE_LOCAL, "got action %d\n", action);

    r = MsiGetComponentStateA(hinst, "dangler", &state, &action);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, state == INSTALLSTATE_ABSENT, "got state %d\n", state);
    ok(hinst, action == INSTALLSTATE_UNKNOWN, "got action %d\n", action);

    r = MsiGetComponentStateA(hinst, "component", &state, &action);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, state == INSTALLSTATE_UNKNOWN, "got state %d\n", state);
    ok(hinst, action == INSTALLSTATE_LOCAL, "got action %d\n", action);

    r = MsiSetComponentStateA(hinst, NULL, INSTALLSTATE_ABSENT);
    ok(hinst, r == ERROR_UNKNOWN_COMPONENT, "got %u\n", r);

    r = MsiSetComponentStateA(hinst, "One", INSTALLSTATE_SOURCE);
    ok(hinst, !r, "got %u\n", r);

    r = MsiGetComponentStateA(hinst, "One", &state, &action);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, state == INSTALLSTATE_ABSENT, "got state %d\n", state);
    ok(hinst, action == INSTALLSTATE_SOURCE, "got action %d\n", action);

    r = MsiSetComponentStateA(hinst, "One", INSTALLSTATE_LOCAL);
    ok(hinst, !r, "got %u\n", r);

    r = MsiGetComponentStateA(hinst, "One", &state, &action);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, state == INSTALLSTATE_ABSENT, "got state %d\n", state);
    ok(hinst, action == INSTALLSTATE_LOCAL, "got action %d\n", action);
}

static void test_format_record(MSIHANDLE hinst)
{
    WCHAR bufferW[10];
    char buffer[10];
    MSIHANDLE rec;
    DWORD sz;
    UINT r;

    r = MsiFormatRecordA(hinst, 0, NULL, NULL);
    ok(hinst, r == ERROR_INVALID_HANDLE, "got %u\n", r);

    rec = MsiCreateRecord(1);
    MsiRecordSetStringA(rec, 0, "foo [1]");
    MsiRecordSetInteger(rec, 1, 123);

    r = MsiFormatRecordA(hinst, rec, NULL, NULL);
    ok(hinst, !r, "got %u\n", r);

    r = MsiFormatRecordA(hinst, rec, buffer, NULL);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    /* Returned size is in bytes, not chars, but only for custom actions. */

    sz = 0;
    r = MsiFormatRecordA(hinst, rec, NULL, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 14, "got size %lu\n", sz);

    sz = 0;
    strcpy(buffer,"q");
    r = MsiFormatRecordA(hinst, rec, buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "q"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 14, "got size %lu\n", sz);

    sz = 1;
    strcpy(buffer,"x");
    r = MsiFormatRecordA(hinst, rec, buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !buffer[0], "got \"%s\"\n", buffer);
    ok(hinst, sz == 14, "got size %lu\n", sz);

    sz = 7;
    strcpy(buffer,"x");
    r = MsiFormatRecordA(hinst, rec, buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "foo 12"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 14, "got size %lu\n", sz);

    sz = 8;
    strcpy(buffer,"x");
    r = MsiFormatRecordA(hinst, rec, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "foo 123"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 7, "got size %lu\n", sz);

    r = MsiFormatRecordW(hinst, rec, NULL, NULL);
    ok(hinst, !r, "got %u\n", r);

    r = MsiFormatRecordW(hinst, rec, bufferW, NULL);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    sz = 0;
    r = MsiFormatRecordW(hinst, rec, NULL, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 7, "got size %lu\n", sz);

    sz = 0;
    bufferW[0] = 'q';
    r = MsiFormatRecordW(hinst, rec, bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, bufferW[0] == 'q', "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 7, "got size %lu\n", sz);

    sz = 1;
    bufferW[0] = 'q';
    r = MsiFormatRecordW(hinst, rec, bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !bufferW[0], "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 7, "got size %lu\n", sz);

    sz = 7;
    bufferW[0] = 'q';
    r = MsiFormatRecordW(hinst, rec, bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !lstrcmpW(bufferW, L"foo 12"), "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 7, "got size %lu\n", sz);

    sz = 8;
    bufferW[0] = 'q';
    r = MsiFormatRecordW(hinst, rec, bufferW, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !lstrcmpW(bufferW, L"foo 123"), "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 7, "got size %lu\n", sz);

    /* check that properties work */
    MsiSetPropertyA(hinst, "fmtprop", "foobar");
    MsiRecordSetStringA(rec, 0, "[fmtprop]");
    sz = sizeof(buffer);
    r = MsiFormatRecordA(hinst, rec, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "foobar"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 6, "got size %lu\n", sz);

    MsiCloseHandle(rec);
}

static void test_costs(MSIHANDLE hinst)
{
    WCHAR bufferW[10];
    char buffer[10];
    int cost, temp;
    DWORD sz;
    UINT r;

    cost = 0xdead;
    r = MsiGetFeatureCostA(hinst, NULL, MSICOSTTREE_CHILDREN, INSTALLSTATE_LOCAL, &cost);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);
    todo_wine
    ok(hinst, !cost, "got %d\n", cost);

    r = MsiGetFeatureCostA(hinst, "One", MSICOSTTREE_CHILDREN, INSTALLSTATE_LOCAL, NULL);
    ok(hinst, r == RPC_X_NULL_REF_POINTER, "got %u\n", r);

    cost = 0xdead;
    r = MsiGetFeatureCostA(hinst, "One", MSICOSTTREE_CHILDREN, INSTALLSTATE_LOCAL, &cost);
    ok(hinst, !r, "got %u\n", r);
    todo_wine
    ok(hinst, cost == 8, "got %d\n", cost);

    sz = cost = temp = 0xdead;
    r = MsiEnumComponentCostsA(hinst, "One", 0, INSTALLSTATE_LOCAL, NULL, &sz, &cost, &temp);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);
    ok(hinst, sz == 0xdead, "got size %lu\n", sz);
    ok(hinst, cost == 0xdead, "got cost %d\n", cost);
    ok(hinst, temp == 0xdead, "got temp %d\n", temp);

    cost = temp = 0xdead;
    r = MsiEnumComponentCostsA(hinst, "One", 0, INSTALLSTATE_LOCAL, buffer, NULL, &cost, &temp);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);
    ok(hinst, cost == 0xdead, "got cost %d\n", cost);
    ok(hinst, temp == 0xdead, "got temp %d\n", temp);

    sz = temp = 0xdead;
    r = MsiEnumComponentCostsA(hinst, "One", 0, INSTALLSTATE_LOCAL, buffer, &sz, NULL, &temp);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);
    ok(hinst, sz == 0xdead, "got size %lu\n", sz);
    ok(hinst, temp == 0xdead, "got temp %d\n", temp);

    sz = cost = 0xdead;
    r = MsiEnumComponentCostsA(hinst, "One", 0, INSTALLSTATE_LOCAL, buffer, &sz, &cost, NULL);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);
    ok(hinst, sz == 0xdead, "got size %lu\n", sz);
    ok(hinst, cost == 0xdead, "got cost %d\n", cost);

    cost = temp = 0xdead;
    sz = sizeof(buffer);
    r = MsiEnumComponentCostsA(hinst, NULL, 0, INSTALLSTATE_LOCAL, buffer, &sz, &cost, &temp);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 2, "got size %lu\n", sz);
    ok(hinst, !strcmp(buffer, "C:"), "got '%s'\n", buffer);
    ok(hinst, !cost, "got cost %d\n", cost);
    ok(hinst, temp && temp != 0xdead, "got temp %d\n", temp);

    cost = temp = 0xdead;
    sz = sizeof(buffer);
    r = MsiEnumComponentCostsA(hinst, "One", 0, INSTALLSTATE_LOCAL, buffer, &sz, &cost, &temp);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 2, "got size %lu\n", sz);
    ok(hinst, !strcmp(buffer, "C:"), "got '%s'\n", buffer);
    ok(hinst, cost == 8, "got cost %d\n", cost);
    ok(hinst, !temp, "got temp %d\n", temp);

    /* same string behaviour */
    cost = temp = 0xdead;
    sz = 0;
    strcpy(buffer,"q");
    r = MsiEnumComponentCostsA(hinst, "One", 0, INSTALLSTATE_LOCAL, buffer, &sz, &cost, &temp);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "q"), "got \"%s\"\n", buffer);
    todo_wine
    ok(hinst, sz == 4, "got size %lu\n", sz);
    ok(hinst, cost == 8, "got cost %d\n", cost);
    ok(hinst, !temp, "got temp %d\n", temp);

    sz = 1;
    strcpy(buffer,"x");
    r = MsiEnumComponentCostsA(hinst, "One", 0, INSTALLSTATE_LOCAL, buffer, &sz, &cost, &temp);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    todo_wine {
    ok(hinst, !buffer[0], "got \"%s\"\n", buffer);
    ok(hinst, sz == 4, "got size %lu\n", sz);
    }

    sz = 2;
    strcpy(buffer,"x");
    r = MsiEnumComponentCostsA(hinst, "One", 0, INSTALLSTATE_LOCAL, buffer, &sz, &cost, &temp);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    todo_wine {
    ok(hinst, !strcmp(buffer, "C"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 4, "got size %lu\n", sz);
    }

    sz = 3;
    strcpy(buffer,"x");
    r = MsiEnumComponentCostsA(hinst, "One", 0, INSTALLSTATE_LOCAL, buffer, &sz, &cost, &temp);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "C:"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 2, "got size %lu\n", sz);

    sz = 0;
    bufferW[0] = 'q';
    r = MsiEnumComponentCostsW(hinst, L"One", 0, INSTALLSTATE_LOCAL, bufferW, &sz, &cost, &temp);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, bufferW[0] == 'q', "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 2, "got size %lu\n", sz);

    sz = 1;
    bufferW[0] = 'q';
    r = MsiEnumComponentCostsW(hinst, L"One", 0, INSTALLSTATE_LOCAL, bufferW, &sz, &cost, &temp);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !bufferW[0], "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 2, "got size %lu\n", sz);

    sz = 2;
    bufferW[0] = 'q';
    r = MsiEnumComponentCostsW(hinst, L"One", 0, INSTALLSTATE_LOCAL, bufferW, &sz, &cost, &temp);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !lstrcmpW(bufferW, L"C"), "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 2, "got size %lu\n", sz);

    sz = 3;
    bufferW[0] = 'q';
    r = MsiEnumComponentCostsW(hinst, L"One", 0, INSTALLSTATE_LOCAL, bufferW, &sz, &cost, &temp);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !lstrcmpW(bufferW, L"C:"), "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 2, "got size %lu\n", sz);
}

static void test_invalid_functions(MSIHANDLE hinst)
{
    char path[MAX_PATH], package_name[20];
    MSIHANDLE db, preview, package;
    UINT r;

    r = MsiGetDatabaseState(hinst);
    ok(hinst, r == MSIDBSTATE_ERROR, "got %u\n", r);

    db = MsiGetActiveDatabase(hinst);
    ok(hinst, db, "MsiGetActiveDatabase failed\n");

    r = MsiDatabaseGenerateTransformA(db, db, "bogus.mst", 0, 0);
    todo_wine ok(hinst, r == ERROR_INVALID_HANDLE, "got %u\n", r);

    r = MsiDatabaseApplyTransformA(db, "bogus.mst", 0);
    ok(hinst, r == ERROR_INVALID_HANDLE, "got %u\n", r);

    r = MsiCreateTransformSummaryInfoA(db, db, "bogus.mst", 0, 0);
    todo_wine ok(hinst, r == ERROR_INSTALL_PACKAGE_OPEN_FAILED ||
                        r == ERROR_INSTALL_PACKAGE_INVALID /* winxp */,
                 "got %u\n", r);

    GetCurrentDirectoryA(sizeof(path), path);
    r = MsiDatabaseExportA(db, "Test", path, "bogus.idt");
    ok(hinst, r == ERROR_INVALID_HANDLE, "got %u\n", r);

    r = MsiDatabaseImportA(db, path, "bogus.idt");
    ok(hinst, r == ERROR_INVALID_HANDLE, "got %u\n", r);

    r = MsiDatabaseCommit(db);
    ok(hinst, r == ERROR_SUCCESS, "got %u\n", r);

    r = MsiDatabaseMergeA(db, db, "MergeErrors");
    ok(hinst, r == ERROR_INVALID_HANDLE, "got %u\n", r);

    r = MsiGetDatabaseState(db);
    ok(hinst, r == MSIDBSTATE_ERROR, "got %u\n", r);

    r = MsiEnableUIPreview(db, &preview);
    ok(hinst, r == ERROR_INVALID_HANDLE, "got %u\n", r);

    sprintf(package_name, "#%lu", db);
    r = MsiOpenPackageA(package_name, &package);
    ok(hinst, r == ERROR_INVALID_HANDLE, "got %u\n", r);

    MsiCloseHandle(db);
}

static void test_view_get_error(MSIHANDLE hinst)
{
    MSIHANDLE db, view, rec;
    char buffer[5];
    MSIDBERROR err;
    DWORD sz;
    UINT r;

    db = MsiGetActiveDatabase(hinst);
    ok(hinst, db, "MsiGetActiveDatabase failed\n");

    r = MsiDatabaseOpenViewA(db, "SELECT * FROM `test2`", &view);
    ok(hinst, !r, "got %u\n", r);

    r = MsiViewExecute(view, 0);
    ok(hinst, !r, "got %u\n", r);

    sz = 0;
    err = MsiViewGetErrorA(0, NULL, &sz);
    todo_wine ok(hinst, err == MSIDBERROR_FUNCTIONERROR, "got %d\n", err);
    ok(hinst, sz == 0, "got size %lu\n", sz);

    err = MsiViewGetErrorA(view, NULL, NULL);
    ok(hinst, err == MSIDBERROR_INVALIDARG, "got %d\n", err);

    sz = 0;
    err = MsiViewGetErrorA(view, NULL, &sz);
    ok(hinst, err == MSIDBERROR_FUNCTIONERROR, "got %d\n", err);
    ok(hinst, sz == 0, "got size %lu\n", sz);

    sz = 0;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    ok(hinst, err == MSIDBERROR_FUNCTIONERROR, "got %d\n", err);
    ok(hinst, !strcmp(buffer, "x"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 0, "got size %lu\n", sz);

    sz = 1;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    ok(hinst, err == MSIDBERROR_NOERROR, "got %d\n", err);
    ok(hinst, !buffer[0], "got \"%s\"\n", buffer);
    ok(hinst, sz == 0, "got size %lu\n", sz);

    rec = MsiCreateRecord(2);
    MsiRecordSetInteger(rec, 1, 1);
    MsiRecordSetInteger(rec, 2, 2);
    r = MsiViewModify(view, MSIMODIFY_VALIDATE_NEW, rec);
    ok(hinst, r == ERROR_INVALID_DATA, "got %u\n", r);

    sz = 2;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    ok(hinst, err == MSIDBERROR_DUPLICATEKEY, "got %d\n", err);
    ok(hinst, !strcmp(buffer, "A"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 1, "got size %lu\n", sz);

    sz = 2;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    todo_wine ok(hinst, err == MSIDBERROR_NOERROR, "got %d\n", err);
    todo_wine ok(hinst, !buffer[0], "got \"%s\"\n", buffer);
    todo_wine ok(hinst, sz == 0, "got size %lu\n", sz);

    r = MsiViewModify(view, MSIMODIFY_VALIDATE_NEW, rec);
    ok(hinst, r == ERROR_INVALID_DATA, "got %u\n", r);

    sz = 1;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    ok(hinst, err == MSIDBERROR_MOREDATA, "got %d\n", err);
    ok(hinst, !buffer[0], "got \"%s\"\n", buffer);
    ok(hinst, sz == 1, "got size %lu\n", sz);

    sz = 1;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    todo_wine ok(hinst, err == MSIDBERROR_NOERROR, "got %d\n", err);
    ok(hinst, !buffer[0], "got \"%s\"\n", buffer);
    todo_wine ok(hinst, sz == 0, "got size %lu\n", sz);

    r = MsiViewModify(view, MSIMODIFY_VALIDATE_NEW, rec);
    ok(hinst, r == ERROR_INVALID_DATA, "got %u\n", r);

    sz = 0;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    ok(hinst, err == MSIDBERROR_FUNCTIONERROR, "got %d\n", err);
    ok(hinst, !strcmp(buffer, "x"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 0, "got size %lu\n", sz);

    sz = 0;
    strcpy(buffer, "x");
    err = MsiViewGetErrorA(view, buffer, &sz);
    ok(hinst, err == MSIDBERROR_FUNCTIONERROR, "got %d\n", err);
    ok(hinst, !strcmp(buffer, "x"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 0, "got size %lu\n", sz);

    MsiCloseHandle(rec);
    MsiCloseHandle(view);
    MsiCloseHandle(db);
}

/* Main test. Anything that doesn't depend on a specific install configuration
 * or have undesired side effects should go here. */
UINT WINAPI main_test(MSIHANDLE hinst)
{
    IUnknown *unk = NULL;
    HRESULT hr;

    /* Test for an MTA apartment */
    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    ok(hinst, hr == S_OK, "CoCreateInstance failed with %08lx\n", hr);

    if (unk) IUnknown_Release(unk);

    /* but ours is uninitialized */
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hinst, hr == S_OK, "got %#lx\n", hr);
    CoUninitialize();

    test_props(hinst);
    test_db(hinst);
    test_doaction(hinst);
    test_targetpath(hinst);
    test_misc(hinst);
    test_feature_states(hinst);
    test_format_record(hinst);
    test_costs(hinst);
    test_invalid_functions(hinst);
    test_view_get_error(hinst);

    return ERROR_SUCCESS;
}

UINT WINAPI test_retval(MSIHANDLE hinst)
{
    char prop[10];
    DWORD len = sizeof(prop);
    UINT retval;

    MsiGetPropertyA(hinst, "TEST_RETVAL", prop, &len);
    sscanf(prop, "%u", &retval);
    return retval;
}

static void append_file(MSIHANDLE hinst, const char *filename, const char *text)
{
    DWORD size;
    HANDLE file = CreateFileA(filename, GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(hinst, file != INVALID_HANDLE_VALUE, "CreateFile failed, error %lu\n", GetLastError());

    SetFilePointer(file, 0, NULL, FILE_END);
    WriteFile(file, text, strlen(text), &size, NULL);
    CloseHandle(file);
}

UINT WINAPI da_immediate(MSIHANDLE hinst)
{
    char prop[300];
    DWORD len = sizeof(prop);

    MsiGetPropertyA(hinst, "TESTPATH", prop, &len);

    append_file(hinst, prop, "one");

    ok(hinst, !MsiGetMode(hinst, MSIRUNMODE_SCHEDULED), "shouldn't be scheduled\n");
    ok(hinst, !MsiGetMode(hinst, MSIRUNMODE_ROLLBACK), "shouldn't be rollback\n");
    ok(hinst, !MsiGetMode(hinst, MSIRUNMODE_COMMIT), "shouldn't be commit\n");

    return ERROR_SUCCESS;
}

UINT WINAPI da_deferred(MSIHANDLE hinst)
{
    char prop[300];
    DWORD len = sizeof(prop);
    LANGID lang;
    UINT r;

    /* Test that we were in fact deferred */
    r = MsiGetPropertyA(hinst, "CustomActionData", prop, &len);
    ok(hinst, r == ERROR_SUCCESS, "got %u\n", r);
    ok(hinst, prop[0], "CustomActionData was empty\n");

    append_file(hinst, prop, "two");

    /* Test available properties */
    len = sizeof(prop);
    r = MsiGetPropertyA(hinst, "ProductCode", prop, &len);
    ok(hinst, r == ERROR_SUCCESS, "got %u\n", r);
    ok(hinst, prop[0], "got %s\n", prop);

    len = sizeof(prop);
    r = MsiGetPropertyA(hinst, "UserSID", prop, &len);
    ok(hinst, r == ERROR_SUCCESS, "got %u\n", r);
    ok(hinst, prop[0], "got %s\n", prop);

    len = sizeof(prop);
    r = MsiGetPropertyA(hinst, "TESTPATH", prop, &len);
    ok(hinst, r == ERROR_SUCCESS, "got %u\n", r);
    todo_wine
    ok(hinst, !prop[0], "got %s\n", prop);

    /* Test modes */
    ok(hinst, MsiGetMode(hinst, MSIRUNMODE_SCHEDULED), "should be scheduled\n");
    ok(hinst, !MsiGetMode(hinst, MSIRUNMODE_ROLLBACK), "shouldn't be rollback\n");
    ok(hinst, !MsiGetMode(hinst, MSIRUNMODE_COMMIT), "shouldn't be commit\n");

    lang = MsiGetLanguage(hinst);
    ok(hinst, lang != ERROR_INVALID_HANDLE, "MsiGetLanguage failed\n");

    return ERROR_SUCCESS;
}

static int global_state;

UINT WINAPI process1(MSIHANDLE hinst)
{
    SetEnvironmentVariableA("MSI_PROCESS_TEST","1");
    global_state++;
    return ERROR_SUCCESS;
}

UINT WINAPI process2(MSIHANDLE hinst)
{
    char env[2] = {0};
    DWORD r = GetEnvironmentVariableA("MSI_PROCESS_TEST", env, sizeof(env));
    ok(hinst, r == 1, "got %lu, error %lu\n", r, GetLastError());
    ok(hinst, !strcmp(env, "1"), "got %s\n", env);
    ok(hinst, !global_state, "got global_state %d\n", global_state);
    return ERROR_SUCCESS;
}

UINT WINAPI async1(MSIHANDLE hinst)
{
    HANDLE event = CreateEventA(NULL, TRUE, FALSE, "wine_msi_async_test");
    HANDLE event2 = CreateEventA(NULL, TRUE, FALSE, "wine_msi_async_test2");
    DWORD r = WaitForSingleObject(event, 10000);
    ok(hinst, !r, "wait timed out\n");
    SetEvent(event2);
    return ERROR_SUCCESS;
}

UINT WINAPI async2(MSIHANDLE hinst)
{
    HANDLE event = CreateEventA(NULL, TRUE, FALSE, "wine_msi_async_test");
    HANDLE event2 = CreateEventA(NULL, TRUE, FALSE, "wine_msi_async_test2");
    DWORD r;
    SetEvent(event);
    r = WaitForSingleObject(event2, 10000);
    ok(hinst, !r, "wait timed out\n");
    return ERROR_SUCCESS;
}

static BOOL pf_exists(const char *file)
{
    char path[MAX_PATH];

    if (FAILED(SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, path)))
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path);
    strcat(path, "\\");
    strcat(path, file);
    return GetFileAttributesA(path) != INVALID_FILE_ATTRIBUTES;
}

UINT WINAPI cf_present(MSIHANDLE hinst)
{
    ok(hinst, pf_exists("msitest\\first"), "folder absent\n");
    ok(hinst, pf_exists("msitest\\second"), "folder absent\n");
    ok(hinst, pf_exists("msitest\\third"), "folder absent\n");
    return ERROR_SUCCESS;
}

UINT WINAPI cf_absent(MSIHANDLE hinst)
{
    ok(hinst, !pf_exists("msitest\\first"), "folder present\n");
    ok(hinst, !pf_exists("msitest\\second"), "folder present\n");
    ok(hinst, !pf_exists("msitest\\third"), "folder present\n");
    return ERROR_SUCCESS;
}

UINT WINAPI file_present(MSIHANDLE hinst)
{
    ok(hinst, pf_exists("msitest\\first\\one.txt"), "file absent\n");
    ok(hinst, pf_exists("msitest\\second\\two.txt"), "file absent\n");
    return ERROR_SUCCESS;
}

UINT WINAPI file_absent(MSIHANDLE hinst)
{
    ok(hinst, !pf_exists("msitest\\first\\one.txt"), "file present\n");
    ok(hinst, !pf_exists("msitest\\second\\two.txt"), "file present\n");
    return ERROR_SUCCESS;
}

UINT WINAPI crs_present(MSIHANDLE hinst)
{
    ok(hinst, pf_exists("msitest\\shortcut.lnk"), "shortcut absent\n");
    return ERROR_SUCCESS;
}

UINT WINAPI crs_absent(MSIHANDLE hinst)
{
    ok(hinst, !pf_exists("msitest\\shortcut.lnk"), "shortcut present\n");
    return ERROR_SUCCESS;
}

UINT WINAPI sds_present(MSIHANDLE hinst)
{
    SC_HANDLE manager, service;
    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    service = OpenServiceA(manager, "TestService3", GENERIC_ALL);
    ok(hinst, !!service, "service absent: %lu\n", GetLastError());
    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return ERROR_SUCCESS;
}

UINT WINAPI sds_absent(MSIHANDLE hinst)
{
    SC_HANDLE manager, service;
    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    service = OpenServiceA(manager, "TestService3", GENERIC_ALL);
    ok(hinst, !service, "service present\n");
    if (service) CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return ERROR_SUCCESS;
}

UINT WINAPI sis_present(MSIHANDLE hinst)
{
    SC_HANDLE manager, service;
    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    service = OpenServiceA(manager, "TestService", GENERIC_ALL);
    ok(hinst, !!service, "service absent: %lu\n", GetLastError());
    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return ERROR_SUCCESS;
}

UINT WINAPI sis_absent(MSIHANDLE hinst)
{
    SC_HANDLE manager, service;
    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    service = OpenServiceA(manager, "TestService", GENERIC_ALL);
    ok(hinst, !service, "service present\n");
    if (service) CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return ERROR_SUCCESS;
}

UINT WINAPI sss_started(MSIHANDLE hinst)
{
    SC_HANDLE manager, service;
    SERVICE_STATUS status;
    BOOL ret;

    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    service = OpenServiceA(manager, "Spooler", SC_MANAGER_ALL_ACCESS);
    ret = QueryServiceStatus(service, &status);
    ok(hinst, ret, "QueryServiceStatus failed: %lu\n", GetLastError());
    ok(hinst, status.dwCurrentState == SERVICE_RUNNING, "got %lu\n", status.dwCurrentState);

    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return ERROR_SUCCESS;
}

UINT WINAPI sss_stopped(MSIHANDLE hinst)
{
    SC_HANDLE manager, service;
    SERVICE_STATUS status;
    BOOL ret;

    manager = OpenSCManagerA(NULL, NULL, SC_MANAGER_ALL_ACCESS);
    service = OpenServiceA(manager, "Spooler", SC_MANAGER_ALL_ACCESS);
    ret = QueryServiceStatus(service, &status);
    ok(hinst, ret, "QueryServiceStatus failed: %lu\n", GetLastError());
    ok(hinst, status.dwCurrentState == SERVICE_STOPPED, "got %lu\n", status.dwCurrentState);

    CloseServiceHandle(service);
    CloseServiceHandle(manager);
    return ERROR_SUCCESS;
}

UINT WINAPI rd_present(MSIHANDLE hinst)
{
    ok(hinst, pf_exists("msitest\\original2.txt"), "file absent\n");
    ok(hinst, pf_exists("msitest\\duplicate.txt"), "file absent\n");
    ok(hinst, !pf_exists("msitest\\original3.txt"), "file present\n");
    ok(hinst, !pf_exists("msitest\\duplicate2.txt"), "file present\n");
    return ERROR_SUCCESS;
}

UINT WINAPI rd_absent(MSIHANDLE hinst)
{
    ok(hinst, !pf_exists("msitest\\original2.txt"), "file present\n");
    ok(hinst, !pf_exists("msitest\\duplicate.txt"), "file present\n");
    ok(hinst, !pf_exists("msitest\\original3.txt"), "file present\n");
    ok(hinst, !pf_exists("msitest\\duplicate2.txt"), "file present\n");
    return ERROR_SUCCESS;
}

UINT WINAPI odbc_present(MSIHANDLE hinst)
{
    int gotdriver = 0, gotdriver2 = 0;
    char buffer[1000], *p;
    WORD len;
    BOOL r;

    buffer[0] = 0;
    len = sizeof(buffer);
    r = SQLGetInstalledDrivers(buffer, sizeof(buffer), &len);
    if (r) ok(hinst, len < sizeof(buffer), "buffer too small\n");
    for (p = buffer; *p; p += strlen(p) + 1)
    {
        if (!strcmp(p, "ODBC test driver"))
            gotdriver = 1;
        if (!strcmp(p, "ODBC test driver2"))
            gotdriver2 = 1;
    }
    ok(hinst, gotdriver, "driver absent\n");
    ok(hinst, gotdriver2, "driver 2 absent\n");
    return ERROR_SUCCESS;
}

UINT WINAPI odbc_absent(MSIHANDLE hinst)
{
    int gotdriver = 0, gotdriver2 = 0;
    char buffer[1000], *p;
    WORD len;
    BOOL r;

    buffer[0] = 0;
    len = sizeof(buffer);
    r = SQLGetInstalledDrivers(buffer, sizeof(buffer), &len);
    if (r) ok(hinst, len < sizeof(buffer), "buffer too small\n");
    for (p = buffer; *p; p += strlen(p) + 1)
    {
        if (!strcmp(p, "ODBC test driver"))
            gotdriver = 1;
        if (!strcmp(p, "ODBC test driver2"))
            gotdriver2 = 1;
    }
    ok(hinst, !gotdriver, "driver present\n");
    ok(hinst, !gotdriver2, "driver 2 present\n");
    return ERROR_SUCCESS;
}

UINT WINAPI mov_present(MSIHANDLE hinst)
{
    ok(hinst, pf_exists("msitest\\canada"), "file absent\n");
    ok(hinst, pf_exists("msitest\\dominica"), "file absent\n");
    return ERROR_SUCCESS;
}

UINT WINAPI mov_absent(MSIHANDLE hinst)
{
    ok(hinst, !pf_exists("msitest\\canada"), "file present\n");
    ok(hinst, !pf_exists("msitest\\dominica"), "file present\n");
    return ERROR_SUCCESS;
}

static void check_reg_str(MSIHANDLE hinst, HKEY key, const char *name, const char *expect)
{
    char value[300];
    DWORD sz;
    LONG res;

    sz = sizeof(value);
    res = RegQueryValueExA(key, name, NULL, NULL, (BYTE *)value, &sz);
    if (expect)
    {
        ok(hinst, !res, "failed to get value \"%s\": %ld\n", name, res);
        ok(hinst, !strcmp(value, expect), "\"%s\": expected \"%s\", got \"%s\"\n",
            name, expect, value);
    }
    else
        ok(hinst, res == ERROR_FILE_NOT_FOUND, "\"%s\": expected missing, got %ld\n",
            name, res);
}

static const char path_dotnet[] = "Software\\Microsoft\\Installer\\Assemblies\\Global";
static const char name_dotnet[] = "Wine.Dotnet.Assembly,processorArchitecture=\"MSIL\","
    "publicKeyToken=\"abcdef0123456789\",version=\"1.0.0.0\",culture=\"neutral\"";

UINT WINAPI pa_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CURRENT_USER, path_dotnet, &key);
    ok(hinst, !res, "got %ld\n", res);
    check_reg_str(hinst, key, name_dotnet, "rcHQPHq?CA@Uv-XqMI1e>Z'q,T*76M@=YEg6My?~]");
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI pa_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CURRENT_USER, path_dotnet, &key);
    ok(hinst, !res || res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    if (!res)
    {
        check_reg_str(hinst, key, name_dotnet, NULL);
        RegCloseKey(key);
    }
    return ERROR_SUCCESS;
}

static const char ppc_key[] = "Software\\Microsoft\\Windows\\CurrentVersion\\"
    "Installer\\UserData\\S-1-5-18\\Components\\CBABC2FDCCB35E749A8944D8C1C098B5";

UINT WINAPI ppc_present(MSIHANDLE hinst)
{
    char expect[MAX_PATH];
    HKEY key;
    UINT r;

    r = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ppc_key, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key);
    ok(hinst, !r, "got %u\n", r);

    if (FAILED(SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, expect)))
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, expect);
    strcat(expect, "\\msitest\\maximus");
    check_reg_str(hinst, key, "84A88FD7F6998CE40A22FB59F6B9C2BB", expect);

    RegCloseKey(key);
    return ERROR_SUCCESS;
}

UINT WINAPI ppc_absent(MSIHANDLE hinst)
{
    HKEY key;
    UINT r;

    r = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ppc_key, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key);
    ok(hinst, r == ERROR_FILE_NOT_FOUND, "got %u\n", r);
    return ERROR_SUCCESS;
}

static const char pub_key[] = "Software\\Microsoft\\Installer\\Components\\0CBCFA296AC907244845745CEEB2F8AA";

UINT WINAPI pub_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CURRENT_USER, pub_key, &key);
    ok(hinst, !res, "got %ld\n", res);
    res = RegQueryValueExA(key, "english.txt", NULL, NULL, NULL, NULL);
    ok(hinst, !res, "got %ld\n", res);
    RegCloseKey(key);
    return ERROR_SUCCESS;
}

UINT WINAPI pub_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CURRENT_USER, pub_key, &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);
    return ERROR_SUCCESS;
}

static const char pf_classkey[] = "Installer\\Features\\84A88FD7F6998CE40A22FB59F6B9C2BB";
static const char pf_userkey[] = "Software\\Microsoft\\Windows\\CurrentVersion\\"
    "Installer\\UserData\\S-1-5-18\\Products\\84A88FD7F6998CE40A22FB59F6B9C2BB\\Features";

UINT WINAPI pf_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, pf_classkey, 0, KEY_READ | KEY_WOW64_64KEY, &key);
    ok(hinst, !res, "got %ld\n", res);
    check_reg_str(hinst, key, "feature", "");
    check_reg_str(hinst, key, "montecristo", "");
    RegCloseKey(key);

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, pf_userkey, 0, KEY_READ | KEY_WOW64_64KEY, &key);
    ok(hinst, !res, "got %ld\n", res);
    check_reg_str(hinst, key, "feature", "VGtfp^p+,?82@JU1j_KE");
    check_reg_str(hinst, key, "montecristo", "VGtfp^p+,?82@JU1j_KE");
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI pf_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, pf_classkey, 0, KEY_READ | KEY_WOW64_64KEY, &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, pf_userkey, 0, KEY_READ | KEY_WOW64_64KEY, &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    return ERROR_SUCCESS;
}

static const char pp_prodkey[] = "Installer\\Products\\84A88FD7F6998CE40A22FB59F6B9C2BB";

UINT WINAPI pp_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, pp_prodkey, 0, KEY_READ | KEY_WOW64_64KEY, &key);
    ok(hinst, !res, "got %ld\n", res);
    check_reg_str(hinst, key, "ProductName", "MSITEST");
    check_reg_str(hinst, key, "PackageCode", "AC75740029052C94DA02821EECD05F2F");
    check_reg_str(hinst, key, "Clients", ":");

    RegCloseKey(key);
    return ERROR_SUCCESS;
}

UINT WINAPI pp_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, pp_prodkey, 0, KEY_READ | KEY_WOW64_64KEY, &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    return ERROR_SUCCESS;
}

UINT WINAPI rci_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, "CLSID\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}",
        0, KEY_READ | KEY_WOW64_32KEY, &key);
    ok(hinst, !res, "got %ld\n", res);
    RegCloseKey(key);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "FileType\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}", &key);
    ok(hinst, !res, "got %ld\n", res);
    RegCloseKey(key);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "AppID\\{CFCC3B38-E683-497D-9AB4-CB40AAFE307F}", &key);
    ok(hinst, !res, "got %ld\n", res);
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI rci_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, "CLSID\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}",
        0, KEY_READ | KEY_WOW64_32KEY, &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "FileType\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}", &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "AppID\\{CFCC3B38-E683-497D-9AB4-CB40AAFE307F}", &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    return ERROR_SUCCESS;
}

UINT WINAPI rei_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, ".extension", &key);
    ok(hinst, !res, "got %ld\n", res);
    RegCloseKey(key);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Prog.Id.1\\shell\\Open\\command", &key);
    ok(hinst, !res, "got %ld\n", res);
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI rei_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, ".extension", &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Prog.Id.1\\shell\\Open\\command", &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    return ERROR_SUCCESS;
}

static const char font_key[] = "Software\\Microsoft\\Windows NT\\CurrentVersion\\Fonts";

UINT WINAPI font_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, font_key, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key);
    ok(hinst, !res, "got %ld\n", res);
    res = RegQueryValueExA(key, "msi test font", NULL, NULL, NULL, NULL);
    ok(hinst, !res, "got %ld\n", res);
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI font_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, font_key, 0, KEY_QUERY_VALUE | KEY_WOW64_64KEY, &key);
    ok(hinst, !res, "got %ld\n", res);
    check_reg_str(hinst, key, "msi test font", NULL);
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI rmi_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "MIME\\Database\\Content Type\\mime/type", &key);
    ok(hinst, !res, "got %ld\n", res);

    return ERROR_SUCCESS;
}

UINT WINAPI rmi_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "MIME\\Database\\Content Type\\mime/type", &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    return ERROR_SUCCESS;
}

static const char rp_key[] = "Software\\Microsoft\\Windows\\CurrentVersion\\"
    "Uninstall\\{7DF88A48-996F-4EC8-A022-BF956F9B2CBB}";

UINT WINAPI rp_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, rp_key, 0, KEY_READ | KEY_WOW64_32KEY, &key);
    ok(hinst, !res, "got %ld\n", res);
    check_reg_str(hinst, key, "DisplayName", "MSITEST");
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI rp_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, rp_key, 0, KEY_READ | KEY_WOW64_32KEY, &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    return ERROR_SUCCESS;
}

UINT WINAPI rpi_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, "CLSID\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}",
        0, KEY_READ | KEY_WOW64_32KEY, &key);
    ok(hinst, !res, "got %ld\n", res);
    RegCloseKey(key);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Class.1", &key);
    ok(hinst, !res, "got %ld\n", res);
    RegCloseKey(key);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Class", &key);
    ok(hinst, !res, "got %ld\n", res);
    RegCloseKey(key);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Class.2", &key);
    ok(hinst, !res, "got %ld\n", res);
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI rpi_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_CLASSES_ROOT, "CLSID\\{110913E7-86D1-4BF3-9922-BA103FCDDDFA}",
        0, KEY_READ | KEY_WOW64_32KEY, &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Class.1", &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Class", &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "Winetest.Class.2", &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    return ERROR_SUCCESS;
}

static const CHAR ru_key[] = "Software\\Microsoft\\Windows\\CurrentVersion\\Installer"
    "\\UserData\\S-1-5-18\\Products\\84A88FD7F6998CE40A22FB59F6B9C2BB\\InstallProperties";

UINT WINAPI ru_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ru_key, 0, KEY_READ | KEY_WOW64_64KEY, &key);
    ok(hinst, !res, "got %ld\n", res);
    check_reg_str(hinst, key, "ProductID", "none");
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI ru_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyExA(HKEY_LOCAL_MACHINE, ru_key, 0, KEY_READ | KEY_WOW64_64KEY, &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    return ERROR_SUCCESS;
}

static const GUID LIBID_register_test =
    {0xeac5166a, 0x9734, 0x4d91, {0x87,0x8f, 0x1d,0xd0,0x23,0x04,0xc6,0x6c}};

UINT WINAPI tl_present(MSIHANDLE hinst)
{
    ITypeLib *tlb;
    HRESULT hr;

    hr = LoadRegTypeLib(&LIBID_register_test, 7, 1, 0, &tlb);
    ok(hinst, hr == S_OK, "got %#lx\n", hr);
    ITypeLib_Release(tlb);

    return ERROR_SUCCESS;
}

UINT WINAPI tl_absent(MSIHANDLE hinst)
{
    ITypeLib *tlb;
    HRESULT hr;

    hr = LoadRegTypeLib(&LIBID_register_test, 7, 1, 0, &tlb);
    ok(hinst, hr == TYPE_E_LIBNOTREGISTERED, "got %#lx\n", hr);

    return ERROR_SUCCESS;
}

UINT WINAPI sr_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "selfreg_test", &key);
    ok(hinst, !res, "got %ld\n", res);
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI sr_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CLASSES_ROOT, "selfreg_test", &key);
    ok(hinst, res == ERROR_FILE_NOT_FOUND, "got %ld\n", res);

    return ERROR_SUCCESS;
}

UINT WINAPI env_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CURRENT_USER, "Environment", &key);
    ok(hinst, !res, "got %ld\n", res);
    check_reg_str(hinst, key, "MSITESTVAR3", "1");
    check_reg_str(hinst, key, "MSITESTVAR4", "1");
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI env_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CURRENT_USER, "Environment", &key);
    ok(hinst, !res, "got %ld\n", res);
    check_reg_str(hinst, key, "MSITESTVAR3", NULL);
    check_reg_str(hinst, key, "MSITESTVAR4", NULL);
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI ini_present(MSIHANDLE hinst)
{
    char path[MAX_PATH], buf[10];
    DWORD len;

    if (FAILED(SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, path)))
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path);
    strcat(path, "\\msitest\\test.ini");

    len = GetPrivateProfileStringA("section1", "key1", NULL, buf, sizeof(buf), path);
    ok(hinst, len == 6, "got %lu\n", len);

    return ERROR_SUCCESS;
}

UINT WINAPI ini_absent(MSIHANDLE hinst)
{
    char path[MAX_PATH], buf[10];
    DWORD len;

    if (FAILED(SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILESX86, NULL, 0, path)))
        SHGetFolderPathA(NULL, CSIDL_PROGRAM_FILES, NULL, 0, path);
    strcat(path, "\\msitest\\test.ini");

    len = GetPrivateProfileStringA("section1", "key1", NULL, buf, sizeof(buf), path);
    ok(hinst, !len, "got %lu\n", len);

    return ERROR_SUCCESS;
}

UINT WINAPI wrv_present(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CURRENT_USER, "msitest", &key);
    ok(hinst, !res, "got %ld\n", res);
    check_reg_str(hinst, key, "sz", "string");
    RegCloseKey(key);

    return ERROR_SUCCESS;
}

UINT WINAPI wrv_absent(MSIHANDLE hinst)
{
    HKEY key;
    LONG res;

    res = RegOpenKeyA(HKEY_CURRENT_USER, "msitest", &key);
    ok(hinst, !res, "got %ld\n", res);
    check_reg_str(hinst, key, "sz", NULL);
    RegCloseKey(key);

    return ERROR_SUCCESS;
}
