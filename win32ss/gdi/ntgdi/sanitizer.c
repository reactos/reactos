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

/* #define PUNISH_SUSPECTED */

#define UNINIT_BYTE 0xDD
#define FREED_BYTE 0xEE

#ifdef _WIN64
    #define UNINIT_POINTER ((PVOID)(UINT_PTR)0xDDDDDDDDDDDDDDDD)
    #define FREED_POINTER ((PVOID)(UINT_PTR)0xEEEEEEEEEEEEEEEE)
#else
    #define UNINIT_POINTER ((PVOID)(UINT_PTR)0xDDDDDDDD)
    #define FREED_POINTER ((PVOID)(UINT_PTR)0xEEEEEEEE)
#endif

VOID FASTCALL SanitizeReadPtr(LPCVOID lp, UINT_PTR ucb, BOOL bCanBeNull)
{
    volatile const BYTE *pb;

    if (bCanBeNull && !lp)
        return;

    for (pb = lp; ucb-- > 0; ++pb)
    {
        if (*pb != *pb)
        {
            pb = NULL;
            *pb = 0;
            break;
        }
    }
}

VOID FASTCALL SanitizeWritePtr(LPVOID lp, UINT_PTR ucb, BOOL bCanBeNull)
{
    volatile const BYTE *pb;

    if (bCanBeNull && !lp)
        return;

    for (pb = lp; ucb-- > 0; ++pb)
    {
        *pb = *pb;
    }
}

VOID FASTCALL SanitizeStringPtrA(LPSTR lpsz, BOOL bCanBeNull)
{
    volatile const CHAR *pch;

    if (bCanBeNull && !lpsz)
        return;

    for (pch = lpsz; *pch; ++pch)
    {
        *pch = *pch;
    }
    *pch = *pch;
}

VOID FASTCALL SanitizeStringPtrW(LPWSTR lpsz, BOOL bCanBeNull)
{
    volatile const WCHAR *pch;

    if (bCanBeNull && !lpsz)
        return;

    for (pch = lpsz; *pch; ++pch)
    {
        *pch = *pch;
    }
    *pch = *pch;
}

VOID FASTCALL SanitizeHeapSystem(VOID)
{
    // FIXME
}

SIZE_T FASTCALL SanitizeHeapMemory(PVOID P, ULONG Tag)
{
    if (!P)
        return 0;

    if (P == FREED_POINTER || P == UNINIT_POINTER)
    {
        ASSERT(0);
        return 0;
    }

    // FIXME
    return 0;
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
        RtlFillMemory(ret, NumberOfBytes, UNINIT_BYTE);

    SanitizeHeapSystem();

    return ret;
}

static VOID FASTCALL SanitizeBeforeExFreePool(PVOID P, SIZE_T NumberOfBytes, ULONG TagToFree)
{
    volatile BYTE *pb = P;
    SIZE_T NumberOfBytes, cb, cbSuspicous;

    NumberOfBytes = SanitizeHeapMemory(P, TagToFree);
    if (!NumberOfBytes)
        return;

    for (cb = cbSuspicous = 0; cb < NumberOfBytes; ++cb, ++pb)
    {
        if (*pb == FREED_BYTE)
            ++cbSuspicous;
    }

    if (cbSuspicous >= 4)
    {
        DPRINT1("%p is double-free suspicous\n", P);
#ifdef PUNISH_SUSPECTED
        ASSERT(0);
#endif
    }

    RtlFillMemory(P, NumberOfBytes, FREED_BYTE);
}

VOID FASTCALL ExFreePoolWithTagSanitize(PVOID P, ULONG TagToFree)
{
    SanitizeHeapSystem();

    if (P)
        SanitizeBeforeExFreePool(P, TagToFree);

    ExFreePoolWithTag(P, TagToFree);

    SanitizeHeapSystem();
}
