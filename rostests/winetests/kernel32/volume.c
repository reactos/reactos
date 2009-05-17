/*
 * Unit test suite
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
#include <stdio.h>

static HINSTANCE hdll;
static BOOL (WINAPI * pGetVolumeNameForVolumeMountPointA)(LPCSTR, LPSTR, DWORD);
static BOOL (WINAPI * pGetVolumeNameForVolumeMountPointW)(LPCWSTR, LPWSTR, DWORD);
static HANDLE (WINAPI *pFindFirstVolumeA)(LPSTR,DWORD);
static BOOL (WINAPI *pFindNextVolumeA)(HANDLE,LPSTR,DWORD);
static BOOL (WINAPI *pFindVolumeClose)(HANDLE);
static UINT (WINAPI *pGetLogicalDriveStringsA)(UINT,LPSTR);
static UINT (WINAPI *pGetLogicalDriveStringsW)(UINT,LPWSTR);
static BOOL (WINAPI *pGetVolumeInformationA)(LPCSTR, LPSTR, DWORD, LPDWORD, LPDWORD, LPDWORD, LPSTR, DWORD);

/* ############################### */

static void test_query_dos_deviceA(void)
{
    char drivestr[] = "a:";
    char *p, *buffer, buffer2[2000];
    DWORD ret, ret2, buflen=32768;
    BOOL found = FALSE;

    if (!pFindFirstVolumeA) {
        win_skip("On win9x, HARDDISK and RAMDISK not present\n");
        return;
    }

    buffer = HeapAlloc( GetProcessHeap(), 0, buflen );
    ret = QueryDosDeviceA( NULL, buffer, buflen );
    ok(ret && GetLastError() != ERROR_INSUFFICIENT_BUFFER,
        "QueryDosDevice buffer too small\n");
    if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
        HeapFree( GetProcessHeap(), 0, buffer );
        return;
    }
    ok(ret, "QueryDosDeviceA failed to return list, last error %u\n", GetLastError());
    if (ret) {
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
        ret = QueryDosDeviceA( drivestr, buffer, buflen);
        if(ret) {
            for (p = buffer; *p; p++) *p = toupper(*p);
            if (strstr(buffer, "HARDDISK") || strstr(buffer, "RAMDISK")) found = TRUE;
        }
    }
    ok(found, "expected at least one devicename to contain HARDDISK or RAMDISK\n");
    HeapFree( GetProcessHeap(), 0, buffer );
}

static void test_FindFirstVolume(void)
{
    char volume[51];
    HANDLE handle;

    if (!pFindFirstVolumeA) {
        skip("FindFirstVolumeA not found\n");
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
    DWORD len = sizeof(volume);

    /* not present before w2k */
    if (!pGetVolumeNameForVolumeMountPointA) {
        skip("GetVolumeNameForVolumeMountPointA not found\n");
        return;
    }

    ret = pGetVolumeNameForVolumeMountPointA(path, volume, 0);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");

    if (0) { /* these crash on XP */
    ret = pGetVolumeNameForVolumeMountPointA(path, NULL, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");

    ret = pGetVolumeNameForVolumeMountPointA(NULL, volume, len);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");
    }

    ret = pGetVolumeNameForVolumeMountPointA(path, volume, len);
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointA failed\n");
}

static void test_GetVolumeNameForVolumeMountPointW(void)
{
    BOOL ret;
    WCHAR volume[MAX_PATH], path[] = {'c',':','\\',0};
    DWORD len = sizeof(volume) / sizeof(WCHAR);

    /* not present before w2k */
    if (!pGetVolumeNameForVolumeMountPointW) {
        skip("GetVolumeNameForVolumeMountPointW not found\n");
        return;
    }

    ret = pGetVolumeNameForVolumeMountPointW(path, volume, 0);
    ok(ret == FALSE, "GetVolumeNameForVolumeMountPointA succeeded\n");

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

    if(!pGetLogicalDriveStringsA) {
        win_skip("GetLogicalDriveStringsA not available\n");
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
        ok(('A' <= *ptr && *ptr <= 'Z') ||
           (broken('a' <= *ptr && *ptr <= 'z')), /* Win9x and WinMe */
           "device name '%c' is not uppercase\n", *ptr);
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

    if(!pGetLogicalDriveStringsW) {
        win_skip("GetLogicalDriveStringsW not available\n");
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
    char Root_Dir0[]="C:";
    char Root_Dir1[]="C:\\";
    char Root_Dir2[]="\\\\?\\C:\\";
    char volume[MAX_PATH+1];
    DWORD vol_name_size=MAX_PATH+1, vol_serial_num=-1, max_comp_len=0, fs_flags=0, fs_name_len=MAX_PATH+1;
    char vol_name_buf[MAX_PATH+1], fs_name_buf[MAX_PATH+1];
    char windowsdir[MAX_PATH+10];

    if (!pGetVolumeInformationA) {
        win_skip("GetVolumeInformationA not found\n");
        return;
    }
    if (!pGetVolumeNameForVolumeMountPointA) {
        win_skip("GetVolumeNameForVolumeMountPointA not found\n");
        return;
    }

    /* get windows drive letter and update strings for testing */
    result = GetWindowsDirectory(windowsdir, sizeof(windowsdir));
    ok(result < sizeof(windowsdir), "windowsdir is abnormally long!\n");
    ok(result != 0, "GetWindowsDirectory: error %d\n", GetLastError());
    Root_Dir0[0] = windowsdir[0];
    Root_Dir1[0] = windowsdir[0];
    Root_Dir2[4] = windowsdir[0];

    /* get the unique volume name for the windows drive  */
    ret = pGetVolumeNameForVolumeMountPointA(Root_Dir1, volume, MAX_PATH);
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointA failed\n");

    /*  ****  now start the tests       ****  */
    /* check for error on no trailing \   */
    ret = pGetVolumeInformationA(Root_Dir0, vol_name_buf, vol_name_size, NULL,
            NULL, NULL, fs_name_buf, fs_name_len);
    ok(!ret && GetLastError() == ERROR_INVALID_NAME,
        "GetVolumeInformationA w/o '\\' did not fail, last error %u\n", GetLastError());

    /* try null root directory to return "root of the current directory"  */
    ret = pGetVolumeInformationA(NULL, vol_name_buf, vol_name_size, NULL,
            NULL, NULL, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA failed on null root dir, last error %u\n", GetLastError());

    /* Try normal drive letter with trailing \  */
    ret = pGetVolumeInformationA(Root_Dir1, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    ok(ret, "GetVolumeInformationA failed, root=%s, last error=%u\n", Root_Dir1, GetLastError());

    /* try again with dirve letter and the "disable parsing" prefix */
    ret = pGetVolumeInformationA(Root_Dir2, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    todo_wine ok(ret, "GetVolumeInformationA failed, root=%s, last error=%u\n", Root_Dir2, GetLastError());

    /* try again with unique voluem name */
    ret = pGetVolumeInformationA(volume, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    todo_wine ok(ret, "GetVolumeInformationA failed, root=%s, last error=%u\n", volume, GetLastError());

    /* try again with device name space  */
    Root_Dir2[2] = '.';
    ret = pGetVolumeInformationA(Root_Dir2, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    todo_wine ok(ret, "GetVolumeInformationA failed, root=%s, last error=%u\n", Root_Dir2, GetLastError());

    /* try again with a directory off the root - should generate error  */
    if (windowsdir[strlen(windowsdir)-1] != '\\') strcat(windowsdir, "\\");
    ret = pGetVolumeInformationA(windowsdir, vol_name_buf, vol_name_size,
            &vol_serial_num, &max_comp_len, &fs_flags, fs_name_buf, fs_name_len);
    todo_wine ok(!ret && GetLastError()==ERROR_DIR_NOT_ROOT,
          "GetVolumeInformationA failed, root=%s, last error=%u\n", windowsdir, GetLastError());
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
    ret = GetWindowsDirectory( windowsdir, sizeof(windowsdir) );
    ok(ret < sizeof(windowsdir), "windowsdir is abnormally long!\n");
    ok(ret != 0, "GetWindowsDirecory: error %d\n", GetLastError());
    path[0] = windowsdir[0];

    /* get the unique volume name for the windows drive  */
    ret = pGetVolumeNameForVolumeMountPointA( path, Volume_1, MAX_PATH );
    ok(ret == TRUE, "GetVolumeNameForVolumeMountPointA failed\n");
todo_wine
    ok(strlen(Volume_1) == 49, "GetVolumeNameForVolumeMountPointA returned wrong length name %s\n", Volume_1);

    /* get first unique volume name of list  */
    hFind = pFindFirstVolumeA( Volume_2, MAX_PATH );
    ok(hFind != INVALID_HANDLE_VALUE, "FindFirstVolume failed, err=%u\n",
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
todo_wine
    ok(found, "volume name %s not found by Find[First/Next]Volume\n", Volume_1);
    pFindVolumeClose( hFind );
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

    test_query_dos_deviceA();
    test_FindFirstVolume();
    test_GetVolumeNameForVolumeMountPointA();
    test_GetVolumeNameForVolumeMountPointW();
    test_GetLogicalDriveStringsA();
    test_GetLogicalDriveStringsW();
    test_GetVolumeInformationA();
    test_enum_vols();
}
