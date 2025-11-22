/*
 * PROJECT:     ReactOS Win32 Base API
 * LICENSE:     GPL-2.0-or-later (https://spdx.org/licenses/GPL-2.0-or-later)
 * PURPOSE:     One-Time initialization API
 * COPYRIGHT:   Copyright 2023 Ratin Gao <ratin@knsoft.org>
 */

#include "k32_vista.h"

BOOL
WINAPI
InitOnceExecuteOnce(
    _Inout_ PINIT_ONCE InitOnce,
    _In_ __callback PINIT_ONCE_FN InitFn,
    _Inout_opt_ PVOID Parameter,
    _Outptr_opt_result_maybenull_ LPVOID *Context)
{
    return NT_SUCCESS(RtlRunOnceExecuteOnce(InitOnce,
                                            (PRTL_RUN_ONCE_INIT_FN)InitFn,
                                            Parameter,
                                            Context));
}

BOOL
WINAPI
InitOnceBeginInitialize(
    _Inout_ LPINIT_ONCE lpInitOnce,
    _In_ DWORD dwFlags,
    _Out_ PBOOL fPending,
    _Outptr_opt_result_maybenull_ LPVOID *lpContext)
{
    NTSTATUS Status;

    Status = RtlRunOnceBeginInitialize(lpInitOnce, dwFlags, lpContext);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *fPending = (Status == STATUS_PENDING);
    return TRUE;
}

BOOL
WINAPI
InitOnceComplete(
    _Inout_ LPINIT_ONCE lpInitOnce,
    _In_ DWORD dwFlags,
    _In_opt_ LPVOID lpContext)
{
    NTSTATUS Status;

    Status = RtlRunOnceComplete(lpInitOnce, dwFlags, lpContext);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}
