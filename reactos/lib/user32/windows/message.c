/* $Id: message.c,v 1.6 2002/06/13 20:36:40 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/message.c
 * PURPOSE:         Messages
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */
#include <windows.h>
#include <user32.h>
#include <debug.h>

LPARAM
STDCALL
GetMessageExtraInfo(VOID)
{
  return (LPARAM)0;
}

DWORD
STDCALL
GetMessagePos(VOID)
{
  return 0;
}

LONG
STDCALL
GetMessageTime(VOID)
{
  return 0;
}
WINBOOL
STDCALL
InSendMessage(VOID)
{
  return FALSE;
}

DWORD
STDCALL
InSendMessageEx(
  LPVOID lpReserved)
{
  return 0;
}
WINBOOL
STDCALL
ReplyMessage(
  LRESULT lResult)
{
  return FALSE;
}
LPARAM
STDCALL
SetMessageExtraInfo(
  LPARAM lParam)
{
  return (LPARAM)0;
}
LRESULT
STDCALL
CallWindowProcA(
  WNDPROC lpPrevWndFunc,
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
CallWindowProcW(
  WNDPROC lpPrevWndFunc,
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}


LPMSG
MsgiAnsiToUnicodeMessage(
  LPMSG AnsiMsg,
  LPMSG UnicodeMsg)
{
  /* FIXME: Convert */
  RtlMoveMemory(UnicodeMsg, AnsiMsg, sizeof(MSG));

  return UnicodeMsg;
}


LRESULT
STDCALL
DispatchMessageA(
  CONST MSG *lpmsg)
{
  MSG Msg;

  return NtUserDispatchMessage(MsgiAnsiToUnicodeMessage((LPMSG)lpmsg, &Msg));
}

LRESULT
STDCALL
DispatchMessageW(
  CONST MSG *lpmsg)
{
  return NtUserDispatchMessage((LPMSG)lpmsg);
}

WINBOOL
STDCALL
GetMessageA(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax)
{
  return NtUserGetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

WINBOOL
STDCALL
GetMessageW(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax)
{
  return NtUserGetMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax);
}

WINBOOL
STDCALL
PeekMessageA(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg)
{
  return NtUserPeekMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

WINBOOL
STDCALL
PeekMessageW(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg)
{
  return NtUserPeekMessage(lpMsg, hWnd, wMsgFilterMin, wMsgFilterMax, wRemoveMsg);
}

WINBOOL
STDCALL
PostMessageA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return NtUserPostMessage(hWnd, Msg, wParam, lParam);
}

WINBOOL
STDCALL
PostMessageW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return NtUserPostMessage(hWnd, Msg, wParam, lParam);
}

VOID
STDCALL
PostQuitMessage(
  int nExitCode)
{
}

WINBOOL
STDCALL
PostThreadMessageA(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return NtUserPostThreadMessage(idThread, Msg, wParam, lParam);
}

WINBOOL
STDCALL
PostThreadMessageW(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return NtUserPostThreadMessage(idThread, Msg, wParam, lParam);
}

LRESULT
STDCALL
SendMessageA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

WINBOOL
STDCALL
SendMessageCallbackA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData)
{
  return NtUserSendMessageCallback(
    hWnd,
    Msg,
    wParam,
    lParam,
    lpCallBack,
    dwData);
}

WINBOOL
STDCALL
SendMessageCallbackW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData)
{
  return NtUserSendMessageCallback(
    hWnd,
    Msg,
    wParam,
    lParam,
    lpCallBack,
    dwData);
}

LRESULT
STDCALL
SendMessageTimeoutA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  UINT fuFlags,
  UINT uTimeout,
  PDWORD_PTR lpdwResult)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
SendMessageTimeoutW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  UINT fuFlags,
  UINT uTimeout,
  PDWORD_PTR lpdwResult)
{
  return (LRESULT)0;
}


LRESULT
STDCALL
SendMessageW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

WINBOOL
STDCALL
SendNotifyMessageA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
SendNotifyMessageW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
TranslateMessage(
  CONST MSG *lpMsg)
{
  return NtUserTranslateMessage((LPMSG)lpMsg, 0);
}

WINBOOL
STDCALL
WaitMessage(VOID)
{
  return FALSE;
}

UINT STDCALL
RegisterWindowMessageA(LPCSTR lpString)
{
  UNICODE_STRING String;
  BOOLEAN Result;
  UINT Atom;

  Result = RtlCreateUnicodeStringFromAsciiz(&String, lpString);
  if (!Result)
    {
      return(0);
    }
  Atom = RegisterWindowMessageW(String.Buffer);
  RtlFreeUnicodeString(&String);
  return(Atom);
}

UINT STDCALL
RegisterWindowMessageW(LPCWSTR lpString)
{
  return(NtUserRegisterWindowMessage(lpString));
}


/* EOF */
