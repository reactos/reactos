/*++


  Intel Corporation Proprietary Information
  Copyright (c) 1995 Intel Corporation

  This listing is supplied under the terms of a license agreement with
  Intel Corporation and may not be used, copied, nor disclosed except in
  accordance with the terms of that agreeement.


Module Name:

    nsstate.h

Abstract:

    This  module  gives  the class definition for the NSPROVIDERSTATE object
    type.  The NSPROVIDERSTATE object holds the pointer to the provider object
    and the handle for a WSALookup{Begin/Next/End} series.

Author:

    Dirk Brandewie (dirk@mink.intel.com)  04-12-1995

Notes:

$Revision:   1.10  $

$Modtime:   15 Feb 1996 16:50:42  $


Revision History:

    04-Dec-1995 dirk@mink.intel.com
    created

--*/

#ifndef _NSPROVIDERSTATE_
#define _NSPROVIDERSTATE_

#include "classfwd.h"
#include "llist.h"
#include "nsprovid.h"

class NSPROVIDERSTATE
{
  public:

    NSPROVIDERSTATE();

    INT
    Initialize(
        PNSPROVIDER  pNamespaceProvider
        );

    INT
    WINAPI
    LookupServiceBegin(
        IN  LPWSAQUERYSETW          lpqsRestrictions,
        IN  LPWSASERVICECLASSINFOW  lpServiceClassInfo,
        IN  DWORD                   dwControlFlags
        );

    INT
    WINAPI
    LookupServiceNext(
        IN     DWORD           dwContolFlags,
        IN OUT LPDWORD         lpdwBufferLength,
        OUT    LPWSAQUERYSETW  lpqsResults
        );

    INT
    WINAPI
    LookupServiceEnd();

    INT 
    WINAPI
    SetService(
        IN  LPWSASERVICECLASSINFOW   lpServiceClassInfo,
        IN  LPWSAQUERYSETW           lpqsRegInfo,
        IN  WSAESETSERVICEOP         essOperation,
        IN  DWORD                    dwControlFlags
        );

    INT
    WINAPI
    InstallServiceClass(
        IN  LPWSASERVICECLASSINFOW   lpServiceClassInfo
        );

    INT 
    WINAPI
    RemoveServiceClass(
        IN  LPGUID  lpServiceClassId
        );

    INT
    WINAPI
    GetServiceClassInfo(
        IN OUT  LPDWORD                 lpdwBufSize,
        IN OUT  LPWSASERVICECLASSINFOW  lpServiceClassInfo
        );


    ~NSPROVIDERSTATE();

    LIST_ENTRY   m_query_linkage;
    //Public data member to support putting this object on a linked list

  private:

    PNSPROVIDER  m_provider;
    // Pointer to the NSPROVIDER object associated with this boject.

    HANDLE       m_provider_query_handle;
    // The handle returned from NSPLookupServiceBegin() to be passed to
    // NSPlookupServiceNext and NSPLookupSeviceEnd.

};  // class NSPROVIDERSTATE

inline
NSPROVIDERSTATE::NSPROVIDERSTATE()
/*++

Routine Description:

    Constructor for the NSPROVIDERSTATE object.  The first member function
    called after this must be Initialize.

Arguments:

    None

Return Value:

    Returns a pointer to a NSPROVIDERSTATE object.
--*/
{
    m_provider = NULL;
    m_provider_query_handle = NULL;
}



inline
INT
NSPROVIDERSTATE::Initialize(
    PNSPROVIDER  pNamespaceProvider
    )
/*++

Routine Description:

    This  procedure  performs  all initialization for the NSPROVIDERSTATE
    object.  This function  must  be  invoked  after the constructor, before
    any other member function is invoked.

Arguments:

    pNamespaceProvider - A pointer to a namespace provider object.

    ProviderQueryHandle - A handle to used in calls to Lookup
Return Value:

    If  the  function  is  successful,  it  returns ERROR_SUCCESS, otherwise it
    returns an appropriate WinSock 2 error code.
--*/
{
    assert (m_provider==NULL);
    pNamespaceProvider->Reference ();
    m_provider = pNamespaceProvider;
    return(ERROR_SUCCESS);
}





inline
INT
WINAPI
NSPROVIDERSTATE::LookupServiceBegin(
    IN  LPWSAQUERYSETW          lpqsRestrictions,
    IN  LPWSASERVICECLASSINFOW   lpServiceClassInfo,
    IN  DWORD                   dwControlFlags
    )
/*++

Routine Description:

    See description in NSPROVID.H

Arguments:

    None

Return Value:

    None
--*/
{
    return( m_provider->NSPLookupServiceBegin(
        lpqsRestrictions,
        lpServiceClassInfo,
        dwControlFlags,
        &m_provider_query_handle
        ));
}



inline
INT
WINAPI
NSPROVIDERSTATE::LookupServiceNext(
    IN     DWORD           dwContolFlags,
    IN OUT LPDWORD        lpdwBufferLength,
    OUT    LPWSAQUERYSETW  lpqsResults
    )
/*++

Routine Description:

    See description in NSPROVID.H

Arguments:

    None

Return Value:

    None
--*/
{
    return(m_provider->NSPLookupServiceNext(
        m_provider_query_handle,
        dwContolFlags,
        lpdwBufferLength,
        lpqsResults
        ));
}



inline
INT
WINAPI
NSPROVIDERSTATE::LookupServiceEnd()
/*++

Routine Description:

    See description in NSPROVID.H

Arguments:

    None

Return Value:

    None
--*/
{
    return(m_provider->NSPLookupServiceEnd(m_provider_query_handle));
}

inline
NSPROVIDERSTATE::~NSPROVIDERSTATE()
/*++

Routine Description:

    Denstructor for the NSPROVIDERSTATE object. 

Arguments:

    None

Return Value:

    None
--*/
{
    if (m_provider!=NULL) {
        m_provider->Dereference ();
        m_provider = NULL;
    }
}


#endif // _NSPROVIDERSTATE_




