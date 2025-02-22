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
 * FILE:            win32ss/user/user32/windows/hook.c
 * PURPOSE:         Hooks
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

#include <user32.h>

WINE_DEFAULT_DEBUG_CHANNEL(user32);

typedef struct _NOTIFYEVENT
{
   DWORD event;
   LONG  idObject;
   LONG  idChild;
   DWORD flags;
} NOTIFYEVENT, *PNOTIFYEVENT;

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

/*
   Since ReactOS uses User32 as the main message source this was needed.
   Base on the funny rules from the wine tests it left it with this option.
   8^(
 */
VOID
FASTCALL
IntNotifyWinEvent(
                 DWORD event,
                 HWND  hwnd,
                 LONG  idObject,
                 LONG  idChild,
                 DWORD flags
                 )
{
  NOTIFYEVENT ne;
  ne.event    = event;
  ne.idObject = idObject;
  ne.idChild  = idChild;
  ne.flags    = flags;
  if (gpsi->dwInstalledEventHooks & GetMaskFromEvent(event))
  NtUserxNotifyWinEvent(hwnd, &ne);
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
  MSG Msg;
  if ( NtCurrentTeb()->Win32ThreadInfo &&
      (ISITHOOKED(WH_MSGFILTER) || ISITHOOKED(WH_SYSMSGFILTER)) )
  {
     if ( lpMsg->message & ~WM_MAXIMUM )
     {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
     }
     RtlCopyMemory(&Msg, lpMsg, sizeof(MSG));
     return NtUserCallMsgFilter( &Msg, nCode);
  }
  return FALSE;
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
  MSG Msg;
  if ( NtCurrentTeb()->Win32ThreadInfo &&
      (ISITHOOKED(WH_MSGFILTER) || ISITHOOKED(WH_SYSMSGFILTER)) )
  {
     if ( lpMsg->message & ~WM_MAXIMUM )
     {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
     }
     RtlCopyMemory(&Msg, lpMsg, sizeof(MSG));
     return  NtUserCallMsgFilter( &Msg, nCode);
  }
  return FALSE;
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
  PHOOK pHook, phkNext;
  LRESULT lResult = 0;

  ClientInfo = GetWin32ClientInfo();

  if (!ClientInfo->phkCurrent) return 0;

  pHook = DesktopPtrToUser(ClientInfo->phkCurrent);

  if (!pHook->phkNext) return 0; // Nothing to do....

  phkNext = DesktopPtrToUser(pHook->phkNext);

  if ( phkNext->HookId == WH_CALLWNDPROC ||
       phkNext->HookId == WH_CALLWNDPROCRET)
  {
     Save = ClientInfo->dwHookData;
     Flags = ClientInfo->CI_flags & CI_CURTHPRHOOK;
// wParam: If the message was sent by the current thread/process, it is
// nonzero; otherwise, it is zero.
     if (wParam) ClientInfo->CI_flags |= CI_CURTHPRHOOK;
     else        ClientInfo->CI_flags &= ~CI_CURTHPRHOOK;

     if (phkNext->HookId == WH_CALLWNDPROC)
     {
        PCWPSTRUCT pCWP = (PCWPSTRUCT)lParam;

        NtUserMessageCall( pCWP->hwnd,
                           pCWP->message,
                           pCWP->wParam,
                           pCWP->lParam,
                          (ULONG_PTR)&lResult,
                           FNID_CALLWNDPROC,
                           phkNext->Ansi);
     }
     else
     {
        PCWPRETSTRUCT pCWPR = (PCWPRETSTRUCT)lParam;

        ClientInfo->dwHookData = pCWPR->lResult;

        NtUserMessageCall( pCWPR->hwnd,
                           pCWPR->message,
                           pCWPR->wParam,
                           pCWPR->lParam,
                          (ULONG_PTR)&lResult,
                           FNID_CALLWNDPROCRET,
                           phkNext->Ansi);
     }
     ClientInfo->CI_flags ^= ((ClientInfo->CI_flags ^ Flags) & CI_CURTHPRHOOK);
     ClientInfo->dwHookData = Save;
  }
  else
     lResult = NtUserCallNextHookEx(Code, wParam, lParam, pHook->Ansi);

  return lResult;
}


/*
 * @implemented
 */
HHOOK
WINAPI
SetWindowsHookW(int idHook, HOOKPROC lpfn)
{
  DWORD ThreadId = PtrToUint(NtCurrentTeb()->ClientId.UniqueThread);
  return IntSetWindowsHook(idHook, lpfn, NULL, ThreadId, FALSE);
//  return NtUserSetWindowsHookAW(idHook, lpfn, FALSE);
}

/*
 * @implemented
 */
HHOOK
WINAPI
SetWindowsHookA(int idHook, HOOKPROC lpfn)
{
  DWORD ThreadId = PtrToUint(NtCurrentTeb()->ClientId.UniqueThread);
  return IntSetWindowsHook(idHook, lpfn, NULL, ThreadId, TRUE);
//  return NtUserSetWindowsHookAW(idHook, lpfn, TRUE);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
DeregisterShellHookWindow(HWND hWnd)
{
  return NtUserxDeregisterShellHookWindow(hWnd);
}

/*
 * @unimplemented
 */
BOOL
WINAPI
RegisterShellHookWindow(HWND hWnd)
{
  return NtUserxRegisterShellHookWindow(hWnd);
}

/*
 * @implemented
 */
BOOL
WINAPI
UnhookWindowsHook ( int nCode, HOOKPROC pfnFilterProc )
{
  return NtUserxUnhookWindowsHook(nCode, pfnFilterProc);
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
  if (gpsi->dwInstalledEventHooks & GetMaskFromEvent(event)) // Check to see.
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
  PUNICODE_STRING pusmodName;

  RtlInitUnicodeString(&USModuleName, NULL);

  if ((hmodWinEventProc != NULL) && (dwFlags & WINEVENT_INCONTEXT))
  {
      if (0 == GetModuleFileNameW(hmodWinEventProc, ModuleName, MAX_PATH))
      {
          return NULL;
      }
      RtlInitUnicodeString(&USModuleName, ModuleName);
      pusmodName = &USModuleName;
  }
  else
  {
      pusmodName = NULL;
  }

  return NtUserSetWinEventHook(eventMin,
                               eventMax,
                       hmodWinEventProc,
                             pusmodName,
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
  if ((PTHREADINFO)NtCurrentTeb()->Win32ThreadInfo)
  {
     return (gpsi->dwInstalledEventHooks & GetMaskFromEvent(event)) != 0;
  }
  return FALSE;
}

/*
 * @implemented
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
 * @implemented
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

static
BOOL
ClientLoadLibrary(
    PUNICODE_STRING pstrLibName,
    PUNICODE_STRING pstrInitFunc,
    BOOL bUnload,
    BOOL bApiHook)
{
    HINSTANCE hLibrary;
    PVOID pInitFunction;
    NTSTATUS Status;
    ANSI_STRING InitFuncName;
    BOOL bResult = FALSE;

    TRACE("ClientLoadLibrary: pid: %d, strLibraryName: %S, "
          "strInitFuncName: %S, bUnload: %d, bApiHook:%d\n",
          GetCurrentProcessId(),
          pstrLibName->Buffer,
          pstrInitFunc->Buffer,
          bUnload,
          bApiHook);

    /* Check if we have to load the module */
    if (bUnload == FALSE)
    {
        ASSERT(pstrLibName->Buffer != NULL);

        /* Load it */
        hLibrary = LoadLibrary(pstrLibName->Buffer);
        if (hLibrary == 0)
        {
            return FALSE;
        }

        if (!bApiHook)
        {
            /* There is nothing more to do for a global hook*/
            return TRUE;
        }

        /* Initialize the user api hook */
        ASSERT(pstrInitFunc->Buffer);
        Status = RtlUnicodeStringToAnsiString(&InitFuncName,
                                              pstrInitFunc,
                                              TRUE);
        if (!NT_SUCCESS(Status))
        {
            FreeLibrary(hLibrary);
            return FALSE;
        }

        /* Get the address of the initialization routine */
        pInitFunction = GetProcAddress(hLibrary, InitFuncName.Buffer);
        if (pInitFunction)
        {
            /* Call the initialization routine */
            bResult = InitUserApiHook(hLibrary, (USERAPIHOOKPROC)pInitFunction);
        }

        RtlFreeAnsiString(&InitFuncName);

        /* In case of error unload the library */
        if (bResult == FALSE)
        {
            FreeLibrary(hLibrary);
        }
    }
    else
    {
        /* Cleanup user api hook before unloading */
        if (bApiHook)
        {
            hLibrary = ghmodUserApiHook;
            bResult = ClearUserApiHook(ghmodUserApiHook);

            /* Check if we can we unload it now */
            if (!bResult)
            {
                /* Return success because we are going to free
                   the library in EndUserApiHook*/
                return TRUE;
            }
        }
        else
        {
            /* Get the library handle from the name */
            hLibrary = GetModuleHandle(pstrLibName->Buffer);
            if (hLibrary == NULL)
            {
                return FALSE;
            }
        }

        bResult = FreeLibrary(hLibrary);
    }

    return bResult;
}

NTSTATUS WINAPI
User32CallClientLoadLibraryFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
    BOOL bResult;
    PCLIENT_LOAD_LIBRARY_ARGUMENTS Argument;

    /* Retireve the callback parameters */
    Argument = (PCLIENT_LOAD_LIBRARY_ARGUMENTS)Arguments;
    if(Argument->strLibraryName.Buffer != NULL)
    {
        Argument->strLibraryName.Buffer = (PWCHAR)((ULONG_PTR)Argument->strLibraryName.Buffer + (ULONG_PTR)Argument);
    }
    if(Argument->strInitFuncName.Buffer != NULL)
    {
        Argument->strInitFuncName.Buffer = (PWCHAR)((ULONG_PTR)Argument->strInitFuncName.Buffer + (ULONG_PTR)Argument);
    }

    /* Call the implementation of the callback */
    bResult = ClientLoadLibrary(&Argument->strLibraryName,
                                &Argument->strInitFuncName,
                                Argument->Unload,
                                Argument->ApiHook);

    return ZwCallbackReturn(&bResult, sizeof(HINSTANCE), STATUS_SUCCESS);
}

NTSTATUS WINAPI
User32CallHookProcFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PHOOKPROC_CALLBACK_ARGUMENTS Common;
  CREATESTRUCTW *pCsw = NULL;
  CBT_CREATEWNDW *pCbtCreatewndw = NULL;
  PHOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS CbtCreatewndExtra = NULL;
  KBDLLHOOKSTRUCT KeyboardLlData, *pKeyboardLlData;
  MSLLHOOKSTRUCT MouseLlData, *pMouseLlData;
  MSG *pcMsg, *pMsg;
  PMOUSEHOOKSTRUCT pMHook;
  CWPSTRUCT *pCWP;
  CWPRETSTRUCT *pCWPR;
  PRECTL prl;
  LPCBTACTIVATESTRUCT pcbtas;
  HOOKPROC Proc;
  WPARAM wParam = 0;
  LPARAM lParam = 0;
  LRESULT Result = 0;
  BOOL Hit = FALSE, Loaded = FALSE;
  HMODULE mod = NULL;
  NTSTATUS Status = STATUS_SUCCESS;

  Common = (PHOOKPROC_CALLBACK_ARGUMENTS) Arguments;

  Proc = Common->Proc;
  // HookProc Justin Case module is from another process.
  if (Common->offPfn && Common->Mod)
  {
     if (!(mod = GetModuleHandleW((LPCWSTR)Common->ModuleName)))
     {
        TRACE("Reloading Hook Module.\n");
        if (!(mod = LoadLibraryExW((LPCWSTR)Common->ModuleName, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)))
        {
           ERR("Failed to load Hook Module.\n");
        }
        else
        {
           Loaded = TRUE; // Free it only when loaded.
        }
     }
     if (mod)
     {
        TRACE("Loading Hook Module. %S\n",Common->ModuleName);
        Proc = (HOOKPROC)((char *)mod + Common->offPfn);
     }
  }

  switch(Common->HookId)
  {
    case WH_CBT:
    {
      //ERR("WH_CBT: Code %d\n", Common->Code);
      switch(Common->Code)
      {
        case HCBT_CREATEWND:
          CbtCreatewndExtra = (PHOOKPROC_CBT_CREATEWND_EXTRA_ARGUMENTS)
                              ((PCHAR) Common + Common->lParam);

          pCbtCreatewndw = (CBT_CREATEWNDW*)HeapAlloc(GetProcessHeap(), 0, sizeof(CBT_CREATEWNDW));
          RtlCopyMemory(pCbtCreatewndw, CbtCreatewndExtra, sizeof(CBT_CREATEWNDW));

          pCsw = (CREATESTRUCTW*)HeapAlloc(GetProcessHeap(), 0, sizeof(CREATESTRUCTW));
          RtlCopyMemory(pCsw, &CbtCreatewndExtra->Cs, sizeof(CREATESTRUCTW));

          pCbtCreatewndw->lpcs = pCsw;
          pCbtCreatewndw->hwndInsertAfter = CbtCreatewndExtra->WndInsertAfter;
          wParam = Common->wParam;
          lParam = (LPARAM) pCbtCreatewndw;
          //ERR("HCBT_CREATEWND: hWnd %p Csw %p Name %p Class %p\n", Common->wParam, pCsw, pCsw->lpszName, pCsw->lpszClass);
          break;
        case HCBT_CLICKSKIPPED:
            pMHook = (PMOUSEHOOKSTRUCT)((PCHAR) Common + Common->lParam);
            lParam = (LPARAM) pMHook;
            wParam = Common->wParam;
            break;
        case HCBT_MOVESIZE:
            prl = (PRECTL)((PCHAR) Common + Common->lParam);
            lParam = (LPARAM) prl;
            wParam = Common->wParam;
            break;
        case HCBT_ACTIVATE:
            //ERR("HCBT_ACTIVATE: hwnd %p\n",Common->wParam);
            pcbtas = (LPCBTACTIVATESTRUCT)((PCHAR) Common + Common->lParam);
            lParam = (LPARAM) pcbtas;
            wParam = Common->wParam;
            break;
        case HCBT_KEYSKIPPED: /* The rest SEH support */
        case HCBT_MINMAX:
        case HCBT_SETFOCUS:
        case HCBT_SYSCOMMAND:
        case HCBT_DESTROYWND:
        case HCBT_QS:
            wParam = Common->wParam;
            lParam = Common->lParam;
            break;
        default:
          if (Loaded) FreeLibrary(mod);
          ERR("HCBT_ not supported = %d\n", Common->Code);
          return ZwCallbackReturn(NULL, 0, STATUS_NOT_SUPPORTED);
      }

      if (Proc)
      {
         _SEH2_TRY
         {
            Result = Proc(Common->Code, wParam, lParam);
         }
         _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
         {
            Hit = TRUE;
         }
         _SEH2_END;
      }
      else
      {
         ERR("Null Proc! Common = 0x%x, Proc = 0x%x\n",Common,Proc);
      }
      switch(Common->Code)
      {
        case HCBT_CREATEWND:
          CbtCreatewndExtra->WndInsertAfter = pCbtCreatewndw->hwndInsertAfter;
          CbtCreatewndExtra->Cs.x  = pCbtCreatewndw->lpcs->x;
          CbtCreatewndExtra->Cs.y  = pCbtCreatewndw->lpcs->y;
          CbtCreatewndExtra->Cs.cx = pCbtCreatewndw->lpcs->cx;
          CbtCreatewndExtra->Cs.cy = pCbtCreatewndw->lpcs->cy;
          HeapFree(GetProcessHeap(), 0, pCsw);
          HeapFree(GetProcessHeap(), 0, pCbtCreatewndw);
          break;
      }
      break;
    }
    case WH_KEYBOARD_LL:
      //ERR("WH_KEYBOARD_LL: Code %d, wParam %d\n",Common->Code,Common->wParam);
      pKeyboardLlData = (PKBDLLHOOKSTRUCT)((PCHAR) Common + Common->lParam);
      RtlCopyMemory(&KeyboardLlData, pKeyboardLlData, sizeof(KBDLLHOOKSTRUCT));
      Result = Proc(Common->Code, Common->wParam, (LPARAM) &KeyboardLlData);
      break;
    case WH_MOUSE_LL:
      //ERR("WH_MOUSE_LL: Code %d, wParam %d\n",Common->Code,Common->wParam);
      pMouseLlData = (PMSLLHOOKSTRUCT)((PCHAR) Common + Common->lParam);
      RtlCopyMemory(&MouseLlData, pMouseLlData, sizeof(MSLLHOOKSTRUCT));
      Result = Proc(Common->Code, Common->wParam, (LPARAM) &MouseLlData);
      break;
    case WH_MOUSE: /* SEH support */
      pMHook = (PMOUSEHOOKSTRUCT)((PCHAR) Common + Common->lParam);
      _SEH2_TRY
      {
         Result = Proc(Common->Code, Common->wParam, (LPARAM) pMHook);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Hit = TRUE;
      }
      _SEH2_END;
      break;
    case WH_CALLWNDPROC:
    {
      PCWP_Struct pcwps = (PCWP_Struct)Common;
      CWPSTRUCT *pCWPT = &pcwps->cwps;
      pCWP = HeapAlloc(GetProcessHeap(), 0, Common->lParamSize + sizeof(CWPSTRUCT));
      RtlCopyMemory(pCWP, pCWPT, sizeof(CWPSTRUCT));
      //ERR("WH_CALLWNDPROC: Code %d, wParam %d hwnd %p msg %d\n",Common->Code,Common->wParam,pCWP->hwnd,pCWP->message);
      /* If more memory is reserved, then lParam is a pointer.
       * Size of the buffer is stocked in the lParam member, and its content
       * is at the end of the argument buffer */
      if ( Common->lParamSize )
      {
         pCWP->lParam = (LPARAM)((PCHAR)pCWP + sizeof(CWPSTRUCT));
         RtlCopyMemory( (PCHAR)pCWP + sizeof(CWPSTRUCT), &pcwps->Extra, Common->lParamSize );
      }
      Result = Proc(Common->Code, Common->wParam, (LPARAM) pCWP);
      HeapFree(GetProcessHeap(), 0, pCWP);
    }
      break;
    case WH_CALLWNDPROCRET:
      /* Almost the same as WH_CALLWNDPROC */
    {
      PCWPR_Struct pcwprs = (PCWPR_Struct)Common;
      CWPRETSTRUCT *pCWPRT = &pcwprs->cwprs;
      pCWPR = HeapAlloc(GetProcessHeap(), 0, Common->lParamSize + sizeof(CWPRETSTRUCT));
      RtlCopyMemory(pCWPR, pCWPRT, sizeof(CWPRETSTRUCT));
      //ERR("WH_CALLWNDPROCRET: Code %d, wParam %d hwnd %p msg %d\n",Common->Code,Common->wParam,pCWPRT->hwnd,pCWPRT->message);
      if ( Common->lParamSize )
      {
         pCWPR->lParam = (LPARAM)((PCHAR)pCWPR + sizeof(CWPRETSTRUCT));
         RtlCopyMemory( (PCHAR)pCWPR + sizeof(CWPRETSTRUCT), &pcwprs->Extra, Common->lParamSize );
      }
      Result = Proc(Common->Code, Common->wParam, (LPARAM) pCWPR);
      HeapFree(GetProcessHeap(), 0, pCWPR);
    }
      break;
    case WH_MSGFILTER: /* All SEH support */
    case WH_SYSMSGFILTER:
    case WH_GETMESSAGE:
      pMsg = (PMSG)((PCHAR) Common + Common->lParam);
      pcMsg = HeapAlloc( GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(MSG));
      RtlCopyMemory(pcMsg, pMsg, sizeof(MSG));
//      ERR("pMsg %d  pcMsg %d\n",pMsg->message, pcMsg->message);
      _SEH2_TRY
      {
         Result = Proc(Common->Code, Common->wParam, (LPARAM) pcMsg);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Hit = TRUE;
      }
      _SEH2_END;
      if (!Hit && Common->HookId == WH_GETMESSAGE)
         RtlCopyMemory(pMsg, pcMsg, sizeof(MSG));
      HeapFree( GetProcessHeap(), 0, pcMsg );
      break;
    case WH_KEYBOARD:
    case WH_SHELL:
      Result = Proc(Common->Code, Common->wParam, Common->lParam);
      break;
    case WH_FOREGROUNDIDLE: /* <-- SEH support */
      _SEH2_TRY
      {
         Result = Proc(Common->Code, Common->wParam, Common->lParam);
      }
      _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
      {
         Hit = TRUE;
      }
      _SEH2_END;
      break;
    default:
      if (Loaded) FreeLibrary(mod);
      ERR("WH_ not supported = %d\n", Common->HookId);
      return ZwCallbackReturn(NULL, 0, STATUS_NOT_SUPPORTED);
  }
  if (Hit)
  {
     ERR("Hook Exception! Id: %d, Code %d, Proc 0x%x\n",Common->HookId,Common->Code,Proc);
     Status = STATUS_UNSUCCESSFUL;
  }
  if (Loaded) FreeLibrary(mod);
  Common->Result = Result;
  return ZwCallbackReturn(Arguments, ArgumentLength, Status);
}

NTSTATUS WINAPI
User32CallEventProcFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PEVENTPROC_CALLBACK_ARGUMENTS Common;
  WINEVENTPROC Proc;
  WCHAR module[MAX_PATH];
  DWORD len;
  HMODULE mod = NULL;
  BOOL Loaded = FALSE;

  Common = (PEVENTPROC_CALLBACK_ARGUMENTS) Arguments;

  Proc = Common->Proc;

  if (Common->offPfn && Common->Mod)
  {  // Validate the module again.
     if (!(len = GetModuleFileNameW((HINSTANCE)Common->Mod, module, MAX_PATH)) || len >= MAX_PATH)
     {
        ERR("Error check for module!\n");
        Common->Mod = 0;
     }

     if (Common->Mod && !(mod = GetModuleHandleW(module)))
     {
        TRACE("Reloading Event Module.\n");
        if (!(mod = LoadLibraryExW(module, NULL, LOAD_WITH_ALTERED_SEARCH_PATH)))
        {
           ERR("Failed to load Event Module.\n");
        }
        else
        {
           Loaded = TRUE; // Free it only when loaded.
        }
     }

     if (mod)
     {
        TRACE("Loading Event Module. %S\n",module);
        Proc = (WINEVENTPROC)((char *)mod + Common->offPfn);
     }
  }

  Proc(Common->hook,
       Common->event,
       Common->hwnd,
       Common->idObject,
       Common->idChild,
       Common->dwEventThread,
       Common->dwmsEventTime);

  if (Loaded) FreeLibrary(mod);

  return ZwCallbackReturn(NULL, 0, STATUS_SUCCESS);
}



