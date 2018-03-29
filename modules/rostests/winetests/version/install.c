/*
 * Copyright (C) 2005 Stefan Leichter
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winver.h"
#include "shlobj.h"

static void test_find_file(void)
{
    DWORD ret;
    UINT dwCur, dwOut ;
    char tmp[MAX_PATH];
    char appdir[MAX_PATH];
    char curdir[MAX_PATH];
    char filename[MAX_PATH];
    char outBuf[MAX_PATH];
    char windir[MAX_PATH];
    static const char empty[]       = "",
                      regedit[]     = "regedit",
                      regedit_exe[] = "regedit.exe";

    memset(appdir, 0, MAX_PATH);
    memset(windir, 0, MAX_PATH);

    dwCur=MAX_PATH;
    dwOut=MAX_PATH;
    memset(curdir, 0, MAX_PATH);
    memset(outBuf, 0, MAX_PATH);
    ret = VerFindFileA(0, regedit, empty, empty, curdir, &dwCur, outBuf, &dwOut);
    switch(ret) {
    case 0L:
    ok(dwCur == 1, "Wrong length of buffer for current location: "
       "got %d(%s) expected 1\n", dwCur, curdir);
    ok(dwOut == 1, "Wrong length of buffer for the recommended installation location: "
       "got %d(%s) expected 1\n", dwOut, outBuf);
        break;
    case VFF_BUFFTOOSMALL:
        ok(dwCur == MAX_PATH, "Wrong length of buffer for current location: "
           "got %d(%s) expected MAX_PATH\n", dwCur, curdir);
        ok(dwOut == MAX_PATH, "Wrong length of buffer for the recommended installation location: "
           "got %d(%s) expected MAX_PATH\n", dwOut, outBuf);
        break;
    default:
        ok(0, "Got unexpected return value %x\n", ret);
    }

    if(!GetWindowsDirectoryA(windir, MAX_PATH))
        trace("GetWindowsDirectoryA failed\n");
    else {
        sprintf(appdir, "%s\\regedit.exe", windir);
        if(INVALID_FILE_ATTRIBUTES == GetFileAttributesA(appdir))
            trace("GetFileAttributesA(%s) failed\n", appdir);
        else {
            dwCur=MAX_PATH;
            dwOut=MAX_PATH;
            memset(curdir, 0, MAX_PATH);
            memset(outBuf, 0, MAX_PATH);
            ret = VerFindFileA(0, regedit_exe, empty, empty, curdir, &dwCur, outBuf, &dwOut);
            switch(ret) {
            case VFF_CURNEDEST:
                ok(dwCur == 1 + strlen(windir), "Wrong length of buffer for current location: "
               "got %d(%s) expected %d\n", dwCur, curdir, lstrlenA(windir)+1);
            ok(dwOut == 1, "Wrong length of buffer for the recommended installation location: "
               "got %d(%s) expected 1\n", dwOut, outBuf);
                break;
            case VFF_BUFFTOOSMALL:
                ok(dwCur == MAX_PATH, "Wrong length of buffer for current location: "
                   "got %d(%s) expected MAX_PATH\n", dwCur, curdir);
                ok(dwOut == MAX_PATH, "Wrong length of buffer for the recommended installation location: "
                   "got %d(%s) expected MAX_PATH\n", dwOut, outBuf);
                break;
            default:
                ok(0, "Got unexpected return value %x\n", ret);
            }

            dwCur=MAX_PATH;
            dwOut=MAX_PATH;
            memset(curdir, 0, MAX_PATH);
            memset(outBuf, 0, MAX_PATH);
            ret = VerFindFileA(0, regedit_exe, NULL, NULL, curdir, &dwCur, outBuf, &dwOut);
            switch(ret) {
            case VFF_CURNEDEST:
                ok(dwCur == 1 + strlen(windir), "Wrong length of buffer for current location: "
               "got %d(%s) expected %d\n", dwCur, curdir, lstrlenA(windir)+1);
            ok(dwOut == 1, "Wrong length of buffer for the recommended installation location: "
               "got %d(%s) expected 1\n", dwOut, outBuf);
                break;
            case VFF_BUFFTOOSMALL:
                ok(dwCur == MAX_PATH, "Wrong length of buffer for current location: "
                   "got %d(%s) expected MAX_PATH\n", dwCur, curdir);
                ok(dwOut == MAX_PATH, "Wrong length of buffer for the recommended installation location: "
                   "got %d(%s) expected MAX_PATH\n", dwOut, outBuf);
                break;
            default:
                ok(0, "Got unexpected return value %x\n", ret);
            }
        }
    }
    if(!GetSystemDirectoryA(windir, MAX_PATH) ||
       !SHGetSpecialFolderPathA(0, appdir, CSIDL_PROGRAM_FILES, FALSE) ||
       !GetTempPathA(MAX_PATH, tmp) ||
       !GetTempFileNameA(tmp, "tes", 0, filename))
        ok(0, "GetSystemDirectoryA, SHGetSpecialFolderPathA, GetTempPathA or GetTempFileNameA failed\n");
    else {
        char *p = strrchr(filename, '\\');
        if(p) {
            *(p++) ='\0';
            SetCurrentDirectoryA(filename);
            memmove(filename, p, 1 + strlen(p));
        }

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        memset(outBuf, 0, MAX_PATH);
        memset(curdir, 0, MAX_PATH);
        ret = VerFindFileA(0, filename, NULL, NULL, curdir, &dwCur, outBuf, &dwOut);
        switch(ret) {
        case VFF_CURNEDEST:
        ok(dwOut == 1, "Wrong length of buffer for the recommended installation location"
           "got %d(%s) expected 1\n", dwOut, outBuf);
            break;
        case VFF_BUFFTOOSMALL:
            ok(dwOut == MAX_PATH, "Wrong length of buffer for the recommended installation location"
               "got %d(%s) expected MAX_PATH\n", dwOut, outBuf);
            break;
        default:
            todo_wine ok(0, "Got unexpected return value %x\n", ret);
        }

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        memset(outBuf, 0, MAX_PATH);
        memset(curdir, 0, MAX_PATH);
        ret = VerFindFileA(VFFF_ISSHAREDFILE, filename, NULL, appdir, curdir, &dwCur, outBuf, &dwOut);
        ok(VFF_CURNEDEST == ret, "Wrong return value got %x expected VFF_CURNEDEST\n", ret);
        ok(dwOut == 1 + strlen(windir), "Wrong length of buffer for current location: "
           "got %d(%s) expected %d\n", dwOut, outBuf, lstrlenA(windir)+1);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        memset(outBuf, 0, MAX_PATH);
        memset(curdir, 0, MAX_PATH);
        ret = VerFindFileA(0, filename, NULL, appdir, curdir, &dwCur, outBuf, &dwOut);
        ok(VFF_CURNEDEST == ret, "Wrong return value got %x expected VFF_CURNEDEST\n", ret);
        ok(dwOut == 1 + strlen(appdir), "Wrong length of buffer for current location: "
           "got %d(%s) expected %d\n", dwOut, outBuf, lstrlenA(appdir)+1);

        /* search for filename */
        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, filename, NULL, NULL, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, filename, NULL, empty, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, filename, NULL, appdir, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, filename, NULL, NULL, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, filename, NULL, empty, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, filename, NULL, appdir, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        /* search for regedit */
        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, "regedit", NULL, NULL, curdir, &dwCur, outBuf, &dwOut);
        ok(!ret, "Wrong return value got %x expected 0\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, "regedit", NULL, empty, curdir, &dwCur, outBuf, &dwOut);
        ok(!ret, "Wrong return value got %x expected 0\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, "regedit", NULL, appdir, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, "regedit", NULL, NULL, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, "regedit", NULL, empty, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, "regedit", NULL, appdir, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        /* search for regedit.exe */
        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, "regedit.exe", NULL, NULL, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, "regedit.exe", NULL, empty, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, "regedit.exe", NULL, appdir, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, "regedit.exe", NULL, NULL, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, "regedit.exe", NULL, empty, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, "regedit.exe", NULL, appdir, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        /* nonexistent filename */
        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, "doesnotexist.exe", NULL, NULL, curdir, &dwCur, outBuf, &dwOut);
        ok(!ret, "Wrong return value got %x expected 0\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, "doesnotexist.exe", NULL, empty, curdir, &dwCur, outBuf, &dwOut);
        ok(!ret, "Wrong return value got %x expected 0\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, "doesnotexist.exe", NULL, appdir, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(0, "doesnotexist.exe", NULL, "C:\\random_path_does_not_exist", curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, "doesnotexist.exe", NULL, NULL, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, "doesnotexist.exe", NULL, empty, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, "doesnotexist.exe", NULL, appdir, curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        ret = VerFindFileA(VFFF_ISSHAREDFILE, "doesnotexist.exe", NULL, "C:\\random_path_does_not_exist", curdir, &dwCur, outBuf, &dwOut);
        ok(ret & VFF_CURNEDEST, "Wrong return value got %x expected VFF_CURNEDEST set\n", ret);

        DeleteFileA(filename);
    }
}

static void test_install_file(void)
{
    CHAR tmpname[MAX_PATH];
    UINT size = MAX_PATH;
    DWORD rc;
    static const CHAR szSrcFileName[] = "nofile.txt";
    static const CHAR szDestFileName[] = "nofile2.txt";
    static const CHAR szSrcDir[] = "D:\\oes\\not\\exist";
    static const CHAR szDestDir[] = "D:\\oes\\not\\exist\\either";
    static const CHAR szCurDir[] = "C:\\";

    /* testing Invalid Parameters */
    memset(tmpname,0,sizeof(tmpname));
    rc = VerInstallFileA(0x0, NULL, NULL, NULL, NULL, NULL, tmpname, &size);
    ok (rc == 0x10000 && tmpname[0]==0," expected return 0x10000 and no tempname, got %08x/\'%s\'\n",rc,tmpname);
    memset(tmpname,0,sizeof(tmpname));
    size = MAX_PATH;
    rc = VerInstallFileA(0x0, szSrcFileName, NULL, NULL, NULL, NULL, tmpname, &size);
    ok (rc == 0x10000 && tmpname[0]==0," expected return 0x10000 and no tempname, got %08x/\'%s\'\n",rc,tmpname);
    memset(tmpname,0,sizeof(tmpname));
    size = MAX_PATH;
    rc = VerInstallFileA(0x0, szSrcFileName, szDestFileName, NULL, NULL, NULL, tmpname, &size);
    ok (rc == 0x10000 && tmpname[0]==0," expected return 0x10000 and no tempname, got %08x/\'%s\'\n",rc,tmpname);
    memset(tmpname,0,sizeof(tmpname));
    size = MAX_PATH;
    rc = VerInstallFileA(0x0, szSrcFileName, szDestFileName, szSrcDir, NULL, NULL, tmpname, &size);
    ok (rc == 0x10000 && tmpname[0]==0," expected return 0x10000 and no tempname, got %08x/\'%s\'\n",rc,tmpname);

    /* Source file does not exist*/

    memset(tmpname,0,sizeof(tmpname));
    size = MAX_PATH;
    rc = VerInstallFileA(0x0, szSrcFileName, szDestFileName, szSrcDir, szDestDir, NULL, tmpname, &size);
    ok (rc == 0x10000 && tmpname[0]==0," expected return 0x10000 and no tempname, got %08x/\'%s\'\n",rc,tmpname);
    memset(tmpname,0,sizeof(tmpname));
    size = MAX_PATH;
    rc = VerInstallFileA(0x0, szSrcFileName, szDestFileName,  szSrcDir, szDestDir, szCurDir, tmpname, &size);
    ok (rc == 0x10000 && tmpname[0]==0," expected return 0x10000 and no tempname, got %08x/\'%s\'\n",rc,tmpname);
}

START_TEST(install)
{
    test_find_file();
    test_install_file();
}
