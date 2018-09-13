/*++

Copyright (c) 1994 Microsoft Corporation

Module Name:

    open.cxx

Abstract:

    This file contains the implementation of the HttpOpenRequestA API.

    The following functions are exported by this module:

        HttpOpenRequestA
        HttpOpenRequestW
        ParseHttpUrl
        ParseHttpUrl_Fsm

Author:

    Keith Moore (keithmo) 16-Nov-1994

Revision History:

    Modified to make HttpOpenRequestA remotable. madana (2/8/95)

--*/

#include <wininetp.h>
#include "httpp.h"

//
// functions
//


INTERNETAPI
HINTERNET
WINAPI
HttpOpenRequestA(
    IN HINTERNET hConnect,
    IN LPCSTR lpszVerb OPTIONAL,
    IN LPCSTR lpszObjectName OPTIONAL,
    IN LPCSTR lpszVersion OPTIONAL,
    IN LPCSTR lpszReferrer OPTIONAL,
    IN LPCSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Creates a new HTTP request handle and stores the specified parameters
    in that context.

Arguments:

    hConnect            - An open Internet handle returned by InternetConnect()

    lpszVerb            - The verb to use in the request. May be NULL in which
                          case "GET" will be used

    lpszObjectName      - The target object for the specified verb. This is
                          typically a file name, an executable module, or a
                          search specifier. May be NULL in which case the empty
                          string will be used

    lpszVersion         - The version string for the request. May be NULL in
                          which case "HTTP/1.0" will be used

    lpszReferrer        - Specifies the address (URI) of the document from
                          which the URI in the request (lpszObjectName) was
                          obtained. May be NULL in which case no referer is
                          specified

    lplpszAcceptTypes   - Points to a NULL-terminated array of LPCTSTR pointers
                          to content-types accepted by the client. This value
                          may be NULL in which case the default content-type
                          (text/html) is used

    dwFlags             - open options

    dwContext           - app-supplied context value for call-backs

    BUGBUG: WHAT IS THE DEFAULT CONTENT-TRANSFER-ENCODING?

Return Value:

    HINTERNET

        Success - non-NULL (open) handle to an HTTP request

        Failure - NULL. Error status is available by calling GetLastError()

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "HttpOpenRequestA",
                     "%#x, %.80q, %.80q, %.80q, %.80q, %#x, %#08x, %#08x",
                     hConnect,
                     lpszVerb,
                     lpszObjectName,
                     lpszVersion,
                     lpszReferrer,
                     lplpszAcceptTypes,
                     dwFlags,
                     dwContext
                     ));

    DWORD error;
    HINTERNET hConnectMapped = NULL;
    BOOL fRequestUsingProxy;
    HINTERNET hRequest = NULL;

    if (!GlobalDataInitialized) {
        error = ERROR_INTERNET_NOT_INITIALIZED;
        goto done;
    }

    //
    // get the per-thread info
    //

    LPINTERNET_THREAD_INFO lpThreadInfo;

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto done;
    }

    _InternetIncNestingCount();

    //
    // map the handle
    //

    error = MapHandleToAddress(hConnect, (LPVOID *)&hConnectMapped, FALSE);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // find path from internet handle and validate handle
    //

    BOOL isLocal;
    BOOL isAsync;

    error = RIsHandleLocal(hConnectMapped,
                           &isLocal,
                           &isAsync,
                           TypeHttpConnectHandle
                           );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // validate parameters. Allow lpszVerb to default to "GET" if a NULL pointer
    // is supplied
    //

    if (!ARGUMENT_PRESENT(lpszVerb) || (*lpszVerb == '\0')) {
        lpszVerb = DEFAULT_HTTP_REQUEST_VERB;
    }

    //
    // if a NULL pointer or empty string is supplied for the object name, then
    // convert to the default object name (root object)
    //

    if (!ARGUMENT_PRESENT(lpszObjectName) || (*lpszObjectName == '\0')) {
        lpszObjectName = "/";
    }

    //
    // check the rest of the parameters
    //


    if (dwFlags & ~INTERNET_FLAGS_MASK) {
        error = ERROR_INVALID_PARAMETER;
        goto quit;
    }

    //
    // default to the current supported version
    //

    char versionBuffer[sizeof("HTTP/4294967295.4294967295")];
    DWORD verMajor;
    DWORD verMinor;

    if (!ARGUMENT_PRESENT(lpszVersion) || (*lpszVersion == '\0')) {
        wsprintf(versionBuffer,
                 "HTTP/%d.%d",
                 HttpVersionInfo.dwMajorVersion,
                 HttpVersionInfo.dwMinorVersion
                 );
        lpszVersion = versionBuffer;
        verMajor = HttpVersionInfo.dwMajorVersion;
        verMinor = HttpVersionInfo.dwMinorVersion;
    } else if (strnicmp(lpszVersion, "HTTP/", sizeof("HTTP/") - 1) == 0) {

        LPSTR p = (LPSTR)lpszVersion + sizeof("HTTP/") - 1;

        ExtractInt(&p, 0, (LPINT)&verMajor);
        while (!isdigit(*p) && (*p != '\0')) {
            ++p;
        }
        ExtractInt(&p, 0, (LPINT)&verMinor);
    } else {
        verMajor = 1;
        verMinor = 0;
    }

    //
    // if we have HTTP 1.1 enabled in the registry and the version is < 1.1
    // then convert
    //

    if (GlobalEnableHttp1_1
    && (((verMajor == 1) && (verMinor == 0)) || (verMajor < 1))) {
        lpszVersion = "HTTP/1.1";
    }

    //
    // allow empty strings to be equivalent to NULL pointer
    //

    if (ARGUMENT_PRESENT(lpszReferrer) && (*lpszReferrer == '\0')) {
        lpszReferrer = NULL;
    }

    //
    // if the caller has specified CERN proxy access then we convert the
    // object request to the URL that the CERN proxy will use
    //

    INTERNET_CONNECT_HANDLE_OBJECT * pConnect;
    INTERNET_HANDLE_OBJECT * pInternet;
    LPSTR hostName;
    DWORD hostNameLength;
    INTERNET_PORT hostPort;
    BOOL isProxy;
    INTERNET_SCHEME schemeType;
    BOOL bSchemeChanged;

    pConnect = (INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped;
    pInternet = (INTERNET_HANDLE_OBJECT *)pConnect->GetParent();

    INET_ASSERT(pInternet != NULL);
    INET_ASSERT(pInternet->IsValid(TypeInternetHandle) == ERROR_SUCCESS);

    hostName = pConnect->GetHostName(&hostNameLength);
    hostPort = pConnect->GetHostPort();
    isProxy = pConnect->GetServerInfo()->IsCernProxy();
    schemeType = pConnect->GetSchemeType();

    //
    // set the per-thread info: parent handle object and context value
    //

    _InternetSetObjectHandle(lpThreadInfo, hConnect, hConnectMapped);
    _InternetSetContext(lpThreadInfo, dwContext);

    //
    // make local HTTP request handle object before we can add headers to it
    //

    error = RMakeHttpReqObjectHandle(hConnectMapped,
                                     &hRequest,
                                     NULL,  // (CLOSE_HANDLE_FUNC)wHttpCloseRequest
                                     dwFlags,
                                     dwContext
                                     );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    //
    // if the scheme type changed and we are going via proxy, get the SERVER_INFO
    // for the new proxy (if we changed it). N.B. We ONLY change the proxy for
    // this request object, NOT for the connect object
    //

    //if (isProxy && bSchemeChanged) {
    //    ((HTTP_REQUEST_HANDLE_OBJECT *)hRequest)->SetServerInfo(schemeType);
    //}

    HTTP_REQUEST_HANDLE_OBJECT * pRequest;

    pRequest = (HTTP_REQUEST_HANDLE_OBJECT *)hRequest;

    //
    // add the request line
    //

    INET_ASSERT((lpszVerb != NULL) && (*lpszVerb != '\0'));
    INET_ASSERT((lpszObjectName != NULL) && (*lpszObjectName != '\0'));
    INET_ASSERT((lpszVersion != NULL) && (*lpszVersion != '\0'));

    pRequest->LockHeaders();

    //
    // encode the URL-path
    //

    error = pRequest->AddRequest((LPSTR)lpszVerb,
                                 (LPSTR)lpszObjectName,
                                 (LPSTR)lpszVersion
                                 );
    if (error != ERROR_SUCCESS) {
        pRequest->UnlockHeaders();
        goto quit;
    }

    //
    // set the method type from the verb
    //

    pRequest->SetMethodType(lpszVerb);

    //
    // add the headers
    //

    if (lpszReferrer != NULL) {
        error = pRequest->AddRequestHeader(HTTP_QUERY_REFERER,
                                           (LPSTR)lpszReferrer,
                                           lstrlen(lpszReferrer),
                                           0,
                                           CLEAN_HEADER
                                           );
        if (error != ERROR_SUCCESS) {
            pRequest->UnlockHeaders();
            goto quit;
        }
    }

    if (lplpszAcceptTypes != NULL) {
        while (*lplpszAcceptTypes) {
            error = pRequest->AddRequestHeader(HTTP_QUERY_ACCEPT,
                                               (LPSTR)*lplpszAcceptTypes,
                                               lstrlen(*(LPSTR*)lplpszAcceptTypes),
                                               0,
                                               CLEAN_HEADER | COALESCE_HEADER_WITH_COMMA
                                               );
            if (error != ERROR_SUCCESS) {
                pRequest->UnlockHeaders();
                goto quit;
            }
            ++lplpszAcceptTypes;
        }
    }

    INET_ASSERT(error == ERROR_SUCCESS);

    pRequest->UnlockHeaders();

    //
    // change the object state to opened
    //

    pRequest->SetState(HttpRequestStateOpen);
    ((HTTP_REQUEST_HANDLE_OBJECT *)hRequest)->SetRequestUsingProxy(
                                                    FALSE
                                                    );

    switch (pRequest->GetMethodType()) {
    case HTTP_METHOD_TYPE_GET:
    case HTTP_METHOD_TYPE_POST:
        break;

    default:
        dwFlags |= (INTERNET_FLAG_RELOAD | INTERNET_FLAG_NO_CACHE_WRITE);
        break;
    }

    pRequest->SetCacheFlags(dwFlags);

    //
    // if the object name is not set then all cache methods fail
    //

    URLGEN_FUNC fn;

    fn = (URLGEN_FUNC)pHttpGetUrlString;

    //
    // BUGBUG - change prototype to take LPCSTR
    //

    error = pRequest->SetObjectName((LPSTR)lpszObjectName,
                                    NULL,
                                    &fn
                                    );

    //
    // Record whether the original object was empty ("") or slash ("/")
    //

    if (lpszObjectName[0] == '/' &&
        lpszObjectName[1] == 0x00 ) {
        pRequest->SetObjectRoot();
    }

quit:

    _InternetDecNestingCount(1);

done:

    if (error != ERROR_SUCCESS) {
        if (hRequest != NULL) {
            InternetCloseHandle(((HANDLE_OBJECT *)hRequest)->GetPseudoHandle());
        }

        DEBUG_ERROR(HTTP, error);

        SetLastError(error);
        hRequest = NULL;
    } else {

        //
        // success - don't return the object address, return the pseudo-handle
        // value we generated
        //

        hRequest = ((HANDLE_OBJECT *)hRequest)->GetPseudoHandle();
    }

    if (hConnectMapped != NULL) {
        DereferenceObject((LPVOID)hConnectMapped);
    }

    DEBUG_LEAVE_API(hRequest);

    return hRequest;
}


INTERNETAPI
HINTERNET
WINAPI
HttpOpenRequestW(
    IN HINTERNET hConnect,
    IN LPCWSTR lpszVerb,
    IN LPCWSTR lpszObjectName,
    IN LPCWSTR lpszVersion,
    IN LPCWSTR lpszReferrer OPTIONAL,
    IN LPCWSTR FAR * lplpszAcceptTypes OPTIONAL,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    Creates a new HTTP request handle and stores the specified parameters
    in that context.

Arguments:

    hHttpSession        - An open Internet handle returned by InternetConnect()

    lpszVerb            - The verb to use in the request

    lpszObjectName      - The target object for the specified verb. This is
                          typically a file name, an executable module, or a
                          search specifier

    lpszVersion         - The version string for the request

    lpszReferrer        - Specifies the address (URI) of the document from
                          which the URI in the request (lpszObjectName) was
                          obtained. May be NULL in which case no referer is
                          specified

    lplpszAcceptTypes   - Points to a NULL-terminated array of LPCTSTR pointers
                          to content-types accepted by the client. This value
                          may be NULL in which case the default content-type
                          (text/html) is used

    dwFlags             - open options

    dwContext           - app-supplied context value for call-backs

    BUGBUG: WHAT IS THE DEFAULT CONTENT-TRANSFER-ENCODING?

Return Value:

    !NULL - An open handle to an HTTP request.

    NULL - The operation failed. Error status is available by calling
        GetLastError().

Comments:

--*/

{
    DEBUG_ENTER_API((DBG_API,
                     Handle,
                     "HttpOpenRequestW",
                     "%#x, %.80wq, %.80wq, %.80wq, %.80wq, %#x, %#08x, %#08x",
                     hConnect,
                     lpszVerb,
                     lpszObjectName,
                     lpszVersion,
                     lpszReferrer,
                     lplpszAcceptTypes,
                     dwFlags,
                     dwContext
                     ));

    DWORD dwErr = ERROR_SUCCESS;
    HINTERNET hInternet = NULL;
    MEMORYPACKET mpVerb, mpObjectName, mpVersion, mpReferrer;
    MEMORYPACKETTABLE mptAcceptTypes;

    if (lpszVerb)
    {
        ALLOC_MB(lpszVerb,0,mpVerb);
        if (!mpVerb.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszVerb,mpVerb);
    }
    if (lpszObjectName)
    {
        ALLOC_MB(lpszObjectName,0,mpObjectName);
        if (!mpObjectName.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszObjectName,mpObjectName);
    }
    if (lpszVersion)
    {
        ALLOC_MB(lpszVersion,0,mpVersion);
        if (!mpVersion.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszVersion,mpVersion);
    }
    if (lpszReferrer)
    {
        ALLOC_MB(lpszReferrer,0,mpReferrer);
        if (!mpReferrer.psStr)
        {
            dwErr = ERROR_NOT_ENOUGH_MEMORY;
            goto cleanup;
        }
        UNICODE_TO_ANSI(lpszReferrer,mpReferrer);
    }

    // Create a table of ansi strings
    if (lplpszAcceptTypes)
    {
        WORD csTmp=0;
        for (;lplpszAcceptTypes[csTmp];csTmp++);
        mptAcceptTypes.SetUpFor(csTmp);
        for (WORD ce=0; ce < csTmp; ce++)
        {
            mptAcceptTypes.pdwAlloc[ce] = (lstrlenW(lplpszAcceptTypes[ce]) + 1)*sizeof(WCHAR);
            mptAcceptTypes.ppsStr[ce] = (LPSTR)ALLOC_BYTES(mptAcceptTypes.pdwAlloc[ce]*sizeof(CHAR));
            if (!mptAcceptTypes.ppsStr[ce])
            {
                dwErr = ERROR_NOT_ENOUGH_MEMORY;
                goto cleanup;
            }
            mptAcceptTypes.pdwSize[ce] = WideCharToMultiByte(CP_ACP,
                                                                0,
                                                                lplpszAcceptTypes[ce],
                                                                mptAcceptTypes.pdwAlloc[ce]/sizeof(WCHAR),
                                                                mptAcceptTypes.ppsStr[ce],
                                                                mptAcceptTypes.pdwAlloc[ce],NULL,NULL);
        }
    }

    hInternet = HttpOpenRequestA(hConnect, mpVerb.psStr, mpObjectName.psStr, mpVersion.psStr,
                               mpReferrer.psStr, (LPCSTR*)mptAcceptTypes.ppsStr,
                               dwFlags, dwContext);

cleanup:
    if (dwErr!=ERROR_SUCCESS)
    {
        SetLastError(dwErr);
        DEBUG_ERROR(HTTP, dwErr);
    }
    DEBUG_LEAVE_API(hInternet);
    return hInternet;
}


DWORD
ParseHttpUrl(
    IN OUT LPHINTERNET phInternet,
    IN LPSTR lpszUrl,
    IN DWORD dwSchemeLength,
    IN LPSTR lpszHeaders,
    IN DWORD dwHeadersLength,
    IN DWORD dwFlags,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    URL parser for HTTP URLs. Support function for InternetOpenUrl() and
    ParseUrl().

    This is a macro function that just cracks the URL and calls HTTP APIs to
    do the work

Arguments:

    phInternet      - IN: InternetOpen() handle
                      OUT: if successful HttpOpenRequest(), else undefined

    lpszUrl         - pointer to string containing HTTP URL to open

    dwSchemeLength  - length of the URL scheme, exluding "://"

    lpszHeaders     - additional HTTP headers

    dwHeadersLength - length of Headers

    dwFlags         - optional flags for opening a file (cache/no-cache, etc.)

    dwContext       - context value for callbacks

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                    The URL passed in could not be parsed

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "ParseHttpUrl",
                 "%#x [%#x], %q, %d, %.80q, %d, %08x, %08x",
                 phInternet,
                 *phInternet,
                 lpszUrl,
                 dwSchemeLength,
                 lpszHeaders,
                 dwHeadersLength,
                 dwFlags,
                 dwContext
                 ));

    INET_ASSERT(GlobalDataInitialized);

    DWORD error = DoFsm(new CFsm_ParseHttpUrl(phInternet,
                                              lpszUrl,
                                              dwSchemeLength,
                                              lpszHeaders,
                                              dwHeadersLength,
                                              dwFlags,
                                              dwContext
                                              ));

    DEBUG_LEAVE(error);

    return error;
}


DWORD
ParseHttpUrl_Fsm(
    IN CFsm_ParseHttpUrl * Fsm
    )

/*++

Routine Description:

    Run next ParseHttpUrl state. Note this FSM has no RunSM(). Since there is
    no object to instantiate, we don't need one

Arguments:

    Fsm - pointer to FSM controlling operation

Return Value:

    DWORD
        Success - ERROR_SUCCESS

                  ERROR_IO_PENDING

        Failure -

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "ParseHttpUrl_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_ParseHttpUrl & fsm = *Fsm;
    DWORD error = fsm.GetError();
    BOOL success;

    if (fsm.GetState() == FSM_STATE_CONTINUE) {
        goto parse_continue;
    }

    LPSTR userName;
    DWORD userNameLength;
    LPSTR password;
    DWORD passwordLength;
    LPSTR pHostName;
    DWORD hostNameLength;
    DWORD urlLength;
    INTERNET_PORT port;
    LPSTR schemeName;

    //
    // new scheme - we now pass in the entire URL, so find the start of the
    // address info again. Keep a pointer to the scheme (may be different than
    // http:// if going via proxy); we already know the scheme length from the
    // parameters
    //

    schemeName = fsm.m_lpszUrl;
    fsm.m_lpszUrl += fsm.m_dwSchemeLength + sizeof("://") - 1;

    //
    // extract the address information - no user name or password
    //

    error = GetUrlAddress(&fsm.m_lpszUrl,
                          &urlLength,
                          &userName,
                          &userNameLength,
                          &password,
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
    // if we got user name & password, convert them to zero-terminated strings.
    // The URL is a copy, so we can write into it, and the characters that
    // terminate user name & password (@ & : resp) are not significant
    //

    if (userName != NULL) {
        userName[userNameLength] = '\0';
    }
    if (password != NULL) {
        password[passwordLength] = '\0';
    }

    //
    // get the HTTP object path - decode any escape sequences
    //

    //if (*Url != '\0') {
    //
    //    INET_ASSERT((int)urlLength > 0);
    //
    //    error = DecodeUrlStringInSitu(Url, &urlLength);
    //    if (error != ERROR_SUCCESS) {
    //        goto quit;
    //    }
    //}

    //
    // convert the host name pointer and length to an ASCIIZ string
    //

    char hostName[INTERNET_MAX_HOST_NAME_LENGTH + 1];
    DWORD len;

    len = min(hostNameLength, sizeof(hostName) - 1);
    memcpy(hostName, pHostName, len);
    hostName[len] = '\0';

    //
    // make the GET request
    //

    HINTERNET hConnect;
    HINTERNET hRequest;

    fsm.m_hConnect = NULL;
    fsm.m_hRequest = NULL;

    //
    // if there is no port specified and we're sending an FTP or gopher request
    // then we have to map the port, or else we will try to use the HTTP port
    // (80)
    //

    INTERNET_SCHEME scheme;

    scheme = MapUrlSchemeName(schemeName, fsm.m_dwSchemeLength);
    if (port == INTERNET_INVALID_PORT_NUMBER) {
        switch (scheme) {
        case INTERNET_SCHEME_FTP:
            port = INTERNET_DEFAULT_FTP_PORT;
            break;

        case INTERNET_SCHEME_GOPHER:
            port = INTERNET_DEFAULT_GOPHER_PORT;
            break;
        }
    }

    fsm.m_hConnect = InternetConnect(*fsm.m_phInternet,
                                     hostName,
                                     port,
                                     userName,
                                     password,
                                     INTERNET_SERVICE_HTTP,

                                     //
                                     // we can ignore EXISTING_CONNECT for HTTP
                                     // connect handle objects
                                     //

                                     fsm.m_dwFlags & ~INTERNET_FLAG_EXISTING_CONNECT,

                                     //
                                     // we are creating a "hidden" handle - don't
                                     // tell the app about it
                                     //

                                     INTERNET_NO_CALLBACK
                                     );
    if (fsm.m_hConnect != NULL) {

        //
        // set the real scheme type. e.g. we may be performing a CERN proxy
        // request for an FTP site, in which case the real scheme type is FTP
        //

        HINTERNET hConnectMapped;

        error = MapHandleToAddress(fsm.m_hConnect, (LPVOID *)&hConnectMapped, FALSE);
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        ((INTERNET_CONNECT_HANDLE_OBJECT *)hConnectMapped)->SetSchemeType(scheme);

        DereferenceObject((LPVOID)hConnectMapped);

        //
        // create the request object. Both proxy and non-proxy paths submit only
        // the URL-path
        //

        //
        // BUGBUG - this should be fixed in java download code!
        //
        // java downloads (synchronous) are requesting INTERNET_FLAG_EXISTING_CONNECT
        // when they really want INTERNET_FLAG_KEEP_CONNECTION
        //

        if (fsm.m_dwFlags & INTERNET_FLAG_EXISTING_CONNECT) {
            fsm.m_dwFlags |= INTERNET_FLAG_KEEP_CONNECTION;
        }
        fsm.m_hRequest = HttpOpenRequest(fsm.m_hConnect,
                                         NULL,    // default verb is GET
                                         fsm.m_lpszUrl,
                                         NULL,    // default version is HTTP/1.0
                                         NULL,    // default referrer
                                         NULL,    // default accept encodings
                                         fsm.m_dwFlags,
                                         fsm.m_dwContext
                                         );
        if (fsm.m_hRequest != NULL) {
            success = HttpSendRequest(fsm.m_hRequest,
                                      fsm.m_lpszHeaders,
                                      fsm.m_dwHeadersLength,
                                      NULL,
                                      0
                                      );
            if (!success) {
                error = GetLastError();
                if (error == ERROR_IO_PENDING) {
                    goto quit;

parse_continue:

                    //
                    // in the async case, the result from HttpSendRequest() is
                    // returned as a DWORD error. Convert it back to a BOOL
                    //

                    success = (BOOL)(error == ERROR_SUCCESS);
                }
            }
        }
    } else {
        error = GetLastError();
    }
    if (success) {

        //
        // associate the connect handle with the request handle, so that when
        // we close the request handle, the connect handle is also closed
        //

        HINTERNET hConnectMapped = NULL;
        HINTERNET hRequestMapped = NULL;

        error = MapHandleToAddress(fsm.m_hConnect, (LPVOID *)&hConnectMapped, FALSE);
        if (error == ERROR_SUCCESS) {
            error = MapHandleToAddress(fsm.m_hRequest, (LPVOID *)&hRequestMapped, FALSE);
            if (error == ERROR_SUCCESS) {
                RSetParentHandle(hRequestMapped, hConnectMapped, TRUE);

                //
                // return the request handle
                //

                DEBUG_PRINT(HTTP,
                            INFO,
                            ("returning handle %#x\n",
                            fsm.m_hRequest
                            ));

                *fsm.m_phInternet = fsm.m_hRequest;
            }
        }

        //
        // dereference the handles referenced by MapHandleToAddress()
        //

        if (hRequestMapped != NULL) {
            DereferenceObject((LPVOID)hRequestMapped);
        }
        if (hConnectMapped != NULL) {
            DereferenceObject((LPVOID)hConnectMapped);
        }
    }

quit:

    if ((error != ERROR_SUCCESS) && (error != ERROR_IO_PENDING)) {
        if (fsm.m_hRequest != NULL) {
            InternetCloseHandle(fsm.m_hRequest);
        }
        if (fsm.m_hConnect != NULL) {
            InternetCloseHandle(fsm.m_hConnect);
        }
    }
    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();
    }

    DEBUG_LEAVE(error);

    return error;
}
