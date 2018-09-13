/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    filemap.cxx

Abstract:

    contains implementation of MEMMAP_FILE class.

Author:

    Madan Appiah (madana)  28-April-1995

Environment:

    User Mode - Win32

Revision History:

    Shishir Pardikar (shishirp) added: (as of (7/6/96)

    1) Fix crossproces problems on win95 in checksizegrowandremap
    2) Exception handling to deal with badsector being memorymapped
    3) More robust validation at init time
    4) Reinitialization code to really clear the cache
    5) Bug fixes in GrowMap while growing partially filled dword

--*/

#include <cache.hxx>


#define FILE_SIZE_MAX_DIGITS 16


DWORD
ValidateAndCreatePath(
    LPTSTR PathName
    )
{
    DWORD Error, len;
    DWORD FileAttribute;
    LPTSTR PathDelimit;

    //
    // check to see the path specified is there.
    //

    FileAttribute = GetFileAttributes( PathName );

    if( FileAttribute != 0xFFFFFFFF ) {

        //
        // check to see the attribute says it is a dir.
        //

        if( !(FileAttribute & FILE_ATTRIBUTE_DIRECTORY) ) {
            
            return( ERROR_INVALID_PARAMETER );
        }

        // We found the file and it is a dir.
        // Set the system attribute just in case
        // it has been unset.
        SetFileAttributes(PathName, FILE_ATTRIBUTE_SYSTEM);
        return( ERROR_SUCCESS );
    }

    Error = GetLastError();

    if( (Error != ERROR_FILE_NOT_FOUND) &&
        (Error != ERROR_PATH_NOT_FOUND) ) {

        return( Error );
    }

    //
    // we did not find the path, so create it.
    //

    if( CreateDirectory( PathName, NULL ) ) {

        //
        // done.
        //
        SetFileAttributes(PathName, FILE_ATTRIBUTE_SYSTEM);
        return( ERROR_SUCCESS );
    }

    Error = GetLastError();

    if( Error != ERROR_PATH_NOT_FOUND ) {

        return( Error );
    }

    //
    // sub-path is not found, create it first.
    //

    len = lstrlen( PathName );

    if (len < 5) {

        SetLastError(ERROR_INVALID_NAME);

        return (ERROR_INVALID_NAME);
    }

    PathDelimit = PathName + len -1 ;

    // step back from the trailing backslash

    if( *PathDelimit == PATH_CONNECT_CHAR ) {
        PathDelimit--;
    }

    //
    // find the last path delimiter.
    //

    while( PathDelimit >  PathName ) {
        if( *PathDelimit == PATH_CONNECT_CHAR ) {
            break;
        }

        PathDelimit--;
    }

    if( PathDelimit == PathName ) {
        return( ERROR_INVALID_PARAMETER );
    }

    *PathDelimit = TEXT('\0');

    //
    // validate sub-path now.
    //

    Error = ValidateAndCreatePath( PathName ) ;

    //
    // replace the connect char anyway.
    //

    *PathDelimit = PATH_CONNECT_CHAR;

    if( Error != ERROR_SUCCESS ) {

        return( Error );
    }

    //
    // try to create one more time.
    //

    if( CreateDirectory( PathName, NULL ) ) {

        //
        // done.
        //

        return( ERROR_SUCCESS );
    }

    Error = GetLastError();
    return( Error );
}


DWORD
MEMMAP_FILE::CheckSizeGrowAndRemapAddress(
    VOID
    )
{
    DWORD dwNewFileSize;

#ifdef WIN95_BUG
    if( _FileSize == (dwNewFileSize = _HeaderInfo->FileSize )) {
        return( ERROR_SUCCESS );
    }
#endif //WIN95_BUG

    // ideally we would have liked to do as in the above two lines
    // this works right on NT but doesn't on win95.

    // This is because the filesize is a part of the mapname
    // In the initial state the index file size is 8192. So the
    // memorymap name is c:_windows_temporaray internet files_8192.
    // Both the processes have this map in their address space.
    // Process B starts pumping in the data, and at some point the index
    // file needs to be grown. Process B, increases the index file to 16384,
    // updates the filesize in the header "while it is still mapped in the
    // map corresponding to the old filesize" and then remaps to the new map
    // with the name c:_windows_temporaray internet files_16384.
    // Any subsequent growth is now recorded in this map.
    // The old map c:_windows_temporaray internet files_8192 still has only
    // the first transition.


    // the work around is to actually get the filesize from the filesystem
    // This works correctly on win95 and NT both. Optimally, we would
    // check for a transition and then get the real size, but we will do that
    // after IE30 ships.



    //NB!!!!!!! The check below is the basis of our cross process
    // cache. All APIs finally make this call before touching the memory
    // mapped file. If there is a chneg, they remap it to the new size
    // with the sizename as part of the mapping, so they get the latest
    // stuff.
    // When anyone gets here, they are protected by a crossprocess mutex


    if( _FileSize == (dwNewFileSize = GetFileSize(_FileHandle, NULL))) {
        return( ERROR_SUCCESS );
    }

    //
    // so other user of the memmap file has increased the file size,
    // let's remap our address space so that the new portion is
    // visible to us too.
    //

    DWORD Error;
    DWORD OldFileSize;
    DWORD OldNumBitMapDWords;

   //
   // set our internal file size and num bit map entries.
   //

    OldFileSize = _FileSize;
    OldNumBitMapDWords = _NumBitMapDWords;


    _FileSize = dwNewFileSize;

    Error = RemapAddress();

    if( Error != ERROR_SUCCESS ) {

        //
        // reset the file size.
        //

        _FileSize = OldFileSize;
        _NumBitMapDWords = OldNumBitMapDWords;
    }
    else {

#if INET_DEBUG
        if ((GetFileSize(_FileHandle, NULL)) != (_HeaderInfo->FileSize)) {

            TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE,
                "GetFileSize!= (_FileSize)\n" ));

            TcpsvcsDbgAssert(FALSE);

        }
#endif //INET_DEBG

        _NumBitMapDWords =
            (_HeaderInfo->NumUrlInternalEntries + (NUM_BITS_IN_DWORD - 1)) /
                NUM_BITS_IN_DWORD; // cell
    }

    return( Error );
}

BOOL
MEMMAP_FILE::ValidateCache(
    VOID
    )
/*++

    This private member function validates the cache file content.

Arguments:

    NONE.

Return Value:

    TRUE - if the cache is valid.
     FALSE - otherwise.

--*/
{
    BOOL ReturnCode = FALSE;
    int i, k;
    DWORD BitPosition, TotalAlloced, MaxAllocedPosition, RunningCounter;


    __try {

        // validate signatue.
        if( memcmp(
                _HeaderInfo->FileSignature,
                CACHE_SIGNATURE,
                MAX_SIG_SIZE * sizeof(TCHAR) ) != 0 ) {

            TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE,
                "File signature does not match.\n" ));
            goto Cleanup;
        }

        // Also check the index does not contain entries with a higher
        // version than the current machine can handle.  This can happen
        // due to Windows kludgy concept of roaming, which replicates 
        // parts of the file system and registry hkcu.
        
        LPSTR pszHighVer = (LPSTR) (_HeaderInfo->dwHeaderData
            + CACHE_HEADER_DATA_HIGH_VERSION_STRING);
        if (pszHighVer[0] != 'V' || pszHighVer[3] != 0)
            memset (pszHighVer, 0, sizeof(DWORD));
        else if (!g_szFixup[0] || strcmp (g_szFixup, pszHighVer) < 0)
        {
            TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE,
                "Cannot handle uplevel index file.\n" ));
            goto Cleanup;
        }

        // check the hash table root offset is valid
        if( _HeaderInfo->dwHashTableOffset != 0 ) {

            if( _HeaderInfo->dwHashTableOffset > _FileSize ) {
                TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE,
                    "invalid b-tree root offset.\n" ));
                goto Cleanup;
            }
        }

        // check file size.
        if( _HeaderInfo->FileSize != _FileSize ) {
            TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE,
                "invalid file size.\n" ));
            goto Cleanup;
        }

        // one more file size check.
        DWORD ExpectedFileSize;
        ExpectedFileSize =
            HEADER_ENTRY_SIZE +
                _HeaderInfo->NumUrlInternalEntries * _EntrySize;

        // cell the size to GlobalMapFileGrowSize.
        if( ExpectedFileSize % GlobalMapFileGrowSize ) {
                ExpectedFileSize =
                ((ExpectedFileSize /  GlobalMapFileGrowSize) + 1) *
                        GlobalMapFileGrowSize;
        }

        if( _FileSize != ExpectedFileSize ) {

            // it is ok if the file size is one block bigger.
            TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE,
                "Invalid file size.\n" ));
            goto Cleanup;
        }


        if(_HeaderInfo->NumUrlInternalEntries < _HeaderInfo->NumUrlEntriesAlloced) {
            TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE,
                "Invalid alloc entires.\n" ));
            goto Cleanup;
        }


        TotalAlloced = 0;
        MaxAllocedPosition = 0;
        RunningCounter = 0;

        // scan the enire bitmap and do some consistency check for allocated bits

        for(i=0; i<BIT_MAP_ARRAY_SIZE; ++i) {
            // k goes from 0 to 31
            // BitPosition goes from 0x00000001 to 0x80000000

            for(BitPosition=1, k=0; k<NUM_BITS_IN_DWORD; ++k, BitPosition <<=1) {

                ++RunningCounter;
                if(_HeaderInfo->AllocationBitMap[i] & BitPosition) {

                    ++TotalAlloced;

                    MaxAllocedPosition = RunningCounter;

                }

            }
        }

        // if the max allocated bit is greter than the number of
        // possible entries for this filesize,
        // or the total allocated bits are greater (the above condition subsumes
        // this one, but it is OK to be paranoid)
        // or totalbits alloced don't match the count
        // there this header is not OK

        if ((MaxAllocedPosition > _HeaderInfo->NumUrlInternalEntries)
            ||(TotalAlloced > _HeaderInfo->NumUrlInternalEntries)
            ||(TotalAlloced != _HeaderInfo->NumUrlEntriesAlloced)) {

            TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE,
                "Invalid alloc bitmap\n" ));
            goto Cleanup;

        }
        //
        // every thing is fine.
        //

        ReturnCode = TRUE;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {

        ReturnCode = FALSE;

        _Status = ERROR_WRITE_FAULT;
    }
    ENDEXCEPT

Cleanup:

    if( ReturnCode == FALSE ) {
        TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE, "Invalid Cache, or bad disk\n" ));
    }

    return( ReturnCode );
}

void MEMMAP_FILE::CloseMapping (void)
{
    if (_BaseAddr) // view
    {
        UnmapViewOfFile(_BaseAddr);
        _BaseAddr = NULL;
    }
    if (_FileMappingHandle) // mapping
    {
        CloseHandle (_FileMappingHandle);
        _FileMappingHandle = NULL;
    }
    if (_FileHandle) // file
    {
        CloseHandle (_FileHandle);
        _FileHandle = NULL;
    }
}


DWORD
MEMMAP_FILE::RemapAddress(
    VOID
    )
/*++

    This private member function remaps the memory mapped file just after
    the file size has been modified.

    Container must be locked when this function is called.

Arguments:

    NONE.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error = ERROR_SUCCESS;
    PVOID OldBaseAddr;
    DWORD OldViewSize;
    PVOID VirtualBase;
    BOOL BoolError;
    LPTSTR MapName = NULL;

    CloseMapping();
    
    //
    // Create/Open memory mapped file.
    //

    _FileHandle =
        CreateFile(
            _FileName,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
                // share this file with others while it is being used.
            CreateAllAccessSecurityAttributes(NULL, NULL, NULL),
            OPEN_ALWAYS,
            FILE_FLAG_RANDOM_ACCESS,
            NULL );


    if( _FileHandle ==  INVALID_HANDLE_VALUE ) {

        Error = _Status = GetLastError();
        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "Reinitialize:File open failed, %ld.\n",_Status));

        TcpsvcsDbgAssert( FALSE );

        _FileHandle = NULL;
        goto Cleanup;
    }


#ifndef unix
    /*******
     * UNIX:
     *       Mainwin does not support MapName in CreateFileMapping API
     *       Let us leave the MapName as NULL till this functionality
     *       is available.
     */

    //
    // make a map name.
    //

    DWORD MapNameSize;

    MapNameSize =
        (lstrlen(_FullPathName) +
            lstrlen( _FileName) +
                1 +
                FILE_SIZE_MAX_DIGITS ) * sizeof(TCHAR) ;

    MapName = (LPTSTR) CacheHeap->Alloc( MapNameSize );

    if( MapName == NULL ) {
        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    memcpy(MapName, _FileName, _FullPathNameLen + sizeof(MEMMAP_FILE_NAME));
    memcpy(MapName + _FullPathNameLen + sizeof(MEMMAP_FILE_NAME) - 1, DIR_SEPARATOR_STRING, sizeof(DIR_SEPARATOR_STRING));
    wsprintf(MapName + lstrlen(MapName), "%u", _FileSize);

#ifndef unix
#define BACKSLASH_CHAR          TEXT('\\')
#else
#define BACKSLASH_CHAR          TEXT('/')
#endif /* unix */
#define UNDERSCORE_CHAR         TEXT('_')
#define TERMINATING_CHAR        TEXT('\0')

    LPTSTR ScanMapName;

    //
    // Replace '\' with '_'.
    //

    ScanMapName = MapName;

    while( *ScanMapName != TERMINATING_CHAR ) {

        if( *ScanMapName == BACKSLASH_CHAR ) {
            *ScanMapName = UNDERSCORE_CHAR;
        }

        ScanMapName++;
    }
#endif /* !unix */



    //
    // re-create memory mapping.
    //
    _FileMappingHandle = OpenFileMapping(FILE_MAP_WRITE, FALSE, MapName);

    if (_FileMappingHandle == NULL && (GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_INVALID_NAME))
    {
        _FileMappingHandle =
            CreateFileMapping(
                _FileHandle,
                CreateAllAccessSecurityAttributes(NULL, NULL, NULL),
                PAGE_READWRITE,
                0, // high dword of max memory mapped file size.
    #if defined(UNIX) && defined(ux10)
                1024 * 1024, // map entire file.
    #else
                0, // map entire file.
    #endif
                MapName);
    }

    if( _FileMappingHandle == NULL ) {
        Error = _Status = GetLastError();
        goto Cleanup;
    }

    //
    // remap view region.
    //

    _BaseAddr =
        MapViewOfFileEx(
            _FileMappingHandle,
            FILE_MAP_WRITE,
            0,
            0,
#if defined(UNIX) && defined(ux10)
            1024 * 1024,   // MAP entire file.
#else
            0,   // MAP entire file.
#endif
            NULL );

#if defined(UNIX) && defined(ux10)
    DWORD FilePointer = SetFilePointer(
                            _FileHandle,
                            _FileSize,
                            NULL,
                            FILE_BEGIN );
    if (FilePointer == 0xFFFFFFFF)
    {
        Error = _Status = GetLastError();
        goto Cleanup;
    }

    BoolError = SetEndOfFile( _FileHandle );

    if (BoolError == FALSE)
    {
        Error = _Status = GetLastError();
        goto Cleanup;
    }
#endif

    if( _BaseAddr == NULL ) 
    {
        Error = _Status = GetLastError();
        TcpsvcsDbgAssert( FALSE );

        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "MapViewOfFile failed to extend address space, %ld.\n",
                Error ));

       goto Cleanup;
    }

    //
    // reset other pointers.
    //

    _HeaderInfo = (LPMEMMAP_HEADER)_BaseAddr;
    _EntryArray = ((LPBYTE)_BaseAddr + HEADER_ENTRY_SIZE );

    _Status = Error = ERROR_SUCCESS;

Cleanup:


    if( MapName != NULL ) {
        CacheHeap->Free( MapName );
    }

        return( Error );
}

DWORD
MEMMAP_FILE::GrowMapFile(DWORD dwMapFileGrowSize)
/*++

    This private member function extends the memory mapped file and
    creates more free url store entries.

Arguments:

    NONE.

Return Value:

    Windows Error Code.

--*/
{
    DWORD Error, i;
    BOOL BoolError;
    DWORD FilePointer;
    DWORD OldNumUrlInternalEntries;
    char  buff[PAGE_SIZE];

    //
    // check to see that we have reached the limit.
    // we can hold only MAX_URL_ENTRIES url entries.
    // so the file size can grow more than
    //
    //  HEADER_ENTRY_SIZE + MAX_URL_ENTRIES * _EntrySize
    //

#if INET_DEBUG
    if (GetFileSize(_FileHandle, NULL) != (_FileSize)) {

        TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE,
                "GetFileSize!= (_FileSize)\n" ));

        TcpsvcsDbgAssert(FALSE);

    }
#endif //INET_DEBG

    //BUGBUG - need to fix this
    if( (_FileSize + dwMapFileGrowSize) >=
            (HEADER_ENTRY_SIZE +
                MAX_URL_ENTRIES * _EntrySize) ) {

        //
        // best matching error code.
        //

        Error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    FilePointer = SetFilePointer(
                            _FileHandle,
                            dwMapFileGrowSize,
                            NULL,
                            FILE_END );


    if (FilePointer != (_FileSize + dwMapFileGrowSize))
    {
        TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE,
                "FilePointer != (_FileSize + dwMapFileGrowSize)\n" ));

        TcpsvcsDbgAssert(FALSE);
        
        _Status = GetLastError();
        Error = _Status;

        goto Cleanup;
    }

    if( FilePointer == 0xFFFFFFFF ) {
        Error = GetLastError();
        TcpsvcsDbgAssert(FALSE);
        goto Cleanup;
    }

    BoolError = SetEndOfFile( _FileHandle );

    if( BoolError != TRUE ) {
        Error = GetLastError();
        TcpsvcsDbgAssert(FALSE);
        goto Cleanup;
    }

#if INET_DEBUG
    if (GetFileSize(_FileHandle, NULL) != (_FileSize + dwMapFileGrowSize)) {

        TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE,
                "GetFileSize!= (_FileSize + dwMapFileGrowSize)\n" ));

        TcpsvcsDbgAssert(FALSE);

    }
#endif

    //
    // adjust internal size parameters.
    //

    _FileSize += dwMapFileGrowSize;

    //
    // also set the new file size in the memory mapped file so that
    // other user will remap their address space and view the new portion.
    //

    _HeaderInfo->FileSize = _FileSize;

    OldNumUrlInternalEntries = _HeaderInfo->NumUrlInternalEntries;
    _HeaderInfo->NumUrlInternalEntries +=
        dwMapFileGrowSize / _EntrySize;

    _NumBitMapDWords =
        (_HeaderInfo->NumUrlInternalEntries + (NUM_BITS_IN_DWORD - 1)) /
            NUM_BITS_IN_DWORD; // cell

    //
    // remap
    //

    Error = RemapAddress();

    if( Error != ERROR_SUCCESS ) {

        goto Cleanup;
    }

    memset(
          (_EntryArray + _EntrySize * OldNumUrlInternalEntries),
           0,
          dwMapFileGrowSize );

    Error = ERROR_SUCCESS;


Cleanup:

    return( Error );
}

BOOL MEMMAP_FILE::CheckNextNBits(DWORD& nArrayIndex, DWORD &dwStartMask, 
                                DWORD nBitsRequired, DWORD& nBitsFound)
{
/*++
    Determines if the next N bits are unset.

Arguments:
    [IN/OUT]
    DWORD &nArrayIndex, DWORD &dwMask

    [IN]
    DWORD nBitsRequired

    [OUT]
    DWORD &nBitsFound

Return Value:

    TRUE if the next N bits were found unset.
    FALSE otherwise.

Notes:
    This function assumes that the range of bits to be checked lie
    within a valid area of the bit map. 
--*/
    DWORD i, j;
    DWORD nIdx = nArrayIndex;
    DWORD dwMask = dwStartMask;
    BOOL fFound = FALSE;
    LPDWORD BitMap = &_HeaderInfo->AllocationBitMap[nIdx];

    nBitsFound = 0;

    // Check if the next nBitsRequired bits are unset
    for (i = 0; i < nBitsRequired; i++)
    {
        // Is this bit unset?
        if ((*BitMap & dwMask) == 0)
        {
            // Have sufficient unset bits been found?
            if (++nBitsFound == nBitsRequired)
            {
                // Found sufficient bits. Success.
                fFound = TRUE;
                goto exit;
            }
        }

        // Ran into a set bit. Fail.
        else
        {
            // Indicate the array and bit index 
            // of the set bit encountered.
            nArrayIndex = nIdx;
            dwStartMask = dwMask;
            goto exit;
        }

        // Left rotate the bit mask.
        dwMask <<= 1;
        if (dwMask == 0x0)
        {
            dwMask = 0x1;
            BitMap = &_HeaderInfo->AllocationBitMap[++nIdx];
        }

    } // Loop nBitsRequired times.


exit:
    return fFound;
}
 

BOOL MEMMAP_FILE::SetNextNBits(DWORD nIdx, DWORD dwMask, 
                                DWORD nBitsRequired)
/*++
    Given an array index and bit mask, sets the next N bits.

Arguments:
    [IN]
    DWORD nIdx, DWORD dwMask, DWORD nBitsRequired

Return Value:

    TRUE if the next N bits were found unset, and successfully set.
    FALSE if unable to set all the required bits.

Notes:
    This function assumes that the range of bits to be set lie
    within a valid area of the bit map. If the function returns
    false, no bits are set.
 --*/
{
    DWORD i, j, nBitsSet = 0;
    LPDWORD BitMap = &_HeaderInfo->AllocationBitMap[nIdx];
    BitMap = &_HeaderInfo->AllocationBitMap[nIdx];

    for (i = 0; i < nBitsRequired; i++)
    {    
        // Check that this bit is not already set.
        if (*BitMap & dwMask)
        {
            INET_ASSERT(FALSE);

            // Fail. Unset the bits we just set and exit.
            for (j = nBitsSet; j > 0; j--)
            {
                INET_ASSERT((*BitMap & dwMask) == 0);

                // Right rotate the bit mask.
                dwMask >>= 1;
                if (dwMask == 0x0)
                {
                    dwMask = 0x80000000;
                    BitMap = &_HeaderInfo->AllocationBitMap[--nIdx];
                }                        
                *BitMap &= ~dwMask;
            }             
            return FALSE;
        }

        *BitMap |= dwMask;
        nBitsSet++;
    
        // Left rotate the bit mask.
        dwMask <<= 1;
        if (dwMask == 0x0)
        {
            dwMask = 0x1;
            BitMap = &_HeaderInfo->AllocationBitMap[++nIdx];
        }                        
    
    }

    // Success.
    return TRUE;
}


DWORD
MEMMAP_FILE::GetAndSetNextFreeEntry(
    DWORD nBitsRequired
    )
/*++
    This private member function computes the first available free entry
    index.

Arguments:

    DWORD nBitsRequired

Return Value:

    Next available free entry Index.
--*/
{
    DWORD i, nReturnBit = 0xFFFFFFFF;
    
    // Align if 4k or greater
    BOOL fAlign = (nBitsRequired >= NUM_BITS_IN_DWORD ? TRUE : FALSE);            
    
    // Scan DWORDS from the beginning of the byte array.
    DWORD nArrayIndex = 0;
    while (nArrayIndex < _NumBitMapDWords)
    {
        // Process starting from this DWORD if alignment is not required 
        // and there are free bits, or alignment is required and all bits
        // are free. 
        if (_HeaderInfo->AllocationBitMap[nArrayIndex] !=  0xFFFFFFFF
            && (!fAlign || (fAlign && _HeaderInfo->AllocationBitMap[nArrayIndex] == 0)))
        {
            DWORD nBitIndex = 0;
            DWORD dwMask = 0x1;
            LPDWORD BitMap = &_HeaderInfo->AllocationBitMap[nArrayIndex];

            // Find a candidate slot.
            while (nBitIndex < NUM_BITS_IN_DWORD)
            {
                // Found first bit of a candidate slot.
                if ((*BitMap & dwMask) == 0)
                {
                    // Calculate leading bit value.
                    DWORD nLeadingBit = NUM_BITS_IN_DWORD * nArrayIndex + nBitIndex;
          
                    // Don't exceed the number of internal entries.
                    if (nLeadingBit + nBitsRequired > _HeaderInfo->NumUrlInternalEntries)
                    {
                        // Overstepped last internal entry
                        goto exit;
                    }

                    // If we just need one bit, then we're done.
                    if (nBitsRequired == 1)
                    {
                        *BitMap |= dwMask;
                        nReturnBit = nLeadingBit;
                        _HeaderInfo->NumUrlEntriesAlloced += 1;
                        goto exit;
                    }

                    // Additional bits required.
                    DWORD nBitsFound;
                    DWORD nIdx = nArrayIndex;

                    // Check the next nBitsRequired bits. Set them if free.
                    if (CheckNextNBits(nIdx, dwMask, nBitsRequired, nBitsFound))
                    {
                        if (SetNextNBits(nIdx, dwMask, nBitsRequired))
                        {
                            // Return the offset of the leading bit.
                            _HeaderInfo->NumUrlEntriesAlloced += nBitsRequired;
                            nReturnBit = nLeadingBit;
                            goto exit;
                        }
                        // Bad news.
                        else
                        {
                            // The bits are free, but we couldn't set them. Fail.
                            goto exit;
                        }
                    }
                    else
                    {
                        // This slot has insufficient contiguous free bits. 
                        // Update the array index. We break back to looping
                        // over the bits in the DWORD where the interrupting
                        // bit was found.
                        nArrayIndex = nIdx;
                        nBitIndex = (nBitIndex + nBitsFound) % NUM_BITS_IN_DWORD;
                        break;
                    }

                } // Found a free leading bit.
                else                
                {
                    // Continue looking at bits in this DWORD.
                    nBitIndex++;
                    dwMask <<= 1;
                }

            } // Loop over bits in DWORD.

        } // If we found a candidate DWORD.

        nArrayIndex++;

    } // Loop through all DWORDS.
	exit:
    return nReturnBit;
}


MemMapStatus MEMMAP_FILE::Init(LPTSTR PathName, DWORD EntrySize)
/*++

    MEMMAP_FILE object constructor.

Arguments:

    PathName : full path name of the memory mapped file.

    EntrySize : size of the each entry in this container.

Return Value:

    NONE.

--*/
{
    DWORD cb;

    _EntrySize =  EntrySize;
    _FullPathName = NULL;
    _FileName = NULL;
    _FileSize = 0;
    _FileHandle = NULL;
    _FileMappingHandle = NULL;
    _BaseAddr = NULL;
    _HeaderInfo = NULL;
    _EntryArray = NULL;
    _NumBitMapDWords = 0;

    // Validate the path and create the path if it is not already there.
    _Status = ValidateAndCreatePath( PathName );
    if( _Status != ERROR_SUCCESS ) 
        goto Cleanup;

    // Path to memory mapped file.
    cb = strlen(PathName);
    _FullPathName = (LPTSTR)CacheHeap->Alloc(cb + sizeof(DIR_SEPARATOR_STRING));

    if( _FullPathName == NULL ) 
    {
        _Status = ERROR_NOT_ENOUGH_MEMORY;
        INET_ASSERT(FALSE);
        goto Cleanup;
    }

    memcpy(_FullPathName, PathName, cb + 1);
    AppendSlashIfNecessary(_FullPathName, cb);
    
    _FullPathNameLen = cb;

    // Construct memory mapped file name.
    _FileName = (LPTSTR)CacheHeap->Alloc(cb + sizeof(MEMMAP_FILE_NAME));
    if (!_FileName)
    {
        _Status = ERROR_NOT_ENOUGH_MEMORY;
        INET_ASSERT(FALSE);
        goto Cleanup;
    }
    memcpy(_FileName, _FullPathName, cb);
    memcpy(_FileName + cb, 
        MEMMAP_FILE_NAME, sizeof(MEMMAP_FILE_NAME));
    
    // Create/Open memory mapped file.
    _FileHandle =
        CreateFile(
            _FileName,
            GENERIC_READ | GENERIC_WRITE,
            FILE_SHARE_READ | FILE_SHARE_WRITE,
            CreateAllAccessSecurityAttributes(NULL, NULL, NULL),
            OPEN_ALWAYS,
            FILE_FLAG_RANDOM_ACCESS,
            NULL );

    _Status = GetLastError();

    if( _FileHandle ==  INVALID_HANDLE_VALUE ) 
    {
        _FileHandle = NULL;
        TcpsvcsDbgAssert(FALSE);
        goto Cleanup;
    }
    else
    {
        SetFileTime(_FileHandle, NULL, NULL, (LPFILETIME)&dwdwSessionStartTime);
    }

    // Check to this file is new.
    if ( _Status == ERROR_ALREADY_EXISTS ) 
    {

        // Old file.

        _Status = ERROR_SUCCESS;
       _NewFile = FALSE;

       _FileSize = GetFileSize( _FileHandle, NULL );

       if( _FileSize == 0xFFFFFFFF ) 
       {
           _Status = GetLastError();
           TcpsvcsDbgAssert(FALSE);
           goto Cleanup;
       }

       if ((_FileSize < GlobalMapFileGrowSize) || ((_FileSize %GlobalMapFileGrowSize) != 0)) 
       {
            TcpsvcsDbgAssert(FALSE);
            if(!Reinitialize()) 
            {
                TcpsvcsDbgAssert(FALSE);
                SetLastError(_Status);
                goto Cleanup;
            }
            // Reinitialization results in new file.
            _NewFile = TRUE;
       }
    }
    else if( _Status == ERROR_SUCCESS) 
    {
        BOOL BoolError;
        DWORD FilePointer;

        // New file.
        _NewFile = TRUE;

        // Set initial file size.
        _FileSize = GlobalMapFileGrowSize;
        FilePointer = SetFilePointer( _FileHandle, _FileSize, NULL, FILE_BEGIN );

        if( FilePointer == 0xFFFFFFFF ) 
        {
            _Status = GetLastError();
            goto Cleanup;
        }

        if (FilePointer != _FileSize )
        {
            TcpsvcsDbgPrint(( DEBUG_FILE_VALIDATE,
                    "FilePointer != (_FileSize)\n" ));

            TcpsvcsDbgAssert(FALSE);
        }

        BoolError = SetEndOfFile( _FileHandle );

        if( BoolError != TRUE ) 
        {
            _Status = GetLastError();
            goto Cleanup;
        }
    }
    else 
    {
        // We should not reach here.
        TcpsvcsDbgAssert(FALSE);
    }
    _Status = RemapAddress();

    if( _Status != ERROR_SUCCESS ) 
    {
        TcpsvcsDbgAssert(FALSE);
        goto Cleanup;
    }

    TcpsvcsDbgPrint(( DEBUG_ERRORS,
                        "Header Size, %ld.\n",
                    HEADER_ENTRY_SIZE));

    TcpsvcsDbgPrint(( DEBUG_ERRORS,
                        "Size of elements, %ld.\n",
                        sizeof(MEMMAP_HEADER_SMALL)));


    TcpsvcsDbgPrint(( DEBUG_ERRORS,
                        "Bit Array size, %ld.\n",
                    BIT_MAP_ARRAY_SIZE));

    TcpsvcsDbgPrint(( DEBUG_ERRORS,
                        "Memmap Header Size, %ld.\n",
                    sizeof(MEMMAP_HEADER)));

    TcpsvcsDbgAssert( HEADER_ENTRY_SIZE >= sizeof(MEMMAP_HEADER) );

    // validate the file content if the file is not new.
    if( _NewFile != TRUE ) 
    {
        if( ValidateCache() == FALSE) 
        {
            if (!Reinitialize()) 
            {
                _Status = ERROR_INTERNET_INTERNAL_ERROR;
                goto Cleanup;
            }

            // Succeeded in re-initializing the file, we 
            // treat this as if we created a new file.
            _NewFile = TRUE;
        }
    }
    else
    {
        // It is a brand new file. Initialize file header.
        if(!InitHeaderInfo()) 
        {
            // This can happen if there is an exception while
            // initializing headers
            _Status = ERROR_INTERNET_INTERNAL_ERROR;
            goto Cleanup;

        }
    }

    // Compute number of bitmap DWORDs used.
    _NumBitMapDWords =
        (_HeaderInfo->NumUrlInternalEntries + (NUM_BITS_IN_DWORD - 1)) /
            NUM_BITS_IN_DWORD; //cell

    // We are done.
    _Status = ERROR_SUCCESS;

Cleanup:

    if( _Status != ERROR_SUCCESS ) 
    {
        INET_ASSERT(FALSE);
        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "MEMMAP_FILE::Initfailed, %ld\n", _Status ));
        
        SetLastError(_Status);
    }

    if (_NewFile)
        return MEMMAP_STATUS_REINITIALIZED;
    else
        return MEMMAP_STATUS_OPENED_EXISTING;
}

MEMMAP_FILE::~MEMMAP_FILE(
    VOID
    )
/*++

Routine Description:

    MEMMAP_FILE object destructor.

Arguments:

    None.

Return Value:

    None.

--*/
{
    CloseMapping();
    CacheHeap->Free( _FileName );
    CacheHeap->Free( _FullPathName );
}


BOOL MEMMAP_FILE::ReAllocateEntry(LPFILEMAP_ENTRY pEntry, DWORD cbBytes)
/*++

Routine Description:

    Attempts to reallocate an entry at the location given.

Arguments:

    LPFILEMAP_ENTRY pEntry: Pointer to location in file map.
    DWORD cbBytes : Number of bytes requested

Return Value:

    Original value of pEntry if successful. pEntry->nBlocks is set to the new
    value, but all other fields in the entry are unmodified. If insufficient contiguous 
    bits are found at the end of the original entry, NULL is returned, indicating failure.
    In this case the entry remains unmodified. 

Notes:
    
    The Map file should *not* be grown if insufficient additional bits are not found.

--*/
{
    // Validate cbBytes
    if (cbBytes > MAX_ENTRY_SIZE)
    {
        INET_ASSERT(FALSE);
        return FALSE;
    }

    // Validate pEntry.
    DWORD cbEntryOffset = (DWORD) PtrDifference(pEntry, _EntryArray);
    if (IsBadOffset(cbEntryOffset))
    {
        INET_ASSERT(FALSE);
        return FALSE;
    }

    // Calculate number of blocks required for this entry.
    DWORD nBlocksRequired = NUMBLOCKS(ROUNDUPBLOCKS(cbBytes));
    
    // Sufficient space in current slot?
    if (nBlocksRequired <= pEntry->nBlocks)
    {
        // We're done.
        return TRUE;
    }
    else
    {           
        // Determine if additional free bits are 
        // available at the end of this entry.
        // If not, return NULL.

        // Determine the array and bit indicese of the first
        // free bit immediately following the last set bit of
        // the entry.
        DWORD nTrailingIndex = cbEntryOffset / _EntrySize + pEntry->nBlocks;
        DWORD nArrayIndex = nTrailingIndex / NUM_BITS_IN_DWORD;
        DWORD nBitIndex = nTrailingIndex % NUM_BITS_IN_DWORD;
        DWORD dwMask = 0x1 << nBitIndex;
        DWORD nAdditionalBlocksRequired = nBlocksRequired - pEntry->nBlocks;
        DWORD nBlocksFound;

        // Don't exceed the number of internal entries.
        if (nTrailingIndex + nAdditionalBlocksRequired 
            > _HeaderInfo->NumUrlInternalEntries)
        {
            // Overstepped last internal entry. Here we should fail
            // by returning NULL. Note - DO NOT attempt to grow the 
            // map file at this point. The caller does not expect this.
            return FALSE;
        }

        if (CheckNextNBits(nArrayIndex, dwMask, 
            nAdditionalBlocksRequired, nBlocksFound))
        {
            // We were able to grow the entry.
            SetNextNBits(nArrayIndex, dwMask, nAdditionalBlocksRequired);
            pEntry->nBlocks = nBlocksRequired;
            _HeaderInfo->NumUrlEntriesAlloced += nAdditionalBlocksRequired;
            return TRUE;
        }
        else
            // Couldn't grow the entry.
            return FALSE;
    }
}

LPFILEMAP_ENTRY MEMMAP_FILE::AllocateEntry(DWORD cbBytes)
/*++

Routine Description:

    Member function that returns an free entry from the cache list. If
    none is available free, it grows the map file, makes more free
    entries.

Arguments:

    DWORD cbBytes : Number of bytes requested
    DWORD cbOffset: Offset from beginning of bit map where allocation is requested.

Return Value:

    If NULL, GetStatus() will return actual error code.

--*/
{
    LPFILEMAP_ENTRY NewEntry;

    // Validate cbBytes
    if (cbBytes > MAX_ENTRY_SIZE)
    {
        INET_ASSERT(FALSE);
        return 0;
    }

    // Find and mark off a set of contiguous bits
    // spanning the requested number of bytes.
    DWORD nBlocksRequired = NUMBLOCKS(ROUNDUPBLOCKS(cbBytes));
    DWORD FreeEntryIndex = GetAndSetNextFreeEntry(nBlocksRequired);

    // Failed to find space.
    if( FreeEntryIndex == 0xFFFFFFFF ) 
    {
        // Map file is full, grow it now.
        _Status = GrowMapFile(cbBytes <= GlobalMapFileGrowSize ?
            GlobalMapFileGrowSize : ROUNDUPTOPOWEROF2(cbBytes, ALLOC_PAGES * PAGE_SIZE) );

        // Failed to grow map file.
        if( _Status != ERROR_SUCCESS ) 
        {
            return NULL;
        }

        // Retry with enlarged map file.
        FreeEntryIndex = GetAndSetNextFreeEntry(nBlocksRequired);

        TcpsvcsDbgAssert( FreeEntryIndex != 0xFFFFFFFF );

        // Failed to allocate bytes after enlarging map file.
        if( FreeEntryIndex == 0xFFFFFFFF ) 
        {
            return NULL;
        }
    }

    INET_ASSERT(  (cbBytes < PAGE_SIZE) 
        || ( (cbBytes >= PAGE_SIZE) && !((_EntrySize * FreeEntryIndex) % PAGE_SIZE)) );
    
    // Cast the memory.
    NewEntry = (LPFILEMAP_ENTRY)
        (_EntryArray + _EntrySize * FreeEntryIndex);
    
    // Mark the allocated space.
    #ifdef DBG
        ResetEntryData(NewEntry, SIG_ALLOC, nBlocksRequired);
    #else
        NewEntry->dwSig = SIG_ALLOC;
    #endif // DBG

    // Set the number of blocks in the entry.
    NewEntry->nBlocks = nBlocksRequired;
        
    return NewEntry;
}


BOOL MEMMAP_FILE::FreeEntry(LPFILEMAP_ENTRY Entry)
/*++

    This public member function frees up a file cache entry.

Arguments:

    UrlEntry : pointer to the entry that being freed.

Return Value:

    TRUE - if the entry is successfully removed from the cache.
    FALSE - otherwise.

--*/
{
    DWORD nIndex, nArrayIndex, 
        nOffset, nBlocks, BitMask;

    LPDWORD BitMap;

    //
    // Validate the pointer passed in.
    //
    if( ((LPBYTE)Entry < _EntryArray) 
        || ((LPBYTE)Entry >=
           (_EntryArray + _EntrySize *
           _HeaderInfo->NumUrlInternalEntries) ) ) 
    {
        TcpsvcsDbgAssert(FALSE);
        return FALSE;
    }

    // Compute and check offset (number of bytes from start).
    nOffset = (DWORD) PtrDifference(Entry, _EntryArray);
    if( nOffset % _EntrySize ) 
    {
        // Pointer does not point to a valid entry.
        TcpsvcsDbgAssert(FALSE);
        return FALSE;
    }
    
    nBlocks = Entry->nBlocks;

    if (nBlocks > (MAX_ENTRY_SIZE / NORMAL_ENTRY_SIZE))
    {
        INET_ASSERT(FALSE);
        return FALSE;
    }

    // Compute indicese
    nIndex = nOffset / _EntrySize;
    nArrayIndex = nIndex / NUM_BITS_IN_DWORD;

    //
    // Unmark the index bits in the map.
    //

    BitMap = &_HeaderInfo->AllocationBitMap[nArrayIndex];
    BitMask = 0x1 << (nIndex % NUM_BITS_IN_DWORD);
    for (DWORD i = 0; i < nBlocks; i++)
    {
        // Check we don't free unset bits
        if (!(*BitMap & BitMask))
        {
            TcpsvcsDbgPrint(( DEBUG_ERRORS, "Attempted to free unset bits. Ignoring...\n"));
            return FALSE;
        }

        *BitMap &= ~BitMask;
        BitMask <<= 1;
        if (BitMask == 0x0)
        {
            BitMask = 0x1;
            BitMap = &_HeaderInfo->AllocationBitMap[++nArrayIndex];
        }
    }

    // Mark the freed space.
    ResetEntryData(Entry, SIG_FREE, nBlocks);

    // Reduce the count of allocated entries.
    TcpsvcsDbgAssert(_HeaderInfo->NumUrlEntriesAlloced  > 0);
    _HeaderInfo->NumUrlEntriesAlloced -= nBlocks;

return TRUE;
}


BOOL
MEMMAP_FILE::Reinitialize(void)
/*++

    This  member function reinitializes a cache index file

Arguments:



Return Value:

    Windows error code


--*/
{
    TcpsvcsDbgAssert( _FileHandle != NULL );

    // Close view, mapping, and file.
    CloseMapping();
    
    BOOL BoolError, fReinited = FALSE;
    DWORD FilePointer;

    // check for exclusive access, we do this by opening the
    // file in exclsive mode, if we succeed we are the only one
    _FileHandle = CreateFile
        (
            _FileName,
            GENERIC_WRITE,
            0,    // no read/write sharing
            CreateAllAccessSecurityAttributes(NULL, NULL, NULL),
            OPEN_ALWAYS,
            FILE_ATTRIBUTE_NORMAL,
            NULL
        );

    if (_FileHandle == INVALID_HANDLE_VALUE)
        _FileHandle = NULL;
    else
    {
        DWORD FilePointer = SetFilePointer
            ( _FileHandle, GlobalMapFileGrowSize, NULL, FILE_BEGIN);
    
        if( FilePointer != 0xFFFFFFFF)
        {
            if (SetEndOfFile (_FileHandle))
            {
                // Success!
                _FileSize = GlobalMapFileGrowSize;
                fReinited = TRUE;
            }
            else
            {
                TcpsvcsDbgPrint(( DEBUG_ERRORS, "SetEndOfFile failed: %u\n",
                    GetLastError()));
            }
        }            

        // Following will be done by RemapAddress calling CloseMapping
        // CloseHandle (_FileHandle);
        // _FileHandle = NULL
    }

    // Re-attach to the file.

    _Status = RemapAddress();

    if( _Status != ERROR_SUCCESS )
    {
        TcpsvcsDbgPrint(( DEBUG_ERRORS,
            "Reinitialize:Remap failed, %ld.\n",_Status));
        TcpsvcsDbgAssert( FALSE );
        goto Cleanup;
    }

    if (fReinited)
    {
        // if there is an exception due to bad sector, this will set
        // _status to something other than ERROR_SUCCESS
        if(!InitHeaderInfo()) 
            goto Cleanup;

        _NumBitMapDWords =
            (_HeaderInfo->NumUrlInternalEntries + (NUM_BITS_IN_DWORD - 1)) /
                NUM_BITS_IN_DWORD; // cell
    }

Cleanup:

    return fReinited;
}

BOOL
MEMMAP_FILE::InitHeaderInfo()
/*++

    This  member function intializes the memorymapped headerinfo
    structure

Arguments:



Return Value:

    None

--*/
{
    //
    // initialize file header.
    //
    BOOL fSuccess = TRUE;

    __try {
        TcpsvcsDbgAssert( _HeaderInfo != NULL );

        memcpy(_HeaderInfo->FileSignature, CACHE_SIGNATURE, sizeof(CACHE_SIGNATURE));

        _HeaderInfo->FileSize = _FileSize; // set file size in the memmap file.
        _HeaderInfo->dwHashTableOffset = 0;
        _HeaderInfo->CacheSize = (LONGLONG)0;
        _HeaderInfo->CacheLimit = (LONGLONG)0;
        _HeaderInfo->ExemptUsage = (LONGLONG)0;
        _HeaderInfo->nDirCount = 0;
        
        for (int i = 0; i < DEFAULT_MAX_DIRS; i++)
        {
            _HeaderInfo->DirArray[i].nFileCount = 0;
            _HeaderInfo->DirArray[i].sDirName[0] = '\0';
        }
        
        _HeaderInfo->NumUrlInternalEntries =
            ((_FileSize - HEADER_ENTRY_SIZE ) /
                _EntrySize );

        _HeaderInfo->NumUrlEntriesAlloced = 0;

        memset( _HeaderInfo->AllocationBitMap, 0,  sizeof(_HeaderInfo->AllocationBitMap) );
        memset( _EntryArray, 0, (_FileSize - HEADER_ENTRY_SIZE) );
        memset( _HeaderInfo->dwHeaderData, 0, sizeof(DWORD) * NUM_HEADER_DATA_DWORDS);

        _Status = ERROR_SUCCESS;
    }
    __except(EXCEPTION_EXECUTE_HANDLER) {

        _Status = ERROR_WRITE_FAULT;
        fSuccess = FALSE;
    }
    ENDEXCEPT

    return (fSuccess);
}

LPFILEMAP_ENTRY MEMMAP_FILE::FindNextEntry (DWORD* pdwEnum, DWORD dwFilter, GROUPID GroupId)
{
    while (1)
    {
        // Get the next item in the hash table.
        HASH_ITEM *pItem = HashGetNextItem (this, (LPBYTE)_BaseAddr, pdwEnum, 0);
        if (!pItem)
            return NULL;

        // continue if search entry within group but hash bit says no group 
        // (may avoid unnecessary page hit by pulling non-relevent pEntry)
        if( GroupId && !pItem->HasGroup() )
            continue;

            
        // Get the entry from the item.
        URL_FILEMAP_ENTRY* pEntry = ValidateUrlOffset (pItem->dwOffset);
        if (!pEntry)
        {
            pItem->MarkFree();
            continue;
        }
        
        // No filter - continue enum until ERROR_NO_MORE_ITEMS.
        if (!dwFilter)
            continue;

        // Temporary hack to always show 1.1 entries 
        // until we have a better way of dealing with them.
        dwFilter |= INCLUDE_BY_DEFAULT_CACHE_ENTRY;

        // Continue enum if no match on cache entry type.
        if ((dwFilter & pEntry->CacheEntryType) != pEntry->CacheEntryType)
            continue;

        // Continue enum if no match on group.
        if (GroupId ) 
        {
            if( pItem->HasMultiGroup() )
            {
                // need to search the list
                LIST_GROUP_ENTRY*   pListGroup = NULL;
                pListGroup = ValidateListGroupOffset(pEntry->dwGroupOffset);
                if( !pListGroup )
                    continue;

                BOOL fFoundOnList = FALSE;
                while( pListGroup && pListGroup->dwGroupOffset )
                {
                    GROUP_ENTRY* pGroup = NULL;
                    pGroup = ValidateGroupOffset( 
                                pListGroup->dwGroupOffset, pItem); 
                    if( !pGroup )
                    {
                        break;
                    }

                    if( GroupId ==  pGroup->gid )
                    {
                        fFoundOnList = TRUE;
                        break;
                    }

                    if( !pListGroup->dwNext )
                    {
                        break;
                    }

                    // next group on list
                    pListGroup = ValidateListGroupOffset(pListGroup->dwNext);
                }
               
                if( !fFoundOnList )
                    continue; 

            }
            else if( GroupId != 
                        ((GROUP_ENTRY*)( (LPBYTE)_BaseAddr + 
                                 pEntry->dwGroupOffset))->gid ) 
            { 
                continue;
            }

        }

        return (LPFILEMAP_ENTRY) (((LPBYTE)_BaseAddr) + pItem->dwOffset);
    }
}

BOOL MEMMAP_FILE::IsBadOffset (DWORD dwOffset)
{

    ASSERT_ISPOWEROF2 (_EntrySize);
    return (dwOffset == 0
        || (dwOffset & (_EntrySize-1))
        || (dwOffset >= _FileSize));

    return FALSE;

}


BOOL MEMMAP_FILE::IsBadGroupOffset (DWORD dwOffset)
{
    return (dwOffset == 0 || (dwOffset >= _FileSize));
    return FALSE;
}


GROUP_ENTRY* MEMMAP_FILE::ValidateGroupOffset (DWORD dwOffset, HASH_ITEM* hItem)
{
    GROUP_ENTRY *pEntry = NULL;

    // if hash item is available, check the group bit first.
    if( hItem && !hItem->HasGroup())
    {
        return NULL;
    }

    // check the offset 
    if (IsBadGroupOffset (dwOffset))
    {
        INET_ASSERT (FALSE);
        return NULL;
    }

    //
    // Validate page signature.
    // since we know all the allocated page are aligned with
    // 4K boundary, so from the offset, we can get the 
    // the offset of this page by:
    //   pageOffset = Offset - Offset(mod)4K
    //

    DWORD dwOffsetInPage = dwOffset & 0x00000FFF;
    FILEMAP_ENTRY* pFM = (FILEMAP_ENTRY*) 
        ( (LPBYTE)_BaseAddr + dwOffset - dwOffsetInPage );
    
    // Get the Group.
    if( pFM->dwSig == SIG_ALLOC && pFM->nBlocks )
    {
        pEntry = (GROUP_ENTRY *) ((LPBYTE) _BaseAddr + dwOffset);
    }
    
    return pEntry;
}



URL_FILEMAP_ENTRY* MEMMAP_FILE::ValidateUrlOffset (DWORD dwOffset)
{
    // Validate offset.
    if (IsBadOffset (dwOffset))
    {
        INET_ASSERT (FALSE);
        return NULL;
    }

    // Validate signature.
    URL_FILEMAP_ENTRY *pEntry =
        (URL_FILEMAP_ENTRY *) ((LPBYTE) _BaseAddr + dwOffset);
    if (pEntry->dwSig != SIG_URL)
    {
        INET_ASSERT (FALSE);
        return NULL;
    }
    
    // TODO: validate entry offsets, string terminations etc.
    return pEntry;
}


LIST_GROUP_ENTRY* MEMMAP_FILE::ValidateListGroupOffset (DWORD dwOffset)
{
    LIST_GROUP_ENTRY *pEntry = NULL;

    // Validate offset.
    if (IsBadGroupOffset (dwOffset))
    {
        INET_ASSERT (FALSE);
        return NULL ;
    }

    //
    // Validate page signature.
    // since we know all the allocated page are aligned with
    // 4K boundary, so from the offset, we can get the 
    // the offset of this page by:
    //   pageOffset = Offset - Offset(mod)4K
    //

    DWORD dwOffsetInPage = dwOffset & 0x00000FFF;
    FILEMAP_ENTRY* pFM = (FILEMAP_ENTRY*) 
        ( (LPBYTE)_BaseAddr + dwOffset - dwOffsetInPage );
    

    if( pFM->dwSig == SIG_ALLOC && pFM->nBlocks )
    {
        pEntry = (LIST_GROUP_ENTRY*) ((LPBYTE) _BaseAddr + dwOffset);
    }
    
    return pEntry;
}

