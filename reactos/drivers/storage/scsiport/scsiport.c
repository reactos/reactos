/* $Id: scsiport.c,v 1.1 2001/07/21 07:30:26 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            services/storage/scsiport/scsiport.c
 * PURPOSE:         SCSI port driver
 * PROGRAMMER:      Eric Kohl (ekohl@rz-online.de)
 */

#include <ddk/ntddk.h>
#include "../include/srb.h"


#define UNIMPLEMENTED do {DbgPrint("%s:%d: Function not implemented", __FILE__, __LINE__); for(;;);} while (0)

#define VERSION "0.0.1"

//  -------------------------------------------------------  Public Interface

//    DriverEntry
//
//  DESCRIPTION:
//    This function initializes the driver.
//
//  RUN LEVEL:
//    PASSIVE_LEVEL
//
//  ARGUMENTS:
//    IN  PDRIVER_OBJECT   DriverObject  System allocated Driver Object
//                                       for this driver
//    IN  PUNICODE_STRING  RegistryPath  Name of registry driver service 
//                                       key
//
//  RETURNS:
//    NTSTATUS

NTSTATUS STDCALL
DriverEntry(IN PDRIVER_OBJECT DriverObject,
	    IN PUNICODE_STRING RegistryPath)
{
  DbgPrint("ScsiPort Driver %s\n", VERSION);
  return(STATUS_SUCCESS);
}


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


VOID STDCALL
ScsiPortCompleteRequest(IN PVOID HwDeviceExtension,
			IN UCHAR PathId,
			IN UCHAR TargetId,
			IN UCHAR Lun,
			IN UCHAR SrbStatus)
{
  UNIMPLEMENTED;
}


ULONG STDCALL
ScsiPortConvertPhysicalAddressToUlong(IN SCSI_PHYSICAL_ADDRESS Address)
{
  return Address.u.LowPart;
}


VOID STDCALL
ScsiPortFlushDma(IN PVOID HwDeviceExtension)
{
  UNIMPLEMENTED;
}


VOID STDCALL
ScsiPortFreeDeviceBase(IN PVOID HwDeviceExtension,
		       IN PVOID MappedAddress)
{
  UNIMPLEMENTED;
}


ULONG STDCALL
ScsiPortGetBusData(IN PVOID DeviceExtension,
		   IN ULONG BusDataType,
		   IN ULONG SystemIoBusNumber,
		   IN ULONG SlotNumber,
		   IN PVOID Buffer,
		   IN ULONG Length)
{
  UNIMPLEMENTED;
}


PVOID STDCALL
ScsiPortGetDeviceBase(IN PVOID HwDeviceExtension,
		      IN INTERFACE_TYPE BusType,
		      IN ULONG SystemIoBusNumber,
		      IN SCSI_PHYSICAL_ADDRESS IoAddress,
		      IN ULONG NumberOfBytes,
		      IN BOOLEAN InIoSpace)
{
  ULONG AddressSpace;
  PHYSICAL_ADDRESS TranslatedAddress;
  PVOID VirtualAddress;
  PVOID Buffer;
  BOOLEAN rc;

  AddressSpace = (ULONG)InIoSpace;

  if (!HalTranslateBusAddress(BusType,
			      SystemIoBusNumber,
			      IoAddress,
			      &AddressSpace,
			      &TranslatedAddress))
    return NULL;

  /* i/o space */
  if (AddressSpace != 0)
    return (PVOID)TranslatedAddress.u.LowPart;

  VirtualAddress = MmMapIoSpace(TranslatedAddress,
				NumberOfBytes,
				FALSE);

  Buffer = ExAllocatePool(NonPagedPool,0x20);
  if (Buffer == NULL)
    return VirtualAddress;

  return NULL;  /* ?? */
}


PVOID STDCALL
ScsiPortGetLogicalUnit(IN PVOID HwDeviceExtension,
		       IN UCHAR PathId,
		       IN UCHAR TargetId,
		       IN UCHAR Lun)
{
  UNIMPLEMENTED;
}


SCSI_PHYSICAL_ADDRESS STDCALL
ScsiPortGetPhysicalAddress(IN PVOID HwDeviceExtension,
			   IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
			   IN PVOID VirtualAddress,
			   OUT ULONG *Length)
{
  UNIMPLEMENTED;
}


PSCSI_REQUEST_BLOCK STDCALL
ScsiPortGetSrb(IN PVOID DeviceExtension,
	       IN UCHAR PathId,
	       IN UCHAR TargetId,
	       IN UCHAR Lun,
	       IN LONG QueueTag)
{
  UNIMPLEMENTED;
}


PVOID STDCALL
ScsiPortGetUncachedExtension(IN PVOID HwDeviceExtension,
			     IN PPORT_CONFIGURATION_INFORMATION ConfigInfo,
			     IN ULONG NumberOfBytes)
{
  UNIMPLEMENTED;
}


PVOID STDCALL
ScsiPortGetVirtualAddress(IN PVOID HwDeviceExtension,
			  IN SCSI_PHYSICAL_ADDRESS PhysicalAddress)
{
  UNIMPLEMENTED;
}


ULONG STDCALL
ScsiPortInitialize(IN PVOID Argument1,
		   IN PVOID Argument2,
		   IN struct _HW_INITIALIZATION_DATA *HwInitializationData,
		   IN PVOID HwContext)
{
  UNIMPLEMENTED;
  return(STATUS_SUCCESS);
}


VOID STDCALL
ScsiPortIoMapTransfer(IN PVOID HwDeviceExtension,
		      IN PSCSI_REQUEST_BLOCK Srb,
		      IN ULONG LogicalAddress,
		      IN ULONG Length)
{
  UNIMPLEMENTED;
}


VOID STDCALL
ScsiPortLogError(IN PVOID HwDeviceExtension,
		 IN PSCSI_REQUEST_BLOCK Srb OPTIONAL,
		 IN UCHAR PathId,
		 IN UCHAR TargetId,
		 IN UCHAR Lun,
		 IN ULONG ErrorCode,
		 IN ULONG UniqueId)
{
  UNIMPLEMENTED;
}


VOID STDCALL
ScsiPortMoveMemory(OUT PVOID Destination,
		   IN PVOID Source,
		   IN ULONG Length)
{
  RtlMoveMemory(Destination,
		Source,
		Length);
}


VOID
ScsiPortNotification(IN SCSI_NOTIFICATION_TYPE NotificationType,
		     IN PVOID HwDeviceExtension,
		     ...)
{
  UNIMPLEMENTED;
}


ULONG STDCALL
ScsiPortSetBusDataByOffset(IN PVOID DeviceExtension,
			   IN ULONG BusDataType,
			   IN ULONG SystemIoBusNumber,
			   IN ULONG SlotNumber,
			   IN PVOID Buffer,
			   IN ULONG Offset,
			   IN ULONG Length)
{
  return(HalSetBusDataByOffset(BusDataType,
			       SystemIoBusNumber,
			       SlotNumber,
			       Buffer,
			       Offset,
			       Length));
}


BOOLEAN STDCALL
ScsiPortValidateRange(IN PVOID HwDeviceExtension,
		      IN INTERFACE_TYPE BusType,
		      IN ULONG SystemIoBusNumber,
		      IN SCSI_PHYSICAL_ADDRESS IoAddress,
		      IN ULONG NumberOfBytes,
		      IN BOOLEAN InIoSpace)
{
  return(TRUE);
}

/* EOF */
