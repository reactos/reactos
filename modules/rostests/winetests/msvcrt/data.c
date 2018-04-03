/*
 * Tests msvcrt/data.c
 *
 * Copyright 2006 Andrew Ziem
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
#include <direct.h>

void __cdecl __getmainargs(int *, char ***, char ***, int, int *);
static int* (__cdecl *p___p___argc)(void);
static char*** (__cdecl *p___p___argv)(void);

typedef void (__cdecl *_INITTERMFUN)(void);
static void (__cdecl *p_initterm)(_INITTERMFUN *start, _INITTERMFUN *end);

static int (__cdecl *p_get_pgmptr)(char **p);

static int callbacked;

static void __cdecl initcallback(void)
{
   callbacked++;
}

#define initterm_test(start, end, expected) \
    callbacked = 0; \
    p_initterm(start, end); \
    ok(expected == callbacked,"_initterm: callbacks count mismatch: got %i, expected %i\n", callbacked, expected);

static void test_initterm(void)
{
    int i;
    static _INITTERMFUN callbacks[4];

    if (!p_initterm)
        return;

    for (i = 0; i < 4; i++)
    {
        callbacks[i] = initcallback;
    }

    initterm_test(&callbacks[0], &callbacks[1], 1);
    initterm_test(&callbacks[0], &callbacks[2], 2);
    initterm_test(&callbacks[0], &callbacks[3], 3);

    callbacks[1] = NULL;
    initterm_test(&callbacks[0], &callbacks[3], 2);
}

static void test_initvar( HMODULE hmsvcrt )
{
    OSVERSIONINFOA osvi = { sizeof(OSVERSIONINFOA) };
    int *pp_winver   = (int*)GetProcAddress(hmsvcrt, "_winver");
    int *pp_winmajor = (int*)GetProcAddress(hmsvcrt, "_winmajor");
    int *pp_winminor = (int*)GetProcAddress(hmsvcrt, "_winminor");
    int *pp_osver    = (int*)GetProcAddress(hmsvcrt, "_osver");
    int *pp_osplatform = (int*)GetProcAddress(hmsvcrt, "_osplatform");
    unsigned int winver, winmajor, winminor, osver, osplatform;

    if( !( pp_winmajor && pp_winminor && pp_winver)) {
        win_skip("_winver variables are not available\n");
        return;
    }
    winver = *pp_winver;
    winminor = *pp_winminor;
    winmajor = *pp_winmajor;
    GetVersionExA( &osvi);
    ok( winminor == osvi.dwMinorVersion, "Wrong value for _winminor %02x expected %02x\n",
            winminor, osvi.dwMinorVersion);
    ok( winmajor == osvi.dwMajorVersion, "Wrong value for _winmajor %02x expected %02x\n",
            winmajor, osvi.dwMajorVersion);
    ok( winver == ((osvi.dwMajorVersion << 8) | osvi.dwMinorVersion),
            "Wrong value for _winver %02x expected %02x\n",
            winver, ((osvi.dwMajorVersion << 8) | osvi.dwMinorVersion));
    if( !pp_osver || !pp_osplatform ) {
        win_skip("_osver variables are not available\n");
        return;
    }
    osver = *pp_osver;
    osplatform = *pp_osplatform;
    ok( osver == (osvi.dwBuildNumber & 0xffff) ||
            ((osvi.dwBuildNumber >> 24) == osvi.dwMajorVersion &&
                 ((osvi.dwBuildNumber >> 16) & 0xff) == osvi.dwMinorVersion), /* 95/98/ME */
            "Wrong value for _osver %04x expected %04x\n",
            osver, osvi.dwBuildNumber);
    ok(osplatform == osvi.dwPlatformId,
            "Wrong value for _osplatform %x expected %x\n",
            osplatform, osvi.dwPlatformId);
}

static void test_get_pgmptr(void)
{
    char *pgm = NULL;
    int res;

    if (!p_get_pgmptr)
        return;

    res = p_get_pgmptr(&pgm);

    ok( res == 0, "Wrong _get_pgmptr return value %d expected 0\n", res);
    ok( pgm != NULL, "_get_pgmptr returned a NULL pointer\n" );
}

static void test___getmainargs(void)
{
    int argc, new_argc, mode;
    char **argv, **new_argv, **envp;
    char tmppath[MAX_PATH], filepath[MAX_PATH];
    FILE *f;

    ok(GetTempPathA(MAX_PATH, tmppath) != 0, "GetTempPath failed\n");

    mode = 0;
    __getmainargs(&argc, &argv, &envp, 0, &mode);
    ok(argc == 4, "argc = %d\n", argc);
    ok(!strcmp(argv[1], "data"), "argv[1] = %s\n", argv[1]);
    sprintf(filepath, "%s*\\*", tmppath);
    ok(!strcmp(argv[2], filepath), "argv[2] = %s\n", argv[2]);
    sprintf(filepath, "%swine_test/*", tmppath);
    ok(!strcmp(argv[3], filepath), "argv[3] = %s\n", argv[3]);
    ok(!argv[4], "argv[4] != NULL\n");

    if(p___p___argc && p___p___argv) {
        new_argc = *p___p___argc();
        new_argv = *p___p___argv();
        ok(new_argc == 4, "*__p___argc() = %d\n", new_argc);
        ok(new_argv == argv, "*__p___argv() = %p, expected %p\n", new_argv, argv);
    }else {
        win_skip("__p___argc or __p___argv is not available\n");
    }

    mode = 0;
    __getmainargs(&argc, &argv, &envp, 1, &mode);
    ok(argc == 5, "argc = %d\n", argc);
    ok(!strcmp(argv[1], "data"), "argv[1] = %s\n", argv[1]);
    sprintf(filepath, "%s*\\*", tmppath);
    ok(!strcmp(argv[2], filepath), "argv[2] = %s\n", argv[2]);
    sprintf(filepath, "%swine_test/a", tmppath);
    if(argv[3][strlen(argv[3])-1] == 'a') {
        ok(!strcmp(argv[3], filepath), "argv[3] = %s\n", argv[3]);
        sprintf(filepath, "%swine_test/test", tmppath);
        ok(!strcmp(argv[4], filepath), "argv[4] = %s\n", argv[4]);
    }else {
        ok(!strcmp(argv[4], filepath), "argv[4] = %s\n", argv[4]);
        sprintf(filepath, "%swine_test/test", tmppath);
        ok(!strcmp(argv[3], filepath), "argv[3] = %s\n", argv[3]);
    }
    ok(!argv[5], "argv[5] != NULL\n");

    if(p___p___argc && p___p___argv) {
        new_argc = *p___p___argc();
        new_argv = *p___p___argv();
        ok(new_argc == argc, "*__p___argc() = %d, expected %d\n", new_argc, argc);
        ok(new_argv == argv, "*__p___argv() = %p, expected %p\n", new_argv, argv);
    }

    sprintf(filepath, "%swine_test/b", tmppath);
    f = fopen(filepath, "w");
    ok(f != NULL, "fopen(%s) failed: %d\n", filepath, errno);
    fclose(f);
    mode = 0;
    __getmainargs(&new_argc, &new_argv, &envp, 1, &mode);
    ok(new_argc == argc+1, "new_argc = %d, expected %d\n", new_argc, argc+1);
    _unlink(filepath);
}

static void test___getmainargs_parent(char *name)
{
    char cmdline[3*MAX_PATH];
    char tmppath[MAX_PATH], filepath[MAX_PATH];
    STARTUPINFOA startup;
    PROCESS_INFORMATION proc;
    FILE *f;
    int ret;

    ok(GetTempPathA(MAX_PATH, tmppath) != 0, "GetTempPath failed\n");
    sprintf(cmdline, "%s data %s*\\* %swine_test/*", name, tmppath, tmppath);

    sprintf(filepath, "%swine_test", tmppath);
    ret = _mkdir(filepath);
    ok(!ret, "_mkdir failed: %d\n", errno);
    sprintf(filepath, "%swine_test\\a", tmppath);
    f = fopen(filepath, "w");
    ok(f != NULL, "fopen(%s) failed: %d\n", filepath, errno);
    fclose(f);
    sprintf(filepath, "%swine_test\\test", tmppath);
    f = fopen(filepath, "w");
    ok(f != NULL, "fopen(%s) failed: %d\n", filepath, errno);
    fclose(f);

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    CreateProcessA(NULL, cmdline, NULL, NULL, TRUE, CREATE_DEFAULT_ERROR_MODE|NORMAL_PRIORITY_CLASS, NULL, NULL, &startup, &proc);
    winetest_wait_child_process(proc.hProcess);

    _unlink(filepath);
    sprintf(filepath, "%swine_test\\a", tmppath);
    _unlink(filepath);
    sprintf(filepath, "%swine_test", tmppath);
    _rmdir(filepath);
}

START_TEST(data)
{
    HMODULE hmsvcrt;
    int arg_c;
    char** arg_v;

    hmsvcrt = GetModuleHandleA("msvcrt.dll");
    if (!hmsvcrt)
        hmsvcrt = GetModuleHandleA("msvcrtd.dll");
    if (hmsvcrt)
    {
        p_initterm=(void*)GetProcAddress(hmsvcrt, "_initterm");
        p_get_pgmptr=(void*)GetProcAddress(hmsvcrt, "_get_pgmptr");
        p___p___argc=(void*)GetProcAddress(hmsvcrt, "__p___argc");
        p___p___argv=(void*)GetProcAddress(hmsvcrt, "__p___argv");
    }

    arg_c = winetest_get_mainargs(&arg_v);
    if(arg_c >= 3) {
        test___getmainargs();
        return;
    }

    test_initterm();
    test_initvar(hmsvcrt);
    test_get_pgmptr();
    test___getmainargs_parent(arg_v[0]);
}
