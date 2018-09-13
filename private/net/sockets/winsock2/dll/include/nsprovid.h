/*++


  Intel Corporation Proprietary Information
  Copyright (c) 1995 Intel Corporation

  This listing is supplied under the terms of a license agreement with
  Intel Corporation and may not be used, copied, nor disclosed except in
  accordance with the terms of that agreeement.


  Module Name:

      nsprovid.h

  Abstract:

      This module defines the WinSock2 class NSPROVIDER along with its
      methods.

  Author:

      Dirk Brandewie (dirk@mink.intel.com)  05-Dec-1995

  Revision History:

      09-Nov-1995 dirk@mink.intel.com
      Initial Revision
  --*/
#ifndef _NSPROVIDER_
#define _NSPROVIDER_

#include <winsock2.h>
#include <ws2spi.h>
#include "classfwd.h"
#include "llist.h"
#include "dthook.h"


class NSPROVIDER {

  public:

    NSPROVIDER();

    INT
    Initialize(
        IN LPWSTR  lpszLibFile,
        IN LPGUID  lpProviderId
        );


    INT WSAAPI
    NSPUnInstallNameSpace (
        );

//
// Client Query APIs
//

    INT WSAAPI
    NSPLookupServiceBegin(
        IN  LPWSAQUERYSETW           lpqsRestrictions,
        IN  LPWSASERVICECLASSINFOW   lpServiceClassInfo,
        IN  DWORD                    dwControlFlags,
        OUT LPHANDLE                 lphLookup
        );

    INT WSAAPI
    NSPLookupServiceNext(
        IN     HANDLE           hLookup,
        IN     DWORD            dwcontrolFlags,
        IN OUT LPDWORD          lpdwBufferLength,
        OUT    LPWSAQUERYSETW   lpqsResults
        );

    INT WSAAPI
    NSPLookupServiceEnd(
        IN HANDLE  hLookup
        );

//
// Service Address Registration and Deregistration APIs and Data Types.
//

    INT WSAAPI
    NSPSetService(
        IN  LPWSASERVICECLASSINFOW   lpServiceClassInfo,
        IN  LPWSAQUERYSETW           lpqsRegInfo,
        IN  WSAESETSERVICEOP         essOperation,
        IN  DWORD                    dwControlFlags
        );


//
// Service Installation/Removal APIs and Data Types.
//

    INT WSAAPI
    NSPInstallServiceClass(
        IN  LPWSASERVICECLASSINFOW   lpServiceClassInfo
        );

    INT WSAAPI
    NSPRemoveServiceClass(
        IN  LPGUID  lpServiceClassId
        );

    INT WSAAPI
    NSPGetServiceClassInfo(
        IN OUT  LPDWORD                 lpdwBufSize,
        IN OUT  LPWSASERVICECLASSINFOW  lpServiceClassInfo
        );


    // Provider cleanup
    INT WSAAPI
    NSPCleanup (
        );

    VOID
    Reference (
        );

    VOID
    Dereference (
        );

  private:

    // Should never be called directly, but through dereferencing.
    ~NSPROVIDER();

    LONG             m_reference_count;
    // How many time this structure was referenced

    DWORD            m_namespace_id;
    // The identifier of the namespace supported by the service provider.

    HINSTANCE        m_library_handle;
    // The handle to the service provider DLL.

    NSP_ROUTINE      m_proctable;
    // Structure containing the fuction pointers to the entry points of the
    // service provider DLL.

    GUID             m_provider_id;
    // The GUID associated with an interface in the service provider DLL.

#ifdef DEBUG_TRACING
    LPSTR            m_library_name;
    // The name of the service provider DLL.
#endif

};

inline
VOID
NSPROVIDER::Reference () {
    //
    // Object is created with reference count of 1
    // and is destroyed whenever it gets back to 0.
    //
    assert (m_reference_count>0);
    InterlockedIncrement (&m_reference_count);
}

inline
VOID
NSPROVIDER::Dereference () {
    assert (m_reference_count>0);
    if (InterlockedDecrement (&m_reference_count)==0)
        delete this;
}

//
// Client Query APIs
//


inline INT WSAAPI
NSPROVIDER::NSPLookupServiceBegin(
    IN  LPWSAQUERYSETW          lpqsRestrictions,
    IN  LPWSASERVICECLASSINFOW  lpServiceClassInfo,
    IN  DWORD                   dwControlFlags,
    OUT LPHANDLE                lphLookup
    )
/*++

Routine Description:

    NSPLookupServiceBegin() is used to initiate a client query that is
    constrained by the information contained within a WSAQUERYSET
    structure. WSALookupServiceBegin() only returns a handle, which should be
    used by subsequent calls to NSPLookupServiceNext() to get the actual
    results.



Arguments:

    lpProviderId - Contains the specific provider ID that should be used for
                   the query.

    lpqsRestrictions - contains the search criteria.

    lpServiceClassInfo - A WSASERVICECLASSINFOW structure which contains all of
                         the schema information for the service.

    dwControlFlags - controls the depth of the search:

    lphLookup - Handle to be used in subsequent calls to NSPLookupServiceNext
                in order to retrieve the results set.

Return Value:

    The function should return NO_ERROR (0) if the routine succeeds.  It should
    return SOCKET_ERROR (-1) if the routine fails
--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_NSPLookupServiceBegin,
                       &ReturnValue,
                       (LPSTR) m_library_name,
                       &m_provider_id,
                       &lpqsRestrictions,
                       &lpServiceClassInfo,
                       &dwControlFlags,
                       &lphLookup )) ) {
        return(ReturnValue);
    }

    ReturnValue =   ReturnValue = m_proctable.NSPLookupServiceBegin(
        &m_provider_id,
        lpqsRestrictions,
        lpServiceClassInfo,
        dwControlFlags,
        lphLookup
        );

    POSTAPINOTIFY((DTCODE_NSPLookupServiceBegin,
                   &ReturnValue,
                   m_library_name,
                   &m_provider_id,
                   &lpqsRestrictions,
                   &lpServiceClassInfo,
                   &dwControlFlags,
                   &lphLookup ));

    assert (m_reference_count>0);
    return(ReturnValue);


}

inline INT WSAAPI
NSPROVIDER::NSPLookupServiceNext(
    IN     HANDLE           hLookup,
    IN     DWORD            dwControlFlags,
    IN OUT LPDWORD          lpdwBufferLength,
    OUT    LPWSAQUERYSETW   lpqsResults
    )
/*++

Routine Description:

    NSPLookupServiceNext() is called after obtaining a Handle from a previous
    call to NSPLookupServiceBegin() in order to retrieve the requested service
    information.  The provider will pass back a WSAQUERYSET structure in the
    lpqsResults buffer.  The client should continue to call this API until it
    returns WSA_E_NOMORE, indicating that all of the WSAQUERYSET have been
    returned.

Arguments:

    hLookup - Handle returned from the previous call to
              NSPLookupServiceBegin().

    dwControlFlags - Flags to control the next operation.  This is currently
                     used  to indicate to the provider what to do if the result
                     set is too big for the buffer.  If on the previous call to
                     NSPLookupServiceNext() the result set was too large for
                     the buffer, the client can choose to do one of two things
                     on this call.  First, it can choose to pass a bigger
                     buffer and try again.  Second, if it cannot or is
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
                  set  in a WSAQUERYSET structure on return.

Return Value:

    The function should return NO_ERROR (0) if the routine succeeds.  It should
    return SOCKET_ERROR (-1) if the routine fails.
--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_NSPLookupServiceNext,
                       &ReturnValue,
                       m_library_name,
                       &hLookup,
                       &dwControlFlags,
                       &lpdwBufferLength,
                       &lpqsResults )) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.NSPLookupServiceNext(
        hLookup,
        dwControlFlags,
        lpdwBufferLength,
        lpqsResults
        );

    POSTAPINOTIFY((DTCODE_NSPLookupServiceNext,
                   &ReturnValue,
                   m_library_name,
                   &hLookup,
                   &dwControlFlags,
                   &lpdwBufferLength,
                   &lpqsResults  ));

    assert (m_reference_count>0);
    return(ReturnValue);
}


inline INT WSAAPI
NSPROVIDER::NSPLookupServiceEnd(
    IN HANDLE  hLookup
    )
/*++

Routine Description:

    NSPLookupServiceEnd() is called to free the handle after previous calls to
    NSPLookupServiceBegin() and NSPLookupServiceNext().  It is possible to
    receive a NSPLookupServiceEnd() call on another thread while processing a
    NSPLookupServiceNext().  This indicates that the client has cancelled the
    request, and the provider should close the handle and return from the
    NSPLookupServiceNext() call as well, setting the last error to
    WSA_E_CANCELLED.


Arguments:

    hLookup - Handle previously obtained by calling NSPLookupServiceBegin().


Return Value:

    The function should return NO_ERROR (0) if the routine succeeds.  It should
    return SOCKET_ERROR (-1) if the routine fails.
--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_NSPLookupServiceEnd,
                       &ReturnValue,
                       m_library_name,
                       &hLookup )) ) {
        return(ReturnValue);
    }

    ReturnValue =m_proctable.NSPLookupServiceEnd(
        hLookup );

    POSTAPINOTIFY((DTCODE_NSPLookupServiceEnd,
                   &ReturnValue,
                   m_library_name,
                   &hLookup ));

    assert (m_reference_count>0);
    return(ReturnValue);
}


//
// Service Address Registration and Deregistration APIs and Data Types.
//

inline INT WSAAPI
NSPROVIDER::NSPSetService(
    IN  LPWSASERVICECLASSINFOW   lpServiceClassInfo,
    IN  LPWSAQUERYSETW           lpqsRegInfo,
    IN  WSAESETSERVICEOP         essOperation,
    IN  DWORD                    dwControlFlags
    )
/*++

Routine Description:

    NSPSetService() is used to register or deregister a service instance within
    a name space.

Arguments:

    lpProviderId - Pointer to the GUID of the specific name space provider that
                   this service is being registered in.

    lpServiceClasslnfo - contains service class schema information.

    lpqsRegInfo - specifies property information to be updated upon
                  registration.

    essOperation - an enumeration.

    dwControlFlags - ControlFlags.

Return Value:

    The function should return NO_ERROR (0) if the routine succeeds.  It should
    return SOCKET_ERROR (-1) if the routine fails.
--*/
{
    INT ReturnValue=NO_ERROR;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_NSPSetService
                       &ReturnValue,
                       m_library_name,
                       (PCHAR) &m_provider_id,
                       &lpServiceClassInfo,
                       &lpqsRegInfo,
                       &essOperation,
                       &dwControlFlags )) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.NSPSetService(
        &m_provider_id,
        lpServiceClassInfo,
        lpqsRegInfo,
        essOperation,
        dwControlFlags);

    POSTAPINOTIFY((DTCODE_NSPSetService,
                   &ReturnValue,
                   m_library_name,
                   &m_provider_id,
                   &lpServiceClassInfo,
                   &lpqsRegInfo,
                   &essOperation,
                   &dwControlFlags ));

    assert (m_reference_count>0);
    return(ReturnValue);
}



//
// Service Installation/Removal APIs and Data Types.
//

inline INT WSAAPI
NSPROVIDER::NSPInstallServiceClass(
    IN  LPWSASERVICECLASSINFOW   lpServiceClassInfo
    )
/*++

Routine Description:

    NSPInstallServiceClass() is used to register service class schema within
    the name space providers.  The schema includes the class name, class id,
    and any name space specific type information that is common to all
    instances of the service, such as SAP ID or object ID.  A name space
    provider is expected to store any class info associated with that
    namespace.

Arguments:

    lpProviderId - Pointer to the GUID of the specific name space provider that
                   this  service class schema is being registered in.

    lpServiceClasslnfo - contains service class schema information.

Return Value:

    The function should return NO_ERROR (0) if the routine succeeds.  It should
    return SOCKET_ERROR (-1) if the routine fails.
--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_NSPInstallServiceClass,
                       &ReturnValue,
                       m_library_name,
                       &m_provider_id,
                       &lpServiceClassInfo )) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.NSPInstallServiceClass(
        &m_provider_id,
        lpServiceClassInfo);

    POSTAPINOTIFY(( DTCODE_NSPInstallServiceClass,
                    &ReturnValue,
                    m_library_name,
                    &m_provider_id,
                    &lpServiceClassInfo ));

    assert (m_reference_count>0);
    return(ReturnValue);
}


inline INT WSAAPI
NSPROVIDER::NSPRemoveServiceClass(
    IN  LPGUID  lpServiceClassId
    )
/*++

Routine Description:

    NSPRemoveServiceClass() is used to permanently remove a specified service
    class from the name space.

Arguments:

    lpServiceClassId    Pointer to the service class ID that is to be removed.

Return Value:

    The function should return NO_ERROR (0) if the routine succeeds.  It should
    return SOCKET_ERROR (-1) if the routine fails.
--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_NSPRemoveServiceClass,
                       &ReturnValue,
                       m_library_name,
                       &m_provider_id,
                       &lpServiceClassId )) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.NSPRemoveServiceClass(
        &m_provider_id,
        lpServiceClassId);

    POSTAPINOTIFY((DTCODE_NSPRemoveServiceClass,
                   &ReturnValue,
                   m_library_name,
                   &m_provider_id,
                   &lpServiceClassId ));

    assert (m_reference_count>0);
    return(ReturnValue);
}


inline INT WSAAPI
NSPROVIDER::NSPGetServiceClassInfo(
    IN OUT  LPDWORD                 lpdwBufSize,
    IN OUT  LPWSASERVICECLASSINFOW  lpServiceClassInfo
    )
/*++

Routine Description:

    NSPGetServiceClassInfo() is used to retrieve all of the class information
    (schema) pertaining to the service from the name space providers.  This
    call retrieves any name space specific information that is common to all
    instances of the service, including connection information for SAP, or port
    information for SAP or TCP.

Arguments:

    lpdwBufferLength - on input, the number of bytes contained in the buffer
                       pointed to by lpServiceClassInfos.  On output - if the
                       API fails, and the error is WSAEFAULT, then it contains
                       the minimum number of bytes to pass for the
                       lpServiceClassInfo to retrieve the record.

    lpServiceClasslnfo - returns service class to name space specific mapping
                         information.  The lpServiceClassId field must be
                         filled in to indicate which SERVICECLASSINFOW record
                         should be returned.

Return Value:

    The function should return NO_ERROR (0) if the routine succeeds.  It should
    return SOCKET_ERROR (-1) if the routine fails.
--*/
{
    INT ReturnValue;

    assert (m_reference_count>0);
    if (PREAPINOTIFY(( DTCODE_NSPGetServiceClassInfo,
                       &ReturnValue,
                       m_library_name,
                       &m_provider_id,
                       &lpdwBufSize,
                       &lpServiceClassInfo )) ) {
        return(ReturnValue);
    }

    ReturnValue = m_proctable.NSPGetServiceClassInfo(
        &m_provider_id,
        lpdwBufSize,
        lpServiceClassInfo);

    POSTAPINOTIFY((DTCODE_NSPGetServiceClassInfo,
                   &ReturnValue,
                   m_library_name,
                   &m_provider_id,
                   &lpdwBufSize,
                   &lpServiceClassInfo ));

    assert (m_reference_count>0);
    return(ReturnValue);
}

#endif // _NSPROVIDER_






