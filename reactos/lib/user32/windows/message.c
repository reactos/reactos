/* $Id: message.c,v 1.13 2003/05/02 07:52:33 gvg Exp $
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
#include <string.h>
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

VOID STATIC
User32FreeAsciiConvertedMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
  switch(Msg)
    {
    case WM_GETTEXT:
      {
	ANSI_STRING AnsiString;
	UNICODE_STRING UnicodeString;
	LPSTR TempString;
	LPSTR InString;
	InString = (LPSTR)lParam;
	TempString = RtlAllocateHeap(RtlGetProcessHeap(), 0, strlen(InString));
	strcpy(TempString, InString);
	RtlInitAnsiString(&AnsiString, TempString);
	UnicodeString.Length = wParam;
	UnicodeString.MaximumLength = wParam;
	UnicodeString.Buffer = (PWSTR)lParam;
	RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, FALSE);
	RtlFreeHeap(RtlGetProcessHeap(), 0, TempString);
	break;
      }
    case WM_NCCREATE:
      {
	CREATESTRUCTA* Cs;

	Cs = (CREATESTRUCTA*)lParam;
	RtlFreeHeap(RtlGetProcessHeap(), 0, (LPSTR)Cs->lpszName);
	RtlFreeHeap(RtlGetProcessHeap(), 0, (LPSTR)Cs->lpszClass);
	RtlFreeHeap(RtlGetProcessHeap(), 0, Cs);
	break;
      }
    }
}

VOID STATIC
User32ConvertToAsciiMessage(UINT* Msg, WPARAM* wParam, LPARAM* lParam)
{
  switch((*Msg))
    {
    case WM_NCCREATE:
      {
	CREATESTRUCTA* CsA;
	CREATESTRUCTW* CsW;
	UNICODE_STRING UString;
	ANSI_STRING AString;

	CsW = (CREATESTRUCTW*)(*lParam);
	CsA = RtlAllocateHeap(RtlGetProcessHeap(), 0, sizeof(CREATESTRUCTA));
	memcpy(CsA, CsW, sizeof(CREATESTRUCTW));

	RtlInitUnicodeString(&UString, CsW->lpszName);
	RtlUnicodeStringToAnsiString(&AString, &UString, TRUE);
	CsA->lpszName = AString.Buffer;

	RtlInitUnicodeString(&UString, CsW->lpszClass);
	RtlUnicodeStringToAnsiString(&AString, &UString, TRUE);
	CsA->lpszClass = AString.Buffer;

	(*lParam) = (LPARAM)CsA;
	break;
      }
    }
  return;
}

VOID STATIC
User32FreeUnicodeConvertedMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
}

VOID STATIC
User32ConvertToUnicodeMessage(UINT* Msg, WPARAM* wParam, LPARAM* lParam)
{
}

LRESULT STDCALL
CallWindowProcA(WNDPROC lpPrevWndFunc,
		HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam)
{
  if (IsWindowUnicode(hWnd))
    {
      LRESULT Result;
      User32ConvertToUnicodeMessage(&Msg, &wParam, &lParam);
      Result = lpPrevWndFunc(hWnd, Msg, wParam, lParam);
      User32FreeUnicodeConvertedMessage(Msg, wParam, lParam);
      return(Result);
    }
  else
    {
      return(lpPrevWndFunc(hWnd, Msg, wParam, lParam));
    }
}

LRESULT STDCALL
CallWindowProcW(WNDPROC lpPrevWndFunc,
		HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam)
{
  if (!IsWindowUnicode(hWnd))
    {
      LRESULT Result;
      User32ConvertToAsciiMessage(&Msg, &wParam, &lParam);
      Result = lpPrevWndFunc(hWnd, Msg, wParam, lParam);
      User32FreeAsciiConvertedMessage(Msg, wParam, lParam);
      return(Result);
    }
  else
    {
      return(lpPrevWndFunc(hWnd, Msg, wParam, lParam));
    }
}


BOOL
MsgiAnsiToUnicodeMessage(LPMSG UnicodeMsg, LPMSG AnsiMsg)
{
  *UnicodeMsg = *AnsiMsg;
  switch (AnsiMsg->message)
    {
    case WM_GETTEXT:
      {
	UnicodeMsg->wParam = UnicodeMsg->wParam / 2;
	break;
      }
    }
  return(TRUE);
}

BOOL
MsgiAnsiToUnicodeReply(LPMSG UnicodeMsg, LPMSG AnsiMsg, LRESULT Result)
{
  switch (AnsiMsg->message)
    {
    case WM_GETTEXT:
      {
	ANSI_STRING AnsiString;
	UNICODE_STRING UnicodeString;
	LPWSTR TempString;
	LPWSTR InString;
	InString = (LPWSTR)UnicodeMsg->lParam;
	TempString = RtlAllocateHeap(RtlGetProcessHeap(), 0, 
				     wcslen(InString) * sizeof(WCHAR));
	wcscpy(TempString, InString);
	RtlInitUnicodeString(&UnicodeString, TempString);
	AnsiString.Length = AnsiMsg->wParam;
	AnsiString.MaximumLength = AnsiMsg->wParam;
	AnsiString.Buffer = (PSTR)AnsiMsg->lParam;
	RtlUnicodeStringToAnsiString(&AnsiString, &UnicodeString, FALSE);
	RtlFreeHeap(RtlGetProcessHeap(), 0, TempString);
	break;
      }
    }
  return(TRUE);
}


LRESULT STDCALL
DispatchMessageA(CONST MSG *lpmsg)
{
  return(NtUserDispatchMessage(lpmsg));
}

LRESULT STDCALL
DispatchMessageW(CONST MSG *lpmsg)
{
  return(NtUserDispatchMessage((LPMSG)lpmsg));
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
  (void) NtUserPostMessage(NULL, WM_QUIT, nExitCode, 0);
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

LRESULT STDCALL
SendMessageW(HWND hWnd,
	     UINT Msg,
	     WPARAM wParam,
	     LPARAM lParam)
{
  return(NtUserSendMessage(hWnd, Msg, wParam, lParam));
}


LRESULT STDCALL
SendMessageA(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  MSG AnsiMsg;
  MSG UcMsg;
  LRESULT Result;

  AnsiMsg.hwnd = hWnd;
  AnsiMsg.message = Msg;
  AnsiMsg.wParam = wParam;
  AnsiMsg.lParam = lParam;

  if (!MsgiAnsiToUnicodeMessage(&UcMsg, &AnsiMsg))
    {
      return(FALSE);
    }
  Result = SendMessageW(UcMsg.hwnd, UcMsg.message, UcMsg.wParam, UcMsg.lParam);
  if (!MsgiAnsiToUnicodeReply(&UcMsg, &AnsiMsg, Result))
    {
      return(FALSE);
    }
  return(Result);
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

WINBOOL STDCALL
TranslateMessage(CONST MSG *lpMsg)
{
  return(NtUserTranslateMessage((LPMSG)lpMsg, 0));
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

  Result = RtlCreateUnicodeStringFromAsciiz(&String, (PCSZ)lpString);
  if (!Result)
    {
      return(0);
    }
  Atom = NtUserRegisterWindowMessage(&String);
  RtlFreeUnicodeString(&String);
  return(Atom);
}

UINT STDCALL
RegisterWindowMessageW(LPCWSTR lpString)
{
  UNICODE_STRING String;

  RtlInitUnicodeString(&String, lpString);
  return(NtUserRegisterWindowMessage(&String));
}


/* EOF */
