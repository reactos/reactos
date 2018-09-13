/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    apc.c

Abstract:

    This module implements the APC helper functions for the WinSock 2.0
    helper library. This particular implementation is specific to NT 3.51.

Author:

    Keith Moore (keithmo)       20-Jun-1995

Revision History:

--*/


#include "precomp.h"


VOID
WINAPI
WahpIntermediateApc(
    LPVOID Context,
    LPVOID SystemArgument1,
    LPVOID SystemArgument2
    );


//
//  Public functions.
//


DWORD
WINAPI
WahOpenApcHelper(
    OUT LPHANDLE HelperHandle
    )

/*++

Routine Description:

    This routine opens the WinSock 2.0 APC helper device.

Arguments:

    HelperHandle - Points to a HANDLE that will receive an open handle
        to the APC helper device.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    NTSTATUS status;
    OBJECT_ATTRIBUTES objectAttributes;
    UNICODE_STRING helperName;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    //  Validate parameters.
    //

    if( HelperHandle == NULL ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Open the helper device.
    //

    RtlInitUnicodeString( &helperName, L"\\Device\\Afd\\Helper" );

    InitializeObjectAttributes(
        &objectAttributes,                      // ObjectAttributes
        &helperName,                            // ObjectName
        OBJ_CASE_INSENSITIVE,                   // Attributes
        NULL,                                   // RootDirectory
        NULL                                    // SecurityDescriptor
        );

    status = NtCreateFile(
                HelperHandle,                   // FileHandle
                GENERIC_READ |                  // DesiredAccess
                    GENERIC_WRITE |
                    SYNCHRONIZE,
                &objectAttributes,              // ObjectAttributes
                &ioStatusBlock,                 // IoStatusBlock
                NULL,                           // AllocationSize
                0,                              // FileAttributes
                FILE_SHARE_READ |               // ShareAccess
                    FILE_SHARE_WRITE,
                FILE_OPEN_IF,                   // CreateDisposition
                FILE_SYNCHRONOUS_IO_NONALERT,   // CreateOptions
                NULL,                           // EaBuffer
                0                               // EaLength
                );

    if( NT_SUCCESS(status) ) {

        return NO_ERROR;

    }

    return RtlNtStatusToDosError( status );

}   // WahOpenApcHelper


DWORD
WINAPI
WahCloseApcHelper(
    IN HANDLE HelperHandle
    )

/*++

Routine Description:

    This function closes the WinSock 2.0 APC helper device.

Arguments:

    HelperHandle - The handle to close.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    NTSTATUS status;

    //
    //  Validate parameters.
    //

    if( HelperHandle == NULL ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Close the handle.
    //

    status = NtClose( HelperHandle );

    if( NT_SUCCESS(status) ) {

        return NO_ERROR;

    }

    return RtlNtStatusToDosError( status );

}   // WahCloseApcHelper


DWORD
WINAPI
WahOpenCurrentThread(
    IN  HANDLE HelperHandle,
    OUT LPWSATHREADID ThreadId
    )

/*++

Routine Description:

    This function opens a handle to the current thread.

Arguments:

    HelperHandle - An open handle to the APC helper device.

    ThreadId - Points to a WSATHREADID structure that will receive
        an open handle to the current thread and an (optional) OS-
        dependent thread identifier.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    NTSTATUS status;

    //
    //  Validate parameters.
    //

    if( ( HelperHandle == NULL ) ||
        ( ThreadId == NULL ) ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Duplicate the current thread pseudo handle.
    //

    status = NtDuplicateObject(
                NtCurrentProcess(),                 // SourceProcessHandle
                NtCurrentThread(),                  // SourceHandle
                NtCurrentProcess(),                 // TargetProcessHandle
                &ThreadId->ThreadHandle,            // TargetHandle
                0,                                  // DesiredAccess
                0,                                  // HandleAttributes
                DUPLICATE_SAME_ACCESS               // Options
                );

    if( NT_SUCCESS(status) ) {

        //
        //  The NT implementation of the APC helper does not really
        //  need the OS-dependent thread identifier, but we'll store
        //  the current thread ID in the structure just for completeness.
        //

        ThreadId->Reserved = (DWORD)NtCurrentTeb()->ClientId.UniqueThread;

        return NO_ERROR;

    }

    return RtlNtStatusToDosError( status );

}   // WahOpenCurrentThread


DWORD
WINAPI
WahCloseThread(
    IN HANDLE HelperHandle,
    IN LPWSATHREADID ThreadId
    )

/*++

Routine Description:

    This routine closes an open thread handle.

Arguments:

    HelperHandle - An open handle to the APC helper device.

    ThreadId - Points to a WSATHREADID structure initialized by a
        previous call to WahOpenCurrentThread().

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    NTSTATUS status;

    UNREFERENCED_PARAMETER( HelperHandle );

    //
    //  Validate parameters.
    //

    if( ( ThreadId == NULL ) ||
        ( ThreadId->ThreadHandle == NULL ) ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Close the handle.
    //

    status = NtClose( ThreadId->ThreadHandle );

    if( NT_SUCCESS(status) ) {

        //
        //  Clear the fields in case the client tries something stupid.
        //

        ThreadId->ThreadHandle = NULL;
        ThreadId->Reserved = 0;

        return NO_ERROR;

    }

    return RtlNtStatusToDosError( status );

}   // WahCloseThread


DWORD
WINAPI
WahQueueUserApc(
    IN HANDLE HelperHandle,
    IN LPWSATHREADID ThreadId,
    IN LPWSAUSERAPC ApcRoutine,
    IN DWORD ApcContext OPTIONAL
    )

/*++

Routine Description:

    This routine queues a user-mode APC for the specified thread.

Arguments:

    HelperHandle - An open handle to the APC helper device.

    ThreadId - Points to a WSATHREADID structure initialized by a
        previous call to WahOpenCurrentThread().

    ApcRoutine - Points to the APC code to execute when the specified
        thread enters an alertable wait.

    ApcContext - An uninterpreted context value to pass to the APC routine.

Return Value:

    DWORD - NO_ERROR if successful, a Win32 error code if not.

--*/

{

    NTSTATUS status;
    AFD_QUEUE_APC_INFO apcInfo;
    IO_STATUS_BLOCK ioStatusBlock;

    //
    //  Validate parameters.
    //

    if( ( HelperHandle == NULL ) ||
        ( ThreadId == NULL ) ||
        ( ThreadId->ThreadHandle == NULL ) ||
        ( ApcRoutine == NULL ) ) {

        return ERROR_INVALID_PARAMETER;

    }

    //
    //  Ask the helper to queue the APC.  We'll actually schedule
    //  WahpIntermediateApc as the "real" APC, which will unpack
    //  the parameters and call the routine specified by ApcRoutine.
    //

    apcInfo.Thread = ThreadId->ThreadHandle;
    apcInfo.ApcRoutine = WahpIntermediateApc;
    apcInfo.ApcContext = (PVOID)ApcContext;
    apcInfo.SystemArgument1 = ApcRoutine;
    apcInfo.SystemArgument2 = NULL;

    status = NtDeviceIoControlFile(
                HelperHandle,                       // FileHandle
                NULL,                               // Event
                NULL,                               // ApcRoutine
                NULL,                               // ApcContext
                &ioStatusBlock,                     // IoStatusBlock
                IOCTL_AFD_QUEUE_APC,                // IoControlCode
                &apcInfo,                           // InputBuffer
                sizeof(apcInfo),                    // InputBufferLength
                NULL,                               // OutputBuffer
                0                                   // OutputBufferLength
                );

    if( NT_SUCCESS(status) ) {

        return NO_ERROR;

    }

    return RtlNtStatusToDosError( status );

}   // WahQueueUserApc



//
//  Private functions.
//

VOID
WINAPI
WahpIntermediateApc(
    LPVOID Context,
    LPVOID SystemArgument1,
    LPVOID SystemArgument2
    )

/*++

Routine Description:

    This is the "real" APC as scheduled by the NT kernel.  It packages
    the parameters into the "portable" (with Win95) form, then calls the
    user-specified APC function.

Arguments:

    Context - The APC context.  This is just the ApcContext as passed
        into WahQueueUserApc.

    SystemArgument1 - This points to the "portable" APC function (this
        is the ApcRoutine as passed into WahQueueUserApc).

    SystemArgument2 - Unused.

Return Value:

    None.

--*/

{

    UNREFERENCED_PARAMETER( SystemArgument2 );

    //
    //  Just call through to the user's APC function.
    //

    ((LPWSAUSERAPC)SystemArgument1)( (DWORD)Context );

}   // WahpIntermediateApc

