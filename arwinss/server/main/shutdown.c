/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            server/main/misc.c
 * PURPOSE:         Shutdown code
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

//#define NDEBUG
#include <debug.h>

#include <ntstatus.h>

/* Our local copy of shutdown flags */
static ULONG gdwShutdownFlags = 0;

HWND hwndSAS = NULL;

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

BOOL
NotifyLogon(IN HWND hWndSta,
            IN PLUID CallerLuid,
            IN ULONG Flags,
            IN NTSTATUS ShutdownStatus)
{
    // LUID SystemLuid = SYSTEM_LUID;
    ULONG Notif, Param;

    DPRINT("NotifyLogon(0x%lx, 0x%lx)\n", Flags, ShutdownStatus);

    /* If no Winlogon notifications are needed, just return */
    if (Flags & EWX_NONOTIFY)
        return FALSE;

    /* In case we cancelled the shutdown...*/
    if (Flags & EWX_SHUTDOWN_CANCELED)
    {
        /* ... send a LN_LOGOFF_CANCELED to Winlogon with the real cancel status... */
        Notif = LN_LOGOFF_CANCELED;
        Param = ShutdownStatus;
    }
    else
    {
        /* ... otherwise it's a real logoff. Send the shutdown flags in that case. */
        Notif = LN_LOGOFF;
        Param = Flags;
    }

    // FIXME: At the moment, always send the notifications... In real world some checks are done.
    // if (hwndSAS && ( (Flags & EWX_SHUTDOWN) || RtlEqualLuid(CallerLuid, &SystemLuid)) )
    if (hwndSAS)
    {
        DPRINT("\tSending %s, Param 0x%x message to Winlogon\n", Notif == LN_LOGOFF ? "LN_LOGOFF" : "LN_LOGOFF_CANCELED", Param);
        //UserPostMessage(hwndSAS, WM_LOGONNOTIFY, Notif, (LPARAM)Param);
        UNIMPLEMENTED;
        return TRUE;
    }
    else
    {
        DPRINT1("hwndSAS == NULL\n");
    }

    return FALSE;
}


NTSTATUS
UserInitiateShutdown(IN PETHREAD Thread,
                     IN OUT PULONG pFlags)
{
    NTSTATUS Status;
    ULONG Flags = *pFlags;
    LUID CallerLuid;
    LUID SystemLuid = SYSTEM_LUID;
    /*static PRIVILEGE_SET ShutdownPrivilege =
    {
        1, PRIVILEGE_SET_ALL_NECESSARY,
        { {{SE_SHUTDOWN_PRIVILEGE, 0}, 0} }
    };*/

    PPROCESSINFO ppi;

    DPRINT("UserInitiateShutdown\n");

    /* Get the caller's LUID */
    Status = GetProcessLuid(Thread, &CallerLuid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetProcessLuid failed\n");
        return Status;
    }

    /*
     * Check if this is the System LUID, and adjust flags if needed.
     * In particular, be sure there is no EWX_CALLER_SYSTEM flag
     * spuriously set (could be the sign of malicous app!).
     */
    if (RtlEqualLuid(&CallerLuid, &SystemLuid))
        Flags |= EWX_CALLER_SYSTEM;
    else
        Flags &= ~EWX_CALLER_SYSTEM;

    *pFlags = Flags;

    /* Retrieve the Win32 process info */
    ppi = PsGetProcessWin32Process(PsGetThreadProcess(Thread));
    if (ppi == NULL)
        return STATUS_INVALID_HANDLE;

#if 0
    /* If the caller is not Winlogon, do some security checks */
    if (PsGetThreadProcessId(Thread) != gpidLogon)
    {
        /*
         * Here also, be sure there is no EWX_CALLER_WINLOGON flag
         * spuriously set (could be the sign of malicous app!).
         */
        Flags &= ~EWX_CALLER_WINLOGON;

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

    /* If the caller is not Winlogon, possibly notify it to perform the real shutdown */
    if (PsGetThreadProcessId(Thread) != gpidLogon)
    {
        // FIXME: HACK!! Do more checks!!
        ERR("UserInitiateShutdown -- Notify Winlogon for shutdown\n");
        NotifyLogon(hwndSAS, &CallerLuid, Flags, STATUS_SUCCESS);
        return STATUS_PENDING;
    }

    // If we reach this point, that means it's Winlogon that triggered the shutdown.
    ERR("UserInitiateShutdown -- Winlogon is doing a shutdown\n");

    /*
     * Update and save the shutdown flags globally for renotifying
     * Winlogon if needed, when calling EndShutdown.
     */
    Flags |= EWX_CALLER_WINLOGON; // Winlogon is doing a shutdown, be sure the internal flag is set.
    *pFlags = Flags;

    /* Save the shutdown flags now */
    gdwShutdownFlags = Flags;
#else
    UNIMPLEMENTED;
#endif
    return STATUS_SUCCESS;
}

NTSTATUS
UserEndShutdown(IN PETHREAD Thread,
                IN NTSTATUS ShutdownStatus)
{
    NTSTATUS Status;
    ULONG Flags;
    LUID CallerLuid;

    DPRINT("UserEndShutdown\n");

    /*
     * FIXME: Some cleanup should be done when shutdown succeeds,
     * and some reset should be done when shutdown is cancelled.
     */
    Status = GetProcessLuid(Thread, &CallerLuid);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("GetProcessLuid failed\n");
        return Status;
    }

    /* Copy the global flags because we're going to modify them for our purposes */
    Flags = gdwShutdownFlags;

    if (NT_SUCCESS(ShutdownStatus))
    {
        /* Just report success, and keep the shutdown flags as they are */
        ShutdownStatus = STATUS_SUCCESS;
    }
    else
    {
        /* Report the status to Winlogon and say that we cancel the shutdown */
        Flags |= EWX_SHUTDOWN_CANCELED;
        // FIXME: Should we reset gdwShutdownFlags to 0 ??
    }

    DPRINT("UserEndShutdown -- Notify Winlogon for end of shutdown\n");

#if 0
    NotifyLogon(hwndSAS, &CallerLuid, Flags, ShutdownStatus);
#else
    UNIMPLEMENTED
#endif

    /* Always return success */
    return STATUS_SUCCESS;
}

/* EOF */
