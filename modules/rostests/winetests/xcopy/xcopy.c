/*
 * Copyright 2013 Francois Gouget
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

#include <windows.h>

#include "wine/test.h"


static DWORD runcmd(const char* cmd)
{
    STARTUPINFOA si = {sizeof(STARTUPINFOA)};
    PROCESS_INFORMATION pi;
    char* wcmd;
    DWORD rc;

    /* Create a writable copy for CreateProcessA() */
    wcmd = HeapAlloc(GetProcessHeap(), 0, strlen(cmd) + 1);
    strcpy(wcmd, cmd);

    /* On Windows 2003 and older, xcopy.exe fails if stdin is not a console
     * handle, even with '/I /Y' options.
     */
    rc = CreateProcessA(NULL, wcmd, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &pi);
    HeapFree(GetProcessHeap(), 0, wcmd);
    if (!rc)
        return 260;

    rc = WaitForSingleObject(pi.hProcess, 5000);
    if (rc == WAIT_OBJECT_0)
        GetExitCodeProcess(pi.hProcess, &rc);
    else
        TerminateProcess(pi.hProcess, 1);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    return rc;
}

static void test_date_format(void)
{
    DWORD rc;

    rc = runcmd("xcopy /D:20-01-2000 xcopy1 xcopytest");
    ok(rc == 4, "xcopy /D:d-m-y test returned rc=%u\n", rc);
    ok(GetFileAttributesA("xcopytest\\xcopy1") == INVALID_FILE_ATTRIBUTES,
       "xcopy should not have created xcopytest\\xcopy1\n");

    rc = runcmd("xcopy /D:01-20-2000 xcopy1 xcopytest");
    ok(rc == 0, "xcopy /D:m-d-y test failed rc=%u\n", rc);
    ok(GetFileAttributesA("xcopytest\\xcopy1") != INVALID_FILE_ATTRIBUTES,
       "xcopy did not create xcopytest\\xcopy1\n");
    DeleteFileA("xcopytest\\xcopy1");

    rc = runcmd("xcopy /D:1-20-2000 xcopy1 xcopytest");
    ok(rc == 0, "xcopy /D:m-d-y test failed rc=%u\n", rc);
    ok(GetFileAttributesA("xcopytest\\xcopy1") != INVALID_FILE_ATTRIBUTES,
       "xcopy did not create xcopytest\\xcopy1\n");
    DeleteFileA("xcopytest\\xcopy1");
}

static void test_parms_syntax(void)
{
    DWORD rc;

    rc = runcmd("xcopy /H/D:20-01-2000 xcopy1 xcopytest");
    ok(rc == 4, "xcopy /H/D:d-m-y test returned rc=%u\n", rc);
    ok(GetFileAttributesA("xcopytest\\xcopy1") == INVALID_FILE_ATTRIBUTES,
       "xcopy should not have created xcopytest\\xcopy1\n");

    rc = runcmd("xcopy /D:01-20-2000/H xcopy1 xcopytest");
    ok(rc == 0, "xcopy /H/D:m-d-y test failed rc=%u\n", rc);
    ok(GetFileAttributesA("xcopytest\\xcopy1") != INVALID_FILE_ATTRIBUTES,
       "xcopy did not create xcopytest\\xcopy1\n");
    DeleteFileA("xcopytest\\xcopy1");

    /* The following test is commented out as under wine it generates
       a recursively deep directory tree (todo_wine)
    rc = runcmd("xcopy /D:1-20-2000/E xcopy1 xcopytest");
    ok(rc == 0, "xcopy /D:m-d-y/E test failed rc=%u\n", rc);
    ok(GetFileAttributesA("xcopytest\\xcopy1") != INVALID_FILE_ATTRIBUTES,
       "xcopy did not create xcopytest\\xcopy1\n");
    DeleteFileA("xcopytest\\xcopy1"); */

    rc = runcmd("xcopy /D/S xcopytest xcopytest2\\");
    ok(rc == 0, "xcopy /D/S test failed rc=%u\n", rc);
    ok(GetFileAttributesA("xcopytest2") == INVALID_FILE_ATTRIBUTES,
       "xcopy copied empty directory incorrectly\n");

    rc = runcmd("xcopy /D/S/E xcopytest xcopytest2\\");
    ok(rc == 0, "xcopy /D/S/E test failed rc=%u\n", rc);
    ok(GetFileAttributesA("xcopytest2") != INVALID_FILE_ATTRIBUTES,
       "xcopy failed to copy empty directory\n");
    RemoveDirectoryA("xcopytest2");
}

static void test_keep_attributes(void)
{
    DWORD rc;

    SetFileAttributesA("xcopy1", FILE_ATTRIBUTE_READONLY);

    rc = runcmd("xcopy xcopy1 xcopytest");
    ok(rc == 0, "xcopy failed to copy read only file\n");
    ok((GetFileAttributesA("xcopytest\\xcopy1") & FILE_ATTRIBUTE_READONLY) != FILE_ATTRIBUTE_READONLY,
       "xcopy should not have copied file permissions\n");
    SetFileAttributesA("xcopytest\\xcopy1", FILE_ATTRIBUTE_NORMAL);
    DeleteFileA("xcopytest\\xcopy1");

    rc = runcmd("xcopy /K xcopy1 xcopytest");
    ok(rc == 0, "xcopy failed to copy read only file with /k\n");
    ok((GetFileAttributesA("xcopytest\\xcopy1") & FILE_ATTRIBUTE_READONLY) == FILE_ATTRIBUTE_READONLY,
       "xcopy did not keep file permissions\n");
    SetFileAttributesA("xcopytest\\xcopy1", FILE_ATTRIBUTE_NORMAL);
    DeleteFileA("xcopytest\\xcopy1");

    SetFileAttributesA("xcopy1", FILE_ATTRIBUTE_NORMAL);

    }

START_TEST(xcopy)
{
    char tmpdir[MAX_PATH];
    HANDLE hfile;

    GetTempPathA(MAX_PATH, tmpdir);
    SetCurrentDirectoryA(tmpdir);
    trace("%s\n", tmpdir);

    CreateDirectoryA("xcopytest", NULL);
    hfile = CreateFileA("xcopy1", GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
            FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "Failed to create xcopy1 file\n");
    if (hfile == INVALID_HANDLE_VALUE)
    {
        skip("skipping xcopy tests\n");
        return;
    }
    CloseHandle(hfile);

    test_date_format();
    test_parms_syntax();
    test_keep_attributes();

    DeleteFileA("xcopy1");
    RemoveDirectoryA("xcopytest");
}
