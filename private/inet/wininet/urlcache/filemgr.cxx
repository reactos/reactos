/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:  filemgr.cxx

Abstract:

    Manages cache file & directory creation/deletion.

Author:
    Adriaan Canter (adriaanc) 04-02-97

Modifications:
    Ahsan Kabir (akabir) 25-Sept-97 made minor alterations.
    
--*/


#include <cache.hxx>
#define WWW_DOT "www."

#define MAP_KEY_TO_PATH    0
#define MAP_PATH_TO_KEY    1


//
//==================== CFileMgr Public Functions =============================
//


/*-----------------------------------------------------------------------------
CFileMgr::CFileMgr
----------------------------------------------------------------------------*/
CFileMgr::CFileMgr(MEMMAP_FILE* mmFile, DWORD dwOptions) : _mmFile(mmFile), _dwOptions(dwOptions)
{
    INET_ASSERT(_mmFile);

    // GetFullPathNameLen includes trailing backslash.
    _cbBasePathLen = _mmFile->GetFullPathNameLen();
}


/*-----------------------------------------------------------------------------
CFileMgr::~CFileMgr
----------------------------------------------------------------------------*/
CFileMgr::~CFileMgr()
{}


/*-----------------------------------------------------------------------------
virtual CFileMgr::Init
----------------------------------------------------------------------------*/
BOOL CFileMgr::Init()
{
    return TRUE;
}


/*-----------------------------------------------------------------------------
virtual GetDirLen
Returns length of cache dir path.
----------------------------------------------------------------------------*/
DWORD CFileMgr::GetDirLen(DWORD nKey)
{
    return _cbBasePathLen;
}


/*-----------------------------------------------------------------------------
virtual CFileMgr::CreateUniqueFile
Generates cache files.
----------------------------------------------------------------------------*/
DWORD CFileMgr::CreateUniqueFile(LPCSTR szUrl, LPTSTR szFileName,
                                LPTSTR szFileExtension, HANDLE *phfHandle)
{
    return CreateUniqueFile((LPCSTR) szUrl, (LPTSTR) _mmFile->GetFullPathName(),
        (LPTSTR) szFileName, (LPTSTR) szFileExtension, (HANDLE*) phfHandle);
}


/*-----------------------------------------------------------------------------
virtual CFileMgr::NotifyCommit
No-op.
----------------------------------------------------------------------------*/
BOOL CFileMgr::NotifyCommit(DWORD nDirIndex)
{
    return TRUE;
}


/*-----------------------------------------------------------------------------
CFileMgr::DeleteCache
----------------------------------------------------------------------------*/
BOOL CFileMgr::DeleteCache(LPSTR szRoot)
{
    BOOL fHasCacheVu = IsValidCacheSubDir(szRoot);

    if ( fHasCacheVu)
        DisableCacheVu(szRoot);
        
    if (DeleteCachedFilesInDir(szRoot) == ERROR_SUCCESS)
    {
        SetFileAttributes(szRoot, FILE_ATTRIBUTE_DIRECTORY);
        RemoveDirectory(szRoot);
    }

    if( fHasCacheVu)
        EnableCacheVu( szRoot);
    
    return TRUE;
}

/*-----------------------------------------------------------------------------
CFileMgr::Cleanup
----------------------------------------------------------------------------*/
BOOL CFileMgr::Cleanup()
{
    return TRUE;
}

/*-----------------------------------------------------------------------------
virtual CFileMgr::GetDirIndex
----------------------------------------------------------------------------*/
BOOL CFileMgr::GetDirIndex(LPSTR szFilePath, LPDWORD pnIndex)
{
    *pnIndex = NOT_A_CACHE_SUBDIRECTORY;
    return TRUE;
}


/*-----------------------------------------------------------------------------
virtual CFileMgr::GetFilePathFromEntry

Retrieves the full path to the cache subdirectory for a cache entry.
Maps the directory index from the URL_FILEMAP_ENTRY pointer passed in
to a string containing the full path.
----------------------------------------------------------------------------*/
BOOL CFileMgr::GetFilePathFromEntry(URL_FILEMAP_ENTRY *pEntry,
                                    LPSTR szSubDirPath, LPDWORD pcb)
{
    INET_ASSERT(pEntry && szSubDirPath && pcb && *pcb);

    // "anyuser@msn.txt"
    LPTSTR szFile = (LPTSTR) OFFSET_TO_POINTER(pEntry, pEntry->InternalFileNameOffset);

    // Lengths of path and file.
    DWORD cbFile = strlen(szFile);
    DWORD cbPath = _mmFile->GetFullPathNameLen();

    // Don't overflow output buffer.
    DWORD cbSubDirPath = cbPath + cbFile;
    if (cbSubDirPath + 1 > *pcb)
    {
        INET_ASSERT(FALSE);
        return FALSE;
    }

    // "C:\Windows\Profiles\anyuser\Cookies\"
    memcpy(szSubDirPath, _mmFile->GetFullPathName(), cbPath);

    // "C:\Windows\Profiles\anyuser\Cookies\anyuser@msn.txt"
    memcpy(szSubDirPath + cbPath, szFile, cbFile + 1);

    *pcb = cbSubDirPath;

    return TRUE;
}


/*-----------------------------------------------------------------------------
virtual CFileMgr::DeleteOneCachedFile
Deletes a single cache file given the absolute path.
----------------------------------------------------------------------------*/
BOOL CFileMgr::DeleteOneCachedFile(LPSTR lpszFileName,
                                   DWORD dostEntry, DWORD nIndex)
{
    return ::DeleteOneCachedFile(lpszFileName, dostEntry);
}


/*-----------------------------------------------------------------------------
    virtual BOOL  CreateDirWithSecureName( LPSTR);

Creates a cache directory with a given name to allow existing directories
to be copied into another cache file.  Just the eight letters of the new
directory are given.
----------------------------------------------------------------------------*/
BOOL CFileMgr::CreateDirWithSecureName( LPSTR szDirName)
{
    return _mmFile->CreateDirWithSecureName( szDirName);
}


//
//================== CFileMgr Protected Functions =============================
//

/*-----------------------------------------------------------------------------
CFileMgr::GetStoreDirectory
    Returns "%windir%\web\" - ie "C:\Windows\Web\" and length. There
    is currently only ONE store directory and this is it.
----------------------------------------------------------------------------*/
BOOL CFileMgr::GetStoreDirectory(LPSTR szPath, LPDWORD pcbPath)
{
    DWORD cb;

    // Hardwired to "%windir%\Web\"
    if ((cb = GetWindowsDirectory(szPath, MAX_PATH)) && cb<=MAX_PATH)
    {
        AppendSlashIfNecessary(szPath, cb);
        memcpy(szPath + cb, WEBDIR_STRING, sizeof(WEBDIR_STRING));
        cb += sizeof(WEBDIR_STRING) - 1;
        *pcbPath = cb;
        return TRUE;
    }
    INET_ASSERT(FALSE);
    return FALSE;
}

/*-----------------------------------------------------------------------------
CFileMgr::MapStoreKey
    Maps path to storage directory key (stored in the FILEMAP_ENTRY::DirIndex)
    or storage directory key to path (ie C:\Windows\Web\). There is currently
    only one key and it is INSTALLED_DIRECTORY_KEY. Mapping depends on dwFlag.
----------------------------------------------------------------------------*/
BOOL CFileMgr::MapStoreKey(LPSTR szPath, LPDWORD pcbPath, 
                           LPDWORD dwKey, DWORD dwFlag)
{
    DWORD cb;
    BOOL fReturn = FALSE;
    CHAR szStoreDir[MAX_PATH];

    // Must be able to get store dir in any case.
    if (GetStoreDirectory(szStoreDir, &cb))
    {
        // Mapping a path to a key requested.
        if (dwFlag == MAP_PATH_TO_KEY)
        {
            // Path matches?
            if ((*pcbPath == cb) 
                && !strnicmp(szStoreDir, szPath, cb))
            {
                // We only map one directory for now.
                *dwKey = INSTALLED_DIRECTORY_KEY;
                fReturn = TRUE;
            }
        }

        // Mapping a key to a path requested.    
        else if (dwFlag == MAP_KEY_TO_PATH)
        {
            if (*dwKey == INSTALLED_DIRECTORY_KEY)
            {
                memcpy(szPath, szStoreDir, cb+1);
                *pcbPath = cb;
                fReturn = TRUE;
            }
        }
    }
    //INET_ASSERT(fReturn);
    return fReturn;
}



/*-----------------------------------------------------------------------------
CreateUniqueFile

Routine Description:

Arguments:

    UrlName : pointer to url name.

    Path : pointer to cache path.

    FileName : pointer to a buffer that receives the full path name of the
        newly created file.

    Extension : if specified the extension is used to make random file.

Return Value:

    Windows Error Code.
----------------------------------------------------------------------------*/
DWORD CFileMgr::CreateUniqueFile(LPCSTR UrlName, LPTSTR Path,
                                 LPTSTR FileName, LPTSTR Extension, 
                                 HANDLE *phfHandle)
{
    DWORD cbPath, cbName, cbFull;
    cbPath = strlen(Path);

    DWORD Error, len, lenExt=0;

    TCHAR RandomFileName[MAX_PATH];

    TCHAR FullFileName[MAX_PATH];

    HANDLE FileHandle;

    DWORD dwCollision = 0, dwTotalCollissionCount;

    char szHost[MAX_PATH], szUrl[MAX_PATH], szExtraInfo[MAX_PATH];
    URL_COMPONENTS sUrl;

    LPTSTR FileNamePtr = FileName, lpT;
    
    BOOL fUseFileName = FALSE, fPrettyName = FALSE;

    DWORD cbFileName;
    CHAR szExt[MAX_PATH];
    *szExt = '\0';    

    if (phfHandle)
        *phfHandle = INVALID_HANDLE_VALUE;

    // If a filename has been passed in attempt to use it.
    if (FileName[0] != '\0')
    {
        // Various checks to determine validity of name.

        // First strip any trailing whitespace.
        cbFileName = strlen(FileName);
        StripTrailingWhiteSpace(FileName, &cbFileName);

        // Check length.
        if (cbFileName < MAX_PATH)
        {            

            // '.' and '..' are illegal.
            if (memcmp(FileName, ".", sizeof("."))
                && memcmp(FileName, "..", sizeof("..")))
            {
                // slashes and backslashes are illegal.
                LPTSTR ptr = FileName;
                while (*ptr != '\0')
                {
                    if (IsDBCSLeadByte(*ptr))
                        ++ptr;
                    else
                    if (*ptr == '\\' || *ptr == '/')
                        break;
                    ptr++;
                }
                

                // Filename has no slashes in it.
                if (!*ptr)
                {
                    // Preliminary judgment. Creating
                    // this file could possibly fail,
                    // depending on further tests.
                    fUseFileName = TRUE;
                }
            }
        }
    }

    // Preliminary checks indicate valid filename.
    if (fUseFileName)
    {
        // Attempt to parse a file extension.

        CHAR* pExt = StrChr(FileName, '.');

        // Found a file extension delimiter.
        if (pExt)
        {
            // '.' but no extension (eg "foo.")
            if (*(pExt + 1) == '\0')
            {
                *pExt = '\0';
                len = cbFileName - 1;
            }

            // '.' at beginning (eg ".foo") Valid file, no extension.
            else if (pExt == FileName)
            {
                len = cbFileName;
            }

            // Common case (eg foo.bar)
            else
            {
                // Separate out the file extension w/o '.'
                lenExt = (DWORD) (cbFileName - (pExt - FileName) - 1);  // 64BIT
                memcpy(szExt, pExt+1, lenExt + 1);

                // Filename without extension.
                *pExt = '\0';
                len = (DWORD) (pExt - FileName);     // 64BIT
            }
        }     

        // No file extension found
        else
        {
            len = cbFileName;
        }
        fPrettyName = TRUE;
        goto have_file_name;
    }

    // No or bad filename passed in.
    else
    {
        // Copy over any extension passed in,
        // limiting the length as necessary.
        if (Extension)
        {
            lenExt = strlen(Extension);
            if (lenExt >= MAX_PATH)
            {
                lenExt = MAX_PATH - 1;
            }
            memcpy(szExt, Extension, lenExt);
            szExt[lenExt] = '\0';
        }
        else
        {
            *szExt = '\0';
            lenExt = 3;
        }
    }

    memset(&sUrl, 0, sizeof(sUrl));

    sUrl.dwStructSize = sizeof(sUrl);

    sUrl.lpszHostName = szHost;
    sUrl.dwHostNameLength = sizeof(szHost);

    sUrl.lpszUrlPath = szUrl;
    sUrl.dwUrlPathLength = sizeof(szUrl);


    sUrl.lpszExtraInfo = szExtraInfo;
    sUrl.dwExtraInfoLength = sizeof(szExtraInfo);



    if (InternetCrackUrl(UrlName, lstrlen(UrlName), 0, &sUrl)) {
        fPrettyName = TRUE;

        if ((sUrl.dwUrlPathLength == 1) && (szUrl[0] == '/')) {

            FileNamePtr = szHost;
            len = sUrl.dwHostNameLength;

            // strip out www., this info is redundant

            if (!strnicmp(FileNamePtr, WWW_DOT, sizeof(WWW_DOT)-1)) {

                len -= (sizeof(WWW_DOT)-1);

                // copy the NULL terminator too

                memmove(FileNamePtr, FileNamePtr+sizeof(WWW_DOT)-1,len+1);

            }
        }
        else {

            FileNamePtr = szUrl;
            len = sUrl.dwUrlPathLength;

            // if there is a terminating slash let us fix it.
            // len-1 wont' break because we know the url is more than 1 char

            if (FileNamePtr[len-1] == '/') {

                FileNamePtr[len-1] = 0;
                --len;

            }

            // get the tail
            if (lpT=StrRChrA(FileNamePtr, FileNamePtr+len-1, '/'))
            {
                ++lpT;

                len = len - (DWORD)PtrDifference(lpT, FileNamePtr);

                // copy the NULL terminator as well
                memmove(FileNamePtr, lpT, len+1);
            }

            // Special hack for cookies: Ensure that the username is
            // prepended on to the filename. The preceding filename
            // generation code does not generate this for cookies
            // which specify paths in addition to domains.      
            if (!memcmp(UrlName, COOKIE_PREFIX, sizeof(COOKIE_PREFIX) - 1))
            {                
                // This is a cookie url of the form Cookie:username@domain/path
                if (GetWininetUserName())
                {
                    DWORD cb = vdwCurrentUserLen;
                    if (len + cb + 1 < MAX_PATH)
                    {
                        if (memcmp(FileNamePtr, vszCurrentUser, cb) 
                            || FileNamePtr[cb] != '@'
                            || FileNamePtr[cb+1] == '\0')
                        {
                            memmove(FileNamePtr + cb + 1, FileNamePtr, len+1);
                            FileNamePtr[cb] = '@';
                            memcpy(FileNamePtr, vszCurrentUser, cb);
                            len += cb + 1;
                        }
                    }
                }
            }
        
        }

        
        if (lpT=StrRChrA(FileNamePtr, FileNamePtr+len-1, '.'))
        {
            *lpT = 0;
            len = (DWORD) PtrDifference(lpT, FileNamePtr);
        }

        // convert all invalid char (including '%') to '_'
        for(lpT = FileNamePtr; *lpT; ++lpT) 
        {
            if (IsDBCSLeadByte(*lpT))
                ++lpT;
            else if ((strchr(vszInvalidFilenameChars, *lpT))) 
                *lpT = '_';
        }

        if ((cbPath+len+lenExt+2) > MAX_PATH) {

            fPrettyName = FALSE;

        }
    }
    else {

        fPrettyName = FALSE;
    }


have_file_name:


    for(dwTotalCollissionCount = 0;
        dwTotalCollissionCount < MAX_COLLISSION_ATTEMPTS;
        dwTotalCollissionCount++) {


        //
        // make a random file name.
        //
        if (!fPrettyName) 
        {
            // If fUseFileName is TRUE, it means we've attempted to create
            // a file using the filename passed in and failed. We still want 
            // to create a cache file, but since the extension parsed from the
            // filename is also suspect, we want to create a cache filename
            // without any passed in extension, or NULL.
            if (fUseFileName)
            {
                if (Extension)
                {
                    lenExt = strlen(Extension);
                    memcpy(szExt, Extension, lenExt+1);
                }
                else
                {
                    lenExt = 0;
                    *szExt = '\0';
                }
            }

            Error = MakeRandomFileName(UrlName, RandomFileName, szExt);

        }
        else {

            DWORD digit;
            DWORD cb = strlen(FileNamePtr);
            memcpy(RandomFileName, FileNamePtr, cb+1);

            lpT = RandomFileName+len;

            // Always decorate the cache file name with [1-99]
            // We use square brackets instead of parens because
            // NT cmd shell barfs on parens.
            
            if (++dwCollision > 99)
            {
                fPrettyName = FALSE;
                continue;
            }

#ifndef UNIX
            lpT += wsprintf (lpT, "[%d]", dwCollision);
#else
            /* Square brackets cause problems on UNIX */
            lpT += wsprintf (lpT, "(%d)", dwCollision);
#endif /* UNIX */
            if (*szExt)
            {
                *lpT++ = '.';
                memcpy(lpT, szExt, lenExt + 1);
            }
            
            Error = ERROR_SUCCESS;

        }

        if (Error != ERROR_SUCCESS) {
            INET_ASSERT(FALSE);
            return (Error);

        }

        cbName = strlen(RandomFileName);
        cbFull = cbPath + cbName + 1;

        if (cbFull > MAX_PATH)
        {
            INET_ASSERT(FALSE);
            return(ERROR_FILENAME_EXCED_RANGE);
        }
#ifndef UNIX
        // Hack for special DOS filenames:
        // aux.*, com[0-9].*, con.*, lpt[0-9].*, 
        // nul.* and prn.* on non-NT platforms.
        if (!IsPlatformWinNT())
        {
            DWORD cbMajor = cbName - lenExt;
            if (cbMajor == 4 || cbMajor == 5)
            {
                switch(tolower(*RandomFileName))
                {
                    // Test for aux.*
                    case 'a':
                    if (!strnicmp(RandomFileName + 1, "ux.", 3))
                    {
                        continue;
                    }
                    break;

                    // Test for com[0-9].* and con.*
                    case 'c':
                    if (tolower(RandomFileName[1]) == 'o')
                    {
                        CHAR c = tolower(RandomFileName[2]);
                        if (c == 'm')
                        {
                            if (isdigit(RandomFileName[3])
                                && RandomFileName[4] == '.')
                            {
                                continue;
                            }
                        }
                        else if (c == 'n')
                        {
                            if (RandomFileName[3] == '.')
                            {
                                continue;
                            }
                        }
                    }
                    break;
    
                    // Test for lpt[0-9].*
                    case 'l':
                    {
                        if (!strnicmp(RandomFileName + 1, "pt", 2)
                            && isdigit(RandomFileName[3])
                            && RandomFileName[4] == '.')
                        {
                            continue;
                        }
                        break;
                    }

                    // Test for nul.*
                    case 'n':
                    {
                        if (!strnicmp(RandomFileName + 1, "ul.", 3))
                        {
                            continue;
                        }
                        break;
                    }

                    // Test for prn.*
                    case 'p':
                    {
                        if (!strnicmp(RandomFileName + 1, "rn.", 3))
                        {
                            continue;
                        }
                        break;
                    }
                }
            }
        }
#endif /* !UNIX */

        // Make full path name.
        memcpy(FullFileName, Path, cbPath);
        memcpy(FullFileName + cbPath, RandomFileName, cbName + 1);

        // Check if this file exists.
        if (GetFileAttributes(FullFileName)!=0xffffffff) 
        {
            // A file or dir by this name exists.
            // This will also take care of special DOS filenames
            // on NT, which return !0xffffffff.
            continue;
        }



        FileHandle = CreateFile(
                        FullFileName,
                        GENERIC_WRITE,
                        FILE_SHARE_READ | FILE_SHARE_WRITE,
                        NULL,
                        CREATE_NEW,
                        FILE_ATTRIBUTE_NORMAL,
                        NULL );

        if( FileHandle != INVALID_HANDLE_VALUE ) {

            //
            // successfully create a new file; 
            // either return handle or close it and return.
            //
            if ( phfHandle )
                *phfHandle = FileHandle;
            else
                CloseHandle( FileHandle );

            break;
        }
        else
        {
            // Couldn't create the file. This is possibly due to the file
            // already existing or to the fact that the directory was deleted.

            // Check for the existance of the directory:
            if (GetFileAttributes(Path) == 0xffffffff)
            {
                // Directory was deleted. Create one and then
                // create the file.
                if (CreateDirectory(Path, NULL))
                {
                    // Set system attribute.
                    SetFileAttributes(Path, FILE_ATTRIBUTE_SYSTEM);

                    // Enable cachevu in this directory
                    if (!(GetOptions() & INTERNET_CACHE_CONTAINER_NODESKTOPINIT))
                        EnableCacheVu(Path);

                    FileHandle = CreateFile(
                            FullFileName,
                            GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

                    // We just created the directory and the
                    // child file, so the file handle should
                    // be valid.
                    if( FileHandle != INVALID_HANDLE_VALUE )
                    {
                        if (phfHandle)
                            *phfHandle = FileHandle;
                        // Successfully create a new file close it and return.
                        else
                            CloseHandle( FileHandle );
                        break;
                    }
                }
            }
        }

        Error = GetLastError();

        if( Error != ERROR_FILE_EXISTS )
        {
            if (!fPrettyName)
            {
                INET_ASSERT(FALSE);
                return( Error );
            }
            else
            {
                fPrettyName = FALSE;
                Error = ERROR_SUCCESS;
            }
        }
        else {

            // We found that the file exists
            // if it is zero size, let us just use it.
            // this in itself is an unlikely occurrence
            // but we any way try to work around the IBM virus software

            // ACHTUNG!!! this is a special hack for IBM antivirus software

            FileHandle = CreateFile(
                            FullFileName,
                            GENERIC_WRITE,
                            FILE_SHARE_READ | FILE_SHARE_WRITE,
                            NULL,
                            OPEN_EXISTING,
                            FILE_ATTRIBUTE_NORMAL,
                            NULL );

            if (FileHandle != INVALID_HANDLE_VALUE) {

                // this looks usable
                if (GetFileSize(FileHandle, NULL) == 0)
                {
                    if (phfHandle)
                        *phfHandle = FileHandle;
                    else
                        CloseHandle( FileHandle );
                    break;
                }
                    
                CloseHandle( FileHandle );
                INET_ASSERT(FALSE);
            }
            return (ERROR_DISK_FULL);
        }

        //
        // try another random file.
        //
    } // end of the for loop

    if (dwTotalCollissionCount < MAX_COLLISSION_ATTEMPTS) {

        memcpy(FileName, FullFileName, cbFull);
        return( ERROR_SUCCESS );

    }
    else {
        INET_ASSERT(FALSE);
        return (ERROR_DISK_OPERATION_FAILED);

    }
}

/*-----------------------------------------------------------------------------
MakeRandomFileName

  Routine Description:

    Creates a random 8.3 file name. The format of the name will be as
    below:

        ca(0-99999).(0-999)

    Ex ca19200.340
    Ex ca19354.tmp - if an extension (tmp) is specified.

Arguments:

    UrlName : pointer to an URL string

    FileName : pointer to a string buffer where the random file name is
        returned. The buffer length must be atleast 8+3+1+1= 13 wchars.

    Extension : pointer to an extension string. if this is non-NULL, then
        the specified extension is used otherwise random extension as
        explained above is used.

Return Value:

    none.
----------------------------------------------------------------------------*/
DWORD CFileMgr::MakeRandomFileName(LPCSTR UrlName,
                                      LPTSTR FileName, LPTSTR Extension)
{
    DWORD RandNum;
    LPTSTR FileNamePtr = FileName;
    static Counter;
    DWORD i;
    DWORD cbExtension = 0;

    if (Extension)
        cbExtension = lstrlen(Extension) + 1;

    if (cbExtension > (MAX_PATH-(8+1)))
    {
        return(ERROR_FILENAME_EXCED_RANGE);
    }

    // Additional special hack for cookie urls.
    if (!memcmp(UrlName, COOKIE_PREFIX, sizeof(COOKIE_PREFIX) - 1))
    {                
        // This is a cookie url of the form Cookie:username@domain/path
        if (GetWininetUserName())
        {
          DWORD cb = vdwCurrentUserLen;
          
          if (cb + 8 + cbExtension + 1 < MAX_PATH)
          {
              memcpy(FileName, vszCurrentUser, cb);
              FileName[cb] = '@';
              FileNamePtr += (cb + 1);
          }
        }
    }
    
    // Check that the total name doesn't exceed MAX_PATH
    // Our total name is 8 chars basename + a dot + the extension + 0


    *FileNamePtr++ = L'C';
    *FileNamePtr++ = L'A';

    //
    // generate a six digits random string;
    //

    //
    // We can't use rand() alone to generate a random number because it will
    // repeat the same sequence for each new thread that comes in.  We can't
    // use the TickCount alone because it is a little too predicable.  But
    // the two combined should be nice.  Adding in Counter will take care of
    // the case of two brand-new threads coming in at the same time.
    //


    for ( i = 0; i < 6; i++) {
        UINT digit;

        RandNum = (GetTickCount() * rand()) + Counter++;

        digit = RandNum % 36; // 10 digits + 26 letters

        *FileNamePtr++  = ( digit < 10 ) ? (CHAR)('0' + digit) : (CHAR)('A' + (digit - 10));
    }

    *FileNamePtr++ = L'.';
  
    
    //
    // if an extension is specified, use it.
    //
    if( Extension != NULL )
    {
        // if a 0 extension if provided, we will create a
        // file with no extension
        memcpy(FileNamePtr, Extension, cbExtension);
        return ERROR_SUCCESS;
    }

    // Append default file extension.
    memcpy(FileNamePtr, DEFAULT_FILE_EXTENSION, sizeof(DEFAULT_FILE_EXTENSION));
    return ERROR_SUCCESS;
}






//
//===================== CSecFileMgr Public Functions ==========================
//


/*-----------------------------------------------------------------------------
CSecFileMgr::CSecFileMgr
----------------------------------------------------------------------------*/
CSecFileMgr::CSecFileMgr(MEMMAP_FILE* mmFile, DWORD dwOptions) : CFileMgr(mmFile, dwOptions)
{
    INET_ASSERT(_mmFile);

    // BUGBUG - have to guard against getting out of sync with dirs.
    if (_mmFile->GetDirCount() == 0)
        Init();
}


/*-----------------------------------------------------------------------------
CSecFileMgr::~CSecFileMgr
----------------------------------------------------------------------------*/
CSecFileMgr::~CSecFileMgr()
{}


/*-----------------------------------------------------------------------------
CSecFileMgr::Init
----------------------------------------------------------------------------*/
BOOL CSecFileMgr::Init()
{
    if (!(GetOptions() & INTERNET_CACHE_CONTAINER_NODESKTOPINIT))
        EnableCacheVu(_mmFile->GetFullPathName());

    return CreateAdditionalSubDirectories(DEFAULT_DIR_TABLE_GROW_SIZE);
}


/*-----------------------------------------------------------------------------
GetDirLen()
Returns cache dir path length.
----------------------------------------------------------------------------*/
DWORD CSecFileMgr::GetDirLen(DWORD nKey)
{
    DWORD cb = 0;

    if (nKey < DEFAULT_MAX_DIRS)
    {    
        // + 1 to account for trailing backslash.
        cb = _cbBasePathLen + DIR_NAME_SIZE + 1;
    }
    else
    {
        CHAR szStoreDir[MAX_PATH];
        GetStoreDirectory(szStoreDir, &cb);
    }
    INET_ASSERT(cb);
    return cb;
}

/*-----------------------------------------------------------------------------
CSecFileMgr::CreateUniqueFile
Creates a cache file.
----------------------------------------------------------------------------*/
DWORD CSecFileMgr::CreateUniqueFile(LPCSTR szUrl, LPTSTR szFileName,
                                   LPTSTR szFileExtension, HANDLE *phfHandle)
{
    DWORD nDir, nFiles;
    DWORD nDirCount = _mmFile->GetDirCount();

    INET_ASSERT(nDirCount <= DEFAULT_MAX_DIRS);

    FindMinFilesSubDir(nDir, nFiles);

    if (nFiles >= MAX_FILES_PER_CACHE_DIRECTORY
        && nDirCount < DEFAULT_MAX_DIRS)
    {
        if (CreateAdditionalSubDirectories(DEFAULT_DIR_TABLE_GROW_SIZE))
            nDir++;
    }

    // Get the cache path and subdirectory
    // from the memory mapped file
    CHAR szSubDirPath[MAX_PATH];

    DWORD cb = _mmFile->GetFullPathNameLen();
    memcpy(szSubDirPath, _mmFile->GetFullPathName(), cb);

    _mmFile->GetDirName(nDir, szSubDirPath + cb);
    memcpy(szSubDirPath + cb + DIR_NAME_SIZE, DIR_SEPARATOR_STRING, sizeof(DIR_SEPARATOR_STRING));
    return CFileMgr::CreateUniqueFile((LPCSTR) szUrl, (LPTSTR) szSubDirPath,
                                      (LPTSTR) szFileName, (LPTSTR) szFileExtension,
                                      (HANDLE*) phfHandle);
}


/*-----------------------------------------------------------------------------
CSecFileMgr::NotifyCommit
Tracks committed cache file counts.
----------------------------------------------------------------------------*/
BOOL CSecFileMgr::NotifyCommit(DWORD nDirIndex)
{
    INET_ASSERT(_mmFile->GetDirCount() <= DEFAULT_MAX_DIRS);

    // Regular random subdir
    if (nDirIndex < _mmFile->GetDirCount())
    {
        _mmFile->IncrementFileCount(nDirIndex);
        return TRUE;
    }
    // Not a directory.
    else if (nDirIndex == NOT_A_CACHE_SUBDIRECTORY)
    {
        //INET_ASSERT(FALSE);
        //return FALSE;
        // May be an absolute path EDITED_CACHE_ENTRY so pass
        return TRUE;
    }

    // Otherwise this had better be an installed directory.
    INET_ASSERT(nDirIndex == INSTALLED_DIRECTORY_KEY);
    return TRUE;
}


/*-----------------------------------------------------------------------------
CSecFileMgr::Cleanup
----------------------------------------------------------------------------*/
BOOL CSecFileMgr::Cleanup()
{

    CHAR szPath[MAX_PATH];

    DWORD cb = _mmFile->GetFullPathNameLen();
    memcpy(szPath, _mmFile->GetFullPathName(), cb+1);

    if (!AppendSlashIfNecessary(szPath, cb))
        return FALSE;

    memcpy(szPath + cb, "*.*", sizeof("*.*"));

    WIN32_FIND_DATA fd;

    HANDLE handle = FindFirstFile(szPath, &fd);

    if (handle == INVALID_HANDLE_VALUE)
        return FALSE;

    do
    {
        if ((fd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            && (strcmp(fd.cFileName, ".") && strcmp(fd.cFileName, "..")))
        {
            memcpy(szPath + cb, fd.cFileName, strlen(fd.cFileName) + 1);

            // Only delete directory if it is a valid subdirectory.
            if (IsValidCacheSubDir(szPath))
            {
                if (_mmFile->GetDirIndex(szPath) == NOT_A_CACHE_SUBDIRECTORY)
                {
                    DisableCacheVu(szPath);
                    if (DeleteCachedFilesInDir(szPath) == ERROR_SUCCESS)
                    {
                        SetFileAttributes(szPath, FILE_ATTRIBUTE_DIRECTORY);
                        RemoveDirectory(szPath);
                    }
                }
            }
        }
    } while (FindNextFile(handle, &fd));


    FindClose(handle);

  return TRUE;
}


/*-----------------------------------------------------------------------------
CSecFileMgr::GetDirIndex
Returns index of random cache subdirectory from an absolute file path.
----------------------------------------------------------------------------*/
BOOL CSecFileMgr::GetDirIndex(LPSTR szAbsPath, LPDWORD pnIndex)
{
    BOOL fReturn = FALSE;
    DWORD idx;

    INET_ASSERT(pnIndex);

    if (szAbsPath && *szAbsPath)
    {
        // First look in mem map file for regular dir.
        idx = _mmFile->GetDirIndex(szAbsPath);

        // If didn't find it in the mem map file,
        // check if it is the storage directory.
        if (idx == NOT_A_CACHE_SUBDIRECTORY)
        {
            // First we need to find the path to the file sans \filename.ext
            DWORD cbAbsPath = strlen(szAbsPath);
            LPSTR ptr = StrRChr(szAbsPath, szAbsPath + cbAbsPath, DIR_SEPARATOR_CHAR);
            if (ptr)
            {
                // Separate path from filename and attempt to map.
                // Note - trailing slash is included in path mapped.
                DWORD cbPath = (DWORD) (ptr - szAbsPath + 1);   // 64BIT
                if (MapStoreKey(szAbsPath, &cbPath, &idx, MAP_PATH_TO_KEY))
                {
                    *pnIndex = idx;
                    fReturn = TRUE;
                }
                // Must be an EDITED_CACHE_ENTRY set at an absolute path so just update the idx and fail
                else
                {
                    *pnIndex = NOT_A_CACHE_SUBDIRECTORY;
                }
            }
        }

        // Otherwise, this should be a valid cache subdirectory.
        else
        {
            *pnIndex = idx;
            fReturn = TRUE;
        }
    }
    else
    {
        *pnIndex = NOT_A_CACHE_SUBDIRECTORY;
    }
    
    return fReturn;
}



/*-----------------------------------------------------------------------------
CSecFileMgr::GetFilePathFromEntry

Retrieves the full path to the cache subdirectory for a cache entry.
Maps the directory index from the URL_FILEMAP_ENTRY pointer passed in
to a string containing the full path.
----------------------------------------------------------------------------*/
BOOL CSecFileMgr::GetFilePathFromEntry(URL_FILEMAP_ENTRY *pEntry,
                                       LPSTR szAbsPath, LPDWORD pcb)
{
    DWORD cbSubDirPath, cbFile;

    INET_ASSERT(pEntry && szAbsPath && pcb && *pcb);
    
    // Get file name and length - eg "default.html"
    LPTSTR szFile = (LPTSTR) OFFSET_TO_POINTER(pEntry, pEntry->InternalFileNameOffset);
    cbFile = strlen(szFile);
    
    // Make real time check?
    INET_ASSERT(cbFile && (cbFile < MAX_PATH));
    
    // Does entry reside in on of the random subdirs?
    if (pEntry->DirIndex != NOT_A_CACHE_SUBDIRECTORY 
        && pEntry->DirIndex < DEFAULT_MAX_DIRS)
    {
        // Path length.
        DWORD cbFull = _mmFile->GetFullPathNameLen();

        // Don't overflow output buffer.
        cbSubDirPath =
              cbFull
            + DIR_NAME_SIZE
            + sizeof(DIR_SEPARATOR_STRING) - 1
            + cbFile;

        if (cbSubDirPath + 1 > *pcb)
        {
            // INET_ASSERT(FALSE);
            return FALSE;
        }

        // "C:\Windows\Profiles\anyuser\Temporary Internet Files\"
        memcpy(szAbsPath, _mmFile->GetFullPathName(), cbFull);

        // "C:\Windows\Profiles\anyuser\Temporary Internet Files\XAQRTLY7"
        _mmFile->GetDirName(pEntry->DirIndex, szAbsPath + cbFull);

        // "C:\Windows\Profiles\anyuser\Temporary Internet Files\XAQRTLY7\"
        memcpy(szAbsPath + cbFull + DIR_NAME_SIZE, DIR_SEPARATOR_STRING, sizeof (DIR_SEPARATOR_STRING));

        // "C:\Windows\Profiles\anyuser\Temporary Internet Files\XAQRTLY7\default.htm"
        memcpy(szAbsPath + cbFull + DIR_NAME_SIZE + sizeof(DIR_SEPARATOR_STRING) - 1, szFile, cbFile + 1);
    }
 
    // There is no cache subdirectory which has been can be mapped
    // from the index. See if there is an existing store mapping.
    else
    {

        if (pEntry->DirIndex == NOT_A_CACHE_SUBDIRECTORY) 
        // Assume an ECE absolute path item as AddURL only writes NACS entries with ECE set
        {
            cbSubDirPath = cbFile + 1;  // Add terminator to size
            if (cbSubDirPath > *pcb)
            {
                INET_ASSERT(FALSE);
                return FALSE;
            }
            memcpy(szAbsPath, szFile, cbSubDirPath);
        }
        else
        {

            INET_ASSERT(pEntry->DirIndex == INSTALLED_DIRECTORY_KEY);
    
            DWORD cbPath = 0;
            DWORD dwIndex = INSTALLED_DIRECTORY_KEY;
            if (MapStoreKey(szAbsPath, &cbPath, &dwIndex, MAP_KEY_TO_PATH))
            {
                // "C:\Winnt\Web\"
                cbSubDirPath = cbPath + cbFile;
                if (cbSubDirPath + 1 > *pcb)
                {
                    INET_ASSERT(FALSE);                
                    return FALSE;
                }
                // "C:\Winnt\Web\default.html"
                memcpy(szAbsPath + cbPath, szFile, cbFile + 1);
            }
            else
            {
                INET_ASSERT(FALSE);
                return FALSE;
            }
        }            
        
    }

    // Hand out the absolute path to the file.
    *pcb = cbSubDirPath;
    
    return TRUE;
}


/*-----------------------------------------------------------------------------
CSecFileMgr::DeleteOneCachedFile
Deletes one cache file and decrements the file count.
----------------------------------------------------------------------------*/
BOOL CSecFileMgr::DeleteOneCachedFile(LPSTR lpszFileName,
                         DWORD dostEntry, DWORD nDirIndex)
{
    if (!::DeleteOneCachedFile(lpszFileName, dostEntry))
        return FALSE;

    INET_ASSERT(nDirIndex != NOT_A_CACHE_SUBDIRECTORY);
    _mmFile->DecrementFileCount(nDirIndex);

    return TRUE;
}



//
//==================== CSecFileMgr Protected Functions =======================
//



/*-----------------------------------------------------------------------------
CSecFileMgr::CreateRandomDirName
Creates a random subdirectory name under the root container path.
----------------------------------------------------------------------------*/
BOOL CSecFileMgr::CreateRandomDirName(LPSTR szDirName)
{
    DWORD RandNum;
    LPSTR ptr = szDirName;
    static Counter;

    INET_ASSERT(szDirName);

    // Stolen from MakeRandomFileName.
    for (DWORD i = 0; i < DIR_NAME_SIZE; i++)
    {
        UINT digit;
        RandNum = (GetTickCount() * rand()) + Counter++;

        // 10 digits + 26 letters
        digit = RandNum % 36;
        *ptr++  = ( digit < 10 ) ? (CHAR)('0' + digit) : (CHAR)('A' + (digit - 10));
    }

    *ptr = '\0';

    return TRUE;
}


/*-----------------------------------------------------------------------------
CSecFileMgr::CreateAdditionalSubDirectories
Creates nAdditionalDirs random subdirectories, up to DEFAULT_MAX_DIRS.
----------------------------------------------------------------------------*/
BOOL CSecFileMgr::CreateAdditionalSubDirectories(DWORD nAdditionalDirs)
{
    DWORD nTotalDirs;
    DWORD nDirCount = _mmFile->GetDirCount();
    BOOL bSuccess = TRUE;

    INET_ASSERT(nDirCount <= DEFAULT_MAX_DIRS);

    // Don't create more than the max allowed dirs.
    nTotalDirs = nAdditionalDirs + nDirCount;
    INET_ASSERT(nTotalDirs <= DEFAULT_MAX_DIRS);

    // Create the dir and set the file count to 0.
    DWORD i = nDirCount;
    DWORD nTotalTries = 0;
    do
    {
        if (CreateSubDirectory(i))
        {
            _mmFile->SetFileCount(i, 0);
            _mmFile->IncrementDirCount();
            i++;
        }
        else
        {
            INET_ASSERT(FALSE);
            bSuccess = FALSE;
        }

        if (nTotalTries++ > 100)
        {
            bSuccess = FALSE;
            break;
        }

    } while (i < nTotalDirs);

    return bSuccess;
}


/*-----------------------------------------------------------------------------
CSecFileMgr::CreateSubDirectory(DWORD nIdx)
Actual creation of subdirectory.
----------------------------------------------------------------------------*/
BOOL CSecFileMgr::CreateSubDirectory(DWORD nIdx)
{
    CHAR szCacheDir[MAX_PATH];
    CHAR szSubDir[DIR_NAME_SIZE + 1];

    // Generate full path to random dir.
    CreateRandomDirName(szSubDir);
    DWORD cb = _mmFile->GetFullPathNameLen();

    memcpy(szCacheDir, _mmFile->GetFullPathName(), cb);
    memcpy(szCacheDir + cb, szSubDir, DIR_NAME_SIZE + 1);

    // Create the directory and add it to
    // the list of directories in the index.
    if (CreateDirectory(szCacheDir, NULL))
    {
        _mmFile->SetDirName(nIdx, szSubDir);

        // For cachevu must be hidden and system.
        // BUGBUG - sure it must be hidden?
        SetFileAttributes(szCacheDir, FILE_ATTRIBUTE_SYSTEM);

        if (!(GetOptions() & INTERNET_CACHE_CONTAINER_NODESKTOPINIT))
           EnableCacheVu(szCacheDir);

    }
    else
    {
        // Couldn't create the directory.
        INET_ASSERT(FALSE);
        return FALSE;
    }
    return TRUE;
}


/*-----------------------------------------------------------------------------
CSecFileMgr::FindMinFilesSubDir
Determines the cache subdirectory with the minimum file count for load balancing.
----------------------------------------------------------------------------*/
BOOL CSecFileMgr::FindMinFilesSubDir(DWORD& nMinFileDir, DWORD& nFiles)
{
    DWORD nDirCount = _mmFile->GetDirCount();

    if (nDirCount == 0 || nDirCount > DEFAULT_MAX_DIRS)
    {
        INET_ASSERT(FALSE);
        _mmFile->SetDirCount(0);
        CreateAdditionalSubDirectories(DEFAULT_DIR_TABLE_GROW_SIZE);
        nDirCount = _mmFile->GetDirCount();
    }

    nMinFileDir = 0;
    DWORD nMinFiles = _mmFile->GetFileCount(0);

    for (DWORD i = 1; i < nDirCount; i++)
    {
        if (_mmFile->GetFileCount(i) < nMinFiles)
        {
            nMinFiles = _mmFile->GetFileCount(i);
            nMinFileDir = i;
        }

    }
    nFiles = _mmFile->GetFileCount(nMinFileDir);
    return TRUE;
}

