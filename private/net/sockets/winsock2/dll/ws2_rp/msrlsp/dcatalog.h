//---------------------------------------------------------------------------
//  Copyright (c) 1997 Microsoft Corporation
//  Copyright c 1996 Intel Corporation
//
//  dcatalog.h
//
//  This  module  contains  the  interface  to  the  catalog  of  protocol_info
//  structures and their associated providers.
//
//  Revision History:
//
//  edwardr    11-09-97    Initial version.
//
//---------------------------------------------------------------------------

#ifndef _DCATALOG_
#define _DCATALOG_

#include <windows.h>
#include "fwdref.h"
#include "llist.h"


typedef
BOOL
(* CATALOGITERATION) (
    IN DWORD                PassBack,
    IN PPROTO_CATALOG_ITEM  CatalogEntry
    );
/*++

Routine Description:

    CATALOGITERATION  is  a place-holder for a function supplied by the client.
    The  function  is  called once for each PROTO_CATALOG_ITEM structure in the
    catalog while enumerating the catalog.  The client can stop the enumeration
    early by returning FALSE from the function.

    Note  that  the RPROVIDER associated with an enumerated DPROTO_CATALOG_ITEM
    may  be  NULL.   To retrieve DPROTO_CATALOG_ITEM structure that has had its
    RPROVIDER      loaded      and      initialized,      you      can      use
    GetCatalogItemFromCatalogEntryId.

Arguments:

    PassBack     - Supplies  to  the  client an uninterpreted, unmodified value
                   that  was  specified  by the client in the original function
                   that  requested  the  enumeration.   The client can use this
                   value  to  carry context between the requesting site and the
                   enumeration function.

    CatalogEntry - Supplies  to  the client a reference to a PROTO_CATALOG_ITEM
                   structure with values for this item of the enumeration. 

Return Value:

    TRUE  - The  enumeration  should continue with more iterations if there are
            more structures to enumerate.

    FALSE - The enumeration should stop with this as the last iteration even if
            there are more structures to enumerate.

--*/




class DCATALOG
{
public:

    DCATALOG();

    INT
    Initialize();

    ~DCATALOG();

    VOID
    EnumerateCatalogItems(
        IN CATALOGITERATION  Iteration,
        IN DWORD             PassBack
        );

    INT
    FindNextProviderInChain(
        IN  LPWSAPROTOCOL_INFOW lpLocalProtocolInfo,
        OUT LPDWORD             LocalProviderCatalogEntryId,
        OUT LPDWORD             NextProviderCatalogEntryId
        );

    INT
    GetCatalogItemFromCatalogEntryId(
        IN  DWORD                     CatalogEntryId,
        OUT PPROTO_CATALOG_ITEM FAR * CatalogItem
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
    AppendCatalogItem(
        IN  PPROTO_CATALOG_ITEM  CatalogItem
        );

    VOID
    RemoveCatalogItem(
        IN  PPROTO_CATALOG_ITEM  CatalogItem
        );

private:


    INT
    LoadProvider(
        IN PPROTO_CATALOG_ITEM CatalogEntry,
        OUT PRPROVIDER FAR* Provider
        );


    LIST_ENTRY  m_protocol_list;
    // The head of the list of protocol catalog items

    HANDLE  m_catalog_registry_mutex;
    // a  mutex  that assures serialization of operations involving the copy of
    // the catalog residing in the registry.  All catalog objects open the same
    // underlying  system mutex by name, so cross-process synchronization using
    // this mutex can be performed.

    CRITICAL_SECTION m_catalog_lock;
    // A critical section object protecting this class.

    DWORD m_num_items;
    // Number of items in this catalog.

    BOOL m_catalog_initialized;
    // Set to TRUE after InitializeEmptyCatalog() has succeeded.

};  // class dcatalog

inline
VOID
DCATALOG::AcquireCatalogLock(
    VOID
    )
/*++

Routine Description:

    Acquires the critical section used to protect the list of catalog items.

Arguments:

    None

Return Value:

    None
--*/
{
    EnterCriticalSection( &m_catalog_lock );
}

inline
VOID
DCATALOG::ReleaseCatalogLock(
    VOID
    )
/*++

Routine Description:

    Releases the critical section used to protect the list of catalog items.

Arguments:


Return Value:

    None
--*/
{
    LeaveCriticalSection( &m_catalog_lock );
}


#endif // _DCATALOG
