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

/*
 * FUNCTION:  Waits in kernel mode indefinitely for a file object lock.
 * ARGUMENTS: PFILE_OBJECT to wait for.
 * RETURNS:   Status of the wait.
 */
NTSTATUS
MmspWaitForFileLock(PFILE_OBJECT File)
{
    return STATUS_SUCCESS;
   //return KeWaitForSingleObject(&File->Lock, 0, KernelMode, FALSE, NULL);
}

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
    ObjectNameInfo = ExAllocatePoolWithTag(PagedPool, 1024, TAG_MM_PAGEOP);
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
   PROS_SECTION_OBJECT Section;
   PMEMORY_AREA MemoryArea;
   PMMSUPPORT AddressSpace;
   POBJECT_NAME_INFORMATION ModuleNameInformation;
   NTSTATUS Status = STATUS_ADDRESS_NOT_ASSOCIATED;

   /* Get the MM_AVL_TABLE from EPROCESS */
   if (Address >= MmSystemRangeStart)
   {
      AddressSpace = MmGetKernelAddressSpace();
   }
   else
   {
      AddressSpace = &PsGetCurrentProcess()->Vm;
   }

   /* Lock address space */
   MmLockAddressSpace(AddressSpace);

   /* Locate the memory area for the process by address */
   MemoryArea = MmLocateMemoryAreaByAddress(AddressSpace, Address);

   /* Make sure it's a section view type */
   if ((MemoryArea != NULL) && (MemoryArea->Type == MEMORY_AREA_SECTION_VIEW))
   {
      /* Get the section pointer to the SECTION_OBJECT */
      Section = MemoryArea->Data.SectionData.Section;

      /* Unlock address space */
      MmUnlockAddressSpace(AddressSpace);

      /* Get the filename of the section */
      Status = MmGetFileNameForSection(Section,&ModuleNameInformation);

      if (NT_SUCCESS(Status))
      {
         /* Init modulename */
         RtlCreateUnicodeString(ModuleName,
                                ModuleNameInformation->Name.Buffer);

         /* Free temp taged buffer from MmGetFileNameForSection() */
         ExFreePoolWithTag(ModuleNameInformation, '  mM');
         DPRINT("Found ModuleName %S by address %p\n",
                ModuleName->Buffer,Address);
      }
   }
   else
   {
      /* Unlock address space */
      MmUnlockAddressSpace(AddressSpace);
   }

   return Status;
}

NTSTATUS
NTAPI
MiSimpleReadComplete
(PDEVICE_OBJECT DeviceObject,
 PIRP Irp,
 PVOID Context)
{
   /* Unlock MDL Pages, page 167. */
	DPRINT("MiSimpleReadComplete %x\n", Irp);
    PMDL Mdl = Irp->MdlAddress;
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
    
    Irp->Flags |= IRP_PAGING_IO | IRP_SYNCHRONOUS_PAGING_IO | IRP_NOCACHE | IRP_SYNCHRONOUS_API;
    
    Irp->UserEvent = &ReadWait;
    Irp->Tail.Overlay.OriginalFileObject = FileObject;
    Irp->Tail.Overlay.Thread = PsGetCurrentThread();
    IrpSp = IoGetNextIrpStackLocation(Irp);
	IrpSp->Control |= SL_INVOKE_ON_SUCCESS | SL_INVOKE_ON_ERROR;
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
    
    ObReferenceObject(FileObject);
	DeviceObject = MmGetDeviceObjectForFile(FileObject);
    ASSERT(DeviceObject);
    
    DPRINT
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

typedef struct _WRITE_SCHEDULE_ENTRY {
	LIST_ENTRY Entry;
	PFILE_OBJECT FileObject;
	LARGE_INTEGER FileOffset;
	ULONG Length;
	PFN_TYPE Page;
} WRITE_SCHEDULE_ENTRY, *PWRITE_SCHEDULE_ENTRY;

extern KEVENT MpwThreadEvent;
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

	KeSetEvent(&MpwThreadEvent, IO_NO_INCREMENT, FALSE);

	return STATUS_SUCCESS;
}


NTSTATUS
NTAPI
MiWriteBackPage
(PFILE_OBJECT FileObject,
 PLARGE_INTEGER FileOffset,
 ULONG Length,
 PFN_TYPE Page)
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

	DPRINT("MiWriteBackPage(%wZ,%08x%08x)\n", &FileObject->FileName, FileOffset->u.HighPart, FileOffset->u.LowPart);
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

VOID
NTAPI
MiWriteThread()
{
	BOOLEAN Complete;
	NTSTATUS Status;
	LIST_ENTRY OldHead;
	PLIST_ENTRY Entry;
	PWRITE_SCHEDULE_ENTRY WriteEntry;

	ExAcquireFastMutex(&MiWriteMutex);
	Complete = IsListEmpty(&MiWriteScheduleListHead);
	ExReleaseFastMutex(&MiWriteMutex);
	if (Complete)
	{
		DPRINT1("No items await writing\n");
		KeSetEvent(&CcpLazyWriteEvent, IO_NO_INCREMENT, FALSE);
	}
	
	DPRINT("Lazy write items are available\n");
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
		Status = MiWriteBackPage(WriteEntry->FileObject, &WriteEntry->FileOffset, WriteEntry->Length, WriteEntry->Page);

		if (!NT_SUCCESS(Status))
		{
			DPRINT1("MiSimpleWrite failed (%x)\n", Status);
		}

		MmDereferencePage(WriteEntry->Page);
		ObDereferenceObject(WriteEntry->FileObject);
		RemoveEntryList(&WriteEntry->Entry);
		ExFreePool(WriteEntry);
	}
}
