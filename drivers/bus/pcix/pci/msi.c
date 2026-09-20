/*
 * PROJECT:     ReactOS PCI Bus Driver
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Message Signaled Interrupt Support
 * COPYRIGHT:   Copyright 2026 Justin Miller <justin.miller@reactos.org>
 */

/* INCLUDES *******************************************************************/

#include <pci.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

#define PCI_CONNECTION_DATA_SIZE(Count)                 \
    (FIELD_OFFSET(INTERRUPT_CONNECTION_DATA, Vectors) + \
     ((Count) * sizeof(INTERRUPT_VECTOR_DATA)))

#define PCI_MESSAGE_POLICY_KEY      L"Interrupt Management\\MessageSignaledInterruptProperties"
#define PCI_MSI_MAX_LEVEL_MESSAGES  16
#define PCI_MAX_REQUESTED_MESSAGES  32

/* The interrupt arbiter publishes the messages it granted under this property */
static const DEVPROPKEY PciInterruptConnectionDataKey =
{
    { 0xF0E20F09, 0xD97A, 0x49A9, { 0x80, 0x46, 0xBB, 0x6E, 0x22, 0xE6, 0xBB, 0x2E } },
    2
};

typedef NTSTATUS
(NTAPI *PCI_GET_DEVICE_PROPERTY_DATA)(
    _In_ PDEVICE_OBJECT Pdo,
    _In_ CONST DEVPROPKEY *PropertyKey,
    _In_ LCID Lcid,
    _In_ ULONG Flags,
    _In_ ULONG Size,
    _Out_writes_bytes_opt_(Size) PVOID Data,
    _Out_ PULONG RequiredSize,
    _Out_ PDEVPROPTYPE Type);

static PCI_GET_DEVICE_PROPERTY_DATA PciGetDevicePropertyDataRoutine;
static BOOLEAN PciGetDevicePropertyDataResolved;

/* FUNCTIONS ******************************************************************/

/* IoGetDevicePropertyData is newer than this driver's import floor */
static
PCI_GET_DEVICE_PROPERTY_DATA
NTAPI
PciLocateGetDevicePropertyData(VOID)
{
    UNICODE_STRING RoutineName;

    if (PciGetDevicePropertyDataResolved)
        return PciGetDevicePropertyDataRoutine;

    RtlInitUnicodeString(&RoutineName, L"IoGetDevicePropertyData");
    PciGetDevicePropertyDataRoutine = MmGetSystemRoutineAddress(&RoutineName);
    PciGetDevicePropertyDataResolved = TRUE;

    if (!PciGetDevicePropertyDataRoutine)
        DPRINT1("PCI: IoGetDevicePropertyData is not available\n");

    return PciGetDevicePropertyDataRoutine;
}

/* The caller frees the returned block */
static
NTSTATUS
NTAPI
PciQueryGrantedMessages(
    _In_ PPCI_PDO_EXTENSION PdoExtension,
    _Outptr_ PINTERRUPT_CONNECTION_DATA *ConnectionData)
{
    PCI_GET_DEVICE_PROPERTY_DATA GetPropertyData;
    PINTERRUPT_CONNECTION_DATA Data;
    DEVPROPTYPE PropertyType;
    NTSTATUS Status;
    ULONG Size, MaxCount;
    PAGED_CODE();

    *ConnectionData = NULL;

    GetPropertyData = PciLocateGetDevicePropertyData();
    if (!GetPropertyData)
        return STATUS_NOT_SUPPORTED;

    Size = 0;
    Status = GetPropertyData(PdoExtension->PhysicalDeviceObject,
                             &PciInterruptConnectionDataKey,
                             0,
                             0,
                             0,
                             NULL,
                             &Size,
                             &PropertyType);
    if (Status != STATUS_BUFFER_TOO_SMALL)
        return NT_SUCCESS(Status) ? STATUS_UNSUCCESSFUL : Status;

    if (Size < PCI_CONNECTION_DATA_SIZE(1))
        return STATUS_DATA_ERROR;

    Data = ExAllocatePoolWithTag(PagedPool, Size, PCI_POOL_TAG);
    if (!Data)
        return STATUS_INSUFFICIENT_RESOURCES;

    Status = GetPropertyData(PdoExtension->PhysicalDeviceObject,
                             &PciInterruptConnectionDataKey,
                             0,
                             0,
                             Size,
                             Data,
                             &Size,
                             &PropertyType);
    if (!NT_SUCCESS(Status))
    {
        ExFreePoolWithTag(Data, 0);
        return Status;
    }

    /* Never trust the count past the size of the property */
    MaxCount = (Size - FIELD_OFFSET(INTERRUPT_CONNECTION_DATA, Vectors)) /
               sizeof(INTERRUPT_VECTOR_DATA);
    if (!Data->Count || (Data->Count > MaxCount))
    {
        DPRINT1("PCI: Connection data for %p is short (%lu bytes, %lu vectors)\n",
                PdoExtension,
                Size,
                Data->Count);
        ExFreePoolWithTag(Data, 0);
        return STATUS_DATA_ERROR;
    }

    *ConnectionData = Data;
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
PciGetMsiXCapability(
    _Inout_ PPCI_PDO_EXTENSION PdoExtension)
{
    PPCI_MSIX_CAP_INFO MsiX = &PdoExtension->MessageInfo.MsiXCap;
    PCI_CAPABILITIES_HEADER Header;
    USHORT Control;
    ULONG TableRegister, PbaRegister;
    UCHAR Offset;
    PAGED_CODE();

    Offset = PciReadDeviceCapability(PdoExtension,
                                     PdoExtension->CapabilitiesPtr,
                                     PCI_CAPABILITY_ID_MSIX,
                                     &Header,
                                     sizeof(Header));
    if (!Offset)
        return;

    PciReadDeviceConfig(PdoExtension,
                        &Control,
                        Offset + PCI_MESSAGE_CONTROL_OFFSET,
                        sizeof(Control));
    PciReadDeviceConfig(PdoExtension,
                        &TableRegister,
                        Offset + PCI_MSIX_TABLE_OFFSET,
                        sizeof(TableRegister));
    PciReadDeviceConfig(PdoExtension,
                        &PbaRegister,
                        Offset + PCI_MSIX_PBA_OFFSET,
                        sizeof(PbaRegister));

    /* The table size field is encoded as one less than the entry count */
    MsiX->CapabilityPtr = Offset;
    MsiX->RequestedCount = (Control & PCI_MSIX_CONTROL_TABLE_SIZE_MASK) + 1;
    MsiX->TableBarIndex = (UCHAR)(TableRegister & PCI_MSIX_BIR_MASK);
    MsiX->TableBarOffset = TableRegister & PCI_MSIX_OFFSET_MASK;
    MsiX->PbaBarIndex = (UCHAR)(PbaRegister & PCI_MSIX_BIR_MASK);
    MsiX->PbaBarOffset = PbaRegister & PCI_MSIX_OFFSET_MASK;

    /* Some functions report their even entry count without subtracting one */
    if (PdoExtension->HackFlags & PCI_HACK_MSIX_TABLE_SIZE_IS_COUNT)
        MsiX->RequestedCount &= ~1;

    DPRINT1("PCI: MSI-X at 0x%x, %u message(s), table in BAR %u at 0x%lx\n",
            Offset,
            MsiX->RequestedCount,
            MsiX->TableBarIndex,
            MsiX->TableBarOffset);
}

static
VOID
NTAPI
PciGetMsiCapability(
    _Inout_ PPCI_PDO_EXTENSION PdoExtension)
{
    PPCI_MSI_CAP_INFO Msi = &PdoExtension->MessageInfo.MsiCap;
    PCI_CAPABILITIES_HEADER Header;
    USHORT Control, CapableShift;
    UCHAR Offset;
    PAGED_CODE();

    Offset = PciReadDeviceCapability(PdoExtension,
                                     PdoExtension->CapabilitiesPtr,
                                     PCI_CAPABILITY_ID_MSI,
                                     &Header,
                                     sizeof(Header));
    if (!Offset)
        return;

    PciReadDeviceConfig(PdoExtension,
                        &Control,
                        Offset + PCI_MESSAGE_CONTROL_OFFSET,
                        sizeof(Control));

    /* The capable field is a power of two, and anything past 32 messages is reserved */
    CapableShift = (Control & PCI_MSI_CONTROL_MMC_MASK) >> PCI_MSI_CONTROL_MMC_SHIFT;
    if (CapableShift > PCI_MSI_MAX_MESSAGE_SHIFT)
        CapableShift = PCI_MSI_MAX_MESSAGE_SHIFT;

    Msi->CapabilityPtr = Offset;
    Msi->RequestedCount = (USHORT)(1 << CapableShift);
    Msi->Is64Bit = (Control & PCI_MSI_CONTROL_64BIT) != 0;
    Msi->MaskCapable = (Control & PCI_MSI_CONTROL_MASKING) != 0;

    DPRINT1("PCI: MSI at 0x%x, %u message(s), %s address\n",
            Offset,
            Msi->RequestedCount,
            Msi->Is64Bit ? "64-bit" : "32-bit");
}

/**
 * @brief
 * Records the MSI and MSI-X capabilities of a new function. Which one is
 * used is picked by PciSelectMessageType once its BARs are known.
 *
 * @param[in,out] PdoExtension
 * The PDO extension of the function being enumerated.
 */
VOID
NTAPI
PciGetMessageCapabilities(
    _Inout_ PPCI_PDO_EXTENSION PdoExtension)
{
    PAGED_CODE();

    RtlZeroMemory(&PdoExtension->MessageInfo, sizeof(PdoExtension->MessageInfo));
    PdoExtension->MessageInfo.Type = PciMessageNone;

    if (!PdoExtension->CapabilitiesPtr)
        return;

    PciGetMsiXCapability(PdoExtension);
    PciGetMsiCapability(PdoExtension);
}

/* An MSI-X structure has to lie inside a writable memory BAR of the function */
static
BOOLEAN
NTAPI
PciDoesMsiXStructureFit(
    _In_ PPCI_PDO_EXTENSION PdoExtension,
    _In_ UCHAR BarIndex,
    _In_ ULONG Offset,
    _In_ ULONG Length)
{
    PIO_RESOURCE_DESCRIPTOR Limit;
    ULONGLONG BarLength;
    PAGED_CODE();

    if (!PdoExtension->Resources || (BarIndex >= PCI_TYPE0_ADDRESSES))
        return FALSE;

    Limit = &PdoExtension->Resources->Limit[BarIndex];
    if ((Limit->Type != CmResourceTypeMemory) && (Limit->Type != CmResourceTypeMemoryLarge))
        return FALSE;

    /* Bridge windows have no length, and a ROM limit is read only */
    if (!PciIsRequirementDescriptor(Limit) || (Limit->Flags & CM_RESOURCE_MEMORY_READ_ONLY))
        return FALSE;

    BarLength = RtlIoDecodeMemIoResource(Limit, NULL, NULL, NULL);
    return ((ULONGLONG)Offset + Length) <= BarLength;
}

static
BOOLEAN
NTAPI
PciAreMsiXStructuresValid(
    _In_ PPCI_PDO_EXTENSION PdoExtension)
{
    PPCI_MSIX_CAP_INFO MsiX = &PdoExtension->MessageInfo.MsiXCap;
    ULONGLONG TableEnd, PbaEnd;
    ULONG TableLength, PbaLength;
    PAGED_CODE();

    TableLength = MsiX->RequestedCount * sizeof(PCI_MSIX_VECTOR);

    /* One pending bit per table entry, kept in 64-bit words */
    PbaLength = ((MsiX->RequestedCount + 63) / 64) * sizeof(ULONGLONG);

    if (!PciDoesMsiXStructureFit(PdoExtension,
                                 MsiX->TableBarIndex,
                                 MsiX->TableBarOffset,
                                 TableLength) ||
        !PciDoesMsiXStructureFit(PdoExtension,
                                 MsiX->PbaBarIndex,
                                 MsiX->PbaBarOffset,
                                 PbaLength))
    {
        return FALSE;
    }

    if (MsiX->TableBarIndex != MsiX->PbaBarIndex)
        return TRUE;

    /* Both can share a BAR as long as they do not overlap */
    TableEnd = (ULONGLONG)MsiX->TableBarOffset + TableLength;
    PbaEnd = (ULONGLONG)MsiX->PbaBarOffset + PbaLength;
    return (TableEnd <= MsiX->PbaBarOffset) || (PbaEnd <= MsiX->TableBarOffset);
}

/**
 * @brief
 * Picks the message capability a function uses, once its BAR limits are known.
 * MSI-X wins when its table and pending bits fit its BARs, otherwise MSI is used.
 *
 * @param[in,out] PdoExtension
 * The PDO extension of the function being enumerated.
 */
VOID
NTAPI
PciSelectMessageType(
    _Inout_ PPCI_PDO_EXTENSION PdoExtension)
{
    PPCI_MESSAGE_INFO MessageInfo = &PdoExtension->MessageInfo;
    PAGED_CODE();

    MessageInfo->Type = PciMessageNone;

    if (MessageInfo->MsiXCap.CapabilityPtr)
    {
        if (PciAreMsiXStructuresValid(PdoExtension))
        {
            MessageInfo->Type = PciMessageMsiX;
            return;
        }

        DPRINT1("PCI: MSI-X structures of %p do not fit its BARs\n", PdoExtension);
    }

    if (MessageInfo->MsiCap.CapabilityPtr)
        MessageInfo->Type = PciMessageMsi;
}

static
NTSTATUS
NTAPI
PciReadMessagePolicyDword(
    _In_ HANDLE DeviceKey,
    _In_ PWCHAR ValueName,
    _Out_ PULONG Value)
{
    NTSTATUS Status;
    PULONG Buffer;
    ULONG Length;
    PAGED_CODE();

    Status = PciGetRegistryValue(ValueName,
                                 PCI_MESSAGE_POLICY_KEY,
                                 DeviceKey,
                                 REG_DWORD,
                                 (PVOID*)&Buffer,
                                 &Length);
    if (!NT_SUCCESS(Status))
        return Status;

    if (Length >= sizeof(ULONG))
        *Value = *Buffer;
    else
        Status = STATUS_BUFFER_TOO_SMALL;

    ExFreePoolWithTag(Buffer, 0);
    return Status;
}

/**
 * @brief
 * Works out how many messages to ask the interrupt arbiter for.
 *
 * @param[in] PdoExtension
 * The PDO extension of the function.
 *
 * @param[in] HasLineInterrupt
 * TRUE if the function also asks for a wired interrupt line.
 *
 * @return
 * The number of messages to request, or 0 to only use the wired line.
 */
ULONG
NTAPI
PciGetRequestableMessageCount(
    _In_ PPCI_PDO_EXTENSION PdoExtension,
    _In_ BOOLEAN HasLineInterrupt)
{
    PPCI_MESSAGE_INFO MessageInfo = &PdoExtension->MessageInfo;
    HANDLE DeviceKey;
    NTSTATUS Status;
    ULONG Count, Value;
    BOOLEAN IsExpressPort;
    PAGED_CODE();

    if (MessageInfo->Type == PciMessageNone)
        return 0;

    if (!NT_SUCCESS(IoOpenDeviceRegistryKey(PdoExtension->PhysicalDeviceObject,
                                            PLUGPLAY_REGKEY_DEVICE,
                                            KEY_READ,
                                            &DeviceKey)))
    {
        return 0;
    }

    /* With a wired line to fall back on, the driver has to opt in to messages */
    IsExpressPort = (PdoExtension->ExpressDeviceType == PciExpressRootPort) ||
                    (PdoExtension->ExpressDeviceType == PciExpressDownstreamSwitchPort);
    if (HasLineInterrupt && !IsExpressPort)
    {
        Status = PciReadMessagePolicyDword(DeviceKey, L"MSISupported", &Value);
        if (!NT_SUCCESS(Status) || !Value)
        {
            ZwClose(DeviceKey);
            return 0;
        }
    }

    if (MessageInfo->Type == PciMessageMsiX)
        Count = MessageInfo->MsiXCap.RequestedCount;
    else
        Count = MessageInfo->MsiCap.RequestedCount;

    Status = PciReadMessagePolicyDword(DeviceKey, L"MessageNumberLimit", &Value);
    ZwClose(DeviceKey);

    if (Status == STATUS_BUFFER_TOO_SMALL)
        return 0;

    if (NT_SUCCESS(Status) && (Value < Count))
        Count = Value;

    /* An MSI run has to fit inside one interrupt level */
    if ((MessageInfo->Type == PciMessageMsi) && (Count > PCI_MSI_MAX_LEVEL_MESSAGES))
        Count = PCI_MSI_MAX_LEVEL_MESSAGES;

    /* The interrupt arbiter reserves at most this many messages for one request */
    if (Count > PCI_MAX_REQUESTED_MESSAGES)
        Count = PCI_MAX_REQUESTED_MESSAGES;

    return Count;
}

/* Turns one granted message into the address and data the device writes to raise it */
static
NTSTATUS
NTAPI
PciGetMessageAddressAndData(
    _In_ PINTERRUPT_VECTOR_DATA VectorData,
    _Out_ PPHYSICAL_ADDRESS Address,
    _Out_ PULONG Data)
{
    HAL_MESSAGE_SIGNAL_TARGET_REQUEST Request;
    INTERRUPT_CONNECTION_DATA Routed;
    NTSTATUS Status;

    if (VectorData->Type == InterruptTypeXapicMessage)
    {
        *Address = VectorData->XapicMessage.Address;
        *Data = VectorData->XapicMessage.DataPayload;
        return STATUS_SUCCESS;
    }

    /* HyperTransport would need an MSI mapping capability this driver does not program */
    if (VectorData->Type != InterruptTypeMessageRequest)
        return STATUS_NOT_SUPPORTED;

    RtlZeroMemory(&Request, sizeof(Request));
    Request.InterruptTargetType = TargetApic;
    Request.ApicTarget.InterruptVector = VectorData->Vector;
    Request.ApicTarget.ApicDestinationMode = VectorData->MessageRequest.DestinationMode;
    Request.ApicTarget.TargetProcessors = VectorData->TargetProcessors;
#if (NTDDI_VERSION >= NTDDI_WIN7)
    Request.ApicTarget.InterruptRemapInfo = VectorData->IntRemapInfo;
#endif

    RtlZeroMemory(&Routed, sizeof(Routed));
    Status = HalGetMessageRoutingInfo(&Request, &Routed);
    if (!NT_SUCCESS(Status))
        return Status;

    if (Routed.Vectors[0].Type != InterruptTypeXapicMessage)
        return STATUS_UNSUCCESSFUL;

    *Address = Routed.Vectors[0].XapicMessage.Address;
    *Data = Routed.Vectors[0].XapicMessage.DataPayload;
    return STATUS_SUCCESS;
}

static
VOID
NTAPI
PciClearInterruptDisable(
    _In_ PPCI_PDO_EXTENSION PdoExtension)
{
    USHORT Command;
    PAGED_CODE();

    PciReadDeviceConfig(PdoExtension,
                        &Command,
                        FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                        sizeof(Command));
    Command &= ~PCI_DISABLE_LEVEL_INTERRUPT;
    PciWriteDeviceConfig(PdoExtension,
                         &Command,
                         FIELD_OFFSET(PCI_COMMON_HEADER, Command),
                         sizeof(Command));
}

/* MSI has one address and data pair, and message N is raised with N added to the data */
static
NTSTATUS
NTAPI
PciProgramMsi(
    _Inout_ PPCI_PDO_EXTENSION PdoExtension,
    _In_ PINTERRUPT_CONNECTION_DATA ConnectionData,
    _In_ ULONG Count)
{
    PPCI_MSI_CAP_INFO Msi = &PdoExtension->MessageInfo.MsiCap;
    PHYSICAL_ADDRESS Address;
    NTSTATUS Status;
    USHORT Control;
    ULONG Data, Mask, DataOffset, EnableShift, Enabled;
    UCHAR Offset;
    PAGED_CODE();

    Status = PciGetMessageAddressAndData(&ConnectionData->Vectors[0], &Address, &Data);
    if (!NT_SUCCESS(Status))
        return Status;

    /* Only a power of two run of messages can be enabled, so round the grant down */
    EnableShift = 0;
    while ((EnableShift < PCI_MSI_MAX_MESSAGE_SHIFT) && ((2UL << EnableShift) <= Count))
        EnableShift++;
    Enabled = 1UL << EnableShift;

    Offset = Msi->CapabilityPtr;
    PciReadDeviceConfig(PdoExtension,
                        &Control,
                        Offset + PCI_MESSAGE_CONTROL_OFFSET,
                        sizeof(Control));

    /* Keep it off while the address and data change */
    Control &= ~PCI_MSI_CONTROL_ENABLE;
    PciWriteDeviceConfig(PdoExtension,
                         &Control,
                         Offset + PCI_MESSAGE_CONTROL_OFFSET,
                         sizeof(Control));

    PciWriteDeviceConfig(PdoExtension,
                         &Address.LowPart,
                         Offset + PCI_MSI_ADDRESS_OFFSET,
                         sizeof(ULONG));

    DataOffset = PCI_MSI_ADDRESS_OFFSET + sizeof(ULONG);
    if (Msi->Is64Bit)
    {
        PciWriteDeviceConfig(PdoExtension,
                             &Address.HighPart,
                             Offset + DataOffset,
                             sizeof(ULONG));
        DataOffset += sizeof(ULONG);
    }

    PciWriteDeviceConfig(PdoExtension, &Data, Offset + DataOffset, sizeof(USHORT));

    /* The mask register follows the data register and its reserved word */
    if (Msi->MaskCapable)
    {
        Mask = (Enabled >= 32) ? 0 : ~((1UL << Enabled) - 1);
        PciWriteDeviceConfig(PdoExtension,
                             &Mask,
                             Offset + DataOffset + sizeof(ULONG),
                             sizeof(ULONG));
    }

    Control &= ~PCI_MSI_CONTROL_MME_MASK;
    Control |= (USHORT)(EnableShift << PCI_MSI_CONTROL_MME_SHIFT);
    Control |= PCI_MSI_CONTROL_ENABLE;
    PciWriteDeviceConfig(PdoExtension,
                         &Control,
                         Offset + PCI_MESSAGE_CONTROL_OFFSET,
                         sizeof(Control));

    /* Some functions only raise messages with the legacy interrupt disable bit clear */
    if (PdoExtension->HackFlags & PCI_HACK_CLEAR_INT_DISABLE_FOR_MSI)
        PciClearInterruptDisable(PdoExtension);

    PdoExtension->MessageInfo.GrantedCount = (USHORT)Enabled;
    DPRINT1("PCI: MSI enabled on %p, %lu message(s) at 0x%08lx data 0x%lx\n",
            PdoExtension,
            Enabled,
            Address.LowPart,
            Data);
    return STATUS_SUCCESS;
}

/* Every table entry is written, and entries past the grant share the first message */
static
NTSTATUS
NTAPI
PciProgramMsiX(
    _Inout_ PPCI_PDO_EXTENSION PdoExtension,
    _In_ PINTERRUPT_CONNECTION_DATA ConnectionData,
    _In_ ULONG Count)
{
    PPCI_MSIX_CAP_INFO MsiX = &PdoExtension->MessageInfo.MsiXCap;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR TableBar;
    PPCI_MSIX_VECTOR Table;
    PHYSICAL_ADDRESS Address, TableAddress;
    NTSTATUS Status;
    USHORT Control;
    ULONG Data, Index, Message, TableSize, EntryControl;
    ULONGLONG BarLength;
    UCHAR Offset;
    PAGED_CODE();

    if (!PdoExtension->Resources || (MsiX->TableBarIndex >= PCI_TYPE0_ADDRESSES))
        return STATUS_DEVICE_CONFIGURATION_ERROR;

    TableBar = &PdoExtension->Resources->Current[MsiX->TableBarIndex];
    if ((TableBar->Type != CmResourceTypeMemory) &&
        (TableBar->Type != CmResourceTypeMemoryLarge))
    {
        DPRINT1("PCI: MSI-X table BAR %u of %p is not memory\n",
                MsiX->TableBarIndex,
                PdoExtension);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    TableSize = MsiX->RequestedCount * sizeof(PCI_MSIX_VECTOR);
    BarLength = RtlCmDecodeMemIoResource(TableBar, NULL);
    if ((MsiX->TableBarOffset > BarLength) ||
        (TableSize > (BarLength - MsiX->TableBarOffset)))
    {
        DPRINT1("PCI: MSI-X table of %p does not fit in its BAR\n", PdoExtension);
        return STATUS_DEVICE_CONFIGURATION_ERROR;
    }

    TableAddress.QuadPart = TableBar->u.Memory.Start.QuadPart + MsiX->TableBarOffset;
    Table = MmMapIoSpace(TableAddress, TableSize, MmNonCached);
    if (!Table)
        return STATUS_INSUFFICIENT_RESOURCES;

    /* Enable with the whole function masked so no entry fires half written */
    Offset = MsiX->CapabilityPtr;
    PciReadDeviceConfig(PdoExtension,
                        &Control,
                        Offset + PCI_MESSAGE_CONTROL_OFFSET,
                        sizeof(Control));
    Control |= PCI_MSIX_CONTROL_FUNCTION_MASK | PCI_MSIX_CONTROL_ENABLE;
    PciWriteDeviceConfig(PdoExtension,
                         &Control,
                         Offset + PCI_MESSAGE_CONTROL_OFFSET,
                         sizeof(Control));

    Status = STATUS_SUCCESS;
    for (Index = 0; Index < MsiX->RequestedCount; Index++)
    {
        Message = (Index < Count) ? Index : 0;
        Status = PciGetMessageAddressAndData(&ConnectionData->Vectors[Message],
                                             &Address,
                                             &Data);
        if (!NT_SUCCESS(Status))
            break;

        WRITE_REGISTER_ULONG(&Table[Index].AddressLowPart, Address.LowPart);
        WRITE_REGISTER_ULONG(&Table[Index].AddressHighPart, (ULONG)Address.HighPart);
        WRITE_REGISTER_ULONG(&Table[Index].Data, Data);

        /* Only the mask bit is ours, the rest of the control word must be preserved */
        EntryControl = READ_REGISTER_ULONG(&Table[Index].Control);
        WRITE_REGISTER_ULONG(&Table[Index].Control,
                             EntryControl & ~PCI_MSIX_VECTOR_CONTROL_MASK);
    }

    MmUnmapIoSpace(Table, TableSize);

    if (!NT_SUCCESS(Status))
    {
        Control &= ~PCI_MSIX_CONTROL_ENABLE;
        PciWriteDeviceConfig(PdoExtension,
                             &Control,
                             Offset + PCI_MESSAGE_CONTROL_OFFSET,
                             sizeof(Control));
        return Status;
    }

    Control &= ~PCI_MSIX_CONTROL_FUNCTION_MASK;
    PciWriteDeviceConfig(PdoExtension,
                         &Control,
                         Offset + PCI_MESSAGE_CONTROL_OFFSET,
                         sizeof(Control));

    if (PdoExtension->HackFlags & PCI_HACK_CLEAR_INT_DISABLE_FOR_MSI)
        PciClearInterruptDisable(PdoExtension);

    PdoExtension->MessageInfo.GrantedCount = (USHORT)Count;
    DPRINT1("PCI: MSI-X enabled on %p, %lu message(s)\n", PdoExtension, Count);
    return STATUS_SUCCESS;
}

/**
 * @brief
 * Programs the messages the interrupt arbiter granted into the function.
 *
 * @param[in,out] PdoExtension
 * The PDO extension of the function being started.
 *
 * @param[in] Granted
 * The messages granted in the start request, 0 if unknown.
 *
 * @return
 * STATUS_SUCCESS if the function was programmed.
 */
NTSTATUS
NTAPI
PciProgramMessageInterrupt(
    _Inout_ PPCI_PDO_EXTENSION PdoExtension,
    _In_ ULONG Granted)
{
    PPCI_MESSAGE_INFO MessageInfo = &PdoExtension->MessageInfo;
    PINTERRUPT_CONNECTION_DATA ConnectionData;
    NTSTATUS Status;
    ULONG Count, Requested;
    PAGED_CODE();

    if (MessageInfo->Type == PciMessageMsiX)
        Requested = MessageInfo->MsiXCap.RequestedCount;
    else if (MessageInfo->Type == PciMessageMsi)
        Requested = MessageInfo->MsiCap.RequestedCount;
    else
        return STATUS_NOT_SUPPORTED;

    Status = PciQueryGrantedMessages(PdoExtension, &ConnectionData);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("PCI: No granted messages for %p (0x%lx)\n", PdoExtension, Status);
        return Status;
    }

    /* Never tell the device about a message that has no vector behind it */
    Count = ConnectionData->Count;
    if (Count > Requested)
        Count = Requested;

    if (Granted && (Granted < Count))
        Count = Granted;

    if (MessageInfo->Type == PciMessageMsiX)
        Status = PciProgramMsiX(PdoExtension, ConnectionData, Count);
    else
        Status = PciProgramMsi(PdoExtension, ConnectionData, Count);

    ExFreePoolWithTag(ConnectionData, 0);
    return Status;
}

static
VOID
NTAPI
PciClearMessageEnable(
    _In_ PPCI_PDO_EXTENSION PdoExtension,
    _In_ ULONG CapabilityId,
    _In_ USHORT EnableBit)
{
    PCI_CAPABILITIES_HEADER Header;
    USHORT Control;
    UCHAR Offset;
    PAGED_CODE();

    Offset = PciReadDeviceCapability(PdoExtension,
                                     PdoExtension->CapabilitiesPtr,
                                     CapabilityId,
                                     &Header,
                                     sizeof(Header));
    if (!Offset)
        return;

    PciReadDeviceConfig(PdoExtension,
                        &Control,
                        Offset + PCI_MESSAGE_CONTROL_OFFSET,
                        sizeof(Control));
    if (!(Control & EnableBit))
        return;

    Control &= ~EnableBit;
    PciWriteDeviceConfig(PdoExtension,
                         &Control,
                         Offset + PCI_MESSAGE_CONTROL_OFFSET,
                         sizeof(Control));
}

/**
 * @brief
 * Turns off MSI and MSI-X on a function, including whichever one this
 * driver does not use but firmware may have left enabled.
 *
 * @param[in,out] PdoExtension
 * The PDO extension of the function.
 */
VOID
NTAPI
PciDisableMessageInterrupt(
    _Inout_ PPCI_PDO_EXTENSION PdoExtension)
{
    PAGED_CODE();

    PdoExtension->MessageInfo.GrantedCount = 0;

    if (!PdoExtension->CapabilitiesPtr)
        return;

    PciClearMessageEnable(PdoExtension, PCI_CAPABILITY_ID_MSIX, PCI_MSIX_CONTROL_ENABLE);
    PciClearMessageEnable(PdoExtension, PCI_CAPABILITY_ID_MSI, PCI_MSI_CONTROL_ENABLE);
}

/**
 * @brief
 * Finds the first interrupt of a start request and adds up the messages granted.
 *
 * @param[in] ResourceList
 * The raw resources assigned in the start request.
 *
 * @param[out] Messages
 * Receives the message count of every message descriptor. MSI has one
 * descriptor holding all of its messages, MSI-X one descriptor per message.
 *
 * @return
 * The first interrupt descriptor, or NULL if none was assigned.
 */
static
PCM_PARTIAL_RESOURCE_DESCRIPTOR
NTAPI
PciFindInterruptResource(
    _In_opt_ PCM_RESOURCE_LIST ResourceList,
    _Out_ PULONG Messages)
{
    PCM_FULL_RESOURCE_DESCRIPTOR FullDescriptor;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Partial;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR First = NULL;
    ULONG ListIndex, PartialIndex, Count;

    *Messages = 0;

    if (!ResourceList)
        return NULL;

    FullDescriptor = ResourceList->List;
    for (ListIndex = 0; ListIndex < ResourceList->Count; ListIndex++)
    {
        Partial = FullDescriptor->PartialResourceList.PartialDescriptors;
        for (PartialIndex = 0;
             PartialIndex < FullDescriptor->PartialResourceList.Count;
             PartialIndex++)
        {
            if (Partial->Type == CmResourceTypeInterrupt)
            {
                if (!First)
                    First = Partial;

                /* The arbiter packs the message count into the upper word of the level */
                if (Partial->Flags & CM_RESOURCE_INTERRUPT_MESSAGE)
                {
                    Count = Partial->u.Interrupt.Level >> 16;
                    *Messages += Count ? Count : 1;
                }
            }

            Partial = CmiGetNextPartialDescriptor(Partial);
        }

        /* The next full descriptor starts right after the last partial one */
        FullDescriptor = (PCM_FULL_RESOURCE_DESCRIPTOR)Partial;
    }

    return First;
}

/**
 * @brief
 * Programs the interrupt the arbiter granted a starting function. Called
 * once its windows decode, since an MSI-X table lives inside one of them.
 *
 * @param[in,out] PdoExtension
 * The PDO extension of the function being started.
 *
 * @param[in] ResourceList
 * The raw resources assigned in the start request.
 *
 * @return
 * STATUS_SUCCESS if the function can raise the interrupt it was given.
 */
NTSTATUS
NTAPI
PciProgramGrantedInterrupt(
    _Inout_ PPCI_PDO_EXTENSION PdoExtension,
    _In_opt_ PCM_RESOURCE_LIST ResourceList)
{
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Interrupt;
    ULONG Messages;
    PAGED_CODE();

    /* The grant can differ on every start, so nothing enabled before it is kept */
    PciDisableMessageInterrupt(PdoExtension);

    Interrupt = PciFindInterruptResource(ResourceList, &Messages);
    if (!Interrupt)
        return STATUS_SUCCESS;

    if (Interrupt->Flags & CM_RESOURCE_INTERRUPT_MESSAGE)
        return PciProgramMessageInterrupt(PdoExtension, Messages);

    /* A wired line only fires with the interrupt disable bit clear */
    PciClearInterruptDisable(PdoExtension);
    return STATUS_SUCCESS;
}

/* EOF */
