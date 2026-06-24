/*
 * PROJECT:     ReactOS runtime library
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     NtUser PFN registration
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include <rtl.h>
#include <ndk/rtlfuncs.h>

#define NDEBUG
#include <debug.h>

typedef struct _NTUSER_PFN
{
    PVOID WndProcsA[RTLP_NUMBER_OF_NTUSER_WNDPROCS];
    PVOID WndProcsW[RTLP_NUMBER_OF_NTUSER_WNDPROCS];
    PVOID WorkerProcs[RTLP_NUMBER_OF_NTUSER_WORKERPROCS];
} NTUSER_PFN, *PNTUSER_PFN;

const NTUSER_PFN RtlpNtUserPfns = { 0 };

extern const NTUSER_PFN NtDllUserStubs;

static BOOLEAN NtUserPfnsInitialized;

NTSTATUS
NTAPI
RtlInitializeNtUserPfn(
    _In_reads_bytes_(SizeOfWndProcsA) LPCVOID WndProcsA,
    _In_ SIZE_T SizeOfWndProcsA,
    _In_reads_bytes_(SizeOfWndProcsW) LPCVOID WndProcsW,
    _In_ SIZE_T SizeOfWndProcsW,
    _In_reads_bytes_(SizeOfWorkerProcs) LPCVOID WorkerProcs,
    _In_ SIZE_T SizeOfWorkerProcs)
{
    ULONG OldProtect;
    PVOID BaseAddress;
    SIZE_T RegionSize;
    NTSTATUS Status;

    /* Validate parameters */
    if ((SizeOfWndProcsA != sizeof(RtlpNtUserPfns.WndProcsA)) ||
        (SizeOfWndProcsW != sizeof(RtlpNtUserPfns.WndProcsW)) ||
        (SizeOfWorkerProcs != sizeof(RtlpNtUserPfns.WorkerProcs)))
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Check if already initialized */
    if (NtUserPfnsInitialized != FALSE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Change page protection, so we can set up the PFNs */
    BaseAddress = (PVOID)&RtlpNtUserPfns;
    RegionSize = sizeof(RtlpNtUserPfns);
    Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &BaseAddress,
                                    &RegionSize,
                                    PAGE_READWRITE,
                                    &OldProtect);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    RtlCopyMemory((PVOID)RtlpNtUserPfns.WndProcsA, WndProcsA, SizeOfWndProcsA);
    RtlCopyMemory((PVOID)RtlpNtUserPfns.WndProcsW, WndProcsW, SizeOfWndProcsW);
    RtlCopyMemory((PVOID)RtlpNtUserPfns.WorkerProcs, WorkerProcs, SizeOfWorkerProcs);

    /* Restore original page protection */
    Status = NtProtectVirtualMemory(NtCurrentProcess(),
                                    &BaseAddress,
                                    &RegionSize,
                                    OldProtect,
                                    &OldProtect);
    if (!NT_SUCCESS(Status))
    {
        return Status;
    }

    NtUserPfnsInitialized = TRUE;

    return STATUS_SUCCESS;
}


NTSTATUS
WINAPI
RtlRetrieveNtUserPfn(
    _Out_ LPCVOID* NtdllWndProcsA,
    _Out_ LPCVOID* NtdllWndProcsW,
    _Out_ LPCVOID* NtdllWorkerProcs)
{
    if (NtUserPfnsInitialized == FALSE)
    {
        return STATUS_INVALID_PARAMETER;
    }

    *NtdllWndProcsA = NtDllUserStubs.WndProcsA;
    *NtdllWndProcsW = NtDllUserStubs.WndProcsW;
    *NtdllWorkerProcs = NtDllUserStubs.WorkerProcs;

    return STATUS_SUCCESS;
}
