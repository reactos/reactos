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

#include <w32k.h>

#define NDEBUG
#include <debug.h>

PWINDOW_OBJECT FASTCALL
UserGetCaptureWindow()
{
   PUSER_MESSAGE_QUEUE ForegroundQueue = UserGetFocusMessageQueue();
   return ForegroundQueue != NULL ? GetWnd(ForegroundQueue->Input->CaptureWindow) : 0;
}

PWINDOW_OBJECT FASTCALL
UserGetFocusWindow()
{
   PUSER_MESSAGE_QUEUE ForegroundQueue = UserGetFocusMessageQueue();
   return ForegroundQueue != NULL ? GetWnd(ForegroundQueue->Input->FocusWindow) : 0;
}

PWINDOW_OBJECT FASTCALL
UserGetThreadFocusWindow()
{
   PUSER_MESSAGE_QUEUE ThreadQueue;
   ThreadQueue = UserGetCurrentQueue();
   return ThreadQueue != NULL ? GetWnd(ThreadQueue->Input->FocusWindow) : 0;
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
      OwnerWnd = IntGetWindowObject(Child->Owner);

      if(!OwnerWnd)
         continue;

      if(OwnerWnd == Owner)
      {
         Ret = Child->Self;
         return Ret;
      }
   }

   return NULL;
}

STATIC BOOL FASTCALL
IntSetForegroundAndFocusWindow(PWINDOW_OBJECT Window, PWINDOW_OBJECT FocusWindow, BOOL MouseActivate)
{
   HWND hWnd = Window->Self;
   HWND hWndPrev = NULL;
   HWND hWndFocus = FocusWindow->Self;
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

   PrevForegroundQueue = UserGetFocusMessageQueue();
   if (PrevForegroundQueue != 0)
   {
      //FIXME uhm... focus er ikke samme som active window... er UserGetFocusMessageQueue riktig?
      hWndPrev = PrevForegroundQueue->Input->ActiveWindow;
   }

   if (hWndPrev == hWnd)
   {
      DPRINT("Failed - Same\n");
      return TRUE;
   }

   hWndFocusPrev = (PrevForegroundQueue == FocusWindow->MessageQueue
                    ? FocusWindow->MessageQueue->Input->FocusWindow : NULL);

   /* FIXME: Call hooks. */

   IntSendDeactivateMessages(hWndPrev, hWnd);
   IntSendKillFocusMessages(GetWnd(hWndFocusPrev), GetWnd(hWndFocus));

   IntSetFocusMessageQueue(Window->MessageQueue);
   if (Window->MessageQueue)
   {
      Window->MessageQueue->Input->ActiveWindow = hWnd;
   }
   if (FocusWindow->MessageQueue)
   {
      FocusWindow->MessageQueue->Input->FocusWindow = hWndFocus;
   }

   if (PrevForegroundQueue != Window->MessageQueue)
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
   PUSER_MESSAGE_QUEUE ThreadQueue;
   PWINDOW_OBJECT WndPrev;

   ThreadQueue = UserGetCurrentQueue();

   ASSERT(ThreadQueue != 0);

   if (Wnd)
   {
      if (!(Wnd->Style & WS_VISIBLE) ||
            (Wnd->Style & (WS_POPUP | WS_CHILD)) == WS_CHILD)
      {
         //FIXME: commented out meaningless check.  ASSERT(ThreadQueue != 0) above.
         //return ThreadQueue ? 0 : ThreadQueue->ActiveWindow;
         //         if (ThreadQueue)
         //            return 0;
         //         else
         //         {
         WndPrev = IntGetWindowObject(ThreadQueue->Input->ActiveWindow);
         return( WndPrev ? WndPrev->Self : NULL );

         //         }
      }
   }

   WndPrev = IntGetWindowObject(ThreadQueue->Input->ActiveWindow);
   if (WndPrev == Wnd)
   {
      return WndPrev;
   }
   //FIXME!!! hva hvis prev active var invalid dvs. NULL? skal vi returnere her nå??


   /* FIXME: Call hooks. */

   ThreadQueue->Input->ActiveWindow = GetHwnd(Wnd);

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
   PUSER_MESSAGE_QUEUE ThreadQueue;

   ThreadQueue = UserGetCurrentQueue();
   ASSERT(ThreadQueue != 0);

   hWndPrev = ThreadQueue->Input->FocusWindow;
   if (hWndPrev == GetHwnd(Wnd))
   {
      return GetWnd(hWndPrev);
   }

   ThreadQueue->Input->FocusWindow = GetHwnd(Wnd);

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
   PUSER_MESSAGE_QUEUE ForegroundQueue = UserGetFocusMessageQueue();

   if (ForegroundQueue && ForegroundQueue->Input->ActiveWindow)
      return IntGetWindowObject(ForegroundQueue->Input->ActiveWindow);

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
   PUSER_MESSAGE_QUEUE ThreadQueue;

   ThreadQueue = UserGetCurrentQueue();
   if (ThreadQueue && ThreadQueue->Input->ActiveWindow)
   {
      PWINDOW_OBJECT ActiveWnd;

      if ((ActiveWnd = IntGetWindowObject(ThreadQueue->Input->ActiveWindow)))
      {
         return ActiveWnd;
      }
      else
      {
         /* active hwnd not valid. set to NULL */
         ThreadQueue->Input->ActiveWindow = NULL;
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
      PUSER_MESSAGE_QUEUE ThreadQueue;

      Window = IntGetWindowObject(hWnd);
      if (Window == NULL)
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         RETURN(0);
      }

      DPRINT("(%wZ)\n", &Window->WindowName);

      ThreadQueue = UserGetCurrentQueue();

      if (Window->MessageQueue != ThreadQueue)
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         RETURN(0);
      }

      PrevWnd = IntSetActiveWindow(Window);
   }
   else
   {
      PrevWnd = IntSetActiveWindow(0);
   }

   RETURN(PrevWnd ? PrevWnd->Self : 0);

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
   PUSER_MESSAGE_QUEUE ThreadQueue;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserGetCapture\n");
   UserEnterExclusive();

   ThreadQueue = UserGetCurrentQueue();//FIXME??? current??
   RETURN(ThreadQueue ? ThreadQueue->Input->CaptureWindow : 0);

CLEANUP:
   DPRINT("Leave NtUserGetCapture, ret=%i\n",_ret_);
   UserLeave();
   END_CLEANUP;
}

/*
 * @implemented
 */
HWND STDCALL
NtUserSetCapture(HWND hWnd)
{
   PUSER_MESSAGE_QUEUE ThreadQueue;
   PWINDOW_OBJECT Window;
   HWND hWndPrev;
   DECLARE_RETURN(HWND);

   DPRINT("Enter NtUserSetCapture(%x)\n", hWnd);
   UserEnterExclusive();

   ThreadQueue = UserGetCurrentQueue();
   if((Window = IntGetWindowObject(hWnd)))
   {
      if(Window->MessageQueue != ThreadQueue)
      {
         RETURN(NULL);
      }
   }
   hWndPrev = MsqSetStateWindow(ThreadQueue->Input, MSQ_STATE_CAPTURE, hWnd);

   /* also remove other windows if not capturing anymore */
   if(hWnd == NULL)
   {
      MsqSetStateWindow(ThreadQueue->Input, MSQ_STATE_MENUOWNER, NULL);
      MsqSetStateWindow(ThreadQueue->Input, MSQ_STATE_MOVESIZE, NULL);
   }

   IntPostOrSendMessage(hWndPrev, WM_CAPTURECHANGED, 0, (LPARAM)hWnd);
   ThreadQueue->Input->CaptureWindow = hWnd;

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
      PUSER_MESSAGE_QUEUE ThreadQueue;

      Wnd = IntGetWindowObject(hWnd);
      if (Wnd == NULL)
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         RETURN(0);
      }

      ThreadQueue = UserGetCurrentQueue();

      if (Wnd->Style & (WS_MINIMIZE | WS_DISABLED))
      {
         RETURN(ThreadQueue ? ThreadQueue->Input->FocusWindow : 0);
      }

      if (Wnd->MessageQueue != ThreadQueue)
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
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

   RETURN(PrevWnd ? PrevWnd->Self : NULL);

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
      PUSER_MESSAGE_QUEUE ThreadQueue;

      ThreadQueue = UserGetCurrentQueue();

      if (Wnd->Style & (WS_MINIMIZE | WS_DISABLED))
      {
         return(ThreadQueue ? GetWnd(ThreadQueue->Input->FocusWindow) : 0);
      }

      if (Wnd->MessageQueue != ThreadQueue)
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
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
