/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

dsocket.cpp

Abstract:

This module contains the implemetation of the dsocket object used
by winsock2.dll

Author:

Dirk Brandewie  dirk@mink.intel.com  14-JUL-1995

Notes:

$Revision:   1.15  $

$Modtime:   08 Mar 1996 05:15:30  $

Revision History:
    21-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved single line functions to
        inlines in header file. Added debug/trace code.
--*/

#include "precomp.h"

#define m_reference_count   RefCount
#define m_socket_handle     Handle

LPCONTEXT_TABLE DSOCKET::sm_context_table=NULL;


INT
DSOCKET::DSocketClassInitialize(
    )
/*++
Routine Description:

    DSOCKET  class initializer.  This funtion must be called before any DSOCKET
    objects  are  created.   It  takes  care  of initializing the socket handle
    mapping table that maps socket handles to DSOCKET object references.

Arguments:

    None

Return Value:

    If  the function succeeds, it returns ERROR_SUCCESS, otherwise it returns a
    WinSock specific error code.
--*/
{
    if (sm_context_table==NULL) {
        return WahCreateHandleContextTable (&sm_context_table);
    }
    else
        return NO_ERROR;

} // DSocketClassInitialize




INT
DSOCKET::DSocketClassCleanup(
    )
/*++
Routine Description:

    DSOCKET  class  cleanup  function.   This function must be called after all
    DSOCKET  objects  have  been  destroyed.   It  takes care of destroying the
    socket  handle  mapping  table  that  maps socket handles to DSOCKET object
    references.

Arguments:

    None

Return Value:

    If  the function succeeds, it returns ERROR_SUCCESS, otherwise it returns a
    WinSock specific error code.
--*/
{
    DWORD   rc = NO_ERROR;
    if (sm_context_table!=NULL) {
        rc = WahDestroyHandleContextTable (sm_context_table);
        sm_context_table = NULL;
    }

    return rc;
} // DSocketClassCleanup




DSOCKET::DSOCKET(
    )
/*++

Routine Description:

    DSOCKET  object  constructor.   Creates and returns a DSOCKET object.  Note
    that  the  DSOCKET object has not been fully initialized.  The "Initialize"
    member function must be the first member function called on the new DSOCKET
    object.

Arguments:

    None

Return Value:

    None
--*/
{
    // Set our data members to known values
    m_reference_count   = 2;
    m_provider          = NULL;
    m_process           = NULL;
    m_handle_context    = NULL;
    m_socket_handle     = (HANDLE)INVALID_SOCKET;
    m_catalog_item      = NULL;
    m_pvd_socket        = FALSE;
    m_api_socket        = FALSE;
    m_overlapped_socket = TRUE;     // This is the default for socket call.
}




INT
DSOCKET::Initialize(
    IN PDPROCESS            Process,
    IN PPROTO_CATALOG_ITEM  CatalogEntry
    )
/*++

Routine Description:

    Completes  the  initialization  of  the  DSOCKET object.  This must be the
    first  member  function  called  for  the  DSOCKET object.  

Arguments:

    Process  - Supplies a reference to the DPROCESS object associated with this
               DSOCKET object.

    CatalogEntry - Supplies  a  reference  to  the catalog item object associated with
               this DSOCKET object.

Return Value:

    The  function returns ERROR_SUCCESS if successful.  Otherwise it
    returns an appropriate WinSock error code if the initialization
    cannot be completed.
--*/
{
    PDTHREAD    currentThread;

    // Store the provider, catalog, and process object.
    CatalogEntry->Reference ();
    m_catalog_item = CatalogEntry;
    m_provider = CatalogEntry->GetProvider ();
    m_process = Process;

    if (DTHREAD::GetCurrentDThread (Process, &currentThread)==NO_ERROR) {
        m_overlapped_socket = (currentThread->GetOpenType ()==0);
    }

    DEBUGF( DBG_TRACE,
            ("Initializing socket %X\n",this));
    return(ERROR_SUCCESS);
}



DSOCKET::~DSOCKET()
/*++

Routine Description:

    DSOCKET  object  destructor.   This  procedure  has  the  responsibility to
    perform  any required shutdown operations for the DSOCKET object before the
    object  memory  is  deallocated.  The caller is reponsible for removing the
    object  from its list in the DPROCESS object and removing the object/handle
    association  from  the  socket handle association manager before destroying
    the DSOCKET object.

Arguments:

    None

Return Value:

    None
--*/
{
    DEBUGF( DBG_TRACE,
            ("Destroying socket %X\n",this));
    if (m_catalog_item) {
        m_catalog_item->Dereference ();
        m_catalog_item = NULL;
    }
#ifdef DEBUG_TRACING
    {
        PDSOCKET Socket;
        if (sm_context_table!=NULL) {
            Socket = GetCountedDSocketFromSocketNoExport ((SOCKET)m_socket_handle);
            if (Socket!=NULL) {
                assert (Socket!=this);
                Socket->DropDSocketReference ();
            }
        }
    }
#endif
}

VOID
DSOCKET::DestroyDSocket()
/*++

Routine Description:

    Destroy DSocket object
Arguments:

    None

Return Value:

    None
--*/
{
    delete this;
}



INT
DSOCKET::AssociateSocketHandle(
    IN  SOCKET SocketHandle,
    IN  BOOLEAN ProviderSocket
    )
/*++
Routine Description:

    This  procedure  takes  the  socket  handle  that will be given to external
    clients  and  stores  it in the DSOCKET object.  It also enters this handle
    into  the  association table so that the client socket handle can be mapped
    to  a  DSOCKET  reference.  Note that this procedure must be called at some
    point for both IFS and non-IFS sockets.

Arguments:

    SocketHandle - Supplies  the  client  socket  handle  to  be  stored in and
                   associated with the DSOCKET object.
    ProviderSocket  - TRUE if socket is created by the provider

Return Value:

    If  the function succeeds, it returns ERROR_SUCCESS, otherwise it returns a
    WinSock specific error code.
--*/
{
    INT					return_code;
    PDSOCKET            oldSocket;

    return_code = ERROR_SUCCESS;
	m_socket_handle = (HANDLE)SocketHandle;
    m_pvd_socket = ProviderSocket;

    oldSocket = static_cast<PDSOCKET>(WahInsertHandleContext(
                                        sm_context_table,
                                        this));
    if (oldSocket==this) {
        //
        // We managed to insert new socket object into the table, done
        //
        ;
    }
    else if (oldSocket!=NULL) {
        PPROTO_CATALOG_ITEM oldItem;
        //
        // There was another socket object associated with the same
        // handle. This could happen in three cases:
        //  1) the socket was closed via CloseHandle and we never
        //      have had a chance to free it.
        //  2) the layered provider is reusing socket created by
        //      the base provider
        //  3) the socket was used by layered service provider
        //      which also never calls closesocket on it (just
        //      WSPCloseSocket which we never see -> yet another bug
        //      in the spec).
        // Of course, there could be a fourth case where the provider
        // gives us a bogus handle value, but we can't check for it
        // 
        // Wah call replaces the context in the table, so we just
        // need to dereference the old one, to account to refernce
        // we add when we create the object
        //

        oldSocket->DropDSocketReference();

    }
    else
        return_code = WSAENOBUFS;

    return return_code;
} // AssociateSocketHandle



INT
DSOCKET::DisassociateSocketHandle(
    )
/*++
Routine Description:

    This  procedure  removes  the (handle, DSOCKET) pair from the handle table.
    It also optionally destroys the handle.

Arguments:

    None
Return Value:

    None
--*/
{
    return WahRemoveHandleContext (sm_context_table, this);
}




PDSOCKET
DSOCKET::FindIFSSocket(
    IN  SOCKET     SocketHandle
    )
/*++
Routine Description

    This routine queries all IFS provider for the socket handle.
    If provider recognizes the socket, DSOCKET object for it
    is read from the table

Arguments:

    SocketHandle   - Supplies the client-level socket handle to be mapped.


Return Value:

    DSOCKET object or NULL in case the object cannot be found
--*/
{
    DWORD     flags;
    INT       result;
    PDPROCESS process = NULL;
    PDCATALOG catalog = NULL;
    PDSOCKET  temp_dsocket;

    //
    // Cannot find an association for the socket. Find the current
    // protocol catalog, and ask it to search the installed IFS providers
    // for one recognizing the socket.  Make sure the handle is valid
    //

    temp_dsocket = NULL;  // until proven otherwise

    if ( SocketHandle!=INVALID_SOCKET && // (NtCurrentProcess==(HANDLE)-1)
            GetHandleInformation( (HANDLE)SocketHandle, &flags ) ) {

        process = DPROCESS::GetCurrentDProcess();

        if( process!=NULL ) {

            catalog = process->GetProtocolCatalog();
            assert( catalog != NULL );

            result = catalog->FindIFSProviderForSocket( SocketHandle );

            if( result == ERROR_SUCCESS ) {
                //
                // One of the installed IFS providers recognized the socket.
                // Requery the context. If this fails, we'll just give up.
                //
                temp_dsocket = GetCountedDSocketFromSocketNoExport (SocketHandle);

                //
                // If we successed, mark socket as API socket because
                // we are going to return it from some API call.
                //
                if (temp_dsocket!=NULL)
                    temp_dsocket->m_api_socket = TRUE;
            }
        }
    }
   

    return(temp_dsocket);
} // FindIFSSocket


INT
DSOCKET::AddSpecialApiReference(
    IN SOCKET SocketHandle
    )
/*++

Routine Description:

    Mark socket so that we know that it was returned via API call to the
    application

Arguments:

    SocketHandle - The handle to reference.

Return Value:

    INT - 0 if successful, WinSock error code if not.

--*/
{
    PDSOCKET Socket;

    //
    // First, get a pointer to the (newly created) socket.
    // No need to do export in this case.
    //

    Socket = GetCountedDSocketFromSocketNoExport(
              SocketHandle
              );

    if( Socket!=NULL ) {
        // The app may start using handle before it event sees it
        // which causes as to import it from the provider and set
        // this flag.
        // At least one java test app does this.
        //assert (Socket->m_api_socket==FALSE);
        Socket->m_api_socket = TRUE;
        Socket->DropDSocketReference();
        return NO_ERROR;
    }
    else {
        //
        // This can only happen if we are being cleaned up
        //
        assert (DPROCESS::GetCurrentDProcess()==NULL);
        return WSASYSCALLFAILURE;
    }

} // AddSpecialApiReference
