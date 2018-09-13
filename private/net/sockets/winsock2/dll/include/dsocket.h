/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

dsocket.h

Abstract:

This  header  defines the "DSOCKET" class.  The DSOCKET class defines state
variables  and  operations for DSOCKET objects within the WinSock 2 DLL.  A
DSOCKET  object  represents  all  of the information that the WinSock 2 DLL
knows about a socket created using the Windows Sockets API.

Author:

Paul Drews (drewsxpa@ashland.intel.com) 30-June-1995

Notes:

$Revision:   1.12  $

$Modtime:   08 Mar 1996 00:07:38  $

Revision History:

most-recent-revision-date email-name
description

07-14-1995  dirk@mink.intel.com
    Moved member function descriptions to the implementation file
    dsocket.cpp

07-09-1995  drewsxpa@ashland.intel.com
    Completed  first  complete  version with clean compile and released for
    subsequent implementation.

07-08-95  drewsxpa@ashland.intel.com
Original version

--*/

#ifndef _DSOCKET_
#define _DSOCKET_

#include "winsock2.h"
#include <windows.h>
#include "llist.h"
#include "classfwd.h"
#include "ws2help.h"

class DSOCKET: private WSHANDLE_CONTEXT
{
  public:

    static
    INT
    DSocketClassInitialize();

    static
    INT
    DSocketClassCleanup();

    static
    INT
    AddSpecialApiReference(
        IN SOCKET SocketHandle
        );

    static
    PDSOCKET
    GetCountedDSocketFromSocket(
        IN  SOCKET     SocketHandle
        );

    static
    PDSOCKET
    GetCountedDSocketFromSocketNoExport(
        IN  SOCKET     SocketHandle
        );

    DSOCKET();

    INT
    Initialize(
        IN PDPROCESS            Process,
        IN PPROTO_CATALOG_ITEM  CatalogEntry
        );

    ~DSOCKET();

    SOCKET
    GetSocketHandle();

    PDPROVIDER
    GetDProvider();

    PDPROCESS
    GetDProcess();

    DWORD_PTR
    GetContext();

    VOID
    SetContext(
        IN DWORD_PTR Context
        );

    PPROTO_CATALOG_ITEM
    GetCatalogItem();

    INT
    AssociateSocketHandle(
        IN  SOCKET  SocketHandle,
        IN  BOOLEAN ProviderSocket
        );

    INT
    DisassociateSocketHandle( );

    VOID
    AddDSocketReference(
        );

    VOID
    DropDSocketReference(
        );

    VOID
    DestroyDSocket(
        );

    BOOL
    IsProviderSocket(
        );

    BOOL
    IsApiSocket (
        );

    BOOL
    IsOverlappedSocket (
        );

#ifndef WS2_DEBUGGER_EXTENSION
//
// Give debugger extension access to all fields
//
  private:
#endif

    static
    PDSOCKET
    FindIFSSocket (
        IN  SOCKET     SocketHandle
        );

    friend class DPROCESS;

    static LPCONTEXT_TABLE  sm_context_table;
    // Context tables

//#define m_reference_count   RefCount
//#define m_socket_handle     Handle
    DWORD_PTR   m_handle_context;
    // The  uninterpreted  socket  handle  context  value  that  was  set  by
    // SetContext at the time of WPUCreateSocketHandle.

    PDPROVIDER  m_provider;
    // Reference  to  the  DPROVIDER object representing the service provider
    // that controls this socket.

    PDPROCESS  m_process;
    // Reference to the DPROCESS object with which this socket is associated.

    PPROTO_CATALOG_ITEM m_catalog_item;
    // The protocol catalog item used to create this socket

    BOOLEAN m_pvd_socket;
    // TRUE if this socket comes from the provider and not created by
    // helper DLL on provider request (WPUCreateSocketHandle).

    BOOLEAN m_api_socket;
    // TRUE if socket was returned via socket/WSASocketA/WSASocketW call,
    //      or imported from IFS provider in other API calls
    // FALSE for sockets that are used only at SPI level (created on
    //      request from layered provider and never exposed to the
    //      application)

    BOOLEAN m_overlapped_socket;
    // To support the hack of creations of non-overlapped handles
    // during Accept and JoinLeaf by handle helper.

#if defined(DEBUG_TRACING) || defined(WS2_DEBUGGER_EXTENSION)
#define SOCKET_STACK_BACKTRACE_DEPTH 2
  public:
	PVOID	m_CreatorBackTrace[SOCKET_STACK_BACKTRACE_DEPTH];
    // Socket creator information
#endif


};   // class DSOCKET



inline SOCKET
DSOCKET::GetSocketHandle()
/*++

Routine Description:

    Retrieves  the  external socket-handle value corresponding to this internal
    DSOCKET object.

Arguments:

    None

Return Value:

    The corresponding external socket-handle value.
--*/
{
    return((SOCKET)Handle);
}




inline PDPROVIDER
DSOCKET::GetDProvider()
/*++

Routine Description:

    Retrieves  a reference to the DPROVIDER object associated with this DSOCKET
    object.

Arguments:

    None

Return Value:

    The reference to the DPROVIDER object associated with this DSOCKET object.
--*/
{
    return(m_provider);
}




inline PDPROCESS
DSOCKET::GetDProcess()
/*++

Routine Description:

    Retrieves  a  reference to the DPROCESS object associated with this DSOCKET
    object.

Arguments:

    None

Return Value:

    The reference to the DPROCESS object associated with this DSOCKET object.
--*/
{
    return(m_process);
}




inline DWORD_PTR
DSOCKET::GetContext()
/*++

Routine Description:

    This  function  retrieves  the  socket  handle  context  value set with the
    SetContext  operation.   This  function  is typically called at the time of
    WPUQuerySocketHandleContext.  The return value is unspecified if SetContext
    has not been called.

Arguments:

    None

Return Value:

    Returns  the  context  value  that  was  set  by SetContext.  This value is
    uninterpreted by the WinSock 2 DLL.
--*/
{
    return(m_handle_context);
}




inline VOID
DSOCKET::SetContext(
    IN  DWORD_PTR Context
    )
/*++

Routine Description:

    This  function  sets  the  socket  handle  context value.  This function is
    typically called at the time of WPUCreateSocketHandle.

Arguments:

    lpContext - Supplies  the  uninterpreted  socket handle context value to be
                associated with this socket.

Return Value:

    None
--*/
{
    m_handle_context = Context;
}



inline PPROTO_CATALOG_ITEM
DSOCKET::GetCatalogItem()
/*++

Routine Description:

    Retreives the pointer to the catalog item associated with this socket.

Arguments:

Return Value:

    The pointer to the catalog item associated with this socket.
--*/
{
    return(m_catalog_item);
}






inline
BOOL
DSOCKET::IsProviderSocket(
    )
/*++
Routine Description:

    This function returns a boolean indicating whether the object is
    for the socket created by the provider (presumably IFS).

Arguments:

    None

Return Value:

    TRUE  - The object is for the socket created by the provider.

    FALSE - The object is for the socket created by helper DLL.
--*/
{
    return m_pvd_socket;

} // IsProviderSocket



inline
BOOL
DSOCKET::IsApiSocket(
    )
/*++
Routine Description:

    This function returns a boolean indicating whether the object represents
    socket used by API client.

Arguments:

    None

Return Value:

    TRUE  - The socket is used by API client .

    FALSE - The object is used by SPI client.
--*/
{
    return m_api_socket;

} // IsApiSocket


inline
BOOL
DSOCKET::IsOverlappedSocket(
    )
/*++
Routine Description:

    This function returns a boolean indicating whether the object represents
    overlapped socket.

Arguments:

    None

Return Value:

    TRUE  - The socket is overlapped.

    FALSE - The object is non-overlapped.
--*/
{
    return m_overlapped_socket;

} // IsOverlappedSocket





inline VOID
DSOCKET::AddDSocketReference(
    )
/*++
Routine Description:

    Adds a reference to the DSOCKET.

Arguments:

    None

Return Value:

    None
--*/
{

    WahReferenceHandleContext(this);

} // AddDSocketReference


inline VOID
DSOCKET::DropDSocketReference(
    )
/*++
Routine Description:

    Drops the DSOCKET reference and destroys the object
    if reference count is 0.

Arguments:

    None

Return Value:

    None
--*/
{

    if (WahDereferenceHandleContext(this)==0)
        DestroyDSocket ();
} // DropDSocketReference

inline
PDSOCKET
DSOCKET::GetCountedDSocketFromSocket(
    IN  SOCKET     SocketHandle
    )
/*++
Routine Description

    This procedure takes a client socket handle and maps it to a DSOCKET object
    reference.  The reference is counted.

    If socket object corresponding to the handle cannot be found in the table
    this function queires all IFS providers to see if one of the recognizes
    the handle.

    Whenever  this procedure successfuly returns a counted reference, it is the
    responsibility of the caller to eventually call DropDSocketReference.

    Note  that  this  procedure  assumes that the caller has already checked to
    make sure that WinSock is initialized.

Arguments:

    SocketHandle   - Supplies the client-level socket handle to be mapped.


Return Value:
    DSOCKET object or NULL in case the object cannot be found
--*/
{
    PDSOCKET    Socket;
    Socket = static_cast<PDSOCKET>(WahReferenceContextByHandle (
                                        sm_context_table,
                                        (HANDLE)SocketHandle));
    if (Socket!=NULL)
        return Socket;
    else
        return FindIFSSocket (SocketHandle);
}

inline
PDSOCKET
DSOCKET::GetCountedDSocketFromSocketNoExport(
    IN  SOCKET     SocketHandle
    )
/*++
Routine Description

    This procedure takes a client socket handle and maps it to a DSOCKET object
    reference.  The reference is counted.

    No attempt is made to find IFS provider for the socket if it cannot be found
    in the table.  This function is intented for calls from non-IFS providers
    such as in context of WPUQuerySocketHandleContext.

    Whenever  this procedure successfuly returns a counted reference, it is the
    responsibility of the caller to eventually call DropDSocketReference.

    Note  that  this  procedure  assumes that the caller has already checked to
    make sure that WinSock is initialized.

Arguments:

    SocketHandle   - Supplies the client-level socket handle to be mapped.


Return Value:
    DSOCKET object or NULL in case the object cannot be found
--*/
{
    return static_cast<PDSOCKET>(WahReferenceContextByHandle (
                                    sm_context_table,
                                    (HANDLE)SocketHandle));
}

#endif // _DSOCKET_
