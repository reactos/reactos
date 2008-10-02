/*
 * PROJECT:         ReactOS Win32K
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            subsystems/win32/win32k/ntuser/hooks.c
 * PURPOSE:         Hook Routines
 * PROGRAMMERS:     Stefan Ginsberg (stefan__100__@hotmail.com)
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>
#define NDEBUG
#include <debug.h>

/* PUBLIC FUNCTIONS **********************************************************/

DWORD
APIENTRY
NtUserRegisterUserApiHook(DWORD dwUnknown1,
                          DWORD dwUnknown2)
{
    UNIMPLEMENTED;
    return 0;
}

DWORD
APIENTRY
NtUserUnregisterUserApiHook(VOID)
{
    UNIMPLEMENTED;
    return 0;
}

BOOL
APIENTRY
NtUserCallMsgFilter(LPMSG msg,
                    INT code)
{
    UNIMPLEMENTED;
    return FALSE;
}

LRESULT
APIENTRY
NtUserCallNextHookEx(INT nCode,
                     WPARAM wParam,
                     LPARAM lParam,
                     BOOL bAnsi)
{
    UNIMPLEMENTED;
    return 0;
}

HWINEVENTHOOK
APIENTRY
NtUserSetWinEventHook(UINT eventMin,
                      UINT eventMax,
                      HMODULE hmodWinEventProc,
                      PUNICODE_STRING puString,
                      WINEVENTPROC lpfnWinEventProc,
                      DWORD idProcess,
                      DWORD idThread,
                      UINT dwflags)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserUnhookWinEvent(HWINEVENTHOOK hWinEventHook)
{
    UNIMPLEMENTED;
    return FALSE;
}

HHOOK
APIENTRY
NtUserSetWindowsHookAW(INT idHook,
                       HOOKPROC lpfn,
                       BOOL Ansi)
{
    UNIMPLEMENTED;
    return NULL;
}

HHOOK
APIENTRY
NtUserSetWindowsHookEx(HINSTANCE Mod,
                       PUNICODE_STRING ModuleName,
                       DWORD ThreadId,
                       INT HookId,
                       HOOKPROC HookProc,
                       BOOL Ansi)
{
    UNIMPLEMENTED;
    return NULL;
}

BOOL
APIENTRY
NtUserUnhookWindowsHookEx(HHOOK Hook)
{
    UNIMPLEMENTED;
    return FALSE;
}
