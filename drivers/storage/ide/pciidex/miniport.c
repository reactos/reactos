/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     Miniport functions
 * COPYRIGHT:   Copyright 2005 Hervé Poussineau <hpoussin@reactos.org>
 */

#include "pciidex.h"

#define NDEBUG
#include <debug.h>

/** @brief Global debugging level. Valid values are between 0 (Error) and 3 (Trace). */
ULONG PciIdeDebug = 0;

CODE_SEG("PAGE")
NTSTATUS
PciIdeXStartMiniport(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension)
{
    PPCIIDEX_DRIVER_EXTENSION DriverExtension;
    NTSTATUS Status;

    PAGED_CODE();

    if (FdoExtension->MiniportStarted)
        return STATUS_SUCCESS;

    DPRINT("Starting miniport\n");

    DriverExtension = IoGetDriverObjectExtension(FdoExtension->DriverObject,
                                                 FdoExtension->DriverObject);
    ASSERT(DriverExtension);

    FdoExtension->Properties.Size = sizeof(IDE_CONTROLLER_PROPERTIES);
    FdoExtension->Properties.ExtensionSize = DriverExtension->MiniControllerExtensionSize;
    Status = DriverExtension->HwGetControllerProperties(FdoExtension->MiniControllerExtension,
                                                        &FdoExtension->Properties);
    if (!NT_SUCCESS(Status))
        return Status;

    FdoExtension->MiniportStarted = TRUE;
    return STATUS_SUCCESS;
}

CODE_SEG("PAGE")
IDE_CHANNEL_STATE
PciIdeXChannelState(
    _In_ PFDO_DEVICE_EXTENSION FdoExtension,
    _In_ ULONG Channel)
{
    PCIIDE_CHANNEL_ENABLED MiniportChannelEnabled;

    PAGED_CODE();

    MiniportChannelEnabled = FdoExtension->Properties.PciIdeChannelEnabled;
    if (MiniportChannelEnabled)
        return MiniportChannelEnabled(FdoExtension->MiniControllerExtension, Channel);

    return ChannelStateUnknown;
}

/**
 * @brief Prints the given string with printf-like formatting to the kernel debugger.
 * @param[in] DebugPrintLevel Level of the debug message.
 *                            Valid values are between 0 (Error) and 3 (Trace).
 * @param[in] DebugMessage    Format of the string/arguments.
 * @param[in] ...             Variable number of arguments matching the format
 *                            specified in \a DebugMessage.
 * @sa PciIdeDebug
 */
VOID
PciIdeXDebugPrint(
    _In_ ULONG DebugPrintLevel,
    _In_z_ _Printf_format_string_ PCCHAR DebugMessage,
    ...)
{
    va_list ap;

    /* Check if we can print anything */
    if (DebugPrintLevel <= PciIdeDebug)
        DebugPrintLevel = 0;

    va_start(ap, DebugMessage);
    vDbgPrintEx(DPFLTR_PCIIDE_ID, DebugPrintLevel, DebugMessage, ap);
    va_end(ap);
}

/* May be called at IRQL <= DISPATCH_LEVEL */
NTSTATUS
NTAPI
PciIdeXGetBusData(
    _In_ PVOID DeviceExtension,
    _Out_writes_bytes_all_(BufferLength) PVOID Buffer,
    _In_ ULONG ConfigDataOffset,
    _In_ ULONG BufferLength)
{
    PFDO_DEVICE_EXTENSION FdoExtension;
    ULONG BytesRead;

    DPRINT("PciIdeXGetBusData(%p %p 0x%lx 0x%lx)\n",
           DeviceExtension, Buffer, ConfigDataOffset, BufferLength);

    FdoExtension = CONTAINING_RECORD(DeviceExtension,
                                     FDO_DEVICE_EXTENSION,
                                     MiniControllerExtension);

    BytesRead = (*FdoExtension->BusInterface.GetBusData)(FdoExtension->BusInterface.Context,
                                                         PCI_WHICHSPACE_CONFIG,
                                                         Buffer,
                                                         ConfigDataOffset,
                                                         BufferLength);
    if (BytesRead != BufferLength)
        return STATUS_UNSUCCESSFUL;

    return STATUS_SUCCESS;
}

/* May be called at IRQL <= DISPATCH_LEVEL */
NTSTATUS
NTAPI
PciIdeXSetBusData(
    _In_ PVOID DeviceExtension,
    _In_reads_bytes_(BufferLength) PVOID Buffer,
    _In_reads_bytes_(BufferLength) PVOID DataMask,
    _In_ ULONG ConfigDataOffset,
    _In_ ULONG BufferLength)
{
    PFDO_DEVICE_EXTENSION FdoExtension;
    ULONG i, BytesWritten;
    PUCHAR CurrentBuffer;
    KIRQL OldIrql;
    NTSTATUS Status;

    DPRINT("PciIdeXSetBusData(%p %p %p 0x%lx 0x%lx)\n",
           DeviceExtension, Buffer, DataMask, ConfigDataOffset, BufferLength);

    CurrentBuffer = ExAllocatePoolWithTag(NonPagedPool, BufferLength, TAG_PCIIDEX);
    if (!CurrentBuffer)
        return STATUS_INSUFFICIENT_RESOURCES;

    FdoExtension = CONTAINING_RECORD(DeviceExtension,
                                     FDO_DEVICE_EXTENSION,
                                     MiniControllerExtension);

    KeAcquireSpinLock(&FdoExtension->BusDataLock, &OldIrql);

    Status = PciIdeXGetBusData(DeviceExtension, Buffer, ConfigDataOffset, BufferLength);
    if (!NT_SUCCESS(Status))
        goto Cleanup;

    for (i = 0; i < BufferLength; i++)
    {
        CurrentBuffer[i] = (CurrentBuffer[i] & ~((PUCHAR)DataMask)[i]) |
                           (((PUCHAR)DataMask)[i] & ((PUCHAR)Buffer)[i]);
    }

    BytesWritten = (*FdoExtension->BusInterface.SetBusData)(FdoExtension->BusInterface.Context,
                                                         PCI_WHICHSPACE_CONFIG,
                                                         CurrentBuffer,
                                                         ConfigDataOffset,
                                                         BufferLength);
    if (BytesWritten != BufferLength)
        Status = STATUS_UNSUCCESSFUL;
    else
        Status = STATUS_SUCCESS;

Cleanup:
    KeReleaseSpinLock(&FdoExtension->BusDataLock, OldIrql);

    ExFreePoolWithTag(CurrentBuffer, TAG_PCIIDEX);
    return Status;
}
