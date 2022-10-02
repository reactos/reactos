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

#define SANITIZER_ENABLED
#include "sanitizer.h"

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
    if (bNullOK && ptr == NULL)
        return;

    ProbeForRead(ptr, cb, 1);
}

VOID FASTCALL SanitizeWritePtr(LPVOID ptr, UINT_PTR cb, BOOL bNullOK)
{
    if (bNullOK && ptr == NULL)
        return;

    ProbeForWrite(ptr, cb, 1);
}

VOID FASTCALL SanitizeStringPtrA(LPSTR psz, BOOL bNullOK)
{
    size_t cch;

    if (bNullOK && psz == NULL)
        return;

    cch = strlen(psz) + 1;
    ProbeForWrite(psz, cch * sizeof(CHAR), 1);
}

VOID FASTCALL SanitizeStringPtrW(LPWSTR psz, BOOL bNullOK)
{
    size_t cch;

    if (bNullOK && psz == NULL)
        return;

    cch = wcslen(psz) + 1;
    ProbeForWrite(psz, cch * sizeof(WCHAR), 1);
}

VOID FASTCALL SanitizeUnicodeString(PUNICODE_STRING pustr)
{
    ASSERT(pustr != NULL);
    ASSERT(pustr->Buffer != UNINIT_POINTER);
    ASSERT(pustr->Buffer != FREED_POINTER);
    ProbeForReadUnicodeString(pustr);
    // FIXME: pustr->Buffer
}

SIZE_T FASTCALL SanitizePoolMemory(PVOID P, ULONG Tag)
{
    BOOLEAN QuotaCharged;

    if (P == NULL)
    {
        ASSERT(FALSE);
        return 0;
    }

    if (P == FREED_POINTER || P == UNINIT_POINTER)
    {
        DPRINT1("%p is bad pointer\n", P);
        ASSERT(FALSE);
        return 0;
    }

    return ExQueryPoolBlockSize(P, &QuotaCharged); // FIXME: Implement
}

#undef ExAllocatePoolWithTag

PVOID FASTCALL
SanitizeExAllocatePoolWithTag(POOL_TYPE PoolType,
                              SIZE_T NumberOfBytes,
                              ULONG Tag)
{
    PVOID ret;

    ret = ExAllocatePoolWithTag(PoolType, NumberOfBytes, Tag);
    if (ret)
        RtlFillMemory(ret, NumberOfBytes, UNINIT_BYTE);

    return ret;
}

VOID FASTCALL SanitizeBeforeExFreePool(PVOID P, ULONG TagToFree)
{
    volatile BYTE *pb = P;
    SIZE_T NumberOfBytes, cb, cbFreed;

    NumberOfBytes = SanitizePoolMemory(P, TagToFree);
    if (!NumberOfBytes)
        return;

    for (cb = cbFreed = 0; cb < NumberOfBytes; ++cb, ++pb)
    {
        if (*pb == FREED_BYTE)
            ++cbFreed;
    }

    if (cbFreed >= 4)
    {
        DPRINT1("%p is double-free suspicious\n", P);
#ifdef PUNISH_SUSPECTED
        ASSERT(FALSE);
#endif
    }

    RtlFillMemory(P, NumberOfBytes, FREED_BYTE);
}

#undef ExFreePoolWithTag

VOID FASTCALL SanitizeExFreePoolWithTag(PVOID P, ULONG TagToFree)
{
    if (P)
        SanitizeBeforeExFreePool(P, TagToFree);

    ExFreePoolWithTag(P, TagToFree);
}
