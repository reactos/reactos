/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        tools/cabman/cabman.cxx
 * PURPOSE:     Main program
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 *              Colin Finck <mail@colinfinck.de>
 * REVISIONS:
 *   CSH 21/03-2001 Created
 *   CSH 15/08-2003 Made it portable
 *   CF  04/05-2007 Made it compatible with 64-bit operating systems
 */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include "cabman.h"


#if DBG

ULONG DebugTraceLevel = MIN_TRACE;
//ULONG DebugTraceLevel = MID_TRACE;
//ULONG DebugTraceLevel = MAX_TRACE;

#endif /* DBG */


char* Pad(char* Str, char PadChar, ULONG Length)
/*
 * FUNCTION: Pads a string with a character to make a given length
 * ARGUMENTS:
 *     Str     = Pointer to string to pad
 *     PadChar = Character to pad with
 *     Length  = Disired length of string
 * RETURNS:
 *     Pointer to string
 * NOTES:
 *     Str must be at least Length + 1 bytes
 */
{
    ULONG Len;

    Len = (ULONG)strlen(Str);

    if (Len < Length)
    {
        memcpy(&Str[Length - Len], Str, Len + 1);
        memset(Str, PadChar, Length - Len);
    }
    return Str;
}


char* Date2Str(char* Str, USHORT Date)
/*
 * FUNCTION: Converts a DOS style date to a string
 * ARGUMENTS:
 *     Str  = Pointer to destination string
 *     Date = DOS style date
 * RETURNS:
 *     Pointer to string
 */
{
    ULONG dw;

    /* Month */
    Str[0] = (char)('0' + ((Date & 0x01E0) >> 5) / 10);
    Str[1] = (char)('0' + ((Date & 0x01E0) >> 5) % 10);
    Str[2] = '-';
    /* Day */
    Str[3] = (char)('0' + (Date & 0x001F) / 10);
    Str[4] = (char)('0' + (Date & 0x001F) % 10);
    Str[5] = '-';
    /* Year */
    dw = 1980 + ((Date & 0xFE00) >> 9);
    Str[6] = (char)('0' + dw / 1000); dw %= 1000;
    Str[7] = (char)('0' + dw / 100);  dw %= 100;
    Str[8] = (char)('0' + dw / 10);   dw %= 10;
    Str[9] = (char)('0' + dw % 10);
    Str[10] = '\0';
    return Str;
}


char* Time2Str(char* Str, USHORT Time)
/*
 * FUNCTION: Converts a DOS style time to a string
 * ARGUMENTS:
 *     Str  = Pointer to destination string
 *     Time = DOS style time
 * RETURNS:
 *     Pointer to string
 */
{
    bool PM;
    ULONG Hour;
    ULONG dw;

    Hour = ((Time & 0xF800) >> 11);
    PM = (Hour >= 12);
    Hour %= 12;
    if (Hour == 0)
        Hour = 12;

    if (Hour >= 10)
        Str[0] = (char)('0' + Hour / 10);
    else Str[0] = ' ';
    Str[1] = (char)('0' + Hour % 10);
    Str[2] = ':';
    /* Minute */
    Str[3] = (char)('0' + ((Time & 0x07E0) >> 5) / 10);
    Str[4] = (char)('0' + ((Time & 0x07E0) >> 5) % 10);
    Str[5] = ':';
    /* Second */
    dw = 2 * (Time & 0x001F);
    Str[6] = (char)('0' + dw / 10);
    Str[7] = (char)('0' + dw % 10);

    Str[8] = PM? 'p' : 'a';
    Str[9] = '\0';
    return Str;
}


char* Attr2Str(char* Str, USHORT Attr)
/*
 * FUNCTION: Converts attributes to a string
 * ARGUMENTS:
 *     Str  = Pointer to destination string
 *     Attr = Attributes
 * RETURNS:
 *     Pointer to string
 */
{
    /* Archive */
    if (Attr & CAB_ATTRIB_ARCHIVE)
        Str[0] = 'A';
    else
        Str[0] = '-';

    /* Hidden */
    if (Attr & CAB_ATTRIB_HIDDEN)
        Str[1] = 'H';
    else
        Str[1] = '-';

    /* Read only */
    if (Attr & CAB_ATTRIB_READONLY)
        Str[2] = 'R';
    else
        Str[2] = '-';

    /* System */
    if (Attr & CAB_ATTRIB_SYSTEM)
        Str[3] = 'S';
    else
        Str[3] = '-';

    Str[4] = '\0';
    return Str;
}


/* CCABManager */

CCABManager::CCABManager()
/*
 * FUNCTION: Default constructor
 */
{
    ProcessAll = false;
    InfFileOnly = false;
    Mode = CM_MODE_DISPLAY;
    FileName[0] = 0;
    Verbose = false;
}


CCABManager::~CCABManager()
/*
 * FUNCTION: Default destructor
 */
{
}


void CCABManager::Usage()
/*
 * FUNCTION: Display usage information on screen
 */
{
    printf("ReactOS Cabinet Manager\n\n");
    printf("CABMAN [-D | -E] [-A] [-L dir] cabinet [filename ...]\n");
    printf("CABMAN [-M mode] -C dirfile [-I] [-RC file] [-P dir]\n");
    printf("CABMAN [-M mode] -S cabinet filename [-F folder] [filename] [...]\n");
    printf("  cabinet   Cabinet file.\n");
    printf("  filename  Name of the file to add to or extract from the cabinet.\n");
    printf("            Wild cards and multiple filenames\n");
    printf("            (separated by blanks) may be used.\n\n");

    printf("  dirfile   Name of the directive file to use.\n");

    printf("  -A        Process ALL cabinets. Follows cabinet chain\n");
    printf("            starting in first cabinet mentioned.\n");
    printf("  -C        Create cabinet.\n");
    printf("  -D        Display cabinet directory.\n");
    printf("  -E        Extract files from cabinet.\n");
    printf("  -F        Put the files from the next 'filename' filter in the cab in folder\filename.\n");
    printf("  -I        Don't create the cabinet, only the .inf file.\n");
    printf("  -L dir    Location to place extracted or generated files\n");
    printf("            (default is current directory).\n");
    printf("  -M mode   Specify the compression method to use:\n");
    printf("               raw    - No compression\n");
    printf("               mszip  - MsZip compression (default)\n");
    printf("  -N        Don't create the .inf file, only the cabinet.\n");
    printf("  -RC       Specify file to put in cabinet reserved area\n");
    printf("            (size must be less than 64KB).\n");
    printf("  -S        Create simple cabinet.\n");
    printf("  -P dir    Files in the .dff are relative to this directory.\n");
    printf("  -V        Verbose mode (prints more messages).\n");
}

bool CCABManager::ParseCmdline(int argc, char* argv[])
/*
 * FUNCTION: Parse command line arguments
 * ARGUMENTS:
 *     argc = Number of arguments on command line
 *     argv = Pointer to list of command line arguments
 * RETURNS:
 *    true if command line arguments was successfully parsed, false if not
 */
{
    int i;
    bool ShowUsage;
    bool FoundCabinet = false;
    std::string NextFolder;
    ShowUsage = (argc < 2);

    for (i = 1; i < argc; i++)
    {
        if (argv[i][0] == '-')
        {
            switch (argv[i][1])
            {
                case 'a':
                case 'A':
                    ProcessAll = true;
                    break;

                case 'c':
                case 'C':
                    Mode = CM_MODE_CREATE;
                    break;

                case 'd':
                case 'D':
                    Mode = CM_MODE_DISPLAY;
                    break;

                case 'e':
                case 'E':
                    Mode = CM_MODE_EXTRACT;
                    break;

                case 'f':
                case 'F':
                    if (argv[i][2] == 0)
                    {
                        i++;
                        NextFolder = argv[i];
                    }
                    else
                    {
                        NextFolder = argv[i] + 2;
                    }
                    break;

                case 'i':
                case 'I':
                    InfFileOnly = true;
                    break;

                case 'l':
                case 'L':
                    if (argv[i][2] == 0)
                    {
                        i++;
                        SetDestinationPath(&argv[i][0]);
                    }
                    else
                        SetDestinationPath(&argv[i][2]);

                    break;

                case 'm':
                case 'M':
                    // Set the compression codec (only affects compression, not decompression)
                    if(argv[i][2] == 0)
                    {
                        i++;

                        if( !SetCompressionCodec(&argv[i][0]) )
                            return false;
                    }
                    else
                    {
                        if( !SetCompressionCodec(&argv[i][2]) )
                            return false;
                    }

                    break;

                case 'n':
                case 'N':
                    DontGenerateInf = true;
                    break;

                case 'R':
                    switch (argv[i][2])
                    {
                        case 'C': /* File to put in cabinet reserved area */
                            if (argv[i][3] == 0)
                            {
                                i++;
                                if (!SetCabinetReservedFile(&argv[i][0]))
                                {
                                    printf("ERROR: Cannot open cabinet reserved area file.\n");
                                    return false;
                                }
                            }
                            else
                            {
                                if (!SetCabinetReservedFile(&argv[i][3]))
                                {
                                    printf("ERROR: Cannot open cabinet reserved area file.\n");
                                    return false;
                                }
                            }
                            break;

                        default:
                            printf("ERROR: Bad parameter %s.\n", argv[i]);
                            return false;
                    }
                    break;

                case 's':
                case 'S':
                    Mode = CM_MODE_CREATE_SIMPLE;
                    break;

                case 'P':
                    if (argv[i][2] == 0)
                    {
                        i++;
                        SetFileRelativePath(&argv[i][0]);
                    }
                    else
                        SetFileRelativePath(&argv[i][2]);

                    break;

                case 'V':
                    Verbose = true;
                    break;

                default:
                    printf("ERROR: Bad parameter %s.\n", argv[i]);
                    return false;
            }
        }
        else
        {
            if(Mode == CM_MODE_CREATE)
            {
                if(FileName[0])
                {
                    printf("ERROR: You may only specify one directive file!\n");
                    return false;
                }
                else
                {
                    // For creating cabinets, this argument is the path to the directive file
                    strcpy(FileName, argv[i]);
                }
            }
            else if(FoundCabinet)
            {
                // For creating simple cabinets, displaying or extracting them, add the argument as a search criteria
                AddSearchCriteria(argv[i], NextFolder);
                NextFolder.clear();
            }
            else
            {
                SetCabinetName(argv[i]);
                FoundCabinet = true;
            }
        }
    }

    if (ShowUsage)
    {
        Usage();
        return false;
    }

    // Select MsZip by default for creating cabinets
    if( (Mode == CM_MODE_CREATE || Mode == CM_MODE_CREATE_SIMPLE) && !IsCodecSelected() )
        SelectCodec(CAB_CODEC_MSZIP);

    // Search criteria (= the filename argument) is necessary for creating a simple cabinet
    if( Mode == CM_MODE_CREATE_SIMPLE && !HasSearchCriteria())
    {
        printf("ERROR: You have to enter input file names!\n");
        return false;
    }

    return true;
}


bool CCABManager::CreateCabinet()
/*
 * FUNCTION: Create cabinet
 */
{
    ULONG Status;

    Status = Load(FileName);
    if (Status != CAB_STATUS_SUCCESS)
    {
        printf("ERROR: Specified directive file could not be found: %s.\n", FileName);
        return false;
    }

    Status = Parse();

    return (Status == CAB_STATUS_SUCCESS ? true : false);
}

bool CCABManager::DisplayCabinet()
/*
 * FUNCTION: Display cabinet contents
 */
{
    CAB_SEARCH Search;
    char Str[20];
    ULONG FileCount = 0;
    ULONG ByteCount = 0;

    if (Open() == CAB_STATUS_SUCCESS)
    {
        if (Verbose)
        {
            printf("Cabinet %s\n\n", GetCabinetName());
        }

        if (FindFirst(&Search) == CAB_STATUS_SUCCESS)
        {
            do
            {
                if (Search.File->FileControlID != CAB_FILE_CONTINUED)
                {
                    printf("%s ", Date2Str(Str, Search.File->FileDate));
                    printf("%s ", Time2Str(Str, Search.File->FileTime));
                    printf("%s ", Attr2Str(Str, Search.File->Attributes));
                    sprintf(Str, "%u", (UINT)Search.File->FileSize);
                    printf("%s ", Pad(Str, ' ', 13));
                    printf("%s\n", Search.FileName.c_str());

                    FileCount++;
                    ByteCount += Search.File->FileSize;
                }
            } while (FindNext(&Search) == CAB_STATUS_SUCCESS);
        }

        DestroySearchCriteria();

        if (FileCount > 0) {
            if (FileCount == 1)
                printf("                 1 file    ");
            else
            {
                sprintf(Str, "%u", (UINT)FileCount);
                printf("      %s files   ", Pad(Str, ' ', 12));
            }

            if (ByteCount == 1)
                printf("           1 byte\n");
            else
            {
                sprintf(Str, "%u", (UINT)ByteCount);
                printf("%s bytes\n", Pad(Str, ' ', 12));
            }
        }
        else
        {
            /* There should be at least one file in a cabinet */
            printf("WARNING: No files in cabinet.");
        }
        return true;
    }
    else
        printf("ERROR: Cannot open file: %s\n", GetCabinetName());

    return false;
}


bool CCABManager::ExtractFromCabinet()
/*
 * FUNCTION: Extract file(s) from cabinet
 */
{
    bool bRet = true;
    CAB_SEARCH Search;
    ULONG Status;

    if (Open() == CAB_STATUS_SUCCESS)
    {
        if (Verbose)
        {
            printf("Cabinet %s\n\n", GetCabinetName());
        }

        if (FindFirst(&Search) == CAB_STATUS_SUCCESS)
        {
            do
            {
                switch (Status = ExtractFile(Search.FileName.c_str()))
                {
                    case CAB_STATUS_SUCCESS:
                        break;

                    case CAB_STATUS_INVALID_CAB:
                        printf("ERROR: Cabinet contains errors.\n");
                        bRet = false;
                        break;

                    case CAB_STATUS_UNSUPPCOMP:
                        printf("ERROR: Cabinet uses unsupported compression type.\n");
                        bRet = false;
                        break;

                    case CAB_STATUS_CANNOT_WRITE:
                        printf("ERROR: You've run out of free space on the destination volume or the volume is damaged.\n");
                        bRet = false;
                        break;

                    default:
                        printf("ERROR: Unspecified error code (%u).\n", (UINT)Status);
                        bRet = false;
                        break;
                }

                if(!bRet)
                    break;
            } while (FindNext(&Search) == CAB_STATUS_SUCCESS);

            DestroySearchCriteria();
        }

        return bRet;
    }
    else
        printf("ERROR: Cannot open file: %s.\n", GetCabinetName());

    return false;
}


bool CCABManager::Run()
/*
 * FUNCTION: Process cabinet
 */
{
    if (Verbose)
    {
        printf("ReactOS Cabinet Manager\n\n");
    }

    switch (Mode)
    {
        case CM_MODE_CREATE:
            return CreateCabinet();

        case CM_MODE_DISPLAY:
            return DisplayCabinet();

        case CM_MODE_EXTRACT:
            return ExtractFromCabinet();

        case CM_MODE_CREATE_SIMPLE:
            return CreateSimpleCabinet();

        default:
            break;
    }
    return false;
}


/* Event handlers */

bool CCABManager::OnOverwrite(PCFFILE File,
                              const char* FileName)
/*
 * FUNCTION: Called when extracting a file and it already exists
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being extracted
 *     Filename = Pointer to buffer with name of file (full path)
 * RETURNS
 *     true if the file should be overwritten, false if not
 */
{
    if (Mode == CM_MODE_CREATE)
        return true;

    /* Always overwrite */
    return true;
}


void CCABManager::OnExtract(PCFFILE File,
                            const char* FileName)
/*
 * FUNCTION: Called just before extracting a file
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being extracted
 *     FileName = Pointer to buffer with name of file (full path)
 */
{
    if (Verbose)
    {
        printf("Extracting %s\n", GetFileName(FileName).c_str());
    }
}



void CCABManager::OnDiskChange(const char* CabinetName,
    const char* DiskLabel)
    /*
     * FUNCTION: Called when a new disk is to be processed
     * ARGUMENTS:
     *     CabinetName = Pointer to buffer with name of cabinet
     *     DiskLabel   = Pointer to buffer with label of disk
     */
{
    if (Verbose)
    {
        printf("\nChanging to cabinet %s - %s\n\n", CabinetName, DiskLabel);
    }
}


void CCABManager::OnAdd(PCFFILE File,
                        const char* FileName)
/*
 * FUNCTION: Called just before adding a file to a cabinet
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being added
 *     FileName = Pointer to buffer with name of file (full path)
 */
{
    if (Verbose)
    {
        printf("Adding %s\n", GetFileName(FileName).c_str());
    }
}

void CCABManager::OnVerboseMessage(const char* Message)
{
    if (Verbose)
    {
        printf("%s", Message);
    }
}

int main(int argc, char * argv[])
/*
 * FUNCTION: Main entry point
 * ARGUMENTS:
 *     argc = Number of arguments on command line
 *     argv = Pointer to list of command line arguments
 */
{
    CCABManager CABMgr;

    if (!CABMgr.ParseCmdline(argc, argv))
        return 2;

    return CABMgr.Run() ? 0 : 1;
}

/* EOF */
