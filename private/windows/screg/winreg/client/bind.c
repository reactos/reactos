/*++

Copyright (c) 1992 Microsoft Corporation

Module Name:

    Bind.c

Abstract:

    This module contains routines for binding and unbinding to the Win32
    Registry server.

Author:

    David J. Gilman (davegi) 06-Feb-1992

--*/

#include <ntrpcp.h>
#include <rpc.h>
#include "regrpc.h"

//
// wRegConn_bind - common function to bind to a transport and free the
//                                      string binding.
//

wRegConn_bind(
    LPWSTR *    StringBinding,
    RPC_BINDING_HANDLE * pBindingHandle
    )
{
    DWORD RpcStatus;

    RpcStatus = RpcBindingFromStringBindingW(*StringBinding,pBindingHandle);

    RpcStringFreeW(StringBinding);
    if ( RpcStatus != RPC_S_OK ) {
        *pBindingHandle = NULL;
        return RpcStatus;
    }
    return(ERROR_SUCCESS);
}


/*++

Routine Description for the RegConn_* functions:

    Bind to the RPC server over the specified transport

Arguments:

    ServerName - Name of server to bind with (or netaddress).

    pBindingHandle - Location where binding handle is to be placed

Return Value:

    ERROR_SUCCESS - The binding has been successfully completed.

    ERROR_INVALID_COMPUTER_NAME - The ServerName syntax is invalid.

    ERROR_NO_MEMORY - There is not sufficient memory available to the
        caller to perform the binding.

--*/



//
// wRegConn_Netbios - Worker function to get a binding handle for any of the
//                                              netbios protocols
//

DWORD wRegConn_Netbios(
    IN  LPWSTR  rpc_protocol,
    IN  LPCWSTR  ServerName,
    OUT RPC_BINDING_HANDLE * pBindingHandle
    )

{
    RPC_STATUS        RpcStatus;
    LPWSTR            StringBinding;
    LPCWSTR           PlainServerName;

    *pBindingHandle = NULL;

    //
    // Ignore leading "\\"
    //

    if ((ServerName[0] == '\\') && (ServerName[1] == '\\')) {
        PlainServerName = &ServerName[2];
    } else {
        PlainServerName = ServerName;
    }

    RpcStatus = RpcStringBindingComposeW(0,
                                         rpc_protocol,
                                         (LPWSTR)PlainServerName,
                                         NULL,   // endpoint
                                         NULL,
                                         &StringBinding);

    if ( RpcStatus != RPC_S_OK ) {
        return( ERROR_BAD_NETPATH );
    }
    return(wRegConn_bind(&StringBinding, pBindingHandle));
}

DWORD
RegConn_nb_nb(
    IN  LPCWSTR ServerName,
    OUT RPC_BINDING_HANDLE * pBindingHandle
    )
{
        return(wRegConn_Netbios(L"ncacn_nb_nb",
                                ServerName,
                                pBindingHandle));
}

DWORD
RegConn_nb_tcp(
    IN  LPCWSTR ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
{
        return(wRegConn_Netbios(L"ncacn_nb_tcp",
                                ServerName,
                                pBindingHandle));
}

DWORD
RegConn_nb_ipx(
    IN  LPCWSTR               ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
{
        return(wRegConn_Netbios(L"ncacn_nb_ipx",
                                ServerName,
                                pBindingHandle));
}


//
// RegConn_np - get a remote registry RPC binding handle for an NT server
//                              (Win95 does not support server-side named pipes)
//

DWORD
RegConn_np(
    IN  LPCWSTR              ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
{
    RPC_STATUS  RpcStatus;
    LPWSTR      StringBinding;
    LPWSTR      SlashServerName;
    int         have_slashes;
    ULONG       NameLen;

    *pBindingHandle = NULL;

    if (ServerName[1] == L'\\') {
        have_slashes = 1;
    } else {
        have_slashes = 0;
    }

    //
    // Be nice and prepend slashes if not supplied.
    //

    NameLen = lstrlenW(ServerName);
    if ((!have_slashes) &&
        (NameLen > 0)) {

        //
        // Allocate new buffer large enough for two forward slashes and a
        // NULL terminator.
        //
        SlashServerName = LocalAlloc(LMEM_FIXED, (NameLen + 3) * sizeof(WCHAR));
        if (SlashServerName == NULL) {
            return(ERROR_NOT_ENOUGH_MEMORY);
        }
        SlashServerName[0] = L'\\';
        SlashServerName[1] = L'\\';
        lstrcpyW(&SlashServerName[2], ServerName);
    } else {
        SlashServerName = (LPWSTR)ServerName;
    }

    RpcStatus = RpcStringBindingComposeW(0,
                                         L"ncacn_np",
                                         SlashServerName,
                                         L"\\PIPE\\winreg",
                                         NULL,
                                         &StringBinding);
    if (SlashServerName != ServerName) {
        LocalFree(SlashServerName);
    }

    if ( RpcStatus != RPC_S_OK ) {
        return( ERROR_BAD_NETPATH );
    }

    return(wRegConn_bind(&StringBinding, pBindingHandle));
}


//
// RegConn_spx - Use the Netbios connection function, RPC will resolve the name
//                               via winsock.
//

DWORD
RegConn_spx(
    IN  LPCWSTR              ServerName,
    OUT RPC_BINDING_HANDLE   * pBindingHandle
    )
{
    return(wRegConn_Netbios(L"ncacn_spx",
                            ServerName,
                            pBindingHandle));
}


DWORD RegConn_ip_tcp(
    IN  LPCWSTR  ServerName,
    OUT RPC_BINDING_HANDLE * pBindingHandle
    )

{
    return(wRegConn_Netbios(L"ncacn_ip_tcp",
                            ServerName,
                            pBindingHandle));
}

RPC_BINDING_HANDLE
PREGISTRY_SERVER_NAME_bind(
        PREGISTRY_SERVER_NAME ServerName
    )

/*++

Routine Description:

    To make the remote registry multi-protocol aware, PREGISTRY_SERVER_NAME
        parameter actually points to an already bound binding handle.
        PREGISTRY_SERVER_NAME is declared a PWSTR only to help maintain
        compatibility with NT.

--*/

{
    return(*(RPC_BINDING_HANDLE *)ServerName);
}


void
PREGISTRY_SERVER_NAME_unbind(
    PREGISTRY_SERVER_NAME ServerName,
    RPC_BINDING_HANDLE BindingHandle
    )

/*++

Routine Description:

    This routine unbinds the RPC client from the server. It is called
    directly from the RPC stub that references the handle.

Arguments:

    ServerName - Not used.

    BindingHandle - Supplies the handle to unbind.

Return Value:

    None.

--*/

{
    DWORD    Status;

    UNREFERENCED_PARAMETER( ServerName );
    return;

}
