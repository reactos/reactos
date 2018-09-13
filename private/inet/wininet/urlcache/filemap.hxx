/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    filemap.hxx

Abstract:

    contains class definitions for memory mapped file (MEMMAP_FILE)
    class objects.

Author:

    Madan Appiah (madana)  28-April-1995

Environment:

    User Mode - Win32

Revision History:

--*/

#define NUM_BITS_IN_DWORD   (sizeof(DWORD) * 8)

/*++

Class Description:

    Class that maps the URL object containter to a memory mapped file.

Private Member functions:

    ValidateCache : this private function validate the cache when the
        cache is read to memory first time.

Public Member functions:

    GetStatus : returns status of the object.

    AllocateEntry
    FreeEntry

    FindNextEntry : Find next entry in the URL list.

    GetHeapStart : returns start of the virtual memory address.

--*/

//
// ----------------- Memory map file header -----------------------------//
//

#define DIR_NAME_SIZE 8

struct CacheDir
{
    DWORD nFileCount;
    CHAR sDirName[DIR_NAME_SIZE];
};

typedef struct _MEMMAP_HEADER_SMALL 
{

    TCHAR    FileSignature[MAX_SIG_SIZE];
    DWORD    FileSize;
    DWORD    dwHashTableOffset;
    DWORD    NumUrlInternalEntries;
    DWORD    NumUrlEntriesAlloced;
//  DWORD    dwGarbage; // due to /Zp8 struct alignment
    LONGLONG CacheLimit;
    LONGLONG CacheSize;
    LONGLONG ExemptUsage;
    DWORD    nDirCount;
    CacheDir DirArray[DEFAULT_MAX_DIRS];
    DWORD    dwHeaderData[NUM_HEADER_DATA_DWORDS];
} MEMMAP_HEADER_SMALL;

#define BIT_MAP_ARRAY_SIZE (((HEADER_ENTRY_SIZE - sizeof(MEMMAP_HEADER_SMALL)))/sizeof(DWORD))

typedef struct _MEMMAP_HEADER : _MEMMAP_HEADER_SMALL 
{
    DWORD AllocationBitMap[BIT_MAP_ARRAY_SIZE];

} MEMMAP_HEADER, *LPMEMMAP_HEADER;


class MEMMAP_FILE 
{

private:

    DWORD _Status;

    //
    // parameters that passed as object create.
    //

    DWORD _EntrySize;
    LPTSTR _FullPathName;       // full path name of the cache directory.
    DWORD _FullPathNameLen;     // full path name string length.
    LPTSTR _FileName;           // full path name of the memory mapped file.
    DWORD _FileSize;            // current size of the memory mapped file.

    HANDLE _FileHandle;         // file handle of the memory mapped file.
    HANDLE _FileMappingHandle;  // mapping object handle
    LPVOID _BaseAddr;

    LPMEMMAP_HEADER _HeaderInfo;
    LPBYTE _EntryArray;

    DWORD _NumBitMapDWords;
    BOOL _NewFile;

    BOOL ValidateCache( VOID );
    DWORD GrowMapFile( DWORD );
    DWORD GetAndSetNextFreeEntry( DWORD );
    BOOL  CheckNextNBits(DWORD&, DWORD&, DWORD, DWORD&);
    BOOL  SetNextNBits(DWORD, DWORD, DWORD);
    DWORD RemapAddress( VOID );
    DWORD DeleteFiles( LPTSTR Files );
    BOOL InitHeaderInfo( VOID );

public:

    MEMMAP_FILE() {}
    ~MEMMAP_FILE( VOID );
    MemMapStatus Init(LPTSTR PathName, DWORD EntrySize);

    void                CloseMapping(void);
    BOOL                Reinitialize(void);
    BOOL                ReAllocateEntry(LPFILEMAP_ENTRY, DWORD);
    LPFILEMAP_ENTRY     AllocateEntry(DWORD);
    LPFILEMAP_ENTRY     FindNextEntry (DWORD* pdwEnum, DWORD dwFilter, GROUPID GroupId);
    BOOL                FreeEntry(LPFILEMAP_ENTRY Entry);
    DWORD               CheckSizeGrowAndRemapAddress(VOID);
    BOOL                IsBadOffset (DWORD dwOffset);
    URL_FILEMAP_ENTRY*  ValidateUrlOffset(DWORD dwOffset);

    BOOL                IsBadGroupOffset(DWORD dwOffset);
    GROUP_ENTRY*        ValidateGroupOffset(DWORD dwOffset, HASH_ITEM* hItem);
    LIST_GROUP_ENTRY*   ValidateListGroupOffset(DWORD dwOffset);
    
    //
    // WARNING: URL_CONTAINER::CleanupAllUrls assumes
    // that Handle is really an offset to a HASH_ITEM*.
    //

    LPBYTE *GetHeapStart(VOID)
    {
        return( (LPBYTE *)&_BaseAddr );
    }

    LPDWORD GetPtrToHashTableOffset (VOID)
    {
        return &_HeaderInfo->dwHashTableOffset;
    }

    LPDWORD GetPtrToLeakListOffset (VOID)
    {
        return _HeaderInfo->dwHeaderData
            + CACHE_HEADER_DATA_ROOT_LEAK_OFFSET;
    }

    //
    // --------- Inline directory related funcs ------------------
    //

    DWORD GetFileCount(DWORD idx)
    {
        return _HeaderInfo->DirArray[idx].nFileCount;
    }

    VOID SetFileCount(DWORD idx, DWORD nFiles)
    {
        if (idx < DEFAULT_MAX_DIRS)
            _HeaderInfo->DirArray[idx].nFileCount = nFiles;
        else
            INET_ASSERT(FALSE);
    }

    VOID IncrementFileCount(DWORD idx)
    {
        if (idx < DEFAULT_MAX_DIRS)
            _HeaderInfo->DirArray[idx].nFileCount++;
        else
            INET_ASSERT(FALSE);
    }

    VOID DecrementFileCount(DWORD idx)
    {
        if (idx < DEFAULT_MAX_DIRS)
            _HeaderInfo->DirArray[idx].nFileCount--;
        else
            INET_ASSERT(FALSE);
    }

    DWORD GetDirCount()
    {
        return _HeaderInfo->nDirCount;
    }

    VOID SetDirCount(DWORD nDirCount)
    {
        _HeaderInfo->nDirCount = nDirCount;
    }

    VOID IncrementDirCount()
    {
        _HeaderInfo->nDirCount++;
    }

    VOID DecrementDirCount()
    {
        _HeaderInfo->nDirCount--;
    }
    
    VOID SetDirName(DWORD idx, LPSTR sDirName)
    {
        INET_ASSERT(strlen(sDirName) == DIR_NAME_SIZE);

        if (idx < DEFAULT_MAX_DIRS)
            memcpy(_HeaderInfo->DirArray[idx].sDirName, 
                   sDirName, DIR_NAME_SIZE);
        else
            INET_ASSERT(FALSE);
    }

    VOID GetDirName(DWORD idx, LPSTR szDirName)
    {
        memcpy(szDirName, _HeaderInfo->DirArray[idx].sDirName, DIR_NAME_SIZE);
        szDirName[DIR_NAME_SIZE] = '\0';
    }
    
    DWORD GetDirIndex(LPSTR szDirName)
    {
        CHAR* ptr = szDirName + strlen(_FullPathName);

        for (DWORD idx = 0; idx < _HeaderInfo->nDirCount; idx++)
            if (!strnicmp(ptr, _HeaderInfo->DirArray[idx].sDirName, DIR_NAME_SIZE))
                return idx;
        return NOT_A_CACHE_SUBDIRECTORY;
    }


    //Creates a cache directory with a given name to allow existing directories
    //to be copied into another cache file.  Just the eight letters of the new
    //directory are given.  
    BOOL CreateDirWithSecureName( LPSTR szDirName)
    {
        BOOL retVal = FALSE;
        CHAR szFullDirName[MAX_PATH];
        DWORD nNewDir;

        //  MEMMAP_FILE::GetDirIndex requires the full directory name
        memcpy( szFullDirName, _FullPathName, _FullPathNameLen);
        lstrcpy( szFullDirName + _FullPathNameLen, szDirName);

        // only try to add the subdirectory if it doesn't already exist..
        if( GetDirIndex( szFullDirName) != NOT_A_CACHE_SUBDIRECTORY)
            goto exitCreateDirWithSecureName;
            
        // don't create more than DEFAULT_MAX_DIRS directories
        if( (nNewDir = GetDirCount()) >= DEFAULT_MAX_DIRS)
            goto exitCreateDirWithSecureName;

        IncrementDirCount();
        SetDirName( nNewDir, szDirName);
        SetFileCount( nNewDir, 0);

        retVal = TRUE;
        
    exitCreateDirWithSecureName:
        return retVal;
    }


    //
    // --------- General Get/Set type routines ------------------
    //
    
    DWORD GetStatus( VOID ) 
    {
        return( _Status );
    }

    DWORD IsNewFile( VOID ) 
    {
        return( _NewFile );
    }

    LPSTR GetFullPathName()
    {
        return _FullPathName;
    }

    DWORD GetFullPathNameLen()
    {
        return _FullPathNameLen;
    }

    LONGLONG GetCacheSize()
    {
        return _HeaderInfo->CacheSize + _FileSize;
    }

    LONGLONG GetCacheLimit()
    {
        return(_HeaderInfo->CacheLimit);
    }

    LONGLONG GetExemptUsage()
    {
        return _HeaderInfo->ExemptUsage;
    }

    VOID SetCacheLimit(LONGLONG CacheLimit) 
    {
        _HeaderInfo->CacheLimit = CacheLimit;
    }

    VOID AdjustCacheSize(LONGLONG DeltaSize)
    {
        if (_HeaderInfo->CacheSize >= -DeltaSize)
            _HeaderInfo->CacheSize += DeltaSize;
        else
        {
            // INET_ASSERT (FALSE); // underflow
            _HeaderInfo->CacheSize = 0;
        }
    }

    VOID SetCacheSize(LONGLONG Delta)
    {
        INET_ASSERT((Delta>=0));
        _HeaderInfo->CacheSize = (Delta>=0) ? Delta : 0;
    }
    
    VOID AdjustExemptUsage (LONGLONG DeltaSize)
    {
        if (_HeaderInfo->ExemptUsage >= -DeltaSize)
            _HeaderInfo->ExemptUsage += DeltaSize;
        else
        {
            INET_ASSERT (FALSE); // underflow
            _HeaderInfo->ExemptUsage = 0;
        }
    }

    BOOL SetHeaderData(DWORD nIdx, DWORD dwData)
    {
        INET_ASSERT(nIdx < NUM_HEADER_DATA_DWORDS);
        _HeaderInfo->dwHeaderData[nIdx] = dwData;
        return TRUE;
    }
        
    BOOL GetHeaderData(DWORD nIdx, LPDWORD pdwData) 
    {
        INET_ASSERT(nIdx < NUM_HEADER_DATA_DWORDS);
        *pdwData = _HeaderInfo->dwHeaderData[nIdx];
        return TRUE;
    }

    BOOL IncrementHeaderData(DWORD nIdx, LPDWORD pdwData) 
    {
        INET_ASSERT(nIdx < NUM_HEADER_DATA_DWORDS);
        *pdwData = _HeaderInfo->dwHeaderData[nIdx];
        ++*pdwData;
        _HeaderInfo->dwHeaderData[nIdx] = *pdwData;
        return TRUE;
    }
    
  VOID ResetEntryData(LPFILEMAP_ENTRY Entry, DWORD dwResetValue, DWORD nBlocks) 
    {
        for (DWORD i = 0; i < (_EntrySize * nBlocks) / sizeof(DWORD); i++)
        {
            *((DWORD*) Entry + i) = dwResetValue;
        }        
    }

   
};


