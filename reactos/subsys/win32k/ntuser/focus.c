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
 * $Id: focus.c,v 1.24.4.5 2004/09/27 13:17:12 weiden Exp $
 */
#include <w32k.h>

#define NDEBUG
#include <debug.h>


PWINDOW_OBJECT INTERNAL_CALL
IntGetActiveWindow(VOID)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
  return (ThreadQueue != NULL ? ThreadQueue->ActiveWindow : NULL);
}

PWINDOW_OBJECT INTERNAL_CALL
IntGetCaptureWindow(VOID)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  ThreadQueue = (PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue;
  /* FIXME - return NULL if we're not the active input queue? */
  return (ThreadQueue != NULL ? ThreadQueue->CaptureWindow : NULL);
}

PWINDOW_OBJECT INTERNAL_CALL
IntGetFocusWindow(VOID)
{
  PUSER_MESSAGE_QUEUE ForegroundQueue = IntGetActiveMessageQueue();
  
  if(ForegroundQueue != NULL && ForegroundQueue != PsGetWin32Thread()->MessageQueue)
  {
    /*
     * GetFocus() only returns the handle if the current thread is the active
     * input thread
     */
    return ForegroundQueue->FocusWindow;
  }
  
  return NULL;
}

PWINDOW_OBJECT INTERNAL_CALL
IntGetThreadFocusWindow(VOID)
{
  PUSER_MESSAGE_QUEUE ThreadQueue = PsGetWin32Thread()->MessageQueue;
  return (ThreadQueue != NULL ? ThreadQueue->FocusWindow : NULL);
}

BOOL INTERNAL_CALL
IntSetForegroundWindow(PWINDOW_OBJECT Window)
{
  PDESKTOP_OBJECT pdo;
  PWINDOW_OBJECT PrevActiveWindow;
  PUSER_MESSAGE_QUEUE PrevForegroundQueue;
  
  ASSERT(Window);
  
  /* FIXME - check if the calling process is allowed to set the foreground window.
             See AllowSetForegroundWindow() */
  
  /* FIXME - check if changing the foreground window is locked for the current process.
             See LockSetForegroundWindow() - Is this related to AllowSetForegroundWindow()? */
  
  if((Window->Style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
  {
    DPRINT("Failed - Child\n");
    return FALSE;
  }
  
  if(!(Window->Style & WS_VISIBLE))
  {
    DPRINT("Failed - Invisible\n");
    return FALSE;
  }
  
  /*
   * It's not permitted to change the foreground window if the current input
   * message queue has active menus (2000/XP and later)
   */
  
  pdo = PsGetWin32Thread()->Desktop;
  if((pdo->ActiveMessageQueue != NULL) &&
     (pdo->ActiveMessageQueue->MenuOwner != NULL))
  {
    DPRINT("Can't set foreground window, menus are opened!\n");
    return FALSE;
  }
  
  /* Switch the message queue */
  PrevForegroundQueue = IntSetActiveMessageQueue(Window->MessageQueue);
  
  if(PrevForegroundQueue != NULL && PrevForegroundQueue != Window->MessageQueue)
  {
    /* We have to change the active message queue. Notify the active and focus
       windows of the previous active message queue */
    if(PrevForegroundQueue->ActiveWindow != NULL)
    {
      #if 0
      IntSendDeactivateMessages(PrevForegroundQueue->ActiveWindow, Window);
      #endif
    }
    if(PrevForegroundQueue->FocusWindow != NULL)
    {
      #if 0
      IntSendKillFocusMessages(PrevForegroundQueue->FocusWindow, Window->MessageQueue->FocusWindow);
      #endif
    }
  }
  
  /*
   * We're ready to switch the active window and 
   * send an activation message to the new active window
   */
  
  PrevActiveWindow = MsqSetStateWindow(Window->MessageQueue, MSQ_STATE_ACTIVE, Window);
  if(PrevActiveWindow != NULL && PrevActiveWindow != Window)
  {
    /* Send a deactivation message to the previous active window */
    #if 0
    IntSendDeactivateMessages(PrevActiveWindow, Window);
    #endif
  }
  
  /* FIXME - send a focus message, too? */
  
  return TRUE;
}

STATIC BOOL INTERNAL_CALL
IntSetForegroundAndFocusWindow(PWINDOW_OBJECT Window, PWINDOW_OBJECT FocusWindow, BOOL MouseActivate)
{
  PUSER_MESSAGE_QUEUE PrevForegroundQueue;
  PWINDOW_OBJECT PrevActiveWindow, PrevFocusWindow;
  
  ASSERT(Window);
  ASSERT(FocusWindow);
  
  /* Window and FocusWindow MUST belong to the same thread! */
  ASSERT(Window->MessageQueue == FocusWindow->MessageQueue);
  
  if((Window->Style & (WS_CHILD | WS_POPUP)) == WS_CHILD)
  {
    DPRINT("Failed - Child\n");
    return FALSE;
  }
  
  if(!(Window->Style & WS_VISIBLE))
  {
    DPRINT("Failed - Invisible\n");
    return FALSE;
  }
  
  /*
   * Change the active/focus window in the message queue. This may not be the
   * active message queue but we switch the windows here already.
   */
  
  PrevActiveWindow = MsqSetStateWindow(Window->MessageQueue, MSQ_STATE_ACTIVE, Window);
  if(PrevActiveWindow != NULL && PrevActiveWindow != Window)
  {
    /* Send deactivation message */
    #if 0
    IntSendDeactivateMessages(PrevWindow, Window);
    #endif
  }
  
  PrevFocusWindow = MsqSetStateWindow(Window->MessageQueue, MSQ_STATE_FOCUS, FocusWindow);
  if(PrevFocusWindow != NULL && PrevFocusWindow != FocusWindow)
  {
    /* Send kill focus message */
    #if 0
    IntSendKillFocusMessages(PrevFocusWindow, FocusWindow);
    #endif
  }
  
  /*
   * Switch the desktop's active message queue
   */
  
  PrevForegroundQueue = IntSetActiveMessageQueue(Window->MessageQueue);
  
  /*
   * If we actually changed the active message queue, we have to notify
   * the previous active message queue. We don't send but post messages in this
   * case.
   */
  
  if(PrevForegroundQueue != NULL && PrevForegroundQueue != Window->MessageQueue)
  {
    /* Changed the desktop's foreground message queue, we need to notify the old thread */
    if(PrevForegroundQueue->ActiveWindow != NULL)
    {
      #if 0
      IntSendDeactivateMessages(PrevForegroundQueue->ActiveWindow, Window);
      #endif
    }
    /* we also killed the focus */
    if(PrevForegroundQueue->FocusWindow != NULL)
    {
      #if 0
      IntSendKillFocusMessages(PrevForegroundQueue->FocusWindow, FocusWindow);
      #endif
    }
  }
  
  /*
   * We're ready to notify the windows of our message queue that had been
   * activated.
   */
  
  if(Window != NULL && PrevActiveWindow != Window)
  {
    #if 0
    IntSendActivateMessages(PrevActiveWindow, Window, MouseActivate);
    #endif
  }
  
  if(FocusWindow != NULL && PrevFocusWindow != FocusWindow)
  {
    #if 0
    IntSendSetFocusMessages(PrevFocusWindow, FocusWindow);
    #endif
  }
  
  return TRUE;
}

PWINDOW_OBJECT INTERNAL_CALL
IntFindChildWindowToOwner(PWINDOW_OBJECT Root, PWINDOW_OBJECT Owner)
{
  PWINDOW_OBJECT Child;
  
  ASSERT(Root);
  
  if(Owner != NULL)
  {
    for(Child = Root->FirstChild; Child != NULL; Child = Child->NextSibling)
    {
      if(Child->Owner == Owner)
      {
        return Child;
      }
    }
  }
  
  return NULL;
}

BOOL INTERNAL_CALL
IntMouseActivateWindow(PWINDOW_OBJECT DesktopWindow, PWINDOW_OBJECT Window)
{
  PWINDOW_OBJECT TopWindow;
  
  ASSERT(Window);
  ASSERT(DesktopWindow);
  
  if(Window->Style & WS_DISABLED)
  {
    if((TopWindow = IntFindChildWindowToOwner(DesktopWindow, Window)))
    {
      /* FIXME - do we need some pretection to prevent stack overflows? */
      return IntMouseActivateWindow(DesktopWindow, TopWindow);
    }
    
    return FALSE;
  }
  
  if((TopWindow = IntGetAncestor(Window, GA_ROOT)))
  {
    IntSetForegroundAndFocusWindow(TopWindow, Window, TRUE);
  }
  
  return TRUE;
}

PWINDOW_OBJECT INTERNAL_CALL
IntSetActiveWindow(PWINDOW_OBJECT Window)
{
  PWINDOW_OBJECT PrevWindow;
  PUSER_MESSAGE_QUEUE ThreadQueue;
  
  ThreadQueue = PsGetWin32Thread()->MessageQueue;
  
  if(Window != NULL)
  {
    if(ThreadQueue != Window->MessageQueue)
    {
      /* don't allow to change the active window, if it doesn't belong to the
         calling thread */
      return NULL;
    }
    
    if(!(Window->Style & WS_VISIBLE) ||
       (Window->Style & (WS_POPUP | WS_CHILD)) == WS_CHILD)
    {
      /* FIXME - Is that right? */
      return ThreadQueue->ActiveWindow;
    }
  }
  
  PrevWindow = MsqSetStateWindow(ThreadQueue, MSQ_STATE_ACTIVE, Window);
  
  if(PrevWindow != Window)
  {
    #if 0
    if(PrevWindow != NULL)
    {
      IntSendDeactivateMessages(PrevWindow, Window);
    }
    IntSendActivateMessages(PrevWindow, Window, FALSE);
    #endif
  }
  
  return PrevWindow;
}

PWINDOW_OBJECT INTERNAL_CALL
IntSetFocusWindow(PUSER_MESSAGE_QUEUE WindowMessageQueue, PWINDOW_OBJECT Window)
{
  PWINDOW_OBJECT PrevWindow;
  
  PrevWindow = MsqSetStateWindow(WindowMessageQueue, MSQ_STATE_FOCUS, Window);
  
  #if 0
  if(Window != PrevWindow)
  {
    IntSendKillFocusMessages(PrevWindow, Window);
    IntSendSetFocusMessages(PrevWindow, Window);
  }
  #endif
  
  return PrevWindow;
}

PWINDOW_OBJECT INTERNAL_CALL
IntSetFocus(PWINDOW_OBJECT Window)
{
  PUSER_MESSAGE_QUEUE ThreadQueue = PsGetWin32Thread()->MessageQueue;
  
  if(Window != NULL)
  {
    PWINDOW_OBJECT WindowTop;
    
    if(Window->Style & (WS_MINIMIZE | WS_DISABLED))
    {
      /* FIXME - Is this right? */
      return (ThreadQueue ? ThreadQueue->FocusWindow : NULL);
    }
    
    if(Window->MessageQueue != ThreadQueue)
    {
      /* don't allow to set focus to windows that don't belong to the calling
         thread */
      return NULL;
    }
    
    WindowTop = IntGetAncestor(Window, GA_ROOT);
    if(WindowTop != ThreadQueue->ActiveWindow &&
       WindowTop->MessageQueue == ThreadQueue)
    {
      /* only activate the top level window belongs to the calling thread and
         isn't already active */
      IntSetActiveWindow(WindowTop);
    }
    
    return IntSetFocusWindow(ThreadQueue, Window);
  }
  
  return IntSetFocusWindow(ThreadQueue, NULL);
}

PWINDOW_OBJECT INTERNAL_CALL
IntSetCapture(PWINDOW_OBJECT Window)
{
  PUSER_MESSAGE_QUEUE ThreadQueue;
  
  ThreadQueue = PsGetWin32Thread()->MessageQueue;
  
  if(Window != NULL && (Window->MessageQueue != ThreadQueue))
  {
    /* don't allow to set focus to windows that don't belong to the calling
       thread */
    return NULL;
  }
  
  return MsqSetStateWindow(ThreadQueue, MSQ_STATE_CAPTURE, Window);
}

