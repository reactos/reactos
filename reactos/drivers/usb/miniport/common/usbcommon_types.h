#ifndef _USBMP_COMMON_TYPES_H_
#define _USBMP_COMMON_TYPES_H_

typedef struct _USBMP_DRIVER_EXTENSION
{
   //OHCI_HW_INITIALIZATION_DATA InitializationData;
   //PVOID HwContext;
   //UNICODE_STRING RegistryPath;

} USBMP_DRIVER_EXTENSION, *PUSBMP_DRIVER_EXTENSION;

typedef struct _USBMP_DEVICE_EXTENSTION
{
   ULONG DeviceNumber;
   PDEVICE_OBJECT PhysicalDeviceObject;
   PDEVICE_OBJECT FunctionalDeviceObject;
   PDEVICE_OBJECT NextDeviceObject;
   //UNICODE_STRING RegistryPath;
   UNICODE_STRING HcdInterfaceName;
   //PKINTERRUPT InterruptObject;
   //KSPIN_LOCK InterruptSpinLock;
   PCM_RESOURCE_LIST AllocatedResources;
   ULONG InterruptVector;
   ULONG InterruptLevel;
   PHYSICAL_ADDRESS BaseAddress;
   ULONG BaseAddrLength;
   ULONG Flags;
   //ULONG AdapterInterfaceType;
   ULONG SystemIoBusNumber;
   ULONG SystemIoSlotNumber;
   //LIST_ENTRY AddressMappingListHead;

   // DMA stuff, and buffers
   PDMA_ADAPTER pDmaAdapter;
   //PVOID MapRegisterBase;
   ULONG mapRegisterCount;
#ifdef USB_DMA_SINGLE_SUPPORT
   //PHYSICAL_ADDRESS Buffer;
   //PVOID VirtualBuffer;
   //ULONG BufferSize;

   // Mdl used for single DMA transfers
   //PMDL Mdl;
#endif

   //KDPC DpcObject;
   PUSBMP_DRIVER_EXTENSION DriverExtension;
   ULONG DeviceOpened;
   //KMUTEX DeviceLock;
   //CHAR MiniPortDeviceExtension[1];
   BOOLEAN IsFDO;
   struct pci_dev * pdev;
   PDEVICE_OBJECT RootHubPdo;
} USBMP_DEVICE_EXTENSION, *PUSBMP_DEVICE_EXTENSION;

#endif
