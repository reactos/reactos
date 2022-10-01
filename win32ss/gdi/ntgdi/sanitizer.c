/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/sanitizer.c
 * PURPOSE:         Address/heap sanitizer
 * PROGRAMMERS:     Copyright 2022 Katayama Hirofumi MZ.
 */

/** Includes ******************************************************************/

#include <win32k.h>

/* FUNCTIONS ******************************************************************/

DWORD FASTCALL SanitizeReadPtr(LPCVOID lp, UINT_PTR ucb, BOOL bCanBeNull)
{
    volatile const BYTE *pb;
    DWORD ret;

    if (bCanBeNull && !lp)
        return 0;

    pb = lp;
    ret = 0;
    while (ucb-- > 0)
    {
        ret += *pb;
        ++pb;
    }
    return ret;
}

VOID FASTCALL SanitizeWritePtr(LPVOID lp, UINT_PTR ucb, BOOL bCanBeNull)
{
    volatile const BYTE *pb;

    if (bCanBeNull && !lp)
        return 0;

    pb = lp;
    while (ucb-- > 0)
    {
        *pb = *pb;
        ++pb;
    }
}

VOID FASTCALL SanitizeStringPtrA(LPSTR lpsz, BOOL bCanBeNull)
{
    volatile const CHAR *pch = lpsz;
    if (bCanBeNull && !lpsz)
        return;
    while (*pch)
    {
        *pch = *pch;
        ++pch;
    }
}

VOID FASTCALL SanitizeStringPtrW(LPWSTR lpsz, BOOL bCanBeNull)
{
    volatile const WCHAR *pch = lpsz;
    if (bCanBeNull && !lpsz)
        return;
    while (*pch)
    {
        *pch = *pch;
        ++pch;
    }
}

VOID FASTCALL SanitizeHeapSystem(VOID)
{
    // FIXME
}

VOID FASTCALL SanitizeHeapMemory(PVOID P, POOL_TYPE PoolType, ULONG Tag)
{
    // FIXME
}

SIZE_T FASTCALL SanitizeGetExPoolSize(LPCVOID pv)
{
    return 0; // FIXME
}

PVOID FASTCALL
ExAllocatePoolWithTagSanitize(POOL_TYPE PoolType,
                              SIZE_T NumberOfBytes,
                              ULONG Tag)
{
    PVOID ret;

    SanitizeHeapSystem();

    ret = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
    if (ret)
    {
        memset(ret, 0xDD, NumberOfBytes);
    }

    SanitizeHeapSystem();

    return ret;
}

VOID FASTCALL
ExFreePoolWithTagSanitize(PVOID P, ULONG TagToFree)
{
    SIZE_T NumberOfBytes;

    SanitizeHeapSystem();

    if (P)
    {
        NumberOfBytes = SanitizeGetExPoolSize(P);
        memset(P, 0xEE, NumberOfBytes);
    }

    ExFreePoolWithTag(P, TagToFree);

    SanitizeHeapSystem();
}
