/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: hook.c,v 1.15 2004/04/09 20:03:14 navaraf Exp $
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <user32/callback.h>
#define NTOS_MODE_USER
#include <ntos.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
STDCALL
UnhookWindowsHookEx(
  HHOOK Hook)
{
  return NtUserUnhookWindowsHookEx(Hook);
}
#if 0
BOOL
STDCALL
CallMsgFilter(
  LPMSG lpMsg,
  int nCode)
{
  UNIMPLEMENTED;
  return FALSE;
}
#endif


/*
 * @unimplemented
 */
BOOL
STDCALL
CallMsgFilterA(
  LPMSG lpMsg,
  int nCode)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL
STDCALL
CallMsgFilterW(
  LPMSG lpMsg,
  int nCode)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
LRESULT
STDCALL
CallNextHookEx(
  HHOOK Hook,
  int Code,
  WPARAM wParam,
  LPARAM lParam)
{
  return NtUserCallNextHookEx(Hook, Code, wParam, lParam);
}

STATIC
HHOOK
FASTCALL
IntSetWindowsHook(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hMod,
    DWORD dwThreadId,
    BOOL bAnsi)
{
  WCHAR ModuleName[MAX_PATH];
  UNICODE_STRING USModuleName;

  if (NULL != hMod)
    {
      if (0 == GetModuleFileNameW(hMod, ModuleName, MAX_PATH))
        {
          return NULL;
        }
      RtlInitUnicodeString(&USModuleName, ModuleName);
    }
  else
    {
      RtlInitUnicodeString(&USModuleName, NULL);
    }

  return NtUserSetWindowsHookEx(hMod, &USModuleName, dwThreadId, idHook, lpfn, bAnsi);
}

/*
 * @unimplemented
 */
HHOOK
STDCALL
SetWindowsHookW(int idHook, HOOKPROC lpfn)
{
  return IntSetWindowsHook(idHook, lpfn, NULL, 0, FALSE);
}

/*
 * @unimplemented
 */
HHOOK
STDCALL
SetWindowsHookA(int idHook, HOOKPROC lpfn)
{
  return IntSetWindowsHook(idHook, lpfn, NULL, 0, TRUE);
}

/*
 * @unimplemented
 */
BOOL
STDCALL
DeregisterShellHookWindow(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
RegisterShellHookWindow(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
UnhookWindowsHook ( int nCode, HOOKPROC pfnFilterProc )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
VOID
STDCALL
NotifyWinEvent(
	       DWORD event,
	       HWND  hwnd,
	       LONG  idObject,
	       LONG  idChild
	       )
{
  UNIMPLEMENTED;
}

/*
 * @unimplemented
 */
HWINEVENTHOOK
STDCALL
SetWinEventHook(
		UINT         eventMin,
		UINT         eventMax,
		HMODULE      hmodWinEventProc,
		WINEVENTPROC pfnWinEventProc,
		DWORD        idProcess,
		DWORD        idThread,
		UINT         dwFlags
		)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
UnhookWinEvent ( HWINEVENTHOOK hWinEventHook )
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
BOOL
STDCALL
IsWinEventHookInstalled(
    DWORD event)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HHOOK
STDCALL
SetWindowsHookExA(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hMod,
    DWORD dwThreadId)
{
  return IntSetWindowsHook(idHook, lpfn, hMod, dwThreadId, TRUE);
}


/*
 * @unimplemented
 */
HHOOK
STDCALL
SetWindowsHookExW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hMod,
    DWORD dwThreadId)
{
  return IntSetWindowsHook(idHook, lpfn, hMod, dwThreadId, FALSE);
}

NTSTATUS STDCALL
User32CallHookProcFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PHOOKPROC_CALLBACK_ARGUMENTS Common;
  LRESULT Result;
  CREATESTRUCTW Csw;
  CBT_CREATEWNDW CbtCreatewndw;
  UNICODE_STRING UString;
  CREATESTRUCTA Csa;
  CBT_CREATEWNDA CbtCreatewnda;
  ANSI_STRING AString;
  PHOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS CbtCreatewndExtra;
  WPARAM wParam;
  LPARAM lParam;

  Common = (PHOOKPROC_CALLBACK_ARGUMENTS) Arguments;

  switch(Common->HookId)
    {
    case WH_CBT:
      switch(Common->Code)
        {
        case HCBT_CREATEWND:
          CbtCreatewndExtra = (PHOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS)
                              ((PCHAR) Common + Common->lParam);
          Csw = CbtCreatewndExtra->Cs;
          if (NULL != CbtCreatewndExtra->Cs.lpszName)
            {
              Csw.lpszName = (LPCWSTR)((PCHAR) CbtCreatewndExtra
                                       + (ULONG) CbtCreatewndExtra->Cs.lpszName);
            }
          if (0 != HIWORD(CbtCreatewndExtra->Cs.lpszClass))
            {
              Csw.lpszClass = (LPCWSTR)((PCHAR) CbtCreatewndExtra
                                         + LOWORD((ULONG) CbtCreatewndExtra->Cs.lpszClass));
            }
          wParam = Common->wParam;
          if (Common->Ansi)
            {
              memcpy(&Csa, &Csw, sizeof(CREATESTRUCTW));
              if (NULL != Csw.lpszName)
                {
                  RtlInitUnicodeString(&UString, Csw.lpszName);
                  RtlUnicodeStringToAnsiString(&AString, &UString, TRUE);
                  Csa.lpszName = AString.Buffer;
                }
              if (0 != HIWORD(Csw.lpszClass))
                {
                  RtlInitUnicodeString(&UString, Csw.lpszClass);
                  RtlUnicodeStringToAnsiString(&AString, &UString, TRUE);
                  Csa.lpszClass = AString.Buffer;
                }
              CbtCreatewnda.lpcs = &Csa;
              CbtCreatewnda.hwndInsertAfter = CbtCreatewndExtra->WndInsertAfter;
              lParam = (LPARAM) &CbtCreatewnda;
            }
          else
            {
              CbtCreatewndw.lpcs = &Csw;
              CbtCreatewndw.hwndInsertAfter = CbtCreatewndExtra->WndInsertAfter;
              lParam = (LPARAM) &CbtCreatewndw;
            }
          break;
        default:
          return ZwCallbackReturn(NULL, 0, STATUS_NOT_SUPPORTED);
        }

      Result = Common->Proc(Common->Code, wParam, lParam);

      switch(Common->Code)
        {
        case HCBT_CREATEWND:
          if (Common->Ansi)
            {
              if (0 != HIWORD(Csa.lpszClass))
                {
                  RtlFreeHeap(GetProcessHeap(), 0, (LPSTR) Csa.lpszClass);
                }
              if (NULL != Csa.lpszName)
                {
                  RtlFreeHeap(GetProcessHeap(), 0, (LPSTR) Csa.lpszName);
                }
            }
          break;
        }
      break;
    default:
      return ZwCallbackReturn(NULL, 0, STATUS_NOT_SUPPORTED);
    }

  return ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS);
}
