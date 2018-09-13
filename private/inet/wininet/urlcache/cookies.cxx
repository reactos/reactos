/*-----------------------------------------------------------------------------
Copyright (c) 1996  Microsoft Corporation

Module Name:  cookies.cxx

Abstract:

  Cookie upgrade object implementation

  Upgrades cookies to new format by parsing existing cookies
  files and adding them to the newly created cookie cache index.

  Currently upgrades v3.2 to v4.0.


Author:
    Adriaan Canter (adriaanc) 01-Nov-1996.

Modification history:
    Ahsan Kabir (akabir) 25-Sep-1997 made a few minor alterations.
-------------------------------------------------------------------------------*/
#include <cache.hxx>

/*-----------------------------------------------------------------------------
    CCookieLoader::GetHKLMCookiesDirectory

  ----------------------------------------------------------------------------*/
DWORD CCookieLoader::GetHKLMCookiesDirectory(CHAR *szCookiesDirectory)
{
    DWORD dwError;
    REGISTRY_OBJ roCookiePath(HKEY_LOCAL_MACHINE, IE3_COOKIES_PATH_KEY);
    DWORD cbKeyLen = MAX_PATH;

    if ((dwError=roCookiePath.GetStatus())==ERROR_SUCCESS)
    {
        dwError = roCookiePath.GetValue(CACHE_DIRECTORY_VALUE, (LPBYTE)szCookiesDirectory,  &cbKeyLen);
    }

    return dwError;

}
/*-----------------------------------------------------------------------------
    CCookieLoader::ParseNextCookie

    Upgrades cookies from Cache Version 3.2 to Cache Version 4.0
  ----------------------------------------------------------------------------*/
CHAR* CCookieLoader::ParseNextCookie(CHAR* ptr, CHAR** ppszHash,
    FILETIME* pftExpire, FILETIME* pftLast)
{
    CHAR *pszName, *pszValue, *pszFlags,
         *pszExpireTimeLow, *pszExpireTimeHigh,
         *pszLastTimeHigh,  *pszLastTimeLow,
         *pszDelimiter, *pszNextCookie;

    __try
    {
        // Get the first token (cookie name).
        pszName           = StrTokEx(&ptr, "\n");
        if (!pszName)                               // Cookie name.
        {
            // Normal termination of the parse.
            pszNextCookie = 0;
            goto exit;
        }

        // Parse the rest of the cookie
        pszValue          = StrTokEx(&ptr, "\n");      // Cookie value.
        *ppszHash         = StrTokEx(&ptr, "\n");      // Combo of domain and path.
        pszFlags          = StrTokEx(&ptr, "\n");      // Cookie flags.
        pszExpireTimeLow  = StrTokEx(&ptr, "\n");      // Expire time.
        pszExpireTimeHigh = StrTokEx(&ptr, "\n");
        pszLastTimeLow    = StrTokEx(&ptr, "\n");      // Last Modified time.
        pszLastTimeHigh   = StrTokEx(&ptr, "\n");
        pszDelimiter      = StrTokEx(&ptr, "\n");      // Delimiter should be "*"


        // Abnormal termination of parse.
        if (!pszDelimiter || pszDelimiter[0] != '*')
        {
            TcpsvcsDbgAssert(FALSE);
            pszNextCookie = 0;
            goto exit;
        }

        // Set the times.
        pftExpire->dwLowDateTime  = atoi(pszExpireTimeLow);
        pftExpire->dwHighDateTime = atoi(pszExpireTimeHigh);
        pftLast->dwLowDateTime    = atoi(pszLastTimeLow);
        pftLast->dwHighDateTime   = atoi(pszLastTimeHigh);

        pszNextCookie = pszDelimiter+2;
    }

    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        TcpsvcsDbgAssert(FALSE);
        pszNextCookie = 0;
        goto exit;
    }
    ENDEXCEPT

exit:
    return pszNextCookie;

}


/*-----------------------------------------------------------------------------
    CCookieLoader::LoadCookies
  ----------------------------------------------------------------------------*/
DWORD CCookieLoader::LoadCookies(URL_CONTAINER *UrlContainer)
{
    HANDLE             hFind = INVALID_HANDLE_VALUE;
    HANDLE             hFile = INVALID_HANDLE_VALUE;

    FILETIME           ftExpire, ftLast;
    CHAR               szCookieFileName        [MAX_PATH],
                       szCookieFileNamePattern [MAX_PATH],
                       szOldMemMapFilePath     [MAX_PATH],
                       szHKLMCookiesPath       [MAX_PATH],
                       szCookieName            [MAX_PATH],
                       szHKLMCookieFileName    [MAX_PATH],
                       szHKCUCookieFileName    [MAX_PATH];

    CHAR               *pszHash, *ptr,
                       *pszCurrentCookie, *szBuffer;

    WIN32_FIND_DATA    FindData;
    BOOL               bReturn;
    DWORD              cbRead = 0, dwError = ERROR_SUCCESS;

    // Data for a single cookie should fit in 2 pages.
    BYTE bCacheEntryInfoBuffer[2 * PAGE_SIZE];
    LPCACHE_ENTRY_INFO pCacheEntryInfo;
    DWORD cbCacheEntryInfoBuffer;
    DWORD dwDIR_SEP_STRING = strlen(DIR_SEPARATOR_STRING);
    DWORD dwLen;
    REGISTRY_OBJ roCachePath(HKEY_CURRENT_USER, OLD_CACHE_KEY);
    DWORD cbKeyLen = MAX_PATH;

    szBuffer = 0;

    __try
    {

        if (!UrlContainer)
        {
            dwError = ERROR_INVALID_PARAMETER;
            goto exit;
        }

        TCHAR szSigKey[MAX_PATH];
        
        // Check to see if we are upgrading cookies
        // from local machine to per user.
        // If IE4's signature isn't present, then we'll guess that IE3 might have been.
        if (UrlContainer->IsPerUserItem()
            &&
            ((roCachePath.GetStatus()!=ERROR_SUCCESS)
             ||  
             (roCachePath.GetValue(CACHE_SIGNATURE_VALUE, (LPBYTE) szSigKey, &cbKeyLen)!=ERROR_SUCCESS)))
        {
            DWORD cb = MAX_PATH;
            CHAR szUserName[MAX_PATH];

            // We are converting cookies from HKLM to HKCU.
            // This is done by enumerating the user's cookies
            // files and copying them to the per-user diretory.
            // Once this is accomplished, cookie converting will
            // proceed normally.

            // Get the cookies directory as specified by HKLM.
            GetHKLMCookiesDirectory(szHKLMCookiesPath);

            // Get the current user name.
            GetUserName(szUserName, &cb);

            // szCookieFileNamePattern will look like c:\winnt\cookies\joeuser@*.txt
            dwLen = strlen(szHKLMCookiesPath) +
                    strlen(szUserName) +
                    dwDIR_SEP_STRING   +
                    7;        // strlen("@*.txt" + '\0';

            if( dwLen > MAX_PATH )
            {
                dwError =  ERROR_INSUFFICIENT_BUFFER;
                goto exit;
            }

            strcpy(szCookieFileNamePattern, szHKLMCookiesPath);
            strcat(szCookieFileNamePattern, DIR_SEPARATOR_STRING);
            strcat(szCookieFileNamePattern, szUserName);
            strcat(szCookieFileNamePattern, "@*.txt");

            // Enumerate the users cache files
            hFind = FindFirstFile(szCookieFileNamePattern, &FindData);
            if (hFind != INVALID_HANDLE_VALUE)
            {
                // One or more cookie files exist.
                do
                {
                    // Construct absolute path from HKLM to cookies file.
                    dwLen = strlen(szHKLMCookiesPath) +
                            strlen(FindData.cFileName) +
                            dwDIR_SEP_STRING +
                            1;
                    if( dwLen > MAX_PATH )
                    {
                        dwError =  ERROR_INSUFFICIENT_BUFFER;
                        goto exit;
                    }
                    strcpy(szHKLMCookieFileName, szHKLMCookiesPath);
                    strcat(szHKLMCookieFileName, DIR_SEPARATOR_STRING);
                    strcat(szHKLMCookieFileName, FindData.cFileName);

                    // Construct absolute path from HKCU to cookies file.
                    dwLen = strlen(UrlContainer->GetCachePath()) +
                            strlen(FindData.cFileName) +
                            1;
    
                    // We should rescue as many cookies as we can.
                    if( dwLen <= MAX_PATH )
                    {
                        strcpy(szHKCUCookieFileName, UrlContainer->GetCachePath());
                        strcat(szHKCUCookieFileName, FindData.cFileName);
                        // Move the file to the per-user directory.
                        CopyFile(szHKLMCookieFileName, szHKCUCookieFileName, TRUE);
                    }
                } while (FindNextFile(hFind, &FindData));

                // Close the Find handle.
                if (hFind != INVALID_HANDLE_VALUE)
                {
                    FindClose(hFind);
                    hFind = INVALID_HANDLE_VALUE;
                }
            } // Per-user upgrade.
        }

        // No per-user upgrade. szCookieFileNamePattern will look like
        // c:\winnt\cookies\*@*.txt or c:\winnt\profiles\joeuser\cookies\*@*.txt.
        dwLen = strlen(UrlContainer->GetCachePath()) + 8;        // strlen("*@*.txt" + '\0';

        if( dwLen > MAX_PATH )
        {
            dwError =  ERROR_INSUFFICIENT_BUFFER;
            goto exit;
        }
        strcpy(szCookieFileNamePattern, UrlContainer->GetCachePath());
        strcat(szCookieFileNamePattern, "*@*.txt");

        // Enumerate the cache files.
        hFind = FindFirstFile(szCookieFileNamePattern, &FindData);
        if (hFind == INVALID_HANDLE_VALUE)
        {
            // OK, No cookies files to upgrade.
            // BUGBUG - should we verify this?
            dwError = ERROR_SUCCESS;
            goto exit;
        }

        // One or more cookie files exist.
        do
        {
            // Construct absolute path to cookie file.
            dwLen = strlen(UrlContainer->GetCachePath()) +
                    strlen(FindData.cFileName) +
                    1;
            if( dwLen > MAX_PATH )
            {
                continue;
            }

            strcpy(szCookieFileName, UrlContainer->GetCachePath());
            strcat(szCookieFileName, FindData.cFileName);

            // Get the WIN32_FILE_ATTRIBUTE for the call to AddUrl
            // This wrapper works for Win95 and WinNT.

            // Open the cookie file.
            hFile = CreateFile(
                    szCookieFileName,       // Absolute path to cookies file.
                    GENERIC_READ,           // Read only.
                    FILE_SHARE_READ,        // Share.
                    0,                      // Security Attribute (ignored in W95).
                    OPEN_EXISTING,          // Fail if doesn't exist.
                    FILE_ATTRIBUTE_NORMAL,  // No special attributes.
                    0                       // Attribute template.
                    );

            // File handle must be valid.
            TcpsvcsDbgAssert(hFile != INVALID_HANDLE_VALUE);
            if (hFile != INVALID_HANDLE_VALUE)
            {
                // Allocate memory for cookie file contents.
                // BUGBUG - put an upper limit on this? ->
                // 300 cookies * 4k/cookie = 1200k plus sundry.
                szBuffer = new CHAR[FindData.nFileSizeLow + 1];
                if (!szBuffer)
                {
                    dwError = ERROR_NOT_ENOUGH_MEMORY;
                    goto exit;
                }

                // Read the file into memory.
                bReturn = ReadFile(hFile, szBuffer, FindData.nFileSizeLow, &cbRead, NULL);

                // ReadFile must be successful.
                TcpsvcsDbgAssert(bReturn);
                if (bReturn)
                {
                    // Null terminate buffer.
                    szBuffer[cbRead] = '\0';

                    // Parse each cookie out of the buffer.
                    pszCurrentCookie = szBuffer;
                    while (pszCurrentCookie = ParseNextCookie(pszCurrentCookie,
                        &pszHash, &ftExpire, &ftLast))
                    {
                        // Construct the cookie name from the following strings:
                        // FindData.cFileName is like "user@foobar.txt"
                        // pszHash is like "foobar.com/"
                        // szCookieName should then be "Cookie:user@foobar.com/"
                        strcpy(szCookieName, COOKIE_PREFIX);
                        strcat(szCookieName, FindData.cFileName);

                        ptr = strstr(szCookieName, "@");

                        // Downcase the username portion of the file.
                        CHAR* tptr = ptr;
                        while (*--tptr != ':')
                            *tptr = tolower(*tptr);

                        strcpy(ptr+1, pszHash);

                        // Check to see if an earlier version of this cookie
                        // has already been added to the cache index file.
                        BOOL fAddToCache = TRUE;
                        pCacheEntryInfo = (LPCACHE_ENTRY_INFO) bCacheEntryInfoBuffer;
                        cbCacheEntryInfoBuffer = sizeof(bCacheEntryInfoBuffer);

                        dwError = UrlContainer->GetUrlInfo(szCookieName, &pCacheEntryInfo,
                            &cbCacheEntryInfoBuffer, 0, 0, 0);

#ifndef UNIX
                        if (dwError == ERROR_SUCCESS
                            && CompareFileTime(pCacheEntryInfo->LastModifiedTime, ftLast) > 0)
#else
                        /* There is a problem with multiple cookies in a single
                         * cookie file. When adding the second cookie, we will
                         * try to delete the existing cookie (the first one that
                         * was added), and thus try to delete the cookie file
                         * itself. But, deletion of the cookie file will fail on
                         * Win32 because the file is already open above for
                         * parsing. On Unix, the deletion will succeed.
                         * So, the work-around is to not add the second cookie
                         * which is from the same site. The entry will remain
                         * in the cookie file anyway.
                         */
                        if (dwError == ERROR_SUCCESS)
#endif /* UNIX */
                            fAddToCache = FALSE;

                        if (fAddToCache)
                        {
                            // Either this cookie was not found in the index file or
                            // it was found and the last modified time on it is
                            // less than the currently parsed cookie. Proceed
                            // to add this cookie to the index file.

                            // Add it to the cookie container.
                            // BUGBUG - besides assert, what to do if this fails?

                            AddUrlArg Args;
                            memset(&Args, 0, sizeof(Args));
                            Args.pszUrl      = szCookieName;  // user@foobar.com
                            Args.pszFilePath = szCookieFileName; // c:\winnt\cookies\user@foobar.txt
                            Args.qwExpires   = FT2LL(ftExpire); // Expire time.
                            Args.qwLastMod   = FT2LL(ftLast); // Last modified time.
                            Args.dwEntryType |= COOKIE_CACHE_ENTRY;
                            dwError = UrlContainer->AddUrl(&Args);

                            TcpsvcsDbgAssert(dwError == ERROR_SUCCESS);
                        }

                    } // Successful next cookie field.

                } // Successful read.

                // Done with this cookie file. Delete the buffer.
                delete [] szBuffer;

                // And close the file
                CloseHandle(hFile);
                hFile = INVALID_HANDLE_VALUE;

            } // File handle is valid.

        } while (FindNextFile(hFind, &FindData));


        // No more cookie files or an error occured.
        if ((dwError = GetLastError()) != ERROR_NO_MORE_FILES)
            goto exit;

        // Normal termination.
        dwError = ERROR_SUCCESS;

    exit:

        // Close the file handle.
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);

        // Close the Find handle.
        if (hFind != INVALID_HANDLE_VALUE)
            FindClose(hFind);


    } // try

    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // Cleanup.
        delete [] szBuffer;

        if (hFind != INVALID_HANDLE_VALUE)
            FindClose(hFind);
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);

        TcpsvcsDbgAssert(FALSE);
        dwError = ERROR_EXCEPTION_IN_SERVICE;
        return dwError;
    }

    ENDEXCEPT
                
    return dwError;
}

