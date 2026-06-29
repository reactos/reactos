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
#include <direct.h>
#include <sys/stat.h>
#include <io.h>
#include <mbctype.h>
#include <windef.h>
#include <winbase.h>
#include <winnls.h>
#include <process.h>
#include <errno.h>

static int (__cdecl *p_makepath_s)(char *, size_t, const char *, const char *, const char *, const char *);
static int (__cdecl *p_wmakepath_s)(wchar_t *, size_t, const wchar_t *,const wchar_t *, const wchar_t *, const wchar_t *);
static int (__cdecl *p_searchenv_s)(const char*, const char*, char*, size_t);
static int (__cdecl *p_wsearchenv_s)(const wchar_t*, const wchar_t*, wchar_t*, size_t);

static void init(void)
{
    HMODULE hmod = GetModuleHandleA("msvcrt.dll");

    p_makepath_s = (void *)GetProcAddress(hmod, "_makepath_s");
    p_wmakepath_s = (void *)GetProcAddress(hmod, "_wmakepath_s");
    p_searchenv_s = (void *)GetProcAddress(hmod, "_searchenv_s");
    p_wsearchenv_s = (void *)GetProcAddress(hmod, "_wsearchenv_s");
}

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

    for (i = 0; i < ARRAY_SIZE(makepath_cases); ++i)
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

typedef struct
{
    const char* buffer;
    size_t length;
    const char* drive;
    const char* dir;
    const char* file;
    const char* ext;
    const char* expected;
    const WCHAR *expected_unicode;
    size_t expected_length;
} makepath_s_case;

static const makepath_s_case makepath_s_cases[] =
{
    /* Behavior with directory parameter containing backslash. */
    {NULL, 1, "c:", "d\\", "file", "ext", "\0XXXXXXXXXXXX", L"\0XXXXXXXXXXXX", 13},
    {NULL, 2, "c:", "d\\", "file", "ext", "\0XXXXXXXXXXXX", L"\0XXXXXXXXXXXX", 13},
    {NULL, 3, "c:", "d\\", "file", "ext", "\0:XXXXXXXXXXX", L"\0:XXXXXXXXXXX", 13},
    {NULL, 4, "c:", "d\\", "file", "ext", "\0:dXXXXXXXXXX", L"\0:dXXXXXXXXXX", 13},
    {NULL, 5, "c:", "d\\", "file", "ext", "\0:d\\XXXXXXXXX", L"\0:d\\XXXXXXXXX", 13},
    {NULL, 6, "c:", "d\\", "file", "ext", "\0:d\\fXXXXXXXX", L"\0:d\\fXXXXXXXX", 13},
    {NULL, 7, "c:", "d\\", "file", "ext", "\0:d\\fiXXXXXXX", L"\0:d\\fiXXXXXXX", 13},
    {NULL, 8, "c:", "d\\", "file", "ext", "\0:d\\filXXXXXX", L"\0:d\\filXXXXXX", 13},
    {NULL, 9, "c:", "d\\", "file", "ext", "\0:d\\fileXXXXX", L"\0:d\\fileXXXXX", 13},
    {NULL, 10, "c:", "d\\", "file", "ext", "\0:d\\file.XXXX", L"\0:d\\file.XXXX", 13},
    {NULL, 11, "c:", "d\\", "file", "ext", "\0:d\\file.eXXX", L"\0:d\\file.eXXX", 13},
    {NULL, 12, "c:", "d\\", "file", "ext", "\0:d\\file.exXX", L"\0:d\\file.exXX", 13},
    /* Behavior with directory parameter lacking backslash. */
    {NULL, 3, "c:", "dir", "f", "ext", "\0:XXXXXXXX", L"\0:XXXXXXXX", 10},
    {NULL, 4, "c:", "dir", "f", "ext", "\0:dXXXXXXX", L"\0:dXXXXXXX", 10},
    {NULL, 5, "c:", "dir", "f", "ext", "\0:diXXXXXX", L"\0:diXXXXXX", 10},
    {NULL, 6, "c:", "dir", "f", "ext", "\0:dirXXXXX", L"\0:dirXXXXX", 10},
    {NULL, 7, "c:", "dir", "f", "ext", "\0:dir\\XXXX", L"\0:dir\\XXXX", 10},
    /* Behavior with overlapped buffer. */
    {"foo", 2, USE_BUFF, NULL, NULL, NULL, "\0oo", L"\0oo", 3},
    {"foo", 4, NULL, USE_BUFF, NULL, NULL, "\0oo\0X", L"\0oo\0X", 5},
    {"foo", 3, NULL, NULL, USE_BUFF, NULL, "\0oo\0", L"\0oo", 4},
    {"foo", 4, NULL, USE_BUFF, "file", NULL, "\0oo\0XXXXX", L"\0oo\0XXXXX", 9},
    {"foo", 8, NULL, USE_BUFF, "file", NULL, "\0oo\\filXX", L"\0oo\\filXX", 9},
    {"foo", 4, NULL, USE_BUFF, "file", "ext", "\0oo\0XXXXXXXXX", L"\0oo\0XXXXXXXXX", 13},
    {"foo", 8, NULL, USE_BUFF, "file", "ext", "\0oo\\filXXXXXX", L"\0oo\\filXXXXXX", 13},
    {"foo", 12, NULL, USE_BUFF, "file", "ext", "\0oo\\file.exXX", L"\0oo\\file.exXX", 13},
    {"foo", 4, NULL, NULL, USE_BUFF, "ext", "\0oo\0XXXX", L"\0oo\0XXXX", 8},
    {"foo", 7, NULL, NULL, USE_BUFF, "ext", "\0oo.exXX", L"\0oo.exXX", 8},
};

static void test_makepath_s(void)
{
    WCHAR driveW[MAX_PATH];
    WCHAR dirW[MAX_PATH];
    WCHAR fileW[MAX_PATH];
    WCHAR extW[MAX_PATH];
    WCHAR bufferW[MAX_PATH];
    char buffer[MAX_PATH];
    int ret;
    unsigned int i, n;

    if (!p_makepath_s || !p_wmakepath_s)
    {
        win_skip("Safe makepath functions are not available\n");
        return;
    }

    errno = EBADF;
    ret = p_makepath_s(NULL, 0, NULL, NULL, NULL, NULL);
    ok(ret == EINVAL, "Expected _makepath_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    errno = EBADF;
    ret = p_makepath_s(buffer, 0, NULL, NULL, NULL, NULL);
    ok(ret == EINVAL, "Expected _makepath_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    errno = EBADF;
    ret = p_wmakepath_s(NULL, 0, NULL, NULL, NULL, NULL);
    ok(ret == EINVAL, "Expected _wmakepath_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    errno = EBADF;
    ret = p_wmakepath_s(bufferW, 0, NULL, NULL, NULL, NULL);
    ok(ret == EINVAL, "Expected _wmakepath_s to return EINVAL, got %d\n", ret);
    ok(errno == EINVAL, "Expected errno to be EINVAL, got %d\n", errno);

    /* Test with the normal _makepath cases. */
    for (i = 0; i < ARRAY_SIZE(makepath_cases); i++)
    {
        const makepath_case *p = makepath_cases + i;

        memset(buffer, 'X', MAX_PATH);
        if (p->buffer)
            strcpy(buffer, p->buffer);

        /* Ascii */
        ret = p_makepath_s(buffer, MAX_PATH,
                           p->drive == USE_BUFF ? buffer : p->drive,
                           p->dir == USE_BUFF ? buffer : p->dir,
                           p->file == USE_BUFF? buffer : p->file,
                           p->ext == USE_BUFF ? buffer : p->ext);
        ok(ret == 0, "[%d] Expected _makepath_s to return 0, got %d\n", i, ret);

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

        ret = p_wmakepath_s(bufferW, MAX_PATH,
                            p->drive == USE_BUFF ? bufferW : p->drive ? driveW : NULL,
                            p->dir == USE_BUFF ? bufferW : p->dir ? dirW : NULL,
                            p->file == USE_BUFF? bufferW : p->file ? fileW : NULL,
                            p->ext == USE_BUFF ? bufferW : p->ext ? extW : NULL);
        ok(ret == 0, "[%d] Expected _wmakepath_s to return 0, got %d\n", i, ret);

        bufferW[MAX_PATH - 1] = '\0';
        WideCharToMultiByte(CP_ACP, 0, bufferW, -1, buffer, MAX_PATH, NULL, NULL);
        ok(!strcmp(p->expected, buffer), "got '%s' for unicode case %d\n", buffer, i);
    }

    /* Try insufficient length cases. */
    for (i = 0; i < ARRAY_SIZE(makepath_s_cases); i++)
    {
        const makepath_s_case *p = makepath_s_cases + i;

        memset(buffer, 'X', MAX_PATH);
        if (p->buffer)
            strcpy(buffer, p->buffer);

        /* Ascii */
        errno = EBADF;
        ret = p_makepath_s(buffer, p->length,
                           p->drive == USE_BUFF ? buffer : p->drive,
                           p->dir == USE_BUFF ? buffer : p->dir,
                           p->file == USE_BUFF? buffer : p->file,
                           p->ext == USE_BUFF ? buffer : p->ext);
        ok(ret == ERANGE, "[%d] Expected _makepath_s to return ERANGE, got %d\n", i, ret);
        ok(errno == ERANGE, "[%d] Expected errno to be ERANGE, got %d\n", i, errno);
        ok(!memcmp(p->expected, buffer, p->expected_length), "unexpected output for case %d\n", i);

        /* Unicode */
        if (p->drive != USE_BUFF) MultiByteToWideChar(CP_ACP, 0, p->drive, -1, driveW, MAX_PATH);
        if (p->dir != USE_BUFF) MultiByteToWideChar(CP_ACP, 0, p->dir, -1, dirW, MAX_PATH);
        if (p->file != USE_BUFF) MultiByteToWideChar(CP_ACP, 0, p->file, -1, fileW, MAX_PATH);
        if (p->ext != USE_BUFF) MultiByteToWideChar(CP_ACP, 0, p->ext, -1, extW, MAX_PATH);

        memset(buffer, 0, MAX_PATH);
        for (n = 0; n < MAX_PATH; ++n)
            bufferW[n] = 'X';
        if (p->buffer) MultiByteToWideChar( CP_ACP, 0, p->buffer, -1, bufferW, MAX_PATH);

        errno = EBADF;
        ret = p_wmakepath_s(bufferW, p->length,
                            p->drive == USE_BUFF ? bufferW : p->drive ? driveW : NULL,
                            p->dir == USE_BUFF ? bufferW : p->dir ? dirW : NULL,
                            p->file == USE_BUFF? bufferW : p->file ? fileW : NULL,
                            p->ext == USE_BUFF ? bufferW : p->ext ? extW : NULL);
        ok(ret == ERANGE, "[%d] Expected _wmakepath_s to return ERANGE, got %d\n", i, ret);
        ok(errno == ERANGE, "[%d] Expected errno to be ERANGE, got %d\n", i, errno);

        ok(!memcmp(p->expected_unicode, bufferW, p->expected_length * sizeof(WCHAR)), "unexpected output for case %d\n", i);
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
    GetCurrentDirectoryA(MAX_PATH, prevpath);
    GetTempPathA(MAX_PATH,tmppath);
    strcpy(level1,tmppath);
    strcat(level1,"msvcrt-test\\");

    rc = CreateDirectoryA(level1,NULL);
    if (!rc && GetLastError()==ERROR_ALREADY_EXISTS)
        free1=FALSE;

    strcpy(level2,level1);
    strcat(level2,"nextlevel\\");
    rc = CreateDirectoryA(level2,NULL);
    if (!rc && GetLastError()==ERROR_ALREADY_EXISTS)
        free2=FALSE;
    SetCurrentDirectoryA(level2);

    ok(_fullpath(full,"test", MAX_PATH)!=NULL,"_fullpath failed\n");
    strcpy(teststring,level2);
    strcat(teststring,"test");
    ok(strcmp(full,teststring)==0,"Invalid Path returned %s\n",full);
    ok(_fullpath(full,"\\test", MAX_PATH)!=NULL,"_fullpath failed\n");
    memcpy(teststring,level2,3);
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

    SetCurrentDirectoryA(prevpath);
    if (free2)
        RemoveDirectoryA(level2);
    if (free1)
        RemoveDirectoryA(level1);
}

static void test_splitpath(void)
{
    const char* path = "c:\\\x83\x5c\x83\x74\x83\x67.bin";
    char drive[3], dir[MAX_PATH], fname[MAX_PATH], ext[MAX_PATH];
    int prev_cp = _getmbcp();

    /* SBCS codepage */
    _setmbcp(1252);
    _splitpath(path, drive, dir, fname, ext);
    ok(!strcmp(drive, "c:"), "got %s\n", drive);
    ok(!strcmp(dir, "\\\x83\x5c"), "got %s\n", dir);
    ok(!strcmp(fname, "\x83\x74\x83\x67"), "got %s\n", fname);
    ok(!strcmp(ext, ".bin"), "got %s\n", ext);

    /* MBCS (Japanese) codepage */
    _setmbcp(932);
    _splitpath(path, drive, dir, fname, ext);
    ok(!strcmp(drive, "c:"), "got %s\n", drive);
    ok(!strcmp(dir, "\\"), "got %s\n", dir);
    ok(!strcmp(fname, "\x83\x5c\x83\x74\x83\x67"), "got %s\n", fname);
    ok(!strcmp(ext, ".bin"), "got %s\n", ext);

    _setmbcp(prev_cp);
}

static void test_searchenv(void)
{
    const char *dirs[] = {
        "\\search_env_test",
        "\\search_env_test\\dir1",
        "\\search_env_test\\dir2",
        "\\search_env_test\\dir3longer"
    };

    const char *files[] = {
        "\\search_env_test\\dir1\\1.dat",
        "\\search_env_test\\dir1\\2.dat",
        "\\search_env_test\\dir2\\1.dat",
        "\\search_env_test\\dir2\\3.dat",
        "\\search_env_test\\dir3longer\\3.dat"
    };

    char env1[4*MAX_PATH], env2[4*MAX_PATH], tmppath[MAX_PATH], path[2*MAX_PATH];
    char result[MAX_PATH], exp[2*MAX_PATH];
    WCHAR result_w[MAX_PATH];
    int i, path_len;
    FILE *tmp_file;

    if (getenv("TEST_PATH")) {
        skip("TEST_PATH environment variable already set\n");
        return;
    }

    path_len = GetTempPathA(MAX_PATH, tmppath);
    ok(path_len, "GetTempPath failed\n");
    memcpy(path, tmppath, path_len);

    for (i=0; i<ARRAY_SIZE(dirs); i++) {
        strcpy(path+path_len, dirs[i]);
	ok(!mkdir(path), "mkdir failed (dir = %s)\n", path);
    }

    for (i=0; i<ARRAY_SIZE(files); i++) {
        strcpy(path+path_len, files[i]);
        tmp_file = fopen(path, "wb");
	ok(tmp_file != NULL, "fopen failed (file = %s)\n", path);
        fclose(tmp_file);
    }

    strcpy(env1, "TEST_PATH=");
    strcpy(env2, "TEST_PATH=;");
    for (i=1; i<ARRAY_SIZE(dirs); i++) {
        strcat(env1, tmppath);
        strcat(env1, dirs[i]);
        strcat(env1, ";");

        strcat(env2, tmppath);
        strcat(env2, dirs[i]);
        strcat(env2, ";;");
    }

    if (!p_searchenv_s || !p_wsearchenv_s)
        win_skip("searchenv_s or wsearchenv_s function is not available\n");

    putenv(env1);
    memset(result, 'x', sizeof(result));
    _searchenv("fail", "TEST_PATH", result);
    ok(!result[0], "got %s, expected ''\n", result);

    if (p_searchenv_s) {
        memset(result, 'x', sizeof(result));
        i = p_searchenv_s("fail", "TEST_PATH", result, MAX_PATH);
        ok(i == ENOENT, "searchenv_s returned %d\n", i);
        ok(!result[0], "got %s, expected ''\n", result);
    }

    memset(result, 'x', sizeof(result));
    strcpy(exp, tmppath);
    strcat(exp, files[0]);
    _searchenv("1.dat", "TEST_PATH", result);
    ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);

    if (p_searchenv_s) {
        memset(result, 'x', sizeof(result));
        i = p_searchenv_s("1.dat", "TEST_PATH", result, MAX_PATH);
        ok(!i, "searchenv_s returned %d\n", i);
        ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);
    }

    memset(result_w, 'x', sizeof(result_w));
    _wsearchenv(L"1.dat", L"TEST_PATH", result_w);
    WideCharToMultiByte(CP_ACP, 0, result_w, -1, result, MAX_PATH, NULL, NULL);
    ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);

    if (p_wsearchenv_s) {
        memset(result_w, 'x', sizeof(result_w));
        i = p_wsearchenv_s(L"1.dat", L"TEST_PATH", result_w, MAX_PATH);
        ok(!i, "wsearchenv_s returned %d\n", i);
        ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);
    }

    memset(result, 'x', sizeof(result));
    strcpy(exp, tmppath);
    strcat(exp, files[3]);
    _searchenv("3.dat", "TEST_PATH", result);
    ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);

    if (p_searchenv_s) {
        memset(result, 'x', sizeof(result));
        i = p_searchenv_s("3.dat", "TEST_PATH", result, MAX_PATH);
        ok(!i, "searchenv_s returned %d\n", i);
        ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);
    }

    memset(result_w, 'x', sizeof(result_w));
    _wsearchenv(L"3.dat", L"TEST_PATH", result_w);
    WideCharToMultiByte(CP_ACP, 0, result_w, -1, result, MAX_PATH, NULL, NULL);
    ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);

    if (p_wsearchenv_s) {
        memset(result_w, 'x', sizeof(result_w));
        i = p_wsearchenv_s(L"3.dat", L"TEST_PATH", result_w, MAX_PATH);
        ok(!i, "wsearchenv_s returned %d\n", i);
        ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);
    }

    putenv(env2);
    memset(result, 'x', sizeof(result));
    strcpy(exp, tmppath);
    strcat(exp, files[0]);
    _searchenv("1.dat", "TEST_PATH", result);
    ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);

    if (p_searchenv_s) {
        memset(result, 'x', sizeof(result));
        i = p_searchenv_s("1.dat", "TEST_PATH", result, MAX_PATH);
        ok(!i, "searchenv_s returned %d\n", i);
        ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);
    }

    memset(result_w, 'x', sizeof(result_w));
    _wsearchenv(L"1.dat", L"TEST_PATH", result_w);
    WideCharToMultiByte(CP_ACP, 0, result_w, -1, result, MAX_PATH, NULL, NULL);
    ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);

    if (p_wsearchenv_s) {
        memset(result_w, 'x', sizeof(result_w));
        i = p_wsearchenv_s(L"1.dat", L"TEST_PATH", result_w, MAX_PATH);
        ok(!i, "wsearchenv_s returned %d\n", i);
        ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);
    }

    memset(result, 'x', sizeof(result));
    strcpy(exp, tmppath);
    strcat(exp, files[3]);
    _searchenv("3.dat", "TEST_PATH", result);
    ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);

    if (p_searchenv_s) {
        memset(result, 'x', sizeof(result));
        i = p_searchenv_s("3.dat", "TEST_PATH", result, MAX_PATH);
        ok(!i, "searchenv_s returned %d\n", i);
        ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);
    }

    memset(result_w, 'x', sizeof(result_w));
    _wsearchenv(L"3.dat", L"TEST_PATH", result_w);
    WideCharToMultiByte(CP_ACP, 0, result_w, -1, result, MAX_PATH, NULL, NULL);
    ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);

    if (p_wsearchenv_s) {
        memset(result_w, 'x', sizeof(result_w));
        i = p_wsearchenv_s(L"3.dat", L"TEST_PATH", result_w, MAX_PATH);
        ok(!i, "wsearchenv_s returned %d\n", i);
        ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);
    }

    strcpy(env1, "TEST_PATH=");
    strcat(env1, tmppath);
    strcat(env1, "\"\\search_env_test\\\"d\"i\"r\"1");
    putenv(env1);
    strcpy(exp, tmppath);
    strcat(exp, files[0]);
    _searchenv("1.dat", "TEST_PATH", result);
    ok(!strcmp(result, exp), "got %s, expected '%s'\n", result, exp);

    strcat(env1, ";");
    putenv(env1);
    _searchenv("1.dat", "TEST_PATH", result);
    ok(!result[0], "got %s, expected ''\n", result);

    putenv("TEST_PATH=");

    for (i=ARRAY_SIZE(files)-1; i>=0; i--) {
        strcpy(path+path_len, files[i]);
        ok(!remove(path), "remove failed (file = %s)\n", path);
    }

    for (i=ARRAY_SIZE(dirs)-1; i>=0; i--) {
        strcpy(path+path_len, dirs[i]);
        ok(!rmdir(path), "rmdir failed (dir = %s)\n", path);
    }
}

START_TEST(dir)
{
    init();

    test_fullpath();
    test_makepath();
    test_makepath_s();
    test_splitpath();
    test_searchenv();
}
