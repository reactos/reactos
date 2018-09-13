/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    channel.c

Abstract:

   This module implements the executive channel object. Channel obects
   provide a very high speed local interprocess communication mechanism.

Author:

    David N. Cutler (davec) 26-Mar-1995

Environment:

    Kernel mode only.

Revision History:

--*/

#include "ki.h"

//
// Define local function prototypes.
//

VOID
KiAllocateReceiveBufferChannel (
    VOID
    );

VOID
KiCloseChannel (
    IN PEPROCESS Process,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    );

VOID
KiDeleteChannel (
    IN PVOID Object
    );

NTSTATUS
KiListenChannel (
    IN PRECHANNEL ServerChannel,
    IN KPROCESSOR_MODE WaitMode,
    OUT PCHANNEL_MESSAGE *Message
    );

PKTHREAD
KiRendezvousWithThread (
    IN PRECHANNEL WaitChannel,
    IN ULONG WaitMode
    );

//
// Address of event object type descriptor.
//

POBJECT_TYPE KeChannelType;

//
// Structure that describes the mapping of generic access rights to object
// specific access rights for event objects.
//

GENERIC_MAPPING KiChannelMapping = {
    STANDARD_RIGHTS_READ |
        CHANNEL_READ_MESSAGE,
    STANDARD_RIGHTS_WRITE |
        CHANNEL_WRITE_MESSAGE,
    STANDARD_RIGHTS_EXECUTE |
        SYNCHRONIZE,
    CHANNEL_ALL_ACCESS
};

//
// Define function sections.
//

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, KiAllocateReceiveBufferChannel)
#pragma alloc_text(INIT, KiChannelInitialization)
#pragma alloc_text(PAGE, KiDeleteChannel)
#pragma alloc_text(PAGE, KiRundownChannel)
#pragma alloc_text(PAGE, NtCreateChannel)
#pragma alloc_text(PAGE, NtListenChannel)
#pragma alloc_text(PAGE, NtOpenChannel)
#pragma alloc_text(PAGE, NtSetContextChannel)
#endif

NTSTATUS
NtCreateChannel (
    OUT PHANDLE ChannelHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL
    )

/*++

Routine Description:

    This function creates a server listen channel object and opens a handle
    to the object with the specified desired access.

Arguments:

    ChannelHandle - Supplies a pointer to a variable that will receive the
        channel object handle.

    ObjectAttributes - Supplies a pointer to an object attributes structure.

Return Value:

    If the channel object is created, then a success status is returned.
    Otherwise, a failure status is returned.

--*/

{

#if 0

    PVOID ChannelObject;
    KPROCESSOR_MODE PreviousMode;
    PRECHANNEL ServerChannel;
    HANDLE ServerHandle;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe and zero the output handle
    // address, and attempt to create a channel object. If the probe fails
    // or access to the object attributes fails, then return the exception
    // code as the service status.
    //

    PreviousMode = KeGetPreviousMode();
    try {

        //
        // Get previous processor mode and probe output handle address if
        // necessary.
        //

        if (PreviousMode != KernelMode) {
            ProbeAndZeroHandle(ChannelHandle);
        }

        //
        // Allocate channel object.
        //

        Status = ObCreateObject(PreviousMode,
                                KeChannelType,
                                ObjectAttributes,
                                PreviousMode,
                                NULL,
                                sizeof(ECHANNEL),
                                0,
                                0,
                                &ChannelObject);

    } except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // If the channel object was successfully created, then initialize the
    // channel object and insert the channel object in the process handle
    // table.
    //

    if (NT_SUCCESS(Status)) {
        ServerChannel = (PRECHANNEL)ChannelObject;
        ServerChannel->Type = LISTEN_CHANNEL;
        ServerChannel->State = ServerIdle;
        ServerChannel->OwnerProcess = &PsGetCurrentProcess()->Pcb;
        ServerChannel->ClientThread = NULL;
        ServerChannel->ServerThread = NULL;
        ServerChannel->ServerContext = NULL;
        ServerChannel->ServerChannel = NULL;
        KeInitializeEvent(&ServerChannel->ReceiveEvent,
                          SynchronizationEvent,
                          FALSE);

        KeInitializeEvent(&ServerChannel->ClearToSendEvent,
                          SynchronizationEvent,
                          FALSE);

        Status = ObInsertObject(ServerChannel,
                                NULL,
                                CHANNEL_ALL_ACCESS,
                                0,
                                NULL,
                                &ServerHandle);

        //
        // If the channel object was successfully inserted in the process
        // handle table, then attempt to write the channel object handle
        // value. If the write attempt fails, then do not report an error.
        // When the caller attempts to access the handle value, an access
        // violation will occur.
        //

        if (NT_SUCCESS(Status)) {
            try {
                *ChannelHandle = ServerHandle;

            } except(ExSystemExceptionFilter()) {
            }
        }
    }

    //
    // Return service status.
    //

    return Status;

#else

    return STATUS_NOT_IMPLEMENTED;

#endif

}

NTSTATUS
NtListenChannel (
    IN HANDLE ChannelHandle,
    OUT PCHANNEL_MESSAGE *Message
    )

/*++

Routine Description:

    This function listens for a client message.

    N.B. This function can only be executed from a server thread.

Arguments:

    ChannelHandle - Supplies a handle to a listen channel on which the
        server thread listens.

    Message - Supplies a pointer to a variable that receives a pointer
        to the client message header.

Return Value:

    If the function is successfully completed, then a success status is
    returned. Otherwise, a failure status is returned.

--*/

{

#if 0

    KPROCESSOR_MODE PreviousMode;
    PRECHANNEL ServerChannel;
    PRKTHREAD ServerThread;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the output message address,
    // and allocate a receive buffer if necessary. If the probe fails or
    // the receive buffer allocation is not successful, then return the
    // exception code as the service status.
    //

    ServerThread = KeGetCurrentThread();
    try {

        //
        // Get previous processor mode and probe output message address.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeAndNullPointer(Message);
        }

        //
        // If the current thread does not have an associated receive buffer,
        // then attempt to allocate one now. If the allocation fails, then
        // an exception is raised.
        //

        if (ServerThread->Section == NULL) {
            KiAllocateReceiveBufferChannel();
        }

    } except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // Reference channel object by handle.
    //

    Status = ObReferenceObjectByHandle(ChannelHandle,
                                       CHANNEL_ALL_ACCESS,
                                       KeChannelType,
                                       PreviousMode,
                                       &ServerChannel,
                                       NULL);

    //
    // If the reference was successful and the channel is a listen channel,
    // then wait for a client message to arrive.
    //

    if (NT_SUCCESS(Status)) {
        if (ServerChannel->ServerChannel != NULL) {
            Status = STATUS_INVALID_PARAMETER; // **** fix ****

        } else {
            Status = KiListenChannel(ServerChannel, PreviousMode, Message);
        }

        ObDereferenceObject(ServerChannel);
    }

    //
    // Return service status.
    //

    return Status;

#else

    return STATUS_NOT_IMPLEMENTED;

#endif

}

NTSTATUS
NtOpenChannel (
    OUT PHANDLE ChannelHandle,
    IN POBJECT_ATTRIBUTES ObjectAttributes
    )

/*++

Routine Description:

    This function opens a handle to a server channel by creating a message
    channel that is connected to the specified server channel.

Arguments:

    ChannelHandle - Supplies a pointer to a variable that will receive the
        channel object handle.

    ObjectAttributes - Supplies a pointer to an object attributes structure.

Return Value:

    If the channel object is opened, then a success status is returned.
    Otherwise, a failure status is returned.

--*/

{

#if 0

    PRECHANNEL ClientChannel;
    HANDLE ClientHandle;
    PKTHREAD ClientThread;
    KPROCESSOR_MODE PreviousMode;
    PRECHANNEL ServerChannel;
    PVOID ServerObject;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe and zero the output handle
    // address, and attempt to open the server channel object. If the
    // probe fails, then return the exception code as the service status.
    //

    try {

        //
        // Get previous processor mode and probe output handle address
        // if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeAndZeroHandle(ChannelHandle);
        }

        //
        // Reference the server channel object with the specified desired
        // access.
        //

        Status = ObReferenceObjectByName(ObjectAttributes->ObjectName,
                                        ObjectAttributes->Attributes,
                                        NULL,
                                        CHANNEL_ALL_ACCESS,
                                        KeChannelType,
                                        PreviousMode,
                                        NULL,
                                        (PVOID *)&ServerObject);

    } except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // If the reference was successful, then attempt to create a client
    // channel object.
    //

    if (NT_SUCCESS(Status)) {

        //
        // If the owner process of the server channel is the same as
        // the current process, then a server thread is attempting to
        // open a client handle. This is not allowed since it would
        // not be possible to distinguish the server from the cient.
        //

        ClientThread = KeGetCurrentThread();
        ServerChannel = (PECHANNEL)ServerObject;
        if (ServerChannel->OwnerProcess == ClientThread->ApcState.Process) {
            Status = STATUS_INVALID_PARAMETER; // **** fix ***

        } else {
            Status = ObCreateObject(PreviousMode,
                                    KeChannelType,
                                    NULL,
                                    PreviousMode,
                                    NULL,
                                    sizeof(ECHANNEL),
                                    0,
                                    0,
                                    (PVOID *)&ClientChannel);

            //
            // If the channel object was successfully created, then
            // initialize the channel object and attempt to insert the
            // channel object in the server process channel table.
            //

            if (NT_SUCCESS(Status)) {
                ClientChannel->Type = MESSAGE_CHANNEL;
                ClientChannel->State = ClientIdle;
                ClientChannel->OwnerProcess = &PsGetCurrentProcess()->Pcb;
                ClientChannel->ClientThread = NULL;
                ClientChannel->ServerThread = NULL;
                ClientChannel->ServerContext = NULL;
                ClientChannel->ServerChannel = ServerChannel;
                KeInitializeEvent(&ClientChannel->ReceiveEvent,
                                  SynchronizationEvent,
                                  FALSE);

                KeInitializeEvent(&ClientChannel->ClearToSendEvent,
                                  SynchronizationEvent,
                                  FALSE);

                //
                // Create a handle to the message channel object.
                //

                Status = ObInsertObject(ClientChannel,
                                        NULL,
                                        CHANNEL_ALL_ACCESS,
                                        0,
                                        NULL,
                                        &ClientHandle);

                //
                // If the channel object was successfully inserted in the
                // client process handle table, then attempt to write the
                // client channel object handle value. If the write attempt
                // fails, then do not report an error. When the caller
                // attempts to access the handle value, an access violation
                // will occur.
                //

                if (NT_SUCCESS(Status)) {
                    try {
                        *ChannelHandle = ClientHandle;

                    } except(ExSystemExceptionFilter()) {
                    }

                }

                return Status;
            }
        }

        ObDereferenceObject(ServerChannel);
    }

    //
    // Return service status.
    //

    return Status;

#else

    return STATUS_NOT_IMPLEMENTED;

#endif

}

NTSTATUS
NtReplyWaitSendChannel (
    IN PVOID Text,
    IN ULONG Length,
    OUT PCHANNEL_MESSAGE *Message
    )

/*++

Routine Description:

    This function sends a reply message to a client and waits for a send.

    N.B. This function can only be executed from a server thread that
        has an assoicated message channel.

Arguments:

    Text - Supplies a pointer to the message text.

    Length - Supplies the length of the message text.

    Message - Supplies a pointer to a variable that receives the send
        message header.

Return Value:

    If the function is successfully completed, then a succes status is
    returned. Otherwise, a failure status is returned.

--*/

{

#if 0

    PKTHREAD ClientThread;
    PCHANNEL_MESSAGE ClientView;
    PRECHANNEL MessageChannel;
    KPROCESSOR_MODE PreviousMode;
    PECHANNEL ServerChannel;
    PKTHREAD ServerThread;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the output message address,
    // probe the message text, and allocate a receive buffer if necessary.
    // If either of the probes fail or the receive buffer allocation is
    // not successful, then return the exception code as the service
    // status.
    //

    ServerThread = KeGetCurrentThread();
    try {

        //
        // Get previous processor mode and probe output message address and
        // the message text if necessary.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForRead(Text, Length, sizeof(CHAR));
            ProbeAndNullPointer(Message);
        }

        //
        // If the current thread does not have an associated receive buffer,
        // then attempt to allocate one now. If the allocation fails, then
        // an exception is raised.
        //

        if (ServerThread->Section == NULL) {
            KiAllocateReceiveBufferChannel();
        }

    } except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // If the message length is greater than the host page size minus
    // the message header length, then return an error.
    //

    if (Length >= (PAGE_SIZE - sizeof(CHANNEL_MESSAGE))) {
        return  STATUS_BUFFER_OVERFLOW;
    }

    //
    // If the server thread has an associated message channel, the channel
    // is in server receive message state, and the channel server thread
    // matches the current thread.
    //
    // This implies that:
    //
    // 1. The channel is a message channel.
    //
    // 2. The channel is being accessed by the server thread.
    //
    // 3. The channel is associated with a listen channel.
    //
    // 4. There is currently a server channel owner.
    //
    // 5. There is currently a client channel owner.
    //

    KiLockDispatcherDatabase(&ServerThread->WaitIrql);
    MessageChannel = ServerThread->Channel;
    if ((MessageChannel == NULL) ||
        (MessageChannel->State != ServerReceiveMessage) ||
        (MessageChannel->ServerThread != ServerThread)) {

        //
        // A message is not associated with the current thread,
        // the message channel is in the wrong state, or the
        // current thread is not the owner of the channel.
        //

        KiUnlockDispatcherDatabase(ServerThread->WaitIrql);
        Status = STATUS_INVALID_PARAMETER; // **** fix ****

    } else {

        //
        // Rendezvous with the client thread so a transfer of the
        // reply text to the client thread can occur.
        //

        ClientThread = KiRendezvousWithThread(MessageChannel, PreviousMode);

        //
        // Control is returned when:
        //
        // 1. The server thread is being terminated (USER_APC).
        //
        // 2. A rendezvous with a client thread has occured.
        //
        // N.B. If the status value is less than zero, then it
        //      is the address of the client thread.
        //

        if ((LONG)ClientThread < 0) {

            //
            // The client thread is returned as the rendezvous status
            // with the thread in the transition state. Get the address
            // of the client thread system view, establish an exception
            // handler, and transfer the  message text from the server's
            // buffer to the client's receive buffer. If an exception
            // occurs during the copy, then return the exception code
            // as the service status.
            //

            ClientView = ClientThread->SystemView;
            Status = STATUS_SUCCESS;
            if (Length != 0) {
                try {
                    RtlCopyMemory(ClientView + 1, Text, Length);

                } except (ExSystemExceptionFilter()) {
                    Status = GetExceptionCode();
                }
            }

            //
            // Set the channel message parameters.
            //

            ClientView->Text = (PVOID)(ClientThread->ThreadView + 1);
            ClientView->Length = Length;
            ClientView->Context = NULL;
            ClientView->Base = Text;
            ClientView->Close = FALSE;

            //
            // Raise IRQL to dispatch level, lock the dispatcher
            // database, and check if the message was successfully
            // transfered to the client's receive buffer. If the
            // message was successfully transfered to the client's
            // receive buffer. then reset the channel state, fill
            // in the message parameters, ready the client thread,
            // and listen for the next message. Otherwise, set the
            // client wait status and ready the client thread for
            // execution.
            //

            KiLockDispatcherDatabase(&ServerThread->WaitIrql);
            if (NT_SUCCESS(Status)) {
                MessageChannel->State = ClientIdle;
                MessageChannel->ClientThread = NULL;
                MessageChannel->ServerThread = NULL;
                ClientThread->WaitStatus = STATUS_SUCCESS;

                //
                // Reference the server channel and dereference the
                // message channel.
                //

                ServerChannel = MessageChannel->ServerChannel;
                ObReferenceObject(ServerChannel);
                ObDereferenceObject(MessageChannel);

                //
                // If there are no clients waiting to send to the server,
                // then switch directly to the client thread. Otherwise,
                // ready the client thread, then listen for the next
                // message.
                //

                if (IsListEmpty(&ServerChannel->ClearToSendEvent.Header.WaitListHead) == FALSE) {
                    KiReadyThread(ClientThread);
                    KiUnlockDispatcherDatabase(ServerThread->WaitIrql);
                    Status = KiListenChannel(ServerChannel,
                                             PreviousMode,
                                             Message);

                } else {
                    Status = KiSwitchToThread(ClientThread,
                                              WrRendezvous,
                                              PreviousMode,
                                              &ServerChannel->ReceiveEvent);

                    //
                    // If a client message was successfully received, then
                    // attempt to write the address of the send message
                    // address. If the write attempt fails, then do not
                    // report an error. When the caller attempts to access
                    // the message address, an access violation will occur.
                    //

                    if (NT_SUCCESS(Status)) {
                        try {
                            *Message = ServerThread->ThreadView;

                        } except(ExSystemExceptionFilter()) {
                        }
                    }
                }

                ObDereferenceObject(ServerChannel);

            } else {

                //
                // The reply message was not successfully transfered
                // to the client receive buffer because of an access
                // violation encountered durring the access to the
                // server buffer.
                //

                ClientThread->WaitStatus = STATUS_KERNEL_APC;
                KiReadyThread(ClientThread);
                KiUnlockDispatcherDatabase(ServerThread->WaitIrql);
            }

        } else {

            //
            // The server thread is terminating and the channel
            // structures will be cleaned up by the termiantion
            // code.
            //

            Status = (NTSTATUS)ClientThread;
        }
    }

    //
    // Return service status.
    //

    return Status;

#else

    return STATUS_NOT_IMPLEMENTED;

#endif

}

NTSTATUS
NtSendWaitReplyChannel (
    IN HANDLE ChannelHandle,
    IN PVOID Text,
    IN ULONG Length,
    OUT PCHANNEL_MESSAGE *Message
    )

/*++

Routine Description:

    This function sends a message to a server and waits for a reply.

    N.B. This function can only be executed from a client thread.

Arguments:

    ChannelHandle - Supplies a handle to a message channel over which the
        specified message text is sent.

    Text - Supplies a pointer to the message text.

    Length - Supplies the length of the message text.

    Message - Supplies a pointer to a variable that receives a pointer
        to the reply message header.

Return Value:

    If the function is successfully completed, then a success status is
    returned. Otherwise, a failure status is returned.

--*/

{

#if 0

    PKTHREAD ClientThread;
    PRECHANNEL MessageChannel;
    KPROCESSOR_MODE PreviousMode;
    PRECHANNEL ServerChannel;
    PKTHREAD ServerThread;
    PCHANNEL_MESSAGE ServerView;
    NTSTATUS Status;

    //
    // Establish an exception handler, probe the output message address,
    // probe the message text, and allocate a receive buffer if necessary.
    // If either of the probes fail or the receive buffer allocation is
    // not successful, then return the exception code as the service
    // status.
    //

    ClientThread = KeGetCurrentThread();
    try {

        //
        // Get previous processor mode and probe output message address
        // and the message text.
        //

        PreviousMode = KeGetPreviousMode();
        if (PreviousMode != KernelMode) {
            ProbeForRead(Text, Length, sizeof(UCHAR));
            ProbeAndNullPointer(Message);
        }

        //
        // If the current thread does not have an associated receive buffer,
        // then attempt to allocate one now. If the allocation fails, then
        // an exception is raised.
        //

        if (ClientThread->Section == NULL) {
            KiAllocateReceiveBufferChannel();
        }

    } except(ExSystemExceptionFilter()) {
        return GetExceptionCode();
    }

    //
    // If the message length is greater than the host page size minus
    // the message header length, then return an error.
    //

    if (Length >= (PAGE_SIZE - sizeof(CHANNEL_MESSAGE))) {
        return STATUS_BUFFER_OVERFLOW;
    }

    //
    // Reference channel object by handle.
    //

    Status = ObReferenceObjectByHandle(ChannelHandle,
                                       CHANNEL_ALL_ACCESS,
                                       KeChannelType,
                                       PreviousMode,
                                       (PVOID *)&MessageChannel,
                                       NULL);

    //
    // If the reference was successful, then check if the channel is in
    // the client idle state.
    //
    // This implies that:
    //
    // 1. The channel is a message channel.
    //
    // 2. The channel is being accessed by a client thread.
    //
    // 3. The channel is connected to a listen channel.
    //
    // 4. There is currently no client thread channel owner.
    //
    // 5. There is currently no server thread channel owner.
    //

    if (NT_SUCCESS(Status)) {
        KiLockDispatcherDatabase(&ClientThread->WaitIrql);
        if (MessageChannel->State != ClientIdle) {

            //
            // The message channel is in the wrong state.
            //

            KiUnlockDispatcherDatabase(ClientThread->WaitIrql);
            Status = STATUS_INVALID_PARAMETER; // **** fix ****

        } else {

            //
            // Set the channel state, set the client owner thread, and
            // rendezvous with a server thread.
            //

            MessageChannel->State = ClientSendWaitReply;
            MessageChannel->ClientThread = ClientThread;
            ClientThread->Channel = MessageChannel;
            ServerChannel = MessageChannel->ServerChannel;
            ServerThread = KiRendezvousWithThread(ServerChannel, PreviousMode);

            //
            // Control is returned when:
            //
            // 1. The client thread is being terminated (USER_APC).
            //
            // 2. A rendezvous with a server thread has occured.
            //
            // N.B. If the status value is less than zero, then it
            //      is the address of the server thread.
            //

            if ((LONG)ServerThread < 0) {

                //
                // The server thread is returned as the rendezvous status
                // with the thread in the transition state. Get the address
                // of the server thread system view, establish an exception
                // handler, and transfer the message text from the client's
                // buffer to the server's receive buffer. If an exception
                // occurs during the copy, then return the exception code
                // as the service status.
                //

                ServerView = ServerThread->SystemView;
                if (Length != 0) {
                    try {
                        RtlCopyMemory(ServerView + 1, Text, Length);

                    } except (ExSystemExceptionFilter()) {
                        Status = GetExceptionCode();
                    }
                }

                //
                // Set the channel message parameters.
                //

                ServerView->Text = (PVOID)(ServerThread->ThreadView + 1);
                ServerView->Length = Length;
                ServerView->Context = MessageChannel->ServerContext;
                ServerView->Base = Text;
                ServerView->Close = FALSE;

                //
                // Raise IRQL to dispatch level, lock the dispatcher
                // database and check if the message was successfully
                // transfered to the server's receive buffer. If the
                // message was successfully transfered, then set the
                // channel state, set the server thread address, set
                // the address of the message channel in the server
                // thread, increment the message channel reference
                // count, fill in the message parameters, and switch
                // directly to the server thread. Otherwise, set the
                // channel state, and reready the server thread for
                // execution.
                //

                KiLockDispatcherDatabase(&ClientThread->WaitIrql);
                if (NT_SUCCESS(Status)) {
                    MessageChannel->State = ServerReceiveMessage;
                    MessageChannel->ServerThread = ServerThread;
                    ObReferenceObject(MessageChannel);
                    ServerThread->Channel = MessageChannel;
                    Status = KiSwitchToThread(ServerThread,
                                              WrRendezvous,
                                              PreviousMode,
                                              &MessageChannel->ReceiveEvent);

                    //
                    // If the send and subsequent reply from the server
                    // thread is successful, then attempt to write the
                    // address of the reply message address. If the write
                    // attempt fails, then do not report an error. When
                    // the caller attempts to access the message address,
                    // an access violation will occur.
                    //

                    if (NT_SUCCESS(Status)) {
                        try {
                            *Message = ClientThread->ThreadView;

                        } except(ExSystemExceptionFilter()) {
                        }
                    }

                } else {

                    //
                    // The send message was not successfully transfered
                    // to the server receive buffer because of an access
                    // violation encountered durring the access to the
                    // client buffer.
                    //

                    MessageChannel->State = ClientIdle;
                    MessageChannel->ClientThread = NULL;
                    ClientThread->Channel = NULL;
                    ServerThread->WaitStatus = STATUS_KERNEL_APC;
                    KiReadyThread(ServerThread);
                    KiUnlockDispatcherDatabase(ClientThread->WaitIrql);
                }

            } else {

                //
                // The client thread is terminating and the channel
                // structures will be cleaned up by the termination
                // code.
                //

                Status = (NTSTATUS)ServerThread;
            }
        }

        ObDereferenceObject(MessageChannel);
    }

    //
    // Return service status.
    //

    return Status;

#else

    return STATUS_NOT_IMPLEMENTED;

#endif

}

NTSTATUS
NtSetContextChannel (
    IN PVOID Context
    )

/*++

Routine Description:

    This function stores a context value for the current associated
    message channel.

    N.B. This function can only be executed from a server thread that
        has an associated message channel.

Arguments:

    Context - Supplies a context value that is to be stored in the
        associated message channel.

Return Value:

    If the channel information is set, then a success status is returned.
    Otherwise, a failure status is returned.

--*/

{

#if 0

    PRECHANNEL MessageChannel;
    PKTHREAD CurrentThread;
    NTSTATUS Status;

    //
    // If the thread has an assoicated channel and the server thread for
    // the channel is the current thread, then store the channel context.
    //

    CurrentThread = KeGetCurrentThread();
    MessageChannel = CurrentThread->Channel;
    if ((MessageChannel == NULL) ||
        (CurrentThread != MessageChannel->ServerThread)) {
        Status = STATUS_INVALID_PARAMETER; // ****** FIX *****

    } else {
        MessageChannel->ServerContext = Context;
        Status = STATUS_SUCCESS;
    }

    //
    // Return service status.
    //

    return Status;

#else

    return STATUS_NOT_IMPLEMENTED;

#endif

}

#if 0


VOID
KiAllocateReceiveBufferChannel (
    VOID
    )

/*++

Routine Description:

    This function creates an unnamed section with a single page, maps
    a view of the section into the current process and into the system
    address space, and associates the view with the current thread.

Arguments:

    None.

Return Value:

    If a channel receive buffer is not allocated, then raise an exception.

--*/

{

    LARGE_INTEGER MaximumSize;
    PEPROCESS Process;
    NTSTATUS Status;
    PKTHREAD Thread;
    LARGE_INTEGER ViewOffset;
    ULONG ViewSize;

    //
    // Create an unnamed section object.
    //

    Thread = KeGetCurrentThread();

    ASSERT(Thread->Section == NULL);

    MaximumSize.QuadPart = PAGE_SIZE;
    Status = MmCreateSection(&Thread->Section,
                             0,
                             NULL,
                             &MaximumSize,
                             PAGE_READWRITE,
                             SEC_COMMIT,
                             NULL,
                             NULL);

    if (NT_SUCCESS(Status)) {

        //
        // Map a view of the section into the current process.
        //

        Process = PsGetCurrentProcess();
        ViewOffset.QuadPart = 0;
        ViewSize = PAGE_SIZE;
        Status = MmMapViewOfSection(Thread->Section,
                                    Process,
                                    &Thread->ThreadView,
                                    0,
                                    ViewSize,
                                    &ViewOffset,
                                    &ViewSize,
                                    ViewUnmap,
                                    0,
                                    PAGE_READWRITE);

        if (NT_SUCCESS(Status)) {

            //
            // Map a view of the section into the system address
            // space.
            //

            Status = MmMapViewInSystemSpace(Thread->Section,
                                            &Thread->SystemView,
                                            &ViewSize);

            if (NT_SUCCESS(Status)) {
                return;
            }

            MmUnmapViewOfSection(Process, Thread->ThreadView);
        }

        ObDereferenceObject(Thread->Section);
    }

    //
    // The allocation of a receive buffer was not successful. Raise an
    // exception that will be caught by the caller.
    //

    ExRaiseStatus(Status);
    return;
}

BOOLEAN
KiChannelInitialization (
    VOID
    )

/*++

Routine Description:

    This function creates the channel object type descriptor at system
    initialization and stores the address of the object type descriptor
    in global storage.

Arguments:

    None.

Return Value:

    A value of TRUE is returned if the channel object type descriptor is
    successfully initialized. Otherwise a value of FALSE is returned.

--*/

{

    OBJECT_TYPE_INITIALIZER ObjectTypeInitializer;
    NTSTATUS Status;
    UNICODE_STRING TypeName;

    //
    // Initialize string descriptor.
    //

    RtlInitUnicodeString(&TypeName, L"Channel");

    //
    // Create channel object type descriptor.
    //

    RtlZeroMemory(&ObjectTypeInitializer, sizeof(ObjectTypeInitializer));
    ObjectTypeInitializer.Length = sizeof(ObjectTypeInitializer);
    ObjectTypeInitializer.GenericMapping = KiChannelMapping;
    ObjectTypeInitializer.PoolType = NonPagedPool;
    ObjectTypeInitializer.DefaultNonPagedPoolCharge = sizeof(ECHANNEL);
    ObjectTypeInitializer.ValidAccessMask = CHANNEL_ALL_ACCESS;
    ObjectTypeInitializer.InvalidAttributes = OBJ_EXCLUSIVE | OBJ_INHERIT | OBJ_PERMANENT;
    ObjectTypeInitializer.CloseProcedure = KiCloseChannel;
    ObjectTypeInitializer.DeleteProcedure = KiDeleteChannel;
    Status = ObCreateObjectType(&TypeName,
                                &ObjectTypeInitializer,
                                NULL,
                                &KeChannelType);

    //
    // If the channel object type descriptor was successfully created, then
    // return a value of TRUE. Otherwise return a value of FALSE.
    //

    return (BOOLEAN)(NT_SUCCESS(Status));
}

VOID
KiCloseChannel (
    IN PEPROCESS Process,
    IN PVOID Object,
    IN ACCESS_MASK GrantedAccess,
    IN ULONG ProcessHandleCount,
    IN ULONG SystemHandleCount
    )

/*++

Routine Description:

    This function is called when a handle to a channel is closed. If the
    hanlde is the last handle in the process to the channel object and
    the channel object is a message channel, then send a message to the
    server indicating that the client handle is being closed.

Arguments:

    Object - Supplies a pointer to an executive channel.

Return Value:

    None.

--*/

{
    PECHANNEL MessageChannel = (PECHANNEL)Object;

    //
    // If the object is a message channel and hte last handle is being
    // closed, then send a message to the server indicating that the
    // channel is being closed.
    //

    if ((MessageChannel->ServerChannel != NULL) &&
        (ProcessHandleCount == 1)) {
    }

    return;
}

VOID
KiDeleteChannel (
    IN PVOID Object
    )

/*++

Routine Description:

    This function is the delete routine for channel objects. Its function
    is to ...

Arguments:

    Object - Supplies a pointer to an executive channel.

Return Value:

    None.

--*/

{

    PRECHANNEL ChannelObject = (PECHANNEL)Object;

    //
    // If the channel is a message channel, then dereference the connnected
    // server channel.
    //

    if (ChannelObject->ServerChannel != NULL) {
        ObDereferenceObject(ChannelObject->ServerChannel);
    }

    return;
}

VOID
KiRundownChannel (
    VOID
    )

/*++

Routine Description:

    This function runs down associated channel object and receive buffers
    when the a thread is terminated.

Arguments:

    None.

Return Value:

    None.

--*/

{

    PKTHREAD Thread;

    //
    // If the current thread has an associated receive buffer, then unmap
    // the receive buffer and dereference the underlying section.
    //

    Thread = KeGetCurrentThread();
    if (Thread->Section != NULL) {
        MmUnmapViewOfSection(PsGetCurrentProcess(), Thread->ThreadView);
        MmUnmapViewInSystemSpace(Thread->SystemView);
        ObDereferenceObject(Thread->Section);
        Thread->Section = NULL;
    }

    //
    // If the current thread has an associated channel, then ...
    //

    return;
}

NTSTATUS
KiListenChannel (
    IN PRECHANNEL ServerChannel,
    IN KPROCESSOR_MODE WaitMode,
    OUT PCHANNEL_MESSAGE *Message
    )

/*++

Routine Description:

    This function listens for a client message to arrive.

    N.B. This function can only be executed from a server thread.

Arguments:

    ServerChannel - Supplies a pointer to a litent channel on which the
        server thread listens.

    WaitMode - Supplies the processor wait mode.

    Message - Supplies a pointer to a variable that receives a pointer
        to the client message header.

Return Value:

    If the function is successfully completed, then a success status is
    returned. Otherwise, a failure status is returned.

--*/

{

    PKEVENT ClearToSendEvent;
    PKTHREAD ClientThread;
    PKQUEUE Queue;
    PKTHREAD ServerThread;
    PKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;
    NTSTATUS WaitStatus;

    //
    // Raise IRQL to dispatch level and lock the dispatcher database.
    //

    ServerThread = KeGetCurrentThread();
    KiLockDispatcherDatabase(&ServerThread->WaitIrql);

    //
    // Start of wait loop.
    //
    // Note this loop is repeated if a kernel APC is delivered in the
    // middle of the wait or a kernel APC is pending on the first attempt
    // through the loop.
    //

    do {

        //
        // Check if there is a thread waiting on the clear to send event.
        //

        ClearToSendEvent = &ServerChannel->ClearToSendEvent;
        WaitEntry = ClearToSendEvent->Header.WaitListHead.Flink;
        if (WaitEntry != &ClearToSendEvent->Header.WaitListHead) {
            WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
            ClientThread = WaitBlock->Thread;

            //
            // Remove the wait block from the wait list of the receive event,
            // and remove the client thread from the wait list.
            //

            RemoveEntryList(&WaitBlock->WaitListEntry);
            RemoveEntryList(&ClientThread->WaitListEntry);

            //
            // If the client thread is processing a queue entry, then increment the
            // count of currently active threads.
            //

            Queue = ClientThread->Queue;
            if (Queue != NULL) {
                Queue->CurrentCount += 1;
            }

            //
            // Set the wait completion status to kernel APC so the client
            // will attempt another rendezvous and ready the client thread
            // for execution.
            //

            ClientThread->WaitStatus = STATUS_KERNEL_APC;
            KiReadyThread(ClientThread);
        }

        //
        // Test to determine if a kernel APC is pending.
        //
        // If a kernel APC is pending and the previous IRQL was less than
        // APC_LEVEL, then a kernel APC was queued by another processor
        // just after IRQL was raised to DISPATCH_LEVEL, but before the
        // dispatcher database was locked.
        //
        // N.B. that this can only happen in a multiprocessor system.
        //

        if (ServerThread->ApcState.KernelApcPending &&
            (ServerThread->WaitIrql < APC_LEVEL)) {

            //
            // Unlock the dispatcher database and lower IRQL to its
            // previous value. An APC interrupt will immediately occur
            // which will result in the delivery of the kernel APC if
            // possible.
            //

            KiUnlockDispatcherDatabase(ServerThread->WaitIrql);

        } else {

            //
            // Test if a user APC is pending.
            //

            if ((WaitMode != KernelMode) &&
                (ServerThread->ApcState.UserApcPending)) {
                WaitStatus = STATUS_USER_APC;
                break;
            }

            //
            // Construct a wait block for the clear to send event object.
            //

            WaitBlock = &ServerThread->WaitBlock[0];
            ServerThread->WaitBlockList = WaitBlock;
            ServerThread->WaitStatus = (NTSTATUS)0;
            WaitBlock->Object = (PVOID)&ServerChannel->ReceiveEvent;
            WaitBlock->NextWaitBlock = WaitBlock;
            WaitBlock->WaitKey = (CSHORT)STATUS_SUCCESS;
            WaitBlock->WaitType = WaitAny;
            InsertTailList(&ServerChannel->ReceiveEvent.Header.WaitListHead,
                           &WaitBlock->WaitListEntry);

            //
            // If the current thread is processing a queue entry, then
            // attempt to activate another thread that is blocked on the
            // queue object.
            //

            Queue = ServerThread->Queue;
            if (Queue != NULL) {
                KiActivateWaiterQueue(Queue);
            }

            //
            // Set the thread wait parameters, set the thread dispatcher
            // state to Waiting, and insert the thread in the wait list.
            //

            ServerThread->Alertable = FALSE;
            ServerThread->WaitMode = WaitMode;
            ServerThread->WaitReason = WrRendezvous;
            ServerThread->WaitTime = KiQueryLowTickCount();
            ServerThread->State = Waiting;
            KiInsertWaitList(WaitMode, ServerThread);

            //
            // Switch context to selected thread.
            //
            // Control is returned at the original IRQL.
            //

            ASSERT(KeIsExecutingDpc() == FALSE);
            ASSERT(ServerThread->WaitIrql <= DISPATCH_LEVEL);

            WaitStatus = KiSwapThread();

            //
            // If the thread was not awakened to deliver a kernel mode APC,
            // then return wait status.
            //

            if (WaitStatus != STATUS_KERNEL_APC) {

                //
                // If a client message was successfully received, then
                // attempt to write the address of the send message
                // address. If the write attempt fails, then do not
                // report an error. When the caller attempts to access
                // the message address, an access violation will occur.
                //

                  if (NT_SUCCESS(WaitStatus)) {
                      try {
                          *Message = ServerThread->ThreadView;

                      } except(ExSystemExceptionFilter()) {
                      }
                  }

                return WaitStatus;
            }
        }

        //
        // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database.
        //

        KiLockDispatcherDatabase(&ServerThread->WaitIrql);
    } while (TRUE);

    //
    // Unlock the dispatcher database and return the target thread.
    //

    KiUnlockDispatcherDatabase(ServerThread->WaitIrql);
    return WaitStatus;
}

PKTHREAD
KiRendezvousWithThread (
    IN PRECHANNEL WaitChannel,
    IN ULONG WaitMode
    )

/*++

Routine Description:

    This function performs a rendezvous with a thread waiting on the
    channel receive event.

    N.B. This routine is called with the dispatcher database locked.

    N.B. The wait IRQL is assumed to be set for the current thread.

    N.B. Control is returned from this function with the dispatcher
        database unlocked.

Arguments:

    WaitChannel - Supplies a pointer to a channel whose receive event
        is the target of the rendezvous operation.

    WaitMode - Supplies the processor wait mode.

Return Value:

    If a thread rendezvous is successfully performed, then the address
    of the thread object is returned as the completion status. Otherwise,
    if the wait completes because of a timeout or because the thread is
    being terminated, then the appropriate status is returned.

--*/

{

    PKTHREAD CurrentThread;
    PKQUEUE Queue;
    PKTHREAD TargetThread;
    PKWAIT_BLOCK WaitBlock;
    PLIST_ENTRY WaitEntry;
    NTSTATUS WaitStatus;

    //
    // Start of wait loop.
    //
    // Note this loop is repeated if a kernel APC is delivered in the
    // middle of the wait or a kernel APC is pending on the first attempt
    // through the loop.
    //
    // If the rendezvous event wait list is not empty, then remove the first
    // entry from the list, compute the address of the respective thread,
    // cancel the thread timer if appropraite, and return the thread address.
    // Otherwise, wait for a thread to rendezvous with.
    //

    CurrentThread = KeGetCurrentThread();
    do {

        //
        // Check if there is a thread waiting on the rendezvous event.
        //

        WaitEntry = WaitChannel->ReceiveEvent.Header.WaitListHead.Flink;
        if (WaitEntry != &WaitChannel->ReceiveEvent.Header.WaitListHead) {
            WaitBlock = CONTAINING_RECORD(WaitEntry, KWAIT_BLOCK, WaitListEntry);
            TargetThread = WaitBlock->Thread;

            //
            // Remove the wait block from the wait list of the receive event,
            // and remove the target thread from the wait list.
            //

            RemoveEntryList(&WaitBlock->WaitListEntry);
            RemoveEntryList(&TargetThread->WaitListEntry);

            //
            // If the target thread is processing a queue entry, then increment the
            // count of currently active threads.
            //

            Queue = TargetThread->Queue;
            if (Queue != NULL) {
                Queue->CurrentCount += 1;
            }

            //
            // Set the thread state to transistion.
            //

            TargetThread->State = Transition;
            break;

        } else {

            //
            // Test to determine if a kernel APC is pending.
            //
            // If a kernel APC is pending and the previous IRQL was less than
            // APC_LEVEL, then a kernel APC was queued by another processor
            // just after IRQL was raised to DISPATCH_LEVEL, but before the
            // dispatcher database was locked.
            //
            // N.B. that this can only happen in a multiprocessor system.
            //

            if (CurrentThread->ApcState.KernelApcPending &&
                (CurrentThread->WaitIrql < APC_LEVEL)) {

                //
                // Unlock the dispatcher database and lower IRQL to its
                // previous value. An APC interrupt will immediately occur
                // which will result in the delivery of the kernel APC if
                // possible.
                //

                KiUnlockDispatcherDatabase(CurrentThread->WaitIrql);

            } else {

                //
                // Test if a user APC is pending.
                //

                if ((WaitMode != KernelMode) &&
                    (CurrentThread->ApcState.UserApcPending)) {
                    TargetThread = (PKTHREAD)STATUS_USER_APC;
                    break;
                }

                //
                // Construct a wait block for the clear to send event object.
                //

                WaitBlock = &CurrentThread->WaitBlock[0];
                CurrentThread->WaitBlockList = WaitBlock;
                CurrentThread->WaitStatus = (NTSTATUS)0;
                WaitBlock->Object = (PVOID)&WaitChannel->ClearToSendEvent;
                WaitBlock->NextWaitBlock = WaitBlock;
                WaitBlock->WaitKey = (CSHORT)STATUS_SUCCESS;
                WaitBlock->WaitType = WaitAny;
                InsertTailList(&WaitChannel->ClearToSendEvent.Header.WaitListHead,
                               &WaitBlock->WaitListEntry);

                //
                // If the current thread is processing a queue entry, then
                // attempt to activate another thread that is blocked on the
                // queue object.
                //

                Queue = CurrentThread->Queue;
                if (Queue != NULL) {
                    KiActivateWaiterQueue(Queue);
                }

                //
                // Set the thread wait parameters, set the thread dispatcher
                // state to Waiting, and insert the thread in the wait list.
                //

                CurrentThread->Alertable = FALSE;
                CurrentThread->WaitMode = (KPROCESSOR_MODE)WaitMode;
                CurrentThread->WaitReason = WrRendezvous;
                CurrentThread->WaitTime = KiQueryLowTickCount();
                CurrentThread->State = Waiting;
                KiInsertWaitList(WaitMode, CurrentThread);

                //
                // Switch context to selected thread.
                //
                // Control is returned at the original IRQL.
                //

                ASSERT(KeIsExecutingDpc() == FALSE);
                ASSERT(CurrentThread->WaitIrql <= DISPATCH_LEVEL);

                WaitStatus = KiSwapThread();

                //
                // If the thread was not awakened to deliver a kernel mode APC,
                // then return wait status.
                //

                if (WaitStatus != STATUS_KERNEL_APC) {
                    return (PKTHREAD)WaitStatus;
                }
            }

            //
            // Raise IRQL to DISPATCH_LEVEL and lock the dispatcher database.
            //

            KiLockDispatcherDatabase(&CurrentThread->WaitIrql);
        }

    } while (TRUE);

    //
    // Unlock the dispatcher database and return the target thread.
    //

    KiUnlockDispatcherDatabase(CurrentThread->WaitIrql);
    return TargetThread;
}

#endif
