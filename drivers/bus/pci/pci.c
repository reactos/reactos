/*
 * PROJECT:         ReactOS PCI Bus driver
 * FILE:            pci.c
 * PURPOSE:         Driver entry
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      10-09-2001  CSH  Created
 */

#include "pci.h"

#include <stdio.h>

#define NDEBUG
#include <debug.h>

static DRIVER_DISPATCH PciDispatchDeviceControl;
static NTSTATUS NTAPI PciDispatchDeviceControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

static DRIVER_ADD_DEVICE PciAddDevice;
static NTSTATUS NTAPI PciAddDevice(IN PDRIVER_OBJECT DriverObject, IN PDEVICE_OBJECT PhysicalDeviceObject);

static DRIVER_DISPATCH PciPowerControl;
static NTSTATUS NTAPI PciPowerControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

static DRIVER_DISPATCH PciPnpControl;
static NTSTATUS NTAPI PciPnpControl(IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp);

/*** PUBLIC ******************************************************************/

PPCI_DRIVER_EXTENSION DriverExtension = NULL;
BOOLEAN HasDebuggingDevice = FALSE;
PCI_TYPE1_CFG_CYCLE_BITS PciDebuggingDevice[2] = {0};

/*** PRIVATE *****************************************************************/

static NTSTATUS
NTAPI
PciDispatchDeviceControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(DeviceObject);
    DPRINT("Called. IRP is at (0x%p)\n", Irp);

    Irp->IoStatus.Information = 0;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);
    switch (IrpSp->Parameters.DeviceIoControl.IoControlCode)
    {
        default:
            DPRINT("Unknown IOCTL 0x%X\n", IrpSp->Parameters.DeviceIoControl.IoControlCode);
            Status = STATUS_NOT_IMPLEMENTED;
            break;
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;

        DPRINT("Completing IRP at 0x%p\n", Irp);

        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DPRINT("Leaving. Status 0x%X\n", Status);

    return Status;
}


static NTSTATUS
NTAPI
PciPnpControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
/*
 * FUNCTION: Handle Plug and Play IRPs
 * ARGUMENTS:
 *     DeviceObject = Pointer to PDO or FDO
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
    PCOMMON_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    DPRINT("IsFDO %u\n", DeviceExtension->IsFDO);

    if (DeviceExtension->IsFDO)
    {
        Status = FdoPnpControl(DeviceObject, Irp);
    }
    else
    {
        Status = PdoPnpControl(DeviceObject, Irp);
    }

    return Status;
}


static NTSTATUS
NTAPI
PciPowerControl(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp)
/*
 * FUNCTION: Handle power management IRPs
 * ARGUMENTS:
 *     DeviceObject = Pointer to PDO or FDO
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
    PCOMMON_DEVICE_EXTENSION DeviceExtension;
    NTSTATUS Status;

    DeviceExtension = (PCOMMON_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    if (DeviceExtension->IsFDO)
    {
        Status = FdoPowerControl(DeviceObject, Irp);
    }
    else
    {
        Status = PdoPowerControl(DeviceObject, Irp);
    }

    return Status;
}


static NTSTATUS
NTAPI
PciAddDevice(
    IN PDRIVER_OBJECT DriverObject,
    IN PDEVICE_OBJECT PhysicalDeviceObject)
{
    PFDO_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_OBJECT Fdo;
    NTSTATUS Status;

    DPRINT("Called\n");
    if (PhysicalDeviceObject == NULL)
        return STATUS_SUCCESS;

    Status = IoCreateDevice(DriverObject,
                            sizeof(FDO_DEVICE_EXTENSION),
                            NULL,
                            FILE_DEVICE_BUS_EXTENDER,
                            FILE_DEVICE_SECURE_OPEN,
                            TRUE,
                            &Fdo);
    if (!NT_SUCCESS(Status))
    {
        DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
        return Status;
    }

    DeviceExtension = (PFDO_DEVICE_EXTENSION)Fdo->DeviceExtension;

    RtlZeroMemory(DeviceExtension, sizeof(FDO_DEVICE_EXTENSION));

    DeviceExtension->Common.IsFDO = TRUE;

    DeviceExtension->Ldo = IoAttachDeviceToDeviceStack(Fdo,
                                                       PhysicalDeviceObject);

    DeviceExtension->State = dsStopped;

    Fdo->Flags &= ~DO_DEVICE_INITIALIZING;

    //Fdo->Flags |= DO_POWER_PAGABLE;

    DPRINT("Done AddDevice\n");

    return STATUS_SUCCESS;
}

DRIVER_UNLOAD PciUnload;

VOID
NTAPI
PciUnload(
    IN PDRIVER_OBJECT DriverObject)
{
    /* The driver object extension is destroyed by the I/O manager */
    UNREFERENCED_PARAMETER(DriverObject);
}

static
CODE_SEG("INIT")
VOID
PciLocateKdDevices(VOID)
{
    ULONG i;
    NTSTATUS Status;
    WCHAR KeyNameBuffer[16];
    ULONG BusNumber, SlotNumber;
    RTL_QUERY_REGISTRY_TABLE QueryTable[3];

    RtlZeroMemory(QueryTable, sizeof(QueryTable));
    QueryTable[0].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[0].Name = L"Bus";
    QueryTable[0].EntryContext = &BusNumber;
    QueryTable[1].Flags = RTL_QUERY_REGISTRY_DIRECT | RTL_QUERY_REGISTRY_REQUIRED;
    QueryTable[1].Name = L"Slot";
    QueryTable[1].EntryContext = &SlotNumber;

    for (i = 0; i < RTL_NUMBER_OF(PciDebuggingDevice); ++i)
    {
        PCI_SLOT_NUMBER PciSlot;

        RtlStringCbPrintfW(KeyNameBuffer, sizeof(KeyNameBuffer), L"PCI\\Debug\\%d", i);

        Status = RtlQueryRegistryValues(RTL_REGISTRY_SERVICES,
                                        KeyNameBuffer,
                                        QueryTable,
                                        NULL,
                                        NULL);
        if (!NT_SUCCESS(Status))
            return;

        HasDebuggingDevice = TRUE;

        PciSlot.u.AsULONG = SlotNumber;
        PciDebuggingDevice[i].DeviceNumber = PciSlot.u.bits.DeviceNumber;
        PciDebuggingDevice[i].FunctionNumber = PciSlot.u.bits.FunctionNumber;
        PciDebuggingDevice[i].BusNumber = BusNumber;
        PciDebuggingDevice[i].InUse = TRUE;

        DPRINT1("PCI debugging device %02x:%02x.%x\n",
                BusNumber,
                PciSlot.u.bits.DeviceNumber,
                PciSlot.u.bits.FunctionNumber);
    }
}

CODE_SEG("INIT")
NTSTATUS
NTAPI
DriverEntry(
    IN PDRIVER_OBJECT DriverObject,
    IN PUNICODE_STRING RegistryPath)
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(RegistryPath);
    DPRINT("Peripheral Component Interconnect Bus Driver\n");

    DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = PciDispatchDeviceControl;
    DriverObject->MajorFunction[IRP_MJ_PNP] = PciPnpControl;
    DriverObject->MajorFunction[IRP_MJ_POWER] = PciPowerControl;
    DriverObject->DriverExtension->AddDevice = PciAddDevice;
    DriverObject->DriverUnload = PciUnload;

    Status = IoAllocateDriverObjectExtension(DriverObject,
                                             DriverObject,
                                             sizeof(PCI_DRIVER_EXTENSION),
                                             (PVOID*)&DriverExtension);
    if (!NT_SUCCESS(Status))
        return Status;

    RtlZeroMemory(DriverExtension, sizeof(PCI_DRIVER_EXTENSION));

    InitializeListHead(&DriverExtension->BusListHead);
    KeInitializeSpinLock(&DriverExtension->BusListLock);

    PciLocateKdDevices();

    return STATUS_SUCCESS;
}


NTSTATUS
PciCreateDeviceIDString(PUNICODE_STRING DeviceID,
                        PPCI_DEVICE Device)
{
    WCHAR Buffer[256];

    swprintf(Buffer,
             L"PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%02X",
             Device->PciConfig.VendorID,
             Device->PciConfig.DeviceID,
             (Device->PciConfig.u.type0.SubSystemID << 16) +
             Device->PciConfig.u.type0.SubVendorID,
             Device->PciConfig.RevisionID);

    return RtlCreateUnicodeString(DeviceID, Buffer) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}


NTSTATUS
PciCreateInstanceIDString(PUNICODE_STRING InstanceID,
                          PPCI_DEVICE Device)
{
    WCHAR Buffer[3];

    swprintf(Buffer, L"%02X", Device->SlotNumber.u.AsULONG & 0xff);

    return RtlCreateUnicodeString(InstanceID, Buffer) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}


NTSTATUS
PciCreateHardwareIDsString(PUNICODE_STRING HardwareIDs,
                           PPCI_DEVICE Device)
{
    WCHAR Buffer[256];
    UNICODE_STRING BufferU;
    ULONG Index;

    Index = 0;
    Index += swprintf(&Buffer[Index],
                      L"PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X&REV_%02X",
                      Device->PciConfig.VendorID,
                      Device->PciConfig.DeviceID,
                      (Device->PciConfig.u.type0.SubSystemID << 16) +
                      Device->PciConfig.u.type0.SubVendorID,
                      Device->PciConfig.RevisionID);
    Index++;

    Index += swprintf(&Buffer[Index],
                      L"PCI\\VEN_%04X&DEV_%04X&SUBSYS_%08X",
                      Device->PciConfig.VendorID,
                      Device->PciConfig.DeviceID,
                      (Device->PciConfig.u.type0.SubSystemID << 16) +
                      Device->PciConfig.u.type0.SubVendorID);
    Index++;

    Index += swprintf(&Buffer[Index],
                      L"PCI\\VEN_%04X&DEV_%04X&CC_%02X%02X%02X",
                      Device->PciConfig.VendorID,
                      Device->PciConfig.DeviceID,
                      Device->PciConfig.BaseClass,
                      Device->PciConfig.SubClass,
                      Device->PciConfig.ProgIf);
    Index++;

    Index += swprintf(&Buffer[Index],
                      L"PCI\\VEN_%04X&DEV_%04X&CC_%02X%02X",
                      Device->PciConfig.VendorID,
                      Device->PciConfig.DeviceID,
                      Device->PciConfig.BaseClass,
                      Device->PciConfig.SubClass);
    Index++;

    Buffer[Index] = UNICODE_NULL;

    BufferU.Length = BufferU.MaximumLength = (USHORT) Index * sizeof(WCHAR);
    BufferU.Buffer = Buffer;

    return PciDuplicateUnicodeString(0, &BufferU, HardwareIDs);
}


NTSTATUS
PciCreateCompatibleIDsString(PUNICODE_STRING CompatibleIDs,
                             PPCI_DEVICE Device)
{
    WCHAR Buffer[256];
    UNICODE_STRING BufferU;
    ULONG Index;

    Index = 0;
    Index += swprintf(&Buffer[Index],
                      L"PCI\\VEN_%04X&DEV_%04X&REV_%02X",
                      Device->PciConfig.VendorID,
                      Device->PciConfig.DeviceID,
                      Device->PciConfig.RevisionID);
    Index++;

    Index += swprintf(&Buffer[Index],
                      L"PCI\\VEN_%04X&DEV_%04X",
                      Device->PciConfig.VendorID,
                      Device->PciConfig.DeviceID);
    Index++;

    Index += swprintf(&Buffer[Index],
                      L"PCI\\VEN_%04X&CC_%02X%02X%02X",
                      Device->PciConfig.VendorID,
                      Device->PciConfig.BaseClass,
                      Device->PciConfig.SubClass,
                      Device->PciConfig.ProgIf);
    Index++;

    Index += swprintf(&Buffer[Index],
                      L"PCI\\VEN_%04X&CC_%02X%02X",
                      Device->PciConfig.VendorID,
                      Device->PciConfig.BaseClass,
                      Device->PciConfig.SubClass);
    Index++;

    Index += swprintf(&Buffer[Index],
                      L"PCI\\VEN_%04X",
                      Device->PciConfig.VendorID);
    Index++;

    Index += swprintf(&Buffer[Index],
                      L"PCI\\CC_%02X%02X%02X",
                      Device->PciConfig.BaseClass,
                      Device->PciConfig.SubClass,
                      Device->PciConfig.ProgIf);
    Index++;

    Index += swprintf(&Buffer[Index],
                      L"PCI\\CC_%02X%02X",
                      Device->PciConfig.BaseClass,
                      Device->PciConfig.SubClass);
    Index++;

    Buffer[Index] = UNICODE_NULL;

    BufferU.Length = BufferU.MaximumLength = (USHORT)Index * sizeof(WCHAR);
    BufferU.Buffer = Buffer;

    return PciDuplicateUnicodeString(0, &BufferU, CompatibleIDs);
}


NTSTATUS
PciCreateDeviceDescriptionString(PUNICODE_STRING DeviceDescription,
                                 PPCI_DEVICE Device)
{
    PCWSTR Description;

    switch (Device->PciConfig.BaseClass)
    {
        case PCI_CLASS_PRE_20:
            switch (Device->PciConfig.SubClass)
            {
                case PCI_SUBCLASS_PRE_20_VGA:
                    Description = L"VGA device";
                    break;

                default:
                case PCI_SUBCLASS_PRE_20_NON_VGA:
                    Description = L"PCI device";
                    break;
            }
            break;

        case PCI_CLASS_MASS_STORAGE_CTLR:
            switch (Device->PciConfig.SubClass)
            {
                case PCI_SUBCLASS_MSC_SCSI_BUS_CTLR:
                    Description = L"SCSI controller";
                    break;

                case PCI_SUBCLASS_MSC_IDE_CTLR:
                    Description = L"IDE controller";
                    break;

                case PCI_SUBCLASS_MSC_FLOPPY_CTLR:
                    Description = L"Floppy disk controller";
                    break;

                case PCI_SUBCLASS_MSC_IPI_CTLR:
                    Description = L"IPI controller";
                    break;

                case PCI_SUBCLASS_MSC_RAID_CTLR:
                    Description = L"RAID controller";
                    break;

                default:
                    Description = L"Mass storage controller";
                    break;
            }
            break;

        case PCI_CLASS_NETWORK_CTLR:
            switch (Device->PciConfig.SubClass)
            {
                case PCI_SUBCLASS_NET_ETHERNET_CTLR:
                    Description = L"Ethernet controller";
                    break;

                case PCI_SUBCLASS_NET_TOKEN_RING_CTLR:
                    Description = L"Token-Ring controller";
                    break;

                case PCI_SUBCLASS_NET_FDDI_CTLR:
                    Description = L"FDDI controller";
                    break;

                case PCI_SUBCLASS_NET_ATM_CTLR:
                    Description = L"ATM controller";
                    break;

                default:
                    Description = L"Network controller";
                    break;
            }
            break;

        case PCI_CLASS_DISPLAY_CTLR:
            switch (Device->PciConfig.SubClass)
            {
                case PCI_SUBCLASS_VID_VGA_CTLR:
                    Description = L"VGA display controller";
                    break;

                case PCI_SUBCLASS_VID_XGA_CTLR:
                    Description = L"XGA display controller";
                    break;

                case PCI_SUBCLASS_VID_3D_CTLR:
                    Description = L"Multimedia display controller";
                    break;

                default:
                    Description = L"Other display controller";
                    break;
            }
            break;

        case PCI_CLASS_MULTIMEDIA_DEV:
            switch (Device->PciConfig.SubClass)
            {
                case PCI_SUBCLASS_MM_VIDEO_DEV:
                    Description = L"Multimedia video device";
                    break;

                case PCI_SUBCLASS_MM_AUDIO_DEV:
                    Description = L"Multimedia audio device";
                    break;

                case PCI_SUBCLASS_MM_TELEPHONY_DEV:
                    Description = L"Multimedia telephony device";
                    break;

                default:
                    Description = L"Other multimedia device";
                    break;
            }
            break;

        case PCI_CLASS_MEMORY_CTLR:
            switch (Device->PciConfig.SubClass)
            {
                case PCI_SUBCLASS_MEM_RAM:
                    Description = L"PCI Memory";
                    break;

                case PCI_SUBCLASS_MEM_FLASH:
                    Description = L"PCI Flash Memory";
                    break;

                default:
                    Description = L"Other memory controller";
                    break;
            }
            break;

        case PCI_CLASS_BRIDGE_DEV:
            switch (Device->PciConfig.SubClass)
            {
                case PCI_SUBCLASS_BR_HOST:
                    Description = L"PCI-Host bridge";
                    break;

                case PCI_SUBCLASS_BR_ISA:
                    Description = L"PCI-ISA bridge";
                    break;

                case PCI_SUBCLASS_BR_EISA:
                    Description = L"PCI-EISA bridge";
                    break;

                case PCI_SUBCLASS_BR_MCA:
                    Description = L"PCI-Micro Channel bridge";
                    break;

                case PCI_SUBCLASS_BR_PCI_TO_PCI:
                    Description = L"PCI-PCI bridge";
                    break;

                case PCI_SUBCLASS_BR_PCMCIA:
                    Description = L"PCI-PCMCIA bridge";
                    break;

                case PCI_SUBCLASS_BR_NUBUS:
                    Description = L"PCI-NUBUS bridge";
                    break;

                case PCI_SUBCLASS_BR_CARDBUS:
                    Description = L"PCI-CARDBUS bridge";
                    break;

                default:
                    Description = L"Other bridge device";
                    break;
            }
            break;

        case PCI_CLASS_SIMPLE_COMMS_CTLR:
            switch (Device->PciConfig.SubClass)
            {

                default:
                    Description = L"Communication device";
                    break;
            }
            break;

        case PCI_CLASS_BASE_SYSTEM_DEV:
            switch (Device->PciConfig.SubClass)
            {

                default:
                    Description = L"System device";
                    break;
            }
            break;

        case PCI_CLASS_INPUT_DEV:
            switch (Device->PciConfig.SubClass)
            {

                default:
                    Description = L"Input device";
                    break;
            }
            break;

        case PCI_CLASS_DOCKING_STATION:
            switch (Device->PciConfig.SubClass)
            {

                default:
                    Description = L"Docking station";
                    break;
            }
            break;

        case PCI_CLASS_PROCESSOR:
            switch (Device->PciConfig.SubClass)
            {

                default:
                    Description = L"Processor";
                    break;
            }
            break;

        case PCI_CLASS_SERIAL_BUS_CTLR:
            switch (Device->PciConfig.SubClass)
            {
                case PCI_SUBCLASS_SB_IEEE1394:
                    Description = L"FireWire controller";
                    break;

                case PCI_SUBCLASS_SB_ACCESS:
                    Description = L"ACCESS bus controller";
                    break;

                case PCI_SUBCLASS_SB_SSA:
                    Description = L"SSA controller";
                    break;

                case PCI_SUBCLASS_SB_USB:
                    Description = L"USB controller";
                    break;

                case PCI_SUBCLASS_SB_FIBRE_CHANNEL:
                    Description = L"Fibre Channel controller";
                    break;

                case PCI_SUBCLASS_SB_SMBUS:
                    Description = L"SMBus controller";
                    break;

                default:
                    Description = L"Other serial bus controller";
                    break;
            }
            break;

        default:
            Description = L"Other PCI Device";
            break;
    }

    return RtlCreateUnicodeString(DeviceDescription, Description) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}


NTSTATUS
PciCreateDeviceLocationString(PUNICODE_STRING DeviceLocation,
                              PPCI_DEVICE Device)
{
    WCHAR Buffer[256];

    swprintf(Buffer,
             L"PCI-Bus %lu, Device %u, Function %u",
             Device->BusNumber,
             Device->SlotNumber.u.bits.DeviceNumber,
             Device->SlotNumber.u.bits.FunctionNumber);

    return RtlCreateUnicodeString(DeviceLocation, Buffer) ? STATUS_SUCCESS : STATUS_INSUFFICIENT_RESOURCES;
}

NTSTATUS
PciDuplicateUnicodeString(
    IN ULONG Flags,
    IN PCUNICODE_STRING SourceString,
    OUT PUNICODE_STRING DestinationString)
{
    if (SourceString == NULL ||
        DestinationString == NULL ||
        SourceString->Length > SourceString->MaximumLength ||
        (SourceString->Length == 0 && SourceString->MaximumLength > 0 && SourceString->Buffer == NULL) ||
        Flags == RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING ||
        Flags >= 4)
    {
        return STATUS_INVALID_PARAMETER;
    }

    if ((SourceString->Length == 0) &&
        (Flags != (RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE |
                   RTL_DUPLICATE_UNICODE_STRING_ALLOCATE_NULL_STRING)))
    {
        DestinationString->Length = 0;
        DestinationString->MaximumLength = 0;
        DestinationString->Buffer = NULL;
    }
    else
    {
        USHORT DestMaxLength = SourceString->Length;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestMaxLength += sizeof(UNICODE_NULL);

        DestinationString->Buffer = ExAllocatePoolWithTag(PagedPool, DestMaxLength, TAG_PCI);
        if (DestinationString->Buffer == NULL)
            return STATUS_NO_MEMORY;

        RtlCopyMemory(DestinationString->Buffer, SourceString->Buffer, SourceString->Length);
        DestinationString->Length = SourceString->Length;
        DestinationString->MaximumLength = DestMaxLength;

        if (Flags & RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE)
            DestinationString->Buffer[DestinationString->Length / sizeof(WCHAR)] = 0;
    }

    return STATUS_SUCCESS;
}

/* EOF */
