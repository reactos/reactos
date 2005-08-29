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
coUserSendDeactivateMessages(HWND hWndPrev, HWND hWnd)
{
   if (hWndPrev)
   {
      coUserSendMessage(hWndPrev, WM_NCACTIVATE, FALSE, 0);
      coUserSendMessage(hWndPrev, WM_ACTIVATE,
                           MAKEWPARAM(WA_INACTIVE, UserGetWindowLong(GetWnd(hWndPrev), GWL_STYLE, FALSE) & WS_MINIMIZE),
                           (LPARAM)hWnd);
   }
}

VOID FASTCALL
coUserSendActivateMessages(HWND hWndPrev, HWND hWnd, BOOL MouseActivate)
{
   PWINDOW_OBJECT Window, Owner, Parent;

   if (hWnd)
   {
      /* Send palette messages */
      if (coUserSendMessage(hWnd, WM_QUERYNEWPALETTE, 0, 0))
      {
         coUserSendMessage(HWND_BROADCAST, WM_PALETTEISCHANGING,
                              (WPARAM)hWnd, 0);
      }

      if (UserGetWindow(GetWnd(hWnd), GW_HWNDPREV) != NULL)
         coWinPosSetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
                            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

      Window = IntGetWindowObject(hWnd);
      if (Window)
      {
         Owner = IntGetOwner(Window);
         if (!Owner)
         {
            Parent = UserGetParent(Window);
            if (!Parent)
               IntShellHookNotify(HSHELL_WINDOWACTIVATED, (LPARAM) hWnd);
         }

      }

      /* FIXME: IntIsWindow */

      coUserSendMessage(hWnd, WM_NCACTIVATE, (WPARAM)(Window == UserGetForegroundWindow()), 0);
      /* FIXME: WA_CLICKACTIVE */
      coUserSendMessage(hWnd, WM_ACTIVATE,
                           MAKEWPARAM(MouseActivate ? WA_CLICKACTIVE : WA_ACTIVE,
                                      UserGetWindowLong(GetWnd(hWnd), GWL_STYLE, FALSE) & WS_MINIMIZE),
                           (LPARAM)hWndPrev);
   }
}

#if 0
VOID FASTCALL
IntSendKillFocusMessages(PWINDOW_OBJECT WndPrev, PWINDOW_OBJECT Wnd)
{
   if (WndPrev)
   {
      coUserPostOrSendMessage(GetHwnd(WndPrev), WM_KILLFOCUS, (WPARAM)GetHwnd(Wnd), 0);
   }
}

VOID FASTCALL
IntSendSetFocusMessages(PWINDOW_OBJECT WndPrev, PWINDOW_OBJECT Wnd)
{
   if (Wnd)
   {
      coUserPostOrSendMessage(GetHwnd(Wnd), WM_SETFOCUS, (WPARAM)GetHwnd(WndPrev), 0);
   }
}
#endif

HWND FASTCALL
IntFindChildWindowToOwner(PWINDOW_OBJECT Root, PWINDOW_OBJECT Owner)
{
   HWND Ret;
   PWINDOW_OBJECT Child, OwnerWnd;

   for(Child = Root->FirstChild; Child; Child = Child->NextSibling)
   {
      OwnerWnd = IntGetWindowObject(Child->hOwner);

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
coUserSetForegroundAndFocusWindow(PWINDOW_OBJECT Window, PWINDOW_OBJECT FocusWindow, BOOL MouseActivate)
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

   coUserSendDeactivateMessages(hWndPrev, hWnd);
   //IntSendKillFocusMessages(GetWnd(hWndFocusPrev), GetWnd(hWndFocus));
   if (hWndFocusPrev)
      coUserSendMessage(hWndFocusPrev, WM_KILLFOCUS, (WPARAM)hWndFocus, 0);

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
      coUserSendMessage(hWndFocus, WM_SETFOCUS, (WPARAM)hWndFocusPrev, 0);
   
   coUserSendActivateMessages(hWndPrev, hWnd, MouseActivate);

   return TRUE;
}

BOOL FASTCALL
coUserSetForegroundWindow(PWINDOW_OBJECT Window)
{
   return coUserSetForegroundAndFocusWindow(Window, Window, FALSE);
}

BOOL FASTCALL
coUserMouseActivateWindow(PWINDOW_OBJECT Window)
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
            //FIXME: recursion in kmode is baaad. Please find another way...
            return coUserMouseActivateWindow(TopWnd);
         }
      }
      return FALSE;
   }

   TopWnd = UserGetAncestor(Window, GA_ROOT);
   if (TopWnd == NULL)
      TopWnd = UserGetDesktopWindow();

   /* TMN: Check return value from this function? */
   coUserSetForegroundAndFocusWindow(TopWnd, Window, TRUE);

   return TRUE;
}

PWINDOW_OBJECT FASTCALL
coUserSetActiveWindow(PWINDOW_OBJECT Wnd /* can be NULL */)
{
   PUSER_MESSAGE_QUEUE Queue;
   PWINDOW_OBJECT WndPrev;

   Queue = UserGetCurrentQueue();

   ASSERT(Queue);

   if (Wnd)
   {
      /* window must be attached to the calling thread input queue (callers of this func must verify )*/ 
      ASSERT(Wnd->Queue->Input == Queue->Input);
      
      if (!(Wnd->Style & WS_VISIBLE) || (Wnd->Style & (WS_POPUP | WS_CHILD)) == WS_CHILD)
      {
         return IntGetWindowObject(Queue->Input->hActiveWindow);
      }
   }

   WndPrev = IntGetWindowObject(Queue->Input->hActiveWindow);
   if (WndPrev == Wnd)
   {
      return WndPrev;
   }
   //FIXME!!! hva hvis prev active var invalid dvs. NULL? skal vi returnere her nå??


   /* FIXME: Call hooks. */

   Queue->Input->hActiveWindow = GetHwnd(Wnd);

   //FIXME!!! hva hvis prev active var invalid dvs. NULL?
   //FIXME!!! hva hvis Wnd er NULL?
   coUserSendDeactivateMessages(GetHwnd(WndPrev), GetHwnd(Wnd));
   coUserSendActivateMessages(GetHwnd(WndPrev), GetHwnd(Wnd), FALSE);

   /* FIXME */
   /*   return IntIsWindow(hWndPrev) ? hWndPrev : 0;*/
   return WndPrev;
}

PWINDOW_OBJECT FASTCALL
coUserSetFocusWindow(PWINDOW_OBJECT Wnd)
{
   HWND hWndPrev = 0;
   PUSER_MESSAGE_QUEUE Queue;

   Queue = UserGetCurrentQueue();

   ASSERT(Queue);

   hWndPrev = Queue->Input->hFocusWindow;
   if (hWndPrev == GetHwnd(Wnd))
   {
      return GetWnd(hWndPrev);
   }

   Queue->Input->hFocusWindow = GetHwnd(Wnd);

   //IntSendKillFocusMessages(GetWnd(hWndPrev), Wnd);
   if (hWndPrev)
      coUserSendMessage(hWndPrev, WM_KILLFOCUS, (WPARAM)GetHwnd(Wnd), 0);


//   IntSendSetFocusMessages(GetWnd(hWndPrev), Wnd);
   if (Wnd)
      coUserSendMessage(GetHwnd(Wnd), WM_SETFOCUS, (WPARAM)hWndPrev, 0);   

   return GetWnd(hWndPrev);
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
   RETURN(GetHwnd(UserGetForegroundWindow()));

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
      return IntGetWindowObject(ForegroundQueue->Input->hActiveWindow);

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

   RETURN(GetHwnd(UserGetActiveWindow()));

CLEANUP:
   DPRINT("Leave NtUserGetActiveWindow, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


PWINDOW_OBJECT FASTCALL
UserGetActiveWindow(VOID)
{
   PUSER_MESSAGE_QUEUE Queue;

   Queue = UserGetCurrentQueue();

   if (Queue && Queue->Input->hActiveWindow)
   {
      PWINDOW_OBJECT ActiveWnd;

      if ((ActiveWnd = IntGetWindowObject(Queue->Input->hActiveWindow)))
      {
         return ActiveWnd;
      }
      else
      {
         /* active hwnd not valid. set to NULL */
         Queue->Input->hActiveWindow = NULL;
      }
   }
   return NULL;
}

HWND STDCALL
NtUserSetActiveWindow(HWND hWnd)
{
   PWINDOW_OBJECT PrevWnd;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserSetActiveWindow(%x)\n", hWnd);
   UserEnterExclusive();

   if (hWnd)
   {
      PWINDOW_OBJECT Window;
      PUSER_MESSAGE_QUEUE Queue;

      Window = IntGetWindowObject(hWnd);
      if (Window == NULL)
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

      PrevWnd = coUserSetActiveWindow(Window);
   }
   else
   {
      PrevWnd = coUserSetActiveWindow(0);
   }

   RETURN(PrevWnd ? PrevWnd->hSelf : 0);

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

   //FIXME: verify hwnd?
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
   if((Window = IntGetWindowObject(hWnd)))
   {
      if(Window->Queue != Queue)
      {
         RETURN(NULL);
      }
   }
   
   //FIXME: Window can be NULL?
   
   hWndPrev = MsqSetStateWindow(Queue->Input, MSQ_STATE_CAPTURE, hWnd);

   /* also remove other windows if not capturing anymore */
   if(hWnd == NULL)
   {
      MsqSetStateWindow(Queue->Input, MSQ_STATE_MENUOWNER, NULL);
      MsqSetStateWindow(Queue->Input, MSQ_STATE_MOVESIZE, NULL);
   }

   coUserSendMessage(hWndPrev, WM_CAPTURECHANGED, 0, (LPARAM)hWnd);
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
   PWINDOW_OBJECT PrevWnd;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserSetFocus(%x)\n", hWnd);
   UserEnterExclusive();

   if (hWnd)
   {
      PWINDOW_OBJECT Wnd, WndTop;
      PUSER_MESSAGE_QUEUE Queue;

      Wnd = IntGetWindowObject(hWnd);
      if (Wnd == NULL)
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

      WndTop = UserGetAncestor(Wnd, GA_ROOT);
      if (WndTop != UserGetActiveWindow())
      {
         coUserSetActiveWindow(WndTop);
      }

      PrevWnd = coUserSetFocusWindow(Wnd);
   }
   else
   {
      PrevWnd = coUserSetFocusWindow(NULL);
   }

   RETURN(PrevWnd ? PrevWnd->hSelf : NULL);

CLEANUP:
   DPRINT("Leave NtUserSetFocus, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}


/*
 * NOTE: Wnd can be NULL
 */
PWINDOW_OBJECT FASTCALL
coUserSetFocus(PWINDOW_OBJECT Wnd OPTIONAL)
{
   if (Wnd)
   {
      PWINDOW_OBJECT WndTop;
      PUSER_MESSAGE_QUEUE Queue;

      Queue = UserGetCurrentQueue();

      if (Wnd->Style & (WS_MINIMIZE | WS_DISABLED))
      {
         return(GetWnd(Queue->Input->hFocusWindow));
      }

      //FIXME: move up a notch?
      if (Wnd->Queue != Queue)
      {
         SetLastWin32Error(ERROR_ACCESS_DENIED);
         return(0);
      }

      WndTop = UserGetAncestor(Wnd, GA_ROOT);
      if (WndTop != UserGetActiveWindow())
      {
         coUserSetActiveWindow(WndTop);
      }

      return coUserSetFocusWindow(Wnd);
   }

   return coUserSetFocusWindow(NULL);
}

/* EOF */
