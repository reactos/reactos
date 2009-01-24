/* $Id$
 *
 * PROJECT:         ReactOS ACPI bus driver
 * FILE:            acpi/ospm/fdo.c
 * PURPOSE:         ACPI device object dispatch routines
 * PROGRAMMERS:     Casper S. Hornstrup (chorns@users.sourceforge.net)
 *                  Hervé Poussineau (hpoussin@reactos.com)
 * UPDATE HISTORY:
 *      08-08-2001  CSH  Created
 */
#include <acpi.h>

#define NDEBUG
#include <debug.h>

FADT_DESCRIPTOR_REV2 acpi_fadt;

/*** PRIVATE *****************************************************************/


BOOLEAN
AcpiCreateUnicodeString(
  PUNICODE_STRING Destination,
  PWSTR Source,
  POOL_TYPE PoolType)
{
  ULONG Length;

  if (!Source)
  {
    RtlInitUnicodeString(Destination, NULL);
    return TRUE;
  }

  Length = (wcslen(Source) + 1) * sizeof(WCHAR);

  Destination->Buffer = ExAllocatePool(PoolType, Length);
  if (Destination->Buffer == NULL)
  {
    return FALSE;
  }

  RtlCopyMemory(Destination->Buffer, Source, Length);

  Destination->MaximumLength = Length;

  Destination->Length = Length - sizeof(WCHAR);

  return TRUE;
}

BOOLEAN
AcpiCreateDeviceIDString(PUNICODE_STRING DeviceID,
                         BM_NODE *Node)
{
  WCHAR Buffer[256];

  swprintf(Buffer,
           L"ACPI\\%S",
           Node->device.id.hid);

  return AcpiCreateUnicodeString(DeviceID, Buffer, PagedPool);
}


BOOLEAN
AcpiCreateHardwareIDsString(PUNICODE_STRING HardwareIDs,
                            BM_NODE *Node)
{
  WCHAR Buffer[256];
  ULONG Length;
  ULONG Index;

  Index = 0;
  Index += swprintf(&Buffer[Index],
                    L"ACPI\\%S",
                    Node->device.id.hid);
  Index++;

  Index += swprintf(&Buffer[Index],
                    L"*%S",
                    Node->device.id.hid);
  Index++;
  Buffer[Index] = UNICODE_NULL;

  Length = (Index + 1) * sizeof(WCHAR);
  HardwareIDs->Buffer = ExAllocatePool(PagedPool, Length);
  if (HardwareIDs->Buffer == NULL)
  {
    return FALSE;
  }

  HardwareIDs->Length = Length - sizeof(WCHAR);
  HardwareIDs->MaximumLength = Length;
  RtlCopyMemory(HardwareIDs->Buffer, Buffer, Length);

  return TRUE;
}


BOOLEAN
AcpiCreateInstanceIDString(PUNICODE_STRING InstanceID,
                           BM_NODE *Node)
{
  WCHAR Buffer[10];

  if (Node->device.id.uid[0])
    swprintf(Buffer, L"%S", Node->device.id.uid);
  else
    /* FIXME: Generate unique id! */
    swprintf(Buffer, L"%S", L"0000");

  return AcpiCreateUnicodeString(InstanceID, Buffer, PagedPool);
}


BOOLEAN
AcpiCreateDeviceDescriptionString(PUNICODE_STRING DeviceDescription,
                                  BM_NODE *Node)
{
  PWSTR Buffer;

  if (RtlCompareMemory(Node->device.id.hid, "PNP000", 6) == 6)
    Buffer = L"Programmable interrupt controller";
  else if (RtlCompareMemory(Node->device.id.hid, "PNP010", 6) == 6)
    Buffer = L"System timer";
  else if (RtlCompareMemory(Node->device.id.hid, "PNP020", 6) == 6)
    Buffer = L"DMA controller";
  else if (RtlCompareMemory(Node->device.id.hid, "PNP03", 5) == 5)
    Buffer = L"Keyboard";
  else if (RtlCompareMemory(Node->device.id.hid, "PNP040", 6) == 6)
    Buffer = L"Parallel port";
  else if (RtlCompareMemory(Node->device.id.hid, "PNP05", 5) == 5)
    Buffer = L"Serial port";
  else if (RtlCompareMemory(Node->device.id.hid, "PNP06", 5) ==5)
    Buffer = L"Disk controller";
  else if (RtlCompareMemory(Node->device.id.hid, "PNP07", 5) == 5)
    Buffer = L"Disk controller";
  else if (RtlCompareMemory(Node->device.id.hid, "PNP09", 5) == 5)
    Buffer = L"Display adapter";
  else if (RtlCompareMemory(Node->device.id.hid, "PNP0A0", 6) == 6)
    Buffer = L"Bus controller";
  else if (RtlCompareMemory(Node->device.id.hid, "PNP0E0", 6) == 6)
    Buffer = L"PCMCIA controller";
  else if (RtlCompareMemory(Node->device.id.hid, "PNP0F", 5) == 5)
    Buffer = L"Mouse device";
  else if (RtlCompareMemory(Node->device.id.hid, "PNP8", 4) == 4)
    Buffer = L"Network adapter";
  else if (RtlCompareMemory(Node->device.id.hid, "PNPA0", 5) == 5)
    Buffer = L"SCSI controller";
  else if (RtlCompareMemory(Node->device.id.hid, "PNPB0", 5) == 5)
    Buffer = L"Multimedia device";
  else if (RtlCompareMemory(Node->device.id.hid, "PNPC00", 6) == 6)
    Buffer = L"Modem";
  else
    Buffer = L"Other ACPI device";

  return AcpiCreateUnicodeString(DeviceDescription, Buffer, PagedPool);
}


static BOOLEAN
AcpiCreateResourceList(PCM_RESOURCE_LIST* pResourceList,
                       PULONG ResourceListSize,
                       PIO_RESOURCE_REQUIREMENTS_LIST* pRequirementsList,
                       PULONG RequirementsListSize,
                       RESOURCE* resources)
{
  BOOLEAN Done;
  ULONG NumberOfResources = 0;
  PCM_RESOURCE_LIST ResourceList;
  PIO_RESOURCE_REQUIREMENTS_LIST RequirementsList;
  PCM_PARTIAL_RESOURCE_DESCRIPTOR ResourceDescriptor;
  PIO_RESOURCE_DESCRIPTOR RequirementDescriptor;
  RESOURCE* resource;
  ULONG i;

  /* Count number of resources */
  Done = FALSE;
  resource = resources;
  while (!Done)
  {
    switch (resource->id)
    {
      case irq:
      {
        IRQ_RESOURCE *irq_data = (IRQ_RESOURCE*) &resource->data;
        NumberOfResources += irq_data->number_of_interrupts;
        break;
      }
      case dma:
      {
        DMA_RESOURCE *dma_data = (DMA_RESOURCE*) &resource->data;
        NumberOfResources += dma_data->number_of_channels;
        break;
      }
      case io:
      {
        NumberOfResources++;
        break;
      }
      case end_tag:
      {
        Done = TRUE;
        break;
      }
      default:
      {
        break;
      }
    }
    resource = NEXT_RESOURCE(resource);
  }

  /* Allocate memory */
  *ResourceListSize = sizeof(CM_RESOURCE_LIST) + sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR) * (NumberOfResources - 1);
  ResourceList = (PCM_RESOURCE_LIST)ExAllocatePool(PagedPool, *ResourceListSize);
  *pResourceList = ResourceList;
  if (!ResourceList)
    return FALSE;
  ResourceList->Count = 1;
  ResourceList->List[0].InterfaceType = Internal; /* FIXME */
  ResourceList->List[0].BusNumber = 0; /* We're the only ACPI bus device in the system */
  ResourceList->List[0].PartialResourceList.Version = 1;
  ResourceList->List[0].PartialResourceList.Revision = 1;
  ResourceList->List[0].PartialResourceList.Count = NumberOfResources;
  ResourceDescriptor = ResourceList->List[0].PartialResourceList.PartialDescriptors;

  *RequirementsListSize = sizeof(IO_RESOURCE_REQUIREMENTS_LIST) + sizeof(IO_RESOURCE_DESCRIPTOR) * (NumberOfResources - 1);
  RequirementsList = (PIO_RESOURCE_REQUIREMENTS_LIST)ExAllocatePool(PagedPool, *RequirementsListSize);
  *pRequirementsList = RequirementsList;
  if (!RequirementsList)
  {
    ExFreePool(ResourceList);
    return FALSE;
  }
  RequirementsList->ListSize = *RequirementsListSize;
  RequirementsList->InterfaceType = ResourceList->List[0].InterfaceType;
  RequirementsList->BusNumber = ResourceList->List[0].BusNumber;
  RequirementsList->SlotNumber = 0; /* Not used by WDM drivers */
  RequirementsList->AlternativeLists = 1;
  RequirementsList->List[0].Version = 1;
  RequirementsList->List[0].Revision = 1;
  RequirementsList->List[0].Count = NumberOfResources;
  RequirementDescriptor = RequirementsList->List[0].Descriptors;

  /* Fill resources list structure */
  Done = FALSE;
  resource = resources;
  while (!Done)
  {
    switch (resource->id)
    {
      case irq:
      {
        IRQ_RESOURCE *irq_data = (IRQ_RESOURCE*) &resource->data;
        for (i = 0; i < irq_data->number_of_interrupts; i++)
        {
          ResourceDescriptor->Type = CmResourceTypeInterrupt;

          ResourceDescriptor->ShareDisposition =
            (irq_data->shared_exclusive == SHARED ? CmResourceShareShared : CmResourceShareDeviceExclusive);
          ResourceDescriptor->Flags =
            (irq_data->edge_level == LEVEL_SENSITIVE ? CM_RESOURCE_INTERRUPT_LEVEL_SENSITIVE : CM_RESOURCE_INTERRUPT_LATCHED);
          ResourceDescriptor->u.Interrupt.Level = irq_data->interrupts[i];
          ResourceDescriptor->u.Interrupt.Vector = 0;
          ResourceDescriptor->u.Interrupt.Affinity = (KAFFINITY)(-1);

          RequirementDescriptor->Option = 0; /* Required */
          RequirementDescriptor->Type = ResourceDescriptor->Type;
          RequirementDescriptor->ShareDisposition = ResourceDescriptor->ShareDisposition;
          RequirementDescriptor->Flags = ResourceDescriptor->Flags;
          RequirementDescriptor->u.Interrupt.MinimumVector = RequirementDescriptor->u.Interrupt.MaximumVector
            = irq_data->interrupts[i];

          ResourceDescriptor++;
          RequirementDescriptor++;
        }
        break;
      }
      case dma:
      {
        DMA_RESOURCE *dma_data = (DMA_RESOURCE*) &resource->data;
        for (i = 0; i < dma_data->number_of_channels; i++)
        {
          ResourceDescriptor->Type = CmResourceTypeDma;
          ResourceDescriptor->Flags = 0;
          switch (dma_data->type)
          {
            case TYPE_A: ResourceDescriptor->Flags |= CM_RESOURCE_DMA_TYPE_A; break;
            case TYPE_B: ResourceDescriptor->Flags |= CM_RESOURCE_DMA_TYPE_B; break;
            case TYPE_F: ResourceDescriptor->Flags |= CM_RESOURCE_DMA_TYPE_F; break;
          }
          if (dma_data->bus_master == BUS_MASTER)
            ResourceDescriptor->Flags |= CM_RESOURCE_DMA_BUS_MASTER;
          switch (dma_data->transfer)
          {
            case TRANSFER_8: ResourceDescriptor->Flags |= CM_RESOURCE_DMA_8; break;
            case TRANSFER_16: ResourceDescriptor->Flags |= CM_RESOURCE_DMA_16; break;
            case TRANSFER_8_16: ResourceDescriptor->Flags |= CM_RESOURCE_DMA_8_AND_16; break;
          }
          ResourceDescriptor->u.Dma.Channel = dma_data->channels[i];

          RequirementDescriptor->Option = 0; /* Required */
          RequirementDescriptor->Type = ResourceDescriptor->Type;
          RequirementDescriptor->ShareDisposition = ResourceDescriptor->ShareDisposition;
          RequirementDescriptor->Flags = ResourceDescriptor->Flags;
          RequirementDescriptor->u.Dma.MinimumChannel = RequirementDescriptor->u.Dma.MaximumChannel
            = ResourceDescriptor->u.Dma.Channel;

          ResourceDescriptor++;
          RequirementDescriptor++;
        }
        break;
      }
      case io:
      {
        IO_RESOURCE *io_data = (IO_RESOURCE*) &resource->data;
        ResourceDescriptor->Type = CmResourceTypePort;
        ResourceDescriptor->ShareDisposition = CmResourceShareDriverExclusive;
        ResourceDescriptor->Flags = CM_RESOURCE_PORT_IO;
        if (io_data->io_decode == DECODE_16)
          ResourceDescriptor->Flags |= CM_RESOURCE_PORT_16_BIT_DECODE;
        else
          ResourceDescriptor->Flags |= CM_RESOURCE_PORT_10_BIT_DECODE;
        ResourceDescriptor->u.Port.Start.u.HighPart = 0;
        ResourceDescriptor->u.Port.Start.u.LowPart = io_data->min_base_address;
        ResourceDescriptor->u.Port.Length = io_data->range_length;

        RequirementDescriptor->Option = 0; /* Required */
        RequirementDescriptor->Type = ResourceDescriptor->Type;
        RequirementDescriptor->ShareDisposition = ResourceDescriptor->ShareDisposition;
        RequirementDescriptor->Flags = ResourceDescriptor->Flags;
        RequirementDescriptor->u.Port.Length = ResourceDescriptor->u.Port.Length;
        RequirementDescriptor->u.Port.Alignment = 1; /* Start address is specified, so it doesn't matter */
        RequirementDescriptor->u.Port.MinimumAddress = RequirementDescriptor->u.Port.MaximumAddress
          = ResourceDescriptor->u.Port.Start;

        ResourceDescriptor++;
        RequirementDescriptor++;
        break;
      }
      case end_tag:
      {
        Done = TRUE;
        break;
      }
      default:
      {
        break;
      }
    }
    resource = NEXT_RESOURCE(resource);
  }

  acpi_rs_dump_resource_list(resource);
  return TRUE;
}


static BOOLEAN
AcpiCheckIfIsSerialDebugPort(
  IN PACPI_DEVICE Device)
{
  ACPI_STATUS AcpiStatus;
  BM_NODE *Node;
  ACPI_BUFFER Buffer;
  BOOLEAN Done;
  RESOURCE* resource;

  AcpiStatus = bm_get_node(Device->BmHandle, 0, &Node);
  if (!ACPI_SUCCESS(AcpiStatus))
    return FALSE;

  /* Get current resources */
  Buffer.length = 0;
  AcpiStatus = acpi_get_current_resources(Node->device.acpi_handle, &Buffer);
  if ((AcpiStatus & ACPI_OK) == 0)
    return FALSE;
  if (Buffer.length == 0)
    return FALSE;

  Buffer.pointer = ExAllocatePool(PagedPool, Buffer.length);
  if (!Buffer.pointer)
    return FALSE;
  AcpiStatus = acpi_get_current_resources(Node->device.acpi_handle, &Buffer);
  if (!ACPI_SUCCESS(AcpiStatus))
  {
    ExFreePool(Buffer.pointer);
    return FALSE;
  }

  /* Loop through the list of resources to see if the
   * device is using the serial port address
   */
  Done = FALSE;
  resource = (RESOURCE*)Buffer.pointer;
  while (!Done)
  {
    switch (resource->id)
    {
      case io:
      {
        IO_RESOURCE *io_data = (IO_RESOURCE*) &resource->data;
        if (KdComPortInUse == (PUCHAR)io_data->min_base_address)
        {
          ExFreePool(Buffer.pointer);
          return TRUE;
        }
        break;
      }
      case end_tag:
      {
        Done = TRUE;
        break;
      }
      default:
      {
        break;
      }
    }
    resource = (RESOURCE *) ((NATIVE_UINT) resource + (NATIVE_UINT) resource->length);
  }

  ExFreePool(Buffer.pointer);
  return FALSE;
}

static NTSTATUS
FdoQueryBusRelations(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PPDO_DEVICE_EXTENSION PdoDeviceExtension;
  PFDO_DEVICE_EXTENSION DeviceExtension;
  PDEVICE_RELATIONS Relations;
  PLIST_ENTRY CurrentEntry;
  ACPI_STATUS AcpiStatus;
  PACPI_DEVICE Device;
  NTSTATUS Status = STATUS_SUCCESS;
  BM_NODE *Node;
  ULONG Size;
  ULONG i;

  DPRINT("Called\n");

  DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  Size = sizeof(DEVICE_RELATIONS) + sizeof(Relations->Objects) *
    (DeviceExtension->DeviceListCount - 1);
  Relations = (PDEVICE_RELATIONS)ExAllocatePool(PagedPool, Size);
  if (!Relations)
    return STATUS_INSUFFICIENT_RESOURCES;

  Relations->Count = DeviceExtension->DeviceListCount;

  i = 0;
  CurrentEntry = DeviceExtension->DeviceListHead.Flink;
  while (CurrentEntry != &DeviceExtension->DeviceListHead)
  {
    ACPI_BUFFER Buffer;
    Device = CONTAINING_RECORD(CurrentEntry, ACPI_DEVICE, DeviceListEntry);

    if (AcpiCheckIfIsSerialDebugPort(Device))
    {
      /* Skip this device */
      DPRINT("Found debug serial port ; skipping it\n");
      Relations->Count--;
      CurrentEntry = CurrentEntry->Flink;
      continue;
    }

    /* FIXME: For ACPI namespace devices on the motherboard create filter DOs
       and attach them just above the ACPI bus device object (PDO) */

    /* FIXME: For other devices in ACPI namespace, but not on motherboard,
       create PDOs */

    if (!Device->Pdo)
    {
      /* Create a physical device object for the
         device as it does not already have one */
      Status = IoCreateDevice(DeviceObject->DriverObject,
                              sizeof(PDO_DEVICE_EXTENSION),
                              NULL,
                              FILE_DEVICE_CONTROLLER,
                              FILE_AUTOGENERATED_DEVICE_NAME,
                              FALSE,
                              &Device->Pdo);
      if (!NT_SUCCESS(Status))
      {
        DPRINT("IoCreateDevice() failed with status 0x%X\n", Status);
        /* FIXME: Cleanup all new PDOs created in this call */
        ExFreePool(Relations);
        return Status;
      }

      PdoDeviceExtension = (PPDO_DEVICE_EXTENSION)Device->Pdo->DeviceExtension;

      RtlZeroMemory(PdoDeviceExtension, sizeof(PDO_DEVICE_EXTENSION));

      Device->Pdo->Flags |= DO_BUS_ENUMERATED_DEVICE;

      Device->Pdo->Flags &= ~DO_DEVICE_INITIALIZING;

      //Device->Pdo->Flags |= DO_POWER_PAGABLE;

      PdoDeviceExtension->Common.DeviceObject = Device->Pdo;

      PdoDeviceExtension->Common.DevicePowerState = PowerDeviceD0;

//      PdoDeviceExtension->Common.Ldo = IoAttachDeviceToDeviceStack(DeviceObject,
//                                                                   Device->Pdo);

      RtlInitUnicodeString(&PdoDeviceExtension->DeviceID, NULL);
      RtlInitUnicodeString(&PdoDeviceExtension->InstanceID, NULL);
      RtlInitUnicodeString(&PdoDeviceExtension->HardwareIDs, NULL);

      AcpiStatus = bm_get_node(Device->BmHandle, 0, &Node);
      if (ACPI_SUCCESS(AcpiStatus))
      {
        /* Get current resources */
        Buffer.length = 0;
        Status = acpi_get_current_resources(Node->device.acpi_handle, &Buffer);
        if ((Status & ACPI_OK) == 0)
        {
          ASSERT(FALSE);
        }
        if (Buffer.length > 0)
        {
          Buffer.pointer = ExAllocatePool(PagedPool, Buffer.length);
          if (!Buffer.pointer)
          {
            ASSERT(FALSE);
          }
          Status = acpi_get_current_resources(Node->device.acpi_handle, &Buffer);
          if (ACPI_FAILURE(Status))
          {
            ASSERT(FALSE);
          }
          if (!AcpiCreateResourceList(&PdoDeviceExtension->ResourceList,
                                      &PdoDeviceExtension->ResourceListSize,
                                      &PdoDeviceExtension->ResourceRequirementsList,
                                      &PdoDeviceExtension->ResourceRequirementsListSize,
                                      (RESOURCE*)Buffer.pointer))
          {
            ASSERT(FALSE);
          }
          ExFreePool(Buffer.pointer);
        }

        /* Add Device ID string */
        if (!AcpiCreateDeviceIDString(&PdoDeviceExtension->DeviceID,
                                      Node))
        {
          ASSERT(FALSE);
//          ErrorStatus = STATUS_INSUFFICIENT_RESOURCES;
//          ErrorOccurred = TRUE;
//          break;
        }

        if (!AcpiCreateInstanceIDString(&PdoDeviceExtension->InstanceID,
                                        Node))
        {
          ASSERT(FALSE);
//          ErrorStatus = STATUS_INSUFFICIENT_RESOURCES;
//          ErrorOccurred = TRUE;
//          break;
        }

        if (!AcpiCreateHardwareIDsString(&PdoDeviceExtension->HardwareIDs,
                                         Node))
        {
          ASSERT(FALSE);
//          ErrorStatus = STATUS_INSUFFICIENT_RESOURCES;
//          ErrorOccurred = TRUE;
//          break;
        }

        if (!AcpiCreateDeviceDescriptionString(&PdoDeviceExtension->DeviceDescription,
                                            Node))
        {
          ASSERT(FALSE);
//          ErrorStatus = STATUS_INSUFFICIENT_RESOURCES;
//          ErrorOccurred = TRUE;
//          break;
        }
      }
    }

    /* Reference the physical device object. The PnP manager
       will dereference it again when it is no longer needed */
    ObReferenceObject(Device->Pdo);

    Relations->Objects[i] = Device->Pdo;

    i++;

    CurrentEntry = CurrentEntry->Flink;
  }

  Irp->IoStatus.Information = (ULONG)Relations;

  return Status;
}

#ifndef NDEBUG
static VOID
ACPIPrintInfo(
  PFDO_DEVICE_EXTENSION DeviceExtension)
{
  DbgPrint("ACPI: System firmware supports:\n");

  /*
   * Print out basic system information
   */
  DbgPrint("+------------------------------------------------------------\n");
  DbgPrint("| Sx states: %cS0 %cS1 %cS2 %cS3 %cS4 %cS5\n",
           (DeviceExtension->SystemStates[0]?'+':'-'),
           (DeviceExtension->SystemStates[1]?'+':'-'),
           (DeviceExtension->SystemStates[2]?'+':'-'),
           (DeviceExtension->SystemStates[3]?'+':'-'),
           (DeviceExtension->SystemStates[4]?'+':'-'),
           (DeviceExtension->SystemStates[5]?'+':'-'));
  DbgPrint("+------------------------------------------------------------\n");
}
#endif

static NTSTATUS
ACPIInitializeInternalDriver(
  PFDO_DEVICE_EXTENSION DeviceExtension,
  ACPI_DRIVER_FUNCTION Initialize,
  ACPI_DRIVER_FUNCTION Terminate)
{
  ACPI_STATUS AcpiStatus;

  AcpiStatus = Initialize();
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("BN init status 0x%X\n", AcpiStatus);
    return STATUS_UNSUCCESSFUL;
  }
#if 0
  AcpiDevice = (PACPI_DEVICE)ExAllocatePool(
    NonPagedPool, sizeof(ACPI_DEVICE));
  if (!AcpiDevice) {
    return STATUS_INSUFFICIENT_RESOURCES;
  }

  AcpiDevice->Initialize = Initialize;
  AcpiDevice->Terminate = Terminate;

  /* FIXME: Create PDO */

  AcpiDevice->Pdo = NULL;
  //AcpiDevice->BmHandle = HandleList.handles[i];

  ExInterlockedInsertHeadList(&DeviceExtension->DeviceListHead,
    &AcpiDevice->ListEntry, &DeviceExtension->DeviceListLock);
#endif
  return STATUS_SUCCESS;
}


static NTSTATUS
ACPIInitializeInternalDrivers(
  PFDO_DEVICE_EXTENSION DeviceExtension)
{
  NTSTATUS Status;

  Status = ACPIInitializeInternalDriver(DeviceExtension,
    bn_initialize, bn_terminate);

  return STATUS_SUCCESS;
}


static NTSTATUS
FdoStartDevice(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp)
{
  PFDO_DEVICE_EXTENSION DeviceExtension;
	ACPI_PHYSICAL_ADDRESS rsdp;
	ACPI_SYSTEM_INFO SysInfo;
  ACPI_STATUS AcpiStatus;
  ACPI_BUFFER	Buffer;
	UCHAR TypeA, TypeB;
  ULONG i;

  DPRINT("Called\n");

  DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  ASSERT(DeviceExtension->State == dsStopped);

  AcpiStatus = acpi_initialize_subsystem();
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("acpi_initialize_subsystem() failed with status 0x%X\n", AcpiStatus);
    return STATUS_UNSUCCESSFUL;
  }

  AcpiStatus = acpi_find_root_pointer(&rsdp);
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("acpi_find_root_pointer() failed with status 0x%X\n", AcpiStatus);
    return STATUS_UNSUCCESSFUL;
  }

  /* From this point on, on error we must call acpi_terminate() */

  AcpiStatus = acpi_load_tables(rsdp);
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("acpi_load_tables() failed with status 0x%X\n", AcpiStatus);
    acpi_terminate();
    return STATUS_UNSUCCESSFUL;
  }

  Buffer.length  = sizeof(SysInfo);
  Buffer.pointer = &SysInfo;

  AcpiStatus = acpi_get_system_info(&Buffer);
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("acpi_get_system_info() failed with status 0x%X\n", AcpiStatus);
    acpi_terminate();
    return STATUS_UNSUCCESSFUL;
  }

  DPRINT("ACPI CA Core Subsystem version 0x%X\n", SysInfo.acpi_ca_version);

  ASSERT(SysInfo.num_table_types > ACPI_TABLE_FADT);

  RtlMoveMemory(&acpi_fadt,
    &SysInfo.table_info[ACPI_TABLE_FADT],
    sizeof(FADT_DESCRIPTOR_REV2));

  AcpiStatus = acpi_enable_subsystem(ACPI_FULL_INITIALIZATION);
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("acpi_enable_subsystem() failed with status 0x%X\n", AcpiStatus);
    acpi_terminate();
    return STATUS_UNSUCCESSFUL;
  }

  DPRINT("ACPI CA Core Subsystem enabled\n");

  /*
   * Sx States:
   * ----------
   * Figure out which Sx states are supported
   */
  for (i=0; i<=ACPI_S_STATES_MAX; i++) {
    AcpiStatus = acpi_hw_obtain_sleep_type_register_data(
			i,
			&TypeA,
			&TypeB);
    DPRINT("acpi_hw_obtain_sleep_type_register_data (%d) status 0x%X\n",
      i, AcpiStatus);
    if (ACPI_SUCCESS(AcpiStatus)) {
      DeviceExtension->SystemStates[i] = TRUE;
    }
  }

#ifndef NDEBUG
  ACPIPrintInfo(DeviceExtension);
#endif

  /* Initialize ACPI bus manager */
  AcpiStatus = bm_initialize();
  if (!ACPI_SUCCESS(AcpiStatus)) {
    DPRINT("bm_initialize() failed with status 0x%X\n", AcpiStatus);
    acpi_terminate();
    return STATUS_UNSUCCESSFUL;
  }

  InitializeListHead(&DeviceExtension->DeviceListHead);
  KeInitializeSpinLock(&DeviceExtension->DeviceListLock);
  DeviceExtension->DeviceListCount = 0;

  ACPIEnumerateDevices(DeviceExtension);

  ACPIInitializeInternalDrivers(DeviceExtension);

  DeviceExtension->State = dsStarted;

  return STATUS_SUCCESS;
}


static NTSTATUS
FdoSetPower(
  IN PDEVICE_OBJECT DeviceObject,
  IN PIRP Irp,
  PIO_STACK_LOCATION IrpSp)
{
  PFDO_DEVICE_EXTENSION DeviceExtension;
  ACPI_STATUS AcpiStatus;
  NTSTATUS Status;
  ULONG AcpiState;

  DPRINT("Called\n");

  DeviceExtension = (PFDO_DEVICE_EXTENSION)DeviceObject->DeviceExtension;

  if (IrpSp->Parameters.Power.Type == SystemPowerState) {
    Status = STATUS_SUCCESS;
    switch (IrpSp->Parameters.Power.State.SystemState) {
    case PowerSystemSleeping1:
      AcpiState = ACPI_STATE_S1;
      break;
    case PowerSystemSleeping2:
      AcpiState = ACPI_STATE_S2;
      break;
    case PowerSystemSleeping3:
      AcpiState = ACPI_STATE_S3;
      break;
    case PowerSystemHibernate:
      AcpiState = ACPI_STATE_S4;
      break;
    case PowerSystemShutdown:
      AcpiState = ACPI_STATE_S5;
      break;
    default:
      Status = STATUS_UNSUCCESSFUL;
      return Status;
    }
    if (!DeviceExtension->SystemStates[AcpiState]) {
      DPRINT("System sleep state S%d is not supported by hardware\n", AcpiState);
      Status = STATUS_UNSUCCESSFUL;
    }

    if (NT_SUCCESS(Status)) {
      DPRINT("Trying to enter sleep state %d\n", AcpiState);

      AcpiStatus = acpi_enter_sleep_state(AcpiState);
      if (!ACPI_SUCCESS(AcpiStatus)) {
        DPRINT("Failed to enter sleep state %d (Status 0x%X)\n",
          AcpiState, AcpiStatus);
        Status = STATUS_UNSUCCESSFUL;
      }
    }
  } else {
    Status = STATUS_UNSUCCESSFUL;
  }

  return Status;
}


/*** PUBLIC ******************************************************************/

NTSTATUS
NTAPI
FdoPnpControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: Handle Plug and Play IRPs for the ACPI device
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the ACPI driver
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
  //case IRP_MN_CANCEL_REMOVE_DEVICE:
  //  Status = STATUS_NOT_IMPLEMENTED;
  //  break;

  //case IRP_MN_CANCEL_STOP_DEVICE:
  //  Status = STATUS_NOT_IMPLEMENTED;
  //  break;

  //case IRP_MN_DEVICE_USAGE_NOTIFICATION:
  //  Status = STATUS_NOT_IMPLEMENTED;
  //  break;

  //case IRP_MN_FILTER_RESOURCE_REQUIREMENTS:
  //  Status = STATUS_NOT_IMPLEMENTED;
  //  break;

  case IRP_MN_QUERY_DEVICE_RELATIONS:
    Status = FdoQueryBusRelations(DeviceObject, Irp, IrpSp);
    break;

  //case IRP_MN_QUERY_PNP_DEVICE_STATE:
  //  Status = STATUS_NOT_IMPLEMENTED;
  //  break;

  //case IRP_MN_QUERY_REMOVE_DEVICE:
  //  Status = STATUS_NOT_IMPLEMENTED;
  //  break;

  //case IRP_MN_QUERY_STOP_DEVICE:
  //  Status = STATUS_NOT_IMPLEMENTED;
  //  break;

  //case IRP_MN_REMOVE_DEVICE:
  //  Status = STATUS_NOT_IMPLEMENTED;
  //  break;

  case IRP_MN_START_DEVICE:
    DPRINT("IRP_MN_START_DEVICE received\n");
    Status = FdoStartDevice(DeviceObject, Irp);
    break;

  case IRP_MN_STOP_DEVICE:
    /* Currently not supported */
    //bm_terminate();
    Status = STATUS_UNSUCCESSFUL;
    break;

  //case IRP_MN_SURPRISE_REMOVAL:
  //  Status = STATUS_NOT_IMPLEMENTED;
  //  break;

  default:
    DPRINT("Unknown IOCTL 0x%X\n", IrpSp->MinorFunction);
    Status = Irp->IoStatus.Status;
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
NTAPI
FdoPowerControl(
  PDEVICE_OBJECT DeviceObject,
  PIRP Irp)
/*
 * FUNCTION: Handle power management IRPs for the ACPI device
 * ARGUMENTS:
 *     DeviceObject = Pointer to functional device object of the ACPI driver
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
    Status = FdoSetPower(DeviceObject, Irp, IrpSp);
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
