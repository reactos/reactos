/*
 * PROJECT:     ReactOS Win32k Subsystem
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Job object UI restrictions header
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#pragma once

/* USER side state of a job whose UIRestrictionsClass is non-zero */
typedef struct _JOBINFO
{
    struct _JOBINFO *Next;                  /* Next entry in gJobInfoList */
    PEJOB            pEJob;                 /* The kernel job object we belong to */
    PRTL_ATOM_TABLE  pAtomTable;            /* Private table, for UILIMIT_GLOBALATOMS */
    ULONG            UIRestrictions;        /* JOB_OBJECT_UILIMIT_* */
    ULONG            ProcessCount;          /* Entries used in pProcesses */
    ULONG            ProcessCountMax;       /* Entries allocated in pProcesses */
    PPROCESSINFO    *pProcesses;            /* Processes of this job known to USER */
    ULONG            GrantedHandleCount;    /* Entries used in pGrantedHandles */
    ULONG            GrantedHandleCountMax; /* Entries allocated in pGrantedHandles */
    HANDLE          *pGrantedHandles;       /* USER handles explicitly granted to the job */
} JOBINFO, *PJOBINFO;

NTSTATUS NTAPI Win32kJobCallout(_In_ PWIN32_JOBCALLOUT_PARAMETERS Parameters);

_Requires_exclusive_lock_held_(UserLock)
NTSTATUS FASTCALL IntJobConnectProcess(_In_ PPROCESSINFO ppi);
_Requires_exclusive_lock_held_(UserLock)
VOID FASTCALL IntJobDisconnectProcess(_In_ PPROCESSINFO ppi);

_Requires_exclusive_lock_held_(UserLock)
VOID FASTCALL IntCleanupGrantedHandle(_In_ HANDLE hUserHandle);
