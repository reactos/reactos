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

    ASSERT(ptr != NULL);
    ASSERT(ptr != UNINIT_POINTER);
    ASSERT(ptr != FREED_POINTER);

    _SEH2_TRY
    {
        ProbeForRead(ptr, cb, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("%p, %p\n", ptr, cb);
        ASSERT(FALSE);
    }
    _SEH2_END;
}

VOID FASTCALL SanitizeWritePtr(LPVOID ptr, UINT_PTR cb, BOOL bNullOK)
{
    if (bNullOK && ptr == NULL)
        return;

    ASSERT(ptr != NULL);
    ASSERT(ptr != UNINIT_POINTER);
    ASSERT(ptr != FREED_POINTER);

    _SEH2_TRY
    {
        ProbeForWrite(ptr, cb, 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("%p, %p\n", ptr, cb);
        ASSERT(FALSE);
    }
    _SEH2_END;
}

VOID FASTCALL SanitizeStringPtrA(LPSTR psz, BOOL bNullOK)
{
    size_t cch;

    if (bNullOK && psz == NULL)
        return;

    ASSERT(psz != NULL);
    ASSERT(psz != UNINIT_POINTER);
    ASSERT(psz != FREED_POINTER);

    _SEH2_TRY
    {
        cch = strlen(psz) + 1;
        ProbeForWrite(psz, cch * sizeof(CHAR), 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("%p, %p\n", psz, cch);
        ASSERT(FALSE);
    }
    _SEH2_END;
}

VOID FASTCALL SanitizeStringPtrW(LPWSTR psz, BOOL bNullOK)
{
    size_t cch;

    if (bNullOK && psz == NULL)
        return;

    ASSERT(psz != NULL);
    ASSERT(psz != UNINIT_POINTER);
    ASSERT(psz != FREED_POINTER);

    _SEH2_TRY
    {
        cch = wcslen(psz) + 1;
        ProbeForWrite(psz, cch * sizeof(WCHAR), 1);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        DPRINT1("%p, %p\n", psz, cch);
        ASSERT(FALSE);
    }
    _SEH2_END;
}

VOID FASTCALL SanitizeUnicodeString(PUNICODE_STRING pustr, BOOL bNullOK)
{
    if (bNullOK && pustr == NULL)
        return;

    ASSERT(pustr != NULL);
    ASSERT(pustr->Buffer != UNINIT_POINTER);
    ASSERT(pustr->Buffer != FREED_POINTER);

    _SEH2_TRY
    {
        // FIXME: pustr->Buffer
        ProbeForReadUnicodeString(pustr);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        ASSERT(FALSE);
    }
    _SEH2_END;
}

SIZE_T FASTCALL SanitizePoolMemory(PVOID P, ULONG Tag, BOOL bNullOK)
{
    BOOLEAN QuotaCharged;
    SIZE_T Size;

    if (bNullOK && P == NULL)
        return 0;

    ASSERT(P != NULL);
    ASSERT(P != UNINIT_POINTER);
    ASSERT(P != FREED_POINTER);

    Size = ExQueryPoolBlockSize(P, &QuotaCharged); // FIXME: Implement
    if (Size)
        SanitizeReadPtr(P, Size, FALSE);

    return Size;
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

    NumberOfBytes = SanitizePoolMemory(P, TagToFree, TRUE);
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
