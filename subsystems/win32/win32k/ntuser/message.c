/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/message.c
 * PURPOSE:         Message Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

LRESULT
APIENTRY
NtUserDispatchMessage(PMSG Msg)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserGetMessage(PMSG Msg,
                 HWND hWnd,
                 UINT wMsgFilterMin,
                 UINT wMsgFilterMax)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserRealInternalGetMessage(DWORD dwUnknown1,
                             DWORD dwUnknown2,
                             DWORD dwUnknown3,
                             DWORD dwUnknown4,
                             DWORD dwUnknown5,
                             DWORD dwUnknown6)
{
    UNIMPLEMENTED;
    return 0;
}

LRESULT
APIENTRY
NtUserMessageCall(HWND hWnd,
                  UINT Msg,
                  WPARAM wParam,
                  LPARAM lParam,
                  ULONG_PTR ResultInfo,
                  DWORD dwType,
                  BOOL Ansi)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserPeekMessage(PMSG Msg,
                  HWND hWnd,
                  UINT wMsgFilterMin,
                  UINT wMsgFilterMax,
                  UINT wRemoveMsg)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserPostMessage(HWND hWnd,
                  UINT Msg,
                  WPARAM wParam,
                  LPARAM lParam)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserPostThreadMessage(DWORD idThread,
                        UINT Msg,
                        WPARAM wParam,
                        LPARAM lParam)
{
    UNIMPLEMENTED;
    return FALSE;
}

BOOL
APIENTRY
NtUserTranslateMessage(LPMSG lpMsg,
                       HKL dwhkl)
{
    UNIMPLEMENTED;
    return FALSE;
}

UINT
APIENTRY
NtUserRegisterWindowMessage(PUNICODE_STRING MessageName)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserWaitMessage(VOID)
{
    UNIMPLEMENTED;
    return FALSE;
}

DWORD
APIENTRY
NtUserRealWaitMessageEx(DWORD dwUnknown1,
                       DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserQuerySendMessage(DWORD Unknown0)
{
    UNIMPLEMENTED;
    return 0;
}
