/* $Id: image.c,v 1.5 2003/05/15 11:03:21 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/ntdll/rtl/image.c
 * PURPOSE:         Image handling functions
 * PROGRAMMER:      Eric Kohl
 * UPDATE HISTORY:
 *                  17/03/2000 Created
 */

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* FUNCTIONS ****************************************************************/

PIMAGE_NT_HEADERS STDCALL
RtlImageNtHeader (IN PVOID BaseAddress)
{
  PIMAGE_NT_HEADERS NtHeader;
  PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)BaseAddress;

  if (DosHeader && DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
      DPRINT1("DosHeader->e_magic %x\n", DosHeader->e_magic);
      DPRINT1("NtHeader %x\n", (BaseAddress + DosHeader->e_lfanew));
    }

  if (DosHeader && DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
    {
      NtHeader = (PIMAGE_NT_HEADERS)(BaseAddress + DosHeader->e_lfanew);
      if (NtHeader->Signature == IMAGE_NT_SIGNATURE)
	return NtHeader;
    }

  return NULL;
}


PVOID
STDCALL
RtlImageDirectoryEntryToData (
	PVOID	BaseAddress,
	BOOLEAN	bFlag,
	ULONG	Directory,
	PULONG	Size
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

	if (bFlag)
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

/* EOF */
