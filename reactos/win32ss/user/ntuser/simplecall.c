/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          NtUserCallXxx call stubs
 * FILE:             subsystem/win32/win32k/ntuser/simplecall.c
 * PROGRAMER:        Ge van Geldorp (ge@gse.nl)
 */

#include <win32k.h>

#include <winlogon.h>

DBG_DEFAULT_CHANNEL(UserMisc);

/* Registered logon process ID */
HANDLE gpidLogon = 0;

BOOL FASTCALL
co_IntRegisterLogonProcess(HANDLE ProcessId, BOOL Register)
{
   NTSTATUS Status;
   PEPROCESS Process;

   Status = PsLookupProcessByProcessId(ProcessId, &Process);
   if (!NT_SUCCESS(Status))
   {
      EngSetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
   }

   ProcessId = Process->UniqueProcessId;

   ObDereferenceObject(Process);

   if (Register)
   {
      /* Register the logon process */
      if (gpidLogon != 0)
         return FALSE;

      gpidLogon = ProcessId;
   }
   else
   {
      /* Deregister the logon process */
      if (gpidLogon != ProcessId)
         return FALSE;

      gpidLogon = 0;
   }

   return TRUE;
}

/*
 * @unimplemented
 */
DWORD_PTR
APIENTRY
NtUserCallNoParam(DWORD Routine)
{
   DWORD_PTR Result = 0;
   DECLARE_RETURN(DWORD_PTR);

   TRACE("Enter NtUserCallNoParam\n");
   UserEnterExclusive();

   switch(Routine)
   {
      case NOPARAM_ROUTINE_CREATEMENU:
         Result = (DWORD_PTR)UserCreateMenu(GetW32ThreadInfo()->rpdesk, FALSE);
         break;

      case NOPARAM_ROUTINE_CREATEMENUPOPUP:
         Result = (DWORD_PTR)UserCreateMenu(GetW32ThreadInfo()->rpdesk, TRUE);
         break;

      case NOPARAM_ROUTINE_DESTROY_CARET:
         Result = (DWORD_PTR)co_IntDestroyCaret(PsGetCurrentThread()->Tcb.Win32Thread);
         break;

      case NOPARAM_ROUTINE_INIT_MESSAGE_PUMP:
         Result = (DWORD_PTR)IntInitMessagePumpHook();
         break;

      case NOPARAM_ROUTINE_UNINIT_MESSAGE_PUMP:
         Result = (DWORD_PTR)IntUninitMessagePumpHook();
         break;

      case NOPARAM_ROUTINE_GETMESSAGEEXTRAINFO:
         Result = (DWORD_PTR)MsqGetMessageExtraInfo();
         break;

      case NOPARAM_ROUTINE_MSQCLEARWAKEMASK:
         RETURN( (DWORD_PTR)IntMsqClearWakeMask());

      case NOPARAM_ROUTINE_GETMSESSAGEPOS:
      {
         PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
         RETURN( (DWORD_PTR)MAKELONG(pti->ptLast.x, pti->ptLast.y));
      }

      case NOPARAM_ROUTINE_RELEASECAPTURE:
         RETURN( (DWORD_PTR)IntReleaseCapture());

      case NOPARAM_ROUTINE_LOADUSERAPIHOOK:
         RETURN(UserLoadApiHook());

      case NOPARAM_ROUTINE_ZAPACTIVEANDFOUS:
      {
         PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
         TRACE("Zapping the Active and Focus window out of the Queue!\n");
         pti->MessageQueue->spwndFocus = NULL;
         pti->MessageQueue->spwndActive = NULL;
         RETURN(0);
      }

      /* this is a Reactos only case and is needed for gui-on-demand */
      case NOPARAM_ROUTINE_ISCONSOLEMODE:
          RETURN( ScreenDeviceContext == NULL );

      default:
         ERR("Calling invalid routine number 0x%x in NtUserCallNoParam\n", Routine);
         EngSetLastError(ERROR_INVALID_PARAMETER);
         break;
   }
   RETURN(Result);

CLEANUP:
   TRACE("Leave NtUserCallNoParam, ret=%p\n",(PVOID)_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
DWORD_PTR
APIENTRY
NtUserCallOneParam(
   DWORD_PTR Param,
   DWORD Routine)
{
   DECLARE_RETURN(DWORD_PTR);

   TRACE("Enter NtUserCallOneParam\n");

   UserEnterExclusive();

   switch(Routine)
   {
      case ONEPARAM_ROUTINE_POSTQUITMESSAGE:
          {
                PTHREADINFO pti;
                pti = PsGetCurrentThreadWin32Thread();
                MsqPostQuitMessage(pti, Param);
                RETURN(TRUE);
          }

      case ONEPARAM_ROUTINE_BEGINDEFERWNDPOS:
         {
             PSMWP psmwp;
             HDWP hDwp = NULL;
             INT count = (INT)Param;

             if (count < 0)
             {
                EngSetLastError(ERROR_INVALID_PARAMETER);
                RETURN(0);
             }
             /* Windows allows zero count, in which case it allocates context for 8 moves */
             if (count == 0) count = 8;

             psmwp = (PSMWP) UserCreateObject( gHandleTable,
                                               NULL, 
                                               NULL, 
                                              (PHANDLE)&hDwp,
                                               TYPE_SETWINDOWPOS,
                                               sizeof(SMWP));
             if (!psmwp) RETURN(0);
             psmwp->acvr = ExAllocatePoolWithTag(PagedPool, count * sizeof(CVR), USERTAG_SWP);
             if (!psmwp->acvr)
             {
                UserDeleteObject(hDwp, TYPE_SETWINDOWPOS);
                RETURN(0);
             }
             RtlZeroMemory(psmwp->acvr, count * sizeof(CVR));
             psmwp->bHandle = TRUE;
             psmwp->ccvr = 0;          // actualCount
             psmwp->ccvrAlloc = count; // suggestedCount
             RETURN((DWORD_PTR)hDwp);
         }

      case ONEPARAM_ROUTINE_SHOWCURSOR:
         RETURN( (DWORD_PTR)UserShowCursor((BOOL)Param) );

      case ONEPARAM_ROUTINE_GETDESKTOPMAPPING:
         {
             PTHREADINFO ti;
             ti = GetW32ThreadInfo();
             if (ti != NULL)
             {
                /* Try convert the pointer to a user mode pointer if the desktop is
                   mapped into the process */
                RETURN((DWORD_PTR)DesktopHeapAddressToUser((PVOID)Param));
             }
             else
             {
                RETURN(0);
             }
         }

      case ONEPARAM_ROUTINE_WINDOWFROMDC:
         RETURN( (DWORD_PTR)IntWindowFromDC((HDC)Param));

      case ONEPARAM_ROUTINE_SWAPMOUSEBUTTON:
         {
            DWORD_PTR Result;

            Result = gspv.bMouseBtnSwap;
            gspv.bMouseBtnSwap = Param ? TRUE : FALSE;
            gpsi->aiSysMet[SM_SWAPBUTTON] = gspv.bMouseBtnSwap;
            RETURN(Result);
         }

      case ONEPARAM_ROUTINE_SWITCHCARETSHOWING:
         RETURN( (DWORD_PTR)IntSwitchCaretShowing((PVOID)Param));

      case ONEPARAM_ROUTINE_SETCARETBLINKTIME:
         RETURN( (DWORD_PTR)IntSetCaretBlinkTime((UINT)Param));

      case ONEPARAM_ROUTINE_SETMESSAGEEXTRAINFO:
         RETURN( (DWORD_PTR)MsqSetMessageExtraInfo((LPARAM)Param));

      case ONEPARAM_ROUTINE_CREATEEMPTYCUROBJECT:
         {
			DWORD_PTR Result ;

            if (!(Result = (DWORD_PTR)IntCreateCurIconHandle((DWORD)Param)))
            {
               EngSetLastError(ERROR_NOT_ENOUGH_MEMORY);
               RETURN(0);
            }
			RETURN(Result);
         }

      case ONEPARAM_ROUTINE_ENABLEPROCWNDGHSTING:
         {
            BOOL Enable;
            PPROCESSINFO Process = PsGetCurrentProcessWin32Process();

            if(Process != NULL)
            {
               Enable = (BOOL)(Param != 0);

               if(Enable)
               {
                  Process->W32PF_flags &= ~W32PF_NOWINDOWGHOSTING;
               }
               else
               {
                  Process->W32PF_flags |= W32PF_NOWINDOWGHOSTING;
               }

               RETURN( TRUE);
            }

            RETURN( FALSE);
         }

      case ONEPARAM_ROUTINE_GETINPUTEVENT:
         RETURN( (DWORD_PTR)IntMsqSetWakeMask(Param));

      case ONEPARAM_ROUTINE_GETKEYBOARDTYPE:
         RETURN( UserGetKeyboardType(Param));

      case ONEPARAM_ROUTINE_GETKEYBOARDLAYOUT:
         RETURN( (DWORD_PTR)UserGetKeyboardLayout(Param));

      case ONEPARAM_ROUTINE_RELEASEDC:
         RETURN (UserReleaseDC(NULL, (HDC) Param, FALSE));

      case ONEPARAM_ROUTINE_REALIZEPALETTE:
         RETURN (UserRealizePalette((HDC) Param));

      case ONEPARAM_ROUTINE_GETQUEUESTATUS:
      {
         RETURN (IntGetQueueStatus((DWORD)Param));
      }
      case ONEPARAM_ROUTINE_ENUMCLIPBOARDFORMATS:
         /* FIXME: Should use UserEnterShared */
         RETURN(UserEnumClipboardFormats(Param));

      case ONEPARAM_ROUTINE_GETCURSORPOS:
      {
          BOOL Ret = TRUE;
          PPOINTL pptl;
          PTHREADINFO pti = PsGetCurrentThreadWin32Thread();
          if (pti->rpdesk != IntGetActiveDesktop()) RETURN(FALSE);
          _SEH2_TRY
          {
              ProbeForWrite((POINT*)Param,sizeof(POINT),1);
             pptl = (PPOINTL)Param;
             *pptl = gpsi->ptCursor;
          }
          _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
          {
             Ret = FALSE;
          }
          _SEH2_END;
          RETURN(Ret);
      }
      case ONEPARAM_ROUTINE_SETPROCDEFLAYOUT:
      {
          PPROCESSINFO ppi;
          if (Param & LAYOUT_ORIENTATIONMASK)
          {
             ppi = PsGetCurrentProcessWin32Process();
             ppi->dwLayout = Param;
             RETURN(TRUE);
          }
          EngSetLastError(ERROR_INVALID_PARAMETER);
          RETURN(FALSE);
      }
      case ONEPARAM_ROUTINE_GETPROCDEFLAYOUT:
      {
          BOOL Ret = TRUE;
          PPROCESSINFO ppi;
          PDWORD pdwLayout;
          if ( PsGetCurrentProcess() == gpepCSRSS)
          {
             EngSetLastError(ERROR_INVALID_ACCESS);
             RETURN(FALSE);
          }
          ppi = PsGetCurrentProcessWin32Process();
          _SEH2_TRY
          {
             pdwLayout = (PDWORD)Param;
             *pdwLayout = ppi->dwLayout;
          }
          _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
          {
             SetLastNtError(_SEH2_GetExceptionCode());
             Ret = FALSE;
          }
          _SEH2_END;
          RETURN(Ret);
      }
      case ONEPARAM_ROUTINE_REPLYMESSAGE:
          RETURN (co_MsqReplyMessage((LRESULT) Param));
      case ONEPARAM_ROUTINE_MESSAGEBEEP:
          RETURN ( UserPostMessage(hwndSAS, WM_LOGONNOTIFY, LN_MESSAGE_BEEP, Param) );
		  /* TODO: Implement sound sentry */
      case ONEPARAM_ROUTINE_CREATESYSTEMTHREADS:
          RETURN(CreateSystemThreads(Param));
      case ONEPARAM_ROUTINE_LOCKFOREGNDWINDOW:
          RETURN( (DWORD_PTR)IntLockSetForegroundWindow(Param));
      case ONEPARAM_ROUTINE_ALLOWSETFOREGND:
          RETURN( (DWORD_PTR)IntAllowSetForegroundWindow(Param));
   }
   ERR("Calling invalid routine number 0x%x in NtUserCallOneParam(), Param=0x%x\n",
           Routine, Param);
   EngSetLastError(ERROR_INVALID_PARAMETER);
   RETURN( 0);

CLEANUP:
   TRACE("Leave NtUserCallOneParam, ret=%p\n", (PVOID)_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @implemented
 */
DWORD_PTR
APIENTRY
NtUserCallTwoParam(
   DWORD_PTR Param1,
   DWORD_PTR Param2,
   DWORD Routine)
{
   PWND Window;
   DECLARE_RETURN(DWORD_PTR);

   TRACE("Enter NtUserCallTwoParam\n");
   UserEnterExclusive();

   switch(Routine)
   {
      case TWOPARAM_ROUTINE_SETMENUBARHEIGHT:
         {
            DWORD_PTR Ret;
            PMENU MenuObject = IntGetMenuObject((HMENU)Param1);
            if(!MenuObject)
               RETURN( 0);

            if(Param2 > 0)
            {
               Ret = (MenuObject->cyMenu == (int)Param2);
               MenuObject->cyMenu = (int)Param2;
            }
            else
               Ret = (DWORD_PTR)MenuObject->cyMenu;
            IntReleaseMenuObject(MenuObject);
            RETURN( Ret);
         }

      case TWOPARAM_ROUTINE_SETGUITHRDHANDLE:
         {
            PTHREADINFO pti = (PTHREADINFO)PsGetCurrentThreadWin32Thread();
            ASSERT(pti->MessageQueue);
            RETURN( (DWORD_PTR)MsqSetStateWindow(pti, (ULONG)Param1, (HWND)Param2));
         }

      case TWOPARAM_ROUTINE_ENABLEWINDOW:
         RETURN( IntEnableWindow((HWND)Param1, (BOOL)Param2));

      case TWOPARAM_ROUTINE_SHOWOWNEDPOPUPS:
      {
         Window = UserGetWindowObject((HWND)Param1);
         if (!Window) RETURN(0);

         RETURN( (DWORD_PTR)IntShowOwnedPopups(Window, (BOOL) Param2));
      }

      case TWOPARAM_ROUTINE_ROS_UPDATEUISTATE:
      {
          WPARAM wParam;
          Window = UserGetWindowObject((HWND)Param1);
          if (!Window) RETURN(0);

          /* Unpack wParam */
          wParam = MAKEWPARAM((Param2 >> 3) & 0x3,
                              Param2 & (UISF_HIDEFOCUS | UISF_HIDEACCEL | UISF_ACTIVE));

          RETURN( UserUpdateUiState(Window, wParam) );
      }

      case TWOPARAM_ROUTINE_SWITCHTOTHISWINDOW:
         STUB
         RETURN( 0);

      case TWOPARAM_ROUTINE_SETCARETPOS:
         RETURN( (DWORD_PTR)co_IntSetCaretPos((int)Param1, (int)Param2));

      case TWOPARAM_ROUTINE_REGISTERLOGONPROCESS:
         RETURN( (DWORD_PTR)co_IntRegisterLogonProcess((HANDLE)Param1, (BOOL)Param2));

      case TWOPARAM_ROUTINE_SETCURSORPOS:
         RETURN( (DWORD_PTR)UserSetCursorPos((int)Param1, (int)Param2, 0, 0, FALSE));

      case TWOPARAM_ROUTINE_UNHOOKWINDOWSHOOK:
         RETURN( IntUnhookWindowsHook((int)Param1, (HOOKPROC)Param2));
      case TWOPARAM_ROUTINE_EXITREACTOS:
          if(hwndSAS == NULL)
          {
              ASSERT(hwndSAS);
              RETURN(STATUS_NOT_FOUND);
          }
         RETURN( co_IntSendMessage (hwndSAS, PM_WINLOGON_EXITWINDOWS, (WPARAM) Param1, (LPARAM)Param2));
   }
   ERR("Calling invalid routine number 0x%x in NtUserCallTwoParam(), Param1=0x%x Parm2=0x%x\n",
           Routine, Param1, Param2);
   EngSetLastError(ERROR_INVALID_PARAMETER);
   RETURN( 0);

CLEANUP:
   TRACE("Leave NtUserCallTwoParam, ret=%p\n", (PVOID)_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * @unimplemented
 */
BOOL
APIENTRY
NtUserCallHwndLock(
   HWND hWnd,
   DWORD Routine)
{
   BOOL Ret = 0;
   PWND Window;
   USER_REFERENCE_ENTRY Ref;
   DECLARE_RETURN(BOOLEAN);

   TRACE("Enter NtUserCallHwndLock\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }
   UserRefObjectCo(Window, &Ref);

   /* FIXME: Routine can be 0x53 - 0x5E */
   switch (Routine)
   {
      case HWNDLOCK_ROUTINE_ARRANGEICONICWINDOWS:
         co_WinPosArrangeIconicWindows(Window);
         break;

      case HWNDLOCK_ROUTINE_DRAWMENUBAR:
         {
            TRACE("HWNDLOCK_ROUTINE_DRAWMENUBAR\n");
            Ret = TRUE;
            if ((Window->style & (WS_CHILD | WS_POPUP)) != WS_CHILD)
               co_WinPosSetWindowPos( Window,
                                      HWND_DESKTOP,
                                      0,0,0,0,
                                      SWP_NOSIZE|
                                      SWP_NOMOVE|
                                      SWP_NOZORDER|
                                      SWP_NOACTIVATE|
                                      SWP_FRAMECHANGED );
            break;
         }

      case HWNDLOCK_ROUTINE_REDRAWFRAME:
         co_WinPosSetWindowPos( Window,
                                HWND_DESKTOP,
                                0,0,0,0,
                                SWP_NOSIZE|
                                SWP_NOMOVE|
                                SWP_NOZORDER|
                                SWP_NOACTIVATE|
                                SWP_FRAMECHANGED );
         Ret = TRUE;
         break;

      case HWNDLOCK_ROUTINE_REDRAWFRAMEANDHOOK:
         co_WinPosSetWindowPos( Window,
                                HWND_DESKTOP,
                                0,0,0,0,
                                SWP_NOSIZE|
                                SWP_NOMOVE|
                                SWP_NOZORDER|
                                SWP_NOACTIVATE|
                                SWP_FRAMECHANGED );
         if (!Window->spwndOwner && !IntGetParent(Window))
         {
            co_IntShellHookNotify(HSHELL_REDRAW, (WPARAM) hWnd, FALSE); // FIXME Flashing?
         }
         Ret = TRUE;
         break;

      case HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOW:
         TRACE("co_IntSetForegroundWindow 1 0x%p\n",hWnd);
         Ret = co_IntSetForegroundWindow(Window);
         TRACE("co_IntSetForegroundWindow 2 0x%p\n",hWnd);
         break;

      case HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOWMOUSE:
         TRACE("co_IntSetForegroundWindow M 1 0x%p\n",hWnd);
         Ret = co_IntSetForegroundWindowMouse(Window);
         TRACE("co_IntSetForegroundWindow M 2 0x%p\n",hWnd);
         break;

      case HWNDLOCK_ROUTINE_UPDATEWINDOW:
         Ret = co_UserRedrawWindow( Window, NULL, 0, RDW_UPDATENOW | RDW_ALLCHILDREN);
         break;
   }

   UserDerefObjectCo(Window);

   RETURN( Ret);

CLEANUP:
   TRACE("Leave NtUserCallHwndLock, ret=%u\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @unimplemented
 */
HWND
APIENTRY
NtUserCallHwndOpt(
   HWND hWnd,
   DWORD Routine)
{
   switch (Routine)
   {
      case HWNDOPT_ROUTINE_SETPROGMANWINDOW:
         GetW32ThreadInfo()->pDeskInfo->hProgmanWindow = hWnd;
         break;

      case HWNDOPT_ROUTINE_SETTASKMANWINDOW:
         GetW32ThreadInfo()->pDeskInfo->hTaskManWindow = hWnd;
         break;
   }

   return hWnd;
}

DWORD
APIENTRY
NtUserCallHwnd(
   HWND hWnd,
   DWORD Routine)
{
   switch (Routine)
   {
      case HWND_ROUTINE_GETWNDCONTEXTHLPID:
      {
         PWND Window;
         PPROPERTY HelpId;
         USER_REFERENCE_ENTRY Ref;

         UserEnterExclusive();

         if (!(Window = UserGetWindowObject(hWnd)))
         {
            UserLeave();
            return 0;
         }
         UserRefObjectCo(Window, &Ref);

         HelpId = IntGetProp(Window, gpsi->atomContextHelpIdProp);

         UserDerefObjectCo(Window);
         UserLeave();
         return (DWORD)HelpId->Data;
      }
      case HWND_ROUTINE_REGISTERSHELLHOOKWINDOW:
         if (IntIsWindow(hWnd))
            return IntRegisterShellHookWindow(hWnd);
         return FALSE;
         break;
      case HWND_ROUTINE_DEREGISTERSHELLHOOKWINDOW:
         if (IntIsWindow(hWnd))
            return IntDeRegisterShellHookWindow(hWnd);
         return FALSE;
      case HWND_ROUTINE_SETMSGBOX:
      {
         PWND Window;
         UserEnterExclusive();
         if ((Window = UserGetWindowObject(hWnd)))
         {
            Window->state |= WNDS_MSGBOX;
         }
         UserLeave();
         return FALSE;
      }
   }
   STUB;

   return 0;
}

DWORD
APIENTRY
NtUserCallHwndParam(
   HWND hWnd,
   DWORD Param,
   DWORD Routine)
{

   switch (Routine)
   {
      case HWNDPARAM_ROUTINE_KILLSYSTEMTIMER:
          return IntKillTimer(UserGetWindowObject(hWnd), (UINT_PTR)Param, TRUE);

      case HWNDPARAM_ROUTINE_SETWNDCONTEXTHLPID:
      {
         PWND Window;

         UserEnterExclusive();
         if(!(Window = UserGetWindowObject(hWnd)))
         {
            UserLeave();
            return FALSE;
         }

         if ( Param )
            IntSetProp(Window, gpsi->atomContextHelpIdProp, (HANDLE)Param);
         else
            IntRemoveProp(Window, gpsi->atomContextHelpIdProp);

         UserLeave();
         return TRUE;
      }

      case HWNDPARAM_ROUTINE_SETDIALOGPOINTER:
      {
         PWND pWnd;
         USER_REFERENCE_ENTRY Ref;

         UserEnterExclusive();

         if (!(pWnd = UserGetWindowObject(hWnd)))
         {
            UserLeave();
            return 0;
         }
         UserRefObjectCo(pWnd, &Ref);

         if (pWnd->head.pti->ppi == PsGetCurrentProcessWin32Process() &&
             pWnd->cbwndExtra == DLGWINDOWEXTRA &&
             !(pWnd->state & WNDS_SERVERSIDEWINDOWPROC))
         {
            if (Param)
            {
               if (!pWnd->fnid) pWnd->fnid = FNID_DIALOG;
               pWnd->state |= WNDS_DIALOGWINDOW;
            }
            else
            {
               pWnd->fnid |= FNID_DESTROY;
               pWnd->state &= ~WNDS_DIALOGWINDOW;
            }
         }

         UserDerefObjectCo(pWnd);
         UserLeave();
         return 0;
      }

      case HWNDPARAM_ROUTINE_ROS_NOTIFYWINEVENT:
      {
         PWND pWnd;
         PNOTIFYEVENT pne;
         UserEnterExclusive();
         pne = (PNOTIFYEVENT)Param;
         if (hWnd)
            pWnd = UserGetWindowObject(hWnd);
         else
            pWnd = NULL;
         IntNotifyWinEvent(pne->event, pWnd, pne->idObject, pne->idChild, pne->flags);
         UserLeave();
         return 0;
      }
      case HWNDPARAM_ROUTINE_CLEARWINDOWSTATE:
      {
         PWND pWnd;
         UserEnterExclusive();
         pWnd = UserGetWindowObject(hWnd);
         if (pWnd) IntClearWindowState(pWnd, (UINT)Param);
         UserLeave();
         return 0;
      }
      case HWNDPARAM_ROUTINE_SETWINDOWSTATE:
      {
         PWND pWnd;
         UserEnterExclusive();
         pWnd = UserGetWindowObject(hWnd);
         if (pWnd) IntSetWindowState(pWnd, (UINT)Param);
         UserLeave();
         return 0;
      }
   }

   STUB;

   return 0;
}

DWORD
APIENTRY
NtUserCallHwndParamLock(
   HWND hWnd,
   DWORD Param,
   DWORD Routine)
{
   DWORD Ret = 0;
   PWND Window;
   USER_REFERENCE_ENTRY Ref;
   DECLARE_RETURN(DWORD);

   TRACE("Enter NtUserCallHwndParamLock\n");
   UserEnterExclusive();

   if (!(Window = UserGetWindowObject(hWnd)))
   {
      RETURN( FALSE);
   }
   UserRefObjectCo(Window, &Ref);

   switch (Routine)
   {
      case TWOPARAM_ROUTINE_VALIDATERGN:
      {
          PREGION Rgn = RGNOBJAPI_Lock((HRGN)Param, NULL);
          if (Rgn)
          {
              Ret = (DWORD)co_UserRedrawWindow( Window, NULL, Rgn, RDW_VALIDATE);
              RGNOBJAPI_Unlock(Rgn);
          }
          break;
      }
   }

   UserDerefObjectCo(Window);

   RETURN( Ret);

CLEANUP:
   TRACE("Leave NtUserCallHwndParamLock, ret=%lu\n",_ret_);
   UserLeave();
   END_CLEANUP;

}

/* EOF */
