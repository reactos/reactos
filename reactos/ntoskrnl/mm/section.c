/*
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

#define NDEBUG
#include <internal/debug.h>

/* GLOBALS *******************************************************************/

POBJECT_TYPE MmSectionType = NULL;

/* FUNCTIONS *****************************************************************/

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
   MmSectionType->Delete = NULL;
   MmSectionType->Parse = NULL;
   MmSectionType->Security = NULL;
   MmSectionType->QueryName = NULL;
   MmSectionType->OkayToClose = NULL;
   
   RtlInitAnsiString(&AnsiString,"Section");
   RtlAnsiStringToUnicodeString(&MmSectionType->TypeName,
				&AnsiString,TRUE);
   return(STATUS_SUCCESS);
}

NTSTATUS STDCALL NtCreateSection(OUT PHANDLE SectionHandle, 
				 IN ACCESS_MASK DesiredAccess,
			    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,  
	                    IN PLARGE_INTEGER MaximumSize OPTIONAL,  
	                    IN ULONG SectionPageProtection OPTIONAL,
	                    IN ULONG AllocationAttributes,
	                    IN HANDLE FileHandle OPTIONAL)
{
   return(ZwCreateSection(SectionHandle,
			  DesiredAccess,
			  ObjectAttributes,
			  MaximumSize,
			  SectionPageProtection,
			  AllocationAttributes,
			  FileHandle));
}

NTSTATUS STDCALL ZwCreateSection(OUT PHANDLE SectionHandle, 
				 IN ACCESS_MASK DesiredAccess,
			    IN POBJECT_ATTRIBUTES ObjectAttributes OPTIONAL,  
	                    IN PLARGE_INTEGER MaximumSize OPTIONAL,  
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
   
   DPRINT("ZwCreateSection()\n");
   
   Section = ObGenericCreateObject(SectionHandle,
				   DesiredAccess,
				   ObjectAttributes,
				   MmSectionType);
   
   if (MaximumSize != NULL)
     {
	Section->MaximumSize = *MaximumSize;
     }
   else
     {
        LARGE_INTEGER_QUAD_PART(Section->MaximumSize) = 0xffffffff;
     }
   Section->SectionPageProtection = SectionPageProtection;
   Status = ObReferenceObjectByHandle(FileHandle,
				      FILE_READ_DATA,
				      IoFileType,
				      UserMode,
				      (PVOID*)&Section->FileObject,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   Section->AllocateAttributes = AllocationAttributes;
   
   return(STATUS_SUCCESS);
}

NTSTATUS NtOpenSection(PHANDLE SectionHandle,
		       ACCESS_MASK DesiredAccess,
		       POBJECT_ATTRIBUTES ObjectAttributes)
{
   return(ZwOpenSection(SectionHandle,
			DesiredAccess,
			ObjectAttributes));
}

NTSTATUS ZwOpenSection(PHANDLE SectionHandle,
		       ACCESS_MASK DesiredAccess,
		       POBJECT_ATTRIBUTES ObjectAttributes)
{
   PVOID Object;
   NTSTATUS Status;
   PWSTR Ignored;
   
   *SectionHandle = 0;
   
   Status = ObOpenObjectByName(ObjectAttributes,&Object,&Ignored);
   if (!NT_SUCCESS(Status))
     {
	return(Status);
     }
       
   if (BODY_TO_HEADER(Object)->ObjectType!=MmSectionType)
     {	
	return(STATUS_UNSUCCESSFUL);
     }
   
   *SectionHandle = ObInsertHandle(KeGetCurrentProcess(),Object,
				   DesiredAccess,FALSE);
   return(STATUS_SUCCESS);
}

NTSTATUS NtMapViewOfSection(HANDLE SectionHandle,
			    HANDLE ProcessHandle,
			    PVOID* BaseAddress,
			    ULONG ZeroBits,
			    ULONG CommitSize,
			    PLARGE_INTEGER SectionOffset,
			    PULONG ViewSize,
			    SECTION_INHERIT InheritDisposition,
			    ULONG AllocationType,
			    ULONG Protect)
{
   return(ZwMapViewOfSection(SectionHandle,
			     ProcessHandle,
			     BaseAddress,
			     ZeroBits,
			     CommitSize,
			     SectionOffset,
			     ViewSize,
			     InheritDisposition,
			     AllocationType,
			     Protect));
}

NTSTATUS ZwMapViewOfSection(HANDLE SectionHandle,
			    HANDLE ProcessHandle,
			    PVOID* BaseAddress,
			    ULONG ZeroBits,
			    ULONG CommitSize,
			    PLARGE_INTEGER SectionOffset,
			    PULONG ViewSize,
			    SECTION_INHERIT InheritDisposition,
			    ULONG AllocationType,
			    ULONG Protect)
/*
 * FUNCTION: Maps a view of a section into the virtual address space of a 
 *           process
 * ARGUMENTS:
 *        SectionHandle = Handle of the section
 *        ProcessHandle = Handle of the process
 *        BaseAddress = Desired base address (or NULL) on entry
 *                      Actual base address of the view on exit
 *        ZeroBits = Number of high order address bits that must be zero
 *        CommitSize = Size in bytes of the initially committed section of 
 *                     the view 
 *        SectionOffset = Offset in bytes from the beginning of the section
 *                        to the beginning of the view
 *        ViewSize = Desired length of map (or zero to map all) on entry
 *                   Actual length mapped on exit
 *        InheritDisposition = Specified how the view is to be shared with
 *                            child processes
 *        AllocateType = Type of allocation for the pages
 *        Protect = Protection for the committed region of the view
 * RETURNS: Status
 */
{
   PSECTION_OBJECT Section;
   PEPROCESS Process;
   MEMORY_AREA* Result;
   NTSTATUS Status;
   
   DPRINT("ZwMapViewOfSection(SectionHandle %x, ProcessHandle %x)\n",
	  SectionHandle,ProcessHandle);
   
   Status = ObReferenceObjectByHandle(SectionHandle,
				      SECTION_MAP_READ,
				      MmSectionType,
				      UserMode,
				      (PVOID*)&Section,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	DPRINT("%s() = %x\n",Status);
	return(Status);
     }
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   
   if ((*ViewSize) > GET_LARGE_INTEGER_LOW_PART(Section->MaximumSize))
     {
	(*ViewSize) = GET_LARGE_INTEGER_LOW_PART(Section->MaximumSize);
     }
   
   MmCreateMemoryArea(UserMode,
		      Process,
		      MEMORY_AREA_SECTION_VIEW_COMMIT,
		      BaseAddress,
		      *ViewSize,
		      Protect,
		      &Result);
   Result->Data.SectionData.Section = Section;
   Result->Data.SectionData.ViewOffset = GET_LARGE_INTEGER_LOW_PART(*SectionOffset);
   
   DPRINT("*BaseAddress %x\n",*BaseAddress);
   DPRINT("Result->Data.SectionData.Section->FileObject %x\n",
	    Result->Data.SectionData.Section->FileObject);
   
   return(STATUS_SUCCESS);
}

NTSTATUS NtUnmapViewOfSection(HANDLE ProcessHandle, PVOID BaseAddress)
{
   return(ZwUnmapViewOfSection(ProcessHandle,BaseAddress));
}

NTSTATUS ZwUnmapViewOfSection(HANDLE ProcessHandle, PVOID BaseAddress)
{
   PEPROCESS Process;
   NTSTATUS Status;
   
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_VM_OPERATION,
				      PsProcessType,
				      UserMode,
				      (PVOID*)&Process,
				      NULL);
   if (Status != STATUS_SUCCESS)
     {
	return(Status);
     }
   return(MmFreeMemoryArea(Process,BaseAddress,0,TRUE));
}

NTSTATUS STDCALL NtQuerySection(IN HANDLE SectionHandle,
				IN CINT SectionInformationClass,
				OUT PVOID SectionInformation,
				IN ULONG Length, 
				OUT PULONG ResultLength)
{
   return(ZwQuerySection(SectionHandle,
			 SectionInformationClass,
			 SectionInformation,
			 Length,
			 ResultLength));
}

NTSTATUS STDCALL ZwQuerySection(IN HANDLE SectionHandle,
				IN CINT SectionInformationClass,
				OUT PVOID SectionInformation,
				IN ULONG Length, 
				OUT PULONG ResultLength)
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

NTSTATUS STDCALL NtExtendSection(IN HANDLE SectionHandle,
				 IN ULONG NewMaximumSize)
{
   return(ZwExtendSection(SectionHandle,NewMaximumSize));
}

NTSTATUS STDCALL ZwExtendSection(IN HANDLE SectionHandle,
				 IN ULONG NewMaximumSize)
{
   UNIMPLEMENTED;
}
