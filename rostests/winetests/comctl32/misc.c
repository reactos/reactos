/*
 * Misc tests
 *
 * Copyright 2006 Paul Vriens
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

#include <stdio.h>
#include <windows.h>

#include "wine/test.h"

static PVOID (WINAPI * pAlloc)(LONG);
static PVOID (WINAPI * pReAlloc)(PVOID, LONG);
static BOOL (WINAPI * pFree)(PVOID);
static LONG (WINAPI * pGetSize)(PVOID);

static INT (WINAPI * pStr_GetPtrA)(LPCSTR, LPSTR, INT);
static BOOL (WINAPI * pStr_SetPtrA)(LPSTR, LPCSTR);
static INT (WINAPI * pStr_GetPtrW)(LPCWSTR, LPWSTR, INT);
static BOOL (WINAPI * pStr_SetPtrW)(LPWSTR, LPCWSTR);

static HMODULE hComctl32 = 0;

#define COMCTL32_GET_PROC(ordinal, func) \
    p ## func = (void*)GetProcAddress(hComctl32, (LPSTR)ordinal); \
    if(!p ## func) { \
      trace("GetProcAddress(%d)(%s) failed\n", ordinal, #func); \
      FreeLibrary(hComctl32); \
    }

#define expect(expected, got) ok(expected == got, "expected %d, got %d\n", expected,got)

static BOOL InitFunctionPtrs(void)
{
    hComctl32 = LoadLibraryA("comctl32.dll");

    if(!hComctl32)
    {
        trace("Could not load comctl32.dll\n");
        return FALSE;
    }

    COMCTL32_GET_PROC(71, Alloc);
    COMCTL32_GET_PROC(72, ReAlloc);
    COMCTL32_GET_PROC(73, Free);
    COMCTL32_GET_PROC(74, GetSize);

    COMCTL32_GET_PROC(233, Str_GetPtrA)
    COMCTL32_GET_PROC(234, Str_SetPtrA)
    COMCTL32_GET_PROC(235, Str_GetPtrW)
    COMCTL32_GET_PROC(236, Str_SetPtrW)

    return TRUE;
}

static void test_GetPtrAW(void)
{
    if (pStr_GetPtrA)
    {
        static const char source[] = "Just a source string";
        static const char desttest[] = "Just a destination string";
        static char dest[MAX_PATH];
        int sourcelen;
        int destsize = MAX_PATH;
        int count = -1;

        sourcelen = strlen(source) + 1;

        count = pStr_GetPtrA(NULL, NULL, 0);
        ok (count == 0, "Expected count to be 0, it was %d\n", count);

        if (0)
        {
            /* Crashes on W98, NT4, W2K, XP, W2K3
             * Our implementation also crashes and we should probably leave
             * it like that.
             */
            count = -1;
            count = pStr_GetPtrA(NULL, NULL, destsize);
            trace("count : %d\n", count);
        }

        count = 0;
        count = pStr_GetPtrA(source, NULL, 0);
        ok (count == sourcelen, "Expected count to be %d, it was %d\n", sourcelen , count);

        count = 0;
        strcpy(dest, desttest);
        count = pStr_GetPtrA(source, dest, 0);
        ok (count == sourcelen, "Expected count to be %d, it was %d\n", sourcelen , count);
        ok (!lstrcmp(dest, desttest), "Expected destination to not have changed\n");

        count = 0;
        count = pStr_GetPtrA(source, NULL, destsize);
        ok (count == sourcelen, "Expected count to be %d, it was %d\n", sourcelen , count);

        count = 0;
        count = pStr_GetPtrA(source, dest, destsize);
        ok (count == sourcelen, "Expected count to be %d, it was %d\n", sourcelen , count);
        ok (!lstrcmp(source, dest), "Expected source and destination to be the same\n");

        count = -1;
        strcpy(dest, desttest);
        count = pStr_GetPtrA(NULL, dest, destsize);
        ok (count == 0, "Expected count to be 0, it was %d\n", count);
        ok (dest[0] == '\0', "Expected destination to be cut-off and 0 terminated\n");

        count = 0;
        destsize = 15;
        count = pStr_GetPtrA(source, dest, destsize);
        ok (count == 15, "Expected count to be 15, it was %d\n", count);
        ok (!memcmp(source, dest, 14), "Expected first part of source and destination to be the same\n");
        ok (dest[14] == '\0', "Expected destination to be cut-off and 0 terminated\n");
    }
}

static void test_Alloc(void)
{
    PCHAR p = pAlloc(0);
    ok(p != NULL, "p=%p\n", p);
    ok(pFree(p), "\n");
    p = pAlloc(1);
    ok(p != NULL, "\n");
    *p = '\0';
    expect(1, pGetSize(p));
    p = pReAlloc(p, 2);
    ok(p != NULL, "\n");
    expect(2, pGetSize(p));
    ok(pFree(p), "\n");
    ok(pFree(NULL), "\n");
    p = pReAlloc(NULL, 2);
    ok(p != NULL, "\n");
    ok(pFree(p), "\n");
}

START_TEST(misc)
{
    if(!InitFunctionPtrs())
        return;

    test_GetPtrAW();
    test_Alloc();

    FreeLibrary(hComctl32);
}
