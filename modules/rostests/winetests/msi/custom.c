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

#include <stdarg.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#define COBJMACROS
#include <msxml.h>
#include <msi.h>
#include <msiquery.h>

static void ok_(MSIHANDLE hinst, int todo, const char *file, int line, int condition, const char *msg, ...)
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
#define ok(hinst, condition, ...)           ok_(hinst, 0, __FILE__, __LINE__, condition, __VA_ARGS__)
#define todo_wine_ok(hinst, condition, ...) ok_(hinst, 1, __FILE__, __LINE__, condition, __VA_ARGS__)

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
    ok(hinst, sz == strlen(buffer), "'%s': expected %u, got %u\n", prop, strlen(buffer), sz);
    ok(hinst, !strcmp(buffer, expect), "expected '%s', got '%s'\n", expect, buffer);
}

static void test_props(MSIHANDLE hinst)
{
    static const WCHAR booW[] = {'b','o','o',0};
    static const WCHAR xyzW[] = {'x','y','z',0};
    static const WCHAR xyW[] = {'x','y',0};
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
    ok(hinst, sz == 0, "got size %u\n", sz);

    sz = 0;
    strcpy(buffer,"x");
    r = MsiGetPropertyA(hinst, "boo", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "x"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 0, "got size %u\n", sz);

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetPropertyA(hinst, "boo", buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !buffer[0], "got \"%s\"\n", buffer);
    ok(hinst, sz == 0, "got size %u\n", sz);

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
    ok(hinst, sz == 6, "got size %u\n", sz);

    sz = 0;
    strcpy(buffer,"q");
    r = MsiGetPropertyA(hinst, "boo", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "q"), "got \"%s\"\n", buffer);
    todo_wine_ok(hinst, sz == 6, "got size %u\n", sz);

    sz = 1;
    strcpy(buffer,"x");
    r = MsiGetPropertyA(hinst, "boo", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !buffer[0], "got \"%s\"\n", buffer);
    todo_wine_ok(hinst, sz == 6, "got size %u\n", sz);

    sz = 3;
    strcpy(buffer,"x");
    r = MsiGetPropertyA(hinst, "boo", buffer, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "xy"), "got \"%s\"\n", buffer);
    todo_wine_ok(hinst, sz == 6, "got size %u\n", sz);

    sz = 4;
    strcpy(buffer,"x");
    r = MsiGetPropertyA(hinst, "boo", buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !strcmp(buffer, "xyz"), "got \"%s\"\n", buffer);
    ok(hinst, sz == 3, "got size %u\n", sz);

    sz = 0;
    r = MsiGetPropertyW(hinst, booW, NULL, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 3, "got size %u\n", sz);

    sz = 0;
    lstrcpyW(bufferW, booW);
    r = MsiGetPropertyW(hinst, booW, bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !lstrcmpW(bufferW, booW), "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 3, "got size %u\n", sz);

    sz = 1;
    lstrcpyW(bufferW, booW);
    r = MsiGetPropertyW(hinst, booW, bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !bufferW[0], "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 3, "got size %u\n", sz);

    sz = 3;
    lstrcpyW(bufferW, booW);
    r = MsiGetPropertyW(hinst, booW, bufferW, &sz);
    ok(hinst, r == ERROR_MORE_DATA, "got %u\n", r);
    ok(hinst, !lstrcmpW(bufferW, xyW), "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 3, "got size %u\n", sz);

    sz = 4;
    lstrcpyW(bufferW, booW);
    r = MsiGetPropertyW(hinst, booW, bufferW, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !lstrcmpW(bufferW, xyzW), "got %s\n", dbgstr_w(bufferW));
    ok(hinst, sz == 3, "got size %u\n", sz);

    r = MsiSetPropertyA(hinst, "boo", NULL);
    ok(hinst, !r, "got %u\n", r);
    check_prop(hinst, "boo", "");

    sz = 0;
    r = MsiGetPropertyA(hinst, "embednullprop", NULL, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 6, "got size %u\n", sz);

    sz = 4;
    memset(buffer, 0xcc, sizeof(buffer));
    r = MsiGetPropertyA(hinst, "embednullprop", buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == 3, "got size %u\n", sz);
    ok(hinst, !memcmp(buffer, "a\0\0\0\xcc", 5), "wrong data\n");
}

static void test_db(MSIHANDLE hinst)
{
    MSIHANDLE hdb, view, rec, rec2, suminfo;
    char buffer[10];
    DWORD sz;
    UINT r;

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
    ok(hinst, sz == strlen(buffer), "got size %u\n", sz);
    ok(hinst, !strcmp(buffer, "Name"), "got '%s'\n", buffer);

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
    ok(hinst, sz == strlen(buffer), "got size %u\n", sz);
    ok(hinst, !strcmp(buffer, "one"), "got '%s'\n", buffer);

    r = MsiRecordGetInteger(rec2, 2);
    ok(hinst, r == 1, "got %d\n", r);

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
    ok(hinst, sz == strlen(buffer), "got size %u\n", sz);
    ok(hinst, !strcmp(buffer, "two"), "got '%s'\n", buffer);

    r = MsiRecordGetInteger(rec2, 2);
    ok(hinst, r == 2, "got %d\n", r);

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
    ok(hinst, !r, "got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetStringA(rec2, 1, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == strlen(buffer), "got size %u\n", sz);
    ok(hinst, !strcmp(buffer, "two"), "got '%s'\n", buffer);

    r = MsiRecordGetInteger(rec2, 2);
    ok(hinst, r == 2, "got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordReadStream(rec2, 3, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, !memcmp(buffer, "duo", 3), "wrong data\n");

    r = MsiCloseHandle(rec2);
    ok(hinst, !r, "got %u\n", r);

    r = MsiViewFetch(view, &rec2);
    ok(hinst, r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    ok(hinst, !rec2, "got %u\n", rec2);

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
    ok(hinst, r == 1, "got %d\n", r);

    r = MsiCloseHandle(rec2);
    ok(hinst, !r, "got %u\n", r);

    r = MsiViewFetch(view, &rec2);
    ok(hinst, r == ERROR_NO_MORE_ITEMS, "got %u\n", r);
    ok(hinst, !rec2, "got %u\n", rec2);

    r = MsiCloseHandle(rec);
    ok(hinst, !r, "got %u\n", r);

    r = MsiCloseHandle(view);
    ok(hinst, !r, "got %u\n", r);

    /* test MsiDatabaseGetPrimaryKeys() */
    r = MsiDatabaseGetPrimaryKeysA(hdb, "Test", &rec);
    ok(hinst, !r, "got %u\n", r);

    r = MsiRecordGetFieldCount(rec);
    ok(hinst, r == 1, "got %d\n", r);

    sz = sizeof(buffer);
    r = MsiRecordGetStringA(rec, 0, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == strlen(buffer), "got size %u\n", sz);
    ok(hinst, !strcmp(buffer, "Test"), "got '%s'\n", buffer);

    sz = sizeof(buffer);
    r = MsiRecordGetStringA(rec, 1, buffer, &sz);
    ok(hinst, !r, "got %u\n", r);
    ok(hinst, sz == strlen(buffer), "got size %u\n", sz);
    ok(hinst, !strcmp(buffer, "Name"), "got '%s'\n", buffer);

    r = MsiCloseHandle(rec);
    ok(hinst, !r, "got %u\n", r);

    r = MsiGetSummaryInformationA(hdb, NULL, 1, NULL);
    ok(hinst, r == ERROR_INVALID_PARAMETER, "got %u\n", r);

    r = MsiGetSummaryInformationA(hdb, NULL, 1, &suminfo);
    ok(hinst, !r, "got %u\n", r);

    r = MsiCloseHandle(suminfo);
    ok(hinst, !r, "got %u\n", r);

    r = MsiCloseHandle(hdb);
    ok(hinst, !r, "got %u\n", r);
}

/* Main test. Anything that doesn't depend on a specific install configuration
 * or have undesired side effects should go here. */
UINT WINAPI main_test(MSIHANDLE hinst)
{
    UINT res;
    IUnknown *unk = NULL;
    HRESULT hr;

    /* Test for an MTA apartment */
    hr = CoCreateInstance(&CLSID_XMLDocument, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    todo_wine_ok(hinst, hr == S_OK, "CoCreateInstance failed with %08x\n", hr);

    if (unk) IUnknown_Release(unk);

    /* but ours is uninitialized */
    hr = CoInitializeEx(NULL, COINIT_APARTMENTTHREADED);
    ok(hinst, hr == S_OK, "got %#x\n", hr);
    CoUninitialize();

    /* Test MsiGetDatabaseState() */
    res = MsiGetDatabaseState(hinst);
    todo_wine_ok(hinst, res == MSIDBSTATE_ERROR, "expected MSIDBSTATE_ERROR, got %u\n", res);

    test_props(hinst);
    test_db(hinst);

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
    ok(hinst, file != INVALID_HANDLE_VALUE, "CreateFile failed, error %u\n", GetLastError());

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
    todo_wine_ok(hinst, !prop[0], "got %s\n", prop);

    /* Test modes */
    ok(hinst, MsiGetMode(hinst, MSIRUNMODE_SCHEDULED), "should be scheduled\n");
    ok(hinst, !MsiGetMode(hinst, MSIRUNMODE_ROLLBACK), "shouldn't be rollback\n");
    ok(hinst, !MsiGetMode(hinst, MSIRUNMODE_COMMIT), "shouldn't be commit\n");

    lang = MsiGetLanguage(hinst);
    ok(hinst, lang != ERROR_INVALID_HANDLE, "MsiGetLanguage failed\n");

    return ERROR_SUCCESS;
}
