/*
 * PROJECT:     ReactOS Intel KDNET Extension
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     KDNet extensibility implementation
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

#include "kd8086.h"

PKDNET_EXTENSIBILITY_IMPORTS KdNetExtensibilityImports;

VOID
DriverEntry(
    _In_ PDRIVER_OBJECT DriverObject,
    _In_ PUNICODE_STRING RegistryPath
    )
{
    UNREFERENCED_PARAMETER(DriverObject);
    UNREFERENCED_PARAMETER(RegistryPath);
}

VOID
KdSetHibernateRange(
    VOID
    )
{
    //TODO:
}

NTSTATUS
NTAPI
KdInitializeLibrary(
    _In_ PKDNET_EXTENSIBILITY_IMPORTS ImportTable,
    _In_opt_ PCHAR LoaderOptions,
    _Inout_ struct _DEBUG_DEVICE_DESCRIPTOR *Device
    )
{
    PKDNET_EXTENSIBILITY_EXPORTS Exports;
    NTSTATUS Status = STATUS_SUCCESS;
    
    KdNetExtensibilityImports = ImportTable;
    Exports = KdNetExtensibilityImports->Exports;

    if (!KdNetExtensibilityImports || !KdNetExtensibilityImports->KdNetDbgPrintf)
        return STATUS_INVALID_PARAMETER;
    
    KdNetExtensibilityImports->KdNetDbgPrintf("KdInitializeLibrary: ExportsFnCount=%lu Expected=%lu\n",
                                                      ImportTable->Exports->FunctionCount,
                                                      KDNET_EXT_EXPORTS);
    KdNetExtensibilityImports->KdNetDbgPrintf("KdInitializeLibrary: ImportFnCount=%lu Expected=%lu\n",
                                                  ImportTable ? ImportTable->FunctionCount : 0,
                                                  (ULONG)KDNET_EXT_IMPORTS);
    
    Exports->KdInitializeController =   (KD_INITIALIZE_CONTROLLER)E1000InitializeController;
    Exports->KdShutdownController =     (KD_SHUTDOWN_CONTROLLER)E1000ShutdownController;
    Exports->KdSetHibernateRange =      (KD_SET_HIBERNATE_RANGE)KdSetHibernateRange;
    Exports->KdGetRxPacket =            (KD_GET_RX_PACKET)E1000GetRxPacket;
    Exports->KdReleaseRxPacket =        (KD_RELEASE_RX_PACKET)E1000ReleaseRxPacket;
    Exports->KdGetTxPacket =            (KD_GET_TX_PACKET)E1000GetTxPacket;
    Exports->KdSendTxPacket =           (KD_SEND_TX_PACKET)E1000SendTxPacket;
    Exports->KdGetPacketAddress =       (KD_GET_PACKET_ADDRESS)E1000GetPacketAddress;
    Exports->KdGetPacketLength =        (KD_GET_PACKET_LENGTH)E1000GetPacketLength;
    Exports->KdGetHardwareContextSize = (KD_GET_HARDWARE_CONTEXT_SIZE)E1000GetHardwareContextSize;

    return Status;
}
