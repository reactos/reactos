/*
 * PROJECT:     ReactOS Kernel
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     Sanitizing address, memory and heap
 * COPYRIGHT:   Copyright 2022 Katayama Hirofumi MZ <katayama.hirofumi.mz@gmail.com>
 */

#pragma once

#define UNINIT_BYTE 0xDD
#define FREED_BYTE 0xEE

#ifdef _WIN64
    #define UNINIT_POINTER ((PVOID)(UINT_PTR)0xDDDDDDDDDDDDDDDD)
    #define FREED_POINTER ((PVOID)(UINT_PTR)0xEEEEEEEEEEEEEEEE)
#else
    #define UNINIT_POINTER ((PVOID)(UINT_PTR)0xDDDDDDDD)
    #define FREED_POINTER ((PVOID)(UINT_PTR)0xEEEEEEEE)
#endif

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
