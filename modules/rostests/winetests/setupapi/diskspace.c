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

static inline const char* debugstr_longlong(ULONGLONG ll)
{
    static char string[17];
    if (sizeof(ll) > sizeof(unsigned long) && ll >> 32)
        sprintf(string, "%lx%08lx", (unsigned long)(ll >> 32), (unsigned long)ll);
    else
        sprintf(string, "%lx", (unsigned long)ll);
    return string;
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
}
