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
/* $Id: scsiport.c,v 1.34 2003/09/05 11:48:03 ekohl Exp $
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

#define NDEBUG
#include <debug.h>


#define VERSION "0.0.1"

#include "scsiport_int.h"

/* TYPES *********************************************************************/

#define IRP_FLAG_COMPLETE	0x00000001
#define IRP_FLAG_NEXT		0x00000002


/* GLOBALS *******************************************************************/

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

static IO_ALLOCATION_ACTION STDCALL
ScsiPortAllocateController(IN PDEVICE_OBJECT DeviceObject,
			   IN PIRP Irp,
			   IN PVOID MapRegisterBase,
			   IN PVOID Context);

static BOOLEAN STDCALL
ScsiPortStartPacket(IN OUT PVOID Context);

static NTSTATUS
ScsiPortCreatePortDevice(IN PDRIVER_OBJECT DriverObject,
			 IN PSCSI_PORT_DEVICE_EXTENSION PseudoDeviceExtension,
			 IN ULONG PortCount,
			 IN OUT PSCSI_PORT_DEVICE_EXTENSION *RealDeviceExtension);

static PSCSI_PORT_LUN_EXTENSION
SpiAllocateLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
			 IN UCHAR PathId,
			 IN UCHAR TargetId,
			 IN UCHAR Lun);

static PSCSI_PORT_LUN_EXTENSION
SpiGetLunExtension (IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		    IN UCHAR PathId,
		    IN UCHAR TargetId,
		    IN UCHAR Lun);

static VOID
ScsiPortInquire(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension);

static ULONG
SpiGetInquiryData(IN PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
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

static NTSTATUS
ScsiPortBuildDeviceMap(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
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

  DPRINT("ScsiPortGetDeviceBase() called\n");

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
  DPRINT1("ScsiPortGetPhysicalAddress(%p %p %p %p)\n",
	  HwDeviceExtension, Srb, VirtualAddress, Length);
  UNIMPLEMENTED;
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
  DPRINT("ScsiPortGetSrb()\n");
  UNIMPLEMENTED;
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
      DeviceDescription.Version = DEVICE_DESCRIPTION_VERSION1;
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
  DeviceExtension->CommonBufferLength = NumberOfBytes;
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

  return DeviceExtension->VirtualAddress;
}


/*
 * @unimplemented
 */
PVOID STDCALL
ScsiPortGetVirtualAddress(IN PVOID HwDeviceExtension,
			  IN SCSI_PHYSICAL_ADDRESS PhysicalAddress)
{
  DPRINT("ScsiPortGetVirtualAddress()\n");
  UNIMPLEMENTED;
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
  PUNICODE_STRING RegistryPath = (PUNICODE_STRING)Argument2;
  PSCSI_PORT_DEVICE_EXTENSION PseudoDeviceExtension;
  PSCSI_PORT_DEVICE_EXTENSION RealDeviceExtension;
  PCONFIGURATION_INFORMATION SystemConfig;
  PPORT_CONFIGURATION_INFORMATION PortConfig;
  BOOLEAN Again;
  ULONG i;
  ULONG Result;
  NTSTATUS Status;
  ULONG MaxBus;
  PACCESS_RANGE AccessRanges;
  ULONG ExtensionSize;

  DPRINT("ScsiPortInitialize() called!\n");

  if ((HwInitializationData->HwInitialize == NULL) ||
      (HwInitializationData->HwStartIo == NULL) ||
      (HwInitializationData->HwInterrupt == NULL) ||
      (HwInitializationData->HwFindAdapter == NULL) ||
      (HwInitializationData->HwResetBus == NULL))
    return(STATUS_INVALID_PARAMETER);

  DriverObject->DriverStartIo = ScsiPortStartIo;
  DriverObject->MajorFunction[IRP_MJ_CREATE] = (PDRIVER_DISPATCH)ScsiPortCreateClose;
  DriverObject->MajorFunction[IRP_MJ_CLOSE] = (PDRIVER_DISPATCH)ScsiPortCreateClose;
  DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] = (PDRIVER_DISPATCH)ScsiPortDeviceControl;
  DriverObject->MajorFunction[IRP_MJ_SCSI] = (PDRIVER_DISPATCH)ScsiPortDispatchScsi;


  SystemConfig = IoGetConfigurationInformation();

  ExtensionSize = sizeof(SCSI_PORT_DEVICE_EXTENSION) +
		  HwInitializationData->DeviceExtensionSize;
  PseudoDeviceExtension = ExAllocatePool(PagedPool,
					 ExtensionSize);
  RtlZeroMemory(PseudoDeviceExtension,
		ExtensionSize);
  PseudoDeviceExtension->Length = ExtensionSize;
  PseudoDeviceExtension->MiniPortExtensionSize = HwInitializationData->DeviceExtensionSize;
  PseudoDeviceExtension->LunExtensionSize = HwInitializationData->SpecificLuExtensionSize;
  PseudoDeviceExtension->HwStartIo = HwInitializationData->HwStartIo;
  PseudoDeviceExtension->HwInterrupt = HwInitializationData->HwInterrupt;

  PortConfig = &PseudoDeviceExtension->PortConfig;

  PortConfig->Length = sizeof(PORT_CONFIGURATION_INFORMATION);
  PortConfig->AdapterInterfaceType = HwInitializationData->AdapterInterfaceType;
  PortConfig->InterruptMode =
   (PortConfig->AdapterInterfaceType == PCIBus) ? LevelSensitive : Latched;
  PortConfig->AtdiskPrimaryClaimed = SystemConfig->AtDiskPrimaryAddressClaimed;
  PortConfig->AtdiskSecondaryClaimed = SystemConfig->AtDiskSecondaryAddressClaimed;
  PortConfig->NumberOfAccessRanges = HwInitializationData->NumberOfAccessRanges;

  PortConfig->AccessRanges =
    ExAllocatePool(PagedPool,
		   sizeof(ACCESS_RANGE) * PortConfig->NumberOfAccessRanges);

  for (i = 0; i < SCSI_MAXIMUM_BUSES; i++)
    PortConfig->InitiatorBusId[i] = 255;

  PortConfig->SystemIoBusNumber = 0;
  PortConfig->SlotNumber = 0;

  MaxBus = (PortConfig->AdapterInterfaceType == PCIBus) ? 8 : 1;

  DPRINT("MaxBus: %lu\n", MaxBus);

  while (TRUE)
    {
      DPRINT("Calling HwFindAdapter() for Bus %lu\n", PortConfig->SystemIoBusNumber);

      InitializeListHead(&PseudoDeviceExtension->DeviceBaseListHead);

//      RtlZeroMemory(AccessRanges,
//		    sizeof(ACCESS_RANGE) * PortConfig->NumberOfAccessRanges);

      RtlZeroMemory(PseudoDeviceExtension->MiniPortDeviceExtension,
		    PseudoDeviceExtension->MiniPortExtensionSize);

      /* Note: HwFindAdapter is called once for each bus */
      Result = (HwInitializationData->HwFindAdapter)(&PseudoDeviceExtension->MiniPortDeviceExtension,
						     HwContext,
						     NULL,	/* BusInformation */
						     NULL,	/* ArgumentString */
						     &PseudoDeviceExtension->PortConfig,
						     &Again);
      DPRINT("HwFindAdapter() result: %lu\n", Result);

      if (Result == SP_RETURN_FOUND)
	{
	  DPRINT("ScsiPortInitialize(): Found HBA!\n");

	  Status = ScsiPortCreatePortDevice(DriverObject,
					    PseudoDeviceExtension,
					    SystemConfig->ScsiPortCount,
					    &RealDeviceExtension);

	  if (!NT_SUCCESS(Status))
	    {
	      DbgPrint("ScsiPortCreatePortDevice() failed! (Status 0x%lX)\n", Status);

	      ExFreePool(PortConfig->AccessRanges);
	      ExFreePool(PseudoDeviceExtension);

	      return(Status);
	    }

	  /* Get inquiry data */
	  ScsiPortInquire(RealDeviceExtension);

	  /* Build the registry device map */
	  ScsiPortBuildDeviceMap(RealDeviceExtension,
				 (PUNICODE_STRING)Argument2);

	  /* Update the configuration info */
	  SystemConfig->AtDiskPrimaryAddressClaimed = PortConfig->AtdiskPrimaryClaimed;
	  SystemConfig->AtDiskSecondaryAddressClaimed = PortConfig->AtdiskSecondaryClaimed;
	  SystemConfig->ScsiPortCount++;
	}

      if (Again == FALSE)
	{
	  PortConfig->SystemIoBusNumber++;
	  PortConfig->SlotNumber = 0;
	}

      DPRINT("Bus: %lu  MaxBus: %lu\n", PortConfig->SystemIoBusNumber, MaxBus);
      if (PortConfig->SystemIoBusNumber >= MaxBus)
	{
	  DPRINT("Scanned all buses!\n");
	  break;
	}
    }

  ExFreePool(PortConfig->AccessRanges);
  ExFreePool(PseudoDeviceExtension);

  DPRINT("ScsiPortInitialize() done!\n");

  return(STATUS_SUCCESS);
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
  DPRINT("ScsiPortIoMapTransfer()\n");
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

  DPRINT("ScsiPortLogError() called\n");

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

  DPRINT("ScsiPortNotification() called\n");

  DeviceExtension = CONTAINING_RECORD(HwDeviceExtension,
				      SCSI_PORT_DEVICE_EXTENSION,
				      MiniPortDeviceExtension);

  DPRINT("DeviceExtension %p\n", DeviceExtension);

  DPRINT("Initializing = %s\n", (DeviceExtension->Initializing)?"TRUE":"FALSE");

  if (DeviceExtension->Initializing == TRUE)
    return;

  switch (NotificationType)
    {
      case RequestComplete:
	DPRINT("Notify: RequestComplete\n");
	DeviceExtension->IrpFlags |= IRP_FLAG_COMPLETE;
	break;

      case NextRequest:
	DPRINT("Notify: NextRequest\n");
	DeviceExtension->IrpFlags |= IRP_FLAG_NEXT;
	break;

      default:
	break;
    }
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
 *	...
  		Additional output data (see printf()).
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

  switch (Srb->Function)
    {
      case SRB_FUNCTION_EXECUTE_SCSI:
	IoStartPacket(DeviceObject, Irp, NULL, NULL);
	return(STATUS_PENDING);

      case SRB_FUNCTION_SHUTDOWN:
      case SRB_FUNCTION_FLUSH:
	if (DeviceExtension->PortConfig.CachesData == TRUE)
	  {
	    IoStartPacket(DeviceObject, Irp, NULL, NULL);
	    return(STATUS_PENDING);
	  }
	break;

      case SRB_FUNCTION_CLAIM_DEVICE:
	{
	  PSCSI_PORT_LUN_EXTENSION LunExtension;

	  DPRINT("  SRB_FUNCTION_CLAIM_DEVICE\n");
	  DPRINT("PathId: %lu  TargetId: %lu  Lun: %lu\n", Srb->PathId, Srb->TargetId, Srb->Lun);

	  LunExtension = SpiGetLunExtension(DeviceExtension,
					    Srb->PathId,
					    Srb->TargetId,
					    Srb->Lun);
	  if (LunExtension != NULL)
	    {
	      /* Reference device object and keep the pointer */
	      ObReferenceObject(DeviceObject);
	      LunExtension->DeviceObject = DeviceObject;
	      LunExtension->DeviceClaimed = TRUE;
	      Srb->DataBuffer = DeviceObject;
	    }
	  else
	    {
	      Srb->DataBuffer = NULL;
	    }
	}
	break;

      case SRB_FUNCTION_RELEASE_DEVICE:
	{
	  PSCSI_PORT_LUN_EXTENSION LunExtension;

	  DPRINT("  SRB_FUNCTION_RELEASE_DEVICE\n");
	  DPRINT("PathId: %lu  TargetId: %lu  Lun: %lu\n", Srb->PathId, Srb->TargetId, Srb->Lun);

	  LunExtension = SpiGetLunExtension(DeviceExtension,
					    Srb->PathId,
					    Srb->TargetId,
					    Srb->Lun);
	  if (LunExtension != NULL)
	    {
	      /* Dereference device object */
	      ObDereferenceObject(LunExtension->DeviceObject);
	      LunExtension->DeviceObject = NULL;
	      LunExtension->DeviceClaimed = FALSE;
	    }
	}
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
  PIO_STACK_LOCATION IrpStack;
  KIRQL OldIrql;

  DPRINT("ScsiPortStartIo() called!\n");

  DeviceExtension = DeviceObject->DeviceExtension;
  IrpStack = IoGetCurrentIrpStackLocation(Irp);

  // FIXME: implement the supported functions

  switch (IrpStack->MajorFunction)
    {
      case IRP_MJ_SCSI:
	{
	  BOOLEAN Result;
	  PSCSI_REQUEST_BLOCK Srb;
	  KIRQL oldIrql;

	  DPRINT("IRP_MJ_SCSI\n");

	  Srb = IrpStack->Parameters.Scsi.Srb;

	  DPRINT("DeviceExtension %p\n", DeviceExtension);

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
	      IoStartNextPacket(DeviceObject,
				FALSE);
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
	      KeReleaseSpinLock(&DeviceExtension->IrpLock, oldIrql);
	      IoStartNextPacket(DeviceObject,
				FALSE);
	    }
	  else
	    {
	      KeReleaseSpinLock(&DeviceExtension->IrpLock, oldIrql);
	    }
	}
	break;

      default:
	Irp->IoStatus.Status = STATUS_NOT_SUPPORTED;
	Irp->IoStatus.Information = 0;
	IoCompleteRequest(Irp,
			  IO_NO_INCREMENT);
	IoStartNextPacket(DeviceObject,
			  FALSE);
	break;
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


/**********************************************************************
 * NAME							INTERNAL
 *	ScsiPortCreatePortDevice
 *
 * DESCRIPTION
 *	Creates and initializes a SCSI port device object.
 *
 * RUN LEVEL
 *	PASSIVE_LEVEL
 *
 * ARGUMENTS
 *	DriverObject
 *		...
 *
 *	PseudoDeviceExtension
 *		...
 *
 *	PortNumber
 *		...
 *
 * RETURNS
 *	NTSTATUS
 */

static NTSTATUS
ScsiPortCreatePortDevice(IN PDRIVER_OBJECT DriverObject,
			 IN PSCSI_PORT_DEVICE_EXTENSION PseudoDeviceExtension,
			 IN ULONG PortNumber,
			 IN OUT PSCSI_PORT_DEVICE_EXTENSION *RealDeviceExtension)
{
  PSCSI_PORT_DEVICE_EXTENSION PortDeviceExtension;
  PIO_SCSI_CAPABILITIES PortCapabilities;
  PDEVICE_OBJECT PortDeviceObject;
  WCHAR NameBuffer[80];
  UNICODE_STRING DeviceName;
  WCHAR DosNameBuffer[80];
  UNICODE_STRING DosDeviceName;
  NTSTATUS Status;
  ULONG AccessRangeSize;
  ULONG MappedIrq;
  KIRQL Dirql;
  KAFFINITY Affinity;

  DPRINT("ScsiPortCreatePortDevice() called\n");

  *RealDeviceExtension = NULL;

  MappedIrq = HalGetInterruptVector(PseudoDeviceExtension->PortConfig.AdapterInterfaceType,
				    PseudoDeviceExtension->PortConfig.SystemIoBusNumber,
				    PseudoDeviceExtension->PortConfig.BusInterruptLevel,
				    PseudoDeviceExtension->PortConfig.BusInterruptVector,
				    &Dirql,
				    &Affinity);

  /* Create a unicode device name */
  swprintf(NameBuffer,
	   L"\\Device\\ScsiPort%lu",
	   PortNumber);
  RtlInitUnicodeString(&DeviceName,
		       NameBuffer);

  DPRINT("Creating device: %wZ\n", &DeviceName);

  /* Create the port device */
  Status = IoCreateDevice(DriverObject,
			  PseudoDeviceExtension->Length,
			  &DeviceName,
			  FILE_DEVICE_CONTROLLER,
			  0,
			  FALSE,
			  &PortDeviceObject);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("IoCreateDevice call failed! (Status 0x%lX)\n", Status);
      return(Status);
    }

  DPRINT("Created device: %wZ\n", &DeviceName);

  /* Set the buffering strategy here... */
  PortDeviceObject->Flags |= DO_DIRECT_IO;
  PortDeviceObject->AlignmentRequirement = FILE_WORD_ALIGNMENT;

  PortDeviceExtension = PortDeviceObject->DeviceExtension;

  /* Copy pseudo device extension into the real device extension */
  memcpy(PortDeviceExtension,
	 PseudoDeviceExtension,
	 PseudoDeviceExtension->Length);

  /* Copy access ranges */
  AccessRangeSize =
    sizeof(ACCESS_RANGE) * PseudoDeviceExtension->PortConfig.NumberOfAccessRanges;
  PortDeviceExtension->PortConfig.AccessRanges = ExAllocatePool(NonPagedPool,
								AccessRangeSize);
  memcpy(PortDeviceExtension->PortConfig.AccessRanges,
	 PseudoDeviceExtension->PortConfig.AccessRanges,
	 AccessRangeSize);

  /* Copy device base list */
  if (IsListEmpty(&PseudoDeviceExtension->DeviceBaseListHead))
    {
      InitializeListHead(&PortDeviceExtension->DeviceBaseListHead);
    }
  else
    {
      PseudoDeviceExtension->DeviceBaseListHead.Flink =
	PortDeviceExtension->DeviceBaseListHead.Flink;
      PseudoDeviceExtension->DeviceBaseListHead.Blink =
	PortDeviceExtension->DeviceBaseListHead.Blink;
      PortDeviceExtension->DeviceBaseListHead.Blink->Flink =
	&PortDeviceExtension->DeviceBaseListHead;
      PortDeviceExtension->DeviceBaseListHead.Flink->Blink =
	&PortDeviceExtension->DeviceBaseListHead;
    }

  PortDeviceExtension->DeviceObject = PortDeviceObject;
  PortDeviceExtension->PortNumber = PortNumber;

  /* Initialize the spin lock in the controller extension */
  KeInitializeSpinLock(&PortDeviceExtension->IrpLock);
  KeInitializeSpinLock(&PortDeviceExtension->SpinLock);

  /* Register an interrupt handler for this device */
  Status = IoConnectInterrupt(&PortDeviceExtension->Interrupt,
			      ScsiPortIsr,
			      PortDeviceExtension,
			      &PortDeviceExtension->SpinLock,
			      MappedIrq,
			      Dirql,
			      Dirql,
			      PortDeviceExtension->PortConfig.InterruptMode,
			      TRUE,
			      Affinity,
			      FALSE);
  if (!NT_SUCCESS(Status))
    {
      DbgPrint("Could not Connect Interrupt %d\n",
	       PortDeviceExtension->PortConfig.BusInterruptVector);
      IoDeleteDevice(PortDeviceObject);
      return(Status);
    }

  /* Initialize the DPC object */
  IoInitializeDpcRequest(PortDeviceExtension->DeviceObject,
			 ScsiPortDpcForIsr);

  /* Initialize the device timer */
  PortDeviceExtension->TimerState = IDETimerIdle;
  PortDeviceExtension->TimerCount = 0;
  IoInitializeTimer(PortDeviceExtension->DeviceObject,
		    ScsiPortIoTimer,
		    PortDeviceExtension);

  /* Initialize port capabilities */
  PortCapabilities = ExAllocatePool(NonPagedPool,
				    sizeof(IO_SCSI_CAPABILITIES));
  PortDeviceExtension->PortCapabilities = PortCapabilities;
  PortCapabilities->Length = sizeof(IO_SCSI_CAPABILITIES);
  PortCapabilities->MaximumTransferLength =
    PortDeviceExtension->PortConfig.MaximumTransferLength;
  PortCapabilities->MaximumPhysicalPages =
    PortCapabilities->MaximumTransferLength / PAGE_SIZE;
  PortCapabilities->SupportedAsynchronousEvents = 0; /* FIXME */
  PortCapabilities->AlignmentMask =
    PortDeviceExtension->PortConfig.AlignmentMask;
  PortCapabilities->TaggedQueuing =
    PortDeviceExtension->PortConfig.TaggedQueuing;
  PortCapabilities->AdapterScansDown =
    PortDeviceExtension->PortConfig.AdapterScansDown;
  PortCapabilities->AdapterUsesPio = TRUE; /* FIXME */

  /* Initialize LUN-Extension list */
  InitializeListHead(&PortDeviceExtension->LunExtensionListHead);

  DPRINT("DeviceExtension %p\n", PortDeviceExtension);

  /* FIXME: Copy more configuration data? */


  /* Create the dos device link */
  swprintf(DosNameBuffer,
	   L"\\??\\Scsi%lu:",
	   PortNumber);
  RtlInitUnicodeString(&DosDeviceName,
		       DosNameBuffer);

  IoCreateSymbolicLink(&DosDeviceName,
		       &DeviceName);

  *RealDeviceExtension = PortDeviceExtension;

  DPRINT("ScsiPortCreatePortDevice() done\n");

  return(STATUS_SUCCESS);
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

  return LunExtension;
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


static VOID
ScsiPortInquire(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension)
{
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  SCSI_REQUEST_BLOCK Srb;
  ULONG Bus;
  ULONG Target;
  ULONG Lun;
  BOOLEAN Result;

  DPRINT("ScsiPortInquire() called\n");

  DeviceExtension->Initializing = TRUE;

  RtlZeroMemory(&Srb,
		sizeof(SCSI_REQUEST_BLOCK));
  Srb.DataBuffer = ExAllocatePool(NonPagedPool, 256);
  Srb.Function = SRB_FUNCTION_EXECUTE_SCSI;
  Srb.DataTransferLength = 256;
  Srb.Cdb[0] = SCSIOP_INQUIRY;

  for (Bus = 0; Bus < DeviceExtension->PortConfig.NumberOfBuses; Bus++)
    {
      Srb.PathId = Bus;

      for (Target = 0; Target < DeviceExtension->PortConfig.MaximumNumberOfTargets; Target++)
	{
	  Srb.TargetId = Target;

	  for (Lun = 0; Lun < SCSI_MAXIMUM_LOGICAL_UNITS; Lun++)
	    {
	      Srb.Lun = Lun;
	      Srb.SrbStatus = SRB_STATUS_SUCCESS;

	      Result = DeviceExtension->HwStartIo(&DeviceExtension->MiniPortDeviceExtension,
						  &Srb);
	      DPRINT("Result: %s  Srb.SrbStatus %lx\n", (Result)?"True":"False", Srb.SrbStatus);

	      if (Result == TRUE && Srb.SrbStatus == SRB_STATUS_SUCCESS)
		{
		  LunExtension = SpiAllocateLunExtension(DeviceExtension,
							 Bus,
							 Target,
							 Lun);
		  if (LunExtension != NULL)
		    {
		      /* Copy inquiry data */
		      memcpy(&LunExtension->InquiryData,
			     Srb.DataBuffer,
			     sizeof(INQUIRYDATA));
		    }
		}
	    }
	}
    }

  ExFreePool(Srb.DataBuffer);

  DeviceExtension->Initializing = FALSE;

  DPRINT("ScsiPortInquire() done\n");
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
  AdapterBusInfo->NumberOfBuses = DeviceExtension->PortConfig.NumberOfBuses;

  UnitInfo = (PSCSI_INQUIRY_DATA)
	((PUCHAR)AdapterBusInfo + sizeof(SCSI_ADAPTER_BUS_INFO) +
	 (sizeof(SCSI_BUS_DATA) * (AdapterBusInfo->NumberOfBuses - 1)));

  for (Bus = 0; Bus < AdapterBusInfo->NumberOfBuses; Bus++)
    {
      AdapterBusInfo->BusData[Bus].InitiatorBusId =
	DeviceExtension->PortConfig.InitiatorBusId[Bus];
      AdapterBusInfo->BusData[Bus].InquiryDataOffset =
	(ULONG)((PUCHAR)UnitInfo - (PUCHAR)AdapterBusInfo);

      PrevUnit = NULL;
      UnitCount = 0;

      for (Target = 0; Target < DeviceExtension->PortConfig.MaximumNumberOfTargets; Target++)
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
		  memcpy(&UnitInfo->InquiryData,
			 &LunExtension->InquiryData,
			 INQUIRYDATABUFFERSIZE);
		  if (PrevUnit != NULL)
		    PrevUnit->NextInquiryDataOffset = (ULONG)((PUCHAR)UnitInfo-(PUCHAR)AdapterBusInfo);
		  PrevUnit = UnitInfo;
		  UnitInfo = (PSCSI_INQUIRY_DATA)((PUCHAR)UnitInfo + sizeof(SCSI_INQUIRY_DATA)+INQUIRYDATABUFFERSIZE-1);
		  UnitCount++;
		}
	    }
	}
      DPRINT("UnitCount: %lu\n", UnitCount);
      AdapterBusInfo->BusData[Bus].NumberOfLogicalUnits = UnitCount;
      if (UnitCount == 0)
	AdapterBusInfo->BusData[Bus].InquiryDataOffset = 0;
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
  KIRQL oldIrql;

  DPRINT("ScsiPortDpcForIsr(Dpc %p  DpcDeviceObject %p  DpcIrp %p  DpcContext %p)\n",
	 Dpc, DpcDeviceObject, DpcIrp, DpcContext);

  DeviceExtension = (PSCSI_PORT_DEVICE_EXTENSION)DpcContext;

  KeAcquireSpinLock(&DeviceExtension->IrpLock, &oldIrql);
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
	  DeviceExtension->OriginalSrb = NULL;
	}
      else if ((SRB_STATUS(Srb->SrbStatus) != SRB_STATUS_SUCCESS) &&
	       (Srb->ScsiStatus == SCSISTAT_CHECK_CONDITION))
	{
	  DPRINT("SCSIOP_REQUEST_SENSE required!\n");

	  DeviceExtension->OriginalSrb = Srb;
	  IrpStack->Parameters.Scsi.Srb = ScsiPortInitSenseRequestSrb(DeviceExtension,
								      Srb);
	  KeReleaseSpinLock(&DeviceExtension->IrpLock, oldIrql);
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
	  KeReleaseSpinLock(&DeviceExtension->IrpLock, oldIrql);
	  IoStartNextPacket(DpcDeviceObject, FALSE);
	}
      else
	{
	  KeReleaseSpinLock(&DeviceExtension->IrpLock, oldIrql);
	}
    }
  else
    {
      KeReleaseSpinLock(&DeviceExtension->IrpLock, oldIrql);
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
 *	ScsiPortBuildDeviceMap
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
ScsiPortBuildDeviceMap(PSCSI_PORT_DEVICE_EXTENSION DeviceExtension,
		       PUNICODE_STRING RegistryPath)
{
  PSCSI_PORT_LUN_EXTENSION LunExtension;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  WCHAR NameBuffer[64];
  ULONG Disposition;
  HANDLE ScsiKey;
  HANDLE ScsiPortKey;
  HANDLE ScsiBusKey;
  HANDLE ScsiInitiatorKey;
  HANDLE ScsiTargetKey;
  HANDLE ScsiLunKey;
  ULONG BusNumber;
  ULONG Target;
  ULONG CurrentTarget;
  ULONG Lun;
  PWCHAR DriverName;
  ULONG UlongData;
  PWCHAR TypeName;
  NTSTATUS Status;

  DPRINT("ScsiPortBuildDeviceMap() called\n");

  if (DeviceExtension == NULL || RegistryPath == NULL)
    {
      DPRINT1("Invalid parameter\n");
      return(STATUS_INVALID_PARAMETER);
    }

  /* Open or create the 'Scsi' subkey */
  RtlInitUnicodeStringFromLiteral(&KeyName,
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
  UlongData = (ULONG)DeviceExtension->PortConfig.BusInterruptLevel;
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
  UlongData = ScsiPortConvertPhysicalAddressToUlong(DeviceExtension->PortConfig.AccessRanges[0].RangeStart);
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
  for (BusNumber = 0; BusNumber < DeviceExtension->PortConfig.NumberOfBuses; BusNumber++)
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
	  ZwClose(ScsiPortKey);
	  return(Status);
	}

      /* Create 'Initiator Id X' key */
      DPRINT("      Initiator Id %u\n",
	      DeviceExtension->PortConfig.InitiatorBusId[BusNumber]);
      swprintf(NameBuffer,
	       L"Initiator Id %u",
	       DeviceExtension->PortConfig.InitiatorBusId[BusNumber]);
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
	  ZwClose(ScsiBusKey);
	  ZwClose(ScsiPortKey);
	  return(Status);
	}

      /* FIXME: Are there any initiator values (??) */

      ZwClose(ScsiInitiatorKey);


      /* Enumerate targets */
      CurrentTarget = (ULONG)-1;
      ScsiTargetKey = NULL;
      for (Target = 0; Target < DeviceExtension->PortConfig.MaximumNumberOfTargets; Target++)
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
			  ZwClose(ScsiBusKey);
			  ZwClose(ScsiPortKey);
			  return(Status);
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
		      ZwClose(ScsiTargetKey);
		      ZwClose(ScsiBusKey);
		      ZwClose(ScsiPortKey);
		      return(Status);
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
		      ZwClose(ScsiLunKey);
		      ZwClose(ScsiTargetKey);
		      ZwClose(ScsiBusKey);
		      ZwClose(ScsiPortKey);
		      return(Status);
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
		      ZwClose(ScsiLunKey);
		      ZwClose(ScsiTargetKey);
		      ZwClose(ScsiBusKey);
		      ZwClose(ScsiPortKey);
		      return(Status);
		    }

		  ZwClose(ScsiLunKey);
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
    }

  ZwClose(ScsiPortKey);

  DPRINT("ScsiPortBuildDeviceMap() done\n");

  return(Status);
}

/* EOF */
