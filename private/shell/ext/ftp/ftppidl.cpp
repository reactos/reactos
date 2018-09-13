    /*****************************************************************************\
    ftppidl.cpp - Pointers to Item ID Lists

    This is the only file that knows the internal format of our IDLs.
\*****************************************************************************/

#include "priv.h"
#include "ftppidl.h"
#include "ftpurl.h"
#include "cookie.h"

#define NOT_INITIALIZED         10
DWORD g_fNoPasswordsInAddressBar = NOT_INITIALIZED;

#define SESSIONKEY      FILETIME

// Private FtpServerID Helpers
HRESULT FtpServerID_GetServer(LPCITEMIDLIST pidl, LPTSTR szServer, DWORD cchSize);
DWORD FtpItemID_GetTypeID(LPCITEMIDLIST pidl);

// v0 never went to customers but was used in NT5 before 1799       - Shipped in: Never.
// v1 This switch was to use password cookies for a security fix.   - Shipped in: Never.
// v2 this was done to not use the IDelegate's IMalloc for non-first ItemIDs  - Shipped in: Never (5/15/98)
// v3 add extra padding to ItemIDs so their dwType matches that of ServerIDs - Shipped in: IE5b1, IE5b2, NT5b2 (5/25/98)
// v4 add wzDisplayName to FtpItemID                                - Shipped in: IE5 RTM & NT5 b3  (11/16/98)

#define PIDL_VERSION_NUMBER_UPGRADE 3
#define PIDL_VERSION_NUMBER 4



#define     SIZE_ITEMID_SIZEFIELD        (sizeof(DWORD) + sizeof(WORD))
#define     SIZE_ITEMID_TERMINATOR       (sizeof(DWORD))


/****************************************************\
    IDType

    DESCRIPTION:
        These bits go into FTPIDLIST.dwIDType and describe
    what type of pidl it is AND which areas of the
    data structure have been verified by getting the
    data directly from the server.
\****************************************************/

#define IDTYPE_ISVALID           0x00000001    // Set if TYPE is valid
#define IDTYPE_SERVER            (0x00000002 | IDTYPE_ISVALID)    // Server
#define IDTYPE_DIR               (0x00000004 | IDTYPE_ISVALID)    // Folder/Dir
#define IDTYPE_FILE              (0x00000008 | IDTYPE_ISVALID)    // File
#define IDTYPE_FILEORDIR         (0x00000010 | IDTYPE_ISVALID)    // File or Dir.  Wasn't specified.
#define IDTYPE_FRAGMENT          (0x00000020 | IDTYPE_ISVALID)    // File Fragment (i.e. foobar.htm#SECTION_3)

// These are bits that indicate
// For Server ItemIDs
#define IDVALID_PORT_NUM         0x00000100     // Was the port number specified
#define IDVALID_USERNAME         0x00000200     // Was the login name specified
#define IDVALID_PASSWORD         0x00000400     // Was the password specified
#define IDVALID_DLTYPE           0x00000800     // Download Type is specified.
#define IDVALID_DL_ASCII         0x00001000     // Download as ASCII if set, otherwise, download as BINARY.
#define IDVALID_HIDE_PASSWORD    0x00002000     // The Password entry is invalid so use the sessionkey to look it up.

#define VALID_SERVER_BITS (IDTYPE_ISVALID|IDTYPE_SERVER|IDVALID_PORT_NUM|IDVALID_USERNAME|IDVALID_PASSWORD|IDVALID_DLTYPE|IDVALID_DL_ASCII|IDVALID_HIDE_PASSWORD)
#define IS_VALID_SERVER_ITEMID(pItemId) (!(pItemId & ~VALID_SERVER_BITS))

// For Dir/File ItemIDs
#define IDVALID_FILESIZE         0x00010000     // Did we get the file size from the server?
#define IDVALID_MOD_DATE         0x00020000     // Did we get the modification date from the server?

#define VALID_DIRORFILE_BITS (IDTYPE_ISVALID|IDTYPE_DIR|IDTYPE_FILE|IDTYPE_FILEORDIR|IDTYPE_FRAGMENT|IDVALID_FILESIZE|IDVALID_MOD_DATE)
#define IS_VALID_DIRORFILE_ITEMID(pItemId) (!(pItemId & (~VALID_DIRORFILE_BITS & ~IDTYPE_ISVALID)))


#define IS_FRAGMENT(pFtpIDList)       (IDTYPE_ISVALID != (IDTYPE_FRAGMENT & pFtpIDList->dwIDType))

///////////////////////////////////////////////////////////
// FTP Pidl Helper Functions 
///////////////////////////////////////////////////////////

/*****************************************************************************\
    FUNCTION: UrlGetAbstractPathFromPidl

    DESCRIPTION:
        pszUrlPath will be UNEscaped and in Wire Bytes.
\*****************************************************************************/
HRESULT UrlGetAbstractPathFromPidl(LPCITEMIDLIST pidl, BOOL fDirsOnly, BOOL fInWireBytes, void * pvPath, DWORD cchUrlPathSize)
{
    HRESULT hr = S_OK;
    LPWIRESTR pwWirePath = (LPWIRESTR) pvPath;
    LPWSTR pwzDisplayPath = (LPWSTR) pvPath;

    if (!EVAL(FtpPidl_IsValid(pidl)))
        return E_INVALIDARG;

    ASSERT(pvPath && (0 < cchUrlPathSize));
    ASSERT(IsValidPIDL(pidl));

    if (fInWireBytes)
    {
        pwWirePath[0] = '/';
        pwWirePath[1] = '\0'; // Make this path absolute.
    }
    else
    {
        pwzDisplayPath[0] = L'/';
        pwzDisplayPath[1] = L'\0'; // Make this path absolute.
    }

    if (!ILIsEmpty(pidl) && FtpID_IsServerItemID(pidl))       // If it's not a server, we are screwed.
        pidl = _ILNext(pidl);   // Skip past the Server Pidl.

    for (; !ILIsEmpty(pidl); pidl = _ILNext(pidl))
    {
        if (!fDirsOnly || FtpItemID_IsDirectory(pidl, TRUE) || !ILIsEmpty(_ILNext(pidl)))
        {
            if (!FtpItemID_IsFragment(pidl))
            {
                if (fInWireBytes)
                {
                    LPCWIRESTR pwWireName = FtpItemID_GetWireNameReference(pidl);

                    if (pwWireName)
                    {
                        // The caller should never need the URL Path escaped because
                        // that will happen when it's converted into an URL.
                        WirePathAppend(pwWirePath, cchUrlPathSize, pwWireName);
                    }
                }
                else
                {
                    LPCWSTR pwzDisplayName = FtpItemID_GetDisplayNameReference(pidl);

                    if (pwzDisplayName)
                    {
                        // The caller should never need the URL Path escaped because
                        // that will happen when it's converted into an URL.
                        DisplayPathAppend(pwzDisplayPath, cchUrlPathSize, pwzDisplayName);
                    }
                }
            }
        }

        if (SUCCEEDED(hr) && (FtpItemID_IsDirectory(pidl, FALSE) || (FtpItemID_GetCompatFlags(pidl) & COMPAT_WEBBASEDDIR)))
        {
            if (fInWireBytes)
                WirePathAppendSlash(pwWirePath, cchUrlPathSize); // Always make sure dirs end in '/'.
            else
                DisplayPathAppendSlash(pwzDisplayPath, cchUrlPathSize); // Always make sure dirs end in '/'.
        }
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: GetDisplayPathFromPidl

    DESCRIPTION:
        pwzDisplayPath will be UNEscaped and in display unicode.
\*****************************************************************************/
HRESULT GetDisplayPathFromPidl(LPCITEMIDLIST pidl, LPWSTR pwzDisplayPath, DWORD cchUrlPathSize, BOOL fDirsOnly)
{
    return UrlGetAbstractPathFromPidl(pidl, fDirsOnly, FALSE, (void *) pwzDisplayPath, cchUrlPathSize);
}


/*****************************************************************************\
    FUNCTION: GetWirePathFromPidl

    DESCRIPTION:
        pszUrlPath will be UNEscaped and in Wire Bytes.
\*****************************************************************************/
HRESULT GetWirePathFromPidl(LPCITEMIDLIST pidl, LPWIRESTR pwWirePath, DWORD cchUrlPathSize, BOOL fDirsOnly)
{
    return UrlGetAbstractPathFromPidl(pidl, fDirsOnly, TRUE, (void *) pwWirePath, cchUrlPathSize);
}


/*****************************************************************************\
    FUNCTION: UrlGetFileNameFromPidl

    DESCRIPTION:
        pszFileName will be filled with the file name if it exists.  If it doesn't,
    S_FALSE will be returned if the PIDL only points to a directory and not
    a file.
HRESULT UrlGetFileNameFromPidl(LPCITEMIDLIST pidl, LPTSTR pszFileName, DWORD cchSize)
{
    HRESULT hr = S_FALSE;

    if (!EVAL(FtpPidl_IsValid(pidl)))
        return E_INVALIDARG;

    ASSERT(pszFileName && (0 < cchSize));
    ASSERT(IsValidPIDL(pidl));
    pszFileName[0] = TEXT('\0');

    if (EVAL(FtpID_IsServerItemID(pidl)))       // If it's not a server, we are screwed.
        pidl = _ILNext(pidl);   // Skip past the Server Pidl.

    for (; !ILIsEmpty(pidl); pidl = _ILNext(pidl))
    {
        if (!FtpItemID_IsDirectory(pidl, FALSE))
        {
            // This isn't a directory, so the only time it's valid to not
            // be the last ItemID is if this is a file ItemID and the next
            // ItemID is a Fragment.
            ASSERT(ILIsEmpty(_ILNext(pidl)) || FtpItemID_IsFragment(_ILNext(pidl)));
            SHAnsiToTChar(FtpItemID_GetFileName(pidl), pszFileName, cchSize);
            hr = S_OK;
            break;
        }
    }

    return hr;
}
\*****************************************************************************/


#ifndef UNICODE
/*****************************************************************************\
    FUNCTION: UrlCreateFromPidlW

    DESCRIPTION:
        x.
\*****************************************************************************/
HRESULT UrlCreateFromPidlW(LPCITEMIDLIST pidl, DWORD shgno, LPWSTR pwzUrl, DWORD cchSize, DWORD dwFlags, BOOL fHidePassword)
{
    HRESULT hr;
    TCHAR szUrl[MAX_URL_STRING];

    hr = UrlCreateFromPidl(pidl, shgno, szUrl, ARRAYSIZE(szUrl), dwFlags, fHidePassword);
    if (SUCCEEDED(hr))
        SHTCharToUnicode(szUrl, pwzUrl, cchSize);

    return hr;
}

#else // UNICODE

/*****************************************************************************\
    FUNCTION: UrlCreateFromPidlA

    DESCRIPTION:
        x.
\*****************************************************************************/
HRESULT UrlCreateFromPidlA(LPCITEMIDLIST pidl, DWORD shgno, LPSTR pszUrl, DWORD cchSize, DWORD dwFlags, BOOL fHidePassword)
{
    HRESULT hr;
    TCHAR szUrl[MAX_URL_STRING];

    hr = UrlCreateFromPidl(pidl, shgno, szUrl, ARRAYSIZE(szUrl), dwFlags, fHidePassword);
    if (SUCCEEDED(hr))
        SHTCharToAnsi(szUrl, pszUrl, cchSize);

    return hr;
}

#endif // UNICODE


BOOL IncludePassword(void)
{
    if (NOT_INITIALIZED == g_fNoPasswordsInAddressBar)
        g_fNoPasswordsInAddressBar = !SHRegGetBoolUSValue(SZ_REGKEY_FTPFOLDER, SZ_REGVALUE_PASSWDSIN_ADDRBAR, FALSE, TRUE);

    return g_fNoPasswordsInAddressBar;
}


HRESULT ParseUrlCreateFromPidl(LPCITEMIDLIST pidl, LPTSTR pszUrl, DWORD cchSize, DWORD dwFlags, BOOL fHidePassword)
{
    HRESULT hr = S_OK;
    TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];
    TCHAR szUrlPath[MAX_URL_STRING];
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
    TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];
    TCHAR szFragment[MAX_PATH];
    TCHAR szDownloadType[MAX_PATH] = TEXT("");
    INTERNET_PORT ipPortNum = INTERNET_DEFAULT_FTP_PORT;

    if (ILIsEmpty(pidl))
    {
        ASSERT(0); // BUGBUG: Work around until we can figure out why CFtpFolder has ILIsEmpty(m_pidlHere).
        szServer[0] = szUrlPath[0] = szUserName[0] = szPassword[0] = TEXT('\0');
        hr = E_FAIL;
    }
    else
    {
        FtpPidl_GetServer(pidl, szServer, ARRAYSIZE(szServer));
        GetDisplayPathFromPidl(pidl, szUrlPath, ARRAYSIZE(szUrlPath), FALSE);
        FtpPidl_GetUserName(pidl, szUserName, ARRAYSIZE(szUserName));
        if (FAILED(FtpPidl_GetPassword(pidl, szPassword, ARRAYSIZE(szPassword), !fHidePassword)))
            szPassword[0] = 0;

        FtpPidl_GetFragment(pidl, szFragment, ARRAYSIZE(szPassword));
        FtpPidl_GetDownloadTypeStr(pidl, szDownloadType, ARRAYSIZE(szDownloadType));
        UrlPathAdd(szUrlPath, ARRAYSIZE(szUrlPath), szDownloadType);
        ipPortNum = FtpPidl_GetPortNum(pidl);
    }

    if (SUCCEEDED(hr))
    {
        TCHAR szUserNameEscaped[INTERNET_MAX_USER_NAME_LENGTH];

        szUserNameEscaped[0] = 0;
        if (szUserName[0])
            EscapeString(szUserName, szUserNameEscaped, ARRAYSIZE(szUserNameEscaped));

        hr = UrlCreateEx(szServer, NULL_FOR_EMPTYSTR(szUserNameEscaped), szPassword, szUrlPath, szFragment, ipPortNum, szDownloadType, pszUrl, cchSize, dwFlags);
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: GetFullPrettyName
    
    DESCRIPTION:
        The user wants a pretty name so these are the cases we need to worry
    about:
    URL:                                               Pretty Name:
    ----------------------------------                 ---------------------
    ftp://joe:psswd@serv/                              serv
    ftp://joe:psswd@serv/dir1/                         dir1 on serv
    ftp://joe:psswd@serv/dir1/dir2/                    dir2 on serv
    ftp://joe:psswd@serv/dir1/dir2/file.txt            file.txt on serv
\*****************************************************************************/
HRESULT GetFullPrettyName(LPCITEMIDLIST pidl, LPTSTR pszUrl, DWORD cchSize)
{
    HRESULT hr = S_OK;
    TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];

    FtpPidl_GetServer(pidl, szServer, ARRAYSIZE(szServer));
    // Is there anything after the ServerItemID?
    if (!ILIsEmpty(_ILNext(pidl)))
    {
        // Yes, so let's get the name of the last item and
        // make the string "<LastItemName> on <Server>".
        LPCWSTR pwzLastItem = FtpItemID_GetDisplayNameReference(ILFindLastID(pidl));
        LPTSTR pszStrArray[] = {szServer, (LPTSTR)pwzLastItem};
        TCHAR szTemplate[MAX_PATH];
        
        EVAL(LoadString(HINST_THISDLL, IDS_PRETTYNAMEFORMAT, szTemplate, ARRAYSIZE(szTemplate)));
        EVAL(FormatMessage((FORMAT_MESSAGE_FROM_STRING | FORMAT_MESSAGE_ARGUMENT_ARRAY), (LPVOID)szTemplate,
                            0, 0, pszUrl, cchSize, (va_list*)pszStrArray));
    }
    else
    {
        // No, so we are done.
        StrCpyN(pszUrl, szServer, cchSize);
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: UrlCreateFromPidl
    
    DESCRIPTION:
        Common worker that handles SHGDN_FORPARSING style GetDisplayNameOf's.

    Note! that since we do not support junctions (duh), we can
    safely walk down the pidl generating goop as we go, secure
    in the knowledge that we are in charge of every subpidl.

    _CHARSET_:  Since FTP filenames are always in the ANSI character
    set, by RFC 1738, we can return ANSI display names without loss
    of fidelity.  In a general folder implementation, we should be
    using cStr to return display names, so that the UNICODE
    version of the shell extension can handle UNICODE names.
\*****************************************************************************/
HRESULT UrlCreateFromPidl(LPCITEMIDLIST pidl, DWORD shgno, LPTSTR pszUrl, DWORD cchSize, DWORD dwFlags, BOOL fHidePassword)
{
    HRESULT hr = S_OK;

    pszUrl[0] = 0;
    if (!EVAL(pidl) ||
        !EVAL(IsValidPIDL(pidl)) ||
        !FtpPidl_IsValid(pidl) ||
        !FtpID_IsServerItemID(pidl) ||
        !EVAL(pszUrl && (0 < cchSize)))
    {
        return E_INVALIDARG;
    }

    if (shgno & SHGDN_INFOLDER)
    {
        // shgno & SHGDN_INFOLDER ?
        LPCITEMIDLIST pidlLast = ILGetLastID(pidl);

        if (EVAL(pidlLast && !ILIsEmpty(pidlLast)))
        {
            hr = FtpPidl_GetDisplayName(pidlLast, pszUrl, cchSize);

            // Do they want to reparse it later?  If they do and it's
            // a server, we need to give out the scheme also.
            // (SHGDN_INFOLDER) = "ServerName"
            // (SHGDN_INFOLDER|SHGDN_FORPARSING) = "ftp://ServerName/"
            if ((shgno & SHGDN_FORPARSING) &&
                (FtpID_IsServerItemID(pidlLast)))
            {
                // Yes, so we need to add the server name.
                TCHAR szServerName[MAX_PATH];

                StrCpyN(szServerName, pszUrl, ARRAYSIZE(szServerName));
                wnsprintf(pszUrl, cchSize, TEXT("ftp://%s/"), szServerName);
            }
        }
        else
            hr = E_FAIL;
    }
    else
    {
        // Assume they want the full URL.
        if (!EVAL((shgno & SHGDN_FORPARSING) || 
               (shgno & SHGDN_FORADDRESSBAR) ||
               (shgno == SHGDN_NORMAL)))
        {
            TraceMsg(TF_ALWAYS, "UrlCreateFromPidl() shgno=%#08lx and I dont know what to do with that.", shgno);
        }

        if ((shgno & SHGDN_FORPARSING) || (shgno & SHGDN_FORADDRESSBAR))
        {
            hr = ParseUrlCreateFromPidl(pidl, pszUrl, cchSize, dwFlags, fHidePassword);
        }
        else
            hr = GetFullPrettyName(pidl, pszUrl, cchSize);
    }

//    TraceMsg(TF_FTPURL_UTILS, "UrlCreateFromPidl() pszUrl=%ls, shgno=%#08lX", pszUrl, shgno);
    return hr;
}


/*****************************************************************************\
    FUNCTION: CreateFtpPidlFromDisplayPathHelper

    DESCRIPTION:
        The work done in CreateFtpPidlFromUrlPath requires a fair amount of
    stack space so we do most of the work in CreateFtpPidlFromDisplayPathHelper
    to prevent overflowing the stack.
\*****************************************************************************/
HRESULT CreateFtpPidlFromDisplayPathHelper(LPCWSTR pwzFullPath, CWireEncoding * pwe, ULONG *pcchEaten, LPITEMIDLIST * ppidl, BOOL fIsTypeKnown, BOOL fIsDir, LPITEMIDLIST * ppidlCurrentID, LPWSTR * ppwzRemaining)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl;
    WCHAR wzFirstItem[MAX_PATH];
    WIRECHAR wFirstWireItem[MAX_PATH];
    WCHAR wzRemaining[MAX_PATH];
    BOOL fIsCurrSegmentADir = FALSE;
    BOOL fIsCurrSegmentTypeKnown = fIsTypeKnown;
    BOOL fIsFragSeparator = FALSE;

    *ppwzRemaining = NULL;
    *ppidl = 0;

    if (pcchEaten)
        *pcchEaten = 0;     // The caller will parse the entire URL so we don't need to fill this in.

    if (L'/' == pwzFullPath[0])
        pwzFullPath = (LPWSTR) CharNextW(pwzFullPath);

    DisplayPathGetFirstSegment(pwzFullPath, wzFirstItem, ARRAYSIZE(wzFirstItem), NULL, wzRemaining, ARRAYSIZE(wzRemaining), &fIsCurrSegmentADir);
    // Is this the last segment?
    if (!wzRemaining[0])
    {
        // Yes, so if the caller knows the type of the last segment, use it now.
        if (fIsTypeKnown)
            fIsCurrSegmentADir = fIsDir;
    }
    else
    {
        // No, so we are assured that fIsDirCurrent is correct because it must have been followed
        // by a '/', or how could it be followed by another path segment?
        fIsCurrSegmentTypeKnown = TRUE;
        ASSERT(fIsCurrSegmentADir);
    }

    // NOTE: If the user entered "ftp://serv/Dir1/Dir2" fIsDir will be false for Dir2.
    //       It will be marked as ambigious. (TODO: Check for extension?)

    EVAL(SUCCEEDED(pwe->UnicodeToWireBytes(NULL, wzFirstItem, ((pwe && pwe->IsUTF8Supported()) ? WIREENC_USE_UTF8 : WIREENC_NONE), wFirstWireItem, ARRAYSIZE(wFirstWireItem))));
    hr = FtpItemID_CreateFake(wzFirstItem, wFirstWireItem, fIsCurrSegmentTypeKnown, !fIsCurrSegmentADir, FALSE, &pidl);
    ASSERT(IsValidPIDL(pidl));

    if (EVAL(SUCCEEDED(hr)))
    {
        if (wzRemaining[0])
        {
            Str_SetPtrW(ppwzRemaining, wzRemaining);
            *ppidlCurrentID = pidl;
        }
        else
            *ppidl = pidl;
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: CreateFtpPidlFromUrlPath

    DESCRIPTION:
        This function will be passed the 'Path' of the URL and will create
    each of the IDs for each path segment.  This will happen by creating an ID
    for the first path segment and then Combining that with the remaining
    IDs which are obtained by a recursive call.

    URL = "ftp://<UserName>:<Password>@<HostName>:<PortNum>/Dir1/Dir2/Dir3/file.txt[;Type=[a|b|d]]"
    Url Path = "Dir1/Dir2/Dir3/file.txt"

    pszFullPath - This URL will contain an URL Path (/Dir1/Dir2/MayBeFileOrDir).
    fIsTypeKnown - We can detect all directories w/o ambiguity because they end
                   end '/' except for the last directory.  fIsTypeKnown is used
                   if this information is known.  If TRUE, fIsDir will be used to
                   disambiguate the last item.  If FALSE, the last item will be marked
                   a directory if it doesn't have an extension.

    The incoming name is %-encoded, but if we see an illegal %-sequence,
    just leave the % alone.

    Note that we return E_FAIL when given an unparseable path,
    not E_INVALIDARG.
\*****************************************************************************/
HRESULT CreateFtpPidlFromDisplayPath(LPCWSTR pwzFullPath, CWireEncoding * pwe, ULONG *pcchEaten, LPITEMIDLIST * ppidl, BOOL fIsTypeKnown, BOOL fIsDir)
{
    HRESULT hr = E_FAIL;
    LPWSTR pwzRemaining = NULL;
    LPITEMIDLIST pidlCurrentID = NULL;

    hr = CreateFtpPidlFromDisplayPathHelper(pwzFullPath, pwe, pcchEaten, ppidl, fIsTypeKnown, fIsDir, &pidlCurrentID, &pwzRemaining);
    if (EVAL(SUCCEEDED(hr)) && pwzRemaining)
    {
        LPITEMIDLIST pidlSub;

        hr = CreateFtpPidlFromDisplayPath(pwzRemaining, pwe, pcchEaten, &pidlSub, fIsTypeKnown, fIsDir);
        if (EVAL(SUCCEEDED(hr)))
        {
            *ppidl = ILCombine(pidlCurrentID, pidlSub);
            hr = *ppidl ? S_OK : E_OUTOFMEMORY;
            ILFree(pidlSub);
        }

        ILFree(pidlCurrentID);
        Str_SetPtrW(&pwzRemaining, NULL);
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: CreateFtpPidlFromDisplayPathHelper

    DESCRIPTION:
        The work done in CreateFtpPidlFromUrlPath requires a fair amount of
    stack space so we do most of the work in CreateFtpPidlFromDisplayPathHelper
    to prevent overflowing the stack.
\*****************************************************************************/
HRESULT CreateFtpPidlFromFtpWirePathHelper(LPCWIRESTR pwFtpWirePath, CWireEncoding * pwe, ULONG *pcchEaten, LPITEMIDLIST * ppidl, BOOL fIsTypeKnown, BOOL fIsDir, LPITEMIDLIST * ppidlCurrentID, LPWIRESTR * ppwRemaining)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidl;
    WIRECHAR wFirstItem[MAX_PATH];
    WCHAR wzFirstItemDisplayName[MAX_PATH];
    WIRECHAR wRemaining[MAX_PATH];
    BOOL fIsCurrSegmentADir = FALSE;
    BOOL fIsCurrSegmentTypeKnown = fIsTypeKnown;
    BOOL fIsFragSeparator = FALSE;

    *ppwRemaining = NULL;
    *ppidl = 0;

    if (pcchEaten)
        *pcchEaten = 0;     // The caller will parse the entire URL so we don't need to fill this in.

    if ('/' == pwFtpWirePath[0])
        pwFtpWirePath = (LPWIRESTR) CharNextA(pwFtpWirePath);

    WirePathGetFirstSegment(pwFtpWirePath, wFirstItem, ARRAYSIZE(wFirstItem), NULL, wRemaining, ARRAYSIZE(wRemaining), &fIsCurrSegmentADir);
    // Is this the last segment?
    if (!wRemaining[0])
    {
        // Yes, so if the caller knows the type of the last segment, use it now.
        if (fIsTypeKnown)
            fIsCurrSegmentADir = fIsDir;
    }
    else
    {
        // No, so we are assured that fIsDirCurrent is correct because it must have been followed
        // by a '/', or how could it be followed by another path segment?
        fIsCurrSegmentTypeKnown = TRUE;
        ASSERT(fIsCurrSegmentADir);
    }

    // NOTE: If the user entered "ftp://serv/Dir1/Dir2" fIsDir will be false for Dir2.
    //       It will be marked as ambigious. (TODO: Check for extension?)
    EVAL(SUCCEEDED(pwe->WireBytesToUnicode(NULL, wFirstItem, WIREENC_IMPROVE_ACCURACY, wzFirstItemDisplayName, ARRAYSIZE(wzFirstItemDisplayName))));
    hr = FtpItemID_CreateFake(wzFirstItemDisplayName, wFirstItem, fIsCurrSegmentTypeKnown, !fIsCurrSegmentADir, FALSE, &pidl);
    ASSERT(IsValidPIDL(pidl));

    if (EVAL(SUCCEEDED(hr)))
    {
        if (wRemaining[0])
        {
            Str_SetPtrA(ppwRemaining, wRemaining);
            *ppidlCurrentID = pidl;
        }
        else
            *ppidl = pidl;
    }

    return hr;
}


HRESULT CreateFtpPidlFromFtpWirePath(LPCWIRESTR pwFtpWirePath, CWireEncoding * pwe, ULONG *pcchEaten, LPITEMIDLIST * ppidl, BOOL fIsTypeKnown, BOOL fIsDir)
{
    HRESULT hr = E_FAIL;
    LPWIRESTR pwRemaining = NULL;
    LPITEMIDLIST pidlCurrentID = NULL;

    *ppidl = NULL;
    if (!pwFtpWirePath[0] || (0 == StrCmpA(pwFtpWirePath, SZ_URL_SLASHA)))
        return S_OK;

    hr = CreateFtpPidlFromFtpWirePathHelper(pwFtpWirePath, pwe, pcchEaten, ppidl, fIsTypeKnown, fIsDir, &pidlCurrentID, &pwRemaining);
    if (EVAL(SUCCEEDED(hr)) && pwRemaining)
    {
        LPITEMIDLIST pidlSub;

        hr = CreateFtpPidlFromFtpWirePath(pwRemaining, pwe, pcchEaten, &pidlSub, fIsTypeKnown, fIsDir);
        if (EVAL(SUCCEEDED(hr)))
        {
            *ppidl = ILCombine(pidlCurrentID, pidlSub);
            hr = *ppidl ? S_OK : E_OUTOFMEMORY;
            ILFree(pidlSub);
        }

        ILFree(pidlCurrentID);
        Str_SetPtrA(&pwRemaining, NULL);
    }

    return hr;
}


HRESULT CreateFtpPidlFromUrlPathAndPidl(LPCITEMIDLIST pidl, CWireEncoding * pwe, LPCWIRESTR pwFtpWirePath, LPITEMIDLIST * ppidl)
{
    HRESULT hr = E_FAIL;
    LPITEMIDLIST pidlNew = ILClone(pidl);

    if (pidlNew)
    {
        LPITEMIDLIST pidlLast = (LPITEMIDLIST) ILGetLastID(pidlNew);

        while (!FtpID_IsServerItemID(pidlLast))
        {
            pidlLast->mkid.cb = 0;  // Remove this ID.
            pidlLast = (LPITEMIDLIST) ILGetLastID(pidlNew);
        }

        LPITEMIDLIST pidlUrlPath = NULL;
        hr = CreateFtpPidlFromFtpWirePath(pwFtpWirePath, pwe, NULL, &pidlUrlPath, TRUE, TRUE);
        if (EVAL(SUCCEEDED(hr)))
        {
            *ppidl = ILCombine(pidlNew, pidlUrlPath);
        }

        if (pidlLast)
            ILFree(pidlLast);

        if (pidlUrlPath)
            ILFree(pidlUrlPath);
    }

    return hr;
}


/*****************************************************************************\
    CreateFtpPidlFromUrl

    The incoming name is %-encoded, but if we see an illegal %-sequence,
    just leave the % alone.

    Note that we return E_FAIL when given an unparseable path,
    not E_INVALIDARG.
\*****************************************************************************/
HRESULT CreateFtpPidlFromUrl(LPCTSTR pszUrl, CWireEncoding * pwe, ULONG *pcchEaten, LPITEMIDLIST * ppidl, IMalloc * pm, BOOL fHidePassword)
{
    return CreateFtpPidlFromUrlEx(pszUrl, pwe, pcchEaten, ppidl, pm, fHidePassword, FALSE, FALSE);
}


/*****************************************************************************\
    FUNCTION: CreateFtpPidlFromUrlEx

    DESCRIPTION:
    pszUrl - This URL will contain an URL Path (/Dir1/Dir2/MayBeFileOrDir).
    fIsTypeKnown - We can detect all directories w/o ambiguity because they end
                   end '/' except for the last directory.  fIsTypeKnown is used
                   if this information is known.  If TRUE, fIsDir will be used to
                   disambiguate the last item.  If FALSE, the last item will be marked
                   a directory if it doesn't have an extension.

    The incoming name is %-encoded, but if we see an illegal %-sequence,
    just leave the % alone.

    Note that we return E_FAIL when given an unparseable path,
    not E_INVALIDARG.
\*****************************************************************************/
HRESULT CreateFtpPidlFromUrlEx(LPCTSTR pszUrl, CWireEncoding * pwe, ULONG *pcchEaten, LPITEMIDLIST * ppidl, IMalloc * pm, BOOL fHidePassword, BOOL fIsTypeKnown, BOOL fIsDir)
{
    URL_COMPONENTS urlComps = {0};
    HRESULT hr = E_FAIL;

    // URL = "ftp://<UserName>:<Password>@<HostName>:<PortNum>/Dir1/Dir2/Dir3/file.txt[;Type=[a|b|d]]"
    TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];
    TCHAR szUrlPath[MAX_URL_STRING];
    TCHAR szExtraInfo[MAX_PATH];    // Includes Port Number and download type (ASCII, Binary, Detect)
    TCHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];
    TCHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];

    *ppidl = 0;

    urlComps.dwStructSize = sizeof(urlComps);
    urlComps.lpszHostName = szServer;
    urlComps.dwHostNameLength = ARRAYSIZE(szServer);
    urlComps.lpszUrlPath = szUrlPath;
    urlComps.dwUrlPathLength = ARRAYSIZE(szUrlPath);

    urlComps.lpszUserName = szUserName;
    urlComps.dwUserNameLength = ARRAYSIZE(szUserName);
    urlComps.lpszPassword = szPassword;
    urlComps.dwPasswordLength = ARRAYSIZE(szPassword);
    urlComps.lpszExtraInfo = szExtraInfo;
    urlComps.dwExtraInfoLength = ARRAYSIZE(szExtraInfo);

    BOOL fResult = InternetCrackUrl(pszUrl, 0, ICU_DECODE, &urlComps);
    if (fResult && (INTERNET_SCHEME_FTP == urlComps.nScheme))
    {
        LPITEMIDLIST pidl;
        DWORD dwDownloadType = 0;   // Indicate that it hasn't yet been specified.
        BOOL fASCII;

        ASSERT(INTERNET_SCHEME_FTP == urlComps.nScheme);
        // NOTE:
        //          If the user is trying to give an NT UserName/DomainName pair, a bug will be encountered.
        //          Url in AddressBand="ftp://DomainName\UserName:Password@ServerName/"
        //          Url passed to us="ftp://DomainName/UserName:Password@ServerName/"
        //          We need to detect this case and fix it because this will cause "DomainName" to become
        //          the server name and the rest will become the UrlPath.
        // ASSERT(!StrChr(szUrlPath, TEXT(':')) && !StrChr(szUrlPath, TEXT('@')));

        if (S_OK == UrlRemoveDownloadType(szUrlPath, NULL, &fASCII))
        {
            if (fASCII)
                dwDownloadType = (IDVALID_DLTYPE | IDVALID_DL_ASCII);
            else
                dwDownloadType = IDVALID_DLTYPE;
        }

        if (!szServer[0])
        {
            TraceMsg(TF_FTPURL_UTILS, "CreateFtpPidlFromUrl() failed because szServer=%s", szServer);
            hr = E_FAIL;    // Bad URL so fail.
        }
        else
        {
            //TraceMsg(TF_FTPURL_UTILS, "CreateFtpPidlFromUrl() szServer=%s, szUrlPath=%s, szUserName=%s, szPassword=%s", szServer, szUrlPath, szUserName, szPassword);
            hr = FtpServerID_Create(szServer, szUserName, szPassword, dwDownloadType, urlComps.nPort, &pidl, pm, fHidePassword);
            if (EVAL(SUCCEEDED(hr)))
            {
                ASSERT(IsValidPIDL(pidl));
                if (szUrlPath[0] && StrCmp(szUrlPath, SZ_URL_SLASH))
                {
                    LPITEMIDLIST pidlSub;

                    hr = CreateFtpPidlFromDisplayPath(szUrlPath, pwe, pcchEaten, &pidlSub, fIsTypeKnown, fIsDir);
                    if (EVAL(SUCCEEDED(hr)))
                    {
                        // If the URL path ends in a slash, then it's safe to assume that web-based FTP
                        // is looking for a directory.  Otherwise, we choke during requests through Netscape
                        // proxies when the GET is redirected by the proxy to include the slash.
                        if (IsWebBasedFtpEnabled() && szUrlPath[lstrlen(szUrlPath)-1] == TEXT(CH_URL_URL_SLASHA))
                        {
                            LPCITEMIDLIST pidlLast = ILGetLastID(pidlSub);

                            if (pidlLast)
                                FtpItemID_SetCompatFlags(pidlLast, FtpItemID_GetCompatFlags(pidlLast) | COMPAT_WEBBASEDDIR);
                        }

                        *ppidl = ILCombine(pidl, pidlSub);
                        if (szExtraInfo[0])
                        {
                            LPITEMIDLIST pidlFragment;
                            WIRECHAR wFragment[MAX_PATH];

                            // The code page is just whatever the user is using but oh well, I don't 
                            // care about fragments.
                            SHUnicodeToAnsi(szExtraInfo, wFragment, ARRAYSIZE(wFragment));
                            // There is a fragment, so we need to add it.
                            hr = FtpItemID_CreateFake(szExtraInfo, wFragment, TRUE, FALSE, TRUE, &pidlFragment);
                            if (EVAL(SUCCEEDED(hr)))
                            {
                                LPITEMIDLIST pidlPrevious = *ppidl;

                                *ppidl = ILCombine(pidlPrevious, pidlFragment);
                                ILFree(pidlPrevious);
                                ILFree(pidlFragment);
                            }
                        }

                        hr = *ppidl ? S_OK : E_OUTOFMEMORY;
                        ILFree(pidlSub);
                    }
                    ILFree(pidl);
                }
                else
                    *ppidl = pidl;

                if (SUCCEEDED(hr))
                {
                    ASSERT(IsValidPIDL(*ppidl));
                    if (pcchEaten)
                        *pcchEaten = lstrlen(pszUrl);      // TODO: Someday we can do this recursively.
                }
            }
        }
    }
    else
        TraceMsg(TF_FTPURL_UTILS, "CreateFtpPidlFromUrl() failed InternetCrackUrl() because pszUrl=%s, fResult=%d, urlComps.nScheme=%d", pszUrl, fResult, urlComps.nScheme);

    //TraceMsg(TF_FTPURL_UTILS, "CreateFtpPidlFromUrl() is returning, hr=%#08lx", hr);
    return hr;
}


/*****************************************************************************\
     FUNCTION: Win32FindDataFromPidl
 
    DESCRIPTION:
        Fill in the WIN32_FIND_DATA data structure from the info in the pidl.
\*****************************************************************************/
HRESULT Win32FindDataFromPidl(LPCITEMIDLIST pidl, LPWIN32_FIND_DATAW pwfd, BOOL fFullPath, BOOL fInDisplayFormat)
{
    HRESULT hr = E_INVALIDARG;
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);

    ASSERT(pwfd);
    if (!EVAL(FtpPidl_IsValid(pidl)))
        return E_INVALIDARG;

    // I don't want to lie when I pass out File Size and Date info.
    if ((IDVALID_FILESIZE | IDVALID_MOD_DATE) & FtpItemID_GetTypeID(pidlLast))
    {
        pwfd->nFileSizeLow = FtpItemID_GetFileSizeLo(pidlLast);
        pwfd->nFileSizeHigh = FtpItemID_GetFileSizeHi(pidlLast);
        pwfd->dwFileAttributes = FtpItemID_GetAttributes(pidlLast);

        // See the notes in priv.h on how time works.
        pwfd->ftCreationTime = FtpPidl_GetFTPFileTime(pidlLast);
        pwfd->ftLastWriteTime = pwfd->ftCreationTime;
        pwfd->ftLastAccessTime = pwfd->ftCreationTime;

        if (fFullPath)
        {
            if (fInDisplayFormat)
                hr = GetDisplayPathFromPidl(pidl, pwfd->cFileName, ARRAYSIZE(pwfd->cFileName), FALSE);
            else
                hr = GetWirePathFromPidl(pidl, (LPWIRESTR)pwfd->cFileName, ARRAYSIZE(pwfd->cFileName), FALSE);
        }
        else
        { 
            hr = S_OK;
            if (fInDisplayFormat)
                StrCpyNW(pwfd->cFileName, FtpPidl_GetLastFileDisplayName(pidl), ARRAYSIZE(pwfd->cFileName));
            else
                StrCpyNA((LPWIRESTR)pwfd->cFileName, FtpPidl_GetLastItemWireName(pidl), ARRAYSIZE(pwfd->cFileName));
        }
    }

    return hr;
}






/****************************************************\
    FTP Server ItemIDs
\****************************************************/

/****************************************************\
    FTP PIDL Cooking functions
\****************************************************/

/*****************************************************************************\
    DATA STRUCTURE: FTPIDLIST

    DESCRIPTION:
        What our private IDLIST looks like for a file, a dir, or a fragment.

    The bytes sent to an ftp server or received from an FTP server are
    wire bytes (could be UTF-8 or DBCS/MBCS) encoded.  We also store
    a unicode version that has already been converted after trying to guess
    the code page.

    Note that the use of any TCHAR inside an IDLIST is COMPLETELY WRONG!
    IDLISTs can be saved in a file and reloaded later.  If it were saved
    by an ANSI version of the shell extension but loaded by a UNICODE
    version, things would turn ugly real fast.
\*****************************************************************************/

/*****************************************************************************\
    FTPSERVERIDLIST structure

    A typical full pidl looks like this:
    <Not Our ItemID> [Our ItemID]

    <The Internet>\[server,username,password,port#,downloadtype]\[subdir]\...\[file]

    The <The Internet> part is whatever the shell gives us in our
    CFtpFolder::_Initialize, telling us where in the namespace
    we are rooted.

    We are concerned only with the parts after the <The Internet> root,
    the offset to which is remembered in the CFtpFolder class
    in m_ibPidlRoot.  Ways of accessing various bits of
    information related to our full pidl are provided by our
    CFtpFolder implementation, qv.

    The first FTP IDList entry describes the server.  The remaining
    entries describe objects (files or folders) on the server.
\*****************************************************************************/

typedef struct tagFTPSERVERIDLIST
{
    DWORD dwIDType;                 // Server ItemID or Dir ItemID?  Which Bits are valid?
    DWORD dwVersion;                // version
    SESSIONKEY sessionKey;          // Session Key
    DWORD dwPasswordCookie;         // Password Cookie
    DWORD dwReserved1;              // for future use
    DWORD dwReserved2;              // for future use
    DWORD dwReserved3;              // for future use
    DWORD dwPortNumber;             // Port Number on server
    DWORD cchServerSize;            // StrLen of szServer
    CHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];        // Server
    DWORD cchUserNameSize;          // StrLen of szUserName
    CHAR szUserName[INTERNET_MAX_USER_NAME_LENGTH];      // User Name for Login
    DWORD cchPasswordSize;          // StrLen of szPassword
    CHAR szPassword[INTERNET_MAX_PASSWORD_LENGTH];      // Password for Login
} FTPSERVERIDLIST;

typedef UNALIGNED FTPSERVERIDLIST * LPFTPSERVERIDLIST;


LPFTPSERVERIDLIST FtpServerID_GetData(LPCITEMIDLIST pidl)
{
    LPFTPSERVERIDLIST pFtpServerItemId = (LPFTPSERVERIDLIST) ProtocolIdlInnerData(pidl);

    if (!FtpPidl_IsValid(pidl) || 
        !IS_VALID_SERVER_ITEMID(pFtpServerItemId->dwIDType)) // If any other bits are sit, it's invalid.
        pFtpServerItemId = NULL;

    return pFtpServerItemId;
}


LPFTPSERVERIDLIST FtpServerID_GetDataSafe(LPCITEMIDLIST pidl)
{
    LPFTPSERVERIDLIST pFtpServerItemId = NULL;
    
    if (EVAL(pidl) && !ILIsEmpty(pidl))
        pFtpServerItemId = (LPFTPSERVERIDLIST) ProtocolIdlInnerData(pidl);

    return pFtpServerItemId;
}


BOOL FtpID_IsServerItemID(LPCITEMIDLIST pidl)
{
    LPFTPSERVERIDLIST pFtpServerItemID = FtpServerID_GetDataSafe(pidl);
    BOOL fIsServerItemID = FALSE;

    if (pFtpServerItemID && IS_VALID_SERVER_ITEMID(pFtpServerItemID->dwIDType))
        fIsServerItemID = TRUE;

    return fIsServerItemID;
}


LPCITEMIDLIST FtpID_GetLastIDReferense(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlCurrent = pidl;
    LPCITEMIDLIST pidlNext = pidl;

    if (!pidl || ILIsEmpty(pidl))
        return pidl;

    for (; !ILIsEmpty(pidlNext); pidl = _ILNext(pidl))
    {
        pidlCurrent = pidlNext;
        pidlNext = _ILNext(pidlNext);
    }

    return pidlCurrent;
}


CCookieList * g_pCookieList = NULL;

CCookieList * GetCookieList(void)
{
    ENTERCRITICAL;
    if (!g_pCookieList)
        g_pCookieList = new CCookieList();
    ASSERT(g_pCookieList);
    LEAVECRITICAL;

    return g_pCookieList;
}

SESSIONKEY g_SessionKey = {-1, -1};

HRESULT PurgeSessionKey(void)
{
    GetSystemTimeAsFileTime(&g_SessionKey);

    return S_OK;
}

SESSIONKEY GetSessionKey(void)
{    
    if (-1 == g_SessionKey.dwHighDateTime)
        PurgeSessionKey();
    
    return g_SessionKey;
}

BOOL AreSessionKeysEqual(SESSIONKEY sk1, SESSIONKEY sk2)
{
    if ((sk1.dwHighDateTime == sk2.dwHighDateTime) &&
        (sk1.dwLowDateTime == sk2.dwLowDateTime))
    {
        return TRUE;
    }

    return FALSE;
}

// This is used in order to make sure Alpha machines don't get DWORD mis-aligned.
#define LENGTH_AFTER_ALIGN(nLen, nAlignSize)        (((nLen) % (nAlignSize)) ? ((nLen) + ((nAlignSize) - ((nLen) % (nAlignSize)))) : (nLen))

/****************************************************\
    FUNCTION: FtpServerID_Create

    DESCRIPTION:
        Create a Ftp Server ItemID and fill it in.
\****************************************************/
HRESULT FtpServerID_Create(LPCTSTR pszServer, LPCTSTR pszUserName, LPCTSTR pszPassword, 
                     DWORD dwFlags, INTERNET_PORT ipPortNum, LPITEMIDLIST * ppidl, IMalloc *pm, BOOL fHidePassword)
{
    HRESULT hr;
    DWORD cb;
    LPITEMIDLIST pidl;
    LPFTPSERVERIDLIST pFtpServerID = NULL;
    DWORD cchServerLen = lstrlen(pszServer);
    DWORD cchUserNameLen = lstrlen(pszUserName);
    DWORD cchPasswordLen = lstrlen(pszPassword);

    cchServerLen = LENGTH_AFTER_ALIGN(cchServerLen + 1, sizeof(DWORD));
    cchUserNameLen = LENGTH_AFTER_ALIGN(cchUserNameLen + 1, sizeof(DWORD));
    cchPasswordLen = LENGTH_AFTER_ALIGN(cchPasswordLen + 1, sizeof(DWORD));

    if (!(EVAL(ppidl) && pszServer[0]))
        return E_FAIL;

    // Set bits in dwFlags that are appropriate
    if (pszUserName[0])
        dwFlags |= IDVALID_USERNAME;

    if (pszPassword[0])
        dwFlags |= IDVALID_PASSWORD;

    // Find lenght of FTPSERVERIDLIST struct without the MAX_PATH strings
    cb = (sizeof(*pFtpServerID) - sizeof(pFtpServerID->szServer) - sizeof(pFtpServerID->szUserName) - sizeof(pFtpServerID->szPassword));

    // Add the size of the strings.
    cb += (cchServerLen + cchUserNameLen + cchPasswordLen);

    ASSERT(0 == (cb % sizeof(DWORD)));  // Make sure it's DWORD aligned for Alpha machines.

    pidl = (LPITEMIDLIST) pm->Alloc(cb);
    if (pidl)
    {
        LPSTR pszNext;

        pFtpServerID = FtpServerID_GetDataSafe(pidl);
        pszNext = pFtpServerID->szServer;

        ZeroMemory(pFtpServerID, cb);
        pFtpServerID->dwIDType = (dwFlags | IDTYPE_ISVALID | IDTYPE_SERVER | IDVALID_PORT_NUM);
        ASSERT(IS_VALID_SERVER_ITEMID(pFtpServerID->dwIDType));

        pFtpServerID->dwVersion = PIDL_VERSION_NUMBER;
        pFtpServerID->sessionKey = GetSessionKey();
        pFtpServerID->dwPasswordCookie = -1;
        pFtpServerID->dwPortNumber = ipPortNum;

        pFtpServerID->cchServerSize = cchServerLen;
        SHTCharToAnsi(pszServer, pszNext, pFtpServerID->cchServerSize);

        pszNext += cchServerLen; // Advance to cchUserNameSize
        *((LPDWORD) pszNext) = cchUserNameLen;  // Fill in cchUserNameSize
        pszNext = (LPSTR)(((BYTE *) pszNext) + sizeof(DWORD)); // Advance to szUserName
        SHTCharToAnsi(pszUserName, pszNext, cchUserNameLen);  // Fill in szUserName

        if (fHidePassword)
        {
            pFtpServerID->dwIDType |= IDVALID_HIDE_PASSWORD;
            if (EVAL(GetCookieList()))
                pFtpServerID->dwPasswordCookie = GetCookieList()->GetCookie(pszPassword);

            ASSERT(-1 != pFtpServerID->dwPasswordCookie);
            pszPassword = TEXT("");
        }

//        TraceMsg(TF_FTPURL_UTILS, "FtpServerID_Create(\"ftp://%s:%s@%s/\") dwIDType=%#80lx", pszUserName, pszPassword, pszServer, pFtpServerID->dwIDType);
        pszNext += cchUserNameLen; // Advance to cchPasswordLen
        *((LPDWORD) pszNext) = cchPasswordLen;  // Fill in cchPasswordLen
        pszNext = (LPSTR)(((BYTE *) pszNext) + sizeof(DWORD)); // Advance to szPassword
        SHTCharToAnsi(pszPassword, pszNext, cchPasswordLen);  // Fill in pszPassword
    }

    *ppidl = pidl;
    hr = pidl ? S_OK : E_OUTOFMEMORY;
    ASSERT(IsValidPIDL(*ppidl));

    return hr;
}


DWORD FtpServerID_GetTypeID(LPCITEMIDLIST pidl)
{
    LPFTPSERVERIDLIST pFtpServerID = FtpServerID_GetData(pidl);

    ASSERT(FtpID_IsServerItemID(pidl));
    if (EVAL(pFtpServerID) && 
        EVAL(FtpPidl_IsValid(pidl)))
        return pFtpServerID->dwIDType;

    return 0;
}


HRESULT FtpServerID_GetServer(LPCITEMIDLIST pidl, LPTSTR pszServer, DWORD cchSize)
{
    HRESULT hr = S_OK;
    LPFTPSERVERIDLIST pFtpServerID = FtpServerID_GetData(pidl);

    if (pFtpServerID)
        SHAnsiToTChar(pFtpServerID->szServer, pszServer, cchSize);
    else
        hr = E_FAIL;

    return hr;
}


BOOL FtpServerID_ServerStrCmp(LPCITEMIDLIST pidl, LPCTSTR pszServer)
{
    BOOL fMatch = FALSE;
    LPFTPSERVERIDLIST pFtpServerID = FtpServerID_GetData(pidl);
#ifdef UNICODE
    CHAR szServerAnsi[MAX_PATH];

    SHUnicodeToAnsi(pszServer, szServerAnsi, ARRAYSIZE(szServerAnsi));
#endif // UNICODE

    if (pFtpServerID)
    {
#ifdef UNICODE
        fMatch = (0 == StrCmpA(pFtpServerID->szServer, szServerAnsi));
#else // UNICODE
        fMatch = (0 == StrCmpA(pFtpServerID->szServer, pszServer));
#endif // UNICODE
    }

    return fMatch;
}


HRESULT FtpServerID_GetUserName(LPCITEMIDLIST pidl, LPTSTR pszUserName, DWORD cchSize)
{
    HRESULT hr = E_FAIL;
    LPFTPSERVERIDLIST pFtpServerID = FtpServerID_GetData(pidl);

    if (EVAL(pFtpServerID))
    {
        LPCSTR pszSourceUserName = pFtpServerID->szServer + pFtpServerID->cchServerSize + sizeof(DWORD);

        SHAnsiToTChar(pszSourceUserName, pszUserName, cchSize);
        hr = S_OK;
    }

    return hr;
}

HRESULT FtpServerID_GetPassword(LPCITEMIDLIST pidl, LPTSTR pszPassword, DWORD cchSize, BOOL fIncludingHiddenPassword)
{
    HRESULT hr = E_FAIL;
    LPFTPSERVERIDLIST pFtpServerID = FtpServerID_GetData(pidl);

    pszPassword[0] = 0;
    if (EVAL(pFtpServerID))
    {
        // Was the password hidden?
        if (fIncludingHiddenPassword &&
            (IDVALID_HIDE_PASSWORD & pFtpServerID->dwIDType))
        {
            // Yes, so get it out of the cookie jar (list)
            if (EVAL(GetCookieList()) &&
                AreSessionKeysEqual(pFtpServerID->sessionKey, GetSessionKey()))
            {
                hr = GetCookieList()->GetString(pFtpServerID->dwPasswordCookie, pszPassword, cchSize);
            }
        }
        else
        {
            // No, so what's in the pidl is the real password.
            BYTE * pvSizeOfUserName = (BYTE *) (pFtpServerID->szServer + pFtpServerID->cchServerSize);
            DWORD dwSizeOfUserName = *(DWORD *) pvSizeOfUserName;
            LPCSTR pszSourcePassword = (LPCSTR) (pvSizeOfUserName + dwSizeOfUserName + 2*sizeof(DWORD));

            SHAnsiToTChar(pszSourcePassword, pszPassword, cchSize);
            hr = S_OK;
        }
    }

    return hr;
}

INTERNET_PORT FtpServerID_GetPortNum(LPCITEMIDLIST pidl)
{
    LPFTPSERVERIDLIST pFtpServerID = FtpServerID_GetData(pidl);

    ASSERT(FtpID_IsServerItemID(pidl));
    if (EVAL(pFtpServerID))
        return (INTERNET_PORT)pFtpServerID->dwPortNumber;

    return INTERNET_DEFAULT_FTP_PORT;
}


HRESULT FtpServerID_SetHiddenPassword(LPITEMIDLIST pidl, LPCTSTR pszPassword)
{
    HRESULT hr = E_INVALIDARG;
    LPFTPSERVERIDLIST pFtpServerID = FtpServerID_GetData(pidl);

    ASSERT(FtpID_IsServerItemID(pidl));
    if (EVAL(pFtpServerID))
    {
        pFtpServerID->sessionKey = GetSessionKey();
        pFtpServerID->dwIDType |= IDVALID_HIDE_PASSWORD;
        if (EVAL(GetCookieList()))
            pFtpServerID->dwPasswordCookie = GetCookieList()->GetCookie(pszPassword);
        hr = S_OK;
    }

    return hr;
}


HRESULT FtpServerID_GetStrRet(LPCITEMIDLIST pidl, LPSTRRET lpName)
{
    LPFTPSERVERIDLIST pFtpServerID = FtpServerID_GetData(pidl);

    ASSERT(FtpID_IsServerItemID(pidl));
    if (pFtpServerID)
    {
        lpName->uType = STRRET_OFFSET;
        lpName->uOffset = (DWORD) (sizeof(FTPSERVERIDLIST) - sizeof(pFtpServerID->szServer) + (LPBYTE)pFtpServerID - (LPBYTE)pidl);
    }
    else
    {
        lpName->uType = STRRET_CSTR;
        lpName->cStr[0] = '\0';
    }

    return S_OK;
}





/****************************************************\
    FTP File/Dir ItemIDs
\****************************************************/

typedef struct tagFTPIDLIST
{
    DWORD dwIDType;         // Server ItemID or Dir ItemID?  Which Bits are valid?
    DWORD dwAttributes;     // What are the file/dir attributes
    ULARGE_INTEGER uliFileSize;
    FILETIME ftModified;    // Stored in Local Time Zone. (FTP Time)
    DWORD dwUNIXPermission; // UNIX CHMOD Permissions (0x00000777, 4=Read, 2=Write, 1=Exec, <Owner><Group><All>)
    DWORD dwCompatFlags;    // Any special case that needs to be handled
    WIRECHAR szWireName[MAX_PATH];          // Needs to go last.
    WCHAR wzDisplayName[MAX_PATH];  // Converted to unicode to be displayed in the UI.
} FTPIDLIST;

typedef UNALIGNED FTPIDLIST * LPFTPIDLIST;

HRESULT FtpItemID_Alloc(LPFTPIDLIST pfi, LPITEMIDLIST * ppidl);


typedef struct _FTPIDLIST_WITHHEADER
{
    USHORT  cb;             // size
    FTPIDLIST fidListData;
    USHORT  cbTerminator;   // size of next ID (Empty)
} FTPIDLIST_WITHHEADER;



LPFTPIDLIST FtpItemID_GetData(LPCITEMIDLIST pidl)
{
    BYTE * pbData = (BYTE *) pidl;

    pbData += SIZE_ITEMID_SIZEFIELD;      // Skip over the size.
    LPFTPIDLIST pFtpItemId = (LPFTPIDLIST) pbData;

    if (!EVAL(IS_VALID_DIRORFILE_ITEMID(pFtpItemId->dwIDType))) // If any other bits are sit, it's invalid.
        pFtpItemId = NULL;

    return pFtpItemId;
}

LPCWSTR FtpItemID_GetDisplayNameReference(LPCITEMIDLIST pidl)
{
    BYTE * pbData = (BYTE *) pidl;
    LPCWSTR pwzDisplayName = NULL;
    DWORD cbWireName;

    // Is the version OK?
//    if (PIDL_VERSION_NUMBER > FtpPidl_GetVersion(pidl))
//        return NULL;

    pbData += SIZE_ITEMID_SIZEFIELD;      // Skip over the size.
    LPFTPIDLIST pFtpItemId = (LPFTPIDLIST) pbData;

    cbWireName = LENGTH_AFTER_ALIGN((lstrlenA(pFtpItemId->szWireName) + 1), sizeof(DWORD));
    pwzDisplayName = (LPCWSTR) ((BYTE *)(&pFtpItemId->szWireName[0]) + cbWireName);

    if (!EVAL(IS_VALID_DIRORFILE_ITEMID(pFtpItemId->dwIDType))) // If any other bits are sit, it's invalid.
        pwzDisplayName = NULL;

    return pwzDisplayName;
}

DWORD FtpItemID_GetTypeID(LPCITEMIDLIST pidl)
{
    LPFTPIDLIST pFtpItemId = FtpItemID_GetData(pidl);

    return (pFtpItemId ? pFtpItemId->dwIDType : 0);
}


void FtpItemID_SetTypeID(LPITEMIDLIST pidl, DWORD dwNewTypeID)
{
    LPFTPIDLIST pFtpItemId = FtpItemID_GetData(pidl);
    ASSERT(pFtpItemId);
    pFtpItemId->dwIDType = dwNewTypeID;
}


/****************************************************\
    FUNCTION: FtpItemID_Alloc

    DESCRIPTION:
        We are passed a pointer to a FTPIDLIST data
    structure and our goal is to create a ItemID from
    it.  This mainly includes making it only big enough
    for the current string(s).
\****************************************************/
HRESULT FtpItemID_Alloc(LPFTPIDLIST pfi, LPITEMIDLIST * ppidl)
{
    HRESULT hr;
    WORD cbTotal;
    WORD cbDataFirst;
    WORD cbData;
    BYTE * pbMemory;
    DWORD cchSizeOfName = lstrlenA(pfi->szWireName);
    DWORD cchSizeOfDispName = lstrlenW(pfi->wzDisplayName);

    ASSERT(pfi && ppidl);

    // Find lenght of FTPIDLIST struct if the szName member only needed enought room
    // for the string, not the full MAX_PATH.
    // Size EQUALS: (Everything in the struct) - (the 2 full statusly sized strings) + (the 2 packed strings + alignment)
    cbDataFirst = (WORD)((sizeof(*pfi) - sizeof(pfi->szWireName) - sizeof(pfi->wzDisplayName)) + LENGTH_AFTER_ALIGN(cchSizeOfName + 1, sizeof(DWORD)) - sizeof(DWORD));
    cbData = cbDataFirst + (WORD) LENGTH_AFTER_ALIGN((cchSizeOfDispName + 1) * sizeof(WCHAR), sizeof(DWORD));

    ASSERT((cbData % sizeof(DWORD)) == 0);  // Verify it's DWORD aligned.
    cbTotal = (SIZE_ITEMID_SIZEFIELD + cbData + SIZE_ITEMID_TERMINATOR);

    pbMemory = (BYTE *) CoTaskMemAlloc(cbTotal);
    if (EVAL(pbMemory))
    {
        USHORT * pIDSize = (USHORT *)pbMemory;
        BYTE * pbData = (pbMemory + SIZE_ITEMID_SIZEFIELD);        // the Data starts at the second DWORD.
        USHORT * pIDTerminator = (USHORT *)(pbMemory + SIZE_ITEMID_SIZEFIELD + cbData);

        pIDSize[0] = (cbTotal - SIZE_ITEMID_TERMINATOR);      // Set the size of the ItemID (including the next ItemID as terminator)
        ASSERT(cbData <= sizeof(*pfi)); // Don't let me copy too much.
        CopyMemory(pbData, pfi, cbDataFirst);
        CopyMemory((pbData + cbDataFirst), &(pfi->wzDisplayName), ((cchSizeOfDispName + 1) * sizeof(WCHAR)));
        pIDTerminator[0] = 0;  // Terminate the next ID.

//        TraceMsg(TF_FTPURL_UTILS, "FtpItemID_Alloc(\"%ls\") dwIDType=%#08lx, dwAttributes=%#08lx", pfi->wzDisplayName, pfi->dwIDType, pfi->dwAttributes);
    }

    *ppidl = (LPITEMIDLIST) pbMemory;
    hr = pbMemory ? S_OK : E_OUTOFMEMORY;

    ASSERT(IsValidPIDL(*ppidl));
    ASSERT_POINTER_MATCHES_HRESULT(*ppidl, hr);

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpItemID_CreateReal

    DESCRIPTION:
        Cook up a pidl based on a WIN32_FIND_DATA.

    The cFileName field is itself MAX_PATH characters long,
    so its length cannot possibly exceed MAX_PATH...
\*****************************************************************************/
HRESULT FtpItemID_CreateReal(const LPFTP_FIND_DATA pwfd, LPCWSTR pwzDisplayName, LPITEMIDLIST * ppidl)
{
    HRESULT hr;
    FTPIDLIST fi = {0};

    // Fill in fi.
    fi.dwIDType = (IDTYPE_ISVALID | IDVALID_FILESIZE | IDVALID_MOD_DATE);
    fi.uliFileSize.LowPart = pwfd->nFileSizeLow;
    fi.uliFileSize.HighPart = pwfd->nFileSizeHigh;
    fi.ftModified = pwfd->ftLastWriteTime;
    fi.dwAttributes = pwfd->dwFileAttributes;
    fi.dwUNIXPermission = pwfd->dwReserved0;    // Set by WININET
    StrCpyNA(fi.szWireName, pwfd->cFileName, ARRAYSIZE(fi.szWireName));
    StrCpyN(fi.wzDisplayName, pwzDisplayName, ARRAYSIZE(fi.wzDisplayName));

    if (pwfd->dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
        fi.dwIDType |= IDTYPE_DIR;
    else
        fi.dwIDType |= IDTYPE_FILE;

    hr = FtpItemID_Alloc(&fi, ppidl);
    ASSERT(IsValidPIDL(*ppidl));

    return hr;
}


/****************************************************\
    FUNCTION: FtpItemID_CreateFake

    DESCRIPTION:
        Create a ItemID but we are only setting the
    name.  We don't know the true file attributes,
    file size, or modification date yet because
    we haven't touched the server yet.  If we did,
    we would use the returned WIN32_FIND_DATA struct
    to create the ItemID by using FtpItemID_CreateReal().
\****************************************************/
HRESULT FtpItemID_CreateFake(LPCWSTR pwzDisplayName, LPCWIRESTR pwWireName, BOOL fTypeKnown, BOOL fIsFile, BOOL fIsFragment, LPITEMIDLIST * ppidl)
{
    HRESULT hr;
    DWORD dwType = IDTYPE_ISVALID;
    FTPIDLIST fi = {0};

    // Is it unknown?
    if (!fTypeKnown)
    {
        // HACK: We will assume everything w/o a file extension is a Dir
        //    and everything w/an extension is a file.
        fTypeKnown = TRUE;
        fIsFile = (0 != *PathFindExtension(pwzDisplayName)) ? TRUE : FALSE;
    }
    if (fTypeKnown)
    {
        if (fIsFile)
            dwType |= IDTYPE_FILE;
        else if (fIsFragment)
            dwType |= IDTYPE_FRAGMENT;
        else
            dwType |= IDTYPE_DIR;
    }
    else
    {
        // You need to know if it's a fragment because there is no
        // heuristic to find out.
        ASSERT(!fIsFragment);

        dwType |= IDTYPE_FILEORDIR;
    }

    fi.dwIDType = dwType;
    fi.dwAttributes = (fIsFile ? FILE_ATTRIBUTE_NORMAL : FILE_ATTRIBUTE_NORMAL|FILE_ATTRIBUTE_DIRECTORY);
    fi.uliFileSize.QuadPart = 0;
    StrCpyNW(fi.wzDisplayName, pwzDisplayName, ARRAYSIZE(fi.wzDisplayName));
    StrCpyNA(fi.szWireName, pwWireName, ARRAYSIZE(fi.szWireName));

    hr = FtpItemID_Alloc(&fi, ppidl);
    ASSERT(IsValidPIDL(*ppidl));
    ASSERT_POINTER_MATCHES_HRESULT(*ppidl, hr);

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpItemID_SetName

    DESCRIPTION:
        The user chose a new name for the ftp file or dir (in unicode).  We
    now need to create the name in wire bytes and we will use the original
    wire byte name to decide how to do that (from pidl).
\*****************************************************************************/
HRESULT FtpItemID_CreateWithNewName(LPCITEMIDLIST pidl, LPCWSTR pwzDisplayName, LPCWIRESTR pwWireName, LPITEMIDLIST * ppidlOut)
{
    HRESULT hr;
    FTPIDLIST fi;
    const FTPIDLIST * pfi = FtpItemID_GetData(pidl);
    CWireEncoding cWireEncoding;

    CopyMemory(&fi, pfi, sizeof(FTPIDLIST) - sizeof(fi.szWireName) - sizeof(fi.wzDisplayName));
    StrCpyNW(fi.wzDisplayName, pwzDisplayName, ARRAYSIZE(fi.wzDisplayName));
    StrCpyNA(fi.szWireName, pwWireName, ARRAYSIZE(fi.szWireName));

    hr = FtpItemID_Alloc(&fi, ppidlOut);
    ASSERT(IsValidPIDL(*ppidlOut));

    return hr;
}


HRESULT Private_GetFileInfo(SHFILEINFO *psfi, DWORD rgf, LPCTSTR pszName, DWORD dwFileAttributes)
{
    HRESULT hr = E_FAIL;

    if (SHGetFileInfo(pszName, dwFileAttributes, psfi, sizeof(*psfi), rgf | SHGFI_USEFILEATTRIBUTES))
        hr = S_OK;

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpPidl_GetFileInfo

    DESCRIPTION:
        _UNDOCUMENTED_:  We strip the Hidden and System bits so
    that SHGetFileInfo won't think that we're passing something
    that might be a junction.

    We also force the SHGFI_USEFILEATTRIBUTES bit to remind the shell
    that this isn't a file.
\*****************************************************************************/
HRESULT FtpPidl_GetFileInfo(LPCITEMIDLIST pidl, SHFILEINFO *psfi, DWORD rgf)
{
    HRESULT hr;
    TCHAR szDisplayName[MAX_PATH];

    psfi->iIcon = 0;
    psfi->hIcon = NULL;
    psfi->dwAttributes = 0;
    psfi->szDisplayName[0] = 0;
    psfi->szTypeName[0] = 0;

    ASSERT(IsValidPIDL(pidl));
    if (FtpID_IsServerItemID(pidl))
    {
        FtpServerID_GetServer(pidl, szDisplayName, ARRAYSIZE(szDisplayName));
        hr = Private_GetFileInfo(psfi, rgf, szDisplayName, FILE_ATTRIBUTE_DIRECTORY);
        if (psfi->hIcon)
            DestroyIcon(psfi->hIcon);

        psfi->hIcon = LoadIcon(HINST_THISDLL, MAKEINTRESOURCE(IDI_FTPFOLDER));
        ASSERT(psfi->hIcon);

        // Now replace the type (szTypeName) with "FTP Server" because
        // it could go in the Properties dialog
        EVAL(LoadString(HINST_THISDLL, IDS_ITEMTYPE_SERVER, psfi->szTypeName, ARRAYSIZE(psfi->szTypeName)));
    }
    else
    {
        LPFTPIDLIST pfi = FtpItemID_GetData(pidl);
        FtpItemID_GetDisplayName(pidl, szDisplayName, ARRAYSIZE(szDisplayName));
        hr = Private_GetFileInfo(psfi, rgf, szDisplayName, (pfi->dwAttributes & ~(FILE_ATTRIBUTE_HIDDEN | FILE_ATTRIBUTE_SYSTEM)));
    }

    return hr;
}

HRESULT FtpPidl_GetFileType(LPCITEMIDLIST pidl, LPTSTR pszType, DWORD cchSize)
{
    SHFILEINFO sfi;
    HRESULT hr;

    ASSERT(IsValidPIDL(pidl));
    hr = FtpPidl_GetFileInfo(pidl, &sfi, SHGFI_TYPENAME);
    if (EVAL(SUCCEEDED(hr)))
    {
        StrCpyN(pszType, sfi.szTypeName, cchSize);
        if (sfi.hIcon)
            DestroyIcon(sfi.hIcon);
    }

    return hr;
}


HRESULT FtpPidl_GetFileTypeStrRet(LPCITEMIDLIST pidl, LPSTRRET pstr)
{
    WCHAR szType[MAX_URL_STRING];
    HRESULT hr;

    ASSERT(IsValidPIDL(pidl));
    hr = FtpPidl_GetFileType(pidl, szType, ARRAYSIZE(szType));
    if (EVAL(SUCCEEDED(hr)))
        StringToStrRetW(szType, pstr);

    return hr;
}


/*****************************************************************************\
    FUNCTION: _FtpItemID_CompareOneID

    DESCRIPTION:
        ici - attribute (column) to compare

    Note! that UNIX filenames are case-*sensitive*.

    We make two passes on the name.  If the names are different in other
    than case, we return the result of that comparison.  Otherwise,
    we return the result of a case-sensitive comparison.

    This algorithm ensures that the items sort themselves in a
    case-insensitive way, with ties broken by a case-sensitive
    comparison.  This makes ftp folders act "mostly" like normal
    folders.

    _UNDOCUMENTED_: The documentation says that the ici parameter
    is undefined and must be zero.  In reality, it is the column
    number (defined by IShellView) for which the comparison is to
    be made.
\*****************************************************************************/
HRESULT _FtpItemID_CompareOneID(LPARAM ici, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, DWORD dwCompFlags)
{
    int iRc = 0;    // 0 means we don't know.
    HRESULT hr = S_OK;

    ASSERT(IsValidPIDL(pidl1));
    ASSERT(IsValidPIDL(pidl2));

    // Are they both the same type? (Both Dirs or both files?)
    if (!(dwCompFlags & FCMP_GROUPDIRS) || (!FtpPidl_IsDirectory(pidl1, FALSE) == !FtpPidl_IsDirectory(pidl2, FALSE)))
    {
        switch (ici & SHCIDS_COLUMNMASK)
        {
        case COL_NAME:
        {
            // Yes they are the same, so we will key off the name...
            WIRECHAR szName1[MAX_PATH];
            WIRECHAR szName2[MAX_PATH];

            szName1[0] = TEXT('\0');
            szName2[0] = TEXT('\0');

            FtpPidl_GetWireName(pidl1, szName1, ARRAYSIZE(szName1));
            FtpPidl_GetWireName(pidl2, szName2, ARRAYSIZE(szName2));

            iRc = StrCmpIA(szName1, szName2);
            if (0 == iRc)
            {
                if (!(dwCompFlags & FCMP_CASEINSENSE))
                    iRc = StrCmpA(szName1, szName2);

/*
                // They are the same name, so now lets check on the username
                // if they are Server IDs.
                if ((0 == iRc) && (FtpID_IsServerItemID(pidl1)))
                {
                    FtpPidl_GetUserName(pidl1, szName1, ARRAYSIZE(szName1));
                    FtpPidl_GetUserName(pidl2, szName2, ARRAYSIZE(szName2));
                    iRc = StrCmp(szName1, szName2);
                }
*/
            }
        }
        break;

        case COL_SIZE:
            if (FtpPidl_GetFileSize(pidl1) < FtpPidl_GetFileSize(pidl2))
                iRc = -1;
            else if (FtpPidl_GetFileSize(pidl1) > FtpPidl_GetFileSize(pidl2))
                iRc = +1;
            else
                iRc = 0;        // I don't know
            break;

        case COL_TYPE:
            if (!FtpID_IsServerItemID(pidl1) && !FtpID_IsServerItemID(pidl2))
            {
                TCHAR szType1[MAX_PATH];

                hr = FtpPidl_GetFileType(pidl1, szType1, ARRAYSIZE(szType1));
                if (EVAL(SUCCEEDED(hr)))
                {
                    TCHAR szType2[MAX_PATH];
                    hr = FtpPidl_GetFileType(pidl2, szType2, ARRAYSIZE(szType2));
                    if (EVAL(SUCCEEDED(hr)))
                        iRc = StrCmpI(szType1, szType2);
                }
            }
            break;

        case COL_MODIFIED:
        {
            FILETIME ft1 = FtpPidl_GetFileTime(pidl1);
            FILETIME ft2 = FtpPidl_GetFileTime(pidl2);
            iRc = CompareFileTime(&ft1, &ft2);
        }
            break;

        default:
            hr = E_NOTIMPL;
            break;
        }
    }
    else
    {
        // No they are different.  We want the Folder to always come first.
        // This doesn't seam right, but it forces folders to bubble to the top
        // in the most frequent case and it matches DefView's Behavior.
        if (FtpPidl_IsDirectory(pidl1, FALSE))
            iRc = -1;
        else
            iRc = 1;
    }

    if (EVAL(S_OK == hr))
        hr = HRESULT_FROM_SUCCESS_VALUE(iRc);   // encode the sort value in the return code.

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpItemID_CompareIDs
    
    DESCRIPTION:
        ici - attribute (column) to compare

    Note! that we rely on the fact that IShellFolders are
    uniform; we do not need to bind to the shell folder in
    order to compare its sub-itemids.

    _UNDOCUMENTED_: The documentation does not say whether or not
    complex pidls can be received.  In fact, they can.

    The reason why the shell asks you to handle complex pidls
    is that you can often short-circuit the comparison by walking
    the ID list directly.  (Formally speaking, you need to bind
    to each ID and then call yourself recursively.  But if your
    pidls are uniform, you can just use a loop like the one below.)
\*****************************************************************************/
HRESULT FtpItemID_CompareIDs(LPARAM ici, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, DWORD dwCompFlags)
{
    HRESULT hr;

    if (!pidl1 || ILIsEmpty(pidl1))
    {
        if (!pidl2 || ILIsEmpty(pidl2))
            hr = HRESULT_FROM_SUCCESS_VALUE(0);        // Both ID lists are empty
        else
            hr = HRESULT_FROM_SUCCESS_VALUE(-1);        // pidl1 is empty, pidl2 is nonempty
    }
    else
    {
        if (!pidl2 || ILIsEmpty(pidl2))
            hr = HRESULT_FROM_SUCCESS_VALUE(1);     // pidl1 is nonempty, pidl2 is empty
        else
        {
            ASSERT(IsValidPIDL(pidl1));
            ASSERT(IsValidPIDL(pidl2));
            hr = _FtpItemID_CompareOneID(ici, pidl1, pidl2, dwCompFlags);    // both are nonempty
        }
    }

    // If this level of ItemsIDs are equal, then we will compare the next
    // level of ItemIDs
    if ((hr == HRESULT_FROM_SUCCESS_VALUE(0)) && pidl1 && !ILIsEmpty(pidl1))
        hr = FtpItemID_CompareIDs(ici, _ILNext(pidl1), _ILNext(pidl2), dwCompFlags);

    return hr;
}


int FtpItemID_CompareIDsInt(LPARAM ici, LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2, DWORD dwCompFlags)
{
    HRESULT hr = FtpItemID_CompareIDs(ici, pidl1, pidl2, dwCompFlags);
    int nResult = (DWORD)(short)hr;

    return nResult;
}

DWORD FtpItemID_GetAttributes(LPCITEMIDLIST pidl)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
        return NULL;

    return pFtpIDList->dwAttributes;
}

HRESULT FtpItemID_SetAttributes(LPCITEMIDLIST pidl, DWORD dwAttribs)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
        return E_INVALIDARG;

    pFtpIDList->dwAttributes = dwAttribs;
    return S_OK;
}


DWORD FtpItemID_GetUNIXPermissions(LPCITEMIDLIST pidl)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
        return NULL;

    return pFtpIDList->dwUNIXPermission;
}


HRESULT FtpItemID_SetUNIXPermissions(LPCITEMIDLIST pidl, DWORD dwUNIXPermission)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
        return E_INVALIDARG;

    pFtpIDList->dwUNIXPermission = dwUNIXPermission;
    return S_OK;
}


DWORD FtpItemID_GetCompatFlags(LPCITEMIDLIST pidl)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
        return NULL;

    return pFtpIDList->dwCompatFlags;
}


HRESULT FtpItemID_SetCompatFlags(LPCITEMIDLIST pidl, DWORD dwCompatFlags)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
        return E_INVALIDARG;

    pFtpIDList->dwCompatFlags = dwCompatFlags;
    return S_OK;
}


ULONGLONG FtpItemID_GetFileSize(LPCITEMIDLIST pidl)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
        return NULL;


    ASSERT(IsFlagSet(pFtpIDList->dwIDType, IDVALID_FILESIZE));
    return pFtpIDList->uliFileSize.QuadPart;
}

void FtpItemID_SetFileSize(LPCITEMIDLIST pidl, ULARGE_INTEGER uliFileSize)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
        return;

    pFtpIDList->uliFileSize = uliFileSize;
    pFtpIDList->dwIDType |= IDVALID_FILESIZE;
}

DWORD FtpItemID_GetFileSizeLo(LPCITEMIDLIST pidl)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
        return NULL;

    ASSERT(IsFlagSet(pFtpIDList->dwIDType, IDVALID_FILESIZE));
    return pFtpIDList->uliFileSize.LowPart;
}

DWORD FtpItemID_GetFileSizeHi(LPCITEMIDLIST pidl)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
        return NULL;

    ASSERT(IsFlagSet(pFtpIDList->dwIDType, IDVALID_FILESIZE));
    return pFtpIDList->uliFileSize.HighPart;
}


// Return value is in Local Time Zone.
FILETIME FtpItemID_GetFileTime(LPCITEMIDLIST pidl)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
    {
        FILETIME ftEmpty = {0};
        return ftEmpty;
    }

    ASSERT(IsFlagSet(pFtpIDList->dwIDType, IDVALID_MOD_DATE));
    return pFtpIDList->ftModified;
}


LPCWIRESTR FtpItemID_GetWireNameReference(LPCITEMIDLIST pidl)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList) || IS_FRAGMENT(pFtpIDList))
        return NULL;

    return pFtpIDList->szWireName;
}


HRESULT FtpItemID_GetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwzName, DWORD cchSize)
{
    HRESULT hr = S_OK;
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (pFtpIDList && !IS_FRAGMENT(pFtpIDList))
    {
        // The display name wasn't stored in v3
        StrCpyN(pwzName, FtpItemID_GetDisplayNameReference(pidl), cchSize);
    }
    else 
    {
        pwzName[0] = TEXT('\0');
        hr = E_FAIL;
    }

    return hr;
}


HRESULT FtpItemID_GetWireName(LPCITEMIDLIST pidl, LPWIRESTR pszName, DWORD cchSize)
{
    HRESULT hr = S_OK;
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (pFtpIDList && !IS_FRAGMENT(pFtpIDList))
        StrCpyNA(pszName, pFtpIDList->szWireName, cchSize);
    else 
    {
        pszName[0] = TEXT('\0');
        hr = E_FAIL;
    }

    return hr;
}


HRESULT FtpItemID_GetFragment(LPCITEMIDLIST pidl, LPWSTR pwzFragmentStr, DWORD cchSize)
{
    HRESULT hr = S_OK;
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (pFtpIDList && IS_FRAGMENT(pFtpIDList))
        StrCpyNW(pwzFragmentStr, FtpItemID_GetDisplayNameReference(pidl), cchSize);
    else 
    {
        pwzFragmentStr[0] = TEXT('\0');
        hr = E_FAIL;
    }

    return hr;
}


BOOL FtpItemID_IsFragment(LPCITEMIDLIST pidl)
{
    BOOL fIsFrag = FALSE;
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (pFtpIDList && IS_FRAGMENT(pFtpIDList))
        fIsFrag = TRUE;

    return fIsFrag;
}


// fileTime In UTC
void FtpItemID_SetFileTime(LPCITEMIDLIST pidl, FILETIME fileTime)
{
    FILETIME fileTimeLocal;
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
        return;

    FileTimeToLocalFileTime(&fileTime, &fileTimeLocal);
    pFtpIDList->ftModified = fileTimeLocal;
    pFtpIDList->dwIDType |= IDVALID_MOD_DATE;
}

BOOL FtpItemID_IsDirectory(LPCITEMIDLIST pidl, BOOL fAssumeDirForUnknown)
{
    LPFTPIDLIST pFtpIDList = FtpItemID_GetData(pidl);

    if (!EVAL(pFtpIDList))
        return NULL;

    BOOL fIsDir = (IsFlagSet(pFtpIDList->dwIDType, IDTYPE_DIR));
    
    if (fAssumeDirForUnknown && (IDTYPE_FILEORDIR == pFtpIDList->dwIDType))
    {
//        TraceMsg(TF_FTPURL_UTILS, "FtpItemID_IsDirectory() IDTYPE_FILEORDIR is set, so we assume %s", (fAssumeDirForUnknown ? TEXT("DIR") : TEXT("FILE")));
        fIsDir = TRUE;
    }
    else
    {
//        TraceMsg(TF_FTPURL_UTILS, "FtpItemID_IsDirectory() It is known to be a %s", (fIsDir ? TEXT("DIR") : TEXT("FILE")));
    }

    return fIsDir;
}




/****************************************************\
    Functions to work on an entire FTP PIDLs
\****************************************************/
#define SZ_ASCII_DOWNLOAD_TYPE       TEXT("a")
#define SZ_BINARY_DOWNLOAD_TYPE      TEXT("b")

HRESULT FtpPidl_GetDownloadTypeStr(LPCITEMIDLIST pidl, LPTSTR szDownloadType, DWORD cchTypeStrSize)
{
    HRESULT hr = S_FALSE;   // We may not have a type.
    DWORD dwTypeID = FtpServerID_GetTypeID(pidl);

    szDownloadType[0] = TEXT('\0');
    if (IDVALID_DLTYPE & dwTypeID)
    {
        hr = S_OK;
        StrCpyN(szDownloadType, SZ_FTP_URL_TYPE, cchTypeStrSize);

        if (IDVALID_DL_ASCII & dwTypeID)
            StrCatBuff(szDownloadType, SZ_ASCII_DOWNLOAD_TYPE, cchTypeStrSize);
        else
            StrCatBuff(szDownloadType, SZ_BINARY_DOWNLOAD_TYPE, cchTypeStrSize);
    }

    return hr;
}

DWORD FtpPidl_GetDownloadType(LPCITEMIDLIST pidl)
{
    DWORD dwAttribs = FTP_TRANSFER_TYPE_UNKNOWN;
    DWORD dwTypeID = FtpServerID_GetTypeID(pidl);

    ASSERT(FtpID_IsServerItemID(pidl));
    if (IDVALID_DLTYPE & dwTypeID)
    {
        if (IDVALID_DL_ASCII & dwTypeID)
            dwAttribs = FTP_TRANSFER_TYPE_ASCII;
        else
            dwAttribs = FTP_TRANSFER_TYPE_BINARY;
    }

    return dwAttribs;
}


INTERNET_PORT FtpPidl_GetPortNum(LPCITEMIDLIST pidl)
{
    ASSERT(FtpID_IsServerItemID(pidl));

    return FtpServerID_GetPortNum(pidl);
}


BOOL FtpPidl_IsDirectory(LPCITEMIDLIST pidl, BOOL fAssumeDirForUnknown)
{
    BOOL fIsDir = FALSE;
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);

    if (!FtpID_IsServerItemID(pidlLast))
        fIsDir = FtpItemID_IsDirectory(pidlLast, fAssumeDirForUnknown);

    return fIsDir;
}


BOOL FtpPidl_IsAnonymous(LPCITEMIDLIST pidl)
{
    BOOL fIsAnonymous = TRUE;

    if (IDVALID_USERNAME & FtpServerID_GetTypeID(pidl))
        fIsAnonymous = FALSE;

    return fIsAnonymous;
}


HRESULT FtpPidl_GetServer(LPCITEMIDLIST pidl, LPTSTR pszServer, DWORD cchSize)
{
    if (!FtpID_IsServerItemID(pidl)) // Will fail if we are handed a non-server ID.
        return E_FAIL;

    return FtpServerID_GetServer(pidl, pszServer, cchSize);
}


BOOL FtpPidl_IsDNSServerName(LPCITEMIDLIST pidl)
{
    BOOL fIsDNSServer = FALSE;
    TCHAR szServer[INTERNET_MAX_HOST_NAME_LENGTH];

    if (EVAL(SUCCEEDED(FtpPidl_GetServer(pidl, szServer, ARRAYSIZE(szServer)))))
        fIsDNSServer = !IsIPAddressStr(szServer);

    return fIsDNSServer;
}


HRESULT FtpPidl_GetUserName(LPCITEMIDLIST pidl, LPTSTR pszUserName, DWORD cchSize)
{
    ASSERT(FtpID_IsServerItemID(pidl));
    return FtpServerID_GetUserName(pidl, pszUserName, cchSize);
}

HRESULT FtpPidl_GetPassword(LPCITEMIDLIST pidl, LPTSTR pszPassword, DWORD cchSize, BOOL fIncludingHiddenPassword)
{
    ASSERT(FtpID_IsServerItemID(pidl));
    return FtpServerID_GetPassword(pidl, pszPassword, cchSize, fIncludingHiddenPassword);
}


ULONGLONG FtpPidl_GetFileSize(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);
    ULONGLONG ullFileSize;
    ullFileSize = 0;

    if (!FtpID_IsServerItemID(pidlLast))
        ullFileSize = FtpItemID_GetFileSize(pidlLast);

    return ullFileSize;
}


HRESULT FtpPidl_SetFileSize(LPCITEMIDLIST pidl, DWORD dwSizeHigh, DWORD dwSizeLow)
{
    HRESULT hr = E_INVALIDARG;
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);

    if (!FtpID_IsServerItemID(pidlLast))
    {
        ULARGE_INTEGER uliFileSize;

        uliFileSize.HighPart = dwSizeHigh;
        uliFileSize.LowPart = dwSizeLow;
        FtpItemID_SetFileSize(pidlLast, uliFileSize);
        hr = S_OK;
    }

    return hr;
}

// Return value in UTC time.
FILETIME FtpPidl_GetFileTime(LPCITEMIDLIST pidl)
{
    FILETIME fileTimeFTP = FtpPidl_GetFTPFileTime(pidl);   // This is what servers will be.
    FILETIME fileTime;

    EVAL(LocalFileTimeToFileTime(&fileTimeFTP, &fileTime));

    return fileTime;
}


// Return value is in Local Time Zone.
FILETIME FtpPidl_GetFTPFileTime(LPCITEMIDLIST pidl)
{
    FILETIME fileTime = {0};   // This is what servers will be.

    if (!FtpID_IsServerItemID(pidl))
        fileTime = FtpItemID_GetFileTime(pidl);

    return fileTime;
}


HRESULT FtpPidl_GetDisplayName(LPCITEMIDLIST pidl, LPWSTR pwzName, DWORD cchSize)
{
    HRESULT hr = E_INVALIDARG;

    if (pidl)
    {
        if (FtpID_IsServerItemID(pidl))
            hr = FtpServerID_GetServer(pidl, pwzName, cchSize);
        else
            hr = FtpItemID_GetDisplayName(pidl, pwzName, cchSize);
    }

    return hr;
}


HRESULT FtpPidl_GetWireName(LPCITEMIDLIST pidl, LPWIRESTR pwName, DWORD cchSize)
{
    HRESULT hr = E_INVALIDARG;

    if (pidl)
    {
        if (FtpID_IsServerItemID(pidl))
        {
            WCHAR wzServerName[INTERNET_MAX_HOST_NAME_LENGTH];

            // It's a good thing Server Names need to be in US ANSI
            hr = FtpServerID_GetServer(pidl, wzServerName, ARRAYSIZE(wzServerName));
            SHUnicodeToAnsi(wzServerName, pwName, cchSize);
        }
        else
            hr = FtpItemID_GetWireName(pidl, pwName, cchSize);
    }

    return hr;
}


HRESULT FtpPidl_GetFragment(LPCITEMIDLIST pidl, LPTSTR pszFragment, DWORD cchSize)
{
    HRESULT hr = E_INVALIDARG;
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);

    if (!FtpID_IsServerItemID(pidlLast))
        hr = FtpItemID_GetFragment(pidlLast, pszFragment, cchSize);
    else
    {
        pszFragment[0] = 0;
    }

    return hr;
}


DWORD FtpPidl_GetAttributes(LPCITEMIDLIST pidl)
{
    DWORD dwAttribs = FILE_ATTRIBUTE_READONLY | FILE_ATTRIBUTE_TEMPORARY;
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);

    if (!FtpID_IsServerItemID(pidlLast))
        dwAttribs = FtpItemID_GetAttributes(pidlLast);
    else
        dwAttribs = FILE_ATTRIBUTE_DIRECTORY;

    return dwAttribs;
}


HRESULT FtpPidl_SetAttributes(LPCITEMIDLIST pidl, DWORD dwAttribs)
{
    HRESULT hr = E_INVALIDARG;
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);

    if (!FtpID_IsServerItemID(pidlLast))
        hr = FtpItemID_SetAttributes(pidlLast, dwAttribs);

    return hr;
}


// ftTimeDate In UTC
HRESULT FtpPidl_SetFileTime(LPCITEMIDLIST pidl, FILETIME ftTimeDate)
{
    HRESULT hr = E_INVALIDARG;
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);

    if (!FtpID_IsServerItemID(pidlLast))
    {
        FtpItemID_SetFileTime(pidlLast, ftTimeDate);
        hr = S_OK;
    }

    return hr;
}


/****************************************************\
    FUNCTION: FtpPidl_GetFileWireName

    DESCRIPTION:
        Get the file name.
\****************************************************/
LPCWIRESTR FtpPidl_GetFileWireName(LPCITEMIDLIST pidl)
{
    if (EVAL(!FtpID_IsServerItemID(pidl)) &&
        !FtpItemID_IsFragment(pidl))
    {
        return FtpItemID_GetWireNameReference(pidl);
    }

    return NULL;
}


/****************************************************\
    FUNCTION: FtpPidl_GetFileDisplayName

    DESCRIPTION:
        Get the file name.
\****************************************************/
LPCWSTR FtpPidl_GetFileDisplayName(LPCITEMIDLIST pidl)
{
    if (EVAL(!FtpID_IsServerItemID(pidl)) &&
        !FtpItemID_IsFragment(pidl))
    {
        return FtpItemID_GetDisplayNameReference(pidl);
    }

    return NULL;
}


/****************************************************\
    FUNCTION: FtpPidl_GetLastItemDisplayName

    DESCRIPTION:
        This will get the last item name, even if that
    last item is a fragment.
\****************************************************/
LPCWSTR FtpPidl_GetLastItemDisplayName(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);

    if (EVAL(!FtpID_IsServerItemID(pidlLast)))
        return FtpItemID_GetDisplayNameReference(pidlLast);

    return NULL;
}


/****************************************************\
    FUNCTION: FtpPidl_GetLastItemWireName

    DESCRIPTION:
        This will get the last item name, even if that
    last item is a fragment.
\****************************************************/
LPCWIRESTR FtpPidl_GetLastItemWireName(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);

    if (FtpItemID_IsFragment(pidlLast) && (pidlLast != pidl))
    {
        // Oops, we went to far.  Step back one.
        LPCITEMIDLIST pidlFrag = pidlLast;

        pidlLast = pidl;    // Start back at the beginning.
        while (!FtpItemID_IsEqual(_ILNext(pidlLast), pidlFrag))
        {
            if (ILIsEmpty(pidlLast))
                return NULL;    // Break infinite loop.

            pidlLast = _ILNext(pidlLast);
        }
    }

    return FtpPidl_GetFileWireName(pidlLast);
}


/****************************************************\
    FUNCTION: FtpPidl_GetLastFileDisplayName

    DESCRIPTION:
        This will get the last item name, even if that
    last item is a fragment.
\****************************************************/
LPCWSTR FtpPidl_GetLastFileDisplayName(LPCITEMIDLIST pidl)
{
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);

    if (FtpItemID_IsFragment(pidlLast) && (pidlLast != pidl))
    {
        // Oops, we went to far.  Step back one.
        LPCITEMIDLIST pidlFrag = pidlLast;

        pidlLast = pidl;    // Start back at the beginning.
        while (!FtpItemID_IsEqual(_ILNext(pidlLast), pidlFrag))
        {
            if (ILIsEmpty(pidlLast))
                return NULL;    // Break infinite loop.

            pidlLast = _ILNext(pidlLast);
        }
    }

    if (EVAL(!FtpID_IsServerItemID(pidlLast)))
        return FtpItemID_GetDisplayNameReference(pidlLast);

    return NULL;
}


/****************************************************\
    FUNCTION: FtpPidl_InsertVirtualRoot

    DESCRIPTION:
        This function will insert the virtual root path
    (pidlVirtualRoot) into pidlFtpPath.

    PARAMETERS:
        pidlVirtualRoot: Does not have a ServerID
        pidlFtpPath: Can have a ServerID
        *ppidl: Will be pidlVirtualRoot with item ItemIDs from
            pidlFtpPath behind it. (No ServerID)
\****************************************************/
HRESULT FtpPidl_InsertVirtualRoot(LPCITEMIDLIST pidlVirtualRoot, LPCITEMIDLIST pidlFtpPath, LPITEMIDLIST * ppidl)
{
    HRESULT hr = E_OUTOFMEMORY;

    if (FtpID_IsServerItemID(pidlFtpPath))
        pidlFtpPath = _ILNext(pidlFtpPath);

    *ppidl = ILCombine(pidlVirtualRoot, pidlFtpPath);
    if (*ppidl)
        hr = S_OK;

    return hr;
}


DWORD FtpPidl_GetVersion(LPCITEMIDLIST pidl)
{
    if (!EVAL(FtpID_IsServerItemID(pidl)))
        return 0;

    LPFTPSERVERIDLIST pFtpServerItemId = (LPFTPSERVERIDLIST) ProtocolIdlInnerData(pidl);
    return pFtpServerItemId->dwVersion;
}


BOOL FtpPidl_IsValid(LPCITEMIDLIST pidl)
{
    if (!EVAL(IsValidPIDL(pidl)))
        return FALSE;

    return TRUE;
}


BOOL FtpPidl_IsValidFull(LPCITEMIDLIST pidl)
{
    if (!EVAL(FtpID_IsServerItemID(pidl)))
        return FALSE;

    if (!EVAL(FtpPidl_IsValid(pidl)))
        return FALSE;

    // We consider anything older than PIDL_VERSION_NUMBER_UPGRADE
    // to be invalid.
    return ((PIDL_VERSION_NUMBER_UPGRADE - 1) < FtpPidl_GetVersion(pidl));
}


BOOL FtpPidl_IsValidRelative(LPCITEMIDLIST pidl)
{
    if (!EVAL(!FtpID_IsServerItemID(pidl)))
        return FALSE;       // This is a server item id which is not relative.

    return FtpPidl_IsValid(pidl);
}


LPITEMIDLIST ILCloneFirstItemID(LPITEMIDLIST pidl)
{
    LPITEMIDLIST pidlCopy = ILClone(pidl);

    if (pidlCopy && pidlCopy->mkid.cb)
    {
        LPITEMIDLIST pSecondID = (LPITEMIDLIST)_ILNext(pidlCopy);

        ASSERT(pSecondID);
        // Remove the last one
        pSecondID->mkid.cb = 0; // null-terminator
    }

    return pidlCopy;
}

BOOL FtpPidl_HasPath(LPCITEMIDLIST pidl)
{
    BOOL fResult = TRUE;
    
    if (!FtpPidl_IsValid(pidl) || !EVAL(FtpID_IsServerItemID(pidl)))
        return FALSE;

    if (!ILIsEmpty(pidl) && ILIsEmpty(_ILNext(pidl)))
        fResult = FALSE;

    return fResult;
}


HRESULT FtpPidl_SetFileItemType(LPITEMIDLIST pidl, BOOL fIsDir)
{
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);
    HRESULT hr = E_INVALIDARG;
        
    if (EVAL(FtpPidl_IsValid(pidl)) && EVAL(!FtpID_IsServerItemID(pidlLast)))
    {
        DWORD dwIDType = FtpItemID_GetTypeID(pidlLast);

        ClearFlag(dwIDType, (IDTYPE_FILEORDIR | IDTYPE_DIR | IDTYPE_FILE));
        SetFlag(dwIDType, (fIsDir ? IDTYPE_DIR : IDTYPE_FILE));
        FtpItemID_SetTypeID((LPITEMIDLIST) pidlLast, dwIDType);

        hr = S_OK;
    }

    return hr;
}


BOOL IsFtpPidlQuestionable(LPCITEMIDLIST pidl)
{
    BOOL fIsQuestionable = FALSE;
    LPCITEMIDLIST pidlLast = ILGetLastID(pidl);

    // Is it a Server Pidl? (All Server pidls aren't questionable)
    if (FtpPidl_IsValid(pidl) && !FtpID_IsServerItemID(pidlLast))
    {
        // No, so it might be questionable.

        // Does it have "File or Dir" bit set?
        if (IsFlagSet(FtpItemID_GetTypeID(pidlLast), IDTYPE_FILEORDIR))
            fIsQuestionable = TRUE;
    }
    
    return fIsQuestionable;
}


LPITEMIDLIST FtpCloneServerID(LPCITEMIDLIST pidl)
{
    LPITEMIDLIST pidlResult = NULL;

    if (EVAL(FtpID_IsServerItemID(pidl)))
    {
        pidlResult = ILClone(pidl);

        while (!ILIsEmpty(_ILNext(pidlResult)))
            ILRemoveLastID(pidlResult);
    }

    return pidlResult;
}


/*****************************************************************************\
    FUNCTION: FtpPidl_ReplacePath

    DESCRIPTION:
        This function will fill in *ppidlOut with a pidl that contains the
    FtpServerID from pidlServer and the FtpItemIDs from pidlFtpPath.
\*****************************************************************************/
HRESULT FtpPidl_ReplacePath(LPCITEMIDLIST pidlServer, LPCITEMIDLIST pidlFtpPath, LPITEMIDLIST * ppidlOut)
{
    HRESULT hr = E_INVALIDARG;

    *ppidlOut = NULL;
    if (EVAL(FtpID_IsServerItemID(pidlServer)))
    {
        LPITEMIDLIST pidlServerOnly = FtpCloneServerID(pidlServer);

        if (pidlServerOnly)
        {
            if (FtpID_IsServerItemID(pidlFtpPath))
                pidlFtpPath = _ILNext(pidlFtpPath);

            *ppidlOut = ILCombine(pidlServerOnly, pidlFtpPath);
            if (*ppidlOut)
                hr = S_OK;
            else
                hr = E_FAIL;

            ILFree(pidlServerOnly);
        }
    }

    ASSERT_POINTER_MATCHES_HRESULT(*ppidlOut, hr);
    return hr;
}


BOOL FtpItemID_IsEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    // Don't repeat recursively.
    return (S_OK == _FtpItemID_CompareOneID(COL_NAME, pidl1, pidl2, FALSE));
}


BOOL FtpPidl_IsPathEqual(LPCITEMIDLIST pidl1, LPCITEMIDLIST pidl2)
{
    // This works recursively.
    return ((0 == FtpItemID_CompareIDsInt(COL_NAME, pidl1, pidl2, FCMP_NORMAL)) ? TRUE : FALSE);
}


// is pidlChild a child of pidlParent
BOOL FtpItemID_IsParent(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild)
{
    BOOL fIsChild = TRUE;

    if (pidlChild)
    {
        LPITEMIDLIST pidl1Iterate = (LPITEMIDLIST) pidlParent;
        LPITEMIDLIST pidl2Iterate = (LPITEMIDLIST) pidlChild;

        ASSERT(!FtpID_IsServerItemID(pidl1Iterate) && pidlParent && !FtpID_IsServerItemID(pidl2Iterate));

        // Let's see if pidl starts off with 
        while (fIsChild && pidl1Iterate && !ILIsEmpty(pidl1Iterate) &&
                pidl2Iterate && !ILIsEmpty(pidl2Iterate) && 
                FtpItemID_IsEqual(pidl1Iterate, pidl2Iterate))
        {
            fIsChild = FtpItemID_IsEqual(pidl1Iterate, pidl2Iterate);

            pidl1Iterate = _ILNext(pidl1Iterate);
            pidl2Iterate = _ILNext(pidl2Iterate);
        }

        if (!(ILIsEmpty(pidl1Iterate) && !ILIsEmpty(pidl2Iterate)))
            fIsChild = FALSE;
    }
    else
        fIsChild = FALSE;

    return fIsChild;
}


// is pidlChild a child of pidlParent, so all the itemIDs in
// pidlParent are in pidlChild, but pidlChild has more.
// This will return a pointer to those itemIDs.
LPCITEMIDLIST FtpItemID_FindDifference(LPCITEMIDLIST pidlParent, LPCITEMIDLIST pidlChild)
{
    LPCITEMIDLIST pidlDiff = (LPITEMIDLIST) pidlChild;

    if (pidlChild)
    {
        LPITEMIDLIST pidl1Iterate = (LPITEMIDLIST) pidlParent;

        if (FtpID_IsServerItemID(pidl1Iterate))
            pidl1Iterate = _ILNext(pidl1Iterate);

        if (FtpID_IsServerItemID(pidlDiff))
            pidlDiff = _ILNext(pidlDiff);

        // Let's see if pidl starts off with 
        while (pidl1Iterate && !ILIsEmpty(pidl1Iterate) &&
                pidlDiff && !ILIsEmpty(pidlDiff) && 
                FtpItemID_IsEqual(pidl1Iterate, pidlDiff))
        {
            pidlDiff = _ILNext(pidlDiff);
            pidl1Iterate = _ILNext(pidl1Iterate);
        }
    }

    return pidlDiff;
}


