/*****************************************************************************
 *
 *    ftpurl.cpp - Creating, encoding, and decoding URLs
 *
 *****************************************************************************/

#include "priv.h"
#include "ftpurl.h"



///////////////////////////////////////////////////////////////////////
// URL Path Functions (Obsolete?)
///////////////////////////////////////////////////////////////////////

/*****************************************************************************\
    FUNCTION: UrlGetPath

    DESCRIPTION:
        pszUrlPath will NOT include the fragment if there is any.
\*****************************************************************************/
HRESULT UrlGetDifference(LPCTSTR pszBaseUrl, LPCTSTR pszSuperUrl, LPTSTR pszPathDiff, DWORD cchSize)
{
    HRESULT hr = E_INVALIDARG;

    pszPathDiff[0] = TEXT('\0');
    if ((lstrlen(pszBaseUrl) <= lstrlen(pszSuperUrl)) &&
        !StrCmpN(pszBaseUrl, pszSuperUrl, lstrlen(pszBaseUrl) - 1))
    {
        LPTSTR pszDelta = (LPTSTR) &pszSuperUrl[lstrlen(pszBaseUrl)];

        if (TEXT('/') == pszDelta[0])
            pszDelta = CharNext(pszDelta);  // Skip past this.
        
        StrCpyN(pszPathDiff, pszDelta, cchSize);
        hr = S_OK;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: UrlGetPath

    DESCRIPTION:
        pszUrlPath will NOT include the fragment if there is any.
\*****************************************************************************/
HRESULT UrlPathToFilePath(LPCTSTR pszSourceUrlPath, LPTSTR pszDestFilePath, DWORD cchSize)
{
    HRESULT hr = E_INVALIDARG;
    LPTSTR pszSeparator;

    // Is the source and destination the differnt?
    if (pszSourceUrlPath != pszDestFilePath)
    {
        // Yes, so we need to fill the dest before we start modifying it.
        StrCpyN(pszDestFilePath, pszSourceUrlPath, cchSize);
    }

    while (pszSeparator = StrChr(pszDestFilePath, TEXT('/')))
        pszSeparator[0] = TEXT('\\');

    // Some idiots use "Test%20File.txt" when "%20" is really in the file name.
//    ASSERT(!StrChr(pszDestFilePath, TEXT('%'))); // Assert it doesn't contain '%' or it probably has escaped url stuff.
    return hr;
}


/*****************************************************************************\
    FUNCTION: UrlGetPath

    DESCRIPTION:
        pszUrlPath will NOT include the fragment if there is any.
HRESULT UrlGetPath(LPCTSTR pszUrl, DWORD dwFlags, LPTSTR pszUrlPath, DWORD cchUrlPathSize)
{
    HRESULT hr = S_OK;
    URL_COMPONENTS urlComps = {sizeof(URL_COMPONENTS), NULL, 0, INTERNET_SCHEME_FTP, NULL, 0,
                                0, NULL, 0, NULL, 0, pszUrlPath, cchUrlPathSize, NULL, 0};

    hr = InternetCrackUrl(pszUrl, 0, ICU_DECODE, &urlComps) ? S_OK : E_FAIL;
    return hr;
}
\*****************************************************************************/


/*****************************************************************************\
    FUNCTION: UrlPathAppendSlash

    DESCRIPTION:
HRESULT UrlPathAppendSlash(LPTSTR pszUrlPath)
{
    LPTSTR pszEndOfPath = &pszUrlPath[lstrlen(pszUrlPath) - 1];

    // Is it missing a backslash?
    if ((pszEndOfPath >= pszUrlPath) && TEXT('/') != pszEndOfPath[0])
        StrCat(pszEndOfPath, SZ_URL_SLASH);    // Yes, so add it.

    return S_OK;
}
\*****************************************************************************/


/*****************************************************************************\
    FUNCTION: UrlPathRemoveSlashW

    DESCRIPTION:
\*****************************************************************************/
HRESULT UrlPathRemoveSlashW(LPWSTR pszUrlPath)
{
    LPWSTR pszEndOfPath = &pszUrlPath[lstrlenW(pszUrlPath) - 1];

    // Is it missing a backslash?
    if ((pszEndOfPath >= pszUrlPath) && (CH_URL_URL_SLASHW == pszEndOfPath[0]))
        pszEndOfPath[0] = 0;    // Yes, so remove it.

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: UrlPathRemoveSlashA

    DESCRIPTION:
\*****************************************************************************/
HRESULT UrlPathRemoveSlashA(LPSTR pszUrlPath)
{
    LPSTR pszEndOfPath = &pszUrlPath[lstrlenA(pszUrlPath) - 1];

    // Is it missing a backslash?
    if ((pszEndOfPath >= pszUrlPath) && (CH_URL_URL_SLASHA == pszEndOfPath[0]))
        pszEndOfPath[0] = 0;    // Yes, so remove it.

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: UrlPathRemoveFrontSlashW

    DESCRIPTION:
\*****************************************************************************/
HRESULT UrlPathRemoveFrontSlashW(LPWSTR pszUrlPath)
{
    if (pszUrlPath && (CH_URL_URL_SLASHW == pszUrlPath[0]))
        return CharReplaceWithStrW(pszUrlPath, lstrlen(pszUrlPath), 1, SZ_EMPTYW);
    else
        return S_OK;
}


/*****************************************************************************\
    FUNCTION: UrlPathRemoveFrontSlashA

    DESCRIPTION:
\*****************************************************************************/
HRESULT UrlPathRemoveFrontSlashA(LPSTR pszUrlPath)
{
    if (pszUrlPath && (CH_URL_URL_SLASHA == pszUrlPath[0]))
        return CharReplaceWithStrA(pszUrlPath, lstrlenA(pszUrlPath), 1, SZ_EMPTYA);
    else
        return S_OK;
}


/*****************************************************************************\
    FUNCTION: UrlPathToFilePathW

    DESCRIPTION:
\*****************************************************************************/
HRESULT UrlPathToFilePathW(LPWSTR pszPath)
{
    while (pszPath = StrChrW(pszPath, CH_URL_URL_SLASHW))
        pszPath[0] = CH_URL_SLASHW;

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: UrlPathToFilePathA

    DESCRIPTION:
\*****************************************************************************/
HRESULT UrlPathToFilePathA(LPSTR pszPath)
{
    while (pszPath = StrChrA(pszPath, CH_URL_URL_SLASHA))
        pszPath[0] = CH_URL_SLASHA;

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: FilePathToUrlPathW

    DESCRIPTION:
\*****************************************************************************/
HRESULT FilePathToUrlPathW(LPWSTR pszPath)
{
    while (pszPath = StrChrW(pszPath, CH_URL_SLASHW))
        pszPath[0] = CH_URL_URL_SLASHW;

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: FilePathToUrlPathA

    DESCRIPTION:
\*****************************************************************************/
HRESULT FilePathToUrlPathA(LPSTR pszPath)
{
    while (pszPath = StrChrA(pszPath, CH_URL_SLASHA))
        pszPath[0] = CH_URL_URL_SLASHA;

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: UrlPathAdd

    DESCRIPTION:
        ...
\*****************************************************************************/
HRESULT UrlPathAdd(LPTSTR pszUrl, DWORD cchUrlSize, LPCTSTR pszSegment)
{
    // If the segment starts with a slash, skip it.
    if (TEXT('/') == pszSegment[0])
        pszSegment = CharNext(pszSegment);

    StrCatBuff(pszUrl, pszSegment, cchUrlSize);

    return S_OK;
}


/*****************************************************************************\
    FUNCTION: UrlPathAppend

    DESCRIPTION:
        ...
HRESULT UrlPathAppend(LPTSTR pszUrl, DWORD cchUrlSize, LPCTSTR pszSegment)
{
    if (!EVAL(pszSegment))
        return E_INVALIDARG;

    UrlPathAppendSlash(pszUrl); // Make sure the base url ends in a '/'. Note it may be "ftp://".

    return UrlPathAdd(pszUrl, cchUrlSize, pszSegment);
}
\*****************************************************************************/


/*****************************************************************************\
     StrRetFromFtpPidl
\*****************************************************************************/
HRESULT StrRetFromFtpPidl(LPSTRRET pStrRet, DWORD shgno, LPCITEMIDLIST pidl)
{
    HRESULT hr = S_OK;
    TCHAR szUrl[MAX_URL_STRING];

    szUrl[0] = 0;
    hr = UrlCreateFromPidl(pidl, shgno, szUrl, ARRAYSIZE(szUrl), ICU_ESCAPE | ICU_USERNAME, TRUE);
    if (SUCCEEDED(hr))
    {
        // Will it fit into STRRET.cStr?
        if (lstrlen(szUrl) < ARRAYSIZE(pStrRet->cStr))
        {
            // Yes, so there it goes...
            pStrRet->uType = STRRET_CSTR;
            SHTCharToAnsi(szUrl, pStrRet->cStr, ARRAYSIZE(pStrRet->cStr));
        }
        else
        {
            // No, so we will need to allocate it
            LPWSTR pwzAllocedStr = NULL;
            UINT cch = lstrlen(szUrl) + 1;

            pwzAllocedStr = (LPWSTR) SHAlloc(CbFromCchW(cch));
            pStrRet->uType = STRRET_WSTR;
            pStrRet->pOleStr = pwzAllocedStr;
            if (pwzAllocedStr)
                SHTCharToUnicode(szUrl, pwzAllocedStr, cch);
            else
                hr = E_OUTOFMEMORY;
        }
    }

    return hr;
}


/*****************************************************************************\
     FUNCTION: UrlCreateFromFindData
 
    DESCRIPTION:
        This function will use a base url and a path from Find Data to create
    a fully qualified URL.

    PARAMETERS:
        pszBaseUrl - This needs to be Escaped.
        pwfd->cFileName - This needs to be UNEscaped.
        pszFullUrl - This will to be Escaped.
HRESULT UrlCreateFromFindData(LPCTSTR pszBaseUrl, const LPWIN32_FIND_DATA pwfd, LPTSTR pszFullUrl, DWORD cchFullUrlSize)
{
    TCHAR szBaseUrl[MAX_URL_STRING];
    HRESULT hr = E_FAIL;
    TCHAR szLastItemEscaped[MAX_PATH];
    TCHAR szLastItem[MAX_PATH];
    LPCTSTR pszLastItem = pwfd->cFileName;

    StrCpyN(szBaseUrl, pszBaseUrl, ARRAYSIZE(szBaseUrl));
    UrlPathAppendSlash(szBaseUrl);

    if (CH_URL_URL_SLASH == pszLastItem[0])
        pszLastItem++;  // Don't let pwfd->cFileName replace the existing Url Path 

    StrCpyN(szLastItem, pszLastItem, ARRAYSIZE(szLastItem));
    if (CH_URL_URL_SLASH == szLastItem[lstrlen(szLastItem)-1])
        szLastItem[lstrlen(szLastItem)-1] = 0;  // Remove the last slash because it's escaped

    if (EVAL(SUCCEEDED(EscapeString(szLastItem, szLastItemEscaped, ARRAYSIZE(szLastItemEscaped)))))
    {
        DWORD cchSize = cchFullUrlSize;

        if (EVAL(InternetCombineUrl(szBaseUrl, szLastItemEscaped, pszFullUrl, &cchSize, ICU_NO_ENCODE)))
        {
            hr = S_OK;

            // Is it a directory?
            if (FILE_ATTRIBUTE_DIRECTORY & pwfd->dwFileAttributes)
            {
                // Yes, so make sure it has a trailing slash to disambiguate it from
                // a file.
                UrlPathAppendSlash(pszFullUrl);
            }
        }
    }

    return hr;
}
\*****************************************************************************/


/*****************************************************************************\
    FUNCTION: GetLastSegment

    DESCRIPTION:
\*****************************************************************************/
LPTSTR GetLastSegment(LPCTSTR pszUrl)
{
    LPTSTR pszLastSeg = (LPTSTR) pszUrl;
    LPTSTR pszNextPossibleSeg;

    while (pszNextPossibleSeg = StrChr(pszLastSeg, TEXT('/')))
    {
        if (TEXT('\0') != CharNext(pszNextPossibleSeg))
            pszLastSeg = CharNext(pszNextPossibleSeg);
        else
            break;  // We are done.
    }

    if (TEXT('/') == pszLastSeg[0])
        pszLastSeg = CharNext(pszLastSeg);

    return pszLastSeg;
}


/*****************************************************************************\
    FUNCTION: UrlPathGetLastSegment

    DESCRIPTION:
HRESULT UrlPathGetLastSegment(LPCTSTR pszUrl, LPTSTR pszSegment, DWORD cchSegSize)
{
    HRESULT hr = S_FALSE;
    TCHAR szTemp[MAX_URL_STRING];
    LPTSTR pszLastSeg;

    StrCpyN(szTemp, pszUrl, ARRAYSIZE(szTemp));
    UrlPathRemoveSlash(szTemp);
    pszLastSeg = GetLastSegment(szTemp);
    StrCpyN(pszSegment, pszLastSeg, cchSegSize);

    return hr;
}
\*****************************************************************************/


/*****************************************************************************\
    FUNCTION: UrlPathRemoveLastSegment

    DESCRIPTION:
HRESULT UrlPathRemoveLastSegment(LPTSTR pszUrl)
{
    HRESULT hr = S_FALSE;
    LPTSTR pszLastSeg;

    UrlPathRemoveSlash(pszUrl);
    pszLastSeg = GetLastSegment(pszUrl);

    pszLastSeg[0] = TEXT('\0');

    return hr;
}
\*****************************************************************************/


/*****************************************************************************\
    FUNCTION: UrlRemoveDownloadType

    DESCRIPTION:
\*****************************************************************************/
HRESULT UrlRemoveDownloadType(LPTSTR pszUrlPath, BOOL * pfTypeSpecified, BOOL * pfType)
{
    HRESULT hr = S_FALSE;   // Specified? (Not yet)
    LPTSTR pszDownloadType;

    if (pfTypeSpecified)
        *pfTypeSpecified = TRUE;

    // Did the user specify a download type. szPath="Dir1/Dir2/file.txt;type=a".
    // TODO: Search Recursively because each segment in the path can have a 
    //       type.
    //       Example Url="ftp://server/Dir1;type=a/Dir2;type=a/File.txt;type=b
    if (pszDownloadType = StrStrI(pszUrlPath, SZ_FTP_URL_TYPE))
    {
        TCHAR chType;

        if (pfTypeSpecified)
            *pfTypeSpecified = TRUE;

        pszDownloadType[0] = TEXT('\0');   // Terminate pszUrlPath and remove this junk.
        chType = pszDownloadType[ARRAYSIZE(SZ_FTP_URL_TYPE) - 1];

        if (pfType)
        {
            if ((TEXT('a') == chType) || (TEXT('A') == chType))
                *pfType = TRUE;
            else
                *pfType = TRUE;
        }

        hr = S_OK;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: IsIPAddressStr

    DESCRIPTION:
    This function exists to detect an IP Address server name ("124.42.3.53") vs.
    a DNS domain name ("foobar", or "ftp.foobar.com").  I current accept more than
    4 segments because of 6-bit IP address. (BUGBUG: Valid?)

    TODO: To be thurough, I should probably made sure each segment is
          smaller than 256.
\*****************************************************************************/
BOOL IsIPAddressStr(LPTSTR pszServer)
{
    BOOL fIsIPAddressStr = TRUE;
    LPTSTR pszCurrentChar = pszServer;
    int nDigits = 0;
    int nSegments = 1;

    while (fIsIPAddressStr && pszCurrentChar[0])
    {
        if (TEXT('.') == pszCurrentChar[0])
        {
            nSegments++;
            if ((0 == nDigits) || (4 < nDigits))
                fIsIPAddressStr = FALSE;    // it started with a '.', ie, ".xxxxx"

            nDigits = 0;
        }

        nDigits++;
        if (nDigits > 4)
            fIsIPAddressStr = FALSE;    // To many digits, ie "12345.xxxx"

        if (((TEXT('0') > pszCurrentChar[0]) || (TEXT('9') < pszCurrentChar[0])) &&
            (TEXT('.') != pszCurrentChar[0]))
        {
            fIsIPAddressStr = FALSE;    // it's outside of the 0-9 range.
        }

        pszCurrentChar++;   // Next character.
    }

    if (nSegments != 4)
        fIsIPAddressStr = FALSE;    // Needs to have at least 4 segments ("1.2.3.4", "1.2.3.4.5")

    return fIsIPAddressStr;
}


/*****************************************************************************\
    FUNCTION: PidlGenerateSiteLookupStr

    DESCRIPTION:
    Sample Input: "ftp://user:password@ftp.server.com:69/Dir1/Dir2/File.txt"
    Sample Output: "ftp://user:password@ftp.server.com:69/"

    This is used to keep track of unique servers for CFtpSite.  A CFtpSite needs
    to be created for each unique site, which includes different users that are
    logged onto the same site because of rooted directories.
\*****************************************************************************/
HRESULT PidlGenerateSiteLookupStr(LPCITEMIDLIST pidl, LPTSTR pszLookupStr, DWORD cchSize)
{
    HRESULT hr = E_FAIL;

    // Some strange clients pass in non-Server IDs, like comdlg.
    if (FtpID_IsServerItemID(pidl))
    {
        LPITEMIDLIST pidlServer = FtpCloneServerID(pidl);
        
        if (pidlServer)
        {
            hr = UrlCreateFromPidlW(pidlServer, SHGDN_FORPARSING, pszLookupStr, cchSize, (ICU_ESCAPE | ICU_USERNAME), FALSE);
            ILFree(pidlServer);
        }
        else
            hr = E_OUTOFMEMORY;
    }

    return hr;
}

/*
HRESULT PidlReplaceUrlPath(LPCITEMIDLIST pidlIn, LPITEMIDLIST * ppidlOut, IMalloc * pm, LPCTSTR pszUrlPath)
{
    HRESULT hr;
    TCHAR szUrl[MAX_URL_STRING];

    *ppidlOut = NULL;
    hr = UrlCreateFromPidl(pidlIn, SHGDN_FORPARSING, szUrl, ARRAYSIZE(szUrl), ICU_ESCAPE, FALSE);
    if (SUCCEEDED(hr))
    {
        hr = UrlReplaceUrlPath(szUrl, ARRAYSIZE(szUrl), pszUrlPath);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlWithPath;

            hr = CreateFtpPidlFromUrlEx(szUrl, NULL, &pidlWithPath, pm, FALSE, TRUE, FtpPidl_IsDirectory(pidlIn, TRUE));
            if (EVAL(SUCCEEDED(hr)))
            {
                LPITEMIDLIST pidlServer = ILClone(pidlIn);

                if (pidlServer)
                {
                    if (_ILNext(pidlServer))
                        _ILNext(pidlServer)->mkid.cb = 0;     // We need the first original itemID because it contains the password information.

                    *ppidlOut = ILCombine(pidlServer, _ILNext(pidlWithPath));  // Original ServerID, plus the itemIDs from the new dest.
                    ILFree(pidlServer);
                }

                ILFree(pidlWithPath);
            }
            
        }
    }

    return hr;
}

// BUGBUG: pszUrlPath won't work with fragments
HRESULT UrlReplaceUrlPath(LPTSTR pszUrl, DWORD cchSize, LPCTSTR pszUrlPath)
{
    HRESULT hr = E_FAIL;
    URL_COMPONENTS urlComps = {0};
    TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
    TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];
    TCHAR szExtraInfo[MAX_PATH];    // Includes Port Number and download type (ASCII, Binary, Detect)
    INTERNET_PORT ipPortNum = INTERNET_DEFAULT_FTP_PORT;
    BOOL fResult;

    urlComps.dwStructSize = sizeof(urlComps);
    urlComps.lpszHostName = szServer;
    urlComps.dwHostNameLength = ARRAYSIZE(szServer);
    urlComps.lpszUrlPath = NULL;
    urlComps.dwUrlPathLength = 0;

    urlComps.lpszUserName = szUserName;
    urlComps.dwUserNameLength = ARRAYSIZE(szUserName);
    urlComps.lpszPassword = szPassword;
    urlComps.dwPasswordLength = ARRAYSIZE(szPassword);
    urlComps.lpszExtraInfo = szExtraInfo;
    urlComps.dwExtraInfoLength = ARRAYSIZE(szExtraInfo);

    fResult = InternetCrackUrl(pszUrl, 0, ICU_DECODE, &urlComps);
    if (fResult)
    {
        urlComps.dwStructSize = sizeof(urlComps);
        urlComps.lpszUrlPath = (LPTSTR)pszUrlPath;
        urlComps.dwUrlPathLength = (pszUrlPath ? lstrlen(pszUrlPath) : 0);

        fResult = InternetCreateUrl(&urlComps, (ICU_ESCAPE | ICU_USERNAME), pszUrl, &cchSize);
        if (fResult)
        {
            hr = S_OK;
        }
    }

    return hr;
}
*/


HRESULT PidlReplaceUserPassword(LPCITEMIDLIST pidlIn, LPITEMIDLIST * ppidlOut, IMalloc * pm, LPCTSTR pszUserName, LPCTSTR pszPassword)
{
    TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
    HRESULT hr = FtpPidl_GetServer(pidlIn, szServer, ARRAYSIZE(szServer));

    if (!pszUserName)   // May be NULL.
    {
        pszUserName = szUserName;
        EVAL(SUCCEEDED(FtpPidl_GetUserName(pidlIn, szUserName, ARRAYSIZE(szUserName))));
    }

    *ppidlOut = NULL;
    if (SUCCEEDED(hr))
    {
        LPITEMIDLIST pidlServer;

        hr = FtpServerID_Create(szServer, pszUserName, pszPassword, FtpServerID_GetTypeID(pidlIn), FtpServerID_GetPortNum(pidlIn), &pidlServer, pm, TRUE);
        if (SUCCEEDED(hr))
        {
            LPITEMIDLIST pidlFtpPath = _ILNext(pidlIn);

            *ppidlOut = ILCombine(pidlServer, pidlFtpPath);
            ILFree(pidlServer);
        }
    }

    return hr;
}


HRESULT UrlReplaceUserPassword(LPTSTR pszUrlPath, DWORD cchSize, LPCTSTR pszUserName, LPCTSTR pszPassword)
{
    HRESULT hr = E_FAIL;
    URL_COMPONENTS urlComps = {0};
    TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];
    TCHAR szUrlPath[MAX_URL_STRING];
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
    TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];
    TCHAR szExtraInfo[MAX_PATH];    // Includes Port Number and download type (ASCII, Binary, Detect)
    BOOL fResult;

    urlComps.dwStructSize = sizeof(urlComps);
    urlComps.lpszHostName = szServer;
    urlComps.dwHostNameLength = ARRAYSIZE(szServer);
    urlComps.lpszUrlPath = szUrlPath;
    urlComps.dwUrlPathLength = ARRAYSIZE(szUrlPath);
    urlComps.lpszExtraInfo = szExtraInfo;
    urlComps.dwExtraInfoLength = ARRAYSIZE(szExtraInfo);

    urlComps.lpszUserName = szUserName;
    urlComps.dwUserNameLength = ARRAYSIZE(szUserName);
    urlComps.lpszPassword = szPassword;
    urlComps.dwPasswordLength = ARRAYSIZE(szPassword);

    fResult = InternetCrackUrl(pszUrlPath, 0, ICU_DECODE, &urlComps);
    if (fResult)
    {
        urlComps.dwStructSize = sizeof(urlComps);
        urlComps.lpszHostName = szServer;

        urlComps.lpszUserName = (LPTSTR)(pszUserName ? pszUserName : szUserName);
        urlComps.dwUserNameLength = (pszUserName ? lstrlen(pszUserName) : lstrlen(szUserName));
        urlComps.lpszPassword = (LPTSTR)pszPassword;    // It may be valid for caller to pass NULL
        urlComps.dwPasswordLength = (pszPassword ? lstrlen(pszPassword) : 0);
        urlComps.lpszExtraInfo = szExtraInfo;
        urlComps.dwExtraInfoLength = ARRAYSIZE(szExtraInfo);

        fResult = InternetCreateUrl(&urlComps, (ICU_ESCAPE | ICU_USERNAME), pszUrlPath, &cchSize);
        if (fResult)
        {
            hr = S_OK;
        }
    }

    return hr;
}


// InternetCreateUrlW() will write into pszUrl but won't terminate the string.
// This is hard to detect because half the time the place where the terminator should
// go may coincidentally contain a terminator.  This code forces the bug to happen.
#define TEST_FOR_INTERNETCREATEURL_BUG      1
#define INTERNETCREATEURL_BUG_WORKAROUND     1

HRESULT UrlCreateEx(LPCTSTR pszServer, LPCTSTR pszUser, LPCTSTR pszPassword, LPCTSTR pszUrlPath, LPCTSTR pszFragment, INTERNET_PORT ipPortNum, LPCTSTR pszDownloadType, LPTSTR pszUrl, DWORD cchSize, DWORD dwFlags)
{
    HRESULT hr = E_FAIL;
    DWORD cchSizeCopy = cchSize;

#if DEBUG && TEST_FOR_INTERNETCREATEURL_BUG
    LPTSTR pszDebugStr = pszUrl;
    for (DWORD dwIndex = (cchSize - 2); dwIndex; dwIndex--)
    {
#ifndef INTERNETCREATEURL_BUG_WORKAROUND
        pszDebugStr[0] = -1;         // This will force a buffer w/o terminators.
#else // INTERNETCREATEURL_BUG_WORKAROUND
        pszDebugStr[0] = 0;         // This will work around the bug.
#endif // INTERNETCREATEURL_BUG_WORKAROUND
        pszDebugStr++;
    }
#endif // DEBUG && TEST_FOR_INTERNETCREATEURL_BUG

    URL_COMPONENTS urlComp = {sizeof(URL_COMPONENTS), NULL, 0, INTERNET_SCHEME_FTP, (LPTSTR) pszServer, 0,
                              ipPortNum, (LPTSTR) NULL_FOR_EMPTYSTR(pszUser), 0, (LPTSTR) NULL_FOR_EMPTYSTR(pszPassword), 0,
                              (LPTSTR) pszUrlPath, 0, (LPTSTR) NULL, 0};
    
    if (EVAL(InternetCreateUrl(&urlComp, dwFlags | ICU_USERNAME, pszUrl, &cchSizeCopy)))
    {
        hr = S_OK;
        if (pszFragment)
            StrCatBuff(pszUrl, pszFragment, cchSize);
    }

#if DEBUG && TEST_FOR_INTERNETCREATEURL_BUG
#ifdef INTERNETCREATEURL_BUG_WORKAROUND
    // Make sure we hit a terminator and not a -1, which should never happen in URL strings.
    for (pszDebugStr = pszUrl; pszDebugStr[0]; pszDebugStr++)
        ASSERT(-1 != pszDebugStr[0]);
#endif // INTERNETCREATEURL_BUG_WORKAROUND
#endif // DEBUG && TEST_FOR_INTERNETCREATEURL_BUG

    return hr;
}


HRESULT UrlCreate(LPCTSTR pszServer, LPCTSTR pszUser, LPCTSTR pszPassword, LPCTSTR pszUrlPath, LPCTSTR pszFragment, INTERNET_PORT ipPortNum, LPCTSTR pszDownloadType, LPTSTR pszUrl, DWORD cchSize)
{
    return UrlCreateEx(pszServer, pszUser, pszPassword, pszUrlPath, pszFragment, ipPortNum, pszDownloadType, pszUrl, cchSize, ICU_ESCAPE);
}


BOOL IsEmptyUrlPath(LPCTSTR pszUrlPath)
{
    BOOL fResult = FALSE;

    if (!pszUrlPath ||
        !pszUrlPath[0] ||
        (((TEXT('/') == pszUrlPath[0]) && (!pszUrlPath[1]))))
    {
        fResult = TRUE;
    }

    return fResult;
}



///////////////////////////////////////////////////////////////////////
// Wire Path Functions (UTF-8 or DBCS/MBCS)
///////////////////////////////////////////////////////////////////////
/*****************************************************************************\
    FUNCTION: WirePathAdd

    DESCRIPTION:
        ...
\*****************************************************************************/
HRESULT WirePathAdd(LPWIRESTR pwWirePath, DWORD cchUrlSize, LPCWIRESTR pwWireSegment)
{
    // If the segment starts with a slash, skip it.
    if ('/' == pwWireSegment[0])
        pwWireSegment = CharNextA(pwWireSegment);

    StrCatBuffA(pwWirePath, pwWireSegment, cchUrlSize);
    return S_OK;
}


/*****************************************************************************\
    FUNCTION: WirePathAppendSlash

    DESCRIPTION:
\*****************************************************************************/
HRESULT WirePathAppendSlash(LPWIRESTR pwWirePath, DWORD cchWirePathSize)
{
    HRESULT hr = E_FAIL;
    DWORD cchSize = lstrlenA(pwWirePath);

    // Is there enough room?
    if (cchSize < (cchWirePathSize - 1))
    {
        LPWIRESTR pwEndOfPath = &pwWirePath[cchSize - 1];

        // Is it missing a backslash?
        if ((pwEndOfPath >= pwWirePath) && '/' != pwEndOfPath[0])
            StrCatA(pwEndOfPath, SZ_URL_SLASHA);    // Yes, so add it.

        hr = S_OK;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: WirePathAppend

    DESCRIPTION:
        ...
\*****************************************************************************/
HRESULT WirePathAppend(LPWIRESTR pwWirePath, DWORD cchUrlSize, LPCWIRESTR pwWireSegment)
{
    if (!EVAL(pwWireSegment))
        return E_INVALIDARG;

    WirePathAppendSlash(pwWirePath, cchUrlSize); // Make sure the base url ends in a '/'. Note it may be "ftp://".
    return WirePathAdd(pwWirePath, cchUrlSize, pwWireSegment);
}


/*****************************************************************************\
    FUNCTION: UrlGetFirstPathSegment

    PARAMETERS:
    [IN]  pszFullPath - "Dir1\Dir2\Dir3"
    [OUT] szFirstItem - "Dir1"              [OPTIONAL]
    [OUT] szRemaining - "Dir2\Dir3"         [OPTIONAL]
\*****************************************************************************/
HRESULT WirePathGetFirstSegment(LPCWIRESTR pwFtpWirePath, LPWIRESTR wFirstItem, DWORD cchFirstItemSize, BOOL * pfWasFragSeparator, LPWIRESTR wRemaining, DWORD cchRemainingSize, BOOL * pfIsDir)
{
    HRESULT hr = S_OK;
    LPCWIRESTR pwSegEnding = StrChrA(pwFtpWirePath, CH_URL_URL_SLASH);

    if (pfIsDir)
        *pfIsDir = FALSE;

    ASSERT((CH_URL_URL_SLASHA != pwFtpWirePath[0]));    // You will probably not get what you want.
    if (pwSegEnding)
    {
        if (wFirstItem)
        {
            DWORD cchSize = (DWORD) (pwSegEnding - pwFtpWirePath + 1);
            StrCpyNA(wFirstItem, pwFtpWirePath, (cchSize <= cchFirstItemSize) ? cchSize : cchFirstItemSize);
        }

        if (pfIsDir && (CH_URL_URL_SLASHA == pwSegEnding[0]))
            *pfIsDir = TRUE;    // Tell them that it is a directory.

        if (wRemaining)
            StrCpyNA(wRemaining, CharNextA(pwSegEnding), cchRemainingSize);

        if (0 == pwSegEnding[1])
            hr = S_FALSE;   // End of the line.
    }
    else
    {
        if (wFirstItem)
            StrCpyNA(wFirstItem, pwFtpWirePath, cchFirstItemSize);    // pszFullPath contains only one segment 

        if (wRemaining)
            wRemaining[0] = 0;
        hr = S_FALSE;       // Indicate that there aren't any more directories left.
    }

    return hr;
}




///////////////////////////////////////////////////////////////////////
// Display Path Functions (Unicode)
///////////////////////////////////////////////////////////////////////
/*****************************************************************************\
    FUNCTION: DisplayPathAdd

    DESCRIPTION:
        ...
\*****************************************************************************/
HRESULT DisplayPathAdd(LPWSTR pwzUrl, DWORD cchUrlSize, LPCWSTR pwzSegment)
{
    // If the segment starts with a slash, skip it.
    if (L'/' == pwzSegment[0])
        pwzSegment = CharNext(pwzSegment);

    StrCatBuffW(pwzUrl, pwzSegment, cchUrlSize);
    return S_OK;
}


/*****************************************************************************\
    FUNCTION: DisplayPathAppendSlash

    DESCRIPTION:
\*****************************************************************************/
HRESULT DisplayPathAppendSlash(LPWSTR pwzDisplayPath, DWORD cchSize)
{
    DWORD cchCurrentSize = lstrlenW(pwzDisplayPath);
    HRESULT hr = CO_E_PATHTOOLONG;

    if (cchCurrentSize < (cchSize - 2))
    {
        LPWSTR pwzEndOfPath = &pwzDisplayPath[cchCurrentSize - 1];

        // Is it missing a backslash?
        if ((pwzEndOfPath >= pwzDisplayPath) && TEXT('/') != pwzEndOfPath[0])
            StrCatBuff(pwzEndOfPath, SZ_URL_SLASH, (cchCurrentSize + 2));    // Yes, so add it.

        hr = S_OK;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: DisplayPathAppend

    DESCRIPTION:
        ...
\*****************************************************************************/
HRESULT DisplayPathAppend(LPWSTR pwzDisplayPath, DWORD cchUrlSize, LPCWSTR pwzDisplaySegment)
{
    if (!EVAL(pwzDisplaySegment))
        return E_INVALIDARG;

    DisplayPathAppendSlash(pwzDisplayPath, cchUrlSize); // Make sure the base url ends in a '/'. Note it may be "ftp://".
    return DisplayPathAdd(pwzDisplayPath, cchUrlSize, pwzDisplaySegment);
}


/*****************************************************************************\
    FUNCTION: DisplayPathGetFirstSegment

    PARAMETERS:
    [IN]  pszFullPath - "Dir1\Dir2\Dir3"
    [OUT] szFirstItem - "Dir1"              [OPTIONAL]
    [OUT] szRemaining - "Dir2\Dir3"         [OPTIONAL]
\*****************************************************************************/
HRESULT DisplayPathGetFirstSegment(LPCWSTR pwzFullPath, LPWSTR pwzFirstItem, DWORD cchFirstItemSize, BOOL * pfWasFragSeparator, LPWSTR pwzRemaining, DWORD cchRemainingSize, BOOL * pfIsDir)
{
    HRESULT hr = S_OK;
    LPWSTR pwzSegEnding = StrChrW(pwzFullPath, CH_URL_URL_SLASH);

    if (pfIsDir)
        *pfIsDir = FALSE;

    // This will happen if the user enters an incorrect URL, like "ftp://wired//"
    //  ASSERT((CH_URL_URL_SLASHW != pwzFullPath[0]));    // You will probably not get what you want.
    if (pwzSegEnding)
    {
        if (pwzFirstItem)
        {
            DWORD cchSize = (DWORD) (pwzSegEnding - pwzFullPath + 1);
            StrCpyNW(pwzFirstItem, pwzFullPath, (cchSize <= cchFirstItemSize) ? cchSize : cchFirstItemSize);
        }

        if (pfIsDir && (CH_URL_URL_SLASHW == pwzSegEnding[0]))
            *pfIsDir = TRUE;    // Tell them that it is a directory.

        if (pwzRemaining)
            StrCpyNW(pwzRemaining, CharNextW(pwzSegEnding), cchRemainingSize);

        if (0 == pwzSegEnding[1])
            hr = S_FALSE;   // End of the line.
    }
    else
    {
        if (pwzFirstItem)
            StrCpyNW(pwzFirstItem, pwzFullPath, cchFirstItemSize);    // pszFullPath contains only one segment 

        if (pwzRemaining)
            pwzRemaining[0] = 0;
        hr = S_FALSE;       // Indicate that there aren't any more directories left.
    }

    return hr;
}


