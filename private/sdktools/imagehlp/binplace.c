#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#include <private.h>
#include <crt\io.h>
#include <share.h>
#include <time.h>
#include <setupapi.h>
#include "..\..\windows\base\advapi\rsa.h"
#include "..\..\windows\base\advapi\md5.h"
#include "symutil.h"

#ifndef MOVEFILE_CREATE_HARDLINK
    #define MOVEFILE_CREATE_HARDLINK  0x00000010
#endif

#define BINPLACE_ERR 77
#define BINPLACE_OK 0

BOOL fUpDriver;
BOOL fUsage;
BOOL fVerbose;
BOOL fSymChecking;
BOOL fTestMode;
BOOL fSplitSymbols;
BOOL fSetupMode;
BOOL fSetupModeAllFiles;
BOOL fSetupModeScriptFile;
BOOL fPatheticOS;
BOOL fLiveSystem;
BOOL fKeepAttributes;
BOOL fDigitalSign;
BOOL fHardLinks;
BOOL fIgnoreHardLinks;
BOOL fDontLog;
BOOL fPlaceWin95SymFile;
BOOL fNoClassInSymbolsDir;
BOOL fMakeErrorOnDumpCopy;
BOOL fDontExit;
BOOL fForcePlace;
BOOL fSignCode;
BOOL fVerifyLc;

HINSTANCE hSetupApi;
HINSTANCE hLcManager;
HRESULT (WINAPI * pVerifyLocConstraintA) (IN PCSTR FileName, IN PCSTR LcFileName);
BOOL (WINAPI * pSetupGetIntField) (IN PINFCONTEXT Context, IN DWORD FieldIndex, OUT PINT IntegerValue);
BOOL (WINAPI * pSetupFindFirstLineA) (IN HINF InfHandle, IN PCSTR Section, IN PCSTR Key, OPTIONAL OUT PINFCONTEXT Context );
BOOL (WINAPI * pSetupGetStringFieldA) (IN PINFCONTEXT Context, IN DWORD FieldIndex, OUT PSTR ReturnBuffer, OPTIONAL IN DWORD ReturnBufferSize, OUT PDWORD RequiredSize);
HINF (WINAPI * pSetupOpenInfFileA) ( IN PCSTR FileName, IN PCSTR InfClass, OPTIONAL IN DWORD InfStyle, OUT PUINT ErrorLine OPTIONAL );
HINF (WINAPI * pSetupOpenMasterInf) (VOID);

ULONG SplitFlags = 0;

LPSTR CurrentImageName;
LPSTR PlaceFileName;
LPSTR PlaceRootName;
LPSTR ExcludeFileName;
LPSTR DumpOverride;
LPSTR LayoutInfName;
LPSTR NormalPlaceSubdir;
LPSTR CommandScriptName;
LPSTR SymbolFilePath;
LPSTR PrivateSymbolFilePath;
LPSTR BinplaceLcDir;
LPSTR LcFilePart;

HINF LayoutInf;

FILE *PlaceFile;
FILE *LogFile;
FILE *CommandScriptFile;
CHAR gFullFileName[MAX_PATH+1];
UCHAR SetupFilePath[ MAX_PATH+1 ];
UCHAR DebugFilePath[ MAX_PATH+1 ];
UCHAR PlaceFilePath[ MAX_PATH+1 ];
UCHAR ExcludeFilePath[ MAX_PATH+1 ];
UCHAR DefaultSymbolFilePath[ MAX_PATH+1 ];
UCHAR szAltPlaceRoot[ MAX_PATH+1 ];
UCHAR LcFullFileName[ MAX_PATH+1 ];
UCHAR szExtraInfo[4096];

PEXCLUDE_LIST ExcludeList;

#define DEFAULT_PLACE_FILE    "\\public\\sdk\\lib\\placefil.txt"
#define DEFAULT_NTROOT        "\\nt"
#define DEFAULT_NTDRIVE       "c:"
#define DEFAULT_DUMP          "dump"
#define DEFAULT_LCDIR         "LcINF"
#define DEFAULT_EXCLUDE_FILE  "\\public\\tools\\symbad.txt"

typedef struct _CLASS_TABLE {
    LPSTR ClassName;
    LPSTR ClassLocation;
} CLASS_TABLE, *PCLASS_TABLE;

BOOL
PlaceTheFile();

typedef
BOOL
(WINAPI *PCREATEHARDLINKA)(
                          LPCSTR lpFileName,
                          LPCSTR lpExistingFileName,
                          LPSECURITY_ATTRIBUTES lpSecurityAttributes
                          );

PCREATEHARDLINKA pCreateHardLinkA;

BOOL
CopyTheFile(
           LPSTR SourceFileName,
           LPSTR SourceFilePart,
           LPSTR DestinationSubdir,
           LPSTR DestinationFilePart
           );

BOOL
BinplaceCopyPdb (
                LPSTR DestinationFile,
                LPSTR SourceFileName,       // Used for redist case
                BOOL  CopyFromSourceOnly,
                BOOL  StripPrivate
                );

BOOL
VerifyFinalImage(
                IN  PCHAR FileName,
                IN  BOOL  fRetail,
                OUT PBOOL BinplaceLc
                );

BOOL
SourceIsNewer(
             IN LPSTR SourceFile,
             IN LPSTR TargetFile
             );

BOOL
SetupModeRetailFile(
                   IN  LPSTR FullFileName,
                   IN  LPSTR FileNamePart,
                   OUT PBOOL PutInDump
                   );

__inline BOOL
SearchOneDirectory(
                  IN  LPSTR Directory,
                  IN  LPSTR FileToFind,
                  IN  LPSTR SourceFullName,
                  IN  LPSTR SourceFilePart,
                  OUT PBOOL FoundInTree
                  )
{
    //
    // This was way too slow. Just say we didn't find the file.
    //
    *FoundInTree = FALSE;
    return(TRUE);
}

BOOL
FileExists(
          IN  LPCSTR FileName,
          OUT PWIN32_FIND_DATA FindData
          );

BOOL
SignWithIDWKey(
              IN  LPCSTR  FileName);


CLASS_TABLE CommonClassTable[] = {
    {"retail",  "."},
    {"system",  "system32"},
    {"system16","system"},
    {"windows", "."},
    {"drivers", "system32\\drivers"},
    {"drvetc",  "system32\\drivers\\etc"},
    {"config",  "system32\\config"},
    {NULL,NULL}
};

CLASS_TABLE i386SpecificClassTable[] = {
    {"hal","system32"},
    {"printer","system32\\spool\\drivers\\w32x86"},
    {"prtprocs","system32\\spool\\prtprocs\\w32x86"},
    {NULL,NULL}
};

CLASS_TABLE Axp64SpecificClassTable[] = {
    {"hal",".."},
    {"printer","system32\\spool\\drivers\\w32axp64"},
    {"prtprocs","system32\\spool\\prtprocs\\w32axp64"},
    {NULL,NULL}
};

CLASS_TABLE AlphaSpecificClassTable[] = {
    {"hal",".."},
    {"printer","system32\\spool\\drivers\\w32alpha"},
    {"prtprocs","system32\\spool\\prtprocs\\w32alpha"},
    {NULL,NULL}
};

CLASS_TABLE ia64SpecificClassTable[] = {
    {"hal",".."},
    {"printer","system32\\spool\\drivers\\w32ia64"},
    {"prtprocs","system32\\spool\\prtprocs\\w32ia64"},
    {NULL,NULL}
    };

//
// Names of sections in layout.inx
//
LPCSTR szSourceDisksFiles = "SourceDisksFiles";
LPCSTR szSourceDisksAxp64 = "SourceDisksFiles.axp64";
LPCSTR szSourceDisksAlpha = "SourceDisksFiles.alpha";
LPCSTR szSourceDisksX86   = "SourceDisksFiles.x86";
LPCSTR szSourceDisksIA64  = "SourceDisksFiles.ia64";

typedef struct _PLACE_FILE_RECORD {
    LPSTR FileNameEntry;
    LPSTR FileClass;
} PLACE_FILE_RECORD, *PPLACE_FILE_RECORD;

int MaxNumberOfRecords;
int NumberOfRecords;
PPLACE_FILE_RECORD PlaceFileRecords;

int __cdecl
pfcomp(
      const void *e1,
      const void *e2
      )
{
    PPLACE_FILE_RECORD p1;
    PPLACE_FILE_RECORD p2;

    p1 = (PPLACE_FILE_RECORD)e1;
    p2 = (PPLACE_FILE_RECORD)e2;

    return (strcmp(p1->FileNameEntry,p2->FileNameEntry));
}

CHAR PlaceFileDir[4096];
CHAR PlaceFileClass[4096];
CHAR PlaceFileEntry[4096];

BOOL
SortPlaceFileRecord()
{
    int cfield;
    PPLACE_FILE_RECORD NewRecords;

    NumberOfRecords = 0;
    MaxNumberOfRecords = 0;

    //
    // get space for 6k records. Grow if need to.
    //
    PlaceFileRecords = (PPLACE_FILE_RECORD) malloc( sizeof(*PlaceFileRecords)*7000 );
    if ( !PlaceFileRecords ) {
        return FALSE;
    }
    MaxNumberOfRecords = 7000;

    fseek(PlaceFile,0,SEEK_SET);
    while (fgets(PlaceFileDir,sizeof(PlaceFileDir),PlaceFile)) {

        PlaceFileEntry[0] = '\0';
        PlaceFileClass[0] = '\0';

        cfield = sscanf(
                       PlaceFileDir,
//                       "%s %[A-Za-z0-9.,_!@#\\$+=%^&()~ -]s",
                       "%s %s",
                       PlaceFileEntry,
                       PlaceFileClass
                       );

        if (cfield <= 0 || PlaceFileEntry[0] == ';') {
            continue;
        }

#ifdef _AXP64_
{
    char *volatile p = PlaceFileEntry;
    PlaceFileRecords[NumberOfRecords].FileNameEntry = (LPSTR) malloc( strlen(p)+1 );
    p = PlaceFileClass;
    PlaceFileRecords[NumberOfRecords].FileClass = (LPSTR) malloc( strlen(p)+1 );
}
#else
        PlaceFileRecords[NumberOfRecords].FileNameEntry = (LPSTR) malloc( strlen(PlaceFileEntry)+1 );
        PlaceFileRecords[NumberOfRecords].FileClass = (LPSTR) malloc( strlen(PlaceFileClass)+1 );
#endif
        if (!PlaceFileRecords[NumberOfRecords].FileClass || !PlaceFileRecords[NumberOfRecords].FileNameEntry) {
            free(PlaceFileRecords);
            PlaceFileRecords = NULL;
            return FALSE;
        }
        strcpy(PlaceFileRecords[NumberOfRecords].FileNameEntry,PlaceFileEntry);
        strcpy(PlaceFileRecords[NumberOfRecords].FileClass,PlaceFileClass);
        NumberOfRecords++;
        if ( NumberOfRecords >= MaxNumberOfRecords ) {
            MaxNumberOfRecords += 200;
            NewRecords = (PPLACE_FILE_RECORD) realloc(
                                                     PlaceFileRecords,
                                                     sizeof(*PlaceFileRecords)*MaxNumberOfRecords
                                                     );
            if ( !NewRecords ) {
                PlaceFileRecords = NULL;
                return FALSE;
            }
            PlaceFileRecords = NewRecords;
        }
    }
    qsort((void *)PlaceFileRecords,(size_t)NumberOfRecords,(size_t)sizeof(*PlaceFileRecords),pfcomp);
    return TRUE;
}

PPLACE_FILE_RECORD
LookupPlaceFileRecord(
                     LPSTR FileName
                     )
{
    LONG High;
    LONG Low;
    LONG Middle;
    LONG Result;

    //
    // Lookup the name using a binary search.
    //

    if ( !PlaceFileRecords ) {
        return NULL;
    }

    Low = 0;
    High = NumberOfRecords - 1;
    while (High >= Low) {

        //
        // Compute the next probe index and compare the import name
        // with the export name entry.
        //

        Middle = (Low + High) >> 1;
        Result = _stricmp(FileName, PlaceFileRecords[Middle].FileNameEntry);

        if (Result < 0) {
            High = Middle - 1;

        } else if (Result > 0) {
            Low = Middle + 1;

        } else {
            break;
        }
    }

    if (High < Low) {
        return NULL;
    } else {
        return &PlaceFileRecords[Middle];
    }
}

int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{
    char c, *p, *OverrideFlags, *s, **newargv;
    LPSTR LogFileName = NULL;
    LPSTR LcFileName  = NULL;
    int len = 0;
    int i, n;
    BOOL NoPrivateSplit = FALSE;
    OSVERSIONINFO VersionInformation;
    LPTSTR platform;

    //
    // Win 95 can't compare file times very well, this hack neuters the SourceIsNewer function
    // on Win 95
    //
    VersionInformation.dwOSVersionInfoSize = sizeof( VersionInformation );
    if (GetVersionEx( &VersionInformation ) && VersionInformation.dwPlatformId != VER_PLATFORM_WIN32_NT) {
        fPatheticOS = TRUE;
    }

    envp;
    fUpDriver = FALSE;
    fUsage = FALSE;
    fVerbose = FALSE;
    fSymChecking = FALSE;
    fTestMode = FALSE;
    fSplitSymbols = FALSE;
    fSetupMode = FALSE;
    fSetupModeAllFiles = FALSE;
    fSetupModeScriptFile = FALSE;
    fLiveSystem = FALSE;
    fKeepAttributes = FALSE;
    fDigitalSign = FALSE;
    fHardLinks = FALSE;
    fIgnoreHardLinks = FALSE;
    fDontExit = FALSE;
    fForcePlace = FALSE;
    fSignCode = FALSE;
    fVerifyLc = FALSE;
    NormalPlaceSubdir = NULL;
    pVerifyLocConstraintA = NULL;

    if (argc < 2) {
        goto showUsage;
    }

    LayoutInfName = NULL;

    setvbuf(stderr, NULL, _IONBF, 0);
    setvbuf(stdout, NULL, _IONBF, 0);

    if (!(PlaceFileName = getenv( "BINPLACE_PLACEFILE" ))) {
        if ((PlaceFileName = getenv("_NTDRIVE")) == NULL) {
            PlaceFileName = DEFAULT_NTDRIVE;
        }
        strcpy((PCHAR) PlaceFilePath, PlaceFileName);
        if ((PlaceFileName = getenv("_NTROOT")) == NULL) {
            PlaceFileName = DEFAULT_NTROOT;
        }
        strcat((PCHAR) PlaceFilePath, PlaceFileName);
        strcat((PCHAR) PlaceFilePath, DEFAULT_PLACE_FILE);
        PlaceFileName = (PCHAR) PlaceFilePath;
    }

    if (!(ExcludeFileName = getenv( "BINPLACE_EXCLUDE_FILE" ))) {
        if ((ExcludeFileName = getenv("_NTDRIVE")) == NULL) {
            ExcludeFileName = DEFAULT_NTDRIVE;
        }
        strcpy((PCHAR) ExcludeFilePath, ExcludeFileName);
        if ((ExcludeFileName = getenv("_NTROOT")) == NULL) {
            ExcludeFileName = DEFAULT_NTROOT;
        }
        strcat((PCHAR) ExcludeFilePath, ExcludeFileName);
        strcat((PCHAR) ExcludeFilePath, DEFAULT_EXCLUDE_FILE);
        ExcludeFileName = (PCHAR) ExcludeFilePath;
    }

    if (!(BinplaceLcDir = getenv( "BINPLACE_LCDIR" ))) {
        BinplaceLcDir = DEFAULT_LCDIR;
    }

    if ( getenv("NT_SIGNCODE") != NULL ) {
        fSignCode=TRUE;
    }

    //
    // Support Cross compile as well
    //
#if defined(_AXP64_)
    PlaceRootName = getenv( "_NTAXP64TREE" );
#elif defined(_ALPHA_)
    PlaceRootName = getenv( "_NTALPHATREE" );
    if ((platform = getenv("AXP64")) != NULL) {
        PlaceRootName = getenv( "_NTAXP64TREE" );
    }
#elif defined(_IA64_)
    PlaceRootName = getenv( "_NTIA64TREE" );
#else // defined(_X86_)
    PlaceRootName = getenv( "_NT386TREE" );
    if ((platform = getenv("IA64")) != NULL) {
        PlaceRootName = getenv( "_NTIA64TREE" );
    }
#endif

    CurrentImageName = NULL;

    OverrideFlags = getenv( "BINPLACE_OVERRIDE_FLAGS" );
    if (OverrideFlags != NULL) {
        s = OverrideFlags;
        n = 0;
        while (*s) {
            while (*s && *s <= ' ')
                s += 1;
            if (*s) {
                n += 1;
                while (*s > ' ')
                    s += 1;

                if (*s)
                    *s++ = '\0';
            }
        }

        if (n) {
            newargv = malloc( (argc + n + 1) * sizeof( char * ) );
            memcpy( &newargv[n], argv, argc * sizeof( char * ) );
            argv = newargv;
            argv[ 0 ] = argv[ n ];
            argc += n;
            s = OverrideFlags;
            for (i=1; i<=n; i++) {
                while (*s && *s <= ' ')
                    s += 1;
                argv[ i ] = s;
                while (*s++)
                    ;
            }
        }
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
                        SplitFlags |= SPLITSYM_EXTRACT_ALL;
                        break;

                    case 'B':
                        argc--, argv++;
                        NormalPlaceSubdir = *argv;
                        break;

                    case 'C':
                        fDigitalSign = TRUE;
                        break;

                    case 'D':
                        argc--, argv++;
                        DumpOverride = *argv;
                        break;

                    case 'E':
                        fDontExit = TRUE;
                        break;

                    case 'F':
                        fForcePlace = TRUE;
                        break;

                    case 'G':
                        argc--, argv++;
                        LcFileName = *argv;
                        break;

                    case 'H':
                        if ((VersionInformation.dwPlatformId != VER_PLATFORM_WIN32_NT) ||
                            (VersionInformation.dwMajorVersion < 5) ||
                            (pCreateHardLinkA = (PCREATEHARDLINKA)GetProcAddress( GetModuleHandle( "KERNEL32" ),
                                                                                  "CreateHardLinkA"
                                                                                )
                            ) == NULL
                           ) {
                            fprintf( stderr, "BINPLACE: Hard links not supported.  Defaulting to CopyFile\n" );
                            fHardLinks = FALSE;
                        } else {
                            fHardLinks = TRUE;
                        }

                        break;

                    case 'I':
                        argc--, argv++;
                        LayoutInfName = *argv;
                        break;

                    case 'J':
                        fSymChecking = TRUE;
                        break;

                    case 'K':
                        fKeepAttributes = TRUE;
                        break;

                    case 'L':
                        fLiveSystem++;
                        break;

                    case 'M':
                        fMakeErrorOnDumpCopy = TRUE;
                        break;

                    case 'N':
                        argc--, argv++;
                        PrivateSymbolFilePath = *argv;
                        break;

                    case 'O':
                        argc--, argv++;
                        if (PlaceRootName != NULL) {
                            strcpy(szAltPlaceRoot,PlaceRootName);
                            strcat(szAltPlaceRoot,"\\");
                            strcat(szAltPlaceRoot,*argv);
                            PlaceRootName = szAltPlaceRoot;
                        }
                        break;

                    case 'P':
                        argc--, argv++;
                        PlaceFileName = *argv;
                        break;

                    case 'Q':
                        fDontLog = TRUE;
                        break;

                    case 'R':
                        argc--, argv++;
                        PlaceRootName = *argv;
                        break;

                    case 'S':
                        argc--, argv++;
                        SymbolFilePath = *argv;
                        fSplitSymbols = TRUE;
                        fIgnoreHardLinks = TRUE;
                        break;

                    case 'T':
                        fTestMode = TRUE;
                        break;

                    case 'U':
                        fUpDriver = TRUE;
                        break;

                    case 'V':
                        fVerbose = TRUE;
                        break;

                    case 'W':
                        fPlaceWin95SymFile = TRUE;
                        break;

                    case 'X':
                        SplitFlags |= SPLITSYM_REMOVE_PRIVATE;
                        break;

                    case 'Y':
                        fNoClassInSymbolsDir = TRUE;
                        break;

                    case 'Z':
                        NoPrivateSplit = TRUE;
                        break;

                    case '!':
                        hSetupApi = LoadLibrary("setupapi.dll");
                        if (hSetupApi) {
                            (VOID *) pSetupGetIntField     = GetProcAddress(hSetupApi, "SetupGetIntField");
                            (VOID *) pSetupFindFirstLineA  = GetProcAddress(hSetupApi, "SetupFindFirstLineA");
                            (VOID *) pSetupGetStringFieldA = GetProcAddress(hSetupApi, "SetupGetStringFieldA");
                            (VOID *) pSetupOpenInfFileA    = GetProcAddress(hSetupApi, "SetupOpenInfFileA");
                            (VOID *) pSetupOpenMasterInf   = GetProcAddress(hSetupApi, "SetupOpenMasterInf");

                            if (pSetupGetIntField     &&
                                pSetupFindFirstLineA  &&
                                pSetupGetStringFieldA &&
                                pSetupOpenInfFileA    &&
                                pSetupOpenMasterInf) {
                                fSetupMode = TRUE;
                            } else {
                                printf("Unable to bind to the necessary SETUPAPI.DLL functions... Ignoring setup mode switch\n");
                            }
                        }

                        if (*(p+1) == '!') {
                            p++;
                            if (fSetupMode)
                                fSetupModeAllFiles = TRUE;

                            if (*(p+1) == '!') {
                                p++;
                                argc--, argv++;
                                CommandScriptName = *argv;
                                if (fSetupMode) {
                                    fSetupModeScriptFile = TRUE;
                                    CommandScriptFile = fopen(CommandScriptName, "a");
                                    if (!CommandScriptFile) {
                                        fprintf(stderr,"BINPLACE : fatal error BNP0000: fopen of script file %s failed %d\n",CommandScriptName,GetLastError());
                                        exit(BINPLACE_ERR);
                                    }
                                }
                            } else {
                                fIgnoreHardLinks = TRUE;
                            }
                        } else {
                            fIgnoreHardLinks = TRUE;
                        }
                        break;

                    default:
                        fprintf( stderr, "BINPLACE : error BNP0000: Invalid switch - /%c\n", c );
                        fUsage = TRUE;
                        break;
                }

            if ( fUsage ) {
                showUsage:
                fputs(
                     "usage: binplace [switches] image-names... \n"
                     "where: [-?] display this message\n"
                     "       [-a] Used with -s, extract all symbols\n"
                     "       [-b subdir] put file in subdirectory of normal place\n"
                     "       [-c] digitally sign image with IDW key\n"
                     "       [-d dump-override]\n"
                     "       [-e] don't exit if a file in list could not be binplaced\n"
                     "       [-f] force placement by disregarding file timestamps\n"
                     "       [-g lc-file] verify image with localization constraint file\n"
                     "       [-h] modifies behavior to use hard links instead of CopyFile.\n"
                     "            (ignored if -s, -! or -!! is present)\n"
                     "       [-i layout-inf] Used with -!, -!! or -!!!, override master inf location\n"
                     "       [-j] verify proper symbols exist before copying\n"
                     "       [-k] keep attributes (don't turn off archive)\n"
                     "       [-l] operate over a live system\n"
                     "       [-n <Path>] Used with -x - Private pdb symbol path\n"
                     "       [-o place-root-subdir] alternate project subdirectory\n"
                     "       [-p place-file]\n"
                     "       [-q] suppress writing to log file %BINPLACE_LOG%\n"
                     "       [-r place-root]\n"
                     "       [-s Symbol file path] split symbols from image files\n"
                     "       [-t] test mode\n"
                     "       [-u] UP driver\n"
                     "       [-v] verbose output\n"
                     "       [-w] copy the Win95 Sym file to the symbols tree\n"
                     "       [-x] Used with -s, delete private symbolic when splitting\n"
                     "       [-y] Used with -s, don't create class subdirs in the symbols tree\n"
                     "       [-z] ignore -x if present\n"
                     "       [-!] setup mode (ignore optional files)\n"
                     "       [-!!] setup mode (copy optional files)\n"
                     "       [-!!! script-file] setup mode with command script\n"
                     "\n"
                     "BINPLACE looks for the following environment variable names:\n"
                     "   BINPLACE_OVERRIDE_FLAGS - may contain additional switches\n"
                     "   BINPLACE_PLACEFILE - default value for -p flag\n"
                     "   _NT386TREE - default value for -r flag on x86 platform\n"
                     "   _NTALPHATREE - default value for -r flag on Alpha platform\n"
                     "   _NTAXP64TREE - default value for -r flag on Axp64 platform\n"
                     "   _NTIA64TREE - default value for -r flag on IA64 platform\n"
                     "\n"
                     ,stderr
                     );

                exit(BINPLACE_ERR);
            }
        } else {
            WIN32_FIND_DATA FindData;
            HANDLE h;

            if (!PlaceRootName) {
                // If there's no root, just exit.
                exit(BINPLACE_OK);
            }

            //
            // Workaround for bogus setargv: ignore directories
            //
            if (NoPrivateSplit) {
                SplitFlags &= ~SPLITSYM_REMOVE_PRIVATE;
            }

            h = FindFirstFile(p,&FindData);
            if (h != INVALID_HANDLE_VALUE) {
                FindClose(h);
                if (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                    if ( fVerbose ) {
                        fprintf(stdout,"BINPLACE : warning BNP0000: ignoring directory %s\n",p);
                    }
                    continue;
                }
            }

            CurrentImageName = p;

            //
            // If the master place file has not been opened, open
            // it up.
            //

            if ( !PlaceFile ) {
                PlaceFile = fopen(PlaceFileName, "rt");
                if (!PlaceFile) {
                    fprintf(stderr,"BINPLACE : fatal error BNP0000: fopen of placefile %s failed %d\n",PlaceFileName,GetLastError());
                    exit(BINPLACE_ERR);
                }
                if (fSetupMode && !fSetupModeScriptFile) {
                    SortPlaceFileRecord();
                }
            }

            //
            // Check for bogus -g lc-file switch
            //
            if ( LcFileName != NULL ) {
                h = FindFirstFile(LcFileName, &FindData);
                if (h == INVALID_HANDLE_VALUE ||
                    (FindData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
                    if (fVerbose ) {
                        fprintf(stdout,"BINPLACE : warning BNP0000: invalid file %s. Ignoring -G switch.\n", LcFileName);
                    }
                    LcFileName = NULL;
                }
                if (h != INVALID_HANDLE_VALUE) {
                    FindClose(h);
                }
            }
            if ( LcFileName != NULL ) {
                DWORD cb = GetFullPathName(LcFileName,MAX_PATH+1,LcFullFileName,&LcFilePart);
                if (!cb || cb > MAX_PATH+1) {
                    fprintf(stderr,"BINPLACE : fatal error BNP0000: GetFullPathName %s failed %d\n",LcFileName, GetLastError());
                    exit(BINPLACE_ERR);
                }

                hLcManager = LoadLibraryA("lcman.DLL");
                if (hLcManager != NULL) {
                    (VOID *) pVerifyLocConstraintA = GetProcAddress(hLcManager, "VerifyLocConstraintA");
                }
                if (pVerifyLocConstraintA != NULL) {
                    fVerifyLc = TRUE;
                } else {
                    fprintf(stdout,"BINPLACE : warning BNP0000: Unable to bind to the necessary LCMAN.DLL functions... Ignoring -G switch\n");
                }
            }

            // Get the Exclude List

            ExcludeList = GetExcludeList( ExcludeFileName );

            //
            // If the log file has not been opened,
            // and we haven't suppressed logging, open it up
            //

            if ( !LogFile && !fDontLog) {
                if ((LogFileName = getenv("BINPLACE_LOG")) != NULL) {
                    LogFile = _fsopen(LogFileName, "a", _SH_DENYNO);
                    if ( !LogFile ) {
                        fprintf(stderr,"BINPLACE : error BNP0000: fopen of log file %s failed %d\n", LogFileName,GetLastError());
                    } else {
                        time_t Time;
                        FILE *fSlmIni;
                        UCHAR szProject[MAX_PATH];
                        UCHAR szSlmServer[MAX_PATH];
                        UCHAR szEnlistment[MAX_PATH];
                        UCHAR szSlmDir[MAX_PATH];
                        UCHAR *szTime="";
                        // Get some other interesting info.
                        fSlmIni = fopen("slm.ini", "r");
                        if (fSlmIni) {
                            fgets(szProject, sizeof(szProject), fSlmIni);
                            fgets(szSlmServer, sizeof(szSlmServer), fSlmIni);
                            fgets(szEnlistment, sizeof(szEnlistment), fSlmIni);
                            fgets(szSlmDir, sizeof(szSlmDir), fSlmIni);
                            // Get rid of the trailing newlines
                            szProject[strlen(szProject)-1] = '\0';
                            szSlmServer[strlen(szSlmServer)-1] = '\0';
                            szSlmDir[strlen(szSlmDir)-1] = '\0';
                            fclose(fSlmIni);
                        } else {
                            szSlmServer[0] = '\0';
                            szProject[0] = '\0';
                            szSlmDir[0] = '\0';
                        }
                        Time = time(NULL);
                        szTime = ctime(&Time);
                        sprintf(szExtraInfo,
                                "%s\t%s\t%s\t%s",
                                szSlmServer,
                                szProject,
                                szSlmDir,
                                szTime);
                    }
                }
            }

            if (PlaceRootName == NULL) {
                fprintf(stderr,"BINPLACE : fatal error BNP0000: Place Root not defined - exiting.\n");
                exit(BINPLACE_ERR);
            }

            // If the SymbolFilePath has not been set, make a default value.
            if (!SymbolFilePath) {
                strcpy(DefaultSymbolFilePath, PlaceRootName);
                strcat(DefaultSymbolFilePath, "\\symbols");
                SymbolFilePath = DefaultSymbolFilePath;
            }

            if ( !PlaceTheFile() ) {
                if (fDontExit) {
                    fprintf(stderr,"BINPLACE : error BNP0000: Unable to place file %s.\n",CurrentImageName);
                } else {
                    fprintf(stderr,"BINPLACE : fatal error BNP0000: Unable to place file %s - exiting.\n",CurrentImageName);
                    exit(BINPLACE_ERR);
                }
            } else {
                if ( LogFile ) {
                    len = fprintf(LogFile,"%s\t%s",gFullFileName,szExtraInfo);
                    if ( len < 0 ) {
                        fprintf(stderr,"BINPLACE : error BNP0000: write to log file %s failed %d\n", LogFileName, GetLastError());
                    }
                }
            }
        }
    }
    exit(BINPLACE_OK);
    return BINPLACE_OK;
}

BOOL
PlaceTheFile()
{
    CHAR FullFileName[MAX_PATH+1];
    LPSTR PlaceFileNewName;
    LPSTR FilePart;
    LPSTR Separator;
    LPSTR PlaceFileClassPart;
    DWORD cb;
    int cfield;
    PCLASS_TABLE ClassTablePointer;
    BOOLEAN ClassMatch;
    BOOL    fCopyResult;
    LPSTR Extension;
    BOOL PutInDump;
    BOOL PutInDebug = FALSE;
    BOOL PutInLcDir = FALSE;

    cb = GetFullPathName(CurrentImageName,MAX_PATH+1,FullFileName,&FilePart);

    if (!cb || cb > MAX_PATH+1) {
        fprintf(stderr,"BINPLACE : fatal error BNP0000: GetFullPathName failed %d\n",GetLastError());
        return FALSE;
    }

    if (LogFile) {
        strcpy(gFullFileName,FullFileName);
    }

    if (fVerbose) {
        fprintf(stdout,"BINPLACE : warning BNP0000: Looking at file %s\n",FilePart);
    }

    Extension = strrchr(FilePart,'.');
    if (Extension) {
        if (!_stricmp(Extension,".DBG")) {
            PutInDebug = TRUE;
        }
        else if (!_stricmp(Extension,".LC")) {
            PutInLcDir = TRUE;
        }
    }

    if (!DumpOverride) {

        if (fSetupMode && !fSetupModeScriptFile) {
            PPLACE_FILE_RECORD PfRec;

            PfRec = LookupPlaceFileRecord(FilePart);

            if ( PfRec ) {
                strncpy(PlaceFileEntry,PfRec->FileNameEntry, sizeof(PlaceFileEntry));
                strncpy(PlaceFileClass,PfRec->FileClass, sizeof(PlaceFileClass));
                PlaceFileNewName = NULL;
                goto fastfound;
            }
        }
        fseek(PlaceFile,0,SEEK_SET);
        while (fgets(PlaceFileDir,sizeof(PlaceFileDir),PlaceFile)) {

            PlaceFileEntry[0] = '\0';
            PlaceFileClass[0] = '\0';

            cfield = sscanf(
                           PlaceFileDir,
//                           "%s %[A-Za-z0-9.,_!@#\\$+=%^&()~ -]s",
                           "%s %s",
                           PlaceFileEntry,
                           PlaceFileClass
                           );

            if (cfield <= 0 || PlaceFileEntry[0] == ';') {
                continue;
            }

            if (PlaceFileNewName = strchr(PlaceFileEntry,'!')) {
                *PlaceFileNewName++ = '\0';
            }

            if (!_stricmp(FilePart,PlaceFileEntry)) {
                fastfound:
                //
                // now that we have the file and class, search the
                // class tables for the directory.
                //
                Separator = PlaceFileClass - 1;
                while (Separator) {

                    PlaceFileClassPart = Separator+1;
                    Separator = strchr(PlaceFileClassPart,':');
                    if (Separator) {
                        *Separator = '\0';
                    }

                    //
                    // If the class is "retail" and we're in Setup mode,
                    // handle this file specially. Setup mode is used to
                    // incrementally binplace files into an existing installation.
                    //
                    SetupFilePath[0] = '\0';
                    if (fSetupMode && !_stricmp(PlaceFileClassPart,"retail")) {
                        if (SetupModeRetailFile(FullFileName,FilePart,&PutInDump)) {
                            //
                            // No error. Either the file was handled or we need to
                            // put it in the dump directory.
                            //
                            if (PutInDump) {
                                fCopyResult = CopyTheFile(
                                                         FullFileName,
                                                         FilePart,
                                                         (DumpOverride ? DumpOverride : DEFAULT_DUMP),
                                                         NULL
                                                         );
                            } else {
                                fCopyResult = TRUE;
                            }
                        } else {
                            //
                            // Got an error, return error status.
                            //
                            fCopyResult = FALSE;
                        }

                        if (!fSetupModeScriptFile) {
                            return(fCopyResult);
                        }
                    }

                    PlaceFileDir[0]='\0';
                    ClassMatch = FALSE;
                    ClassTablePointer = &CommonClassTable[0];
                    while (ClassTablePointer->ClassName) {
                        if (!_stricmp(ClassTablePointer->ClassName,PlaceFileClassPart)) {
                            strcpy(PlaceFileDir,ClassTablePointer->ClassLocation);
                            ClassMatch = TRUE;

                            //
                            // If the class is a driver and a UP driver is
                            // specified, then put the driver in the UP
                            // subdirectory.
                            //
                            // Do the same for retail. We assume the -u switch is passed
                            // only when actually needed.
                            //
                            if (fUpDriver
                                && (   !_stricmp(PlaceFileClass,"drivers")
                                       || !_stricmp(PlaceFileClass,"retail"))) {
                                strcat(PlaceFileDir,"\\up");
                            }
                            break;
                        }

                        ClassTablePointer++;
                    }

                    if (!ClassMatch) {
                        //
                        // Search Specific classes
                        //
                        // We need to support cross compiling here.
                        LPTSTR platform;

#if   defined(_AXP64_)
                        ClassTablePointer = &Axp64SpecificClassTable[0];
#elif defined(_ALPHA_)
                        ClassTablePointer = &AlphaSpecificClassTable[0];
                        if ((platform = getenv("AXP64")) != NULL) {
                            ClassTablePointer = &Axp64SpecificClassTable[0];
                        }
#elif defined(_IA64_)
                        ClassTablePointer = &ia64SpecificClassTable[0];
#else // defined(_X86_)
                        ClassTablePointer = &i386SpecificClassTable[0];
                        if ((platform = getenv("IA64")) != NULL) {
                            ClassTablePointer = &ia64SpecificClassTable[0];
                        }
#endif
                        while (ClassTablePointer->ClassName) {

                            if (!_stricmp(ClassTablePointer->ClassName,PlaceFileClassPart)) {
                                strcpy(PlaceFileDir,ClassTablePointer->ClassLocation);
                                ClassMatch = TRUE;
                                break;
                            }

                            ClassTablePointer++;
                        }
                    }

                    if (!ClassMatch) {

                        char * asterisk;

                        //
                        // Still not found in class table. Use the class as the
                        // directory
                        //

                        if ( fVerbose ) {
                            fprintf(stderr,"BINPLACE : warning BNP0000: Class %s Not found in Class Tables\n",PlaceFileClassPart);
                        }
                        if ( asterisk = strchr( PlaceFileClassPart, '*')) {
                            //
                            // Expand * to platform
                            //
                            LPTSTR platform;
                            ULONG PlatformSize;
                            LPTSTR PlatformPath;

#if   defined(_AXP64_)
                            PlatformSize = 5;
                            PlatformPath = TEXT("axp64");
#elif defined(_ALPHA_)
                            PlatformSize = 5;
                            PlatformPath = TEXT("alpha");
                            if ((platform = getenv("AXP64")) != NULL) {
                                PlatformPath = TEXT("axp64");
                            }
#elif defined(_IA64_)
                            PlatformSize = 4;
                            PlatformPath = TEXT("ia64");
#else // defined(_X86_)
                            PlatformSize = 4;
                            PlatformPath = TEXT("i386");
                            if ((platform = getenv("IA64")) != NULL) {
                                PlatformPath = TEXT("ia64");
                            }
#endif

                            strncpy(PlaceFileDir,PlaceFileClassPart, (int)(asterisk - PlaceFileClassPart));
                            strcpy(PlaceFileDir + (asterisk - PlaceFileClassPart), PlatformPath);
                            strcpy(PlaceFileDir + (asterisk - PlaceFileClassPart) + PlatformSize, asterisk + 1);

                        } else {
                            strcpy(PlaceFileDir,PlaceFileClassPart);
                        }
                    }

                    if (SetupFilePath[0] == '\0') {
                        lstrcpy(SetupFilePath, PlaceFileDir);
                        lstrcat(SetupFilePath, "\\");
                        lstrcat(SetupFilePath, FilePart);
                    }

                    if (NormalPlaceSubdir) {
                        strcat(PlaceFileDir,"\\");
                        strcat(PlaceFileDir,NormalPlaceSubdir);
                    }

                    fCopyResult = CopyTheFile(FullFileName,FilePart,PlaceFileDir,PlaceFileNewName);
                    if (!fCopyResult) {
                        break;
                    }
                }

                return(fCopyResult);
            }
        }
    }

    if (fMakeErrorOnDumpCopy) {
        fprintf(stderr, "BINPLACE : error BNP0000: File '%s' is not listed in '%s'. Copying to dump.\n", FullFileName, PlaceFileName);
    }

    return CopyTheFile(
               FullFileName,
               FilePart,
               PutInDebug ? "Symbols" : (PutInLcDir ? BinplaceLcDir : (DumpOverride ? DumpOverride : DEFAULT_DUMP)),
               NULL
               );

}

BOOL
CopyTheFile(
           LPSTR SourceFileName,
           LPSTR SourceFilePart,
           LPSTR DestinationSubdir,
           LPSTR DestinationFilePart
           )
{
    CHAR DestinationFile[MAX_PATH+1];
    CHAR TmpDestinationFile[MAX_PATH];
    CHAR TmpDestinationDir[MAX_PATH];
    CHAR DestinationLcFile[MAX_PATH+1];
    char Drive[_MAX_DRIVE];
    char Dir[_MAX_DIR];
    char Ext[_MAX_EXT];
    char Name[_MAX_FNAME];
    char TmpName[_MAX_FNAME];
    char TmpPath[_MAX_PATH];
    char FileSystemType[8];
    char DriveRoot[4];
    CHAR *TmpSymbolFilePath;
    DWORD dwFileSystemFlags;
    DWORD dwMaxCompLength;
    CHAR ErrMsg[MAX_SYM_ERR];
    BOOL fBinplaceLc;

    if ( !PlaceRootName ) {
        fprintf(stderr,"BINPLACE : warning BNP0000: PlaceRoot is not specified\n");
        return FALSE;
    }

    //
    // We also neuter SourceIsNewer on FAT partitions since they have a 2 second
    // file time granularity
    //
    _splitpath(SourceFileName, DriveRoot, Dir, NULL, NULL);
    lstrcat(DriveRoot, "\\");
    GetVolumeInformation(DriveRoot, NULL, 0, NULL, &dwMaxCompLength, &dwFileSystemFlags, FileSystemType, 7);
    if (lstrcmpi(FileSystemType, "FAT") == 0 || lstrcmpi(FileSystemType, "FAT32") == 0)
        fPatheticOS = TRUE;

    strcpy(DestinationFile,PlaceRootName);
    strcat(DestinationFile,"\\");
    strcat(DestinationFile,DestinationSubdir);
    strcat(DestinationFile,"\\");

    strcpy (TmpDestinationDir, DestinationFile);


    if (!MakeSureDirectoryPathExists(DestinationFile)) {
        fprintf(stderr, "BINPLACE : error BNP0000: Unable to create directory path '%s' (%u)\n",
                DestinationFile, GetLastError()
               );
    }

    if (DestinationFilePart) {
        strcat(DestinationFile,DestinationFilePart);
    } else {
        strcat(DestinationFile,SourceFilePart);
    }

    if (!fSetupMode && (fVerbose || fTestMode)) {
        fprintf(stdout,"BINPLACE : warning BNP0000: place %s in %s\n",SourceFileName,DestinationFile);
    }

    if (!fSetupMode) {
        BOOL fRetail = (*DestinationSubdir == '.') && (*(DestinationSubdir+1) == '\0');
        if (SourceIsNewer(SourceFileName,DestinationFile)) {
            fprintf(stdout, "binplace %s\n", SourceFileName);
            if (!VerifyFinalImage(SourceFileName, fRetail, &fBinplaceLc))
                return FALSE;

            // Verify Symbols
            if (fRetail && fSymChecking && !fSignCode) {
                _splitpath(SourceFileName,Drive, Dir, Name, Ext );
                strcpy(TmpName,Name);
                strcat(TmpName,Ext);
                strcpy(TmpPath,Drive);
                strcat(TmpPath,Dir);
                if (!CheckSymbols(ErrMsg, TmpPath,SourceFileName, NULL,
                                  IGNORE_IF_SPLIT,FALSE) ) {
                    if ( !InExcludeList(TmpName,ExcludeList) ) {
                        fprintf(stderr,"BINPLACE : error BNP0000: %s",ErrMsg);
                        return FALSE;
                    } else {
                        fprintf(stdout,"BINPLACE : warning BNP0000: %s",ErrMsg);
                    }
                }
            }
        }
    }

    if (!fTestMode) {
        //
        // In Setup mode, copy the file only if it's newer than
        // the one that's already there.
        //
        if (!fSetupModeScriptFile) {
            if (SourceIsNewer(SourceFileName,DestinationFile)) {
                if (fVerbose) {
                    fprintf(stdout,"BINPLACE : warning BNP0000: copy %s to %s\n",SourceFileName,DestinationFile);
                }
            } else {
                return(TRUE);
            }
        }

        SetFileAttributes(DestinationFile,FILE_ATTRIBUTE_NORMAL);

        if (fSetupModeScriptFile) {
            fprintf( CommandScriptFile, "%s %s\n", DestinationFile, SetupFilePath );
        }

        if (!fIgnoreHardLinks && fHardLinks) {
            if ((*pCreateHardLinkA)(SourceFileName, DestinationFile, NULL)) {
                if (!fKeepAttributes)
                    SetFileAttributes(DestinationFile,FILE_ATTRIBUTE_NORMAL);
                return(TRUE);
            }
        }

        if ( !CopyFile(SourceFileName,DestinationFile, FALSE)) {
            fprintf(stderr,"BINPLACE : warning BNP0000: CopyFile(%s,%s) failed %d\n",SourceFileName,DestinationFile,GetLastError());

            if (!fLiveSystem) {
                return FALSE;
            }

            //  If CopyFile failed and we are instructed to do this over a live
            //  system, attempt to do a safe copy

            if (GetTempFileName (TmpDestinationDir, "bin", 0, TmpDestinationFile) == 0) {
                fprintf (stderr, "BINPLACE : error BNP0000: GetTempFileName (%s, %s) failed - %d\n",
                         DestinationSubdir, TmpDestinationFile, GetLastError ());
                return FALSE;
            }

            if (fVerbose) {
                fprintf (stdout, "BINPLACE : warning BNP0000: temp file name is %s\n", TmpDestinationFile);
            }

            //  rename target file to temp file
            if (!MoveFileEx (DestinationFile, TmpDestinationFile, MOVEFILE_REPLACE_EXISTING)) {
                //  Move failed, get rid of temp file
                ULONG error = GetLastError ();
                if (fVerbose) {
                    fprintf (stdout, "BINPLACE : error BNP0000: MoveFileEx (%s, %s) failed %d",
                             DestinationFile, TmpDestinationFile, error);
                }
                DeleteFile (TmpDestinationFile);
                SetLastError (error);
                return FALSE;
            }

            //  copy again
            if (!CopyFile (SourceFileName, DestinationFile, TRUE)) {
                //  Copy failed.  Delete the destination (perhaps due to out of space
                //  and replace original destination)
                ULONG error = GetLastError ();
                if (fVerbose) {
                    fprintf (stdout, "BINPLACE : error BNP0000: CopyFile (%s, %s) failed %d",
                             SourceFileName, DestinationFile, error);
                }
                DeleteFile (DestinationFile);
                MoveFile (TmpDestinationFile, DestinationFile);
                SetLastError (error);
                return FALSE;
            }

            //  mark temp for delete
            if (!MoveFileEx (TmpDestinationFile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT)) {
                //  Could not make old file for deletion.  Delete destination
                //  and replace original destination)
                ULONG error = GetLastError ();
                if (fVerbose) {
                    fprintf (stdout, "BINPLACE : error BNP0000: MoveFileEx (%s, NULL) failed %d",
                             TmpDestinationFile, error);
                }
                DeleteFile (DestinationFile);
                MoveFile (TmpDestinationFile, DestinationFile);
                return FALSE;
            }
        }
        if (fSetupMode && !fSetupModeScriptFile) {
            fprintf(stdout,"%s ==> %s\n",SourceFileName,DestinationFile);
        }

        if (!fKeepAttributes)
            SetFileAttributes(DestinationFile,FILE_ATTRIBUTE_NORMAL);

        if (!fNoClassInSymbolsDir) {
            strcpy(TmpDestinationDir, SymbolFilePath);
            if ((DestinationSubdir[0] == '.') && (DestinationSubdir[1] == '\0')) {
                strcat(TmpDestinationDir, "\\retail");
            } else {
                char * pSubdir;
                char * pTmp;
                strcat(TmpDestinationDir, "\\");

                pSubdir = DestinationSubdir;
		if (pSubdir[0] == '.' && pSubdir[1] == '\\')
		{
		    pSubdir += 2;
		}

                // Put the root dir only on the path
                pTmp = strchr(pSubdir, '\\');
                if (pTmp) {
                    strcpy(TmpDestinationFile, pSubdir);
                    TmpDestinationFile[pTmp - pSubdir] = '\0';
                    strcat(TmpDestinationDir, TmpDestinationFile);
                } else {
                    strcat(TmpDestinationDir, pSubdir);
                }
            }
            TmpSymbolFilePath = TmpDestinationDir;
        } else {
            TmpSymbolFilePath = SymbolFilePath;
        }

        if (fSplitSymbols && !fUpDriver) {
            _splitpath(SourceFileName, Drive, Dir, NULL, Ext);
            _makepath(DebugFilePath, Drive, Dir, NULL, NULL);
            SplitFlags |= SPLITSYM_SYMBOLPATH_IS_SRC;
            if (SplitSymbols( DestinationFile, TmpSymbolFilePath, (PCHAR) DebugFilePath, SplitFlags )) {
                if (fVerbose)
                    fprintf( stdout, "BINPLACE : warning BNP0000: Symbols stripped from %s into %s\n", DestinationFile, DebugFilePath );

                if ((SplitFlags & SPLITSYM_REMOVE_PRIVATE) && (PrivateSymbolFilePath != NULL)) {
                    CHAR Dir1[_MAX_PATH];
                    CHAR Dir2[_MAX_PATH];
                    _splitpath(DebugFilePath, Drive, Dir, NULL, NULL);
                    _makepath(Dir1, Drive, Dir, NULL, NULL);
                    strcpy(Dir2, PrivateSymbolFilePath);
                    strcat(Dir2, Dir1+strlen(SymbolFilePath));
                    MakeSureDirectoryPathExists(Dir2);
                    BinplaceCopyPdb(Dir2, SourceFileName, TRUE, FALSE);
                }
            } else {
                if (fVerbose)
                    fprintf( stdout, "BINPLACE : warning BNP0000: No symbols to strip from %s\n", DestinationFile );
                strcpy(DebugFilePath, TmpSymbolFilePath);
                strcat(DebugFilePath, "\\");
                strcat(DebugFilePath, &Ext[1]);
                strcat(DebugFilePath, "\\");
                BinplaceCopyPdb(DebugFilePath, SourceFileName, TRUE, SplitFlags & SPLITSYM_REMOVE_PRIVATE);
            }

            if ((SplitFlags & SPLITSYM_REMOVE_PRIVATE) && (PrivateSymbolFilePath != NULL)) {
                CHAR Dir1[_MAX_PATH];
                CHAR Dir2[_MAX_PATH];
                _splitpath(DebugFilePath, Drive, Dir, NULL, NULL);
                _makepath(Dir1, Drive, Dir, NULL, NULL);
                strcpy(Dir2, PrivateSymbolFilePath);
                strcat(Dir2, Dir1+strlen(SymbolFilePath));
                MakeSureDirectoryPathExists(Dir2);
                BinplaceCopyPdb(Dir2, SourceFileName, TRUE, FALSE);
            }

        } else {
            BinplaceCopyPdb(DestinationFile, SourceFileName, FALSE, fSplitSymbols ? (SplitFlags & SPLITSYM_REMOVE_PRIVATE) : FALSE);
        }

        if (fPlaceWin95SymFile) {
            char DestSymPath[_MAX_PATH];
            char SrcSymPath[_MAX_PATH];

            _splitpath(CurrentImageName, Drive, Dir, Name, Ext);
            _makepath(SrcSymPath, Drive, Dir, Name, ".sym");

            if (!_access(SrcSymPath, 0)) {
                if (fSplitSymbols) {
                    strcpy(DestSymPath, TmpSymbolFilePath);
                    strcat(DestSymPath, "\\");
                    strcat(DestSymPath, Ext[0] == '.' ? &Ext[1] : Ext);
                    strcat(DestSymPath, "\\");
                    strcat(DestSymPath, Name);
                    strcat(DestSymPath, ".sym");
                } else {
                    _splitpath(DestinationFile, Drive, Dir, NULL, NULL);
                    _makepath(DestSymPath, Drive, Dir, Name, ".sym");
                }

                SetFileAttributes(DestSymPath, FILE_ATTRIBUTE_NORMAL);

                if (SourceIsNewer(SrcSymPath, SourceFileName)) {
                    // Only binplace the .sym file if it was built AFTER the image itself.
                    if (!CopyFile(SrcSymPath, DestSymPath, FALSE)) {
                        fprintf(stderr,"BINPLACE : warning BNP0000: CopyFile(%s,%s) failed %d\n", SrcSymPath, DestSymPath ,GetLastError());
                    }
                }

                if (!fKeepAttributes)
                    SetFileAttributes(DestinationFile,FILE_ATTRIBUTE_NORMAL);
            } else {
                if (fVerbose) {
                    fprintf( stdout, "BINPLACE : warning BNP0000: Unable to locate \"%s\" for \"%s\"\n", SrcSymPath, CurrentImageName );
                }
            }

        }

        if (fDigitalSign) {
            SignWithIDWKey( DestinationFile );
        }

        if (!fSetupMode && fBinplaceLc) {
            strcpy(DestinationLcFile,PlaceRootName);
            strcat(DestinationLcFile,"\\");
            strcat(DestinationLcFile,BinplaceLcDir);
            strcat(DestinationLcFile,"\\");
            strcat(DestinationLcFile,DestinationSubdir);
            strcat(DestinationLcFile,"\\");

            if (!MakeSureDirectoryPathExists(DestinationLcFile)) {
                fprintf(stderr, "BINPLACE : error BNP0000: Unable to create directory path '%s' (%u)\n",
                        DestinationLcFile, GetLastError()
                       );
            }

            strcat(DestinationLcFile, LcFilePart);

            if (!CopyFile(LcFullFileName, DestinationLcFile, FALSE)) {
               fprintf(stderr,"BINPLACE : warning BNP0000: CopyFile(%s,%s) failed %d\n",
                       LcFullFileName,DestinationLcFile,GetLastError());
            }
        }

    } else {
        if (fSetupMode) {
            if (SourceIsNewer(SourceFileName,DestinationFile)) {
                if (fVerbose) {
                    fprintf(stdout,"BINPLACE : warning BNP0000: copy %s to %s\n",SourceFileName,DestinationFile);
                }
            } else {
                return(TRUE);
            }
        }

        if ( fSetupMode ) {
            fprintf(stdout,"%s ==> %s\n",SourceFileName,DestinationFile);
        }
    }

    return TRUE;
}


BOOL VerifyLc(
             PCHAR FileName,
             BOOL  fRetail
             )
{
    HRESULT hr = (*pVerifyLocConstraintA)(FileName, LcFullFileName);

    if (FAILED(hr)) {
        if (hr == HRESULT_FROM_WIN32(ERROR_NO_MATCH)) {
            fprintf(stderr,
                "BINPLACE : %s BNP0000: resource conflicts with localization constraint \"%s\"\n",
                fRetail ? "error" : "warning",
                FileName);
        }
        else {
            fprintf(stderr,
                "BINPLACE : %s BNP0000: VerifyLc %s failed 0x%lX\n",
                fRetail ? "error" : "warning", FileName, hr);
        }
        return FALSE;
    }

    return TRUE;
}


typedef DWORD (WINAPI *PFNGVS)(LPSTR, LPDWORD);

BOOL
VerifyFinalImage(
                IN  PCHAR FileName,
                IN  BOOL  fRetail,
                OUT PBOOL BinplaceLc
                )
{
    HINSTANCE hVersion;
    PFNGVS pfnGetFileVersionInfoSize;
    DWORD dwSize;
    DWORD dwReturn;
    BOOL  fRC = TRUE;

    *BinplaceLc = FALSE;

    if (fVerifyLc) {
        if (!VerifyLc(FileName, fRetail)) {
            fRC = fRetail ? FALSE : TRUE;
            goto End1;
        }
        *BinplaceLc = TRUE;
    }

    hVersion = LoadLibraryA("VERSION.DLL");
    if (hVersion == NULL) {
        goto End1;
    }

    pfnGetFileVersionInfoSize = (PFNGVS) GetProcAddress(hVersion, "GetFileVersionInfoSizeA");
    if (pfnGetFileVersionInfoSize == NULL) {
        goto End2;
    }

    if ((dwReturn = pfnGetFileVersionInfoSize(FileName, &dwSize)) == 0) {

        // See if this is even a binary
            LOADED_IMAGE LoadedImage;
            if (MapAndLoad(FileName, NULL, &LoadedImage, FALSE, TRUE) == FALSE) {
                // Not a binary.  See if it's one of the other types we care about (like typelibs)

                CHAR szExt[_MAX_EXT];

                _splitpath(FileName, NULL, NULL, NULL, szExt);

                if (_stricmp(szExt, ".tlb")) {
                    goto End2;
                }
            } else if ((LoadedImage.FileHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_I386) &&
                (LoadedImage.FileHeader->FileHeader.Machine != IMAGE_FILE_MACHINE_ALPHA)) {
                goto End2;
            }

            fRC = fRetail ? FALSE : TRUE;
            fprintf(stderr,
                    "BINPLACE : %s BNP0000: no version resource detected for \"%s\"\n",
                    fRetail ? "error" : "warning",
                    FileName);

            UnMapAndLoad(&LoadedImage);
    }

End2:
    FreeLibrary(hVersion);
End1:
    return fRC;
}


BOOL
SourceIsNewer(
             IN LPSTR SourceFile,
             IN LPSTR TargetFile
             )
{
    BOOL Newer;
    WIN32_FIND_DATA TargetInfo;
    WIN32_FIND_DATA SourceInfo;

    //
    // If force placement (-f option) was specified, just return TRUE
    // If the target file doesn't exist, then the source is newer.
    // If the source file doesn't exist, just return TRUE and hope
    // the caller will catch it.
    if ((fForcePlace == FALSE) && (FileExists(TargetFile,&TargetInfo) && FileExists(SourceFile,&SourceInfo))) {

        Newer = (fLiveSystem || !fPatheticOS)
                ? (CompareFileTime(&SourceInfo.ftLastWriteTime,&TargetInfo.ftLastWriteTime) > 0)
                : (CompareFileTime(&SourceInfo.ftLastWriteTime,&TargetInfo.ftLastWriteTime) >= 0);

    } else {

        Newer = TRUE;
    }

    return(Newer);
}


BOOL
SetupModeRetailFile(
                   IN  LPSTR FullFileName,
                   IN  LPSTR FileNamePart,
                   OUT PBOOL PutInDump
                   )
{
    BOOL FoundInTree;
    INFCONTEXT InfContext;
    DWORD DontCare;
    INT IntVal;
    CHAR DirSpec[24];
    CHAR Directory[MAX_PATH];
    CHAR Rename[MAX_PATH];
    LPSTR p;

    //
    // Find and update all instances of the file in the target tree.
    //
    *PutInDump = FALSE;
    FoundInTree = FALSE;
    if (!SearchOneDirectory(PlaceRootName,FileNamePart,FullFileName,FileNamePart,&FoundInTree)) {
        return(FALSE);
    }

    if (!FoundInTree) {
        //
        // OK, now things get tricky. Load master layout inf if
        // not already loaded.
        //
        if (!LayoutInf) {
            if (LayoutInfName) {
                //
                // Use GetFullPathName(). Otherwise a name without a dir spec
                // will be assumed to be in %sysroot%\inf, which is probably not
                // what people would expect.
                //
                GetFullPathName(LayoutInfName,MAX_PATH,Directory,&p);
                LayoutInf = (*pSetupOpenInfFileA)(Directory,NULL,INF_STYLE_WIN4,NULL);
            } else {
                LayoutInf = (*pSetupOpenMasterInf)();
            }
            if (LayoutInf == INVALID_HANDLE_VALUE) {

                LayoutInf = NULL;

                fprintf(
                       stderr,
                       "BINPLACE : error BNP0000: Unable to load %s\n",
                       LayoutInfName ? LayoutInfName : "%%sysroot%%\\inf\\layout.inf"
                       );

                return(FALSE);
            }
        }

        //
        // Look up the file in the master inf.
        //
        if (!(*pSetupFindFirstLineA)(LayoutInf,szSourceDisksFiles,FileNamePart,&InfContext)) {

            LPTSTR platform;
            LPCTSTR szSourceDisksFPlat;

#if   defined(_AXP64_)

            szSourceDisksFPlat = &szSourceDisksAxp64[0];

#elif defined(_ALPHA_)

            szSourceDisksFPlat = &szSourceDisksAlpha[0];
            if ((platform = getenv("AXP64")) != NULL) {
                szSourceDisksFPlat = &szSourceDisksAxp64[0];
            }

#elif defined(_IA64_)

            szSourceDisksFPlat = &szSourceDisksIA64[0];

#else // defined(_X86_)

            szSourceDisksFPlat = &szSourceDisksX86[0];
            if ((platform = getenv("IA64")) != NULL) {
                szSourceDisksFPlat = &szSourceDisksIA64[0];
            }
#endif

            if (!(*pSetupFindFirstLineA)(LayoutInf,szSourceDisksFPlat,FileNamePart,&InfContext)) {

                if ( fVerbose ) {
                    fprintf(stderr,"BINPLACE : warning BNP0000: warning: unknown retail file %s\n",FileNamePart);
                }
                *PutInDump = TRUE;
                return(TRUE);
            }
        }

        //
        // See if the file gets renamed in the target tree.
        // If so, try to find the renamed version in the target.
        //
        if ((*pSetupGetStringFieldA)(&InfContext,11,Rename,MAX_PATH,&DontCare)
            && lstrcmpi(Rename,FileNamePart)) {
            FoundInTree = FALSE;
            if (!SearchOneDirectory(PlaceRootName,Rename,FullFileName,FileNamePart,&FoundInTree)) {
                return(FALSE);
            }

            //
            // If we found the renamed file in the target tree, we're done.
            //
            if (FoundInTree) {
                return(TRUE);
            }
        } else {
            //
            // Assume name in target is same as name in source.
            //
            strcpy(Rename,FileNamePart);
        }

        //
        // We couldn't find the file in the target tree.
        // The file might be new. Check the copy disposition for
        // non-upgrades -- if the file is marked "copy always" then we want
        // to copy it. Otherwise ignore the file. This way someone who
        // uses this tool to 'upgrade' a build doesn't get a pile of files
        // they don't need placed into their nt tree.
        //
        // This behavior is overrideable by using -!! instead of -!.
        //
        if (!fSetupModeAllFiles && (!(*pSetupGetIntField)(&InfContext,10,&IntVal) || IntVal)) {
            //
            // File is not marked "copy always" so ignore it, assuming it's
            // configuration-specific and the user doesn't need it.
            //
            return(TRUE);
        }

        //
        // File needs to be copied into the target tree.
        // Get the directory spec.
        //
        DirSpec[0] = 0;
        (*pSetupGetStringFieldA)(&InfContext,8,DirSpec,sizeof(DirSpec),&DontCare);

        if (!(*pSetupFindFirstLineA)(LayoutInf,"WinntDirectories",DirSpec,&InfContext)
            || !(*pSetupGetStringFieldA)(&InfContext,1,Directory,MAX_PATH,&DontCare)) {
            if (strlen(DirSpec)) {
                fprintf(stderr,"BINPLACE : error BNP0000: unknown directory spec %s in layout.inf for file %s\n",DirSpec,FileNamePart);
                return(FALSE);
            } else {
                return(TRUE);
            }
        }

        //
        // If the spec begins with a slash, then for root dir, replace with .
        // otherwise, skip over leading slash in the non-root case.
        //
        if ((Directory[0] == '\\')) {
            if (!Directory[1]) {
                Directory[0] = '.';
            } else {
                lstrcpy(Directory, Directory+1);
            }
        }

        if (fSetupModeScriptFile) {
            lstrcpy(SetupFilePath, Directory);
            lstrcat(SetupFilePath, "\\");
            lstrcat(SetupFilePath, Rename);
            return FALSE;
        }

        //
        // Got what we need -- copy the file.
        //
        return CopyTheFile(
                          FullFileName,
                          FileNamePart,
                          Directory,
                          Rename
                          );
    }

    return(TRUE);
}


BOOL
BinplaceCopyPdb (
                LPSTR DestinationFile,
                LPSTR SourceFileName,
                BOOL CopyFromSourceOnly,
                BOOL StripPrivate
                )
{
    LOADED_IMAGE LoadedImage;
    DWORD DirCnt;
    PIMAGE_DEBUG_DIRECTORY DebugDirs, CvDebugDir;

    if (MapAndLoad(
                   CopyFromSourceOnly ? SourceFileName : DestinationFile,
                   NULL,
                   &LoadedImage,
                   FALSE,
                   CopyFromSourceOnly ? TRUE : FALSE) == FALSE) {
        return (FALSE);
    }

    DebugDirs = (PIMAGE_DEBUG_DIRECTORY) ImageDirectoryEntryToData(
                                                                  LoadedImage.MappedAddress,
                                                                  FALSE,
                                                                  IMAGE_DIRECTORY_ENTRY_DEBUG,
                                                                  &DirCnt
                                                                  );

    if (!DebugDirectoryIsUseful(DebugDirs, DirCnt)) {
        UnMapAndLoad(&LoadedImage);
        return(FALSE);
    }

    DirCnt /= sizeof(IMAGE_DEBUG_DIRECTORY);
    CvDebugDir = NULL;

    while (DirCnt) {
        DirCnt--;
        if (DebugDirs[DirCnt].Type == IMAGE_DEBUG_TYPE_CODEVIEW) {
            CvDebugDir = &DebugDirs[DirCnt];
            break;
        }
    }

    if (!CvDebugDir) {
        // Didn't find any CV debug dir.  Bail.
        UnMapAndLoad(&LoadedImage);
        return(FALSE);
    }

    if (CvDebugDir->PointerToRawData != 0) {

        // If there's a .pdb, copy it to the same location as the referencing file.
        typedef struct NB10I {                 // NB10 debug info
            DWORD   nb10;                      // NB10
            DWORD   off;                       // offset, always 0
            DWORD   sig;
            DWORD   age;
        } NB10I;

        NB10I *pNB10Info;

        pNB10Info = (NB10I *) (CvDebugDir->PointerToRawData + (PCHAR)LoadedImage.MappedAddress);
        if (pNB10Info->nb10 == '01BN') {
            // Got a PDB.  The name immediately follows the signature.

            CHAR PdbName[_MAX_PATH];
            CHAR NewPdbName[_MAX_PATH];
            CHAR Drive[_MAX_DRIVE];
            CHAR Dir[_MAX_DIR];
            CHAR Filename[_MAX_FNAME];
            CHAR FileExt[_MAX_EXT];

            memset(PdbName, 0, sizeof(PdbName));
            memcpy(PdbName, ((PCHAR)pNB10Info) + sizeof(NB10I), CvDebugDir->SizeOfData - sizeof(NB10I));

            _splitpath(PdbName, NULL, NULL, Filename, FileExt);
            _splitpath(DestinationFile, Drive, Dir, NULL, NULL);
            _makepath(NewPdbName, Drive, Dir, Filename, FileExt);

            if (!fSetupMode && (fVerbose || fTestMode)) {
                fprintf(stdout,"BINPLACE : warning BNP0000: place %s in %s\n", PdbName, NewPdbName);
            }

            if (!MakeSureDirectoryPathExists(NewPdbName)) {
                fprintf(stderr, "BINPLACE : error BNP0000: Unable to create directory path '%s' (%u)\n",
                        NewPdbName, GetLastError());
            }

            SetFileAttributes(NewPdbName,FILE_ATTRIBUTE_NORMAL);

            if ( !CopyPdb(PdbName, NewPdbName, StripPrivate)) {
                if (!fSetupMode && (fVerbose || fTestMode)) {
                    fprintf(stderr,"BINPLACE : warning BNP0000: Unable to copy (%s,%s) %d\n", PdbName, NewPdbName, GetLastError());
                }
                // It's possible the name in the pdb isn't in the same location as it was when built.  See if we can
                //  find it in the same dir as the image...
                _splitpath(SourceFileName, Drive, Dir, NULL, NULL);
                _makepath(PdbName, Drive, Dir, Filename, FileExt);
                if (!fSetupMode && (fVerbose || fTestMode)) {
                    fprintf(stdout,"BINPLACE : warning BNP0000: place %s in %s\n", PdbName, NewPdbName);
                }

                if ( !CopyPdb(PdbName, NewPdbName, StripPrivate)) {
                    fprintf(stderr,"BINPLACE : warning BNP0000: CopyPdb(%s,%s) failed %d\n", PdbName, NewPdbName, GetLastError());
                }
            }

            if (!fKeepAttributes)
                SetFileAttributes(NewPdbName, FILE_ATTRIBUTE_NORMAL);

            if (!CopyFromSourceOnly) {
                PVOID pCertificates = ImageDirectoryEntryToData(LoadedImage.MappedAddress,
                                                          FALSE,
                                                          IMAGE_DIRECTORY_ENTRY_SECURITY,
                                                          &DirCnt
                                                          );

                if (!pCertificates && !DirCnt) {
                    // Only change the data in the image if it hasn't been signed (otherwise the sig is invalidated).
                    strcpy(((char *)pNB10Info) + sizeof(NB10I), Filename);
                    strcat(((char *)pNB10Info) + sizeof(NB10I), FileExt);
                    CvDebugDir->SizeOfData = sizeof(NB10I) + strlen(Filename) + strlen(FileExt) + 1;
                }
            }
        }
        UnMapAndLoad(&LoadedImage);
        return(TRUE);
    }

    UnMapAndLoad(&LoadedImage);
    return(FALSE);
}


BOOL
FileExists(
          IN  LPCSTR FileName,
          OUT PWIN32_FIND_DATA FindData
          )
{
    UINT OldMode;
    BOOL Found;
    HANDLE FindHandle;

    OldMode = SetErrorMode(SEM_FAILCRITICALERRORS);

    FindHandle = FindFirstFile(FileName,FindData);
    if (FindHandle == INVALID_HANDLE_VALUE) {
        Found = FALSE;
    } else {
        FindClose(FindHandle);
        Found = TRUE;
    }

    SetErrorMode(OldMode);
    return(Found);
}


//////////////////////////////////////////////////////////////////////
//                                                                  //
//  Digital Signature Stuff                                         //
//                                                                  //
//////////////////////////////////////////////////////////////////////

LPBSAFE_PUB_KEY         PUB;
LPBSAFE_PRV_KEY         PRV;

unsigned char pubmodulus[] =
{
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x3d, 0x3a, 0x5e, 0xbd, 0x72, 0x43, 0x3e, 0xc9,
    0x4d, 0xbb, 0xc1, 0x1e, 0x4a, 0xba, 0x5f, 0xcb,
    0x3e, 0x88, 0x20, 0x87, 0xef, 0xf5, 0xc1, 0xe2,
    0xd7, 0xb7, 0x6b, 0x9a, 0xf2, 0x52, 0x45, 0x95,
    0xce, 0x63, 0x65, 0x6b, 0x58, 0x3a, 0xfe, 0xef,
    0x7c, 0xe7, 0xbf, 0xfe, 0x3d, 0xf6, 0x5c, 0x7d,
    0x6c, 0x5e, 0x06, 0x09, 0x1a, 0xf5, 0x61, 0xbb,
    0x20, 0x93, 0x09, 0x5f, 0x05, 0x6d, 0xea, 0x87,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};

unsigned char prvmodulus[] =
{
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00,
    0x3d, 0x3a, 0x5e, 0xbd,
    0x72, 0x43, 0x3e, 0xc9, 0x4d, 0xbb, 0xc1, 0x1e,
    0x4a, 0xba, 0x5f, 0xcb, 0x3e, 0x88, 0x20, 0x87,
    0xef, 0xf5, 0xc1, 0xe2, 0xd7, 0xb7, 0x6b, 0x9a,
    0xf2, 0x52, 0x45, 0x95, 0xce, 0x63, 0x65, 0x6b,
    0x58, 0x3a, 0xfe, 0xef, 0x7c, 0xe7, 0xbf, 0xfe,
    0x3d, 0xf6, 0x5c, 0x7d, 0x6c, 0x5e, 0x06, 0x09,
    0x1a, 0xf5, 0x61, 0xbb, 0x20, 0x93, 0x09, 0x5f,
    0x05, 0x6d, 0xea, 0x87, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x3f, 0xbd, 0x29, 0x20,
    0x57, 0xd2, 0x3b, 0xf1, 0x07, 0xfa, 0xdf, 0xc1,
    0x16, 0x31, 0xe4, 0x95, 0xea, 0xc1, 0x2a, 0x46,
    0x2b, 0xad, 0x88, 0x57, 0x55, 0xf0, 0x57, 0x58,
    0xc6, 0x6f, 0x95, 0xeb, 0x00, 0x00, 0x00, 0x00,
    0x83, 0xdd, 0x9d, 0xd0, 0x03, 0xb1, 0x5a, 0x9b,
    0x9e, 0xb4, 0x63, 0x02, 0x43, 0x3e, 0xdf, 0xb0,
    0x52, 0x83, 0x5f, 0x6a, 0x03, 0xe7, 0xd6, 0x78,
    0x45, 0x83, 0x6a, 0x5b, 0xc4, 0xcb, 0xb1, 0x93,
    0x00, 0x00, 0x00, 0x00, 0x65, 0x9d, 0x43, 0xe8,
    0x48, 0x17, 0xcd, 0x29, 0x7e, 0xb9, 0x26, 0x5c,
    0x79, 0x66, 0x58, 0x61, 0x72, 0x86, 0x6a, 0xa3,
    0x63, 0xad, 0x63, 0xb8, 0xe1, 0x80, 0x4c, 0x0f,
    0x36, 0x7d, 0xd9, 0xa6, 0x00, 0x00, 0x00, 0x00,
    0x75, 0x3f, 0xef, 0x5a, 0x01, 0x5f, 0xf6, 0x0e,
    0xd7, 0xcd, 0x59, 0x1c, 0xc6, 0xec, 0xde, 0xf3,
    0x5a, 0x03, 0x09, 0xff, 0xf5, 0x23, 0xcc, 0x90,
    0x27, 0x1d, 0xaa, 0x29, 0x60, 0xde, 0x05, 0x6e,
    0x00, 0x00, 0x00, 0x00, 0xc0, 0x17, 0x0e, 0x57,
    0xf8, 0x9e, 0xd9, 0x5c, 0xf5, 0xb9, 0x3a, 0xfc,
    0x0e, 0xe2, 0x33, 0x27, 0x59, 0x1d, 0xd0, 0x97,
    0x4a, 0xb1, 0xb1, 0x1f, 0xc3, 0x37, 0xd1, 0xd6,
    0xe6, 0x9b, 0x35, 0xab, 0x00, 0x00, 0x00, 0x00,
    0x87, 0xa7, 0x19, 0x32, 0xda, 0x11, 0x87, 0x55,
    0x58, 0x00, 0x16, 0x16, 0x25, 0x65, 0x68, 0xf8,
    0x24, 0x3e, 0xe6, 0xfa, 0xe9, 0x67, 0x49, 0x94,
    0xcf, 0x92, 0xcc, 0x33, 0x99, 0xe8, 0x08, 0x60,
    0x17, 0x9a, 0x12, 0x9f, 0x24, 0xdd, 0xb1, 0x24,
    0x99, 0xc7, 0x3a, 0xb8, 0x0a, 0x7b, 0x0d, 0xdd,
    0x35, 0x07, 0x79, 0x17, 0x0b, 0x51, 0x9b, 0xb3,
    0xc7, 0x10, 0x01, 0x13, 0xe7, 0x3f, 0xf3, 0x5f,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00
};

BOOL initkey(void)
{
    DWORD       bits;

    PUB = (LPBSAFE_PUB_KEY)pubmodulus;

    PUB->magic = RSA1;
    PUB->keylen = 0x48;
    PUB->bitlen = 0x0200;
    PUB->datalen = 0x3f;
    PUB->pubexp = 0xc0887b5b;

    PRV = (LPBSAFE_PRV_KEY)prvmodulus;
    PRV->magic = RSA2;
    PRV->keylen = 0x48;
    PRV->bitlen = 0x0200;
    PRV->datalen = 0x3f;
    PRV->pubexp = 0xc0887b5b;

    bits = PRV->bitlen;

    return TRUE;
}


BOOL
SignWithIDWKey(
              IN  LPCSTR  FileName)
{

    HANDLE  hFile;
    HANDLE  hMapping;
    PUCHAR  pMap;
    HANDLE  hSigFile;
    DWORD   Size;
    MD5_CTX HashState;
    BYTE    SigHash[ 0x48 ];
    BYTE    Signature[ 0x48 ];
    CHAR    SigFilePath[ MAX_PATH ];
    PSTR    pszDot;

    BOOL    Return = FALSE;

    if (!initkey()) {
        return( FALSE );
    }

    hFile = CreateFile( FileName, GENERIC_READ,
                        FILE_SHARE_READ, NULL,
                        OPEN_EXISTING, 0, NULL );

    if (hFile != INVALID_HANDLE_VALUE) {
        hMapping = CreateFileMapping(   hFile,
                                        NULL,
                                        PAGE_READONLY,
                                        0, 0, NULL );

        if (hMapping) {
            pMap = MapViewOfFileEx( hMapping,
                                    FILE_MAP_READ,
                                    0, 0, 0, NULL );

            if (pMap) {
                Size = GetFileSize( hFile, NULL );

                MD5Init( &HashState );

                MD5Update( &HashState, pMap, Size );

                MD5Final( &HashState );

                memset(SigHash, 0xff, 0x40);

                SigHash[0x40-1] = 0;
                SigHash[0x40-2] = 1;
                SigHash[16] = 0;

                memcpy(SigHash, HashState.digest, 16);

                //
                // Encrypt the signature data
                //

                BSafeDecPrivate(PRV, SigHash, Signature );;

                //
                // Create and store it in a .sig file
                //

                strcpy( SigFilePath, FileName );

                pszDot = strrchr( SigFilePath, '.' );

                if (!pszDot) {
                    pszDot = SigFilePath + strlen( SigFilePath );
                }

                strcpy( pszDot, ".sig");

                hSigFile = CreateFile( SigFilePath, GENERIC_WRITE,
                                       0, NULL,
                                       CREATE_ALWAYS, 0, NULL );

                if (hSigFile != INVALID_HANDLE_VALUE) {
                    WriteFile(  hSigFile,
                                Signature,
                                sizeof( Signature ),
                                &Size, NULL );

                    CloseHandle( hSigFile );

                    Return = TRUE ;

                    if (fVerbose)
                        fprintf( stdout, "BINPLACE : warning BNP0000: Signature file generated in %s\n", SigFilePath);

                } else {
                    fprintf( stderr, "BINPLACE : error BNP0000: Unable to create file %s, %d\n",
                             SigFilePath, GetLastError() );
                }

                UnmapViewOfFile( pMap );

            } else {
                fprintf(stderr, "BINPLACE : error BNP0000: unable to map view, %d\n", GetLastError());
            }

            CloseHandle( hMapping );

        } else {
            fprintf(stderr, "BINPLACE : error BNP0000: CreateFileMapping of %s failed, %d\n",
                    FileName, GetLastError() );

        }

        CloseHandle( hFile );
    } else {
        fprintf( stderr, "BINPLACE : error BNP0000: could not open %s, %d\n",
                 FileName, GetLastError() );
    }

    return( Return );
}

BOOL                            // Keep as BOOL for the future (used by rsa code)
GenRandom (ULONG huid, BYTE *pbBuffer, size_t dwLength)
{
    return( FALSE );
}

#include <copypdb.c>
