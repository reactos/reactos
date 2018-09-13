/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    request.cxx

Abstract:

    Contains HTTP utility functions

    Contents:
        pHttpGetUrlLen
        pHttpGetUrlString
        pHttpBuildUrl

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

--*/

#include <wininetp.h>
#include "httpp.h"

//
// functions
//


DWORD
pHttpGetUrlLen(
    IN INTERNET_SCHEME SchemeType,
    IN LPSTR lpszTargetName,
    IN LPSTR lpszObjectName,
    IN DWORD dwPort,
    OUT LPDWORD lpdwUrlLen
    )

/*++

Routine Description:

    This routine finds the length of an HTTP URL from targethostname
    port and the object and returns the length

Arguments:

    SchemeType      - type of scheme for URL

    lpszTargetName  - host name

    lpszObjectName  - URL-path

    dwPort          - port (if not default)

    lpdwUrlLen      - returned URL length

Return Value:

    DWORD

--*/

{
    LPSTR schemeName;
    DWORD schemeLength;

    schemeName = MapUrlScheme(SchemeType, &schemeLength);
    if (schemeName == NULL) {
        return ERROR_INTERNET_UNRECOGNIZED_SCHEME;
    }

    int portLen;

    *lpdwUrlLen = 0;

    if (dwPort) {

        CHAR TcpipPortString[32];

        //itoa(dwPort, TcpipPortString, 10);
        wsprintf(TcpipPortString, "%d", dwPort);
        
        portLen = lstrlen(TcpipPortString);
    } else {
        portLen = 0;
    }

    *lpdwUrlLen = schemeLength
                + sizeof("://")
                + portLen
                + lstrlen(lpszTargetName)
                + lstrlen(lpszObjectName)
                ;

    return ERROR_SUCCESS;
}

DWORD
pHttpGetUrlString(
    IN INTERNET_SCHEME SchemeType,
    IN LPSTR lpszTargetName,
    IN LPSTR lpszCWD,
    IN LPSTR lpszObjectName,
    IN LPSTR lpszExtension,
    IN DWORD dwPort,
    OUT LPSTR * lplpUrlName,
    OUT LPDWORD lpdwUrlLen
    )

/*++

Routine Description:

    This routine returns a LocaAlloc'ed buffer containing an HTTP URL constructed
    from the TargetHost, the ObjectName and the port. The caller is responsible
    for freeing the memory.

Arguments:

    SchemeType      -
    lpszTargetName  -
    lpszCWD         -
    lpszObjectName  -
    lpszExtension   -
    dwPort          -
    lplpUrlName     -
    lpdwUrlLen      -

Return Value:

    DWORD

--*/

{
    DWORD dwError, dwSav, i;
    URL_COMPONENTS sUrlComp;
    char cBuff[INTERNET_MAX_URL_LENGTH];


    INET_ASSERT(lpszCWD == NULL);

    *lplpUrlName = NULL;

    memset(&sUrlComp, 0, sizeof(URL_COMPONENTS));

    sUrlComp.dwStructSize = sizeof(URL_COMPONENTS);
    sUrlComp.nScheme = SchemeType;
    sUrlComp.lpszHostName = lpszTargetName;
    sUrlComp.lpszUrlPath = lpszObjectName;
    sUrlComp.nPort = (INTERNET_PORT)dwPort;

    dwSav = sizeof(cBuff);

    if(!InternetCreateUrl(&sUrlComp, 0, cBuff, &dwSav)){
        dwError = GetLastError();
        goto Cleanup;
    }

    // BUGBUG, this is because InternetCreateUrl is not returning
    // the correct size

    dwSav = strlen(cBuff)+5;

    for(i=0;i<2;++i) {

        *lplpUrlName = (LPSTR)ALLOCATE_MEMORY(LPTR, dwSav);

        if (*lplpUrlName) {

            if(!InternetCanonicalizeUrl(cBuff, *lplpUrlName, &dwSav, ICU_ENCODE_SPACES_ONLY)){

                FREE_MEMORY(*lplpUrlName);

                // general paranoia
                *lplpUrlName = NULL;

                dwError = GetLastError();

                if ((i == 1) || (dwError != ERROR_INSUFFICIENT_BUFFER)) {
                    goto Cleanup;
                }
            }
            else {

                dwError = ERROR_SUCCESS;
                *lpdwUrlLen = dwSav;
                break;

            }
        }
        else {
            SetLastError(dwError = ERROR_NOT_ENOUGH_MEMORY);
            goto Cleanup;
        }
    }



Cleanup:
    if (dwError != ERROR_SUCCESS) {

        INET_ASSERT(!*lplpUrlName);

        *lpdwUrlLen = 0;
    }

    return (dwError);
}

DWORD
pHttpBuildUrl(
    IN INTERNET_SCHEME SchemeType,
    IN LPSTR lpszTargetName,
    IN LPSTR lpszObjectName,
    IN DWORD dwPort,
    IN LPSTR lpszUrl,
    IN OUT LPDWORD lpdwBuffSize
    )

/*++

Routine Description:

    This routine builds an HTTP URL in the buffer passed. If the size is not
    enough it returns ERROR_INSUFFICIENT_BUFFER.

Arguments:

    SchemeType      - type of scheme - http, gopher, etc.

    lpszTargetName  - host name

    lpszObjectName  - URL-path

    dwPort          - port number (if not default)

    lpszUrl         - place to write URL

    lpdwBuffSize    - IN: size of lpszUrl buffer
                      OUT: size of URL written to lpszUrl

Return Value:

    DWORD

--*/

{
    DWORD dwBuffLen;
    DWORD error;

    error = pHttpGetUrlLen(SchemeType,
                           lpszTargetName,
                           lpszObjectName,
                           dwPort,
                           &dwBuffLen
                           );
    if (error != ERROR_SUCCESS) {
        return error;
    }
    if (dwBuffLen > *lpdwBuffSize) {
        return (ERROR_INSUFFICIENT_BUFFER);
    }

    LPSTR schemeName;
    DWORD schemeLength;

    schemeName = MapUrlScheme(SchemeType, &schemeLength);
    if (schemeName == NULL) {

        //
        // should never happen
        //

        INET_ASSERT(FALSE);

        return ERROR_INTERNET_UNRECOGNIZED_SCHEME;
    }

    LPSTR p = lpszUrl;
    int len;
    int urlLength;

    memcpy((LPVOID)p, (LPVOID)schemeName, schemeLength);
    p += schemeLength;
    urlLength = schemeLength;

    memcpy((LPVOID)p, (LPVOID)"://", sizeof("://") - 1);
    p += sizeof("://") - 1;
    urlLength += sizeof("://") - 1;

    len = lstrlen(lpszTargetName);
    memcpy((LPVOID)p, (LPVOID)lpszTargetName, len);
    p += len;
    urlLength += len;

    if (dwPort && (dwPort != INTERNET_DEFAULT_HTTP_PORT)) {

        CHAR TcpipPortString[32];

        //itoa(dwPort, TcpipPortString, 10);
        wsprintf(TcpipPortString, "%d", dwPort);

        INET_ASSERT(TcpipPortString[0] != '\0');

        *p++ = ':';
        len = lstrlen(TcpipPortString);
        memcpy((LPVOID)p, (LPVOID)TcpipPortString, len);
        p += len;
        urlLength += len + 1;
    }

    len = lstrlen(lpszObjectName);
    memcpy((LPVOID)p, (LPVOID)lpszObjectName, len);
    urlLength += len;

    *lpdwBuffSize = urlLength;

    return ERROR_SUCCESS;
}
