/*
 * PROJECT:     ReactOS Win32 Base API
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Threadpool Calls
 * COPYRIGHT:   Copyright 2025 Justin Miller <justin.miller@reactos.org>
 */

#include "k32_vista.h"

NTSTATUS WINAPI TpAllocWork( TP_WORK **out, PTP_WORK_CALLBACK callback, PVOID userdata,
                             TP_CALLBACK_ENVIRON *environment );
NTSTATUS WINAPI TpSimpleTryPost( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                 TP_CALLBACK_ENVIRON *environment );
/***********************************************************************
 *           CreateThreadpoolWork   (kernelbase.@)
 */
PTP_WORK WINAPI CreateThreadpoolWork( PTP_WORK_CALLBACK callback, PVOID userdata,
                                      TP_CALLBACK_ENVIRON *environment )
{
    TP_WORK *work;
    NTSTATUS status;
    status =  TpAllocWork( &work, callback, userdata, environment );
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
    }
    return work;
}

/***********************************************************************
 *           TrySubmitThreadpoolCallback   (kernelbase.@)
 */
BOOL WINAPI TrySubmitThreadpoolCallback( PTP_SIMPLE_CALLBACK callback, PVOID userdata,
                                        TP_CALLBACK_ENVIRON *environment )
{

    NTSTATUS status;
    status =  TpSimpleTryPost( callback, userdata, environment );
    if (!NT_SUCCESS(status))
    {
        SetLastError(RtlNtStatusToDosError(status));
        return FALSE;
    }
    return TRUE;
}

