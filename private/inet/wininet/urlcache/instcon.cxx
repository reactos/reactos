/*++
Copyright (c) 1997  Microsoft Corporation

Module Name:  instcon.cxx

Abstract:

    Installed container class derived from URL_CONTAINER
    
Author:
    Adriaan Canter (adriaanc) 04-10-97
    
--*/

#include <cache.hxx>

#define HTTP_OK "HTTP/1.0 200 OK\r\n\r\n"



/*--------------------- CInstCon Public Functions-----------------------------*/


/*-----------------------------------------------------------------------------
    CInstCon::CInstCon
-----------------------------------------------------------------------------*/
CInstCon::CInstCon(LPSTR CacheName, LPSTR VolumeLabel, LPSTR VolumeTitle,
                   LPSTR CachePath, LPSTR CachePrefix, LPSTR PrefixMap, 
                   LONGLONG CacheLimit, DWORD dwOptions)

    : URL_CONTAINER(CacheName, CachePath, CachePrefix, CacheLimit, dwOptions)
{
    if (_Status != ERROR_SUCCESS)
    {
        INET_ASSERT(FALSE);
        return;
    }
        
    _szVolumeLabel = NewString(VolumeLabel);
    _szVolumeTitle = NewString(VolumeTitle);
    _szPrefixMap   = NewString(PrefixMap);    
    
    if (!(_szVolumeLabel && _szVolumeTitle && _szPrefixMap))
    {
        INET_ASSERT(FALSE);
        _Status = ERROR_NOT_ENOUGH_MEMORY;
        return;
    }

    _cbPrefixMap = strlen(PrefixMap);
    _cbMaxFileSize = MAX_FILE_SIZE_TO_MIGRATE;
    _Status = ERROR_SUCCESS;
}

/*-----------------------------------------------------------------------------
    CInstCon::~CInstCon
-----------------------------------------------------------------------------*/
CInstCon::~CInstCon()
{
    delete _szPrefixMap;
    delete _szVolumeLabel;
    delete _szVolumeTitle;
}


/*------------------ URL_CONTAINER virtual overrides-------------------------*/

/*-----------------------------------------------------------------------------
    CInstCon::GetPrefixMap
-----------------------------------------------------------------------------*/
LPSTR CInstCon::GetPrefixMap()
{
    return _szPrefixMap;
}

/*-----------------------------------------------------------------------------
    CInstCon::GetVolumeLabel
-----------------------------------------------------------------------------*/
LPSTR CInstCon::GetVolumeLabel()
{
    return _szVolumeLabel;
}

/*-----------------------------------------------------------------------------
    CInstCon::GetVolumeTitle
-----------------------------------------------------------------------------*/
LPSTR CInstCon::GetVolumeTitle()
{
    return _szVolumeTitle;
}


/*-----------------------------------------------------------------------------
    CInstCon::AddUrl
-----------------------------------------------------------------------------*/
DWORD CInstCon::AddUrl (AddUrlArg* pArgs)
{
    pArgs->dwEntryType |= INSTALLED_CACHE_ENTRY;
    return URL_CONTAINER::AddUrl(pArgs);
}


/*-----------------------------------------------------------------------------
    CInstCon::RetrieveUrl
-----------------------------------------------------------------------------*/
DWORD CInstCon::RetrieveUrl(LPCSTR  UrlName, LPCACHE_ENTRY_INFO EntryInfo, 
                            LPDWORD EntryInfoSize, DWORD dwLookupFlags, 
                            DWORD dwRetrievalFlags)    
{
    DWORD dwError;
    BOOL fMustUnlock;

    INET_ASSERT(EntryInfo && EntryInfoSize);

    if (!LockContainer(&fMustUnlock))
    {
        dwError = GetLastError();
        goto exit;
    }

    dwError = GetEntry(UrlName, EntryInfo, EntryInfoSize, dwLookupFlags);

exit:
    if (fMustUnlock) UnlockContainer();
    return dwError;
}



/*-----------------------------------------------------------------------------
    CInstCon::GetUrlInfo
-----------------------------------------------------------------------------*/
DWORD CInstCon::GetUrlInfo(LPCSTR  szUrlName, LPCACHE_ENTRY_INFO pei, 
                           LPDWORD pcbei, DWORD dwLookupFlags, DWORD dwEntryFlags)    
{
    DWORD dwError;
    DWORD cbeiTemp = 0x256;
    BYTE bTemp[0x256];
    BOOL fMustUnlock;

    if (!LockContainer(&fMustUnlock))
    {
        dwError = GetLastError();
        goto exit;
    }

    // Zero buffer case.
    if (pei && pcbei)
    {
        dwError = GetEntry(szUrlName, pei, pcbei, dwLookupFlags);
        goto exit;
    }

    // Zero buffer case.
    if (dwEntryFlags & INTERNET_CACHE_FLAG_ENTRY_OR_MAPPING)
    {
        // Return success to indicate that a mapping exists. 
        // We wouldn't have gotten here otherwise.
        dwError = ERROR_SUCCESS;
    }
    else
    {
        // Otherwise, no flag passed in. Only return
        // success if the entry has been successfully found.
        dwError = GetEntry(szUrlName, (LPCACHE_ENTRY_INFO) bTemp, 
            &cbeiTemp, dwLookupFlags);
    }

exit:
    if (fMustUnlock) UnlockContainer();
    return dwError;
}
    

/*--------------------- CInstCon Private Functions-----------------------------*/



/*-----------------------------------------------------------------------------
    CInstCon::GetEntry
-----------------------------------------------------------------------------*/
DWORD CInstCon::GetEntry(LPCSTR  UrlName, LPCACHE_ENTRY_INFO EntryInfo, 
                         LPDWORD EntryInfoSize, DWORD dwLookupFlags)
{    
    INT cbOld, cbNew, cbDiff;
    DWORD cbOriginalInfoSize = *EntryInfoSize;
    DWORD dwError = ERROR_SUCCESS, dwCDStatus;

    DWORD cb;
    LPSTR ptr;
        

    // Try to get it from the CD.
    dwCDStatus = GetEntryFromCD(UrlName, EntryInfo, EntryInfoSize);

    switch(dwCDStatus)
    {
        // Couldn't find it on the CD.
        case ERROR_FILE_NOT_FOUND:
        case ERROR_PATH_NOT_FOUND:
 
            // Not on CD. Look in memory mapped file.
            dwError = URL_CONTAINER::RetrieveUrl(UrlName, (EntryInfo ? &EntryInfo : NULL), 
                                                 EntryInfoSize, dwLookupFlags,
                                                 RETRIEVE_WITH_CHECKS);
            break;
        
        // CD not in drive.        
        case ERROR_INVALID_DRIVE:
        case ERROR_NOT_READY:
        case ERROR_WRONG_DISK:
                
            dwError = ERROR_INTERNET_INSERT_CDROM;
            break;

        // Found it on the CD.
        case ERROR_SUCCESS:

            dwError = ERROR_SUCCESS;
            break;

        default:

            // Some other error.
            dwError = ERROR_INTERNET_INTERNAL_ERROR;
    }            

    return dwError;
}
    



/*-----------------------------------------------------------------------------
    CInstCon::MapUrlToAbsPath
-----------------------------------------------------------------------------*/
VOID CInstCon::MapUrlToAbsPath(LPSTR UrlName, 
                               LPSTR szAbsPath, LPDWORD pcbAbsPath)
{
    // Construct the absolute path to the file.
    memcpy(szAbsPath, _szPrefixMap, _cbPrefixMap + 1);

    LPSTR ptr = UrlName + _CachePrefixLen;
    DWORD cbSuffix = strlen(ptr);
 
    *pcbAbsPath = _cbPrefixMap + cbSuffix;
    memcpy(szAbsPath + _cbPrefixMap, ptr, cbSuffix + 1);
    
    // Convert all forward slashes to back slashes,
    // including any that were inadvertently placed
    // in the prefix map. This path might be returned 
    // to the caller.
    ptr = szAbsPath;
    while (*ptr++)
    {
#ifndef unix
        if (*ptr == '/')
            *ptr = '\\';
#else
        if (*ptr == '\\')
            *ptr = '/';
#endif /* unix */
    }

    // Unescape - final length may be less than
    // pcbAbsPath, but is always null terminated.
    UrlUnescapeInPlace(szAbsPath, NULL);
}


/*-----------------------------------------------------------------------------
    CInstCon::GetEntryFromCD
-----------------------------------------------------------------------------*/
DWORD CInstCon::GetEntryFromCD(LPCSTR UrlName, 
                               LPCACHE_ENTRY_INFO EntryInfo, 
                               LPDWORD EntryInfoSize)
{
    LPBYTE pb;
    CHAR szAbsPath[MAX_PATH], *ptr;
  
    DWORD cbUrl, cbSuffix, cbAbsPath, cbSizeRequired, cbExt, i,
          dwError = ERROR_SUCCESS;

    LONGLONG llZero        = (LONGLONG) 0;
    WIN32_FILE_ATTRIBUTE_DATA FileAttributes;

    CHAR szVolumeLabel[MAX_PATH];

    CHAR szVolRoot[4];
    memcpy(szVolRoot, _szPrefixMap, 2);
    memcpy(szVolRoot + 2, DIR_SEPARATOR_STRING, sizeof(DIR_SEPARATOR_STRING));
    // First check that the correct CD is inserted.
    if (GetVolumeInformation(szVolRoot, szVolumeLabel, MAX_PATH, 
                             NULL, NULL, NULL, NULL, 0))
    {
        if (strcmp(_szVolumeLabel, szVolumeLabel))
        {
            dwError = ERROR_WRONG_DISK;
            goto exit;
        }
    }
    else
    {
        dwError = ERROR_INVALID_DRIVE;
        goto exit;
    }

    // ---- Find the file and file info from the url -----


    cbUrl = strlen(UrlName);

    // Formulate path to file.
    MapUrlToAbsPath((LPSTR) UrlName, szAbsPath, &cbAbsPath);

    // Determine required CACHE_ENTRY_INFO buffer size.
    cbSizeRequired = sizeof(CACHE_ENTRY_INFO)
        + cbUrl + 1 + cbAbsPath + 1 + MAX_EXTENSION_LEN + 1 + sizeof(HTTP_OK);
    if (cbSizeRequired > *EntryInfoSize)
    {
        dwError = ERROR_INSUFFICIENT_BUFFER;
        goto exit;
    }
    
    // Get the file information. Shouldn't be a directory.
    dwError = GetFileSizeAndTimeByName(szAbsPath, &FileAttributes);
    if (dwError != ERROR_SUCCESS)
    {
        if (FileAttributes.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        {
            dwError = ERROR_FILE_NOT_FOUND;
        }
        goto exit;
    }


    // ---- Assemble the CACHE_ENTRY_INFO structure to return ----   
    
    
    // Urlname.
    pb = (LPBYTE) EntryInfo + sizeof(CACHE_ENTRY_INFO);
    memcpy(pb, UrlName, cbUrl + 1);
    EntryInfo->lpszSourceUrlName = (LPSTR) pb;
    
    // Filename.
    pb += cbUrl + 1;
    strcpy((LPSTR) pb, szAbsPath);
    EntryInfo->lpszLocalFileName = (LPSTR) pb;
    
    // Header.
    pb += cbAbsPath + 1;
    memcpy(pb, HTTP_OK, sizeof(HTTP_OK));
    EntryInfo->lpHeaderInfo = (LPSTR)pb;    
        
    // File extension
    cbExt = 0;
    ptr = (LPSTR) UrlName + cbUrl;
    for (i = 0; i < MAX_EXTENSION_LEN+1; i++)
    {
        if (*(--ptr) == '.')
        {            
            cbExt = i+1;
            break;
        }
    }
    if (cbExt)
    {
        pb += sizeof(HTTP_OK);
        memcpy(pb, ptr+1, cbExt + 1);
        EntryInfo->lpszFileExtension = (LPSTR) pb;    
    }
    else
        EntryInfo->lpszFileExtension = NULL;

    pb += cbExt +1;

    INET_ASSERT((DWORD) ((LPCACHE_ENTRY_INFO) pb - EntryInfo) < cbSizeRequired);
    
    // Version , type, count, hit rate, file size.
    EntryInfo->dwStructSize = URL_CACHE_VERSION_NUM;
    EntryInfo->CacheEntryType = INSTALLED_CACHE_ENTRY;
    EntryInfo->dwUseCount = 0;
    EntryInfo->dwHitRate = 0;
    EntryInfo->dwSizeHigh = FileAttributes.nFileSizeHigh;
    EntryInfo->dwSizeLow = FileAttributes.nFileSizeLow;    

    // Times: modified, expired, accessed, synced.
    EntryInfo->LastModifiedTime = FileAttributes.ftCreationTime;
    EntryInfo->ExpireTime = *(FILETIME*) &llZero;

    // BUGBUG - getcurrent time for both.
    EntryInfo->LastAccessTime = FileAttributes.ftLastAccessTime;
    EntryInfo->LastSyncTime = *(FILETIME*) &llZero;

    // Header size, file extension, exempt delta.
    EntryInfo->dwHeaderInfoSize = sizeof(HTTP_OK);
    EntryInfo->dwExemptDelta = 0;
    
    // Buffer consumed.
    *EntryInfoSize = cbSizeRequired - sizeof(CACHE_ENTRY_INFO);


exit:
    return dwError;
}

/*-----------------------------------------------------------------------------
    CInstCon::AddEntryToIndex
-----------------------------------------------------------------------------*/
DWORD CInstCon::AddEntryToIndex(LPCACHE_ENTRY_INFO EntryInfo, 
                                LPDWORD EntryInfoSize) 
{
    DWORD dwError;
    AddUrlArg args;
    CHAR szFileName[MAX_PATH];

    // Create a local file name for the hard disk.
    *szFileName = '\0';
    dwError = URL_CONTAINER::CreateUniqueFile(EntryInfo->lpszSourceUrlName, 
        NULL, EntryInfo->lpszFileExtension, szFileName, NULL);
    if (dwError != ERROR_SUCCESS)
        goto exit;

    // Copy the file from the CD.
    // CreateUniqueFile has already created a file of 0 bytes.
    if (!CopyFile(EntryInfo->lpszLocalFileName, szFileName, FALSE))
    {
        dwError = ERROR_INTERNET_INTERNAL_ERROR;
        goto exit;
    }

    // Add the entry to the index.

    // Url, filename, file size and extension.
    args.pszUrl = EntryInfo->lpszSourceUrlName;
    args.pszFilePath = szFileName;
    args.dwFileSize = EntryInfo->dwSizeLow;
    args.pszFileExt = EntryInfo->lpszFileExtension;
    args.dwEntryType = EntryInfo->CacheEntryType;    
    // Headers.
    args.pbHeaders = EntryInfo->lpHeaderInfo;
    args.cbHeaders = EntryInfo->dwHeaderInfoSize;

    // Times: last modified and expired.
    args.qwLastMod = FT2LL(EntryInfo->LastModifiedTime);
    args.qwExpires = FT2LL(EntryInfo->ExpireTime);

    // Redirect.    
    args.pszRedirect = NULL;

    args.fImage      = FALSE;
    
    // Add the url to the index.
    dwError = AddUrl(&args);
    if (dwError != ERROR_SUCCESS)
        goto exit;
                   
    // Retrieve the entry from the index. We do this because the EntryInfo
    // structure returned from the CD references the CD filename.
    // PERFPERF - we could optimize this by fixing up the EntryInfo
    // structure to reference the CD filename.
    dwError = URL_CONTAINER::RetrieveUrl(EntryInfo->lpszSourceUrlName, 
        (EntryInfo ? &EntryInfo : NULL), EntryInfoSize, NULL, RETRIEVE_WITH_CHECKS);

exit:
    return dwError;

}
