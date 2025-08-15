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
    char existing_drive_letter = 0;
    DWORD logical_drives;
    UINT type;

    logical_drives = GetLogicalDrives();
    ok(logical_drives != 0, "GetLogicalDrives error %ld\n", GetLastError());

    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
        type = GetDriveTypeA(drive);
        ok(type > DRIVE_UNKNOWN && type <= DRIVE_RAMDISK,
           "not a valid drive %c: type %u\n", drive[0], type);

        if (!(logical_drives & 1))
            ok(type == DRIVE_NO_ROOT_DIR,
               "GetDriveTypeA should return DRIVE_NO_ROOT_DIR for inexistent drive %c: but not %u\n",
               drive[0], type);
        else if (type != DRIVE_NO_ROOT_DIR)
            existing_drive_letter = drive[0];

        logical_drives >>= 1;
    }

    if (!existing_drive_letter) {
        skip("No drives found, skipping drive spec format tests.\n");
        return;
    }

    drive[0] = existing_drive_letter;
    drive[2] = 0; /* C: */
    type = GetDriveTypeA(drive);
    ok(type > DRIVE_NO_ROOT_DIR && type <= DRIVE_RAMDISK, "got %u for drive spec '%s'\n", type, drive);

    drive[1] = '?'; /* C? */
    type = GetDriveTypeA(drive);
    ok(type == DRIVE_NO_ROOT_DIR, "got %u for drive spec '%s'\n", type, drive);

    drive[1] = 0; /* C */
    type = GetDriveTypeA(drive);
    ok(type == DRIVE_NO_ROOT_DIR, "got %u for drive spec '%s'\n", type, drive);

    drive[0] = '?'; /* the string "?" */
    type = GetDriveTypeA(drive);
    ok(type == DRIVE_NO_ROOT_DIR, "got %u for drive spec '%s'\n", type, drive);

    drive[0] = 0; /* the empty string */
    type = GetDriveTypeA(drive);
    ok(type == DRIVE_NO_ROOT_DIR, "got %u for drive spec '%s'\n", type, drive);
}

static void test_GetDriveTypeW(void)
{
    WCHAR drive[] = {'?',':','\\',0};
    WCHAR existing_drive_letter = 0;
    DWORD logical_drives;
    UINT type;

    logical_drives = GetLogicalDrives();
    ok(logical_drives != 0, "GetLogicalDrives error %ld\n", GetLastError());

    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
        type = GetDriveTypeW(drive);
        ok(type > DRIVE_UNKNOWN && type <= DRIVE_RAMDISK,
           "not a valid drive %c: type %u\n", drive[0], type);

        if (!(logical_drives & 1))
            ok(type == DRIVE_NO_ROOT_DIR,
               "GetDriveTypeW should return DRIVE_NO_ROOT_DIR for inexistent drive %c: but not %u\n",
               drive[0], type);
        else if (type != DRIVE_NO_ROOT_DIR)
            existing_drive_letter = drive[0];

        logical_drives >>= 1;
    }

    if (!existing_drive_letter) {
        skip("No drives found, skipping drive spec format tests.\n");
        return;
    }

    drive[0] = existing_drive_letter;
    drive[2] = 0; /* C: */
    type = GetDriveTypeW(drive);
    ok(type > DRIVE_NO_ROOT_DIR && type <= DRIVE_RAMDISK, "got %u for drive spec '%s'\n",
       type, wine_dbgstr_w(drive));

    drive[1] = '?'; /* C? */
    type = GetDriveTypeW(drive);
    ok(type == DRIVE_NO_ROOT_DIR, "got %u for drive spec '%s'\n", type, wine_dbgstr_w(drive));

    drive[1] = 0; /* C */
    type = GetDriveTypeW(drive);
    ok(type == DRIVE_NO_ROOT_DIR, "got %u for drive spec '%s'\n", type, wine_dbgstr_w(drive));

    drive[0] = '?'; /* the string "?" */
    type = GetDriveTypeW(drive);
    ok(type == DRIVE_NO_ROOT_DIR, "got %u for drive spec '%s'\n", type, wine_dbgstr_w(drive));

    drive[0] = 0; /* the empty string */
    type = GetDriveTypeW(drive);
    ok(type == DRIVE_NO_ROOT_DIR, "got %u for drive spec '%s'\n", type, wine_dbgstr_w(drive));
}

static void test_GetDiskFreeSpaceA(void)
{
    BOOL ret;
    DWORD sectors_per_cluster, bytes_per_sector, free_clusters, total_clusters;
    char volume_guid_path[50];
    char drive[] = "?:\\";
    DWORD logical_drives;

    ret = GetDiskFreeSpaceA(NULL, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceA error %ld\n", GetLastError());

    ret = GetDiskFreeSpaceA("", &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(!ret && (GetLastError() == ERROR_PATH_NOT_FOUND || GetLastError() == ERROR_INVALID_NAME),
       "GetDiskFreeSpaceA(\"\"): ret=%d GetLastError=%ld\n",
       ret, GetLastError());

    ret = GetDiskFreeSpaceA("\\", &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceA error %ld\n", GetLastError());

    ret = GetDiskFreeSpaceA("/", &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceA error %ld\n", GetLastError());

    ret = GetDiskFreeSpaceA("C:\\", &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceA error %ld\n", GetLastError());

    ret = GetVolumeNameForVolumeMountPointA("C:\\", volume_guid_path, ARRAY_SIZE(volume_guid_path));
    ok(ret, "GetVolumeNameForVolumeMountPointA error %ld\n", GetLastError());

    ret = GetDiskFreeSpaceA(volume_guid_path, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceA error %ld\n", GetLastError());

    logical_drives = GetLogicalDrives();
    ok(logical_drives != 0, "GetLogicalDrives error %ld\n", GetLastError());

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
                   "GetDiskFreeSpaceA(%s): ret=%d GetLastError=%ld\n",
                   drive, ret, GetLastError());
            else
            {

                if (!ret)
                    /* GetDiskFreeSpaceA() should succeed, but it can fail with too many
                       different GetLastError() results to be usable for an ok() */
                    trace("GetDiskFreeSpaceA(%s) failed with %ld\n", drive, GetLastError());

                if( GetVersion() & 0x80000000)
                    /* win3.0 through winME */
                    ok( total_clusters <= 65535,
                            "total clusters is %ld > 65535\n", total_clusters);
                else if (pGetDiskFreeSpaceExA) {
                    /* NT, 2k, XP : GetDiskFreeSpace should be accurate */
                    ULARGE_INTEGER totEx, tot, d;

                    tot.QuadPart = sectors_per_cluster;
                    tot.QuadPart = (tot.QuadPart * bytes_per_sector) * total_clusters;
                    ret = pGetDiskFreeSpaceExA( drive, &d, &totEx, NULL);

                    if (!ret)
                        /* GetDiskFreeSpaceExA() should succeed, but it can fail with too many
                           different GetLastError() results to be usable for an ok() */
                        trace("GetDiskFreeSpaceExA(%s) failed with %ld\n", drive, GetLastError());

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
    WCHAR volume_guid_path[50];
    WCHAR drive[] = {'?',':','\\',0};
    DWORD logical_drives;
    static const WCHAR empty_pathW[] = { 0 };
    static const WCHAR root_pathW[] = { '\\', 0 };
    static const WCHAR unix_style_root_pathW[] = { '/', 0 };
    static const WCHAR c_drive_pathW[] = { 'C', ':', '\\', 0 };

    ret = GetDiskFreeSpaceW(NULL, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    if (ret == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("GetDiskFreeSpaceW is not available\n");
        return;
    }
    ok(ret, "GetDiskFreeSpaceW error %ld\n", GetLastError());

    ret = GetDiskFreeSpaceW(empty_pathW, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND,
       "GetDiskFreeSpaceW(\"\"): ret=%d GetLastError=%ld\n",
       ret, GetLastError());

    ret = GetDiskFreeSpaceW(root_pathW, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceW(\"\") error %ld\n", GetLastError());

    ret = GetDiskFreeSpaceW(unix_style_root_pathW, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceW error %ld\n", GetLastError());

    ret = GetDiskFreeSpaceW(c_drive_pathW, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceW error %ld\n", GetLastError());

    ret = GetVolumeNameForVolumeMountPointW(c_drive_pathW, volume_guid_path, ARRAY_SIZE(volume_guid_path));
    ok(ret, "GetVolumeNameForVolumeMountPointW error %ld\n", GetLastError());

    ret = GetDiskFreeSpaceW(volume_guid_path, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
    ok(ret, "GetDiskFreeSpaceW error %ld\n", GetLastError());

    logical_drives = GetLogicalDrives();
    ok(logical_drives != 0, "GetLogicalDrives error %ld\n", GetLastError());

    for (drive[0] = 'A'; drive[0] <= 'Z'; drive[0]++)
    {
	UINT drivetype = GetDriveTypeW(drive);
        /* Skip floppy drives because NT4 pops up a MessageBox if no floppy is present */
        if (drivetype != DRIVE_REMOVABLE && drivetype != DRIVE_NO_ROOT_DIR)
        {
            ret = GetDiskFreeSpaceW(drive, &sectors_per_cluster, &bytes_per_sector, &free_clusters, &total_clusters);
            if (!(logical_drives & 1))
                ok(!ret && GetLastError() == ERROR_PATH_NOT_FOUND,
                   "GetDiskFreeSpaceW(%c): ret=%d GetLastError=%ld\n",
                   drive[0], ret, GetLastError());
            else if (!ret)
                /* GetDiskFreeSpaceW() should succeed, but it can fail with too many
                   different GetLastError() results to be usable for an ok() */
                trace("GetDiskFreeSpaceW(%c) failed with %ld\n", drive[0], GetLastError());
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
