/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS WinSock 2 API
 * FILE:        dll/win32/ws2_32_new/src/socklife.c
 * PURPOSE:     Socket Lifetime Support
 * PROGRAMMER:  Alex Ionescu (alex@relsoft.net)
 */

/* INCLUDES ******************************************************************/

#include <ws2_32.h>

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
SOCKET
WSAAPI
accept(IN  SOCKET s,
       OUT LPSOCKADDR addr,
       OUT INT FAR* addrlen)
{
    /* Let WSA do it */
    return WSAAccept(s, addr, addrlen, NULL, 0);
}

/*
 * @implemented
 */
INT
WSAAPI
bind(IN SOCKET s,
     IN CONST struct sockaddr *name,
     IN INT namelen)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    DPRINT("bind: %lx, %p, %lx\n", s, name, namelen);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPBind(s,
                                                         name, 
                                                         namelen,
                                                         &ErrorCode);
            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Return Provider Value */
            if (Status == ERROR_SUCCESS) return Status;

            /* If everything seemed fine, then the WSP call failed itself */
            if (ErrorCode == NO_ERROR) ErrorCode = WSASYSCALLFAILURE;
        }
        else
        {
            /* No Socket Context Found */
            ErrorCode = WSAENOTSOCK;
        }
    }

    /* Return with an Error */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}

/*
 * @implemented
 */
INT
WSAAPI
closesocket(IN SOCKET s)
{
    PWSSOCKET Socket;
    INT Status;
    INT ErrorCode;
    DPRINT("closesocket: %lx\n", s);

    /* Check for WSAStartup */
    if ((ErrorCode = WsQuickProlog()) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Make the call */
            Status = Socket->Provider->Service.lpWSPCloseSocket(s, &ErrorCode);

            /* Check if this is a provider socket */
            if ((Status == ERROR_SUCCESS) && (Socket->IsProvider))
            {
                /* Disassociate the handle */
                if (WsSockDisassociateHandle(Socket) == ERROR_SUCCESS)
                {
                    /* Deference the Socket Context */
                    WsSockDereference(Socket);
                }

                /* Remove the last reference */
                WsSockDereference(Socket);

                /* Return success if everything is OK */
                if (ErrorCode == ERROR_SUCCESS) return ErrorCode;
            }
        }
        else
        {
            /* No Socket Context Found */
            ErrorCode = WSAENOTSOCK;
        }
    }

    /* Return with an Error */
    SetLastError(ErrorCode);
    return SOCKET_ERROR;
}

/*
 * @implemented
 */
SOCKET
WSAAPI
socket(IN INT af,
       IN INT type,
       IN INT protocol)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    DWORD Flags = 0;
    INT ErrorCode;
    DPRINT("socket: %lx, %lx, %lx\n", af, type, protocol);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Fail here */
        SetLastError(ErrorCode);
        return INVALID_SOCKET;
    }

    /* Check the current open type and use overlapped if it's default */
    if (!Thread->OpenType) Flags = WSA_FLAG_OVERLAPPED;

    /* Make the protocol negative if this is NETBIOS */
    if ((af == AF_NETBIOS) && (protocol > 0)) protocol *= -1;

    /* Now let WSA handle it */
    return WSASocketW(af, type, protocol, NULL, 0, Flags);
}

/*
 * @unimplemented
 */
INT
WSPAPI
WPUCloseSocketHandle(IN SOCKET s,
                     OUT LPINT lpErrno)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @unimplemented
 */
SOCKET
WSPAPI
WPUCreateSocketHandle(IN DWORD dwCatalogEntryId,
                      IN DWORD_PTR dwContext,
                      OUT LPINT lpErrno)
{
    UNIMPLEMENTED;
    return (SOCKET)0;
}

/*
 * @implemented
 */
SOCKET
WSPAPI
WPUModifyIFSHandle(IN DWORD dwCatalogEntryId,
                   IN SOCKET ProposedHandle,
                   OUT LPINT lpErrno)
{
    SOCKET Handle = INVALID_SOCKET;
    DWORD ErrorCode = ERROR_SUCCESS;
    PWSPROCESS Process;
    PTCATALOG Catalog;
    PTCATALOG_ENTRY Entry;
    PWSSOCKET Socket;
    DPRINT("WPUModifyIFSHandle: %lx, %lx\n", dwCatalogEntryId, ProposedHandle);

    /* Get the current process */
    if ((Process = WsGetProcess()))
    {
        /* Get the Transport Catalog */
        if ((Catalog = WsProcGetTCatalog(Process)))
        {
            /* Get the entry for this ID */
            ErrorCode = WsTcGetEntryFromCatalogEntryId(Catalog,
                                                       dwCatalogEntryId,
                                                       &Entry);
            /* Check for success */
            if (ErrorCode == ERROR_SUCCESS)
            {
                /* Create a socket object */
                if ((Socket = WsSockAllocate()))
                {
                    /* Initialize it */
                    WsSockInitialize(Socket, Entry);

                    /* Associate it */
                    ErrorCode = WsSockAssociateHandle(Socket,
                                                      ProposedHandle,
                                                      TRUE);
                    /* Check for success */
                    if (ErrorCode == ERROR_SUCCESS)
                    {
                        /* Return */
                        Handle = ProposedHandle;
                        *lpErrno = ERROR_SUCCESS;
                    }
                    else
                    {
                        /* Fail */
                        WsSockDereference(Socket);
                        *lpErrno = ErrorCode;
                    }

                    /* Dereference the extra count */
                    WsSockDereference(Socket);
                }
                else
                {
                    /* No memory to allocate a socket */
                    *lpErrno = WSAENOBUFS;
                }

                /* Dereference the catalog entry */
                WsTcEntryDereference(Entry);
            }
            else
            {
                /* Entry not found */
                *lpErrno = ErrorCode;
            }
        }
        else
        {
            /* Catalog not found */
            *lpErrno = WSANOTINITIALISED;
        }
    }
    else
    {
        /* Process not ready */
        *lpErrno = WSANOTINITIALISED;
    }

    /* Return */
    return Handle;
}

/*
 * @unimplemented
 */
INT
WSPAPI
WPUQuerySocketHandleContext(IN SOCKET s,
                            OUT PDWORD_PTR lpContext,
                            OUT LPINT lpErrno)
{
    UNIMPLEMENTED;
    return 0;
}

/*
 * @implemented
 */
SOCKET
WSAAPI
WSAAccept(IN SOCKET s,
          OUT LPSOCKADDR addr,
          IN OUT LPINT addrlen,
          IN LPCONDITIONPROC lpfnCondition,
          IN DWORD_PTR dwCallbackData)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PWSSOCKET Socket;
    DWORD OpenType;
    INT ErrorCode;
    SOCKET Status;
    DPRINT("WSAAccept: %lx, %lx, %lx, %p\n", s, addr, addrlen, lpfnCondition);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Get the old open type and set new one */
            OpenType = Thread->OpenType;
            Thread->OpenType = Socket->Overlapped ? 0 : SO_SYNCHRONOUS_NONALERT;

            /* Make the call */
            Status = Socket->Provider->Service.lpWSPAccept(s,
                                                           addr,
                                                           addrlen,
                                                           lpfnCondition,
                                                           dwCallbackData,
                                                           &ErrorCode);
            /* Restore open type */
            Thread->OpenType = OpenType;

            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Check if we got a valid socket */
            if (Status != INVALID_SOCKET)
            {
                /* Check if we got a new socket */
                if (Status != s)
                {
                    /* Add a new reference */
                    WsSockAddApiReference(Status);
                }

                /* Return */
                return Status;
            }
        }
        else
        {
            /* No Socket Context Found */
            ErrorCode = WSAENOTSOCK;
        }
    }

    /* Return with an Error */
    SetLastError(ErrorCode);
    return INVALID_SOCKET;
}

/*
 * @implemented
 */
SOCKET
WSAAPI
WSAJoinLeaf(IN SOCKET s,
            IN CONST struct sockaddr *name,
            IN INT namelen,
            IN LPWSABUF lpCallerData,
            OUT LPWSABUF lpCalleeData,
            IN LPQOS lpSQOS,
            IN LPQOS lpGQOS,
            IN DWORD dwFlags)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    PWSSOCKET Socket;
    DWORD OpenType;
    INT ErrorCode;
    SOCKET Status;
    DPRINT("WSAJoinLeaf: %lx, %lx, %lx\n", s, name, namelen);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) == ERROR_SUCCESS)
    {
        /* Get the Socket Context */
        if ((Socket = WsSockGetSocket(s)))
        {
            /* Get the old open type and set new one */
            OpenType = Thread->OpenType;
            Thread->OpenType = Socket->Overlapped ? 0 : SO_SYNCHRONOUS_NONALERT;

            /* Make the call */
            Status = Socket->Provider->Service.lpWSPJoinLeaf(s,
                                                             name,
                                                             namelen,
                                                             lpCallerData,
                                                             lpCalleeData,
                                                             lpSQOS,
                                                             lpGQOS,
                                                             dwFlags,
                                                             &ErrorCode);
            /* Restore open type */
            Thread->OpenType = OpenType;

            /* Deference the Socket Context */
            WsSockDereference(Socket);

            /* Check if we got a valid socket */
            if (Status != INVALID_SOCKET)
            {
                /* Check if we got a new socket */
                if (Status != s)
                {
                    /* Add a new reference */
                    WsSockAddApiReference(Status);
                }

                /* Return */
                return Status;
            }
        }
        else
        {
            /* No Socket Context Found */
            ErrorCode = WSAENOTSOCK;
        }
    }

    /* Return with an Error */
    SetLastError(ErrorCode);
    return INVALID_SOCKET;
}

/*
 * @implemented
 */
SOCKET
WSAAPI
WSASocketA(IN INT af,
           IN INT type,
           IN INT protocol,
           IN LPWSAPROTOCOL_INFOA lpProtocolInfo,
           IN GROUP g,
           IN DWORD dwFlags)
{
    WSAPROTOCOL_INFOW ProtocolInfoW;
    LPWSAPROTOCOL_INFOW p = &ProtocolInfoW;

    /* Convert Protocol Info to Wide */
    if (lpProtocolInfo) 
    {    
        /* Copy the Data */
        memcpy(&ProtocolInfoW,
               lpProtocolInfo,
               sizeof(WSAPROTOCOL_INFOA) - sizeof(CHAR) * (WSAPROTOCOL_LEN + 1));

        /* Convert the String */
        MultiByteToWideChar(CP_ACP,
                            0,
                            lpProtocolInfo->szProtocol,
                            -1,
                            ProtocolInfoW.szProtocol,
                            sizeof(ProtocolInfoW.szProtocol) / sizeof(WCHAR));
    } 
    else 
    {
        /* No Protocol Info Specified */
        p = NULL;
    }

    /* Call the Unicode Function */
    return WSASocketW(af,
                      type,
                      protocol,
                      p,
                      g,
                      dwFlags);
}

/*
 * @implemented
 */
SOCKET
WSAAPI 
WSASocketW(IN INT af,
           IN INT type,
           IN INT protocol,
           IN LPWSAPROTOCOL_INFOW lpProtocolInfo,
           IN GROUP g,
           IN DWORD dwFlags)
{
    PWSPROCESS Process;
    PWSTHREAD Thread;
    INT ErrorCode;
    PTCATALOG Catalog;
    DWORD CatalogId;
    PTCATALOG_ENTRY CatalogEntry;
    LPWSAPROTOCOL_INFOW ProtocolInfo;
    DWORD OpenType;
    SOCKET Status = INVALID_SOCKET;
    DPRINT("WSASocketW: %lx, %lx, %lx, %p\n", af, type, protocol, lpProtocolInfo);

    /* Enter prolog */
    if ((ErrorCode = WsApiProlog(&Process, &Thread)) != ERROR_SUCCESS)
    {
        /* Fail now */
        SetLastError(ErrorCode);
        return INVALID_SOCKET;
    }

    /* Get the catalog */
    Catalog = WsProcGetTCatalog(Process);

    /* Find a Provider for the Catalog ID */
    if (lpProtocolInfo) 
    {   
        /* Get the catalog ID */
        CatalogId = lpProtocolInfo->dwCatalogEntryId;

        /* Get the Catalog Entry */
        ErrorCode = WsTcGetEntryFromCatalogEntryId(Catalog,
                                                   CatalogId,
                                                   &CatalogEntry);
    }
    else  
    {   
        /* No ID */
        CatalogId = 0;

DoLookup:
        /* Get the Catalog Data from the Socket Info */
        ErrorCode = WsTcGetEntryFromTriplet(Catalog,
                                            af,
                                            type,
                                            protocol,
                                            CatalogId,
                                            &CatalogEntry);
    }

    /* Check for Success */
    if (ErrorCode == ERROR_SUCCESS)
    {
        /* Use the default Protocol Info if none given */
        ProtocolInfo = lpProtocolInfo ? lpProtocolInfo : &CatalogEntry->ProtocolInfo;

        /* Save the open type and set new one */
        OpenType = Thread->OpenType;
        Thread->OpenType = (dwFlags & WSA_FLAG_OVERLAPPED) ?
                            0 : SO_SYNCHRONOUS_NONALERT;

        /* Call the Provider to create the Socket */
        Status = CatalogEntry->Provider->Service.lpWSPSocket(af,
                                                             type,
                                                             protocol,
                                                             ProtocolInfo,
                                                             g,
                                                             dwFlags,
                                                             &ErrorCode);
        /* Restore open type */
        Thread->OpenType = OpenType;

        /* Get the catalog ID now, and dereference */
        CatalogId = ProtocolInfo->dwCatalogEntryId;
        WsTcEntryDereference(CatalogEntry);

        /* Did we fail with WSAEINPROGRESS and had no specific provider? */
        if ((Status == INVALID_SOCKET) && 
            (ErrorCode == WSAEINPROGRESS) && 
            !(lpProtocolInfo))
        {
            /* In that case, restart the lookup from this ID */
            goto DoLookup;
        }

        /* Check if we got a valid socket */
        if (Status != INVALID_SOCKET)
        {
            /* Add an API reference and return */
            WsSockAddApiReference(Status);
            return Status;
        }
    }

    /* Return with an Error */
    SetLastError(ErrorCode);
    return INVALID_SOCKET;
}
