/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    dbgdump.c

Abstract:

    This is the main source file for the DBGDUMP utility program.  This
    program can be used to dump out symbolic debugging information for an
    image.

Author:

    Steve Wood (stevewo) 03-May-1993

Revision History:

--*/

#include <private.h>


BOOL fVerbose;
BOOL fRemote;

UCHAR CurrentImageName[ MAX_PATH ];
UCHAR SymbolPath[ MAX_PATH ];

void
Usage( void )
{
    fputs("usage: DBGDUMP [-?] [-v] [-r] image-names...\n"
          "              [-?] display this message\n"
          "              [-v] verbose output\n"
          "              [-r symbol path] assume image names are from remote system.\n",
          stderr);
    exit( 1 );
}

void
ShowDebugInfo(
    PIMAGE_DEBUG_INFORMATION64 DebugInfo
    );

VOID
DumpSectionHeader(
    IN ULONG i,
    IN PIMAGE_SECTION_HEADER Sh
    );


int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    char c, *s;
    LPSTR FilePart;
    HANDLE FileHandle;
    PIMAGE_DEBUG_INFORMATION64 DebugInfo;

    if (argc <= 1) {
        Usage();
        }

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
                        strcpy( (PCHAR) SymbolPath, *++argv );
                        fRemote = TRUE;
                        }
                    else {
                        fprintf( stderr, "DBGDUMP: Argument to /%c switch missing\n", c );
                        Usage();
                        }
                    break;

                default:
                    fprintf( stderr, "DBGDUMP: Invalid switch - /%c\n", c );
                    Usage();
                    break;
                }
            }
        else {
            DebugInfo = NULL;
            if (!fRemote) {
                if (!GetFullPathNameA( s, sizeof( CurrentImageName ), (PCHAR) CurrentImageName, &FilePart )) {
                    fprintf( stderr, "DBGDUMP: invalid file name - %s (%u)\n", s, GetLastError() );
                    }
                else {
                    FileHandle = CreateFileA( (PCHAR)CurrentImageName,
                                             GENERIC_READ,
                                             FILE_SHARE_READ | FILE_SHARE_WRITE,
                                             NULL,
                                             OPEN_EXISTING,
                                             0,
                                             NULL
                                           );
                    if (FileHandle == INVALID_HANDLE_VALUE) {
                        fprintf( stderr, "DBGDUMP: unable to open - %s (%u)\n", CurrentImageName, GetLastError() );
                        }
                    else {
                        DebugInfo = MapDebugInformation64( FileHandle, NULL, NULL, 0 );
                        }
                    }
                }
            else {
                strcpy( (PCHAR) CurrentImageName, s );
                DebugInfo = MapDebugInformation64( NULL, (PCHAR) CurrentImageName, (PCHAR) SymbolPath, 0 );
                }

            if (DebugInfo != NULL) {
                ShowDebugInfo( DebugInfo );
                UnmapDebugInformation64( DebugInfo );
                }
            }
        }

    exit( 0 );
    return 0;
}


VOID
DumpSectionHeader(
    IN ULONG i,
    IN PIMAGE_SECTION_HEADER Sh
    )
{
    PCHAR name;
    ULONG li, lj;
    USHORT memFlags, alignFlags;

    printf( "\nSECTION HEADER #%hX\n% 8.8s name\n", i, Sh->Name );

    printf( "% 8lX physical address\n% 8lX virtual address\n% 8lX size of raw data\n% 8lX file pointer to raw data\n% 8lX file pointer to relocation table\n",
           Sh->Misc.PhysicalAddress,
           Sh->VirtualAddress,
           Sh->SizeOfRawData,
           Sh->PointerToRawData,
           Sh->PointerToRelocations );

    printf( "% 8lX file pointer to line numbers\n% 8hX number of relocations\n% 8hX number of line numbers\n% 8lX flags\n",
           Sh->PointerToLinenumbers,
           Sh->NumberOfRelocations,
           Sh->NumberOfLinenumbers,
           Sh->Characteristics );

    memFlags = alignFlags = 0;
    for (li=Sh->Characteristics, lj=0L; li; li=li>>1, lj++) {
        if (li & 1) {
            switch((li & 1) << lj) {
                case IMAGE_SCN_TYPE_NO_PAD    : name = (PCHAR) "No Pad"; break;


                case IMAGE_SCN_CNT_CODE       : name = (PCHAR) "Code"; break;
                case IMAGE_SCN_CNT_INITIALIZED_DATA : name = (PCHAR) "Initialized Data"; break;
                case IMAGE_SCN_CNT_UNINITIALIZED_DATA : name = (PCHAR) "Uninitialized Data"; break;

                case IMAGE_SCN_LNK_OTHER      : name = (PCHAR) "Other"; break;
                case IMAGE_SCN_LNK_INFO       : name = (PCHAR) "Info"; break;
                case IMAGE_SCN_LNK_REMOVE     : name = (PCHAR) "Remove"; break;
                case IMAGE_SCN_LNK_COMDAT     : name = (PCHAR) "Communal"; break;

                case IMAGE_SCN_ALIGN_1BYTES   :
                case IMAGE_SCN_ALIGN_2BYTES   :
                case IMAGE_SCN_ALIGN_4BYTES   :
                case IMAGE_SCN_ALIGN_8BYTES   :
                case IMAGE_SCN_ALIGN_16BYTES  :
                case IMAGE_SCN_ALIGN_32BYTES  :
                case IMAGE_SCN_ALIGN_64BYTES  : name = (PCHAR) ""; break;

                case IMAGE_SCN_MEM_DISCARDABLE: name = (PCHAR) "Discardable"; break;
                case IMAGE_SCN_MEM_NOT_CACHED : name = (PCHAR) "Not Cached"; break;
                case IMAGE_SCN_MEM_NOT_PAGED  : name = (PCHAR) "Not Paged"; break;
                case IMAGE_SCN_MEM_SHARED     : name = (PCHAR) "Shared"; break;
                case IMAGE_SCN_MEM_EXECUTE    : name = (PCHAR) ""; memFlags |= 1; break;
                case IMAGE_SCN_MEM_READ       : name = (PCHAR) ""; memFlags |= 2; break;
                case IMAGE_SCN_MEM_WRITE      : name = (PCHAR) ""; memFlags |= 4; break;


                default : name = (PCHAR) "RESERVED - UNKNOWN";
            }
            if (*name) {
                printf( "         %s\n", name );
            }
        }
    }

    if (Sh->Characteristics & IMAGE_SCN_ALIGN_64BYTES) {
        switch(Sh->Characteristics & IMAGE_SCN_ALIGN_64BYTES) {
            case IMAGE_SCN_ALIGN_1BYTES  : name = (PCHAR) "Align1";  break;
            case IMAGE_SCN_ALIGN_2BYTES  : name = (PCHAR) "Align2";  break;
            case IMAGE_SCN_ALIGN_4BYTES  : name = (PCHAR) "Align4";  break;
            case IMAGE_SCN_ALIGN_8BYTES  : name = (PCHAR) "Align8";  break;
            case IMAGE_SCN_ALIGN_16BYTES : name = (PCHAR) "Align16"; break;
            case IMAGE_SCN_ALIGN_32BYTES : name = (PCHAR) "Align32"; break;
            case IMAGE_SCN_ALIGN_64BYTES : name = (PCHAR) "Align64"; break;
        }
        printf( "         %s\n", name );
    }

    if (memFlags) {
        switch(memFlags) {
            case 1 : name = (PCHAR) "Execute Only"; break;
            case 2 : name = (PCHAR) "Read Only"; break;
            case 3 : name = (PCHAR) "Execute Read"; break;
            case 4 : name = (PCHAR) "Write Only"; break;
            case 5 : name = (PCHAR) "Execute Write"; break;
            case 6 : name = (PCHAR) "Read Write"; break;
            case 7 : name = (PCHAR) "Execute Read Write"; break;
            default : name = (PCHAR) "Unknown Memory Flags"; break;
        }
        printf( "         %s\n", name );
    }
}


char *FrameType[] = {
    "FRAME_FPO",
    "FRAME_TRAP",
    "FRAME_TSS",
    "FRAME_NONFPO",
    "FRAME_UNKNOWN"
};

void
ShowDebugInfo(
    PIMAGE_DEBUG_INFORMATION64 DebugInfo
    )
{
    ULONG i;
    PIMAGE_SECTION_HEADER Section;
    PIMAGE_FUNCTION_ENTRY FunctionEntry;
    PFPO_DATA FpoEntry;
    LPSTR s;


    printf( "Debug information at % p\n", DebugInfo );
    printf( "    Size             % 8lx\n", DebugInfo->Size );
    printf( "    Mapped at        % p\n", DebugInfo->MappedBase );
    printf( "    Machine          % 8hx\n", DebugInfo->Machine );
    printf( "    Characteristics  % 8hx\n", DebugInfo->Characteristics );
    printf( "    Time/Date stamp  % 8lx",   DebugInfo->TimeDateStamp );
    if (DebugInfo->TimeDateStamp && (s = ctime( (time_t *)&DebugInfo->TimeDateStamp ))) {
        printf( " %s", s );
        }
    else {
        putchar( '\n' );
        }
    printf( "    CheckSum         % 8lx\n", DebugInfo->CheckSum );
    printf( "    ImageBase        % 8lx\n", DebugInfo->ImageBase );
    printf( "    SizeOfImage      % 8lx\n", DebugInfo->SizeOfImage );
    printf( "    NumberOfSections % 8lx\n", DebugInfo->NumberOfSections );
    printf( "    ExportedNamesSize% 8lx\n", DebugInfo->ExportedNamesSize );
    printf( "    #Function Entries% 8lx\n", DebugInfo->NumberOfFunctionTableEntries );
    printf( "    #FPO Entries     % 8lx\n", DebugInfo->NumberOfFpoTableEntries );
    printf( "    Coff Symbol Size % 8lx\n", DebugInfo->SizeOfCoffSymbols );
    printf( "    CV Symbol Size   % 8lx\n", DebugInfo->SizeOfCodeViewSymbols );
    printf( "    Image Path               %s\n", DebugInfo->ImageFilePath );
    printf( "    Image Name               %s\n", DebugInfo->ImageFileName );
    printf( "    Debug Path               %s\n", DebugInfo->DebugFilePath );
    printf( "\n" );

    if (DebugInfo->NumberOfSections != 0) {
        printf( "Section Headers:\n" );
        Section = DebugInfo->Sections;
        for (i=0; i<DebugInfo->NumberOfSections; i++) {
            DumpSectionHeader( i, Section++ );
            }
        printf( "\n" );
        }

    if (DebugInfo->ExportedNamesSize != 0) {
        printf( "Exported Names:\n" );
        s = DebugInfo->ExportedNames;
        while (*s) {
            printf( "    %s\n", s );
            while (*s++) {
                }
            }
        printf( "\n" );
        }

    if (DebugInfo->NumberOfFunctionTableEntries != 0) {
        printf( "Function Table:\n" );
        FunctionEntry = DebugInfo->FunctionTableEntries;
        for (i=0; i<DebugInfo->NumberOfFunctionTableEntries; i++) {
            printf( "    % 4x: % 8x % 8x % 8x\n",
                    i,
                    FunctionEntry->StartingAddress,
                    FunctionEntry->EndingAddress,
                    FunctionEntry->EndOfPrologue
                  );

            FunctionEntry += 1;
            }
        printf( "\n" );
        }

    if (DebugInfo->NumberOfFpoTableEntries != 0) {
        printf( "FPO Table:\n" );
        FpoEntry = DebugInfo->FpoTableEntries;
        for (i=0; i<DebugInfo->NumberOfFpoTableEntries; i++) {
            printf( "    % 4x: % 8x % 8x % 8x [%02x %1x%s%s %s]\n",
                    i,
                    FpoEntry->ulOffStart,
                    FpoEntry->cbProcSize,
                    FpoEntry->cdwParams,
                    FpoEntry->cbProlog,
                    FpoEntry->cbRegs,
                    FpoEntry->fHasSEH ? " SEH" : "",
                    FpoEntry->fUseBP ? " EBP" : "",
                    FrameType[ FpoEntry->cbFrame ]
                  );

            FpoEntry += 1;
            }

        printf( "\n" );
        }

    return;
}
