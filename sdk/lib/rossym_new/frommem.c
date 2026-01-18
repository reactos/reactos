/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/frommem.c
 * PURPOSE:         Creating rossym info from an in-memory image
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#include <ntifs.h>
#include <ndk/ntndk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"
#include <ntimage.h>

#define NDEBUG
#include <debug.h>

#include "dwarf.h"
#include "pe.h"

#define SYMBOL_SIZE 18

BOOLEAN
RosSymCreateFromMem(PVOID ImageStart, ULONG_PTR ImageSize, PROSSYM_INFO *RosSymInfo)
{
	ANSI_STRING PendingName = { };
	PIMAGE_DOS_HEADER DosHeader;
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_SECTION_HEADER OrigSectionHeaders;
	PeSect *SectionHeaders;
	ULONG SectionIndex;
	unsigned SymbolTable, NumSymbols;

	/* Check if MZ header is valid */
	DosHeader = (PIMAGE_DOS_HEADER) ImageStart;
	if (ImageSize < sizeof(IMAGE_DOS_HEADER)
		|| ! ROSSYM_IS_VALID_DOS_HEADER(DosHeader))
    {
		return FALSE;
    }

	/* Locate NT header  */
	NtHeaders = (PIMAGE_NT_HEADERS)((char *) ImageStart + DosHeader->e_lfanew);
	if (ImageSize < DosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS)
		|| ! ROSSYM_IS_VALID_NT_HEADERS(NtHeaders))
    {
		return FALSE;
    }

	SymbolTable = NtHeaders->FileHeader.PointerToSymbolTable;
	NumSymbols = NtHeaders->FileHeader.NumberOfSymbols;

	/* Allocate PeSect array (extended section headers with name strings) */
	OrigSectionHeaders = IMAGE_FIRST_SECTION(NtHeaders);
	SectionHeaders = RosSymAllocMem(NtHeaders->FileHeader.NumberOfSections * sizeof(PeSect));
	if (!SectionHeaders)
		return FALSE;
	RtlZeroMemory(SectionHeaders, NtHeaders->FileHeader.NumberOfSections * sizeof(PeSect));

	/* Copy section headers and convert names to ANSI_STRINGs */
	for (SectionIndex = 0; SectionIndex < NtHeaders->FileHeader.NumberOfSections;
		 SectionIndex++)
	{
		PendingName.Buffer = NULL;
		PendingName.Length = 0;
		PendingName.MaximumLength = 0;

		RtlCopyMemory(&SectionHeaders[SectionIndex].hdr,
		              &OrigSectionHeaders[SectionIndex],
		              sizeof(IMAGE_SECTION_HEADER));

		if (OrigSectionHeaders[SectionIndex].Name[0] != '/') {
			PendingName.Buffer = RosSymAllocMem(IMAGE_SIZEOF_SHORT_NAME);
			if (!PendingName.Buffer) goto freeall;
			RtlCopyMemory(PendingName.Buffer, OrigSectionHeaders[SectionIndex].Name, IMAGE_SIZEOF_SHORT_NAME);
			PendingName.MaximumLength = IMAGE_SIZEOF_SHORT_NAME;
			PendingName.Length = GetStrnlen(PendingName.Buffer, IMAGE_SIZEOF_SHORT_NAME);
		} else {
			/*
			 * Long section name - the name is stored as "/NNN" where NNN is an offset
			 * into the COFF string table. The string table is NOT mapped in memory,
			 * so we need to look up the actual name from within the image.
			 */
			UNICODE_STRING intConv;
			NTSTATUS Status;
			ULONG StringOffset;
			ULONG VirtualOffset = 0;
			PCHAR ResolvedName = NULL;

			if (RtlCreateUnicodeStringFromAsciiz(&intConv, (PCSZ)OrigSectionHeaders[SectionIndex].Name + 1)) {
				Status = RtlUnicodeStringToInteger(&intConv, 10, &StringOffset);
				RtlFreeUnicodeString(&intConv);
				if (NT_SUCCESS(Status)) {
					/*
					 * Use a simple linear search through original section headers.
					 * We can't use pefindrva here because the SectionHeaders array
					 * isn't fully initialized yet.
					 */
					ULONG TargetPhysical = SymbolTable + (NumSymbols * SYMBOL_SIZE) + StringOffset;
					for (ULONG k = 0; k < NtHeaders->FileHeader.NumberOfSections; k++) {
						if (TargetPhysical >= OrigSectionHeaders[k].PointerToRawData &&
							TargetPhysical < OrigSectionHeaders[k].PointerToRawData + OrigSectionHeaders[k].SizeOfRawData) {
							VirtualOffset = TargetPhysical - OrigSectionHeaders[k].PointerToRawData + OrigSectionHeaders[k].VirtualAddress;
							break;
						}
					}

					/* If we couldn't find it in mapped sections, try using the section characteristics
					 * to identify DWARF sections by their typical attributes */
					if (!VirtualOffset) {
						ULONG chars = OrigSectionHeaders[SectionIndex].Characteristics;
						/* DWARF sections have IMAGE_SCN_CNT_INITIALIZED_DATA (0x40) and
						 * IMAGE_SCN_MEM_DISCARDABLE (0x02000000) or IMAGE_SCN_MEM_READ (0x40000000) */
						if ((chars & 0x40) && (chars & 0x42000000)) {
							/* Try to identify by order - DWARF sections typically come after .reloc */
							static const char* DwarfNames[] = {
								".debug_aranges", ".debug_info", ".debug_abbrev", ".debug_line",
								".debug_frame", ".debug_str", ".debug_loc", ".debug_ranges", ".debug_pubnames"
							};
							/* Find the first DWARF section index by looking for the first section
							 * with debug characteristics after all regular sections */
							ULONG FirstDebugSection = 0;
							for (ULONG k = 0; k < NtHeaders->FileHeader.NumberOfSections; k++) {
								ULONG kchars = OrigSectionHeaders[k].Characteristics;
								/* Section with long name and debug-like characteristics */
								if ((OrigSectionHeaders[k].Name[0] == '/') &&
									(kchars & 0x40) && (kchars & 0x42000000)) {
									FirstDebugSection = k;
									break;
								}
							}
							if (FirstDebugSection && SectionIndex >= FirstDebugSection) {
								ULONG DebugIndex = SectionIndex - FirstDebugSection;
								if (DebugIndex < sizeof(DwarfNames)/sizeof(DwarfNames[0])) {
									ResolvedName = (PCHAR)DwarfNames[DebugIndex];
								}
							}
						}
					}
				}
			}

			if (ResolvedName) {
				/* Use the resolved DWARF section name */
				PendingName.Length = strlen(ResolvedName);
				PendingName.MaximumLength = PendingName.Length + 1;
				PendingName.Buffer = RosSymAllocMem(PendingName.MaximumLength);
				if (!PendingName.Buffer) goto freeall;
				RtlCopyMemory(PendingName.Buffer, ResolvedName, PendingName.MaximumLength);
			} else if (VirtualOffset) {
				PendingName.Buffer = RosSymAllocMem(MAXIMUM_DWARF_NAME_SIZE);
				if (!PendingName.Buffer) goto freeall;
				PCHAR StringTarget = ((PCHAR)ImageStart)+VirtualOffset;
				PCHAR EndOfImage = ((PCHAR)ImageStart) + NtHeaders->OptionalHeader.SizeOfImage;
				if (StringTarget < EndOfImage) {
					ULONG PossibleStringLength = EndOfImage - StringTarget;
					if (PossibleStringLength > MAXIMUM_DWARF_NAME_SIZE)
						PossibleStringLength = MAXIMUM_DWARF_NAME_SIZE;
					RtlCopyMemory(PendingName.Buffer, StringTarget, PossibleStringLength);
					PendingName.Length = GetStrnlen(PendingName.Buffer, PossibleStringLength);
					PendingName.MaximumLength = MAXIMUM_DWARF_NAME_SIZE;
				} else {
					/* Can't resolve - use raw offset form */
					RosSymFreeMem(PendingName.Buffer);
					PendingName.Buffer = RosSymAllocMem(IMAGE_SIZEOF_SHORT_NAME);
					if (!PendingName.Buffer) goto freeall;
					RtlCopyMemory(PendingName.Buffer, OrigSectionHeaders[SectionIndex].Name, IMAGE_SIZEOF_SHORT_NAME);
					PendingName.MaximumLength = IMAGE_SIZEOF_SHORT_NAME;
					PendingName.Length = GetStrnlen(PendingName.Buffer, IMAGE_SIZEOF_SHORT_NAME);
				}
			} else {
				/* String table not in memory - use raw name bytes */
				PendingName.Buffer = RosSymAllocMem(IMAGE_SIZEOF_SHORT_NAME);
				if (!PendingName.Buffer) goto freeall;
				RtlCopyMemory(PendingName.Buffer, OrigSectionHeaders[SectionIndex].Name, IMAGE_SIZEOF_SHORT_NAME);
				PendingName.MaximumLength = IMAGE_SIZEOF_SHORT_NAME;
				PendingName.Length = GetStrnlen(PendingName.Buffer, IMAGE_SIZEOF_SHORT_NAME);
			}
		}
		*ANSI_NAME_STRING(&SectionHeaders[SectionIndex]) = PendingName;
		PendingName.Buffer = NULL;
	}

	Pe *pe = RosSymAllocMem(sizeof(*pe));
	if (!pe) goto freeall;
	pe->fd = ImageStart;
	pe->e2 = peget2;
	pe->e4 = peget4;
	pe->e8 = peget8;
	pe->loadbase = (ULONG_PTR)ImageStart;
	pe->imagebase = NtHeaders->OptionalHeader.ImageBase;
	pe->imagesize = NtHeaders->OptionalHeader.SizeOfImage;
	pe->codestart = NtHeaders->OptionalHeader.BaseOfCode;
#ifdef _WIN64
	pe->datastart = 0;
#else
	pe->datastart = NtHeaders->OptionalHeader.BaseOfData;
#endif
	pe->nsections = NtHeaders->FileHeader.NumberOfSections;
	pe->sect = SectionHeaders;
	pe->loadsection = loadmemsection;
	*RosSymInfo = dwarfopen(pe);

	return !!*RosSymInfo;

freeall:
	if (PendingName.Buffer) RosSymFreeMem(PendingName.Buffer);
	for (SectionIndex = 0; SectionIndex < NtHeaders->FileHeader.NumberOfSections;
		 SectionIndex++)
		RtlFreeAnsiString(ANSI_NAME_STRING(&SectionHeaders[SectionIndex]));
	RosSymFreeMem(SectionHeaders);

	return FALSE;
}

/* EOF */
