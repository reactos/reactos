/* $Id: section.c,v 1.29 2000/04/07 02:24:01 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/mm/section.c
 * PURPOSE:         Implements section objects
 * PROGRAMMER:      David Welch (welch@mcmail.com)
 * UPDATE HISTORY:
 *                  Created 22/05/98
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/mm.h>
#include <internal/ob.h>
#include <internal/io.h>
#include <internal/ps.h>
#include <wchar.h>

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE EXPORTED MmSectionObjectType = NULL;

/* FUNCTIONS *****************************************************************/

VOID MmSetPageEntrySection(PSECTION_OBJECT Section,
			   ULONG Offset,
			   PVOID Entry)
{
   PSECTION_PAGE_TABLE Table;
   ULONG DirectoryOffset;
   ULONG TableOffset;
   
   DirectoryOffset = PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(Offset);
   Table = Section->PageDirectory.PageTables[DirectoryOffset];
   if (Table == NULL)
     {
	Table = 
	  Section->PageDirectory.PageTables[DirectoryOffset] =
	  ExAllocatePool(NonPagedPool, sizeof(SECTION_PAGE_TABLE));
     }
   TableOffset = PAGE_TO_SECTION_PAGE_TABLE_OFFSET(Offset);
   Table->Pages[TableOffset] = Entry;
}

PVOID MmGetPageEntrySection(PSECTION_OBJECT Section,
			    ULONG Offset)
{
   PSECTION_PAGE_TABLE Table;
   PVOID Entry;
   ULONG DirectoryOffset;
   ULONG TableOffset;
   
   DirectoryOffset = PAGE_TO_SECTION_PAGE_DIRECTORY_OFFSET(Offset);
   Table = Section->PageDirectory.PageTables[DirectoryOffset];
   if (Table == NULL)
     {
	return(NULL);
     }
   TableOffset = PAGE_TO_SECTION_PAGE_TABLE_OFFSET(Offset);
   Entry = Table->Pages[TableOffset];
   return(Entry);
}

PVOID MiTryToSharePageInSection(PSECTION_OBJECT Section,
				ULONG Offset)
{
   KIRQL oldIrql;
   PLIST_ENTRY current_entry;
   PMEMORY_AREA current;
   PVOID Address;
   ULONG PhysPage;
   
   DPRINT("MiTryToSharePageInSection(Section %x, Offset %x)\n",
	  Section, Offset);
   
   KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
   
   current_entry = Section->ViewListHead.Flink;
   
   while (current_entry != &Section->ViewListHead)
     {
	current = CONTAINING_RECORD(current_entry, MEMORY_AREA,
				    Data.SectionData.ViewListEntry);
	
	if (current->Data.SectionData.ViewOffset <= Offset &&
	    (current->Data.SectionData.ViewOffset + current->Length) >= Offset)
	  {
	     Address = current->BaseAddress + 
	       (Offset - current->Data.SectionData.ViewOffset);
	     
	     PhysPage = MmGetPhysicalAddressForProcess(current->Process,
						       Address);
	     MmReferencePage((PVOID)PhysPage);
	     KeReleaseSpinLock(&Section->ViewListLock, oldIrql);
	     DPRINT("MiTryToSharePageInSection() = %x\n", PhysPage);
	     return((PVOID)PhysPage);
	  }
	
	current_entry = current_entry->Flink;
     }
   
   KeReleaseSpinLock(&Section->ViewListLock, oldIrql);
   DPRINT("MiTryToSharePageInSection() finished\n");
   return(NULL);
}

VOID MmpDeleteSection(PVOID ObjectBody)
{
   DPRINT("MmpDeleteSection(ObjectBody %x)\n", ObjectBody);
}

VOID MmpCloseSection(PVOID ObjectBody,
		     ULONG HandleCount)
{
   DPRINT("MmpCloseSection(OB %x, HC %d) RC %d\n",
	   ObjectBody, HandleCount, ObGetReferenceCount(ObjectBody));
}

NTSTATUS MmpCreateSection(PVOID ObjectBody,
			  PVOID Parent,
			  PWSTR RemainingPath,
			  POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   
   DPRINT("MmpCreateDevice(ObjectBody %x, Parent %x, RemainingPath %S)\n",
	  ObjectBody, Parent, RemainingPath);
   
   if (RemainingPath == NULL)
     {
	return(STATUS_SUCCESS);
     }
   
   if (wcschr(RemainingPath+1, L'\\') != NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
   
   Status = ObReferenceObjectByPointer(Parent,
				       STANDARD_RIGHTS_REQUIRED,
				       ObDirectoryType,
				       UserMode);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
   
   ObAddEntryDirectory(Parent, ObjectBody, RemainingPath+1);
   ObDereferenceObject(Parent);
   
   return(STATUS_SUCCESS);
}

NTSTATUS MmInitSectionImplementation(VOID)
{
   MmSectionObjectType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   RtlInitUnicodeString(&MmSectionObjectType->TypeName, L"Section");

   MmSectionObjectType->TotalObjects = 0;
   MmSectionObjectType->TotalHandles = 0;
   MmSectionObjectType->MaxObjects = ULONG_MAX;
   MmSectionObjectType->MaxHandles = ULONG_MAX;
   MmSectionObjectType->PagedPoolCharge = 0;
   MmSectionObjectType->NonpagedPoolCharge = sizeof(SECTION_OBJECT);
   MmSectionObjectType->Dump = NULL;
   MmSectionObjectType->Open = NULL;
   MmSectionObjectType->Close = MmpCloseSection;
   MmSectionObjectType->Delete = MmpDeleteSection;
   MmSectionObjectType->Parse = NULL;
   MmSectionObjectType->Security = NULL;
   MmSectionObjectType->QueryName = NULL;
   MmSectionObjectType->OkayToClose = NULL;
   MmSectionObjectType->Create = MmpCreateSection;
   
   return(STATUS_SUCCESS);
}


/* FIXME: NtCS should call MmCS */
NTSTATUS STDCALL NtCreateSection (OUT PHANDLE SectionHandle, 
				  IN ACCESS_MASK DesiredAccess,
				  IN POBJECT_ATTRIBUTES	ObjectAttributes OPTIONAL,
				  IN PLARGE_INTEGER MaximumSize	OPTIONAL,  
				  IN ULONG SectionPageProtection OPTIONAL,
				  IN ULONG AllocationAttributes,
				  IN HANDLE FileHandle OPTIONAL)
/*
 * FUNCTION: Creates a section object.
 * ARGUMENTS:
 *        SectionHandle (OUT) = Caller supplied storage for the resulting 
 *                              handle
 *        DesiredAccess = Specifies the desired access to the section can be a
 *                        combination of STANDARD_RIGHTS_REQUIRED | 
 *                        SECTION_QUERY | SECTION_MAP_WRITE |
 *                        SECTION_MAP_READ | SECTION_MAP_EXECUTE.
 *        ObjectAttribute = Initialized attributes for the object can be used 
 *                          to create a named section
 *        MaxiumSize = Maximizes the size of the memory section. Must be 
 *                     non-NULL for a page-file backed section. 
 *                     If value specified for a mapped file and the file is 
 *                     not large enough, file will be extended. 
 *        SectionPageProtection = Can be a combination of PAGE_READONLY | 
 *                                PAGE_READWRITE | PAGE_WRITEONLY | 
 *                                PAGE_WRITECOPY.
 *        AllocationAttributes = can be a combination of SEC_IMAGE | 
 *                               SEC_RESERVE
 *        FileHandle = Handle to a file to create a section mapped to a file 
 *                     instead of a memory backed section.
 * RETURNS: Status
 */
{
   PSECTION_OBJECT Section;
   NTSTATUS Status;
   
   DPRINT("NtCreateSection()\n");

   Section = ObCreateObject(SectionHandle,
			    DesiredAccess,
			    ObjectAttributes,
			    MmSectionObjectType);
   DPRINT("SectionHandle %x\n", SectionHandle);
   if (Section == NULL)
     {
	return(STATUS_UNSUCCESSFUL);
     }
     
   if (MaximumSize != NULL)
     {
	Section->MaximumSize = *MaximumSize;
     }
   else
     {
        Section->MaximumSize.QuadPart = 0xffffffff;
     }
   Section->SectionPageProtection = SectionPageProtection;
   Section->AllocateAttributes = AllocationAttributes;
   InitializeListHead(&Section->ViewListHead);
   KeInitializeSpinLock(&Section->ViewListLock);
   
   if (FileHandle != (HANDLE)0xffffffff)
     {
	Status = ObReferenceObjectByHandle(FileHandle,
					   FILE_READ_DATA,
					   IoFileObjectType,
					   UserMode,
					   (PVOID*)&Section->FileObject,
					   NULL);
	if (!NT_SUCCESS(Status))
	  {
	     /*
	      * Delete section object
	      */
	     DPRINT("NtCreateSection() = %x\n",Status);
	     ZwClose(SectionHandle);
	     ObDereferenceObject(Section);
	     return(Status);
	  }
     }
   else
     {
	Section->FileObject = NULL;
     }
      
   DPRINT("NtCreateSection() = STATUS_SUCCESS\n");
   ObDereferenceObject(Section);
   return(STATUS_SUCCESS);
}


/**********************************************************************
 * NAME
 * 	NtOpenSection
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * 	SectionHandle
 *
 * 	DesiredAccess
 *
 * 	ObjectAttributes
 *
 * RETURN VALUE
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL NtOpenSection(PHANDLE			SectionHandle,
			       ACCESS_MASK		DesiredAccess,
			       POBJECT_ATTRIBUTES	ObjectAttributes)
{
   PVOID		Object;
   NTSTATUS	Status;
   
   *SectionHandle = 0;

   Status = ObReferenceObjectByName(ObjectAttributes->ObjectName,
				    ObjectAttributes->Attributes,
				    NULL,
				    DesiredAccess,
				    MmSectionObjectType,
				    UserMode,
				    NULL,
				    &Object);
   if (!NT_SUCCESS(Status))
     {
	return Status;
     }
   
   Status = ObCreateHandle(PsGetCurrentProcess(),
			   Object,
			   DesiredAccess,
			   FALSE,
			   SectionHandle);
   ObDereferenceObject(Object);
   return(Status);
}


/**********************************************************************
 * NAME							EXPORTED
 *	NtMapViewOfSection
 *
 * DESCRIPTION
 * 	Maps a view of a section into the virtual address space of a 
 *	process.
 *	
 * ARGUMENTS
 *	SectionHandle
 *		Handle of the section.
 *		
 *	ProcessHandle
 *		Handle of the process.
 *		
 *	BaseAddress
 *		Desired base address (or NULL) on entry;
 *		Actual base address of the view on exit.
 *		
 *	ZeroBits
 *		Number of high order address bits that must be zero.
 *		
 *	CommitSize
 *		Size in bytes of the initially committed section of 
 *		the view.
 *		
 *	SectionOffset
 *		Offset in bytes from the beginning of the section
 *		to the beginning of the view.
 *		
 *	ViewSize
 *		Desired length of map (or zero to map all) on entry
 *		Actual length mapped on exit.
 *		
 *	InheritDisposition
 *		Specified how the view is to be shared with
 *              child processes.
 *              
 *	AllocateType
 *		Type of allocation for the pages.
 *		
 *	Protect
 *		Protection for the committed region of the view.
 *
 * RETURN VALUE
 * 	Status.
 */
NTSTATUS STDCALL NtMapViewOfSection(HANDLE SectionHandle,
				    HANDLE ProcessHandle,
				    PVOID* BaseAddress,
				    ULONG ZeroBits,
				    ULONG CommitSize,
				    PLARGE_INTEGER SectionOffset,
				    PULONG ViewSize,
				    SECTION_INHERIT	InheritDisposition,
				    ULONG AllocationType,
				    ULONG Protect)
{
   PSECTION_OBJECT	Section;
   PEPROCESS	Process;
   MEMORY_AREA	* Result;
   NTSTATUS	Status;
   KIRQL oldIrql;
   ULONG ViewOffset;
   PMADDRESS_SPACE AddressSpace;
   
   DPRINT("NtMapViewOfSection(Section:%08lx, Process:%08lx,\n"
	  "  Base:%08lx, ZeroBits:%08lx, CommitSize:%08lx,\n"
	  "  SectionOffs:%08lx, *ViewSize:%08lx, InheritDisp:%08lx,\n"
	  "  AllocType:%08lx, Protect:%08lx)\n",
	  SectionHandle, ProcessHandle, BaseAddress, ZeroBits,
	  CommitSize, SectionOffset, *ViewSize, InheritDisposition,
	  AllocationType, Protect);
   
   DPRINT("  *Base:%08lx\n", *BaseAddress);
   
   Status = ObReferenceObjectByHandle(SectionHandle,
				      SECTION_MAP_READ,
				      MmSectionObjectType,
				      UserMode,
				      (PVOID*)&Section,
				      NULL);
   if (!(NT_SUCCESS(Status)))
     {
	DPRINT("ObReference failed rc=%x\n",Status);	   
	return Status;
     }
   
   DPRINT("Section %x\n",Section);
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObReferenceObjectByHandle(ProcessHandle, ...) failed (%x)\n",
	       Status);
	ObDereferenceObject(Section);		
	return Status;
     }
   
   AddressSpace = &Process->Pcb.AddressSpace;
   
   DPRINT("Process %x\n", Process);
   DPRINT("ViewSize %x\n",ViewSize);
   
   if (SectionOffset == NULL)
     {
	ViewOffset = 0;
     }
   else
     {
	ViewOffset = SectionOffset->u.LowPart;
     }
   
   if (((*ViewSize)+ViewOffset) > Section->MaximumSize.u.LowPart)
	{
	   (*ViewSize) = Section->MaximumSize.u.LowPart - ViewOffset;
	}
   
   DPRINT("Creating memory area\n");
   MmLockAddressSpace(AddressSpace);
   Status = MmCreateMemoryArea(Process,
			       &Process->Pcb.AddressSpace,
			       MEMORY_AREA_SECTION_VIEW_COMMIT,
			       BaseAddress,
			       *ViewSize,
			       Protect,
			       &Result);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtMapViewOfSection() = %x\n",Status);
	
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	ObDereferenceObject(Section);
	
	return Status;
     }
   
   KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
   InsertTailList(&Section->ViewListHead, 
		  &Result->Data.SectionData.ViewListEntry);
   KeReleaseSpinLock(&Section->ViewListLock, oldIrql);
   
   Result->Data.SectionData.Section = Section;
   Result->Data.SectionData.ViewOffset = ViewOffset;
   
   DPRINT("SectionOffset %x\n",SectionOffset);
   
   
   DPRINT("*BaseAddress %x\n",*BaseAddress);
   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);   
   
   DPRINT("NtMapViewOfSection() returning (Status %x)\n", STATUS_SUCCESS);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL MmUnmapViewOfSection(PEPROCESS Process,
			      PMEMORY_AREA MemoryArea)
{
   PSECTION_OBJECT Section;
   KIRQL oldIrql;
   
   Section = MemoryArea->Data.SectionData.Section;
   
   DPRINT("MmUnmapViewOfSection(Section %x) SectionRC %d\n",
	   Section, ObGetReferenceCount(Section));
   
   KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
   RemoveEntryList(&MemoryArea->Data.SectionData.ViewListEntry);
   KeReleaseSpinLock(&Section->ViewListLock, oldIrql);
   ObDereferenceObject(Section);
   
   return(STATUS_SUCCESS);
}

/**********************************************************************
 * NAME							EXPORTED
 *	NtUnmapViewOfSection
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 *	ProcessHandle
 *
 *	BaseAddress
 *
 * RETURN VALUE
 *	Status.
 *
 * REVISIONS
 *
 */
NTSTATUS STDCALL NtUnmapViewOfSection (HANDLE	ProcessHandle,
				       PVOID	BaseAddress)
{
   PEPROCESS	Process;
   NTSTATUS	Status;
   PMEMORY_AREA MemoryArea;
   PMADDRESS_SPACE AddressSpace;
   
   DPRINT("NtUnmapViewOfSection(ProcessHandle %x, BaseAddress %x)\n",
	   ProcessHandle, BaseAddress);
   
   DPRINT("Referencing process\n");
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("ObReferenceObjectByHandle failed (Status %x)\n", Status);
	return(Status);
     }
   
   AddressSpace = &Process->Pcb.AddressSpace;
   
   DPRINT("Opening memory area Process %x BaseAddress %x\n",
	   Process, BaseAddress);
   MmLockAddressSpace(AddressSpace);
   MemoryArea = MmOpenMemoryAreaByAddress(AddressSpace,
					  BaseAddress);
   if (MemoryArea == NULL)
     {
	MmUnlockAddressSpace(AddressSpace);
	ObDereferenceObject(Process);
	return(STATUS_UNSUCCESSFUL);
     }
   
   Status = MmUnmapViewOfSection(Process,
				 MemoryArea);
   
   DPRINT("MmFreeMemoryArea()\n");
   Status = MmFreeMemoryArea(&Process->Pcb.AddressSpace,
			     BaseAddress,
			     0,
			     TRUE);
   MmUnlockAddressSpace(AddressSpace);
   ObDereferenceObject(Process);

   return Status;
}


NTSTATUS STDCALL NtQuerySection (IN	HANDLE	SectionHandle,
				 IN	CINT	SectionInformationClass,
				 OUT	PVOID	SectionInformation,
				 IN	ULONG	Length, 
				 OUT	PULONG	ResultLength)
/*
 * FUNCTION: Queries the information of a section object.
 * ARGUMENTS: 
 *        SectionHandle = Handle to the section link object
 *	  SectionInformationClass = Index to a certain information structure
 *        SectionInformation (OUT)= Caller supplies storage for resulting 
 *                                  information
 *        Length =  Size of the supplied storage 
 *        ResultLength = Data written
 * RETURNS: Status
 *
 */
{
   return(STATUS_UNSUCCESSFUL);
}


NTSTATUS STDCALL NtExtendSection(IN	HANDLE	SectionHandle,
				 IN	ULONG	NewMaximumSize)
{
   UNIMPLEMENTED;
}


/**********************************************************************
 * NAME							INTERNAL
 * 	MmAllocateSection@4
 *
 * DESCRIPTION
 *
 * ARGUMENTS
 * 	Length
 *
 * RETURN VALUE
 *
 * NOTE
 * 	Code taken from ntoskrnl/mm/special.c.
 *
 * REVISIONS
 *
 */
PVOID STDCALL MmAllocateSection (IN ULONG Length)
{
   PVOID Result;
   MEMORY_AREA* marea;
   NTSTATUS Status;
   ULONG i;
   PMADDRESS_SPACE AddressSpace;
   
   DPRINT("MmAllocateSection(Length %x)\n",Length);
   
   AddressSpace = MmGetKernelAddressSpace();
   Result = NULL;
   MmLockAddressSpace(AddressSpace);
   Status = MmCreateMemoryArea (NULL,
				AddressSpace,
				MEMORY_AREA_SYSTEM,
				&Result,
				Length,
				0,
				&marea);
   if (STATUS_SUCCESS != Status)
     {
	return (NULL);
	}
   DPRINT("Result %p\n",Result);
   for (i = 0; (i <= (Length / PAGESIZE)); i++)
     {
	MmSetPage (NULL,
		   (Result + (i * PAGESIZE)),
		   PAGE_READWRITE,
		   (ULONG) MmAllocPage ());
     }
   MmUnlockAddressSpace(AddressSpace);
   return ((PVOID)Result);
}


/**********************************************************************
 * NAME							EXPORTED
 *	MmMapViewOfSection@40
 *
 * DESCRIPTION
 *	
 * ARGUMENTS
 * 	FIXME: stack space allocated is 40 bytes, but nothing
 * 	is known about what they are filled with.
 *
 * RETURN VALUE
 * 	Status.
 *
 */
PVOID
STDCALL
MmMapViewOfSection (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5,
	DWORD	Unknown6,
	DWORD	Unknown7,
	DWORD	Unknown8,
	DWORD	Unknown9
	)
{
	UNIMPLEMENTED;
	return (NULL);
}


BOOLEAN
STDCALL
MmCanFileBeTruncated (
	IN	PSECTION_OBJECT_POINTERS	SectionObjectPointer,
	IN	PLARGE_INTEGER			NewFileSize
	)
{
	UNIMPLEMENTED;
	return (FALSE);
}


BOOLEAN
STDCALL
MmDisableModifiedWriteOfSection (
	DWORD	Unknown0
	)
{
	UNIMPLEMENTED;
	return (FALSE);
}

BOOLEAN
STDCALL
MmFlushImageSection (
	IN	PSECTION_OBJECT_POINTERS	SectionObjectPointer,
	IN	MMFLUSH_TYPE			FlushType
	)
{
	UNIMPLEMENTED;
	return (FALSE);
}

BOOLEAN
STDCALL
MmForceSectionClosed (
	DWORD	Unknown0,
	DWORD	Unknown1
	)
{
	UNIMPLEMENTED;
	return (FALSE);
}


NTSTATUS
STDCALL
MmMapViewInSystemSpace (
	IN	PVOID	Section,
	OUT	PVOID	* MappedBase,
	IN	PULONG	ViewSize
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}

NTSTATUS
STDCALL
MmUnmapViewInSystemSpace (
	DWORD	Unknown0
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


NTSTATUS
STDCALL
MmSetBankedSection (
	DWORD	Unknown0,
	DWORD	Unknown1,
	DWORD	Unknown2,
	DWORD	Unknown3,
	DWORD	Unknown4,
	DWORD	Unknown5
	)
{
	UNIMPLEMENTED;
	return (STATUS_NOT_IMPLEMENTED);
}


/**********************************************************************
 * NAME							EXPORTED
 * 	MmCreateSection@
 * 	
 * DESCRIPTION
 * 	Creates a section object.
 * 	
 * ARGUMENTS
 *	SectionObjiect (OUT)
 *		Caller supplied storage for the resulting pointer
 *		to a SECTION_BOJECT instance;
 *		
 *	DesiredAccess
 *		Specifies the desired access to the section can be a
 *		combination of:
 *			STANDARD_RIGHTS_REQUIRED	|
 *			SECTION_QUERY			|
 *			SECTION_MAP_WRITE		|
 *			SECTION_MAP_READ		|
 *			SECTION_MAP_EXECUTE
 *			
 *	ObjectAttributes [OPTIONAL]
 *		Initialized attributes for the object can be used 
 *		to create a named section;
 *
 *	MaximumSize
 *		Maximizes the size of the memory section. Must be 
 *		non-NULL for a page-file backed section. 
 *		If value specified for a mapped file and the file is 
 *		not large enough, file will be extended.
 *		
 *	SectionPageProtection
 *		Can be a combination of:
 *			PAGE_READONLY	| 
 *			PAGE_READWRITE	|
 *			PAGE_WRITEONLY	| 
 *			PAGE_WRITECOPY
 *			
 *	AllocationAttributes
 *		Can be a combination of:
 *			SEC_IMAGE	| 
 *			SEC_RESERVE
 *			
 *	FileHandle
 *		Handle to a file to create a section mapped to a file
 *		instead of a memory backed section;
 *
 *	File
 *		Unknown.
 *	
 * RETURN VALUE
 * 	Status.
 */
NTSTATUS
STDCALL
MmCreateSection (
	OUT	PSECTION_OBJECT		* SectionObject,
	IN	ACCESS_MASK		DesiredAccess,
	IN	POBJECT_ATTRIBUTES	ObjectAttributes	OPTIONAL,
	IN	PLARGE_INTEGER		MaximumSize,
	IN	ULONG			SectionPageProtection,
	IN	ULONG			AllocationAttributes,
	IN	HANDLE			FileHandle		OPTIONAL,
	IN	PFILE_OBJECT		File			OPTIONAL
	)
{
	return (STATUS_NOT_IMPLEMENTED);
}

/* EOF */
