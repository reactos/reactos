/*
 * Unit tests for disk space functions
 *
 * Copyright 2010 Andrew Nguyen
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

#include "windef.h"
#include "winbase.h"
#include "winnls.h"
#include "winuser.h"
#include "winreg.h"
#include "setupapi.h"

#include "wine/test.h"

#define STD_HEADER "[Version]\r\nSignature=\"$CHICAGO$\"\r\n"

static inline const char* debugstr_longlong(ULONGLONG ll)
{
    static char string[17];
    if (sizeof(ll) > sizeof(unsigned long) && ll >> 32)
        sprintf(string, "%lx%08lx", (unsigned long)(ll >> 32), (unsigned long)ll);
    else
        sprintf(string, "%lx", (unsigned long)ll);
    return string;
}

/* create a new file with specified contents and open it */
static HINF inf_open_file_content(const char * tmpfilename, const char *data, UINT *err_line)
{
    DWORD res;
    HANDLE handle = CreateFileA(tmpfilename, GENERIC_READ|GENERIC_WRITE,
                                FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0);
    if (handle == INVALID_HANDLE_VALUE) return 0;
    if (!WriteFile( handle, data, strlen(data), &res, NULL )) trace( "write error\n" );
    CloseHandle( handle );
    return SetupOpenInfFileA( tmpfilename, 0, INF_STYLE_WIN4, err_line );
}

static void test_SetupCreateDiskSpaceListA(void)
{
    HDSKSPC ret;

    ret = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(ret != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    ret = SetupCreateDiskSpaceListA(NULL, 0, SPDSL_IGNORE_DISK);
    ok(ret != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListA(NULL, 0, ~0U);
    ok(ret == NULL ||
       broken(ret != NULL), /* NT4/Win9x/Win2k */
       "Expected SetupCreateDiskSpaceListA to return NULL, got %p\n", ret);
    if (!ret)
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
           GetLastError());
    else
        ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListA(NULL, 0xdeadbeef, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListA to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win9x/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListA((void *)0xdeadbeef, 0, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListA to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win9x/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListA((void *)0xdeadbeef, 0xdeadbeef, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListA to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win9x/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());
}

static void test_SetupCreateDiskSpaceListW(void)
{
    HDSKSPC ret;

    ret = SetupCreateDiskSpaceListW(NULL, 0, 0);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetupCreateDiskSpaceListW is not implemented\n");
        return;
    }
    ok(ret != NULL,
       "Expected SetupCreateDiskSpaceListW to return a valid handle, got NULL\n");

    ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    ret = SetupCreateDiskSpaceListW(NULL, 0, SPDSL_IGNORE_DISK);
    ok(ret != NULL,
       "Expected SetupCreateDiskSpaceListW to return a valid handle, got NULL\n");

    ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListW(NULL, 0, ~0U);
    ok(ret == NULL ||
       broken(ret != NULL), /* NT4/Win2k */
       "Expected SetupCreateDiskSpaceListW to return NULL, got %p\n", ret);
    if (!ret)
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
           GetLastError());
    else
        ok(SetupDestroyDiskSpaceList(ret), "Expected SetupDestroyDiskSpaceList to succeed\n");

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListW(NULL, 0xdeadbeef, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListW to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListW((void *)0xdeadbeef, 0, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListW to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupCreateDiskSpaceListW((void *)0xdeadbeef, 0xdeadbeef, 0);
    ok(ret == NULL,
       "Expected SetupCreateDiskSpaceListW to return NULL, got %p\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == 0xdeadbeef), /* NT4/Win2k */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());
}

static void test_SetupDuplicateDiskSpaceListA(void)
{
    HDSKSPC handle, duplicate;

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(NULL, NULL, 0, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(NULL, (void *)0xdeadbeef, 0, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(NULL, NULL, 0xdeadbeef, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
      "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(NULL, NULL, 0, ~0U);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    handle = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    if (!handle)
    {
        skip("Failed to create a disk space handle\n");
        return;
    }

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(handle, (void *)0xdeadbeef, 0, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(handle, NULL, 0xdeadbeef, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(handle, NULL, 0, SPDSL_IGNORE_DISK);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListA(handle, NULL, 0, ~0U);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    duplicate = SetupDuplicateDiskSpaceListA(handle, NULL, 0, 0);
    ok(duplicate != NULL, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(duplicate != handle,
       "Expected new handle (%p) to be different from the old handle (%p)\n", duplicate, handle);

    ok(SetupDestroyDiskSpaceList(duplicate), "Expected SetupDestroyDiskSpaceList to succeed\n");
    ok(SetupDestroyDiskSpaceList(handle), "Expected SetupDestroyDiskSpaceList to succeed\n");
}

static void test_SetupDuplicateDiskSpaceListW(void)
{
    HDSKSPC handle, duplicate;

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(NULL, NULL, 0, 0);
    if (!duplicate && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetupDuplicateDiskSpaceListW is not available\n");
        return;
    }
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(NULL, (void *)0xdeadbeef, 0, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(NULL, NULL, 0xdeadbeef, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(NULL, NULL, 0, ~0U);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    handle = SetupCreateDiskSpaceListW(NULL, 0, 0);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListW to return a valid handle, got NULL\n");

    if (!handle)
    {
        skip("Failed to create a disk space handle\n");
        return;
    }

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(handle, (void *)0xdeadbeef, 0, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(handle, NULL, 0xdeadbeef, 0);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(handle, NULL, 0, SPDSL_IGNORE_DISK);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    duplicate = SetupDuplicateDiskSpaceListW(handle, NULL, 0, ~0U);
    ok(!duplicate, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    duplicate = SetupDuplicateDiskSpaceListW(handle, NULL, 0, 0);
    ok(duplicate != NULL, "Expected SetupDuplicateDiskSpaceList to return NULL, got %p\n", duplicate);
    ok(duplicate != handle,
       "Expected new handle (%p) to be different from the old handle (%p)\n", duplicate, handle);

    ok(SetupDestroyDiskSpaceList(duplicate), "Expected SetupDestroyDiskSpaceList to succeed\n");
    ok(SetupDestroyDiskSpaceList(handle), "Expected SetupDestroyDiskSpaceList to succeed\n");
}

static LONGLONG get_file_size(char *path)
{
    HANDLE file;
    LARGE_INTEGER size;

    file = CreateFileA(path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE,
                       NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (file == INVALID_HANDLE_VALUE) return 0;

    if (!GetFileSizeEx(file, &size))
        size.QuadPart = 0;

    CloseHandle(file);
    return size.QuadPart;
}

static void test_SetupQuerySpaceRequiredOnDriveA(void)
{
    BOOL ret;
    HDSKSPC handle;
    LONGLONG space;
    char windir[MAX_PATH];
    char drive[3];
    char tmp[MAX_PATH];
    LONGLONG size;

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveA(NULL, NULL, NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(NULL, NULL, &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
    "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
    GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveA(NULL, "", NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(NULL, "", &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n",
       GetLastError());

    handle = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveA(handle, NULL, NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_INVALID_DRIVE, /* Win9x */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, NULL, &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_INVALID_DRIVE, /* Win9x */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveA(handle, "", NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_DRIVE,
       "Expected GetLastError() to return ERROR_INVALID_DRIVE, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, "", &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveA to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_DRIVE,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    GetWindowsDirectoryA(windir, MAX_PATH);
    drive[0] = windir[0]; drive[1] = windir[1]; drive[2] = 0;

    snprintf(tmp, MAX_PATH, "%c:\\wine-test-should-not-exist.txt", drive[0]);
    ret = SetupAddToDiskSpaceListA(handle, tmp, 0x100000, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");

    space = 0;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret, "Expected SetupQuerySpaceRequiredOnDriveA to succeed\n");
    ok(space == 0x100000, "Expected 0x100000 as required space, got %s\n", debugstr_longlong(space));

    /* adding the same file again doesn't sum up the size */
    ret = SetupAddToDiskSpaceListA(handle, tmp, 0x200000, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");

    space = 0;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret, "Expected SetupQuerySpaceRequiredOnDriveA to succeed\n");
    ok(space == 0x200000, "Expected 0x200000 as required space, got %s\n", debugstr_longlong(space));

    /* the device doesn't need to exist */
    snprintf(tmp, MAX_PATH, "F:\\wine-test-should-not-exist.txt");
    ret = SetupAddToDiskSpaceListA(handle, tmp, 0x200000, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");

    ret = SetupQuerySpaceRequiredOnDriveA(handle, "F:", &space, NULL, 0);
    ok(ret, "Expected SetupQuerySpaceRequiredOnDriveA to succeed\n");
    ok(space == 0x200000, "Expected 0x200000 as required space, got %s\n", debugstr_longlong(space));

    snprintf(tmp, MAX_PATH, "F:\\wine-test-should-not-exist.txt");
    ret = SetupAddToDiskSpaceListA(handle, tmp, 0x200000, FILEOP_DELETE, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");

    ret = SetupQuerySpaceRequiredOnDriveA(handle, "F:", &space, NULL, 0);
    ok(ret, "Expected SetupQuerySpaceRequiredOnDriveA to succeed\n");
    ok(space == 0x200000, "Expected 0x200000 as required space, got %s\n", debugstr_longlong(space));

    ok(SetupDestroyDiskSpaceList(handle),
       "Expected SetupDestroyDiskSpaceList to succeed\n");

    handle = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    /* the real size is subtracted unless SPDSL_IGNORE_DISK is specified */
    snprintf(tmp, MAX_PATH, "%s\\regedit.exe", windir);

    size = get_file_size(tmp);
    ret = SetupAddToDiskSpaceListA(handle, tmp, size, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");
    space = 0;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret, "Expected SetupQuerySpaceRequiredOnDriveA to succeed\n");
    ok(space == 0 || broken(space == -0x5000) || broken(space == -0x7000),
       "Expected 0x0 as required space, got %s\n", debugstr_longlong(space));

    ret = SetupAddToDiskSpaceListA(handle, tmp, size + 0x100000, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret, "Expected SetupQuerySpaceRequiredOnDriveA to succeed\n");
    ok(space == 0x100000 || broken(space == 0xf9000) || broken(space == 0xfb000),
       "Expected 0x100000 as required space, got %s\n", debugstr_longlong(space));

    ret = SetupAddToDiskSpaceListA(handle, tmp, size - 0x1000, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret, "Expected SetupQuerySpaceRequiredOnDriveA to succeed\n");
    ok(space == -0x1000 || broken(space == -0x6000) || broken(space == -0x8000),
       "Expected -0x1000 as required space, got %s\n", debugstr_longlong(space));

    ok(SetupDestroyDiskSpaceList(handle),
       "Expected SetupDestroyDiskSpaceList to succeed\n");

    /* test FILEOP_DELETE, then FILEOP_COPY */
    handle = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    ret = SetupAddToDiskSpaceListA(handle, tmp, size, FILEOP_DELETE, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");
    ret = SetupAddToDiskSpaceListA(handle, tmp, size, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");

    space = 0;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret, "Expected SetupQuerySpaceRequiredOnDriveA to succeed\n");
    ok(space == 0 || broken(space == -0x5000) || broken(space == -0x7000),
       "Expected 0x0 as required space, got %s\n", debugstr_longlong(space));

    ok(SetupDestroyDiskSpaceList(handle),
       "Expected SetupDestroyDiskSpaceList to succeed\n");

    /* test FILEOP_COPY, then FILEOP_DELETE */
    handle = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    ret = SetupAddToDiskSpaceListA(handle, tmp, size, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");
    ret = SetupAddToDiskSpaceListA(handle, tmp, size, FILEOP_DELETE, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");

    space = 0;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret, "Expected SetupQuerySpaceRequiredOnDriveA to succeed\n");
    ok(space == 0 || broken(space == -0x5000) || broken(space == -0x7000),
       "Expected 0x0 as required space, got %s\n", debugstr_longlong(space));

    ok(SetupDestroyDiskSpaceList(handle),
       "Expected SetupDestroyDiskSpaceList to succeed\n");

    /* test FILEOP_DELETE without SPDSL_IGNORE_DISK */
    handle = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    ret = SetupAddToDiskSpaceListA(handle, tmp, size, FILEOP_DELETE, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");
    space = 0;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret, "Expected SetupQuerySpaceRequiredOnDriveA to succeed\n");
    ok(space <= -size, "Expected space <= -size, got %s\n", debugstr_longlong(space));

    ok(SetupDestroyDiskSpaceList(handle),
       "Expected SetupDestroyDiskSpaceList to succeed\n");

    /* test FILEOP_COPY and FILEOP_DELETE with SPDSL_IGNORE_DISK */
    handle = SetupCreateDiskSpaceListA(NULL, 0, SPDSL_IGNORE_DISK);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    ret = SetupAddToDiskSpaceListA(handle, tmp, size, FILEOP_DELETE, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");
    space = 0;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret, "Expected SetupQuerySpaceRequiredOnDriveA to succeed\n");
    ok(space == 0, "Expected size = 0, got %s\n", debugstr_longlong(space));

    ret = SetupAddToDiskSpaceListA(handle, tmp, size, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");
    ret = SetupAddToDiskSpaceListA(handle, tmp, size, FILEOP_DELETE, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");

    space = 0;
    ret = SetupQuerySpaceRequiredOnDriveA(handle, drive, &space, NULL, 0);
    ok(ret, "Expected SetupQuerySpaceRequiredOnDriveA to succeed\n");
    ok(space >= size, "Expected size >= %s\n", debugstr_longlong(space));

    ok(SetupDestroyDiskSpaceList(handle),
       "Expected SetupDestroyDiskSpaceList to succeed\n");
}

static void test_SetupQuerySpaceRequiredOnDriveW(void)
{
    static const WCHAR emptyW[] = {0};

    BOOL ret;
    HDSKSPC handle;
    LONGLONG space;

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveW(NULL, NULL, NULL, NULL, 0);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetupQuerySpaceRequiredOnDriveW is not available\n");
        return;
    }
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveW(NULL, NULL, &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveW(NULL, emptyW, NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveW(NULL, emptyW, &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n",
       GetLastError());

    handle = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(handle != NULL,
       "Expected SetupCreateDiskSpaceListA to return a valid handle, got NULL\n");

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveW(handle, NULL, NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_INVALID_DRIVE, /* NT4/Win2k/XP/Win2k3 */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveW(handle, NULL, &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER ||
       GetLastError() == ERROR_INVALID_DRIVE, /* NT4/Win2k/XP/Win2k3 */
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetupQuerySpaceRequiredOnDriveW(handle, emptyW, NULL, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_DRIVE,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    space = 0xdeadbeef;
    ret = SetupQuerySpaceRequiredOnDriveW(handle, emptyW, &space, NULL, 0);
    ok(!ret, "Expected SetupQuerySpaceRequiredOnDriveW to return FALSE, got %d\n", ret);
    ok(space == 0xdeadbeef, "Expected output space parameter to be untouched\n");
    ok(GetLastError() == ERROR_INVALID_DRIVE,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n",
       GetLastError());

    ok(SetupDestroyDiskSpaceList(handle),
       "Expected SetupDestroyDiskSpaceList to succeed\n");
}

static void test_SetupAddToDiskSpaceListA(void)
{
    HDSKSPC handle;
    BOOL ret;

    ret = SetupAddToDiskSpaceListA(NULL, "C:\\some-file.dat", 0, FILEOP_COPY, 0, 0);
    ok(!ret, "Expected SetupAddToDiskSpaceListA to return FALSE, got %d\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "Expected GetLastError() to return ERROR_INVALID_HANDLE, got %lu\n", GetLastError());

    handle = SetupCreateDiskSpaceListA(NULL, 0, 0);
    ok(handle != NULL,"Expected SetupCreateDiskSpaceListA to return a valid handle\n");

    ret = SetupAddToDiskSpaceListA(handle, NULL, 0, FILEOP_COPY, 0, 0);
    ok(ret || broken(!ret) /* >= Vista */, "Expected SetupAddToDiskSpaceListA to succeed\n");

    ret = SetupAddToDiskSpaceListA(handle, "C:\\some-file.dat", -20, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");

    ret = SetupAddToDiskSpaceListA(handle, "C:\\some-file.dat", 0, FILEOP_RENAME, 0, 0);
    ok(!ret, "Expected SetupAddToDiskSpaceListA to return FALSE\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected GetLastError() to return ERROR_INVALID_PARAMETER, got %lu\n", GetLastError());

    ret = SetupAddToDiskSpaceListA(handle, NULL, 0, FILEOP_RENAME, 0, 0);
    ok(ret || broken(!ret) /* >= Vista */, "Expected SetupAddToDiskSpaceListA to succeed\n");

    ret = SetupAddToDiskSpaceListA(NULL, NULL, 0, FILEOP_RENAME, 0, 0);
    ok(ret || broken(!ret) /* >= Vista */, "Expected SetupAddToDiskSpaceListA to succeed\n");

    ok(SetupDestroyDiskSpaceList(handle),
       "Expected SetupDestroyDiskSpaceList to succeed\n");
}

static void test_SetupQueryDrivesInDiskSpaceListA(void)
{
    char buffer[MAX_PATH];
    HDSKSPC handle;
    DWORD size;
    BOOL ret;

    handle = SetupCreateDiskSpaceListA(NULL, 0, SPDSL_IGNORE_DISK);
    ok(handle != NULL,"Expected SetupCreateDiskSpaceListA to return a valid handle\n");

    ret = SetupQueryDrivesInDiskSpaceListA(handle, NULL, 0, NULL);
    ok(ret, "Expected SetupQueryDrivesInDiskSpaceListA to succeed\n");

    size = 0;
    ret = SetupQueryDrivesInDiskSpaceListA(handle, NULL, 0, &size);
    ok(ret, "Expected SetupQueryDrivesInDiskSpaceListA to succeed\n");
    ok(size == 1, "Expected size 1, got %lu\n", size);

    ret = SetupAddToDiskSpaceListA(handle, "F:\\random-file.dat", 0, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");

    ret = SetupAddToDiskSpaceListA(handle, "G:\\random-file.dat", 0, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");

    ret = SetupAddToDiskSpaceListA(handle, "G:\\random-file2.dat", 0, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");

    ret = SetupAddToDiskSpaceListA(handle, "X:\\random-file.dat", 0, FILEOP_COPY, 0, 0);
    ok(ret, "Expected SetupAddToDiskSpaceListA to succeed\n");

    size = 0;
    ret = SetupQueryDrivesInDiskSpaceListA(handle, NULL, 0, &size);
    ok(ret, "Expected SetupQueryDrivesInDiskSpaceListA to succeed\n");
    ok(size == 10, "Expected size 10, got %lu\n", size);

    size = 0;
    ret = SetupQueryDrivesInDiskSpaceListA(handle, buffer, 0, &size);
    ok(!ret, "Expected SetupQueryDrivesInDiskSpaceListA to fail\n");
    ok(size == 4, "Expected size 4, got %lu\n", size);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %lu\n", GetLastError());

    size = 0;
    ret = SetupQueryDrivesInDiskSpaceListA(handle, buffer, 4, &size);
    ok(!ret, "Expected SetupQueryDrivesInDiskSpaceListA to fail\n");
    ok(size == 7, "Expected size 7, got %lu\n", size);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %lu\n", GetLastError());

    size = 0;
    ret = SetupQueryDrivesInDiskSpaceListA(handle, buffer, 7, &size);
    ok(!ret, "Expected SetupQueryDrivesInDiskSpaceListA to fail\n");
    ok(size == 10, "Expected size 10, got %lu\n", size);
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "Expected GetLastError() to return ERROR_INSUFFICIENT_BUFFER, got %lu\n", GetLastError());

    size = 0;
    memset(buffer, 0xff, sizeof(buffer));
    ret = SetupQueryDrivesInDiskSpaceListA(handle, buffer, sizeof(buffer), &size);
    ok(ret, "Expected SetupQueryDrivesInDiskSpaceListA to succeed\n");
    ok(size == 10, "Expected size 10, got %lu\n", size);
    ok(!memcmp("f:\0g:\0x:\0\0", buffer, 10), "Device list does not match\n");

    memset(buffer, 0xff, sizeof(buffer));
    ret = SetupQueryDrivesInDiskSpaceListA(handle, buffer, sizeof(buffer), NULL);
    ok(ret, "Expected SetupQueryDrivesInDiskSpaceListA to succeed\n");
    ok(!memcmp("f:\0g:\0x:\0\0", buffer, 10), "Device list does not match\n");

    ok(SetupDestroyDiskSpaceList(handle),
       "Expected SetupDestroyDiskSpaceList to succeed\n");
}

struct device_usage
{
    const char *dev;
    LONGLONG usage;
};

struct section
{
    const char *name;
    UINT fileop;
    BOOL result;
    DWORD error_code;
};

static const struct
{
    const char *data;
    struct section sections[2];
    const char *devices;
    int device_length;
    struct device_usage usage[2];
}
section_test[] =
{
    /* 0 */
    {STD_HEADER "[a]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\r\n",
     {{"a", FILEOP_COPY, TRUE, 0}, {NULL, 0, TRUE, 0}},
     "c:\00", sizeof("c:\00"), {{"c:", 4096}, {NULL, 0}}},
    /* 1 */
    {STD_HEADER "[a]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\r\n",
     {{"a", FILEOP_DELETE, TRUE, 0}, {NULL, 0, TRUE, 0}},
     "c:\00", sizeof("c:\00"), {{"c:", 0}, {NULL, 0}}},
    /* 2 */
    {STD_HEADER "[a]\ntest,,,\n\r\n",
     {{"a", FILEOP_COPY, FALSE, ERROR_LINE_NOT_FOUND}, {NULL, 0, TRUE, 0}},
     "", sizeof(""), {{NULL, 0}, {NULL, 0}}},
    /* 3 */
    {STD_HEADER "[a]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\n[DestinationDirs]\nDefaultDestDir=-1,F:\\test\r\n",
     {{"a", FILEOP_COPY, TRUE, 0}, {NULL, 0, TRUE, 0}},
     "f:\00", sizeof("f:\00"), {{"f:", 4096}, {NULL, 0}}},
    /* 4 */
    {STD_HEADER "[a]\ntest,test2,,\n[SourceDisksFiles]\ntest2=1,,4096\r\n",
     {{"a", FILEOP_COPY, TRUE, 0}, {NULL, 0, TRUE, 0}},
     "c:\00", sizeof("c:\00"), {{"c:", 4096}, {NULL, 0}}},
    /* 5 */
    {STD_HEADER "[a]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\r\n",
     {{"b", FILEOP_COPY, FALSE, ERROR_SECTION_NOT_FOUND}, {NULL, 0, TRUE, 0}},
     "", sizeof(""), {{NULL, 0}, {NULL, 0}}},
    /* 6 */
    {STD_HEADER "[a]\ntest,,,\n[b]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\r\n",
     {{"a", FILEOP_COPY, TRUE, 0}, {"b", FILEOP_COPY, TRUE, 0}},
     "c:\00", sizeof("c:\00"), {{"c:", 4096}, {NULL, 0}}},
    /* 7 */
    {STD_HEADER "[a]\ntest,,,\n[b]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\n[DestinationDirs]\nb=-1,F:\\test\r\n",
     {{"a", FILEOP_COPY, TRUE, 0}, {"b", FILEOP_COPY, TRUE, 0}},
     "c:\00f:\00", sizeof("c:\00f:\00"), {{"c:", 4096}, {"f:", 4096}}},
    /* 8 */
    {STD_HEADER "[a]\ntest,test1,,\n[b]\ntest,test2,,\n[SourceDisksFiles]\ntest1=1,,4096\ntest2=1,,8192\r\n",
     {{"a", FILEOP_COPY, TRUE, 0}, {"b", FILEOP_COPY, TRUE, 0}},
     "c:\00", sizeof("c:\00"), {{"c:", 8192}, {NULL, 0}}},
    /* 9 */
    {STD_HEADER "[a]\ntest1,test,,\n[b]\ntest2,test,,\n[SourceDisksFiles]\ntest=1,,4096\r\n",
     {{"a", FILEOP_COPY, TRUE, 0}, {"b", FILEOP_COPY, TRUE, 0}},
     "c:\00", sizeof("c:\00"), {{"c:", 8192}, {NULL, 0}}},
};

static void test_SetupAddSectionToDiskSpaceListA(void)
{
    char tmp[MAX_PATH];
    char tmpfilename[MAX_PATH];
    char buffer[MAX_PATH];
    HDSKSPC diskspace;
    UINT err_line;
    LONGLONG space;
    BOOL ret;
    int i, j;
    HINF inf;

    if (!GetTempPathA(MAX_PATH, tmp))
    {
        win_skip("GetTempPath failed with error %lu\n", GetLastError());
        return;
    }

    if (!GetTempFileNameA(tmp, "inftest", 0, tmpfilename))
    {
        win_skip("GetTempFileNameA failed with error %lu\n", GetLastError());
        return;
    }

    inf = inf_open_file_content(tmpfilename, STD_HEADER "[a]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\r\n", &err_line);
    ok(!!inf, "Failed to open inf file (%lu, line %d)\n", GetLastError(), err_line);

    diskspace = SetupCreateDiskSpaceListA(NULL, 0, SPDSL_IGNORE_DISK);
    ok(diskspace != NULL,"Expected SetupCreateDiskSpaceListA to return a valid handle\n");

    ret = SetupAddSectionToDiskSpaceListA(diskspace, NULL, NULL, "a", FILEOP_COPY, 0, 0);
    ok(!ret, "Expected SetupAddSectionToDiskSpaceListA to fail\n");
    ok(GetLastError() == ERROR_SECTION_NOT_FOUND, "Expected ERROR_SECTION_NOT_FOUND as error, got %lu\n",
       GetLastError());

    ret = SetupAddSectionToDiskSpaceListA(NULL, inf, NULL, "a", FILEOP_COPY, 0, 0);
    ok(!ret, "Expected SetupAddSectionToDiskSpaceListA to fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE as error, got %lu\n",
       GetLastError());

    ret = SetupAddSectionToDiskSpaceListA(NULL, inf, NULL, "b", FILEOP_COPY, 0, 0);
    ok(!ret, "Expected SetupAddSectionToDiskSpaceListA to fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE as error, got %lu\n",
       GetLastError());

    ret = SetupAddSectionToDiskSpaceListA(diskspace, inf, NULL, "a", 0, 0, 0);
    ok(ret, "Expected SetupAddSectionToDiskSpaceListA to succeed (%lu)\n", GetLastError());

    ok(SetupDestroyDiskSpaceList(diskspace),
       "Expected SetupDestroyDiskSpaceList to succeed\n");

    for (i = 0; i < sizeof(section_test) / sizeof(section_test[0]); i++)
    {
        err_line = 0;

        inf = inf_open_file_content(tmpfilename, section_test[i].data, &err_line);
        ok(!!inf, "test %d: Failed to open inf file (%lu, line %d)\n", i, GetLastError(), err_line);
        if (!inf) continue;

        diskspace = SetupCreateDiskSpaceListA(NULL, 0, SPDSL_IGNORE_DISK);
        ok(diskspace != NULL, "Expected SetupCreateDiskSpaceListA to return a valid handle\n");

        for (j = 0; j < 2; j++)
        {
            const struct section *section = &section_test[i].sections[j];
            if (!section->name)
                continue;

            SetLastError(0xdeadbeef);
            ret = SetupAddSectionToDiskSpaceListA(diskspace, inf, NULL, section->name, section->fileop, 0, 0);
            if (section->result)
                ok(ret, "test %d: Expected adding section %d to succeed (%lu)\n", i, j, GetLastError());
            else
            {
                ok(!ret, "test %d: Expected adding section %d to fail\n", i, j);
                ok(GetLastError() == section->error_code, "test %d: Expected %lu as error, got %lu\n",
                   i, section->error_code, GetLastError());
            }
        }

        memset(buffer, 0x0, sizeof(buffer));
        ret = SetupQueryDrivesInDiskSpaceListA(diskspace, buffer, sizeof(buffer), NULL);
        ok(ret, "test %d: Expected SetupQueryDrivesInDiskSpaceListA to succeed (%lu)\n", i, GetLastError());
        ok(!memcmp(section_test[i].devices, buffer, section_test[i].device_length),
           "test %d: Device list (%s) does not match\n", i, buffer);

        for (j = 0; j < 2; j++)
        {
            const struct device_usage *usage = &section_test[i].usage[j];
            if (!usage->dev)
                continue;

            space = 0;
            ret = SetupQuerySpaceRequiredOnDriveA(diskspace, usage->dev, &space, NULL, 0);
            ok(ret, "test %d: Expected SetupQuerySpaceRequiredOnDriveA to succeed for device %s (%lu)\n",
               i, usage->dev, GetLastError());
            ok(space == usage->usage, "test %d: Expected size %lu for device %s, got %lu\n",
               i, (DWORD)usage->usage, usage->dev, (DWORD)space);
        }

        ok(SetupDestroyDiskSpaceList(diskspace),
           "Expected SetupDestroyDiskSpaceList to succeed\n");

        SetupCloseInfFile(inf);
    }

    DeleteFileA(tmpfilename);
}

struct section_i
{
    const char *name;
    BOOL result;
    DWORD error_code;
};

static const struct
{
    const char *data;
    struct section_i sections[2];
    const char *devices;
    int device_length;
    struct device_usage usage[2];
}
section_test_i[] =
{
    /* 0 */
    {STD_HEADER "[a.Install]\nCopyFiles=a.CopyFiles\n"
                "[a.CopyFiles]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\r\n",
     {{"a.Install", TRUE, 0}, {NULL, TRUE, 0}}, "c:\00", sizeof("c:\00"), {{"c:", 4096}, {NULL, 0}}},
    /* 1 */
    {STD_HEADER "[a]\nCopyFiles=a.CopyFiles\n"
                "[a.CopyFiles]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\r\n",
     {{"a", TRUE, 0}, {NULL, TRUE, 0}}, "c:\00", sizeof("c:\00"), {{"c:", 4096}, {NULL, 0}}},
    /* 2 */
    {STD_HEADER "[a]\nCopyFiles=a.CopyFiles\nCopyFiles=a.CopyFiles2\n"
                "[a.CopyFiles]\ntest,,,\n[a.CopyFiles2]\ntest2,,,\n"
                "[SourceDisksFiles]\ntest=1,,4096\ntest2=1,,4096\r\n",
     {{"a", TRUE, 0}, {NULL, TRUE, 0}}, "c:\00", sizeof("c:\00"), {{"c:", 8192}, {NULL, 0}}},
    /* 3 */
    {STD_HEADER "[a]\nCopyFiles=a.CopyFiles,a.CopyFiles2\n"
                "[a.CopyFiles]\ntest,,,\n[a.CopyFiles2]\ntest2,,,\n"
                "[SourceDisksFiles]\ntest=1,,4096\ntest2=1,,4096\r\n",
     {{"a", TRUE, 0}, {NULL, TRUE, 0}}, "c:\00", sizeof("c:\00"), {{"c:", 8192}, {NULL, 0}}},
    /* 4 */
    {STD_HEADER "[a]\r\n",
     {{"a", TRUE, 0}, {NULL, TRUE, 0}}, "", sizeof(""), {{NULL, 0}, {NULL, 0}}},
    /* 5 */
    {STD_HEADER "[a]\nDelFiles=a.DelFiles\n"
                "[a.nDelFiles]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\r\n",
     {{"a", TRUE, 0}, {NULL, TRUE, 0}}, "", sizeof(""), {{NULL, 0}, {NULL, 0}}},
    /* 6 */
    {STD_HEADER "[a]\nCopyFiles=a.CopyFiles\nDelFiles=a.DelFiles\n"
                "[a.CopyFiles]\ntest,,,\n[a.DelFiles]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\r\n",
     {{"a", TRUE, 0}, {NULL, TRUE, 0}}, "c:\00", sizeof("c:\00"), {{"c:", 4096}, {NULL, 0}}},
    /* 7 */
    {STD_HEADER "[a]\nCopyFiles=a.CopyFiles\n[b]\nDelFiles=b.DelFiles\n"
                "[a.CopyFiles]\ntest,,,\n[b.DelFiles]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\r\n",
     {{"a", TRUE, 0}, {"b", TRUE, 0}}, "c:\00", sizeof("c:\00"), {{"c:", 4096}, {NULL, 0}}},
    /* 7 */
    {STD_HEADER "[a]\nCopyFiles=\r\n",
     {{"a", TRUE, 0}, {NULL, TRUE, 0}}, "", sizeof(""), {{NULL, 0}, {NULL, 0}}},
    /* 8 */
    {STD_HEADER "[a]\nCopyFiles=something\r\n",
     {{"a", TRUE, 0}, {NULL, TRUE, 0}}, "", sizeof(""), {{NULL, 0}, {NULL, 0}}},
    /* 9 */
    {STD_HEADER "[a]\nCopyFiles=a.CopyFiles,b.CopyFiles\n[a.CopyFiles]\ntest,,,\n[b.CopyFiles]\ntest,,,\n"
                "[SourceDisksFiles]\ntest=1,,4096\n[DestinationDirs]\nb.CopyFiles=-1,F:\\test\r\n",
     {{"a", TRUE, 0}, {NULL, TRUE, 0}}, "c:\00f:\00", sizeof("c:\00f:\00"), {{"c:", 4096}, {"f:", 4096}}},
};

static void test_SetupAddInstallSectionToDiskSpaceListA(void)
{
    char tmp[MAX_PATH];
    char tmpfilename[MAX_PATH];
    char buffer[MAX_PATH];
    HDSKSPC diskspace;
    LONGLONG space;
    UINT err_line;
    BOOL ret;
    int i, j;
    HINF inf;

    if (!GetTempPathA(MAX_PATH, tmp))
    {
        win_skip("GetTempPath failed with error %ld\n", GetLastError());
        return;
    }

    if (!GetTempFileNameA(tmp, "inftest", 0, tmpfilename))
    {
        win_skip("GetTempFileNameA failed with error %ld\n", GetLastError());
        return;
    }

    inf = inf_open_file_content(tmpfilename, STD_HEADER "[a]\nCopyFiles=b\n[b]\ntest,,,\n[SourceDisksFiles]\ntest=1,,4096\r\n", &err_line);
    ok(!!inf, "Failed to open inf file (%ld, line %u)\n", GetLastError(), err_line);

    diskspace = SetupCreateDiskSpaceListA(NULL, 0, SPDSL_IGNORE_DISK);
    ok(diskspace != NULL,"Expected SetupCreateDiskSpaceListA to return a valid handle\n");

    ret = SetupAddInstallSectionToDiskSpaceListA(diskspace, NULL, NULL, "a", 0, 0);
    ok(ret, "Expected SetupAddInstallSectionToDiskSpaceListA to succeed\n");

    ret = SetupAddInstallSectionToDiskSpaceListA(NULL, inf, NULL, "a", 0, 0);
    ok(!ret, "Expected SetupAddInstallSectionToDiskSpaceListA to fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE as error, got %lu\n",
       GetLastError());

    ret = SetupAddInstallSectionToDiskSpaceListA(diskspace, inf, NULL, NULL, 0, 0);
    ok(!ret || broken(ret), "Expected SetupAddSectionToDiskSpaceListA to fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(ret),
       "Expected ERROR_INVALID_PARAMETER as error, got %lu\n", GetLastError());

    ret = SetupAddInstallSectionToDiskSpaceListA(diskspace, inf, NULL, "", 0, 0);
    ok(ret, "Expected SetupAddInstallSectionToDiskSpaceListA to succeed (%lu)\n", GetLastError());

    ok(SetupDestroyDiskSpaceList(diskspace),
       "Expected SetupDestroyDiskSpaceList to succeed\n");

    for (i = 0; i < sizeof(section_test_i) / sizeof(section_test_i[0]); i++)
    {
        err_line = 0;

        inf = inf_open_file_content(tmpfilename, section_test_i[i].data, &err_line);
        ok(!!inf, "test %u: Failed to open inf file (%lu, line %u)\n", i, GetLastError(), err_line);
        if (!inf) continue;

        diskspace = SetupCreateDiskSpaceListA(NULL, 0, SPDSL_IGNORE_DISK);
        ok(diskspace != NULL,"Expected SetupCreateDiskSpaceListA to return a valid handle\n");

        for (j = 0; j < 2; j++)
        {
            const struct section_i *section = &section_test_i[i].sections[j];
            if (!section->name)
                continue;

            SetLastError(0xdeadbeef);
            ret = SetupAddInstallSectionToDiskSpaceListA(diskspace, inf, NULL, section->name, 0, 0);
            if (section->result)
                ok(ret, "test %d: Expected adding section %d to succeed (%lu)\n", i, j, GetLastError());
            else
            {
                ok(!ret, "test %d: Expected adding section %d to fail\n", i, j);
                ok(GetLastError() == section->error_code, "test %d: Expected %lu as error, got %lu\n",
                   i, section->error_code, GetLastError());
            }
        }

        memset(buffer, 0x0, sizeof(buffer));
        ret = SetupQueryDrivesInDiskSpaceListA(diskspace, buffer, sizeof(buffer), NULL);
        ok(ret, "test %d: Expected SetupQueryDrivesInDiskSpaceListA to succeed (%lu)\n", i, GetLastError());
        ok(!memcmp(section_test_i[i].devices, buffer, section_test_i[i].device_length),
           "test %d: Device list (%s) does not match\n", i, buffer);

        for (j = 0; j < 2; j++)
        {
            const struct device_usage *usage = &section_test_i[i].usage[j];
            if (!usage->dev)
                continue;

            space = 0;
            ret = SetupQuerySpaceRequiredOnDriveA(diskspace, usage->dev, &space, NULL, 0);
            ok(ret, "test %d: Expected SetupQuerySpaceRequiredOnDriveA to succeed for device %s (%lu)\n",
               i, usage->dev, GetLastError());
            ok(space == usage->usage, "test %d: Expected size %lu for device %s, got %lu\n",
               i, (DWORD)usage->usage, usage->dev, (DWORD)space);
        }

        ok(SetupDestroyDiskSpaceList(diskspace),
           "Expected SetupDestroyDiskSpaceList to succeed\n");

        SetupCloseInfFile(inf);
    }

    DeleteFileA(tmpfilename);
}

START_TEST(diskspace)
{
    test_SetupCreateDiskSpaceListA();
    test_SetupCreateDiskSpaceListW();
    test_SetupDuplicateDiskSpaceListA();
    test_SetupDuplicateDiskSpaceListW();
    test_SetupQuerySpaceRequiredOnDriveA();
    test_SetupQuerySpaceRequiredOnDriveW();
    test_SetupAddToDiskSpaceListA();
    test_SetupQueryDrivesInDiskSpaceListA();
    test_SetupAddSectionToDiskSpaceListA();
    test_SetupAddInstallSectionToDiskSpaceListA();
}
