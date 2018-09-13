/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    dcatitem.h

Abstract:

    This  file  contains the class definition for the PROTO_CATALOG_ITEM class.
    This  class  defines the interface to the entries that can be installed and
    retrieved in the protocol catalog.

Author:

    Paul Drews (drewsxpa@ashland.intel.com) 31-July-1995

Notes:

    $Revision:   1.7  $

    $Modtime:   12 Jan 1996 15:09:02  $

Revision History:

    most-recent-revision-date email-name
        description

    07-31-1995 drewsxpa@ashland.intel.com
        Created  original  version  from  definitions  separated  out  from the
        dcatalog module.

--*/

#ifndef _DCATITEM_
#define _DCATITEM_

#include "winsock2.h"
#include <windows.h>
#include "llist.h"
#include "classfwd.h"


class PROTO_CATALOG_ITEM {
public:

    PROTO_CATALOG_ITEM();

    INT
    InitializeFromRegistry(
        IN  HKEY  ParentKey,
        IN  INT   SequenceNum
        );

    INT
    InitializeFromValues(
        IN  LPSTR               LibraryPath,
        IN  LPWSAPROTOCOL_INFOW ProtoInfo
        );

    LPWSAPROTOCOL_INFOW
    GetProtocolInfo();

    LPGUID
    GetProviderId();

    PCHAR
    GetLibraryPath();

    PDPROVIDER
    GetProvider();

    INT WriteToRegistry(
        IN  HKEY  ParentKey,
        IN  INT   SequenceNum
        );


    VOID
    Reference ();

    VOID
    Dereference ();

private:

    INT
    IoRegistry(
        IN  HKEY  EntryKey,
        IN  BOOL  IsRead
        );


    // Should never be called directly, but through dereferencing.
    ~PROTO_CATALOG_ITEM();

friend class DCATALOG;  // So it can access some of the private fields 
                        // and methods below.
    VOID
    SetProvider(
        IN  PDPROVIDER  Provider
        );

    LIST_ENTRY     m_CatalogLinkage;
    // Used  to  link  items  in  catalog.

    LONG        m_reference_count;
    // This object's reference count

    PDPROVIDER  m_Provider;
    // Pointer to the dprovider object attached to this catalog entry.

    WSAPROTOCOL_INFOW m_ProtoInfo;
    // The cataloged WSAPROTOCOL_INFOW structure.  This is typically used for
    // comparison  when  selecting  a  provider by address family, socket
    // type, etc.

    char m_LibraryPath[MAX_PATH];
    // Fully qualified path to the provider's DLL image.


};  // class PROTO_CATALOG_ITEM

inline
VOID
PROTO_CATALOG_ITEM::Reference () {
    //
    // Object is created with reference count of 1
    // and is destroyed whenever it gets back to 0.
    //
    assert (m_reference_count>0);
    InterlockedIncrement (&m_reference_count);
}


inline
VOID
PROTO_CATALOG_ITEM::Dereference () {
    assert (m_reference_count>0);
    if (InterlockedDecrement (&m_reference_count)==0)
        delete this;
}

inline
LPWSAPROTOCOL_INFOW
PROTO_CATALOG_ITEM::GetProtocolInfo()
/*++

Routine Description:

    This  procedure  retrieves a reference to the protocol info associated with
    the  catalog  item.   Note  that  the  reference is to storage owned by the
    catalog item.  It is the caller's responsibility to make sure the reference
    is no longer used after the catalog item is destroyed.

Arguments:

    None

Return Value:

    Returns a pointer to the associated protocol info.
--*/
{
    return(& m_ProtoInfo);
}  // GetProtocolInfo



inline
LPGUID
PROTO_CATALOG_ITEM::GetProviderId()
/*++

Routine Description:

    This procedure retrieves a the unique ID of the provider associated with
    the catalog item.

Arguments:

    None

Return Value:

    Returns the provider ID (a GUID).
--*/
{
    return &m_ProtoInfo.ProviderId;
}  // GetProviderId



inline
PCHAR
PROTO_CATALOG_ITEM::GetLibraryPath()
/*++

Routine Description:

    This    procedure   retrieves   a   reference   to   the   zero-terminated,
    fully-qualified  path  of  the library that is the service provider for the
    protocol  associated  with the catalog item.  Note that the reference is to
    storage  owned  by  the catalog item.  It is the caller's responsibility to
    make  sure  the  reference  is  no  longer  used  after the catalog item is
    destroyed.

Arguments:

    None

Return Value:

    Returns a pointer to the library path string.
--*/
{
    assert(m_LibraryPath[0] != '\0');
    return(m_LibraryPath);
}  // GetLibraryPath

inline
PDPROVIDER
PROTO_CATALOG_ITEM::GetProvider()
/*++

Routine Description:

    This  procedure  retrieves  a  reference to the DPROVIDER associated with a
    catalog  entry.  Note that the reference may be NULL if no provider has yet
    been loaded for this protocol.

Arguments:

    None

Return Value:

    Returns  the  current provider reference, or NULL if there is no associated
    provider.
--*/
{
    return(m_Provider);
}  // GetProvider

#endif // _DCATITEM_
