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
/*
 *
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/input.c
 * PURPOSE:         Input
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <user32.h>

#include <wine/debug.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

/* PRIVATE FUNCTIONS *********************************************************/

static
DWORD
FASTCALL
GetMaskFromEvent(DWORD Event)
{
  DWORD Ret = 0;

  if ( Event > EVENT_OBJECT_STATECHANGE )
  {
    if ( Event == EVENT_OBJECT_LOCATIONCHANGE ) return SRV_EVENT_LOCATIONCHANGE;
    if ( Event == EVENT_OBJECT_NAMECHANGE )     return SRV_EVENT_NAMECHANGE;
    if ( Event == EVENT_OBJECT_VALUECHANGE )    return SRV_EVENT_VALUECHANGE;
    return SRV_EVENT_CREATE;
  }

  if ( Event == EVENT_OBJECT_STATECHANGE ) return SRV_EVENT_STATECHANGE;

  Ret = SRV_EVENT_RUNNING;

  if ( Event < EVENT_SYSTEM_MENUSTART )    return SRV_EVENT_CREATE;

  if ( Event <= EVENT_SYSTEM_MENUPOPUPEND )
  {
    Ret = SRV_EVENT_MENU;
  }
  else
  {
    if ( Event <= EVENT_CONSOLE_CARET-1 )         return SRV_EVENT_CREATE;
    if ( Event <= EVENT_CONSOLE_END_APPLICATION ) return SRV_EVENT_END_APPLICATION;
    if ( Event != EVENT_OBJECT_FOCUS )            return SRV_EVENT_CREATE;
  }
  return Ret;
}

static
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


/* FUNCTIONS *****************************************************************/

#if 0
BOOL
WINAPI
CallMsgFilter(
  LPMSG lpMsg,
  int nCode)
{
  UNIMPLEMENTED;
  return FALSE;
}
#endif

/*
 * @implemented
 */
BOOL
WINAPI
CallMsgFilterA(
  LPMSG lpMsg,
  int nCode)
{
   BOOL ret = FALSE;

  if (nCode != HCBT_CREATEWND) ret = NtUserCallMsgFilter((LPMSG) lpMsg, nCode);
  else
     {
        UNICODE_STRING usBuffer;
        CBT_CREATEWNDA *cbtcwA = (CBT_CREATEWNDA *)lpMsg->lParam;
        CBT_CREATEWNDW cbtcwW;
        CREATESTRUCTW csW;
	MSG Msg;

	Msg.hwnd = lpMsg->hwnd;
	Msg.message = lpMsg->message;
	Msg.time = lpMsg->time;
	Msg.pt = lpMsg->pt;
	Msg.wParam = lpMsg->wParam;

        cbtcwW.lpcs = &csW;
        cbtcwW.hwndInsertAfter = cbtcwA->hwndInsertAfter;
        csW = *(CREATESTRUCTW *)cbtcwA->lpcs;

        if (HIWORD(cbtcwA->lpcs->lpszName))
        {
            RtlCreateUnicodeStringFromAsciiz(&usBuffer,cbtcwA->lpcs->lpszName);
            csW.lpszName = usBuffer.Buffer;
        }
        if (HIWORD(cbtcwA->lpcs->lpszClass))
        {
            RtlCreateUnicodeStringFromAsciiz(&usBuffer,cbtcwA->lpcs->lpszClass);
            csW.lpszClass = usBuffer.Buffer;
        }
        Msg.lParam =(LPARAM) &cbtcwW;

        ret = NtUserCallMsgFilter((LPMSG)&Msg, nCode);

        lpMsg->time = Msg.time;
        lpMsg->pt = Msg.pt;

        cbtcwA->hwndInsertAfter = cbtcwW.hwndInsertAfter;
        if (HIWORD(csW.lpszName)) HeapFree( GetProcessHeap(), 0, (LPWSTR)csW.lpszName );
        if (HIWORD(csW.lpszClass)) HeapFree( GetProcessHeap(), 0, (LPWSTR)csW.lpszClass );
     }
  return ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
CallMsgFilterW(
  LPMSG lpMsg,
  int nCode)
{
  return  NtUserCallMsgFilter((LPMSG) lpMsg, nCode);
}


/*
 * @implemented
 */
LRESULT
WINAPI
CallNextHookEx(
  HHOOK Hook,  // Windows NT/XP/2003: Ignored.
  int Code,
  WPARAM wParam,
  LPARAM lParam)
{
  PCLIENTINFO ClientInfo;
  DWORD Flags, Save;
  PHOOK pHook;
  LRESULT lResult = 0;

  GetConnected();

  ClientInfo = GetWin32ClientInfo();

  if (!ClientInfo->phkCurrent) return 0;
  
  pHook = SharedPtrToUser(ClientInfo->phkCurrent);

  if (pHook->HookId == WH_CALLWNDPROC || pHook->HookId == WH_CALLWNDPROCRET)
  {
     Save = ClientInfo->dwHookData;
     Flags = ClientInfo->CI_flags & CI_CURTHPRHOOK;
// wParam: If the message was sent by the current thread/process, it is
// nonzero; otherwise, it is zero.
     if (wParam) ClientInfo->CI_flags |= CI_CURTHPRHOOK;
     else        ClientInfo->CI_flags &= ~CI_CURTHPRHOOK;

     if (pHook->HookId == WH_CALLWNDPROC)
     {
        PCWPSTRUCT pCWP = (PCWPSTRUCT)lParam;

        lResult = NtUserMessageCall( pCWP->hwnd,
                                  pCWP->message,
                                   pCWP->wParam,
                                   pCWP->lParam, 
                                              0,
                               FNID_CALLWNDPROC,
                                    pHook->Ansi);
     }
     else
     {
        PCWPRETSTRUCT pCWPR = (PCWPRETSTRUCT)lParam;

        ClientInfo->dwHookData = pCWPR->lResult;

        lResult = NtUserMessageCall( pCWPR->hwnd,
                                  pCWPR->message,
                                   pCWPR->wParam,
                                   pCWPR->lParam, 
                                               0,
                             FNID_CALLWNDPROCRET,
                                     pHook->Ansi);
     }
     ClientInfo->CI_flags ^= ((ClientInfo->CI_flags ^ Flags) & CI_CURTHPRHOOK);
     ClientInfo->dwHookData = Save;
  }
  else
     lResult = NtUserCallNextHookEx(Code, wParam, lParam, pHook->Ansi);

  return lResult;
}


/*
 * @unimplemented
 */
HHOOK
WINAPI
SetWindowsHookW(int idHook, HOOKPROC lpfn)
{
  return IntSetWindowsHook(idHook, lpfn, NULL, 0, FALSE);
}

/*
 * @unimplemented
 */
HHOOK
WINAPI
SetWindowsHookA(int idHook, HOOKPROC lpfn)
{
  return IntSetWindowsHook(idHook, lpfn, NULL, 0, TRUE);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DeregisterShellHookWindow(HWND hWnd)
{
  return NtUserCallHwnd(hWnd, HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
RegisterShellHookWindow(HWND hWnd)
{
  return NtUserCallHwnd(hWnd, HWND_ROUTINE_REGISTERSHELLHOOKWINDOW);
}

/*
 * @implemented
 */
BOOL
WINAPI
UnhookWindowsHook ( int nCode, HOOKPROC pfnFilterProc )
{
  return NtUserCallTwoParam(nCode, (DWORD)pfnFilterProc, TWOPARAM_ROUTINE_UNHOOKWINDOWSHOOK);
}

/*
 * @implemented
 */
VOID
WINAPI
NotifyWinEvent(
	       DWORD event,
	       HWND  hwnd,
	       LONG  idObject,
	       LONG  idChild
	       )
{
// "Servers call NotifyWinEvent to announce the event to the system after the
// event has occurred; they must never notify the system of an event before
// the event has occurred." msdn on NotifyWinEvent.
  if (g_psi->SrvEventActivity & GetMaskFromEvent(event)) // Check to see.
      NtUserNotifyWinEvent(event, hwnd, idObject, idChild);
}

/*
 * @implemented
 */
HWINEVENTHOOK
WINAPI
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
  WCHAR ModuleName[MAX_PATH];
  UNICODE_STRING USModuleName;

  if ((hmodWinEventProc != NULL) && (dwFlags & WINEVENT_INCONTEXT))
  {
      if (0 == GetModuleFileNameW(hmodWinEventProc, ModuleName, MAX_PATH))
      {
          return NULL;
      }
      RtlInitUnicodeString(&USModuleName, ModuleName);
  }
  else
  {
      RtlInitUnicodeString(&USModuleName, NULL);
  }

  return NtUserSetWinEventHook(eventMin,
                               eventMax,
                       hmodWinEventProc,
                          &USModuleName,
                        pfnWinEventProc,
                              idProcess,
                               idThread,
                                dwFlags);
}

/*
 * @implemented
 */
BOOL
WINAPI
IsWinEventHookInstalled(
    DWORD event)
{
  if ((PW32THREADINFO)NtCurrentTeb()->Win32ThreadInfo)
  {
     return (g_psi->SrvEventActivity & GetMaskFromEvent(event)) != 0;
  }
  return FALSE;
}

/*
 * @unimplemented
 */
HHOOK
WINAPI
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
WINAPI
SetWindowsHookExW(
    int idHook,
    HOOKPROC lpfn,
    HINSTANCE hMod,
    DWORD dwThreadId)
{
  return IntSetWindowsHook(idHook, lpfn, hMod, dwThreadId, FALSE);
}

NTSTATUS WINAPI
User32CallHookProcFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PHOOKPROC_CALLBACK_ARGUMENTS Common;
  LRESULT Result;
  CREATESTRUCTW Csw;
  CBT_CREATEWNDW CbtCreatewndw;
  CREATESTRUCTA Csa;
  CBT_CREATEWNDA CbtCreatewnda;
  PHOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS CbtCreatewndExtra;
  WPARAM wParam;
  LPARAM lParam;
  PKBDLLHOOKSTRUCT KeyboardLlData;
  PMSLLHOOKSTRUCT MouseLlData;
  PMSG Msg;
  PMOUSEHOOKSTRUCT MHook;
  PCWPSTRUCT CWP;
  PCWPRETSTRUCT CWPR;

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

      if (Common->Proc)
         Result = Common->Proc(Common->Code, wParam, lParam);
      else
      {
         ERR("Common = 0x%x, Proc = 0x%x\n",Common,Common->Proc);
      }

      switch(Common->Code)
        {
        case HCBT_CREATEWND:
          CbtCreatewndExtra->WndInsertAfter = CbtCreatewndw.hwndInsertAfter; 
          break;
        }
      break;
    case WH_KEYBOARD_LL:
      KeyboardLlData = (PKBDLLHOOKSTRUCT)((PCHAR) Common + Common->lParam);
      Result = Common->Proc(Common->Code, Common->wParam, (LPARAM) KeyboardLlData);
      break;
    case WH_MOUSE_LL:
      MouseLlData = (PMSLLHOOKSTRUCT)((PCHAR) Common + Common->lParam);
      Result = Common->Proc(Common->Code, Common->wParam, (LPARAM) MouseLlData);
      break;
    case WH_MOUSE:
      MHook = (PMOUSEHOOKSTRUCT)((PCHAR) Common + Common->lParam);
      Result = Common->Proc(Common->Code, Common->wParam, (LPARAM) MHook);
      break;
    case WH_CALLWNDPROC:
      CWP = (PCWPSTRUCT)((PCHAR) Common + Common->lParam);
      Result = Common->Proc(Common->Code, Common->wParam, (LPARAM) CWP);
      break;
    case WH_CALLWNDPROCRET:
      CWPR = (PCWPRETSTRUCT)((PCHAR) Common + Common->lParam);
      Result = Common->Proc(Common->Code, Common->wParam, (LPARAM) CWPR);
      break;
    case WH_MSGFILTER:
    case WH_SYSMSGFILTER:
    case WH_GETMESSAGE:
      Msg = (PMSG)((PCHAR) Common + Common->lParam);
//      FIXME("UHOOK Memory: %x: %x\n",Common, Msg);
      Result = Common->Proc(Common->Code, Common->wParam, (LPARAM) Msg);
      break;
    case WH_FOREGROUNDIDLE:
    case WH_KEYBOARD:
    case WH_SHELL:
      Result = Common->Proc(Common->Code, Common->wParam, Common->lParam);
      break;
    default:
      return ZwCallbackReturn(NULL, 0, STATUS_NOT_SUPPORTED);
    }

  return ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS);
}

NTSTATUS WINAPI
User32CallEventProcFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PEVENTPROC_CALLBACK_ARGUMENTS Common;

  Common = (PEVENTPROC_CALLBACK_ARGUMENTS) Arguments;
  
  Common->Proc(Common->hook,
              Common->event,
               Common->hwnd,
           Common->idObject,
            Common->idChild,
      Common->dwEventThread,
      Common->dwmsEventTime);
  
  return ZwCallbackReturn(NULL, 0, STATUS_SUCCESS);
}



