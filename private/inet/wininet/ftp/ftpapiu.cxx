/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    ftpapiu.cxx

Abstract:

    Common sub-API level FTP functions (created from dll\parseurl.c)

    Contents:
        ParseFtpUrl

Author:

    Richard L Firth (rfirth) 31-May-1995

Environment:

    Win32 user-level DLL

Revision History:

    31-May-1995 rfirth
        Created

--*/

#include <wininetp.h>
#include "ftpapih.h"

//  because wininet doesnt know IStream
#define NO_SHLWAPI_STREAM
#include <shlwapi.h>
#include <shlwapip.h>

//
// functions
//


DWORD
ParseFtpUrl(
    IN OUT LPHINTERNET lphInternet,
    IN LPSTR Url,
    IN DWORD SchemeLength,
    IN LPSTR Headers,
    IN DWORD HeadersLength,
    IN DWORD OpenFlags,
    IN DWORD_PTR Context
    )

/*++

Routine Description:

    URL parser for FTP URLs. Support function for InternetOpenUrl() and
    ParseUrl().

    This is a macro function that just cracks the URL and calls FTP APIs to
    do the work

Arguments:

    lphInternet     - IN: pointer to InternetOpen handle
                      OUT: if successful handle of opened item, else undefined

    Url             - pointer to string containing FTP URL to open

    SchemeLength    - length of the URL scheme, exluding "://"

    Headers         - unused for FTP

    HeadersLength   - unused for FTP

    OpenFlags       - optional flags for opening a file (cache/no-cache, etc.)

    Context         - app-supplied context value for call-backs

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                    The URL passed in could not be parsed

--*/

{
    DEBUG_ENTER((DBG_FTP,
                 Dword,
                 "ParseFtpUrl",
                 "%#x [%#x], %q, %d, %#x, %d, %#x, %#x",
                 lphInternet,
                 *lphInternet,
                 Url,
                 SchemeLength,
                 Headers,
                 HeadersLength,
                 OpenFlags,
                 Context
                 ));

    UNREFERENCED_PARAMETER(Headers);
    UNREFERENCED_PARAMETER(HeadersLength);

    //
    // parse out the name[:password] and host[:port] parts
    //

    DWORD urlLength;
    LPSTR pUserName;
    DWORD userNameLength;
    LPSTR pPassword;
    DWORD passwordLength;
    LPSTR pHostName;
    DWORD hostNameLength;
    INTERNET_PORT port;
    LPSTR lpszUrl = NULL, lpszBackup = NULL;
    char firstUrlPathCharacter;

    HINTERNET hConnect = NULL;
    DWORD error;

    // The passed in Url string gets munged during this function.
    // Make a copy, so the proper URL is set to the mapped connection handle.
    lpszUrl = NewString((LPCSTR)Url);
    if (lpszUrl == NULL)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    lpszBackup = lpszUrl;

    lpszUrl += SchemeLength + sizeof("://") - 1;

    error = GetUrlAddress(&lpszUrl,
                          &urlLength,
                          &pUserName,
                          &userNameLength,
                          &pPassword,
                          &passwordLength,
                          &pHostName,
                          &hostNameLength,
                          &port,
                          NULL
                          );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // we can safely zero-terminate the address parts - the '/' between address
    // info and url-path is not significant
    //

    //if (*Url == '/') {
    //    ++Url;
    //    --urlLength;
    //}
    if (pUserName != NULL) {
        pUserName[userNameLength] = '\0';
    }
    if (pPassword != NULL) {
        pPassword[passwordLength] = '\0';
    }

    //
    // now get the FTP file/directory information
    //

    BOOL isDirectory;

    if ((*lpszUrl == '\0') || (*(lpszUrl + 1) == '\0')) {

        //
        // if the URL just consisted of ftp://host then by default we are
        // referencing an FTP directory (the root directory)
        //

        isDirectory = TRUE;
    } else {

        LPSTR pSemiColon;

        pSemiColon = strchr(lpszUrl, ';');
        if (pSemiColon != NULL) {

            //
            // if there's not enough space left in the string after ';' for the
            // "type=?" substring, then assume this URL is bad
            //

            if ((urlLength - (pSemiColon - lpszUrl)) < 6) {
                error = ERROR_INTERNET_INVALID_URL;
                goto quit;
            }
            if (strnicmp(pSemiColon + 1, "type=", 5) == 0) {
                switch (tolower(*(pSemiColon + 6))) {
                case 'a':
                    OpenFlags |= FTP_TRANSFER_TYPE_ASCII;
                    isDirectory = FALSE;
                    break;

                case 'i':
                    OpenFlags |= FTP_TRANSFER_TYPE_BINARY;
                    isDirectory = FALSE;
                    break;

                case 'd':
                    isDirectory = TRUE;
                    break;

                default:
                    error = ERROR_INTERNET_INVALID_URL;
                    goto quit;
                }
            } else {

                //
                // found a ';', but not "type=". Don't understand this URL
                //

                error = ERROR_INTERNET_INVALID_URL;
                goto quit;
            }
            urlLength = (DWORD) (pSemiColon - lpszUrl);
        } else {

            //
            // there is no ;type= field to help us out. If the string ends in /
            // then it is a directory. Further, if the url-path refers to a
            // file, we don't know which mode to use to transfer it - ASCII or
            // BINARY. We'll default to binary
            //

            if (lpszUrl[urlLength - 1] == '/') {
                isDirectory = TRUE;
            } else {
                OpenFlags |= FTP_TRANSFER_TYPE_BINARY;
                isDirectory = FALSE;
            }
        }

        //
        // decode the url-path
        //
        if(FAILED(UrlUnescapeInPlace(lpszUrl, 0))){
            goto quit;
        }
        urlLength = lstrlen(lpszUrl);
    }

    //
    // we potentially need to go round this loop 3 times:
    //
    //  1. try to get the item from the cache
    //  2. try to get the item from the origin server
    //  3. only if we got an existing connect & the origin server request
    //     failed, reopen the connect handle & try step 2 again
    //
    // however, we only need make one attempt if we're in OFFLINE mode - either
    // we can get the item from the cache, or we can't
    //

    HINTERNET hInternetMapped;

    //
    // BUGBUG - this function should receive the handle already mapped
    //

    error = MapHandleToAddress(*lphInternet, (LPVOID *)&hInternetMapped, FALSE);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    INET_ASSERT(hInternetMapped != NULL);

    //
    // if the InternetOpen() handle was created in OFFLINE mode then is an
    // offline request
    //
        DEBUG_PRINT(FTP,
                    INFO,
                    ("ParseFtpUrl: pre-OpenFlags check: OpenFlags = %#x\n",
                    OpenFlags
                    ));

    OpenFlags |= ((INTERNET_HANDLE_OBJECT *)hInternetMapped)->GetInternetOpenFlags()
        & INTERNET_FLAG_OFFLINE;

        DEBUG_PRINT(FTP,
                    INFO,
                    ("ParseFtpUrl: post-OpenFlags check: OpenFlags = %#x\n",
                    OpenFlags
                    ));


    DereferenceObject((LPVOID)hInternetMapped);

    DWORD limit;

    limit = (OpenFlags & INTERNET_FLAG_OFFLINE) ? 1 : 3;

    //
    // resynchronize same as reload for FTP
    //

    if (OpenFlags & INTERNET_FLAG_RESYNCHRONIZE) {
        OpenFlags |= INTERNET_FLAG_RELOAD;
        DEBUG_PRINT(FTP,
                    INFO,
                    ("ParseFtpUrl: INTERNET_FLAG_RESYNCHRONIZE set\n"));
    }

    DWORD i;
    BOOL bFromCache;

    i = 0;
    bFromCache = (OpenFlags & INTERNET_FLAG_RELOAD || (GlobalUrlCacheSyncMode == WININET_SYNC_MODE_ALWAYS)) ? FALSE : TRUE;

    while (i < limit) {

        //
        // ok, all parts present and correct; open a handle to the FTP resource
        //

        DWORD dwFlags = OpenFlags;

        if (bFromCache) {

            //
            // attempting to get item from cache
            //

            dwFlags |= INTERNET_FLAG_OFFLINE;
        } else {

            //
            // performing net request
            //

            dwFlags |= INTERNET_FLAG_RELOAD;
            dwFlags &= ~INTERNET_FLAG_OFFLINE;
        }

        //
        // zero-terminating the host name will wipe out the first '/' of the
        // URL-path which we must restore before using
        //

        firstUrlPathCharacter = *lpszUrl;
        if (pHostName != NULL) {
            pHostName[hostNameLength] = '\0';
        }

        //
        // record current online/offline state
        //

        BOOL bOffline = IsOffline();

        //
        // create a connect handle object or find an existing one if using
        // INTERNET_FLAG_EXISTING_CONNECT
        //

        if ( hConnect )
        {
            _InternetCloseHandle(hConnect); // nuke old connect handle, otherwise we leak.
        }

        hConnect = InternetConnectA(*lphInternet,
                                    pHostName,
                                    port,
                                    pUserName,
                                    pPassword,
                                    INTERNET_SERVICE_FTP,
                                    dwFlags,

                                    //
                                    // we are creating a "hidden" handle - don't
                                    // tell the app about it
                                    //

                                    INTERNET_NO_CALLBACK
                                    );

        //
        // restore URL-path, but only if its not '\0' - we may have a const
        // string (we can't write to it - we change to "/" below, which is a
        // const string)
        //

        if (*lpszUrl == '\0') {
            *(LPSTR)lpszUrl = firstUrlPathCharacter;
        }

        HINTERNET hConnectMapped = NULL;

        if (hConnect != NULL) {

            //
            // lock the handle by mapping it
            //

            error = MapHandleToAddress(hConnect,
                                       (LPVOID *)&hConnectMapped,
                                       FALSE
                                       );

            INET_ASSERT(error == ERROR_SUCCESS);
            INET_ASSERT(hConnectMapped != NULL);

            if (error != ERROR_SUCCESS) {
                break;
            }

            INTERNET_CONNECT_HANDLE_OBJECT * pConnectMapped;

            pConnectMapped = (INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped;

            //
            // the ref count should be 2: either we created the connect handle
            // or we picked up an EXISTING_CONNECT handle which should not be
            // used by any other requests
            //

            INET_ASSERT(pConnectMapped->ReferenceCount() == 2);

            //
            // first off, associate the last response info, possibly including
            // the server welcome message, with the connection
            //

            pConnectMapped->AttachLastResponseInfo();
            pConnectMapped->SetURL(Url);

            HINTERNET hRequest;

            if (isDirectory) {
                if (*lpszUrl == '\0') {
                    lpszUrl = "/";
                }

                //
                // if we are reading from cache then set the working directory
                // locally, else also set the CWD at the server
                //

                if (bFromCache) {
                    error = pConnectMapped->SetCurrentWorkingDirectory((LPSTR)lpszUrl);
                } else if (FtpSetCurrentDirectory(hConnect, lpszUrl)) {
                    error = ERROR_SUCCESS;
                } else {
                    error = GetLastError();
                }
                if (error == ERROR_SUCCESS) {

                    // if we are not asked to give raw data
                    // then set htmlfind to TRUE

                    if (!(dwFlags & INTERNET_FLAG_RAW_DATA)) {
                        pConnectMapped->SetHtmlFind(TRUE);
                    }
                    hRequest = InternalFtpFindFirstFileA(hConnect,
                                              NULL,
                                              NULL,
                                              dwFlags,
                                              Context,
                                              bFromCache,
                                              pConnectMapped->IsHtmlFind() // allow empty
                                              );
                } else {
                    hRequest = NULL;
                }
            } else /* if (!isDirectory) */ {
                hRequest = InternalFtpOpenFileA(hConnect,
                                                lpszUrl,
                                                GENERIC_READ,
                                                dwFlags,
                                                Context,
                                                bFromCache
                                                );

                //
                // we may have failed because we're not trying to get a file
                // after all - we've been given a directory without a trailing
                // slash
                //

                if (hRequest == NULL) {
                    if (!(dwFlags & INTERNET_FLAG_RAW_DATA)) {
                        pConnectMapped->SetHtmlFind(TRUE);
                    }
                    error = pConnectMapped->SetCurrentWorkingDirectory((LPSTR)lpszUrl);

                    INET_ASSERT(error == ERROR_SUCCESS);

                    if (error == ERROR_SUCCESS) {
                        if (!bFromCache) {
                            if (!FtpSetCurrentDirectory(hConnect, lpszUrl)) {
                                error = GetLastError();
                            }
                        }
                        if (error == ERROR_SUCCESS) {
                            hRequest = InternalFtpFindFirstFileA(
                                            hConnect,
                                            NULL,
                                            NULL,
                                            dwFlags,
                                            Context,
                                            bFromCache,
                                            pConnectMapped->IsHtmlFind()
                                            );
                        }
                    }
                }
            }

            //
            // link the request and connect handles so that the connect handle
            // object will be deleted when the request handle is closed
            //

            if (hRequest != NULL) {

                HINTERNET hRequestMapped = NULL;

                error = MapHandleToAddress(hRequest,
                                           (LPVOID *)&hRequestMapped,
                                           FALSE
                                           );
                if (error == ERROR_SUCCESS) {
                    RSetParentHandle(hRequestMapped, hConnectMapped, TRUE);
                }

                //
                // dereference the handles referenced by MapHandleToAddress()
                //

                if (hRequestMapped != NULL) {
                    DereferenceObject((LPVOID)hRequestMapped);
                }

                //
                // return the request handle
                //

                *lphInternet = hRequest;
            }

            //
            // unmap and dereference the connect handle
            //

            DereferenceObject((LPVOID)hConnectMapped);

            //
            // if we succeeded in opening the item then we're done
            //

            if (hRequest != NULL) {
                break;
            } else {
                error = GetLastError();

                //
                // close the handle without modifying the per-thread handle and
                // context values
                //

                //DWORD closeError = _InternetCloseHandleNoContext(hConnect);
                DWORD closeError = ERROR_SUCCESS;

                INET_ASSERT(closeError == ERROR_SUCCESS);

                //
                // if we failed because we went offline then make a cache
                // request if we can
                //

                if (IsOffline() && !bFromCache) {

                    //
                    // this will be the last chance
                    //

                    bFromCache = TRUE;
                    continue;
                }
            }
        } else {

            //
            // InternetConnect() failed. If the offline state didn't change then
            // its a real error - quit
            //

            error = GetLastError();
            if (IsOffline() == bOffline) {
                break;
            }

            //
            // we must have transitioned offline state. If we went offline then
            // attempt to read from cache only
            //

            if (IsOffline() && !bFromCache) {
                bFromCache = TRUE;
                continue;
            }
        }

        //
        // next iteration - second & subsequent not from cache unless we are
        // offline
        //

        bFromCache = FALSE;
        ++i;
    }

quit:
    if (lpszBackup)
        FREE_MEMORY(lpszBackup);

    DEBUG_LEAVE(error);

    return error;
}
