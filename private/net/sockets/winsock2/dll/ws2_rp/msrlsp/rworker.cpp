//---------------------------------------------------------------------------
//  Copyright (c) 1997 Microsoft Corporation
//
//  rworker.cpp
//
//  Implements RWORKERTHREAD objects.
//
//  Revision History:
//
//  edwardr    11-05-97    Initial version.
//
//---------------------------------------------------------------------------
/*++

     Copyright (c) 1996 Intel Corporation
     Copyright (c) 1996 Microsoft Corporation
     All Rights Reserved

     Permission is granted to use, copy and distribute this software and
     its documentation for any purpose and without fee, provided, that
     the above copyright notice and this statement appear in all copies.
     Intel makes no representations about the suitability of this
     software for any purpose.  This software is provided "AS IS."

     Intel specifically disclaims all warranties, express or implied,
     and all liability, including consequential and other indirect
     damages, for the use of this software, including liability for
     infringement of any proprietary rights, and including the
     warranties of merchantability and fitness for a particular purpose.
     Intel does not assume any responsibility for any errors which may
     appear in this software nor any responsibility to update it.


--*/

#include "precomp.h"
#include "globals.h"

VOID
CALLBACK
APCProc(
    ULONG_PTR Context
    )
/*++
Routine Description:

    This routine unpacks the context value passed to WPUQueueApc() and calls
    the users completion function. This function is called in the clients
    thread context.

Arguments:

    Context - The context value passed to WPUQueueApc().

Return Value:

    None

--*/
{
    PAPCCONTEXT ApcContext;

    ApcContext = (PAPCCONTEXT) Context;

    (ApcContext->CompletetionProc)(
        ApcContext->Error,
        ApcContext->BytesTransferred,
        ApcContext->lpOverlapped,
        0);

    gWorkerThread->FreeApcContext(
        ApcContext);


}

VOID
CALLBACK
OverlappedCompletionProc(
    DWORD  dwError,
    DWORD  cbTransferred,
    LPWSAOVERLAPPED lpOverlapped,
    DWORD dwFlags
    )
/*++
Routine Description:

    This routine is the completion routine for all overlapped operations
    initiated with the lower level provider.

Arguments:

    dwError - The error code for the overlapped operation.

    cbTransferred - The number of bytes transfered by the overlapped
                    operation.

    lpOverlapped - a pointer to the WSAOVERLAPPED struct associated with the
                   overlapped operation.

    dwFlags -  Not Used must be zero.

Return Value:

    None

--*/
{
    INT                                ReturnCode;
    DWORD                              BytesCopied;
    LPWSAOVERLAPPED                    UserOverlappedStruct;
    LPWSAOVERLAPPED_COMPLETION_ROUTINE UserCompletionRoutine;
    LPWSATHREADID                      UserThreadId;
    LPWSABUF                           UserBuffer;
    DWORD                              UserBufferCount;
    LPWSABUF                           InternalBuffer;
    DWORD                              InternalBufferCount;
    PAPCCONTEXT                        ApcContext;
    INT                                Errno;
    DWORD                              OperationType;


    // Get the stored overlapped operation parameters
    ReturnCode = gWorkerThread->RetrieveOverlappedParams(
        lpOverlapped,
        &OperationType,
        &UserOverlappedStruct,
        &UserCompletionRoutine,
        &UserThreadId,
        &UserBuffer,
        &UserBufferCount,
        &InternalBuffer,
        &InternalBufferCount);

    // If the completed operation was a recieve operation copy the internal
    // buffers into the users buffers.
    if (NO_ERROR == ReturnCode){
        if ((OperationType == WSP_RECV) ||
            (OperationType == WSP_RECVFROM)){
            gBufferManager->CopyBuffer(
                InternalBuffer,
                InternalBufferCount,
                UserBuffer,
                UserBufferCount,
                &BytesCopied);
        } //if

        gBufferManager->FreeBuffer(
            InternalBuffer,
            InternalBufferCount);

        UserOverlappedStruct->InternalHigh = BytesCopied;
        UserOverlappedStruct->Internal = dwError;

        // If the user requested completion routine notification of the I/O
        // completion queue an APC to the user thread else signal the users
        // event.
        if (UserCompletionRoutine){
            ApcContext = gWorkerThread->AllocateApcContext();

            ApcContext->Error = dwError;
            ApcContext->BytesTransferred = BytesCopied;
            ApcContext->lpOverlapped = UserOverlappedStruct;
            ApcContext->CompletetionProc = UserCompletionRoutine;

            g_UpCallTable.lpWPUQueueApc(
                UserThreadId,
                APCProc,
                (ULONG_PTR)ApcContext,
                &Errno);

        } //if
        else{
            SetEvent(UserOverlappedStruct->hEvent);
        } //else
        gWorkerThread->FreeOverlappedStruct(
            lpOverlapped);
    } //if
}

DWORD
OuterThreadProc(
    DWORD  Context
    )
/*++
Routine Description:

    Thread procedure passed to CreatThread().

Arguments:

    Context - Context value passed to CreateThread().  The context value is the
              value of gWorkerThread.

Return Value:

    The Return value of the worker thread

--*/

{
    PRWORKERTHREAD Thread;

    Thread = (PRWORKERTHREAD) Context;
    return(Thread->ThreadProc());
}



RWORKERTHREAD::RWORKERTHREAD()
/*++
Routine Description:

    Creates any internal state.

Arguments:

    None

Return Value:

    None

--*/

{
    m_thread_handle = NULL;
    m_wakeup_event = NULL;
    m_event_count   = 0;
    m_exit_thread   = FALSE;
    memset(&m_wait_array, 0, sizeof(m_wait_array));
    memset(&m_socket_array, 0, sizeof(m_socket_array));

    InitializeCriticalSection(
        &m_array_lock);

    InitializeCriticalSection(
        &m_overlapped_operation_queue_lock);

    InitializeListHead(
        &m_overlapped_operation_queue);
}



RWORKERTHREAD::~RWORKERTHREAD()
/*++
Routine Description:

    destroys any internal state.

Arguments:

    None

Return Value:

    None

--*/
{

    // If we made it through Initialize. Wake up our thread tell it to exit.
    if (m_wakeup_event){
        m_exit_thread = TRUE;
        SetEvent(m_wakeup_event);
        CloseHandle(m_wakeup_event);
    } //if

    if (m_thread_handle){
        CloseHandle(m_thread_handle);
    } //if

    delete(m_overlapped_struct_manager);
    m_overlapped_struct_manager = NULL;

    DeleteCriticalSection(
        &m_array_lock);

    DeleteCriticalSection(
        &m_overlapped_operation_queue_lock);

    DEBUGF( DBG_TRACE,
            ("Destroyed worker thread object\n"));
}



INT
RWORKERTHREAD::Initialize(

    )
/*++
Routine Description:

    Initializes the RWORKERTHREAD object.

Arguments:

    NONE

Return Value:

    If no error occurs, Initialize() returns NO_ERROR.  Otherwise the value
    SOCKET_ERROR  is  returned,  and  a  specific  error  code  is available in
    lpErrno.

--*/
{
    INT ReturnCode =WSAENETDOWN;
    DWORD ThreadId;

    DEBUGF( DBG_TRACE,
            ("Initializing worker thread \n"));

    // Create the event we will use to communicat with the worker thread.
    m_wakeup_event = CreateEvent(
        NULL,
        FALSE,
        FALSE,
        NULL);
    if ( m_wakeup_event){
        // Add the wakeup event to the event array that the worker thread will
        // be waiting on
        m_wait_array[0] = m_wakeup_event;
        m_event_count++;
        ReturnCode = NO_ERROR;
    } //if

    //
    // Create the worker thread and wait for the new thred to finish its
    // initialization.
    //
    if (NO_ERROR == ReturnCode){
        ReturnCode =WSAENETDOWN;
        m_thread_handle = CreateThread(
            NULL,
            0,
            (LPTHREAD_START_ROUTINE)OuterThreadProc,
            this,
            0,
            &ThreadId);
        if (m_thread_handle){
            ReturnCode = NO_ERROR;
        } //if
    } //if

    //
    // Init the overlapped structure manager
    //
    if (NO_ERROR == ReturnCode){
        ReturnCode =WSAENETDOWN;
        m_overlapped_struct_manager = new DOVERLAPPEDSTRUCTMGR;
        if ( m_overlapped_struct_manager){
            ReturnCode = m_overlapped_struct_manager->Initialize();
        } //if
    } //if

    if (NO_ERROR != ReturnCode){
        //Cleanup any resources we may have allocated.
        if (m_thread_handle){
            m_exit_thread = TRUE;
            SetEvent(m_wakeup_event);
            CloseHandle(m_thread_handle);
            m_thread_handle = NULL;
        } //if
        if (m_wakeup_event){
            CloseHandle(m_wakeup_event);
            m_wakeup_event = NULL;
        } //if
    } //if
    return(ReturnCode);
} //Initailize



INT
RWORKERTHREAD::RegisterSocket(
    PRSOCKET Socket
    )
/*++
Routine Description:

    Routine adds a Socket to the array of sockets that the worker thread is
    watching.

Arguments:

    Socket - A pointer to a RSOCKET object for the socket that is registering.

Return Value:

    On success NO_ERROR else a valid winsock error code.

--*/
{
    INT        ReturnCode;
    HANDLE     SocketEvent;
    LONG       SocketEventMask;
    SOCKET     SocketHandle;
    PRPROVIDER Provider;

    ReturnCode = WSAEFAULT;

    // Get the info we need from the socket
    SocketHandle    = Socket->GetSocketHandle();
    SocketEvent     = Socket->GetAsyncEventHandle();
    SocketEventMask = Socket->GetAsyncEventMask();
    Provider        = Socket->GetDProvider();

    //Add the socket to the wait array
    ReturnCode = AddSocket(
        Socket,
        SocketEvent);

    if (NO_ERROR == ReturnCode){

        // The socket was successfully added to the array so tell the thread
        // that the wait array has changed.
        SetEvent(m_wakeup_event);

        ReturnCode = NO_ERROR;
        // Issue WSPEvent on the socket
        Provider->WSPEventSelect(
            SocketHandle,
            SocketEvent,
            SocketEventMask,
            &ReturnCode);

        if (NO_ERROR != ReturnCode){
            RemoveSocket(
                Socket);
            SetEvent(m_wakeup_event);
        } //if
    } //if
    return(ReturnCode);
}

INT
RWORKERTHREAD::UnregisterSocket(
    PRSOCKET Socket
    )
/*++
Routine Description:

   Remove a socket for the set of sockets the worker thread is interested in.

Arguments:

    Socket - A pointer to a RSOCKET object for the socket that is
             unregistering.

Return Value:

     On success NO_ERROR else a valid winsock error code.

--*/
{
    // Remove the socket from the array of registered sockets.
    RemoveSocket(
        Socket);
    // Tell the thread that the array has changed
    SetEvent(m_wakeup_event);
    return(NO_ERROR);
}

DWORD
RWORKERTHREAD::ThreadProc()
/*++
Routine Description:

    The thread procedure for this object.

Arguments:

    NONE

Return Value:

    NO_ERROR

--*/
{
    DWORD ReturnCode;
    DWORD Index;
    PRSOCKET Socket;
    PINTERNALOVERLAPPEDSTRUCT OverlappedStruct;
    DWORD                     BytesTransfered;
    INT                       Errno;

    g_UpCallTable.lpWPUOpenCurrentThread(
        &m_thread_id,
        &Errno);

    while (!m_exit_thread){
        ReturnCode = WaitForMultipleObjectsEx(
            m_event_count,
            &m_wait_array[0],
            FALSE,
            INFINITE,
            TRUE);

        // Was one of the socket events signaled. We ignore WAIT_OBJECT_0 since
        // it is the internal event the outer object uses to communicate with
        // the thread.
        if ((ReturnCode >= WAIT_OBJECT_0 + 1) &&
            (ReturnCode <= (WAIT_OBJECT_0 + m_event_count -1))){

            Index = (ReturnCode - WAIT_OBJECT_0);

            EnterCriticalSection(&m_array_lock);

            Socket = (PRSOCKET)m_socket_array[Index];
            if (Socket){
                Socket->SignalAsyncEvent();
            } //if
            LeaveCriticalSection(&m_array_lock);
        } //if


        if ((ReturnCode == WAIT_OBJECT_0) && !m_exit_thread){

            // Is there a queued overlapped operation that needs to be
            // initiated.
            OverlappedStruct = NextOverlappedOperation();

            if (OverlappedStruct){
                switch (OverlappedStruct->OperationType)
                    {
                    case WSP_RECV:
                        OverlappedStruct->Provider->WSPRecv(
                            OverlappedStruct->Socket,
                            OverlappedStruct->InternalBuffer,
                            OverlappedStruct->InternalBufferCount,
                            &BytesTransfered,
                            OverlappedStruct->FlagsPtr,
                            &OverlappedStruct->InternalOverlappedStruct,
                            OverlappedCompletionProc,
                            &m_thread_id,
                            &Errno);
                        break;

                    case WSP_RECVFROM:
                        OverlappedStruct->Provider->WSPRecvFrom(
                            OverlappedStruct->Socket,
                            OverlappedStruct->InternalBuffer,
                            OverlappedStruct->InternalBufferCount,
                            &BytesTransfered,
                            OverlappedStruct->FlagsPtr,
                            OverlappedStruct->SockAddr,
                            OverlappedStruct->SockAddrLenPtr,
                            &OverlappedStruct->InternalOverlappedStruct,
                            OverlappedCompletionProc,
                            &m_thread_id,
                            &Errno);
                        break;

                    case WSP_SEND:
                         OverlappedStruct->Provider->WSPSend(
                            OverlappedStruct->Socket,
                            OverlappedStruct->InternalBuffer,
                            OverlappedStruct->InternalBufferCount,
                            &BytesTransfered,
                            OverlappedStruct->Flags,
                            &OverlappedStruct->InternalOverlappedStruct,
                            OverlappedCompletionProc,
                            &m_thread_id,
                            &Errno);
                        break;
                    case WSP_SENDTO:
                        OverlappedStruct->Provider->WSPSendTo(
                            OverlappedStruct->Socket,
                            OverlappedStruct->InternalBuffer,
                            OverlappedStruct->InternalBufferCount,
                            &BytesTransfered,
                            OverlappedStruct->Flags,
                            OverlappedStruct->SockAddr,
                            OverlappedStruct->SockAddrLen,
                            &OverlappedStruct->InternalOverlappedStruct,
                            OverlappedCompletionProc,
                            &m_thread_id,
                            &Errno);
                        break;

                    default:
                        ;
                    } //switch
            } //if
        } //if
    } //while
    g_UpCallTable.lpWPUCloseThread(
        &m_thread_id,
        &Errno);

    ExitThread(NO_ERROR);
    return(NO_ERROR);
}

INT
RWORKERTHREAD::QueueOverlappedRecv(
    PRPROVIDER                         Provider,
    SOCKET                             ProviderSocket,
    LPWSABUF                           UserBuffers,
    DWORD                              UserBufferCount,
    LPDWORD                            UserBytesRecvd,
    LPDWORD                            UserFlags,
    LPWSAOVERLAPPED                    UserOverlappedStruct,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE UserlpCompletionRoutine,
    LPWSATHREADID                      UserThreadId,
    LPWSABUF                           InternalBuffers,
    DWORD                              InternalBufferCount,
    LPINT                              Errno
    )
/*++

Routine Description:

    this routine allocates an internal overlapped structure stores its
    arguments in the allocated structure and enqueues the structure for the
    worker thread to complet the I/O operation.

Arguments:

    Provider - A pointer to the provider object that sould be used to satisfy
               this request.

    ProviderSocket - The providers socket descriptor.

    UserBuffers - The pointer to the user buffer(s).

    UserBufferCount - The number of user buffers.

    UserBytesRecvd - The pointer to the user BytesRecvd parameter.

    UserFlags - A pointer to the user flags argument.

    UserOverlappedStruct - The user overlapped struct pointer.

    UserlpCompletionRoutine - The user overlapped completion routine.

    UserThreadId - The user thread ID.

    InternalBuffers - A pointer to our internal buffer(s).

    InternalBufferCount - The number of internal buffers.

    Errno - A pointer to the user errno parameter.

Return Value:

    NO_ERROR on success else a valid winsock2 error code.

--*/
{
    INT ReturnCode;

    PINTERNALOVERLAPPEDSTRUCT InternalOverlappedStruct;

    ReturnCode = SOCKET_ERROR;
    *Errno = WSAENOBUFS;

    InternalOverlappedStruct =
        m_overlapped_struct_manager->AllocateOverlappedStruct();
    if (InternalOverlappedStruct){
        StoreOverlappedParams(
            InternalOverlappedStruct,
            WSP_RECV,
            Provider,
            ProviderSocket,
            UserOverlappedStruct,
            UserlpCompletionRoutine,
            UserThreadId,
            UserBuffers,
            UserBufferCount,
            InternalBuffers,
            InternalBufferCount,
            NULL,
            0,
            NULL,
            0,
            UserFlags
            );
        AddOverlappedOperation(
            InternalOverlappedStruct);
        *Errno = WSA_IO_PENDING;

    } //if
    return(ReturnCode);
}


INT
RWORKERTHREAD::QueueOverlappedRecvFrom(
    PRPROVIDER                         Provider,
    SOCKET                             ProviderSocket,
    LPWSABUF                           UserBuffers,
    DWORD                              UserBufferCount,
    LPDWORD                            UserBytesRecvd,
    LPDWORD                            UserFlags,
    struct sockaddr FAR *              UserFrom,
    LPINT                              UserFromLen,
    LPWSAOVERLAPPED                    UserOverlappedStruct,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE UserlpCompletionRoutine,
    LPWSATHREADID                      UserThreadId,
    LPWSABUF                           InternalBuffers,
    DWORD                              InternalBufferCount,
    LPINT                              Errno
    )
/*++

Routine Description:

    this routine allocates an internal overlapped structure stores its
    arguments in the allocated structure and enqueues the structure for the
    worker thread to complet the I/O operation.

Arguments:

    Provider - A pointer to the provider object that sould be used to satisfy
               this request.

    ProviderSocket - The providers socket descriptor.

    UserBuffers - The pointer to the user buffer(s).

    UserBufferCount - The number of user buffers.

    UserBytesRecvd - The pointer to the user BytesRecvd parameter.

    UserFlags - A pointer to the user flags argument.

    UserFrom  - A pinter to the user sockaddr structure,

    UserFromLen - A pointer to the length of UserFrom.

    UserOverlappedStruct - The user overlapped struct pointer.

    UserlpCompletionRoutine - The user overlapped completion routine.

    UserThreadId - The user thread ID.

    InternalBuffers - A pointer to our internal buffer(s).

    InternalBufferCount - The number of internal buffers.

    Errno - A pointer to the user errno parameter.

Return Value:

    NO_ERROR on success else a valid winsock2 error code.

--*/
{
    INT                       ReturnCode;
    PINTERNALOVERLAPPEDSTRUCT InternalOverlappedStruct;

    ReturnCode = SOCKET_ERROR;
    *Errno = WSAENOBUFS;

    InternalOverlappedStruct =
        m_overlapped_struct_manager->AllocateOverlappedStruct();
    if (InternalOverlappedStruct){
        StoreOverlappedParams(
            InternalOverlappedStruct,
            WSP_RECVFROM,
            Provider,
            ProviderSocket,
            UserOverlappedStruct,
            UserlpCompletionRoutine,
            UserThreadId,
            UserBuffers,
            UserBufferCount,
            InternalBuffers,
            InternalBufferCount,
            UserFrom,
            0,
            UserFromLen,
            0,
            UserFlags
            );

        AddOverlappedOperation(
            InternalOverlappedStruct);
        *Errno = WSA_IO_PENDING;

    } //if
    return(ReturnCode);
}

INT
RWORKERTHREAD::QueueOverlappedSend(
    PRPROVIDER Provider,
    SOCKET                             ProviderSocket,
    LPWSABUF                           UserBuffers,
    DWORD                              UserBufferCount,
    LPDWORD                            UserBytesSent,
    DWORD                              UserFlags,
    LPWSAOVERLAPPED                    UserOverlappedStruct,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE UserlpCompletionRoutine,
    LPWSATHREADID                      UserThreadId,
    LPINT           Errno
    )
/*++

Routine Description:

    this routine allocates an internal overlapped structure stores its
    arguments in the allocated structure and enqueues the structure for the
    worker thread to complet the I/O operation.

Arguments:

    Provider - A pointer to the provider object that sould be used to satisfy
               this request.

    ProviderSocket - The providers socket descriptor.

    UserBuffers - The pointer to the user buffer(s).

    UserBufferCount - The number of user buffers.

    UserBytesSent - The pointer to the user BytesSent parameter.

    UserFlags - The user flags .

    UserOverlappedStruct - The user overlapped struct pointer.

    UserlpCompletionRoutine - The user overlapped completion routine.

    UserThreadId - The user thread ID.

    InternalBuffers - A pointer to our internal buffer(s).

    InternalBufferCount - The number of internal buffers.

    Errno - A pointer to the user errno parameter.

Return Value:

    NO_ERROR on success else a valid winsock2 error code.

--*/
{
    INT                       ReturnCode;
    PINTERNALOVERLAPPEDSTRUCT InternalOverlappedStruct;

    ReturnCode = SOCKET_ERROR;
    *Errno = WSAENOBUFS;

    InternalOverlappedStruct =
        m_overlapped_struct_manager->AllocateOverlappedStruct();
    if (InternalOverlappedStruct){
        StoreOverlappedParams(
            InternalOverlappedStruct,
            WSP_SEND,
            Provider,
            ProviderSocket,
            UserOverlappedStruct,
            UserlpCompletionRoutine,
            UserThreadId,
            UserBuffers,
            UserBufferCount,
            UserBuffers,
            UserBufferCount,
            NULL,
            0,
            NULL,
            UserFlags,
            NULL
            );

        AddOverlappedOperation(
            InternalOverlappedStruct);
        *Errno = WSA_IO_PENDING;

    } //if
    return(ReturnCode);
}


INT
RWORKERTHREAD::QueueOverlappedSendTo(
    PRPROVIDER                         Provider,
    SOCKET                             ProviderSocket,
    LPWSABUF                           UserBuffers,
    DWORD                              UserBufferCount,
    LPDWORD                            UserBytesSent,
    DWORD                              UserFlags,
    const struct sockaddr FAR *        UserTo,
    INT                                UserToLen,
    LPWSAOVERLAPPED                    UserOverlappedStruct,
    LPWSAOVERLAPPED_COMPLETION_ROUTINE UserlpCompletionRoutine,
    LPWSATHREADID                      UserThreadId,
    LPINT                              Errno
    )
/*++

Routine Description:

    this routine allocates an internal overlapped structure stores its
    arguments in the allocated structure and enqueues the structure for the
    worker thread to complet the I/O operation.

Arguments:

    Provider - A pointer to the provider object that sould be used to satisfy
               this request.

    ProviderSocket - The providers socket descriptor.

    UserBuffers - The pointer to the user buffer(s).

    UserBufferCount - The number of user buffers.

    UserBytesRecvd - The pointer to the user BytesRecvd parameter.

    UserFlags - A pointer to the user flags argument.

    UserTo - A pointer to the user sockaddr structure.

    UserToLen - The length of the user sockaddr structure.

    UserOverlappedStruct - The user overlapped struct pointer.

    UserlpCompletionRoutine - The user overlapped completion routine.

    UserThreadId - The user thread ID.

    InternalBuffers - A pointer to our internal buffer(s).

    InternalBufferCount - The number of internal buffers.

    Errno - A pointer to the user errno parameter.

Return Value:

    NO_ERROR on success else a valid winsock2 error code.

--*/
{
    INT                       ReturnCode;
    PINTERNALOVERLAPPEDSTRUCT InternalOverlappedStruct;

    ReturnCode = SOCKET_ERROR;
    *Errno = WSAENOBUFS;

    InternalOverlappedStruct =
        m_overlapped_struct_manager->AllocateOverlappedStruct();
    if (InternalOverlappedStruct){
        StoreOverlappedParams(
            InternalOverlappedStruct,
            WSP_SENDTO,
            Provider,
            ProviderSocket,
            UserOverlappedStruct,
            UserlpCompletionRoutine,
            UserThreadId,
            UserBuffers,
            UserBufferCount,
            UserBuffers,
            UserBufferCount,
            (struct sockaddr*)((DWORD_PTR)UserTo),
            UserToLen,
            NULL,
            UserFlags,
            NULL
            );

        AddOverlappedOperation(
            InternalOverlappedStruct);

        *Errno = WSA_IO_PENDING;
    } //if
    return(ReturnCode);
}



INT
RWORKERTHREAD::AddSocket(
    PRSOCKET Socket,
    HANDLE   Event
    )
/*++
Routine Description:

    Adds a RSOCKET object and its associated event the set of events the worker
    thread is interested in.

Arguments:

    Socket - A pointer to a RSOCKET object.

    Event - The handle to the event associated with the socket object.

Return Value:

    NO_ERROR on success else a valid winsock error code.

--*/
{
    INT ReturnCode;
    UINT  Index;
    BOOL Found = FALSE;

    ReturnCode = WSAENOBUFS;

    //See if the socket already exists in the array
    for (Index =0;
         (Index < MAXIMUM_WAIT_OBJECTS) && (Index < m_event_count) ;
         Index++ ){
        if (m_socket_array[Index] == (DWORD_PTR) Socket){
            Found = TRUE;

            EnterCriticalSection(&m_array_lock);
            m_wait_array[Index] = Event;

            LeaveCriticalSection(&m_array_lock);

            ReturnCode = NO_ERROR;
            break;
        } //if
    } //for

    // If this is a new socket and we have room for the event in the wait array
    // add the socket and the event to the array
    if (!Found && (m_event_count < MAXIMUM_WAIT_OBJECTS)){

        EnterCriticalSection(&m_array_lock);

        m_socket_array[m_event_count] = (DWORD_PTR)Socket;
        m_wait_array[m_event_count]   = Event;
        m_event_count++;

        LeaveCriticalSection(&m_array_lock);

        ReturnCode = NO_ERROR;
    } //if
    return(ReturnCode);
}


INT
RWORKERTHREAD::RemoveSocket(
    PRSOCKET Socket
    )
/*++
Routine Description:

    Removes a socket from the set registered with the worker thread.

Arguments:

    Socket - A pointer to the socket object to remove.

Return Value:

    NO_ERROR on success else a valid winsock error code.

--*/
{
    INT ReturnCode;
    UINT  Index;
    BOOL Found = FALSE;

    ReturnCode = WSAEINVAL;

    //See if the socket already exists in the array
    for (Index =0;
         (Index < MAXIMUM_WAIT_OBJECTS) && (Index < m_event_count) ;
         Index++ ){
        if (m_socket_array[Index] == (DWORD_PTR) Socket){
            EnterCriticalSection(&m_array_lock);

            // Zero out the slot for this socket.
            m_wait_array[Index]   = NULL;
            m_socket_array[Index] = NULL;

            // Repack the array
            m_wait_array[Index]   = m_wait_array[(m_event_count-1)];
            m_socket_array[Index] = m_socket_array[(m_event_count-1)];;

            //Update the count.
            m_event_count--;

            LeaveCriticalSection(&m_array_lock);

            ReturnCode = NO_ERROR;
            break;
        } //if
    } //for
    return(ReturnCode);
}

INT
RWORKERTHREAD::StoreOverlappedParams(
    IN PINTERNALOVERLAPPEDSTRUCT          pOverlappedStruct,
    IN DWORD                              OperationType,
    IN PRPROVIDER                         Provider,
    IN SOCKET                             Socket,
    IN LPWSAOVERLAPPED                    pUserOverlappedStruct,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE pUserCompletionRoutine,
    IN LPWSATHREADID                      UserThreadId,
    IN LPWSABUF                           pUserBuffer,
    IN DWORD                              UserBufferCount,
    IN LPWSABUF                           pInternalBuffer,
    IN DWORD                              InternalBufferCount,
    IN struct sockaddr FAR *        SockAddr,
    IN INT                                SockAddrLen,
    IN LPINT                              SockAddrLenPtr,
    IN DWORD                              Flags,
    IN LPDWORD                            FlagsPtr
    )
/*++
Routine Description:

    This routine stores its arguments into an internal overlapped struct. This
    routine is used by all the overlapped operation queuing routines.

Arguments:

    pOverlappedStruct - A pointer to an internal overlapped structure.

    OperationType - The operation code for the operation being performed.

    Provider - A pointer to a RPROVIDER object that is receive the overlapped
               request.
    Socket - The provider socket descriptor.

    pUserOverlappedStruct - The pointer to the users overlapped structure.

    pUserCompletionRoutine - The pointer to the users completetion routine.

    UserThreadId - The users thread ID.

    pUserBuffer - The pointer to the user buffer(s)

    UserBufferCount - The number of user buffers.

    pInternalBuffer - The pointer to our internal buffers.

    InternalBufferCount - The number of internal buffers.

    SockAddr - The pointer to the user sockaddr structure associated with this
               operation.
    SockAddrLen - The lenght of SockAddr.

    SockAddrLenPtr - The pointer to the user sockaddr length.

    Flags - The user flags for the operation.

    FlagsPtr - A pointer to the user flags parameter.

Return Value:

    NO_ERROR

--*/
{
    INT ReturnCode;

    pOverlappedStruct->OperationType = OperationType;
    pOverlappedStruct->Provider = Provider;
    pOverlappedStruct->Socket = Socket;
    pOverlappedStruct->UserOverlappedStruct = pUserOverlappedStruct ;
    pOverlappedStruct->UserCompletionRoutine = pUserCompletionRoutine;
    pOverlappedStruct->UserThreadId = UserThreadId;
    pOverlappedStruct->UserBuffer = pUserBuffer;
    pOverlappedStruct->UserBufferCount = UserBufferCount;
    pOverlappedStruct->InternalBuffer = pInternalBuffer;
    pOverlappedStruct->InternalBufferCount = InternalBufferCount;
    pOverlappedStruct->SockAddr = SockAddr;
    pOverlappedStruct->SockAddrLen = SockAddrLen;
    pOverlappedStruct->SockAddrLenPtr = SockAddrLenPtr;
    pOverlappedStruct->Flags = Flags;
    pOverlappedStruct->FlagsPtr = FlagsPtr;

    ReturnCode = NO_ERROR;

    return(ReturnCode);
}


INT
RWORKERTHREAD::RetrieveOverlappedParams(
    IN  LPWSAOVERLAPPED                     pCompletedOverlappedStruct,
    OUT LPDWORD                             Operation,
    OUT LPWSAOVERLAPPED*                    pUserOverlappedStruct,
    OUT LPWSAOVERLAPPED_COMPLETION_ROUTINE* pUserCompletionRoutine,
    OUT LPWSATHREADID*                      UserThreadId,
    OUT LPWSABUF*                           pUserBuffer,
    OUT DWORD*                              UserBufferCount,
    OUT LPWSABUF*                           pInternalBuffer,
    OUT DWORD*                              InternalBufferCount
    )
{
    INT ReturnCode;
    PINTERNALOVERLAPPEDSTRUCT pOverlappedStruct;

    ReturnCode = WSAEINVAL;

    pOverlappedStruct = m_overlapped_struct_manager->GetInternalStuctPointer(
        pCompletedOverlappedStruct);

    if (STRUCTSIGNATURE == pOverlappedStruct->Signature){
        *Operation              = pOverlappedStruct->OperationType;
        *pUserOverlappedStruct  = pOverlappedStruct->UserOverlappedStruct;
        *pUserCompletionRoutine = pOverlappedStruct->UserCompletionRoutine;
        *UserThreadId           = pOverlappedStruct->UserThreadId;
        *pUserBuffer            = pOverlappedStruct->UserBuffer;
        *UserBufferCount        = pOverlappedStruct->UserBufferCount;
        *pInternalBuffer        = pOverlappedStruct->InternalBuffer;
        *InternalBufferCount    = pOverlappedStruct->InternalBufferCount;

        ReturnCode = NO_ERROR;
    } //if
    return(ReturnCode);
}

VOID
RWORKERTHREAD::AddOverlappedOperation(
    IN PINTERNALOVERLAPPEDSTRUCT OverlappedOperation
    )
/*++
Routine Description:

    This routine adds an internal overlapped structure to the queue of requests
    to be completed by the worker thread.

Arguments:

    OverlappedOperation - A pointer to an internal overlapped structure that
                          describes the operation to be performed by the worker
                          thread.

Return Value:

    NONE
--*/
{
    EnterCriticalSection(&m_overlapped_operation_queue_lock);
    InsertTailList(
        &m_overlapped_operation_queue,
        &OverlappedOperation->ListLinkage);
    LeaveCriticalSection(&m_overlapped_operation_queue_lock);
    SetEvent(m_wakeup_event);
}


PINTERNALOVERLAPPEDSTRUCT
RWORKERTHREAD::NextOverlappedOperation()
/*++
Routine Description:

    This routine returns the first internal overlapped structure from the queue
    of requests to be completed by the worker thread.

Arguments:

    NONE

Return Value:

    A pointer to an internal overlapped structure or NULL

--*/
{
    PINTERNALOVERLAPPEDSTRUCT ReturnValue;
    PLIST_ENTRY               ListEntry;

    ReturnValue = NULL;

    EnterCriticalSection(&m_overlapped_operation_queue_lock);

    if (!IsListEmpty(&m_overlapped_operation_queue)){
        ListEntry = RemoveHeadList(
            &m_overlapped_operation_queue);
        ReturnValue = CONTAINING_RECORD(
            ListEntry,
            INTERNALOVERLAPPEDSTRUCT,
            ListLinkage);
    } //if

    LeaveCriticalSection(&m_overlapped_operation_queue_lock);
    return(ReturnValue);
}


