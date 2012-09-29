/*
 * Usage: rsym input-file output-file
 *
 * There are two sources of information: the .stab/.stabstr
 * sections of the executable and the COFF symbol table. Most
 * of the information is in the .stab/.stabstr sections.
 * However, most of our asm files don't contain .stab directives,
 * so routines implemented in assembler won't show up in the
 * .stab section. They are present in the COFF symbol table.
 * So, we mostly use the .stab/.stabstr sections, but we augment
 * the info there with info from the COFF symbol table when
 * possible.
 *
 * This is a tool and is compiled using the host compiler,
 * i.e. on Linux gcc and not mingw-gcc (cross-compiler).
 * Therefore we can't include SDK headers and we have to
 * duplicate some definitions here.
 * Also note that the internal functions are "old C-style",
 * returning an int, where a return of 0 means success and
 * non-zero is failure.
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "rsym.h"

int
IsDebugSection(PIMAGE_SECTION_HEADER Section)
{
    /* This is a hack, but works for us */
    return (Section->Name[0] == '/');
}

int main(int argc, char* argv[])
{
    unsigned int i;
    PSYMBOLFILE_HEADER SymbolFileHeader;
    PIMAGE_NT_HEADERS NtHeaders;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_FILE_HEADER FileHeader;
    PIMAGE_OPTIONAL_HEADER OptionalHeader;
    PIMAGE_SECTION_HEADER SectionHeaders, LastSection;
    char* path1;
    char* path2;
    FILE* out;
    size_t FileSize;
    void *FileData;
    char elfhdr[] = { '\377', 'E', 'L', 'F' };

    if (argc != 3)
    {
        fprintf(stderr, "Usage: rsym <exefile> <symfile>\n");
        exit(1);
    }

    path1 = convert_path(argv[1]);
    path2 = convert_path(argv[2]);

    /* Load the input file into memory */
    FileData = load_file( path1, &FileSize);
    if ( !FileData )
    {
        fprintf(stderr, "An error occured loading '%s'\n", path1);
        exit(1);
    }

    /* Check if MZ header exists  */
    DosHeader = (PIMAGE_DOS_HEADER) FileData;
    if (DosHeader->e_magic != IMAGE_DOS_MAGIC || DosHeader->e_lfanew == 0L)
    {
        /* Ignore elf */
        if (!memcmp(DosHeader, elfhdr, sizeof(elfhdr)))
            exit(0);
        perror("Input file is not a PE image.\n");
        free(FileData);
        exit(1);
    }

    /* Locate the headers */
    NtHeaders = (PIMAGE_NT_HEADERS)((char*)FileData + DosHeader->e_lfanew);
    FileHeader = &NtHeaders->FileHeader;
    OptionalHeader = &NtHeaders->OptionalHeader;

    /* Locate PE section headers  */
    SectionHeaders = (PIMAGE_SECTION_HEADER)((char*)OptionalHeader +
                                             FileHeader->SizeOfOptionalHeader);

    /* Loop all sections */
    for (i = 0; i < FileHeader->NumberOfSections; i++)
    {
        /* Check if this is a debug section */
        if (IsDebugSection(&SectionHeaders[i]))
        {
            /* Make sure we have the correct characteristics */
            SectionHeaders[i].Characteristics |= IMAGE_SCN_CNT_INITIALIZED_DATA;
            SectionHeaders[i].Characteristics &= ~(IMAGE_SCN_MEM_PURGEABLE | IMAGE_SCN_MEM_DISCARDABLE);
        }
    }

    /* Get a pointer to the last section header */
    LastSection = &SectionHeaders[FileHeader->NumberOfSections - 1];

    /* Set the size of the last section to cover the rest of the PE */
    LastSection->SizeOfRawData = FileSize - LastSection->PointerToRawData;

    /* Check if the virtual section size is smaller than the raw data */
    if (LastSection->Misc.VirtualSize < LastSection->SizeOfRawData)
    {
        /* Make sure the virtual size of the section cover the raw data */
        LastSection->Misc.VirtualSize = ROUND_UP(LastSection->SizeOfRawData,
                                                 OptionalHeader->SectionAlignment);

        /* Fix up image size */
        OptionalHeader->SizeOfImage = LastSection->VirtualAddress +
                                      LastSection->Misc.VirtualSize;
    }

    /* Open the output file */
    out = fopen(path2, "wb");
    if (out == NULL)
    {
        perror("Cannot open output file");
        free(FileData);
        exit(1);
    }

    /* Write the output file */
    fwrite(FileData, 1, FileSize, out);
    fclose(out);
    free(FileData);

    return 0;
}

/* EOF */
