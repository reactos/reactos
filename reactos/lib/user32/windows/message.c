/* $Id: message.c,v 1.18 2003/07/10 21:04:32 chorns Exp $
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


/*
 * @unimplemented
 */
LPARAM
STDCALL
GetMessageExtraInfo(VOID)
{
  UNIMPLEMENTED;
  return (LPARAM)0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
GetMessagePos(VOID)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
LONG
STDCALL
GetMessageTime(VOID)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
InSendMessage(VOID)
{
  return FALSE;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
InSendMessageEx(
  LPVOID lpReserved)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
ReplyMessage(
  LRESULT lResult)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
LPARAM
STDCALL
SetMessageExtraInfo(
  LPARAM lParam)
{
  UNIMPLEMENTED;
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
	TempString = RtlAllocateHeap(RtlGetProcessHeap(), 0, strlen(InString) + 1);
	strcpy(TempString, InString);
	RtlInitAnsiString(&AnsiString, TempString);
	UnicodeString.Length = wParam * sizeof(WCHAR);
	UnicodeString.MaximumLength = wParam * sizeof(WCHAR);
	UnicodeString.Buffer = (PWSTR)lParam;
	if (! NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString,
	                                              &AnsiString,
	                                              FALSE)))
	  {
	    if (1 <= wParam)
	      {
		UnicodeString.Buffer[0] = L'\0';
	      }
	  }
	RtlFreeHeap(RtlGetProcessHeap(), 0, TempString);
	break;
      }
    case WM_SETTEXT:
      {
	ANSI_STRING AnsiString;
	RtlInitAnsiString(&AnsiString, (PSTR) lParam);
	RtlFreeAnsiString(&AnsiString);
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
    case WM_SETTEXT:
      {
	ANSI_STRING AnsiString;
	UNICODE_STRING UnicodeString;
	RtlInitUnicodeString(&UnicodeString, (PWSTR) *lParam);
	if (NT_SUCCESS(RtlUnicodeStringToAnsiString(&AnsiString,
	                                            &UnicodeString,
	                                            TRUE)))
	  {
	    *lParam = (LPARAM) AnsiString.Buffer;
	  }
	break;
      }
    }
  return;
}


VOID STATIC
User32FreeUnicodeConvertedMessage(UINT Msg, WPARAM wParam, LPARAM lParam)
{
  UNIMPLEMENTED;
}


VOID STATIC
User32ConvertToUnicodeMessage(UINT* Msg, WPARAM* wParam, LPARAM* lParam)
{
  UNIMPLEMENTED;
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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
    case WM_SETTEXT:
      {
	ANSI_STRING AnsiString;
	UNICODE_STRING UnicodeString;
	RtlInitAnsiString(&AnsiString, (PSTR) AnsiMsg->lParam);
	if (! NT_SUCCESS(RtlAnsiStringToUnicodeString(&UnicodeString,
	                                              &AnsiString,
	                                              TRUE)))
	  {
	  return FALSE;
	  }
	UnicodeMsg->lParam = (LPARAM) UnicodeString.Buffer;
	break;
      }
    }
  return TRUE;
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
    case WM_SETTEXT:
      {
	UNICODE_STRING UnicodeString;
	RtlInitUnicodeString(&UnicodeString, (PCWSTR) UnicodeMsg->lParam);
	RtlFreeUnicodeString(&UnicodeString);
	break;
      }
    }
  return(TRUE);
}


/*
 * @implemented
 */
LRESULT STDCALL
DispatchMessageA(CONST MSG *lpmsg)
{
  return(NtUserDispatchMessage(lpmsg));
}


/*
 * @implemented
 */
LRESULT STDCALL
DispatchMessageW(CONST MSG *lpmsg)
{
  return(NtUserDispatchMessage((LPMSG)lpmsg));
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
VOID
STDCALL
PostQuitMessage(
  int nExitCode)
{
  (void) NtUserPostMessage(NULL, WM_QUIT, nExitCode, 0);
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
LRESULT STDCALL
SendMessageW(HWND hWnd,
	     UINT Msg,
	     WPARAM wParam,
	     LPARAM lParam)
{
  return(NtUserSendMessage(hWnd, Msg, wParam, lParam));
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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


/*
 * @implemented
 */
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
  UNIMPLEMENTED;
  return (LRESULT)0;
}


/*
 * @implemented
 */
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
  UNIMPLEMENTED;
  return (LRESULT)0;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SendNotifyMessageA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
SendNotifyMessageW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
TranslateMessage(CONST MSG *lpMsg)
{
  return(NtUserTranslateMessage((LPMSG)lpMsg, 0));
}


/*
 * @implemented
 */
WINBOOL
STDCALL
WaitMessage(VOID)
{
  return NtUserWaitMessage();
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
UINT STDCALL
RegisterWindowMessageW(LPCWSTR lpString)
{
  UNICODE_STRING String;

  RtlInitUnicodeString(&String, lpString);
  return(NtUserRegisterWindowMessage(&String));
}

/* EOF */
