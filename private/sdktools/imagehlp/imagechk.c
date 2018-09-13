/*++

Copyright (c) 1998  Microsoft Corporation

Module Name:

    imagechk.c

Abstract:

    this module implements a sanity check of certain image characteristics

Author:

    NT Base

Revision History:


Notes:


--*/

#ifdef __cplusplus
extern "C" {
#endif
#include <nt.h>
#include <ntrtl.h>
#include <nturtl.h>
#ifdef __cplusplus
}
#endif

#include <errno.h>
#include <direct.h>
#include <cvinfo.h>
#include <private.h>

typedef struct _SYMMODLIST{
    char *ModName;
    void *ModBase;
    struct _SYMMODLIST *Next;
} SYMMODLIST, *PSYMMODLIST;

typedef struct List {
    char            Name[40];
    unsigned long   Attributes;
} List, *pList;

typedef struct _LogListItem {
    char *LogLine;
    struct _LogListItem *Next;
} LogListItem, *pLogListItem;

//
// decarations
//

VOID
FindFiles();

VOID
Imagechk(
    List *rgpList,
    TCHAR *szDirectory
    );

VOID
ParseArgs(
    int *pargc,
    char **argv
    );

int
__cdecl
CompFileAndDir(
    const void *elem1,
    const void *elem2
    );

int
__cdecl
CompName(
    const void *elem1,
    const void *elem2
    );

VOID
Usage(
    VOID
    );

int
_cdecl
_cwild(
    VOID
    );

PSYMMODLIST
MakeModList(
    HANDLE
    );

void
FreeModList(
    PSYMMODLIST
    );

BOOL
CALLBACK
SymEnumerateModulesCallback(
    LPSTR,
    ULONG64,
    PVOID
    );

void *
GetModAddrFromName(
    PSYMMODLIST,
    char *
    );

BOOL
VerifyVersionResource(
    PCHAR FileName,
    BOOL fSelfRegister
    );

BOOL
ValidatePdata(
    PIMAGE_DOS_HEADER DosHeader
    );

BOOL
ImageNeedsOleSelfRegister(
    PIMAGE_DOS_HEADER DosHeader
    );

NTSTATUS
MiVerifyImageHeader (
    IN PIMAGE_NT_HEADERS NtHeader,
    IN PIMAGE_DOS_HEADER DosHeader,
    IN DWORD NtHeaderSize
    );

pLogListItem
LogAppend(
    char *,
    pLogListItem
    );

void
LogOutAndClean(
    BOOL
    );

void
LogPrintf(
    const char *format,
    ...
    );

#define X64K (64*1024)

#define MM_SIZE_OF_LARGEST_IMAGE ((ULONG)0x10000000)

#define MM_MAXIMUM_IMAGE_HEADER (2 * PageSize)

#define MM_MAXIMUM_IMAGE_SECTIONS                       \
     ((MM_MAXIMUM_IMAGE_HEADER - (4096 + sizeof(IMAGE_NT_HEADERS))) /  \
            sizeof(IMAGE_SECTION_HEADER))

#define MMSECTOR_SHIFT 9  //MUST BE LESS THAN OR EQUAL TO PageShift

#define MMSECTOR_MASK 0x1ff

#define MI_ROUND_TO_SIZE(LENGTH,ALIGNMENT)     \
                    (((ULONG)LENGTH + ALIGNMENT - 1) & ~(ALIGNMENT - 1))

#define BYTES_TO_PAGES(Size)  (((ULONG)(Size) >> PageShift) + \
                               (((ULONG)(Size) & (PageSize - 1)) != 0))

#define ArgFlag_OK      1
#define ArgFlag_CKMZ    2
#define ArgFlag_SymCK   4
#define ArgFlag_OLESelf 8
#define ArgFlag_CKBase  16

//
// file global data
//

BOOL fRecurse;
BOOL fFileOut;
BOOL fNotCurrent;
BOOL fPattern;
BOOL fSingleFile;
BOOL fPathOverride;
BOOL fSingleSlash;
BOOL fDebugMapped;
FILE* fout;
CHAR *szFileName = {"*.*"};
CHAR *pszRootDir;
CHAR *pszFileOut;
CHAR szDirectory[MAX_PATH] = {"."};
CHAR szSympath[MAX_PATH] = {0};
CHAR *szPattern;
int endpath, DirNum=1, ProcessedFiles;
ULONG PageSize;
ULONG PageShift;
PVOID HighestUserAddress;
USHORT ValidMachineIDMin;
USHORT ValidMachineIDMax;
DWORD ArgFlag;

//
// logging support
//

pLogListItem pLogList = NULL;
pLogListItem pLogListTmp = NULL;

typedef
NTSTATUS
(NTAPI *LPLDRVERIFYIMAGECHKSUM)(
    IN HANDLE ImageFileHandle
    );

LPLDRVERIFYIMAGECHKSUM lpOldLdrVerifyImageMatchesChecksum;

typedef
NTSTATUS
(NTAPI *LPLDRVERIFYIMAGEMATCHESCHECKSUM) (
    IN HANDLE ImageFileHandle,
    IN PLDR_IMPORT_MODULE_CALLBACK ImportCallbackRoutine OPTIONAL,
    IN PVOID ImportCallbackParameter,
    OUT PUSHORT ImageCharacteristics OPTIONAL
    );

LPLDRVERIFYIMAGEMATCHESCHECKSUM lpNewLdrVerifyImageMatchesChecksum;

typedef
NTSTATUS
(NTAPI *LPNTQUERYSYSTEMINFORMATION) (
    IN SYSTEM_INFORMATION_CLASS SystemInformationClass,
    OUT PVOID SystemInformation,
    IN ULONG SystemInformationLength,
    OUT PULONG ReturnLength OPTIONAL
    );

LPNTQUERYSYSTEMINFORMATION lpNtQuerySystemInformation;


OSVERSIONINFO VersionInformation;

//
// function definitions
//

VOID __cdecl
main(
    int argc,
    char *argv[],
    char *envp[]
    )
/*++

Routine Description:

    program entry

Arguments:

    int     argc,
    char    *argv[]
    char    *envp[]

Return Value:

    none

Notes:


--*/
{
    TCHAR CWD[MAX_PATH];
    int dirlen=0;

    if (argc < 2) {
        Usage();
    }

    ParseArgs(&argc, argv);

    GetCurrentDirectory(MAX_PATH, CWD);

    VersionInformation.dwOSVersionInfoSize = sizeof(VersionInformation);
    if (!GetVersionEx( &VersionInformation )) {
        fprintf(stderr, "Unable to detect OS version.  Terminating.\n" );
        exit(1);
    }
    if ((VersionInformation.dwPlatformId != VER_PLATFORM_WIN32_NT) ||
        (VersionInformation.dwBuildNumber < 1230))
    {
        lpOldLdrVerifyImageMatchesChecksum = (LPLDRVERIFYIMAGECHKSUM)
            GetProcAddress(GetModuleHandle(TEXT("NTDLL.DLL")), TEXT("LdrVerifyImageMatchesChecksum"));
        if (lpOldLdrVerifyImageMatchesChecksum == NULL) {
            fprintf(stderr, "Incorrect operating system version.\n" );
            exit(1);
        }
    } else {
        lpOldLdrVerifyImageMatchesChecksum = NULL;
        if ((VersionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT) &&
            (VersionInformation.dwBuildNumber >= 1230))
        {
            lpNewLdrVerifyImageMatchesChecksum = (LPLDRVERIFYIMAGEMATCHESCHECKSUM)
                GetProcAddress(GetModuleHandle(TEXT("NTDLL.DLL")), TEXT("LdrVerifyImageMatchesChecksum"));
            if (lpNewLdrVerifyImageMatchesChecksum == NULL) {
                fprintf(stderr, "OS is screwed up.  NTDLL doesn't export LdrVerifyImageMatchesChecksum.\n" );
                exit(1);
            }
        }
    }

    if (VersionInformation.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        SYSTEM_BASIC_INFORMATION SystemInformation;

        if (VersionInformation.dwBuildNumber <= 1465) {
            goto UseWin9x;
        }

        ValidMachineIDMin = USER_SHARED_DATA->ImageNumberLow;
        ValidMachineIDMax = USER_SHARED_DATA->ImageNumberHigh;
        lpNtQuerySystemInformation = (LPNTQUERYSYSTEMINFORMATION)
            GetProcAddress(GetModuleHandle(TEXT("NTDLL.DLL")), TEXT("NtQuerySystemInformation"));
        if (!lpNtQuerySystemInformation) {
            fprintf(stderr, "Incorrect operation system version.\n");
            exit(1);
        }
        if (!NT_SUCCESS((*lpNtQuerySystemInformation)(SystemBasicInformation,
                                                     &SystemInformation,
                                                     sizeof(SystemInformation),
                                                     NULL))) {
            fprintf(stderr, "OS is screwed up.  NtQuerySystemInformation failed.\n");
            exit(1);
        }
        HighestUserAddress = (PVOID)SystemInformation.MaximumUserModeAddress;
    } else {
UseWin9x:
        HighestUserAddress = (PVOID) 0x7FFE0000;
#ifdef _M_IX86
        ValidMachineIDMin = IMAGE_FILE_MACHINE_I386;
        ValidMachineIDMax = IMAGE_FILE_MACHINE_I386;
#elif defined(_M_ALPHA)
        ValidMachineIDMin = IMAGE_FILE_MACHINE_ALPHA;
        ValidMachineIDMax = IMAGE_FILE_MACHINE_ALPHA;
#elif defined(_M_IA64)
        ValidMachineIDMin = IMAGE_FILE_MACHINE_IA64;
        ValidMachineIDMax = IMAGE_FILE_MACHINE_IA64;
#elif defined(_M_AXP64)
        ValidMachineIDMin = IMAGE_FILE_MACHINE_AXP64;
        ValidMachineIDMax = IMAGE_FILE_MACHINE_AXP64;
#else
#error("Unknown machine type")
#endif
    }

    if (fPathOverride) {
        if (_chdir(szDirectory) == -1){   // cd to dir
            fprintf(stderr, "Path not found: %s\n", szDirectory);
            Usage();
        }
    }
    // remove trailing '\' needed only for above chdir, not for output formatting
    if (fSingleSlash) {
        dirlen = strlen(szDirectory);
        szDirectory[dirlen-1] = '\0';
    }

    FindFiles();

    fprintf(stdout, "%d files processed in %d directories\n", ProcessedFiles, DirNum);
}

VOID
FindFiles()
/*++

Routine Description:

    make list of files to check, then check them

Arguments:

    none

Return Value:

    none

Notes:


--*/
{

    HANDLE fh;
    TCHAR CWD[MAX_PATH];
    char *q;
    WIN32_FIND_DATA *pfdata;
    BOOL fFilesInDir=FALSE;
    BOOL fDirsFound=FALSE;
    int dnCounter=0, cNumDir=0, i=0, Length=0, NameSize=0, total=0, cNumFiles=0;

    pList rgpList[5000];

    pfdata = (WIN32_FIND_DATA*)malloc(sizeof(WIN32_FIND_DATA));
    if (!pfdata) {
        fprintf(stderr, "Not enough memory.\n");
        return;
    }

    if (!fRecurse) {
        fh = FindFirstFile(szFileName, pfdata);  // find only filename (pattern) if not recursive
    } else {
        fh = FindFirstFile("*.*", pfdata);       // find all if recursive in order to determine subdirectory names
    }

    if (fh == INVALID_HANDLE_VALUE) {
        fprintf(fout==NULL? stderr : fout , "File not found: %s\n", szFileName);
        return;
    }

    // loop to find all files and directories in current directory
    // and copy pertinent data to individual List structures.
    do {
        if (strcmp(pfdata->cFileName, ".") && strcmp(pfdata->cFileName, "..")) {  // skip . and ..
            rgpList[dnCounter] = (pList)malloc(sizeof(List));  // allocate the memory
            if (!rgpList[dnCounter]) {
                fprintf(stderr, "Not enough memory.\n");
                return;
            }

            if (!(pfdata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {   // if file

                fFilesInDir=TRUE;

                // see if given pattern wildcard extension matches pfdata->cFileName extension
                if (fPattern) {
                    q = strchr(pfdata->cFileName, '.');    // find first instance of "." in filename
                    if (q == NULL) goto blah;             // "." not found
                    _strlwr(q);                            // lowercase before compare
                    if (strcmp(q, szPattern)) goto blah;  // if pattern and name doesn't match goto
                }                                        // OK, I used a goto, get over it.

                if (fSingleFile) {
                    _strlwr(pfdata->cFileName);
                    _strlwr(szFileName);
                    if (strcmp(pfdata->cFileName, szFileName)) goto blah;
                }

                // if pattern && match || no pattern
                strcpy(rgpList[dnCounter]->Name, pfdata->cFileName);
                _strlwr(rgpList[dnCounter]->Name);  // all lowercase for strcmp in CompName

                memcpy(&(rgpList[dnCounter]->Attributes), &pfdata->dwFileAttributes, 4);
                dnCounter++;
                cNumFiles++;
            } else {
                if (pfdata->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {   // if dir

                    fDirsFound=TRUE;
                    //cNumDir++;

                    if (fRecurse) {
                        strcpy(rgpList[dnCounter]->Name, pfdata->cFileName);
                        _strlwr(rgpList[dnCounter]->Name);  // all lowercase for strcmp in CompName
                        memcpy(&(rgpList[dnCounter]->Attributes), &pfdata->dwFileAttributes, 4);
                        cNumDir++;
                        dnCounter++;
                    }
                }
            }
        }
blah: ;

    } while (FindNextFile(fh, pfdata));

    FindClose(fh); // close the file handle

    // Sort Array arranging FILE entries at top
    qsort( (void *)rgpList, dnCounter, sizeof(List *), CompFileAndDir);

    // Sort Array alphabetizing only FILE names
    qsort( (void *)rgpList, dnCounter-cNumDir, sizeof(List *), CompName);

    // Sort Array alphabetizing only DIRectory names
    if (fRecurse) {
        qsort( (void *)&rgpList[dnCounter-cNumDir], cNumDir, sizeof(List *), CompName);
    }

    // Process newly sorted structures.
    for (i=0; i < dnCounter; ++i) {

        if (rgpList[i]->Attributes & FILE_ATTRIBUTE_DIRECTORY) {  // if Dir
            if (fRecurse) {

                if (_chdir(rgpList[i]->Name) == -1){   // cd into subdir and check for error
                    fprintf(stderr, "Unable to change directory: %s\n", rgpList[i]->Name);

                } else {

                    NameSize = strlen(rgpList[i]->Name);
                    strcat(szDirectory, "\\");
                    strcat(szDirectory, rgpList[i]->Name); //append name to directory path
                    total = strlen(szDirectory);
                    DirNum++;      // directory counter

                    // start another iteration of FindFiles
                    FindFiles();

                    // get back to previous directory when above iteration returns
                    _chdir("..");

                    // cut off previously appended directory name - for output only
                    szDirectory[total-(NameSize+1)]='\0';
                }
            }
        } else {
            if (!(rgpList[i]->Attributes & FILE_ATTRIBUTE_DIRECTORY))   // check image if not dir
                Imagechk(rgpList[i], szDirectory);
        }
    }
} // end FindFiles

VOID
Imagechk(
    List *rgpList,
    TCHAR *szDirectory
    )
/*++

Routine Description:

    check various things, including:
        image type, header alignment, image size, machine type
        alignment, some properties of various sections, checksum integrity
        symbol / image file checksum agreement, existence of symbols, etc

Arguments:

    List *  rgpList,
    TCHAR * szDirectory

Return Value:

    none

Notes:


--*/
{

    HANDLE File;
    HANDLE MemMap;
    PIMAGE_DOS_HEADER DosHeader;
    PIMAGE_NT_HEADERS NtHeader;
    NTSTATUS Status;
    BY_HANDLE_FILE_INFORMATION FileInfo;
    ULONG NumberOfPtes;
    ULONG SectionVirtualSize = 0;
    ULONG i;
    PIMAGE_SECTION_HEADER SectionTableEntry;
    ULONG NumberOfSubsections;
    PCHAR ExtendedHeader = NULL;
    ULONG_PTR PreferredImageBase;
    ULONG_PTR NextVa;
    ULONG ImageFileSize;
    ULONG OffsetToSectionTable;
    ULONG ImageAlignment;
    ULONG PtesInSubsection;
    ULONG StartingSector;
    ULONG EndingSector;
    LPSTR ImageName;
    LPSTR MachineType = "Unknown";
    BOOL MachineTypeMismatch;
    BOOL ImageOk;
    BOOL fHasPdata;
    OSVERSIONINFO OSVerInfo;

    ImageName = rgpList->Name;
    OSVerInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
    GetVersionEx(&OSVerInfo);

    LogPrintf("ImageChk: %s\\%s \n", szDirectory, ImageName);

    ProcessedFiles++;

    DosHeader = NULL;
    ImageOk = TRUE;
    File = CreateFile (ImageName,
                        GENERIC_READ | FILE_EXECUTE,
                        OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ? (FILE_SHARE_READ | FILE_SHARE_DELETE) : FILE_SHARE_READ,
                        NULL,
                        OPEN_EXISTING,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL);

    if (File == INVALID_HANDLE_VALUE) {
        LogPrintf("Error, CreateFile() %d\n", GetLastError());
        ImageOk = FALSE;
        goto NextImage;
    }

    MemMap = CreateFileMapping (File,
                        NULL,           // default security.
                        PAGE_READONLY,  // file protection.
                        0,              // high-order file size.
                        0,
                        NULL);

    if (!GetFileInformationByHandle(File, &FileInfo)) {
        fprintf(stderr,"Error, GetFileInfo() %d\n", GetLastError());
        CloseHandle(File);
        ImageOk = FALSE; goto NextImage;
    }

    DosHeader = (PIMAGE_DOS_HEADER) MapViewOfFile(MemMap,
                              FILE_MAP_READ,
                              0,  // high
                              0,  // low
                              0   // whole file
                              );

    CloseHandle(MemMap);
    if (!DosHeader) {
        fprintf(stderr,"Error, MapViewOfFile() %d\n", GetLastError());
        ImageOk = FALSE; goto NextImage;
    }

    //
    // Check to determine if this is an NT image (PE format) or
    // a DOS image, Win-16 image, or OS/2 image.  If the image is
    // not NT format, return an error indicating which image it
    // appears to be.
    //

    if (DosHeader->e_magic != IMAGE_DOS_SIGNATURE) {

        if (ArgFlag & ArgFlag_CKMZ) {
            LogPrintf("MZ header not found\n");
            ImageOk = FALSE;
        }
        goto NeImage;
    }


    if (((ULONG)DosHeader->e_lfanew & 3) != 0) {

        //
        // The image header is not aligned on a long boundary.
        // Report this as an invalid protect mode image.
        //

        LogPrintf("Image header not on Long boundary\n");
        ImageOk = FALSE;
        goto NeImage;
    }


    if ((ULONG)DosHeader->e_lfanew > FileInfo.nFileSizeLow) {
        LogPrintf("Image size bigger than size of file\n");
        ImageOk = FALSE;
        goto NeImage;
    }

    NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)DosHeader + (ULONG)DosHeader->e_lfanew);

    if (NtHeader->Signature != IMAGE_NT_SIGNATURE) { //if not PE image

        LogPrintf("Non 32-bit image");
        ImageOk = TRUE;
        goto NeImage;
    }

    //
    // Check to see if this is an NT image or a DOS or OS/2 image.
    //

    Status = MiVerifyImageHeader (NtHeader, DosHeader, 50000);
    if (Status != STATUS_SUCCESS) {
        ImageOk = FALSE;            //continue checking the image but don't print "OK"
    }

    //
    // Verify machine type.
    //

    fHasPdata = TRUE;       // Most do

    switch (NtHeader->FileHeader.Machine) {
        case IMAGE_FILE_MACHINE_I386:
            MachineType = "x86";
            PageSize = 4096;
            PageShift = 12;
            fHasPdata = FALSE;
            break;

        case IMAGE_FILE_MACHINE_ALPHA:
            MachineType = "Alpha";
            PageSize = 8192;
            PageShift = 13;
            break;

        case IMAGE_FILE_MACHINE_IA64:
            MachineType = "Intel64";
            PageSize = 8192;
            PageShift = 13;
            break;

        case IMAGE_FILE_MACHINE_ALPHA64:
            MachineType = "Alpha64";
            PageSize = 8192;
            PageShift = 13;
            break;

        default:
            LogPrintf("Unrecognized machine type x%lx\n",
                NtHeader->FileHeader.Machine);
            ImageOk = FALSE;
            break;
        }

    if ((NtHeader->FileHeader.Machine < ValidMachineIDMin) ||
        (NtHeader->FileHeader.Machine > ValidMachineIDMax)) {
        MachineTypeMismatch = TRUE;
    } else {
        MachineTypeMismatch = FALSE;
    }

    ImageAlignment = NtHeader->OptionalHeader.SectionAlignment;

    NumberOfPtes = BYTES_TO_PAGES (NtHeader->OptionalHeader.SizeOfImage);

    NextVa = NtHeader->OptionalHeader.ImageBase;

    if ((NextVa & (X64K - 1)) != 0) {

        //
        // Image header is not aligned on a 64k boundary.
        //

        LogPrintf("image base not on 64k boundary %lx\n",NextVa);

        ImageOk = FALSE;
        goto BadPeImageSegment;
    }

    //BasedAddress = (PVOID)NextVa;
    PtesInSubsection = MI_ROUND_TO_SIZE (
                                       NtHeader->OptionalHeader.SizeOfHeaders,
                                       ImageAlignment
                                   ) >> PageShift;

    if (ImageAlignment >= PageSize) {

        //
        // Aligmment is PageSize of greater.
        //

        if (PtesInSubsection > NumberOfPtes) {

            //
            // Inconsistent image, size does not agree with header.
            //

            LogPrintf("Image size in header (%ld.) not consistent with sections (%ld.)\n",
                    NumberOfPtes, PtesInSubsection);
            ImageOk = FALSE;
            goto BadPeImageSegment;
        }

        NumberOfPtes -= PtesInSubsection;

        EndingSector = NtHeader->OptionalHeader.SizeOfHeaders >> MMSECTOR_SHIFT;

        for (i = 0; i < PtesInSubsection; i++) {

            NextVa += PageSize;
        }
    }

    //
    // Build the next subsections.
    //

    NumberOfSubsections = NtHeader->FileHeader.NumberOfSections;
    PreferredImageBase = NtHeader->OptionalHeader.ImageBase;

    //
    // At this point the object table is read in (if it was not
    // already read in) and may displace the image header.
    //

    OffsetToSectionTable = sizeof(ULONG) +
                              sizeof(IMAGE_FILE_HEADER) +
                              NtHeader->FileHeader.SizeOfOptionalHeader;

    SectionTableEntry = (PIMAGE_SECTION_HEADER)((PCHAR)NtHeader + OffsetToSectionTable);

    if (ImageAlignment < PageSize) {

        // The image header is no longer valid, TempPte is
        // used to indicate that this image alignment is
        // less than a PageSize.

        //
        // Loop through all sections and make sure there is no
        // unitialized data.
        //

        while (NumberOfSubsections > 0) {
            if (SectionTableEntry->Misc.VirtualSize == 0) {
                SectionVirtualSize = SectionTableEntry->SizeOfRawData;
            } else {
                SectionVirtualSize = SectionTableEntry->Misc.VirtualSize;
            }

            //
            // If the pointer to raw data is zero and the virtual size
            // is zero, OR, the section goes past the end of file, OR
            // the virtual size does not match the size of raw data, then
            // return an error.
            //

            if (((SectionTableEntry->PointerToRawData !=
                  SectionTableEntry->VirtualAddress))
                        ||
                ((SectionTableEntry->SizeOfRawData +
                        SectionTableEntry->PointerToRawData) >
                     FileInfo.nFileSizeLow)
                        ||
               (SectionVirtualSize > SectionTableEntry->SizeOfRawData)) {

                LogPrintf("invalid BSS/Trailingzero section/file size\n");

                ImageOk = FALSE;
                goto NeImage;
            }
            SectionTableEntry += 1;
            NumberOfSubsections -= 1;
        }
        goto PeReturnSuccess;
    }

    while (NumberOfSubsections > 0) {

        //
        // Handle case where virtual size is 0.
        //

        if (SectionTableEntry->Misc.VirtualSize == 0) {
            SectionVirtualSize = SectionTableEntry->SizeOfRawData;
        } else {
            SectionVirtualSize = SectionTableEntry->Misc.VirtualSize;
        }

        if (!strcmp(SectionTableEntry->Name, ".debug")) {
            fDebugMapped = TRUE;
        }

        if (SectionVirtualSize == 0) {
            //
            // The specified virtual address does not align
            // with the next prototype PTE.
            //

            LogPrintf("Section virtual size is 0, NextVa for section %lx %lx\n",
                    SectionTableEntry->VirtualAddress, NextVa);
            ImageOk = FALSE;
            goto BadPeImageSegment;
        }

        if (NextVa !=
                (PreferredImageBase + SectionTableEntry->VirtualAddress)) {

            //
            // The specified virtual address does not align
            // with the next prototype PTE.
            //

            LogPrintf("Section Va not set to alignment, NextVa for section %lx %lx\n",
                    SectionTableEntry->VirtualAddress, NextVa);
            ImageOk = FALSE;
            goto BadPeImageSegment;
        }

        PtesInSubsection =
            MI_ROUND_TO_SIZE (SectionVirtualSize, ImageAlignment) >> PageShift;

        if (PtesInSubsection > NumberOfPtes) {

            //
            // Inconsistent image, size does not agree with object tables.
            //
            LogPrintf("Image size in header not consistent with sections, needs %ld. pages\n",
                PtesInSubsection - NumberOfPtes);
            LogPrintf("va of bad section %lx\n",SectionTableEntry->VirtualAddress);

            ImageOk = FALSE;
            goto BadPeImageSegment;
        }
        NumberOfPtes -= PtesInSubsection;

        StartingSector = SectionTableEntry->PointerToRawData >> MMSECTOR_SHIFT;
        EndingSector =
                         (SectionTableEntry->PointerToRawData +
                                     SectionVirtualSize);
        EndingSector = EndingSector >> MMSECTOR_SHIFT;

        ImageFileSize = SectionTableEntry->PointerToRawData +
                                    SectionTableEntry->SizeOfRawData;

        for (i = 0; i < PtesInSubsection; i++) {

            //
            // Set all the prototype PTEs to refer to the control section.
            //

            NextVa += PageSize;
        }

        SectionTableEntry += 1;
        NumberOfSubsections -= 1;
    }

    //
    // If the file size is not as big as the image claimed to be,
    // return an error.
    //

    if (ImageFileSize > FileInfo.nFileSizeLow) {

        //
        // Invalid image size.
        //

        LogPrintf("invalid image size - file size %lx - image size %lx\n",
            FileInfo.nFileSizeLow, ImageFileSize);
        ImageOk = FALSE;
        goto BadPeImageSegment;
    }

    {
        // Validate the debug information (as much as we can).
        PVOID ImageBase;
        ULONG DebugDirectorySize, NumberOfDebugDirectories, i;
        PIMAGE_DEBUG_DIRECTORY DebugDirectory;

        ImageBase = (PVOID) DosHeader;

        DebugDirectory = (PIMAGE_DEBUG_DIRECTORY)
            ImageDirectoryEntryToData(
                ImageBase,
                FALSE,
                IMAGE_DIRECTORY_ENTRY_DEBUG,
                &DebugDirectorySize );

        if (!DebugDirectoryIsUseful(DebugDirectory, DebugDirectorySize)) {

            // Not useful.  Are they valid? (both s/b zero)

            if (DebugDirectory || DebugDirectorySize) {
                LogPrintf("Debug directory values [%x, %x] are invalid\n",
                        DebugDirectory,
                        DebugDirectorySize);
                ImageOk = FALSE;
            }

            goto DebugDirsDone;
        }

        NumberOfDebugDirectories = DebugDirectorySize / sizeof( IMAGE_DEBUG_DIRECTORY );

        for (i=0; i < NumberOfDebugDirectories; i++) {
            if (DebugDirectory->PointerToRawData > FileInfo.nFileSizeLow) {
                LogPrintf("Invalid debug directory entry[%d] - File Offset %x is beyond the end of the file\n",
                        i,
                        DebugDirectory->PointerToRawData
                       );
                ImageOk = FALSE;
                goto BadPeImageSegment;
            }

            if ((DebugDirectory->PointerToRawData + DebugDirectory->SizeOfData) > FileInfo.nFileSizeLow) {
                LogPrintf("Invalid debug directory entry[%d] - File Offset (%X) + Size (%X) is beyond the end of the file (filesize: %X)\n",
                        i,
                        DebugDirectory->PointerToRawData,
                        DebugDirectory->SizeOfData,
                        FileInfo.nFileSizeLow
                       );
                ImageOk = FALSE;
                goto BadPeImageSegment;
            }

            if (DebugDirectory->AddressOfRawData != 0) {
                if (!fDebugMapped) {
                    LogPrintf("Invalid debug directory entry[%d] - VA is non-zero (%X), but no .debug section exists\n",
                            i,
                            DebugDirectory->AddressOfRawData);
                    ImageOk = FALSE;
                    goto BadPeImageSegment;
                }
                if (DebugDirectory->AddressOfRawData > ImageFileSize){
                    LogPrintf("Invalid debug directory entry[%d] - VA (%X) is beyond the end of the image VA (%X)\n",
                            i,
                            DebugDirectory->AddressOfRawData,
                            ImageFileSize);
                    ImageOk = FALSE;
                    goto BadPeImageSegment;
                }

                if ((DebugDirectory->AddressOfRawData + DebugDirectory->SizeOfData )> ImageFileSize){
                    LogPrintf("Invalid debug directory entry[%d] - VA (%X) + size (%X) is beyond the end of the image VA (%X)\n",
                            i,
                            DebugDirectory->AddressOfRawData,
                            DebugDirectory->SizeOfData,
                            ImageFileSize);
                    ImageOk = FALSE;
                    goto BadPeImageSegment;
                }
            }

            if (DebugDirectory->Type <= 0x7fffffff) {
                switch (DebugDirectory->Type) {
                    case IMAGE_DEBUG_TYPE_MISC:
                        {
                            PIMAGE_DEBUG_MISC pDebugMisc;
                            // MISC should point to an IMAGE_DEBUG_MISC structure
                            pDebugMisc = (PIMAGE_DEBUG_MISC)((PCHAR)ImageBase + DebugDirectory->PointerToRawData);
                            if (pDebugMisc->DataType != IMAGE_DEBUG_MISC_EXENAME) {
                                LogPrintf("MISC Debug has an invalid DataType\n");
                                ImageOk = FALSE;
                                goto BadPeImageSegment;
                            }
                            if (pDebugMisc->Length != DebugDirectory->SizeOfData) {
                                LogPrintf("MISC Debug has an invalid size.\n");
                                ImageOk = FALSE;
                                goto BadPeImageSegment;
                            }

                            if (!pDebugMisc->Unicode) {
                                i= 0;
                                while (i < pDebugMisc->Length - sizeof(IMAGE_DEBUG_MISC)) {
                                    if (!isprint(pDebugMisc->Data[i]) &&
                                        (pDebugMisc->Data[i] != '\0') )
                                    {
                                        LogPrintf("MISC Debug has unprintable characters... Possibly corrupt\n");
                                        ImageOk = FALSE;
                                        goto BadPeImageSegment;
                                    }
                                    i++;
                                }

                                // The data must be a null terminated string.
                                if (strlen(pDebugMisc->Data) > (pDebugMisc->Length - sizeof(IMAGE_DEBUG_MISC))) {
                                    LogPrintf("MISC Debug has invalid data... Possibly corrupt\n");
                                    ImageOk = FALSE;
                                    goto BadPeImageSegment;
                                }
                            }
                        }
                        break;

                    case IMAGE_DEBUG_TYPE_CODEVIEW:
                        // CV will point to either a NB09 or an NB10 signature.  Make sure it does.
                        {
                            OMFSignature * CVDebug;
                            CVDebug = (OMFSignature *)((PCHAR)ImageBase + DebugDirectory->PointerToRawData);
                            if (((*(PULONG)(CVDebug->Signature)) != '90BN') &&
                                ((*(PULONG)(CVDebug->Signature)) != '01BN'))
                            {
                                LogPrintf("CV Debug has an invalid signature\n");
                                ImageOk = FALSE;
                                goto BadPeImageSegment;
                            }
                        }
                        break;

                    case IMAGE_DEBUG_TYPE_COFF:
                    case IMAGE_DEBUG_TYPE_FPO:
                    case IMAGE_DEBUG_TYPE_EXCEPTION:
                    case IMAGE_DEBUG_TYPE_FIXUP:
                    case IMAGE_DEBUG_TYPE_OMAP_TO_SRC:
                    case IMAGE_DEBUG_TYPE_OMAP_FROM_SRC:
                        // Not much we can do about these now.
                        break;

                    default:
                        LogPrintf("Invalid debug directory type: %d\n", DebugDirectory->Type);
                        ImageOk = FALSE;
                        goto BadPeImageSegment;
                        break;
                }
            }
        }

    }

DebugDirsDone:

    //
    // The total number of PTEs was decremented as sections were built,
    // make sure that there are less than 64ks worth at this point.
    //

    if (NumberOfPtes >= (ImageAlignment >> PageShift)) {

        //
        // Inconsistent image, size does not agree with object tables.
        //

        LogPrintf("invalid image - PTEs left %lx\n",
            NumberOfPtes);

        ImageOk = FALSE;
        goto BadPeImageSegment;
    }

    //
    // check checksum.
    //

PeReturnSuccess:
    if (NtHeader->OptionalHeader.CheckSum == 0) {
        LogPrintf("(checksum is zero)\n");
    } else {
        __try {
            if (lpOldLdrVerifyImageMatchesChecksum == NULL) {
                if (lpNewLdrVerifyImageMatchesChecksum == NULL) {
                    Status = STATUS_SUCCESS;
                    LogPrintf("Unable to validate checksum\n");
                } else {
                    Status = (*lpNewLdrVerifyImageMatchesChecksum)(File, NULL, NULL, NULL);
                }
            } else {
                Status = (*lpOldLdrVerifyImageMatchesChecksum)(File);
            }

            if (NT_ERROR(Status)) {
                LogPrintf("checksum mismatch\n");
                ImageOk = FALSE;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            ImageOk = FALSE;
            LogPrintf("checksum mismatch\n");
        }
    }

    if (fHasPdata && ImageOk) {
        ImageOk = ValidatePdata(DosHeader);
    }

    if (ImageOk) {
        ImageOk = VerifyVersionResource(ImageName, ImageNeedsOleSelfRegister(DosHeader));
    }

    //
    // sanity test for symbols
    // basically : if this does not work, debugging probably will not either
    // these high-level debugging api's will also call a pdb validation routine
    //

    if(ArgFlag & ArgFlag_SymCK)
    {
        HANDLE hProcess = 0;
        char Target[MAX_PATH] = {0};
        char drive[_MAX_DRIVE];
        char dir[_MAX_DIR];
        char fname[_MAX_FNAME];
        char ext[_MAX_EXT];
        IMAGEHLP_MODULE64 ModuleInfo = {0};
        PSYMMODLIST ModList = 0;
        void *vpAddr;
        PLOADED_IMAGE pLImage = NULL;
        DWORD64 symLMflag;

        strcpy(Target, szDirectory);
        strcat(Target, "\\");
        strcat(Target, ImageName);

        //
        // set up for debugging
        //

        hProcess = GetCurrentProcess();

        if(!SymInitialize(hProcess, szSympath, FALSE))
        {
            LogPrintf("ERROR:SymInitialize failed!\n");
            hProcess = 0;
            goto symckend;
        }

        //
        // attempt to use symbols
        //

        _splitpath(Target, drive, dir, fname, ext );

        symLMflag = SymLoadModule64(hProcess, NULL, Target, fname, 0, 0);
        if(!symLMflag)
        {
            LogPrintf("ERROR:SymLoadModule failed! last error:0x%x\n", GetLastError());
            goto symckend;
        }

        //
        // identify module type
        // find module, symgetmoduleinfo, check dbg type
        //

        ModuleInfo.SizeOfStruct = sizeof(ModuleInfo);
        ModList = MakeModList(hProcess);
        vpAddr = GetModAddrFromName(ModList, fname);

        if(!SymGetModuleInfo64(hProcess, (DWORD64)vpAddr, &ModuleInfo))
        {
            LogPrintf("ERROR:SymGetModuleInfo failed! last error:0x%x\n", GetLastError());
            goto symckend;
        }

        if(ModuleInfo.SymType != SymPdb)
        {
            LogPrintf("WARNING: No pdb info for file!\n");
            switch(ModuleInfo.SymType){
                case SymNone:
                    LogPrintf("symtype: SymNone\n");
                    break;
                case SymCoff:
                    LogPrintf("symtype: SymCoff\n");
                    break;
                case SymCv:
                    LogPrintf("symtype: SymCv\n");
                    break;
                case SymPdb:
                    LogPrintf("symtype: SymPdb\n");
                    break;
                case SymExport:
                    LogPrintf("symtype: SymExport\n");
                    break;
                case SymDeferred:
                    LogPrintf("symtype: SymDeferred\n");
                    break;
                case SymSym:
                    LogPrintf("symtype: SymSym\n");
                    break;
            }
        }

        //
        // get image, symbol checksum, compare
        //

        pLImage = ImageLoad(Target, NULL);

        {
            CHAR szDbgPath[_MAX_PATH];
            HANDLE DbgFileHandle;

            DbgFileHandle = FindDebugInfoFile(Target, szSympath, szDbgPath);
            if (DbgFileHandle != INVALID_HANDLE_VALUE) {
                IMAGE_SEPARATE_DEBUG_HEADER DbgHeader;
                DWORD BytesRead;
                BOOL ReadSuccess;

                SetFilePointer(DbgFileHandle, 0, 0, FILE_BEGIN);
                ReadSuccess = ReadFile(DbgFileHandle, &DbgHeader, sizeof(DbgHeader), &BytesRead, NULL);

                if (ReadSuccess && (BytesRead == sizeof(DbgHeader))) {
                    // Got enough to check if it's a valid dbg file.
                    if(((PIMAGE_NT_HEADERS)pLImage->FileHeader)->OptionalHeader.CheckSum != DbgHeader.CheckSum) {
                        LogPrintf("ERROR! image / debug file checksum not equal\n");
                        ImageOk = FALSE;
                    }
                }
                CloseHandle(DbgFileHandle);
            }
        }

        //
        // cleanup
        //

symckend:
        if(ModList)
        {
            FreeModList(ModList);
        }
        if(pLImage)
        {
            ImageUnload(pLImage);
        }
        if(symLMflag)
        {
            SymUnloadModule64(hProcess, (DWORD)symLMflag);
        }
        if(hProcess)
        {
            SymCleanup(hProcess);
        }
    }

NextImage:
BadPeImageSegment:
NeImage:
    if ( ImageOk && (ArgFlag & ArgFlag_OK)) {
        if (MachineTypeMismatch) {
            LogPrintf(" OK [%s]\n", MachineType);
        } else {
            LogPrintf(" OK\n");
        }
    }

    //
    // print out results
    //

    if (ImageOk)
    {
        LogOutAndClean((ArgFlag & ArgFlag_OK) ? TRUE : FALSE);
    } else {
        LogOutAndClean(TRUE);
    }

    if ( File != INVALID_HANDLE_VALUE ) {
        CloseHandle(File);
    }
    if ( DosHeader ) {
        UnmapViewOfFile(DosHeader);
    }
}

NTSTATUS
MiVerifyImageHeader (
    IN PIMAGE_NT_HEADERS NtHeader,
    IN PIMAGE_DOS_HEADER DosHeader,
    IN ULONG NtHeaderSize
    )
/*++

Routine Description:

    Checks image header for consistency.

Arguments:

    IN PIMAGE_NT_HEADERS    NtHeader
    IN PIMAGE_DOS_HEADER    DosHeader
    IN ULONG                NtHeaderSize

Return Value:

    Returns the status value.

    TBS

--*/
{

    if ((NtHeader->FileHeader.Machine == 0) &&
        (NtHeader->FileHeader.SizeOfOptionalHeader == 0)) {

        //
        // This is a bogus DOS app which has a 32-bit portion
        // mascarading as a PE image.
        //

        LogPrintf("Image machine type and size of optional header bad\n");
        return STATUS_INVALID_IMAGE_PROTECT;
    }

    if (!(NtHeader->FileHeader.Characteristics & IMAGE_FILE_EXECUTABLE_IMAGE)) {
        LogPrintf("Characteristics not image file executable\n");
        return STATUS_INVALID_IMAGE_FORMAT;
    }

#ifdef i386

    //
    // Make sure the image header is aligned on a Long word boundary.
    //

    if (((ULONG)NtHeader & 3) != 0) {
        LogPrintf("NtHeader is not aligned on longword boundary\n");
        return STATUS_INVALID_IMAGE_FORMAT;
    }
#endif

    // Non-driver code must have file alignment set to a multiple of 512

    if (((NtHeader->OptionalHeader.FileAlignment & 511) != 0) &&
        (NtHeader->OptionalHeader.FileAlignment !=
         NtHeader->OptionalHeader.SectionAlignment)) {
        LogPrintf("file alignment is not multiple of 512 and power of 2\n");
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    //
    // File aligment must be power of 2.
    //

    if ((((NtHeader->OptionalHeader.FileAlignment << 1) - 1) &
        NtHeader->OptionalHeader.FileAlignment) !=
        NtHeader->OptionalHeader.FileAlignment) {
        LogPrintf("file alignment not power of 2\n");
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if (NtHeader->OptionalHeader.SectionAlignment < NtHeader->OptionalHeader.FileAlignment) {
        LogPrintf("SectionAlignment < FileAlignment\n");
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if (NtHeader->OptionalHeader.SizeOfImage > MM_SIZE_OF_LARGEST_IMAGE) {
        LogPrintf("Image too big %lx\n",NtHeader->OptionalHeader.SizeOfImage);
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if (NtHeader->FileHeader.NumberOfSections > MM_MAXIMUM_IMAGE_SECTIONS) {
        LogPrintf("Too many image sections %ld.\n",
                NtHeader->FileHeader.NumberOfSections);
        return STATUS_INVALID_IMAGE_FORMAT;
    }

    if (ArgFlag & ArgFlag_CKBase) {
       if ((PVOID)NtHeader->OptionalHeader.ImageBase >= HighestUserAddress) {
          LogPrintf("Image base (%lx) is invalid on this machine\n",
                NtHeader->OptionalHeader.ImageBase);
          return STATUS_SUCCESS;
       }
    }

    return STATUS_SUCCESS;
}


VOID
ParseArgs(
    int *pargc,
    char **argv
    )
/*++

Routine Description:

    parse arguments to this program

Arguments:

    int *pargc
    char **argv

Return Value:

    none

Notes:

    command line args:
    (original)
    case '?': call usage and exit
    case 'b': check whether base address of image is in user space for this machine
    case 's': /s <sympath> check symbols
    case 'p': PE Errors only
    case 'r': recurse subdirectories
    (new)
    case 'v': verbose - output "OK"
    case 'o': output "OleSelfRegister not set"

--*/
{
    CHAR cswitch, c, *p;
    CHAR sztmp[MAX_PATH];
    int argnum = 1, i=0, len=0, count=0;
    BOOL fslashfound = FALSE;

    //
    // set default flags here
    //

    ArgFlag |= ArgFlag_CKBase;

    while ( argnum < *pargc ) {
        _strlwr(argv[argnum]);
        cswitch = *(argv[argnum]);
        if (cswitch == '/' || cswitch == '-') {
            c = *(argv[argnum]+1);

            switch (c) {
                case 'o':
                    ArgFlag |= ArgFlag_OLESelf;
                    break;

                case 'v':
                    ArgFlag |= ArgFlag_OK | ArgFlag_CKMZ | ArgFlag_OLESelf;
                    break;

                case '?':
                    Usage();
                    break;

                case 'b':
                    ArgFlag ^= ArgFlag_CKBase;
                    break;

                case 's':
                    if (argv[argnum+1]) {
                        strcpy(szSympath, (argv[argnum+1]));
                        ArgFlag |= ArgFlag_SymCK;
                        argnum++;
                    }
                    break;

                case 'p':
                    ArgFlag |= ArgFlag_CKMZ;
                    break;

                case 'r':
                    fRecurse = TRUE;
                    if (argv[argnum+1]) {
                        fPathOverride=TRUE;
                        strcpy(szDirectory, (argv[argnum+1]));
                        if (!(strcmp(szDirectory, "\\"))) {  // if just '\'
                            fSingleSlash=TRUE;
                        }
                        //LogPrintf("dir %s\n", szDirectory);
                        argnum++;
                    }

                    break;

                default:
                    fprintf(stderr, "Invalid argument.\n");
                    Usage();
            }
        } else {
            // Check for path\filename or wildcards

            // Search for '\' in string
            strcpy(sztmp, (argv[argnum]));
            len = strlen(sztmp);
            for (i=0; i < len; i++) {
                if (sztmp[i]=='\\') {
                    count++;
                    endpath=i;         // mark last '\' char found
                    fslashfound=TRUE;  // found backslash, so must be a path\filename combination
                }
            }

            if (fslashfound && !fRecurse) { // if backslash found and not a recursive operation
                                            // seperate the directory and filename into two strings
                fPathOverride=TRUE;
                strcpy(szDirectory, sztmp);

                if (!(strcmp(szDirectory, "\\"))) {
                    Usage();
                }

                szFileName = _strdup(&(sztmp[endpath+1]));


                if (count == 1) { //&& szDirectory[1] == ':') { // if only one '\' char and drive letter indicated
                    fSingleSlash=TRUE;
                    szDirectory[endpath+1]='\0';  // keep trailing '\' in order to chdir properly
                }  else {
                    szDirectory[endpath]='\0';
                }

                if (szFileName[0] == '*' && szFileName[1] == '.' && szFileName[2] != '*') {
                    _strlwr(szFileName);
                    szPattern = strchr(szFileName, '.'); //search for '.'
                    fPattern = TRUE;
                }
            } else {  // no backslash found, assume filename without preceeding path

                szFileName = _strdup(argv[argnum]);
                //
                // filename or wildcard
                //
                if ( (*(argv[argnum]) == '*') && (*(argv[argnum]+1) == '.') && (*(argv[argnum]+2) != '*') ){
                    // *.xxx
                    _strlwr(szFileName);
                    szPattern = strchr(szFileName, '.'); //search for '.'
                    fPattern = TRUE;
                } else if ( (*(argv[argnum]) == '*') && (*(argv[argnum]+1) == '.') && (*(argv[argnum]+2) == '*') ) {
                    // *.*
                } else {
                    // probably a single filename
                    _strlwr(szFileName);
                    fSingleFile = TRUE;
                }

                if (fRecurse && strchr(szFileName, '\\') ) { // don't want path\filename when recursing
                    Usage();
                }

            }
            //fprintf(stdout, "dir %s\nfile %s\n", szDirectory, szFileName);
        }
        ++argnum;
    }
    if (szFileName[0] == '\0') {
        Usage();
    }
} // parseargs


int
__cdecl
CompFileAndDir(
    const void *elem1,
    const void *elem2
    )
/*++

Routine Description:

    Purpose: a comparision routine passed to QSort.  It compares elem1 and elem2
    based upon their attribute, i.e., is it a file or directory.

Arguments:

    const void *elem1,
    const void *elem2

Return Value:

    result of comparison function

Notes:


--*/
{
    pList p1, p2;
    // qsort passes a void universal pointer.  Use a typecast (List**)
    // so the compiler recognizes the data as a List structure.
    // Typecast pointer-to-pointer-to-List and dereference ONCE
    // leaving a pList.  I don't dereference the remaining pointer
    // in the p1 and p2 definitions to avoid copying the structure.

    p1 = (*(List**)elem1);
    p2 = (*(List**)elem2);

    if ( (p1->Attributes & FILE_ATTRIBUTE_DIRECTORY) &&  (p2->Attributes & FILE_ATTRIBUTE_DIRECTORY))
        return 0;
    //both dirs
    if (!(p1->Attributes & FILE_ATTRIBUTE_DIRECTORY) && !(p2->Attributes & FILE_ATTRIBUTE_DIRECTORY))
        return 0;
    //both files
    if ( (p1->Attributes & FILE_ATTRIBUTE_DIRECTORY) && !(p2->Attributes & FILE_ATTRIBUTE_DIRECTORY))
        return 1;
    // elem1 is dir and elem2 is file
    if (!(p1->Attributes & FILE_ATTRIBUTE_DIRECTORY) &&  (p2->Attributes & FILE_ATTRIBUTE_DIRECTORY))
        return -1;
    // elem1 is file and elem2 is dir

    return 0; // if none of the above
}

int
__cdecl
CompName(
    const void *elem1,
    const void *elem2
    )
/*++

Routine Description:

    another compare routine passed to QSort that compares the two Name strings

Arguments:

    const void *elem1,
    const void *elem2

Return Value:

    result of comparison function

Notes:

    this uses a noignore-case strcmp

--*/
{
   return strcmp( (*(List**)elem1)->Name, (*(List**)elem2)->Name );
}


VOID
Usage(
    VOID
    )
/*++

Routine Description:


Arguments:


Return Value:


Notes:


--*/
{
   fputs("Usage: imagechk  [/?] displays this message\n"
         "                 [/r dir] recurse from directory dir\n"
         "                 [/b] don't check image base address\n"
         "                 [/v] verbose - output everything\n"
         "                 [/o] output \"OleSelfRegister not set\" warning\n"
         "                 [/p] output \"MZ header not found\"\n"
         "                 [/s <sympath>] check pdb symbols\n"
         "                 [filename] file to check\n"
         " Accepts wildcard extensions such as *.exe\n"
         " imagechk /r . \"*.exe\"   check all *.exe recursing on current directory\n"
         " imagechk /r \\ \"*.exe\"  check all *.exe recursing from root of current drive\n"
         " imagechk \"*.exe\"        check all *.exe in current directory\n"
         " imagechk c:\\bar.exe      check c:\\bar.exe only\n",
         stderr);
   exit(1);
}

int
__cdecl
_cwild()
/*++

Routine Description:


Arguments:


Return Value:


Notes:


--*/
{
   return(0);
}

typedef DWORD (WINAPI *PFNGVS)(LPSTR, LPDWORD);
typedef BOOL (WINAPI *PFNGVI)(LPTSTR, DWORD, DWORD, LPVOID);
typedef BOOL (WINAPI *PFNVQV)(const LPVOID, LPTSTR, LPVOID *, PUINT);

BOOL
VerifyVersionResource(
    PCHAR FileName,
    BOOL fSelfRegister
    )
/*++

Routine Description:

    validate the version resource in a file

Arguments:

    PCHAR FileName
    BOOL fSelfRegister

Return Value:

    TRUE    if: no version.dll found
    FALSE   if: version resource missing


Notes:


--*/
{
    static HINSTANCE hVersion = NULL;
    static PFNGVS pfnGetFileVersionInfoSize = NULL;
    static PFNGVI pfnGetFileVersionInfo = NULL;
    static PFNVQV pfnVerQueryValue = NULL;
    DWORD dwSize;
    DWORD lpInfoSize;
    LPVOID lpData = NULL, lpInfo;
    BOOL rc = FALSE;
    DWORD dwDefLang = 0x00000409;
    DWORD *pdwTranslation, uLen;
    CHAR buf[60];

    CHAR szVersionDll[_MAX_PATH];

    if (GetSystemDirectory(szVersionDll, sizeof(szVersionDll))) {
        strcat(szVersionDll, "\\version.dll");
    } else {
        strcpy(szVersionDll, "version.dll");
    }

    if (!hVersion) {
        hVersion = LoadLibraryA(szVersionDll);
        if (hVersion == NULL) {
            return TRUE;
        }

        pfnGetFileVersionInfoSize = (PFNGVS) GetProcAddress(hVersion, "GetFileVersionInfoSizeA");
        pfnGetFileVersionInfo = (PFNGVI) GetProcAddress(hVersion, "GetFileVersionInfoA");
        pfnVerQueryValue = (PFNVQV) GetProcAddress(hVersion, "VerQueryValueA");
    }

    if (!pfnGetFileVersionInfoSize || !pfnGetFileVersionInfo || !pfnVerQueryValue) {
        rc = TRUE;
        goto cleanup;
    }

    if ((dwSize = (*pfnGetFileVersionInfoSize)(FileName, &dwSize)) == 0){
        LogPrintf("No version resource detected\n");
        goto cleanup;
    }

    if (!fSelfRegister) {
        // All we need to do is see if the version resource exists.  Ole Self Register not necessary.
        rc = TRUE;
        goto cleanup;
    }

    if ((lpData = malloc(dwSize)) == NULL) {
        LogPrintf("Out of memory\n");
        goto cleanup;
    }

    if (!(*pfnGetFileVersionInfo)(FileName, 0, dwSize, lpData)) {
        LogPrintf("Unable to read version info\n - %d", GetLastError());
        goto cleanup;
    }

    if(!pfnVerQueryValue(lpData, "\\VarFileInfo\\Translation", &pdwTranslation, &uLen)) {
        pdwTranslation = &dwDefLang;
        uLen = sizeof(DWORD);
    }

    sprintf(buf, "\\StringFileInfo\\%04x%04x\\OleSelfRegister", LOWORD(*pdwTranslation), HIWORD(*pdwTranslation));

    if (!pfnVerQueryValue(lpData, buf, &lpInfo, &lpInfoSize) && (ArgFlag & ArgFlag_OLESelf )) {
        LogPrintf("OleSelfRegister not set\n");
    } else {
        rc = TRUE;
    }

cleanup:
    if (lpData) {
        free(lpData);
    }

    // No need to free the hVersion
    return(rc);
}

BOOL
ValidatePdata(
    PIMAGE_DOS_HEADER DosHeader
    )
/*++

Routine Description:

    validates the PIMAGE_RUNTIME_FUNCTION_ENTRY in the executable

Arguments:

    PIMAGE_DOS_HEADER   DosHeader

Return Value:

    TRUE    if:
    FALSE   if: no exception data
                exception table size incorrect
                exception table corrupt

Notes:


--*/
{
    // The machine type indicates this image should have pdata (an exception table).
    // Ensure it looks reasonable.

    // Todo: Add a range check for exception handler and data

    PIMAGE_RUNTIME_FUNCTION_ENTRY ExceptionTable;
    DWORD ExceptionTableSize, i;
    DWORD_PTR LastEnd;
    BOOL fRc;
    PIMAGE_NT_HEADERS NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)DosHeader + (ULONG)DosHeader->e_lfanew);
    ULONG_PTR ImageBase = NtHeader->OptionalHeader.ImageBase;
    DWORD PDataStart = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].VirtualAddress;
    DWORD PDataSize = NtHeader->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_EXCEPTION].Size;


    ExceptionTable = (PIMAGE_RUNTIME_FUNCTION_ENTRY)
        ImageDirectoryEntryToData(
            DosHeader,
            FALSE,
            IMAGE_DIRECTORY_ENTRY_EXCEPTION,
            &ExceptionTableSize );

    if (!ExceptionTable ||
        (ExceptionTable && (ExceptionTableSize == 0)))
    {
        // No Exception table.
        return(TRUE);
    }

    if (ExceptionTableSize % sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY)) {
        // The size isn't an even multiple.
        LogPrintf("exception table size is not correct\n");
        return(FALSE);
    }

    LastEnd = 0;
    fRc = TRUE;
    for (i=0; i < (ExceptionTableSize / sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY)); i++) {

        if (!ExceptionTable[i].BeginAddress) {
            if (fRc != FALSE) {
                LogPrintf("exception table is corrupt.\n");
            }
            LogPrintf("PDATA Entry[%d]: zero value for BeginAddress\n",
                    i);
            fRc = FALSE;
        }
        if (!ExceptionTable[i].EndAddress) {
            if (fRc != FALSE) {
                LogPrintf("exception table is corrupt.\n");
            }
            LogPrintf("PDATA Entry[%d]: zero value for EndAddress\n",
                    i);
            fRc = FALSE;
        }
#if defined(_IA64_)
        if (!ExceptionTable[i].UnwindInfoAddress) {
            if (fRc != FALSE) {
                LogPrintf("exception table is corrupt.\n");
            }
            LogPrintf("PDATA Entry[%d]: zero value for UnwindInfoAddress\n",
                    i);
            fRc = FALSE;
        }
#else
        if (!ExceptionTable[i].PrologEndAddress) {
            if (fRc != FALSE) {
                LogPrintf("exception table is corrupt.\n");
            }
            LogPrintf("PDATA Entry[%d]: zero value for PrologEndAddress\n",
                    i);
            fRc = FALSE;
        }

#endif // defined(_IA64_)

        if (ExceptionTable[i].BeginAddress < LastEnd) {
            if (fRc != FALSE) {
                LogPrintf("exception table is corrupt.\n");
            }
            LogPrintf("PDATA Entry[%d]: the begin address [%8.8x] is out of sequence.  Prior end was [%8.8x]\n",
                    i,
                    ExceptionTable[i].BeginAddress,
                    LastEnd);
            fRc = FALSE;
        }

        if (ExceptionTable[i].EndAddress < ExceptionTable[i].BeginAddress) {
            if (fRc != FALSE) {
                LogPrintf("exception table is corrupt.\n");
            }
            LogPrintf("PDATA Entry[%d]: the end address [%8.8x] is before the begin address[%8.8X]\n",
                    i,
                    ExceptionTable[i].EndAddress,
                    ExceptionTable[i].BeginAddress);
            fRc = FALSE;
        }

#if !defined(_IA64_)
        if (!((ExceptionTable[i].PrologEndAddress >= ExceptionTable[i].BeginAddress) &&
              (ExceptionTable[i].PrologEndAddress <= ExceptionTable[i].EndAddress)))
        {
            if (NtHeader->FileHeader.Machine == IMAGE_FILE_MACHINE_ALPHA) {
                // Change this test.  On Alpha, the PrologEndAddress is allowed to be
                // outside the Function Start/End range.  If this is true, the PrologEnd
                // - ImageBase - pdata section VA s/b divisible by sizeof IMAGE_RUNTIME_FUNCTION_ENTRY
                // AND within the bounds of the PdataSize.  It's supposed to be an index into the
                // pdata data that descibes the real scoping function.

                LONG PrologAddress;
                PrologAddress = (LONG) (ExceptionTable[i].PrologEndAddress - ImageBase - PDataStart);
                if (PrologAddress % sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY)) {
                    if (fRc != FALSE) {
                        LogPrintf("exception table is corrupt.\n");
                    }
                    LogPrintf("PDATA Entry[%d]: the secondary prolog end address[%8.8x] does not evenly index into the exception table.\n",
                            i,
                            ExceptionTable[i].PrologEndAddress,
                            ExceptionTable[i].BeginAddress,
                            ExceptionTable[i].EndAddress
                            );
                    fRc = FALSE;
                } else {
                    if ((PrologAddress < 0) || (PrologAddress > (LONG)(PDataStart - sizeof(IMAGE_RUNTIME_FUNCTION_ENTRY)))) {
                        if (fRc != FALSE) {
                            LogPrintf("exception table is corrupt.\n");
                        }
                        LogPrintf("PDATA Entry[%d]: the secondary prolog end address[%8.8x] does not index into the exception table.\n",
                                i,
                                ExceptionTable[i].PrologEndAddress,
                                ExceptionTable[i].BeginAddress,
                                ExceptionTable[i].EndAddress
                                );
                        fRc = FALSE;
                    }
                }
            } else {
                if (fRc != FALSE) {
                    LogPrintf("exception table is corrupt.\n");
                }
                LogPrintf("PDATA Entry[%d]: the prolog end address[%8.8x] is not within the bounds of the frame [%8.8X] - [%8.8X]\n",
                        i,
                        ExceptionTable[i].PrologEndAddress,
                        ExceptionTable[i].BeginAddress,
                        ExceptionTable[i].EndAddress
                        );
                fRc = FALSE;
            }
        }
#endif // !defined(_IA64_)

        LastEnd = ExceptionTable[i].EndAddress;
    }

    return(fRc);
}

BOOL
ImageNeedsOleSelfRegister(
    PIMAGE_DOS_HEADER DosHeader
    )
/*++

Routine Description:


Arguments:

    PIMAGE_DOS_HEADER   DosHeader

Return Value:

    TRUE if DllRegisterServer or DllUnRegisterServer is exported

--*/
{
    PIMAGE_EXPORT_DIRECTORY ExportDirectory;
    PIMAGE_NT_HEADERS NtHeader;
    PIMAGE_SECTION_HEADER SectionHeader;
    DWORD ExportDirectorySize, i;
    USHORT x;
    PCHAR  rvaDelta;
    PULONG NameTable;

    ExportDirectory = (PIMAGE_EXPORT_DIRECTORY)
        ImageDirectoryEntryToData(
            DosHeader,
            FALSE,
            IMAGE_DIRECTORY_ENTRY_EXPORT,
            &ExportDirectorySize );

    if (!ExportDirectory ||
        !ExportDirectorySize ||
        !ExportDirectory->NumberOfNames)
    {
        // No exports (no directory, no size, or no names).
        return(FALSE);
    }

    // Walk the section headers and find the va/raw offsets.

    NtHeader = (PIMAGE_NT_HEADERS)((PCHAR)DosHeader + (ULONG)DosHeader->e_lfanew);
    SectionHeader = IMAGE_FIRST_SECTION(NtHeader);

    for (x = 0; x < NtHeader->FileHeader.NumberOfSections; x++) {
        if (((ULONG)((PCHAR)ExportDirectory - (PCHAR)DosHeader) >= SectionHeader->PointerToRawData) &&
            ((ULONG)((PCHAR)ExportDirectory - (PCHAR)DosHeader) <
                   (SectionHeader->PointerToRawData + SectionHeader->SizeOfRawData))) {
            break;
        } else {
            SectionHeader++;
        }
    }

    if (x == NtHeader->FileHeader.NumberOfSections) {
        // We didn't find the section that contained the export table.  Assume it's not there.
        return(FALSE);
    }

    rvaDelta = (PCHAR)DosHeader + SectionHeader->PointerToRawData - SectionHeader->VirtualAddress;

    NameTable = (PULONG)(rvaDelta + ExportDirectory->AddressOfNames);

    for (i = 0; i < ExportDirectory->NumberOfNames; i++) {
        if (!strcmp("DllRegisterServer", rvaDelta + NameTable[i]) ||
            !strcmp("DllUnRegisterServer", rvaDelta + NameTable[i]))
        {
            return(TRUE);
        }
    }

    return(FALSE);
}

//
// support routines for symbol checker - could all
// be done without this using lower-level internal api's
//

PSYMMODLIST
MakeModList(
    HANDLE hProcess
    )
/*++

Routine Description:

    build a list of loaded symbol modules and addresses

Arguments:

    HANDLE hProcess

Return Value:

    PSYMMODLIST

Notes:


--*/
{
    PSYMMODLIST ModList;

    ModList = (PSYMMODLIST)calloc(1, sizeof(SYMMODLIST));
    SymEnumerateModules64(hProcess, SymEnumerateModulesCallback, ModList);

    return(ModList);
}

BOOL
CALLBACK
SymEnumerateModulesCallback(
    LPSTR ModuleName,
    ULONG64 BaseOfDll,
    PVOID UserContext
    )
/*++

Routine Description:

    callback routine for SymEnumerateModules
    in this case, UserContext is a pointer to a head of a _SYMMODLIST struct
    that will have a new item appended
    We are avoiding global state for these lists so we can use several at once,
    they will be short, so we will find the end each time we want to add
    runs slower, simpler to maintain

Arguments:

    LPSTR   ModuleName
    ULONG64 BaseOfDll
    PVOID   UserContext

Return Value:

    TRUE

Notes:


--*/
{
    PSYMMODLIST pSymModList;

    //
    // find end of list, key on pSymModList->ModBase
    //

    pSymModList = (PSYMMODLIST)UserContext;
    while (pSymModList->ModBase)
    {
        pSymModList = pSymModList->Next;
    }

    //
    // append entry
    //

    pSymModList->ModName = malloc(strlen(ModuleName) + 1);
    strcpy(pSymModList->ModName, ModuleName);
    pSymModList->ModBase = (void *)BaseOfDll;
    pSymModList->Next = (PSYMMODLIST)calloc(1, sizeof(SYMMODLIST));

    return(TRUE);
}

void *
GetModAddrFromName(
    PSYMMODLIST ModList,
    char *ModName
    )
/*++

Routine Description:

    gets module address from a SYMMODLIST given module base name

Arguments:

    PSYMMODLIST ModList
    char *      ModName

Return Value:

    module address

--*/
{
    while (ModList->Next != 0)
    {
        if (strcmp(ModList->ModName, ModName) == 0)
        {
            break;
        }
        ModList = ModList->Next;
    }

    return(ModList->ModBase);
}

void
FreeModList(
    PSYMMODLIST ModList
    )
/*++

Routine Description:

    free a list of loaded symbol modules and addresses

Arguments:

    PSYMMODLIST ModList

Return Value:

    none

--*/
{
    PSYMMODLIST ModListNext;

    while (ModList)
    {
        if(ModList->ModName)
        {
            free(ModList->ModName);
        }
        ModListNext = ModList->Next;
        free(ModList);
        ModList = ModListNext;
    }
}

pLogListItem LogAppend(
    char *logitem,
    pLogListItem plog
    )
/*++

Routine Description:

    add a log line to the linked list of log lines

Arguments:

    char *  logitem     - a formatted line of text to be logged
    pLogListItem plog   - pointer to LogListItem

Return Value:

    a pointer to the LogListItem allocated
    the first call to this function should save this pointer and use
    it for the head of the list, and it should be used when calling
    LogOutAndClean() to print the list and free all the memory

    you can call this with plog == head of list, or == to last item
    if plog == 0, this means that the item being allocated is the head
    of the list.
    If plog == head of list, search for end of list
    if plog == last item allocated, then the search is much faster

--*/
{
    pLogListItem ptemp;

    ptemp = plog;
    if(plog)
    {
        while(ptemp->Next)
        {
            ptemp = ptemp->Next;
        }
    }

    if(!ptemp)
    {
        ptemp = (pLogListItem)calloc(sizeof(LogListItem), 1);
    } else {
        ptemp->Next = (pLogListItem)calloc(sizeof(LogListItem), 1);
        ptemp = ptemp->Next;
    }

    ptemp->LogLine = (char *)malloc(strlen(logitem) + 1);
    strcpy(ptemp->LogLine, logitem);
    return (ptemp);
}

void LogOutAndClean(
    BOOL print
    )
/*++

Routine Description:

    output the log output, and free all the items in the list

Arguments:

    none

Return Value:

    none

--*/
{
    pLogListItem ptemp;
    pLogListItem plog = pLogList;

    while(plog)
    {
        ptemp = plog;
        if(print)
        {
            fprintf(stderr, plog->LogLine);
        }
        plog = plog->Next;
        free(ptemp->LogLine);
        free(ptemp);
    }
    if(print)
    {
        fprintf(stderr, "\n");
    }

    pLogListTmp = pLogList = NULL;

}

void
LogPrintf(
    const char *format,
    ...
    )
/*++

Routine Description:

    logging wrapper for fprintf

Arguments:

    none

Return Value:

    none

--*/
{
    va_list arglist;
    char LogStr[1024];

    va_start(arglist, format);
    vsprintf(LogStr, format, arglist);

    if(pLogList == NULL)
    {
        //
        // initialize log
        //

        pLogListTmp = pLogList = LogAppend(LogStr, NULL);

    } else {

        //
        // append to log
        //

        pLogListTmp = LogAppend(LogStr, pLogListTmp);

    }
}
