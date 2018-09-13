/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dlluistb.c

Abstract:

    Debug Subsystem DbgUi API Stubs

Author:

    Mark Lucovsky (markl) 23-Jan-1990

Revision History:

--*/

#include "dbgdllp.h"

#define DbgStateChangeSemaphore (NtCurrentTeb()->DbgSsReserved[0])
#define DbgUiApiPort (NtCurrentTeb()->DbgSsReserved[1])

NTSTATUS
DbgUiConnectToDbg( VOID )

/*++

Routine Description:

    This routine makes a connection between the caller and the DbgUi
    port in the Dbg subsystem.  In addition to returning a handle to a
    port object, a handle to a state change semaphore is returned.  This
    semaphore is used in DbgUiWaitStateChange APIs.

Arguments:

    None.

Return Value:

    TBD.

--*/

{
    NTSTATUS st;
    UNICODE_STRING PortName;
    ULONG ConnectionInformationLength;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;

    //
    // if app is already connected, don't reconnect
    //
    st = STATUS_SUCCESS;
    if ( !DbgUiApiPort ) {
        //
        // Set up the security quality of service parameters to use over the
        // port.  Use the most efficient (least overhead) - which is dynamic
        // rather than static tracking.
        //

        DynamicQos.ImpersonationLevel = SecurityImpersonation;
        DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
        DynamicQos.EffectiveOnly = TRUE;


        RtlInitUnicodeString(&PortName,L"\\DbgUiApiPort");
        ConnectionInformationLength = sizeof(DbgStateChangeSemaphore);
        st = NtConnectPort(
                &DbgUiApiPort,
                &PortName,
                &DynamicQos,
                NULL,
                NULL,
                NULL,
                (PVOID)&DbgStateChangeSemaphore,
                &ConnectionInformationLength
                );
        if ( NT_SUCCESS(st) ) {
            NtRegisterThreadTerminatePort(DbgUiApiPort);
        } else {
            DbgUiApiPort = NULL;
        }
    }
    return st;

}

NTSTATUS
DbgUiWaitStateChange (
    OUT PDBGUI_WAIT_STATE_CHANGE StateChange,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

Routine Description:

    This function causes the calling user interface to wait for a
    state change to occur in one of it's application threads. The
    wait is ALERTABLE.

Arguments:

    StateChange - Supplies the address of state change record that
        will contain the state change information.

Return Value:

    TBD

--*/

{
    NTSTATUS st;
    DBGUI_APIMSG ApiMsg;
    PDBGUI_WAIT_STATE_CHANGE args;


    //
    // Wait for a StateChange to occur
    //

top:
    st = NtWaitForSingleObject(DbgStateChangeSemaphore,TRUE,Timeout);
    if ( st != STATUS_SUCCESS ) {
        return st;
    }

    //
    // Pick up the state change
    //

    args = &ApiMsg.u.WaitStateChange;

    DBGUI_FORMAT_API_MSG(ApiMsg,DbgUiWaitStateChangeApi,sizeof(*args));
    st = NtRequestWaitReplyPort(
            DbgUiApiPort,
            (PPORT_MESSAGE) &ApiMsg,
            (PPORT_MESSAGE) &ApiMsg
            );

    if ( NT_SUCCESS(st) ) {
        if ( ApiMsg.ReturnedStatus == DBG_NO_STATE_CHANGE ) {
            DbgPrint("DBGUISTB: Waring No State Change\n");
            goto top;
        }
        *StateChange = *args;
        return ApiMsg.ReturnedStatus;
    } else {
        DbgPrint("NTDLL: DbgUiWaitStateChange failing with status %x\n", st);
        return st;
    }
}

NTSTATUS
DbgUiContinue (
    IN PCLIENT_ID AppClientId,
    IN NTSTATUS ContinueStatus
    )

/*++

Routine Description:

    This function continues an application thread whose state change was
    previously reported through DbgUiWaitStateChange.

Arguments:

    AppClientId - Supplies the address of the ClientId of the
        application thread being continued.  This must be an application
        thread that previously notified the caller through
        DbgUiWaitStateChange but has not yet been continued.

    ContinueStatus - Supplies the continuation status to the thread
        being continued.  valid values for this are:

        DBG_EXCEPTION_HANDLED
        DBG_EXCEPTION_NOT_HANDLED
        DBG_TERMINATE_THREAD
        DBG_TERMINATE_PROCESS
        DBG_CONTINUE

Return Value:

    STATUS_SUCCESS - Successful call to DbgUiContinue

    STATUS_INVALID_CID - An invalid ClientId was specified for the
        AppClientId, or the specified Application was not waiting
        for a continue.

    STATUS_INVALID_PARAMETER - An invalid continue status was specified.

--*/

{
    NTSTATUS st;
    DBGUI_APIMSG ApiMsg;
    PDBGUI_CONTINUE args;


    args = &ApiMsg.u.Continue;
    args->AppClientId = *AppClientId;
    args->ContinueStatus = ContinueStatus;

    DBGUI_FORMAT_API_MSG(ApiMsg,DbgUiContinueApi,sizeof(*args));

    st = NtRequestWaitReplyPort(
            DbgUiApiPort,
            (PPORT_MESSAGE) &ApiMsg,
            (PPORT_MESSAGE) &ApiMsg
            );

    if ( NT_SUCCESS(st) ) {
        if ( !(NT_SUCCESS(ApiMsg.ReturnedStatus))) {
            DbgPrint("NTDLL: DbgUiContinue success with status %x\n", ApiMsg.ReturnedStatus);
        }
        return ApiMsg.ReturnedStatus;
    } else {
        DbgPrint("NTDLL: DbgUiContinue failing with status %x\n", st);
        return st;
    }
}
