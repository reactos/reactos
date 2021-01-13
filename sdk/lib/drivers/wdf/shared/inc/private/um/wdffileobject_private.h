/*++

Copyright (c) Microsoft Corporation.  All rights reserved.

Module Name:

    wdffileobject_private.h

Abstract:

    Defines private DDIs for WDF File Object

Environment:

    User mode

Revision History:

--*/

#ifndef _WDFFILEOBJECT_PRIVATE_H_
#define _WDFFILEOBJECT_PRIVATE_H_

//
// Interface available through WdfObjectQuery
//
// a4870f73-7c63-4e8d-ab57-ab191b522aca
DEFINE_GUID(GUID_WDFP_FILEOBJECT_INTERFACE,
    0xa4870f73, 0x7c63, 0x4e8d, 0xab, 0x57, 0xab, 0x19, 0x1b, 0x52, 0x2a, 0xca);

typedef
NTSTATUS
(*PFN_WDFP_FILEOBJECT_INCREMENT_PROCESS_KEEP_ALIVE_COUNT)(
    _In_ PWDF_DRIVER_GLOBALS WdfDriverGlobals,
    _In_ WDFFILEOBJECT FileObject
    );

typedef
NTSTATUS
(*PFN_WDFP_FILEOBJECT_DECREMENT_PROCESS_KEEP_ALIVE_COUNT)(
    _In_ PWDF_DRIVER_GLOBALS WdfDriverGlobals,
    _In_ WDFFILEOBJECT FileObject
    );

//
// Structure passed in by the driver and populated by WdfObjectQuery
//
typedef struct _WDFP_FILEOBJECT_INTERFACE {

    //
    // Size of this structure in bytes
    //
    ULONG Size;

    //
    // Private DDIs exposed through WdfObjectQuery
    //
    PFN_WDFP_FILEOBJECT_INCREMENT_PROCESS_KEEP_ALIVE_COUNT
        WdfpFileObjectIncrementProcessKeepAliveCount;

    PFN_WDFP_FILEOBJECT_DECREMENT_PROCESS_KEEP_ALIVE_COUNT
        WdfpFileObjectDecrementProcessKeepAliveCount;

} WDFP_FILEOBJECT_INTERFACE, *PWDFP_FILEOBJECT_INTERFACE;

//
// Used by a driver to initialize this structure
//
VOID
__inline
WDFP_FILEOBJECT_INTERFACE_INIT(
    _Out_ PWDFP_FILEOBJECT_INTERFACE FileObjectInterface
    )
{
    RtlZeroMemory(FileObjectInterface, sizeof(WDFP_FILEOBJECT_INTERFACE));

    FileObjectInterface->Size = sizeof(WDFP_FILEOBJECT_INTERFACE);
}

#endif // _WDFFILEOBJECT_PRIVATE_H_
