/*++

Copyright (c) 1993  Microsoft Corporation

Module Name:

    checksum.c

Abstract:

    This module implements functions for splitting debugging information
    out of an image file and into a separate .DBG file.

Author:

    Steven R. Wood (stevewo) 4-May-1993

Revision History:

--*/

#include <private.h>
#include <symbols.h>

API_VERSION ApiVersion = { (VER_PRODUCTVERSION_W >> 8), (VER_PRODUCTVERSION_W & 0xff), API_VERSION_NUMBER, 0 };

///////////////////////////////////////////////////////////////////////
//
// DON'T UPDATE THIS VERSION NUMBER!!!!
//
// If the app does not call ImagehlpApiVersionEx, always assume
// that it is for NT 4.0.
//
API_VERSION AppVersion = { 4, 0, 5, 0 };
///////////////////////////////////////////////////////////////////////

#define ROUNDUP(x, y) ((x + (y-1)) & ~(y-1))

/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
#if !defined(_WIN64)

PIMAGE_DEBUG_INFORMATION
IMAGEAPI
MapDebugInformation(
    HANDLE FileHandle,
    LPSTR FileName,
    LPSTR SymbolPath,
    ULONG ImageBase
    )

// Here's what we're going to try.  MapDebugInformation was only
// documented as returning COFF symbolic and every user I can find
// in the tree uses COFF exclusively.  Rather than try to make this
// api do everything possible, let's just leave it as a COFF only thing.

// The new debug info api (ImgHlpFindDebugInfo) will be internal only.

{
    PIMAGE_DEBUG_INFORMATION pIDI;
    CHAR szName[_MAX_FNAME];
    CHAR szExt[_MAX_EXT];
    PIMGHLP_DEBUG_DATA pIDD;
    PPIDI              pPIDI;
    DWORD sections;
    MODULE_ENTRY       mi;
    BOOL               SymbolsLoaded;
    HANDLE             hProcess;
    LPSTR sz;
    HANDLE hdb;
    DWORD dw;
    hProcess = GetCurrentProcess();

    pIDD = ImgHlpFindDebugInfo(NULL, FileHandle, FileName, SymbolPath, ImageBase, NO_PE64_IMAGES);
    if (!pIDD)
        return NULL;

    pPIDI = MemAlloc(sizeof(PIDI));
    ZeroMemory(pPIDI, sizeof(PIDI));
    if (!pPIDI)
        return NULL;
    pIDI = &pPIDI->idi;
    pPIDI->hdr.pIDD = pIDD;

    pIDI->ReservedSize            = sizeof(IMAGE_DEBUG_INFORMATION);
    pIDI->ReservedMachine         = pIDD->Machine;
    pIDI->ReservedCharacteristics = (USHORT)pIDD->Characteristics;
    pIDI->ReservedCheckSum        = pIDD->CheckSum;
    pIDI->ReservedTimeDateStamp   = pIDD->TimeDateStamp;
    pIDI->ReservedRomImage        = pIDD->fROM;

    // read info

    InitializeListHead( &pIDI->List );
    pIDI->ImageBase = (ULONG)pIDD->ImageBaseFromImage;

    pIDI->ImageFilePath = MemAlloc(strlen(pIDD->ImageFilePath)+1);
    if (pIDI->ImageFilePath) {
        strcpy( pIDI->ImageFilePath, pIDD->ImageFilePath );
    }

    pIDI->ImageFileName = MemAlloc(strlen(pIDD->OriginalImageFileName)+1);
    if (pIDI->ImageFileName) {
        strcpy(pIDI->ImageFileName, pIDD->OriginalImageFileName);
    }

    if (pIDD->pMappedCoff) {
        pIDI->CoffSymbols = MemAlloc(pIDD->cMappedCoff);
        if (pIDI->CoffSymbols) {
            memcpy(pIDI->CoffSymbols, pIDD->pMappedCoff, pIDD->cMappedCoff);
        }
        pIDI->SizeOfCoffSymbols = pIDD->cMappedCoff;
    }

    if (pIDD->pFpo) {
        pIDI->ReservedNumberOfFpoTableEntries = pIDD->cFpo;
        pIDI->ReservedFpoTableEntries = (PFPO_DATA)pIDD->pFpo;
    }

    pIDI->SizeOfImage = pIDD->SizeOfImage;

    if (pIDD->DbgFilePath && *pIDD->DbgFilePath) {
        pIDI->ReservedDebugFilePath = MemAlloc(strlen(pIDD->DbgFilePath)+1);
        if (pIDI->ReservedDebugFilePath) {
            strcpy(pIDI->ReservedDebugFilePath, pIDD->DbgFilePath);
        }
    }

    if (pIDD->pMappedCv) {
        pIDI->ReservedCodeViewSymbols       = pIDD->pMappedCv;
        pIDI->ReservedSizeOfCodeViewSymbols = pIDD->cMappedCv;
    }

    // for backwards compatibility
    if (pIDD->ImageMap) {
        sections = (DWORD)((char *)pIDD->pCurrentSections - (char *)pIDD->ImageMap);
        pIDI->ReservedMappedBase = MapItRO(pIDD->ImageFileHandle);
        if (pIDI->ReservedMappedBase) {
            pIDI->ReservedSections = (PIMAGE_SECTION_HEADER)pIDD->pCurrentSections;
            pIDI->ReservedNumberOfSections = pIDD->cCurrentSections;
            if (pIDD->ddva) {
                pIDI->ReservedDebugDirectory = (PIMAGE_DEBUG_DIRECTORY)((PCHAR)pIDI->ReservedMappedBase + pIDD->ddva);
                pIDI->ReservedNumberOfDebugDirectories = pIDD->cdd;
            }
        }
    }

    return pIDI;
}

BOOL
UnmapDebugInformation(
    PIMAGE_DEBUG_INFORMATION pIDI
    )
{
    PPIDI pPIDI;

    if (!pIDI)
        return TRUE;

    if (pIDI->ImageFileName){
        MemFree(pIDI->ImageFileName);
    }

    if (pIDI->ImageFilePath) {
        MemFree(pIDI->ImageFilePath);
    }

    if (pIDI->ReservedDebugFilePath) {
        MemFree(pIDI->ReservedDebugFilePath);
    }

    if (pIDI->CoffSymbols) {
        MemFree(pIDI->CoffSymbols);
    }

    if (pIDI->ReservedMappedBase) {
        UnmapViewOfFile(pIDI->ReservedMappedBase);
    }

    pPIDI = (PPIDI)(PCHAR)((PCHAR)pIDI - sizeof(PIDI_HEADER));
    ImgHlpReleaseDebugInfo(pPIDI->hdr.pIDD, IMGHLP_FREE_ALL);
    MemFree(pPIDI);

    return TRUE;
}

#endif


LPSTR
ExpandPath(
    LPSTR lpPath
    )
{
    LPSTR   p, newpath, p1, p2, p3;
    CHAR    envvar[MAX_PATH];
    CHAR    envstr[MAX_PATH];
    ULONG   i, PathMax;

    if (!lpPath) {
        return(NULL);
    }

    p = lpPath;
    PathMax = strlen(lpPath) + MAX_PATH + 1;
    p2 = newpath = (LPSTR) MemAlloc( PathMax );

    if (!newpath) {
        return(NULL);
    }

    while( p && *p) {
        if (*p == '%') {
            i = 0;
            p++;
            while (p && *p && *p != '%') {
                envvar[i++] = *p++;
            }
            p++;
            envvar[i] = '\0';
            p1 = envstr;
            *p1 = 0;
            GetEnvironmentVariable( envvar, p1, MAX_PATH );
            while (p1 && *p1) {
                *p2++ = *p1++;
                if (p2 >= newpath + PathMax) {
                    PathMax += MAX_PATH;
                    p3 = MemReAlloc(newpath, PathMax);
                    if (!p3) {
                        MemFree(newpath);
                        return(NULL);
                    } else {
                        p2 = p3 + (p2 - newpath);
                        newpath = p3;
                    }
                }
            }
        }
        *p2++ = *p++;
        if (p2 >= newpath + PathMax) {
            PathMax += MAX_PATH;
            p3 = MemReAlloc(newpath, PathMax);
            if (!p3) {
                MemFree(newpath);
                return(NULL);
            } else {
                p2 = p3 + (p2 - newpath);
                newpath = p3;
            }
        }
    }
    *p2 = '\0';

    return newpath;
}


BOOL
IMAGEAPI
FindFileInSearchPath(
    HANDLE hprocess,
    LPSTR  SearchPath,
    LPSTR  FileName,
    DWORD  one,
    DWORD  two,
    DWORD  three,
    LPSTR  FilePath
    )
{
    PPROCESS_ENTRY  ProcessEntry;
    char path[MAX_PATH];
    char szpath[MAX_PATH * 4];
    LPSTR emark;
    LPSTR spath;

    *FilePath = 0;

    // setup local copy of the symbol path

    *szpath = 0;

    if (!SearchPath & !*SearchPath) {
        if (hprocess) {
            ProcessEntry = FindProcessEntry(hprocess);
            if (ProcessEntry && ProcessEntry->SymbolSearchPath) {
                strcpy(szpath, ProcessEntry->SymbolSearchPath);
            }
        }
    } else {
        strcpy(szpath, SearchPath);
    }

    if (!*szpath) {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
    
    spath = szpath;

    // for each node in the search path, look
    // for the file, or for it's symsrv entry

    do {
        emark = strchr(spath, ';');
        
        if (emark) {
            *emark = 0;
        }
 
        if (!_strnicmp(spath, "SYMSRV*", 7)) {

            GetSymbolFileFromServer(spath, 
                                    FileName,
                                    one,
                                    two,
                                    three,
                                    FilePath);
            if (*FilePath) 
                break;
                
        } else {

            strcpy(path, spath);
            EnsureTrailingBackslash(path);
            strcat(path, FileName);
            if (GetFileAttributes(path) != 0xFFFFFFFF) {
                strcpy(FilePath, path);
                break;
            }
        }

        if (emark) {
            *emark = ';';
             emark++;
        }
        spath = emark;
    } while (emark);

    return (*FilePath) ? TRUE : FALSE;
}


HANDLE
IMAGEAPI
FindExecutableImage(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath
    )
{
    return FindExecutableImageEx(FileName, SymbolPath, ImageFilePath, NULL, NULL);
}


HANDLE
IMAGEAPI
FindExecutableImageEx(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR ImageFilePath,
    PFIND_EXE_FILE_CALLBACK Callback,
    PVOID CallerData
    )
{
    LPSTR Start, End;
    HANDLE FileHandle = NULL;
    UCHAR DirectoryPath[ MAX_PATH ];
    LPSTR NewSymbolPath = NULL;

    __try {
        __try {
            NewSymbolPath = ExpandPath(SymbolPath);

            if (GetFullPathName( FileName, MAX_PATH, ImageFilePath, &Start )) {
                DPRINTF(NULL, "FindExecutableImageEx-> Looking for %s... ", ImageFilePath);
                FileHandle = CreateFile( ImageFilePath,
                                         GENERIC_READ,
                                         OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ? (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE) : (FILE_SHARE_READ | FILE_SHARE_WRITE),
                                         NULL,
                                         OPEN_EXISTING,
                                         0,
                                         NULL
                                       );

                if (FileHandle != INVALID_HANDLE_VALUE) {
                    if (Callback) {
                        if (!Callback(FileHandle, ImageFilePath, CallerData)) {
                            EPRINTF(NULL, "mismatched timestamp\n");
                            CloseHandle(FileHandle);
                            FileHandle = INVALID_HANDLE_VALUE;
                        }
                    }
                    if (FileHandle != INVALID_HANDLE_VALUE) {
                        EPRINTF(NULL, "OK\n");
                        MemFree( NewSymbolPath );
                        return FileHandle;
                    }
                } else {
                    EPRINTF(NULL, "no file\n");
                }
            }

            Start = NewSymbolPath;
            while (Start && *Start != '\0') {
                if (End = strchr( Start, ';' )) {
                    int Len = (int)(End - Start);
                    Len = min(Len, sizeof(DirectoryPath)-1);

                    strncpy( (PCHAR) DirectoryPath, Start, Len );
                    DirectoryPath[ Len ] = '\0';
                    End += 1;
                } else {
                    strcpy( (PCHAR) DirectoryPath, Start );
                }

                if (!_strnicmp(DirectoryPath, "SYMSRV*", 7)) {
                    goto next;
                }

                DPRINTF(NULL, "FindExecutableImageEx-> Searching %s for %s... ", DirectoryPath, FileName);
                if (SearchTreeForFile( (PCHAR) DirectoryPath, FileName, ImageFilePath )) {
                    EPRINTF(NULL, "found\n");
                    DPRINTF(NULL, "FindExecutableImageEx-> Opening %s... ", ImageFilePath);
                    FileHandle = CreateFile( ImageFilePath,
                                             GENERIC_READ,
                                             OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT ? (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE) : (FILE_SHARE_READ | FILE_SHARE_WRITE),
                                             NULL,
                                             OPEN_EXISTING,
                                             0,
                                             NULL
                                           );

                    if (FileHandle != INVALID_HANDLE_VALUE) {
                        if (Callback) {
                            if (!Callback(FileHandle, ImageFilePath, CallerData)) {
                                EPRINTF(NULL, "mismatched timestamp\n");
                                CloseHandle(FileHandle);
                                FileHandle = INVALID_HANDLE_VALUE;
                            }
                        }
                        if (FileHandle != INVALID_HANDLE_VALUE) {
                            EPRINTF(NULL, "OK\n");
                            MemFree( NewSymbolPath );
                            return FileHandle;
                        }
                    } else {
                        EPRINTF(NULL, "no file\n");
                    }
                } else {
                    EPRINTF(NULL, "no file\n");
                }

next:
                Start = End;
            }
        } __except (EXCEPTION_EXECUTE_HANDLER) {
            SetLastError(ERROR_INVALID_PARAMETER);
        }

        ImageFilePath[0] = '\0';

    } __except (EXCEPTION_EXECUTE_HANDLER) {
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    if (FileHandle) {
        CloseHandle(FileHandle);
    }

    if (NewSymbolPath) {
        MemFree( NewSymbolPath );
    }

    return NULL;
}


HANDLE
IMAGEAPI
FindDebugInfoFile(
    LPSTR FileName,
    LPSTR SymbolPath,
    LPSTR DebugFilePath
    )
{
    return FindDebugInfoFileEx(FileName, SymbolPath, DebugFilePath, NULL, NULL);
}


HANDLE
IMAGEAPI
FindDebugInfoFileEx(
    IN  LPSTR FileName,
    IN  LPSTR SymbolPath,
    OUT LPSTR DebugFilePath,
    IN  PFIND_DEBUG_FILE_CALLBACK Callback,
    IN  PVOID CallerData
    )
{
    return fnFindDebugInfoFileEx(FileName,
                                 SymbolPath,
                                 DebugFilePath,
                                 Callback,
                                 CallerData,
                                 0);
}


HANDLE
IMAGEAPI
fnFindDebugInfoFileEx(
    IN  LPSTR FileName,
    IN  LPSTR SymbolPath,
    OUT LPSTR DebugFilePath,
    IN  PFIND_DEBUG_FILE_CALLBACK Callback,
    IN  PVOID CallerData,
    IN  DWORD flag
    )
/*++

Routine Description:

 The rules are:
   Look for
     1. <SymbolPath>\Symbols\<ext>\<filename>.dbg
     3. <SymbolPath>\<ext>\<filename>.dbg
     5. <SymbolPath>\<filename>.dbg
     7. <FileNamePath>\<filename>.dbg

Arguments:
    FileName - Supplies a file name in one of three forms: fully qualified,
                <ext>\<filename>.dbg, or just filename.dbg
    SymbolPath - semi-colon delimited

    DebugFilePath -

    Callback - May be NULL. Callback that indicates whether the Symbol file is valid, or whether
        the function should continue searching for another Symbol file.
        The callback returns TRUE if the Symbol file is valid, or FALSE if the function should
        continue searching.

    CallerData - May be NULL. Data passed to the callback.

    Flag - indicates that PDBs shouldn't be searched for

Return Value:

  The name of the Symbol file (either .dbg or .sym) and a handle to that file.

--*/
{
    HANDLE FileHandle = INVALID_HANDLE_VALUE;
    LPSTR ExpSymbolPath = NULL, SymPathStart, PathEnd;
    DWORD ShareAttributes, cnt;
    LPSTR InitialPath, Sub1, Sub2, FileExt;
    CHAR FilePath[_MAX_PATH + 1];
    CHAR Drive[_MAX_DRIVE], Dir[_MAX_DIR], SubDirPart[_MAX_DIR], FilePart[_MAX_FNAME], Ext[_MAX_EXT];
    CHAR *ExtDir;
    DWORD i;
    PIMGHLP_DEBUG_DATA pIDD;
    BOOL  found = FALSE;
    BOOL  symsrv = TRUE;
    DWORD err = 0;

    if (OSVerInfo.dwPlatformId == VER_PLATFORM_WIN32_NT) {
        ShareAttributes = (FILE_SHARE_DELETE | FILE_SHARE_READ | FILE_SHARE_WRITE);
    } else {
        ShareAttributes = (FILE_SHARE_READ | FILE_SHARE_WRITE);
    }

    __try {
        *DebugFilePath = '\0';

        // Step 1.  What do we have?
        _splitpath(FileName, Drive, Dir, FilePart, Ext);

        if (!_stricmp(Ext, ".dbg")) {
            // We got a filename of the form: ext\filename.dbg.  Dir holds the extension already.
            ExtDir = Dir;
        } else {
            // Otherwise, skip the period and null out the Dir.
            ExtDir = CharNext(Ext);
        }

        ExpSymbolPath = ExpandPath(SymbolPath);
        SymPathStart = ExpSymbolPath;
        cnt = 0;

        do {
            if (PathEnd = strchr( SymPathStart, ';' )) {
                *PathEnd = '\0';
            }

            if (!_strnicmp(SymPathStart, "SYMSRV*", 7)) {

                *DebugFilePath = 0;
                if (symsrv && CallerData) {
                    strcpy(FilePath, FilePart);
                    strcat(FilePath, ".dbg");
                    pIDD = (PIMGHLP_DEBUG_DATA)CallerData;
                    GetSymbolFileFromServer(SymPathStart,
                                            FilePath,
                                            pIDD->TimeDateStamp,
                                            pIDD->SizeOfImage,
                                            0,
                                            DebugFilePath);
                    symsrv = FALSE;
                }

            } else {

                switch (cnt) {

                case 0: // <SymbolPath>\symbols\<ext>\<filename>.ext
                    InitialPath = SymPathStart;
                    Sub1 = "symbols";
                    Sub2 = ExtDir;
                    break;

                case 1: // <SymbolPath>\<ext>\<filename>.ext
                    InitialPath = SymPathStart;
                    Sub1 = "";
                    Sub2 = ExtDir;
                    break;

                case 2: // <SymbolPath>\<filename>.ext
                    InitialPath = SymPathStart;
                    Sub1 = "";
                    Sub2 = "";
                    break;

                case 3: // <FileNamePath>\<filename>.ext - A.K.A. what was passed to us
                    InitialPath = Drive;
                    Sub1 = "";
                    Sub2 = Dir;
                    // this stops us from checking out everything in the sympath
                    cnt++;
                    break;
                }

               // build fully-qualified filepath to look for

                strcpy(FilePath, InitialPath);
                EnsureTrailingBackslash(FilePath);
                strcat(FilePath, Sub1);
                EnsureTrailingBackslash(FilePath);
                strcat(FilePath, Sub2);
                EnsureTrailingBackslash(FilePath);
                strcat(FilePath, FilePart);

                strcpy(DebugFilePath, FilePath);
                strcat(DebugFilePath, ".dbg");
            }

            // try to open the file

            if (*DebugFilePath) {
                DPRINTF(NULL, "FindDebugInfoFileEx-> Looking for %s... ", DebugFilePath);
                FileHandle = CreateFile(DebugFilePath,
                                        GENERIC_READ,
                                        ShareAttributes,
                                        NULL,
                                        OPEN_EXISTING,
                                        0,
                                        NULL);

                // if the file opens, bail from this loop

                if (FileHandle != INVALID_HANDLE_VALUE) {
                    found = TRUE;
                    if (!Callback) {
                        break;
                    } else if (Callback(FileHandle, DebugFilePath, CallerData)) {
                        break;
                    } else {
                        EPRINTF(NULL, "mismatched timestamp\n");
                        CloseHandle(FileHandle);
                        FileHandle = INVALID_HANDLE_VALUE;
                    }
                } else {
                    err = GetLastError();
                    switch (err)
                    {
                    case ERROR_FILE_NOT_FOUND:
                        EPRINTF(NULL, "file not found\n");
                        break;
                    case ERROR_PATH_NOT_FOUND:
                        EPRINTF(NULL, "path not found\n");
                        break;
                    default:
                        EPRINTF(NULL, "file error 0x%x\n", err);
                        break;
                    }
                }
                // if file is open, bail from this loop too - else continue

                if (FileHandle != INVALID_HANDLE_VALUE)
                    break;
            }

            // go to next item in the sympath

            if (PathEnd) {
                *PathEnd = ';';
                SymPathStart = PathEnd + 1;
                symsrv = TRUE;
            } else {
                SymPathStart = ExpSymbolPath;
                cnt++;
            }
        } while (cnt < 4);

    } __except(EXCEPTION_EXECUTE_HANDLER) {
        if (FileHandle != INVALID_HANDLE_VALUE) {
            CloseHandle(FileHandle);
        }
        FileHandle = INVALID_HANDLE_VALUE;
    }

    if (ExpSymbolPath) {
        MemFree(ExpSymbolPath);
    }

    if (FileHandle == INVALID_HANDLE_VALUE) {
        FileHandle = NULL;
        DebugFilePath[0] = '\0';
    } else {
        EPRINTF(NULL, "OK\n");
    }

    if (!FileHandle                 // if we didn't get the right file...
        && found                    // but we found some file...
        && (flag & fdifRECURSIVE))  // and we were told to run recursively...
    {
        // try again without timestamp checking
        FileHandle = fnFindDebugInfoFileEx(FileName,
                                           SymbolPath,
                                           FilePath,
                                           NULL,
                                           0,
                                           flag);
        if (FileHandle && FileHandle != INVALID_HANDLE_VALUE)
            strcpy(DebugFilePath, FilePath);
    }

    return FileHandle;
}


BOOL
GetImageNameFromMiscDebugData(
    IN  HANDLE FileHandle,
    IN  PVOID MappedBase,
    IN  PIMAGE_NT_HEADERS32 NtHeaders,
    IN  PIMAGE_DEBUG_DIRECTORY DebugDirectories,
    IN  ULONG NumberOfDebugDirectories,
    OUT LPSTR ImageFilePath
    )
{
    IMAGE_DEBUG_MISC TempMiscData;
    PIMAGE_DEBUG_MISC DebugMiscData;
    ULONG BytesToRead, BytesRead;
    BOOLEAN FoundImageName;
    LPSTR ImageName;
    PIMAGE_OPTIONAL_HEADER32 OptionalHeader32 = NULL;
    PIMAGE_OPTIONAL_HEADER64 OptionalHeader64 = NULL;

    while (NumberOfDebugDirectories) {
        if (DebugDirectories->Type == IMAGE_DEBUG_TYPE_MISC) {
            break;
        } else {
            DebugDirectories += 1;
            NumberOfDebugDirectories -= 1;
        }
    }

    if (NumberOfDebugDirectories == 0) {
        return FALSE;
    }

    OptionalHeadersFromNtHeaders(NtHeaders, &OptionalHeader32, &OptionalHeader64);

    if ((OPTIONALHEADER(MajorLinkerVersion) < 3) &&
        (OPTIONALHEADER(MinorLinkerVersion) < 36) ) {
        BytesToRead = FIELD_OFFSET( IMAGE_DEBUG_MISC, Reserved );
    } else {
        BytesToRead = FIELD_OFFSET( IMAGE_DEBUG_MISC, Data );
    }

    DebugMiscData = NULL;
    FoundImageName = FALSE;
    if (MappedBase == 0) {
        if (SetFilePointer( FileHandle,
                            DebugDirectories->PointerToRawData,
                            NULL,
                            FILE_BEGIN
                          ) == DebugDirectories->PointerToRawData
           ) {
            if (ReadFile( FileHandle,
                          &TempMiscData,
                          BytesToRead,
                          &BytesRead,
                          NULL
                        ) &&
                BytesRead == BytesToRead
               ) {
                DebugMiscData = &TempMiscData;
                if (DebugMiscData->DataType == IMAGE_DEBUG_MISC_EXENAME) {
                    BytesToRead = DebugMiscData->Length - BytesToRead;
                    BytesToRead = BytesToRead > MAX_PATH ? MAX_PATH : BytesToRead;
                    if (ReadFile( FileHandle,
                                  ImageFilePath,
                                  BytesToRead,
                                  &BytesRead,
                                  NULL
                                ) &&
                        BytesRead == BytesToRead
                       ) {
                            FoundImageName = TRUE;
                    }
                }
            }
        }
    } else {
        DebugMiscData = (PIMAGE_DEBUG_MISC)((PCHAR)MappedBase +
                                            DebugDirectories->PointerToRawData );
        if (DebugMiscData->DataType == IMAGE_DEBUG_MISC_EXENAME) {
            ImageName = (PCHAR)DebugMiscData + BytesToRead;
            BytesToRead = DebugMiscData->Length - BytesToRead;
            BytesToRead = BytesToRead > MAX_PATH ? MAX_PATH : BytesToRead;
            if (*ImageName != '\0' ) {
                memcpy( ImageFilePath, ImageName, BytesToRead );
                FoundImageName = TRUE;
            }
        }
    }

    return FoundImageName;
}



#define MAX_DEPTH 32

BOOL
IMAGEAPI
SearchTreeForFile(
    LPSTR RootPath,
    LPSTR InputPathName,
    LPSTR OutputPathBuffer
    )
{
    // UnSafe...

    PCHAR FileName;
    PUCHAR Prefix = (PUCHAR) "";
    CHAR PathBuffer[ MAX_PATH ];
    ULONG Depth;
    PCHAR PathTail[ MAX_DEPTH ];
    PCHAR FindHandle[ MAX_DEPTH ];
    LPWIN32_FIND_DATA FindFileData;
    UCHAR FindFileBuffer[ MAX_PATH + sizeof( WIN32_FIND_DATA ) ];
    BOOL Result;

    strcpy( PathBuffer, RootPath );
    FileName = InputPathName;
    while (*InputPathName) {
        if (*InputPathName == ':' || *InputPathName == '\\' || *InputPathName == '/') {
            FileName = ++InputPathName;
        } else {
            InputPathName = CharNext(InputPathName);
        }
    }
    FindFileData = (LPWIN32_FIND_DATA)FindFileBuffer;
    Depth = 0;
    Result = FALSE;
    while (TRUE) {
startDirectorySearch:
        PathTail[ Depth ] = strchr( PathBuffer, '\0' );
        if (PathTail[ Depth ] > PathBuffer
            && *CharPrev(PathBuffer, PathTail[ Depth ]) != '\\') {
            *(PathTail[ Depth ])++ = '\\';
        }

        strcpy( PathTail[ Depth ], "*.*" );
        FindHandle[ Depth ] = (PCHAR) FindFirstFile( PathBuffer, FindFileData );

        if (FindHandle[ Depth ] == INVALID_HANDLE_VALUE) {
            return FALSE;
        }

        do {
            if (FindFileData->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) {
                if (strcmp( FindFileData->cFileName, "." ) &&
                    strcmp( FindFileData->cFileName, ".." ) &&
                    Depth < MAX_DEPTH
                   ) {
                        strcpy(PathTail[ Depth ], FindFileData->cFileName);
                        strcat(PathTail[ Depth ], "\\");

                        Depth++;
                        goto startDirectorySearch;
                }
            } else
            if (!_stricmp( FindFileData->cFileName, FileName )) {
                strcpy( PathTail[ Depth ], FindFileData->cFileName );
                strcpy( OutputPathBuffer, PathBuffer );
                Result = TRUE;
            }

restartDirectorySearch:
            if (Result) {
                break;
            }
        }
        while (FindNextFile( FindHandle[ Depth ], FindFileData ));
        FindClose( FindHandle[ Depth ] );

        if (Depth == 0) {
            break;
        }

        Depth--;
        goto restartDirectorySearch;
    }

    return Result;
}


BOOL
IMAGEAPI
MakeSureDirectoryPathExists(
    LPCSTR DirPath
    )
{
    LPSTR p, DirCopy;
    DWORD dw;

    // Make a copy of the string for editing.

    __try {
        DirCopy = (LPSTR) MemAlloc(strlen(DirPath) + 1);

        if (!DirCopy) {
            return FALSE;
        }

        strcpy(DirCopy, DirPath);

        p = DirCopy;

        //  If the second character in the path is "\", then this is a UNC
        //  path, and we should skip forward until we reach the 2nd \ in the path.

        if ((*p == '\\') && (*(p+1) == '\\')) {
            p++;            // Skip over the first \ in the name.
            p++;            // Skip over the second \ in the name.

            //  Skip until we hit the first "\" (\\Server\).

            while (*p && *p != '\\') {
                p = CharNext(p);
            }

            // Advance over it.

            if (*p) {
                p++;
            }

            //  Skip until we hit the second "\" (\\Server\Share\).

            while (*p && *p != '\\') {
                p = CharNext(p);
            }

            // Advance over it also.

            if (*p) {
                p++;
            }

        } else
        // Not a UNC.  See if it's <drive>:
        if (*(p+1) == ':' ) {

            p++;
            p++;

            // If it exists, skip over the root specifier

            if (*p && (*p == '\\')) {
                p++;
            }
        }

        while( *p ) {
            if ( *p == '\\' ) {
                *p = '\0';
                dw = GetFileAttributes(DirCopy);
                // Nothing exists with this name.  Try to make the directory name and error if unable to.
                if ( dw == 0xffffffff ) {
                    if ( !CreateDirectory(DirCopy,NULL) ) {
                        if( GetLastError() != ERROR_ALREADY_EXISTS ) {
                            MemFree(DirCopy);
                            return FALSE;
                        }
                    }
                } else {
                    if ( (dw & FILE_ATTRIBUTE_DIRECTORY) != FILE_ATTRIBUTE_DIRECTORY ) {
                        // Something exists with this name, but it's not a directory... Error
                        MemFree(DirCopy);
                        return FALSE;
                    }
                }

                *p = '\\';
            }
            p = CharNext(p);
        }
    } __except (EXCEPTION_EXECUTE_HANDLER) {
        ImagepSetLastErrorFromStatus( GetExceptionCode() );
        MemFree(DirCopy);
        return(FALSE);
    }

    MemFree(DirCopy);
    return TRUE;
}

LPAPI_VERSION
IMAGEAPI
ImagehlpApiVersion(
    VOID
    )
{
    //
    // don't tell old apps about the new version.  It will
    // just scare them.
    //
    return &AppVersion;
}

LPAPI_VERSION
IMAGEAPI
ImagehlpApiVersionEx(
    LPAPI_VERSION av
    )
{
    __try {
        AppVersion = *av;
    } __except (EXCEPTION_EXECUTE_HANDLER) {
    }

    if (AppVersion.Revision < 6) {
        //
        // For older debuggers, just tell them what they want to hear.
        //
        ApiVersion = AppVersion;
    }
    return &ApiVersion;
}

