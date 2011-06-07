#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../pecoff.h"

static
void
Usage(void)
{
    printf("Converts a coff object file into a raw binary file.\n"
           "Syntax: obj2bin <source file> <dest file>\n");
}

int main(int argc, char *argv[])
{
    char *pszSourceFile;
    char *pszDestFile;
    FILE *pSourceFile, *pDestFile;
    IMAGE_FILE_HEADER FileHeader;
    IMAGE_SECTION_HEADER SectionHeader;
    unsigned int i;
    size_t nSize;
    void *pData;

    if ((argc != 3) || (strcmp(argv[1], "--help") == 0)) Usage();

    pszSourceFile = argv[1];
    pszDestFile = argv[2];

    pSourceFile = fopen(pszSourceFile, "rb");
    if (!pSourceFile)
    {
        fprintf(stderr, "Couldn't open source file '%s'\n", pszSourceFile);
        return -1;
    }

    pDestFile = fopen(pszDestFile, "wb");
    if (!pszDestFile)
    {
        fprintf(stderr, "Couldn't open dest file '%s'\n", pszDestFile);
        return -2;
    }

    /* Load the coff header */
    nSize = fread(&FileHeader, 1, sizeof(FileHeader), pSourceFile);
    if (nSize != sizeof(FileHeader))
    {
        fprintf(stderr, "Failed to read source file\n");
        return -3;
    }

    /* Jump to section headers (skip optional header) */
    if (fseek(pSourceFile, FileHeader.SizeOfOptionalHeader, SEEK_CUR))
    {
        fprintf(stderr, "Failed to set file pointer\n");
        return -4;
    }

    /* Loop all sections */
    for (i = 0; i < FileHeader.NumberOfSections; i++)
    {
        /* Read section header */
        nSize = fread(&SectionHeader, 1, sizeof(SectionHeader), pSourceFile);
        if (nSize != sizeof(SectionHeader))
        {
            fprintf(stderr, "Failed to read section %ld file\n", i);
            return -5;
        }

        /* Check if this is '.text' section */
        if (strcmp(SectionHeader.Name, ".text") == 0) break;
    }

    if (i == FileHeader.NumberOfSections)
    {
        fprintf(stderr, "No .text section found\n");
        return -6;
    }

    /* Move file pointer to the start of the section*/
    if (fseek(pSourceFile, SectionHeader.PointerToRawData, SEEK_SET))
    {
        fprintf(stderr, "Failed to set file pointer\n");
        return -7;
    }

    /* Allocate memory for the section */
    pData = malloc(SectionHeader.SizeOfRawData);
    if (!pData)
    {
        fprintf(stderr, "Failed to allocate %ld bytes\n", SectionHeader.SizeOfRawData);
        return -8;
    }

    /* Read section data */
    if (!fread(pData, SectionHeader.SizeOfRawData, 1, pSourceFile))
    {
        fprintf(stderr, "Failed to read section %ld file\n", i);
        return -5;
    }

    /* Write the section to the destination file */
    if (!fwrite(pData, SectionHeader.SizeOfRawData, 1, pDestFile))
    {
        fprintf(stderr, "Failed to write data\n");
        return -9;
    }

    fclose(pDestFile);
    fclose(pSourceFile);

    return 0;
}

