/*++


Copyright (c) 1992  Microsoft Corporation

Module Name:

    symedit.c

Abstract:


Author:

    Wesley A. Witt (wesw) 19-April-1993

Environment:

    Win32, User Mode

--*/

#include <windows.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "symcvt.h"
#include "cv.h"
#include "cofftocv.h"
#include "symtocv.h"
#include "strings.h"

#include <dbghelp.h>


/*
 *      prototypes for this module
 */

BOOL    CalculateOutputFilePointers( PIMAGEPOINTERS pi, PIMAGEPOINTERS po );
void    ProcessCommandLineArgs( int argc, char *argv[] );
void    PrintCopyright( void );
void    PrintUsage( void );
void    FatalError( int, ... );
BOOL    MapOutputFile ( PPOINTERS p, char *fname, int );
void    ComputeChecksum(  char *szExeFile );
void    ReadDebugInfo( PPOINTERS p );
void    WriteDebugInfo( PPOINTERS p, BOOL, BOOL );
void    MungeDebugHeadersCoffToCv( PPOINTERS  p, BOOL fAddCV );
void    MungeExeName( PPOINTERS p, char * szExeName );
void    DoCoffToCv(char *, char *, BOOL);
void    DoSymToCv(char *, char *, char *, char *);
void    DoNameChange(char *, char *, char *);
void    DoExtract(char *);

IMAGE_DEBUG_DIRECTORY   DbgDirSpare;
IMAGE_DEBUG_DIRECTORY   DbgDirCoff;

#define AdjustPtr(ptr) (((ptr) != NULL) ? ((DWORD)ptr - (DWORD)pi->fptr + (DWORD)po->fptr) : ((DWORD)(ptr)))



int
WINAPIV
main(
     int        argc,
     char *     argv[]
     )
/*++

Routine Description:

    Shell for this utility.

Arguments:

    argc     - argument count
    argv     - argument pointers


Return Value:

    0        - image was converted
    >0       - image could not be converted

--*/

{
    /*
     *  Scan the command line and check what operations we are doing
     */

    ProcessCommandLineArgs( argc, argv );
    return 0;
}

void
PrintCopyright( void )

/*++

Routine Description:

    Prints the MS copyright message to stdout.

Arguments:

    void


Return Value:

    void

--*/

{
    puts( "\nMicrosoft(R) Windows NT SymEdit Version 3.51\n"
          "(C) 1989-1995 Microsoft Corp. All rights reserved.\n");
}

void
PrintUsage( void )

/*++

Routine Description:

    Prints the command line help to stdout

Arguments:

    void


Return Value:

    void

--*/

{
    PrintCopyright();
    puts("\nUsage: SYMEDIT <OPERATION> -q -o<file out> <file in>\n\n"
        "\t<OPERATION> is:\n"
        "\tA\tAdd debug information\n"
        "\tC\tConvert symbol information\n"
        "\tN\tEdit name field\n"
        "\tX\tExtract debug information\n"
        "\tS\tStrip all debug information\n\n"
        "Options:\n"
        "\t-a\t\tAdd CodeView debug info to file\n"
        "\t-n<name>\tName to change to\n"
        "\t-o<file>\tspecify output file\n"
        "\t-q\t\tquiet mode\n"
        "\t-s<file>\tSym file source");
}

void
ProcessCommandLineArgs(
    int argc,
    char *argv[]
    )

/*++

Routine Description:

    Processes the command line arguments and sets global flags to
    indicate the user's desired behavior.

Arguments:

    argc     - argument count
    argv     - argument pointers


Return Value:

    void

--*/

{
    int     i;
    BOOL        fQuiet = FALSE;
    BOOL        fSilent = FALSE;
    char *      szOutputFile = NULL;
    char *      szInputFile = NULL;
    char *      szExeName = NULL;
    char *      szDbgFile = NULL;
    char *      szSymFile = NULL;
    int         iOperation;
    BOOLEAN     fAddCV = FALSE;

    /*
     * Minimun number of of arguments is 2 -- program and operation
     */

    if (argc < 2) {
        PrintUsage();
        exit(1);
    }

    if ((strcmp(argv[1], "-?") == 0) || (strcmp(argv[1], "?") == 0)) {
        PrintUsage();
        exit(1);
    }

    /*
     *  All operations on 1 character wide
     */

    if (argv[1][1] != 0) {
        FatalError(ERR_OP_UNKNOWN, argv[1]);
    }

    /*
     *  Validate the operation
     */

    switch( argv[1][0] ) {
    case 'C':
    case 'N':
    case 'X':
    case 'S':
        iOperation = argv[1][0];
        break;
    default:
        FatalError(ERR_OP_UNKNOWN, argv[1]);
    }

    /*
     *  Parse out any other switches on the command line
     */

    for (i=2; i<argc; i++) {
        if ((argv[i][0] == '-') || (argv[i][0] == '/')) {
            switch (argv[i][1]) {
                /*
                 *   Add the CV debug information section rather than
                 *  replace the COFF section with the CV info.
                 */
            case 'a':
            case 'A':
                fAddCV = TRUE;
                break;

                /*
                 *  Specify the output name for the DBG file
                 */

            case 'd':
            case 'D':
                if (argv[i][2] == 0) {
                    i += 1;
                    szDbgFile = argv[i];
                } else {
                    szDbgFile = &argv[i][2];
                }
                break;

                /*
                 *  Specify a new name to shove into the name of the
                 *  debuggee field in the Misc. Debug info field
                 */

            case 'N':
            case 'n':
                if (argv[i][2] == 0) {
                    i += 1;
                    szExeName = argv[i];
                } else {
                    szExeName = &argv[i][2];
                }
                break;

                /*
                 * Specify the name of the output file
                 */

            case 'O':
            case 'o':
                if (argv[i][2] == 0) {
                    i += 1;
                    szOutputFile = argv[i];
                } else {
                    szOutputFile = &argv[i][2];
                }
                break;

                /*
                 *  Be quite and don't put out the banner
                 */

            case 'Q':
            case 'q':
                fQuiet = TRUE;
                fSilent = TRUE;
                break;

                /*
                 *  Replace COFF debug information with CODEVIEW debug information
                 */

            case 'R':
            case 'r':
                break;

                /*
                 *  Convert a Symbol File to CV info
                 */

            case 'S':
            case 's':
                if (argv[i][2] == 0) {
                    i += 1;
                    szSymFile = argv[i];
                } else {
                    szSymFile = &argv[i][2];
                }
                break;

                /*
                 * Print the command line options
                 */

            case '?':
                PrintUsage();
                exit(1);
                break;

                /*
                 * Unrecognized option
                 */

            default:
                FatalError( ERR_OP_UNKNOWN, argv[i] );
                break;
            }
        } else {
            /*
             *  No leading switch character -- must be a file name
             */

            szInputFile = &argv[i][0];

            /*
             *  Process the file(s)
             */

            if (!fQuiet) {
                PrintCopyright();
                fQuiet = TRUE;
            }

            if (!fSilent) {
                printf("processing file: %s\n", szInputFile );
            }

            /*
             *  Do switch validation cheching and setup any missing global variables
             */

            switch ( iOperation ) {
                /*
                 *  Add debug information to an exe file
                 *
                 *      Must specify in input file
                 *      Optional outputfile
                 *      Must specify a debug info file
                 */

            case 'A':
                break;

                /*
                 * For conversions -- there are three types
                 *
                 *      1.  Coff to CV -- add
                 *      2.  Coff to CV -- replace
                 *      3.  SYM to CV --- add
                 *      4.  SYM to CV -- seperate file
                 *
                 *      Optional input file (not needed for case 4)
                 *      Optional output file
                 *      Optional sym file (implys sym->CV)
                 *      Optional DBG file
                 */

            case 'C':
                if (szSymFile == NULL) {
                    DoCoffToCv(szInputFile, szOutputFile, fAddCV);
                } else {
                    DoSymToCv(szInputFile, szOutputFile, szDbgFile, szSymFile);
                }
                szInputFile = NULL;
                szOutputFile = NULL;
                szDbgFile = NULL;
                szSymFile = NULL;
                break;

                /*
                 *  For changing the name of the debuggee --
                 *      Must specify input file
                 *      Must specify new name
                 *      Optional output file
                 */

            case 'N':
                DoNameChange(szInputFile, szOutputFile, szExeName);
                szInputFile = NULL;
                szOutputFile = NULL;
                szExeName = NULL;
                break;

                /*
                 *  For extraction of debug information
                 *      Must specify input file
                 *      Optional output file name
                 *      Optional debug file name
                 */

            case 'X':
            case 'S':
                DoExtract(szInputFile);
                break;
            }
        }
    }
    return;
}                               /* ProcessCommandLineArgs() */


void
ReadDebugInfo(
    PPOINTERS   p
    )
/*++

Routine Description:

    This function will go out and read in all of the debug information
    into memory -- this is required because the input and output
    files might be the same, if so then writing out informaiton may
    destory data we need at a later time.

Arguments:

    p   - Supplies a pointer to the structure describing the debug info file

Return Value:

    None.

--*/

{
    int                         i;
    int                         cb;
    char *                      pb;
    PIMAGE_COFF_SYMBOLS_HEADER  pCoffDbgInfo;


    /*
     *   Allocate space to save pointers to debug info
     */

    p->iptrs.rgpbDebugSave = (PCHAR *) malloc(p->iptrs.cDebugDir * sizeof(PCHAR));
    memset(p->iptrs.rgpbDebugSave, 0, p->iptrs.cDebugDir * sizeof(PCHAR));

    /*
     * Check each possible debug type record
     */

    for (i=0; i<p->iptrs.cDebugDir; i++) {
        /*
         *   If there was debug information then copy over the
         *      description block and cache in the actual debug
         *      data.
         */

        if ((i != IMAGE_DEBUG_TYPE_COFF) && (p->iptrs.rgDebugDir[i] != NULL)) {
            p->iptrs.rgpbDebugSave[i] =
              malloc( p->iptrs.rgDebugDir[i]->SizeOfData );
            if (p->iptrs.rgpbDebugSave[i] == NULL) {
                FatalError(ERR_NO_MEMORY);
            }
            _try {
                memcpy(p->iptrs.rgpbDebugSave[i],
                       p->iptrs.fptr +
                       p->iptrs.rgDebugDir[i]->PointerToRawData,
                       p->iptrs.rgDebugDir[i]->SizeOfData );
            } _except(EXCEPTION_EXECUTE_HANDLER ) {
                free(p->iptrs.rgpbDebugSave[i]);
                p->iptrs.rgpbDebugSave[i] = NULL;
            }
        }
    }

    /*
     *  Treat COFF debug information seperately -- since the linker loves
     *  to mix it in with other things
     */

    if (p->iptrs.rgDebugDir[IMAGE_DEBUG_TYPE_COFF] != NULL) {
        DbgDirCoff = *COFF_DIR(&p->iptrs);

        /*
         *  Allocate space to save the coff debug info
         */

        pb = p->iptrs.rgpbDebugSave[i] = malloc(DbgDirCoff.SizeOfData);

        /*
         *  Copy over and point to the description for the
         *      debug info.
         */

        memcpy(pb, DbgDirCoff.PointerToRawData + p->iptrs.fptr,
               sizeof(IMAGE_COFF_SYMBOLS_HEADER));
        pCoffDbgInfo = (PIMAGE_COFF_SYMBOLS_HEADER) pb;

        /*
         *  Now figure out how much space we really have -- only things
         *      after the first symbol are "good" -- everything else is
         *      suspect as part of something else.
         */

        cb = DbgDirCoff.SizeOfData - pCoffDbgInfo->LvaToFirstSymbol;

        /*
         *  Now copy over the real symbol information and set up the
         *      new pointers in the header record
         */

        memcpy(pb+sizeof(IMAGE_COFF_SYMBOLS_HEADER),
               p->iptrs.fptr + DbgDirCoff.PointerToRawData +
               pCoffDbgInfo->LvaToFirstSymbol, cb);
        pCoffDbgInfo->LvaToFirstSymbol = sizeof(IMAGE_COFF_SYMBOLS_HEADER);
        DbgDirCoff.SizeOfData = cb + sizeof(IMAGE_COFF_SYMBOLS_HEADER);
        pCoffDbgInfo->NumberOfLinenumbers = 0;
        pCoffDbgInfo->LvaToFirstLinenumber = 0;
    }
    return;
}                               /* ReadDebugInfo() */



void
WriteDebugInfo(
    PPOINTERS   p,
    BOOL        fAddCV,
    BOOL        fStrip
    )
/*++

Routine Description:

    This function will go out and read in all of the debug information
    into memory -- this is required because the input and output
    files might be the same, if so then writing out informaiton may
    destory data we need at a later time.

Arguments:

    p   - Supplies a pointer to the structure describing the debug info file

Return Value:

    None.

--*/

{
    ULONG       PointerToDebugData = 0; /*  Offset from the start of the file
                                         *  to the current location to write
                                         *  debug information out.
                                         */
    ULONG       BaseOfDebugData = 0;
    int                         i;
    PIMAGE_DEBUG_DIRECTORY      pDir;
    int                         flen;
    PIMAGE_DEBUG_DIRECTORY      pDbgDir = NULL;


    if (p->optrs.debugSection) {
        BaseOfDebugData = PointerToDebugData =
          p->optrs.debugSection->PointerToRawData;
    } else if (p->optrs.sepHdr) {
        BaseOfDebugData =  PointerToDebugData =
          sizeof(IMAGE_SEPARATE_DEBUG_HEADER) +
          p->optrs.sepHdr->NumberOfSections * sizeof(IMAGE_SECTION_HEADER) +
          p->optrs.sepHdr->ExportedNamesSize;
    }


    /*
     *  Step 2. If the debug information is mapped, we know this
     *          from the section headers, then we may need to write
     *          out a new debug director to point to the debug information
     */

    if (fAddCV) {
        if (p->optrs.optHdr) {
            p->optrs.optHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].
              VirtualAddress = p->optrs.debugSection->VirtualAddress;
            p->optrs.optHdr->DataDirectory[IMAGE_DIRECTORY_ENTRY_DEBUG].Size +=
              sizeof(IMAGE_DEBUG_DIRECTORY);
        } else if (p->optrs.sepHdr) {
            p->optrs.sepHdr->DebugDirectorySize +=
              sizeof(IMAGE_DEBUG_DIRECTORY);
        } else {
            exit(1);
        }

        if (p->optrs.sepHdr) {
            pDbgDir = (PIMAGE_DEBUG_DIRECTORY) malloc(p->optrs.cDebugDir * sizeof(IMAGE_DEBUG_DIRECTORY));
            for (i=0; i<p->optrs.cDebugDir; i++) {
                if (p->optrs.rgDebugDir[i] != NULL) {
                    pDbgDir[i] = *(p->optrs.rgDebugDir[i]);
                    p->optrs.rgDebugDir[i] = &pDbgDir[i];
                }
            }
        }
        for (i=0; i<p->optrs.cDebugDir; i++) {
            if (p->optrs.rgDebugDir[i]) {
                pDir = (PIMAGE_DEBUG_DIRECTORY) (PointerToDebugData +
                                                 p->optrs.fptr);
                *pDir = *(p->optrs.rgDebugDir[i]);
                p->optrs.rgDebugDir[i] = pDir;
                PointerToDebugData += sizeof(IMAGE_DEBUG_DIRECTORY);
            }
        }
    }

    /*
     *  For every debug info type, write out the debug information
     *  and update any header information required
     */

    for (i=0; i<p->optrs.cDebugDir; i++) {
        if (p->optrs.rgDebugDir[i] != NULL) {
            if (!fStrip ||
                (i == IMAGE_DEBUG_TYPE_FPO) ||
                (i == IMAGE_DEBUG_TYPE_MISC)) {
                if (p->optrs.rgpbDebugSave[i] != NULL) {
                    p->optrs.rgDebugDir[i]->PointerToRawData =
                      PointerToDebugData;
                    if (p->optrs.debugSection) {
                        p->optrs.rgDebugDir[i]->AddressOfRawData =
                          p->optrs.debugSection->VirtualAddress +
                            PointerToDebugData - BaseOfDebugData;
                    }
                    memcpy(p->optrs.fptr + PointerToDebugData,
                           p->optrs.rgpbDebugSave[i],
                           p->optrs.rgDebugDir[i]->SizeOfData);

                    if ((i == IMAGE_DEBUG_TYPE_COFF) &&
                        (p->optrs.fileHdr != NULL)) {
                        p->optrs.fileHdr->PointerToSymbolTable =
                          PointerToDebugData + sizeof(IMAGE_COFF_SYMBOLS_HEADER);
                    }
                }
                PointerToDebugData += p->optrs.rgDebugDir[i]->SizeOfData;
            }
        }
    }

    if ((PointerToDebugData == BaseOfDebugData) && fStrip) {
        p->optrs.fileHdr->NumberOfSections -= 1;
    }

    /*
     *   Step 4.  Clean up any COFF structures if we are replacing
     *          the coff information with CV info.
     */

    if ((p->optrs.rgDebugDir[IMAGE_DEBUG_TYPE_COFF] == NULL) &&
        (p->optrs.fileHdr != NULL)) {
        /*
         *  Since there is no coff debug information -- clean out
         *      both fields pointing to the debug info
         */

        p->optrs.fileHdr->PointerToSymbolTable = 0;
        p->optrs.fileHdr->NumberOfSymbols = 0;
    }

    /*
     *  Step 5.  Correct the alignments if needed.  If there is
     *          a real .debug section in the file (i.e. it is mapped)
     *          then update the description of the section.
     */

    if (p->optrs.debugSection) {
        p->optrs.debugSection->SizeOfRawData =
          FileAlign(PointerToDebugData - BaseOfDebugData);
        /*
         * update the optional header with the new image size
         */

        p->optrs.optHdr->SizeOfImage =
          SectionAlign(p->optrs.debugSection->VirtualAddress +
                       p->optrs.debugSection->SizeOfRawData);
        p->optrs.optHdr->SizeOfInitializedData +=
          p->optrs.debugSection->SizeOfRawData;
    }

    /*
     *  calculate the new file size
     */

    if (p->optrs.optHdr != NULL) {
        flen = FileAlign(PointerToDebugData);
    } else {
        flen = PointerToDebugData;
    }

    /*
     *  finally, update the eof pointer and close the file
     */

    UnmapViewOfFile( p->optrs.fptr );

    if (!SetFilePointer( p->optrs.hFile, flen, 0, FILE_BEGIN )) {
        FatalError( ERR_FILE_PTRS );
    }

    if (!SetEndOfFile( p->optrs.hFile )) {
        FatalError( ERR_SET_EOF );
    }

    CloseHandle( p->optrs.hFile );

    /*
     *  Exit -- we are done.
     */

    return;
}                               /* WriteDebugInfo() */



void
MungeDebugHeadersCoffToCv(
    PPOINTERS   p,
    BOOL        fAddCV
    )

/*++

Routine Description:


Arguments:

    p        - pointer to a POINTERS structure (see cofftocv.h)

Return Value:

    void

--*/

{
    if (!fAddCV) {
        CV_DIR(&p->optrs) = COFF_DIR(&p->optrs);
        COFF_DIR(&p->optrs) = 0;
    } else {
        CV_DIR(&p->optrs) = &DbgDirSpare;
        *(COFF_DIR(&p->optrs)) = *(COFF_DIR(&p->iptrs));
    };

    *CV_DIR(&p->optrs) = *(COFF_DIR(&p->iptrs));
    CV_DIR(&p->optrs)->Type = IMAGE_DEBUG_TYPE_CODEVIEW;
    CV_DIR(&p->optrs)->SizeOfData =  p->pCvStart.size;
    p->optrs.rgpbDebugSave[IMAGE_DEBUG_TYPE_CODEVIEW] = p->pCvStart.ptr;

    return;
}                               /* MungeDebugHeadersCoffToCv() */



BOOL
MapOutputFile (
    PPOINTERS p,
    char *fname,
    int cb
    )

/*++

Routine Description:

    Maps the output file specified by the fname argument and saves the
    file handle & file pointer in the POINTERS structure.


Arguments:

    p        - pointer to a POINTERS structure (see cofftocv.h)
    fname    - ascii string for the file name


Return Value:

    TRUE     - file mapped ok
    FALSE    - file could not be mapped

--*/

{
    BOOL        rval;
    HANDLE      hMap   = NULL;
    DWORD       oSize;

    rval = FALSE;

    p->optrs.hFile = CreateFile( fname,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_ALWAYS,
                        0,
                        NULL );

    if (p->optrs.hFile == INVALID_HANDLE_VALUE) {
       goto exit;
    }

    oSize = p->iptrs.fsize;
    if (p->pCvStart.ptr != NULL) {
        oSize += p->pCvStart.size;
    }
    oSize += cb;
    oSize += p->iptrs.cDebugDir * sizeof(IMAGE_DEBUG_DIRECTORY);

    hMap = CreateFileMapping( p->optrs.hFile, NULL, PAGE_READWRITE,
                              0, oSize, NULL );

    if (hMap == NULL) {
       goto exit;
    }

    p->optrs.fptr = MapViewOfFile( hMap, FILE_MAP_WRITE, 0, 0, 0 );

    if (p->optrs.fptr == NULL) {
       goto exit;
    }
    rval = TRUE;
exit:
    return rval;
}                               /* MapOutputFile() */

BOOL
CalculateOutputFilePointers(
    PIMAGEPOINTERS pi,
    PIMAGEPOINTERS po
    )

/*++

Routine Description:

    This function calculates the output file pointers based on the
    input file pointers.  The same address is used but they are all
    re-based off the output file's file pointer.

Arguments:

    p        - pointer to a IMAGEPOINTERS structure (see cofftocv.h)


Return Value:

    TRUE     - pointers were created
    FALSE    - pointers could not be created

--*/
{
    int i;

    // fixup the pointers relative the fptr for the output file
    po->dosHdr       = (PIMAGE_DOS_HEADER)      AdjustPtr(pi->dosHdr);
    po->ntHdr        = (PIMAGE_NT_HEADERS)      AdjustPtr(pi->ntHdr);
    po->fileHdr      = (PIMAGE_FILE_HEADER)     AdjustPtr(pi->fileHdr);
    po->optHdr       = (PIMAGE_OPTIONAL_HEADER) AdjustPtr(pi->optHdr);
    po->sectionHdrs  = (PIMAGE_SECTION_HEADER)  AdjustPtr(pi->sectionHdrs);
    po->sepHdr       = (PIMAGE_SEPARATE_DEBUG_HEADER) AdjustPtr(pi->sepHdr);
    po->debugSection = (PIMAGE_SECTION_HEADER)  AdjustPtr(pi->debugSection);
    po->AllSymbols   = (PIMAGE_SYMBOL)          AdjustPtr(pi->AllSymbols);
    po->stringTable  = (PUCHAR)                 AdjustPtr(pi->stringTable);

    // move the data from the input file to the output file
    memcpy( po->fptr, pi->fptr, pi->fsize );

    po->cDebugDir = pi->cDebugDir;
    po->rgDebugDir = malloc(po->cDebugDir * sizeof(po->rgDebugDir[0]));
    memset(po->rgDebugDir, 0, po->cDebugDir * sizeof(po->rgDebugDir[0]));

    for (i=0; i<po->cDebugDir; i++) {
        po->rgDebugDir[i] = (PIMAGE_DEBUG_DIRECTORY) AdjustPtr(pi->rgDebugDir[i]);
    }
    po->rgpbDebugSave = pi->rgpbDebugSave;

    /*
     *  Point the input stream to the modified coff descriptor
     */

    if (pi->rgDebugDir[IMAGE_DEBUG_TYPE_COFF] != NULL) {
        pi->rgDebugDir[IMAGE_DEBUG_TYPE_COFF] = &DbgDirCoff;
    }

    return TRUE;
}


void
FatalError(
    int  idMsg,
    ...
    )
/*++

Routine Description:

    Prints a message string to stderr and then exits.

Arguments:

    s        - message string to be printed

Return Value:

    void

--*/

{
    va_list     marker;
    char        rgchFormat[256];
    char        rgch[256];

    LoadString(GetModuleHandle(NULL), idMsg, rgchFormat, sizeof(rgchFormat));

    va_start(marker, idMsg);
    vsprintf(rgch, rgchFormat, marker);
    va_end(marker);

    fprintf(stderr, "%s\n", rgch);

    exit(1);
}                               /* FatalError() */


void
ComputeChecksum(
    char *szExeFile
    )

/*++

Routine Description:

    Computes a new checksum for the image by calling dbghelp.dll

Arguments:

    szExeFile - exe file name


Return Value:

    void

--*/

{
    DWORD              dwHeaderSum = 0;
    DWORD              dwCheckSum = 0;
    HANDLE             hFile;
    DWORD              cb;
    IMAGE_DOS_HEADER   dosHdr;
    IMAGE_NT_HEADERS   ntHdr;

    if (MapFileAndCheckSum(szExeFile, &dwHeaderSum, &dwCheckSum) != CHECKSUM_SUCCESS) {
        FatalError( ERR_CHECKSUM_CALC );
    }

    hFile = CreateFile( szExeFile,
                        GENERIC_READ | GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL
                      );

    // seek to the beginning of the file
    SetFilePointer( hFile, 0, 0, FILE_BEGIN );

    // read in the dos header
    if ((ReadFile(hFile, &dosHdr, sizeof(dosHdr), &cb, 0) == FALSE) || (cb != sizeof(dosHdr))) {
        FatalError( ERR_CHECKSUM_CALC );
    }

    // read in the pe header
    if ((dosHdr.e_magic != IMAGE_DOS_SIGNATURE) ||
        (SetFilePointer(hFile, dosHdr.e_lfanew, 0, FILE_BEGIN) == -1L)) {
        FatalError( ERR_CHECKSUM_CALC );
    }

    // read in the nt header
    if ((!ReadFile(hFile, &ntHdr, sizeof(ntHdr), &cb, 0)) || (cb != sizeof(ntHdr))) {
        FatalError( ERR_CHECKSUM_CALC );
    }

    if (SetFilePointer(hFile, dosHdr.e_lfanew, 0, FILE_BEGIN) == -1L) {
        FatalError( ERR_CHECKSUM_CALC );
    }

    ntHdr.OptionalHeader.CheckSum = dwCheckSum;

    if (!WriteFile(hFile, &ntHdr, sizeof(ntHdr), &cb, NULL)) {
        FatalError( ERR_CHECKSUM_CALC );
    }

    CloseHandle(hFile);
    return;
}


void
MungeExeName(
    PPOINTERS   p,
    char *      szExeName
    )
/*++

Routine Description:

    description-of-function.

Arguments:

    argument-name - Supplies | Returns description of argument.
    .
    .

Return Value:

    None.

--*/

{
    PIMAGE_DEBUG_MISC   pMiscIn;
    PIMAGE_DEBUG_MISC   pMiscOut;
    int                 cb;
    int                 i;

    for (i=0; i<p->iptrs.cDebugDir; i++) {
        if (p->optrs.rgDebugDir[i] != 0) {
            *(p->optrs.rgDebugDir[i]) = *(p->iptrs.rgDebugDir[i]);
        }
    }

    pMiscIn = (PIMAGE_DEBUG_MISC)
      p->iptrs.rgpbDebugSave[IMAGE_DEBUG_TYPE_MISC];

    if (p->optrs.rgDebugDir[IMAGE_DEBUG_TYPE_MISC] == NULL) {
        p->optrs.rgDebugDir[IMAGE_DEBUG_TYPE_MISC] = &DbgDirSpare;
        memset(&DbgDirSpare, 0, sizeof(DbgDirSpare));
    }

    pMiscOut = (PIMAGE_DEBUG_MISC)
      p->optrs.rgpbDebugSave[IMAGE_DEBUG_TYPE_MISC] =
      malloc(p->optrs.rgDebugDir[IMAGE_DEBUG_TYPE_MISC]->SizeOfData +
             strlen(szExeName));
    cb = p->optrs.rgDebugDir[IMAGE_DEBUG_TYPE_MISC]->SizeOfData;

    while ( cb > 0 ) {
        if (pMiscIn->DataType == IMAGE_DEBUG_MISC_EXENAME) {
            pMiscOut->DataType = IMAGE_DEBUG_MISC_EXENAME;
            pMiscOut->Length = (sizeof(IMAGE_DEBUG_MISC) +
                                strlen(szExeName) + 3) & ~3;
            pMiscOut->Unicode = FALSE;
            strcpy(&pMiscOut->Data[0], szExeName);
            szExeName = NULL;
        } else {
            memcpy(pMiscOut, pMiscIn, pMiscIn->Length);
        }

        p->optrs.rgDebugDir[IMAGE_DEBUG_TYPE_MISC]->SizeOfData +=
          (pMiscOut->Length - pMiscIn->Length);

        cb -= pMiscIn->Length;
        pMiscIn = (PIMAGE_DEBUG_MISC) (((char *) pMiscIn) + pMiscIn->Length);
        pMiscOut = (PIMAGE_DEBUG_MISC) (((char *) pMiscOut) + pMiscOut->Length);
    }

    if (szExeName) {
        pMiscOut->DataType = IMAGE_DEBUG_MISC_EXENAME;
        pMiscOut->Length = (sizeof(IMAGE_DEBUG_MISC) +
                            strlen(szExeName) + 3) & ~3;
        pMiscOut->Unicode = FALSE;
        strcpy(&pMiscOut->Data[0], szExeName);
    }

    return;
}                               /* MungeExeName() */


/***    DoCoffToCv
 *
 *
 */

void
DoCoffToCv(
    char * szInput,
    char * szOutput,
    BOOL fAddCV
    )
{
    POINTERS    p;

    /*
     *  Do default checking
     */

    if (szOutput == NULL) {
        szOutput = szInput;
    }

    /*
     *  Open the input file name and setup the pointers into the file
     */

    if (!MapInputFile( &p, NULL, szInput )) {
        FatalError( ERR_OPEN_INPUT_FILE, szInput );
    }

    /*
     *  Now, if we thing we are playing with PE exes then we need
     *  to setup the pointers into the map file
     */

    if (!CalculateNtImagePointers( &p.iptrs )) {
        FatalError( ERR_INVALID_PE, szInput );
    }

    /*
     *  We are about to try and do the coff to cv symbol conversion.
     *
     *  Verify that the operation is legal.
     *
     *      1.  We need to have coff debug information to start with
     *      2.  If the debug info is not mapped then we must not
     *              be trying to add CodeView info.
     */

    if ((p.iptrs.AllSymbols == NULL) || (COFF_DIR(&p.iptrs) == NULL)) {
        FatalError( ERR_NO_COFF );
    }

    if (fAddCV && (p.iptrs.debugSection == 0) && (p.iptrs.sepHdr == NULL)) {
        FatalError( ERR_NOT_MAPPED );
    }

    /*
     *  Now go out an preform the acutal conversion.
     */

    if (!ConvertCoffToCv( &p )) {
        FatalError( ERR_COFF_TO_CV );
    }

    /*
     *  Read in any additional debug information in the file
     */

    ReadDebugInfo(&p);

    /*
     *  Open the output file and adjust the pointers so that
     *      we are ok.
     */

    if (!MapOutputFile( &p, szOutput, 0 )) {
        FatalError( ERR_MAP_FILE, szOutput );
    }

    CalculateOutputFilePointers( &p.iptrs, &p.optrs );

    /*
     *      Munge the various debug information structures
     *      to preform the correct operations
     */

    MungeDebugHeadersCoffToCv( &p, fAddCV );

    /*
     *  Write out the debug information to the end of the exe
     */

    WriteDebugInfo( &p, fAddCV, FALSE );

    /*
     *  and finally compute the checksum
     */

    if (p.iptrs.fileHdr != NULL) {
        ComputeChecksum( szOutput );
    }

    return;
}                               /* DoCoffToCv() */

/***    DoSymToCv
 *
 */

void
DoSymToCv(
    char * szInput,
    char * szOutput,
    char * szDbg,
    char * szSym
    )
{
    POINTERS    p;
    HANDLE      hFile;
    DWORD       cb;
    OFSTRUCT    ofs;

    /*
     *  Open the input file name and setup the pointers into the file
     */

    if (!MapInputFile( &p, NULL, szSym )) {
        FatalError(ERR_OPEN_INPUT_FILE, szSym);
    }

    /*
     *   Now preform the desired operation
     */

    if ((szOutput == NULL) && (szDbg == NULL)) {
        szOutput = szInput;
    }

    ConvertSymToCv( &p );

    if (szOutput) {
        if (szOutput != szInput) {
            if (OpenFile(szInput, &ofs, OF_EXIST) == (HFILE)INVALID_HANDLE_VALUE) {
                FatalError(ERR_OPEN_INPUT_FILE, szInput);
            }
            if (CopyFile(szInput, szOutput, FALSE) == 0) {
                FatalError(ERR_OPEN_WRITE_FILE, szOutput);
            }
        }
        hFile = CreateFile(szOutput, GENERIC_WRITE, 0, NULL,
                           CREATE_ALWAYS,
                           FILE_ATTRIBUTE_NORMAL,  NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            FatalError(ERR_OPEN_WRITE_FILE, szOutput);
        }
        SetFilePointer(hFile, 0, 0, FILE_END);
    } else if (szDbg) {
        hFile = CreateFile(szDbg, GENERIC_WRITE, 0, NULL,
                               OPEN_ALWAYS,
                               FILE_ATTRIBUTE_NORMAL,  NULL);
        if (hFile == INVALID_HANDLE_VALUE) {
            FatalError(ERR_OPEN_WRITE_FILE, szDbg);
        }
    }

    WriteFile(hFile, p.pCvStart.ptr, p.pCvStart.size, &cb, NULL);
    CloseHandle(hFile);

    return;
}                               /* DoSymToCv() */


void
DoNameChange(
    char * szInput,
    char * szOutput,
    char * szNewName
    )
{
    POINTERS    p;

    /*
     *  Open the input file name and setup the pointers into the file
     */

    if (!MapInputFile( &p, NULL, szInput )) {
        FatalError(ERR_OPEN_INPUT_FILE, szInput);
    }

    /*
     *  Now, if we thing we are playing with PE exes then we need
     *  to setup the pointers into the map file
     */

    if (!CalculateNtImagePointers( &p.iptrs )) {
        FatalError(ERR_INVALID_PE, szInput);
    }

    /*
     *   Now preform the desired operation
     */

    if (szOutput == NULL) {
        szOutput = szInput;
    }

    if (szNewName == NULL) {
        szNewName = szOutput;
    }

    if (p.iptrs.sepHdr != NULL) {
        FatalError(ERR_EDIT_DBG_FILE);
    }

    /*
     *  Read in all of the debug information
     */

    ReadDebugInfo(&p);

    /*
     *   Open the output file and adjust the pointers.
     */

    if (!MapOutputFile(&p, szOutput,
                       sizeof(szNewName) * 2 + sizeof(IMAGE_DEBUG_MISC))) {
        FatalError(ERR_MAP_FILE, szOutput);
    }

    CalculateOutputFilePointers(&p.iptrs, &p.optrs);

    /*
     *      Munge the name of the file
     */

    MungeExeName(&p, szNewName);

    /*
     *  Write out the debug information to the end of the exe
     */

    WriteDebugInfo(&p, FALSE, FALSE);

    /*
     *  and finally compute the checksum
     */

    if (p.iptrs.fileHdr != NULL) {
        ComputeChecksum( szOutput );
    }

    return;
}                               /* DoNameChange() */


void
DoExtract(
    char * szInput
    )
{
    char OutFile[_MAX_PATH];
    // Just call SplitSymbols in dbghelp.dll

    SplitSymbols(szInput, NULL, OutFile, SPLITSYM_EXTRACT_ALL);
    printf("Symbols for \"%s\" extracted into \"%s\"\n", szInput, OutFile);
}
