/*
 * VideoPort driver
 *
 * Copyright (C) 2002, 2003, 2004 ReactOS Team
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this library; see the file COPYING.LIB.
 * If not, write to the Free Software Foundation,
 * 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 * $Id$
 */

#include "videoprt.h"
#include <initguid.h>
#include <wdmguid.h>

/* PRIVATE FUNCTIONS **********************************************************/

NTSTATUS
IopInitiatePnpIrp(
  PDEVICE_OBJECT DeviceObject,
  PIO_STATUS_BLOCK IoStatusBlock,
  ULONG MinorFunction,
  PIO_STACK_LOCATION Stack OPTIONAL)
{
  PDEVICE_OBJECT TopDeviceObject;
  PIO_STACK_LOCATION IrpSp;
  NTSTATUS Status;
  KEVENT Event;
  PIRP Irp;

  /* Always call the top of the device stack */
  TopDeviceObject = IoGetAttachedDeviceReference(DeviceObject);

  KeInitializeEvent(
    &Event,
    NotificationEvent,
    FALSE);

  Irp = IoBuildSynchronousFsdRequest(
    IRP_MJ_PNP,
    TopDeviceObject,
    NULL,
    0,
    NULL,
    &Event,
    IoStatusBlock);

  /* PNP IRPs are always initialized with a status code of
     STATUS_NOT_IMPLEMENTED */
  Irp->IoStatus.Status = STATUS_NOT_IMPLEMENTED;
  Irp->IoStatus.Information = 0;

  IrpSp = IoGetNextIrpStackLocation(Irp);
  IrpSp->MinorFunction = MinorFunction;

  if (Stack)
  {
    RtlMoveMemory(
      &IrpSp->Parameters,
      &Stack->Parameters,
      sizeof(Stack->Parameters));
  }

  Status = IoCallDriver(TopDeviceObject, Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject(
        &Event,
        Executive,
        KernelMode,
        FALSE,
        NULL);
      Status = IoStatusBlock->Status;
    }

  ObDereferenceObject(TopDeviceObject);

  return Status;
}


BOOLEAN NTAPI
IntAgpCommitPhysical(
   IN PVOID HwDeviceExtension,
   IN PVOID PhysicalContext,
   IN ULONG Pages,
   IN ULONG Offset)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PAGP_BUS_INTERFACE_STANDARD AgpBusInterface;
   PHYSICAL_ADDRESS MappingAddr = {{0}};
   PVIDEO_PORT_AGP_MAPPING AgpMapping;
   NTSTATUS Status;

   DPRINT("AgpCommitPhysical - PhysicalContext: 0x%x Pages: %d, Offset: 0x%x\n",
          PhysicalContext, Pages, Offset);

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   AgpBusInterface = &DeviceExtension->AgpInterface;
   AgpMapping = (PVIDEO_PORT_AGP_MAPPING)PhysicalContext;

   Status = AgpBusInterface->CommitMemory(AgpBusInterface->AgpContext,
                                          AgpMapping->MapHandle, Pages, Offset,
                                          NULL, &MappingAddr);

   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Warning: AgpBusInterface->CommitMemory failed (Status = 0x%x)\n",
              Status);
   }
   return NT_SUCCESS(Status);
}

VOID NTAPI
IntAgpFreePhysical(
   IN PVOID HwDeviceExtension,
   IN PVOID PhysicalContext,
   IN ULONG Pages,
   IN ULONG Offset)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PAGP_BUS_INTERFACE_STANDARD AgpBusInterface;
   PVIDEO_PORT_AGP_MAPPING AgpMapping;
   NTSTATUS Status;

   DPRINT("AgpFreePhysical - PhysicalContext: 0x%x Pages: %d, Offset: 0x%x\n",
          PhysicalContext, Pages, Offset);

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   AgpBusInterface = &DeviceExtension->AgpInterface;
   AgpMapping = (PVIDEO_PORT_AGP_MAPPING)PhysicalContext;

   Status = AgpBusInterface->FreeMemory(AgpBusInterface->AgpContext,
                                        AgpMapping->MapHandle, Pages, Offset);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Warning: AgpBusInterface->FreeMemory failed (Status = 0x%x)\n",
              Status);
   }
}

VOID NTAPI
IntAgpReleasePhysical(
   IN PVOID HwDeviceExtension,
   IN PVOID PhysicalContext)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PAGP_BUS_INTERFACE_STANDARD AgpBusInterface;
   PVIDEO_PORT_AGP_MAPPING AgpMapping;
   NTSTATUS Status;

   DPRINT("AgpReleasePhysical - PhysicalContext: 0x%x\n", PhysicalContext);

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   AgpBusInterface = &DeviceExtension->AgpInterface;
   AgpMapping = (PVIDEO_PORT_AGP_MAPPING)PhysicalContext;
   
   /* Release memory */
   Status = AgpBusInterface->ReleaseMemory(AgpBusInterface->AgpContext,
                                           AgpMapping->MapHandle);
   if (!NT_SUCCESS(Status))
   {
      DPRINT1("Warning: AgpBusInterface->ReleaseMemory failed (Status = 0x%x)\n",
              Status);
   }
   
   /* Free resources */
   ExFreePool(AgpMapping);
}

PHYSICAL_ADDRESS NTAPI
IntAgpReservePhysical(
   IN  PVOID HwDeviceExtension,
   IN  ULONG Pages,
   IN  VIDEO_PORT_CACHE_TYPE Caching,
   OUT PVOID *PhysicalContext)
{
   PHYSICAL_ADDRESS ZeroAddress = {{0}};
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PAGP_BUS_INTERFACE_STANDARD AgpBusInterface;
   MEMORY_CACHING_TYPE MemCachingType;
   PVIDEO_PORT_AGP_MAPPING AgpMapping;
   NTSTATUS Status;

   DPRINT("AgpReservePhysical - Pages: %d, Caching: 0x%x\n", Pages, Caching);

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   AgpBusInterface = &DeviceExtension->AgpInterface;

   /* Translate memory caching type */
   if (Caching == VpNonCached)
     MemCachingType = MmNonCached;
   else if (Caching == VpCached)
     MemCachingType = MmCached;
   else if (Caching == VpWriteCombined)
     MemCachingType = MmWriteCombined;
   else
   {
      DPRINT1("Invalid caching type %d!\n", Caching);
      return ZeroAddress;
   }

   /* Allocate an AGP mapping structure */
   AgpMapping = ExAllocatePoolWithTag(PagedPool,
                                      sizeof(VIDEO_PORT_AGP_MAPPING),
                                      TAG_VIDEO_PORT);
   if (AgpMapping == NULL)
   {
      DPRINT1("Out of memory! Couldn't allocate AGP mapping structure!\n");
      return ZeroAddress;
   }
   RtlZeroMemory(AgpMapping, sizeof(VIDEO_PORT_AGP_MAPPING));

   /* Reserve memory for the AGP bus */
   Status = AgpBusInterface->ReserveMemory(AgpBusInterface->AgpContext,
                                           Pages,
                                           MemCachingType,
                                           &AgpMapping->MapHandle,
                                           &AgpMapping->PhysicalAddress);
   if (!NT_SUCCESS(Status) || AgpMapping->MapHandle == NULL)
   {
      ExFreePool(AgpMapping);
      DPRINT1("Warning: AgpBusInterface->ReserveMemory failed (Status = 0x%x)\n",
              Status);
      return ZeroAddress;
   }

   /* Fill the rest of the AGP mapping */
   AgpMapping->NumberOfPages = Pages;

   *PhysicalContext = (PVOID)AgpMapping;
   return AgpMapping->PhysicalAddress;
}
   

PVOID NTAPI
IntAgpCommitVirtual(
   IN PVOID HwDeviceExtension,
   IN PVOID VirtualContext,
   IN ULONG Pages,
   IN ULONG Offset)
{
   PVIDEO_PORT_AGP_VIRTUAL_MAPPING VirtualMapping;
   PVOID BaseAddress = NULL;
   NTSTATUS Status;

   DPRINT("AgpCommitVirtual - VirtualContext: 0x%x Pages: %d, Offset: 0x%x\n",
          VirtualContext, Pages, Offset);

   VirtualMapping = (PVIDEO_PORT_AGP_VIRTUAL_MAPPING)VirtualContext;

   /* I think the NT API provides no way of reserving a part of the address space
    * and setting it up to map into a specified range of physical memory later.
    * This means that we will have to release some of the reserved virtual memory
    * and map the physical memory into it using MapViewOfSection.
    *
    * - blight (2004-12-21)
    */

   if (VirtualMapping->ProcessHandle == NULL)
   {
      /* FIXME: not implemented */
   }
   else /* ProcessHandle != NULL */
   {
      /* Release some virtual memory */
      ULONG Size = Pages * PAGE_SIZE;
      ULONG OffsetInBytes = Offset * PAGE_SIZE;
      BaseAddress = (PVOID)((ULONG_PTR)VirtualMapping->MappedAddress +
                                       OffsetInBytes);
      PHYSICAL_ADDRESS PhysicalAddress = VirtualMapping->AgpMapping->PhysicalAddress;
      PhysicalAddress.QuadPart += OffsetInBytes;

      Status = ZwFreeVirtualMemory(VirtualMapping->ProcessHandle,
                                   &BaseAddress,
                                   &Size, MEM_RELEASE);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Warning: ZwFreeVirtualMemory() failed: Status = 0x%x\n", Status);
         return NULL;
      }
      ASSERT(Size == Pages * PAGE_SIZE);
      ASSERT(BaseAddress == (PVOID)((ULONG_PTR)VirtualMapping->MappedAddress +
                                               OffsetInBytes));
      
      /* Map the physical memory into the released virtual memory area */
      Status = IntVideoPortMapPhysicalMemory(VirtualMapping->ProcessHandle,
                                             PhysicalAddress,
                                             Size,
                                             PAGE_READWRITE,
                                             &BaseAddress);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Warning: IntVideoPortMapPhysicalMemory() failed: Status = 0x%x\n", Status);
         /* Reserve the released virtual memory area again */
         Status = ZwAllocateVirtualMemory(VirtualMapping->ProcessHandle,
                                          &BaseAddress, 0, &Size, MEM_RESERVE,
                                          PAGE_NOACCESS);
         if (!NT_SUCCESS(Status))
         {
            DPRINT1("Warning: ZwAllocateVirtualMemory() failed: Status = 0x%x\n", Status);
            /* FIXME: What to do now?? */
            ASSERT(0);
            return NULL;
         }
         ASSERT(Size == Pages * PAGE_SIZE);
         ASSERT(BaseAddress == (PVOID)((ULONG_PTR)VirtualMapping->MappedAddress +
                                               OffsetInBytes));
         return NULL;
      }
      ASSERT(BaseAddress == (PVOID)((ULONG_PTR)VirtualMapping->MappedAddress +
                                               OffsetInBytes));
   }

   return BaseAddress;
}

VOID NTAPI
IntAgpFreeVirtual(
   IN PVOID HwDeviceExtension,
   IN PVOID VirtualContext,
   IN ULONG Pages,
   IN ULONG Offset)
{
   PVIDEO_PORT_AGP_VIRTUAL_MAPPING VirtualMapping;
   PVOID BaseAddress = NULL;
   NTSTATUS Status;

   DPRINT("AgpFreeVirtual - VirtualContext: 0x%x Pages: %d, Offset: 0x%x\n",
          VirtualContext, Pages, Offset);

   VirtualMapping = (PVIDEO_PORT_AGP_VIRTUAL_MAPPING)VirtualContext;

   if (VirtualMapping->ProcessHandle == NULL)
   {
      /* FIXME: not implemented */
   }
   else /* ProcessHandle != NULL */
   {
      /* Unmap the section view */
      ULONG Size = Pages * PAGE_SIZE;
      ULONG OffsetInBytes = Offset * PAGE_SIZE;
      BaseAddress = (PVOID)((ULONG_PTR)VirtualMapping->MappedAddress +
                                       OffsetInBytes);

      Status = ZwUnmapViewOfSection(VirtualMapping->ProcessHandle, BaseAddress);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Warning: ZwUnmapViewOfSection() failed: Status = 0x%x\n", Status);
         /* FIXME: What to do now?? */
         ASSERT(0);
         return;
      }

      /* And reserve the virtual memory area again */
      Status = ZwAllocateVirtualMemory(VirtualMapping->ProcessHandle,
                                       &BaseAddress, 0, &Size, MEM_RESERVE,
                                       PAGE_NOACCESS);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Warning: ZwAllocateVirtualMemory() failed: Status = 0x%x\n", Status);
         /* FIXME: What to do now?? */
         ASSERT(0);
         return;
      }
      ASSERT(Size == Pages * PAGE_SIZE);
      ASSERT(BaseAddress == (PVOID)((ULONG_PTR)VirtualMapping->MappedAddress +
                                               OffsetInBytes));
   }
}

VOID NTAPI
IntAgpReleaseVirtual(
   IN PVOID HwDeviceExtension,
   IN PVOID VirtualContext)
{
   PVIDEO_PORT_AGP_VIRTUAL_MAPPING VirtualMapping;
   NTSTATUS Status;

   DPRINT("AgpReleaseVirtual - VirtualContext: 0x%x\n", VirtualContext);

   VirtualMapping = (PVIDEO_PORT_AGP_VIRTUAL_MAPPING)VirtualContext;
   
   /* Release the virtual memory */
   if (VirtualMapping->ProcessHandle == NULL)
   {
      /* FIXME: not implemented */
   }
   else /* ProcessHandle != NULL */
   {
      /* Release the allocated virtual memory */
      ULONG Size = VirtualMapping->AgpMapping->NumberOfPages * PAGE_SIZE;
      Status = ZwFreeVirtualMemory(VirtualMapping->ProcessHandle,
                                   &VirtualMapping->MappedAddress,
                                   &Size, MEM_RELEASE);
      if (!NT_SUCCESS(Status))
      {
         DPRINT1("Warning: ZwFreeVirtualMemory() failed: Status = 0x%x\n", Status);
      }
   }
   
   /* Free resources */
   ExFreePool(VirtualMapping);
}

PVOID NTAPI
IntAgpReserveVirtual(
   IN  PVOID HwDeviceExtension,
   IN  HANDLE ProcessHandle,
   IN  PVOID PhysicalContext,
   OUT PVOID *VirtualContext)
{
   PVIDEO_PORT_AGP_MAPPING AgpMapping;
   PVIDEO_PORT_AGP_VIRTUAL_MAPPING VirtualMapping;
   PVOID MappedAddress;
   NTSTATUS Status;

   DPRINT("AgpReserveVirtual - ProcessHandle: 0x%x PhysicalContext: 0x%x\n",
          ProcessHandle, PhysicalContext);

   AgpMapping = (PVIDEO_PORT_AGP_MAPPING)PhysicalContext;

   /* Allocate an AGP virtual mapping structure */
   VirtualMapping = ExAllocatePoolWithTag(PagedPool,
                                          sizeof(VIDEO_PORT_AGP_VIRTUAL_MAPPING),
                                          TAG_VIDEO_PORT);
   if (VirtualMapping == NULL)
   {
      DPRINT1("Out of memory! Couldn't allocate AGP virtual mapping structure!\n");
      return NULL;
   }
   RtlZeroMemory(VirtualMapping, sizeof(VIDEO_PORT_AGP_VIRTUAL_MAPPING));

   /* Reserve a virtual memory area for the physical pages. */
   if (ProcessHandle == NULL)
   {
      /* FIXME: What to do in this case? */
      ExFreePool(VirtualMapping);
      return NULL;
   }
   else /* ProcessHandle != NULL */
   {
      /* Reserve memory for usermode */
      ULONG Size = AgpMapping->NumberOfPages * PAGE_SIZE;
      MappedAddress = NULL;
      Status = ZwAllocateVirtualMemory(ProcessHandle, &MappedAddress, 0, &Size,
                                       MEM_RESERVE, PAGE_NOACCESS);
      if (!NT_SUCCESS(Status))
      {
         ExFreePool(VirtualMapping);
         DPRINT("ZwAllocateVirtualMemory() failed: Status = 0x%x\n", Status);
         return NULL;
      }
   }

   /* Fill the AGP virtual mapping */
   VirtualMapping->AgpMapping = AgpMapping;
   VirtualMapping->ProcessHandle = ProcessHandle;
   VirtualMapping->MappedAddress = MappedAddress;

   *VirtualContext = (PVOID)VirtualMapping;
   return MappedAddress;
}


BOOLEAN NTAPI
IntAgpSetRate(
   IN PVOID HwDeviceExtension,
   IN ULONG Rate)
{
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PAGP_BUS_INTERFACE_STANDARD AgpBusInterface;

   DPRINT("AgpSetRate - Rate: %d\n", Rate);

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   AgpBusInterface = &DeviceExtension->AgpInterface;
   
   return NT_SUCCESS(AgpBusInterface->SetRate(AgpBusInterface->AgpContext, Rate));
}


NTSTATUS NTAPI
IntAgpGetInterface(
   IN PVOID HwDeviceExtension,
   IN OUT PINTERFACE Interface)
{
   IO_STATUS_BLOCK IoStatusBlock;
   IO_STACK_LOCATION IoStack;
   NTSTATUS Status;
   PVIDEO_PORT_AGP_INTERFACE_2 AgpInterface;
   PVIDEO_PORT_DEVICE_EXTENSION DeviceExtension;
   PAGP_BUS_INTERFACE_STANDARD AgpBusInterface;

   DeviceExtension = VIDEO_PORT_GET_DEVICE_EXTENSION(HwDeviceExtension);
   AgpBusInterface = &DeviceExtension->AgpInterface;
   AgpInterface = (PVIDEO_PORT_AGP_INTERFACE_2)Interface;

   ASSERT((Interface->Version == VIDEO_PORT_AGP_INTERFACE_VERSION_2 &&
           Interface->Size >= sizeof(VIDEO_PORT_AGP_INTERFACE_2)) ||
          (Interface->Version == VIDEO_PORT_AGP_INTERFACE_VERSION_1 &&
           Interface->Size >= sizeof(VIDEO_PORT_AGP_INTERFACE)));

   if (DeviceExtension->NextDeviceObject == NULL)
   {
      DPRINT("DeviceExtension->NextDeviceObject is NULL!\n");
      return STATUS_UNSUCCESSFUL;
   }

   /* Query the interface from the AGP bus driver */
   if (DeviceExtension->AgpInterface.Size == 0)
   {
      AgpBusInterface->Size = sizeof(AGP_BUS_INTERFACE_STANDARD);
      if (Interface->Version == VIDEO_PORT_AGP_INTERFACE_VERSION_1)
         AgpBusInterface->Version = AGP_BUS_INTERFACE_V1;
      else /* if (InterfaceVersion == VIDEO_PORT_AGP_INTERFACE_VERSION_2) */
         AgpBusInterface->Version = AGP_BUS_INTERFACE_V2;
      IoStack.Parameters.QueryInterface.Size = AgpBusInterface->Size;
      IoStack.Parameters.QueryInterface.Version = AgpBusInterface->Version;
      IoStack.Parameters.QueryInterface.Interface = (PINTERFACE)AgpBusInterface;
      IoStack.Parameters.QueryInterface.InterfaceType =
         &GUID_AGP_TARGET_BUS_INTERFACE_STANDARD;
      Status = IopInitiatePnpIrp(DeviceExtension->NextDeviceObject,
         &IoStatusBlock, IRP_MN_QUERY_INTERFACE, &IoStack);
      if (!NT_SUCCESS(Status))
      {
         DPRINT("IopInitiatePnpIrp() failed! (Status 0x%x)\n", Status);
         return Status;
      }
      DPRINT("Got AGP driver interface!\n");
   }

   /* FIXME: Not sure if we should wrap the reference/dereference functions */
   AgpInterface->Context = AgpBusInterface->AgpContext;
   AgpInterface->InterfaceReference = AgpBusInterface->InterfaceReference;
   AgpInterface->InterfaceDereference = AgpBusInterface->InterfaceDereference;
   AgpInterface->AgpReservePhysical = IntAgpReservePhysical;
   AgpInterface->AgpReleasePhysical = IntAgpReleasePhysical;
   AgpInterface->AgpCommitPhysical = IntAgpCommitPhysical;
   AgpInterface->AgpFreePhysical = IntAgpFreePhysical;
   AgpInterface->AgpReserveVirtual = IntAgpReserveVirtual;
   AgpInterface->AgpReleaseVirtual = IntAgpReleaseVirtual;
   AgpInterface->AgpCommitVirtual = IntAgpCommitVirtual;
   AgpInterface->AgpFreeVirtual = IntAgpFreeVirtual;
   AgpInterface->AgpAllocationLimit = 0x1000000; /* FIXME: using 16 MB for now */

   if (AgpInterface->Version >= VIDEO_PORT_AGP_INTERFACE_VERSION_2)
   {
      AgpInterface->AgpSetRate = IntAgpSetRate;
   }

   return STATUS_SUCCESS;
}

