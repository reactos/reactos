/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     ReactOS cabinet manager
 * FILE:        apps/cabman/main.cpp
 * PURPOSE:     Main program
 * PROGRAMMERS: Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISIONS:
 *   CSH 21/03-2001 Created
 */
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <conio.h>
#include <windows.h>
#include <reactos/buildno.h>
#include "cabman.h"


#ifdef DBG

DWORD DebugTraceLevel = MIN_TRACE;
//DWORD DebugTraceLevel = MID_TRACE;
//DWORD DebugTraceLevel = MAX_TRACE;

#endif /* DBG */


#define CM_VERSION  KERNEL_VERSION_STR


LPTSTR Pad(LPTSTR Str, CHAR PadChar, UINT Length)
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
    UINT Len;

    Len = lstrlen(Str);

    if (Len < Length) {
        memcpy(&Str[Length - Len], Str, Len + 1);
        memset(Str, PadChar, Length - Len);
    }
    return Str;
}


LPTSTR Date2Str(LPTSTR Str, WORD Date)
/*
 * FUNCTION: Converts a DOS style date to a string
 * ARGUMENTS:
 *     Str  = Pointer to destination string
 *     Date = DOS style date
 * RETURNS:
 *     Pointer to string
 */
{
    DWORD dw;

    /* Month */
    Str[0] = (CHAR)('0' + ((Date & 0x01E0) >> 5) / 10);
    Str[1] = (CHAR)('0' + ((Date & 0x01E0) >> 5) % 10);
    Str[2] = '-';
    /* Day */
    Str[3] = (CHAR)('0' + (Date & 0x001F) / 10);
    Str[4] = (CHAR)('0' + (Date & 0x001F) % 10);
    Str[5] = '-';
    /* Year */
    dw = 1980 + ((Date & 0xFE00) >> 9);
    Str[6] = (CHAR)('0' + dw / 1000); dw %= 1000;
    Str[7] = (CHAR)('0' + dw / 100);  dw %= 100;
    Str[8] = (CHAR)('0' + dw / 10);   dw %= 10;
    Str[9] = (CHAR)('0' + dw % 10);
    Str[10] = '\0';
    return Str;
}


LPTSTR Time2Str(LPTSTR Str, WORD Time)
/*
 * FUNCTION: Converts a DOS style time to a string
 * ARGUMENTS:
 *     Str  = Pointer to destination string
 *     Time = DOS style time
 * RETURNS:
 *     Pointer to string
 */
{
    BOOL PM;
    DWORD Hour;
    DWORD dw;

    Hour = ((Time & 0xF800) >> 11);
    PM = (Hour >= 12);
    Hour %= 12;
    if (Hour == 0)
        Hour = 12;

    if (Hour >= 10)
        Str[0] = (CHAR)('0' + Hour / 10);
    else Str[0] = ' ';
    Str[1] = (CHAR)('0' + Hour % 10);
    Str[2] = ':';
    /* Minute */
    Str[3] = (CHAR)('0' + ((Time & 0x07E0) >> 5) / 10);
    Str[4] = (CHAR)('0' + ((Time & 0x07E0) >> 5) % 10);
    Str[5] = ':';
    /* Second */
    dw = 2 * (Time & 0x001F);
    Str[6] = (CHAR)('0' + dw / 10);
    Str[7] = (CHAR)('0' + dw % 10);

    Str[8] = PM? 'p' : 'a';
    Str[9] = '\0';
    return Str;
}


LPTSTR Attr2Str(LPTSTR Str, WORD Attr)
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
    ProcessAll        = FALSE;
    Mode              = CM_MODE_DISPLAY;
    PromptOnOverwrite = TRUE;
}


CCABManager::~CCABManager()
/*
 * FUNCTION: Default destructor
 */
{
}


VOID CCABManager::Usage()
/*
 * FUNCTION: Display usage information on screen
 */
{
    printf("ReactOS Cabinet Manager - Version %s\n\n", CM_VERSION);
    printf("CABMAN [/D | /E] [/A] [/L dir] [/Y] cabinet [filename ...]\n");
    printf("CABMAN /C dirfile\n");
    printf("  cabinet   Cabinet file.\n");
    printf("  filename  Name of the file to extract from the cabinet.\n");
    printf("            Wild cards and multiple filenames\n");
    printf("            (separated by blanks) may be used.\n\n");
    printf("  dirfile   Name of the directive file to use.\n\n");
    

    printf("  /A        Process ALL cabinets. Follows cabinet chain\n");
    printf("            starting in first cabinet mentioned.\n");
    printf("  /C        Create cabinet.\n");
    printf("  /D        Display cabinet directory.\n");
    printf("  /E        Extract files from cabinet.\n");
    printf("  /L dir    Location to place extracted files\n");
    printf("            (default is current directory).\n");
    printf("  /Y        Do not prompt before overwriting an existing file.\n\n");
}


BOOL CCABManager::ParseCmdline(INT argc, PCHAR argv[])
/*
 * FUNCTION: Parse command line arguments
 * ARGUMENTS:
 *     argc = Number of arguments on command line
 *     argv = Pointer to list of command line arguments
 * RETURNS:
 *    TRUE if command line arguments was successfully parsed, false if not
 */
{
    INT i;
	BOOL ShowUsage;
    BOOL FoundCabinet = FALSE;
	
    ShowUsage = (argc < 2);

	for (i = 1; i < argc; i++) {
		if (argv[i][0] == '/') {
			switch (argv[i][1]) {
            case 'a':
            case 'A': ProcessAll = TRUE; break;
            case 'c':
            case 'C': Mode = CM_MODE_CREATE; break;
            case 'd':
            case 'D': Mode = CM_MODE_DISPLAY; break;
            case 'e':
			case 'E': Mode = CM_MODE_EXTRACT; break;
            case 'l':
			case 'L':
                if (argv[i][2] == ' ') {
                    i++;
                    SetDestinationPath((LPTSTR)&argv[i][0]);
                } else
                    SetDestinationPath((LPTSTR)&argv[i][1]);
                break;
            case 'y':
			case 'Y': PromptOnOverwrite = FALSE; break;
			default:
                printf("Bad parameter %s.\n", argv[i]);
                return FALSE;
			}
		} else {
			if ((FoundCabinet) || (Mode == CM_MODE_CREATE)) {
                /* FIXME: There may be many of these if Mode != CM_MODE_CREATE */
                lstrcpy((LPTSTR)FileName, argv[i]);
            } else {
                SetCabinetName(argv[i]);
                FoundCabinet = TRUE;
            }
		}
    }

	if (ShowUsage) {
		Usage();
		return FALSE;
	}

    /* FIXME */
    SelectCodec(CAB_CODEC_MSZIP);

	return TRUE;
}


VOID CCABManager::CreateCabinet()
/*
 * FUNCTION: Create cabinet
 */
{
    ULONG Status;

    Status = Load((LPTSTR)&FileName);
    if (Status != CAB_STATUS_SUCCESS) {
        printf("Specified directive file could not be found: %s.\n", (LPTSTR)&FileName);
        return;
    }

    Parse();
}


VOID CCABManager::DisplayCabinet()
/*
 * FUNCTION: Display cabinet contents
 */
{
    CAB_SEARCH Search;
    TCHAR Str[20];
    DWORD FileCount = 0;
    DWORD ByteCount = 0;

    if (Open() == CAB_STATUS_SUCCESS) {
        printf("Cabinet %s\n\n", GetCabinetName());

        if (FindFirst("", &Search) == CAB_STATUS_SUCCESS) {
            do {
                if (Search.File->FileControlID != CAB_FILE_CONTINUED) {
                    printf("%s ", Date2Str((LPTSTR)&Str, Search.File->FileDate));
                    printf("%s ", Time2Str((LPTSTR)&Str, Search.File->FileTime));
                    printf("%s ", Attr2Str((LPTSTR)&Str, Search.File->Attributes));
                    printf("%s ", Pad(itoa(Search.File->FileSize, (LPTSTR)&Str, 10), ' ', 13));
                    printf("%s\n", Search.FileName);

                    FileCount++;
                    ByteCount += Search.File->FileSize;
                }
            } while (FindNext(&Search) == CAB_STATUS_SUCCESS);
        }

        if (FileCount > 0) {
            if (FileCount == 1)
                printf("                 1 file    ");
            else
                printf("      %s files   ", 
                    Pad(itoa(FileCount, (LPTSTR)&Str, 10), ' ', 12));

            if (ByteCount == 1)
                printf("           1 byte\n");
            else
                printf("%s bytes\n",
                    Pad(itoa(ByteCount, (LPTSTR)&Str, 10), ' ', 12));
        } else {
            /* There should be at least one file in a cabinet */
            printf("No files in cabinet.");
        }
    } else
        printf("Cannot not open file: %s\n", GetCabinetName());
}


VOID CCABManager::ExtractFromCabinet()
/*
 * FUNCTION: Extract file(s) from cabinet
 */
{
    CAB_SEARCH Search;
    ULONG Status;

    if (Open() == CAB_STATUS_SUCCESS) {
        printf("Cabinet %s\n\n", GetCabinetName());

        if (FindFirst("", &Search) == CAB_STATUS_SUCCESS) {
            do {
                switch (Status = ExtractFile(Search.FileName)) {
                    case CAB_STATUS_SUCCESS:
                        break;
                    case CAB_STATUS_INVALID_CAB:
                        printf("Cabinet contains errors.\n");
                        return;
                    case CAB_STATUS_UNSUPPCOMP:
                        printf("Cabinet uses unsupported compression type.\n");
                        return;
                    case CAB_STATUS_CANNOT_WRITE:
                        printf("You've run out of free space on the destination volume or the volume is damaged.\n");
                        return;
                    default:
                        printf("Unspecified error code (%d).\n", (UINT)Status);
                        return;
                }
            } while (FindNext(&Search) == CAB_STATUS_SUCCESS);
        }
    } else
        printf("Cannot not open file: %s.\n", GetCabinetName());
}


VOID CCABManager::Run()
/*
 * FUNCTION: Process cabinet
 */
{
    printf("ReactOS Cabinet Manager - Version %s\n\n", CM_VERSION);

    switch (Mode) {
    case CM_MODE_CREATE:  CreateCabinet(); break;
    case CM_MODE_DISPLAY: DisplayCabinet(); break;
    case CM_MODE_EXTRACT: ExtractFromCabinet(); break;
    default:
        break;
    }
    printf("\n");
}


/* Event handlers */

BOOL CCABManager::OnOverwrite(PCFFILE File,
                              LPTSTR FileName)
/*
 * FUNCTION: Called when extracting a file and it already exists
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being extracted
 *     Filename = Pointer to buffer with name of file (full path)
 * RETURNS
 *     TRUE if the file should be overwritten, FALSE if not
 */
{
    TCHAR ch;

    if (Mode == CM_MODE_CREATE)
        return TRUE;

    /* Should we prompt on overwrite? */
    if (!PromptOnOverwrite)
        return TRUE;

    /* Ask if file should be overwritten */
    printf("Overwrite %s (Yes/No/All)? ", GetFileName(FileName));
    
    for (;;) {
        ch = _getch();
        switch (ch) {
            case 'Y':
            case 'y': printf("%c\n", ch); return TRUE;
            case 'N':
            case 'n': printf("%c\n", ch); return FALSE;
            case 'A':
            case 'a': printf("%c\n", ch); PromptOnOverwrite = FALSE; return TRUE;
        }
    }
}


VOID CCABManager::OnExtract(PCFFILE File,
                            LPTSTR FileName)
/*
 * FUNCTION: Called just before extracting a file
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being extracted
 *     FileName = Pointer to buffer with name of file (full path)
 */
{
    printf("Extracting %s\n", GetFileName(FileName));
}



VOID CCABManager::OnDiskChange(LPTSTR CabinetName,
                               LPTSTR DiskLabel)
/*
 * FUNCTION: Called when a new disk is to be processed
 * ARGUMENTS:
 *     CabinetName = Pointer to buffer with name of cabinet
 *     DiskLabel   = Pointer to buffer with label of disk
 */
{
    printf("\nChanging to cabinet %s - %s\n\n", CabinetName, DiskLabel);
}


VOID CCABManager::OnAdd(PCFFILE File,
                        LPTSTR FileName)
/*
 * FUNCTION: Called just before adding a file to a cabinet
 * ARGUMENTS:
 *     File     = Pointer to CFFILE for file being added
 *     FileName = Pointer to buffer with name of file (full path)
 */
{
    printf("Adding %s\n", GetFileName(FileName));
}


INT main(INT argc, PCHAR argv[])
/*
 * FUNCTION: Main entry point
 * ARGUMENTS:
 *     argc = Number of arguments on command line
 *     argv = Pointer to list of command line arguments
 */
{
    CCABManager CABMgr;

    if (CABMgr.ParseCmdline(argc, argv))
        CABMgr.Run();

    return 0;
}

/* EOF */
