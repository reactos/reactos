/*
 * Unit test suite for MAPI utility functions
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

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "winerror.h"
#include "winnt.h"
#include "mapiutil.h"
#include "mapitags.h"
#include "mapi32_test.h"

static HMODULE hMapi32 = 0;

static SCODE (WINAPI *pScInitMapiUtil)(ULONG);
static void  (WINAPI *pDeinitMapiUtil)(void);
static void  (WINAPI *pSwapPword)(PUSHORT,ULONG);
static void  (WINAPI *pSwapPlong)(PULONG,ULONG);
static void  (WINAPI *pHexFromBin)(LPBYTE,int,LPWSTR);
static BOOL  (WINAPI *pFBinFromHex)(LPWSTR,LPBYTE);
static UINT  (WINAPI *pUFromSz)(LPCSTR);
static ULONG (WINAPI *pUlFromSzHex)(LPCSTR);
static ULONG (WINAPI *pCbOfEncoded)(LPCSTR);
static BOOL  (WINAPI *pIsBadBoundedStringPtr)(LPCSTR,ULONG);
static SCODE (WINAPI *pMAPIInitialize)(LPVOID);
static void  (WINAPI *pMAPIUninitialize)(void);

static void init_function_pointers(void)
{
    hMapi32 = LoadLibraryA("mapi32.dll");

    pScInitMapiUtil = (void*)GetProcAddress(hMapi32, "ScInitMapiUtil@4");
    pDeinitMapiUtil = (void*)GetProcAddress(hMapi32, "DeinitMapiUtil@0");
    pSwapPword = (void*)GetProcAddress(hMapi32, "SwapPword@8");
    pSwapPlong = (void*)GetProcAddress(hMapi32, "SwapPlong@8");
    pHexFromBin = (void*)GetProcAddress(hMapi32, "HexFromBin@12");
    pFBinFromHex = (void*)GetProcAddress(hMapi32, "FBinFromHex@8");
    pUFromSz = (void*)GetProcAddress(hMapi32, "UFromSz@4");
    pUlFromSzHex = (void*)GetProcAddress(hMapi32, "UlFromSzHex@4");
    pCbOfEncoded = (void*)GetProcAddress(hMapi32, "CbOfEncoded@4");
    pIsBadBoundedStringPtr = (void*)GetProcAddress(hMapi32, "IsBadBoundedStringPtr@8");
    pMAPIInitialize = (void*)GetProcAddress(hMapi32, "MAPIInitialize");
    pMAPIUninitialize = (void*)GetProcAddress(hMapi32, "MAPIUninitialize");
}

static void test_SwapPword(void)
{
    USHORT shorts[3];

    if (!pSwapPword)
    {
        win_skip("SwapPword is not available\n");
        return;
    }

    shorts[0] = 0xff01;
    shorts[1] = 0x10ff;
    shorts[2] = 0x2001;
    pSwapPword(shorts, 2);
    ok((shorts[0] == 0x01ff && shorts[1] == 0xff10 && shorts[2] == 0x2001),
       "Expected {0x01ff,0xff10,0x2001}, got {0x%04x,0x%04x,0x%04x}\n",
       shorts[0], shorts[1], shorts[2]);
}

static void test_SwapPlong(void)
{
    ULONG longs[3];

    if (!pSwapPlong)
    {
        win_skip("SwapPlong is not available\n");
        return;
    }

    longs[0] = 0xffff0001;
    longs[1] = 0x1000ffff;
    longs[2] = 0x20000001;
    pSwapPlong(longs, 2);
    ok((longs[0] == 0x0100ffff && longs[1] == 0xffff0010 && longs[2] == 0x20000001),
       "Expected {0x0100ffff,0xffff0010,0x20000001}, got {0x%08lx,0x%08lx,0x%08lx}\n",
       longs[0], longs[1], longs[2]);
}

static void test_HexFromBin(void)
{
    static char res[] =       { "000102030405060708090A0B0C0D0E0F101112131415161"
      "718191A1B1C1D1E1F202122232425262728292A2B2C2D2E2F303132333435363738393A3B"
      "3C3D3E3F404142434445464748494A4B4C4D4E4F505152535455565758595A5B5C5D5E5F6"
      "06162636465666768696A6B6C6D6E6F707172737475767778797A7B7C7D7E7F8081828384"
      "85868788898A8B8C8D8E8F909192939495969798999A9B9C9D9E9FA0A1A2A3A4A5A6A7A8A"
      "9AAABACADAEAFB0B1B2B3B4B5B6B7B8B9BABBBCBDBEBFC0C1C2C3C4C5C6C7C8C9CACBCCCD"
      "CECFD0D1D2D3D4D5D6D7D8D9DADBDCDDDEDFE0E1E2E3E4E5E6E7E8E9EAEBECEDEEEFF0F1F"
      "2F3F4F5F6F7F8F9FAFBFCFDFE\0X" };
    BYTE data[255];
    WCHAR strw[256];
    BOOL bOk;
    int i;

    if (!pHexFromBin || !pFBinFromHex)
    {
        win_skip("Hexadecimal conversion functions are not available\n");
        return;
    }

    for (i = 0; i < 255; i++)
        data[i] = i;
    memset(strw, 'X', sizeof(strw));
    pHexFromBin(data, sizeof(data), strw);

    ok(memcmp(strw, res, sizeof(res) - 1) == 0, "HexFromBin: Result differs\n");

    memset(data, 0, sizeof(data));
    pFBinFromHex((LPWSTR)res, data);
    bOk = TRUE;
    for (i = 0; i < 255; i++)
        if (data[i] != i)
            bOk = FALSE;
    ok(bOk == TRUE, "FBinFromHex: Result differs\n");
}

static void test_UFromSz(void)
{
    if (!pUFromSz)
    {
        win_skip("UFromSz is not available\n");
        return;
    }

    ok(pUFromSz("105679") == 105679u,
       "UFromSz: expected 105679, got %d\n", pUFromSz("105679"));

    ok(pUFromSz(" 4") == 0, "UFromSz: expected 0. got %d\n",
       pUFromSz(" 4"));
}

static void test_UlFromSzHex(void)
{
    if (!pUlFromSzHex)
    {
        win_skip("UlFromSzHex is not available\n");
        return;
    }

    ok(pUlFromSzHex("fF") == 0xffu,
       "UlFromSzHex: expected 0xff, got 0x%lx\n", pUlFromSzHex("fF"));

    ok(pUlFromSzHex(" c") == 0, "UlFromSzHex: expected 0x0. got 0x%lx\n",
       pUlFromSzHex(" c"));
}

static void test_CbOfEncoded(void)
{
    char buff[129];
    unsigned int i;

    if (!pCbOfEncoded)
    {
        win_skip("CbOfEncoded is not available\n");
        return;
    }

    for (i = 0; i < sizeof(buff) - 1; i++)
    {
        ULONG ulRet, ulExpected = (((i | 3) >> 2) + 1) * 3;

        memset(buff, '\0', sizeof(buff));
        memset(buff, '?', i);
        ulRet = pCbOfEncoded(buff);
        ok(ulRet == ulExpected,
           "CbOfEncoded(length %d): expected %ld, got %ld\n",
           i, ulExpected, ulRet);
    }
}

static void test_IsBadBoundedStringPtr(void)
{
    if (!pIsBadBoundedStringPtr)
    {
        win_skip("IsBadBoundedStringPtr is not available\n");
        return;
    }

    ok(pIsBadBoundedStringPtr(NULL, 0) == TRUE, "IsBadBoundedStringPtr: expected TRUE\n");
    ok(pIsBadBoundedStringPtr("TEST", 4) == TRUE, "IsBadBoundedStringPtr: expected TRUE\n");
    ok(pIsBadBoundedStringPtr("TEST", 5) == FALSE, "IsBadBoundedStringPtr: expected FALSE\n");
}

START_TEST(util)
{
    SCODE ret;

    if (!HaveDefaultMailClient())
    {
        win_skip("No default mail client installed\n");
        return;
    }

    init_function_pointers();

    if (!pScInitMapiUtil || !pDeinitMapiUtil)
    {
        win_skip("MAPI utility initialization functions are not available\n");
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

    test_SwapPword();
    test_SwapPlong();

    /* We call MAPIInitialize here for the benefit of native extended MAPI
     * providers which crash in the HexFromBin tests when MAPIInitialize has
     * not been called. Since MAPIInitialize is irrelevant for HexFromBin on
     * Wine, we do not care whether MAPIInitialize succeeds. */
    if (pMAPIInitialize)
        ret = pMAPIInitialize(NULL);
    test_HexFromBin();
    if (pMAPIUninitialize && ret == S_OK)
        pMAPIUninitialize();

    test_UFromSz();
    test_UlFromSzHex();
    test_CbOfEncoded();
    test_IsBadBoundedStringPtr();

    pDeinitMapiUtil();
    FreeLibrary(hMapi32);
}
