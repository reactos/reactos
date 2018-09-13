/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gfrapiu.cxx

Abstract:

    Common sub-API level functions

    Contents:
        TestLocatorType
        GetAttributes
        MakeAttributeRequest
        ParseGopherUrl
        GopherLocatorToUrl

Author:

    Richard L Firth (rfirth) 19-Nov-1994

Environment:

    Win32 user-level DLL

Revision History:

    19-Nov-1994
        Created

--*/

#include <wininetp.h>
#include "gfrapih.h"

//  because wininet doesnt know IStream
#define NO_SHLWAPI_STREAM
#include <shlwapi.h>
#include <shlwapip.h>

#define INTERNET_DEFAULT_CSO_PORT     105
#define INTERNET_MAX_WELL_KNOWN_PORT 1023
//
// functions
//

BOOL IsInappropriateGopherPort (INTERNET_PORT port)
/*++

Routine Description:

    Gopher URLs can encode arbitrary data to arbitrary ports.  This characteristic
    enables malicious web pages to redirect IE to exploit security holes, for
    example, to spoof a mailer daemon inside a firewall.  Based on experimentation,
    Netscape apparently disables gopher on ports 1 and 7 though 25 odd.  That range
    covers many of the well-known ports catalogued by IANA but misses many others
    like 137 through 139, assigned for netbios over tcp/ip .  Since gopher is
    becoming increasingly irrelevant, we prefer to be stricter .  IE3 now disables
    gopher on ports less than 1024, except for 70, the standard gopher port, and
    105, typically used for CSO name searches.

Arguments: Port number

Return Value: TRUE for success, FALSE for failure

--*/
{
    if (port > INTERNET_MAX_WELL_KNOWN_PORT)
        return FALSE;
    switch (port) {
        case INTERNET_INVALID_PORT_NUMBER:
        case INTERNET_DEFAULT_GOPHER_PORT:
        case INTERNET_DEFAULT_CSO_PORT:
            return FALSE;
        default:
            return TRUE;
    }
}




DWORD
TestLocatorType(
    IN LPCSTR Locator,
    IN DWORD TypeMask
    )

/*++

Routine Description:

    Checks that Locator is valid and checks if it is of the specified type.
    This function is mainly for use by GfrIsXxxx APIs

Arguments:

    Locator     - pointer to app-supplied locator string

    TypeMask    - gopher type mask to check for

Return Value:

    DWORD
        Success - ERROR_SUCCESS
                    Locator is good and of the specified type

        Failure - ERROR_INVALID_PARAMETER
                    Locator is bad

                  ERROR_INVALID_FUNCTION
                    Locator is good, but not of the specified type

--*/

{
    DWORD error;
    BOOL success = FALSE;

    //
    // BUGBUG - 1. Do we really want to test this parameter?
    //          2. If so, is the length sufficient?
    //

    if (IsBadStringPtr(Locator, MAX_GOPHER_LOCATOR_LENGTH)) {
        error = ERROR_INVALID_PARAMETER;
    } else {

        DWORD gopherType;

        gopherType = GopherCharToType(*Locator);
        if (gopherType == INVALID_GOPHER_TYPE) {

            //
            // not a recognizable type - Locator is bogus
            //

            error = ERROR_INVALID_PARAMETER;
        } else if (gopherType & TypeMask) {
            error = ERROR_SUCCESS;
        } else {

            //
            // slight bogosity - need an error code to differentiate matched
            // vs. not-matched: INVALID_FUNCTION will do
            //

            error = ERROR_INVALID_FUNCTION;
        }
    }
    return error;
}

#if defined(GOPHER_ATTRIBUTE_SUPPORT)


DWORD
GetAttributes(
    IN GOPHER_ATTRIBUTE_ENUMERATOR Enumerator,
    IN DWORD CategoryId,
    IN DWORD AttributeId,
    IN LPCSTR AttributeName,
    IN LPSTR InBuffer,
    IN DWORD InBufferLength,
    OUT LPBYTE OutBuffer,
    IN DWORD OutBufferLength,
    OUT LPDWORD CharactersReturned
    )

/*++

Routine Description:

    Pulls attributes out of a buffer and puts them in the caller's buffer or
    enumerates them (if Enumerator supplied)

Arguments:

    Enumerator          - address of caller's enumerator function

    CategoryId          - category of attribute(s)

    AttributeId         - the attribute to return

    AttributeName       - name of the attribute if not a known attribute

    InBuffer            - pointer to buffer containing gopher+ attributes

    InBufferLength      - length of attribute buffer

    OutBuffer           - pointer to caller's buffer where attributes returned

    OutBufferLength     - length of caller's buffer

    CharactersReturned  - pointer to returned buffer length

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_GOPHER_ATTRIBUTE_NOT_FOUND
                    We couldn't find the requested attribute/category

                  ERROR_INSUFFICIENT_BUFFER
                    The caller's buffer isn't large enough to contain the
                    attributes. *lpdwCharactersReturned will contain the
                    required length

                  ERROR_GOPHER_DATA_ERROR
                    We couldn't parse the attributes for some reason

--*/

{
    DWORD error;

    *CharactersReturned = 0;

    //
    // the buffer starts with the "+INFO:" attribute describing the locator. We
    // don't return this as an attribute
    //

    if (SkipLine(&InBuffer, &InBufferLength)) {

        LPSTR endSection;
        BOOL done;
        BOOL found;
        BOOL more;

        if (CategoryId != GOPHER_CATEGORY_ID_ALL) {

            //
            // advance InBuffer to the line that contains the requested
            // attribute
            //

            found = FindAttribute(CategoryId,
                                  AttributeId,
                                  AttributeName,
                                  &InBuffer,
                                  &InBufferLength
                                  );
            if (found) {

                //
                // if the caller requested that we return all attributes in a
                // section, then skip the line containing the category name
                //

                if (AttributeId == GOPHER_ATTRIBUTE_ID_ALL) {
                    found = SkipLine(&InBuffer, &InBufferLength);
                }
                if (found) {

                    DWORD bufferLeft;

                    //
                    // get the end of the section or line in endSection
                    //

                    endSection = InBuffer;
                    bufferLeft = InBufferLength;
                    FindNextAttribute(CategoryId,
                                      AttributeId,
                                      &endSection,
                                      &bufferLeft
                                      );
                }
            }
            error = found ? ERROR_SUCCESS : ERROR_GOPHER_ATTRIBUTE_NOT_FOUND;
        } else {
            endSection = InBuffer + InBufferLength;
        }

        more = TRUE;
        done = FALSE;

        while ((error == ERROR_SUCCESS) && (InBuffer < endSection) && more) {

            LPSTR linePtr;
            char lineBuffer[256];  // arbitrary
            DWORD lineLength;
            BOOL ok;

            linePtr = lineBuffer;
            lineLength = sizeof(lineBuffer);
            ok = CopyToEol(&linePtr,
                           &lineLength,
                           &InBuffer,
                           &InBufferLength
                           );
            if (ok) {
                if (Enumerator != NULL) {

                    //
                    // if the line starts with a '+' then (we assume) we are
                    // enumerating all attributes, in which case this line
                    // just serves to identify the next attribute section. We
                    // don't return any info
                    //

                    if (*linePtr == '+') {

                        char newCategory[32];   // arbitrary
                        int i;

                        for (i = 0; i < sizeof(newCategory); ++i) {

                            char ch;

                            ch = linePtr[i];
                            if ((ch == '\r') || (ch == '\n') || (ch == ' ') || (ch == ':')) {
                                break;
                            }
                            newCategory[i] = ch;
                        }
                        newCategory[i] = '\0';
                        MapAttributeToIds((LPCSTR)newCategory,
                                          &CategoryId,
                                          &AttributeId
                                          );
                        if (CategoryId == GOPHER_CATEGORY_ID_ABSTRACT) {

                            //
                            // BUGBUG - the remainder of this line may contain
                            //          a locator identifying the location of
                            //          a file containing the abstract
                            //

                        }
                    } else {
                        error = EnumerateAttribute(Enumerator,
                                                   linePtr,
                                                   lineLength,
                                                   OutBuffer,
                                                   OutBufferLength,
                                                   &more
                                                   );
                        done = TRUE;
                    }
                } else {

                    //
                    // get the length of the line in lineLength. N.B. We have
                    // to subtract an extra 1 because CopyToEol adds a '\0'
                    //

                    lineLength = sizeof(lineBuffer) - lineLength - 1;
                    if (OutBufferLength >= lineLength) {
                        memcpy(OutBuffer, lineBuffer, lineLength);
                        OutBuffer += lineLength;
                        OutBufferLength -= lineLength;
                        done = TRUE;
                    } else {
                        error = ERROR_INSUFFICIENT_BUFFER;
                    }

                    //
                    // always update the characters copied/required parameter
                    //

                    *CharactersReturned += lineLength;
                }
            } else {
                error = ERROR_GOPHER_DATA_ERROR;
            }
        }

        //
        // if nothing was copied or enumerated then the attribute was not found
        //

        if (!done && (error == ERROR_SUCCESS)) {
            error = ERROR_GOPHER_ATTRIBUTE_NOT_FOUND;
        }
    } else {
        error = ERROR_GOPHER_DATA_ERROR;
    }
    return error;
}


LPSTR
MakeAttributeRequest(
    IN LPSTR Selector,
    IN LPSTR Attribute
    )

/*++

Routine Description:

    Converts a gopher+ request into a request for attributes. E.g. turns
    "0Foo" into "0Foo\t!+ADMIN"

Arguments:

    Selector    - pointer to identifier of gopher+ item to get attributes for

    Attribute   - pointer to name of attribute(s) to retrieve

Return Value:

    LPSTR
        Success - pointer to allocated memory containing attribute requester

        Failure - NULL

--*/

{
    INT selectorLength;
    INT attributeLength;
    LPSTR request;

    selectorLength = (Selector != NULL) ? strlen(Selector) : 0;
    attributeLength = (Attribute != NULL) ? strlen(Attribute) : 0;
    request = NEW_MEMORY(selectorLength

                         //
                         // sizeof(GOPHER_PLUS_INFO_REQUEST) includes 2 for
                         // <CR><LF> and 1 for terminator
                         //

                         + sizeof(GOPHER_PLUS_INFO_REQUEST)
                         + attributeLength,
                         CHAR
                         );
    if (request != NULL) {
        if (Selector != NULL) {
            memcpy(request, Selector, selectorLength);
        }
        memcpy(&request[selectorLength],
               GOPHER_PLUS_ITEM_INFO,
               sizeof(GOPHER_PLUS_ITEM_INFO) - 1
               );
        selectorLength += sizeof(GOPHER_PLUS_ITEM_INFO) - 1;
        if (Attribute != NULL) {
            memcpy(&request[selectorLength], Attribute, attributeLength);
            selectorLength += attributeLength;
        }
        memcpy(&request[selectorLength],
               GOPHER_REQUEST_TERMINATOR,
               sizeof(GOPHER_REQUEST_TERMINATOR)
               );
    }
    return request;
}

#endif // defined(GOPHER_ATTRIBUTE_SUPPORT)


DWORD
ParseGopherUrl(
    IN OUT LPHINTERNET hInternet,
    IN LPSTR Url,
    IN DWORD SchemeLength,
    IN LPSTR Headers,
    IN DWORD HeadersLength,
    IN DWORD OpenFlags,
    IN DWORD_PTR Context
    )

/*++

Routine Description:

    URL parser for gopher URLs. Support function for InternetOpenUrl() and
    ParseUrl().

    This is a macro function that just cracks the URL and calls gopher APIs to
    do the work

Arguments:

    hInternet       - IN: Internet gateway handle
                      OUT: if successful handle of opened item, else undefined

    Url             - pointer to string containing gopher URL to open

    SchemeLength    - length of the URL scheme, exluding "://"

    Headers         - unused for gopher

    HeadersLength   - unused for gopher

    OpenFlags       - optional flags for opening a file (cache/no-cache, etc.)

    Context         - app-supplied context value for call-backs

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INVALID_URL
                    The URL passed in could not be parsed

--*/

{
    DEBUG_ENTER((DBG_GOPHER,
                 Dword,
                 "ParseGopherUrl",
                 "%#x [%#x], %q, %d, %#x, %d, %#x, %#x",
                 hInternet,
                 *hInternet,
                 Url,
                 SchemeLength,
                 Headers,
                 HeadersLength,
                 OpenFlags,
                 Context
                 ));

    DWORD error;
    DWORD gopherType;
    LPSTR selector;
    LPSTR searchString;
    HINTERNET hMapped = NULL;

    UNREFERENCED_PARAMETER(Headers);
    UNREFERENCED_PARAMETER(HeadersLength);

    //
    // extract the address information - no user name or password
    //

    DWORD urlLength;
    LPSTR pHostName;
    DWORD hostNameLength;
    INTERNET_PORT port;
    LPSTR lpszUrl = Url;

    Url += SchemeLength + sizeof("://") - 1;
    error = GetUrlAddress(&Url,
                          &urlLength,
                          NULL,
                          NULL,
                          NULL,
                          NULL,
                          &pHostName,
                          &hostNameLength,
                          &port,
                          NULL
                          );
    if (error != ERROR_SUCCESS) {
        goto quit;
    }

    if (IsInappropriateGopherPort(port)) {
        error = ERROR_INTERNET_INVALID_URL;
        goto quit;
    }

    //
    // a '/' between address and url-path is not significant to gopher
    //

    if (*Url == '/') {
        ++Url;
        --urlLength;

        //
        // the fact that we can ignore the '/' between address and url-path
        // means that it is okay to write a '\0' at the end of the host name
        //

        pHostName[hostNameLength] = '\0';
    }

    //
    // if the URL just consisted of gopher://host[:port] then by default we are
    // referencing the root gopher directory
    //

    if (*Url != '\0') {

        //
        // Before decoding, convert '?' to tab and thereafter any '+' to ' '
        //

        LPSTR lpszScan = strchr (Url, '?');
        if (lpszScan)
        {
            *lpszScan++ = '\t';
            while (*lpszScan)
            {
                INET_ASSERT (*lpszScan != '?'); // should be encoded
                if (*lpszScan == '+')
                    *lpszScan = ' ';
                lpszScan++;
            }
        }

        //
        // we need to convert the url-path before checking it for the search
        // string and gopher+ fields because we need to search for '\t' which
        // is currently encoded
        //

        if(FAILED(UrlUnescapeInPlace(Url, 0))){
            goto quit;
        }
        urlLength = lstrlen(Url);

        //
        // find the type of the gopher resource; if unknown, treat as a file
        //

        gopherType = GopherCharToType(Url[0]);
        selector = &Url[1];

        //
        // urlLength is now the length of the converted selector
        //

        --urlLength;
        searchString = (LPSTR)memchr((LPVOID)selector, '\t', urlLength);
        if (searchString != NULL) {

            LPSTR plusString;

            //
            // zero-terminate the search string, then check if for a gopher+
            // component
            //

            *searchString++ = '\0';
            plusString = (LPSTR)memchr((LPVOID)searchString,
                                       '\t',
                                       urlLength - (DWORD) (searchString - selector)
                                       );
            if (plusString != NULL) {
                *plusString++ = '\0';
                gopherType |= GOPHER_TYPE_GOPHER_PLUS;

                //
                // if the URL defines a file then we may have a view type
                //

                //
                // BUGBUG - need to handle:
                //
                //      - alternate file views
                //      - attribute requests (?)
                //      - ASK forms
                //
            }
        }
    } else {
        gopherType = GOPHER_TYPE_DIRECTORY;
        selector = NULL;
        searchString = NULL;
    }

    HINTERNET hConnect;

    //
    // initialize in case of error
    //

    hConnect = NULL;

    //
    // get the offline state
    //

    BOOL bOffline;
    DWORD dwFlags;

    bOffline = IsOffline();
    if (bOffline || (OpenFlags & INTERNET_FLAG_OFFLINE)) {
        dwFlags = INTERNET_FLAG_OFFLINE;
    } else {
        dwFlags = 0;
    }

    //
    // try to create a locator from the various parts
    //

    char locator[MAX_GOPHER_LOCATOR_LENGTH + 1];
    DWORD locatorLength;

    locatorLength = sizeof(locator);
    if (GopherCreateLocator(pHostName,
                            port,
                            NULL,
                            selector,
                            gopherType,
                            locator,
                            &locatorLength
                            )) {

        //
        // ok, all parts present and correct; open a handle to the gopher
        // resource
        //

        hConnect = InternetConnect(*hInternet,
                                   pHostName,
                                   port,
                                   NULL,    // lpszUserName
                                   NULL,    // lpszPassword
                                   INTERNET_SERVICE_GOPHER,
                                   dwFlags,

                                   //
                                   // we are creating a "hidden" handle - don't
                                   // tell the app about it
                                   //

                                   INTERNET_NO_CALLBACK
                                   );

try_again:

        if (hConnect != NULL) {

            HINTERNET handle;

            if ( hMapped == NULL )
            {
                error = MapHandleToAddress(hConnect, (LPVOID *)&hMapped, FALSE);

                if ( (error != ERROR_SUCCESS) && (hMapped == NULL) )
                {
                    goto error_quit;
                }

                error = ERROR_SUCCESS;
            }

            INET_ASSERT(hMapped != NULL);

            ((INTERNET_CONNECT_HANDLE_OBJECT *)hMapped)->SetURL(lpszUrl);

            if (  IS_GOPHER_DIRECTORY(gopherType)
               || IS_GOPHER_SEARCH_SERVER(gopherType)) {

                // set htmlfind only if RAW is not asked

                if (!(OpenFlags & INTERNET_FLAG_RAW_DATA)) {

                    ((INTERNET_CONNECT_HANDLE_OBJECT *)hMapped)->SetHtmlFind(TRUE);

                    //
                    // BUGBUG: we don't have time to handle CSO searches
                    //
                    if (IS_GOPHER_PHONE_SERVER (gopherType))
                        goto cso_hack;

                    if ( IS_GOPHER_SEARCH_SERVER(gopherType)
                          && (!searchString || !searchString[0])) {

cso_hack:
                        handle = NULL;

                        if (ERROR_SUCCESS == RMakeGfrFixedObjectHandle
                            (hMapped, &handle, gopherType)) {
                            handle = ((HANDLE_OBJECT *)handle)->GetPseudoHandle();
                        }

                        DereferenceObject((LPVOID)hMapped);
                        goto got_handle;
                    }

                }

                handle = GopherFindFirstFile(hConnect,
                                             locator,
                                             searchString,
                                             NULL,
                                             OpenFlags | dwFlags,
                                             Context
                                             );


            } else {

                handle = GopherOpenFile(hConnect,
                                        locator,
                                        NULL,
                                        OpenFlags | dwFlags,
                                        Context
                                        );
            }

got_handle:

            if (handle != NULL) {

                //
                // map the handles
                //

                HINTERNET hRequestMapped;
                error = MapHandleToAddress(handle, (LPVOID *)&hRequestMapped, FALSE);
                INET_ASSERT(error == ERROR_SUCCESS);

                HINTERNET hConnectMapped;
                error = MapHandleToAddress(hConnect, (LPVOID *)&hConnectMapped, FALSE);
                INET_ASSERT(error == ERROR_SUCCESS);

                //
                // link the request and connect handles so that the connect handle
                // object will be deleted when the request handle is closed
                //

                RSetParentHandle(hRequestMapped, hConnectMapped, TRUE);

                //
                // reduce the reference counts incremented by MapHandleToAddress()
                //

                if (hRequestMapped != NULL) {
                    DereferenceObject((LPVOID)hRequestMapped);
                }
                if (hConnectMapped != NULL) {
                    DereferenceObject((LPVOID)hConnectMapped);
                }

                //
                // return the request handle to the caller
                //

                *hInternet = handle;

                error = ERROR_SUCCESS;
                goto quit;
            } else if (!bOffline && IsOffline() && !(dwFlags & INTERNET_FLAG_OFFLINE)) {

                //
                // we went offline during the request. Try again, this time
                // from cache
                //

                dwFlags = INTERNET_FLAG_OFFLINE;
                goto try_again;
            }
        }
    }

error_quit:

    if ( hMapped != NULL )
    {
        DereferenceObject((LPVOID)hMapped);
        hMapped = NULL;
    }


    error = GetLastError();
    if (hConnect != NULL) {

        //
        // BUGBUG - this should close the item handle also (if open)
        //

        _InternetCloseHandle(hConnect);
    }

    INET_ASSERT(error != ERROR_SUCCESS);

quit:


    DEBUG_LEAVE(error);
    return error;
}


DWORD
GopherLocatorToUrl(
    IN LPSTR Locator,
    OUT LPSTR Buffer,
    IN DWORD BufferLength,
    OUT LPDWORD UrlLength
    )

/*++

Routine Description:

    Converts a gopher locator to a gopher URL. E.g. converts:

        1foo\tFoo Directory\tfoo.host\t77\t+

    to the URL:

        gopher://foo.host:77/1Foo%20Directory%09%09%2B

Arguments:

    Locator         - pointer to gopher locator to convert

    Buffer          - pointer to buffer where URL is written

    BufferLength    - size of Buffer in bytes

    UrlLength       - number of bytes written to Buffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INTERNET_INTERNAL_ERROR
                    We blew an internal buffer limit

                  ERROR_INSUFFICIENT_BUFFER
                    Buffer is not large enough to hold the converted URL

--*/

{
    DWORD gopherType;
    char selector[MAX_GOPHER_SELECTOR_TEXT + 1];
    DWORD selectorLength;
    char hostName[MAX_GOPHER_HOST_NAME + 1];
    DWORD hostNameLength;
    DWORD gopherPort;
    LPSTR gopherPlus;
    char urlBuf[INTERNET_MAX_URL_LENGTH];
    DWORD urlBufferLength;
    LPSTR urlBuffer;
    DWORD error;
    DWORD bufLen;

    urlBufferLength = sizeof(urlBuf);
    urlBuffer = urlBuf;
    bufLen = BufferLength;

    //
    // start with the gopher protocol specifier
    //

    if (bufLen > sizeof("gopher://")) {
        memcpy(Buffer, "gopher://", sizeof("gopher://") - 1);
        Buffer += sizeof("gopher://") - 1;
        bufLen -= sizeof("gopher://") - 1;
    } else {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // use CrackLocator() to get the individual parts of the locator
    //

    selectorLength = sizeof(selector);
    hostNameLength = sizeof(hostName);
    if (!CrackLocator(Locator,
                      &gopherType,
                      NULL,  // DisplayString - we don't care about this in the URL
                      NULL,  // DisplayStringLength
                      selector,
                      &selectorLength,
                      hostName,
                      &hostNameLength,
                      &gopherPort,
                      &gopherPlus
                      )) {

        //
        // most likely we bust a limit!
        //

        return ERROR_INTERNET_INTERNAL_ERROR;
    }

    //
    // add in the host name
    //

    if (bufLen > hostNameLength) {
        memcpy(Buffer, hostName, hostNameLength);
        Buffer += hostNameLength;
        bufLen -= hostNameLength;
    } else {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // add the port, but only if it is not the default (70)
    //

    if (gopherPort != INTERNET_DEFAULT_GOPHER_PORT) {
        if (bufLen > 1 + INTERNET_MAX_PORT_NUMBER_LENGTH) {

            int n;

            n = wsprintf(Buffer, ":%u", gopherPort);
            Buffer += n;
            bufLen -= (DWORD)n;
        } else {
            return ERROR_INSUFFICIENT_BUFFER;
        }
    }

    //
    // add the URL-path separator and the locator type character
    //

    if (bufLen > 2) {
        *Buffer++ = '/';
        *Buffer++ = *Locator;
        bufLen -= 2;
    }

    //
    // copy the selector string, and any gopher+ addenda to a separater buffer
    //

    if (urlBufferLength > selectorLength) {
        memcpy(urlBuffer, selector, selectorLength);
        urlBuffer += selectorLength;
        urlBufferLength -=  selectorLength;
    }

    //
    // if the locator specifies a gopher+ item then add the gopher+ indicator
    //

    if (gopherPlus != NULL) {
        if (urlBufferLength > 3) {
            memcpy(urlBuffer, "\t\t+", 3);
            urlBufferLength -= 3;
            urlBuffer += 3;
        }
    }

    //
    // finally terminate the URL
    //

    if (urlBufferLength >= 1) {
        *urlBuffer++ = '\0';
        --urlBufferLength;
    } else {
        return ERROR_INSUFFICIENT_BUFFER;
    }

    //
    // now escape any special characters (e.g. space, tab, etc.) in the url-path
    //

    *UrlLength = bufLen;

    error = EncodeUrlPath(NO_ENCODE_PATH_SEP,
                          SCHEME_GOPHER,
                          urlBuf,
                          sizeof(urlBuf) - urlBufferLength - 1,
                          Buffer,
                          UrlLength
                          );
    if (error == ERROR_SUCCESS) {
        *UrlLength += BufferLength - bufLen;
    }
    return error;
}
