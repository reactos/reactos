/*
 *  ReactOS kernel
 *  Copyright (C) 2001, 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: scsiport.c,v 1.61 2004/06/15 09:29:41 hbirr Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/scsiport/scsiport.c
 * PURPOSE:         SCSI port driver
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 *                  Hartmut Birr
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/srb.h>
#include <ddk/scsi.h>
#include <ddk/ntddscsi.h>
#include <ntos/minmax.h>
#include <rosrtl/string.h>

#define NDEBUG
#include <debug.h>

#include "scsiport_int.h"

/* TYPES *********************************************************************/

#define IRP_FLAG_COMPLETE	0x00000001
#define IRP_FLAG_NEXT		0x00000002
#define IRP_FLAG_NEXT_LU	0x00000004

/* GLOBALS *******************************************************************/

static VOID
SpiProcessRequests(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension, 
	           PIRP NextIrp);

static VOID 
SpiStartIo(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
	   PIRP Irp);

static BOOLEAN
SpiGetPciConfigData (IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
		     IN OUT PPORT_CONFIGURATION_INFORMATION PortConfig,
		     IN ULONG BusNumber,
		     IN OUT PPCI_SLOT_NUMBER NextSlotNumber);

static NTSTATUS STDCALL
ScsiPortCreateClose(IN PDEVICE_OBJECT DeviceObject,
		    IN PIRP Irp);

static NTSTATUS STDCALL
ScsiPortDispatchScsi(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp);

static NTSTATUS STDCALL
ScsiPortDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp);

static BOOLEAN STDCALL
ScsiPortStartPacket(IN OUT PVOID Context);


static PSCSI_PORT_LUN_EXTENSION
SpiAllocateLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
			 IN UCHAR PathId,
			 IN UCHAR TargetId,
			 IN UCHAR Lun);

static VOID
SpiRemoveLunExtension (IN PSCSI_PORT_LUN_EXTENSION LunExtension);

static PSCSI_PORT_LUN_EXTENSION
SpiGetLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		    IN UCHAR PathId,
		    IN UCHAR TargetId,
		    IN UCHAR Lun);

static NTSTATUS
SpiSendInquiry (IN PDEVICE_OBJECT DeviceObject,
		IN OUT PSCSI_REQUEST_BLOCK Srb,
		IN OUT PIO_STATUS_BLOCK IoStatusBlock,
		IN OUT PKEVENT Event);

static VOID
SpiScanAdapter (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

static ULONG
SpiGetInquiryData (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		   OUT PSCSI_ADAPTER_BUS_INFO AdapterBusInfo);

static BOOLEAN STDCALL
ScsiPortIsr(IN PKINTERRUPT Interrupt,
	    IN PVOID ServiceContext);

static VOID STDCALL
ScsiPortDpc(IN PKDPC Dpc,
	    IN PDEVICE_OBJECT DpcDeviceObject,
	    IN PIRP DpcIrp,
	    IN PVOID DpcContext);

static VOID STDCALL
ScsiPortIoTimer(PDEVICE_OBJECT DeviceObject,
		PVOID Context);

static PSCSI_REQUEST_BLOCK
ScsiPortInitSenseRequestSrb(PSCSI_REQUEST_BLOCK OriginalSrb);

static NTSTATUS
SpiBuildDeviceMap (PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		   PUNICODE_STRING RegistryPath);

static VOID
SpiAllocateSrbExtension(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
			PSCSI_REQUEST_BLOCK Srb);

static VOID
SpiFreeSrbExtension(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		    PSCSI_REQUEST_BLOCK Srb);


/* FUNCTIONS *****************************************************************/

/**********************************************************************
 * NAME							EXPORTED
 *	DriverEntry
 *
 * DESCRIPTION
 *	This function initializes the driver.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DriverObject
 *		System allocated Driver Object for this driver.
 *
 *	RegistryPath
 *		Name of registry driver service key.
 *
 * RETURN VALUE
 * 	Status.
 */

NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject,
	    IN PUNICODE_STRING RegistryPath)
{
  DPRINT("ScsiPort Driver %s\n", VERSION);
  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							EXPORTED
 *	ScsiDebugPrint
 *
 * DESCRIPTION
 *	Prints debugging messages.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DebugPrintLevel
 *		Debug level of the given message.
 *
 *	DebugMessage
 *		Pointer to printf()-compatible format string.
 *
 *	...
  		Additional output data (see printf()).
 *
 * RETURN VALUE
 * 	None.
 *
 * @implemented
 */

VOID
ScsiDebugPrint(IN ULONG DebugPrintLevel,
	       IN PCHAR DebugMessage,
	       ...)
{
  char Buffer[256];
  va_list ap;

#if 0
  if (DebugPrintLevel > InternalDebugLevel)
    return;
#endif

  va_start(ap, DebugMessage);
  vsprintf(Buffer, DebugMessage, ap);
  va_end(ap);

  DbgPrint(Buffer);
}


/*
 * @unimplemented
 */
VOID STDCALL
ScsiPortCompleteRequest(IN PVOID HwDeviceExtension,
			IN UCHAR PathId,
			IN UCHAR TargetId,
			IN UCHAR Lun,
			IN UCHAR SrbStatus)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  PLIST_ENTRY Entry;
  PIRP Irp;
  PSCSI_REQUEST_BLOCK Srb;

  DPRINT("ScsiPortCompleteRequest(HwDeviceExtension %x, PathId %d, TargetId %d, Lun %d, SrbStatus %x)\n",
         HwDeviceExtension, PathId, TargetId, Lun, SrbStatus);

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  Entry = DeviceExtension->LunExtensionListHead.Flink;
  while (Entry != &DeviceExtension->LunExtensionListHead)
    {
      LunExtension = CONTAINING_RECORD(Entry,
				       SCSI_PORT_LUN_EXTENSION,
				       List);



      if (PathId == (UCHAR)SP_UNTAGGED || 
	  (PathId == LunExtension->PathId && TargetId == (UCHAR)SP_UNTAGGED) ||
          (PathId == LunExtension->PathId && TargetId == LunExtension->TargetId && Lun == (UCHAR)SP_UNTAGGED) ||
          (PathId == LunExtension->PathId && TargetId == LunExtension->TargetId && Lun == LunExtension->Lun))
        {
	  Irp = LunExtension->NextIrp;
	  while (Irp)
	    {
	      Srb = (PSCSI_REQUEST_BLOCK)Irp->Tail.Overlay.DriverContext[3];
	      if (Srb->SrbFlags & SRB_FLAGS_IS_ACTIVE)
	        {
		  Srb->SrbStatus = SrbStatus;
		  ScsiPortNotification(RequestComplete,
		                       HwDeviceExtension,
				       Srb);
		}
              Irp = Irp->Tail.Overlay.DriverContext[1];
	    }
	}
      Entry = Entry->Flink;
    }
}


/*
 * @implemented
 */
ULONG STDCALL
ScsiPortConvertPhysicalAddressToUlong(IN SCSI_PHYSICAL_ADDRESS Address)
{
  DPRINT("ScsiPortConvertPhysicalAddressToUlong()\n");
  return(Address.u.LowPart);
}


/*
 * @unimplemented
 */
VOID STDCALL
ScsiPortFlushDma(IN PVOID HwDeviceExtension)
{
  DPRINT("ScsiPortFlushDma()\n");
  UNIMPLEMENTED;
}


/*
 * @implemented
 */
VOID STDCALL
ScsiPortFreeDeviceBase(IN PVOID HwDeviceExtension,
		       IN PVOID MappedAddress)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PSCSI_PORT_DEVICE_BASE DeviceBase;
  PLIST_ENTRY Entry;

  DPRINT("ScsiPortFreeDeviceBase() called\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);
  if (IsListEmpty(&DeviceExtension->DeviceBaseListHead))
    return;

  Entry = DeviceExtension->DeviceBaseListHead.Flink;
  while (Entry != &DeviceExtension->DeviceBaseListHead)
    {
      DeviceBase = CONTAINING_RECORD(Entry,
				     SCSI_PORT_DEVICE_BASE,
				     List);
      if (DeviceBase->MappedAddress == MappedAddress)
	{
	  MmUnmapIoSpace(DeviceBase->MappedAddress,
			 DeviceBase->NumberOfBytes);
	  RemoveEntryList(Entry);
	  ExFreePool(DeviceBase);

	  return;
	}

      Entry = Entry->Flink;
    }
}


/*
 * @implemented
 */
ULONG STDCALL
ScsiPortGetBusData(IN PVOID DeviceExtension,
		   IN ULONG BusDataType,
		   IN ULONG SystemIoBusNumber,
		   IN ULONG SlotNumber,
		   IN PVOID Buffer,
		   IN ULONG Length)
{
  return(HalGetBusData(BusDataType,
		       SystemIoBusNumber,
		       SlotNumber,
		       Buffer,
		       Length));
}


/*
 * @implemented
 */
PVOID STDCALL
ScsiPortGetDeviceBase(IN PVOID HwDeviceExtension,
		      IN INTERFACE_TYPE BusType,
		      IN ULONG SystemIoBusNumber,
		      IN SCSI_PHYSICAL_ADDRESS IoAddress,
		      IN ULONG NumberOfBytes,
		      IN BOOLEAN InIoSpace)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PHYSICAL_ADDRESS TranslatedAddress;
  PSCSI_PORT_DEVICE_BASE DeviceBase;
  ULONG AddressSpace;
  PVOID MappedAddress;

  DPRINT ("ScsiPortGetDeviceBase() called\n");

  AddressSpace = (ULONG)InIoSpace;
  if (HalTranslateBusAddress(BusType,
			     SystemIoBusNumber,
			     IoAddress,
			     &AddressSpace,
			     &TranslatedAddress) == FALSE)
    return NULL;

  /* i/o space */
  if (AddressSpace != 0)
    return((PVOID)TranslatedAddress.u.LowPart);

  MappedAddress = MmMapIoSpace(TranslatedAddress,
			       NumberOfBytes,
			       MmNonCached);

  DeviceBase = ExAllocatePool(NonPagedPool,
			      sizeof(SCSI_PORT_DEVICE_BASE));
  if (DeviceBase == NULL)
    return(MappedAddress);

  DeviceBase->MappedAddress = MappedAddress;
  DeviceBase->NumberOfBytes = NumberOfBytes;
  DeviceBase->IoAddress = IoAddress;
  DeviceBase->SystemIoBusNumber = SystemIoBusNumber;

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  InsertHeadList(&DeviceExtension->DeviceBaseListHead,
		 &DeviceBase->List);

  return(MappedAddress);
}


/*
 * @implemented
 */
PVOID STDCALL
ScsiPortGetLogicalUnit(IN PVOID HwDeviceExtension,
		       IN UCHAR PathId,
		       IN UCHAR TargetId,
		       IN UCHAR Lun)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  PLIST_ENTRY Entry;

  DPRINT("ScsiPortGetLogicalUnit() called\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);
  if (IsListEmpty(&DeviceExtension->LunExtensionListHead))
    return NULL;

  Entry = DeviceExtension->LunExtensionListHead.Flink;
  while (Entry != &DeviceExtension->LunExtensionListHead)
    {
      LunExtension = CONTAINING_RECORD(Entry,
				       SCSI_PORT_LUN_EXTENSION,
				       List);
      if (LunExtension->PathId == PathId &&
	  LunExtension->TargetId == TargetId &&
	  LunExtension->Lun == Lun)
	{
	  return (PVOID)&LunExtension->MiniportLunExtension;
	}

      Entry = Entry->Flink;
    }

  return NULL;
}


/*
 * @unimplemented
 */
SCSI_PHYSICAL_ADDRESS STDCALL
ScsiPortGetPhysicalAddress(IN PVOID HwDeviceExtension,
			   IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
			   IN PVOID VirtualAddress,
			   OUT ULONG *Length)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  SCSI_PHYSICAL_ADDRESS PhysicalAddress;
  SCSI_PHYSICAL_ADDRESS NextPhysicalAddress;
  ULONG BufferLength = 0;
  ULONG Offset;
  PVOID EndAddress;

  DPRINT("ScsiPortGetPhysicalAddress(%p %p %p %p)\n",
	 HwDeviceExtension, Srb, VirtualAddress, Length);

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  if (Length != NULL)
    {
      *Length = 0;
    }
  if (Srb == NULL)
    {
      EndAddress = DeviceExtension->VirtualAddress + DeviceExtension->CommonBufferLength;
      if (VirtualAddress >= DeviceExtension->VirtualAddress && VirtualAddress < EndAddress)
        {
	  Offset = (ULONG_PTR)VirtualAddress - (ULONG_PTR)DeviceExtension->VirtualAddress;
	  PhysicalAddress.QuadPart = DeviceExtension->PhysicalAddress.QuadPart + Offset;
          BufferLength = (ULONG_PTR)EndAddress - (ULONG_PTR)VirtualAddress;
	}
      else
        {
	  /*
	   * The given virtual address is not within the range 
	   * of the drivers uncached extension or srb extension.
	   */
	  /*
	   * FIXME: 
	   *   Check if the address is a sense info buffer of an active srb.
	   */
	  PhysicalAddress = MmGetPhysicalAddress(VirtualAddress);
	  if (PhysicalAddress.QuadPart == 0LL)
	    {
	      CHECKPOINT;
	      return PhysicalAddress;
	    }
	  BufferLength = PAGE_SIZE - PhysicalAddress.u.LowPart % PAGE_SIZE;
	}
    }
  else
    {
      EndAddress = Srb->DataBuffer + Srb->DataTransferLength;
      if (VirtualAddress == NULL)
	{
	  VirtualAddress = Srb->DataBuffer;
	}
      else if (VirtualAddress < Srb->DataBuffer || VirtualAddress >= EndAddress)
	{
	  EndAddress = Srb->SenseInfoBuffer + Srb->SenseInfoBufferLength;
	  if (VirtualAddress < Srb->SenseInfoBuffer || VirtualAddress >= EndAddress)
	    {
	      PhysicalAddress.QuadPart = 0LL;
	      CHECKPOINT;
	      return PhysicalAddress;
	    }
	}

      PhysicalAddress = MmGetPhysicalAddress(VirtualAddress);
      if (PhysicalAddress.QuadPart == 0LL)
	{
	  CHECKPOINT;
	  return PhysicalAddress;
	}

      BufferLength = PAGE_SIZE - (ULONG_PTR)VirtualAddress % PAGE_SIZE;
      while (VirtualAddress + BufferLength < EndAddress)
	{
	  NextPhysicalAddress = MmGetPhysicalAddress(VirtualAddress + BufferLength);
	  if (PhysicalAddress.QuadPart + BufferLength != NextPhysicalAddress.QuadPart)
	    {
	      break;
	    }
	  BufferLength += PAGE_SIZE;
	}
      if (VirtualAddress + BufferLength >= EndAddress)
	{
	  BufferLength = EndAddress - VirtualAddress;
	}
    }
  if (Length != NULL)
    {
      *Length = BufferLength;
    }
  DPRINT("Address %I64x, Length %d\n", PhysicalAddress.QuadPart, BufferLength);
  return PhysicalAddress;
}


/*
 * @unimplemented
 */
PSCSI_REQUEST_BLOCK STDCALL
ScsiPortGetSrb(IN PVOID HwDeviceExtension,
	       IN UCHAR PathId,
	       IN UCHAR TargetId,
	       IN UCHAR Lun,
	       IN LONG QueueTag)
{
  DPRINT1("ScsiPortGetSrb() unimplemented\n");
  UNIMPLEMENTED;
  return NULL;
}


/*
 * @implemented
 */
PVOID STDCALL
ScsiPortGetUncachedExtension(IN PVOID HwDeviceExtension,
			     IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			     IN ULONG NumberOfBytes)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  DEVICE_DESCRIPTION DeviceDescription;

  DPRINT("ScsiPortGetUncachedExtension(%p %p %lu)\n",
	 HwDeviceExtension, ConfigInfo, NumberOfBytes);

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  /* Check for allocated common DMA buffer */
  if (DeviceExtension->VirtualAddress != NULL)
    {
      DPRINT1("The HBA has already got a common DMA buffer!\n");
      return NULL;
    }

  /* Check for DMA adapter object */
  if (DeviceExtension->AdapterObject == NULL)
    {
      /* Initialize DMA adapter description */
      RtlZeroMemory(&DeviceDescription,
		    sizeof(DEVICE_DESCRIPTION));
      DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION;
      DeviceDescription.Master = ConfigInfo->Master;
      DeviceDescription.ScatterGather = ConfigInfo->ScatterGather;
      DeviceDescription.DemandMode = ConfigInfo->DemandMode;
      DeviceDescription.Dma32BitAddresses = ConfigInfo->Dma32BitAddresses;
      DeviceDescription.BusNumber = ConfigInfo->SystemIoBusNumber;
      DeviceDescription.DmaChannel = ConfigInfo->DmaChannel;
      DeviceDescription.InterfaceType = ConfigInfo->AdapterInterfaceType;
      DeviceDescription.DmaWidth = ConfigInfo->DmaWidth;
      DeviceDescription.DmaSpeed = ConfigInfo->DmaSpeed;
      DeviceDescription.MaximumLength = ConfigInfo->MaximumTransferLength;
      DeviceDescription.DmaPort = ConfigInfo->DmaPort;

      /* Get a DMA adapter object */
      DeviceExtension->AdapterObject = HalGetAdapter(&DeviceDescription,
						     &DeviceExtension->MapRegisterCount);
      if (DeviceExtension->AdapterObject == NULL)
	{
	  DPRINT1("HalGetAdapter() failed\n");
	  return NULL;
	}
    }
  if (DeviceExtension->SrbExtensionSize > 0)
    {
      PVOID Buffer;
      DeviceExtension->CurrentSrbExtensions = 0;
      if (DeviceExtension->PortConfig->MultipleRequestPerLu)
        {
          DeviceExtension->MaxSrbExtensions = 1024;
	}
      else
        {
	  DeviceExtension->MaxSrbExtensions = 32;
	}
      Buffer = ExAllocatePool(NonPagedPool, ROUND_UP(DeviceExtension->MaxSrbExtensions / 8, sizeof(ULONG)));
      if (Buffer == NULL)
        {
          KEBUGCHECK(0);
          return NULL;
        }
      RtlInitializeBitMap(&DeviceExtension->SrbExtensionAllocMap, Buffer, DeviceExtension->MaxSrbExtensions);
      RtlClearAllBits(&DeviceExtension->SrbExtensionAllocMap);
    }

  /* Allocate a common DMA buffer */
  DeviceExtension->CommonBufferLength =
    NumberOfBytes + PAGE_ROUND_UP(DeviceExtension->SrbExtensionSize * DeviceExtension->MaxSrbExtensions);
  DeviceExtension->VirtualAddress =
    HalAllocateCommonBuffer(DeviceExtension->AdapterObject,
			    DeviceExtension->CommonBufferLength,
			    &DeviceExtension->PhysicalAddress,
			    FALSE);
  if (DeviceExtension->VirtualAddress == NULL)
    {
      DPRINT1("HalAllocateCommonBuffer() failed!\n");
      DeviceExtension->CommonBufferLength = 0;
      return NULL;
    }

  return (PVOID)((ULONG_PTR)DeviceExtension->VirtualAddress +
                 PAGE_ROUND_UP(DeviceExtension->SrbExtensionSize * DeviceExtension->MaxSrbExtensions));
}


/*
 * @implemented
 */
PVOID STDCALL
ScsiPortGetVirtualAddress(IN PVOID HwDeviceExtension,
			  IN SCSI_PHYSICAL_ADDRESS PhysicalAddress)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  ULONG Offset;

  DPRINT("ScsiPortGetVirtualAddress(%p %I64x)\n",
	 HwDeviceExtension, PhysicalAddress.QuadPart);

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  if (DeviceExtension->PhysicalAddress.QuadPart > PhysicalAddress.QuadPart)
    return NULL;

  Offset = (ULONG)(PhysicalAddress.QuadPart - DeviceExtension->PhysicalAddress.QuadPart);
  if (Offset >= DeviceExtension->CommonBufferLength)
    return NULL;

  return (PVOID)((ULONG_PTR)DeviceExtension->VirtualAddress + Offset);
}


/**********************************************************************
 * NAME							EXPORTED
 *	ScsiPortInitialize
 *
 * DESCRIPTION
 *	Initializes SCSI port driver specific data.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Argument1
 *		Pointer to the miniport driver's driver object.
 *
 *	Argument2
 *		Pointer to the miniport driver's registry path.
 *
 *	HwInitializationData
 *		Pointer to port driver specific configuration data.
 *
 *	HwContext
  		Miniport driver specific context.
 *
 * RETURN VALUE
 * 	Status.
 *
 * @implemented
 */

ULONG STDCALL
ScsiPortInitialize(IN PVOID Argument1,
		   IN PVOID Argument2,
		   IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
		   IN PVOID HwContext)
{
  PDRIVER_OBJECT DriverObject = (PDRIVER_OBJECT)Argument1;
//  PUNICODE_STRING RegistryPath = (PUNICODE_STRING)Argument2;
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PCONFIGURATION_INFORMATION SystemConfig;
  PPORT_CONFIGURATION_INFORMATION PortConfig;
  ULONG DeviceExtensionSize;
  ULONG PortConfigSize;
  BOOLEAN Again;
  BOOLEAN DeviceFound = FALSE;
  ULONG i;
  ULONG Result;
  NTSTATUS Status;
  ULONG MaxBus;
  ULONG BusNumber;
  PCI_SLOT_NUMBER SlotNumber;

  PDEVICE_OBJECT PortDeviceObject;
  WCHAR NameBuffer[80];
  UNICODE_STRING DeviceName;
  WCHAR DosNameBuffer[80];
  UNICODE_STRING DosDeviceName;
  PIO_SCSI_CAPABILITIES PortCapabilities;
  ULONG MappedIrq;
  KIRQL Dirql;
  KAFFINITY Affinity;


  DPRINT ("ScsiPortInitialize() called!\n");
#if 0
  DPRINT1("HwInitializationDataSize: %d\n", HwInitializationData->HwInitializationDataSize);
  DPRINT1("AdapterInterfaceType:     %d\n", HwInitializationData->AdapterInterfaceType);
  DPRINT1("HwInitialize:             %x\n", HwInitializationData->HwInitialize);
  DPRINT1("HwStartIo:                %x\n", HwInitializationData->HwStartIo);
  DPRINT1("HwInterrupt:              %x\n", HwInitializationData->HwInterrupt);
  DPRINT1("HwFindAdapter:            %x\n", HwInitializationData->HwFindAdapter);
  DPRINT1("HwResetBus:               %x\n", HwInitializationData->HwResetBus);
  DPRINT1("HwDmaStarted:             %x\n", HwInitializationData->HwDmaStarted);
  DPRINT1("HwAdapterState:           %x\n", HwInitializationData->HwAdapterState);
  DPRINT1("DeviceExtensionSize:      %d\n", HwInitializationData->DeviceExtensionSize);
  DPRINT1("SpecificLuExtensionSize:  %d\n", HwInitializationData->SpecificLuExtensionSize);
  DPRINT1("SrbExtensionSize:         %d\n", HwInitializationData->SrbExtensionSize);
  DPRINT1("NumberOfAccessRanges:     %d\n", HwInitializationData->NumberOfAccessRanges);
  DPRINT1("Reserved:                 %x\n", HwInitializationData->Reserved);
  DPRINT1("MapBuffers:               %d\n", HwInitializationData->MapBuffers);
  DPRINT1("NeedPhysicalAddresses:    %d\n", HwInitializationData->NeedPhysicalAddresses);
  DPRINT1("TaggedQueueing:           %d\n", HwInitializationData->TaggedQueueing);
  DPRINT1("AutoRequestSense:         %d\n", HwInitializationData->AutoRequestSense);
  DPRINT1("MultipleRequestPerLu:     %d\n", HwInitializationData->MultipleRequestPerLu);
  DPRINT1("ReceiveEvent:             %d\n", HwInitializationData->ReceiveEvent);
  DPRINT1("VendorIdLength:           %d\n", HwInitializationData->VendorIdLength);
  DPRINT1("VendorId:                 %x\n", HwInitializationData->VendorId);
  DPRINT1("ReservedUshort:           %d\n", HwInitializationData->ReservedUshort);
  DPRINT1("DeviceIdLength:           %d\n", HwInitializationData->DeviceIdLength);
  DPRINT1("DeviceId:                 %x\n", HwInitializationData->DeviceId);
#endif
  if ((HwInitializationData->HwInitialize == NULL) ||
      (HwInitializationData->HwStartIo == NULL) ||
      (HwInitializationData->HwInterrupt == NULL) ||
      (HwInitializationData->HwFindAdapter == NULL) ||
      (HwInitializationData->HwResetBus == NULL))
    return(STATUS_INVALID_PARAMETER);

  DriverObject->DriverStartIo = NULL;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = ScsiPortCreateClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = ScsiPortCreateClose;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = ScsiPortDeviceControl;
  DriverObject->MajorFunction[IRP_MJ_SCSI] = ScsiPortDispatchScsi;

  SystemConfig = IoGetConfigurationInformation();

  DeviceExtensionSize = sizeof(SCSI_PORT_DEVICE_EXTENSION) +
    HwInitializationData->DeviceExtensionSize;
  PortConfigSize = sizeof(PORT_CONFIGURATION_INFORMATION) + 
    HwInitializationData->NumberOfAccessRanges * sizeof(ACCESS_RANGE);


  MaxBus = (HwInitializationData->AdapterInterfaceType == PCIBus) ? 8 : 1;
  DPRINT("MaxBus: %lu\n", MaxBus);

  PortDeviceObject = NULL;
  BusNumber = 0;
  SlotNumber.u.AsULONG = 0;
  while (TRUE)
    {
      /* Create a unicode device name */
      swprintf (NameBuffer,
		L"\\Device\\ScsiPort%lu",
		SystemConfig->ScsiPortCount);
      RtlInitUnicodeString (&DeviceName,
			    NameBuffer);

      DPRINT("Creating device: %wZ\n", &DeviceName);

      /* Create the port device */
      Status = IoCreateDevice (DriverObject,
			       DeviceExtensionSize,
			       &DeviceName,
			       FILE_DEVICE_CONTROLLER,
			       0,
			       FALSE,
			       &PortDeviceObject);
      if (!NT_SUCCESS(Status))
	{
	  DbgPrint ("IoCreateDevice call failed! (Status 0x%lX)\n", Status);
	  PortDeviceObject = NULL;
	  goto ByeBye;
	}

      DPRINT ("Created device: %wZ (%p)\n", &DeviceName, PortDeviceObject);

      /* Set the buffering strategy here... */
      PortDeviceObject->Flags |= DO_DIRECT_IO;
      PortDeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;

      DeviceExtension = PortDeviceObject->DeviceExtension;
      RtlZeroMemory(DeviceExtension, DeviceExtensionSize);
      DeviceExtension->Length = DeviceExtensionSize;
      DeviceExtension->DeviceObject = PortDeviceObject;
      DeviceExtension->PortNumber = SystemConfig->ScsiPortCount;

      DeviceExtension->MiniPortExtensionSize = HwInitializationData->DeviceExtensionSize;
      DeviceExtension->LunExtensionSize = HwInitializationData->SpecificLuExtensionSize;
      DeviceExtension->SrbExtensionSize = HwInitializationData->SrbExtensionSize;
      DeviceExtension->HwStartIo = HwInitializationData->HwStartIo;
      DeviceExtension->HwInterrupt = HwInitializationData->HwInterrupt;

      DeviceExtension->AdapterObject = NULL;
      DeviceExtension->MapRegisterCount = 0;
      DeviceExtension->PhysicalAddress.QuadPart = 0ULL;
      DeviceExtension->VirtualAddress = NULL;
      DeviceExtension->CommonBufferLength = 0;

      /* Initialize the device base list */
      InitializeListHead (&DeviceExtension->DeviceBaseListHead);

      /* Initialize the irp lists */
      InitializeListHead (&DeviceExtension->PendingIrpListHead);
      DeviceExtension->NextIrp = NULL;
      DeviceExtension->PendingIrpCount = 0;
      DeviceExtension->ActiveIrpCount = 0;

      /* Initialize LUN-Extension list */
      InitializeListHead (&DeviceExtension->LunExtensionListHead);

      /* Initialize the spin lock in the controller extension */
      KeInitializeSpinLock (&DeviceExtension->Lock);

      /* Initialize the DPC object */
      IoInitializeDpcRequest (PortDeviceObject,
			      ScsiPortDpc);

      /* Initialize the device timer */
      DeviceExtension->TimerState = IDETimerIdle;
      DeviceExtension->TimerCount = 0;
      IoInitializeTimer (PortDeviceObject,
			 ScsiPortIoTimer,
			 DeviceExtension);

      /* Allocate and initialize port configuration info */
      DeviceExtension->PortConfig = ExAllocatePool (NonPagedPool,
						    PortConfigSize);
      if (DeviceExtension->PortConfig == NULL)
	{
	  Status = STATUS_INSUFFICIENT_RESOURCES;
	  goto ByeBye;
	}
      RtlZeroMemory (DeviceExtension->PortConfig,
		     PortConfigSize);

      PortConfig = DeviceExtension->PortConfig;
      PortConfig->Length = sizeof(PORT_CONFIGURATION_INFORMATION);
      PortConfig->SystemIoBusNumber = BusNumber;
      PortConfig->AdapterInterfaceType = HwInitializationData->AdapterInterfaceType;
      PortConfig->InterruptMode =
	(PortConfig->AdapterInterfaceType == PCIBus) ? LevelSensitive : Latched;
      PortConfig->MaximumTransferLength = SP_UNINITIALIZED_VALUE;
      PortConfig->NumberOfPhysicalBreaks = SP_UNINITIALIZED_VALUE;
      PortConfig->DmaChannel = SP_UNINITIALIZED_VALUE;
      PortConfig->DmaPort = SP_UNINITIALIZED_VALUE;
      PortConfig->DmaWidth = 0;
      PortConfig->DmaSpeed = Compatible;
      PortConfig->AlignmentMask = 0;
      PortConfig->NumberOfAccessRanges = HwInitializationData->NumberOfAccessRanges;
      PortConfig->NumberOfBuses = 0;

      for (i = 0; i < SCSI_MAXIMUM_BUSES; i++)
	PortConfig->InitiatorBusId[i] = 255;

      PortConfig->ScatterGather = FALSE;
      PortConfig->Master = FALSE;
      PortConfig->CachesData = FALSE;
      PortConfig->AdapterScansDown = FALSE;
      PortConfig->AtdiskPrimaryClaimed = SystemConfig->AtDiskPrimaryAddressClaimed;
      PortConfig->AtdiskSecondaryClaimed = SystemConfig->AtDiskSecondaryAddressClaimed;
      PortConfig->Dma32BitAddresses = FALSE;
      PortConfig->DemandMode = FALSE;
      PortConfig->MapBuffers = HwInitializationData->MapBuffers;
      PortConfig->NeedPhysicalAddresses = HwInitializationData->NeedPhysicalAddresses;
      PortConfig->TaggedQueuing = HwInitializationData->TaggedQueueing;
      PortConfig->AutoRequestSense = HwInitializationData->AutoRequestSense;
      PortConfig->MultipleRequestPerLu = HwInitializationData->MultipleRequestPerLu;
      PortConfig->ReceiveEvent = HwInitializationData->ReceiveEvent;
      PortConfig->RealModeInitialized = FALSE;
      PortConfig->BufferAccessScsiPortControlled = FALSE;
      PortConfig->MaximumNumberOfTargets = SCSI_MAXIMUM_TARGETS;
//      PortConfig->MaximumNumberOfLogicalUnits = SCSI_MAXIMUM_LOGICAL_UNITS;

      PortConfig->SrbExtensionSize = HwInitializationData->SrbExtensionSize;
      PortConfig->SpecificLuExtensionSize = HwInitializationData->SpecificLuExtensionSize;

      PortConfig->AccessRanges = (PACCESS_RANGE)(PortConfig + 1);

      /* Search for matching PCI device */
      if ((HwInitializationData->AdapterInterfaceType == PCIBus) &&
	  (HwInitializationData->VendorIdLength > 0) &&
	  (HwInitializationData->VendorId != NULL) &&
	  (HwInitializationData->DeviceIdLength > 0) &&
	  (HwInitializationData->DeviceId != NULL))
	{
	  /* Get PCI device data */
	  DPRINT("VendorId '%.*s'  DeviceId '%.*s'\n",
		 HwInitializationData->VendorIdLength,
		 HwInitializationData->VendorId,
		 HwInitializationData->DeviceIdLength,
		 HwInitializationData->DeviceId);

	  if (!SpiGetPciConfigData (HwInitializationData,
				    PortConfig,
				    BusNumber,
				    &SlotNumber))
	    {
	      Status = STATUS_UNSUCCESSFUL;
	      goto ByeBye;
	    }
	}

      /* Note: HwFindAdapter is called once for each bus */
      Again = FALSE;
      DPRINT("Calling HwFindAdapter() for Bus %lu\n", PortConfig->SystemIoBusNumber);
      Result = (HwInitializationData->HwFindAdapter)(&DeviceExtension->MiniPortDeviceExtension,
						     HwContext,
						     0,  /* BusInformation */
						     "", /* ArgumentString */
						     PortConfig,
						     &Again);
      DPRINT("HwFindAdapter() Result: %lu  Again: %s\n",
	     Result, (Again) ? "True" : "False");

      if (Result == SP_RETURN_FOUND)
	{
	  DPRINT("ScsiPortInitialize(): Found HBA! (%x)\n", PortConfig->BusInterruptVector);

#if 0
	  DPRINT1("SystemIoBusNumber:               %x\n", PortConfig->SystemIoBusNumber);
	  DPRINT1("AdapterInterfaceType:            %x\n", PortConfig->AdapterInterfaceType);
	  DPRINT1("BusInterruptLevel:               %x\n", PortConfig->BusInterruptLevel);
	  DPRINT1("BusInterruptVector:              %x\n", PortConfig->BusInterruptVector);
	  DPRINT1("InterruptMode:                   %x\n", PortConfig->InterruptMode);
	  DPRINT1("MaximumTransferLength:           %x\n", PortConfig->MaximumTransferLength);
	  DPRINT1("NumberOfPhysicalBreaks:          %x\n", PortConfig->NumberOfPhysicalBreaks);
	  DPRINT1("DmaChannel:                      %x\n", PortConfig->DmaChannel);
	  DPRINT1("DmaPort:                         %d\n", PortConfig->DmaPort);
	  DPRINT1("DmaWidth:                        %d\n", PortConfig->DmaWidth);
	  DPRINT1("DmaSpeed:                        %d\n", PortConfig->DmaSpeed);
	  DPRINT1("AlignmentMask:                   %d\n", PortConfig->AlignmentMask);
	  DPRINT1("NumberOfAccessRanges:            %d\n", PortConfig->NumberOfAccessRanges);
	  DPRINT1("NumberOfBuses:                   %d\n", PortConfig->NumberOfBuses);
	  DPRINT1("ScatterGather:                   %d\n", PortConfig->ScatterGather);
	  DPRINT1("Master:                          %d\n", PortConfig->Master);
	  DPRINT1("CachesData:                      %d\n", PortConfig->CachesData);
	  DPRINT1("AdapterScansDown:                %d\n", PortConfig->AdapterScansDown);
	  DPRINT1("AtdiskPrimaryClaimed:            %d\n", PortConfig->AtdiskPrimaryClaimed);
	  DPRINT1("AtdiskSecondaryClaimed:          %d\n", PortConfig->AtdiskSecondaryClaimed);
	  DPRINT1("Dma32BitAddresses:               %d\n", PortConfig->Dma32BitAddresses);
	  DPRINT1("DemandMode:                      %d\n", PortConfig->DemandMode);
	  DPRINT1("MapBuffers:                      %d\n", PortConfig->MapBuffers);
	  DPRINT1("NeedPhysicalAddresses:           %d\n", PortConfig->NeedPhysicalAddresses);
	  DPRINT1("TaggedQueuing:                   %d\n", PortConfig->TaggedQueuing);
	  DPRINT1("AutoRequestSense:                %d\n", PortConfig->AutoRequestSense);
	  DPRINT1("MultipleRequestPerLu:            %d\n", PortConfig->MultipleRequestPerLu);
	  DPRINT1("ReceiveEvent:                    %d\n", PortConfig->ReceiveEvent);
	  DPRINT1("RealModeInitialized:             %d\n", PortConfig->RealModeInitialized);
	  DPRINT1("BufferAccessScsiPortControlled:  %d\n", PortConfig->BufferAccessScsiPortControlled);
	  DPRINT1("MaximumNumberOfTargets:          %d\n", PortConfig->MaximumNumberOfTargets);
	  DPRINT1("SlotNumber:                      %d\n", PortConfig->SlotNumber);
	  DPRINT1("BusInterruptLevel2:              %x\n", PortConfig->BusInterruptLevel2);
	  DPRINT1("BusInterruptVector2:             %x\n", PortConfig->BusInterruptVector2);
	  DPRINT1("InterruptMode2:                  %x\n", PortConfig->InterruptMode2);
	  DPRINT1("DmaChannel2:                     %d\n", PortConfig->DmaChannel2);
	  DPRINT1("DmaPort2:                        %d\n", PortConfig->DmaPort2);
	  DPRINT1("DmaWidth2:                       %d\n", PortConfig->DmaWidth2);
	  DPRINT1("DmaSpeed2:                       %d\n", PortConfig->DmaSpeed2);
	  DPRINT1("DeviceExtensionSize:             %d\n", PortConfig->DeviceExtensionSize);
	  DPRINT1("SpecificLuExtensionSize:         %d\n", PortConfig->SpecificLuExtensionSize);
	  DPRINT1("SrbExtensionSize:                %d\n", PortConfig->SrbExtensionSize);

#endif

          if (DeviceExtension->VirtualAddress == NULL && DeviceExtension->SrbExtensionSize)
	    {
	      ScsiPortGetUncachedExtension(&DeviceExtension->MiniPortDeviceExtension,
		                           PortConfig,
					   0);
	    }
	      
	  /* Register an interrupt handler for this device */
	  MappedIrq = HalGetInterruptVector(PortConfig->AdapterInterfaceType,
					    PortConfig->SystemIoBusNumber,
					    PortConfig->BusInterruptLevel,
#if 1
/* 
 * FIXME:
 *   Something is wrong in our interrupt conecting code. 
 *   The promise Ultra100TX driver returns 0 for BusInterruptVector
 *   and a nonzero value for BusInterruptLevel. The driver does only 
 *   work with this fix.
 */
					    PortConfig->BusInterruptLevel,
#else
					    PortConfig->BusInterruptVector,
#endif
					    &Dirql,
					    &Affinity);
	  DPRINT("AdapterInterfaceType %x, SystemIoBusNumber %x, BusInterruptLevel %x, BusInterruptVector %x\n",
	          PortConfig->AdapterInterfaceType, PortConfig->SystemIoBusNumber, 
		  PortConfig->BusInterruptLevel, PortConfig->BusInterruptVector); 
	  Status = IoConnectInterrupt(&DeviceExtension->Interrupt,
				      ScsiPortIsr,
				      DeviceExtension,
				      NULL,
				      MappedIrq,
				      Dirql,
				      Dirql,
				      PortConfig->InterruptMode,
				      TRUE,
				      Affinity,
				      FALSE);
	  if (!NT_SUCCESS(Status))
	    {
	      DbgPrint("Could not connect interrupt %d\n",
		       PortConfig->BusInterruptVector);
	      goto ByeBye;
	    }

	  if (!(HwInitializationData->HwInitialize)(&DeviceExtension->MiniPortDeviceExtension))
	    {
	      DbgPrint("HwInitialize() failed!");
	      Status = STATUS_UNSUCCESSFUL;
	      goto ByeBye;
	    }

	  /* Initialize port capabilities */
	  DeviceExtension->PortCapabilities = ExAllocatePool(NonPagedPool,
							     sizeof(IO_SCSI_CAPABILITIES));
	  if (DeviceExtension->PortCapabilities == NULL)
	    {
	      DbgPrint("Failed to allocate port capabilities!\n");
	      Status = STATUS_INSUFFICIENT_RESOURCES;
	      goto ByeBye;
	    }

	  PortCapabilities = DeviceExtension->PortCapabilities;
	  PortCapabilities->Length = sizeof(IO_SCSI_CAPABILITIES);
          if (PortConfig->ScatterGather == FALSE ||
	      PortConfig->NumberOfPhysicalBreaks >= (0x100000000LL >> PAGE_SHIFT) ||
	      PortConfig->MaximumTransferLength < PortConfig->NumberOfPhysicalBreaks * PAGE_SIZE)
	    {
	      PortCapabilities->MaximumTransferLength =
	        PortConfig->MaximumTransferLength;
	    }
	  else
	    {
	      PortCapabilities->MaximumTransferLength =
	        PortConfig->NumberOfPhysicalBreaks * PAGE_SIZE;
	    }
      
	  PortCapabilities->MaximumPhysicalPages =
	    PortCapabilities->MaximumTransferLength / PAGE_SIZE;
	  PortCapabilities->SupportedAsynchronousEvents = 0; /* FIXME */
	  PortCapabilities->AlignmentMask =
	    PortConfig->AlignmentMask;
	  PortCapabilities->TaggedQueuing =
	    PortConfig->TaggedQueuing;
	  PortCapabilities->AdapterScansDown =
	    PortConfig->AdapterScansDown;
	  PortCapabilities->AdapterUsesPio = TRUE; /* FIXME */

	  /* Scan the adapter for devices */
	  SpiScanAdapter (DeviceExtension);

	  /* Build the registry device map */
	  SpiBuildDeviceMap (DeviceExtension,
			     (PUNICODE_STRING)Argument2);

	  /* Create the dos device link */
	  swprintf(DosNameBuffer,
		   L"\\??\\Scsi%lu:",
		   SystemConfig->ScsiPortCount);
	  RtlInitUnicodeString(&DosDeviceName,
			       DosNameBuffer);
	  IoCreateSymbolicLink(&DosDeviceName,
			       &DeviceName);

	  /* Update the system configuration info */
	  if (PortConfig->AtdiskPrimaryClaimed == TRUE)
	    SystemConfig->AtDiskPrimaryAddressClaimed = TRUE;
	  if (PortConfig->AtdiskSecondaryClaimed == TRUE)
	    SystemConfig->AtDiskSecondaryAddressClaimed = TRUE;

	  SystemConfig->ScsiPortCount++;
	  PortDeviceObject = NULL;
	  DeviceFound = TRUE;
	}
      else
	{
	  DPRINT("HwFindAdapter() Result: %lu\n", Result);

	  ExFreePool (PortConfig);
	  IoDeleteDevice (PortDeviceObject);
	  PortDeviceObject = NULL;
	}

      DPRINT("Bus: %lu  MaxBus: %lu\n", BusNumber, MaxBus);
      if (BusNumber >= MaxBus)
	{
	  DPRINT("Scanned all buses!\n");
	  Status = STATUS_SUCCESS;
	  goto ByeBye;
	}

      if (Again == FALSE)
	{
	  BusNumber++;
	  SlotNumber.u.AsULONG = 0;
	}
    }

ByeBye:
  /* Clean up the mess */
  if (PortDeviceObject != NULL)
    {
      DPRINT("Delete device: %p\n", PortDeviceObject);

      DeviceExtension = PortDeviceObject->DeviceExtension;

      if (DeviceExtension->PortCapabilities != NULL)
	{
	  IoDisconnectInterrupt (DeviceExtension->Interrupt);
	  ExFreePool (DeviceExtension->PortCapabilities);
	}

      if (DeviceExtension->PortConfig != NULL)
	{
	  ExFreePool (DeviceExtension->PortConfig);
	}

      IoDeleteDevice (PortDeviceObject);
    }

  DPRINT("ScsiPortInitialize() done!\n");

  return (DeviceFound == FALSE) ? Status : STATUS_SUCCESS;
}


/*
 * @unimplemented
 */
VOID STDCALL
ScsiPortIoMapTransfer(IN PVOID HwDeviceExtension,
		      IN PSCSI_REQUEST_BLOCK Srb,
		      IN ULONG LogicalAddress,
		      IN ULONG Length)
{
  DPRINT1("ScsiPortIoMapTransfer()\n");
  UNIMPLEMENTED;
}


/*
 * @unimplemented
 */
VOID STDCALL
ScsiPortLogError(IN PVOID HwDeviceExtension,
		 IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
		 IN UCHAR PathId,
		 IN UCHAR TargetId,
		 IN UCHAR Lun,
		 IN ULONG ErrorCode,
		 IN ULONG UniqueId)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT1("ScsiPortLogError() called\n");
  DPRINT1("Srb %x, PathId %d, TargetId %d, Lun %d, ErrorCode %x, UniqueId %x\n",
          Srb, PathId, TargetId, Lun, ErrorCode, UniqueId);

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);


  DPRINT("ScsiPortLogError() done\n");
}


/*
 * @implemented
 */
VOID STDCALL
ScsiPortMoveMemory(OUT PVOID Destination,
		   IN PVOID Source,
		   IN ULONG Length)
{
  RtlMoveMemory(Destination,
		Source,
		Length);
}


/*
 * @implemented
 */
VOID
ScsiPortNotification(IN SCSI_NOTIFICATION_TYPE NotificationType,
		     IN PVOID HwDeviceExtension,
		     ...)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  va_list ap;

  DPRINT("ScsiPortNotification() called\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  DPRINT("DeviceExtension %p\n", DeviceExtension);

  va_start(ap, HwDeviceExtension);

  switch (NotificationType)
    {
      case RequestComplete:
	{
	  PSCSI_REQUEST_BLOCK Srb;

	  Srb = (PSCSI_REQUEST_BLOCK) va_arg (ap, PSCSI_REQUEST_BLOCK);

	  DPRINT("Notify: RequestComplete (Srb %p)\n", Srb);
	  DeviceExtension->Flags |= IRP_FLAG_COMPLETE;
	  Srb->SrbFlags &= ~SRB_FLAGS_IS_ACTIVE;
	}
	break;

      case NextRequest:
	DPRINT("Notify: NextRequest\n");
	DeviceExtension->Flags |= IRP_FLAG_NEXT;
	break;

      case NextLuRequest:
	{
	  UCHAR PathId;
	  UCHAR TargetId;
	  UCHAR Lun;
          PSCSI_PORT_LUN_EXTENSION LunExtension;

	  PathId = (UCHAR) va_arg (ap, int);
	  TargetId = (UCHAR) va_arg (ap, int);
	  Lun = (UCHAR) va_arg (ap, int);

	  DPRINT ("Notify: NextLuRequest(PathId %u  TargetId %u  Lun %u)\n",
		   PathId, TargetId, Lun);

          LunExtension = SpiGetLunExtension(DeviceExtension,
				            PathId,
				            TargetId,
				            Lun);
	  if (LunExtension)
	    {
	      DeviceExtension->Flags |= IRP_FLAG_NEXT_LU;
	      LunExtension->Flags |= IRP_FLAG_NEXT_LU;
	    }
	}
	break;

      case ResetDetected:
	DPRINT1("Notify: ResetDetected\n");
	/* FIXME: ??? */
	break;

      default:
	DPRINT1 ("Unsupported notification %lu\n", NotificationType);
	break;
    }
  if (KeGetCurrentIrql() >= DISPATCH_LEVEL)
    {
      IoRequestDpc(DeviceExtension->DeviceObject,
                   NULL,
		   DeviceExtension);
    }
  else
    {
      SpiProcessRequests(DeviceExtension, NULL);
    }

  va_end(ap);
}


/*
 * @implemented
 */
ULONG STDCALL
ScsiPortSetBusDataByOffset(IN PVOID DeviceExtension,
			   IN ULONG BusDataType,
			   IN ULONG SystemIoBusNumber,
			   IN ULONG SlotNumber,
			   IN PVOID Buffer,
			   IN ULONG Offset,
			   IN ULONG Length)
{
  DPRINT("ScsiPortSetBusDataByOffset()\n");
  return(HalSetBusDataByOffset(BusDataType,
			       SystemIoBusNumber,
			       SlotNumber,
			       Buffer,
			       Offset,
			       Length));
}


/*
 * @implemented
 */
BOOLEAN STDCALL
ScsiPortValidateRange(IN PVOID HwDeviceExtension,
		      IN INTERFACE_TYPE BusType,
		      IN ULONG SystemIoBusNumber,
		      IN SCSI_PHYSICAL_ADDRESS IoAddress,
		      IN ULONG NumberOfBytes,
		      IN BOOLEAN InIoSpace)
{
  DPRINT("ScsiPortValidateRange()\n");
  return(TRUE);
}


/* INTERNAL FUNCTIONS ********************************************************/


static BOOLEAN
SpiGetPciConfigData (IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
		     IN OUT PPORT_CONFIGURATION_INFORMATION PortConfig,
		     IN ULONG BusNumber,
		     IN OUT PPCI_SLOT_NUMBER NextSlotNumber)
{
  PCI_COMMON_CONFIG PciConfig;
  PCI_SLOT_NUMBER SlotNumber;
  ULONG DataSize;
  ULONG DeviceNumber;
  ULONG FunctionNumber;
  CHAR VendorIdString[8];
  CHAR DeviceIdString[8];
  ULONG i;
  ULONG RangeLength;

  DPRINT ("SpiGetPciConfiguration() called\n");

  if (NextSlotNumber->u.bits.FunctionNumber >= PCI_MAX_FUNCTION)
    {
      NextSlotNumber->u.bits.FunctionNumber = 0;
      NextSlotNumber->u.bits.DeviceNumber++;
    }

  if (NextSlotNumber->u.bits.DeviceNumber >= PCI_MAX_DEVICES)
    {
      NextSlotNumber->u.bits.DeviceNumber = 0;
      return FALSE;
    }

  for (DeviceNumber = NextSlotNumber->u.bits.DeviceNumber; DeviceNumber < PCI_MAX_DEVICES; DeviceNumber++)
    {
      SlotNumber.u.bits.DeviceNumber = DeviceNumber;

      for (FunctionNumber = NextSlotNumber->u.bits.FunctionNumber; FunctionNumber < PCI_MAX_FUNCTION; FunctionNumber++)
	{
	  SlotNumber.u.bits.FunctionNumber = FunctionNumber;

	  DataSize = HalGetBusData (PCIConfiguration,
				    BusNumber,
				    SlotNumber.u.AsULONG,
				    &PciConfig,
				    PCI_COMMON_HDR_LENGTH);
	  if (DataSize != PCI_COMMON_HDR_LENGTH)
	    {
	      if (FunctionNumber == 0)
		{
		  break;
		}
	      else
		{
		  continue;
		}
	    }

	  sprintf (VendorIdString, "%04hx", PciConfig.VendorID);
	  sprintf (DeviceIdString, "%04hx", PciConfig.DeviceID);

	  if (!_strnicmp(VendorIdString, HwInitializationData->VendorId, HwInitializationData->VendorIdLength) &&
	      !_strnicmp(DeviceIdString, HwInitializationData->DeviceId, HwInitializationData->DeviceIdLength))
	    {
	      DPRINT ("Found device 0x%04hx 0x%04hx at %1lu %2lu %1lu\n",
		      PciConfig.VendorID,
		      PciConfig.DeviceID,
		      BusNumber,
		      SlotNumber.u.bits.DeviceNumber,
		      SlotNumber.u.bits.FunctionNumber);

	      PortConfig->BusInterruptLevel =
	      PortConfig->BusInterruptVector = PciConfig.u.type0.InterruptLine;
	      PortConfig->SlotNumber = SlotNumber.u.AsULONG;

	      /* Initialize access ranges */
	      if (PortConfig->NumberOfAccessRanges > 0)
		{
		  if (PortConfig->NumberOfAccessRanges > PCI_TYPE0_ADDRESSES)
		    PortConfig->NumberOfAccessRanges = PCI_TYPE0_ADDRESSES;

		  for (i = 0; i < PortConfig->NumberOfAccessRanges; i++)
		    {
		      PortConfig->AccessRanges[i].RangeStart.QuadPart =
			PciConfig.u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_ADDRESS_MASK;
		      if (PortConfig->AccessRanges[i].RangeStart.QuadPart != 0)
			{
			  RangeLength = (ULONG)-1;
			  HalSetBusDataByOffset (PCIConfiguration,
						 BusNumber,
						 SlotNumber.u.AsULONG,
						 (PVOID)&RangeLength,
						 0x10 + (i * sizeof(ULONG)),
						 sizeof(ULONG));

			  HalGetBusDataByOffset (PCIConfiguration,
						 BusNumber,
						 SlotNumber.u.AsULONG,
						 (PVOID)&RangeLength,
						 0x10 + (i * sizeof(ULONG)),
						 sizeof(ULONG));

			  HalSetBusDataByOffset (PCIConfiguration,
						 BusNumber,
						 SlotNumber.u.AsULONG,
						 (PVOID)&PciConfig.u.type0.BaseAddresses[i],
						 0x10 + (i * sizeof(ULONG)),
						 sizeof(ULONG));
			  if (RangeLength != 0)
			    {
			      PortConfig->AccessRanges[i].RangeLength =
			        -(RangeLength & PCI_ADDRESS_IO_ADDRESS_MASK);
			      PortConfig->AccessRanges[i].RangeInMemory =
				!(PciConfig.u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_SPACE);

			      DPRINT("RangeStart 0x%lX  RangeLength 0x%lX  RangeInMemory %s\n",
				     PciConfig.u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_ADDRESS_MASK,
				     -(RangeLength & PCI_ADDRESS_IO_ADDRESS_MASK),
				     (PciConfig.u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_SPACE)?"FALSE":"TRUE");
			    }
			}
		    }
		}

	      NextSlotNumber->u.bits.DeviceNumber = DeviceNumber;
	      NextSlotNumber->u.bits.FunctionNumber = FunctionNumber;

              PortConfig->SlotNumber = NextSlotNumber->u.AsULONG;

	      NextSlotNumber->u.bits.FunctionNumber += 1;

	      return TRUE;
	    }


	  if (FunctionNumber == 0 && !(PciConfig.HeaderType & PCI_MULTIFUNCTION))
	    {
	      break;
	    }
	}
       NextSlotNumber->u.bits.FunctionNumber = 0;
    }

  DPRINT ("No device found\n");

  return FALSE;
}



/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortCreateClose
 *
 * DESCRIPTION
 *	Answer requests for Create/Close calls: a null operation.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceObject
 *		Pointer to a device object.
 *
 *	Irp
 *		Pointer to an IRP.
 *
 * RETURN VALUE
 * 	Status.
 */

static NTSTATUS STDCALL
ScsiPortCreateClose(IN PDEVICE_OBJECT DeviceObject,
		    IN PIRP Irp)
{
  DPRINT("ScsiPortCreateClose()\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = FILE_OPENED;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortDispatchScsi
 *
 * DESCRIPTION
 *	Answer requests for SCSI calls
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Standard dispatch arguments
 *
 * RETURNS
 *	NTSTATUS
 */

static NTSTATUS STDCALL
ScsiPortDispatchScsi(IN PDEVICE_OBJECT DeviceObject,
		     IN PIRP Irp)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  PIO_STACK_LOCATION Stack;
  PSCSI_REQUEST_BLOCK Srb;
  NTSTATUS Status = STATUS_SUCCESS;
  ULONG DataSize = 0;

  DPRINT("ScsiPortDispatchScsi(DeviceObject %p  Irp %p)\n",
	 DeviceObject, Irp);

  DeviceExtension = DeviceObject->DeviceExtension;
  Stack = IoGetCurrentIrpStackLocation(Irp);

  Srb = Stack->Parameters.Scsi.Srb;
  if (Srb == NULL)
    {
      Status = STATUS_UNSUCCESSFUL;

      Irp->IoStatus.Status = Status;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return(Status);
    }

  DPRINT("Srb: %p\n", Srb);
  DPRINT("Srb->Function: %lu\n", Srb->Function);
  DPRINT("PathId: %lu  TargetId: %lu  Lun: %lu\n", Srb->PathId, Srb->TargetId, Srb->Lun);

  LunExtension = SpiGetLunExtension(DeviceExtension,
				    Srb->PathId,
				    Srb->TargetId,
				    Srb->Lun);
  if (LunExtension == NULL)
    {
      Status = STATUS_NO_SUCH_DEVICE;

      Srb->SrbStatus = SRB_STATUS_NO_DEVICE;
      Irp->IoStatus.Status = Status;
      Irp->IoStatus.Information = 0;

      IoCompleteRequest(Irp, IO_NO_INCREMENT);

      return(Status);
    }

  switch (Srb->Function)
    {
      case SRB_FUNCTION_EXECUTE_SCSI:
      case SRB_FUNCTION_IO_CONTROL:
        IoMarkIrpPending(Irp);
	Srb->OriginalRequest = LunExtension;
        Irp->Tail.Overlay.DriverContext[3] = Srb;
	SpiProcessRequests(DeviceExtension, Irp);
	return(STATUS_PENDING);

      case SRB_FUNCTION_SHUTDOWN:
      case SRB_FUNCTION_FLUSH:
	if (DeviceExtension->PortConfig->CachesData == TRUE)
	  {
            IoMarkIrpPending(Irp);
	    Srb->OriginalRequest = LunExtension;
            Irp->Tail.Overlay.DriverContext[3] = Srb;
	    SpiProcessRequests(DeviceExtension, Irp);
	    return(STATUS_PENDING);
	  }
	break;

      case SRB_FUNCTION_CLAIM_DEVICE:
	DPRINT ("  SRB_FUNCTION_CLAIM_DEVICE\n");

	/* Reference device object and keep the device object */
	ObReferenceObject(DeviceObject);
	LunExtension->DeviceObject = DeviceObject;
	LunExtension->DeviceClaimed = TRUE;
	Srb->DataBuffer = DeviceObject;
	break;

      case SRB_FUNCTION_RELEASE_DEVICE:
	DPRINT ("  SRB_FUNCTION_RELEASE_DEVICE\n");
	DPRINT ("PathId: %lu  TargetId: %lu  Lun: %lu\n",
		Srb->PathId, Srb->TargetId, Srb->Lun);

	/* Dereference device object and clear the device object */
	ObDereferenceObject(LunExtension->DeviceObject);
	LunExtension->DeviceObject = NULL;
	LunExtension->DeviceClaimed = FALSE;
	break;

      default:
	DPRINT1("SRB function not implemented (Function %lu)\n", Srb->Function);
	Status = STATUS_NOT_IMPLEMENTED;
	break;
    }

  Irp->IoStatus.Status = Status;
  Irp->IoStatus.Information = DataSize;

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(Status);
}


/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortDeviceControl
 *
 * DESCRIPTION
 *	Answer requests for device control calls
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	Standard dispatch arguments
 *
 * RETURNS
 *	NTSTATUS
 */

static NTSTATUS STDCALL
ScsiPortDeviceControl(IN PDEVICE_OBJECT DeviceObject,
		      IN PIRP Irp)
{
  PIO_STACK_LOCATION Stack;
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("ScsiPortDeviceControl()\n");

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = 0;


  Stack = IoGetCurrentIrpStackLocation(Irp);
  DeviceExtension = DeviceObject->DeviceExtension;

  switch (Stack->Parameters.DeviceIoControl.IoControlCode)
    {
      case IOCTL_SCSI_GET_DUMP_POINTERS:
	{
	  PDUMP_POINTERS DumpPointers;
	  DPRINT("  IOCTL_SCSI_GET_DUMP_POINTERS\n");
	  DumpPointers = (PDUMP_POINTERS)Irp->AssociatedIrp.SystemBuffer;
	  DumpPointers->DeviceObject = DeviceObject;
	  
	  Irp->IoStatus.Information = sizeof(DUMP_POINTERS);
	}
	break;

      case IOCTL_SCSI_GET_CAPABILITIES:
	{
	  DPRINT("  IOCTL_SCSI_GET_CAPABILITIES\n");

	  *((PIO_SCSI_CAPABILITIES *)Irp->AssociatedIrp.SystemBuffer) =
	    DeviceExtension->PortCapabilities;

	  Irp->IoStatus.Information = sizeof(PIO_SCSI_CAPABILITIES);
	}
	break;

      case IOCTL_SCSI_GET_INQUIRY_DATA:
	{
	  DPRINT("  IOCTL_SCSI_GET_INQUIRY_DATA\n");

	  /* Copy inquiry data to the port device extension */
	  Irp->IoStatus.Information =
	    SpiGetInquiryData(DeviceExtension,
			      Irp->AssociatedIrp.SystemBuffer);
	  DPRINT("Inquiry data size: %lu\n", Irp->IoStatus.Information);
	}
	break;

      default:
	DPRINT1("  unknown ioctl code: 0x%lX\n",
	       Stack->Parameters.DeviceIoControl.IoControlCode);
	break;
    }

  IoCompleteRequest(Irp, IO_NO_INCREMENT);

  return(STATUS_SUCCESS);
}

static VOID 
SpiAllocateSrbExtension(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
			PSCSI_REQUEST_BLOCK Srb)
{
  ULONG index;

  DPRINT("SpiAllocateSrbExtension\n");

  DPRINT("DeviceExtension->VirtualAddress %x, DeviceExtension->SrbExtensionSize %x\n", 
         DeviceExtension->VirtualAddress, DeviceExtension->SrbExtensionSize);

  Srb->SrbExtension = NULL;
  if (DeviceExtension->VirtualAddress != NULL &&
      DeviceExtension->SrbExtensionSize > 0)
    {
      index = RtlFindClearBitsAndSet(&DeviceExtension->SrbExtensionAllocMap, 1, 0); 
      if (index != 0xffffffff)
        {
	  DeviceExtension->CurrentSrbExtensions++;  
          Srb->SrbExtension = DeviceExtension->VirtualAddress + index * DeviceExtension->SrbExtensionSize;
//	  Srb->QueueTag = i;
//	  Srb->QueueAction = SRB_SIMPLE_TAG_REQUEST;
	}
    }
  DPRINT("%x\n", Srb->SrbExtension);
}

static VOID
SpiFreeSrbExtension(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		    PSCSI_REQUEST_BLOCK Srb)
{
  ULONG index;

  if (DeviceExtension->VirtualAddress != NULL &&
      DeviceExtension->SrbExtensionSize > 0 &&
      Srb->SrbExtension != NULL)
    {
      index = ((ULONG_PTR)Srb->SrbExtension - (ULONG_PTR)DeviceExtension->VirtualAddress) / DeviceExtension->SrbExtensionSize;
      RtlClearBits(&DeviceExtension->SrbExtensionAllocMap, index, 1);
      DeviceExtension->CurrentSrbExtensions--;
    }
  Srb->SrbExtension = NULL;
}


static BOOLEAN STDCALL
ScsiPortStartPacket(IN OUT PVOID Context)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PSCSI_REQUEST_BLOCK Srb;
  PIRP Irp;
  PIO_STACK_LOCATION IrpStack;

  DPRINT("ScsiPortStartPacket(Context %x) called\n", Context);
    
  Srb = (PSCSI_REQUEST_BLOCK)Context;
  Irp = (PIRP)Srb->OriginalRequest;
  IrpStack = IoGetCurrentIrpStackLocation(Irp);
  DeviceExtension = IrpStack->DeviceObject->DeviceExtension;

  return(DeviceExtension->HwStartIo(&DeviceExtension->MiniPortDeviceExtension,
				    Srb));
}


static PSCSI_PORT_LUN_EXTENSION
SpiAllocateLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
			 IN UCHAR PathId,
			 IN UCHAR TargetId,
			 IN UCHAR Lun)
{
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  ULONG LunExtensionSize;

  DPRINT("SpiAllocateLunExtension (%p %u %u %u)\n",
	 DeviceExtension, PathId, TargetId, Lun);

  LunExtensionSize =
    sizeof(SCSI_PORT_LUN_EXTENSION) + DeviceExtension->LunExtensionSize;
  DPRINT("LunExtensionSize %lu\n", LunExtensionSize);

  LunExtension = ExAllocatePool(NonPagedPool,
				LunExtensionSize);
  if (LunExtension == NULL)
    {
      return NULL;
    }

  RtlZeroMemory(LunExtension,
		LunExtensionSize);

  InsertTailList(&DeviceExtension->LunExtensionListHead,
		 &LunExtension->List);

  LunExtension->PathId = PathId;
  LunExtension->TargetId = TargetId;
  LunExtension->Lun = Lun;

  LunExtension->PendingIrpCount = 0;
  LunExtension->ActiveIrpCount = 0;

  LunExtension->NextIrp = NULL;

  return LunExtension;
}


static VOID
SpiRemoveLunExtension (IN PSCSI_PORT_LUN_EXTENSION LunExtension)
{
  DPRINT("SpiRemoveLunExtension(%p) called\n",
	 LunExtension);

  if (LunExtension == NULL)
    return;

  RemoveEntryList (&LunExtension->List);


  /* Release LUN extersion data */


  ExFreePool (LunExtension);

  return;
}


static PSCSI_PORT_LUN_EXTENSION
SpiGetLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		    IN UCHAR PathId,
		    IN UCHAR TargetId,
		    IN UCHAR Lun)
{
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  PLIST_ENTRY Entry;

  DPRINT("SpiGetLunExtension(%p %u %u %u) called\n",
	 DeviceExtension, PathId, TargetId, Lun);

  if (IsListEmpty(&DeviceExtension->LunExtensionListHead))
    return NULL;

  Entry = DeviceExtension->LunExtensionListHead.Flink;
  while (Entry != &DeviceExtension->LunExtensionListHead)
    {
      LunExtension = CONTAINING_RECORD(Entry,
				       SCSI_PORT_LUN_EXTENSION,
				       List);
      if (LunExtension->PathId == PathId &&
	  LunExtension->TargetId == TargetId &&
	  LunExtension->Lun == Lun)
	{
	  return LunExtension;
	}

      Entry = Entry->Flink;
    }

  return NULL;
}


static NTSTATUS
SpiSendInquiry (IN PDEVICE_OBJECT DeviceObject,
		IN OUT PSCSI_REQUEST_BLOCK Srb,
		IN OUT PIO_STATUS_BLOCK IoStatusBlock,
		IN OUT PKEVENT Event)
{
  PIO_STACK_LOCATION IrpStack;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT ("SpiSendInquiry() called\n");


  KeInitializeEvent (Event,
		     NotificationEvent,
		     FALSE);

  Irp = IoBuildDeviceIoControlRequest (IOCTL_SCSI_EXECUTE_OUT,
				       DeviceObject,
				       NULL,
				       0,
				       Srb->DataBuffer,
				       Srb->DataTransferLength,
				       TRUE,
				       Event,
				       IoStatusBlock);
  if (Irp == NULL)
    {
      DPRINT("IoBuildDeviceIoControlRequest() failed\n");
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  /* Attach Srb to the Irp */
  IrpStack = IoGetNextIrpStackLocation (Irp);
  IrpStack->Parameters.Scsi.Srb = Srb;
  Srb->OriginalRequest = Irp;

  /* Call the driver */
  Status = IoCallDriver (DeviceObject,
			 Irp);

  return Status;
}

static VOID
SpiScanAdapter (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
  PSCSI_REQUEST_BLOCK Srb;
  PCDB Cdb;
  ULONG Bus;
  ULONG Target;
  PSCSI_PORT_SCAN_ADAPTER ScanDataArray;
  PSCSI_PORT_SCAN_ADAPTER ScanData;
  ULONG i;
  ULONG MaxCount;
  ULONG WaitCount;
  ULONG ActiveCount;
  PVOID* EventArray;
  PKWAIT_BLOCK WaitBlockArray;
  
  DPRINT ("SpiScanAdapter() called\n");

  MaxCount = DeviceExtension->PortConfig->NumberOfBuses * 
               DeviceExtension->PortConfig->MaximumNumberOfTargets;

  ScanDataArray = ExAllocatePool(NonPagedPool, MaxCount * (sizeof(SCSI_PORT_SCAN_ADAPTER) + sizeof(PVOID) + sizeof(KWAIT_BLOCK)));
  if (ScanDataArray == NULL)
    {
      return;
    }
  EventArray = (PVOID*)((PUCHAR)ScanDataArray + MaxCount * sizeof(SCSI_PORT_SCAN_ADAPTER));
  WaitBlockArray = (PKWAIT_BLOCK)((PUCHAR)EventArray + MaxCount * sizeof(PVOID));

  for (Bus = 0; Bus < DeviceExtension->PortConfig->NumberOfBuses; Bus++)
    {
      for (Target = 0; Target < DeviceExtension->PortConfig->MaximumNumberOfTargets; Target++)
	{
	  ScanData = &ScanDataArray[Bus * DeviceExtension->PortConfig->MaximumNumberOfTargets + Target];
	  ScanData->Bus = Bus;
	  ScanData->Target = Target;
	  ScanData->Lun = 0;
	  ScanData->Active = FALSE;
	}
    }
  do
    {
      ActiveCount = 0;
      WaitCount = 0;
      for (i = 0; i < MaxCount; i++)
        {
          ScanData = &ScanDataArray[i];
	  Srb = &ScanData->Srb;
          if (ScanData->Active)
            {
	      if (ScanData->Status == STATUS_PENDING &&
		  0 == KeReadStateEvent(&ScanData->Event))
	        {
		  ActiveCount++;
		  continue;
		}
	      else
	        {
                  ScanData->Status = ScanData->IoStatusBlock.Status;
	        }
	      ScanData->Active = FALSE;
	      DPRINT ("Target %lu  Lun %lu\n", ScanData->Target, ScanData->Lun);
	      DPRINT ("Status %lx  Srb.SrbStatus %x\n", ScanData->Status, Srb->SrbStatus);
	      DPRINT ("DeviceTypeQualifier %x\n", ((PINQUIRYDATA)Srb->DataBuffer)->DeviceTypeQualifier);

	      if (NT_SUCCESS(ScanData->Status) &&
		  (Srb->SrbStatus == SRB_STATUS_SUCCESS || 
		   (Srb->SrbStatus == SRB_STATUS_DATA_OVERRUN && 
		    Srb->DataTransferLength >= INQUIRYDATABUFFERSIZE)) &&
		  ((PINQUIRYDATA)Srb->DataBuffer)->DeviceTypeQualifier == 0)
		{
		  /* Copy inquiry data */
		  RtlCopyMemory (&ScanData->LunExtension->InquiryData,
				 Srb->DataBuffer,
				 min(sizeof(INQUIRYDATA), Srb->DataTransferLength));
	          ScanData->Lun++;
		}
	      else
		{
		  SpiRemoveLunExtension (ScanData->LunExtension);
		  ScanData->Lun = SCSI_MAXIMUM_LOGICAL_UNITS;
		}
	    }
          if (ScanData->Lun >= SCSI_MAXIMUM_LOGICAL_UNITS)
            {
	      continue;
	    }
          RtlZeroMemory(Srb, sizeof(SCSI_REQUEST_BLOCK));
          Srb->SrbFlags = SRB_FLAGS_DATA_IN;
          Srb->DataBuffer = ScanData->DataBuffer;
          Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
          Srb->DataTransferLength = 255; //256;
          Srb->CdbLength = 6;
          Srb->Lun = ScanData->Lun;
          Srb->PathId = ScanData->Bus;
          Srb->TargetId = ScanData->Target;
          Srb->SrbStatus = SRB_STATUS_SUCCESS;
	  Srb->TimeOutValue = 2;
          Cdb = (PCDB) &Srb->Cdb;

          Cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;
          Cdb->CDB6INQUIRY.AllocationLength = 255;
          Cdb->CDB6INQUIRY.LogicalUnitNumber = ScanData->Lun;

          RtlZeroMemory(Srb->DataBuffer, 256);

          ScanData->LunExtension = SpiAllocateLunExtension (DeviceExtension,
						            ScanData->Bus,
						            ScanData->Target,
						            ScanData->Lun);
          if (ScanData->LunExtension == NULL)
	    {
	      DPRINT1("Failed to allocate the LUN extension!\n");
	      ScanData->Lun = SCSI_MAXIMUM_LOGICAL_UNITS;
	      continue;
	    }
          ScanData->Status = SpiSendInquiry (DeviceExtension->DeviceObject,
				             Srb,
				             &ScanData->IoStatusBlock,
				             &ScanData->Event);
	  ScanData->Active = TRUE;
	  ActiveCount++;
	  if (ScanData->Status == STATUS_PENDING)
	    {
              EventArray[WaitCount] = &ScanData->Event;
              WaitCount++;
            }
	}
      if (WaitCount > 0 && WaitCount == ActiveCount)
        {
          KeWaitForMultipleObjects(WaitCount,
                                   EventArray,
				   WaitAny,
				   Executive,
				   KernelMode,
				   FALSE,
				   NULL,
				   WaitBlockArray);
	}
    } 
  while (ActiveCount > 0);

  ExFreePool(ScanDataArray);
 
  DPRINT ("SpiScanAdapter() done\n");
}


static ULONG
SpiGetInquiryData(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		  OUT PSCSI_ADAPTER_BUS_INFO AdapterBusInfo)
{
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  PSCSI_INQUIRY_DATA UnitInfo, PrevUnit;
  ULONG Bus;
  ULONG Target;
  ULONG Lun;
  ULONG UnitCount;

  DPRINT("SpiGetInquiryData() called\n");

  /* Copy inquiry data to the port device extension */
  AdapterBusInfo->NumberOfBuses = DeviceExtension->PortConfig->NumberOfBuses;

  UnitInfo = (PSCSI_INQUIRY_DATA)
	((PUCHAR)AdapterBusInfo + sizeof(SCSI_ADAPTER_BUS_INFO) +
	 (sizeof(SCSI_BUS_DATA) * (AdapterBusInfo->NumberOfBuses - 1)));

  for (Bus = 0; Bus < AdapterBusInfo->NumberOfBuses; Bus++)
    {
      AdapterBusInfo->BusData[Bus].InitiatorBusId =
	DeviceExtension->PortConfig->InitiatorBusId[Bus];
      AdapterBusInfo->BusData[Bus].InquiryDataOffset =
	(ULONG)((PUCHAR)UnitInfo - (PUCHAR)AdapterBusInfo);

      PrevUnit = NULL;
      UnitCount = 0;

      for (Target = 0; Target < DeviceExtension->PortConfig->MaximumNumberOfTargets; Target++)
	{
	  for (Lun = 0; Lun < SCSI_MAXIMUM_LOGICAL_UNITS; Lun++)
	    {
	      LunExtension = SpiGetLunExtension(DeviceExtension,
						Bus,
						Target,
						Lun);
	      if (LunExtension != NULL)
		{
		  DPRINT("(Bus %lu Target %lu Lun %lu)\n",
			 Bus, Target, Lun);

		  UnitInfo->PathId = Bus;
		  UnitInfo->TargetId = Target;
		  UnitInfo->Lun = Lun;
		  UnitInfo->InquiryDataLength = INQUIRYDATABUFFERSIZE;
		  RtlCopyMemory (&UnitInfo->InquiryData,
				 &LunExtension->InquiryData,
				 INQUIRYDATABUFFERSIZE);
		  if (PrevUnit != NULL)
		    {
		      PrevUnit->NextInquiryDataOffset =
			(ULONG)((ULONG_PTR)UnitInfo-(ULONG_PTR)AdapterBusInfo);
		    }
		  PrevUnit = UnitInfo;
		  UnitInfo = (PSCSI_INQUIRY_DATA)((PUCHAR)UnitInfo + sizeof(SCSI_INQUIRY_DATA)+INQUIRYDATABUFFERSIZE-1);
		  UnitCount++;
		}
	    }
	}
      DPRINT("UnitCount: %lu\n", UnitCount);
      AdapterBusInfo->BusData[Bus].NumberOfLogicalUnits = UnitCount;
      if (UnitCount == 0)
	{
	  AdapterBusInfo->BusData[Bus].InquiryDataOffset = 0;
	}
    }

  DPRINT("Data size: %lu\n", (ULONG)UnitInfo - (ULONG)AdapterBusInfo);

  return (ULONG)((PUCHAR)UnitInfo-(PUCHAR)AdapterBusInfo);
}


static BOOLEAN STDCALL
ScsiPortIsr(IN PKINTERRUPT Interrupt,
	    IN PVOID ServiceContext)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("ScsiPortIsr() called!\n");

  DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)ServiceContext;

  return DeviceExtension->HwInterrupt(&DeviceExtension->MiniPortDeviceExtension);
}


//    ScsiPortDpc
//  DESCRIPTION:
//
//  RUN LEVEL:
//
//  ARGUMENTS:
//    IN PKDPC          Dpc
//    IN PDEVICE_OBJECT DpcDeviceObject
//    IN PIRP           DpcIrp
//    IN PVOID          DpcContext
//
static VOID STDCALL
ScsiPortDpc(IN PKDPC Dpc,
	    IN PDEVICE_OBJECT DpcDeviceObject,
	    IN PIRP DpcIrp,
	    IN PVOID DpcContext)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;

  DPRINT("ScsiPortDpc(Dpc %p  DpcDeviceObject %p  DpcIrp %p  DpcContext %p)\n",
	 Dpc, DpcDeviceObject, DpcIrp, DpcContext);

  DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)DpcContext;

  SpiProcessRequests(DeviceExtension, NULL);

  DPRINT("ScsiPortDpc() done\n");
}


//    ScsiPortIoTimer
//  DESCRIPTION:
//    This function handles timeouts and other time delayed processing
//
//  RUN LEVEL:
//
//  ARGUMENTS:
//    IN  PDEVICE_OBJECT  DeviceObject  Device object registered with timer
//    IN  PVOID           Context       the Controller extension for the
//                                      controller the device is on
//
static VOID STDCALL
ScsiPortIoTimer(PDEVICE_OBJECT DeviceObject,
		PVOID Context)
{
  DPRINT1("ScsiPortIoTimer()\n");
}


static PSCSI_REQUEST_BLOCK
ScsiPortInitSenseRequestSrb(PSCSI_REQUEST_BLOCK OriginalSrb)
{
  PSCSI_REQUEST_BLOCK Srb;
  ULONG Length;
  PCDB Cdb;

  Length = sizeof(SCSI_REQUEST_BLOCK) + sizeof(SENSE_DATA) + 32;
  Srb = ExAllocatePoolWithTag(NonPagedPool, 
                              Length,
		              TAG('S', 'S', 'r', 'b'));
  if (Srb == NULL)
    {
      return NULL;
    }

  RtlZeroMemory(Srb, Length);

  Srb->PathId = OriginalSrb->PathId;
  Srb->TargetId = OriginalSrb->TargetId;
  Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
  Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
  Srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
  Srb->OriginalRequest = OriginalSrb->OriginalRequest;

  Srb->TimeOutValue = 4;

  Srb->CdbLength = 6;
  /* The DataBuffer must be located in contiguous physical memory if 
   * the miniport driver uses dma for the sense info. The size of 
   * the sense data is 18 byte. If the buffer starts at a 32 byte 
   * boundary than is the buffer always in one memory page. 
   */
  Srb->DataBuffer = (PVOID)ROUND_UP((ULONG_PTR)(Srb + 1), 32);
  Srb->DataTransferLength = sizeof(SENSE_DATA);

  Cdb = (PCDB)Srb->Cdb;
  Cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
  Cdb->CDB6INQUIRY.AllocationLength = sizeof(SENSE_DATA);

  return(Srb);
}

/**********************************************************************
 * NAME							INTERNAL
 *	SpiBuildDeviceMap
 *
 * DESCRIPTION
 *	Builds the registry device map of all device which are attached
 *	to the given SCSI HBA port. The device map is located at:
 *	  \Registry\Machine\DeviceMap\Scsi
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DeviceExtension
 *		...
 *
 *	RegistryPath
 *		Name of registry driver service key.
 *
 * RETURNS
 *	NTSTATUS
 */

static NTSTATUS
SpiBuildDeviceMap (PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		   PUNICODE_STRING RegistryPath)
{
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  WCHAR NameBuffer[64];
  ULONG Disposition;
  HANDLE ScsiKey;
  HANDLE ScsiPortKey = NULL;
  HANDLE ScsiBusKey = NULL;
  HANDLE ScsiInitiatorKey = NULL;
  HANDLE ScsiTargetKey = NULL;
  HANDLE ScsiLunKey = NULL;
  ULONG BusNumber;
  ULONG Target;
  ULONG CurrentTarget;
  ULONG Lun;
  PWCHAR DriverName;
  ULONG UlongData;
  PWCHAR TypeName;
  NTSTATUS Status;

  DPRINT("SpiBuildDeviceMap() called\n");

  if (DeviceExtension == NULL || RegistryPath == NULL)
    {
      DPRINT1("Invalid parameter\n");
      return(STATUS_INVALID_PARAMETER);
    }

  /* Open or create the 'Scsi' subkey */
  RtlRosInitUnicodeStringFromLiteral(&KeyName,
				  L"\\Registry\\Machine\\Hardware\\DeviceMap\\Scsi");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE | OBJ_OPENIF,
			     0,
			     NULL);
  Status = ZwCreateKey(&ScsiKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       &Disposition);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
      return(Status);
    }

  /* Create new 'Scsi Port X' subkey */
  DPRINT("Scsi Port %lu\n",
	 DeviceExtension->PortNumber);

  swprintf(NameBuffer,
	   L"Scsi Port %lu",
	   DeviceExtension->PortNumber);
  RtlInitUnicodeString(&KeyName,
		       NameBuffer);
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     0,
			     ScsiKey,
			     NULL);
  Status = ZwCreateKey(&ScsiPortKey,
		       KEY_ALL_ACCESS,
		       &ObjectAttributes,
		       0,
		       NULL,
		       REG_OPTION_VOLATILE,
		       &Disposition);
  ZwClose(ScsiKey);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
      return(Status);
    }

  /*
   * Create port-specific values
   */

  /* Set 'DMA Enabled' (REG_DWORD) value */
  UlongData = (ULONG)!DeviceExtension->PortCapabilities->AdapterUsesPio;
  DPRINT("  DMA Enabled = %s\n", (UlongData) ? "TRUE" : "FALSE");
  RtlInitUnicodeString(&ValueName,
		       L"DMA Enabled");
  Status = ZwSetValueKey(ScsiPortKey,
			 &ValueName,
			 0,
			 REG_DWORD,
			 &UlongData,
			 sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwSetValueKey('DMA Enabled') failed (Status %lx)\n", Status);
      ZwClose(ScsiPortKey);
      return(Status);
    }

  /* Set 'Driver' (REG_SZ) value */
  DriverName = wcsrchr(RegistryPath->Buffer, L'\\') + 1;
  RtlInitUnicodeString(&ValueName,
		       L"Driver");
  Status = ZwSetValueKey(ScsiPortKey,
			 &ValueName,
			 0,
			 REG_SZ,
			 DriverName,
			 (wcslen(DriverName) + 1) * sizeof(WCHAR));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwSetValueKey('Driver') failed (Status %lx)\n", Status);
      ZwClose(ScsiPortKey);
      return(Status);
    }

  /* Set 'Interrupt' (REG_DWORD) value (NT4 only) */
  UlongData = (ULONG)DeviceExtension->PortConfig->BusInterruptLevel;
  DPRINT("  Interrupt = %lu\n", UlongData);
  RtlInitUnicodeString(&ValueName,
		       L"Interrupt");
  Status = ZwSetValueKey(ScsiPortKey,
			 &ValueName,
			 0,
			 REG_DWORD,
			 &UlongData,
			 sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwSetValueKey('Interrupt') failed (Status %lx)\n", Status);
      ZwClose(ScsiPortKey);
      return(Status);
    }

  /* Set 'IOAddress' (REG_DWORD) value (NT4 only) */
  UlongData = ScsiPortConvertPhysicalAddressToUlong(DeviceExtension->PortConfig->AccessRanges[0].RangeStart);
  DPRINT("  IOAddress = %lx\n", UlongData);
  RtlInitUnicodeString(&ValueName,
		       L"IOAddress");
  Status = ZwSetValueKey(ScsiPortKey,
			 &ValueName,
			 0,
			 REG_DWORD,
			 &UlongData,
			 sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("ZwSetValueKey('IOAddress') failed (Status %lx)\n", Status);
      ZwClose(ScsiPortKey);
      return(Status);
    }

  /* Enumerate buses */
  for (BusNumber = 0; BusNumber < DeviceExtension->PortConfig->NumberOfBuses; BusNumber++)
    {
      /* Create 'Scsi Bus X' key */
      DPRINT("    Scsi Bus %lu\n", BusNumber);
      swprintf(NameBuffer,
	       L"Scsi Bus %lu",
	       BusNumber);
      RtlInitUnicodeString(&KeyName,
			   NameBuffer);
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyName,
				 0,
				 ScsiPortKey,
				 NULL);
      Status = ZwCreateKey(&ScsiBusKey,
			   KEY_ALL_ACCESS,
			   &ObjectAttributes,
			   0,
			   NULL,
			   REG_OPTION_VOLATILE,
			   &Disposition);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
	  goto ByeBye;
	}

      /* Create 'Initiator Id X' key */
      DPRINT("      Initiator Id %u\n",
	      DeviceExtension->PortConfig->InitiatorBusId[BusNumber]);
      swprintf(NameBuffer,
	       L"Initiator Id %u",
	       (unsigned int)(UCHAR)DeviceExtension->PortConfig->InitiatorBusId[BusNumber]);
      RtlInitUnicodeString(&KeyName,
			   NameBuffer);
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyName,
				 0,
				 ScsiBusKey,
				 NULL);
      Status = ZwCreateKey(&ScsiInitiatorKey,
			   KEY_ALL_ACCESS,
			   &ObjectAttributes,
			   0,
			   NULL,
			   REG_OPTION_VOLATILE,
			   &Disposition);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
	  goto ByeBye;
	}

      /* FIXME: Are there any initiator values (??) */

      ZwClose(ScsiInitiatorKey);
      ScsiInitiatorKey = NULL;


      /* Enumerate targets */
      CurrentTarget = (ULONG)-1;
      ScsiTargetKey = NULL;
      for (Target = 0; Target < DeviceExtension->PortConfig->MaximumNumberOfTargets; Target++)
	{
	  for (Lun = 0; Lun < SCSI_MAXIMUM_LOGICAL_UNITS; Lun++)
	    {
	      LunExtension = SpiGetLunExtension(DeviceExtension,
						BusNumber,
						Target,
						Lun);
	      if (LunExtension != NULL)
		{
		  if (Target != CurrentTarget)
		    {
		      /* Close old target key */
		      if (ScsiTargetKey != NULL)
			{
			  ZwClose(ScsiTargetKey);
			  ScsiTargetKey = NULL;
			}

		      /* Create 'Target Id X' key */
		      DPRINT("      Target Id %lu\n", Target);
		      swprintf(NameBuffer,
			       L"Target Id %lu",
			       Target);
		      RtlInitUnicodeString(&KeyName,
					   NameBuffer);
		      InitializeObjectAttributes(&ObjectAttributes,
						 &KeyName,
						 0,
						 ScsiBusKey,
						 NULL);
		      Status = ZwCreateKey(&ScsiTargetKey,
					   KEY_ALL_ACCESS,
					   &ObjectAttributes,
					   0,
					   NULL,
					   REG_OPTION_VOLATILE,
					   &Disposition);
		      if (!NT_SUCCESS(Status))
			{
			  DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
			  goto ByeBye;
			}

		      CurrentTarget = Target;
		    }

		  /* Create 'Logical Unit Id X' key */
		  DPRINT("        Logical Unit Id %lu\n", Lun);
		  swprintf(NameBuffer,
			   L"Logical Unit Id %lu",
			   Lun);
		  RtlInitUnicodeString(&KeyName,
				       NameBuffer);
		  InitializeObjectAttributes(&ObjectAttributes,
					     &KeyName,
					     0,
					     ScsiTargetKey,
					     NULL);
		  Status = ZwCreateKey(&ScsiLunKey,
				       KEY_ALL_ACCESS,
				       &ObjectAttributes,
				       0,
				       NULL,
				       REG_OPTION_VOLATILE,
				       &Disposition);
		  if (!NT_SUCCESS(Status))
		    {
		      DPRINT("ZwCreateKey() failed (Status %lx)\n", Status);
		      goto ByeBye;
		    }

		  /* Set 'Identifier' (REG_SZ) value */
		  swprintf(NameBuffer,
			   L"%.8S%.16S%.4S",
			   LunExtension->InquiryData.VendorId,
			   LunExtension->InquiryData.ProductId,
			   LunExtension->InquiryData.ProductRevisionLevel);
		  DPRINT("          Identifier = '%S'\n", NameBuffer);
		  RtlInitUnicodeString(&ValueName,
				       L"Identifier");
		  Status = ZwSetValueKey(ScsiLunKey,
					 &ValueName,
					 0,
					 REG_SZ,
					 NameBuffer,
					 (wcslen(NameBuffer) + 1) * sizeof(WCHAR));
		  if (!NT_SUCCESS(Status))
		    {
		      DPRINT("ZwSetValueKey('Identifier') failed (Status %lx)\n", Status);
		      goto ByeBye;
		    }

		  /* Set 'Type' (REG_SZ) value */
		  switch (LunExtension->InquiryData.DeviceType)
		    {
		      case 0:
			TypeName = L"DiskPeripheral";
			break;
		      case 1:
			TypeName = L"TapePeripheral";
			break;
		      case 2:
			TypeName = L"PrinterPeripheral";
			break;
		      case 4:
			TypeName = L"WormPeripheral";
			break;
		      case 5:
			TypeName = L"CdRomPeripheral";
			break;
		      case 6:
			TypeName = L"ScannerPeripheral";
			break;
		      case 7:
			TypeName = L"OpticalDiskPeripheral";
			break;
		      case 8:
			TypeName = L"MediumChangerPeripheral";
			break;
		      case 9:
			TypeName = L"CommunicationPeripheral";
			break;
		      default:
			TypeName = L"OtherPeripheral";
			break;
		    }
		  DPRINT("          Type = '%S'\n", TypeName);
		  RtlInitUnicodeString(&ValueName,
				       L"Type");
		  Status = ZwSetValueKey(ScsiLunKey,
					 &ValueName,
					 0,
					 REG_SZ,
					 TypeName,
					 (wcslen(TypeName) + 1) * sizeof(WCHAR));
		  if (!NT_SUCCESS(Status))
		    {
		      DPRINT("ZwSetValueKey('Type') failed (Status %lx)\n", Status);
		      goto ByeBye;
		    }

		  ZwClose(ScsiLunKey);
		  ScsiLunKey = NULL;
		}
	    }

	  /* Close old target key */
	  if (ScsiTargetKey != NULL)
	    {
	      ZwClose(ScsiTargetKey);
	      ScsiTargetKey = NULL;
	    }
	}

      ZwClose(ScsiBusKey);
      ScsiBusKey = NULL;
    }

ByeBye:
  if (ScsiLunKey != NULL)
    ZwClose (ScsiLunKey);

  if (ScsiInitiatorKey != NULL)
    ZwClose (ScsiInitiatorKey);

  if (ScsiTargetKey != NULL)
    ZwClose (ScsiTargetKey);

  if (ScsiBusKey != NULL)
    ZwClose (ScsiBusKey);

  if (ScsiPortKey != NULL)
    ZwClose (ScsiPortKey);

  DPRINT("SpiBuildDeviceMap() done\n");

  return Status;
}

static VOID 
SpiRemoveActiveIrp(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension, 
	           PIRP Irp,
	           PIRP PrevIrp)
{
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  PIRP CurrentIrp;
  LunExtension = Irp->Tail.Overlay.DriverContext[2];
  InterlockedDecrement((PLONG)&LunExtension->ActiveIrpCount);
  InterlockedDecrement((PLONG)&DeviceExtension->ActiveIrpCount);
  if (PrevIrp)
    {
      InterlockedExchangePointer(&PrevIrp->Tail.Overlay.DriverContext[0], 
	                         Irp->Tail.Overlay.DriverContext[0]);
    }
  else
    {
      InterlockedExchangePointer(&DeviceExtension->NextIrp,
	                         Irp->Tail.Overlay.DriverContext[0]);
    }
  if (LunExtension->NextIrp == Irp)
    {
      InterlockedExchangePointer(&LunExtension->NextIrp,
	                         Irp->Tail.Overlay.DriverContext[1]);
      return;
    }
  else
    {
      CurrentIrp = LunExtension->NextIrp;
      while (CurrentIrp)
        {
	  if (CurrentIrp->Tail.Overlay.DriverContext[1] == Irp)
	    {
	      InterlockedExchangePointer(&CurrentIrp->Tail.Overlay.DriverContext[1],
		                         Irp->Tail.Overlay.DriverContext[1]);
	      return;
	    }
          CurrentIrp = CurrentIrp->Tail.Overlay.DriverContext[1];
	}
      KEBUGCHECK(0);
    }
}

static VOID
SpiAddActiveIrp(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		PIRP Irp)
{
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  PSCSI_REQUEST_BLOCK Srb;
  LunExtension = Irp->Tail.Overlay.DriverContext[2];
  Srb = Irp->Tail.Overlay.DriverContext[3];
  Srb->SrbFlags |= SRB_FLAGS_IS_ACTIVE;
  Irp->Tail.Overlay.DriverContext[0] = (PVOID)DeviceExtension->NextIrp;
  InterlockedExchangePointer(&DeviceExtension->NextIrp, Irp);
  Irp->Tail.Overlay.DriverContext[1] = (PVOID)LunExtension->NextIrp;
  InterlockedExchangePointer(&LunExtension->NextIrp, Irp);
  InterlockedIncrement((PLONG)&LunExtension->ActiveIrpCount);
  InterlockedIncrement((PLONG)&DeviceExtension->ActiveIrpCount);
}

static VOID
SpiProcessRequests(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension, 
	           IN PIRP NextIrp)
{
  /*
   * Using of some fields from Srb and Irp while processing requests:
   *
   * NextIrp on entry:
   *   Srb->OriginalRequest -> LunExtension
   *   Irp->Tail.Overlay.DriverContext[3] -> original Srb
   *   IoStack->Parameters.Scsi.Srb -> original Srb
   *
   * Irp is within the pending irp list:
   *   Srb->OriginalRequest -> LunExtension
   *   Irp->Tail.Overlay.DriverContext[0] and DriverContext[1] -> ListEntry for queue
   *   Irp->Tail.Overlay.DriverContext[2] -> sort key (from Srb->QueueSortKey)
   *   Irp->Tail.Overlay.DriverContext[3] -> current Srb (original or sense request)
   *   IoStack->Parameters.Scsi.Srb -> original Srb
   *   
   * Irp is within the active irp list or while other processing:
   *   Srb->OriginalRequest -> Irp
   *   Irp->Tail.Overlay.DriverContext[0] -> next irp, DeviceExtension->NextIrp is head.
   *   Irp->Tail.Overlay.DriverContext[1] -> next irp, LunExtension->NextIrp is head.
   *   Irp->Tail.Overlay.DriverContext[2] -> LunExtension
   *   Irp->Tail.Overlay.DriverContext[3] -> current Srb (original or sense request)
   *   IoStack->Parameters.Scsi.Srb -> original Srb
   */
  PIO_STACK_LOCATION IrpStack;
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  PLIST_ENTRY ListEntry;
  KIRQL oldIrql;
  PIRP Irp;
  LIST_ENTRY NextIrpListHead;
  LIST_ENTRY CompleteIrpListHead;
  PSCSI_REQUEST_BLOCK Srb;
  PSCSI_REQUEST_BLOCK OriginalSrb;
  PIRP PrevIrp;

  DPRINT("SpiProcessRequests() called\n");

  InitializeListHead(&NextIrpListHead);
  InitializeListHead(&CompleteIrpListHead);

  KeAcquireSpinLock(&DeviceExtension->Lock, &oldIrql);

  if (NextIrp)
    {
      Srb = NextIrp->Tail.Overlay.DriverContext[3];
      /* 
       * FIXME:
       *   Is this the right place to set this flag ?
       */
      if (DeviceExtension->PortConfig->MultipleRequestPerLu)
        {
	  Srb->SrbFlags |= SRB_FLAGS_QUEUE_ACTION_ENABLE;
	}
      NextIrp->Tail.Overlay.DriverContext[2] = (PVOID)Srb->QueueSortKey;
      LunExtension = Srb->OriginalRequest;

      ListEntry = DeviceExtension->PendingIrpListHead.Flink;
      while (ListEntry != &DeviceExtension->PendingIrpListHead)
        {
          Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.DriverContext[0]);
          if ((ULONG)Irp->Tail.Overlay.DriverContext[2] > Srb->QueueSortKey)
            {
              break;
	    }
          ListEntry = ListEntry->Flink;
	}
      InsertTailList(ListEntry, (PLIST_ENTRY)&NextIrp->Tail.Overlay.DriverContext[0]);
      DeviceExtension->PendingIrpCount++;
      LunExtension->PendingIrpCount++;
    }

  while (DeviceExtension->Flags & IRP_FLAG_COMPLETE ||
         (((DeviceExtension->SrbExtensionSize == 0 || DeviceExtension->CurrentSrbExtensions < DeviceExtension->MaxSrbExtensions) && 
	  DeviceExtension->PendingIrpCount > 0 &&
	  (DeviceExtension->Flags & (IRP_FLAG_NEXT|IRP_FLAG_NEXT_LU) || DeviceExtension->NextIrp == NULL))))
    {
      DPRINT ("RequestComplete %d, NextRequest %d, NextLuRequest %d, PendingIrpCount %d, ActiveIrpCount %d\n", 
	      DeviceExtension->Flags & IRP_FLAG_COMPLETE ? 1 : 0, 
	      DeviceExtension->Flags & IRP_FLAG_NEXT ? 1 : 0,
	      DeviceExtension->Flags & IRP_FLAG_NEXT_LU ? 1 : 0, 
	      DeviceExtension->PendingIrpCount, 
	      DeviceExtension->ActiveIrpCount);


      if (DeviceExtension->Flags & IRP_FLAG_COMPLETE)
        {
	  DeviceExtension->Flags &= ~IRP_FLAG_COMPLETE;
	  PrevIrp = NULL;
	  Irp = DeviceExtension->NextIrp;
	  while (Irp)
	    {
	      NextIrp = (PIRP)Irp->Tail.Overlay.DriverContext[0];
	      Srb = Irp->Tail.Overlay.DriverContext[3];
              if (!(Srb->SrbFlags & SRB_FLAGS_IS_ACTIVE))
	        {
                  BOOLEAN CompleteThisRequest;
                  LunExtension = Irp->Tail.Overlay.DriverContext[2];
		  IrpStack = IoGetCurrentIrpStackLocation(Irp);
		  OriginalSrb = IrpStack->Parameters.Scsi.Srb;

		  if (Srb->SrbStatus == SRB_STATUS_BUSY)
		    {
		      SpiRemoveActiveIrp(DeviceExtension, Irp, PrevIrp);
                      SpiFreeSrbExtension(DeviceExtension, OriginalSrb);
		      InsertHeadList(&DeviceExtension->PendingIrpListHead, (PLIST_ENTRY)&Irp->Tail.Overlay.DriverContext[0]);
		      DeviceExtension->PendingIrpCount++;
		      LunExtension->PendingIrpCount++;
		      Irp = NextIrp;
		      continue;
		    }

                  if (OriginalSrb != Srb)
	            {
	              SENSE_DATA* SenseInfoBuffer;

		      SenseInfoBuffer = Srb->DataBuffer;

	              DPRINT("Got sense data!\n");

	              DPRINT("Valid: %x\n", SenseInfoBuffer->Valid);
	              DPRINT("ErrorCode: %x\n", SenseInfoBuffer->ErrorCode);
	              DPRINT("SenseKey: %x\n", SenseInfoBuffer->SenseKey);
	              DPRINT("SenseCode: %x\n", SenseInfoBuffer->AdditionalSenseCode);
	      
	              /* Copy sense data */
                      RtlCopyMemory(OriginalSrb->SenseInfoBuffer,
		                    SenseInfoBuffer,
		                    sizeof(SENSE_DATA));
	              OriginalSrb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
		      ExFreePool(Srb);
		      CompleteThisRequest = TRUE;
		    }
		  else if (SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS &&
	                   Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION &&
	                   Srb->SenseInfoBuffer != NULL &&
	                   Srb->SenseInfoBufferLength >= sizeof(SENSE_DATA) &&
	                   !(Srb->SrbStatus & SRB_STATUS_AUTOSENSE_VALID))
	            {
	              DPRINT("SCSIOP_REQUEST_SENSE required!\n");

       	              Srb = ScsiPortInitSenseRequestSrb(OriginalSrb);

	              if (Srb)
	                {
			  CompleteThisRequest = FALSE;
			  Irp->Tail.Overlay.DriverContext[3] = Srb;
			  SpiRemoveActiveIrp(DeviceExtension, Irp, PrevIrp);
	                  SpiFreeSrbExtension(DeviceExtension, Srb);

                          Srb->OriginalRequest = LunExtension;
                          Irp->Tail.Overlay.DriverContext[2] = 0;

			  InsertHeadList(&DeviceExtension->PendingIrpListHead, (PLIST_ENTRY)&Irp->Tail.Overlay.DriverContext[0]);
			  DeviceExtension->PendingIrpCount++;
			  LunExtension->PendingIrpCount++;
			  Irp = NextIrp;
			  continue;
			}
		      else
		        {
			  CompleteThisRequest = TRUE;
			}
		    }
		  else
		    {
	              DPRINT("Complete Request\n");
		      CompleteThisRequest = TRUE;
		    }
		  if (CompleteThisRequest)
		    {
		      SpiRemoveActiveIrp(DeviceExtension, Irp, PrevIrp);
	              InsertHeadList(&CompleteIrpListHead, (PLIST_ENTRY)&Irp->Tail.Overlay.DriverContext[0]);
                      SpiFreeSrbExtension(DeviceExtension, OriginalSrb);
		    }
		  else
		    {
		      PrevIrp = Irp;
		    }
		  Irp = NextIrp;
		  continue;
		}	     
	      PrevIrp = Irp;
	      Irp = NextIrp;
	    }
	}
      if (!IsListEmpty(&CompleteIrpListHead))
        {
          KeReleaseSpinLockFromDpcLevel(&DeviceExtension->Lock);
	  while (!IsListEmpty(&CompleteIrpListHead))
	    {
	      ListEntry = RemoveTailList(&CompleteIrpListHead);
	      Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.DriverContext[0]);
	      IoCompleteRequest(Irp, IO_NO_INCREMENT);
	    }
	  KeAcquireSpinLockAtDpcLevel(&DeviceExtension->Lock);
        }
      if (DeviceExtension->Flags & (IRP_FLAG_NEXT|IRP_FLAG_NEXT_LU) &&
          (DeviceExtension->SrbExtensionSize == 0 || DeviceExtension->CurrentSrbExtensions < DeviceExtension->MaxSrbExtensions)) 
        {
	  BOOLEAN StartThisRequest;
	  ListEntry = DeviceExtension->PendingIrpListHead.Flink;
	  while (ListEntry != &DeviceExtension->PendingIrpListHead)
	    {
	      Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.DriverContext[0]);
	      ListEntry = ListEntry->Flink;
	      Srb = Irp->Tail.Overlay.DriverContext[3];
	      LunExtension = Srb->OriginalRequest;
	      if (DeviceExtension->SrbExtensionSize > 0 &&
		  DeviceExtension->CurrentSrbExtensions >= DeviceExtension->MaxSrbExtensions)
	        {
		  break;
		}
	      if (LunExtension->Flags & IRP_FLAG_NEXT_LU)
                {
		  StartThisRequest = TRUE;
		  LunExtension->Flags &= ~IRP_FLAG_NEXT_LU;
                  DeviceExtension->Flags &= ~IRP_FLAG_NEXT_LU;
		}
	      else if (DeviceExtension->Flags & IRP_FLAG_NEXT && 
		       LunExtension->ActiveIrpCount == 0)
	        {
		  StartThisRequest = TRUE;
                  DeviceExtension->Flags &= ~IRP_FLAG_NEXT;
		}
	      else
	        {
		  StartThisRequest = FALSE;
		}
	      if (StartThisRequest)
	        {
		  LunExtension->PendingIrpCount--;
		  DeviceExtension->PendingIrpCount--;
                  RemoveEntryList((PLIST_ENTRY)&Irp->Tail.Overlay.DriverContext[0]);
		  Irp->Tail.Overlay.DriverContext[2] = LunExtension;
		  Srb->OriginalRequest = Irp;
		  SpiAllocateSrbExtension(DeviceExtension, Srb);

                  InsertHeadList(&NextIrpListHead, (PLIST_ENTRY)&Irp->Tail.Overlay.DriverContext[0]);		   
		}
	    }
	}

      if (!IsListEmpty(&NextIrpListHead))
        {
	  while (!IsListEmpty(&NextIrpListHead))
	    {
	      ListEntry = RemoveTailList(&NextIrpListHead);
	      Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.DriverContext[0]);
	      KeReleaseSpinLockFromDpcLevel(&DeviceExtension->Lock);

	      // Start this Irp
	      SpiStartIo(DeviceExtension, Irp);
	      KeAcquireSpinLockAtDpcLevel(&DeviceExtension->Lock);
	    }
	}

      if (!IsListEmpty(&DeviceExtension->PendingIrpListHead) &&
	  DeviceExtension->NextIrp == NULL &&
	  (DeviceExtension->SrbExtensionSize == 0 || DeviceExtension->CurrentSrbExtensions < DeviceExtension->MaxSrbExtensions))
        {
	  ListEntry = RemoveHeadList(&DeviceExtension->PendingIrpListHead);
          Irp = CONTAINING_RECORD(ListEntry, IRP, Tail.Overlay.DriverContext[0]);
          Srb = Irp->Tail.Overlay.DriverContext[3];
	  LunExtension = Srb->OriginalRequest;
	  Irp->Tail.Overlay.DriverContext[2] = LunExtension;
	  Srb->OriginalRequest = Irp;

	  LunExtension->PendingIrpCount--;
	  DeviceExtension->PendingIrpCount--;
	  SpiAllocateSrbExtension(DeviceExtension, Srb);
          KeReleaseSpinLockFromDpcLevel(&DeviceExtension->Lock);

          /* Start this irp */
          SpiStartIo(DeviceExtension, Irp);
	  KeAcquireSpinLockAtDpcLevel(&DeviceExtension->Lock);
        }
     }
   KeReleaseSpinLock(&DeviceExtension->Lock, oldIrql);

   DPRINT("SpiProcessRequests() done\n");
 }

static VOID
SpiStartIo(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
	   IN PIRP Irp)
{
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  PSCSI_REQUEST_BLOCK Srb;

  DPRINT("SpiStartIo() called!\n");

  assert(KeGetCurrentIrql() == DISPATCH_LEVEL);

  Srb = Irp->Tail.Overlay.DriverContext[3];
  LunExtension = Irp->Tail.Overlay.DriverContext[2];

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = Srb->DataTransferLength;

  SpiAddActiveIrp(DeviceExtension, Irp);

  if (!KeSynchronizeExecution(DeviceExtension->Interrupt,
			      ScsiPortStartPacket,
                              Srb))
    {
      DPRINT1("Synchronization failed!\n");
      DPRINT1("Irp %x, Srb->Function %02x, Srb->Cdb[0] %02x, Srb->SrbStatus %02x\n", Irp, Srb->Function, Srb->Cdb[0], Srb->SrbStatus);
      ScsiPortNotification(RequestComplete,
	                   &DeviceExtension->MiniPortDeviceExtension, 
			   Srb);
    }

  DPRINT("SpiStartIo() done\n");
}



/* EOF */
