/* $Id: io.h,v 1.10 2000/03/29 13:11:51 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            include/internal/io.h
 * PURPOSE:         Internal io manager declarations
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *               28/05/97: Created
 */

#ifndef __INCLUDE_INTERNAL_IO_H
#define __INCLUDE_INTERNAL_IO_H

#include <ddk/ntddk.h>
#include <internal/ob.h>

extern POBJECT_TYPE EXPORTED	IoDeviceObjectType;
extern POBJECT_TYPE EXPORTED	IoFileObjectType;
extern POBJECT_TYPE		IoSymbolicLinkObjectType;

/*
 * FUNCTION: Called to initalize a loaded driver
 * ARGUMENTS: 
 *          entry = pointer to the driver initialization routine
 * RETURNS: Success or failure
 */
NTSTATUS IoInitializeDriver(PDRIVER_INITIALIZE DriverEntry);



VOID IoInitCancelHandling(VOID);
VOID IoInitSymbolicLinkImplementation(VOID);
VOID IoInitFileSystemImplementation(VOID);

NTSTATUS IoTryToMountStorageDevice(PDEVICE_OBJECT DeviceObject);
POBJECT IoOpenSymlink(POBJECT SymbolicLink);
POBJECT IoOpenFileOnDevice(POBJECT SymbolicLink, PWCHAR Name);

PIRP IoBuildFilesystemControlRequest(ULONG MinorFunction,
				     PDEVICE_OBJECT DeviceObject,
				     PKEVENT UserEvent,
				     PIO_STATUS_BLOCK IoStatusBlock,
				     PDEVICE_OBJECT DeviceToMount);
VOID IoSecondStageCompletion(PIRP Irp, CCHAR PriorityBoost);

NTSTATUS IopCreateFile (PVOID ObjectBody,
			PVOID Parent,
			PWSTR RemainingPath,
			POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS IopCreateDevice(PVOID ObjectBody,
			 PVOID Parent,
			 PWSTR RemainingPath,
			 POBJECT_ATTRIBUTES ObjectAttributes);
NTSTATUS IoAttachVpb(PDEVICE_OBJECT DeviceObject);

PIRP IoBuildSynchronousFsdRequestWithMdl(ULONG MajorFunction,
					 PDEVICE_OBJECT DeviceObject,
					 PMDL Mdl,
					 PLARGE_INTEGER StartingOffset,
					 PKEVENT Event,
					 PIO_STATUS_BLOCK IoStatusBlock);
VOID IoShutdownIoManager(VOID);

#endif
