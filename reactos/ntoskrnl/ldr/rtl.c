/* $Id: rtl.c,v 1.8.2.1 2000/07/24 23:38:05 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            ntoskrnl/ldr/loader.c
 * PURPOSE:         Loader utilities
 * PROGRAMMERS:     Jean Michault
 *                  Rex Jolliff (rex@lvcablemodem.com)
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <internal/module.h>
#include <internal/ntoskrnl.h>
#include <internal/ob.h>
#include <internal/ps.h>
#include <string.h>
#include <internal/string.h>
#include <internal/ldr.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/


PIMAGE_NT_HEADERS STDCALL RtlImageNtHeader (IN PVOID BaseAddress)
{
   PIMAGE_DOS_HEADER DosHeader;
   PIMAGE_NT_HEADERS NTHeaders;
   
   DPRINT("BaseAddress %x\n", BaseAddress);
   DosHeader = (PIMAGE_DOS_HEADER)BaseAddress;
   DPRINT("DosHeader %x\n", DosHeader);
   NTHeaders = (PIMAGE_NT_HEADERS)(BaseAddress + DosHeader->e_lfanew);
   DPRINT("NTHeaders %x\n", NTHeaders);
   DPRINT("DosHeader->e_magic %x DosHeader->e_lfanew %x\n",
	  DosHeader->e_magic, DosHeader->e_lfanew);
   DPRINT("*NTHeaders %x\n", *(PULONG)NTHeaders);
   if ((DosHeader->e_magic != IMAGE_DOS_MAGIC)
       || (DosHeader->e_lfanew == 0L)
       || (*(PULONG) NTHeaders != IMAGE_PE_MAGIC))
     {
	return(NULL);
     }
   return(NTHeaders);
}


PVOID STDCALL
RtlImageDirectoryEntryToData (
	IN PVOID	BaseAddress,
	IN BOOLEAN	ImageLoaded,
	IN ULONG	Directory,
	OUT PULONG	Size
	)
{
	PIMAGE_NT_HEADERS NtHeader;
	PIMAGE_SECTION_HEADER SectionHeader;
	ULONG Va;
	ULONG Count;

	NtHeader = RtlImageNtHeader (BaseAddress);
	if (NtHeader == NULL)
		return NULL;

	if (Directory >= NtHeader->OptionalHeader.NumberOfRvaAndSizes)
		return NULL;

	Va = NtHeader->OptionalHeader.DataDirectory[Directory].VirtualAddress;
	if (Va == 0)
		return NULL;

	if (Size)
		*Size = NtHeader->OptionalHeader.DataDirectory[Directory].Size;

	if (ImageLoaded)
		return (PVOID)(BaseAddress + Va);

	/* image mapped as ordinary file, we must find raw pointer */
	SectionHeader = (PIMAGE_SECTION_HEADER)(NtHeader + 1);
	Count = NtHeader->FileHeader.NumberOfSections;
	while (Count--)
	{
		if (SectionHeader->VirtualAddress == Va)
			return (PVOID)(BaseAddress + SectionHeader->PointerToRawData);
		SectionHeader++;
	}

	return NULL;
}


NTSTATUS STDCALL
LdrGetProcedureAddress (IN PVOID BaseAddress,
                        IN PANSI_STRING Name,
                        IN ULONG Ordinal,
                        OUT PVOID *ProcedureAddress)
{
   PIMAGE_EXPORT_DIRECTORY ExportDir;
   PUSHORT OrdinalPtr;
   PULONG NamePtr;
   PULONG AddressPtr;
   ULONG i = 0;

   /* get the pointer to the export directory */
   ExportDir = (PIMAGE_EXPORT_DIRECTORY)
	RtlImageDirectoryEntryToData (BaseAddress, TRUE, IMAGE_DIRECTORY_ENTRY_EXPORT, &i);

   if (!ExportDir || !i || !ProcedureAddress)
     {
	return STATUS_INVALID_PARAMETER;
     }

   AddressPtr = (PULONG)((ULONG)BaseAddress + (ULONG)ExportDir->AddressOfFunctions);
   if (Name && Name->Length)
     {
	/* by name */
	OrdinalPtr = (PUSHORT)((ULONG)BaseAddress + (ULONG)ExportDir->AddressOfNameOrdinals);
	NamePtr = (PULONG)((ULONG)BaseAddress + (ULONG)ExportDir->AddressOfNames);
	for (i = 0; i < ExportDir->NumberOfNames; i++, NamePtr++, OrdinalPtr++)
	  {
	     if (!_strnicmp(Name->Buffer, (char*)(BaseAddress + *NamePtr), Name->Length))
	       {
		  *ProcedureAddress = (PVOID)((ULONG)BaseAddress + (ULONG)AddressPtr[*OrdinalPtr]);
		  return STATUS_SUCCESS;
	       }
	  }
	DbgPrint("LdrGetProcedureAddress: Can't resolve symbol '%Z'\n", Name);
     }
   else
     {
	/* by ordinal */
	Ordinal &= 0x0000FFFF;
	if (Ordinal - ExportDir->Base < ExportDir->NumberOfFunctions)
	  {
	     *ProcedureAddress = (PVOID)((ULONG)BaseAddress + (ULONG)AddressPtr[Ordinal - ExportDir->Base]);
	     return STATUS_SUCCESS;
	  }
	DbgPrint("LdrGetProcedureAddress: Can't resolve symbol @%d\n", Ordinal);
  }

   return STATUS_PROCEDURE_NOT_FOUND;
}

/* EOF */
