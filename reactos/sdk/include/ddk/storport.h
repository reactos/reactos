/*
 * storport.h
 *
 * StorPort interface
 *
 * This file is part of the w32api package.
 *
 * Contributors:
 *   Created by Casper S. Hornstrup <chorns@users.sourceforge.net>
 *
 * THIS SOFTWARE IS NOT COPYRIGHTED
 *
 * This source code is offered for use in the public domain. You may
 * use, modify or distribute it freely.
 *
 * This code is distributed in the hope that it will be useful but
 * WITHOUT ANY WARRANTY. ALL WARRANTIES, EXPRESS OR IMPLIED ARE HEREBY
 * DISCLAIMED. This includes but is not limited to warranties of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 *
 */

#ifndef __STORPORT_H
#define __STORPORT_H

#include "srb.h"

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_STORPORT_)
#define STORPORTAPI
#else
#define STORPORTAPI DECLSPEC_IMPORT
#endif

typedef PHYSICAL_ADDRESS STOR_PHYSICAL_ADDRESS;

typedef struct _STOR_SCATTER_GATHER_ELEMENT {
  STOR_PHYSICAL_ADDRESS PhysicalAddress;
  ULONG Length;
  ULONG_PTR Reserved;
} STOR_SCATTER_GATHER_ELEMENT, *PSTOR_SCATTER_GATHER_ELEMENT;

typedef struct _STOR_SCATTER_GATHER_LIST {
  ULONG NumberOfElements;
  ULONG_PTR Reserved;
  STOR_SCATTER_GATHER_ELEMENT List[0];
} STOR_SCATTER_GATHER_LIST, *PSTOR_SCATTER_GATHER_LIST;

typedef struct _SCSI_WMI_REQUEST_BLOCK {
  USHORT Length;
  UCHAR Function;
  UCHAR SrbStatus;
  UCHAR WMISubFunction;
  UCHAR PathId;
  UCHAR TargetId;
  UCHAR Lun;
  UCHAR Reserved1;
  UCHAR WMIFlags;
  UCHAR Reserved2[2];
  ULONG SrbFlags;
  ULONG DataTransferLength;
  ULONG TimeOutValue;
  PVOID DataBuffer;
  PVOID DataPath;
  PVOID Reserved3;
  PVOID OriginalRequest;
  PVOID SrbExtension;
  ULONG Reserved4;
  UCHAR Reserved5[16];
} SCSI_WMI_REQUEST_BLOCK, *PSCSI_WMI_REQUEST_BLOCK;

STORPORTAPI
ULONG
NTAPI
StorPortInitialize(
  _In_ PVOID Argument1,
  _In_ PVOID Argument2,
  _In_ PHW_INITIALIZATION_DATA HwInitializationData,
  _In_opt_ PVOID Unused);

STORPORTAPI
VOID
NTAPI
StorPortFreeDeviceBase(
  _In_ PVOID HwDeviceExtension,
  _In_ PVOID MappedAddress);

STORPORTAPI
ULONG
NTAPI
StorPortGetBusData(
  _In_ PVOID DeviceExtension,
  _In_ ULONG BusDataType,
  _In_ ULONG SystemIoBusNumber,
  _In_ ULONG SlotNumber,
  _Out_ _When_(Length != 0, _Out_writes_bytes_(Length)) PVOID Buffer,
  _In_ ULONG Length);

STORPORTAPI
ULONG
NTAPI
StorPortSetBusDataByOffset(
  _In_ PVOID DeviceExtension,
  _In_ ULONG BusDataType,
  _In_ ULONG SystemIoBusNumber,
  _In_ ULONG SlotNumber,
  _In_reads_bytes_(Length) PVOID Buffer,
  _In_ ULONG Offset,
  _In_ ULONG Length);

STORPORTAPI
PVOID
NTAPI
StorPortGetDeviceBase(
  _In_ PVOID HwDeviceExtension,
  _In_ INTERFACE_TYPE BusType,
  _In_ ULONG SystemIoBusNumber,
  _In_ SCSI_PHYSICAL_ADDRESS IoAddress,
  _In_ ULONG NumberOfBytes,
  _In_ BOOLEAN InIoSpace);

STORPORTAPI
PVOID
NTAPI
StorPortGetLogicalUnit(
  _In_ PVOID HwDeviceExtension,
  _In_ UCHAR PathId,
  _In_ UCHAR TargetId,
  _In_ UCHAR Lun);

STORPORTAPI
PSCSI_REQUEST_BLOCK
NTAPI
StorPortGetSrb(
  _In_ PVOID DeviceExtension,
  _In_ UCHAR PathId,
  _In_ UCHAR TargetId,
  _In_ UCHAR Lun,
  _In_ LONG QueueTag);

STORPORTAPI
STOR_PHYSICAL_ADDRESS
NTAPI
StorPortGetPhysicalAddress(
  _In_ PVOID HwDeviceExtension,
  _In_opt_ PSCSI_REQUEST_BLOCK Srb,
  _In_ PVOID VirtualAddress,
  _Out_ ULONG *Length);

STORPORTAPI
PVOID
NTAPI
StorPortGetVirtualAddress(
  _In_ PVOID HwDeviceExtension,
  _In_ STOR_PHYSICAL_ADDRESS PhysicalAddress);

STORPORTAPI
PVOID
NTAPI
StorPortGetUncachedExtension(
  _In_ PVOID HwDeviceExtension,
  _In_ PPORT_CONFIGURATION_INFORMATION ConfigInfo,
  _In_ ULONG NumberOfBytes);

STORPORTAPI
VOID
__cdecl
StorPortNotification(
  _In_ SCSI_NOTIFICATION_TYPE NotificationType,
  _In_ PVOID HwDeviceExtension,
  ...);

STORPORTAPI
VOID
NTAPI
StorPortLogError(
  _In_ PVOID HwDeviceExtension,
  _In_opt_ PSCSI_REQUEST_BLOCK Srb,
  _In_ UCHAR PathId,
  _In_ UCHAR TargetId,
  _In_ UCHAR Lun,
  _In_ ULONG ErrorCode,
  _In_ ULONG UniqueId);

STORPORTAPI
VOID
NTAPI
StorPortCompleteRequest(
  _In_ PVOID HwDeviceExtension,
  _In_ UCHAR PathId,
  _In_ UCHAR TargetId,
  _In_ UCHAR Lun,
  _In_ UCHAR SrbStatus);

STORPORTAPI
VOID
NTAPI
StorPortMoveMemory(
  _Out_writes_bytes_(Length) PVOID WriteBuffer,
  _In_reads_bytes_(Length) PVOID ReadBuffer,
  _In_ ULONG Length);

STORPORTAPI
VOID
NTAPI
StorPortStallExecution(
  _In_ ULONG Delay);

STORPORTAPI
STOR_PHYSICAL_ADDRESS
NTAPI
StorPortConvertUlong64ToPhysicalAddress(
  _In_ ULONG64 UlongAddress);

STORPORTAPI
ULONG64
NTAPI
StorPortConvertPhysicalAddressToUlong64(
  _In_ STOR_PHYSICAL_ADDRESS Address);

STORPORTAPI
BOOLEAN
NTAPI
StorPortValidateRange(
  _In_ PVOID HwDeviceExtension,
  _In_ INTERFACE_TYPE BusType,
  _In_ ULONG SystemIoBusNumber,
  _In_ STOR_PHYSICAL_ADDRESS IoAddress,
  _In_ ULONG NumberOfBytes,
  _In_ BOOLEAN InIoSpace);

STORPORTAPI
VOID
__cdecl
StorPortDebugPrint(
  _In_ ULONG DebugPrintLevel,
  _In_ PCCHAR DebugMessage,
  ...);

STORPORTAPI
UCHAR
NTAPI
StorPortReadPortUchar(
  _In_ PVOID HwDeviceExtension, 
  _In_ PUCHAR Port);

STORPORTAPI
ULONG
NTAPI
StorPortReadPortUlong(
  _In_ PVOID HwDeviceExtension,
  _In_ PULONG Port);

STORPORTAPI
USHORT
NTAPI
StorPortReadPortUshort(
  _In_ PVOID HwDeviceExtension,
  _In_ PUSHORT Port);

STORPORTAPI
UCHAR
NTAPI
StorPortReadRegisterUchar(
  _In_ PVOID HwDeviceExtension,
  _In_ PUCHAR Register);

STORPORTAPI
ULONG
NTAPI
StorPortReadRegisterUlong(
  _In_ PVOID HwDeviceExtension,
  _In_ PULONG Register);

STORPORTAPI
USHORT
NTAPI
StorPortReadRegisterUshort(
  _In_ PVOID HwDeviceExtension,
  _In_ PUSHORT Register);

STORPORTAPI
VOID
NTAPI
StorPortWritePortUchar(
  _In_ PVOID HwDeviceExtension,
  _In_ PUCHAR Port,
  _In_ UCHAR Value);

STORPORTAPI
VOID
NTAPI
StorPortWritePortUlong(
  _In_ PVOID HwDeviceExtension,
  _In_ PULONG Port,
  _In_ ULONG Value);

STORPORTAPI
VOID
NTAPI
StorPortWritePortUshort(
  _In_ PVOID HwDeviceExtension,
  _In_ PUSHORT Port,
  _In_ USHORT Value);

STORPORTAPI
VOID
NTAPI
StorPortWriteRegisterUchar(
  _In_ PVOID HwDeviceExtension,
  _In_ PUCHAR Register,
  _In_ UCHAR Value);

STORPORTAPI
VOID
NTAPI
StorPortWriteRegisterUlong(
  _In_ PVOID HwDeviceExtension,
  _In_ PULONG Register,
  _In_ ULONG Value);

STORPORTAPI
VOID
NTAPI
StorPortWriteRegisterUshort(
  _In_ PVOID HwDeviceExtension,
  _In_ PUSHORT Register,
  _In_ USHORT Value);

STORPORTAPI
BOOLEAN
NTAPI
StorPortPauseDevice(
  _In_ PVOID HwDeviceExtension,
  _In_ UCHAR PathId,
  _In_ UCHAR TargetId,
  _In_ UCHAR Lun,
  _In_ ULONG TimeOut);

STORPORTAPI
BOOLEAN
NTAPI
StorPortResumeDevice(
  _In_ PVOID HwDeviceExtension,
  _In_ UCHAR PathId,
  _In_ UCHAR TargetId,
  _In_ UCHAR Lun);

STORPORTAPI
BOOLEAN
NTAPI
StorPortPause(
  _In_ PVOID HwDeviceExtension,
  _In_ ULONG TimeOut);

STORPORTAPI
BOOLEAN
NTAPI
StorPortResume(
  _In_ PVOID HwDeviceExtension);

STORPORTAPI
BOOLEAN
NTAPI
StorPortDeviceBusy(
  _In_ PVOID HwDeviceExtension,
  _In_ UCHAR PathId,
  _In_ UCHAR TargetId,
  _In_ UCHAR Lun,
  _In_ ULONG RequestsToComplete);

STORPORTAPI
BOOLEAN
NTAPI
StorPortDeviceReady(
  _In_ PVOID HwDeviceExtension,
  _In_ UCHAR PathId,
  _In_ UCHAR TargetId,
  _In_ UCHAR Lun);

STORPORTAPI
BOOLEAN
NTAPI
StorPortBusy(
  _In_ PVOID HwDeviceExtension,
  _In_ ULONG RequestsToComplete);

STORPORTAPI
BOOLEAN
NTAPI
StorPortReady(
  _In_ PVOID HwDeviceExtension);

STORPORTAPI
PSTOR_SCATTER_GATHER_LIST
NTAPI
StorPortGetScatterGatherList(
  _In_ PVOID DeviceExtension,
  _In_ PSCSI_REQUEST_BLOCK Srb);

typedef BOOLEAN
(NTAPI STOR_SYNCHRONIZED_ACCESS)(
  _In_ PVOID HwDeviceExtension,
  _In_ PVOID Context);
typedef STOR_SYNCHRONIZED_ACCESS *PSTOR_SYNCHRONIZED_ACCESS;

STORPORTAPI
VOID
NTAPI
StorPortSynchronizeAccess(
  _In_ PVOID HwDeviceExtension,
  _In_ PSTOR_SYNCHRONIZED_ACCESS SynchronizedAccessRoutine,
  _In_opt_ PVOID Context);

#if DBG
#define DebugPrint(x) StorPortDebugPrint x
#else
#define DebugPrint(x)
#endif

#ifdef __cplusplus
}
#endif

#endif /* __STORPORT_H */
