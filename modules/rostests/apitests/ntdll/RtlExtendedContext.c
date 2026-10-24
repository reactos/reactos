/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for RTL extended context functions
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"
#include <XStateHelpers.h>

// Windows 7, see https://ntdoc.m417z.com/rtlcopyextendedcontext
typedef
NTSTATUS
NTAPI
FN_RtlCopyExtendedContext(
    _Out_ PCONTEXT_EX Destination,
    _In_ ULONG ContextFlags,
    _In_ const CONTEXT_EX* Source);

// Windows 7, see https://ntdoc.m417z.com/rtlgetenabledextendedfeatures
typedef
ULONG64
NTAPI
FN_RtlGetEnabledExtendedFeatures(
    _In_ ULONG64 FeatureMask);

// Windows 7, see https://ntdoc.m417z.com/rtlgetextendedcontextlength
typedef
ULONG
NTAPI
FN_RtlGetExtendedContextLength(
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength);

// Windows 10, see https://ntdoc.m417z.com/rtlgetextendedcontextlength2
typedef
ULONG
NTAPI
FN_RtlGetExtendedContextLength2(
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength,
    _In_ ULONG64 XStateCompactionMask);

// Windows 7, see https://ntdoc.m417z.com/rtlgetextendedfeaturesmask
typedef
ULONG64
NTAPI
FN_RtlGetExtendedFeaturesMask(
    _In_ const CONTEXT_EX* ContextEx);

// Windows 7, see https://ntdoc.m417z.com/rtlinitializeextendedcontext
typedef
ULONG
NTAPI
FN_RtlInitializeExtendedContext(
    _Out_ PVOID ContextBuffer,
    _In_ ULONG ContextFlags,
    _Outptr_ PCONTEXT_EX* ContextEx);

// Windows 10, see https://ntdoc.m417z.com/rtlinitializeextendedcontext2
typedef
ULONG
NTAPI
FN_RtlInitializeExtendedContext2(
    _Out_ PVOID ContextBuffer,
    _In_ ULONG ContextFlags,
    _Outptr_ PCONTEXT_EX* ContextEx,
    _In_ ULONG64 XStateCompactionMask);

// Windows 7, see https://ntdoc.m417z.com/rtllocateextendedfeature
typedef
PVOID
NTAPI
FN_RtlLocateExtendedFeature(
    _In_ const CONTEXT_EX* ContextEx,
    _In_ ULONG FeatureId,
    _Out_opt_ PULONG Length);

// Windows 10, see https://ntdoc.m417z.com/rtllocateextendedfeature2
typedef
PVOID
NTAPI
FN_RtlLocateExtendedFeature2(
    _In_ const CONTEXT_EX* ContextEx,
    _In_ ULONG FeatureId,
    _In_ PXSTATE_CONFIGURATION XState,
    _Out_opt_ PULONG Length);

// RtlLocateLegacyContext Win7
typedef
PCONTEXT
NTAPI
FN_RtlLocateLegacyContext(
    _In_ const CONTEXT_EX* ContextEx,
    _Out_opt_ PULONG Length);

// Windows 7, see https://ntdoc.m417z.com/rtlsetextendedfeaturesmask
typedef
VOID
NTAPI
FN_RtlSetExtendedFeaturesMask(
    _Inout_ PCONTEXT_EX ContextEx,
    _In_ ULONG64 FeatureMask);


FN_RtlCopyExtendedContext* pRtlCopyExtendedContext;
FN_RtlGetEnabledExtendedFeatures* pRtlGetEnabledExtendedFeatures;
FN_RtlGetExtendedContextLength* pRtlGetExtendedContextLength;
FN_RtlGetExtendedContextLength2* pRtlGetExtendedContextLength2;
FN_RtlGetExtendedFeaturesMask* pRtlGetExtendedFeaturesMask;
FN_RtlInitializeExtendedContext* pRtlInitializeExtendedContext;
FN_RtlInitializeExtendedContext2* pRtlInitializeExtendedContext2;
FN_RtlLocateExtendedFeature* pRtlLocateExtendedFeature;
FN_RtlLocateExtendedFeature2* pRtlLocateExtendedFeature2;
FN_RtlLocateLegacyContext* pRtlLocateLegacyContext;
FN_RtlSetExtendedFeaturesMask* pRtlSetExtendedFeaturesMask;

static void InitializeFunctionPointers(const char* DllName)
{
    HMODULE hNtdll = GetModuleHandleA(DllName);
    if (!pRtlCopyExtendedContext)
        pRtlCopyExtendedContext = (FN_RtlCopyExtendedContext*)GetProcAddress(hNtdll, "RtlCopyExtendedContext");
    if (!pRtlGetEnabledExtendedFeatures)
        pRtlGetEnabledExtendedFeatures = (FN_RtlGetEnabledExtendedFeatures*)GetProcAddress(hNtdll, "RtlGetEnabledExtendedFeatures");
    if (!pRtlGetExtendedContextLength)
        pRtlGetExtendedContextLength = (FN_RtlGetExtendedContextLength*)GetProcAddress(hNtdll, "RtlGetExtendedContextLength");
    if (!pRtlGetExtendedContextLength2)
        pRtlGetExtendedContextLength2 = (FN_RtlGetExtendedContextLength2*)GetProcAddress(hNtdll, "RtlGetExtendedContextLength2");
    if (!pRtlGetExtendedFeaturesMask)
        pRtlGetExtendedFeaturesMask = (FN_RtlGetExtendedFeaturesMask*)GetProcAddress(hNtdll, "RtlGetExtendedFeaturesMask");
    if (!pRtlInitializeExtendedContext)
        pRtlInitializeExtendedContext = (FN_RtlInitializeExtendedContext*)GetProcAddress(hNtdll, "RtlInitializeExtendedContext");
    if (!pRtlInitializeExtendedContext2)
        pRtlInitializeExtendedContext2 = (FN_RtlInitializeExtendedContext2*)GetProcAddress(hNtdll, "RtlInitializeExtendedContext2");
    if (!pRtlLocateExtendedFeature)
        pRtlLocateExtendedFeature = (FN_RtlLocateExtendedFeature*)GetProcAddress(hNtdll, "RtlLocateExtendedFeature");
    if (!pRtlLocateExtendedFeature2)
        pRtlLocateExtendedFeature2 = (FN_RtlLocateExtendedFeature2*)GetProcAddress(hNtdll, "RtlLocateExtendedFeature2");
    if (!pRtlLocateLegacyContext)
        pRtlLocateLegacyContext = (FN_RtlLocateLegacyContext*)GetProcAddress(hNtdll, "RtlLocateLegacyContext");
    if (!pRtlSetExtendedFeaturesMask)
        pRtlSetExtendedFeaturesMask = (FN_RtlSetExtendedFeaturesMask*)GetProcAddress(hNtdll, "RtlSetExtendedFeaturesMask");
}

static void Test_RtlGetEnabledExtendedFeatures(void)
{
    if (pRtlGetEnabledExtendedFeatures == NULL)
    {
        skip("RtlGetEnabledExtendedFeatures not found\n");
        return;
    }

    PXSTATE_CONFIGURATION XstateConfig = KUSER_SHARED_DATA_XState(SharedUserData);
    ULONG64 ExpectedFeatures = XSTATE_CONFIGURATION_EnabledFeatures(XstateConfig) |
                               XSTATE_CONFIGURATION_EnabledUserVisibleSupervisorFeatures(XstateConfig);
    ULONG64 ret;

    ret = pRtlGetEnabledExtendedFeatures(0);
    ok_eq_hex64(ret, 0);

    ret = pRtlGetEnabledExtendedFeatures(ULLONG_MAX);
    ok_eq_hex64(ret, ExpectedFeatures);
#ifdef _M_AMD64
    ok((ret & 3) == 3, "Legacy x87 and SSE features should always be enabled\n");
#elif defined(_M_IX86)
    ok((ret & 1) == 1, "Legacy x87 feature should always be enabled\n");
#endif
}

static ULONG GetExtendedContextXStateLength(ULONG64 CompactionMask)
{
    PXSTATE_CONFIGURATION XstateConfig = KUSER_SHARED_DATA_XState(SharedUserData);
    ULONG64 EnabledFeatures = XSTATE_CONFIGURATION_EnabledFeatures(XstateConfig) |
                              XSTATE_CONFIGURATION_EnabledUserVisibleSupervisorFeatures(XstateConfig);
    ULONG XSaveSize = XSTATE_CONFIGURATION_GetXSaveSize(XstateConfig, CompactionMask & EnabledFeatures);

    /* Subtract 0x200, as the legacy part is not part of the extended context */
    return XSaveSize - 0x200;
}

static ULONG GetExpectedLength(ULONG ContextFlags, ULONG64 CompactionMask)
{
    ULONG ExtendedContextLength = 0;

    /* Check architecture flags */
    ULONG Architecture = ContextFlags & (CONTEXT_i386 | CONTEXT_AMD64 | CONTEXT_ARM32 | CONTEXT_ARM64);
    if (Architecture == CONTEXT_i386)
    {
        if (ContextFlags & ~I386_CONTEXT_ALLOWED_FLAGS)
        {
            return 0;
        }

        ExtendedContextLength = TYPE_ALIGNMENT(I386_CONTEXT) - 1;
        ExtendedContextLength += sizeof(I386_CONTEXT); // Start wwith sizeof(CONTEXT)
        ExtendedContextLength += 3 * sizeof(CONTEXT_CHUNK); // This is the CONTEXT_EX without alignment padding
        if (ContextFlags & 0x40) // Check if XSTATE is requested
        {
            /* We need up to 3 * 4 bytes alignment */
            ExtendedContextLength += 3 * sizeof(ULONG);
            ExtendedContextLength += GetExtendedContextXStateLength(CompactionMask); // Add size for XSTATE
            ExtendedContextLength += 0x30; // For optional alignment of XSTATE features
        }

        /* Add bytes for optional alignment */
        return ExtendedContextLength;
    }
    else if (Architecture == CONTEXT_AMD64)
    {
        if (ContextFlags & ~AMD64_CONTEXT_ALLOWED_FLAGS)
        {
            return 0;
        }

        ExtendedContextLength = TYPE_ALIGNMENT(AMD64_CONTEXT) - 1;
        ExtendedContextLength += sizeof(AMD64_CONTEXT); // Start wwith sizeof(CONTEXT)
        ExtendedContextLength += 3 * sizeof(CONTEXT_CHUNK); // This is the CONTEXT_EX without alignment padding
        if (ContextFlags & 0x40) // Check if XSTATE is requested
        {
            /* In this case we need the alignment field from CONTEXT_EX as well */
            ExtendedContextLength += sizeof(ULONG64);
            ExtendedContextLength += GetExtendedContextXStateLength(CompactionMask); // Add size for XSTATE
            ExtendedContextLength += 0x30; // For optional alignment of XSTATE features
        }

        return ExtendedContextLength;
    }
    else if (Architecture == CONTEXT_ARM32)
    {
        if (ContextFlags & ~ARM32_CONTEXT_ALLOWED_FLAGS)
        {
            return 0;
        }

        ExtendedContextLength = TYPE_ALIGNMENT(ARM32_CONTEXT) - 1;
        ExtendedContextLength += sizeof(ARM32_CONTEXT);
        ExtendedContextLength += 3 * sizeof(CONTEXT_CHUNK); // This is the CONTEXT_EX without alignment padding
        return ExtendedContextLength;
    }
    else if (Architecture == CONTEXT_ARM64)
    {
        if (ContextFlags & ~ARM64_CONTEXT_ALLOWED_FLAGS)
        {
            return 0;
        }

        ExtendedContextLength = TYPE_ALIGNMENT(ARM64_CONTEXT) - 1;
        ExtendedContextLength += sizeof(ARM64_CONTEXT);
        ExtendedContextLength += 3 * sizeof(CONTEXT_CHUNK); // This is the CONTEXT_EX without alignment padding
        return ExtendedContextLength;
    }

    return 0;
}

static void Test_RtlGetExtendedContextLength(void)
{
    PXSTATE_CONFIGURATION XstateConfig = KUSER_SHARED_DATA_XState(SharedUserData);
    ULONG64 EnabledFeatures = XSTATE_CONFIGURATION_EnabledFeatures(XstateConfig) |
                              XSTATE_CONFIGURATION_EnabledUserVisibleSupervisorFeatures(XstateConfig);
    NTSTATUS Status, ExpectedStatus;
    ULONG Length, ExpectedLength;

    if (pRtlGetExtendedContextLength == NULL)
    {
        skip("RtlGetExtendedContextLength not found\n");
        return;
    }

    /* Valid call */
    ok_eq_hex(pRtlGetExtendedContextLength(CONTEXT_ALL, &Length), STATUS_SUCCESS);

    /* This one is an access violation */
    StartSeh()
        pRtlGetExtendedContextLength(CONTEXT_ALL, NULL);
    EndSeh(STATUS_ACCESS_VIOLATION);

    /* Check different architectures in ContextFlags */
    for (ULONG ContextFlags = 1; ContextFlags != 0; ContextFlags <<= 1)
    {
        ExpectedStatus = STATUS_INVALID_PARAMETER;
        if ((ContextFlags == CONTEXT_i386) ||
            (ContextFlags == CONTEXT_AMD64) ||
            (ContextFlags == CONTEXT_ARM32) ||
            (ContextFlags == CONTEXT_ARM64))
        {
            ExpectedStatus = STATUS_SUCCESS;
        }

        Status = pRtlGetExtendedContextLength(ContextFlags, &Length);
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedStatus, Status);
    }

    /* Test invalid architecture combinations */
    ok_eq_hex(pRtlGetExtendedContextLength(CONTEXT_i386 | CONTEXT_AMD64, &Length), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pRtlGetExtendedContextLength(CONTEXT_i386 | CONTEXT_ARM32, &Length), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pRtlGetExtendedContextLength(CONTEXT_i386 | CONTEXT_ARM64, &Length), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pRtlGetExtendedContextLength(CONTEXT_AMD64 | CONTEXT_ARM32, &Length), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pRtlGetExtendedContextLength(CONTEXT_AMD64 | CONTEXT_ARM64, &Length), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pRtlGetExtendedContextLength(CONTEXT_ARM32 | CONTEXT_ARM64, &Length), STATUS_INVALID_PARAMETER);

    ExpectedLength = GetExpectedLength(CONTEXT_i386 | 0x40, EnabledFeatures);

    /* Test allowed context flags with CONTEXT_i386 */
    for (ULONG ExtraFlags = 1; ExtraFlags != 0; ExtraFlags <<= 1)
    {
        ULONG ContextFlags = CONTEXT_i386 | ExtraFlags;
        ExpectedLength = GetExpectedLength(ContextFlags, EnabledFeatures);
        ExpectedStatus = (ExpectedLength == 0) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;

        Length = 0;
        Status = pRtlGetExtendedContextLength(ContextFlags, &Length); // here 0x10080
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedStatus, Status);
        ok(Length == ExpectedLength,
           "Unexpected Length for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedLength, Length);
    }

    /* Test allowed context flags with CONTEXT_AMD64 */
    for (ULONG ExtraFlags = 1; ExtraFlags != 0; ExtraFlags <<= 1)
    {
        ULONG ContextFlags = CONTEXT_AMD64 | ExtraFlags;
        ExpectedLength = GetExpectedLength(ContextFlags, EnabledFeatures);
        ExpectedStatus = (ExpectedLength == 0) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;

        Length = 0;
        Status = pRtlGetExtendedContextLength(ContextFlags, &Length);
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedStatus, Status);
        ok(Length == ExpectedLength,
           "Unexpected Length for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedLength, Length);
    }

    /* Test allowed context flags with CONTEXT_ARM32 */
    for (ULONG ExtraFlags = 1; ExtraFlags != 0; ExtraFlags <<= 1)
    {
        ULONG ContextFlags = CONTEXT_ARM32 | ExtraFlags;
        ExpectedLength = GetExpectedLength(ContextFlags, EnabledFeatures);
        ExpectedStatus = (ExpectedLength == 0) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;

        Length = 0;
        Status = pRtlGetExtendedContextLength(ContextFlags, &Length);
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedStatus, Status);
        ok(Length == ExpectedLength,
           "Unexpected Length for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedLength, Length);
    }

    /* Test allowed context flags with CONTEXT_ARM64 */
    for (ULONG ExtraFlags = 1; ExtraFlags != 0; ExtraFlags <<= 1)
    {
        ULONG ContextFlags = CONTEXT_ARM64 | ExtraFlags;
        ExpectedLength = GetExpectedLength(ContextFlags, EnabledFeatures);
        ExpectedStatus = (ExpectedLength == 0) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        Length = 0;
        Status = pRtlGetExtendedContextLength(ContextFlags, &Length);
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedStatus, Status);
        ok(Length == ExpectedLength,
           "Unexpected Length for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedLength, Length);
    }
}

static void Test_RtlGetExtendedContextLength2(void)
{
    NTSTATUS Status, ExpectedStatus;
    ULONG Length, ExpectedLength;

    if (pRtlGetExtendedContextLength2 == NULL)
    {
        skip("RtlGetExtendedContextLength2 not found\n");
        return;
    }

    /* Valid call */
    ok_eq_hex(pRtlGetExtendedContextLength2(CONTEXT_ALL, &Length, 0), STATUS_SUCCESS);

    /* This one is an access violation */
    StartSeh()
        pRtlGetExtendedContextLength2(CONTEXT_ALL, NULL, 0);
    EndSeh(STATUS_ACCESS_VIOLATION);

    /* Check different architectures in ContextFlags */
    for (ULONG ContextFlags = 1; ContextFlags != 0; ContextFlags <<= 1)
    {
        NTSTATUS ExpectedStatus = STATUS_INVALID_PARAMETER;
        if ((ContextFlags == CONTEXT_i386) ||
            (ContextFlags == CONTEXT_AMD64) ||
            (ContextFlags == CONTEXT_ARM32) ||
            (ContextFlags == CONTEXT_ARM64))
        {
            ExpectedStatus = STATUS_SUCCESS;
        }

        NTSTATUS Status = pRtlGetExtendedContextLength2(ContextFlags, &Length, 0);
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedStatus, Status);
    }

    /* Test invalid architecture combinations */
    ok_eq_hex(pRtlGetExtendedContextLength2(CONTEXT_i386 | CONTEXT_AMD64, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pRtlGetExtendedContextLength2(CONTEXT_i386 | CONTEXT_ARM32, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pRtlGetExtendedContextLength2(CONTEXT_i386 | CONTEXT_ARM64, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pRtlGetExtendedContextLength2(CONTEXT_AMD64 | CONTEXT_ARM32, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pRtlGetExtendedContextLength2(CONTEXT_AMD64 | CONTEXT_ARM64, &Length, 0), STATUS_INVALID_PARAMETER);
    ok_eq_hex(pRtlGetExtendedContextLength2(CONTEXT_ARM32 | CONTEXT_ARM64, &Length, 0), STATUS_INVALID_PARAMETER);

    /* Test allowed context flags with CONTEXT_i386 */
    for (ULONG ExtraFlags = 1; ExtraFlags != 0; ExtraFlags <<= 1)
    {
        ULONG ContextFlags = CONTEXT_i386 | ExtraFlags;
        ExpectedLength = GetExpectedLength(ContextFlags, ULLONG_MAX);
        ExpectedStatus = (ExpectedLength == 0) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;

        Length = 0;
        Status = pRtlGetExtendedContextLength2(ContextFlags, &Length, ULLONG_MAX);
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedStatus, Status);
        ok(Length == ExpectedLength,
           "Unexpected Length for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedLength, Length);
    }

    /* Test allowed context flags with CONTEXT_AMD64 */
    for (ULONG ExtraFlags = 1; ExtraFlags != 0; ExtraFlags <<= 1)
    {
        ULONG ContextFlags = CONTEXT_AMD64 | ExtraFlags;
        ExpectedLength = GetExpectedLength(ContextFlags, ULLONG_MAX);
        ExpectedStatus = (ExpectedLength == 0) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;

        Length = 0;
        Status = pRtlGetExtendedContextLength2(ContextFlags, &Length, ULLONG_MAX);
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedStatus, Status);
        ok(Length == ExpectedLength,
           "Unexpected Length for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedLength, Length);
    }

    /* Test allowed context flags with CONTEXT_ARM32 */
    for (ULONG ExtraFlags = 1; ExtraFlags != 0; ExtraFlags <<= 1)
    {
        ULONG ContextFlags = CONTEXT_ARM32 | ExtraFlags;
        ExpectedLength = GetExpectedLength(ContextFlags, ULLONG_MAX);
        ExpectedStatus = (ExpectedLength == 0) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;

        Length = 0;
        Status = pRtlGetExtendedContextLength2(ContextFlags, &Length, ULLONG_MAX);
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedStatus, Status);
        ok(Length == ExpectedLength,
           "Unexpected Length for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedLength, Length);
    }

    /* Test allowed context flags with CONTEXT_ARM64 */
    for (ULONG ExtraFlags = 1; ExtraFlags != 0; ExtraFlags <<= 1)
    {
        ULONG ContextFlags = CONTEXT_ARM64 | ExtraFlags;
        ExpectedLength = GetExpectedLength(ContextFlags, ULLONG_MAX);
        ExpectedStatus = (ExpectedLength == 0) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        Length = 0;
        Status = pRtlGetExtendedContextLength2(ContextFlags, &Length, ULLONG_MAX);
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedStatus, Status);
        ok(Length == ExpectedLength,
           "Unexpected Length for flags 0x%lx: expected 0x%lx, got 0x%lx\n",
           ContextFlags, ExpectedLength, Length);
    }

    /* Check status for different compaction masks */
    for (ULONG64 CompactionMask = 4; CompactionMask != 8; CompactionMask <<= 1)
    {
        /* i386 */
        ExpectedLength = GetExpectedLength(CONTEXT_i386 | 0x40, CompactionMask);
        ExpectedStatus = (ExpectedLength == 0) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        Length = 0;
        Status = pRtlGetExtendedContextLength2(CONTEXT_i386 | 0x40, &Length, CompactionMask);
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx, CompactionMask 0x%llx: expected 0x%lx, got 0x%lx\n",
           CONTEXT_i386 | 0x40, CompactionMask, ExpectedStatus, Status);
        ok(Length == ExpectedLength,
           "Unexpected Length for flags 0x%lx, CompactionMask 0x%llx: expected 0x%lx, got 0x%lx\n",
           CONTEXT_i386 | 0x40, CompactionMask, ExpectedLength, Length);

        /* AMD64 */
        ExpectedLength = GetExpectedLength(CONTEXT_AMD64 | 0x40, CompactionMask);
        ExpectedStatus = (ExpectedLength == 0) ? STATUS_INVALID_PARAMETER : STATUS_SUCCESS;
        Length = 0;
        Status = pRtlGetExtendedContextLength2(CONTEXT_AMD64 | 0x40, &Length, CompactionMask);
        ok(Status == ExpectedStatus,
           "Unexpected Status for flags 0x%lx, CompactionMask 0x%llx: expected 0x%lx, got 0x%lx\n",
           CONTEXT_AMD64 | 0x40, CompactionMask, ExpectedStatus, Status);
        ok(Length == ExpectedLength,
           "Unexpected Length for flags 0x%lx, CompactionMask 0x%llx: expected 0x%lx, got 0x%lx\n",
           CONTEXT_AMD64 | 0x40, CompactionMask, ExpectedLength, Length);

    }
}

static PVOID AllocateExtendedContextBuffer(ULONG ContextFlags)
{
    ULONG ContextLength, AllocationLength;
    NTSTATUS Status;

    if (pRtlGetExtendedContextLength == NULL)
    {
        skip("pRtlGetExtendedContextLength not found\n");
        return NULL;
    }

    Status = pRtlGetExtendedContextLength(ContextFlags, &ContextLength);
    if (!NT_SUCCESS(Status))
    {
        return NULL;
    }

    /* Add 64 bytes for alignment, and 64 bytes reserve for unaligning it later */
    AllocationLength = ContextLength + 128;

    PVOID Buffer = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, AllocationLength);
    if (Buffer == NULL)
    {
        return NULL;
    }

    RtlFillMemory(Buffer, AllocationLength, 0xBB);
    return Buffer;
}

static ULONG EqualBytes(PVOID Buffer, SIZE_T BufferSize, ULONG_PTR Offset, UCHAR Value)
{
    PUCHAR Ptr = (PUCHAR)Buffer + Offset;
    SIZE_T Remaining = BufferSize - Offset;
    ULONG Lenth = 0;

    while (Lenth < Remaining && *Ptr == Value)
    {
        Ptr++;
        Lenth++;
    }

    return Lenth;
}

static void Test_RtlInitializeExtendedContext(void)
{
    PVOID ContextBuffer;
    PCONTEXT Context;
    PCONTEXT_EX ContextEx;
    NTSTATUS Status;
    ULONG MatchingBytes;
    SIZE_T StartOffset;
    PXSAVE_AREA_HEADER XSaveAreaHeader;

    if ((pRtlGetExtendedContextLength == NULL) ||
        (pRtlInitializeExtendedContext == NULL))
    {
        skip("RtlInitializeExtendedContext not found\n");
        return;
    }

    /* Check if compaction is enabled */
    PXSTATE_CONFIGURATION XStateConfig = KUSER_SHARED_DATA_XState(SharedUserData);
    ULONG ControlFlags = XSTATE_CONFIGURATION_ControlFlags(XStateConfig);
    ULONG64 CompactionEnabledBit = (ControlFlags & 2) ? 0x8000000000000000ull : 0;

    /* Test native context flags */
    ContextBuffer = AllocateExtendedContextBuffer(CONTEXT_ALL);
    ok(ContextBuffer != NULL, "AllocateExtendedContextBuffer failed\n");
    Context = (PCONTEXT)ALIGN_UP_POINTER_BY(ContextBuffer, 64);
    Status = pRtlInitializeExtendedContext(Context, CONTEXT_ALL, &ContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)Context, sizeof(CONTEXT));
    ok_eq_hex(Context->ContextFlags, CONTEXT_ALL);
    ok_eq_hex(ContextEx->All.Offset, -(LONG)sizeof(CONTEXT));
    ok_eq_hex(ContextEx->All.Length, sizeof(CONTEXT) + 3 * sizeof(CONTEXT_CHUNK));
    ok_eq_hex(ContextEx->Legacy.Offset, -(LONG)sizeof(CONTEXT));
    ok_eq_hex(ContextEx->Legacy.Length, sizeof(CONTEXT));
    ok_eq_hex(ContextEx->XState.Offset, 25);
    ok_eq_hex(ContextEx->XState.Length, 0);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Test invalid context flags */
    Status = pRtlInitializeExtendedContext(Context, CONTEXT_i386 | CONTEXT_AMD64, &ContextEx);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

#if defined(_M_IX86) || defined(_M_AMD64)
    /* Use native context flags with XSTATE */
    ULONG XStateLength = GetExtendedContextXStateLength(ULLONG_MAX);
    ContextBuffer = AllocateExtendedContextBuffer(CONTEXT_ALL | CONTEXT_XSTATE);
    ok(ContextBuffer != NULL, "AllocateExtendedContextBuffer failed\n");
    Context = (PCONTEXT)ALIGN_UP_POINTER_BY(ContextBuffer, 64);
    Status = pRtlInitializeExtendedContext(Context, CONTEXT_ALL | CONTEXT_XSTATE, &ContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)Context, sizeof(CONTEXT));
    ok_eq_hex(Context->ContextFlags, CONTEXT_ALL | CONTEXT_XSTATE);
    ok_eq_hex(ContextEx->All.Offset, -(LONG)sizeof(CONTEXT));
    ok_eq_hex(ContextEx->All.Length, sizeof(CONTEXT) + ContextEx->XState.Offset + ContextEx->XState.Length);
    ok_eq_hex(ContextEx->Legacy.Offset, -(LONG)sizeof(CONTEXT));
    ok_eq_hex(ContextEx->Legacy.Length, sizeof(CONTEXT));
#ifdef _M_IX86
    ok_eq_hex(ContextEx->XState.Offset, sizeof(CONTEXT_EX) + 28);
#else
    ok_eq_hex(ContextEx->XState.Offset, sizeof(CONTEXT_EX) + 24);
#endif
    ok_eq_hex(ContextEx->XState.Length, XStateLength);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);
#endif

    /* Use CONTEXT_i386 */
    ContextBuffer = AllocateExtendedContextBuffer(CONTEXT_i386);
    ok(ContextBuffer != NULL, "AllocateExtendedContextBuffer failed\n");
    PI386_CONTEXT Contexti386 = (PI386_CONTEXT)ALIGN_UP_POINTER_BY(ContextBuffer, 64);
    Status = pRtlInitializeExtendedContext((PCONTEXT)Contexti386, CONTEXT_i386, &ContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)Contexti386, sizeof(I386_CONTEXT));
    ok_eq_hex(Contexti386->ContextFlags, CONTEXT_i386);
    ok_eq_hex(ContextEx->All.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(ContextEx->All.Length, sizeof(I386_CONTEXT) + + 3 * sizeof(CONTEXT_CHUNK));
    ok_eq_hex(ContextEx->Legacy.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(ContextEx->Legacy.Length, FIELD_OFFSET(I386_CONTEXT, ExtendedRegisters));
    ok_eq_hex(ContextEx->XState.Offset, 25);
    ok_eq_hex(ContextEx->XState.Length, 0);
    MatchingBytes = EqualBytes(ContextBuffer, 64, 0, 0xBB);
    ok_eq_hex(MatchingBytes, (ULONG)((ULONG_PTR)Contexti386 - (ULONG_PTR)ContextBuffer));
    MatchingBytes = EqualBytes(Contexti386, ContextEx->All.Length, sizeof(ULONG), 0xBB);
    ok_eq_hex(MatchingBytes, sizeof(I386_CONTEXT) - sizeof(ULONG));
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Use CONTEXT_i386 with extended registers */
    ContextBuffer = AllocateExtendedContextBuffer(I386_CONTEXT_EXTENDED_REGISTERS);
    ok(ContextBuffer != NULL, "AllocateExtendedContextBuffer failed\n");
    Contexti386 = (PI386_CONTEXT)ALIGN_UP_POINTER_BY(ContextBuffer, 64);
    Status = pRtlInitializeExtendedContext((PCONTEXT)Contexti386, I386_CONTEXT_EXTENDED_REGISTERS, &ContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)Contexti386, sizeof(I386_CONTEXT));
    ok_eq_hex(Contexti386->ContextFlags, I386_CONTEXT_EXTENDED_REGISTERS);
    ok_eq_hex(ContextEx->All.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(ContextEx->All.Length, sizeof(I386_CONTEXT) + + 3 * sizeof(CONTEXT_CHUNK));
    ok_eq_hex(ContextEx->Legacy.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(ContextEx->Legacy.Length, sizeof(I386_CONTEXT));
    ok_eq_hex(ContextEx->XState.Offset, 25);
    ok_eq_hex(ContextEx->XState.Length, 0);

    /* Use CONTEXT_i386 with unaligned buffer */
    PUCHAR Contexti386Unaligned = ((PUCHAR)ALIGN_UP_POINTER_BY(ContextBuffer, 64) + 3);
    Status = pRtlInitializeExtendedContext((PCONTEXT)Contexti386Unaligned, CONTEXT_i386, &ContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)Contexti386Unaligned, sizeof(I386_CONTEXT) + 4 - 3);
    Contexti386 = (PI386_CONTEXT)ALIGN_UP_POINTER_BY(Contexti386Unaligned, 4);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)Contexti386, sizeof(I386_CONTEXT));
    ok_eq_hex(Contexti386->ContextFlags, CONTEXT_i386);
    ok_eq_hex(ContextEx->All.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(ContextEx->All.Length, sizeof(I386_CONTEXT) + + 3 * sizeof(CONTEXT_CHUNK));
    ok_eq_hex(ContextEx->Legacy.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(ContextEx->Legacy.Length, FIELD_OFFSET(I386_CONTEXT, ExtendedRegisters));
    ok_eq_hex(ContextEx->XState.Offset, 25);
    ok_eq_hex(ContextEx->XState.Length, 0);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Use CONTEXT_i386 with XSTATE */
    ContextBuffer = AllocateExtendedContextBuffer(I386_CONTEXT_ALL | I386_CONTEXT_XSTATE);
    ok(ContextBuffer != NULL, "AllocateExtendedContextBuffer failed\n");
    Contexti386 = (PI386_CONTEXT)ALIGN_UP_POINTER_BY(ContextBuffer, 64);
    Status = pRtlInitializeExtendedContext((PCONTEXT)Contexti386, I386_CONTEXT_ALL | I386_CONTEXT_XSTATE, &ContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)Contexti386, sizeof(I386_CONTEXT));
    ok_eq_hex(Contexti386->ContextFlags, I386_CONTEXT_ALL | I386_CONTEXT_XSTATE);
    ok_eq_hex(ContextEx->All.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(ContextEx->All.Length, sizeof(I386_CONTEXT) + ContextEx->XState.Offset + ContextEx->XState.Length);
    ok_eq_hex(ContextEx->Legacy.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(ContextEx->Legacy.Length, sizeof(I386_CONTEXT));
    ok_eq_hex(ContextEx->XState.Offset, sizeof(CONTEXT_EX) + 28);
    ok_eq_hex(ContextEx->XState.Length, XStateLength);
    XSaveAreaHeader = (PXSAVE_AREA_HEADER)((PUCHAR)ContextEx + ContextEx->XState.Offset);
    ok_eq_hex64(XSaveAreaHeader->Mask, 0);
    ULONG64 ExpectedCompactionMask = pRtlGetEnabledExtendedFeatures(ULLONG_MAX) | CompactionEnabledBit;
    ok(XSaveAreaHeader->CompactionMask == ExpectedCompactionMask, "Unexpected CompactionMask: expected 0x%llx, got 0x%llx\n", ExpectedCompactionMask, XSaveAreaHeader->CompactionMask);
    MatchingBytes = EqualBytes(ContextBuffer, 64, 0, 0xBB);
    ok_eq_hex(MatchingBytes, (ULONG)((ULONG_PTR)Contexti386 - (ULONG_PTR)ContextBuffer));
    MatchingBytes = EqualBytes(Contexti386, ContextEx->All.Length, sizeof(ULONG), 0xBB);
    ok_eq_hex(MatchingBytes, sizeof(I386_CONTEXT) - sizeof(ULONG));
    StartOffset = sizeof(I386_CONTEXT) + 3 * sizeof(CONTEXT_CHUNK);
    MatchingBytes = EqualBytes(Contexti386, ContextEx->All.Length, StartOffset, 0xBB);
    ok_eq_hex(MatchingBytes, 0x1C); // Alignment padding before XSTATE
    StartOffset = sizeof(I386_CONTEXT) + 3 * sizeof(CONTEXT_CHUNK) + 0x1C + 2 * sizeof(ULONG64);
    MatchingBytes = EqualBytes(Contexti386, ContextEx->All.Length, StartOffset, 0x00);
    ok_eq_hex(MatchingBytes, ContextEx->XState.Length - 2 * sizeof(ULONG64));
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Use CONTEXT_AMD64 */
    ContextBuffer = AllocateExtendedContextBuffer(CONTEXT_AMD64);
    ok(ContextBuffer != NULL, "AllocateExtendedContextBuffer failed\n");
    PAMD64_CONTEXT ContextAmd64 = (PAMD64_CONTEXT)ALIGN_UP_POINTER_BY(ContextBuffer, 64);
    Status = pRtlInitializeExtendedContext((PCONTEXT)ContextAmd64, CONTEXT_AMD64, &ContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)ContextAmd64, sizeof(AMD64_CONTEXT));
    ok_eq_hex(ContextAmd64->ContextFlags, CONTEXT_AMD64);
    ok_eq_hex(ContextEx->All.Offset, -(LONG)sizeof(AMD64_CONTEXT));
    ok_eq_hex(ContextEx->All.Length, sizeof(AMD64_CONTEXT) + + 3 * sizeof(CONTEXT_CHUNK));
    ok_eq_hex(ContextEx->Legacy.Offset, -(LONG)sizeof(AMD64_CONTEXT));
    ok_eq_hex(ContextEx->Legacy.Length, sizeof(AMD64_CONTEXT));
    ok_eq_hex(ContextEx->XState.Offset, 25);
    ok_eq_hex(ContextEx->XState.Length, 0);

    /* Use CONTEXT_AMD64 with unaligned buffer */
    PUCHAR ContextAmd64Unaligned = ((PUCHAR)ALIGN_UP_POINTER_BY(ContextBuffer, 64) + 3);
    Status = pRtlInitializeExtendedContext((PCONTEXT)ContextAmd64Unaligned, CONTEXT_AMD64, &ContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)ContextAmd64Unaligned, sizeof(AMD64_CONTEXT) + 16 - 3);
    ContextAmd64 = (PAMD64_CONTEXT)ALIGN_UP_POINTER_BY(ContextAmd64Unaligned, 16);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)ContextAmd64, sizeof(AMD64_CONTEXT));
    ok_eq_hex(ContextAmd64->ContextFlags, CONTEXT_AMD64);
    ok_eq_hex(ContextEx->All.Offset, -(LONG)sizeof(AMD64_CONTEXT));
    ok_eq_hex(ContextEx->All.Length, sizeof(AMD64_CONTEXT) + + 3 * sizeof(CONTEXT_CHUNK));
    ok_eq_hex(ContextEx->Legacy.Offset, -(LONG)sizeof(AMD64_CONTEXT));
    ok_eq_hex(ContextEx->Legacy.Length, sizeof(AMD64_CONTEXT));
    ok_eq_hex(ContextEx->XState.Offset, 25);
    ok_eq_hex(ContextEx->XState.Length, 0);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Use CONTEXT_ARM32 */
    ContextBuffer = AllocateExtendedContextBuffer(CONTEXT_ARM32);
    ok(ContextBuffer != NULL, "AllocateExtendedContextBuffer failed\n");
    PARM32_CONTEXT ContextArm32 = (PARM32_CONTEXT)ALIGN_UP_POINTER_BY(ContextBuffer, 64);
    Status = pRtlInitializeExtendedContext((PCONTEXT)ContextArm32, CONTEXT_ARM32, &ContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)ContextArm32, sizeof(ARM32_CONTEXT));
    ok_eq_hex(ContextArm32->ContextFlags, CONTEXT_ARM32);
    ok_eq_hex(ContextEx->All.Offset, -(LONG)sizeof(ARM32_CONTEXT));
    ok_eq_hex(ContextEx->All.Length, sizeof(ARM32_CONTEXT) + + 3 * sizeof(CONTEXT_CHUNK));
    ok_eq_hex(ContextEx->Legacy.Offset, -(LONG)sizeof(ARM32_CONTEXT));
    ok_eq_hex(ContextEx->Legacy.Length, sizeof(ARM32_CONTEXT));
    ok_eq_hex(ContextEx->XState.Offset, 25);
    ok_eq_hex(ContextEx->XState.Length, 0);

    /* Use CONTEXT_ARM32 with unaligned buffer */
    PUCHAR ContextArm32Unaligned = ((PUCHAR)ALIGN_UP_POINTER_BY(ContextBuffer, 64) + 3);
    Status = pRtlInitializeExtendedContext((PCONTEXT)ContextArm32Unaligned, CONTEXT_ARM32, &ContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)ContextArm32Unaligned, sizeof(ARM32_CONTEXT) + 8 - 3);
    ContextArm32 = (PARM32_CONTEXT)ALIGN_UP_POINTER_BY(ContextArm32Unaligned, 8);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)ContextArm32, sizeof(ARM32_CONTEXT));
    ok_eq_hex(ContextArm32->ContextFlags, CONTEXT_ARM32);
    ok_eq_hex(ContextEx->All.Offset, -(LONG)sizeof(ARM32_CONTEXT));
    ok_eq_hex(ContextEx->All.Length, sizeof(ARM32_CONTEXT) + + 3 * sizeof(CONTEXT_CHUNK));
    ok_eq_hex(ContextEx->Legacy.Offset, -(LONG)sizeof(ARM32_CONTEXT));
    ok_eq_hex(ContextEx->Legacy.Length, sizeof(ARM32_CONTEXT));
    ok_eq_hex(ContextEx->XState.Offset, 25);
    ok_eq_hex(ContextEx->XState.Length, 0);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Use CONTEXT_ARM64 */
    ContextBuffer = AllocateExtendedContextBuffer(CONTEXT_ARM64);
    ok(ContextBuffer != NULL, "AllocateExtendedContextBuffer failed\n");
    PARM64_CONTEXT ContextArm64 = (PARM64_CONTEXT)ALIGN_UP_POINTER_BY(ContextBuffer, 64);
    Status = pRtlInitializeExtendedContext((PCONTEXT)ContextArm64, CONTEXT_ARM64, &ContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)ContextArm64, sizeof(ARM64_CONTEXT));
    ok_eq_hex(ContextArm64->ContextFlags, CONTEXT_ARM64);
    ok_eq_hex(ContextEx->All.Offset, -(LONG)sizeof(ARM64_CONTEXT));
    ok_eq_hex(ContextEx->All.Length, sizeof(ARM64_CONTEXT) + + 3 * sizeof(CONTEXT_CHUNK));
    ok_eq_hex(ContextEx->Legacy.Offset, -(LONG)sizeof(ARM64_CONTEXT));
    ok_eq_hex(ContextEx->Legacy.Length, sizeof(ARM64_CONTEXT));
    ok_eq_hex(ContextEx->XState.Offset, 25);
    ok_eq_hex(ContextEx->XState.Length, 0);

    /* Use CONTEXT_ARM64 with unaligned buffer */
    PUCHAR ContextArm64Unaligned = ((PUCHAR)ALIGN_UP_POINTER_BY(ContextBuffer, 64) + 3);
    Status = pRtlInitializeExtendedContext((PCONTEXT)ContextArm64Unaligned, CONTEXT_ARM64, &ContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)ContextArm64Unaligned, sizeof(ARM64_CONTEXT) + 16 - 3);
    ContextArm64 = (PARM64_CONTEXT)ALIGN_UP_POINTER_BY(ContextArm64Unaligned, 16);
    ok_eq_size((PUCHAR)ContextEx - (PUCHAR)ContextArm64, sizeof(ARM64_CONTEXT));
    ok_eq_hex(ContextArm64->ContextFlags, CONTEXT_ARM64);
    ok_eq_hex(ContextEx->All.Offset, -(LONG)sizeof(ARM64_CONTEXT));
    ok_eq_hex(ContextEx->All.Length, sizeof(ARM64_CONTEXT) + + 3 * sizeof(CONTEXT_CHUNK));
    ok_eq_hex(ContextEx->Legacy.Offset, -(LONG)sizeof(ARM64_CONTEXT));
    ok_eq_hex(ContextEx->Legacy.Length, sizeof(ARM64_CONTEXT));
    ok_eq_hex(ContextEx->XState.Offset, 25);
    ok_eq_hex(ContextEx->XState.Length, 0);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);
}

static void Test_RtlInitializeExtendedContext2(void)
{
    PVOID ContextBuffer;
    PCONTEXT_EX ContextEx;
    PXSAVE_AREA_HEADER XSaveAreaHeader;
    ULONG XStateLength;
    NTSTATUS Status;

    if (pRtlInitializeExtendedContext2 == NULL)
    {
        skip("RtlInitializeExtendedContext2 not found\n");
        return;
    }

    /* Get enabled features */
    ULONG64 EnabledFeatures = pRtlGetEnabledExtendedFeatures(ULLONG_MAX);

    /* Check if compaction is enabled */
    PXSTATE_CONFIGURATION XStateConfig = KUSER_SHARED_DATA_XState(SharedUserData);
    ULONG ControlFlags = XSTATE_CONFIGURATION_ControlFlags(XStateConfig);
    ULONG64 CompactionEnabledBit = (ControlFlags & 2) ? 0x8000000000000000ull : 0;

    /* Use CONTEXT_i386 with XSTATE */
    ContextBuffer = AllocateExtendedContextBuffer(I386_CONTEXT_ALL | I386_CONTEXT_XSTATE);
    Status = pRtlInitializeExtendedContext2(ContextBuffer, I386_CONTEXT_ALL | I386_CONTEXT_XSTATE, &ContextEx, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    XStateLength = GetExtendedContextXStateLength(0);
    ok_eq_hex(ContextEx->XState.Length, XStateLength);
    XSaveAreaHeader = (PXSAVE_AREA_HEADER)((PUCHAR)ContextEx + ContextEx->XState.Offset);
    ok_eq_hex64(XSaveAreaHeader->Mask, 0);
    ok_eq_hex64(XSaveAreaHeader->CompactionMask, CompactionEnabledBit);
    Status = pRtlInitializeExtendedContext2(ContextBuffer, I386_CONTEXT_ALL | I386_CONTEXT_XSTATE, &ContextEx, EnabledFeatures);
    ok_eq_hex(Status, STATUS_SUCCESS);
    XStateLength = GetExtendedContextXStateLength(EnabledFeatures);
    XSaveAreaHeader = (PXSAVE_AREA_HEADER)((PUCHAR)ContextEx + ContextEx->XState.Offset);
    ok_eq_hex64(XSaveAreaHeader->Mask, 0);
    ok_eq_hex64(XSaveAreaHeader->CompactionMask, EnabledFeatures | CompactionEnabledBit);


    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    //__debugbreak(); // !!!!

    // LocateExtendedFeature2: check XState offset outside of All

    // TODO
}

static PVOID CreateExtendedContext(ULONG ContextFlags, ULONG_PTR PtrOffset, PCONTEXT_EX* ContextExOut)
{
    PVOID ContextBuffer;
    PCONTEXT Context;
    PCONTEXT_EX ContextEx;
    NTSTATUS Status;

    if ((pRtlGetExtendedContextLength == NULL) ||
        (pRtlInitializeExtendedContext == NULL))
    {
        skip("RtlInitializeExtendedContext not found\n");
        return NULL;
    }

    ContextBuffer = AllocateExtendedContextBuffer(ContextFlags);
    if (ContextBuffer == NULL)
    {
        skip("AllocateExtendedContextBuffer failed\n");
        return NULL;
    }

    Context = (PCONTEXT)ALIGN_UP_POINTER_BY(ContextBuffer, 64);
    Context = (PCONTEXT)((PUCHAR)Context + PtrOffset);
    Status = pRtlInitializeExtendedContext(Context, ContextFlags, &ContextEx);
    if (!NT_SUCCESS(Status))
    {
        HeapFree(GetProcessHeap(), 0, ContextBuffer);
        return NULL;
    }

    *ContextExOut = ContextEx;
    return ContextBuffer;
}

static ULONG GetFeatureOffset(ULONG FeatureIndex, PULONG Length)
{
    PXSTATE_CONFIGURATION XStateConfig;
    XSTATE_FEATURE* Features;
    PULONG AllFeatures;
    ULONG64 EnabledFeatures;
    ULONG ControlFlags;
    ULONG Offset = 0x240;

    if ((FeatureIndex < 2) || (FeatureIndex >= 64))
    {
        *Length = 0;
        return 0;
    }

    XStateConfig = KUSER_SHARED_DATA_XState(SharedUserData);

    EnabledFeatures = XSTATE_CONFIGURATION_EnabledFeatures(XStateConfig) |
           XSTATE_CONFIGURATION_EnabledUserVisibleSupervisorFeatures(XStateConfig);
    if ((EnabledFeatures & (1ULL << FeatureIndex)) == 0)
    {
        *Length = 0;
        return 0;
    }

    Features = XSTATE_CONFIGURATION_Features(XStateConfig);
    AllFeatures = XSTATE_CONFIGURATION_AllFeatures(XStateConfig);

    if (AllFeatures != NULL)
        *Length = AllFeatures[FeatureIndex];
    else
        *Length = Features[FeatureIndex].Size;

    /* Check if compaction is enabled */
    ControlFlags = XSTATE_CONFIGURATION_ControlFlags(XStateConfig);
    if (ControlFlags & 2)
    {
        for (ULONG i = 2; i < FeatureIndex; i++)
        {
            if (EnabledFeatures & (1ULL << i))
            {
                Offset += AllFeatures[i];
            }
        }

        return Offset;
    }
    else
    {
        return Features[FeatureIndex].Offset;
    }
}

static void Test_RtlLocateExtendedFeature(void)
{
    ULONG64 EnabledFeatures = 0;
    PVOID ContextBuffer;
    PVOID XStateBase;
    PCONTEXT_EX ContextEx;
    PVOID Feature;
    ULONG FeatureOffset;
    ULONG Length, ExpectedLength;

    if (pRtlLocateExtendedFeature == NULL)
    {
        skip("RtlLocateExtendedFeature not found\n");
        return;
    }

    EnabledFeatures = pRtlGetEnabledExtendedFeatures(ULLONG_MAX);
    trace("Enabled features: 0x%llx\n", EnabledFeatures);

    ContextBuffer = CreateExtendedContext(CONTEXT_ALL, 0, &ContextEx);
    for (ULONG i = 0; i < 256; i++)
    {
        Length = 0xdeadbeef;
        Feature = pRtlLocateExtendedFeature(ContextEx, i, &Length);
        ok_eq_pointer(Feature, NULL);
        ok_eq_hex(Length, 0xdeadbeef);
    }
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Test CONTEXT_i386 with XSTATE */
    ContextBuffer = CreateExtendedContext(I386_CONTEXT_ALL | I386_CONTEXT_XSTATE, 0, &ContextEx);
    XStateBase = (PUCHAR)ContextEx + ContextEx->XState.Offset;
    Feature = pRtlLocateExtendedFeature(ContextEx, 0, NULL);
    ok_eq_pointer(Feature, NULL);
    Feature = pRtlLocateExtendedFeature(ContextEx, 1, NULL);
    ok_eq_pointer(Feature, NULL);
    ok_eq_hex(Length, 0xdeadbeef);
    for (ULONG i = 2; i < 64; i++)
    {
        Length = 0xdeadbeef;
        Feature = pRtlLocateExtendedFeature(ContextEx, i, &Length);
        if ((1ULL << i) & EnabledFeatures)
        {
            FeatureOffset = GetFeatureOffset(i, &ExpectedLength);
            PVOID ExpectedAddress = (PUCHAR)XStateBase + FeatureOffset - 0x200;
            ok(Feature == ExpectedAddress, "Expected feature %lu to be at 0x%p, got 0x%p\n", i, ExpectedAddress, Feature);
            ok(Length == ExpectedLength, "Expected feature %lu to have length %lu, got %lu\n", i, ExpectedLength, Length);
        }
        else
        {
            ok(Feature == NULL, "Expected feature %lu not to be present\n", i);
            ok(Length == 0xdeadbeef, "Expected feature %lu to have length 0 when not present, got %lu\n", i, Length);
        }
    }
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Test CONTEXT_AMD64 with XSTATE */
    ContextBuffer = CreateExtendedContext(AMD64_CONTEXT_XSTATE, 0, &ContextEx);
    XStateBase = (PUCHAR)ContextEx + ContextEx->XState.Offset;
    Feature = pRtlLocateExtendedFeature(ContextEx, 0, NULL);
    ok_eq_pointer(Feature, NULL);
    Feature = pRtlLocateExtendedFeature(ContextEx, 1, NULL);
    ok_eq_pointer(Feature, NULL);
    for (ULONG i = 2; i < 64; i++)
    {
        Length = 0xdeadbeef;
        Feature = pRtlLocateExtendedFeature(ContextEx, i, &Length);
        if ((1ULL << i) & EnabledFeatures)
        {
            FeatureOffset = GetFeatureOffset(i, &ExpectedLength);
            PVOID ExpectedAddress = (PUCHAR)XStateBase + FeatureOffset - 0x200;
            ok(Feature == ExpectedAddress, "Expected feature %lu to be at 0x%p, got 0x%p\n", i, ExpectedAddress, Feature);
            ok(Length == ExpectedLength, "Expected feature %lu to have length %lu, got %lu\n", i, ExpectedLength, Length);
        }
        else
        {
            ok(Feature == NULL, "Expected feature %lu not to be present\n", i);
            ok(Length == 0xdeadbeef, "Expected feature %lu to have length 0 when not present, got %lu\n", i, Length);
        }
    }

    HeapFree(GetProcessHeap(), 0, ContextBuffer);
}

static void Test_RtlLocateExtendedFeature2(void)
{
    PXSTATE_CONFIGURATION XStateConfig = KUSER_SHARED_DATA_XState(SharedUserData);
    PXSTATE_CONFIGURATION NewXStateConfig;
    ULONG64 EnabledFeatures = 0;
    PVOID ContextBuffer;
    PXSAVE_AREA_HEADER XSaveAreaHeader;
    PCONTEXT_EX ContextEx;
    PVOID Feature;
    ULONG FeatureOffset;
    ULONG Length, ExpectedLength;

    if (pRtlLocateExtendedFeature2 == NULL)
    {
        skip("RtlLocateExtendedFeature2 not found\n");
        return;
    }

    EnabledFeatures = pRtlGetEnabledExtendedFeatures(ULLONG_MAX);
    if ((EnabledFeatures & ~3) == 0)
    {
        skip("No extended features enabled, skipping test\n");
        return;
    }

    /* Find the lowest enabled feature */
    ULONG LowestFeatureIndex = 0;
    for (ULONG i = 2; i < 64; i++)
    {
        if ((1ULL << i) & EnabledFeatures)
        {
            LowestFeatureIndex = i;
            break;
        }
    }

    /* Make a copy of the XSTATE configuration */
    SIZE_T MaxSize = (ULONG_PTR)SharedUserData + PAGE_SIZE - (ULONG_PTR)XStateConfig;
    NewXStateConfig = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, MaxSize);
    RtlCopyMemory(NewXStateConfig, XStateConfig, MaxSize);

    /* Check parameters */
    Feature = pRtlLocateExtendedFeature2(NULL, 0, NULL, NULL);
    ok_eq_pointer(Feature, NULL);
    StartSeh()
        Feature = pRtlLocateExtendedFeature2(NULL, 2, NULL, NULL); 
    EndSeh(STATUS_ACCESS_VIOLATION);
    StartSeh()
        Feature = pRtlLocateExtendedFeature2(NULL, 2, XStateConfig, NULL); 
    EndSeh(STATUS_ACCESS_VIOLATION);
    //__debugbreak();
    StartSeh()
        Feature = pRtlLocateExtendedFeature2(NULL, 8, XStateConfig, NULL); 
    EndSeh(STATUS_SUCCESS);
    ok_eq_pointer(Feature, NULL);

    /* Test CONTEXT_i386 with XSTATE */
    ContextBuffer = CreateExtendedContext(I386_CONTEXT_ALL | I386_CONTEXT_XSTATE, 0, &ContextEx);
    XSaveAreaHeader = (PXSAVE_AREA_HEADER)((PUCHAR)ContextEx + ContextEx->XState.Offset);

    StartSeh()
        Feature = pRtlLocateExtendedFeature2(ContextEx, 0, NULL, NULL); 
        Feature = pRtlLocateExtendedFeature2(ContextEx, 1, NULL, NULL); 
    EndSeh(STATUS_SUCCESS);
    StartSeh()
        Feature = pRtlLocateExtendedFeature2(ContextEx, XSTATE_AVX, NULL, &Length); 
    EndSeh(STATUS_ACCESS_VIOLATION);

    for (ULONG i = 2; i < 64; i++)
    {
        Length = 0xdeadbeef;
        Feature = pRtlLocateExtendedFeature2(ContextEx, i, XStateConfig, &Length);
        if ((1ULL << i) & EnabledFeatures)
        {
            FeatureOffset = GetFeatureOffset(i, &ExpectedLength);
            PVOID ExpectedAddress = (PUCHAR)XSaveAreaHeader + FeatureOffset - 0x200;
            ok(Feature == ExpectedAddress, "Expected feature %lu to be at 0x%p, got 0x%p\n", i, ExpectedAddress, Feature);
            ok(Length == ExpectedLength, "Expected feature %lu to have length %lu, got %lu\n", i, ExpectedLength, Length);
        }
        else
        {
            ok(Feature == NULL, "Expected feature %lu not to be present\n", i);
            ok(Length == 0xdeadbeef, "Expected feature %lu to have length 0 when not present, got %lu\n", i, Length);
        }
    }

    /* Check if it is present */
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature != NULL, "Expected lowest enabled feature %lu to be present\n", LowestFeatureIndex);

    /* Disable compaction in the XSTATE configuration */
    NewXStateConfig->CompactionEnabled = 0;
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature != NULL, "Expected Feature != NULL with compaction disabled\n");

    /* Set CompactionMask to 0 */
    XSaveAreaHeader->CompactionMask = 0;
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature != NULL, "Expected lowest enabled feature %lu to be present\n", LowestFeatureIndex);

    /* Disable the lowest enabled feature in the XSTATE configuration */
    NewXStateConfig->EnabledFeatures &= ~(1ULL << LowestFeatureIndex);
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature == NULL, "Expected lowest enabled feature %lu to be absent\n", LowestFeatureIndex);
#if 0 // FIXME: This only works on Windows 10
    NewXStateConfig->EnabledUserVisibleSupervisorFeatures |= (1ULL << LowestFeatureIndex);
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature != NULL, "Expected lowest enabled feature %lu to be present\n", LowestFeatureIndex);
    NewXStateConfig->EnabledUserVisibleSupervisorFeatures &= ~(1ULL << LowestFeatureIndex);
#endif
    NewXStateConfig->EnabledFeatures |= (1ULL << LowestFeatureIndex);

    /* Enable compaction in the XSTATE configuration */
    NewXStateConfig->CompactionEnabled = 1;
    XSaveAreaHeader->CompactionMask |= (1ULL << LowestFeatureIndex);
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature != NULL, "Expected Feature != NULL with compaction enabled\n");

    /* Disable the lowest enabled feature in the XSTATE configuration */
    NewXStateConfig->EnabledFeatures &= ~(1ULL << LowestFeatureIndex);
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature == NULL, "Expected Feature == NULL when feature is disabled in XSTATE\n");
    NewXStateConfig->EnabledFeatures |= (1ULL << LowestFeatureIndex);

    /* Disable the compaction in the XSAVE area header */
    XSaveAreaHeader->CompactionMask &= ~(1ULL << LowestFeatureIndex);
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature == NULL, "Expected Feature == NULL when feature is not in XSave area CompactionMask\n");
    XSaveAreaHeader->CompactionMask |= (1ULL << LowestFeatureIndex);

    /* Test with modified XState chunk boundaries */
    ContextEx->XState.Length = 0;
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature != NULL, "Expected Feature != NULL for XState.Length = 0\n");
    ULONG OriginalXStateOffset = ContextEx->XState.Offset;
    ContextEx->XState.Offset = OriginalXStateOffset + 64;
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature == NULL, "Expected Feature == NULL for XState.Offset += 64\n");
    ContextEx->XState.Offset = OriginalXStateOffset + 1;
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature == NULL, "Expected Feature == NULL for XState.Offset += 1\n");
    ContextEx->XState.Offset = OriginalXStateOffset - 1;
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature == NULL, "Expected Feature == NULL for XState.Offset -= 1\n");
    ContextEx->XState.Offset = OriginalXStateOffset - 64;
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, NewXStateConfig, &Length);
    ok(Feature == NULL, "Expected Feature == NULL for XState.Offset -= 64\n");

    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Test CONTEXT_i386 without XSTATE */
    ContextBuffer = CreateExtendedContext(I386_CONTEXT_ALL, 0, &ContextEx);
    Feature = pRtlLocateExtendedFeature2(ContextEx, LowestFeatureIndex, XStateConfig, &Length);
    ok(Feature == NULL, "Expected lowest enabled feature %lu to be absent\n", LowestFeatureIndex);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

}

static void Test_RtlLocateLegacyContext(void)
{
    PVOID ContextBuffer;
    PCONTEXT_EX ContextEx;
    PVOID LegacyContext, Expected;
    ULONG Length;

    if (pRtlLocateLegacyContext == NULL)
    {
        skip("RtlLocateLegacyContext not found\n");
        return;
    }

    /* Test native context */
    ContextBuffer = CreateExtendedContext(CONTEXT_ALL, 0, &ContextEx);
    Expected = (PUCHAR)ContextEx + ContextEx->Legacy.Offset;
    LegacyContext = pRtlLocateLegacyContext(ContextEx, NULL);
    ok_eq_pointer(LegacyContext, Expected);
    LegacyContext = pRtlLocateLegacyContext(ContextEx, &Length);
    ok_eq_pointer(LegacyContext, Expected);
    ok_eq_hex(Length, ContextEx->Legacy.Length);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Test I386 context with XSTATE */
    ContextBuffer = CreateExtendedContext(I386_CONTEXT_ALL | I386_CONTEXT_XSTATE, 0, &ContextEx);
    Expected = (PUCHAR)ContextEx + ContextEx->Legacy.Offset;
    LegacyContext = pRtlLocateLegacyContext(ContextEx, &Length);
    ok_eq_pointer(LegacyContext, Expected);
    ok_eq_hex(Length, sizeof(I386_CONTEXT));
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Test I386 context without extended registers */
    ContextBuffer = CreateExtendedContext(I386_CONTEXT_INTEGER, 0, &ContextEx);
    Expected = (PUCHAR)ContextEx + ContextEx->Legacy.Offset;
    LegacyContext = pRtlLocateLegacyContext(ContextEx, &Length);
    ok_eq_pointer(LegacyContext, Expected);
    ok_eq_hex(Length, FIELD_OFFSET(I386_CONTEXT, ExtendedRegisters));
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Test I386 context with offset */
    ContextBuffer = CreateExtendedContext(I386_CONTEXT_ALL, 3, &ContextEx);
    Expected = (PUCHAR)ContextEx + ContextEx->Legacy.Offset;
    LegacyContext = pRtlLocateLegacyContext(ContextEx, &Length);
    ok_eq_pointer(LegacyContext, Expected);
    ok_eq_hex(Length, sizeof(I386_CONTEXT));
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Test AMD64 context with XSTATE */
    ContextBuffer = CreateExtendedContext(AMD64_CONTEXT_ALL | AMD64_CONTEXT_XSTATE, 0, &ContextEx);
    Expected = (PUCHAR)ContextEx + ContextEx->Legacy.Offset;
    LegacyContext = pRtlLocateLegacyContext(ContextEx, &Length);
    ok_eq_pointer(LegacyContext, Expected);
    ok_eq_hex(Length, sizeof(AMD64_CONTEXT));
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Test ARM32 context */
    ContextBuffer = CreateExtendedContext(ARM32_CONTEXT_ALL, 0, &ContextEx);
    Expected = (PUCHAR)ContextEx + ContextEx->Legacy.Offset;
    LegacyContext = pRtlLocateLegacyContext(ContextEx, &Length);
    ok_eq_pointer(LegacyContext, Expected);
    ok_eq_hex(Length, sizeof(ARM32_CONTEXT));
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Test ARM64 context */
    ContextBuffer = CreateExtendedContext(ARM64_CONTEXT_ALL, 0, &ContextEx);
    Expected = (PUCHAR)ContextEx + ContextEx->Legacy.Offset;
    LegacyContext = pRtlLocateLegacyContext(ContextEx, &Length);
    ok_eq_pointer(LegacyContext, Expected);
    ok_eq_hex(Length, sizeof(ARM64_CONTEXT));
    HeapFree(GetProcessHeap(), 0, ContextBuffer);
}

static void Test_RtlGetExtendedFeaturesMask(void)
{
    PVOID ContextBuffer;
    PCONTEXT_EX ContextEx;
    ULONG64 FeatureMask;
    PXSAVE_AREA_HEADER XSaveAreaHeader;

    if (pRtlGetExtendedFeaturesMask == NULL)
    {
        skip("RtlGetExtendedFeaturesMask not found\n");
        return;
    }

    /* Test native context without XSTATE */
    ContextBuffer = CreateExtendedContext(CONTEXT_ALL, 0, &ContextEx);

    /* This one is an access violation on Windows */
    StartSeh()
        FeatureMask = pRtlGetExtendedFeaturesMask(ContextEx);
    EndSeh(is_reactos() ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    ContextEx->XState.Offset = 0;
    StartSeh()
        FeatureMask = pRtlGetExtendedFeaturesMask(ContextEx);
    EndSeh(STATUS_SUCCESS);
    ContextEx->XState.Offset = -17;
    StartSeh()
        FeatureMask = pRtlGetExtendedFeaturesMask(ContextEx);
    EndSeh(STATUS_SUCCESS);

#if defined(_M_IX86) || defined(_M_AMD64)
    /* Test native context with XSTATE */
    ContextBuffer = CreateExtendedContext(CONTEXT_XSTATE, 0, &ContextEx);
    XSaveAreaHeader = (PXSAVE_AREA_HEADER)((PUCHAR)ContextEx + ContextEx->XState.Offset);
    FeatureMask = pRtlGetExtendedFeaturesMask(ContextEx);
    ok_eq_ulonglong(FeatureMask, 0);
    XSaveAreaHeader->Mask = ULLONG_MAX;
    FeatureMask = pRtlGetExtendedFeaturesMask(ContextEx);
    ok_eq_hex64(FeatureMask, ULLONG_MAX & ~3);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);
#endif

    /* Test I386 context with XSTATE */
    ContextBuffer = CreateExtendedContext(I386_CONTEXT_ALL | I386_CONTEXT_XSTATE, 0, &ContextEx);
    XSaveAreaHeader = (PXSAVE_AREA_HEADER)((PUCHAR)ContextEx + ContextEx->XState.Offset);
    FeatureMask = pRtlGetExtendedFeaturesMask(ContextEx);
    ok_eq_hex64(FeatureMask, 0);
    XSaveAreaHeader->Mask = ULLONG_MAX;
    FeatureMask = pRtlGetExtendedFeaturesMask(ContextEx);
    ok_eq_hex64(FeatureMask, ULLONG_MAX & ~3);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Test AMD64 context with XSTATE */
    ContextBuffer = CreateExtendedContext(AMD64_CONTEXT_XSTATE, 0, &ContextEx);
    XSaveAreaHeader = (PXSAVE_AREA_HEADER)((PUCHAR)ContextEx + ContextEx->XState.Offset);
    FeatureMask = pRtlGetExtendedFeaturesMask(ContextEx);
    ok_eq_hex64(FeatureMask, 0);
    XSaveAreaHeader->Mask = ULLONG_MAX;
    FeatureMask = pRtlGetExtendedFeaturesMask(ContextEx);
    ok_eq_hex64(FeatureMask, ULLONG_MAX & ~3);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);
}

static void Test_RtlSetExtendedFeaturesMask(void)
{
    PVOID ContextBuffer;
    PCONTEXT_EX ContextEx;
    PXSAVE_AREA_HEADER XSaveAreaHeader;
    ULONG64 EnabledFeatures;

    if ((pRtlGetEnabledExtendedFeatures == NULL) ||
        (pRtlSetExtendedFeaturesMask == NULL))
    {
        skip("RtlSetExtendedFeaturesMask not found\n");
        return;
    }

    EnabledFeatures = pRtlGetEnabledExtendedFeatures(ULLONG_MAX);

    /* Test native context without XSTATE */
    ContextBuffer = CreateExtendedContext(CONTEXT_ALL, 0, &ContextEx);

    /* This one is an access violation on Windows */
    StartSeh()
        pRtlSetExtendedFeaturesMask(ContextEx, 0);
    EndSeh(is_reactos() ? STATUS_SUCCESS : STATUS_ACCESS_VIOLATION);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

#if defined(_M_IX86) || defined(_M_AMD64)
    /* Test native context with XSTATE */
    ContextBuffer = CreateExtendedContext(CONTEXT_XSTATE, 0, &ContextEx);
    XSaveAreaHeader = (PXSAVE_AREA_HEADER)((PUCHAR)ContextEx + ContextEx->XState.Offset);
    ok_eq_hex64(XSaveAreaHeader->Mask, 0);
    pRtlSetExtendedFeaturesMask(ContextEx, ULLONG_MAX);
    ok_eq_hex64(XSaveAreaHeader->Mask, EnabledFeatures & ~3);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);
#endif

    /* Test I386 context with XSTATE */
    ContextBuffer = CreateExtendedContext(I386_CONTEXT_ALL | I386_CONTEXT_XSTATE, 0, &ContextEx);
    XSaveAreaHeader = (PXSAVE_AREA_HEADER)((PUCHAR)ContextEx + ContextEx->XState.Offset);
    ok_eq_hex64(XSaveAreaHeader->Mask, 0);
    pRtlSetExtendedFeaturesMask(ContextEx, ULLONG_MAX);
    ok_eq_hex64(XSaveAreaHeader->Mask, EnabledFeatures & ~3);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);

    /* Test AMD64 context with XSTATE */
    ContextBuffer = CreateExtendedContext(AMD64_CONTEXT_XSTATE, 0, &ContextEx);
    XSaveAreaHeader = (PXSAVE_AREA_HEADER)((PUCHAR)ContextEx + ContextEx->XState.Offset);
    ok_eq_hex64(XSaveAreaHeader->Mask, 0);
    pRtlSetExtendedFeaturesMask(ContextEx, ULLONG_MAX);
    ok_eq_hex64(XSaveAreaHeader->Mask, EnabledFeatures & ~3);
    HeapFree(GetProcessHeap(), 0, ContextBuffer);
}

static void Test_RtlCopyExtendedContext(void)
{
    NTSTATUS Status;
    PVOID SrcContextBuffer, DstContextBuffer;
    PCONTEXT_EX SrcContextEx, DstContextEx;
    PAMD64_CONTEXT SrcContextAmd64, DstContextAmd64;
    PXSAVE_AREA_HEADER SrcXSaveAreaHeader, DstXSaveAreaHeader;
    ULONG MatchingBytes;
    ULONG64 EnabledFeatures;

    if (pRtlCopyExtendedContext == NULL)
    {
        skip("RtlCopyExtendedContext not found\n");
        return;
    }

    EnabledFeatures = pRtlGetEnabledExtendedFeatures(ULLONG_MAX);

    /* Check if compaction is enabled */
    PXSTATE_CONFIGURATION XStateConfig = KUSER_SHARED_DATA_XState(SharedUserData);
    ULONG ControlFlags = XSTATE_CONFIGURATION_ControlFlags(XStateConfig);
    ULONG64 CompactionEnabledBit = (ControlFlags & 2) ? 0x8000000000000000ull : 0;

    Status = pRtlCopyExtendedContext(NULL, 0, NULL);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    SrcContextBuffer = CreateExtendedContext(CONTEXT_ALL, 0, &SrcContextEx);
    Status = pRtlCopyExtendedContext(NULL, 0, SrcContextEx);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    DstContextBuffer = CreateExtendedContext(CONTEXT_ALL, 0, &DstContextEx);
    Status = pRtlCopyExtendedContext(DstContextEx, 0, SrcContextEx);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlCopyExtendedContext(DstContextEx, CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = pRtlCopyExtendedContext(DstContextEx, CONTEXT_CONTROL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    HeapFree(GetProcessHeap(), 0, SrcContextBuffer);

    SrcContextBuffer = CreateExtendedContext(CONTEXT_CONTROL, 0, &SrcContextEx);
    Status = pRtlCopyExtendedContext(DstContextEx, CONTEXT_CONTROL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = pRtlCopyExtendedContext(DstContextEx, CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    HeapFree(GetProcessHeap(), 0, SrcContextBuffer);
    HeapFree(GetProcessHeap(), 0, DstContextBuffer);

    /* Test I386 context integer -> all */
    SrcContextBuffer = CreateExtendedContext(I386_CONTEXT_INTEGER, 0, &SrcContextEx);
    ok_eq_hex(SrcContextEx->Legacy.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(SrcContextEx->Legacy.Length, FIELD_OFFSET(I386_CONTEXT, ExtendedRegisters));
    DstContextBuffer = CreateExtendedContext(I386_CONTEXT_ALL, 0, &DstContextEx);
    ok_eq_hex(DstContextEx->Legacy.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(DstContextEx->Legacy.Length, sizeof(I386_CONTEXT));
    Status = pRtlCopyExtendedContext(DstContextEx, I386_CONTEXT_CONTROL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = pRtlCopyExtendedContext(DstContextEx, I386_CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(DstContextEx->Legacy.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(DstContextEx->Legacy.Length, sizeof(I386_CONTEXT));
    HeapFree(GetProcessHeap(), 0, SrcContextBuffer);
    HeapFree(GetProcessHeap(), 0, DstContextBuffer);

    /* Test I386 context all -> integer */
    SrcContextBuffer = CreateExtendedContext(I386_CONTEXT_ALL, 0, &SrcContextEx);
    ok_eq_hex(SrcContextEx->Legacy.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(SrcContextEx->Legacy.Length, sizeof(I386_CONTEXT));
    DstContextBuffer = CreateExtendedContext(I386_CONTEXT_INTEGER, 0, &DstContextEx);
    ok_eq_hex(DstContextEx->Legacy.Offset, -(LONG)sizeof(I386_CONTEXT));
    ok_eq_hex(DstContextEx->Legacy.Length, FIELD_OFFSET(I386_CONTEXT, ExtendedRegisters));
    Status = pRtlCopyExtendedContext(DstContextEx, I386_CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlCopyExtendedContext(DstContextEx, I386_CONTEXT_INTEGER, SrcContextEx);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    HeapFree(GetProcessHeap(), 0, SrcContextBuffer);
    HeapFree(GetProcessHeap(), 0, DstContextBuffer);

    /* Test AMD64 context integer -> all */
    SrcContextBuffer = CreateExtendedContext(AMD64_CONTEXT_INTEGER, 0, &SrcContextEx);
    ok_eq_hex(SrcContextEx->Legacy.Offset, -(LONG)sizeof(AMD64_CONTEXT));
    ok_eq_hex(SrcContextEx->Legacy.Length, sizeof(AMD64_CONTEXT));
    DstContextBuffer = CreateExtendedContext(AMD64_CONTEXT_ALL, 0, &DstContextEx);
    ok_eq_hex(DstContextEx->Legacy.Offset, -(LONG)sizeof(AMD64_CONTEXT));
    ok_eq_hex(DstContextEx->Legacy.Length, sizeof(AMD64_CONTEXT));
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_CONTROL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(DstContextEx->Legacy.Offset, -(LONG)sizeof(AMD64_CONTEXT));
    ok_eq_hex(DstContextEx->Legacy.Length, sizeof(AMD64_CONTEXT));
    HeapFree(GetProcessHeap(), 0, SrcContextBuffer);
    HeapFree(GetProcessHeap(), 0, DstContextBuffer);

    /* Test AMD64 context all -> integer */
    SrcContextBuffer = CreateExtendedContext(AMD64_CONTEXT_ALL, 0, &SrcContextEx);
    ok_eq_hex(SrcContextEx->Legacy.Offset, -(LONG)sizeof(AMD64_CONTEXT));
    ok_eq_hex(SrcContextEx->Legacy.Length, sizeof(AMD64_CONTEXT));
    DstContextBuffer = CreateExtendedContext(AMD64_CONTEXT_INTEGER, 0, &DstContextEx);
    ok_eq_hex(DstContextEx->Legacy.Offset, -(LONG)sizeof(AMD64_CONTEXT));
    ok_eq_hex(DstContextEx->Legacy.Length, sizeof(AMD64_CONTEXT));
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_INTEGER, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_XSTATE, SrcContextEx);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    SrcContextAmd64 = (PAMD64_CONTEXT)((PUCHAR)SrcContextEx + SrcContextEx->Legacy.Offset);
    SrcContextAmd64->ContextFlags |= 0x200;
    DstContextAmd64 = (PAMD64_CONTEXT)((PUCHAR)DstContextEx + DstContextEx->Legacy.Offset);
    DstContextAmd64->ContextFlags |= 0x200;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_INTEGER, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    DstContextAmd64->ContextFlags |= CONTEXT_i386;;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_INTEGER, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    DstContextAmd64->ContextFlags &= CONTEXT_AMD64;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_INTEGER, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    DstContextEx->Legacy.Length -= 1;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_INTEGER, SrcContextEx);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    DstContextEx->Legacy.Length += 2;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_INTEGER, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    DstContextEx->Legacy.Offset -= 1;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_INTEGER, SrcContextEx);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    DstContextEx->Legacy.Offset += 2;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_INTEGER, SrcContextEx);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    HeapFree(GetProcessHeap(), 0, SrcContextBuffer);
    HeapFree(GetProcessHeap(), 0, DstContextBuffer);

    /* Test I386 context all -> AMD64 context */
    SrcContextBuffer = CreateExtendedContext(I386_CONTEXT_ALL, 0, &SrcContextEx);
    DstContextBuffer = CreateExtendedContext(AMD64_CONTEXT_ALL, 0, &DstContextEx);
    Status = pRtlCopyExtendedContext(DstContextEx, I386_CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    HeapFree(GetProcessHeap(), 0, SrcContextBuffer);
    HeapFree(GetProcessHeap(), 0, DstContextBuffer);

    /* Test AMD64 context with XSTATE -> without XSTATE */
    SrcContextBuffer = CreateExtendedContext(AMD64_CONTEXT_ALL | AMD64_CONTEXT_XSTATE, 0, &SrcContextEx);
    SrcContextAmd64 = (PAMD64_CONTEXT)((PUCHAR)SrcContextEx + SrcContextEx->Legacy.Offset);
    DstContextBuffer = CreateExtendedContext(AMD64_CONTEXT_ALL, 0, &DstContextEx);
    DstContextAmd64 = (PAMD64_CONTEXT)((PUCHAR)DstContextEx + DstContextEx->Legacy.Offset);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    SrcContextAmd64->Rip = 0x123456789ABCDEF0ull;
    DstContextAmd64->Rip = 0;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_XSTATE, SrcContextEx);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_hex64(DstContextAmd64->Rip, 0);

    HeapFree(GetProcessHeap(), 0, SrcContextBuffer);
    HeapFree(GetProcessHeap(), 0, DstContextBuffer);

    /* Test AMD64 context without XSTATE -> with XSTATE */
    SrcContextBuffer = CreateExtendedContext(AMD64_CONTEXT_ALL, 0, &SrcContextEx);
    SrcContextAmd64 = (PAMD64_CONTEXT)((PUCHAR)SrcContextEx + SrcContextEx->Legacy.Offset);
    DstContextBuffer = CreateExtendedContext(AMD64_CONTEXT_ALL | AMD64_CONTEXT_XSTATE, 0, &DstContextEx);
    DstContextBuffer = CreateExtendedContext(AMD64_CONTEXT_ALL, 0, &DstContextEx);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    SrcContextAmd64->Rip = 0x123456789ABCDEF0ull;
    DstContextAmd64->Rip = 0;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_XSTATE, SrcContextEx);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_hex64(DstContextAmd64->Rip, 0);
    HeapFree(GetProcessHeap(), 0, SrcContextBuffer);
    HeapFree(GetProcessHeap(), 0, DstContextBuffer);

    /* Create source and dest contextx and get pointers to the legacy context and XSTATE */
    SrcContextBuffer = CreateExtendedContext(AMD64_CONTEXT_ALL | AMD64_CONTEXT_XSTATE, 0, &SrcContextEx);
    SrcContextAmd64 = (PAMD64_CONTEXT)((PUCHAR)SrcContextEx + SrcContextEx->Legacy.Offset);
    SrcXSaveAreaHeader = (PXSAVE_AREA_HEADER)((PUCHAR)SrcContextEx + SrcContextEx->XState.Offset);
    DstContextBuffer = CreateExtendedContext(AMD64_CONTEXT_ALL | AMD64_CONTEXT_XSTATE, 0, &DstContextEx);
    DstContextAmd64 = (PAMD64_CONTEXT)((PUCHAR)DstContextEx + DstContextEx->Legacy.Offset);
    DstXSaveAreaHeader = (PXSAVE_AREA_HEADER)((PUCHAR)DstContextEx + DstContextEx->XState.Offset);

    /* Try copying legacy context with messed up context flags */
    SrcContextAmd64->ContextFlags = 0xDEADBEEF;
    DstContextAmd64->ContextFlags = 0x0F00BAAB;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_INTEGER, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(SrcContextAmd64->ContextFlags, 0xDEADBEEF);
    ok_eq_hex(DstContextAmd64->ContextFlags, AMD64_CONTEXT_INTEGER);

    /* Copy individual parts with different values */
    RtlFillMemory(DstContextAmd64, sizeof(*DstContextAmd64), 0xCC);
    RtlFillMemory(SrcContextAmd64, sizeof(*SrcContextAmd64), 0x11);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_CONTROL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(DstContextAmd64->ContextFlags, AMD64_CONTEXT_CONTROL);
    RtlFillMemory(SrcContextAmd64, sizeof(*SrcContextAmd64), 0x22);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_INTEGER, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(DstContextAmd64->ContextFlags, AMD64_CONTEXT_INTEGER);
    RtlFillMemory(SrcContextAmd64, sizeof(*SrcContextAmd64), 0x33);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_SEGMENTS, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(DstContextAmd64->ContextFlags, AMD64_CONTEXT_SEGMENTS);
    RtlFillMemory(SrcContextAmd64, sizeof(*SrcContextAmd64), 0x44);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_FLOATING_POINT, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(DstContextAmd64->ContextFlags, AMD64_CONTEXT_FLOATING_POINT);
    RtlFillMemory(SrcContextAmd64, sizeof(*SrcContextAmd64), 0x55);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_DEBUG_REGISTERS, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex(DstContextAmd64->ContextFlags, AMD64_CONTEXT_DEBUG_REGISTERS);

    /* Now check that individual parts have the appropriate values */
    ok_eq_hex64(DstContextAmd64->P1Home, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(DstContextAmd64->P2Home, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(DstContextAmd64->P3Home, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(DstContextAmd64->P4Home, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(DstContextAmd64->P5Home, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(DstContextAmd64->P6Home, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex(DstContextAmd64->ContextFlags, AMD64_CONTEXT_DEBUG_REGISTERS);
    ok_eq_hex(DstContextAmd64->MxCsr, 0x44444444);
    ok_eq_hex(DstContextAmd64->SegCs, 0x1111);
    ok_eq_hex(DstContextAmd64->SegDs, 0x3333);
    ok_eq_hex(DstContextAmd64->SegEs, 0x3333);
    ok_eq_hex(DstContextAmd64->SegFs, 0x3333);
    ok_eq_hex(DstContextAmd64->SegGs, 0x3333);
    ok_eq_hex(DstContextAmd64->SegSs, 0x1111);
    ok_eq_hex(DstContextAmd64->EFlags, 0x11111111);
    ok_eq_hex64(DstContextAmd64->Dr0, 0x5555555555555555ull);
    ok_eq_hex64(DstContextAmd64->Dr1, 0x5555555555555555ull);
    ok_eq_hex64(DstContextAmd64->Dr2, 0x5555555555555555ull);
    ok_eq_hex64(DstContextAmd64->Dr3, 0x5555555555555555ull);
    ok_eq_hex64(DstContextAmd64->Dr6, 0x5555555555555555ull);
    ok_eq_hex64(DstContextAmd64->Dr7, 0x5555555555555555ull);
    ok_eq_hex64(DstContextAmd64->Rax, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->Rcx, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->Rdx, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->Rbx, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->Rsp, 0x1111111111111111ull);
    ok_eq_hex64(DstContextAmd64->Rbp, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->Rsi, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->Rdi, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->R8, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->R9, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->R10, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->R11, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->R12, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->R13, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->R14, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->R15, 0x2222222222222222ull);
    ok_eq_hex64(DstContextAmd64->Rip, 0x1111111111111111ull);
    MatchingBytes = EqualBytes(&DstContextAmd64->FltSave, FIELD_OFFSET(XMM_SAVE_AREA64, Reserved4), 0, 0x44);
    ok_eq_ulong(MatchingBytes, FIELD_OFFSET(XMM_SAVE_AREA64, Reserved4));
    MatchingBytes = EqualBytes(&DstContextAmd64->FltSave.Reserved4, sizeof(DstContextAmd64->FltSave.Reserved4), 0, 0xCC);
    ok_eq_ulong(MatchingBytes, sizeof(DstContextAmd64->FltSave.Reserved4));
    MatchingBytes = EqualBytes(&DstContextAmd64->VectorRegister, sizeof(DstContextAmd64->VectorRegister), 0, 0xCC);
    ok_eq_ulong(MatchingBytes, sizeof(DstContextAmd64->VectorRegister));
    ok_eq_hex64(DstContextAmd64->VectorControl, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(DstContextAmd64->DebugControl, 0xCCCCCCCCCCCCCCCCull);
    ok_eq_hex64(DstContextAmd64->LastBranchToRip, 0x5555555555555555ull);
    ok_eq_hex64(DstContextAmd64->LastBranchFromRip, 0x5555555555555555ull);
    ok_eq_hex64(DstContextAmd64->LastExceptionToRip, 0x5555555555555555ull);
    ok_eq_hex64(DstContextAmd64->LastExceptionFromRip, 0x5555555555555555ull);

    /* The source context length must not be larger then the destination context length */
    RtlFillMemory(SrcContextAmd64, sizeof(*SrcContextAmd64), 0x11);
    DstContextEx->Legacy.Length = 10;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok_eq_hex64(DstContextAmd64->LastExceptionFromRip, 0x5555555555555555ull);
    SrcContextEx->Legacy.Length = 10;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex64(DstContextAmd64->LastExceptionFromRip, 0x1111111111111111ull);
    SrcContextEx->Legacy.Length = 2 * sizeof(AMD64_CONTEXT);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    DstContextEx->Legacy.Length = 2 * sizeof(AMD64_CONTEXT);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    SrcContextEx->Legacy.Length = sizeof(AMD64_CONTEXT);
    DstContextEx->Legacy.Length = sizeof(AMD64_CONTEXT);

    /* Now copy XSTATE */
    RtlFillMemory(SrcXSaveAreaHeader, sizeof(*DstXSaveAreaHeader), 0xAA);
    RtlFillMemory(DstXSaveAreaHeader, sizeof(*DstXSaveAreaHeader), 0xBB);
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_XSTATE, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex64(DstXSaveAreaHeader->Mask, SrcXSaveAreaHeader->Mask & EnabledFeatures & ~3);
    ok_eq_hex64(DstXSaveAreaHeader->CompactionMask, (SrcXSaveAreaHeader->CompactionMask & EnabledFeatures) | CompactionEnabledBit);
    MatchingBytes = EqualBytes(DstXSaveAreaHeader->Reserved2, sizeof(DstXSaveAreaHeader->Reserved2), 0, 0x00);
    ok_eq_hex(MatchingBytes, sizeof(DstXSaveAreaHeader->Reserved2));

    /* XState length is not validated, if the XState contains invalid flags and the entire thing is zeroed! */
    RtlFillMemory(SrcXSaveAreaHeader, sizeof(*DstXSaveAreaHeader), 0xAA);
    RtlFillMemory(DstXSaveAreaHeader, sizeof(*DstXSaveAreaHeader), 0xBB);
    SrcContextAmd64->LastExceptionFromRip = 0x4545454545454545ull;
    DstContextAmd64->LastExceptionFromRip = 0x1111111111111111ull;
    SrcContextEx->XState.Length -= 1;
    DstContextEx->XState.Length -= 1;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL | AMD64_CONTEXT_XSTATE, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex64(DstContextAmd64->LastExceptionFromRip, 0x4545454545454545ull);
    DstContextAmd64->LastExceptionFromRip = 0x1111111111111111ull;
    SrcContextEx->XState.Length += 2;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL | AMD64_CONTEXT_XSTATE, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex64(DstContextAmd64->LastExceptionFromRip, 0x4545454545454545ull);
    ok_eq_hex64(DstXSaveAreaHeader->Mask, SrcXSaveAreaHeader->Mask & EnabledFeatures & ~3);
    ok_eq_hex64(DstXSaveAreaHeader->CompactionMask, (SrcXSaveAreaHeader->CompactionMask & EnabledFeatures) | CompactionEnabledBit);
    MatchingBytes = EqualBytes(DstXSaveAreaHeader->Reserved2, sizeof(DstXSaveAreaHeader->Reserved2), 0, 0x00);
    ok_eq_hex(MatchingBytes, sizeof(DstXSaveAreaHeader->Reserved2));

    /* With valid flags, the buffer size is validated and neither must be too small */
    SrcContextAmd64->LastExceptionFromRip = 0x4545454545454545ull;
    DstContextAmd64->LastExceptionFromRip = 0x1111111111111111ull;
    SrcXSaveAreaHeader->Mask = EnabledFeatures & ~3;
    SrcXSaveAreaHeader->CompactionMask = EnabledFeatures | CompactionEnabledBit;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL | AMD64_CONTEXT_XSTATE, SrcContextEx);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_hex64(DstContextAmd64->LastExceptionFromRip, 0x4545454545454545ull);
    DstContextAmd64->LastExceptionFromRip = 0x1111111111111111ull;
    DstContextEx->XState.Length += 1;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL | AMD64_CONTEXT_XSTATE, SrcContextEx);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_hex64(DstContextAmd64->LastExceptionFromRip, 0x4545454545454545ull);
    DstContextAmd64->LastExceptionFromRip = 0x1111111111111111ull;
    SrcContextEx->XState.Length -= 2;
    Status = pRtlCopyExtendedContext(DstContextEx, AMD64_CONTEXT_ALL | AMD64_CONTEXT_XSTATE, SrcContextEx);
    ok_eq_hex(Status, STATUS_BUFFER_OVERFLOW);
    ok_eq_hex64(DstContextAmd64->LastExceptionFromRip, 0x4545454545454545ull);

    HeapFree(GetProcessHeap(), 0, SrcContextBuffer);
    HeapFree(GetProcessHeap(), 0, DstContextBuffer);

    // TODO: ARM contexts
}

START_TEST(RtlExtendedContext)
{
    InitializeFunctionPointers("ntdll.dll");
    if (is_reactos())
    {
        /* Try ntdll_vista.dll as well */
        InitializeFunctionPointers("ntdll_vista.dll");
    }

    if (is_reactos() || GetNTVersion() >= _WIN32_WINNT_WIN7)
    {
        ok(pRtlCopyExtendedContext != NULL, "RtlCopyExtendedContext not found\n");
        ok(pRtlGetEnabledExtendedFeatures != NULL, "RtlGetEnabledExtendedFeatures not found\n");
        ok(pRtlGetExtendedContextLength != NULL, "RtlGetExtendedContextLength not found\n");
        ok(pRtlGetExtendedFeaturesMask != NULL, "RtlGetExtendedFeaturesMask not found\n");
        ok(pRtlInitializeExtendedContext != NULL, "RtlInitializeExtendedContext not found\n");
        ok(pRtlLocateExtendedFeature != NULL, "RtlLocateExtendedFeature not found\n");
        ok(pRtlLocateLegacyContext != NULL, "RtlLocateLegacyContext not found\n");
        ok(pRtlSetExtendedFeaturesMask != NULL, "RtlSetExtendedFeaturesMask not found\n");
    }

    if (is_reactos() || GetNTVersion() >= _WIN32_WINNT_WIN10)
    {
        ok(pRtlGetExtendedContextLength2 != NULL, "RtlGetExtendedContextLength2 not found\n");
        ok(pRtlInitializeExtendedContext2 != NULL, "RtlInitializeExtendedContext2 not found\n");
        ok(pRtlLocateExtendedFeature2 != NULL, "RtlLocateExtendedFeature2 not found\n");
    }

    Test_RtlGetEnabledExtendedFeatures();
    Test_RtlGetExtendedContextLength();
    Test_RtlGetExtendedContextLength2();
    Test_RtlInitializeExtendedContext();
    Test_RtlInitializeExtendedContext2();
    Test_RtlLocateExtendedFeature();
    Test_RtlLocateExtendedFeature2();
    Test_RtlLocateLegacyContext();
    Test_RtlGetExtendedFeaturesMask();
    Test_RtlSetExtendedFeaturesMask();
    Test_RtlCopyExtendedContext();
}
