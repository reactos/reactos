/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Test for RtlRetrieveNtUserPfn
 * COPYRIGHT:   Copyright 2026 Timo Kreuzer <timo.kreuzer@reactos.org>
 */

#include "precomp.h"

typedef
NTSTATUS
NTAPI
FN_RtlInitializeNtUserPfn(
    PVOID* ProcsA,
    SIZE_T SizeOfProcsA,
    PVOID* ProcsW,
    SIZE_T SizeOfProcsW,
    PVOID* OtherProcs,
    SIZE_T SizeOfOtherProcs);

FN_RtlInitializeNtUserPfn* pRtlInitializeNtUserPfn;

START_TEST(RtlInitializeNtUserPfn)
{
    HINSTANCE hinstNtdll = GetModuleHandleW(L"ntdll.dll");
    pRtlInitializeNtUserPfn = (FN_RtlInitializeNtUserPfn*)GetProcAddress(hinstNtdll, "RtlInitializeNtUserPfn");
    if (pRtlInitializeNtUserPfn == NULL)
    {
        skip("RtlInitializeNtUserPfn is not available\n");
        return;
    }

    NTSTATUS Status;
    PVOID ProcsA[24] = { 0 };
    PVOID ProcsW[24] = { 0 };
    PVOID OtherProcs[11] = { 0 };
    ULONG Count1, Count2;

    Status = pRtlInitializeNtUserPfn(NULL, 1, NULL, 0, NULL, 0);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlInitializeNtUserPfn(NULL, 0, NULL, 1, NULL, 0);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlInitializeNtUserPfn(NULL, 0, NULL, 0, NULL, 1);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlInitializeNtUserPfn(ProcsA, 0, ProcsW, 0, OtherProcs, 1);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    printf("NTDDI version: %lx\n", GetNTDDIVersion());

    /* OS version specific count of A/W functions */
    Count1 = (GetNTDDIVersion() >= NTDDI_WIN10) ? 24 : 23;

    /* OS version specific count of other functions */
    Count2 = (GetNTDDIVersion() >= NTDDI_WIN11) ? 18 : 11;

    /* Try different variants of too large sizes */
    Status = pRtlInitializeNtUserPfn(ProcsA, (Count1 + 1) * sizeof(PVOID), ProcsW, Count1 * sizeof(PVOID), OtherProcs, Count2 * sizeof(PVOID));
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlInitializeNtUserPfn(ProcsA, Count1 * sizeof(PVOID), ProcsW, (Count1 + 1) * sizeof(PVOID), OtherProcs, Count2 * sizeof(PVOID));
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlInitializeNtUserPfn(ProcsA, (Count1 + 1) * sizeof(PVOID), ProcsW, (Count1 + 1) * sizeof(PVOID), OtherProcs, Count2 * sizeof(PVOID));
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlInitializeNtUserPfn(ProcsA, Count1 * sizeof(PVOID), ProcsW, Count1 * sizeof(PVOID), OtherProcs, (Count2 + 1) * sizeof(PVOID));
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    if (GetNTVersion() < _WIN32_WINNT_WIN10)
    {
        /* Try different variants of too small sizes (these succeed on Windows 10) */
        Status = pRtlInitializeNtUserPfn(ProcsA, (Count1 - 1) * sizeof(PVOID), ProcsW, Count1 * sizeof(PVOID), OtherProcs, Count2 * sizeof(PVOID));
        ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
        Status = pRtlInitializeNtUserPfn(ProcsA, Count1 * sizeof(PVOID), ProcsW, (Count1 - 1) * sizeof(PVOID), OtherProcs, Count2 * sizeof(PVOID));
        ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
        Status = pRtlInitializeNtUserPfn(ProcsA, (Count1 - 1) * sizeof(PVOID), ProcsW, (Count1 - 1) * sizeof(PVOID), OtherProcs, Count2 * sizeof(PVOID));
        ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
        Status = pRtlInitializeNtUserPfn(ProcsA, Count1 * sizeof(PVOID), ProcsW, Count1 * sizeof(PVOID), OtherProcs, (Count2 - 1) * sizeof(PVOID));
        ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    }

    /* Try unaligned sizes */
    Status = pRtlInitializeNtUserPfn(ProcsA, (Count1 * sizeof(PVOID)) - 1, ProcsW, Count1 * sizeof(PVOID), OtherProcs, Count2 * sizeof(PVOID));
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlInitializeNtUserPfn(ProcsA, Count1 * sizeof(PVOID), ProcsW, (Count1 * sizeof(PVOID)) - 1, OtherProcs, Count2 * sizeof(PVOID));
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
    Status = pRtlInitializeNtUserPfn(ProcsA, Count1 * sizeof(PVOID), ProcsW, Count1 * sizeof(PVOID), OtherProcs, (Count2 * sizeof(PVOID)) - 1);
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);

    /* Pass the expected sizes */
    Status = pRtlInitializeNtUserPfn(ProcsA, Count1 * sizeof(PVOID), ProcsW, Count1 * sizeof(PVOID), OtherProcs, Count2 * sizeof(PVOID));
    ok_ntstatus(Status, STATUS_SUCCESS);

    /* Try to initialize again */
    Status = pRtlInitializeNtUserPfn(ProcsA, Count1 * sizeof(PVOID), ProcsW, Count1 * sizeof(PVOID), OtherProcs, Count2 * sizeof(PVOID));
    ok_ntstatus(Status, STATUS_INVALID_PARAMETER);
}
