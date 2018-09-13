/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    splitsym.c

Abstract:

    This is the main source file for the SPLITSYM utility program.  This
    program can be used to split the debugging information contained in
    an executable image file into a separate .DBG file and strip it from
    the image file.  Allow stripped image files to be distributed which
    significantly reduces disk space requirements, but allows full
    debugging by accesing the associated .DBG files over the network
    when needed.

Author:

    Steve Wood (stevewo) 03-May-1993

Revision History:

--*/

#include <private.h>


BOOL fVerbose;
BOOL fRecurse;
ULONG SplitFlags = 0;

UCHAR RecurseDirectory[ MAX_PATH ];

UCHAR CurrentImageName[ MAX_PATH ];

UCHAR SymbolPath[ MAX_PATH ];

UCHAR DbgFileName[ MAX_PATH ];

VOID
SplitSymbolsInTree(
    LPSTR RootPath
    );

void
Usage( void )
{
    fputs ( "usage: SPLITSYM [-?] [-v] [-p] [-a] [-s symbol directory] [-r directory] image-names...\n"
            "              [-?] display this message\n"
            "              [-v] verbose output\n"
            "              [-p] remove private debug info when creating .dbg file\n"
            "              [-a] extract all debug info into .dbg file\n"
            "              [-r directory] - recursively process all image files.\n"
            "              [-s symbol directory] - where to put .DBG files.\n"
            "                                      Default is same place as image file.\n",
            stderr );
    exit( 1 );
}

int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    char c, *s;
    LPSTR FilePart;

    if (argc <= 1) {
        Usage();
        }

    SymbolPath[ 0 ] = '\0';
    while (--argc) {
        s = *++argv;
        if (*s == '/' || *s == '-') {
            while (c = *++s)
                switch (toupper( c )) {
                case '?':
                    Usage();
                    break;

                case 'V':
                    fVerbose = TRUE;
                    break;

                case 'R':
                    if (--argc) {
                        fRecurse = TRUE;
                        strcpy( (PCHAR) RecurseDirectory, *++argv );
                        }
                    else {
                        fprintf( stderr, "SPLITSYM: Argument to /%c switch missing\n", c );
                        Usage();
                        }
                    break;

                case 'S':
                    if (--argc) {
                        strcpy( (PCHAR) SymbolPath, *++argv );
                        }
                    else {
                        fprintf( stderr, "SPLITSYM: Argument to /%c switch missing\n", c );
                        Usage();
                        }
                    break;

                case 'P':
                    SplitFlags |= SPLITSYM_REMOVE_PRIVATE;
                    break;

                case 'A':
                    SplitFlags |= SPLITSYM_EXTRACT_ALL;
                    break;

                default:
                    fprintf( stderr, "SPLITSYM: Invalid switch - /%c\n", c );
                    Usage();
                    break;
                }
            }
        else {
            if (fRecurse) {
                fprintf( stderr, "SPLITSYM: May not specify specific file names with /R switch.\n" );
                Usage();
                }
            else
            if (!GetFullPathNameA( s, sizeof( CurrentImageName ), (PCHAR) CurrentImageName, &FilePart )) {
                fprintf( stderr, "SPLITSYM: invalid file name - %s (%u)\n", s, GetLastError() );
                }
            else {
                if (SplitSymbols( (PCHAR) CurrentImageName, (PCHAR) SymbolPath, (PCHAR) DbgFileName, SplitFlags )) {
                    if (fVerbose) {
                        fprintf( stdout,
                                 "SPLITSYM: %16s symbols split into %s\n",
                                 FilePart,
                                 DbgFileName
                               );
                        }
                    }
                else
                if (GetLastError() != ERROR_BAD_EXE_FORMAT &&
                    GetLastError() != ERROR_ALREADY_ASSIGNED
                   ) {
                    fprintf( stderr, "SPLITSYM: Unable to split symbols from '%s' into '%s' (%u)\n",
                             CurrentImageName,
                             DbgFileName,
                             GetLastError()
                           );
                    }
                }
            }
        }

    if (fRecurse) {
        SplitSymbolsInTree( (PCHAR) RecurseDirectory );
        }

    exit( 0 );
    return 0;
}


#define MAX_DEPTH 32

VOID
SplitSymbolsInTree(
    LPSTR RootPath
    )
{
    LPSTR FilePart;
    PUCHAR Prefix = (PUCHAR) "";
    CHAR PathBuffer[ MAX_PATH ];
    ULONG Depth;
    PCHAR PathTail[ MAX_DEPTH ];
    PCHAR FindHandle[ MAX_DEPTH ];
    LPWIN32_FIND_DATA FindFileData;
    UCHAR FindFileBuffer[ MAX_PATH + sizeof( WIN32_FIND_DATA ) ];

    strcpy( PathBuffer, RootPath );
    FindFileData = (LPWIN32_FIND_DATA)FindFileBuffer;
    Depth = 0;
    while (TRUE) {
startDirectorySearch:
        PathTail[ Depth ] = strchr( PathBuffer, '\0' );
        if (PathTail[ Depth ] > PathBuffer && PathTail[ Depth ][ -1 ] != '\\') {
            *(PathTail[ Depth ])++ = '\\';
            }

        strcpy( PathTail[ Depth ], "*.*" );
        FindHandle[ Depth ] = (PCHAR) FindFirstFile( PathBuffer, FindFileData );
        if (FindHandle[ Depth ] != INVALID_HANDLE_VALUE) {
            do {
                if (FindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if (strcmp( FindFileData->cFileName, "." ) &&
                        strcmp( FindFileData->cFileName, ".." ) &&
                        Depth < MAX_DEPTH
                       ) {
                        sprintf( PathTail[ Depth ], "%s\\", FindFileData->cFileName );
                        Depth++;
                        goto startDirectorySearch;
                        }
                    }
                else
                if (!(FindFileData->dwFileAttributes & FILE_ATTRIBUTE_READONLY)) {
                    strcpy( PathTail[ Depth ], FindFileData->cFileName );
                    if (!GetFullPathNameA( PathBuffer, sizeof( CurrentImageName ), (PCHAR) CurrentImageName, &FilePart )) {
                        fprintf( stderr, "SPLITSYM: invalid file name - %s (%u)\n", PathBuffer, GetLastError() );
                        }
                    else {
                        if (SplitSymbols( (PCHAR) CurrentImageName, (PCHAR) SymbolPath, (PCHAR) DbgFileName, SplitFlags )) {
                            if (fVerbose) {
                                fprintf( stdout,
                                         "SPLITSYM: %16s symbols split into %s\n",
                                         FilePart,
                                         DbgFileName
                                       );
                                }
                            }
                        else
                        if (GetLastError() != ERROR_BAD_EXE_FORMAT ) {
                            fprintf( stderr, "SPLITSYM: Unable to split symbols from '%s' into '%s' (%u)\n",
                                     CurrentImageName,
                                     DbgFileName,
                                     GetLastError()
                                   );
                            }
                        }
                    }

restartDirectorySearch:
                ;
                }
            while (FindNextFile( FindHandle[ Depth ], FindFileData ));
            FindClose( FindHandle[ Depth ] );

            if (Depth == 0) {
                break;
                }

            Depth--;
            goto restartDirectorySearch;
            }
        }

    return;
}
