/*
 * Unit test suite for serialui API functions
 *
 * Copyright 2007 Detlef Riekenberg
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
 *
 */

#include <stdarg.h>
#include <stdio.h>

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnls.h"

#include "wine/test.h"


static HINSTANCE   hdll;
static DWORD  (WINAPI *pCommConfigDialogA)(LPCSTR, HWND, LPCOMMCONFIG);
static DWORD  (WINAPI *pCommConfigDialogW)(LPCWSTR, HWND, LPCOMMCONFIG);
static DWORD  (WINAPI *pGetDefaultCommConfigA)(LPCSTR, LPCOMMCONFIG, LPDWORD);
static DWORD  (WINAPI *pGetDefaultCommConfigW)(LPCWSTR, LPCOMMCONFIG, LPDWORD);


static const CHAR  com1A[]      = "com1";
static const CHAR  emptyA[]     = "";
static const CHAR  fmt_comA[]   = "com%d";
static const CHAR  str_colonA[] = ":";

static const WCHAR com1W[]      = {'c','o','m','1',0};
static const WCHAR emptyW[]     = {0};
static const WCHAR str_colonW[] = {':',0};

/* ################# */

static LPCSTR load_functions(void)
{
    LPCSTR ptr;

    ptr = "serialui.dll";
    hdll = LoadLibraryA(ptr);
    if (!hdll) return ptr;

    ptr = "drvCommConfigDialogA";
    pCommConfigDialogA = (VOID *) GetProcAddress(hdll, ptr);
    if (!pCommConfigDialogA) return ptr;

    ptr = "drvCommConfigDialogW";
    pCommConfigDialogW = (VOID *) GetProcAddress(hdll, ptr);
    if (!pCommConfigDialogW) return ptr;

    ptr = "drvGetDefaultCommConfigA";
    pGetDefaultCommConfigA = (VOID *) GetProcAddress(hdll, ptr);
    if (!pGetDefaultCommConfigA) return ptr;

    ptr = "drvGetDefaultCommConfigW";
    pGetDefaultCommConfigW = (VOID *) GetProcAddress(hdll, ptr);
    if (!pGetDefaultCommConfigW) return ptr;


    return NULL;
}

/* ################# */

static void test_drvCommConfigDialogA(void)
{
    COMMCONFIG  pCC[3];
    CHAR        bufferA[16];
    DWORD       i;
    DWORD       res;
    DWORD       len;


    /* test ports "com1" - "com4" */
    for (i = 1; i < 5 ; i++) {
        sprintf(bufferA, fmt_comA, i);
        len = sizeof(pCC);
        ZeroMemory(pCC, sizeof(pCC));
        SetLastError(0xdeadbeef);
        res = pGetDefaultCommConfigA(bufferA, pCC, &len);
        if (res == ERROR_CALL_NOT_IMPLEMENTED) {
            /* NT does not implement the ANSI API */
            win_skip("*A not implemented\n");
            return;
        }

        if (res == ERROR_SUCCESS) {

            if (winetest_interactive) {
                SetLastError(0xdeadbeef);
                res = pCommConfigDialogA(bufferA, NULL, pCC);
                /* OK: ERROR_SUCCESS,  Cancel: ERROR_CANCELLED */
                trace("returned %u with %u for '%s'\n", res, GetLastError(), bufferA);
            }

            ZeroMemory(pCC, sizeof(pCC));
            SetLastError(0xdeadbeef);
            res = pCommConfigDialogA(bufferA, NULL, pCC);
            ok( res == ERROR_INSUFFICIENT_BUFFER,
                "returned %u with %u for '%s' (expected ERROR_INSUFFICIENT_BUFFER)\n",
                res, GetLastError(), bufferA);


            SetLastError(0xdeadbeef);
            res = pCommConfigDialogA(bufferA, NULL, NULL);
            ok( res == ERROR_INVALID_PARAMETER,
                "returned %u with %u for '%s' (expected ERROR_INVALID_PARAMETER)\n",
                res, GetLastError(), bufferA);
        }
    }


    ZeroMemory(pCC, sizeof(pCC));
    SetLastError(0xdeadbeef);
    res = pCommConfigDialogA(emptyA, NULL, pCC);
    ok( res == ERROR_INSUFFICIENT_BUFFER,
        "returned %u with %u (expected ERROR_INSUFFICIENT_BUFFER)\n",
        res, GetLastError());


    ZeroMemory(pCC, sizeof(pCC));
    pCC[0].dwSize = sizeof(COMMCONFIG);
    SetLastError(0xdeadbeef);
    res = pCommConfigDialogA(emptyA, NULL, pCC);
    ok( res == ERROR_BADKEY, "returned %u with %u (expected ERROR_BADKEY)\n",
        res, GetLastError());


    ZeroMemory(pCC, sizeof(pCC));
    SetLastError(0xdeadbeef);
    res = pCommConfigDialogA(NULL, NULL, pCC);
    ok( res == ERROR_INVALID_PARAMETER,
        "returned %u with %u (expected ERROR_INVALID_PARAMETER)\n",
        res, GetLastError());
}

/* ################# */

static void test_drvCommConfigDialogW(void)
{
    COMMCONFIG  pCC[3];
    CHAR        bufferA[16];
    WCHAR       bufferW[16];
    DWORD       i;
    DWORD       res;
    DWORD       len;


    /* test ports "com1" - "com4" */
    for (i = 1; i < 5 ; i++) {
        sprintf(bufferA, fmt_comA, i);
        MultiByteToWideChar( CP_ACP, 0, bufferA, -1, bufferW, sizeof(bufferW)/sizeof(WCHAR) );
        len = sizeof(pCC);
        ZeroMemory(pCC, sizeof(pCC));
        SetLastError(0xdeadbeef);
        res = pGetDefaultCommConfigW(bufferW, pCC, &len);
        if (res == ERROR_CALL_NOT_IMPLEMENTED) {
            win_skip("*W not implemented\n");
            return;
        }

        if (res == ERROR_SUCCESS) {

            if (winetest_interactive) {
                SetLastError(0xdeadbeef);
                res = pCommConfigDialogW(bufferW, NULL, pCC);
                /* OK: ERROR_SUCCESS,  Cancel: ERROR_CANCELLED */
                trace("returned %u with %u for '%s'\n", res, GetLastError(), bufferA);
            }

            ZeroMemory(pCC, sizeof(pCC));
            SetLastError(0xdeadbeef);
            res = pCommConfigDialogW(bufferW, NULL, pCC);
            ok( res == ERROR_INSUFFICIENT_BUFFER,
                "returned %u with %u for '%s' (expected ERROR_INSUFFICIENT_BUFFER)\n",
                res, GetLastError(), bufferA);

            SetLastError(0xdeadbeef);
            res = pCommConfigDialogW(bufferW, NULL, NULL);
            ok( res == ERROR_INVALID_PARAMETER,
                "returned %u with %u for '%s' (expected ERROR_INVALID_PARAMETER)\n",
                res, GetLastError(), bufferA);
        }
    }


    ZeroMemory(pCC, sizeof(pCC));
    SetLastError(0xdeadbeef);
    res = pCommConfigDialogW(emptyW, NULL, pCC);
    ok( res == ERROR_INSUFFICIENT_BUFFER,
        "returned %u with %u (expected ERROR_INSUFFICIENT_BUFFER)\n",
        res, GetLastError());


    ZeroMemory(pCC, sizeof(pCC));
    pCC[0].dwSize = sizeof(COMMCONFIG);
    SetLastError(0xdeadbeef);
    res = pCommConfigDialogW(emptyW, NULL, pCC);
    ok( res == ERROR_BADKEY, "returned %u with %u (expected ERROR_BADKEY)\n",
        res, GetLastError());


    ZeroMemory(pCC, sizeof(pCC));
    SetLastError(0xdeadbeef);
    res = pCommConfigDialogW(NULL, NULL, pCC);
    ok( res == ERROR_INVALID_PARAMETER,
        "returned %u with %u (expected ERROR_INVALID_PARAMETER)\n",
        res, GetLastError());
}


/* ################# */

static void test_drvGetDefaultCommConfigA(void)
{
    COMMCONFIG  pCC[3];
    CHAR        bufferA[16];
    DWORD       i;
    DWORD       res;
    DWORD       len;


    /* off by one: one byte smaller */
    i   = sizeof(COMMCONFIG);
    len = sizeof(COMMCONFIG) -1;
    ZeroMemory(pCC, sizeof(pCC));
    SetLastError(0xdeadbeef);
    res = pGetDefaultCommConfigA(com1A, pCC, &len);
    if (res == ERROR_CALL_NOT_IMPLEMENTED) {
        /* NT does not implement the ANSI API */
        win_skip("*A not implemented\n");
        return;
    }
    ok( (res == ERROR_INSUFFICIENT_BUFFER) && (len >= i),
        "returned %u with %u and %u (expected "
        "ERROR_INSUFFICIENT_BUFFER and '>= %u')\n", res, GetLastError(), len, i);

    /* test ports "com0" - "com10" */
    for (i = 0; i < 11 ; i++) {
        sprintf(bufferA, fmt_comA, i);
        len = sizeof(pCC);
        ZeroMemory(pCC, sizeof(pCC));
        SetLastError(0xdeadbeef);
        res = pGetDefaultCommConfigA(bufferA, pCC, &len);
        if (i == 0) {
            ok( res == ERROR_BADKEY,
                "returned %u with %u and %u for %s (expected "
                "ERROR_BADKEY)\n", res, GetLastError(), len, bufferA);
        }
        else
        {
            ok((res == ERROR_SUCCESS) || (res == ERROR_BADKEY),
               "returned %u with %u and %u for %s (expected ERROR_SUCCESS or "
               "ERROR_BADKEY)\n", res, GetLastError(), len, bufferA);
        }

        /* a name with a colon is invalid */
        if (res == ERROR_SUCCESS) {
            lstrcatA(bufferA, str_colonA);
            len = sizeof(pCC);
            ZeroMemory(pCC, sizeof(pCC));
            res = pGetDefaultCommConfigA(bufferA, pCC, &len);
            ok( res == ERROR_BADKEY,
                "returned %u with %u and %u for %s (expected '0' with "
                "ERROR_BADKEY)\n", res, GetLastError(), len, bufferA);
        }
    }


    /* an empty String is not allowed */
    len = sizeof(pCC);
    ZeroMemory(pCC, sizeof(pCC));
    SetLastError(0xdeadbeef);
    res = pGetDefaultCommConfigA(emptyA, pCC, &len);
    ok( res == ERROR_BADKEY,
        "returned %u with %u and %u for %s (expected ERROR_BADKEY)\n",
        res, GetLastError(), len, emptyA);

    /* some NULL checks */
    len = sizeof(pCC);
    ZeroMemory(pCC, sizeof(pCC));
    SetLastError(0xdeadbeef);
    res = pGetDefaultCommConfigA(NULL, pCC, &len);
    ok( res == ERROR_INVALID_PARAMETER,
        "returned %u with %u and %u for NULL (expected ERROR_INVALID_PARAMETER)\n",
        res, GetLastError(), len);


    len = sizeof(pCC);
    SetLastError(0xdeadbeef);
    res = pGetDefaultCommConfigA(com1A, NULL, &len);
    ok( res == ERROR_INVALID_PARAMETER,
        "returned %u with %u and %u (expected ERROR_INVALID_PARAMETER)\n",
        res, GetLastError(), len);


    SetLastError(0xdeadbeef);
    res = pGetDefaultCommConfigA(com1A, pCC, NULL);
    ok( res == ERROR_INVALID_PARAMETER,
        "returned %u with %u (expected ERROR_INVALID_PARAMETER)\n",
        res, GetLastError());
}

/* ################# */

static void test_drvGetDefaultCommConfigW(void)
{
    COMMCONFIG  pCC[3];
    WCHAR       bufferW[16];
    CHAR        bufferA[16];
    DWORD       i;
    DWORD       res;
    DWORD       len;


    /* off by one: one byte smaller */
    i   = sizeof(COMMCONFIG);
    len = sizeof(COMMCONFIG) -1;
    ZeroMemory(pCC, sizeof(pCC));
    SetLastError(0xdeadbeef);
    res = pGetDefaultCommConfigW(com1W, pCC, &len);
    if (res == ERROR_CALL_NOT_IMPLEMENTED) {
        win_skip("*W not implemented\n");
        return;
    }
    ok( (res == ERROR_INSUFFICIENT_BUFFER) && (len >= i),
        "returned %u with %u and %u (expected "
        "ERROR_INSUFFICIENT_BUFFER and '>= %u')\n", res, GetLastError(), len, i);

    /* test ports "com0" - "com10" */
    for (i = 0; i < 11 ; i++) {
        sprintf(bufferA, fmt_comA, i);
        MultiByteToWideChar( CP_ACP, 0, bufferA, -1, bufferW, sizeof(bufferW)/sizeof(WCHAR) );
        len = sizeof(pCC);
        ZeroMemory(pCC, sizeof(pCC));
        SetLastError(0xdeadbeef);
        res = pGetDefaultCommConfigW(bufferW, pCC, &len);
        if (i == 0) {
            ok( res == ERROR_BADKEY,
                "returned %u with %u and %u for %s (expected "
                "ERROR_BADKEY)\n", res, GetLastError(), len, bufferA);
        }
        else
        {
            ok((res == ERROR_SUCCESS) || (res == ERROR_BADKEY),
               "returned %u with %u and %u for %s (expected ERROR_SUCCESS or "
               "ERROR_BADKEY)\n", res, GetLastError(), len, bufferA);
        }

        /* a name with a colon is invalid */
        if (res == ERROR_SUCCESS) {
            lstrcatA(bufferA, str_colonA);
            lstrcatW(bufferW, str_colonW);
            len = sizeof(pCC);
            ZeroMemory(pCC, sizeof(pCC));
            res = pGetDefaultCommConfigW(bufferW, pCC, &len);
            ok( res == ERROR_BADKEY,
                "returned %u with %u and %u for %s (expected '0' with "
                "ERROR_BADKEY)\n", res, GetLastError(), len, bufferA);
        }
    }

    /* an empty String is not allowed */
    len = sizeof(pCC);
    ZeroMemory(pCC, sizeof(pCC));
    SetLastError(0xdeadbeef);
    res = pGetDefaultCommConfigW(emptyW, pCC, &len);
    ok( res == ERROR_BADKEY,
        "returned %u with %u and %u for %s (expected ERROR_BADKEY)\n",
        res, GetLastError(), len, emptyA);

    /* some NULL checks */
    len = sizeof(pCC);
    ZeroMemory(pCC, sizeof(pCC));
    SetLastError(0xdeadbeef);
    res = pGetDefaultCommConfigW(NULL, pCC, &len);
    ok( res == ERROR_INVALID_PARAMETER,
        "returned %u with %u and %u for NULL (expected ERROR_INVALID_PARAMETER)\n",
        res, GetLastError(), len);


    len = sizeof(pCC);
    SetLastError(0xdeadbeef);
    res = pGetDefaultCommConfigW(com1W, NULL, &len);
    ok( res == ERROR_INVALID_PARAMETER,
        "returned %u with %u and %u (expected ERROR_INVALID_PARAMETER)\n",
        res, GetLastError(), len);


    SetLastError(0xdeadbeef);
    res = pGetDefaultCommConfigW(com1W, pCC, NULL);
    ok( res == ERROR_INVALID_PARAMETER,
        "returned %u with %u (expected ERROR_INVALID_PARAMETER)\n",
        res, GetLastError());

}

/* ################# */

START_TEST(confdlg)
{
    LPCSTR ptr;

    ptr = load_functions();
    if (ptr) {
        win_skip("got NULL with %u for %s\n", GetLastError(), ptr);
        return;
    }

    test_drvCommConfigDialogA();
    test_drvCommConfigDialogW();
    test_drvGetDefaultCommConfigA();
    test_drvGetDefaultCommConfigW();

}
