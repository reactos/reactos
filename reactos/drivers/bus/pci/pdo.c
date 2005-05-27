/* $Id$
 *
 * PROJECT:         ReactOS PCI bus driver
 * FILE:            pdo.c
 * PURPOSE:         Child device object dispatch routines
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      10-09-2001  CSH  Created
 */

#include <ddk/ntddk.h>

#include "pcidef.h"
#include "pci.h"

#define NDEBUG
#include <debug.h>

DEFINE_GUID(GUID_BUS_TYPE_PCI, 0xc8ebdfb0L, 0xb510, 0x11d0, 0x80, 0xe5, 0x00, 0xa0, 0xc9, 0x25, 0x42, 0xe3);

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
        &String,
        &DeviceExtension->DeviceID,
        PagedPool);

      DPRINT("DeviceID: %S\n", String.Buffer);

      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryHardwareIDs:
      Status = PciDuplicateUnicodeString(
        &String,
        &DeviceExtension->HardwareIDs,
        PagedPool);

      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryCompatibleIDs:
      Status = PciDuplicateUnicodeString(
        &String,
        &DeviceExtension->CompatibleIDs,
        PagedPool);

      Irp->IoStatus.Information = (ULONG_PTR)String.Buffer;
      break;

    case BusQueryInstanceID:
      Status = PciDuplicateUnicodeString(
        &String,
        &DeviceExtension->InstanceID,
        PagedPool);

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
    BusInformation->BusNumber = DeviceExtension->BusNumber;

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

  DPRINT("Called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;
  DeviceCapabilities = IrpSp->Parameters.DeviceCapabilities.Capabilities;

  if (DeviceCapabilities->Version != 1)
    return STATUS_UNSUCCESSFUL;

  DeviceCapabilities->UniqueID = FALSE;
  DeviceCapabilities->Address = DeviceExtension->SlotNumber.u.AsULONG;
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
                              DeviceExtension->BusNumber,
                              DeviceExtension->SlotNumber.u.AsULONG,
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
                              DeviceExtension->BusNumber,
                              DeviceExtension->SlotNumber.u.AsULONG,
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
                              DeviceExtension->BusNumber,
                              DeviceExtension->SlotNumber.u.AsULONG,
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
                              DeviceExtension->BusNumber,
                              DeviceExtension->SlotNumber.u.AsULONG,
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
                      DeviceExtension->BusNumber,
                      DeviceExtension->SlotNumber.u.AsULONG,
                      &PciConfig,
                      sizeof(PCI_COMMON_CONFIG));
  DPRINT("Size %lu\n", Size);
  if (Size < sizeof(PCI_COMMON_CONFIG))
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
  }
  else if (PCI_CONFIGURATION_TYPE(&PciConfig) == PCI_CARDBUS_BRIDGE_TYPE)
  {
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
  ListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST);
  if (ResCount > 1)
  {
    ListSize += ((ResCount - 1) * sizeof(IO_RESOURCE_DESCRIPTOR));
  }

  DPRINT("ListSize %lu (0x%lx)\n", ListSize, ListSize);

  /* Allocate the resource requirements list */
  ResourceList = ExAllocatePool(PagedPool,
                                ListSize);
  if (ResourceList == NULL)
  {
    Irp->IoStatus.Information = 0;
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  ResourceList->ListSize = ListSize;
  ResourceList->InterfaceType = PCIBus;
  ResourceList->BusNumber = DeviceExtension->BusNumber,
  ResourceList->SlotNumber = DeviceExtension->SlotNumber.u.AsULONG,
  ResourceList->AlternativeLists = 1;

  ResourceList->List[0].Version = 1;
  ResourceList->List[0].Revision = 1;
  ResourceList->List[0].Count = ResCount;

  Descriptor = &ResourceList->List[0].Descriptors[0];
  if (PCI_CONFIGURATION_TYPE(&PciConfig) == 0)
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
  else if (PCI_CONFIGURATION_TYPE(&PciConfig) == 1)
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
  }
  else if (PCI_CONFIGURATION_TYPE(&PciConfig) == 2)
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
                      DeviceExtension->BusNumber,
                      DeviceExtension->SlotNumber.u.AsULONG,
                      &PciConfig,
                      sizeof(PCI_COMMON_CONFIG));
  DPRINT("Size %lu\n", Size);
  if (Size < sizeof(PCI_COMMON_CONFIG))
  {
    Irp->IoStatus.Information = 0;
    return STATUS_UNSUCCESSFUL;
  }

  DPRINT("Command register: 0x%04hx\n", PciConfig.Command);

  /* Count required resource descriptors */
  ResCount = 0;
  if (PCI_CONFIGURATION_TYPE(&PciConfig) == 0)
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
  else if (PCI_CONFIGURATION_TYPE(&PciConfig) == 1)
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
  }
  else if (PCI_CONFIGURATION_TYPE(&PciConfig) == 2)
  {

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
  ListSize = sizeof(CM_RESOURCE_LIST);
  if (ResCount > 1)
  {
    ListSize += ((ResCount - 1) * sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR));
  }

  /* Allocate the resource list */
  ResourceList = ExAllocatePool(PagedPool,
                                ListSize);
  if (ResourceList == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

  ResourceList->Count = 1;
  ResourceList->List[0].InterfaceType = PCIConfiguration;
  ResourceList->List[0].BusNumber = DeviceExtension->BusNumber;

  PartialList = &ResourceList->List[0].PartialResourceList;
  PartialList->Version = 0;
  PartialList->Revision = 0;
  PartialList->Count = ResCount;

  Descriptor = &PartialList->PartialDescriptors[0];
  if (PCI_CONFIGURATION_TYPE(&PciConfig) == 0)
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
      Descriptor->u.Interrupt.Vector = PciConfig.u.type0.InterruptLine;
      Descriptor->u.Interrupt.Affinity = 0xFFFFFFFF;
    }
  }
  else if (PCI_CONFIGURATION_TYPE(&PciConfig) == 1)
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
  }
  else if (PCI_CONFIGURATION_TYPE(&PciConfig) == 2)
  {
    /* FIXME: Cardbus */
  }

  Irp->IoStatus.Information = (ULONG_PTR)ResourceList;

  return STATUS_SUCCESS;
}


static NTSTATUS
PdoReadConfig(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION DeviceExtension;
  ULONG Size;

  DPRINT1("PdoReadConfig() called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

#if 0
  if (IrpSp->Parameters.ReadWriteConfig.WhichSpace != PCI_WHICHSPACE_CONFIG)
    return STATUS_NOT_SUPPORTED;
#endif

  /* Get PCI configuration space */
  Size= HalGetBusDataByOffset(PCIConfiguration,
                              DeviceExtension->BusNumber,
                              DeviceExtension->SlotNumber.u.AsULONG,
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
  PPDO_DEVICE_EXTENSION DeviceExtension;
  ULONG Size;

  DPRINT1("PdoWriteConfig() called\n");

  DeviceExtension = (PPDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

#if 0
  if (IrpSp->Parameters.ReadWriteConfig.WhichSpace != PCI_WHICHSPACE_CONFIG)
    return STATUS_NOT_SUPPORTED;
#endif

  /* Get PCI configuration space */
  Size= HalSetBusDataByOffset(PCIConfiguration,
                              DeviceExtension->BusNumber,
                              DeviceExtension->SlotNumber.u.AsULONG,
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

  case IRP_MN_READ_CONFIG:
    DPRINT1("IRP_MN_READ_CONFIG received\n");
    Status = PdoReadConfig(DeviceObject, Irp, IrpSp);
    break;

  case IRP_MN_WRITE_CONFIG:
    DPRINT1("IRP_MN_WRITE_CONFIG received\n");
    Status = PdoWriteConfig(DeviceObject, Irp, IrpSp);
    break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
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
