/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gopher.hxx

Abstract:

    Contains the client-side gopher handle class

    Contents:

Author:

    Richard L Firth (rfirth) 03-Jan-1996

Revision History:

    03-Jan-1996 rfirth
        Created

--*/

//
// classes
//

/*++

Class Description:

    This class defines the GOPHER_FIND_HANDLE_OBJECT.

Private Member functions:

    None.

Public Member functions:

    GetHandle : Virtual function that gets the service handle value from
        the generic object handle.

--*/

class GOPHER_FIND_HANDLE_OBJECT : public INTERNET_CONNECT_HANDLE_OBJECT {

private:

    HINTERNET _FindHandle;
    CLOSE_HANDLE_FUNC _wCloseFunction;
    BOOL _IsHtml;
    HTML_STATE _HtmlState;
    LPSTR _lpszUrl;
    LPSTR _lpszDirEntry;
    DWORD _dwFixedType; // for CSO or gopher index search text

    //
    // _QueryBuffer - buffer used to query socket data available
    //

    LPVOID _QueryBuffer;

    //
    // _QueryBufferLength - length of _QueryBuffer
    //

    DWORD _QueryBufferLength;

    //
    // _QueryOffset - offset of next read from _QueryBuffer
    //

    DWORD _QueryOffset;

    //
    // _QueryBytesAvailable - number of bytes we think are available for this
    // socket in the query buffer
    //

    DWORD _QueryBytesAvailable;

public:

    GOPHER_FIND_HANDLE_OBJECT
        (INTERNET_CONNECT_HANDLE_OBJECT * InternetConnectObj,
            LPTSTR Locator,
            LPTSTR SearchString,
            LPGOPHER_FIND_DATA Buffer,
            DWORD_PTR dwContext
            );

    GOPHER_FIND_HANDLE_OBJECT(
        INTERNET_CONNECT_HANDLE_OBJECT * Parent,
        HINTERNET Child,
        CLOSE_HANDLE_FUNC wCloseFunc,
        DWORD_PTR dwContext
        );

    GOPHER_FIND_HANDLE_OBJECT(
        INTERNET_CONNECT_HANDLE_OBJECT * Parent,
        HINTERNET Child,
        DWORD dwFixedType
    );

    virtual ~GOPHER_FIND_HANDLE_OBJECT(VOID);

    virtual HINTERNET GetHandle(VOID);

    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID) {
        return _IsHtml ? TypeGopherFindHandleHtml : TypeGopherFindHandle;
    }

    virtual VOID SetHtml(VOID) {
        _HtmlState = HTML_STATE_START;
        _IsHtml = TRUE;
    }

    DWORD GetFixedType (VOID) {
        return _dwFixedType;
    }

    virtual VOID SetHtmlState(HTML_STATE State) {
        _HtmlState = State;
    }

    virtual HTML_STATE GetHtmlState(VOID) {
        return _HtmlState;
    }

    virtual LPSTR GetUrl(VOID) {
        return _lpszUrl;
    }

    virtual VOID SetUrl(LPSTR Url) {
        _lpszUrl = Url;
    }

    virtual VOID SetDirEntry(LPSTR DirEntry) {
        _lpszDirEntry = DirEntry;
    }

    virtual LPSTR GetDirEntry(VOID) {
        return _lpszDirEntry;
    }

    VOID SetFindHandle(HINTERNET hInternet) {

        INET_ASSERT(_FindHandle == NULL);

        _FindHandle = hInternet;
    }

    DWORD AllocateQueryBuffer(VOID) {

        INET_ASSERT(_QueryBuffer == NULL);
        INET_ASSERT(_QueryBufferLength == 0);
        INET_ASSERT(_QueryOffset == 0);
        INET_ASSERT(_QueryBytesAvailable == 0);

        _QueryBuffer = ALLOCATE_MEMORY(LMEM_FIXED,
                                       DEFAULT_HTML_QUERY_BUFFER_LENGTH
                                       );
        if (_QueryBuffer != NULL) {
            _QueryBufferLength = DEFAULT_HTML_QUERY_BUFFER_LENGTH;
            return ERROR_SUCCESS;
        }
        return ERROR_NOT_ENOUGH_MEMORY;
    }

    VOID FreeQueryBuffer(VOID) {
        if (_QueryBuffer != NULL) {

            DEBUG_PRINT(API,
                        INFO,
                        ("Freeing gopher query buffer %#x\n",
                        _QueryBuffer
                        ));

            FREE_MEMORY((HLOCAL)_QueryBuffer);
            _QueryBuffer = NULL;
            _QueryBufferLength = 0;
            _QueryOffset = 0;
            _QueryBytesAvailable = 0;
        }
    }

    BOOL HaveQueryData(VOID) {
        return (_QueryBytesAvailable != 0) ? TRUE : FALSE;
    }

    DWORD CopyQueriedData(LPVOID lpBuffer, DWORD dwBufferLength) {

        INET_ASSERT(lpBuffer != NULL);
        INET_ASSERT(dwBufferLength != 0);

        DWORD len = min(_QueryBytesAvailable, dwBufferLength);

        if (len != 0) {
            memcpy(lpBuffer,
                   (LPVOID)((LPBYTE)_QueryBuffer + _QueryOffset),
                   len
                   );

            DEBUG_PRINT(API,
                        INFO,
                        ("Copied %d bytes from query buffer @ %#x - %d left\n",
                        len,
                        (LPBYTE)_QueryBuffer + _QueryOffset,
                        _QueryBytesAvailable - len
                        ));

            _QueryOffset += len;
            _QueryBytesAvailable -= len;
        }
        return len;
    }

    DWORD
    QueryHtmlDataAvailable(
        OUT LPDWORD lpdwNumberOfBytesAvailable
        );
};

/*++

Class Description:

    This class defines the GOPHER_FILE_HANDLE_OBJECT.

Private Member functions:

    None.

Public Member functions:

    GetHandle : Virtual function that gets the service handle value from
        the generic object handle.

--*/

class GOPHER_FILE_HANDLE_OBJECT : public INTERNET_CONNECT_HANDLE_OBJECT {

private:

    HINTERNET _FileHandle;
    CLOSE_HANDLE_FUNC _wCloseFunction;
    BOOL _IsHtml;
    HTML_STATE _HtmlState;
    LPSTR _lpszUrl;
    LPSTR _lpszDirEntry;

public:

    GOPHER_FILE_HANDLE_OBJECT
        (INTERNET_CONNECT_HANDLE_OBJECT * InternetConnectObj,
            LPTSTR Locator,
            LPTSTR View,
            DWORD Flags,
            DWORD_PTR dwContext
            );

    GOPHER_FILE_HANDLE_OBJECT(
        INTERNET_CONNECT_HANDLE_OBJECT * Parent,
        HINTERNET Child,
        CLOSE_HANDLE_FUNC wCloseFunc,
        DWORD_PTR dwContext
        );

    virtual ~GOPHER_FILE_HANDLE_OBJECT(VOID);

    virtual HINTERNET GetHandle(VOID);

    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID) {
        return _IsHtml ? TypeGopherFileHandleHtml : TypeGopherFileHandle;
    }

    virtual VOID SetHtml(VOID) {
        _HtmlState = HTML_STATE_START;
        _IsHtml = TRUE;
    }

    virtual VOID SetHtmlState(HTML_STATE State) {
        _HtmlState = State;
    }

    virtual HTML_STATE GetHtmlState(VOID) {
        return _HtmlState;
    }

    virtual LPSTR GetUrl(VOID) {
        return _lpszUrl;
    }

    virtual VOID SetUrl(LPSTR Url) {
        _lpszUrl = Url;
    }

    virtual VOID SetDirEntry(LPSTR DirEntry) {
        _lpszDirEntry = DirEntry;
    }

    virtual LPSTR GetDirEntry(VOID) {
        return _lpszDirEntry;
    }

    VOID SetFileHandle(HINTERNET hInternet) {

        INET_ASSERT(_FileHandle == NULL);

        _FileHandle = hInternet;
    }
};
