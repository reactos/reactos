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
/* $Id: scsiport.c,v 1.48 2004/03/22 19:59:31 navaraf Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/scsiport/scsiport.c
 * PURPOSE:         SCSI port driver
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ddk/srb.h>
#include <ddk/scsi.h>
#include <ddk/ntddscsi.h>
#include <rosrtl/string.h>

//#define NDEBUG
#include <debug.h>


#define VERSION "0.0.1"

#include "scsiport_int.h"

/* #define USE_DEVICE_QUEUES */

/* TYPES *********************************************************************/

#define IRP_FLAG_COMPLETE	0x00000001
#define IRP_FLAG_NEXT		0x00000002
#define IRP_FLAG_NEXT_LU	0x00000004


/* GLOBALS *******************************************************************/

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

static VOID STDCALL
ScsiPortStartIo(IN PDEVICE_OBJECT DeviceObject,
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
		IN OUT PSCSI_REQUEST_BLOCK Srb);

static VOID
SpiScanAdapter (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

static ULONG
SpiGetInquiryData (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		   OUT PSCSI_ADAPTER_BUS_INFO AdapterBusInfo);

static BOOLEAN STDCALL
ScsiPortIsr(IN PKINTERRUPT Interrupt,
	    IN PVOID ServiceContext);

static VOID STDCALL
ScsiPortDpcForIsr(IN PKDPC Dpc,
		  IN PDEVICE_OBJECT DpcDeviceObject,
		  IN PIRP DpcIrp,
		  IN PVOID DpcContext);

static VOID STDCALL
ScsiPortIoTimer(PDEVICE_OBJECT DeviceObject,
		PVOID Context);

static PSCSI_REQUEST_BLOCK
ScsiPortInitSenseRequestSrb(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
			    PSCSI_REQUEST_BLOCK OriginalSrb);

static VOID
ScsiPortFreeSenseRequestSrb(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

static NTSTATUS
SpiBuildDeviceMap (PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		   PUNICODE_STRING RegistryPath);


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
  DPRINT("ScsiPortCompleteRequest()\n");
  UNIMPLEMENTED;
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
			       FALSE);

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

  *Length = 0;

  if (Srb == NULL)
    {
      if ((ULONG_PTR)DeviceExtension->VirtualAddress > (ULONG_PTR)VirtualAddress)
	{
	  PhysicalAddress.QuadPart = 0ULL;
	  return PhysicalAddress;
	}

      Offset = (ULONG_PTR)VirtualAddress - (ULONG_PTR)DeviceExtension->VirtualAddress;
      if (Offset >= DeviceExtension->CommonBufferLength)
	{
	  PhysicalAddress.QuadPart = 0ULL;
	  return PhysicalAddress;
	}

      PhysicalAddress.QuadPart =
	DeviceExtension->PhysicalAddress.QuadPart + (ULONGLONG)Offset;
      BufferLength = DeviceExtension->CommonBufferLength - Offset;
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
	  PhysicalAddress.QuadPart = 0LL;
	  return PhysicalAddress;
	}
      
      PhysicalAddress = MmGetPhysicalAddress(VirtualAddress);
      if (PhysicalAddress.QuadPart == 0LL)
        {
	  return PhysicalAddress;
	}
     
      Offset = (ULONG_PTR)VirtualAddress & (PAGE_SIZE - 1);
#if 1      
      /* 
       * FIXME:
       *   MmGetPhysicalAddress doesn't return the offset within the page.
       *   We must set the correct offset.
       */
      PhysicalAddress.u.LowPart = (PhysicalAddress.u.LowPart & ~(PAGE_SIZE - 1)) + Offset;
#endif
      BufferLength += PAGE_SIZE - Offset;
      while (VirtualAddress + BufferLength < EndAddress)
        {
	  NextPhysicalAddress = MmGetPhysicalAddress(VirtualAddress + BufferLength);
	  if (PhysicalAddress.QuadPart + (ULONGLONG)BufferLength != NextPhysicalAddress.QuadPart)
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

  *Length = BufferLength;

  return PhysicalAddress;
}


/*
 * @unimplemented
 */
PSCSI_REQUEST_BLOCK STDCALL
ScsiPortGetSrb(IN PVOID DeviceExtension,
	       IN UCHAR PathId,
	       IN UCHAR TargetId,
	       IN UCHAR Lun,
	       IN LONG QueueTag)
{
  DPRINT1("ScsiPortGetSrb()\n");
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

  DPRINT1("ScsiPortGetUncachedExtension(%p %p %lu)\n",
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

  /* Allocate a common DMA buffer */
  DeviceExtension->CommonBufferLength =
    NumberOfBytes + DeviceExtension->SrbExtensionSize;
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
                 DeviceExtension->SrbExtensionSize);
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

  if ((HwInitializationData->HwInitialize == NULL) ||
      (HwInitializationData->HwStartIo == NULL) ||
      (HwInitializationData->HwInterrupt == NULL) ||
      (HwInitializationData->HwFindAdapter == NULL) ||
      (HwInitializationData->HwResetBus == NULL))
    return(STATUS_INVALID_PARAMETER);

  DriverObject->DriverStartIo = ScsiPortStartIo;
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
      DeviceExtension->Length = DeviceExtensionSize;
      DeviceExtension->DeviceObject = PortDeviceObject;
      DeviceExtension->PortNumber = SystemConfig->ScsiPortCount;

      DeviceExtension->MiniPortExtensionSize = HwInitializationData->DeviceExtensionSize;
      DeviceExtension->LunExtensionSize = HwInitializationData->SpecificLuExtensionSize;
      DeviceExtension->SrbExtensionSize = HwInitializationData->SrbExtensionSize;
      DeviceExtension->HwStartIo = HwInitializationData->HwStartIo;
      DeviceExtension->HwInterrupt = HwInitializationData->HwInterrupt;

#if 0
      DeviceExtension->AdapterObject = NULL;
      DeviceExtension->MapRegisterCount = 0;
      DeviceExtension->PhysicalAddress.QuadPart = 0ULL;
      DeviceExtension->VirtualAddress = NULL;
      DeviceExtension->CommonBufferLength = 0;
#endif

      /* Initialize the device base list */
      InitializeListHead (&DeviceExtension->DeviceBaseListHead);

      /* Initialize LUN-Extension list */
      InitializeListHead (&DeviceExtension->LunExtensionListHead);

      /* Initialize the spin lock in the controller extension */
      KeInitializeSpinLock (&DeviceExtension->IrpLock);
      KeInitializeSpinLock (&DeviceExtension->SpinLock);

      /* Initialize the DPC object */
      IoInitializeDpcRequest (PortDeviceObject,
			      ScsiPortDpcForIsr);

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
//  PortConfig->DmaWidth =
//  PortConfig->DmaSpeed =
//  PortConfig->AlignmentMask =
      PortConfig->NumberOfAccessRanges = HwInitializationData->NumberOfAccessRanges;
//  PortConfig->NumberOfBuses =

      for (i = 0; i < SCSI_MAXIMUM_BUSES; i++)
	PortConfig->InitiatorBusId[i] = 255;

//  PortConfig->ScatterGather =
//  PortConfig->Master =
//  PortConfig->CachesData =
//  PortConfig->AdapterScansDown =
      PortConfig->AtdiskPrimaryClaimed = SystemConfig->AtDiskPrimaryAddressClaimed;
      PortConfig->AtdiskSecondaryClaimed = SystemConfig->AtDiskSecondaryAddressClaimed;
//  PortConfig->Dma32BitAddresses =
//  PortConfig->DemandMode =
      PortConfig->MapBuffers = HwInitializationData->MapBuffers;
      PortConfig->NeedPhysicalAddresses = HwInitializationData->NeedPhysicalAddresses;
      PortConfig->TaggedQueuing = HwInitializationData->TaggedQueueing;
      PortConfig->AutoRequestSense = HwInitializationData->AutoRequestSense;
      PortConfig->MultipleRequestPerLu = HwInitializationData->MultipleRequestPerLu;
      PortConfig->ReceiveEvent = HwInitializationData->ReceiveEvent;
//  PortConfig->RealModeInitialized =
//  PortConfig->BufferAccessScsiPortControlled =
      PortConfig->MaximumNumberOfTargets = SCSI_MAXIMUM_TARGETS;
//  PortConfig->MaximumNumberOfLogicalUnits = SCSI_MAXIMUM_LOGICAL_UNITS;

      PortConfig->SlotNumber = SlotNumber.u.AsULONG;

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

	  /* Register an interrupt handler for this device */
	  MappedIrq = HalGetInterruptVector(PortConfig->AdapterInterfaceType,
					    PortConfig->SystemIoBusNumber,
					    PortConfig->BusInterruptLevel,
					    PortConfig->BusInterruptVector,
					    &Dirql,
					    &Affinity);
	  Status = IoConnectInterrupt(&DeviceExtension->Interrupt,
				      ScsiPortIsr,
				      DeviceExtension,
				      &DeviceExtension->SpinLock,
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
	  PortCapabilities->MaximumTransferLength =
	    PortConfig->MaximumTransferLength;
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

  return Status;
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
	  DeviceExtension->IrpFlags |= IRP_FLAG_COMPLETE;
	}
	break;

      case NextRequest:
	DPRINT("Notify: NextRequest\n");
	DeviceExtension->IrpFlags |= IRP_FLAG_NEXT;
	break;

      case NextLuRequest:
	{
	  UCHAR PathId;
	  UCHAR TargetId;
	  UCHAR Lun;

	  PathId = (UCHAR) va_arg (ap, int);
	  TargetId = (UCHAR) va_arg (ap, int);
	  Lun = (UCHAR) va_arg (ap, int);

	  DPRINT1 ("Notify: NextLuRequest(PathId %u  TargetId %u  Lun %u)\n",
		   PathId, TargetId, Lun);
	  /* FIXME: Implement it! */

//	  DeviceExtension->IrpFlags |= IRP_FLAG_NEXT_LU;

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
			      PortConfig->AccessRanges[0].RangeLength =
			        -(RangeLength & PCI_ADDRESS_IO_ADDRESS_MASK);
			      PortConfig->AccessRanges[i].RangeInMemory =
				!(PciConfig.u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_SPACE);

			      DPRINT1("RangeStart 0x%lX  RangeLength 0x%lX  RangeInMemory %s\n",
				     PciConfig.u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_ADDRESS_MASK,
				     -(RangeLength & PCI_ADDRESS_IO_ADDRESS_MASK),
				     (PciConfig.u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_SPACE)?"FALSE":"TRUE");
			    }
			}
		    }
		}

	      NextSlotNumber->u.bits.DeviceNumber = DeviceNumber;
	      NextSlotNumber->u.bits.FunctionNumber = FunctionNumber + 1;

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
#ifdef USE_DEVICE_QUEUES
	if (Srb->SrbFlags & SRB_FLAGS_BYPASS_FROZEN_QUEUE)
	  {
	    IoMarkIrpPending(Irp);
	    IoStartPacket (DeviceObject, Irp, NULL, NULL);
	  }
	else
	  {
	    KIRQL oldIrql;

	    KeRaiseIrql (DISPATCH_LEVEL,
			 &oldIrql);

	    if (!KeInsertByKeyDeviceQueue (&LunExtension->DeviceQueue,
					   &Irp->Tail.Overlay.DeviceQueueEntry,
					   Srb->QueueSortKey))
	      {
		Srb->SrbStatus = SRB_STATUS_SUCCESS;
		IoMarkIrpPending(Irp);
		IoStartPacket (DeviceObject, Irp, NULL, NULL);
	      }

	    KeLowerIrql (oldIrql);
	  }
#else
        IoMarkIrpPending(Irp);
        IoStartPacket (DeviceObject, Irp, NULL, NULL);
#endif
	return(STATUS_PENDING);

      case SRB_FUNCTION_SHUTDOWN:
      case SRB_FUNCTION_FLUSH:
	if (DeviceExtension->PortConfig->CachesData == TRUE)
	  {
            IoMarkIrpPending(Irp);
	    IoStartPacket(DeviceObject, Irp, NULL, NULL);
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


static VOID STDCALL
ScsiPortStartIo(IN PDEVICE_OBJECT DeviceObject,
		IN PIRP Irp)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  PIO_STACK_LOCATION IrpStack;
  PSCSI_REQUEST_BLOCK Srb;
  KIRQL oldIrql;

  DPRINT("ScsiPortStartIo() called!\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  DPRINT("DeviceExtension %p\n", DeviceExtension);

  oldIrql = KeGetCurrentIrql();

  if (IrpStack->MajorFunction != IRP_MJ_SCSI)
    {
      DPRINT("No IRP_MJ_SCSI!\n");
      Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest (Irp,
			 IO_NO_INCREMENT);
      if (oldIrql < DISPATCH_LEVEL)
	{
	  KeRaiseIrql (DISPATCH_LEVEL,
		       &oldIrql);
	  IoStartNextPacket (DeviceObject,
			     FALSE);
	  KeLowerIrql (oldIrql);
	}
      else
	{
	  IoStartNextPacket (DeviceObject,
			     FALSE);
	}
      return;
    }

  Srb = IrpStack->Parameters.Scsi.Srb;

  LunExtension = SpiGetLunExtension(DeviceExtension,
				    Srb->PathId,
				    Srb->TargetId,
				    Srb->Lun);
  if (LunExtension == NULL)
    {
      DPRINT("Can't get LunExtension!\n");
      Irp->IoStatus.Status = STATUS_NO_SUCH_DEVICE;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest (Irp,
			 IO_NO_INCREMENT);
      if (oldIrql < DISPATCH_LEVEL)
	{
	  KeRaiseIrql (DISPATCH_LEVEL,
		       &oldIrql);
	  IoStartNextPacket (DeviceObject,
			     FALSE);
	  KeLowerIrql (oldIrql);
	}
      else
	{
	  IoStartNextPacket (DeviceObject,
			     FALSE);
	}
      return;
    }

  Irp->IoStatus.Status = STATUS_SUCCESS;
  Irp->IoStatus.Information = Srb->DataTransferLength;

  /* Allocte SRB extension */
  if (DeviceExtension->SrbExtensionSize != 0)
    {
      Srb->SrbExtension = DeviceExtension->VirtualAddress;
    }

  DeviceExtension->CurrentIrp = Irp;

  if (!KeSynchronizeExecution(DeviceExtension->Interrupt,
			      ScsiPortStartPacket,
			      DeviceExtension))
    {
      DPRINT("Synchronization failed!\n");

      Irp->IoStatus.Status = STATUS_UNSUCCESSFUL;
      Irp->IoStatus.Information = 0;
      IoCompleteRequest(Irp,
			IO_NO_INCREMENT);
      if (oldIrql < DISPATCH_LEVEL)
	{
	  KeRaiseIrql (DISPATCH_LEVEL,
		       &oldIrql);
	  IoStartNextPacket (DeviceObject,
			     FALSE);
	  KeLowerIrql (oldIrql);
	}
      else
	{
	  IoStartNextPacket (DeviceObject,
			     FALSE);
	}
    }

  KeAcquireSpinLock(&DeviceExtension->IrpLock, &oldIrql);
  if (DeviceExtension->IrpFlags & IRP_FLAG_COMPLETE)
    {
      DeviceExtension->IrpFlags &= ~IRP_FLAG_COMPLETE;
      IoCompleteRequest(Irp,
			IO_NO_INCREMENT);
    }

  if (DeviceExtension->IrpFlags & IRP_FLAG_NEXT)
    {
      DeviceExtension->IrpFlags &= ~IRP_FLAG_NEXT;
      KeReleaseSpinLockFromDpcLevel(&DeviceExtension->IrpLock);
      IoStartNextPacket(DeviceObject,
			FALSE);
      KeLowerIrql(oldIrql);
    }
  else
    {
      KeReleaseSpinLock(&DeviceExtension->IrpLock, oldIrql);
    }

  DPRINT("ScsiPortStartIo() done\n");
}


static BOOLEAN STDCALL
ScsiPortStartPacket(IN OUT PVOID Context)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpStack;
  PSCSI_REQUEST_BLOCK Srb;

  DPRINT("ScsiPortStartPacket() called\n");

  DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)Context;

  IrpStack = IoGetCurrentIrpStackLocation(DeviceExtension->CurrentIrp);
  Srb = IrpStack->Parameters.Scsi.Srb;

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

  KeInitializeDeviceQueue (&LunExtension->DeviceQueue);

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
		IN OUT PSCSI_REQUEST_BLOCK Srb)
{
  IO_STATUS_BLOCK IoStatusBlock;
  PIO_STACK_LOCATION IrpStack;
  PKEVENT Event;
  PIRP Irp;
  NTSTATUS Status;

  DPRINT ("SpiSendInquiry() called\n");

  Event = ExAllocatePool (NonPagedPool,
			  sizeof(KEVENT));
  if (Event == NULL)
    return STATUS_INSUFFICIENT_RESOURCES;

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
				       &IoStatusBlock);
  if (Irp == NULL)
    {
      DPRINT("IoBuildDeviceIoControlRequest() failed\n");
      ExFreePool (Event);
      return STATUS_INSUFFICIENT_RESOURCES;
    }

  /* Attach Srb to the Irp */
  IrpStack = IoGetNextIrpStackLocation (Irp);
  IrpStack->Parameters.Scsi.Srb = Srb;
  Srb->OriginalRequest = Irp;

  /* Call the driver */
  Status = IoCallDriver (DeviceObject,
			 Irp);
  if (Status == STATUS_PENDING)
    {
      KeWaitForSingleObject (Event,
			     Suspended,
			     KernelMode,
			     FALSE,
			     NULL);
      Status = IoStatusBlock.Status;
    }

  ExFreePool (Event);

  return Status;
}


static VOID
SpiScanAdapter (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  SCSI_REQUEST_BLOCK Srb;
  PCDB Cdb;
  ULONG Bus;
  ULONG Target;
  ULONG Lun;
  NTSTATUS Status;

  DPRINT ("SpiScanAdapter() called\n");

  RtlZeroMemory(&Srb,
		sizeof(SCSI_REQUEST_BLOCK));
  Srb.SrbFlags = SRB_FLAGS_DATA_IN;
  Srb.DataBuffer = ExAllocatePool(NonPagedPool, 256);
  Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
  Srb.DataTransferLength = 256;
  Srb.CdbLength = 6;

  Cdb = (PCDB) &Srb.Cdb;

  Cdb->CDB6INQUIRY.OperationCode = SCSIOP_INQUIRY;

  for (Bus = 0; Bus < DeviceExtension->PortConfig->NumberOfBuses; Bus++)
    {
      Srb.PathId = Bus;

      for (Target = 0; Target < DeviceExtension->PortConfig->MaximumNumberOfTargets; Target++)
	{
	  Srb.TargetId = Target;

	  for (Lun = 0; Lun < SCSI_MAXIMUM_LOGICAL_UNITS; Lun++)
	    {
	      Srb.Lun = Lun;
	      Srb.SrbStatus = SRB_STATUS_SUCCESS;

	      Cdb->CDB6INQUIRY.LogicalUnitNumber = Lun;

	      LunExtension = SpiAllocateLunExtension (DeviceExtension,
						      Bus,
						      Target,
						      Lun);
	      if (LunExtension == NULL)
		{
		  DPRINT("Failed to allocate the LUN extension!\n");
		  ExFreePool(Srb.DataBuffer);
		  return;
		}

	      Status = SpiSendInquiry (DeviceExtension->DeviceObject,
				       &Srb);
	      DPRINT("Status %lx  Srb.SrbStatus %lx\n", Status, Srb.SrbStatus);

	      if (NT_SUCCESS(Status) && Srb.SrbStatus == SRB_STATUS_SUCCESS)
		{
		  /* Copy inquiry data */
		  RtlCopyMemory (&LunExtension->InquiryData,
				 Srb.DataBuffer,
				 sizeof(INQUIRYDATA));
		}
	      else
		{
		  SpiRemoveLunExtension (LunExtension);
		}
	    }
	}
    }

  ExFreePool(Srb.DataBuffer);

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
  BOOLEAN Result;

  DPRINT("ScsiPortIsr() called!\n");

  DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)ServiceContext;

  Result = DeviceExtension->HwInterrupt(&DeviceExtension->MiniPortDeviceExtension);
  if (Result == FALSE)
    {
      return(FALSE);
    }

  if (DeviceExtension->IrpFlags)
    {
      IoRequestDpc(DeviceExtension->DeviceObject,
		   DeviceExtension->CurrentIrp,
		   DeviceExtension);
    }

  return(TRUE);
}


//    ScsiPortDpcForIsr
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
ScsiPortDpcForIsr(IN PKDPC Dpc,
		  IN PDEVICE_OBJECT DpcDeviceObject,
		  IN PIRP DpcIrp,
		  IN PVOID DpcContext)
{
  PSCSI_PORT_DEVICE_EXTENSION DeviceExtension;
  PIO_STACK_LOCATION IrpStack;
  PSCSI_REQUEST_BLOCK Srb;

  DPRINT("ScsiPortDpcForIsr(Dpc %p  DpcDeviceObject %p  DpcIrp %p  DpcContext %p)\n",
	 Dpc, DpcDeviceObject, DpcIrp, DpcContext);

  DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)DpcContext;

  KeAcquireSpinLockAtDpcLevel(&DeviceExtension->IrpLock);
  if (DeviceExtension->IrpFlags)
    {
      IrpStack = IoGetCurrentIrpStackLocation(DeviceExtension->CurrentIrp);
      Srb = IrpStack->Parameters.Scsi.Srb;

      if (DeviceExtension->OriginalSrb != NULL)
	{
	  DPRINT("Got sense data!\n");

	  DPRINT("Valid: %x\n", DeviceExtension->InternalSenseData.Valid);
	  DPRINT("ErrorCode: %x\n", DeviceExtension->InternalSenseData.ErrorCode);
	  DPRINT("SenseKey: %x\n", DeviceExtension->InternalSenseData.SenseKey);
	  DPRINT("SenseCode: %x\n", DeviceExtension->InternalSenseData.AdditionalSenseCode);

	  /* Copy sense data */
	  if (DeviceExtension->OriginalSrb->SenseInfoBufferLength != 0)
	    {
	      RtlCopyMemory(DeviceExtension->OriginalSrb->SenseInfoBuffer,
			    &DeviceExtension->InternalSenseData,
			    sizeof(SENSE_DATA));
	      DeviceExtension->OriginalSrb->SrbStatus |= SRB_STATUS_AUTOSENSE_VALID;
	    }

	  /* Clear current sense data */
	  RtlZeroMemory(&DeviceExtension->InternalSenseData, sizeof(SENSE_DATA));

	  IrpStack->Parameters.Scsi.Srb = DeviceExtension->OriginalSrb;
	  ScsiPortFreeSenseRequestSrb (DeviceExtension);
	}
      else if ((SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS) &&
	       (Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION))
	{
	  DPRINT("SCSIOP_REQUEST_SENSE required!\n");

	  DeviceExtension->OriginalSrb = Srb;
	  IrpStack->Parameters.Scsi.Srb = ScsiPortInitSenseRequestSrb(DeviceExtension,
								      Srb);
	  KeReleaseSpinLockFromDpcLevel(&DeviceExtension->IrpLock);
	  if (!KeSynchronizeExecution(DeviceExtension->Interrupt,
				      ScsiPortStartPacket,
				      DeviceExtension))
	    {
	      DPRINT1("Synchronization failed!\n");

	      DpcIrp->IoStatus.Status = STATUS_UNSUCCESSFUL;
	      DpcIrp->IoStatus.Information = 0;
	      IoCompleteRequest(DpcIrp,
				IO_NO_INCREMENT);
	      IoStartNextPacket(DpcDeviceObject,
				FALSE);
	    }

	  return;
	}

      DeviceExtension->CurrentIrp = NULL;

//      DpcIrp->IoStatus.Information = 0;
//      DpcIrp->IoStatus.Status = STATUS_SUCCESS;

      if (DeviceExtension->IrpFlags & IRP_FLAG_COMPLETE)
	{
	  DeviceExtension->IrpFlags &= ~IRP_FLAG_COMPLETE;
	  IoCompleteRequest(DpcIrp, IO_NO_INCREMENT);
	}

      if (DeviceExtension->IrpFlags & IRP_FLAG_NEXT)
	{
	  DeviceExtension->IrpFlags &= ~IRP_FLAG_NEXT;
	  KeReleaseSpinLockFromDpcLevel(&DeviceExtension->IrpLock);
	  IoStartNextPacket(DpcDeviceObject, FALSE);
	}
      else
	{
	  KeReleaseSpinLockFromDpcLevel(&DeviceExtension->IrpLock);
	}
    }
  else
    {
      KeReleaseSpinLockFromDpcLevel(&DeviceExtension->IrpLock);
    }

  DPRINT("ScsiPortDpcForIsr() done\n");
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
ScsiPortInitSenseRequestSrb(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
			    PSCSI_REQUEST_BLOCK OriginalSrb)
{
  PSCSI_REQUEST_BLOCK Srb;
  PCDB Cdb;

  Srb = &DeviceExtension->InternalSrb;

  RtlZeroMemory(Srb,
		sizeof(SCSI_REQUEST_BLOCK));

  Srb->PathId = OriginalSrb->PathId;
  Srb->TargetId = OriginalSrb->TargetId;
  Srb->Function = SRB_FUNCTION_EXECUTE_SCSI;
  Srb->Length = sizeof(SCSI_REQUEST_BLOCK);
  Srb->SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;

  Srb->TimeOutValue = 4;

  Srb->CdbLength = 6;
  Srb->DataBuffer = &DeviceExtension->InternalSenseData;
  Srb->DataTransferLength = sizeof(SENSE_DATA);

  Cdb = (PCDB)Srb->Cdb;
  Cdb->CDB6INQUIRY.OperationCode = SCSIOP_REQUEST_SENSE;
  Cdb->CDB6INQUIRY.AllocationLength = sizeof(SENSE_DATA);

  return(Srb);
}


static VOID
ScsiPortFreeSenseRequestSrb(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
  DeviceExtension->OriginalSrb = NULL;
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

/* EOF */
