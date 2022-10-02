/*
 * PROJECT:         ReactOS win32 kernel mode subsystem
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            win32ss/gdi/ntgdi/sanitizer.c
 * PURPOSE:         Address/heap sanitizer
 * PROGRAMMERS:     Copyright 2022 Katayama Hirofumi MZ.
 */

/** Includes ******************************************************************/

#include <win32k.h>
#include <debug.h>
#include "sanitizer.h"

/* #define DO_HEAVY_CHECK */
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

/* FUNCTIONS ******************************************************************/

VOID FASTCALL SanitizeReadPtr(LPCVOID ptr, UINT_PTR cb, BOOL bNullOK)
{
    volatile const BYTE *pb;

    if (bNullOK && !ptr)
        return;

    for (pb = ptr; cb-- > 0; ++pb)
    {
        if (*pb != *pb)
        {
            pb = NULL;
            *pb = 0;
            break;
        }
    }
}

VOID FASTCALL SanitizeWritePtr(LPVOID ptr, UINT_PTR cb, BOOL bNullOK)
{
    volatile const BYTE *pb;

    if (bNullOK && !ptr)
        return;

    for (pb = ptr; cb-- > 0; ++pb)
    {
        *pb = *pb;
    }
}

VOID FASTCALL SanitizeStringPtrA(LPSTR psz, BOOL bNullOK)
{
    volatile const CHAR *pch;

    if (bNullOK && !psz)
        return;

    for (pch = psz; *pch; ++pch)
    {
        *pch = *pch;
    }
    *pch = *pch;
}

VOID FASTCALL SanitizeStringPtrW(LPWSTR psz, BOOL bNullOK)
{
    volatile const WCHAR *pch;

    if (bNullOK && !psz)
        return;

    for (pch = psz; *pch; ++pch)
    {
        *pch = *pch;
    }
    *pch = *pch;
}

VOID FASTCALL SanitizeUnicodeString(PUNICODE_STRING pustr)
{
    // FIXME
}

VOID FASTCALL SanitizeHeapSystem(VOID)
{
    // FIXME
}

SIZE_T FASTCALL SanitizePoolMemory(PVOID P, ULONG Tag)
{
    if (!P)
        return 0;

    if (P == FREED_POINTER || P == UNINIT_POINTER)
    {
        DPRINT1("%p is bad pointer\n", P);
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

#ifdef DO_HEAVY_CHECK
    SanitizeHeapSystem();
#endif

    ret = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
    if (ret)
        RtlFillMemory(ret, NumberOfBytes, UNINIT_BYTE);

#ifdef DO_HEAVY_CHECK
    SanitizeHeapSystem();
#endif

    return ret;
}

static VOID FASTCALL SanitizeBeforeExFreePool(PVOID P, SIZE_T NumberOfBytes, ULONG TagToFree)
{
    volatile BYTE *pb = P;
    SIZE_T NumberOfBytes, cb, cbSuspicous;

    NumberOfBytes = SanitizePoolMemory(P, TagToFree);
    if (!NumberOfBytes)
        return;

    for (cb = cbSuspicous = 0; cb < NumberOfBytes; ++cb, ++pb)
    {
        if (*pb == FREED_BYTE)
            ++cbSuspicous;
    }

    if (cbSuspicous >= 4)
    {
        DPRINT1("%p is double-free suspicious\n", P);
#ifdef PUNISH_SUSPECTED
        ASSERT(0);
#endif
    }

    RtlFillMemory(P, NumberOfBytes, FREED_BYTE);
}

VOID FASTCALL ExFreePoolWithTagSanitize(PVOID P, ULONG TagToFree)
{
#ifdef DO_HEAVY_CHECK
    SanitizeHeapSystem();
#endif

    if (P)
        SanitizeBeforeExFreePool(P, TagToFree);

    ExFreePoolWithTag(P, TagToFree);

#ifdef DO_HEAVY_CHECK
    SanitizeHeapSystem();
#endif
}
