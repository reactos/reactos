/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/sysdll.c
 * PURPOSE:         Loaders for PE executables
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 * UPDATE HISTORY:
 *   DW   26/01/00  Created
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/i386/segment.h>
#include <internal/linkage.h>
#include <internal/module.h>
#include <internal/ntoskrnl.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <string.h>
#include <internal/string.h>
#include <internal/teb.h>
#include <internal/ldr.h>

#define NDEBUG
#include <internal/debug.h>

#include "syspath.h"

/* FUNCTIONS *****************************************************************/

NTSTATUS LdrpMapImage(HANDLE ProcessHandle,
		      HANDLE SectionHandle,
		      PVOID* ReturnedImageBase)
/*
 * FUNCTION: LdrpMapImage maps a user-mode image into an address space
 * PARAMETERS:
 *   ProcessHandle
 *              Points to the process to map the image into
 * 
 *   SectionHandle
 *              Points to the section to map
 * 
 * RETURNS: Status
 */
{   
   PVOID			ImageBase;
   NTSTATUS		Status;
   PIMAGE_NT_HEADERS	NTHeaders;
   ULONG			InitialViewSize;
   ULONG			i;
   PEPROCESS Process;
   PVOID FinalBase;
   ULONG NumberOfSections;
   
   DPRINT("Referencing process\n");
   Status = ObReferenceObjectByHandle(ProcessHandle,
				      PROCESS_ALL_ACCESS,
				      PsProcessType,
				      KernelMode,
				      (PVOID*)&Process,
				      NULL);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("ObReferenceObjectByProcess() failed (Status %x)\n", Status);
	return(Status);
     }
   
   /*
    * map the dos header into the process
    */
   DPRINT("Mapping view of section\n");
   InitialViewSize = sizeof(PIMAGE_DOS_HEADER);
   ImageBase = NULL;
   
   Status = ZwMapViewOfSection(SectionHandle,
			       ProcessHandle,
			       (PVOID*)&ImageBase,
			       0,
			       InitialViewSize,
			       NULL,
			       &InitialViewSize,
			       0,
			       MEM_COMMIT,
			       PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Image map view of section failed (Status %x)", Status);
	return(Status);
     }
   
   /*
    * Map the pe headers into the process
    */
   DPRINT("Attaching to process\n");
   KeAttachProcess(Process);
   InitialViewSize = ((PIMAGE_DOS_HEADER)ImageBase)->e_lfanew +
     sizeof(IMAGE_NT_HEADERS);
   DPRINT("InitialViewSize %d\n", InitialViewSize);
   KeDetachProcess();
   
   DPRINT("Unmapping view of section\n");
   Status = ZwUnmapViewOfSection(ProcessHandle,
				 ImageBase);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("ZwUnmapViewOfSection failed (Status %x)\n", Status);
	return(Status);
     }
   
   DPRINT("Mapping view of section\n");
   Status = ZwMapViewOfSection(SectionHandle,
			       ProcessHandle,
			       (PVOID*)&ImageBase,
			       0,
			       InitialViewSize,
			       NULL,
			       &InitialViewSize,
			       0,
			       MEM_COMMIT,
			       PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Image map view of section failed (Status %x)", Status);
	return(Status);
     }
   
   
   /*
    * TBD
    */
   DPRINT("Attaching to process\n");
   KeAttachProcess(Process);
   NTHeaders = RtlImageNtHeader(ImageBase);
   InitialViewSize = ((PIMAGE_DOS_HEADER)ImageBase)->e_lfanew +
     sizeof(IMAGE_NT_HEADERS) +
     (sizeof (IMAGE_SECTION_HEADER) * NTHeaders->FileHeader.NumberOfSections);
   DPRINT("InitialViewSize %x\n", InitialViewSize);
   FinalBase = (PVOID)NTHeaders->OptionalHeader.ImageBase;
   NumberOfSections = NTHeaders->FileHeader.NumberOfSections;
   KeDetachProcess();
   
   DPRINT("Unmapping view of section\n");
   Status = ZwUnmapViewOfSection(ProcessHandle,
				 ImageBase);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("ZwUnmapViewOfSection failed (Status %x)\n", Status);
	return(Status);
     }
   
   ImageBase = FinalBase;
   
   DPRINT("Mapping view of section\n");
   Status = ZwMapViewOfSection(SectionHandle,
			       ProcessHandle,
			       (PVOID*)&ImageBase,
			       0,
			       InitialViewSize,
			       NULL,
			       &InitialViewSize,
			       0,
			       MEM_COMMIT,
			       PAGE_READWRITE);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("Image map view of section failed (Status %x)", Status);
	return(Status);
     }
   
   
   DPRINT("Mapping view of all sections\n");
   for (i = 0; i < NumberOfSections; i++)
     {
	PIMAGE_SECTION_HEADER	Sections;
	LARGE_INTEGER		Offset;
	ULONG			Base;
	ULONG Size;
	
	KeAttachProcess(Process);
	Sections = (PIMAGE_SECTION_HEADER) SECHDROFFSET(ImageBase);      
	DPRINT("Sections %x\n", Sections);
	Base = (ULONG)(Sections[i].VirtualAddress + ImageBase);
	Offset.u.LowPart = Sections[i].PointerToRawData;
	Offset.u.HighPart = 0;
	Size = Sections[i].Misc.VirtualSize;
	KeDetachProcess();
	
	Status = ZwMapViewOfSection(SectionHandle,
				    ProcessHandle,
				    (PVOID *)&Base,
				    0,
				    Size,
				    &Offset,
				    (PULONG)&Size,
				    0,
				    MEM_COMMIT,
				    PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("Image map view of secion failed (Status %x)\n", Status);
	     return(Status);
	  }
     }
   
   DPRINT("Returning\n");
   *ReturnedImageBase = ImageBase;
   
   return(STATUS_SUCCESS);
}
