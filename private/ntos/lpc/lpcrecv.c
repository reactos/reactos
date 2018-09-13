/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcrecv.c

Abstract:

    Local Inter-Process Communication (LPC) receive system services.

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "lpcp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtReplyWaitReceivePort)
#endif


NTSTATUS
NtReplyWaitReceivePort (
    IN HANDLE PortHandle,
    OUT PVOID *PortContext OPTIONAL,
    IN PPORT_MESSAGE ReplyMessage OPTIONAL,
    OUT PPORT_MESSAGE ReceiveMessage
    )

/*++

Routine Description:

    This procedure is used by the server process to wait for a message from a
    client process

    A client and server process can receive messages using the
    NtReplyWaitReceivePort service:

    If the ReplyMessage parameter is specified, then the reply will be sent
    using NtReplyPort.

    If the PortHandle parameter specifies a connection port, then the receive
    will return whenever a message is sent to a server communication port that
    does not have its own receive queue and the message is therefore queued to
    the receive queue of the connection port.

    If the PortHandle parameter specifies a server communication port that
    does not have a receive queue, then behaves as if the associated
    connection port handle was specified.  Otherwise the receive will return
    whenever message is placed in the receive queue associated with the
    server communication port.

    The received message will be returned in the variable specified by the
    ReceiveMessage parameter.  If the MapInfoOffset field of the reply message
    is non-zero, then the PORT_MAP_INFORMATION structure it points to will be
    processed and the relevant pages will be mapped into the caller's address
    space.  The service returns an error if there is not enough room in the
    caller's address space to accomodate the mappings.

Arguments:

    PortHandle - Specifies the handle of the connection or communication port
        to do the receive from.

    PortContext - Specifies an optional pointer to a variable that is to
        receive the context value associated with the communication port that
        the message is being received from.  This context variable was
        specified on the call to the NtAcceptConnectPort service.

    ReplyMessage - This optional parameter specifies the address of a reply
        message to be sent.  The ClientId and MessageId fields determine which
        thread will get the reply.  See description of NtReplyPort for how the
        reply is sent.  The reply is sent before blocking for the receive.

    ReceiveMessage - Specifies the address of a variable to receive the
        message.

Return Value:

    Status code that indicates whether or not the operation was successful.

--*/

{
    PLPCP_PORT_OBJECT PortObject;
    PLPCP_PORT_OBJECT ReceivePort;
    PORT_MESSAGE CapturedReplyMessage;
    KPROCESSOR_MODE PreviousMode;
    KPROCESSOR_MODE WaitMode;
    NTSTATUS Status;
    PLPCP_MESSAGE Msg;
    PETHREAD CurrentThread;
    PETHREAD WakeupThread;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread();

    //
    //  Get previous processor mode
    //

    PreviousMode = KeGetPreviousMode();
    WaitMode = PreviousMode;

    if (PreviousMode != KernelMode) {

        try {

            if (ARGUMENT_PRESENT( PortContext )) {

                ProbeForWriteUlong( (PULONG)PortContext );
            }

            if (ARGUMENT_PRESENT( ReplyMessage)) {

                ProbeForRead( ReplyMessage,
                              sizeof( *ReplyMessage ),
                              sizeof( ULONG ));

                CapturedReplyMessage = *ReplyMessage;
            }

            ProbeForWrite( ReceiveMessage,
                           sizeof( *ReceiveMessage ),
                           sizeof( ULONG ));

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return GetExceptionCode();
        }

    } else {

        //
        //  Kernel mode threads call with wait mode of user so that their
        //  kernel stacks are swappable.  Main consumer of this is
        //  SepRmCommandThread
        //

        if ( IS_SYSTEM_THREAD(CurrentThread) ) {

            WaitMode = UserMode;
        }

        if (ARGUMENT_PRESENT( ReplyMessage )) {

            CapturedReplyMessage = *ReplyMessage;
        }
    }

    if (ARGUMENT_PRESENT( ReplyMessage )) {

        //
        //  Make sure DataLength is valid with respect to header size and total
        //  length
        //

        if ((((CLONG)CapturedReplyMessage.u1.s1.DataLength) + sizeof( PORT_MESSAGE )) >
            ((CLONG)CapturedReplyMessage.u1.s1.TotalLength)) {

            return STATUS_INVALID_PARAMETER;
        }

        //
        //  Make sure the user didn't give us a bogus reply message id
        //

        if (CapturedReplyMessage.MessageId == 0) {

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  Reference the port object by handle and if that doesn't work try
    //  a waitable port object type
    //

    Status = LpcpReferencePortObject( PortHandle,
                                      0,
                                      PreviousMode,
                                      &PortObject );

    if (!NT_SUCCESS( Status )) {

        Status = ObReferenceObjectByHandle( PortHandle,
                                            0,
                                            LpcWaitablePortObjectType,
                                            PreviousMode,
                                            &PortObject,
                                            NULL );

        if ( !NT_SUCCESS( Status )) {

            return Status;
        }
    }

    LpcpSaveThread (PortObject);

    //
    //  Validate the message length
    //

    if (ARGUMENT_PRESENT( ReplyMessage )) {

        if (((ULONG)CapturedReplyMessage.u1.s1.TotalLength > PortObject->MaxMessageLength) ||
            ((ULONG)CapturedReplyMessage.u1.s1.TotalLength <= (ULONG)CapturedReplyMessage.u1.s1.DataLength)) {

            ObDereferenceObject( PortObject );

            return STATUS_PORT_MESSAGE_TOO_LONG;
        }
    }

    //
    //  The receive port we use is either the connection port for the port
    //  object we were given if we were given a client communication port then
    //  we expect to recieve the reply on the communication port itself.
    //

    if ((PortObject->Flags & PORT_TYPE) != CLIENT_COMMUNICATION_PORT) {

        ReceivePort = PortObject->ConnectionPort;

    } else {

        ReceivePort = PortObject;
    }

    //
    //  If ReplyMessage argument present, then send reply
    //

    if (ARGUMENT_PRESENT( ReplyMessage )) {

        //
        //  Translate the ClientId from the connection request into a
        //  thread pointer.  This is a referenced pointer to keep the thread
        //  from evaporating out from under us.
        //

        Status = PsLookupProcessThreadByCid( &CapturedReplyMessage.ClientId,
                                             NULL,
                                             &WakeupThread );

        if (!NT_SUCCESS( Status )) {

            ObDereferenceObject( PortObject );
            return Status;
        }

        //
        //  Acquire the global Lpc mutex that gaurds the LpcReplyMessage
        //  field of the thread and get the pointer to the message that
        //  the thread is waiting for a reply to.
        //

        LpcpAcquireLpcpLock();

        Msg = (PLPCP_MESSAGE)LpcpAllocateFromPortZone( CapturedReplyMessage.u1.s1.TotalLength );

        if (Msg == NULL) {

            LpcpReleaseLpcpLock();

            ObDereferenceObject( WakeupThread );
            ObDereferenceObject( PortObject );

            return STATUS_NO_MEMORY;
        }

        //
        //  See if the thread is waiting for a reply to the message
        //  specified on this call.  If not then a bogus message
        //  has been specified, so release the mutex, dereference the thread
        //  and return failure.
        //
        //  We also fail this request if the caller isn't replying to a request
        //  message.  For example, if the caller is replying to a connection
        //  request
        //

        if ((WakeupThread->LpcReplyMessageId != CapturedReplyMessage.MessageId)

                ||

            ((WakeupThread->LpcReplyMessage != NULL) &&
             (((PLPCP_MESSAGE)(WakeupThread->LpcReplyMessage))->Request.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) != LPC_REQUEST)) {

            LpcpPrint(( "%s Attempted ReplyWaitReceive to Thread %lx (%s)\n",
                        PsGetCurrentProcess()->ImageFileName,
                        WakeupThread,
                        THREAD_TO_PROCESS( WakeupThread )->ImageFileName ));

            LpcpPrint(( "failed.  MessageId == %u  Client Id: %x.%x\n",
                        CapturedReplyMessage.MessageId,
                        CapturedReplyMessage.ClientId.UniqueProcess,
                        CapturedReplyMessage.ClientId.UniqueThread ));

            LpcpPrint(( "         Thread MessageId == %u  Client Id: %x.%x\n",
                        WakeupThread->LpcReplyMessageId,
                        WakeupThread->Cid.UniqueProcess,
                        WakeupThread->Cid.UniqueThread ));

#if DBG
            if (LpcpStopOnReplyMismatch) {

                DbgBreakPoint();
            }
#endif

            LpcpFreeToPortZone( Msg, TRUE );

            LpcpReleaseLpcpLock();

            ObDereferenceObject( WakeupThread );
            ObDereferenceObject( PortObject );

            return STATUS_REPLY_MESSAGE_MISMATCH;
        }

        //
        //  Copy the reply message to the request message buffer.  Do this before
        //  we actually fiddle with the wakeup threads fields.  Otherwise we
        //  could mess up its state
        //

        try {

            LpcpMoveMessage( &Msg->Request,
                             &CapturedReplyMessage,
                             (ReplyMessage + 1),
                             LPC_REPLY,
                             NULL );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            LpcpFreeToPortZone( Msg, TRUE );

            LpcpReleaseLpcpLock();

            ObDereferenceObject( WakeupThread );
            ObDereferenceObject( PortObject );

            return (Status = GetExceptionCode());
        }

        LpcpTrace(( "%s Sending Reply Msg %lx (%u.%u, %x) [%08x %08x %08x %08x] to Thread %lx (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    Msg,
                    CapturedReplyMessage.MessageId,
                    CapturedReplyMessage.CallbackId,
                    CapturedReplyMessage.u2.s2.DataInfoOffset,
                    *((PULONG)(Msg+1)+0),
                    *((PULONG)(Msg+1)+1),
                    *((PULONG)(Msg+1)+2),
                    *((PULONG)(Msg+1)+3),
                    WakeupThread,
                    THREAD_TO_PROCESS( WakeupThread )->ImageFileName ));

        //
        //  Locate and free the messsage from the port.  This call use to
        //  test for (CapturedReplyMessage.u2.s2.DataInfoOffset != 0) as a
        //  prerequisite for doing the call.
        //

        LpcpFreeDataInfoMessage( PortObject,
                                 CapturedReplyMessage.MessageId,
                                 CapturedReplyMessage.CallbackId );

        //
        //  Add an extra reference so LpcExitThread does not evaporate
        //  the pointer before we get to the wait below
        //

        ObReferenceObject( WakeupThread );

        //
        //  Release the mutex that guards the LpcReplyMessage field
        //  after marking message as being replied to.
        //

        Msg->RepliedToThread = WakeupThread;

        WakeupThread->LpcReplyMessageId = 0;
        WakeupThread->LpcReplyMessage = (PVOID)Msg;

        //
        //  Remove the thread from the reply rundown list as we are sending the reply.
        //

        if (!WakeupThread->LpcExitThreadCalled && !IsListEmpty( &WakeupThread->LpcReplyChain )) {

            RemoveEntryList( &WakeupThread->LpcReplyChain );

            InitializeListHead( &WakeupThread->LpcReplyChain );
        }

        if ((CurrentThread->LpcReceivedMsgIdValid) &&
            (CurrentThread->LpcReceivedMessageId == CapturedReplyMessage.MessageId)) {

            CurrentThread->LpcReceivedMessageId = 0;

            CurrentThread->LpcReceivedMsgIdValid = FALSE;
        }

        LpcpTrace(( "%s Waiting for message to Port %x (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    ReceivePort,
                    LpcpGetCreatorName( ReceivePort )));

        LpcpReleaseLpcpLock();

        //
        //  Wake up the thread that is waiting for an answer to its request
        //  inside of NtRequestWaitReplyPort or NtReplyWaitReplyPort
        //

        KeReleaseSemaphore( &WakeupThread->LpcReplySemaphore,
                            1,
                            1,
                            FALSE );

        ObDereferenceObject( WakeupThread );

        Status = KeWaitForSingleObject( ReceivePort->MsgQueue.Semaphore,
                                        WrLpcReceive,
                                        WaitMode,
                                        FALSE,
                                        NULL );

        //
        //  Fall into receive code.  Client thread reference will be
        //  returned by the client when it wakes up.
        //

    } else {

        //
        //  The user did not give us a reply message to send off
        //
        //  **** it looks like the only reason to have the then and else
        //       clause both do a wait it to have the lpcp trace denote
        //       the wait without a preceeding reply
        //
        //  Wait for a message
        //

        LpcpTrace(( "%s Waiting for message to Port %x (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    ReceivePort,
                    LpcpGetCreatorName( ReceivePort )));

        Status = KeWaitForSingleObject( ReceivePort->MsgQueue.Semaphore,
                                        WrLpcReceive,
                                        WaitMode,
                                        FALSE,
                                        NULL );
    }

    //
    //  At this point we've awoke from our wait for a receive
    //

    if (Status == STATUS_SUCCESS) {

        LpcpAcquireLpcpLock();

        //
        //  See if we awoke without a message in our receive port
        //

        if (IsListEmpty( &ReceivePort->MsgQueue.ReceiveHead )) {

            if ( ReceivePort->Flags & PORT_WAITABLE ) {

                KeResetEvent( &ReceivePort->WaitEvent );
            }

            LpcpReleaseLpcpLock();

            ObDereferenceObject( PortObject );

            return STATUS_UNSUCCESSFUL;
        }

        //
        //  We have a message in our recieve port.  So let's pull it out
        //

        Msg = (PLPCP_MESSAGE)RemoveHeadList( &ReceivePort->MsgQueue.ReceiveHead );

        if ( IsListEmpty( &ReceivePort->MsgQueue.ReceiveHead)) {

            if ( ReceivePort->Flags & PORT_WAITABLE ) {

                KeResetEvent( &ReceivePort->WaitEvent );
            }
        }

        InitializeListHead( &Msg->Entry );

        LpcpTrace(( "%s Receive Msg %lx (%u) from Port %lx (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    Msg,
                    Msg->Request.MessageId,
                    ReceivePort,
                    LpcpGetCreatorName( ReceivePort )));

        //
        //  Now make the thread state to be the message we're currently
        //  working on
        //

        CurrentThread->LpcReceivedMessageId = Msg->Request.MessageId;
        CurrentThread->LpcReceivedMsgIdValid = TRUE;

        LpcpReleaseLpcpLock();

        try {

            //
            //  Check if the message is a connection request
            //

            if ((Msg->Request.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) == LPC_CONNECTION_REQUEST) {

                PLPCP_CONNECTION_MESSAGE ConnectMsg;
                ULONG ConnectionInfoLength;
                PLPCP_MESSAGE TempMsg;

                ConnectMsg = (PLPCP_CONNECTION_MESSAGE)(Msg + 1);

                ConnectionInfoLength = Msg->Request.u1.s1.DataLength - sizeof( *ConnectMsg );

                //
                //  Dont free message until NtAcceptConnectPort called, and if it's never called
                //  then we'll keep the message until the client exits.
                //

                TempMsg = Msg;
                Msg = NULL;

                *ReceiveMessage = TempMsg->Request;

                ReceiveMessage->u1.s1.TotalLength = (CSHORT)(sizeof( *ReceiveMessage ) + ConnectionInfoLength);
                ReceiveMessage->u1.s1.DataLength = (CSHORT)ConnectionInfoLength;

                RtlMoveMemory( ReceiveMessage+1,
                               ConnectMsg + 1,
                               ConnectionInfoLength );

                if (ARGUMENT_PRESENT( PortContext )) {

                    *PortContext = NULL;
                }

            //
            //  Check if the message is not a reply
            //

            } else if ((Msg->Request.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) != LPC_REPLY) {

                LpcpMoveMessage( ReceiveMessage,
                                 &Msg->Request,
                                 (&Msg->Request) + 1,
                                 0,
                                 NULL );

                if (ARGUMENT_PRESENT( PortContext )) {

                    *PortContext = Msg->PortContext;
                }

                //
                //  If message contains DataInfo for access via NtRead/WriteRequestData
                //  then put the message on a list in the communication port and dont
                //  free it.  It will be freed when the server replies to the message.
                //

                if (Msg->Request.u2.s2.DataInfoOffset != 0) {

                    LpcpSaveDataInfoMessage( PortObject, Msg );
                    Msg = NULL;
                }

            //
            //  Otherwise this is a reply message we just recieved
            //

            } else {

                LpcpPrint(( "LPC: Bogus reply message (%08x) in receive queue of connection port %08x\n",
                            Msg, ReceivePort ));

                KdBreakPoint();
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();    // FIX, FIX
        }

        //
        //  Acquire the LPC mutex and decrement the reference count for the
        //  message.  If the reference count goes to zero the message will be
        //  deleted.
        //

        if (Msg != NULL) {

            LpcpFreeToPortZone( Msg, FALSE );
        }
    }

    ObDereferenceObject( PortObject );

    //
    //  And return to our caller
    //

    return Status;
}

NTSTATUS
NtReplyWaitReceivePortEx(
    IN HANDLE PortHandle,
    OUT PVOID *PortContext OPTIONAL,
    IN PPORT_MESSAGE ReplyMessage OPTIONAL,
    OUT PPORT_MESSAGE ReceiveMessage,
    IN PLARGE_INTEGER Timeout OPTIONAL
    )

/*++

Routine Description:

    See NtReplyWaitReceivePort.

Arguments:

    See NtReplyWaitReceivePort.

    Timeout - Supplies an optional timeout value to use when waiting for a
        receive.

Return Value:

    See NtReplyWaitReceivePort.

--*/

{
    PLPCP_PORT_OBJECT PortObject;
    PLPCP_PORT_OBJECT ReceivePort;
    PORT_MESSAGE CapturedReplyMessage;
    KPROCESSOR_MODE PreviousMode;
    KPROCESSOR_MODE WaitMode;
    NTSTATUS Status;
    PLPCP_MESSAGE Msg;
    PETHREAD CurrentThread;
    PETHREAD WakeupThread;
    LARGE_INTEGER TimeoutValue ;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread();

    TimeoutValue.QuadPart = 0 ;

    //
    //  Get previous processor mode
    //

    PreviousMode = KeGetPreviousMode();
    WaitMode = PreviousMode;

    if (PreviousMode != KernelMode) {

        try {

            if (ARGUMENT_PRESENT( PortContext )) {

                ProbeForWriteUlong( (PULONG)PortContext );
            }

            if (ARGUMENT_PRESENT( ReplyMessage)) {

                ProbeForRead( ReplyMessage,
                              sizeof( *ReplyMessage ),
                              sizeof( ULONG ));

                CapturedReplyMessage = *ReplyMessage;
            }

            if (ARGUMENT_PRESENT( Timeout )) {

                TimeoutValue = ProbeAndReadLargeInteger( Timeout );

                Timeout = &TimeoutValue ;
            }

            ProbeForWrite( ReceiveMessage,
                           sizeof( *ReceiveMessage ),
                           sizeof( ULONG ));

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return GetExceptionCode();
        }

    } else {

        //
        //  Kernel mode threads call with wait mode of user so that their
        //  kernel // stacks are swappable. Main consumer of this is
        //  SepRmCommandThread
        //

        if ( IS_SYSTEM_THREAD(CurrentThread) ) {

            WaitMode = UserMode;
        }

        if (ARGUMENT_PRESENT( ReplyMessage)) {

            CapturedReplyMessage = *ReplyMessage;
        }
    }

    if (ARGUMENT_PRESENT( ReplyMessage)) {

        //
        //  Make sure DataLength is valid with respect to header size and total
        //  length
        //

        if ((((CLONG)CapturedReplyMessage.u1.s1.DataLength) + sizeof( PORT_MESSAGE )) >
            ((CLONG)CapturedReplyMessage.u1.s1.TotalLength)) {

            return STATUS_INVALID_PARAMETER;
        }

        //
        //  Make sure the user didn't give us a bogus reply message id
        //

        if (CapturedReplyMessage.MessageId == 0) {

            return STATUS_INVALID_PARAMETER;
        }
    }

    //
    //  Reference the port object by handle and if that doesn't work try
    //  a waitable port object type
    //

    Status = LpcpReferencePortObject( PortHandle,
                                      0,
                                      PreviousMode,
                                      &PortObject );

    if (!NT_SUCCESS( Status )) {

        Status = ObReferenceObjectByHandle( PortHandle,
                                            0,
                                            LpcWaitablePortObjectType,
                                            PreviousMode,
                                            &PortObject,
                                            NULL );

        if ( !NT_SUCCESS( Status ) ) {

            return Status;
        }
    }

    LpcpSaveThread (PortObject);

    //
    //  The receive port we use is either the connection port for the port
    //  object we were given if we were given a client communication port then
    //  we expect to recieve the reply on the communication port itself.
    //

    if ((PortObject->Flags & PORT_TYPE) != CLIENT_COMMUNICATION_PORT) {

        ReceivePort = PortObject->ConnectionPort;

    } else {

        ReceivePort = PortObject;
    }

    //
    //  If ReplyMessage argument present, then send reply
    //

    if (ARGUMENT_PRESENT( ReplyMessage )) {

        //
        //  Translate the ClientId from the connection request into a
        //  thread pointer.  This is a referenced pointer to keep the thread
        //  from evaporating out from under us.
        //

        Status = PsLookupProcessThreadByCid( &CapturedReplyMessage.ClientId,
                                             NULL,
                                             &WakeupThread );

        if (!NT_SUCCESS( Status )) {

            ObDereferenceObject( PortObject );
            return Status;
        }

        //
        //  Acquire the global Lpc mutex that gaurds the LpcReplyMessage
        //  field of the thread and get the pointer to the message that
        //  the thread is waiting for a reply to.
        //

        LpcpAcquireLpcpLock();

        Msg = (PLPCP_MESSAGE)LpcpAllocateFromPortZone( CapturedReplyMessage.u1.s1.TotalLength );

        if (Msg == NULL) {

            LpcpReleaseLpcpLock();

            ObDereferenceObject( WakeupThread );
            ObDereferenceObject( PortObject );

            return STATUS_NO_MEMORY;
        }

        //
        //  See if the thread is waiting for a reply to the message
        //  specified on this call.  If not then a bogus message
        //  has been specified, so release the mutex, dereference the thread
        //  and return failure.
        //
        //  We also fail this request if the caller isn't replying to a request
        //  message.  For example, if the caller is replying to a connection
        //  request
        //

        if ((WakeupThread->LpcReplyMessageId != CapturedReplyMessage.MessageId)

                ||

            ((WakeupThread->LpcReplyMessage != NULL) &&
             (((PLPCP_MESSAGE)(WakeupThread->LpcReplyMessage))->Request.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) != LPC_REQUEST)) {

            LpcpPrint(( "%s Attempted ReplyWaitReceive to Thread %lx (%s)\n",
                        PsGetCurrentProcess()->ImageFileName,
                        WakeupThread,
                        THREAD_TO_PROCESS( WakeupThread )->ImageFileName ));

            LpcpPrint(( "failed.  MessageId == %u  Client Id: %x.%x\n",
                        CapturedReplyMessage.MessageId,
                        CapturedReplyMessage.ClientId.UniqueProcess,
                        CapturedReplyMessage.ClientId.UniqueThread ));

            LpcpPrint(( "         Thread MessageId == %u  Client Id: %x.%x\n",
                        WakeupThread->LpcReplyMessageId,
                        WakeupThread->Cid.UniqueProcess,
                        WakeupThread->Cid.UniqueThread ));

#if DBG
            if (LpcpStopOnReplyMismatch) {

                DbgBreakPoint();
            }
#endif

            LpcpFreeToPortZone( Msg, TRUE );

            LpcpReleaseLpcpLock();

            ObDereferenceObject( WakeupThread );
            ObDereferenceObject( PortObject );

            return STATUS_REPLY_MESSAGE_MISMATCH;
        }

        //
        //  Copy the reply message to the request message buffer.  Do this before
        //  we actually fiddle with the wakeup threads fields.  Otherwise we
        //  could mess up its state
        //

        try {

            LpcpMoveMessage( &Msg->Request,
                             &CapturedReplyMessage,
                             (ReplyMessage + 1),
                             LPC_REPLY,
                             NULL );

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            LpcpFreeToPortZone( Msg, TRUE );

            LpcpReleaseLpcpLock();

            ObDereferenceObject( WakeupThread );
            ObDereferenceObject( PortObject );

            return (Status = GetExceptionCode());
        }

        LpcpTrace(( "%s Sending Reply Msg %lx (%u.%u, %x) [%08x %08x %08x %08x] to Thread %lx (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    Msg,
                    CapturedReplyMessage.MessageId,
                    CapturedReplyMessage.CallbackId,
                    CapturedReplyMessage.u2.s2.DataInfoOffset,
                    *((PULONG)(Msg+1)+0),
                    *((PULONG)(Msg+1)+1),
                    *((PULONG)(Msg+1)+2),
                    *((PULONG)(Msg+1)+3),
                    WakeupThread,
                    THREAD_TO_PROCESS( WakeupThread )->ImageFileName ));

        //
        //  Locate and free the messsage from the port.  This call use to
        //  test for (CapturedReplyMessage.u2.s2.DataInfoOffset != 0) as a
        //  prerequisite for doing the call.
        //

        LpcpFreeDataInfoMessage( PortObject,
                                 CapturedReplyMessage.MessageId,
                                 CapturedReplyMessage.CallbackId );

        //
        //  Add an extra reference so LpcExitThread does not evaporate
        //  the pointer before we get to the wait below
        //

        ObReferenceObject( WakeupThread );

        //
        //  Release the mutex that guards the LpcReplyMessage field
        //  after marking message as being replied to.
        //

        Msg->RepliedToThread = WakeupThread;

        WakeupThread->LpcReplyMessageId = 0;
        WakeupThread->LpcReplyMessage = (PVOID)Msg;

        //
        //  Remove the thread from the reply rundown list as we are sending the reply.
        //

        if ((!WakeupThread->LpcExitThreadCalled) && (!IsListEmpty( &WakeupThread->LpcReplyChain ))) {

            RemoveEntryList( &WakeupThread->LpcReplyChain );

            InitializeListHead( &WakeupThread->LpcReplyChain );
        }

        if ((CurrentThread->LpcReceivedMsgIdValid) &&
            (CurrentThread->LpcReceivedMessageId == CapturedReplyMessage.MessageId)) {

            CurrentThread->LpcReceivedMessageId = 0;

            CurrentThread->LpcReceivedMsgIdValid = FALSE;
        }

        LpcpTrace(( "%s Waiting for message to Port %x (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    ReceivePort,
                    LpcpGetCreatorName( ReceivePort )));

        LpcpReleaseLpcpLock();

        //
        //  Wake up the thread that is waiting for an answer to its request
        //  inside of NtRequestWaitReplyPort or NtReplyWaitReplyPort
        //

        KeReleaseSemaphore( &WakeupThread->LpcReplySemaphore,
                            1,
                            1,
                            FALSE );

        ObDereferenceObject( WakeupThread );

        //
        //  **** the timeout on this wait and the next wait appear to be the
        //       only substantial difference between NtReplyWaitReceivePort
        //       and NtReplyWaitReceivePortEx

        Status = KeWaitForSingleObject( ReceivePort->MsgQueue.Semaphore,
                                        WrLpcReceive,
                                        WaitMode,
                                        FALSE,
                                        Timeout );

        //
        //  Fall into receive code.  Client thread reference will be
        //  returned by the client when it wakes up.
        //

    } else {

        //
        //  The user did not give us a reply message to send off
        //
        //  **** it looks like the only reason to have the then and else
        //       clause both do a wait it to have the lpcp trace denote
        //       the wait without a preceeding reply
        //
        //  Wait for a message
        //

        LpcpTrace(( "%s Waiting for message to Port %x (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    ReceivePort,
                    LpcpGetCreatorName( ReceivePort )));

        Status = KeWaitForSingleObject( ReceivePort->MsgQueue.Semaphore,
                                        WrLpcReceive,
                                        WaitMode,
                                        FALSE,
                                        Timeout );
    }

    //
    //  At this point we've awoke from our wait for a receive
    //

    if (Status == STATUS_SUCCESS) {

        LpcpAcquireLpcpLock();

        //
        //  See if we awoke without a message in our receive port
        //

        if (IsListEmpty( &ReceivePort->MsgQueue.ReceiveHead )) {

            if ( ReceivePort->Flags & PORT_WAITABLE ) {

                KeResetEvent( &ReceivePort->WaitEvent );
            }

            LpcpReleaseLpcpLock();

            ObDereferenceObject( PortObject );

            return STATUS_UNSUCCESSFUL;
        }

        //
        //  We have a message in our recieve port.  So let's pull it out
        //

        Msg = (PLPCP_MESSAGE)RemoveHeadList( &ReceivePort->MsgQueue.ReceiveHead );

        if ( IsListEmpty( &ReceivePort->MsgQueue.ReceiveHead ) ) {

            if ( ReceivePort->Flags & PORT_WAITABLE ) {

                KeResetEvent( &ReceivePort->WaitEvent );
            }
        }

        InitializeListHead( &Msg->Entry );

        LpcpTrace(( "%s Receive Msg %lx (%u) from Port %lx (%s)\n",
                    PsGetCurrentProcess()->ImageFileName,
                    Msg,
                    Msg->Request.MessageId,
                    ReceivePort,
                    LpcpGetCreatorName( ReceivePort )));

        //
        //  Now make the thread state to be the message we're currently
        //  working on
        //

        CurrentThread->LpcReceivedMessageId = Msg->Request.MessageId;
        CurrentThread->LpcReceivedMsgIdValid = TRUE;

        LpcpReleaseLpcpLock();

        try {

            //
            //  Check if the message is a connection request
            //

            if ((Msg->Request.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) == LPC_CONNECTION_REQUEST) {

                PLPCP_CONNECTION_MESSAGE ConnectMsg;
                ULONG ConnectionInfoLength;
                PLPCP_MESSAGE TempMsg;

                ConnectMsg = (PLPCP_CONNECTION_MESSAGE)(Msg + 1);

                ConnectionInfoLength = Msg->Request.u1.s1.DataLength - sizeof( *ConnectMsg );

                //
                //  Dont free message until NtAcceptConnectPort called, and if it's never called
                //  then we'll keep the message until the client exits.
                //

                TempMsg = Msg;
                Msg = NULL;

                *ReceiveMessage = TempMsg->Request;

                ReceiveMessage->u1.s1.TotalLength = (CSHORT)(sizeof( *ReceiveMessage ) + ConnectionInfoLength);
                ReceiveMessage->u1.s1.DataLength = (CSHORT)ConnectionInfoLength;

                RtlMoveMemory( ReceiveMessage+1,
                               ConnectMsg + 1,
                               ConnectionInfoLength );

                if (ARGUMENT_PRESENT( PortContext )) {

                    *PortContext = NULL;
                }

            //
            //  Check if the message is not a reply
            //

            } else if ((Msg->Request.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) != LPC_REPLY) {

                LpcpMoveMessage( ReceiveMessage,
                                 &Msg->Request,
                                 (&Msg->Request) + 1,
                                 0,
                                 NULL );

                if (ARGUMENT_PRESENT( PortContext )) {

                    *PortContext = Msg->PortContext;
                }

                //
                //  If message contains DataInfo for access via NtRead/WriteRequestData
                //  then put the message on a list in the communication port and dont
                //  free it.  It will be freed when the server replies to the message.
                //

                if (Msg->Request.u2.s2.DataInfoOffset != 0) {

                    LpcpSaveDataInfoMessage( PortObject, Msg );
                    Msg = NULL;
                }

            //
            //  Otherwise this is a reply message we just recieved
            //

            } else {

                LpcpPrint(( "LPC: Bogus reply message (%08x) in receive queue of connection port %08x\n",
                            Msg, ReceivePort ));

                KdBreakPoint();
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            Status = GetExceptionCode();    // FIX, FIX
        }

        //
        //  Acquire the LPC mutex and decrement the reference count for the
        //  message.  If the reference count goes to zero the message will be
        //  deleted.
        //

        if (Msg != NULL) {

            LpcpFreeToPortZone( Msg, FALSE );
        }
    }

    ObDereferenceObject( PortObject );

    //
    //  And return to our caller
    //

    return Status;
}
