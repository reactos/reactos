/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    dthread.cpp

Abstract:

    This  module  contains  the  implementation  of  the  DTHREAD class used in
    winsock2 DLL.

Author:

    Dirk Brandewie (dirk@mink.intel.com)   14-July-1995

Notes:

    $Revision:   1.27  $

    $Modtime:   08 Mar 1996 14:59:46  $

Revision History:

    most-recent-revision-date email-name
        description
    23-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved includes into precomp.h. Added
        debug/trace code.

--*/
#include "precomp.h"

extern DWORD gdwTlsIndex;

DWORD DTHREAD::sm_tls_index = TLS_OUT_OF_INDEXES;
// Initialize the static member to a known variable



INT
DTHREAD::DThreadClassInitialize()
/*++

Routine Description:

    This  function  performs  global  initialization  required  for the DTHREAD
    class.   This  function  must  be  called  before  any  DTHREAD objects are
    created.  In particular, this function reserves a thread-local storage slot
    for  the thread-local storage used by the WinSock 2 DLL.  Note that this is
    a "static" function with global scope instead of object-instance scope.

Arguments:

    None

Return Value:

    The  function  returns ERROR_SUCCESS if successful, otherwise it
    returns an appropriate WinSock error code.
--*/
{
    INT ReturnCode = WSASYSCALLFAILURE; // the user return code

    if (sm_tls_index == TLS_OUT_OF_INDEXES) {
        DEBUGF( DBG_TRACE,
                ("Initializing dthread class\n"));
        sm_tls_index = gdwTlsIndex;
        if (sm_tls_index != TLS_OUT_OF_INDEXES) {
            ReturnCode = ERROR_SUCCESS;
        } //if
    } //if
    else {
         ReturnCode = ERROR_SUCCESS;
    } //else

    return(ReturnCode);
} //DThreadClassInitialize




VOID
DTHREAD::DThreadClassCleanup()
/*++

Routine Description:

    This  routine  de-inits the thread class.  The thread local storage slot is
    freed.   Note that this is a "static" function with global scope instead of
    object-instance scope.

Arguments:

    None

Return Value:

    None
--*/
{
    //
    // NOTE: The following is bogus, as it means the tls index will never
    // get freed.  Have since taken care of things by alloc'ing/free'ing
    // tls index in DllMain (process attach/detach handlers)
    //

    //
    // This code is elided. We retain the tls index so that thread terminating
    // after the WSACleanup can free the per-thread storage. Since this
    // operation is done only when the thread actually detaches, there's
    // a race between deleting this tls index and the thread completing.
    // So, keep it around. Note that if another WSAStartup is done, the
    // code will simply use this index.
    //

    //
    // Resurected this code but now calling from DllMain (DLL_PROCESS_DETACH)
    // VadimE.
    //
    DEBUGF( DBG_TRACE,
            ("Cleaning up dthread class\n"));
    // Killing it again
    // taking care of it directly in DLLMain
    // VadimE
    if (sm_tls_index != TLS_OUT_OF_INDEXES)
        {
        // TlsFree(sm_tls_index); // Free the TLS slot
        sm_tls_index = TLS_OUT_OF_INDEXES;
    } //if

} //DThreadClassCleanup



INT
DTHREAD::CreateDThreadForCurrentThread(
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
    INT ReturnCode = WSASYSCALLFAILURE;  // Return Code
    PDTHREAD LocalThread=NULL;            // Temp thread object pointer

    if (sm_tls_index != TLS_OUT_OF_INDEXES){
        // No thread object for the current thread so create one
        // and initialize the new object
        LocalThread = new(DTHREAD);
        if (LocalThread) {
            if (LocalThread->Initialize(Process) == ERROR_SUCCESS) {
                if (TlsSetValue(sm_tls_index, LocalThread)) {
                    if (Process->DThreadAttach(
                            LocalThread
                            ) == ERROR_SUCCESS) {
                        *CurrentThread = LocalThread;
                        ReturnCode = ERROR_SUCCESS;
                    } //if
                } //if
            } //if

            if (ERROR_SUCCESS != ReturnCode){
                delete(LocalThread);
            } //if
        } //if
    } //if
    return(ReturnCode);
} //CreateDThreadForCurrentThread

VOID
DTHREAD::DestroyCurrentThread()
/*++

Routine Description:

    This routine destroys the thread object associated with the
    currently running thread.

Arguments:

Return Value:

    The  function  returns TRUE if the thread was sucessfully
    destroyed else FALSE
--*/
{
    INT       ReturnCode;
    PDPROCESS Process;
    PDTHREAD  Thread;

    // Is Thread local Storage been inited
    if (sm_tls_index != TLS_OUT_OF_INDEXES)
        {
        Thread = (DTHREAD*)TlsGetValue(sm_tls_index);
        if (Thread)
        {
            Process = DPROCESS::GetCurrentDProcess();
            // Is there a valid process object. If so detach This thread from
            // it.
            if (Process!=NULL)
            {
                Process->DThreadDetach(Thread);
            } //if
            delete(Thread);
        } //if
    } //if
}



DTHREAD::DTHREAD()
/*++

Routine Description:

    DTHREAD  object  constructor.   Creates and returns a DTHREAD object.  Note
    that  the  DTHREAD object has not been fully initialized.  The "Initialize"
    member function must be the first member function called on the new DTHREAD
    object.

    Note  that  this  procedure  should  not be used to create a DTHREAD object
    outside  of  the  DTHREAD  class.   This procedure is only for internal use
    within  the DTHREAD class.  The static "GetCurrentDThread" procedure should
    be  used  to  retrieve  a reference to a DTHREAD object outside the DTHREAD
    class.

Arguments:

    None

Return Value:

--*/
{
    // Set data member to known values
    m_blocking_hook        = (FARPROC)&DTHREAD::DefaultBlockingHook;
    m_blocking_callback    = NULL;
    m_process              = NULL;
    m_hostent_buffer       = NULL;
    m_servent_buffer       = NULL;
    m_hostent_size         = 0;
    m_servent_size         = 0;
    m_is_blocking          = FALSE;
    m_io_cancelled         = FALSE;
    m_cancel_blocking_call = NULL;
    m_open_type            = 0;
    m_proto_info           = NULL;

} //DTHREAD




INT
DTHREAD::Initialize(
    IN PDPROCESS  Process
    )
/*++

Routine Description:

    Completes the initialization of the DTHREAD object.  This must be the first
    member  function  called  for the DTHREAD object.  This procedure should be
    called only once for the object.

    Note  that  this  procedure  should  only  be  called internally within the
    DTHREAD  class.   Outside  of  the class, the "GetCurrentDThread" procedure
    should  be  used  to  retrieve  a  reference to a fully initialized DTHREAD
    object.

Arguments:

    Process - Supplies  a reference to the DPROCESS object associated with this
              DTHREAD object.

Return Value:

    The  function  returns ERROR_SUCCESS if successful, otherwise it
    returns an appropriate WinSock error code.
--*/
{
    INT ReturnCode= WSASYSCALLFAILURE;

    m_process = Process; // Store process pointer

    DEBUGF( DBG_TRACE,
            ("Initializing dthread %X\n", this));

    // Init WAH thread:
    // Open the helper device
    if (Process->GetAsyncHelperDeviceID(&m_wah_helper_handle) ==
        ERROR_SUCCESS) {
        //Initialize helper thread ID structure
        if (WahOpenCurrentThread(m_wah_helper_handle,
                                 & m_wah_thread_id) == ERROR_SUCCESS) {
                ReturnCode = ERROR_SUCCESS;
        } //if
    } //if

    return(ReturnCode);
} //Initialize




DTHREAD::~DTHREAD()
/*++

Routine Description:

    DTHREAD  object  destructor.   This  procedure  has  the  responsibility to
    perform  any required shutdown operations for the DTHREAD object before the
    object  memory  is  deallocated.  The caller is reponsible for removing the
    object  from  its list in the DPROCESS object before destroying the DTHREAD
    object.

    This  procedure takes care of removing the DTHREAD object from thread-local
    storage.

Arguments:

    None

Return Value:

    None
--*/
{
    BOOL bresult;

    DEBUGF( DBG_TRACE,
            ("Freeing dthread %X\n", this));

    assert(sm_tls_index != TLS_OUT_OF_INDEXES);

    m_blocking_hook = NULL;

    delete m_hostent_buffer;
    delete m_servent_buffer;
    delete m_proto_info;

    bresult = TlsSetValue(
        sm_tls_index,  // dwTlsIndex
        (LPVOID) NULL  // lpvTlsValue
        );
    if (! bresult) {
        DEBUGF(
            DBG_WARN,
            ("Resetting Thread-local storage for this thread\n"));
    }

    WahCloseThread(
        m_wah_helper_handle,
        & m_wah_thread_id);
    m_wah_helper_handle = NULL;

    m_process = NULL;
} //~DTHREAD


INT
DTHREAD::CancelBlockingCall()
{
    INT result;
    INT err;

    //
    // Bail if the thread is not blocking.
    //

    if( !m_is_blocking ) {

        return WSAEINVAL;

    }

    //
    // Verify we've got the blocking pointers setup correctly.
    //

    assert( m_blocking_callback != NULL );
    assert( m_cancel_blocking_call != NULL );

    //
    // If the IO request has not already been cancelled, call the
    // cancellation routine.
    //

    if( !m_io_cancelled ) {

        result = (m_cancel_blocking_call)( &err );

        if( result != ERROR_SUCCESS ) {
            return err;
        }

        m_io_cancelled = TRUE;

    }

    return ERROR_SUCCESS;

}   // DTHREAD::CancelBlockingCall


FARPROC
DTHREAD::SetBlockingHook(
    FARPROC lpBlockFunc
    )
{
    FARPROC PreviousHook;

    //
    // Snag the current hook so we can return it as the previous hook.
    //

    PreviousHook = m_blocking_hook;

    //
    // Set the current hook & the appropriate blocking callback.
    //

    if( lpBlockFunc == (FARPROC)&DTHREAD::DefaultBlockingHook ) {
        m_blocking_callback = NULL;
    } else {
        m_blocking_callback = &DTHREAD::BlockingCallback;
    }

    m_blocking_hook = lpBlockFunc;

    return PreviousHook;

}   // DTHREAD::SetBlockingHook


INT
DTHREAD::UnhookBlockingHook()
{

    //
    // Just reset everything back to defaults.
    //

    m_blocking_hook = (FARPROC)DTHREAD::DefaultBlockingHook;
    m_blocking_callback = NULL;

    return ERROR_SUCCESS;

}   // DTHREAD::UnhookBlockingHook


INT
WINAPI
DTHREAD::DefaultBlockingHook()
{

    MSG msg;
    BOOL retrievedMessage;

    __try {
        //
        // Get the next message for this thread, if any.
        //

        retrievedMessage = PeekMessage( &msg, NULL, 0, 0, PM_REMOVE );

        //
        // Process the message if we got one.
        //

        if ( retrievedMessage ) {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }
    __except (WS2_EXCEPTION_FILTER ()) {
        retrievedMessage = FALSE; 
    }

    //
    // If we got a message, indicate that we want to be called again.
    //

    return retrievedMessage;

}   // DTHREAD::DefaultBlockingHook


BOOL
CALLBACK
DTHREAD::BlockingCallback(
    DWORD_PTR dwContext
    )
{
    PDTHREAD Thread;

    assert( dwContext != 0 );

    //
    // Just grab the DTHREAD pointer directly out of the thread local
    // storage. Since we came in through a blocking hook, we must have
    // already setup this stuff.
    //

    Thread = (DTHREAD *)TlsGetValue( sm_tls_index );
    assert( Thread != NULL );

    //
    // Set the blocking flag and the pointer to the cancel function
    // and clear the i/o cancelled flag.
    //

    Thread->m_is_blocking = TRUE;
    Thread->m_cancel_blocking_call = (LPWSPCANCELBLOCKINGCALL)dwContext;
    Thread->m_io_cancelled = FALSE;

    //
    // Call the user's blocking hook.
    //

    assert( Thread->m_blocking_hook != NULL );
    assert( Thread->m_blocking_hook != (FARPROC)&DTHREAD::DefaultBlockingHook );

    while( (Thread->m_blocking_hook)() ) {

        //
        // This space intentionally left blank.
        //

    }

    //
    // Reset the blocking flag and return TRUE if everything was OK,
    // FALSE if the operation was cancelled.
    //

    Thread->m_is_blocking = FALSE;

    return !Thread->m_io_cancelled;

}   // DTHREAD::BlockingCallback





PGETPROTO_INFO
DTHREAD::GetProtoInfo()
/*++

Routine Description:

    Returns a pointer to the state structure used for the
    getprotobyXxx() APIs.

Arguments:

    None.

Return Value:

    Pointer to the state structure.

--*/
{

    //
    // Allocate the buffer if necessary.
    //

    if( m_proto_info == NULL ) {

        m_proto_info = new GETPROTO_INFO;

    }

    return m_proto_info;

} // GetProtoInfo
