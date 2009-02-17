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

PFILE_OBJECT
NTAPI
MmGetFileObjectForSection(IN PROS_SECTION_OBJECT Section)
{
    PAGED_CODE();
    ASSERT(Section);

    /* Return the file object */
    return Section->FileObject; // Section->ControlArea->FileObject on NT
}

NTSTATUS
NTAPI
MmGetFileNameForSection(IN PROS_SECTION_OBJECT Section,
                        OUT POBJECT_NAME_INFORMATION *ModuleName)
{
    POBJECT_NAME_INFORMATION ObjectNameInfo;
    NTSTATUS Status;
    ULONG ReturnLength;

    /* Make sure it's an image section */
    *ModuleName = NULL;
    if (!(Section->AllocationAttributes & SEC_IMAGE))
    {
        /* It's not, fail */
        return STATUS_SECTION_NOT_IMAGE;
    }

    /* Allocate memory for our structure */
    ObjectNameInfo = ExAllocatePoolWithTag(PagedPool,
                                           1024,
                                           TAG('M', 'm', ' ', ' '));
    if (!ObjectNameInfo) return STATUS_NO_MEMORY;

    /* Query the name */
    Status = ObQueryNameString(Section->FileObject,
                               ObjectNameInfo,
                               1024,
                               &ReturnLength);
    if (!NT_SUCCESS(Status))
    {
        /* Failed, free memory */
        ExFreePool(ObjectNameInfo);
        return Status;
    }

    /* Success */
    *ModuleName = ObjectNameInfo;
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MmGetFileNameForAddress(IN PVOID Address,
                        OUT PUNICODE_STRING ModuleName)
{
    /*
     * FIXME: TODO.
     * Filip says to get the MM_AVL_TABLE from EPROCESS,
     * then use the MmMarea routines to locate the Marea that
     * corresponds to the address. Then make sure it's a section
     * view type (MEMORY_AREA_SECTION_VIEW) and use the marea's
     * per-type union to get the .u.SectionView.Section pointer to
     * the SECTION_OBJECT. Then we can use MmGetFileNameForSection
     * to get the full filename.
     */
    RtlCreateUnicodeString(ModuleName, L"C:\\ReactOS\\system32\\ntdll.dll");
    return STATUS_SUCCESS;
}

NTSTATUS
NTAPI
MiSimpleReadComplete
(PDEVICE_OBJECT DeviceObject,
 PIRP Irp,
 PVOID Context)
{
    /* Unlock MDL Pages, page 167. */
    PMDL Mdl = Irp->MdlAddress;
    while (Mdl)
    {
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
    
    ASSERT(DeviceObject);
    
//#if 0
    DPRINT1
		("PAGING READ: FileObject %x Offset %x Length %d\n", 
		 &FileObject, 
		 FileOffset->LowPart,
		 Length);
//#endif
    
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
    
    Irp->Flags |= IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO | IRP_NOCACHE;
    
    ObReferenceObject(FileObject);
    
    Irp->UserEvent = &ReadWait;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->FileObject = FileObject;
    IrpSp->CompletionRoutine = MiSimpleReadComplete;
    
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
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
			ObDereferenceObject(FileObject);
			return Status;
		}
    }
    
    ObDereferenceObject(FileObject);
    
    DPRINT1("Paging IO Done: %08x\n", ReadStatus->Status);
	Status = 
		ReadStatus->Status == STATUS_END_OF_FILE ? 
		STATUS_SUCCESS : ReadStatus->Status;
    return Status;
}

NTSTATUS
NTAPI
MiSimpleWrite
(PFILE_OBJECT FileObject, 
 PLARGE_INTEGER FileOffset,
 PVOID Buffer, 
 ULONG Length,
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
    
    ASSERT(DeviceObject);
    
    DPRINT1
		("PAGING WRITE: FileObject %x Offset %x Length %d\n", 
		 &FileObject, 
		 FileOffset->LowPart,
		 Length);
    
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
		return STATUS_NO_MEMORY;
    }
    
    Irp->Flags |= IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO | IRP_NOCACHE;
    
    ObReferenceObject(FileObject);
    
    Irp->UserEvent = &ReadWait;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IrpSp = IoGetNextIrpStackLocation(Irp);
    IrpSp->FileObject = FileObject;
    IrpSp->CompletionRoutine = MiSimpleReadComplete;
    
    Status = IoCallDriver(DeviceObject, Irp);
    if (Status == STATUS_PENDING)
    {
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
			ObDereferenceObject(FileObject);
			return Status;
		}
    }
    
    ObDereferenceObject(FileObject);
    
    DPRINT1("Paging IO Done: %08x\n", ReadStatus->Status);
    return ReadStatus->Status;
}

typedef struct _WRITE_SCHEDULE_ENTRY {
	LIST_ENTRY Entry;
	PFILE_OBJECT FileObject;
	LARGE_INTEGER FileOffset;
	ULONG Length;
	PFN_TYPE Page;
} WRITE_SCHEDULE_ENTRY, *PWRITE_SCHEDULE_ENTRY;

CLIENT_ID MiWriteThreadId;
HANDLE MiWriteThreadHandle;
KEVENT MiWriteEvent;
FAST_MUTEX MiWriteMutex;
LIST_ENTRY MiWriteScheduleListHead;

NTSTATUS
NTAPI
MiScheduleForWrite
(PFILE_OBJECT FileObject,
 PLARGE_INTEGER FileOffset,
 PFN_TYPE Page,
 ULONG Length)
{
	PWRITE_SCHEDULE_ENTRY WriteEntry = 
		ExAllocatePool(NonPagedPool, sizeof(WRITE_SCHEDULE_ENTRY));

	if (WriteEntry == NULL)
		return STATUS_NO_MEMORY;

	ObReferenceObject(FileObject);
	WriteEntry->FileObject = FileObject;
	WriteEntry->FileOffset = *FileOffset;
	WriteEntry->Length = Length;
	MmReferencePage(Page);
	WriteEntry->Page = Page;

	ExAcquireFastMutex(&MiWriteMutex);
	InsertTailList(&MiWriteScheduleListHead, &WriteEntry->Entry);
	ExReleaseFastMutex(&MiWriteMutex);

	KeSetEvent(&MiWriteEvent, IO_NO_INCREMENT, FALSE);

	return STATUS_SUCCESS;
}

VOID
NTAPI
MiWriteThread(PVOID Unused)
{
	BOOLEAN Complete;
	NTSTATUS Status;
	PVOID Hyperspace;
	LIST_ENTRY OldHead;
	PLIST_ENTRY Entry;
	IO_STATUS_BLOCK Iosb;
	PWRITE_SCHEDULE_ENTRY WriteEntry;

	while (TRUE)
	{
		ExAcquireFastMutex(&MiWriteMutex);
		Complete = IsListEmpty(&MiWriteScheduleListHead);
		ExReleaseFastMutex(&MiWriteMutex);
		if (Complete)
		{
			DPRINT1("No items await writing\n");
			KeSetEvent(&CcpLazyWriteEvent, IO_NO_INCREMENT, FALSE);
			KeWaitForSingleObject
				(&MiWriteEvent,
				 Executive,
				 KernelMode,
				 TRUE,
				 NULL);
		}

		DPRINT1("Lazy write items are available\n");
		KeResetEvent(&CcpLazyWriteEvent);

		ExAcquireFastMutex(&MiWriteMutex);
		RtlCopyMemory(&OldHead, &MiWriteScheduleListHead, sizeof(OldHead));
		OldHead.Flink->Blink = &OldHead;
		OldHead.Blink->Flink = &OldHead;
		InitializeListHead(&MiWriteScheduleListHead);
		ExReleaseFastMutex(&MiWriteMutex);
		
		for (Entry = OldHead.Flink;
			 !IsListEmpty(&OldHead);
			 Entry = OldHead.Flink)
		{
			WriteEntry = CONTAINING_RECORD(Entry, WRITE_SCHEDULE_ENTRY, Entry);
			Hyperspace = MmCreateHyperspaceMapping(WriteEntry->Page);
			
			Status = MiSimpleWrite
				(WriteEntry->FileObject,
				 &WriteEntry->FileOffset,
				 Hyperspace,
				 WriteEntry->Length,
				 &Iosb);

			if (!NT_SUCCESS(Status))
			{
				DPRINT1("MiSimpleWrite failed (%x)\n", Status);
			}
			
			MmDeleteHyperspaceMapping(Hyperspace);
			MmDereferencePage(WriteEntry->Page);
			ObDereferenceObject(WriteEntry->FileObject);
			RemoveEntryList(&WriteEntry->Entry);
			ExFreePool(WriteEntry);
		}

		DPRINT1("Finished a lazy write pass\n");
	}
}

NTSTATUS
NTAPI
MmWriteThreadInit()
{
	KeInitializeEvent(&MiWriteEvent, SynchronizationEvent, FALSE);
	ExInitializeFastMutex(&MiWriteMutex);
	InitializeListHead(&MiWriteScheduleListHead);
	return PsCreateSystemThread
		(&MiWriteThreadHandle,
		 THREAD_ALL_ACCESS,
		 NULL,
		 NULL,
		 &MiWriteThreadId,
		 (PKSTART_ROUTINE) MiWriteThread,
		 NULL);
}

