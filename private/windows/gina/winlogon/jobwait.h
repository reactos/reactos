//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1997.
//
//  File:       jobwait.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    9-11-98   RichardW   Created
//
//----------------------------------------------------------------------------

#ifndef __JOBWAIT_H__
#define __JOBWAIT_H__


typedef struct _WINLOGON_JOB {
    LIST_ENTRY List ;                   // List structure
    LUID UniqueId ;
    LONG RefCount ;                     // Reference Count
    ULONG Flags ;                       // Flags
    HANDLE Job ;                        // Job Object
    HANDLE RootProcess ;                // First process in job
    ULONG Timeout ;                     // Timeout, either absolute or 0xFFFFFFFF
    HANDLE Event ;                      // Event to signal
    LPTHREAD_START_ROUTINE Callback ;   // Function to callback
    PVOID Parameter ;                   // Parameter
} WINLOGON_JOB, * PWINLOGON_JOB ;

#define WINLOGON_JOB_MONITOR_ROOT_PROCESS   0x00000001
#define WINLOGON_JOB_AUTONOMOUS             0x00000002

BOOL
InitializeJobControl(
    VOID
    );


PWINLOGON_JOB
CreateWinlogonJob(
    VOID
    );

BOOL
SetWinlogonJobTimeout(
    PWINLOGON_JOB pJob,
    ULONG Timeout
    );

BOOL
SetWinlogonJobOption(
    PWINLOGON_JOB Job,
    ULONG Options
    );

BOOL
SetJobCallback(
    IN PWINLOGON_JOB Job,
    IN LPTHREAD_START_ROUTINE Callback,
    IN PVOID Parameter
    );

typedef enum {
    ProcessAsUser,
    ProcessAsSystem,
    ProcessAsSystemRestricted
} JOB_PROCESS_TYPE ;

BOOL
StartProcessInJob(
    IN PTERMINAL pTerm,
    IN JOB_PROCESS_TYPE ProcessType,
    IN LPWSTR lpDesktop,
    IN PVOID pEnvironment,
    IN PWSTR lpCmdLine,
    IN DWORD Flags,
    IN DWORD StartFlags,
    IN PWINLOGON_JOB pJob
    );

BOOL
TerminateJob(
    PWINLOGON_JOB Job,
    DWORD ExitCode
    );

BOOL
WaitForJob(
    PWINLOGON_JOB Job,
    DWORD Timeout
    );

BOOL
DeleteJob(
    PWINLOGON_JOB Job
    );

BOOL
IsJobActive(
    PWINLOGON_JOB Job
    );

#endif // __JOBWAIT_H__
