/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/dde.c
 * PURPOSE:         Dynamic Data Exchange Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtUserDdeInitialize(DWORD Unknown0,
                    DWORD Unknown1,
                    DWORD Unknown2,
                    DWORD Unknown3,
                    DWORD Unknown4)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserDdeGetQualityOfService(IN HWND hwndClient,
                             IN HWND hWndServer,
                             OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserDdeSetQualityOfService(IN HWND hwndClient,
                             IN PSECURITY_QUALITY_OF_SERVICE pqosNew,
                             OUT PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserImpersonateDdeClientWindow(HWND hWndClient,
                                 HWND hWndServer)
{
    UNIMPLEMENTED;
    return FALSE;
}
