//---------------------------------------------------------------------------
//  Copyright (c) 1997 Microsoft Corporation
//
//  rworker.h
//
//  Define RWORKERTHREAD class for the restricted process's TCP/IP socket.
//  It is an thread object that its used to wait on and signal for events
//  for implementation of WSAEventSelect().
//
//  Revision History:
//
//  edwardr    11-05-97    Initial version.
//
//---------------------------------------------------------------------------
/*++


     Copyright c 1996 Intel Corporation
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

#ifndef _RWORKERTHREAD_
#define _RWORKERTHREAD_

#include <windows.h>
#include "llist.h"
#include "doverlap.h"
#include "fwdref.h"

#define WSP_RECV       0x00000001
#define WSP_RECVFROM   0x00000002
#define WSP_SEND       0x00000004
#define WSP_SENDTO     0x00000008


class RWORKERTHREAD
{
  public:

    RWORKERTHREAD();

    INT
    Initialize(
        );

    ~RWORKERTHREAD();

    INT
    RegisterSocket(
        PRSOCKET Socket
        );
    INT
    UnregisterSocket(
        PRSOCKET Socket
        );

    DWORD
    ThreadProc();

    LPWSATHREADID
    GetThreadId();

    INT
    QueueOverlappedRecv(
        PRPROVIDER                         Provider,
        SOCKET                             UserSocket,
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
        );

    INT
    QueueOverlappedRecvFrom(
        PRPROVIDER                         Provider,
        SOCKET                             UserSocket,
        LPWSABUF                           UserBuffers,
        DWORD                              UserBufferCount,
        LPDWORD                            UserBytesRecvd,
        LPDWORD                            UserFlags,
        struct sockaddr FAR *              UserFrom,
        LPINT                              UserFromlen,
        LPWSAOVERLAPPED                    UserOverlappedStruct,
        LPWSAOVERLAPPED_COMPLETION_ROUTINE UserlpCompletionRoutine,
        LPWSATHREADID                      UserThreadId,
        LPWSABUF                           InternalBuffers,
        DWORD                              InternalBufferCount,
        LPINT                              Errno
        );
    INT
    QueueOverlappedSend(
        PRPROVIDER Provider,
        SOCKET                             UserSocket,
        LPWSABUF                           UserBuffers,
        DWORD                              UserBufferCount,
        LPDWORD                            UserBytesSent,
        DWORD                            UserFlags,
        LPWSAOVERLAPPED                    UserOverlappedStruct,
        LPWSAOVERLAPPED_COMPLETION_ROUTINE UserlpCompletionRoutine,
        LPWSATHREADID                      UserThreadId,
        LPINT           Errno
        );

    INT
    QueueOverlappedSendTo(
        PRPROVIDER                         Provider,
        SOCKET                             UserSocket,
        LPWSABUF                           UserBuffers,
        DWORD                              UserBufferCount,
        LPDWORD                            UserBytesSent,
        DWORD                              UserFlags,
        const struct sockaddr FAR *        UserTo,
        INT                                UserTolen,
        LPWSAOVERLAPPED                    UserOverlappedStruct,
        LPWSAOVERLAPPED_COMPLETION_ROUTINE UserlpCompletionRoutine,
        LPWSATHREADID                      UserThreadId,
        LPINT                              Errno
        );
    INT
    StoreOverlappedParams(
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
        IN sockaddr FAR *        SockAddr,
        INT                                   SockAddrLen,
        LPINT                                 SockAddrLenPtr,
        DWORD                                 Flags,
        LPDWORD                               FlagsPtr
        );

    INT
    RetrieveOverlappedParams(
        IN  LPWSAOVERLAPPED                     pOverlappedStruct,
        OUT LPDWORD                             Operation,
        OUT LPWSAOVERLAPPED*                    pUserOverlappedStruct,
        OUT LPWSAOVERLAPPED_COMPLETION_ROUTINE* pUserCompletionRoutine,
        OUT LPWSATHREADID*                      UserThreadId,
        OUT LPWSABUF*                           pUserBuffer,
        OUT DWORD*                              UserBufferCount,
        OUT LPWSABUF*                           pInternalBuffer,
        OUT DWORD*                              InternalBufferCount
        );

    VOID
    FreeApcContext(
        PAPCCONTEXT Context
        );

    PAPCCONTEXT
    AllocateApcContext();

    VOID
    FreeOverlappedStruct(
        LPWSAOVERLAPPED   pOverlappedStruct
        );

  private:
    INT
    AddSocket(
        PRSOCKET Socket,
        HANDLE   Event
        );

    INT
    RemoveSocket(
        PRSOCKET Socket
        );

    VOID
    AddOverlappedOperation(
        IN PINTERNALOVERLAPPEDSTRUCT OverlappedOperation
        );

    PINTERNALOVERLAPPEDSTRUCT
    NextOverlappedOperation();

    HANDLE m_thread_handle;
    // The handle to our worker thread.

    HANDLE m_wakeup_event;
    // The handle to the WIN32 event used to communicate with our worker
    // thread.

    DWORD  m_event_count;
    // The number of events our worker thread is waiting on.

    BOOL   m_exit_thread;
    // A boolean to tell our thread to exit at object destruction time.

    HANDLE m_wait_array[MAXIMUM_WAIT_OBJECTS];
    // An array of objects our thread is waiting on.

    DWORD_PTR m_socket_array[MAXIMUM_WAIT_OBJECTS];
    // An array of RSOCKET pointers that correspond the the events the thread
    // is waiting on.

    CRITICAL_SECTION m_array_lock;
    // CriticalSection to protect access to m_wait_array and m_socket_array.

    LIST_ENTRY       m_overlapped_operation_queue;

    CRITICAL_SECTION m_overlapped_operation_queue_lock;

    PDOVERLAPPEDSTRUCTMGR  m_overlapped_struct_manager;

    WSATHREADID  m_thread_id;
    // The thread ID for the worker thread.

};   // class RWORKERTHREAD

inline
LPWSATHREADID
RWORKERTHREAD::GetThreadId()
{
    return(&m_thread_id);
}

inline
VOID
RWORKERTHREAD::FreeApcContext(
    PAPCCONTEXT Context
    )
{
    m_overlapped_struct_manager->FreeApcContext(
        Context);
}
inline
PAPCCONTEXT
RWORKERTHREAD::AllocateApcContext()
{
    return(m_overlapped_struct_manager->AllocateApcContext());
}

inline
VOID
RWORKERTHREAD::FreeOverlappedStruct(
        LPWSAOVERLAPPED   pOverlappedStruct
        )
{
    m_overlapped_struct_manager->FreeOverlappedStruct(pOverlappedStruct);
}


#endif // _RWORKERTHREAD_
