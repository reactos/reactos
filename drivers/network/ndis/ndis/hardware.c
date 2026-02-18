/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        ndis/hardware.c
 * PURPOSE:     Hardware related routines
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Vizzini (vizzini@plasmic.com)
 * REVISIONS:
 *   CSH 01/08-2000 Created
 *   25 Aug 2003 Vizzini - NDIS4/5 and PnP additions
 *   3  Oct 2003 Vizzini - formatting and minor bugfixes
 *
 */

#include "ndissys.h"
#include <wdmguid.h>

NTSTATUS
NdisQueryPciBusInterface(
    IN PLOGICAL_ADAPTER Adapter)
{
    KEVENT Event;
    NTSTATUS Status;
    PIRP Irp;
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IrpStack;

    if (Adapter->BusInterfaceQueried) {
      return (Adapter->BusInterface.GetBusData != NULL) ? STATUS_SUCCESS : STATUS_NOT_SUPPORTED;
    }

    Adapter->BusInterfaceQueried = TRUE;

    KeInitializeEvent(&Event, NotificationEvent, FALSE);

    Irp = IoBuildSynchronousFsdRequest(IRP_MJ_PNP,
                       Adapter->NdisMiniportBlock.NextDeviceObject,
                       NULL,
                       0,
                       NULL,
                       &Event,
                       &IoStatusBlock);
    if (Irp == NULL) {
      return STATUS_INSUFFICIENT_RESOURCES;
    }

    IrpStack = IoGetNextIrpStackLocation(Irp);
    IrpStack->MajorFunction = IRP_MJ_PNP;
    IrpStack->MinorFunction = IRP_MN_QUERY_INTERFACE;
    IrpStack->Parameters.QueryInterface.InterfaceType = &GUID_BUS_INTERFACE_STANDARD;
    IrpStack->Parameters.QueryInterface.Size = sizeof(Adapter->BusInterface);
    IrpStack->Parameters.QueryInterface.Version = PCI_BUS_INTERFACE_STANDARD_VERSION;
    IrpStack->Parameters.QueryInterface.Interface = (PINTERFACE)&Adapter->BusInterface;
    IrpStack->Parameters.QueryInterface.InterfaceSpecificData = NULL;

    Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;

    Status = IoCallDriver(Adapter->NdisMiniportBlock.NextDeviceObject, Irp);
    if (Status == STATUS_PENDING) {
      KeWaitForSingleObject(&Event, Executive, KernelMode, FALSE, NULL);
      Status = IoStatusBlock.Status;
    }

    if (!NT_SUCCESS(Status))
    {
        RtlZeroMemory(&Adapter->BusInterface, sizeof(Adapter->BusInterface));
    }

    return Status;
}

/*
 * @implemented
 */
ULONG
EXPORT
NdisImmediateReadPciSlotInformation(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       SlotNumber,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
{
  PNDIS_WRAPPER_CONTEXT WrapperContext = (PNDIS_WRAPPER_CONTEXT)WrapperConfigurationContext;
  /* Slot number is ignored. */
  return HalGetBusDataByOffset(PCIConfiguration, WrapperContext->BusNumber,
                               WrapperContext->SlotNumber, Buffer, Offset, Length);
}

/*
 * @implemented
 */
ULONG
EXPORT
NdisImmediateWritePciSlotInformation(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  ULONG       SlotNumber,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
{
  PNDIS_WRAPPER_CONTEXT WrapperContext = (PNDIS_WRAPPER_CONTEXT)WrapperConfigurationContext;
  /* Slot number is ignored. */
  return HalSetBusDataByOffset(PCIConfiguration, WrapperContext->BusNumber,
                               WrapperContext->SlotNumber, Buffer, Offset, Length);
}

/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisMPciAssignResources(
    IN  NDIS_HANDLE             MiniportAdapterHandle,
    IN  ULONG                   SlotNumber,
    OUT PNDIS_RESOURCE_LIST     *AssignedResources)
{
  PLOGICAL_ADAPTER Adapter = MiniportAdapterHandle;

  if (Adapter->NdisMiniportBlock.BusType != NdisInterfacePci ||
      Adapter->NdisMiniportBlock.AllocatedResources == NULL)
    {
      NDIS_DbgPrint(MIN_TRACE, ("Bad bus type or no resources\n"));
      *AssignedResources = NULL;
      return NDIS_STATUS_FAILURE;
    }

  *AssignedResources = &Adapter->NdisMiniportBlock.AllocatedResources->List[0].PartialResourceList;

  return NDIS_STATUS_SUCCESS;
}

/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisPciAssignResources(
    IN  NDIS_HANDLE         NdisMacHandle,
    IN  NDIS_HANDLE         NdisWrapperHandle,
    IN  NDIS_HANDLE         WrapperConfigurationContext,
    IN  ULONG               SlotNumber,
    OUT PNDIS_RESOURCE_LIST *AssignedResources)
{
    PNDIS_WRAPPER_CONTEXT WrapperContext = (PNDIS_WRAPPER_CONTEXT)WrapperConfigurationContext;
    PLOGICAL_ADAPTER Adapter = WrapperContext->DeviceObject->DeviceExtension;

	return NdisMPciAssignResources(Adapter,
                                   SlotNumber,
                                   AssignedResources);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisMQueryAdapterResources(
    OUT     PNDIS_STATUS        Status,
    IN      NDIS_HANDLE         WrapperConfigurationContext,
    OUT     PNDIS_RESOURCE_LIST ResourceList,
    IN OUT  PUINT               BufferSize)
/*
 * FUNCTION: returns a nic's hardware resources
 * ARGUMENTS:
 *     Status: on return, contains the status of the operation
 *     WrapperConfigurationContext: handle input to MiniportInitialize
 *     ResourceList: on return, contains the list of resources for the nic
 *     BufferSize: size of ResourceList
 * NOTES:
 *     - Caller must allocate Status and ResourceList
 *     - Must be called at IRQL = PASSIVE_LEVEL;
 */
{
  PNDIS_WRAPPER_CONTEXT WrapperContext = (PNDIS_WRAPPER_CONTEXT)WrapperConfigurationContext;
  PLOGICAL_ADAPTER Adapter = WrapperContext->DeviceObject->DeviceExtension;
  ULONG ResourceListSize;

  PAGED_CODE();
  ASSERT((Status && ResourceList) || (BufferSize && *BufferSize == 0));

  NDIS_DbgPrint(MAX_TRACE, ("Called\n"));

  if (Adapter->NdisMiniportBlock.AllocatedResources == NULL)
    {
      NDIS_DbgPrint(MIN_TRACE, ("No allocated resources!\n"));
      *Status = NDIS_STATUS_FAILURE;
      return;
    }

  ResourceListSize =
    FIELD_OFFSET(CM_PARTIAL_RESOURCE_LIST, PartialDescriptors) +
    Adapter->NdisMiniportBlock.AllocatedResources->List[0].PartialResourceList.Count *
    sizeof(CM_PARTIAL_RESOURCE_DESCRIPTOR);

  if (*BufferSize >= ResourceListSize)
    {
      RtlCopyMemory(ResourceList,
                    &Adapter->NdisMiniportBlock.AllocatedResources->List[0].PartialResourceList,
                    ResourceListSize);
      *BufferSize = ResourceListSize;
      *Status = NDIS_STATUS_SUCCESS;
    }
  else
    {
      *BufferSize = ResourceListSize;
      *Status = NDIS_STATUS_RESOURCES;
    }
}


/*
 * @implemented
 */
NDIS_STATUS
EXPORT
NdisQueryMapRegisterCount(
    IN  NDIS_INTERFACE_TYPE BusType,
    OUT PUINT               MapRegisterCount)
/*
 * On X86 (and all other current hardware), map registers aren't real hardware,
 * and there is no real limit to the number that can be allocated.
 * As such, we do what microsoft does on the x86 hals and return as follows
 */
{
  return NDIS_STATUS_NOT_SUPPORTED;
}


/*
 * @implemented
 */
ULONG
EXPORT
NdisReadPciSlotInformation(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  ULONG       SlotNumber,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
{
  PLOGICAL_ADAPTER Adapter = NdisAdapterHandle;

  /* Slot number is ignored since W2K for all NDIS drivers. */
  if (Adapter->BusInterface.GetBusData != NULL) {
      ULONG Result;
      Result = Adapter->BusInterface.GetBusData(Adapter->BusInterface.Context,
                                                PCI_WHICHSPACE_CONFIG,
                                                Buffer,
                                                Offset,
                                                Length);
      NDIS_DbgPrint(MAX_TRACE, ("NdisReadPciSlotInformation: Using bus interface, read %u bytes\n", Result));
      return Result;
  }

  /* Fall back to HAL functions ONLY if bus interface is not available */
  return HalGetBusDataByOffset(PCIConfiguration,
                               Adapter->NdisMiniportBlock.BusNumber, Adapter->NdisMiniportBlock.SlotNumber,
                               Buffer, Offset, Length);
}

/*
 * @implemented
 */
ULONG
EXPORT
NdisWritePciSlotInformation(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  ULONG       SlotNumber,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
{
  PLOGICAL_ADAPTER Adapter = NdisAdapterHandle;

  /* Slot number is ignored since W2K for all NDIS drivers. */
  if (Adapter->BusInterface.SetBusData != NULL) {
      ULONG Result;
      Result = Adapter->BusInterface.SetBusData(Adapter->BusInterface.Context,
                                                PCI_WHICHSPACE_CONFIG,
                                                Buffer,
                                                Offset,
                                                Length);
      NDIS_DbgPrint(MAX_TRACE, ("NdisWritePciSlotInformation: Using bus interface, wrote %u bytes\n", Result));
      return Result;
  }

  /* Fall back to HAL functions ONLY if bus interface is not available */
  return HalSetBusDataByOffset(PCIConfiguration,
                               Adapter->NdisMiniportBlock.BusNumber, Adapter->NdisMiniportBlock.SlotNumber,
                               Buffer, Offset, Length);
}


/*
 * @implemented
 */
VOID
EXPORT
NdisReadEisaSlotInformation(
    OUT PNDIS_STATUS                    Status,
    IN  NDIS_HANDLE                     WrapperConfigurationContext,
    OUT PUINT                           SlotNumber,
    OUT PNDIS_EISA_FUNCTION_INFORMATION EisaData)
{
    PNDIS_WRAPPER_CONTEXT Wrapper = WrapperConfigurationContext;
    ULONG Ret;
    PVOID Buffer;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    /* We are called only at PASSIVE_LEVEL */
    Buffer = ExAllocatePool(PagedPool, sizeof(NDIS_EISA_FUNCTION_INFORMATION));
    if (!Buffer) {
         NDIS_DbgPrint(MIN_TRACE, ("Insufficient resources.\n"));
        *Status = NDIS_STATUS_RESOURCES;
        return;
    }

    Ret = HalGetBusData(EisaConfiguration,
                        Wrapper->BusNumber,
                        Wrapper->SlotNumber,
                        Buffer,
                        sizeof(NDIS_EISA_FUNCTION_INFORMATION));

    if (Ret == 0 || Ret == 2) {
        NDIS_DbgPrint(MIN_TRACE, ("HalGetBusData failed.\n"));
        ExFreePool(Buffer);
        *Status = NDIS_STATUS_FAILURE;
        return;
    }

    *SlotNumber = Wrapper->SlotNumber;

    RtlCopyMemory(EisaData, Buffer, sizeof(NDIS_EISA_FUNCTION_INFORMATION));

    ExFreePool(Buffer);

    *Status = NDIS_STATUS_SUCCESS;
}


/*
 * @implemented
 */
ULONG
EXPORT
NdisReadPcmciaAttributeMemory(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PLOGICAL_ADAPTER Adapter = NdisAdapterHandle;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return HalGetBusDataByOffset(PCMCIAConfiguration,
                                 Adapter->NdisMiniportBlock.BusNumber,
                                 Adapter->NdisMiniportBlock.SlotNumber,
                                 Buffer,
                                 Offset,
                                 Length);
}


/*
 * @implemented
 */
ULONG
EXPORT
NdisWritePcmciaAttributeMemory(
    IN  NDIS_HANDLE NdisAdapterHandle,
    IN  ULONG       Offset,
    IN  PVOID       Buffer,
    IN  ULONG       Length)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 5.0
 */
{
    PLOGICAL_ADAPTER Adapter = NdisAdapterHandle;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    return HalSetBusDataByOffset(PCMCIAConfiguration,
                                 Adapter->NdisMiniportBlock.BusNumber,
                                 Adapter->NdisMiniportBlock.SlotNumber,
                                 Buffer,
                                 Offset,
                                 Length);
}

/*
 * @implemented
 */
VOID
EXPORT
NdisOverrideBusNumber(
    IN  NDIS_HANDLE WrapperConfigurationContext,
    IN  NDIS_HANDLE MiniportAdapterHandle   OPTIONAL,
    IN  ULONG       BusNumber)
/*
 * FUNCTION:
 * ARGUMENTS:
 * NOTES:
 *    NDIS 4.0
 */
{
    PNDIS_WRAPPER_CONTEXT Wrapper = WrapperConfigurationContext;
    PLOGICAL_ADAPTER Adapter = MiniportAdapterHandle;

    NDIS_DbgPrint(MAX_TRACE, ("Called.\n"));

    Wrapper->BusNumber = BusNumber;

    if (Adapter)
        Adapter->NdisMiniportBlock.BusNumber = BusNumber;
}

/* EOF */
