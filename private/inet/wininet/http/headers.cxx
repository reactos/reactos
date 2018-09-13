/*++

Copyright (c) 1995  Microsoft Corporation

Module Name:

    headers.cxx

Abstract:

    Methods for HTTP_HEADERS (..\inc\http.hxx) classes

    Contents:

        HEADER_STRING::CreateHash
        HTTP_HEADERS::HeaderMatch
        HTTP_HEADERS::AllocateHeaders
        HTTP_HEADERS::FreeHeaders
        HTTP_HEADERS::CopyHeaders
        HTTP_HEADERS::FindFreeSlot
        HTTP_HEADERS::AddHeader
        HTTP_HEADERS::ReplaceHeader
        HTTP_HEADERS::FindHeader
        HTTP_HEADERS::QueryRawHeaders
        HTTP_HEADERS::AddRequest
        HTTP_HEADERS::ModifyRequest
        HTTP_HEADERS::SetRequestVersion
        HTTP_REQUEST_HANDLE_OBJECT::CreateRequestBuffer
        HTTP_REQUEST_HANDLE_OBJECT::QueryRequestHeader
        HTTP_REQUEST_HANDLE_OBJECT::AddInternalResponseHeader
        HTTP_REQUEST_HANDLE_OBJECT::UpdateResponseHeaders
        HTTP_REQUEST_HANDLE_OBJECT::CreateResponseHeaders
        HTTP_REQUEST_HANDLE_OBJECT::QueryResponseVersion
        HTTP_REQUEST_HANDLE_OBJECT::QueryStatusCode
        HTTP_REQUEST_HANDLE_OBJECT::QueryStatusText
        HTTP_REQUEST_HANDLE_OBJECT::QueryRawResponseHeaders
        HTTP_REQUEST_HANDLE_OBJECT::RemoveAllRequestHeadersByName
        HTTP_REQUEST_HANDLE_OBJECT::CheckWellKnownHeaders
        MapHttpMethodType
        CreateEscapedUrlPath
        (CalculateHashNoCase)
Author:

    Richard L Firth (rfirth) 20-Dec-1995

Revision History:

    20-Dec-1995 rfirth
        Created

--*/

#include <wininetp.h>
#include <perfdiag.hxx>
#include "httpp.h"

//
// private manifests
//

#define DATE_AND_TIME_STRING_BUFFER_LENGTH  128

#ifdef COMPRESSED_HEADERS

typedef struct tagHEADER_MAP {

    LPSTR   lpszLongHeader;
    DWORD   dwLenLongHeader;
    LPSTR   lpszShortHeader;
    DWORD   dwLenShortHeader;

}
HEADER_MAP;


// Sorted Compression map

HEADER_MAP rgsHeaderMap[] = {
{"",                    0,                                 "",    0              },
{"Accept",             sizeof("Accept")-1,               "#A", sizeof("#A")-1},
{"Accept-Charset",     sizeof("Accept-Charset")-1,       "#B", sizeof("#B")-1},
{"Accept-Encoding",    sizeof("Accept-Encoding")-1,      "#C", sizeof("#C")-1},
{"Accept-Language",    sizeof("Accept-Language")-1,      "#D", sizeof("#D")-1},
{"Accept-Ranges",      sizeof("Accept-Ranges")-1,        "#E", sizeof("#E")-1},
{"Age",                sizeof("Age")-1,                  "#F", sizeof("#F")-1},
{"Allow",              sizeof("Allow")-1,                "#G", sizeof("#G")-1},
{"Authorization",      sizeof("Authorization")-1,        "#H", sizeof("#H")-1},
{"Cache-Control",      sizeof("Cache-Control")-1,        "#I", sizeof("#I")-1},
{"Connection",         sizeof("Connection")-1,           "#J", sizeof("#J")-1},
{"Content-Base",       sizeof("Content-Base")-1,         "#K", sizeof("#K")-1},
{"Content-Encoding",   sizeof("Content-Encoding")-1,     "#L", sizeof("#L")-1},
{"Content-Language",   sizeof("Content-Language")-1,     "#M", sizeof("#M")-1},
{"Content-Length",     sizeof("Content-Length")-1,       "#N", sizeof("#N")-1},
{"Content-Location",   sizeof("Content-Location")-1,     "#O", sizeof("#O")-1},
{"Content-MD5",        sizeof("Content-MD5")-1,          "#P", sizeof("#P")-1},
{"Content-Range",      sizeof("Content-Range")-1,        "#Q", sizeof("#Q")-1},
{"Content-Type",       sizeof("Content-Type")-1,         "#R", sizeof("#R")-1},

{"Cookie",             sizeof("Cookie")-1,               "#5", sizeof("#5")-1},

{"Date",               sizeof("Date")-1,                 "#S", sizeof("#S")-1},
{"ETag",               sizeof("ETag")-1,                 "#T", sizeof("#T")-1},
{"Expires",            sizeof("Expires")-1,              "#U", sizeof("#U")-1},
{"From",               sizeof("From")-1,                 "#V", sizeof("#V")-1},
{"Host",               sizeof("Host")-1,                 "#W", sizeof("#W")-1},
{"If-Modified-Since",  sizeof("If-Modified-Since")-1,    "#X", sizeof("#X")-1},
{"If-Match",           sizeof("If-Match")-1,             "#Y", sizeof("#Y")-1},
{"If-None-Match",      sizeof("If-None-Match")-1,        "#Z", sizeof("#Z")-1},
{"If-Range",           sizeof("If-Range")-1,             "#a", sizeof("#a")-1},
{"If-Unmodified-Since",sizeof("If-Unmodified-Since")-1,  "#b", sizeof("#b")-1},
{"Last-Modified",      sizeof("Last-Modified")-1,        "#c", sizeof("#c")-1},
{"Location",           sizeof("Location")-1,             "#d", sizeof("#d")-1},
{"Max-Forwards",       sizeof("Max-Forwards")-1,         "#e", sizeof("#e")-1},
{"Pragma",             sizeof("Pragma")-1,               "#f", sizeof("#f")-1},
{"Proxy-Authenticate", sizeof("Proxy-Authenticate")-1,   "#g", sizeof("#g")-1},
{"Proxy-Authorization",sizeof("Proxy-Authorization")-1,  "#h", sizeof("#h")-1},
{"Public",             sizeof("Public")-1,               "#I", sizeof("#I")-1},
{"Range",              sizeof("Range")-1,                "#j", sizeof("#j")-1},
{"Referer",            sizeof("Referer")-1,              "#k", sizeof("#k")-1},
{"Retry-After",        sizeof("Retry-After")-1,          "#l", sizeof("#l")-1},
{"Server",             sizeof("Server")-1,               "#m", sizeof("#m")-1},
{"Transfer-Encoding",  sizeof("Transfer-Encoding")-1,    "#n", sizeof("#n")-1},

{"UA-color",           sizeof("UA-color")-1,             "#1", sizeof("#1")-1},
{"UA-cpu",             sizeof("UA-cpu")-1,               "#2", sizeof("#2")-1},
{"UA-OS",              sizeof("UA-OS")-1,                "#3", sizeof("#3")-1},
{"UA-pixels",          sizeof("UA-pixels")-1,            "#4", sizeof("#4")-1},

{"Upgrade",            sizeof("Upgrade")-1,              "#o", sizeof("#o")-1},
{"User-Agent",         sizeof("User-Agent")-1,           "#p", sizeof("#p")-1},
{"Vary",               sizeof("Vary")-1,                 "#q", sizeof("#q")-1},
{"Via",                sizeof("Via")-1,                  "#r", sizeof("#r")-1},
{"Warning",            sizeof("Warning")-1,              "#s", sizeof("#s")-1},
{"WWW-Authenticate",   sizeof("WWW-Authenticate")-1,     "#t", sizeof("#t")-1}
};


#endif //COMPRESSED_HEADERS





//
// Private functions
//

PRIVATE
BOOL
FMatchList(
    LPSTR *lplpList,
    DWORD cListLen,
    HEADER_STRING *lpHeader,
    LPSTR    lpBase
    );




//
// external functions
//

extern
BOOL
HttpDateToSystemTime(
    IN LPSTR lpszHttpDate,
    OUT LPSYSTEMTIME lpSystemTime
    );

#ifdef COMPRESSED_HEADERS
DWORD
LookupHeadermap(
    LPSTR   lpszHeader
);

extern BOOL vfCompressedHeaders;
#endif //COMPRESSED_HEADERS


DWORD
FASTCALL
CalculateHashNoCase(
    IN LPSTR lpszString,
    IN DWORD dwStringLength
    )

/*++

Routine Description:

    Calculate a case-insensitive hash number given a string. Assumes input is
    7-bit ASCII

Arguments:

    lpszString      - string to hash

    dwStringLength  - length of lpszString, or -1 if we need to calculate

Return Value:

    DWORD - a generated hash value

--*/

{
    DWORD dwHash = HEADER_HASH_SEED;

    while (dwStringLength != 0) {
        CHAR ch = *lpszString;

        if ((ch >= 'A') && (ch <= 'Z')) {
            ch = MAKE_LOWER(ch);
        }
        dwHash += (DWORD)(dwHash << 5) + ch; /*+ *pszName++;*/

        ++lpszString;
        --dwStringLength;
    }
    return dwHash;
}

//
// methods
//

VOID
inline
HEADER_STRING::CreateHash(
    LPSTR lpszBase
    )
{
    DWORD i = 0;
    LPSTR string = StringAddress(lpszBase);

    while ((i < (DWORD)StringLength())
           && !((string[i] == ':')
                || (string[i] == ' ')
                || (string[i] == '\r')
                || (string[i] == '\n'))) {
        ++i;
    }
    m_Hash = CalculateHashNoCase(string, i);
}



DWORD
HTTP_HEADERS::AllocateHeaders(
    IN DWORD dwNumberOfHeaders
    )

/*++

Routine Description:

    Allocates or grows the array of header pointers (HEADER_STRING objects)

Arguments:

    dwNumberOfHeaders   - number of additional header slots to create

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "AllocateHeaders",
                 "%d",
                 dwNumberOfHeaders
                 ));

    PERF_ENTER(AllocateHeaders);

    //
    // we really need to be able to realloc an array of HEADER_STRING objects
    // (see below)
    //

    DWORD error;
    DWORD slots = _TotalSlots;


    if ( (_TotalSlots + dwNumberOfHeaders) >  (INVALID_HEADER_INDEX-1))
    {
        INET_ASSERT(FALSE);
        _NextOpenSlot = 0;
        _TotalSlots = 0;
        _FreeSlots = 0;
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }


    _lpHeaders = (HEADER_STRING *)ResizeBuffer((HLOCAL)_lpHeaders,
                                               (_TotalSlots + dwNumberOfHeaders)
                                                    * sizeof(HEADER_STRING),
                                               FALSE // not moveable
                                               );
    if (_lpHeaders != NULL) {
        _NextOpenSlot = _TotalSlots;
        _TotalSlots += dwNumberOfHeaders;
        _FreeSlots += dwNumberOfHeaders;

        //
        // this is slightly ugly, but it seems there's no easy C++ way to
        // do this - we need to be able to realloc() an array of objects
        // created by new(), but so far, it can't be done
        //

        for (; slots < _TotalSlots; ++slots) {
            _lpHeaders[slots].Clear();
        }
        error = ERROR_SUCCESS;
    } else {

        INET_ASSERT(FALSE);
        _NextOpenSlot = 0;
        _TotalSlots = 0;
        _FreeSlots = 0;
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

quit:

    INET_ASSERT(_FreeSlots <= _TotalSlots);

    PERF_LEAVE(AllocateHeaders);

    DEBUG_LEAVE(error);

    return error;
}


VOID
HTTP_HEADERS::FreeHeaders(
    VOID
    )

/*++

Routine Description:

    Free the headers strings and the headers array

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "FreeHeaders",
                 NULL
                 ));

    LockHeaders();

    //
    // free up each individual entry (free string buffers)
    //

    for (DWORD i = 0; i < _TotalSlots; ++i) {
        _lpHeaders[i] = (LPSTR)NULL;
    }

    //
    // followed by the array itself
    //

    if (_lpHeaders) {
        _lpHeaders = (HEADER_STRING *)FREE_MEMORY((HLOCAL)_lpHeaders);

        INET_ASSERT(_lpHeaders == NULL);
    }

    _TotalSlots = 0;
    _FreeSlots = 0;
    _HeadersLength = 0;
    _lpszVerb = NULL;
    _dwVerbLength = 0;
    _lpszObjectName = NULL;
    _dwObjectNameLength = 0;
    _lpszVersion = NULL;
    _dwVersionLength = 0;

    UnlockHeaders();

    DEBUG_LEAVE(0);
}


VOID
HTTP_HEADERS::CopyHeaders(
    IN OUT LPSTR * lpBuffer,
    IN LPSTR lpszObjectName,
    IN DWORD dwObjectNameLength
    )

/*++

Routine Description:

    Copy the headers to the caller's buffer. Each header is terminated by CR-LF.
    This method is called to convert the request headers list to a buffer that
    we can send to the server

    N.B. This function MUST be called with the headers already locked

Arguments:

    lpBuffer            - pointer to pointer to buffer where headers are
                          written. We update the pointer

    lpszObjectName      - optional object name

    dwObjectNameLength  - optional object name length

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "CopyHeaders",
                 "%#x, %#x [%q], %d",
                 lpBuffer,
                 lpszObjectName,
                 lpszObjectName,
                 dwObjectNameLength
                 ));

    LockHeaders();

    DWORD i = 0;

    if (lpszObjectName != NULL) {
        memcpy(*lpBuffer, _lpszVerb, _dwVerbLength);
        *lpBuffer += _dwVerbLength;
        *(*lpBuffer)++ = ' ';
        memcpy(*lpBuffer, lpszObjectName, dwObjectNameLength);
        *lpBuffer += dwObjectNameLength;
        *(*lpBuffer)++ = ' ';
        memcpy(*lpBuffer, _lpszVersion, _dwVersionLength);
        *lpBuffer += _dwVersionLength;
        *(*lpBuffer)++ = '\r';
        *(*lpBuffer)++ = '\n';
        i = 1;
    }
    for (; i < _TotalSlots; ++i) {
        if (_lpHeaders[i].HaveString()) {
            _lpHeaders[i].CopyTo(*lpBuffer);
            *lpBuffer += _lpHeaders[i].StringLength();
            *(*lpBuffer)++ = '\r';
            *(*lpBuffer)++ = '\n';
        }
    }

    UnlockHeaders();

    DEBUG_LEAVE(0);
}

#ifdef COMPRESSED_HEADERS

VOID
HTTP_HEADERS::CopyCompressedHeaders(
    IN OUT LPSTR * lpBuffer
    )

/*++

Routine Description:

    Copy the headers to the caller's buffer. Each header is terminated by CR-LF.
    This method is called to convert the request headers list to a buffer that
    we can send to the server

    N.B. This function MUST be called with the headers already locked

Arguments:

    lpBuffer    - pointer to pointer to buffer where headers are written. We
                  update the pointer

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "CopyCompressedHeaders",
                 "%#x",
                 lpBuffer
                 ));
    LPSTR   lpszHeaderString;
    DWORD j;
    LockHeaders();

    for (DWORD i = 0; i < _TotalSlots; ++i) {
        if (_lpHeaders[i].HaveString()) {

            lpszHeaderString = _lpHeaders[i].StringAddress(NULL);

            j = LookupHeadermap(lpszHeaderString);

            if (j) {
                // copy the corresponding short header and bump the pointer
                // ahead


                DEBUG_PRINT(
                    HTTP,
                    INFO,
                    ("Compressed:%s using %s\n",
                        lpszHeaderString,
                        rgsHeaderMap[j].lpszShortHeader
                    )
                );

                strcpy(*lpBuffer, rgsHeaderMap[j].lpszShortHeader);

                *lpBuffer += rgsHeaderMap[j].dwLenShortHeader;

                // then copy the value of the header
                // and bump the destination further by the right amount
                strncpy(*lpBuffer,
                        lpszHeaderString + rgsHeaderMap[j].dwLenLongHeader,
                        _lpHeaders[i].StringLength() - rgsHeaderMap[j].dwLenLongHeader
                );

                *lpBuffer += (_lpHeaders[i].StringLength() - rgsHeaderMap[j].dwLenLongHeader);

            }
            else {

                // No match found, must be a header we don't know about
                DEBUG_PRINT(
                    HTTP,
                    INFO,
                    ("Couldn't compress header for %s\n",
                        lpszHeaderString
                    )
                );

                _lpHeaders[i].CopyTo(*lpBuffer);
                *lpBuffer += _lpHeaders[i].StringLength();

            }

            *(*lpBuffer)++ = '\r';
            *(*lpBuffer)++ = '\n';

        }
    }

    UnlockHeaders();

    DEBUG_LEAVE(0);
}
#endif //COMPRESSED_HEADERS


HEADER_STRING *
FASTCALL
HTTP_HEADERS::FindFreeSlot(
    DWORD* piSlot
    )

/*++

Routine Description:

    Finds the next free slot in the headers list, or adds some new slots

    N.B. This function MUST be called with the headers already locked

Arguments:

    piSlot: returns index of slot found

Return Value:

    HEADER_STRING *  - pointer to next free slot

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Pointer,
                 "FindFreeSlot",
                 NULL
                 ));

    PERF_ENTER(FindFreeSlot);

    DWORD i;
    DWORD error;
    HEADER_STRING * header = NULL;

    //
    // if there are no free slots, allocate some more
    //

    if (_FreeSlots == 0) {
        i = _TotalSlots;
        error = AllocateHeaders(HEADERS_INCREMENT);
    } else {
        i = 0;
        error = ERROR_SUCCESS;
        if (!_lpHeaders[_NextOpenSlot].HaveString())
        {
            --_FreeSlots;
            header = &_lpHeaders[_NextOpenSlot];
            *piSlot = _NextOpenSlot;
            _NextOpenSlot = (_NextOpenSlot == (_TotalSlots-1)) ? (_TotalSlots-1) : _NextOpenSlot++;
            goto quit;
        }
    }
    if (error == ERROR_SUCCESS) {
        for (; i < _TotalSlots; ++i) {
            if (!_lpHeaders[i].HaveString()) {
                --_FreeSlots;
                header = &_lpHeaders[i];
                *piSlot = i;
                _NextOpenSlot = (i == (_TotalSlots-1)) ? (_TotalSlots-1) : (i+1);
                break;
            }
        }
        if (header == NULL) {

            //
            // we would have just allocated extra slots if we didn't have
            // any, so we shouldn't be here
            //

            INET_ASSERT(FALSE);

            error = ERROR_INTERNET_INTERNAL_ERROR;
        }
    }

quit:
    _Error = error;

    PERF_LEAVE(FindFreeSlot);

    DEBUG_LEAVE(header);

    return header;
}


VOID
HTTP_HEADERS::ShrinkHeader(
    IN LPBYTE pbBase,
    IN DWORD  iSlot,
    IN DWORD  dwOldQueryIndex,
    IN DWORD  dwNewQueryIndex,
    IN DWORD  cbNewSize
    )

/*++

Routine Description:

    Low level function that does a surgical replace of one header with another.
    This code updates internal structures such as bKnownHeaders and the stored
    hash value for the new Header.

    N.B. This function MUST be called with the headers already locked

Arguments:


Return Value:

    None.

--*/

{
    HEADER_STRING* pHeader = _lpHeaders + iSlot;

    INET_ASSERT(_bKnownHeaders[dwOldQueryIndex] == (BYTE) iSlot ||
                dwNewQueryIndex == dwOldQueryIndex );

    //
    // Swap in the new header.  Update Length, Hash, and its cached position
    //  in the known header array.
    //

    _bKnownHeaders[dwOldQueryIndex] = INVALID_HEADER_INDEX;
    _bKnownHeaders[dwNewQueryIndex] = (BYTE) iSlot;

    pHeader->SetLength (cbNewSize);
    pHeader->SetHash (GlobalKnownHeaders[dwNewQueryIndex].HashVal);
}

DWORD
inline
HTTP_HEADERS::SlowFind(
    IN LPSTR lpBase,
    IN LPSTR lpszHeaderName,
    IN DWORD dwHeaderNameLength,
    IN DWORD dwIndex,
    IN DWORD dwHash,
    OUT DWORD *lpdwQueryIndex,
    OUT BYTE  **lplpbPrevIndex
    )

/*++

Routine Description:

    Finds the next occurance of lpszHeaderName in the header array, uses
    a cached table of well known headers to accerlate the search if the
    string is a known header.

    N.B. This function MUST be called with the headers already locked

Arguments:


Return Value:

    DWORD  - index to Slot in array, or INVALID_HEADER_SLOT if not found

--*/

{

    //
    // Now see if this is a known header passed in as a string,
    //   If it is, we save ourselves the loop, and just map it right in to a known header
    //

    DWORD dwKnownQueryIndex = GlobalHeaderHashs[(dwHash % MAX_HEADER_HASH_SIZE)];

    *lpdwQueryIndex = INVALID_HEADER_SLOT;

    if ( dwKnownQueryIndex != 0 )
    {
        dwKnownQueryIndex--;

        if ( ((int)dwHeaderNameLength >= GlobalKnownHeaders[dwKnownQueryIndex].Length) &&
             strnicmp(lpszHeaderName,
                      GlobalKnownHeaders[dwKnownQueryIndex].Text,
                      GlobalKnownHeaders[dwKnownQueryIndex].Length) == 0)
        {
            *lpdwQueryIndex = dwKnownQueryIndex;

            INET_ASSERT((int)(dwHeaderNameLength) == GlobalKnownHeaders[dwKnownQueryIndex].Length);

            if ( lplpbPrevIndex )
            {
                return FastNukeFind(
                        dwKnownQueryIndex,
                        dwIndex,
                        lplpbPrevIndex
                        );
            }
            else
            {
                return FastFind(
                        dwKnownQueryIndex,
                        dwIndex
                        );
            }
        }
    }

    //
    // Otherwise we painfully enumerate the whole array of headers
    //

    for (DWORD i = 0; i < _TotalSlots; ++i)
    {
        HEADER_STRING * pString;

        pString = &_lpHeaders[i];

        if (!pString->HaveString()) {
            continue;
        }

        if (pString->HashStrnicmp(lpBase,
                                  lpszHeaderName,
                                  dwHeaderNameLength,
                                  dwHash) == 0)
        {

            //
            // if we haven't reached the required index yet, continue
            //

            if (dwIndex != 0) {
                --dwIndex;
                continue;
            }

            return i; // found index/slot
        }
    }

    return INVALID_HEADER_SLOT; // not found
}


DWORD
inline
HTTP_HEADERS::FastFind(
    IN DWORD  dwQueryIndex,
    IN DWORD  dwIndex
    )
/*++

Routine Description:

    Finds the next occurance of a known header string in the lpHeaders array.
    Since this is a known string, an index is used to refer to it.
    A cached table of well known headers is used to accerlate the search.

    N.B. This function MUST be called with the headers already locked

Arguments:


Return Value:

    DWORD  - index to Slot in array, or INVALID_HEADER_SLOT if not found

--*/

{
    DWORD dwSlot;

    dwSlot = _bKnownHeaders[dwQueryIndex];

    while ( (dwIndex > 0) && (dwSlot < INVALID_HEADER_INDEX) )
    {
        HEADER_STRING * pString;

        pString = &_lpHeaders[dwSlot];
        dwSlot  = pString->GetNextKnownIndex();

        dwIndex--;
    }

    if ( dwSlot >= INVALID_HEADER_INDEX)
    {
        return INVALID_HEADER_SLOT;
    }

    return dwSlot; // found it.
}


DWORD
inline
HTTP_HEADERS::FastNukeFind(
    IN DWORD  dwQueryIndex,
    IN DWORD  dwIndex,
    OUT BYTE **lplpbPrevIndex
    )
/*++

Routine Description:

    Finds the next occurance of a known header string in the lpHeaders array.
    Since this is a known string, an index is used to refer to it.
    A cached table of well known headers is used to accerlate the search.
    Also provides a ptr to ptr to the slot which directs us to the one found.
    This is needed for deletion purposes.

    N.B. This function MUST be called with the headers already locked

Arguments:


Return Value:

    DWORD  - index to Slot in array, or INVALID_HEADER_SLOT if not found

--*/

{
    BYTE *lpbSlot;

    *lplpbPrevIndex = lpbSlot = &_bKnownHeaders[dwQueryIndex];
    dwIndex++;

    while ( (dwIndex > 0) && (*lpbSlot < INVALID_HEADER_INDEX) )
    {
        HEADER_STRING * pString;

        pString = &_lpHeaders[*lpbSlot];
        *lplpbPrevIndex = lpbSlot;
        lpbSlot  = pString->GetNextKnownIndexPtr();

        dwIndex--;
    }

    if ( **lplpbPrevIndex >= INVALID_HEADER_INDEX ||
         dwIndex > 0 )
    {
        return INVALID_HEADER_SLOT;
    }

    return ((DWORD) **lplpbPrevIndex); // found it.
}

VOID
HTTP_HEADERS::RemoveAllByIndex(
    IN DWORD dwQueryIndex
    )
/*++

Routine Description:

    Removes all Known Headers found in the header array.

    N.B. This function MUST be called with the headers already locked

Arguments:

    dwQueryIndex - index to known header string to remove from array.

Return Value:

    None.

--*/


{
    BYTE bSlot;
    BYTE bPrevSlot;

    bSlot = bPrevSlot  = _bKnownHeaders[dwQueryIndex];

    while (bSlot < INVALID_HEADER_INDEX)
    {
        HEADER_STRING * pString;

        bPrevSlot   = bSlot;
        pString     = &_lpHeaders[bSlot];
        bSlot       = (BYTE) pString->GetNextKnownIndex();

        RemoveHeader(bPrevSlot, dwQueryIndex, &_bKnownHeaders[dwQueryIndex]);
    }

    _bKnownHeaders[dwQueryIndex] = INVALID_HEADER_INDEX;

    return;
}


BOOL
inline
HTTP_HEADERS::HeaderMatch(
    IN DWORD dwHash,
    IN LPSTR lpszHeaderName,
    IN DWORD dwHeaderNameLength,
    OUT DWORD *lpdwQueryIndex
    )

/*++

Routine Description:

    Looks up a Known HTTP header string using its Hash value and
     string contained the name of the header.

Arguments:

    dwHash              - Hash value of header name string

    lpszHeaderName      - name of header we are matching

    dwHeaderNameLength  - length of header name string

    lpdwQueryIndex      - If found, this is the HTTP_QUERY_* based index to the header.

Return Value:

    BOOL
        Success - The string and hash matched againsted a known header

        Failure - There is no known header for that hash & string pair.

--*/

{
    *lpdwQueryIndex = GlobalHeaderHashs[(dwHash % MAX_HEADER_HASH_SIZE)];

    if ( *lpdwQueryIndex != 0 )
    {
        (*lpdwQueryIndex)--;

        if ( ((int)dwHeaderNameLength == GlobalKnownHeaders[*lpdwQueryIndex].Length) &&
             strnicmp(lpszHeaderName,
                      GlobalKnownHeaders[*lpdwQueryIndex].Text,
                      GlobalKnownHeaders[*lpdwQueryIndex].Length) == 0)
        {
            return TRUE;
        }
    }

    return FALSE;
}


BYTE
inline
HTTP_HEADERS::FastAdd(
    IN DWORD  dwQueryIndex,
    IN DWORD  dwSlot
    )
/*++

Routine Description:

    Rapidly adds a known string to the header array, this function
     is used to matain coherency of the _bKnownHeaders which
     contained indexed offsets into the header array for known headers.

    Note that this function is used instead of latter listed below
     in order to maintain proper order in headers received.

    N.B. This function MUST be called with the headers already locked

Arguments:

    dwQueryIndex - index to known header string to remove from array.

    dwSlot - Slot in which this header is being added.

Return Value:

    None.

--*/


{
    BYTE *lpbSlot;

    lpbSlot = &_bKnownHeaders[dwQueryIndex];

    while ( (*lpbSlot < INVALID_HEADER_INDEX) )
    {
        HEADER_STRING * pString;

        pString  = &_lpHeaders[*lpbSlot];
        lpbSlot  = pString->GetNextKnownIndexPtr();
    }

    INET_ASSERT(*lpbSlot == INVALID_HEADER_INDEX);

    *lpbSlot = (BYTE) dwSlot;
    return INVALID_HEADER_INDEX;
}


//BYTE
//inline
//HTTP_HEADERS::FastAdd(
//    IN DWORD  dwQueryIndex,
//    IN DWORD  dwSlot
//    )
//{
//    BYTE bOldSlot;
//
//    bOldSlot = _bKnownHeaders[dwQueryIndex];
//    _bKnownHeaders[dwQueryIndex] = (BYTE) dwSlot;
//
//    return bOldSlot;
//}




DWORD
HTTP_HEADERS::AddHeader(
    IN LPSTR lpszHeaderName,
    IN DWORD dwHeaderNameLength,
    IN LPSTR lpszHeaderValue,
    IN DWORD dwHeaderValueLength,
    IN DWORD dwIndex,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Adds a single header to the array of headers, given the header name and
    value. Called via HttpOpenRequest()

Arguments:

    lpszHeaderName      - pointer to name of header to add, e.g. "Accept:"

    dwHeaderNameLength  - length of the header name

    lpszHeaderValue     - pointer to value of header to add, e.g. "text/html"

    dwHeaderValueLength - length of the header value

    dwIndex             - if coalescing headers, index of header to update

    dwFlags             - flags controlling function:

                            COALESCE_HEADER_WITH_COMMA
                            COALESCE_HEADER_WITH_SEMICOLON
                                - headers of the same name can be combined

                            CLEAN_HEADER
                                - header is supplied by user, so we must ensure
                                  it has correct format

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Ran out of memory allocating string

                  ERROR_INVALID_PARAMETER
                    The header value was bad

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "AddHeader",
                 "%.*q, %d, %.*q, %d, %d, %#x",
                 min(dwHeaderNameLength + 1, 80),
                 lpszHeaderName,
                 dwHeaderNameLength,
                 min(dwHeaderValueLength + 1, 80),
                 lpszHeaderValue,
                 dwHeaderValueLength,
                 dwIndex,
                 dwFlags
                 ));

    PERF_ENTER(AddHeader);

    LockHeaders();

    INET_ASSERT(lpszHeaderName != NULL);
    INET_ASSERT(*lpszHeaderName != '\0');
    INET_ASSERT(dwHeaderNameLength != 0);
    INET_ASSERT(lpszHeaderValue != NULL);
    INET_ASSERT(*lpszHeaderValue != '\0');
    INET_ASSERT(dwHeaderValueLength != 0);
    INET_ASSERT(_FreeSlots <= _TotalSlots);

    //
    // we may have been handed a header with a trailing colon. We don't care
    // for such nasty imagery
    //

    if (lpszHeaderName[dwHeaderNameLength - 1] == ':') {
        --dwHeaderNameLength;
    }

    DWORD error = ERROR_HTTP_HEADER_NOT_FOUND;
    DWORD dwQueryIndex;
    DWORD dwHash = CalculateHashNoCase(lpszHeaderName, dwHeaderNameLength);

    DWORD i = 0;

    //
    // if we are coalescing headers then find a header with the same name
    //

    if ((dwFlags & COALESCE_HEADER_WITH_COMMA) ||
        (dwFlags & COALESCE_HEADER_WITH_SEMICOLON) )
    {
        DWORD dwSlot;

        dwSlot = SlowFind(
                    NULL,
                    lpszHeaderName,
                    dwHeaderNameLength,
                    dwIndex,
                    dwHash,
                    &dwQueryIndex,
                    NULL
                    );

        if (dwSlot != ((DWORD) -1))
        {

            HEADER_STRING * pString;

            pString = &_lpHeaders[dwSlot];

            //
            // found what we are looking for. Coalesce it
            //

            pString->ResizeString((sizeof("; ")-1) + dwHeaderValueLength); // save us from multiple reallocs

            pString->Strncat(
                             (dwFlags & COALESCE_HEADER_WITH_SEMICOLON) ?
                                 "; " :
                                 ", ",
                              2);

            pString->Strncat(lpszHeaderValue, dwHeaderValueLength);
            _HeadersLength += 2 + dwHeaderValueLength;
            error = ERROR_SUCCESS;

        }
    }
    else
    {

        //
        // Check to verify that the header we're adding is a known header,
        //   If its a known header we use dwQueryIndex to update the known header array
        //   otherwise, IF ITS NOT, we make sure to set dwQueryIndex to INVALID_...
        //

        if (! HeaderMatch(dwHash, lpszHeaderName, dwHeaderNameLength, &dwQueryIndex) )
        {
            dwQueryIndex = INVALID_HEADER_SLOT;
        }

        /*
        // Perhaps this more efficent ???
        dwQueryIndex = GlobalHeaderHashs[(dwHash % MAX_HEADER_HASH_SIZE)];

        if ( dwQueryIndex != 0 )
        {
            dwQueryIndex--;

            if ( ((int)dwHeaderNameLength < GlobalKnownHeaders[dwQueryIndex].Length) ||
                 strnicmp(lpszHeaderName,
                          GlobalKnownHeaders[dwQueryIndex].Text,
                          GlobalKnownHeaders[dwQueryIndex].Length) != 0)
            {
                dwQueryIndex = INVALID_HEADER_SLOT;
            }
        }
        else
        {
            dwQueryIndex = INVALID_HEADER_SLOT;
        }
        */
    }


    //
    // if we didn't find the header value or we are not coalescing then add the
    // header
    //

    if (error == ERROR_HTTP_HEADER_NOT_FOUND)
    {
        //
        // find the next slot for this header
        //

        HEADER_STRING * freeHeader;
        DWORD iSlot;

        freeHeader = FindFreeSlot(&iSlot);
        if (freeHeader == NULL) {
            error = GetError();

            INET_ASSERT(error != ERROR_SUCCESS);

            goto quit;
        }


        freeHeader->CreateStringBuffer((LPVOID)lpszHeaderName,
                                       dwHeaderNameLength,
                                       dwHeaderNameLength
                                       + sizeof(": ") - 1
                                       + dwHeaderValueLength
                                       + 1 // for extra NULL terminator
                                       );
        if (freeHeader->IsError()) {
            error = ::GetLastError();

            INET_ASSERT(error != ERROR_SUCCESS);

            goto quit;
        }
        freeHeader->Strncat((LPVOID)": ", sizeof(": ") - 1);
        freeHeader->Strncat((LPVOID)lpszHeaderValue, dwHeaderValueLength);
        _HeadersLength += dwHeaderNameLength
                        + (sizeof(": ") - 1)
                        + dwHeaderValueLength
                        + (sizeof("\r\n") - 1)
                        ;
        freeHeader->SetHash(dwHash);

        if ( dwQueryIndex != INVALID_HEADER_SLOT )
        {
            freeHeader->SetNextKnownIndex(FastAdd(dwQueryIndex, iSlot));
        }

        error = ERROR_SUCCESS;
    }

quit:

    UnlockHeaders();

    PERF_LEAVE(AddHeader);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_HEADERS::AddHeader(
    IN DWORD dwQueryIndex,
    IN LPSTR lpszHeaderValue,
    IN DWORD dwHeaderValueLength,
    IN DWORD dwIndex,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Adds a single header to the array of headers, given the header name and
    value. Called via HttpOpenRequest()

Arguments:

    dwQueryIndex        - a index into a array of known HTTP headers, see wininet.h HTTP_QUERY_* codes

    lpszHeaderValue     - pointer to value of header to add, e.g. "text/html"

    dwHeaderValueLength - length of the header value

    dwIndex             - if coalescing headers, index of header to update

    dwFlags             - flags controlling function:

                            COALESCE_HEADER_WITH_COMMA
                            COALESCE_HEADER_WITH_SEMICOLON
                                - headers of the same name can be combined

                            CLEAN_HEADER
                                - header is supplied by user, so we must ensure
                                  it has correct format

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Ran out of memory allocating string

                  ERROR_INVALID_PARAMETER
                    The header value was bad

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "AddHeader",
                 "%q, %u, %.*q, %d, %d, %#x",
                 GlobalKnownHeaders[dwQueryIndex].Text,
                 dwQueryIndex,
                 min(dwHeaderValueLength + 1, 80),
                 lpszHeaderValue,
                 dwHeaderValueLength,
                 dwIndex,
                 dwFlags
                 ));

    PERF_ENTER(AddHeader);

    INET_ASSERT(dwQueryIndex <= HTTP_QUERY_MAX);
    INET_ASSERT(lpszHeaderValue != NULL);
    INET_ASSERT(*lpszHeaderValue != '\0');
    INET_ASSERT(dwHeaderValueLength != 0);
    INET_ASSERT(_FreeSlots <= _TotalSlots);

    DWORD error = ERROR_HTTP_HEADER_NOT_FOUND;
    DWORD i = 0;
    LPSTR lpszHeaderName;
    DWORD dwHeaderNameLength;
    DWORD dwHash;

    dwHash             = GlobalKnownHeaders[dwQueryIndex].HashVal;
    lpszHeaderName     = GlobalKnownHeaders[dwQueryIndex].Text;
    dwHeaderNameLength = GlobalKnownHeaders[dwQueryIndex].Length;

    //
    // if we are coalescing headers then find a header with the same name
    //

    if ((dwFlags & COALESCE_HEADER_WITH_COMMA) ||
        (dwFlags & COALESCE_HEADER_WITH_SEMICOLON) )
    {
        DWORD dwSlot;

        dwSlot = FastFind(
                    dwQueryIndex,
                    dwIndex
                    );

        if (dwSlot != INVALID_HEADER_SLOT)
        {

            HEADER_STRING * pString;

            pString = &_lpHeaders[dwSlot];

            //
            // found what we are looking for. Coalesce it
            //

            pString->ResizeString((sizeof("; ")-1) + dwHeaderValueLength); // save us from multiple reallocs

            pString->Strncat(
                             (dwFlags & COALESCE_HEADER_WITH_SEMICOLON) ?
                                 "; " :
                                 ", ",
                              2);

            pString->Strncat(lpszHeaderValue, dwHeaderValueLength);
            _HeadersLength += 2 + dwHeaderValueLength;
            error = ERROR_SUCCESS;

        }
    }


    //
    // if we didn't find the header value or we are not coalescing then add the
    // header
    //

    if (error == ERROR_HTTP_HEADER_NOT_FOUND)
    {
        //
        // find the next slot for this header
        //

        HEADER_STRING * freeHeader;
        DWORD iSlot;

        freeHeader = FindFreeSlot(&iSlot);
        if (freeHeader == NULL) {
            error = GetError();

            INET_ASSERT(error != ERROR_SUCCESS);

            goto quit;
        }


        freeHeader->CreateStringBuffer((LPVOID)lpszHeaderName,
                                       dwHeaderNameLength,
                                       dwHeaderNameLength
                                       + sizeof(": ") - 1
                                       + dwHeaderValueLength
                                       + 1 // for extra NULL terminator
                                       );
        if (freeHeader->IsError()) {
            error = ::GetLastError();

            INET_ASSERT(error != ERROR_SUCCESS);

            goto quit;
        }
        freeHeader->Strncat((LPVOID)": ", sizeof(": ") - 1);
        freeHeader->Strncat((LPVOID)lpszHeaderValue, dwHeaderValueLength);
        _HeadersLength += dwHeaderNameLength
                        + (sizeof(": ") - 1)
                        + dwHeaderValueLength
                        + (sizeof("\r\n") - 1)
                        ;
        freeHeader->SetHash(dwHash);
        freeHeader->SetNextKnownIndex(FastAdd(dwQueryIndex, iSlot));

        error = ERROR_SUCCESS;
    }

quit:

    PERF_LEAVE(AddHeader);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_HEADERS::ReplaceHeader(
    IN LPSTR lpszHeaderName,
    IN DWORD dwHeaderNameLength,
    IN LPSTR lpszHeaderValue,
    IN DWORD dwHeaderValueLength,
    IN DWORD dwIndex,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Replaces a HTTP (request) header. The header can be replaced with a NULL
    value, meaning that the header is removed

Arguments:

    lpszHeaderName      - pointer to the header name

    dwHeaderNameLength  - length of the header name

    lpszHeaderValue     - pointer to the header value

    dwHeaderValueLength - length of the header value

    dwIndex             - index of header to replace

    dwFlags             - flags controlling function. Allowed flags are:

                            COALESCE_HEADER_WITH_COMMA
                            COALESCE_HEADER_WITH_SEMICOLON
                                - headers of the same name can be combined

                            ADD_HEADER
                                - if the header-name is not found and there is
                                  a valid header-value, then the header is added

                            ADD_HEADER_IF_NEW
                                - if the header-name exists then we return an
                                  error, else we add the header-value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_HTTP_HEADER_NOT_FOUND
                    The requested header wasn't found

                  ERROR_HTTP_HEADER_ALREADY_EXISTS
                    The header already exists, and was not added or replaced

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "ReplaceHeader",
                 "%.*q, %d, %.*q, %d, %d, %#x",
                 min(dwHeaderNameLength + 1, 80),
                 lpszHeaderName,
                 dwHeaderNameLength,
                 min(dwHeaderValueLength + 1, 80),
                 lpszHeaderValue,
                 dwHeaderValueLength,
                 dwIndex,
                 dwFlags
                 ));

    PERF_ENTER(ReplaceHeader);

    INET_ASSERT(lpszHeaderName != NULL);
    INET_ASSERT(dwHeaderNameLength != 0);
    INET_ASSERT(lpszHeaderName[dwHeaderNameLength - 1] != ':');

    DWORD error = ERROR_HTTP_HEADER_NOT_FOUND;
    DWORD dwHash = CalculateHashNoCase(lpszHeaderName, dwHeaderNameLength);
    DWORD dwSlot;
    DWORD dwQueryIndex;
    BYTE *pbPrevByte;

    LockHeaders();

    dwSlot = SlowFind(
                NULL,
                lpszHeaderName,
                dwHeaderNameLength,
                dwIndex,
                dwHash,
                &dwQueryIndex,
                &pbPrevByte
                );

    if ( dwSlot != ((DWORD) -1))
    {
        //
        // if ADD_HEADER_IF_NEW is set, then we already have the header
        //

        if (dwFlags & ADD_HEADER_IF_NEW) {
            error = ERROR_HTTP_HEADER_ALREADY_EXISTS;
            goto quit;
        }

        //
        // for both replace and remove operations, we are going to remove
        // the current header
        //

        RemoveHeader(dwSlot, dwQueryIndex, pbPrevByte);

        //
        // if replacing then add the new header value
        //

        if (dwHeaderValueLength != 0)
        {
            if ( dwQueryIndex != ((DWORD) -1) )
            {
                error = AddHeader(dwQueryIndex,
                                  lpszHeaderValue,
                                  dwHeaderValueLength,
                                  0,
                                  dwFlags
                                  );
            }
            else
            {
                error = AddHeader(lpszHeaderName,
                                  dwHeaderNameLength,
                                  lpszHeaderValue,
                                  dwHeaderValueLength,
                                  0,
                                  dwFlags
                                  );
            }


        } else {
            error = ERROR_SUCCESS;
        }
    }

    //
    // if we didn't find the header but ADD_HEADER is set then we simply add it
    // but only if the value length is not zero
    //

    if ((error == ERROR_HTTP_HEADER_NOT_FOUND)
    && (dwHeaderValueLength != 0)
    && (dwFlags & (ADD_HEADER | ADD_HEADER_IF_NEW)))
    {
        if ( dwQueryIndex != ((DWORD) -1) )
        {
            error = AddHeader(dwQueryIndex,
                              lpszHeaderValue,
                              dwHeaderValueLength,
                              0,
                              dwFlags
                              );
        }
        else
        {
            error = AddHeader(lpszHeaderName,
                              dwHeaderNameLength,
                              lpszHeaderValue,
                              dwHeaderValueLength,
                              0,
                              dwFlags
                              );

        }
    }

quit:

    UnlockHeaders();

    PERF_LEAVE(ReplaceHeader);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_HEADERS::ReplaceHeader(
    IN DWORD dwQueryIndex,
    IN LPSTR lpszHeaderValue,
    IN DWORD dwHeaderValueLength,
    IN DWORD dwIndex,
    IN DWORD dwFlags
    )

/*++

Routine Description:

    Replaces a HTTP (request) header. The header can be replaced with a NULL
    value, meaning that the header is removed

Arguments:

    lpszHeaderValue     - pointer to the header value

    dwQueryIndex        - a index into a array of known HTTP headers, see wininet.h HTTP_QUERY_* codes

    dwHeaderValueLength - length of the header value

    dwIndex             - index of header to replace

    dwFlags             - flags controlling function. Allowed flags are:

                            COALESCE_HEADER_WITH_COMMA
                            COALESCE_HEADER_WITH_SEMICOLON
                                - headers of the same name can be combined

                            ADD_HEADER
                                - if the header-name is not found and there is
                                  a valid header-value, then the header is added

                            ADD_HEADER_IF_NEW
                                - if the header-name exists then we return an
                                  error, else we add the header-value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_HTTP_HEADER_NOT_FOUND
                    The requested header wasn't found

                  ERROR_HTTP_HEADER_ALREADY_EXISTS
                    The header already exists, and was not added or replaced

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "ReplaceHeader",
                 "%q, %u, %.*q, %d, %d, %#x",
                 GlobalKnownHeaders[dwQueryIndex].Text,
                 dwQueryIndex,
                 min(dwHeaderValueLength + 1, 80),
                 lpszHeaderValue,
                 dwHeaderValueLength,
                 dwIndex,
                 dwFlags
                 ));

    PERF_ENTER(ReplaceHeader);

    DWORD error = ERROR_HTTP_HEADER_NOT_FOUND;
    DWORD dwSlot;
    BYTE *pbPrevByte;

    LockHeaders();

    dwSlot = FastNukeFind(
                dwQueryIndex,
                dwIndex,
                &pbPrevByte
                );

    if ( dwSlot != INVALID_HEADER_SLOT)
    {
        //
        // if ADD_HEADER_IF_NEW is set, then we already have the header
        //

        if (dwFlags & ADD_HEADER_IF_NEW) {
            error = ERROR_HTTP_HEADER_ALREADY_EXISTS;
            goto quit;
        }

        //
        // for both replace and remove operations, we are going to remove
        // the current header
        //

        RemoveHeader(dwSlot, dwQueryIndex, pbPrevByte);

        //
        // if replacing then add the new header value
        //

        if (dwHeaderValueLength != 0)
        {
            error = AddHeader(dwQueryIndex,
                              lpszHeaderValue,
                              dwHeaderValueLength,
                              0,
                              dwFlags
                              );
        } else {
            error = ERROR_SUCCESS;
        }
    }

    //
    // if we didn't find the header but ADD_HEADER is set then we simply add it
    // but only if the value length is not zero
    //

    if ((error == ERROR_HTTP_HEADER_NOT_FOUND)
    && (dwHeaderValueLength != 0)
    && (dwFlags & (ADD_HEADER | ADD_HEADER_IF_NEW)))
    {
        error = AddHeader(dwQueryIndex,
                          lpszHeaderValue,
                          dwHeaderValueLength,
                          0,
                          dwFlags
                          );
    }

quit:

    UnlockHeaders();

    PERF_LEAVE(ReplaceHeader);

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_HEADERS::FindHeader(
    IN LPSTR lpBase,
    IN LPSTR lpszHeaderName,
    IN DWORD dwHeaderNameLength,
    IN DWORD dwModifiers,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex
    )

/*++

Routine Description:

    Finds a request or response header

Arguments:

    lpBase              - base for offset HEADER_STRINGs

    lpszHeaderName      - pointer to header name

    dwHeaderNameLength  - length of header name

    dwModifiers         - flags which modify returned value

    lpBuffer            - pointer to buffer for results

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: length of results, or required length of lpBuffer

    lpdwIndex           - IN: 0-based index of header to find
                          OUT: next header index if success returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    *lpdwBufferLength contains the amount required

                  ERROR_HTTP_HEADER_NOT_FOUND
                    The specified header (or index of header) was not found

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_HEADERS::FindHeader",
                 "%#x [%.*q], %d, %#x, %#x [%#x], %#x, %#x [%d]",
                 lpszHeaderName,
                 min(dwHeaderNameLength + 1, 80),
                 lpszHeaderName,
                 dwHeaderNameLength,
                 lpBuffer,
                 lpdwBufferLength,
                 *lpdwBufferLength,
                 dwModifiers,
                 lpdwIndex,
                 *lpdwIndex
                 ));


    PERF_ENTER(FindHeader);



    INET_ASSERT(lpdwIndex != NULL);

    DWORD error = ERROR_HTTP_HEADER_NOT_FOUND;
    DWORD dwSlot;
    HEADER_STRING * pString;
    DWORD dwQueryIndex;
    DWORD dwHash = CalculateHashNoCase(lpszHeaderName, dwHeaderNameLength);

    LockHeaders();

    dwSlot = SlowFind(
                lpBase,
                lpszHeaderName,
                dwHeaderNameLength,
                *lpdwIndex,
                dwHash,
                &dwQueryIndex,
                NULL
                );

    if ( dwSlot != ((DWORD) -1) )
    {
        pString = &_lpHeaders[dwSlot];

        //
        // found the header - get to the value
        //

        DWORD stringLen;
        LPSTR value;

        stringLen = pString->StringLength();

        INET_ASSERT(stringLen > dwHeaderNameLength);

        //
        // get a pointer to the value string
        //

        value = pString->StringAddress(lpBase) + dwHeaderNameLength;
        stringLen -= dwHeaderNameLength;

        //
        // the input string could be a substring of a different header
        //

        //INET_ASSERT(*value != ':');

        //
        // find the first non-space character in the value.
        //
        // N.B.: Servers can return empty headers, so we may end up with a
        // zero length string
        //

        do {
            ++value;
            --stringLen;
        } while ((stringLen > 0) && (*value == ' '));

        //
        // get the data in the format requested by the app
        //

        LPVOID lpData;
        DWORD dwDataSize;
        DWORD dwRequiredSize;
        SYSTEMTIME systemTime;
        DWORD number;

        //
        // error is no longer ERROR_HTTP_HEADER_NOT_FOUND, but it might not
        // really be success either...
        //

        error = ERROR_SUCCESS;

        if (dwModifiers & HTTP_QUERY_FLAG_SYSTEMTIME) {

            char buf[DATE_AND_TIME_STRING_BUFFER_LENGTH];

            if (stringLen < sizeof(buf)) {

                //
                // value probably does not point at a zero-terminated string
                // which HttpDateToSystemTime() expects, so we make a copy
                // and terminate it
                //

                memcpy((LPVOID)buf, (LPVOID)value, stringLen);
                buf[stringLen] = '\0';
                if (HttpDateToSystemTime(buf, &systemTime)) {
                    lpData = (LPVOID)&systemTime;
                    dwRequiredSize = dwDataSize = sizeof(systemTime);
                } else {

                    //
                    // couldn't convert date/time. Presume header must be bogus
                    //

                    error = ERROR_HTTP_INVALID_QUERY_REQUEST;

                    DEBUG_PRINT(HTTP,
                                ERROR,
                                ("cannot convert %.40q to SYSTEMTIME\n",
                                value
                                ));

                }
            } else {

                //
                // we would break the date/time buffer!
                //

                error = ERROR_INTERNET_INTERNAL_ERROR;
            }
        } else if (dwModifiers & HTTP_QUERY_FLAG_NUMBER) {
            if (isdigit(*value)) {
                number = 0;
                for (int i = 0;
                    (stringLen > 0) && isdigit(value[i]);
                    ++i, --stringLen) {

                    number = number * 10 + (DWORD)(value[i] - '0');
                }
                lpData = (LPVOID)&number;
                dwRequiredSize = dwDataSize = sizeof(number);
            } else {

                //
                // not a numeric field. Request must be bogus for this header
                //

                error = ERROR_HTTP_INVALID_QUERY_REQUEST;

                DEBUG_PRINT(HTTP,
                            ERROR,
                            ("cannot convert %.20q to NUMBER\n",
                            value
                            ));

            }
        } else {
            lpData = (LPVOID)value;
            dwDataSize = stringLen;
            dwRequiredSize = dwDataSize + 1;
        }

        //
        // if error == ERROR_SUCCESS then we can attempt to copy the data
        //

        if (error == ERROR_SUCCESS) {
            if (*lpdwBufferLength < dwRequiredSize) {
                *lpdwBufferLength = dwRequiredSize;
                error = ERROR_INSUFFICIENT_BUFFER;
            } else {
                memcpy(lpBuffer, lpData, dwDataSize);
                *lpdwBufferLength = dwDataSize;

                //
                // if dwRequiredSize > dwDataSize, then this is a variable-
                // length item (i.e. a STRING!) so we add a terminating '\0'
                //

                if (dwRequiredSize > dwDataSize) {

                    INET_ASSERT(dwRequiredSize - dwDataSize == 1);

                    ((LPSTR)lpBuffer)[dwDataSize] = '\0';
                }

                //
                // successfully retrieved the requested header - bump the
                // index
                //

                ++*lpdwIndex;
            }
        }
    }

    UnlockHeaders();

    PERF_LEAVE(FindHeader);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_HEADERS::FindHeader(
    IN LPSTR lpBase,
    IN DWORD dwQueryIndex,
    IN DWORD dwModifiers,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN OUT LPDWORD lpdwIndex
    )
/*++

Routine Description:

    Finds a request or response header, based on index to the header name we are searching for.

Arguments:

    lpBase              - base for offset HEADER_STRINGs

    dwQueryIndex        - a index into a array of known HTTP headers, see wininet.h HTTP_QUERY_* codes

    dwModifiers         - flags which modify returned value

    lpBuffer            - pointer to buffer for results

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: length of results, or required length of lpBuffer

    lpdwIndex           - IN: 0-based index of header to find
                          OUT: next header index if success returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    *lpdwBufferLength contains the amount required

                  ERROR_HTTP_HEADER_NOT_FOUND
                    The specified header (or index of header) was not found

--*/
{

    DWORD error;
    LPSTR lpData;
    DWORD dwDataSize = 0;
    DWORD dwRequiredSize;
    SYSTEMTIME systemTime;
    DWORD number;

    error = FastFindHeader(
                lpBase,
                dwQueryIndex,
                (LPVOID *)&lpData,
                &dwDataSize,
                *lpdwIndex
                );

    if ( error != ERROR_SUCCESS )
    {
        goto quit;
    }

    //
    // get the data in the format requested by the app
    //

    if (dwModifiers & HTTP_QUERY_FLAG_SYSTEMTIME)
    {
        char buf[DATE_AND_TIME_STRING_BUFFER_LENGTH];

        if (dwDataSize < sizeof(buf))
        {

            //
            // value probably does not point at a zero-terminated string
            // which HttpDateToSystemTime() expects, so we make a copy
            // and terminate it
            //

            memcpy((LPVOID)buf, (LPVOID)lpData, dwDataSize);
            buf[dwDataSize] = '\0';
            if (HttpDateToSystemTime(buf, &systemTime)) {
                lpData = (LPSTR)&systemTime;
                dwRequiredSize = dwDataSize = sizeof(systemTime);
            } else {

                //
                // couldn't convert date/time. Presume header must be bogus
                //

                error = ERROR_HTTP_INVALID_QUERY_REQUEST;

                DEBUG_PRINT(HTTP,
                            ERROR,
                            ("cannot convert %.40q to SYSTEMTIME\n",
                            lpData
                            ));

            }
        }
        else
        {

            //
            // we would break the date/time buffer!
            //

            error = ERROR_INTERNET_INTERNAL_ERROR;
        }
    }
    else if (dwModifiers & HTTP_QUERY_FLAG_NUMBER)
    {
        if (isdigit(*lpData)) {
            number = 0;
            for (int i = 0;
                (dwDataSize > 0) && isdigit(lpData[i]);
                ++i, --dwDataSize) {

                number = number * 10 + (DWORD)(lpData[i] - '0');
            }
            lpData = (LPSTR)&number;
            dwRequiredSize = dwDataSize = sizeof(number);
        } else {

            //
            // not a numeric field. Request must be bogus for this header
            //

            error = ERROR_HTTP_INVALID_QUERY_REQUEST;

            DEBUG_PRINT(HTTP,
                        ERROR,
                        ("cannot convert %.20q to NUMBER\n",
                        lpData
                        ));

        }
    }
    else
    {
        dwRequiredSize = dwDataSize + 1;
    }

    //
    // if error == ERROR_SUCCESS then we can attempt to copy the data
    //

    if (error == ERROR_SUCCESS)
    {
        if (*lpdwBufferLength < dwRequiredSize)
        {
            *lpdwBufferLength = dwRequiredSize;
            error = ERROR_INSUFFICIENT_BUFFER;
        }
        else
        {
            memcpy(lpBuffer, lpData, dwDataSize);
            *lpdwBufferLength = dwDataSize;

            //
            // if dwRequiredSize > dwDataSize, then this is a variable-
            // length item (i.e. a STRING!) so we add a terminating '\0'
            //

            if (dwRequiredSize > dwDataSize)
            {
                INET_ASSERT(dwRequiredSize - dwDataSize == 1);

                ((LPSTR)lpBuffer)[dwDataSize] = '\0';
            }

            //
            // successfully retrieved the requested header - bump the
            // index
            //

            ++*lpdwIndex;
        }
    }
quit:

    return error;
}



DWORD
HTTP_HEADERS::FastFindHeader(
    IN LPSTR lpBase,
    IN DWORD dwQueryIndex,
    OUT LPVOID *lplpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwIndex
    )

/*++

Routine Description:

    Finds a request or response header slightly quicker than its higher level
     cousin, FindHeader.   Unlike FindHeader this function simply returns
     a pointer and length, and does not copy header data.


    lpBase              - base address of strings

    dwQueryIndex        - a index into a array known HTTP headers, see wininet.h HTTP_QUERY_* codes

    lplpBuffer          - pointer to pointer of the actual header to be returned in.

    lpdwBufferLength    - OUT: if successful, length of output buffer, minus 1
                               for any trailing EOS, or if the buffer is not
                               large enough, the size required

    dwIndex             - a index of which header we're asking for, as there can be multiple headers
                          under the same name.

Arguments:

    lpBase              - base for offset HEADER_STRINGs

    lpszHeaderName      - pointer to header name

    dwHeaderNameLength  - length of header name

    dwModifiers         - flags which modify returned value

    lpBuffer            - pointer to buffer for results

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: length of results, or required length of lpBuffer

    lpdwIndex           - IN: 0-based index of header to find
                          OUT: next header index if success returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    *lpdwBufferLength contains the amount required

                  ERROR_HTTP_HEADER_NOT_FOUND
                    The specified header (or index of header) was not found

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_HEADERS::FastFindHeader",
                 "%q, %#x, %#x [%#x], %u",
                 GlobalKnownHeaders[dwQueryIndex].Text,
                 lplpBuffer,
                 lpdwBufferLength,
                 *lpdwBufferLength,
                 dwIndex
                 ));

    PERF_ENTER(FindHeader);

    DWORD error = ERROR_HTTP_HEADER_NOT_FOUND;

    HEADER_STRING * curHeader;
    DWORD dwSlot;

    dwSlot = FastFind(dwQueryIndex, dwIndex);

    if ( dwSlot != INVALID_HEADER_SLOT)
    {
        //
        // found the header - get to the value
        //

        DWORD stringLen;
        LPSTR value;

        curHeader = GetSlot(dwSlot);

        //
        // get a pointer to the value string
        //

        value     = curHeader->StringAddress(lpBase) + (GlobalKnownHeaders[dwQueryIndex].Length+1);
        stringLen = curHeader->StringLength() - (GlobalKnownHeaders[dwQueryIndex].Length+1);

        //
        // find the first non-space character in the value.
        //
        // N.B.: Servers can return empty headers, so we may end up with a
        // zero length string
        //

        while ((stringLen > 0) && (*value == ' '))
        {
            ++value;
            --stringLen;
        }

        //
        // get the data in the format requested by the app
        //

        //
        // error is no longer ERROR_HTTP_HEADER_NOT_FOUND, but it might not
        // really be success either...
        //

        error = ERROR_SUCCESS;

        *lplpBuffer = (LPVOID)value;
        *lpdwBufferLength = stringLen;
    }

    PERF_LEAVE(FindHeader);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_HEADERS::QueryRawHeaders(
    IN LPSTR lpBase,
    IN BOOL bCrLfTerminated,
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns all the request or response headers in a single buffer. The headers
    can be returned as ASCIIZ strings, or CR-LF terminated strings

Arguments:

    lpBase              - base address of strings

    bCrLfTerminated     - TRUE if each string is terminated with CR-LF

    lpBuffer            - pointer to buffer to write headers

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: if successful, length of output buffer, minus 1
                               for any trailing EOS, or if the buffer is not
                               large enough, the size required

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER

--*/

{
    PERF_ENTER(QueryRawHeaders);

    DWORD requiredLength = 0;
    LPSTR lpszBuffer = (LPSTR)lpBuffer;

    LockHeaders();

    for (DWORD i = 0; i < _TotalSlots; ++i) {
        if (_lpHeaders[i].HaveString()) {

            DWORD length;

            length = _lpHeaders[i].StringLength();

            requiredLength += length + (bCrLfTerminated ? 2 : 1);
            if (*lpdwBufferLength > requiredLength) {
                _lpHeaders[i].CopyTo(lpBase, lpszBuffer);
                lpszBuffer += length;
                if (bCrLfTerminated) {
                    *lpszBuffer++ = '\r';
                    *lpszBuffer++ = '\n';
                } else {
                    *lpszBuffer++ = '\0';
                }
            }
        }
    }

    if (bCrLfTerminated)
    {
        requiredLength += 2;
        if (*lpdwBufferLength > requiredLength)
        {
            *lpszBuffer++ = '\r';
            *lpszBuffer++ = '\n';
        }
    }

    UnlockHeaders();

    ++requiredLength;

    DWORD error;

    if (*lpdwBufferLength < requiredLength) {
        error = ERROR_INSUFFICIENT_BUFFER;
    } else {
        *lpszBuffer = '\0';
        --requiredLength;   // remove 1 for trailing '\0'
        error = ERROR_SUCCESS;
    }
    *lpdwBufferLength = requiredLength;

    PERF_LEAVE(QueryRawHeaders);

    return error;
}


DWORD
HTTP_HEADERS::QueryFilteredRawHeaders(
    IN LPSTR lpBase,
    IN LPSTR *lplpFilterList,
    IN DWORD cListElements,
    IN BOOL  fExclude,
    IN BOOL  fSkipVerb,
    IN BOOL bCrLfTerminated,
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns all the request or response headers in a single buffer. The headers
    can be returned as ASCIIZ strings, or CR-LF terminated strings

Arguments:

    lpBase              - base address of strings

    bCrLfTerminated     - TRUE if each string is terminated with CR-LF

    lpBuffer            - pointer to buffer to write headers

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: if successful, length of output buffer, minus 1
                               for any trailing EOS, or if the buffer is not
                               large enough, the size required

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER

--*/

{
    DWORD error = ERROR_NOT_SUPPORTED;

    DWORD requiredLength = 0;
    LPSTR lpszBuffer = (LPSTR)lpBuffer;
    BOOL fCopy;

    DWORD i = fSkipVerb ? 1 : 0;
    for (; i < _TotalSlots; ++i) {
       if (_lpHeaders[i].HaveString()) {
          fCopy = TRUE;
          if (lplpFilterList
             && FMatchList(lplpFilterList, cListElements, _lpHeaders+i, lpBase)) {
             fCopy = fExclude?FALSE:TRUE;
          }
          if (fCopy) {
              DWORD length;

              length = _lpHeaders[i].StringLength();
              requiredLength += length + (bCrLfTerminated ? 2 : 1);
              if (*lpdwBufferLength > requiredLength) {
                    _lpHeaders[i].CopyTo(lpBase, lpszBuffer);
                   lpszBuffer += length;
                   if (bCrLfTerminated) {
                       *lpszBuffer++ = '\r';
                       *lpszBuffer++ = '\n';
                    } else {
                       *lpszBuffer++ = '\0';
                   }
                }
            }
        }
    }

    if (bCrLfTerminated)
    {
        requiredLength += 2;
        if (*lpdwBufferLength > requiredLength)
        {
            *lpszBuffer++ = '\r';
            *lpszBuffer++ = '\n';
        }
    }

    ++requiredLength;


    if (*lpdwBufferLength < requiredLength) {
        error = ERROR_INSUFFICIENT_BUFFER;
    } else {
        *lpszBuffer = '\0';
        --requiredLength;   // remove 1 for trailing '\0'
        error = ERROR_SUCCESS;
    }
    *lpdwBufferLength = requiredLength;
    return error;
}


DWORD
HTTP_HEADERS::AddRequest(
    IN LPSTR lpszVerb,
    IN LPSTR lpszObject,
    IN LPSTR lpszVersion
    )

/*++

Routine Description:

    Builds the request line from its constituent parts. The request line is the
    first (0th) header in the request headers

    Assumes:    1. This is the one-and-only call to this method
                2. lpszObject must already be escaped if necessary

Arguments:

    lpszVerb    - pointer to HTTP verb, e.g. "GET"

    lpszObject  - pointer to HTTP object name, e.g. "/users/albert/~emc2.htm".

    lpszVersion - pointer to HTTP version string, e.g. "HTTP/1.0"

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    PERF_ENTER(AddRequest);

    //
    // there must not be a header when this method is called
    //

    INET_ASSERT(_HeadersLength == 0);

    DWORD error = ERROR_SUCCESS;
    int verbLen = lstrlen(lpszVerb);
    int objectLen = lstrlen(lpszObject);
    int versionLen = lstrlen(lpszVersion);
    int len = verbLen       // "GET"
            + 1             //     ' '
            + objectLen     //        "/users/albert/~emc2.htm"
            + 1             //                                 ' '
            + versionLen    //                                    "HTTP/1.0"
            + 1             //                                              '\0'
            ;

    //
    // we are about to start updating the headers for the current
    // HTTP_REQUEST_HANDLE_OBJECT. Serialize access
    //

    HEADER_STRING * pRequest = GetFirstHeader();
    HEADER_STRING & request = *pRequest;

    if (pRequest == NULL) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    INET_ASSERT(!request.HaveString());

    _lpszVerb = NULL;
    _dwVerbLength = 0;
    _lpszObjectName = NULL;
    _dwObjectNameLength = 0;
    _lpszVersion = NULL;
    _dwVersionLength = 0;

    request.CreateStringBuffer((LPVOID)lpszVerb, verbLen, len);
    if (request.IsError()) {
        error = GetLastError();

        INET_ASSERT(error != ERROR_SUCCESS);

    } else {
        request += ' ';
        request.Strncat((LPVOID)lpszObject, objectLen);
        request += ' ';
        request.Strncat((LPVOID)lpszVersion, versionLen);

        _HeadersLength = len - 1 + (sizeof("\r\n") - 1);

        //
        // we have used the first free slot in the headers array
        //

        --_FreeSlots;

        //
        // update the component variables in case of a ModifyRequest()
        //

        _lpszVerb = request.StringAddress();
        _dwVerbLength = verbLen;
        _lpszObjectName = _lpszVerb + verbLen + 1;
        _dwObjectNameLength = objectLen;
        _lpszVersion = _lpszObjectName + objectLen + 1;
        _dwVersionLength = versionLen;
        SetRequestVersion();
        error = request.IsError() ? ::GetLastError() : ERROR_SUCCESS;
    }

quit:

    PERF_LEAVE(AddRequest);

    return error;
}


DWORD
HTTP_HEADERS::ModifyRequest(
    IN HTTP_METHOD_TYPE tMethod,
    IN LPSTR lpszObjectName,
    IN DWORD dwObjectNameLength,
    IN LPSTR lpszVersion OPTIONAL,
    IN DWORD dwVersionLength
    )

/*++

Routine Description:

    Updates the request line. Used in redirection

Arguments:

    tMethod             - type of new method

    lpszObjectName      - pointer to new object name

    dwObjectNameLength  - length of new object name

    lpszVersion         - optional pointer to version string

    dwVersionLength     - length of lpszVersion string if present

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "ModifyRequest",
                 "%s, %q, %d, %q, %d",
                 MapHttpMethodType(tMethod),
                 lpszObjectName,
                 dwObjectNameLength,
                 lpszVersion,
                 dwVersionLength
                 ));

    PERF_ENTER(ModifyRequest);

    INET_ASSERT(lpszObjectName != NULL);
    INET_ASSERT(dwObjectNameLength != 0);

    //
    // there must already be a header when this method is called
    //

    INET_ASSERT(_HeadersLength != 0);

    //
    // we are about to start updating the headers for the current
    // HTTP_REQUEST_HANDLE_OBJECT. Serialize access
    //

    //
    // BUGBUG [arthurbi] using two HEADER_STRINGs here causes an extra
    //  ReAlloc when use the Copy operator between the two.
    //

    HEADER_STRING * pRequest = GetFirstHeader();
    HEADER_STRING & request = *pRequest;
    HEADER_STRING newRequest;
    LPCSTR lpcszVerb;
    DWORD verbLength;
    DWORD error = ERROR_SUCCESS;
    DWORD length;

    //
    // there must already be a request line
    //

    if (pRequest == NULL) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    INET_ASSERT(request.HaveString());

    //
    // get the verb/method to use.
    //

    if (tMethod == HTTP_METHOD_TYPE_UNKNOWN) {

        //
        // the method is unknown, read the old one out of the string
        //  and save off, basically we're reusing the previous one.
        //

        lpcszVerb = request.StringAddress();

        for (int i = 0; i < request.StringLength(); i++) {
            if (lpcszVerb[i] == ' ') {
                break;
            }
        }

        INET_ASSERT((i > 0) && (i < (int)request.StringLength()));

        verbLength = (DWORD)i;
    } else {

        //
        // its one of the normal kind, just map it.
        //

        verbLength = MapHttpMethodType(tMethod, &lpcszVerb);
    }
    if (lpszVersion == NULL) {
        lpszVersion = _lpszVersion;
        dwVersionLength = _dwVersionLength;
    }

    _lpszVerb = NULL;
    _dwVerbLength = 0;
    _lpszObjectName = NULL;
    _dwObjectNameLength = 0;
    _lpszVersion = NULL;
    _dwVersionLength = 0;

    //
    // calculate the new length from the component lengths we originally set
    // in AddRequest(), and the new object name
    //

    length = verbLength + 1 + dwObjectNameLength + 1 + dwVersionLength + 1;

    //
    // create a new request line
    //

    newRequest.CreateStringBuffer((LPVOID)lpcszVerb, verbLength, length);
    if (newRequest.IsError()) {
        error = GetLastError();
    } else {
        newRequest += ' ';
        newRequest.Strncat((LPVOID)lpszObjectName, dwObjectNameLength);
        newRequest += ' ';
        newRequest.Strncat((LPVOID)lpszVersion, dwVersionLength);

        //
        // remove the current request line length from the header buffer
        // aggregate
        //

        _HeadersLength -= request.StringLength();

        //
        // make the current request line the new one
        //

        request = newRequest.StringAddress();

        //
        // and update the address and length variables (version length is the
        // only thing that stays the same)
        //

        if (!request.IsError()) {
            _lpszVerb = request.StringAddress();
            _dwVerbLength = verbLength;
            _lpszObjectName = _lpszVerb + verbLength + 1;
            _dwObjectNameLength = dwObjectNameLength;
            _lpszVersion = _lpszObjectName + dwObjectNameLength + 1;
            _dwVersionLength = dwVersionLength;
            SetRequestVersion();

        //
        // and the new request line length to the aggregate header length
        //

            _HeadersLength += request.StringLength();
        } else {
            error = GetLastError();
        }
    }

quit:

    PERF_LEAVE(ModifyRequest);

    DEBUG_LEAVE(error);

    return error;
}


VOID
HTTP_HEADERS::SetRequestVersion(
    VOID
    )

/*++

Routine Description:

    Set _RequestVersionMajor and _RequestVersionMinor based on the HTTP
    version string

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_HEADERS::SetRequestVersion",
                 NULL
                 ));

    INET_ASSERT(_lpszVersion != NULL);

    _RequestVersionMajor = 0;
    _RequestVersionMinor = 0;
    if (strncmp(_lpszVersion, "HTTP/", sizeof("HTTP/") - 1) == 0) {

        LPSTR pNum = _lpszVersion + sizeof("HTTP/") - 1;

        ExtractInt(&pNum, 0, (LPINT)&_RequestVersionMajor);
        while (!isdigit(*pNum) && (*pNum != '\0')) {
            ++pNum;
        }
        ExtractInt(&pNum, 0, (LPINT)&_RequestVersionMinor);

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("request version = %d.%d\n",
                    _RequestVersionMajor,
                    _RequestVersionMinor
                    ));

    } else {

        DEBUG_PRINT(HTTP,
                    WARNING,
                    ("\"HTTP/\" not found in %q\n",
                    _lpszVersion
                    ));

    }

    DEBUG_LEAVE(0);
}


LPSTR
HTTP_REQUEST_HANDLE_OBJECT::CreateRequestBuffer(
    OUT LPDWORD lpdwRequestLength,
    IN LPVOID lpOptional,
    IN DWORD dwOptionalLength,
    IN BOOL bExtraCrLf,
    IN DWORD dwMaxPacketLength,
    OUT LPBOOL lpbCombinedData
    )

/*++

Routine Description:

    Creates a request buffer from the HTTP request and headers

Arguments:

    lpdwRequestLength   - pointer to returned buffer length

    lpOptional          - pointer to optional data

    dwOptionalLength    - length of optional data

    bExtraCrLf          - TRUE if we need to add additional CR-LF to buffer

    dwMaxPacketLength   - maximum length of buffer

    lpbCombinedData     - output TRUE if data successfully combined into one

Return Value:

    LPSTR
        Success - pointer to allocated buffer

        Failure - NULL

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Pointer,
                 "HTTP_REQUEST_HANDLE_OBJECT::CreateRequestBuffer",
                 "%#x, %#x, %d, %B, %d, %#x",
                 lpdwRequestLength,
                 lpOptional,
                 dwOptionalLength,
                 bExtraCrLf,
                 dwMaxPacketLength,
                 lpbCombinedData
                 ));

    PERF_ENTER(CreateRequestBuffer);

    *lpbCombinedData = FALSE;

    _RequestHeaders.LockHeaders();

    DWORD headersLength;
    DWORD requestLength;
    DWORD optionalLength;
    HEADER_STRING * pRequest = _RequestHeaders.GetFirstHeader();
    HEADER_STRING & request = *pRequest;
    LPSTR requestBuffer = NULL;
    WCHAR wszUrl[1024];
    LPWSTR pwszUrl = NULL;
    BYTE utf8Url[2048];
    LPBYTE pbUrl = NULL;
    LPSTR pszObject = _RequestHeaders.ObjectName();
    DWORD dwObjectLength = _RequestHeaders.ObjectNameLength();

    if (pRequest == NULL) {
        goto quit;
    }

    INET_ASSERT(request.HaveString());

    headersLength = _RequestHeaders.HeadersLength();
    requestLength = headersLength + (sizeof("\r\n") - 1);

/*------------------------------------------------------------------
    GlobalEnableUtf8Encoding = FALSE;
    if (GlobalEnableUtf8Encoding
        && StringContainsHighAnsi(pszObject, dwObjectLength)) {

        pwszUrl = wszUrl;

        DWORD arrayElements = ARRAY_ELEMENTS(wszUrl);

        if (dwObjectLength > ARRAY_ELEMENTS(wszUrl)) {
            arrayElements = dwObjectLength;
            pwszUrl = (LPWSTR)ALLOCATE_FIXED_MEMORY(arrayElements * sizeof(*pwszUrl));
            if (pwszUrl == NULL) {
                goto utf8_cleanup;
            }
        }



        PFNINETMULTIBYTETOUNICODE pfnMBToUnicode;
        pfnMBToUnicode = GetInetMultiByteToUnicode( );
        if (pfnMBToUnicode == NULL) {
            goto utf8_cleanup;
        }

        HRESULT hr;
        DWORD dwMode;
        INT nMBChars;
        INT nWChars;

        nMBChars = dwObjectLength;
        nWChars = arrayElements;
        dwMode = 0;

        hr = pfnMBToUnicode(&dwMode,
                                GetCodePage(),
                                pszObject,
                                &nMBChars,
                                pwszUrl,
                                &nWChars
                               );
        if (hr != S_OK || nWChars == 0) {
            goto utf8_cleanup;
        }

        DWORD nBytes;

        nBytes = CountUnicodeToUtf8(pwszUrl, (DWORD)nWChars, TRUE);
        pbUrl = utf8Url;
        if (nBytes > ARRAY_ELEMENTS(utf8Url)) {
            pbUrl = (LPBYTE)ALLOCATE_FIXED_MEMORY(nBytes);
            if (pbUrl == NULL) {
                goto utf8_cleanup;
            }
        }

        DWORD error;

        error = ConvertUnicodeToUtf8(pwszUrl,
                                     (DWORD)nWChars,
                                     pbUrl,
                                     nBytes,
                                     TRUE
                                     );

        INET_ASSERT(error == ERROR_SUCCESS);

        if (error != ERROR_SUCCESS) {
            goto utf8_cleanup;
        }

        requestLength = requestLength - dwObjectLength + nBytes;
        headersLength = headersLength - dwObjectLength + nBytes;
        pszObject = (LPSTR)pbUrl;
        dwObjectLength = nBytes;
        goto after_utf8;

utf8_cleanup:

        if ((pwszUrl != wszUrl) && (pwszUrl != NULL)) {
            FREE_MEMORY(pwszUrl);
        }
        pwszUrl = NULL;
        if ((pbUrl != utf8Url) && (pbUrl != NULL)) {
            FREE_MEMORY(pbUrl);
        }
        pbUrl = NULL;
        pszObject = NULL;
        dwObjectLength = 0;
    }

after_utf8:
------------------------------------------------------------------*/

    optionalLength = dwOptionalLength + (bExtraCrLf ? (sizeof("\r\n") - 1) : 0);
    if (requestLength + optionalLength <= dwMaxPacketLength) {
        requestLength += optionalLength;
    } else {
        optionalLength = 0;
        bExtraCrLf = FALSE;
    }

    requestBuffer = (LPSTR)ResizeBuffer(NULL, requestLength, FALSE);
    if (requestBuffer != NULL) {
        if (optionalLength != 0) {
            *lpbCombinedData = TRUE;
        }
    } else if (optionalLength != 0) {
        requestLength = headersLength + (sizeof("\r\n") - 1);
        optionalLength = 0;
        bExtraCrLf = FALSE;
        requestBuffer = (LPSTR)ResizeBuffer(NULL, requestLength, FALSE);
    }
    if (requestBuffer != NULL) {

        LPSTR buffer = requestBuffer;

        //
        // copy the headers. Remember: header 0 is the request
        //

//#ifdef COMPRESSED_HEADERS
//        if (vfCompressedHeaders) {
//            DEBUG_PRINT(HTTP,
//                        INFO,
//                        ("Compressing Headers")
//                        );
//            _RequestHeaders.CopyCompressedHeaders(&buffer);
//
//        }
//        else
//#endif //COMPRESSED_HEADERS
        {
            _RequestHeaders.CopyHeaders(&buffer, pszObject, dwObjectLength);
        }

        //
        // terminate the request
        //

        *buffer++ = '\r';
        *buffer++ = '\n';

//#ifdef COMPRESSED_HEADERS
//        if (vfCompressedHeaders) {
//
//            *lpdwRequestLength = ((DWORD)buffer - (DWORD)requestBuffer);
//
//            DEBUG_PRINT(HTTP,
//                        INFO,
//                        ("Compressed Headers: Old Length=%d, New Length = %d, Saved=%d\n",
//                        requestLength,
//                        *lpdwRequestLength,
//                        requestLength - *lpdwRequestLength
//                        )
//                        );
//        } else {
//#endif //COMPRESSED_HEADERS

        if (optionalLength != 0) {
            if (dwOptionalLength != 0) {
                memcpy(buffer, lpOptional, dwOptionalLength);
                buffer += dwOptionalLength;
            }
            if (bExtraCrLf) {
                *buffer++ = '\r';
                *buffer++ = '\n';
            }
        }

        INET_ASSERT((SIZE_T)(buffer-requestBuffer) == requestLength);

        *lpdwRequestLength = requestLength;

//#ifdef COMPRESSED_HEADERS
//        }
//#endif
    }

quit:

    _RequestHeaders.UnlockHeaders();

    DEBUG_PRINT(HTTP,
                INFO,
                ("request length = %d, combined = %B\n",
                *lpdwRequestLength,
                *lpbCombinedData
                ));

    if ((pbUrl != NULL) && (pbUrl != utf8Url)) {
        FREE_MEMORY(pbUrl);
    }
    if ((pwszUrl != NULL) && (pwszUrl != wszUrl)) {
        FREE_MEMORY(pwszUrl);
    }

    PERF_LEAVE(CreateRequestBuffer);

    DEBUG_LEAVE(requestBuffer);

    return requestBuffer;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryRequestHeader(
    IN LPSTR lpszHeaderName,
    IN DWORD dwHeaderNameLength,
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwModifiers,
    IN OUT LPDWORD lpdwIndex
    )

/*++

Routine Description:

    Searches for an arbitrary request header and if found, returns its value

Arguments:

    lpszHeaderName      - pointer to the name of the header to find

    dwHeaderNameLength  - length of the header

    lpBuffer            - pointer to buffer for results

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: length of the returned header value, or required
                               length of lpBuffer

    dwModifiers         - how to return the data: as number, as SYSTEMTIME
                          structure, etc.

    lpdwIndex           - IN: 0-based index of header to find
                          OUT: next header index if success returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    lpBuffer not large enough for results

                  ERROR_INTERNET_INCORRECT_FORMAT
                    Can't convert the data to the requested format

                  ERROR_HTTP_HEADER_NOT_FOUND
                    Couldn't find the requested header

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "QueryRequestHeader",
                 "%#x [%.*q], %d, %#x, %#x [%#x], %#x, %#x [%d]",
                 lpszHeaderName,
                 min(dwHeaderNameLength + 1, 80),
                 lpszHeaderName,
                 dwHeaderNameLength,
                 lpBuffer,
                 lpdwBufferLength,
                 *lpdwBufferLength,
                 dwModifiers,
                 lpdwIndex,
                 *lpdwIndex
                 ));

    PERF_ENTER(QueryRequestHeader);

    DWORD error;

    error = _RequestHeaders.FindHeader(NULL,
                                       lpszHeaderName,
                                       dwHeaderNameLength,
                                       dwModifiers,
                                       lpBuffer,
                                       lpdwBufferLength,
                                       lpdwIndex
                                       );

    PERF_LEAVE(QueryRequestHeader);

    DEBUG_LEAVE(error);

    return error;
}

DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryRequestHeader(
    IN DWORD dwQueryIndex,
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwModifiers,
    IN OUT LPDWORD lpdwIndex
    )

/*++

Routine Description:

    Searches for an arbitrary request header and if found, returns its value

Arguments:

    lpszHeaderName      - pointer to the name of the header to find

    dwHeaderNameLength  - length of the header

    lpBuffer            - pointer to buffer for results

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: length of the returned header value, or required
                               length of lpBuffer

    dwModifiers         - how to return the data: as number, as SYSTEMTIME
                          structure, etc.

    lpdwIndex           - IN: 0-based index of header to find
                          OUT: next header index if success returned

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER
                    lpBuffer not large enough for results

                  ERROR_INTERNET_INCORRECT_FORMAT
                    Can't convert the data to the requested format

                  ERROR_HTTP_HEADER_NOT_FOUND
                    Couldn't find the requested header

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "QueryRequestHeader",
                 "%u, %#x [%#x], %#x, %#x [%d]",
                 dwQueryIndex,
                 lpBuffer,
                 lpdwBufferLength,
                 *lpdwBufferLength,
                 dwModifiers,
                 lpdwIndex,
                 *lpdwIndex
                 ));

    PERF_ENTER(QueryRequestHeader);

    DWORD error;

    error = _RequestHeaders.FindHeader(NULL,
                                       dwQueryIndex,
                                       dwModifiers,
                                       lpBuffer,
                                       lpdwBufferLength,
                                       lpdwIndex
                                       );

    PERF_LEAVE(QueryRequestHeader);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_REQUEST_HANDLE_OBJECT::AddInternalResponseHeader(
    IN DWORD dwHeaderIndex,
    IN LPSTR lpszHeader,
    IN DWORD dwHeaderLength
    )

/*++

Routine Description:

    Adds a created response header to the response header array. Unlike normal
    response headers, this will be a pointer to an actual string, not an offset
    into the response buffer.

    Even if the address of the response buffer changes, created response headers
    will remain fixed

    N.B. The header MUST NOT have a CR-LF terminator
    N.B.-2 This function must be called under the header lock.

Arguments:

    dwHeaderIndex   - index into header value we are actually creating

    lpszHeader      - pointer to created (internal) header to add

    dwHeaderLength  - length of response header, or -1 if ASCIIZ

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "AddInternalResponseHeader",
                 "%u [%q], %q, %d",
                 dwHeaderIndex,
                 GlobalKnownHeaders[dwHeaderIndex].Text,
                 lpszHeader,
                 dwHeaderLength
                 ));

    DWORD error;

    if (dwHeaderLength == (DWORD)-1) {
        dwHeaderLength = lstrlen(lpszHeader);
    }

    INET_ASSERT((lpszHeader[dwHeaderLength - 1] != '\r')
                && (lpszHeader[dwHeaderLength - 1] != '\n'));

    //
    // find the next slot for this header
    //

    HEADER_STRING * freeHeader;

    //
    // if we already have all the headers (the 'empty' header is the last one
    // in the array) then change the last header to be the one we are adding
    // and add a new empty header, else just add this one
    //

    DWORD iSlot;
    freeHeader = _ResponseHeaders.FindFreeSlot(&iSlot);
    if (freeHeader == NULL) {
        error = _ResponseHeaders.GetError();

        INET_ASSERT(error != ERROR_SUCCESS);

    } else {

        HEADER_STRING * lastHeader;

        lastHeader = _ResponseHeaders.GetEmptyHeader();
        if (lastHeader != NULL) {

            //
            // make copy of last header - its an offset string
            //

            *freeHeader = *lastHeader;

            //
            // use what was last header as free header
            //

            freeHeader = lastHeader;
        }
        freeHeader->MakeCopy(lpszHeader, dwHeaderLength);
        freeHeader->SetNextKnownIndex(_ResponseHeaders.FastAdd(dwHeaderIndex, iSlot));
        error = ERROR_SUCCESS;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::UpdateResponseHeaders(
    IN OUT LPBOOL lpbEof
    )

/*++

Routine Description:

    Given the next chunk of the response, updates the response headers. The
    buffer pointer, buffer length and number of bytes received values are all
    maintained in this object (_ResponseBuffer, _ResponseBufferLength and
    _BytesReceived, resp.)

Arguments:

    lpbEof  - IN: TRUE if we have reached the end of the response
              OUT: TRUE if we have reached the end of the response or the end
                   of the headers

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::UpdateResponseHeaders",
                 "%#x [%.*q], %d, %d, %#x [%B]",
                 _ResponseBuffer + _ResponseScanned,
                 min(_ResponseBufferLength + 1, 80),
                 _ResponseBuffer + _ResponseScanned,
                 _ResponseBufferLength,
                 _BytesReceived,
                 lpbEof,
                 *lpbEof
                 ));

    PERF_ENTER(UpdateResponseHeaders);

    LPSTR lpszBuffer = (LPSTR)_ResponseBuffer + _ResponseScanned;
    DWORD dwBytesReceived = _BytesReceived - _ResponseScanned;
    DWORD error;
    BOOL  success = TRUE;
    HEADER_STRING * statusLine;

    //
    // if input EOF is set then the caller is telling us that the end of the
    // response has been reached at transport level (the server closed the
    // connectiion)
    //

    if (*lpbEof) {
        SetEof(TRUE);
    }

    //
    // lock down the response headers for the duration of this request. The only
    // way another thread is going to wait on this lock is if the reference on
    // the HTTP request object goes to zero, which *shouldn't* happen
    //

    _ResponseHeaders.LockHeaders();

    //
    // if we don't yet know whether we have a HTTP/1.0 (or greater) or HTTP/0.9
    // response yet, then try to find out.
    //
    // Only responses greater than HTTP/0.9 start with the "HTTP/#.#" string
    //

    if (!IsDownLevel() && !IsUpLevel()) {

        if ((dwBytesReceived < sizeof("Secure-HTTP/")) && !*lpbEof) {
            goto done;
        }

#define MAKE_VERSION_ENTRY(string)  string, sizeof(string) - 1

        static struct {
            LPSTR Version;
            DWORD Length;
        } KnownVersionsStrings[] = {
            MAKE_VERSION_ENTRY("HTTP/"),
            MAKE_VERSION_ENTRY("S-HTTP/"),
            MAKE_VERSION_ENTRY("SHTTP/"),
            MAKE_VERSION_ENTRY("Secure-HTTP/"),

            //
            // allow for servers generating slightly off-the-wall responses
            //

            MAKE_VERSION_ENTRY("HTTP ")
        };

#define NUM_HTTP_VERSIONS   ARRAY_ELEMENTS(KnownVersionsStrings)

        //
        // We know this is the start of a HTTP response, but there may be some
        // noise at the start from bad HTML authoring, or bad content-length on
        // the previous response on a keep-alive connection. We will try to sync
        // up to the HTTP header (we will only look for this - I have never seen
        // any of the others, and I doubt its worth the increased complexity and
        // processing time)
        //

        LPSTR lpszBuf;
        DWORD bytesLeft;
        BOOL bFoundStart;

        lpszBuf = lpszBuffer;
        bytesLeft = dwBytesReceived;
        bFoundStart = FALSE;

        do {
            while ((bytesLeft > 0) && (*lpszBuf != 'H') && (*lpszBuf != 'h')) {
                ++lpszBuf;
                --bytesLeft;
                ++_ResponseScanned;
            }
            if (bytesLeft == 0) {
                break;
            }

            //
            // scan for the known version strings
            //

            for (int i = 0; i < NUM_HTTP_VERSIONS; ++i) {

                LPSTR version = KnownVersionsStrings[i].Version;
                DWORD length = KnownVersionsStrings[i].Length;

                if ((bytesLeft >= length)

                //
                // try the most common case as a direct comparison. memcmp()
                // should expand to cmpsd && cmpsb on x86 (most common platform
                // and one on which we are most interested in improving perf)
                //

                && (((i == 0)
                    && (memcmp(lpszBuf, "HTTP/", sizeof("HTTP/") - 1) == 0))
                    //&& (lpszBuf[0] == 'H')
                    //&& (lpszBuf[1] == 'T')
                    //&& (lpszBuf[2] == 'T')
                    //&& (lpszBuf[3] == 'P')
                    //&& (lpszBuf[4] == '/'))

                //
                //  "Clients should be tolerant in parsing the Status-Line"
                //  quote from HTTP/1.1 spec, therefore we perform a
                //  case-insensitive string comparison here
                //

                || (_strnicmp(lpszBuf, version, length) == 0))) {

                    //
                    // it starts with one of the recognized protocol version strings.
                    // We assume its not a down-level server, although it could be,
                    // sending back a plain text document that has e.g. "HTTP/1.0..."
                    // at its start
                    //
                    // According to the HTTP "spec", though, it is mentioned that 0.9
                    // servers typically only return HTML, hence we shouldn't see
                    // even a 0.9 response start with non-HTML data
                    //

                    SetUpLevel(TRUE);

                    //
                    // this response has headers
                    //

                    SetNoHeaders(FALSE);

                    //
                    // we have start of this response
                    //

                    lpszBuffer = lpszBuf;
                    bFoundStart = TRUE;
                    break;
                }
            }

            //
            // if we didn't find the start of the HTTP response then search again
            //

            if (!bFoundStart) {
                ++lpszBuf;
                --bytesLeft;
                ++_ResponseScanned;
            }
        } while (!bFoundStart && (bytesLeft > 0));

        //
        // if we didn't find a recognizable HTTP 1.x response then we assume its
        // a down-level response
        //

        if (!IsUpLevel()) {

            //
            // if we didn't find the start of a valid HTTP response and we have
            // not filled the response buffer or hit the end of the connection
            // then quit so we can get the next packet
            //

            if ((_BytesReceived < _ResponseBufferLength) && !IsEof()) {

                DEBUG_PRINT(HTTP,
                            WARNING,
                            ("Didn't find start of response. Try again\n"
                            ));

//dprintf("*** didn't find start of response. Try again\n");
                goto done;
            }

            //
            // this may be a real down-level server, or it may be the response
            // from an FTP or gopher server via a proxy, in which case there
            // will be no headers. We will add some default headers to make
            // life easier for higher level software
            //

            AddInternalResponseHeader(HTTP_QUERY_STATUS_TEXT, // use non-standard index, since we never query this normally
                                      "HTTP/1.0 200 OK",
                                      sizeof("HTTP/1.0 200 OK") - 1
                                      );
            _StatusCode = HTTP_STATUS_OK;
            //SetDownLevel(TRUE);

            //
            // we're now ready for the app to start reading data out
            //

            SetData(TRUE);

            //
            // down-level server: we're done
            //

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("Server is down-level\n"
                        ));

            goto done;
        }
    }

    //
    // this stuff's only for uplevel responses, sorry
    //

    INET_ASSERT(IsUpLevel());

    //
    // Note: at this point we can't store pointers into the response buffer
    // because it might move during a subsequent reallocation. We have to
    // maintain offsets into the buffer and convert to pointers when we come to
    // read the data out of the buffer (when the response is complete, or at
    // least we've finished receiving headers)
    //

    //
    // if we haven't checked the response yet, then the first thing to
    // get is the status line
    //

    statusLine = GetStatusLine();

    if (statusLine == NULL) {
        error = ERROR_NOT_ENOUGH_MEMORY;
        goto quit;
    }

    if (!statusLine->HaveString())
    {
        int majorVersion = 0;
        int minorVersion = 0;
        BOOL fSupportsHttp1_1;

        _StatusCode = 0;

        //
        // find the status line
        //

        success = _ResponseHeaders.ParseStatusLine(
            (LPSTR)_ResponseBuffer,
            _BytesReceived,
            IsEof(),
            &_ResponseScanned,
            &_StatusCode,
            (LPDWORD)&majorVersion,
            (LPDWORD)&minorVersion
            );

        if ( !success )
        {
            error = ERROR_SUCCESS;
            goto quit;
        }


        DEBUG_PRINT(HTTP,
                    INFO,
                    ("Version = %d.%d\n",
                    majorVersion,
                    minorVersion
                    ));

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("_StatusCode = %d\n",
                    _StatusCode
                    ));

        fSupportsHttp1_1 = FALSE;

        if ( majorVersion > 1 )
        {
            //
            // for higher version servers, the 1.1 spec dictates
            //  that we return the highest version the client
            //  supports, and in our case that is 1.1.
            //

            fSupportsHttp1_1 = TRUE;
        }
        else if ( majorVersion == 1 )
        {
            if ( minorVersion >= 1 )
            {
                fSupportsHttp1_1 = TRUE;
            }
        } else if ((majorVersion < 0) || (minorVersion < 0)) {
            error = ERROR_HTTP_INVALID_SERVER_RESPONSE;
            goto quit;
        }

        SetResponseHttp1_1(fSupportsHttp1_1);

        //
        // record the server HTTP version in the server info object
        //

        CServerInfo * pServerInfo = GetServerInfo();

        if (pServerInfo != NULL) {
            if (fSupportsHttp1_1) {
                pServerInfo->SetHttp1_1();
            } else {
                pServerInfo->SetHttp1_0();

                //
                // up the connection limit from HTTP 1.1 (default 2) to
                // HTTP 1.0 (default 4)
                //

                pServerInfo->SetNewLimit(GlobalMaxConnectionsPer1_0Server);
            }
        }

        if (_StatusCode == 0) {

            //
            // BUGBUG [arthurbi] malformed header, should we really just accept it?
            //      what if we get indeterminate garbage?
            //


            INET_ASSERT(FALSE);

            AddInternalResponseHeader(HTTP_QUERY_STATUS_TEXT, // use non-standard index, since we never query this normally
                          "HTTP/1.0 200 OK",
                          sizeof("HTTP/1.0 200 OK") - 1
                          );
            _StatusCode = HTTP_STATUS_OK;
            error = ERROR_SUCCESS;

            goto quit;
        }
    }

    //
    // continue scanning headers here until we have tested all the current
    // buffer, or we have found the start of the data
    //

    BOOL fFoundEndOfHeaders;

    error = _ResponseHeaders.ParseHeaders(
                (LPSTR)_ResponseBuffer,
                _BytesReceived,
                IsEof(),
                &_ResponseScanned,
                &success,
                &fFoundEndOfHeaders
                );

    if ( error != ERROR_SUCCESS )
    {
        goto quit;
    }


    if ( fFoundEndOfHeaders )
    {
        //
        // we found the end of the headers
        //

        SetEof(TRUE);

        //
        // and the start of the data
        //

        SetData(TRUE);
        _DataOffset = _ResponseScanned;

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("found end of headers. _DataOffset = %d\n",
                    _DataOffset
                    ));

    }

done:

    //
    // if we have reached the end of the headers then we communicate this fact
    // to the caller
    //

    if (IsData() || IsEof()) {
        CheckWellKnownHeaders();
        *lpbEof = TRUE;

        /*

        Set connection persistency based on these rules:

        persistent = (1.0Request && Con: K-A && 1.0Response && Con: K-A)
                     || (1.1Request && Con: K-A && 1.0Response && Con: K-A)
                     || (1.0Request && Con: K-A && 1.1Response && Con: K-A)
                     || (1.1Request && !Con: Close && 1.1Response && !Con: Close)

        therefore,

        persistent = 1.1Request && 1.1Response
                        ? (!Con: Close in request || response)
                        : Con: K-A in request && response

        */

        if (IsRequestHttp1_1() && IsResponseHttp1_1()) {

            BOOL bHaveConnCloseRequest;

            bHaveConnCloseRequest = FindConnCloseRequestHeader(
                                        IsRequestUsingProxy()
                                            ? HTTP_QUERY_PROXY_CONNECTION
                                            : HTTP_QUERY_CONNECTION
                                            );
            if (!(IsConnCloseResponse() || bHaveConnCloseRequest)) {

                DEBUG_PRINT(HTTP,
                            INFO,
                            ("HTTP/1.1 persistent connection\n"
                            ));

                SetKeepAlive(TRUE);
                SetPersistentConnection(IsRequestUsingProxy()
                                        && !IsTalkingToSecureServerViaProxy()
                                        );
            } else {

                DEBUG_PRINT(HTTP,
                            INFO,
                            ("HTTP/1.1 non-persistent connection: close on: request: %B; response: %B\n",
                            bHaveConnCloseRequest,
                            IsConnCloseResponse()
                            ));

                SetKeepAlive(FALSE);
                SetNoLongerKeepAlive();
                ClearPersistentConnection();
            }
        }
    }

    error = ERROR_SUCCESS;

quit:

    //
    // we are finished updating the response headers (no other thread should be
    // waiting for this if the reference count and object state is correct)
    //

    _ResponseHeaders.UnlockHeaders();

    PERF_LEAVE(UpdateResponseHeaders);

    DEBUG_LEAVE(error);

    return error;
}



DWORD
HTTP_REQUEST_HANDLE_OBJECT::CreateResponseHeaders(
    IN OUT LPSTR* ppszBuffer,
    IN DWORD      dwBufferLength
    )

/*++

Routine Description:

    Create the response headers given a buffer containing concatenated headers.
    Called when we are creating this object from the cache

Arguments:

    lpszBuffer      - pointer to buffer containing headers

    dwBufferLength  - length of lpszBuffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY
                    Couldn't create headers

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "HTTP_REQUEST_HANDLE_OBJECT::CreateResponseHeaders",
                 "%.32q, %d",
                 ppszBuffer,
                 dwBufferLength
                 ));

    //
    // there SHOULD NOT already be a response buffer if we're adding an
    // external buffer
    //

    INET_ASSERT(_ResponseBuffer == NULL);

    DWORD error;
    BOOL eof = FALSE;

    _ResponseBuffer = (LPBYTE) *ppszBuffer;
    _ResponseBufferLength = dwBufferLength;
    _BytesReceived = dwBufferLength;
    error = UpdateResponseHeaders(&eof);
    if (error != ERROR_SUCCESS) {

        //
        // if we failed, we will clean up our variables including clearing
        // out the response buffer address and length, but leave freeing
        // the buffer to the caller
        //

        _ResponseBuffer = NULL;
        _ResponseBufferLength = 0;
        ResetResponseVariables();

    } else {

        //
        // Success - the object owns the buffer so the caller should not free.
        //

        *ppszBuffer = NULL;
    }

    DEBUG_LEAVE(error);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryResponseVersion(
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns the HTTP version string from the status line

Arguments:

    lpBuffer            - pointer to buffer to copy version string into

    lpdwBufferLength    - IN: size of lpBuffer
                          OUT: size of version string excluding terminating '\0'
                               if successful, else required buffer length

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER

--*/

{
    PERF_ENTER(QueryResponseVersion);

    DWORD error;

    HEADER_STRING * statusLine = GetStatusLine();

    if ((statusLine == NULL) || statusLine->IsError()) {
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    LPSTR string;
    DWORD length;

    //
    // get a pointer into the response buffer where the status line starts
    // and its length
    //

    string = statusLine->StringAddress((LPSTR)_ResponseBuffer);
    length = (DWORD)statusLine->StringLength();

    //
    // the version string is the first token on the line, delimited by spaces
    //

    DWORD index;

    for (index = 0; index < length; ++index) {

        //
        // we'll also check for CR and LF, although just space should be
        // sufficient
        //

        if ((string[index] == ' ')
        || (string[index] == '\r')
        || (string[index] == '\n')) {
            break;
        }
    }
    if (*lpdwBufferLength > index) {
        memcpy(lpBuffer, (LPVOID)string, index);
        ((LPSTR)lpBuffer)[index] = '\0';
        *lpdwBufferLength = index;
        error = ERROR_SUCCESS;
    } else {
        *lpdwBufferLength = index + 1;
        error = ERROR_INSUFFICIENT_BUFFER;
    }

quit:

    PERF_LEAVE(QueryResponseVersion);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryStatusCode(
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength,
    IN DWORD dwModifiers
    )

/*++

Routine Description:

    Returns the status code as a string or a number

Arguments:

    lpBuffer            - pointer to buffer where results written

    lpdwBufferLength    - IN: length of buffer
                          OUT: size of returned information, or required size'
                               of buffer

    dwModifiers         - flags which modify returned value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER

--*/

{
    PERF_ENTER(QueryStatusCode);

    DWORD error;
    DWORD requiredSize;

    if (dwModifiers & HTTP_QUERY_FLAG_NUMBER) {
        requiredSize = sizeof(_StatusCode);
        if (*lpdwBufferLength >= requiredSize) {
            *(LPDWORD)lpBuffer = _StatusCode;
            error = ERROR_SUCCESS;
        } else {
            error = ERROR_INSUFFICIENT_BUFFER;
        }
    } else {

        //
        // the number should always be only 3 characters long, but we'll be
        // flexible (just in case)
        //

        char numBuf[sizeof("4294967296")];

        requiredSize = wsprintf(numBuf, "%u", _StatusCode) + 1;

#ifdef DEBUG
        // Debug check to make sure everything is good because the above
        // used to be ultoa.
        char debugBuf[sizeof("4294967296")];
        ultoa(_StatusCode, debugBuf, 10);
        if (strcmp(debugBuf,numBuf))
        {
            INET_ASSERT(FALSE);
        }

        INET_ASSERT(requiredSize == lstrlen(numBuf) + 1);
#endif

        if (*lpdwBufferLength >= requiredSize) {
            memcpy(lpBuffer, (LPVOID)numBuf, requiredSize);
            *lpdwBufferLength = requiredSize - 1;
            error = ERROR_SUCCESS;
        } else {
            *lpdwBufferLength = requiredSize;
            error = ERROR_INSUFFICIENT_BUFFER;
        }
    }

    PERF_LEAVE(QueryStatusCode);

    return error;
}


DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryStatusText(
    IN LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Returns the status text - if any - returned by the server in the status line

Arguments:

    lpBuffer            - pointer to buffer where status text is written

    lpdwBufferLength    - IN: size of lpBuffer
                          OUT: length of the status text string minus 1 for the
                               '\0', or the required buffer length if we return
                               ERROR_INSUFFICIENT_BUFFER

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_INSUFFICIENT_BUFFER

--*/

{
    PERF_ENTER(QueryStatusText);

    DWORD error;

    HEADER_STRING * statusLine = GetStatusLine();

    if ((statusLine == NULL) || statusLine->IsError()) {
        error = ERROR_INTERNET_INTERNAL_ERROR;
        goto quit;
    }

    LPSTR str;
    DWORD len;

    //
    // find the third token on the status line. The status line has the form
    //
    //  "HTTP/1.0 302 Try again, sucker\r\n"
    //
    //   ^        ^   ^
    //   |        |   |
    //   |        |   +- status text
    //   |        +- status code
    //   +- version
    //

    str = statusLine->StringAddress((LPSTR)_ResponseBuffer);
    len = statusLine->StringLength();

    DWORD i;

    i = 0;

    int j;

    for (j = 0; j < 2; ++j) {
        while ((i < len) && (str[i] != ' ')) {
            ++i;
        }
        while ((i < len) && (str[i] == ' ')) {
            ++i;
        }
    }
    len -= i;
    if (*lpdwBufferLength > len) {
        memcpy(lpBuffer, (LPVOID)&str[i], len);
        ((LPSTR)lpBuffer)[len] = '\0';
        *lpdwBufferLength = len;
        error = ERROR_SUCCESS;
    } else {
        *lpdwBufferLength = len + 1;
        error = ERROR_INSUFFICIENT_BUFFER;
    }

quit:

    PERF_LEAVE(QueryStatusText);

    return error;
}



DWORD
HTTP_REQUEST_HANDLE_OBJECT::QueryRawResponseHeaders(
    IN BOOL bCrLfTerminated,
    OUT LPVOID lpBuffer,
    IN OUT LPDWORD lpdwBufferLength
    )

/*++

Routine Description:

    Gets the raw response headers

Arguments:

    bCrLfTerminated     - TRUE if we want RAW_HEADERS_CRLF else RAW_HEADERS

    lpBuffer            - pointer to buffer where headers returned

    lpdwBufferLength    - IN: length of lpBuffer
                          OUT: returned length of lpBuffer

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 Dword,
                 "QueryRawHeaders",
                 "%B, %#x, %#x [%d]",
                 bCrLfTerminated,
                 lpBuffer,
                 lpdwBufferLength,
                 *lpdwBufferLength
                 ));

    PERF_ENTER(QueryRawHeaders);

    DWORD error = _ResponseHeaders.QueryRawHeaders(
                    (LPSTR)_ResponseBuffer,
                    bCrLfTerminated,
                    lpBuffer,
                    lpdwBufferLength
                    );

    IF_DEBUG_CODE() {
        if (error == ERROR_INSUFFICIENT_BUFFER) {

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("*lpdwBufferLength = %d\n",
                        *lpdwBufferLength
                        ));

        }
    }

    PERF_LEAVE(QueryRawHeaders);

    DEBUG_LEAVE(error);

    return error;

}


VOID
HTTP_REQUEST_HANDLE_OBJECT::RemoveAllRequestHeadersByName(
    IN DWORD dwQueryIndex
    )

/*++

Routine Description:

    Removes all headers of a particular type from the request object

Arguments:

    lpszHeaderName  - name of header to remove

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "RemoveAllRequestHeadersByName",
                 "%q, %u",
                 GlobalKnownHeaders[dwQueryIndex].Text,
                 dwQueryIndex
                 ));

    PERF_ENTER(RemoveAllRequestHeadersByName);

    _RequestHeaders.RemoveAllByIndex(dwQueryIndex);

    PERF_LEAVE(RemoveAllRequestHeadersByName);

    DEBUG_LEAVE(0);
}

//
// private methods
//


PRIVATE
VOID
HTTP_REQUEST_HANDLE_OBJECT::CheckWellKnownHeaders(
    VOID
    )

/*++

Routine Description:

    Tests for a couple of well-known headers that are important to us as well as
    the app:

        "Connection: Keep-Alive"
        "Proxy-Connection: Keep-Alive"
        "Connection: Close"
        "Proxy-Connection: Close"
        "Transfer-Encoding: chunked"
        "Content-Length: ####"
        "Content-Range: bytes ####-####/####"

    The header DOES NOT contain CR-LF. That is, dwHeaderLength will not include
    any counts for line termination

    We need to know if the server honoured a request for a keep-alive connection
    so that we don't try to receive until we hit the end of the connection. The
    server will keep it open.

    We need to know the content length if we are talking over a persistent (keep
    alive) connection.

    If either header is found, we set the corresponding flag in the HTTP_HEADERS
    object, and in the case of "Content-Length:" we parse out the length.

Arguments:

    None.

Return Value:

    None.

--*/

{
    DEBUG_ENTER((DBG_HTTP,
                 None,
                 "HTTP_REQUEST_HANDLE_OBJECT::CheckWellKnownHeaders",
                 NULL
                 ));

    //
    // check for "Content-Length:" and "Content-Range"
    //

    if ( IsResponseHeaderPresent(HTTP_QUERY_CONTENT_RANGE) )
    {
        _iSlotContentRange = _ResponseHeaders._bKnownHeaders[HTTP_QUERY_CONTENT_RANGE];
    }

    if ( IsResponseHeaderPresent(HTTP_QUERY_CONTENT_LENGTH) )
    {
        HEADER_STRING * curHeader;
        DWORD dwHeaderLength;
        LPSTR lpszHeader;

        _iSlotContentLength = _ResponseHeaders._bKnownHeaders[HTTP_QUERY_CONTENT_LENGTH];
        curHeader = _ResponseHeaders.GetSlot(_iSlotContentLength);

        lpszHeader     = curHeader->StringAddress((LPSTR)_ResponseBuffer);
        dwHeaderLength = curHeader->StringLength();

        dwHeaderLength -= GlobalKnownHeaders[HTTP_QUERY_CONTENT_LENGTH].Length+1;
        lpszHeader     += GlobalKnownHeaders[HTTP_QUERY_CONTENT_LENGTH].Length+1;

        while (dwHeaderLength && (*lpszHeader == ' ')) {
            --dwHeaderLength;
            ++lpszHeader;
        }
        while (dwHeaderLength && isdigit(*lpszHeader)) {
            _ContentLength = _ContentLength * 10 + (*lpszHeader - '0');
            --dwHeaderLength;
            ++lpszHeader;
        }

        //
        // once we have _ContentLength, we don't modify it (unless
        // we fix it up when using a 206 partial response to resume
        // a partial download.)  The header value should be returned
        // by HttpQueryInfo().  Instead, we keep account of the
        // amount of keep-alive data left to copy in _BytesRemaining
        //

        _BytesRemaining = _ContentLength;

        //
        // although we said we may be one past the end of the header, in
        // reality, if we received a buffer with "Content-Length:" then we
        // expect it to be terminated by CR-LF (or CR-CR-LF or just LF,
        // depending on the wackiness quotient of the server)
        //

        INET_ASSERT((*lpszHeader == '\r') || (*lpszHeader == '\n'));

        SetHaveContentLength(TRUE);

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("_ContentLength = %d\n",
                    _ContentLength
                    ));

        _BytesInSocket = (_ContentLength != 0)
                ? (_ContentLength - (_BytesReceived - _DataOffset))
                : 0;

        //
        // we could have multiple responses in the same buffer. If
        // the amount received is greater than the content length
        // then we have all the data; there are no bytes left in
        // the socket for the current response
        //

        if ((int)_BytesInSocket < 0) {
            _BytesInSocket = 0;
        }

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("bytes left in socket = %d\n",
                    _BytesInSocket
                    ));

    }


    if ( IsResponseHeaderPresent(HTTP_QUERY_CONNECTION) ||
         IsResponseHeaderPresent(HTTP_QUERY_PROXY_CONNECTION) )
    {
        //
        // check for "Connection: Keep-Alive" or "Proxy-Connection: Keep-Alive".
        // This test protects us against the unlikely
        // event of a server returning to us a keep-alive response header (because
        // that would cause problems for the proxy)
        //

        if (IsWantKeepAlive() && (!IsKeepAlive() || IsResponseHttp1_1()))
        {
            HEADER_STRING * curHeader;
            DWORD dwHeaderLength, headerNameLength;
            LPSTR lpszHeader;


            DWORD iSlot;

            char ch;

            if (IsRequestUsingProxy() &&
                IsResponseHeaderPresent(HTTP_QUERY_PROXY_CONNECTION))
            {
                iSlot = _ResponseHeaders._bKnownHeaders[HTTP_QUERY_PROXY_CONNECTION];
                headerNameLength = GlobalKnownHeaders[HTTP_QUERY_PROXY_CONNECTION].Length+1;
            }
            else if (IsResponseHeaderPresent(HTTP_QUERY_CONNECTION))
            {
                iSlot = _ResponseHeaders._bKnownHeaders[HTTP_QUERY_CONNECTION];
                headerNameLength = GlobalKnownHeaders[HTTP_QUERY_CONNECTION].Length+1;
            }
            else
            {
                iSlot = _ResponseHeaders._bKnownHeaders[HTTP_QUERY_PROXY_CONNECTION];
                headerNameLength = GlobalKnownHeaders[HTTP_QUERY_PROXY_CONNECTION].Length+1;
                INET_ASSERT(FALSE);
            }

            curHeader      = _ResponseHeaders.GetSlot(iSlot);
            lpszHeader     = curHeader->StringAddress((LPSTR)_ResponseBuffer);
            dwHeaderLength = curHeader->StringLength();

            dwHeaderLength -= headerNameLength;
            lpszHeader     += headerNameLength;

            while (dwHeaderLength && (*lpszHeader == ' ')) {
                ++lpszHeader;
                --dwHeaderLength;
            }

            //
            // both headers use "Keep-Alive" as header-value ONLY for HTTP 1.0 servers
            //

            if (((int)dwHeaderLength >= KEEP_ALIVE_LEN)
            && !strnicmp(lpszHeader, KEEP_ALIVE_SZ, KEEP_ALIVE_LEN)) {

                DEBUG_PRINT(HTTP,
                            INFO,
                            ("Connection: Keep-Alive\n"
                            ));

                //
                // BUGBUG - we are setting k-a when coming from cache!
                //

                SetKeepAlive(TRUE);
                SetPersistentConnection(headerNameLength == HTTP_PROXY_CONNECTION_LEN);
            }

            //
            // also check for "Close" as header-value ONLY for HTTP 1.1 servers
            //

            else if ((*lpszHeader == 'C' || *lpszHeader == 'c')
                     && ((int)dwHeaderLength >= CLOSE_LEN)
                     && IsResponseHttp1_1()
                     && !strnicmp(lpszHeader, CLOSE_SZ, CLOSE_LEN)) {

                DEBUG_PRINT(HTTP,
                            INFO,
                            ("Connection: Close (HTTP/1.1)\n"
                            ));

                SetConnCloseResponse(TRUE);
            }
        }
    }

    //
    // check for "Refresh"
    //

    if ( IsResponseHeaderPresent(HTTP_QUERY_REFRESH)
        && !IsRefresh() )
    {

        DEBUG_PRINT(HTTP,
                    INFO,
                    ("have \"Refresh:\" header\n"
                    ));

        SetRefresh(TRUE);
    }


    //
    // check for "Transfer-Encoding:"
    //

    if (IsResponseHeaderPresent(HTTP_QUERY_TRANSFER_ENCODING) &&
        IsResponseHttp1_1())
    {

        //
        // If Http 1.1, check for Chunked Transfer
        //

        HEADER_STRING * curHeader;
        DWORD dwHeaderLength;
        LPSTR lpszHeader;
        DWORD iSlot;

        iSlot = _ResponseHeaders._bKnownHeaders[HTTP_QUERY_TRANSFER_ENCODING];
        curHeader = _ResponseHeaders.GetSlot(iSlot);

        lpszHeader     = curHeader->StringAddress((LPSTR)_ResponseBuffer);
        dwHeaderLength = curHeader->StringLength();

        dwHeaderLength -= GlobalKnownHeaders[HTTP_QUERY_TRANSFER_ENCODING].Length+1;
        lpszHeader     += GlobalKnownHeaders[HTTP_QUERY_TRANSFER_ENCODING].Length+1;

        while (dwHeaderLength && (*lpszHeader == ' ')) {
            ++lpszHeader;
            --dwHeaderLength;
        }

        //
        // look for "chunked" entry that confirms that we're doing chunked transfer encoding
        //

        if (((int)dwHeaderLength >= CHUNKED_LEN)
        && !strnicmp(lpszHeader, CHUNKED_SZ, CHUNKED_LEN))
        {

            SetHaveChunkEncoding(TRUE);

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("server is sending Chunked Transfer Encoding\n"
                        ));

            //
            // if both "transfer-encoding: chunked" and "content-length:"
            // were received then the chunking takes precedence
            //

            INET_ASSERT(!(IsChunkEncoding() && IsContentLength()));

            if (IsContentLength()) {
                SetHaveContentLength(FALSE);
            }

        }
    }

    SetBadNSServer(FALSE);

    if (IsResponseHttp1_1())
    {

        //
        // For IIS 4.0 Servers, and all other normal servers, if we make
        //  a HEAD request, we should ignore the Content-Length.
        //
        // IIS 3.0 servers send an illegal body, and this is a bug in the server.
        //  since they're not HTTP 1.1 we should be ok here.
        //

        if ( (GetMethodType() == HTTP_METHOD_TYPE_HEAD) &&
             (_ContentLength > 0) &&
             IsWantKeepAlive()
             )
        {

            //
            // set length to 0
            //

            _ContentLength = 0;

        }

        if ( IsRequestHttp1_1() )
        {


            //
            // check for NS servers that don't return correct HTTP/1.1 responses
            //

            LPSTR buffer;
            DWORD buflen;
            DWORD status = FastQueryResponseHeader(HTTP_QUERY_SERVER,
                                                   (LPVOID*)&buffer,
                                                   &buflen,
                                                   0
                                                   );

    #define NSEP    "Netscape-Enterprise/"
    #define NSEPLEN (sizeof(NSEP) - 1)
    #define NSFT    "Netscape-FastTrack/"
    #define NSFTLEN (sizeof(NSFT) - 1)
    #define NSCS    "Netscape-Commerce/"
    #define NSCSLEN (sizeof(NSCS) - 1)

            if (status == ERROR_SUCCESS) {

                BOOL fIsBadServer = ((buflen > NSEPLEN) && !strnicmp(buffer, NSEP, NSEPLEN))
                                 || ((buflen > NSFTLEN) && !strnicmp(buffer, NSFT, NSFTLEN))
                                 || ((buflen > NSCSLEN) && !strnicmp(buffer, NSCS, NSCSLEN));

                if ( fIsBadServer )
                {
                    CServerInfo * pServerInfo = GetServerInfo();

                    SetBadNSServer(fIsBadServer);

                    if (pServerInfo != NULL)
                    {
                        //
                        // Note this Bad Server info in the server info obj,
                        //   as we they fail to do keep-alive with SSL properly
                        //

                        pServerInfo->SetBadNSServer();
                    }


                    DEBUG_PRINT(HTTP,
                                INFO,
                                ("IsBadNSServer() == %B\n",
                                IsBadNSServer()
                                ));
                }
            }
        }

        //
        // BUGBUG - content-type: multipart/byteranges means we
        //          also have data
        //

        DWORD statusCode = GetStatusCode();

        if (!IsBadNSServer()
            && !IsContentLength()
            && !IsChunkEncoding()
            && (((statusCode >= HTTP_STATUS_CONTINUE)               // 100
                && (statusCode < HTTP_STATUS_OK))                   // 200
                || (statusCode == HTTP_STATUS_NO_CONTENT)           // 204
                || (statusCode == HTTP_STATUS_MOVED)                // 301
                || (statusCode == HTTP_STATUS_REDIRECT)             // 302
                || (statusCode == HTTP_STATUS_REDIRECT_METHOD)      // 303
                || (statusCode == HTTP_STATUS_NOT_MODIFIED)         // 304
                || (statusCode == HTTP_STATUS_REDIRECT_KEEP_VERB))  // 307
            || (GetMethodType() == HTTP_METHOD_TYPE_HEAD)) {

            DEBUG_PRINT(HTTP,
                        INFO,
                        ("header-only HTTP/1.1 response\n"
                        ));

            SetData(FALSE);
        }
    }

    DEBUG_LEAVE(0);
}


//
// this array has the same order as the HTTP_METHOD_TYPE enum
//

#define MAKE_REQUEST_METHOD_TYPE(Type) \
    sizeof(# Type) - 1, # Type, HTTP_METHOD_TYPE_ ## Type

//
// darrenmi - need a new macro because *_M-POST isn't a valid enum member.
// we need a seperate enum type and string value.
//
// map HTTP_METHOD_TYPE_MPOST <=> "M-POST"
//

#define MAKE_REQUEST_METHOD_TYPE2(EnumType,Type) \
    sizeof(# Type) - 1, # Type, HTTP_METHOD_TYPE_ ## EnumType

static const struct _REQUEST_METHOD {
    int Length;
    LPSTR Name;
    HTTP_METHOD_TYPE MethodType;
} MethodNames[] = {
    MAKE_REQUEST_METHOD_TYPE(GET),
    MAKE_REQUEST_METHOD_TYPE(HEAD),
    MAKE_REQUEST_METHOD_TYPE(POST),
    MAKE_REQUEST_METHOD_TYPE(PUT),
    MAKE_REQUEST_METHOD_TYPE(PROPFIND),
    MAKE_REQUEST_METHOD_TYPE(PROPPATCH),
    MAKE_REQUEST_METHOD_TYPE(LOCK),
    MAKE_REQUEST_METHOD_TYPE(UNLOCK),
    MAKE_REQUEST_METHOD_TYPE(COPY),
    MAKE_REQUEST_METHOD_TYPE(MOVE),
    MAKE_REQUEST_METHOD_TYPE(MKCOL),
    MAKE_REQUEST_METHOD_TYPE(CONNECT),
    MAKE_REQUEST_METHOD_TYPE(DELETE),
    MAKE_REQUEST_METHOD_TYPE(LINK),
    MAKE_REQUEST_METHOD_TYPE(UNLINK),
    MAKE_REQUEST_METHOD_TYPE(BMOVE),
    MAKE_REQUEST_METHOD_TYPE(BCOPY),
    MAKE_REQUEST_METHOD_TYPE(BPROPFIND),
    MAKE_REQUEST_METHOD_TYPE(BPROPPATCH),
    MAKE_REQUEST_METHOD_TYPE(BDELETE),
    MAKE_REQUEST_METHOD_TYPE(SUBSCRIBE),
    MAKE_REQUEST_METHOD_TYPE(UNSUBSCRIBE),
    MAKE_REQUEST_METHOD_TYPE(NOTIFY),
    MAKE_REQUEST_METHOD_TYPE(POLL), 
    MAKE_REQUEST_METHOD_TYPE(CHECKIN),
    MAKE_REQUEST_METHOD_TYPE(CHECKOUT),
    MAKE_REQUEST_METHOD_TYPE(INVOKE),
    MAKE_REQUEST_METHOD_TYPE(SEARCH),
    MAKE_REQUEST_METHOD_TYPE(PIN),
    MAKE_REQUEST_METHOD_TYPE2(MPOST,M-POST)
};


HTTP_METHOD_TYPE
MapHttpRequestMethod(
    IN LPCSTR lpszVerb
    )

/*++

Routine Description:

    Maps request method string to type. Method names *are* case-sensitive

Arguments:

    lpszVerb    - method (verb) string

Return Value:

    HTTP_METHOD_TYPE

--*/

{
    int verbLen = strlen(lpszVerb);

    for (int i = 0; i < ARRAY_ELEMENTS(MethodNames); ++i) {
        if ((MethodNames[i].Length == verbLen)
        && (memcmp(lpszVerb, MethodNames[i].Name, verbLen) == 0)) {
            return MethodNames[i].MethodType;
        }
    }

    //
    // we now hande HTTP_METHOD_TYPE_UNKNOWN
    //

    return HTTP_METHOD_TYPE_UNKNOWN;
}


DWORD
MapHttpMethodType(
    IN HTTP_METHOD_TYPE tMethod,
    OUT LPCSTR * lplpcszName
    )

/*++

Routine Description:

    Map a method type to the corresponding name and length

Arguments:

    tMethod     - to map

    lplpcszName - pointer to pointer to returned name

Return Value:

    DWORD
        Success - length of method name

        Failure - (DWORD)-1

--*/

{
    DWORD length;

    if ((tMethod >= HTTP_METHOD_TYPE_FIRST) && (tMethod <= HTTP_METHOD_TYPE_LAST)) {
        *lplpcszName = MethodNames[tMethod].Name;
        length = MethodNames[tMethod].Length;
    } else {
        length = (DWORD)-1;
    }
    return length;
}

#if INET_DEBUG

LPSTR
MapHttpMethodType(
    IN HTTP_METHOD_TYPE tMethod
    )
{
    return (tMethod == HTTP_METHOD_TYPE_UNKNOWN)
        ? "Unknown"
        : MethodNames[tMethod].Name;
}

#endif

//
//DWORD
//CreateEscapedUrlPath(
//    IN LPSTR lpszUrlPath,
//    OUT LPSTR * lplpszEncodedUrlPath
//    )
//
///*++
//
//Routine Description:
//
//    Given an URL-path, encodes it into a new buffer
//
//Arguments:
//
//    lpszUrlPath             - URL-path to encode
//
//    lplpszEncodedUrlPath    - pointer to returned allocated buffer containing
//                              escaped URL-path
//
//Return Value:
//
//    DWORD
//        Success - ERROR_SUCCESS
//
//        Failure -
//
//--*/
//
//{
//    LPSTR lpszEncodedUrlPath = NULL;
//    DWORD urlPathLength;
//    DWORD encodedUrlPathLength;
//    DWORD error;
//
//    //
//    // we need to encode the URL-path into a separate buffer (it may grow)
//    //
//
//    urlPathLength = strlen(lpszUrlPath);
//    encodedUrlPathLength = INTERNET_MAX_PATH_LENGTH;
//
//    do {
//
//        //
//        // we allow ourselves to fail due to insufficient buffer (at least once)
//        //
//
//        lpszEncodedUrlPath = (LPSTR)ResizeBuffer(lpszEncodedUrlPath,
//                                                 encodedUrlPathLength,
//                                                 FALSE
//                                                 );
//        if (lpszEncodedUrlPath != NULL) {
//
//            DWORD previousLength = encodedUrlPathLength;
//
//            error = EncodeUrlPath(NO_ENCODE_PATH_SEP,
//
//                                  //
//                                  // BUGBUG - assuming HTTP
//                                  //
//
//                                  SCHEME_HTTP,
//                                  lpszUrlPath,
//                                  urlPathLength,
//                                  lpszEncodedUrlPath,
//                                  &encodedUrlPathLength
//                                  );
//
//            if ((error == ERROR_INSUFFICIENT_BUFFER)
//            && (previousLength >= encodedUrlPathLength)) {
//
//                //
//                // this should never happen, but we will avoid a loop if it does
//                //
//
//                INET_ASSERT(FALSE);
//
//                error = ERROR_INTERNET_INTERNAL_ERROR;
//            }
//        } else {
//
//            //
//            // failed to (re)alloc
//            //
//
//            error = ERROR_NOT_ENOUGH_MEMORY;
//        }
//    } while (error == ERROR_INSUFFICIENT_BUFFER);
//
//    *lplpszEncodedUrlPath = lpszEncodedUrlPath;
//
//    return error;
//}

PRIVATE
BOOL
FMatchList(
    LPSTR *lplpList,
    DWORD cListLen,
    HEADER_STRING *lpHeader,
    LPSTR    lpBase
    )
{
    DWORD i;
    for (i=0; i < cListLen; ++i) {
       if (!lpHeader->Strnicmp(lpBase, lplpList[i], strlen(lplpList[i]))) {
          return (TRUE);
       }
    }
    return(FALSE);
}

#ifdef COMPRESSED_HEADERS
DWORD
LookupHeadermap(
    LPSTR   lpszHeader
)
{
    DWORD   top, mid, bottom, ret = 0;
    int cmp;
    LPSTR lpszColon;

    lpszColon = strchr(lpszHeader, ':');

    if (!lpszColon || (lpszColon == lpszHeader)) {
        return(ret);
    }

    // yuk
    *lpszColon = 0;

    top = 1;

    bottom = sizeof(rgsHeaderMap)/sizeof(HEADER_MAP);

    INET_ASSERT(bottom >= top);

    do {

        mid = (bottom+top)/2;
        if (!(cmp = stricmp(   rgsHeaderMap[mid].lpszLongHeader,
                                lpszHeader
                                ))) {
            // we found a matching header,
            ret = mid;
            break;
        }

        if (cmp > 0) {
            // the mid header is larger than the passed in header
            // so we must check at the upper end of the sorted array of headers
            bottom = mid-1;
        }
        else {
            // the mid header is smaller than the passed in header
            // so we must check at the lower end of the sorted array of headers
            top = mid+1;
        }
    } while (bottom >= top);

    *lpszColon = ':';
    return (ret);
}
#endif //COMPRESSED_HEADERS


//
// HTTP_HEADER_PARSER implementation
//

HTTP_HEADER_PARSER::HTTP_HEADER_PARSER(
    IN LPSTR szHeaders,
    IN DWORD cbHeaders
    ) : HTTP_HEADERS()

/*++

Routine Description:

    Constructor for the HTTP_HEADER_PARSER object.  Calls ParseHeaders to
      build a parsed version of the header string passed in.

Arguments:

    szHeaders      - pointer to the headers to parse

    cbHeaders      - length of the headers

Return Value:

    None.

--*/

{
    DWORD dwBytesScaned = 0;
    BOOL fFoundCompleteLine;
    BOOL fFoundEndOfHeaders;
    DWORD error;

    error = ParseHeaders(
        szHeaders,
        cbHeaders,
        TRUE, // Eof
        &dwBytesScaned,
        &fFoundCompleteLine,
        &fFoundEndOfHeaders
        );

    INET_ASSERT(error == ERROR_SUCCESS);
    INET_ASSERT(fFoundCompleteLine);
    INET_ASSERT(fFoundEndOfHeaders);
}


BOOL
HTTP_HEADER_PARSER::ParseStatusLine(
    IN LPSTR lpHeaderBase,
    IN DWORD dwBufferLength,
    IN BOOL fEof,
    IN OUT DWORD *lpdwBufferLengthScanned,
    OUT DWORD *lpdwStatusCode,
    OUT DWORD *lpdwMajorVersion,
    OUT DWORD *lpdwMinorVersion
    )

/*++

Routine Description:

    Parses the Status line of an HTTP server response.  Takes care of adding the status
     line to HTTP header array.

Arguments:

    lpszHeader      - pointer to the header to check

    dwHeaderLength  - length of the header

Return Value:

    BOOL  - TRUE if line was successively parsed and processed, FALSE otherwise

--*/

{

#define BEFORE_VERSION_NUMBERS 0
#define MAJOR_VERSION_NUMBER   1
#define MINOR_VERSION_NUMBER   2
#define STATUS_CODE_NUMBER     3
#define AFTER_STATUS_CODE      4
#define MAX_STATUS_INTS        4

    LPSTR lpszEnd = lpHeaderBase + dwBufferLength;
    LPSTR response = lpHeaderBase + *lpdwBufferLengthScanned;
    DWORD dwBytesScanned = 0;
    DWORD dwStatusLineSize = 0;
    LPSTR lpszStatusLine;
    int ver_state = BEFORE_VERSION_NUMBERS;
    DWORD adwStatusInts[MAX_STATUS_INTS];
    BOOL success = TRUE;

    for ( int i = 0; i < MAX_STATUS_INTS; i++)
        adwStatusInts[i] = 0;

    lpszStatusLine = response;

    //
    // While walking the Status Line looking for terminating \r\n,
    //   we extract the Major.Minor Versions and Status Code in that order.
    //   text and spaces will lie between/before/after the three numbers
    //   but the idea is to remeber which number we're calculating based on a numeric state
    //   If all goes well the loop will churn out an array with the 3 numbers plugged in as DWORDs
    //

    while ((response < lpszEnd) && (*response != '\r') && (*response != '\n'))
    {
        // below should be wrapped in while (response[i] != ' ') to be more robust???
        switch (ver_state)
        {
            case BEFORE_VERSION_NUMBERS:
                if (*response == '/')
                {
                    INET_ASSERT(ver_state == BEFORE_VERSION_NUMBERS);
                    ver_state++; // = MAJOR_VERSION_NUMBER
                }
                else if (*response == ' ')
                {
                    ver_state = STATUS_CODE_NUMBER;
                }

                break;

            case MAJOR_VERSION_NUMBER:

                if (*response == '.')
                {
                    INET_ASSERT(ver_state == MAJOR_VERSION_NUMBER);
                    ver_state++; // = MINOR_VERSION_NUMBER
                    break;
                }
                // fall through

            case MINOR_VERSION_NUMBER:

                if (*response == ' ')
                {
                    INET_ASSERT(ver_state == MINOR_VERSION_NUMBER);
                    ver_state++; // = STATUS_CODE_NUMBER
                    break;
                }
                // fall through

            case STATUS_CODE_NUMBER:

                if (isdigit(*response)) {
                    int val = *response - '0';
                    adwStatusInts[ver_state] = adwStatusInts[ver_state] * 10 + val;
                }
                else if ( adwStatusInts[STATUS_CODE_NUMBER] > 0 )
                {
                    //
                    // we eat spaces before status code is found,
                    //  once we have the status code we can go on to the next
                    //  state on the next non-digit. This is done
                    //  to cover cases with several spaces between version
                    //  and the status code number.
                    //

                    INET_ASSERT(ver_state == STATUS_CODE_NUMBER);
                    ver_state++; // = AFTER_STATUS_CODE
                    break;
                } else if (!isspace(*response)) {
                    adwStatusInts[ver_state] = (DWORD)-1;
                }

                break;

            case AFTER_STATUS_CODE:
                break;

        }

        ++response;
        ++dwBytesScanned;
    }

    dwStatusLineSize = dwBytesScanned;

    if (response == lpszEnd) {

        //
        // response now points one past the end of the buffer. We may be looking
        // over the edge...
        //
        // if we're at the end of the connection then the server sent us an
        // incorrectly formatted response. Probably an error.
        //
        // Otherwise its a partial response. We need more
        //


        DEBUG_PRINT(HTTP,
                    INFO,
                    ("found end of short response in status line\n"
                    ));

        success = fEof ? TRUE : FALSE;

        //
        // if we really hit the end of the response then update the amount of
        // headers scanned
        //

        if (!success) {
            dwBytesScanned = 0;
        }

        goto quit;

    }

    while ((response < lpszEnd)
    && ((*response == '\r') || (*response == ' '))) {
        ++response;
        ++dwBytesScanned;
    }

    if (response == lpszEnd) {

        //
        // hit end of buffer without finding LF
        //

        success = FALSE;

        DEBUG_PRINT(HTTP,
                    WARNING,
                    ("hit end of buffer without finding LF\n"
                    ));

        goto quit;

    } else if (*response == '\n') {
        ++response;
        ++dwBytesScanned;

        //
        // if we found the empty line then we are done
        //

        success = TRUE;
    }


    INET_ASSERT(success);

    //
    // Now we have our parsed header to add to the array
    //

    HEADER_STRING * freeHeader;
    DWORD iSlot;

    freeHeader = FindFreeSlot(&iSlot);
    if (freeHeader == NULL) {
        INET_ASSERT(FALSE);
        success = FALSE;
        goto quit;
    } else {
        INET_ASSERT(iSlot == 0); // status line should always be first
        freeHeader->CreateOffsetString((DWORD)(lpszStatusLine - lpHeaderBase), dwStatusLineSize);
        freeHeader->SetHash(0); // status line has no hash value.
    }


quit:

    *lpdwStatusCode    = adwStatusInts[STATUS_CODE_NUMBER];
    *lpdwMajorVersion  = adwStatusInts[MAJOR_VERSION_NUMBER];
    *lpdwMinorVersion  = adwStatusInts[MINOR_VERSION_NUMBER];

    *lpdwBufferLengthScanned += dwBytesScanned;

    return success;
}

DWORD
HTTP_HEADER_PARSER::ParseHeaders(
    IN LPSTR lpHeaderBase,
    IN DWORD dwBufferLength,
    IN BOOL fEof,
    IN OUT DWORD *lpdwBufferLengthScanned,
    OUT LPBOOL pfFoundCompleteLine,
    OUT LPBOOL pfFoundEndOfHeaders
    )

/*++

Routine Description:

    Loads headers into HTTP_HEADERS member for subsequent parsing.

    Parses string based headers and adds their parts to an internally stored
    array of HTTP_HEADERS.

    Input is assumed to be well formed Header Name/Value pairs, each deliminated
    by ':' and '\r\n'.

Arguments:

    lpszHeader      - pointer to the header to check

    dwHeaderLength  - length of the header

Return Value:

    None.

--*/


{

    LPSTR lpszEnd = lpHeaderBase + dwBufferLength;
    LPSTR response = lpHeaderBase + *lpdwBufferLengthScanned;
    DWORD dwBytesScanned = 0;
    BOOL success = FALSE;
    DWORD error = ERROR_SUCCESS;

    *pfFoundEndOfHeaders  = FALSE;

    //
    // Each iteration of the following loop
    // walks an HTTP header line of the form:
    //  HeaderName: HeaderValue\r\n
    //

    do
    {
        DWORD dwHash = HEADER_HASH_SEED;
        LPSTR lpszHeaderName;
        DWORD dwHeaderNameLength = 0;
        DWORD dwHeaderLineLength = 0;
        DWORD dwPreviousAmountOfBytesScanned = dwBytesScanned;

        //
        // Remove leading whitespace from header
        //

        while ( (response < lpszEnd) && ((*response == ' ') || (*response == '\t')) )
        {
            ++response;
            ++dwBytesScanned;
        }

        //
        // Scan for HeaderName:
        //

        lpszHeaderName = response;
        dwPreviousAmountOfBytesScanned = dwBytesScanned;

        while ((response < lpszEnd) && (*response != ':') && (*response != '\r') && (*response != '\n'))
        {
            //
            // This code incapsulates CalculateHashNoCase as an optimization,
            //   we attempt to calculate the Hash value as we parse the header.
            //

            CHAR ch = *response;

            if ((ch >= 'A') && (ch <= 'Z')) {
                ch = MAKE_LOWER(ch);
            }
            dwHash += (DWORD)(dwHash << 5) + ch;

            ++response;
            ++dwBytesScanned;
        }

        dwHeaderNameLength = (DWORD) (response - lpszHeaderName);

        //
        // catch bogus responses: if we find what looks like one of a (very)
        // small set of HTML tags, then assume the previous header was the
        // last
        //

        if ((dwHeaderNameLength >= sizeof("<HTML>") - 1)
            && (*lpszHeaderName == '<')
            && (!strnicmp(lpszHeaderName, "<HTML>", sizeof("<HTML>") - 1)
                || !strnicmp(lpszHeaderName, "<HEAD>", sizeof("<HEAD>") - 1))) {
            *pfFoundEndOfHeaders  = TRUE;
            break;
        }

        //
        // Keep scanning till end of the line.
        //

        while ((response < lpszEnd) && (*response != '\r') && (*response != '\n'))
        {
            ++response;
            ++dwBytesScanned;
        }

        dwHeaderLineLength = (DWORD) (response - lpszHeaderName); // note: this headerLINElength

        if (response == lpszEnd) {

            //
            // response now points one past the end of the buffer. We may be looking
            // over the edge...
            //
            // if we're at the end of the connection then the server sent us an
            // incorrectly formatted response. Probably an error.
            //
            // Otherwise its a partial response. We need more
            //


            DEBUG_PRINT(HTTP,
                        INFO,
                        ("found end of short response\n"
                        ));

            success = fEof ? TRUE : FALSE;

            //
            // if we really hit the end of the response then update the amount of
            // headers scanned
            //

            if (!success) {
                dwBytesScanned = dwPreviousAmountOfBytesScanned;
            }

            break;

        }
        else
        {

            //
            // we reached a CR or LF. This is the end of this current header. Find
            // the start of the next one
            //

            //
            // first, strip off any trailing spaces from the current header. We do
            // this by simply reducing the string length. We only look for space
            // and tab characters. Only do this if we have a non-zero length header
            //

            if (dwHeaderLineLength != 0) {
                for (int i = -1; response[i] == ' ' || response[i] == '\t'; --i) {
                    --dwHeaderLineLength;
                }
            }

            INET_ASSERT((int)dwHeaderLineLength >= 0);

            //
            // some servers respond with "\r\r\n". Lame
            // A new twist: "\r \r\n". Lamer
            //

            while ((response < lpszEnd)
            && ((*response == '\r') || (*response == ' '))) {
                ++response;
                ++dwBytesScanned;
            }
            if (response == lpszEnd) {

                //
                // hit end of buffer without finding LF
                //

                success = FALSE;

                DEBUG_PRINT(HTTP,
                            WARNING,
                            ("hit end of buffer without finding LF\n"
                            ));

                //
                // get more data, reparse this line
                //

                dwBytesScanned = dwPreviousAmountOfBytesScanned;
                break;
            } else if (*response == '\n') {
                ++response;
                ++dwBytesScanned;

                //
                // if we found the empty line then we are done
                //

                if (dwHeaderLineLength == 0) {
                    *pfFoundEndOfHeaders  = TRUE;
                    break;
                }

                success = TRUE;
            }
        }

        //
        // Now we have our parsed header to add to the array
        //

        HEADER_STRING * freeHeader;
        DWORD iSlot;

        freeHeader = FindFreeSlot(&iSlot);
        if (freeHeader == NULL) {
            error = GetError();

            INET_ASSERT(error != ERROR_SUCCESS);
            goto quit;

        } else {
            freeHeader->CreateOffsetString((DWORD) (lpszHeaderName - lpHeaderBase), dwHeaderLineLength);
            freeHeader->SetHash(dwHash);
        }


        //CHAR szTemp[256];
        //
        //memcpy(szTemp, lpszHeaderName, dwHeaderLineLength);
        //lpszHeaderName[dwHeaderLineLength] = '\0';

        //DEBUG_PRINT(HTTP,
        //    INFO,
        //    ("ParseHeaders: adding=%q\n", lpszHeaderName
        //    ));


        //
        // Now see if this is a known header we are adding, if so then we note that fact
        //

        DWORD dwKnownQueryIndex;

        if (HeaderMatch(dwHash, lpszHeaderName, dwHeaderNameLength, &dwKnownQueryIndex) )
        {
            freeHeader->SetNextKnownIndex(FastAdd(dwKnownQueryIndex, iSlot));
        }
    } while (TRUE);

quit:

    *lpdwBufferLengthScanned += dwBytesScanned;
    *pfFoundCompleteLine = success;

    return error;
}

#if 0
//
// Slower version of the function above used for performance work!!! Keep around
//   until we're sure it will never be used.
//

DWORD
HTTP_HEADER_PARSER::ParseHeaders(
    IN LPSTR lpHeaderBase,
    IN DWORD dwBufferLength,
    IN BOOL fEof,
    IN OUT DWORD *lpdwBufferLengthScanned,
    OUT LPBOOL pfFoundCompleteLine,
    OUT LPBOOL pfFoundEndOfHeaders
    )

/*++

Routine Description:

    Parses string based headers and adds their parts to an internally stored
    array of HTTP_HEADERS.


    Input is assumed to be well formed Header Name/Value pairs, each deliminated
    by ':' and '\r\n'.

Arguments:

    lpszHeader      - pointer to the header to check

    dwHeaderLength  - length of the header

Return Value:

    None.

--*/


{

#define HTTP_HEADER_PARSE_LEADING_SPACE 0
#define HTTP_HEADER_PARSE_NAME 1
#define HTTP_HEADER_PARSE_VALUE 2
#define HTTP_HEADER_PARSE_CR 3
#define HTTP_HEADER_PARSE_LF 4


    LPSTR lpszEnd = lpHeaderBase + dwBufferLength;
    LPSTR response = lpHeaderBase + *lpdwBufferLengthScanned;
    DWORD dwBytesScanned = 0;
    BOOL success = FALSE;
    DWORD error = ERROR_SUCCESS;
    DWORD state = HTTP_HEADER_PARSE_LEADING_SPACE;

    *pfFoundEndOfHeaders  = FALSE;

    //
    // Each iteration of the following loop
    // walks an HTTP header line of the form:
    //  HeaderName: HeaderValue\r\n
    //

        DWORD dwHash = HEADER_HASH_SEED;
        LPSTR lpszHeaderName;
        DWORD dwHeaderNameLength = 0;
        DWORD dwHeaderLineLength = 0;
        DWORD dwPreviousAmountOfBytesScanned = dwBytesScanned;
        DWORD dwWhiteSpace = 0;


        while ( (response < lpszEnd) )
        {
            switch (state)
            {

                case HTTP_HEADER_PARSE_LEADING_SPACE:

                    //
                    // Remove leading whitespace from header
                    //

                    if ( *response == ' ' ||
                         *response == '\t' )
                    {
                        break;
                    }

                    //
                    // Scan for HeaderName:
                    //

                    state = HTTP_HEADER_PARSE_NAME;
                    lpszHeaderName = response;
                    dwPreviousAmountOfBytesScanned = dwBytesScanned;

                    // fall through

                case HTTP_HEADER_PARSE_NAME:

                    switch (*response)
                    {
                        case ':':
                            //
                            // Now parse the Header Value
                            //

                            state = HTTP_HEADER_PARSE_VALUE;
                            dwHeaderNameLength = (DWORD) (response - lpszHeaderName);
                            break;


                        case '\r':
                            state = HTTP_HEADER_PARSE_CR;
                            // note: this is headerLINElength
                            dwHeaderLineLength = (DWORD) (response - lpszHeaderName);
                            break;

                        case '\n':

                            state = HTTP_HEADER_PARSE_LF;
                            // note: this is headerLINElength
                            dwHeaderLineLength = (DWORD) (response - lpszHeaderName);
                            goto Got_LF;
                            //break;

                        default:
                        {
                            CHAR ch = *response;

                            if ((ch >= 'A') && (ch <= 'Z')) {
                                ch = MAKE_LOWER(ch);
                            }
                            dwHash += (DWORD)(dwHash << 5) + ch;

                            break;
                        }
                    }

                    break;

                case HTTP_HEADER_PARSE_VALUE:

                    switch ( *response )
                    {
                        case '\r':
                            state = HTTP_HEADER_PARSE_CR;
                            // note: this is headerLINElength
                            dwHeaderLineLength = (DWORD) (response - lpszHeaderName) - dwWhiteSpace;
                            break;

                        case '\n':

                            state = HTTP_HEADER_PARSE_LF;
                            // note: this is headerLINElength
                            dwHeaderLineLength = (DWORD) (response - lpszHeaderName) - dwWhiteSpace;
                            goto Got_LF;

                        case ' ':
                        case '\t':
                            // count whitespace at end of the line
                            dwWhiteSpace++;
                            break;

                        default:
                            dwWhiteSpace = 0;
                            break;
                    }

                    break;

                case HTTP_HEADER_PARSE_CR:

                    if (*response == ' ' ||
                        *response == '\r' )
                    {
                        break;
                    }

                    state = HTTP_HEADER_PARSE_LF;
                    // fall through

                case HTTP_HEADER_PARSE_LF:

Got_LF:

                    //
                    // if we found the empty line then we are done
                    //

                    success = TRUE;

                    if (dwHeaderLineLength == 0) {
                        ++dwBytesScanned;
                        ++response;
                        *pfFoundEndOfHeaders  = TRUE;
                        goto quit;
                    }


                    {
                        //
                        // Now we have our parsed header to add to the array
                        //

                        HEADER_STRING * freeHeader;
                        DWORD iSlot;

                        freeHeader = FindFreeSlot(&iSlot);
                        if (freeHeader == NULL) {
                            error = GetError();

                            INET_ASSERT(error != ERROR_SUCCESS);
                            goto quit;

                        } else {
                            freeHeader->CreateOffsetString((lpszHeaderName - lpHeaderBase), dwHeaderLineLength);
                            freeHeader->SetHash(dwHash);
                        }

                        CHAR szTemp[256];

                        memcpy(szTemp, lpszHeaderName, dwHeaderLineLength);
                        szTemp[dwHeaderLineLength] = '\0';

                        DEBUG_PRINT(HTTP,
                            INFO,
                            ("ParseHeaders: adding=%q\n", lpszHeaderName
                            ));


                        //
                        // Now see if this is a known header we are adding, if so then we note that fact
                        //

                        DWORD dwKnownQueryIndex;

                        if (HeaderMatch(dwHash, lpszHeaderName, dwHeaderNameLength, &dwKnownQueryIndex) )
                        {
                            freeHeader->SetNextKnownIndex(FastAdd(dwKnownQueryIndex, iSlot));
                        }
                    }

                    //
                    // Move on to our next header.
                    //

                    dwHash = HEADER_HASH_SEED;
                    dwHeaderNameLength = 0;
                    dwHeaderLineLength = 0;
                    dwPreviousAmountOfBytesScanned = dwBytesScanned;
                    dwWhiteSpace = 0;

                    state = HTTP_HEADER_PARSE_LEADING_SPACE;

                    break;

            } // switch (state)

            ++response;
            ++dwBytesScanned;

        } // while (response < lpszEnd)


        //
        // response now points one past the end of the buffer. We may be looking
        // over the edge...
        //
        // if we're at the end of the connection then the server sent us an
        // incorrectly formatted response. Probably an error.
        //
        // Otherwise its a partial response. We need more
        //


        DEBUG_PRINT(HTTP,
                    INFO,
                    ("found end of short response\n"
                    ));

        success = fEof ? TRUE : FALSE;

        //
        // if we really hit the end of the response then update the amount of
        // headers scanned
        //

        if (!success) {
            dwBytesScanned = dwPreviousAmountOfBytesScanned;
        }

quit:

  *lpdwBufferLengthScanned += dwBytesScanned;
  *pfFoundCompleteLine    = success;

  return error;
}

#endif
