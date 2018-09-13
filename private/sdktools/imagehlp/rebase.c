/*++

Copyright (c) 1992  Microsoft Corporation

Module Name:

    rebase.c

Abstract:

    Source file for the REBASE utility that takes a group of image files and
    rebases them so they are packed as closely together in the virtual address
    space as possible.

Author:

    Mark Lucovsky (markl) 30-Apr-1993

Revision History:

--*/

#include <private.h>

#define ROUNDUP(x, y) ((x + (y-1)) & ~(y-1))

VOID
RemoveRelocations(
    PCHAR ImageName
    );


#define REBASE_ERR 99
#define REBASE_OK  0
ULONG ReturnCode = REBASE_OK;

#define ROUND_UP( Size, Amount ) (((ULONG)(Size) + ((Amount) - 1)) & ~((Amount) - 1))

BOOL fVerbose;
BOOL fQuiet;
BOOL fGoingDown;
BOOL fSumOnly;
BOOL fRebaseSysfileOk;
BOOL fShowAllBases;
BOOL fCoffBaseIncExt;
FILE *CoffBaseDotTxt;
FILE *BaseAddrFile;
FILE *RebaseLog;
BOOL fSplitSymbols;
ULONG SplitFlags;
BOOL fRemovePrivteSym;
BOOL fRemoveRelocs;
BOOL fUpdateSymbolsOnly;

LPSTR BaseAddrFileName;

BOOL
ProcessGroupList(
    LPSTR ImagesRoot,
    LPSTR GroupListFName,
    BOOL  fReBase,
    BOOL  fOverlay
    );

BOOL
FindInIgnoreList(
    LPSTR chName
    );

ULONG64
FindInBaseAddrFile(
    LPSTR Name,
    PULONG pulSize
    );

VOID
ReBaseFile(
    LPSTR pstrName,
    BOOL  fReBase
    );

VOID
ParseSwitch(
    CHAR chSwitch,
    int *pArgc,
    char **pArgv[]
    );


VOID
ShowUsage(
    VOID
    );

typedef struct _GROUPNODE {
    struct _GROUPNODE *pgnNext;
    PCHAR chName;
} GROUPNODE, *PGROUPNODE;

PGROUPNODE pgnIgnoreListHdr, pgnIgnoreListEnd;

typedef BOOL (__stdcall *REBASEIMAGE64) (
    IN     PSTR CurrentImageName,
    IN     PSTR SymbolPath,
    IN     BOOL  fReBase,          // TRUE if actually rebasing, false if only summing
    IN     BOOL  fRebaseSysfileOk, // TRUE is system images s/b rebased
    IN     BOOL  fGoingDown,       // TRUE if the image s/b rebased below the given base
    IN     ULONG CheckImageSize,   // Max size allowed  (0 if don't care)
    OUT    ULONG *OldImageSize,    // Returned from the header
    OUT    ULONG64 *OldImageBase,  // Returned from the header
    OUT    ULONG *NewImageSize,    // Image size rounded to next separation boundary
    IN OUT ULONG64 *NewImageBase,  // (in) Desired new address.
                                   // (out) Next address (actual if going down)
    IN     ULONG TimeStamp         // new timestamp for image, if non-zero
    );

REBASEIMAGE64 pReBaseImage64;

UCHAR ImagesRoot[ MAX_PATH+1 ];
PCHAR SymbolPath;
UCHAR DebugFilePath[ MAX_PATH+1 ];

ULONG64 OriginalImageBase;
ULONG OriginalImageSize;
ULONG64 NewImageBase;
ULONG NewImageSize;

ULONG64 InitialBase = 0;
ULONG64 MinBase = (~((ULONG64)0));
ULONG64 TotalSize;


int __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
{

    char chChar, *pchChar;
    envp;
    _tzset();

    pgnIgnoreListHdr = (PGROUPNODE) malloc( sizeof ( GROUPNODE ) );
    pgnIgnoreListHdr->chName = NULL;
    pgnIgnoreListHdr->pgnNext = NULL;
    pgnIgnoreListEnd = pgnIgnoreListHdr;

    pReBaseImage64 = (REBASEIMAGE64) GetProcAddress(GetModuleHandle("imagehlp.dll"), "ReBaseImage64");
    if (!pReBaseImage64) {
        puts("REBASE: Warning\n"
             "REBASE: Warning - unable to correctly rebase 64-bit images - update your imagehlp.dll\n"
             "REBASE: Warning");
        pReBaseImage64 = (REBASEIMAGE64) GetProcAddress(GetModuleHandle("imagehlp.dll"), "ReBaseImage");
    }

    fVerbose = FALSE;
    fQuiet = FALSE;
    fGoingDown = FALSE;
    fSumOnly = FALSE;
    fRebaseSysfileOk = FALSE;
    fShowAllBases = FALSE;

    ImagesRoot[ 0 ] = '\0';

    if (argc <= 1) {
        ShowUsage();
        }

    while (--argc) {
        pchChar = *++argv;
        if (*pchChar == '/' || *pchChar == '-') {
            while (chChar = *++pchChar) {
                ParseSwitch( chChar, &argc, &argv );
                }
            }
        else {
            if ( !FindInIgnoreList( pchChar ) ) {
                ReBaseFile( pchChar, TRUE );
                }
            }
        }

    if ( !fQuiet ) {

        if ( BaseAddrFile ) {
            InitialBase = MinBase;
        }

        if ( fGoingDown ) {
            TotalSize = InitialBase - NewImageBase;
        }
        else {
            TotalSize = NewImageBase - InitialBase;
        }

        fprintf( stdout, "\n" );
        fprintf( stdout, "REBASE: Total Size of mapping 0x%016I64x\n", TotalSize );
        fprintf( stdout, "REBASE: Range 0x%016I64x -0x%016I64x\n",
                 min(NewImageBase, InitialBase), max(NewImageBase, InitialBase));

        if (RebaseLog) {
            fprintf( RebaseLog, "\nTotal Size of mapping 0x%016I64x\n", TotalSize );
            fprintf( RebaseLog, "Range 0x%016I64x -0x%016I64x\n\n",
                     min(NewImageBase, InitialBase), max(NewImageBase, InitialBase));
        }
    }

    if (RebaseLog) {
        fclose(RebaseLog);
        }

    if (BaseAddrFile){
        fclose(BaseAddrFile);
        }

    if (CoffBaseDotTxt){
        fclose(CoffBaseDotTxt);
        }

    return ReturnCode;
}


VOID
ShowUsage(
    VOID
    )
{
    fputs( "usage: REBASE [switches]\n"
           "              [-R image-root [-G filename] [-O filename] [-N filename]]\n"
           "              image-names... \n"
           "\n"
           "              One of -b and -i switches are mandatory.\n"
           "\n"
           "              [-a] Used with -x.  extract All debug info into .dbg file\n"
           "              [-b InitialBase] specify initial base address\n"
           "              [-c coffbase_filename] generate coffbase.txt\n"
           "                  -C includes filename extensions, -c does not\n"
           "              [-d] top down rebase\n"
           "              [-f] Strip relocs after rebasing the image\n"
           "              [-i coffbase_filename] get base addresses from coffbase_filename\n"
           "              [-l logFilePath] write image bases to log file.\n"
           "              [-p] Used with -x.  Remove private debug info when extracting\n"
           "              [-q] minimal output\n"
           "              [-s] just sum image range\n"
           "              [-u symbol_dir] Update debug info in .DBG along this path\n"
           "              [-v] verbose output\n"
           "              [-x symbol_dir] extract debug info into separate .DBG file first\n"
           "              [-z] allow system file rebasing\n"
           "              [-?] display this message\n"
           "\n"
           "              [-R image_root] set image root for use by -G, -O, -N\n"
           "              [-G filename] group images together in address space\n"
           "              [-O filename] overlay images in address space\n"
           "              [-N filename] leave images at their origional address\n"
           "                  -G, -O, -N, may occur multiple times.  File \"filename\"\n"
           "                  contains a list of files (relative to \"image-root\")\n" ,
           stderr );

    exit( REBASE_ERR );
}


VOID
ParseSwitch(
    CHAR chSwitch,
    int *pArgc,
    char **pArgv[]
    )
{

    switch (toupper( chSwitch )) {

        case '?':
            ShowUsage();
            break;

        case 'A':
            SplitFlags |= SPLITSYM_EXTRACT_ALL;
            break;

        case 'B':
            if (!--(*pArgc)) {
                ShowUsage();
                }
            (*pArgv)++;
            sscanf(**pArgv, "%I64x", &InitialBase);
            NewImageBase = InitialBase;
            break;

        case 'C':
            if (!--(*pArgc)) {
                ShowUsage();
                }
            (*pArgv)++;
            fCoffBaseIncExt = (chSwitch == 'C');
            CoffBaseDotTxt = fopen( *(*pArgv), "at" );
            if ( !CoffBaseDotTxt ) {
                fprintf( stderr, "REBASE: fopen %s failed %d\n", *(*pArgv), errno );
                ExitProcess( REBASE_ERR );
                }
            break;

        case 'D':
            fGoingDown = TRUE;
            break;

        case 'F':
            fRemoveRelocs = TRUE;
            break;

        case 'G':
        case 'O':
        case 'N':
            if (!--(*pArgc)) {
                ShowUsage();
                }
            (*pArgv)++;
            if (!ImagesRoot[0]) {
                fprintf( stderr, "REBASE: -R must preceed -%c\n", chSwitch );
                exit( REBASE_ERR );
                }
            ProcessGroupList( (PCHAR) ImagesRoot,
                              *(*pArgv),
                              toupper(chSwitch) != 'N',
                              toupper(chSwitch) == 'O');
            break;

        case 'I':
            if (!--(*pArgc)) {
                ShowUsage();
                }
            (*pArgv)++;
            BaseAddrFileName = *(*pArgv);
            BaseAddrFile = fopen( *(*pArgv), "rt" );
            if ( !BaseAddrFile ) {
                fprintf( stderr, "REBASE: fopen %s failed %d\n", *(*pArgv), errno );
                ExitProcess( REBASE_ERR );
                }
            break;

        case 'L':
            if (!--(*pArgc)) {
                ShowUsage();
                }
            (*pArgv)++;
            RebaseLog = fopen( *(*pArgv), "at" );
            if ( !RebaseLog ) {
                fprintf( stderr, "REBASE: fopen %s failed %d\n", *(*pArgv), errno );
                ExitProcess( REBASE_ERR );
                }
            break;

        case 'P':
            SplitFlags |= SPLITSYM_REMOVE_PRIVATE;
            break;

        case 'Q':
            fQuiet = TRUE;
            break;

        case 'R':
            if (!--(*pArgc)) {
                ShowUsage();
                }
            (*pArgv)++;
            strcpy( (PCHAR) ImagesRoot, *(*pArgv) );
            break;

        case 'S':
            fprintf(stdout,"\n");
            fSumOnly = TRUE;
            break;

        case 'U':
            if (!--(*pArgc)) {
                ShowUsage();
                }
            (*pArgv)++;
            fUpdateSymbolsOnly = TRUE;
            SymbolPath = **pArgv;
            break;

        case 'V':
            fVerbose = TRUE;
            break;

        case 'X':
            if (!--(*pArgc)) {
                ShowUsage();
                }
            (*pArgv)++;
            SymbolPath = **pArgv;
            fSplitSymbols = TRUE;
            break;

        case 'Z':
            fRebaseSysfileOk = TRUE;
            break;

        default:
            fprintf( stderr, "REBASE: Invalid switch - /%c\n", chSwitch );
            ShowUsage();
            break;

        }
}


BOOL
ProcessGroupList(
    LPSTR ImagesRoot,
    LPSTR GroupListFName,
    BOOL  fReBase,
    BOOL  fOverlay
    )
{

    PGROUPNODE pgn;
    FILE *GroupList;

    CHAR  chName[MAX_PATH+1];
    int   ateof;
    ULONG64 SavedImageBase;
    ULONG MaxImageSize=0;

    DWORD dw;
    CHAR  Buffer[ MAX_PATH+1 ];
    LPSTR FilePart;


    if (RebaseLog) {
        fprintf( RebaseLog, "*** %s\n", GroupListFName );
    }

    GroupList = fopen( GroupListFName, "rt" );
    if ( !GroupList ) {
        fprintf( stderr, "REBASE: fopen %s failed %d\n", GroupListFName, errno );
        ExitProcess( REBASE_ERR );
    }

    ateof = fscanf( GroupList, "%s", chName );

    SavedImageBase = NewImageBase;

    while ( ateof && ateof != EOF ) {

        dw = SearchPath( ImagesRoot, chName, NULL, sizeof(Buffer), Buffer, &FilePart );
        if ( dw == 0 || dw > sizeof( Buffer ) ) {
            if (!fQuiet) {
                fprintf( stderr, "REBASE: Could Not Find %s\\%s\n", ImagesRoot, chName );
            }
        }
        else {

            _strlwr( Buffer );  // Lowercase for consistency when displayed.

            pgn = (PGROUPNODE) malloc( sizeof( GROUPNODE ) );
            pgn->chName = _strdup( Buffer );
            if ( NULL == pgn->chName ) {
                fprintf( stderr, "REBASE: *** strdup failed (%s).\n", Buffer );
                ExitProcess( REBASE_ERR );
            }
            pgn->pgnNext = NULL;
            pgnIgnoreListEnd->pgnNext = pgn;
            pgnIgnoreListEnd = pgn;

            ReBaseFile( Buffer, fReBase );

            if ( fOverlay ) {
                if ( MaxImageSize < NewImageSize ) {
                    MaxImageSize = NewImageSize;
                }
                NewImageBase = SavedImageBase;
            }
        }

        ateof = fscanf( GroupList, "%s", chName );
    }

    fclose( GroupList );

    if ( fOverlay ) {
        if ( fGoingDown ) {
            NewImageBase -= ROUND_UP( MaxImageSize, IMAGE_SEPARATION );
        }
        else {
            NewImageBase += ROUND_UP( MaxImageSize, IMAGE_SEPARATION );
        }
    }

    if (RebaseLog) {
        fprintf( RebaseLog, "\n" );
    }

    return TRUE;
}


BOOL
FindInIgnoreList(
    LPSTR chName
    )
{
    PGROUPNODE pgn;

    DWORD dw;
    CHAR  Buffer[ MAX_PATH+1 ];
    LPSTR FilePart;


    dw = GetFullPathName( chName, sizeof(Buffer), Buffer, &FilePart );
    if ( dw == 0 || dw > sizeof( Buffer ) ) {
        fprintf( stderr, "REBASE: *** GetFullPathName failed (%s).\n", chName );
        ExitProcess( REBASE_ERR );
        }

    for (pgn = pgnIgnoreListHdr->pgnNext;
         pgn != NULL;
         pgn = pgn->pgnNext) {

        if (!_stricmp( Buffer, pgn->chName ) ) {
            return TRUE;
            }

        }

    return FALSE;
}

/////////////////////////////////////////////////////////////////////////////
/*
******************************************************************************
On a Hydra System, we don't want imaghlp.dll to load user32.dll since it
prevents CSRSS from exiting when running a under a debugger.
The following function has been copied from user32.dll so that we don't
link to user32.dll.
******************************************************************************
*/
////////////////////////////////////////////////////////////////////////////

LPSTR CharPrev(
    LPCSTR lpStart,
    LPCSTR lpCurrentChar)
{
    if (lpCurrentChar > lpStart) {
        LPCSTR lpChar;
        BOOL bDBC = FALSE;

        for (lpChar = --lpCurrentChar - 1 ; lpChar >= lpStart ; lpChar--) {
            if (!IsDBCSLeadByte(*lpChar))
                break;
            bDBC = !bDBC;
        }

        if (bDBC)
            lpCurrentChar--;
    }
    return (LPSTR)lpCurrentChar;
}

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////


VOID
ReBaseFile(
    LPSTR CurrentImageName,
    BOOL fReBase
    )
{
    DWORD dw;
    CHAR  Buffer[ MAX_PATH+1 ];
    CHAR  Buffer2[ MAX_PATH+1 ];
    LPSTR FilePart;
    LPSTR LocalSymbolPath;
    ULONG ThisImageExpectedSize = 0;
    ULONG64 ThisImageRequestedBase = NewImageBase;
    ULONG TimeStamp;

    if ( !InitialBase && !BaseAddrFile ) {
        fprintf( stderr, "REBASE: -b switch must specify a non-zero base  --or--\n" );
        fprintf( stderr, "        -i must specify a filename\n" );
        exit( REBASE_ERR );
        }

    if ( BaseAddrFile && ( InitialBase || fGoingDown || CoffBaseDotTxt ) ) {
        fprintf( stderr, "REBASE: -i is incompatible with -b, -d, and -c\n" );
        exit( REBASE_ERR );
    }

    dw = GetFullPathName( CurrentImageName, sizeof(Buffer), Buffer, &FilePart );
    if ( dw == 0 || dw > sizeof(Buffer) ) {
        FilePart = CurrentImageName;
    }
    _strlwr( FilePart );  // Lowercase for consistency when displayed.

    if ( BaseAddrFile && !(NewImageBase = ThisImageRequestedBase = FindInBaseAddrFile( FilePart, &ThisImageExpectedSize )) ) {
        fprintf( stdout, "REBASE: %-16s Not listed in %s\n", FilePart, BaseAddrFileName );
    }

    if (fSplitSymbols && !fSumOnly ) {

        if ( SplitSymbols( CurrentImageName, SymbolPath, (PCHAR) DebugFilePath, SplitFlags ) ) {
            if ( fVerbose ) {
                fprintf( stdout, "REBASE: %16s symbols split into %s\n", FilePart, DebugFilePath );
            }
        }
        else if (GetLastError() != ERROR_ALREADY_ASSIGNED && GetLastError() != ERROR_BAD_EXE_FORMAT) {
            fprintf( stdout, "REBASE: %-16s - unable to split symbols (%u)\n", FilePart, GetLastError() );
        }
    }

    if (fUpdateSymbolsOnly) {
        // On update, the symbol path is a semi-colon delimited path.  Find the one we want and
        // then fix the path for RebaseImage.
        HANDLE hDebugFile;
        CHAR Drive[_MAX_DRIVE];
        CHAR Dir[_MAX_DIR];
        PCHAR s;
        hDebugFile = FindDebugInfoFile(CurrentImageName, SymbolPath, DebugFilePath);
        if ( hDebugFile ) {
            CloseHandle(hDebugFile);
            _splitpath(DebugFilePath, Drive, Dir, NULL, NULL);
            _makepath(Buffer2, Drive, Dir, NULL, NULL);
            s = Buffer2 + strlen(Buffer2);
            s = CharPrev(Buffer2, s);
            if (*s == '\\') {
                *s = '\0';
            }
            LocalSymbolPath = Buffer2;
        } else {
            LocalSymbolPath = NULL;
        }
    } else {
        LocalSymbolPath = SymbolPath;
    }

    NewImageSize = (ULONG) -1;  // Hack so we can tell when system images are skipped.

    time( (time_t *) &TimeStamp );

    if (!(*pReBaseImage64)( CurrentImageName,
                      (PCHAR) LocalSymbolPath,
                      fReBase && !fSumOnly,
                      fRebaseSysfileOk,
                      fGoingDown,
                      ThisImageExpectedSize,
                      &OriginalImageSize,
                      &OriginalImageBase,
                      &NewImageSize,
                      &ThisImageRequestedBase,
                      TimeStamp ) ) {

        if (ThisImageRequestedBase == 0) {
            fprintf(stderr,
                    "REBASE: %-16s ***Grew too large (Size=0x%x; ExpectedSize=0x%x)\n",
                    FilePart,
                    OriginalImageSize,
                    ThisImageExpectedSize);
        } else {
            if (GetLastError() == ERROR_BAD_EXE_FORMAT) {
                if (fVerbose) {
                    fprintf( stderr,
                            "REBASE: %-16s DOS or OS/2 image ignored\n",
                            FilePart );
                }
            } else
            if (GetLastError() == ERROR_INVALID_ADDRESS) {
                fprintf( stderr,
                        "REBASE: %-16s Rebase failed.  Relocations are missing or new address is invalid\n",
                        FilePart );
                if (RebaseLog) {
                    fprintf( RebaseLog,
                             "%16s based at 0x%016I64x (size 0x%08x)  Unable to rebase. (missing relocations or new address is invalid)\n",
                             FilePart,
                             OriginalImageBase,
                             OriginalImageSize);
                }
            } else {
                fprintf( stderr,
                        "REBASE: *** RelocateImage failed (%s).  Image may be corrupted\n",
                        FilePart );
            }
        }

        ReturnCode = REBASE_ERR;
        return;

    } else {
        if (GetLastError() == ERROR_INVALID_DATA) {
            fprintf(stderr, "REBASE: Warning: DBG checksum did not match image.\n");
        }
    }

    // Keep track of the lowest base address.

    if (MinBase > NewImageBase) {
        MinBase = NewImageBase;
    }

    if ( fSumOnly || !fReBase ) {
        if (!fQuiet) {
            fprintf( stdout,
                     "REBASE: %16s mapped at %016I64x (size 0x%08x)\n",
                     FilePart,
                     OriginalImageBase,
                     OriginalImageSize);
        }
    } else {
        if (RebaseLog) {
            fprintf( RebaseLog,
                     "%16s rebased to 0x%016I64x (size 0x%08x)\n",
                     FilePart,
                     fGoingDown ? ThisImageRequestedBase : NewImageBase,
                     NewImageSize);
        }

        if ((NewImageSize != (ULONG64) -1) &&
            (OriginalImageBase != (fGoingDown ? ThisImageRequestedBase : NewImageBase)) &&
            ( fVerbose || fQuiet )
           ) {
            if ( fVerbose ) {
                fprintf( stdout,
                         "REBASE: %16s initial base at 0x%016I64x (size 0x%08x)\n",
                         FilePart,
                         OriginalImageBase,
                         OriginalImageSize);
            }

            fprintf( stdout,
                     "REBASE: %16s rebased to 0x%016I64x (size 0x%08x)\n",
                     FilePart,
                     fGoingDown ? ThisImageRequestedBase : NewImageBase,
                     NewImageSize);

            if ( fVerbose && (fSplitSymbols || fUpdateSymbolsOnly) && DebugFilePath[0]) {
                char szExt[_MAX_EXT];
                _splitpath(DebugFilePath, NULL, NULL, NULL, szExt);
                if (_stricmp(szExt, ".pdb")) {
                    fprintf( stdout, "REBASE: %16s updated image base in %s\n", FilePart, DebugFilePath );
                }
            }
        }

        if (fRemoveRelocs) {
            RemoveRelocations(CurrentImageName);
        }
    }

    if ( CoffBaseDotTxt ) {
        if ( !fCoffBaseIncExt ) {
            char *n;
            if ( n  = strrchr(FilePart,'.') ) {
                *n = '\0';
            }
        }

        fprintf( CoffBaseDotTxt,
                 "%-16s 0x%016I64x 0x%08x\n",
                 FilePart,
                 fSumOnly ? OriginalImageBase : (fGoingDown ? ThisImageRequestedBase : NewImageBase),
                 NewImageSize);
    }

    NewImageBase = ThisImageRequestedBase;   // Set up the next one...
}

ULONG64
FindInBaseAddrFile(
    LPSTR Name,
    PULONG pulSize
    )
{

    struct {
        CHAR  Name[MAX_PATH+1];
        ULONG64 Base;
        ULONG Size;
    } BAFileEntry;

    CHAR NameNoExt[MAX_PATH+1];
//    PCHAR pchExt;
    int ateof;


    strcpy(NameNoExt,Name);
//    if (pchExt = strrchr(NameNoExt,'.')) {
//        *pchExt = '\0';
//        }

    fseek(BaseAddrFile, 0, SEEK_SET);

    ateof = fscanf(BaseAddrFile,"%s %I64x %x",BAFileEntry.Name,&BAFileEntry.Base,&BAFileEntry.Size);
    while ( ateof && ateof != EOF ) {
        if ( !_stricmp(NameNoExt,BAFileEntry.Name) ) {
            *pulSize = BAFileEntry.Size;
            return BAFileEntry.Base;
            }
        ateof = fscanf(BaseAddrFile,"%s %I64x %x",BAFileEntry.Name,&BAFileEntry.Base,&BAFileEntry.Size);
        }

    *pulSize = 0;
    return 0;
}

VOID
RemoveRelocations(
    PCHAR ImageName
    )
{
    // UnSafe...

    LOADED_IMAGE li;
    IMAGE_SECTION_HEADER RelocSectionHdr, *Section, *pRelocSecHdr;
    PIMAGE_DEBUG_DIRECTORY DebugDirectory;
    ULONG DebugDirectorySize, i, RelocSecNum;
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;
    PIMAGE_FILE_HEADER FileHeader;

    if (!MapAndLoad(ImageName, NULL, &li, FALSE, FALSE)) {
        return;
    }

    FileHeader = &li.FileHeader->FileHeader;

    OptionalHeadersFromNtHeaders((PIMAGE_NT_HEADERS32)li.FileHeader, &OptionalHeader32, &OptionalHeader64);

    // See if the image has already been stripped or there are no relocs.

    if ((FileHeader->Characteristics & IMAGE_FILE_RELOCS_STRIPPED) ||
        (!OPTIONALHEADER(DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size))) {
        UnMapAndLoad(&li);
        return;
    }

    for (Section = li.Sections, i = 0; i < li.NumberOfSections; Section++, i++) {
        if (Section->PointerToRawData != 0) {
            if (!_stricmp( (char *) Section->Name, ".reloc" )) {
                RelocSectionHdr = *Section;
                pRelocSecHdr = Section;
                RelocSecNum = i + 1;
            }
        }
    }

    RelocSectionHdr.Misc.VirtualSize = ROUNDUP(RelocSectionHdr.Misc.VirtualSize, OPTIONALHEADER(SectionAlignment));
    RelocSectionHdr.SizeOfRawData = ROUNDUP(RelocSectionHdr.SizeOfRawData, OPTIONALHEADER(FileAlignment));

    if (RelocSecNum != li.NumberOfSections) {
        // Move everything else up and fixup old addresses.
        for (i = RelocSecNum - 1, Section = pRelocSecHdr;i < li.NumberOfSections - 1; Section++, i++) {
            *Section = *(Section + 1);
            Section->VirtualAddress -= RelocSectionHdr.Misc.VirtualSize;
            Section->PointerToRawData -= RelocSectionHdr.SizeOfRawData;
        }
    }

    // Zero out the last one.

    RtlZeroMemory(Section, sizeof(IMAGE_SECTION_HEADER));

    // Reduce the section count.

    FileHeader->NumberOfSections--;

    // Set the strip bit in the header

    FileHeader->Characteristics |= IMAGE_FILE_RELOCS_STRIPPED;

    // If there's a pointer to the coff symbol table, move it back.

    if (FileHeader->PointerToSymbolTable) {
        FileHeader->PointerToSymbolTable -= RelocSectionHdr.SizeOfRawData;
    }

    // Clear out the base reloc entry in the data dir.

    OPTIONALHEADER_LV(DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].Size) = 0;
    OPTIONALHEADER_LV(DataDirectory[IMAGE_DIRECTORY_ENTRY_BASERELOC].VirtualAddress) = 0;

    // Reduce the Init Data size.

    OPTIONALHEADER_LV(SizeOfInitializedData) -= RelocSectionHdr.Misc.VirtualSize;

    // Reduce the image size.

    OPTIONALHEADER_LV(SizeOfImage) -=
        ((RelocSectionHdr.SizeOfRawData +
          (OPTIONALHEADER(SectionAlignment) - 1)
         ) & ~(OPTIONALHEADER(SectionAlignment) - 1));

    // Move the debug info up (if there is any).

    DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)
                            ImageDirectoryEntryToData( li.MappedAddress,
                                                      FALSE,
                                                      IMAGE_DIRECTORY_ENTRY_DEBUG,
                                                      &DebugDirectorySize
                                                    );
    if (DebugDirectoryIsUseful(DebugDirectory, DebugDirectorySize)) {
        for (i = 0; i < (DebugDirectorySize / sizeof(IMAGE_DEBUG_DIRECTORY)); i++) {
            RtlMoveMemory(li.MappedAddress + DebugDirectory->PointerToRawData - RelocSectionHdr.SizeOfRawData,
                            li.MappedAddress + DebugDirectory->PointerToRawData,
                            DebugDirectory->SizeOfData);

            DebugDirectory->PointerToRawData -= RelocSectionHdr.SizeOfRawData;

            if (DebugDirectory->AddressOfRawData) {
                DebugDirectory->AddressOfRawData -= RelocSectionHdr.Misc.VirtualSize;
            }

            DebugDirectory++;
        }
    }

    // Truncate the image size

    li.SizeOfImage -= RelocSectionHdr.SizeOfRawData;

    // And we're done.

    UnMapAndLoad(&li);
}
