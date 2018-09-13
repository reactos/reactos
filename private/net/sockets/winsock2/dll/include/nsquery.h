/*++


  Intel Corporation Proprietary Information
  Copyright (c) 1995 Intel Corporation

  This listing is supplied under the terms of a license agreement with
  Intel Corporation and may not be used, copied, nor disclosed except in
  accordance with the terms of that agreeement.


Module Name:

    nsquery.h

Abstract:

    This  module  gives  the class definition for the NSQUERY object type.  The
    NSQUERY    object   holds   all   the   state   information   regarding   a
    WSALookup{Begin/Next/End}   series   of  operations.   It  supplies  member
    functions that implement the API-level operations in terms of the SPI-level
    operations.

Author:

    Paul Drews (drewsxpa@ashland.intel.com) 09-November-1995

Notes:

    $Revision:   1.8  $

    $Modtime:   15 Feb 1996 16:54:32  $

Revision History:

    most-recent-revision-date email-name
        description

    09-November-1995 drewsxpa@ashland.intel.com
        created

--*/

#ifndef _NSQUERY_
#define _NSQUERY_


#include "winsock2.h"
#include <windows.h>
#include "classfwd.h"
#include "llist.h"


#define QUERYSIGNATURE 0xbeadface
// A signature bit pattern used to validate an object of this type is still
// valid.

class NSQUERY
{
  public:

    NSQUERY();

    INT
    Initialize(
        );
  BOOL
    IsValid();


    ~NSQUERY();

    INT
    WINAPI
    LookupServiceBegin(
        IN  LPWSAQUERYSETW  lpqsRestrictions,
        IN  DWORD           dwControlFlags,
        IN PNSCATALOG       NsCatalog
        );

    INT
    WINAPI
    LookupServiceNext(
        IN     DWORD           dwControlFlags,
        IN OUT LPDWORD         lpdwBufferLength,
        IN OUT LPWSAQUERYSETW  lpqsResults
        );

    INT
    WINAPI
    LookupServiceEnd();

    BOOL
    WINAPI
    IsDeletable();


    BOOL
    RemoveProvider(
        PNSPROVIDER  pNamespaceProvider
        );

    BOOL
    AddProvider(
        PNSPROVIDER  pNamespaceProvider
        );



  private:

    PNSPROVIDERSTATE
    NextProvider(
        PNSPROVIDERSTATE Provider
        );



    CRITICAL_SECTION  m_members_guard;
    // This  critical  section  must be entered when updating the values of
    // any  of  the member variables of the NSQUERY object.  This keeps the
    // values  consistent even though there may be concurrent threads using
    // the  object  with  LookupServiceNext or LookupServiceEnd operations.
    // Do not keep this critical section entered while calling through to a
    // service provider.

    BOOL  m_shutting_down;
    // When   set  to  true,  the  object  will  return  WSA_E_NOMORE  from
    // LookupServiceNext.   Some  threads  may  not  yet have returned from
    // LookupServiceNext.

    LONG  m_calls_in_progress;
    // The number of threads currently using the object.  Used to determine
    // when the object can be deleted.

    LIST_ENTRY  m_provider_list;
    // The  ordered  list  of  remaining  providers to which the LookupNext
    // operation  can be directed.  A provider is deleted from the front of
    // the  list  as  WSA_E_NOMORE  is first encountered from the provider.
    // The   actual   type   of   the   list  entries  is  private  to  the
    // implementation.

    PNSPROVIDERSTATE  m_current_provider;
    // This  keeps  track  of  the  sequence  number  of  the current first
    // provider  in  the  provider  list.   When  a  LookupNext  encounters
    // WSA_E_NOMORE, this number is compared against the number that was at
    // the  start  of  the operation.  The provider list is updated only if
    // these  two  numbers  are  equal.  This covers the case where several
    // threads are doing concurrent LookupNext operations.

#ifdef RASAUTODIAL
    LPWSAQUERYSETW m_query_set;
    // The LPWSAQUERYSET structure passed in to LookupServiceBegin, in case
    // we need to restart the query (call LookupServiceBegin).

    DWORD m_control_flags;
    // The control flags of the query, in case we have to restart the query
    // (call LookupServiceBegin) due to an autodial attempt.

    PNSCATALOG m_catalog;
    // The catalog of the original query, in case we have to restart the
    // query (call LookupServiceBegin), due to an autodial attempt.

    BOOL m_restartable;
    // TRUE if no results have been returned for this query; FALSE
    // otherwise.
#endif // RASAUTODIAL

    DWORD m_signature;
    // The signature of the object.

};  // class NSQUERY

#endif // _NSQUERY_

