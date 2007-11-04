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

static void test_makepath(void)
{
    char buffer[MAX_PATH];

    _makepath(buffer, "C", "\\foo", "dummy", "txt");
    ok( strcmp(buffer, "C:\\foo\\dummy.txt") == 0, "unexpected result: %s\n", buffer);
    _makepath(buffer, "C:", "\\foo\\", "dummy", ".txt");
    ok( strcmp(buffer, "C:\\foo\\dummy.txt") == 0, "unexpected result: %s\n", buffer);

    /* this works with native and e.g. Freelancer depends on it */
    strcpy(buffer, "foo");
    _makepath(buffer, NULL, buffer, "dummy.txt", NULL);
    ok( strcmp(buffer, "foo\\dummy.txt") == 0, "unexpected result: %s\n", buffer);
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
