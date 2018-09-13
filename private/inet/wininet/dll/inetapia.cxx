/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    inetapia.cxx

Abstract:

    Contains the ANSI and character-mode-independent Internet APIs

    Contents:
        InternetCrackUrlA
        InternetCreateUrlA
        InternetCanonicalizeUrlA
        InternetCombineUrlA
        InternetOpenA
        InternetCloseHandle
        _InternetCloseHandle
        _InternetCloseHandleNoContext
        InternetConnectA
        InternetOpenUrlA
        InternetReadFile
        ReadFile_End
        InternetReadFileExA
        InternetWriteFile
        InternetWriteFileExA
        InternetSetFilePointer
        InternetQueryDataAvailable
        InternetFindNextFileA
        InternetQueryOptionA
        InternetSetOptionA
        InternetSetOptionExA
        InternetGetLastResponseInfoA
        InternetSetStatusCallbackA
        //InternetCancelAsyncRequest
        (wInternetCloseConnectA)
        (GetEmailNameAndPassword)
        InternetAttemptConnect
        (CreateDeleteSocket)
        InternetLockRequestFile
        InternetUnlockRequestFile
        InternetCheckConnectionA

Author:

    Richard L Firth (rfirth) 02-Mar-1995

Environment:

    Win32 user-mode DLL

Revision History:

    02-Mar-1995 rfirth
        Created

    07-Mar-1995 madana


--*/


#include <wininetp.h>
#include <perfdiag.hxx>
#include "inetapiu.h"

//  because wininet doesnt know IStream
#define NO_SHLWAPI_STREAM
#include <shlwapi.h>
#include <shlwapip.h>

#include "autodial.h"

//
// public ..?
//

extern "C" {

INTERNETAPI
BOOL
WINAPI
InternetGetCertByURLA(
    IN LPSTR     lpszURL,
    IN OUT LPSTR lpszCertText,
    OUT DWORD    dwcbCertText
    );

}


//
// private manifests
//

//
// private prototypes
//

PRIVATE
DWORD
ReadFile_Fsm(
    IN CFsm_ReadFile * Fsm
    );

PRIVATE
DWORD
ReadFileEx_Fsm(
    IN CFsm_ReadFileEx * Fsm
    );

PRIVATE
VOID
ReadFile_End(
    IN BOOL bDeref,
    IN BOOL bSuccess,
    IN HINTERNET hFileMapped,
    IN DWORD dwBytesRead,
    IN LPVOID lpBuffer OPTIONAL,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead OPTIONAL
    );

PRIVATE
DWORD
QueryAvailable_Fsm(
    IN CFsm_QueryAvailable * Fsm
    );


PRIVATE
DWORD
wInternetCloseConnectA(
    IN HINTERNET lpConnectHandle,
    IN DWORD ServiceType
    );

PRIVATE
DWORD
GetEmailNameAndPassword(
    IN OUT LPSTR* lplpszUserName,
    IN OUT LPSTR* lplpszPassword,
    OUT LPSTR lpszEmailName,
    IN DWORD dwEmailNameLength
    );

PRIVATE
BOOL
InternetParseCommon(
    IN LPCTSTR lpszBaseUrl,
    IN LPCTSTR lpszRelativeUrl,
    OUT LPTSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    );

//PRIVATE
//DWORD
//CreateDeleteSocket(
//    VOID
//    );

BOOL
GetWininetUserName(
    VOID
);



//
// functions
//


INTERNETAPI
BOOL
WINAPI
InternetCrackUrlA(
    IN LPCSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN DWORD dwFlags,
    IN LPURL_COMPONENTSA lpUrlComponents
    )

/*++

Routine Description:

    Cracks an URL into its constituent parts. Optionally escapes the url-path.
    We assume that the user has supplied large enough buffers for the various
    URL parts

Arguments:

    lpszUrl         - pointer to URL to crack

    dwUrlLength     - 0 if lpszUrl is ASCIIZ string, else length of lpszUrl

    dwFlags         - flags controlling operation

    lpUrlComponents - pointer to URL_COMPONENTS

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCrackUrlA",
                     "%q, %#x, %#x, %#x",
                     lpszUrl,
                     dwUrlLength,
                     dwFlags,
                     lpUrlComponents
                     ));

    DWORD error;

    //
    // validate parameters
    //

    if (ARGUMENT_PRESENT(lpszUrl)) {
        if (dwUrlLength == 0) {
            error = ProbeString((LPSTR)lpszUrl, &dwUrlLength);
        } else {
            error = ProbeReadBuffer((LPVOID)lpszUrl, dwUrlLength);
        }
    } else {
        error = ERROR_INVALID_PARAMETER;
    }
    if (error != ERROR_SUCCESS) {
        goto quit;
    }
    if ((lpUrlComponents == NULL)
    || (lpUrlComponents->dwStructSize != sizeof(*lpUrlComponents))) {
        error = ERROR_INVALID_PARAMETER;
    } else {
        error = ProbeWriteBuffer((LPVOID)lpUrlComponents,
                                 sizeof(*lpUrlComponents)
                                 );
    }
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // we only allow two flags for this API
    //

    if (dwFlags & ~(ICU_ESCAPE | ICU_DECODE)) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // get the individual components to return. If they reference a buffer then
    // check it for writeability
    //

    LPSTR lpUrl;
    LPSTR urlCopy;
    INTERNET_SCHEME schemeType;
    LPSTR schemeName;
    DWORD schemeNameLength;
    LPSTR hostName;
    DWORD hostNameLength;
    INTERNET_PORT nPort;
    LPSTR userName;
    DWORD userNameLength;
    LPSTR password;
    DWORD passwordLength;
    LPSTR urlPath;
    DWORD urlPathLength;
    LPSTR extraInfo;
    DWORD extraInfoLength;
    BOOL copyComponent;
    BOOL havePort;

    copyComponent = FALSE;

    schemeName = lpUrlComponents->lpszScheme;
    schemeNameLength = lpUrlComponents->dwSchemeLength;
    if ((schemeName != NULL) && (schemeNameLength != 0)) {
        error = ProbeWriteBuffer((LPVOID)schemeName, schemeNameLength);
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
        *schemeName = '\0';
        copyComponent = TRUE;
    }

    hostName = lpUrlComponents->lpszHostName;
    hostNameLength = lpUrlComponents->dwHostNameLength;
    if ((hostName != NULL) && (hostNameLength != 0)) {
        error = ProbeWriteBuffer((LPVOID)hostName, hostNameLength);
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
        *hostName = '\0';
        copyComponent = TRUE;
    }

    userName = lpUrlComponents->lpszUserName;
    userNameLength = lpUrlComponents->dwUserNameLength;
    if ((userName != NULL) && (userNameLength != 0)) {
        error = ProbeWriteBuffer((LPVOID)userName, userNameLength);
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
        *userName = '\0';
        copyComponent = TRUE;
    }

    password = lpUrlComponents->lpszPassword;
    passwordLength = lpUrlComponents->dwPasswordLength;
    if ((password != NULL) && (passwordLength != 0)) {
        error = ProbeWriteBuffer((LPVOID)password, passwordLength);
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
        *password = '\0';
        copyComponent = TRUE;
    }

    urlPath = lpUrlComponents->lpszUrlPath;
    urlPathLength = lpUrlComponents->dwUrlPathLength;
    if ((urlPath != NULL) && (urlPathLength != 0)) {
        error = ProbeWriteBuffer((LPVOID)urlPath, urlPathLength);
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
        *urlPath = '\0';
        copyComponent = TRUE;
    }

    extraInfo = lpUrlComponents->lpszExtraInfo;
    extraInfoLength = lpUrlComponents->dwExtraInfoLength;
    if ((extraInfo != NULL) && (extraInfoLength != 0)) {
        error = ProbeWriteBuffer((LPVOID)extraInfo, extraInfoLength);
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
        *extraInfo = '\0';
        copyComponent = TRUE;
    }

    //
    // we can only escape or decode the URL if the caller has provided us with
    // buffers to write the escaped strings into
    //

    if (dwFlags & (ICU_ESCAPE | ICU_DECODE)) {
        if (!copyComponent) {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }

        //
        // create a copy of the URL. CrackUrl() will modify this in situ. We
        // need to copy the results back to the user's buffer(s)
        //

        urlCopy = NewString((LPSTR)lpszUrl, dwUrlLength);
        if (urlCopy == NULL) {
            error = ERROR_NOT_ENOUGH_MEMORY;
            goto quit;
        }
        lpUrl = urlCopy;
    } else {
        lpUrl = (LPSTR)lpszUrl;
        urlCopy = NULL;
    }

    //
    // crack the URL into its constituent parts
    //

    error = CrackUrl(lpUrl,
                     dwUrlLength,
                     (dwFlags & ICU_ESCAPE) ? TRUE : FALSE,
                     &schemeType,
                     &schemeName,
                     &schemeNameLength,
                     &hostName,
                     &hostNameLength,
                     &nPort,
                     &userName,
                     &userNameLength,
                     &password,
                     &passwordLength,
                     &urlPath,
                     &urlPathLength,
                     extraInfoLength ? &extraInfo : NULL,
                     extraInfoLength ? &extraInfoLength : 0,
                     &havePort
                     );
    if (error != ERROR_SUCCESS) {
        goto crack_error;
    }

    BOOL copyFailure;

    copyFailure = FALSE;

    //
    // update the URL_COMPONENTS structure based on the results, and what was
    // asked for
    //

    if (lpUrlComponents->lpszScheme != NULL) {
        if (lpUrlComponents->dwSchemeLength > schemeNameLength) {
            memcpy((LPVOID)lpUrlComponents->lpszScheme,
                   (LPVOID)schemeName,
                   schemeNameLength
                   );
            lpUrlComponents->lpszScheme[schemeNameLength] = '\0';
            if (dwFlags & ICU_DECODE) {
                UrlUnescapeInPlace(lpUrlComponents->lpszScheme, 0);
            }
        } else {
            ++schemeNameLength;
            copyFailure = TRUE;
        }
        lpUrlComponents->dwSchemeLength = schemeNameLength;
    } else if (lpUrlComponents->dwSchemeLength != 0) {
        lpUrlComponents->lpszScheme = schemeName;
        lpUrlComponents->dwSchemeLength = schemeNameLength;
    }

    if (lpUrlComponents->lpszHostName != NULL) {
        if (lpUrlComponents->dwHostNameLength > hostNameLength) {
            memcpy((LPVOID)lpUrlComponents->lpszHostName,
                   (LPVOID)hostName,
                   hostNameLength
                   );
            lpUrlComponents->lpszHostName[hostNameLength] = '\0';
            if (dwFlags & ICU_DECODE) {
                UrlUnescapeInPlace(lpUrlComponents->lpszHostName, 0);
            }
        } else {
            ++hostNameLength;
            copyFailure = TRUE;
        }
        lpUrlComponents->dwHostNameLength = hostNameLength;
    } else if (lpUrlComponents->dwHostNameLength != 0) {
        lpUrlComponents->lpszHostName = hostName;
        lpUrlComponents->dwHostNameLength = hostNameLength;
    }

    if (lpUrlComponents->lpszUserName != NULL) {
        if (lpUrlComponents->dwUserNameLength > userNameLength) {
            memcpy((LPVOID)lpUrlComponents->lpszUserName,
                   (LPVOID)userName,
                   userNameLength
                   );
            lpUrlComponents->lpszUserName[userNameLength] = '\0';
            if (dwFlags & ICU_DECODE) {
                UrlUnescapeInPlace(lpUrlComponents->lpszUserName, 0);
            }
        } else {
            ++userNameLength;
            copyFailure = TRUE;
        }
        lpUrlComponents->dwUserNameLength = userNameLength;
    } else if (lpUrlComponents->dwUserNameLength != 0) {
        lpUrlComponents->lpszUserName = userName;
        lpUrlComponents->dwUserNameLength = userNameLength;
    }

    if (lpUrlComponents->lpszPassword != NULL) {
        if (lpUrlComponents->dwPasswordLength > passwordLength) {
            memcpy((LPVOID)lpUrlComponents->lpszPassword,
                   (LPVOID)password,
                   passwordLength
                   );
            lpUrlComponents->lpszPassword[passwordLength] = '\0';
            if (dwFlags & ICU_DECODE) {
                UrlUnescapeInPlace(lpUrlComponents->lpszPassword, 0);
            }
        } else {
            ++passwordLength;
            copyFailure = TRUE;
        }
        lpUrlComponents->dwPasswordLength = passwordLength;
    } else if (lpUrlComponents->dwPasswordLength != 0) {
        lpUrlComponents->lpszPassword = password;
        lpUrlComponents->dwPasswordLength = passwordLength;
    }

    if (lpUrlComponents->lpszUrlPath != NULL) {
        if(schemeType == INTERNET_SCHEME_FILE)
        {
            //
            //  for file: urls we return the path component
            //  as a valid dos path.
            //

            copyFailure = FAILED(PathCreateFromUrl(lpUrl, lpUrlComponents->lpszUrlPath, &(lpUrlComponents->dwUrlPathLength), 0));
        }
        else if (lpUrlComponents->dwUrlPathLength > urlPathLength) {
            memcpy((LPVOID)lpUrlComponents->lpszUrlPath,
                   (LPVOID)urlPath,
                   urlPathLength
                   );
            lpUrlComponents->lpszUrlPath[urlPathLength] = '\0';
            if (dwFlags & ICU_DECODE) {
                UrlUnescapeInPlace(lpUrlComponents->lpszUrlPath, 0);
            }
            lpUrlComponents->dwUrlPathLength = urlPathLength;
        } else {
            ++urlPathLength;
            copyFailure = TRUE;
            lpUrlComponents->dwUrlPathLength = urlPathLength;
        }
    } else if (lpUrlComponents->dwUrlPathLength != 0) {
        lpUrlComponents->lpszUrlPath = urlPath;
        lpUrlComponents->dwUrlPathLength = urlPathLength;
    }

    if (lpUrlComponents->lpszExtraInfo != NULL) {
        if (lpUrlComponents->dwExtraInfoLength > extraInfoLength) {
            memcpy((LPVOID)lpUrlComponents->lpszExtraInfo,
                   (LPVOID)extraInfo,
                   extraInfoLength
                   );
            lpUrlComponents->lpszExtraInfo[extraInfoLength] = '\0';
            if (dwFlags & ICU_DECODE) {
                UrlUnescapeInPlace(lpUrlComponents->lpszExtraInfo, 0);
            }
        } else {
            ++extraInfoLength;
            copyFailure = TRUE;
        }
        lpUrlComponents->dwExtraInfoLength = extraInfoLength;
    } else if (lpUrlComponents->dwExtraInfoLength != 0) {
        lpUrlComponents->lpszExtraInfo = extraInfo;
        lpUrlComponents->dwExtraInfoLength = extraInfoLength;
    }

    //
    // we may have failed to copy one or more components because we didn't have
    // enough buffer space.
    //
    // N.B. Don't change error below here. If need be, move this test lower
    //

    if (copyFailure) {
        error = ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // copy the scheme type
    //

    lpUrlComponents->nScheme = schemeType;

    //
    // convert 0 port (not in URL) to default value for scheme
    //

    if (nPort == INTERNET_INVALID_PORT_NUMBER && !havePort) {
        switch (schemeType) {
        case INTERNET_SCHEME_FTP:
            nPort = INTERNET_DEFAULT_FTP_PORT;
            break;

        case INTERNET_SCHEME_GOPHER:
            nPort = INTERNET_DEFAULT_GOPHER_PORT;
            break;

        case INTERNET_SCHEME_HTTP:
            nPort = INTERNET_DEFAULT_HTTP_PORT;
            break;

        case INTERNET_SCHEME_HTTPS:
            nPort = INTERNET_DEFAULT_HTTPS_PORT;
            break;
        }
    }
    lpUrlComponents->nPort = nPort;

crack_error:

    if (urlCopy != NULL) {
        DEL_STRING(urlCopy);
    }

quit:
    BOOL success = (error==ERROR_SUCCESS);

    if (!success) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);
    return success;
}


INTERNETAPI
BOOL
WINAPI
InternetCreateUrlA(
    IN LPURL_COMPONENTSA lpUrlComponents,
    IN DWORD dwFlags,
    OUT LPSTR lpszUrl OPTIONAL,
    IN OUT LPDWORD lpdwUrlLength
    )

/*++

Routine Description:

    Creates an URL from its constituent parts

Arguments:

    lpUrlComponents - pointer to URL_COMPONENTS structure containing pointers
                      and lengths of components of interest

    dwFlags         - flags controlling function:

                        ICU_ESCAPE  - the components contain characters that
                                      must be escaped in the output URL

    lpszUrl         - pointer to buffer where output URL will be written

    lpdwUrlLength   - IN: number of bytes in lpszUrl buffer
                      OUT: if success, number of characters in lpszUrl, else
                           number of bytes required for buffer

Return Value:

    BOOL
        Success - URL written to lpszUrl

        Failure - call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCreateUrlA",
                     "%#x, %#x, %#x, %#x",
                     lpUrlComponents,
                     dwFlags,
                     lpszUrl,
                     lpdwUrlLength
                     ));

#if INET_DEBUG

    LPSTR lpszUrlOriginal = lpszUrl;

#endif

    DWORD error = ERROR_SUCCESS;
    LPSTR encodedUrlPath = NULL;
    LPSTR encodedExtraInfo = NULL;

    //
    // validate parameters
    //

    if ((lpUrlComponents == NULL)
    || (lpUrlComponents->dwStructSize != sizeof(*lpUrlComponents))
    || (dwFlags & ~(ICU_ESCAPE | ICU_USERNAME))
    || (lpdwUrlLength == NULL)) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    if (!ARGUMENT_PRESENT(lpszUrl)) {
        *lpdwUrlLength = 0;
    }

    //
    // allocate large buffers from heap
    //

    encodedUrlPath = (LPSTR)ALLOCATE_MEMORY(LMEM_FIXED, INTERNET_MAX_URL_LENGTH + 1);
    encodedExtraInfo = (LPSTR)ALLOCATE_MEMORY(LMEM_FIXED, INTERNET_MAX_URL_LENGTH + 1);
    if ((encodedUrlPath == NULL) || (encodedExtraInfo == NULL)) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // if we get an exception, we return ERROR_INVALID_PARAMETER
    //

    __try {

        //
        // get the individual components to copy
        //

        LPSTR schemeName;
        DWORD schemeNameLength;
        DWORD schemeFlags;
        LPSTR hostName;
        DWORD hostNameLength;
        INTERNET_PORT nPort;
        DWORD portLength;
        LPSTR userName;
        DWORD userNameLength;
        LPSTR password;
        DWORD passwordLength;
        LPSTR urlPath;
        DWORD urlPathLength;
        DWORD extraLength;
        DWORD encodedUrlPathLength;
        LPSTR extraInfo;
        DWORD extraInfoLength;
        DWORD encodedExtraInfoLength;
        LPSTR schemeSep;
        DWORD schemeSepLength;
        INTERNET_SCHEME schemeType;
        INTERNET_PORT defaultPort;

        //
        // if the scheme name is absent then we use the default
        //

        schemeName = lpUrlComponents->lpszScheme;
        schemeType = lpUrlComponents->nScheme;

        if (schemeName == NULL) {
            if (schemeType == INTERNET_SCHEME_DEFAULT){
                schemeName = DEFAULT_URL_SCHEME_NAME;
                schemeNameLength = sizeof(DEFAULT_URL_SCHEME_NAME) - 1;
            }
            else {
                schemeName = MapUrlScheme(schemeType, &schemeNameLength);
            }
        } else {
            schemeNameLength = lpUrlComponents->dwSchemeLength;
            if (schemeNameLength == 0) {
                schemeNameLength = lstrlen(schemeName);
            }
        }

        //
        // doesn't have to be a host name
        //

        hostName = lpUrlComponents->lpszHostName;
        portLength = 0;
        if (hostName != NULL) {
            hostNameLength = lpUrlComponents->dwHostNameLength;
            if (hostNameLength == 0) {
                hostNameLength = lstrlen(hostName);
            }

        //
        // if the port is default then we don't add it to the URL, else we need to
        // copy it as a string
        //
        // there won't be a port unless there's host.

            schemeType = MapUrlSchemeName(schemeName, schemeNameLength ? schemeNameLength : -1);
            switch (schemeType) {
            case INTERNET_SCHEME_FTP:
                defaultPort = INTERNET_DEFAULT_FTP_PORT;
                break;

            case INTERNET_SCHEME_GOPHER:
                defaultPort = INTERNET_DEFAULT_GOPHER_PORT;
                break;

            case INTERNET_SCHEME_HTTP:
                defaultPort = INTERNET_DEFAULT_HTTP_PORT;
                break;

            case INTERNET_SCHEME_HTTPS:
                defaultPort = INTERNET_DEFAULT_HTTPS_PORT;
                break;

            default:
                defaultPort = INTERNET_INVALID_PORT_NUMBER;
                break;
            }

            if (lpUrlComponents->nPort != defaultPort) {

                INTERNET_PORT divisor;

                nPort = lpUrlComponents->nPort;
                if (nPort) {
                    divisor = 10000;
                    portLength = 6; // max is 5 characters, plus 1 for ':'
                    while ((nPort / divisor) == 0) {
                        --portLength;
                        divisor /= 10;
                    }
                } else {
                    portLength = 2;         // port is ":0"
                }
            }
        } else {
            hostNameLength = 0;
        }


        //
        // doesn't have to be a user name
        //

        userName = lpUrlComponents->lpszUserName;
        if (userName != NULL) {
            userNameLength = lpUrlComponents->dwUserNameLength;
            if (userNameLength == 0) {
                userNameLength = lstrlen(userName);
            }
        } else {

            //
            // BUGBUG - if ICU_USERNAME then we get the value from the registry
            //

            userNameLength = 0;
        }

        //
        // doesn't have to be a password
        //

        password = lpUrlComponents->lpszPassword;
        if (password != NULL) {
            passwordLength = lpUrlComponents->dwPasswordLength;
            if (passwordLength == 0) {
                passwordLength = lstrlen(password);
            }
        } else {

            //
            // BUGBUG - if ICU_USERNAME then we get the value from the registry
            //

            passwordLength = 0;
        }

        //
        // but if there's a password without a user name, then its an error
        //

        if (password && !userName) {
            error = ERROR_INVALID_PARAMETER;
        } else {

            //
            // determine the scheme type for possible uses below
            //

            schemeFlags = 0;
            if (strnicmp(schemeName, "http", schemeNameLength) == 0) {
                schemeFlags = SCHEME_HTTP;
            } else if (strnicmp(schemeName, "ftp", schemeNameLength) == 0) {
                schemeFlags = SCHEME_FTP;
            } else if (strnicmp(schemeName, "gopher", schemeNameLength) == 0) {
                schemeFlags = SCHEME_GOPHER;
            }

            //
            // doesn't have to be an URL-path. Empty string is default
            //

            urlPath = lpUrlComponents->lpszUrlPath;
            if (urlPath != NULL) {
                urlPathLength = lpUrlComponents->dwUrlPathLength;
                if (urlPathLength == 0) {
                    urlPathLength = lstrlen(urlPath);
                }
                if ((*urlPath != '/') && (*urlPath != '\\')) {
                    extraLength = 1;
                } else {
                    extraLength = 0;
                }

                //
                // if requested, we will encode the URL-path
                //

                if (dwFlags & ICU_ESCAPE) {

                    //
                    // only encode the URL-path if it's a recognized scheme
                    //

                    if (schemeFlags != 0) {
                        encodedUrlPathLength = INTERNET_MAX_URL_LENGTH + 1;
                        error = EncodeUrlPath(NO_ENCODE_PATH_SEP,
                                              schemeFlags,
                                              urlPath,
                                              urlPathLength,
                                              encodedUrlPath,
                                              &encodedUrlPathLength
                                              );
                        if (error == ERROR_SUCCESS) {
                            urlPath = encodedUrlPath;
                            urlPathLength = encodedUrlPathLength;
                        }
                    }
                }
            } else {
                urlPathLength = 0;
                extraLength = 0;
            }

            //
            // handle extra info if present
            //

            if (error == ERROR_SUCCESS) {
                extraInfo = lpUrlComponents->lpszExtraInfo;
                if (extraInfo != NULL) {
                    extraInfoLength = lpUrlComponents->dwExtraInfoLength;
                    if (extraInfoLength == 0) {
                        extraInfoLength = lstrlen(extraInfo);
                    }

                    //
                    // if requested, we will encode the extra info
                    //

                    if (dwFlags & ICU_ESCAPE) {

                        //
                        // only encode the extra info if it's a recognized scheme
                        //

                        if (schemeFlags != 0) {
                            encodedExtraInfoLength = INTERNET_MAX_URL_LENGTH + 1;
                            error = EncodeUrlPath(0,
                                                  schemeFlags,
                                                  extraInfo,
                                                  extraInfoLength,
                                                  encodedExtraInfo,
                                                  &encodedExtraInfoLength
                                                  );
                            if (error == ERROR_SUCCESS) {
                                extraInfo = encodedExtraInfo;
                                extraInfoLength = encodedExtraInfoLength;
                            }
                        }
                    }
                } else {
                    extraInfoLength = 0;
                }
            }

            DWORD requiredSize;

            if (error == ERROR_SUCCESS) {

                //
                // Determine if we have a protocol scheme that requires slashes
                //

                if (DoesSchemeRequireSlashes(schemeName, schemeNameLength, (hostName != NULL))
                || ((schemeType == INTERNET_SCHEME_NEWS)
                && urlPath
                && (strchr(urlPath, '/') || strchr(urlPath, '\\')))) {
                    schemeSep = "://";
                    schemeSepLength = sizeof("://") - 1;
                } else {
                    schemeSep = ":";
                    schemeSepLength = sizeof(":") - 1;
                }

                //
                // ensure we have enough buffer space
                //

                requiredSize = schemeNameLength
                             + schemeSepLength
                             + hostNameLength
                             + portLength
                             + (userName ? userNameLength + 1 : 0) // +1 for '@'
                             + (password ? passwordLength + 1 : 0) // +1 for ':'
                             + urlPathLength
                             + extraLength
                             + extraInfoLength
                             + 1                                // +1 for '\0'
                             ;

                //
                // if there is enough buffer, copy the URL
                //

                if (*lpdwUrlLength >= requiredSize) {
                    memcpy((LPVOID)lpszUrl, (LPVOID)schemeName, schemeNameLength);
                    lpszUrl += schemeNameLength;
                    memcpy((LPVOID)lpszUrl, (LPVOID)schemeSep, schemeSepLength);
                    lpszUrl += schemeSepLength;
                    if (userName) {
                        memcpy((LPVOID)lpszUrl, (LPVOID)userName, userNameLength);
                        lpszUrl += userNameLength;
                        if (password) {
                            *lpszUrl++ = ':';
                            memcpy((LPVOID)lpszUrl, (LPVOID)password, passwordLength);
                            lpszUrl += passwordLength;
                        }
                        *lpszUrl++ = '@';
                    }
                    if (hostName) {
                        memcpy((LPVOID)lpszUrl, (LPVOID)hostName, hostNameLength);
                        lpszUrl += hostNameLength;

                        // We won't attach a port unless there's a host to go with it.
                        if (portLength) {
                            lpszUrl += wsprintf(lpszUrl, ":%d", nPort & 0xffff);
                        }

                    }
                    if (urlPath) {

                        //
                        // Only do extraLength if we've actually copied something
                        // after the scheme.  Also, don't copy slash if it's
                        // mailto:
                        //

                        if (extraLength != 0 && (userName || hostName || portLength) &&
                            schemeType != INTERNET_SCHEME_MAILTO) {
                            *lpszUrl++ = '/';
                        } else if (extraLength != 0) {
                            --requiredSize;
                        }
                        memcpy((LPVOID)lpszUrl, (LPVOID)urlPath, urlPathLength);
                        lpszUrl += urlPathLength;
                    } else if (extraLength != 0) {
                        --requiredSize;
                    }
                    if (extraInfo) {
                        memcpy((LPVOID)lpszUrl, (LPVOID)extraInfo, extraInfoLength);
                        lpszUrl += extraInfoLength;
                    }

                    //
                    // terminate string
                    //

                    *lpszUrl = '\0';

                    //
                    // -1 for terminating '\0'
                    //

                    --requiredSize;
                } else {

                    //
                    // not enough buffer space - just return the required buffer length
                    //

                    error = ERROR_INSUFFICIENT_BUFFER;
                }
            }

            //
            // update returned parameters
            //

            *lpdwUrlLength = requiredSize;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        error = ERROR_INVALID_PARAMETER;
    }
    ENDEXCEPT
quit:

    //
    // clear up the buffers we allocated
    //


    if (encodedUrlPath != NULL) {
        FREE_MEMORY(encodedUrlPath);
    }
    if (encodedExtraInfo != NULL) {
        FREE_MEMORY(encodedExtraInfo);
    }

    BOOL success = (error==ERROR_SUCCESS);

    if (success) {

        DEBUG_PRINT_API(API,
                        INFO,
                        ("URL = %q\n",
                        lpszUrlOriginal
                        ));
    } else {

        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);
    return success;
}

//
//  ICUHrToWin32Error() is specifically for converting the return codes for
//  Url* APIs in shlwapi into win32 errors.
//  WARNING:  it should not be used for any other purpose.
//
DWORD
ICUHrToWin32Error(HRESULT hr)
{
    DWORD err = ERROR_INVALID_PARAMETER;
    switch(hr)
    {
    case E_OUTOFMEMORY:
        err = ERROR_NOT_ENOUGH_MEMORY;
        break;

    case E_POINTER:
        err = ERROR_INSUFFICIENT_BUFFER;
        break;

    case S_OK:
        err = ERROR_SUCCESS;
        break;

    default:
        break;
    }
    return err;
}



INTERNETAPI
BOOL
WINAPI
InternetCanonicalizeUrlA(
    IN LPCSTR lpszUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Combines a relative URL with a base URL to form a new full URL.

Arguments:

    lpszUrl             - pointer to URL to be canonicalize
    lpszBuffer          - pointer to buffer where new URL is written
    lpdwBufferLength    - size of buffer on entry, length of new URL on exit
    dwFlags             - flags controlling operation

Return Value:

    BOOL                - TRUE if successful, FALSE if not

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCanonicalizeUrlA",
                     "%q, %#x, %#x [%d], %#x",
                     lpszUrl,
                     lpszBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0,
                     dwFlags
                     ));

    HRESULT hr ;
    BOOL bRet = TRUE;;

    INET_ASSERT(lpszUrl);
    INET_ASSERT(lpszBuffer);
    INET_ASSERT(lpdwBufferLength && (*lpdwBufferLength > 0));

    //
    //  the flags for the Url* APIs in shlwapi should be the same
    //  except that NO_ENCODE is on by default.  so we need to flip it
    //
    dwFlags ^= ICU_NO_ENCODE;

    // Check for invalid parameters

    if (!lpszUrl || !lpszBuffer || !lpdwBufferLength || *lpdwBufferLength == 0)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = UrlCanonicalizeA(lpszUrl, lpszBuffer,
                    lpdwBufferLength, dwFlags | URL_WININET_COMPATIBILITY);
    }

    if(FAILED(hr))
    {
        DWORD dw = ICUHrToWin32Error(hr);

        bRet = FALSE;

        DEBUG_ERROR(API, dw);

        SetLastError(dw);
    }

    DEBUG_LEAVE_API(bRet);

    return bRet;
}


INTERNETAPI
BOOL
WINAPI
InternetCombineUrlA(
    IN LPCSTR lpszBaseUrl,
    IN LPCSTR lpszRelativeUrl,
    OUT LPSTR lpszBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Combines a relative URL with a base URL to form a new full URL.

Arguments:

    lpszBaseUrl         - pointer to base URL
    lpszRelativeUrl     - pointer to relative URL
    lpszBuffer          - pointer to buffer where new URL is written
    lpdwBufferLength    - size of buffer on entry, length of new URL on exit
    dwFlags             - flags controlling operation

Return Value:

    BOOL                - TRUE if successful, FALSE if not

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCombineUrlA",
                     "%q, %q, %#x, %#x [%d], %#x",
                     lpszBaseUrl,
                     lpszRelativeUrl,
                     lpszBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0,
                     dwFlags
                     ));

    HRESULT hr ;
    BOOL bRet;

    INET_ASSERT(lpszBaseUrl);
    INET_ASSERT(lpszRelativeUrl);
    INET_ASSERT(lpdwBufferLength);

    //
    //  the flags for the Url* APIs in shlwapi should be the same
    //  except that NO_ENCODE is on by default.  so we need to flip it
    //
    dwFlags ^= ICU_NO_ENCODE;

    // Check for invalid parameters

    if (!lpszBaseUrl || !lpszRelativeUrl || !lpdwBufferLength)
    {
        hr = E_INVALIDARG;
    }
    else
    {
        hr = UrlCombineA(lpszBaseUrl, lpszRelativeUrl, lpszBuffer,
                    lpdwBufferLength, dwFlags | URL_WININET_COMPATIBILITY);
    }

    if(FAILED(hr))
    {
        DWORD dw = ICUHrToWin32Error(hr);

        bRet = FALSE;

        DEBUG_ERROR(API, dw);

        SetLastError(dw);
    }
    else
        bRet = TRUE;

    IF_DEBUG_CODE() {
        if (bRet) {
            DEBUG_PRINT_API(API,
                            INFO,
                            ("URL = %q\n",
                            lpszBuffer
                            ));
        }
    }

    DEBUG_LEAVE_API(bRet);

    return bRet;
}



INTERNETAPI
HINTERNET
WINAPI
InternetOpenA(
    IN LPCSTR lpszAgent,
    IN DWORD dwAccessType,
    IN LPCSTR lpszProxy OPTIONAL,
    IN LPCSTR lpszProxyBypass OPTIONAL,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Opens a root Internet handle from which all HINTERNET objects are derived

Arguments:

    lpszAgent       - name of the application making the request (arbitrary
                      identifying string). Used in "User-Agent" header when
                      communicating with HTTP servers, if the application does
                      not add a User-Agent header of its own

    dwAccessType    - type of access required. Can be

                        INTERNET_OPEN_TYPE_PRECONFIG
                            - Gets the configuration from the registry

                        INTERNET_OPEN_TYPE_DIRECT
                            - Requests are made directly to the nominated server

                        INTERNET_OPEN_TYPE_PROXY
                            - Requests are made via the nominated proxy

                        INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY
                            - Like Pre-Config, but prevents JavaScript, INS
                                and other auto-proxy types from being used.

    lpszProxy       - if INTERNET_OPEN_TYPE_PROXY, a list of proxy servers to
                      use

    lpszProxyBypass - if INTERNET_OPEN_TYPE_PROXY, a list of servers which we
                      will communicate with directly

    dwFlags         - flags to control the operation of this API or potentially
                      all APIs called on the handle generated by this API.
                      Currently supported are:

                        INTERNET_FLAG_ASYNC
                            - if specified then all subsequent API calls made
                              against the handle returned from this API, or
                              handles descended from the handle returned by
                              this API, have the opportunity to complete
                              asynchronously, depending on other factors
                              relevant at the time the API is called

Return Value:

    HINTERNET
        Success - handle of Internet object

        Failure - NULL. For more information, call GetLastError()

--*/

{
    PERF_INIT();

    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetOpenA",
                     "%q, %s (%d), %q, %q, %#x",
                     lpszAgent,
                     InternetMapOpenType(dwAccessType),
                     dwAccessType,
                     lpszProxy,
                     lpszProxyBypass,
                     dwFlags
                     ));

    DWORD error;
    HINTERNET hInternet = NULL;

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    //
    // we are doing GetUserName here instead of in DLL_PROCESS_ATTACH
    // As every caller of wininet has to do this first, we ensure
    // that the username is initialized when they get to actually doing
    // any real operation
    //

    GetWininetUserName();

    //
    // validate parameters
    //

    if (!
         (
              (dwAccessType == INTERNET_OPEN_TYPE_DIRECT)
           || (dwAccessType == INTERNET_OPEN_TYPE_PROXY)
           || (dwAccessType == INTERNET_OPEN_TYPE_PRECONFIG)
           || (dwAccessType == INTERNET_OPEN_TYPE_PRECONFIG_WITH_NO_AUTOPROXY)
           || (
                (dwAccessType == INTERNET_OPEN_TYPE_PROXY)
                &&
                    (
                       !ARGUMENT_PRESENT(lpszProxy)
                    || (*lpszProxy == '\0')

                    )
              )
           || (dwFlags & ~INTERNET_FLAGS_MASK)
         )
       )
    {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }


    GlobalHaveInternetOpened = TRUE;

    //
    // Initalize an auto proxy dll if needed,
    //  as long as the caller is allowing us free rein to do this
    //  by calling us with INTERNET_OPEN_TYPE_PRECONFIG.
    //

    //if ( dwAccessType == INTERNET_OPEN_TYPE_PRECONFIG )
    //{
    //    if ( ! InitalizeAutoConfigDllIfNeeded() )
    //  {
    //      error = GetLastError();
    //
    //      INET_ASSERT(error != ERROR_SUCCESS);
    //
    //      goto quit;
    //  }
    //
    //


    INTERNET_HANDLE_OBJECT * lpInternet;

    lpInternet = new INTERNET_HANDLE_OBJECT(lpszAgent,
                                            dwAccessType,
                                            (LPSTR)lpszProxy,
                                            (LPSTR)lpszProxyBypass,
                                            dwFlags
                                            );
    if (lpInternet == NULL) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }
    error = lpInternet->GetStatus();
    if (error == ERROR_SUCCESS) {
        hInternet = (HINTERNET)lpInternet;

        //
        // success - don't return the object address, return the pseudo-handle
        // value we generated
        //

        hInternet = ((HANDLE_OBJECT *)hInternet)->GetPseudoHandle();

        //
        // start async support now if required. If we can't start it, we'll get
        // another chance the next time we create an async request
        //

        if (dwFlags & INTERNET_FLAG_ASYNC) {
            InitializeAsyncSupport();
        }
    } else {

        //
        // hack fix to stop InternetIndicateStatus (called from the handle
        // object destructor) blowing up if there is no handle object in the
        // thread info block. We can't call back anyway
        //

        LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

        if (lpThreadInfo) {

            //
            // BUGBUG - incorrect handle value
            //

            _InternetSetObjectHandle(lpThreadInfo, lpInternet, lpInternet);
        }

        //
        // we failed during initialization. Kill the handle using Dereference()
        // (in order to stop the debug version complaining about the reference
        // count not being 0. Invalidate for same reason)
        //

        lpInternet->Invalidate();
        lpInternet->Dereference();

        INET_ASSERT(hInternet == NULL);

    }

quit:

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(API, error);

        SetLastError(error);
    }

    DEBUG_LEAVE_API(hInternet);

    return hInternet;
}


INTERNETAPI
BOOL
WINAPI
InternetCloseHandle(
    IN HINTERNET hInternet
    )

/*++

Routine Description:

    Closes any open internet handle object

Arguments:

    hInternet   - handle of internet object to close

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. For more information call GetLastError()

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCloseHandle",
                     "%#x",
                     hInternet
                     ));

    PERF_ENTER(InternetCloseHandle);

    DWORD error;
    BOOL success = FALSE;
    HINTERNET hInternetMapped = NULL;
    static DWORD ticks = GetTickCount();

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    //
    // map the handle. Don't invalidate it (_InternetCloseHandle() does this)
    //

    error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, FALSE);
    if (error != ERROR_SUCCESS) {
        if (hInternetMapped == NULL) {

            //
            // the handle never existed or has been completely destroyed
            //

            DEBUG_PRINT(API,
                        ERROR,
                        ("Handle %#x is invalid\n",
                        hInternet
                        ));

            //
            // catch invalid handles - may help caller
            //

            DEBUG_BREAK(INVALID_HANDLES);

        } else {

            //
            // this handle is already being closed (it's invalidated). We only
            // need one InternetCloseHandle() operation to invalidate the handle.
            // All other threads will simply dereference the handle, and
            // eventually it will be destroyed
            //

            DereferenceObject((LPVOID)hInternetMapped);
        }
        goto quit;
    }

    //
    // the handle is not invalidated
    //

    HANDLE_OBJECT * pHandle;

    pHandle = (HANDLE_OBJECT *)hInternetMapped;

    if ( ! ((INTERNET_HANDLE_OBJECT *)hInternetMapped)->IsAsyncHandle() )
    {
        if (GetTickCount() - ticks >= 5000)
        {
            PurgeServerInfoList(FALSE);

            InterlockedExchange((LPLONG) &ticks, (LONG) GetTickCount());
        }
    }

    DEBUG_PRINT(INET,
                INFO,
                ("handle %#x == %#x == %s\n",
                hInternet,
                hInternetMapped,
                InternetMapHandleType(pHandle->GetHandleType())
                ));

    //
    // if this is an http request handle, notify all filters.
    //

    if (pHandle->GetHandleType() == TypeHttpRequestHandle) {
        HttpFiltOnTransactionComplete (hInternet);
    }

    //
    // if this is a delete-parent-with-child subtree then find the root node
    //

    while (pHandle->GetDeleteWithChild()) {

        HINTERNET handleObject;

        handleObject = pHandle->GetParent();

        INET_ASSERT(handleObject != NULL);

        //
        // remove the delete-parent-with-child indication, or we'll get stuck
        // in a loop
        //

        pHandle->SetParent(handleObject, FALSE);

        //
        // if the parent handle is an EXISTING_CONNECT connect handle then we
        // just mark it unused & close the current handle
        //

        HINTERNET_HANDLE_TYPE handleType;

        handleType = ((HANDLE_OBJECT *)handleObject)->GetHandleType();
        if ((handleType == TypeFtpConnectHandle)
        || (handleType == TypeGopherConnectHandle)
        || (handleType == TypeHttpConnectHandle)) {

            INTERNET_CONNECT_HANDLE_OBJECT * pConnect;

            pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)handleObject;

            //
            // SetUnused() will only operate on a connect handle object that
            // has been created with INTERNET_FLAG_EXISTING_CONNECT
            //

            if (pConnect->SetUnused()) {

                //
                // only handle type should be FTP connect handle for now
                //

                INET_ASSERT(handleType == TypeFtpConnectHandle);

                DEBUG_PRINT(INET,
                            INFO,
                            ("caching unused %s connect handle object %#x. RefCount = %d\n",
                            (handleType == TypeFtpConnectHandle)
                                ? "FTP"
                                : (handleType == TypeGopherConnectHandle)
                                    ? "Gopher"
                                    : "HTTP",
                            ((HANDLE_OBJECT *)handleObject)->GetPseudoHandle(),
                            ((HANDLE_OBJECT *)handleObject)->ReferenceCount()
                            ));
                break;
            }
        }
        pHandle = (HANDLE_OBJECT *)handleObject;
        hInternet = pHandle->GetPseudoHandle();
    }

    //
    // close all child handles first
    //

    while (pHandle->HaveChildren()) {

        //
        // we'll fall out at the first error we get. It *should* mean that this
        // handle (and its descendents) is already being closed
        //

        if (!InternetCloseHandle(pHandle->NextChild())) {
            break;
        }
    }

    //
    // clear the handle object last error variables
    //

    InternetClearLastError();


    //
    // remove the reference added by MapHandleToAddress(), or the handle won't
    // be destroyed by _InternetCloseHandle()
    //

    DereferenceObject((LPVOID)hInternetMapped);

    //
    // use _InternetCloseHandle() to do the work
    //

    success = _InternetCloseHandle(hInternet);

quit:

    // SetLastError must be called after PERF_LEAVE !!!
    PERF_LEAVE(InternetCloseHandle);

    if (error != ERROR_SUCCESS) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);

    return success;
}


BOOL
_InternetCloseHandle(
    IN HINTERNET hInternet
    )

/*++

Routine Description:

    Same as InternetCloseHandle() except does not clear out the last error text.
    Mainly for FTP

Arguments:

    hInternet   - handle of internet object to close

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. For more information call GetLastError()

--*/

{
    DEBUG_ENTER((DBG_INET,
                 Bool,
                 "_InternetCloseHandle",
                 "%#x",
                 hInternet
                 ));

    DWORD error;
    BOOL success;
    HINTERNET hInternetMapped = NULL;
    LPINTERNET_THREAD_INFO lpThreadInfo = InternetGetThreadInfo();

    if (lpThreadInfo == NULL) {
        if (InDllCleanup) {
            error = ERROR_INTERNET_SHUTDOWN;
        } else {

            INET_ASSERT(FALSE);

            error = ERROR_INTERNET_INTERNAL_ERROR;
        }
        goto quit;
    }

    //
    // map the handle and invalidate it. This will cause any new requests with
    // the handle as a parameter to fail
    //

    error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, TRUE);
    if (error != ERROR_SUCCESS) {
        if (hInternetMapped != NULL) {

            //
            // the handle is already being closed, or is already deleted
            //

            DereferenceObject((LPVOID)hInternetMapped);
        }

        //
        // since this is the only function that can invalidate a handle, if we
        // are here then the handle is just waiting for its refcount to go to
        // zero. We already removed the refcount we added above, so we're in
        // the clear
        //

        goto quit;
    }

    //
    // there may be an active socket operation. We close the socket to abort the
    // operation
    //

    ((INTERNET_HANDLE_OBJECT *)hInternetMapped)->AbortSocket();

    //
    // we need the parent handle - we will set this as the handle object being
    // processed by this thread. This is required for async worker threads (see
    // below)
    //

    HINTERNET hParent;
    HINTERNET hParentMapped;
    DWORD_PTR dwParentContext;

    hParentMapped = ((HANDLE_OBJECT *)hInternetMapped)->GetParent();
    if (hParentMapped != NULL) {
        hParent = ((HANDLE_OBJECT *)hParentMapped)->GetPseudoHandle();
        dwParentContext = ((HANDLE_OBJECT *)hParentMapped)->GetContext();
    }

    //
    // set the object handle and context in the per-thread data structure
    //

    _InternetSetObjectHandle(lpThreadInfo, hInternet, hInternetMapped);
    _InternetSetContext(lpThreadInfo,
                        ((INTERNET_HANDLE_OBJECT *)hInternetMapped)->GetContext()
                        );

    //
    // at this point, there should *always* be at least 2 references on the
    // handle - one added when the object was created, and one added by
    // MapHandleToAddress() above. If the object is still alive after the 2
    // dereferences, then it will be destroyed when the current owning thread
    // dereferences it
    //

    (void)DereferenceObject((LPVOID)hInternetMapped);
    error = DereferenceObject((LPVOID)hInternetMapped);

    //
    // now set the object to be the parent. This is necessary for e.g.
    // FtpGetFile() and async requests (where the async worker thread will make
    // an extra callback to deliver the results of the async request)
    //

    if (hParentMapped != NULL) {
        _InternetSetObjectHandle(lpThreadInfo, hParent, hParentMapped);
        if (dwParentContext != 0) {
            _InternetSetContext(lpThreadInfo, dwParentContext);
        }
    }

    //
    // if the handle was still alive after dereferencing it then we will inform
    // the app that the close is pending
    //

quit:

    //
    // if the handle is still alive then we return success - it is invalidated
    // and will be deleted as soon as possible
    //

    if (error == ERROR_INTERNET_HANDLE_EXISTS) {
        error = ERROR_SUCCESS;
    }
    success = (error==ERROR_SUCCESS);
    if (!success) {
        SetLastError(error);
        DEBUG_ERROR(INET, error);
    }
    DEBUG_LEAVE(success);
    return success;
}


DWORD
_InternetCloseHandleNoContext(
    IN HINTERNET hInternet
    )

/*++

Routine Description:

    Same as _InternetCloseHandle() except does not change the per-thread info
    structure handle/context values

    BUGBUG - This should be handled via a parameter to _InternetCloseHandle(),
             but its close to shipping...

Arguments:

    hInternet   - handle of internet object to close

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_HANDLE

--*/

{
    DEBUG_ENTER((DBG_INET,
                 Bool,
                 "_InternetCloseHandleNoContext",
                 "%#x",
                 hInternet
                 ));

    DWORD error;
    HINTERNET hInternetMapped = NULL;

    //
    // map the handle and invalidate it. This will cause any new requests with
    // the handle as a parameter to fail
    //

    error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, TRUE);
    if (error != ERROR_SUCCESS) {
        if (hInternetMapped != NULL) {

            //
            // the handle is already being closed, or is already deleted
            //

            DereferenceObject((LPVOID)hInternetMapped);
        }

        //
        // since this is the only function that can invalidate a handle, if we
        // are here then the handle is just waiting for its refcount to go to
        // zero. We already removed the refcount we added above, so we're in
        // the clear
        //

        goto quit;
    }

    //
    // there may be an active socket operation. We close the socket to abort the
    // operation
    //

    ((INTERNET_HANDLE_OBJECT *)hInternetMapped)->AbortSocket();

    //
    // at this point, there should *always* be at least 2 references on the
    // handle - one added when the object was created, and one added by
    // MapHandleToAddress() above. If the object is still alive after the 2
    // dereferences, then it will be destroyed when the current owning thread
    // dereferences it
    //

    (void)DereferenceObject((LPVOID)hInternetMapped);
    error = DereferenceObject((LPVOID)hInternetMapped);

quit:

    //
    // if the handle is still alive then we return success - it is invalidated
    // and will be deleted as soon as possible
    //

    if (error == ERROR_INTERNET_HANDLE_EXISTS) {
        error = ERROR_SUCCESS;
    }

    DEBUG_LEAVE(error);

    return error;
}


INTERNETAPI
BOOL
WINAPI
InternetGetCertByURLA(
    IN LPSTR     lpszURL,
    IN OUT LPSTR lpszCertText,
    OUT DWORD    dwcbCertText
    )
/*++

Routine Description:

    Does a high-level lookup against the Certificate Cache.
    Searches by URL (broken down into hostname) for the Certificate,
    and returns it in a formatted (&localized) string.

Arguments:

    lpszUrl         - pointer to URL to crack

    lpszCertText    - Output of formatted certifcate

    dwcbCertText    - Size of lpszCertText

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{

    BOOL fSuccess = FALSE;

/*    LPSTR lpszHostName;
    DWORD dwcbHostName;
    INTERNET_CERTIFICATE_INFO cInfo;
    CHAR chBackup;
    DWORD error = ERROR_SUCCESS;

    ZeroMemory(&cInfo, sizeof(INTERNET_CERTIFICATE_INFO));

    error = CrackUrl(lpszURL,
             lstrlen(lpszURL),
             FALSE,
             NULL,          //  Scheme Type
             NULL,          //  Scheme Name
             NULL,          //  Scheme Length
             &lpszHostName, //  Host Name
             &dwcbHostName, //  Host Length
             NULL,          //  Internet Port
             NULL,          //  UserName
             NULL,          //  UserName Length
             NULL,          //  Password
             NULL,          //  Password Lenth
             NULL,          //  Path
             NULL,          //  Path Length
             NULL,          //  Extra Info
             NULL,          //  Extra Info Length
             NULL
             );


    if ( error != ERROR_SUCCESS)
        goto quit;

    chBackup = lpszHostName[dwcbHostName];
    lpszHostName[dwcbHostName] = '\0';

    fSuccess = GlobalCertCache.GetCert(
                    lpszHostName,
                    &cInfo
                    );


    lpszHostName[dwcbHostName] = chBackup;

    if ( ! fSuccess )
    {
        error = ERROR_INTERNET_INVALID_OPERATION;
        goto quit;
    }

    LPSTR szResult;

    szResult = FormatCertInfo(&cInfo);

    if ( ! szResult )
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    DWORD dwcbResult;

    dwcbResult = lstrlen(szResult);

    if ( dwcbCertText < (dwcbResult+1) )
    {
        error = ERROR_INSUFFICIENT_BUFFER;
        goto quit;
    }

    memcpy(
        lpszCertText,
        szResult,
        (dwcbResult + 1) * sizeof(TCHAR));


quit:

    if (NULL != szResult) {
        FREE_MEMORY(szResult);
    }
    if (NULL != cInfo.lpszSubjectInfo) {
        FREE_MEMORY(cInfo.lpszSubjectInfo);
    }
    if (NULL != cInfo.lpszIssuerInfo) {
        FREE_MEMORY(cInfo.lpszIssuerInfo);
    }
    if (NULL != cInfo.lpszSignatureAlgName) {
        FREE_MEMORY(cInfo.lpszSignatureAlgName);
    }
    if (NULL != cInfo.lpszEncryptionAlgName) {
        FREE_MEMORY(cInfo.lpszEncryptionAlgName);
    }
    if (NULL != cInfo.lpszProtocolName) {
        FREE_MEMORY(cInfo.lpszProtocolName);
    }

    fSuccess = TRUE;

    if ( error != ERROR_SUCCESS )
    {
        fSuccess = FALSE;
        SetLastError(error);
    }*/

    return fSuccess;

}


INTERNETAPI
BOOL
WINAPI
InternetShowSecurityInfoByURLA(
    IN LPSTR     lpszURL,
    IN HWND      hwndRootWindow
    )
/*++

Routine Description:

    Does a high-level lookup against the Certificate Cache.
    Searches by URL (broken down into hostname) for the Certificate,
    and returns it in a formatted (&localized) string.

Arguments:

    lpszUrl         - pointer to URL to crack

    lpszCertText    - Output of formatted certifcate

    dwcbCertText    - Size of lpszCertText

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_INET,
                 Bool,
                 "InternetShowSecurityInfoA",
                 "%q %#x",
                 lpszURL,
                 hwndRootWindow
                 ));

    LPSTR lpszHostName;
    DWORD dwcbHostName;
    INTERNET_SECURITY_INFO cInfo;
    CHAR chBackup;
    DWORD dwFlags;
    DWORD error = ERROR_SUCCESS;
    WCHAR   szTitle[MAX_PATH];
    WCHAR   szMessage[MAX_PATH];
    INTERNET_SCHEME ustSchemeType;
    BOOL fResult = FALSE;

    if (!GlobalDataInitialized) {
        if (GlobalDataInitialize() != ERROR_SUCCESS) {
            goto Cleanup;
        }
    }

    ZeroMemory(&cInfo, sizeof(INTERNET_SECURITY_INFO));

    error = CrackUrl(lpszURL,
             lstrlen(lpszURL),
             FALSE,
             &ustSchemeType,
             NULL,          //  Scheme Name
             NULL,          //  Scheme Length
             &lpszHostName, //  Host Name
             &dwcbHostName, //  Host Length
             NULL,          //  Internet Port
             NULL,          //  UserName
             NULL,          //  UserName Length
             NULL,          //  Password
             NULL,          //  Password Lenth
             NULL,          //  Path
             NULL,          //  Path Length
             NULL,          //  Extra Info
             NULL,          //  Extra Info Length
             NULL
             );


    if ( error != ERROR_SUCCESS)
    {
        goto Cleanup;
    }

    if ( ustSchemeType != INTERNET_SCHEME_HTTPS )
    {
        goto Cleanup;
    }

    if ( lpszHostName == NULL || dwcbHostName == 0 )
    {
        fResult = TRUE;
        goto done;
    }


    chBackup = lpszHostName[dwcbHostName];
    lpszHostName[dwcbHostName] = '\0';
    SECURITY_CACHE_LIST_ENTRY *pEntry;
    pEntry = GlobalCertCache.Find(lpszHostName);

    lpszHostName[dwcbHostName] = chBackup;

    if(pEntry)
    {
        pEntry->CopyOut(cInfo);
        pEntry->Release();
        ShowSecurityInfo(hwndRootWindow,
                         &cInfo);
        CertFreeCertificateContext(cInfo.pCertificate);
        fResult = TRUE;
        goto done;
    }

Cleanup:
    // No certificate info, display messagebox.
    LoadStringWrapW(
            GlobalDllHandle,
            IDS_NOCERT_TITLE,
            szTitle,
            sizeof(szTitle) / sizeof(szTitle[0]));

    LoadStringWrapW(
            GlobalDllHandle,
            IDS_NOCERT,
            szMessage,
            sizeof(szMessage) / sizeof(szMessage[0]));

     MessageBoxWrapW(hwndRootWindow,
         szMessage,
         szTitle,
         MB_ICONINFORMATION | MB_OK);

done:
    DEBUG_LEAVE_API(fResult);
    return fResult;
}


INTERNETAPI
HINTERNET
WINAPI
InternetConnectA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszServerName,
    IN INTERNET_PORT nServerPort,
    IN LPCSTR lpszUserName OPTIONAL,
    IN LPCSTR lpszPassword OPTIONAL,
    IN DWORD dwService,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Opens a connection with a server, logging-on the user in the process.

Arguments:

    hInternet       - Internet handle, returned by InternetOpen()

    lpszServerName  - name of server with which to connect

    nServerPort     - port at which server listens

    lpszUserName    - name of current user

    lpszPassword    - password of current user

    dwService       - service required. Controls type of handle generated.
                      May be one of:
                        - INTERNET_SERVICE_FTP
                        - INTERNET_SERVICE_GOPHER
                        - INTERNET_SERVICE_HTTP

    dwFlags         - protocol-specific flags. The following are defined:
                        - INTERNET_FLAG_PASSIVE (FTP)
                        - INTERNET_FLAG_KEEP_CONNECTION (HTTP)
                        - INTERNET_FLAG_SECURE (HTTP)

    dwContext       - application-supplied value used to identify this
                      request in callbacks

Return Value:

    HINTERNET
        Success - address of a new handle object

        Failure - NULL. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetConnectA",
                     "%#x, %q, %d, %q, %q, %s (%d), %#08x, %#x",
                     hInternet,
                     lpszServerName,
                     nServerPort,
                     lpszUserName,
                     lpszPassword,
                     InternetMapService(dwService),
                     dwService,
                     dwFlags,
                     dwContext
                     ));

    char emailName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    char proxyBuf[INTERNET_MAX_HOST_NAME_LENGTH + 1];

    HINTERNET connectHandle = NULL;
    HINTERNET hInternetMapped = NULL;
    HINTERNET hObject;
    HINTERNET hObjectMapped = NULL;

    LPINTERNET_THREAD_INFO lpThreadInfo;

    BOOL fUseProxy = FALSE;

    LPSTR serverName = NULL;
    LPSTR userName = (LPSTR)lpszUserName;
    LPSTR password = (LPSTR)lpszPassword;
    LPSTR realServerName = (LPSTR)lpszServerName;
    LPSTR realUserName = (LPSTR)lpszUserName;

    BOOL existingConnection = FALSE;
    BOOL viaProxy = FALSE;

    INTERNET_CONNECT_HANDLE_OBJECT * pConnect = NULL;
    //CServerInfo * lpServerInfo;

    BOOL bProtocolLevel = !(dwFlags & INTERNET_FLAG_OFFLINE);
    BOOL bIsWorker = FALSE;
    BOOL bNonNestedAsync = FALSE;
    BOOL isLocal;
    BOOL isAsync;
    BOOL bFTPSetPerUserItem = FALSE;

    DWORD error = ERROR_SUCCESS;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the per-thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();
    bIsWorker = lpThreadInfo->IsAsyncWorkerThread;
    bNonNestedAsync = bIsWorker && (lpThreadInfo->NestedRequests == 1);

    //
    // handle any global proxy settings changes first
    //

    if (InternetSettingsChanged()) {
        ChangeGlobalSettings();
    }

    //
    // handle/refcount munging:
    //
    //  sync:
    //      map hInternet on input (+1 ref)
    //      generate connect handle (1 ref)
    //      if failure && !connect handle
    //          close connect handle (0 refs: delete)
    //      if success
    //          deref hInternet (-1 ref)
    //      else if going async
    //          ref connect handle (2 refs)
    //
    //  async:
    //      hInternet is mapped connect handle (2 refs)
    //      get real hInternet from connect handle parent (2 refs (e.g.))
    //      deref connect handle (1 ref)
    //      if failure
    //          close connect handle (0 refs: delete)
    //      deref open handle (-1 ref)
    //
    // N.B. the final deref of the *indicated* handle on async callback will
    // happen in the async code
    //

    if (bNonNestedAsync) {
        connectHandle = hInternet;
        hInternetMapped = ((HANDLE_OBJECT *)connectHandle)->GetParent();
        hInternet = ((HANDLE_OBJECT *)hInternetMapped)->GetPseudoHandle();
    } else {
        error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, FALSE);
        if ((error != ERROR_SUCCESS) && (hInternetMapped == NULL)) {
            goto quit;
        }

        //
        // set the info context and clear the last error info
        //

        _InternetSetObjectHandle(lpThreadInfo, hInternet, hInternetMapped);
        _InternetClearLastError(lpThreadInfo);
        _InternetSetContext(lpThreadInfo, dwContext);

        //
        // quit now if the handle object is invalidated
        //

        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // validate the handle & discover local/remote & sync/async
        //

        error = RIsHandleLocal(hInternetMapped,
                               &isLocal,
                               &isAsync,
                               TypeInternetHandle
                               );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // we allow all valid flags to be passed in
        //

        if ((dwFlags & ~INTERNET_FLAGS_MASK)
            || (lpszServerName == NULL)
            || (*lpszServerName == '\0')) {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }
    }

    INTERNET_SCHEME schemeType;

    switch (dwService) {
    case INTERNET_SERVICE_FTP:
        schemeType = INTERNET_SCHEME_FTP;
        break;

    case INTERNET_SERVICE_GOPHER:
        schemeType = INTERNET_SCHEME_GOPHER;
        break;

    case INTERNET_SERVICE_HTTP:
        schemeType = (dwFlags & INTERNET_FLAG_SECURE)
                        ? INTERNET_SCHEME_HTTPS
                        : INTERNET_SCHEME_HTTP;
        break;

    default:
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // validate arguments if we're not in the async thread context, in which
    // case we did this when the original request was made
    //

    if (bNonNestedAsync) {
        pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)connectHandle;
        goto sync_path;
    }

    //
    // app thread or in async worker thread but being called from another
    // async API, such as InternetOpenUrl()
    //

    //
    // special case: if the server name is the NULL pointer or empty string, and
    // the port is 0 AND we have a proxy configured for this protocol then the
    // app is asking to connect directly to the proxy (the proxy server itself
    // had better be in the bypass list!)
    //
    // BUGBUG - not sure if this is really where we want to do this
    //

    INTERNET_HANDLE_OBJECT * lpInternet;

    lpInternet = (INTERNET_HANDLE_OBJECT * )hInternetMapped;

    //
    // if the port value is 0 convert it to the default port for the
    // protocol
    //

    if (nServerPort == INTERNET_INVALID_PORT_NUMBER) {
        switch (dwService) {
        case INTERNET_SERVICE_FTP:
            nServerPort = INTERNET_DEFAULT_FTP_PORT;
            break;

        case INTERNET_SERVICE_GOPHER:
            nServerPort = INTERNET_DEFAULT_GOPHER_PORT;
            break;

        case INTERNET_SERVICE_HTTP:
            if (dwFlags & INTERNET_FLAG_SECURE) {
                nServerPort = INTERNET_DEFAULT_HTTPS_PORT;
            } else {
                nServerPort = INTERNET_DEFAULT_HTTP_PORT;
            }
            break;
        }
    }

    //
    // if we have been given a net (i.e. IP) address, try to convert it to the
    // corresponding host name
    //

    //if (IsNetAddress((LPSTR)lpszServerName)) {
        //lpszServerName = (LPCSTR)MapNetAddressToName((LPSTR)lpszServerName);
        realServerName = (LPSTR)lpszServerName;
    //}

    //
    // we need to get the username and password for the current user before we
    // make the connection proper. The reason for this is that if we leave it
    // to the server, it will end up with a username of "SYSTEM" for all
    // anonymous FTP connects
    //

    if (dwService == INTERNET_SERVICE_FTP) {

        //
        // Make sure we have the correct Proxy-Network Settings, at this point.
        //

        InternetAutodialIfNotLocalHost(NULL, (LPSTR) lpszServerName);

        //
        // check the user name & password. If NULLs were supplied, use the values
        // from the registry
        //

        //
        // Do we need to set this item as per user? If a username was supplied (
        // either cracked from the URL or set on the handle, userName will be
        // non-null. Currently, this is the only criteria for when we set pu.
        //
        // Need to check this BEFORE calling GetEmailNameAndPassword which will
        // plugin "anonymous" if no username provided.
        //

        bFTPSetPerUserItem = userName ? TRUE : FALSE;
        DEBUG_PRINT(FTP,
                    INFO,
                    ("InternetConnectA:FTP: bFTPSetPerUserItem = %d\n",
                    bFTPSetPerUserItem
                    ));

        error = GetEmailNameAndPassword(&userName,
                                        &password,
                                        emailName,
                                        sizeof(emailName)
                                        );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // this is the user name we will use for the object, i.e. either the
        // name supplied, or "anonymous" as mapped above
        //

        realUserName = userName;

        //
        // if this request is going via an FTP proxy, then convert the parameters
        // now. We convert the username to <username>@<servername>, the password
        // remains the same, and the server name & port become the proxy server
        // name and port
        //
        // N.B. We ONLY do this once on the initial (synchronous) path
        //

        AUTO_PROXY_ASYNC_MSG proxyInfoQuery(
            INTERNET_SCHEME_FTP,
            (LPSTR)lpszServerName,
            lstrlen((LPSTR)lpszServerName)
            );

        AUTO_PROXY_ASYNC_MSG *pProxyInfoQuery;

        pProxyInfoQuery = &proxyInfoQuery;


        proxyInfoQuery.SetAvoidAsyncCall(TRUE);

        error = lpInternet->GetProxyInfo(
                    &pProxyInfoQuery
                    );

        if ( error != ERROR_SUCCESS )
        {
            goto quit;
        }

        if ( proxyInfoQuery.IsUseProxy() )
        {
            if (proxyInfoQuery.GetProxyScheme() == INTERNET_SCHEME_FTP)
            {

                int ulen = lstrlen(userName);
                int slen = lstrlen(lpszServerName);

                INET_ASSERT((ulen + slen) < (sizeof(proxyBuf) - 1));

                if ((ulen + slen) < (sizeof(proxyBuf) - 1))
                {
                    memcpy(proxyBuf, userName, ulen);
                    proxyBuf[ulen++] = '@';
                    memcpy(&proxyBuf[ulen], lpszServerName, slen + 1);

                    //
                    // keep a pointer to the real user name for when we
                    // create the object
                    //

                    realUserName = userName;
                    userName = proxyBuf;

                    //
                    // create a copy of the proxy name. We have to do
                    // this in case the current proxy list is replaced
                    // while we are using this string
                    //

                    //
                    // N.B. we can't be here if we determined that the
                    // proxy server was the destination (i.e. we mapped2
                    // the empty server name above)
                    //

                    INET_ASSERT(serverName == NULL);

                    serverName = NewString((LPCSTR)proxyInfoQuery._lpszProxyHostName);

                    if ( serverName == NULL )
                    {
                        error = ERROR_NOT_ENOUGH_MEMORY;
                        goto quit;
                    }

                    //
                    // keep a pointer to the real (origin) server name
                    // for when we create the object
                    //

                    realServerName = (LPSTR)lpszServerName;
                    lpszServerName = serverName;

                    //
                    // BUGBUG - what if proxyPort != nServerPort? Where
                    //          should the port go (user@server:port?)
                    //

                    nServerPort = proxyInfoQuery._nProxyHostPort;

                    //
                    // this request will go via proxy
                    //

                    viaProxy = TRUE;

                }
                else
                {

                    //
                    // blew internal limit
                    //

                    error = ERROR_INTERNET_INTERNAL_ERROR;
                    goto quit;
                }
            }
        }

    }
    else
    {
        if (userName != NULL) {
            if (IsBadStringPtr(userName, INTERNET_MAX_USER_NAME_LENGTH)) {
                error = ERROR_INVALID_PARAMETER;
                goto quit;
            } else if (*userName == '\0') {
                userName = NULL;
            }
        }
        if (password != NULL) {
            if (IsBadStringPtr(password, INTERNET_MAX_PASSWORD_LENGTH)) {
                error = ERROR_INVALID_PASSWORD;
                goto quit;
            } else if (*password == '\0') {
                password = NULL;
            }
        }
    }

    //
    // find the handle object if EXISTING_CONNECT AND we are creating protocol-
    // level connections, else create it
    //

    INET_ASSERT(connectHandle == NULL);
    INET_ASSERT(error == ERROR_SUCCESS);

    if ((dwFlags & INTERNET_FLAG_EXISTING_CONNECT) && bProtocolLevel) {
        connectHandle = FindExistingConnectObject(hInternet,
                                                  realServerName,
                                                  nServerPort,
                                                  realUserName,
                                                  password,
                                                  dwService,
                                                  dwFlags,
                                                  dwContext
                                                  );
    }
    if (connectHandle != NULL) {
        existingConnection = TRUE;
    } else {

        //
        // turn off INTERNET_FLAG_EXISTING_CONNECT if we are creating a cache
        // handle - we don't want the handle to hang around after we delete
        // the request handle (i.e. be set unused by InternetCloseHandle()).
        // N.B. We don't need this flag after this operation, so its safe to
        // remove it from dwFlags
        //

        if (!bProtocolLevel) {
            dwFlags &= ~INTERNET_FLAG_EXISTING_CONNECT;
        }
        error = RMakeInternetConnectObjectHandle(
                    hInternetMapped,
                    &connectHandle,
                    (CONNECT_CLOSE_HANDLE_FUNC)wInternetCloseConnectA,
                    realServerName, // origin server, not proxy
                    nServerPort,
                    realUserName,   // just user name, not user@server
                    password,
                    dwService,
                    dwFlags,
                    dwContext
                    );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    //
    // this new handle will be used in callbacks
    //

    _InternetSetObjectHandle(lpThreadInfo,
                             ((HANDLE_OBJECT *)connectHandle)->GetPseudoHandle(),
                             connectHandle
                             );

    //
    // based on whether we have been asked to perform async I/O AND we are not
    // in an async worker thread context AND the request is to connect with an
    // FTP service (currently only FTP because this request performs network
    // I/O - gopher and HTTP just allocate & fill in memory) AND there is a
    // valid context value, we will queue the async request, or execute the
    // request synchronously
    //

    //
    // BUGBUG - GetFlags()
    //

    if ((lpInternet->GetInternetOpenFlags() | dwFlags) & INTERNET_FLAG_OFFLINE) {
        error = ERROR_SUCCESS;
        goto quit;
    }

    pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)connectHandle;
    //lpServerInfo = pConnect->GetServerInfo();
    DEBUG_PRINT(FTP,
                INFO,
                ("bIsWorker = %d isAsync = %d dwContext %s INC dwService %s ISF bProtocolLevel = %d\n",
                bIsWorker,
                isAsync,
                (dwContext == INTERNET_NO_CALLBACK) ? "==":"!=",
                (dwService == INTERNET_SERVICE_FTP) ? "==":"!=",
                bProtocolLevel));

    if (!bIsWorker
        && isAsync
        && (dwContext != INTERNET_NO_CALLBACK)
        && ((dwService == INTERNET_SERVICE_FTP) ? bProtocolLevel : FALSE)) {

        // If we determined item should be set pu, do so now
        pConnect->SetPerUserItem(bFTPSetPerUserItem);
        DEBUG_PRINT(FTP,
                    INFO,
                    ("InternetConnectA:Async Path: SetPerUserItem to %\r\n\
                    <pConnect = 0x%x> <connectHandle = 0x%x>\r\n",
                    pConnect->IsPerUserItem(), pConnect, connectHandle));
        CFsm_FtpConnect * pFsm;

        pFsm = new CFsm_FtpConnect(lpszServerName,
                                   userName,
                                   password,
                                   nServerPort,
                                   dwService,
                                   dwFlags,
                                   dwContext
                                   );
        if (pFsm != NULL &&
            pFsm->GetError() == ERROR_SUCCESS)
        {
            BOOL bDerefConnect = TRUE;

            error = pConnect->Reference();

            if (error == ERROR_ACCESS_DENIED)
            {
                bDerefConnect = FALSE;
            }
            else if (error == ERROR_SUCCESS)
            {
                error = pFsm->QueueWorkItem();
                if (error == ERROR_IO_PENDING) {
                    hInternetMapped = NULL;
                    bDerefConnect = FALSE;
                }
            }
            if (bDerefConnect) {
                pConnect->Dereference();
            }
        }
        else
        {
            error = ERROR_NOT_ENOUGH_MEMORY;

            if ( pFsm )
            {
                error = pFsm->GetError();
                delete pFsm;
                pFsm = NULL;
            }
        }

        //
        // if we're here then ERROR_SUCCESS cannot have been returned from
        // the above calls
        //

        INET_ASSERT(error != ERROR_SUCCESS);

        DEBUG_PRINT(FTP,
                    INFO,
                    ("processing request asynchronously: error = %d\n",
                    error
                    ));

        goto quit;
    }

sync_path:

    if (bProtocolLevel && !existingConnection) {

        //
        // generate the protocol-level connect 'object' if required (for FTP).
        // This simply creates a memory object
        //

        HINTERNET protocolConnectHandle = NULL;

        INET_ASSERT(error == ERROR_SUCCESS);

        if (dwService == INTERNET_SERVICE_FTP) {
            error = wFtpConnect(lpszServerName,
                                nServerPort,
                                userName,
                                password,
                                dwService,
                                dwFlags,
                                &protocolConnectHandle
                                );
            if (error != ERROR_SUCCESS) {
                goto quit;
            }
        }

        //
        // associate the protocol-level handle and INTERNET_CONNECT_HANDLE_OBJECT
        //

        pConnect->SetConnectHandle(protocolConnectHandle);

        // If we determined item should be set pu, do so now
        pConnect->SetPerUserItem(bFTPSetPerUserItem);
        DEBUG_PRINT(FTP,
                    INFO,
                    ("InternetConnectA:Sync Path:SetPerUserItem to %d\r\n\
                        <pConnect = 0x%x> <protocolConnectHandle = 0x%x> <connectHandle = 0x%x>\r\n",
                    pConnect->IsPerUserItem(), pConnect, protocolConnectHandle, connectHandle));


        // for all connect types, get the server info and resolve the server
        // name. If we can't resolve the name then we fail this request
        //

        //lpServerInfo = pConnect->GetServerInfo();
        //if ((lpServerInfo != NULL) && !lpServerInfo->IsNameResolved()) {
        //    error = pConnect->SetServerInfo(schemeType, FALSE, FALSE);
        //    if (error != ERROR_SUCCESS) {
        //        goto quit;
        //    }
        //}

        //
        // if we succeeded in creating the connect object and this is an FTP
        // request then we will now attempt to connect to the server proper.
        //
        // We don't need to do this for gopher and HTTP because (currently) they
        // don't make server connections until we perform some other action,
        // like find file, e.g.
        //

        if (dwService == INTERNET_SERVICE_FTP) {
            error = wFtpMakeConnection(protocolConnectHandle,
                                       userName,
                                       password
                                       );
        }
    }

quit:

    _InternetDecNestingCount(1);

    //
    // free the buffer we used to hold the proxy name if we had to map the
    // empty string to a proxy name
    //

    if (serverName != NULL) {
        DEL_STRING(serverName);
    }

done:

    if (error == ERROR_SUCCESS) {

        //
        // set the via proxy flag
        //

        ((INTERNET_CONNECT_HANDLE_OBJECT *)connectHandle)->SetViaProxy(viaProxy);

        //
        // success - return generated pseudo-handle
        //

        connectHandle = ((HANDLE_OBJECT *)connectHandle)->GetPseudoHandle();

        //
        // created a handle. If we are generating protocol-level connections
        // then flush the existing connection cache
        //

        if (bProtocolLevel) {
            FlushExistingConnectObjects(hInternet);
        }
    } else {
        if (bNonNestedAsync
            && (/*((HANDLE_OBJECT *)connectHandle)->Dereference()
                ||*/ ((HANDLE_OBJECT *)connectHandle)->IsInvalidated())) {
            error = ERROR_INTERNET_OPERATION_CANCELLED;
        }

        //
        // if we are not pending an async request but we created a handle object
        // then close it
        //

        if ((error != ERROR_IO_PENDING) && (connectHandle != NULL)) {

            //
            // use _InternetCloseHandle() to close the handle: it doesn't clear
            // out the last error text, so that an app can find out what the
            // server sent us in the event of an FTP login failure
            //


            if (bNonNestedAsync) {

                //
                // this handle deref'd at async completion
                //

                hInternetMapped = NULL;
            }
            else
            {
                _InternetCloseHandle(((HANDLE_OBJECT *)connectHandle)->GetPseudoHandle());
            }
        }
        connectHandle = NULL;
    }
    if (hInternetMapped != NULL) {
        DereferenceObject((LPVOID)hInternetMapped);
    }
    if (error != ERROR_SUCCESS) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }
    DEBUG_LEAVE_API(connectHandle);
    return connectHandle;
}



INTERNETAPI
HINTERNET
WINAPI
InternetOpenUrlA(
    IN HINTERNET hInternet,
    IN LPCSTR lpszUrl,
    IN LPCSTR lpszHeaders OPTIONAL,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Opens an URL. This consists of creating a handle to the type of item
    identified by the URL - directory or file

Arguments:

    hInternet       - root Internet handle

    lpszUrl         - pointer to the URL to use to open the item

    lpszHeaders     - headers to send to HTTP server. May be NULL

    dwHeadersLength - length of lpszHeaders. May be -1 if the app wants us to
                      perform the strlen()

    dwFlags         - open flags (cache/nocache, etc.)

    dwContext       - app-supplied context value for call-backs

Return Value:

    HINTERNET
        Success - open handle to item described by URL

        Failure - NULL. Use GetLastError() to get more information about why the
                  call failed. There may be error text returned from the server
                  (in the case of a gopher or FTP URL)

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "InternetOpenUrlA",
                     "%#x, %q, %.80q, %d, %#08x, %#x",
                     hInternet,
                     lpszUrl,
                     lpszHeaders,
                     dwHeadersLength,
                     dwFlags,
                     dwContext
                     ));

    LPINTERNET_THREAD_INFO lpThreadInfo = NULL;
    DWORD error;
    DWORD nestingLevel = 0;
    HINTERNET hInternetMapped = NULL;
    HINTERNET hUrlMapped = NULL;
    HINTERNET hUrl = NULL;

    PROXY_STATE * pProxyState = NULL;
    BOOL bDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto quit;
    }

    //
    // get the thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    _InternetIncNestingCount();
    nestingLevel = 1;

    //
    // map the handle
    //

    error = MapHandleToAddress(hInternet, (LPVOID *)&hInternetMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hInternetMapped == NULL)) {
        goto quit;
    }

    //
    // set the info context and clear the last error info
    //

    _InternetSetObjectHandle(lpThreadInfo, hInternet, hInternetMapped);
    _InternetClearLastError(lpThreadInfo);
    _InternetSetContext(lpThreadInfo, dwContext);

    //
    // quit now if the handle object is invalidated
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate the handle & discover async/sync & local/remote
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hInternetMapped,
                           &isLocal,
                           &isAsync,
                           TypeInternetHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate parameters if we're not in the async worker thread context
    //

    if (!lpThreadInfo->IsAsyncWorkerThread) {

        //
        // ensure we have good values for the headers pointer and length
        //

        INET_ASSERT(error == ERROR_SUCCESS);

        if (ARGUMENT_PRESENT(lpszHeaders) && (dwHeadersLength == -1)) {
            __try {
                dwHeadersLength = lstrlen(lpszHeaders);
            } __except(EXCEPTION_EXECUTE_HANDLER) {
                error = ERROR_INVALID_PARAMETER;
            }
            ENDEXCEPT
            if (error != ERROR_SUCCESS) {
                goto quit;
            }
        } else if (!ARGUMENT_PRESENT(lpszHeaders) || (dwHeadersLength == 0)) {
            lpszHeaders = NULL;
            dwHeadersLength = 0;
        }
        if (!ARGUMENT_PRESENT(lpszUrl)
        || (*lpszUrl == '\0')
//        || !IsValidUrl(lpszUrl)
        || (dwFlags & ~INTERNET_FLAGS_MASK)) {
            error = ERROR_INVALID_PARAMETER;
            goto quit;
        }
    }

    //
    // determine if this is a http request or FTP request - either http URL, or via http proxy.
    // or via another protocol.
    // For any Async request, we must go async in order to determine proxy information
    //

    bDeref = FALSE;
    hUrl = hInternet;

    if (isAsync
    && !lpThreadInfo->IsAsyncWorkerThread
    && (dwContext != INTERNET_NO_CALLBACK))
    {
        CFsm_ParseUrlForHttp *pFsm;

        pFsm = new CFsm_ParseUrlForHttp(&hUrl,
                                       (INTERNET_HANDLE_OBJECT *)hInternetMapped,
                                       lpszUrl,
                                       lpszHeaders,
                                       dwHeadersLength,
                                       dwFlags,
                                       dwContext
                                       );

        if (pFsm != NULL) {
            // first call will not be on this API's thread context
            pFsm->ClearOnApiCall();
            error = pFsm->QueueWorkItem();
        } else {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        bDeref = TRUE;
        error = DoFsm(new CFsm_ParseUrlForHttp(&hUrl,
                                               (INTERNET_HANDLE_OBJECT *)hInternetMapped,
                                               lpszUrl,
                                               lpszHeaders,
                                               dwHeadersLength,
                                               dwFlags,
                                               dwContext
                                               ));
    }

    if ( error == ERROR_IO_PENDING )
    {
        bDeref = FALSE;
    }

quit:

    if (bDeref && hInternetMapped != NULL) {
        DereferenceObject((LPVOID)hInternetMapped);
    }

    if ( lpThreadInfo != NULL ) {
        _InternetDecNestingCount(nestingLevel);
    }

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(API, error);

        SetLastError(error);
        hUrl = NULL;
    }

    DEBUG_LEAVE_API(hUrl);

    return hUrl;
}




INTERNETAPI
BOOL
WINAPI
InternetReadFile(
    IN HINTERNET hFile,
    IN LPVOID lpBuffer,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead
    )

/*++

Routine Description:

    This functions reads the next block of data from the file object. The
    following handle/data types are supported:

        TypeGopherFileHandle        - raw gopher file data
        TypeGopherFileHandleHtml    - HTML-encapsulated gopher file data
        TypeGopherFindHandleHtml    - HTML-encapsulated gopher directory data
        TypeFtpFileHandle           - raw FTP file data
        TypeFtpFileHandleHtml       - HTML-encapsulated FTP file data
        TypeFtpFindHandleHtml       - HTML-encapsulated FTP directory data

Arguments:

    hFile                   - handle returned from Open function

    lpBuffer                - pointer to caller's buffer

    dwNumberOfBytesToRead   - size of lpBuffer in BYTEs

    lpdwNumberOfBytesRead   - returned number of bytes read into lpBuffer

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetReadFile",
                     "%#x, %#x, %d, %#x",
                     hFile,
                     lpBuffer,
                     dwNumberOfBytesToRead,
                     lpdwNumberOfBytesRead
                     ));

    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    DWORD error;
    BOOL success = FALSE;
    HINTERNET hFileMapped = NULL;
    DWORD bytesRead = 0;
    BOOL bEndRead = TRUE;
    HINTERNET_HANDLE_TYPE handleType = TypeWildHandle;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // we need the thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    //INET_ASSERT(lpThreadInfo->Fsm == NULL);

    _InternetIncNestingCount();
    nestingLevel = 1;

    //
    // map the handle
    //

    error = MapHandleToAddress(hFile, (LPVOID *)&hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL)) {
        goto quit;
    }

    //
    // set the context, handle, and last-error info in the per-thread data block
    // before we go any further. This allows us to return a status in the async
    // case, even if the handle has been closed
    //

    DWORD_PTR context;

    RGetContext(hFileMapped, &context);

    if (!lpThreadInfo->IsAsyncWorkerThread) {

        PERF_LOG(PE_CLIENT_REQUEST_START,
                 AR_INTERNET_READ_FILE,
                 lpThreadInfo->ThreadId,
                 hFile
                 );

    }

    _InternetSetContext(lpThreadInfo, context);
    _InternetSetObjectHandle(lpThreadInfo, hFile, hFileMapped);
    _InternetClearLastError(lpThreadInfo);

    //
    // if MapHandleToAddress() returned a non-NULL object address, but also an
    // error status, then the handle is being closed - quit
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate handle and retrieve its type
    //

    error = RGetHandleType(hFileMapped, &handleType);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hFileMapped, &isLocal, &isAsync, handleType);
    if (error != ERROR_SUCCESS) {

        //
        // we should not get an error - we already believe the handle object
        // is valid and of the type just retrieved!
        //

        INET_ASSERT(FALSE);

        goto quit;
    }

    //
    // ensure correct handle type
    //

    if ((handleType != TypeHttpRequestHandle)
    && (handleType != TypeFtpFileHandle)
    && (handleType != TypeGopherFileHandle)
    && (handleType != TypeFtpFindHandleHtml)
    && (handleType != TypeGopherFindHandleHtml)
    && (handleType != TypeFtpFileHandleHtml)
    && (handleType != TypeGopherFileHandleHtml)
    && (handleType != TypeFileRequestHandle)) {
        error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto quit;
    }

    //
    // validate parameters
    //

    if (!lpThreadInfo->IsAsyncWorkerThread) {
        error = ProbeAndSetDword(lpdwNumberOfBytesRead, 0);
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
        error = ProbeWriteBuffer(lpBuffer, dwNumberOfBytesToRead);
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        *lpdwNumberOfBytesRead = 0;

        if (((handleType == TypeFtpFindHandleHtml) ||
            (handleType == TypeGopherFindHandleHtml)) &&
            ((INTERNET_CONNECT_HANDLE_OBJECT *)hFileMapped)->
                            IsCacheReadInProgress())
        {
            error = ((INTERNET_CONNECT_HANDLE_OBJECT *)hFileMapped)->
                                        ReadCache((LPBYTE)lpBuffer,
                                        dwNumberOfBytesToRead,
                                        &bytesRead);
            success = (error == ERROR_SUCCESS);
            goto quit;
        }

        if (handleType == TypeHttpRequestHandle) {

            HTTP_REQUEST_HANDLE_OBJECT *lpRequest =
                (HTTP_REQUEST_HANDLE_OBJECT *) hFileMapped;

            // See if request can be fulfilled from file system.
            if (lpRequest->AttemptReadFromFile
                (lpBuffer, dwNumberOfBytesToRead, &bytesRead)) {
                success = TRUE;
                goto quit;
            }

        } // end if (handleType == TypeHttpRequestHandle)

        else
        {
            //
            // trap a zero-length buffer before we go to the trouble of going async.
            // Maintain compatibility with base ReadFile(), although this is
            // POTENTIALLY A BUG. ReadFile() is *supposed* to return TRUE and number
            // of bytes read equal to zero to indicate end-of-file, but it will also
            // return TRUE and zero if a read of zero bytes is requested. According
            // to MarkL, that's the way it is. Good enough for me...
            //
            // For http, AttemptToReadFromFile traps zero-length reads

            if (dwNumberOfBytesToRead == 0) {

                //
                // *lpdwNumberOfBytesRead and error should have been correctly set
                // during parameter validation
                //

                INET_ASSERT(*lpdwNumberOfBytesRead == 0);
                INET_ASSERT(error == ERROR_SUCCESS);

                success = TRUE;
                goto quit;
            }
        } // end else (handleType != TypeHttpRequestHandle)

    } // end if (!lpThreadInfo->IsAsyncWorkerThread)

    //
    // the request will only be made asynchronously if more data is requested
    // than is immediately available AND we haven't reached end of file
    //

    DWORD available;

    available = ((INTERNET_HANDLE_OBJECT *)hFileMapped)->AvailableDataLength();

    BOOL eof;

    eof = ((INTERNET_HANDLE_OBJECT *)hFileMapped)->IsEndOfFile();

    if (!lpThreadInfo->IsAsyncWorkerThread
    && isAsync
    && (context != INTERNET_NO_CALLBACK)
    && (dwNumberOfBytesToRead > available)
    && !eof
    && (handleType != TypeHttpRequestHandle)
    && (handleType != TypeFileRequestHandle)) {

        // MakeAsyncRequest
        CFsm_InternetReadFile * pFsm;

        pFsm = new CFsm_InternetReadFile(hFile, lpBuffer, dwNumberOfBytesToRead, lpdwNumberOfBytesRead);
        if (pFsm != NULL) {
            error = pFsm->QueueWorkItem();
            if ( error == ERROR_IO_PENDING ) {
                bEndRead = FALSE;
            }
        } else {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // if we're here then ERROR_SUCCESS cannot have been returned from
        // the above calls
        //

        INET_ASSERT(error != ERROR_SUCCESS);


        DEBUG_PRINT(FTP,
                    INFO,
                    ("processing request asynchronously: error = %d\n",
                    error
                    ));

        goto quit;

        //
        // we're going synchronous - set the error so we do the right thing when
        // we exit
        //

        error = ERROR_SUCCESS;
    } else if ((available >= dwNumberOfBytesToRead) || eof) {

        DEBUG_PRINT(API,
                    INFO,
                    ("immediate read: %d requested, %d available. EOF = %B\n",
                    dwNumberOfBytesToRead,
                    available,
                    eof
                    ));

    }

    INET_ASSERT(error == ERROR_SUCCESS);

    //
    // just call the underlying API: return whatever it returns, and let it
    // handle setting the last error
    //

    switch (handleType) {
    case TypeFtpFileHandle:
        success = FtpReadFile(hFileMapped,
                              lpBuffer,
                              dwNumberOfBytesToRead,
                              &bytesRead
                              );
        break;

    case TypeGopherFileHandle:
        success = GopherReadFile(hFileMapped,
                                 lpBuffer,
                                 dwNumberOfBytesToRead,
                                 &bytesRead
                                 );
        break;

    case TypeFtpFindHandleHtml:
    case TypeGopherFindHandleHtml:

        //
        // HTML handle types - convert underlying data to HTML document
        //

        success = ReadHtmlUrlData(hFileMapped,
                                  lpBuffer,
                                  dwNumberOfBytesToRead,
                                  &bytesRead
                                  );

        if (((INTERNET_CONNECT_HANDLE_OBJECT *)hFileMapped)->IsCacheWriteInProgress())
        {
            DWORD errorCache;

            if (success) {


                if (bytesRead) {

                    errorCache = ((INTERNET_CONNECT_HANDLE_OBJECT *)hFileMapped)->
                                    WriteCache(
                                        (LPBYTE)lpBuffer,
                                        bytesRead
                                    );
                }
                else {
                    errorCache = ERROR_NO_MORE_FILES;
                }

            }
            else {
                errorCache = GetLastError();
            }

            // if the thing failed because the caller passed in
            // insufficient buffer for internetreadfile
            // then we should do nothing

            if ((errorCache != ERROR_SUCCESS)&&
                (errorCache != ERROR_INSUFFICIENT_BUFFER)) {
                if (handleType == TypeFtpFindHandleHtml) {

                    // we save extension in the index file
                    // this is used to differentiate between html directory
                    // entry from the non-html one for the same url

                    InbLocalEndCacheWrite(  hFileMapped,
                                            "htm",  // save extension in index file
                                            (errorCache == ERROR_NO_MORE_FILES)
                                         );
                }
                else {
                    InbGopherLocalEndCacheWrite(  hFileMapped,
                                                "htm",
                                                (errorCache == ERROR_NO_MORE_FILES)
                                         );
                }
            }
        }

        break;


    case TypeFtpFileHandleHtml:
    case TypeGopherFileHandleHtml:

        //
        // HTML handle types - convert underlying data to HTML document
        //

        success = ReadHtmlUrlData(hFileMapped,
                                  lpBuffer,
                                  dwNumberOfBytesToRead,
                                  &bytesRead
                                  );
        break;

    case TypeHttpRequestHandle:

        {
            //HTTP_REQUEST_HANDLE_OBJECT * lpRequest;
            //
            //lpRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hFileMapped;
            //
            //error = lpRequest->QuickSyncRead(
            //                lpBuffer,
            //                dwNumberOfBytesToRead,
            //                lpdwNumberOfBytesRead,
            //                0
            //                );
            //
            //if ( error == ERROR_IO_PENDING )
            //{
                error = DoFsm(new CFsm_ReadFile(lpBuffer,
                                                dwNumberOfBytesToRead,
                                                lpdwNumberOfBytesRead
                                                ));
            //}

            success = (error == ERROR_SUCCESS) ? TRUE : FALSE;
            bEndRead = FALSE;
            break;
        }

    case TypeFileRequestHandle:

        success = ReadFile(
                    ((INTERNET_FILE_HANDLE_OBJECT *) hFileMapped)->GetFileHandle(),
                    lpBuffer,
                    dwNumberOfBytesToRead,
                    &bytesRead,
                    NULL // overlapped I/O
                    );

        if (!success)
        {
            error = GetLastError();
        }
        else
        {
            error = ERROR_SUCCESS;
        }

        break;

    case TypeFtpFindHandle:
    case TypeGopherFindHandle:

        //
        // you cannot receive RAW directory data using this API. You have
        // to call InternetFindNextFile()
        //

    default:

        //
        // the handle is a valid handle (or else RGetHandleType() would
        // have returned ERROR_INVALID_HANDLE), but this operation is
        // inconsistent with the handle type. Return a more prosaic error
        // code
        //

        error = ERROR_INTERNET_INVALID_OPERATION;
        break;
    }

quit:

    _InternetDecNestingCount(nestingLevel);;

    if (bEndRead) {

        //
        // if handleType is not HttpRequest or File then we are making this
        // request in the context of an uninterruptable async worker thread.
        // HTTP and file requests use the normal mechanism. In the case of non-
        // HTTP and file requests, we need to treat the request as if it were
        // sync and deref the handle
        //

        ReadFile_End(!lpThreadInfo->IsAsyncWorkerThread
                     || !((handleType == TypeHttpRequestHandle)
                          || (handleType == TypeFileRequestHandle)),
                     success,
                     hFileMapped,
                     bytesRead,
                     lpBuffer,
                     dwNumberOfBytesToRead,
                     lpdwNumberOfBytesRead
                     );
    }

    if (lpThreadInfo && !lpThreadInfo->IsAsyncWorkerThread) {

        PERF_LOG(PE_CLIENT_REQUEST_END,
                 AR_INTERNET_READ_FILE,
                 bytesRead,
                 lpThreadInfo->ThreadId,
                 hFile
                 );

    }

done:

    //
    // if error is not ERROR_SUCCESS then this function returning the error,
    // otherwise the error has already been set by the API we called,
    // irrespective of the value of success
    //

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(API, error);

        SetLastError(error);
        success = FALSE;
    }

    DEBUG_LEAVE_API(success);

    return success;
}


PRIVATE
VOID
ReadFile_End(
    IN BOOL bDeref,
    IN BOOL bSuccess,
    IN HINTERNET hFileMapped,
    IN DWORD dwBytesRead,
    IN LPVOID lpBuffer OPTIONAL,
    IN DWORD dwNumberOfBytesToRead,
    OUT LPDWORD lpdwNumberOfBytesRead OPTIONAL
    )

/*++

Routine Description:

    Common end-of-read processing:

        - update bytes read parameter
        - dump data if logging & API data requested
        - dereference handle if not async request

Arguments:

    bDeref                  - TRUE if handle should be dereferenced (should be
                              FALSE for async request)

    bSuccess                - TRUE if Read completed successfully

    hFileMapped             - mapped file handle

    dwBytesRead             - number of bytes read

    lpBuffer                - into this buffer

    dwNumberOfBytesToRead   - originally requested bytes to read

    lpdwNumberOfBytesRead   - where bytes read is stored

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_INET,
                 None,
                 "ReadFile_End",
                 "%B, %B, %#x, %d, %#x, %d, %#x",
                 bDeref,
                 bSuccess,
                 hFileMapped,
                 dwBytesRead,
                 lpBuffer,
                 dwNumberOfBytesToRead,
                 lpdwNumberOfBytesRead
                 ));

    if (bSuccess) {

        //
        // update the amount of immediate data available only if we succeeded
        //

        ((INTERNET_HANDLE_OBJECT *)hFileMapped)->ReduceAvailableDataLength(dwBytesRead);

        if (lpdwNumberOfBytesRead != NULL) {
            *lpdwNumberOfBytesRead = dwBytesRead;

            DEBUG_PRINT(API,
                        INFO,
                        ("*lpdwNumberOfBytesRead = %d\n",
                        *lpdwNumberOfBytesRead
                        ));

            //
            // dump API data only if requested
            //

            IF_DEBUG_CONTROL(DUMP_API_DATA) {
                DEBUG_DUMP_API(API,
                               "Received data:\n",
                               lpBuffer,
                               *lpdwNumberOfBytesRead
                               );
            }
        }
        if (dwBytesRead < dwNumberOfBytesToRead) {

            DEBUG_PRINT(API,
                        INFO,
                        ("(!) bytes read (%d) < bytes requested (%d)\n",
                        dwBytesRead,
                        dwNumberOfBytesToRead
                        ));

        }
    }

    //
    // if async request, handle will be deref'd after REQUEST_COMPLETE callback
    // is delivered
    //

    if (bDeref && (hFileMapped != NULL)) {
        DereferenceObject((LPVOID)hFileMapped);
    }

    PERF_LOG(PE_CLIENT_REQUEST_END,
             AR_INTERNET_READ_FILE,
             dwBytesRead,
             0,
             (!bDeref && hFileMapped) ? ((INTERNET_HANDLE_OBJECT *)hFileMapped)->GetPseudoHandle() : NULL
             );

    DEBUG_LEAVE(0);
}


DWORD
CFsm_ReadFile::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_ReadFile::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    CFsm_ReadFile * stateMachine = (CFsm_ReadFile *)Fsm;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = ReadFile_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
ReadFile_Fsm(
    IN CFsm_ReadFile * Fsm
    )
{
    DEBUG_ENTER((DBG_INET,
                 Dword,
                 "ReadFile_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_ReadFile & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if ((error == ERROR_SUCCESS) && (fsm.GetState() == FSM_STATE_INIT)) {
        error = HttpReadData(fsm.GetMappedHandle(),
                             fsm.m_lpBuffer,
                             fsm.m_dwNumberOfBytesToRead,
                             &fsm.m_dwBytesRead,
                             0
                             );
        if (error == ERROR_IO_PENDING) {
            goto quit;
        }
    }
    ReadFile_End(!fsm.GetThreadInfo()->IsAsyncWorkerThread,
                 (error == ERROR_SUCCESS) ? TRUE : FALSE,
                 fsm.GetMappedHandle(),
                 fsm.m_dwBytesRead,
                 fsm.m_lpBuffer,
                 fsm.m_dwNumberOfBytesToRead,
                 fsm.m_lpdwNumberOfBytesRead
                 );
    fsm.SetDone();

quit:

    DEBUG_LEAVE(error);

    return error;
}


INTERNETAPI
BOOL
WINAPI
InternetReadFileExA(
    IN HINTERNET hFile,
    OUT LPINTERNET_BUFFERSA lpBuffersOut,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetReadFileExA",
                     "%#x, %#x [%#x, %d], %#x, %#x",
                     hFile,
                     lpBuffersOut,
                     (lpBuffersOut ? lpBuffersOut->lpvBuffer : NULL),
                     (lpBuffersOut ? lpBuffersOut->dwBufferLength : 0),
                     dwFlags,
                     dwContext
                     ));

    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    DWORD error;
    HINTERNET hFileMapped = NULL;
    DWORD bytesRead = 0;
    LPVOID lpBuffer = NULL;
    DWORD dwNumberOfBytesToRead;
    BOOL bEndRead = TRUE;
    BOOL success = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // we need the thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();
    nestingLevel = 1;

    //
    // map the handle
    //

    error = MapHandleToAddress(hFile, (LPVOID *)&hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL)) {
        goto quit;
    }

    //
    // set the context, handle, and last-error info in the per-thread data block
    // before we go any further. This allows us to return a status in the async
    // case, even if the handle has been closed
    //

    DWORD_PTR context;

    RGetContext(hFileMapped, &context);

    if (!lpThreadInfo->IsAsyncWorkerThread) {

        PERF_LOG(PE_CLIENT_REQUEST_START,
                 AR_INTERNET_READ_FILE,
                 lpThreadInfo->ThreadId,
                 hFile
                 );

    }

    _InternetSetContext(lpThreadInfo, context);
    _InternetSetObjectHandle(lpThreadInfo, hFile, hFileMapped);
    _InternetClearLastError(lpThreadInfo);

    //
    // if MapHandleToAddress() returned a non-NULL object address, but also an
    // error status, then the handle is being closed - quit
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate handle and retrieve its type
    //

    HINTERNET_HANDLE_TYPE handleType;

    error = RGetHandleType(hFileMapped, &handleType);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hFileMapped, &isLocal, &isAsync, handleType);
    if (error != ERROR_SUCCESS) {

        //
        // we should not get an error - we already believe the handle object
        // is valid and of the type just retrieved!
        //

        INET_ASSERT(FALSE);

        goto quit;
    }

    //
    // only accepting HTTP handles currently
    //

    if (handleType != TypeHttpRequestHandle) {
        error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto quit;
    }

    HTTP_REQUEST_HANDLE_OBJECT * lpRequest;

    lpRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hFileMapped;

    //
    // validate params
    //

    if (lpBuffersOut->dwStructSize != sizeof(INTERNET_BUFFERS)) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    lpBuffer = lpBuffersOut->lpvBuffer;
    dwNumberOfBytesToRead = lpBuffersOut->dwBufferLength;

    INET_ASSERT(dwNumberOfBytesToRead > 0);

    //
    // See if request can be fulfilled from file system.
    //

    if (lpRequest->AttemptReadFromFile(lpBuffer,
                                       dwNumberOfBytesToRead,
                                       &bytesRead)) {
        error = ERROR_SUCCESS;
        goto quit;
    }

    //
    // trap a zero-length buffer before we go to the trouble of going async.
    // Maintain compatibility with base ReadFile(), although this is
    // POTENTIALLY A BUG. ReadFile() is *supposed* to return TRUE and number
    // of bytes read equal to zero to indicate end-of-file, but it will also
    // return TRUE and zero if a read of zero bytes is requested. According
    // to MarkL, that's the way it is. Good enough for me...
    //
    // For http, AttemptToReadFromFile traps zero-length reads

    if (dwNumberOfBytesToRead == 0) {

        //
        // *lpdwNumberOfBytesRead and error should have been correctly set
        // during parameter validation
        //

        INET_ASSERT(error == ERROR_SUCCESS);

        goto quit;
    }

    //error = lpRequest->QuickSyncRead(
    //                lpBuffer,
    //                dwNumberOfBytesToRead,
    //                &bytesRead,
    //                SF_NO_WAIT
    //                );
    //
    //if ( error == ERROR_IO_PENDING )
    //{
        error = DoFsm(new CFsm_ReadFileEx(lpBuffersOut,
                                          dwFlags,
                                          dwContext
                                          ));
    //}

    if (error == ERROR_SUCCESS) {
        bytesRead = lpBuffersOut->dwBufferLength;
    }
    bEndRead = FALSE;

quit:

    _InternetDecNestingCount(nestingLevel);;

    if (bEndRead) {
        ReadFile_End(TRUE,
                     (error == ERROR_SUCCESS),
                     hFileMapped,
                     bytesRead,
                     lpBuffersOut->lpvBuffer,
                     dwNumberOfBytesToRead,
                     &lpBuffersOut->dwBufferLength
                     );

    }

done:

    if (error != ERROR_SUCCESS) {

        DEBUG_ERROR(API, error);

        SetLastError(error);
        success = FALSE;
    }

    DEBUG_LEAVE_API(success);

    return success;
}


DWORD
CFsm_ReadFileEx::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_ReadFileEx::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    CFsm_ReadFileEx * stateMachine = (CFsm_ReadFileEx *)Fsm;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = ReadFileEx_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
ReadFileEx_Fsm(
    IN CFsm_ReadFileEx * Fsm
    )
{
    DEBUG_ENTER((DBG_INET,
                 Dword,
                 "ReadFileEx_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_ReadFileEx & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if ((error == ERROR_SUCCESS) && (fsm.GetState() == FSM_STATE_INIT)) {
        fsm.m_dwNumberOfBytesToRead = fsm.m_lpBuffersOut->dwBufferLength;
        error = HttpReadData(fsm.GetMappedHandle(),
                             fsm.m_lpBuffersOut->lpvBuffer,
                             fsm.m_dwNumberOfBytesToRead,
                             &fsm.m_dwBytesRead,
                             (fsm.m_dwFlags & IRF_NO_WAIT)
                               ? SF_NO_WAIT
                               : 0
                             );
        if (error == ERROR_IO_PENDING) {
            goto quit;
        }
    }

    //
    // if we are asynchronously completing a no-wait read then we don't update
    // any app parameters - we simply return the indication that we completed.
    // The app will then make another no-wait read to get the data
    //

    BOOL bNoOutput;

    bNoOutput = ((fsm.m_dwFlags & IRF_NO_WAIT)
                && fsm.GetThreadInfo()->IsAsyncWorkerThread)
                    ? TRUE
                    : FALSE;

    ReadFile_End(!fsm.GetThreadInfo()->IsAsyncWorkerThread,
                 (error == ERROR_SUCCESS) ? TRUE : FALSE,
                 fsm.GetMappedHandle(),
                 bNoOutput ? 0    : fsm.m_dwBytesRead,
                 bNoOutput ? NULL : fsm.m_lpBuffersOut->lpvBuffer,
                 bNoOutput ? 0    : fsm.m_dwNumberOfBytesToRead,
                 bNoOutput ? NULL : &fsm.m_lpBuffersOut->dwBufferLength
                 );
    fsm.SetDone();

quit:

    DEBUG_LEAVE(error);

    return error;
}


INTERNETAPI
BOOL
WINAPI
InternetWriteFile(
    IN HINTERNET hFile,
    IN LPCVOID lpBuffer,
    IN DWORD dwNumberOfBytesToWrite,
    OUT LPDWORD lpdwNumberOfBytesWritten
    )

/*++

Routine Description:

    This function write next block of data to the internet file. Currently it
    supports the following protocol data:

        FtpWriteFile
        HttpWriteFile
        FileWriteFile

Arguments:

    hFile                       - handle that was obtained by OpenFile Call

    lpBuffer                    - pointer to the data buffer

    dwNumberOfBytesToWrite      - number of bytes in the above buffer

    lpdwNumberOfBytesWritten    -  pointer to a DWORD where the number of bytes
                                   of data actually written is returned

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetWriteFile",
                     "%#x, %#x, %d, %#x",
                     hFile,
                     lpBuffer,
                     dwNumberOfBytesToWrite,
                     lpdwNumberOfBytesWritten
                     ));

    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    DWORD error;
    BOOL success = FALSE;
    BOOL fNeedDeref = TRUE;
    HINTERNET hFileMapped = NULL;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the per-thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();
    nestingLevel = 1;

    //
    // map the handle
    //

    error = MapHandleToAddress(hFile, (LPVOID *)&hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL)) {
        goto quit;
    }

    //
    // set the context, handle, and last-error info in the per-thread data block
    // before we go any further. This allows us to return a status in the async
    // case, even if the handle has been closed
    //

    DWORD_PTR context;

    RGetContext(hFileMapped, &context);
    _InternetSetContext(lpThreadInfo, context);
    _InternetSetObjectHandle(lpThreadInfo, hFile, hFileMapped);
    _InternetClearLastError(lpThreadInfo);

    //
    // if MapHandleToAddress() returned a non-NULL object address, but also an
    // error status, then the handle is being closed - quit
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate handle and retrieve its type
    //

    HINTERNET_HANDLE_TYPE handleType;

    error = RGetHandleType(hFileMapped, &handleType);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hFileMapped, &isLocal, &isAsync, handleType);
    if (error != ERROR_SUCCESS) {

        //
        // we should not get an error - we already believe the handle object
        // is valid and of the type just retrieved!
        //

        INET_ASSERT(FALSE);

        goto quit;
    }

    //
    // validate parameters - write length cannot be 0
    //

    if (!lpThreadInfo->IsAsyncWorkerThread) {
        if (dwNumberOfBytesToWrite != 0) {
            error = ProbeReadBuffer((LPVOID)lpBuffer, dwNumberOfBytesToWrite);
            if (error == ERROR_SUCCESS) {
                error = ProbeAndSetDword(lpdwNumberOfBytesWritten, 0);
            }
        } else {
            error = ERROR_INVALID_PARAMETER;
        }
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }


    // # 62953
    // If the authentication state of the handle is Negotiate,
    // don't submit data to the server but return success.
    // ** Added test for NTLM or Negotiate - Adriaanc.
    if (handleType == TypeHttpRequestHandle)
    {
        HTTP_REQUEST_HANDLE_OBJECT *pRequest;
        pRequest = (HTTP_REQUEST_HANDLE_OBJECT*) hFileMapped;

        if (pRequest->GetAuthState() == AUTHSTATE_NEGOTIATE
            && !((PLUG_CTX*) (pRequest->GetAuthCtx()))->_fNTLMProxyAuth
            && !(pRequest->GetAuthCtx()->GetSchemeType() == AUTHCTX::SCHEME_DPA))
        {
            *lpdwNumberOfBytesWritten = dwNumberOfBytesToWrite;
            error = ERROR_SUCCESS;
            success = TRUE;
            goto quit;
        }
    }
    //
    // we have to do some work. If the file object handle was created with
    // async I/O capability then we will queue an async request, otherwise
    // we will process the request synchronously
    //

    if (isAsync
    && !lpThreadInfo->IsAsyncWorkerThread
    && (handleType != TypeHttpRequestHandle)
    && (handleType != TypeFileRequestHandle)) {

        // MakeAsyncRequest
        CFsm_InternetWriteFile * pFsm;

        pFsm = new CFsm_InternetWriteFile(hFile, lpBuffer, dwNumberOfBytesToWrite, lpdwNumberOfBytesWritten);
        if (pFsm != NULL) {
            error = pFsm->QueueWorkItem();
            if ( error == ERROR_IO_PENDING ) {
                fNeedDeref = FALSE;
            }
        } else {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // if we're here then ERROR_SUCCESS cannot have been returned from
        // the above calls
        //

        INET_ASSERT(error != ERROR_SUCCESS);


        DEBUG_PRINT(FTP,
                    INFO,
                    ("processing request asynchronously: error = %d\n",
                    error
                    ));

        goto quit;

        //
        // we're going synchronous. Change error to ERROR_SUCCESS so that we do
        // the right thing at quit
        //

        error = ERROR_SUCCESS;
    }

    INET_ASSERT(error == ERROR_SUCCESS);

    switch (handleType) {
    case TypeFtpFileHandle:
        success = FtpWriteFile(hFileMapped,
                               (LPVOID)lpBuffer,
                               dwNumberOfBytesToWrite,
                               lpdwNumberOfBytesWritten
                               );
        break;

    case TypeHttpRequestHandle:
        error = HttpWriteData(hFileMapped,
                               (LPVOID)lpBuffer,
                               dwNumberOfBytesToWrite,
                               lpdwNumberOfBytesWritten,
                               0
                               );
        //
        // Don't Derefrence if we're going pending cause the FSM will do
        //  it for us.
        //

        if ( error == ERROR_IO_PENDING )
        {
            fNeedDeref = FALSE;
        }
        success = (error == ERROR_SUCCESS) ? TRUE : FALSE;
        //bEndRead = FALSE;
        break;

    case TypeFileRequestHandle:

        success = WriteFile(((INTERNET_FILE_HANDLE_OBJECT *) hFileMapped)->GetFileHandle(),
                               (LPVOID)lpBuffer,
                               dwNumberOfBytesToWrite,
                               lpdwNumberOfBytesWritten,
                               NULL // overlapped I/O
                               );

        if ( !success )
        {
            error = GetLastError();
        }
        else
        {
            error = ERROR_SUCCESS;
        }

        break;

    default:
        error = ERROR_INVALID_HANDLE;
        break;
    }

quit:

    if (hFileMapped != NULL && fNeedDeref) {
        DereferenceObject((LPVOID)hFileMapped);
    }

    _InternetDecNestingCount(nestingLevel);;

done:

    if (error != ERROR_SUCCESS) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
BOOL
WINAPI
InternetWriteFileExA(
    IN HINTERNET hFile,
    IN LPINTERNET_BUFFERSA lpBuffersIn,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )
{
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


INTERNETAPI
DWORD
WINAPI
InternetSetFilePointer(
    IN HINTERNET hFile,
    IN LONG lDistanceToMove,
    IN PVOID pReserved,
    IN DWORD dwMoveMethod,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Sets a file position for InternetReadFile.  It is a synchronous call,
    however subsequent calls to InternetReadFile may block or return
    pending if the data is not available from the cache and the server
    does not support random access.

Arguments:

    hFile
        A valid handle returned from a previous call to InternetOpenUrl
        or a handle returned from HttpOpenRequest for a GET or HEAD method
        and passed to HttpSendRequest.  The handle must have been created
        without INTERNET_FLAG_DONT_CACHE.

    lDistanceToMove

        Specifies the number of bytes to move the file pointer. A positive
        value moves the pointer forward in the file and a negative value
        moves it backward.

pReserved
        Reserved, pass NULL.

dwMoveMethod
        Specifies the starting point for the file pointer move. This
        parameter can be one of the following values:

        Value            Meaning
        FILE_BEGIN       The starting point is zero or the beginning of the file.
                         If FILE_BEGIN is specified, DistanceToMove is interpreted
                         as an unsigned location for the new file pointer.
        FILE_CURRENT     The current value of the file pointer is the starting point.
        FILE_END         The current end-of-file position is the starting point.
                         This method will fail if the content length is unknown.

Return Value:

    -1 on failure, else the current file position.

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Int,
                     "InternetSetFilePointer",
                     "%#x, %#x, %#x, %#x %#x",
                     hFile,
                     lDistanceToMove,
                     pReserved,
                     dwMoveMethod,
                     dwContext
                     ));

    DWORD dwNewPosition = (DWORD) -1L;
    DWORD error;
    HINTERNET hFileMapped = NULL;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    // Validate parameters...

    error = MapHandleToAddress(hFile, &hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL)) {
        goto quit;
    }

    HINTERNET_HANDLE_TYPE handleType;
    error = RGetHandleType(hFileMapped, &handleType);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    switch (handleType) {
        // case TypeFtpFileHandle:
        // case TypeGopherFileHandle:
        case TypeHttpRequestHandle:
            break;
        default:
            error = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
            goto quit;
    }

    HTTP_REQUEST_HANDLE_OBJECT *lpRequest;
    lpRequest = (HTTP_REQUEST_HANDLE_OBJECT *) hFileMapped;
    dwNewPosition = lpRequest->SetStreamPointer (lDistanceToMove, dwMoveMethod);

quit:

    if (hFileMapped != NULL) {
        DereferenceObject((LPVOID)hFileMapped);
    }

done:

    DEBUG_LEAVE_API(dwNewPosition);

    return dwNewPosition;
}



INTERNETAPI
BOOL
WINAPI
InternetQueryDataAvailable(
    IN HINTERNET hFile,
    OUT LPDWORD lpdwNumberOfBytesAvailable,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Determines the amount of data currently available to be read on the handle

Arguments:

    hFile                       - handle of internet object

    lpdwNumberOfBytesAvailable  - pointer to returned bytes available

    dwFlags                     - flags controlling operation - FUTURE

    dwContext                   - used to differentiate multiple requests - FUTURE

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetQueryDataAvailable",
                     "%#x, %#x, %#x, %#x",
                     hFile,
                     lpdwNumberOfBytesAvailable,
                     dwFlags,
                     dwContext
                     ));

    BOOL success;
    DWORD error;
    LPINTERNET_THREAD_INFO lpThreadInfo = NULL;
    HINTERNET hFileMapped = NULL;
    BOOL bDeref = TRUE;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        bDeref = FALSE;
        goto quit;
    }

    INET_ASSERT(hFile);

    //
    // get the per-thread info block
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    //INET_ASSERT(lpThreadInfo->Fsm == NULL);

    PERF_LOG(PE_CLIENT_REQUEST_START,
             AR_INTERNET_QUERY_DATA_AVAILABLE,
             lpThreadInfo->ThreadId,
             hFile
             );

    //
    // validate parameters
    //

    error = MapHandleToAddress(hFile, &hFileMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFileMapped == NULL)) {
        goto quit;
    }

    INET_ASSERT(hFileMapped);

    //
    // set the context and handle values in the per-thread info block (this API
    // can't return extended error info, so we don't care about it)
    //

    _InternetSetContext(lpThreadInfo,
                        ((INTERNET_HANDLE_OBJECT *)hFileMapped)->GetContext()
                        );
    _InternetSetObjectHandle(lpThreadInfo, hFile, hFileMapped);

    //
    // if the handle is invalid, quit now
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate rest of parameters
    //

    error = ProbeAndSetDword(lpdwNumberOfBytesAvailable, 0);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // get the handle type
    //

    HINTERNET_HANDLE_TYPE handleType;

    handleType = ((HANDLE_OBJECT *)hFileMapped)->GetHandleType();

    //
    // find out if we're sync or async
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hFileMapped, &isLocal, &isAsync, TypeWildHandle);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    // WinSE 4998.  If there's no context on the handle, force the request to be synchronous.
    //
    if (isAsync && lpThreadInfo->Context == INTERNET_NO_CALLBACK)
    {
        DEBUG_PRINT(API,
                    ERROR,
                    ("Zero context: Call is Synchronous\n"
                    ));
        isAsync = FALSE;
    }

    //
    // since the async worker thread doesn't come back through this API, the
    // following test is sufficient. Note that we only go async if there is
    // no data currently available on the handle
    //

    BOOL dataAvailable;

    dataAvailable = ((INTERNET_HANDLE_OBJECT *)hFileMapped)->IsDataAvailable();

    BOOL eof;

    eof = ((INTERNET_HANDLE_OBJECT *)hFileMapped)->IsEndOfFile();

    if ((handleType != TypeHttpRequestHandle)
    && isAsync
    && !dataAvailable
    && !eof) {

        INET_ASSERT(hFileMapped);

        // MakeAsyncRequest
        CFsm_InternetQueryDataAvailable * pFsm;

        pFsm = new CFsm_InternetQueryDataAvailable(hFileMapped, lpdwNumberOfBytesAvailable, dwFlags, dwContext);
        if (pFsm != NULL) {
            error = pFsm->QueueWorkItem();
            if (error == ERROR_IO_PENDING) {
                bDeref = FALSE;
            }
        } else {
            error = ERROR_NOT_ENOUGH_MEMORY;
        }

        //
        // if we're here then ERROR_SUCCESS cannot have been returned from
        // the above calls
        //

        INET_ASSERT(error != ERROR_SUCCESS);


        DEBUG_PRINT(FTP,
                    INFO,
                    ("processing request asynchronously: error = %d\n",
                    error
                    ));

        goto quit;

        //
        // we will continue along the synchronous path, in which case we
        // need to set error back to ERROR_SUCCESS so that our exit
        // processing (at quit) does the right thing
        //

        error = ERROR_SUCCESS;
    } else if (dataAvailable || eof) {

        DWORD available;

        available = ((INTERNET_HANDLE_OBJECT *)hFileMapped)->AvailableDataLength();

        //
        // we have immediate data; if the handle type is FTP or gopher find and
        // the data is coming from cache, then we only want to indicate that a
        // single (fixed-length) find structure is available
        //

        switch (((HANDLE_OBJECT *)hFileMapped)->GetHandleType()) {
        case TypeFtpFindHandle:
            available = min(available, sizeof(WIN32_FIND_DATA));
            break;

        case TypeGopherFindHandle:
            available = min(available, sizeof(GOPHER_FIND_DATA));
            break;
        }

        DEBUG_PRINT(API,
                    INFO,
                    ("%d bytes are immediately available\n",
                    available
                    ));

        *lpdwNumberOfBytesAvailable = available;
        success = TRUE;
        goto finish;
    }

    INET_ASSERT(hFileMapped);

    //
    // sync path. wInternetQueryDataAvailable will set the last error code
    // if it fails
    //

    if (handleType == TypeHttpRequestHandle) {
        error = DoFsm(new CFsm_QueryAvailable(lpdwNumberOfBytesAvailable,
                                              dwFlags,
                                              dwContext
                                              ));
        if (error == ERROR_SUCCESS) {
            success = TRUE;
        } else {
            if (error == ERROR_IO_PENDING) {
                bDeref = FALSE;
            }
            goto quit;
        }
    } else {
        success = wInternetQueryDataAvailable(hFileMapped,
                                              lpdwNumberOfBytesAvailable,
                                              dwFlags,
                                              dwContext
                                              );
    }

finish:

    DEBUG_PRINT_API(API,
                    INFO,
                    ("*lpdwNumberOfBytesAvailable (%#x) = %d\n",
                    lpdwNumberOfBytesAvailable,
                    *lpdwNumberOfBytesAvailable
                    ));

    if (bDeref && (hFileMapped != NULL)) {
        DereferenceObject((LPVOID)hFileMapped);
    }

    if (lpThreadInfo) {

        PERF_LOG(PE_CLIENT_REQUEST_END,
                 AR_INTERNET_QUERY_DATA_AVAILABLE,
                 *lpdwNumberOfBytesAvailable,
                 lpThreadInfo->ThreadId,
                 hFile
                 );

    }

    DEBUG_LEAVE_API(success);

    return success;

    //
    // we only come here if we are returning an error before calling
    // wInternetQueryDataAvailable
    //

    INET_ASSERT(error != ERROR_SUCCESS);

quit:

    DEBUG_ERROR(API, error);

    SetLastError(error);
    success = FALSE;

    goto finish;
}


DWORD
CFsm_QueryAvailable::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_QueryAvailable::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    CFsm_QueryAvailable * stateMachine = (CFsm_QueryAvailable *)Fsm;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = QueryAvailable_Fsm(stateMachine);
        break;

    default:
        error = ERROR_INTERNET_INTERNAL_ERROR;
        Fsm->SetDone(ERROR_INTERNET_INTERNAL_ERROR);

        INET_ASSERT(FALSE);

        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
QueryAvailable_Fsm(
    IN CFsm_QueryAvailable * Fsm
    )
{
    DEBUG_ENTER((DBG_INET,
                 Dword,
                 "QueryAvailable_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_QueryAvailable & fsm = *Fsm;
    DWORD error = fsm.GetError();

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    HTTP_REQUEST_HANDLE_OBJECT * pRequest;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)fsm.GetMappedHandle();

    if (fsm.GetState() == FSM_STATE_INIT) {
        error = pRequest->QueryDataAvailable(fsm.m_lpdwNumberOfBytesAvailable);
    }
    if (error == ERROR_SUCCESS) {
        pRequest->SetAvailableDataLength(*fsm.m_lpdwNumberOfBytesAvailable);

        DEBUG_PRINT(INET,
                    INFO,
                    ("%d bytes available\n",
                    *fsm.m_lpdwNumberOfBytesAvailable
                    ));

        fsm.SetApiData(*fsm.m_lpdwNumberOfBytesAvailable);
    }

quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}


INTERNETAPI
BOOL
WINAPI
InternetFindNextFileA(
    IN HINTERNET hFind,
    OUT LPVOID lpBuffer
    )

/*++

Routine Description:

    Returns the next directory entry in the listing identified by the handle

Arguments:

    hFind       - find handle, as returned by e.g. FtpFindFirstFile()

    lpBuffer    - pointer to buffer where next directory entry information will
                  be written. Contents of buffer may be different depending on
                  type of directory request, and protocol used (FTP/gopher/etc.)

Return Value:

    BOOL
        Success - TRUE

        Failure - FALSE. Call GetLastError() for more info

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetFindNextFileA",
                     "%#x, %#x",
                     hFind,
                     lpBuffer
                     ));

    LPINTERNET_THREAD_INFO lpThreadInfo;
    DWORD nestingLevel = 0;
    DWORD error;
    BOOL success = FALSE;
    BOOL fDeref = TRUE;
    HINTERNET hFindMapped = NULL;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // we need the per-thread info block on all paths
    //

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();
    nestingLevel = 1;

    //
    // map the handle
    //

    error = MapHandleToAddress(hFind, (LPVOID *)&hFindMapped, FALSE);
    if ((error != ERROR_SUCCESS) && (hFindMapped == NULL)) {
        goto quit;
    }

    //
    // set the context, handle, and last-error info in the per-thread data block
    // before we go any further. This allows us to return a status in the async
    // case, even if the handle has been closed
    //

    DWORD_PTR context;

    RGetContext(hFindMapped, &context);
    _InternetSetContext(lpThreadInfo, context);
    _InternetSetObjectHandle(lpThreadInfo, hFind, hFindMapped);
    _InternetClearLastError(lpThreadInfo);

    //
    // if MapHandleToAddress() returned a non-NULL object address, but also an
    // error status, then the handle is being closed - quit
    //

    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // retrieve handle type, and validate in the process
    //

    HINTERNET_HANDLE_TYPE handleType;

    error = RGetHandleType(hFindMapped, &handleType);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // get async/sync and local/remote
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hFindMapped, &isLocal, &isAsync, handleType);
    if (error != ERROR_SUCCESS) {

        //
        // this should never happen - we just successfully called
        // RGetHandleType(), so RIsHandleLocal() should have worked too
        //

        DEBUG_PRINT(INET,
                    ERROR,
                    ("RIsHandleLocal() returns %d\n",
                    error
                    ));

        INET_ASSERT(FALSE);

        goto quit;
    }

    //
    // make sure the handle type is valid for this request. We only support
    // FTP find handle and gopher find handle (both raw data)
    //

    if (!((handleType == TypeFtpFindHandle)
    || (handleType == TypeGopherFindHandle))) {
        error = ERROR_INTERNET_INVALID_OPERATION;
        goto quit;
    }

    //
    // if we're not in an async worker thread context then probe the buffer. If
    // we are in the async worker thread context, then we've already validated
    // the buffer. If is has since become invalid, then the app will fail
    //

    if (!lpThreadInfo->IsAsyncWorkerThread) {
        error = ProbeWriteBuffer(lpBuffer,
                                 (handleType == TypeFtpFindHandle)
                                    ? sizeof(WIN32_FIND_DATA)
                                    : sizeof(GOPHER_FIND_DATA)
                                 );
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // if this is an async request and we are not an async worker thread
        // then queue the request
        //

        if (isAsync) {

            // MakeAsyncRequest
            CFsm_InternetFindNextFile * pFsm;

            pFsm = new CFsm_InternetFindNextFile(hFind, lpBuffer);
            if (pFsm != NULL) {
                error = pFsm->QueueWorkItem();
                if ( error == ERROR_IO_PENDING ) {
                    fDeref = FALSE;
                }
            } else {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }

            //
            // if we're here then ERROR_SUCCESS cannot have been returned from
            // the above calls
            //

            INET_ASSERT(error != ERROR_SUCCESS);


            DEBUG_PRINT(FTP,
                        INFO,
                        ("processing request asynchronously: error = %d\n",
                        error
                        ));

            goto quit;

            //
            // we will continue along the synchronous path, in which case we
            // need to set error back to ERROR_SUCCESS so that our exit
            // processing (at quit) does the right thing
            //

            error = ERROR_SUCCESS;
        }
    }

    //
    // dispatch to the underlying API. Return what the API returns, and let
    // the API SetLastError()
    //
    // N.B. We have already checked the handle type above, and we know at
    // this stage that we have a correct handle type
    //

    INET_ASSERT(error == ERROR_SUCCESS);

    switch (handleType) {
    case TypeFtpFindHandle:
        success = FtpFindNextFileA(hFindMapped,
                                   (LPWIN32_FIND_DATA)lpBuffer
                                   );
        break;

    case TypeGopherFindHandle:
        success = GopherFindNextA(hFindMapped,
                                  (LPGOPHER_FIND_DATA)lpBuffer
                                  );
        break;
    }

quit:

    if (hFindMapped != NULL && fDeref) {
        DereferenceObject((LPVOID)hFindMapped);
    }

    _InternetDecNestingCount(nestingLevel);;

done:

    //
    // if error is not ERROR_SUCCESS then this function returning the error,
    // otherwise the error has already been set by the API we called,
    // irrespective of the value of success
    //

    if (error != ERROR_SUCCESS) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }
    DEBUG_LEAVE_API(success);
    return success;
}



INTERNETAPI
BOOL
WINAPI
InternetGetLastResponseInfoA(
    OUT LPDWORD lpdwErrorCategory,
    IN LPSTR lpszBuffer OPTIONAL,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    This function returns the per-thread last internet error description text
    or server response.

    If this function is successful, *lpdwBufferLength contains the string length
    of lpszBuffer.

    If this function returns a failure indication, *lpdwBufferLength contains
    the number of BYTEs required to hold the response text

Arguments:

    lpdwErrorCategory   - pointer to DWORD location where the error catagory is
                          returned

    lpszBuffer          - pointer to buffer where the error text is returned

    lpdwBufferLength    - IN: length of lpszBuffer
                          OUT: number of characters in lpszBuffer if successful
                          else size of buffer required to hold response text

Return Value:

    BOOL
        Success - TRUE
                    lpszBuffer contains the error text. The caller must check
                    *lpdwBufferLength: if 0 then there was no text to return

        Failure - FALSE
                    Call GetLastError() for more information

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetGetLastResponseInfoA",
                     "%#x, %#x, %#x [%d]",
                     lpdwErrorCategory,
                     lpszBuffer,
                     lpdwBufferLength,
                     lpdwBufferLength ? *lpdwBufferLength : 0
                     ));

    DWORD error;
    BOOL success;
    DWORD textLength;
    LPINTERNET_THREAD_INFO lpThreadInfo;

    //
    // validate parameters
    //

    if (IsBadWritePtr(lpdwErrorCategory, sizeof(*lpdwErrorCategory))
    || IsBadWritePtr(lpdwBufferLength, sizeof(*lpdwBufferLength))
    || (ARGUMENT_PRESENT(lpszBuffer)
        ? IsBadWritePtr(lpszBuffer, *lpdwBufferLength)
        : FALSE)) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // if the buffer pointer is NULL then its the same as a zero-length buffer
    //

    if (!ARGUMENT_PRESENT(lpszBuffer)) {
        *lpdwBufferLength = 0;
    } else if (*lpdwBufferLength != 0) {
        *lpszBuffer = '\0';
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {

        DEBUG_PRINT(INET,
                    ERROR,
                    ("failed to get INTERNET_THREAD_INFO\n"
                    ));

        INET_ASSERT(FALSE);

        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    //
    // there may not be any error text for this thread - either no server
    // error/response has been received, or the error text has been cleared by
    // an intervening API
    //

    if (lpThreadInfo->hErrorText != NULL) {

        //
        // copy as much as we can fit in the user supplied buffer
        //

        textLength = lpThreadInfo->ErrorTextLength;
        if (*lpdwBufferLength) {

            LPBYTE errorText;

            errorText = (LPBYTE)LOCK_MEMORY(lpThreadInfo->hErrorText);
            if (errorText != NULL) {
                textLength = min(textLength, *lpdwBufferLength) - 1;
                memcpy(lpszBuffer, errorText, textLength);

                //
                // the error text should always be zero terminated, so the
                // calling app can treat it as a string
                //

                lpszBuffer[textLength] = '\0';

                UNLOCK_MEMORY(lpThreadInfo->hErrorText);

                if (textLength == lpThreadInfo->ErrorTextLength - 1) {
                    error = ERROR_SUCCESS;
                } else {

                    //
                    // returned length is amount of buffer required
                    //

                    textLength = lpThreadInfo->ErrorTextLength;
                    error = ERROR_INSUFFICIENT_BUFFER;
                }
            } else {

                DEBUG_PRINT(INET,
                            ERROR,
                            ("failed to lock hErrorText (%#x): %d\n",
                            lpThreadInfo->hErrorText,
                            GetLastError()
                            ));

                error = ERROR_INTERNET_INTERNAL_ERROR;
            }
        } else {

            //
            // user's buffer is not large enough to hold the info. We'll
            // let them know the required length
            //

            error = ERROR_INSUFFICIENT_BUFFER;
        }
    } else {

        INET_ASSERT(lpThreadInfo->ErrorTextLength == 0);

        textLength = 0;
        error = ERROR_SUCCESS;
    }

    *lpdwErrorCategory = lpThreadInfo->ErrorNumber;
    *lpdwBufferLength = textLength;

    IF_DEBUG(ANY) {
        if ((error == ERROR_SUCCESS)
        || ((textLength != 0) && (lpszBuffer != NULL))) {

            DEBUG_DUMP_API(API,
                           "Last Response Info:\n",
                           lpszBuffer,
                           textLength
                           );

        }
    }

quit:
    success = (error == ERROR_SUCCESS);
    if (!success) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(success);

    return success;
}


INTERNETAPI
INTERNET_STATUS_CALLBACK
WINAPI
InternetSetStatusCallbackCore(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback,
    IN BOOL fType
    )

/*++

Routine Description:

    Sets the status callback function for the DLL or the handle object

Arguments:

    hInternet               - handle of the object for which we wish to set the
                              status callback

    lpfnInternetCallback    - pointer to caller-supplied status function

Return Value:

    FARPROC
        Success - previous status callback function address

        Failure - INTERNET_INVALID_STATUS_CALLBACK. Call GetLastErrorInfo() for
                  more information:

                    ERROR_INVALID_PARAMETER
                        The callback function is invalid

                    ERROR_INTERNET_INCORRECT_HANDLE_TYPE
                        Cannot set the callback on the supplied handle (probably
                        a NULL handle - per-process callbacks no longer
                        supported)

--*/

{
    DWORD dwErr = ERROR_SUCCESS;
    INTERNET_STATUS_CALLBACK previousCallback = INTERNET_INVALID_STATUS_CALLBACK;
    HINTERNET hObjectMapped = NULL;

    if (!GlobalDataInitialized) {
        dwErr = GlobalDataInitialize();
        if (dwErr != ERROR_SUCCESS) {
            goto cleanup;
        }
    }

    if ((lpfnInternetCallback != NULL) && IsBadCodePtr((FARPROC)lpfnInternetCallback))
    {
        dwErr = ERROR_INVALID_PARAMETER;
        goto cleanup;
    }
    if (!hInternet)
    {
        dwErr = ERROR_INTERNET_INCORRECT_HANDLE_TYPE;
        goto cleanup;
    }

    //
    // map the handle
    //

    dwErr = MapHandleToAddress(hInternet, (LPVOID *)&hObjectMapped, FALSE);
    if (dwErr == ERROR_SUCCESS)
    {
        //
        // swap the new and previous handle object status callbacks, ONLY
        // if there are no pending requests on this handle
        //
        previousCallback = lpfnInternetCallback;
        dwErr = RExchangeStatusCallback(hObjectMapped, &previousCallback, fType);
    }

    if (hObjectMapped != NULL) {
        DereferenceObject((LPVOID)hObjectMapped);
    }

cleanup:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(API, dwErr);
    }
    return previousCallback;
}


INTERNETAPI
INTERNET_STATUS_CALLBACK
WINAPI
InternetSetStatusCallbackA(
    IN HINTERNET hInternet,
    IN INTERNET_STATUS_CALLBACK lpfnInternetCallback
    )

/*++

Routine Description:

    Sets the status callback function for the DLL or the handle object

Arguments:

    hInternet               - handle of the object for which we wish to set the
                              status callback

    lpfnInternetCallback    - pointer to caller-supplied status function

Return Value:

    FARPROC
        Success - previous status callback function address

        Failure - INTERNET_INVALID_STATUS_CALLBACK. Call GetLastErrorInfo() for
                  more information:

                    ERROR_INVALID_PARAMETER
                        The callback function is invalid

                    ERROR_INTERNET_INCORRECT_HANDLE_TYPE
                        Cannot set the callback on the supplied handle (probably
                        a NULL handle - per-process callbacks no longer
                        supported)

--*/

{
    DEBUG_ENTER_API((DBG_INET,
                 Pointer,
                 "InternetSetStatusCallbackA",
                 "%#x, %#x",
                 hInternet,
                 lpfnInternetCallback
                 ));

    INTERNET_STATUS_CALLBACK previousCallback = InternetSetStatusCallbackCore(
                                                    hInternet,
                                                    lpfnInternetCallback,
                                                    FALSE
                                                    );

    DEBUG_LEAVE_API(previousCallback);
    return previousCallback;
}


//
//INTERNETAPI
//BOOL
//WINAPI
//InternetCancelAsyncRequest(
//    IN DWORD dwAsyncId
//    )
//
///*++
//
//Routine Description:
//
//    Cancels an outstanding async request
//
//Arguments:
//
//    dwAsyncId   - identifier of the async I/O request
//
//Return Value:
//
//    BOOL
//        Success - TRUE
//                    Request was cancelled
//
//        Failure - FALSE
//                    Call GetLastError() for more information
//
//--*/
//
//{
//    DEBUG_ENTER((DBG_INET,
//                 Bool,
//                 "InternetCancelAsyncRequest",
//                 "%d",
//                 dwAsyncId
//                 ));
//
//    DWORD error;
//    BOOL success;
//
//    error = CancelAsyncRequest(dwAsyncId);
//    if (error != ERROR_SUCCESS) {
//
//        DEBUG_ERROR(INET, error);
//
//        SetLastError(error);
//        success = FALSE;
//    } else {
//        success = TRUE;
//    }
//
//    DEBUG_LEAVE(success);
//
//    return success;
//}

//
// private functions
//


PRIVATE
DWORD
wInternetCloseConnectA(
    IN HINTERNET hConnect,
    IN DWORD dwService
    )

/*++

Routine Description:

    The obverse of InternetConnect(). Closes the handle created in
    InternetConnect()

Arguments:

    hConnect    - protocol handle created in InternetConnect()

    dwService   - service required. Controls type of handle generated.
                  May be one of:
                    - INTERNET_SERVICE_FTP
                    - INTERNET_SERVICE_GOPHER
                    - INTERNET_SERVICE_HTTP

Return Value:

    Success - ERROR_SUCCESS

    Failure - ERROR_INVALID_PARAMETER
                Incorrect dwService parameter (*never* expect this)

              Windows error
              Wininet error
              WSA error
                Error from protocol-specific disconnect function
--*/

{
    DEBUG_ENTER((DBG_INET,
                 Dword,
                 "wInternetCloseConnectA",
                 "%#x, %d",
                 hConnect,
                 dwService
                 ));

    DWORD error;

    switch (dwService) {
    case INTERNET_SERVICE_FTP :
        error = wFtpDisconnect(hConnect, CF_EXPEDITED_CLOSE);
        break;

    case INTERNET_SERVICE_GOPHER :
        //error = wGopherDisconnect(hConnect);
        error = ERROR_SUCCESS;
        break;

    case INTERNET_SERVICE_HTTP:
        //error = wHttpConnectClose((LPHINTERNET)hConnect);
        error = ERROR_SUCCESS;
        break;

    default:
        error = ERROR_INVALID_PARAMETER;
        break;
    }

    DEBUG_LEAVE(error);

    return error;
}


PRIVATE
DWORD
GetEmailNameAndPassword(
    IN OUT LPSTR* lplpszUserName,
    IN OUT LPSTR* lplpszPassword,
    OUT LPSTR EmailName,
    IN DWORD EmailNameLength
    )

/*++

Routine Description:

    Gets the login name and password for the FTP server (but can be used for any
    other protocol)

Arguments:

    lplpszUserName  - IN: pointer to pointer to user name
                      OUT: pointer to pointer to user name; may be modified

    lplpszPassword  - IN: pointer to pointer to password
                      OUT: pointer to pointer to password; may be modified

    EmailName       - pointer to buffer in which to store password if
                      "anonymous" returned for login name

    EmailNameLength - length of EmailName

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INVALID_PARAMETER

--*/

{
    DWORD error;
    LPSTR lpszUserName;
    LPSTR lpszPassword;

    lpszUserName = *lplpszUserName;
    lpszPassword = *lplpszPassword;

    //
    // validate username and password arguments. Valid combinations are:
    // (N.B. NULL means NULL pointer or NUL string)
    //
    //  lpszUsername    lpszPassword    Result
    //
    //      NULL            NULL        "anonymous", "emailname@domain"
    //      !NULL           NULL        lpszUserName, NULL
    //      NULL            !NULL       ERROR
    //      !NULL           !NULL       lpszUserName, lpszPassword
    //

    error = ERROR_SUCCESS;
    if (lpszUserName != NULL) {
        if (IsBadStringPtr(lpszUserName, INTERNET_MAX_USER_NAME_LENGTH)) {
            error = ERROR_INVALID_PARAMETER;
        } else if (*lpszUserName == '\0') {
            lpszUserName = NULL;
        }
    }
    if (error == ERROR_SUCCESS) {
        if (lpszPassword != NULL) {
            if (IsBadStringPtr(lpszPassword, INTERNET_MAX_PASSWORD_LENGTH)) {
                error = ERROR_INVALID_PASSWORD;
            } else if (*lpszPassword == '\0') {
                lpszPassword = NULL;
            }
        }
    }
    if (error == ERROR_SUCCESS) {
        if (lpszPassword == NULL) {
            if (lpszUserName == NULL) {

                DWORD length;

                //
                // both name and password are null pointers. We will convert to
                // "anonymous", "EmailName@DomainName"
                //

                //
                // because we don't require a client to be running TCP/IP, we
                // may be unable to get a domain name. Hence we now require the
                // EmailName entry in the registry to contain the entire
                // EmailName@DomainName string, including the '@'. If this is
                // not available, then we will just return an error
                //

                lpszUserName = "anonymous";
                length = EmailNameLength;
                error = GetMyEmailName(EmailName, &EmailNameLength);
                if (error == ERROR_SUCCESS) {
                    lpszPassword = EmailName;
                }
            } else {
                lpszPassword = "";
            }
        } else if (lpszUserName == NULL) {
            error = ERROR_INVALID_PARAMETER;
        }
    }

    *lplpszUserName = lpszUserName;
    *lplpszPassword = lpszPassword;

    return error;
}


INTERNETAPI
DWORD
WINAPI
InternetAttemptConnect(
    IN DWORD dwReserved
    )

/*++

Routine Description:

    This routine attempts to make a loopback socket
    Clients call this to either invoke the dialdialog or to see whether
    they are connected to the net (??).

    4/29/97 (darrenmi) This function now calls InternetAutodial to see if a
    connection needs to be made.

    This function
Arguments:


    dwReserved  - ?

Return Value:

    DWORD

        Windows error code, or sockets error code

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Dword,
                     "InternetAttemptConnect",
                     "%d",
                     dwReserved
                     ));

    DWORD error = ERROR_SUCCESS;

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    if(!InternetAutodial(0, 0)) {
        error = ERROR_GEN_FAILURE;
    }

quit:
    DEBUG_LEAVE_API(error);
    return error;
}


INTERNETAPI
BOOL
WINAPI
InternetLockRequestFile(
    IN HINTERNET hInternet,
    OUT HANDLE *lphLockReqHandle
    )

/*++

Routine Description:

    This routine allows the caller to place a lock on the file that he is
    using  by doing a CreateFile. This ensures that if this file is associated
    with this url, and another download on this url tries to commit another
    file, then this file won't vanish because the cache does a safe delete
    when updating or deleting the cache entry.
    The caller can then call the InternetUnlockRequestFile to give wininet
    the permission to delete this file if not committed to the cache.

Arguments:
    hInternet            request object which is doing the download
    lphLocReqHandle     place to return LockRequestHandle

Return Value:
    TRUE - Success
    FALSE - failure, GetLastError returns the error code

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetLockRequest",
                     "%#x, %#x",
                     hInternet,
                     lphLockReqHandle
                     ));

    DWORD error, dwSize, dwUrlLenPlus1, dwFileLenPlus1;
    HINTERNET_HANDLE_TYPE handleType;
    HINTERNET hObjectMapped = NULL;
    INTERNET_CONNECT_HANDLE_OBJECT * pConnect;
    LPLOCK_REQUEST_INFO lpLockReqInfo = NULL;
    LPSTR lpSource;
    BOOL locked = FALSE;

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto Cleanup;
        }
    }

    error = MapHandleToAddress(hInternet, (LPVOID *)&hInternet, FALSE);
    if (error == ERROR_SUCCESS) {
        hObjectMapped = hInternet;
        pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)hInternet;
        error = RGetHandleType(hInternet, &handleType);
    }
    if (error != ERROR_SUCCESS) {
        goto Cleanup;
    }

    if ((handleType ==    TypeGenericHandle)||
        (handleType ==    TypeInternetHandle)||
        (handleType ==    TypeFtpConnectHandle)||
        (handleType ==    TypeGopherConnectHandle)||
        (handleType ==    TypeHttpConnectHandle) ||
        (handleType ==    TypeFileRequestHandle)) {

        error = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }

    EnterCriticalSection(&LockRequestFileCritSec);
    locked = TRUE;

    //
    // If a lock handle was already created, simply increment the refcount.
    //

    if(lpLockReqInfo = (LPLOCK_REQUEST_INFO)(pConnect->GetLockRequestHandle())) {

        lpLockReqInfo->dwCount++;
        *lphLockReqHandle = (HANDLE)lpLockReqInfo;
        error = ERROR_SUCCESS;
        goto Cleanup;
    }

    //
    // Record the URL and associated filename in the lock handle.
    //

    lpSource = pConnect->GetDataFileName();
    if (!lpSource) {
        error = ERROR_FILE_NOT_FOUND;
        goto Cleanup;
    }

    dwSize = sizeof(LOCK_REQUEST_INFO)
            +(dwUrlLenPlus1 = lstrlen(pConnect->GetCacheKey())+1)
            +(dwFileLenPlus1 = lstrlen(lpSource)+1)+3;  // atmost 3 bytes slop

    lpLockReqInfo = (LPLOCK_REQUEST_INFO)ALLOCATE_MEMORY(LPTR,  dwSize);
    if (!lpLockReqInfo) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto Cleanup;
    }

    lpLockReqInfo->dwSignature = LOCK_REQUEST_SIGNATURE;
    lpLockReqInfo->dwSize = dwSize;
    lpLockReqInfo->fNoCacheLookup = FALSE;

    memcpy(lpLockReqInfo->rgBuff, pConnect->GetCacheKey(), dwUrlLenPlus1);
    lpLockReqInfo->UrlName = lpLockReqInfo->rgBuff;

    // align filename to DWORD
    lpLockReqInfo->FileName = &(lpLockReqInfo->rgBuff[((dwUrlLenPlus1+sizeof(DWORD)) & ~(3))]);
    memcpy(lpLockReqInfo->FileName, lpSource, dwFileLenPlus1);

    DEBUG_PRINT(INET,
                INFO,
                ("Url==%s, File== %s\n",
                lpLockReqInfo->UrlName,
                lpLockReqInfo->FileName
                ));

    //
    // Open the file so it will not be deleted upon cache entry delete/update.
    //

    lpLockReqInfo->hFile = CreateFile (
                                lpSource,
                                GENERIC_READ,
                                FILE_SHARE_READ|FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                FILE_ATTRIBUTE_NORMAL,
                                NULL );
    if (lpLockReqInfo->hFile == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        goto Cleanup;
    }

    //
    // Set refcount to 2, one for connect handle and the other for lock handle.
    // Whichever one is closed last will perform cleanup.
    //

    lpLockReqInfo->dwCount = 2;
    *lphLockReqHandle = (HANDLE)lpLockReqInfo;
    pConnect->SetLockRequestHandle((HANDLE)lpLockReqInfo);

    // Check to see if this file corresponds to an installed cache
    // entry. If so, set fNoDelete so unlocking cannot delete the file.
    // Note - because installed cache entries are generally not downloaded
    // by wininet this is ok to do - it is only necessary to check if there
    // is a cache entry in the request object and if it is an installed type.
    if (handleType == TypeHttpRequestHandle)
    {
        LPCACHE_ENTRY_INFO pcei;
        pcei = ((HTTP_REQUEST_HANDLE_OBJECT*) pConnect)->GetCacheEntryInfo();
        if (pcei)
        {
            if (pcei->CacheEntryType & INSTALLED_CACHE_ENTRY)
                lpLockReqInfo->fNoDelete = TRUE;
        }
    }

    error = ERROR_SUCCESS;

Cleanup:
    BOOL fRet = (error==ERROR_SUCCESS);

    if (!fRet) {

        if (lpLockReqInfo) {
            FREE_MEMORY(lpLockReqInfo);
        }

        DEBUG_ERROR(API, error);
        SetLastError(error);
    }
    else {
        DEBUG_PRINT(INET,
                    INFO,
                    ("Url==%s, File== %s RefCount=%d, Handle = %#x\n",
                    lpLockReqInfo->UrlName,
                    lpLockReqInfo->FileName,
                    lpLockReqInfo->dwCount,
                    *lphLockReqHandle
                    ));

    }

    if (locked) {
        LeaveCriticalSection(&LockRequestFileCritSec);
    }

    if (hObjectMapped) {
        DereferenceObject((LPVOID)hObjectMapped);
    }

    DEBUG_LEAVE_API(fRet);

    return fRet;
}


INTERNETAPI
BOOL
WINAPI
InternetUnlockRequestFile(
    IN HANDLE hLockHandle
    )

/*++

Routine Description:

    This routine allows the caller to unlock a request file that was locked
    using the InternetLockRequestFile routine. This allows the file
    to be deleted after the request object is long gone.

Arguments:

    hLockHandle     Lock Request Handle that was returned in InternetLockRequestFile

Return Value:

    TRUE - Success
    FALSE - failure, GetLastError returns the error code

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetUnlockRequest",
                     "%#x",
                     hLockHandle
                     ));

    DWORD error, dwUrlLen, dwFileNameLen;
    LPLOCK_REQUEST_INFO lpLockReqInfo;

    if (!GlobalDataInitialized) {
        error = GlobalDataInitialize();
        if (error != ERROR_SUCCESS) {
            goto quit;
        }
    }

    EnterCriticalSection(&LockRequestFileCritSec);

    lpLockReqInfo = (LPLOCK_REQUEST_INFO)hLockHandle;

    __try {
        if (lpLockReqInfo->dwSignature == LOCK_REQUEST_SIGNATURE) {

            DEBUG_PRINT(INET,
                        INFO,
                        ("Url==%s, File== %s, refcount=%d\n",
                        lpLockReqInfo->UrlName,
                        lpLockReqInfo->FileName,
                        lpLockReqInfo->dwCount
                        ));

            if (--lpLockReqInfo->dwCount == 0) {

                if (!CloseHandle(lpLockReqInfo->hFile)) {

                    DEBUG_PRINT(INET,
                                ERROR,
                                ("Error=%d while Closing OpenHandle for file=%s for url=%s\n",
                                GetLastError(),
                                lpLockReqInfo->FileName,
                                lpLockReqInfo->UrlName
                                ));

                    }

                if (!lpLockReqInfo->fNoDelete) {

                    //
                    // Validate URL and filename.
                    //

                    dwUrlLen = lstrlen(lpLockReqInfo->UrlName);
                    dwFileNameLen =  lstrlen(lpLockReqInfo->FileName);

                    //
                    // Check if there is a cache entry for the URL.
                    //

                    DWORD dwSize;
                    LPINTERNET_CACHE_ENTRY_INFO pCEI;
                    char buf[sizeof(INTERNET_CACHE_ENTRY_INFO)+MAX_PATH+1];

                    pCEI = (LPINTERNET_CACHE_ENTRY_INFO)buf;
                    dwSize = sizeof(buf);

                    if (lpLockReqInfo->fNoCacheLookup) {

                        error = ERROR_FILE_NOT_FOUND;

                    } else {
                        // Grab info and
                        // Check if the filename actually matches.
                        error = GetUrlCacheEntryInfoEx(lpLockReqInfo->UrlName,
                                            pCEI,
                                            &dwSize,
                                            NULL,
                                            NULL,
                                            NULL,
                                            INTERNET_CACHE_FLAG_ADD_FILENAME_ONLY) ?
                                     (lstrcmpi(lpLockReqInfo->FileName, pCEI->lpszLocalFileName) ?
                                              ERROR_FILE_NOT_FOUND
                                            : ERROR_SUCCESS)
                                   : GetLastError();
                    } // end else if (!lpLockReqInfo->fNoCacheLookup)

                    if (error != ERROR_SUCCESS) {

                        //
                        // The file was not committed to cache, so attempt to delete it.
                        //

                        DEBUG_PRINT(INET, INFO,("deleting %q\n",lpLockReqInfo->FileName));

                        if (!DeleteFile(lpLockReqInfo->FileName)) {

                            DEBUG_PRINT(INET,
                                        ERROR,
                                        ("Error=%d while deleting file=%s for url=%s\n",
                                        GetLastError(),
                                        lpLockReqInfo->FileName,
                                        lpLockReqInfo->UrlName
                                        ));

                            if (lpLockReqInfo->fNoCacheLookup) {

                                switch (GetLastError()) {
                                    case ERROR_SHARING_VIOLATION:
                                    case ERROR_ACCESS_DENIED:
                                        UrlCacheAddLeakFile (lpLockReqInfo->FileName);
                                }
                            }

                        } // end if (!DeleteFile(...))
                    }
                } // end if (!lpLockReqInfo->fNoDelete)

                FREE_MEMORY(lpLockReqInfo);
                error = ERROR_SUCCESS;

            } else {

                DEBUG_PRINT(INET,
                            INFO,
                            ("Quitting after decrementing refcount, new refcount=%d\n",
                            lpLockReqInfo->dwCount
                            ));

                error = ERROR_SUCCESS;
            }
        } else {
            error = ERROR_INVALID_PARAMETER;
        }
    } __except(EXCEPTION_EXECUTE_HANDLER) {
        error = ERROR_INVALID_PARAMETER;
    }
    ENDEXCEPT

    LeaveCriticalSection(&LockRequestFileCritSec);

quit:
    BOOL fRet = (error==ERROR_SUCCESS);

    if  (fRet) {
        DEBUG_ERROR(API, error);
        SetLastError(error);
    }

    DEBUG_LEAVE_API(fRet);

    return fRet;
}


INTERNETAPI
BOOL
WINAPI
InternetCheckConnectionA(
    IN LPCSTR lpszUrl,
    IN DWORD dwFlags,
    IN DWORD dwReserved
    )

/*++

Routine Description:

    This routine tells the caller whether he can establish a connection to the
    network. If no URL is specified and dwFlags are set to NULL then wininet
        a) checks whether it has an outstanding socket connection and if so
            then the API returns TRUE.
        b) If there are no outstanding socket connections then a check
           is made in the wininet serverdatabase for servers which were
           connected to in the recent past. If one is found then the API returns TRUE.

        If neither a) or b) succeeds the API returns FALSE.

Arguments:

    lpszUrl     this parameter is an indication to the API to attempt
                a specific host. The use of this parameter is based on the
                flags set in the dwFlags parameter

    dwFlags     a bitwise OR of the following flags

                    INTERNET_FLAG_ICC_FORCE_CONNECTION - force a connection
                    A sockets connection is attempted in the following order
                    1) If lpszUrl is non-NULL then a host value is extracted
                       fromt it used that to ping the specific host
                    2) If the lpszUrl parameter is NULL then if there is an
                       entry in the wininet's internal server database for
                       the nearest server, it is used to do the pinging

                    If neither of these are available then ERROR_NOT_CONNECTED is
                    returned in GetLastError()

    dwReserved  reserved

Return Value:

    TRUE - Success
    FALSE - failure, GetLastError returns the error code

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Bool,
                     "InternetCheckConnectionA",
                     "%s %x",
                     lpszUrl,
                     dwFlags
                     ));

    DWORD           dwError = ERROR_SUCCESS;
    LPSTR           lpszHostName;
    DWORD           dwHostNameLen;
    INTERNET_PORT   ServerPort = INTERNET_DEFAULT_HTTP_PORT;
    INTERNET_SCHEME ustScheme = INTERNET_SCHEME_HTTP;
    ICSocket *pSocket = NULL;
    char            buff[INTERNET_MAX_HOST_NAME_LENGTH+1];
    LPINTERNET_THREAD_INFO lpThreadInfo;
    HINTERNET   hInternet = NULL, hConnect = NULL, hConnectMapped = NULL;
    CServerInfo * lpServerInfo = NULL;

    if (!GlobalDataInitialized) {
        dwError = GlobalDataInitialize();
        if (dwError != ERROR_SUCCESS) {
            goto Cleanup;
        }
    }

    // if the main sockets database thinks we are
    // unconditionally offline, then let us give that to the user
    //if(vSocketsDatabase.IsOffline())
    //{
    //
    //    dwError = ERROR_NOT_CONNECTED;
    //}
    //else
    {

        // We are not explicitly in offline mode

        lpServerInfo = FindNearestServer();

        //
        // FindXXXXServer API does not do additional AddRef on ServerInfo
        // so caller has to do it!
        //
        if( lpServerInfo ) {
            lpServerInfo->Reference();
        }

        if (dwFlags & FLAG_ICC_FORCE_CONNECTION) {

            buff[0] = 0;

            if (lpszUrl) {

                if (((dwError = CrackUrl(
                            (LPSTR)lpszUrl,            // url
                            lstrlen(lpszUrl),   // url length
                            FALSE,              // escape the URL ?
                            &ustScheme,         // scheme type
                            NULL,               // scheme name
                            NULL,               // scheme length
                            &lpszHostName,      // hostname pointer
                            &dwHostNameLen,     // hostname length
                            &ServerPort,        // port
                            NULL,               // UserName
                            NULL,               // UserName Length
                            NULL,               // Password
                            NULL,               // Password Length
                            NULL,               // Path
                            NULL,               // Path Length
                            NULL,               // Extra
                            NULL,               // Extra Length
                            NULL                // have port?
                            )) == ERROR_SUCCESS)&&
                                (dwHostNameLen<=INTERNET_MAX_HOST_NAME_LENGTH))
                {
                    memcpy(buff, lpszHostName, dwHostNameLen);
                    buff[dwHostNameLen] = 0;

                    // make sure we have a valid scheme
                    if(INTERNET_SCHEME_UNKNOWN == ustScheme)
                    {
                        ustScheme = INTERNET_SCHEME_HTTP;
                    }

                    // make sure we have a valid port
                    if(0 == ServerPort)
                    {
                        switch(ustScheme)
                        {
                        case INTERNET_SCHEME_FTP:
                            ServerPort = INTERNET_DEFAULT_FTP_PORT;
                            break;
                        case INTERNET_SCHEME_HTTPS:
                            ServerPort = INTERNET_DEFAULT_HTTPS_PORT;
                            break;
                        default:
                            ServerPort = INTERNET_DEFAULT_HTTP_PORT;
                            break;
                        }
                    }

                    // for our purposes, HTTPS == HTTP
                    if(INTERNET_SCHEME_HTTPS == ustScheme)
                    {
                        ustScheme = INTERNET_SCHEME_HTTP;
                    }

                    // we need the serverinfo struct for this server, not
                    // the nearest one that we already found
                    if(lpServerInfo)
                    {
                        lpServerInfo->Dereference();
                    }

                    lpServerInfo = FindServerInfo(buff);
                    if(lpServerInfo)
                    {
                        lpServerInfo->Reference();
                    }
                    else
                    {
                        // new CServerInfo has ref count 1 already
                        lpServerInfo = new CServerInfo(buff, INTERNET_SERVICE_HTTP, 0);
                        if(NULL == lpServerInfo)
                        {
                            dwError = ERROR_NOT_ENOUGH_MEMORY;
                            goto Cleanup;
                        }
                    }
                }
            }
            else {
                if (lpServerInfo) {

                    buff[sizeof(buff)-1] = 0;

                    strncpy(buff, lpServerInfo->GetHostName(), sizeof(buff)-1);
                }
                else
                {
                    // FORCE but no server to try - set error
                    dwError = ERROR_INTERNET_INVALID_OPERATION;
                    goto Cleanup;
                }
            }

            if (buff[0] && lpServerInfo) {

                // we have a host name, ping it.

                // This threadinfo/InternetOpen stuff is being done
                // because the ICSocket class is intertwined with
                // an internet handle, so we are just getting round that
                // difficulty. Ideally, ICSocket should have been a
                // standalone sockets class

                lpThreadInfo = InternetGetThreadInfo();

                if (lpThreadInfo == NULL) {

                    INET_ASSERT(FALSE);

                    dwError = ERROR_INTERNET_INTERNAL_ERROR;
                    goto Cleanup;
                }


                hInternet = InternetOpen("Internal",
                                            0,
                                            NULL,
                                            NULL,
                                            0
                                            );
                if (!hInternet) {
                    dwError = GetLastError();
                    goto Cleanup;
                }

                hConnect = InternetConnect(hInternet,
                                            buff,
                                            ServerPort,
                                            NULL,
                                            NULL,
                                            ustScheme,
                                            0,
                                            0);
                if (!hConnect) {
                    dwError = GetLastError();
                    goto Cleanup;
                }

                dwError = MapHandleToAddress(hConnect, (LPVOID *)&hConnectMapped, FALSE);

                if (dwError != ERROR_SUCCESS) {
                    goto Cleanup;
                }

                _InternetSetObjectHandle(lpThreadInfo, hConnect, hConnectMapped);

                // Ping the server
                if (pSocket = new ICSocket()) {

                    pSocket->SetPort(ServerPort);

                    dwError = pSocket->SocketConnect(
                        GetTimeoutValue(INTERNET_OPTION_CONNECT_TIMEOUT),
                        GetTimeoutValue(INTERNET_OPTION_CONNECT_RETRIES),
                        0,
                        lpServerInfo
                        );

                    if (dwError == ERROR_SUCCESS) {
                        pSocket->Disconnect();
                    }
                }
                else {

                    dwError = ERROR_NOT_ENOUGH_MEMORY;

                }

            }

        }
        else{
            // caller doesn't ask us to force a connection
            // do the best we can and tell him

            dwError = (/*vSocketsDatabase.GetSocketCount() ||*/ lpServerInfo)?
                      ERROR_SUCCESS:
                      ERROR_NOT_CONNECTED;

        }

    }

Cleanup:

    if (lpServerInfo)
    {
        lpServerInfo->Dereference();
    }
    if (pSocket) {

        pSocket->Dereference();

    }
    if (hConnectMapped) {

        DereferenceObject((LPVOID)hConnectMapped);

    }
    if (hConnect) {

        InternetCloseHandle(hConnect);

    }
    if (hInternet) {

        InternetCloseHandle(hInternet);

    }

    BOOL fRet = (dwError==ERROR_SUCCESS);
    if (FALSE == fRet) {
        SetLastError(dwError);
        DEBUG_ERROR(API, dwError);
    }
    DEBUG_LEAVE_API(fRet);

    return (fRet);
}
