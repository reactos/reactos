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

/*
 * We wrap the pool memory blocks.
 * The 'fake' pool pointer is incompatible to the 'real' pool pointer.
 */
#define REAL2FAKE(ptr) ((PVOID)(((SIZE_T*)(ptr)) + 1))
#define FAKE2REAL(ptr) ((PVOID)(((SIZE_T*)(ptr)) - 1))
#define REAL2DATA(ptr) *((SIZE_T*)(ptr))
#define FAKE2DATA(ptr) REAL2DATA(FAKE2REAL(ptr))
#ifdef _WIN64
    #define DATA_MASK (0xF000000000000000ULL)
#else
    #define DATA_MASK (0xF0000000UL)
#endif
#define IS_FAKE(ptr) ((FAKE2DATA(ptr) & DATA_MASK) == DATA_MASK)

SIZE_T FASTCALL SanitizePoolMemory(PVOID P, ULONG Tag, BOOL bNullOK)
{
    SIZE_T Size;
    PVOID real;

    if (bNullOK && P == NULL)
        return 0;

    ASSERT(P != NULL);
    ASSERT(P != UNINIT_POINTER);
    ASSERT(P != FREED_POINTER);

    real = P;
    Size = 0;

    _SEH2_TRY
    {
        if (IS_FAKE(P))
        {
            real = FAKE2REAL(P);
            Size = (REAL2DATA(real) & DATA_MASK);
        }
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        real = P;
        Size = 0;
    }
    _SEH2_END;

    if (Size > 0)
        SanitizeReadPtr(real, Size, FALSE);

    return Size;
}

PVOID FASTCALL
SanitizeExAllocatePoolWithTag(POOL_TYPE PoolType,
                              SIZE_T NumberOfBytes,
                              ULONG Tag)
{
    PVOID real, fake;

    if (NumberOfBytes == 0)
        return NULL;

    real = ExAllocatePoolWithTag(PoolType, sizeof(SIZE_T) + NumberOfBytes, Tag);
    if (real == NULL)
        return NULL;

    REAL2DATA(real) = (NumberOfBytes | DATA_MASK);
    fake = REAL2FAKE(real);
    RtlFillMemory(fake, NumberOfBytes, UNINIT_BYTE);
    return fake;
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
    PVOID real;

    if (!P)
        return;

    SanitizeBeforeExFreePool(P, TagToFree);

    _SEH2_TRY
    {
        real = (IS_FAKE(P) ? FAKE2REAL(P) : P);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        real = P;
    }
    _SEH2_END;

    ExFreePoolWithTag(real, TagToFree);
}
