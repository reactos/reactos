/*
 * PROJECT:         ReactOS PCI bus driver
 * FILE:            pdo.c
 * PURPOSE:         Child device object dispatch routines
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      10-09-2001  CSH  Created
 */

#include "pci.h"

#ifndef NDEBUG
#define NDEBUG
#endif
#include <debug.h>

/*** PRIVATE *****************************************************************/

static NTSTATUS
PdoQueryDeviceText(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  Status = STATUS_SUCCESS;

  switch (IrpSp->Parameters.QueryDeviceText.DeviceTextType)
  {
    case DeviceTextDescription:
      DPRINT("DeviceTextDescription\n");
      Irp->IoStatus.Information = (ULONG_PTR)DeviceExtension->DeviceDescription.Buffer;
      break;

    case DeviceTextLocationInformation:
      DPRINT("DeviceTextLocationInformation\n");
      Irp->IoStatus.Information = (ULONG_PTR)DeviceExtension->DeviceLocation.Buffer;
      break;

    default:
      Irp->IoStatus.Information = 0;
      Status = STATUS_INVALID_PARAMETER;
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

//  Irp->IoStatus.Information = 0;

  Status = STATUS_SUCCESS;

  RtlInitUnicodeString(&String, NULL);

  switch (IrpSp->Parameters.QueryId.IdType) {
    case BusQueryDeviceID:
      Status = PciDuplicateUnicodeString(
        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
        &DeviceExtension->DeviceID,
        &String);

      DPRINT("DeviceID: %S\n", String.Buffer);

      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryHardwareIDs:
      Status = PciDuplicateUnicodeString(
        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
        &DeviceExtension->HardwareIDs,
        &String);

      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryCompatibleIDs:
      Status = PciDuplicateUnicodeString(
        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
        &DeviceExtension->CompatibleIDs,
        &String);

      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryInstanceID:
      Status = PciDuplicateUnicodeString(
        RTL_DUPLICATE_UNICODE_STRING_NULL_TERMINATE,
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
  PFDO_DEVICE_EXTENSION FdoDeviceExtension;
  PPNP_BUS_INFORMATION BusInformation;

  DPRINT("Called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  FdoDeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceExtension->Fdo->DeviceExtension;
  BusInformation = ExAllocatePool(PagedPool, sizeof(PNP_BUS_INFORMATION));
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

  DPRINT("Called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

  if (DeviceCapabilities->Version != 1)
    return STATUS_UNSUCCESSFUL;

  DeviceNumber = DeviceExtension->PciDevice->SlotNumber.u.bits.DeviceNumber;
  FunctionNumber = DeviceExtension->PciDevice->SlotNumber.u.bits.FunctionNumber;

  DeviceCapabilities->UniqueID = FALSE;
  DeviceCapabilities->Address = ((DeviceNumber << 16) & 0xFFFF0000) + (FunctionNumber & 0xFFFF);
  DeviceCapabilities->UINumber = (ULONG)-1; /* FIXME */

  return STATUS_SUCCESS;
}


static BOOLEAN
PdoGetRangeLength(PPDO_DEVICE_EXTENSION DeviceExtension,
                  ULONG Offset,
                  PULONG Base,
                  PULONG Length,
                  PULONG Flags)
{
  ULONG OrigValue;
  ULONG BaseValue;
  ULONG NewValue;
  ULONG Size;
  ULONG XLength;

  /* Save original value */
  Size= HalGetBusDataByOffset(PCIConfiguration,
                              DeviceExtension->PciDevice->BusNumber,
                              DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                              &OrigValue,
                              Offset,
                              sizeof(ULONG));
  if (Size != sizeof(ULONG))
  {
    DPRINT1("Wrong size %lu\n", Size);
    return FALSE;
  }

  BaseValue = (OrigValue & 0x00000001) ? (OrigValue & ~0x3) : (OrigValue & ~0xF);

  *Base = BaseValue;

  /* Set magic value */
  NewValue = (ULONG)-1;
  Size= HalSetBusDataByOffset(PCIConfiguration,
                              DeviceExtension->PciDevice->BusNumber,
                              DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                              &NewValue,
                              Offset,
                              sizeof(ULONG));
  if (Size != sizeof(ULONG))
  {
    DPRINT1("Wrong size %lu\n", Size);
    return FALSE;
  }

  /* Get the range length */
  Size= HalGetBusDataByOffset(PCIConfiguration,
                              DeviceExtension->PciDevice->BusNumber,
                              DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                              &NewValue,
                              Offset,
                              sizeof(ULONG));
  if (Size != sizeof(ULONG))
  {
    DPRINT1("Wrong size %lu\n", Size);
    return FALSE;
  }

  /* Restore original value */
  Size= HalSetBusDataByOffset(PCIConfiguration,
                              DeviceExtension->PciDevice->BusNumber,
                              DeviceExtension->PciDevice->SlotNumber.u.AsULONG,
                              &OrigValue,
                              Offset,
                              sizeof(ULONG));
  if (Size != sizeof(ULONG))
  {
    DPRINT1("Wrong size %lu\n", Size);
    return FALSE;
  }

  if (NewValue == 0)
  {
    DPRINT("Unused address register\n");
    *Base = 0;
    *Length = 0;
    *Flags = 0;
     return TRUE;
  }

  XLength = ~((NewValue & 0x00000001) ? (NewValue & ~0x3) : (NewValue & ~0xF)) + 1;

#if 0
  DbgPrint("BaseAddress 0x%08lx  Length 0x%08lx",
           BaseValue, XLength);

  if (NewValue & 0x00000001)
  {
    DbgPrint("  IO range");
  }
  else
  {
    DbgPrint("  Memory range");
    if ((NewValue & 0x00000006) == 0)
    {
      DbgPrint(" in 32-Bit address space");
    }
    else if ((NewValue & 0x00000006) == 2)
    {
      DbgPrint(" below 1BM ");
    }
    else if ((NewValue & 0x00000006) == 4)
    {
      DbgPrint(" in 64-Bit address space");
    }

    if (NewValue & 0x00000008)
    {
      DbgPrint(" prefetchable");
    }
  }

  DbgPrint("\n");
#endif

  *Length = XLength;
  *Flags = (NewValue & 0x00000001) ? (NewValue & 0x3) : (NewValue & 0xF);

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
  ULONG i;
  ULONG Base;
  ULONG Length;
  ULONG Flags;

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
    for (i = 0; i < PCI_TYPE0_ADDRESSES; i++)
    {
      if (!PdoGetRangeLength(DeviceExtension,
			     0x10 + i * 4,
			     &Base,
			     &Length,
			     &Flags))
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
    for (i = 0; i < PCI_TYPE1_ADDRESSES; i++)
    {
      if (!PdoGetRangeLength(DeviceExtension,
			     0x10 + i * 4,
			     &Base,
			     &Length,
			     &Flags))
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
    DPRINT1("Unsupported header type %u\n", PCI_CONFIGURATION_TYPE(&PciConfig));
  }

  if (ResCount == 0)
  {
    Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;
  }

  /* Calculate the resource list size */
  ListSize = FIELD_OFFSET(IO_RESOURCE_REQUIREMENTS_LIST, List[0].Descriptors)
    + ResCount * sizeof(IO_RESOURCE_DESCRIPTOR);

  DPRINT("ListSize %lu (0x%lx)\n", ListSize, ListSize);

  /* Allocate the resource requirements list */
  ResourceList = ExAllocatePoolWithTag(PagedPool,
                                ListSize, TAG_PCI);
  if (ResourceList == NULL)
  {
    Irp->IoStatus.Information = 0;
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  RtlZeroMemory(ResourceList, ListSize);
  ResourceList->ListSize = ListSize;
  ResourceList->InterfaceType = PCIBus;
  ResourceList->BusNumber = 0;
  ResourceList->SlotNumber = 0;
  ResourceList->AlternativeLists = 1;

  ResourceList->List[0].Version = 1;
  ResourceList->List[0].Revision = 1;
  ResourceList->List[0].Count = ResCount;

  Descriptor = &ResourceList->List[0].Descriptors[0];
  if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_DEVICE_TYPE)
  {
    for (i = 0; i < PCI_TYPE0_ADDRESSES; i++)
    {
      if (!PdoGetRangeLength(DeviceExtension,
			     0x10 + i * 4,
			     &Base,
			     &Length,
			     &Flags))
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
        Descriptor->u.Port.MinimumAddress.QuadPart = (ULONGLONG)Base;
        Descriptor->u.Port.MaximumAddress.QuadPart = (ULONGLONG)(Base + Length - 1);
      }
      else
      {
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;

        Descriptor->u.Memory.Length = Length;
        Descriptor->u.Memory.Alignment = 1;
        Descriptor->u.Memory.MinimumAddress.QuadPart = (ULONGLONG)Base;
        Descriptor->u.Memory.MaximumAddress.QuadPart = (ULONGLONG)(Base + Length - 1);
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
        Descriptor->u.Port.MinimumAddress.QuadPart = (ULONGLONG)0;
        Descriptor->u.Port.MaximumAddress.QuadPart = (ULONGLONG)0x00000000FFFFFFFF;
      }
      else
      {
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;

        Descriptor->u.Memory.Length = Length;
        Descriptor->u.Memory.Alignment = Length;
        Descriptor->u.Port.MinimumAddress.QuadPart = (ULONGLONG)0;
        Descriptor->u.Port.MaximumAddress.QuadPart = (ULONGLONG)0x00000000FFFFFFFF;
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
    for (i = 0; i < PCI_TYPE1_ADDRESSES; i++)
    {
      if (!PdoGetRangeLength(DeviceExtension,
			     0x10 + i * 4,
			     &Base,
			     &Length,
			     &Flags))
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
        Descriptor->u.Port.MinimumAddress.QuadPart = (ULONGLONG)Base;
        Descriptor->u.Port.MaximumAddress.QuadPart = (ULONGLONG)(Base + Length - 1);
      }
      else
      {
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;

        Descriptor->u.Memory.Length = Length;
        Descriptor->u.Memory.Alignment = 1;
        Descriptor->u.Memory.MinimumAddress.QuadPart = (ULONGLONG)Base;
        Descriptor->u.Memory.MaximumAddress.QuadPart = (ULONGLONG)(Base + Length - 1);
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
        Descriptor->u.Port.MinimumAddress.QuadPart = (ULONGLONG)0;
        Descriptor->u.Port.MaximumAddress.QuadPart = (ULONGLONG)0x00000000FFFFFFFF;
      }
      else
      {
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;

        Descriptor->u.Memory.Length = Length;
        Descriptor->u.Memory.Alignment = Length;
        Descriptor->u.Port.MinimumAddress.QuadPart = (ULONGLONG)0;
        Descriptor->u.Port.MaximumAddress.QuadPart = (ULONGLONG)0x00000000FFFFFFFF;
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
  ULONG i;
  ULONG Base;
  ULONG Length;
  ULONG Flags;

  DPRINT("PdoQueryResources() called\n");

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
    for (i = 0; i < PCI_TYPE0_ADDRESSES; i++)
    {
      if (!PdoGetRangeLength(DeviceExtension,
			     0x10 + i * 4,
			     &Base,
			     &Length,
			     &Flags))
        break;

      if (Length)
        ResCount++;
    }

    if ((PciConfig.u.type0.InterruptPin != 0) &&
        (PciConfig.u.type0.InterruptLine != 0xFF))
      ResCount++;
  }
  else if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_BRIDGE_TYPE)
  {
    for (i = 0; i < PCI_TYPE1_ADDRESSES; i++)
    {
      if (!PdoGetRangeLength(DeviceExtension,
			     0x10 + i * 4,
			     &Base,
			     &Length,
			     &Flags))
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
    DPRINT1("Unsupported header type %u\n", PCI_CONFIGURATION_TYPE(&PciConfig));
  }

  if (ResCount == 0)
  {
    Irp->IoStatus.Information = 0;
    return STATUS_SUCCESS;
  }

  /* Calculate the resource list size */
  ListSize = FIELD_OFFSET(CM_RESOURCE_LIST, List[0].PartialResourceList.PartialDescriptors)
    + ResCount * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

  /* Allocate the resource list */
  ResourceList = ExAllocatePoolWithTag(PagedPool,
                                ListSize, TAG_PCI);
  if (ResourceList == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  RtlZeroMemory(ResourceList, ListSize);
  ResourceList->Count = 1;
  ResourceList->List[0].InterfaceType = PCIBus;
  ResourceList->List[0].BusNumber = 0;

  PartialList = &ResourceList->List[0].PartialResourceList;
  PartialList->Version = 1;
  PartialList->Revision = 1;
  PartialList->Count = ResCount;

  Descriptor = &PartialList->PartialDescriptors[0];
  if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_DEVICE_TYPE)
  {
    for (i = 0; i < PCI_TYPE0_ADDRESSES; i++)
    {
      if (!PdoGetRangeLength(DeviceExtension,
			     0x10 + i * 4,
			     &Base,
			     &Length,
			     &Flags))
      {
        DPRINT1("PdoGetRangeLength() failed\n");
        break;
      }

      if (Length == 0)
      {
        DPRINT("Unused address register\n");
        continue;
      }

      if (Flags & PCI_ADDRESS_IO_SPACE)
      {
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_IO;
        Descriptor->u.Port.Start.QuadPart =
          (ULONGLONG)Base;
        Descriptor->u.Port.Length = Length;
      }
      else
      {
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
        Descriptor->u.Memory.Start.QuadPart =
          (ULONGLONG)Base;
        Descriptor->u.Memory.Length = Length;
      }

      Descriptor++;
    }

    /* Add interrupt resource */
    if ((PciConfig.u.type0.InterruptPin != 0) &&
        (PciConfig.u.type0.InterruptLine != 0xFF))
    {
      Descriptor->Type = CmResourceTypeInterrupt;
      Descriptor->ShareDisposition = CmResourceShareShared;
      Descriptor->Flags = CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE;
      Descriptor->u.Interrupt.Level = PciConfig.u.type0.InterruptLine;
      Descriptor->u.Interrupt.Vector = 0;
      Descriptor->u.Interrupt.Affinity = 0xFFFFFFFF;
    }
  }
  else if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_BRIDGE_TYPE)
  {
    for (i = 0; i < PCI_TYPE1_ADDRESSES; i++)
    {
      if (!PdoGetRangeLength(DeviceExtension,
			     0x10 + i * 4,
			     &Base,
			     &Length,
			     &Flags))
      {
        DPRINT1("PdoGetRangeLength() failed\n");
        break;
      }

      if (Length == 0)
      {
        DPRINT("Unused address register\n");
        continue;
      }

      if (Flags & PCI_ADDRESS_IO_SPACE)
      {
        Descriptor->Type = CmResourceTypePort;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_PORT_IO;
        Descriptor->u.Port.Start.QuadPart =
          (ULONGLONG)Base;
        Descriptor->u.Port.Length = Length;
      }
      else
      {
        Descriptor->Type = CmResourceTypeMemory;
        Descriptor->ShareDisposition = CmResourceShareDeviceExclusive;
        Descriptor->Flags = CM_RESOURCE_MEMORY_READ_WRITE;
        Descriptor->u.Memory.Start.QuadPart =
          (ULONGLONG)Base;
        Descriptor->u.Memory.Length = Length;
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


static BOOLEAN NTAPI
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

  return HalTranslateBusAddress(
    PCIBus, DeviceExtension->PciDevice->BusNumber,
    BusAddress, AddressSpace, TranslatedAddress);
}


static PDMA_ADAPTER NTAPI
InterfaceBusGetDmaAdapter(
  IN PVOID Context,
  IN PDEVICE_DESCRIPTION DeviceDescription,
  OUT PULONG NumberOfMapRegisters)
{
  DPRINT("InterfaceBusGetDmaAdapter(%p %p %p)\n",
    Context, DeviceDescription, NumberOfMapRegisters);
  return (PDMA_ADAPTER)HalGetAdapter(DeviceDescription, NumberOfMapRegisters);
}


static ULONG NTAPI
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


static ULONG NTAPI
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
        if (!(Flags & PCI_USE_SUBSYSTEM_IDS) || (
          PciDevice->PciConfig.u.type0.SubVendorID == SubVendorID &&
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
  if ((Parameters->Flags & PCI_USE_VENDEV_IDS) && (
    PciConfig->VendorID != Parameters->VendorID ||
    PciConfig->DeviceID != Parameters->DeviceID))
  {
    return FALSE;
  }
  if ((Parameters->Flags & PCI_USE_CLASS_SUBCLASS) && (
    PciConfig->BaseClass != Parameters->BaseClass ||
    PciConfig->SubClass != Parameters->SubClass))
  {
    return FALSE;
  }
  if ((Parameters->Flags & PCI_USE_PROGIF) &&
    PciConfig->ProgIf != Parameters->ProgIf)
  {
    return FALSE;
  }
  if ((Parameters->Flags & PCI_USE_SUBSYSTEM_IDS) && (
    PciConfig->u.type0.SubVendorID != Parameters->SubVendorID ||
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
PdoReadConfig(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  ULONG Size;

  DPRINT("PdoReadConfig() called\n");

  Size = InterfaceBusGetBusData(
    DeviceObject,
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
  Size = InterfaceBusSetBusData(
    DeviceObject,
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
PdoSetPower(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION DeviceExtension;
  NTSTATUS Status;

  DPRINT("Called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (IrpSp->Parameters.Power.Type == DevicePowerState) {
    Status = STATUS_SUCCESS;
    switch (IrpSp->Parameters.Power.State.SystemState) {
    default:
      Status = STATUS_UNSUCCESSFUL;
    }
  } else {
    Status = STATUS_UNSUCCESSFUL;
  }

  return Status;
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

  switch (IrpSp->MinorFunction) {
#if 0
  case IRP_MN_DEVICE_USAGE_NOTIFICATION:
    break;

  case IRP_MN_EJECT:
    break;
#endif

  case IRP_MN_QUERY_BUS_INFORMATION:
    Status = PdoQueryBusInformation(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_QUERY_CAPABILITIES:
    Status = PdoQueryCapabilities(DeviceObject, Irp, IrpSp);
    break;

#if 0
  case IRP_MN_QUERY_DEVICE_RELATIONS:
    /* FIXME: Possibly handle for RemovalRelations */
    break;
#endif

  case IRP_MN_QUERY_DEVICE_TEXT:
    DPRINT("IRP_MN_QUERY_DEVICE_TEXT received\n");
    Status = PdoQueryDeviceText(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_QUERY_ID:
    DPRINT("IRP_MN_QUERY_ID received\n");
    Status = PdoQueryId(DeviceObject, Irp, IrpSp);
    break;

#if 0
  case IRP_MN_QUERY_PNP_DEVICE_STATE:
    break;
#endif

  case IRP_MN_QUERY_RESOURCE_REQUIREMENTS:
    DPRINT("IRP_MN_QUERY_RESOURCE_REQUIREMENTS received\n");
    Status = PdoQueryResourceRequirements(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_QUERY_RESOURCES:
    DPRINT("IRP_MN_QUERY_RESOURCES received\n");
    Status = PdoQueryResources(DeviceObject, Irp, IrpSp);
    break;

#if 0
  case IRP_MN_SET_LOCK:
    break;
#endif

  case IRP_MN_START_DEVICE:
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

  if (Status != STATUS_PENDING) {
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
  NTSTATUS Status;

  DPRINT("Called\n");

  IrpSp = IoGetCurrentIrpStackLocation(Irp);

  switch (IrpSp->MinorFunction) {
  case IRP_MN_SET_POWER:
    Status = PdoSetPower(DeviceObject, Irp, IrpSp);
    break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = STATUS_NOT_IMPLEMENTED;
    break;
  }

  if (Status != STATUS_PENDING) {
    Irp->IoStatus.Status = Status;
    IoCompleteRequest(Irp, IO_NO_INCREMENT);
  }

  DPRINT("Leaving. Status 0x%X\n", Status);

  return Status;
}

/* EOF */
