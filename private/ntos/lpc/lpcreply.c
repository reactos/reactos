/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcreply.c

Abstract:

    Local Inter-Process Communication (LPC) reply system services.

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "lpcp.h"

NTSTATUS
LpcpCopyRequestData (
    IN BOOLEAN WriteToMessageData,
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE Message,
    IN ULONG DataEntryIndex,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG NumberOfBytesCopied OPTIONAL
    );

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,NtReplyPort)
#pragma alloc_text(PAGE,NtReplyWaitReplyPort)
#pragma alloc_text(PAGE,NtReadRequestData)
#pragma alloc_text(PAGE,NtWriteRequestData)
#pragma alloc_text(PAGE,LpcpCopyRequestData)
#endif


NTSTATUS
NtReplyPort (
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    A client and server process can send a reply to a previous request
    message with the NtReplyPort service:

    The Type field of the message is set to LPC_REPLY by the service.  If the
    MapInfoOffset field of the reply message is non-zero, then the
    PORT_MAP_INFORMATION structure it points to will be processed and the
    relevant pages in the caller's address space will be unmapped.

    The ClientId and MessageId fields of the ReplyMessage structure are used
    to identify the thread waiting for this reply.  If the target thread is
    in fact waiting for this reply message, then the reply message is copied
    into the thread's message buffer and the thread's wait is satisfied.

    If the thread is not waiting for a reply or is waiting for a reply to
    some other MessageId, then the message is placed in the message queue of
    the port that is connected to the communication port specified by the
    PortHandle parameter and the Type field of the message is set to
    LPC_LOST_REPLY.

Arguments:

    PortHandle - Specifies the handle of the communication port that the
        original message was received from.

    ReplyMessage - Specifies a pointer to the reply message to be sent.
        The ClientId and MessageId fields determine which thread will
        get the reply.

Return Value:

    Status code that indicates whether or not the operation was
    successful.

--*/

{
    KPROCESSOR_MODE PreviousMode;
    PLPCP_PORT_OBJECT PortObject;
    PORT_MESSAGE CapturedReplyMessage;
    NTSTATUS Status;
    PLPCP_MESSAGE Msg;
    PETHREAD CurrentThread;
    PETHREAD WakeupThread;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread();

    //
    //  Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            ProbeForRead( ReplyMessage,
                          sizeof( *ReplyMessage ),
                          sizeof( ULONG ));

            CapturedReplyMessage = *ReplyMessage;

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return GetExceptionCode();
        }

    } else {

        CapturedReplyMessage = *ReplyMessage;
    }

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

    //
    //  Reference the port object by handle
    //

    Status = LpcpReferencePortObject( PortHandle,
                                      0,
                                      PreviousMode,
                                      &PortObject );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    LpcpSaveThread (PortObject);

    //
    //  Validate the message length
    //

    if (((ULONG)CapturedReplyMessage.u1.s1.TotalLength > PortObject->MaxMessageLength) ||
        ((ULONG)CapturedReplyMessage.u1.s1.TotalLength <= (ULONG)CapturedReplyMessage.u1.s1.DataLength)) {

        ObDereferenceObject( PortObject );

        return STATUS_PORT_MESSAGE_TOO_LONG;
    }

    //
    //  Translate the ClientId from the connection request into a thread
    //  pointer.  This is a referenced pointer to keep the thread from
    //  evaporating out from under us.
    //

    Status = PsLookupProcessThreadByCid( &CapturedReplyMessage.ClientId,
                                         NULL,
                                         &WakeupThread );

    if (!NT_SUCCESS( Status )) {

        ObDereferenceObject( PortObject );

        return Status;
    }

    //
    //  Acquire the mutex that guards the LpcReplyMessage field of the thread
    //  and get the pointer to the message that the thread is waiting for a
    //  reply to.
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
    //  See if the thread is waiting for a reply to the message specified on
    //  this call.  If not then a bogus message has been specified, so
    //  release the mutex, dereference the thread and return failure.
    //
    //  We also fail this request if the caller isn't replying to a request
    //  message.  For example, if the caller is replying to a connection
    //  request
    //

    if ((WakeupThread->LpcReplyMessageId != CapturedReplyMessage.MessageId)

            ||

        ((WakeupThread->LpcReplyMessage != NULL) &&
         (((PLPCP_MESSAGE)(WakeupThread->LpcReplyMessage))->Request.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) != LPC_REQUEST)) {

        LpcpPrint(( "%s Attempted reply to Thread %lx (%s)\n",
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

    //
    //  At this point we know the thread is waiting for our reply
    //

    LpcpTrace(( "%s Sending Reply Msg %lx (%u, %x) [%08x %08x %08x %08x] to Thread %lx (%s)\n",
                PsGetCurrentProcess()->ImageFileName,
                Msg,
                CapturedReplyMessage.MessageId,
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
    //  Add an extra reference so LpcExitThread does not evaporate the
    //  pointer before we get to the wait below
    //

    ObReferenceObject( WakeupThread );

    //
    //  Release the mutex that guards the LpcReplyMessage field after marking
    //  message as being replied to.
    //

    Msg->RepliedToThread = WakeupThread;

    WakeupThread->LpcReplyMessageId = 0;
    WakeupThread->LpcReplyMessage = (PVOID)Msg;

    //
    //  Remove the thread from the reply rundown list as we are sending the
    //  reply.
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

    LpcpReleaseLpcpLock();

    //
    //  Wake up the thread that is waiting for an answer to its request
    //  inside of NtRequestWaitReplyPort or NtReplyWaitReplyPort.  That
    //  will dereference itself when it wakes up.
    //

    KeReleaseSemaphore( &WakeupThread->LpcReplySemaphore,
                        0,
                        1L,
                        FALSE );

    ObDereferenceObject( WakeupThread );

    //
    //  Dereference port object and return the system service status.
    //

    ObDereferenceObject( PortObject );

    return Status;
}


NTSTATUS
NtReplyWaitReplyPort (
    IN HANDLE PortHandle,
    IN OUT PPORT_MESSAGE ReplyMessage
    )

/*++

Routine Description:

    A client and server process can send a reply to a previous message and
    block waiting for a reply using the NtReplyWaitReplyPort service:

    This service works the same as NtReplyPort, except that after delivering
    the reply message, it blocks waiting for a reply to a previous message.
    When the reply is received, it will be placed in the location specified
    by the ReplyMessage parameter.

Arguments:

    PortHandle - Specifies the handle of the communication port that the
        original message was received from.

    ReplyMessage - Specifies a pointer to the reply message to be sent.
        The ClientId and MessageId fields determine which thread will
        get the reply.  This buffer also receives any reply that comes
        back from the wait.

Return Value:

    Status code that indicates whether or not the operation was
    successful.

--*/

{
    KPROCESSOR_MODE PreviousMode;
    NTSTATUS Status;
    PLPCP_PORT_OBJECT PortObject;
    PORT_MESSAGE CapturedReplyMessage;
    PLPCP_MESSAGE Msg;
    PETHREAD CurrentThread;
    PETHREAD WakeupThread;

    PAGED_CODE();

    CurrentThread = PsGetCurrentThread();

    //
    //  Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            ProbeForWrite( ReplyMessage,
                           sizeof( *ReplyMessage ),
                           sizeof( ULONG ));

            CapturedReplyMessage = *ReplyMessage;

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return GetExceptionCode();
        }

    } else {

        CapturedReplyMessage = *ReplyMessage;
    }

    //
    //  Make sure DataLength is valid with respect to header size and total length
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

    //
    //  Reference the communication port object by handle.  Return status if
    //  unsuccessful.
    //

    Status = LpcpReferencePortObject( PortHandle,
                                      0,
                                      PreviousMode,
                                      &PortObject );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    LpcpSaveThread (PortObject);

    //
    //  Validate the message length
    //

    if (((ULONG)CapturedReplyMessage.u1.s1.TotalLength > PortObject->MaxMessageLength) ||
        ((ULONG)CapturedReplyMessage.u1.s1.TotalLength <= (ULONG)CapturedReplyMessage.u1.s1.DataLength)) {

        ObDereferenceObject( PortObject );

        return STATUS_PORT_MESSAGE_TOO_LONG;
    }

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
    //  Acquire the mutex that gaurds the LpcReplyMessage field of
    //  the thread and get the pointer to the message that the thread
    //  is waiting for a reply to.
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

        LpcpPrint(( "%s Attempted reply wait reply to Thread %lx (%s)\n",
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

    //
    //  At this point we know the thread is waiting for our reply
    //

    LpcpTrace(( "%s Sending Reply Wait Reply Msg %lx (%u, %x) [%08x %08x %08x %08x] to Thread %lx (%s)\n",
                PsGetCurrentProcess()->ImageFileName,
                Msg,
                CapturedReplyMessage.MessageId,
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

    //
    //  Set ourselves up to get the following reply
    //

    CurrentThread->LpcReplyMessageId = CapturedReplyMessage.MessageId;
    CurrentThread->LpcReplyMessage = NULL;

    if ((CurrentThread->LpcReceivedMsgIdValid) &&
        (CurrentThread->LpcReceivedMessageId == CapturedReplyMessage.MessageId)) {

        CurrentThread->LpcReceivedMessageId = 0;
        CurrentThread->LpcReceivedMsgIdValid = FALSE;
    }

    LpcpReleaseLpcpLock();

    //
    //  Wake up the thread that is waiting for an answer to its request
    //  inside of NtRequestWaitReplyPort or NtReplyWaitReplyPort.  That
    //  will dereference itself when it wakes up.
    //

    KeReleaseSemaphore( &WakeupThread->LpcReplySemaphore,
                        1,
                        1,
                        FALSE );

    ObDereferenceObject( WakeupThread );

    //
    //  And wait for a reply
    //

    Status = KeWaitForSingleObject( &CurrentThread->LpcReplySemaphore,
                                    Executive,
                                    PreviousMode,
                                    FALSE,
                                    NULL );

    if (Status == STATUS_USER_APC) {

        //
        //  if the semaphore is signaled, then clear it
        //

        if (KeReadStateSemaphore( &CurrentThread->LpcReplySemaphore )) {

            KeWaitForSingleObject( &CurrentThread->LpcReplySemaphore,
                                   WrExecutive,
                                   KernelMode,
                                   FALSE,
                                   NULL );

            Status = STATUS_SUCCESS;
        }
    }

    //
    //  If the wait succeeded, copy the reply to the reply buffer.
    //

    if (Status == STATUS_SUCCESS) {

        //
        //  Acquire the mutex that guards the request message queue.  Remove
        //  the request message from the list of messages being processed and
        //  free the message back to the queue's zone.  If the zone's free
        //  list was zero before freeing this message then pulse the free
        //  event after free the message so that threads waiting to allocate
        //  a request message buffer will wake up.  Finally, release the mutex
        //  and return the system service status.
        //

        LpcpAcquireLpcpLock();

        Msg = CurrentThread->LpcReplyMessage;
        CurrentThread->LpcReplyMessage = NULL;

#if DBG
        if (Msg != NULL) {

            LpcpTrace(( "%s Got Reply Msg %lx (%u) [%08x %08x %08x %08x] for Thread %lx (%s)\n",
                        PsGetCurrentProcess()->ImageFileName,
                        Msg,
                        Msg->Request.MessageId,
                        *((PULONG)(Msg+1)+0),
                        *((PULONG)(Msg+1)+1),
                        *((PULONG)(Msg+1)+2),
                        *((PULONG)(Msg+1)+3),
                        CurrentThread,
                        THREAD_TO_PROCESS( CurrentThread )->ImageFileName ));

            if (!IsListEmpty( &Msg->Entry )) {

                LpcpTrace(( "Reply Msg %lx has non-empty list entry\n", Msg ));
            }
        }
#endif

        LpcpReleaseLpcpLock();

        if (Msg != NULL) {

            try {

                LpcpMoveMessage( ReplyMessage,
                                 &Msg->Request,
                                 (&Msg->Request) + 1,
                                 0,
                                 NULL );

            } except( EXCEPTION_EXECUTE_HANDLER ) {

                Status = GetExceptionCode();
            }

            //
            //  Acquire the LPC mutex and decrement the reference count for the
            //  message.  If the reference count goes to zero the message will be
            //  deleted.
            //

            LpcpAcquireLpcpLock();

            if (Msg->RepliedToThread != NULL) {

                ObDereferenceObject( Msg->RepliedToThread );

                Msg->RepliedToThread = NULL;
            }

            LpcpFreeToPortZone( Msg, TRUE );

            LpcpReleaseLpcpLock();
        }
    }

    ObDereferenceObject( PortObject );

    return Status;
}


NTSTATUS
NtReadRequestData (
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE Message,
    IN ULONG DataEntryIndex,
    OUT PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG NumberOfBytesRead OPTIONAL
    )

/*++

Routine Description:

    This routine is used to copy data from a port message into the user
    supplied buffer.

Arguments:

    PortHandle - Supplies the port from which the message is being read

    Message - Supplies the message that we are trying to read

    DataEntryIndex - Supplies the index of the port data entry in the
        preceeding message that we are reading

    Buffer - Supplies the location into which the data is to be read

    BufferSize - Supplies the size, in bytes, of the preceeding buffer

    NumberOfBytesRead - Optionally returns the number of bytes read into
        the buffer

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    PAGED_CODE();

    return LpcpCopyRequestData( FALSE,
                                PortHandle,
                                Message,
                                DataEntryIndex,
                                Buffer,
                                BufferSize,
                                NumberOfBytesRead );
}


NTSTATUS
NtWriteRequestData (
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE Message,
    IN ULONG DataEntryIndex,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG NumberOfBytesWritten OPTIONAL
    )

/*++

Routine Description:

    This routine is used to copy data from the user supplied buffer into the
    port message

Arguments:

    PortHandle - Supplies the port into which the message is being written

    Message - Supplies the message that we are trying to write

    DataEntryIndex - Supplies the index of the port data entry in the
        preceeding message that we are writing

    Buffer - Supplies the location into which the data is to be written

    BufferSize - Supplies the size, in bytes, of the preceeding buffer

    NumberOfBytesRead - Optionally returns the number of bytes written from
        the buffer

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    PAGED_CODE();

    return LpcpCopyRequestData( TRUE,
                                PortHandle,
                                Message,
                                DataEntryIndex,
                                Buffer,
                                BufferSize,
                                NumberOfBytesWritten );
}


//
//  Local support routine
//

NTSTATUS
LpcpCopyRequestData (
    IN BOOLEAN WriteToMessageData,
    IN HANDLE PortHandle,
    IN PPORT_MESSAGE Message,
    IN ULONG DataEntryIndex,
    IN PVOID Buffer,
    IN ULONG BufferSize,
    OUT PULONG NumberOfBytesCopied OPTIONAL
    )

/*++

Routine Description:

    This routine will copy data to or from the user supplied buffer and the
    port message data information buffer

Arguments:

    WriteToMessageData - TRUE if the data is to be copied from the user buffer
        to the message and FALSE otherwise

    PortHandle - Supplies the port into which the message is being manipulated

    Message - Supplies the message that we are trying to manipulate

    DataEntryIndex - Supplies the index of the port data entry in the
        preceeding message that we are transfering

    Buffer - Supplies the location into which the data is to be transfered

    BufferSize - Supplies the size, in bytes, of the preceeding buffer

    NumberOfBytesRead - Optionally returns the number of bytes transfered from
        the buffer

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    KPROCESSOR_MODE PreviousMode;
    PLPCP_PORT_OBJECT PortObject;
    PLPCP_MESSAGE Msg;
    PLIST_ENTRY Head, Next;
    NTSTATUS Status;
    PETHREAD ClientThread;
    PPORT_DATA_INFORMATION DataInfo;
    PPORT_DATA_ENTRY DataEntry;
    PORT_MESSAGE CapturedMessage;
    PORT_DATA_INFORMATION CapturedDataInfo;
    PORT_DATA_ENTRY CapturedDataEntry;
    ULONG BytesCopied;

    PAGED_CODE();

    //
    //  Get previous processor mode and probe output arguments if necessary.
    //

    PreviousMode = KeGetPreviousMode();

    if (PreviousMode != KernelMode) {

        try {

            //
            //  We are either reading or writing the user buffer
            //

            if (WriteToMessageData) {

                ProbeForRead( Buffer,
                              BufferSize,
                              1 );

            } else {

                ProbeForWrite( Buffer,
                               BufferSize,
                               1 );
            }

            ProbeForRead( Message,
                          sizeof( *Message ),
                          sizeof( ULONG ));

            CapturedMessage = *Message;

            if (ARGUMENT_PRESENT( NumberOfBytesCopied )) {

                ProbeForWriteUlong( NumberOfBytesCopied );
            }

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            return GetExceptionCode();
        }

    } else {

        CapturedMessage = *Message;
    }

    //
    //  The message better have at least one data entry
    //

    if (CapturedMessage.u2.s2.DataInfoOffset == 0) {

        return STATUS_INVALID_PARAMETER;
    }

    //
    //  Reference the port object by handle
    //

    Status = LpcpReferencePortObject( PortHandle,
                                      0,
                                      PreviousMode,
                                      &PortObject );

    if (!NT_SUCCESS( Status )) {

        return Status;
    }

    LpcpSaveThread (PortObject);

    //
    //  Translate the ClientId from the connection request into a
    //  thread pointer.  This is a referenced pointer to keep the thread
    //  from evaporating out from under us.
    //

    Status = PsLookupProcessThreadByCid( &CapturedMessage.ClientId,
                                         NULL,
                                         &ClientThread );

    if (!NT_SUCCESS( Status )) {

        ObDereferenceObject( PortObject );

        return Status;
    }

    //
    //  Acquire the mutex that guards the LpcReplyMessage field of
    //  the thread and get the pointer to the message that the thread
    //  is waiting for a reply to.
    //

    LpcpAcquireLpcpLock();

    //
    //  See if the thread is waiting for a reply to the message
    //  specified on this call.  If not then a bogus message
    //  has been specified, so release the mutex, dereference the thread
    //  and return failure.
    //

    if (ClientThread->LpcReplyMessageId != CapturedMessage.MessageId) {

        Status = STATUS_REPLY_MESSAGE_MISMATCH;

    } else {

        Status = STATUS_INVALID_PARAMETER;

        Msg = LpcpFindDataInfoMessage( PortObject,
                                       CapturedMessage.MessageId,
                                       CapturedMessage.CallbackId );

        if (Msg != NULL) {

            DataInfo = (PPORT_DATA_INFORMATION)((PUCHAR)&Msg->Request +
                                                Msg->Request.u2.s2.DataInfoOffset);

            //
            //  Make sure the caller isn't asking for an index beyond what's
            //  in the message
            //

            if (DataInfo->CountDataEntries > DataEntryIndex) {

                DataEntry = &DataInfo->DataEntries[ DataEntryIndex ];
                CapturedDataEntry = *DataEntry;

                if (CapturedDataEntry.Size >= BufferSize) {

                    Status = STATUS_SUCCESS;
                }
            }
        }
    }

    if (!NT_SUCCESS( Status )) {

        LpcpReleaseLpcpLock();

        ObDereferenceObject( ClientThread );
        ObDereferenceObject( PortObject );

        return Status;
    }

    //
    //  Release the mutex that guards the LpcReplyMessage field
    //

    LpcpReleaseLpcpLock();

    //
    //  Copy the message data
    //

    if (WriteToMessageData) {

        Status = MmCopyVirtualMemory( PsGetCurrentProcess(),
                                      Buffer,
                                      THREAD_TO_PROCESS( ClientThread ),
                                      CapturedDataEntry.Base,
                                      BufferSize,
                                      PreviousMode,
                                      &BytesCopied );

    } else {

        Status = MmCopyVirtualMemory( THREAD_TO_PROCESS( ClientThread ),
                                      CapturedDataEntry.Base,
                                      PsGetCurrentProcess(),
                                      Buffer,
                                      BufferSize,
                                      PreviousMode,
                                      &BytesCopied );
    }

    if (ARGUMENT_PRESENT( NumberOfBytesCopied )) {

        try {

            *NumberOfBytesCopied = BytesCopied;

        } except( EXCEPTION_EXECUTE_HANDLER ) {

            NOTHING;
        }
    }

    //
    //  Dereference client thread and return the system service status.
    //

    ObDereferenceObject( ClientThread );
    ObDereferenceObject( PortObject );

    return Status;
}
