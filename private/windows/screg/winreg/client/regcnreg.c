/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Regcnreg.c

Abstract:

    This module contains the Win32 Registry APIs to connect to a remote
    Registry.  That is:

       - RegConnectRegistryA
       - RegConnectRegistryW
Author:

    David J. Gilman (davegi) 25-Mar-1992

Notes:

    The semantics of this API make it local only. That is there is no MIDL
    definition for RegConnectRegistry although it does call other client
    stubs, specifically OpenLocalMachine and OpenUsers.

Revision History:

    John Vert (jvert) 16-Jun-1995
       Added connect support for protocols other than named pipes by
       stealing code from Win95. This enabled NT machines to connect
       to registries on Win95 machines

--*/

#include <rpc.h>
#include "regrpc.h"
#include "client.h"
#include "shutinit.h"
#include "..\regconn\regconn.h"

LONG
BaseBindToMachine(
    IN LPCWSTR lpMachineName,
    IN PBIND_CALLBACK BindCallback,
    IN PVOID Context1,
    IN PVOID Context2
    );


typedef int (* RegConnFunction)(LPCWSTR, handle_t *);

RegConnFunction conn_functions[] = {
        RegConn_np,
        RegConn_spx,
        RegConn_ip_tcp,
        RegConn_nb_nb,
        RegConn_nb_tcp,
        RegConn_nb_ipx,
        NULL
};

LONG
Rpc_OpenPredefHandle(
    IN RPC_BINDING_HANDLE * pbinding OPTIONAL,
    IN HKEY hKey,
    OUT PHKEY phkResult
    )

/*++

Routine Description:

    Win32 Unicode API for establishing a connection to a predefined
    handle on another machine.

Parameters:

    pbinding - This is a pointer to the binding handle in order
                to allow access to multiple protocols (NT remote registry is only over
                named pipes).

    hKey - Supplies the predefined handle to connect to on the remote
        machine. Currently this parameter must be one of:

        - HKEY_LOCAL_MACHINE
        - HKEY_PERFORMANCE_DATA
        - HKEY_USERS

    phkResult - Returns a handle which represents the supplied predefined
        handle on the supplied machine.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.
        On failure, the binding handle is freed.

Notes:

    For administration purposes this API allows programs to access the
    Registry on a remote machine.  In the current system the calling
    application must know the name of the remote machine that it wishes to
    connect to.  However, it is expected that in the future a directory
    service API will return the parameters necessary for this API.

--*/

{
    LONG    Error;
    HKEY    PreviousResult;

    ASSERT( (phkResult != NULL));
    PreviousResult = *phkResult;

    switch ((int)(ULONG_PTR)hKey)
    {
        case (int)(ULONG_PTR)HKEY_LOCAL_MACHINE:

            Error = (LONG)OpenLocalMachine((PREGISTRY_SERVER_NAME) pbinding,
                                           MAXIMUM_ALLOWED,
                                           phkResult );
            break;

        case (int)(ULONG_PTR)HKEY_PERFORMANCE_DATA:

            Error = (LONG)OpenPerformanceData((PREGISTRY_SERVER_NAME) pbinding,
                                              MAXIMUM_ALLOWED,
                                              phkResult );
            break;

        case (int)(ULONG_PTR)HKEY_USERS:

            Error = (LONG)OpenUsers((PREGISTRY_SERVER_NAME) pbinding,
                                    MAXIMUM_ALLOWED,
                                    phkResult );
            break;

        case (int)(ULONG_PTR)HKEY_CLASSES_ROOT:

            Error = (LONG)OpenClassesRoot((PREGISTRY_SERVER_NAME) pbinding,
                                          MAXIMUM_ALLOWED,
                                          phkResult );
            break;

        case (int)(ULONG_PTR)HKEY_CURRENT_USER:

            Error = (LONG)OpenCurrentUser((PREGISTRY_SERVER_NAME) pbinding,
                                          MAXIMUM_ALLOWED,
                                          phkResult );
            break;

        default:
            Error = ERROR_INVALID_HANDLE;
    }

    if( Error != ERROR_SUCCESS) {
        ASSERTMSG("WINREG: RPC failed, but modifed phkResult", *phkResult == PreviousResult);
        if (*pbinding != NULL)
            RpcBindingFree(pbinding);
    }

    return Error;
}


LONG
RegConnectRegistryW (
    IN LPCWSTR lpMachineName OPTIONAL,
    IN HKEY hKey,
    OUT PHKEY phkResult
    )

/*++

Routine Description:

    Win32 Unicode API for establishing a connection to a predefined
    handle on another machine.

Parameters:

    lpMachineName - Supplies a pointer to a null-terminated string that
    names the machine of interest.  If this parameter is NULL, the local
    machine name is used.

    hKey - Supplies the predefined handle to connect to on the remote
    machine. Currently this parameter must be one of:

    - HKEY_LOCAL_MACHINE
    - HKEY_PERFORMANCE_DATA
    - HKEY_USERS

    phkResult - Returns a handle which represents the supplied predefined
    handle on the supplied machine.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

Notes:

    For administration purposes this API allows programs to access the
    Registry on a remote machine.  In the current system the calling
    application must know the name of the remote machine that it wishes to
    connect to.  However, it is expected that in the future a directory
    service API will return the parameters necessary for this API.

    Even though HKEY_CLASSES and HKEY_CURRENT_USER are predefined handles,
    they are not supported by this API as they do not make sense in the
    context of a remote Registry.

--*/

{
    LONG    Error;

    ASSERT( ARGUMENT_PRESENT( phkResult ));

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif
    //
    // Check for local connect
    //

    if (lpMachineName == NULL) {
        *phkResult = hKey;
        return(ERROR_SUCCESS);
    } else if (lpMachineName[0] == L'\0') {
        *phkResult = hKey;
        return(ERROR_SUCCESS);
    }

    Error = BaseBindToMachine(lpMachineName,
                              Rpc_OpenPredefHandle,
                              (PVOID)hKey,
                              (PVOID)phkResult);

    if( Error == ERROR_SUCCESS) {
        TagRemoteHandle( phkResult );
    }
    return Error;
}

LONG
BaseBindToMachine(
    IN LPCWSTR lpMachineName,
    IN PBIND_CALLBACK BindCallback,
    IN PVOID Context1,
    IN PVOID Context2
    )

/*++

Routine Description:

    This is a helper routine used to create an RPC binding from
    a given machine name.

Arguments:

    lpMachineName - Supplies a pointer to a machine name. Must not
                    be NULL.

    BindCallback - Supplies the function that should be called once
                   a binding has been created to initiate the connection.

    Context1 - Supplies the first parameter to pass to the callback routine.

    Context2 - Supplies the second parameter to pass to the callback routine.

Return Value:

    Returns ERROR_SUCCESS (0) for success; error-code for failure.

--*/

{
    LONG    Error;
    int         i;
    RegConnFunction conn_fn;
    RPC_BINDING_HANDLE binding;

    conn_fn = conn_functions[0];
    i = 1;

    //
    // Iterate through the protocols until we find one that
    // can connect.
    //
    do {
        Error = conn_fn(lpMachineName,&binding);

        if (Error == ERROR_SUCCESS) {

            //
            // For the named pipes protocol, we use a static endpoint, so the
            // call to RpcEpResolveBinding is not needed.
            // Also, the server checks the user's credentials on opening
            // the named pipe, so RpcBindingSetAuthInfo is not needed.
            //
            if (conn_fn != RegConn_np) {
                Error = (LONG)RpcEpResolveBinding(binding,winreg_ClientIfHandle);

                if (Error == ERROR_SUCCESS) {
                    Error = (LONG)RpcBindingSetAuthInfo(binding,
                                            "",     // ServerPrincName
                                            RPC_C_AUTHN_LEVEL_CONNECT,
                                            RPC_C_AUTHN_WINNT,
                                            NULL,   // AuthIdentity
                                            RPC_C_AUTHZ_NONE);
                }
            }
            if (Error == ERROR_SUCCESS) {
                Error = (BindCallback)(&binding,
                                       Context1,
                                       Context2);
                RpcBindingFree(&binding);
                if (Error != RPC_S_SERVER_UNAVAILABLE) {
                    return Error;
                }
            } else {
                RpcBindingFree(&binding);
            }
        }

        //
        // Try the next protocol's connection function.
        //
        if (Error) {
            conn_fn = conn_functions[i];
            i++;
        }

    } while (!((Error == ERROR_SUCCESS) || (conn_fn == NULL)));

    if (Error != ERROR_SUCCESS) {
        if ((Error == RPC_S_INVALID_ENDPOINT_FORMAT) ||
            (Error == RPC_S_INVALID_NET_ADDR) ) {
            Error = ERROR_INVALID_COMPUTERNAME;
        } else {
            Error = ERROR_BAD_NETPATH;
        }
    }

    return(Error);
}

    
LONG
APIENTRY
RegConnectRegistryA (
    LPCSTR lpMachineName,
    HKEY hKey,
    PHKEY phkResult
    )

/*++

Routine Description:

    Win32 ANSI API for establishes a connection to a predefined handle on
    another machine.

    RegConnectRegistryA converts the lpMachineName argument to a Unicode
    string and then calls RegConnectRegistryW.

--*/

{
    UNICODE_STRING     MachineName;
    ANSI_STRING     AnsiString;
    NTSTATUS        Status;
    LONG        Error;

#if DBG
    if ( BreakPointOnEntry ) {
        DbgBreakPoint();
    }
#endif


    //
    // Convert the subkey to a counted Unicode string
    //

    RtlInitAnsiString( &AnsiString, lpMachineName );
    Status = RtlAnsiStringToUnicodeString(&MachineName,
                                          &AnsiString,
                                          TRUE);

    if( ! NT_SUCCESS( Status )) {
        return RtlNtStatusToDosError( Status );
    }

    Error = (LONG)RegConnectRegistryW(MachineName.Buffer,
                                      hKey,
                                      phkResult);
    RtlFreeUnicodeString(&MachineName);
    return Error;

}
