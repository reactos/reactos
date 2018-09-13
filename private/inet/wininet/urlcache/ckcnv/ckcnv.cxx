/*-----------------------------------------------------------------------------
Copyright (c) 1996  Microsoft Corporation

Module Name:  ckcnv.cxx

Abstract:
  Upgrades cookies to present urlcache format by enumerating cookie files in the
  cache cookies directory and creates cookie cache index entries in the format of 
  the current wininet.dll. 
    
Author:
    Adriaan Canter (adriaanc) 09-Jan-1997
        Created

    Adriaan Canter (adriaanc) 01-Feb-1997    
        Modified for per-user caches. The class CCookieLoader definition 
        can now be pasted into the urlcache build without re-definition
        and work correctly, as long as the HKLM and HKCU cache keys are
        not modified. BUGBUG - do this.

-----------------------------------------------------------------------------*/
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <wininet.h>
#include "cachedef.h"

#define INET_ASSERT(condition) Assert(condition)

/*-----------------------------------------------------------------------------
    class CCookieLoader

    Class used to perform cookie conversion
  ----------------------------------------------------------------------------*/
class CCookieLoader
{
private:
    DWORD GetHKLMCookiesDirectory(CHAR*);
    DWORD GetHKCUCookiesDirectory(CHAR*);
    CHAR* ParseNextCookie(CHAR*, CHAR**, FILETIME*, FILETIME*);
    
public:
    DWORD LoadCookies(BOOL);
};


// Debug assert code.
#if DBG
#define Assert(Predicate) \
    { \
        if (!(Predicate)) \
            AssertFailed( #Predicate, __FILE__, __LINE__, NULL ); \
    }

VOID
AssertFailed(
    LPSTR FailedAssertion,
    LPSTR FileName,
    DWORD LineNumber,
    LPSTR Message
    )
{

    printf("Assert @ %s \n", FailedAssertion );
    printf("Assert Filename, %s \n", FileName );
    printf("Line Num. = %ld.\n", LineNumber );
    printf("Message is %s\n", Message );

    DebugBreak();
}
#else
#define Assert(_x_)
#endif // DBG

/*-----------------------------------------------------------------------------
    CCookieLoader::GetHKLMCookiesDirectory
  ----------------------------------------------------------------------------*/
DWORD CCookieLoader::GetHKLMCookiesDirectory(CHAR *szCookiesDirectory)
{
    HKEY hKey;
    DWORD dwError, dwKeyType, cbKeyLen = MAX_PATH;

    CHAR szCookiesDirRegKey[MAX_PATH];
    strcpy(szCookiesDirRegKey, CACHE_KEY);
    strcat(szCookiesDirRegKey, "\\");
    strcat(szCookiesDirRegKey, CACHE_SPECIAL_PATHS_KEY);
    strcat(szCookiesDirRegKey, "\\");    
    strcat(szCookiesDirRegKey, COOKIE_PATH_KEY);

    if (dwError = RegOpenKeyEx(HKEY_LOCAL_MACHINE, 
        szCookiesDirRegKey, NULL, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
        goto exit;
    
    if (dwError = RegQueryValueEx(hKey, CACHE_DIRECTORY_VALUE, NULL, &dwKeyType, 
        (LPBYTE) szCookiesDirectory, &cbKeyLen) != ERROR_SUCCESS)
        goto exit;

exit:
    if (hKey != INVALID_HANDLE_VALUE)
        CloseHandle(hKey);

    return dwError;
}

/*-----------------------------------------------------------------------------
    CCookieLoader::GetHKCUCookiesDirectory
  ----------------------------------------------------------------------------*/
DWORD CCookieLoader::GetHKCUCookiesDirectory(CHAR *szCookiesDirectory)
{
    HKEY hKey;
    DWORD dwError, dwKeyType, cbKeyLen = MAX_PATH;
    
    CHAR szCookiesDirRegKey[MAX_PATH];
    strcpy(szCookiesDirRegKey, CACHE_KEY);
    strcat(szCookiesDirRegKey, "\\");
    strcat(szCookiesDirRegKey, COOKIE_PATH_KEY);

    if (dwError = RegOpenKeyEx(HKEY_CURRENT_USER,
        szCookiesDirRegKey, NULL, KEY_ALL_ACCESS, &hKey) != ERROR_SUCCESS)
        goto exit;
    
    if (dwError = RegQueryValueEx(hKey, CACHE_DIRECTORY_VALUE, NULL, &dwKeyType, 
        (LPBYTE) szCookiesDirectory, &cbKeyLen) != ERROR_SUCCESS)
        goto exit;

exit:
    if (hKey != INVALID_HANDLE_VALUE)
        CloseHandle(hKey);

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
            INET_ASSERT(FALSE);
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
        INET_ASSERT(FALSE);
        pszNextCookie = 0;
        goto exit;
    }

exit:
    return pszNextCookie;

}


/*-----------------------------------------------------------------------------
    CCookieLoader::LoadCookies
  ----------------------------------------------------------------------------*/
DWORD CCookieLoader::LoadCookies(BOOL fConvertToPerUser)
{
    HANDLE             hFind = INVALID_HANDLE_VALUE;
    HANDLE             hFile = INVALID_HANDLE_VALUE;

    FILETIME           ftExpire, ftLast;
    CHAR               szCookieFileName        [MAX_PATH],
                       szCookieFileNamePattern [MAX_PATH],
                       szHKLMCookiesPath       [MAX_PATH],
                       szHKCUCookiesPath       [MAX_PATH],
                       szCookieName            [MAX_PATH],    
                       szHKLMCookieFileName    [MAX_PATH],
                       szHKCUCookieFileName    [MAX_PATH];
                    
    CHAR               *pszHash, *ptr, *pszCookiesPath,
                       *pszCurrentCookie, *szBuffer;

    WIN32_FIND_DATA    FindData;
    BOOL               bReturn;
    DWORD              cbRead = 0, dwError = ERROR_SUCCESS;

    // Data for a single cookie should fit in 2 pages.
    BYTE bCacheEntryInfoBuffer[2 * PAGE_SIZE];
    INTERNET_CACHE_ENTRY_INFO *pCacheEntryInfo;
    DWORD cbCacheEntryInfoBuffer;

    __try
    {   
        szBuffer = 0;

        // Check to see if we are upgrading cookies
        // from local machine to per user.
        if (fConvertToPerUser)
        {
            DWORD cb = MAX_PATH;
            CHAR szUserName[MAX_PATH];

            // We are converting cookies from HKLM to HKCU.
            // This is done by enumerating the user's cookies
            // files and copying them to the per-user diretory.            
            // Once this is accomplished, cookie converting will
            // proceed normally.

            // Get the cookies directory as specified by HKLM.
            if (dwError = GetHKLMCookiesDirectory(szHKLMCookiesPath) != ERROR_SUCCESS)
            {
                INET_ASSERT(FALSE);
                goto exit;
            }
            strcpy(szCookieFileNamePattern, szHKLMCookiesPath);

            // Get the cookies directory as specified by HKCU.
            if (dwError = GetHKCUCookiesDirectory(szHKCUCookiesPath) != ERROR_SUCCESS)
            {
                INET_ASSERT(FALSE);
                goto exit;
            }

            // Get the current user name.
            GetUserName(szUserName, &cb);

            // szCookieFileNamePattern will look like c:\winnt\cookies\joeuser@*.txt
            strcat(szCookieFileNamePattern, "\\");
            strcat(szCookieFileNamePattern, szUserName);
            strcat(szCookieFileNamePattern, "@*.txt");
        
            // Enumerate the users cache files        
            hFind = FindFirstFile(szCookieFileNamePattern, &FindData);
            if (hFind == INVALID_HANDLE_VALUE)
            {
                // OK, No cookie files to upgrade.
                dwError = ERROR_SUCCESS;
                goto exit;
            }    
        
            // One or more cookie files exist.
            do
            {
                // Construct absolute path from HKLM to cookies file.
                strcpy(szHKLMCookieFileName, szHKLMCookiesPath);
                strcat(szHKLMCookieFileName, "\\");
                strcat(szHKLMCookieFileName, FindData.cFileName);
                
                // Construct absolute path from HKCU to cookies file.
                strcpy(szHKCUCookieFileName, szHKCUCookiesPath);
                strcat(szHKCUCookieFileName, "\\");
                strcat(szHKCUCookieFileName, FindData.cFileName);
            
                // Copy the file to the per-user directory.
                CopyFile(szHKLMCookieFileName, szHKCUCookieFileName, TRUE);

            } while (FindNextFile(hFind, &FindData)); 
        
            // Close the Find handle.
            if (hFind != INVALID_HANDLE_VALUE)
            {
                FindClose(hFind);        
                hFind = INVALID_HANDLE_VALUE;
            }

        } // Per-user upgrade.
        else
        {
            // No per-user upgrade. szCookieFileNamePattern will look like
            // c:\winnt\cookies\*@*.txt or c:\winnt\profiles\joeuser\cookies\*@*.txt.
            GetHKLMCookiesDirectory(szHKLMCookiesPath);
            strcpy(szCookieFileNamePattern, szHKLMCookiesPath);
            strcat(szCookieFileNamePattern, "\\*@*.txt");
        }

        // We now have the appropriate cookie filename pattern, also need a copy
        // of the cookies directory associated with the current user.
        pszCookiesPath = (fConvertToPerUser ? szHKCUCookiesPath : szHKLMCookiesPath);
        
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
            strcpy(szCookieFileName, pszCookiesPath);
            strcat(szCookieFileName, "\\");
            strcat(szCookieFileName, FindData.cFileName);
            
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
                INET_ASSERT(bReturn);
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
                        strcpy(ptr+1, pszHash);

                        // Check to see if an earlier version of this cookie
                        // has already been added to the cache index file.
                        BOOL fAddToCache = TRUE;
                        pCacheEntryInfo = (INTERNET_CACHE_ENTRY_INFO*) bCacheEntryInfoBuffer;
                        cbCacheEntryInfoBuffer = sizeof(bCacheEntryInfoBuffer);

                        dwError = GetUrlCacheEntryInfo(szCookieName, pCacheEntryInfo, 
                            &cbCacheEntryInfoBuffer);

                        if (dwError == ERROR_SUCCESS 
                            && CompareFileTime(&pCacheEntryInfo->LastModifiedTime, &ftLast) > 0)
                            fAddToCache = FALSE;

                        if (fAddToCache)
                        {
                            // Either this cookie was not found in the index file or 
                            // it was found and the last modified time on it is 
                            // less than the currently parsed cookie. Proceed
                            // to add this cookie to the index file.
                            BOOL bCommit;
                            bCommit = CommitUrlCacheEntry(
                                szCookieName,           // cookie:user@foobar.com.
                                szCookieFileName,       // c:\winnt\cookies\user@foobar.txt.
                                ftExpire,               // Expire time.
                                ftLast,                 // Last modified time.
                                0,                      // CacheEntryType.
                                0,                      // HeaderInfo.
                                0,                      // HeaderSize.
                                0,                      // FileExtension.
                                0);                     // Reserved.

                            INET_ASSERT(bCommit);
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

        return dwError;

    } // try

    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        // Cleanup.
        delete [] szBuffer;

        if (hFind != INVALID_HANDLE_VALUE)
            FindClose(hFind);
        if (hFile != INVALID_HANDLE_VALUE)
            CloseHandle(hFile);
                
        INET_ASSERT(FALSE);
        dwError = ERROR_EXCEPTION_IN_SERVICE;
        return dwError;
    }

}


int WINAPI WinMain(HINSTANCE hInstance,	HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow)
{
    DWORD dwError;
    CHAR szFilename[MAX_PATH];
    CCookieLoader cc;

    __try
    {
    
        // Convert cookies.
        dwError = cc.LoadCookies(FALSE);

        // See if we're supposed to delete this
        // executable after the user reboots.
        if (!_strnicmp(lpCmdLine, "/D", sizeof("/D")))
        {

            // Got this filename?
            if (GetModuleFileName(NULL, szFilename, MAX_PATH))
            {
                OSVERSIONINFO osVersionInfo;
                osVersionInfo.dwOSVersionInfoSize = sizeof(OSVERSIONINFO);
        
                if (GetVersionEx(&osVersionInfo))
                {
                    // Two different methods of deleting this file
                    // depending on the platform ID.
                    if (osVersionInfo.dwPlatformId == VER_PLATFORM_WIN32_NT)
                    {
                        // Platform is Windows NT.
                        MoveFileEx(szFilename, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
                    }
                    else
                    {
                        // Platform is Windows 95
                        WriteProfileSection("NUL", szFilename);
                    }

                }
            }
        }
    }
    

    __except(EXCEPTION_EXECUTE_HANDLER)
    {
        INET_ASSERT(FALSE);
        dwError = ERROR_EXTENDED_ERROR;
    }

    return (dwError == ERROR_SUCCESS ? 0 : 1);

}
