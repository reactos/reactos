/*****************************************************************************\
    FILE: ftpapi.cpp

    DESCRIPTION:
        This file contains functions to perform the following 2 things:

    1. WININET WRAPPERS: Wininet APIs have either wierd bugs or bugs that come thru the APIs
    from the server.  It's also important to keep track of the perf impact
    of each call.  These wrappers solve these problems.

    2. FTP STRs to PIDLs: These wrappers will take ftp filenames and file paths
    that come in from the server and turn them into pidls.  These pidls contain
    both a unicode display string and the filename/path in wire bytes for future
    server requests.
\*****************************************************************************/

#include "priv.h"
#include "util.h"
#include "encoding.h"
#include "ftpapi.h"


///////////////////////////////////////////////////////////////////////////////////////////
// 1. WININET WRAPPERS: Wininet APIs have either wierd bugs or bugs that come thru the APIs
// from the server.  It's also important to keep track of the perf impact
// of each call.  These wrappers solve these problems.
///////////////////////////////////////////////////////////////////////////////////////////
/*****************************************************************************\
    FUNCTION: FtpSetCurrentDirectoryWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 1ms
        To: rigel.cyberpass.net <San Diego, CA>: 140ms - 200ms
        To: ftp.rz.uni-frankfurt.de <Germany>: 570ms - 2496ms
\*****************************************************************************/
HRESULT FtpSetCurrentDirectoryWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPCWIRESTR pwFtpPath)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    // WARNING: FtpSetCurrentDirectory() may fail if it's not really a directory.
    // PERF: Status FtpGetCurrentDirectory/FtpSetCurrentDirectory() takes
    //  180-280ms on ftp.microsoft.com on average.
    //  500-2000ms on ftp://ftp.tu-clausthal.de/ on average
    //  0-10ms on ftp://shapitst/ on average
    DEBUG_CODE(DebugStartWatch());
    if (!FtpSetCurrentDirectoryA(hConnect, pwFtpPath))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "FtpSetCurrentDirectory(%#08lx, \"%hs\") returned %u. Time=%lums", hConnect, pwFtpPath, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpGetCurrentDirectoryWrap

    DESCRIPTION:

    PERF Notes:
    These are for short directory listings.  Long listings can take 3-10x.
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 1ms - 4ms
        To: rigel.cyberpass.net <San Diego, CA>: 132ms - 213ms
        To: ftp.rz.uni-frankfurt.de <Germany>: 507ms - 2012ms
\*****************************************************************************/
HRESULT FtpGetCurrentDirectoryWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPWIRESTR pwFtpPath, DWORD cchCurrentDirectory)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    // PERF: Status FtpGetCurrentDirectory/FtpSetCurrentDirectory() takes
    //  180-280ms on ftp.microsoft.com on average.
    //  500-2000ms on ftp://ftp.tu-clausthal.de/ on average
    //  0-10ms on ftp://shapitst/ on average
    DEBUG_CODE(DebugStartWatch());
    if (!FtpGetCurrentDirectoryA(hConnect, pwFtpPath, &cchCurrentDirectory))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "FtpGetCurrentDirectoryA(%#08lx, \"%hs\") returned %u. Time=%lums", hConnect, pwFtpPath, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpGetFileExWrap

    DESCRIPTION:

    PERF Notes:   (*** Depends on file size ***)
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 100ms - 1,000ms+
        To: rigel.cyberpass.net <San Diego, CA>: 1210ms - 1610ms
        To: ftp.rz.uni-frankfurt.de <Germany>: ???????
\*****************************************************************************/
HRESULT FtpGetFileExWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPCWIRESTR pwFtpPath/*Src*/, LPCWSTR pwzFilePath/*Dest*/, BOOL fFailIfExists,
                       DWORD dwFlagsAndAttributes, DWORD dwFlags, DWORD_PTR dwContext)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    if (!FtpGetFileEx(hConnect, pwFtpPath, pwzFilePath, fFailIfExists, dwFlagsAndAttributes, dwFlags, dwContext))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
        if (HRESULT_FROM_WIN32(ERROR_INTERNET_OPERATION_CANCELLED) == hr)
            hr = HRESULT_FROM_WIN32(ERROR_CANCELLED);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "FtpGetFileEx(%#08lx, \"%hs\", \"%ls\") returned %u. Time=%lums", hConnect, pwFtpPath, pwzFilePath, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpPutFileExWrap

    DESCRIPTION:

    PERF Notes:   (*** Depends on file size ***)
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 194ms - 400ms+
        To: rigel.cyberpass.net <San Diego, CA>: 1662ms - 8454ms
        To: ftp.rz.uni-frankfurt.de <Germany>: ???????
\*****************************************************************************/
HRESULT FtpPutFileExWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPCWSTR pwzFilePath/*Src*/, LPCWIRESTR pwFtpPath/*Dest*/, DWORD dwFlags, DWORD_PTR dwContext)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    if (!FtpPutFileEx(hConnect, pwzFilePath, pwFtpPath, dwFlags, dwContext))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "FtpPutFileEx(%#08lx, \"%ls\", \"%hs\") returned %u. Time=%lums", hConnect, pwzFilePath, pwFtpPath, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpDeleteFileWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 4ms (22ms once in a while)
        To: rigel.cyberpass.net <San Diego, CA>: 175ms - 291ms
        To: ftp.rz.uni-frankfurt.de <Germany>: ???????
\*****************************************************************************/
HRESULT FtpDeleteFileWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPCWIRESTR pwFtpFileName)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    if (!FtpDeleteFileA(hConnect, pwFtpFileName))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "FtpDeleteFile(%#08lx, \"%hs\") returned %u. Time=%lums", hConnect, pwFtpFileName, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpRenameFileWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 4ms
        To: rigel.cyberpass.net <San Diego, CA>: 329ms - 446ms
        To: ftp.rz.uni-frankfurt.de <Germany>: ???????
\*****************************************************************************/
HRESULT FtpRenameFileWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPCWIRESTR pwFtpFileNameExisting, LPCWIRESTR pwFtpFileNameNew)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    if (!FtpRenameFileA(hConnect, pwFtpFileNameExisting, pwFtpFileNameNew))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "FtpRenameFile(%#08lx, \"%hs\", \"%hs\") returned %u. Time=%lums", hConnect, pwFtpFileNameExisting, pwFtpFileNameNew, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpOpenFileWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 2ms
        To: rigel.cyberpass.net <San Diego, CA>: 757ms - 817ms
        To: ftp.rz.uni-frankfurt.de <Germany>: 2112ms - 10026ms
\*****************************************************************************/
HRESULT FtpOpenFileWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPCWIRESTR pwFtpFileName, DWORD dwAccess, DWORD dwFlags, DWORD_PTR dwContext, HINTERNET * phFileHandle)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    *phFileHandle = FtpOpenFileA(hConnect, pwFtpFileName, dwAccess, dwFlags, dwContext);
    if (!*phFileHandle)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "FtpOpenFile(%#08lx, \"%hs\") returned %u. Time=%lums", hConnect, pwFtpFileName, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpCreateDirectoryWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 3ms
        To: rigel.cyberpass.net <San Diego, CA>: 210ms - 350ms
        To: ftp.rz.uni-frankfurt.de <Germany>: ???????
\*****************************************************************************/
HRESULT FtpCreateDirectoryWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPCWIRESTR pwFtpPath)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    if (!FtpCreateDirectoryA(hConnect, pwFtpPath))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "FtpCreateDirectoryA(%#08lx, \"%hs\") returned %u. Time=%lums", hConnect, pwFtpPath, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpRemoveDirectoryWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 2ms
        To: rigel.cyberpass.net <San Diego, CA>: 157ms - 227ms
        To: ftp.rz.uni-frankfurt.de <Germany>: ???????
\*****************************************************************************/
HRESULT FtpRemoveDirectoryWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPCWIRESTR pwFtpPath)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    if (!FtpRemoveDirectoryA(hConnect, pwFtpPath))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "FtpRemoveDirectory(%#08lx, \"%hs\") returned %u. Time=%lums", hConnect, pwFtpPath, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpFindFirstFileWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 166ms - 189ms
        To: rigel.cyberpass.net <San Diego, CA>: 550ms - 815ms
        To: ftp.rz.uni-frankfurt.de <Germany>: 1925ms - 11,390ms
\*****************************************************************************/
HRESULT FtpFindFirstFileWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPCWIRESTR pwFilterStr, LPFTP_FIND_DATA pwfd, DWORD dwINetFlags, DWORD_PTR dwContext, HINTERNET * phFindHandle)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(StrCpyNA(pwfd->cFileName, "<Not Found>", ARRAYSIZE(pwfd->cFileName)));
    ASSERT(phFindHandle);
    DEBUG_CODE(DebugStartWatch());
    //    _UNDOCUMENTED_: If you pass NULL as the second argument, it's the
    //    same as passing TEXT("*.*"), but much faster.
    // PERF: Status
    // FtpFindFirstFile() takes 500-700ms on ftp.microsoft.com on average.
    //                    takes 2-10 secs on ftp://ftp.tu-clausthal.de/ on average
    //                    takes 150-250 secs on ftp://shapitst/ on average
    *phFindHandle = FtpFindFirstFileA(hConnect, pwFilterStr, pwfd, dwINetFlags, dwContext);
    if (!*phFindHandle)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "FtpFindFirstFile(\"%hs\")==\"%hs\" atrb=%#08lx, hr=%#08lx, Time=%lums", EMPTYSTR_FOR_NULLA(pwFilterStr), pwfd->cFileName, pwfd->dwFileAttributes, hr, DebugStopWatch()));

    if (fAssertOnFailure)
    {
//      This fails in normal cases when we are checking if files exist.
//        WININET_ASSERT(SUCCEEDED(hr));
    }

    ASSERT_POINTER_MATCHES_HRESULT(*phFindHandle, hr);
    return hr;
}


// Do not localize this because it's always returned from the server in english.g
#define SZ_FINDFIRSTFILE_FAILURESTR     ": No such file or directory"

/**************************************************************\
    FUNCTION: FtpDoesFileExist

    DESCRIPTION:
        IE #34868 is the fact that some FTP servers erronous
    results on find requests:
    [The file does not exist in the following cases]
    Request: "foo.txt" result: SUCCEEDED & "foo.txt: No such file or directory"
    Request: "foo bat.txt" result: SUCCEEDED & "foo: No such file or directory"
\**************************************************************/
HRESULT FtpDoesFileExist(HINTERNET hConnect, BOOL fAssertOnFailure, LPCWIRESTR pwFilterStr, LPFTP_FIND_DATA pwfd, DWORD dwINetFlags)
{
    FTP_FIND_DATA wfd;
    HINTERNET hIntFind;
    HRESULT hr;

    if (!pwfd)      // pwfd is optional
        pwfd = &wfd;

    // Some servers like "ftp://wired/" will fail to find "BVTBaby.gif" even
    // though it exists.  It will find it if "BVTBaby.gif*" is used.
    // Wininet should fix this or implement this hack but they probably won't.
    WIRECHAR wFilterStr[MAX_PATH];

    StrCpyNA(wFilterStr, pwFilterStr, ARRAYSIZE(wFilterStr));
    // WININET BUGBUG: Wininet won't find "BVTBaby.gif" on an IIS server (ftp://wired/)
    // unless it has an "*" behind it.  So add one if it doesn't exist.
    if ('*' != wFilterStr[lstrlenA(wFilterStr) - 1])
    {
        // We need to add it.
        StrCatBuffA(wFilterStr, "*", ARRAYSIZE(wFilterStr));
    }

    hr = FtpFindFirstFileWrap(hConnect, fAssertOnFailure, wFilterStr, pwfd, dwINetFlags, 0, &hIntFind);
    if (S_OK == hr)
    {
        do
        {
            // is it an exact match?
            // #248535: Make sure we get what we asked for.  Either WININET or
            //     some weird FTP servers are screwing up.  If we ask for
            //     foobar.gif as the filter string, sometimes we get back
            //     ".".
            if (!StrCmpIA(pwfd->cFileName, pwFilterStr))
            {
                // Yes it "Should"
                hr = S_OK;
                break;
            }
            else
            {
                // However, wininet will return TRUE but the display name will be "One: No such file or directory"
                // if the file name is "One Two.htm"
                // This is a work around for bug #34868 because UNIX servers sometimes return success
                // and a file name of "thefile.txt: No such file or directory"
                if ((lstrlenA(pwfd->cFileName) > (ARRAYSIZE(SZ_FINDFIRSTFILE_FAILURESTR) - 1)) && 
                    !StrCmpA(&(pwfd->cFileName[lstrlenA(pwfd->cFileName) - (ARRAYSIZE(SZ_FINDFIRSTFILE_FAILURESTR) - 1)]), SZ_FINDFIRSTFILE_FAILURESTR))
                {
                    hr = S_OK;
                    break;
                }
                else
                    hr = HRESULT_FROM_WIN32(ERROR_FILE_NOT_FOUND);
            }

            // Next...
            hr = InternetFindNextFileWrap(hIntFind, TRUE, pwfd);
        }
        while (S_OK == hr);

        InternetCloseHandle(hIntFind);
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: FtpCommandWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 1ms - 12ms
        To: rigel.cyberpass.net <San Diego, CA>: 133ms - 184ms
        To: ftp.rz.uni-frankfurt.de <Germany>: 1711ms - 2000ms
\*****************************************************************************/
HRESULT FtpCommandWrap(HINTERNET hConnect, BOOL fAssertOnFailure, BOOL fExpectResponse, DWORD dwFlags, LPCWIRESTR pszCommand,
                       DWORD_PTR dwContext, HINTERNET *phFtpCommand)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    if (!FtpCommandA(hConnect, fExpectResponse, dwFlags, pszCommand, dwContext, phFtpCommand))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "FtpCommand(%#08lx, \"%hs\") returned %u. Time=%lums", hConnect, pszCommand, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


/*** TODO

INTERNETAPI
DWORD
WINAPI
FtpGetFileSize(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwFileSizeHigh OPTIONAL
    );
******/



/*****************************************************************************\
    FUNCTION: InternetOpenWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        Destination not applicable. 677-907ms
\*****************************************************************************/
HRESULT InternetOpenWrap(BOOL fAssertOnFailure, LPCTSTR pszAgent, DWORD dwAccessType, LPCTSTR pszProxy, LPCTSTR pszProxyBypass, DWORD dwFlags, HINTERNET * phFileHandle)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    *phFileHandle = InternetOpen(pszAgent, dwAccessType, pszProxy, pszProxyBypass, dwFlags);
    if (!*phFileHandle)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "InternetOpen(\"%ls\") returned %u. Time=%lums", pszAgent, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


HRESULT InternetCloseHandleWrap(HINTERNET hInternet, BOOL fAssertOnFailure)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    if (!InternetCloseHandle(hInternet))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "InternetCloseHandle(%#08lx) returned %u. Time=%lums", hInternet, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: InternetConnectWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 144ms - 250ms (Min: 2; Max: 1,667ms)
        To: rigel.cyberpass.net <San Diego, CA>: 717ms - 1006ms
        To: ftp.rz.uni-frankfurt.de <Germany>: 2609ms - 14,012ms

    COMMON ERROR VALUES:
        These are the return values in these different cases:
    ERROR_INTERNET_NAME_NOT_RESOLVED: No Proxy & DNS Lookup failed.
    ERROR_INTERNET_CANNOT_CONNECT: Some Auth Proxies and Netscape's Web/Auth Proxy
    ERROR_INTERNET_NAME_NOT_RESOLVED: Web Proxy
    ERROR_INTERNET_TIMEOUT: Invalid or Web Proxy blocked IP Address
    ERROR_INTERNET_INCORRECT_PASSWORD: IIS & UNIX, UserName may not exist or password for the user may be incorrect on.
    ERROR_INTERNET_LOGIN_FAILURE: Too many Users on IIS.
    ERROR_INTERNET_INCORRECT_USER_NAME: I haven't seen it.
    ERROR_INTERNET_EXTENDED_ERROR: yahoo.com exists, but ftp.yahoo.com doesn't.
\*****************************************************************************/
HRESULT InternetConnectWrap(HINTERNET hInternet, BOOL fAssertOnFailure, LPCTSTR pszServerName, INTERNET_PORT nServerPort,
                            LPCTSTR pszUserName, LPCTSTR pszPassword, DWORD dwService, DWORD dwFlags, DWORD_PTR dwContext, HINTERNET * phFileHandle)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    // Call BryanSt if this assert fires.
    // Did the user turn off FTP Folders?
    // If so, don't connect.  This will fix NT #406423 where the user turned
    // of FTP Folders because they have a firewall (CISCO filtering Router)
    // that will kill packets in such a way the caller (WinSock/Wininet) needs
    // to wait for a timeout.  During this timeout, the browser will hang causing
    // the user to think it crashed.
    AssertMsg(!SHRegGetBoolUSValue(SZ_REGKEY_FTPFOLDER, SZ_REGKEY_USE_OLD_UI, FALSE, FALSE), TEXT("BUG: We can't hit this code or we will hang the browser for 45 seconds if the user is using a certain kind of proxy. Call BryanSt."));

    DEBUG_CODE(DebugStartWatch());
    *phFileHandle = InternetConnect(hInternet, pszServerName, nServerPort, pszUserName, pszPassword, dwService, dwFlags | FEATURE_PASSIVE_ON_OR_OFF, dwContext);
    if (!*phFileHandle)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "InternetConnect(%#08lx, \"%ls\", \"%ls\", \"%ls\") returned %u. Time=%lums", hInternet, pszServerName, EMPTYSTR_FOR_NULL(pszUserName), EMPTYSTR_FOR_NULL(pszPassword), dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        // ERROR_INTERNET_NAME_NOT_RESOLVED happens when we are blocked by the
        // proxy.
        WININET_ASSERT(SUCCEEDED(hr) ||
            (HRESULT_FROM_WIN32(ERROR_INTERNET_NAME_NOT_RESOLVED) == hr) ||
            (HRESULT_FROM_WIN32(ERROR_INTERNET_LOGIN_FAILURE) == hr) ||
            (HRESULT_FROM_WIN32(ERROR_INTERNET_INCORRECT_PASSWORD) == hr) ||
            (HRESULT_FROM_WIN32(ERROR_INTERNET_INCORRECT_USER_NAME) == hr));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: InternetOpenUrlWrap

    DESCRIPTION:

    PERF Notes:
    [Direct Net Connection]
        To: shapitst <Down the Hall>: 29ms
        To: rigel.cyberpass.net <San Diego, CA>: ???????
        To: ftp.rz.uni-frankfurt.de <Germany>: ???????
\*****************************************************************************/
HRESULT InternetOpenUrlWrap(HINTERNET hInternet, BOOL fAssertOnFailure, LPCTSTR pszUrl, LPCTSTR pszHeaders, DWORD dwHeadersLength, DWORD dwFlags, DWORD_PTR dwContext, HINTERNET * phFileHandle)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    *phFileHandle = InternetOpenUrl(hInternet, pszUrl, pszHeaders, dwHeadersLength, dwFlags | FEATURE_PASSIVE_ON_OR_OFF, dwContext);
    if (!*phFileHandle)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "InternetOpenUrl(%#08lx, \"%ls\") returned %u. Time=%lums", hInternet, pszUrl, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


HRESULT InternetReadFileWrap(HINTERNET hFile, BOOL fAssertOnFailure, LPVOID pvBuffer, DWORD dwNumberOfBytesToRead, LPDWORD pdwNumberOfBytesRead)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

//    DEBUG_CODE(DebugStartWatch());
    if (!InternetReadFile(hFile, pvBuffer, dwNumberOfBytesToRead, pdwNumberOfBytesRead))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
//    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "InternetReadFile(%#08lx, ToRead=%d, Read=%d) returned %u. Time=%lums", hFile, dwNumberOfBytesToRead, (pdwNumberOfBytesRead ? *pdwNumberOfBytesRead : -1), dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


HRESULT InternetWriteFileWrap(HINTERNET hFile, BOOL fAssertOnFailure, LPCVOID pvBuffer, DWORD dwNumberOfBytesToWrite, LPDWORD pdwNumberOfBytesWritten)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

//    DEBUG_CODE(DebugStartWatch());
    if (!InternetWriteFile(hFile, pvBuffer, dwNumberOfBytesToWrite, pdwNumberOfBytesWritten))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
//    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "InternetWriteFile(%#08lx, ToWrite=%d, Writen=%d) returned %u. Time=%lums", hFile, dwNumberOfBytesToWrite, (pdwNumberOfBytesWritten ? *pdwNumberOfBytesWritten : -1), dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


/*****************************************************************************\
    FUNCTION: InternetGetLastResponseInfoWrap

    DESCRIPTION:

    PERF Notes:
        Always takes 0 (zero) ms because it doesn't have to hit the net.
\*****************************************************************************/
HRESULT InternetGetLastResponseInfoWrap(BOOL fAssertOnFailure, LPDWORD pdwError, LPWIRESTR pwBuffer, LPDWORD pdwBufferLength)
{
    HRESULT hr = S_OK;
    DWORD dwDummyError;

    if (!pdwError)
        pdwError = &dwDummyError;

    if (pwBuffer)
        pwBuffer[0] = 0;

    DEBUG_CODE(DebugStartWatch());
    InternetGetLastResponseInfoA(pdwError, pwBuffer, pdwBufferLength);

    if (pwBuffer)
    {
        DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "InternetGetLastResponseInfo(\"%hs\") took %lu milliseconds", pwBuffer, DebugStopWatch()));
    }
    else
    {
        DEBUG_CODE(DebugStopWatch());
    }

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}


HRESULT InternetGetLastResponseInfoDisplayWrap(BOOL fAssertOnFailure, LPDWORD pdwError, LPWSTR pwzBuffer, DWORD cchBufferSize)
{
    LPWIRESTR pwWireResponse;
    DWORD dwError = 0;
    DWORD cchResponse = 0;
    HRESULT hr = InternetGetLastResponseInfoWrap(TRUE, &dwError, NULL, &cchResponse);

    cchResponse++;                /* +1 for the terminating 0 */
    pwWireResponse = (LPWIRESTR)LocalAlloc(LPTR, cchResponse * sizeof(WIRECHAR));
    if (pwWireResponse)
    {
        hr = InternetGetLastResponseInfoWrap(TRUE, &dwError, pwWireResponse, &cchResponse);
        if (SUCCEEDED(hr))
        {
            CWireEncoding cWireEncoding;

            hr = cWireEncoding.WireBytesToUnicode(NULL, pwWireResponse, WIREENC_IMPROVE_ACCURACY, pwzBuffer, cchBufferSize);
        }

        LocalFree(pwWireResponse);
    }
    else
        hr = E_OUTOFMEMORY;

    return hr;
}


INTERNET_STATUS_CALLBACK InternetSetStatusCallbackWrap(HINTERNET hInternet, BOOL fAssertOnFailure, INTERNET_STATUS_CALLBACK pfnInternetCallback)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;
    INTERNET_STATUS_CALLBACK pfnCallBack;

    DEBUG_CODE(DebugStartWatch());
    pfnCallBack = InternetSetStatusCallback(hInternet, pfnInternetCallback);
    if (!pfnCallBack)
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "InternetSetStatusCallback(%#08lx) returned %u. Time=%lums", hInternet, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return pfnCallBack;
}


HRESULT InternetCheckConnectionWrap(BOOL fAssertOnFailure, LPCTSTR pszUrl, DWORD dwFlags, DWORD dwReserved)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(DebugStartWatch());
    if (!InternetCheckConnection(pszUrl, dwFlags, dwReserved))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "InternetCheckConnection(\"%ls\") returned %u. Time=%lums", pszUrl, dwError, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}

//#define FEATURE_OFFLINE

HRESULT InternetAttemptConnectWrap(BOOL fAssertOnFailure, DWORD dwReserved)
{
    HRESULT hr = S_OK;

#ifdef FEATURE_OFFLINE
    DEBUG_CODE(DebugStartWatch());

    hr = HRESULT_FROM_WIN32(InternetAttemptConnect(dwReserved));
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "InternetAttemptConnect() returned hr=%#08lx. Time=%lums", hr, DebugStopWatch()));

    if (fAssertOnFailure)
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }
#endif // FEATURE_OFFLINE

    return hr;
}



/*****************************************************************************\
    FUNCTION: InternetFindNextFileWrap

    DESCRIPTION:

    PERF Notes:
        Always takes 0 (zero) ms because all the work is done in FtpFindFirstFile()
\*****************************************************************************/
HRESULT InternetFindNextFileWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPFTP_FIND_DATA pwfd)
{
    HRESULT hr = S_OK;
    DWORD dwError = 0;

    DEBUG_CODE(StrCpyNA(pwfd->cFileName, "<Not Found>", ARRAYSIZE(pwfd->cFileName)));
    DEBUG_CODE(DebugStartWatch());
    // BUGBUG: Bug #206068
    //       We need to treat dwFileAttributes = 0x00000000 as a directory
    //       link.  We can do this by FtpChangeDirectory() into it, call FtpGetDirectory(),
    //       and then creating a pidl with UrlPath and navigating to it w/o creating history entry if needed.
    //       This will solve the problem that going to ftp://ftp.cdrom.com/pub/ and clicking
    //       on any of the soft links will change into that directory and update the address
    //       bar to show the real destination directory.

    // PERF: The perf of this function normally is nothing because the enum of the entire directory
    //       is done in the FtpFindFirstFile().  It will also cache the results.
    if (!InternetFindNextFileA(hConnect, pwfd))
    {
        dwError = GetLastError();
        hr = HRESULT_FROM_WIN32(dwError);
    }
    DEBUG_CODE(TraceMsg(TF_WININET_DEBUG, "InternetFindNextFile(%#08lx)==\"%hs\", atrbs=%#08lx, hr=%#08lx, Time=%lums", 
        hConnect, pwfd->cFileName, pwfd->dwFileAttributes, hr, DebugStopWatch()));

    if (fAssertOnFailure && (HRESULT_FROM_WIN32(ERROR_NO_MORE_FILES) != hr))
    {
        WININET_ASSERT(SUCCEEDED(hr));
    }

    return hr;
}





///////////////////////////////////////////////////////////////////////////////////////////
// 2. FTP STRs to PIDLs: These wrappers will take ftp filenames and file paths
// that come in from the server and turn them into pidls.  These pidls contain
// both a unicode display string and the filename/path in wire bytes for future
// server requests.
///////////////////////////////////////////////////////////////////////////////////////////

/*****************************************************************************\
    FUNCTION: FtpSetCurrentDirectoryPidlWrap

    DESCRIPTION:
        Change the current directory to the one specified.

    PARAMETERS:
        pidlFtpPath: If this is NULL, then go to "\".
\*****************************************************************************/
HRESULT FtpSetCurrentDirectoryPidlWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPCITEMIDLIST pidlFtpPath, BOOL fAbsolute, BOOL fOnlyDirs)
{
    WIRECHAR wFtpPath[MAX_PATH];
    LPWIRESTR pwFtpPath = wFtpPath;
    HRESULT hr = S_OK;
    
    // If pidlFtpPath is NULL, then go to "\".
    if (pidlFtpPath)
    {
        hr = GetWirePathFromPidl(pidlFtpPath, wFtpPath, ARRAYSIZE(wFtpPath), fOnlyDirs);
        if (!fAbsolute)
            pwFtpPath++;    // Skip past the starting '\'
    }
    else
        StrCpyNA(wFtpPath, SZ_URL_SLASHA, ARRAYSIZE(wFtpPath));

    if (SUCCEEDED(hr))
        hr = FtpSetCurrentDirectoryWrap(hConnect, fAssertOnFailure, pwFtpPath); 

    return hr;
}

HRESULT FtpGetCurrentDirectoryPidlWrap(HINTERNET hConnect, BOOL fAssertOnFailure, CWireEncoding * pwe, LPITEMIDLIST * ppidlFtpPath)
{
    WIRECHAR wFtpPath[MAX_PATH];
    HRESULT hr = FtpGetCurrentDirectoryWrap(hConnect, fAssertOnFailure, wFtpPath, ARRAYSIZE(wFtpPath));

    *ppidlFtpPath = NULL;
    if (SUCCEEDED(hr))
        hr = CreateFtpPidlFromFtpWirePath(wFtpPath, pwe, NULL, ppidlFtpPath, TRUE, TRUE);

    return hr;
}

HRESULT FtpFindFirstFilePidlWrap(HINTERNET hConnect, BOOL fAssertOnFailure, CMultiLanguageCache * pmlc,
        CWireEncoding * pwe, LPCWIRESTR pwFilterStr, LPITEMIDLIST * ppidlFtpItem, DWORD dwINetFlags, DWORD_PTR dwContext, HINTERNET * phFindHandle)
{
    FTP_FIND_DATA wfd;

    *phFindHandle = NULL;
    HRESULT hr = FtpFindFirstFileWrap(hConnect, fAssertOnFailure, pwFilterStr, &wfd, dwINetFlags, dwContext, phFindHandle);
    
    *ppidlFtpItem = NULL;

    if (SUCCEEDED(hr))
    {
        // Skip "." and ".." entries.
        if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || IS_VALID_FILE(wfd.cFileName))
            hr = pwe->CreateFtpItemID(pmlc, &wfd, ppidlFtpItem);
        else
            hr = InternetFindNextFilePidlWrap(*phFindHandle, fAssertOnFailure, pmlc, pwe, ppidlFtpItem);

        if (FAILED(hr) && *phFindHandle)
        {
            InternetCloseHandle(*phFindHandle);
            *phFindHandle = NULL;
        }
    }

    return hr;
}


HRESULT InternetFindNextFilePidlWrap(HINTERNET hConnect, BOOL fAssertOnFailure, CMultiLanguageCache * pmlc, CWireEncoding * pwe, LPITEMIDLIST * ppidlFtpItem)
{
    FTP_FIND_DATA wfd;
    HRESULT hr = InternetFindNextFileWrap(hConnect, fAssertOnFailure, &wfd);
    
    *ppidlFtpItem = NULL;

    if (SUCCEEDED(hr))
    {
        ASSERT(pmlc);   // We use this often enought that this might as well exist.
        // Skip "." and ".." entries.
        if (!(wfd.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY) || IS_VALID_FILE(wfd.cFileName))
            hr = pwe->CreateFtpItemID(pmlc, &wfd, ppidlFtpItem);
        else
            hr = InternetFindNextFilePidlWrap(hConnect, fAssertOnFailure, pmlc, pwe, ppidlFtpItem);
    }

    return hr;
}

HRESULT FtpRenameFilePidlWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPCITEMIDLIST pidlExisting, LPCITEMIDLIST pidlNew)
{
    return FtpRenameFileWrap(hConnect, fAssertOnFailure, FtpPidl_GetLastItemWireName(pidlExisting), FtpPidl_GetLastItemWireName(pidlNew));
}


HRESULT FtpGetFileExPidlWrap(HINTERNET hConnect, BOOL fAssertOnFailure, LPCITEMIDLIST pidlFtpPath/*Src*/, LPCWSTR pwzFilePath/*Dest*/, BOOL fFailIfExists,
                       DWORD dwFlagsAndAttributes, DWORD dwFlags, DWORD_PTR dwContext)
{
    return FtpGetFileExWrap(hConnect, fAssertOnFailure, FtpPidl_GetLastItemWireName(pidlFtpPath), pwzFilePath, fFailIfExists, dwFlagsAndAttributes, dwFlags, dwContext);
}


