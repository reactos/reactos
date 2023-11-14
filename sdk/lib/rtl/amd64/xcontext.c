/*
 * PROJECT:     ReactOS Runtime Library
 * LICENSE:     GPL-2.0+ (https://spdx.org/licenses/GPL-2.0+)
 * PURPOSE:     Functions related to extended contexts
 * COPYRIGHT:   Copyright 20123 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/


NTSYSAPI
ULONG64
NTAPI
RtlGetEnabledExtendedFeatures(
    _In_ ULONG64 FeatureMask)
{
    return SharedUserData->XState.EnabledFeatures & FeatureMask;
}

#ifndef CONTEXT_i386
#define CONTEXT_i386    0x10000
#endif
#ifndef CONTEXT_AMD64
#define CONTEXT_AMD64   0x00100000
#endif
#ifndef CONTEXT_ARM32
#define CONTEXT_ARM32   0x00200000
#endif
#ifndef CONTEXT_ARM64
#define CONTEXT_ARM64   0x00400000
#endif

#define CONTEXT_XSTATE          (CONTEXT_AMD64 | 0x00000040L)
#define CONTEXT_KERNEL_CET      (CONTEXT_AMD64 | 0x00000080L)

NTSYSAPI
ULONG
NTAPI
RtlGetExtendedContextLength2 (
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength,
    _In_ ULONG64 XStateCompactionMask)
{
    ULONG Length;

    ULONG Architecture = ContextFlags & (CONTEXT_i386 | CONTEXT_AMD64 | CONTEXT_ARM32 | CONTEXT_ARM64);

    if (Architecture == CONTEXT_i386)
    {
        Length = sizeof(CONTEXT);
    }
    else if (Architecture == CONTEXT_AMD64)
    {
        Length = sizeof(CONTEXT);
    }
    else if (Architecture == CONTEXT_ARM32)
    {
        Length = sizeof(CONTEXT);
    }
    else if (Architecture == CONTEXT_ARM64)
    {
        Length = sizeof(CONTEXT);
    }
    else
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((ContextFlags & CONTEXT_XSTATE) == CONTEXT_XSTATE)
    {
        Length += 1;
    }

    if (RtlpGetMode() != UserMode)
    {
        if ((ContextFlags & CONTEXT_KERNEL_CET) == CONTEXT_KERNEL_CET)
        {
            Length += sizeof(CONTEXT);
        }
    }

    *ContextLength = Length;
    return STATUS_SUCCESS;
}

NTSYSAPI
ULONG
NTAPI
RtlGetExtendedContextLength (
    _In_ ULONG ContextFlags,
    _Out_ PULONG ContextLength)
{
    ULONG64 CompactionMask;

    CompactionMask = SharedUserData->XState.EnabledFeatures;
    if (RtlpGetMode() != UserMode)
    {
        CompactionMask |= SharedUserData->XState.EnabledUserVisibleSupervisorFeatures;
    }

    return RtlGetExtendedContextLength2(ContextFlags, ContextLength, CompactionMask);
}

typedef struct _CONTEXT_EX *PCONTEXT_EX;

NTSYSAPI
ULONG
NTAPI
RtlInitializeExtendedContext (
    _Out_ PVOID Context,
    _In_ ULONG ContextFlags,
    _Out_ PCONTEXT_EX* ContextEx
);

NTSYSAPI
ULONG
NTAPI
RtlInitializeExtendedContext2 (
    _Out_ PVOID Context,
    _In_ ULONG ContextFlags,
    _Out_ PCONTEXT_EX* ContextEx,
    _In_ ULONG64 XStateCompactionMask
);

NTSYSAPI
PCONTEXT
NTAPI
RtlLocateLegacyContext (
    _In_ PCONTEXT_EX ContextEx,
    _Out_opt_ PULONG Length
);

// These should go to kernel32
_Success_(return != FALSE)
WINBASEAPI
BOOL
WINAPI
InitializeContext(
    _Out_writes_bytes_opt_(*ContextLength) PVOID Buffer,
    _In_ DWORD ContextFlags,
    _Out_ PCONTEXT* Context,
    _Inout_ PDWORD ContextLength
    );

_Success_(return != FALSE)
WINBASEAPI
BOOL
WINAPI
InitializeContext2(
    _Out_writes_bytes_opt_(*ContextLength) PVOID Buffer,
    _In_ DWORD ContextFlags,
    _Out_ PCONTEXT* Context,
    _Inout_ PDWORD ContextLength,
    _In_ ULONG64 XStateCompactionMask
    );
