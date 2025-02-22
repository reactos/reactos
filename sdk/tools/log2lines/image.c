/*
 * ReactOS log2lines
 * Written by Jan Roeloffzen
 *
 * - Image functions for symbol info
 */

#include <errno.h>
#include <string.h>
#include <rsym.h>

#include "compat.h"
#include "util.h"
#include "options.h"
#include "log2lines.h"
#include <sys/types.h>

static PIMAGE_SECTION_HEADER
find_rossym_section(PIMAGE_FILE_HEADER PEFileHeader, PIMAGE_SECTION_HEADER PESectionHeaders)
{
    size_t i;
    for (i = 0; i < PEFileHeader->NumberOfSections; i++)
    {
        if (0 == strcmp((char *)PESectionHeaders[i].Name, ".rossym"))
            return &PESectionHeaders[i];
    }
    return NULL;
}

size_t
fixup_offset(size_t ImageBase, size_t offset)
{
    if (offset > ABS_TRESHOLD)
        offset -= ImageBase;
    return offset;
}

PROSSYM_ENTRY
find_offset(void *data, size_t offset)
{
    PSYMBOLFILE_HEADER RosSymHeader = (PSYMBOLFILE_HEADER)data;
    PROSSYM_ENTRY Entries = (PROSSYM_ENTRY)((char *)data + RosSymHeader->SymbolsOffset);
    size_t symbols = RosSymHeader->SymbolsLength / sizeof(ROSSYM_ENTRY);
    size_t i;

    for (i = 0; i < symbols; i++)
    {
        if (Entries[i].Address > offset)
        {
            if (!i--)
                return NULL;
            else
                return &Entries[i];
        }
    }
    return NULL;
}

PIMAGE_SECTION_HEADER
get_sectionheader(const void *FileData)
{
    PIMAGE_DOS_HEADER PEDosHeader;
    PIMAGE_FILE_HEADER PEFileHeader;
    PIMAGE_OPTIONAL_HEADER PEOptHeader;
    PIMAGE_SECTION_HEADER PESectionHeaders;
    PIMAGE_SECTION_HEADER PERosSymSectionHeader;
    size_t ImageBase;

    /* Check if MZ header exists */
    PEDosHeader = (PIMAGE_DOS_HEADER)FileData;
    if (PEDosHeader->e_magic != IMAGE_DOS_MAGIC || PEDosHeader->e_lfanew == 0L)
    {
        l2l_dbg(0, "Input file is not a PE image.\n");
        summ.offset_errors++;
        return NULL;
    }

    /* Locate PE file header */
    /* sizeof(ULONG) = sizeof(MAGIC) */
    PEFileHeader = (PIMAGE_FILE_HEADER)((char *)FileData + PEDosHeader->e_lfanew + sizeof(ULONG));

    /* Locate optional header */
    PEOptHeader = (PIMAGE_OPTIONAL_HEADER)(PEFileHeader + 1);
    ImageBase = PEOptHeader->ImageBase;

    /* Locate PE section headers */
    PESectionHeaders = (PIMAGE_SECTION_HEADER)((char *)PEOptHeader + PEFileHeader->SizeOfOptionalHeader);

    /* find rossym section */
    PERosSymSectionHeader = find_rossym_section(PEFileHeader, PESectionHeaders);
    if (!PERosSymSectionHeader)
    {
        l2l_dbg(0, "Couldn't find rossym section in executable\n");
        summ.offset_errors++;
        return NULL;
    }

    return PERosSymSectionHeader;
}

int
get_ImageBase(char *fname, size_t *ImageBase)
{
    IMAGE_DOS_HEADER PEDosHeader;
    IMAGE_FILE_HEADER PEFileHeader;
    IMAGE_OPTIONAL_HEADER PEOptHeader;

    FILE *fr;
    off_t readLen;
    int res;

    *ImageBase = INVALID_BASE;
    fr = fopen(fname, "rb");
    if (!fr)
    {
        l2l_dbg(3, "get_ImageBase, cannot open '%s' (%s)\n", fname, strerror(errno));
        return 1;
    }

    readLen = fread(&PEDosHeader, sizeof(IMAGE_DOS_HEADER), 1, fr);
    if (1 != readLen)
    {
        l2l_dbg(1, "get_ImageBase %s, read error IMAGE_DOS_HEADER (%s)\n", fname, strerror(errno));
        fclose(fr);
        return 2;
    }

    /* Check if MZ header exists */
    if (PEDosHeader.e_magic != IMAGE_DOS_MAGIC || PEDosHeader.e_lfanew == 0L)
    {
        l2l_dbg(2, "get_ImageBase %s, MZ header missing\n", fname);
        fclose(fr);
        return 3;
    }

    /* Locate PE file header */
    res = fseek(fr, PEDosHeader.e_lfanew + sizeof(ULONG), SEEK_SET);
    readLen = fread(&PEFileHeader, sizeof(IMAGE_FILE_HEADER), 1, fr);
    if (1 != readLen)
    {
        l2l_dbg(1, "get_ImageBase %s, read error IMAGE_FILE_HEADER (%s)\n", fname, strerror(errno));
        fclose(fr);
        return 4;
    }

    /* Locate optional header */
    readLen = fread(&PEOptHeader, sizeof(IMAGE_OPTIONAL_HEADER), 1, fr);
    if (1 != readLen)
    {
        l2l_dbg(1, "get_ImageBase %s, read error IMAGE_OPTIONAL_HEADER (%s)\n", fname, strerror(errno));
        fclose(fr);
        return 5;
    }

    /* Check if it's really an IMAGE_OPTIONAL_HEADER we are interested in */
    if (PEOptHeader.Magic != IMAGE_NT_OPTIONAL_HDR32_MAGIC &&
        PEOptHeader.Magic != IMAGE_NT_OPTIONAL_HDR64_MAGIC)
    {
        l2l_dbg(2, "get_ImageBase %s, not an IMAGE_NT_OPTIONAL_HDR 32/64 bit\n", fname);
        fclose(fr);
        return 6;
    }

    *ImageBase = PEOptHeader.ImageBase;
    fclose(fr);
    return 0;
}

/* EOF */
