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

/*
  Get capture window via foreground Queue.
*/
HWND FASTCALL
IntGetCaptureWindow(VOID)
{
   PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetFocusMessageQueue();
   return ( ForegroundQueue ? (ForegroundQueue->spwndCapture ? UserHMGetHandle(ForegroundQueue->spwndCapture) : 0) : 0);
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

BOOL FASTCALL
co_IntSendDeactivateMessages(HWND hWndPrev, HWND hWnd)
{
   USER_REFERENCE_ENTRY RefPrev;
   PWND WndPrev;
   BOOL Ret = TRUE;

   if (hWndPrev && (WndPrev = ValidateHwndNoErr(hWndPrev)))
   {
      UserRefObjectCo(WndPrev, &RefPrev);

      if (co_IntSendMessageNoWait(hWndPrev, WM_NCACTIVATE, FALSE, 0)) //(LPARAM)hWnd))
      {
         co_IntSendMessageNoWait(hWndPrev, WM_ACTIVATE,
                    MAKEWPARAM(WA_INACTIVE, WndPrev->style & WS_MINIMIZE),
                    (LPARAM)hWnd);

         if (WndPrev)
            WndPrev->state &= ~WNDS_ACTIVEFRAME;
      }
      else
      {
         ERR("Application is keeping itself Active to prevent the change!\n");
         Ret = FALSE;
      }

      UserDerefObjectCo(WndPrev);
   }
   return Ret;
}

BOOL FASTCALL
co_IntMakeWindowActive(PWND Window)
{
  PWND spwndOwner;
  if (VerifyWnd(Window))
  {  // Set last active for window and it's owner.
     spwndOwner = Window;
     while (spwndOwner->spwndOwner)
     {
       spwndOwner = spwndOwner->spwndOwner;
     }
     spwndOwner->spwndLastActive = Window;
     return TRUE;
   }
   ERR("MakeWindowActive Failed!\n");
   return FALSE;
}

BOOL FASTCALL
co_IntSendActivateMessages(PWND WindowPrev, PWND Window, BOOL MouseActivate, BOOL Async)
{
   USER_REFERENCE_ENTRY Ref, RefPrev;
   HANDLE OldTID, NewTID;
   PTHREADINFO pti, ptiOld, ptiNew;
   BOOL InAAPM = FALSE;

   //ERR("SendActivateMessages\n");

   pti = PsGetCurrentThreadWin32Thread();

   if (Window)
   {
      UserRefObjectCo(Window, &Ref);

      if (WindowPrev) UserRefObjectCo(WindowPrev, &RefPrev);

      /* Send palette messages */
      if (gpsi->PUSIFlags & PUSIF_PALETTEDISPLAY &&
          //co_IntPostOrSendMessage(UserHMGetHandle(Window), WM_QUERYNEWPALETTE, 0, 0))
          co_IntSendMessage(UserHMGetHandle(Window), WM_QUERYNEWPALETTE, 0, 0))
      {
         UserSendNotifyMessage( HWND_BROADCAST,
                                WM_PALETTEISCHANGING,
                               (WPARAM)UserHMGetHandle(Window),
                                0);
      }
      //// Fixes CORE-6434.
      if (!(Window->style & WS_CHILD))
      {
         PWND pwndTemp = co_GetDesktopWindow(Window)->spwndChild;

         while (pwndTemp && !(pwndTemp->style & WS_VISIBLE)) pwndTemp = pwndTemp->spwndNext;

         if (Window != pwndTemp || (WindowPrev && !IntIsWindowVisible(WindowPrev)))
         {
            if (!Async || pti->MessageQueue == gpqForeground)
            {
               UINT flags = SWP_NOSIZE | SWP_NOMOVE;
               if (Window == pwndTemp) flags |= SWP_NOACTIVATE;
               //ERR("co_IntSendActivateMessages SetWindowPos! Async %d pti Q == FGQ %d\n",Async,pti->MessageQueue == gpqForeground);
               co_WinPosSetWindowPos(Window, HWND_TOP, 0, 0, 0, 0, flags);
            }
         }
      }
      ////
      //// CORE-1161 and CORE-6651
      if (Window->spwndPrev)
      {
         HWND *phwndTopLevel, *phwndCurrent;
         PWND pwndCurrent, pwndDesktop;

         pwndDesktop = co_GetDesktopWindow(Window);//UserGetDesktopWindow();
         if (Window->spwndParent == pwndDesktop )
         {
            phwndTopLevel = IntWinListChildren(pwndDesktop);
            phwndCurrent = phwndTopLevel;
            while(*phwndCurrent)
            {
                pwndCurrent = UserGetWindowObject(*phwndCurrent);

                if (pwndCurrent && pwndCurrent->spwndOwner == Window )
                {
                    co_WinPosSetWindowPos(pwndCurrent, HWND_TOP, 0, 0, 0, 0, SWP_NOSIZE|SWP_NOMOVE|SWP_NOACTIVATE);
                }
                phwndCurrent++;
            }
            ExFreePoolWithTag(phwndTopLevel, USERTAG_WINDOWLIST);
          }
      }
      ////
   }

   OldTID = WindowPrev ? IntGetWndThreadId(WindowPrev) : NULL;
   NewTID = Window ? IntGetWndThreadId(Window) : NULL;
   ptiOld = WindowPrev ? WindowPrev->head.pti : NULL;
   ptiNew = Window ? Window->head.pti : NULL;

   //ERR("SendActivateMessage Old -> %x, New -> %x\n", OldTID, NewTID);

   if (!(pti->TIF_flags & TIF_INACTIVATEAPPMSG) &&
        (OldTID != NewTID) )
   {
      PWND cWindow;
      HWND *List, *phWnd;

      List = IntWinListChildren(UserGetDesktopWindow());
      if ( List )
      {
         if ( OldTID )
         {
            ptiOld->TIF_flags |= TIF_INACTIVATEAPPMSG;
            // Note: Do not set pci flags, this does crash!
            for (phWnd = List; *phWnd; ++phWnd)
            {
               cWindow = ValidateHwndNoErr(*phWnd);
               if (cWindow && cWindow->head.pti == ptiOld)
               { // FALSE if the window is being deactivated,
                 // ThreadId that owns the window being activated.
                 //ERR("SendActivateMessage Old\n");
                 co_IntSendMessageNoWait(*phWnd, WM_ACTIVATEAPP, FALSE, (LPARAM)NewTID);
               }
            }
            ptiOld->TIF_flags &= ~TIF_INACTIVATEAPPMSG;
         }
         if ( NewTID )
         {  //// Prevents a resource crash due to reentrance!
            InAAPM = TRUE;
            pti->TIF_flags |= TIF_INACTIVATEAPPMSG;
            ////
            for (phWnd = List; *phWnd; ++phWnd)
            {
               cWindow = ValidateHwndNoErr(*phWnd);
               if (cWindow && cWindow->head.pti == ptiNew)
               { // TRUE if the window is being activated,
                 // ThreadId that owns the window being deactivated.
                 //ERR("SendActivateMessage New\n");
                 co_IntSendMessageNoWait(*phWnd, WM_ACTIVATEAPP, TRUE, (LPARAM)OldTID);
               }
            }
         }
         ExFreePoolWithTag(List, USERTAG_WINDOWLIST);
      }
   }

   if (Window)
   {
      if (WindowPrev)
         UserDerefObjectCo(WindowPrev); // Now allow the previous window to die.

      if (Window->state & WNDS_ACTIVEFRAME)
      {  // If already active frame do not allow NCPaint.
         //ERR("SendActivateMessage Is Active Frame!\n");
         Window->state |= WNDS_NONCPAINT;
      }

      if (Window->style & WS_MINIMIZE)
      {
         TRACE("Widow was minimized\n");
      }

      co_IntMakeWindowActive(Window);

      co_IntSendMessageNoWait( UserHMGetHandle(Window),
                               WM_NCACTIVATE,
                              (WPARAM)(Window == (gpqForeground ? gpqForeground->spwndActive : NULL)),
                               0); //(LPARAM)hWndPrev);

      co_IntSendMessage( UserHMGetHandle(Window),
                         WM_ACTIVATE,
                         MAKEWPARAM(MouseActivate ? WA_CLICKACTIVE : WA_ACTIVE, Window->style & WS_MINIMIZE),
                        (LPARAM)(WindowPrev ? UserHMGetHandle(WindowPrev) : 0));

      if (Window->spwndParent == UserGetDesktopWindow() &&
          Window->spwndOwner == NULL &&
          (!(Window->ExStyle & WS_EX_TOOLWINDOW) ||
           (Window->ExStyle & WS_EX_APPWINDOW)))
      {
         // FIXME lParam; The value is TRUE if the window is in full-screen mode, or FALSE otherwise.
         co_IntShellHookNotify(HSHELL_WINDOWACTIVATED, (WPARAM) UserHMGetHandle(Window), FALSE);
      }
      else
      {
          co_IntShellHookNotify(HSHELL_WINDOWACTIVATED, 0, FALSE);
      }

      Window->state &= ~WNDS_NONCPAINT;

      UserDerefObjectCo(Window);
   }
   return InAAPM;
}

VOID FASTCALL
IntSendFocusMessages( PTHREADINFO pti, PWND pWnd)
{
   PWND pWndPrev;
   PUSER_MESSAGE_QUEUE ThreadQueue = pti->MessageQueue; // Queue can change...

   ThreadQueue->QF_flags &= ~QF_FOCUSNULLSINCEACTIVE;
   if (!pWnd && ThreadQueue->spwndActive)
   {
      ThreadQueue->QF_flags |= QF_FOCUSNULLSINCEACTIVE;
   }

   pWndPrev = ThreadQueue->spwndFocus;

   /* check if the specified window can be set in the input data of a given queue */
   if (!pWnd || ThreadQueue == pWnd->head.pti->MessageQueue)
      /* set the current thread focus window */
      ThreadQueue->spwndFocus = pWnd;

   if (pWnd)
   {
      if (pWndPrev)
      {
         //co_IntPostOrSendMessage(UserHMGetHandle(pWndPrev), WM_KILLFOCUS, (WPARAM)UserHMGetHandle(pWnd), 0);
         co_IntSendMessage(UserHMGetHandle(pWndPrev), WM_KILLFOCUS, (WPARAM)UserHMGetHandle(pWnd), 0);
      }
      if (ThreadQueue->spwndFocus == pWnd)
      {
         IntNotifyWinEvent(EVENT_OBJECT_FOCUS, pWnd, OBJID_CLIENT, CHILDID_SELF, 0);
         //co_IntPostOrSendMessage(UserHMGetHandle(pWnd), WM_SETFOCUS, (WPARAM)(pWndPrev ? UserHMGetHandle(pWndPrev) : NULL), 0);
         co_IntSendMessage(UserHMGetHandle(pWnd), WM_SETFOCUS, (WPARAM)(pWndPrev ? UserHMGetHandle(pWndPrev) : NULL), 0);
      }
   }
   else
   {
      if (pWndPrev)
      {
         IntNotifyWinEvent(EVENT_OBJECT_FOCUS, NULL, OBJID_CLIENT, CHILDID_SELF, 0);
         //co_IntPostOrSendMessage(UserHMGetHandle(pWndPrev), WM_KILLFOCUS, 0, 0);
         co_IntSendMessage(UserHMGetHandle(pWndPrev), WM_KILLFOCUS, 0, 0);
      }
   }
}

VOID FASTCALL
FindRemoveAsyncMsg(PWND Wnd, WPARAM wParam)
{
   PTHREADINFO pti;
   PUSER_SENT_MESSAGE Message;
   PLIST_ENTRY Entry;

   if (!Wnd) return;

   pti = Wnd->head.pti;

   Entry = pti->SentMessagesListHead.Flink;
   while (Entry != &pti->SentMessagesListHead)
   {
      // Scan sent queue messages to see if we received async messages.
      Message = CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry);
      Entry = Entry->Flink;

      if (Message->Msg.message == WM_ASYNC_SETACTIVEWINDOW &&
          Message->Msg.hwnd == UserHMGetHandle(Wnd) &&
          Message->Msg.wParam == wParam)
      {
         WARN("ASYNC SAW: Found one in the Sent Msg Queue! %p Activate/Deactivate %d\n", Message->Msg.hwnd, !!wParam);
         RemoveEntryList(&Message->ListEntry); // Purge the entry.
         ClearMsgBitsMask(pti, Message->QS_Flags);
         ExFreePoolWithTag(Message, TAG_USRMSG);
      }
   }
}

BOOL FASTCALL
ToggleFGActivate(PTHREADINFO pti)
{
   BOOL Ret;
   PPROCESSINFO ppi = pti->ppi;

   Ret = !!(pti->TIF_flags & TIF_ALLOWFOREGROUNDACTIVATE);
   if (Ret)
   {
      pti->TIF_flags &= ~TIF_ALLOWFOREGROUNDACTIVATE;
   }
   else
      Ret = !!(ppi->W32PF_flags & W32PF_ALLOWFOREGROUNDACTIVATE);

   if (Ret)
      ppi->W32PF_flags &= ~W32PF_ALLOWFOREGROUNDACTIVATE;
   //ERR("ToggleFGActivate is %d\n",Ret);
   return Ret;
}

BOOL FASTCALL
IsAllowedFGActive(PTHREADINFO pti, PWND Wnd)
{
   // Not allowed if one or more,,
   if (!ToggleFGActivate(pti) ||              // bits not set,
        pti->rpdesk != gpdeskInputDesktop ||  // not current Desktop,
        pti->MessageQueue == gpqForeground || // if already the queue foreground,
        IsFGLocked() ||                       // foreground is locked,
        Wnd->ExStyle & WS_EX_NOACTIVATE )     // or,,, does not become the foreground window when the user clicks it.
   {
      return FALSE;
   }
   //ERR("IsAllowedFGActive is TRUE\n");
   return TRUE;
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
   //ERR("CanForceFG is FALSE\n");
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
static
BOOL FASTCALL
co_IntSetForegroundAndFocusWindow(
    _In_opt_ PWND Wnd,
    _In_ BOOL MouseActivate)
{
   HWND hWnd = Wnd ? UserHMGetHandle(Wnd) : NULL;
   HWND hWndPrev = NULL;
   PWND pWndPrev = NULL;
   PUSER_MESSAGE_QUEUE PrevForegroundQueue;
   PTHREADINFO pti;
   BOOL fgRet = FALSE, Ret = FALSE;

   if (Wnd) ASSERT_REFS_CO(Wnd);

   //ERR("SetForegroundAndFocusWindow(%x, %s)\n", hWnd, (MouseActivate ? "TRUE" : "FALSE"));

   PrevForegroundQueue = IntGetFocusMessageQueue(); // Use this active desktop.
   pti = PsGetCurrentThreadWin32Thread();

   if (PrevForegroundQueue)
   {  // Same Window Q as foreground just do active.
      if (Wnd && Wnd->head.pti->MessageQueue == PrevForegroundQueue)
      {
         //ERR("Same Window Q as foreground just do active.\n");
         if (pti->MessageQueue == PrevForegroundQueue)
         { // Same WQ and TQ go active.
            //ERR("Same WQ and TQ go active.\n");
            Ret = co_IntSetActiveWindow(Wnd, MouseActivate, TRUE, FALSE);
         }
         else if (Wnd->head.pti->MessageQueue->spwndActive == Wnd)
         { // Same WQ and it is active.
            //ERR("Same WQ and it is active.\n");
            Ret = TRUE;
         }
         else
         { // Same WQ as FG but not the same TQ send active.
            //ERR("Same WQ as FG but not the same TQ send active.\n");
            co_IntSendMessage(hWnd, WM_ASYNC_SETACTIVEWINDOW, (WPARAM)Wnd, (LPARAM)MouseActivate );
            Ret = TRUE;
         }
         return Ret;
      }

      hWndPrev = PrevForegroundQueue->spwndActive ? UserHMGetHandle(PrevForegroundQueue->spwndActive) : 0;
      pWndPrev = PrevForegroundQueue->spwndActive;
   }

   if ( (( !IsFGLocked() || pti->ppi == gppiInputProvider ) &&
         ( CanForceFG(pti->ppi) || pti->TIF_flags & (TIF_SYSTEMTHREAD|TIF_CSRSSTHREAD|TIF_ALLOWFOREGROUNDACTIVATE) )) ||
        pti->ppi == ppiScrnSaver
      )
   {

      //ToggleFGActivate(pti); // win.c line 2662 fail
      if (Wnd)
      {
         IntSetFocusMessageQueue(Wnd->head.pti->MessageQueue);
         gptiForeground = Wnd->head.pti;
         //ERR("Set Foreground pti 0x%p Q 0x%p hWnd 0x%p\n",Wnd->head.pti, Wnd->head.pti->MessageQueue,Wnd->head.h);
      }
      else
      {
         IntSetFocusMessageQueue(NULL);
         gptiForeground = NULL;
         //ERR("Set Foreground pti 0x0 Q 0x0 hWnd 0x0\n");
      }
/*
     Henri Verbeet,
     What happens is that we get the WM_WINE_SETACTIVEWINDOW message sent by the
     other thread after we already changed the foreground window back to our own
     window.
 */
      //ERR("SFAFW: 1\n");
      FindRemoveAsyncMsg(Wnd, 0); // Do this to fix test_SFW todos!

      fgRet = TRUE;
   }

   //  Fix FG Bounce with regedit.
   if (hWndPrev != hWnd )
   {
      if (PrevForegroundQueue &&
          fgRet &&
          PrevForegroundQueue->spwndActive)
      {
         //ERR("SFGW: Send NULL to 0x%x\n",hWndPrev);
         if (pti->MessageQueue == PrevForegroundQueue)
         {
            //ERR("SFGW: TI same as Prev TI\n");
            co_IntSetActiveWindow(NULL, FALSE, TRUE, FALSE);
         }
         else if (pWndPrev)
         {
            //ERR("SFGW Deactivate: TI not same as Prev TI\n");
            // No real reason to wait here.
            co_IntSendMessageNoWait(hWndPrev, WM_ASYNC_SETACTIVEWINDOW, 0, 0 );
         }
      }
   }

   if (!Wnd) return FALSE; // Always return false.

   if (pti->MessageQueue == Wnd->head.pti->MessageQueue)
   {
       //ERR("Same PQ and WQ go active.\n");
       Ret = co_IntSetActiveWindow(Wnd, MouseActivate, TRUE, FALSE);
   }
   else if (Wnd->head.pti->MessageQueue->spwndActive == Wnd)
   {
       //ERR("Same Active and Wnd.\n");
       Ret = TRUE;
   }
   else
   {
       //ERR("Activate Not same PQ and WQ and Wnd.\n");
       co_IntSendMessage(hWnd, WM_ASYNC_SETACTIVEWINDOW, (WPARAM)Wnd, (LPARAM)MouseActivate );
       Ret = TRUE;
   }
   return Ret && fgRet;
}

BOOL FASTCALL
co_IntSetActiveWindow(PWND Wnd OPTIONAL, BOOL bMouse, BOOL bFocus, BOOL Async)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   PWND pWndChg, WndPrev; // State changes.
   HWND hWndPrev;
   HWND hWnd = 0;
   BOOL InAAPM;
   CBTACTIVATESTRUCT cbt;
   //ERR("co_IntSetActiveWindow 1\n");
   if (Wnd)
   {
      ASSERT_REFS_CO(Wnd);
      hWnd = UserHMGetHandle(Wnd);
      if ((Wnd->style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;
      if (Wnd == UserGetDesktopWindow()) return FALSE;
      //ERR("co_IntSetActiveWindow 1a hWnd 0x%p\n",hWnd);
   }

   //ERR("co_IntSetActiveWindow 2\n");
   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   ASSERT(ThreadQueue != 0);

   hWndPrev = ThreadQueue->spwndActive ? UserHMGetHandle(ThreadQueue->spwndActive) : NULL;

   pWndChg = ThreadQueue->spwndActive; // Keep to notify of a preemptive switch.

   while (Wnd)
   {
      BOOL Ret, DoFG, AllowFG;

      if (Wnd->state & WNDS_BEINGACTIVATED) return TRUE;

      if (ThreadQueue == Wnd->head.pti->MessageQueue)
      {
         if (IsAllowedFGActive(pti, Wnd))
         {
             DoFG = TRUE;
         }
         else
         {
             //ERR("co_IntSetActiveWindow 3 Go Out!\n");
             break;
         }
         AllowFG = !pti->cVisWindows; // Nothing is visable.
         //ERR("co_IntSetActiveWindow 3a DoFG = %d AllowFG = %d\n",DoFG,AllowFG);
      }
      else //if (ThreadQueue != Wnd->head.pti->MessageQueue)
      {
         //PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetFocusMessageQueue();
         // Rule 1 & 4, We are foreground so set this FG window or NULL foreground....
         //if (!ForegroundQueue || ForegroundQueue == ThreadQueue)
         if (!gpqForeground || gpqForeground == ThreadQueue)
         {
            DoFG = TRUE;
         }
         else
            DoFG = FALSE;
         if (DoFG)
         {
            if (pti->TIF_flags & TIF_ALLOWFOREGROUNDACTIVATE || pti->cVisWindows)
               AllowFG = TRUE;
            else
               AllowFG = FALSE;
         }
         else
            AllowFG = FALSE;
         //ERR("co_IntSetActiveWindow 3b DoFG = %d AllowFG = %d\n",DoFG,AllowFG);
      }
      Ret = FALSE;
      if (DoFG)
      {
         pti->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
         //ERR("co_IntSetActiveWindow 3c FG set\n");
         Ret = co_IntSetForegroundAndFocusWindow(Wnd, bMouse);
         if (AllowFG)
         {
            pti->TIF_flags |= TIF_ALLOWFOREGROUNDACTIVATE;
         }
         else
         {
            pti->TIF_flags &= ~TIF_ALLOWFOREGROUNDACTIVATE;
         }
      }
      return Ret;
   }

   /* Call CBT hook chain */
   cbt.fMouse     = bMouse;
   cbt.hWndActive = hWndPrev;
   if (co_HOOK_CallHooks( WH_CBT, HCBT_ACTIVATE, (WPARAM)hWnd, (LPARAM)&cbt))
   {
      ERR("SetActiveWindow: WH_CBT Call Hook return!\n");
      return FALSE;
   }

   if ( ThreadQueue->spwndActive && ThreadQueue->spwndActive->state & WNDS_DESTROYED )
      ThreadQueue->spwndActive = NULL;
   else
      ThreadQueue->spwndActivePrev = ThreadQueue->spwndActive;

   WndPrev = ThreadQueue->spwndActive; // Keep to save changing active.

   if (WndPrev)
   {
      if (ThreadQueue == gpqForeground) gpqForegroundPrev = ThreadQueue;
      if (!co_IntSendDeactivateMessages(UserHMGetHandle(WndPrev), hWnd)) return FALSE;
   }

   WndPrev = ThreadQueue->spwndActive; // Again keep to save changing active.

   // While in calling message proc or hook:
   // Fail if a preemptive switch was made, current active not made previous,
   // focus window is dead or no longer the same thread queue.
   if ( ThreadQueue->spwndActivePrev != ThreadQueue->spwndActive ||
        pWndChg != WndPrev ||
        (Wnd && !VerifyWnd(Wnd)) ||
        ThreadQueue != pti->MessageQueue )
   {
      ERR("SetActiveWindow: Summary ERROR, active state changed!\n");
      return FALSE;
   }

   if (!WndPrev) ThreadQueue->QF_flags &= ~QF_FOCUSNULLSINCEACTIVE;

   if (Wnd) Wnd->state |= WNDS_BEINGACTIVATED;

   IntNotifyWinEvent(EVENT_SYSTEM_FOREGROUND, Wnd, OBJID_WINDOW, CHILDID_SELF, WEF_SETBYWNDPTI);
   //// Breaks Atl-Esc/Tab via User32.
   ////FindRemoveAsyncMsg(Wnd,(WPARAM)Wnd); // Clear out activate ASYNC messages.

   /* check if the specified window can be set in the input data of a given queue */
   if ( !Wnd || ThreadQueue == Wnd->head.pti->MessageQueue)
   {
      /* set the current thread active window */
      ThreadQueue->spwndActive = Wnd;
   }

   WndPrev = VerifyWnd(ThreadQueue->spwndActivePrev); // Now should be set but verify it again.

   InAAPM = co_IntSendActivateMessages(WndPrev, Wnd, bMouse, Async);

   /* now change focus if necessary */
   //// Fixes CORE-6452 allows setting focus on window.
   if (bFocus && !(ThreadQueue->QF_flags & QF_FOCUSNULLSINCEACTIVE))
   {
      /* Do not change focus if the window is no longer active */
      if (pti->MessageQueue->spwndActive != IntGetNonChildAncestor(pti->MessageQueue->spwndFocus))
      {
         PWND pWndSend = pti->MessageQueue->spwndActive;
         // Clear focus if the active window is minimized.
         if (pWndSend && pti->MessageQueue->spwndActive->style & WS_MINIMIZE) pWndSend = NULL;
         // Send focus messages and if so, set the focus.
         IntSendFocusMessages( pti, pWndSend);
      }
   }
   ////
   if (InAAPM)
   {
      pti->TIF_flags &= ~TIF_INACTIVATEAPPMSG;
   }

   // FIXME: Used in the menu loop!!!
   //ThreadQueue->QF_flags |= QF_ACTIVATIONCHANGE;

   //ERR("co_IntSetActiveWindow Exit\n");
   if (Wnd) Wnd->state &= ~WNDS_BEINGACTIVATED;
   return (ThreadQueue->spwndActive == Wnd);
}

BOOL FASTCALL
co_IntMouseActivateWindow(PWND Wnd)
{
   TRACE("Mouse Active\n");
   return co_IntSetForegroundAndFocusWindow(Wnd, TRUE);
}

BOOL FASTCALL
UserSetActiveWindow(PWND Wnd)
{
  PTHREADINFO pti = PsGetCurrentThreadWin32Thread();

  if (Wnd) // Must have a window!
  {
     if ((Wnd->style & (WS_POPUP|WS_CHILD)) == WS_CHILD) return FALSE;

     return co_IntSetActiveWindow(Wnd, FALSE, TRUE, FALSE);
  }
  /*
     Yes your eye are not deceiving you~!

     First part of wines Win.c test_SetActiveWindow:

     flush_events( TRUE );
     ShowWindow(hwnd, SW_HIDE);
     SetFocus(0);
     SetActiveWindow(0);
     check_wnd_state(0, 0, 0, 0); <-- This should pass if ShowWindow does it's job!!! As of 10/28/2012 it does!

     Now Handle wines Msg.c test_SetActiveWindow( 0 )...
  */
  TRACE("USAW: Previous active window\n");
  if (  gpqForegroundPrev &&
        gpqForegroundPrev->spwndActivePrev &&
       (gpqForegroundPrev->spwndActivePrev->style & (WS_VISIBLE|WS_DISABLED)) == WS_VISIBLE  &&
      !(gpqForegroundPrev->spwndActivePrev->state2 & WNDS2_BOTTOMMOST) &&
       (Wnd = VerifyWnd(gpqForegroundPrev->spwndActivePrev)) != NULL )
  {
     TRACE("USAW:PAW hwnd %p\n",Wnd?Wnd->head.h:NULL);
     return co_IntSetActiveWindow(Wnd, FALSE, TRUE, FALSE);
  }

  // Activate anyone but the active window.
  if ( pti->MessageQueue->spwndActive &&
      (Wnd = VerifyWnd(pti->MessageQueue->spwndActive)) != NULL )
  {
      ERR("USAW:AOWM hwnd %p\n",Wnd?Wnd->head.h:NULL);
      if (!ActivateOtherWindowMin(Wnd))
      {
         // Okay, now go find someone else to play with!
         ERR("USAW: Going to WPAOW\n");
         co_WinPosActivateOtherWindow(Wnd);
      }
      return TRUE;
  }

  TRACE("USAW: Nothing\n");
  return FALSE;
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

   TRACE("Enter SetFocus hWnd 0x%p pti 0x%p\n",Window ? UserHMGetHandle(Window) : 0, pti );

   hWndPrev = ThreadQueue->spwndFocus ? UserHMGetHandle(ThreadQueue->spwndFocus) : 0;

   if (Window != 0)
   {
      if (hWndPrev == UserHMGetHandle(Window))
      {
         return hWndPrev ? (IntIsWindow(hWndPrev) ? hWndPrev : 0) : 0; /* Nothing to do */
      }

      if (Window->head.pti->MessageQueue != ThreadQueue)
      {
         ERR("SetFocus Must have the same Q!\n");
         return 0;
      }

      /* Check if we can set the focus to this window */
      //// Fixes wine win test_SetParent both "todo" line 3710 and 3720...
      for (pwndTop = Window; pwndTop; pwndTop = pwndTop->spwndParent)
      {
         if (pwndTop->style & (WS_MINIMIZED|WS_DISABLED)) return 0;
         if ((pwndTop->style & (WS_POPUP|WS_CHILD)) != WS_CHILD) break;
         if (pwndTop->spwndParent == NULL) break;
      }
      ////
      if (co_HOOK_CallHooks( WH_CBT, HCBT_SETFOCUS, (WPARAM)Window->head.h, (LPARAM)hWndPrev))
      {
         ERR("SetFocus 1 WH_CBT Call Hook return!\n");
         return 0;
      }

      /* Activate pwndTop if needed. */
      if (pwndTop != ThreadQueue->spwndActive)
      {
         PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetFocusMessageQueue(); // Keep it based on desktop.
         if (ThreadQueue != ForegroundQueue && IsAllowedFGActive(pti, pwndTop)) // Rule 2 & 3.
         {
            //ERR("SetFocus: Set Foreground!\n");
            if (!(pwndTop->style & WS_VISIBLE))
            {
                pti->ppi->W32PF_flags |= W32PF_ALLOWFOREGROUNDACTIVATE;
            }
            if (!co_IntSetForegroundAndFocusWindow(pwndTop, FALSE))
            {
               ERR("SetFocus: Set Foreground and Focus Failed!\n");
               return 0;
            }
         }

         /* Set Active when it is needed. */
         if (pwndTop != ThreadQueue->spwndActive)
         {
            //ERR("SetFocus: Set Active!\n");
            if (!co_IntSetActiveWindow(pwndTop, FALSE, FALSE, FALSE))
            {
               ERR("SetFocus: Set Active Failed!\n");
               return 0;
            }
         }

         /* Abort if window destroyed */
         if (Window->state2 & WNDS2_INDESTROY) return 0;
         /* Do not change focus if the window is no longer active */
         if (pwndTop != ThreadQueue->spwndActive)
         {
            ERR("SetFocus: Top window did not go active!\n");
            return 0;
         }
      }

      // Check again! SetActiveWindow could have set the focus via WM_ACTIVATE.
      hWndPrev = ThreadQueue->spwndFocus ? UserHMGetHandle(ThreadQueue->spwndFocus) : 0;

      IntSendFocusMessages( pti, Window);

      TRACE("Focus: %p -> %p\n", hWndPrev, Window->head.h);
   }
   else /* NULL hwnd passed in */
   {
      if (co_HOOK_CallHooks( WH_CBT, HCBT_SETFOCUS, (WPARAM)0, (LPARAM)hWndPrev))
      {
         ERR("SetFocus: 2 WH_CBT Call Hook return!\n");
         return 0;
      }

      /* set the current thread focus window null */
      IntSendFocusMessages( pti, NULL);
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
   RETURN( ThreadQueue ? (ThreadQueue->spwndCapture ? UserHMGetHandle(ThreadQueue->spwndCapture) : 0) : 0);

CLEANUP:
   TRACE("Leave IntGetCapture, ret=%p\n", _ret_);
   END_CLEANUP;
}

HWND FASTCALL
co_UserSetCapture(HWND hWnd)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   PWND pWnd, Window = NULL;
   HWND hWndPrev;

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;

   if (ThreadQueue->QF_flags & QF_CAPTURELOCKED)
      return NULL;

   if (hWnd && (Window = UserGetWindowObject(hWnd)))
   {
      if (Window->head.pti->MessageQueue != ThreadQueue)
      {
         ERR("Window Thread dos not match Current!\n");
         return NULL;
      }
   }

   hWndPrev = MsqSetStateWindow(pti, MSQ_STATE_CAPTURE, hWnd);

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

      co_IntSendMessage(hWndPrev, WM_CAPTURECHANGED, 0, (LPARAM)hWnd);

      ThreadQueue->QF_flags &= ~QF_CAPTURELOCKED;
   }

   if (hWnd == NULL) // Release mode.
   {
      MOUSEINPUT mi;
   /// These are HACKS!
      /* Also remove other windows if not capturing anymore */
      MsqSetStateWindow(pti, MSQ_STATE_MENUOWNER, NULL);
      MsqSetStateWindow(pti, MSQ_STATE_MOVESIZE, NULL);
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
   if (Window) ASSERT_REFS_CO(Window);

   return co_IntSetForegroundAndFocusWindow(Window, FALSE);
}

/*
  API Call
*/
BOOL FASTCALL
co_IntSetForegroundWindowMouse(PWND Window)
{
   if (Window) ASSERT_REFS_CO(Window);

   return co_IntSetForegroundAndFocusWindow(Window, TRUE);
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
      //ERR("ptiLastInput is CLEARED!!\n");
      ptiLastInput = NULL;
   }
   else
   {  // Rule #3, last input event in force.
      ERR("ptiLastInput is SET!!\n");
      //ptiLastInput = ppi->ptiList; // See CORE-6384 & CORE-7030.
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
   TRACE("Leave NtUserGetForegroundWindow, ret=%p\n",_ret_);
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

   TRACE("Enter NtUserSetActiveWindow(%p)\n", hWnd);
   UserEnterExclusive();

   Window = NULL;
   if (hWnd)
   {
      if (!(Window = UserGetWindowObject(hWnd)))
      {
         ERR("NtUserSetActiveWindow: Invalid handle 0x%p!\n",hWnd);
         RETURN( NULL);
      }
   }

   if (!Window ||
        Window->head.pti->MessageQueue == gptiCurrent->MessageQueue)
   {
      hWndPrev = gptiCurrent->MessageQueue->spwndActive ? UserHMGetHandle(gptiCurrent->MessageQueue->spwndActive) : NULL;
      if (Window) UserRefObjectCo(Window, &Ref);
      UserSetActiveWindow(Window);
      if (Window) UserDerefObjectCo(Window);
      RETURN( hWndPrev ? (IntIsWindow(hWndPrev) ? hWndPrev : 0) : 0 );
   }
   RETURN( NULL);

CLEANUP:
   TRACE("Leave NtUserSetActiveWindow, ret=%p\n",_ret_);
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

   TRACE("Enter NtUserSetCapture(%p)\n", hWnd);
   UserEnterExclusive();

   RETURN( co_UserSetCapture(hWnd));

CLEANUP:
   TRACE("Leave NtUserSetCapture, ret=%p\n", _ret_);
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

   TRACE("Enter NtUserSetFocus(%p)\n", hWnd);
   UserEnterExclusive();

   if (hWnd)
   {
      if (!(Window = UserGetWindowObject(hWnd)))
      {
         ERR("NtUserSetFocus: Invalid handle 0x%p!\n",hWnd);
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
   TRACE("Leave NtUserSetFocus, ret=%p\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/* EOF */
