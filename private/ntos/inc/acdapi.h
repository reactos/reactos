/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    acdapi.h

Abstract:

    Exported routines from the Implicit Connection
    Driver (rasacd.sys) for use by transports to allow a
    user space process to dynamically create a network
    connection upon a network unreachable error.

Author:

    Anthony Discolo (adiscolo)  17-Apr-1995

Environment:

    Kernel Mode

Revision History:

--*/

#ifndef _ACDAPI_
#define _ACDAPI_

//
// A callback from the automatic connection
// driver to the transport to continue the
// connection process.
//
typedef VOID (*ACD_CONNECT_CALLBACK)(
    IN BOOLEAN,
    IN PVOID *);

typedef VOID
(*ACD_NEWCONNECTION)(
    IN PACD_ADDR pszAddr,
    IN PACD_ADAPTER pAdapter
    );

typedef BOOLEAN
(*ACD_STARTCONNECTION)(
    IN ULONG ulDriverId,
    IN PACD_ADDR pszAddr,
    IN ULONG ulFlags,
    IN ACD_CONNECT_CALLBACK pProc,
    IN USHORT nArgs,
    IN PVOID *pArgs
    );

//
// A callback to allow the caller
// to rummage around in the parameters
// to find the right request to cancel.
// To cancel the connection, the
// ACD_CANCEL_CALLBACK routine returns
// TRUE.
//
typedef BOOLEAN (*ACD_CANCEL_CALLBACK)(
    IN PVOID pArg,
    IN ULONG ulFlags,
    IN ACD_CONNECT_CALLBACK pProc,
    IN USHORT nArgs,
    IN PVOID *pArgs
    );

typedef BOOLEAN
(*ACD_CANCELCONNECTION)(
    IN ULONG ulDriverId,
    IN PACD_ADDR pszAddr,
    IN ACD_CANCEL_CALLBACK pProc,
    IN PVOID pArg
    );

//
// The structure a transport client receives
// when it binds (IOCTL_INTERNAL_ACD_BIND) with the driver.
//
typedef struct {
    LIST_ENTRY ListEntry;
    //
    // Provided by the transport.
    //
    KSPIN_LOCK SpinLock;
    ULONG ulDriverId;
    //
    // Filled in by rasacd.sys.
    //
    BOOLEAN fEnabled;
    ACD_NEWCONNECTION lpfnNewConnection;
    ACD_STARTCONNECTION lpfnStartConnection;
    ACD_CANCELCONNECTION lpfnCancelConnection;
} ACD_DRIVER, *PACD_DRIVER;

//
// Internal IOCTL definitions
//
#define IOCTL_INTERNAL_ACD_BIND  \
            _ACD_CTL_CODE(0x1234, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_INTERNAL_ACD_UNBIND  \
            _ACD_CTL_CODE(0x1235, METHOD_BUFFERED, FILE_ANY_ACCESS)

#define IOCTL_INTERNAL_ACD_QUERY_STATE \
            _ACD_CTL_CODE(0x1236, METHOD_BUFFERED, FILE_ANY_ACCESS)

#endif  // ifndef _ACDAPI_
