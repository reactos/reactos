/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    dllssstb.c

Abstract:

    Debug Subsystem DbgSs API Stubs

Author:

    Mark Lucovsky (markl) 22-Jan-1990

Revision History:

--*/

#include "dbgdllp.h"
#include "ldrp.h"

NTSTATUS
DbgSspConnectToDbg( VOID )

/*++

Routine Description:

    This routine makes a connection between the caller and the
    DbgSs port in the Dbg subsystem.

Arguments:

    None.

Return Value:

    TBD.

--*/

{
    NTSTATUS st;
    UNICODE_STRING PortName;
    SECURITY_QUALITY_OF_SERVICE DynamicQos;

    //
    // Set up the security quality of service parameters to use over the
    // port.  Use the most efficient (least overhead) - which is dynamic
    // rather than static tracking.
    //

    DynamicQos.ImpersonationLevel = SecurityImpersonation;
    DynamicQos.ContextTrackingMode = SECURITY_DYNAMIC_TRACKING;
    DynamicQos.EffectiveOnly = TRUE;


    RtlInitUnicodeString(&PortName,L"\\DbgSsApiPort");
    st = NtConnectPort(
            &DbgSspApiPort,
            &PortName,
            &DynamicQos,
            NULL,
            NULL,
            NULL,
            NULL,
            0L
            );

    return st;

}

NTSTATUS
DbgSspException (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PDBGKM_EXCEPTION Exception
    )

/*++

Routine Description:

    This function is called by emulation subsystems to report that
    an exception occured in a thread.

Arguments:

    ContinueKey - Supplies the captured DbgKm API message that needs a reply
        should this API complete successfully.

    AppClientId - Supplies the address of the ClientId of the thread that
        encountered an exception.

    Exception - Supplies the address of the Exception message as sent
        through the applications DebugPort.

Return Value:

    TBD

--*/

{

    NTSTATUS st;
    DBGSS_APIMSG ApiMsg;

    PDBGKM_EXCEPTION args;

    args = &ApiMsg.u.Exception;

    DBGSS_FORMAT_API_MSG(ApiMsg,DbgSsExceptionApi,sizeof(*args),AppClientId,ContinueKey);

    *args = *Exception;

    st = NtRequestPort(DbgSspApiPort, (PPORT_MESSAGE) &ApiMsg);

    return st;
}

NTSTATUS
DbgSspCreateThread (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PDBGKM_CREATE_THREAD NewThread
    )

/*++

Routine Description:

    This function is called by emulation subsystems to report the
    creation of a new thread to the Dbg subsystem.

Arguments:

    ContinueKey - Supplies the captured DbgKm API message that needs a reply
        should this API complete successfully.

    AppClientId - Supplies the address of the new thread's ClientId.

    NewThread - Supplies the address of the NewThread message as sent through
        the applications DebugPort. The calling subsystem may modify the
        SubSystemKey field of this message.

Return Value:

    TBD

--*/

{

    NTSTATUS st;
    DBGSS_APIMSG ApiMsg;

    PDBGKM_CREATE_THREAD args;

    args = &ApiMsg.u.CreateThread;

    DBGSS_FORMAT_API_MSG(ApiMsg,DbgSsCreateThreadApi,sizeof(*args),AppClientId, ContinueKey);

    *args = *NewThread;

    st = NtRequestPort(DbgSspApiPort, (PPORT_MESSAGE) &ApiMsg);

    return st;
}


NTSTATUS
DbgSspCreateProcess (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PCLIENT_ID DebugUiClientId,
    IN PDBGKM_CREATE_PROCESS NewProcess
    )

/*++

Routine Description:

    This function is called by emulation subsystems to report the
    creation of a new process to the Dbg subsystem.  It is the
    responsibility of individual subsystems to track a process and its
    controlling DebugUi.

Arguments:

    ContinueKey - Supplies the captured DbgKm API message that needs a reply
        should this API complete successfully.

    AppClientId - Supplies the address of the new thread's ClientId.

    DebugUiClientId - Supplies the address of the ClientId of the user
        interface controlling the new thread.

    NewProcess - Supplies the address of the NewProcess message as sent through
        the applications DebugPort. The calling subsystem may modify the
        SubSystemKey fields of this message.

Return Value:

    TBD

--*/

{

    NTSTATUS st;
    DBGSS_APIMSG ApiMsg;

    PDBGSS_CREATE_PROCESS args;

    args = &ApiMsg.u.CreateProcessInfo;

    DBGSS_FORMAT_API_MSG(ApiMsg,DbgSsCreateProcessApi,sizeof(*args),AppClientId,ContinueKey);

    args->DebugUiClientId = *DebugUiClientId;
    args->NewProcess = *NewProcess;

    st = NtRequestPort(DbgSspApiPort, (PPORT_MESSAGE) &ApiMsg);

    return st;

}

NTSTATUS
DbgSspExitThread (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PDBGKM_EXIT_THREAD ExitThread
    )

/*++

Routine Description:

    This function is called by emulation subsystems to report that
    a thread is exiting.

Arguments:

    ContinueKey - Supplies the captured DbgKm API message that needs a reply
        should this API complete successfully.

    AppClientId - Supplies the address of the exiting thread's ClientId.

    ExitThread - Supplies the address of the ExitThread message as sent
        through the applications DebugPort.

Return Value:

    TBD

--*/

{

    NTSTATUS st;
    DBGSS_APIMSG ApiMsg;

    PDBGKM_EXIT_THREAD args;

    args = &ApiMsg.u.ExitThread;

    DBGSS_FORMAT_API_MSG(ApiMsg,DbgSsExitThreadApi,sizeof(*args),AppClientId,ContinueKey);

    *args = *ExitThread;

    st = NtRequestPort(DbgSspApiPort, (PPORT_MESSAGE) &ApiMsg);

    return st;
}

NTSTATUS
DbgSspExitProcess (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PDBGKM_EXIT_PROCESS ExitProcess
    )

/*++

Routine Description:

    This function is called by emulation subsystems to report that
    a process is exiting.

Arguments:

    ContinueKey - Supplies the captured DbgKm API message that needs a reply
        should this API complete successfully.

    AppClientId - Supplies the address of the ClientId of the last thread
        in the process to exit.

    ExitProcess - Supplies the address of the ExitProcess message as sent
        through the applications DebugPort.

Return Value:

    TBD

--*/

{

    NTSTATUS st;
    DBGSS_APIMSG ApiMsg;

    PDBGKM_EXIT_PROCESS args;

    args = &ApiMsg.u.ExitProcess;

    DBGSS_FORMAT_API_MSG(ApiMsg,DbgSsExitProcessApi,sizeof(*args),AppClientId,ContinueKey);

    *args = *ExitProcess;

    st = NtRequestPort(DbgSspApiPort, (PPORT_MESSAGE) &ApiMsg);

    return st;
}

NTSTATUS
DbgSspLoadDll (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PDBGKM_LOAD_DLL LoadDll
    )

/*++

Routine Description:

    This function is called by emulation subsystems to report that
    a a process has loaded a DLL

Arguments:

    ContinueKey - Supplies the captured DbgKm API message that needs a reply
        should this API complete successfully.

    AppClientId - Supplies the address of the ClientId of the thread
        that mapped the section.

    LoadDll - Supplies the address of the LoadDll message as sent
        through the applications DebugPort.

Return Value:

    TBD

--*/

{

    NTSTATUS st;
    DBGSS_APIMSG ApiMsg;

    PDBGKM_LOAD_DLL args;

    args = &ApiMsg.u.LoadDll;

    DBGSS_FORMAT_API_MSG(ApiMsg,DbgSsLoadDllApi,sizeof(*args),AppClientId,ContinueKey);

    *args = *LoadDll;

    st = NtRequestPort(DbgSspApiPort, (PPORT_MESSAGE) &ApiMsg);

    return st;
}

NTSTATUS
DbgSspUnloadDll (
    IN PDBGSS_CONTINUE_KEY ContinueKey,
    IN PCLIENT_ID AppClientId,
    IN PDBGKM_UNLOAD_DLL UnloadDll
    )

/*++

Routine Description:

    This function is called by emulation subsystems to report that
    a a process has un-mapped a view of a section

Arguments:

    ContinueKey - Supplies the captured DbgKm API message that needs a reply
        should this API complete successfully.

    AppClientId - Supplies the address of the ClientId of the thread
        that mapped the section.

    UnloadDll - Supplies the address of the UnloadDll message as sent
        through the applications DebugPort.

Return Value:

    TBD

--*/

{

    NTSTATUS st;
    DBGSS_APIMSG ApiMsg;

    PDBGKM_UNLOAD_DLL args;

    args = &ApiMsg.u.UnloadDll;

    DBGSS_FORMAT_API_MSG(ApiMsg,DbgSsUnloadDllApi,sizeof(*args),AppClientId,ContinueKey);

    *args = *UnloadDll;

    st = NtRequestPort(DbgSspApiPort, (PPORT_MESSAGE) &ApiMsg);

    return st;
}


NTSTATUS
DbgSsInitialize(
    IN HANDLE KmReplyPort,
    IN PDBGSS_UI_LOOKUP UiLookUpRoutine,
    IN PDBGSS_SUBSYSTEMKEY_LOOKUP SubsystemKeyLookupRoutine OPTIONAL,
    IN PDBGSS_DBGKM_APIMSG_FILTER KmApiMsgFilter OPTIONAL
    )

/*++

Routine Description:

    This function is called by a subsystem to initialize
    portions of the debug subsystem dll. The main purpose of
    this function is to set up callouts that are needed in order
    to use DbgSsHandleKmApiMsg, and to connect to the debug server.

Arguments:

    KmReplyPort - Supplies a handle to the port that the subsystem
        receives DbgKm API messages on.

    UiLookupRoutine - Supplies the address of a function that will
        be called upon receipt of a process creation message. The
        purpose of this function is to identify the client id of the
        debug ui controlling the process.

    SubsystemKeyLookupRoutine - Supplies the address of a function that
        will be called upon receipt of process creation and thread
        creation messages.  The purpose of this function is to allow a
        subsystem to correlate a key value with a given process or
        thread.

    KmApiMsgFilter - Supplies the address of a function that will be
        called upon receipt of a DbgKm Api message. This function can
        take any action. If it returns any value other than DBG_CONTINUE,
        DbgSsHandleKmApiMsg will not process the message. This function
        is called before any other call outs occur.

Return Value:

    Return code of DbgSsConnectToDbg.

--*/

{
    NTSTATUS st;

    st = DbgSspConnectToDbg();

    if (NT_SUCCESS(st)) {
        DbgSspKmReplyPort = KmReplyPort;
        DbgSspUiLookUpRoutine = UiLookUpRoutine;
        DbgSspSubsystemKeyLookupRoutine = SubsystemKeyLookupRoutine;
        DbgSspKmApiMsgFilter = KmApiMsgFilter;

        st = RtlCreateUserThread(
                NtCurrentProcess(),
                NULL,
                FALSE,
                0L,
                0L,
                0L,
                DbgSspSrvApiLoop,
                NULL,
                NULL,
                NULL
                );
        ASSERT( NT_SUCCESS(st) );
    }

    return st;
}


#if DBG
PSZ DbgpKmApiName[ DbgKmMaxApiNumber+1 ] = {
    "DbgKmException",
    "DbgKmCreateThread",
    "DbgKmCreateProcess",
    "DbgKmExitThread",
    "DbgKmExitProcess",
    "DbgKmLoadDll",
    "DbgKmUnloadDll",
    "Unknown DbgKm Api Number"
};
#endif // DBG

VOID
DbgSsHandleKmApiMsg(
    IN PDBGKM_APIMSG ApiMsg,
    IN HANDLE ReplyEvent OPTIONAL
    )

/*++

Routine Description:

    This function is called by a subsystem upon receipt of a message whose
    type is LPC_DEBUG_EVENT. The purpose of this function is to propagate
    the debug event message to the debug server.

    A number of callouts are performed prior to propagating the message:

      - For all messages, the KmApiMsgFilter is called (if it was
        supplied during DbgSsInitialize.  If this returns anything other
        that DBG_CONTINUE, the message is not propagated by this
        function.  The caller is responsible for event propagation, and
        for replying to the thread reporting the debug event.

      - For create process messages, the UiLookupRoutine is called.  If
        a success code is returned than message is propagated.
        Otherwise, a reply is generated to the thread reporting the
        debug event.

     -  For create process and create thread messages,
        SubsystemKeyLookupRoutine is called.  Failure does not effect
        propagation.  It simply inhibits the update of messages
        SubSystemKey field.

    Based on the debug event type, a DBGSS_APIMSG is formated and sent
    as a datagram to the debug server.

Arguments:

    ApiMsg - Supplies the debug event message to propagate to the debug
        server.

    ReplyEvent - An optional parameter, that if specified supplies a handle
        to an event that is to be signaled rather that generating a reply
        to this message.

Return Value:

    None.

--*/


{
    NTSTATUS st;
    CLIENT_ID DebugUiClientId;
    ULONG SubsystemKey;
    PDBGSS_CONTINUE_KEY ContinueKey;

    ApiMsg->ReturnedStatus = STATUS_PENDING;

#if DBG && 0
    if (ApiMsg->ApiNumber >= DbgKmMaxApiNumber ) {
        ApiMsg->ApiNumber = DbgKmMaxApiNumber;
    }
    DbgPrint( "DBG:  %s Api Request received from %lx.%lx\n",
              DbgpKmApiName[ ApiMsg->ApiNumber ],
              ApiMsg->h.ClientId.UniqueProcess,
              ApiMsg->h.ClientId.UniqueThread
           );
#endif // DBG

    if (DbgSspKmApiMsgFilter) {
        if ( (DbgSspKmApiMsgFilter)(ApiMsg) != DBG_CONTINUE ) {
            return;
        }
    }

    ContinueKey = (PDBGSS_CONTINUE_KEY) RtlAllocateHeap(RtlProcessHeap(), MAKE_TAG( DBG_TAG ), sizeof(*ContinueKey));
    if ( !ContinueKey ) {
        ApiMsg->ReturnedStatus = STATUS_NO_MEMORY;
        if ( ARGUMENT_PRESENT(ReplyEvent) ) {
            st = NtSetEvent(ReplyEvent,NULL);
        } else {
            st = NtReplyPort(DbgSspKmReplyPort,
                             (PPORT_MESSAGE)ApiMsg
                            );
        }
        ASSERT(NT_SUCCESS(st));
        return;
    }
    ContinueKey->KmApiMsg = *ApiMsg;

    ContinueKey->ReplyEvent = ReplyEvent;

    switch (ApiMsg->ApiNumber) {

    case DbgKmExceptionApi :
        st = DbgSspException(
                ContinueKey,
                &ApiMsg->h.ClientId,
                &ApiMsg->u.Exception
                );

        break;

    case DbgKmCreateThreadApi :

        if ( DbgSspSubsystemKeyLookupRoutine ) {

            st = (DbgSspSubsystemKeyLookupRoutine)(
                    &ApiMsg->h.ClientId,
                    &SubsystemKey,
                    FALSE
                    );

            if ( NT_SUCCESS(st) ) {
                ApiMsg->u.CreateThread.SubSystemKey = SubsystemKey;
            }
        }

        st = DbgSspCreateThread(
                ContinueKey,
                &ApiMsg->h.ClientId,
                &ApiMsg->u.CreateThread
                );

        break;

    case DbgKmCreateProcessApi :

        st = (DbgSspUiLookUpRoutine)(
                &ApiMsg->h.ClientId,
                &DebugUiClientId
                );
        if ( !NT_SUCCESS(st) ) {
            break;
        }

        if ( DbgSspSubsystemKeyLookupRoutine ) {

            st = (DbgSspSubsystemKeyLookupRoutine)(
                    &ApiMsg->h.ClientId,
                    &SubsystemKey,
                    TRUE
                    );

            if ( NT_SUCCESS(st) ) {
                ApiMsg->u.CreateProcessInfo.SubSystemKey = SubsystemKey;
            }

            st = (DbgSspSubsystemKeyLookupRoutine)(
                    &ApiMsg->h.ClientId,
                    &SubsystemKey,
                    FALSE
                    );

            if ( NT_SUCCESS(st) ) {
                ApiMsg->u.CreateProcessInfo.InitialThread.SubSystemKey = SubsystemKey;
            }
        }

        st = DbgSspCreateProcess(
                ContinueKey,
                &ApiMsg->h.ClientId,
                &DebugUiClientId,
                &ApiMsg->u.CreateProcessInfo
                );
        break;


    case DbgKmExitThreadApi :
        st = DbgSspExitThread(
                ContinueKey,
                &ApiMsg->h.ClientId,
                &ApiMsg->u.ExitThread
                );
        break;

    case DbgKmExitProcessApi :
        st = DbgSspExitProcess(
                ContinueKey,
                &ApiMsg->h.ClientId,
                &ApiMsg->u.ExitProcess
                );
        break;

    case DbgKmLoadDllApi :
        st = DbgSspLoadDll(
                ContinueKey,
                &ApiMsg->h.ClientId,
                &ApiMsg->u.LoadDll
                );
        break;

    case DbgKmUnloadDllApi :
        st = DbgSspUnloadDll(
                ContinueKey,
                &ApiMsg->h.ClientId,
                &ApiMsg->u.UnloadDll
                );
        break;

    default :
        st = STATUS_NOT_IMPLEMENTED;
    }

    if ( !NT_SUCCESS(st) ) {
        ApiMsg->ReturnedStatus = st;
        RtlFreeHeap(RtlProcessHeap(), 0, ContinueKey);
        if ( ARGUMENT_PRESENT(ReplyEvent) ) {
            st = NtSetEvent(ReplyEvent,NULL);
        } else {
            st = NtReplyPort(DbgSspKmReplyPort,
                             (PPORT_MESSAGE)ApiMsg
                            );
        }
        ASSERT(NT_SUCCESS(st));
    }
}


NTSTATUS
DbgSspSrvApiLoop(
    IN PVOID ThreadParameter
    )

/*++

Routine Description:

    This loop services Dbg Subsystem server originated messages.

Arguments:

    ThreadParameter - Not used.

Return Value:

    None.

--*/

{
    DBGSRV_APIMSG DbgSrvApiMsg;
    PDBGSS_CONTINUE_KEY ContinueKey;
    NTSTATUS st;

    for(;;) {

        st = NtReplyWaitReceivePort(
                DbgSspApiPort,
                NULL,
                NULL,
                (PPORT_MESSAGE) &DbgSrvApiMsg
                );

        if (!NT_SUCCESS( st )) {
            continue;
            }

        ASSERT(DbgSrvApiMsg.ApiNumber < DbgSrvMaxApiNumber);

        switch (DbgSrvApiMsg.ApiNumber ) {
        case DbgSrvContinueApi :

            //
            // Might want to implement continue status based callout
            // like DBG_TERMINATE_PROCESS/THREAD
            //

            ContinueKey = (PDBGSS_CONTINUE_KEY) DbgSrvApiMsg.ContinueKey;
            ContinueKey->KmApiMsg.ReturnedStatus = DbgSrvApiMsg.ReturnedStatus;

            if ( ContinueKey->ReplyEvent ) {
                st = NtSetEvent(ContinueKey->ReplyEvent,NULL);
            } else {
                st = NtReplyPort(DbgSspKmReplyPort,
                                 (PPORT_MESSAGE) &ContinueKey->KmApiMsg
                                );
            }

            RtlFreeHeap(RtlProcessHeap(), 0, ContinueKey);
            break;
        default :
            ASSERT(FALSE);
        }
    }

    //
    // Make the compiler happy
    //

    return st;
}
