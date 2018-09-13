/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    dprocess.cpp

Abstract:

    This module contains the implementation of the dprocess class.

Author:

    Dirk Brandewie dirk@mink.intel.com  11-JUL-1995

Revision History:

    21-Aug-1995 dirk@mink.intel.com
       Cleanup after code review. Moved single line functions to header file as
       inlines. Added debug/trace code. Changed LIST_ENTRY's and
       CRITICAL_SECTION's from pointers to being embedded in the dprocess
       object.

--*/

#include "precomp.h"

// This is a static class member. It contains a pointer to the dprocess object
// for the current process.
PDPROCESS DPROCESS::sm_current_dprocess=NULL;



DPROCESS::DPROCESS(
    )
/*++

Routine Description:

    DPROCESS  object constructor.  Creates and returns a DPROCESS object.  Note
    that  the DPROCESS object has not been fully initialized.  The "Initialize"
    member  function  must  be  the  first  member  function  called on the new
    DPROCESS object.

    In  the Win32 environment, only one DPROCESS object may be in existence for
    a  process.   It  is  the  caller's  responsibility  to  ensure  that  this
    restriction is met.

Arguments:

    None

Return Value:

    Returns a pointer to the new DPROCESS object or NULL if a memory allocation
    failed.

--*/
{
    //
    // Initialize the list objects
    //
#if 0
    // Not used because of inherent race conditions.
    InitializeListHead(&m_thread_list);
#endif

    // Set our data members to known values.
    m_reference_count   = 0;
    m_ApcHelper         = NULL;
    m_HandleHelper      = NULL;
    m_NotificationHelper = NULL;
    m_protocol_catalog  = NULL;
    m_proto_catalog_change_event = NULL;
    m_namespace_catalog = NULL;
    m_ns_catalog_change_event = NULL;
    m_version           = WINSOCK_HIGH_API_VERSION; // until proven otherwise...
    m_lock_initialized = FALSE;
} //DPROCESS




INT
DPROCESS::Initialize(
    )
/*++

Routine Description:

    Completes  the  initialization  of  the DPROCESS object.  This must be the
    first  member  function  called  for  the DPROCESS object.  This procedure
    should be called only once for the object.

Arguments:

  None

Return Value:

  The  function returns 0 if successful.  Otherwise it returns an appropriate
  WinSock error code if the initialization cannot be completed.

--*/
{
    INT ReturnCode = WSAEFAULT;  // user return value
    HKEY RegistryKey = NULL;

    TRY_START(mem_guard){

        //
        // Initialize our critical sections
        //
        __try {
            InitializeCriticalSection( &m_thread_list_lock );
        }
        __except (WS2_EXCEPTION_FILTER ()) {
            ReturnCode = WSA_NOT_ENOUGH_MEMORY;
            TRY_THROW(mem_guard);
        }
        m_lock_initialized = TRUE;


        RegistryKey = OpenWinSockRegistryRoot();
        if (!RegistryKey) {
            DEBUGF(
                DBG_ERR,
                ("Opening Winsock Registry Root\n"));
            ReturnCode = WSASYSCALLFAILURE;
            TRY_THROW(mem_guard);
        } //if

        m_proto_catalog_change_event = CreateEvent(
                    (LPSECURITY_ATTRIBUTES) NULL,
                    TRUE,       // manual reset
                    FALSE,      // initially non-signaled
                    NULL        // unnamed
                    );
        if (m_proto_catalog_change_event==NULL) {
			ReturnCode = GetLastError ();
            DEBUGF(
                DBG_ERR,
                ("Creating protocol catalog change event, err:%ld\n",
                ReturnCode));
            TRY_THROW(mem_guard);
        }

        //
        // Build the protocol catalog
        //

        m_protocol_catalog = new(DCATALOG);
        if (!m_protocol_catalog) {
            DEBUGF(
                DBG_ERR,
                ("Allocating dcatalog object\n"));
            ReturnCode = WSA_NOT_ENOUGH_MEMORY;
            TRY_THROW(mem_guard);
        } //if

        ReturnCode = m_protocol_catalog->InitializeFromRegistry(
                            RegistryKey,
                            m_proto_catalog_change_event);
        if (ERROR_SUCCESS != ReturnCode) {
            DEBUGF(
                DBG_ERR,
                ("Initializing protocol catalog from registry\n"));
            TRY_THROW(mem_guard);
        } //if


        m_ns_catalog_change_event = CreateEvent(
                    (LPSECURITY_ATTRIBUTES) NULL,
                    TRUE,       // manual reset
                    FALSE,      // initially non-signaled
                    NULL        // unnamed
                    );
        if (m_ns_catalog_change_event==NULL) {
			ReturnCode = GetLastError ();
            DEBUGF(
                DBG_ERR,
                ("Creating ns catalog change event, err:%ld\n",
                ReturnCode));
            TRY_THROW(mem_guard);
        }

        //
        // Build the namespace catalog
        //

        m_namespace_catalog = new(NSCATALOG);
        if (!m_namespace_catalog) {
            DEBUGF(
                DBG_ERR,
                ("Allocating nscatalog object\n"));
            ReturnCode = WSA_NOT_ENOUGH_MEMORY;
            TRY_THROW(mem_guard);
        } //if

        ReturnCode = m_namespace_catalog->InitializeFromRegistry (
                            RegistryKey,                // ParentKey
                            m_ns_catalog_change_event   // ChangeEvent
                            );
        if (ERROR_SUCCESS != ReturnCode) {
            DEBUGF(
                DBG_ERR,
                ("Initializing name space catalog from registry\n"));
            TRY_THROW(mem_guard);
        } //if


        // Set helper object pointers to null
        m_ApcHelper = NULL;
        m_HandleHelper = NULL;
        m_NotificationHelper = NULL;

    } TRY_CATCH(mem_guard) {
		assert (ReturnCode!=ERROR_SUCCESS);
        if (m_protocol_catalog!=NULL) {
            delete(m_protocol_catalog);
            m_protocol_catalog = NULL;
        }

        if (m_proto_catalog_change_event!=NULL) {
            CloseHandle (m_proto_catalog_change_event);
            m_proto_catalog_change_event = NULL;
        }

        if (m_namespace_catalog!=NULL) {
            delete(m_namespace_catalog);
            m_namespace_catalog = NULL;
        }

        if (m_ns_catalog_change_event!=NULL) {
            CloseHandle (m_ns_catalog_change_event);
            m_ns_catalog_change_event = NULL;
        }

    } TRY_END(mem_guard);

    { // declaration block
        LONG close_result;
        if (RegistryKey) {
            close_result = RegCloseKey(
                RegistryKey);  // hkey
            assert(close_result == ERROR_SUCCESS);
        } // if
    } // declaration block

    return (ReturnCode);
} //Initialize

BOOL  
DeleteSockets(
    LPVOID              EnumCtx,
    LPWSHANDLE_CONTEXT  HContext
    ) {
    return ((PDPROCESS)EnumCtx)->DSocketDetach (HContext);
}

BOOL
DPROCESS::DSocketDetach (
    IN LPWSHANDLE_CONTEXT   HContext
    )
{
    PDSOCKET    Socket = static_cast<PDSOCKET>(HContext);
    //
    // Remove socket from the table, so no-one can find and reference
    // it again
    //
    Socket->DisassociateSocketHandle ();

    //
    // For non-IFS provider we force socket closure because provider
    // won't be able to find this socket anymore
    //
    if (!Socket->IsProviderSocket ()) {
        if (m_HandleHelper) {
            WahCloseSocketHandle (m_HandleHelper, Socket->GetSocketHandle ());
        }
    }

    //
    // Drop active reference on the socket.
    // No-one can find it anymore and thus no-one can call closesocket 
    // or WPUCloseSocket handle on it to remove active reference.
    //
    Socket->DropDSocketReference ();
    return TRUE;
}


BOOL
CleanupProtocolProviders (
    IN PVOID                PassBack,
    IN PPROTO_CATALOG_ITEM  CatalogEntry
    )
{
	PDPROVIDER		Provider;

	Provider = CatalogEntry->GetProvider ();
	if (Provider!=NULL) {
		INT	ErrorCode, ReturnValue;
        //
        // Set exception handler around this call since we
        // hold critical section (catalog lock).
        //
        __try {
		    ReturnValue = Provider->WSPCleanup (&ErrorCode);
		    if (ReturnValue!=NO_ERROR) {
			    DEBUGF (DBG_WARN,
				    ("Calling provider %ls cleanup function, ret: %ld, err: %ld.\n",
				    CatalogEntry->GetProtocolInfo()->szProtocol,
				    ReturnValue,
				    ErrorCode));
		    }
        }
        __except (WS2_EXCEPTION_FILTER ()) {
            DEBUGF(DBG_ERR, ("Unhandled exception in WSPCleanup for provider @ %p, code:%lx\n",
                    Provider, GetExceptionCode()));
            assert (FALSE);
        }
	}

	return TRUE;
}


BOOL
CleanupNamespaceProviders (
    IN PVOID                PassBack,
    IN PNSCATALOGENTRY      CatalogEntry
    )
{
	PNSPROVIDER		Provider;

	Provider = CatalogEntry->GetProvider ();
	if (Provider!=NULL) {
		INT	ReturnValue;
        //
        // Set exception handler around this call since we
        // hold critical section (catalog lock).
        //
        __try {
    		ReturnValue = Provider->NSPCleanup ();
		    if (ReturnValue!=NO_ERROR) {
			    DEBUGF (DBG_WARN,
				    ("Calling provider %ls cleanup function, ret: %ld, err: %ld.\n",
				    CatalogEntry->GetProviderDisplayString (),
				    ReturnValue,
				    GetLastError ()));
		    }
        }
        __except (WS2_EXCEPTION_FILTER ()) {
            DEBUGF(DBG_ERR, ("Unhandled exception in NSPCleanup for provider @ %p, code:%lx\n",
                    Provider, GetExceptionCode()));
            assert (FALSE);
        }
	}

	return TRUE;
}


DPROCESS::~DPROCESS()
/*++

Routine Description:

    DPROCESS  object  destructor.   This  procedure  has  the responsibility to
    perfrom any required shutdown operations for the DPROCESS object before the
    object  memory  is  deallocated.   The caller is required to do removal and
    destruction  of  all explicitly-attached objects (e.g., DPROVIDER, DSOCKET,
    DTHREAD).   Removal  or  shutdown  for  implicitly-attached  objects is the
    responsibility of this function.

Arguments:

    None

Return Value:

    None

--*/
{
    PDTHREAD    Thread;
    PDSOCKET    Socket;
    DWORD       i, j;

    //
    // Check if initialization succeeded
    //
    if (!m_lock_initialized)
        return;

    sm_current_dprocess = NULL;

    //
    // Walk the list of sockets removing each socket from the list and
    // deleting the socket
    //

    if (DSOCKET::sm_context_table)
    {
        WahEnumerateHandleContexts (DSOCKET::sm_context_table,
                                    DeleteSockets,
                                    this
                                    );
    }


    //
    // this has been removed to eliminate the race with the thread
    // detach code  which also tries to delete the thread. It is
    // impossibe to use a mutex since holding up the thread detach
    // code ties up the PEB mutex.
    // Doing this delete is desirable as it cleans up
    // memory ASAP. The only trouble is it doesn't work -- do you
    // want it fast or do you want it right?
    //
#if 0
    while (!IsListEmpty(&m_thread_list)) {
        Thread = CONTAINING_RECORD(
            m_thread_list.Flink,
            DTHREAD,
            m_dprocess_linkage);
        DEBUGF(DBG_TRACE, ("Deleting thread\n"));
        DThreadDetach(Thread);
        delete(Thread);
    } //while
#endif

    // If we opened the async helper close it now
    if (m_ApcHelper) {
        DEBUGF(DBG_TRACE, ("Closing APC helper\n"));
        WahCloseApcHelper(m_ApcHelper);
    } //if

    // If we opened the handle helper close it now
    if (m_HandleHelper) {
        DEBUGF(DBG_TRACE, ("Closing Handle helper\n"));
        WahCloseHandleHelper(m_HandleHelper);
    } //if

    // If we opened the notification helper close it now
    if (m_NotificationHelper) {
        DEBUGF(DBG_TRACE, ("Closing Notification helper\n"));
        WahCloseNotificationHandleHelper(m_NotificationHelper);
    } //if

    // delete the protocol catalog and its change event if any. 
    if (m_protocol_catalog!=NULL) {
		// First call cleanup procedures of all loaded providers.
		m_protocol_catalog->EnumerateCatalogItems (
								CleanupProtocolProviders,	// Iteration
								NULL				        // Passback
								);
        delete(m_protocol_catalog);
        m_protocol_catalog = NULL;
    }

    if (m_proto_catalog_change_event != NULL) {

        CloseHandle (m_proto_catalog_change_event);
        m_proto_catalog_change_event = NULL;
    }

    // delete the name space catalog and its change event if any. 
    if (m_namespace_catalog!=NULL) {
		// First call cleanup procedures of all loaded providers.
		m_namespace_catalog->EnumerateCatalogItems (
								CleanupNamespaceProviders,	// Iteration
								NULL				        // Passback
								);
        delete(m_namespace_catalog);
        m_namespace_catalog = NULL;
    }

    if (m_ns_catalog_change_event != NULL) {

        CloseHandle (m_ns_catalog_change_event);
        m_ns_catalog_change_event = NULL;
    }

    // Clean up critical sections
    DeleteCriticalSection( &m_thread_list_lock );

} //~DPROCESS





INT
DPROCESS::DThreadAttach(
    IN PDTHREAD NewThread
    )
/*++

Routine Description:

    Adds  a  DTHREAD  reference  into  the  list  of  threads belonging to this
    process.   The  operation  takes  care of locking and unlocking the list as
    necessary.

Arguments:

    NewThread - Supplies  the  reference to the DTHREAD object to be added into
    the list

Return Value:

    The  function  returns 0 if successful, otherwise it returns an appropriate
    WinSock error code.
--*/
{
#if 0                 // don't use this list, it has too many races
    LockDThreadList();

    DEBUGF(DBG_TRACE, ("Adding thread %X to Process\n", NewThread));
    // Add the new thread to the list of threads connected to this process.
    InsertHeadList(&m_thread_list, &(NewThread->m_dprocess_linkage));

    UnLockDThreadList();
#endif
    return(0);
} //DThreadAttach



INT
DPROCESS::DThreadDetach(
    IN PDTHREAD  OldThread
    )
/*++

Routine Description:

    Removes  a  DTHREAD  reference  from  the list of threads belonging to this
    process.   The  operation  takes  care of locking and unlocking the list as
    necessary.

Arguments:

    OldThread - Supplies the reference to the DTHREAD object to be removed from
    the list.

Return Value:

    The  function  returns 0 if successful, otherwise it returns an appropriate
    WinSock error code.
--*/
{
#if 0              // don't use this it has too many races
    LockDThreadList();

    DEBUGF(DBG_TRACE, ("Removing thread %X From Process\n", OldThread));
    RemoveEntryList(&(OldThread->m_dprocess_linkage));

    UnLockDThreadList();
#endif
    return(0);
} //DThreadDetach



INT
DPROCESS::DProcessClassInitialize(
    IN VOID
    )
/*++

Routine Description:

    Performs  global  initialization for the DPROCESS class.  In particular, it
    creates  the  global  DPROCESS  object  and  stores  it  in a static member
    variable.

Arguments:

    None

Return Value:

    The  function  returns ERROR_SUCCESS if successful, otherwise it returns an
    appropriate WinSock error code.

--*/
{
    INT ReturnCode=WSAEFAULT;

    assert( sm_current_dprocess == NULL );

    sm_current_dprocess = new(DPROCESS);

    if (sm_current_dprocess) {
        ReturnCode = sm_current_dprocess->Initialize();
        if (ReturnCode != ERROR_SUCCESS) {
            DEBUGF( DBG_ERR,
                    ("Failed to Initialize dprocess object\n"));
            delete(sm_current_dprocess);
            sm_current_dprocess = NULL;
        }
    } //if
    else {
        DEBUGF( DBG_ERR,
                ("Failed to allocate dprocess object\n"));
        ReturnCode = WSA_NOT_ENOUGH_MEMORY;
    } //else
    return(ReturnCode);
} //DProcessClassInitialize




INT
DPROCESS::OpenAsyncHelperDevice(
    OUT LPHANDLE HelperHandle
    )
/*++

Routine Description:

    Retrieves  the  opened  Async  Helper  device  ID  required  for processing
    callbacks  in  the  overlapped  I/O  model.   The operation opens the Async
    Helper device if necessary.

Arguments:

    HelperHandle - Returns the requested Async Helper device ID.

Return Value:

    The  function  returns ERROR_SUCESS if successful, otherwise it
    returns an appropriate WinSock error code.
--*/
{
    INT ReturnCode;

    // Protect from multiple opens
    LockDThreadList ();
    // If the helper device has not been opened yet for this process
    // lets go out and open it.
    if (m_ApcHelper || (WahOpenApcHelper(&m_ApcHelper) == 0)) {
        *HelperHandle = m_ApcHelper;
        ReturnCode = ERROR_SUCCESS;
    } //if
    else {
        DEBUGF( DBG_ERR, ("Failed to open APC helper\n"));
        ReturnCode = WSASYSCALLFAILURE;
    } //else
    UnLockDThreadList ();
    return(ReturnCode);
} //OpenAsyncHelperDevice



INT
DPROCESS::OpenHandleHelperDevice(
    OUT LPHANDLE HelperHandle
    )
/*++

Routine Description:

    Retrieves  the  opened  Handle  Helper  device  ID  required  for allocation
    of socket handles for non-IFS providers.   The operation opens the Handle
    Helper device if necessary.

Arguments:

    HelperHandle - Returns the requested Handle Helper device ID.

Return Value:

    The  function  returns ERROR_SUCESS if successful, otherwise it
    returns an appropriate WinSock error code.
--*/
{
    INT ReturnCode;

    // Protect from double opens
    LockDThreadList ();

    // If the helper device has not been opened yet for this process
    // lets go out and open it.
    if (m_HandleHelper || (WahOpenHandleHelper(&m_HandleHelper) == 0)) {
        *HelperHandle = m_HandleHelper;
        ReturnCode = ERROR_SUCCESS;
    } //if
    else {
        DEBUGF( DBG_ERR, ("Failed to open Handle helper\n"));
        ReturnCode = WSASYSCALLFAILURE;
    } //else
    UnLockDThreadList ();
    return(ReturnCode);
} //GetHandleHelperDeviceID


INT
DPROCESS::OpenNotificationHelperDevice(
    OUT LPHANDLE HelperHandle
    )
/*++

Routine Description:

    Retrieves  the  opened  Notification  Helper  device  ID  required  for allocation
    of notification handles for catalog change notification. 
    The operation opens the Notification Helper device if necessary.

Arguments:

    HelperHandle - Returns the requested Notification Helper device ID.

Return Value:

    The  function  returns ERROR_SUCESS if successful, otherwise it
    returns an appropriate WinSock error code.
--*/
{
    INT ReturnCode;

    // Protect from double opens
    LockDThreadList ();

    // If the helper device has not been opened yet for this process
    // lets go out and open it.
    if (m_NotificationHelper || (WahOpenNotificationHandleHelper(&m_NotificationHelper) == 0)) {
        *HelperHandle = m_NotificationHelper;
        ReturnCode = ERROR_SUCCESS;
    } //if
    else {
        DEBUGF( DBG_ERR, ("Failed to open Notification helper\n"));
        ReturnCode = WSASYSCALLFAILURE;
    } //else
    UnLockDThreadList ();
    return(ReturnCode);
} //OpenNotificationHelperDevice


VOID
DPROCESS::SetVersion( WORD Version )
/*++

Routine Description:

    This function sets the WinSock version number for this process.

Arguments:

    Version - The WinSock version number.

Return Value:

    None.

--*/
{

    WORD newMajor;
    WORD newMinor;

    assert(Version != 0);

    newMajor = LOBYTE( Version );
    newMinor = HIBYTE( Version );

    //
    // If the version number is getting downgraded from a previous
    // setting, save the new (updated) number.
    //

    if( newMajor < GetMajorVersion() ||
        ( newMajor == GetMajorVersion() &&
          newMinor < GetMinorVersion() ) ) {

        m_version = Version;

    }

} // SetVersion


PDCATALOG
DPROCESS::GetProtocolCatalog()
/*++

Routine Description:
    Returns the protocol catalog associated with the process object.

Arguments:

    None

Return Value:

    The value of m_protocol_catalog
--*/
{
    if (HasCatalogChanged (m_proto_catalog_change_event))
        m_protocol_catalog->RefreshFromRegistry (m_proto_catalog_change_event);
    return(m_protocol_catalog);
}


PNSCATALOG
DPROCESS::GetNamespaceCatalog()
/*++

Routine Description:
    Returns the namespace catalog associated with the process object.

Arguments:

    None

Return Value:

    The value of m_namespace_catalog
--*/
{
    if (HasCatalogChanged (m_ns_catalog_change_event))
        m_namespace_catalog->RefreshFromRegistry (m_ns_catalog_change_event);
    return(m_namespace_catalog);
}


