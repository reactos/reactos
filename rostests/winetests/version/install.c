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

static void test_find_file(void)
{
    DWORD ret;
    UINT dwCur, dwOut ;
    char appdir[MAX_PATH];
    char curdir[MAX_PATH];
    char filename[MAX_PATH];
    char outBuf[MAX_PATH];
    char windir[MAX_PATH];
    static CHAR empty[]    = "",
               regedit[] = "regedit",
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
                todo_wine ok(0, "Got unexpected return value %x\n", ret);
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
                todo_wine ok(0, "Got unexpected return value %x\n", ret);
            }
        }
    }
    if(!GetModuleFileNameA(NULL, filename, MAX_PATH) ||
       !GetSystemDirectoryA(windir, MAX_PATH) ||
       !GetTempPathA(MAX_PATH, appdir))
        trace("GetModuleFileNameA, GetSystemDirectoryA or GetTempPathA failed\n");
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
        todo_wine ok(VFF_CURNEDEST == ret, "Wrong return value got %x expected VFF_CURNEDEST\n", ret);
        ok(dwOut == 1 + strlen(windir), "Wrong length of buffer for current location: "
           "got %d(%s) expected %d\n", dwOut, outBuf, lstrlenA(windir)+1);

        dwCur=MAX_PATH;
        dwOut=MAX_PATH;
        memset(outBuf, 0, MAX_PATH);
        memset(curdir, 0, MAX_PATH);
        ret = VerFindFileA(0, filename, NULL, appdir, curdir, &dwCur, outBuf, &dwOut);
        todo_wine ok(VFF_CURNEDEST == ret, "Wrong return value got %x expected VFF_CURNEDEST\n", ret);
        ok(dwOut == 1 + strlen(appdir), "Wrong length of buffer for current location: "
           "got %d(%s) expected %d\n", dwOut, outBuf, lstrlenA(appdir)+1);
    }
}

START_TEST(install)
{
    test_find_file();
}
