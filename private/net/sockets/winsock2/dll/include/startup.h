/*++


    Intel Corporation Proprietary Information
    Copyright (c) 1995 Intel Corporation

    This listing is supplied under the terms of a license agreement with
    Intel Corporation and may not be used, copied, nor disclosed except in
    accordance with the terms of that agreeement.


Module Name:

    startup.h

Abstract:

    This  module  defines  procedures  to  be called at the time of loading and
    unloading  the  WinSock  2  DLL (typically from DllMain).  These procedures
    create and destroy the Startup/Cleanup synchronization mechanism.

Author:

    Paul Drews (drewsxpa@ashland.intel.com) 31-Aug-1995

Notes:

    $Revision:   1.4  $

    $Modtime:   12 Jan 1996 15:09:00  $

Revision History:

    1995-11-04 keithmo@microsoft.com
        Added support for WPUPostMessage() upcall.

    1995-08-31 drewsxpa@ashland.intel.com
        created

--*/

#ifndef _STARTUP_
#define _STARTUP_

#include "warnoff.h"
#include <windows.h>



extern
#if defined(__cplusplus)
"C"
#endif  // defined(__cplusplus)
VOID
CreateStartupSynchronization();

typedef
BOOL
(WINAPI *PWINSOCK_POST_ROUTINE)(
    HWND hWnd,
    UINT Msg,
    WPARAM wParam,
    LPARAM lParam
    );

extern PWINSOCK_POST_ROUTINE SockPostRoutine;

PWINSOCK_POST_ROUTINE
GetSockPostRoutine(
    VOID
    );

#define GET_SOCK_POST_ROUTINE()      \
    (SockPostRoutine ? SockPostRoutine : GetSockPostRoutine())


extern
#if defined(__cplusplus)
"C"
#endif  // defined(__cplusplus)
VOID
DestroyStartupSynchronization();

#endif // _STARTUP_
