/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ftp.cxx

Abstract:

    Contains methods for FTP_FIND_HANDLE_OBJECT and FTP_FILE_HANDLE_OBJECT classes

    Contents:
        RMakeFtpFindObjectHandle
        RMakeFtpFileObjectHandle
        RMakeFtpErrorObjectHandle
        FTP_ERROR_HANDLE_OBJECT::SetErrorText
        FTP_ERROR_HANDLE_OBJECT::QueryHtmlDataAvailable

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:

   Sophia Chung (sophiac) 14-Feb-1995 (added FTP and Archie class impl.)
   (code adopted from madana)

--*/

#include <wininetp.h>

//
// functions
//


DWORD
RMakeFtpFindObjectHandle(
    IN HINTERNET ParentHandle,
    IN OUT HINTERNET * ChildHandle,
    IN CLOSE_HANDLE_FUNC wCloseFunc,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    C-callable wrapper for creating an FTP_FIND_HANDLE_OBJECT

Arguments:

    ParentHandle    - mapped address of parent (connect) handle

    ChildHandle     - IN: protocol-specific handle value associated with object
                      OUT: mapped address of FTP_FIND_HANDLE_OBJECT

    wCloseFunc      - address of protocol-specific function to be called when
                      object is closed

    dwContext       - app-supplied context value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DWORD error;
    FTP_FIND_HANDLE_OBJECT * hFind;

    hFind = new FTP_FIND_HANDLE_OBJECT(
                    (INTERNET_CONNECT_HANDLE_OBJECT *)ParentHandle,
                    *ChildHandle,
                    wCloseFunc,
                    dwContext
                    );

    if (hFind != NULL) {
        error = hFind->GetStatus();
        if (error == ERROR_SUCCESS) {

            //
            // inform the app of the new handle
            //

            error = InternetIndicateStatusNewHandle((LPVOID)hFind);

            //
            // ERROR_INTERNET_OPERATION_CANCELLED is the only error that we are
            // expecting here. If we get this error then the app has cancelled
            // the operation. Either way, the handle we just generated will be
            // already deleted
            //

            if (error != ERROR_SUCCESS) {

                INET_ASSERT(error == ERROR_INTERNET_OPERATION_CANCELLED);

                hFind = NULL;
            }
        } else {
            delete hFind;
            hFind = NULL;
        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ChildHandle = (HINTERNET)hFind;

    return error;
}


DWORD
RMakeFtpFileObjectHandle(
    IN HINTERNET ParentHandle,
    IN OUT HINTERNET * ChildHandle,
    IN CLOSE_HANDLE_FUNC wCloseFunc,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    C-callable wrapper for creating an FTP_FILE_HANDLE_OBJECT

Arguments:

    ParentHandle    - mapped address of parent (connect) handle

    ChildHandle     - IN: protocol-specific handle value associated with object
                      OUT: mapped address of FTP_FILE_HANDLE_OBJECT

    wCloseFunc      - address of protocol-specific function to be called when
                      object is closed

    dwContext       - app-supplied context value

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DWORD error;
    FTP_FILE_HANDLE_OBJECT * hFile;
    DEBUG_PRINT(FTP,
                INFO,
                ("RMakeFtpFileObject(0x%x 0x%x 0x%x 0x%x)\r\n", 
                ParentHandle, ChildHandle, wCloseFunc, dwContext));


    hFile = new FTP_FILE_HANDLE_OBJECT(
                    (INTERNET_CONNECT_HANDLE_OBJECT *)ParentHandle,
                    *ChildHandle,
                    wCloseFunc,
                    dwContext
                    );

    if (hFile != NULL) {
        error = hFile->GetStatus();
        if (error == ERROR_SUCCESS) {

            //
            // inform the app of the new handle
            //

            error = InternetIndicateStatusNewHandle((LPVOID)hFile);

            //
            // ERROR_INTERNET_OPERATION_CANCELLED is the only error that we are
            // expecting here. If we get this error then the app has cancelled
            // the operation. Either way, the handle we just generated will be
            // already deleted
            //

            if (error != ERROR_SUCCESS) {

                INET_ASSERT(error == ERROR_INTERNET_OPERATION_CANCELLED);

                hFile = NULL;
            }
        } else {
            delete hFile;
            hFile = NULL;
        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    *ChildHandle = (HINTERNET)hFile;

    return error;
}

#ifdef EXTENDED_ERROR_HTML

DWORD
RMakeFtpErrorObjectHandle(
    IN HINTERNET hConnect,
    OUT LPHINTERNET lphError
    )

/*++

Routine Description:

    Creates an FTP_ERROR_HANDLE_OBJECT. Used to return extended error info as
    HTML

Arguments:

    hConnect    - pointer to INTERNET_CONNECT_HANDLE_OBJECT

    lphError    - pointer to returned FTP_ERROR_HANDLE_OBJECT

Return Value:

    DWORD

--*/

{
    FTP_ERROR_HANDLE_OBJECT * hError;

    hError = new FTP_ERROR_HANDLE_OBJECT(
                    (INTERNET_CONNECT_HANDLE_OBJECT *)hConnect
                    );

    DWORD error;

    if (hError != NULL) {
        error = hError->GetStatus();
        if (error == ERROR_SUCCESS) {

            //
            // inform the app of the new handle
            //

            error = InternetIndicateStatusNewHandle((LPVOID)hError);

            //
            // ERROR_INTERNET_OPERATION_CANCELLED is the only error that we are
            // expecting here. If we get this error then the app has cancelled
            // the operation. Either way, the handle we just generated will be
            // already deleted
            //

            if (error != ERROR_SUCCESS) {

                INET_ASSERT(error == ERROR_INTERNET_OPERATION_CANCELLED);

                hError = NULL;
            }
        } else {
            delete hError;
            hError = NULL;
        }
    } else {
        error = ERROR_NOT_ENOUGH_MEMORY;
    }

    *lphError = (HINTERNET)hError;

    return error;
}
#endif

//
// FTP_FIND_HANDLE_OJBECT class implementation
//

FTP_FIND_HANDLE_OBJECT::FTP_FIND_HANDLE_OBJECT(
    INTERNET_CONNECT_HANDLE_OBJECT *Parent,
    HINTERNET Child,
    CLOSE_HANDLE_FUNC wCloseFunc,
    DWORD_PTR dwContext
    ) : INTERNET_CONNECT_HANDLE_OBJECT(Parent)
{
    _FindHandle = Child;
    _wCloseFunction = wCloseFunc;
    _dwFtpFindBools = 0;
    _lpszUrl = NULL;
    _lpszDirEntry = NULL;
    _QueryBuffer = NULL;
    _QueryBufferLength = 0;
    _QueryOffset = 0;
    _QueryBytesAvailable = 0;
    _Context = dwContext;
    SetObjectType(TypeFtpFindHandle);
}

FTP_FIND_HANDLE_OBJECT::~FTP_FIND_HANDLE_OBJECT(
    VOID
    )
{
    //
    // if local internet handle, closed by local close function
    //

    if (_FindHandle != NULL) {
        _Status = _wCloseFunction(_FindHandle);

        //INET_ASSERT(_Status == ERROR_SUCCESS);

    } else {
        _Status = ERROR_SUCCESS;
    }

    //
    // clear out any strings we allocated
    //

    if (_lpszUrl != NULL) {
        DEL_STRING(_lpszUrl);
    }
    if (_lpszDirEntry != NULL) {
        DEL_STRING(_lpszDirEntry);
    }

    //
    // and the query buffer
    //

    FreeQueryBuffer();
}

HANDLE
FTP_FIND_HANDLE_OBJECT::GetHandle(
    VOID
    )
{
    return _FindHandle;
}

DWORD
FTP_FIND_HANDLE_OBJECT::QueryHtmlDataAvailable(
    OUT LPDWORD lpdwNumberOfBytesAvailable
    )
{
    DWORD error;

    if (_QueryBuffer != NULL) {
        error = ERROR_SUCCESS;
    } else {
        error = AllocateQueryBuffer();
    }

    INET_ASSERT(_QueryBytesAvailable == 0);

    if (error == ERROR_SUCCESS) {
        _QueryOffset = 0;
        if (ReadHtmlUrlData((HINTERNET)this,
                            _QueryBuffer,
                            _QueryBufferLength,
                            lpdwNumberOfBytesAvailable
                            )) {
            _QueryBytesAvailable = *lpdwNumberOfBytesAvailable;
            //SetAvailableDataLength(_QueryBytesAvailable);
            if (_QueryBytesAvailable == 0) {
                SetEndOfFile();
            }
        } else {
            error = GetLastError();
        }
    }
    return error;
}

//
// FTP_FILE_HANDLE_OJBECT class implementation
//

FTP_FILE_HANDLE_OBJECT::FTP_FILE_HANDLE_OBJECT(
    INTERNET_CONNECT_HANDLE_OBJECT *Parent,
    HINTERNET Child,
    CLOSE_HANDLE_FUNC wCloseFunc,
    DWORD_PTR dwContext
    ) : INTERNET_CONNECT_HANDLE_OBJECT(Parent)
{
    _FileHandle = Child;
    _wCloseFunction = wCloseFunc;
    _IsHtml = FALSE;
    _lpszUrl = NULL;
    _lpszDirEntry = NULL;
    _Context = dwContext;
    SetObjectType(TypeFtpFileHandle);
}

FTP_FILE_HANDLE_OBJECT::~FTP_FILE_HANDLE_OBJECT(
    VOID
    )
{

    //
    // if local internet handle, closed by local close function
    //

    if (_FileHandle != NULL) {
        _Status = _wCloseFunction(_FileHandle);

        //INET_ASSERT(_Status == ERROR_SUCCESS);

    } else {
        _Status = ERROR_INVALID_HANDLE;
    }

    //
    // clear out any strings we allocated
    //

    if (_lpszUrl != NULL) {
        DEL_STRING(_lpszUrl);
    }
    if (_lpszDirEntry != NULL) {
        DEL_STRING(_lpszDirEntry);
    }
}

HINTERNET
FTP_FILE_HANDLE_OBJECT::GetHandle(
    VOID
    )
{
    return _FileHandle;
}

#ifdef EXTENDED_ERROR_HTML

//
// FTP_ERROR_HANDLE_OBJECT class implementation
//

FTP_ERROR_HANDLE_OBJECT::FTP_ERROR_HANDLE_OBJECT(
    INTERNET_CONNECT_HANDLE_OBJECT* hConnect
    ) : INTERNET_CONNECT_HANDLE_OBJECT(hConnect)
{
    m_lpszErrorText = NULL;
    m_dwErrorTextLength = 0;
    m_QueryBuffer = NULL;
    m_QueryBufferLength = 0;
    m_QueryOffset = 0;
    m_QueryBytesAvailable = 0;
    m_HtmlState = HTML_STATE_START;
    SetObjectType(TypeFtpFileHandle);
    SetErrorText();
}

FTP_ERROR_HANDLE_OBJECT::~FTP_ERROR_HANDLE_OBJECT(
    VOID
    )
{
    //
    // clear out any strings we allocated
    //

    if (m_lpszErrorText != NULL) {
        DEL_STRING(m_lpszErrorText);
    }

    //
    // and the query buffer
    //

    FreeQueryBuffer();
}

DWORD
FTP_ERROR_HANDLE_OBJECT::SetErrorText(
    VOID
    )

/*++

Routine Description:

    Copies last error info to this handle object

Arguments:

    None.

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure -

--*/

{
    INET_ASSERT(m_lpszErrorText == NULL);
    INET_ASSERT(m_dwErrorTextLength == 0);

    DWORD error;
    DWORD category;

    if (!InternetGetLastResponseInfo(&category, NULL, &m_dwErrorTextLength)) {
        if (GetLastError() == ERROR_INSUFFICIENT_BUFFER) {
            m_lpszErrorText = (LPSTR)ALLOCATE_MEMORY(
                                        LMEM_FIXED,
                                        m_dwErrorTextLength
                                        );
            if (m_lpszErrorText != NULL) {
                if (!InternetGetLastResponseInfo(&category,
                                                 m_lpszErrorText,
                                                 &m_dwErrorTextLength)) {
                    m_lpszErrorText[0] = '\0';
                    m_dwErrorTextLength = 0;
                }
                error = ERROR_SUCCESS;
            } else {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }
        }
    }
    return error;
}

DWORD
FTP_ERROR_HANDLE_OBJECT::QueryHtmlDataAvailable(
    OUT LPDWORD lpdwNumberOfBytesAvailable
    )
{
    DWORD error;

    if (m_QueryBuffer != NULL) {
        error = ERROR_SUCCESS;
    } else {
        error = AllocateQueryBuffer();
    }

    INET_ASSERT(m_QueryBytesAvailable == 0);

    if (error == ERROR_SUCCESS) {
        m_QueryOffset = 0;
        if (ReadHtmlUrlData((HINTERNET)this,
                            m_QueryBuffer,
                            m_QueryBufferLength,
                            lpdwNumberOfBytesAvailable
                            )) {
            m_QueryBytesAvailable = *lpdwNumberOfBytesAvailable;
            if (m_QueryBytesAvailable == 0) {
                SetEndOfFile();
            }
        } else {
            error = GetLastError();
        }
    }
    return error;
}

#endif
