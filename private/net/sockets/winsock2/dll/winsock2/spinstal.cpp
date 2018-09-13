/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    spinstall.cpp

Abstract:

    This module contains the entry points for service provider installation and
    deinstallation.

Author:

    Paul Drews (drewsxpa@ashland.intel.com) 2-Aug-1995

Notes:

    $Revision:   1.16  $

    $Modtime:   12 Jan 1996 14:55:36  $

Revision History:

    most-recent-revision-date email-name
        description

    23-Aug-1995 dirk@mink.intel.com
        Moved includes to precomp.h

    2-Aug-1995 drewsxpa@ashland.intel.com
        Original created

--*/


#include "precomp.h"


// The  following  type is used to pass context back and forth to an enumerator
// iteration procedure when attempting to match a provider.
typedef struct {
    GUID                ProviderId;
    PPROTO_CATALOG_ITEM CatalogItem;
} GUID_MATCH_CONTEXT,  FAR * PGUID_MATCH_CONTEXT;


BOOL
GuidMatcher(
    IN PVOID                PassBack,
    IN PPROTO_CATALOG_ITEM  CatalogEntry
    )
/*++

Routine Description:

    This  procedure determines if ProviderId of a passed CatalogEntry matches
    a  target  provider GUID.  If so, it sets a flag to indicate that the match
    was  found  and  returns  FALSE to terminate the enumeration.  Otherwise it
    returns TRUE and the enumeration continues.

Arguments:

    PassBack     - Supplies  a  reference  to  a  GUID_MATCH_CONTEXT structure.
                    Returns catalog entry reference if it was discovered

    CatalogEntry - Supplies  to  the client a reference to a PROTO_CATALOG_ITEM
                   structure with values for this item of the enumeration.

Return Value:

    If a match is found, the function returns FALSE to terminate the iteration,
    otherwise it returns TRUE to continue the iteration.
--*/
{
    PGUID_MATCH_CONTEXT  context;

    context = (PGUID_MATCH_CONTEXT)PassBack;

    if( context->ProviderId == *(CatalogEntry->GetProviderId()) ) {
        context->CatalogItem = CatalogEntry;
        return FALSE;  // do not continue iteration
    }

    return TRUE;  // continue iteration
}  // GuidMatcher




int
WSPAPI
WSCInstallProvider(
    IN  LPGUID lpProviderId,
    IN  const WCHAR FAR * lpszProviderDllPath,
    IN  const LPWSAPROTOCOL_INFOW lpProtocolInfoList,
    IN  DWORD dwNumberOfEntries,
    OUT LPINT lpErrno
    )
/*++

Routine Description:

    This   procedure   installs   the   specified   provider  into  the  system
    configuration  database.   After  this  call,  a  WinSock  2  DLL  instance
    initialized  via  a  first call to WSAStartup will return the new protocols
    from the WSAEnumProtocols function.

    It  is the caller's responsibility to perform required file installation or
    service provider specific configuration.

Arguments:

    lpProviderId        - Supplies a GUID giving the locally unique identifier
                          for the newly installed provider.

    lpszProviderDllPath - Supplies a reference to a fully qualified path to the
                          providers  DLL image.  This path may contain embedded
                          environment  strings  (such  as  %SystemRoot%).  Such
                          environment strings are expanded whenever the WinSock
                          2  DLL  needs  to  load  the provider DLL.  After any
                          embedded   environment   strings  are  expanded,  the
                          WinSock  2  DLL  passes the resulting string into the
                          LoadLibrary() API to load the provider into memory.

    lpProtocolInfoList  - Supplies a reference to an array of WSAPROTOCOL_INFOW
                          structures.      Each     structure     defines     a
                          protocol/address_family/socket_type  supported by the
                          provider.

    dwNumberOfEntries   - Supplies    the    number    of    entries   in   the
                          lpProtocolInfoList array.

    lpErrno             - Returns the error code.

Return Value:

    If no error occurs, WSCInstallProvider() returns ERROR_SUCCESS.  Otherwise,
    it  returns  SOCKET_ERROR, and a specific error code is returned in the int
    referenced by lpErrno.

Implementation Notes:

    open winsock registry
    create catalog from registry
    check provider name for uniqueness
    providerid = allocate provider id
    for each protocolinfo in list
        allocate catalog entry id
        write provider id and catalog entry id into protocol info
        create catalogitem from values
        append item to catalog
        check if item is for NonIFS provider
    end for
    enable non-IFS handle support if any NonIFS providers
    write catalog to registry
    close winsock registry
--*/
{
    int  errno_result;
    int  return_value;
    HKEY  registry_root;
    DWORD  entry_id;
    int pindex;
    WSAPROTOCOL_INFOW proto_info;
    BOOL NonIFS = FALSE;

    // objects protected by "try" block
    PDCATALOG            catalog = NULL;
    PPROTO_CATALOG_ITEM  item = NULL;
    PCHAR                provider_path = NULL;
    BOOL InstalledNonIFS = FALSE;

    registry_root = OpenWinSockRegistryRoot();
    if (registry_root == NULL) {
        DEBUGF(
            DBG_ERR,
            ("Opening registry root\n"));
        * lpErrno = WSANO_RECOVERY;
        return SOCKET_ERROR;
    }

    //
    // Check the current protocol catalog key. If it doesn't match
    // the expected value, blow away the old key and update the
    // stored value.
    //

    ValidateCurrentCatalogName(
        registry_root,
        WINSOCK_CURRENT_PROTOCOL_CATALOG_NAME,
        DCATALOG::GetCurrentCatalogName()
        );

    errno_result = ERROR_SUCCESS;
    return_value = ERROR_SUCCESS;

    TRY_START(guard_memalloc) {
        GUID_MATCH_CONTEXT  context;

        context.CatalogItem = NULL;
        __try {
            context.ProviderId = *lpProviderId;
            provider_path = ansi_dup_from_wcs((LPWSTR)lpszProviderDllPath);
        }
        __except (WS2_EXCEPTION_FILTER()) {
            errno_result = WSAEFAULT;
            TRY_THROW (guard_memalloc);
        }

        if (provider_path == NULL) {
            errno_result = WSA_NOT_ENOUGH_MEMORY;
            TRY_THROW(guard_memalloc);
        }

        catalog = new DCATALOG();
        if (catalog == NULL) {
            errno_result = WSA_NOT_ENOUGH_MEMORY;
            TRY_THROW(guard_memalloc);
        }

        errno_result = catalog->InitializeFromRegistry(
            registry_root,  // ParentKey
            NULL            // ChangeEvent
            );
        if (errno_result != ERROR_SUCCESS) {
            TRY_THROW(guard_memalloc);
        }

        catalog->EnumerateCatalogItems(
            GuidMatcher,         // Iteration
            & context  // PassBack
            );
        if (context.CatalogItem!=NULL) {
            errno_result = WSANO_RECOVERY;
            TRY_THROW(guard_memalloc);
        }

        for (pindex = 0; pindex < (int) dwNumberOfEntries; pindex++) {
            entry_id = catalog->AllocateCatalogEntryId();
            __try {
                proto_info = lpProtocolInfoList[pindex];
            }
            __except (WS2_EXCEPTION_FILTER()) {
                errno_result = WSAEFAULT;
                TRY_THROW(guard_memalloc);
            }

            if (!(proto_info.dwServiceFlags1 & XP1_IFS_HANDLES)) {
                NonIFS = TRUE;
            }

            proto_info.ProviderId = *lpProviderId;
            proto_info.dwCatalogEntryId = entry_id;

            item = new PROTO_CATALOG_ITEM();
            if (item == NULL) {
                errno_result = WSA_NOT_ENOUGH_MEMORY;
                TRY_THROW(guard_memalloc);
            }
            errno_result = item->InitializeFromValues(
                provider_path,  // LibraryPath
                & proto_info    // ProtoInfo
                );
            if (errno_result != ERROR_SUCCESS) {
                TRY_THROW(guard_memalloc);
            }

            catalog->AppendCatalogItem(
                item  // CatalogItem
                );
            item = NULL;  // item deletion is now covered by catalog
        }  // for pindex
    
        if (NonIFS) {
            errno_result = WahEnableNonIFSHandleSupport();
            if (errno_result==ERROR_SUCCESS)
                InstalledNonIFS = TRUE;
            else if (errno_result==ERROR_SERVICE_ALREADY_RUNNING) {
                errno_result = ERROR_SUCCESS;
            }
            else {
                TRY_THROW(guard_memalloc);
            }
        }

        errno_result = catalog->WriteToRegistry();
        if (errno_result!=ERROR_SUCCESS) {
            TRY_THROW(guard_memalloc);
        }

        delete catalog;
        delete provider_path;

    } TRY_CATCH(guard_memalloc) {
        if (errno_result == ERROR_SUCCESS) {
            errno_result = WSANO_RECOVERY;
        }
        if (item != NULL) {
            item->Dereference ();
        }
        if (catalog != NULL) {
            delete catalog;
        }
        if (provider_path != NULL) {
            delete provider_path;
        }

        if (InstalledNonIFS)
            WahDisableNonIFSHandleSupport ();
    } TRY_END(guard_memalloc);

    CloseWinSockRegistryRoot(registry_root);

    if (errno_result == ERROR_SUCCESS) {
        HANDLE hHelper;

        //
        // Alert all interested apps of change via the notification method
        //

        if (WahOpenNotificationHandleHelper( &hHelper) == ERROR_SUCCESS) {
            WahNotifyAllProcesses( hHelper );
            WahCloseNotificationHandleHelper( hHelper );
        }
        else {
            //
            // This in non-fatal and catalog was updated anyway.
            //
        }

        return ERROR_SUCCESS;
    }
    else {

        __try {
            * lpErrno = errno_result;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            // Not much we can do about this
        }
        return SOCKET_ERROR;
    }

}  // WSCInstallProvider

BOOL
NonIFSFinder(
    IN PVOID                PassBack,
    IN PPROTO_CATALOG_ITEM  CatalogEntry)
/*++

Routine Description:

    This  procedure  checks catalog for NonIFS transport service providers.
    If one is found, it clears a flag to indicate that NonIFS handle support
    should not be removed and returns FALSE to terminate the enumeration.  
    Otherwise it returns TRUE and the enumeration continues.

Arguments:

    PassBack     - Supplies a pointer to bool that servers as the flag to
                    tell if NonIFS provider is found

    CatalogEntry - Supplies  to  the client a reference to a PROTO_CATALOG_ITEM
                   structure with values for this item of the enumeration.  The
                   pointer  is  not guaranteed to be valid after this procedure
                   returns, so the client should copy data if required.

Return Value:

    If a match is found, the function returns FALSE to terminate the iteration,
    otherwise it returns TRUE.
--*/
{
    PBOOL  context;

    context = (PBOOL)PassBack;

    if( !(CatalogEntry->GetProtocolInfo()->dwServiceFlags1 & XP1_IFS_HANDLES) ) {
        *context = FALSE;
        return FALSE;  // do not continue iteration
    }

    return TRUE;  // continue iteration
}  // NonIFSFinder



int
WSPAPI
WSCDeinstallProvider(
    IN  LPGUID lpProviderId,
    OUT LPINT lpErrno
    )
/*++

Routine Description:

    This procedure removes the specified provider from the system configuration
    database.   After  this  call,  a  WinSock 2 DLL instance initialized via a
    first  call  to  WSAStartup  will no longer return the specified provider's
    protocols from the WSAEnumProtocols function.

    Any  additional  file  removal  or  service provider specific configuration
    information  removal  needed  to completely de-install the service provider
    must be performed by the caller.

Arguments:

    lpProviderId - Supplies  the  locally  unique identifier of the provider to
                   deinstall.   This  must  be  a  value previously passed to
                   WSCInstallProvider().

    lpErrno      - Returns the error code.

Return Value:

    If   no   error   occurs,   WSCDeinstallProvider()  returns  ERROR_SUCCESS.
    Otherwise,  it returns SOCKET_ERROR, and a specific error code is available
    in lpErrno.

Implementation Notes:

    open winsock registry
    create catalog from registry
    while (item = enumerate until find provider id) do
        remove item from catalog
        delete item
    end while
    write catalog to registry
    close winsock registry

--*/
{
    int  errno_result;
    int  return_value;
    HKEY  registry_root;
    BOOL  items_found;
    BOOL  DeinstallNonIFS = FALSE;

    // objects protected by "try" block
    PDCATALOG            catalog = NULL;

    registry_root = OpenWinSockRegistryRoot();
    if (registry_root == NULL) {
        DEBUGF(
            DBG_ERR,
            ("Opening registry root\n"));
        * lpErrno = WSANO_RECOVERY;
        return SOCKET_ERROR;
    }

    errno_result = ERROR_SUCCESS;
    return_value = ERROR_SUCCESS;

    TRY_START(guard_memalloc) {
        GUID_MATCH_CONTEXT  context;
        __try {
            context.ProviderId = *lpProviderId;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            errno_result = WSAEFAULT;
            TRY_THROW(guard_memalloc);
        }

        catalog = new DCATALOG();
        if (catalog == NULL) {
            errno_result = WSA_NOT_ENOUGH_MEMORY;
            TRY_THROW(guard_memalloc);
        }
        errno_result = catalog->InitializeFromRegistry(
            registry_root,  // ParentKey
            NULL            // ChangeEvent
            );
        if (errno_result != ERROR_SUCCESS) {
            TRY_THROW(guard_memalloc);
        }

        items_found = FALSE;
        do {
            context.CatalogItem = NULL;
            catalog->EnumerateCatalogItems(
                GuidMatcher,    // Iteration
                & context           // PassBack
                );
            if (context.CatalogItem!=NULL) {
                items_found = TRUE;
                //
                // Check if provider we are deinstalling is 
                // a non-ifs provider, we may want to disable
                // non-ifs support is this was the last non-ifs 
                // provider.
                //
                if (!DeinstallNonIFS) {
                    DeinstallNonIFS 
                        = !(context.CatalogItem->GetProtocolInfo()->dwServiceFlags1 
                            & XP1_IFS_HANDLES);
                }
                catalog->RemoveCatalogItem(context.CatalogItem);
                context.CatalogItem->Dereference ();
            }
        }
        while (context.CatalogItem!=NULL);

        if (! items_found) {
            errno_result = WSAEFAULT;
            TRY_THROW(guard_memalloc);
        }

        if (DeinstallNonIFS) {
            //
            // Check if there are any remaining non-IFS
            // providers left, if none left we will disable
            // non-IFS support
            //
            catalog->EnumerateCatalogItems(
                NonIFSFinder,               // Iteration
                & DeinstallNonIFS           // PassBack
                );
        }
        errno_result = catalog->WriteToRegistry();
        if (errno_result!=ERROR_SUCCESS) {
            TRY_THROW(guard_memalloc);
        }

        delete catalog;

        if (DeinstallNonIFS)
            WahDisableNonIFSHandleSupport();

    } TRY_CATCH(guard_memalloc) {
        if (errno_result == ERROR_SUCCESS) {
            errno_result = WSANO_RECOVERY;
        }
        if (catalog != NULL) {
            delete catalog;
        }
    } TRY_END(guard_memalloc);

    CloseWinSockRegistryRoot(registry_root);

    if (errno_result == ERROR_SUCCESS) {
        HANDLE hHelper;

        //
        // Alert all interested apps of change via the notification method
        //


        if (WahOpenNotificationHandleHelper( &hHelper )==ERROR_SUCCESS) {
            WahNotifyAllProcesses( hHelper );
            WahCloseNotificationHandleHelper( hHelper );
        }
        else {
            // This is non-fatal and catalog was updated anyway
        }

        return ERROR_SUCCESS;
    }
    else {

        __try {
            * lpErrno = errno_result;
        }
        __except (WS2_EXCEPTION_FILTER()) {
            // Not much we can do about this
        }
        return SOCKET_ERROR;
    }

}  // WSCDeinstallProvider



// The  following  type is used to pass context back and forth to an enumerator
// iteration procedure when removing all items form the catalog
typedef struct {
    IN  DWORD               Count;      // Number of ids / size of item array
    IN  DWORD               *CatIds;    // Array of catalog id's to match against
    OUT PPROTO_CATALOG_ITEM *Items;     // Array of item pointers to return
    OUT INT                 ErrorCode;
} PROVIDER_SNAP_CONTEXT,  FAR * PPROVIDER_SNAP_CONTEXT;




BOOL
ProviderSnap(
    IN PVOID                PassBack,
    IN PPROTO_CATALOG_ITEM  CatalogEntry)
/*++

Routine Description:
    Snaps all the catalog items in the current catalog

Arguments:

    PassBack     - Supplies  a reference to a PROVIDER_SNAP_CONTEXT structure.
                   Returns an array of items in the order specified by catalog
                   id array.

    CatalogEntry - Supplies  to  the client a reference to a PROTO_CATALOG_ITEM
                   structure with values for this item of the enumeration.  The
                   pointer  is  not guaranteed to be valid after this procedure
                   returns, so the client should copy data if required.

Return Value:

    If an item is found that could not be matched to catalog id in the array,
    the function returns FALSE to terminate the iteration,
    otherwise it returns TRUE.
--*/
{
    PPROVIDER_SNAP_CONTEXT      context;
    DWORD                       i;

    context = (PPROVIDER_SNAP_CONTEXT)PassBack;

    for (i=0; i<context->Count; i++) {
        if (context->CatIds[i]
                ==CatalogEntry->GetProtocolInfo ()->dwCatalogEntryId) {
            assert (context->Items[i]==NULL);
            context->Items[i] = CatalogEntry;
            return TRUE;
        }
    }

    return FALSE;
}  // ProviderRemover




int
WSPAPI
WSCWriteProviderOrder (
    IN LPDWORD lpwdCatalogEntryId,
    IN DWORD dwNumberOfEntries
    )
/*++

Routine Description:

    Reorder existing WinSock2 service providers.  The order of the service
    providers determines their priority in being selected for use.  The
    sporder.exe tool will show you the installed provider and their ordering,
    Alternately, WSCEnumProtocols(), in conjunction with this function,
    will allow you to write your own tool.

Arguments:

    lpwdCatalogEntryId  [in]
      An array of CatalogEntryId elements as found in the WSAPROTOCOL_INFO
      structure.  The order of the CatalogEntryId elements is the new
      priority ordering for the service providers.

    dwNumberOfEntries  [in]
      The number of elements in the lpwdCatalogEntryId array.


Return Value:

    ERROR_SUCCESS   - the service providers have been reordered.
    WSAEINVAL       - input parameters were bad, no action was taken.
    WSAEFAULT       - CatalogEnryId array is not fully contained within
                        process address space.
    WSATRY_AGAIN    - the routine is being called by another thread or process.
    any registry error code


Comments:

    Here are scenarios in which the WSCWriteProviderOrder function may fail:

      The dwNumberOfEntries is not equal to the number of registered service
      providers.

      The lpwdCatalogEntryId contains an invalid catalog ID.

      The lpwdCatalogEntryId does not contain all valid catalog IDs exactly
      1 time.

      The routine is not able to access the registry for some reason
      (e.g. inadequate user persmissions)

      Another process (or thread) is currently calling the routine.


--*/
{
    INT     errno_result;
    HKEY    registry_root;
    PPROTO_CATALOG_ITEM item;
    PPROTO_CATALOG_ITEM *items = NULL;
    DWORD   i;

    // object protected by "try" block
    PDCATALOG           catalog = NULL;


    items = new PPROTO_CATALOG_ITEM[dwNumberOfEntries];
    if (items==NULL) {
        DEBUGF(
            DBG_ERR,
            ("Allocating items array\n"));
        return WSA_NOT_ENOUGH_MEMORY;
    }

    memset (items, 0, sizeof (PPROTO_CATALOG_ITEM)*dwNumberOfEntries);
    errno_result = ERROR_SUCCESS;

    TRY_START(guard_memalloc) {
        PROVIDER_SNAP_CONTEXT context;
        registry_root = OpenWinSockRegistryRoot();
        if (registry_root == NULL) {
            DEBUGF(
                DBG_ERR,
                ("Opening registry root\n"));
            errno_result = WSANO_RECOVERY;
            TRY_THROW(guard_memalloc);
        }
        catalog = new DCATALOG();
        if (catalog == NULL) {
            errno_result = WSA_NOT_ENOUGH_MEMORY;
            TRY_THROW(guard_memalloc);
        }

        errno_result = catalog->InitializeFromRegistry(
            registry_root,  // ParentKey
            NULL            // ChangeEvent
            );
        if (errno_result != ERROR_SUCCESS) {
            TRY_THROW(guard_memalloc);
        }

        context.Items = items;
        context.CatIds = lpwdCatalogEntryId;
        context.Count = dwNumberOfEntries;
        context.ErrorCode = ERROR_SUCCESS;

        catalog->EnumerateCatalogItems(
            ProviderSnap,         // Iteration
            & context               // PassBack
            );

        if (context.ErrorCode!=ERROR_SUCCESS) {
            errno_result = context.ErrorCode;
            TRY_THROW(guard_memalloc);
        }

        for (i=0; i<dwNumberOfEntries; i++) {
            if (context.Items[i]!=NULL) {
                //
                // Remove catalog item and add it in the end.
                //
                catalog->RemoveCatalogItem (context.Items[i]);
                catalog->AppendCatalogItem (context.Items[i]);
            }
            else {
                DEBUGF (DBG_ERR,
                    ("Checking item array against catalog, item: %ld(CatId:%ld).\n",
                    i,lpwdCatalogEntryId[i]));
                errno_result = WSAEINVAL;
                TRY_THROW(guard_memalloc);
            }
        } // for i

        errno_result = catalog->WriteToRegistry();
        if (errno_result!=ERROR_SUCCESS) {
            TRY_THROW(guard_memalloc);
        }

        delete catalog;
        CloseWinSockRegistryRoot(registry_root);

    } TRY_CATCH(guard_memalloc) {
        assert (errno_result != ERROR_SUCCESS);
        if (catalog != NULL) {
            delete catalog; // This destroys the items as well
        }

        if (registry_root!=NULL) {
            CloseWinSockRegistryRoot(registry_root);
        }
    } TRY_END(guard_memalloc);

    delete items;

    if (errno_result == ERROR_SUCCESS) {
        HANDLE hHelper;

        //
        // Alert all interested apps of change via the notification method
        //

        if (WahOpenNotificationHandleHelper( &hHelper) == ERROR_SUCCESS) {
            WahNotifyAllProcesses( hHelper );
            WahCloseNotificationHandleHelper( hHelper );
        }
        else {
            //
            // This in non-fatal and catalog was updated anyway.
            //
        }
    }

    return errno_result;
}

