/* $Id: image.c,v 1.1.14.1 2004/12/08 21:57:21 hyperion Exp $
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

/*
 * @implemented
 */
PIMAGE_NT_HEADERS STDCALL
RtlImageNtHeader (IN PVOID BaseAddress)
{
  PIMAGE_NT_HEADERS NtHeader;
  char * Magic = BaseAddress;

  /* HACK: PE executable */
  if (Magic[0] == 'M' && Magic[1] == 'Z')
    {
      PIMAGE_DOS_HEADER DosHeader = BaseAddress;
      DPRINT("DosHeader %x\n", DosHeader);

      if (DosHeader->e_lfanew == 0L ||
          *(PULONG)((PUCHAR)BaseAddress + DosHeader->e_lfanew) != IMAGE_PE_MAGIC)
        {
          DPRINT("Image has bad header\n");
          return NULL;
        }

      NtHeader = (PIMAGE_NT_HEADERS)((PUCHAR)BaseAddress + DosHeader->e_lfanew);
    }
  /* HACK: ReactOS ELF executable */
  else if (Magic[0] == 0x7f &&
           Magic[1] == 'E' &&
           Magic[2] == 'L' &&
           Magic[3] == 'F')
    {
      DbgPrint("TODO: ElfGetSymbolAddress()\n");
      /*NtHeader = ElfGetSymbolAddress(BaseAddress, "nt_header");*/
    }
  else
    {
      DPRINT("Unknown image format\n");
      NtHeader = NULL;
    }

  return NtHeader;
}


/*
 * @implemented
 */
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
	ULONG Va;

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
	return (PVOID)RtlImageRvaToVa (NtHeader, BaseAddress, Va, NULL);
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

/* EOF */
