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

/*
Only the interactive WinSta can recieve input. This means
there can exist only one foreground window/queue/thread per
session. But its possibly that this variable should be in
the WinSta anyways. -Gunnar
*/
PUSER_MESSAGE_QUEUE gForegroundQueue = NULL;


/* FUNCTIONS *****************************************************************/


inline PUSER_MESSAGE_QUEUE FASTCALL UserGetForegroundQueue()
{
   return gForegroundQueue;
}

inline VOID FASTCALL UserSetForegroundQueue(PUSER_MESSAGE_QUEUE Queue)
{
   gForegroundQueue = Queue;
}


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
IntSendDeactivateMessages(HWND hWndPrev, HWND hWnd)
{
   if (hWndPrev)
   {
      IntPostOrSendMessage(hWndPrev, WM_NCACTIVATE, FALSE, 0);
      IntPostOrSendMessage(hWndPrev, WM_ACTIVATE,
                           MAKEWPARAM(WA_INACTIVE, UserGetWindowLong(GetWnd(hWndPrev), GWL_STYLE, FALSE) & WS_MINIMIZE),
                           (LPARAM)hWnd);
   }
}

VOID FASTCALL
IntSendActivateMessages(HWND hWndPrev, HWND hWnd, BOOL MouseActivate)
{
   PWINDOW_OBJECT Window, Owner, Parent;

   if (hWnd)
   {
      /* Send palette messages */
      if (IntPostOrSendMessage(hWnd, WM_QUERYNEWPALETTE, 0, 0))
      {
         IntPostOrSendMessage(HWND_BROADCAST, WM_PALETTEISCHANGING,
                              (WPARAM)hWnd, 0);
      }

      if (UserGetWindow(GetWnd(hWnd), GW_HWNDPREV) != NULL)
         WinPosSetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
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

      IntPostOrSendMessage(hWnd, WM_NCACTIVATE, (WPARAM)(Window == UserGetForegroundWindow()), 0);
      /* FIXME: WA_CLICKACTIVE */
      IntPostOrSendMessage(hWnd, WM_ACTIVATE,
                           MAKEWPARAM(MouseActivate ? WA_CLICKACTIVE : WA_ACTIVE,
                                      UserGetWindowLong(GetWnd(hWnd), GWL_STYLE, FALSE) & WS_MINIMIZE),
                           (LPARAM)hWndPrev);
   }
}

VOID FASTCALL
IntSendKillFocusMessages(PWINDOW_OBJECT WndPrev, PWINDOW_OBJECT Wnd)
{
   if (WndPrev)
   {
      IntPostOrSendMessage(GetHwnd(WndPrev), WM_KILLFOCUS, (WPARAM)GetHwnd(Wnd), 0);
   }
}

VOID FASTCALL
IntSendSetFocusMessages(PWINDOW_OBJECT WndPrev, PWINDOW_OBJECT Wnd)
{
   if (Wnd)
   {
      IntPostOrSendMessage(GetHwnd(Wnd), WM_SETFOCUS, (WPARAM)GetHwnd(WndPrev), 0);
   }
}

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
IntSetForegroundAndFocusWindow(PWINDOW_OBJECT Window, PWINDOW_OBJECT FocusWindow, BOOL MouseActivate)
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
   if (PrevForegroundQueue != 0)
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

   IntSendDeactivateMessages(hWndPrev, hWnd);
   IntSendKillFocusMessages(GetWnd(hWndFocusPrev), GetWnd(hWndFocus));

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

   IntSendSetFocusMessages(GetWnd(hWndFocusPrev), GetWnd(hWndFocus));
   IntSendActivateMessages(hWndPrev, hWnd, MouseActivate);

   return TRUE;
}

BOOL FASTCALL
IntSetForegroundWindow(PWINDOW_OBJECT Window)
{
   return IntSetForegroundAndFocusWindow(Window, Window, FALSE);
}

BOOL FASTCALL
IntMouseActivateWindow(PWINDOW_OBJECT Window)
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
            return IntMouseActivateWindow(TopWnd);
         }
      }
      return FALSE;
   }

   TopWnd = UserGetAncestor(Window, GA_ROOT);
   if (TopWnd == NULL)
      TopWnd = UserGetDesktopWindow();

   /* TMN: Check return value from this function? */
   IntSetForegroundAndFocusWindow(TopWnd, Window, TRUE);

   return TRUE;
}

PWINDOW_OBJECT FASTCALL
IntSetActiveWindow(PWINDOW_OBJECT Wnd)
{
   PUSER_MESSAGE_QUEUE Queue;
   PWINDOW_OBJECT WndPrev;

   Queue = UserGetCurrentQueue();

   ASSERT(Queue);

   if (Wnd)
   {
      if (!(Wnd->Style & WS_VISIBLE) || (Wnd->Style & (WS_POPUP | WS_CHILD)) == WS_CHILD)
      {
         //FIXME: commented out meaningless check.  ASSERT(ThreadQueue != 0) above.
         //return ThreadQueue ? 0 : ThreadQueue->ActiveWindow;
         //         if (ThreadQueue)
         //            return 0;
         //         else
         //         {
         //WndPrev = IntGetWindowObject(Queue->Input->hActiveWindow);
         //return( WndPrev ? WndPrev->hSelf : NULL );
         return IntGetWindowObject(Queue->Input->hActiveWindow);

         //         }
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
   IntSendDeactivateMessages(GetHwnd(WndPrev), GetHwnd(Wnd));
   IntSendActivateMessages(GetHwnd(WndPrev), GetHwnd(Wnd), FALSE);

   /* FIXME */
   /*   return IntIsWindow(hWndPrev) ? hWndPrev : 0;*/
   return WndPrev;
}

PWINDOW_OBJECT FASTCALL
UserSetFocusWindow(PWINDOW_OBJECT Wnd)
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

   IntSendKillFocusMessages(GetWnd(hWndPrev), Wnd);
   IntSendSetFocusMessages(GetWnd(hWndPrev), Wnd);

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

      PrevWnd = IntSetActiveWindow(Window);
   }
   else
   {
      PrevWnd = IntSetActiveWindow(0);
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

   IntPostOrSendMessage(hWndPrev, WM_CAPTURECHANGED, 0, (LPARAM)hWnd);
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
         IntSetActiveWindow(WndTop);
      }

      PrevWnd = UserSetFocusWindow(Wnd);
   }
   else
   {
      PrevWnd = UserSetFocusWindow(NULL);
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
UserSetFocus(PWINDOW_OBJECT Wnd OPTIONAL)
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
         IntSetActiveWindow(WndTop);
      }

      return UserSetFocusWindow(Wnd);
   }

   return UserSetFocusWindow(NULL);
}

/* EOF */
