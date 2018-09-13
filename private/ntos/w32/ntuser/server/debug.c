/****************************** Module Header ******************************\
* Module Name: debug.c
*
* Copyright (c) 1985 - 1999, Microsoft Corporation
*
* This module contains random debugging related functions.
*
* History:
* 17-May-1991 DarrinM   Created.
* 22-Jan-1992 IanJa     ANSI/Unicode neutral (all debug output is ANSI)
\***************************************************************************/

#include "precomp.h"
#pragma hdrstop

extern FARPROC gpfnAttachRoutine;

/**************************************************************************\
* ActivateDebugger
*
* Force an exception on the active application's context so it will break
* into the debugger.
*
* History:
* 05-10-91 DarrinM      Created.
\***************************************************************************/

ULONG SrvActivateDebugger(
    IN OUT PCSR_API_MSG m,
    IN OUT PCSR_REPLY_STATUS ReplyStatus)
{
    PACTIVATEDEBUGGERMSG a = (PACTIVATEDEBUGGERMSG)&m->u.ApiMessageData;
    PCSR_THREAD Thread;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(ReplyStatus);

    /*
     * If the process is CSR, break
     */
    if (a->ClientId.UniqueProcess == NtCurrentTeb()->ClientId.UniqueProcess) {
        DbgBreakPoint();
        return STATUS_SUCCESS;
    }

    /*
     * Impersonate the client if this is a user mode request.
     */
    if (m->h.u2.s2.Type == LPC_REQUEST) {
        if (!CsrImpersonateClient(NULL)) {
            return STATUS_UNSUCCESSFUL;
        }
    }

    /*
     * Lock the client thread
     */
    Status = CsrLockThreadByClientId(a->ClientId.UniqueThread, &Thread);
    if (NT_SUCCESS(Status)) {
        ASSERT(a->ClientId.UniqueProcess == Thread->ClientId.UniqueProcess);

        /*
         * Now that everything is set, rtlremote call to a debug breakpoint.
         * This causes the process to enter the debugger with a breakpoint.
         */
        Status = RtlRemoteCall(
                    Thread->Process->ProcessHandle,
                    Thread->ThreadHandle,
                    (PVOID)gpfnAttachRoutine,
                    0,
                    NULL,
                    TRUE,
                    FALSE
                    );
        UserAssert(NT_SUCCESS(Status));
        Status = NtAlertThread(Thread->ThreadHandle);
        UserAssert(NT_SUCCESS(Status));
        CsrUnlockThread(Thread);
    }

    /*
     * Stop impersonating the client.
     */
    if (m->h.u2.s2.Type == LPC_REQUEST) {
        CsrRevertToSelf();
    }

    return Status;
}


