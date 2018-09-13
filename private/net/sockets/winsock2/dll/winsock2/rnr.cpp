/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    rnr.cpp

Abstract:

    This module contains the implementation of the Registration and
    Name Resolution API for the WinSock2 API

    This module contains the following functions. For functions whose function
    signature contains sting arguments both and ASCII and Wide charater version
    of the function are supplied

    WSAEnumNameSpaceProviders
    WSCEnumNameSpaceProviders
    WSALookupServiceBegin
    WSALookupServiceNext
    WSALookupServiceEnd
    WSASetService
    WSAInstallServiceClass
    WSARemoveServiceClass
    WSAGetServiceClassNameByClassId

Author:

    Dirk Brandewie dirk@mink.intel.com  12-1-1995

[Environment:]

[Notes:]

Revision History:

    12-Dec-1995 dirk@mink.intel.com
        Initial Revision

--*/

#include "precomp.h"

typedef struct
{
    BOOL    AllProviders;
    LPDWORD RequiredBufferSize;
    DWORD   BufferSize;
    PBYTE   Buffer;
    PBYTE   BufferFreePtr;
    BOOL    Ansi;
    INT     NumItemsEnumerated;
    INT     ErrorCode;
} NSCATALOG_ENUMERATION_CONTEXT, * PNSCATALOG_ENUMERATION_CONTEXT;

//
// BONUSSIZE is a hack that is used  to bias the computed size
// when WSALookupServiceNextA gets a WSAEFAULT from the
// WSALookupServiceNextW call. It is the "maximum" padding
// that we might need. Note this assumes all values returned
// and a limit of 3 addresses. There is no way to know exactly
// what the makeup of the returned data will be, so this is
// a "best guess". The right fix is to redo the code to
// pack the result optimally, so no padding is needed.
//

#define BONUSSIZE (3 + 3 + 3 + (3 * 3))

BOOL
CalculateBufferSize(
    PVOID PassBack,
    PNSCATALOGENTRY CatalogEntry
    )
/*++

Routine Description:

    This fuction calculates the size of buffer required to return the
    NAMESPACE_INFO structs for a call to WSAEnumNameSpaces(). This function is
    a callback function used as an argument to the name space catalog
    enumeration funtion

Arguments:

    PassBack - A context value passed thru the catalog enumeration
               function. This passback value is really a pointer to a
               NSCATALOG_ENUMERATION_CONTEXT.

    CatalogEntry - A pointer to the current name space catalog entry to be
                   inspected.
Return Value:

   TRUE, Signalling the catalog enumeration function should continue the
   enumeration.

--*/
{
    PNSCATALOG_ENUMERATION_CONTEXT Context;
    LPWSTR                         EntryDisplayString;

    Context = (PNSCATALOG_ENUMERATION_CONTEXT)PassBack;

    // Add the fixed length of the WSANAMESPACE_INFO struct
    *(Context->RequiredBufferSize) += sizeof(WSANAMESPACE_INFO);

    // Add room for the GUID
    *(Context->RequiredBufferSize) += sizeof(GUID);

    // Add room for the display string
    EntryDisplayString = CatalogEntry->GetProviderDisplayString();
    *(Context->RequiredBufferSize) += ((wcslen(EntryDisplayString)+1) *
                                           sizeof(WCHAR));
    return(TRUE); // Continue the enumeration
}

BOOL
CopyFixedPortionNameSpaceInfo(
    PVOID PassBack,
    PNSCATALOGENTRY CatalogEntry
    )
/*++

Routine Description:

    This Funtion copies the fixed size elements of a NSCATALOGENTRY object into
    a user buffer for return from a call to WSAEnumNameSpaces(). It also
    increments the number of fixed size elements copied so far.

Arguments:

    PassBack - A context value passed thru the catalog enumeration
               function. This passback value is really a pointer to a
               NSCATALOG_ENUMERATION_CONTEXT.

    CatalogEntry - A pointer to the current name space catalog entry to be
                   inspected.

Return Value:

  TRUE, Signalling the catalog enumeration function should continue the
   enumeration.

--*/
{
    PNSCATALOG_ENUMERATION_CONTEXT Context;
    LPWSANAMESPACE_INFOW CurrentNSInfo;

    Context =  (PNSCATALOG_ENUMERATION_CONTEXT)PassBack;

    CurrentNSInfo = (LPWSANAMESPACE_INFOW)Context->BufferFreePtr;
    __try {
        CurrentNSInfo->dwNameSpace  = CatalogEntry->GetNamespaceId();
        CurrentNSInfo->fActive      = CatalogEntry->GetEnabledState();
        CurrentNSInfo->dwVersion    = CatalogEntry->GetVersion();
        CurrentNSInfo->NSProviderId = *(CatalogEntry->GetProviderId());
        Context->BufferFreePtr += sizeof(WSANAMESPACE_INFO);
        Context->NumItemsEnumerated++;
        return(TRUE); // Continue the enumeration
    }
    __except (WS2_EXCEPTION_FILTER()) {
        Context->ErrorCode = WSAEFAULT;
        return(FALSE);
   }


}

BOOL
CopyVariablePortionNameSpaceInfo(
    PVOID PassBack,
    PNSCATALOGENTRY CatalogEntry
    )
/*++

Routine Description:

    This Funtion copies the variable size elements of a NSCATALOGENTRY object
    into a user buffer for return from a call to WSAEnumNameSpaces().

Arguments:

    PassBack - A context value passed thru the catalog enumeration
               function. This passback value is really a pointer to a
               NSCATALOG_ENUMERATION_CONTEXT.

    CatalogEntry - A pointer to the current name space catalog entry to be
                   inspected.

Return Value:

  TRUE, Signalling the catalog enumeration function should continue the
   enumeration.

--*/
{
    PNSCATALOG_ENUMERATION_CONTEXT Context;
    LPWSANAMESPACE_INFOW CurrentNSInfo;
    LPWSTR DisplayString;
    INT    StringLength;

    Context =  (PNSCATALOG_ENUMERATION_CONTEXT)PassBack;

    CurrentNSInfo = (LPWSANAMESPACE_INFOW)Context->Buffer;

    // Copy over the display string
    DisplayString = CatalogEntry->GetProviderDisplayString();
    StringLength = ((wcslen(DisplayString)+1) * sizeof(WCHAR));

    CurrentNSInfo->lpszIdentifier = (LPWSTR)Context->BufferFreePtr;
    __try {
        if (Context->Ansi){
            WideCharToMultiByte(
                     CP_ACP,                                   // CodePage (ANSI)
                     0,                                        // dwFlags
                     DisplayString,                            // lpWideCharStr
                     -1,                                       // cchWideChar
                     (char*)CurrentNSInfo->lpszIdentifier,     // lpMultiByteStr
                     StringLength,                             // cchMultiByte
                     NULL,                                     // lpDefaultChar
                     NULL                                      // lpUsedDefaultChar
                     );
            Context->BufferFreePtr += lstrlen(
                (LPSTR)CurrentNSInfo->lpszIdentifier)+1;
        } //if
        else{
            memcpy(CurrentNSInfo->lpszIdentifier,
                   DisplayString,
                   StringLength);
            Context->BufferFreePtr += StringLength;

        } //else

        // point to the next struct
        Context->Buffer += sizeof(WSANAMESPACE_INFO);
        return(TRUE); // Continue the enumeration
    }
    __except (WS2_EXCEPTION_FILTER()) {
        Context->ErrorCode = WSAEFAULT;
        return FALSE;
    }
}

INT WSAAPI
EnumNameSpaceProviders(
    IN     PNSCATALOG           Catalog,
    IN     BOOL                 Ansi,
    IN OUT LPDWORD              BufferLength,
    IN OUT LPWSANAMESPACE_INFOW Buffer,
    OUT    LPDWORD              ErrorCode
    )
/*++

Routine Description:

    This Function is used by WSAEnumNameSpaceProvidersA and
    WSAEnumNameSpaceProvidersW to fill in the user buffer with the information
    about each name spcae provider install on the system.

Arguments:

    Catalog - A pointer to a NSCATALOG object containing the requested
              information.

    Ansi - A boolean value marking whether the user requested the ansi or
           unicode version of the WSANAMESPACE_INFO struct should be returned.

    BufferLength - The size of the user buffer in bytes.

    Buffer - A pointer to the user buffer.

    ErrorCode - A pointer to a DWORD to contain the error return from this
                function.

Return Value:

    If the function is successful it returns the number of name space providers
    enumerated.   Otherwise it returns SOCKET_ERROR.  If the user buffer is too
    small  to  contain  all  the  the WSANAMESPACE_INFO structs SOCKET_ERROR is
    returned,  the  error code is set to WSAEFAULT, and BufferLength is updated
    to  reflect  the  size  of  buffer  required  to  hold  all  the  requested
    information.
--*/
{
    INT        ReturnCode;
    DWORD      RequiredBufferSize;

    // Setup for early return
    ReturnCode = SOCKET_ERROR;
    *ErrorCode = WSAEFAULT;

    // Find out if the user handed in a big enough buffer
    RequiredBufferSize = 0;
    NSCATALOG_ENUMERATION_CONTEXT Context;

    Context.RequiredBufferSize = &RequiredBufferSize;
    Catalog->EnumerateCatalogItems(
        CalculateBufferSize,
        &Context);

    __try {
        if (Buffer!=NULL && RequiredBufferSize <= *BufferLength){
            Context.BufferSize = *BufferLength;
        }
        else {
            // Error code is set above
            *BufferLength = RequiredBufferSize;
            return (ReturnCode);
        }
    }
    __except (WS2_EXCEPTION_FILTER()) {
        // Everything is set
        return (ReturnCode);
    }

    Context.Buffer = (PBYTE)Buffer;
    Context.BufferFreePtr = (PBYTE)Buffer;
    Context.Ansi = Ansi;
    Context.NumItemsEnumerated = 0;
    Context.ErrorCode = ERROR_SUCCESS;

    //Copy over the fixed part of the WSANAMESPACE_INFO struct(s) into the
    //user buffer
    Catalog->EnumerateCatalogItems(
        CopyFixedPortionNameSpaceInfo,
        &Context);
    if (Context.ErrorCode==ERROR_SUCCESS) {
        //Copy over the variable part of the WSANAMESPACE_INFO struct(s) into
        //the user buffer
         Catalog->EnumerateCatalogItems(
            CopyVariablePortionNameSpaceInfo,
            &Context);
        if (Context.ErrorCode==ERROR_SUCCESS) {
            ReturnCode = Context.NumItemsEnumerated;
        }
        else
            *ErrorCode = Context.ErrorCode;
    }
    else
        *ErrorCode = Context.ErrorCode;

    return(ReturnCode);
}

INT WSAAPI
WSAEnumNameSpaceProvidersA(
    IN OUT LPDWORD              lpdwBufferLength,
    IN OUT LPWSANAMESPACE_INFOA lpnspBuffer
    )
/*++

Routine Description:

    Retrieve information about available name spaces.

Arguments:

    lpdwBufferLength - on input, the number of bytes contained in the buffer
                       pointed to by lpnspBuffer.  On output (if the API fails,
                       and the error is  WSAEFAULT), the minimum number of
                       bytes to pass for the lpnspBuffer to retrieve all the
                       requested information. The passed-in buffer must be
                       sufficient to hold all of the name space information.

    lpnspBuffer - A buffer which is filled with WSANAMESPACE_INFO structures
                  described below.  The returned structures are located
                  consecutively at the head of the buffer. Variable sized
                  information referenced by pointers in the structures point to
                  locations within the buffer located between the end of the
                  fixed sized structures and the end of the buffer.  The number
                  of structures filled in is the return value of
                  WSAEnumNameSpaceProviders().

Return Value:

    WSAEnumNameSpaceProviders() returns the number of WSANAMESPACE_INFO
    structures copied into lpnspBuffer. Otherwise the value SOCKET_ERROR is
    returned, and a specific error number may be retrieved by calling
    WSAGetLastError().

--*/
{
    INT        ReturnValue;
    PDPROCESS  Process;
    PDTHREAD   Thread;
    INT        ErrorCode;
    PNSCATALOG Catalog;

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if


    Catalog = Process->GetNamespaceCatalog();

    ReturnValue = EnumNameSpaceProviders(
        Catalog,
        TRUE,    // Ansi
        lpdwBufferLength,
        (LPWSANAMESPACE_INFOW)lpnspBuffer,
        (LPDWORD)&ErrorCode);

    // If there was an error set this threads lasterror
    if (SOCKET_ERROR == ReturnValue ) {
        SetLastError(ErrorCode);
    } //if
    return(ReturnValue);
}

INT WSAAPI
WSAEnumNameSpaceProvidersW(
    IN OUT LPDWORD              lpdwBufferLength,
    IN OUT LPWSANAMESPACE_INFOW lpnspBuffer
    )
/*++

Routine Description:

    Retrieve information about available name spaces.

Arguments:

    lpdwBufferLength - on input, the number of bytes contained in the buffer
                       pointed to by lpnspBuffer.  On output (if the API fails,
                       and the error is  WSAEFAULT), the minimum number of
                       bytes to pass for the lpnspBuffer to retrieve all the
                       requested information. The passed-in buffer must be
                       sufficient to hold all of the name space information.

    lpnspBuffer - A buffer which is filled with WSANAMESPACE_INFO structures
                  described below.  The returned structures are located
                  consecutively at the head of the buffer. Variable sized
                  information referenced by pointers in the structures point to
                  locations within the buffer located between the end of the
                  fixed sized structures and the end of the buffer.  The number
                  of structures filled in is the return value of
                  WSAEnumNameSpaceProviders().

Return Value:

    WSAEnumNameSpaceProviders() returns the number of WSANAMESPACE_INFO
    structures copied into lpnspBuffer. Otherwise the value SOCKET_ERROR is
    returned, and a specific error number may be retrieved by calling
    WSAGetLastError().

--*/
{
    INT        ReturnValue;
    PDPROCESS  Process;
    PDTHREAD   Thread;
    INT        ErrorCode;
    PNSCATALOG Catalog;

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    Catalog = Process->GetNamespaceCatalog();

    ReturnValue = EnumNameSpaceProviders(
        Catalog,
        FALSE,    //Unicode
        lpdwBufferLength,
        lpnspBuffer,
        (LPDWORD)&ErrorCode);

    // If there was an error set this threads lasterror
    if (SOCKET_ERROR == ReturnValue ) {
        SetLastError(ErrorCode);
    } //if
    return(ReturnValue);
}



INT WSAAPI
WSALookupServiceBeginA(
    IN  LPWSAQUERYSETA lpqsRestrictions,
    IN  DWORD          dwControlFlags,
    OUT LPHANDLE       lphLookup
    )
/*++
Routine Description:
    WSALookupServiceBegin() is used to initiate a client query that is
    constrained by the information contained within a WSAQUERYSET
    structure. WSALookupServiceBegin() only returns a handle, which should be
    used by subsequent calls to WSALookupServiceNext() to get the actual
    results.

Arguments:
    lpqsRestrictions - contains the search criteria.

    dwControlFlags - controls the depth of the search.

    lphLookup - A pointer Handle to be used when calling WSALookupServiceNext
                in order to start retrieving the results set.
Returns:
    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.

--*/
{
    INT            ReturnCode;
    LPWSAQUERYSETW UniCodeBuffer;
    DWORD          UniCodeBufferSize;

    //
    // The Winsock spec says that these are ignored fields, clear them
    // so that they don't cause a problem in the Ansi to Unicode copy
    // routines - NT bug #91655
    //
    lpqsRestrictions->dwOutputFlags = 0;
    lpqsRestrictions->lpszComment = NULL;
    lpqsRestrictions->dwNumberOfCsAddrs = 0;
    lpqsRestrictions->lpcsaBuffer;

    //
    // Verify that pointer is valid (lphLookup verified in XxxW func)
    //

    if (IsBadReadPtr (lpqsRestrictions, sizeof(*lpqsRestrictions)))
    {
        SetLastError (WSAEFAULT);
        return SOCKET_ERROR;
    }

    // Find how big a buffer we need to allocate
    UniCodeBuffer = NULL;
    UniCodeBufferSize = 0;
    ReturnCode = MapAnsiQuerySetToUnicode(
        lpqsRestrictions,
        &UniCodeBufferSize,
        UniCodeBuffer);
    if (WSAEFAULT == ReturnCode){
        //Get a buffer
        UniCodeBuffer = (LPWSAQUERYSETW)new BYTE[UniCodeBufferSize];
        if (UniCodeBuffer){
            ReturnCode = MapAnsiQuerySetToUnicode(
                lpqsRestrictions,
                &UniCodeBufferSize,
                UniCodeBuffer);
            if (ERROR_SUCCESS == ReturnCode){
                ReturnCode = WSALookupServiceBeginW(
                    UniCodeBuffer,
                    dwControlFlags,
                    lphLookup);
            } //if
            else{
                SetLastError(ReturnCode);
                ReturnCode=SOCKET_ERROR;
            } //else
            delete(UniCodeBuffer);
        } //if
        else{
            SetLastError(WSAEFAULT);
            ReturnCode = SOCKET_ERROR;
        } //else
    } //if
    else{
        SetLastError(ReturnCode);
        ReturnCode=SOCKET_ERROR;
    } //else
    return(ReturnCode);
}

INT WSAAPI
WSALookupServiceBeginW(
    IN  LPWSAQUERYSETW lpqsRestrictions,
    IN  DWORD          dwControlFlags,
    OUT LPHANDLE       lphLookup
    )
/*++
Routine Description:
    WSALookupServiceBegin() is used to initiate a client query that is
    constrained by the information contained within a WSAQUERYSET
    structure. WSALookupServiceBegin() only returns a handle, which should be
    used by subsequent calls to WSALookupServiceNext() to get the actual
    results.

Arguments:
    lpqsRestrictions - contains the search criteria.

    dwControlFlags - controls the depth of the search.

    lphLookup - A pointer Handle to be used when calling WSALookupServiceNext
                in order to start retrieving the results set.
Returns:
    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.

--*/
{
    INT       ReturnValue;
    PDPROCESS Process;
    PDTHREAD  Thread;
    INT       ErrorCode;
    PNSQUERY  Query;

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    //
    // Verify that pointers are valid
    //

    if (IsBadWritePtr (lphLookup, sizeof(*lphLookup)) ||
        IsBadReadPtr (lpqsRestrictions, sizeof(*lpqsRestrictions)))
    {
        SetLastError (WSAEFAULT);
        return SOCKET_ERROR;
    }

    //
    // Make sure we've got a current name space catalog
    //

    Query = new NSQUERY;
    if (Query){
        Query->Initialize();
        ReturnValue = Query->LookupServiceBegin(
            lpqsRestrictions,
            dwControlFlags,
            Process->GetNamespaceCatalog());
        if (ERROR_SUCCESS == ReturnValue){
            *lphLookup = (LPHANDLE)Query;
        } //if
        else{
            *lphLookup = NULL;
            delete Query;
        } //else
    } //if
    else {
        ReturnValue = SOCKET_ERROR;
        SetLastError (WSAENOBUFS);
    }

    return(ReturnValue);
}


INT WSAAPI
WSALookupServiceNextA(
    IN     HANDLE           hLookup,
    IN     DWORD            dwControlFlags,
    IN OUT LPDWORD          lpdwBufferLength,
    OUT    LPWSAQUERYSETA   lpqsResults
    )
/*++
Routine Description:
    WSALookupServiceNext() is called after obtaining a Handle from a previous
    call to WSALookupServiceBegin() in order to retrieve the requested service
    information.  The provider will pass back a WSAQUERYSET structure in the
    lpqsResults buffer.  The client should continue to call this API until it
    returns WSA_E_NOMORE, indicating that all of the WSAQUERYSET have been
    returned.

Arguments:
    hLookup - A Handle returned from the previous call to
              WSALookupServiceBegin().

    dwControlFlags - Flags to control the next operation.  This is currently
                     used to indicate to the provider what to do if the result
                     set is too big for the buffer.  If on the previous call to
                     WSALookupServiceNext() the result set was too large for
                     the buffer, the application can choose to do one of two
                     things on this call.  First, it can choose to pass a
                     bigger buffer and try again.  Second, if it cannot or is
                     unwilling to allocate a larger buffer, it can pass
                     LUP_FLUSHPREVIOUS to tell the provider to throw away the
                     last result set - which was too large - and move on to the
                     next set for this call.

    lpdwBufferLength - on input, the number of bytes contained in the buffer
                       pointed  to by lpresResults.  On output - if the API
                       fails, and the error is WSAEFAULT, then it contains the
                       minimum number of bytes to pass for the lpqsResults to
                       retrieve the record.

    lpqsResults - a pointer to a block of memory, which will contain one result
                  set in a WSAQUERYSET structure on return.


Returns:
    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.
--*/
{
    INT            ReturnCode;
    DWORD          ErrorCode;
    LPWSAQUERYSETW UniCodeBuffer;
    DWORD          UniCodeBufferLength;

    //
    // Verify that pointers are valid
    //

    if (IsBadReadPtr (lpdwBufferLength, sizeof(*lpdwBufferLength)) ||
        (*lpdwBufferLength != 0  &&
            IsBadWritePtr (lpqsResults, *lpdwBufferLength)))
    {
        SetLastError (WSAEFAULT);
        return SOCKET_ERROR;
    }

    // Find how big a buffer we need to allocate. Base first guess on the
    // user's provided buffer. The alogirthm is as follows:
    // If the user supplied a buffer, allocate a buffer of size
    // (user buffer - sizeof(WSAQUERYSET) * sizeof(WCHAR). This
    //  is guaranteed to hold the data that could be held in
    // the user's buffer.

    UniCodeBufferLength = *lpdwBufferLength;
    if(UniCodeBufferLength >= sizeof(WSAQUERYSETW))
    {
        // Assume all space, except the defined structure, is to
        // be string space. So scale it by the size of a UNICODE
        // character. It won't be that bad, but this seems "safe".
        //
        // UniCodeBufferLength = 
                              // BUGBUG - This calculation doesn't work out
                              // correctly. Just use the size that the caller
                              // is trying to use.
                              // (UniCodeBufferLength * sizeof(WCHAR)) -
                              //    sizeof(WSAQUERYSETW);
        UniCodeBuffer = (LPWSAQUERYSETW) new BYTE[UniCodeBufferLength];
        if(!UniCodeBuffer)
        {
            UniCodeBufferLength = 0;        // memory allocation failure
        }
    }
    else
    {
        UniCodeBuffer = 0;
        UniCodeBufferLength = 0;
    }

    ReturnCode = WSALookupServiceNextW(
        hLookup,
        dwControlFlags,
        &UniCodeBufferLength,
        UniCodeBuffer);

    //
    // if the call did not supply a buffer, the user does have a buffer,
    // and it the call failed, do it again. This should never happen,
    // and if it does things are very odd, but account for it nonetheless.
    //
    if(!UniCodeBuffer
              &&
       (*lpdwBufferLength >= sizeof(WSAQUERYSET))
              &&
       (ReturnCode == SOCKET_ERROR))
    {
        ErrorCode = GetLastError();
        if (WSAEFAULT == ErrorCode)
        {
            //
            // delete old buffer, if any, and get a new buffer of the
            // proper size.
            //
            delete  (PBYTE)UniCodeBuffer;

            UniCodeBuffer = (LPWSAQUERYSETW) new BYTE[UniCodeBufferLength];

            //
            // if a buffer is allocated, call the provider again. Else,
            // return the EFAULT and the buffer size to the
            // caller to handle it.
            //
            if (UniCodeBuffer){
                ReturnCode = WSALookupServiceNextW(
                    hLookup,
                    dwControlFlags,
                    &UniCodeBufferLength,
                    UniCodeBuffer);
            }
        }
    }
    //
    // Either it worked, in which case UniCodeBuffer contains the results,
    // or it didn't work for one of the above branches.
    //
    if (ERROR_SUCCESS == ReturnCode)
    {
        ReturnCode = MapUnicodeQuerySetToAnsi(
                        UniCodeBuffer,
                        lpdwBufferLength,
                        lpqsResults);
        if (ERROR_SUCCESS != ReturnCode)
        {
            SetLastError(ReturnCode);
            ReturnCode=SOCKET_ERROR;
        } //if
    } //if
    else
    {
        if(GetLastError() == WSAEFAULT)
        {
            *lpdwBufferLength = UniCodeBufferLength + BONUSSIZE;
        }
    }

    if (UniCodeBuffer!=0) {
        delete (PBYTE)UniCodeBuffer;
    }
    return(ReturnCode);
}

INT WSAAPI
WSALookupServiceNextW(
    IN     HANDLE           hLookup,
    IN     DWORD            dwControlFlags,
    IN OUT LPDWORD          lpdwBufferLength,
    OUT    LPWSAQUERYSETW   lpqsResults
    )
/*++
Routine Description:
    WSALookupServiceNext() is called after obtaining a Handle from a previous
    call to WSALookupServiceBegin() in order to retrieve the requested service
    information.  The provider will pass back a WSAQUERYSET structure in the
    lpqsResults buffer.  The client should continue to call this API until it
    returns WSA_E_NOMORE, indicating that all of the WSAQUERYSET have been
    returned.

Arguments:
    hLookup - A Handle returned from the previous call to
              WSALookupServiceBegin().

    dwControlFlags - Flags to control the next operation.  This is currently
                     used to indicate to the provider what to do if the result
                     set is too big for the buffer.  If on the previous call to
                     WSALookupServiceNext() the result set was too large for
                     the buffer, the application can choose to do one of two
                     things on this call.  First, it can choose to pass a
                     bigger buffer and try again.  Second, if it cannot or is
                     unwilling to allocate a larger buffer, it can pass
                     LUP_FLUSHPREVIOUS to tell the provider to throw away the
                     last result set - which was too large - and move on to the
                     next set for this call.

    lpdwBufferLength - on input, the number of bytes contained in the buffer
                       pointed  to by lpresResults.  On output - if the API
                       fails, and the error is WSAEFAULT, then it contains the
                       minimum number of bytes to pass for the lpqsResults to
                       retrieve the record.

    lpqsResults - a pointer to a block of memory, which will contain one result
                  set in a WSAQUERYSET structure on return.


Returns:
    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.
--*/
{
    INT       ReturnValue;
    PDPROCESS Process;
    PDTHREAD  Thread;
    INT       ErrorCode;
    PNSQUERY  Query;

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    //
    // Verify that pointers are valid
    //

    if (IsBadReadPtr (lpdwBufferLength, sizeof(*lpdwBufferLength)) ||
        (*lpdwBufferLength != 0  &&
            IsBadWritePtr (lpqsResults, *lpdwBufferLength)))
    {
        SetLastError (WSAEFAULT);
        return SOCKET_ERROR;
    }

    if (!hLookup){
        SetLastError(WSA_INVALID_HANDLE);
        return(SOCKET_ERROR);
    } //if

    Query = (PNSQUERY) hLookup;
    if (!Query->IsValid()){
        SetLastError(WSA_INVALID_HANDLE);
        return(SOCKET_ERROR);
    } //if

    ReturnValue = Query->LookupServiceNext(
        dwControlFlags,
        lpdwBufferLength,
        lpqsResults);

    if (SOCKET_ERROR == ReturnValue){
        if (Query->IsDeletable()){
            delete Query;
        } //if
    } //if
    return(ReturnValue);
}



INT WSAAPI
WSALookupServiceEnd(
    IN HANDLE  hLookup
    )
/*++
Routine Description:
    WSALookupServiceEnd() is called to free the handle after previous calls to
    WSALookupServiceBegin() and WSALookupServiceNext().  Note that if you call
    WSALookupServiceEnd() from another thread while an existing
    WSALookupServiceNext() is blocked, then the end call will have the same
    effect as a cancel, and will cause the WSALookupServiceNext() call to
    return immediately.

Arguments:
    hLookup - Handle previously obtained by calling WSALookupServiceBegin().

Returns:
    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.
--*/
{
    INT       ReturnValue;
    PDPROCESS Process;
    PDTHREAD  Thread;
    INT       ErrorCode;
    PNSQUERY  Query;

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

     if (!hLookup){
        SetLastError(WSA_INVALID_HANDLE);
        return(SOCKET_ERROR);
    } //if

    Query = (PNSQUERY) hLookup;
    if (!Query->IsValid()){
        SetLastError(WSA_INVALID_HANDLE);
        return(SOCKET_ERROR);
    } //if

    ReturnValue = Query->LookupServiceEnd();

    if (Query->IsDeletable()){
        delete Query;
    } //

    //
    // BUGBUG Why ?
    //
    return(NO_ERROR);

}


INT WSAAPI
WSASetServiceA(
    IN  LPWSAQUERYSETA    lpqsRegInfo,
    IN  WSAESETSERVICEOP  essOperation,
    IN  DWORD             dwControlFlags
    )
/*++
Routine Description:
    WSASetService() is used to register or deregister a service instance within
    one or more name spaces.  This function may be used to affect a specific
    name space provider, all providers associated with a specific name space,
    or all providers across all name spaces.
Arguments:
    lpqsRegInfo - specifies service information for registration, identifies
                  service for deregistration.

    essOperation - an enumeration whose values include:
        REGISTER register the service.  For SAP, this means sending out a
        periodic broadcast.  This is a NOP for the DNS name space.  For
        persistent data stores this means updating the address information.

        DEREGISTER deregister the service.  For SAP, this means stop sending
        out the periodic broadcast.  This is a NOP for the DNS name space.  For
        persistent data stores this means deleting address information.

        FLUSH used to initiate the registration requests that have previously
        occurred.

    dwControlFlags - The meaning of dwControlFlags is dependent on the value of
    essOperation as follows:

        essOperation    dwControlFlags    Meaning
        REGISTER        SERVICE_DEFER     delay the request (use FLUSH to
                                          subsequently issue the request)
                        SERVICE_HARD      send the request immediately.
                        SERVICE_MULTIPLE  the registering service can be
                                          represented by multiple instances.
        DEREGISTER      SERVICE_HARD      remove all knowledge of the object
                                          within the name space.
Returns:
    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.
--*/
{
    INT            ReturnCode;
    DWORD          ErrorCode;
    LPWSAQUERYSETW UniCodeBuffer;
    DWORD          UniCodeBufferSize;

    ReturnCode = SOCKET_ERROR;
    UniCodeBuffer = NULL;
    UniCodeBufferSize = 0;

    if ( !lpqsRegInfo )
    {
        SetLastError (WSAEFAULT);
        return SOCKET_ERROR;
    }

    //find out how big a buffer we need
    ErrorCode = MapAnsiQuerySetToUnicode(
        lpqsRegInfo,
        &UniCodeBufferSize,
        UniCodeBuffer);
    if (WSAEFAULT == ErrorCode){
        UniCodeBuffer = (LPWSAQUERYSETW) new BYTE[UniCodeBufferSize];
        if (UniCodeBuffer){
            ErrorCode = MapAnsiQuerySetToUnicode(
                lpqsRegInfo,
                &UniCodeBufferSize,
                UniCodeBuffer);
            if (ERROR_SUCCESS == ErrorCode){
                ReturnCode = WSASetServiceW(
                    UniCodeBuffer,
                    essOperation,
                    dwControlFlags);
            } //if
            delete UniCodeBuffer;
        } //if
    } //if
    return(ReturnCode);
}


typedef class NSCATALOGENTRYSTATE *PNSCATALOGENTRYSTATE;
class NSCATALOGENTRYSTATE {
public:
    NSCATALOGENTRYSTATE();

    INT
    Initialize(
        PNSCATALOGENTRY  CatalogEntry
        );

    PNSPROVIDER
    GetProvider(
        IN  PNSCATALOG    Catalog
        );

    ~NSCATALOGENTRYSTATE();

    LIST_ENTRY   m_context_linkage;
    //Public data member to support putting this object on a linked list
private:
    PNSCATALOGENTRY  m_catalog_entry;
    // Pointer to the NSCATALOGENTRY object associated with this boject.
}; // NSCATALOGENTRYSTATE

inline
NSCATALOGENTRYSTATE::NSCATALOGENTRYSTATE()
/*++

Routine Description:

    Constructor for the NSCATALOGENTRYSTATE object.  The first member function
    called after this must be Initialize.

Arguments:

    None

Return Value:

    Returns a pointer to a NSCATALOGENTRYSTATE object.
--*/
{
    m_catalog_entry = NULL;
}

inline
INT
NSCATALOGENTRYSTATE::Initialize(
    PNSCATALOGENTRY  CatalogEntry
    )
/*++

Routine Description:

    This  procedure  performs  all initialization for the NSCATALOGENTRYSTATE
    object.  This function  must  be  invoked  after the constructor, before
    any other member function is invoked.

Arguments:

    CatalogEntry - A pointer to a namespace catalog entry object.

Return Value:

    If  the  function  is  successful,  it  returns ERROR_SUCCESS, otherwise it
    returns an appropriate WinSock 2 error code.
--*/
{
    assert (m_catalog_entry==NULL);
    CatalogEntry->Reference ();
    m_catalog_entry = CatalogEntry;
    return(ERROR_SUCCESS);
}

PNSPROVIDER
NSCATALOGENTRYSTATE::GetProvider(
    IN  PNSCATALOG    Catalog
    )
/*++

Routine Description:

    Returns provider object associated with this object
    Loads it if necessary

Arguments:

    None

Return Value:

    NS provider object
--*/
{
    PNSPROVIDER     Provider;
    Provider = m_catalog_entry->GetProvider ();
    if (Provider==NULL) {
        INT ErrorCode = Catalog->LoadProvider (m_catalog_entry);
        if (ErrorCode==ERROR_SUCCESS) {
            Provider = m_catalog_entry->GetProvider ();
            assert (Provider!=NULL);
        }
    }

    return Provider;
}

inline
NSCATALOGENTRYSTATE::~NSCATALOGENTRYSTATE()
/*++

Routine Description:

    Denstructor for the NSCATALOGENTRYSTATE object. 

Arguments:

    None

Return Value:

    None
--*/
{
    if (m_catalog_entry!=NULL) {
        m_catalog_entry->Dereference ();
        m_catalog_entry = NULL;
    }
}



typedef struct _MATCH_PROVIDERS_CONTEXT {
    IN  BOOL            UseGuid;
    union {
        IN  GUID        ProviderId;
        IN  DWORD       NameSpaceId;
    };
    OUT LIST_ENTRY      EntryList;
    OUT INT             ErrorCode;
} MATCH_PROVIDERS_CONTEXT, * PMATCH_PROVIDERS_CONTEXT;

BOOL
MatchProviders(
    IN PVOID                PassBack,
    IN PNSCATALOGENTRY      CatalogEntry
    )
/*++

Routine Description:

    This function is the enumeration procedure passed to EnumerateCatalogItems
    in a call to WSASetServiceW(). This function inspects the current catalog
    item to see if it meets the selection criteria contained in the context
    value passed back from EnumerateCatalogItems(). 

Arguments:

    PassBack - The context value passed to EnumerateCatalogItems().

    CatalogEntry - A pointer to a NSCATALOGENTRY object.

Return Value:

    TRUE if the enumeration should be continued else FALSE.

--*/
{
    PMATCH_PROVIDERS_CONTEXT Context;
    BOOLEAN ContinueEnumeration = TRUE;
    BOOLEAN UseThisProvider = FALSE;

    Context = (PMATCH_PROVIDERS_CONTEXT)PassBack;
    if (Context->UseGuid) {
        if (Context->ProviderId==*(CatalogEntry->GetProviderId())) {
            UseThisProvider = TRUE;
        }
    }
    else {
        if (CatalogEntry->GetEnabledState () &&
                ((Context->NameSpaceId==CatalogEntry->GetNamespaceId()) ||
                    Context->NameSpaceId==NS_ALL)) {
            UseThisProvider = TRUE;
        }
    }

    if (UseThisProvider) {
        PNSCATALOGENTRYSTATE    EntryState;

        EntryState = new NSCATALOGENTRYSTATE;
        if (EntryState!=NULL) {
            EntryState->Initialize (CatalogEntry);
            InsertTailList (&Context->EntryList,
                                &EntryState->m_context_linkage);
        }
        else {
            Context->ErrorCode = WSA_NOT_ENOUGH_MEMORY;
            ContinueEnumeration = FALSE;
        }
    }

    return ContinueEnumeration;
}

INT WSAAPI
WSASetServiceW(
    IN  LPWSAQUERYSETW    lpqsRegInfo,
    IN  WSAESETSERVICEOP  essOperation,
    IN  DWORD             dwControlFlags
    )
/*++
Routine Description:
    WSASetService() is used to register or deregister a service instance within
    one or more name spaces.  This function may be used to affect a specific
    name space provider, all providers associated with a specific name space,
    or all providers across all name spaces.
Arguments:
    lpqsRegInfo - specifies service information for registration, identifies
                  service for deregistration.

    essOperation - an enumeration whose values include:
        REGISTER register the service.  For SAP, this means sending out a
        periodic broadcast.  This is a NOP for the DNS name space.  For
        persistent data stores this means updating the address information.

        DEREGISTER deregister the service.  For SAP, this means stop sending
        out the periodic broadcast.  This is a NOP for the DNS name space.  For
        persistent data stores this means deleting address information.

        FLUSH used to initiate the registration requests that have previously
        occurred.

    dwControlFlags - The meaning of dwControlFlags is dependent on the value of
    essOperation as follows:

        essOperation    dwControlFlags    Meaning
        REGISTER        SERVICE_DEFER     delay the request (use FLUSH to
                                          subsequently issue the request)
                        SERVICE_HARD      send the request immediately.
                        SERVICE_MULTIPLE  the registering service can be
                                          represented by multiple instances.
        DEREGISTER      SERVICE_HARD      remove all knowledge of the object
                                          within the name space.
Returns:
    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.
--*/
{
    PDPROCESS          Process;
    PDTHREAD           Thread;
    INT                ErrorCode;
    PNSCATALOG         Catalog;
    MATCH_PROVIDERS_CONTEXT Context;

    if ( !lpqsRegInfo )
    {
        SetLastError (WSAEFAULT);
        return SOCKET_ERROR;
    }

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    Catalog = Process->GetNamespaceCatalog();

    __try {
        if (lpqsRegInfo->lpNSProviderId!=NULL) {
            Context.ProviderId = *(lpqsRegInfo->lpNSProviderId);
            Context.UseGuid = TRUE;
        }
        else {
            Context.NameSpaceId = lpqsRegInfo->dwNameSpace;
            Context.UseGuid = FALSE;
        }
    }
    __except (WS2_EXCEPTION_FILTER()) {
        SetLastError (WSAEFAULT);
        return(SOCKET_ERROR);
    }

    InitializeListHead (&Context.EntryList);
    Context.ErrorCode = ERROR_SUCCESS;

    Catalog->EnumerateCatalogItems(
        MatchProviders,
        &Context);

    if (Context.ErrorCode == ERROR_SUCCESS) {
        ErrorCode = NO_DATA;
        while (!IsListEmpty (&Context.EntryList)) {
            PNSCATALOGENTRYSTATE    EntryState;
            PLIST_ENTRY             ListItem;
            PNSPROVIDER             Provider;
            ListItem = RemoveHeadList (&Context.EntryList);
            EntryState = CONTAINING_RECORD (ListItem,
                                                NSCATALOGENTRYSTATE,
                                                m_context_linkage
                                                );
            Provider = EntryState->GetProvider (Catalog);
            if (Provider!=NULL) {
                if (Provider->NSPSetService(
                                        NULL, // lpServiceClassInfo
                                        lpqsRegInfo,
                                        essOperation,
                                        dwControlFlags)==ERROR_SUCCESS) {
                    ErrorCode = ERROR_SUCCESS;
                }
                else {
                    if (ErrorCode!=ERROR_SUCCESS) {
                        ErrorCode = GetLastError ();
                        //
                        // Reset error code if provider fails
                        // to set last error for some reason
                        //
                        if (ErrorCode==ERROR_SUCCESS)
                            ErrorCode = NO_DATA;
                    }
                }
            }
            delete EntryState;
        }
    }
    else
        ErrorCode = Context.ErrorCode;

    if (ErrorCode == ERROR_SUCCESS) {
        return (ERROR_SUCCESS);
    }
    else {
        SetLastError(ErrorCode);
        return (SOCKET_ERROR);
    }
}


INT WSAAPI
WSAInstallServiceClassA(
    IN  LPWSASERVICECLASSINFOA   lpServiceClassInfo
    )
/*++
Routine Description:
    WSAInstallServiceClass() is used to register a service class schema within
    a name space. This schema includes the class name, class id, and any name
    space specific information that is common to all instances of the service,
    such as the SAP ID or object ID.

Arguments:
    lpServiceClasslnfo - contains service class to name space specific type
                         mapping information.  Multiple mappings can be handled
                         at one time.
Returns:
    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.
--*/
{
    LPWSASERVICECLASSINFOW WideServiceClassInfo;
    DWORD WideServiceClassInfoSize;
    DWORD ErrorCode;
    INT   ReturnCode;

    if ( !lpServiceClassInfo ) {
        SetLastError (WSAEINVAL);
        return SOCKET_ERROR;
    }

    WideServiceClassInfo = NULL;
    WideServiceClassInfoSize = 0;
    ReturnCode = SOCKET_ERROR;

    //Find the size of buffer we are going to need
    ErrorCode = MapAnsiServiceClassInfoToUnicode(
        lpServiceClassInfo,
        &WideServiceClassInfoSize,
        WideServiceClassInfo);

    if (WSAEFAULT == ErrorCode){
        WideServiceClassInfo = (LPWSASERVICECLASSINFOW)
            new BYTE[WideServiceClassInfoSize];
        if (WideServiceClassInfo){
            ErrorCode = MapAnsiServiceClassInfoToUnicode(
                lpServiceClassInfo,
                &WideServiceClassInfoSize,
                WideServiceClassInfo);
            if (ERROR_SUCCESS == ErrorCode){
                ReturnCode = WSAInstallServiceClassW(
                    WideServiceClassInfo);
            } //if
            delete WideServiceClassInfo;
        } //if
    } //if
    else{
        SetLastError(ErrorCode);
    } //else
    return(ReturnCode);

}

INT WSAAPI
WSAInstallServiceClassW(
    IN  LPWSASERVICECLASSINFOW   lpServiceClassInfo
    )
/*++
Routine Description:
    WSAInstallServiceClass() is used to register a service class schema within
    a name space. This schema includes the class name, class id, and any name
    space specific information that is common to all instances of the service,
    such as the SAP ID or object ID.

Arguments:
    lpServiceClasslnfo - contains service class to name space specific type
                         mapping information.  Multiple mappings can be handled
                         at one time.
Returns:
    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.
--*/
{
    PDPROCESS       Process;
    PDTHREAD        Thread;
    INT             ErrorCode;
    PNSCATALOG      Catalog;
    MATCH_PROVIDERS_CONTEXT Context;

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    if ( !lpServiceClassInfo ) {
        SetLastError(WSAEINVAL);
        return(SOCKET_ERROR);
    }

    Catalog = Process->GetNamespaceCatalog();

    //
    // Specifying all namespaces gives us all enabled providers
    // which is exactly what we want
    //

    Context.NameSpaceId = NS_ALL;
    Context.UseGuid = FALSE;
    InitializeListHead (&Context.EntryList);
    Context.ErrorCode = ERROR_SUCCESS;

    Catalog->EnumerateCatalogItems(
        MatchProviders,
        &Context);

    if (Context.ErrorCode == ERROR_SUCCESS)
    {
        ErrorCode = NO_DATA;

        while (!IsListEmpty (&Context.EntryList))
        {
            PNSCATALOGENTRYSTATE    EntryState;
            PLIST_ENTRY             ListItem;
            PNSPROVIDER             Provider;

            ListItem = RemoveHeadList (&Context.EntryList);
            EntryState = CONTAINING_RECORD (ListItem,
                                                NSCATALOGENTRYSTATE,
                                                m_context_linkage
                                                );

            Provider = EntryState->GetProvider (Catalog);

            if (Provider!=NULL)
            {
                INT Error = Provider->NSPInstallServiceClass(
                                         lpServiceClassInfo);

                if (!Error)
                {
                    ErrorCode = ERROR_SUCCESS;
                }
                else
                {
                    if (ErrorCode)
                    {
                        ErrorCode = GetLastError();
                        //
                        // Reset error code if provider fails
                        // to set last error for some reason
                        //
                        if (ErrorCode==ERROR_SUCCESS)
                            ErrorCode = NO_DATA;
                    }
                }
            }
            delete EntryState;
        }
    }
    else
        ErrorCode = Context.ErrorCode;

    if (ErrorCode == ERROR_SUCCESS)
    {
        return (ERROR_SUCCESS);
    }
    else
    {
        SetLastError(ErrorCode);
        return (SOCKET_ERROR);
    }
}


INT WSAAPI
WSARemoveServiceClass(
    IN  LPGUID  lpServiceClassId
    )
/*++
Routine Description:
    WSARemoveServiceClass() is used to permanently unregister service class
    schema.
Arguments:
    lpServiceClassId - Pointer to the service class GUID that you wish to
                       remove.
Returns:
    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.

--*/
{
    PDPROCESS       Process;
    PDTHREAD        Thread;
    INT             ErrorCode;
    PNSCATALOG      Catalog;
    MATCH_PROVIDERS_CONTEXT Context;

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS)
    {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    if ( !lpServiceClassId )
    {
        SetLastError(WSAEINVAL);
        return(SOCKET_ERROR);
    }

    Catalog = Process->GetNamespaceCatalog();
    //
    // Specifying all namespaces gives us all enabled providers
    // which is exactly what we want
    //

    Context.NameSpaceId = NS_ALL;
    Context.UseGuid = FALSE;
    InitializeListHead (&Context.EntryList);
    Context.ErrorCode = ERROR_SUCCESS;

    Catalog->EnumerateCatalogItems(
        MatchProviders,
        &Context);

    if (Context.ErrorCode == ERROR_SUCCESS)
    {
        ErrorCode = NO_DATA;

        while (!IsListEmpty (&Context.EntryList))
        {
            PNSCATALOGENTRYSTATE    EntryState;
            PLIST_ENTRY             ListItem;
            PNSPROVIDER             Provider;

            ListItem = RemoveHeadList (&Context.EntryList);
            EntryState = CONTAINING_RECORD (ListItem,
                                                NSCATALOGENTRYSTATE,
                                                m_context_linkage
                                                );

            Provider = EntryState->GetProvider (Catalog);

            if (Provider!=NULL)
            {
                INT Error = Provider->NSPRemoveServiceClass(lpServiceClassId);

                if (!Error)
                {
                    ErrorCode = ERROR_SUCCESS;
                }
                else
                {
                    if (ErrorCode)
                    {
                        ErrorCode = GetLastError();
                        //
                        // Reset error code if provider fails
                        // to set last error for some reason
                        //
                        if (ErrorCode==ERROR_SUCCESS)
                            ErrorCode = NO_DATA;
                    }
                }
            }
            delete EntryState;
        }
    }
    else
        ErrorCode = Context.ErrorCode;

    if (ErrorCode == ERROR_SUCCESS)
    {
        return (ERROR_SUCCESS);
    }
    else
    {
        SetLastError(ErrorCode);
        return (SOCKET_ERROR);
    }
}


INT WSAAPI
WSAGetServiceClassNameByClassIdA(
    IN      LPGUID  lpServiceClassId,
    OUT     LPSTR lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength
    )
/*++
Routine Description:
    This API will return the name of the service associated with the given
    type.  This name is the generic service name, like FTP, or SNA, and not the
    name of a specific instance of that service.

Arguments:
    lpServiceClassId - pointer to the GUID for the service class.

    lpszServiceClassName - service name.

    lpdwBufferLength - on input length of buffer returned by
                       lpszServiceClassName. On output, the length of the
                       service name copied into lpszServiceClassName.

Returns:
    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.

--*/
{
    PDPROCESS       Process;
    PDTHREAD        Thread;
    INT             ErrorCode;
    PNSCATALOG      Catalog;
    MATCH_PROVIDERS_CONTEXT Context;

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    Catalog = Process->GetNamespaceCatalog();
    //
    // Specifying all namespaces gives us all enabled providers
    // which is exactly what we want
    //

    Context.NameSpaceId = NS_ALL;
    Context.UseGuid = FALSE;
    InitializeListHead (&Context.EntryList);
    Context.ErrorCode = ERROR_SUCCESS;

    Catalog->EnumerateCatalogItems(
        MatchProviders,
        &Context);

    if (Context.ErrorCode == ERROR_SUCCESS) {
        ErrorCode = NO_DATA;
        while (!IsListEmpty (&Context.EntryList)) {
            PNSCATALOGENTRYSTATE    EntryState;
            PLIST_ENTRY             ListItem;
            PNSPROVIDER             Provider;
            WSASERVICECLASSINFOW    Buffer, *pBuffer;
            DWORD                   BufferSize;

            ListItem = RemoveHeadList (&Context.EntryList);
            EntryState = CONTAINING_RECORD (ListItem,
                                                NSCATALOGENTRYSTATE,
                                                m_context_linkage
                                                );

            Provider = EntryState->GetProvider (Catalog);
            if (Provider!=NULL) {
                BufferSize = sizeof (Buffer);
                Buffer.lpServiceClassId = lpServiceClassId;
                ErrorCode = Provider->NSPGetServiceClassInfo(
                                                &BufferSize,
                                                &Buffer);
                if(ErrorCode == ERROR_SUCCESS)
                {
                    //
                    // this is impossible. The provider has made an error, so
                    // concoct an error for it.
                    //
                    //
                    // ErrorCode = WSANO_DATA; // done above
                }
                else
                {
                    ErrorCode = GetLastError();
                    if (ErrorCode==ERROR_SUCCESS)
                        ErrorCode = WSANO_DATA;
                }

                if (WSAEFAULT == ErrorCode){
                    // The service provider claimed that it had an answer but our
                    // buffer was to small big suprise :-() so get a new buffer and go
                    // get the answer.
                    pBuffer = (LPWSASERVICECLASSINFOW) new BYTE[BufferSize];

                    if( pBuffer != NULL ) {

                        pBuffer->lpServiceClassId = lpServiceClassId;

                        ErrorCode = Provider->NSPGetServiceClassInfo(
                                        &BufferSize,
                                        pBuffer);
                        if ( ErrorCode == ERROR_SUCCESS &&
                             pBuffer->lpszServiceClassName )
                        {
                            DWORD StringLen = ((wcslen(pBuffer->lpszServiceClassName)+1)
                                                * sizeof(WCHAR));

                            __try {
                                if (*lpdwBufferLength >= StringLen){
                                    WideCharToMultiByte(
                                        CP_ACP,                         // CodePage (ANSI)
                                        0,                              // dwFlags
                                        pBuffer->lpszServiceClassName,  // lpWideCharStr
                                        -1,                             // cchWideChar
                                        lpszServiceClassName,           // lpMultiByteStr
                                        StringLen,                      // cchMultiByte
                                        NULL,                           // lpDefaultChar
                                        NULL                            // lpUsedDefaultChar
                                        );
                    
                                } //if
                                else{
                                    ErrorCode  = WSAEFAULT;
                                } //else
                                *lpdwBufferLength = StringLen;
                            }
                            __except (WS2_EXCEPTION_FILTER()) {
                                // Not much more we can do
                                ErrorCode = WSAEFAULT;
                            }

                        }
                        else
                        {
                            ErrorCode = GetLastError();
                            if (ErrorCode==ERROR_SUCCESS)
                                ErrorCode = WSANO_DATA;
                        }
                        delete pBuffer;
                    }
                    else {
                        ErrorCode = WSAENOBUFS;
                    }

                    delete EntryState;
                    // Provider at least once told us that he has
                    // something for us. Delete the rest and complete.
                    while (!IsListEmpty (&Context.EntryList)) {
                        ListItem = RemoveHeadList (&Context.EntryList);
                        EntryState = CONTAINING_RECORD (ListItem,
                                                            NSCATALOGENTRYSTATE,
                                                            m_context_linkage
                                                            );
                        delete EntryState;
                    }
                    break;
                } //if GetSize call succeeded
            } //if Provider is loaded
            delete EntryState;
        }
    }
    else
        ErrorCode = Context.ErrorCode;

    if (ErrorCode == ERROR_SUCCESS) {
        return (ERROR_SUCCESS);
    }
    else {
        SetLastError(ErrorCode);
        return (SOCKET_ERROR);
    }
}

INT WSAAPI
WSAGetServiceClassNameByClassIdW(
    IN      LPGUID  lpServiceClassId,
    OUT     LPWSTR lpszServiceClassName,
    IN OUT  LPDWORD lpdwBufferLength
    )
/*++
Routine Description:
    This API will return the name of the service associated with the given
    type.  This name is the generic service name, like FTP, or SNA, and not the
    name of a specific instance of that service.

Arguments:
    lpServiceClassId - pointer to the GUID for the service class.

    lpszServiceClassName - service name.

    lpdwBufferLength - on input length of buffer returned by
                       lpszServiceClassName. On output, the length of the
                       service name copied into lpszServiceClassName.

Returns:
    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.

--*/
{
    PDPROCESS       Process;
    PDTHREAD        Thread;
    INT             ErrorCode;
    PNSCATALOG      Catalog;
    MATCH_PROVIDERS_CONTEXT Context;

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    Catalog = Process->GetNamespaceCatalog();
    //
    // Specifying all namespaces gives us all enabled providers
    // which is exactly what we want
    //

    Context.NameSpaceId = NS_ALL;
    Context.UseGuid = FALSE;
    InitializeListHead (&Context.EntryList);
    Context.ErrorCode = ERROR_SUCCESS;

    Catalog->EnumerateCatalogItems(
        MatchProviders,
        &Context);

    if (Context.ErrorCode == ERROR_SUCCESS) {
        ErrorCode = NO_DATA;
        while (!IsListEmpty (&Context.EntryList)) {
            PNSCATALOGENTRYSTATE    EntryState;
            PLIST_ENTRY             ListItem;
            PNSPROVIDER             Provider;
            WSASERVICECLASSINFOW    Buffer, *pBuffer;
            DWORD                   BufferSize;

            ListItem = RemoveHeadList (&Context.EntryList);
            EntryState = CONTAINING_RECORD (ListItem,
                                                NSCATALOGENTRYSTATE,
                                                m_context_linkage
                                                );

            Provider = EntryState->GetProvider (Catalog);
            if (Provider!=NULL) {
                BufferSize = sizeof (Buffer);
                Buffer.lpServiceClassId = lpServiceClassId;
                ErrorCode = Provider->NSPGetServiceClassInfo(
                                                &BufferSize,
                                                &Buffer);
                if(ErrorCode == ERROR_SUCCESS)
                {
                    //
                    // this is impossible. The provider has made an error, so
                    // concoct an error for it.
                    //
                    //
                    // ErrorCode = WSANO_DATA; // done above
                }
                else
                {
                    ErrorCode = GetLastError();
                    if (ErrorCode==ERROR_SUCCESS)
                        ErrorCode = WSANO_DATA;
                }

                if (WSAEFAULT == ErrorCode){
                    // The service provider claimed that it had an answer but our
                    // buffer was to small big suprise :-() so get a new buffer and go
                    // get the answer.
                    pBuffer = (LPWSASERVICECLASSINFOW) new BYTE[BufferSize];

                    if( pBuffer != NULL ) {

                        pBuffer->lpServiceClassId = lpServiceClassId;

                        ErrorCode = Provider->NSPGetServiceClassInfo(
                                        &BufferSize,
                                        pBuffer);
                        if(ErrorCode == ERROR_SUCCESS) {
                            DWORD StringLen = ((wcslen(pBuffer->lpszServiceClassName)+1)
                                                * sizeof(WCHAR));

                            __try {
                                if (*lpdwBufferLength >= StringLen){
                                    wcscpy( lpszServiceClassName,
                                            pBuffer->lpszServiceClassName);
                    
                                } //if
                                else{
                                    ErrorCode  = WSAEFAULT;
                                } //else
                                *lpdwBufferLength = StringLen;
                            }
                            __except (WS2_EXCEPTION_FILTER()) {
                                // Not much more we can do
                                ErrorCode = WSAEFAULT;
                            }

                        }
                        else
                        {
                            ErrorCode = GetLastError();
                            if (ErrorCode==ERROR_SUCCESS)
                                ErrorCode = WSANO_DATA;
                        }
                        delete pBuffer;
                    }
                    else {
                        ErrorCode = WSAENOBUFS;
                    }

                    delete EntryState;
                    // Provider at least once told us that he has
                    // something for us. Delete the rest and complete.
                    while (!IsListEmpty (&Context.EntryList)) {
                        ListItem = RemoveHeadList (&Context.EntryList);
                        EntryState = CONTAINING_RECORD (ListItem,
                                                            NSCATALOGENTRYSTATE,
                                                            m_context_linkage
                                                            );
                        delete EntryState;
                    }
                    break;
                } //if GetSize call succeeded
            } //if Provider is loaded
            delete EntryState;
        }
    }
    else
        ErrorCode = Context.ErrorCode;

    if (ErrorCode == ERROR_SUCCESS) {
        return (ERROR_SUCCESS);
    }
    else {
        SetLastError(ErrorCode);
        return (SOCKET_ERROR);
    }
}




INT
WSAAPI
WSAGetServiceClassInfoA(
    IN  LPGUID                  lpProviderId,
    IN  LPGUID                  lpServiceClassId,
    OUT LPDWORD                 lpdwBufSize,
    OUT LPWSASERVICECLASSINFOA  lpServiceClassInfo
    )
/*++

Routine Description:

    WSAGetServiceClassInfo() is used to retrieve all of the class information
    (schema) pertaining to a specified service class from a specified name
    space provider.

Arguments:

    lpProviderId - Pointer to a GUID which identifies a specific name space
                   provider.

    lpServiceClassId - Pointer to a GUID identifying the service class in
                       question.

    lpdwBufferLength - on input, the number of bytes contained in the buffer
                       pointed  to by lpServiceClassInfos.  On output - if the
                       API fails, and the error is WSAEFAULT, then it contains
                       the minimum number of bytes to pass for the
                       lpServiceClassInfo to retrieve the record.

    lpServiceClasslnfo - returns service class information from the indicated
                         name space provider for the specified service class.

Return Value:

    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.

--*/
{
    LPWSASERVICECLASSINFOW WideServiceClassInfo;
    INT   ReturnCode;
    DWORD ErrorCode;

    if (!lpProviderId ||      // Fix for bug #102088
        !lpServiceClassId ||
        !lpdwBufSize ||
        !lpServiceClassInfo ) {
        SetLastError(WSAEINVAL);
        return(SOCKET_ERROR);
    }

    ReturnCode = SOCKET_ERROR;
    ErrorCode = WSAEINVAL;

    WideServiceClassInfo =(LPWSASERVICECLASSINFOW) new BYTE[*lpdwBufSize];
    if (WideServiceClassInfo){
        ReturnCode = WSAGetServiceClassInfoW(
            lpProviderId,
            lpServiceClassId,
            lpdwBufSize,
            WideServiceClassInfo);
        if (ERROR_SUCCESS == ReturnCode){
            MapUnicodeServiceClassInfoToAnsi(
                WideServiceClassInfo,
                lpdwBufSize,
                lpServiceClassInfo);
        } //if
        else{
            ErrorCode = GetLastError();
        } //else
        delete WideServiceClassInfo;
    } //if

    if (ERROR_SUCCESS != ReturnCode){
        SetLastError(ErrorCode);
    } //if
    return(ReturnCode);
}


INT
WSAAPI
WSAGetServiceClassInfoW(
    IN  LPGUID  lpProviderId,
    IN  LPGUID  lpServiceClassId,
    IN  OUT LPDWORD  lpdwBufSize,
    OUT LPWSASERVICECLASSINFOW lpServiceClassInfo
)
/*++

Routine Description:

    WSAGetServiceClassInfo() is used to retrieve all of the class information
    (schema) pertaining to a specified service class from a specified name
    space provider.

Arguments:

    lpProviderId - Pointer to a GUID which identifies a specific name space
                   provider.

    lpServiceClassId - Pointer to a GUID identifying the service class in
                       question.

    lpdwBufferLength - on input, the number of bytes contained in the buffer
                       pointed  to by lpServiceClassInfos.  On output - if the
                       API fails, and the error is WSAEFAULT, then it contains
                       the minimum number of bytes to pass for the
                       lpServiceClassInfo to retrieve the record.

    lpServiceClasslnfo - returns service class information from the indicated
                         name space provider for the specified service class.

Return Value:

    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.

--*/
{

    PDPROCESS       Process;
    PDTHREAD        Thread;
    INT             ErrorCode;
    PNSCATALOG      Catalog;
    PNSPROVIDER     Provider;
    PNSCATALOGENTRY CatalogEntry;

    if (!lpProviderId ||      // Fix for bug #102088
        !lpServiceClassId ||
        !lpdwBufSize ||
        !lpServiceClassInfo ) {
        SetLastError(WSAEINVAL);
        return(SOCKET_ERROR);
    }

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    }

    Catalog = Process->GetNamespaceCatalog();
    ErrorCode = Catalog->GetCountedCatalogItemFromProviderId(
        lpProviderId,
        &CatalogEntry);
    if(ERROR_SUCCESS == ErrorCode){
        if (CatalogEntry->GetEnabledState()) {
            WSASERVICECLASSINFOW scliTemp;
            Provider = CatalogEntry->GetProvider();
            if (Provider==NULL) {
                ErrorCode = Catalog->LoadProvider (CatalogEntry);
                if (ErrorCode!=NO_ERROR) {
                    goto DereferenceEntry;
                }
                Provider = CatalogEntry->GetProvider ();
                assert (Provider!=NULL);
            }


            __try {
                if(*lpdwBufSize < sizeof(*lpServiceClassInfo)) {
                    //
                    // this is sleazy as we don't adjust the buffer
                    // size. But it makes things work
                    //
                    lpServiceClassInfo = &scliTemp;
                }
                lpServiceClassInfo->lpServiceClassId = lpServiceClassId;
            }
            __except (WS2_EXCEPTION_FILTER()) {
                ErrorCode = WSAEFAULT;
                goto DereferenceEntry;
            }
            if (Provider->NSPGetServiceClassInfo(
                   lpdwBufSize,
                   lpServiceClassInfo)!=ERROR_SUCCESS) {
                ErrorCode = GetLastError ();
                if (ErrorCode==ERROR_SUCCESS)
                    ErrorCode = WSANO_DATA;

            }
        }
        else {
            ErrorCode = WSAEINVAL;
        }
    DereferenceEntry:
        CatalogEntry->Dereference ();
    }

    if(ErrorCode == ERROR_SUCCESS) {
        return ERROR_SUCCESS;
    }
    else {
        SetLastError(ErrorCode);
        return SOCKET_ERROR;
    }

}

INT
WSAAPI
WSAAddressToStringW(
    IN     LPSOCKADDR          lpsaAddress,
    IN     DWORD               dwAddressLength,
    IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN OUT LPWSTR              lpszAddressString,
    IN OUT LPDWORD             lpdwAddressStringLength
    )
/*++

Routine Description:

    WSAAddressToString() converts a SOCKADDR structure into a human-readable
    string representation of the address.  This is intended to be used mainly
    for display purposes. If the caller wishes the translation to be done by a
    particular provider, it should supply the corresponding WSAPROTOCOL_INFO
    struct in the lpProtocolInfo parameter.

Arguments:

    lpsaAddress - points to a SOCKADDR structure to translate into a string.

    dwAddressLength - the length of the Address SOCKADDR.

    lpProtocolInfo - (optional) the WSAPROTOCOL_INFO struct for a particular
                     provider.

    lpszAddressString - a buffer which receives the human-readable address
                        string.

    lpdwAddressStringLength - on input, the length of the AddressString buffer.
                              On output, returns the length of  the string
                              actually copied into the buffer.

Return Value:

    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned
--*/
{
    INT                 ReturnValue;
    PDPROCESS           Process;
    PDTHREAD            Thread;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDCATALOG           Catalog;
    PPROTO_CATALOG_ITEM CatalogEntry;
    LPWSAPROTOCOL_INFOW ProtocolInfo;

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    if (!lpsaAddress ||      // Fix for bug #114256
        !dwAddressLength ||
        !lpszAddressString ||
        !lpdwAddressStringLength ) {
        SetLastError(WSAEINVAL);
        return(SOCKET_ERROR);
    }

    // Find a provider that can support the user request
    Catalog = Process->GetProtocolCatalog();

    if (lpProtocolInfo) {
        DWORD   dwCatalogEntryId;
        __try {
            dwCatalogEntryId = lpProtocolInfo->dwCatalogEntryId;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            SetLastError(WSAEFAULT);
            return(SOCKET_ERROR);
        }

        ErrorCode =  Catalog->GetCountedCatalogItemFromCatalogEntryId(
            dwCatalogEntryId,
            &CatalogEntry);
    } //if
    else {
        int family;
        __try {
            family = lpsaAddress->sa_family;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            SetLastError(WSAEFAULT);
            return(SOCKET_ERROR);
        }

        ErrorCode = Catalog->GetCountedCatalogItemFromAddressFamily(
            family,
            &CatalogEntry);
    }

    if ( ERROR_SUCCESS == ErrorCode) {
        ProtocolInfo = CatalogEntry->GetProtocolInfo();
        assert( ProtocolInfo != NULL );
        Provider = CatalogEntry->GetProvider();
        ReturnValue = Provider->WSPAddressToString(
            lpsaAddress,
            dwAddressLength,
            ProtocolInfo,
            lpszAddressString,
            lpdwAddressStringLength,
            &ErrorCode);
        CatalogEntry->Dereference ();
        if (ReturnValue==ERROR_SUCCESS)
            return ERROR_SUCCESS;
    } //if

    SetLastError(ErrorCode);
    return(SOCKET_ERROR);

}


INT
WSAAPI
WSAAddressToStringA(
    IN     LPSOCKADDR          lpsaAddress,
    IN     DWORD               dwAddressLength,
    IN     LPWSAPROTOCOL_INFOA lpProtocolInfo,
    IN OUT LPSTR               lpszAddressString,
    IN OUT LPDWORD             lpdwAddressStringLength
    )
/*++

Routine Description:

    WSAAddressToString() converts a SOCKADDR structure into a human-readable
    string representation of the address.  This is intended to be used mainly
    for display purposes. If the caller wishes the translation to be done by a
    particular provider, it should supply the corresponding WSAPROTOCOL_INFO
    struct in the lpProtocolInfo parameter.

Arguments:

    lpsaAddress - points to a SOCKADDR structure to translate into a string.

    dwAddressLength - the length of the Address SOCKADDR.

    lpProtocolInfo - (optional) the WSAPROTOCOL_INFO struct for a particular
                     provider.

    lpszAddressString - a buffer which receives the human-readable address
                        string.

    lpdwAddressStringLength - on input, the length of the AddressString buffer.
                              On output, returns the length of  the string
                              actually copied into the buffer.

Return Value:

    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned
--*/
{
    INT                 ReturnValue;
    PDPROCESS           Process;
    PDTHREAD            Thread;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDCATALOG           Catalog;
    PPROTO_CATALOG_ITEM CatalogEntry;
    LPWSAPROTOCOL_INFOW ProtocolInfo;
    LPWSTR              LocalString;
    DWORD               LocalStringLength;


    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    if (!lpsaAddress ||      // Fix for bug #114256
        !dwAddressLength ||
        !lpszAddressString ||
        !lpdwAddressStringLength ) {
        SetLastError(WSAEINVAL);
        return(SOCKET_ERROR);
    }

    //Get a buffer to hold the unicode string the service provider is going to
    //return
    __try {
        LocalStringLength = *lpdwAddressStringLength;
    }
    __except (WS2_EXCEPTION_FILTER()) {
        SetLastError(WSAEFAULT);
        return(SOCKET_ERROR);
    }

    LocalString = (LPWSTR) new WCHAR[LocalStringLength];
    if (LocalString==NULL) {
        SetLastError(WSAENOBUFS);
        return(SOCKET_ERROR);
    } //if

    // Find a provider that can support the user request
    Catalog = Process->GetProtocolCatalog();

    if (lpProtocolInfo) {
        DWORD   dwCatalogEntryId;
        __try {
            dwCatalogEntryId = lpProtocolInfo->dwCatalogEntryId;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            delete(LocalString);
            SetLastError(WSAEFAULT);
            return(SOCKET_ERROR);
        }

        ErrorCode =  Catalog->GetCountedCatalogItemFromCatalogEntryId(
            dwCatalogEntryId,
            &CatalogEntry);
    } //if
    else {
        int family;
        __try {
            family = lpsaAddress->sa_family;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            delete(LocalString);
            SetLastError(WSAEFAULT);
            return(SOCKET_ERROR);
        }

        ErrorCode = Catalog->GetCountedCatalogItemFromAddressFamily(
            family,
            &CatalogEntry);
    }

    if ( ERROR_SUCCESS == ErrorCode) {
        ProtocolInfo = CatalogEntry->GetProtocolInfo();
        assert( ProtocolInfo != NULL );
        Provider = CatalogEntry->GetProvider();

        ReturnValue = Provider->WSPAddressToString(
            lpsaAddress,
            dwAddressLength,
            ProtocolInfo,
            LocalString,
            lpdwAddressStringLength,
            &ErrorCode);

        if (ERROR_SUCCESS == ReturnValue){
            __try {
                WideCharToMultiByte(
                    CP_ACP,                        // CodePage (Ansi)
                    0,                             // dwFlags
                    LocalString,                   // lpWideCharStr
                    -1,                            // cchWideCharStr
                    lpszAddressString,             // lpMultiByte
                    LocalStringLength,             // cchMultiByte
                    NULL,
                    NULL);
            }
            __except (WS2_EXCEPTION_FILTER()) {
                ErrorCode = WSAEFAULT;
                ReturnValue = SOCKET_ERROR;
            }
        } //if

        delete(LocalString);
        CatalogEntry->Dereference ();
        if (ReturnValue==ERROR_SUCCESS)
            return ERROR_SUCCESS;

    } //if
    else {
        delete(LocalString);
    }

    SetLastError(ErrorCode);
    return(SOCKET_ERROR);
}

INT
WSAAPI
WSAStringToAddressW(
    IN     LPWSTR              AddressString,
    IN     INT                 AddressFamily,
    IN     LPWSAPROTOCOL_INFOW lpProtocolInfo,
    IN OUT LPSOCKADDR          lpAddress,
    IN OUT LPINT               lpAddressLength
    )
/*++

Routine Description:

    WSAStringToAddress() converts a human-readable string to a socket address
    structure (SOCKADDR) suitable for pass to Windows Sockets routines which
    take such a structure.  If the caller wishes the translation to be done by
    a particular provider, it should supply the corresponding WSAPROTOCOL_INFOW
    struct in the lpProtocolInfo parameter.

Arguments:

    AddressString - points to the zero-terminated human-readable string to
                    convert.

    AddressFamily - the address family to which the string belongs.

    lpProtocolInfo - (optional) the WSAPROTOCOL_INFOW struct for a particular
                     provider.

    Address - a buffer which is filled with a single SOCKADDR structure.

    lpAddressLength - The length of the Address buffer.  Returns the size of
                      the resultant SOCKADDR structure.

Return Value:

    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.

--*/
{
    INT                 ReturnValue;
    PDPROCESS           Process;
    PDTHREAD            Thread;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDCATALOG           Catalog;
    PPROTO_CATALOG_ITEM CatalogEntry;
    LPWSAPROTOCOL_INFOW ProtocolInfo;

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    // Find a provider that can support the user request
    Catalog = Process->GetProtocolCatalog();

    if (lpProtocolInfo) {
        DWORD dwCatalogEntryId;
        __try {
            dwCatalogEntryId =  lpProtocolInfo->dwCatalogEntryId;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            SetLastError(WSAEFAULT);
            return(SOCKET_ERROR);
        }
        ErrorCode =  Catalog->GetCountedCatalogItemFromCatalogEntryId(
            dwCatalogEntryId,
            &CatalogEntry);
    } //if
    else{
        ErrorCode = Catalog->GetCountedCatalogItemFromAddressFamily(
            AddressFamily,
            &CatalogEntry);
    }

    if ( ERROR_SUCCESS == ErrorCode) {
        ProtocolInfo = CatalogEntry->GetProtocolInfo();
        assert( ProtocolInfo != NULL );
        Provider = CatalogEntry->GetProvider();
        ReturnValue = Provider->WSPStringToAddress(
            AddressString,
            AddressFamily,
            ProtocolInfo,
            lpAddress,
            lpAddressLength,
            &ErrorCode);
        CatalogEntry->Dereference ();
        if (ReturnValue==ERROR_SUCCESS)
            return ERROR_SUCCESS;
    } //if

    SetLastError(ErrorCode);
    return(SOCKET_ERROR);
}

INT
WSAAPI
WSAStringToAddressA(
    IN     LPSTR               AddressString,
    IN     INT                 AddressFamily,
    IN     LPWSAPROTOCOL_INFOA lpProtocolInfo,
    IN OUT LPSOCKADDR          lpAddress,
    IN OUT LPINT               lpAddressLength
    )
/*++

Routine Description:

    WSAStringToAddress() converts a human-readable string to a socket address
    structure (SOCKADDR) suitable for pass to Windows Sockets routines which
    take such a structure.  If the caller wishes the translation to be done by
    a particular provider, it should supply the corresponding WSAPROTOCOL_INFOA
    struct in the lpProtocolInfo parameter.

Arguments:

    AddressString - points to the zero-terminated human-readable string to
                    convert.

    AddressFamily - the address family to which the string belongs.

    lpProtocolInfo - (optional) the WSAPROTOCOL_INFOA struct for a particular
                     provider.

    Address - a buffer which is filled with a single SOCKADDR structure.

    lpAddressLength - The length of the Address buffer.  Returns the size of
                      the resultant SOCKADDR structure.

Return Value:

    The return value is 0 if the operation was successful.  Otherwise the value
    SOCKET_ERROR is returned.

--*/
{
    INT                 ReturnValue;
    PDPROCESS           Process;
    PDTHREAD            Thread;
    INT                 ErrorCode;
    PDPROVIDER          Provider;
    PDCATALOG           Catalog;
    PPROTO_CATALOG_ITEM CatalogEntry;
    LPWSAPROTOCOL_INFOW ProtocolInfo;
    LPWSTR               LocalString;
    INT                 LocalStringLength;

    ErrorCode = PROLOG(&Process,
                        &Thread);
    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    __try {
        // Get a buffer to hold the ansi string handed in by the user.
        LocalStringLength = strlen(AddressString)+1;
        LocalString = (LPWSTR)new WCHAR[LocalStringLength];
        if (LocalString==NULL) {
            SetLastError (WSAENOBUFS);
            return (SOCKET_ERROR);
        }

        MultiByteToWideChar(
            CP_ACP,                          // CodePage (Ansi)
            0,                               // dwFlags
            AddressString,                   // lpMultiByte
            -1,                              // cchMultiByte
            LocalString,                     // lpWideChar
            LocalStringLength);              // ccWideChar
    }
    __except (WS2_EXCEPTION_FILTER()) {
        SetLastError(WSAEFAULT);
        return(SOCKET_ERROR);
    }

    // Find a provider that can support the user request
    Catalog = Process->GetProtocolCatalog();

    if (lpProtocolInfo) {
        DWORD dwCatalogEntryId;
        __try {
            dwCatalogEntryId =  lpProtocolInfo->dwCatalogEntryId;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            delete (LocalString);
            SetLastError(WSAEFAULT);
            return(SOCKET_ERROR);
        }

        ErrorCode =  Catalog->GetCountedCatalogItemFromCatalogEntryId(
            dwCatalogEntryId,
            &CatalogEntry);
    } //if
    else{
        ErrorCode = Catalog->GetCountedCatalogItemFromAddressFamily(
            AddressFamily,
            &CatalogEntry);
    }

    if ( ERROR_SUCCESS == ErrorCode) {
        ProtocolInfo = CatalogEntry->GetProtocolInfo();
        assert( ProtocolInfo != NULL );

        Provider = CatalogEntry->GetProvider();

        ReturnValue = Provider->WSPStringToAddress(
            LocalString,
            AddressFamily,
            ProtocolInfo,
            lpAddress,
            lpAddressLength,
            &ErrorCode);
        CatalogEntry->Dereference ();
        delete(LocalString);
        if (ReturnValue==ERROR_SUCCESS)
            return ERROR_SUCCESS;
    } //if
    else {
        delete(LocalString);
    }


    SetLastError(ErrorCode);
    return(SOCKET_ERROR);
}


PNSCATALOG
OpenInitializedNameSpaceCatalog()
{
    BOOL ReturnCode = TRUE;
    PNSCATALOG ns_catalog;
    HKEY RegistryKey = 0;

     TRY_START(mem_guard){

        //
        // Build the protocol catalog
        //

        ns_catalog = new (NSCATALOG);

        if (!ns_catalog) {

            DEBUGF (DBG_ERR, ("Failed to allocate nscatalog object\n"));
            TRY_THROW(mem_guard);
        }

        RegistryKey = OpenWinSockRegistryRoot();

        if (!RegistryKey) {

            DEBUGF (DBG_ERR, ("OpenWinSockRegistryRoot Failed\n"));
            TRY_THROW(mem_guard);
        }

        ReturnCode = ns_catalog->InitializeFromRegistry(
                                    RegistryKey,
                                    NULL
                                    );

        if (ERROR_SUCCESS != ReturnCode) {

            DEBUGF (DBG_ERR, ("dcatalog InitializeFromRegistry Failed\n"));
            TRY_THROW(mem_guard);
        }

    } TRY_CATCH(mem_guard) {

        delete (ns_catalog);
        ns_catalog = NULL;

    } TRY_END(mem_guard);

    LONG close_result;

    if (RegistryKey) {

        close_result = RegCloseKey (RegistryKey);
        assert(close_result == ERROR_SUCCESS);
    }

    return (ns_catalog);
}


