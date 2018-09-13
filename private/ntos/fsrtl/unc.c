/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    unc.c

Abstract:

    This file contains functions to support multiple UNC providers
    on a single NT machine.

Author:

    Manny Weiser     [MannyW]    20-Dec-1991

Revision History:

    Isaac Heizer     [IsaacHe]   16-Nov-1994  Defer loading the MUP
                                              Rewrite

    Milan Shah       [MilanS]    7-Mar-1996   Check for Dfs client status
                                              before loading the MUP

--*/

#include "fsrtlp.h"
#include <zwapi.h>
#include <ntddmup.h>
#include <ntddnull.h>

static WCHAR MupRegKey[] = L"\\Registry\\Machine\\System\\CurrentControlSet\\Services\\Mup";
static WCHAR UNCSymbolicLink[] = L"\\DosDevices\\UNC";
static WCHAR DevNull[] = L"\\Device\\Null";
static WCHAR DevMup[] = DD_MUP_DEVICE_NAME;

//
//  Define a tag for general pool allocations from this module
//

#undef MODULE_POOL_TAG
#define MODULE_POOL_TAG                  ('nuSF')

//
// Local prototypes
//

NTSTATUS
FsRtlpRegisterProviderWithMUP
(
    IN HANDLE mupHandle,
    IN PUNICODE_STRING RedirDevName,
    IN BOOLEAN MailslotsSupported
);

NTSTATUS
FsRtlpOpenDev(
    IN OUT PHANDLE Handle,
    IN LPWSTR DevNameStr
);

VOID
FsRtlpSetSymbolicLink(
    IN PUNICODE_STRING DevName OPTIONAL
);

BOOLEAN
FsRtlpIsDfsEnabled();

#ifdef ALLOC_PRAGMA
#pragma alloc_text(PAGE, FsRtlpRegisterProviderWithMUP)
#pragma alloc_text(PAGE, FsRtlpOpenDev)
#pragma alloc_text(PAGE, FsRtlpSetSymbolicLink)
#pragma alloc_text(PAGE, FsRtlDeregisterUncProvider)
#pragma alloc_text(PAGE, FsRtlRegisterUncProvider)
#endif

//
// We defer calling the MUP with the registration data until
//   the second redir loads and Dfs is disabled.  This structure holds the
//   data necessary to make that call.
//
struct {
    HANDLE MupHandle;
    HANDLE ReturnedHandle;
    UNICODE_STRING RedirDevName;
    BOOLEAN MailslotsSupported;
} FsRtlpDRD = {0};

//
// Resource protection
//
KSEMAPHORE FsRtlpUncSemaphore;

//
// Number of times we've loaded redirs.
//
ULONG FsRtlpRedirs = 0;


NTSTATUS
FsRtlpRegisterProviderWithMUP
(
    IN HANDLE mupHandle,
    IN PUNICODE_STRING RedirDevName,
    IN BOOLEAN MailslotsSupported
)
/*++

Routine Description:

    This private routine does the FSCTL to the MUP to tell it about
        a new redir

Arguments:

    mupHandle - Handle to the MUP

    RedirDevName - The device name of the redir.

    MailslotsSupported - If TRUE, this redir supports mailslots.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS status;
    IO_STATUS_BLOCK ioStatusBlock;
    ULONG paramLength;
    PREDIRECTOR_REGISTRATION params;

    PAGED_CODE();

    paramLength = sizeof( REDIRECTOR_REGISTRATION ) +
                      RedirDevName->Length;

    params = ExAllocatePoolWithTag( NonPagedPool, paramLength, MODULE_POOL_TAG );
    if( params == NULL )
        return STATUS_INSUFFICIENT_RESOURCES;

    params->DeviceNameOffset = sizeof( REDIRECTOR_REGISTRATION );
    params->DeviceNameLength = RedirDevName->Length;
    params->MailslotsSupported = MailslotsSupported;

    RtlMoveMemory(
        (PCHAR)params + params->DeviceNameOffset,
        RedirDevName->Buffer,
        RedirDevName->Length
        );

    status = NtFsControlFile(
                 mupHandle,
                 0,
                 NULL,
                 NULL,
                 &ioStatusBlock,
                 FSCTL_MUP_REGISTER_UNC_PROVIDER,
                 params,
                 paramLength,
                 NULL,
                 0
                 );

    if ( status == STATUS_PENDING ) {
        status = NtWaitForSingleObject( mupHandle, TRUE, NULL );
    }

    if ( NT_SUCCESS( status ) ) {
        status = ioStatusBlock.Status;
    }

    ASSERT( NT_SUCCESS( status ) );

    ExFreePool( params );

    return status;
}

NTSTATUS
FsRtlpOpenDev(
    IN OUT PHANDLE Handle,
    IN LPWSTR DevNameStr
)
{
    NTSTATUS status;
    UNICODE_STRING DevName;
    OBJECT_ATTRIBUTES objectAttributes;
    IO_STATUS_BLOCK ioStatusBlock;

    PAGED_CODE();

    RtlInitUnicodeString( &DevName, DevNameStr );

    InitializeObjectAttributes(
        &objectAttributes,
        &DevName,
        0,
        0,
        NULL
        );

    status = ZwCreateFile(
                 Handle,
                 GENERIC_WRITE,
                 &objectAttributes,
                 &ioStatusBlock,
                 NULL,
                 FILE_ATTRIBUTE_NORMAL,
                 FILE_SHARE_READ | FILE_SHARE_WRITE,
                 FILE_OPEN,
                 0,
                 NULL,
                 0
                 );

    if ( NT_SUCCESS( status ) ) {
        status = ioStatusBlock.Status;
    }

    if( !NT_SUCCESS( status ) ) {
        *Handle = (HANDLE)-1;
    }

    return status;
}

VOID
FsRtlpSetSymbolicLink( IN PUNICODE_STRING DevName OPTIONAL )
{
    NTSTATUS status;
    UNICODE_STRING UncSymbolicName;

    PAGED_CODE();

    RtlInitUnicodeString( &UncSymbolicName, UNCSymbolicLink );
    (VOID)IoDeleteSymbolicLink( &UncSymbolicName );
    if( ARGUMENT_PRESENT( DevName ) ) {
        status = IoCreateSymbolicLink( &UncSymbolicName, DevName );
        ASSERT( NT_SUCCESS( status ) );
    }
}

NTSTATUS
FsRtlRegisterUncProvider(
    IN OUT PHANDLE MupHandle,
    IN PUNICODE_STRING RedirDevName,
    IN BOOLEAN MailslotsSupported
    )
/*++

Routine Description:

    This routine registers a redir as a UNC provider.

Arguments:

    Handle - Pointer to a handle.  The handle is returned by the routine
        to be used when calling FsRtlDeregisterUncProvider.
        It is valid only if the routines returns STATUS_SUCCESS.

    RedirDevName - The device name of the redir.

    MailslotsSupported - If TRUE, this redir supports mailslots.

Return Value:

    NTSTATUS - The status of the operation.

--*/
{
    NTSTATUS status;
    HANDLE mupHandle = (HANDLE)-1;
    UNICODE_STRING mupDriverName;
    BOOLEAN dfsEnabled;

    PAGED_CODE();

    KeWaitForSingleObject(&FsRtlpUncSemaphore, Executive, KernelMode, FALSE, NULL );

    if (FsRtlpRedirs == 0) {

        dfsEnabled = FsRtlpIsDfsEnabled();

        if (dfsEnabled) {
            FsRtlpRedirs = 1;
            RtlZeroMemory((PVOID) &FsRtlpDRD, sizeof(FsRtlpDRD));
        }

    }

    switch( FsRtlpRedirs ) {
    case 0:
        //
        // Ok, the MUP isn't there and we don't need to use the
        //   MUP for the first redir.
        //
        // We need to return a handle, but we're not really using the MUP yet.
        //   And we may never use it (if there's only 1 redir).  Return
        //   a handle to the NULL device object, since we're committed to returning
        //   a handle to our caller.  Our caller isn't supposed to do anything with
        //   the handle except to call FsRtlDeregisterUncProvider() with it.
        //
        status = FsRtlpOpenDev( &mupHandle, DevNull );

        if( !NT_SUCCESS( status ) )
            break;

        //
        // Save up enough state to allow us to call the MUP later with
        // this registration info if necessary.
        //
        FsRtlpDRD.RedirDevName.Buffer = ExAllocatePoolWithTag( NonPagedPool, 
                                                               RedirDevName->MaximumLength, 
                                                               MODULE_POOL_TAG );

        if( FsRtlpDRD.RedirDevName.Buffer == NULL ) {
            status =  STATUS_INSUFFICIENT_RESOURCES;
            break;
        }

        FsRtlpDRD.RedirDevName.Length = RedirDevName->Length;
        FsRtlpDRD.RedirDevName.MaximumLength = RedirDevName->MaximumLength;

        RtlMoveMemory(
                (PCHAR)FsRtlpDRD.RedirDevName.Buffer,
                RedirDevName->Buffer,
                RedirDevName->MaximumLength
        );

        FsRtlpDRD.MailslotsSupported = MailslotsSupported;
        FsRtlpDRD.ReturnedHandle = mupHandle;
        FsRtlpDRD.MupHandle = (HANDLE)-1;

        //
        // Set the UNC symbolic link to point to the redir we just loaded
        //
        FsRtlpSetSymbolicLink( RedirDevName );

        break;

    default:
        //
        // This is the second or later redir load -- MUST use the MUP
        //
        status = FsRtlpOpenDev( &mupHandle, DevMup );

        if( !NT_SUCCESS( status ) ) {

            RtlInitUnicodeString( &mupDriverName, MupRegKey );

            (VOID)ZwLoadDriver( &mupDriverName );

            status = FsRtlpOpenDev( &mupHandle, DevMup );
            if( !NT_SUCCESS( status ) )
                break;
        }

        //
        // See if we need to tell the MUP about the first redir that registered
        //
        if( FsRtlpDRD.RedirDevName.Buffer ) {

            status = FsRtlpRegisterProviderWithMUP( mupHandle,
                    &FsRtlpDRD.RedirDevName,
                    FsRtlpDRD.MailslotsSupported );

            if( !NT_SUCCESS( status ) )
                break;

            FsRtlpDRD.MupHandle = mupHandle;

            ExFreePool( FsRtlpDRD.RedirDevName.Buffer );
            FsRtlpDRD.RedirDevName.Buffer = NULL;

            //
            // Set the UNC symbolic link to point to the MUP
            //
            RtlInitUnicodeString(  &mupDriverName, DevMup );
            FsRtlpSetSymbolicLink( &mupDriverName );

            status = FsRtlpOpenDev( &mupHandle, DevMup );

            if( !NT_SUCCESS( status ) )
                break;
        }

        //
        //  Pass the request to the MUP for this redir
        //
        status = FsRtlpRegisterProviderWithMUP( mupHandle,
                        RedirDevName,
                        MailslotsSupported );
        break;

    }

    if( NT_SUCCESS( status ) ) {
        FsRtlpRedirs++;
        *MupHandle = mupHandle;

    } else {
        if( mupHandle != (HANDLE)-1 && mupHandle != NULL ) {
            ZwClose( mupHandle );
        }

        *MupHandle = (HANDLE)-1;
    }

    KeReleaseSemaphore(&FsRtlpUncSemaphore, 0, 1, FALSE );
    return status;
}


VOID
FsRtlDeregisterUncProvider(
    IN HANDLE Handle
    )

/*++

Routine Description:

    This routine deregisters a redir as a UNC provider.

Arguments:

    Handle - A handle to the Multiple UNC router, returned by the
        registration call.

Return Value:

    None.

--*/

{
    NTSTATUS status;

    PAGED_CODE();

    if( Handle == (HANDLE)-1 || Handle == NULL )
        return;

    status = ZwClose( Handle );

    if( !NT_SUCCESS( status ) ) {
        return;
    }

    KeWaitForSingleObject(&FsRtlpUncSemaphore, Executive, KernelMode, FALSE, NULL );

    ASSERT( FsRtlpRedirs > 0 );

    if( Handle == FsRtlpDRD.ReturnedHandle ) {

        //
        // The first redir in the system is closing.  Release the state we saved
        //  for it, and pass the close on to the MUP if necessary
        //

        if( FsRtlpDRD.RedirDevName.Buffer != NULL ) {
            ExFreePool( FsRtlpDRD.RedirDevName.Buffer );
            FsRtlpDRD.RedirDevName.Buffer = NULL;
        }

        if( FsRtlpDRD.MupHandle != (HANDLE)-1 ) {
            ZwClose( FsRtlpDRD.MupHandle );
            FsRtlpDRD.MupHandle = (HANDLE)-1;
        }

        FsRtlpDRD.ReturnedHandle = (HANDLE)-1;

    }

    if( --FsRtlpRedirs == 0 ) {
        FsRtlpSetSymbolicLink( (PUNICODE_STRING)NULL );
    }

    KeReleaseSemaphore(&FsRtlpUncSemaphore, 0, 1, FALSE );
}


BOOLEAN
FsRtlpIsDfsEnabled()

/*++

Routine Description:

    This routine checks a registry key to see if the Dfs client is enabled.
    The client is assumed to be enabled by default, and disabled only if there
    is a registry value indicating that it should be disabled.

Arguments:

    None

Return Value:

    TRUE if Dfs client is enabled, FALSE otherwise.

--*/

{
    NTSTATUS status;
    HANDLE mupRegHandle;
    OBJECT_ATTRIBUTES objectAttributes;
    ULONG valueSize;
    BOOLEAN dfsEnabled = TRUE;

    UNICODE_STRING mupRegKey = {
        sizeof(MupRegKey) - sizeof(WCHAR),
        sizeof(MupRegKey),
        MupRegKey};

#define DISABLE_DFS_VALUE_NAME  L"DisableDfs"

    UNICODE_STRING disableDfs = {
        sizeof(DISABLE_DFS_VALUE_NAME) - sizeof(WCHAR),
        sizeof(DISABLE_DFS_VALUE_NAME),
        DISABLE_DFS_VALUE_NAME};

    struct {
        KEY_VALUE_PARTIAL_INFORMATION Info;
        ULONG Buffer;
    } disableDfsValue;


    InitializeObjectAttributes(
        &objectAttributes,
        &mupRegKey,
        OBJ_CASE_INSENSITIVE,
        0,
        NULL
        );

    status = ZwOpenKey(&mupRegHandle, KEY_READ, &objectAttributes);

    if (NT_SUCCESS(status)) {

        status = ZwQueryValueKey(
                    mupRegHandle,
                    &disableDfs,
                    KeyValuePartialInformation,
                    (PVOID) &disableDfsValue,
                    sizeof(disableDfsValue),
                    &valueSize);

        if (NT_SUCCESS(status) && disableDfsValue.Info.Type == REG_DWORD) {

            if ( (*((PULONG) disableDfsValue.Info.Data)) == 1 )
                dfsEnabled = FALSE;

        }

        ZwClose( mupRegHandle );

    }

    return( dfsEnabled );

}
