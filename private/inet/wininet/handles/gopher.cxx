/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    gopher.cxx

Abstract:

    Contains methods for GOPHER_FIND_HANDLE_OBJECT and GOPHER_FILE_HANDLE_OBJECT
    classes

    Contents:
        RMakeGfrFindObjectHandle
        RMakeGfrFileObjectHandle
        RMakeGfrFixedObjectHandle

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
RMakeGfrFindObjectHandle(
    IN HINTERNET ParentHandle,
    IN OUT HINTERNET * ChildHandle,
    IN CLOSE_HANDLE_FUNC wCloseFunc,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

Routine Description:

    C-callable wrapper for creating a GOPHER_FIND_HANDLE_OBJECT

Arguments:

    ParentHandle    - mapped address of parent (connect) handle

    ChildHandle     - IN: protocol-specific handle value associated with object
                      OUT: mapped address of GOPHER_FIND_HANDLE_OBJECT

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
    GOPHER_FIND_HANDLE_OBJECT * hFind;

    hFind = new GOPHER_FIND_HANDLE_OBJECT(
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
RMakeGfrFixedObjectHandle(
    IN HINTERNET ParentHandle,
    IN OUT HINTERNET * ChildHandle,
    IN DWORD dwFixedType
    )

/*++

Routine Description:

Routine Description:

    C-callable wrapper for creating a GOPHER_FIND_HANDLE_OBJECT

Arguments:

    ParentHandle    - mapped address of parent (connect) handle

    ChildHandle     - IN: protocol-specific handle value associated with object
                      OUT: mapped address of GOPHER_FIND_HANDLE_OBJECT

Return Value:

    DWORD
        Success - ERROR_SUCCESS

        Failure - ERROR_NOT_ENOUGH_MEMORY

--*/

{
    DWORD error;
    GOPHER_FIND_HANDLE_OBJECT * hFind;

    hFind = new GOPHER_FIND_HANDLE_OBJECT(
                    (INTERNET_CONNECT_HANDLE_OBJECT *)ParentHandle,
                    *ChildHandle,
                    dwFixedType
                    );

    if (!hFind) {
        error = ERROR_NOT_ENOUGH_MEMORY;
    } else {
        error = hFind->GetStatus();
        if (error != ERROR_SUCCESS) {
            delete hFind;
            hFind = NULL;
        }
    }

    *ChildHandle = (HINTERNET)hFind;
    return error;
}



DWORD
RMakeGfrFileObjectHandle(
    IN HINTERNET ParentHandle,
    IN OUT HINTERNET * ChildHandle,
    IN CLOSE_HANDLE_FUNC wCloseFunc,
    IN DWORD_PTR dwContext
    )

/*++

Routine Description:

    C-callable wrapper for creating a GOPHER_FILE_HANDLE_OBJECT

Arguments:

    ParentHandle    - mapped address of parent (connect) handle

    ChildHandle     - IN: protocol-specific handle value associated with object
                      OUT: mapped address of GOPHER_FILE_HANDLE_OBJECT

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
    GOPHER_FILE_HANDLE_OBJECT * hFile;

    hFile = new GOPHER_FILE_HANDLE_OBJECT(
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

//
// GOPHER_FIND_HANDLE_OBJECT class implementation
//

GOPHER_FIND_HANDLE_OBJECT::GOPHER_FIND_HANDLE_OBJECT(
    INTERNET_CONNECT_HANDLE_OBJECT *Parent,
    HINTERNET Child,
    CLOSE_HANDLE_FUNC wCloseFunc,
    DWORD_PTR dwContext
    ) : INTERNET_CONNECT_HANDLE_OBJECT(Parent)
{
    _FindHandle = Child;
    _wCloseFunction = wCloseFunc;
    _IsHtml = FALSE;
    _dwFixedType = 0;
    _lpszUrl = NULL;
    _lpszDirEntry = NULL;
    _QueryBuffer = NULL;
    _QueryBufferLength = 0;
    _QueryOffset = 0;
    _QueryBytesAvailable = 0;
    _Context = dwContext;
    SetObjectType(TypeGopherFindHandle);
}

//
// Constructor for poser find handle to return html form...
//

GOPHER_FIND_HANDLE_OBJECT::GOPHER_FIND_HANDLE_OBJECT(
    INTERNET_CONNECT_HANDLE_OBJECT *Parent,
    HINTERNET Child,
    DWORD dwFixedType
    ) : INTERNET_CONNECT_HANDLE_OBJECT (Parent)
{
    _FindHandle = Child;
    _wCloseFunction = NULL;
    _IsHtml = FALSE;  // must set FALSE so RSetHtmlHandleType can set TRUE!
    _dwFixedType = dwFixedType;
    _lpszUrl = NULL;
    _lpszDirEntry = NULL;
    _QueryBuffer = NULL;
    _QueryBufferLength = 0;
    _QueryOffset = 0;
    _QueryBytesAvailable = 0;
    _Context = 0;
    SetObjectType(TypeGopherFindHandle);
}


GOPHER_FIND_HANDLE_OBJECT::~GOPHER_FIND_HANDLE_OBJECT(
    VOID
    )
{
    //
    // close local handle with appropriate function.
    //

    if (_FindHandle != NULL) {
        _Status = _wCloseFunction(_FindHandle);
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

HINTERNET
GOPHER_FIND_HANDLE_OBJECT::GetHandle(
    VOID
    )
{
    return _FindHandle;
}

DWORD
GOPHER_FIND_HANDLE_OBJECT::QueryHtmlDataAvailable(
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

        DWORD nRead;

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
// GOPHER_FILE_HANDLE_OBJECT class implementation
//

GOPHER_FILE_HANDLE_OBJECT::GOPHER_FILE_HANDLE_OBJECT(
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
    SetObjectType(TypeGopherFileHandle);
}

GOPHER_FILE_HANDLE_OBJECT::~GOPHER_FILE_HANDLE_OBJECT(
    VOID
    )
{
    //
    // close local handle with appropriate function.
    //

    if (_FileHandle != NULL) {
        _Status = _wCloseFunction(_FileHandle);
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
}

HINTERNET
GOPHER_FILE_HANDLE_OBJECT::GetHandle(
    VOID
    )
{
    return _FileHandle;
}
