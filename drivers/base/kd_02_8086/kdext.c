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
NTAPI
KdNicSetHibernateRange(
    VOID
    )
{
    //TODO:
}

ULONG
NTAPI
KdNicGetHardwareContextSize(
    _In_ struct _DEBUG_DEVICE_DESCRIPTOR *Device
)
{
    return E1000GetHardwareContextSize();
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

    if (!KdNetExtensibilityImports || !KdNetDbgPrintf)
        return STATUS_INVALID_PARAMETER;

    KdNetDbgPrintf("KdInitializeLibrary: ExportsFnCount=%lu Expected=%lu\n",
                                                      ImportTable->Exports->FunctionCount,
                                                      KDNET_EXT_EXPORTS);
    KdNetDbgPrintf("KdInitializeLibrary: ImportFnCount=%lu Expected=%lu\n",
                                                  ImportTable ? ImportTable->FunctionCount : 0,
                                                  (ULONG)KDNET_EXT_IMPORTS);

    Exports->KdInitializeController =   E1000InitializeController;
    Exports->KdShutdownController =     E1000ShutdownController;
    Exports->KdSetHibernateRange =      KdNicSetHibernateRange;
    Exports->KdGetRxPacket =            E1000GetRxPacket;
    Exports->KdReleaseRxPacket =        E1000ReleaseRxPacket;
    Exports->KdGetTxPacket =            E1000GetTxPacket;
    Exports->KdSendTxPacket =           E1000SendTxPacket;
    Exports->KdGetPacketAddress =       E1000GetPacketAddress;
    Exports->KdGetPacketLength =        E1000GetPacketLength;
    Exports->KdGetHardwareContextSize = KdNicGetHardwareContextSize;

    return Status;
}
