/*
 * Unit test suite for dir functions
 *
 * Copyright 2006 CodeWeavers, Aric Stewart
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
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <io.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <process.h>
#include <errno.h>

typedef struct
{
    const char* buffer;
    const char* drive;
    const char* dir;
    const char* file;
    const char* ext;
    const char* expected;
} makepath_case;

#define USE_BUFF ((char*)~0ul)
static const makepath_case makepath_cases[] =
{
    { NULL, NULL, NULL, NULL, NULL, "" }, /* 0 */
    { NULL, "c", NULL, NULL, NULL, "c:" },
    { NULL, "c:", NULL, NULL, NULL, "c:" },
    { NULL, "c:\\", NULL, NULL, NULL, "c:" },
    { NULL, NULL, "dir", NULL, NULL, "dir\\" },
    { NULL, NULL, "dir\\", NULL, NULL, "dir\\" },
    { NULL, NULL, "\\dir", NULL, NULL, "\\dir\\" },
    { NULL, NULL, NULL, "file", NULL, "file" },
    { NULL, NULL, NULL, "\\file", NULL, "\\file" },
    { NULL, NULL, NULL, "file", NULL, "file" },
    { NULL, NULL, NULL, NULL, "ext", ".ext" }, /* 10 */
    { NULL, NULL, NULL, NULL, ".ext", ".ext" },
    { "foo", NULL, NULL, NULL, NULL, "" },
    { "foo", USE_BUFF, NULL, NULL, NULL, "f:" },
    { "foo", NULL, USE_BUFF, NULL, NULL, "foo\\" },
    { "foo", NULL, NULL, USE_BUFF, NULL, "foo" },
    { "foo", NULL, USE_BUFF, "file", NULL, "foo\\file" },
    { "foo", NULL, USE_BUFF, "file", "ext", "foo\\file.ext" },
    { "foo", NULL, NULL, USE_BUFF, "ext", "foo.ext" },
    /* remaining combinations of USE_BUFF crash native */
    { NULL, "c", "dir", "file", "ext", "c:dir\\file.ext" },
    { NULL, "c:", "dir", "file", "ext", "c:dir\\file.ext" }, /* 20 */
    { NULL, "c:\\", "dir", "file", "ext", "c:dir\\file.ext" }
};

static void test_makepath(void)
{
    WCHAR driveW[MAX_PATH];
    WCHAR dirW[MAX_PATH];
    WCHAR fileW[MAX_PATH];
    WCHAR extW[MAX_PATH];
    WCHAR bufferW[MAX_PATH];
    char buffer[MAX_PATH];

    unsigned int i, n;

    for (i = 0; i < sizeof(makepath_cases)/sizeof(makepath_cases[0]); ++i)
    {
        const makepath_case* p = &makepath_cases[i];

        memset(buffer, 'X', MAX_PATH);
        if (p->buffer)
            strcpy(buffer, p->buffer);

        /* Ascii */
        _makepath(buffer,
                  p->drive == USE_BUFF ? buffer : p->drive,
                  p->dir == USE_BUFF ? buffer : p->dir,
                  p->file == USE_BUFF? buffer : p->file,
                  p->ext == USE_BUFF ? buffer : p->ext);

        buffer[MAX_PATH - 1] = '\0';
        ok(!strcmp(p->expected, buffer), "got '%s' for case %d\n", buffer, i);

        /* Unicode */
        if (p->drive != USE_BUFF) MultiByteToWideChar(CP_ACP, 0, p->drive, -1, driveW, MAX_PATH);
        if (p->dir != USE_BUFF) MultiByteToWideChar(CP_ACP, 0, p->dir, -1, dirW, MAX_PATH);
        if (p->file != USE_BUFF) MultiByteToWideChar(CP_ACP, 0, p->file, -1, fileW, MAX_PATH);
        if (p->ext != USE_BUFF) MultiByteToWideChar(CP_ACP, 0, p->ext, -1, extW, MAX_PATH);

        memset(buffer, 0, MAX_PATH);
        for (n = 0; n < MAX_PATH; ++n)
            bufferW[n] = 'X';
        if (p->buffer) MultiByteToWideChar( CP_ACP, 0, p->buffer, -1, bufferW, MAX_PATH);

        _wmakepath(bufferW,
                   p->drive == USE_BUFF ? bufferW : p->drive ? driveW : NULL,
                   p->dir == USE_BUFF ? bufferW : p->dir ? dirW : NULL,
                   p->file == USE_BUFF? bufferW : p->file ? fileW : NULL,
                   p->ext == USE_BUFF ? bufferW : p->ext ? extW : NULL);

        bufferW[MAX_PATH - 1] = '\0';
        WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, MAX_PATH, NULL, NULL);
        ok(!strcmp(p->expected, buffer), "got '%s' for unicode case %d\n", buffer, i);
    }
}

static void test_fullpath(void)
{
    char full[MAX_PATH];
    char tmppath[MAX_PATH];
    char prevpath[MAX_PATH];
    char level1[MAX_PATH];
    char level2[MAX_PATH];
    char teststring[MAX_PATH];
    char *freeme;
    BOOL rc,free1,free2;

    free1=free2=TRUE;
    GetCurrentDirectory(MAX_PATH, prevpath);
    GetTempPath(MAX_PATH,tmppath);
    strcpy(level1,tmppath);
    strcat(level1,"msvcrt-test\\");

    rc = CreateDirectory(level1,NULL);
    if (!rc && GetLastError()==ERROR_ALREADY_EXISTS)
        free1=FALSE;

    strcpy(level2,level1);
    strcat(level2,"nextlevel\\");
    rc = CreateDirectory(level2,NULL);
    if (!rc && GetLastError()==ERROR_ALREADY_EXISTS)
        free2=FALSE;
    SetCurrentDirectory(level2);

    ok(_fullpath(full,"test", MAX_PATH)!=NULL,"_fullpath failed\n");
    strcpy(teststring,level2);
    strcat(teststring,"test");
    ok(strcmp(full,teststring)==0,"Invalid Path returned %s\n",full);
    ok(_fullpath(full,"\\test", MAX_PATH)!=NULL,"_fullpath failed\n");
    strncpy(teststring,level2,3);
    teststring[3]=0;
    strcat(teststring,"test");
    ok(strcmp(full,teststring)==0,"Invalid Path returned %s\n",full);
    ok(_fullpath(full,"..\\test", MAX_PATH)!=NULL,"_fullpath failed\n");
    strcpy(teststring,level1);
    strcat(teststring,"test");
    ok(strcmp(full,teststring)==0,"Invalid Path returned %s\n",full);
    ok(_fullpath(full,"..\\test", 10)==NULL,"_fullpath failed to generate error\n");

    freeme = _fullpath(NULL,"test", 0);
    ok(freeme!=NULL,"No path returned\n");
    strcpy(teststring,level2);
    strcat(teststring,"test");
    ok(strcmp(freeme,teststring)==0,"Invalid Path returned %s\n",freeme);
    free(freeme);

    SetCurrentDirectory(prevpath);
    if (free2)
        RemoveDirectory(level2);
    if (free1)
        RemoveDirectory(level1);
}

START_TEST(dir)
{
    test_fullpath();
    test_makepath();
}
