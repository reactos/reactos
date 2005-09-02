/*
 * ReactOS Win32 Subsystem
 * Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 *
 * $Id$
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS *******************************************************************/

/* This is knows as the Foreground Input Queue or just Foreground Queue in MS docs.
 * This is somewhat confusing since its not really a queue...
 *
 * Why have this global var u say? Why not use gForegroundQueue->Input?
 * This is bcause gForegroundQueue may be NULL (the foreground thread exited) while
 * its refcounted Input might still be around (someone attached to it). We need a way
 * to get to the Input after the thread is gone (when gForegroundQueue is NULL).
 *
 * But...why not drop gForegroundQueue, add a thread
 * backpointer in the input struct and only use gForegroundInput u might think?
 * That wont work! Remeber that no single thread owns the Input. The Input can be
 * owned by many threads.
 * -Gunnar
 */
//PUSER_THREAD_INPUT gForegroundInput = NULL;


/* This is knows as the Foreground Thread in MS docs.
 * Since the message queue and the w32thread has 1:1 relship, its just a
 * matter of taste if we want this named gForegroundQueue or gForegroundWThread.
 * They point to the same thing anyways.
 * -Gunnar
 */
//PUSER_MESSAGE_QUEUE gForegroundQueue = NULL;


/* FUNCTIONS *****************************************************************/


//inline PUSER_MESSAGE_QUEUE FASTCALL UserGetForegroundQueue()
//{
//   return gInputDesktop->ActiveQueue;
//}

//inline VOID FASTCALL UserSetForegroundQueue(PUSER_MESSAGE_QUEUE Queue)
//{
//   gInputDesktop->ActiveQueue = Queue;
//}


PWINDOW_OBJECT FASTCALL
UserGetCaptureWindow()
{
   PUSER_MESSAGE_QUEUE ForegroundQueue = UserGetForegroundQueue();
   return ForegroundQueue ? GetWnd(ForegroundQueue->Input->hCaptureWindow) : 0;
}

PWINDOW_OBJECT FASTCALL
UserGetFocusWindow()
{
   PUSER_MESSAGE_QUEUE ForegroundQueue = UserGetForegroundQueue();
   return ForegroundQueue ? GetWnd(ForegroundQueue->Input->hFocusWindow) : 0;
}

PWINDOW_OBJECT FASTCALL
UserGetThreadFocusWindow()
{
   PUSER_MESSAGE_QUEUE Queue = UserGetCurrentQueue();
   return Queue ? GetWnd(Queue->Input->hFocusWindow) : 0;
}

VOID FASTCALL
co_UserSendDeactivateMessages(PWINDOW_OBJECT WndPrev, PWINDOW_OBJECT Wnd OPTIONAL)
{
   if (WndPrev)
   {
      co_UserSendMessage(WndPrev->hSelf, WM_NCACTIVATE, FALSE, 0);
      co_UserSendMessage(WndPrev->hSelf, WM_ACTIVATE,
                           MAKEWPARAM(WA_INACTIVE, WndPrev->Style & WS_MINIMIZE),
                           (LPARAM) GetHwndSafe(Wnd));
   }
}

VOID FASTCALL
co_UserSendActivateMessages(PWINDOW_OBJECT Wnd, PWINDOW_OBJECT WndPrev OPTIONAL, BOOL MouseActivate)
{
   PWINDOW_OBJECT Owner, Parent;

   if (Wnd)
   {
      /* Send palette messages */
      if (co_UserSendMessage(Wnd->hSelf, WM_QUERYNEWPALETTE, 0, 0))
      {
         co_UserSendMessage(HWND_BROADCAST, WM_PALETTEISCHANGING, (WPARAM)Wnd->hSelf, 0);
      }

      if (UserGetWindow(Wnd, GW_HWNDPREV) != NULL)
         co_WinPosSetWindowPos(Wnd->hSelf, HWND_TOP, 0, 0, 0, 0,
                            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

//      Window = UserGetWindowObject(hWnd);
//      if (Window)
//      {
      Owner = UserGetOwner(Wnd);
      if (!Owner)
      {
         Parent = UserGetParent(Wnd);
         if (!Parent)
            co_UserShellHookNotify(HSHELL_WINDOWACTIVATED, (LPARAM) Wnd->hSelf);
      }

//      }

      /* FIXME: IntIsWindow */

      co_UserSendMessage(Wnd->hSelf, WM_NCACTIVATE, (WPARAM)(Wnd == UserGetForegroundWindow()), 0);
      /* FIXME: WA_CLICKACTIVE */
      co_UserSendMessage(Wnd->hSelf, WM_ACTIVATE,
                           MAKEWPARAM(MouseActivate ? WA_CLICKACTIVE : WA_ACTIVE,
                                      Wnd->Style & WS_MINIMIZE),
                           (LPARAM) GetHwndSafe(WndPrev) );
   }
}


HWND FASTCALL
IntFindChildWindowToOwner(PWINDOW_OBJECT Root, PWINDOW_OBJECT Owner)
{
   HWND Ret;
   PWINDOW_OBJECT Child, OwnerWnd;

   for(Child = Root->FirstChild; Child; Child = Child->NextSibling)
   {
      OwnerWnd = UserGetWindowObject(Child->hOwnerWnd);

      if(!OwnerWnd)
         continue;

      if(OwnerWnd == Owner)
      {
         Ret = Child->hSelf;
         return Ret;
      }
   }

   return NULL;
}

STATIC BOOL FASTCALL
co_UserSetForegroundAndFocusWindow(PWINDOW_OBJECT Window, PWINDOW_OBJECT FocusWindow, BOOL MouseActivate)
{
   HWND hWnd = Window->hSelf;
   HWND hWndPrev = NULL;
   HWND hWndFocus = FocusWindow->hSelf;
   HWND hWndFocusPrev = NULL;
   PUSER_MESSAGE_QUEUE PrevForegroundQueue;

   DPRINT("IntSetForegroundAndFocusWindow(%x, %x, %s)\n", hWnd, hWndFocus, MouseActivate ? "TRUE" : "FALSE");
   DPRINT("(%wZ)\n", &Window->WindowName);

   if ((Window->Style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
   {
      DPRINT("Failed - Child\n");
      return FALSE;
   }

   if (0 == (Window->Style & WS_VISIBLE))
   {
      DPRINT("Failed - Invisible\n");
      return FALSE;
   }

   PrevForegroundQueue = UserGetForegroundQueue();
   if (PrevForegroundQueue)
   {
      //FIXME uhm... focus er ikke samme som active window... er UserGetFocusMessageQueue riktig?
      hWndPrev = PrevForegroundQueue->Input->hActiveWindow;
   }

   if (hWndPrev == hWnd)
   {
      DPRINT("Failed - Same\n");
      return TRUE;
   }

   hWndFocusPrev = (PrevForegroundQueue == FocusWindow->Queue
                    ? FocusWindow->Queue->Input->hFocusWindow : NULL); //FIXME: huhuh??

   /* FIXME: Call hooks. */

   co_UserSendDeactivateMessages(GetWnd(hWndPrev), GetWnd(hWnd));
   //IntSendKillFocusMessages(GetWnd(hWndFocusPrev), GetWnd(hWndFocus));
   if (hWndFocusPrev)
      co_UserSendMessage(hWndFocusPrev, WM_KILLFOCUS, (WPARAM)hWndFocus, 0);

   UserSetForegroundQueue(Window->Queue);
   
   if (Window->Queue->Input)
   {
      Window->Queue->Input->hActiveWindow = hWnd;
   }
   
   if (FocusWindow->Queue->Input)
   {
      FocusWindow->Queue->Input->hFocusWindow = hWndFocus;
   }

   if (PrevForegroundQueue != Window->Queue)
   {
      /* FIXME: Send WM_ACTIVATEAPP to all thread windows. */
   }

   //IntSendSetFocusMessages(GetWnd(hWndFocusPrev), GetWnd(hWndFocus));
   if (hWndFocus)
      co_UserSendMessage(hWndFocus, WM_SETFOCUS, (WPARAM)hWndFocusPrev, 0);
   
   co_UserSendActivateMessages(GetWnd(hWnd), GetWnd(hWndPrev), MouseActivate);

   return TRUE;
}

BOOL FASTCALL
co_UserSetForegroundWindow(PWINDOW_OBJECT Window)
{
   return co_UserSetForegroundAndFocusWindow(Window, Window, FALSE);
}

BOOL FASTCALL
co_UserMouseActivateWindow(PWINDOW_OBJECT Window)
{
   PWINDOW_OBJECT TopWnd;

   if(Window->Style & WS_DISABLED)
   {
      //    PWINDOW_OBJECT TopWnd;
      PWINDOW_OBJECT DesktopWindow = UserGetDesktopWindow();

      if(DesktopWindow)
      {
         TopWnd = GetWnd(IntFindChildWindowToOwner(DesktopWindow, Window));
         if(TopWnd)//(TopWnd = IntGetWindowObject(Top)))
         {
            //FIXME: recursion... find another way?
            return co_UserMouseActivateWindow(TopWnd);
         }
      }
      return FALSE;
   }

   TopWnd = UserGetAncestor(Window, GA_ROOT);
   if (TopWnd == NULL)
      TopWnd = UserGetDesktopWindow();

   /* TMN: Check return value from this function? */
   co_UserSetForegroundAndFocusWindow(TopWnd, Window, TRUE);

   return TRUE;
}

PWINDOW_OBJECT FASTCALL co_UserSetActiveWindow(PWINDOW_OBJECT Wnd OPTIONAL)
{
   PUSER_MESSAGE_QUEUE Queue;
   PWINDOW_OBJECT PrevWnd;

   Queue = UserGetCurrentQueue();

   ASSERT(Queue);

   PrevWnd = UserGetWindowObject(Queue->Input->hActiveWindow);
   
   if (Wnd)
   {
      /* window must be attached to the calling thread input queue (callers of this func must verify )*/ 
//remove this for now...      ASSERT(Wnd->Queue->Input == Queue->Input);
      
      if (!(Wnd->Style & WS_VISIBLE) || (Wnd->Style & (WS_POPUP | WS_CHILD)) == WS_CHILD)
      {
         return PrevWnd;
      }
   }

   if (PrevWnd == Wnd)
      return PrevWnd;

   /* set last active for window and its owner */ /* dunno if its the right place (eg. move this code up/down) */
   if (Wnd)
   {
      PWINDOW_OBJECT Owner = UserGetOwner(Wnd);
      if (Owner) Owner->hLastActiveWnd = Wnd->hSelf;
      
      Wnd->hLastActiveWnd = Wnd->hSelf;
   }

   //FIXME!!! hva hvis prev active var invalid dvs. NULL? skal vi returnere her nå??
   
   /* FIXME: Call hooks. */

   //FIXME!!! hva hvis prev active var invalid dvs. NULL?
   //FIXME!!! hva hvis Wnd er NULL?
   co_UserSendDeactivateMessages(PrevWnd, Wnd);
   co_UserSendActivateMessages(Wnd, PrevWnd, FALSE);

   /* FIXME */
   /*   return IntIsWindow(hWndPrev) ? hWndPrev : 0;*/
   
   return PrevWnd;
}

PWINDOW_OBJECT FASTCALL co_UserSetFocusWindow(PWINDOW_OBJECT Wnd OPTIONAL)
{
   PWINDOW_OBJECT PrevWnd;
   PUSER_MESSAGE_QUEUE Queue;

   Queue = UserGetCurrentQueue();

   ASSERT(Queue);

   PrevWnd = UserGetWindowObject(Queue->Input->hFocusWindow);

   if (PrevWnd == Wnd)
      return PrevWnd;

   Queue->Input->hFocusWindow = GetHwndSafe(Wnd);

   if (PrevWnd)
      co_UserSendMessage(PrevWnd->hSelf, WM_KILLFOCUS, (WPARAM) GetHwndSafe(Wnd), 0);

   if (Wnd)
      co_UserSendMessage(Wnd->hSelf, WM_SETFOCUS, (WPARAM) GetHwndSafe(PrevWnd), 0);   

   return PrevWnd;
}

/*
 * @implemented
 */
HWND STDCALL
NtUserGetForegroundWindow(VOID)
{
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetForegroundWindow\n");
   UserEnterExclusive();

   //PUSER_MESSAGE_QUEUE ForegroundQueue = UserGetFocusMessageQueue();
   //RETURN(ForegroundQueue != NULL ? ForegroundQueue->ActiveWindow : 0);
   RETURN( GetHwndSafe(UserGetForegroundWindow()));

CLEANUP:
   DPRINT("Leave NtUserGetForegroundWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}



PWINDOW_OBJECT FASTCALL
UserGetForegroundWindow(VOID)
{
   PUSER_MESSAGE_QUEUE ForegroundQueue = UserGetForegroundQueue();
   if (ForegroundQueue && ForegroundQueue->Input->hActiveWindow)
      return UserGetWindowObject(ForegroundQueue->Input->hActiveWindow);

   return NULL;
}


/*
 * @implemented
 */
HWND STDCALL
NtUserGetActiveWindow(VOID)
{
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetActiveWindow\n");
   UserEnterExclusive();

   RETURN( GetHwndSafe(UserGetActiveWindow()));

CLEANUP:
   DPRINT("Leave NtUserGetActiveWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


PWINDOW_OBJECT FASTCALL UserGetActiveWindow(VOID)
{
   PUSER_MESSAGE_QUEUE Queue;
   PWINDOW_OBJECT ActiveWnd;
   
   Queue = UserGetCurrentQueue();

   if (Queue && (ActiveWnd = UserGetWindowObject(Queue->Input->hActiveWindow)))
   {
      return ActiveWnd;
   }
   
   return NULL;
}

HWND STDCALL NtUserSetActiveWindow(HWND hWnd)
{
   DECLARE_RETURN(HWND);
   PWINDOW_OBJECT PrevWnd;

   DPRINT("Enter NtUserSetActiveWindow(%x)\n", hWnd);
   UserEnterExclusive();

   if (hWnd)
   {
      PWINDOW_OBJECT Window;
      PUSER_MESSAGE_QUEUE Queue;

      if (!(Window = UserGetWindowObject(hWnd)))
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         RETURN(0);
      }

      DPRINT("(%wZ)\n", &Window->WindowName);

      Queue = UserGetCurrentQueue();
      
      /* window must be attached to input queue */
      if (Window->Queue->Input != Queue->Input)
      {
         SetLastWin32Error(ERROR_ACCESS_DENIED);
         RETURN(0);
      }
      
      PrevWnd = co_UserSetActiveWindow(Window);
   }
   else
   {
      PrevWnd = co_UserSetActiveWindow(0);
   }

   RETURN( GetHwndSafe(PrevWnd));

CLEANUP:
   DPRINT("Leave NtUserSetActiveWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
HWND STDCALL
NtUserGetCapture(VOID)
{
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetCapture\n");
   UserEnterExclusive();

   RETURN(UserGetCurrentQueue()->Input->hCaptureWindow);

CLEANUP:
   DPRINT("Leave NtUserGetCapture, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
HWND STDCALL
NtUserSetCapture(HWND hWnd)//FIXME: can be NULL?
{
   PUSER_MESSAGE_QUEUE Queue;
   PWINDOW_OBJECT Window;
   HWND hWndPrev;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserSetCapture(%x)\n", hWnd);
   UserEnterExclusive();

   Queue = UserGetCurrentQueue();
   if((Window = UserGetWindowObject(hWnd)))
   {
      if(Window->Queue != Queue)
      {
         RETURN(NULL);
      }
   }
   
   //FIXME: Window can be NULL?
   //FICME: Window is NOT USED!!
   hWndPrev = MsqSetStateWindow(Queue->Input, MSQ_STATE_CAPTURE, hWnd);//FIME_ pass Wnd

   /* also remove other windows if not capturing anymore */
   if(hWnd == NULL)
   {
      MsqSetStateWindow(Queue->Input, MSQ_STATE_MENUOWNER, NULL);
      MsqSetStateWindow(Queue->Input, MSQ_STATE_MOVESIZE, NULL);
   }

   co_UserSendMessage(hWndPrev, WM_CAPTURECHANGED, 0, (LPARAM)hWnd);
   Queue->Input->hCaptureWindow = hWnd;

   RETURN(hWndPrev);

CLEANUP:
   DPRINT("Leave NtUserSetCapture, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
HWND STDCALL
NtUserSetFocus(HWND hWnd)
{
   DECLARE_RETURN(HWND);
   PWINDOW_OBJECT PrevWnd;

   DPRINT("Enter NtUserSetFocus(%x)\n", hWnd);
   UserEnterExclusive();

   if (hWnd)
   {
      PWINDOW_OBJECT Wnd, TopWnd;
      PUSER_MESSAGE_QUEUE Queue;

      if (!(Wnd = UserGetWindowObject(hWnd)))
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         RETURN(0);
      }

      Queue = UserGetCurrentQueue();

      if (Wnd->Style & (WS_MINIMIZE | WS_DISABLED))
      {
         RETURN(Queue->Input->hFocusWindow);
      }

       //FIXME: move up a notch? (right after the handle->object lookup)
      if (Wnd->Queue != Queue)
      {
         SetLastWin32Error(ERROR_ACCESS_DENIED);
         RETURN(0);
      }

      TopWnd = UserGetAncestor(Wnd, GA_ROOT);
      if (TopWnd != UserGetActiveWindow())
      {
         co_UserSetActiveWindow(TopWnd);
      }

      PrevWnd = co_UserSetFocusWindow(Wnd);
   }
   else
   {
      PrevWnd = co_UserSetFocusWindow(NULL);
   }

   RETURN( GetHwndSafe(PrevWnd));

CLEANUP:
   DPRINT("Leave NtUserSetFocus, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


PWINDOW_OBJECT FASTCALL co_UserSetFocus(PWINDOW_OBJECT Wnd OPTIONAL)
{
   if (Wnd)
   {
      PWINDOW_OBJECT WndTop;
      PUSER_MESSAGE_QUEUE Queue = UserGetCurrentQueue();

      if (Wnd->Style & (WS_MINIMIZE | WS_DISABLED))
      {
         return UserGetWindowObject(Queue->Input->hFocusWindow);
      }

      //FIXME: move up a notch (before the stuff above?)?
      if (Wnd->Queue != Queue)
      {
         SetLastWin32Error(ERROR_ACCESS_DENIED);
         return(0);
      }

      WndTop = UserGetAncestor(Wnd, GA_ROOT);
      if (WndTop != UserGetActiveWindow())
      {
         co_UserSetActiveWindow(WndTop); //check retval??
      }

      return co_UserSetFocusWindow(Wnd);
   }

   return co_UserSetFocusWindow(NULL);
}

/* EOF */
