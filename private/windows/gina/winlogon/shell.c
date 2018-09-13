//+---------------------------------------------------------------------------
//
//  Microsoft Windows
//  Copyright (C) Microsoft Corporation, 1992 - 1993.
//
//  File:       shell.c
//
//  Contents:   Microsoft Logon GUI DLL
//
//  History:    7-14-94   RichardW   Created
//
//----------------------------------------------------------------------------

#include "precomp.h"
#pragma hdrstop


BOOL
SetProcessQuotas(
    PPROCESS_INFORMATION ProcessInformation,
    PUSER_PROCESS_DATA UserProcessData
    )
{
    NTSTATUS Status = STATUS_SUCCESS;
    BOOL Result;
    QUOTA_LIMITS RequestedLimits;

    RequestedLimits = UserProcessData->Quotas;
    RequestedLimits.MinimumWorkingSetSize = 0;
    RequestedLimits.MaximumWorkingSetSize = 0;

    if (UserProcessData->Quotas.PagedPoolLimit != 0) {

        Result = EnablePrivilege(SE_INCREASE_QUOTA_PRIVILEGE, TRUE);
        if (!Result) {
            DebugLog((DEB_ERROR, "failed to enable increase_quota privilege\n"));
            return(FALSE);
        }

        Status = NtSetInformationProcess(
                    ProcessInformation->hProcess,
                    ProcessQuotaLimits,
                    (PVOID)&RequestedLimits,
                    (ULONG)sizeof(QUOTA_LIMITS)
                    );

        Result = EnablePrivilege(SE_INCREASE_QUOTA_PRIVILEGE, FALSE);
        if (!Result) {
            DebugLog((DEB_ERROR, "failed to disable increase_quota privilege\n"));
        }
    }


#if DBG
    if (!NT_SUCCESS(Status)) {
        DebugLog((DEB_ERROR, "SetProcessQuotas failed. Status: 0x%lx\n", Status));
    }
#endif //DBG

    return (NT_SUCCESS(Status));
}


BOOL
ExecApplication(
    IN LPTSTR    pch,
    IN LPTSTR    Desktop,
    IN PTERMINAL pTerm,
    IN PVOID    pEnvironment,
    IN DWORD    Flags,
    IN DWORD    StartupFlags,
    OUT PPROCESS_INFORMATION ProcessInformation
    )
{
    STARTUPINFO si;
    BOOL Result, IgnoreResult;
    HANDLE ImpersonationHandle;


    //
    // Initialize process startup info
    //
    si.cb = sizeof(STARTUPINFO);
    si.lpReserved = pch;
    si.lpTitle = pch;
    si.dwX = si.dwY = si.dwXSize = si.dwYSize = 0L;
    si.dwFlags = StartupFlags;
    si.wShowWindow = SW_SHOW;   // at least let the guy see it
    si.lpReserved2 = NULL;
    si.cbReserved2 = 0;
    si.lpDesktop = Desktop;

    //
    // Impersonate the user so we get access checked correctly on
    // the file we're trying to execute
    //

    ImpersonationHandle = ImpersonateUser(&pTerm->pWinStaWinlogon->UserProcessData, NULL);
    if (ImpersonationHandle == NULL) {
        DebugLog((DEB_ERROR, "ExecApplication failed to impersonate user\n"));
        return(FALSE);
    }


    //
    // Create the app suspended
    //
    DebugLog((DEB_TRACE, "About to create process of %ws, on desktop %ws\n", pch, Desktop));
    Result = CreateProcessAsUser(
                      pTerm->pWinStaWinlogon->UserProcessData.UserToken,
                      NULL,
                      pch,
                      NULL,
                      NULL,
                      FALSE,
                      Flags | CREATE_SUSPENDED | CREATE_UNICODE_ENVIRONMENT,
                      pEnvironment,
                      NULL,
                      &si,
                      ProcessInformation);


    IgnoreResult = StopImpersonating(ImpersonationHandle);
    ASSERT(IgnoreResult);

    return(Result);

}



BOOL
WINAPI
WlxStartApplication(
    PVOID                   pWlxContext,
    PWSTR                   pszDesktop,
    PVOID                   pEnvironment,
    PWSTR                   pszCmdLine
    )
{
    PROCESS_INFORMATION ProcessInformation;
    BOOL   bExec;

    bExec = ExecApplication (pszCmdLine,
                             pszDesktop,
                             g_pTerminals,
                             pEnvironment,
                             0,
                             STARTF_USESHOWWINDOW,
                             &ProcessInformation);

    if (!bExec) {
        return(FALSE);
    }

    if (SetProcessQuotas(&ProcessInformation,
                         &g_pTerminals->pWinStaWinlogon->UserProcessData)) {
        ResumeThread(ProcessInformation.hThread);

    } else {
        TerminateProcess(ProcessInformation.hProcess,
                        ERROR_ACCESS_DENIED);
    }


    CloseHandle(ProcessInformation.hThread);
    CloseHandle(ProcessInformation.hProcess);

    return(TRUE);
}
