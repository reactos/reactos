/*
 *  Generates assembly definitions from the target headers.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

#define IMAGE_FILE_MACHINE_I386  0x014c
#define IMAGE_FILE_MACHINE_AMD64 0x8664
#define IMAGE_FILE_MACHINE_ARMNT 0x01c4
#define IMAGE_FILE_MACHINE_ARM64 0xaa64

#ifdef _MSC_VER
#define PRIx64 "I64x"
#else
#include <inttypes.h>
#define _stricmp strcasecmp
#endif

typedef struct
{
    char Type;
    char Name[55];
    uint64_t Value;
} ASMGENDATA;

#define TYPE_END 0
#define TYPE_RAW 1
#define TYPE_CONSTANT 2
#define TYPE_HEADER 3

int main(int argc, char* argv[])
{
    FILE *input, *output;
    ASMGENDATA data;
    int i, result = -1;
    int ms_format = 0;
    char header[20];
    uint32_t e_lfanew, signature;
    uint16_t Machine, NumberOfSections, SizeOfOptionalHeader;
    typedef struct
    {
        char Name[8];
        uint32_t VirtualSize;
        uint32_t VirtualAddress;
        uint32_t RawSize;
        uint32_t RawAddress;
        uint32_t RelocAddress;
        uint32_t LineNumbers;
        uint16_t RelocationsNumber;
        uint16_t LineNumbersNumber;
        uint32_t Characteristics;
    } SECTION;
    SECTION section;

    if (argc >= 4 && _stricmp(argv[3], "-ms") == 0) ms_format = 1;

    /* Open the input file */
    input = fopen(argv[1], "rb");
    if (!input)
    {
        fprintf(stderr, "Could not open input file '%s'\n", argv[1]);
        return -1;
    }

    /* Open the output file */
    output = fopen(argv[2], "w");
    if (!output)
    {
        fclose(input);
        fprintf(stderr, "Could not open output file '%s'\n", argv[2]);
        return -1;
    }

    /* Read the DOS header */
    if (fread(&header, 1, 2, input) != 2)
    {
        fprintf(stderr, "Error reading header.\n");
        goto quit;
    }

    if (header[0] != 0x4d || header[1] != 0x5a)
    {
        fprintf(stderr, "Not a PE file.\n");
        goto quit;
    }

    fseek(input, 0x3C, SEEK_SET);
    if (fread(&e_lfanew, 1, 4, input) != 4)
    {
        fprintf(stderr, "Could not read e_lfanew.\n");
        goto quit;
    }

    fseek(input, e_lfanew, SEEK_SET);
    if (fread(&signature, 1, 4, input) != 4)
    {
        fprintf(stderr, "Could not read signature.\n");
        goto quit;
    }

    /* Verify the PE signature */
    if (signature != 0x4550)
    {
        fprintf(stderr, "Invalid signature: 0x%x.\n", signature);
        goto quit;
    }

    /* Read Machine */
    fseek(input, e_lfanew + 4, SEEK_SET);
    if (fread(&Machine, 1, 2, input) != 2)
    {
        fprintf(stderr, "Could not read ExportDirectoryRVA.\n");
        goto quit;
    }

    if ((Machine != IMAGE_FILE_MACHINE_I386) &&
        (Machine != IMAGE_FILE_MACHINE_AMD64) &&
        (Machine != IMAGE_FILE_MACHINE_ARMNT) &&
        (Machine != IMAGE_FILE_MACHINE_ARM64))
    {
        fprintf(stderr, "Invalid Machine: 0x%x.\n", Machine);
        goto quit;
    }

    /* Read NumberOfSections */
    if (fread(&NumberOfSections, 1, 2, input) != 2)
    {
        fprintf(stderr, "Could not read NumberOfSections.\n");
        goto quit;
    }

    fseek(input, e_lfanew + 0x14, SEEK_SET);
    if (fread(&SizeOfOptionalHeader, 1, 2, input) != 2)
    {
        fprintf(stderr, "Could not read SizeOfOptionalHeader.\n");
        goto quit;
    }

    /* Read the section table */
    fseek(input, e_lfanew + 0x18 + SizeOfOptionalHeader, SEEK_SET);

    /* Search for the .asmdef section */
    for (i = 0; i < NumberOfSections; i++)
    {
        if (fread(&section, 1, sizeof(SECTION), input) !=  sizeof(SECTION))
        {
            fprintf(stderr, "Could not read section.\n");
            goto quit;
        }

        if (strcmp(section.Name, ".asmdef") == 0)
        {
            break;
        }
    }

    if (i == NumberOfSections)
    {
        fprintf(stderr, "Could not find section.\n");
        goto quit;
    }

    /* Read the section table */
    fseek(input, section.RawAddress, SEEK_SET);

    while (1)
    {
        /* Read one entry */
        if (fread(&data, 1, sizeof(data), input) != sizeof(data))
        {
            fprintf(stderr, "Error reading input file.\n");
            goto quit;
        }

        switch(data.Type)
        {
            case TYPE_END:
                break;

            case TYPE_RAW:
                fprintf(output, "%s\n", data.Name);
                continue;

            case TYPE_CONSTANT:
                if (ms_format)
                {
                    if (Machine == IMAGE_FILE_MACHINE_ARMNT)
                    {
                        fprintf(output, "%s equ 0x%"PRIx64"\n", data.Name, data.Value);
                    }
                    else
                    {
                        fprintf(output, "%s equ 0%"PRIx64"h\n", data.Name, data.Value);
                    }
                }
                else
                {
                    fprintf(output, "%s = 0x%"PRIx64"\n", data.Name, data.Value);
                }
                continue;

            case TYPE_HEADER:
                if (ms_format)
                {
                    fprintf(output, "\n; %s\n", data.Name);
                }
                else
                {
                    fprintf(output, "\n/* %s */\n", data.Name);
                }
                continue;
        }

        break;
    }

    result = 0;

quit:
    /* Close files */
    fclose(input);
    fclose(output);

    return result;
}
