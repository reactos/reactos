/* $Id: haltypes.h,v 1.1.2.1 2004/10/25 01:24:07 ion Exp $
 *
 *  ReactOS Headers
 *  Copyright (C) 1998-2004 ReactOS Team
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
/*
 * PROJECT:         ReactOS Native Headers
 * FILE:            include/ndk/haltypes.h
 * PURPOSE:         Definitions for Hardware Abstraction Layer types not defined in DDK/IFS
 * PROGRAMMER:      Alex Ionescu (alex@relsoft.net)
 * UPDATE HISTORY:
 *                  Created 06/10/04
 */
#ifndef _HALTYPES_H
#define _HALTYPES_H

#include <reactos/helper.h>

extern ULONG EXPORTED KdComPortInUse;

typedef struct _DEVICE_HANDLER_OBJECT *PDEVICE_HANDLER_OBJECT;

typedef struct _DEVICE_CONTROL_CONTEXT
{
  NTSTATUS Status;
  PDEVICE_HANDLER_OBJECT DeviceHandler;
  PDEVICE_OBJECT DeviceObject;
  ULONG ControlCode;
  PVOID Buffer;
  PULONG BufferLength;
  PVOID Context;
} DEVICE_CONTROL_CONTEXT, *PDEVICE_CONTROL_CONTEXT;

/* HalReturnToFirmware */
#define FIRMWARE_HALT        1
#define FIRMWARE_POWERDOWN   2
#define FIRMWARE_REBOOT      3

/* Boot Flags */
#define MB_FLAGS_MEM_INFO         (0x1)
#define MB_FLAGS_BOOT_DEVICE      (0x2)
#define MB_FLAGS_COMMAND_LINE     (0x4)
#define MB_FLAGS_MODULE_INFO      (0x8)
#define MB_FLAGS_AOUT_SYMS        (0x10)
#define MB_FLAGS_ELF_SYMS         (0x20)
#define MB_FLAGS_MMAP_INFO        (0x40)
#define MB_FLAGS_DRIVES_INFO      (0x80)
#define MB_FLAGS_CONFIG_TABLE     (0x100)
#define MB_FLAGS_BOOT_LOADER_NAME (0x200)
#define MB_FLAGS_APM_TABLE        (0x400)
#define MB_FLAGS_GRAPHICS_TABLE   (0x800)

typedef struct _LOADER_PARAMETER_BLOCK
{
   ULONG Flags;
   ULONG MemLower;
   ULONG MemHigher;
   ULONG BootDevice;
   ULONG CommandLine;
   ULONG ModsCount;
   ULONG ModsAddr;
   UCHAR Syms[12];
   ULONG MmapLength;
   ULONG MmapAddr;
   ULONG DrivesCount;
   ULONG DrivesAddr;
   ULONG ConfigTable;
   ULONG BootLoaderName;
} LOADER_PARAMETER_BLOCK, *PLOADER_PARAMETER_BLOCK;

typedef enum _HAL_QUERY_INFORMATION_CLASS
{
  HalInstalledBusInformation,
  HalProfileSourceInformation,
  HalSystemDockInformation,
  HalPowerInformation,
  HalProcessorSpeedInformation,
  HalCallbackInformation,
  HalMapRegisterInformation,
  HalMcaLogInformation,
  HalFrameBufferCachingInformation,
  HalDisplayBiosInformation
  /* information levels >= 0x8000000 reserved for OEM use */
} HAL_QUERY_INFORMATION_CLASS, *PHAL_QUERY_INFORMATION_CLASS;

typedef enum _HAL_SET_INFORMATION_CLASS
{
  HalProfileSourceInterval,
  HalProfileSourceInterruptHandler,
  HalMcaRegisterDriver
} HAL_SET_INFORMATION_CLASS, *PHAL_SET_INFORMATION_CLASS;

/* Function Type Defintions for Dispatch Functions */

typedef BOOLEAN STDCALL
(*PHAL_RESET_DISPLAY_PARAMETERS)(IN ULONG Columns,
				ULONG Rows);

typedef VOID STDCALL
(*PDEVICE_CONTROL_COMPLETION)(IN PDEVICE_CONTROL_CONTEXT ControlContext);

typedef NTSTATUS STDCALL
(*pHalDeviceControl)(IN PDEVICE_HANDLER_OBJECT DeviceHandler,
		     IN PDEVICE_OBJECT DeviceObject,
		     IN ULONG ControlCode,
		     IN OUT PVOID Buffer OPTIONAL,
		     IN OUT PULONG BufferLength OPTIONAL,
		     IN PVOID Context,
		     IN PDEVICE_CONTROL_COMPLETION CompletionRoutine);

typedef VOID FASTCALL
(*pHalExamineMBR)(IN PDEVICE_OBJECT DeviceObject,
		  IN ULONG SectorSize,
		  IN ULONG MBRTypeIdentifier,
		  OUT PVOID *Buffer);

typedef VOID FASTCALL
(*pHalIoAssignDriveLetters)(IN PLOADER_PARAMETER_BLOCK LoaderBlock,
			    IN PSTRING NtDeviceName,
			    OUT PUCHAR NtSystemPath,
			    OUT PSTRING NtSystemPathString);

typedef NTSTATUS FASTCALL
(*pHalIoReadPartitionTable)(IN PDEVICE_OBJECT DeviceObject,
			    IN ULONG SectorSize,
			    IN BOOLEAN ReturnRecognizedPartitions,
			    OUT PDRIVE_LAYOUT_INFORMATION *PartitionBuffer);

typedef NTSTATUS FASTCALL
(*pHalIoSetPartitionInformation)(IN PDEVICE_OBJECT DeviceObject,
				 IN ULONG SectorSize,
				 IN ULONG PartitionNumber,
				 IN ULONG PartitionType);

typedef NTSTATUS FASTCALL
(*pHalIoWritePartitionTable)(IN PDEVICE_OBJECT DeviceObject,
			     IN ULONG SectorSize,
			     IN ULONG SectorsPerTrack,
			     IN ULONG NumberOfHeads,
			     IN PDRIVE_LAYOUT_INFORMATION PartitionBuffer);

typedef PBUS_HANDLER FASTCALL
(*pHalHandlerForBus)(IN INTERFACE_TYPE InterfaceType,
		     IN ULONG BusNumber);

typedef VOID FASTCALL
(*pHalReferenceBusHandler)(IN PBUS_HANDLER BusHandler);

typedef NTSTATUS STDCALL
(*pHalQuerySystemInformation)(IN HAL_QUERY_INFORMATION_CLASS InformationClass,
			      IN ULONG BufferSize,
			      IN OUT PVOID Buffer,
			      OUT PULONG ReturnedLength);


typedef NTSTATUS STDCALL
(*pHalSetSystemInformation)(IN HAL_SET_INFORMATION_CLASS InformationClass,
			    IN ULONG BufferSize,
			    IN PVOID Buffer);


typedef NTSTATUS STDCALL
(*pHalQueryBusSlots)(IN PBUS_HANDLER BusHandler,
		     IN ULONG BufferSize,
		     OUT PULONG SlotNumbers,
		     OUT PULONG ReturnedLength);

/* FIXME: Make this NT5+ Compatible (version 2 or 3) */
typedef struct _HAL_DISPATCH
{
  ULONG				Version;
  pHalQuerySystemInformation	HalQuerySystemInformation;
  pHalSetSystemInformation	HalSetSystemInformation;
  pHalQueryBusSlots		HalQueryBusSlots;
  pHalDeviceControl		HalDeviceControl;
  pHalExamineMBR		HalExamineMBR;
  pHalIoAssignDriveLetters	HalIoAssignDriveLetters;
  pHalIoReadPartitionTable	HalIoReadPartitionTable;
  pHalIoSetPartitionInformation	HalIoSetPartitionInformation;
  pHalIoWritePartitionTable	HalIoWritePartitionTable;
  pHalHandlerForBus		HalReferenceHandlerForBus;
  pHalReferenceBusHandler	HalReferenceBusHandler;
  pHalReferenceBusHandler	HalDereferenceBusHandler;
} HAL_DISPATCH, *PHAL_DISPATCH;

#ifdef __NTOSKRNL__
#define HALDISPATCH (&HalDispatchTable)
#else
#define HALDISPATCH ((PHAL_DISPATCH)&HalDispatchTable)
#endif
#endif

