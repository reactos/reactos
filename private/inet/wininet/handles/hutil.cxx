/*++

Copyright (c) 1994  Microsoft Corporation

Module Name:

    hutil.cxx

Abstract:

    contains outdated c-c++ interface functions

Author:

    Madan Appiah (madana)  16-Nov-1994

Environment:

    User Mode - Win32

Revision History:

   Sophia Chung (sophiac) 14-Feb-1995 (added FTP and Archie class impl.)
   (code adopted from madana)

--*/

#include <wininetp.h>

DWORD
RIsHandleLocal(
    HINTERNET Handle,
    BOOL * IsLocalHandle,
    BOOL * IsAsyncHandle,
    HINTERNET_HANDLE_TYPE ExpectedHandleType
    )
{
    BOOL fLocalHandle;
    DWORD Error;
    HANDLE_OBJECT *HandleObj;
    HINTERNET InternetHandle;

    HandleObj = (HANDLE_OBJECT *)Handle;

    Error = HandleObj->IsValid(ExpectedHandleType);
    if (Error != ERROR_SUCCESS) {
        goto Cleanup;
    }

    InternetHandle = HandleObj->GetInternetHandle();

#if 0
    if( InternetHandle == INET_INVALID_HANDLE_VALUE ) {
        Error = ERROR_INVALID_HANDLE;
        goto Cleanup;
    }
#endif // 0

    if( InternetHandle == LOCAL_INET_HANDLE ) {
        fLocalHandle = TRUE;
    }
    else {

        //
        // is a remote handle.
        //

        fLocalHandle = FALSE;
    }

    *IsLocalHandle = fLocalHandle;
    *IsAsyncHandle = ((INTERNET_HANDLE_OBJECT *)Handle)->IsAsyncHandle();
    Error = ERROR_SUCCESS;

Cleanup:

    return( Error );
}

DWORD
RGetHandleType(
    HINTERNET Handle,
    LPHINTERNET_HANDLE_TYPE HandleType
    )
{
    HANDLE_OBJECT *HandleObj = (HANDLE_OBJECT *)Handle;
    DWORD error;

    //
    // validate handle before we use it.
    //

    error = HandleObj->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {

        //
        // find the handle type.
        //

        *HandleType = HandleObj->GetHandleType();
    }
    return error;
}

DWORD
RSetHtmlHandleType(
    HINTERNET Handle
    )
{
    HANDLE_OBJECT *HandleObj = (HANDLE_OBJECT *)Handle;
    HINTERNET_HANDLE_TYPE handleType;
    DWORD error;

    //
    // validate handle before we use it.
    //

    error = HandleObj->IsValid(TypeWildHandle);
    if (error != ERROR_SUCCESS) {
        return error;
    }

    //
    // find the handle type.
    //

    handleType = HandleObj->GetHandleType();

    //
    // if the handle type is one of those that can be accessed as RAW data or
    // HTML, then set the HTML flag. HTTP requests are HTML by default. Setting
    // HTML data for any other handle type is an error.
    //
    // Originally, we allowed File and Find handles to be HTML-ised, but we now
    // only allow Find handles: files are returned via the normal mechanism
    //

    if ((handleType == TypeFtpFindHandle)
    || (handleType == TypeGopherFindHandle)) {
        HandleObj->SetHtml();
        return ERROR_SUCCESS;
    } else if (handleType == TypeHttpRequestHandle) {
        return ERROR_SUCCESS;
    } else {
        return ERROR_INTERNET_INVALID_OPERATION;
    }
}

DWORD
RSetHtmlState(
    HINTERNET Handle,
    HTML_STATE State
    )
{
    HANDLE_OBJECT *HandleObj = (HANDLE_OBJECT *)Handle;
    DWORD error;

    //
    // validate handle before we use it.
    //

    error = HandleObj->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {
        HandleObj->SetHtmlState(State);
    }
    return error;
}

DWORD
RGetHtmlState(
    HINTERNET Handle,
    LPHTML_STATE lpState
    )
{
    HANDLE_OBJECT *HandleObj = (HANDLE_OBJECT *)Handle;
    DWORD error;

    //
    // validate handle before we use it.
    //

    error = HandleObj->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {
        *lpState = HandleObj->GetHtmlState();
    }
    return error;
}

DWORD
RSetUrl(
    HINTERNET Handle,
    LPSTR lpszUrl
    )
{
    HANDLE_OBJECT *HandleObj = (HANDLE_OBJECT *)Handle;
    DWORD error;

    //
    // validate handle before we use it.
    //

    error = HandleObj->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {

        LPSTR url;

        url = HandleObj->GetUrl();
        if (url != NULL) {
            DEL_STRING(url);
        }
        if (lpszUrl != NULL) {
            url = NEW_STRING(lpszUrl);
            if (url == NULL) {
                error = ERROR_NOT_ENOUGH_MEMORY;
            }
        } else {
            url = NULL;
        }
        if (error == ERROR_SUCCESS) {
            HandleObj->SetUrl(url);
        }
    }
    return error;
}

DWORD
RGetUrl(
    HINTERNET Handle,
    LPSTR* lpszUrl
    )
{
    HANDLE_OBJECT *HandleObj = (HANDLE_OBJECT *)Handle;
    DWORD error;

    //
    // validate handle before we use it.
    //

    error = HandleObj->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {
        *lpszUrl = HandleObj->GetUrl();
    }
    return error;
}

DWORD
RSetDirEntry(
    HINTERNET Handle,
    LPSTR lpszDirEntry
    )
{
    HANDLE_OBJECT *HandleObj = (HANDLE_OBJECT *)Handle;
    LPSTR dirEntry;
    DWORD error;

    //
    // validate handle before we use it.
    //

    error = HandleObj->IsValid(TypeWildHandle);
    if (error != ERROR_SUCCESS ) {
        return error;
    }

    dirEntry = HandleObj->GetDirEntry();

    if (dirEntry != NULL) {

        //
        // we should not be replacing one non-NULL dir entry with another
        //

        INET_ASSERT(lpszDirEntry == NULL);

        DEL_STRING(dirEntry);
    }

    //
    // make a copy of the string and add it to the object
    //

    if (lpszDirEntry != NULL) {
        dirEntry = NEW_STRING(lpszDirEntry);
        if (dirEntry == NULL) {
            return ERROR_NOT_ENOUGH_MEMORY;
        }
    } else {
        dirEntry = NULL;
    }
    HandleObj->SetDirEntry(dirEntry);
    return ERROR_SUCCESS;
}

DWORD
RGetDirEntry(
    HINTERNET Handle,
    LPSTR* lpszDirEntry
    )
{
    HANDLE_OBJECT *HandleObj = (HANDLE_OBJECT *)Handle;
    DWORD error;

    //
    // validate handle before we use it.
    //

    error = HandleObj->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {
        *lpszDirEntry = HandleObj->GetDirEntry();
    }
    return error;
}

DWORD
RSetParentHandle(
    HINTERNET hChild,
    HINTERNET hParent,
    BOOL DeleteWithChild
    )
{
    DWORD error;

    //
    // ensure the child handle is valid
    //

    error = ((HANDLE_OBJECT*)hChild)->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {

        //
        // and so too the parent
        //

        error = ((HANDLE_OBJECT*)hParent)->IsValid(TypeWildHandle);
        if (error == ERROR_SUCCESS) {

            //
            // make the association
            //

            ((HANDLE_OBJECT*)hChild)->SetParent(hParent, DeleteWithChild);
        }
    }
    return error;
}

DWORD
RGetContext(
    HINTERNET hInternet,
    DWORD_PTR *lpdwContext
    )
{
    DWORD error;

    //
    // ensure the handle is valid
    //

    error = ((HANDLE_OBJECT*)hInternet)->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {
        *lpdwContext = ((INTERNET_HANDLE_OBJECT*)hInternet)->GetContext();
    }
    return error;
}

DWORD
RSetContext(
    HINTERNET hInternet,
    DWORD_PTR dwContext
    )
{
    DWORD error;

    //
    // ensure the handle is valid
    //

    error = ((HANDLE_OBJECT*)hInternet)->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {
        ((INTERNET_HANDLE_OBJECT*)hInternet)->SetContext(dwContext);
    }
    return error;
}

DWORD
RGetTimeout(
    HINTERNET hInternet,
    DWORD dwTimeoutOption,
    LPDWORD lpdwTimeoutValue
    )
{
    DWORD error;

    //
    // ensure the handle is valid
    //

    error = ((HANDLE_OBJECT*)hInternet)->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {
        *lpdwTimeoutValue = ((INTERNET_HANDLE_OBJECT*)hInternet)->GetTimeout(dwTimeoutOption);
    }
    return error;
}

DWORD
RSetTimeout(
    HINTERNET hInternet,
    DWORD dwTimeoutOption,
    DWORD dwTimeoutValue
    )
{
    DWORD error;

    //
    // ensure the handle is valid
    //

    error = ((HANDLE_OBJECT*)hInternet)->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {
        ((INTERNET_HANDLE_OBJECT*)hInternet)->SetTimeout(dwTimeoutOption, dwTimeoutValue);
    }
    return error;
}

DWORD
RGetBufferSize(
    HINTERNET hInternet,
    DWORD dwBufferSizeOption,
    LPDWORD lpdwBufferSize
    )
{
    DWORD error;

    error = ((HANDLE_OBJECT*)hInternet)->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {

        //
        // BUGBUG - this handle must be a connect handle, or object derived from
        //          connect handle
        //

        *lpdwBufferSize = ((INTERNET_CONNECT_HANDLE_OBJECT*)hInternet)
            ->GetBufferSize(dwBufferSizeOption);
    }
    return error;
}

DWORD
RSetBufferSize(
    HINTERNET hInternet,
    DWORD dwBufferSizeOption,
    DWORD dwBufferSize
    )
{
    DWORD error;

    error = ((HANDLE_OBJECT*)hInternet)->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {

        //
        // BUGBUG - this handle must be a connect handle, or object derived from
        //          connect handle
        //

        ((INTERNET_CONNECT_HANDLE_OBJECT*)hInternet)->SetBufferSize(
            dwBufferSizeOption,
            dwBufferSize
            );
    }
    return error;
}

DWORD
RGetStatusCallback(
    IN HINTERNET Handle,
    OUT LPINTERNET_STATUS_CALLBACK lpStatusCallback
    )
{
    //
    // NULL handle should have been caught before we got here
    // (its in InternetQueryOption())
    //

    INET_ASSERT(Handle != NULL);

    *lpStatusCallback = ((INTERNET_HANDLE_OBJECT *)Handle)->GetStatusCallback();
    return ERROR_SUCCESS;
}

DWORD
RExchangeStatusCallback(
    IN HINTERNET Handle,
    IN OUT LPINTERNET_STATUS_CALLBACK lpStatusCallback,
    IN BOOL fType)
{
    DWORD error;

    //
    // NULL handle value should have been caught already
    // (in InternetSetStatusCallback())
    //

    INET_ASSERT(Handle != NULL);

    error = ((HANDLE_OBJECT *)Handle)->IsValid(TypeWildHandle);
    if (error == ERROR_SUCCESS) {
        error = ((INTERNET_HANDLE_OBJECT *)Handle)->
                                ExchangeStatusCallback(lpStatusCallback, fType);
    }
    return error;
}

//DWORD
//RAddAsyncRequest(
//    IN HINTERNET Handle,
//    BOOL fNoCallbackOK
//    )
//{
//    DWORD error;
//
//    if (Handle != NULL) {
//        error = ((HANDLE_OBJECT *)Handle)->IsValid(TypeWildHandle);
//        if (error == ERROR_SUCCESS) {
//            error = ((INTERNET_HANDLE_OBJECT *)Handle)->AddAsyncRequest(fNoCallbackOK);
//        }
//    } else {
//        error = ERROR_INVALID_HANDLE;
//    }
//    return error;
//}
//
//DWORD
//RRemoveAsyncRequest(
//    IN HINTERNET Handle
//    )
//{
//    DWORD error;
//
//    if (Handle != NULL) {
//        error = ((HANDLE_OBJECT *)Handle)->IsValid(TypeWildHandle);
//        if (error == ERROR_SUCCESS) {
//            ((INTERNET_HANDLE_OBJECT *)Handle)->RemoveAsyncRequest();
//        }
//    } else {
//        error = ERROR_INVALID_HANDLE;
//    }
//    return error;
//}

DWORD
RGetLocalHandle(
    HINTERNET Handle,
    HINTERNET *LocalHandle
    )
{
    DWORD Error;
    HANDLE_OBJECT *ObjectHandle = (HANDLE_OBJECT *)Handle;

#if 1
    Error = ObjectHandle->IsValid(TypeWildHandle);
    if (Error != ERROR_SUCCESS) {
        goto Cleanup;
    }
#endif // DBG

    Error = ObjectHandle->GetStatus();

    if( Error != ERROR_SUCCESS ) {
        goto Cleanup;
    }

    *LocalHandle = ObjectHandle->GetHandle();

Cleanup:

    return( Error );
}
