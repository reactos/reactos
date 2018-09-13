/*++ BUILD Version: 0001
 *
 *  WOW v1.0
 *
 *  Copyright (c) 1993, Microsoft Corporation
 *
 *  WKLDEG.H
 *  WOW32 KRNL FAST SEGMENT LOADER
 *
 *  History:
 *  Created 4-Jan-1993 by Matthew Felton (mattfe)
--*/

#define BADPTR 0xDEADBEEF
#define FINDMAPFILECACHE(f)     FindMapFileCache(f)
#define ALLOCMAPFILECACHE()     AllocMapFileCache()
#define FREEMAPFILECACHE(h)     FreeMapFileCache(h)

#define MAX_MAPPED_FILES 12
#define CACHE_BYTE_THRESHOLD 64*1024
#define CACHE_READ_THRESHOLD 32*1024
#define CACHE_ACCESS_THRESHOLD 20

#define DOS_ATTR_MASK  0x0037 // File attribute bits which are the same
                              // for NT and DOS. See dem\dosdef.h

#define IS_ASCII_PATH_SEPARATOR(ch)     (((ch) == '/') || ((ch) == '\\'))


typedef struct _HMAPPEDFILEALIAS {         /* HMAPPEDFILEALIAS */
    struct  _HMAPPEDFILEALIAS *hpfNext;  // Pointer to Next MappedFileCacheEntry
    HANDLE  hfile32;
    HANDLE  hMappedFileObject;
    LPBYTE  lpStartingAddressOfView;
    DWORD   lFilePointer;
    DWORD   dwFileSize;
    BOOL    fAccess;
} HMAPPEDFILEALIAS, *PHMAPPEDFILEALIAS;

PHMAPPEDFILEALIAS FindMapFileCache(HANDLE hFile);
PHMAPPEDFILEALIAS AllocMapFileCache();

VOID    FreeMapFileCache(HANDLE hFile);
ULONG FASTCALL WK32WOWFileOpen(PVDMFRAME pFrame);
ULONG FASTCALL WK32WOWFileCreate(PVDMFRAME pFrame);
ULONG FASTCALL WK32WOWFileClose(PVDMFRAME pFrame);
ULONG FASTCALL WK32WOWFileGetAttributes(PVDMFRAME pFrame);
ULONG FASTCALL WK32WOWFileSetAttributes(PVDMFRAME pFrame);
ULONG FASTCALL WK32WOWFileGetDateTime(PVDMFRAME pFrame);
ULONG FASTCALL WK32WOWFileSetDateTime(PVDMFRAME pFrame);
ULONG FASTCALL WK32WOWFileLock(PVDMFRAME pFrame);
ULONG FASTCALL WK32WOWFileRead(PVDMFRAME pFrame);
ULONG FASTCALL WK32WOWFileWrite(PVDMFRAME pFrame);
ULONG FASTCALL WK32WOWFileLSeek(PVDMFRAME pFrame);
ULONG FASTCALL WK32WOWFindFirst(PVDMFRAME pFrame);
ULONG FASTCALL WK32WOWFindNext(PVDMFRAME pFrame);
ULONG FASTCALL WK32FindFirstFile(PVDMFRAME pFrame);
ULONG FASTCALL WK32FindNextFile(PVDMFRAME pFrame);
ULONG FASTCALL WK32FindClose(PVDMFRAME pFrame);

VOID    InitMapFileCache();
VOID    InsertMapFileCache( PHMAPPEDFILEALIAS pCache );
BOOL    W32MapViewOfFile( PHMAPPEDFILEALIAS pCache, HANDLE hFile);
VOID FlushMapFileCaches(VOID);
PSTR NormalizeDosPath(PSTR pszPath, WORD wCurrentDriveNumber, PBOOL ItsANamedPipe);

extern INT fileoclevel;
