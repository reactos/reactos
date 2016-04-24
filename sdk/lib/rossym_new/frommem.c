/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/frommem.c
 * PURPOSE:         Creating rossym info from an in-memory image
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#define NTOSAPI
#include <ntifs.h>
#include <ndk/ntndk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"
#include <ntimage.h>

#define NDEBUG
#include <debug.h>

#include "dwarf.h"
#include "pe.h"

BOOLEAN
RosSymCreateFromMem(PVOID ImageStart, ULONG_PTR ImageSize, PROSSYM_INFO *RosSymInfo)
{
	ANSI_STRING AnsiNameString = { };
	PIMAGE_DOS_HEADER DosHeader;
	PIMAGE_NT_HEADERS NtHeaders;
	PIMAGE_SECTION_HEADER SectionHeaders;
	ULONG SectionIndex;
	unsigned SymbolTable, NumSymbols;
	
	/* Check if MZ header is valid */
	DosHeader = (PIMAGE_DOS_HEADER) ImageStart;
	if (ImageSize < sizeof(IMAGE_DOS_HEADER)
		|| ! ROSSYM_IS_VALID_DOS_HEADER(DosHeader))
    {
		DPRINT1("Image doesn't have a valid DOS header\n");
		return FALSE;
    }
	
	/* Locate NT header  */
	NtHeaders = (PIMAGE_NT_HEADERS)((char *) ImageStart + DosHeader->e_lfanew);
	if (ImageSize < DosHeader->e_lfanew + sizeof(IMAGE_NT_HEADERS)
		|| ! ROSSYM_IS_VALID_NT_HEADERS(NtHeaders))
    {
		DPRINT1("Image doesn't have a valid PE header\n");
		return FALSE;
    }
	
	SymbolTable = NtHeaders->FileHeader.PointerToSymbolTable;
	NumSymbols = NtHeaders->FileHeader.NumberOfSymbols;
	
	/* Search for the section header */
	ULONG SectionHeaderSize = NtHeaders->FileHeader.NumberOfSections * sizeof(IMAGE_SECTION_HEADER);
	SectionHeaders = RosSymAllocMem(SectionHeaderSize);
	RtlCopyMemory(SectionHeaders, IMAGE_FIRST_SECTION(NtHeaders), SectionHeaderSize);
	
	// Convert names to ANSI_STRINGs
	for (SectionIndex = 0; SectionIndex < NtHeaders->FileHeader.NumberOfSections;
		 SectionIndex++) 
	{
		if (SectionHeaders[SectionIndex].Name[0] != '/') {
			AnsiNameString.Buffer = RosSymAllocMem(IMAGE_SIZEOF_SHORT_NAME);
			RtlCopyMemory(AnsiNameString.Buffer, SectionHeaders[SectionIndex].Name, IMAGE_SIZEOF_SHORT_NAME);
			AnsiNameString.MaximumLength = IMAGE_SIZEOF_SHORT_NAME;
			AnsiNameString.Length = GetStrnlen(AnsiNameString.Buffer, IMAGE_SIZEOF_SHORT_NAME);
		} else {
			UNICODE_STRING intConv;
			NTSTATUS Status;
			ULONG StringOffset;
			
			if (!RtlCreateUnicodeStringFromAsciiz(&intConv, (PCSZ)SectionHeaders[SectionIndex].Name + 1))
				goto freeall;
			Status = RtlUnicodeStringToInteger(&intConv, 10, &StringOffset);
			RtlFreeUnicodeString(&intConv);
			if (!NT_SUCCESS(Status)) goto freeall;
			ULONG VirtualOffset = pefindrva(SectionHeaders, NtHeaders->FileHeader.NumberOfSections, SymbolTable+(NumSymbols*SYMBOL_SIZE)+StringOffset);
			if (!VirtualOffset) goto freeall;
			AnsiNameString.Buffer = RosSymAllocMem(MAXIMUM_DWARF_NAME_SIZE);
			if (!AnsiNameString.Buffer) goto freeall;
			PCHAR StringTarget = ((PCHAR)ImageStart)+VirtualOffset;
			PCHAR EndOfImage = ((PCHAR)ImageStart) + NtHeaders->OptionalHeader.SizeOfImage;
			if (StringTarget >= EndOfImage) goto freeall;
			ULONG PossibleStringLength = EndOfImage - StringTarget;
			if (PossibleStringLength > MAXIMUM_DWARF_NAME_SIZE)
				PossibleStringLength = MAXIMUM_DWARF_NAME_SIZE;
			RtlCopyMemory(AnsiNameString.Buffer, StringTarget, PossibleStringLength);
			AnsiNameString.Length = strlen(AnsiNameString.Buffer);
			AnsiNameString.MaximumLength = MAXIMUM_DWARF_NAME_SIZE;		  
		}
		memcpy
			(&SectionHeaders[SectionIndex],
			 &AnsiNameString,
			 sizeof(AnsiNameString));
	}
	
	Pe *pe = RosSymAllocMem(sizeof(*pe));
	pe->fd = ImageStart;
	pe->e2 = peget2;
	pe->e4 = peget4;
	pe->e8 = peget8;
	pe->loadbase = (ULONG)ImageStart;
	pe->imagebase = NtHeaders->OptionalHeader.ImageBase;
	pe->imagesize = NtHeaders->OptionalHeader.SizeOfImage;
	pe->nsections = NtHeaders->FileHeader.NumberOfSections;
	pe->sect = SectionHeaders;
	pe->nsymbols = NtHeaders->FileHeader.NumberOfSymbols;
	pe->symtab = malloc(pe->nsymbols * sizeof(CoffSymbol));
	PSYMENT SymbolData = (PSYMENT)
		(((PCHAR)ImageStart) + 
		 pefindrva
		 (pe->sect, 
		  pe->nsections, 
		  NtHeaders->FileHeader.PointerToSymbolTable));
	int i, j;
	for (i = 0, j = 0; i < pe->nsymbols; i++) {
		if ((SymbolData[i].e_scnum < 1) || 
			(SymbolData[i].e_sclass != C_EXT &&
			 SymbolData[i].e_sclass != C_STAT))
			continue;
		int section = SymbolData[i].e_scnum - 1;
		if (SymbolData[i].e.e.e_zeroes) {
			pe->symtab[j].name = malloc(sizeof(SymbolData[i].e.e_name)+1);
			strcpy(pe->symtab[j].name, SymbolData[i].e.e_name);
		} else {
			PCHAR SymbolName = ((PCHAR)ImageStart) + 
				pefindrva
				(pe->sect, 
				 pe->nsections, 
				 NtHeaders->FileHeader.PointerToSymbolTable + 
				 (NtHeaders->FileHeader.NumberOfSymbols * 18) + 
				 SymbolData[i].e.e.e_offset);
			pe->symtab[j].name = malloc(strlen(SymbolName)+1);
			strcpy(pe->symtab[j].name, SymbolName);
		}
		if (pe->symtab[j].name[0] == '.') {
			free(pe->symtab[j].name);
			continue;
		}
		pe->symtab[j].address = pe->sect[section].VirtualAddress + SymbolData[i].e_value;
		j++;
	}
	pe->nsymbols = j;
	pe->loadsection = loadmemsection;
	*RosSymInfo = dwarfopen(pe);  

	return !!*RosSymInfo;
	
freeall:
	if (AnsiNameString.Buffer) RosSymFreeMem(AnsiNameString.Buffer);
	for (SectionIndex = 0; SectionIndex < NtHeaders->FileHeader.NumberOfSections;
		 SectionIndex++)
		RtlFreeAnsiString(ANSI_NAME_STRING(&SectionHeaders[SectionIndex]));
	RosSymFreeMem(SectionHeaders);
	
	return FALSE;
}

/* EOF */
