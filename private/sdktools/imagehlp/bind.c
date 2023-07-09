/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    bind.c

Abstract:

Author:

Revision History:

--*/

#include <private.h>

BOOL
Match(
    char *Pattern,
    char *Text
    )
{
    switch (*Pattern) {
       case '\0':
            return *Text == '\0';

        case '?':
            return *Text != '\0' && Match( Pattern + 1, Text + 1 );

        case '*':
            do {
                if (Match( Pattern + 1, Text ))
                    return TRUE;
                    }
            while (*Text++);
            return FALSE;

        default:
            return toupper( *Text ) == toupper( *Pattern ) && Match( Pattern + 1, Text + 1 );
        }
}


BOOL
AnyMatches(
    char *Name,
    int  *NumList,
    int  Length,
    char **StringList
    )
{
    if (Length == 0) {
        return FALSE;
        }

    return (Match( StringList[ NumList[ 0 ] ], Name ) ||
            AnyMatches( Name, NumList + 1, Length - 1, StringList )
           );
}

BOOL
BindStatusRoutine(
    IMAGEHLP_STATUS_REASON Reason,
    LPSTR ImageName,
    LPSTR DllName,
    PVOID Va,
    ULONG_PTR Parameter
    );

#define BIND_ERR 99
#define BIND_OK  0

PCHAR SymbolPath;

BOOL fVerbose;
BOOL fNoUpdate = TRUE;
BOOL fDisableNewImports;
BOOL fNoCacheImportDlls;
BOOL fBindSysImages;
DWORD BindFlags;

int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    char c, *p;
    int ExcludeList[256];
    int ExcludeListLength = 0;

    BOOL fUsage = FALSE;
    LPSTR DllPath;
    LPSTR CurrentImageName;

    int ArgNumber = argc;
    char **ArgList = argv;

    envp;

    DllPath = NULL;
    CurrentImageName = NULL;

    if (argc < 2) {
        goto usage;
    }

    while (--argc) {
        p = *++argv;
        if (*p == '/' || *p == '-') {
            while (c = *++p)
            switch (toupper( c )) {
                case '?':
                    fUsage = TRUE;
                    break;

                case 'C':
                    fNoCacheImportDlls = TRUE;
                    break;

                case 'O':
                    fDisableNewImports = TRUE;
                    break;

                case 'P':
                    if (--argc) {
                        DllPath = *++argv;
                    } else {
                        fprintf( stderr, "BIND: Parameter missing for /%c\n", c );
                        fUsage = TRUE;
                    }
                    break;

                case 'S':
                    if (--argc) {
                        SymbolPath = *++argv;
                    } else {
                        fprintf( stderr, "BIND: Parameter missing for /%c\n", c );
                        fUsage = TRUE;
                    }
                    break;

                case 'U':
                    fNoUpdate = FALSE;
                    break;

                case 'V':
                    fVerbose = TRUE;
                    break;

                case 'X' :
                    if (--argc) {
                        ++argv;
                        ExcludeList[ExcludeListLength] = ArgNumber - argc;
                        ExcludeListLength++;
                    } else {
                        fprintf( stderr, "BIND: Parameter missing for /%c\n", c );
                        fUsage = TRUE;
                    }
                    break;

                case 'Y':
                    fBindSysImages = TRUE;
                    break;

                default:
                    fprintf( stderr, "BIND: Invalid switch - /%c\n", c );
                    fUsage = TRUE;
                    break;
                }
            if (fUsage) {
usage:
                fputs("usage: BIND [switches] image-names... \n"
                      "            [-?] display this message\n"
                      "            [-c] no caching of import dlls\n"
                      "            [-o] disable new import descriptors\n"
                      "            [-p dll search path]\n"
                      "            [-s Symbol directory] update any associated .DBG file\n"
                      "            [-u] update the image\n"
                      "            [-v] verbose output\n"
                      "            [-x image name] exclude this image from binding\n"
                      "            [-y] allow binding on images located above 2G",
                      stderr
                     );
                return BIND_ERR;
            }
        } else {
            CurrentImageName = p;
            if (fVerbose) {
                fprintf( stdout,
                         "BIND: binding %s using DllPath %s\n",
                         CurrentImageName,
                         DllPath ? DllPath : "Default"
                       );
            }

            if (AnyMatches( CurrentImageName, ExcludeList, ExcludeListLength, ArgList )) {
                if (fVerbose) {
                    fprintf( stdout, "BIND: skipping %s\n", CurrentImageName );
                }
            } else {
                BindFlags = 0;

                if (!fNoCacheImportDlls) {
                    // Always cache across calls unless the user indicates otherwise.
                    BindFlags |= BIND_CACHE_IMPORT_DLLS;
                }
                if (fNoUpdate) {
                    BindFlags |= BIND_NO_UPDATE;
                }
                if (fDisableNewImports) {
                    BindFlags |= BIND_NO_BOUND_IMPORTS;
                }
                if (fBindSysImages) {
                    BindFlags |= BIND_ALL_IMAGES;
                }

                BindImageEx( BindFlags,
                             CurrentImageName,
                             DllPath,
                             SymbolPath,
                             (PIMAGEHLP_STATUS_ROUTINE)BindStatusRoutine
                           );
            }
        }
    }

    return BIND_OK;
}


BOOL
BindStatusRoutine(
    IMAGEHLP_STATUS_REASON Reason,
    LPSTR ImageName,
    LPSTR DllName,
    PVOID Va,
    ULONG_PTR Parameter
    )
{
    PIMAGE_BOUND_IMPORT_DESCRIPTOR NewImports, NewImport;
    PIMAGE_BOUND_FORWARDER_REF NewForwarder;
    UINT i;

    switch( Reason ) {
        case BindOutOfMemory:
            fprintf( stderr, "BIND: Out of memory - needed %u bytes.\n", Parameter );
            ExitProcess( 1 );

        case BindRvaToVaFailed:
            fprintf( stderr, "BIND: %s contains invalid Rva - %p\n", ImageName, Va );
            break;

        case BindNoRoomInImage:
            fprintf( stderr,
                     "BIND: Not enough room for new format import table.  Defaulting to unbound image.\n"
                   );
            break;

        case BindImportModuleFailed:
            fprintf( stderr,"BIND: %s - Unable to find %s\n", ImageName, DllName );
            break;

        case BindImportProcedureFailed:
            fprintf( stderr,
                     "BIND: %s - %s entry point not found in %s\n",
                     ImageName,
                     Parameter,
                     DllName
                   );
            break;

        case BindImportModule:
            if (fVerbose) {
                fprintf( stderr,"BIND: %s - Imports from %s\n", ImageName, DllName );
                }
            break;

        case BindImportProcedure:
            if (fVerbose) {
                fprintf( stderr,
                         "BIND: %s - %s Bound to %p\n",
                         ImageName,
                         Parameter,
                         Va
                       );
                }
            break;

        case BindForwarder:
            if (fVerbose) {
                fprintf( stderr, "BIND: %s - %s forwarded to %s [%p]\n",
                         ImageName,
                         DllName,
                         Parameter,
                         Va
                       );
            }
            break;

        case BindForwarderNOT:
            if (fVerbose) {
                fprintf( stderr,
                         "BIND: %s - Forwarder %s not snapped [%p]\n",
                         ImageName,
                         Parameter,
                         Va
                       );
                }
            break;

        case BindImageModified:
            fprintf( stdout, "BIND: binding %s\n", ImageName );
            break;


        case BindExpandFileHeaders:
            if (fVerbose) {
                fprintf( stderr,
                         "    Expanded %s file headers to %x\n",
                         ImageName,
                         Parameter
                       );
                }
            break;

        case BindMismatchedSymbols:
            fprintf(stderr, "BIND: Warning: %s checksum did not match %s\n",
                            ImageName,
                            (LPSTR)Parameter);
            break;

        case BindSymbolsNotUpdated:
            fprintf(stderr, "BIND: Warning: symbol file %s not updated.\n",
                            (LPSTR)Parameter);
            break;

        case BindImageComplete:
            if (fVerbose) {
                fprintf(stderr, "BIND: Details of binding of %s\n", ImageName );
                NewImports = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)Va;
                NewImport = NewImports;
                while (NewImport->OffsetModuleName) {
                    fprintf( stderr, "    Import from %s [%x]",
                             (LPSTR)NewImports + NewImport->OffsetModuleName,
                             NewImport->TimeDateStamp
                           );
                    if (NewImport->NumberOfModuleForwarderRefs != 0) {
                        fprintf( stderr, " with %u forwarders", NewImport->NumberOfModuleForwarderRefs );
                    }
                    fprintf( stderr, "\n" );
                    NewForwarder = (PIMAGE_BOUND_FORWARDER_REF)(NewImport+1);
                    for ( i=0; i<NewImport->NumberOfModuleForwarderRefs; i++ ) {
                        fprintf( stderr, "        Forward to %s [%x]\n",
                                 (LPSTR)NewImports + NewForwarder->OffsetModuleName,
                                 NewForwarder->TimeDateStamp
                               );
                        NewForwarder += 1;
                    }
                    NewImport = (PIMAGE_BOUND_IMPORT_DESCRIPTOR)NewForwarder;
                }
            }
            break;
    }

    return TRUE;
}
