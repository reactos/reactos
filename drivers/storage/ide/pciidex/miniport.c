/*
 * PROJECT:     PCI IDE bus driver extension
 * LICENSE:     See COPYING in the top level directory
 * PURPOSE:     Miniport functions
 * COPYRIGHT:   Copyright 2005 Hervé Poussineau <hpoussin@reactos.org>
 */

#include "pciidex.h"

/** @brief Global debugging level. Valid values are between 0 (Error) and 3 (Trace). */
ULONG PciIdeDebug = 0;

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
    PFDO_DEVICE_EXTENSION FdoExt;
    PATA_CONTROLLER Controller;
    ULONG BytesRead;

    INFO("PciIdeXGetBusData(%p %p 0x%lx 0x%lx)\n",
         DeviceExtension, Buffer, ConfigDataOffset, BufferLength);

    FdoExt = CONTAINING_RECORD(DeviceExtension, FDO_DEVICE_EXTENSION, MiniControllerExtension);
    Controller = &FdoExt->Controller;

    BytesRead = (*Controller->BusInterface.GetBusData)(Controller->BusInterface.Context,
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
    PFDO_DEVICE_EXTENSION FdoExt;
    PATA_CONTROLLER Controller;
    UCHAR LocalBuffer[4];
    ULONG i, BytesWritten;
    PUCHAR CurrentBuffer;
    KIRQL OldIrql;
    NTSTATUS Status;

    INFO("PciIdeXSetBusData(%p %p %p 0x%lx 0x%lx)\n",
         DeviceExtension, Buffer, DataMask, ConfigDataOffset, BufferLength);

    /* Optimize the common case (4-byte PCI access) */
    if (BufferLength <= sizeof(LocalBuffer))
    {
        CurrentBuffer = LocalBuffer;
    }
    else
    {
        CurrentBuffer = ExAllocatePoolUninitialized(NonPagedPool, BufferLength, TAG_PCIIDEX);
        if (!CurrentBuffer)
            return STATUS_INSUFFICIENT_RESOURCES;
    }

    FdoExt = CONTAINING_RECORD(DeviceExtension, FDO_DEVICE_EXTENSION, MiniControllerExtension);
    Controller = &FdoExt->Controller;

    /*
     * This spinlock protects the PCI configuration space against concurrent modifications.
     * For example, the PIIX register SIDETIM (0x44) is a byte-sized register
     * and controls both of the IDE channels.
     */
    KeAcquireSpinLock(&Controller->Lock, &OldIrql);

    Status = PciIdeXGetBusData(DeviceExtension, Buffer, ConfigDataOffset, BufferLength);
    if (!NT_SUCCESS(Status))
        goto Cleanup;

    for (i = 0; i < BufferLength; i++)
    {
        CurrentBuffer[i] = (CurrentBuffer[i] & ~((PUCHAR)DataMask)[i]) |
                           (((PUCHAR)DataMask)[i] & ((PUCHAR)Buffer)[i]);
    }

    BytesWritten = (*Controller->BusInterface.SetBusData)(Controller->BusInterface.Context,
                                                          PCI_WHICHSPACE_CONFIG,
                                                          CurrentBuffer,
                                                          ConfigDataOffset,
                                                          BufferLength);
    if (BytesWritten != BufferLength)
        Status = STATUS_UNSUCCESSFUL;
    else
        Status = STATUS_SUCCESS;

Cleanup:
    KeReleaseSpinLock(&Controller->Lock, OldIrql);

    if (CurrentBuffer != LocalBuffer)
        ExFreePoolWithTag(CurrentBuffer, TAG_PCIIDEX);
    return Status;
}

NTSTATUS
NTAPI
PciIdexGetBusLocation(
    _In_ PVOID DeviceExtension,
    _Out_ PULONG BusLocation)
{
    PFDO_DEVICE_EXTENSION FdoExt;
    ULONG Length;
    NTSTATUS Status;

    FdoExt = CONTAINING_RECORD(DeviceExtension, FDO_DEVICE_EXTENSION, MiniControllerExtension);

    Status = IoGetDeviceProperty(FdoExt->Pdo,
                                 DevicePropertyAddress,
                                 sizeof(*BusLocation),
                                 BusLocation,
                                 &Length);
    return Status;
}
