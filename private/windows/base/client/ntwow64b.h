/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    ntwow64b.h

Abstract:

    This header contains the fake Nt functions in Win32 Base used WOW64 to call
    into 64 bit code.

Author:

    Michael Zoran (mzoran) 21-Jun-1998

Revision History:

--*/

extern BOOL RunningInWow64;

//
//  csrbeep.c
//
VOID
NTAPI
NtWow64CsrBasepSoundSentryNotification(
    IN ULONG VideoMode
    );

//
//  csrdlini.c
//
NTSTATUS
NTAPI
NtWow64CsrBasepRefreshIniFileMapping(
    IN PUNICODE_STRING BaseFileName
    );

//
//  csrdosdv.c 
//
NTSTATUS
NTAPI
NtWow64CsrBasepDefineDosDevice(
    IN DWORD dwFlags,
    IN PUNICODE_STRING pDeviceName,
    IN PUNICODE_STRING pTargetPath
    );

//
//  csrpathm.c
//
UINT
NTAPI
NtWow64CsrBasepGetTempFile(
    VOID
    );

//
//  csrpro.c
//

NTSTATUS
NtWow64CsrBasepCreateProcess(
    IN PBASE_CREATEPROCESS_MSG a
    );

VOID
NtWow64CsrBasepExitProcess(
    IN UINT uExitCode
    );

NTSTATUS
NtWow64CsrBasepSetProcessShutdownParam(
    IN DWORD dwLevel,
    IN DWORD dwFlags
    );

NTSTATUS
NtWow64CsrBasepGetProcessShutdownParam(
    OUT LPDWORD lpdwLevel,
    OUT LPDWORD lpdwFlags
    );

//
//  csrterm.c
//
NTSTATUS
NtWow64CsrBasepSetTermsrvAppInstallMode(
    IN BOOL bState
    );

//
//  csrthrd.c
//
NTSTATUS
NtWow64CsrBasepCreateThread(
    IN HANDLE ThreadHandle,
    IN CLIENT_ID ClientId
    );

//
//  csrbinit.c
//
NTSTATUS
NtWow64CsrBaseClientConnectToServer(
    IN PWSTR szSessionDir,
    OUT PHANDLE phMutant,
    OUT PBOOLEAN pServerProcess
    );

//
//  csrdebug.c
// 
NTSTATUS
NtWow64CsrBasepDebugProcess(
    IN CLIENT_ID DebuggerClientId,
    IN DWORD dwProcessId,
    IN PVOID AttachCompleteRoutine
    );









    



    


    



    

    
