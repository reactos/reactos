/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    upddbg.c

Abstract:

    This tool updates debug files to match corresponding binary checksum,
    base address and timestamp

Author:

    Matthew Hoehnen (matthoe) 08-Jun-1995

Revision History:

--*/
#define _IMAGEHLP_SOURCE_
#include <private.h>

BOOL                            fUpdate;
LPSTR                           CurrentImageName;
LOADED_IMAGE                    CurrentImage;
CHAR                            DebugFilePath[_MAX_PATH];
CHAR                            SymbolPathBuffer[MAX_PATH*10];
LPSTR                           SymbolPath;
DWORD                           dw;
LPSTR                           FilePart;
CHAR                            Buffer[MAX_PATH];
PIMAGE_LOAD_CONFIG_DIRECTORY    ConfigInfo;
CHAR                            c;
LPSTR                           p;
BOOL                            DbgHeaderModified;
ULONG                           CheckSum;
HANDLE                          hDbgFile;
ULONG                           cb;
IMAGE_SEPARATE_DEBUG_HEADER     DbgHeader;


VOID
DisplayUsage(
    VOID
    )
{
    fputs("usage: UPDDBG [switches] image-names... \n"
          "              [-?] display this message\n"
          "              [-u] update image\n"
          "              [-s] path to symbol files\n", stderr
          );
}



int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    if (argc <= 1) {
        DisplayUsage();
        return 1;
    }

    _tzset();

    while (--argc) {
        p = *++argv;
        if (*p == '/' || *p == '-') {
            while (c = *++p)
            switch (tolower( c )) {
                case '?':
                    DisplayUsage();
                    return 0;

                case 'u':
                    fUpdate = TRUE;
                    break;

                case 's':
                    argc--, argv++;
                    SymbolPath = *argv;
                    break;

                default:
                    fprintf( stderr, "UPDDBG: Invalid switch - /%c\n", c );
                    DisplayUsage();
                    return 1;
            }
        }
    }

    if (!SymbolPath) {
        if (GetEnvironmentVariable( "_nt_symbol_path", SymbolPathBuffer, sizeof(SymbolPathBuffer)-1 )) {
            SymbolPath = SymbolPathBuffer;
        }
    }

    if (!SymbolPath) {
        fprintf( stderr, "UPDDBG: uknown symbol file path\n" );
        return 1;
    }

    CurrentImageName = p;

    //
    // Map and load the current image
    //

    if (!MapAndLoad( CurrentImageName, NULL, &CurrentImage, FALSE, TRUE )) {
        fprintf( stderr, "UPDDBG: failure mapping and loading %s\n", CurrentImageName );
        return 1;
    }

    CurrentImageName = CurrentImage.ModuleName;

    FlushViewOfFile( CurrentImage.MappedAddress, 0 );

    if (!fUpdate) {
        hDbgFile = FindDebugInfoFile( CurrentImageName, SymbolPath, DebugFilePath );
        if (hDbgFile == INVALID_HANDLE_VALUE || hDbgFile == NULL) {
            fprintf( stderr, "UPDDBG: could not locate DBG file %s\n", CurrentImageName );
            return 1;
        }

        if (!ReadFile( hDbgFile, &DbgHeader, sizeof(IMAGE_SEPARATE_DEBUG_HEADER), &cb, NULL )) {
            fprintf( stderr, "UPDDBG: could not read DBG file %s\n", CurrentImageName );
            return 1;
        }

        printf( "*\n" );
        if (((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.CheckSum != DbgHeader.CheckSum) {
            printf( "*************************************\n" );
            printf( "* WARNING: checksums do not match   *\n" );
            printf( "*************************************\n" );
            printf( "*\n" );
        }

        _strlwr( CurrentImageName );
        _strlwr( DebugFilePath );

        printf( "Image    0x%08x %s\n", ((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.CheckSum, CurrentImageName );
        printf( "DBG File 0x%08x %s\n", DbgHeader.CheckSum, DebugFilePath );

        return 0;
    }

    if (!((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->FileHeader.Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
        fprintf( stderr, "UPDDBG: symbols have not been split %s\n", CurrentImageName );
        return 1;
    }

    if ( UpdateDebugInfoFileEx( CurrentImageName,
                                SymbolPath,
                                DebugFilePath,
                                (PIMAGE_NT_HEADERS32)(CurrentImage.FileHeader),
                                ((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->OptionalHeader.CheckSum
                                ) ) {
        if (GetLastError() == ERROR_INVALID_DATA) {
            printf( "UPDDBG: Warning - Old checksum did not match for %s\n", DebugFilePath );
        }
        printf( "Updated symbols for %s\n", DebugFilePath );
    } else {
        printf( "Unable to update symbols: %s\n", DebugFilePath );
    }

    return 0;
}

#define _BUILDING_UPDDBG_
#include "upddbgi.c"

