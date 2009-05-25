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

typedef void (*_INITTERMFUN)(void);
static void (*p_initterm)(_INITTERMFUN *start, _INITTERMFUN *end);

static int callbacked;

static void initcallback(void)
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
    OSVERSIONINFO osvi = { sizeof(OSVERSIONINFO) };
    int *pp_winver   = (int*)GetProcAddress(hmsvcrt, "_winver");
    int *pp_winmajor = (int*)GetProcAddress(hmsvcrt, "_winmajor");
    int *pp_winminor = (int*)GetProcAddress(hmsvcrt, "_winminor");
    int *pp_osver    = (int*)GetProcAddress(hmsvcrt, "_osver");
    unsigned int winver, winmajor, winminor, osver;

    if( !( pp_winmajor && pp_winminor && pp_winver)) {
        win_skip("_winver variables are not available\n");
        return;
    }
    winver = *pp_winver;
    winminor = *pp_winminor;
    winmajor = *pp_winmajor;
    GetVersionEx( &osvi);
    ok( winminor == osvi.dwMinorVersion, "Wrong value for _winminor %02x expected %02x\n",
            winminor, osvi.dwMinorVersion);
    ok( winmajor == osvi.dwMajorVersion, "Wrong value for _winmajor %02x expected %02x\n",
            winmajor, osvi.dwMajorVersion);
    ok( winver == ((osvi.dwMajorVersion << 8) | osvi.dwMinorVersion),
            "Wrong value for _winver %02x expected %02x\n",
            winver, ((osvi.dwMajorVersion << 8) | osvi.dwMinorVersion));
    if( !pp_osver) {
        win_skip("_osver variables are not available\n");
        return;
    }
    osver = *pp_osver;
    ok( osver == (osvi.dwBuildNumber & 0xffff) ||
            ((osvi.dwBuildNumber >> 24) == osvi.dwMajorVersion &&
                 ((osvi.dwBuildNumber >> 16) & 0xff) == osvi.dwMinorVersion), /* 95/98/ME */
            "Wrong value for _osver %04x expected %04x\n",
            osver, osvi.dwBuildNumber);
}

START_TEST(data)
{
    HMODULE hmsvcrt;

    hmsvcrt = GetModuleHandleA("msvcrt.dll");
    if (!hmsvcrt)
        hmsvcrt = GetModuleHandleA("msvcrtd.dll");
    if (hmsvcrt)
        p_initterm=(void*)GetProcAddress(hmsvcrt, "_initterm");
    test_initterm();
    test_initvar(hmsvcrt);
}
