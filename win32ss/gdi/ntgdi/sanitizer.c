/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Sanitizing address, memory and heap
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

/** Includes ******************************************************************/

#include <win32k.h>
#include <debug.h>

#define SANITIZER_ENABLED
#include "sanitizer.h"

/* #define PUNISH_SUSPECTED */

#undef ExAllocatePoolWithTag
#undef ExFreePoolWithTag

/* FUNCTIONS ******************************************************************/

VOID FASTCALL SanitizeReadPtr(LPCVOID ptr, UINT_PTR cb, BOOL bNullOK)
{
    if (bNullOK && ptr == NULL)
        return;

    _SEH2_TRY
    {
        ProbeForRead(ptr, cb, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ASSERT(FALSE);
    }
    _SEH2_END;
}

VOID FASTCALL SanitizeWritePtr(LPVOID ptr, UINT_PTR cb, BOOL bNullOK)
{
    if (bNullOK && ptr == NULL)
        return;

    _SEH2_TRY
    {
        ProbeForWrite(ptr, cb, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ASSERT(FALSE);
    }
    _SEH2_END;
}

VOID FASTCALL SanitizeStringPtrA(LPSTR psz, BOOL bNullOK)
{
    size_t cch;

    if (bNullOK && psz == NULL)
        return;

    _SEH2_TRY
    {
        cch = strlen(psz) + 1;
        ProbeForWrite(psz, cch * sizeof(CHAR), 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ASSERT(FALSE);
    }
    _SEH2_END;
}

VOID FASTCALL SanitizeStringPtrW(LPWSTR psz, BOOL bNullOK)
{
    size_t cch;

    if (bNullOK && psz == NULL)
        return;

    _SEH2_TRY
    {
        cch = wcslen(psz) + 1;
        ProbeForWrite(psz, cch * sizeof(WCHAR), 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ASSERT(FALSE);
    }
    _SEH2_END;
}

VOID FASTCALL SanitizeUnicodeString(PUNICODE_STRING pustr)
{
    _SEH2_TRY
    {
        // FIXME: pustr->Buffer
        ASSERT(pustr != NULL);
        ASSERT(pustr->Buffer != UNINIT_POINTER);
        ASSERT(pustr->Buffer != FREED_POINTER);
        ProbeForReadUnicodeString(pustr);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ASSERT(FALSE);
    }
    _SEH2_END;
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

VOID FASTCALL SanitizeExFreePoolWithTag(PVOID P, ULONG TagToFree)
{
    if (P)
        SanitizeBeforeExFreePool(P, TagToFree);

    ExFreePoolWithTag(P, TagToFree);
}
