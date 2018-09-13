/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    parseurl.cxx

Abstract:

    Contains functions to parse the basic URLs - FTP, Gopher, HTTP.

    An URL parser simply acts as a macro: it must break out the protocol-specific
    information from the URL and initiate opening the identified resource: all
    this can be accomplished by calling the relevant Internet protocol APIs.

    Code in this module is based on RFC1738

    Contents:
        IsValidUrl
        DoesSchemeRequireSlashes
        ParseUrl
        CrackUrl
        EncodeUrlPath
        (HexCharToNumber)
        (NumberToHexChar)
        DecodeUrl
        DecodeUrlInSitu
        DecodeUrlStringInSitu
        GetUrlAddressInfo
        GetUrlAddress
        MapUrlSchemeName
        MapUrlScheme
        MapUrlSchemeToName

Author:

    Richard L Firth (rfirth) 26-Apr-1995

Environment:

    Win32(s) user-mode DLL

Revision History:

    26-Apr-1995
        Created

--*/

#include <wininetp.h>
#include "autodial.h"

//
// private manifests
//

#define RESERVED    SAFE

//
// private macros
//

//#define HEX_CHAR_TO_NUMBER(ch) \
//    ((ch <= '9') \
//        ? (ch - '0') \
//        : ((ch >= 'a') \
//            ? ((ch - 'a') + 10) \
//            : ((ch - 'A') + 10)))

#define NUMBER_TO_HEX_CHAR(n) \
    (((n) <= 9) ? ((char)(n) + '0') : (((char)(n) - 10) + 'A'))

#define IS_UNSAFE_URL_CHARACTER(Char, Scheme) \
    (((UCHAR)(Char) <= 0x20) || ((UCHAR)(Char) >= 0x7f) \
    || (SafetyList[(Char) - 0x21] & (UNSAFE | Scheme)))

//
// private types
//

typedef struct {
    LPSTR SchemeName;
    DWORD SchemeLength;
    INTERNET_SCHEME SchemeType;
    DWORD SchemeFlags;
    BOOL NeedSlashes;
    LPFN_URL_PARSER SchemeParser;
    DWORD OpenFlags;
} URL_SCHEME_INFO;

//
// private prototypes
//

PRIVATE
char
HexCharToNumber(
    IN char ch
    );

PRIVATE
char
NumberToHexChar(
    IN int Number
    );

PRIVATE
DWORD
ParseFileUrl(
    IN OUT LPHINTERNET lphInternet,
    IN LPSTR Url,
    IN DWORD SchemeLength,
    IN LPSTR Headers,
    IN DWORD HeadersLength,
    IN DWORD OpenFlags,
    IN DWORD_PTR Context
    );


//
// private data
//

//
// SafetyList - the list of characters above 0x20 and below 0x7f that are
// classified as safe, unsafe or scheme-specific. Safe characters do not need
// to be escaped for any URL scheme. Unsafe characters must be escaped for all
// URL schemes. Scheme-specific characters need only be escaped for the relevant
// scheme(s)
//

PRIVATE
UCHAR
SafetyList[] = {

    //
    // UNSAFE: 0x00..0x20
    //

    SAFE,                       // 0x21 (!)
    UNSAFE,                     // 0x22 (")
    UNSAFE,                     // 0x23 (#)
    SAFE,                       // 0x24 ($)
    UNSAFE,                     // 0x25 (%)
    RESERVED,                   // 0x26 (&)
    SAFE,                       // 0x27 (')
    SAFE,                       // 0x28 (()
    SAFE,                       // 0x29 ())
    SAFE,                       // 0x2A (*)
    SCHEME_GOPHER,              // 0x2B (+)
    SAFE,                       // 0x2C (,)
    SAFE,                       // 0x2D (-)
    SAFE,                       // 0x2E (.)
    RESERVED,                   // 0x2F (/)
    SAFE,                       // 0x30 (0)
    SAFE,                       // 0x31 (1)
    SAFE,                       // 0x32 (2)
    SAFE,                       // 0x33 (3)
    SAFE,                       // 0x34 (4)
    SAFE,                       // 0x35 (5)
    SAFE,                       // 0x36 (6)
    SAFE,                       // 0x37 (7)
    SAFE,                       // 0x38 (8)
    SAFE,                       // 0x39 (9)
    RESERVED,                   // 0x3A (:)
    RESERVED,                   // 0x3B (;)
    UNSAFE,                     // 0x3C (<)
    RESERVED,                   // 0x3D (=)
    UNSAFE,                     // 0x3E (>)
    RESERVED | SCHEME_GOPHER,   // 0x3F (?)
    RESERVED,                   // 0x40 (@)
    SAFE,                       // 0x41 (A)
    SAFE,                       // 0x42 (B)
    SAFE,                       // 0x43 (C)
    SAFE,                       // 0x44 (D)
    SAFE,                       // 0x45 (E)
    SAFE,                       // 0x46 (F)
    SAFE,                       // 0x47 (G)
    SAFE,                       // 0x48 (H)
    SAFE,                       // 0x49 (I)
    SAFE,                       // 0x4A (J)
    SAFE,                       // 0x4B (K)
    SAFE,                       // 0x4C (L)
    SAFE,                       // 0x4D (M)
    SAFE,                       // 0x4E (N)
    SAFE,                       // 0x4F (O)
    SAFE,                       // 0x50 (P)
    SAFE,                       // 0x51 (Q)
    SAFE,                       // 0x42 (R)
    SAFE,                       // 0x43 (S)
    SAFE,                       // 0x44 (T)
    SAFE,                       // 0x45 (U)
    SAFE,                       // 0x46 (V)
    SAFE,                       // 0x47 (W)
    SAFE,                       // 0x48 (X)
    SAFE,                       // 0x49 (Y)
    SAFE,                       // 0x5A (Z)
    UNSAFE,                     // 0x5B ([)
    UNSAFE,                     // 0x5C (\)
    UNSAFE,                     // 0x5D (])
    UNSAFE,                     // 0x5E (^)
    SAFE,                       // 0x5F (_)
    UNSAFE,                     // 0x60 (`)
    SAFE,                       // 0x61 (a)
    SAFE,                       // 0x62 (b)
    SAFE,                       // 0x63 (c)
    SAFE,                       // 0x64 (d)
    SAFE,                       // 0x65 (e)
    SAFE,                       // 0x66 (f)
    SAFE,                       // 0x67 (g)
    SAFE,                       // 0x68 (h)
    SAFE,                       // 0x69 (i)
    SAFE,                       // 0x6A (j)
    SAFE,                       // 0x6B (k)
    SAFE,                       // 0x6C (l)
    SAFE,                       // 0x6D (m)
    SAFE,                       // 0x6E (n)
    SAFE,                       // 0x6F (o)
    SAFE,                       // 0x70 (p)
    SAFE,                       // 0x71 (q)
    SAFE,                       // 0x72 (r)
    SAFE,                       // 0x73 (s)
    SAFE,                       // 0x74 (t)
    SAFE,                       // 0x75 (u)
    SAFE,                       // 0x76 (v)
    SAFE,                       // 0x77 (w)
    SAFE,                       // 0x78 (x)
    SAFE,                       // 0x79 (y)
    SAFE,                       // 0x7A (z)
    UNSAFE,                     // 0x7B ({)
    UNSAFE,                     // 0x7C (|)
    UNSAFE,                     // 0x7D (})
    UNSAFE                      // 0x7E (~)

    //
    // UNSAFE: 0x7F..0xFF
    //

};

//
// UrlSchemeList - the list of schemes that we support
//

PRIVATE
URL_SCHEME_INFO
UrlSchemeList[] = {
    NULL,           0,  INTERNET_SCHEME_DEFAULT,    0,              FALSE,  NULL,           0,
    "ftp",          3,  INTERNET_SCHEME_FTP,        SCHEME_FTP,     TRUE,   ParseFtpUrl,    0,
    "gopher",       6,  INTERNET_SCHEME_GOPHER,     SCHEME_GOPHER,  TRUE,   ParseGopherUrl, 0,
    "http",         4,  INTERNET_SCHEME_HTTP,       SCHEME_HTTP,    TRUE,   ParseHttpUrl,   0,
    "https",        5,  INTERNET_SCHEME_HTTPS,      SCHEME_HTTP,    TRUE,   ParseHttpUrl,   INTERNET_FLAG_SECURE,
    "file",         4,  INTERNET_SCHEME_FILE,       0,              TRUE,   ParseFileUrl,   0,
    "news",         4,  INTERNET_SCHEME_NEWS,       0,              FALSE,  NULL,           0,
    "mailto",       6,  INTERNET_SCHEME_MAILTO,     0,              FALSE,  NULL,           0,
    "socks",        5,  INTERNET_SCHEME_SOCKS,      0,              FALSE,  NULL,           0,
    "javascript",   10, INTERNET_SCHEME_JAVASCRIPT, 0,              FALSE,  NULL,           0,
    "vbscript",     8,  INTERNET_SCHEME_VBSCRIPT,   0,              FALSE,  NULL,           0
};

#define NUMBER_OF_URL_SCHEMES   ARRAY_ELEMENTS(UrlSchemeList)

BOOL ScanSchemes(LPTSTR pszToCheck, DWORD ccStr, PDWORD pwResult)
{
    for (DWORD i=0; i<NUMBER_OF_URL_SCHEMES; i++)
    {
        if ((UrlSchemeList[i].SchemeLength == ccStr)
            && (strnicmp(UrlSchemeList[i].SchemeName, pszToCheck, ccStr)==0))
        {
            *pwResult = i;
            return TRUE;
        }
    }
    return FALSE;
}

//
// functions
//


BOOL
IsValidUrl(
    IN LPCSTR lpszUrl
    )

/*++

Routine Description:

    Determines whether an URL has a valid format

Arguments:

    lpszUrl - pointer to URL to check.

    Assumes:    1. lpszUrl is non-NULL, non-empty string

Return Value:

    BOOL

--*/

{
    INET_ASSERT(lpszUrl != NULL);
    INET_ASSERT(*lpszUrl != '\0');

    while (*lpszUrl != '\0') {
        if (IS_UNSAFE_URL_CHARACTER(*lpszUrl, SCHEME_ANY)) {
            return FALSE;
        }
        ++lpszUrl;
    }
    return TRUE;
}


BOOL
DoesSchemeRequireSlashes(
    IN LPSTR lpszScheme,
    IN DWORD dwSchemeLength,
    IN BOOL bHasHostName
    )

/*++

Routine Description:

    Determines whether a protocol scheme requires slashes

Arguments:

    lpszScheme      - pointer to protocol scheme in question
                      (does not include ':' or slashes, just scheme name)

    dwUrlLength     - if not 0, string length of lpszScheme

Return Value:

    BOOL

--*/

{
    DWORD i;

    //
    // if dwSchemeLength is 0 then lpszUrl is ASCIIZ. Find its length
    //

    if (dwSchemeLength == 0) {
        dwSchemeLength = strlen(lpszScheme);
    }

    if (ScanSchemes(lpszScheme, dwSchemeLength, &i))
    {
        return UrlSchemeList[i].NeedSlashes;
    }
    return bHasHostName;
}



PRIVATE
DWORD
ParseFileUrl(
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

    URL parser for generic File URLs. Support function for InternetOpenUrl() and
    ParseUrl().

    This is a macro function that just cracks the URL and calls Win32 File APIs to
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
                 "ParseFileUrl",
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


    DWORD urlLength;
    DWORD error = ERROR_SUCCESS;
    HINTERNET hInternetMapped = NULL;
    HANDLE hFileHandle;
    INTERNET_FILE_HANDLE_OBJECT *pFileHandleObj;

    //
    // parse "file://" component of the Url.
    //

    Url += SchemeLength + sizeof("://") - 1;

    //
    // BUGBUG - this function should receive the handle already mapped
    //

    error = MapHandleToAddress(*lphInternet, (LPVOID *)&hInternetMapped, FALSE);
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    INET_ASSERT(hInternetMapped != NULL);

    hFileHandle = CreateFile( Url,
                              GENERIC_READ,
                              FILE_SHARE_READ,
                              NULL, // security attributes
                              OPEN_EXISTING,
                              FILE_FLAG_SEQUENTIAL_SCAN/*FILE_ATTRIBUTE_NORMAL*/,
                              NULL // handle to file attributes
                              );

    if ( hFileHandle == INVALID_HANDLE_VALUE)
    {
        error = GetLastError();
        goto quit;
    }

    pFileHandleObj = new INTERNET_FILE_HANDLE_OBJECT(
                                (INTERNET_HANDLE_OBJECT *)hInternetMapped,
                                Url,
                                hFileHandle,
                                OpenFlags,
                                Context
                                );

    if ( pFileHandleObj == NULL)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    //
    // Link File Handle object to Internet Open handle.
    //

    RSetParentHandle(pFileHandleObj, hInternetMapped, FALSE);

    //
    // return the new file handle
    //

    *lphInternet = pFileHandleObj->GetPseudoHandle();

quit:

    if ( hInternetMapped != NULL )
    {
        DereferenceObject((LPVOID)hInternetMapped);
    }

    DEBUG_LEAVE(error);

    return error;
}

DWORD
CFsm_ParseUrlForHttp::ScanProxyUrl(
    IN CFsm_ParseUrlForHttp * Fsm
    )
/*++

Routine Description:

    Attempts to Determine what Protocol should be used for a given
      navigation on an URL, maps the scheme (FTP, HTTP, etc) to
      protocol parser

Arguments:

    Fsm - Containing the current request info

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                    The URL passed in could not be parsed

--*/


{
    DEBUG_ENTER((DBG_UTIL,
                 Dword,
                 "ScanProxyUrl",
                 "%#x",
                 Fsm
                 ));

    CFsm_ParseUrlForHttp & fsm = *Fsm;
    DWORD error = ERROR_SUCCESS;

    //
    // get parser based on the protocol name
    //

    for (fsm.m_dwSchemeLength = 0; fsm.m_lpcszUrl[fsm.m_dwSchemeLength] != ':'; ++fsm.m_dwSchemeLength) {
        if (fsm.m_lpcszUrl[fsm.m_dwSchemeLength] == '\0') {

            //
            // no ':' in URL
            //

            error = ERROR_INTERNET_UNRECOGNIZED_SCHEME;
            goto quit;
        }
    }

    DWORD i;

    if (ScanSchemes((LPSTR)fsm.m_lpcszUrl, fsm.m_dwSchemeLength, &i))
    {
        fsm.m_SchemeType = UrlSchemeList[i].SchemeType;
        fsm.m_dwFlags   |= UrlSchemeList[i].OpenFlags;
        fsm.m_pUrlParser = UrlSchemeList[i].SchemeParser;
    }

    //
    // we are only supporting Internet URL schemes, so the next token in the
    // URL should be "://"
    //

    if ((fsm.m_pUrlParser == NULL) ||
        (memcmp(&fsm.m_lpcszUrl[fsm.m_dwSchemeLength], "://", 3) != 0))
    {
        error = ERROR_INTERNET_UNRECOGNIZED_SCHEME;
        goto quit;
    }


    //
    // allocate a new URL that can be overwritten by the parsers (to avoid
    // having to allocate buffers/use stack buffers for the various sub-
    // strings contained within the URL)
    //

    fsm.m_lpszUrlCopy = NEW_STRING((LPSTR)fsm.m_lpcszUrl);
    if (fsm.m_lpszUrlCopy == NULL)
    {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

quit:

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CFsm_ParseUrlForHttp::BuildProxyMessage(
    IN CFsm_ParseUrlForHttp * Fsm
    )

/*++

Routine Description:

    Assembles the necessary parsed URL info that will be used
      for resolving and requesting proxy info

Arguments:

    Fsm - Containing the current request info

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                    The URL passed in could not be parsed

--*/

{
    DWORD error = ERROR_SUCCESS;

    CFsm_ParseUrlForHttp & fsm = *Fsm;

    LPSTR currentUrl;
    DWORD currentUrlLength;
    INTERNET_SCHEME currentScheme;
    LPSTR currentHostName;
    DWORD currentHostNameLength;
    INTERNET_PORT currentHostNamePort;

    //
    // Gather the URL off the handle
    //

    currentUrl = (LPSTR) fsm.m_lpcszUrl;
    currentUrlLength = lstrlen(currentUrl);

    //
    // crack the current URL
    //

    error = CrackUrl(currentUrl,
                     currentUrlLength,
                     FALSE, // don't escape URL-path
                     &currentScheme,
                     NULL,  // don't care about Scheme Name
                     NULL,
                     &currentHostName,
                     &currentHostNameLength,
                     &currentHostNamePort,
                     NULL,  // don't care about user name
                     NULL,
                     NULL,  // or password
                     NULL,
                     NULL,
                     NULL,
                     NULL,  // no extra
                     NULL,
                     NULL
                     );

    if ( error == ERROR_SUCCESS )
    {
        fsm.m_pProxyInfoQuery->SetProxyMsg(
            currentScheme,
            currentUrl,
            currentUrlLength,
            currentHostName,
            currentHostNameLength,
            currentHostNamePort
            );
    }

    return error;
}

DWORD
CFsm_ParseUrlForHttp::QueryProxySettings(
    IN CFsm_ParseUrlForHttp * Fsm,
    IN BOOL fCallback
    )
/*++

Routine Description:

    Calls internal proxy methods to determine what
     proxy (if any) a given request should use

Arguments:

    Fsm - Containing the current request info

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                    The URL passed in could not be parsed

--*/

{
    DWORD error = ERROR_SUCCESS;

    CFsm_ParseUrlForHttp & fsm = *Fsm;
    INTERNET_HANDLE_OBJECT * pMapped = (INTERNET_HANDLE_OBJECT *)fsm.m_hInternetMapped;

    if ( ! fCallback ) {
        fsm.m_pProxyInfoQuery->SetBlockUntilCompletetion(TRUE);
    }

    error = pMapped->GetProxyInfo(
                &fsm.m_pProxyInfoQuery
                );

    if ( error != ERROR_SUCCESS ) {
        goto quit;
    }

    if ( fsm.m_pProxyInfoQuery->IsUseProxy() &&
          (fsm.m_pProxyInfoQuery->GetProxyScheme() == INTERNET_SCHEME_HTTP ||
           fsm.m_pProxyInfoQuery->GetProxyScheme() == INTERNET_SCHEME_DEFAULT))
    {
        fsm.m_pUrlParser = ParseHttpUrl;
    }

quit:

    return error;
}


DWORD
CFsm_ParseUrlForHttp::CompleteParseUrl(
    IN CFsm_ParseUrlForHttp * Fsm,
    IN LPINTERNET_THREAD_INFO lpThreadInfo,
    IN DWORD error
    )

/*++

Routine Description:

    Handles the return code path after a protcol parser is
        called.  Needs to patch up ref counts and handle
        Html Handle types.

Arguments:

    Fsm - Containing the current request info
    lpThreadInfo - thread info
    error - Current error code in the parser

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                    The URL passed in could not be parsed

--*/

{
    HINTERNET hUrlMapped = NULL;
    CFsm_ParseUrlForHttp & fsm = *Fsm;

    //
    // use our handle value in case the caller doesn't expect the input
    // handle to be modified
    //

    if (error == ERROR_SUCCESS) {

        //
        // map the new URL (Ftp,Gopher,etc) handle
        //

        error = MapHandleToAddress(fsm.m_hInternet, (LPVOID *)&hUrlMapped, FALSE);
        if (error != ERROR_SUCCESS) {
            goto quit;
        }

        //
        // we successfully created the handle object. If we are to return
        // HTML data (default) for this handle then we need to modify the
        // handle object to be the correct type
        //

        if ((fsm.m_pUrlParser != ParseHttpUrl) &&
           !(fsm.m_dwFlags & INTERNET_FLAG_RAW_DATA)) {

            DWORD handleError;

            //
            // RSetHtmlHandleType() does the right thing - it will set the
            // handle object to be HTML if the handle object is of a type
            // which can be set to HTML. Otherwise it will return an error
            //

            handleError = RSetHtmlHandleType(hUrlMapped);

            //
            // if ERROR_INTERNET_INVALID_OPERATION is returned then that is
            // OK, it just means the handle is not a type that we can convert
            // to HTML. Subsequent calls to InternetReadFile() will go the
            // normal route
            //

            if (handleError != ERROR_INTERNET_INVALID_OPERATION) {
                error = handleError;

                INET_ASSERT((error == ERROR_SUCCESS) || (error == ERROR_INVALID_HANDLE));

                //
                // if we successfully set the HTML handle type then we need
                // to associate the URL with the handle - we need the URL
                // when we create the HTML document in InternetReadFile().
                // RSetUrl() will create a copy of the URL and attach it to
                // the handle object
                //

                if (error == ERROR_SUCCESS) {
                    error = RSetUrl(hUrlMapped, (LPSTR)fsm.m_lpszUrlCopy);

                    INET_ASSERT(error == ERROR_SUCCESS);
                }
            }
        }

        //
        // BUGBUG - we may have some cleaning up to do if RSetHtmlHandleType()
        //          returns ERROR_INVALID_HANDLE (couldn't happen!) or if
        //          RSetUrl() fails (also couldn't happen!)
        //

    }

    //
    // Non-HTTP based parsers can sometimes polute the thread info
    //  structure with the wrong handle value (i.e. they may leave a Find handle,
    //   instead of the internet handle)
    //

    if (lpThreadInfo->IsAsyncWorkerThread)
    {
        _InternetSetObjectHandle(lpThreadInfo, fsm.m_hInternetCopy, fsm.m_hInternetMapped);
        _InternetSetContext(lpThreadInfo, fsm.m_dwContext);
    }

quit:

    if (hUrlMapped != NULL) {
        DereferenceObject((LPVOID)hUrlMapped);
    }

    return error;
}


#ifdef EXTENDED_ERROR_HTML
#error Not completed
DWORD
CFsm_ParseUrlForHttp::ExtendedErrorHtml(
    DWORD error
    )
{
    //
    // if we got extended error info AND we requested an FTP entity AND
    // we are not returning raw data then we will return the error info
    // as HTML. We need to generate a handle for this info
    //

    if ((error == ERROR_INTERNET_EXTENDED_ERROR)
    && !(OpenFlags & INTERNET_FLAG_RAW_DATA)
    && (protocolScheme == INTERNET_SCHEME_FTP)) {

        //
        // first we have to create a dummy connect handle object
        //

        HINTERNET hConnect = NULL;

        InternetSetObjectHandle(pMapped->GetPseudoHandle(), hMapped);
        InternetSetContext(INTERNET_NO_CALLBACK);
        error = RMakeInternetConnectObjectHandle(
            pMapped,
            &hConnect,
            NULL,                   // close function
            //hostName,               // server name
            NULL,
            0,                      // server port
            NULL,                   // user name
            NULL,                   // password
            INTERNET_SERVICE_FTP,
            0,                      // flags
            INTERNET_NO_CALLBACK    // context
            );
        if (error == ERROR_SUCCESS) {
            InternetSetObjectHandle(
                ((HANDLE_OBJECT *)hConnect)->GetPseudoHandle(),
                hConnect
                );
            InternetSetContext(Context);
            error = RMakeFtpErrorObjectHandle(hConnect, hInternet);
        }

        //
        // if we failed to generate the handle then just return the
        // original error & let the caller sort it out
        //

        if (error == ERROR_SUCCESS) {

            //
            // link child & parent so that InternetCloseHandle() on the
            // child (error) handle also derefs the parent (connect)
            // handle
            //

            RSetParentHandle(*hInternet, hConnect, TRUE);

            //
            // return the pseudo handle, not the object address
            //

            *hInternet = ((HANDLE_OBJECT *)(*hInternet))->GetPseudoHandle();
        } else {
            if (hConnect != NULL) {
                _InternetCloseHandleNoContext(
                    ((HANDLE_OBJECT *)hConnect)->GetPseudoHandle()
                    );
            }
            error = ERROR_INTERNET_EXTENDED_ERROR;
        }
    }
}
#endif // def EXTENDED_ERROR_HTML





DWORD
CFsm_ParseUrlForHttp::RunSM(
    IN CFsm * Fsm
    )
{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "CFsm_ParseUrlForHttp::RunSM",
                 "%#x",
                 Fsm
                 ));

    DWORD error;
    CFsm_ParseUrlForHttp * stateMachine = (CFsm_ParseUrlForHttp *)Fsm;

    switch (Fsm->GetState()) {
    case FSM_STATE_INIT:
    case FSM_STATE_CONTINUE:
        error = ParseUrlForHttp_Fsm(stateMachine);
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



DWORD
ParseUrlForHttp_Fsm(
    IN CFsm_ParseUrlForHttp * Fsm
    )

/*++

Routine Description:

    The Root FSM that handles HTTP, FTP, and all other protcols
        for InternetOpenUrl

Arguments:

    Fsm - Containing the current request info

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                    The URL passed in could not be parsed

--*/

{
    DEBUG_ENTER((DBG_UTIL,
                 Dword,
                 "ParseUrlForHttp_Fsm",
                 "%#x",
                 Fsm
                 ));

    CFsm_ParseUrlForHttp & fsm = *Fsm;
    DWORD error = fsm.GetError();
    FSM_STATE state = fsm.GetState();
    LPINTERNET_THREAD_INFO lpThreadInfo = NULL;
    BOOL fOnApiCall = fsm.IsOnApiCall(); // Is the API on the stack?

    AUTO_PROXY_ASYNC_MSG proxyInfoQuery;
    DWORD schemeLength;
    INTERNET_SCHEME protocolScheme;
    INTERNET_HANDLE_OBJECT * pMapped = (INTERNET_HANDLE_OBJECT *)fsm.m_hInternetMapped;

    fsm.ClearOnApiCall();

    if (state != FSM_STATE_INIT)
    {
        state = fsm.GetFunctionState();
    }

    lpThreadInfo = InternetGetThreadInfo();
    if (lpThreadInfo == NULL) {
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    switch (state)
    {
        case FSM_STATE_INIT:
        case FSM_STATE_1:
            {
                fsm.SetFunctionState(FSM_STATE_1);

                error = fsm.ScanProxyUrl(Fsm);

                if ( error != ERROR_SUCCESS) {
                    goto quit;
                }

                fsm.m_pProxyInfoQuery = &proxyInfoQuery;

                error = fsm.BuildProxyMessage(Fsm);

                if ( error != ERROR_SUCCESS ) {
                    goto quit;
                }


                if (fsm.m_pUrlParser == ParseFileUrl)
                {
                    LPFN_URL_PARSER parser = fsm.m_pUrlParser;

                    //
                    // call the FILE Parser right away
                    //

                    error = parser(&fsm.m_hInternet,
                                   fsm.m_lpszUrlCopy,
                                   fsm.m_dwSchemeLength,
                                   (LPSTR)fsm.m_lpcszHeaders,
                                   fsm.m_dwHeadersLength,
                                   fsm.m_dwFlags,
                                   fsm.m_dwContext
                                   );
                    goto quit;
                }

                if(0 == (pMapped->GetInternetOpenFlags() & INTERNET_FLAG_OFFLINE))
                {
                    InternetAutodialIfNotLocalHost(fsm.m_lpszUrlCopy, NULL);
                }

                fsm.SetFunctionState(FSM_STATE_2);
                error = fsm.QueryProxySettings(Fsm, FALSE);
                if ( error != ERROR_SUCCESS ) {
                    goto quit;
                } else {
                    fsm.SetFunctionState(FSM_STATE_3);
                }

                // fall through...
            }

        case FSM_STATE_2:
            {
                if ( fsm.GetFunctionState() == FSM_STATE_2 ) {
                    fsm.SetFunctionState(FSM_STATE_3);
                    error = fsm.QueryProxySettings(Fsm, TRUE);
                    if ( error != ERROR_SUCCESS ) {
                        goto quit;
                    }
                }

                // fall through...
            }

        case FSM_STATE_3:
            {

                fsm.SetFunctionState(FSM_STATE_4);

                if ( fsm.m_pUrlParser == ParseHttpUrl)
                {
                    DWORD dwFlags = fsm.m_dwFlags;

                    //
                    // call the protocol-specific parser to open the entity identified
                    // by the URL
                    //

                    fsm.SetBlocking(FALSE);
                    _InternetIncNestingCount();

                    if  (dwFlags & INTERNET_FLAG_RESYNCHRONIZE
                        && (fsm.m_pProxyInfoQuery->GetUrlScheme() == INTERNET_SCHEME_FTP
                        ||  fsm.m_pProxyInfoQuery->GetUrlScheme() == INTERNET_SCHEME_GOPHER)) {

                        //
                        // For ftp and gopher via proxy, there is no if-modified-since,
                        // so we force a reload, otherwise proxy may serve stale data.
                        //

                        dwFlags |= INTERNET_FLAG_RELOAD;
                    }

                    //
                    // remove local proxy info, before going pending
                    //

                    if ( !fsm.m_pProxyInfoQuery->IsAlloced()) {
                        fsm.m_pProxyInfoQuery = NULL;
                    }

                    error = DoFsm(new CFsm_ParseHttpUrl(&fsm.m_hInternet,
                                                        fsm.m_lpszUrlCopy,
                                                        fsm.m_dwSchemeLength,
                                                        (LPSTR)fsm.m_lpcszHeaders,
                                                        fsm.m_dwHeadersLength,
                                                        dwFlags,
                                                        fsm.m_dwContext
                                                        ));
                    _InternetDecNestingCount(1);
                    if (error == ERROR_IO_PENDING) {
                        goto quit;
                    }
                }
                else if (fsm.m_pUrlParser != NULL)
                {
                    LPFN_URL_PARSER parser = fsm.m_pUrlParser;

                    fsm.SetBlocking(TRUE);
                    _InternetIncNestingCount();

                    //
                    // call the protocol-specific parser to open the entity identified
                    // by the URL
                    //

                    error = parser(&fsm.m_hInternet,
                                   fsm.m_lpszUrlCopy,
                                   fsm.m_dwSchemeLength,
                                   (LPSTR)fsm.m_lpcszHeaders,
                                   fsm.m_dwHeadersLength,
                                   fsm.m_dwFlags,
                                   fsm.m_dwContext
                                   );

#ifdef EXTENDED_ERROR_HTML
                    error = fsm.ExtendedErrorHtml(Fsm, error);
#endif
                    _InternetDecNestingCount(1);

                    //if ( error != ERROR_SUCCESS ) {
                    //    goto quit;
                    //}

                }
                else
                {
                    error = ERROR_INTERNET_UNRECOGNIZED_SCHEME;
                }
            }

        case FSM_STATE_4:
            {

                error = fsm.CompleteParseUrl(Fsm, lpThreadInfo, error);

                //
                // and free the copy of the URL
                //

                if ( fsm.m_lpszUrlCopy ) {
                    DEL_STRING(fsm.m_lpszUrlCopy);
                }

                //if (fsm.m_pProxyState) {
                //    if (pMapped->RedoSendRequest(&error, fsm.m_pProxyState)) {
                //        fsm.SetNextState(FSM_STATE_INIT);
                //        goto retry_with_different_proxy;
                //    }
                //    fsm.m_pProxyState = NULL;
                //}
            }

    } // end of switch(state)
quit:

    if (error != ERROR_IO_PENDING) {
        fsm.SetDone();

        if ( fsm.m_pProxyInfoQuery &&
             fsm.m_pProxyInfoQuery->IsAlloced() )
        {
            delete fsm.m_pProxyInfoQuery;
            fsm.m_pProxyInfoQuery = NULL;
        }

        fsm.m_pProxyInfoQuery = NULL;  // just in case

        if (fsm.m_pProxyState) {
            delete fsm.m_pProxyState;
        }

        //
        // only deref hInternet if we are in sync mode, otherwise it is done in
        // async request completion
        //
        // BUGBUG - RLF 05/17/97
        //
        // Need to fix this - it is confusing and error-prone. Right fix is to
        // do like old async code does - additional ref when starting async
        // request && additional deref when completing async processing
        //

        if (!fOnApiCall &&
            (fsm.m_hInternetMapped != NULL))
        {
            fsm.m_hInternetMapped = NULL;
        }
        if (error == ERROR_SUCCESS) {
            if (fsm.GetThreadInfo()->IsAsyncWorkerThread) {

                //
                // async request - return handle in FSM - lphInternet points
                // into app thread's stack - original request completed with
                // pending so we can't return it there
                //

                fsm.SetApiResult(fsm.m_hInternet);
            } else {

                //
                // sync request - app thread's stack is still there so we are
                // safe to return the handle through the pointer
                //

                *fsm.m_lphInternet = fsm.m_hInternet;
            }
        }
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
CrackUrl(
    IN OUT LPSTR lpszUrl,
    IN DWORD dwUrlLength,
    IN BOOL bEscape,
    OUT LPINTERNET_SCHEME lpSchemeType OPTIONAL,
    OUT LPSTR* lpszSchemeName OPTIONAL,
    OUT LPDWORD lpdwSchemeNameLength OPTIONAL,
    OUT LPSTR* lpszHostName OPTIONAL,
    OUT LPDWORD lpdwHostNameLength OPTIONAL,
    OUT LPINTERNET_PORT lpServerPort OPTIONAL,
    OUT LPSTR* lpszUserName OPTIONAL,
    OUT LPDWORD lpdwUserNameLength OPTIONAL,
    OUT LPSTR* lpszPassword OPTIONAL,
    OUT LPDWORD lpdwPasswordLength OPTIONAL,
    OUT LPSTR* lpszUrlPath OPTIONAL,
    OUT LPDWORD lpdwUrlPathLength OPTIONAL,
    OUT LPSTR* lpszExtraInfo OPTIONAL,
    OUT LPDWORD lpdwExtraInfoLength OPTIONAL,
    OUT LPBOOL pHavePort
    )

/*++

Routine Description:

    Cracks an URL into its constituent parts

    Assumes: 1. If one of the optional lpsz fields is present (e.g. lpszUserName)
                then the accompanying lpdw field must also be supplied

Arguments:

    lpszUrl                 - pointer to URL to crack. This buffer WILL BE
                              OVERWRITTEN if it contains escape sequences that
                              we will convert back to ANSI characters

    dwUrlLength             - if not 0, string length of lpszUrl

    bEscape                 - TRUE if we are to escape the url-path

    lpSchemeType            - returned scheme type - e.g. INTERNET_SCHEME_HTTP

    lpszSchemeName          - returned scheme name

    lpdwSchemeNameLength    - length of scheme name

    lpszHostName            - returned host name

    lpdwHostNameLength      - length of host name buffer

    lpServerPort            - returned server port if present in the URL, else 0

    lpszUserName            - returned user name if present

    lpdwUserNameLength      - length of user name buffer

    lpszPassword            - returned password if present

    lpdwPasswordLength      - length of password buffer

    lpszUrlPath             - returned, canonicalized URL path

    lpdwUrlPathLength       - length of url-path buffer

    lpszExtraInfo           - returned search string or intra-page link if present

    lpdwExtraInfoLength     - length of extra info buffer

    pHavePort               - returned boolean indicating whether port was specified

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_UNRECOGNIZED_SCHEME

--*/

{
    DWORD error;
    DWORD schemeLength;
    INTERNET_SCHEME schemeType;

    //
    // if dwUrlLength is 0 then lpszUrl is ASCIIZ. Find its length
    //

    if (dwUrlLength == 0) {
        dwUrlLength = strlen(lpszUrl);
    }

    //
    // get parser based on the protocol name
    //

    for (schemeLength = 0; lpszUrl[schemeLength] != ':'; ++schemeLength) {
        if ((dwUrlLength == 0) || (lpszUrl[schemeLength] == '\0')) {

            //
            // no ':' in URL? Bogus (dude)
            //

            error = ERROR_INTERNET_UNRECOGNIZED_SCHEME;
            goto quit;
        }
        --dwUrlLength;
    }

    DWORD i;
    int skip;
    BOOL isGeneric;
    BOOL needSlashes;
    BOOL haveSlashes;

    isGeneric = FALSE;
    needSlashes = FALSE;
    haveSlashes = FALSE;

    schemeType = INTERNET_SCHEME_UNKNOWN;

    if (ScanSchemes(lpszUrl, schemeLength, &i))
    {
        schemeType = UrlSchemeList[i].SchemeType;
        needSlashes = UrlSchemeList[i].NeedSlashes;
    }

    skip = 1;       // skip ':'

    if ((dwUrlLength > 3) && (memcmp(&lpszUrl[schemeLength], "://", 3) == 0)) {
        skip = 3;   // skip "://"
        haveSlashes = TRUE;
    }

    if (schemeType == INTERNET_SCHEME_FILE)
        isGeneric = TRUE;

    if (schemeType == INTERNET_SCHEME_NEWS ||
        schemeType == INTERNET_SCHEME_UNKNOWN) {

        //
        //  urls can be hierarchical or opaque.  if the slashes
        //  exist, then we should assume hierarchical
        //  when we dont know the scheme or it is news:.
        //  otherwise it is opaque (isGeneric)
        //

        needSlashes = haveSlashes;
        isGeneric = !haveSlashes;
    }

    //
    // If we don't have slashes, make sure we don't need them.
    // If we have slashes, make sure they are required.
    //

    if ((!haveSlashes && !needSlashes) || (haveSlashes && needSlashes)) {
        if (ARGUMENT_PRESENT(lpSchemeType)) {
            *lpSchemeType = schemeType;
        }
        if (ARGUMENT_PRESENT(lpszSchemeName)) {
            *lpszSchemeName = lpszUrl;
            *lpdwSchemeNameLength = schemeLength;
        }
        lpszUrl += schemeLength + skip;
        dwUrlLength -= skip;
        if (isGeneric) {
            if (ARGUMENT_PRESENT(lpszUserName)) {
                *lpszUserName = NULL;
                *lpdwUserNameLength = 0;
            }
            if (ARGUMENT_PRESENT(lpszPassword)) {
                *lpszPassword = NULL;
                *lpdwPasswordLength = 0;
            }
            if (ARGUMENT_PRESENT(lpszHostName)) {
                *lpszHostName = NULL;
                *lpdwHostNameLength = 0;
            }
            if (ARGUMENT_PRESENT(lpServerPort)) {
                *lpServerPort = 0;
            }
            error = ERROR_SUCCESS;
        } else {
            error = GetUrlAddress(&lpszUrl,
                                  &dwUrlLength,
                                  lpszUserName,
                                  lpdwUserNameLength,
                                  lpszPassword,
                                  lpdwPasswordLength,
                                  lpszHostName,
                                  lpdwHostNameLength,
                                  lpServerPort,
                                  pHavePort
                                  );
        }
        if (bEscape && (error == ERROR_SUCCESS)) {
            error = DecodeUrlInSitu(lpszUrl, &dwUrlLength);
        }
        if ((error == ERROR_SUCCESS) && ARGUMENT_PRESENT(lpszExtraInfo)) {
            *lpdwExtraInfoLength = 0;
            for (i = 0; i < (int)dwUrlLength; i++) {
                if (lpszUrl[i] == '?' || lpszUrl[i] == '#') {
                    *lpszExtraInfo = &lpszUrl[i];
                    *lpdwExtraInfoLength = dwUrlLength - i;
                    dwUrlLength -= *lpdwExtraInfoLength;
                }
            }
        }
        if ((error == ERROR_SUCCESS) && ARGUMENT_PRESENT(lpszUrlPath)) {
            *lpszUrlPath = lpszUrl;
            *lpdwUrlPathLength = dwUrlLength;
        }
    } else {
        error = ERROR_INTERNET_UNRECOGNIZED_SCHEME;
    }

quit:

    return error;
}


DWORD
EncodeUrlPath(
    IN DWORD Flags,
    IN DWORD SchemeFlags,
    IN LPSTR UrlPath,
    IN DWORD UrlPathLength,
    OUT LPSTR EncodedUrlPath,
    IN OUT LPDWORD EncodedUrlPathLength
    )

/*++

Routine Description:

    Encodes an URL-path. That is, escapes the string. Creates a new URL-path in
    which all the 'unsafe' and reserved characters for this scheme have been
    converted to escape sequences

Arguments:

    Flags                   - controlling expansion

    SchemeFlags             - which scheme we are encoding for -
                              SCHEME_HTTP, etc.

    UrlPath                 - pointer to the unescaped string

    UrlPathLength           - length of Url

    EncodedUrlPath          - pointer to buffer where encoded URL will be
                              written

    EncodedUrlPathLength    - IN: size of EncodedUrlPath
                              OUT: number of bytes written to EncodedUrlPath

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    UrlPathLength not large enough to store encoded URL path

--*/

{
    DWORD error;
    DWORD len;

    len = *EncodedUrlPathLength;
    while (len > 0) {

        UCHAR ch;

        ch = (UCHAR)*UrlPath++;
        if (ch == '\0') {

            //
            // end of input URL. Done
            //

            break;
        }

        //
        // check whether this character is safe. For now, we encode all unsafe
        // and scheme-specific characters the same way (i.e. irrespective of
        // scheme)
        //
        // We are allowing '/' to be copied unmodified
        //

        if (IS_UNSAFE_URL_CHARACTER(ch, SchemeFlags)
        && !((ch == '/') && (Flags & NO_ENCODE_PATH_SEP))) {
            if (len < 3) {

                //
                // set the length to zero so that we return
                // ERROR_INSUFFICIENT_BUFFER
                //

                len = 0;
                break;
            }
            *EncodedUrlPath++ = '%';
            //*EncodedUrlPath++ = NumberToHexChar((int)ch / 16);
            *EncodedUrlPath++ = NUMBER_TO_HEX_CHAR((int)ch / 16);
            //*EncodedUrlPath++ = NumberToHexChar((int)ch % 16);
            *EncodedUrlPath++ = NUMBER_TO_HEX_CHAR((int)ch % 16);
            len -= 2; // extra --len below
        } else {
            *EncodedUrlPath++ = (signed char)ch;
        }
        --len;
    }
    if (len >= 1) {
        *EncodedUrlPath = '\0';
        *EncodedUrlPathLength -= len;
        error = ERROR_SUCCESS;
    } else {
        error = ERROR_INSUFFICIENT_BUFFER;
    }
    return error;
}


PRIVATE
char
HexCharToNumber(
    IN char ch
    )

/*++

Routine Description:

    Converts an ANSI character in the range '0'..'9' 'A'..'F' 'a'..'f' to its
    corresponding hexadecimal value (0..f)

Arguments:

    ch  - character to convert

Return Value:

    char
        hexadecimal value of ch, as an 8-bit (signed) character value

--*/

{
    return (ch <= '9') ? (ch - '0')
                       : ((ch >= 'a') ? ((ch - 'a') + 10) : ((ch - 'A') + 10));
}


PRIVATE
char
NumberToHexChar(
    IN int Number
    )

/*++

Routine Description:

    Converts a number in the range 0..15 to its ASCII character hex representation
    ('0'..'F')

Arguments:

    Number  - to convert

Return Value:

    char
        character in above range

--*/

{
    return (Number <= 9) ? (char)('0' + Number) : (char)('A' + (Number - 10));
}


DWORD
DecodeUrl(
    IN LPSTR Url,
    IN DWORD UrlLength,
    OUT LPSTR DecodedString,
    IN OUT LPDWORD DecodedLength
    )

/*++

Routine Description:

    Converts an URL string with embedded escape sequences (%xx) to a counted
    string

    It is safe to pass the same pointer for the string to convert, and the
    buffer for the converted results: if the current character is not escaped,
    it just gets overwritten, else the input pointer is moved ahead 2 characters
    further than the output pointer, which is benign

Arguments:

    Url             - pointer to URL string to convert

    UrlLength       - number of characters in UrlString

    DecodedString   - pointer to buffer that receives converted string

    DecodedLength   - IN: number of characters in buffer
                      OUT: number of characters converted

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                    UrlString couldn't be converted

                  ERROR_INSUFFICIENT_BUFFER
                    ConvertedString isn't large enough to hold all the converted
                    UrlString

--*/

{
    DWORD bufferRemaining;

    bufferRemaining = *DecodedLength;
    while (UrlLength && bufferRemaining) {

        char ch;

        if (*Url == '%') {

            //
            // BUGBUG - would %00 ever appear in an URL?
            //

            ++Url;
            if (isxdigit(*Url)) {
                ch = HexCharToNumber(*Url++) << 4;
                if (isxdigit(*Url)) {
                    ch |= HexCharToNumber(*Url++);
                } else {
                    return ERROR_INTERNET_INVALID_URL;
                }
            } else {
                return ERROR_INTERNET_INVALID_URL;
            }
            UrlLength -= 3;
        } else {
            ch = *Url++;
            --UrlLength;
        }
        *DecodedString++ = ch;
        --bufferRemaining;
    }
    if (UrlLength == 0) {
        *DecodedLength -= bufferRemaining;
        return ERROR_SUCCESS;
    } else {
        return ERROR_INSUFFICIENT_BUFFER;
    }
}


DWORD
DecodeUrlInSitu(
    IN LPSTR BufferAddress,
    IN OUT LPDWORD BufferLength
    )

/*++

Routine Description:

    Decodes an URL string, if it contains escape sequences. The conversion is
    done in place, since we know that a string containing escapes is longer than
    the string with escape sequences (3 bytes) converted to characters (1 byte)

Arguments:

    BufferAddress   - pointer to the string to convert

    BufferLength    - IN: number of characters to convert
                      OUT: length of converted string

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                  ERROR_INSUFFICIENT_BUFFER

--*/

{
    DWORD stringLength;

    stringLength = *BufferLength;
    if (memchr(BufferAddress, '%', stringLength)) {
        return DecodeUrl(BufferAddress,
                         stringLength,
                         BufferAddress,
                         BufferLength
                         );
    } else {

        //
        // no escape character in the string, just return success
        //

        return ERROR_SUCCESS;
    }
}


DWORD
DecodeUrlStringInSitu(
    IN LPSTR BufferAddress,
    IN OUT LPDWORD BufferLength
    )

/*++

Routine Description:

    Performs DecodeUrlInSitu() on a string and zero terminates it

    Assumes: 1. Even if no decoding is performed, *BufferLength is large enough
                to fit an extra '\0' character

Arguments:

    BufferAddress   - pointer to the string to convert

    BufferLength    - IN: number of characters to convert
                      OUT: length of converted string, excluding '\0'

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                  ERROR_INSUFFICIENT_BUFFER

--*/

{
    DWORD error;

    error = DecodeUrlInSitu(BufferAddress, BufferLength);
    if (error == ERROR_SUCCESS) {
        BufferAddress[*BufferLength] = '\0';
    }
    return error;
}


DWORD
GetUrlAddressInfo(
    IN OUT LPSTR* Url,
    IN OUT LPDWORD UrlLength,
    OUT LPSTR* PartOne,
    OUT LPDWORD PartOneLength,
    OUT LPBOOL PartOneEscape,
    OUT LPSTR* PartTwo,
    OUT LPDWORD PartTwoLength,
    OUT LPBOOL PartTwoEscape
    )

/*++

Routine Description:

    Given a string of the form foo:bar, splits them into 2 counted strings about
    the ':' character. The address string may or may not contain a ':'.

    This function is intended to split into substrings the host:port and
    username:password strings commonly used in Internet address specifications
    and by association, in URLs

Arguments:

    Url             - pointer to pointer to string containing URL. On output
                      this is advanced past the address parts

    UrlLength       - pointer to length of URL in UrlString. On output this is
                      reduced by the number of characters parsed

    PartOne         - pointer which will receive first part of address string

    PartOneLength   - pointer which will receive length of first part of address
                      string

    PartOneEscape   - TRUE on output if PartOne contains escape sequences

    PartTwo         - pointer which will receive second part of address string

    PartTwoLength   - pointer which will receive length of second part of address
                      string

    PartOneEscape   - TRUE on output if PartTwo contains escape sequences

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL

--*/

{
    LPSTR pString;
    LPSTR pColon;
    DWORD partLength;
    LPBOOL partEscape;
    DWORD length;

    //
    // parse out <host>[:<port>] or <name>[:<password>] (i.e. <part1>[:<part2>]
    //

    pString = *Url;
    pColon = NULL;
    partLength = 0;
    *PartOne = pString;
    *PartOneLength = 0;
    *PartOneEscape = FALSE;
    *PartTwoEscape = FALSE;
    partEscape = PartOneEscape;
    length = *UrlLength;
    while ((*pString != '/') && (*pString != '\0') && (length != 0)) {
        if (*pString == '%') {

            //
            // if there is a % in the string then it *must* (RFC 1738) be the
            // start of an escape sequence. This function just reports the
            // address of the substrings and their lengths; calling functions
            // must handle the escape sequences (i.e. it is their responsibility
            // to decide where to put the results)
            //

            *partEscape = TRUE;
        }
        if (*pString == ':') {
            if (pColon != NULL) {

                //
                // we don't expect more than 1 ':'
                //

                return ERROR_INTERNET_INVALID_URL;
            }
            pColon = pString;
            *PartOneLength = partLength;
            if (partLength == 0) {
                *PartOne = NULL;
            }
            partLength = 0;
            partEscape = PartTwoEscape;
        } else {
            ++partLength;
        }
        ++pString;
        --length;
    }

    //
    // we either ended on the host (or user) name or the port number (or
    // password), one of which we don't know the length of
    //

    if (pColon == NULL) {
        *PartOneLength = partLength;
        *PartTwo = NULL;
        *PartTwoLength = 0;
        *PartTwoEscape = FALSE;
    } else {
        *PartTwoLength = partLength;
        *PartTwo = pColon + 1;

        //
        // in both the <user>:<password> and <host>:<port> cases, we cannot have
        // the second part without the first, although both parts being zero
        // length is OK (host name will be sorted out elsewhere, but (for now,
        // at least) I am allowing <>:<> for username:password, since I don't
        // see it expressly disallowed in the RFC. I may be revisiting this code
        // later...)
        //
        // N.B.: ftp://ftp.microsoft.com uses http://:0/-http-gw-internal-/menu.gif

//      if ((*PartOneLength == 0) && (partLength != 0)) {
//          return ERROR_INTERNET_INVALID_URL;
//      }
    }

    //
    // update the URL pointer and length remaining
    //

    *Url = pString;
    *UrlLength = length;

    return ERROR_SUCCESS;
}


DWORD
GetUrlAddress(
    IN OUT LPSTR* lpszUrl,
    OUT LPDWORD lpdwUrlLength,
    OUT LPSTR* lpszUserName OPTIONAL,
    OUT LPDWORD lpdwUserNameLength OPTIONAL,
    OUT LPSTR* lpszPassword OPTIONAL,
    OUT LPDWORD lpdwPasswordLength OPTIONAL,
    OUT LPSTR* lpszHostName OPTIONAL,
    OUT LPDWORD lpdwHostNameLength OPTIONAL,
    OUT LPINTERNET_PORT lpPort OPTIONAL,
    OUT LPBOOL pHavePort
    )

/*++

Routine Description:

    This function extracts any and all parts of the address information for a
    generic URL. If any of the address parts contain escaped characters (%nn)
    then they are converted in situ

    The generic addressing format (RFC 1738) is:

        <user>:<password>@<host>:<port>

    The addressing information cannot contain a password without a user name,
    or a port without a host name
    NB: ftp://ftp.microsoft.com uses URL's that have a port without a host name!
    (e.g. http://:0/-http-gw-internal-/menu.gif)

    Although only the lpszUrl and lpdwUrlLength fields are required, the address
    parts will be checked for presence and completeness

    Assumes: 1. If one of the optional lpsz fields is present (e.g. lpszUserName)
                then the accompanying lpdw field must also be supplied

Arguments:

    lpszUrl             - IN: pointer to the URL to parse
                          OUT: URL remaining after address information

                          N.B. The url-path is NOT canonicalized (unescaped)
                          because it may contain protocol-specific information
                          which must be parsed out by the protocol-specific
                          parser

    lpdwUrlLength       - returned length of the remainder of the URL after the
                          address information

    lpszUserName        - returned pointer to the user name
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user names in the URL

    lpdwUserNameLength  - returned length of the user name part
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user names in the URL

    lpszPassword        - returned pointer to the password
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user passwords in the URL

    lpdwPasswordLength  - returned length of the password
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user passwords in the URL

    lpszHostName        - returned pointer to the host name
                          This parameter can be omitted by those protocol parsers
                          that do not require the host name info

    lpdwHostNameLength  - returned length of the host name
                          This parameter can be omitted by those protocol parsers
                          that do not require the host name info

    lpPort              - returned value of the port field
                          This parameter can be omitted by those protocol parsers
                          that do not require or expect user port number

    pHavePort           - returned boolean indicating whether a port was specified
                          in the URL or not.  This value is not returned if the
                          lpPort parameter is omitted.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                    We could not parse some part of the address info, or we
                    found address info where the protocol parser didn't expect
                    any

                  ERROR_INSUFFICIENT_BUFFER
                    We could not convert an escaped string

--*/

{
    LPSTR pAt;
    DWORD urlLength;
    LPSTR pUrl;
    BOOL part1Escape;
    BOOL part2Escape;
    char portNumber[INTERNET_MAX_PORT_NUMBER_LENGTH + 1];
    DWORD portNumberLength;
    LPSTR pPortNumber;
    DWORD error;
    LPSTR hostName;
    DWORD hostNameLength;

    pUrl = *lpszUrl;
    urlLength = strlen(pUrl);

    //
    // check to see if there is an '@' separating user name & password. If we
    // see a '/' or get to the end of the string before we see the '@' then
    // there is no username:password part
    //

    pAt = NULL;
    for (DWORD i = 0; i < urlLength; ++i) {
        if (pUrl[i] == '/') {
            break;
        } else if (pUrl[i] == '@') {
            pAt = &pUrl[i];
            break;
        }
    }

    if (pAt != NULL) {

        DWORD addressPartLength;
        LPSTR userName;
        DWORD userNameLength;
        LPSTR password;
        DWORD passwordLength;

        addressPartLength = (DWORD) (pAt - pUrl);
        urlLength -= addressPartLength;
        error = GetUrlAddressInfo(&pUrl,
                                  &addressPartLength,
                                  &userName,
                                  &userNameLength,
                                  &part1Escape,
                                  &password,
                                  &passwordLength,
                                  &part2Escape
                                  );
        if (error != ERROR_SUCCESS) {
            return error;
        }

        //
        // ensure there is no address information unparsed before the '@'
        //

        INET_ASSERT(addressPartLength == 0);
        INET_ASSERT(pUrl == pAt);

        if (ARGUMENT_PRESENT(lpszUserName)) {

            INET_ASSERT(ARGUMENT_PRESENT(lpdwUserNameLength));

            //
            // convert the user name in situ
            //

            if (part1Escape) {

                INET_ASSERT(userName != NULL);
                INET_ASSERT(userNameLength != 0);

                error = DecodeUrlInSitu(userName, &userNameLength);
                if (error != ERROR_SUCCESS) {
                    return error;
                }
            }
            *lpszUserName = userName;
            *lpdwUserNameLength = userNameLength;
        }

        if (ARGUMENT_PRESENT(lpszPassword)) {

            //
            // convert the password in situ
            //

            if (part2Escape) {

                INET_ASSERT(userName != NULL);
                INET_ASSERT(userNameLength != 0);
                INET_ASSERT(password != NULL);
                INET_ASSERT(passwordLength != 0);

                error = DecodeUrlInSitu(password, &passwordLength);
                if (error != ERROR_SUCCESS) {
                    return error;
                }
            }
            *lpszPassword = password;
            *lpdwPasswordLength = passwordLength;
        }

        //
        // the URL pointer now points at the host:port fields (remember that
        // ExtractAddressParts() must have bumped pUrl up to the end of the
        // password field (if present) which ends at pAt)
        //

        ++pUrl;

        //
        // similarly, bump urlLength to account for the '@'
        //

        --urlLength;
    } else {

        //
        // no '@' therefore no username or password
        //

        if (ARGUMENT_PRESENT(lpszUserName)) {

            INET_ASSERT(ARGUMENT_PRESENT(lpdwUserNameLength));

            *lpszUserName = NULL;
            *lpdwUserNameLength = 0;
        }
        if (ARGUMENT_PRESENT(lpszPassword)) {

            INET_ASSERT(ARGUMENT_PRESENT(lpdwPasswordLength));

            *lpszPassword = NULL;
            *lpdwPasswordLength = 0;
        }
    }

    //
    // now get the host name and the optional port
    //

    pPortNumber = portNumber;
    portNumberLength = sizeof(portNumber);
    error = GetUrlAddressInfo(&pUrl,
                              &urlLength,
                              &hostName,
                              &hostNameLength,
                              &part1Escape,
                              &pPortNumber,
                              &portNumberLength,
                              &part2Escape
                              );
    if (error != ERROR_SUCCESS) {
        return error;
    }

    //
    // the URL address information MUST contain the host name
    //

//  if ((hostName == NULL) || (hostNameLength == 0)) {
//      return ERROR_INTERNET_INVALID_URL;
//  }

    if (ARGUMENT_PRESENT(lpszHostName)) {

        INET_ASSERT(ARGUMENT_PRESENT(lpdwHostNameLength));

        //
        // if the host name contains escaped characters, convert them in situ
        //

        if (part1Escape) {
            error = DecodeUrlInSitu(hostName, &hostNameLength);
            if (error != ERROR_SUCCESS) {
                return error;
            }
        }
        *lpszHostName = hostName;
        *lpdwHostNameLength = hostNameLength;
    }

    //
    // if there is a port field, convert it if there are escaped characters,
    // check it for valid numeric characters, and convert it to a number
    //

    if (ARGUMENT_PRESENT(lpPort)) {
        if (portNumberLength != 0) {

            DWORD i;
            DWORD port;

            INET_ASSERT(pPortNumber != NULL);

            if (part2Escape) {
                error = DecodeUrlInSitu(pPortNumber, &portNumberLength);
                if (error != ERROR_SUCCESS) {
                    return error;
                }
            }

            //
            // ensure all characters in the port number buffer are numeric, and
            // calculate the port number at the same time
            //

            for (i = 0, port = 0; i < portNumberLength; ++i) {
                if (!isdigit(*pPortNumber)) {
                    return ERROR_INTERNET_INVALID_URL;
                }
                port = port * 10 + (int)(*pPortNumber++ - '0');
                // We won't allow ports larger than 65535 ((2^16)-1)
                // We have to check this every time to make sure that someone
                // doesn't try to overflow a DWORD.
                if (port > 65535)
                {
                    return ERROR_INTERNET_INVALID_URL;
                }
            }
            *lpPort = (INTERNET_PORT)port;
            if (ARGUMENT_PRESENT(pHavePort)) {
                *pHavePort = TRUE;
            }
        } else {
            *lpPort = INTERNET_INVALID_PORT_NUMBER;
            if (ARGUMENT_PRESENT(pHavePort)) {
                *pHavePort = FALSE;
            }
        }
    }

    //
    // update the URL pointer and the length of the url-path
    //

    *lpszUrl = pUrl;
    *lpdwUrlLength = urlLength;

    return ERROR_SUCCESS;
}


INTERNET_SCHEME
MapUrlSchemeName(
    IN LPSTR lpszSchemeName,
    IN DWORD dwSchemeNameLength
    )

/*++

Routine Description:

    Maps a scheme name/length to a scheme name type

Arguments:

    lpszSchemeName      - pointer to name of scheme to map

    dwSchemeNameLength  - length of scheme (if -1, lpszSchemeName is ASCIZ)

Return Value:

    INTERNET_SCHEME

--*/

{
    if (dwSchemeNameLength == (DWORD)-1) {
        dwSchemeNameLength = (DWORD)lstrlen(lpszSchemeName);
    }

    DWORD i;
    if (ScanSchemes(lpszSchemeName, dwSchemeNameLength, &i))
    {
        return UrlSchemeList[i].SchemeType;
    }
    return INTERNET_SCHEME_UNKNOWN;
}


LPSTR
MapUrlScheme(
    IN INTERNET_SCHEME Scheme,
    OUT LPDWORD lpdwSchemeNameLength
    )

/*++

Routine Description:

    Maps the enumerated scheme name type to the name

Arguments:

    Scheme                  - enumerated scheme type to map

    lpdwSchemeNameLength    - pointer to returned length of scheme name

Return Value:

    LPSTR   - pointer to scheme name or NULL

--*/

{
    if ((Scheme >= INTERNET_SCHEME_FIRST)
    && (Scheme <= INTERNET_SCHEME_LAST)) {
        *lpdwSchemeNameLength = UrlSchemeList[Scheme].SchemeLength;
        return UrlSchemeList[Scheme].SchemeName;
    }
    return NULL;
}


LPSTR
MapUrlSchemeToName(
    IN INTERNET_SCHEME Scheme
    )

/*++

Routine Description:

    Maps the enumerated scheme name type to the name

Arguments:

    Scheme  - enumerated scheme type to map

Return Value:

    LPSTR   - pointer to scheme name or NULL

--*/

{
    if ((Scheme >= INTERNET_SCHEME_FIRST)
    && (Scheme <= INTERNET_SCHEME_LAST)) {
        return UrlSchemeList[Scheme].SchemeName;
    }
    return NULL;
}
