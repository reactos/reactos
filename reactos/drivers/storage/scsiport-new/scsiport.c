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
/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/scsiport/scsiport.c
 * PURPOSE:         SCSI port driver
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

/* INCLUDES *****************************************************************/

#include <ntddk.h>
#include <srb.h>
#include <scsi.h>
#include <ntddscsi.h>
#include <ntddstor.h>
#include <ntdddisk.h>
#include <stdio.h>
#include <stdarg.h>

//#define NDEBUG
#include <debug.h>

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
SpiAllocateLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

static PSCSI_PORT_LUN_EXTENSION
SpiGetLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		    IN UCHAR PathId,
		    IN UCHAR TargetId,
		    IN UCHAR Lun);

static NTSTATUS
SpiSendInquiry (IN PDEVICE_OBJECT DeviceObject,
		IN PSCSI_LUN_INFO LunInfo);

static VOID
SpiScanAdapter (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

static NTSTATUS
SpiGetInquiryData (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		   IN PIRP Irp);

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

  //DPRINT("ScsiPortFreeDeviceBase() called\n");

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

  //DPRINT ("ScsiPortGetDeviceBase() called\n");

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
    UNIMPLEMENTED;
#if 0
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
#endif
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
      EndAddress = (PVOID)((ULONG_PTR)Srb->DataBuffer + Srb->DataTransferLength);
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
      while ((ULONG_PTR)VirtualAddress + BufferLength < (ULONG_PTR)EndAddress)
	{
	  NextPhysicalAddress = MmGetPhysicalAddress((PVOID)((ULONG_PTR)VirtualAddress + BufferLength));
	  if (PhysicalAddress.QuadPart + (ULONGLONG)BufferLength != NextPhysicalAddress.QuadPart)
	    {
	      break;
	    }
	  BufferLength += PAGE_SIZE;
	}
      if ((ULONG_PTR)VirtualAddress + BufferLength >= (ULONG_PTR)EndAddress)
	{
	  BufferLength = (ULONG_PTR)EndAddress - (ULONG_PTR)VirtualAddress;
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
    PortConfig->NumberOfBuses = MaxBus;

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
      PortConfig->TaggedQueuing = HwInitializationData->TaggedQueuing;
      PortConfig->AutoRequestSense = HwInitializationData->AutoRequestSense;
      PortConfig->MultipleRequestPerLu = HwInitializationData->MultipleRequestPerLu;
      PortConfig->ReceiveEvent = HwInitializationData->ReceiveEvent;
//  PortConfig->RealModeInitialized =
//  PortConfig->BufferAccessScsiPortControlled =
      PortConfig->MaximumNumberOfTargets = SCSI_MAXIMUM_TARGETS;
//  PortConfig->MaximumNumberOfLogicalUnits = SCSI_MAXIMUM_LOGICAL_UNITS;

      PortConfig->SlotNumber = SlotNumber.u.AsULONG;

      PortConfig->AccessRanges = (ACCESS_RANGE(*)[])(PortConfig + 1);

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

          /* Initialize bus scanning information */
          DeviceExtension->BusesConfig = ExAllocatePool(PagedPool,
              sizeof(PSCSI_BUS_SCAN_INFO) * DeviceExtension->PortConfig->NumberOfBuses
              + sizeof(ULONG));

          if (!DeviceExtension->BusesConfig)
          {
              DPRINT1("Out of resources!\n");
              Status = STATUS_INSUFFICIENT_RESOURCES;
              goto ByeBye;
          }

          /* Zero it */
          RtlZeroMemory(DeviceExtension->BusesConfig,
              sizeof(PSCSI_BUS_SCAN_INFO) * DeviceExtension->PortConfig->NumberOfBuses
              + sizeof(ULONG));

          /* Store number of buses there */
          DeviceExtension->BusesConfig->NumberOfBuses = DeviceExtension->PortConfig->NumberOfBuses;

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
		      IN PVOID LogicalAddress,
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

	  DeviceExtension->IrpFlags |= IRP_FLAG_NEXT;
//	  DeviceExtension->IrpFlags |= IRP_FLAG_NEXT_LU;

	  /* Hack! */
	  DeviceExtension->IrpFlags |= IRP_FLAG_NEXT;
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
		      (*PortConfig->AccessRanges)[i].RangeStart.QuadPart =
			PciConfig.u.type0.BaseAddresses[i] & PCI_ADDRESS_IO_ADDRESS_MASK;
		      if ((*PortConfig->AccessRanges)[i].RangeStart.QuadPart != 0)
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
			      (*PortConfig->AccessRanges)[i].RangeLength =
			        -(RangeLength & PCI_ADDRESS_IO_ADDRESS_MASK);
			      (*PortConfig->AccessRanges)[i].RangeInMemory =
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
	  Irp->IoStatus.Status = SpiGetInquiryData(DeviceExtension, Irp);
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

  /* Allocte SRB extension */
  if (DeviceExtension->SrbExtensionSize != 0)
    {
      Srb->SrbExtension = DeviceExtension->VirtualAddress;
    }

  return(DeviceExtension->HwStartIo(&DeviceExtension->MiniPortDeviceExtension,
				    Srb));
}


static PSCSI_PORT_LUN_EXTENSION
SpiAllocateLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    ULONG LunExtensionSize;

    DPRINT("SpiAllocateLunExtension (%p)\n",
        DeviceExtension);

    /* Round LunExtensionSize first to the sizeof LONGLONG */
    LunExtensionSize = (DeviceExtension->LunExtensionSize +
        sizeof(LONGLONG) - 1) & ~(sizeof(LONGLONG) - 1);

    LunExtensionSize += sizeof(SCSI_PORT_LUN_EXTENSION);
    DPRINT("LunExtensionSize %lu\n", LunExtensionSize);

    LunExtension = ExAllocatePool(NonPagedPool, LunExtensionSize);
    if (LunExtension == NULL)
    {
        DPRINT1("Out of resources!\n");
        return NULL;
    }

    /* Zero everything */
    RtlZeroMemory(LunExtension, LunExtensionSize);

    /* Initialize a list of requests */
    InitializeListHead(&LunExtension->SrbInfo.Requests);

    /* TODO: Initialize other fields */

    /* Initialize request queue */
    KeInitializeDeviceQueue (&LunExtension->DeviceQueue);

    return LunExtension;
}

static PSCSI_PORT_LUN_EXTENSION
SpiGetLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		    IN UCHAR PathId,
		    IN UCHAR TargetId,
		    IN UCHAR Lun)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;

    DPRINT("SpiGetLunExtension(%p %u %u %u) called\n",
        DeviceExtension, PathId, TargetId, Lun);

    /* Get appropriate list */
    LunExtension = DeviceExtension->LunExtensionList[(TargetId + Lun) % LUS_NUMBER];

    /* Iterate it until we find what we need */
    while (!LunExtension)
    {
        if (LunExtension->TargetId == TargetId &&
            LunExtension->Lun == Lun &&
            LunExtension->PathId == PathId)
        {
            /* All matches, return */
            return LunExtension;
        }

        /* Advance to the next item */
        LunExtension = LunExtension->Next;
    }

    /* We did not find anything */
    DPRINT("Nothing found\n");
    return NULL;
}


static NTSTATUS
SpiSendInquiry (IN PDEVICE_OBJECT DeviceObject,
		IN PSCSI_LUN_INFO LunInfo)
{
    IO_STATUS_BLOCK IoStatusBlock;
    PIO_STACK_LOCATION IrpStack;
    KEVENT Event;
    PIRP Irp;
    NTSTATUS Status;
    PINQUIRYDATA InquiryBuffer;
    PSENSE_DATA SenseBuffer;
    BOOLEAN KeepTrying = TRUE;
    ULONG RetryCount = 0;
    SCSI_REQUEST_BLOCK Srb;
    PCDB Cdb;

    DPRINT ("SpiSendInquiry() called\n");

    InquiryBuffer = ExAllocatePool (NonPagedPool, INQUIRYDATABUFFERSIZE);
    if (InquiryBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    SenseBuffer = ExAllocatePool (NonPagedPool, SENSE_BUFFER_SIZE);
    if (SenseBuffer == NULL)
        return STATUS_INSUFFICIENT_RESOURCES;

    while (KeepTrying)
    {
        /* Initialize event for waiting */
        KeInitializeEvent(&Event,
                          NotificationEvent,
                          FALSE);

        /* Create an IRP */
        Irp = IoBuildDeviceIoControlRequest(IOCTL_SCSI_EXECUTE_IN,
            DeviceObject,
            NULL,
            0,
            InquiryBuffer,
            INQUIRYDATABUFFERSIZE,
            TRUE,
            &Event,
            &IoStatusBlock);
        if (Irp == NULL)
        {
            DPRINT("IoBuildDeviceIoControlRequest() failed\n");
            return STATUS_INSUFFICIENT_RESOURCES;
        }

        /* Prepare SRB */
        RtlZeroMemory(&Srb, sizeof(SCSI_REQUEST_BLOCK));

        Srb.Length = sizeof(SCSI_REQUEST_BLOCK);
        Srb.OriginalRequest = Irp;
        Srb.PathId = LunInfo->PathId;
        Srb.TargetId = LunInfo->TargetId;
        Srb.Lun = LunInfo->Lun;
        Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
        Srb.SrbFlags = SRB_FLAGS_DATA_IN | SRB_FLAGS_DISABLE_SYNCH_TRANSFER;
        Srb.TimeOutValue = 4;
        Srb.CdbLength = 6;

        Srb.SenseInfoBuffer = SenseBuffer;
        Srb.SenseInfoBufferLength = SENSE_BUFFER_SIZE;

        Srb.DataBuffer = InquiryBuffer;
        Srb.DataTransferLength = INQUIRYDATABUFFERSIZE;

        /* Attach Srb to the Irp */
        IrpStack = IoGetNextIrpStackLocation (Irp);
        IrpStack->Parameters.Scsi.Srb = &Srb;

        /* Fill in CDB */
        Cdb = (PCDB)Srb.Cdb;
        Cdb->CDB6INQUIRY.LogicalUnitNumber = LunInfo->Lun;
        Cdb->CDB6INQUIRY.AllocationLength = INQUIRYDATABUFFERSIZE;

        /* Call the driver */
        Status = IoCallDriver(DeviceObject, Irp);

        /* Wait for it to complete */
        if (Status == STATUS_PENDING)
        {
            KeWaitForSingleObject(&Event,
                Executive,
                KernelMode,
                FALSE,
                NULL);
            Status = IoStatusBlock.Status;
        }

        if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_SUCCESS)
        {
            /* All fine, copy data over */
            RtlCopyMemory(LunInfo->InquiryData,
                          InquiryBuffer,
                          INQUIRYDATABUFFERSIZE);

            Status = STATUS_SUCCESS;
        }
        else
        {
            /* Check if queue is frozen */
            if (Srb.SrbStatus & SRB_STATUS_QUEUE_FROZEN)
            {
                /* Something weird happeend */
                ASSERT(FALSE);
            }

            /* Check if data overrun happened */
            if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_DATA_OVERRUN)
            {
                /* TODO: Implement */
                ASSERT(FALSE);
            }
            else if ((Srb.SrbStatus & SRB_STATUS_AUTOSENSE_VALID) &&
                SenseBuffer->SenseKey == SCSI_SENSE_ILLEGAL_REQUEST)
            {
                /* LUN is not valid, but some device responds there.
                   Mark it as invalid anyway */

                Status = STATUS_INVALID_DEVICE_REQUEST;
            }
            else
            {
                /* Retry a couple of times if no timeout happened */
                if ((RetryCount < 2) &&
                    (SRB_STATUS(Srb.SrbStatus) != SRB_STATUS_NO_DEVICE) &&
                    (SRB_STATUS(Srb.SrbStatus) != SRB_STATUS_SELECTION_TIMEOUT))
                {
                    RetryCount++;
                    KeepTrying = TRUE;
                }
                else
                {
                    /* That's all, go to exit */
                    KeepTrying = FALSE;

                    /* Set status according to SRB status */
                    if (SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_BAD_FUNCTION ||
                        SRB_STATUS(Srb.SrbStatus) == SRB_STATUS_BAD_SRB_BLOCK_LENGTH)
                    {
                          Status = STATUS_INVALID_DEVICE_REQUEST;
                    }
                    else
                    {
                        Status = STATUS_IO_DEVICE_ERROR;
                    }
                }
            }
        }
    }

    /* Free buffers */
    ExFreePool(InquiryBuffer);
    ExFreePool(SenseBuffer);

    return Status;
}


/* Scans all SCSI buses */
static VOID
SpiScanAdapter(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
    PSCSI_PORT_LUN_EXTENSION LunExtension;
    ULONG Bus;
    ULONG Target;
    ULONG Lun;
    PSCSI_BUS_SCAN_INFO BusScanInfo;
    PSCSI_LUN_INFO LastLunInfo, LunInfo, LunInfoExists;
    BOOLEAN DeviceExists;
    ULONG Hint;
    NTSTATUS Status;
    ULONG DevicesFound;

    DPRINT ("SpiScanAdapter() called\n");

    /* Scan all buses */
    for (Bus = 0; Bus < DeviceExtension->PortConfig->NumberOfBuses; Bus++)
    {
        DevicesFound = 0;

        /* Get pointer to the scan information */
        BusScanInfo = DeviceExtension->BusesConfig->BusScanInfo[Bus];

        if (BusScanInfo)
        {
            /* Find the last LUN info in the list */
            LunInfo = DeviceExtension->BusesConfig->BusScanInfo[Bus]->LunInfo;
            LastLunInfo = LunInfo;

            while (LunInfo != NULL)
            {
                LastLunInfo = LunInfo;
                LunInfo = LunInfo->Next;
            }
        }
        else
        {
            /* We need to allocate this buffer */
            BusScanInfo = ExAllocatePool(NonPagedPool, sizeof(SCSI_BUS_SCAN_INFO));

            if (!BusScanInfo)
            {
                DPRINT1("Out of resources!\n");
                return;
            }

            /* Fill this struct (length and bus ids for now) */
            BusScanInfo->Length = sizeof(SCSI_BUS_SCAN_INFO);
            BusScanInfo->LogicalUnitsCount = 0;
            BusScanInfo->BusIdentifier = DeviceExtension->PortConfig->InitiatorBusId[Bus];
            BusScanInfo->LunInfo = NULL;

            /* Set pointer to the last LUN info to NULL */
            LastLunInfo = NULL;
        }

        /* Create LUN information structure */
        LunInfo = ExAllocatePool(PagedPool, sizeof(SCSI_LUN_INFO));

        if (LunInfo == NULL)
        {
            DPRINT1("Out of resources!\n");
            return;
        }

        RtlZeroMemory(LunInfo, sizeof(SCSI_LUN_INFO));

        /* Create LunExtension */
        LunExtension = SpiAllocateLunExtension (DeviceExtension);

        /* And send INQUIRY to every target */
        for (Target = 0; Target < DeviceExtension->PortConfig->MaximumNumberOfTargets; Target++)
        {
            /* TODO: Support scan bottom-up */

            /* Skip if it's the same address */
            if (Target == BusScanInfo->BusIdentifier)
                continue;

            /* Try to find an existing device here */
            DeviceExists = FALSE;
            LunInfoExists = BusScanInfo->LunInfo;

            /* Find matching address on this bus */
            while (LunInfoExists)
            {
                if (LunInfoExists->TargetId == Target)
                {
                    DeviceExists = TRUE;
                    break;
                }

                /* Advance to the next one */
                LunInfoExists = LunInfoExists->Next;
            }

            /* No need to bother rescanning, since we already did that before */
            if (DeviceExists)
                continue;

            /* Scan all logical units */
            for (Lun = 0; Lun < SCSI_MAXIMUM_LOGICAL_UNITS; Lun++)
            {
                if (!LunExtension)
                    break;

                /* Add extension to the list */
                Hint = (Target + Lun) % LUS_NUMBER;
                LunExtension->Next = DeviceExtension->LunExtensionList[Hint];
                DeviceExtension->LunExtensionList[Hint] = LunExtension;

                /* Fill Path, Target, Lun fields */
                LunExtension->PathId = LunInfo->PathId = Bus;
                LunExtension->TargetId = LunInfo->TargetId = Target;
                LunExtension->Lun = LunInfo->Lun = Lun;

                /* Set flag to prevent race conditions */
                LunExtension->Flags |= SCSI_PORT_SCAN_IN_PROGRESS;

                /* Zero LU extension contents */
                if (DeviceExtension->LunExtensionSize)
                {
                    RtlZeroMemory(LunExtension + 1,
                                  DeviceExtension->LunExtensionSize);
                }

                /* Finally send the inquiry command */
                Status = SpiSendInquiry(DeviceExtension->DeviceObject, LunInfo);

                if (NT_SUCCESS(Status))
                {
                    /* Let's see if we really found a device */
                    PINQUIRYDATA InquiryData = (PINQUIRYDATA)LunInfo->InquiryData;

                    /* Check if this device is unsupported */
                    if (InquiryData->DeviceTypeQualifier == DEVICE_QUALIFIER_NOT_SUPPORTED)
                    {
                        DeviceExtension->LunExtensionList[Hint] = 
                            DeviceExtension->LunExtensionList[Hint]->Next;

                        continue;
                    }

                    /* Clear the "in scan" flag */
                    LunExtension->Flags &= ~SCSI_PORT_SCAN_IN_PROGRESS;

                    DPRINT("SpiScanAdapter(): Found device of type %d at bus %d tid %d lun %d\n",
                        InquiryData->DeviceType, Bus, Target, Lun);

                    /* Add this info to the linked list */
                    LunInfo->Next = NULL;
                    if (LastLunInfo)
                        LastLunInfo->Next = LunInfo;
                    else
                        BusScanInfo->LunInfo = LunInfo;

                    /* Store the last LUN info */
                    LastLunInfo = LunInfo;

                    /* Store DeviceObject */
                    LunInfo->DeviceObject = DeviceExtension->DeviceObject;

                    /* Allocate another buffer */
                    LunInfo = ExAllocatePool(PagedPool, sizeof(SCSI_LUN_INFO));

                    if (!LunInfo)
                    {
                        DPRINT1("Out of resources!\n");
                        break;
                    }

                    RtlZeroMemory(LunInfo, sizeof(SCSI_LUN_INFO));

                    /* Create a new LU extension */
                    LunExtension = SpiAllocateLunExtension(DeviceExtension);

                    DevicesFound++;
                }
                else
                {
                    /* Remove this LUN from the list */
                    DeviceExtension->LunExtensionList[Hint] = 
                        DeviceExtension->LunExtensionList[Hint]->Next;

                    /* Decide whether we are continuing or not */
                    if (Status == STATUS_INVALID_DEVICE_REQUEST)
                        continue;
                    else
                        break;
                }
            }
        }

        /* Free allocated buffers */
        if (LunExtension)
            ExFreePool(LunExtension);

        if (LunInfo)
            ExFreePool(LunInfo);

        /* Sum what we found */
        BusScanInfo->LogicalUnitsCount += DevicesFound;
    }

    DPRINT ("SpiScanAdapter() done\n");
}


static NTSTATUS
SpiGetInquiryData(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		  PIRP Irp)
{
    ULONG InquiryDataSize;
    PSCSI_LUN_INFO LunInfo;
    ULONG BusCount, LunCount, Length;
    PIO_STACK_LOCATION IrpStack;
    PSCSI_ADAPTER_BUS_INFO AdapterBusInfo;
    PSCSI_INQUIRY_DATA InquiryData;
    PSCSI_BUS_DATA BusData;
    ULONG Bus;
    PUCHAR Buffer;

    DPRINT("SpiGetInquiryData() called\n");

    /* Get pointer to the buffer */
    IrpStack = IoGetCurrentIrpStackLocation(Irp);
    Buffer = Irp->AssociatedIrp.SystemBuffer;

    /* Initialize bus and LUN counters */
    BusCount = DeviceExtension->BusesConfig->NumberOfBuses;
    LunCount = 0;

    /* Calculate total number of LUNs */
    for (Bus = 0; Bus < BusCount; Bus++)
        LunCount += DeviceExtension->BusesConfig->BusScanInfo[Bus]->LogicalUnitsCount;

    /* Calculate size of inquiry data, rounding up to sizeof(ULONG) */
    InquiryDataSize =
        ((sizeof(SCSI_INQUIRY_DATA) - 1 + INQUIRYDATABUFFERSIZE +
        sizeof(ULONG) - 1) & ~(sizeof(ULONG) - 1));

    /* Calculate data size */
    Length = sizeof(SCSI_ADAPTER_BUS_INFO) + (BusCount - 1) *
        sizeof(SCSI_BUS_DATA);
    
    Length += InquiryDataSize * LunCount;

    /* Check, if all data is going to fit into provided buffer */
    if (IrpStack->Parameters.DeviceIoControl.OutputBufferLength < Length)
    {
        Irp->IoStatus.Status = STATUS_BUFFER_TOO_SMALL;
        return STATUS_BUFFER_TOO_SMALL;
    }

    /* Store data size in the IRP */
    Irp->IoStatus.Information = Length;

    DPRINT("Data size: %lu\n", Length);

    AdapterBusInfo = (PSCSI_ADAPTER_BUS_INFO)Buffer;

    AdapterBusInfo->NumberOfBuses = (UCHAR)BusCount;

    /* Point InquiryData to the corresponding place inside Buffer */
    InquiryData = (PSCSI_INQUIRY_DATA)(Buffer + sizeof(SCSI_ADAPTER_BUS_INFO) +
        (BusCount - 1) * sizeof(SCSI_BUS_DATA));

    /* Loop each bus */
    for (Bus = 0; Bus < BusCount; Bus++)
    {
        BusData = &AdapterBusInfo->BusData[Bus];

        /* Calculate and save an offset of the inquiry data */
        BusData->InquiryDataOffset = (PUCHAR)InquiryData - Buffer;

        /* Get a pointer to the LUN information structure */
        LunInfo = DeviceExtension->BusesConfig->BusScanInfo[Bus]->LunInfo;

        /* Store Initiator Bus Id */
        BusData->InitiatorBusId =
            DeviceExtension->BusesConfig->BusScanInfo[Bus]->BusIdentifier;

        /* Store LUN count */
        BusData->NumberOfLogicalUnits =
            DeviceExtension->BusesConfig->BusScanInfo[Bus]->LogicalUnitsCount;

        /* Loop all LUNs */
        while (LunInfo != NULL)
        {
            DPRINT("(Bus %lu Target %lu Lun %lu)\n",
                Bus, LunInfo->TargetId, LunInfo->Lun);

            /* Fill InquiryData with values */
            InquiryData->PathId = LunInfo->PathId;
            InquiryData->TargetId = LunInfo->TargetId;
            InquiryData->Lun = LunInfo->Lun;
            InquiryData->InquiryDataLength = INQUIRYDATABUFFERSIZE;
            InquiryData->DeviceClaimed = LunInfo->DeviceClaimed;
            InquiryData->NextInquiryDataOffset =
                (PUCHAR)InquiryData + InquiryDataSize - Buffer;

            /* Copy data in it */
            RtlCopyMemory(InquiryData->InquiryData,
                          LunInfo->InquiryData,
                          INQUIRYDATABUFFERSIZE);

            /* Move to the next LUN */
            LunInfo = LunInfo->Next;
            InquiryData = (PSCSI_INQUIRY_DATA) ((PCHAR)InquiryData + InquiryDataSize);
        }

        /* Either mark the end, or set offset to 0 */
        if (BusData->NumberOfLogicalUnits != 0)
            ((PSCSI_INQUIRY_DATA) ((PCHAR) InquiryData - InquiryDataSize))->NextInquiryDataOffset = 0;
        else
            BusData->InquiryDataOffset = 0;
    }

    /* Finish with success */
    Irp->IoStatus.Status = STATUS_SUCCESS;
    return STATUS_SUCCESS;
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
  RtlInitUnicodeString(&KeyName,
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
  UlongData = ScsiPortConvertPhysicalAddressToUlong((*DeviceExtension->PortConfig->AccessRanges)[0].RangeStart);
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


#undef ScsiPortConvertPhysicalAddressToUlong
/*
 * @implemented
 */
ULONG STDCALL
ScsiPortConvertPhysicalAddressToUlong(IN SCSI_PHYSICAL_ADDRESS Address)
{
  DPRINT("ScsiPortConvertPhysicalAddressToUlong()\n");
  return(Address.u.LowPart);
}


/* EOF */
