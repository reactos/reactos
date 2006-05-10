/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/image.c
 * PURPOSE:         Image handling functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
PIMAGE_NT_HEADERS NTAPI
RtlImageNtHeader (IN PVOID BaseAddress)
{
  PIMAGE_NT_HEADERS NtHeader;
  PIMAGE_DOS_HEADER DosHeader = (PIMAGE_DOS_HEADER)BaseAddress;

  if (DosHeader && DosHeader->e_magic != IMAGE_DOS_SIGNATURE)
    {
      DPRINT1("DosHeader->e_magic %x\n", DosHeader->e_magic);
      DPRINT1("NtHeader 0x%lx\n", ((ULONG_PTR)BaseAddress + DosHeader->e_lfanew));
    }

  if (DosHeader && DosHeader->e_magic == IMAGE_DOS_SIGNATURE)
    {
      NtHeader = (PIMAGE_NT_HEADERS)((ULONG_PTR)BaseAddress + DosHeader->e_lfanew);
      if (NtHeader->Signature == IMAGE_NT_SIGNATURE)
	return NtHeader;
    }

  return NULL;
}


/*
 * @implemented
 */
PVOID
NTAPI
RtlImageDirectoryEntryToData(PVOID BaseAddress,
                             BOOLEAN MappedAsImage,
                             USHORT Directory,
                             PULONG Size)
{
	PIMAGE_NT_HEADERS NtHeader;
	ULONG Va;

	/* Magic flag for non-mapped images. */
	if ((ULONG_PTR)BaseAddress & 1)
	{
		BaseAddress = (PVOID)((ULONG_PTR)BaseAddress & ~1);
		MappedAsImage = FALSE;
        }


	NtHeader = RtlImageNtHeader (BaseAddress);
	if (NtHeader == NULL)
		return NULL;

	if (Directory >= NtHeader->OptionalHeader.NumberOfRvaAndSizes)
		return NULL;

	Va = NtHeader->OptionalHeader.DataDirectory[Directory].VirtualAddress;
	if (Va == 0)
		return NULL;

	*Size = NtHeader->OptionalHeader.DataDirectory[Directory].Size;

	if (MappedAsImage || Va < NtHeader->OptionalHeader.SizeOfHeaders)
		return (PVOID)((ULONG_PTR)BaseAddress + Va);

	/* image mapped as ordinary file, we must find raw pointer */
	return RtlImageRvaToVa (NtHeader, BaseAddress, Va, NULL);
}


/*
 * @implemented
 */
PIMAGE_SECTION_HEADER
NTAPI
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
PVOID
NTAPI
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

	return (PVOID)((ULONG_PTR)BaseAddress +
	               Rva +
	               Section->PointerToRawData -
	               (ULONG_PTR)Section->VirtualAddress);
}

/* EOF */
