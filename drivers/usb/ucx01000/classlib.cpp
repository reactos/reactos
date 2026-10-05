/*
 * PROJECT:     ReactOS USB Host Controller Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     KMDF class library registration and client binding
 * COPYRIGHT:   Copyright 2026 Justin Miller <justinmiller100@gmail.com>
 */

#include "ucx01000.h"

#define NDEBUG
#include <debug.h>

static PFN_UCXFUNC UcxExportTable[UcxFunctionTableNumEntries];

extern "C"
{
WDF_CLASS_LIBRARY_INFO UcxClassLibraryInfo =
{
    sizeof(UcxClassLibraryInfo),
    { UCX_CLASS_MAJOR_VERSION, UCX_CLASS_MINOR_VERSION, 0 },
    UcxOnClassLoad,
    UcxOnClassUnload,
    UcxOnClientBind,
    UcxOnClientUnbind
};
}

static
ULONG
NTAPI
UcxTableEntriesForMinor(
    _In_ ULONG Minor)
{
    if (Minor >= 4)
        return UcxFunctionTableNumEntries;

    /* SetIdStrings arrived in 1.3 */
    if (Minor == 3)
        return UcxControllerSetIdStringsTableIndex + 1;

    return UcxInitializeDeviceInitTableIndex + 1;
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxOnClassLoad(VOID)
{
    PFN_UCXFUNC *Table = UcxExportTable;

    PAGED_CODE();

    Table[UcxIoDeviceControlTableIndex] = (PFN_UCXFUNC)UcxApiIoDeviceControl;
    Table[UcxControllerCreateTableIndex] = (PFN_UCXFUNC)UcxApiControllerCreate;
    Table[UcxControllerNeedsResetTableIndex] = (PFN_UCXFUNC)UcxApiControllerNeedsReset;
    Table[UcxControllerResetCompleteTableIndex] = (PFN_UCXFUNC)UcxApiControllerResetComplete;
    Table[UcxControllerSetFailedTableIndex] = (PFN_UCXFUNC)UcxApiControllerSetFailed;
    Table[UcxRootHubCreateTableIndex] = (PFN_UCXFUNC)UcxApiRootHubCreate;
    Table[UcxRootHubPortChangedTableIndex] = (PFN_UCXFUNC)UcxApiRootHubPortChanged;
    Table[UcxUsbDeviceCreateTableIndex] = (PFN_UCXFUNC)UcxApiUsbDeviceCreate;
    Table[UcxUsbDeviceInitSetEventCallbacksTableIndex] = (PFN_UCXFUNC)UcxApiUsbDeviceInitSetEventCallbacks;
    Table[UcxUsbDeviceRemoteWakeNotificationTableIndex] = (PFN_UCXFUNC)UcxApiUsbDeviceRemoteWakeNotification;
    Table[UcxEndpointCreateTableIndex] = (PFN_UCXFUNC)UcxApiEndpointCreate;
    Table[UcxEndpointGetStaticStreamsReferencedTableIndex] = (PFN_UCXFUNC)UcxApiEndpointGetStaticStreamsReferenced;
    Table[UcxEndpointNeedToCancelTransfersTableIndex] = (PFN_UCXFUNC)UcxApiEndpointNeedToCancelTransfers;
    Table[UcxEndpointInitSetEventCallbacksTableIndex] = (PFN_UCXFUNC)UcxApiEndpointInitSetEventCallbacks;
    Table[UcxDefaultEndpointInitSetEventCallbacksTableIndex] = (PFN_UCXFUNC)UcxApiDefaultEndpointInitSetEventCallbacks;
    Table[UcxEndpointSetWdfIoQueueTableIndex] = (PFN_UCXFUNC)UcxApiEndpointSetWdfIoQueue;
    Table[UcxEndpointPurgeCompleteTableIndex] = (PFN_UCXFUNC)UcxApiEndpointPurgeComplete;
    Table[UcxEndpointAbortCompleteTableIndex] = (PFN_UCXFUNC)UcxApiEndpointAbortComplete;
    Table[UcxEndpointNoPingResponseErrorTableIndex] = (PFN_UCXFUNC)UcxApiEndpointNoPingResponseError;
    Table[UcxStaticStreamsSetStreamInfoTableIndex] = (PFN_UCXFUNC)UcxApiStaticStreamsSetStreamInfo;
    Table[UcxStaticStreamsCreateTableIndex] = (PFN_UCXFUNC)UcxApiStaticStreamsCreate;
    Table[UcxInitializeDeviceInitTableIndex] = (PFN_UCXFUNC)UcxApiInitializeDeviceInit;
    Table[UcxControllerSetIdStringsTableIndex] = (PFN_UCXFUNC)UcxApiControllerSetIdStrings;
    Table[UcxControllerNotifyTransportCharacteristicsChangeTableIndex] =
        (PFN_UCXFUNC)UcxApiControllerNotifyTransportCharacteristicsChange;

    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
UcxOnClassUnload(VOID)
{
    PAGED_CODE();
}

_IRQL_requires_(PASSIVE_LEVEL)
NTSTATUS
NTAPI
UcxOnClientBind(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _Out_ PWDF_COMPONENT_GLOBALS *ComponentGlobals)
{
    PUCX_DRIVER_GLOBALS *ClientGlobals;
    PUCX_DRIVER_GLOBALS Globals;
    ULONG Minor;

    PAGED_CODE();

    ClientGlobals = (PUCX_DRIVER_GLOBALS *)ClassBindInfo->ClassBindInfo;
    if (ClientGlobals == NULL)
        return STATUS_INVALID_PARAMETER;

    /* KMDF unbinds even after a failed bind, so clear the slot up front */
    *ClientGlobals = NULL;

    Minor = ClassBindInfo->Version.Minor;
    if (ClassBindInfo->Version.Major != UCX_CLASS_MAJOR_VERSION || Minor > UCX_CLASS_MINOR_VERSION)
    {
        DPRINT1("Client wants UCX %lu.%lu, have %u.%u\n",
                ClassBindInfo->Version.Major,
                Minor,
                UCX_CLASS_MAJOR_VERSION,
                UCX_CLASS_MINOR_VERSION);
        return STATUS_REVISION_MISMATCH;
    }

    if (ClassBindInfo->FunctionTableCount != UcxTableEntriesForMinor(Minor))
    {
        DPRINT1("UCX 1.%lu client has %lu table slots\n", Minor, ClassBindInfo->FunctionTableCount);
        return STATUS_INVALID_PARAMETER;
    }

    Globals = (PUCX_DRIVER_GLOBALS)ExAllocatePoolZero(NonPagedPool, sizeof(*Globals), UCX_POOL_TAG);
    if (Globals == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    Globals->Size = sizeof(*Globals);
    Globals->WdfDriverGlobals = (PWDF_DRIVER_GLOBALS)ComponentGlobals;

    RtlCopyMemory(ClassBindInfo->FunctionTable,
                  UcxExportTable,
                  ClassBindInfo->FunctionTableCount * sizeof(UcxExportTable[0]));

    *ClientGlobals = Globals;
    return STATUS_SUCCESS;
}

_IRQL_requires_(PASSIVE_LEVEL)
VOID
NTAPI
UcxOnClientUnbind(
    _In_ PWDF_CLASS_BIND_INFO ClassBindInfo,
    _In_ PWDF_COMPONENT_GLOBALS *ComponentGlobals)
{
    PUCX_DRIVER_GLOBALS *ClientGlobals;

    PAGED_CODE();
    UNREFERENCED_PARAMETER(ComponentGlobals);

    ClientGlobals = (PUCX_DRIVER_GLOBALS *)ClassBindInfo->ClassBindInfo;
    if (ClientGlobals == NULL || *ClientGlobals == NULL)
        return;

    ExFreePoolWithTag(*ClientGlobals, UCX_POOL_TAG);
    *ClientGlobals = NULL;
}
