/*
 * Unit test suite for MAPI IMalloc functions
 *
 * Copyright 2004 Jon Griffiths
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

#define COBJMACROS
#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winnt.h"
#include "mapiutil.h"
#include "mapi32_test.h"

static HMODULE hMapi32 = 0;

static SCODE    (WINAPI *pScInitMapiUtil)(ULONG);
static LPMALLOC (WINAPI *pMAPIGetDefaultMalloc)(void);

static void test_IMalloc(void)
{
    LPVOID lpMem;
    ULONG ulRef;
    int iRet;
    HRESULT hRet;
    LPMALLOC lpMalloc;
    LPVOID lpVoid;

    pMAPIGetDefaultMalloc = (void*)GetProcAddress(hMapi32,
                                                  "MAPIGetDefaultMalloc@0");
    if (!pMAPIGetDefaultMalloc)
    {
        win_skip("MAPIGetDefaultMalloc is not available\n");
        return;
    }

    lpMalloc = pMAPIGetDefaultMalloc();
    ok(lpMalloc != NULL, "Expected MAPIGetDefaultMalloc to return non-NULL\n");
    if (!lpMalloc)
    {
        skip("MAPIGetDefaultMalloc failed\n");
        return;
    }

    lpVoid = NULL;
    hRet = IMalloc_QueryInterface(lpMalloc, &IID_IUnknown, &lpVoid);
    ok (hRet == S_OK && lpVoid != NULL,
        "IID_IUnknown: expected S_OK, non-null, got 0x%08lx, %p\n",
        hRet, lpVoid);

    lpVoid = NULL;
    hRet = IMalloc_QueryInterface(lpMalloc, &IID_IMalloc, &lpVoid);
    ok (hRet == S_OK && lpVoid != NULL,
        "IID_IIMalloc: expected S_OK, non-null, got 0x%08lx, %p\n",
        hRet, lpVoid);

    /* Prove that native mapi uses LocalAlloc/LocalFree */
    lpMem = IMalloc_Alloc(lpMalloc, 61);
    ok (lpMem && IMalloc_GetSize(lpMalloc, lpMem) == LocalSize(lpMem),
        "Expected non-null, same size, got %p, %s size\n", lpMem,
        lpMem ? "different" : "same");

    iRet = IMalloc_DidAlloc(lpMalloc, lpMem);
    ok (iRet == -1, "DidAlloc, expected -1. got %d\n", iRet);

    IMalloc_HeapMinimize(lpMalloc);
 
    LocalFree(lpMem);
 
    ulRef = IMalloc_AddRef(lpMalloc);
    ok (ulRef == 1u, "AddRef expected 1, returned %ld\n", ulRef);
    
    ulRef = IMalloc_Release(lpMalloc);
    ok (ulRef == 1u, "AddRef expected 1, returned %ld\n", ulRef);

    IMalloc_Release(lpMalloc);
}

START_TEST(imalloc)
{
    SCODE ret;

    if (!HaveDefaultMailClient())
    {
        win_skip("No default mail client installed\n");
        return;
    }

    hMapi32 = LoadLibraryA("mapi32.dll");

    pScInitMapiUtil = (void*)GetProcAddress(hMapi32, "ScInitMapiUtil@4");
    if (!pScInitMapiUtil)
    {
        win_skip("ScInitMapiUtil is not available\n");
        FreeLibrary(hMapi32);
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pScInitMapiUtil(0);
    if ((ret != S_OK) && (GetLastError() == ERROR_PROC_NOT_FOUND))
    {
        win_skip("ScInitMapiUtil is not implemented\n");
        FreeLibrary(hMapi32);
        return;
    }
    else if ((ret == E_FAIL) && (GetLastError() == ERROR_INVALID_HANDLE))
    {
        win_skip("ScInitMapiUtil doesn't work on some Win98 and WinME systems\n");
        FreeLibrary(hMapi32);
        return;
    }

    test_IMalloc();

    FreeLibrary(hMapi32);
}
