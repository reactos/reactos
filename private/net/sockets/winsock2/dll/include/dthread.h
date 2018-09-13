/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

dthread.h

Abstract:

This  header  defines the "DTHREAD" class.  The DTHREAD class defines state
variables  and  operations for DTHREAD objects within the WinSock 2 DLL.  A
DTHREAD object represents all of the information known about a thread using
the Windows Sockets API.

Author:

Paul Drews (drewsxpa@ashland.intel.com) 9-July-1995

Notes:

$Revision:   1.19  $

$Modtime:   20 Feb 1996 14:19:04  $

Revision History:

most-recent-revision-date email-name
description

    23-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved single line functions to be inlines.

    07-17-1995  dirk@mink.intel.com
        Moved function descriptions to implementation file. Added member
        variable to hold the handle of the Async Helper device.

    07-09-1995  drewsxpa@ashland.intel.com
        Completed  first  complete  version with clean compile and released for
        subsequent implementation.

    9-July-1995 drewsxpa@ashland.intel.com
        Original version

--*/


#ifndef _DTHREAD_
#define _DTHREAD_

#include "winsock2.h"
#include <windows.h>
#include "llist.h"
#include "ws2help.h"
#include "classfwd.h"


#define RESULT_BUFFER_SIZE 32

#define MAX_PROTO_TEXT_LINE 511
#define MAX_PROTO_ALIASES   35

typedef struct _GETPROTO_INFO {

    struct protoent Proto;
    CHAR * Aliases[MAX_PROTO_ALIASES];
    CHAR TextLine[MAX_PROTO_TEXT_LINE+1];

} GETPROTO_INFO, *PGETPROTO_INFO;


class DTHREAD
{
  public:


    static
    INT
    DThreadClassInitialize(
        VOID);

    static
    VOID
    DThreadClassCleanup(
        VOID);

    static
    INT
    GetCurrentDThread(
        IN  PDPROCESS  Process,
        OUT PDTHREAD FAR * CurrentThread
        );

    static
    LPWSATHREADID
    GetCurrentDThreadID(
        IN  PDPROCESS  Process
        );

    static
    INT
    CreateDThreadForCurrentThread(
        IN  PDPROCESS  Process,
        OUT PDTHREAD FAR * CurrentThread
        );

    static
    VOID
    DestroyCurrentThread(
        VOID);

    DTHREAD(
        VOID);

    INT
    Initialize(
        IN PDPROCESS  Process
        );

    ~DTHREAD();


    PCHAR
    GetResultBuffer();

    PCHAR
    CopyHostEnt(LPBLOB pBlob);

    PCHAR
    CopyServEnt(LPBLOB pBlob);

    LPWSATHREADID
    GetWahThreadID();

    LPBLOCKINGCALLBACK
    GetBlockingCallback();

    BOOL
    IsBlocking();

    INT
    CancelBlockingCall();

    FARPROC
    SetBlockingHook(
        FARPROC lpBlockFunc
        );

    INT
    UnhookBlockingHook();

    VOID
    SetOpenType(
        INT OpenType
        );

    INT
    GetOpenType();

    PGETPROTO_INFO
    GetProtoInfo();
#if 0
    //Data member
    LIST_ENTRY  m_dprocess_linkage;

    // Provides the linkage space for a list of DTHREAD objects maintained by
    // the  DPROCESS  object  associated with this DTHREAD object.  Note that
    // this member variable must be public so that the linked-list macros can
    // maniplate the list linkage from within the DPROCESS object's methods.
#endif 
  private:

    static
    INT
    WINAPI
    DefaultBlockingHook();

    static
    BOOL
    CALLBACK
    BlockingCallback(
        DWORD_PTR dwContext
        );

    static DWORD  sm_tls_index;
    // The  class-scope  index  in  thread-local  storage  where  the DTHREAD
    // reference for the thread is stored.

    WSATHREADID  m_wah_thread_id;
    // The  thread  id  used  by  the  WinSock  Async  Helper  mechanism  for
    // processing IO completion callbacks.

    LPBLOCKINGCALLBACK m_blocking_callback;
    FARPROC m_blocking_hook;
    // The  pointer  to  the current client-level blocking hook procedure for
    // the thread.

    HANDLE  m_wah_helper_handle;
    // Handle to the APC helper device

    CHAR  m_result_buffer[RESULT_BUFFER_SIZE];

    //
    // m_hostent_buffer is used to construct a hostent for calls
    // such as gethostbyname. It also contains space for the
    // WSALookupServiceNext results structure.
    //

    PCHAR  m_hostent_buffer;
    PCHAR  m_servent_buffer;
    WORD   m_hostent_size;
    WORD   m_servent_size;

    PDPROCESS  m_process;
    // Reference to the DPROCESS object with which this thread is associated.

    BOOL m_is_blocking;
    // TRUE if this thread is currently in a blocking API.

    BOOL m_io_cancelled;
    // TRUE if current I/O has been cancelled.

    LPWSPCANCELBLOCKINGCALL m_cancel_blocking_call;
    // Pointer to current provider's cancel routine.

    INT m_open_type;
    // Current default socket() open type.

    PGETPROTO_INFO m_proto_info;
    // State for getprotobyXxx().

};  // class DTHREAD


inline PCHAR
DTHREAD::GetResultBuffer()
/*++

Routine Description:

    This function retrieves the pointer to the thread specific result buffer.

Arguments:

Return Value:

    The pointer to the thread specific buffer.

--*/
{
    return(&m_result_buffer[0]);
} //GetResultBuffer

inline PCHAR
DTHREAD::CopyHostEnt(LPBLOB pBlob)
/*++

Routine Description:

    This function copies the hostent in the blob and returns a pointer
    to the per-thread buffer

Arguments:

Return Value:

    The pointer to the thread specific buffer.

--*/
{
    if(m_hostent_size < pBlob->cbSize)
    {
        delete m_hostent_buffer;
        m_hostent_buffer = new CHAR[pBlob->cbSize];
        m_hostent_size = (WORD)pBlob->cbSize;
    }
    if(m_hostent_buffer)
    {
        memcpy(m_hostent_buffer, pBlob->pBlobData, pBlob->cbSize);
    }
    else
    {
        m_hostent_size = 0;
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
    }
    return(m_hostent_buffer);
}

inline PCHAR
DTHREAD::CopyServEnt(LPBLOB pBlob)
/*++

Routine Description:

    This function copies the servent in the blob and returns a pointer
    to the per-thread buffer

Arguments:

Return Value:

    The pointer to the thread specific buffer.

--*/
{
    if(m_servent_size < pBlob->cbSize)
    {
        delete m_servent_buffer;
        m_servent_buffer = new CHAR[pBlob->cbSize];
        m_servent_size = (WORD)pBlob->cbSize;
    }
    if(m_servent_buffer)
    {
        memcpy(m_servent_buffer, pBlob->pBlobData, pBlob->cbSize);
    }
    else
    {
        m_servent_size = 0;
        SetLastError(WSA_NOT_ENOUGH_MEMORY);
    }
    return(m_servent_buffer);
}



inline LPWSATHREADID
DTHREAD::GetWahThreadID()
/*++

Routine Description:

    This  procedure  retrieves  the  per-thread "Thread ID" used by the WinSock
    Asynchronous  Helper  Thread  ID  mechanism  during  the  delivery  of a IO
    completion callback to the client's thread context.

Arguments:

    None

Return Value:

    Returns  the  WinSock  Asynchronous  Helper  Thread ID corresponding to the
    current thread.

Notes:

    // The WahThreadID is created during Initialize, because otherwise we could
    // encounter  an  error  while  trying to complete an overlapped operation,
    // even though the SP part succeeded.
    //
    // There  is  no special benefit in having the DPROCESS object postpone its
    // Wah-related  initialization  until  demanded,  since it will be demanded
    // essentially right away as a parameter added to each IO function.  If the
    // SPI  semantics were changed to only include the thread ID in cases where
    // async  callbacks  are  really  required,  there  could  be  a benefit in
    // postponing  creation of the WahThreadID until we were sure it was really
    // needed.
--*/
{
    return(& m_wah_thread_id);
} //GetWahThreadID



inline
LPBLOCKINGCALLBACK
DTHREAD::GetBlockingCallback()
/*++

Routine Description:

    Returns the blocking callback function pointer for this thread.

Arguments:

    None.

Return Value:

    The pointer to blocking callback function. Note that this may be NULL.

--*/
{
    return m_blocking_callback;
} // GetBlockingCallback



inline
BOOL
DTHREAD::IsBlocking()
/*++

Routine Description:

    Determines if the current thread is currently in a blocking operation.

Arguments:

    None.

Return Value:

    TRUE if the thread is blocking, FALSE otherwise.

--*/
{
    return m_is_blocking;
} // IsBlocking



inline
VOID
DTHREAD::SetOpenType(
    INT OpenType
    )
/*++

Routine Description:

    Sets the "open type" for this thread, as set by the SO_OPENTYPE socket
    option.

Arguments:

    OpenType - The new open type.

Return Value:

    None.

--*/
{
    m_open_type = OpenType;
} // SetOpenType



inline
INT
DTHREAD::GetOpenType()
/*++

Routine Description:

    Returns "open type" for this thread.

Arguments:

    None.

Return Value:

    The open type for this thread.

--*/
{
    return m_open_type;
} // GetOpenType


inline 
LPWSATHREADID
DTHREAD::GetCurrentDThreadID(
    IN  PDPROCESS  Process
    )
{
    PDTHREAD    Thread;
    Thread = (DTHREAD*)TlsGetValue(sm_tls_index);
    if (Thread!=NULL) {
        return Thread->GetWahThreadID ();
    }
    else
        return NULL;
}


inline INT
DTHREAD::GetCurrentDThread(
    IN  PDPROCESS  Process,
    OUT PDTHREAD FAR * CurrentThread
    )
/*++

Routine Description:

    This  procedure  retrieves a reference to a DTHREAD object corresponding to
    the  current  thread.  It takes care of creating and initializing a DTHREAD
    object and installing it into the thread's thread-local storage if there is
    not already a DTHREAD object for this thread.  Note that this is a "static"
    function with global scope instead of object-instance scope.

    Note  that  this  is  the  ONLY  procedure  that should be used to create a
    DTHREAD   object  outside  of  the  DTHREAD  class.   The  constructor  and
    Initialize  function  should  only  be  used  internally within the DTHREAD
    class.

Arguments:

    Process       - Supplies a reference to the DPROCESS object associated with
                    this DTHREAD object.

    CurrentThread - Returns  the  DTHREAD  object  corresponding to the current
                    thread.

Return Value:

    The  function  returns ERROR_SUCCESS if successful, otherwise it
    returns an appropriate WinSock error code.
--*/
{
    *CurrentThread = (DTHREAD*)TlsGetValue(sm_tls_index);
    if (*CurrentThread) {
        return ERROR_SUCCESS;
    } //if
    else {
        return CreateDThreadForCurrentThread (Process, CurrentThread);
    }
}

#endif // _DTHREAD_

