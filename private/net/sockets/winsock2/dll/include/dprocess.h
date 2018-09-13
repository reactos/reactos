/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    dprocess.h

Abstract:

    This header defines the "DPROCESS" class.  The DPROCESS class defines state
    variables  and operations for DPROCESS objects within the WinSock 2 DLL.  A
    DPROCESS  object  represents  all  of the information known about a process
    using the Windows Sockets API.

Author:

    Paul Drews (drewsxpa@ashland.intel.com) 7-July-1995

Notes:

    $Revision:   1.16  $

    $Modtime:   08 Mar 1996 04:58:14  $

Revision History:

    most-recent-revision-date email-name
        description

    25-July dirk@mink.intel.com
        Moved protocol catalog related items into DCATALOG. Added data
        member to contain a pointer to the protocol catalog. Removed
        provider list moved provider references to into the protocol
        catalog.

    14-July-1995  dirk@mink.intel.com
        Moved member function documentation to implementation file
        dprocess.cpp. Changed critical section data members to be
        pointers to CRITICAL_SECTION. Added inline implementations for
        the list lock/unlock member functions.

    07-09-1995  drewsxpa@ashland.intel.com
        Completed  first  complete  version with clean compile and released for
        subsequent implementation.

    7-July-1995 drewsxpa@ashland.intel.com
        Original version

--*/

#ifndef _DPROCESS_
#define _DPROCESS_

#include "winsock2.h"
#include <windows.h>
#include "llist.h"
#include "ws2help.h"
#include "classfwd.h"



class DPROCESS
{
  public:

  // Static (global-scope) member functions

    static PDPROCESS
    GetCurrentDProcess(
                        VOID
                        );
    static INT
    DProcessClassInitialize(
                            VOID
                            );

  // Normal member functions

    DPROCESS();

    INT Initialize();

    ~DPROCESS();

    INT
    DThreadAttach(
                  IN PDTHREAD NewThread
                  );
    INT
    DThreadDetach(
                  IN PDTHREAD  OldThread
                  );

    BOOL 
    DSocketDetach (
                  IN LPWSHANDLE_CONTEXT   Context
                  );
    PDCATALOG
    GetProtocolCatalog();

    PNSCATALOG
    GetNamespaceCatalog();

    INT
    GetAsyncHelperDeviceID(
                           OUT LPHANDLE HelperHandle
                           );

    INT
    GetHandleHelperDeviceID(
                           OUT LPHANDLE HelperHandle
                           );
    INT
    GetNotificationHelperDeviceID(
                           OUT LPHANDLE HelperHandle
                           );
    VOID
    IncrementRefCount();

    DWORD
    DecrementRefCount();

    BYTE
    GetMajorVersion();

    BYTE
    GetMinorVersion();

    WORD
    GetVersion();

    VOID
    SetVersion( WORD Version );

#ifndef WS2_DEBUGGER_EXTENSION
//
// Give debugger extension access to all fields
//
  private:
#endif
    VOID    LockDThreadList();
    VOID    UnLockDThreadList();
    VOID    UpdateNamespaceCatalog ();

    INT
    OpenAsyncHelperDevice(
                           OUT LPHANDLE HelperHandle
                           );

    INT
    OpenHandleHelperDevice(
                           OUT LPHANDLE HelperHandle
                           );
    INT
    OpenNotificationHelperDevice(
                           OUT LPHANDLE HelperHandle
                           );


  static PDPROCESS sm_current_dprocess;
      // A class-scope reference to the single current DPROCESS object for this
      // process.

  LONG m_reference_count;
      // The   number   of   times   this   object   has   been   refereced  by
      // WSAStarup/WSACleanup.   WSAStartup  increases the count and WSACleanup
      // decreases the count.  Declarations for lists of associated objects:

  WORD m_version;
      // The WinSock version number for this process.

  BOOLEAN m_lock_initialized;
      // For proper cleanup of critical section if initialization fails

  PDCATALOG m_protocol_catalog;
      // Reference  to  the  protocol  catalog for the process

  HANDLE  m_proto_catalog_change_event;
      // Event that keeps track of protocol catalog changes
  
  PNSCATALOG m_namespace_catalog;
      // Reference  to  the  name space  catalog for the process

  HANDLE  m_ns_catalog_change_event;
      // Event that keeps track of name space catalog changes

  // Declarations for Helper objects created on demand:

  HANDLE  m_ApcHelper;
      // Reference to the Asynchronous callback helper device.  An asynchronous
      // callback helper device is only opened on demand.

  HANDLE  m_HandleHelper;
      // Reference to the handle helper device.  A handler
      // helper device is only opened on demand.

  HANDLE  m_NotificationHelper;
      // Reference to the notification handle helper device.  A notification
      // helper device is only opened on demand.
#if 0
      // Thread list not used due to race conditions.
      // Lock is still used.
  LIST_ENTRY  m_thread_list;
#endif
  CRITICAL_SECTION  m_thread_list_lock;
  
};  // class DPROCESS



inline 
PDPROCESS
DPROCESS::GetCurrentDProcess(
    )
/*++

Routine Description:

    Retrieves  a reference to the current DPROCESS object.  Note that this is a
    "static" function with global scope instead of object-instance scope.

Arguments:

    None
Return Value:
    Returns pointer to current DPROCESS object or NULL if process has not been
    initialized yet

--*/
{
    return sm_current_dprocess;
} //GetCurrentDProcess


inline VOID
DPROCESS::IncrementRefCount(
    VOID
    )
/*++

Routine Description:

    This function increases the reference count on this object.

Arguments:

Return Value:

    NONE
--*/
{
    InterlockedIncrement(&m_reference_count);
}



inline DWORD
DPROCESS::DecrementRefCount(
    VOID
    )
/*++

Routine Description:

    This function decreases the reference count on this object.

Arguments:

Return Value:

    Returns the new value of the reference count
--*/
{
    return(InterlockedDecrement(&m_reference_count));
}



inline
BYTE
DPROCESS::GetMajorVersion()
/*++

Routine Description:

    This function returns the major WinSock version number negotiated
    at WSAStartup() time.

Arguments:

    None.

Return Value:

    Returns the major WinSock version number.

--*/
{
    assert(m_version != 0);
    return LOBYTE(m_version);
} // GetMajorVersion



inline
BYTE
DPROCESS::GetMinorVersion()
/*++

Routine Description:

    This function returns the minor WinSock version number negotiated
    at WSAStartup() time.

Arguments:

    None.

Return Value:

    Returns the minor WinSock version number.

--*/
{
    assert(m_version != 0);
    return HIBYTE(m_version);
} // GetMinorVersion



inline
WORD
DPROCESS::GetVersion()
/*++

Routine Description:

    This function returns the WinSock version number negotiated
    at WSAStartup() time.

Arguments:

    None.

Return Value:

    Returns the WinSock version number.

--*/
{
    assert(m_version != 0);
    return m_version;
} // GetVersion



inline VOID
DPROCESS::LockDThreadList()
/*++

  Routine Description:

  This  function  acquires  mutually  exclusive access to the list of DTHREAD
  objects   attached  to  the  DPROCESS  object.   The  companion  procedures
  LockDThreadList  and  UnLockDThreadList  are  used  internally  to  bracket
  operations that add and remove items from the DTHREAD list.

  NOTE:

  Use  a  Critical  Section object for best performance.  Create the Critical
  Section  object  at  DPROCESS  object initialization time and destroy it at
  DPROCESS object destruction time.

  Arguments:

  None

  Return Value:

  None
  --*/
{
    EnterCriticalSection(&m_thread_list_lock);
}


inline VOID
DPROCESS::UnLockDThreadList()
/*++

  Routine Description:

  This  function  releases  mutually  exclusive access to the list of DTHREAD
  objects   attached  to  the  DPROCESS  object.   The  companion  procedures
  LockDThreadList  and  UnLockDThreadList  are  used  internally  to  bracket
  operations that add and remove items from the DTHREAD list.

  NOTE:

  Use  a  Critical  Section object for best performance.  Create the Critical
  Section  object  at  DPROCESS  object initialization time and destroy it at
  DPROCESS object destruction time.

  Arguments:

  None

  Return Value:

  None
  --*/
{
    LeaveCriticalSection(&m_thread_list_lock);
}



inline INT
DPROCESS::GetAsyncHelperDeviceID(
    OUT LPHANDLE HelperHandle
    )
/*++

Routine Description:

    Retrieves  the  opened  Async  Helper  device  ID  required  for processing
    callbacks  in  the  overlapped  I/O  model.   The operation opens the Async
    Helper device if necessary.

Arguments:

    HelperHandle - Returns the requested Async Helper device ID.

Return Value:

    The  function  returns ERROR_SUCESS if successful, otherwise it
    returns an appropriate WinSock error code.
--*/
{
    if (m_ApcHelper) {
        *HelperHandle = m_ApcHelper;
        return ERROR_SUCCESS;
        } //if
    else {
        return OpenAsyncHelperDevice (HelperHandle);
    }
}


inline INT
DPROCESS::GetHandleHelperDeviceID(
    OUT LPHANDLE HelperHandle
    )
/*++

Routine Description:

    Retrieves  the  opened  Handle  Helper  device  ID  required  for allocation
    of socket handles for non-IFS providers.   The operation opens the Handle
    Helper device if necessary.

Arguments:

    HelperHandle - Returns the requested Handle Helper device ID.

Return Value:

    The  function  returns ERROR_SUCESS if successful, otherwise it
    returns an appropriate WinSock error code.
--*/
{
    if (m_HandleHelper) {
        *HelperHandle = m_HandleHelper;
        return ERROR_SUCCESS;
        } //if
    else {
        return OpenHandleHelperDevice (HelperHandle);
    }
}



inline INT
DPROCESS::GetNotificationHelperDeviceID(
    OUT LPHANDLE HelperHandle
    )
/*++

Routine Description:

    Retrieves  the  opened  Async  Helper  device  ID  required  for processing
    callbacks  in  the  overlapped  I/O  model.   The operation opens the Async
    Helper device if necessary.

Arguments:

    HelperHandle - Returns the requested Async Helper device ID.

Return Value:

    The  function  returns ERROR_SUCESS if successful, otherwise it
    returns an appropriate WinSock error code.
--*/
{
    if (m_NotificationHelper) {
        *HelperHandle = m_NotificationHelper;
        return ERROR_SUCCESS;
        } //if
    else {
        return OpenNotificationHelperDevice (HelperHandle);
    }
}

#endif // _DPROCESS_
