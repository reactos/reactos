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
 * $Id: focus.c,v 1.14 2004/01/27 08:49:58 weiden Exp $
 */

#include <win32k/win32k.h>
#include <include/window.h>
#include <include/desktop.h>
#include <include/focus.h>
#include <include/error.h>
#include <include/winpos.h>
/* #define NDEBUG */
#include <win32k/debug1.h>
#include <debug.h>

HWND FASTCALL
IntGetCaptureWindow()
{
   PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetFocusMessageQueue();
   return ForegroundQueue != NULL ? ForegroundQueue->CaptureWindow : 0;
}

HWND FASTCALL
IntGetFocusWindow()
{
   PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetFocusMessageQueue();
   return ForegroundQueue != NULL ? ForegroundQueue->FocusWindow : 0;
}

HWND FASTCALL
IntGetThreadFocusWindow()
{
   PUSER_MESSAGE_QUEUE ThreadQueue;
   ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
   return ThreadQueue != NULL ? ThreadQueue->FocusWindow : 0;
}

VOID FASTCALL
IntSendDeactivateMessages(HWND hWndPrev, HWND hWnd)
{
   if (hWndPrev)
   {
      IntSendMessage(hWndPrev, WM_NCACTIVATE, FALSE, 0);
      IntSendMessage(hWndPrev, WM_ACTIVATE,
         MAKEWPARAM(WA_INACTIVE, NtUserGetWindowLong(hWndPrev, GWL_STYLE, FALSE) & WS_MINIMIZE),
         (LPARAM)hWnd);
   }
}

VOID FASTCALL
IntSendActivateMessages(HWND hWndPrev, HWND hWnd, BOOL MouseActivate)
{
   if (hWnd)
   {
      /* Send palette messages */
      if (IntSendMessage(hWnd, WM_QUERYNEWPALETTE, 0, 0))
      {
         IntSendMessage(HWND_BROADCAST, WM_PALETTEISCHANGING,
            (WPARAM)hWnd, 0);
      }

      if (NtUserGetWindow(hWnd, GW_HWNDPREV) != NULL)
         WinPosSetWindowPos(hWnd, HWND_TOP, 0, 0, 0, 0,
            SWP_NOSIZE | SWP_NOMOVE | SWP_NOACTIVATE | SWP_NOSENDCHANGING);

      /* FIXME: IntIsWindow */

      IntSendMessage(hWnd, WM_NCACTIVATE, (WPARAM)(hWnd == NtUserGetForegroundWindow()), 0);
      /* FIXME: WA_CLICKACTIVE */
      IntSendMessage(hWnd, WM_ACTIVATE,
         MAKEWPARAM(MouseActivate ? WA_CLICKACTIVE : WA_ACTIVE,
                    NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_MINIMIZE),
         (LPARAM)hWndPrev);
   }
}

VOID FASTCALL
IntSendKillFocusMessages(HWND hWndPrev, HWND hWnd)
{
   if (hWndPrev)
   {
      IntSendMessage(hWndPrev, WM_KILLFOCUS, (WPARAM)hWnd, 0);
   }
}

VOID FASTCALL
IntSendSetFocusMessages(HWND hWndPrev, HWND hWnd)
{
   if (hWnd)
   {
      IntSendMessage(hWnd, WM_SETFOCUS, (WPARAM)hWndPrev, 0);
   }
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

   PrevForegroundQueue = IntGetFocusMessageQueue();
   if (PrevForegroundQueue != 0)
   {
      hWndPrev = PrevForegroundQueue->ActiveWindow;
   }

   if (hWndPrev == hWnd)
   {
      DPRINT("Failed - Same\n");
      return TRUE;
   }

   hWndFocusPrev = (PrevForegroundQueue == FocusWindow->MessageQueue
                    ? FocusWindow->MessageQueue->FocusWindow : NULL);

   /* FIXME: Call hooks. */

   IntSetFocusMessageQueue(Window->MessageQueue);
   ExAcquireFastMutex(&Window->MessageQueue->Lock);
   if (Window->MessageQueue)
   {
      Window->MessageQueue->ActiveWindow = hWnd;
   }
   ExReleaseFastMutex(&Window->MessageQueue->Lock);
   ExAcquireFastMutex(&FocusWindow->MessageQueue->Lock);
   if (FocusWindow->MessageQueue)
   {
      FocusWindow->MessageQueue->FocusWindow = hWndFocus;
   }
   ExReleaseFastMutex(&Window->MessageQueue->Lock);

   IntSendDeactivateMessages(hWndPrev, hWnd);
   IntSendKillFocusMessages(hWndFocusPrev, hWndFocus);
   if (PrevForegroundQueue != Window->MessageQueue)
   {
      /* FIXME: Send WM_ACTIVATEAPP to all thread windows. */
   }
   IntSendSetFocusMessages(hWndFocusPrev, hWndFocus);
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
  HWND Top;
  PWINDOW_OBJECT TopWindow;
  
  if(Window->Style & WS_DISABLED)
  {
    /* FIXME - Activate a modal dialog window if available */
    return FALSE;
  }
  
  Top = NtUserGetAncestor(Window->Self, GA_ROOT);
  if (Top != Window->Self)
    {
      TopWindow = IntGetWindowObject(Top);
    }
  else
    {
      TopWindow = Window;
    }

  IntSetForegroundAndFocusWindow(TopWindow, Window, TRUE);

  if (Top != Window->Self)
    {
      IntReleaseWindowObject(TopWindow);
    }
  return TRUE;
}

HWND FASTCALL
IntSetActiveWindow(PWINDOW_OBJECT Window)
{
   PUSER_MESSAGE_QUEUE ThreadQueue;
   HWND hWndPrev;
   HWND hWnd = 0;

   ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
   ASSERT(ThreadQueue != 0);

   if (Window != 0)
   {
      if (!(Window->Style & WS_VISIBLE) ||
          (Window->Style & (WS_POPUP | WS_CHILD)) == WS_CHILD)
      {
         return ThreadQueue ? 0 : ThreadQueue->ActiveWindow;
      }
      hWnd = Window->Self;
   }

   hWndPrev = ThreadQueue->ActiveWindow;
   if (hWndPrev == hWnd)
   {
      return hWndPrev;
   }

   /* FIXME: Call hooks. */

   ExAcquireFastMutex(&ThreadQueue->Lock);
   ThreadQueue->ActiveWindow = hWnd;
   ExReleaseFastMutex(&ThreadQueue->Lock);

   IntSendDeactivateMessages(hWndPrev, hWnd);
   IntSendActivateMessages(hWndPrev, hWnd, FALSE);

/* FIXME */
/*   return IntIsWindow(hWndPrev) ? hWndPrev : 0;*/
   return hWndPrev;
}

HWND FASTCALL
IntSetFocusWindow(PWINDOW_OBJECT Window)
{
   HWND hWnd = Window != 0 ? Window->Self : 0;
   HWND hWndPrev = 0;
   PUSER_MESSAGE_QUEUE ThreadQueue;

   ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
   ASSERT(ThreadQueue != 0);

   hWndPrev = ThreadQueue->FocusWindow;
   if (hWndPrev == hWnd)
   {
      return hWndPrev;
   }

   ExAcquireFastMutex(&ThreadQueue->Lock);
   ThreadQueue->FocusWindow = hWnd;
   ExReleaseFastMutex(&ThreadQueue->Lock);

   IntSendKillFocusMessages(hWndPrev, hWnd);
   IntSendSetFocusMessages(hWndPrev, hWnd);

   return hWndPrev;
}

/*
 * @implemented
 */
HWND STDCALL
NtUserGetForegroundWindow(VOID)
{
   PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetFocusMessageQueue();
   return ForegroundQueue != NULL ? ForegroundQueue->ActiveWindow : 0;
}

/*
 * @implemented
 */
HWND STDCALL
NtUserGetActiveWindow(VOID)
{
   PUSER_MESSAGE_QUEUE ThreadQueue;
   ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
   return ThreadQueue ? ThreadQueue->ActiveWindow : 0;
}

HWND STDCALL
NtUserSetActiveWindow(HWND hWnd)
{
   DPRINT("NtUserSetActiveWindow(%x)\n", hWnd);

   if (hWnd)
   {
      PWINDOW_OBJECT Window;
      PUSER_MESSAGE_QUEUE ThreadQueue;
      HWND hWndPrev;

      Window = IntGetWindowObject(hWnd);
      if (Window == NULL)
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         return 0;
      }

      DPRINT("(%wZ)\n", &Window->WindowName);

      ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;

      if (Window->MessageQueue != ThreadQueue)
      {
         IntReleaseWindowObject(Window);
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         return 0;
      }

      hWndPrev = IntSetActiveWindow(Window);
      IntReleaseWindowObject(Window);

      return hWndPrev;
   }
   else
   {
      return IntSetActiveWindow(0);
   }
}

/*
 * @implemented
 */
HWND STDCALL
NtUserGetCapture(VOID)
{
   PUSER_MESSAGE_QUEUE ThreadQueue;
   ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
   return ThreadQueue ? ThreadQueue->CaptureWindow : 0;
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

   DPRINT("NtUserSetCapture(%x)\n", hWnd);

   ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
   Window = IntGetWindowObject(hWnd);
   if (Window != 0)
   {
      if (Window->MessageQueue != ThreadQueue)
      {
         IntReleaseWindowObject(Window);
         return 0;
      }
   }
   hWndPrev = ThreadQueue->CaptureWindow;
   IntSendMessage(hWndPrev, WM_CAPTURECHANGED, 0, (LPARAM)hWnd);
   ExAcquireFastMutex(&ThreadQueue->Lock);
   ThreadQueue->CaptureWindow = hWnd;
   ExReleaseFastMutex(&ThreadQueue->Lock);

   return hWndPrev;
}

/*
 * @implemented
 */
HWND STDCALL
NtUserSetFocus(HWND hWnd)
{
   DPRINT("NtUserSetFocus(%x)\n", hWnd);

   if (hWnd)
   {
      PWINDOW_OBJECT Window;
      PUSER_MESSAGE_QUEUE ThreadQueue;
      HWND hWndPrev, hWndTop;

      Window = IntGetWindowObject(hWnd);
      if (Window == NULL)
      {
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         return 0;
      }

      ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;

      if (Window->Style & (WS_MINIMIZE | WS_DISABLED))
      {
         IntReleaseWindowObject(Window);
         return ThreadQueue ? 0 : ThreadQueue->FocusWindow;
      }

      if (Window->MessageQueue != ThreadQueue)
      {
         IntReleaseWindowObject(Window);
         SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
         return 0;
      }

      hWndTop = NtUserGetAncestor(hWnd, GA_ROOT);
      if (hWndTop != NtUserGetActiveWindow())
      {
         NtUserSetActiveWindow(hWndTop);
      }

      hWndPrev = IntSetFocusWindow(Window);
      IntReleaseWindowObject(Window);

      return hWndPrev;
   }
   else
   {
      return IntSetFocusWindow(0);
   }
}

/* EOF */
