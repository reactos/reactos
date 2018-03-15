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
#include <objbase.h>
#include <unknwn.h>
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


/* Main test. Anything that doesn't depend on a specific install configuration
 * or have undesired side effects should go here. */
UINT WINAPI main_test(MSIHANDLE hinst)
{
    UINT res;
    IUnknown *unk = NULL;
    HRESULT hres;

    /* Test for an MTA apartment */
    hres = CoCreateInstance(&CLSID_Picture_Metafile, NULL, CLSCTX_INPROC_SERVER, &IID_IUnknown, (void **)&unk);
    todo_wine_ok(hinst, hres == S_OK, "CoCreateInstance failed with %08x\n", hres);

    if (unk) IUnknown_Release(unk);

    /* Test MsiGetDatabaseState() */
    res = MsiGetDatabaseState(hinst);
    todo_wine_ok(hinst, res == MSIDBSTATE_ERROR, "expected MSIDBSTATE_ERROR, got %u\n", res);

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
