/*
 * COPYRIGHT:   See COPYING in the top level directory
 * PROJECT:     Test program for cabinet classes
 * FILE:        apps/cabman/test.cpp
 * PURPOSE:     Test program
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
#include "test.h"


#ifdef DBG

DWORD DebugTraceLevel = MIN_TRACE;
//DWORD DebugTraceLevel = MID_TRACE;
//DWORD DebugTraceLevel = MAX_TRACE;

#endif /* DBG */


/* CCABManager */

CCABTest::CCABTest()
/*
 * FUNCTION: Default constructor
 */
{
    PromptOnOverwrite = FALSE;
}


CCABTest::~CCABTest()
/*
 * FUNCTION: Default destructor
 */
{
}


VOID CCABTest::ExtractFromCabinet()
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
    } else {
        printf("Cannot not open file: %s.\n", GetCabinetName());
	}
}


/* Event handlers */

BOOL CCABTest::OnOverwrite(PCFFILE File,
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


VOID CCABTest::OnExtract(PCFFILE File,
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



VOID CCABTest::OnDiskChange(LPTSTR CabinetName,
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


INT main(INT argc, PCHAR argv[])
/*
 * FUNCTION: Main entry point
 * ARGUMENTS:
 *     argc = Number of arguments on command line
 *     argv = Pointer to list of command line arguments
 */
{
    CCABTest CABTest;

    // Specify your cabinet filename here
    CABTest.SetCabinetName("ros1.cab");
    CABTest.ExtractFromCabinet();

    return 0;
}

/* EOF */
