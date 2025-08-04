/*
 * Unit test suite for volume functions
 *
 * Copyright 2006 Stefan Leichter
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

#include <stdio.h>
#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winioctl.h"
#include "ntddstor.h"
#include "winternl.h"
#include "ddk/ntddcdvd.h"
#include "ddk/mountmgr.h"
#include "wine/test.h"

#include <pshpack1.h>
struct COMPLETE_DVD_LAYER_DESCRIPTOR
{
    DVD_DESCRIPTOR_HEADER Header;
    DVD_LAYER_DESCRIPTOR Descriptor;
    UCHAR Padding;
};
#include <poppack.h>
C_ASSERT(sizeof(struct COMPLETE_DVD_LAYER_DESCRIPTOR) == 22);

#include <pshpack1.h>
struct COMPLETE_DVD_MANUFACTURER_DESCRIPTOR
{
    DVD_DESCRIPTOR_HEADER Header;
    DVD_MANUFACTURER_DESCRIPTOR Descriptor;
    UCHAR Padding;
};
#include <poppack.h>
C_ASSERT(sizeof(struct COMPLETE_DVD_MANUFACTURER_DESCRIPTOR) == 2053);

static HINSTANCE hdll;
static HANDLE (WINAPI *pFindFirstVolumeA)(LPSTR,DWORD);
static BOOL (WINAPI *pFindNextVolumeA)(HANDLE,LPSTR,DWORD);
static BOOL (WINAPI *pFindVolumeClose)(HANDLE);
static UINT (WINAPI *pGetLogicalDriveStringsA)(UINT,LPSTR);
static UINT (WINAPI *pGetLogicalDriveStringsW)(UINT,LPWSTR);
static BOOL (WINAPI *pGetVolumePathNamesForVolumeNameA)(LPCSTR, LPSTR, DWORD, LPDWORD);
static BOOL (WINAPI *pGetVolumePathNamesForVolumeNameW)(LPCWSTR, LPWSTR, DWORD, LPDWORD);
static BOOL (WINAPI *pCreateSymbolicLinkA)(const char *, const char *, DWORD);
static BOOL (WINAPI *pGetVolumeInformationByHandleW)(HANDLE, WCHAR *, DWORD, DWORD *, DWORD *, DWORD *, WCHAR *, DWORD);

/* ############################### */

static void test_query_dos_deviceA(void)
{
    char drivestr[] = "a:";
    char *p, *buffer, buffer2[2000];
    DWORD ret, ret2, buflen=32768;
    BOOL found = FALSE;

    /* callers must guess the buffer size */
    SetLastError(0xdeadbeef);
    ret = QueryDosDeviceA( NULL, NULL, 0 );
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "QueryDosDeviceA(no buffer): returned %lu, le=%lu\n", ret, GetLastError());

    buffer = HeapAlloc( GetProcessHeap(), 0, buflen );
    SetLastError(0xdeadbeef);
    ret = QueryDosDeviceA( NULL, buffer, buflen );
    ok((ret && GetLastError() != ERROR_INSUFFICIENT_BUFFER),
        "QueryDosDeviceA failed to return list, last error %lu\n", GetLastError());

    if (ret && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        p = buffer;
        for (;;) {
            if (!*p) break;
            ret2 = QueryDosDeviceA( p, buffer2, sizeof(buffer2) );
            /* Win10+ exposes the security device which requires extra privileges to be queried. So skip it */
            ok(ret2 || broken( !strcmp( p, "MSSECFLTSYS" ) && GetLastError() == ERROR_ACCESS_DENIED ),
               "QueryDosDeviceA failed to return current mapping for %s, last error %lu\n", p, GetLastError());
            p += strlen(p) + 1;
            if (ret <= (p-buffer)) break;
        }
    }

    for (;drivestr[0] <= 'z'; drivestr[0]++) {
        /* Older W2K fails with ERROR_INSUFFICIENT_BUFFER when buflen is > 32767 */
        ret = QueryDosDeviceA( drivestr, buffer, buflen - 1);
        ok(ret || GetLastError() == ERROR_FILE_NOT_FOUND,
            "QueryDosDeviceA failed to return current mapping for %s, last error %lu\n", drivestr, GetLastError());
        if(ret) {
            for (p = buffer; *p; p++) *p = toupper(*p);
            if (strstr(buffer, "HARDDISK") || strstr(buffer, "RAMDISK")) found = TRUE;
        }
    }
    ok(found, "expected at least one devicename to contain HARDDISK or RAMDISK\n");
    HeapFree( GetProcessHeap(), 0, buffer );
}

static void test_dos_devices(void)
{
    char buf[MAX_PATH], buf2[400];
    char drivestr[3];
    HANDLE file;
    BOOL ret;

    /* Find an unused drive letter */
    drivestr[1] = ':';
    drivestr[2] = 0;
    for (drivestr[0] = 'a'; drivestr[0] <= 'z'; drivestr[0]++) {
        ret = QueryDosDeviceA( drivestr, buf, sizeof(buf));
        if (!ret) break;
    }
    if (drivestr[0] > 'z') {
        skip("can't test creating a dos drive, none available\n");
        return;
    }

    ret = DefineDosDeviceA( 0, drivestr, "C:/windows/" );
    ok(ret, "failed to define drive %s, error %lu\n", drivestr, GetLastError());

    ret = QueryDosDeviceA( drivestr, buf, sizeof(buf) );
    ok(ret, "failed to query drive %s, error %lu\n", drivestr, GetLastError());
    ok(!strcmp(buf, "\\??\\C:\\windows\\"), "got path %s\n", debugstr_a(buf));

    sprintf(buf, "%s/system32", drivestr);
    file = CreateFileA( buf, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    todo_wine ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());
    CloseHandle( file );

    /* but it's not a volume mount point */

    sprintf(buf, "%s\\", drivestr);
    ret = GetVolumeNameForVolumeMountPointA( buf, buf2, sizeof(buf2) );
    ok(!ret, "expected failure\n");
    todo_wine ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());

    ret = DefineDosDeviceA(DDD_REMOVE_DEFINITION, drivestr, NULL);
    ok(ret, "failed to remove drive %s, error %lu\n", drivestr, GetLastError());

    ret = QueryDosDeviceA( drivestr, buf, sizeof(buf) );
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "got error %lu\n", GetLastError());

    sprintf(buf, "%s/system32", drivestr);
    file = CreateFileA( buf, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    ok(file == INVALID_HANDLE_VALUE, "expected failure\n");
    todo_wine ok(GetLastError() == ERROR_PATH_NOT_FOUND, "got error %lu\n", GetLastError());

    /* try with DDD_RAW_TARGET_PATH */

    ret = DefineDosDeviceA( DDD_RAW_TARGET_PATH, drivestr, "\\??\\C:\\windows\\" );
    ok(ret, "failed to define drive %s, error %lu\n", drivestr, GetLastError());

    ret = QueryDosDeviceA( drivestr, buf, sizeof(buf) );
    ok(ret, "failed to query drive %s, error %lu\n", drivestr, GetLastError());
    ok(!strcmp(buf, "\\??\\C:\\windows\\"), "got path %s\n", debugstr_a(buf));

    sprintf(buf, "%s/system32", drivestr);
    file = CreateFileA( buf, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    todo_wine ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());
    CloseHandle( file );

    sprintf(buf, "%s\\", drivestr);
    ret = GetVolumeNameForVolumeMountPointA( buf, buf2, sizeof(buf2) );
    ok(!ret, "expected failure\n");
    todo_wine ok(GetLastError() == ERROR_INVALID_PARAMETER, "got error %lu\n", GetLastError());

    ret = DefineDosDeviceA(DDD_REMOVE_DEFINITION, drivestr, NULL);
    ok(ret, "failed to remove drive %s, error %lu\n", drivestr, GetLastError());

    ret = QueryDosDeviceA( drivestr, buf, sizeof(buf) );
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "got error %lu\n", GetLastError());
}

static void test_FindFirstVolume(void)
{
    char volume[51];
    HANDLE handle;

    /* not present before w2k */
    if (!pFindFirstVolumeA) {
        win_skip("FindFirstVolumeA not found\n");
        return;
    }

    handle = pFindFirstVolumeA( volume, 0 );
    ok( handle == INVALID_HANDLE_VALUE, "succeeded with short buffer\n" );
    ok( GetLastError() == ERROR_MORE_DATA ||  /* XP */
        GetLastError() == ERROR_FILENAME_EXCED_RANGE,  /* Vista */
        "wrong error %lu\n", GetLastError() );
    handle = pFindFirstVolumeA( volume, 49 );
    ok( handle == INVALID_HANDLE_VALUE, "succeeded with short buffer\n" );
    ok( GetLastError() == ERROR_FILENAME_EXCED_RANGE, "wrong error %lu\n", GetLastError() );
    handle = pFindFirstVolumeA( volume, 51 );
    ok( handle != INVALID_HANDLE_VALUE, "failed err %lu\n", GetLastError() );
    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            ok( strlen(volume) == 49, "bad volume name %s\n", volume );
            ok( !memcmp( volume, "\\\\?\\Volume{", 11 ), "bad volume name %s\n", volume );
            ok( !memcmp( volume + 47, "}\\", 2 ), "bad volume name %s\n", volume );
        } while (pFindNextVolumeA( handle, volume, MAX_PATH ));
        ok( GetLastError() == ERROR_NO_MORE_FILES, "wrong error %lu\n", GetLastError() );
        pFindVolumeClose( handle );
    }
}

static void test_GetVolumeNameForVolumeMountPointA(void)
{
    BOOL ret;
    char volume[MAX_PATH], path[] = "c:\\";
    DWORD len = sizeof(volume), reti;
    char temp_path[MAX_PATH];

    reti = GetTempPathA(MAX_PATH, temp_path);
    ok(reti != 0, "GetTempPathA error %ld\n", GetLastError());
    ok(reti < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = GetVolumeNameForVolumeMountPointA(path, volume, 0);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");
    ok(GetLastError() == ERROR_FILENAME_EXCED_RANGE ||
        GetLastError() == ERROR_INVALID_PARAMETER, /* Vista */
        "wrong error, last=%ld\n", GetLastError());

    if (0) { /* these crash on XP */
    ret = GetVolumeNameForVolumeMountPointA(path, NULL, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");

    ret = GetVolumeNameForVolumeMountPointA(NULL, volume, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");
    }

    ret = GetVolumeNameForVolumeMountPointA(path, volume, len);
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointA failed\n");
    ok(!strncmp( volume, "\\\\?\\Volume{", 11),
        "GetVolumeNameForVolumeMountPointA failed to return valid string <%s>\n",
        volume);

    /* test with too small buffer */
    ret = GetVolumeNameForVolumeMountPointA(path, volume, 10);
    ok(ret == FALSE && GetLastError() == ERROR_FILENAME_EXCED_RANGE,
            "GetVolumeNameForVolumeMountPointA failed, wrong error returned, was %ld, should be ERROR_FILENAME_EXCED_RANGE\n",
             GetLastError());

    /* Try on an arbitrary directory */
    /* On FAT filesystems it seems that GetLastError() is set to
       ERROR_INVALID_FUNCTION. */
    ret = GetVolumeNameForVolumeMountPointA(temp_path, volume, len);
    ok(ret == FALSE && (GetLastError() == ERROR_NOT_A_REPARSE_POINT ||
        GetLastError() == ERROR_INVALID_FUNCTION),
        "GetVolumeNameForVolumeMountPointA failed on %s, last=%ld\n",
        temp_path, GetLastError());

    /* Try on a nonexistent dos drive */
    path[2] = 0;
    for (;path[0] <= 'z'; path[0]++) {
        ret = QueryDosDeviceA( path, volume, len);
        if(!ret) break;
    }
    if (path[0] <= 'z')
    {
        path[2] = '\\';
        ret = GetVolumeNameForVolumeMountPointA(path, volume, len);
        ok(ret == FALSE && GetLastError() == ERROR_FILE_NOT_FOUND,
            "GetVolumeNameForVolumeMountPointA failed on %s, last=%ld\n",
            path, GetLastError());

        /* Try without trailing \ and on a nonexistent dos drive  */
        path[2] = 0;
        ret = GetVolumeNameForVolumeMountPointA(path, volume, len);
        ok(ret == FALSE && GetLastError() == ERROR_INVALID_NAME,
            "GetVolumeNameForVolumeMountPointA failed on %s, last=%ld\n",
            path, GetLastError());
    }
}

static void test_GetVolumeNameForVolumeMountPointW(void)
{
    BOOL ret;
    WCHAR volume[MAX_PATH], path[] = {'c',':','\\',0};
    DWORD len = ARRAY_SIZE(volume);

    ret = GetVolumeNameForVolumeMountPointW(path, volume, 0);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointW succeeded\n");
    ok(GetLastError() == ERROR_FILENAME_EXCED_RANGE ||
        GetLastError() == ERROR_INVALID_PARAMETER, /* Vista */
        "wrong error, last=%ld\n", GetLastError());

    if (0) { /* these crash on XP */
    ret = GetVolumeNameForVolumeMountPointW(path, NULL, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointW succeeded\n");

    ret = GetVolumeNameForVolumeMountPointW(NULL, volume, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointW succeeded\n");
    }

    ret = GetVolumeNameForVolumeMountPointW(path, volume, len);
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointW failed\n");
}

static void test_GetLogicalDriveStringsA(void)
{
    UINT size, size2;
    char *buf, *ptr;

    ok( pGetLogicalDriveStringsA != NULL, "GetLogicalDriveStringsA not available\n");
    if(!pGetLogicalDriveStringsA) {
        return;
    }

    size = pGetLogicalDriveStringsA(0, NULL);
    ok(size%4 == 1, "size = %d\n", size);

    buf = HeapAlloc(GetProcessHeap(), 0, size);

    *buf = 0;
    size2 = pGetLogicalDriveStringsA(2, buf);
    ok(size2 == size, "size2 = %d\n", size2);
    ok(!*buf, "buf changed\n");

    size2 = pGetLogicalDriveStringsA(size, buf);
    ok(size2 == size-1, "size2 = %d\n", size2);

    for(ptr = buf; ptr < buf+size2; ptr += 4) {
        ok(('A' <= *ptr && *ptr <= 'Z'), "device name '%c' is not uppercase\n", *ptr);
        ok(ptr[1] == ':', "ptr[1] = %c, expected ':'\n", ptr[1]);
        ok(ptr[2] == '\\', "ptr[2] = %c expected '\\'\n", ptr[2]);
        ok(!ptr[3], "ptr[3] = %c expected nullbyte\n", ptr[3]);
    }
    ok(!*ptr, "buf[size2] is not nullbyte\n");

    HeapFree(GetProcessHeap(), 0, buf);
}

static void test_GetLogicalDriveStringsW(void)
{
    UINT size, size2;
    WCHAR *buf, *ptr;

    ok( pGetLogicalDriveStringsW != NULL, "GetLogicalDriveStringsW not available\n");
    if(!pGetLogicalDriveStringsW) {
        return;
    }

    SetLastError(0xdeadbeef);
    size = pGetLogicalDriveStringsW(0, NULL);
    if (size == 0 && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
        win_skip("GetLogicalDriveStringsW not implemented\n");
        return;
    }
    ok(size%4 == 1, "size = %d\n", size);

    buf = HeapAlloc(GetProcessHeap(), 0, size*sizeof(WCHAR));

    *buf = 0;
    size2 = pGetLogicalDriveStringsW(2, buf);
    ok(size2 == size, "size2 = %d\n", size2);
    ok(!*buf, "buf changed\n");

    size2 = pGetLogicalDriveStringsW(size, buf);
    ok(size2 == size-1, "size2 = %d\n", size2);

    for(ptr = buf; ptr < buf+size2; ptr += 4) {
        ok('A' <= *ptr && *ptr <= 'Z', "device name '%c' is not uppercase\n", *ptr);
        ok(ptr[1] == ':', "ptr[1] = %c, expected ':'\n", ptr[1]);
        ok(ptr[2] == '\\', "ptr[2] = %c expected '\\'\n", ptr[2]);
        ok(!ptr[3], "ptr[3] = %c expected nullbyte\n", ptr[3]);
    }
    ok(!*ptr, "buf[size2] is not nullbyte\n");

    HeapFree(GetProcessHeap(), 0, buf);
}

static void test_GetVolumeInformationA(void)
{
    BOOL ret;
    UINT result;
    char Root_Colon[]="C:";
    char Root_Slash[]="C:\\";
    char Root_UNC[]="\\\\?\\C:\\";
    char volume[MAX_PATH+1];
    DWORD vol_name_size=MAX_PATH+1, vol_serial_num=-1, max_comp_len=0, fs_flags=0, fs_name_len=MAX_PATH+1;
    char vol_name_buf[MAX_PATH+1], fs_name_buf[MAX_PATH+1];
    char windowsdir[MAX_PATH+10];
    char currentdir[MAX_PATH+1];

    /* get windows drive letter and update strings for testing */
    result = GetWindowsDirectoryA(windowsdir, sizeof(windowsdir));
    ok(result < sizeof(windowsdir), "windowsdir is abnormally long!\n");
    ok(result != 0, "GetWindowsDirectory: error %ld\n", GetLastError());
    Root_Colon[0] = windowsdir[0];
    Root_Slash[0] = windowsdir[0];
    Root_UNC[4] = windowsdir[0];

    result = GetCurrentDirectoryA(MAX_PATH, currentdir);
    ok(result, "GetCurrentDirectory: error %ld\n", GetLastError());
    /* Note that GetCurrentDir yields no trailing slash for subdirs */

    /* check for NO error on no trailing \ when current dir is root dir */
    ret = SetCurrentDirectoryA(Root_Slash);
    ok(ret, "SetCurrentDirectory: error %ld\n", GetLastError());
    ret = GetVolumeInformationA(Root_Colon, vol_name_buf, vol_name_size, NULL,
            NULL, NULL, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA root failed, last error %lu\n", GetLastError());

    /* check for error on no trailing \ when current dir is subdir (windows) of queried drive */
    ret = SetCurrentDirectoryA(windowsdir);
    ok(ret, "SetCurrentDirectory: error %ld\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = GetVolumeInformationA(Root_Colon, vol_name_buf, vol_name_size, NULL,
            NULL, NULL, fs_name_buf, fs_name_len);
    ok(!ret && (GetLastError() == ERROR_INVALID_NAME),
        "GetVolumeInformationA did%s fail, last error %lu\n", ret ? " not":"", GetLastError());

    /* reset current directory */
    ret = SetCurrentDirectoryA(currentdir);
    ok(ret, "SetCurrentDirectory: error %ld\n", GetLastError());

    if (toupper(currentdir[0]) == toupper(windowsdir[0])) {
        skip("Please re-run from another device than %c:\n", windowsdir[0]);
        /* FIXME: Use GetLogicalDrives to find another device to avoid this skip. */
    } else {
        char Root_Env[]="=C:"; /* where MS maintains the per volume directory */
        Root_Env[1] = windowsdir[0];

        /* C:\windows becomes the current directory on drive C: */
        /* Note that paths to subdirs are stored without trailing slash, like what GetCurrentDir yields. */
        ret = SetEnvironmentVariableA(Root_Env, windowsdir);
        ok(ret, "SetEnvironmentVariable %s failed\n", Root_Env);

        ret = SetCurrentDirectoryA(windowsdir);
        ok(ret, "SetCurrentDirectory: error %ld\n", GetLastError());
        ret = SetCurrentDirectoryA(currentdir);
        ok(ret, "SetCurrentDirectory: error %ld\n", GetLastError());

        /* windows dir is current on the root drive, call fails */
        SetLastError(0xdeadbeef);
        ret = GetVolumeInformationA(Root_Colon, vol_name_buf, vol_name_size, NULL,
                NULL, NULL, fs_name_buf, fs_name_len);
        ok(!ret && (GetLastError() == ERROR_INVALID_NAME),
           "GetVolumeInformationA did%s fail, last error %lu\n", ret ? " not":"", GetLastError());

        /* Try normal drive letter with trailing \ */
        ret = GetVolumeInformationA(Root_Slash, vol_name_buf, vol_name_size, NULL,
                NULL, NULL, fs_name_buf, fs_name_len);
        ok(ret, "GetVolumeInformationA with \\ failed, last error %lu\n", GetLastError());

        ret = SetCurrentDirectoryA(Root_Slash);
        ok(ret, "SetCurrentDirectory: error %ld\n", GetLastError());
        ret = SetCurrentDirectoryA(currentdir);
        ok(ret, "SetCurrentDirectory: error %ld\n", GetLastError());

        /* windows dir is STILL CURRENT on root drive; the call fails as before,   */
        /* proving that SetCurrentDir did not remember the other drive's directory */
        SetLastError(0xdeadbeef);
        ret = GetVolumeInformationA(Root_Colon, vol_name_buf, vol_name_size, NULL,
                NULL, NULL, fs_name_buf, fs_name_len);
        ok(!ret && (GetLastError() == ERROR_INVALID_NAME),
           "GetVolumeInformationA did%s fail, last error %lu\n", ret ? " not":"", GetLastError());

        /* Now C:\ becomes the current directory on drive C: */
        ret = SetEnvironmentVariableA(Root_Env, Root_Slash); /* set =C:=C:\ */
        ok(ret, "SetEnvironmentVariable %s failed\n", Root_Env);

        /* \ is current on root drive, call succeeds */
        ret = GetVolumeInformationA(Root_Colon, vol_name_buf, vol_name_size, NULL,
                NULL, NULL, fs_name_buf, fs_name_len);
        ok(ret, "GetVolumeInformationA failed, last error %lu\n", GetLastError());

        /* again, SetCurrentDirectory on another drive does not matter */
        ret = SetCurrentDirectoryA(Root_Slash);
        ok(ret, "SetCurrentDirectory: error %ld\n", GetLastError());
        ret = SetCurrentDirectoryA(currentdir);
        ok(ret, "SetCurrentDirectory: error %ld\n", GetLastError());

        /* \ is current on root drive, call succeeds */
        ret = GetVolumeInformationA(Root_Colon, vol_name_buf, vol_name_size, NULL,
                NULL, NULL, fs_name_buf, fs_name_len);
        ok(ret, "GetVolumeInformationA failed, last error %lu\n", GetLastError());
    }

    /* try null root directory to return "root of the current directory"  */
    ret = GetVolumeInformationA(NULL, vol_name_buf, vol_name_size, NULL,
            NULL, NULL, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA failed on null root dir, last error %lu\n", GetLastError());

    /* Try normal drive letter with trailing \  */
    ret = GetVolumeInformationA(Root_Slash, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA failed, root=%s, last error=%lu\n", Root_Slash, GetLastError());

    /* try again with drive letter and the "disable parsing" prefix */
    SetLastError(0xdeadbeef);
    ret = GetVolumeInformationA(Root_UNC, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA did%s fail, root=%s, last error=%lu\n", ret ? " not":"", Root_UNC, GetLastError());

    /* try again with device name space  */
    Root_UNC[2] = '.';
    SetLastError(0xdeadbeef);
    ret = GetVolumeInformationA(Root_UNC, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA did%s fail, root=%s, last error=%lu\n", ret ? " not":"", Root_UNC, GetLastError());

    /* try again with a directory off the root - should generate error  */
    if (windowsdir[strlen(windowsdir)-1] != '\\') strcat(windowsdir, "\\");
    SetLastError(0xdeadbeef);
    ret = GetVolumeInformationA(windowsdir, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(!ret && (GetLastError()==ERROR_DIR_NOT_ROOT),
          "GetVolumeInformationA did%s fail, root=%s, last error=%lu\n", ret ? " not":"", windowsdir, GetLastError());
    /* A subdir with trailing \ yields DIR_NOT_ROOT instead of INVALID_NAME */
    if (windowsdir[strlen(windowsdir)-1] == '\\') windowsdir[strlen(windowsdir)-1] = 0;
    SetLastError(0xdeadbeef);
    ret = GetVolumeInformationA(windowsdir, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(!ret && (GetLastError()==ERROR_INVALID_NAME),
          "GetVolumeInformationA did%s fail, root=%s, last error=%lu\n", ret ? " not":"", windowsdir, GetLastError());

    /* get the unique volume name for the windows drive  */
    ret = GetVolumeNameForVolumeMountPointA(Root_Slash, volume, MAX_PATH);
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointA failed\n");

    /* try again with unique volume name */
    ret = GetVolumeInformationA(volume, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA failed, root=%s, last error=%lu\n", volume, GetLastError());
}

/* Test to check that unique volume name from windows dir mount point  */
/* matches at least one of the unique volume names returned from the   */
/* FindFirstVolumeA/FindNextVolumeA list.                              */
static void test_enum_vols(void)
{
    DWORD   ret;
    HANDLE  hFind = INVALID_HANDLE_VALUE;
    char    Volume_1[MAX_PATH] = {0};
    char    Volume_2[MAX_PATH] = {0};
    char    path[] = "c:\\";
    BOOL    found = FALSE;
    char    windowsdir[MAX_PATH];

    /*get windows drive letter and update strings for testing  */
    ret = GetWindowsDirectoryA( windowsdir, sizeof(windowsdir) );
    ok(ret < sizeof(windowsdir), "windowsdir is abnormally long!\n");
    ok(ret != 0, "GetWindowsDirectory: error %ld\n", GetLastError());
    path[0] = windowsdir[0];

    /* get the unique volume name for the windows drive  */
    ret = GetVolumeNameForVolumeMountPointA( path, Volume_1, MAX_PATH );
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointA failed\n");
    ok(strlen(Volume_1) == 49, "GetVolumeNameForVolumeMountPointA returned wrong length name %s\n", Volume_1);

    /* get first unique volume name of list  */
    hFind = pFindFirstVolumeA( Volume_2, MAX_PATH );
    ok(hFind != INVALID_HANDLE_VALUE, "FindFirstVolume failed, err=%lu\n",
                GetLastError());

    do
    {
        /* validate correct length of unique volume name  */
        ok(strlen(Volume_2) == 49, "Find[First/Next]Volume returned wrong length name %s\n", Volume_1);
        if (memcmp(Volume_1, Volume_2, 49) == 0)
        {
            found = TRUE;
            break;
        }
    } while (pFindNextVolumeA( hFind, Volume_2, MAX_PATH ));
    ok(found, "volume name %s not found by Find[First/Next]Volume\n", Volume_1);
    pFindVolumeClose( hFind );
}

static void test_disk_extents(void)
{
    BOOL ret;
    DWORD size;
    HANDLE handle;
    static DWORD data[16];

    handle = CreateFileA( "\\\\.\\c:", GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0 );
    if (handle == INVALID_HANDLE_VALUE)
    {
        win_skip("can't open c: drive %lu\n", GetLastError());
        return;
    }
    size = 0;
    ret = DeviceIoControl( handle, IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS, &data,
                           sizeof(data), &data, sizeof(data), &size, NULL );
    if (!ret && GetLastError() == ERROR_INVALID_FUNCTION)
    {
        win_skip("IOCTL_VOLUME_GET_VOLUME_DISK_EXTENTS not supported\n");
        CloseHandle( handle );
        return;
    }
    ok(ret, "DeviceIoControl failed %lu\n", GetLastError());
    ok(size == 32, "expected 32, got %lu\n", size);
    CloseHandle( handle );
}

static void test_disk_query_property(void)
{
    STORAGE_PROPERTY_QUERY query = {0};
    STORAGE_DESCRIPTOR_HEADER header = {0};
    STORAGE_DEVICE_DESCRIPTOR descriptor = {0};
    DEVICE_SEEK_PENALTY_DESCRIPTOR seek_pen = {0};
    HANDLE handle;
    DWORD error;
    DWORD size;
    BOOL ret;

    handle = CreateFileA("\\\\.\\PhysicalDrive0", GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING,
                         0, 0);
    if (handle == INVALID_HANDLE_VALUE)
    {
        win_skip("can't open \\\\.\\PhysicalDrive0 %#lx\n", GetLastError());
        return;
    }

    query.PropertyId = StorageDeviceProperty;
    query.QueryType = PropertyStandardQuery;

    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &header, sizeof(header), &size,
                          NULL);
    error = GetLastError();
    ok(ret, "expect ret %#x, got %#x\n", TRUE, ret);
    ok(error == 0xdeadbeef, "expect err %#x, got err %#lx\n", 0xdeadbeef, error);
    ok(size == sizeof(header), "got size %ld\n", size);
    ok(header.Version == sizeof(descriptor), "got header.Version %ld\n", header.Version);
    ok(header.Size >= sizeof(descriptor), "got header.Size %ld\n", header.Size);

    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &descriptor, sizeof(descriptor),
                          &size, NULL);
    error = GetLastError();
    ok(ret, "expect ret %#x, got %#x\n", TRUE, ret);
    ok(error == 0xdeadbeef, "expect err %#x, got err %#lx\n", 0xdeadbeef, error);
    ok(size == sizeof(descriptor), "got size %ld\n", size);
    ok(descriptor.Version == sizeof(descriptor), "got descriptor.Version %ld\n", descriptor.Version);
    ok(descriptor.Size >= sizeof(descriptor), "got descriptor.Size %ld\n", descriptor.Size);


    query.PropertyId = StorageDeviceSeekPenaltyProperty;
    query.QueryType = PropertyStandardQuery;
    SetLastError(0xdeadbeef);
    ret = DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &header, sizeof(header), &size,
                          NULL);
    error = GetLastError();
    if (!ret && error == ERROR_INVALID_FUNCTION)
    {
        win_skip( "StorageDeviceSeekPenaltyProperty is not supported.\n" ); /* Win7 */
    }
    else
    {
        ok(ret, "expect ret %#x, got %#x\n", TRUE, ret);
        ok(error == 0xdeadbeef, "expect err %#x, got err %#lx\n", 0xdeadbeef, error);
        ok(size == sizeof(header), "got size %ld\n", size);
        ok(header.Version == sizeof(seek_pen), "got header.Version %ld\n", header.Version);
        ok(header.Size == sizeof(seek_pen), "got header.Size %ld\n", header.Size);

        memset(&seek_pen, 0xcc, sizeof(seek_pen));
        ret = DeviceIoControl(handle, IOCTL_STORAGE_QUERY_PROPERTY, &query, sizeof(query), &seek_pen, sizeof(seek_pen),
                              &size, NULL);
        error = GetLastError();
        ok(ret || (error == ERROR_INVALID_FUNCTION /* Win8 VMs */ || (error == ERROR_GEN_FAILURE /* VMs */)),
          "got ret %d, error %#lx\n", ret, error);
        if (ret)
        {
            ok(size == sizeof(seek_pen), "got size %ld\n", size);
            ok(seek_pen.Version == sizeof(seek_pen), "got %ld\n", seek_pen.Version);
            ok(seek_pen.Size == sizeof(seek_pen), "got %ld\n", seek_pen.Size);
            ok(seek_pen.IncursSeekPenalty == TRUE || seek_pen.IncursSeekPenalty == FALSE, "got %d.\n", seek_pen.IncursSeekPenalty);
        }
    }

    CloseHandle(handle);
}

static void test_GetVolumePathNameA(void)
{
    char volume_path[MAX_PATH], cwd[MAX_PATH], expect_path[MAX_PATH];
    struct {
        const char *file_name;
        const char *path_name;
        DWORD       path_len;
        DWORD       error;
        DWORD       broken_error;
    } test_paths[] = {
        { /* test 0: NULL parameters, 0 output length */
            NULL, NULL, 0,
            ERROR_INVALID_PARAMETER, 0xdeadbeef /* winxp */
        },
        { /* empty input, NULL output, 0 output length */
            "", NULL, 0,
            ERROR_INVALID_PARAMETER, 0xdeadbeef /* winxp */
        },
        { /* valid input, NULL output, 0 output length */
            "C:\\", NULL, 0,
            ERROR_INVALID_PARAMETER, ERROR_FILENAME_EXCED_RANGE /* winxp */
        },
        { /* valid input, valid output, 0 output length */
            "C:\\", "C:\\", 0,
            ERROR_INVALID_PARAMETER, ERROR_FILENAME_EXCED_RANGE /* winxp */
        },
        { /* valid input, valid output, 1 output length */
            "C:\\", "C:\\", 1,
            ERROR_FILENAME_EXCED_RANGE, NO_ERROR
        },
        { /* test 5: valid input, valid output, valid output length */
            "C:\\", "C:\\", sizeof(volume_path),
            NO_ERROR, NO_ERROR
        },
        { /* lowercase input, uppercase output, valid output length */
            "c:\\", "C:\\", sizeof(volume_path),
            NO_ERROR, NO_ERROR
        },
        { /* really bogus input, valid output, 1 output length */
            "\\\\$$$", "C:\\", 1,
            ERROR_INVALID_NAME, ERROR_FILENAME_EXCED_RANGE
        },
        { /* a reasonable DOS path that is guaranteed to exist */
            "C:\\windows\\system32", "C:\\", sizeof(volume_path),
            NO_ERROR, NO_ERROR
        },
        { /* a reasonable DOS path that shouldn't exist */
            "C:\\windows\\system32\\AnInvalidFolder", "C:\\", sizeof(volume_path),
            NO_ERROR, NO_ERROR
        },
        { /* test 10: a reasonable NT-converted DOS path that shouldn't exist */
            "\\\\?\\C:\\AnInvalidFolder", "\\\\?\\C:\\", sizeof(volume_path),
            NO_ERROR, NO_ERROR
        },
        { /* an unreasonable NT-converted DOS path */
            "\\\\?\\InvalidDrive:\\AnInvalidFolder", "\\\\?\\InvalidDrive:\\" /* win2k, winxp */,
            sizeof(volume_path),
            ERROR_INVALID_NAME, NO_ERROR
        },
        { /* an unreasonable NT volume path */
            "\\\\?\\Volume{00000000-00-0000-0000-000000000000}\\AnInvalidFolder",
            "\\\\?\\Volume{00000000-00-0000-0000-000000000000}\\" /* win2k, winxp */,
            sizeof(volume_path),
            ERROR_INVALID_NAME, NO_ERROR
        },
        { /* an unreasonable NT-ish path */
            "\\\\ReallyBogus\\InvalidDrive:\\AnInvalidFolder",
            "\\\\ReallyBogus\\InvalidDrive:\\" /* win2k, winxp */, sizeof(volume_path),
            ERROR_INVALID_NAME, NO_ERROR
        },
        {
            "M::", "C:\\", 4,
            ERROR_FILE_NOT_FOUND, ERROR_MORE_DATA
        },
        { /* test 15 */
            "C:", "C:", 2,
            ERROR_FILENAME_EXCED_RANGE, NO_ERROR
        },
        {
            "C:", "C:", 3,
            NO_ERROR, ERROR_FILENAME_EXCED_RANGE
        },
        {
            "C:\\", "C:", 2,
            ERROR_FILENAME_EXCED_RANGE, NO_ERROR
        },
        {
            "C:\\", "C:", 3,
            NO_ERROR, ERROR_FILENAME_EXCED_RANGE
        },
        {
            "C::", "C:", 2,
            ERROR_FILENAME_EXCED_RANGE, NO_ERROR
        },
        { /* test 20 */
            "C::", "C:", 3,
            NO_ERROR, ERROR_FILENAME_EXCED_RANGE
        },
        {
            "C:\\windows\\system32\\AnInvalidFolder", "C:", 3,
            NO_ERROR, ERROR_FILENAME_EXCED_RANGE
        },
        {
            "\\\\?\\C:\\AnInvalidFolder", "\\\\?\\C:", 3,
            ERROR_FILENAME_EXCED_RANGE, NO_ERROR
        },
        {
            "\\\\?\\C:\\AnInvalidFolder", "\\\\?\\C:", 6,
            ERROR_FILENAME_EXCED_RANGE, NO_ERROR
        },
        {
            "\\\\?\\C:\\AnInvalidFolder", "\\\\?\\C:", 7,
            NO_ERROR, ERROR_FILENAME_EXCED_RANGE
        },
        { /* test 25 */
            "\\\\?\\c:\\AnInvalidFolder", "\\\\?\\c:", 7,
            NO_ERROR, ERROR_FILENAME_EXCED_RANGE
        },
        {
            "C:/", "C:\\", 4,
            NO_ERROR, ERROR_MORE_DATA
        },
        {
            "M:/", "", 4,
            ERROR_FILE_NOT_FOUND, ERROR_MORE_DATA
        },
        {
            "?:ABC:DEF:\\AnInvalidFolder", "?:\\" /* win2k, winxp */, sizeof(volume_path),
            ERROR_FILE_NOT_FOUND, NO_ERROR
        },
        {
            "s:omefile", "S:\\" /* win2k, winxp */, sizeof(volume_path),
            ERROR_FILE_NOT_FOUND, NO_ERROR
        },
        { /* test 30: a reasonable forward slash path that is guaranteed to exist */
            "C:/windows/system32", "C:\\", sizeof(volume_path),
            NO_ERROR, NO_ERROR
        },
    };

    static const char *relative_tests[] =
    {
        "InvalidDrive:\\AnInvalidFolder",
        "relative/path",
        "somefile:def",
    };

    static const char *global_prefix_tests[] =
    {
        "\\??\\CdRom0",
        "\\??\\ReallyBogus",
        "\\??\\C:\\NonExistent",
        "\\??\\M:\\NonExistent",
    };

    BOOL ret, success;
    DWORD error;
    UINT i;

    for (i=0; i<ARRAY_SIZE(test_paths); i++)
    {
        BOOL broken_ret = test_paths[i].broken_error == NO_ERROR;
        char *output = (test_paths[i].path_name != NULL ? volume_path : NULL);
        BOOL expected_ret = test_paths[i].error == NO_ERROR;

        volume_path[0] = 0;
        if (test_paths[i].path_len < sizeof(volume_path))
            volume_path[ test_paths[i].path_len ] = 0x11;

        SetLastError( 0xdeadbeef );
        ret = GetVolumePathNameA( test_paths[i].file_name, output, test_paths[i].path_len );
        error = GetLastError();
        ok(ret == expected_ret || broken(ret == broken_ret),
                                "GetVolumePathName test %d %s unexpectedly.\n",
                                i, test_paths[i].error == NO_ERROR ? "failed" : "succeeded");

        if (ret)
        {
            ok(!strcmp( volume_path, test_paths[i].path_name )
                    || broken(!strcasecmp( volume_path, test_paths[i].path_name )), /* XP */
                    "GetVolumePathName test %d unexpectedly returned path %s (expected %s).\n",
                    i, volume_path, test_paths[i].path_name);
        }
        else
        {
            /* On success Windows always returns ERROR_MORE_DATA, so only worry about failure */
            success = (error == test_paths[i].error || broken(error == test_paths[i].broken_error));
            ok(success, "GetVolumePathName test %d unexpectedly returned error 0x%lx (expected 0x%lx).\n",
                        i, error, test_paths[i].error);
        }

        if (test_paths[i].path_len < sizeof(volume_path))
            ok(volume_path[ test_paths[i].path_len ] == 0x11,
               "GetVolumePathName test %d corrupted byte after end of buffer.\n", i);
    }

    ret = GetCurrentDirectoryA( sizeof(cwd), cwd );
    ok(ret, "Failed to obtain the current working directory, error %lu.\n", GetLastError());
    ret = GetVolumePathNameA( cwd, expect_path, sizeof(expect_path) );
    ok(ret, "Failed to obtain the current volume path, error %lu.\n", GetLastError());

    for (i = 0; i < ARRAY_SIZE(relative_tests); i++)
    {
        ret = GetVolumePathNameA( relative_tests[i], volume_path, sizeof(volume_path) );
        ok(ret, "GetVolumePathName(%s) failed unexpectedly, error %lu.\n",
                debugstr_a( relative_tests[i] ), GetLastError());
        ok(!strcmp( volume_path, expect_path ), "%s: expected %s, got %s.\n",
                debugstr_a( relative_tests[i] ), debugstr_a( expect_path ), debugstr_a( volume_path ));
    }

    cwd[3] = 0;
    for (i = 0; i < ARRAY_SIZE(global_prefix_tests); i++)
    {
        ret = GetVolumePathNameA( global_prefix_tests[i], volume_path, sizeof(volume_path) );
        ok(ret, "GetVolumePathName(%s) failed unexpectedly, error %lu.\n",
                debugstr_a( global_prefix_tests[i] ), GetLastError());
        ok(!strcmp( volume_path, cwd ), "%s: expected %s, got %s.\n",
                debugstr_a( global_prefix_tests[i] ), debugstr_a( cwd ), debugstr_a( volume_path ));
    }

    ret = GetVolumePathNameA( "C:.", expect_path, sizeof(expect_path) );
    ok(ret, "Failed to obtain the volume path, error %lu.\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = GetVolumePathNameA( "C::", volume_path, 1 );
    ok(!ret, "Expected failure.\n");
    ok(GetLastError() == ERROR_FILENAME_EXCED_RANGE, "Got error %lu.\n", GetLastError());

    ret = GetVolumePathNameA( "C::", volume_path, sizeof(volume_path) );
    ok(ret, "Failed to obtain the volume path, error %lu.\n", GetLastError());
    ok(!strcmp(volume_path, expect_path), "Expected %s, got %s.\n",
            debugstr_a( expect_path ), debugstr_a( volume_path ));

    ret = GetVolumePathNameA( "C:ABC:DEF:\\AnInvalidFolder", volume_path, sizeof(volume_path) );
    ok(ret, "Failed to obtain the volume path, error %lu.\n", GetLastError());
    ok(!strcmp(volume_path, expect_path), "Expected %s, got %s.\n",
            debugstr_a( expect_path ), debugstr_a( volume_path ));
}

static void test_GetVolumePathNameW(void)
{
    WCHAR volume_path[MAX_PATH];
    BOOL ret;

    volume_path[0] = 0;
    volume_path[1] = 0x11;
    ret = GetVolumePathNameW( L"C:\\", volume_path, 1 );
    ok(!ret, "GetVolumePathNameW test succeeded unexpectedly.\n");
    ok(GetLastError() == ERROR_FILENAME_EXCED_RANGE, "GetVolumePathNameW unexpectedly returned error 0x%lx (expected 0x%x).\n",
        GetLastError(), ERROR_FILENAME_EXCED_RANGE);
    ok(volume_path[1] == 0x11, "GetVolumePathW corrupted byte after end of buffer.\n");

    volume_path[0] = 0;
    volume_path[2] = 0x11;
    ret = GetVolumePathNameW( L"C:\\", volume_path, 2 );
    ok(!ret, "GetVolumePathNameW test succeeded unexpectedly.\n");
    ok(GetLastError() == ERROR_FILENAME_EXCED_RANGE, "GetVolumePathNameW unexpectedly returned error 0x%lx (expected 0x%x).\n",
        GetLastError(), ERROR_FILENAME_EXCED_RANGE);
    ok(volume_path[2] == 0x11, "GetVolumePathW corrupted byte after end of buffer.\n");

    volume_path[0] = 0;
    volume_path[3] = 0x11;
    ret = GetVolumePathNameW( L"C:\\", volume_path, 3 );
    ok(ret || broken(!ret) /* win2k */, "GetVolumePathNameW test failed unexpectedly.\n");
    ok(!memcmp(volume_path, L"C:\\", 3) || broken(!volume_path[0]) /* XP */,
            "Got wrong path %s.\n", debugstr_w(volume_path));
    ok(volume_path[3] == 0x11, "GetVolumePathW corrupted byte after end of buffer.\n");

    volume_path[0] = 0;
    volume_path[4] = 0x11;
    ret = GetVolumePathNameW( L"C:\\", volume_path, 4 );
    ok(ret, "GetVolumePathNameW test failed unexpectedly.\n");
    ok(!wcscmp(volume_path, L"C:\\"), "Got wrong path %s.\n", debugstr_w(volume_path));
    ok(volume_path[4] == 0x11, "GetVolumePathW corrupted byte after end of buffer.\n");
}

static void test_GetVolumePathNamesForVolumeNameA(void)
{
    BOOL ret;
    char volume[MAX_PATH], buffer[MAX_PATH];
    DWORD len, error;

    if (!pGetVolumePathNamesForVolumeNameA)
    {
        win_skip("required functions not found\n");
        return;
    }

    ret = GetVolumeNameForVolumeMountPointA( "c:\\", volume, sizeof(volume) );
    ok(ret, "failed to get volume name %lu\n", GetLastError());
    trace("c:\\ -> %s\n", volume);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( NULL, NULL, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %lu\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( "", NULL, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %lu\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( volume, NULL, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_MORE_DATA, "expected ERROR_MORE_DATA got %lu\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( volume, buffer, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_MORE_DATA, "expected ERROR_MORE_DATA got %lu\n", error);

    memset( buffer, 0xff, sizeof(buffer) );
    ret = pGetVolumePathNamesForVolumeNameA( volume, buffer, sizeof(buffer), NULL );
    ok(ret, "failed to get path names %lu\n", GetLastError());
    ok(!strcmp( "C:\\", buffer ), "expected \"\\C:\" got \"%s\"\n", buffer);
    ok(!buffer[4], "expected double null-terminated buffer\n");

    len = 0;
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( NULL, NULL, 0, &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %lu\n", error);

    len = 0;
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( NULL, NULL, sizeof(buffer), &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %lu\n", error);

    len = 0;
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( NULL, buffer, sizeof(buffer), &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %lu\n", error);

    len = 0;
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( NULL, buffer, sizeof(buffer), &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %lu\n", error);

    len = 0;
    memset( buffer, 0xff, sizeof(buffer) );
    ret = pGetVolumePathNamesForVolumeNameA( volume, buffer, sizeof(buffer), &len );
    ok(ret, "failed to get path names %lu\n", GetLastError());
    ok(len == 5 || broken(len == 2), "expected 5 got %lu\n", len);
    ok(!strcmp( "C:\\", buffer ), "expected \"\\C:\" got \"%s\"\n", buffer);
    ok(!buffer[4], "expected double null-terminated buffer\n");
}

static void test_GetVolumePathNamesForVolumeNameW(void)
{
    static const WCHAR empty[] = {0};
    static const WCHAR drive_c[] = {'c',':','\\',0};
    static const WCHAR volume_null[] = {'\\','\\','?','\\','V','o','l','u','m','e',
        '{','0','0','0','0','0','0','0','0','-','0','0','0','0','-','0','0','0','0',
        '-','0','0','0','0','-','0','0','0','0','0','0','0','0','0','0','0','0','}','\\',0};
    BOOL ret;
    WCHAR volume[MAX_PATH], buffer[MAX_PATH];
    DWORD len, error;

    if (!pGetVolumePathNamesForVolumeNameW)
    {
        win_skip("required functions not found\n");
        return;
    }

    ret = GetVolumeNameForVolumeMountPointW( drive_c, volume, ARRAY_SIZE(volume) );
    ok(ret, "failed to get volume name %lu\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameW( empty, NULL, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %lu\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameW( volume, NULL, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_MORE_DATA, "expected ERROR_MORE_DATA got %lu\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameW( volume, buffer, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_MORE_DATA, "expected ERROR_MORE_DATA got %lu\n", error);

    if (0) { /* crash */
    ret = pGetVolumePathNamesForVolumeNameW( volume, NULL, ARRAY_SIZE(buffer), NULL );
    ok(ret, "failed to get path names %lu\n", GetLastError());
    }

    ret = pGetVolumePathNamesForVolumeNameW( volume, buffer, ARRAY_SIZE(buffer), NULL );
    ok(ret, "failed to get path names %lu\n", GetLastError());

    len = 0;
    memset( buffer, 0xff, sizeof(buffer) );
    ret = pGetVolumePathNamesForVolumeNameW( volume, buffer, ARRAY_SIZE(buffer), &len );
    ok(ret, "failed to get path names %lu\n", GetLastError());
    ok(len == 5, "expected 5 got %lu\n", len);
    ok(!buffer[4], "expected double null-terminated buffer\n");

    len = 0;
    volume[1] = '?';
    volume[lstrlenW( volume ) - 1] = 0;
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameW( volume, buffer, ARRAY_SIZE(buffer), &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %lu\n", error);

    len = 0;
    volume[0] = '\\';
    volume[1] = 0;
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameW( volume, buffer, ARRAY_SIZE(buffer), &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    todo_wine ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %lu\n", error);

    len = 0;
    lstrcpyW( volume, volume_null );
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameW( volume, buffer, ARRAY_SIZE(buffer), &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND got %lu\n", error);
}

static void test_dvd_read_structure(HANDLE handle)
{
    int i;
    DWORD nbBytes;
    BOOL ret;
    DVD_READ_STRUCTURE dvdReadStructure;
    DVD_LAYER_DESCRIPTOR dvdLayerDescriptor;
    struct COMPLETE_DVD_LAYER_DESCRIPTOR completeDvdLayerDescriptor;
    DVD_COPYRIGHT_DESCRIPTOR dvdCopyrightDescriptor;
    struct COMPLETE_DVD_MANUFACTURER_DESCRIPTOR completeDvdManufacturerDescriptor;

    dvdReadStructure.BlockByteOffset.QuadPart = 0;
    dvdReadStructure.SessionId = 0;
    dvdReadStructure.LayerNumber = 0;


    /* DvdPhysicalDescriptor */
    dvdReadStructure.Format = 0;

    SetLastError(0xdeadbeef);

    /* Test whether this ioctl is supported */
    ret = DeviceIoControl(handle, IOCTL_DVD_READ_STRUCTURE, &dvdReadStructure, sizeof(DVD_READ_STRUCTURE),
        &completeDvdLayerDescriptor, sizeof(struct COMPLETE_DVD_LAYER_DESCRIPTOR), &nbBytes, NULL);

    if(!ret)
    {
        skip("IOCTL_DVD_READ_STRUCTURE not supported: %lu\n", GetLastError());
        return;
    }

    /* Confirm there is always a header before the actual data */
    ok( completeDvdLayerDescriptor.Header.Length == 0x0802, "Length is 0x%04x instead of 0x0802\n", completeDvdLayerDescriptor.Header.Length);
    ok( completeDvdLayerDescriptor.Header.Reserved[0] == 0, "Reserved[0] is %x instead of 0\n", completeDvdLayerDescriptor.Header.Reserved[0]);
    ok( completeDvdLayerDescriptor.Header.Reserved[1] == 0, "Reserved[1] is %x instead of 0\n", completeDvdLayerDescriptor.Header.Reserved[1]);

    /* TODO: Also check completeDvdLayerDescriptor.Descriptor content (via IOCTL_SCSI_PASS_THROUGH_DIRECT ?) */

    /* Insufficient output buffer */
    for(i=0; i<sizeof(DVD_DESCRIPTOR_HEADER); i++)
    {
        SetLastError(0xdeadbeef);

        ret = DeviceIoControl(handle, IOCTL_DVD_READ_STRUCTURE, &dvdReadStructure, sizeof(DVD_READ_STRUCTURE),
            &completeDvdLayerDescriptor, i, &nbBytes, NULL);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,"IOCTL_DVD_READ_STRUCTURE should fail with small buffer\n");
    }

    SetLastError(0xdeadbeef);

    /* On newer version, an output buffer of sizeof(DVD_READ_STRUCTURE) size fails.
        I think this is to force developers to realize that there is a header before the actual content */
    ret = DeviceIoControl(handle, IOCTL_DVD_READ_STRUCTURE, &dvdReadStructure, sizeof(DVD_READ_STRUCTURE),
        &dvdLayerDescriptor, sizeof(DVD_LAYER_DESCRIPTOR), &nbBytes, NULL);
    ok( (!ret && GetLastError() == ERROR_INVALID_PARAMETER) || broken(ret) /* < Win7 */,
        "IOCTL_DVD_READ_STRUCTURE should have failed\n");

    SetLastError(0xdeadbeef);

    ret = DeviceIoControl(handle, IOCTL_DVD_READ_STRUCTURE, NULL, sizeof(DVD_READ_STRUCTURE),
        &completeDvdLayerDescriptor, sizeof(struct COMPLETE_DVD_LAYER_DESCRIPTOR), &nbBytes, NULL);
    ok( (!ret && GetLastError() == ERROR_INVALID_PARAMETER),
        "IOCTL_DVD_READ_STRUCTURE should have failed\n");

    /* Test wrong input parameters */
    for(i=0; i<sizeof(DVD_READ_STRUCTURE); i++)
    {
        SetLastError(0xdeadbeef);

        ret = DeviceIoControl(handle, IOCTL_DVD_READ_STRUCTURE, &dvdReadStructure, i,
        &completeDvdLayerDescriptor, sizeof(struct COMPLETE_DVD_LAYER_DESCRIPTOR), &nbBytes, NULL);
        ok( (!ret && GetLastError() == ERROR_INVALID_PARAMETER),
            "IOCTL_DVD_READ_STRUCTURE should have failed\n");
    }


    /* DvdCopyrightDescriptor */
    dvdReadStructure.Format = 1;

    SetLastError(0xdeadbeef);

    /* Strangely, with NULL lpOutBuffer, last error is insufficient buffer, not invalid parameter as we could expect */
    ret = DeviceIoControl(handle, IOCTL_DVD_READ_STRUCTURE, &dvdReadStructure, sizeof(DVD_READ_STRUCTURE),
        NULL, sizeof(DVD_COPYRIGHT_DESCRIPTOR), &nbBytes, NULL);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "IOCTL_DVD_READ_STRUCTURE should have failed %d %lu\n", ret, GetLastError());

    for(i=0; i<sizeof(DVD_COPYRIGHT_DESCRIPTOR); i++)
    {
        SetLastError(0xdeadbeef);

        ret = DeviceIoControl(handle, IOCTL_DVD_READ_STRUCTURE, &dvdReadStructure, sizeof(DVD_READ_STRUCTURE),
            &dvdCopyrightDescriptor, i, &nbBytes, NULL);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "IOCTL_DVD_READ_STRUCTURE should have failed %d %lu\n", ret, GetLastError());
    }


    /* DvdManufacturerDescriptor */
    dvdReadStructure.Format = 4;

    SetLastError(0xdeadbeef);

    ret = DeviceIoControl(handle, IOCTL_DVD_READ_STRUCTURE, &dvdReadStructure, sizeof(DVD_READ_STRUCTURE),
        &completeDvdManufacturerDescriptor, sizeof(DVD_MANUFACTURER_DESCRIPTOR), &nbBytes, NULL);
    ok(ret || broken(GetLastError() == ERROR_NOT_READY),
        "IOCTL_DVD_READ_STRUCTURE (DvdManufacturerDescriptor) failed, last error = %lu\n", GetLastError());
    if(!ret)
        return;

    /* Confirm there is always a header before the actual data */
    ok( completeDvdManufacturerDescriptor.Header.Length == 0x0802, "Length is 0x%04x instead of 0x0802\n", completeDvdManufacturerDescriptor.Header.Length);
    ok( completeDvdManufacturerDescriptor.Header.Reserved[0] == 0, "Reserved[0] is %x instead of 0\n", completeDvdManufacturerDescriptor.Header.Reserved[0]);
    ok( completeDvdManufacturerDescriptor.Header.Reserved[1] == 0, "Reserved[1] is %x instead of 0\n", completeDvdManufacturerDescriptor.Header.Reserved[1]);

    SetLastError(0xdeadbeef);

    /* Basic parameter check */
    ret = DeviceIoControl(handle, IOCTL_DVD_READ_STRUCTURE, &dvdReadStructure, sizeof(DVD_READ_STRUCTURE),
        NULL, sizeof(DVD_MANUFACTURER_DESCRIPTOR), &nbBytes, NULL);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "IOCTL_DVD_READ_STRUCTURE should have failed %d %lu\n", ret, GetLastError());
}

static void test_cdrom_ioctl(void)
{
    char drive_letter, drive_path[] = "A:\\", drive_full_path[] = "\\\\.\\A:";
    DWORD bitmask;
    HANDLE handle;

    bitmask = GetLogicalDrives();
    if(!bitmask)
    {
        trace("GetLogicalDrives failed : %lu\n", GetLastError());
        return;
    }

    for(drive_letter='A'; drive_letter<='Z'; drive_letter++)
    {
        if(!(bitmask & (1 << (drive_letter-'A') )))
            continue;

        drive_path[0] = drive_letter;
        if(GetDriveTypeA(drive_path) != DRIVE_CDROM)
        {
            trace("Skipping %c:, not a CDROM drive.\n", drive_letter);
            continue;
        }

        trace("Testing with %c:\n", drive_letter);

        drive_full_path[4] = drive_letter;
        handle = CreateFileA(drive_full_path, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
        if(handle == INVALID_HANDLE_VALUE)
        {
            trace("Failed to open the device : %lu\n", GetLastError());
            continue;
        }

        /* Add your tests here */
        test_dvd_read_structure(handle);

        CloseHandle(handle);
    }

}

static void test_mounted_folder(void)
{
    char name_buffer[200], path[MAX_PATH], volume_name[100], *p;
    FILE_NAME_INFORMATION *name = (FILE_NAME_INFORMATION *)name_buffer;
    FILE_ATTRIBUTE_TAG_INFO info;
    IO_STATUS_BLOCK io;
    BOOL ret, got_path;
    NTSTATUS status;
    HANDLE file;
    DWORD size;

    file = CreateFileA( "C:\\", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL );
    ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    status = NtQueryInformationFile( file, &io, &info, sizeof(info), FileAttributeTagInformation );
    ok(!status, "got status %#lx\n", status);
    ok(!(info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            && (info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY), "got attributes %#lx\n", info.FileAttributes);
    ok(!info.ReparseTag, "got reparse tag %#lx\n", info.ReparseTag);

    CloseHandle( file );

    file = CreateFileA( "C:\\", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    status = NtQueryInformationFile( file, &io, &info, sizeof(info), FileAttributeTagInformation );
    ok(!status, "got status %#lx\n", status);
    ok(!(info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            && (info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY), "got attributes %#lx\n", info.FileAttributes);
    ok(!info.ReparseTag, "got reparse tag %#lx\n", info.ReparseTag);

    CloseHandle( file );

    ret = GetFileAttributesA( "C:\\" );
    ok(!(ret & FILE_ATTRIBUTE_REPARSE_POINT) && (ret & FILE_ATTRIBUTE_DIRECTORY), "got attributes %#x\n", ret);

    ret = CreateDirectoryA("C:\\winetest_mnt", NULL);
    if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
    {
        skip("Not enough permissions to create a folder in the C: drive.\n");
        return;
    }
    ok(ret, "got error %lu\n", GetLastError());

    ret = GetVolumeNameForVolumeMountPointA( "C:\\", volume_name, sizeof(volume_name) );
    ok(ret, "got error %lu\n", GetLastError());

    ret = SetVolumeMountPointA( "C:\\winetest_mnt\\", volume_name );
    if (!ret)
    {
        skip("Not enough permissions to create a mounted folder.\n");
        RemoveDirectoryA( "C:\\winetest_mnt" );
        return;
    }
    todo_wine ok(ret, "got error %lu\n", GetLastError());

    file = CreateFileA( "C:\\winetest_mnt", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OPEN_REPARSE_POINT, NULL );
    ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    status = NtQueryInformationFile( file, &io, &info, sizeof(info), FileAttributeTagInformation );
    ok(!status, "got status %#lx\n", status);
    ok((info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            && (info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY), "got attributes %#lx\n", info.FileAttributes);
    ok(info.ReparseTag == IO_REPARSE_TAG_MOUNT_POINT, "got reparse tag %#lx\n", info.ReparseTag);

    status = NtQueryInformationFile( file, &io, name, sizeof(name_buffer), FileNameInformation );
    ok(!status, "got status %#lx\n", status);
    ok(name->FileNameLength == wcslen(L"\\winetest_mnt") * sizeof(WCHAR), "got length %lu\n", name->FileNameLength);
    ok(!wcsnicmp(name->FileName, L"\\winetest_mnt", wcslen(L"\\winetest_mnt")), "got name %s\n",
            debugstr_wn(name->FileName, name->FileNameLength / sizeof(WCHAR)));

    CloseHandle( file );

    file = CreateFileA( "C:\\winetest_mnt", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    status = NtQueryInformationFile( file, &io, &info, sizeof(info), FileAttributeTagInformation );
    ok(!status, "got status %#lx\n", status);
    ok(!(info.FileAttributes & FILE_ATTRIBUTE_REPARSE_POINT)
            && (info.FileAttributes & FILE_ATTRIBUTE_DIRECTORY), "got attributes %#lx\n", info.FileAttributes);
    ok(!info.ReparseTag, "got reparse tag %#lx\n", info.ReparseTag);

    status = NtQueryInformationFile( file, &io, name, sizeof(name_buffer), FileNameInformation );
    ok(!status, "got status %#lx\n", status);
    ok(name->FileNameLength == wcslen(L"\\") * sizeof(WCHAR), "got length %lu\n", name->FileNameLength);
    ok(!wcsnicmp(name->FileName, L"\\", wcslen(L"\\")), "got name %s\n",
            debugstr_wn(name->FileName, name->FileNameLength / sizeof(WCHAR)));

    CloseHandle( file );

    ret = GetFileAttributesA( "C:\\winetest_mnt" );
    ok(ret != INVALID_FILE_ATTRIBUTES, "got error %lu\n", GetLastError());
    ok((ret & FILE_ATTRIBUTE_REPARSE_POINT) && (ret & FILE_ATTRIBUTE_DIRECTORY), "got attributes %#x\n", ret);

    file = CreateFileA( "C:\\winetest_mnt\\windows", 0, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

    status = NtQueryInformationFile( file, &io, name, sizeof(name_buffer), FileNameInformation );
    ok(!status, "got status %#lx\n", status);
    ok(name->FileNameLength == wcslen(L"\\windows") * sizeof(WCHAR), "got length %lu\n", name->FileNameLength);
    ok(!wcsnicmp(name->FileName, L"\\windows", wcslen(L"\\windows")), "got name %s\n",
            debugstr_wn(name->FileName, name->FileNameLength / sizeof(WCHAR)));

    CloseHandle( file );

    ret = GetVolumePathNameA( "C:\\winetest_mnt", path, sizeof(path) );
    ok(ret, "got error %lu\n", GetLastError());
    ok(!strcmp(path, "C:\\winetest_mnt\\"), "got %s\n", debugstr_a(path));
    SetLastError(0xdeadbeef);
    ret = GetVolumeNameForVolumeMountPointA( "C:\\winetest_mnt", path, sizeof(path) );
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "wrong error %lu\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = GetVolumeInformationA( "C:\\winetest_mnt", NULL, 0, NULL, NULL, NULL, NULL, 0 );
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_NAME, "wrong error %lu\n", GetLastError());

    ret = GetVolumeNameForVolumeMountPointA( "C:\\winetest_mnt\\", path, sizeof(path) );
    ok(ret, "got error %lu\n", GetLastError());
    ok(!strcmp(path, volume_name), "expected %s, got %s\n", debugstr_a(volume_name), debugstr_a(path));
    ret = GetVolumeInformationA( "C:\\winetest_mnt\\", NULL, 0, NULL, NULL, NULL, NULL, 0 );
    ok(ret, "got error %lu\n", GetLastError());

    ret = GetVolumePathNameA( "C:\\winetest_mnt\\windows", path, sizeof(path) );
    ok(ret, "got error %lu\n", GetLastError());
    ok(!strcmp(path, "C:\\winetest_mnt\\"), "got %s\n", debugstr_a(path));
    SetLastError(0xdeadbeef);
    ret = GetVolumeNameForVolumeMountPointA( "C:\\winetest_mnt\\windows\\", path, sizeof(path) );
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_NOT_A_REPARSE_POINT, "wrong error %lu\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = GetVolumeInformationA( "C:\\winetest_mnt\\windows\\", NULL, 0, NULL, NULL, NULL, NULL, 0 );
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_DIR_NOT_ROOT, "wrong error %lu\n", GetLastError());

    ret = GetVolumePathNameA( "C:\\winetest_mnt\\nonexistent\\", path, sizeof(path) );
    ok(ret, "got error %lu\n", GetLastError());
    ok(!strcmp(path, "C:\\winetest_mnt\\"), "got %s\n", debugstr_a(path));
    SetLastError(0xdeadbeef);
    ret = GetVolumeNameForVolumeMountPointA( "C:\\winetest_mnt\\nonexistent\\", path, sizeof(path) );
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %lu\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = GetVolumeInformationA( "C:\\winetest_mnt\\nonexistent\\", NULL, 0, NULL, NULL, NULL, NULL, 0 );
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %lu\n", GetLastError());

    ret = GetVolumePathNameA( "C:\\winetest_mnt\\winetest_mnt", path, sizeof(path) );
    ok(ret, "got error %lu\n", GetLastError());
    ok(!strcmp(path, "C:\\winetest_mnt\\winetest_mnt\\"), "got %s\n", debugstr_a(path));
    ret = GetVolumeNameForVolumeMountPointA( "C:\\winetest_mnt\\winetest_mnt\\", path, sizeof(path) );
    ok(ret, "got error %lu\n", GetLastError());
    ok(!strcmp(path, volume_name), "expected %s, got %s\n", debugstr_a(volume_name), debugstr_a(path));
    ret = GetVolumeInformationA( "C:\\winetest_mnt\\winetest_mnt\\", NULL, 0, NULL, NULL, NULL, NULL, 0 );
    ok(ret, "got error %lu\n", GetLastError());

    ret = GetVolumePathNameA( "C:/winetest_mnt/../winetest_mnt/.", path, sizeof(path) );
    ok(ret, "got error %lu\n", GetLastError());
    ok(!strcmp(path, "C:\\winetest_mnt\\"), "got %s\n", debugstr_a(path));
    ret = GetVolumeNameForVolumeMountPointA( "C:/winetest_mnt/../winetest_mnt/.\\", path, sizeof(path) );
    ok(ret, "got error %lu\n", GetLastError());
    ok(!strcmp(path, volume_name), "expected %s, got %s\n", debugstr_a(volume_name), debugstr_a(path));
    ret = GetVolumeInformationA( "C:/winetest_mnt/../winetest_mnt/.\\", NULL, 0, NULL, NULL, NULL, NULL, 0 );
    ok(ret, "got error %lu\n", GetLastError());

    ret = GetVolumePathNamesForVolumeNameA( volume_name, path, sizeof(path), &size );
    ok(ret, "got error %lu\n", GetLastError());
    got_path = FALSE;
    for (p = path; *p; p += strlen(p) + 1)
    {
        if (!strcmp( p, "C:\\winetest_mnt\\" ))
            got_path = TRUE;
        ok(strcmp( p, "C:\\winetest_mnt\\winetest_mnt\\" ), "GetVolumePathNamesForVolumeName() should not recurse\n");
    }
    ok(got_path, "mount point was not enumerated\n");

    /* test interaction with symbolic links */

    if (pCreateSymbolicLinkA)
    {
        ret = pCreateSymbolicLinkA( "C:\\winetest_link", "C:\\winetest_mnt\\", SYMBOLIC_LINK_FLAG_DIRECTORY );
        ok(ret, "got error %lu\n", GetLastError());

        ret = GetVolumePathNameA( "C:\\winetest_link\\", path, sizeof(path) );
        ok(ret, "got error %lu\n", GetLastError());
        ok(!strcmp(path, "C:\\"), "got %s\n", path);
        SetLastError(0xdeadbeef);
        ret = GetVolumeNameForVolumeMountPointA( "C:\\winetest_link\\", path, sizeof(path) );
        ok(!ret, "expected failure\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER
                || broken(GetLastError() == ERROR_SUCCESS) /* 2008 */, "wrong error %lu\n", GetLastError());
        ret = GetVolumeInformationA( "C:\\winetest_link\\", NULL, 0, NULL, NULL, NULL, NULL, 0 );
        ok(ret, "got error %lu\n", GetLastError());

        ret = GetVolumePathNameA( "C:\\winetest_link\\windows\\", path, sizeof(path) );
        ok(ret, "got error %lu\n", GetLastError());
        ok(!strcmp(path, "C:\\"), "got %s\n", path);
        SetLastError(0xdeadbeef);
        ret = GetVolumeNameForVolumeMountPointA( "C:\\winetest_link\\windows\\", path, sizeof(path) );
        ok(!ret, "expected failure\n");
        ok(GetLastError() == ERROR_NOT_A_REPARSE_POINT, "wrong error %lu\n", GetLastError());
        SetLastError(0xdeadbeef);
        ret = GetVolumeInformationA( "C:\\winetest_link\\windows\\", NULL, 0, NULL, NULL, NULL, NULL, 0 );
        ok(!ret, "expected failure\n");
        ok(GetLastError() == ERROR_DIR_NOT_ROOT, "wrong error %lu\n", GetLastError());

        ret = GetVolumePathNameA( "C:\\winetest_link\\winetest_mnt", path, sizeof(path) );
        ok(ret, "got error %lu\n", GetLastError());
        ok(!strcmp(path, "C:\\winetest_link\\winetest_mnt\\"), "got %s\n", debugstr_a(path));
        ret = GetVolumeNameForVolumeMountPointA( "C:\\winetest_link\\winetest_mnt\\", path, sizeof(path) );
        ok(ret, "got error %lu\n", GetLastError());
        ok(!strcmp(path, volume_name), "expected %s, got %s\n", debugstr_a(volume_name), debugstr_a(path));
        ret = GetVolumeInformationA( "C:\\winetest_link\\winetest_mnt\\", NULL, 0, NULL, NULL, NULL, NULL, 0 );
        ok(ret, "got error %lu\n", GetLastError());

        /* The following test makes it clear that when we encounter a symlink
         * while resolving, we resolve *every* junction in the path, i.e. both
         * mount points and symlinks. */
        ret = GetVolumePathNameA( "C:\\winetest_link\\winetest_mnt\\winetest_link\\windows\\", path, sizeof(path) );
        ok(ret, "got error %lu\n", GetLastError());
        ok(!strcmp(path, "C:\\") || !strcmp(path, "C:\\winetest_link\\winetest_mnt\\") /* 2008 */,
                "got %s\n", debugstr_a(path));

        file = CreateFileA( "C:\\winetest_link\\winetest_mnt\\winetest_link\\windows\\", 0,
                FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
        ok(file != INVALID_HANDLE_VALUE, "got error %lu\n", GetLastError());

        status = NtQueryInformationFile( file, &io, name, sizeof(name_buffer), FileNameInformation );
        ok(!status, "got status %#lx\n", status);
        ok(name->FileNameLength == wcslen(L"\\windows") * sizeof(WCHAR), "got length %lu\n", name->FileNameLength);
        ok(!wcsnicmp(name->FileName, L"\\windows", wcslen(L"\\windows")), "got name %s\n",
                debugstr_wn(name->FileName, name->FileNameLength / sizeof(WCHAR)));

        CloseHandle( file );

        ret = RemoveDirectoryA( "C:\\winetest_link\\" );
        ok(ret, "got error %lu\n", GetLastError());

        /* The following cannot be automatically tested:
         *
         * Suppose C: and D: are mount points of different volumes. If C:/a is
         * symlinked to D:/b, GetVolumePathName("C:\\a") will return "D:\\" if
         * "a" is a directory, but "C:\\" if "a" is a file.
         * Moreover, if D: is mounted at C:/mnt, and C:/a is symlinked to
         * C:/mnt, GetVolumePathName("C:\\mnt\\b") will still return "D:\\". */
    }

    ret = DeleteVolumeMountPointA( "C:\\winetest_mnt\\" );
    ok(ret, "got error %lu\n", GetLastError());
    ret = RemoveDirectoryA( "C:\\winetest_mnt" );
    ok(ret, "got error %lu\n", GetLastError());
}

static void test_GetVolumeInformationByHandle(void)
{
    char DECLSPEC_ALIGN(8) buffer[50];
    FILE_FS_ATTRIBUTE_INFORMATION *attr_info = (void *)buffer;
    FILE_FS_VOLUME_INFORMATION *volume_info = (void *)buffer;
    DWORD serial, filename_len, flags;
    WCHAR label[20], fsname[20];
    char volume[MAX_PATH+1];
    IO_STATUS_BLOCK io;
    HANDLE file;
    NTSTATUS status;
    BOOL ret;

    if (!pGetVolumeInformationByHandleW)
    {
        win_skip("GetVolumeInformationByHandleW is not present.\n");
        return;
    }

    file = CreateFileA( "C:/windows", 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    ok(file != INVALID_HANDLE_VALUE, "failed to open file, error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetVolumeInformationByHandleW( INVALID_HANDLE_VALUE, label, ARRAY_SIZE(label), &serial,
            &filename_len, &flags, fsname, ARRAY_SIZE(fsname) );
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got error %lu\n", GetLastError());

    ret = pGetVolumeInformationByHandleW( file, NULL, 0, NULL, NULL, NULL, NULL, 0 );
    ok(ret, "got error %lu\n", GetLastError());

    ret = pGetVolumeInformationByHandleW( file, label, ARRAY_SIZE(label), &serial,
            &filename_len, &flags, fsname, ARRAY_SIZE(fsname) );
    ok(ret, "got error %lu\n", GetLastError());

    memset(buffer, 0, sizeof(buffer));
    status = NtQueryVolumeInformationFile( file, &io, buffer, sizeof(buffer), FileFsAttributeInformation );
    ok(!status, "got status %#lx\n", status);
    ok(flags == attr_info->FileSystemAttributes, "expected flags %#lx, got %#lx\n",
            attr_info->FileSystemAttributes, flags);
    ok(filename_len == attr_info->MaximumComponentNameLength, "expected filename_len %lu, got %lu\n",
            attr_info->MaximumComponentNameLength, filename_len);
    ok(!wcscmp( fsname, attr_info->FileSystemName ), "expected fsname %s, got %s\n",
            debugstr_w( attr_info->FileSystemName ), debugstr_w( fsname ));
    ok(wcslen( fsname ) == attr_info->FileSystemNameLength / sizeof(WCHAR), "got %Iu\n", wcslen( fsname ));

    SetLastError(0xdeadbeef);
    ret = pGetVolumeInformationByHandleW( file, NULL, 0, NULL, &filename_len, &flags, fsname, 2 );
    ok(!ret, "expected failure\n");
    ok(GetLastError() == ERROR_BAD_LENGTH, "got error %lu\n", GetLastError());

    memset(buffer, 0, sizeof(buffer));
    status = NtQueryVolumeInformationFile( file, &io, buffer, sizeof(buffer), FileFsVolumeInformation );
    ok(!status, "got status %#lx\n", status);
    ok(serial == volume_info->VolumeSerialNumber, "expected serial %08lx, got %08lx\n",
            volume_info->VolumeSerialNumber, serial);
    ok(!wcscmp( label, volume_info->VolumeLabel ), "expected label %s, got %s\n",
            debugstr_w( volume_info->VolumeLabel ), debugstr_w( label ));
    ok(wcslen( label ) == volume_info->VolumeLabelLength / sizeof(WCHAR), "got %Iu\n", wcslen( label ));

    CloseHandle( file );

    /* get the unique volume name for the windows drive  */
    ret = GetVolumeNameForVolumeMountPointA("C:\\", volume, MAX_PATH);
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointA failed\n");

    /* try again with unique volume name */

    file = CreateFileA( volume, 0, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
            OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL );
    ok(file != INVALID_HANDLE_VALUE, "failed to open file, error %lu\n", GetLastError());

    ret = pGetVolumeInformationByHandleW( file, label, ARRAY_SIZE(label), &serial,
            &filename_len, &flags, fsname, ARRAY_SIZE(fsname) );
    ok(ret, "got error %lu\n", GetLastError());

    memset(buffer, 0, sizeof(buffer));
    status = NtQueryVolumeInformationFile( file, &io, buffer, sizeof(buffer), FileFsVolumeInformation );
    ok(!status, "got status %#lx\n", status);
    ok(serial == volume_info->VolumeSerialNumber, "expected serial %08lx, got %08lx\n",
            volume_info->VolumeSerialNumber, serial);
    ok(!wcscmp( label, volume_info->VolumeLabel ), "expected label %s, got %s\n",
            debugstr_w( volume_info->VolumeLabel ), debugstr_w( label ));
    ok(wcslen( label ) == volume_info->VolumeLabelLength / sizeof(WCHAR), "got %Iu\n", wcslen( label ));

    memset(buffer, 0, sizeof(buffer));
    status = NtQueryVolumeInformationFile( file, &io, buffer, sizeof(buffer), FileFsAttributeInformation );
    ok(!status, "got status %#lx\n", status);
    ok(flags == attr_info->FileSystemAttributes, "expected flags %#lx, got %#lx\n",
            attr_info->FileSystemAttributes, flags);
    ok(filename_len == attr_info->MaximumComponentNameLength, "expected filename_len %lu, got %lu\n",
            attr_info->MaximumComponentNameLength, filename_len);
    ok(!wcscmp( fsname, attr_info->FileSystemName ), "expected fsname %s, got %s\n",
            debugstr_w( attr_info->FileSystemName ), debugstr_w( fsname ));
    ok(wcslen( fsname ) == attr_info->FileSystemNameLength / sizeof(WCHAR), "got %Iu\n", wcslen( fsname ));

    CloseHandle( file );
}

static void test_mountmgr_query_points(void)
{
    char input_buffer[64];
    MOUNTMGR_MOUNT_POINTS *output;
    MOUNTMGR_MOUNT_POINT *input = (MOUNTMGR_MOUNT_POINT *)input_buffer;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    HANDLE file;

    output = malloc(1024);

    file = CreateFileW( MOUNTMGR_DOS_DEVICE_NAME, 0, 0, NULL, OPEN_EXISTING, 0, NULL );
    ok(file != INVALID_HANDLE_VALUE, "failed to open mountmgr, error %lu\n", GetLastError());

    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    status = NtDeviceIoControlFile( file, NULL, NULL, NULL, &io,
            IOCTL_MOUNTMGR_QUERY_POINTS, NULL, 0, NULL, 0 );
    ok(status == STATUS_INVALID_PARAMETER, "got %#lx\n", status);
    ok(io.Status == 0xdeadf00d, "got status %#lx\n", io.Status);
    ok(io.Information == 0xdeadf00d, "got information %#Ix\n", io.Information);

    memset( input, 0, sizeof(*input) );

    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    status = NtDeviceIoControlFile( file, NULL, NULL, NULL, &io,
            IOCTL_MOUNTMGR_QUERY_POINTS, input, sizeof(*input) - 1, NULL, 0 );
    ok(status == STATUS_INVALID_PARAMETER, "got %#lx\n", status);
    ok(io.Status == 0xdeadf00d, "got status %#lx\n", io.Status);
    ok(io.Information == 0xdeadf00d, "got information %#Ix\n", io.Information);

    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    status = NtDeviceIoControlFile( file, NULL, NULL, NULL, &io,
            IOCTL_MOUNTMGR_QUERY_POINTS, input, sizeof(*input), NULL, 0 );
    ok(status == STATUS_INVALID_PARAMETER, "got %#lx\n", status);
    ok(io.Status == 0xdeadf00d, "got status %#lx\n", io.Status);
    ok(io.Information == 0xdeadf00d, "got information %#Ix\n", io.Information);

    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    memset(output, 0xcc, sizeof(*output));
    status = NtDeviceIoControlFile( file, NULL, NULL, NULL, &io,
            IOCTL_MOUNTMGR_QUERY_POINTS, input, sizeof(*input), output, sizeof(*output) - 1 );
    ok(status == STATUS_INVALID_PARAMETER, "got %#lx\n", status);
    ok(io.Status == 0xdeadf00d, "got status %#lx\n", io.Status);
    ok(io.Information == 0xdeadf00d, "got information %#Ix\n", io.Information);
    ok(output->Size == 0xcccccccc, "got size %lu\n", output->Size);
    ok(output->NumberOfMountPoints == 0xcccccccc, "got count %lu\n", output->NumberOfMountPoints);

    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    memset(output, 0xcc, sizeof(*output));
    status = NtDeviceIoControlFile( file, NULL, NULL, NULL, &io,
            IOCTL_MOUNTMGR_QUERY_POINTS, input, sizeof(*input), output, sizeof(*output) );
    ok(status == STATUS_BUFFER_OVERFLOW, "got %#lx\n", status);
    ok(io.Status == STATUS_BUFFER_OVERFLOW, "got status %#lx\n", io.Status);
    todo_wine ok(io.Information == offsetof(MOUNTMGR_MOUNT_POINTS, MountPoints[0]), "got information %#Ix\n", io.Information);
    ok(output->Size > offsetof(MOUNTMGR_MOUNT_POINTS, MountPoints[0]), "got size %lu\n", output->Size);
    todo_wine ok(output->NumberOfMountPoints && output->NumberOfMountPoints != 0xcccccccc,
            "got count %lu\n", output->NumberOfMountPoints);

    output = realloc(output, output->Size);

    io.Status = 0xdeadf00d;
    io.Information = 0xdeadf00d;
    status = NtDeviceIoControlFile( file, NULL, NULL, NULL, &io,
            IOCTL_MOUNTMGR_QUERY_POINTS, input, sizeof(*input), output, output->Size );
    ok(!status, "got %#lx\n", status);
    ok(!io.Status, "got status %#lx\n", io.Status);
    ok(io.Information == output->Size, "got size %lu, information %#Ix\n", output->Size, io.Information);
    ok(output->Size > offsetof(MOUNTMGR_MOUNT_POINTS, MountPoints[0]), "got size %lu\n", output->Size);
    ok(output->NumberOfMountPoints && output->NumberOfMountPoints != 0xcccccccc,
            "got count %lu\n", output->NumberOfMountPoints);

    CloseHandle( file );

    free(output);
}

START_TEST(volume)
{
    hdll = GetModuleHandleA("kernel32.dll");
    pFindFirstVolumeA = (void *) GetProcAddress(hdll, "FindFirstVolumeA");
    pFindNextVolumeA = (void *) GetProcAddress(hdll, "FindNextVolumeA");
    pFindVolumeClose = (void *) GetProcAddress(hdll, "FindVolumeClose");
    pGetLogicalDriveStringsA = (void *) GetProcAddress(hdll, "GetLogicalDriveStringsA");
    pGetLogicalDriveStringsW = (void *) GetProcAddress(hdll, "GetLogicalDriveStringsW");
    pGetVolumePathNamesForVolumeNameA = (void *) GetProcAddress(hdll, "GetVolumePathNamesForVolumeNameA");
    pGetVolumePathNamesForVolumeNameW = (void *) GetProcAddress(hdll, "GetVolumePathNamesForVolumeNameW");
    pCreateSymbolicLinkA = (void *) GetProcAddress(hdll, "CreateSymbolicLinkA");
    pGetVolumeInformationByHandleW = (void *) GetProcAddress(hdll, "GetVolumeInformationByHandleW");

    test_query_dos_deviceA();
    test_dos_devices();
    test_FindFirstVolume();
    test_GetVolumePathNameA();
    test_GetVolumePathNameW();
    test_GetVolumeNameForVolumeMountPointA();
    test_GetVolumeNameForVolumeMountPointW();
    test_GetLogicalDriveStringsA();
    test_GetLogicalDriveStringsW();
    test_GetVolumeInformationA();
    test_enum_vols();
    test_disk_extents();
    test_disk_query_property();
    test_GetVolumePathNamesForVolumeNameA();
    test_GetVolumePathNamesForVolumeNameW();
    test_cdrom_ioctl();
    test_mounted_folder();
    test_GetVolumeInformationByHandle();
    test_mountmgr_query_points();
}
