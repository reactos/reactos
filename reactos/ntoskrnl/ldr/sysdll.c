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
#include <internal/symbol.h>
#include <internal/teb.h>

#define NDEBUG
#include <internal/debug.h>

#include "syspath.h"

/* GLOBALS *******************************************************************/

static PVOID SystemDllEntryPoint = NULL;

/* FUNCTIONS *****************************************************************/

PVOID LdrpGetSystemDllEntryPoint(VOID)
{
   return(SystemDllEntryPoint);
}

NTSTATUS LdrpMapSystemDll(HANDLE ProcessHandle,
			  PVOID* LdrStartupAddr)
/*
 * FUNCTION: LdrpMapSystemDll maps the system dll into the specified process
 * address space and returns its startup address.
 * PARAMETERS:
 *   ProcessHandle
 *              Points to the process to map the system dll into
 * 
 *   LdrStartupAddress
 *              Receives the startup address of the system dll on function
 *              completion
 * 
 * RETURNS: Status
 */
{   
   CHAR			BlockBuffer [1024];
   DWORD			ImageBase;
   ULONG			ImageSize;
   NTSTATUS		Status;
   OBJECT_ATTRIBUTES	FileObjectAttributes;
   HANDLE			FileHandle;
   HANDLE			NTDllSectionHandle;
   UNICODE_STRING		DllPathname;
   PIMAGE_DOS_HEADER	DosHeader;
   PIMAGE_NT_HEADERS	NTHeaders;
   ULONG			InitialViewSize;
   ULONG			i;
   WCHAR			TmpNameBuffer [MAX_PATH];

   /*
    * Locate and open NTDLL to determine ImageBase
    * and LdrStartup
    */
   LdrGetSystemDirectory(TmpNameBuffer, sizeof TmpNameBuffer);
   wcscat(TmpNameBuffer, L"\\ntdll.dll");
   RtlInitUnicodeString(&DllPathname, TmpNameBuffer);
   InitializeObjectAttributes(&FileObjectAttributes,
			      &DllPathname,
			      0,
			      NULL,
			      NULL);
   DPRINT("Opening NTDLL\n");
   Status = ZwOpenFile(&FileHandle,
		       FILE_ALL_ACCESS,
		       &FileObjectAttributes,
		       NULL,
		       0,
		       0);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("NTDLL open failed (Status %x)\n", Status);
	return Status;
     }
   Status = ZwReadFile(FileHandle,
		       0,
		       0,
		       0,
		       0,
		       BlockBuffer,
		       sizeof(BlockBuffer),
		       0,
		       0);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("NTDLL header read failed (Status %x)\n", Status);
	ZwClose(FileHandle);
	return Status;
     }

   /*
    * FIXME: this will fail if the NT headers are
    * more than 1024 bytes from start.
    */
   DosHeader = (PIMAGE_DOS_HEADER) BlockBuffer;
   NTHeaders = (PIMAGE_NT_HEADERS) (BlockBuffer + DosHeader->e_lfanew);
   if ((DosHeader->e_magic != IMAGE_DOS_MAGIC)
       || (DosHeader->e_lfanew == 0L)
       || (*(PULONG) NTHeaders != IMAGE_PE_MAGIC))
     {
	DbgPrint("NTDLL format invalid\n");
	ZwClose(FileHandle);	
	return(STATUS_UNSUCCESSFUL);
     }
   ImageBase = NTHeaders->OptionalHeader.ImageBase;   
   ImageSize = NTHeaders->OptionalHeader.SizeOfImage;
   
   /*
    * FIXME: retrieve the offset of LdrStartup from NTDLL
    */
   DPRINT("ImageBase %x\n",ImageBase);
   *LdrStartupAddr = 
     (PVOID)ImageBase + NTHeaders->OptionalHeader.AddressOfEntryPoint;
   DPRINT("LdrStartupAddr %x\n", LdrStartupAddr);
   SystemDllEntryPoint = *LdrStartupAddr;
   
   /*
    * Create a section for NTDLL
    */
   DPRINT("Creating section\n");
   Status = ZwCreateSection(&NTDllSectionHandle,
			    SECTION_ALL_ACCESS,
			    NULL,
			    NULL,
			    PAGE_READWRITE,
			    MEM_COMMIT,
			    FileHandle);
   if (!NT_SUCCESS(Status))
     {
	DbgPrint("NTDLL create section failed (Status %x)\n", Status);
	ZwClose(FileHandle);	
	return(Status);
     }
   ZwClose(FileHandle);
   
   /*
    * Map the NTDLL into the process
    */
   InitialViewSize = DosHeader->e_lfanew + 
     sizeof (IMAGE_NT_HEADERS) + 
     (sizeof (IMAGE_SECTION_HEADER) * NTHeaders->FileHeader.NumberOfSections);
   DPRINT("Mapping view of section\n");
   Status = ZwMapViewOfSection(NTDllSectionHandle,
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
	DbgPrint("NTDLL map view of secion failed (Status %x)", Status);
	ZwClose(NTDllSectionHandle);
	return(Status);
     }

   for (i = 0; i < NTHeaders->FileHeader.NumberOfSections; i++)
     {
	PIMAGE_SECTION_HEADER	Sections;
	LARGE_INTEGER		Offset;
	ULONG			Base;
	
	DPRINT("Mapping view of section %d\n", i);
	Sections = (PIMAGE_SECTION_HEADER) SECHDROFFSET(BlockBuffer);
	DPRINT("Sections %x\n", Sections);
	Base = Sections[i].VirtualAddress + ImageBase;
	DPRINT("Base %x\n", Base);
	Offset.u.LowPart = Sections[i].PointerToRawData;
	Offset.u.HighPart = 0;
	DPRINT("Mapping view of section\n");
	Status = ZwMapViewOfSection(NTDllSectionHandle,
				    ProcessHandle,
				    (PVOID*)&Base,
				    0,
				    Sections[i].Misc.VirtualSize,
				    &Offset,
				    (PULONG)&Sections[i].Misc.VirtualSize,
				    0,
				    MEM_COMMIT,
				    PAGE_READWRITE);
	if (!NT_SUCCESS(Status))
	  {
	     DbgPrint("NTDLL map view of secion failed (Status %x)\n", Status);
	     ZwClose(NTDllSectionHandle);
	     return(Status);
	  }
     }
   DPRINT("Finished mapping\n");
   ZwClose(NTDllSectionHandle);
	
   return(STATUS_SUCCESS);
}
