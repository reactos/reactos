/* $Id: rtl.c,v 1.16 2003/07/10 20:34:50 royce Exp $
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
#include <internal/ldr.h>

#define NDEBUG
#include <internal/debug.h>

/* FUNCTIONS ****************************************************************/


/*
 * @implemented
 */
PIMAGE_NT_HEADERS STDCALL 
RtlImageNtHeader (IN PVOID BaseAddress)
{
   PIMAGE_DOS_HEADER DosHeader;
   PIMAGE_NT_HEADERS NTHeaders;
   
   DosHeader = (PIMAGE_DOS_HEADER)BaseAddress;
   NTHeaders = (PIMAGE_NT_HEADERS)(BaseAddress + DosHeader->e_lfanew);
   if ((DosHeader->e_magic != IMAGE_DOS_MAGIC)
       || (DosHeader->e_lfanew == 0L)
       || (*(PULONG) NTHeaders != IMAGE_PE_MAGIC))
     {
	return(NULL);
     }
   return(NTHeaders);
}


/*
 * @implemented
 */
PVOID STDCALL
RtlImageDirectoryEntryToData (IN PVOID	BaseAddress,
			      IN BOOLEAN	ImageLoaded,
			      IN ULONG	Directory,
			      OUT PULONG	Size)
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


/*
 * @implemented
 */
PIMAGE_SECTION_HEADER
STDCALL
RtlImageRvaToSection (
	PIMAGE_NT_HEADERS	NtHeader,
	PVOID			BaseAddress,
	ULONG			Rva
	)
{
	PIMAGE_SECTION_HEADER Section;
	ULONG Va;
	ULONG Count;

	Count = NtHeader->FileHeader.NumberOfSections;
	Section = (PIMAGE_SECTION_HEADER)((ULONG)&NtHeader->OptionalHeader +
	                                  NtHeader->FileHeader.SizeOfOptionalHeader);
	while (Count)
	{
		Va = Section->VirtualAddress;
		if ((Va <= Rva) &&
		    (Rva < Va + Section->SizeOfRawData))
			return Section;
		Section++;
	}
	return NULL;
}


/*
 * @implemented
 */
ULONG
STDCALL
RtlImageRvaToVa (
	PIMAGE_NT_HEADERS	NtHeader,
	PVOID			BaseAddress,
	ULONG			Rva,
	PIMAGE_SECTION_HEADER	*SectionHeader
	)
{
	PIMAGE_SECTION_HEADER Section = NULL;

	if (SectionHeader)
		Section = *SectionHeader;

	if (Section == NULL ||
	    Rva < Section->VirtualAddress ||
	    Rva >= Section->VirtualAddress + Section->SizeOfRawData)
	{
		Section = RtlImageRvaToSection (NtHeader, BaseAddress, Rva);
		if (Section == NULL)
			return 0;

		if (SectionHeader)
			*SectionHeader = Section;
	}

	return (ULONG)(BaseAddress +
	               Rva +
	               Section->PointerToRawData -
	               Section->VirtualAddress);
}

#define RVA(m, b) ((ULONG)b + m)

/*
 * @implemented
 */
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
	RtlImageDirectoryEntryToData (BaseAddress, TRUE, 
				      IMAGE_DIRECTORY_ENTRY_EXPORT, &i);

   if (!ExportDir || !i || !ProcedureAddress)
     {
	return(STATUS_INVALID_PARAMETER);
     }
   
   AddressPtr = (PULONG)RVA(BaseAddress, ExportDir->AddressOfFunctions);
   if (Name && Name->Length)
     {
       ULONG minn, maxn;

	/* by name */
       OrdinalPtr = 
	 (PUSHORT)RVA(BaseAddress, ExportDir->AddressOfNameOrdinals);
       NamePtr = (PULONG)RVA(BaseAddress, ExportDir->AddressOfNames);

	minn = 0; maxn = ExportDir->NumberOfNames;
	while (minn <= maxn)
	  {
	    ULONG mid;
	    LONG res;

	    mid = (minn + maxn) / 2;
	    res = _strnicmp(Name->Buffer, (PCH)RVA(BaseAddress, NamePtr[mid]),
			    Name->Length);
	    if (res == 0)
	      {
		*ProcedureAddress = 
		  (PVOID)RVA(BaseAddress, AddressPtr[OrdinalPtr[mid]]);
		return(STATUS_SUCCESS);
	      }
	    else if (res > 0)
	      {
		maxn = mid - 1;
	      }
	    else
	      {
		minn = mid + 1;
	      }
	  }

	for (i = 0; i < ExportDir->NumberOfNames; i++, NamePtr++, OrdinalPtr++)
	  {
	     if (!_strnicmp(Name->Buffer, 
			    (char*)(BaseAddress + *NamePtr), Name->Length))
	       {
		  *ProcedureAddress = 
		    (PVOID)((ULONG)BaseAddress + 
			    (ULONG)AddressPtr[*OrdinalPtr]);
		  return STATUS_SUCCESS;
	       }
	  }
	CPRINT("LdrGetProcedureAddress: Can't resolve symbol '%Z'\n", Name);
     }
   else
     {
	/* by ordinal */
	Ordinal &= 0x0000FFFF;
	if (Ordinal - ExportDir->Base < ExportDir->NumberOfFunctions)
	  {
	     *ProcedureAddress = 
	       (PVOID)((ULONG)BaseAddress + 
		       (ULONG)AddressPtr[Ordinal - ExportDir->Base]);
	     return STATUS_SUCCESS;
	  }
	CPRINT("LdrGetProcedureAddress: Can't resolve symbol @%d\n",
		 Ordinal);
  }

   return STATUS_PROCEDURE_NOT_FOUND;
}

/* EOF */
