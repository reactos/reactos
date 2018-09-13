/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    socklife.c

Abstract:

    This module contains the Winsock API functions concerned with socket
    lifetime. The following API functions are contained in this module.

    socket()
    WSASocketA()
    WSASocketW()
    accept()
    WSAAccept()
    WPUCreateSocketHandle()
    WPUCloseSocketHandle
    WPUQuerySocketHandleContext
    WPUModifyIFSHandle
    WSAJoinLeaf()
    closesocket()

Author:

    dirk@mink.intel.com  14-06-1995

Revision History:

    22-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved includes to precomp.h. Added
        asserts for debugging
--*/

#include "precomp.h"
#pragma hdrstop
#include <wsipx.h>
#include <wsnwlink.h>
#include <atalkwsh.h>

#define NSPROTO_MAX (NSPROTO_IPX + 255)


SOCKET WSAAPI
socket(
    IN int af,
    IN int type,
    IN int protocol)
/*++
Routine Description:

     Create a socket which is bound to a specific service provider.

Arguments:
    af - An address family specification.  The
         only format currently supported is
         PF_INET, which is the ARPA Internet
         address format.

    type - A type specification for the new socket.

    protocol- A particular protocol to be used with
              the socket, or 0 if the caller does not
              wish to specify a protocol.

Returns:
    A socket descriptor referencing the new socket. Otherwise, a value
    of INVALID_SOCKET is returned and the error code is stored with
    SetErrorCode.
--*/
{
    PDPROCESS Process;
    PDTHREAD  Thread;
    INT       ErrorCode;
    DWORD     dwFlags;

    ErrorCode = PROLOG(
        &Process,
        &Thread);

    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(INVALID_SOCKET);
    } //if

    if( Thread->GetOpenType() == 0 ) {
        dwFlags = WSA_FLAG_OVERLAPPED;
    } else {
        dwFlags = 0;
    }

    //
    // HACK for NetBIOS!
    //

    if( af == AF_NETBIOS && protocol > 0 ) {
        protocol *= -1;
    }

    return(WSASocketW(
        af,
        type,
        protocol,
        NULL,      // lpProtocolInfo
        0,         // g
        dwFlags));
}

SOCKET WSAAPI
WSASocketW (
    IN int af,
    IN int type,
    IN int protocol,
    IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN GROUP g,
    IN DWORD dwFlags)
/*++
Routine Description:

    Create  a  socket  which is bound to a specific transport service provider,
    optionally create and/or join a socket group.

Arguments:

    af             - An   address   family   specification.   The  only  format
                     currently supported is PF_INET, which is the ARPA Internet
                     address format.

    type           - A type specification for the new socket.

    protocol       - A  particular protocol to be used with the socket, or 0 if
                     the caller does not wish to specify a protocol.

    lpProtocolInfo - A pointer to a WSAPROTOCOL_INFOW struct that defines the
                     characteristics  of  the  socket  to  be created.  If this
                     parameter  is  not  NULL,  the first three parameters (af,
                     type, protocol) are ignored.

     g             - The identifier of the socket group.

     dwFlags       - The socket attribute specification.


Returns:

    A  socket  descriptor  referencing  the  new socket.  Otherwise, a value of
    INVALID_SOCKET is returned and the error code is stored with SetErrorCode.
--*/
{
    SOCKET              ReturnValue;
    PDPROCESS           Process;
    PDTHREAD            CurrentThread;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDCATALOG           Catalog;
    PPROTO_CATALOG_ITEM CatalogEntry;
    LPWSAPROTOCOL_INFOW ProtocolInfo;
    DWORD               dwCatalogId;

    ErrorCode = PROLOG(
        &Process,
        &CurrentThread);

    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(INVALID_SOCKET);
        } //if

    //Set Default return code
    ReturnValue = INVALID_SOCKET;

    // Find a provider that can support the user request
    Catalog = Process->GetProtocolCatalog();

    if (lpProtocolInfo) {

        __try {
            dwCatalogId =  lpProtocolInfo->dwCatalogEntryId;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            SetLastError(WSAEFAULT);
            return(INVALID_SOCKET);
        }
        ErrorCode =  Catalog->GetCountedCatalogItemFromCatalogEntryId(
            dwCatalogId,
            &CatalogEntry);
    } //if
    else {
        dwCatalogId = 0;

RestartCatalogLookupHack:

        ErrorCode = Catalog->GetCountedCatalogItemFromAttributes(
            af,
            type,
            protocol,
            dwCatalogId,
            &CatalogEntry
            );

        //
        // If we failed to find a provider, try to reload the catalog
        // from the registry and retry the lookup. This handles the
        // case (first noticed in CAIRO SETUP) where WS2_32.DLL is loaded
        // and WSAStartup() is called *before* CAIRO SETUP has had the
        // opportunity to install the necessary providers. Later, CAIRO
        // SETUP needs to create sockets.
        //
        // Do not need this anymore, we now support dynamic notifications
        // of protocol installation/removal and check for changes every
        // time we access the catalog.
        //

    } // else choosing from attributes

    if ( ERROR_SUCCESS == ErrorCode) {

        Provider = CatalogEntry->GetProvider();
        if (lpProtocolInfo) {
            // Must  be  sure  we  use  the  client's lpProtocolInfo if one was
            // supplied, to support the WSADuplicateSocket model.
            ProtocolInfo = lpProtocolInfo;
        } //if
        else {
            ProtocolInfo = CatalogEntry->GetProtocolInfo();
        } //else

        assert(ProtocolInfo != NULL);

        //
        // Hack-O-Rama. Temporary set the open type of the thread
        // depending on the overlapped flag so that we can create
        // appropriate socket handle for the layered service
        // provider.  However, if layered service provide caches
        // the handles we create for it, not much we can do.
        //
        {
            INT oldOpenType = CurrentThread->GetOpenType ();
            CurrentThread->SetOpenType ((dwFlags & WSA_FLAG_OVERLAPPED) ? 0 : SO_SYNCHRONOUS_NONALERT);

            // Now we have a provider that can support the user
            // request lets ask get a socket
            ReturnValue = Provider->WSPSocket(
                af,
                type,
                protocol,
                ProtocolInfo,
                g,
                dwFlags,
                &ErrorCode);

            //
            // Reset saved open type
            //
            CurrentThread->SetOpenType (oldOpenType);
        }

        //
        // Hack-O-Rama. If WSPSocket() failed with the distinguished
        // error code WSAEINPROGRESS *and* this was not a request for a
        // specific provider (i.e. lpProtocolInfo == NULL) then
        // restart the catalog lookup starting at the current item
        // (the current catalog id).
        //

        //
        // Snap the catalog id and dereference the catalog entry
        //
        dwCatalogId = ProtocolInfo->dwCatalogEntryId;
        CatalogEntry->Dereference ();

        if( ReturnValue == INVALID_SOCKET &&
                ErrorCode == WSAEINPROGRESS &&
                lpProtocolInfo == NULL ) {
            goto RestartCatalogLookupHack;
        }

        if( ReturnValue != INVALID_SOCKET ) {
            DSOCKET::AddSpecialApiReference( ReturnValue );
            return ReturnValue;
        }
    } //if

    assert (ErrorCode!=ERROR_SUCCESS);
    // There was an error, set this threads lasterror
    SetLastError(ErrorCode);
    return(INVALID_SOCKET);
}


SOCKET WSAAPI
WSASocketA (
    IN int af,
    IN int type,
    IN int protocol,
    IN LPWSAPROTOCOL_INFOA lpProtocolInfo,
    IN GROUP g,
    IN DWORD dwFlags)
/*++

Routine Description:

    ANSI thunk to WSASocketW.

Arguments:

    af             - An   address   family   specification.   The  only  format
                     currently supported is PF_INET, which is the ARPA Internet
                     address format.

    type           - A type specification for the new socket.

    protocol       - A  particular protocol to be used with the socket, or 0 if
                     the caller does not wish to specify a protocol.

    lpProtocolInfo - A pointer to a WSAPROTOCOL_INFOA struct that defines the
                     characteristics  of  the  socket  to  be created.  If this
                     parameter  is  not  NULL,  the first three parameters (af,
                     type, protocol) are ignored.

     g             - The identifier of the socket group.

     dwFlags       - The socket attribute specification.


Returns:

    A  socket  descriptor  referencing  the  new socket.  Otherwise, a value of
    INVALID_SOCKET is returned and the error code is stored with SetErrorCode.
--*/
{

    INT                 error;
    WSAPROTOCOL_INFOW   ProtocolInfoW;

    //
    // Map the ANSI WSAPROTOCOL_INFOA structure to UNICODE.
    //

    if( lpProtocolInfo != NULL ) {

        error = MapAnsiProtocolInfoToUnicode(
                    lpProtocolInfo,
                    &ProtocolInfoW
                    );

        if( error != ERROR_SUCCESS ) {

            SetLastError( error );
            return INVALID_SOCKET;

        }

    }

    //
    // Call through to the UNICODE version.
    //

    return WSASocketW(
               af,
               type,
               protocol,
               lpProtocolInfo
                    ? &ProtocolInfoW
                    : NULL,
               g,
               dwFlags
               );

}   // WSASocketA




SOCKET WSAAPI
accept(
    IN SOCKET s,
    OUT struct sockaddr FAR *addr,
    OUT int FAR *addrlen
    )
/*++
Routine Description:

    Accept a connection on a socket.

Arguments:

    s - A descriptor identifying a socket which is listening for connections
        after a listen().

    addr - An optional pointer to a buffer which receives the address of the
           connecting entity, as known to the communications layer.  The exact
           format of the addr argument is determined by the address family
           established when the socket was  created.

    addrlen - An optional pointer to an integer which contains the length of
              the address addr.

Returns:
    A descriptor for the accepted socket. Otherwise, a value of INVALID_SOCKET
    is returned and the error code is stored with SetErrorCode.
--*/
{
    return(WSAAccept(
        s,
        addr,
        addrlen,
        NULL,   // No condition function
        NULL)); //No callback data
}





SOCKET WSAAPI
WSAAccept(
    IN SOCKET s,
    OUT struct sockaddr FAR *addr,
    OUT LPINT addrlen,
    IN LPCONDITIONPROC lpfnCondition,
    IN DWORD_PTR dwCallbackData
    )
/*++
Routine Description:

     Conditionally accept a connection based on the return value of a
     condition function, and optionally create and/or join a socket
     group.

Arguments:

    s - A descriptor identifying a socket which is listening for connections
        after a listen().

    addr - An optional pointer to a buffer which receives the address of the
           connecting entity, as known to the communications layer. The exact
           format of the addr argument is determined by the address family
           established when the socket was  created.

    addrlen - An optional pointer to an integer which contains the length of
              the address addr.

    lpfnCondition - The procedure instance address of the optional,
                    application-supplied condition function which will make an
                    accept/reject decision based on the caller information
                    passed in as parameters, and optionally create and/or join
                    a socket group by assigning an appropriate value to the
                    result parameter g of this function.


    dwCallbackData - The callback data passed back to the application as a
                     condition function parameter.  This parameter is not
                     interpreted by WinSock.
Returns:
    A socket descriptor for the newly accepted socket on success, otherwise
    INVALID_SOCKET.
--*/
{
    SOCKET             ReturnValue;
    INT                ErrorCode;
    PDSOCKET           Socket;
    PDPROVIDER         Provider;
    PDPROCESS          Process;
    PDTHREAD           CurrentThread;

    ErrorCode = PROLOG(
        &Process,
        &CurrentThread);

    if (ErrorCode == ERROR_SUCCESS) {

		Socket = DSOCKET::GetCountedDSocketFromSocket(s);
		if(Socket != NULL){
            //
            // Hack-O-Rama. Temporary set the open type of the thread
            // depending on the overlapped flag so that we can create
            // appropriate socket handle for the layered service
            // provider.  However, if layered service provide caches
            // the handles we create for it, not much we can do.
            //
            INT oldOpenType = CurrentThread->GetOpenType ();
            CurrentThread->SetOpenType (Socket->IsOverlappedSocket() ? 0 : SO_SYNCHRONOUS_NONALERT);

			Provider = Socket->GetDProvider();
			ReturnValue = Provider->WSPAccept(
				s,
				addr,
				addrlen,
				lpfnCondition,
				dwCallbackData,
				&ErrorCode);

            //
            // Reset saved open type
            //
            CurrentThread->SetOpenType (oldOpenType);

			Socket->DropDSocketReference();
            if( ReturnValue != INVALID_SOCKET) {
				//
				// Add a reference if the socket we got back was different
				// that the one we passed in (just in case someone
                // implements it this way which is not explicitly
                // prohibited by the specification).
				//

                if (ReturnValue != s ) {
				    DSOCKET::AddSpecialApiReference( ReturnValue );
                }
				return ReturnValue;
			}

		} //if
		else {
			ErrorCode = WSAENOTSOCK;
		}
	}

    SetLastError(ErrorCode);
    return(INVALID_SOCKET);
}




SOCKET WSPAPI
WPUCreateSocketHandle(
    IN DWORD dwCatalogEntryId,
    IN DWORD_PTR lpContext,
    OUT LPINT lpErrno )
/*++
Routine Description:

    Creates a new socket handle.

Arguments:

    dwCatalogEntryId - Indentifies the calling service provider.

    lpContext - A context value to associate with the new socket handle.

    lpErrno - A pointer to the error code.

Returns:
    A socket handle if successful, otherwise INVALID_SOCKET.
--*/
{
    SOCKET              ReturnCode=INVALID_SOCKET;
    INT                 ErrorCode=ERROR_SUCCESS;
    PDPROCESS           Process;
    PDCATALOG           Catalog;
    SOCKET              SocketID;
    HANDLE              HelperHandle;


    Process = DPROCESS::GetCurrentDProcess();
    if (Process!=NULL) {
        Catalog = Process->GetProtocolCatalog();
        if (Catalog)
        {
            PPROTO_CATALOG_ITEM CatalogEntry;

            ErrorCode = Catalog->GetCountedCatalogItemFromCatalogEntryId(
                dwCatalogEntryId,
                &CatalogEntry);

            if (ERROR_SUCCESS == ErrorCode) {
#if DBG
                if (CatalogEntry->GetProtocolInfo()->dwServiceFlags1 & XP1_IFS_HANDLES) {
                    DEBUGF(DBG_WARN,("IFS provider %ls asking for non-IFS socket handle\n",
                                        CatalogEntry->GetProtocolInfo()->szProtocol));
                }
#endif
                ErrorCode = Process->GetHandleHelperDeviceID (&HelperHandle);
                if (ErrorCode == ERROR_SUCCESS) {
                    ErrorCode = WahCreateSocketHandle (HelperHandle, &SocketID);
                    if (ErrorCode == ERROR_SUCCESS) {
                        PDSOCKET            Socket;
                        // Alloc new DSocket object
                        Socket = new(DSOCKET);
                        if (Socket) {
                            // Init the new socket
                            Socket->Initialize(
                                Process,
                                CatalogEntry);

                            // Add Socket into the handle table allocated.
                            ErrorCode = Socket->AssociateSocketHandle(
                                    SocketID, // Socket handler
                                    FALSE);   // ProviderSocket
                            if (ErrorCode == ERROR_SUCCESS) {
                                //Finish putting the socket together
                                Socket->SetContext(lpContext);
                                ReturnCode = SocketID;
                            }
                            else {
                                WahCloseSocketHandle (HelperHandle, SocketID);
                                Socket->DropDSocketReference ();
                            }

                            Socket->DropDSocketReference ();
                        } // if socket was allocated
                        else {
                            WahCloseSocketHandle (HelperHandle, SocketID);
                            ErrorCode = WSAENOBUFS;
                        }
                    }
                } // Helper device was loaded OK

                CatalogEntry->Dereference ();
            } //if catalog entry found
            else
            {
                DEBUGF(DBG_ERR,("Failed to find catalog entry for provider %ld\n",
                                    dwCatalogEntryId));

            } //else
        } //if catalog is there
        else {
            ErrorCode = WSANOTINITIALISED;
        }
    } //if process is initialized
    else {
        ErrorCode = WSANOTINITIALISED;
    }

    *lpErrno = ErrorCode;
    return(ReturnCode);

} // WPUCreateSocketHandle


int WSPAPI
WPUCloseSocketHandle(
    IN SOCKET s,
    OUT LPINT lpErrno
    )
/*++
Routine Description:

    Closes an exsisting socket handle.

Arguments:

    s       - Identifies a socket handle created with WPUCreateSocketHandle().

    lpErrno - A pointer to the error code.

Returns:

    Zero on success else SOCKET_ERROR.
--*/
{
    INT                 ReturnValue = ERROR_SUCCESS;
    PDPROCESS           Process;
    INT                 ErrorCode;
    PDSOCKET            Socket;
    HANDLE              HandleHelper;

    assert(lpErrno);

	Process = DPROCESS::GetCurrentDProcess ();
	if (Process!=NULL) {


		//
		// We use the no-export version because there is no way such handle
		// can be exported anyway.
		//

		Socket = DSOCKET::GetCountedDSocketFromSocketNoExport(s);
		if (Socket!=NULL) {
			if (!Socket->IsProviderSocket()) {
				ErrorCode = Socket->DisassociateSocketHandle();
                assert (ErrorCode == ERROR_SUCCESS);
				ErrorCode = Process->GetHandleHelperDeviceID(&HandleHelper);
				if (ErrorCode==ERROR_SUCCESS) {
					ErrorCode = WahCloseSocketHandle (HandleHelper, Socket->GetSocketHandle ());
					if (ErrorCode!=ERROR_SUCCESS) {
						ReturnValue = SOCKET_ERROR;
					}
				}
				else {
					*lpErrno = ErrorCode;
					ReturnValue = SOCKET_ERROR;
				}
				//
				// Drop active reference.  IFS socket's active reference
				// is dropped in closesocket routine.
				//
				Socket->DropDSocketReference();
			}
			else {
				DEBUGF(
					DBG_ERR,
					("Foreign socket handle handed in by service provider for closure\n"));
				*lpErrno = WSAEINVAL;
				ReturnValue = SOCKET_ERROR;
			}
			Socket->DropDSocketReference();
		} // if ERROR_SUCCESS
		else {
			DEBUGF(
				DBG_ERR,
				("Bad socket handle handed in by service provider for closure\n"));
			*lpErrno = WSAENOTSOCK;
			ReturnValue = SOCKET_ERROR;
		}
	}
	else {
		*lpErrno = WSANOTINITIALISED;
		ReturnValue = SOCKET_ERROR;
	}

    return (ReturnValue);
}


int WSPAPI
WPUQuerySocketHandleContext(
    IN SOCKET s,
    OUT PDWORD_PTR lpContext,
    OUT LPINT lpErrno
    )
/*++
Routine Description:

    Queries the context value associated with the specified socket handle.

Arguments:

    s         - Identifies the socket whose context is to be queried.

    lpContext - A pointer to an DWORD that will receive the context value.

    lpErrno   - A pointer to the error code.

Returns:

    If  no error occurs, WPUQuerySocketHandleContext() returns 0 and stores the
    current  context  value  in lpdwContext.  Otherwise, it returns
    SOCKET_ERROR, and a specific error code is available in lpErrno.
--*/
{
    INT ReturnCode=SOCKET_ERROR;
    INT ErrorCode=WSAENOTSOCK;
    PDSOCKET Socket;

    //
    // We use the no-export version because there is no way such handle
    // can be exported anyway.
    //
    Socket = DSOCKET::GetCountedDSocketFromSocketNoExport(s);
    if (Socket!=NULL) {
        if (!Socket->IsProviderSocket()) {
            *lpContext = Socket->GetContext();
            ReturnCode = ERROR_SUCCESS;
        }
        else {
            DEBUGF(
                DBG_ERR,
                ("Foreign socket handle handed in by service provider for query\n"));
            *lpErrno = WSAEINVAL;
        }
        Socket->DropDSocketReference ();
    }
    else {
        DEBUGF(
            DBG_ERR,
            ("Bad socket handle handed in by service provider for query\n"));
        *lpErrno = ErrorCode;
    }
    return(ReturnCode);
}



SOCKET WSPAPI
WPUModifyIFSHandle(
    IN DWORD dwCatalogEntryId,
    IN SOCKET ProposedHandle,
    OUT LPINT lpErrno
    )
/*++
Routine Description:

    Receive (possibly) modifies IFS handle from winsock DLL.

Arguments:

    dwCatalogEntryId - Identifies the calling service provider.

    ProposedHandle   - An  Installable File System(IFS) handle allocated by the
                       provider.

    lpErrno          - A pointer to the error code.

Returns:

    If  no  error  occurs,  WPUModifyIFSHandle()  returns  the  modified socket
    handle.  Otherwise, it returns INVALID_SOCKET, and a specific error code is
    available in lpErrno.
--*/
{
    SOCKET              ReturnCode=INVALID_SOCKET;
    INT                 ErrorCode=ERROR_SUCCESS;
    PDPROCESS           Process;
    PDCATALOG           Catalog;


    // Alloc new DSocket object
    Process = DPROCESS::GetCurrentDProcess();
    if (Process!=NULL) {
        Catalog = Process->GetProtocolCatalog();
        if (Catalog) {
            PPROTO_CATALOG_ITEM CatalogEntry;

            ErrorCode = Catalog->GetCountedCatalogItemFromCatalogEntryId(
                dwCatalogEntryId,
                &CatalogEntry);

            if (ERROR_SUCCESS == ErrorCode) {
                PDSOCKET            Socket;

                // Create new socket object
                Socket = new DSOCKET;

                if (Socket!=NULL) {
                    // Init the new socket
                    Socket->Initialize(
                        Process,
                        CatalogEntry);

                    //
                    // Add socket to the handle table.  In this implementation, we
                    // wind up never changing the proposed IFS handle.
                    //

                    ErrorCode = Socket->AssociateSocketHandle(
                        ProposedHandle, // SocketHandle
                        TRUE);           // ProviderSocket

                    if (ErrorCode == ERROR_SUCCESS) {

                        ReturnCode = ProposedHandle;
                        *lpErrno = ERROR_SUCCESS;
                    } //if
                    else {
                        //
                        // Failed to insert into the table
                        // Remove creation reference
                        //
                        Socket->DropDSocketReference ();
                        *lpErrno = ErrorCode;
                    }

                    //
                    // Note that the new DSOCKET starts out with a ref count
                    // of two, so we'll always need to dereference it once.
                    //
                    Socket->DropDSocketReference ();

                } // if socket was created or found
                else {
                    *lpErrno = WSAENOBUFS;
                }
                CatalogEntry->Dereference ();
            } //if catalog entry was found
            else {
                DEBUGF(DBG_ERR,("Failed to find catalog entry for provider %ld\n",
                                    dwCatalogEntryId));
                *lpErrno = ErrorCode;
            }
        } //if catalog is there
        else
        {
            DEBUGF(DBG_ERR,("Failed to find Catalog object"));
            *lpErrno = WSANOTINITIALISED;

        } //else
    } //if process is initialized
    else {
        *lpErrno = WSANOTINITIALISED;
    }

    return(ReturnCode);

}  // WPUModifyIfsHandle




SOCKET WSAAPI
WSAJoinLeaf(
    IN SOCKET s,
    IN const struct sockaddr FAR * name,
    IN int namelen,
    IN LPWSABUF lpCallerData,
    OUT LPWSABUF lpCalleeData,
    IN LPQOS lpSQOS,
    IN LPQOS lpGQOS,
    IN DWORD dwFlags
    )
/*++
Routine Description:

    Join  a  leaf  node  into  a multipoint session, exchange connect data, and
    specify needed quality of service based on the supplied flow specs.

Arguments:

    s            - A descriptor identifying an multipoint socket.

    name         - The name of the peer to which the socket is to be joined.

    namelen      - The length of the name.

    lpCallerData - A  pointer to the user data that is to be transferred to the
                   peer during multipoint session establishment.

    lpCalleeData - pointer to the user data that is to be transferred back from
                   the peer during multipoint session establishment.

    lpSQOS       - A  pointer  to  the  flow  specs  for socket s, one for each
                   direction.

    lpGQOS       - A  pointer  to  the  flow  specs  for  the  socket group (if
                   applicable).

    dwFlags      - Flags  to  indicate the socket acting as a sender, receiver,
                   or both.

Returns:
    If no error occurs, WSAJoinLeaf() returns a value of type SOCKET which is a
    descriptor  for the newly created multipoint socket.  Otherwise, a value of
    INVALID_SOCKET  is  returned, and a specific error code may be retrieved by
    calling WSAGetLastError().

--*/
{

    SOCKET             ReturnValue;
    INT                ErrorCode;
    PDPROVIDER         Provider;
    PDSOCKET           Socket;
    PDPROCESS          Process;
    PDTHREAD           CurrentThread;

    ErrorCode = PROLOG(
        &Process,
        &CurrentThread);

    if (ErrorCode == ERROR_SUCCESS) {

		Socket = DSOCKET::GetCountedDSocketFromSocket(s);
		if(Socket != NULL){
            //
            // Hack-O-Rama. Temporary set the open type of the thread
            // depending on the overlapped flag so that we can create
            // appropriate socket handle for the layered service
            // provider.  However, if layered service provide caches
            // the handles we create for it, not much we can do.
            //
            INT oldOpenType = CurrentThread->GetOpenType ();
            CurrentThread->SetOpenType (Socket->IsOverlappedSocket() ? 0 : SO_SYNCHRONOUS_NONALERT);

            Provider = Socket->GetDProvider();
			ReturnValue = Provider->WSPJoinLeaf(
				s,
				name,
				namelen,
				lpCallerData,
				lpCalleeData,
				lpSQOS,
				lpGQOS,
				dwFlags,
				&ErrorCode);

            //
            // Reset saved open type
            //
            CurrentThread->SetOpenType (oldOpenType);

            Socket->DropDSocketReference();


			if( ReturnValue != INVALID_SOCKET) {

				//
				// Add a reference if the socket we got back was different
				// that the one we passed in (c_root cases only)
				//
				if (ReturnValue != s) {
					DSOCKET::AddSpecialApiReference( ReturnValue );
				}

				return ReturnValue;
			}

		} //if
		else {
			ErrorCode = WSAENOTSOCK;
		}
	}

    SetLastError(ErrorCode);
    return(INVALID_SOCKET);
}




int WSAAPI
closesocket(
    IN SOCKET s
    )
/*++
Routine Description:

    Close a socket.

Arguments:

    s - A descriptor identifying a socket.

Returns:
    Zero on success else SOCKET_ERROR. The error code is stored with
    SetErrorCode().
--*/
{
    INT                 ReturnValue;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDSOCKET            Socket;

    ErrorCode = TURBO_PROLOG();

    if (ErrorCode == ERROR_SUCCESS) {

		Socket = DSOCKET::GetCountedDSocketFromSocket(s);
		if(Socket != NULL){
			// The   actual  destruction  of  the  DSOCKET  object  closed  through
			// "closesocket"   happens  after  we  return  from  the  provider  and
			// determine  that  no  other  threads have remaining references to the
			// object.

			//
			// First, determine if this handle is for the provider socket so
			// we can clear the context table entry BEFORE closing the
			// socket. This plugs a nasty race condition where the provider
			// closes its handle and another thread creates a new socket
			// with the same handle value BEFORE the first thread manages
			// to clear the handle table entry.
			//
            // This is now handled by new context referencing functions
            // which will not clear handle table entry if it was replaced.
            //

			// if (ProviderSocket) {
			//	Socket->DisassociateSocketHandle();
			// }


			//Call the provider to close the socket.
			Provider = Socket->GetDProvider();
			ReturnValue = Provider->WSPCloseSocket( s,
												   &ErrorCode);
			if( (ReturnValue == ERROR_SUCCESS)
                    && Socket->IsProviderSocket ()){

				//
				// Remove context from the table and "active" reference
                // from the socket.
				// Non-provider generated socket active reference
                // is removed when provider destroys the handle
                // via WPUCloseSocketHandle call.
				//
				// Note the spec flow in case provider generated socket was
				// used only by the layered provider above and never
				// returned via socket/WSASocket call.  closesocket
				// is not called for such socket and we never get to
				// execute the code below. which leads to socket object
				// leak.
                //
                // It may have been replaced by another handle
                // when we call this funciton in which case
                // we do not need to drop the reference count
                // becuase it was done by whoever bumped it
                // out of the table, see comment above
                //
                if (Socket->DisassociateSocketHandle()==NO_ERROR)
                    Socket->DropDSocketReference();
			}

			//
			// Remove the reference added by GetCountedDSocketFromSocket.
			//

			Socket->DropDSocketReference();

			if( ReturnValue == ERROR_SUCCESS)
				return ReturnValue;
            //
			// The close failed. Restore the context table entry if
			// necessary.
			//
            // Don't need to do this anymore, see comment above
            //

			// if( ProviderSocket ) {
			//	if (Socket->AssociateSocketHandle(s, TRUE)!=NO_ERROR) {
					//
					// Failed to reinsert the socket into the table
					// This is the only thing we can do here.
					//
			//		assert (FALSE);
			//	}
			//}

		} // if
		else {
			ErrorCode = WSAENOTSOCK;
		}
	}

    SetLastError(ErrorCode);
    return(SOCKET_ERROR);
}
