/*
 * OHCI WDM/PNP driver
 *
 * Copyright (C) 2005 ReactOS Team
 *
 * Author: Aleksey Bragin (aleksey@reactos.com)
 *
 */

#ifndef OHCI_MAIN_H
#define OHCI_MAIN_H

typedef struct _OHCI_DRIVER_EXTENSION
{
   //OHCI_HW_INITIALIZATION_DATA InitializationData;
   PVOID HwContext;
   //UNICODE_STRING RegistryPath;

} OHCI_DRIVER_EXTENSION, *POHCI_DRIVER_EXTENSION;

typedef struct _OHCI_DEVICE_EXTENSTION
{
   ULONG DeviceNumber;
   PDEVICE_OBJECT PhysicalDeviceObject;
   PDEVICE_OBJECT FunctionalDeviceObject;
   PDEVICE_OBJECT NextDeviceObject;
   //UNICODE_STRING RegistryPath;
   PKINTERRUPT InterruptObject;
   KSPIN_LOCK InterruptSpinLock;
   PCM_RESOURCE_LIST AllocatedResources;
   ULONG InterruptVector;
   ULONG InterruptLevel;
   PHYSICAL_ADDRESS BaseAddress;
   ULONG BaseAddrLength;
   ULONG Flags;
   ULONG AdapterInterfaceType;
   ULONG SystemIoBusNumber;
   ULONG SystemIoSlotNumber;
   LIST_ENTRY AddressMappingListHead;
   //KDPC DpcObject;
   OHCI_DRIVER_EXTENSION *DriverExtension;
   ULONG DeviceOpened;
   //KMUTEX DeviceLock;
   //CHAR MiniPortDeviceExtension[1];
   BOOLEAN IsFDO;
   struct pci_dev * pdev;
} OHCI_DEVICE_EXTENSION, *POHCI_DEVICE_EXTENSION;


#endif
