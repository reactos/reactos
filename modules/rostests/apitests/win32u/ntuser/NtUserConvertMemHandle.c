/*
 * PROJECT:     ReactOS api tests
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Test for NtUserConvertMemHandle
 * COPYRIGHT:   Copyright 2025 Max Korostil (mrmks04@yandex.ru)
 */

#include "../win32nt.h"


HGLOBAL createGlobalMemory(const CHAR* pString, DWORD stringLength)
{
    HGLOBAL hGlobalBuffer = NULL;
    CHAR* pLockedBuffer = NULL;

    hGlobalBuffer = GlobalAlloc(GMEM_DDESHARE, stringLength);
    if (hGlobalBuffer == NULL)
    {
        return hGlobalBuffer;
    }

    pLockedBuffer = (CHAR*)GlobalLock(hGlobalBuffer);
    if (pLockedBuffer)
    {
        memcpy(pLockedBuffer, pString, stringLength);
    }
    
    GlobalUnlock(hGlobalBuffer);
    
    return hGlobalBuffer;
}

HANDLE setClipboardData(UINT uFormat, HANDLE hGlobalMem)
{
    DWORD dwSize = 0;
    PVOID pMem = NULL;
    HANDLE hRet = NULL;
    HANDLE hMem = NULL;
    SETCLIPBDATA scd = {FALSE, FALSE};
   
    // Get global memory
    pMem = GlobalLock(hGlobalMem);
    dwSize = GlobalSize(hGlobalMem);
 
    hMem = NtUserConvertMemHandle(pMem, dwSize);
    GlobalUnlock(hGlobalMem);
    if (hMem == NULL)
    {
        return hMem;
    }

    scd.fGlobalHandle = TRUE;
    hRet = NtUserSetClipboardData(uFormat, hMem, &scd);

    return hRet;
}

HANDLE getClipboardData(UINT uFormat)
{
    HANDLE hData = NULL;
    HANDLE hGlobal = NULL;
    PVOID pData = NULL;
    DWORD cbData = 0;
    GETCLIPBDATA gcd;

    hData = NtUserGetClipboardData(uFormat, &gcd);
    if (gcd.fGlobalHandle)
    {
        NtUserCreateLocalMemHandle(hData, NULL, 0, &cbData);
        hGlobal = GlobalAlloc(GMEM_DDESHARE | GMEM_MOVEABLE, cbData);

        if (hGlobal == NULL)
        {
            return hGlobal;
        }

        pData = GlobalLock(hGlobal);
        NtUserCreateLocalMemHandle(hData, pData, cbData, NULL);
        GlobalUnlock(hGlobal);
    }

    return hGlobal;
}

START_TEST(NtUserConvertMemHandle)
{
    HANDLE hMem;
    CONST CHAR testString[] = "Test string";
    HGLOBAL  hGlobal = NULL;
    
    hMem = NtUserConvertMemHandle((PVOID)(UINT_PTR)0xDEADBEEF, 0xFFFF);
    ok_hdl(hMem, NULL);

    // Alloc global memory
    hGlobal = createGlobalMemory(testString, sizeof(testString));
    if (hGlobal == NULL)
    {
        skip("hGlobal is NULL\n");
    }
    else
    {
        HANDLE hMem = NULL;
        HANDLE hRet = NULL;

        OpenClipboard(NULL);
        hRet = setClipboardData(CF_TEXT, hGlobal);
        CloseClipboard();

        if (hRet == NULL)
        {
            skip("Set clipboard data failed\n");
            goto cleanup;
        }

        OpenClipboard(NULL);
        hMem = getClipboardData(CF_TEXT);
        CloseClipboard();

        if (hMem)
        {
            PVOID pData = GlobalLock(hMem);
            ok_long(memcmp(pData, testString, sizeof(testString)), 0);
            GlobalUnlock(hMem);
        }
        else
        {
            skip("Get clipboard data failed\n");
        }

        cleanup:
        GlobalFree(hGlobal);
    }
}
