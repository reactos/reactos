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
/* $Id: io.h,v 1.12 2001/08/26 17:25:29 ekohl Exp $
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
#define DNF_MADEUP                              0x0001
#define DNF_HAL_NODE                            0x0002
#define DNF_PROCESSED                           0x0004
#define DNF_ENUMERATED                          0x0008
#define DNF_ADDED                               0x0010
#define DNF_HAS_BOOT_CONFIG                     0x0020
#define DNF_BOOT_CONFIG_RESERVED                0x0040
#define DNF_RESOURCE_ASSIGNED                   0x0080
#define DNF_NO_RESOURCE_REQUIRED                0x0100
#define DNF_STARTED                             0x0200
#define DNF_LEGACY_DRIVER                       0x0400
#define DNF_RESOURCE_REQUIREMENTS_NEED_FILTERED 0x0800
#define DNF_HAS_PROBLEM                         0x1000

/* For UserFlags field */
#define DNUF_DONT_SHOW_IN_UI    0x0002
#define DNUF_NOT_DISABLEABLE    0x0008

/* For Problem field */
#define CM_PROB_FAILED_INSTALL  0x0001

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
IopInitializeDriver(PDRIVER_INITIALIZE DriverEntry,
                    PDEVICE_NODE ParentDeviceNode,
                    PUNICODE_STRING Filename,
                    BOOLEAN BootDriversOnly);
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

#endif
