/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    event.cpp

Abstract:

    This module contains the event handling functions from the winsock
    API.  This module contains the following entry points.

    WSACloseEvent()
    WSACreateEvent()
    WSAResetEvent()
    WSASetEvent()
    WSAWaintForMultipleEvents()

Author:

    Dirk Brandewie dirk@mink.intel.com  14-06-1995

Revision History:

    22-Aug-1995 dirk@mink.intel.com
        Cleanup after code review. Moved includes to precomp.h

    dirk@mink.intel.com  21-Jul-1995
        Added warnoff.h to includes

    Mark Hamilton mark_hamilton@ccm.jf.intel.com 19-07-1995

        Implemented all of the functions.

--*/
#include "precomp.h"



BOOL WSAAPI
WSACloseEvent(
              IN WSAEVENT hEvent
              )
/*++
Routine Description:

    Closes an open event object handle.

Arguments:

    hEvent - Identifies an open event object handle.

Returns:

    TRUE on success else FALSE. The error code is stored with
    SetLastError().

--*/
{

    BOOL result;

    //
    // NT will throw an exception if a stale handle is closed,
    // so protect ourselves in try/except so we can return the
    // correct error code.
    //

    __try {

        result = CloseHandle( hEvent );

    } __except( WS2_EXCEPTION_FILTER() ) {

        result = FALSE;

    }

    if( !result ) {

        SetLastError( WSA_INVALID_HANDLE );

    }

    return result;

}



WSAEVENT WSAAPI
WSACreateEvent (
                void
                )
/*++
Routine Description:

    Creates a new event object.

Arguments:

    NONE

Returns:

    The return value is the handle of the event object. If the
    function fails, the return value is WSA_INVALID_EVENT.

--*/
{
    return(CreateEvent(NULL,TRUE,FALSE,NULL));
}




BOOL WSAAPI
WSAResetEvent(
              IN WSAEVENT hEvent
              )
/*++
Routine Description:

    Resets the state of the specified event object to nonsignaled.

Arguments:

    hEvent - Identifies an open event object handle.

Returns:
    TRUE on success else FALSE. The error code is stored with
    SetErrorCode().
--*/
{
    return(ResetEvent(hEvent));
}




BOOL WSAAPI
WSASetEvent(
            IN WSAEVENT hEvent
            )
/*++
Routine Description:

    Sets the state of the specified event object to signaled.

Arguments:

     hEvent - Identifies an open event object handle.

Returns:

    TRUE on success else FALSE. The error code is stored with
    SetErrorCode().

--*/

{
  return( SetEvent(hEvent));
}





DWORD WSAAPI
WSAWaitForMultipleEvents(
                         IN DWORD cEvents,
                         IN const WSAEVENT FAR * lphEvents,
                         IN BOOL fWaitAll,
                         IN DWORD dwTimeout,
                         IN BOOL fAlertable
                         )
/*++
Routine Description:

    Returns  either when any one or when all of the specified event objects are
    in the signaled state, or when the time-out interval expires.

Arguments:

    cEvents    - Specifies  the  number  of  event  object handles in the array
                 pointed  to  by lphEvents.  The maximum number of event object
                 handles is WSA_MAXIMUM_WAIT_EVENTS.

    lphEvents  - Points to an array of event object handles.

    fWaitAll   - Specifies  the  wait type.  If TRUE, the function returns when
                 all  event  objects in the lphEvents array are signaled at the
                 same time.  If FALSE, the function returns when any one of the
                 event  objects  is  signaled.   In the latter case, the return
                 value  indicates  the  event  object  whose  state  caused the
                 function to return.

    dwTimeout  - Specifies   the   time-out  interval,  in  milliseconds.   The
                 function  returns  if the interval expires, even if conditions
                 specified  by  the  fWaitAll  parameter are not satisfied.  If
                 dwTimeout  is  zero,  the  function  tests  the  state  of the
                 specified event objects and returns immediately.  If dwTimeout
                 is   WSA_INFINITE,  the  function's  time-out  interval  never
                 expires.

    fAlertable - Specifies  whether the function returns when the system queues
                 an I/O completion routine for execution by the calling thread.
                 If  TRUE,  the  function returns and the completion routine is
                 executed.   If  FALSE,  the  function  does not return and the
                 completion  routine is not executed.  Note that this parameter
                 is ignored in Win16.

Returns:

     If the function succeeds, the return value indicates the event
     object that caused the function to return. If the function fails,
     the return value is WSA_WAIT_FAILED.
--*/
{
    return(WaitForMultipleObjectsEx(
        cEvents,
        lphEvents,
        fWaitAll,
        dwTimeout,
        fAlertable));
}
