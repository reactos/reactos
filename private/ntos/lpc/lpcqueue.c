/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcqueue.c

Abstract:

    Local Inter-Process Communication (LPC) queue support routines.

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "lpcp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(INIT,LpcpInitializePortZone)
#pragma alloc_text(PAGE,LpcpInitializePortQueue)
#pragma alloc_text(PAGE,LpcpDestroyPortQueue)
#pragma alloc_text(PAGE,LpcpExtendPortZone)
#pragma alloc_text(PAGE,LpcpAllocateFromPortZone)
#pragma alloc_text(PAGE,LpcpFreeToPortZone)
#pragma alloc_text(PAGE,LpcpSaveDataInfoMessage)
#pragma alloc_text(PAGE,LpcpFreeDataInfoMessage)
#pragma alloc_text(PAGE,LpcpFindDataInfoMessage)
#endif


NTSTATUS
LpcpInitializePortQueue (
    IN PLPCP_PORT_OBJECT Port
    )

/*++

Routine Description:

    This routine is used to initialize the message queue for a port object.

Arguments:

    Port - Supplies the port object being initialized

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    PLPCP_NONPAGED_PORT_QUEUE NonPagedPortQueue;

    PAGED_CODE();

    //
    //  Allocate space for the port queue
    //

    NonPagedPortQueue = ExAllocatePoolWithTag( NonPagedPool,
                                               sizeof(LPCP_NONPAGED_PORT_QUEUE),
                                               'troP' );

    if (NonPagedPortQueue == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    //
    //  Initialize the fields in the non paged port queue
    //

    KeInitializeSemaphore( &NonPagedPortQueue->Semaphore, 0, 0x7FFFFFFF );

    NonPagedPortQueue->BackPointer = Port;

    //
    //  Have the port msg queue point to the non nonpaged port queue
    //

    Port->MsgQueue.Semaphore = &NonPagedPortQueue->Semaphore;

    //
    //  Initailize the port msg queue to be empty
    //

    InitializeListHead( &Port->MsgQueue.ReceiveHead );

    //
    //  And return to our caller
    //

    return STATUS_SUCCESS;
}


VOID
LpcpDestroyPortQueue (
    IN PLPCP_PORT_OBJECT Port,
    IN BOOLEAN CleanupAndDestroy
    )

/*++

Routine Description:

    This routine is used to teardown the message queue of a port object.
    After running this message will either be empty (like it was just
    initialized) or completly gone (needs to be initialized)

Arguments:

    Port - Supplies the port containing the message queue being modified

    CleanupAndDestroy - Specifies if the message queue should be set back
        to the freshly initialized state (value of FALSE) or completely
        torn down (value of TRUE)

Return Value:

    None.

--*/

{
    PLIST_ENTRY Next, Head;
    PETHREAD ThreadWaitingForReply;
    PLPCP_MESSAGE Msg;

    PAGED_CODE();

    //
    //  If this port is connected to another port, then disconnect it.
    //  Protect this with a lock in case the other side is going away
    //  at the same time.
    //

    LpcpAcquireLpcpLock();

    if (Port->ConnectedPort != NULL) {

        Port->ConnectedPort->ConnectedPort = NULL;
    }

    //
    //  If connection port, then mark name as deleted
    //

    if ((Port->Flags & PORT_TYPE) == SERVER_CONNECTION_PORT) {

        Port->Flags |= PORT_NAME_DELETED;
    }

    //
    //  Walk list of threads waiting for a reply to a message sent to this
    //  port.  Signal each thread's LpcReplySemaphore to wake them up.  They
    //  will notice that there was no reply and return
    //  STATUS_PORT_DISCONNECTED
    //

    Head = &Port->LpcReplyChainHead;
    Next = Head->Flink;

    while ((Next != NULL) && (Next != Head)) {

        ThreadWaitingForReply = CONTAINING_RECORD( Next, ETHREAD, LpcReplyChain );

        //
        //  If the thread is exiting, in the location of LpcReplyChain is stored the ExitTime
        //  We'll stop to search throught the list.

        if ( ThreadWaitingForReply->LpcExitThreadCalled ) {
            
            break;
        }

        Next = Next->Flink;

        RemoveEntryList( &ThreadWaitingForReply->LpcReplyChain );

        InitializeListHead( &ThreadWaitingForReply->LpcReplyChain );

        if (!KeReadStateSemaphore( &ThreadWaitingForReply->LpcReplySemaphore )) {

            //
            //  Thread is waiting on a message.  Signal the semaphore and free
            //  the message
            //

            Msg = ThreadWaitingForReply->LpcReplyMessage;

            if ( Msg ) {

                //
                //  If the message is a connection request and has a section object
                //  attached, then dereference that section object
                //

                if ((Msg->Request.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) == LPC_CONNECTION_REQUEST) {

                    PLPCP_CONNECTION_MESSAGE ConnectMsg;
                
                    ConnectMsg = (PLPCP_CONNECTION_MESSAGE)(Msg + 1);

                    if ( ConnectMsg->SectionToMap != NULL ) {

                        ObDereferenceObject( ConnectMsg->SectionToMap );
                    }
                }

                ThreadWaitingForReply->LpcReplyMessage = NULL;

                LpcpFreeToPortZone( Msg, TRUE );
            }

            ThreadWaitingForReply->LpcReplyMessageId = 0;

            KeReleaseSemaphore( &ThreadWaitingForReply->LpcReplySemaphore,
                                0,
                                1L,
                                FALSE );
        }
    }

    InitializeListHead( &Port->LpcReplyChainHead );

    //
    //  Walk list of messages queued to this port.  Remove each message from
    //  the list and free it.
    //

    Head = &Port->MsgQueue.ReceiveHead;
    Next = Head->Flink;

    while ((Next != NULL) && (Next != Head)) {

        Msg  = CONTAINING_RECORD( Next, LPCP_MESSAGE, Entry );

        Next = Next->Flink;

        InitializeListHead( &Msg->Entry );

        LpcpFreeToPortZone( Msg, TRUE );
    }

    //
    //  Reinitialize the message queue
    //

    InitializeListHead( &Port->MsgQueue.ReceiveHead );

    LpcpReleaseLpcpLock();

    //
    //  Check if the caller wants it all to go away
    //

    if ( CleanupAndDestroy ) {

        //
        //  Free semaphore associated with the queue.
        //

        if (Port->MsgQueue.Semaphore != NULL) {

            ExFreePool( CONTAINING_RECORD( Port->MsgQueue.Semaphore,
                                           LPCP_NONPAGED_PORT_QUEUE,
                                           Semaphore ));
        }
    }

    //
    //  And return to our caller
    //

    return;
}


NTSTATUS
LpcpInitializePortZone (
    IN ULONG MaxEntrySize,
    IN ULONG SegmentSize,
    IN ULONG MaxPoolUsage
    )

/*++

Routine Description:

    This routine only executes once.  It is used to initialize the zone
    allocator for LPC messages

Arguments:

    MaxEntrySize - Specifies the maximum size, in bytes, that we'll allocate
        from the zone.

    SegmentSize - Specifies the number of total bytes to use in the initial
        zone allocation.  This is also the increment use to grow the zone.

    MaxPoolUsage - Specifies the maximum number of bytes we every want to
        zone to grow to.

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    NTSTATUS Status;
    PVOID Segment;
    PLPCP_MESSAGE Msg;
    LONG SegSize;

    PAGED_CODE();

    //
    //  Set the sizes in the global port zone variable
    //

    LpcpZone.MaxPoolUsage = MaxPoolUsage;
    LpcpZone.GrowSize = SegmentSize;

    //
    //  Allocate and initialize the initial zone
    //

    Segment = ExAllocatePoolWithTag( PagedPool, SegmentSize, 'ZcpL' );

    if (Segment == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    KeInitializeEvent( &LpcpZone.FreeEvent, SynchronizationEvent, FALSE );

    Status = ExInitializeZone( &LpcpZone.Zone,
                               MaxEntrySize,
                               Segment,
                               SegmentSize );

    if (!NT_SUCCESS( Status )) {

        ExFreePool( Segment );
    }

    //
    //  The following code sets up each msg in the zone by filling in its
    //  index, and zeroing the rest.
    //
    //  **** note, this is really a backdoor piece of code.  First, it assumes
    //       segsize is pagesize, and second, it assume the zone doesn't touch
    //       those fields for its own bookkeeping.
    //

    SegSize = PAGE_SIZE;
    LpcpTotalNumberOfMessages = 0;

    //
    //  Skip over the segment header
    //

    Msg = (PLPCP_MESSAGE)((PZONE_SEGMENT_HEADER)Segment + 1);

    //
    //  For each block in the zone pointed at by Msg, pre-initialize the Msg
    //

    while (SegSize >= (LONG)LpcpZone.Zone.BlockSize) {

        Msg->ZoneIndex = (USHORT)++LpcpTotalNumberOfMessages;
        Msg->Reserved0 = 0;
        Msg->Request.MessageId = 0;

        Msg = (PLPCP_MESSAGE)((PCHAR)Msg + LpcpZone.Zone.BlockSize);

        SegSize -= LpcpZone.Zone.BlockSize;
    }

    //
    //  And return to our caller
    //

    return Status;
}


NTSTATUS
LpcpExtendPortZone (
    VOID
    )

/*++

Routine Description:

    This routine is used to increase the size of the global port zone.

    Note that our caller must have already acquired the LpcpLock mutex.

Arguments:

    None.

Return Value:

    NTSTATUS - An appropriate status value

--*/

{
    NTSTATUS Status;
    PVOID Segment;
    PLPCP_MESSAGE Msg;
    LARGE_INTEGER WaitTimeout;
    BOOLEAN AlreadyRetried;
    LONG SegmentSize;

    PAGED_CODE();

    AlreadyRetried = FALSE;

retry:

    //
    //  If we're going to grow across a threshold, we should wait a little in case
    //  an entry gets freed.
    //

    if ((LpcpZone.Zone.TotalSegmentSize + LpcpZone.GrowSize) > LpcpZone.MaxPoolUsage) {

        LpcpPrint(( "Out of space in global LPC zone - current size is %08x\n",
                    LpcpZone.Zone.TotalSegmentSize ));

        //
        //  Wait time is 1 second.
        //
        //  We'll actually only go through this path once because on retry we
        //  bump up max pool usage to avoid this wait
        //

        WaitTimeout.QuadPart = Int32x32To64( 1000, -10000 );

        LpcpReleaseLpcpLock();

        Status = KeWaitForSingleObject( &LpcpZone.FreeEvent,
                                        Executive,
                                        KernelMode,
                                        FALSE,
                                        &WaitTimeout );

        LpcpAcquireLpcpLock();

        if (Status != STATUS_SUCCESS) {

            LpcpPrint(( "Error waiting for %lx->FreeEvent - Status == %X\n",
                        &LpcpZone,
                        Status ));

            if ( !AlreadyRetried ) {

                AlreadyRetried = TRUE;

                LpcpZone.MaxPoolUsage += LpcpZone.GrowSize;

                goto retry;
            }
        }

        //
        //  return to our caller and let the caller retry the allocation
        //  again
        //

        return Status;
    }

    //
    //  We need to extend the zone, so allocate another chunk and add it
    //  to the zone
    //

    Segment = ExAllocatePoolWithTag( PagedPool, LpcpZone.GrowSize, 'ZcpL' );

    if (Segment == NULL) {

        return STATUS_INSUFFICIENT_RESOURCES;
    }

    Status = ExExtendZone( &LpcpZone.Zone,
                           Segment,
                           LpcpZone.GrowSize );

    if (!NT_SUCCESS( Status )) {

        ExFreePool( Segment );
    }

#if DEVL

    else {

        LpcpTrace(( "Extended LPC zone by %x for a total of %x\n",
                    LpcpZone.GrowSize, LpcpZone.Zone.TotalSegmentSize ));

        //
        //  The following code sets up each msg in the zone by filling in its
        //  index, and zeroing the rest.
        //
        //  **** note, this is really a backdoor piece of code.  First, it assumes
        //       we grew by one page, and second, it assume the zone doesn't touch
        //       those fields for its own bookkeeping.
        //

        SegmentSize = PAGE_SIZE;

        Msg = (PLPCP_MESSAGE)((PZONE_SEGMENT_HEADER)Segment + 1);

        while (SegmentSize >= (LONG)LpcpZone.Zone.BlockSize) {

            Msg->ZoneIndex = (USHORT)++LpcpTotalNumberOfMessages;

            Msg = (PLPCP_MESSAGE)((PCHAR)Msg + LpcpZone.Zone.BlockSize);

            SegmentSize -= LpcpZone.Zone.BlockSize;
        }

        //
        //  Now that we've completely initialized the new segment, go
        //  and wake up any waiters
        //

        LpcpReleaseLpcpLock();

        KeSetEvent( &LpcpZone.FreeEvent,
                    LPC_RELEASE_WAIT_INCREMENT,
                    FALSE );

        LpcpAcquireLpcpLock();
    }

#endif

    return Status;
}


PLPCP_MESSAGE
FASTCALL
LpcpAllocateFromPortZone (
    ULONG Size
    )

/*++

Routine Description:

    This procedure is used to allocate a new msg from the global port zone.

    Note that we assume our caller owns the LpcpLock mutex.

Arguments:

    Size - Specifies the size, in bytes, needed for the allocation.  Currently
        this variable is ignored

Return Value:

    PLPCP_MESSAGE - returns a pointer to the newly allocate message

--*/

{
    NTSTATUS Status;
    PLPCP_MESSAGE Msg;

    PAGED_CODE();

    //
    //  Continue looping until we get a message to give back or until when we
    //  extend the zone we get back and error
    //

    do {

        //
        //  Pick off the next message from the zone and if we actually
        //  did get one then initialize some of its fields and return it
        //  to our caller
        //

        Msg = (PLPCP_MESSAGE)ExAllocateFromZone( &LpcpZone.Zone );

        if (Msg != NULL) {

            LpcpTrace(( "Allocate Msg %lx\n", Msg ));

            InitializeListHead( &Msg->Entry );

            Msg->RepliedToThread = NULL;

#if DBG
            //
            //  In the debug case mark the message as allocated
            //

            Msg->ZoneIndex |= LPCP_ZONE_MESSAGE_ALLOCATED;
#endif

            return Msg;
        }

        //
        //  The zone didn't give us an entry so extend the zone and try the
        //  allocation again
        //

        LpcpTrace(( "Extending Zone %lx\n", &LpcpZone.Zone ));

        Status = LpcpExtendPortZone( );

    } while (NT_SUCCESS(Status));

    //
    //  Return NULL (i.e., an error) to our caller
    //

    return NULL;
}


VOID
FASTCALL
LpcpFreeToPortZone (
    IN PLPCP_MESSAGE Msg,
    IN BOOLEAN MutexOwned
    )

/*++

Routine Description:

    This routine is used to free an old msg back to the global message queue

Arguments:

    Msg - Supplies the message being returned

    MutexOwned - Specifies if the LpcpLock is already owned by the caller

Return Value:

    None.

--*/

{
    BOOLEAN ZoneMemoryAvailable = FALSE;
    PLPCP_CONNECTION_MESSAGE ConnectMsg;

    PAGED_CODE();

    //
    //  Take out the global lock if necessary
    //

    if (!MutexOwned) {

        LpcpAcquireLpcpLock();
    }

    LpcpTrace(( "Free Msg %lx\n", Msg ));

#if DBG

    //
    //  In the debug case if the message is not allocated then it's an error
    //

    if (!(Msg->ZoneIndex & LPCP_ZONE_MESSAGE_ALLOCATED)) {

        LpcpPrint(( "Msg %lx has already been freed.\n", Msg ));
        DbgBreakPoint();

        if (!MutexOwned) {

            LpcpReleaseLpcpLock();
        }

        return;
    }

    Msg->ZoneIndex &= ~LPCP_ZONE_MESSAGE_ALLOCATED;

#endif

    //
    //  The non zero reserved0 field tells us that the message has been used
    //

    if (Msg->Reserved0 != 0) {

        //
        //  A entry field connects the message to the message queue of the
        //  owning port object.  If not already removed then remove this
        //  message
        //

        if (!IsListEmpty( &Msg->Entry )) {

            RemoveEntryList( &Msg->Entry );

            InitializeListHead( &Msg->Entry );
        }

        //
        //  If the replied to thread is not null then we have a reference
        //  to the thread that we should now remove
        //

        if (Msg->RepliedToThread != NULL) {

            ObDereferenceObject( Msg->RepliedToThread );

            Msg->RepliedToThread = NULL;
        }

        //
        //  If the msg was for a connection request then we know that
        //  right after the lpcp message is a connection message whose
        //  client port field might need to be dereferenced
        //

        if ((Msg->Request.u2.s2.Type & ~LPC_KERNELMODE_MESSAGE) == LPC_CONNECTION_REQUEST) {

            ConnectMsg = (PLPCP_CONNECTION_MESSAGE)(Msg + 1);

            if (ConnectMsg->ClientPort) {

                PLPCP_PORT_OBJECT ClientPort;

                //
                //  Capture a pointer to the client port then null it
                //  out so that no one else can use it, then release
                //  lpcp lock before we dereference the client port
                //

                ClientPort = ConnectMsg->ClientPort;

                ConnectMsg->ClientPort = NULL;

                LpcpReleaseLpcpLock();

                ObDereferenceObject( ClientPort );

                LpcpAcquireLpcpLock();
            }
        }

        //
        //  Now mark this message a free, and return it to the zone
        //

        Msg->Reserved0 = 0;
        ZoneMemoryAvailable = (BOOLEAN)(ExFreeToZone( &LpcpZone.Zone, &Msg->FreeEntry ) == NULL);
    }

    //
    //  Check if we need to release the global lpcp lock
    //

    if (!MutexOwned) {

        LpcpReleaseLpcpLock();
    }

    //
    //  If we've actually freed up memory then go wake any waiting allocators
    //
    //  **** does this need to check to see if it owns the lpcp lock.  The
    //       release right above us and this test and set event should be
    //       combined
    //

    if (ZoneMemoryAvailable) {

        KeSetEvent( &LpcpZone.FreeEvent,
                    LPC_RELEASE_WAIT_INCREMENT,
                    FALSE );
    }

    //
    //  And return to our caller
    //

    return;
}


VOID
LpcpSaveDataInfoMessage (
    IN PLPCP_PORT_OBJECT Port,
    PLPCP_MESSAGE Msg
    )

/*++

Routine Description:

    This routine is used in place of freeing a message and instead saves the
    message off a seperate queue from the port.

Arguments:

    Port - Specifies the port object under which to save this message

    Msg - Supplies the message being saved

Return Value:

    None.

--*/

{
    PAGED_CODE();

    //
    //  Take out the global lock
    //

    LpcpAcquireLpcpLock();

    //
    //  Make sure we get to the connection port object of this port
    //

    if ((Port->Flags & PORT_TYPE) > UNCONNECTED_COMMUNICATION_PORT) {

        Port = Port->ConnectionPort;
    }

    LpcpTrace(( "%s Saving DataInfo Message %lx (%u.%u)  Port: %lx\n",
                PsGetCurrentProcess()->ImageFileName,
                Msg,
                Msg->Request.MessageId,
                Msg->Request.CallbackId,
                Port ));

    //
    //  Enqueue this message onto the data info chain for the port
    //

    InsertTailList( &Port->LpcDataInfoChainHead, &Msg->Entry );

    //
    //  Free the global lock
    //

    LpcpReleaseLpcpLock();

    //
    //  And return to our caller
    //

    return;
}


VOID
LpcpFreeDataInfoMessage (
    IN PLPCP_PORT_OBJECT Port,
    IN ULONG MessageId,
    IN ULONG CallbackId
    )

/*++

Routine Description:

    This routine is used to free up a saved message in a port

Arguments:

    Port - Supplies the port being manipulated

    MessageId - Supplies the id of the message being freed

    CallbackId - Supplies the callback id of the message being freed

Return Value:

    None.

--*/

{
    PLPCP_MESSAGE Msg;
    PLIST_ENTRY Head, Next;

    PAGED_CODE();

    //
    //  Make sure we get to the connection port object of this port
    //

    if ((Port->Flags & PORT_TYPE) > UNCONNECTED_COMMUNICATION_PORT) {

        Port = Port->ConnectionPort;
    }

    //
    //  Zoom down the data info chain for the connection port object
    //

    Head = &Port->LpcDataInfoChainHead;
    Next = Head->Flink;

    while (Next != Head) {

        Msg = CONTAINING_RECORD( Next, LPCP_MESSAGE, Entry );

        //
        //  If this message matches the callers specification then remove
        //  this message, free it back to the port zone, and return back
        //  to our caller
        //

        if ((Msg->Request.MessageId == MessageId) &&
            (Msg->Request.CallbackId == CallbackId)) {

            LpcpTrace(( "%s Removing DataInfo Message %lx (%u.%u) Port: %lx\n",
                        PsGetCurrentProcess()->ImageFileName,
                        Msg,
                        Msg->Request.MessageId,
                        Msg->Request.CallbackId,
                        Port ));

            RemoveEntryList( &Msg->Entry );

            InitializeListHead( &Msg->Entry );

            LpcpFreeToPortZone( Msg, TRUE );

            return;

        } else {

            //
            //  Keep on going down the data info chain
            //

            Next = Next->Flink;
        }
    }

    //
    //  We didn't find a match so just return to our caller
    //

    LpcpTrace(( "%s Unable to find DataInfo Message (%u.%u)  Port: %lx\n",
                PsGetCurrentProcess()->ImageFileName,
                MessageId,
                CallbackId,
                Port ));

    return;
}


PLPCP_MESSAGE
LpcpFindDataInfoMessage (
    IN PLPCP_PORT_OBJECT Port,
    IN ULONG MessageId,
    IN ULONG CallbackId
    )

/*++

Routine Description:

    This routine is used to locate a specific message stored off the
    data info chain of a port

Arguments:

    Port - Supplies the port being examined

    MessageId - Supplies the ID of the message being searched for

    CallbackId - Supplies the callback ID being searched for

Return Value:

    PLPCP_MESSAGE - returns a pointer to the message satisfying the
        search criteria or NULL of none was found

--*/

{
    PLPCP_MESSAGE Msg;
    PLIST_ENTRY Head, Next;

    PAGED_CODE();

    //
    //  Make sure we get to the connection port object of this port
    //

    if ((Port->Flags & PORT_TYPE) > UNCONNECTED_COMMUNICATION_PORT) {

        Port = Port->ConnectionPort;
    }

    //
    //  Zoom down the data info chain for the connection port object looking
    //  for a match
    //

    Head = &Port->LpcDataInfoChainHead;
    Next = Head->Flink;

    while (Next != Head) {

        Msg = CONTAINING_RECORD( Next, LPCP_MESSAGE, Entry );

        if ((Msg->Request.MessageId == MessageId) &&
            (Msg->Request.CallbackId == CallbackId)) {

            LpcpTrace(( "%s Found DataInfo Message %lx (%u.%u)  Port: %lx\n",
                        PsGetCurrentProcess()->ImageFileName,
                        Msg,
                        Msg->Request.MessageId,
                        Msg->Request.CallbackId,
                        Port ));

            return Msg;

        } else {

            Next = Next->Flink;
        }
    }

    //
    //  We did not find a match so return null to our caller
    //

    LpcpTrace(( "%s Unable to find DataInfo Message (%u.%u)  Port: %lx\n",
                PsGetCurrentProcess()->ImageFileName,
                MessageId,
                CallbackId,
                Port ));

    return NULL;
}
