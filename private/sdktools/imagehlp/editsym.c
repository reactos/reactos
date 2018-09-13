/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    editsym.c

Abstract:

    This is the main source file for the EDITSYM utility program.  This
    program can be used to extract the debugging information contained in
    a separate .DBG file and put it back to the original image file.

Author:

    HonWah Chan 21-March-1994

Revision History:

--*/

#include <private.h>


BOOL fVerbose;

LPSTR FilePart;

UCHAR CurrentImageName[ MAX_PATH ];

UCHAR DbgFileName[ MAX_PATH ];


void
Usage( void )
{
    fputs( "Function: Extract debugging information contained in .dbg file\n"
           "\tand put it back in the original image file\n\n"
           "Usage: EDITSYM [-?] [-s symbol-filename] image-name\n"
           "\t\t[-?] display this message\n"
           "\t\t[-s symbol-filename] - name of the .DBG file.  Default is the same\n"
           "\t\t                        name and location as the image file.\n\n"
           "\tE.G.  EditSym -s Perfmon.dbg Perfmon.exe\n", stderr );
    exit( 1 );
}

BOOL EditSymbols(
    LPSTR pImageName,
    LPSTR pDbgFileName
    )
{
    PIMAGE_NT_HEADERS NtHeaders;
    HANDLE FileHandle, SymbolFileHandle;
    HANDLE hMappedFile;
    LPVOID ImageBase;
    PIMAGE_DEBUG_DIRECTORY DebugDirectories;
    PIMAGE_DEBUG_DIRECTORY DebugDirectoriesSave;
    DWORD DebugDirectorySize, NumberOfDebugDirectories;
    DWORD SavedErrorCode;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    DWORD i;
    DWORD NewFileSize, HeaderSum, CheckSum;
    DWORD DebugDataSize;
    LPBYTE DebugData;
    IMAGE_SEPARATE_DEBUG_HEADER DbgFileHeader;

    ImageBase = NULL;
    hMappedFile = 0;
    FileHandle = SymbolFileHandle = 0;
    DebugDirectoriesSave = NULL;

    //
    // open and map the file.
    //
    FileHandle = CreateFile( pImageName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL
                           );

    if (FileHandle == INVALID_HANDLE_VALUE) {
        return FALSE;
        }


    hMappedFile = CreateFileMapping( FileHandle,
                                     NULL,
                                     PAGE_READWRITE,
                                     0,
                                     0,
                                     NULL
                                   );
    if (!hMappedFile) {
        CloseHandle( FileHandle );
        return FALSE;
        }

    ImageBase = MapViewOfFile( hMappedFile,
                               FILE_MAP_WRITE,
                               0,
                               0,
                               0
                             );
    if (!ImageBase) {
        CloseHandle( hMappedFile );
        CloseHandle( FileHandle );
        return FALSE;
        }

    //
    // Everything is mapped. Now check the image and find nt image headers
    //

    NtHeaders = ImageNtHeader( ImageBase );
    if (NtHeaders == NULL) {
        SetLastError( ERROR_BAD_EXE_FORMAT );
        goto nosyms;
        }

    if ((NtHeaders->OptionalHeader.MajorLinkerVersion < 3) &&
        (NtHeaders->OptionalHeader.MinorLinkerVersion < 5)
       ) {
        SetLastError( ERROR_BAD_EXE_FORMAT );
        goto nosyms;
        }

    if (!(NtHeaders->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED)) {
        SetLastError( ERROR_ALREADY_ASSIGNED );
        goto nosyms;
        }

    DebugDirectories = (PIMAGE_DEBUG_DIRECTORY) ImageDirectoryEntryToData( ImageBase,
                                                  FALSE,
                                                  IMAGE_DIRECTORY_ENTRY_DEBUG,
                                                  &DebugDirectorySize
                                                );
    if (!DebugDirectoryIsUseful(DebugDirectories, DebugDirectorySize)) {
        SetLastError( ERROR_BAD_EXE_FORMAT );
        goto nosyms;
    }

    NumberOfDebugDirectories = DebugDirectorySize / sizeof( IMAGE_DEBUG_DIRECTORY );

    SymbolFileHandle = CreateFile( pDbgFileName,
                                   GENERIC_READ | GENERIC_WRITE,
                                   FILE_SHARE_READ,
                                   NULL,
                                   OPEN_EXISTING,
                                   0,
                                   NULL
                                 );
    if (SymbolFileHandle == INVALID_HANDLE_VALUE)
        goto nosyms;

    if (!ReadFile( SymbolFileHandle,
            &DbgFileHeader,
            sizeof(DbgFileHeader),
            &DebugDataSize,
            NULL) ||
        DebugDataSize != sizeof(DbgFileHeader)) {
        SetLastError( ERROR_BAD_EXE_FORMAT );
        goto nosyms;
        }

    if (DbgFileHeader.Signature != IMAGE_SEPARATE_DEBUG_SIGNATURE ||
        (DbgFileHeader.Flags & ~IMAGE_SEPARATE_DEBUG_FLAGS_MASK) != 0 ||
        DbgFileHeader.Machine != NtHeaders->FileHeader.Machine ||
        DbgFileHeader.Characteristics != NtHeaders->FileHeader.Characteristics ||
        DbgFileHeader.TimeDateStamp != NtHeaders->FileHeader.TimeDateStamp ||
        DbgFileHeader.CheckSum != NtHeaders->OptionalHeader.CheckSum ||
        DbgFileHeader.ImageBase != NtHeaders->OptionalHeader.ImageBase ||
        DbgFileHeader.SizeOfImage != NtHeaders->OptionalHeader.SizeOfImage) {
        SetLastError( ERROR_BAD_EXE_FORMAT );
        goto nosyms;
        }

    if (DbgFileHeader.Flags & IMAGE_SEPARATE_DEBUG_MISMATCH) {
        fprintf(stderr, "Warning: %s updated unsafely; symbols may be wrong\n",
                pDbgFileName);
    }


    // check if this is the right dbg file

    // save the DebugDirectory and get ready to write the
    // debug data to the image file.
    DebugDirectoriesSave = (PIMAGE_DEBUG_DIRECTORY) malloc( DebugDirectorySize );
    if (DebugDirectoriesSave == NULL)
        goto nosyms;

    RtlMoveMemory( DebugDirectoriesSave,
                   DebugDirectories,
                   DebugDirectorySize);

    DebugDirectory = DebugDirectoriesSave;
    NewFileSize = SetFilePointer( FileHandle, 0, NULL, FILE_END );
    NewFileSize = (NewFileSize + 3) & ~3;

    for (i=0; i<NumberOfDebugDirectories; i++) {
        // Is it one of the debug sections we need to special case?
        if (DebugDirectory->Type == IMAGE_DEBUG_TYPE_MISC) {
            // fix the mage name

            RtlCopyMemory(((PCHAR) ImageBase +
                                DebugDirectory->PointerToRawData +
                                FIELD_OFFSET( IMAGE_DEBUG_MISC, Data )),
                          FilePart,
                          strlen(FilePart) + 1);

        }
        else if (DebugDirectory->Type != IMAGE_DEBUG_TYPE_FPO) {
            DebugData = (LPBYTE) malloc( DebugDirectory->SizeOfData );
            if (SetFilePointer( SymbolFileHandle,
                    DebugDirectory->PointerToRawData,
                    NULL,
                    FILE_BEGIN ) != DebugDirectory->PointerToRawData) {
                SetLastError( ERROR_BAD_EXE_FORMAT );
                goto nosyms;
                }

            if (ReadFile( SymbolFileHandle,
                          DebugData,
                          DebugDirectory->SizeOfData,
                          &DebugDataSize,
                          NULL) &&
                DebugDataSize == DebugDirectory->SizeOfData) {
                if (WriteFile( FileHandle,
                               DebugData,
                               DebugDirectory->SizeOfData,
                               &DebugDataSize,
                               NULL) &&
                    DebugDataSize == DebugDirectory->SizeOfData) {
                    DebugDirectory->PointerToRawData = NewFileSize;
                    NewFileSize += DebugDataSize;
                    NewFileSize = (NewFileSize + 3) & ~3;
                }
                else {
                    SetLastError( ERROR_WRITE_FAULT );
                    free( DebugData );
                    goto nosyms;
                }
            }
            else {
                SetLastError( ERROR_BAD_EXE_FORMAT );
                free( DebugData );
                goto nosyms;
            }
            free( DebugData );
        }
        DebugDirectory += 1;
    }


    // somehow I needed to close the file and re-open it again.
    // otherwise it would AV inside CheckSumMappedFile.
    UnmapViewOfFile( ImageBase );
    CloseHandle( hMappedFile );
    ImageBase = NULL;
    hMappedFile = 0;

    SetFilePointer( FileHandle, NewFileSize, NULL, FILE_BEGIN );
    SetEndOfFile( FileHandle );
    CloseHandle( FileHandle );


    //
    // re-open and map the file.
    //
    FileHandle = CreateFile( pImageName,
                             GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ,
                             NULL,
                             OPEN_EXISTING,
                             0,
                             NULL
                           );


    hMappedFile = CreateFileMapping( FileHandle,
                                     NULL,
                                     PAGE_READWRITE,
                                     0,
                                     0,
                                     NULL
                                   );
    if (!hMappedFile) {
        goto nosyms;
        }

    ImageBase = MapViewOfFile( hMappedFile,
                               FILE_MAP_WRITE,
                               0,
                               0,
                               0
                             );
    if (!ImageBase) {
        goto nosyms;
        }

    NtHeaders = ImageNtHeader( ImageBase );
    if (NtHeaders == NULL) {
        SetLastError( ERROR_BAD_EXE_FORMAT );
        goto nosyms;
        }

    DebugDirectories = (PIMAGE_DEBUG_DIRECTORY) ImageDirectoryEntryToData( ImageBase,
                                                  FALSE,
                                                  IMAGE_DIRECTORY_ENTRY_DEBUG,
                                                  &DebugDirectorySize
                                                );

    if (DebugDirectories == NULL || DebugDirectorySize == 0) {
        SetLastError( ERROR_BAD_EXE_FORMAT );
        goto nosyms;
        }


    RtlMoveMemory( DebugDirectories,
                   DebugDirectoriesSave,
                   DebugDirectorySize);

    free( DebugDirectoriesSave );

    NtHeaders->FileHeader.Characteristics &= ~IMAGE_FILE_DEBUG_STRIPPED;

    CheckSumMappedFile( ImageBase,
                        NewFileSize,
                        &HeaderSum,
                        &CheckSum
                      );

    NtHeaders->OptionalHeader.CheckSum = CheckSum;


    CloseHandle( SymbolFileHandle );

    UnmapViewOfFile( ImageBase );
    CloseHandle( hMappedFile );
    CloseHandle( FileHandle );

    return TRUE;

nosyms:
    SavedErrorCode = GetLastError();

    if (DebugDirectoriesSave)
        free( DebugDirectoriesSave );

    if (SymbolFileHandle && SymbolFileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle( SymbolFileHandle );
    }

    if (ImageBase)
        UnmapViewOfFile( ImageBase );

    if (hMappedFile)
        CloseHandle( hMappedFile );

    if (FileHandle && FileHandle != INVALID_HANDLE_VALUE) {
        CloseHandle( FileHandle );
    }

    SetLastError( SavedErrorCode );
    return FALSE;
}

int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    char c, *s;
    LPSTR DbgFilePart;

    if (argc <= 1) {
        Usage();
        }

    DbgFileName[ 0 ] = '\0';
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

                case 'S':
                    if (--argc) {
                        strcpy( (PCHAR) DbgFileName, *++argv );
                        }
                    else {
                        fprintf( stderr, "EDITSYM: Argument to /%c switch missing\n", c );
                        Usage();
                        }
                    break;

                default:
                    fprintf( stderr, "EDITSYM: Invalid switch - /%c\n", c );
                    Usage();
                    break;
                }
            }
        else {
            FilePart = (PCHAR) CurrentImageName;
            if (!GetFullPathNameA( s, sizeof( CurrentImageName ), (PCHAR) CurrentImageName, &FilePart )) {
                fprintf( stderr, "EDITSYM: invalid file name - %s (%u)\n", s, GetLastError() );
                }
            else {
                if (DbgFileName[0] == '\0') {
                    PCHAR  pDbgName;

                    RtlCopyMemory(DbgFileName,
                          CurrentImageName,
                          strlen((PCHAR) CurrentImageName) + 1);
                    pDbgName = (PCHAR) DbgFileName + strlen ((PCHAR) DbgFileName);
                    while (pDbgName > (PCHAR) DbgFileName) {
                        if (*pDbgName == '.') {
                            break;
                        }
                        pDbgName --;
                    }
                    if (*pDbgName != '.') {
                        fprintf( stderr, "EDITSYM: invalid exe file name - %s\n", CurrentImageName );
                    }
                    strcpy (pDbgName, ".DBG");
                }
                else if (!GetFullPathNameA( (PCHAR) DbgFileName, sizeof( DbgFileName ), (PCHAR) DbgFileName, &DbgFilePart )) {
                    fprintf( stderr, "EDITSYM: invalid Dbg file name - %s (%u)\n", s, GetLastError() );
                }

                if (EditSymbols( (PCHAR) CurrentImageName, (PCHAR) DbgFileName )) {
                    if (fVerbose) {
                        fprintf( stdout,
                                 "EDITSYM: %s symbols restored into %s\n",
                                 DbgFileName,
                                 FilePart
                               );
                        }
                    }
                else
                    fprintf( stderr, "EDITSYM: Unable to restore symbols from '%s' into '%s' (%u)\n",
                             CurrentImageName,
                             DbgFileName,
                             GetLastError()
                           );
                }
            }
        }

    exit( 0 );
    return 0;
}
