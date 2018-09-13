/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ftp.hxx

Abstract:

    Contains client-side FTP handle class

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

    This class defines the FTP_FIND_HANDLE_OBJECT.

Private Member functions:

    None.

Public Member functions:

    GetHandle : Virtual function that gets the handle value from
        the generic object handle.

--*/

//
// Bit masks for dwFtpFindBools...
//

#define FTPFIND_ISHTML  0x00000001 // set if contents are wrapped in HTML
#define FTPFIND_ISEMPTY 0x00000002 // set if FtpFindFirst was already done

class FTP_FIND_HANDLE_OBJECT : public INTERNET_CONNECT_HANDLE_OBJECT {

private:

    HINTERNET _FindHandle;
    CLOSE_HANDLE_FUNC _wCloseFunction;
    DWORD _dwFtpFindBools; // encodes flags
    HTML_STATE _HtmlState;
    LPSTR _lpszUrl;
    LPSTR _lpszDirEntry;

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

    FTP_FIND_HANDLE_OBJECT(
        INTERNET_CONNECT_HANDLE_OBJECT * InternetConnectObj,
        LPTSTR SearchString,
        LPWIN32_FIND_DATA Buffer,
        DWORD_PTR dwContext
        );

    FTP_FIND_HANDLE_OBJECT(
        INTERNET_CONNECT_HANDLE_OBJECT * Parent,
        HINTERNET Child,
        CLOSE_HANDLE_FUNC wCloseFunc,
        DWORD_PTR dwContext
        );

    virtual ~FTP_FIND_HANDLE_OBJECT(VOID);

    virtual HINTERNET GetHandle(VOID);

    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID) {
        return (_dwFtpFindBools & FTPFIND_ISHTML)?
            TypeFtpFindHandleHtml : TypeFtpFindHandle;
    }

    virtual VOID SetHtml(VOID) {
        _HtmlState = HTML_STATE_START;
        _dwFtpFindBools |= FTPFIND_ISHTML;
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
                        ("Freeing FTP query buffer %#x\n",
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

    BOOL IsEmpty (void) {
        return _dwFtpFindBools & FTPFIND_ISEMPTY;
    }

    void SetIsEmpty (void) {
        _dwFtpFindBools |= FTPFIND_ISEMPTY;
    }
};

/*++

Class Description:

    This class defines the FTP_FILE_HANDLE_OBJECT.

Private Member functions:

    None.

Public Member functions:

    GetHandle : Virtual function that gets the handle value from the
        generic object handle.

--*/

class FTP_FILE_HANDLE_OBJECT : public INTERNET_CONNECT_HANDLE_OBJECT {

private:

    HINTERNET _FileHandle;
    CLOSE_HANDLE_FUNC _wCloseFunction;
    BOOL _IsHtml;
    HTML_STATE _HtmlState;
    LPSTR _lpszUrl;
    LPSTR _lpszDirEntry;
    LPSTR _lpszFileName;

public:

    //FTP_FILE_HANDLE_OBJECT(
    //    INTERNET_CONNECT_HANDLE_OBJECT * InternetConnectObj,
    //    LPTSTR lpszFileName,
    //    DWORD fdwAccess,
    //    DWORD dwFlags,
    //    DWORD dwContext
    //    );

    FTP_FILE_HANDLE_OBJECT(
        INTERNET_CONNECT_HANDLE_OBJECT * Parent,
        HINTERNET Child,
        CLOSE_HANDLE_FUNC wCloseFunc,
        DWORD_PTR dwContext
        );

    virtual ~FTP_FILE_HANDLE_OBJECT(VOID);

    virtual HINTERNET GetHandle(VOID);

    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID) {
        return _IsHtml ? TypeFtpFileHandleHtml : TypeFtpFileHandle;
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

    DWORD SetFileName(LPCSTR lpszFileName) {        
        _lpszFileName = NewString(lpszFileName);

        if ( _lpszFileName == NULL )
        {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
        else
        {
            return ERROR_SUCCESS;
        }
    }

    LPSTR GetFileName(VOID) {
        return _lpszFileName;
    }

    VOID SetFileHandle(HINTERNET hInternet) {

        INET_ASSERT(_FileHandle == NULL);

        _FileHandle = hInternet;
    }
};

#ifdef EXTENDED_ERROR_HTML

/*++

Class Description:

    This class defines the FTP_ERROR_HANDLE_OBJECT.

Private Member functions:

    None.

Public Member functions:

    GetHandle : Virtual function that gets the handle value from the
        generic object handle.

--*/

class FTP_ERROR_HANDLE_OBJECT : public INTERNET_CONNECT_HANDLE_OBJECT {

private:

    LPSTR m_lpszErrorText;
    DWORD m_dwErrorTextLength;
    LPVOID m_QueryBuffer;
    DWORD m_QueryBufferLength;
    DWORD m_QueryOffset;
    DWORD m_QueryBytesAvailable;
    HTML_STATE m_HtmlState;

public:

    FTP_ERROR_HANDLE_OBJECT(
        INTERNET_CONNECT_HANDLE_OBJECT * InternetConnectObj
        );

    virtual ~FTP_ERROR_HANDLE_OBJECT();

    virtual HINTERNET_HANDLE_TYPE GetHandleType(VOID) {
        return TypeFtpFileHandleHtml;
    }

    virtual VOID SetHtmlState(HTML_STATE State) {
        m_HtmlState = State;
    }

    virtual HTML_STATE GetHtmlState(VOID) {
        return m_HtmlState;
    }

    DWORD SetErrorText(VOID);

    DWORD
    GetErrorText(
        OUT LPSTR lpszBuffer,
        IN DWORD dwBytesToRead,
        OUT LPDWORD lpdwNumberOfBytesRead
        );

    DWORD AllocateQueryBuffer(VOID) {

        INET_ASSERT(m_QueryBuffer == NULL);
        INET_ASSERT(m_QueryBufferLength == 0);
        INET_ASSERT(m_QueryOffset == 0);
        INET_ASSERT(m_QueryBytesAvailable == 0);

        m_QueryBuffer = ALLOCATE_MEMORY(LMEM_FIXED,
                                        DEFAULT_HTML_QUERY_BUFFER_LENGTH
                                        );
        if (m_QueryBuffer != NULL) {
            m_QueryBufferLength = DEFAULT_HTML_QUERY_BUFFER_LENGTH;
            return ERROR_SUCCESS;
        } else {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    }

    VOID FreeQueryBuffer(VOID) {
        if (m_QueryBuffer != NULL) {

            DEBUG_PRINT(API,
                        INFO,
                        ("Freeing FTP query buffer %#x\n",
                        m_QueryBuffer
                        ));

            FREE_MEMORY((HLOCAL)m_QueryBuffer);
            m_QueryBuffer = NULL;
            m_QueryBufferLength = 0;
            m_QueryOffset = 0;
            m_QueryBytesAvailable = 0;
        }
    }

    BOOL HaveQueryData(VOID) {
        return (m_QueryBytesAvailable != 0) ? TRUE : FALSE;
    }

    DWORD CopyQueriedData(LPVOID lpBuffer, DWORD dwBufferLength) {

        INET_ASSERT(lpBuffer != NULL);
        INET_ASSERT(dwBufferLength != 0);

        DWORD len = min(m_QueryBytesAvailable, dwBufferLength);

        if (len != 0) {
            memcpy(lpBuffer,
                   (LPVOID)((LPBYTE)m_QueryBuffer + m_QueryOffset),
                   len
                   );

            DEBUG_PRINT(API,
                        INFO,
                        ("Copied %d bytes from query buffer @ %#x - %d left\n",
                        len,
                        (LPBYTE)m_QueryBuffer + m_QueryOffset,
                        m_QueryBytesAvailable - len
                        ));

            m_QueryOffset += len;
            m_QueryBytesAvailable -= len;
        }
        return len;
    }

    DWORD
    QueryHtmlDataAvailable(
        OUT LPDWORD lpdwNumberOfBytesAvailable
        );
};

//
// prototypes
//

DWORD
RMakeFtpErrorObjectHandle(
    IN HINTERNET hConnect,
    OUT LPHINTERNET lphError
    );

#endif
