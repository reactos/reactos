/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/fromfile.c
 * PURPOSE:         Creating rossym info from a file
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#define NTOSAPI
#include <ntifs.h>
#include <ndk/ntndk.h>
#include <reactos/rossym.h>
#include "rossympriv.h"
#include "pe.h"
#include <ntimage.h>

#include "dwarf.h"

#define NDEBUG
#include <debug.h>

extern NTSTATUS RosSymStatus;

BOOLEAN
RosSymCreateFromFile(PVOID FileContext, PROSSYM_INFO *RosSymInfo)
{
  IMAGE_DOS_HEADER DosHeader;
  IMAGE_NT_HEADERS NtHeaders;
  PIMAGE_SECTION_HEADER SectionHeaders;
  unsigned SectionIndex;
  unsigned SymbolTable, NumSymbols;

  /* Load DOS header */
  if (! RosSymSeekFile(FileContext, 0))
    {
	  DPRINT1("Could not rewind file\n");
	  return FALSE;
	}
  if (! RosSymReadFile(FileContext, &DosHeader, sizeof(IMAGE_DOS_HEADER)))
    {
	  DPRINT1("Failed to read DOS header %x\n", RosSymStatus);
      return FALSE;
    }
  if (! ROSSYM_IS_VALID_DOS_HEADER(&DosHeader))
    {
      DPRINT1("Image doesn't have a valid DOS header\n");
      return FALSE;
    }

  /* Load NT headers */
  if (! RosSymSeekFile(FileContext, DosHeader.e_lfanew))
    {
      DPRINT1("Failed seeking to NT headers\n");
      return FALSE;
    }
  if (! RosSymReadFile(FileContext, &NtHeaders, sizeof(IMAGE_NT_HEADERS)))
    {
      DPRINT1("Failed to read NT headers\n");
      return FALSE;
    }
  if (! ROSSYM_IS_VALID_NT_HEADERS(&NtHeaders))
    {
      DPRINT1("Image doesn't have a valid PE header\n");
      return FALSE;
    }

  SymbolTable = NtHeaders.FileHeader.PointerToSymbolTable;
  NumSymbols = NtHeaders.FileHeader.NumberOfSymbols;

  if (!NumSymbols)
    {
      DPRINT1("Image doesn't have debug symbols\n");
      return FALSE;
    }

  DPRINT("SymbolTable %x NumSymbols %x\n", SymbolTable, NumSymbols);

  /* Load section headers */
  if (! RosSymSeekFile(FileContext, (char *) IMAGE_FIRST_SECTION(&NtHeaders) -
                                    (char *) &NtHeaders + DosHeader.e_lfanew))
    {
      DPRINT1("Failed seeking to section headers\n");
      return FALSE;
    }
  DPRINT("Alloc section headers\n");
  SectionHeaders = RosSymAllocMem(NtHeaders.FileHeader.NumberOfSections
                                  * sizeof(IMAGE_SECTION_HEADER));
  if (NULL == SectionHeaders)
    {
      DPRINT1("Failed to allocate memory for %u section headers\n",
              NtHeaders.FileHeader.NumberOfSections);
      return FALSE;
    }
  if (! RosSymReadFile(FileContext, SectionHeaders,
                       NtHeaders.FileHeader.NumberOfSections
                       * sizeof(IMAGE_SECTION_HEADER)))
    {
      RosSymFreeMem(SectionHeaders);
      DPRINT1("Failed to read section headers\n");
      return FALSE;
    }

  // Convert names to ANSI_STRINGs
  for (SectionIndex = 0; SectionIndex < NtHeaders.FileHeader.NumberOfSections;
	   SectionIndex++) 
  {
	  ANSI_STRING astr;
	  if (SectionHeaders[SectionIndex].Name[0] != '/') {
		  DPRINT("Short name string %d, %s\n", SectionIndex, SectionHeaders[SectionIndex].Name);
		  astr.Buffer = RosSymAllocMem(IMAGE_SIZEOF_SHORT_NAME);
		  memcpy(astr.Buffer, SectionHeaders[SectionIndex].Name, IMAGE_SIZEOF_SHORT_NAME);
		  astr.MaximumLength = IMAGE_SIZEOF_SHORT_NAME;
		  astr.Length = GetStrnlen(astr.Buffer, IMAGE_SIZEOF_SHORT_NAME);
	  } else {
		  UNICODE_STRING intConv;
		  NTSTATUS Status;
		  ULONG StringOffset;

		  Status = RtlCreateUnicodeStringFromAsciiz(&intConv, (PCSZ)SectionHeaders[SectionIndex].Name + 1);
		  if (!NT_SUCCESS(Status)) goto freeall;
		  Status = RtlUnicodeStringToInteger(&intConv, 10, &StringOffset);
		  RtlFreeUnicodeString(&intConv);
		  if (!NT_SUCCESS(Status)) goto freeall;
		  if (!RosSymSeekFile(FileContext, SymbolTable + NumSymbols * SYMBOL_SIZE + StringOffset))
			  goto freeall;
		  astr.Buffer = RosSymAllocMem(MAXIMUM_DWARF_NAME_SIZE);
		  if (!RosSymReadFile(FileContext, astr.Buffer, MAXIMUM_DWARF_NAME_SIZE))
			  goto freeall;
		  astr.Length = GetStrnlen(astr.Buffer, MAXIMUM_DWARF_NAME_SIZE);
		  astr.MaximumLength = MAXIMUM_DWARF_NAME_SIZE;		  
		  DPRINT("Long name %d, %s\n", SectionIndex, astr.Buffer);
	  }
	  *ANSI_NAME_STRING(&SectionHeaders[SectionIndex]) = astr;
  }

  DPRINT("Done with sections\n");
  Pe *pe = RosSymAllocMem(sizeof(*pe));
  pe->fd = FileContext;
  pe->e2 = peget2;
  pe->e4 = peget4;
  pe->e8 = peget8;
  pe->nsections = NtHeaders.FileHeader.NumberOfSections;
  pe->sect = SectionHeaders;
  pe->nsymbols = NtHeaders.FileHeader.NumberOfSymbols;
  pe->symtab = malloc(pe->nsymbols * sizeof(CoffSymbol));
  SYMENT SymbolData;
  int i, j;
  DPRINT("Getting symbol data\n");
  ASSERT(sizeof(SymbolData) == 18);
  for (i = 0, j = 0; i < pe->nsymbols; i++) {
	  if (!RosSymSeekFile
		  (FileContext, 
		   NtHeaders.FileHeader.PointerToSymbolTable + i * sizeof(SymbolData)))
		  goto freeall;
	  if (!RosSymReadFile(FileContext, &SymbolData, sizeof(SymbolData)))
		  goto freeall;
	  if ((SymbolData.e_scnum < 1) || 
		  (SymbolData.e_sclass != C_EXT &&
		   SymbolData.e_sclass != C_STAT))
		  continue;
	  int section = SymbolData.e_scnum - 1;
	  if (SymbolData.e.e.e_zeroes) {
		  pe->symtab[j].name = malloc(sizeof(SymbolData.e.e_name)+1);
		  memcpy(pe->symtab[j].name, SymbolData.e.e_name, sizeof(SymbolData.e.e_name));
		  pe->symtab[j].name[sizeof(SymbolData.e.e_name)] = 0;
	  } else {
		  if (!RosSymSeekFile
			  (FileContext, 
			   NtHeaders.FileHeader.PointerToSymbolTable + 
			   (NtHeaders.FileHeader.NumberOfSymbols * 18) + 
			   SymbolData.e.e.e_offset))
			  goto freeall;
		  pe->symtab[j].name = malloc(MAXIMUM_COFF_SYMBOL_LENGTH+1);
		  pe->symtab[j].name[MAXIMUM_COFF_SYMBOL_LENGTH] = 0;
		  // It's possible that we've got a string just at the end of the file
		  // we'll skip that symbol if needed
		  if (!RosSymReadFile(FileContext, pe->symtab[j].name, MAXIMUM_COFF_SYMBOL_LENGTH)) {
			  free(pe->symtab[j].name);
			  continue;
		  }
	  }
	  if (pe->symtab[j].name[0] == '.') {
		  free(pe->symtab[j].name);
		  continue;
	  }
	  pe->symtab[j].address = pe->sect[section].VirtualAddress + SymbolData.e_value;
	  j++;
  }
  DPRINT("%d symbols\n", j);
  pe->nsymbols = j;
  pe->imagebase = pe->loadbase = NtHeaders.OptionalHeader.ImageBase;
  pe->imagesize = NtHeaders.OptionalHeader.SizeOfImage;
  pe->loadsection = loaddisksection;
  DPRINT("do dwarfopen\n");
  *RosSymInfo = dwarfopen(pe);
  DPRINT("done %x\n", *RosSymInfo);

  return TRUE;

freeall:
  for (SectionIndex = 0; SectionIndex < NtHeaders.FileHeader.NumberOfSections;
	   SectionIndex++)
	  RtlFreeAnsiString(ANSI_NAME_STRING(&SectionHeaders[SectionIndex]));
  RosSymFreeMem(SectionHeaders);

  return FALSE;
}

/* EOF */
