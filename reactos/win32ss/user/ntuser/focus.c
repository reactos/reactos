/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS Win32k subsystem
 * PURPOSE:          Focus functions
 * FILE:             subsystems/win32/win32k/ntuser/focus.c
 * PROGRAMER:        ReactOS Team
 */

#include <win32k.h>
DBG_DEFAULT_CHANNEL(UserFocus);

PUSER_MESSAGE_QUEUE gpqForeground = NULL;
PUSER_MESSAGE_QUEUE gpqForegroundPrev = NULL;
PTHREADINFO gptiForeground = NULL;
PPROCESSINFO gppiLockSFW = NULL;
ULONG guSFWLockCount = 0; // Rule #8, No menus are active. So should be zero.
PTHREADINFO ptiLastInput = NULL;

/*
  Check locking of a process or one or more menus are active.
*/
BOOL FASTCALL
IsFGLocked(VOID)
{
   return (gppiLockSFW || guSFWLockCount);
}

HWND FASTCALL
IntGetCaptureWindow(VOID)
{
   PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetFocusMessageQueue();
   return ForegroundQueue != NULL ? ForegroundQueue->CaptureWindow : 0;
}

HWND FASTCALL
IntGetThreadFocusWindow(VOID)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   if (!ThreadQueue)
     return NULL;
   return ThreadQueue->spwndFocus ? UserHMGetHandle(ThreadQueue->spwndFocus) : 0;
}

VOID FASTCALL
co_IntSendDeactivateMessages(HWND hWndPrev, HWND hWnd)
{
    PWND WndPrev ;

   if (hWndPrev && (WndPrev = UserGetWindowObject(hWndPrev)))
   {
      co_IntSendMessageNoWait(hWndPrev, WM_NCACTIVATE, FALSE, 0);
      co_IntSendMessageNoWait(hWndPrev, WM_ACTIVATE,
                 MAKEWPARAM(WA_INACTIVE, WndPrev->style & WS_MINIMIZE),
                 (LPARAM)hWnd);
   }
}

BOOL FASTCALL
co_IntMakeWindowActive(PWND Window)
{
  PWND spwndOwner;
  if (Window)
  {  // Set last active for window and it's owner.
     Window->spwndLastActive = Window;
     spwndOwner = Window->spwndOwner;
     while (spwndOwner)
     {
       spwndOwner->spwndLastActive = Window;
       spwndOwner = spwndOwner->spwndOwner;
     }
     return TRUE;
   }
   ERR("MakeWindowActive Failed!\n");
   return FALSE;
}

VOID FASTCALL
co_IntSendActivateMessages(HWND hWndPrev, HWND hWnd, BOOL MouseActivate)
{
   USER_REFERENCE_ENTRY Ref, RefPrev;
   PWND Window, WindowPrev = NULL;
   HANDLE OldTID, NewTID;

   if ((Window = UserGetWindowObject(hWnd)))
   { 
      UserRefObjectCo(Window, &Ref);

      WindowPrev = UserGetWindowObject(hWndPrev);

      if (WindowPrev) UserRefObjectCo(WindowPrev, &RefPrev);

      /* Send palette messages */
      if (gpsi->PUSIFlags & PUSIF_PALETTEDISPLAY &&
          co_IntPostOrSendMessage(hWnd, WM_QUERYNEWPALETTE, 0, 0))
      {
         UserSendNotifyMessage( HWND_BROADCAST,
                                WM_PALETTEISCHANGING,
                               (WPARAM)hWnd,
                                0);
      }

      if (Window->spwndPrev != NULL)
         co_WinPosSetWindowPos(Window, HWND_TOP, 0, 0, 0, 0,
                               SWP_NOSIZE | SWP_NOMOVE);

      if (!Window->spwndOwner && !IntGetParent(Window))
      {
         co_IntShellHookNotify(HSHELL_WINDOWACTIVATED, (LPARAM) hWnd);
      }

      if (Window)
      {
         Window->state |= WNDS_ACTIVEFRAME;

         if (Window->style & WS_MINIMIZE)
         {
            TRACE("Widow was minimized\n");
         }
      }

      if (WindowPrev)
         WindowPrev->state &= ~WNDS_ACTIVEFRAME;

      OldTID = WindowPrev ? IntGetWndThreadId(WindowPrev) : NULL;
      NewTID = Window ? IntGetWndThreadId(Window) : NULL;

      TRACE("SendActiveMessage Old -> %x, New -> %x\n", OldTID, NewTID);

      if (OldTID != NewTID)
      {
         PWND cWindow;
         HWND *List, *phWnd;

         List = IntWinListChildren(UserGetWindowObject(IntGetDesktopWindow()));
         if (List)
         {
            if (OldTID)
            {
               for (phWnd = List; *phWnd; ++phWnd)
               {
                  cWindow = UserGetWindowObject(*phWnd);

                  if (cWindow && (IntGetWndThreadId(cWindow) == OldTID))
                  {  // FALSE if the window is being deactivated,
                     // ThreadId that owns the window being activated.
                    co_IntSendMessageNoWait(*phWnd, WM_ACTIVATEAPP, FALSE, (LPARAM)NewTID);
                  }
               }
            }
            if (NewTID)
            {
               for (phWnd = List; *phWnd; ++phWnd)
               {
                  cWindow = UserGetWindowObject(*phWnd);
                  if (cWindow && (IntGetWndThreadId(cWindow) == NewTID))
                  { // TRUE if the window is being activated,
                    // ThreadId that owns the window being deactivated.
                    co_IntSendMessageNoWait(*phWnd, WM_ACTIVATEAPP, TRUE, (LPARAM)OldTID);
                  }
               }
            }
            ExFreePool(List);
         }
      }
      if (WindowPrev)
         UserDerefObjectCo(WindowPrev); // Now allow the previous window to die.

      UserDerefObjectCo(Window);

      /* FIXME: IntIsWindow */
      co_IntSendMessageNoWait(hWnd, WM_NCACTIVATE, (WPARAM)(hWnd == UserGetForegroundWindow()), 0);
      /* FIXME: WA_CLICKACTIVE */
      co_IntSendMessageNoWait(hWnd, WM_ACTIVATE,
                              MAKEWPARAM(MouseActivate ? WA_CLICKACTIVE : WA_ACTIVE,
                              Window->style & WS_MINIMIZE),
                              (LPARAM)hWndPrev);
   }
}

VOID FASTCALL
co_IntSendKillFocusMessages(HWND hWndPrev, HWND hWnd)
{
   if (hWndPrev)
   {
      IntNotifyWinEvent(EVENT_OBJECT_FOCUS, NULL, OBJID_CLIENT, CHILDID_SELF, 0);
      co_IntPostOrSendMessage(hWndPrev, WM_KILLFOCUS, (WPARAM)hWnd, 0);
   }
}

VOID FASTCALL
co_IntSendSetFocusMessages(HWND hWndPrev, HWND hWnd)
{
   if (hWnd)
   {
      PWND pWnd = UserGetWindowObject(hWnd);
      if (pWnd)
      {
         IntNotifyWinEvent(EVENT_OBJECT_FOCUS, pWnd, OBJID_CLIENT, CHILDID_SELF, 0);
         co_IntPostOrSendMessage(hWnd, WM_SETFOCUS, (WPARAM)hWndPrev, 0);
      }
   }
}

HWND FASTCALL
IntFindChildWindowToOwner(PWND Root, PWND Owner)
{
   HWND Ret;
   PWND Child, OwnerWnd;

   for(Child = Root->spwndChild; Child; Child = Child->spwndNext)
   {
       OwnerWnd = Child->spwndOwner;
      if(!OwnerWnd)
         continue;

      if(OwnerWnd == Owner)
      {
         Ret = Child->head.h;
         return Ret;
      }
   }

   return NULL;
}

/*
   Can the system force foreground from one or more conditions.
 */
BOOL FASTCALL
CanForceFG(PPROCESSINFO ppi)
{
   if (!ptiLastInput ||
        ptiLastInput->ppi == ppi ||
       !gptiForeground ||
        gptiForeground->ppi == ppi ||
        ppi->W32PF_flags & (W32PF_ALLOWFOREGROUNDACTIVATE | W32PF_SETFOREGROUNDALLOWED) ||
        gppiInputProvider == ppi ||
       !gpqForeground
      ) return TRUE;
   return FALSE;
}

/*
   MSDN:
   The system restricts which processes can set the foreground window. A process
   can set the foreground window only if one of the following conditions is true: 

    * The process is the foreground process.
    * The process was started by the foreground process.
    * The process received the last input event.
    * There is no foreground process.
    * The foreground process is being debugged.
    * The foreground is not locked (see LockSetForegroundWindow).
    * The foreground lock time-out has expired (see SPI_GETFOREGROUNDLOCKTIMEOUT in SystemParametersInfo). 
    * No menus are active.
*/

static BOOL FASTCALL
co_IntSetForegroundAndFocusWindow(PWND Wnd, BOOL MouseActivate)
{
   HWND hWnd = UserHMGetHandle(Wnd);
   HWND hWndPrev = NULL;
   PUSER_MESSAGE_QUEUE PrevForegroundQueue;
   PTHREADINFO pti;
   BOOL fgRet = FALSE, Ret = FALSE;

   ASSERT_REFS_CO(Wnd);

   TRACE("SetForegroundAndFocusWindow(%x, %x, %s)\n", hWnd, MouseActivate ? "TRUE" : "FALSE");

   PrevForegroundQueue = IntGetFocusMessageQueue(); // Use this active desktop.
   pti = PsGetCurrentThreadWin32Thread();

   if (PrevForegroundQueue)
   {  // Same Window Q as foreground just do active.
      if (Wnd && Wnd->head.pti->MessageQueue == PrevForegroundQueue)
      {
         if (pti->MessageQueue == PrevForegroundQueue)
         { // Same WQ and TQ go active.
            Ret = co_IntSetActiveWindow(Wnd, NULL, MouseActivate, TRUE);
         }
         else if (Wnd->head.pti->MessageQueue->spwndActive == Wnd)
         { // Same WQ and it is active.
            Ret = TRUE;
         }
         else
         { // Same WQ as FG but not the same TQ send active.
            co_IntSendMessageNoWait(hWnd, WM_ASYNC_SETACTIVEWINDOW, (WPARAM)Wnd, (LPARAM)MouseActivate );
            Ret = TRUE;
         }
         return Ret;
      }

      hWndPrev = PrevForegroundQueue->spwndActive ? UserHMGetHandle(PrevForegroundQueue->spwndActive) : 0;
   }

   if ( (( !IsFGLocked() || pti->ppi == gppiInputProvider ) &&
         ( CanForceFG(pti->ppi) || pti->TIF_flags & (TIF_SYSTEMTHREAD|TIF_CSRSSTHREAD|TIF_ALLOWFOREGROUNDACTIVATE) )) ||
        pti->ppi == ppiScrnSaver
      )
   { 
      IntSetFocusMessageQueue(Wnd->head.pti->MessageQueue);
      gptiForeground = Wnd->head.pti;
      fgRet = TRUE;
   }

//// Fix FG Bounce with regedit but breaks test_SFW todos 
   if (hWndPrev != hWnd )
   {
      if (PrevForegroundQueue &&
          fgRet &&
          Wnd->head.pti->MessageQueue != PrevForegroundQueue &&
          PrevForegroundQueue->spwndActive)
      {
         co_IntSendMessageNoWait(hWndPrev, WM_ASYNC_SETACTIVEWINDOW, 0, 0 );
      }
   }

   if (pti->MessageQueue == Wnd->head.pti->MessageQueue)
   {
       Ret = co_IntSetActiveWindow(Wnd, NULL, MouseActivate, TRUE);
   }
   else if (Wnd->head.pti->MessageQueue->spwndActive == Wnd)
   {
       Ret = TRUE;
   }
   else
   {
       co_IntSendMessageNoWait(hWnd, WM_ASYNC_SETACTIVEWINDOW, (WPARAM)Wnd, (LPARAM)MouseActivate );
       Ret = TRUE;
   }

   return Ret && fgRet;
}

BOOL FASTCALL
co_IntMouseActivateWindow(PWND Wnd)
{
   HWND Top;
   PWND TopWindow;
   USER_REFERENCE_ENTRY Ref;

   ASSERT_REFS_CO(Wnd);

   if (Wnd->style & WS_DISABLED)
   {
      BOOL Ret;
      PWND TopWnd;
      PWND DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());
      if (DesktopWindow)
      {
         Top = IntFindChildWindowToOwner(DesktopWindow, Wnd);
         if ((TopWnd = UserGetWindowObject(Top)))
         {
            UserRefObjectCo(TopWnd, &Ref);
            Ret = co_IntMouseActivateWindow(TopWnd);
            UserDerefObjectCo(TopWnd);

            return Ret;
         }
      }
      return FALSE;
   }

   TopWindow = UserGetAncestor(Wnd, GA_ROOT);
   if (!TopWindow) return FALSE;

   /* TMN: Check return valud from this function? */
   UserRefObjectCo(TopWindow, &Ref);

   co_IntSetForegroundAndFocusWindow(TopWindow, TRUE);

   UserDerefObjectCo(TopWindow);

   return TRUE;
}

BOOL FASTCALL
co_IntSetActiveWindow(PWND Wnd OPTIONAL, HWND * Prev, BOOL bMouse, BOOL bFocus)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   HWND hWndPrev;
   HWND hWnd = 0;
   CBTACTIVATESTRUCT cbt;

   if (Wnd)
   {
      ASSERT_REFS_CO(Wnd);
      hWnd = UserHMGetHandle(Wnd);
   }

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   ASSERT(ThreadQueue != 0);

   hWndPrev = ThreadQueue->spwndActive ? UserHMGetHandle(ThreadQueue->spwndActive) : NULL;
   if (Prev) *Prev = hWndPrev;
   if (hWndPrev == hWnd) return TRUE;

   if (Wnd)
   {
      if (ThreadQueue != Wnd->head.pti->MessageQueue)
      {
         PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetFocusMessageQueue();
         // Rule 1 & 4, We are foreground so set this FG window or NULL foreground....
         if (!ForegroundQueue || ForegroundQueue == ThreadQueue)
         {
            return co_IntSetForegroundAndFocusWindow(Wnd, bMouse);
         }
      }

      if (Wnd->state & WNDS_BEINGACTIVATED) return TRUE;
   }

   /* Call CBT hook chain */
   cbt.fMouse     = bMouse;
   cbt.hWndActive = hWndPrev;
   if (co_HOOK_CallHooks( WH_CBT, HCBT_ACTIVATE, (WPARAM)hWnd, (LPARAM)&cbt))
   {
      ERR("SetActiveWindow WH_CBT Call Hook return!\n");
      return FALSE;
   }

   co_IntSendDeactivateMessages(hWndPrev, hWnd);

   if (Wnd) Wnd->state |= WNDS_BEINGACTIVATED;

   IntNotifyWinEvent(EVENT_SYSTEM_FOREGROUND, Wnd, OBJID_WINDOW, CHILDID_SELF, WEF_SETBYWNDPTI);

   /* check if the specified window can be set in the input data of a given queue */
   if ( !Wnd || ThreadQueue == Wnd->head.pti->MessageQueue)
   {
      /* set the current thread active window */
      if (!Wnd || co_IntMakeWindowActive(Wnd))
      {
         ThreadQueue->spwndActivePrev = ThreadQueue->spwndActive;
         ThreadQueue->spwndActive = Wnd;
      }
   }

   co_IntSendActivateMessages(hWndPrev, hWnd, bMouse);

   /* now change focus if necessary */
   if (bFocus)
   {
      /* Do not change focus if the window is no longer active */
      if (ThreadQueue->spwndActive == Wnd)
      {
         if (!ThreadQueue->spwndFocus ||
             !Wnd ||
              UserGetAncestor(ThreadQueue->spwndFocus, GA_ROOT) != Wnd)
         {
            co_UserSetFocus(Wnd);
         }
      }
   }

   if (Wnd) Wnd->state &= ~WNDS_BEINGACTIVATED;
   return TRUE;
}

HWND FASTCALL
co_UserSetFocus(PWND Window)
{
   HWND hWndPrev = 0;
   PWND pwndTop;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   if (Window)
      ASSERT_REFS_CO(Window);

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   ASSERT(ThreadQueue != 0);

   hWndPrev = ThreadQueue->spwndFocus ? UserHMGetHandle(ThreadQueue->spwndFocus) : 0;

   if (Window != 0)
   {
      if (hWndPrev == UserHMGetHandle(Window))
      {
         return hWndPrev; /* Nothing to do */
      }

      if (Window->head.pti->MessageQueue != ThreadQueue)
      {
         ERR("SetFocus Must have the same Q!\n");
         return 0;
      }

      /* Check if we can set the focus to this window */
      pwndTop = Window;
      for (;;)
      {
         if (pwndTop->style & (WS_MINIMIZED|WS_DISABLED)) return 0;
         if (!pwndTop->spwndParent || pwndTop->spwndParent == UserGetDesktopWindow())
         {
            if ((pwndTop->style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return 0;
            break;         
         }
         if (pwndTop->spwndParent == UserGetMessageWindow()) return 0;
         pwndTop = pwndTop->spwndParent;
      }

      if (co_HOOK_CallHooks( WH_CBT, HCBT_SETFOCUS, (WPARAM)Window->head.h, (LPARAM)hWndPrev))
      {
         ERR("SetFocus 1 WH_CBT Call Hook return!\n");
         return 0;
      }

      /* Activate pwndTop if needed. */
      if (pwndTop != ThreadQueue->spwndActive)
      {
         PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetFocusMessageQueue(); // Keep it based on desktop.
         if (ThreadQueue != ForegroundQueue) // HACK see rule 2 & 3.
         {
            if (!co_IntSetForegroundAndFocusWindow(pwndTop, FALSE))
            {
               ERR("SetFocus Set Foreground and Focus Failed!\n");
               return 0;
            }
         }

         /* Set Active when it is needed. */
         if (pwndTop != ThreadQueue->spwndActive)
         {
            if (!co_IntSetActiveWindow(pwndTop, NULL, FALSE, FALSE))
            {
               ERR("SetFocus Set Active Failed!\n");
               return 0;
            }
         }

         /* Abort if window destroyed */
         if (Window->state2 & WNDS2_INDESTROY) return 0;
         /* Do not change focus if the window is no longer active */
         if (pwndTop != ThreadQueue->spwndActive)
         {
            ERR("SetFocus Top window did not go active!\n");
            return 0;
         }
      }

      /* check if the specified window can be set in the input data of a given queue */
      if ( !Window || ThreadQueue == Window->head.pti->MessageQueue)
         /* set the current thread focus window */
         ThreadQueue->spwndFocus = Window;

      TRACE("Focus: %d -> %d\n", hWndPrev, Window->head.h);

      co_IntSendKillFocusMessages(hWndPrev, Window->head.h);
      co_IntSendSetFocusMessages(hWndPrev, Window->head.h);
   }
   else /* NULL hwnd passed in */
   {
      if (!hWndPrev) return 0; /* nothing to do */
      if (co_HOOK_CallHooks( WH_CBT, HCBT_SETFOCUS, (WPARAM)0, (LPARAM)hWndPrev))
      {
         ERR("SetFocusWindow 2 WH_CBT Call Hook return!\n");
         return 0;
      }

      /* set the current thread focus window null */
      ThreadQueue->spwndFocus = 0;

      co_IntSendKillFocusMessages(hWndPrev, 0);
   }
   return hWndPrev ? (IntIsWindow(hWndPrev) ? hWndPrev : 0) : 0;
}

HWND FASTCALL
UserGetForegroundWindow(VOID)
{
   PUSER_MESSAGE_QUEUE ForegroundQueue;

   ForegroundQueue = IntGetFocusMessageQueue();
   return( ForegroundQueue ? (ForegroundQueue->spwndActive ? UserHMGetHandle(ForegroundQueue->spwndActive) : 0) : 0);
}

HWND FASTCALL UserGetActiveWindow(VOID)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   return( ThreadQueue ? (ThreadQueue->spwndActive ? UserHMGetHandle(ThreadQueue->spwndActive) : 0) : 0);
}

HWND APIENTRY
IntGetCapture(VOID)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   DECLARE_RETURN(HWND);

   TRACE("Enter IntGetCapture\n");

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   RETURN( ThreadQueue ? ThreadQueue->CaptureWindow : 0);

CLEANUP:
   TRACE("Leave IntGetCapture, ret=%i\n",_ret_);
   END_CLEANUP;
}

HWND FASTCALL
co_UserSetCapture(HWND hWnd)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   PWND Window, pWnd;
   HWND hWndPrev;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if (ThreadQueue->QF_flags & QF_CAPTURELOCKED)
      return NULL;

   if ((Window = UserGetWindowObject(hWnd)))
   {
      if (Window->head.pti->MessageQueue != ThreadQueue)
      {
         return NULL;
      }
   }
   
   hWndPrev = MsqSetStateWindow(ThreadQueue, MSQ_STATE_CAPTURE, hWnd);

   if (hWndPrev)
   {
      pWnd = UserGetWindowObject(hWndPrev);
      if (pWnd)
         IntNotifyWinEvent(EVENT_SYSTEM_CAPTUREEND, pWnd, OBJID_WINDOW, CHILDID_SELF, WEF_SETBYWNDPTI);
   }

   if (Window)
      IntNotifyWinEvent(EVENT_SYSTEM_CAPTURESTART, Window, OBJID_WINDOW, CHILDID_SELF, WEF_SETBYWNDPTI);

   if (hWndPrev && hWndPrev != hWnd)
   {
      if (ThreadQueue->MenuOwner && Window) ThreadQueue->QF_flags |= QF_CAPTURELOCKED;

      co_IntPostOrSendMessage(hWndPrev, WM_CAPTURECHANGED, 0, (LPARAM)hWnd);

      ThreadQueue->QF_flags &= ~QF_CAPTURELOCKED;
   }

   ThreadQueue->CaptureWindow = hWnd;

   if (hWnd == NULL) // Release mode.
   {
      MOUSEINPUT mi;
   /// These are HACKS!
      /* Also remove other windows if not capturing anymore */
      MsqSetStateWindow(ThreadQueue, MSQ_STATE_MENUOWNER, NULL);
      MsqSetStateWindow(ThreadQueue, MSQ_STATE_MOVESIZE, NULL);
   ///
      /* Somebody may have missed some mouse movements */
      mi.dx = 0;
      mi.dy = 0;
      mi.mouseData = 0;
      mi.dwFlags = MOUSEEVENTF_MOVE;
      mi.time = 0;
      mi.dwExtraInfo = 0;
      UserSendMouseInput(&mi, FALSE);
   }
   return hWndPrev;
}

/*
  API Call
*/
BOOL
FASTCALL
IntReleaseCapture(VOID)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   // Can not release inside WM_CAPTURECHANGED!!
   if (ThreadQueue->QF_flags & QF_CAPTURELOCKED) return FALSE;

   co_UserSetCapture(NULL);

   return TRUE;
}

/*
  API Call
*/
BOOL FASTCALL
co_IntSetForegroundWindow(PWND Window)
{
   ASSERT_REFS_CO(Window);

   return co_IntSetForegroundAndFocusWindow(Window, FALSE);
}

/*
  API Call
*/
BOOL FASTCALL
IntLockSetForegroundWindow(UINT uLockCode)
{
   ULONG Err = ERROR_ACCESS_DENIED;
   PPROCESSINFO ppi = PsGetCurrentProcessWin32Process();
   switch (uLockCode)
   {
      case LSFW_LOCK:
         if ( CanForceFG(ppi) && !gppiLockSFW )
         {
            gppiLockSFW = ppi;
            return TRUE;
         }
         break;
      case LSFW_UNLOCK:
         if ( gppiLockSFW == ppi)
         {
            gppiLockSFW = NULL;
            return TRUE;
         }
         break;
      default:
         Err = ERROR_INVALID_PARAMETER;
   }
   EngSetLastError(Err);
   return FALSE;
}

/*
  API Call
*/
BOOL FASTCALL
IntAllowSetForegroundWindow(DWORD dwProcessId)
{
   PPROCESSINFO ppi, ppiCur;
   PEPROCESS Process = NULL;

   ppi = NULL;
   if (dwProcessId != ASFW_ANY)
   {
      if (!NT_SUCCESS(PsLookupProcessByProcessId((HANDLE)dwProcessId, &Process)))
      {
         EngSetLastError(ERROR_INVALID_PARAMETER);
         return FALSE;
      }
      ppi = PsGetProcessWin32Process(Process);
      if (!ppi)
      {
         ObDereferenceObject(Process);
         return FALSE;
      }
   }
   ppiCur = PsGetCurrentProcessWin32Process();
   if (!CanForceFG(ppiCur))
   {
      if (Process) ObDereferenceObject(Process);
      EngSetLastError(ERROR_ACCESS_DENIED);
      return FALSE;
   }
   if (dwProcessId == ASFW_ANY)
   {  // All processes will be enabled to set the foreground window.
      ptiLastInput = NULL;
   }
   else
   {  // Rule #3, last input event in force.
      ptiLastInput = ppi->ptiList;
      ObDereferenceObject(Process);
   }
   return TRUE;
}

/*
 * @implemented
 */
HWND APIENTRY
NtUserGetForegroundWindow(VOID)
{
   DECLARE_RETURN(HWND);

   TRACE("Enter NtUserGetForegroundWindow\n");
   UserEnterExclusive();

   RETURN( UserGetForegroundWindow());

CLEANUP:
   TRACE("Leave NtUserGetForegroundWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

HWND APIENTRY
NtUserSetActiveWindow(HWND hWnd)
{
   USER_REFERENCE_ENTRY Ref;
   HWND hWndPrev;
   PWND Window;
   DECLARE_RETURN(HWND);

   TRACE("Enter NtUserSetActiveWindow(%x)\n", hWnd);
   UserEnterExclusive();

   Window = NULL;
   if (hWnd)
   {
      if (!(Window = UserGetWindowObject(hWnd)))
      {
         RETURN( 0);
      }
   }

   if (!Window ||
        Window->head.pti->MessageQueue == gptiCurrent->MessageQueue)
   {
      if (Window) UserRefObjectCo(Window, &Ref);
      if (!co_IntSetActiveWindow(Window, &hWndPrev, FALSE, TRUE)) hWndPrev = NULL;
      if (Window) UserDerefObjectCo(Window);
   }
   else
      hWndPrev = NULL;

   RETURN( hWndPrev ? (IntIsWindow(hWndPrev) ? hWndPrev : 0) : 0 );

CLEANUP:
   TRACE("Leave NtUserSetActiveWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
HWND APIENTRY
NtUserSetCapture(HWND hWnd)
{
   DECLARE_RETURN(HWND);

   TRACE("Enter NtUserSetCapture(%x)\n", hWnd);
   UserEnterExclusive();

   RETURN( co_UserSetCapture(hWnd));

CLEANUP:
   TRACE("Leave NtUserSetCapture, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
HWND APIENTRY
NtUserSetFocus(HWND hWnd)
{
   PWND Window;
   USER_REFERENCE_ENTRY Ref;
   DECLARE_RETURN(HWND);
   HWND ret;

   TRACE("Enter NtUserSetFocus(%x)\n", hWnd);
   UserEnterExclusive();

   if (hWnd)
   {
      if (!(Window = UserGetWindowObject(hWnd)))
      {
         RETURN(NULL);
      }

      UserRefObjectCo(Window, &Ref);
      ret = co_UserSetFocus(Window);
      UserDerefObjectCo(Window);

      RETURN(ret);
   }
   else
   {
      RETURN( co_UserSetFocus(0));
   }

CLEANUP:
   TRACE("Leave NtUserSetFocus, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* EOF */
