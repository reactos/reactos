/* $Id: message.c,v 1.2 2001/12/20 03:56:10 dwelch Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Messages
 * FILE:             subsys/win32k/ntuser/message.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/window.h>
#include <include/class.h>
#include <include/error.h>
#include <include/object.h>
#include <include/winsta.h>

#define NDEBUG
#include <debug.h>


NTSTATUS
InitMessageImpl(VOID)
{
  return STATUS_SUCCESS;
}

NTSTATUS
CleanupMessageImpl(VOID)
{
  return STATUS_SUCCESS;
}


LRESULT
STDCALL
NtUserDispatchMessage(
  LPMSG lpmsg)
{
  UNIMPLEMENTED

  return 0;
}

BOOL STDCALL
NtUserGetMessage(LPMSG lpMsg,
		 HWND hWnd,
		 UINT wMsgFilterMin,
		 UINT wMsgFilterMax)
{
  return FALSE;
}

DWORD
STDCALL
NtUserMessageCall(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5,
  DWORD Unknown6)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserPeekMessage(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserPostMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserPostThreadMessage(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserQuerySendMessage(
  DWORD Unknown0)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserSendMessageCallback(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserSendNotifyMessage(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserTranslateMessage(
  LPMSG lpMsg,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

BOOL
STDCALL
NtUserWaitMessage(VOID)
{
  UNIMPLEMENTED

  return 0;
}

/* EOF */
