/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            server/main/misc.c
 * PURPOSE:         Shutdown code
 * PROGRAMMERS:     Aleksey Bragin (aleksey@reactos.org)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#include <ntstatus.h>

#include "object.h"
#include "request.h"
#include "user.h"

#define NDEBUG
#include <debug.h>

/* Our local copy of shutdown flags */
static ULONG gdwShutdownFlags = 0;

/* Registered logon window */
HWND hwndSAS = NULL;

extern HANDLE gpidLogon;

VOID
UserPostMessage(HWND hWnd,
                UINT Msg,
                WPARAM wParam,
                LPARAM lParam)
{
    PTHREADINFO thread;
    struct send_message_request req;
    struct send_message_reply reply;

    // Get that window's thread
    thread = get_window_thread((user_handle_t)hWnd);
    if (!thread) return;

    req.id      = (ULONG)thread->peThread->Cid.UniqueThread;
    req.type    = MSG_NOTIFY; // Or MSG_OTHER_PROCESS ?
    req.flags   = 0;
    req.win     = (user_handle_t)hWnd;
    req.msg     = Msg;
    req.wparam  = wParam;
    req.lparam  = lParam;
    req.timeout = TIMEOUT_INFINITE;

    // No strings attached
    req.__header.request_size = 0;

    // Zero reply's memory area
    memset( &reply, 0, sizeof(reply) );

    req_send_message(&req, &reply);
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
        UserPostMessage(hwndSAS, WM_LOGONNOTIFY, Notif, (LPARAM)Param);
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

    /* If the caller is not Winlogon, do some security checks and notify it to perform the real shutdown */
    if (PsGetThreadProcessId(Thread) != gpidLogon)
    {
        /*
         * Here also, be sure there is no EWX_CALLER_WINLOGON flag
         * spuriously set (could be the sign of malicous app!).
         */
        Flags &= ~EWX_CALLER_WINLOGON;

        *pFlags = Flags;

        // FIXME: HACK!! Do more checks!!

        /* NOTE: USERSRV automatically adds the shutdown flag when we poweroff or reboot. */
        DPRINT("UserInitiateShutdown -- Notify Winlogon for shutdown\n");
        NotifyLogon(hwndSAS, &CallerLuid, Flags, STATUS_SUCCESS);
        return STATUS_PENDING;
    }

    // If we reach this point, that means it's Winlogon that triggered the shutdown.
    DPRINT("UserInitiateShutdown -- Winlogon is doing a shutdown\n");

    /*
     * Update and save the shutdown flags globally for renotifying
     * Winlogon if needed, when calling EndShutdown.
     */
    Flags |= EWX_CALLER_WINLOGON; // Winlogon is doing a shutdown, be sure the internal flag is set.
    *pFlags = Flags;

    /* Save the shutdown flags now */
    gdwShutdownFlags = Flags;

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

    NotifyLogon(hwndSAS, &CallerLuid, Flags, ShutdownStatus);

    /* Always return success */
    return STATUS_SUCCESS;
}

/* EOF */
