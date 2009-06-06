/*
 * Unit test suite for drive functions.
 *
 * Copyright 2002 Dmitry Timoshkov
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"

static DWORD (WINAPI *pGetDiskFreeSpaceExA)(LPCSTR, PULARGE_INTEGER, PULARGE_INTEGER, PULARGE_INTEGER);

static void test_GetDriveTypeA(void)
{
    char drive[] = "?:\\";
    DWORD logical_drives;
    UINT type;

    logical_drives = GetLogicalDrives();
    ok(logical_drives != 0, "GetLogicalDrives error %d\n", GetLastError());

    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
        type = GetDriveTypeA(drive);
        ok(type > DRIVE_UNKNOWN && type <= DRIVE_RAMDISK,
           "not a valid drive %c: type %u\n", drive[0], type);

        if (!(logical_drives & 1))
            ok(type == DRIVE_NO_ROOT_DIR,
               "GetDriveTypeA should return DRIVE_NO_ROOT_DIR for inexistent drive %c: but not %u\n",
               drive[0], type);

        logical_drives >>= 1;
    }
}

static void test_GetDriveTypeW(void)
{
    WCHAR drive[] = {'?',':','\\',0};
    DWORD logical_drives;
    UINT type;

    logical_drives = GetLogicalDrives();
    ok(logical_drives != 0, "GetLogicalDrives error %d\n", GetLastError());

    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
        type = GetDriveTypeW(drive);
        if (type == DRIVE_UNKNOWN && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        {
            win_skip("GetDriveTypeW is not available on Win9x\n");
            return;
        }
        ok(type > DRIVE_UNKNOWN && type <= DRIVE_RAMDISK,
           "not a valid drive %c: type %u\n", drive[0], type);

        if (!(logical_drives & 1))
            ok(type == DRIVE_NO_ROOT_DIR,
               "GetDriveTypeW should return DRIVE_NO_ROOT_DIR for inexistent drive %c: but not %u\n",
               drive[0], type);

        logical_drives >>= 1;
    }
}

static void test_GetDiskFreeSpaceA(void)
{
    BOOL ret;
    DWORD sectors_per_cluster, bytes_per_sector, free_clusters, total_clusters;
    char drive[] = "?:\\";
    DWORD logical_drives;

    ret = GetDiskFreeSpaceA(NULL, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceA error %d\n", GetLastError());

    ret = GetDiskFreeSpaceA("", &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(!ret && (GetLastError() == ERROR_PATH_NOT_FOUND || GetLastError() == ERROR_INVALID_NAME),
       "GetDiskFreeSpaceA(\"\"): ret=%d GetLastError=%d\n",
       ret, GetLastError());

    ret = GetDiskFreeSpaceA("\\", &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceA error %d\n", GetLastError());

    ret = GetDiskFreeSpaceA("/", &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceA error %d\n", GetLastError());

    logical_drives = GetLogicalDrives();
    ok(logical_drives != 0, "GetLogicalDrives error %d\n", GetLastError());

    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
        UINT drivetype = GetDriveTypeA(drive);
        /* Skip floppy drives because NT pops up a MessageBox if no
         * floppy is present
         */
        if (drivetype != DRIVE_REMOVABLE && drivetype != DRIVE_NO_ROOT_DIR)
        {
            ret = GetDiskFreeSpaceA(drive, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
            if (!(logical_drives & 1))
                ok(!ret && (GetLastError() == ERROR_PATH_NOT_FOUND || GetLastError() == ERROR_INVALID_DRIVE),
                   "GetDiskFreeSpaceA(%s): ret=%d GetLastError=%d\n",
                   drive, ret, GetLastError());
            else
            {
                ok(ret ||
                   GetLastError() == ERROR_NOT_READY ||
                   GetLastError() == ERROR_INVALID_FUNCTION ||
                   GetLastError() == ERROR_INVALID_DRIVE ||
                   GetLastError() == ERROR_PATH_NOT_FOUND ||
                   GetLastError() == ERROR_REQUEST_ABORTED ||
                   GetLastError() == ERROR_UNRECOGNIZED_VOLUME,
                   "GetDiskFreeSpaceA(%s): ret=%d GetLastError=%d\n",
                   drive, ret, GetLastError());
                if( GetVersion() & 0x80000000)
                    /* win3.0 thru winME */
                    ok( total_clusters <= 65535,
                            "total clusters is %d > 65535\n", total_clusters);
                else if (pGetDiskFreeSpaceExA) {
                    /* NT, 2k, XP : GetDiskFreeSpace should be accurate */
                    ULARGE_INTEGER totEx, tot, d;

                    tot.QuadPart = sectors_per_cluster;
                    tot.QuadPart = (tot.QuadPart * bytes_per_sector) * total_clusters;
                    ret = pGetDiskFreeSpaceExA( drive, &d, &totEx, NULL);
                    ok( ret ||
                        GetLastError() == ERROR_NOT_READY ||
                        GetLastError() == ERROR_INVALID_FUNCTION ||
                        GetLastError() == ERROR_PATH_NOT_FOUND ||
                        GetLastError() == ERROR_REQUEST_ABORTED ||
                        GetLastError() == ERROR_UNRECOGNIZED_VOLUME,
                        "GetDiskFreeSpaceExA( %s ) failed. GetLastError=%d\n", drive, GetLastError());
                    ok( bytes_per_sector == 0 || /* empty cd rom drive */
                        totEx.QuadPart <= tot.QuadPart,
                        "GetDiskFreeSpaceA should report at least as much bytes on disk %s as GetDiskFreeSpaceExA\n", drive);
                }
            }
        }
        logical_drives >>= 1;
    }
}

static void test_GetDiskFreeSpaceW(void)
{
    BOOL ret;
    DWORD sectors_per_cluster, bytes_per_sector, free_clusters, total_clusters;
    WCHAR drive[] = {'?',':','\\',0};
    DWORD logical_drives;
    static const WCHAR empty_pathW[] = { 0 };
    static const WCHAR root_pathW[] = { '\\', 0 };
    static const WCHAR unix_style_root_pathW[] = { '/', 0 };

    ret = GetDiskFreeSpaceW(NULL, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    if (ret == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetDiskFreeSpaceW is not available\n");
        return;
    }
    ok(ret, "GetDiskFreeSpaceW error %d\n", GetLastError());

    ret = GetDiskFreeSpaceW(empty_pathW, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND,
       "GetDiskFreeSpaceW(\"\"): ret=%d GetLastError=%d\n",
       ret, GetLastError());

    ret = GetDiskFreeSpaceW(root_pathW, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceW(\"\") error %d\n", GetLastError());

    ret = GetDiskFreeSpaceW(unix_style_root_pathW, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceW error %d\n", GetLastError());

    logical_drives = GetLogicalDrives();
    ok(logical_drives != 0, "GetLogicalDrives error %d\n", GetLastError());

    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
	UINT drivetype = GetDriveTypeW(drive);
        /* Skip floppy drives because NT4 pops up a MessageBox if no floppy is present */
        if (drivetype != DRIVE_REMOVABLE && drivetype != DRIVE_NO_ROOT_DIR)
        {
            ret = GetDiskFreeSpaceW(drive, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
            if (!(logical_drives & 1))
                ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND,
                   "GetDiskFreeSpaceW(%c): ret=%d GetLastError=%d\n",
                   drive[0], ret, GetLastError());
            else
                ok( ret ||
                    GetLastError() == ERROR_NOT_READY ||
                    GetLastError() == ERROR_INVALID_FUNCTION ||
                    GetLastError() == ERROR_PATH_NOT_FOUND ||
                    GetLastError() == ERROR_REQUEST_ABORTED ||
                    GetLastError() == ERROR_UNRECOGNIZED_VOLUME,
                   "GetDiskFreeSpaceW(%c): ret=%d GetLastError=%d\n",
                   drive[0], ret, GetLastError());
        }
        logical_drives >>= 1;
    }
}

START_TEST(drive)
{
    HANDLE hkernel32 = GetModuleHandleA("kernel32");
    pGetDiskFreeSpaceExA = (void *) GetProcAddress(hkernel32, "GetDiskFreeSpaceExA");

    test_GetDriveTypeA();
    test_GetDriveTypeW();

    test_GetDiskFreeSpaceA();
    test_GetDiskFreeSpaceW();
}
