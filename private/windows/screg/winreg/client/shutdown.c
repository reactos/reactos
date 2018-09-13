/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    Shutdown.c

Abstract:

    This module contains the client side wrappers for the Win32 remote
    shutdown APIs, that is:

        - InitiateSystemShutdownA
        - InitiateSystemShutdownW
        - AbortSystemShutdownA
        - AbortSystemShutdownW

Author:

    Dave Chalmers (davidc) 29-Apr-1992

Notes:


Revision History:
    
    Dragos C. Sambotin (dragoss) 21-May-1999
        Added support for the new winlogon's Shutdown interface

--*/


#define UNICODE

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

LONG
ShutdownCallback(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PUNICODE_STRING Message,
    IN PVOID Context2
    );

LONG
ShutdownCallbackEx(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PUNICODE_STRING Message,
    IN PVOID Context2
    );

LONG
AbortShutdownCallback(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PVOID Context1,
    IN PVOID Context2
    );

BOOL
APIENTRY
InitiateSystemShutdownW(
    IN LPWSTR lpMachineName OPTIONAL,
    IN LPWSTR lpMessage OPTIONAL,
    IN DWORD dwTimeout,
    IN BOOL bForceAppsClosed,
    IN BOOL bRebootAfterShutdown    
    )

/*++

Routine Description:

    Win32 Unicode API for initiating the shutdown of a (possibly remote) machine.

Arguments:

    lpMachineName - Name of machine to shutdown.

    lpMessage -     Message to display during shutdown timeout period.

    dwTimeout -     Number of seconds to delay before shutting down

    bForceAppsClosed - Normally applications may prevent system shutdown.
                    If this flag is set, all applications are terminated
                    unconditionally.

    bRebootAfterShutdown - TRUE if the system should reboot.
                    FALSE if it should be left in a shutdown state.

Return Value:

    Returns TRUE on success, FALSE on failure (GetLastError() returns error code)

    Possible errors :

        ERROR_SHUTDOWN_IN_PROGRESS - a shutdown has already been started on
                                     the specified machine.

--*/

{
    DWORD Result;
    UNICODE_STRING  Message;
    SHUTDOWN_CONTEXT ShutdownContext;
    BOOL    TryOld = TRUE;

    //
    // Explicitly bind to the given server.
    //
    if (!ARGUMENT_PRESENT(lpMachineName)) {
        lpMachineName = L"";
        TryOld = FALSE;
    }

    ShutdownContext.dwTimeout = dwTimeout;
    ShutdownContext.bForceAppsClosed = (bForceAppsClosed != 0);
    ShutdownContext.bRebootAfterShutdown = (bRebootAfterShutdown != 0);
    RtlInitUnicodeString(&Message, lpMessage);

    //
    // Call the server
    //
    
    //
    // First try to connect to the new InitShutdown interface
    //
    Result = BaseBindToMachineShutdownInterface(lpMachineName,
                                                NewShutdownCallback,
                                                &Message,
                                                &ShutdownContext);

    if( (Result != ERROR_SUCCESS) && (TryOld == TRUE) ) {
        //
        // try the old one, maybe we are calling into a NT4 machine
        // which doesn't know about the new interface
        //
        Result = BaseBindToMachine(lpMachineName,
                                   ShutdownCallback,
                                   &Message,
                                   &ShutdownContext);
    }

    if (Result != ERROR_SUCCESS) {
        SetLastError(Result);
    }

    return(Result == ERROR_SUCCESS);
}


BOOL
APIENTRY
InitiateSystemShutdownExW(
    IN LPWSTR lpMachineName OPTIONAL,
    IN LPWSTR lpMessage OPTIONAL,
    IN DWORD dwTimeout,
    IN BOOL bForceAppsClosed,
    IN BOOL bRebootAfterShutdown,
    IN DWORD dwReason
    )

/*++

Routine Description:

    Win32 Unicode API for initiating the shutdown of a (possibly remote) machine.

Arguments:

    lpMachineName - Name of machine to shutdown.

    lpMessage -     Message to display during shutdown timeout period.

    dwTimeout -     Number of seconds to delay before shutting down

    bForceAppsClosed - Normally applications may prevent system shutdown.
                    If this flag is set, all applications are terminated
                    unconditionally.

    bRebootAfterShutdown - TRUE if the system should reboot.
                    FALSE if it should be left in a shutdown state.

    dwReason        - Reason for initiating the shutdown.  This reason is logged
                        in the eventlog #6006 event.

Return Value:

    Returns TRUE on success, FALSE on failure (GetLastError() returns error code)

    Possible errors :

        ERROR_SHUTDOWN_IN_PROGRESS - a shutdown has already been started on
                                     the specified machine.

--*/

{
    DWORD Result;
    UNICODE_STRING  Message;
    SHUTDOWN_CONTEXTEX ShutdownContext;
    BOOL    TryOld = TRUE;

    //
    // Explicitly bind to the given server.
    //
    if (!ARGUMENT_PRESENT(lpMachineName)) {
        lpMachineName = L"";
        TryOld = FALSE;
    }

    ShutdownContext.dwTimeout = dwTimeout;
    ShutdownContext.bForceAppsClosed = (bForceAppsClosed != 0);
    ShutdownContext.bRebootAfterShutdown = (bRebootAfterShutdown != 0);
    ShutdownContext.dwReason = dwReason;
    RtlInitUnicodeString(&Message, lpMessage);

    //
    // Call the server
    //

    //
    // First try to connect to the new InitShutdown interface
    //
    Result = BaseBindToMachineShutdownInterface(lpMachineName,
                                                NewShutdownCallbackEx,
                                                &Message,
                                                &ShutdownContext);

    if( (Result != ERROR_SUCCESS) && (TryOld == TRUE) ) {
        //
        // try the old one, maybe we are calling into a NT4 machine
        // which doesn't know about the new interface
        //
        Result = BaseBindToMachine(lpMachineName,
                                   ShutdownCallbackEx,
                                   &Message,
                                   &ShutdownContext);
    }

    if (Result != ERROR_SUCCESS) {
        SetLastError(Result);
    }

    return(Result == ERROR_SUCCESS);
}


BOOL
APIENTRY
InitiateSystemShutdownA(
    IN LPSTR lpMachineName OPTIONAL,
    IN LPSTR lpMessage OPTIONAL,
    IN DWORD dwTimeout,
    IN BOOL bForceAppsClosed,
    IN BOOL bRebootAfterShutdown
    )

/*++

Routine Description:

    See InitiateSystemShutdownW

--*/

{
    UNICODE_STRING      MachineName;
    UNICODE_STRING      Message;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    BOOL                Result;

    //
    // Convert the ansi machinename to wide-character
    //

    RtlInitAnsiString( &AnsiString, lpMachineName );
    Status = RtlAnsiStringToUnicodeString(
                &MachineName,
                &AnsiString,
                TRUE
                );

    if( NT_SUCCESS( Status )) {

        //
        // Convert the ansi message to wide-character
        //

        RtlInitAnsiString( &AnsiString, lpMessage );
        Status = RtlAnsiStringToUnicodeString(
                    &Message,
                    &AnsiString,
                    TRUE
                    );

        if (NT_SUCCESS(Status)) {

            //
            // Call the wide-character api
            //

            Result = InitiateSystemShutdownW(
                                MachineName.Buffer,
                                Message.Buffer,
                                dwTimeout,
                                bForceAppsClosed,
                                bRebootAfterShutdown                                
                                );

            RtlFreeUnicodeString(&Message);
        }

        RtlFreeUnicodeString(&MachineName);
    }

    if (!NT_SUCCESS(Status)) {
        SetLastError(RtlNtStatusToDosError(Status));
        Result = FALSE;
    }

    return(Result);
}


BOOL
APIENTRY
InitiateSystemShutdownExA(
    IN LPSTR lpMachineName OPTIONAL,
    IN LPSTR lpMessage OPTIONAL,
    IN DWORD dwTimeout,
    IN BOOL bForceAppsClosed,
    IN BOOL bRebootAfterShutdown,
    IN DWORD dwReason
    )

/*++

Routine Description:

    See InitiateSystemShutdownW

--*/

{
    UNICODE_STRING      MachineName;
    UNICODE_STRING      Message;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    BOOL                Result;

    //
    // Convert the ansi machinename to wide-character
    //

    RtlInitAnsiString( &AnsiString, lpMachineName );
    Status = RtlAnsiStringToUnicodeString(
                &MachineName,
                &AnsiString,
                TRUE
                );

    if( NT_SUCCESS( Status )) {

        //
        // Convert the ansi message to wide-character
        //

        RtlInitAnsiString( &AnsiString, lpMessage );
        Status = RtlAnsiStringToUnicodeString(
                    &Message,
                    &AnsiString,
                    TRUE
                    );

        if (NT_SUCCESS(Status)) {

            //
            // Call the wide-character api
            //

            Result = InitiateSystemShutdownExW(
                                MachineName.Buffer,
                                Message.Buffer,
                                dwTimeout,
                                bForceAppsClosed,
                                bRebootAfterShutdown,
                                dwReason
                                );

            RtlFreeUnicodeString(&Message);
        }

        RtlFreeUnicodeString(&MachineName);
    }

    if (!NT_SUCCESS(Status)) {
        SetLastError(RtlNtStatusToDosError(Status));
        Result = FALSE;
    }

    return(Result);
}



BOOL
APIENTRY
AbortSystemShutdownW(
    IN LPWSTR lpMachineName OPTIONAL
    )

/*++

Routine Description:

    Win32 Unicode API for aborting the shutdown of a (possibly remote) machine.

Arguments:

    lpMachineName - Name of target machine.

Return Value:

    Returns TRUE on success, FALSE on failure (GetLastError() returns error code)

--*/

{
    DWORD   Result;
    RPC_BINDING_HANDLE binding;
    BOOL    TryOld = TRUE;

    //
    // Explicitly bind to the given server.
    //
    if (!ARGUMENT_PRESENT(lpMachineName)) {
        lpMachineName = L"";
        TryOld = FALSE;
    }

    //
    // Call the server
    //

    //
    // First try to connect to the new InitShutdown interface
    //
    Result = BaseBindToMachineShutdownInterface(lpMachineName,
                                                NewAbortShutdownCallback,
                                                NULL,
                                                NULL);

    if( (Result != ERROR_SUCCESS) && (TryOld == TRUE) ) {
        //
        // try the old one, maybe we are calling into a NT4 machine
        // which doesn't know about the new interface
        Result = BaseBindToMachine(lpMachineName,
                                   AbortShutdownCallback,
                                   NULL,
                                   NULL);
    }

    if (Result != ERROR_SUCCESS) {
        SetLastError(Result);
    }

    return(Result == ERROR_SUCCESS);
}



BOOL
APIENTRY
AbortSystemShutdownA(
    IN LPSTR lpMachineName OPTIONAL
    )

/*++

Routine Description:

    See AbortSystemShutdownW

--*/

{
    UNICODE_STRING      MachineName;
    ANSI_STRING         AnsiString;
    NTSTATUS            Status;
    BOOL                Result;

    //
    // Convert the ansi machinename to wide-character
    //

    RtlInitAnsiString( &AnsiString, lpMachineName );
    Status = RtlAnsiStringToUnicodeString(
                &MachineName,
                &AnsiString,
                TRUE
                );

    if( NT_SUCCESS( Status )) {

        //
        // Call the wide-character api
        //

        Result = AbortSystemShutdownW(
                            MachineName.Buffer
                            );

        RtlFreeUnicodeString(&MachineName);
    }


    if (!NT_SUCCESS(Status)) {
        SetLastError(RtlNtStatusToDosError(Status));
        Result = FALSE;
    }

    return(Result);
}

LONG
ShutdownCallback(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PUNICODE_STRING Message,
    IN PSHUTDOWN_CONTEXT ShutdownContext
    )
/*++

Routine Description:

    Callback for binding to a machine to initiate a shutdown.

Arguments:

    pbinding - Supplies a pointer to the RPC binding context

    Message - Supplies message to display during shutdown timeout period.

    ShutdownContext - Supplies remaining parameters for BaseInitiateSystemShutdown

Return Value:

    ERROR_SUCCESS if no error.

--*/

{
    DWORD Result;

    RpcTryExcept {
        Result = BaseInitiateSystemShutdown((PREGISTRY_SERVER_NAME)pbinding,
                                            Message,
                                            ShutdownContext->dwTimeout,
                                            ShutdownContext->bForceAppsClosed,
                                            ShutdownContext->bRebootAfterShutdown);
    } RpcExcept(EXCEPTION_EXECUTE_HANDLER) {
        Result = RpcExceptionCode();
    } RpcEndExcept;

    if (Result != ERROR_SUCCESS) {
        RpcBindingFree(pbinding);
    }
    return(Result);
}

LONG
ShutdownCallbackEx(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PUNICODE_STRING Message,
    IN PSHUTDOWN_CONTEXTEX ShutdownContext
    )
/*++

Routine Description:

    Callback for binding to a machine to initiate a shutdown.

Arguments:

    pbinding - Supplies a pointer to the RPC binding context

    Message - Supplies message to display during shutdown timeout period.

    ShutdownContext - Supplies remaining parameters for BaseInitiateSystemShutdown

Return Value:

    ERROR_SUCCESS if no error.

--*/

{
    DWORD Result;

    RpcTryExcept {
        Result = BaseInitiateSystemShutdownEx((PREGISTRY_SERVER_NAME)pbinding,
                                            Message,
                                            ShutdownContext->dwTimeout,
                                            ShutdownContext->bForceAppsClosed,
                                            ShutdownContext->bRebootAfterShutdown,
                                            ShutdownContext->dwReason);
    } RpcExcept(EXCEPTION_EXECUTE_HANDLER) {
        Result = RpcExceptionCode();
    } RpcEndExcept;

    if (Result != ERROR_SUCCESS) {
        RpcBindingFree(pbinding);
    }
    return(Result);
}

LONG
AbortShutdownCallback(
    IN RPC_BINDING_HANDLE *pbinding,
    IN PVOID Unused1,
    IN PVOID Unused2
    )
/*++

Routine Description:

    Callback for binding to a machine to abort a shutdown.

Arguments:

    pbinding - Supplies a pointer to the RPC binding context

Return Value:

    ERROR_SUCCESS if no error.

--*/

{
    DWORD Result;

    RpcTryExcept {
        Result = BaseAbortSystemShutdown((PREGISTRY_SERVER_NAME)pbinding);
    } RpcExcept(EXCEPTION_EXECUTE_HANDLER) {
        Result = RpcExceptionCode();
    } RpcEndExcept;

    if (Result != ERROR_SUCCESS) {
        RpcBindingFree(pbinding);
    }
    return(Result);
}


