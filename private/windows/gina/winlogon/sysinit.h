//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       sysinit.h
//
//  Contents:
//
//  Classes:
//
//  Functions:
//
//  History:    12-05-94   RichardW   Created
//
//----------------------------------------------------------------------------

VOID
DealWithAutochkLogs(
    VOID
    );

HANDLE
StartLoadingFonts(void);

BOOL InitSystemFontInfo(void);

BOOL SetProcessPriority(
    VOID
    );


#define MAX_SESSION_PATH   256

NTSTATUS
SetWinlogonDeviceMap( ULONG SessionId );

void InitializeSound(void);

VOID CreateTemporaryPageFile();

VOID CreateNetworkProviderEvent(
    VOID
    );


BOOL
StartSystemProcess(
    PWSTR   pszCommandLine,
    PWSTR   pszDesktop,
    DWORD   Flags,
    DWORD   StartupFlags,
    PVOID   pEnvironment,
    DWORD   WlFlags,
    HANDLE *phProcess,
    HANDLE *phThread
    );

#define START_SAVE_HANDLE       0x00000001
#define START_SYSTEM_SERVICE    0x00000002


BOOL
ExecSystemProcesses(
    VOID
    );

BOOL
WaitForSystemProcesses(
    VOID);

extern BOOLEAN  PageFilePopup;
