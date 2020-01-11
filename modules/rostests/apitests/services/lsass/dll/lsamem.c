/*
 * PROJECT:     ReactOS lsass_apitest
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     memory functions
 * COPYRIGHT:   Copyright 2020 Andreas Maier (staubim@quantentunnel.de)
 */

#include <apitest.h>

#include <windef.h>

#include <../shared/dbgutil.h>

BOOL IsValidTag(
    _In_ char* tag3)
{
    if (strlen(tag3) != 3)
    {
        ok(FALSE, "tag has to be 3 chars long!\n");
        return FALSE;
    }
    return TRUE;
}

/**
 * @brief Fast internal function to free memory
 *        No checks are done.
 * @param LPVOID
 * @param tag3
 * @return
 */
BOOL HeapIsTagFast(
    _In_ LPVOID lpMemTagPtr,
    _In_ char* tag3)
{
    if (memcmp(tag3, lpMemTagPtr, 3) != 0)
    {
        ok(FALSE, "Mem tag wrong, expected %s!\n", tag3);
        return FALSE;
    }
    return TRUE;
}

LPVOID HeapAllocTag(
    _In_ HANDLE hHeap,
    _In_ DWORD  dwFlags,
    _In_ SIZE_T dwBytes,
    _In_ char* tag3)
{
    PVOID res;

    if (!IsValidTag(tag3))
        return NULL;

    res = HeapAlloc(hHeap, dwFlags, dwBytes + 3);
    //trace("HeapAllocTag %.*s 0x%p ", 3, tag3, res);
    memcpy(res, tag3, 3);
    res = (PBYTE)res + 3;
    //trace("-> 0x%p\n", res);
    return res;
}

BOOL HeapFreeTag(
    _In_ HANDLE hHeap,
    _In_ DWORD  dwFlags,
    _In_ LPVOID lpMem,
    _In_ char* tag3)
{
    LPVOID lpMemOrig = lpMem;

    if (!IsValidTag(tag3))
        return FALSE;

    lpMem = (PBYTE)lpMem - 3;

    if (!HeapValidate(hHeap, 0, lpMem))
    {
        trace("mem block at 0x%p is not from heap 0x%p!\n", lpMemOrig, hHeap);

        // is the original pointer from our heap (without tag?)
        if (HeapValidate(hHeap, 0, lpMemOrig))
        {
            trace("mem block at 0x%p (without tag) is from heap 0x%p!\n", lpMem, hHeap);
            return HeapFree(hHeap, dwFlags, lpMem);
        }
        else
        {
            HANDLE hHeapAry[20];
            DWORD HeapCount;
            int i;

            HeapCount = GetProcessHeaps(ARRAY_SIZE(hHeapAry), hHeapAry);

            trace("we have %li process heaps!\n", HeapCount);
            for (i = 0; i < HeapCount; i++)
            {
                // only for interesst - is it from ProcessHeap?
                //HANDLE hPsHeap = GetProcessHeap();
                trace("(%i)  0x%p\n", i, hHeapAry[i]);
                if (HeapValidate(hHeapAry[i], 0, lpMemOrig))
                    trace("   ^^ its from this heap!!!\n");
            }
            return FALSE;
        }

        return FALSE;
    }

    if (!HeapIsTagFast(lpMem, tag3))
        return FALSE;

    return HeapFree(hHeap, dwFlags, lpMem);
}

BOOL HeapIsTag(
    _In_ LPVOID lpMem,
    _In_ char* tag3)
{
    if (!IsValidTag(tag3))
        return FALSE;

    lpMem = (PBYTE)lpMem - 3;

    return HeapIsTagFast(lpMem, tag3);
}
