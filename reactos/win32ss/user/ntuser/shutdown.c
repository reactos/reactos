/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Shutdown routines
 * FILE:             subsystems/win32/win32k/ntuser/shutdown.c
 * PROGRAMER:        Hermes Belusca
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserShutdown);

/*
 * Based on CSRSS and described in pages 1115 - 1118 "Windows Internals, Fifth Edition".
 * CSRSS sends WM_CLIENTSHUTDOWN messages to top-level windows, and it is our job
 * to send WM_QUERYENDSESSION / WM_ENDSESSION messages in response.
 */
LRESULT
IntClientShutdown(IN PWND pWindow,
                  IN WPARAM wParam,
                  IN LPARAM lParam)
{
    LPARAM lParams;
    BOOL KillTimers;
    INT i;
    LRESULT lResult = MCSR_GOODFORSHUTDOWN;
    HWND *List;

    lParams = wParam & (ENDSESSION_LOGOFF|ENDSESSION_CRITICAL|ENDSESSION_CLOSEAPP);
    KillTimers = wParam & MCS_SHUTDOWNTIMERS ? TRUE : FALSE;

    /* First, send end sessions to children */
    List = IntWinListChildren(pWindow);

    if (List)
    {
        for (i = 0; List[i]; i++)
        {
            PWND WndChild;

            if (!(WndChild = UserGetWindowObject(List[i])))
                continue;

            if (wParam & MCS_QUERYENDSESSION)
            {
                if (!co_IntSendMessage(WndChild->head.h, WM_QUERYENDSESSION, 0, lParams))
                {
                    lResult = MCSR_DONOTSHUTDOWN;
                    break;
                }
            }
            else
            {
                co_IntSendMessage(WndChild->head.h, WM_ENDSESSION, KillTimers, lParams);
                if (KillTimers)
                {
                    DestroyTimersForWindow(WndChild->head.pti, WndChild);
                }
                lResult = MCSR_SHUTDOWNFINISHED;
            }
        }
        ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
    }

    if (List && (lResult == MCSR_DONOTSHUTDOWN))
        return lResult;

    /* Send to the caller */
    if (wParam & MCS_QUERYENDSESSION)
    {
        if (!co_IntSendMessage(pWindow->head.h, WM_QUERYENDSESSION, 0, lParams))
        {
            lResult = MCSR_DONOTSHUTDOWN;
        }
    }
    else
    {
        co_IntSendMessage(pWindow->head.h, WM_ENDSESSION, KillTimers, lParams);
        if (KillTimers)
        {
            DestroyTimersForWindow(pWindow->head.pti, pWindow);
        }
        lResult = MCSR_SHUTDOWNFINISHED;
    }

    return lResult;
}


NTSTATUS
GetProcessLuid(IN PETHREAD Thread OPTIONAL,
               OUT PLUID Luid)
{
    NTSTATUS Status;
    PACCESS_TOKEN Token;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    BOOLEAN CopyOnOpen, EffectiveOnly;

    if (Thread == NULL)
        Thread = PsGetCurrentThread();

    /* Use a thread token */
    Token = PsReferenceImpersonationToken(Thread,
                                          &CopyOnOpen,
                                          &EffectiveOnly,
                                          &ImpersonationLevel);
    if (Token == NULL)
    {
        /* We don't have a thread token, use a process token */
        Token = PsReferencePrimaryToken(PsGetThreadProcess(Thread));

        /* If no token, fail */
        if (Token == NULL)
            return STATUS_NO_TOKEN;
    }

    /* Query the LUID */
    Status = SeQueryAuthenticationIdToken(Token, Luid);

    /* Get rid of the token and return */
    ObDereferenceObject(Token);
    return Status;
}

BOOLEAN
HasPrivilege(IN PPRIVILEGE_SET Privilege)
{
    BOOLEAN Result;
    SECURITY_SUBJECT_CONTEXT SubjectContext;

    /* Capture and lock the security subject context */
    SeCaptureSubjectContext(&SubjectContext);
    SeLockSubjectContext(&SubjectContext);

    /* Do privilege check */
    Result = SePrivilegeCheck(Privilege, &SubjectContext, UserMode);

    /* Audit the privilege */
#if 0
    SePrivilegeObjectAuditAlarm(NULL,
                                &SubjectContext,
                                0,
                                Privilege,
                                Result,
                                UserMode);
#endif

    /* Unlock and release the security subject context and return */
    SeUnlockSubjectContext(&SubjectContext);
    SeReleaseSubjectContext(&SubjectContext);
    return Result;
}

NTSTATUS
UserInitiateShutdown(IN PETHREAD Thread,
                     IN OUT PULONG pFlags)
{
    NTSTATUS Status;
    ULONG Flags = *pFlags;
    LUID CallerLuid;
    // LUID SystemLuid = SYSTEM_LUID;
    static PRIVILEGE_SET ShutdownPrivilege =
    {
        1, PRIVILEGE_SET_ALL_NECESSARY,
        { {{SE_SHUTDOWN_PRIVILEGE, 0}, 0} }
    };

    PPROCESSINFO ppi;

    ERR("UserInitiateShutdown\n");

    if(hwndSAS == NULL)
        return STATUS_NOT_FOUND;

    /* Get the caller's LUID */
    Status = GetProcessLuid(Thread, &CallerLuid);
    if (!NT_SUCCESS(Status))
    {
        ERR("GetProcessLuid failed\n");
        return Status;
    }

    // FIXME: Check if this is the System LUID, and adjust flags if needed.
    // if (RtlEqualLuid(&CallerLuid, &SystemLuid)) { Flags = ...; }
    *pFlags = Flags;

    /* Retrieve the Win32 process info */
    ppi = PsGetProcessWin32Process(PsGetThreadProcess(Thread));
    if (ppi == NULL)
        return STATUS_INVALID_HANDLE;

    /* If the caller is not Winlogon, do some security checks */
    if (PsGetThreadProcessId(Thread) != gpidLogon)
    {
        // FIXME: Play again with flags...
        *pFlags = Flags;

        /* Check whether the current process is attached to a window station */
        if (ppi->prpwinsta == NULL)
            return STATUS_INVALID_HANDLE;

        /* Check whether the window station of the current process can send exit requests */
        if (!RtlAreAllAccessesGranted(ppi->amwinsta, WINSTA_EXITWINDOWS))
            return STATUS_ACCESS_DENIED;

        /*
         * NOTE: USERSRV automatically adds the shutdown flag when we poweroff or reboot.
         *
         * If the caller wants to shutdown / reboot / power-off...
         */
        if (Flags & EWX_SHUTDOWN)
        {
            /* ... check whether it has shutdown privilege */
            if (!HasPrivilege(&ShutdownPrivilege))
                return STATUS_PRIVILEGE_NOT_HELD;
        }
        else
        {
            /*
             * ... but if it just wants to log-off, in case its
             * window station is a non-IO one, fail the call.
             */
            if (ppi->prpwinsta->Flags & WSS_NOIO)
                return STATUS_INVALID_DEVICE_REQUEST;
        }
    }

    /* If the caller is not Winlogon, notify it to perform the real shutdown */
    if (PsGetThreadProcessId(Thread) != gpidLogon)
    {
        // FIXME: HACK!! Do more checks!!
        UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_LOGOFF, (LPARAM)Flags);
        return STATUS_PENDING;
    }

    // If we reach this point, that means it's Winlogon that triggered the shutdown.

    /*
     * FIXME:
     * Update and save the shutdown flags globally for renotifying Winlogon
     * if needed, when calling EndShutdown.
     */
    *pFlags = Flags;

    return STATUS_SUCCESS;
}

NTSTATUS
UserEndShutdown(IN PETHREAD Thread,
                IN NTSTATUS ShutdownStatus)
{
    ERR("UserEndShutdown\n");
    STUB;
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
