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

#include "wine/test.h"
#include "winbase.h"
#include "winioctl.h"
#include <stdio.h>
#include "wine/ddk/ntddcdvd.h"

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
static BOOL (WINAPI * pGetVolumeNameForVolumeMountPointA)(LPCSTR, LPSTR, DWORD);
static BOOL (WINAPI * pGetVolumeNameForVolumeMountPointW)(LPCWSTR, LPWSTR, DWORD);
static HANDLE (WINAPI *pFindFirstVolumeA)(LPSTR,DWORD);
static BOOL (WINAPI *pFindNextVolumeA)(HANDLE,LPSTR,DWORD);
static BOOL (WINAPI *pFindVolumeClose)(HANDLE);
static UINT (WINAPI *pGetLogicalDriveStringsA)(UINT,LPSTR);
static UINT (WINAPI *pGetLogicalDriveStringsW)(UINT,LPWSTR);
static BOOL (WINAPI *pGetVolumeInformationA)(LPCSTR, LPSTR, DWORD, LPDWORD, LPDWORD, LPDWORD, LPSTR, DWORD);
static BOOL (WINAPI *pGetVolumePathNameA)(LPCSTR, LPSTR, DWORD);
static BOOL (WINAPI *pGetVolumePathNamesForVolumeNameA)(LPCSTR, LPSTR, DWORD, LPDWORD);
static BOOL (WINAPI *pGetVolumePathNamesForVolumeNameW)(LPCWSTR, LPWSTR, DWORD, LPDWORD);

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
       "QueryDosDeviceA(no buffer): returned %u, le=%u\n", ret, GetLastError());

    buffer = HeapAlloc( GetProcessHeap(), 0, buflen );
    SetLastError(0xdeadbeef);
    ret = QueryDosDeviceA( NULL, buffer, buflen );
    ok((ret && GetLastError() != ERROR_INSUFFICIENT_BUFFER),
        "QueryDosDeviceA failed to return list, last error %u\n", GetLastError());

    if (ret && GetLastError() != ERROR_INSUFFICIENT_BUFFER) {
        p = buffer;
        for (;;) {
            if (!strlen(p)) break;
            ret2 = QueryDosDeviceA( p, buffer2, sizeof(buffer2) );
            ok(ret2, "QueryDosDeviceA failed to return current mapping for %s, last error %u\n", p, GetLastError());
            p += strlen(p) + 1;
            if (ret <= (p-buffer)) break;
        }
    }

    for (;drivestr[0] <= 'z'; drivestr[0]++) {
        /* Older W2K fails with ERROR_INSUFFICIENT_BUFFER when buflen is > 32767 */
        ret = QueryDosDeviceA( drivestr, buffer, buflen - 1);
        ok(ret || GetLastError() == ERROR_FILE_NOT_FOUND,
            "QueryDosDeviceA failed to return current mapping for %s, last error %u\n", drivestr, GetLastError());
        if(ret) {
            for (p = buffer; *p; p++) *p = toupper(*p);
            if (strstr(buffer, "HARDDISK") || strstr(buffer, "RAMDISK")) found = TRUE;
        }
    }
    ok(found, "expected at least one devicename to contain HARDDISK or RAMDISK\n");
    HeapFree( GetProcessHeap(), 0, buffer );
}

static void test_define_dos_deviceA(void)
{
    char drivestr[3];
    char buf[MAX_PATH];
    DWORD ret;

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

    /* Map it to point to the current directory */
    ret = GetCurrentDirectoryA(sizeof(buf), buf);
    ok(ret, "GetCurrentDir\n");

    ret = DefineDosDeviceA(0, drivestr, buf);
    todo_wine
    ok(ret, "Could not make drive %s point to %s!\n", drivestr, buf);

    if (!ret) {
        skip("can't test removing fake drive\n");
    } else {
	ret = DefineDosDeviceA(DDD_REMOVE_DEFINITION, drivestr, NULL);
	ok(ret, "Could not remove fake drive %s!\n", drivestr);
    }
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
        "wrong error %u\n", GetLastError() );
    handle = pFindFirstVolumeA( volume, 49 );
    ok( handle == INVALID_HANDLE_VALUE, "succeeded with short buffer\n" );
    ok( GetLastError() == ERROR_FILENAME_EXCED_RANGE, "wrong error %u\n", GetLastError() );
    handle = pFindFirstVolumeA( volume, 51 );
    ok( handle != INVALID_HANDLE_VALUE, "failed err %u\n", GetLastError() );
    if (handle != INVALID_HANDLE_VALUE)
    {
        do
        {
            ok( strlen(volume) == 49, "bad volume name %s\n", volume );
            ok( !memcmp( volume, "\\\\?\\Volume{", 11 ), "bad volume name %s\n", volume );
            ok( !memcmp( volume + 47, "}\\", 2 ), "bad volume name %s\n", volume );
        } while (pFindNextVolumeA( handle, volume, MAX_PATH ));
        ok( GetLastError() == ERROR_NO_MORE_FILES, "wrong error %u\n", GetLastError() );
        pFindVolumeClose( handle );
    }
}

static void test_GetVolumeNameForVolumeMountPointA(void)
{
    BOOL ret;
    char volume[MAX_PATH], path[] = "c:\\";
    DWORD len = sizeof(volume), reti;
    char temp_path[MAX_PATH];

    /* not present before w2k */
    if (!pGetVolumeNameForVolumeMountPointA) {
        win_skip("GetVolumeNameForVolumeMountPointA not found\n");
        return;
    }

    reti = GetTempPathA(MAX_PATH, temp_path);
    ok(reti != 0, "GetTempPathA error %d\n", GetLastError());
    ok(reti < MAX_PATH, "temp path should fit into MAX_PATH\n");

    ret = pGetVolumeNameForVolumeMountPointA(path, volume, 0);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");
    ok(GetLastError() == ERROR_FILENAME_EXCED_RANGE ||
        GetLastError() == ERROR_INVALID_PARAMETER, /* Vista */
        "wrong error, last=%d\n", GetLastError());

    if (0) { /* these crash on XP */
    ret = pGetVolumeNameForVolumeMountPointA(path, NULL, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");

    ret = pGetVolumeNameForVolumeMountPointA(NULL, volume, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");
    }

    ret = pGetVolumeNameForVolumeMountPointA(path, volume, len);
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointA failed\n");
    ok(!strncmp( volume, "\\\\?\\Volume{", 11),
        "GetVolumeNameForVolumeMountPointA failed to return valid string <%s>\n",
        volume);

    /* test with too small buffer */
    ret = pGetVolumeNameForVolumeMountPointA(path, volume, 10);
    ok(ret == FALSE && GetLastError() == ERROR_FILENAME_EXCED_RANGE,
            "GetVolumeNameForVolumeMountPointA failed, wrong error returned, was %d, should be ERROR_FILENAME_EXCED_RANGE\n",
             GetLastError());

    /* Try on an arbitrary directory */
    /* On FAT filesystems it seems that GetLastError() is set to
       ERROR_INVALID_FUNCTION. */
    ret = pGetVolumeNameForVolumeMountPointA(temp_path, volume, len);
    ok(ret == FALSE && (GetLastError() == ERROR_NOT_A_REPARSE_POINT ||
        GetLastError() == ERROR_INVALID_FUNCTION),
        "GetVolumeNameForVolumeMountPointA failed on %s, last=%d\n",
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
        ret = pGetVolumeNameForVolumeMountPointA(path, volume, len);
        ok(ret == FALSE && GetLastError() == ERROR_FILE_NOT_FOUND,
            "GetVolumeNameForVolumeMountPointA failed on %s, last=%d\n",
            path, GetLastError());

        /* Try without trailing \ and on a nonexistent dos drive  */
        path[2] = 0;
        ret = pGetVolumeNameForVolumeMountPointA(path, volume, len);
        ok(ret == FALSE && GetLastError() == ERROR_INVALID_NAME,
            "GetVolumeNameForVolumeMountPointA failed on %s, last=%d\n",
            path, GetLastError());
    }
}

static void test_GetVolumeNameForVolumeMountPointW(void)
{
    BOOL ret;
    WCHAR volume[MAX_PATH], path[] = {'c',':','\\',0};
    DWORD len = sizeof(volume) / sizeof(WCHAR);

    /* not present before w2k */
    if (!pGetVolumeNameForVolumeMountPointW) {
        win_skip("GetVolumeNameForVolumeMountPointW not found\n");
        return;
    }

    ret = pGetVolumeNameForVolumeMountPointW(path, volume, 0);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");
    ok(GetLastError() == ERROR_FILENAME_EXCED_RANGE ||
        GetLastError() == ERROR_INVALID_PARAMETER, /* Vista */
        "wrong error, last=%d\n", GetLastError());

    if (0) { /* these crash on XP */
    ret = pGetVolumeNameForVolumeMountPointW(path, NULL, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointW succeeded\n");

    ret = pGetVolumeNameForVolumeMountPointW(NULL, volume, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointW succeeded\n");
    }

    ret = pGetVolumeNameForVolumeMountPointW(path, volume, len);
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

    ok( pGetVolumeInformationA != NULL, "GetVolumeInformationA not found\n");
    if(!pGetVolumeInformationA) {
        return;
    }

    /* get windows drive letter and update strings for testing */
    result = GetWindowsDirectoryA(windowsdir, sizeof(windowsdir));
    ok(result < sizeof(windowsdir), "windowsdir is abnormally long!\n");
    ok(result != 0, "GetWindowsDirectory: error %d\n", GetLastError());
    Root_Colon[0] = windowsdir[0];
    Root_Slash[0] = windowsdir[0];
    Root_UNC[4] = windowsdir[0];

    result = GetCurrentDirectoryA(MAX_PATH, currentdir);
    ok(result, "GetCurrentDirectory: error %d\n", GetLastError());
    /* Note that GetCurrentDir yields no trailing slash for subdirs */

    /* check for NO error on no trailing \ when current dir is root dir */
    ret = SetCurrentDirectoryA(Root_Slash);
    ok(ret, "SetCurrentDirectory: error %d\n", GetLastError());
    ret = pGetVolumeInformationA(Root_Colon, vol_name_buf, vol_name_size, NULL,
            NULL, NULL, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA root failed, last error %u\n", GetLastError());

    /* check for error on no trailing \ when current dir is subdir (windows) of queried drive */
    ret = SetCurrentDirectoryA(windowsdir);
    ok(ret, "SetCurrentDirectory: error %d\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pGetVolumeInformationA(Root_Colon, vol_name_buf, vol_name_size, NULL,
            NULL, NULL, fs_name_buf, fs_name_len);
    ok(!ret && (GetLastError() == ERROR_INVALID_NAME),
        "GetVolumeInformationA did%s fail, last error %u\n", ret ? " not":"", GetLastError());

    /* reset current directory */
    ret = SetCurrentDirectoryA(currentdir);
    ok(ret, "SetCurrentDirectory: error %d\n", GetLastError());

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
        ok(ret, "SetCurrentDirectory: error %d\n", GetLastError());
        ret = SetCurrentDirectoryA(currentdir);
        ok(ret, "SetCurrentDirectory: error %d\n", GetLastError());

        /* windows dir is current on the root drive, call fails */
        SetLastError(0xdeadbeef);
        ret = pGetVolumeInformationA(Root_Colon, vol_name_buf, vol_name_size, NULL,
                NULL, NULL, fs_name_buf, fs_name_len);
        ok(!ret && (GetLastError() == ERROR_INVALID_NAME),
           "GetVolumeInformationA did%s fail, last error %u\n", ret ? " not":"", GetLastError());

        /* Try normal drive letter with trailing \ */
        ret = pGetVolumeInformationA(Root_Slash, vol_name_buf, vol_name_size, NULL,
                NULL, NULL, fs_name_buf, fs_name_len);
        ok(ret, "GetVolumeInformationA with \\ failed, last error %u\n", GetLastError());

        ret = SetCurrentDirectoryA(Root_Slash);
        ok(ret, "SetCurrentDirectory: error %d\n", GetLastError());
        ret = SetCurrentDirectoryA(currentdir);
        ok(ret, "SetCurrentDirectory: error %d\n", GetLastError());

        /* windows dir is STILL CURRENT on root drive; the call fails as before,   */
        /* proving that SetCurrentDir did not remember the other drive's directory */
        SetLastError(0xdeadbeef);
        ret = pGetVolumeInformationA(Root_Colon, vol_name_buf, vol_name_size, NULL,
                NULL, NULL, fs_name_buf, fs_name_len);
        ok(!ret && (GetLastError() == ERROR_INVALID_NAME),
           "GetVolumeInformationA did%s fail, last error %u\n", ret ? " not":"", GetLastError());

        /* Now C:\ becomes the current directory on drive C: */
        ret = SetEnvironmentVariableA(Root_Env, Root_Slash); /* set =C:=C:\ */
        ok(ret, "SetEnvironmentVariable %s failed\n", Root_Env);

        /* \ is current on root drive, call succeeds */
        ret = pGetVolumeInformationA(Root_Colon, vol_name_buf, vol_name_size, NULL,
                NULL, NULL, fs_name_buf, fs_name_len);
        ok(ret, "GetVolumeInformationA failed, last error %u\n", GetLastError());

        /* again, SetCurrentDirectory on another drive does not matter */
        ret = SetCurrentDirectoryA(Root_Slash);
        ok(ret, "SetCurrentDirectory: error %d\n", GetLastError());
        ret = SetCurrentDirectoryA(currentdir);
        ok(ret, "SetCurrentDirectory: error %d\n", GetLastError());

        /* \ is current on root drive, call succeeds */
        ret = pGetVolumeInformationA(Root_Colon, vol_name_buf, vol_name_size, NULL,
                NULL, NULL, fs_name_buf, fs_name_len);
        ok(ret, "GetVolumeInformationA failed, last error %u\n", GetLastError());
    }

    /* try null root directory to return "root of the current directory"  */
    ret = pGetVolumeInformationA(NULL, vol_name_buf, vol_name_size, NULL,
            NULL, NULL, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA failed on null root dir, last error %u\n", GetLastError());

    /* Try normal drive letter with trailing \  */
    ret = pGetVolumeInformationA(Root_Slash, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA failed, root=%s, last error=%u\n", Root_Slash, GetLastError());

    /* try again with drive letter and the "disable parsing" prefix */
    SetLastError(0xdeadbeef);
    ret = pGetVolumeInformationA(Root_UNC, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA did%s fail, root=%s, last error=%u\n", ret ? " not":"", Root_UNC, GetLastError());

    /* try again with device name space  */
    Root_UNC[2] = '.';
    SetLastError(0xdeadbeef);
    ret = pGetVolumeInformationA(Root_UNC, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA did%s fail, root=%s, last error=%u\n", ret ? " not":"", Root_UNC, GetLastError());

    /* try again with a directory off the root - should generate error  */
    if (windowsdir[strlen(windowsdir)-1] != '\\') strcat(windowsdir, "\\");
    SetLastError(0xdeadbeef);
    ret = pGetVolumeInformationA(windowsdir, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(!ret && (GetLastError()==ERROR_DIR_NOT_ROOT),
          "GetVolumeInformationA did%s fail, root=%s, last error=%u\n", ret ? " not":"", windowsdir, GetLastError());
    /* A subdir with trailing \ yields DIR_NOT_ROOT instead of INVALID_NAME */
    if (windowsdir[strlen(windowsdir)-1] == '\\') windowsdir[strlen(windowsdir)-1] = 0;
    SetLastError(0xdeadbeef);
    ret = pGetVolumeInformationA(windowsdir, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(!ret && (GetLastError()==ERROR_INVALID_NAME),
          "GetVolumeInformationA did%s fail, root=%s, last error=%u\n", ret ? " not":"", windowsdir, GetLastError());

    if (!pGetVolumeNameForVolumeMountPointA) {
        win_skip("GetVolumeNameForVolumeMountPointA not found\n");
        return;
    }
    /* get the unique volume name for the windows drive  */
    ret = pGetVolumeNameForVolumeMountPointA(Root_Slash, volume, MAX_PATH);
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointA failed\n");

    /* try again with unique volume name */
    ret = pGetVolumeInformationA(volume, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA failed, root=%s, last error=%u\n", volume, GetLastError());
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

    if (!pGetVolumeNameForVolumeMountPointA) {
        win_skip("GetVolumeNameForVolumeMountPointA not found\n");
        return;
    }

    /*get windows drive letter and update strings for testing  */
    ret = GetWindowsDirectoryA( windowsdir, sizeof(windowsdir) );
    ok(ret < sizeof(windowsdir), "windowsdir is abnormally long!\n");
    ok(ret != 0, "GetWindowsDirecory: error %d\n", GetLastError());
    path[0] = windowsdir[0];

    /* get the unique volume name for the windows drive  */
    ret = pGetVolumeNameForVolumeMountPointA( path, Volume_1, MAX_PATH );
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointA failed\n");
    ok(strlen(Volume_1) == 49, "GetVolumeNameForVolumeMountPointA returned wrong length name %s\n", Volume_1);

    /* get first unique volume name of list  */
    hFind = pFindFirstVolumeA( Volume_2, MAX_PATH );
    ok(hFind != INVALID_HANDLE_VALUE, "FindFirstVolume failed, err=%u\n",
                GetLastError());
    /* ReactOS */
    if (hFind != INVALID_HANDLE_VALUE) {
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
        win_skip("can't open c: drive %u\n", GetLastError());
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
    ok(ret, "DeviceIoControl failed %u\n", GetLastError());
    ok(size == 32, "expected 32, got %u\n", size);
    CloseHandle( handle );
}

static void test_GetVolumePathNameA(void)
{
    BOOL ret;
    char volume[MAX_PATH];
    char expected[] = "C:\\", pathC1[] = "C:\\", pathC2[] = "C::";
    DWORD error;

    if (!pGetVolumePathNameA)
    {
        win_skip("required functions not found\n");
        return;
    }

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNameA(NULL, NULL, 0);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER
       || broken( error == 0xdeadbeef) /* <=XP */,
       "expected ERROR_INVALID_PARAMETER got %u\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNameA("", NULL, 0);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER
       || broken( error == 0xdeadbeef) /* <=XP */,
       "expected ERROR_INVALID_PARAMETER got %u\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNameA(pathC1, NULL, 0);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER
       || broken(error == ERROR_FILENAME_EXCED_RANGE) /* <=XP */,
       "expected ERROR_INVALID_PARAMETER got %u\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNameA(pathC1, volume, 0);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_PARAMETER
       || broken(error == ERROR_FILENAME_EXCED_RANGE ) /* <=XP */,
       "expected ERROR_INVALID_PARAMETER got %u\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNameA(pathC1, volume, 1);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_FILENAME_EXCED_RANGE, "expected ERROR_FILENAME_EXCED_RANGE got %u\n", error);

    volume[0] = '\0';
    ret = pGetVolumePathNameA(pathC1, volume, sizeof(volume));
    ok(ret, "expected success\n");
    ok(!strcmp(expected, volume), "expected name '%s', returned '%s'\n", pathC1, volume);

    pathC1[0] = tolower(pathC1[0]);
    volume[0] = '\0';
    ret = pGetVolumePathNameA(pathC1, volume, sizeof(volume));
    ok(ret, "expected success\n");
todo_wine
    ok(!strcmp(expected, volume) || broken(!strcasecmp(expected, volume)) /* <=XP */,
       "expected name '%s', returned '%s'\n", expected, volume);

    volume[0] = '\0';
    ret = pGetVolumePathNameA(pathC2, volume, sizeof(volume));
todo_wine
    ok(ret, "expected success\n");
todo_wine
    ok(!strcmp(expected, volume), "expected name '%s', returned '%s'\n", expected, volume);

    /* test an invalid path */
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNameA("\\\\$$$", volume, 1);
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME || broken(ERROR_FILENAME_EXCED_RANGE) /* <=2000 */,
       "expected ERROR_INVALID_NAME got %u\n", error);
}

static void test_GetVolumePathNamesForVolumeNameA(void)
{
    BOOL ret;
    char volume[MAX_PATH], buffer[MAX_PATH];
    DWORD len, error;

    if (!pGetVolumePathNamesForVolumeNameA || !pGetVolumeNameForVolumeMountPointA)
    {
        win_skip("required functions not found\n");
        return;
    }

    ret = pGetVolumeNameForVolumeMountPointA( "c:\\", volume, sizeof(volume) );
    ok(ret, "failed to get volume name %u\n", GetLastError());
    trace("c:\\ -> %s\n", volume);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( NULL, NULL, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %u\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( "", NULL, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %u\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( volume, NULL, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_MORE_DATA, "expected ERROR_MORE_DATA got %u\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( volume, buffer, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_MORE_DATA, "expected ERROR_MORE_DATA got %u\n", error);

    memset( buffer, 0xff, sizeof(buffer) );
    ret = pGetVolumePathNamesForVolumeNameA( volume, buffer, sizeof(buffer), NULL );
    ok(ret, "failed to get path names %u\n", GetLastError());
    ok(!strcmp( "C:\\", buffer ), "expected \"\\C:\" got \"%s\"\n", buffer);
    ok(!buffer[4], "expected double null-terminated buffer\n");

    len = 0;
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( NULL, NULL, 0, &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %u\n", error);

    len = 0;
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( NULL, NULL, sizeof(buffer), &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %u\n", error);

    len = 0;
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( NULL, buffer, sizeof(buffer), &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %u\n", error);

    len = 0;
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameA( NULL, buffer, sizeof(buffer), &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %u\n", error);

    len = 0;
    memset( buffer, 0xff, sizeof(buffer) );
    ret = pGetVolumePathNamesForVolumeNameA( volume, buffer, sizeof(buffer), &len );
    ok(ret, "failed to get path names %u\n", GetLastError());
    ok(len == 5 || broken(len == 2), "expected 5 got %u\n", len);
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

#ifdef __REACTOS__
    /* due to failing all calls to GetVolumeNameForVolumeMountPointW, this
     * buffer never gets initialized and could cause a buffer overflow later */
    volume[0] = 0;
#endif /* __REACTOS__ */

    if (!pGetVolumePathNamesForVolumeNameW || !pGetVolumeNameForVolumeMountPointW)
    {
        win_skip("required functions not found\n");
        return;
    }

    ret = pGetVolumeNameForVolumeMountPointW( drive_c, volume, sizeof(volume)/sizeof(volume[0]) );
    ok(ret, "failed to get volume name %u\n", GetLastError());

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameW( empty, NULL, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %u\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameW( volume, NULL, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_MORE_DATA, "expected ERROR_MORE_DATA got %u\n", error);

    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameW( volume, buffer, 0, NULL );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_MORE_DATA, "expected ERROR_MORE_DATA got %u\n", error);

    if (0) { /* crash */
    ret = pGetVolumePathNamesForVolumeNameW( volume, NULL, sizeof(buffer), NULL );
    ok(ret, "failed to get path names %u\n", GetLastError());
    }

    ret = pGetVolumePathNamesForVolumeNameW( volume, buffer, sizeof(buffer), NULL );
    ok(ret, "failed to get path names %u\n", GetLastError());

    len = 0;
    memset( buffer, 0xff, sizeof(buffer) );
    ret = pGetVolumePathNamesForVolumeNameW( volume, buffer, sizeof(buffer), &len );
    ok(ret, "failed to get path names %u\n", GetLastError());
    ok(len == 5, "expected 5 got %u\n", len);
    ok(!buffer[4], "expected double null-terminated buffer\n");

    len = 0;
    volume[1] = '?';
    volume[lstrlenW( volume ) - 1] = 0;
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameW( volume, buffer, sizeof(buffer), &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_INVALID_NAME, "expected ERROR_INVALID_NAME got %u\n", error);

    len = 0;
    volume[0] = '\\';
    volume[1] = 0;
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameW( volume, buffer, sizeof(buffer), &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    todo_wine ok(error == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER got %u\n", error);

    len = 0;
    lstrcpyW( volume, volume_null );
    SetLastError( 0xdeadbeef );
    ret = pGetVolumePathNamesForVolumeNameW( volume, buffer, sizeof(buffer), &len );
    error = GetLastError();
    ok(!ret, "expected failure\n");
    ok(error == ERROR_FILE_NOT_FOUND, "expected ERROR_FILE_NOT_FOUND got %u\n", error);
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
        skip("IOCTL_DVD_READ_STRUCTURE not supported: %u\n", GetLastError());
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
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "IOCTL_DVD_READ_STRUCTURE should have failed %d %u\n", ret, GetLastError());

    for(i=0; i<sizeof(DVD_COPYRIGHT_DESCRIPTOR); i++)
    {
        SetLastError(0xdeadbeef);

        ret = DeviceIoControl(handle, IOCTL_DVD_READ_STRUCTURE, &dvdReadStructure, sizeof(DVD_READ_STRUCTURE),
            &dvdCopyrightDescriptor, i, &nbBytes, NULL);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "IOCTL_DVD_READ_STRUCTURE should have failed %d %u\n", ret, GetLastError());
    }


    /* DvdManufacturerDescriptor */
    dvdReadStructure.Format = 4;

    SetLastError(0xdeadbeef);

    ret = DeviceIoControl(handle, IOCTL_DVD_READ_STRUCTURE, &dvdReadStructure, sizeof(DVD_READ_STRUCTURE),
        &completeDvdManufacturerDescriptor, sizeof(DVD_MANUFACTURER_DESCRIPTOR), &nbBytes, NULL);
    ok(ret || broken(GetLastError() == ERROR_NOT_READY),
        "IOCTL_DVD_READ_STRUCTURE (DvdManufacturerDescriptor) failed, last error = %u\n", GetLastError());
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
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "IOCTL_DVD_READ_STRUCTURE should have failed %d %u\n", ret, GetLastError());
}

static void test_cdrom_ioctl(void)
{
    char drive_letter, drive_path[] = "A:\\", drive_full_path[] = "\\\\.\\A:";
    DWORD bitmask;
    HANDLE handle;

    bitmask = GetLogicalDrives();
    if(!bitmask)
    {
        trace("GetLogicalDrives failed : %u\n", GetLastError());
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
            trace("Failed to open the device : %u\n", GetLastError());
            continue;
        }

        /* Add your tests here */
        test_dvd_read_structure(handle);

        CloseHandle(handle);
    }

}

START_TEST(volume)
{
    hdll = GetModuleHandleA("kernel32.dll");
    pGetVolumeNameForVolumeMountPointA = (void *) GetProcAddress(hdll, "GetVolumeNameForVolumeMountPointA");
    pGetVolumeNameForVolumeMountPointW = (void *) GetProcAddress(hdll, "GetVolumeNameForVolumeMountPointW");
    pFindFirstVolumeA = (void *) GetProcAddress(hdll, "FindFirstVolumeA");
    pFindNextVolumeA = (void *) GetProcAddress(hdll, "FindNextVolumeA");
    pFindVolumeClose = (void *) GetProcAddress(hdll, "FindVolumeClose");
    pGetLogicalDriveStringsA = (void *) GetProcAddress(hdll, "GetLogicalDriveStringsA");
    pGetLogicalDriveStringsW = (void *) GetProcAddress(hdll, "GetLogicalDriveStringsW");
    pGetVolumeInformationA = (void *) GetProcAddress(hdll, "GetVolumeInformationA");
    pGetVolumePathNameA = (void *) GetProcAddress(hdll, "GetVolumePathNameA");
    pGetVolumePathNamesForVolumeNameA = (void *) GetProcAddress(hdll, "GetVolumePathNamesForVolumeNameA");
    pGetVolumePathNamesForVolumeNameW = (void *) GetProcAddress(hdll, "GetVolumePathNamesForVolumeNameW");

    test_query_dos_deviceA();
    test_define_dos_deviceA();
    test_FindFirstVolume();
    test_GetVolumePathNameA();
    test_GetVolumeNameForVolumeMountPointA();
    test_GetVolumeNameForVolumeMountPointW();
    test_GetLogicalDriveStringsA();
    test_GetLogicalDriveStringsW();
    test_GetVolumeInformationA();
    test_enum_vols();
    test_disk_extents();
    test_GetVolumePathNamesForVolumeNameA();
    test_GetVolumePathNamesForVolumeNameW();
    test_cdrom_ioctl();
}
