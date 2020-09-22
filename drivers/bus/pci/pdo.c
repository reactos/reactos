/*
 * PROJECT:         ReactOS PCI bus driver
 * FILE:            pdo.c
 * PURPOSE:         Child device object dispatch routines
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      10-09-2001  CSH  Created
 */

#include "pci.h"

#include <initguid.h>
#include <wdmguid.h>

#define NDEBUG
#include <debug.h>

#if 0
#define DBGPRINT(...) DbgPrint(__VA_ARGS__)
#else
#define DBGPRINT(...)
#endif

#define PCI_ADDRESS_MEMORY_ADDRESS_MASK_64     0xfffffffffffffff0ull
#define PCI_ADDRESS_IO_ADDRESS_MASK_64         0xfffffffffffffffcull

/*** PRIVATE *****************************************************************/

static NTSTATUS
PdoQueryDeviceText(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;
    UNICODE_STRING String;
    NTSTATUS Status;

    DPRINT("Called\n");

    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    switch (IrpSp->Parameters.QueryDeviceText.DeviceTextType)
    {
        case DeviceTextDescription:
            Status = PciDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                               &DeviceExtension->DeviceDescription,
                                               &String);

            DPRINT("DeviceTextDescription\n");
            Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
            break;

        case DeviceTextLocationInformation:
            Status = PciDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                               &DeviceExtension->DeviceLocation,
                                               &String);

            DPRINT("DeviceTextLocationInformation\n");
            Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
            break;

        default:
            Irp->IoStatus.Information = 0;
            Status = STATUS_INVALID_PARAMETER;
            break;
    }

    return Status;
}


static NTSTATUS
PdoQueryId(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;
    UNICODE_STRING String;
    NTSTATUS Status;

    DPRINT("Called\n");

    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

//    Irp->IoStatus.Information = 0;

    Status = STATUS_SUCCESS;

    RtlInitUnicodeString(&String, NULL);

    switch (IrpSp->Parameters.QueryId.IdType)
    {
        case BusQueryDeviceID:
            Status = PciDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                               &DeviceExtension->DeviceID,
                                               &String);

            DPRINT("DeviceID: %S\n", String.Buffer);

            Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
            break;

        case BusQueryHardwareIDs:
            Status = PciDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                               &DeviceExtension->HardwareIDs,
                                               &String);

            Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
            break;

        case BusQueryCompatibleIDs:
            Status = PciDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                               &DeviceExtension->CompatibleIDs,
                                               &String);

            Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
            break;

        case BusQueryInstanceID:
            Status = PciDuplicateUnicodeString(RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
                                               &DeviceExtension->InstanceID,
                                               &String);

            DPRINT("InstanceID: %S\n", String.Buffer);

            Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
            break;

        case BusQueryDeviceSerialNumber:
        default:
            Status = STATUS_NOT_IMPLEMENTED;
    }

    return Status;
}


static NTSTATUS
PdoQueryBusInformation(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;
    PPNP_BUS_INFORMATION BusInformation;

    UNREFERENCED_PARAMETER(IrpSp);
    DPRINT("Called\n");

    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    BusInformation = ExAllocatePoolWithTag(PagedPool, sizeof(PNP_BUS_INFORMATION), TAG_PCI);
    Irp->IoStatus.Information = (ULONG_PTR)BusInformation;
    if (BusInformation != NULL)
    {
        BusInformation->BusTypeGuid = GUID_BUS_TYPE_PCI;
        BusInformation->LegacyBusType = PCIBus;
        BusInformation->BusNumber = DeviceExtension->PciDevice->BusNumber;

        return STATUS_SUCCESS;
    }

    return STATUS_INSUFFICIENT_RESOURCES;
}


static NTSTATUS
PdoQueryCapabilities(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;
    PDEVICE_CAPABILITIES DeviceCapabilities;
    ULONG DeviceNumber, FunctionNumber;

    UNREFERENCED_PARAMETER(Irp);
    DPRINT("Called\n");

    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
    DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

    if (DeviceCapabilities->Version != 1)
        return STATUS_UNSUCCESSFUL;

    DeviceNumber = DeviceExtension->PciDevice->SlotNumber.u.bits.DeviceNumber;
    FunctionNumber = DeviceExtension->PciDevice->SlotNumber.u.bits.FunctionNumber;

    DeviceCapabilities->UniqueID = FALSE;
    DeviceCapabilities->Address = ((DeviceNumber << 16) & 0xFFFF0000) + (FunctionNumber & 0xFFFF);
    DeviceCapabilities->UINumber = MAXULONG; /* FIXME */

    return STATUS_SUCCESS;
}

static BOOLEAN
PdoReadPciBar(PPDO_DEVICE_EXTENSION DeviceExtension,
              ULONG Offset,
              PULONG OriginalValue,
              PULONG NewValue)
{
    ULONG Size;
    ULONG AllOnes;

    /* Read the original value */
    Size = HalGetBusDataByOffset(PCIConfiguration,
                                 DeviceExtension->PciDevice->BusNumber,
                                 DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                                 OriginalValue,
                                 Offset,
                                 sizeof(ULONG));
    if (Size != sizeof(ULONG))
    {
        DPRINT1("Wrong size %lu\n", Size);
        return FALSE;
    }

    /* Write all ones to determine which bits are held to zero */
    AllOnes = MAXULONG;
    Size = HalSetBusDataByOffset(PCIConfiguration,
                                 DeviceExtension->PciDevice->BusNumber,
                                 DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                                 &AllOnes,
                                 Offset,
                                 sizeof(ULONG));
    if (Size != sizeof(ULONG))
    {
        DPRINT1("Wrong size %lu\n", Size);
        return FALSE;
    }

    /* Get the range length */
    Size = HalGetBusDataByOffset(PCIConfiguration,
                                 DeviceExtension->PciDevice->BusNumber,
                                 DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                                 NewValue,
                                 Offset,
                                 sizeof(ULONG));
    if (Size != sizeof(ULONG))
    {
        DPRINT1("Wrong size %lu\n", Size);
        return FALSE;
    }

    /* Restore original value */
    Size = HalSetBusDataByOffset(PCIConfiguration,
                                 DeviceExtension->PciDevice->BusNumber,
                                 DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                                 OriginalValue,
                                 Offset,
                                 sizeof(ULONG));
    if (Size != sizeof(ULONG))
    {
        DPRINT1("Wrong size %lu\n", Size);
        return FALSE;
    }

    return TRUE;
}

static BOOLEAN
PdoGetRangeLength(PPDO_DEVICE_EXTENSION DeviceExtension,
                  UCHAR Bar,
                  PULONGLONG Base,
                  PULONGLONG Length,
                  PULONG Flags,
                  PUCHAR NextBar,
                  PULONGLONG MaximumAddress)
{
    union {
        struct {
            ULONG Bar0;
            ULONG Bar1;
        } Bars;
        ULONGLONG Bar;
    } OriginalValue;
    union {
        struct {
            ULONG Bar0;
            ULONG Bar1;
        } Bars;
        ULONGLONG Bar;
    } NewValue;
    ULONG Offset;

    /* Compute the offset of this BAR in PCI config space */
    Offset = 0x10 + Bar * 4;

    /* Assume this is a 32-bit BAR until we find wrong */
    *NextBar = Bar + 1;

    /* Initialize BAR values to zero */
    OriginalValue.Bar = 0ULL;
    NewValue.Bar = 0ULL;

    /* Read the first BAR */
    if (!PdoReadPciBar(DeviceExtension, Offset,
                       &OriginalValue.Bars.Bar0,
                       &NewValue.Bars.Bar0))
    {
        return FALSE;
    }

    /* Check if this is a memory BAR */
    if (!(OriginalValue.Bars.Bar0 & PCI_ADDRESS_IO_SPACE))
    {
        /* Write the maximum address if the caller asked for it */
        if (MaximumAddress != NULL)
        {
            if ((OriginalValue.Bars.Bar0 & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_32BIT)
            {
                *MaximumAddress = 0x00000000FFFFFFFFULL;
            }
            else if ((OriginalValue.Bars.Bar0 & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_20BIT)
            {
                *MaximumAddress = 0x00000000000FFFFFULL;
            }
            else if ((OriginalValue.Bars.Bar0 & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)
            {
                *MaximumAddress = 0xFFFFFFFFFFFFFFFFULL;
            }
        }

        /* Check if this is a 64-bit BAR */
        if ((OriginalValue.Bars.Bar0 & PCI_ADDRESS_MEMORY_TYPE_MASK) == PCI_TYPE_64BIT)
        {
            /* We've now consumed the next BAR too */
            *NextBar = Bar + 2;

            /* Read the next BAR */
            if (!PdoReadPciBar(DeviceExtension, Offset + 4,
                               &OriginalValue.Bars.Bar1,
                               &NewValue.Bars.Bar1))
            {
                return FALSE;
            }
        }
    }
    else
    {
        /* Write the maximum I/O port address */
        if (MaximumAddress != NULL)
        {
            *MaximumAddress = 0x00000000FFFFFFFFULL;
        }
    }

    if (NewValue.Bar == 0)
    {
        DPRINT("Unused address register\n");
        *Base = 0;
        *Length = 0;
        *Flags = 0;
        return TRUE;
    }

    *Base = ((OriginalValue.Bar & PCI_ADDRESS_IO_SPACE)
             ? (OriginalValue.Bar & PCI_ADDRESS_IO_ADDRESS_MASK_64)
             : (OriginalValue.Bar & PCI_ADDRESS_MEMORY_ADDRESS_MASK_64));

    *Length = ~((NewValue.Bar & PCI_ADDRESS_IO_SPACE)
                ? (NewValue.Bar & PCI_ADDRESS_IO_ADDRESS_MASK_64)
                : (NewValue.Bar & PCI_ADDRESS_MEMORY_ADDRESS_MASK_64)) + 1;

    *Flags = (NewValue.Bar & PCI_ADDRESS_IO_SPACE)
             ? (NewValue.Bar & ~PCI_ADDRESS_IO_ADDRESS_MASK_64)
             : (NewValue.Bar & ~PCI_ADDRESS_MEMORY_ADDRESS_MASK_64);

    return TRUE;
}


static NTSTATUS
PdoQueryResourceRequirements(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;
    PCI_COMMON_CONFIG PciConfig;
    PIO_RESOURCE_REQUIREMENTS_LIST ResourceList;
    PIO_RESOURCE_DESCRIPTOR Descriptor;
    ULONG Size;
    ULONG ResCount = 0;
    ULONG ListSize;
    UCHAR Bar;
    ULONGLONG Base;
    ULONGLONG Length;
    ULONG Flags;
    ULONGLONG MaximumAddress;

    UNREFERENCED_PARAMETER(IrpSp);
    DPRINT("PdoQueryResourceRequirements() called\n");

    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* Get PCI configuration space */
    Size= HalGetBusData(PCIConfiguration,
                        DeviceExtension->PciDevice->BusNumber,
                        DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                        &PciConfig,
                        PCI_COMMON_HDR_LENGTH);
    DPRINT("Size %lu\n", Size);
    if (Size < PCI_COMMON_HDR_LENGTH)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_UNSUCCESSFUL;
    }

    DPRINT("Command register: 0x%04hx\n", PciConfig.Command);

    /* Count required resource descriptors */
    ResCount = 0;
    if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_DEVICE_TYPE)
    {
        for (Bar = 0; Bar < PCI_TYPE0_ADDRESSES;)
        {
            if (!PdoGetRangeLength(DeviceExtension,
                                   Bar,
                                   &Base,
                                   &Length,
                                   &Flags,
                                   &Bar,
                                   NULL))
                break;

            if (Length != 0)
                ResCount += 2;
        }

        /* FIXME: Check ROM address */

        if (PciConfig.u.type0.InterruptPin != 0)
            ResCount++;
    }
    else if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_BRIDGE_TYPE)
    {
        for (Bar = 0; Bar < PCI_TYPE1_ADDRESSES;)
        {
            if (!PdoGetRangeLength(DeviceExtension,
                                   Bar,
                                   &Base,
                                   &Length,
                                   &Flags,
                                   &Bar,
                                   NULL))
                break;

            if (Length != 0)
                ResCount += 2;
        }

        if (DeviceExtension->PciDevice->PciConfig.BaseClass == PCI_CLASS_BRIDGE_DEV)
            ResCount++;
    }
    else if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_CARDBUS_BRIDGE_TYPE)
    {
        /* FIXME: Count Cardbus bridge resources */
    }
    else
    {
        DPRINT1("Unsupported header type %d\n", PCI_CONFIGURATION_TYPE(&PciConfig));
    }

    if (ResCount == 0)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }

    /* Calculate the resource list size */
    ListSize = FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List[0].Descriptors) +
               ResCount * sizeof(IO_RESOURCE_DESCRIPTOR);

    DPRINT("ListSize %lu (0x%lx)\n", ListSize, ListSize);

    /* Allocate the resource requirements list */
    ResourceList = ExAllocatePoolWithTag(PagedPool,
                                         ListSize,
                                         TAG_PCI);
    if (ResourceList == NULL)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_INSUFFICIENT_RESOURCES;
    }

    RtlZeroMemory(ResourceList, ListSize);
    ResourceList->ListSize = ListSize;
    ResourceList->InterfaceType = PCIBus;
    ResourceList->BusNumber = DeviceExtension->PciDevice->BusNumber;
    ResourceList->SlotNumber = DeviceExtension->PciDevice->SlotNumber.u.AsULONG;
    ResourceList->AlternativeLists = 1;

    ResourceList->List[0].Version = 1;
    ResourceList->List[0].Revision = 1;
    ResourceList->List[0].Count = ResCount;

    Descriptor = &ResourceList->List[0].Descriptors[0];
    if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_DEVICE_TYPE)
    {
        for (Bar = 0; Bar < PCI_TYPE0_ADDRESSES;)
        {
            if (!PdoGetRangeLength(DeviceExtension,
                                   Bar,
                                   &Base,
                                   &Length,
                                   &Flags,
                                   &Bar,
                                   &MaximumAddress))
            {
                DPRINT1("PdoGetRangeLength() failed\n");
                break;
            }

            if (Length == 0)
            {
                DPRINT("Unused address register\n");
                continue;
            }

            /* Set preferred descriptor */
            Descriptor->Option = IO_RESOURCE_PREFERRED;
            if (Flags & PCI_ADDRESS_IO_SPACE)
            {
                Descriptor->Type = CmResourceTypePort;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_PORT_IO |
                                    CM_RESOURCE_PORT_16_BIT_DECODE |
                                    CM_RESOURCE_PORT_POSITIVE_DECODE;

                Descriptor->u.Port.Length = Length;
                Descriptor->u.Port.Alignment = 1;
                Descriptor->u.Port.MinimumAddress.QuadPart = Base;
                Descriptor->u.Port.MaximumAddress.QuadPart = Base + Length - 1;
            }
            else
            {
                Descriptor->Type = CmResourceTypeMemory;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE |
                    (Flags & PCI_ADDRESS_MEMORY_PREFETCHABLE) ? CM_RESOURCE_MEMORY_PREFETCHABLE : 0;

                Descriptor->u.Memory.Length = Length;
                Descriptor->u.Memory.Alignment = 1;
                Descriptor->u.Memory.MinimumAddress.QuadPart = Base;
                Descriptor->u.Memory.MaximumAddress.QuadPart = Base + Length - 1;
            }
            Descriptor++;

            /* Set alternative descriptor */
            Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
            if (Flags & PCI_ADDRESS_IO_SPACE)
            {
                Descriptor->Type = CmResourceTypePort;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_PORT_IO |
                                    CM_RESOURCE_PORT_16_BIT_DECODE |
                                    CM_RESOURCE_PORT_POSITIVE_DECODE;

                Descriptor->u.Port.Length = Length;
                Descriptor->u.Port.Alignment = Length;
                Descriptor->u.Port.MinimumAddress.QuadPart = 0;
                Descriptor->u.Port.MaximumAddress.QuadPart = MaximumAddress;
            }
            else
            {
                Descriptor->Type = CmResourceTypeMemory;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE |
                    (Flags & PCI_ADDRESS_MEMORY_PREFETCHABLE) ? CM_RESOURCE_MEMORY_PREFETCHABLE : 0;

                Descriptor->u.Memory.Length = Length;
                Descriptor->u.Memory.Alignment = Length;
                Descriptor->u.Port.MinimumAddress.QuadPart = 0;
                Descriptor->u.Port.MaximumAddress.QuadPart = MaximumAddress;
            }
            Descriptor++;
        }

        /* FIXME: Check ROM address */

        if (PciConfig.u.type0.InterruptPin != 0)
        {
            Descriptor->Option = 0; /* Required */
            Descriptor->Type = CmResourceTypeInterrupt;
            Descriptor->ShareDisposition = CmResourceShareShared;
            Descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;

            Descriptor->u.Interrupt.MinimumVector = 0;
            Descriptor->u.Interrupt.MaximumVector = 0xFF;
        }
    }
    else if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_BRIDGE_TYPE)
    {
        for (Bar = 0; Bar < PCI_TYPE1_ADDRESSES;)
        {
            if (!PdoGetRangeLength(DeviceExtension,
                                   Bar,
                                   &Base,
                                   &Length,
                                   &Flags,
                                   &Bar,
                                   &MaximumAddress))
            {
                DPRINT1("PdoGetRangeLength() failed\n");
                break;
            }

            if (Length == 0)
            {
                DPRINT("Unused address register\n");
                continue;
            }

            /* Set preferred descriptor */
            Descriptor->Option = IO_RESOURCE_PREFERRED;
            if (Flags & PCI_ADDRESS_IO_SPACE)
            {
                Descriptor->Type = CmResourceTypePort;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_PORT_IO |
                                    CM_RESOURCE_PORT_16_BIT_DECODE |
                                    CM_RESOURCE_PORT_POSITIVE_DECODE;

                Descriptor->u.Port.Length = Length;
                Descriptor->u.Port.Alignment = 1;
                Descriptor->u.Port.MinimumAddress.QuadPart = Base;
                Descriptor->u.Port.MaximumAddress.QuadPart = Base + Length - 1;
            }
            else
            {
                Descriptor->Type = CmResourceTypeMemory;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE |
                    (Flags & PCI_ADDRESS_MEMORY_PREFETCHABLE) ? CM_RESOURCE_MEMORY_PREFETCHABLE : 0;

                Descriptor->u.Memory.Length = Length;
                Descriptor->u.Memory.Alignment = 1;
                Descriptor->u.Memory.MinimumAddress.QuadPart = Base;
                Descriptor->u.Memory.MaximumAddress.QuadPart = Base + Length - 1;
            }
            Descriptor++;

            /* Set alternative descriptor */
            Descriptor->Option = IO_RESOURCE_ALTERNATIVE;
            if (Flags & PCI_ADDRESS_IO_SPACE)
            {
                Descriptor->Type = CmResourceTypePort;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_PORT_IO |
                                    CM_RESOURCE_PORT_16_BIT_DECODE |
                                    CM_RESOURCE_PORT_POSITIVE_DECODE;

                Descriptor->u.Port.Length = Length;
                Descriptor->u.Port.Alignment = Length;
                Descriptor->u.Port.MinimumAddress.QuadPart = 0;
                Descriptor->u.Port.MaximumAddress.QuadPart = MaximumAddress;
            }
            else
            {
                Descriptor->Type = CmResourceTypeMemory;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE |
                    (Flags & PCI_ADDRESS_MEMORY_PREFETCHABLE) ? CM_RESOURCE_MEMORY_PREFETCHABLE : 0;

                Descriptor->u.Memory.Length = Length;
                Descriptor->u.Memory.Alignment = Length;
                Descriptor->u.Port.MinimumAddress.QuadPart = 0;
                Descriptor->u.Port.MaximumAddress.QuadPart = MaximumAddress;
            }
            Descriptor++;
        }

        if (DeviceExtension->PciDevice->PciConfig.BaseClass == PCI_CLASS_BRIDGE_DEV)
        {
            Descriptor->Option = 0; /* Required */
            Descriptor->Type = CmResourceTypeBusNumber;
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;

            ResourceList->BusNumber =
            Descriptor->u.BusNumber.MinBusNumber =
            Descriptor->u.BusNumber.MaxBusNumber = DeviceExtension->PciDevice->PciConfig.u.type1.SecondaryBus;
            Descriptor->u.BusNumber.Length = 1;
            Descriptor->u.BusNumber.Reserved = 0;
        }
    }
    else if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_CARDBUS_BRIDGE_TYPE)
    {
        /* FIXME: Add Cardbus bridge resources */
    }

    Irp->IoStatus.Information = (ULONG_PTR)ResourceList;

    return STATUS_SUCCESS;
}


static NTSTATUS
PdoQueryResources(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;
    PCI_COMMON_CONFIG PciConfig;
    PCM_RESOURCE_LIST ResourceList;
    PCM_PARTIAL_RESOURCE_LIST PartialList;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR Descriptor;
    ULONG Size;
    ULONG ResCount = 0;
    ULONG ListSize;
    UCHAR Bar;
    ULONGLONG Base;
    ULONGLONG Length;
    ULONG Flags;

    DPRINT("PdoQueryResources() called\n");

    UNREFERENCED_PARAMETER(IrpSp);
    DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

    /* Get PCI configuration space */
    Size= HalGetBusData(PCIConfiguration,
                        DeviceExtension->PciDevice->BusNumber,
                        DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                        &PciConfig,
                        PCI_COMMON_HDR_LENGTH);
    DPRINT("Size %lu\n", Size);
    if (Size < PCI_COMMON_HDR_LENGTH)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_UNSUCCESSFUL;
    }

    DPRINT("Command register: 0x%04hx\n", PciConfig.Command);

    /* Count required resource descriptors */
    ResCount = 0;
    if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_DEVICE_TYPE)
    {
        for (Bar = 0; Bar < PCI_TYPE0_ADDRESSES;)
        {
            if (!PdoGetRangeLength(DeviceExtension,
                                   Bar,
                                   &Base,
                                   &Length,
                                   &Flags,
                                   &Bar,
                                   NULL))
                break;

            if (Length)
                ResCount++;
        }

        if ((PciConfig.u.type0.InterruptPin != 0) &&
            (PciConfig.u.type0.InterruptLine != 0) &&
            (PciConfig.u.type0.InterruptLine != 0xFF))
            ResCount++;
    }
    else if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_BRIDGE_TYPE)
    {
        for (Bar = 0; Bar < PCI_TYPE1_ADDRESSES;)
        {
            if (!PdoGetRangeLength(DeviceExtension,
                                   Bar,
                                   &Base,
                                   &Length,
                                   &Flags,
                                   &Bar,
                                   NULL))
                break;

            if (Length != 0)
                ResCount++;
        }

        if (DeviceExtension->PciDevice->PciConfig.BaseClass == PCI_CLASS_BRIDGE_DEV)
            ResCount++;
    }
    else if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_CARDBUS_BRIDGE_TYPE)
    {
        /* FIXME: Count Cardbus bridge resources */
    }
    else
    {
        DPRINT1("Unsupported header type %d\n", PCI_CONFIGURATION_TYPE(&PciConfig));
    }

    if (ResCount == 0)
    {
        Irp->IoStatus.Information = 0;
        return STATUS_SUCCESS;
    }

    /* Calculate the resource list size */
    ListSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors) +
               ResCount * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

    /* Allocate the resource list */
    ResourceList = ExAllocatePoolWithTag(PagedPool,
                                         ListSize,
                                         TAG_PCI);
    if (ResourceList == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    RtlZeroMemory(ResourceList, ListSize);
    ResourceList->Count = 1;
    ResourceList->List[0].InterfaceType = PCIBus;
    ResourceList->List[0].BusNumber = DeviceExtension->PciDevice->BusNumber;

    PartialList = &ResourceList->List[0].PartialResourceList;
    PartialList->Version = 1;
    PartialList->Revision = 1;
    PartialList->Count = ResCount;

    Descriptor = &PartialList->PartialDescriptors[0];
    if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_DEVICE_TYPE)
    {
        for (Bar = 0; Bar < PCI_TYPE0_ADDRESSES;)
        {
            if (!PdoGetRangeLength(DeviceExtension,
                                   Bar,
                                   &Base,
                                   &Length,
                                   &Flags,
                                   &Bar,
                                   NULL))
                break;

            if (Length == 0)
            {
                DPRINT("Unused address register\n");
                continue;
            }

            if (Flags & PCI_ADDRESS_IO_SPACE)
            {
                Descriptor->Type = CmResourceTypePort;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_PORT_IO |
                                    CM_RESOURCE_PORT_16_BIT_DECODE |
                                    CM_RESOURCE_PORT_POSITIVE_DECODE;
                Descriptor->u.Port.Start.QuadPart = (ULONGLONG)Base;
                Descriptor->u.Port.Length = Length;

                /* Enable IO space access */
                DeviceExtension->PciDevice->EnableIoSpace = TRUE;
            }
            else
            {
                Descriptor->Type = CmResourceTypeMemory;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE |
                    (Flags & PCI_ADDRESS_MEMORY_PREFETCHABLE) ? CM_RESOURCE_MEMORY_PREFETCHABLE : 0;
                Descriptor->u.Memory.Start.QuadPart = (ULONGLONG)Base;
                Descriptor->u.Memory.Length = Length;

                /* Enable memory space access */
                DeviceExtension->PciDevice->EnableMemorySpace = TRUE;
            }

            Descriptor++;
        }

        /* Add interrupt resource */
        if ((PciConfig.u.type0.InterruptPin != 0) &&
            (PciConfig.u.type0.InterruptLine != 0) &&
            (PciConfig.u.type0.InterruptLine != 0xFF))
        {
            Descriptor->Type = CmResourceTypeInterrupt;
            Descriptor->ShareDisposition = CmResourceShareShared;
            Descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
            Descriptor->u.Interrupt.Level = PciConfig.u.type0.InterruptLine;
            Descriptor->u.Interrupt.Vector = PciConfig.u.type0.InterruptLine;
            Descriptor->u.Interrupt.Affinity = 0xFFFFFFFF;
        }

        /* Allow bus master mode */
       DeviceExtension->PciDevice->EnableBusMaster = TRUE;
    }
    else if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_BRIDGE_TYPE)
    {
        for (Bar = 0; Bar < PCI_TYPE1_ADDRESSES;)
        {
            if (!PdoGetRangeLength(DeviceExtension,
                                   Bar,
                                   &Base,
                                   &Length,
                                   &Flags,
                                   &Bar,
                                   NULL))
                break;

            if (Length == 0)
            {
                DPRINT("Unused address register\n");
                continue;
            }

            if (Flags & PCI_ADDRESS_IO_SPACE)
            {
                Descriptor->Type = CmResourceTypePort;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_PORT_IO |
                                    CM_RESOURCE_PORT_16_BIT_DECODE |
                                    CM_RESOURCE_PORT_POSITIVE_DECODE;
                Descriptor->u.Port.Start.QuadPart = (ULONGLONG)Base;
                Descriptor->u.Port.Length = Length;

                /* Enable IO space access */
                DeviceExtension->PciDevice->EnableIoSpace = TRUE;
            }
            else
            {
                Descriptor->Type = CmResourceTypeMemory;
                Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
                Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE |
                    (Flags & PCI_ADDRESS_MEMORY_PREFETCHABLE) ? CM_RESOURCE_MEMORY_PREFETCHABLE : 0;
                Descriptor->u.Memory.Start.QuadPart = (ULONGLONG)Base;
                Descriptor->u.Memory.Length = Length;

                /* Enable memory space access */
                DeviceExtension->PciDevice->EnableMemorySpace = TRUE;
            }

            Descriptor++;
        }

        if (DeviceExtension->PciDevice->PciConfig.BaseClass == PCI_CLASS_BRIDGE_DEV)
        {
            Descriptor->Type = CmResourceTypeBusNumber;
            Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;

            ResourceList->List[0].BusNumber =
            Descriptor->u.BusNumber.Start = DeviceExtension->PciDevice->PciConfig.u.type1.SecondaryBus;
            Descriptor->u.BusNumber.Length = 1;
            Descriptor->u.BusNumber.Reserved = 0;
        }
    }
    else if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_CARDBUS_BRIDGE_TYPE)
    {
        /* FIXME: Add Cardbus bridge resources */
    }

    Irp->IoStatus.Information = (ULONG_PTR)ResourceList;

    return STATUS_SUCCESS;
}


static VOID NTAPI
InterfaceReference(
    IN PVOID Context)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;

    DPRINT("InterfaceReference(%p)\n", Context);

    DeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)Context)->DeviceExtension;
    InterlockedIncrement(&DeviceExtension->References);
}


static VOID NTAPI
InterfaceDereference(
    IN PVOID Context)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;

    DPRINT("InterfaceDereference(%p)\n", Context);

    DeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)Context)->DeviceExtension;
    InterlockedDecrement(&DeviceExtension->References);
}

static TRANSLATE_BUS_ADDRESS InterfaceBusTranslateBusAddress;

static
BOOLEAN
NTAPI
InterfaceBusTranslateBusAddress(
    IN PVOID Context,
    IN PHYSICAL_ADDRESS BusAddress,
    IN ULONG Length,
    IN OUT PULONG AddressSpace,
    OUT PPHYSICAL_ADDRESS TranslatedAddress)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;

    DPRINT("InterfaceBusTranslateBusAddress(%p %p 0x%lx %p %p)\n",
           Context, BusAddress, Length, AddressSpace, TranslatedAddress);

    DeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)Context)->DeviceExtension;

    return HalTranslateBusAddress(PCIBus,
                                  DeviceExtension->PciDevice->BusNumber,
                                  BusAddress,
                                  AddressSpace,
                                  TranslatedAddress);
}

static GET_DMA_ADAPTER InterfaceBusGetDmaAdapter;

static
PDMA_ADAPTER
NTAPI
InterfaceBusGetDmaAdapter(
    IN PVOID Context,
    IN PDEVICE_DESCRIPTION DeviceDescription,
    OUT PULONG NumberOfMapRegisters)
{
    DPRINT("InterfaceBusGetDmaAdapter(%p %p %p)\n",
           Context, DeviceDescription, NumberOfMapRegisters);
    return (PDMA_ADAPTER)HalGetAdapter(DeviceDescription, NumberOfMapRegisters);
}

static GET_SET_DEVICE_DATA InterfaceBusSetBusData;

static
ULONG
NTAPI
InterfaceBusSetBusData(
    IN PVOID Context,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;
    ULONG Size;

    DPRINT("InterfaceBusSetBusData(%p 0x%lx %p 0x%lx 0x%lx)\n",
           Context, DataType, Buffer, Offset, Length);

    if (DataType != PCI_WHICHSPACE_CONFIG)
    {
        DPRINT("Unknown DataType %lu\n", DataType);
        return 0;
    }

    DeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)Context)->DeviceExtension;

    /* Get PCI configuration space */
    Size = HalSetBusDataByOffset(PCIConfiguration,
                                 DeviceExtension->PciDevice->BusNumber,
                                 DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                                 Buffer,
                                 Offset,
                                 Length);
    return Size;
}

static GET_SET_DEVICE_DATA InterfaceBusGetBusData;

static
ULONG
NTAPI
InterfaceBusGetBusData(
    IN PVOID Context,
    IN ULONG DataType,
    IN PVOID Buffer,
    IN ULONG Offset,
    IN ULONG Length)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;
    ULONG Size;

    DPRINT("InterfaceBusGetBusData(%p 0x%lx %p 0x%lx 0x%lx) called\n",
           Context, DataType, Buffer, Offset, Length);

    if (DataType != PCI_WHICHSPACE_CONFIG)
    {
        DPRINT("Unknown DataType %lu\n", DataType);
        return 0;
    }

    DeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)Context)->DeviceExtension;

    /* Get PCI configuration space */
    Size = HalGetBusDataByOffset(PCIConfiguration,
                                 DeviceExtension->PciDevice->BusNumber,
                                 DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                                 Buffer,
                                 Offset,
                                 Length);
    return Size;
}


static BOOLEAN NTAPI
InterfacePciDevicePresent(
    IN USHORT VendorID,
    IN USHORT DeviceID,
    IN UCHAR RevisionID,
    IN USHORT SubVendorID,
    IN USHORT SubSystemID,
    IN ULONG Flags)
{
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PPCI_DEVICE PciDevice;
    PLIST_ENTRY CurrentBus, CurrentEntry;
    KIRQL OldIrql;
    BOOLEAN Found = FALSE;

    KeAcquireSpinLock(&DriverExtension->BusListLock, &OldIrql);
    CurrentBus = DriverExtension->BusListHead.Flink;
    while (!Found && CurrentBus != &DriverExtension->BusListHead)
    {
        FdoDeviceExtension = CONTAINING_RECORD(CurrentBus, FDO_DEVICE_EXTENSION, ListEntry);

        KeAcquireSpinLockAtDpcLevel(&FdoDeviceExtension->DeviceListLock);
        CurrentEntry = FdoDeviceExtension->DeviceListHead.Flink;
        while (!Found && CurrentEntry != &FdoDeviceExtension->DeviceListHead)
        {
            PciDevice = CONTAINING_RECORD(CurrentEntry, PCI_DEVICE, ListEntry);
            if (PciDevice->PciConfig.VendorID == VendorID &&
                PciDevice->PciConfig.DeviceID == DeviceID)
            {
                if (!(Flags & PCI_USE_SUBSYSTEM_IDS) ||
                    (PciDevice->PciConfig.u.type0.SubVendorID == SubVendorID &&
                     PciDevice->PciConfig.u.type0.SubSystemID == SubSystemID))
                {
                    if (!(Flags & PCI_USE_REVISION) ||
                        PciDevice->PciConfig.RevisionID == RevisionID)
                    {
                        DPRINT("Found the PCI device\n");
                        Found = TRUE;
                    }
                }
            }

            CurrentEntry = CurrentEntry->Flink;
        }

        KeReleaseSpinLockFromDpcLevel(&FdoDeviceExtension->DeviceListLock);
        CurrentBus = CurrentBus->Flink;
    }
    KeReleaseSpinLock(&DriverExtension->BusListLock, OldIrql);

    return Found;
}


static BOOLEAN
CheckPciDevice(
    IN PPCI_COMMON_CONFIG PciConfig,
    IN PPCI_DEVICE_PRESENCE_PARAMETERS Parameters)
{
    if ((Parameters->Flags & PCI_USE_VENDEV_IDS) &&
        (PciConfig->VendorID != Parameters->VendorID ||
         PciConfig->DeviceID != Parameters->DeviceID))
    {
        return FALSE;
    }

    if ((Parameters->Flags & PCI_USE_CLASS_SUBCLASS) &&
        (PciConfig->BaseClass != Parameters->BaseClass ||
         PciConfig->SubClass != Parameters->SubClass))
    {
        return FALSE;
    }

    if ((Parameters->Flags & PCI_USE_PROGIF) &&
         PciConfig->ProgIf != Parameters->ProgIf)
    {
        return FALSE;
    }

    if ((Parameters->Flags & PCI_USE_SUBSYSTEM_IDS) &&
        (PciConfig->u.type0.SubVendorID != Parameters->SubVendorID ||
         PciConfig->u.type0.SubSystemID != Parameters->SubSystemID))
    {
        return FALSE;
    }

    if ((Parameters->Flags & PCI_USE_REVISION) &&
        PciConfig->RevisionID != Parameters->RevisionID)
    {
        return FALSE;
    }

    return TRUE;
}


static BOOLEAN NTAPI
InterfacePciDevicePresentEx(
    IN PVOID Context,
    IN PPCI_DEVICE_PRESENCE_PARAMETERS Parameters)
{
    PPDO_DEVICE_EXTENSION DeviceExtension;
    PFDO_DEVICE_EXTENSION MyFdoDeviceExtension;
    PFDO_DEVICE_EXTENSION FdoDeviceExtension;
    PPCI_DEVICE PciDevice;
    PLIST_ENTRY CurrentBus, CurrentEntry;
    KIRQL OldIrql;
    BOOLEAN Found = FALSE;

    DPRINT("InterfacePciDevicePresentEx(%p %p) called\n",
           Context, Parameters);

    if (!Parameters || Parameters->Size != sizeof(PCI_DEVICE_PRESENCE_PARAMETERS))
        return FALSE;

    DeviceExtension = (PPDO_DEVICE_EXTENSION)((PDEVICE_OBJECT)Context)->DeviceExtension;
    MyFdoDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceExtension->Fdo->DeviceExtension;

    if (Parameters->Flags & PCI_USE_LOCAL_DEVICE)
    {
        return CheckPciDevice(&DeviceExtension->PciDevice->PciConfig, Parameters);
    }

    KeAcquireSpinLock(&DriverExtension->BusListLock, &OldIrql);
    CurrentBus = DriverExtension->BusListHead.Flink;
    while (!Found && CurrentBus != &DriverExtension->BusListHead)
    {
        FdoDeviceExtension = CONTAINING_RECORD(CurrentBus, FDO_DEVICE_EXTENSION, ListEntry);
        if (!(Parameters->Flags & PCI_USE_LOCAL_BUS) || FdoDeviceExtension == MyFdoDeviceExtension)
        {
            KeAcquireSpinLockAtDpcLevel(&FdoDeviceExtension->DeviceListLock);
            CurrentEntry = FdoDeviceExtension->DeviceListHead.Flink;
            while (!Found && CurrentEntry != &FdoDeviceExtension->DeviceListHead)
            {
                PciDevice = CONTAINING_RECORD(CurrentEntry, PCI_DEVICE, ListEntry);

                if (CheckPciDevice(&PciDevice->PciConfig, Parameters))
                {
                    DPRINT("Found the PCI device\n");
                    Found = TRUE;
                }

                CurrentEntry = CurrentEntry->Flink;
            }

            KeReleaseSpinLockFromDpcLevel(&FdoDeviceExtension->DeviceListLock);
        }
        CurrentBus = CurrentBus->Flink;
    }
    KeReleaseSpinLock(&DriverExtension->BusListLock, OldIrql);

    return Found;
}


static NTSTATUS
PdoQueryInterface(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    NTSTATUS Status;

    UNREFERENCED_PARAMETER(Irp);

    if (RtlCompareMemory(IrpSp->Parameters.QueryInterface.InterfaceType,
                         &GUID_BUS_INTERFACE_STANDARD, sizeof(GUID)) == sizeof(GUID))
    {
        /* BUS_INTERFACE_STANDARD */
        if (IrpSp->Parameters.QueryInterface.Version < 1)
            Status = STATUS_NOT_SUPPORTED;
        else if (IrpSp->Parameters.QueryInterface.Size < sizeof(BUS_INTERFACE_STANDARD))
            Status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            PBUS_INTERFACE_STANDARD BusInterface;
            BusInterface = (PBUS_INTERFACE_STANDARD)IrpSp->Parameters.QueryInterface.Interface;
            BusInterface->Size = sizeof(BUS_INTERFACE_STANDARD);
            BusInterface->Version = 1;
            BusInterface->TranslateBusAddress = InterfaceBusTranslateBusAddress;
            BusInterface->GetDmaAdapter = InterfaceBusGetDmaAdapter;
            BusInterface->SetBusData = InterfaceBusSetBusData;
            BusInterface->GetBusData = InterfaceBusGetBusData;
            Status = STATUS_SUCCESS;
        }
    }
    else if (RtlCompareMemory(IrpSp->Parameters.QueryInterface.InterfaceType,
                              &GUID_PCI_DEVICE_PRESENT_INTERFACE, sizeof(GUID)) == sizeof(GUID))
    {
        /* PCI_DEVICE_PRESENT_INTERFACE */
        if (IrpSp->Parameters.QueryInterface.Version < 1)
            Status = STATUS_NOT_SUPPORTED;
        else if (IrpSp->Parameters.QueryInterface.Size < sizeof(PCI_DEVICE_PRESENT_INTERFACE))
            Status = STATUS_BUFFER_TOO_SMALL;
        else
        {
            PPCI_DEVICE_PRESENT_INTERFACE PciDevicePresentInterface;
            PciDevicePresentInterface = (PPCI_DEVICE_PRESENT_INTERFACE)IrpSp->Parameters.QueryInterface.Interface;
            PciDevicePresentInterface->Size = sizeof(PCI_DEVICE_PRESENT_INTERFACE);
            PciDevicePresentInterface->Version = 1;
            PciDevicePresentInterface->IsDevicePresent = InterfacePciDevicePresent;
            PciDevicePresentInterface->IsDevicePresentEx = InterfacePciDevicePresentEx;
            Status = STATUS_SUCCESS;
        }
    }
    else
    {
        /* Not a supported interface */
        return STATUS_NOT_SUPPORTED;
    }

    if (NT_SUCCESS(Status))
    {
        /* Add a reference for the returned interface */
        PINTERFACE Interface;
        Interface = (PINTERFACE)IrpSp->Parameters.QueryInterface.Interface;
        Interface->Context = DeviceObject;
        Interface->InterfaceReference = InterfaceReference;
        Interface->InterfaceDereference = InterfaceDereference;
        Interface->InterfaceReference(Interface->Context);
    }

    return Status;
}

static NTSTATUS
PdoStartDevice(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    PCM_RESOURCE_LIST RawResList = IrpSp->Parameters.StartDevice.AllocatedResources;
    PCM_FULL_RESOURCE_DESCRIPTOR RawFullDesc;
    PCM_PARTIAL_RESOURCE_DESCRIPTOR RawPartialDesc;
    ULONG i, ii;
    PPDO_DEVICE_EXTENSION DeviceExtension = DeviceObject->DeviceExtension;
    UCHAR Irq;
    USHORT Command;

    UNREFERENCED_PARAMETER(Irp);

    if (!RawResList)
        return STATUS_SUCCESS;

    /* TODO: Assign the other resources we get to the card */

    RawFullDesc = &RawResList->List[0];
    for (i = 0; i < RawResList->Count; i++, RawFullDesc = CmiGetNextResourceDescriptor(RawFullDesc))
    {
        for (ii = 0; ii < RawFullDesc->PartialResourceList.Count; ii++)
        {
            /* Partial resource descriptors can be of variable size (CmResourceTypeDeviceSpecific),
               but only one is allowed and it must be the last one in the list! */
            RawPartialDesc = &RawFullDesc->PartialResourceList.PartialDescriptors[ii];

            if (RawPartialDesc->Type == CmResourceTypeInterrupt)
            {
                DPRINT("Assigning IRQ %u to PCI device 0x%x on bus 0x%x\n",
                        RawPartialDesc->u.Interrupt.Vector,
                        DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                        DeviceExtension->PciDevice->BusNumber);

                Irq = (UCHAR)RawPartialDesc->u.Interrupt.Vector;
                HalSetBusDataByOffset(PCIConfiguration,
                                      DeviceExtension->PciDevice->BusNumber,
                                      DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                                      &Irq,
                                      0x3c /* PCI_INTERRUPT_LINE */,
                                      sizeof(UCHAR));
            }
        }
    }

    Command = 0;

    DBGPRINT("pci!PdoStartDevice: Enabling command flags for PCI device 0x%x on bus 0x%x: ",
            DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
            DeviceExtension->PciDevice->BusNumber);
    if (DeviceExtension->PciDevice->EnableBusMaster)
    {
        Command |= PCI_ENABLE_BUS_MASTER;
        DBGPRINT("[Bus master] ");
    }

    if (DeviceExtension->PciDevice->EnableMemorySpace)
    {
        Command |= PCI_ENABLE_MEMORY_SPACE;
        DBGPRINT("[Memory space enable] ");
    }

    if (DeviceExtension->PciDevice->EnableIoSpace)
    {
        Command |= PCI_ENABLE_IO_SPACE;
        DBGPRINT("[I/O space enable] ");
    }

    if (Command != 0)
    {
        DBGPRINT("\n");

        /* OR with the previous value */
        Command |= DeviceExtension->PciDevice->PciConfig.Command;

        HalSetBusDataByOffset(PCIConfiguration,
                              DeviceExtension->PciDevice->BusNumber,
                              DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                              &Command,
                              FIELD_OFFSET(PCI_COMMON_CONFIG, Command),
                              sizeof(USHORT));
    }
    else
    {
        DBGPRINT("None\n");
    }

    return STATUS_SUCCESS;
}

static NTSTATUS
PdoReadConfig(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    ULONG Size;

    DPRINT("PdoReadConfig() called\n");

    Size = InterfaceBusGetBusData(DeviceObject,
                                  IrpSp->Parameters.ReadWriteConfig.WhichSpace,
                                  IrpSp->Parameters.ReadWriteConfig.Buffer,
                                  IrpSp->Parameters.ReadWriteConfig.Offset,
                                  IrpSp->Parameters.ReadWriteConfig.Length);

    if (Size != IrpSp->Parameters.ReadWriteConfig.Length)
    {
        DPRINT1("Size %lu  Length %lu\n", Size, IrpSp->Parameters.ReadWriteConfig.Length);
        Irp->IoStatus.Information = 0;
        return STATUS_UNSUCCESSFUL;
    }

    Irp->IoStatus.Information = Size;

    return STATUS_SUCCESS;
}


static NTSTATUS
PdoWriteConfig(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    ULONG Size;

    DPRINT1("PdoWriteConfig() called\n");

    /* Get PCI configuration space */
    Size = InterfaceBusSetBusData(DeviceObject,
                                  IrpSp->Parameters.ReadWriteConfig.WhichSpace,
                                  IrpSp->Parameters.ReadWriteConfig.Buffer,
                                  IrpSp->Parameters.ReadWriteConfig.Offset,
                                  IrpSp->Parameters.ReadWriteConfig.Length);

    if (Size != IrpSp->Parameters.ReadWriteConfig.Length)
    {
        DPRINT1("Size %lu  Length %lu\n", Size, IrpSp->Parameters.ReadWriteConfig.Length);
        Irp->IoStatus.Information = 0;
        return STATUS_UNSUCCESSFUL;
    }

    Irp->IoStatus.Information = Size;

    return STATUS_SUCCESS;
}

static NTSTATUS
PdoQueryDeviceRelations(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP Irp,
    PIO_STACK_LOCATION IrpSp)
{
    PDEVICE_RELATIONS DeviceRelations;

    /* We only support TargetDeviceRelation for child PDOs */
    if (IrpSp->Parameters.QueryDeviceRelations.Type != TargetDeviceRelation)
        return Irp->IoStatus.Status;

    /* We can do this because we only return 1 PDO for TargetDeviceRelation */
    DeviceRelations = ExAllocatePoolWithTag(PagedPool, sizeof(*DeviceRelations), TAG_PCI);
    if (!DeviceRelations)
        return STATUS_INSUFFICIENT_RESOURCES;

    DeviceRelations->Count = 1;
    DeviceRelations->Objects[0] = DeviceObject;

    /* The PnP manager will remove this when it is done with the PDO */
    ObReferenceObject(DeviceObject);

    Irp->IoStatus.Information = (ULONG_PTR)DeviceRelations;

    return STATUS_SUCCESS;
}


/*** PUBLIC ******************************************************************/

NTSTATUS
PdoPnpControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
/*
 * FUNCTION: Handle Plug and Play IRPs for the child device
 * ARGUMENTS:
 *     DeviceObject = Pointer to physical device object of the child device
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status;

    DPRINT("Called\n");

    Status = Irp->IoStatus.Status;

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_DEVICE_USAGE_NOTIFICATION:
            DPRINT("Unimplemented IRP_MN_DEVICE_USAGE_NOTIFICATION received\n");
            break;

        case IRP_MN_EJECT:
            DPRINT("Unimplemented IRP_MN_EJECT received\n");
            break;

        case IRP_MN_QUERY_BUS_INFORMATION:
            Status = PdoQueryBusInformation(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_CAPABILITIES:
            Status = PdoQueryCapabilities(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_DEVICE_RELATIONS:
            Status = PdoQueryDeviceRelations(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_DEVICE_TEXT:
            DPRINT("IRP_MN_QUERY_DEVICE_TEXT received\n");
            Status = PdoQueryDeviceText(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_ID:
            DPRINT("IRP_MN_QUERY_ID received\n");
            Status = PdoQueryId(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_PNP_DEVICE_STATE:
            DPRINT("Unimplemented IRP_MN_QUERY_ID received\n");
            break;

        case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
            DPRINT("IRP_MN_QUERY_RESOURCE_REQUIREMENTS received\n");
            Status = PdoQueryResourceRequirements(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_RESOURCES:
            DPRINT("IRP_MN_QUERY_RESOURCES received\n");
            Status = PdoQueryResources(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_SET_LOCK:
            DPRINT("Unimplemented IRP_MN_SET_LOCK received\n");
            break;

        case IRP_MN_START_DEVICE:
            Status = PdoStartDevice(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_QUERY_STOP_DEVICE:
        case IRP_MN_CANCEL_STOP_DEVICE:
        case IRP_MN_STOP_DEVICE:
        case IRP_MN_QUERY_REMOVE_DEVICE:
        case IRP_MN_CANCEL_REMOVE_DEVICE:
        case IRP_MN_REMOVE_DEVICE:
        case IRP_MN_SURPRISE_REMOVAL:
            Status = STATUS_SUCCESS;
            break;

        case IRP_MN_QUERY_INTERFACE:
            DPRINT("IRP_MN_QUERY_INTERFACE received\n");
            Status = PdoQueryInterface(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_READ_CONFIG:
            DPRINT("IRP_MN_READ_CONFIG received\n");
            Status = PdoReadConfig(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_WRITE_CONFIG:
            DPRINT("IRP_MN_WRITE_CONFIG received\n");
            Status = PdoWriteConfig(DeviceObject, Irp, IrpSp);
            break;

        case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
            DPRINT("IRP_MN_FILTER_RESOURCE_REQUIREMENTS received\n");
            /* Nothing to do */
            Irp->IoStatus.Status = Status;
            break;

        default:
            DPRINT1("Unknown IOCTL 0x%lx\n", IrpSp->MinorFunction);
            break;
    }

    if (Status != STATUS_PENDING)
    {
        Irp->IoStatus.Status = Status;
        IoCompleteRequest(Irp, IO_NO_INCREMENT);
    }

    DPRINT("Leaving. Status 0x%X\n", Status);

    return Status;
}

NTSTATUS
PdoPowerControl(
    PDEVICE_OBJECT DeviceObject,
    PIRP Irp)
/*
 * FUNCTION: Handle power management IRPs for the child device
 * ARGUMENTS:
 *     DeviceObject = Pointer to physical device object of the child device
 *     Irp          = Pointer to IRP that should be handled
 * RETURNS:
 *     Status
 */
{
    PIO_STACK_LOCATION IrpSp;
    NTSTATUS Status = Irp->IoStatus.Status;

    DPRINT("Called\n");

    IrpSp = IoGetCurrentIrpStackLocation(Irp);

    switch (IrpSp->MinorFunction)
    {
        case IRP_MN_QUERY_POWER:
        case IRP_MN_SET_POWER:
            Status = STATUS_SUCCESS;
            break;
    }

    PoStartNextPowerIrp(Irp);
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);

    DPRINT("Leaving. Status 0x%X\n", Status);

    return Status;
}

/* EOF */
