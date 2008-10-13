/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS NDIS library
 * FILE:        workitem.c
 * PURPOSE:     Implements the NDIS 6.0 work item interface
 * PROGRAMMERS: Cameron Gutman (aicommander@gmail.com)
 */

#include "ndissys.h"

NDIS_HANDLE
EXPORT
NdisAllocateIoWorkItem(
    IN NDIS_HANDLE NdisObjectHandle)
{
   PLOGICAL_ADAPTER Adapter = NdisObjectHandle;

   return IoAllocateWorkItem(Adapter->NdisMiniportBlock.PhysicalDeviceObject);
}

VOID
EXPORT
NdisQueueIoWorkItem(
    IN NDIS_HANDLE NdisIoWorkItemHandle,
    IN NDIS_IO_WORKITEM_ROUTINE Routine,
    IN PVOID WorkItemContext)
{
   PNDIS_IO_WORKITEM WorkItem = NdisIoWorkItemHandle;

   IoQueueWorkItem(WorkItem,
                   Routine,
                   CriticalWorkQueue,
                   WorkItemContext);
}

VOID
EXPORT
NdisFreeIoWorkItem(
    IN NDIS_HANDLE NdisIoWorkItemHandle)
{
   PNDIS_IO_WORKITEM WorkItem = NdisIoWorkItemHandle;
   IoFreeWorkItem(WorkItem);
}
