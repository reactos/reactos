/*
 *  ReactOS kernel
 *  Copyright (C) 2000 David Welch <welch@cwcom.net>
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
/* $Id: io.h,v 1.13 2001/09/01 15:36:44 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/internal/io.h
 * PURPOSE:         Internal io manager declarations
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *               28/05/97: Created
 */

#ifndef __NTOSKRNL_INCLUDE_INTERNAL_IO_H
#define __NTOSKRNL_INCLUDE_INTERNAL_IO_H

#include <ddk/ntddk.h>
#include <internal/ob.h>

typedef struct _DEVICE_NODE
{
  struct _DEVICE_NODE *Parent;
  struct _DEVICE_NODE *PrevSibling;
  struct _DEVICE_NODE *NextSibling;
  struct _DEVICE_NODE *Child;
  PDRIVER_OBJECT DriverObject;
  PDEVICE_OBJECT Pdo;
  UNICODE_STRING InstancePath;
  UNICODE_STRING ServiceName;
  //TargetDeviceNotifyList?
  DEVICE_CAPABILITIES CapabilityFlags;
  ULONG Flags;
  ULONG UserFlags;
  ULONG DisableableDepends;
  ULONG Problem;
  PCM_RESOURCE_LIST CmResourceList;
  PCM_RESOURCE_LIST BootResourcesList;
  PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList;
} DEVICE_NODE, *PDEVICE_NODE;

/* For Flags field */
#define DNF_PROCESSED                           0x00000001
#define DNF_STARTED                             0x00000002
#define DNF_START_FAILED                        0x00000004
#define DNF_ENUMERATED                          0x00000008
#define DNF_DELETED                             0x00000010
#define DNF_MADEUP                              0x00000020
#define DNF_START_REQUEST_PENDING               0x00000040
#define DNF_NO_RESOURCE_REQUIRED                0x00000080
#define DNF_INSUFFICIENT_RESOURCES              0x00000100
#define DNF_RESOURCE_ASSIGNED                   0x00000200
#define DNF_RESOURCE_REPORTED                   0x00000400
#define DNF_HAL_NODE                            0x00000800 // ???
#define DNF_ADDED                               0x00001000
#define DNF_ADD_FAILED                          0x00002000
#define DNF_LEGACY_DRIVER                       0x00004000
#define DNF_STOPPED                             0x00008000
#define DNF_WILL_BE_REMOVED                     0x00010000
#define DNF_NEED_TO_ENUM                        0x00020000
#define DNF_NOT_CONFIGURED                      0x00040000
#define DNF_REINSTALL                           0x00080000
#define DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED 0x00100000 // ???
#define DNF_DISABLED                            0x00200000
#define DNF_RESTART_OK                          0x00400000
#define DNF_NEED_RESTART                        0x00800000
#define DNF_VISITED                             0x01000000
#define DNF_ASSIGNING_RESOURCES                 0x02000000
#define DNF_BEEING_ENUMERATED                   0x04000000
#define DNF_NEED_ENUMERATION_ONLY               0x08000000
#define DNF_LOCKED                              0x10000000
#define DNF_HAS_BOOT_CONFIG                     0x20000000
#define DNF_BOOT_CONFIG_RESERVED                0x40000000
#define DNF_HAS_PROBLEM                         0x80000000 // ???

/* For UserFlags field */
#define DNUF_DONT_SHOW_IN_UI    0x0002
#define DNUF_NOT_DISABLEABLE    0x0008

/* For Problem field */
#define CM_PROB_NOT_CONFIGURED  1
#define CM_PROB_FAILED_START    10
#define CM_PROB_NORMAL_CONFLICT 12
#define CM_PROB_NEED_RESTART    14
#define CM_PROB_REINSTALL       18
#define CM_PROB_WILL_BE_REMOVED 21
#define CM_PROB_DISABLED        22
#define CM_PROB_FAILED_INSTALL  28
#define CM_PROB_FAILED_ADD      31

extern PDEVICE_NODE IopRootDeviceNode;

extern POBJECT_TYPE IoSymbolicLinkType;

VOID
PnpInit(VOID);

NTSTATUS
STDCALL
PnpRootDriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath);
NTSTATUS
PnpRootCreateDevice(
  PDEVICE_OBJECT *PhysicalDeviceObject);

NTSTATUS
IopGetSystemPowerDeviceObject(PDEVICE_OBJECT *DeviceObject);
NTSTATUS
IopCreateDeviceNode(PDEVICE_NODE ParentNode,
                    PDEVICE_OBJECT PhysicalDeviceObject,
                    PDEVICE_NODE *DeviceNode);
NTSTATUS
IopFreeDeviceNode(PDEVICE_NODE DeviceNode);
NTSTATUS
IopInterrogateBusExtender(PDEVICE_NODE DeviceNode,
                          PDEVICE_OBJECT FunctionDeviceObject,
                          BOOLEAN BootDriversOnly);
VOID
IopLoadBootStartDrivers(VOID);

NTSTATUS
IopCreateDriverObject(PDRIVER_OBJECT *DriverObject);
NTSTATUS
IopInitializeDeviceNodeService(PDEVICE_NODE DeviceNode);
NTSTATUS
IopInitializeDriver(PDRIVER_INITIALIZE DriverEntry,
                    PDEVICE_NODE DeviceNode);
VOID 
IoInitCancelHandling(VOID);
VOID 
IoInitSymbolicLinkImplementation(VOID);
VOID 
IoInitFileSystemImplementation(VOID);
VOID 
IoInitVpbImplementation (VOID);

NTSTATUS IoTryToMountStorageDevice(PDEVICE_OBJECT DeviceObject);
POBJECT IoOpenSymlink(POBJECT SymbolicLink);
POBJECT IoOpenFileOnDevice(POBJECT SymbolicLink, PWCHAR Name);

PIRP IoBuildFilesystemControlRequest(ULONG MinorFunction,
				     PDEVICE_OBJECT DeviceObject,
				     PKEVENT UserEvent,
				     PIO_STATUS_BLOCK IoStatusBlock,
				     PDEVICE_OBJECT DeviceToMount);
VOID IoSecondStageCompletion(PIRP Irp, CCHAR PriorityBoost);

NTSTATUS STDCALL
IopCreateFile(PVOID ObjectBody,
	      PVOID Parent,
	      PWSTR RemainingPath,
	      POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS STDCALL
IopCreateDevice(PVOID ObjectBody,
		PVOID Parent,
		PWSTR RemainingPath,
		POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS IoAttachVpb(PDEVICE_OBJECT DeviceObject);

PIRP IoBuildSynchronousFsdRequestWithMdl(ULONG MajorFunction,
					 PDEVICE_OBJECT DeviceObject,
					 PMDL Mdl,
					 PLARGE_INTEGER StartingOffset,
					 PKEVENT Event,
					 PIO_STATUS_BLOCK IoStatusBlock,
					 ULONG PagingIo);

VOID IoInitShutdownNotification(VOID);
VOID IoShutdownRegisteredDevices(VOID);
VOID IoShutdownRegisteredFileSystems(VOID);

NTSTATUS STDCALL 
IoPageRead (PFILE_OBJECT		FileObject,
	    PMDL			Mdl,
	    PLARGE_INTEGER		Offset,
	    PIO_STATUS_BLOCK	StatusBlock,
	    BOOLEAN PagingIo);
NTSTATUS STDCALL IoPageWrite (PFILE_OBJECT		FileObject,
			      PMDL			Mdl,
			      PLARGE_INTEGER		Offset,
			      PIO_STATUS_BLOCK	StatusBlock);

NTSTATUS
IopInitiatePnpIrp(
  PDEVICE_OBJECT DeviceObject,
  PIO_STATUS_BLOCK IoStatusBlock,
  ULONG MinorFunction,
  PIO_STACK_LOCATION Stack);

#endif
