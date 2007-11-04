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

START_TEST(data)
{
    HMODULE hmsvcrt;
    hmsvcrt = GetModuleHandleA("msvcrt.dll");
    if (!hmsvcrt)
        hmsvcrt = GetModuleHandleA("msvcrtd.dll");
    if (hmsvcrt)
        p_initterm=(void*)GetProcAddress(hmsvcrt, "_initterm");
    test_initterm();
}
