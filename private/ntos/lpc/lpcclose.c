/*++

Copyright (c) 1989  Microsoft Corporation

Module Name:

    lpcclose.c

Abstract:

    Local Inter-Process Communication close procedures that are called when
    a connection port or a communications port is closed.

Author:

    Steve Wood (stevewo) 15-May-1989

Revision History:

--*/

#include "lpcp.h"

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE,LpcpDeletePort)
#pragma alloc_text(PAGE,LpcExitThread)
#endif


VOID
LpcpClosePort (
    IN PEPROCESS Process OPTIONAL,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    )

/*++

Routine Description:

    This routine is the callback used for closing a port object.

Arguments:

    Process - Supplies an optional pointer to the process whose port is being
        closed

    Object - Supplies a pointer to the port object being closed

    GrantedAccess - Supplies the access granted to the handle closing port
        object

    ProcessHandleCount - Supplies the number of process handles remaining to
        the object

    SystemHandleCount - Supplies the number of system handles remaining to
        the object

Return Value:

    None.

--*/

{
    //
    //  Translate the object to what it really is, an LPCP port object
    //

    PLPCP_PORT_OBJECT Port = Object;

    //
    //  We only have work to do if the object is a server communication port
    //

    if ( (Port->Flags & PORT_TYPE) == SERVER_CONNECTION_PORT ) {

        //
        //  If this is a server commucation port without any system handles
        //  then we can completely destroy the communication queue for the
        //  port
        //

        if ( SystemHandleCount == 0 ) {

            LpcpDestroyPortQueue( Port, TRUE );

        //
        //  If there is only one system handle left then we'll reset the
        //  communication queue for the port
        //

        } else if ( SystemHandleCount == 1 ) {

            LpcpDestroyPortQueue( Port, FALSE );
        }

        //
        //  Otherwise we do nothing
        //
    }

    return;
}


VOID
LpcpDeletePort (
    IN PVOID Object
    )

/*++

Routine Description:

    This routine is the callback used for deleting a port object.

Arguments:

    Object - Supplies a pointer to the port object being deleted

Return Value:

    None.

--*/

{
    PLPCP_PORT_OBJECT Port = Object;
    PLPCP_PORT_OBJECT ConnectionPort;
    LPC_CLIENT_DIED_MSG ClientPortClosedDatagram;
    PLPCP_MESSAGE Msg;
    PLIST_ENTRY Head, Next;
    HANDLE CurrentProcessId;

    PAGED_CODE();

    //
    //  If the port is a server communication port then make sure that if
    //  there is a dangling client thread that we get rid of it.  This
    //  handles the case of someone calling NtAcceptConnectPort and not
    //  calling NtCompleteConnectPort
    //

    LpcpPortExtraDataDestroy( Port );

    if ((Port->Flags & PORT_TYPE) == SERVER_COMMUNICATION_PORT) {

        PETHREAD ClientThread;

        LpcpAcquireLpcpLock();

        if ((ClientThread = Port->ClientThread) != NULL) {

            Port->ClientThread = NULL;

            LpcpReleaseLpcpLock();

            ObDereferenceObject( ClientThread );

        } else {

            LpcpReleaseLpcpLock();
        }
    }

    //
    //  Send an LPC_PORT_CLOSED datagram to whoever is connected
    //  to this port so they know they are no longer connected.
    //

    if ((Port->Flags & PORT_TYPE) == CLIENT_COMMUNICATION_PORT) {

        ClientPortClosedDatagram.PortMsg.u1.s1.TotalLength = sizeof( ClientPortClosedDatagram );
        ClientPortClosedDatagram.PortMsg.u1.s1.DataLength = sizeof( ClientPortClosedDatagram.CreateTime );

        ClientPortClosedDatagram.PortMsg.u2.s2.Type = LPC_PORT_CLOSED;
        ClientPortClosedDatagram.PortMsg.u2.s2.DataInfoOffset = 0;

        ClientPortClosedDatagram.CreateTime = PsGetCurrentProcess()->CreateTime;

        LpcRequestPort( Port, (PPORT_MESSAGE)&ClientPortClosedDatagram );
    }

    //
    //  If connected, disconnect the port, and then scan the message queue
    //  for this port and dereference any messages in the queue.
    //

    LpcpDestroyPortQueue( Port, TRUE );

    //
    //  If the client has a port memory view, then unmap it
    //

    if (Port->ClientSectionBase != NULL) {

        MmUnmapViewOfSection( PsGetCurrentProcess(),
                              Port->ClientSectionBase );

    }

    //
    //  If the server has a port memory view, then unmap it
    //

    if (Port->ServerSectionBase != NULL) {

        MmUnmapViewOfSection( PsGetCurrentProcess(),
                              Port->ServerSectionBase  );

    }

    //
    //  Dereference the pointer to the connection port if it is not
    //  this port.
    //

    if (ConnectionPort = Port->ConnectionPort) {

        CurrentProcessId = PsGetCurrentThread()->Cid.UniqueProcess;

        LpcpAcquireLpcpLock();

        Head = &ConnectionPort->LpcDataInfoChainHead;
        Next = Head->Flink;

        while (Next != Head) {

            Msg = CONTAINING_RECORD( Next, LPCP_MESSAGE, Entry );
            Next = Next->Flink;

            if (Msg->Request.ClientId.UniqueProcess == CurrentProcessId) {

                LpcpTrace(( "%s Freeing DataInfo Message %lx (%u.%u)  Port: %lx\n",
                            PsGetCurrentProcess()->ImageFileName,
                            Msg,
                            Msg->Request.MessageId,
                            Msg->Request.CallbackId,
                            ConnectionPort ));

                RemoveEntryList( &Msg->Entry );

                LpcpFreeToPortZone( Msg, TRUE );
            }
        }

        LpcpReleaseLpcpLock();

        if (ConnectionPort != Port) {

            ObDereferenceObject( ConnectionPort );
        }
    }

    if (((Port->Flags & PORT_TYPE) == SERVER_CONNECTION_PORT) &&
        (ConnectionPort->ServerProcess != NULL)) {

        ObDereferenceObject( ConnectionPort->ServerProcess );

        ConnectionPort->ServerProcess = NULL;
    }

    //
    //  Free any static client security context
    //

    LpcpFreePortClientSecurity( Port );

    //
    //  And return to our caller
    //

    return;
}


VOID
LpcExitThread (
    PETHREAD Thread
    )

/*++

Routine Description:

    This routine is called whenever a thread is exiting and need to cleanup the
    lpc port for the thread.

Arguments:

    Thread - Supplies the thread being terminated

Return Value:

    None.

--*/

{
    PLPCP_MESSAGE Msg;

    //
    //  Acquire the mutex that protects the LpcReplyMessage field of
    //  the thread.  Zero the field so nobody else tries to process it
    //  when we release the lock.
    //

    LpcpAcquireLpcpLock();

    if (!IsListEmpty( &Thread->LpcReplyChain )) {

        RemoveEntryList( &Thread->LpcReplyChain );
    }

    //
    //  Indicate that this thread is exiting
    //

    Thread->LpcExitThreadCalled = TRUE;
    Thread->LpcReplyMessageId = 0;

    //
    //  If we need to reply to a message then if the thread that we need to reply
    //  to is still around we want to dereference the thread and free the message
    //

    Msg = Thread->LpcReplyMessage;

    if (Msg != NULL) {

        Thread->LpcReplyMessage = NULL;

        if (Msg->RepliedToThread != NULL) {

            ObDereferenceObject( Msg->RepliedToThread );

            Msg->RepliedToThread = NULL;
        }

        LpcpTrace(( "Cleanup Msg %lx (%d) for Thread %lx allocated\n", Msg, IsListEmpty( &Msg->Entry ), Thread ));

        LpcpFreeToPortZone( Msg, TRUE );
    }

    //
    //  Free the global lpc lock
    //

    LpcpReleaseLpcpLock();

    //
    //  And return to our caller
    //

    return;
}
