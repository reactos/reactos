/* $Id: section.c,v 1.18 1999/11/25 23:37:02 ekohl Exp $
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

POBJECT_TYPE MmSectionType = NULL;

/* FUNCTIONS *****************************************************************/

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
	     KeReleaseSpinLock(&Section->ViewListLock, oldIrql);
	     return((PVOID)PhysPage);
	  }
	
	current_entry = current_entry->Flink;
     }
   
   KeReleaseSpinLock(&Section->ViewListLock, oldIrql);
   return(NULL);
}

VOID MmpDeleteSection(PVOID ObjectBody)
{
}

NTSTATUS MmpCreateSection(PVOID ObjectBody,
			  PVOID Parent,
			  PWSTR RemainingPath,
			  POBJECT_ATTRIBUTES ObjectAttributes)
{
   NTSTATUS Status;
   
   DPRINT("MmpCreateDevice(ObjectBody %x, Parent %x, RemainingPath %w)\n",
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
   ANSI_STRING AnsiString;
   
   MmSectionType = ExAllocatePool(NonPagedPool,sizeof(OBJECT_TYPE));
   
   MmSectionType->TotalObjects = 0;
   MmSectionType->TotalHandles = 0;
   MmSectionType->MaxObjects = ULONG_MAX;
   MmSectionType->MaxHandles = ULONG_MAX;
   MmSectionType->PagedPoolCharge = 0;
   MmSectionType->NonpagedPoolCharge = sizeof(SECTION_OBJECT);
   MmSectionType->Dump = NULL;
   MmSectionType->Open = NULL;
   MmSectionType->Close = NULL;
   MmSectionType->Delete = MmpDeleteSection;
   MmSectionType->Parse = NULL;
   MmSectionType->Security = NULL;
   MmSectionType->QueryName = NULL;
   MmSectionType->OkayToClose = NULL;
   MmSectionType->Create = MmpCreateSection;
   
   RtlInitAnsiString(&AnsiString,"Section");
   RtlAnsiStringToUnicodeString(&MmSectionType->TypeName,
				&AnsiString,TRUE);
   return(STATUS_SUCCESS);
}


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
			    MmSectionType);
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
   
   if (FileHandle != NULL)
     {
	Status = ObReferenceObjectByHandle(FileHandle,
					   FILE_READ_DATA,
					   IoFileType,
					   UserMode,
					   (PVOID*)&Section->FileObject,
					   NULL);
	if (Status != STATUS_SUCCESS)
	  {
	     // Delete section object
	     DPRINT("NtCreateSection() = %x\n",Status);
	     return(Status);
	  }
     }
   else
     {
	Section->FileObject = NULL;
     }
   
   DPRINT("NtCreateSection() = STATUS_SUCCESS\n");
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
				    MmSectionType,
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
				      MmSectionType,
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
   Status = MmCreateMemoryArea(UserMode,
			       Process,
			       MEMORY_AREA_SECTION_VIEW_COMMIT,
			       BaseAddress,
			       *ViewSize,
			       Protect,
			       &Result);
   if (!NT_SUCCESS(Status))
     {
	DPRINT("NtMapViewOfSection() = %x\n",Status);
	
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
   ObDereferenceObject(Process);   
   ObDereferenceObject(Section);
   
   return(STATUS_SUCCESS);
}

NTSTATUS MmUnmapViewOfSection(PEPROCESS Process,
			      PMEMORY_AREA MemoryArea)
{
   PSECTION_OBJECT Section;
   KIRQL oldIrql;
   
   Section = MemoryArea->Data.SectionData.Section;
   KeAcquireSpinLock(&Section->ViewListLock, &oldIrql);
   RemoveEntryList(&MemoryArea->Data.SectionData.ViewListEntry);
   KeReleaseSpinLock(&Section->ViewListLock, oldIrql);
   
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
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   
   MemoryArea = MmOpenMemoryAreaByAddress(Process, BaseAddress);
   if (MemoryArea == NULL)
     {
	ObDereferenceObject(Process);
	return(STATUS_UNSUCCESSFUL);
     }
   
   Status = MmUnmapViewOfSection(Process, MemoryArea);
   
   Status = MmFreeMemoryArea(Process,
			     BaseAddress,
			     0,
			     TRUE);
   
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


/* EOF */
