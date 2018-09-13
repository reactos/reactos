/*++

Copyright (c) 1990  Microsoft Corporation

Module Name:

    rpcbind.c

Abstract:

    Contains the RPC bind and un-bind routines for the Eventlog
    client-side APIs.

Author:

    Rajen Shah      (rajens)    30-Jul-1991

Revision History:

    30-Jul-1991     RajenS
        created

--*/

//
// INCLUDES
//
#include <elfclntp.h>
#include <lmsvc.h>
#include <svcs.h>  // SVCS_LRPC_*

#define SERVICE_EVENTLOG    L"EVENTLOG"


/****************************************************************************/
handle_t
EVENTLOG_HANDLE_W_bind (
    EVENTLOG_HANDLE_W   ServerName)

/*++

Routine Description:
    This routine calls a common bind routine that is shared by all services.
    This routine is called from the ElfOpenEventLog API client stub when
    it is necessary to bind to a server.
    The binding is done to allow impersonation by the server since that is
    necessary for the API calls.

Arguments:

    ServerName - A pointer to a string containing the name of the server
        to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the
    binding is unsuccessful, a NULL will be returned.

--*/
{
    handle_t    bindingHandle;
    RPC_STATUS  status;

    // If we're connecting to the local services use LRPC to avoid silly bugs
    // with cached tokens in named pipes.  (Talk to AlbertT/MarioGo)
    // SVCS_LRPC_* defines come from private\inc\svcs.h

    if (ServerName == NULL ||
        wcscmp(ServerName, L"\\\\.") == 0 ) {
        
        PWSTR sb;
        status = RpcStringBindingComposeW(0,
                                          SVCS_LRPC_PROTOCOL, 
                                          0,
                                          SVCS_LRPC_PORT,
                                          0,
                                          &sb);

        if (status == RPC_S_OK) {
            status = RpcBindingFromStringBindingW(sb, &bindingHandle);

            RpcStringFreeW(&sb);

            if (status == RPC_S_OK) {
                return bindingHandle;
            }
        }
        return NULL;
    }

    status = RpcpBindRpc (
                ServerName,   
                SERVICE_EVENTLOG,
                NULL,
                &bindingHandle);

    // DbgPrint("EVENTLOG_bind: handle=%d\n",bindingHandle);
    return( bindingHandle);
}



/****************************************************************************/
void
EVENTLOG_HANDLE_W_unbind (
    EVENTLOG_HANDLE_W   ServerName,
    handle_t        BindingHandle)

/*++

Routine Description:

    This routine calls a common unbind routine that is shared by
    all services.

Arguments:

    ServerName - This is the name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    none.

--*/
{
    RPC_STATUS  status;

    // DbgPrint("EVENTLOG_HANDLE_unbind: handle=%d\n",BindingHandle);
    status = RpcpUnbindRpc ( BindingHandle);
    return;

    UNREFERENCED_PARAMETER(ServerName);

}


handle_t
EVENTLOG_HANDLE_A_bind (
    EVENTLOG_HANDLE_A   ServerName)

/*++

Routine Description:

    This routine calls EVENTLOG_HANDLE_W_bind to do the work.

Arguments:

    ServerName - A pointer to a UNICODE string containing the name of
    the server to bind with.

Return Value:

    The binding handle is returned to the stub routine.  If the
    binding is unsuccessful, a NULL will be returned.

--*/
{
    UNICODE_STRING  ServerNameU;
    ANSI_STRING     ServerNameA;
    handle_t        bindingHandle;

    //
    // Convert the ANSI string to a UNICODE string before calling the
    // UNICODE routine.
    //
    RtlInitAnsiString (&ServerNameA, (PSTR)ServerName);

    RtlAnsiStringToUnicodeString (
            &ServerNameU,
            &ServerNameA,
            TRUE
            );

    bindingHandle = EVENTLOG_HANDLE_W_bind(
                (EVENTLOG_HANDLE_W)ServerNameU.Buffer
                );

    RtlFreeUnicodeString (&ServerNameU);

    return( bindingHandle);
}



/****************************************************************************/
void
EVENTLOG_HANDLE_A_unbind (
    EVENTLOG_HANDLE_A   ServerName,
    handle_t        BindingHandle)

/*++

Routine Description:

    This routine calls EVENTLOG_HANDLE_W_unbind.

Arguments:

    ServerName - This is the ANSI name of the server from which to unbind.

    BindingHandle - This is the binding handle that is to be closed.

Return Value:

    none.

--*/
{
    UNICODE_STRING  ServerNameU;
    ANSI_STRING     ServerNameA;

    //
    // Convert the ANSI string to a UNICODE string before calling the
    // UNICODE routine.
    //
    RtlInitAnsiString (&ServerNameA, (PSTR)ServerName);

    RtlAnsiStringToUnicodeString (
            &ServerNameU,
            &ServerNameA,
            TRUE
            );

    EVENTLOG_HANDLE_W_unbind( (EVENTLOG_HANDLE_W)ServerNameU.Buffer,
                 BindingHandle );

    RtlFreeUnicodeString (&ServerNameU);

    return;
}
