/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    enumprot.cpp

Abstract:

    This module contains the WSAEnumProtocol entrypoint for the
    winsock API.


Author:

    Dirk Brandewie dirk@mink.intel.com  14-06-1995

[Environment:]

[Notes:]

Revision History:

    22-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved includes to precomp.h


--*/

#include "precomp.h"

#define END_PROTOCOL_LIST_MARKER 0

// The  following  structure is passed as a passback value to the provider path
// iterator.   The  user  arguments  are  copied to ProviderId, PathBuffer, and
// PathBufferLength.   If  the  provider  is found then ProviderFound is set to
// TRUE  and  the  actual  number  of bytes needed for the path is written into
// CharsNeeded.   The  path is copied into the buffer if there is enough space,
// otherwise the buffer is not used.
typedef struct
{
    GUID   ProviderId;
    LPWSTR PathBuffer;
    INT    PathBufferLength;
    INT    CharsNeeded;
    BOOL   ProviderFound;
    INT    ErrorCode;
} PATH_ENUMERATION_CONTEXT,  *PPATH_ENUMERATION_CONTEXT;


//
// Stucture to be used as a passback value to the catalog iterator. The user
// arguments to WSAEnumProtocols are copied into "Protocols", "ProtocolBuffer"
// and BufferLength. BytesUsed and ProtocolCount are used for bookkeeping in
// the enumeration procedure ProtocolIterationProc() and the functions it calls
// to do its work.
//
typedef struct
{
    LPINT               Protocols;
    LPWSAPROTOCOL_INFOW ProtocolBuffer;
    DWORD               BufferLength;
    DWORD               BytesUsed;
    DWORD               ProtocolCount;
    INT                 ErrorCode;
} PROTOCOL_ENUMERATION_CONTEXT, *PPROTOCOL_ENUMERATION_CONTEXT;

//Return whether there is enough room in the application buffer to
//hold another WSAPROTOCOL_INFOW struct
#define OK_TO_COPY_PROTOCOL(p_context)\
(((p_context)->BytesUsed + sizeof(WSAPROTOCOL_INFOW)) <=\
((p_context)->BufferLength)?TRUE:FALSE)

#define NEXT_BUFFER_LOCATION(p_context)\
((char*)(p_context)->ProtocolBuffer + (p_context)->BytesUsed)



static
VOID
CopyProtocolInfo(
    IN LPWSAPROTOCOL_INFOW ProtocolInfo,
    IN PPROTOCOL_ENUMERATION_CONTEXT Context
    )
/*++
Routine Description:

    Copies a protocol info struct onto the end of a user buffer is enough room
    exists in the user buffer.

Arguments:

    ProtocolInfo - A Pointer to a WSAPROTOCOL_INFOW struct to be copied.

    Context - A Pointer to an enumeration context structure. This structure
              contians all the infomation about the user buffer.

Returns:

--*/
{
    if (OK_TO_COPY_PROTOCOL(Context)) {
        CopyMemory(
            NEXT_BUFFER_LOCATION(Context),
            ProtocolInfo,
            sizeof(WSAPROTOCOL_INFOW));

        // So we can tell the user how many WSAPROTOCOL_INFOW struct
        // we copied into their buffer
        Context->ProtocolCount++;
    } //if
}


static
BOOL
IsProtocolInSet(
    IN LPINT Set,
    IN LPWSAPROTOCOL_INFOW ProtocolInfo
    )
/*++
Routine Description:

    This function returns whether the protocol described by ProtocolInfo is a
    member of the set of protocols pointed to by Set.

Arguments:

    Set - Apointer to an array of protocol ID's

    ProtocolInfo - A Pointer to a WSAPROTOCOL_INFOW struct.

Returns:

--*/
{
    BOOL ReturnCode  =FALSE;
    INT   SetIndex   =0;
    INT   ProtocolID =0;

    if (Set) {
        ProtocolID = Set[SetIndex];
        while (ProtocolID != END_PROTOCOL_LIST_MARKER) {
            if ((ProtocolID >= ProtocolInfo->iProtocol) &&
                (ProtocolID <= (ProtocolInfo->iProtocol +
                               ProtocolInfo->iProtocolMaxOffset))
                ) {
                ReturnCode = TRUE;
                break;
            } //if
            SetIndex++;
            ProtocolID = Set[SetIndex];
        } //while
    } //if
    else {
        // If the set pointer is null all protocols are in the set.
        ReturnCode = TRUE;
    } //else
    return(ReturnCode);
}




static
BOOL
ProtocolIterationProcAPI(
    IN OUT PVOID IterationContext,
    IN PPROTO_CATALOG_ITEM  CatalogEntry
    )
/*++
Routine Description:

    This  function  is the enumeration procedure passed to the protocol catalog
    enumeration  function.   It  adds  the  protocol  to  the list if (1) it is
    allowed  by the protocol list restriction set, (2) if it is not hidden, and
    (3) it is not a non-chained layer.  "Adding to the list" only has effect if
    there is room in the buffer.

    The  procedure  updates  the  number  of  bytes "used" for each protocol it
    accepts regardless of whether it actually found room in the buffer or not.

Arguments:

    IterationContext - Passback value from call to
                       DCATALOG::EnumerateCatalogItems();

    CatalofEntry - A pointer to a PROTO_CATALOG_ITEM.

Returns:
    TRUE to continue the enumeration of the protocol catalog

--*/
{
    PPROTOCOL_ENUMERATION_CONTEXT Context;
    LPWSAPROTOCOL_INFOW ProtocolInfo;

    Context = (PPROTOCOL_ENUMERATION_CONTEXT)IterationContext;
    ProtocolInfo = CatalogEntry->GetProtocolInfo();

    __try {
        // If the protocol meets acceptance criteria, then add it
        if (IsProtocolInSet(Context->Protocols, ProtocolInfo)) {
            if (! ((ProtocolInfo->dwProviderFlags) & PFL_HIDDEN)) {
                if (ProtocolInfo->ProtocolChain.ChainLen != LAYERED_PROTOCOL) {
                    CopyProtocolInfo(ProtocolInfo, Context);
                    Context->BytesUsed += sizeof(WSAPROTOCOL_INFOW);
                } // if non-layered
            } //if non-hidden
        } //if is in set
        return(TRUE); //Continue enumeration
    }
    __except (WS2_EXCEPTION_FILTER()) {
        Context->ErrorCode = WSAEFAULT;
        return FALSE;
    }

}




static
BOOL
ProtocolIterationProcSPI(
    IN OUT PVOID IterationContext,
    IN PPROTO_CATALOG_ITEM  CatalogEntry
    )
/*++
Routine Description:

    This  function  is the enumeration procedure passed to the protocol catalog
    enumeration function.  It adds the protocol to the list if it is allowed by
    the protocol list restriction set.  "Adding to the list" only has effect if
    there is room in the buffer.

    The  procedure  updates  the  number  of  bytes "used" for each protocol it
    accepts regardless of whether it actually found room in the buffer or not.

Arguments:

    IterationContext - Passback value from call to
                       DCATALOG::EnumerateCatalogItems();

    CatalofEntry - A pointer to a PROTO_CATALOG_ITEM.

Returns:
    TRUE to continue the enumeration of the protocol catalog

--*/
{
    PPROTOCOL_ENUMERATION_CONTEXT Context;
    LPWSAPROTOCOL_INFOW ProtocolInfo;

    Context = (PPROTOCOL_ENUMERATION_CONTEXT)IterationContext;
    ProtocolInfo = CatalogEntry->GetProtocolInfo();

    __try {
        // If the protocol meets acceptance criteria, then add it.
        if (IsProtocolInSet(Context->Protocols, ProtocolInfo)) {
            CopyProtocolInfo(ProtocolInfo, Context);
            Context->BytesUsed += sizeof(WSAPROTOCOL_INFOW);
        } //if
        return(TRUE); //Continue enumeration
    }
    __except (WS2_EXCEPTION_FILTER()) {
        Context->ErrorCode = WSAEFAULT;
        return FALSE;
    }
}


int WSAAPI
WSAEnumProtocolsW(
    IN LPINT                lpiProtocols,
    OUT LPWSAPROTOCOL_INFOW lpProtocolBuffer,
    IN OUT LPDWORD          lpdwBufferLength
    )
/*++
Routine Description:

    Retrieve information about available transport protocols.

Arguments:

    lpiProtocols     - A NULL-terminated array of protocol ids.  This parameter
                       is optional; if lpiProtocols is NULL, information on all
                       available  protocols  is returned, otherwise information
                       is  retrieved  only  for  those  protocols listed in the
                       array.

    lpProtocolBuffer - A buffer which is filled with WSAPROTOCOL_INFOW
                       structures.  See below for a detailed description of the
                       contents of the WSAPROTOCOL_INFOW structure.

    lpdwBufferLength - On input, the count of bytes in the lpProtocolBuffer
                       buffer passed to WSAEnumProtocols().  On output, the
                       minimum buffer size that can be passed to
                       WSAEnumProtocols() to retrieve all the requested
                       information.  This routine has no ability to enumerate
                       over multiple calls; the passed-in buffer must be large
                       enough to hold all entries in order for the routine to
                       succeed.  This reduces the complexity of the API and
                       should not pose a problem because the number of
                       protocols loaded on a machine is typically small.

Returns:
    The number of protocols to be reported on. Otherwise a value of
    SOCKET_ERROR is returned and a specific  stored with SetLastError().
--*/
{
    INT                          ReturnCode;
    PDPROCESS                    Process;
    PDTHREAD                     Thread;
    INT                          ErrorCode;
    PDCATALOG                    Catalog;
    PROTOCOL_ENUMERATION_CONTEXT EnumerationContext;


    ErrorCode = PROLOG(&Process, &Thread);

    if (ErrorCode != ERROR_SUCCESS) {
        SetLastError(ErrorCode);
        return(SOCKET_ERROR);
    } //if

    // Setup the enumeration context structure to hand the the
    // protocol catalog iterator.
    EnumerationContext.Protocols = lpiProtocols;
    EnumerationContext.ProtocolBuffer = lpProtocolBuffer;
    if (lpProtocolBuffer) {
        __try {
            EnumerationContext.BufferLength = *lpdwBufferLength;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            SetLastError (WSAEFAULT);
            return SOCKET_ERROR;
        }
    }
    else {
        EnumerationContext.BufferLength = 0;
    }

    EnumerationContext.BytesUsed = 0;
    EnumerationContext.ProtocolCount= 0;
    EnumerationContext.ErrorCode = ERROR_SUCCESS;

    Catalog = Process->GetProtocolCatalog();
    assert(Catalog);

    // Start the iteration through the catalog. All the real work is
    // done in ProtocolIterationProc
    Catalog->EnumerateCatalogItems(ProtocolIterationProcAPI,
                                   &EnumerationContext);
    if (EnumerationContext.ErrorCode==ERROR_SUCCESS) {
        ReturnCode = EnumerationContext.ProtocolCount;

        __try {
            if ( EnumerationContext.BufferLength < EnumerationContext.BytesUsed) {
                *lpdwBufferLength = EnumerationContext.BytesUsed;
                SetLastError(WSAENOBUFS);
                ReturnCode = SOCKET_ERROR;
            } //if
        }
        __except (WS2_EXCEPTION_FILTER()) {
            SetLastError (WSAEFAULT);
            ReturnCode = SOCKET_ERROR;
        }
    }
    else {
        SetLastError (EnumerationContext.ErrorCode);
        ReturnCode = SOCKET_ERROR;
    }

    return(ReturnCode);
}



PDCATALOG
OpenInitializedCatalog()
{
    INT ReturnCode;
    PDCATALOG protocol_catalog;
    HKEY RegistryKey = 0;

    //
    // Build the protocol catalog
    //
    protocol_catalog = new(DCATALOG);
    if (protocol_catalog) {
        TRY_START(mem_guard){
            RegistryKey = OpenWinSockRegistryRoot();
            if (!RegistryKey) {
                DEBUGF(
                    DBG_ERR,
                    ("OpenWinSockRegistryRoot Failed \n"));
                TRY_THROW(mem_guard);
            } //if

            ReturnCode = protocol_catalog->InitializeFromRegistry(
                                RegistryKey,        // ParentKey
                                NULL                // ChangeEvent
                                );
            if (ERROR_SUCCESS != ReturnCode) {
                DEBUGF(
                    DBG_ERR,
                    ("dcatalog InitializeFromRegistry Failed\n"));
                TRY_THROW(mem_guard);
            } //if

        }TRY_CATCH(mem_guard) {

            delete(protocol_catalog);
            protocol_catalog = NULL;
        }TRY_END(mem_guard);

        if (RegistryKey) {
            LONG close_result;
            close_result = RegCloseKey(
                RegistryKey);  // hkey
            assert(close_result == ERROR_SUCCESS);
        } // if
    }
    else {
        DEBUGF(
            DBG_ERR,
            ("Failed to allocate dcatalog object\n"));
    } //if

    return(protocol_catalog);
}



int WSPAPI
WSCEnumProtocols(
    IN LPINT                lpiProtocols,
    OUT LPWSAPROTOCOL_INFOW lpProtocolBuffer,
    IN OUT LPDWORD          lpdwBufferLength,
    LPINT lpErrno
    )
/*++
Routine Description:

    Retrieve information about available transport protocols.

Arguments:

    lpiProtocols     - A NULL-terminated array of protocol ids.  This parameter
                       is optional; if lpiProtocols is NULL, information on all
                       available  protocols  is returned, otherwise information
                       is  retrieved  only  for  those  protocols listed in the
                       array.

    lpProtocolBuffer - A buffer which is filled with WSAPROTOCOL_INFOW
                       structures.  See below for a detailed description of the
                       contents of the WSAPROTOCOL_INFOW structure.

    lpdwBufferLength - On input, the count of bytes in the lpProtocolBuffer
                       buffer passed to WSAEnumProtocols().  On output, the
                       minimum buffer size that can be passed to
                       WSAEnumProtocols() to retrieve all the requested
                       information.  This routine has no ability to enumerate
                       over multiple calls; the passed-in buffer must be large
                       enough to hold all entries in order for the routine to
                       succeed.  This reduces the complexity of the API and
                       should not pose a problem because the number of
                       protocols loaded on a machine is typically small.

Returns:
    The number of protocols to be reported on. Otherwise a value of
    SOCKET_ERROR is returned and a specific error code is returned in lpErrno
--*/
{
    INT                          ReturnCode;
    PDCATALOG                    Catalog;
    PROTOCOL_ENUMERATION_CONTEXT EnumerationContext;


    Catalog = OpenInitializedCatalog();
    if (Catalog==NULL) {
        __try {
            *lpErrno = WSAENOBUFS;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            // Not much we can do here
        }
        return SOCKET_ERROR;
    }

    // Setup the enumeration context structure to hand the the
    // protocol catalog iterator.
    EnumerationContext.Protocols = lpiProtocols;
    EnumerationContext.ProtocolBuffer = lpProtocolBuffer;
    if (lpProtocolBuffer) {
        __try {
            EnumerationContext.BufferLength = *lpdwBufferLength;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            SetLastError (WSAEFAULT);
            return SOCKET_ERROR;
        }
    }
    else {
        EnumerationContext.BufferLength = 0;
    }
    EnumerationContext.BytesUsed = 0;
    EnumerationContext.ProtocolCount= 0;
    EnumerationContext.ErrorCode = ERROR_SUCCESS;

    Catalog->EnumerateCatalogItems(ProtocolIterationProcSPI,
                                   &EnumerationContext);
    ReturnCode = EnumerationContext.ProtocolCount;
    if (EnumerationContext.ErrorCode==ERROR_SUCCESS) {
        __try {
            // Start the iteration through the catalog. All the real work is
            // done in ProtocolIterationProc

            if ( EnumerationContext.BufferLength < EnumerationContext.BytesUsed) {
                *lpdwBufferLength = EnumerationContext.BytesUsed;
                *lpErrno = WSAENOBUFS;
                ReturnCode = SOCKET_ERROR;
            } //if
        }
        __except (WS2_EXCEPTION_FILTER()) {
            __try {
                *lpErrno = WSAEFAULT;
            }
            __except (WS2_EXCEPTION_FILTER()) {
                // Not much we can do here
            }
            ReturnCode = SOCKET_ERROR;
        }
    }
    else {
        ReturnCode = SOCKET_ERROR;
        __try {
            *lpErrno = EnumerationContext.ErrorCode;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            // Not much we can do here
        }
    }

    delete(Catalog);
    return(ReturnCode);
}



static
BOOL
PathIterationProc(
    IN OUT PVOID IterationContext,
    IN PPROTO_CATALOG_ITEM  CatalogEntry
    )
/*++
Routine Description:

    This function is the enumeration procedure passed to the protocol catalog
    enumeration function.

Arguments:

    IterationContext - Passback value from call to
                       DCATALOG::EnumerateCatalogItems();

    CatalofEntry - A pointer to a PROTO_CATALOG_ITEM.

Returns:
    TRUE to continue the enumeration of the protocol catalog

--*/
{
    PPATH_ENUMERATION_CONTEXT Context;
    LPWSAPROTOCOL_INFOW ProtocolInfo;
    INT result;

    Context = (PPATH_ENUMERATION_CONTEXT)IterationContext;
    ProtocolInfo = CatalogEntry->GetProtocolInfo();

    // If  the  entry  is  the  correct  one,  update  the context and stop the
    // enumeration.
    if( ProtocolInfo->ProviderId == Context->ProviderId ) {
        PCHAR Path;
        INT   PathLen;

        Path =  CatalogEntry->GetLibraryPath();
        PathLen = lstrlen(Path) + 1;
        Context->CharsNeeded = PathLen;
        Context->ProviderFound = TRUE;
        if (PathLen <= Context->PathBufferLength)
        {
            __try {
                result = MultiByteToWideChar(
                             CP_ACP,
                             0,
                             Path,
                             -1,
                             Context->PathBuffer,
                             Context->PathBufferLength
                             );
            }
            __except (WS2_EXCEPTION_FILTER()) {
                Context->ErrorCode = WSAEFAULT;
            }
        } //if
        return(FALSE); //Discontinue enumeration
    } //if
    return(TRUE); //Continue enumeration
}

int WSPAPI WPUGetProviderPath(
    IN LPGUID lpProviderId,
    OUT WCHAR FAR * lpszProviderDllPath,
    IN OUT LPINT ProviderDLLPathLen,
    OUT LPINT lpErrno )
/*++
Routine Description:

    Returns the path to the provider DLL associated with a specific providerId

Arguments:

    lpProviderId - The ID of the provider to look up.

    lpszProviderDllPath - A pointer to a buffer to hold the path to the
                          provider DLL

    ProviderDLLPathLen - On input the size of lpszProviderDllPath, on output
                         the number of bytes used.

    lpErrno - A pointer to the error value of the function.

Returns:
    WSCGetProviderPath() returns 0. Otherwise, it returns SOCKET_ERROR, and a
    specific error code is available in lpErrno.
--*/
{
    BOOL                      delete_catalog=FALSE;
    PDPROCESS                 Process;
    PDTHREAD                  Thread;
    INT                       ErrorCode;
    PDCATALOG                 Catalog;
    PATH_ENUMERATION_CONTEXT  EnumerationContext;

    ErrorCode = PROLOG(&Process,&Thread);


    if (ErrorCode == ERROR_SUCCESS) {

        Catalog = Process->GetProtocolCatalog();
        assert(Catalog);
    }
    else if (ErrorCode == WSANOTINITIALISED) {

        // Take care of case when called with a prior WSAStartup()
        // as when called from WSCGetProviderPath

        Catalog = OpenInitializedCatalog();
        if (Catalog==NULL) {
            ErrorCode = WSAENOBUFS;
            goto Exit;
        }
        delete_catalog = TRUE;
    }
    else {
        goto Exit;
    }

    __try {
        // Setup the enumeration context structure to hand to the
        // protocol catalog iterator.
        EnumerationContext.ProviderId       = *lpProviderId;
        EnumerationContext.PathBuffer       = lpszProviderDllPath;
        EnumerationContext.PathBufferLength = *ProviderDLLPathLen;
        EnumerationContext.CharsNeeded      = 0;
        EnumerationContext.ProviderFound    = FALSE;
        EnumerationContext.ErrorCode        = ERROR_SUCCESS;
    }
    __except (WS2_EXCEPTION_FILTER()) {
        ErrorCode = WSAEFAULT;
        goto Exit;
    }


    // Start the iteration through the catalog. All the real work is
    // done in ProtocolIterationProc
    Catalog->EnumerateCatalogItems(PathIterationProc,
                                   &EnumerationContext);

    if (EnumerationContext.ErrorCode==ERROR_SUCCESS) {
        __try {
            if (EnumerationContext.ProviderFound) {
                if (EnumerationContext.CharsNeeded > * ProviderDLLPathLen) {
                    ErrorCode = WSAEFAULT;
                    *ProviderDLLPathLen = EnumerationContext.CharsNeeded;
                } //if
            } // if found
            else {
                ErrorCode = WSAEINVAL;
            }
        }
        __except (WS2_EXCEPTION_FILTER()) {
            ErrorCode = WSAEFAULT;
        }
    }
    else {
        ErrorCode = EnumerationContext.ErrorCode;
    }

Exit:
    if (delete_catalog) {

        delete Catalog;
    }

    if (ErrorCode==ERROR_SUCCESS)
        return ERROR_SUCCESS;
    else {
        __try {
            *lpErrno = ErrorCode;
        }
        __except (WS2_EXCEPTION_FILTER ()) {
            ;
        }
        return SOCKET_ERROR;
    }
}


int
WSPAPI
WSCGetProviderPath(
    IN LPGUID lpProviderId,
    OUT WCHAR FAR * lpszProviderDllPath,
    IN OUT LPINT ProviderDLLPathLen,
    OUT LPINT lpErrno )
{
    return WPUGetProviderPath(
               lpProviderId,
               lpszProviderDllPath,
               ProviderDLLPathLen,
               lpErrno
               );
}


int WSAAPI
WSAEnumProtocolsA(
    IN LPINT                lpiProtocols,
    OUT LPWSAPROTOCOL_INFOA lpProtocolBuffer,
    IN OUT LPDWORD          lpdwBufferLength
    )
/*++
Routine Description:

    ANSI thunk to WSAEnumProtocolsW.

Arguments:

    lpiProtocols     - A NULL-terminated array of protocol ids.  This parameter
                       is optional; if lpiProtocols is NULL, information on all
                       available  protocols  is returned, otherwise information
                       is  retrieved  only  for  those  protocols listed in the
                       array.

    lpProtocolBuffer - A buffer which is filled with WSAPROTOCOL_INFOA
                       structures.  See below for a detailed description of the
                       contents of the WSAPROTOCOL_INFOA structure.

    lpdwBufferLength - On input, the count of bytes in the lpProtocolBuffer
                       buffer passed to WSAEnumProtocols().  On output, the
                       minimum buffer size that can be passed to
                       WSAEnumProtocols() to retrieve all the requested
                       information.  This routine has no ability to enumerate
                       over multiple calls; the passed-in buffer must be large
                       enough to hold all entries in order for the routine to
                       succeed.  This reduces the complexity of the API and
                       should not pose a problem because the number of
                       protocols loaded on a machine is typically small.

Returns:
    The number of protocols to be reported on. Otherwise a value of
    SOCKET_ERROR is returned and a specific  stored with SetLastError().
--*/
{

    LPWSAPROTOCOL_INFOW ProtocolInfoW;
    DWORD NumProtocolEntries;
    DWORD ProtocolInfoWLength;
    INT result;
    INT error;
    INT i;

    __try {
        if( *lpdwBufferLength > 0 && lpProtocolBuffer!=NULL) {

            //
            // Since all of the structures are of fixed size (no embedded
            // pointers to variable-sized data) we can calculate the required
            // size of the UNICODE buffer by dividing the buffer size by the size
            // of the ANSI structure, then multiplying by the size of the
            // UNICODE structure.
            //

            NumProtocolEntries  = *lpdwBufferLength / sizeof(WSAPROTOCOL_INFOA);
            ProtocolInfoWLength = NumProtocolEntries * sizeof(WSAPROTOCOL_INFOW);

            //
            // Try to allocate the UNICODE buffer.
            //

            ProtocolInfoW = new WSAPROTOCOL_INFOW[NumProtocolEntries];

            if( ProtocolInfoW == NULL ) {

                SetLastError( WSA_NOT_ENOUGH_MEMORY );
                return SOCKET_ERROR;

            }

        } else {

            ProtocolInfoW = NULL;
            ProtocolInfoWLength = 0;

        }

        //
        // Call through to the UNICODE version.
        //

        result = WSAEnumProtocolsW(
                     lpiProtocols,
                     ProtocolInfoW,
                     &ProtocolInfoWLength
                     );

        //
        // Map the size back to ANSI.
        //

        *lpdwBufferLength = ( ProtocolInfoWLength / sizeof(WSAPROTOCOL_INFOW) ) *
            sizeof(WSAPROTOCOL_INFOA);

    }
    __except (WS2_EXCEPTION_FILTER()) {
        result = SOCKET_ERROR;
        SetLastError (WSAEFAULT);
    }

    if( result == SOCKET_ERROR ) {

        //
        // Could not store the data, probably because the supplied buffer
        // was too small.
        //

        delete ProtocolInfoW;
        return result;

    }

    //
    // OK, we've got the UNICODE data now, and we know the user's buffer
    // is sufficient to hold the ANSI structures. Map them in.
    //

    for( i = 0 ; i < result ; i++ ) {

        error = MapUnicodeProtocolInfoToAnsi(
                    ProtocolInfoW + i,
                    lpProtocolBuffer + i
                    );

        if( error != ERROR_SUCCESS ) {

            delete ProtocolInfoW;
            SetLastError( error );
            return SOCKET_ERROR;

        }

    }

    //
    // Success!
    //

    delete ProtocolInfoW;
    return result;

}   // WSAEnumProtocolsA



BOOL
CompareMemory(
    LPBYTE  p1,
    LPBYTE  p2,
    DWORD   dwSize
    )
/*++

Routine Description:

    Does a binary compare of memory at specified address.  Returns TRUE
    if memory blocks are equal, FALSE otherwise.

Arguments:

    p1     - Pointer to first memory block to compare

    p2     - Pointer to second memory block to compare

    dwSize - Size of memory blocks

--*/
{
    DWORD i;

    for (i = 0; (i < dwSize) && (*p1 == *p2); i++, p1++, p2++);

    return (i == dwSize);
}


int WSAAPI
WSAProviderConfigChange(
    IN OUT LPHANDLE lpNotificationHandle,
    IN LPWSAOVERLAPPED lpOverlapped,
    IN LPWSAOVERLAPPED_COMPLETION_ROUTINE lpCompletionRoutine
    )
/*++
Routine Description:



Arguments:

    lpNotificationHandle -

    lpOverlapped         -

    lpCompletionRoutine  -

Returns:

--*/
{
    INT                          ErrorCode;
    PDPROCESS                    Process;
    PDTHREAD                     Thread;
    HANDLE                       HelperHandle = NULL;

    if (IsBadReadPtr (lpNotificationHandle, sizeof(HANDLE)))
    {
        SetLastError (WSAEFAULT);
        return SOCKET_ERROR;
    }

    ErrorCode = PROLOG(&Process,&Thread);

    if (ErrorCode==ERROR_SUCCESS) {
        ErrorCode = Process->GetNotificationHelperDeviceID(&HelperHandle);
        if (ErrorCode==ERROR_SUCCESS) {
            __try {
                if (*lpNotificationHandle == NULL)
                {
                    ErrorCode = WahCreateNotificationHandle(
                        HelperHandle,
                        lpNotificationHandle
                        );
                }
                else
                {
                    ErrorCode = WahWaitForNotification(
                        HelperHandle,
                        *lpNotificationHandle,
                        lpOverlapped,
                        lpCompletionRoutine
                        );
                }
            }
            __except (WS2_EXCEPTION_FILTER()) {
                ErrorCode = WSAEFAULT;
            }
        }
    }


    if (ErrorCode == ERROR_SUCCESS) {
        return (ERROR_SUCCESS);
    }
    else {
        SetLastError(ErrorCode);
        return (SOCKET_ERROR);
    }
}
