/*
 * Copyright (C) 1998-2005 ReactOS Team (and the authors from the programmers section)
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.
 *
 *
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/section.c
 * PURPOSE:         Implements section objects
 *
 * PROGRAMMERS:     Rex Jolliff
 *                  David Welch
 *                  Eric Kohl
 *                  Emanuele Aliberti
 *                  Eugene Ingerman
 *                  Casper Hornstrup
 *                  KJK::Hyperion
 *                  Guido de Jong
 *                  Ge van Geldorp
 *                  Royce Mitchell III
 *                  Filip Navara
 *                  Aleksey Bragin
 *                  Jason Filby
 *                  Thomas Weidenmueller
 *                  Gunnar Andre' Dalsnes
 *                  Mike Nordell
 *                  Alex Ionescu
 *                  Gregor Anich
 *                  Steven Edwards
 *                  Herve Poussineau
 */

/* INCLUDES *****************************************************************/

#include <ntoskrnl.h>
#include "newmm.h"
#define NDEBUG
#include <debug.h>
#include <reactos/exeformat.h>

#if defined (ALLOC_PRAGMA)
#pragma alloc_text(INIT, MmCreatePhysicalMemorySection)
#pragma alloc_text(INIT, MmInitSectionImplementation)
#endif

KEVENT CcpLazyWriteEvent;

PDEVICE_OBJECT
NTAPI
MmGetDeviceObjectForFile(IN PFILE_OBJECT FileObject)
{
    return IoGetRelatedDeviceObject(FileObject);
}

NTSTATUS
NTAPI
MiSimpleReadComplete
(PDEVICE_OBJECT DeviceObject,
 PIRP Irp,
 PVOID Context)
{
    PMDL Mdl = Irp->MdlAddress;

   /* Unlock MDL Pages, page 167. */
	DPRINT("MiSimpleReadComplete %x\n", Irp);
    while (Mdl)
    {
		DPRINT("MDL Unlock %x\n", Mdl);
		MmUnlockPages(Mdl);
        Mdl = Mdl->Next;
    }
	
    /* Check if there's an MDL */
    while ((Mdl = Irp->MdlAddress))
    {
        /* Clear all of them */
        Irp->MdlAddress = Mdl->Next;
        IoFreeMdl(Mdl);
    }

    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiSimpleRead
(PFILE_OBJECT FileObject, 
 PLARGE_INTEGER FileOffset,
 PVOID Buffer, 
 ULONG Length,
#ifdef __ROS_CMAKE__
 BOOLEAN Paging,
#endif
 PIO_STATUS_BLOCK ReadStatus)
{
    NTSTATUS Status;
    PIRP Irp = NULL;
    KEVENT ReadWait;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION IrpSp;
    
    ASSERT(FileObject);
    ASSERT(FileOffset);
    ASSERT(Buffer);
    ASSERT(ReadStatus);
    
    DeviceObject = MmGetDeviceObjectForFile(FileObject);
	ReadStatus->Status = STATUS_INTERNAL_ERROR;
	ReadStatus->Information = 0;
    
    ASSERT(DeviceObject);

    DPRINT
		("PAGING READ: FileObject %x <%wZ> Offset %08x%08x Length %d\n", 
		 &FileObject, 
		 &FileObject->FileName,
		 FileOffset->HighPart,
		 FileOffset->LowPart,
		 Length);

    KeInitializeEvent(&ReadWait, NotificationEvent, FALSE);
    
    Irp = IoBuildAsynchronousFsdRequest
		(IRP_MJ_READ,
		 DeviceObject,
		 Buffer,
		 Length,
		 FileOffset,
		 ReadStatus);
    
    if (!Irp)
    {
		return STATUS_NO_MEMORY;
    }

#ifndef __ROS_CMAKE__
    Irp->Flags |= IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO | IRP_NOCACHE | IRP_SYNCHRONOUS_API;
#else
    Irp->Flags |= (Paging ? IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO | IRP_NOCACHE : 0) | IRP_SYNCHRONOUS_API;
#endif

    Irp->UserEvent = &ReadWait;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IrpSp = IoGetNextIrpStackLocation(Irp);
	IrpSp->Control |= SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR;
    IrpSp->FileObject = FileObject;
    IrpSp->CompletionRoutine = MiSimpleReadComplete;

#ifdef __ROS_CMAKE__
    ObReferenceObject(FileObject);
#endif

    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
		DPRINT1("KeWaitForSingleObject(&ReadWait)\n");
		if (!NT_SUCCESS
			(KeWaitForSingleObject
			 (&ReadWait, 
			  Suspended, 
			  KernelMode, 
			  FALSE, 
			  NULL)))
		{
			DPRINT1("Warning: Failed to wait for synchronous IRP\n");
			ASSERT(FALSE);
			return Status;
		}
    }
    
    DPRINT("Paging IO Done: %08x\n", ReadStatus->Status);
	Status = 
		ReadStatus->Status == STATUS_END_OF_FILE ? 
		STATUS_SUCCESS : ReadStatus->Status;
    return Status;
}

NTSTATUS
NTAPI
_MiSimpleWrite
(PFILE_OBJECT FileObject, 
 PLARGE_INTEGER FileOffset,
 PVOID Buffer, 
 ULONG Length,
 PIO_STATUS_BLOCK ReadStatus,
 const char *File,
 int Line)
{
    NTSTATUS Status;
    PIRP Irp = NULL;
    KEVENT ReadWait;
    PDEVICE_OBJECT DeviceObject;
    PIO_STACK_LOCATION IrpSp;
    
    ASSERT(FileObject);
    ASSERT(FileOffset);
    ASSERT(Buffer);
    ASSERT(ReadStatus);
    
    ObReferenceObject(FileObject);
	DeviceObject = MmGetDeviceObjectForFile(FileObject);
    ASSERT(DeviceObject);
    
    DPRINT
		("PAGING WRITE: FileObject %x Offset %x Length %d (%s:%d)\n", 
		 &FileObject, 
		 FileOffset->LowPart,
		 Length,
		 File,
		 Line);
    
    KeInitializeEvent(&ReadWait, NotificationEvent, FALSE);

    Irp = IoBuildAsynchronousFsdRequest
		(IRP_MJ_WRITE,
		 DeviceObject,
		 Buffer,
		 Length,
		 FileOffset,
		 ReadStatus);
    
    if (!Irp)
    {
		ObDereferenceObject(FileObject);
		return STATUS_NO_MEMORY;
    }
    
    Irp->Flags = IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO | IRP_NOCACHE | IRP_SYNCHRONOUS_API;
    
    Irp->UserEvent = &ReadWait;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IrpSp = IoGetNextIrpStackLocation(Irp);
	IrpSp->Control |= SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR;
    IrpSp->FileObject = FileObject;
    IrpSp->CompletionRoutine = MiSimpleReadComplete;
    
	DPRINT("Call Driver\n");
    Status = IoCallDriver(DeviceObject, Irp);
	DPRINT("Status %x\n", Status);

	ObDereferenceObject(FileObject);

    if (Status == STATUS_PENDING)
    {
		DPRINT1("KeWaitForSingleObject(&ReadWait)\n");
		if (!NT_SUCCESS
			(KeWaitForSingleObject
			 (&ReadWait, 
			  Suspended, 
			  KernelMode, 
			  FALSE, 
			  NULL)))
		{
			DPRINT1("Warning: Failed to wait for synchronous IRP\n");
			ASSERT(FALSE);
			return Status;
		}
    }
    
    DPRINT("Paging IO Done: %08x\n", ReadStatus->Status);
    return ReadStatus->Status;
}

extern KEVENT MpwThreadEvent;
FAST_MUTEX MiWriteMutex;

NTSTATUS
NTAPI
_MiWriteBackPage
(PFILE_OBJECT FileObject,
 PLARGE_INTEGER FileOffset,
 ULONG Length,
 PFN_NUMBER Page,
 const char *File,
 int Line)
{
	NTSTATUS Status;
	PVOID Hyperspace;
	IO_STATUS_BLOCK Iosb;
	KIRQL OldIrql;
	PVOID PageBuffer = ExAllocatePool(NonPagedPool, PAGE_SIZE);

	if (!PageBuffer) return STATUS_NO_MEMORY;

	OldIrql = KfRaiseIrql(DISPATCH_LEVEL);
	Hyperspace = MmCreateHyperspaceMapping(Page);
	RtlCopyMemory(PageBuffer, Hyperspace, PAGE_SIZE);
	MmDeleteHyperspaceMapping(Hyperspace);
	KfLowerIrql(OldIrql);

	DPRINT1("MiWriteBackPage(%wZ,%08x%08x,%s:%d)\n", &FileObject->FileName, FileOffset->u.HighPart, FileOffset->u.LowPart, File, Line);
	Status = MiSimpleWrite
		(FileObject,
		 FileOffset,
		 PageBuffer,
		 Length,
		 &Iosb);

	ExFreePool(PageBuffer);
	
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("MiSimpleWrite failed (%x)\n", Status);
	}

	return Status;
}
