/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    imagecfg.c

Abstract:

    This function change the image loader configuration information in an image file.

Author:

    Steve Wood (stevewo)   8-Nov-1994

Revision History:

--*/

#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <private.h>

//
// Applications should include the following declaration in their
// global data to create an IMAGE_LOAD_CONFIG_DIRECTORY entry for
// their image.  Non-zero entries override defaults.
//

#if 0

IMAGE_LOAD_CONFIG_DIRECTORY _load_config_used = {
    0,              // Characteristics;
    0,              // TimeDateStamp;
    4,              // MajorVersion;
    0,              // MinorVersion;
    0,              // GlobalFlagsClear;
    0,              // GlobalFlagsSet;
    0,              // CriticalSectionDefaultTimeout;
    0,              // DeCommitFreeBlockThreshold;
    0,              // DeCommitTotalFreeThreshold;
    0,              // LockPrefixTable;
    0,              // MaximumAllocationSize;
    0,              // VirtualMemoryThreshold;
    0,              // ProcessHeapFlags;
    0,              // ProcessAffinityMask;
    0, 0, 0         // Reserved[ 3 ];
};

#endif


struct {
    DWORD Flag;
    LPSTR ClearPrefix;
    LPSTR SetPrefix;
    LPSTR Description;
} NtGlobalFlagNames[] = {
    {FLG_STOP_ON_EXCEPTION,             "Don't ", "", "Stop on exception"},
    {FLG_SHOW_LDR_SNAPS,                "Don't ", "", "Show Loader Debugging Information"},
    {FLG_DEBUG_INITIAL_COMMAND,         "Don't ", "", "Debug Initial Command (WINLOGON)"},
    {FLG_STOP_ON_HUNG_GUI,              "Don't ", "", "Stop on Hung GUI"},
    {FLG_HEAP_ENABLE_TAIL_CHECK,        "Disable", "Enable", " Heap Tail Checking"},
    {FLG_HEAP_ENABLE_FREE_CHECK,        "Disable", "Enable", " Heap Free Checking"},
    {FLG_HEAP_VALIDATE_PARAMETERS,      "Disable", "Enable", " Heap Parameter Validation"},
    {FLG_HEAP_VALIDATE_ALL,             "Disable", "Enable", " Heap Validate on Call"},
    {FLG_POOL_ENABLE_TAGGING,           "Disable", "Enable", " Pool Tagging"},
    {FLG_HEAP_ENABLE_TAGGING,           "Disable", "Enable", " Heap Tagging"},
    {FLG_USER_STACK_TRACE_DB,           "Disable", "Enable", " User Mode Stack Backtrace DB (x86 checked only)"},
    {FLG_KERNEL_STACK_TRACE_DB,         "Disable", "Enable", " Kernel Mode Stack Backtrace DB (x86 checked only)"},
    {FLG_MAINTAIN_OBJECT_TYPELIST,      "Don't ", "", "Maintain list of kernel mode objects by type"},
    {FLG_HEAP_ENABLE_TAG_BY_DLL,        "Disable", "Enable", " Heap DLL Tagging"},
    {FLG_ENABLE_CSRDEBUG,               "Disable", "Enable", " Debugging of CSRSS"},
    {FLG_ENABLE_KDEBUG_SYMBOL_LOAD,     "Disable", "Enable", " Kernel Debugger Symbol load"},
    {FLG_DISABLE_PAGE_KERNEL_STACKS,    "Enable", "Disable", " Paging of Kernel Stacks"},
    {FLG_HEAP_DISABLE_COALESCING,       "Enable", "Disable", " Heap Coalescing on Free"},
    {FLG_ENABLE_CLOSE_EXCEPTIONS,       "Disable", "Enable", " Close Exceptions"},
    {FLG_ENABLE_EXCEPTION_LOGGING,      "Disable", "Enable", " Exception Logging"},
    {FLG_ENABLE_HANDLE_TYPE_TAGGING,    "Disable", "Enable", " Handle type tagging"},
    {FLG_HEAP_PAGE_ALLOCS,              "Disable", "Enable", " Heap page allocs"},
    {FLG_DEBUG_INITIAL_COMMAND_EX,      "Disable", "Enable", " Extended debug initial command"},
    {FLG_DISABLE_DBGPRINT,              "Enable",  "Disable"," DbgPrint to debugger"},
    {0, NULL}
};

void
DisplayGlobalFlags(
    LPSTR IndentString,
    DWORD NtGlobalFlags,
    BOOLEAN Set
    )
{
    ULONG i;

    for (i=0; NtGlobalFlagNames[i].Description; i++) {
        if (NtGlobalFlagNames[i].Flag & NtGlobalFlags) {
            printf( "%s%s%s\n",
                    IndentString,
                    Set ? NtGlobalFlagNames[i].SetPrefix :
                    NtGlobalFlagNames[i].ClearPrefix,
                    NtGlobalFlagNames[i].Description
                  );
        }
    }

    return;
}

BOOL fVerbose;
BOOL fUsage;

BOOL fConfigInfoChanged;
BOOL fImageHasConfigInfo;
BOOL fImageHeaderChanged;

LPSTR CurrentImageName;
PIMAGE_OPTIONAL_HEADER32 OptionalHeader32;
PIMAGE_OPTIONAL_HEADER64 OptionalHeader64;
PIMAGE_FILE_HEADER FileHeader;
LOADED_IMAGE CurrentImage;
IMAGE_LOAD_CONFIG_DIRECTORY ConfigInfo;
CHAR DebugFilePath[_MAX_PATH];
LPSTR SymbolPath;
ULONG GlobalFlagsClear;
ULONG GlobalFlagsSet;
ULONG CriticalSectionDefaultTimeout;
ULONG DeCommitFreeBlockThreshold;
ULONG DeCommitTotalFreeThreshold;
ULONG MaximumAllocationSize;
ULONG VirtualMemoryThreshold;
ULONG ProcessHeapFlags;
ULONG MajorSubsystemVersion;
ULONG MinorSubsystemVersion;
ULONG BuildNumber;
ULONG SizeOfStackReserve;
ULONG SizeOfStackCommit;
PULONG pBuildNumber;
ULONG Win32VersionValue;
ULONG Win32CSDVerValue;
BOOLEAN fUniprocessorOnly;
BOOLEAN fRestrictedWorkingSet;
BOOLEAN fEnableLargeAddresses;
BOOLEAN fNoBind;
BOOLEAN fEnableTerminalServerAware;
BOOLEAN fDisableTerminalServerAware;
BOOLEAN fSwapRunNet;
BOOLEAN fSwapRunCD;
BOOLEAN fQuiet;
DWORD ImageProcessAffinityMask;

VOID
DisplayImageInfo(
                BOOL HasConfigInfo
                );

PVOID
GetAddressOfExportedData(
                        PLOADED_IMAGE Dll,
                        LPSTR ExportedName
                        );

ULONG
ConvertNum(
          char *s
          )
{
    ULONG n, Result;

    if (!_strnicmp( s, "0x", 2 )) {
        n = sscanf( s+2, "%x", &Result );
    } else {
        n = sscanf( s, "%u", &Result );
    }

    if (n != 1) {
        return 0;
    } else {
        return Result;
    }
}

int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    UCHAR c;
    LPSTR p, sMajor, sMinor, sReserve, sCommit;
    ULONG HeaderSum;
    SYSTEMTIME SystemTime;
    FILETIME LastWriteTime;
    DWORD OldChecksum;

    fUsage = FALSE;
    fVerbose = FALSE;

    _tzset();

    if (argc <= 1) {
        goto showUsage;
    }

    while (--argc) {
        p = *++argv;
        if (*p == '/' || *p == '-') {
            while (c = *++p)
                switch (toupper( c )) {
                    case '?':
                        fUsage = TRUE;
                        break;

                    case 'A':
                        if (--argc) {
                            ImageProcessAffinityMask = ConvertNum( *++argv );
                            if (ImageProcessAffinityMask == 0) {
                                fprintf( stderr, "IMAGECFG: invalid affinity mask specified to /a switch.\n" );
                                fUsage = TRUE;
                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /a switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'B':
                        if (--argc) {
                            BuildNumber = ConvertNum( *++argv );
                            if (BuildNumber == 0) {
                                fprintf( stderr, "IMAGECFG: invalid build number specified to /b switch.\n" );
                                fUsage = TRUE;
                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /b switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'C':
                        if (--argc) {
                            if (sscanf( *++argv, "%x", &Win32CSDVerValue ) != 1) {
                                fprintf( stderr, "IMAGECFG: invalid version string specified to /c switch.\n" );
                                fUsage = TRUE;
                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /c switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'D':
                        if (argc >= 2) {
                            argc -= 2;
                            DeCommitFreeBlockThreshold = ConvertNum( *++argv );
                            DeCommitTotalFreeThreshold = ConvertNum( *++argv );
                        } else {
                            fprintf( stderr, "IMAGECFG: /d switch missing arguments.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'G':
                        if (argc >= 2) {
                            argc -= 2;
                            GlobalFlagsClear = ConvertNum( *++argv );
                            GlobalFlagsSet = ConvertNum( *++argv );
                        } else {
                            fprintf( stderr, "IMAGECFG: /g switch missing arguments.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'H':
                        if (argc > 2) {

                            INT flag = -1;

                            if (sscanf( *++argv, "%d", &flag ) != 1) {
                                fprintf( stderr, "IMAGECFG: invalid option string specified to /h switch.\n" );
                                fUsage = TRUE;
                            } else {

                                --argc;

                                if (flag == 0) {
                                    fDisableTerminalServerAware = TRUE;
                                } else if (flag == 1) {
                                    fEnableTerminalServerAware = TRUE;
                                } else {
                                    fprintf( stderr, "IMAGECFG: /h switch invalid argument.\n" );
                                    fUsage = TRUE;
                                }

                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /h switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'K':
                        if (--argc) {
                            sReserve = *++argv;
                            sCommit = strchr( sReserve, '.' );
                            if (sCommit != NULL) {
                                *sCommit++ = '\0';
                                SizeOfStackCommit = ConvertNum( sCommit );
                                SizeOfStackCommit = ((SizeOfStackCommit + 0xFFF) & ~0xFFF);
                                if (SizeOfStackCommit == 0) {
                                    fprintf( stderr, "IMAGECFG: invalid stack commit size specified to /k switch.\n" );
                                    fUsage = TRUE;
                                }
                            }

                            SizeOfStackReserve = ConvertNum( sReserve );
                            SizeOfStackReserve = ((SizeOfStackReserve + 0xFFFF) & ~0xFFFF);
                            if (SizeOfStackReserve == 0) {
                                fprintf( stderr, "IMAGECFG: invalid stack reserve size specified to /k switch.\n" );
                                fUsage = TRUE;
                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /w switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'L':
                        fEnableLargeAddresses = TRUE;
                        break;

                    case 'M':
                        if (--argc) {
                            MaximumAllocationSize = ConvertNum( *++argv );
                        } else {
                            fprintf( stderr, "IMAGECFG: /m switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'N':
                        fNoBind = TRUE;
                        break;

                    case 'O':
                        if (--argc) {
                            CriticalSectionDefaultTimeout = ConvertNum( *++argv );
                        } else {
                            fprintf( stderr, "IMAGECFG: /o switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'P':
                        if (--argc) {
                            ProcessHeapFlags = ConvertNum( *++argv );
                        } else {
                            fprintf( stderr, "IMAGECFG: /p switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'Q':
                        fQuiet = TRUE;
                        break;

                    case 'R':
                        fRestrictedWorkingSet = TRUE;
                        break;

                    case 'S':
                        if (--argc) {
                            SymbolPath = *++argv;
                        } else {
                            fprintf( stderr, "IMAGECFG: /s switch missing path argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'T':
                        if (--argc) {
                            VirtualMemoryThreshold = ConvertNum( *++argv );
                        } else {
                            fprintf( stderr, "IMAGECFG: /t switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'U':
                        fUniprocessorOnly = TRUE;
                        break;

                    case 'V':
                        if (--argc) {
                            sMajor = *++argv;
                            sMinor = strchr( sMajor, '.' );
                            if (sMinor != NULL) {
                                *sMinor++ = '\0';
                                MinorSubsystemVersion = ConvertNum( sMinor );
                            }
                            MajorSubsystemVersion = ConvertNum( sMajor );

                            if (MajorSubsystemVersion == 0) {
                                fprintf( stderr, "IMAGECFG: invalid version string specified to /v switch.\n" );
                                fUsage = TRUE;
                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /v switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'W':
                        if (--argc) {
                            if (sscanf( *++argv, "%x", &Win32VersionValue ) != 1) {
                                fprintf( stderr, "IMAGECFG: invalid version string specified to /w switch.\n" );
                                fUsage = TRUE;
                            }
                        } else {
                            fprintf( stderr, "IMAGECFG: /w switch missing argument.\n" );
                            fUsage = TRUE;
                        }
                        break;

                    case 'X':
                        fSwapRunNet = TRUE;
                        break;

                    case 'Y':
                        fSwapRunCD = TRUE;
                        break;

                    default:
                        fprintf( stderr, "IMAGECFG: Invalid switch - /%c\n", c );
                        fUsage = TRUE;
                        break;
                }

            if ( fUsage ) {
                showUsage:
                fprintf( stderr,
                         "usage: IMAGECFG [switches] image-names... \n"
                         "              [-?] display this message\n"
                         "              [-a Process Affinity mask value in hex]\n"
                         "              [-b BuildNumber]\n"
                         "              [-c Win32 GetVersionEx Service Pack return value in hex]\n"
                         "              [-d decommit thresholds]\n"
                         "              [-g bitsToClear bitsToSet]\n"
                         "              [-h 1|0 (Enable/Disable Terminal Server Compatible bit)\n"
                         "              [-k StackReserve[.StackCommit]\n"
                         "              [-l enable large (>2GB) addresses\n"
                         "              [-m maximum allocation size]\n"
                         "              [-n bind no longer allowed on this image\n"
                         "              [-o default critical section timeout\n"
                         "              [-p process heap flags]\n"
                         "              [-q only print config info if changed\n"
                         "              [-r run with restricted working set]\n"
                         "              [-s path to symbol files]\n"
                         "              [-t VirtualAlloc threshold]\n"
                         "              [-u Marks image as uniprocessor only]\n"
                         "              [-v MajorVersion.MinorVersion]\n"
                         "              [-w Win32 GetVersion return value in hex]\n"
                         "              [-x Mark image as Net - Run From Swapfile\n"
                         "              [-y Mark image as Removable - Run From Swapfile\n"
                       );
                exit( 1 );
            }
        } else {
            //
            // Map and load the current image
            //

            OptionalHeader32 = NULL;
            OptionalHeader64 = NULL;
            FileHeader = NULL;
            CurrentImageName = p;
            if (MapAndLoad( CurrentImageName,
                            NULL,
                            &CurrentImage,
                            FALSE,
                            TRUE
                          )
               ) {
                if (BuildNumber != 0) {
                    pBuildNumber = (PULONG) GetAddressOfExportedData( &CurrentImage, "NtBuildNumber" );
                    if (pBuildNumber == NULL) {
                        fprintf( stderr,
                                 "IMAGECFG: Unable to find exported NtBuildNumber image %s\n",
                                 CurrentImageName
                               );
                    }
                }

                FileHeader = &((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->FileHeader;
                OptionalHeadersFromNtHeaders((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader,
                                             &OptionalHeader32,
                                             &OptionalHeader64);

                //
                // make sure the image has correct configuration information,
                // and that the LockPrefixTable is set up properly
                //

                fConfigInfoChanged = FALSE;
                fImageHeaderChanged = FALSE;
                ZeroMemory(&ConfigInfo, sizeof(ConfigInfo));
                fImageHasConfigInfo = GetImageConfigInformation( &CurrentImage, &ConfigInfo );
                if (!fQuiet) {
                    DisplayImageInfo( fImageHasConfigInfo );
                }
                UnMapAndLoad( &CurrentImage );
                OptionalHeader32 = NULL;
                OptionalHeader64 = NULL;
                FileHeader = NULL;
                if (fConfigInfoChanged || fImageHeaderChanged) {
                    if (!MapAndLoad( CurrentImageName,
                                     NULL,
                                     &CurrentImage,
                                     FALSE,
                                     FALSE
                                   )
                       ) {
                        if (!CurrentImage.fDOSImage) {
                            fprintf( stderr, "IMAGECFG: unable to map and load %s\n", CurrentImageName );
                        } else {
                            fprintf( stderr,
                                     "IMAGECFG: unable to modify DOS or Windows image file - %s\n",
                                     CurrentImageName
                                   );
                        }
                    } else {
                        FileHeader = &((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader)->FileHeader;

                        OptionalHeadersFromNtHeaders((PIMAGE_NT_HEADERS32)CurrentImage.FileHeader,
                                                     &OptionalHeader32,
                                                     &OptionalHeader64);

                        if (GlobalFlagsClear) {
                            ConfigInfo.GlobalFlagsClear = GlobalFlagsClear;
                        }

                        if (GlobalFlagsSet) {
                            ConfigInfo.GlobalFlagsSet = GlobalFlagsSet;
                        }

                        if (CriticalSectionDefaultTimeout) {
                            ConfigInfo.CriticalSectionDefaultTimeout = CriticalSectionDefaultTimeout;
                        }

                        if (ProcessHeapFlags) {
                            ConfigInfo.ProcessHeapFlags = ProcessHeapFlags;
                        }

                        if (DeCommitFreeBlockThreshold) {
                            ConfigInfo.DeCommitFreeBlockThreshold = DeCommitFreeBlockThreshold;
                        }

                        if (DeCommitTotalFreeThreshold) {
                            ConfigInfo.DeCommitTotalFreeThreshold = DeCommitTotalFreeThreshold;
                        }

                        if (MaximumAllocationSize) {
                            ConfigInfo.MaximumAllocationSize = MaximumAllocationSize;
                        }

                        if (VirtualMemoryThreshold) {
                            ConfigInfo.VirtualMemoryThreshold = VirtualMemoryThreshold;
                        }

                        if (ImageProcessAffinityMask) {
                            ConfigInfo.ProcessAffinityMask = ImageProcessAffinityMask;
                        }

                        if (fEnableLargeAddresses) {
                            FileHeader->Characteristics |= IMAGE_FILE_LARGE_ADDRESS_AWARE;
                        }

                        if (fNoBind) {
                            OPTIONALHEADER_SET_FLAG(DllCharacteristics,IMAGE_DLLCHARACTERISTICS_NO_BIND);
                        }

                        if (fEnableTerminalServerAware) {
                            OPTIONALHEADER_SET_FLAG(DllCharacteristics,IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE);
                        }

                        if (fDisableTerminalServerAware) {
                            OPTIONALHEADER_CLEAR_FLAG(DllCharacteristics,IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE);
                        }

                        if (fSwapRunNet) {
                            FileHeader->Characteristics |= IMAGE_FILE_NET_RUN_FROM_SWAP;
                        }

                        if (fSwapRunCD) {
                            FileHeader->Characteristics |= IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP;
                        }

                        if (fUniprocessorOnly) {
                            FileHeader->Characteristics |= IMAGE_FILE_UP_SYSTEM_ONLY;
                        }

                        if (fRestrictedWorkingSet) {
                            FileHeader->Characteristics |= IMAGE_FILE_AGGRESIVE_WS_TRIM;
                        }

                        if (MajorSubsystemVersion != 0) {
                            OPTIONALHEADER_ASSIGN(MajorSubsystemVersion, (USHORT)MajorSubsystemVersion);
                            OPTIONALHEADER_ASSIGN(MinorSubsystemVersion, (USHORT)MinorSubsystemVersion);
                        }

                        if (Win32VersionValue != 0) {
                            OPTIONALHEADER_ASSIGN(Win32VersionValue, Win32VersionValue);
                        }

                        if (Win32CSDVerValue != 0) {
                            ConfigInfo.CSDVersion = (USHORT)Win32CSDVerValue;
                        }

                        if (SizeOfStackReserve) {
                            OPTIONALHEADER_ASSIGN(SizeOfStackReserve, SizeOfStackReserve);
                        }

                        if (SizeOfStackCommit) {
                            OPTIONALHEADER_ASSIGN(SizeOfStackCommit, SizeOfStackCommit);
                        }

                        if (BuildNumber != 0) {
                            pBuildNumber = (PULONG) GetAddressOfExportedData( &CurrentImage, "NtBuildNumber" );
                            if (pBuildNumber == NULL) {
                                fprintf( stderr,
                                         "IMAGECFG: Unable to find exported NtBuildNumber image %s\n",
                                         CurrentImageName
                                       );
                            } else {
                                if (BuildNumber & 0xFFFF0000) {
                                    *pBuildNumber = BuildNumber;
                                } else {
                                    *(PUSHORT)pBuildNumber = (USHORT)BuildNumber;
                                }
                            }
                        }

                        if (fConfigInfoChanged) {
                            if (SetImageConfigInformation( &CurrentImage, &ConfigInfo )) {
                                if (!fQuiet) {
                                    printf( "%s updated with the following configuration information:\n", CurrentImageName );
                                    DisplayImageInfo( fImageHasConfigInfo );
                                }
                            } else {
                                fprintf( stderr, "IMAGECFG: Unable to update configuration information in image.\n" );

                            }
                        }

                        //
                        // recompute the checksum.
                        //

                        OldChecksum = OPTIONALHEADER(CheckSum);
                        OPTIONALHEADER_LV(CheckSum) = 0;
                        CheckSumMappedFile(
                                          (PVOID)CurrentImage.MappedAddress,
                                          CurrentImage.SizeOfImage,
                                          &HeaderSum,
                                          &OPTIONALHEADER_LV(CheckSum)
                                          );

                        // And update the .dbg file (if requested)
                        if (SymbolPath &&
                            FileHeader->Characteristics & IMAGE_FILE_DEBUG_STRIPPED) {
                            if (UpdateDebugInfoFileEx( CurrentImageName,
                                                       SymbolPath,
                                                       DebugFilePath,
                                                       (PIMAGE_NT_HEADERS32)CurrentImage.FileHeader,
                                                       OldChecksum
                                                     )
                               ) {
                                if (GetLastError() == ERROR_INVALID_DATA) {
                                    printf( "Warning: Old checksum did not match for %s\n", DebugFilePath);
                                }
                                printf( "Updated symbols for %s\n", DebugFilePath );
                            } else {
                                printf( "Unable to update symbols: %s\n", DebugFilePath );
                            }
                        }

                        GetSystemTime( &SystemTime );
                        if (SystemTimeToFileTime( &SystemTime, &LastWriteTime )) {
                            SetFileTime( CurrentImage.hFile, NULL, NULL, &LastWriteTime );
                        }

                        UnMapAndLoad( &CurrentImage );
                    }
                }
            } else
                if (!CurrentImage.fDOSImage) {
                fprintf( stderr, "IMAGECFG: unable to map and load %s  GetLastError= %d\n", CurrentImageName, GetLastError() );

            } else {
                fprintf( stderr,
                         "IMAGECFG: unable to modify DOS or Windows image file - %s\n",
                         CurrentImageName
                       );
            }
        }
    }

    exit( 1 );
    return 1;
}

__inline PVOID
GetVaForRva(
           PLOADED_IMAGE Image,
           ULONG Rva
           )
{
    PVOID Va;

    Va = ImageRvaToVa( Image->FileHeader,
                       Image->MappedAddress,
                       Rva,
                       &Image->LastRvaSection
                     );
    return Va;
}


PVOID
GetAddressOfExportedData(
                        PLOADED_IMAGE Dll,
                        LPSTR ExportedName
                        )
{
    PIMAGE_EXPORT_DIRECTORY Exports;
    ULONG ExportSize;
    USHORT HintIndex;
    USHORT OrdinalNumber;
    PULONG NameTableBase;
    PUSHORT NameOrdinalTableBase;
    PULONG FunctionTableBase;
    LPSTR NameTableName;

    Exports = (PIMAGE_EXPORT_DIRECTORY)ImageDirectoryEntryToData( (PVOID)Dll->MappedAddress,
                                                                  FALSE,
                                                                  IMAGE_DIRECTORY_ENTRY_EXPORT,
                                                                  &ExportSize
                                                                );
    if (Exports) {
        NameTableBase = (PULONG)GetVaForRva( Dll, Exports->AddressOfNames );
        NameOrdinalTableBase = (PUSHORT)GetVaForRva( Dll, Exports->AddressOfNameOrdinals );
        FunctionTableBase = (PULONG)GetVaForRva( Dll, Exports->AddressOfFunctions );
        if (NameTableBase != NULL &&
            NameOrdinalTableBase != NULL &&
            FunctionTableBase != NULL
           ) {
            for (HintIndex = 0; HintIndex < Exports->NumberOfNames; HintIndex++) {
                NameTableName = (LPSTR)GetVaForRva( Dll, NameTableBase[ HintIndex ] );
                if (NameTableName) {
                    if (!strcmp( ExportedName, NameTableName )) {
                        OrdinalNumber = NameOrdinalTableBase[ HintIndex ];
                        return FunctionTableBase[ OrdinalNumber ] + Dll->MappedAddress;
                    }
                }
            }
        }
    }

    return NULL;
}


VOID
DisplayImageInfo(
                BOOL HasConfigInfo
                )
{
    printf( "%s contains the following configuration information:\n", CurrentImageName );
//    if (HasConfigInfo) {
        if (ConfigInfo.GlobalFlagsClear != 0) {
            printf( "    NtGlobalFlags to clear: %08x\n",
                    ConfigInfo.GlobalFlagsClear
                  );
            DisplayGlobalFlags( "        ", ConfigInfo.GlobalFlagsClear, FALSE );
        }
        if (GlobalFlagsClear && ConfigInfo.GlobalFlagsClear != GlobalFlagsClear) {
            fConfigInfoChanged = TRUE;
        }

        if (ConfigInfo.GlobalFlagsSet != 0) {
            printf( "    NtGlobalFlags to set:   %08x\n",
                    ConfigInfo.GlobalFlagsSet
                  );
            DisplayGlobalFlags( "        ", ConfigInfo.GlobalFlagsSet, TRUE );
        }
        if (GlobalFlagsSet && ConfigInfo.GlobalFlagsSet != GlobalFlagsSet) {
            fConfigInfoChanged = TRUE;
        }


        if (ConfigInfo.CriticalSectionDefaultTimeout != 0) {
            printf( "    Default Critical Section Timeout: %u milliseconds\n",
                    ConfigInfo.CriticalSectionDefaultTimeout
                  );
        }
        if (CriticalSectionDefaultTimeout &&
            ConfigInfo.CriticalSectionDefaultTimeout != CriticalSectionDefaultTimeout
           ) {
            fConfigInfoChanged = TRUE;
        }

        if (ConfigInfo.ProcessHeapFlags != 0) {
            printf( "    Process Heap Flags: %08x\n",
                    ConfigInfo.ProcessHeapFlags
                  );
        }
        if (ProcessHeapFlags && ConfigInfo.ProcessHeapFlags != ProcessHeapFlags) {
            fConfigInfoChanged = TRUE;
        }

        if (ConfigInfo.DeCommitFreeBlockThreshold != 0) {
            printf( "    Process Heap DeCommit Free Block threshold: %08x\n",
                    ConfigInfo.DeCommitFreeBlockThreshold
                  );
        }
        if (DeCommitFreeBlockThreshold &&
            ConfigInfo.DeCommitFreeBlockThreshold != DeCommitFreeBlockThreshold
           ) {
            fConfigInfoChanged = TRUE;
        }

        if (ConfigInfo.DeCommitTotalFreeThreshold != 0) {
            printf( "    Process Heap DeCommit Total Free threshold: %08x\n",
                    ConfigInfo.DeCommitTotalFreeThreshold
                  );
        }
        if (DeCommitTotalFreeThreshold &&
            ConfigInfo.DeCommitTotalFreeThreshold != DeCommitTotalFreeThreshold
           ) {
            fConfigInfoChanged = TRUE;
        }

        if (ConfigInfo.MaximumAllocationSize != 0) {
            printf( "    Process Heap Maximum Allocation Size: %08x\n",
                    ConfigInfo.MaximumAllocationSize
                  );
        }
        if (MaximumAllocationSize && ConfigInfo.MaximumAllocationSize != MaximumAllocationSize) {
            fConfigInfoChanged = TRUE;
        }

        if (ConfigInfo.VirtualMemoryThreshold != 0) {
            printf( "    Process Heap VirtualAlloc Threshold: %08x\n",
                    ConfigInfo.VirtualMemoryThreshold
                  );
        }
        if (VirtualMemoryThreshold &&
            ConfigInfo.VirtualMemoryThreshold != VirtualMemoryThreshold
           ) {
            fConfigInfoChanged = TRUE;
        }

        if (ConfigInfo.ProcessAffinityMask != 0) {
            printf( "    Process Affinity Mask: %08x\n",
                    ConfigInfo.ProcessAffinityMask
                  );
        }
        if (ImageProcessAffinityMask &&
            ConfigInfo.ProcessAffinityMask != ImageProcessAffinityMask
           ) {
            fConfigInfoChanged = TRUE;
        }
//    } else {
//        memset( &ConfigInfo, 0, sizeof( ConfigInfo ) );
//    }

    printf( "    Subsystem Version of %u.%u\n",
            OPTIONALHEADER(MajorSubsystemVersion),
            OPTIONALHEADER(MinorSubsystemVersion)
          );
    if (MajorSubsystemVersion != 0) {
        if (OPTIONALHEADER(MajorSubsystemVersion) != (USHORT)MajorSubsystemVersion ||
            OPTIONALHEADER(MinorSubsystemVersion) != (USHORT)MinorSubsystemVersion
           ) {
            fImageHeaderChanged = TRUE;
        }
    }

    if (pBuildNumber != NULL) {
        printf( "    Build Number of %08x\n", *pBuildNumber );
        if (BuildNumber != 0) {
            if (BuildNumber & 0xFFFF0000) {
                if (*pBuildNumber != BuildNumber) {
                    fImageHeaderChanged = TRUE;
                }
            } else {
                if (*(PUSHORT)pBuildNumber != (USHORT)BuildNumber) {
                    fImageHeaderChanged = TRUE;
                }
            }
        }
    }

    if (OPTIONALHEADER(Win32VersionValue) != 0) {
        printf( "    Win32 GetVersion return value: %08x\n",
                OPTIONALHEADER(Win32VersionValue)
              );
    }
    if (Win32VersionValue != 0 &&
        OPTIONALHEADER(Win32VersionValue) != Win32VersionValue
       ) {
        fImageHeaderChanged = TRUE;
    }

    if (FileHeader->Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE) {
        printf( "    Image can handle large (>2GB) addresses\n" );
    }

    if (OPTIONALHEADER(DllCharacteristics) & IMAGE_DLLCHARACTERISTICS_TERMINAL_SERVER_AWARE) {
        printf( "    Image is Terminal Server aware\n" );
    }

    if (fEnableLargeAddresses &&
        !(FileHeader->Characteristics & IMAGE_FILE_LARGE_ADDRESS_AWARE)
       ) {
        fImageHeaderChanged = TRUE;
        printf( "    Image is Large Address aware\n" );
    }

    if (fNoBind) {
        fImageHeaderChanged = TRUE;
        printf( "    Image will no longer support binding\n" );
    }

    if (fEnableTerminalServerAware || fDisableTerminalServerAware) {
        printf( "    Image %s Terminal Server Aware\n", fEnableTerminalServerAware ? "is" : "is not");
        fImageHeaderChanged = TRUE;
    }

    if (FileHeader->Characteristics & IMAGE_FILE_NET_RUN_FROM_SWAP) {
        printf( "    Image will run from swapfile if located on net\n" );
    }
    if (fSwapRunNet &&
        !(FileHeader->Characteristics & IMAGE_FILE_NET_RUN_FROM_SWAP)
       ) {
        fImageHeaderChanged = TRUE;
    }

    if (FileHeader->Characteristics & IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP) {
        printf( "    Image will run from swapfile if located on removable media\n" );
    }
    if (fSwapRunCD &&
        !(FileHeader->Characteristics & IMAGE_FILE_REMOVABLE_RUN_FROM_SWAP)
       ) {
        fImageHeaderChanged = TRUE;
    }

    if (FileHeader->Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY) {
        printf( "    Image can only run in uni-processor mode on multi-processor systems\n" );
    }
    if (fUniprocessorOnly &&
        !(FileHeader->Characteristics & IMAGE_FILE_UP_SYSTEM_ONLY)
       ) {
        fImageHeaderChanged = TRUE;
    }

    if (FileHeader->Characteristics & IMAGE_FILE_AGGRESIVE_WS_TRIM) {
        printf( "    Image working set trimmed aggressively on small memory systems\n" );
    }
    if (fRestrictedWorkingSet &&
        !(FileHeader->Characteristics & IMAGE_FILE_AGGRESIVE_WS_TRIM)
       ) {
        fImageHeaderChanged = TRUE;
    }

    if (OPTIONALHEADER(SizeOfStackReserve)) {
        printf( "    Stack Reserve Size: 0x%x\n", OPTIONALHEADER(SizeOfStackReserve) );
    }
    if (SizeOfStackReserve &&
        OPTIONALHEADER(SizeOfStackReserve) != SizeOfStackReserve
       ) {
        fImageHeaderChanged = TRUE;
    }

    if (OPTIONALHEADER(SizeOfStackCommit)) {
        printf( "    Stack Commit Size: 0x%x\n", OPTIONALHEADER(SizeOfStackCommit) );
    }
    if (SizeOfStackCommit &&
        OPTIONALHEADER(SizeOfStackCommit) != SizeOfStackCommit
       ) {
        fImageHeaderChanged = TRUE;
    }

    return;
}
