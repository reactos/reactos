/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS kernel
 * FILE:            lib/rossym/fromfile.c
 * PURPOSE:         Creating rossym info from a file
 *
 * PROGRAMMERS:     Ge van Geldorp (gvg@reactos.com)
 */

#include <precomp.h>

#define NDEBUG
#include <debug.h>

#define SYMBOL_SIZE 18

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
        werrstr("Could not rewind file\n");
        return FALSE;
    }
    if (! RosSymReadFile(FileContext, &DosHeader, sizeof(IMAGE_DOS_HEADER)))
    {
        werrstr("Failed to read DOS header %x\n", RosSymStatus);
        return FALSE;
    }
    if (! ROSSYM_IS_VALID_DOS_HEADER(&DosHeader))
    {
        werrstr("Image doesn't have a valid DOS header\n");
        return FALSE;
    }

    /* Load NT headers */
    if (! RosSymSeekFile(FileContext, DosHeader.e_lfanew))
    {
        werrstr("Failed seeking to NT headers\n");
        return FALSE;
    }
    if (! RosSymReadFile(FileContext, &NtHeaders, sizeof(IMAGE_NT_HEADERS)))
    {
        werrstr("Failed to read NT headers\n");
        return FALSE;
    }
    if (! ROSSYM_IS_VALID_NT_HEADERS(&NtHeaders))
    {
        werrstr("Image doesn't have a valid PE header\n");
        return FALSE;
    }

    SymbolTable = NtHeaders.FileHeader.PointerToSymbolTable;
    NumSymbols = NtHeaders.FileHeader.NumberOfSymbols;

    if (!NumSymbols)
    {
        werrstr("Image doesn't have debug symbols\n");
        return FALSE;
    }

    DPRINT("SymbolTable %x NumSymbols %x\n", SymbolTable, NumSymbols);

    /* Load section headers */
    if (! RosSymSeekFile(FileContext, (char *) IMAGE_FIRST_SECTION(&NtHeaders) -
                         (char *) &NtHeaders + DosHeader.e_lfanew))
    {
        werrstr("Failed seeking to section headers\n");
        return FALSE;
    }
    DPRINT("Alloc section headers\n");
    SectionHeaders = RosSymAllocMem(NtHeaders.FileHeader.NumberOfSections
                                    * sizeof(IMAGE_SECTION_HEADER));
    if (NULL == SectionHeaders)
    {
        werrstr("Failed to allocate memory for %u section headers\n",
                NtHeaders.FileHeader.NumberOfSections);
        return FALSE;
    }
    if (! RosSymReadFile(FileContext, SectionHeaders,
                         NtHeaders.FileHeader.NumberOfSections
                         * sizeof(IMAGE_SECTION_HEADER)))
    {
        RosSymFreeMem(SectionHeaders);
        werrstr("Failed to read section headers\n");
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

            if (!RtlCreateUnicodeStringFromAsciiz(&intConv, (PCSZ)SectionHeaders[SectionIndex].Name + 1))
                goto freeall;
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
    pe->imagebase = pe->loadbase = NtHeaders.OptionalHeader.ImageBase;
    pe->imagesize = NtHeaders.OptionalHeader.SizeOfImage;
    pe->codestart = NtHeaders.OptionalHeader.BaseOfCode;
    pe->datastart = NtHeaders.OptionalHeader.BaseOfData;
    pe->loadsection = loaddisksection;
    *RosSymInfo = dwarfopen(pe);

    return TRUE;

freeall:
    for (SectionIndex = 0; SectionIndex < NtHeaders.FileHeader.NumberOfSections;
         SectionIndex++)
        RtlFreeAnsiString(ANSI_NAME_STRING(&SectionHeaders[SectionIndex]));
    RosSymFreeMem(SectionHeaders);

    return FALSE;
}

/* EOF */
