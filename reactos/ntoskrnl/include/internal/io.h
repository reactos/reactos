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
/* $Id: io.h,v 1.23 2002/08/28 07:13:04 hbirr Exp $
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
  PDEVICE_CAPABILITIES CapabilityFlags;
  ULONG Flags;
  ULONG UserFlags;
  ULONG DisableableDepends;
  ULONG Problem;
  PCM_RESOURCE_LIST CmResourceList;
  PCM_RESOURCE_LIST BootResourcesList;
  PIO_RESOURCE_REQUIREMENTS_LIST ResourceRequirementsList;
  /* Not NT's */
  UNICODE_STRING DeviceID;
  UNICODE_STRING InstanceID;
  UNICODE_STRING HardwareIDs;
  UNICODE_STRING CompatibleIDs;
  UNICODE_STRING DeviceText;
  UNICODE_STRING DeviceTextLocation;
  PPNP_BUS_INFORMATION BusInformation;
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

/*
 * VOID
 * IopDeviceNodeSetFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Flag);
 */
#define IopDeviceNodeSetFlag(DeviceNode, Flag)((DeviceNode)->Flags |= (Flag))

/*
 * VOID
 * IopDeviceNodeClearFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Flag);
 */
#define IopDeviceNodeClearFlag(DeviceNode, Flag)((DeviceNode)->Flags &= ~(Flag))

/*
 * BOOLEAN
 * IopDeviceNodeHasFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Flag);
 */
#define IopDeviceNodeHasFlag(DeviceNode, Flag)(((DeviceNode)->Flags & (Flag)) > 0)

/*
 * VOID
 * IopDeviceNodeSetUserFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG UserFlag);
 */
#define IopDeviceNodeSetUserFlag(DeviceNode, UserFlag)((DeviceNode)->UserFlags |= (UserFlag))

/*
 * VOID
 * IopDeviceNodeClearUserFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG UserFlag);
 */
#define IopDeviceNodeClearUserFlag(DeviceNode, UserFlag)((DeviceNode)->UserFlags &= ~(UserFlag))

/*
 * BOOLEAN
 * IopDeviceNodeHasUserFlag(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG UserFlag);
 */
#define IopDeviceNodeHasUserFlag(DeviceNode, UserFlag)(((DeviceNode)->UserFlags & (UserFlag)) > 0)

 /*
 * VOID
 * IopDeviceNodeSetProblem(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Problem);
 */
#define IopDeviceNodeSetProblem(DeviceNode, Problem)((DeviceNode)->Problem |= (Problem))

/*
 * VOID
 * IopDeviceNodeClearProblem(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Problem);
 */
#define IopDeviceNodeClearProblem(DeviceNode, Problem)((DeviceNode)->Problem &= ~(Problem))

/*
 * BOOLEAN
 * IopDeviceNodeHasProblem(
 *   PDEVICE_NODE DeviceNode,
 *   ULONG Problem);
 */
#define IopDeviceNodeHasProblem(DeviceNode, Problem)(((DeviceNode)->Problem & (Problem)) > 0)


/*
   Called on every visit of a node during a preorder-traversal of the device
   node tree.
   If the routine returns STATUS_UNSUCCESSFUL the traversal will stop and
   STATUS_SUCCESS is returned to the caller who initiated the tree traversal.
   Any other returned status code will be returned to the caller. If a status
   code that indicates an error (other than STATUS_UNSUCCESSFUL) is returned,
   the traversal is stopped immediately and the status code is returned to
   the caller.
 */
typedef NTSTATUS (*DEVICETREE_TRAVERSE_ROUTINE)(
  PDEVICE_NODE DeviceNode,
  PVOID Context);

/* Context information for traversing the device tree */
typedef struct _DEVICETREE_TRAVERSE_CONTEXT
{
  /* Current device node during a traversal */
  PDEVICE_NODE DeviceNode;
  /* Initial device node where we start the traversal */
  PDEVICE_NODE FirstDeviceNode;
  /* Action routine to be called for every device node */
  DEVICETREE_TRAVERSE_ROUTINE Action;
  /* Context passed to the action routine */
  PVOID Context;
} DEVICETREE_TRAVERSE_CONTEXT, *PDEVICETREE_TRAVERSE_CONTEXT;

/*
 * VOID
 * IopInitDeviceTreeTraverseContext(
 *   PDEVICETREE_TRAVERSE_CONTEXT DeviceTreeTraverseContext,
 *   PDEVICE_NODE DeviceNode,
 *   DEVICETREE_TRAVERSE_ROUTINE Action,
 *   PVOID Context);
 */
#define IopInitDeviceTreeTraverseContext( \
  _DeviceTreeTraverseContext, _DeviceNode, _Action, _Context) { \
  (_DeviceTreeTraverseContext)->FirstDeviceNode = (_DeviceNode); \
  (_DeviceTreeTraverseContext)->Action = (_Action); \
  (_DeviceTreeTraverseContext)->Context = (_Context); }


extern PDEVICE_NODE IopRootDeviceNode;

extern POBJECT_TYPE IoSymbolicLinkType;

VOID
PnpInit(VOID);

VOID
IopInitDriverImplementation(VOID);

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
                          PDEVICE_OBJECT Pdo,
                          BOOLEAN BootDriversOnly);
VOID
IopLoadBootStartDrivers(VOID);
NTSTATUS
IopCreateDriverObject(PDRIVER_OBJECT *DriverObject,
		      PUNICODE_STRING ServiceName,
		      BOOLEAN FileSystemDriver);
NTSTATUS
IopInitializeDeviceNodeService(PDEVICE_NODE DeviceNode);
NTSTATUS
IopInitializeDriver(PDRIVER_INITIALIZE DriverEntry,
		    PDEVICE_NODE DeviceNode,
		    BOOLEAN FileSystemDriver);
VOID
IoInitCancelHandling(VOID);
VOID
IoInitSymbolicLinkImplementation(VOID);
VOID
IoInitFileSystemImplementation(VOID);
VOID
IoInitVpbImplementation (VOID);

NTSTATUS
IoMountVolume(IN PDEVICE_OBJECT DeviceObject,
	      IN BOOLEAN AllowRawMount);
POBJECT IoOpenSymlink(POBJECT SymbolicLink);
POBJECT IoOpenFileOnDevice(POBJECT SymbolicLink, PWCHAR Name);

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
IoPageWrite(PFILE_OBJECT	FileObject,
	    PMDL		Mdl,
	    PLARGE_INTEGER	Offset,
	    PKEVENT		Event,
	    PIO_STATUS_BLOCK	StatusBlock);

NTSTATUS
IoCreateArcNames(VOID);

NTSTATUS
IoCreateSystemRootLink(PCHAR ParameterLine);

NTSTATUS
IopInitiatePnpIrp(
  PDEVICE_OBJECT DeviceObject,
  PIO_STATUS_BLOCK IoStatusBlock,
  ULONG MinorFunction,
  PIO_STACK_LOCATION Stack);

BOOLEAN
IopCreateUnicodeString(
  PUNICODE_STRING	Destination,
  PWSTR Source,
  POOL_TYPE PoolType);


NTSTATUS
IoCreateDriverList(VOID);

NTSTATUS
IoDestroyDriverList(VOID);


/* pnproot.c */

NTSTATUS
STDCALL
PnpRootDriverEntry(
  IN PDRIVER_OBJECT DriverObject,
  IN PUNICODE_STRING RegistryPath);

NTSTATUS
PnpRootCreateDevice(
  PDEVICE_OBJECT *PhysicalDeviceObject);

#endif
