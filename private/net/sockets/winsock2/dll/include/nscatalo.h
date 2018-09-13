/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    nscatalo.h

Abstract:

    This module contains the interface to the catalog of name space providers
    for the winsock2 DLL.

Author:

    Dirk Brandewie  dirk@mink.intel.com 9-NOV-1995

Notes:

    $Revision:   1.7  $

    $Modtime:   14 Feb 1996 14:13:32  $


Revision History:

    09-NOV-1995 dirk@mink.intel.com
        Initial revision.
--*/

#ifndef _NSCATALO_
#define _NSCATALO_

#include "winsock2.h"
#include <windows.h>
#include "llist.h"


typedef
BOOL
(* NSCATALOGITERATION) (
    IN PVOID            PassBack,
    IN PNSCATALOGENTRY  CatalogEntry
    );
/*++

Routine Description:

    CATALOGITERATION  is  a place-holder for a function supplied by the client.
    The  function  is  called once for each NSPROTO_CATALOG_ITEM structure in
    the catalog while enumerating the catalog.  The client can stop the
    enumeration early by returning FALSE from the function.

Arguments:

    PassBack     - Supplies  to  the  client an uninterpreted, unmodified value
                   that  was  specified  by the client in the original function
                   that  requested  the  enumeration.   The client can use this
                   value  to  carry context between the requesting site and the
                   enumeration function.

    CatalogEntry - Supplies  to  the client a reference to a NSCATALOGENTRY
                   structure with values for this item of the enumeration.

Return Value:

    TRUE  - The  enumeration  should continue with more iterations if there are
            more structures to enumerate.

    FALSE - The enumeration should stop with this as the last iteration even if
            there are more structures to enumerate.

--*/

PNSCATALOG
OpenInitializedNameSpaceCatalog();
/*++

Routine Description:

    Creates and returns catalog object that represents current reqistry state
Arguments:
    None
Return Value:
    Catalog object or NULL if allocation or registry IO fails

--*/



class NSCATALOG
{
public:

    NSCATALOG();

    INT
    InitializeFromRegistry(
        IN  HKEY    ParentKey,
        IN  HANDLE  CatalogChangeEvent OPTIONAL
        );

    INT
    RefreshFromRegistry (
        IN  HANDLE  CatalogChangeEvent OPTIONAL
        );

    INT
    WriteToRegistry(
        );

    ~NSCATALOG();

    VOID
    EnumerateCatalogItems(
        IN NSCATALOGITERATION  Iteration,
        IN PVOID               PassBack
        );

    INT
    GetCountedCatalogItemFromProviderId(
        IN  LPGUID ProviderId,
        OUT PNSCATALOGENTRY FAR * CatalogItem
        );

    INT
    GetCountedCatalogItemFromNameSpaceId(
        IN  DWORD                 NameSpaceId,
        OUT PNSCATALOGENTRY FAR * CatalogItem
        );


    VOID
    AppendCatalogItem(
        IN  PNSCATALOGENTRY  CatalogItem
        );

    VOID
    RemoveCatalogItem(
        IN  PNSCATALOGENTRY  CatalogItem
        );

    INT WSAAPI
    GetServiceClassInfo(
        IN OUT  LPDWORD                 lpdwBufSize,
        IN OUT  LPWSASERVICECLASSINFOW  lpServiceClassInfo
        );

    INT
    LoadProvider(
        IN PNSCATALOGENTRY CatalogEntry
        );

    static
    LPSTR
    GetCurrentCatalogName(
        VOID
        );

private:

    BOOL
    OpenCatalog(
        IN  HKEY   ParentKey
        );

    VOID
    AcquireCatalogLock(
        VOID
        );

    VOID
    ReleaseCatalogLock(
        VOID
        );


    VOID
    UpdateNamespaceList (
        PLIST_ENTRY new_list
        );

    PNSPROVIDER
    GetClassInfoProvider(
        IN  DWORD BufSize,
        IN  LPWSASERVICECLASSINFOW  lpServiceClassInfo
        );


    LIST_ENTRY  m_namespace_list;
    // The head of the list of protocol catalog items

    ULONG m_num_items;
    // Number of items in this catalog.

    ULONG m_serial_num;
    // The serial number of the catalog (changes every time catalog
    // is changed in the registry)

    HKEY m_reg_key;
    // Handle of the registry key under which catalog resides.
    // We keep it open so we can get notified whenever catalog
    // changes.

    PNSPROVIDER m_classinfo_provider;

    CRITICAL_SECTION m_nscatalog_lock;

};  // class dcatalog

inline
VOID
NSCATALOG::AcquireCatalogLock(
    VOID
    )
{
    EnterCriticalSection( &m_nscatalog_lock );
}



inline
VOID
NSCATALOG::ReleaseCatalogLock(
    VOID
    )
{
    LeaveCriticalSection( &m_nscatalog_lock );
}


#endif // _NSCATALO_
