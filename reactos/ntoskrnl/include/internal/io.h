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
/* $Id: io.h,v 1.9 2001/04/03 17:25:49 dwelch Exp $
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

extern POBJECT_TYPE		IoSymbolicLinkType;

/*
 * FUNCTION: Called to initalize a loaded driver
 * ARGUMENTS: 
 *          entry = pointer to the driver initialization routine
 * RETURNS: Success or failure
 */
NTSTATUS 
IoInitializeDriver(PDRIVER_INITIALIZE DriverEntry);

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
