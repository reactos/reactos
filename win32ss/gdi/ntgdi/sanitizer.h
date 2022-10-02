/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Sanitizing address, memory and heap
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#ifdef SANITIZER_ENABLED
    VOID FASTCALL SanitizeReadPtr(LPCVOID ptr, UINT_PTR cb, BOOL bNullOK);
    VOID FASTCALL SanitizeWritePtr(LPVOID ptr, UINT_PTR cb, BOOL bNullOK);
    VOID FASTCALL SanitizeStringPtrA(LPSTR psz, BOOL bNullOK);
    VOID FASTCALL SanitizeStringPtrW(LPWSTR psz, BOOL bNullOK);
    VOID FASTCALL SanitizeUnicodeString(PUNICODE_STRING pustr);
    SIZE_T FASTCALL SanitizePoolMemory(PVOID P, ULONG Tag);

    PVOID FASTCALL
    SanitizeExAllocatePoolWithTag(POOL_TYPE PoolType,
                                  SIZE_T NumberOfBytes,
                                  ULONG Tag);

    VOID FASTCALL SanitizeExFreePoolWithTag(PVOID P, ULONG TagToFree);

    #define ExAllocatePoolWithTag SanitizeExAllocatePoolWithTag
    #define ExFreePoolWithTag SanitizeExFreePoolWithTag
#else
    #define SanitizeReadPtr(ptr, cb, bNullOK)
    #define SanitizeWritePtr(ptr, cb, bNullOK)
    #define SanitizeStringPtrA(psz, bNullOK)
    #define SanitizeStringPtrW(psz, bNullOK)
    #define SanitizeUnicodeString(pustr)
    #define SanitizePoolMemory(P, Tag) ((SIZE_T)0)
#endif
