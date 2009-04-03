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
  return HalGetBusDataByOffset(PCIConfiguration, WrapperContext->BusNumber,
                               SlotNumber, Buffer, Offset, Length);
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
  return HalSetBusDataByOffset(PCIConfiguration, WrapperContext->BusNumber,
                               SlotNumber, Buffer, Offset, Length);
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

  if (Adapter->NdisMiniportBlock.BusType != PCIBus ||
      Adapter->NdisMiniportBlock.AllocatedResources == NULL)
    {
      *AssignedResources = NULL;
      return NDIS_STATUS_FAILURE;
    }

  *AssignedResources = &Adapter->NdisMiniportBlock.AllocatedResources->List[0].PartialResourceList;

  return NDIS_STATUS_SUCCESS;
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
  return HalSetBusDataByOffset(PCIConfiguration,
                               Adapter->NdisMiniportBlock.BusNumber, Adapter->NdisMiniportBlock.SlotNumber,
                               Buffer, Offset, Length);
}

/* EOF */
