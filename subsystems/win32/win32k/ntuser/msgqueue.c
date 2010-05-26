/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Message queues
 * FILE:             subsystems/win32/win32k/ntuser/msgqueue.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <win32k.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define SYSTEM_MESSAGE_QUEUE_SIZE           (256)

static MSG SystemMessageQueue[SYSTEM_MESSAGE_QUEUE_SIZE];
static ULONG SystemMessageQueueHead = 0;
static ULONG SystemMessageQueueTail = 0;
static ULONG SystemMessageQueueCount = 0;
static KSPIN_LOCK SystemMessageQueueLock;

static ULONG volatile HardwareMessageQueueStamp = 0;
static LIST_ENTRY HardwareMessageQueueHead;
static KMUTANT HardwareMessageQueueLock;

static KEVENT HardwareMessageEvent;

static PAGED_LOOKASIDE_LIST MessageLookasideList;
static PAGED_LOOKASIDE_LIST TimerLookasideList;

#define IntLockSystemMessageQueue(OldIrql) \
  KeAcquireSpinLock(&SystemMessageQueueLock, &OldIrql)

#define IntUnLockSystemMessageQueue(OldIrql) \
  KeReleaseSpinLock(&SystemMessageQueueLock, OldIrql)

#define IntUnLockSystemHardwareMessageQueueLock(Wait) \
  KeReleaseMutant(&HardwareMessageQueueLock, IO_NO_INCREMENT, FALSE, Wait)

/* FUNCTIONS *****************************************************************/

//
// Wakeup any thread/process waiting on idle input.
//
static VOID FASTCALL
IdlePing(VOID)
{
  HWND hWnd;
  PWINDOW_OBJECT Window;
  PPROCESSINFO W32d = PsGetCurrentProcessWin32Process();

  hWnd = UserGetForegroundWindow();

  Window = UserGetWindowObject(hWnd);

  if (Window && Window->pti)
  {
     if (Window->pti->fsHooks & HOOKID_TO_FLAG(WH_FOREGROUNDIDLE))
     {
        co_HOOK_CallHooks(WH_FOREGROUNDIDLE,HC_ACTION,0,0);
     }
  }

  if (W32d && W32d->InputIdleEvent)
     KePulseEvent( W32d->InputIdleEvent, EVENT_INCREMENT, TRUE);
}

HANDLE FASTCALL
IntMsqSetWakeMask(DWORD WakeMask)
{
   PTHREADINFO Win32Thread;
   PUSER_MESSAGE_QUEUE MessageQueue;
   HANDLE MessageEventHandle;

   Win32Thread = PsGetCurrentThreadWin32Thread();
   if (Win32Thread == NULL || Win32Thread->MessageQueue == NULL)
      return 0;

   MessageQueue = Win32Thread->MessageQueue;
   MessageQueue->WakeMask = WakeMask;
   MessageEventHandle = MessageQueue->NewMessagesHandle;

   return MessageEventHandle;
}

BOOL FASTCALL
IntMsqClearWakeMask(VOID)
{
   PTHREADINFO Win32Thread;
   PUSER_MESSAGE_QUEUE MessageQueue;

   Win32Thread = PsGetCurrentThreadWin32Thread();
   if (Win32Thread == NULL || Win32Thread->MessageQueue == NULL)
      return FALSE;

   MessageQueue = Win32Thread->MessageQueue;
// HACK!!!!!!! Newbies that wrote this should hold your head down in shame! (jt)
   MessageQueue->WakeMask = ~0;

   return TRUE;
}

VOID FASTCALL
MsqIncPaintCountQueue(PUSER_MESSAGE_QUEUE Queue)
{
   Queue->PaintCount++;
   Queue->QueueBits |= QS_PAINT;
   Queue->ChangedBits |= QS_PAINT;
   if (Queue->WakeMask & QS_PAINT)
      KeSetEvent(Queue->NewMessages, IO_NO_INCREMENT, FALSE);
}

VOID FASTCALL
MsqDecPaintCountQueue(PUSER_MESSAGE_QUEUE Queue)
{
   Queue->PaintCount--;
}


NTSTATUS FASTCALL
MsqInitializeImpl(VOID)
{
   /*CurrentFocusMessageQueue = NULL;*/
   InitializeListHead(&HardwareMessageQueueHead);
   KeInitializeEvent(&HardwareMessageEvent, NotificationEvent, 0);
   KeInitializeSpinLock(&SystemMessageQueueLock);
   KeInitializeMutant(&HardwareMessageQueueLock, 0);

   ExInitializePagedLookasideList(&MessageLookasideList,
                                  NULL,
                                  NULL,
                                  0,
                                  sizeof(USER_MESSAGE),
                                  TAG_USRMSG,
                                  256);
   ExInitializePagedLookasideList(&TimerLookasideList,
                                  NULL,
                                  NULL,
                                  0,
                                  sizeof(TIMER_ENTRY),
                                  TAG_TIMER,
                                  64);

   return(STATUS_SUCCESS);
}

VOID FASTCALL
MsqInsertSystemMessage(MSG* Msg)
{
   KIRQL OldIrql;
   ULONG Prev;

   /*
    * If we got WM_MOUSEMOVE and there are already messages in the
    * system message queue, check if the last message is mouse move
    * and if it is then just overwrite it.
    */
   IntLockSystemMessageQueue(OldIrql);

   /*
    * Bail out if the queue is full. FIXME: We should handle this case
    * more gracefully.
    */

   if (SystemMessageQueueCount == SYSTEM_MESSAGE_QUEUE_SIZE)
   {
      IntUnLockSystemMessageQueue(OldIrql);
      return;
   }

   if (Msg->message == WM_MOUSEMOVE && SystemMessageQueueCount)
   {
      if (SystemMessageQueueTail == 0)
         Prev = SYSTEM_MESSAGE_QUEUE_SIZE - 1;
      else
         Prev = SystemMessageQueueTail - 1;
      if (SystemMessageQueue[Prev].message == WM_MOUSEMOVE)
      {
         SystemMessageQueueTail = Prev;
         SystemMessageQueueCount--;
      }
   }

   /*
    * Actually insert the message into the system message queue.
    */

   SystemMessageQueue[SystemMessageQueueTail] = *Msg;
   SystemMessageQueueTail =
      (SystemMessageQueueTail + 1) % SYSTEM_MESSAGE_QUEUE_SIZE;
   SystemMessageQueueCount++;

   IntUnLockSystemMessageQueue(OldIrql);

   KeSetEvent(&HardwareMessageEvent, IO_NO_INCREMENT, FALSE);
}

BOOL FASTCALL
MsqIsClkLck(LPMSG Msg, BOOL Remove)
{
   PTHREADINFO pti;
   PSYSTEM_CURSORINFO CurInfo;
   BOOL Res = FALSE;

   pti = PsGetCurrentThreadWin32Thread();
   if (pti->rpdesk == NULL)
   {
      return FALSE;
   }

   CurInfo = IntGetSysCursorInfo();

   switch (Msg->message)
   {
     case WM_LBUTTONUP:
       Res = ((Msg->time - CurInfo->ClickLockTime) >= gspv.dwMouseClickLockTime);
       if (Res && (!CurInfo->ClickLockActive))
       {
         CurInfo->ClickLockActive = TRUE;
       }
       break;
     case WM_LBUTTONDOWN:
       if (CurInfo->ClickLockActive)
       {
         Res = TRUE;
         CurInfo->ClickLockActive = FALSE;
         CurInfo->ClickLockTime = 0;
       }
       else
       {
         CurInfo->ClickLockTime = Msg->time;
       }
       break;
   }
   return Res;
}

BOOL FASTCALL
MsqIsDblClk(LPMSG Msg, BOOL Remove)
{
   PTHREADINFO pti;
   PSYSTEM_CURSORINFO CurInfo;
   LONG dX, dY;
   BOOL Res;

   pti = PsGetCurrentThreadWin32Thread();
   if (pti->rpdesk == NULL)
   {
      return FALSE;
   }

   CurInfo = IntGetSysCursorInfo();
   Res = (Msg->hwnd == (HWND)CurInfo->LastClkWnd) &&
         ((Msg->time - CurInfo->LastBtnDown) < gspv.iDblClickTime);
   if(Res)
   {

      dX = CurInfo->LastBtnDownX - Msg->pt.x;
      dY = CurInfo->LastBtnDownY - Msg->pt.y;
      if(dX < 0)
         dX = -dX;
      if(dY < 0)
         dY = -dY;

      Res = (dX <= gspv.iDblClickWidth) &&
            (dY <= gspv.iDblClickHeight);

      if(Res)
      {
         if(CurInfo->ButtonsDown)
           Res = (CurInfo->ButtonsDown == Msg->message);
      }
   }

   if(Remove)
   {
      CurInfo->LastBtnDownX = Msg->pt.x;
      CurInfo->LastBtnDownY = Msg->pt.y;
      CurInfo->ButtonsDown = Msg->message;
      if (Res)
      {
         CurInfo->LastBtnDown = 0;
         CurInfo->LastClkWnd = NULL;
      }
      else
      {
         CurInfo->LastClkWnd = (HANDLE)Msg->hwnd;
         CurInfo->LastBtnDown = Msg->time;
      }
   }

   return Res;
}

static BOOL APIENTRY
co_MsqTranslateMouseMessage(PUSER_MESSAGE_QUEUE MessageQueue, PWINDOW_OBJECT Window, UINT FilterLow, UINT FilterHigh,
                            PUSER_MESSAGE Message, BOOL Remove, PBOOL Freed,
                            PWINDOW_OBJECT ScopeWin, PPOINT ScreenPoint, BOOL FromGlobalQueue, PLIST_ENTRY *Next)
{
   USHORT Msg = Message->Msg.message;
   PWINDOW_OBJECT CaptureWindow = NULL;
   HWND hCaptureWin;

   ASSERT_REFS_CO(ScopeWin);

   /*
   co_WinPosWindowFromPoint can return a Window, and in that case
   that window has a ref that we need to deref. Thats why we add "dummy"
   refs in all other cases.
   */

   hCaptureWin = IntGetCaptureWindow();
   if (hCaptureWin == NULL)
   {
      if (Msg == WM_MOUSEWHEEL)
      {
         CaptureWindow = UserGetWindowObject(IntGetFocusWindow());
         if (CaptureWindow) UserReferenceObject(CaptureWindow);
      }
      else
      {
         co_WinPosWindowFromPoint(ScopeWin, NULL, &Message->Msg.pt, &CaptureWindow);
         if(CaptureWindow == NULL)
         {
            CaptureWindow = ScopeWin;
            if (CaptureWindow) UserReferenceObject(CaptureWindow);
         }
         else
         {
            /* this is the one case where we dont add a ref, since the returned
            window is already referenced */
         }
      }
   }
   else
   {
      /* FIXME - window messages should go to the right window if no buttons are
                 pressed */
      CaptureWindow = UserGetWindowObject(hCaptureWin);
      if (CaptureWindow) UserReferenceObject(CaptureWindow);
   }



   if (CaptureWindow == NULL)
   {
      if(!FromGlobalQueue)
      {
         RemoveEntryList(&Message->ListEntry);
         if(MessageQueue->MouseMoveMsg == Message)
         {
            MessageQueue->MouseMoveMsg = NULL;
         }
      }
      // when FromGlobalQueue is true, the caller has already removed the Message
      ExFreePool(Message);
      *Freed = TRUE;
      return(FALSE);
   }

   if (CaptureWindow->pti->MessageQueue != MessageQueue)
   {
      if (! FromGlobalQueue)
      {
         DPRINT("Moving msg between private queues\n");
         /* This message is already queued in a private queue, but we need
          * to move it to a different queue, perhaps because a new window
          * was created which now covers the screen area previously taken
          * by another window. To move it, we need to take it out of the
          * old queue. Note that we're already holding the lock mutexes of the
          * old queue */
         RemoveEntryList(&Message->ListEntry);

         /* remove the pointer for the current WM_MOUSEMOVE message in case we
            just removed it */
         if(MessageQueue->MouseMoveMsg == Message)
         {
            MessageQueue->MouseMoveMsg = NULL;
         }
      }

      /* lock the destination message queue, so we don't get in trouble with other
         threads, messing with it at the same time */
      IntLockHardwareMessageQueue(CaptureWindow->pti->MessageQueue);
      InsertTailList(&CaptureWindow->pti->MessageQueue->HardwareMessagesListHead,
                     &Message->ListEntry);
      if(Message->Msg.message == WM_MOUSEMOVE)
      {
         if(CaptureWindow->pti->MessageQueue->MouseMoveMsg)
         {
            /* remove the old WM_MOUSEMOVE message, we're processing a more recent
               one */
            RemoveEntryList(&CaptureWindow->pti->MessageQueue->MouseMoveMsg->ListEntry);
            ExFreePool(CaptureWindow->pti->MessageQueue->MouseMoveMsg);
         }
         /* save the pointer to the WM_MOUSEMOVE message in the new queue */
         CaptureWindow->pti->MessageQueue->MouseMoveMsg = Message;

         CaptureWindow->pti->MessageQueue->QueueBits |= QS_MOUSEMOVE;
         CaptureWindow->pti->MessageQueue->ChangedBits |= QS_MOUSEMOVE;
         if (CaptureWindow->pti->MessageQueue->WakeMask & QS_MOUSEMOVE)
            KeSetEvent(CaptureWindow->pti->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
      }
      else
      {
         CaptureWindow->pti->MessageQueue->QueueBits |= QS_MOUSEBUTTON;
         CaptureWindow->pti->MessageQueue->ChangedBits |= QS_MOUSEBUTTON;
         if (CaptureWindow->pti->MessageQueue->WakeMask & QS_MOUSEBUTTON)
            KeSetEvent(CaptureWindow->pti->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
      }
      IntUnLockHardwareMessageQueue(CaptureWindow->pti->MessageQueue);

      *Freed = FALSE;
      UserDereferenceObject(CaptureWindow);
      return(FALSE);
   }

   /* From here on, we're in the same message queue as the caller! */

   *ScreenPoint = Message->Msg.pt;

   if((Window != NULL && PtrToInt(Window) != 1 && CaptureWindow->hSelf != Window->hSelf) ||
         ((FilterLow != 0 || FilterHigh != 0) && (Msg < FilterLow || Msg > FilterHigh)))
   {
      /* Reject the message because it doesn't match the filter */

      if(FromGlobalQueue)
      {
         /* Lock the message queue so no other thread can mess with it.
            Our own message queue is not locked while fetching from the global
            queue, so we have to make sure nothing interferes! */
         IntLockHardwareMessageQueue(CaptureWindow->pti->MessageQueue);
         /* if we're from the global queue, we need to add our message to our
            private queue so we don't loose it! */
         InsertTailList(&CaptureWindow->pti->MessageQueue->HardwareMessagesListHead,
                        &Message->ListEntry);
      }

      if (Message->Msg.message == WM_MOUSEMOVE)
      {
         if(CaptureWindow->pti->MessageQueue->MouseMoveMsg &&
               (CaptureWindow->pti->MessageQueue->MouseMoveMsg != Message))
         {
            /* delete the old message */
            RemoveEntryList(&CaptureWindow->pti->MessageQueue->MouseMoveMsg->ListEntry);
            ExFreePool(CaptureWindow->pti->MessageQueue->MouseMoveMsg);
            if (!FromGlobalQueue)
            {
               // We might have deleted the next one in our queue, so fix next
               *Next = Message->ListEntry.Flink;
            }
         }
         /* always save a pointer to this WM_MOUSEMOVE message here because we're
            sure that the message is in the private queue */
         CaptureWindow->pti->MessageQueue->MouseMoveMsg = Message;
      }
      if(FromGlobalQueue)
      {
         IntUnLockHardwareMessageQueue(CaptureWindow->pti->MessageQueue);
      }

      UserDereferenceObject(CaptureWindow);
      *Freed = FALSE;
      return(FALSE);
   }

   /* FIXME - only assign if removing? */
   Message->Msg.hwnd = CaptureWindow->hSelf;
   Message->Msg.message = Msg;
   Message->Msg.lParam = MAKELONG(Message->Msg.pt.x, Message->Msg.pt.y);

   /* remove the reference to the current WM_(NC)MOUSEMOVE message, if this message
      is it */
   if (Message->Msg.message == WM_MOUSEMOVE ||
         Message->Msg.message == WM_NCMOUSEMOVE)
   {
      if(FromGlobalQueue)
      {
         /* Lock the message queue so no other thread can mess with it.
            Our own message queue is not locked while fetching from the global
            queue, so we have to make sure nothing interferes! */
         IntLockHardwareMessageQueue(CaptureWindow->pti->MessageQueue);
         if(CaptureWindow->pti->MessageQueue->MouseMoveMsg)
         {
            /* delete the WM_(NC)MOUSEMOVE message in the private queue, we're dealing
               with one that's been sent later */
            RemoveEntryList(&CaptureWindow->pti->MessageQueue->MouseMoveMsg->ListEntry);
            ExFreePool(CaptureWindow->pti->MessageQueue->MouseMoveMsg);
            /* our message is not in the private queue so we can remove the pointer
               instead of setting it to the current message we're processing */
            CaptureWindow->pti->MessageQueue->MouseMoveMsg = NULL;
         }
         IntUnLockHardwareMessageQueue(CaptureWindow->pti->MessageQueue);
      }
      else if (CaptureWindow->pti->MessageQueue->MouseMoveMsg == Message)
      {
         CaptureWindow->pti->MessageQueue->MouseMoveMsg = NULL;
      }
   }

   UserDereferenceObject(CaptureWindow);
   *Freed = FALSE;
   return(TRUE);
}

BOOL APIENTRY
co_MsqPeekHardwareMessage(PUSER_MESSAGE_QUEUE MessageQueue, PWINDOW_OBJECT Window,
                          UINT FilterLow, UINT FilterHigh, BOOL Remove,
                          PUSER_MESSAGE* Message)
{
   KIRQL OldIrql;
   POINT ScreenPoint;
   BOOL Accept, Freed;
   PLIST_ENTRY CurrentEntry;
   PWINDOW_OBJECT DesktopWindow = NULL;
   PVOID WaitObjects[2];
   NTSTATUS WaitStatus;
   DECLARE_RETURN(BOOL);
   USER_REFERENCE_ENTRY Ref;
   PDESKTOPINFO Desk = NULL;

   WaitObjects[1] = MessageQueue->NewMessages;
   WaitObjects[0] = &HardwareMessageQueueLock;
   do
   {
      IdlePing();

      UserLeaveCo();

      WaitStatus = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, UserRequest,
                                            UserMode, FALSE, NULL, NULL);

      UserEnterCo();
   }
   while (NT_SUCCESS(WaitStatus) && STATUS_WAIT_0 != WaitStatus);

   DesktopWindow = UserGetWindowObject(IntGetDesktopWindow());

   if (DesktopWindow)
   {
       UserRefObjectCo(DesktopWindow, &Ref);//can DesktopWindow be NULL?
       Desk = DesktopWindow->pti->pDeskInfo;
   }

   /* Process messages in the message queue itself. */
   IntLockHardwareMessageQueue(MessageQueue);
   CurrentEntry = MessageQueue->HardwareMessagesListHead.Flink;
   while (CurrentEntry != &MessageQueue->HardwareMessagesListHead)
   {
      PUSER_MESSAGE Current =
         CONTAINING_RECORD(CurrentEntry, USER_MESSAGE, ListEntry);
      CurrentEntry = CurrentEntry->Flink;
      if (Current->Msg.message >= WM_MOUSEFIRST &&
            Current->Msg.message <= WM_MOUSELAST)
      {


         Accept = co_MsqTranslateMouseMessage(MessageQueue, Window, FilterLow, FilterHigh,
                                              Current, Remove, &Freed,
                                              DesktopWindow, &ScreenPoint, FALSE, &CurrentEntry);
         if (Accept)
         {
            if (Remove)
            {
               RemoveEntryList(&Current->ListEntry);
            }
            IntUnLockHardwareMessageQueue(MessageQueue);
            IntUnLockSystemHardwareMessageQueueLock(FALSE);
            *Message = Current;

            if (Desk)
                Desk->LastInputWasKbd = FALSE;

            RETURN(TRUE);
         }

      }
   }
   IntUnLockHardwareMessageQueue(MessageQueue);

   /* Now try the global queue. */

   /* Transfer all messages from the DPC accessible queue to the main queue. */
   IntLockSystemMessageQueue(OldIrql);
   while (SystemMessageQueueCount > 0)
   {
      PUSER_MESSAGE UserMsg;
      MSG Msg;

      ASSERT(SystemMessageQueueHead < SYSTEM_MESSAGE_QUEUE_SIZE);
      Msg = SystemMessageQueue[SystemMessageQueueHead];
      SystemMessageQueueHead =
         (SystemMessageQueueHead + 1) % SYSTEM_MESSAGE_QUEUE_SIZE;
      SystemMessageQueueCount--;
      IntUnLockSystemMessageQueue(OldIrql);

      UserMsg = ExAllocateFromPagedLookasideList(&MessageLookasideList);
      /* What to do if out of memory? For now we just panic a bit in debug */
      ASSERT(UserMsg);
      UserMsg->FreeLParam = FALSE;
      UserMsg->Msg = Msg;
      InsertTailList(&HardwareMessageQueueHead, &UserMsg->ListEntry);

      IntLockSystemMessageQueue(OldIrql);
   }
   HardwareMessageQueueStamp++;
   IntUnLockSystemMessageQueue(OldIrql);

   /* Process messages in the queue until we find one to return. */
   CurrentEntry = HardwareMessageQueueHead.Flink;
   while (CurrentEntry != &HardwareMessageQueueHead)
   {
      PUSER_MESSAGE Current =
         CONTAINING_RECORD(CurrentEntry, USER_MESSAGE, ListEntry);
      CurrentEntry = CurrentEntry->Flink;
      RemoveEntryList(&Current->ListEntry);
      HardwareMessageQueueStamp++;
      if (Current->Msg.message >= WM_MOUSEFIRST &&
            Current->Msg.message <= WM_MOUSELAST)
      {
         const ULONG ActiveStamp = HardwareMessageQueueStamp;
         /* Translate the message. */
         Accept = co_MsqTranslateMouseMessage(MessageQueue, Window, FilterLow, FilterHigh,
                                              Current, Remove, &Freed,
                                              DesktopWindow, &ScreenPoint, TRUE, NULL);
         if (Accept)
         {
            /* Check for no more messages in the system queue. */
            IntLockSystemMessageQueue(OldIrql);
            if (SystemMessageQueueCount == 0 &&
                  IsListEmpty(&HardwareMessageQueueHead))
            {
               KeClearEvent(&HardwareMessageEvent);
            }
            IntUnLockSystemMessageQueue(OldIrql);

            /*
            If we aren't removing the message then add it to the private
            queue.
            */
            if (!Remove)
            {
               IntLockHardwareMessageQueue(MessageQueue);
               if(Current->Msg.message == WM_MOUSEMOVE)
               {
                  if(MessageQueue->MouseMoveMsg)
                  {
                     RemoveEntryList(&MessageQueue->MouseMoveMsg->ListEntry);
                     ExFreePool(MessageQueue->MouseMoveMsg);
                  }
                  MessageQueue->MouseMoveMsg = Current;
               }
               InsertTailList(&MessageQueue->HardwareMessagesListHead,
                              &Current->ListEntry);
               IntUnLockHardwareMessageQueue(MessageQueue);
            }
            IntUnLockSystemHardwareMessageQueueLock(FALSE);
            *Message = Current;

            RETURN(TRUE);
         }
         /* If the contents of the queue changed then restart processing. */
         if (HardwareMessageQueueStamp != ActiveStamp)
         {
            CurrentEntry = HardwareMessageQueueHead.Flink;
            continue;
         }
      }
   }

   /* Check if the system message queue is now empty. */
   IntLockSystemMessageQueue(OldIrql);
   if (SystemMessageQueueCount == 0 && IsListEmpty(&HardwareMessageQueueHead))
   {
      KeClearEvent(&HardwareMessageEvent);
   }
   IntUnLockSystemMessageQueue(OldIrql);
   IntUnLockSystemHardwareMessageQueueLock(FALSE);

   RETURN(FALSE);

CLEANUP:
   if (DesktopWindow) UserDerefObjectCo(DesktopWindow);

   END_CLEANUP;
}

//
// Note: Only called from input.c.
//
VOID FASTCALL
co_MsqPostKeyboardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   PUSER_MESSAGE_QUEUE FocusMessageQueue;
   MSG Msg;
   LARGE_INTEGER LargeTickCount;
   KBDLLHOOKSTRUCT KbdHookData;
   BOOLEAN Entered = FALSE;

   DPRINT("MsqPostKeyboardMessage(uMsg 0x%x, wParam 0x%x, lParam 0x%x)\n",
          uMsg, wParam, lParam);

   // Condition may arise when calling MsqPostMessage and waiting for an event.
   if (!UserIsEntered())
   {
         // Fixme: Not sure ATM if this thread is locked.
         UserEnterExclusive();
         Entered = TRUE;
   }

   FocusMessageQueue = IntGetFocusMessageQueue();

   Msg.hwnd = 0;

   if (FocusMessageQueue && (FocusMessageQueue->FocusWindow != (HWND)0))
       Msg.hwnd = FocusMessageQueue->FocusWindow;

   Msg.message = uMsg;
   Msg.wParam = wParam;
   Msg.lParam = lParam;

   KeQueryTickCount(&LargeTickCount);
   Msg.time = MsqCalculateMessageTime(&LargeTickCount);

   /* We can't get the Msg.pt point here since we don't know thread
      (and thus the window station) the message will end up in yet. */

   KbdHookData.vkCode = Msg.wParam;
   KbdHookData.scanCode = (Msg.lParam >> 16) & 0xff;
   KbdHookData.flags = (0 == (Msg.lParam & 0x01000000) ? 0 : LLKHF_EXTENDED) |
                       (0 == (Msg.lParam & 0x20000000) ? 0 : LLKHF_ALTDOWN) |
                       (0 == (Msg.lParam & 0x80000000) ? 0 : LLKHF_UP);
   KbdHookData.time = Msg.time;
   KbdHookData.dwExtraInfo = 0;
   if (co_HOOK_CallHooks(WH_KEYBOARD_LL, HC_ACTION, Msg.message, (LPARAM) &KbdHookData))
   {
      DPRINT("Kbd msg %d wParam %d lParam 0x%08x dropped by WH_KEYBOARD_LL hook\n",
             Msg.message, Msg.wParam, Msg.lParam);
      if (Entered) UserLeave();
      return;
   }

   if (FocusMessageQueue == NULL)
   {
         DPRINT("No focus message queue\n");
         if (Entered) UserLeave();
         return;
   }

   if (FocusMessageQueue->FocusWindow != (HWND)0)
   {
         Msg.hwnd = FocusMessageQueue->FocusWindow;
         DPRINT("Msg.hwnd = %x\n", Msg.hwnd);

         FocusMessageQueue->Desktop->pDeskInfo->LastInputWasKbd = TRUE;

         Msg.pt = gpsi->ptCursor;
         MsqPostMessage(FocusMessageQueue, &Msg, FALSE, QS_KEY);
   }
   else
   {
         DPRINT("Invalid focus window handle\n");
   }

   if (Entered) UserLeave();
   return;
}

VOID FASTCALL
MsqPostHotKeyMessage(PVOID Thread, HWND hWnd, WPARAM wParam, LPARAM lParam)
{
   PWINDOW_OBJECT Window;
   PTHREADINFO Win32Thread;
   MSG Mesg;
   LARGE_INTEGER LargeTickCount;
   NTSTATUS Status;

   Status = ObReferenceObjectByPointer (Thread,
                                        THREAD_ALL_ACCESS,
                                        PsThreadType,
                                        KernelMode);
   if (!NT_SUCCESS(Status))
      return;

   Win32Thread = ((PETHREAD)Thread)->Tcb.Win32Thread;
   if (Win32Thread == NULL || Win32Thread->MessageQueue == NULL)
   {
      ObDereferenceObject ((PETHREAD)Thread);
      return;
   }

   Window = IntGetWindowObject(hWnd);
   if (!Window)
   {
      ObDereferenceObject ((PETHREAD)Thread);
      return;
   }

   Mesg.hwnd = hWnd;
   Mesg.message = WM_HOTKEY;
   Mesg.wParam = wParam;
   Mesg.lParam = lParam;
   KeQueryTickCount(&LargeTickCount);
   Mesg.time = MsqCalculateMessageTime(&LargeTickCount);
   Mesg.pt = gpsi->ptCursor;
   MsqPostMessage(Window->pti->MessageQueue, &Mesg, FALSE, QS_HOTKEY);
   UserDereferenceObject(Window);
   ObDereferenceObject (Thread);

   //  InsertHeadList(&pThread->MessageQueue->PostedMessagesListHead,
   //   &Message->ListEntry);
   //  KeSetEvent(pThread->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
}

PUSER_MESSAGE FASTCALL
MsqCreateMessage(LPMSG Msg, BOOLEAN FreeLParam)
{
   PUSER_MESSAGE Message;

   Message = ExAllocateFromPagedLookasideList(&MessageLookasideList);
   if (!Message)
   {
      return NULL;
   }

   Message->FreeLParam = FreeLParam;
   RtlMoveMemory(&Message->Msg, Msg, sizeof(MSG));

   return Message;
}

VOID FASTCALL
MsqDestroyMessage(PUSER_MESSAGE Message)
{
   ExFreeToPagedLookasideList(&MessageLookasideList, Message);
}

VOID FASTCALL
co_MsqDispatchSentNotifyMessages(PUSER_MESSAGE_QUEUE MessageQueue)
{
   PLIST_ENTRY ListEntry;
   PUSER_SENT_MESSAGE_NOTIFY Message;

   while (!IsListEmpty(&MessageQueue->SentMessagesListHead))
   {
      ListEntry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
      Message = CONTAINING_RECORD(ListEntry, USER_SENT_MESSAGE_NOTIFY,
                                  ListEntry);

      co_IntCallSentMessageCallback(Message->CompletionCallback,
                                    Message->hWnd,
                                    Message->Msg,
                                    Message->CompletionCallbackContext,
                                    Message->Result);

   }

}

BOOLEAN FASTCALL
MsqPeekSentMessages(PUSER_MESSAGE_QUEUE MessageQueue)
{
   return(!IsListEmpty(&MessageQueue->SentMessagesListHead));
}

BOOLEAN FASTCALL
co_MsqDispatchOneSentMessage(PUSER_MESSAGE_QUEUE MessageQueue)
{
   PUSER_SENT_MESSAGE Message;
   PLIST_ENTRY Entry;
   LRESULT Result;
   BOOL SenderReturned;
   PUSER_SENT_MESSAGE_NOTIFY NotifyMessage;

   if (IsListEmpty(&MessageQueue->SentMessagesListHead))
   {
      return(FALSE);
   }

   /* remove it from the list of pending messages */
   Entry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
   Message = CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry);

   /* insert it to the list of messages that are currently dispatched by this
      message queue */
   InsertTailList(&MessageQueue->LocalDispatchingMessagesHead,
                  &Message->ListEntry);

   if (Message->HookMessage == MSQ_ISHOOK)
   {
      Result = co_HOOK_CallHooks(Message->Msg.message,
                                 (INT)(INT_PTR)Message->Msg.hwnd,
                                 Message->Msg.wParam,
                                 Message->Msg.lParam);
   }
   else if (Message->HookMessage == MSQ_ISEVENT)
   {
      Result = co_EVENT_CallEvents( Message->Msg.message,
                                    Message->Msg.hwnd,
                                    Message->Msg.wParam,
                                    Message->Msg.lParam);                                  
   }
   else
   {
      /* Call the window procedure. */
      Result = co_IntSendMessage(Message->Msg.hwnd,
                                 Message->Msg.message,
                                 Message->Msg.wParam,
                                 Message->Msg.lParam);
   }

   /* remove the message from the local dispatching list, because it doesn't need
      to be cleaned up on thread termination anymore */
   RemoveEntryList(&Message->ListEntry);

   /* remove the message from the dispatching list, so lock the sender's message queue */
   SenderReturned = (Message->DispatchingListEntry.Flink == NULL);
   if (!SenderReturned)
   {
      /* only remove it from the dispatching list if not already removed by a timeout */
      RemoveEntryList(&Message->DispatchingListEntry);
   }
   /* still keep the sender's message queue locked, so the sender can't exit the
      MsqSendMessage() function (if timed out) */

   /* Let the sender know the result. */
   if (Message->Result != NULL)
   {
      *Message->Result = Result;
   }

   if (Message->HasPackedLParam == TRUE)
   {
      if (Message->Msg.lParam)
         ExFreePool((PVOID)Message->Msg.lParam);
   }

   /* Notify the sender. */
   if (Message->CompletionEvent != NULL)
   {
      KeSetEvent(Message->CompletionEvent, IO_NO_INCREMENT, FALSE);
   }

   /* Notify the sender if they specified a callback. */
   if (!SenderReturned && Message->CompletionCallback != NULL)
   {
      if(!(NotifyMessage = ExAllocatePoolWithTag(NonPagedPool,
                           sizeof(USER_SENT_MESSAGE_NOTIFY), TAG_USRMSG)))
      {
         DPRINT1("MsqDispatchOneSentMessage(): Not enough memory to create a callback notify message\n");
         goto Notified;
      }
      NotifyMessage->CompletionCallback =
         Message->CompletionCallback;
      NotifyMessage->CompletionCallbackContext =
         Message->CompletionCallbackContext;
      NotifyMessage->Result = Result;
      NotifyMessage->hWnd = Message->Msg.hwnd;
      NotifyMessage->Msg = Message->Msg.message;
      MsqSendNotifyMessage(Message->SenderQueue, NotifyMessage);
   }

Notified:

   /* Only if it is not a no wait message */
   if (!(Message->HookMessage & MSQ_SENTNOWAIT))
   {
      IntDereferenceMessageQueue(Message->SenderQueue);
      IntDereferenceMessageQueue(MessageQueue);
   }

   /* free the message */
   ExFreePool(Message);
   return(TRUE);
}

VOID APIENTRY
MsqRemoveWindowMessagesFromQueue(PVOID pWindow)
{
   PUSER_SENT_MESSAGE SentMessage;
   PUSER_MESSAGE PostedMessage;
   PUSER_MESSAGE_QUEUE MessageQueue;
   PLIST_ENTRY CurrentEntry, ListHead;
   PWINDOW_OBJECT Window = pWindow;

   ASSERT(Window);

   MessageQueue = Window->pti->MessageQueue;
   ASSERT(MessageQueue);

   /* remove the posted messages for this window */
   CurrentEntry = MessageQueue->PostedMessagesListHead.Flink;
   ListHead = &MessageQueue->PostedMessagesListHead;
   while (CurrentEntry != ListHead)
   {
      PostedMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                        ListEntry);
      if (PostedMessage->Msg.hwnd == Window->hSelf)
      {
         RemoveEntryList(&PostedMessage->ListEntry);
         MsqDestroyMessage(PostedMessage);
         CurrentEntry = MessageQueue->PostedMessagesListHead.Flink;
      }
      else
      {
         CurrentEntry = CurrentEntry->Flink;
      }
   }

   /* remove the sent messages for this window */
   CurrentEntry = MessageQueue->SentMessagesListHead.Flink;
   ListHead = &MessageQueue->SentMessagesListHead;
   while (CurrentEntry != ListHead)
   {
      SentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                      ListEntry);
      if(SentMessage->Msg.hwnd == Window->hSelf)
      {
         DPRINT("Notify the sender and remove a message from the queue that had not been dispatched\n");

         RemoveEntryList(&SentMessage->ListEntry);

         /* remove the message from the dispatching list */
         if(SentMessage->DispatchingListEntry.Flink != NULL)
         {
            RemoveEntryList(&SentMessage->DispatchingListEntry);
         }

         /* wake the sender's thread */
         if (SentMessage->CompletionEvent != NULL)
         {
            KeSetEvent(SentMessage->CompletionEvent, IO_NO_INCREMENT, FALSE);
         }

         if (SentMessage->HasPackedLParam == TRUE)
         {
            if (SentMessage->Msg.lParam)
               ExFreePool((PVOID)SentMessage->Msg.lParam);
         }

         /* Only if it is not a no wait message */
         if (!(SentMessage->HookMessage & MSQ_SENTNOWAIT))
         {
            /* dereference our and the sender's message queue */
            IntDereferenceMessageQueue(MessageQueue);
            IntDereferenceMessageQueue(SentMessage->SenderQueue);
         }

         /* free the message */
         ExFreePool(SentMessage);

         CurrentEntry = MessageQueue->SentMessagesListHead.Flink;
      }
      else
      {
         CurrentEntry = CurrentEntry->Flink;
      }
   }
}

VOID FASTCALL
MsqSendNotifyMessage(PUSER_MESSAGE_QUEUE MessageQueue,
                     PUSER_SENT_MESSAGE_NOTIFY NotifyMessage)
{
   InsertTailList(&MessageQueue->NotifyMessagesListHead,
                  &NotifyMessage->ListEntry);
   MessageQueue->QueueBits |= QS_SENDMESSAGE;
   MessageQueue->ChangedBits |= QS_SENDMESSAGE;
   if (MessageQueue->WakeMask & QS_SENDMESSAGE)
      KeSetEvent(MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
}

NTSTATUS FASTCALL
co_MsqSendMessage(PUSER_MESSAGE_QUEUE MessageQueue,
                  HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam,
                  UINT uTimeout, BOOL Block, INT HookMessage,
                  ULONG_PTR *uResult)
{
   PTHREADINFO pti;
   PUSER_SENT_MESSAGE Message;
   KEVENT CompletionEvent;
   NTSTATUS WaitStatus;
   LRESULT Result;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   LARGE_INTEGER Timeout;
   PLIST_ENTRY Entry;

   if(!(Message = ExAllocatePoolWithTag(PagedPool, sizeof(USER_SENT_MESSAGE), TAG_USRMSG)))
   {
      DPRINT1("MsqSendMessage(): Not enough memory to allocate a message");
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   KeInitializeEvent(&CompletionEvent, NotificationEvent, FALSE);

   pti = PsGetCurrentThreadWin32Thread();
   ThreadQueue = pti->MessageQueue;
   ASSERT(ThreadQueue != MessageQueue);

   Timeout.QuadPart = (LONGLONG) uTimeout * (LONGLONG) -10000;

   /* FIXME - increase reference counter of sender's message queue here */

   Result = 0;
   Message->Msg.hwnd = Wnd;
   Message->Msg.message = Msg;
   Message->Msg.wParam = wParam;
   Message->Msg.lParam = lParam;
   Message->CompletionEvent = &CompletionEvent;
   Message->Result = &Result;
   Message->SenderQueue = ThreadQueue;
   IntReferenceMessageQueue(ThreadQueue);
   Message->CompletionCallback = NULL;
   Message->HookMessage = HookMessage;
   Message->HasPackedLParam = FALSE;

   IntReferenceMessageQueue(MessageQueue);

   /* add it to the list of pending messages */
   InsertTailList(&ThreadQueue->DispatchingMessagesHead, &Message->DispatchingListEntry);

   /* queue it in the destination's message queue */
   InsertTailList(&MessageQueue->SentMessagesListHead, &Message->ListEntry);

   MessageQueue->QueueBits |= QS_SENDMESSAGE;
   MessageQueue->ChangedBits |= QS_SENDMESSAGE;
   if (MessageQueue->WakeMask & QS_SENDMESSAGE)
      KeSetEvent(MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);

   /* we can't access the Message anymore since it could have already been deleted! */

   if(Block)
   {
      IdlePing();

      UserLeaveCo();

      /* don't process messages sent to the thread */
      WaitStatus = KeWaitForSingleObject(&CompletionEvent, UserRequest, UserMode,
                                         FALSE, (uTimeout ? &Timeout : NULL));

      UserEnterCo();

      if(WaitStatus == STATUS_TIMEOUT)
      {
         /* look up if the message has not yet dispatched, if so
            make sure it can't pass a result and it must not set the completion event anymore */
         Entry = MessageQueue->SentMessagesListHead.Flink;
         while (Entry != &MessageQueue->SentMessagesListHead)
         {
            if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry)
                  == Message)
            {
               /* we can access Message here, it's secure because the message queue is locked
                  and the message is still hasn't been dispatched */
               Message->CompletionEvent = NULL;
               Message->Result = NULL;
               break;
            }
            Entry = Entry->Flink;
         }

         /* remove from the local dispatching list so the other thread knows,
            it can't pass a result and it must not set the completion event anymore */
         Entry = ThreadQueue->DispatchingMessagesHead.Flink;
         while (Entry != &ThreadQueue->DispatchingMessagesHead)
         {
            if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, DispatchingListEntry)
                  == Message)
            {
               /* we can access Message here, it's secure because the sender's message is locked
                  and the message has definitely not yet been destroyed, otherwise it would
                  have been removed from this list by the dispatching routine right after
               dispatching the message */
               Message->CompletionEvent = NULL;
               Message->Result = NULL;
               RemoveEntryList(&Message->DispatchingListEntry);
               Message->DispatchingListEntry.Flink = NULL;
               break;
            }
            Entry = Entry->Flink;
         }

         DPRINT("MsqSendMessage (blocked) timed out\n");
      }
      while (co_MsqDispatchOneSentMessage(ThreadQueue))
         ;
   }
   else
   {
      PVOID WaitObjects[2];

      WaitObjects[0] = &CompletionEvent;
      WaitObjects[1] = ThreadQueue->NewMessages;
      do
      {
         IdlePing();

         UserLeaveCo();

         WaitStatus = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, UserRequest,
                                               UserMode, FALSE, (uTimeout ? &Timeout : NULL), NULL);

         UserEnterCo();

         if(WaitStatus == STATUS_TIMEOUT)
         {
            /* look up if the message has not yet been dispatched, if so
               make sure it can't pass a result and it must not set the completion event anymore */
            Entry = MessageQueue->SentMessagesListHead.Flink;
            while (Entry != &MessageQueue->SentMessagesListHead)
            {
               if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry)
                     == Message)
               {
                  /* we can access Message here, it's secure because the message queue is locked
                     and the message is still hasn't been dispatched */
                  Message->CompletionEvent = NULL;
                  Message->Result = NULL;
                  break;
               }
               Entry = Entry->Flink;
            }

            /* remove from the local dispatching list so the other thread knows,
               it can't pass a result and it must not set the completion event anymore */
            Entry = ThreadQueue->DispatchingMessagesHead.Flink;
            while (Entry != &ThreadQueue->DispatchingMessagesHead)
            {
               if ((PUSER_SENT_MESSAGE) CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, DispatchingListEntry)
                     == Message)
               {
                  /* we can access Message here, it's secure because the sender's message is locked
                     and the message has definitely not yet been destroyed, otherwise it would
                     have been removed from this list by the dispatching routine right after
                  dispatching the message */
                  Message->CompletionEvent = NULL;
                  Message->Result = NULL;
                  RemoveEntryList(&Message->DispatchingListEntry);
                  Message->DispatchingListEntry.Flink = NULL;
                  break;
               }
               Entry = Entry->Flink;
            }

            DPRINT("MsqSendMessage timed out\n");
            break;
         }
         while (co_MsqDispatchOneSentMessage(ThreadQueue))
            ;
      }
      while (NT_SUCCESS(WaitStatus) && STATUS_WAIT_0 != WaitStatus);
   }

   if(WaitStatus != STATUS_TIMEOUT)
      *uResult = (STATUS_WAIT_0 == WaitStatus ? Result : -1);

   return WaitStatus;
}

VOID FASTCALL
MsqPostMessage(PUSER_MESSAGE_QUEUE MessageQueue, MSG* Msg, BOOLEAN FreeLParam,
               DWORD MessageBits)
{
   PUSER_MESSAGE Message;

   if(!(Message = MsqCreateMessage(Msg, FreeLParam)))
   {
      return;
   }
   InsertTailList(&MessageQueue->PostedMessagesListHead,
                  &Message->ListEntry);
   MessageQueue->QueueBits |= MessageBits;
   MessageQueue->ChangedBits |= MessageBits;
   if (MessageQueue->WakeMask & MessageBits)
      KeSetEvent(MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
}

VOID FASTCALL
MsqPostQuitMessage(PUSER_MESSAGE_QUEUE MessageQueue, ULONG ExitCode)
{
   MessageQueue->QuitPosted = TRUE;
   MessageQueue->QuitExitCode = ExitCode;
   MessageQueue->QueueBits |= QS_POSTMESSAGE;
   MessageQueue->ChangedBits |= QS_POSTMESSAGE;
   if (MessageQueue->WakeMask & QS_POSTMESSAGE)
      KeSetEvent(MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
}

BOOLEAN APIENTRY
co_MsqFindMessage(IN PUSER_MESSAGE_QUEUE MessageQueue,
                  IN BOOLEAN Hardware,
                  IN BOOLEAN Remove,
                  IN PWINDOW_OBJECT Window,
                  IN UINT MsgFilterLow,
                  IN UINT MsgFilterHigh,
                  OUT PUSER_MESSAGE* Message)
{
   PLIST_ENTRY CurrentEntry;
   PUSER_MESSAGE CurrentMessage;
   PLIST_ENTRY ListHead;

   if (Hardware)
   {
      return(co_MsqPeekHardwareMessage( MessageQueue,
                                        Window,
                                        MsgFilterLow,
                                        MsgFilterHigh,
                                        Remove,
                                        Message));
   }

   CurrentEntry = MessageQueue->PostedMessagesListHead.Flink;
   ListHead = &MessageQueue->PostedMessagesListHead;
   while (CurrentEntry != ListHead)
   {
      CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                         ListEntry);
      if ( ( !Window ||
            PtrToInt(Window) == 1 ||
            Window->hSelf == CurrentMessage->Msg.hwnd ) &&
            ( (MsgFilterLow == 0 && MsgFilterHigh == 0) ||
              ( MsgFilterLow <= CurrentMessage->Msg.message &&
                MsgFilterHigh >= CurrentMessage->Msg.message ) ) )
      {
         if (Remove)
         {
            RemoveEntryList(&CurrentMessage->ListEntry);
         }

         *Message = CurrentMessage;
         return(TRUE);
      }
      CurrentEntry = CurrentEntry->Flink;
   }

   return(FALSE);
}

NTSTATUS FASTCALL
co_MsqWaitForNewMessages(PUSER_MESSAGE_QUEUE MessageQueue, PWINDOW_OBJECT WndFilter,
                         UINT MsgFilterMin, UINT MsgFilterMax)
{
   PVOID WaitObjects[2] = {MessageQueue->NewMessages, &HardwareMessageEvent};
   NTSTATUS ret;

   IdlePing(); // Going to wait so send Idle ping.

   UserLeaveCo();

   ret = KeWaitForMultipleObjects(2,
                                  WaitObjects,
                                  WaitAny,
                                  Executive,
                                  UserMode,
                                  FALSE,
                                  NULL,
                                  NULL);
   UserEnterCo();
   return ret;
}

BOOL FASTCALL
MsqIsHung(PUSER_MESSAGE_QUEUE MessageQueue)
{
   LARGE_INTEGER LargeTickCount;

   KeQueryTickCount(&LargeTickCount);
   return ((LargeTickCount.u.LowPart - MessageQueue->LastMsgRead) > MSQ_HUNG);
}

BOOLEAN FASTCALL
MsqInitializeMessageQueue(struct _ETHREAD *Thread, PUSER_MESSAGE_QUEUE MessageQueue)
{
   LARGE_INTEGER LargeTickCount;
   NTSTATUS Status;

   MessageQueue->Thread = Thread;
   MessageQueue->CaretInfo = (PTHRDCARETINFO)(MessageQueue + 1);
   InitializeListHead(&MessageQueue->PostedMessagesListHead);
   InitializeListHead(&MessageQueue->SentMessagesListHead);
   InitializeListHead(&MessageQueue->HardwareMessagesListHead);
   InitializeListHead(&MessageQueue->TimerListHead);
   InitializeListHead(&MessageQueue->DispatchingMessagesHead);
   InitializeListHead(&MessageQueue->LocalDispatchingMessagesHead);
   KeInitializeMutex(&MessageQueue->HardwareLock, 0);
   MessageQueue->QuitPosted = FALSE;
   MessageQueue->QuitExitCode = 0;
   KeQueryTickCount(&LargeTickCount);
   MessageQueue->LastMsgRead = LargeTickCount.u.LowPart;
   MessageQueue->FocusWindow = NULL;
   MessageQueue->PaintCount = 0;
// HACK!!!!!!! Newbies that wrote this should hold your head down in shame! (jt)
   MessageQueue->WakeMask = ~0;
   MessageQueue->NewMessagesHandle = NULL;

   Status = ZwCreateEvent(&MessageQueue->NewMessagesHandle, EVENT_ALL_ACCESS,
                          NULL, SynchronizationEvent, FALSE);
   if (!NT_SUCCESS(Status))
   {
      return FALSE;
   }

   Status = ObReferenceObjectByHandle(MessageQueue->NewMessagesHandle, 0,
                                      ExEventObjectType, KernelMode,
                                      (PVOID*)&MessageQueue->NewMessages, NULL);
   if (!NT_SUCCESS(Status))
   {
      ZwClose(MessageQueue->NewMessagesHandle);
      MessageQueue->NewMessagesHandle = NULL;
      return FALSE;
   }

   return TRUE;
}

VOID FASTCALL
MsqCleanupMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue)
{
   PLIST_ENTRY CurrentEntry;
   PUSER_MESSAGE CurrentMessage;
   PTIMER_ENTRY CurrentTimer;
   PUSER_SENT_MESSAGE CurrentSentMessage;

   /* cleanup posted messages */
   while (!IsListEmpty(&MessageQueue->PostedMessagesListHead))
   {
      CurrentEntry = RemoveHeadList(&MessageQueue->PostedMessagesListHead);
      CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
                                         ListEntry);
      MsqDestroyMessage(CurrentMessage);
   }

   /* remove the messages that have not yet been dispatched */
   while (!IsListEmpty(&MessageQueue->SentMessagesListHead))
   {
      CurrentEntry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                             ListEntry);

      DPRINT("Notify the sender and remove a message from the queue that had not been dispatched\n");

      /* remove the message from the dispatching list */
      if(CurrentSentMessage->DispatchingListEntry.Flink != NULL)
      {
         RemoveEntryList(&CurrentSentMessage->DispatchingListEntry);
      }

      /* wake the sender's thread */
      if (CurrentSentMessage->CompletionEvent != NULL)
      {
         KeSetEvent(CurrentSentMessage->CompletionEvent, IO_NO_INCREMENT, FALSE);
      }

      if (CurrentSentMessage->HasPackedLParam == TRUE)
      {
         if (CurrentSentMessage->Msg.lParam)
            ExFreePool((PVOID)CurrentSentMessage->Msg.lParam);
      }

      /* Only if it is not a no wait message */
      if (!(CurrentSentMessage->HookMessage & MSQ_SENTNOWAIT))
      {
         /* dereference our and the sender's message queue */
         IntDereferenceMessageQueue(MessageQueue);
         IntDereferenceMessageQueue(CurrentSentMessage->SenderQueue);
      }

      /* free the message */
      ExFreePool(CurrentSentMessage);
   }

   /* cleanup timers */
   while (! IsListEmpty(&MessageQueue->TimerListHead))
   {
      CurrentEntry = RemoveHeadList(&MessageQueue->TimerListHead);
      CurrentTimer = CONTAINING_RECORD(CurrentEntry, TIMER_ENTRY, ListEntry);
      ExFreeToPagedLookasideList(&TimerLookasideList, CurrentTimer);
   }

   /* notify senders of dispatching messages. This needs to be cleaned up if e.g.
      ExitThread() was called in a SendMessage() umode callback */
   while (!IsListEmpty(&MessageQueue->LocalDispatchingMessagesHead))
   {
      CurrentEntry = RemoveHeadList(&MessageQueue->LocalDispatchingMessagesHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                             ListEntry);

      /* remove the message from the dispatching list */
      if(CurrentSentMessage->DispatchingListEntry.Flink != NULL)
      {
         RemoveEntryList(&CurrentSentMessage->DispatchingListEntry);
      }

      DPRINT("Notify the sender, the thread has been terminated while dispatching a message!\n");

      /* wake the sender's thread */
      if (CurrentSentMessage->CompletionEvent != NULL)
      {
         KeSetEvent(CurrentSentMessage->CompletionEvent, IO_NO_INCREMENT, FALSE);
      }

      if (CurrentSentMessage->HasPackedLParam == TRUE)
      {
         if (CurrentSentMessage->Msg.lParam)
            ExFreePool((PVOID)CurrentSentMessage->Msg.lParam);
      }

      /* Only if it is not a no wait message */
      if (!(CurrentSentMessage->HookMessage & MSQ_SENTNOWAIT))
      {
         /* dereference our and the sender's message queue */
         IntDereferenceMessageQueue(MessageQueue);
         IntDereferenceMessageQueue(CurrentSentMessage->SenderQueue);
      }
      /* free the message */
      ExFreePool(CurrentSentMessage);
   }

   /* tell other threads not to bother returning any info to us */
   while (! IsListEmpty(&MessageQueue->DispatchingMessagesHead))
   {
      CurrentEntry = RemoveHeadList(&MessageQueue->DispatchingMessagesHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE,
                                             DispatchingListEntry);
      CurrentSentMessage->CompletionEvent = NULL;
      CurrentSentMessage->Result = NULL;

      /* do NOT dereference our message queue as it might get attempted to be
         locked later */
   }

}

PUSER_MESSAGE_QUEUE FASTCALL
MsqCreateMessageQueue(struct _ETHREAD *Thread)
{
   PUSER_MESSAGE_QUEUE MessageQueue;

   MessageQueue = (PUSER_MESSAGE_QUEUE)ExAllocatePoolWithTag(NonPagedPool,
                  sizeof(USER_MESSAGE_QUEUE) + sizeof(THRDCARETINFO),
                  TAG_MSGQ);

   if (!MessageQueue)
   {
      return NULL;
   }

   RtlZeroMemory(MessageQueue, sizeof(USER_MESSAGE_QUEUE) + sizeof(THRDCARETINFO));
   /* hold at least one reference until it'll be destroyed */
   IntReferenceMessageQueue(MessageQueue);
   /* initialize the queue */
   if (!MsqInitializeMessageQueue(Thread, MessageQueue))
   {
      IntDereferenceMessageQueue(MessageQueue);
      return NULL;
   }

   return MessageQueue;
}

VOID FASTCALL
MsqDestroyMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue)
{
   PDESKTOP desk;

   /* remove the message queue from any desktops */
   if ((desk = InterlockedExchangePointer((PVOID*)&MessageQueue->Desktop, 0)))
   {
      (void)InterlockedExchangePointer((PVOID*)&desk->ActiveMessageQueue, 0);
      IntDereferenceMessageQueue(MessageQueue);
   }

   /* clean it up */
   MsqCleanupMessageQueue(MessageQueue);

   /* decrease the reference counter, if it hits zero, the queue will be freed */
   IntDereferenceMessageQueue(MessageQueue);
}

PHOOKTABLE FASTCALL
MsqGetHooks(PUSER_MESSAGE_QUEUE Queue)
{
   return Queue->Hooks;
}

VOID FASTCALL
MsqSetHooks(PUSER_MESSAGE_QUEUE Queue, PHOOKTABLE Hooks)
{
   Queue->Hooks = Hooks;
}

LPARAM FASTCALL
MsqSetMessageExtraInfo(LPARAM lParam)
{
   LPARAM Ret;
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE MessageQueue;

   pti = PsGetCurrentThreadWin32Thread();
   MessageQueue = pti->MessageQueue;
   if(!MessageQueue)
   {
      return 0;
   }

   Ret = MessageQueue->ExtraInfo;
   MessageQueue->ExtraInfo = lParam;

   return Ret;
}

LPARAM FASTCALL
MsqGetMessageExtraInfo(VOID)
{
   PTHREADINFO pti;
   PUSER_MESSAGE_QUEUE MessageQueue;

   pti = PsGetCurrentThreadWin32Thread();
   MessageQueue = pti->MessageQueue;
   if(!MessageQueue)
   {
      return 0;
   }

   return MessageQueue->ExtraInfo;
}

HWND FASTCALL
MsqSetStateWindow(PUSER_MESSAGE_QUEUE MessageQueue, ULONG Type, HWND hWnd)
{
   HWND Prev;

   switch(Type)
   {
      case MSQ_STATE_CAPTURE:
         Prev = MessageQueue->CaptureWindow;
         MessageQueue->CaptureWindow = hWnd;
         return Prev;
      case MSQ_STATE_ACTIVE:
         Prev = MessageQueue->ActiveWindow;
         MessageQueue->ActiveWindow = hWnd;
         return Prev;
      case MSQ_STATE_FOCUS:
         Prev = MessageQueue->FocusWindow;
         MessageQueue->FocusWindow = hWnd;
         return Prev;
      case MSQ_STATE_MENUOWNER:
         Prev = MessageQueue->MenuOwner;
         MessageQueue->MenuOwner = hWnd;
         return Prev;
      case MSQ_STATE_MOVESIZE:
         Prev = MessageQueue->MoveSize;
         MessageQueue->MoveSize = hWnd;
         return Prev;
      case MSQ_STATE_CARET:
         ASSERT(MessageQueue->CaretInfo);
         Prev = MessageQueue->CaretInfo->hWnd;
         MessageQueue->CaretInfo->hWnd = hWnd;
         return Prev;
   }

   return NULL;
}

#ifndef NDEBUG
static VOID FASTCALL
DumpTimerList(PUSER_MESSAGE_QUEUE MessageQueue)
{
   PLIST_ENTRY Current;
   PTIMER_ENTRY Timer;

   Current = MessageQueue->TimerListHead.Flink;
   if (Current == &MessageQueue->TimerListHead)
   {
      DPRINT("timer list is empty for queue %p\n", MessageQueue);
   }
   while (Current != &MessageQueue->TimerListHead)
   {
      Timer = CONTAINING_RECORD(Current, TIMER_ENTRY, ListEntry);
      DPRINT("queue %p timer %p expiry %I64d wnd %x id %p period %u timerproc %p msg %u\n",
             MessageQueue, Timer, Timer->ExpiryTime.QuadPart, Timer->Wnd, Timer->IDEvent,
             Timer->Period, Timer->TimerFunc, Timer->Msg);
      Current = Current->Flink;
   }
}
#endif /* ! defined(NDEBUG) */

/* Must have the message queue locked while calling this */
static VOID FASTCALL
InsertTimer(PUSER_MESSAGE_QUEUE MessageQueue, PTIMER_ENTRY NewTimer)
{
   PLIST_ENTRY Current;

   Current = MessageQueue->TimerListHead.Flink;
   while (Current != &MessageQueue->TimerListHead)
   {
      if (NewTimer->ExpiryTime.QuadPart <
            CONTAINING_RECORD(Current, TIMER_ENTRY, ListEntry)->ExpiryTime.QuadPart)
      {
         break;
      }
      Current = Current->Flink;
   }

   InsertTailList(Current, &NewTimer->ListEntry);
}

/* Must have the message queue locked while calling this */
static PTIMER_ENTRY FASTCALL
RemoveTimer(PUSER_MESSAGE_QUEUE MessageQueue, HWND Wnd, UINT_PTR IDEvent, UINT Msg)
{
   PTIMER_ENTRY Timer;
   PLIST_ENTRY EnumEntry;

   /* Remove timer if already in the queue */
   EnumEntry = MessageQueue->TimerListHead.Flink;
   while (EnumEntry != &MessageQueue->TimerListHead)
   {
      Timer = CONTAINING_RECORD(EnumEntry, TIMER_ENTRY, ListEntry);
      EnumEntry = EnumEntry->Flink;

      if (Timer->Wnd == Wnd &&
            Timer->IDEvent == IDEvent &&
            Timer->Msg == Msg)
      {
         RemoveEntryList(&Timer->ListEntry);
         return Timer;
      }
   }

   return NULL;
}

BOOLEAN FASTCALL
MsqSetTimer(PUSER_MESSAGE_QUEUE MessageQueue, HWND Wnd,
            UINT_PTR IDEvent, UINT Period, TIMERPROC TimerFunc,
            UINT Msg)
{
   PTIMER_ENTRY Timer;
   LARGE_INTEGER CurrentTime;

   DPRINT("MsqSetTimer queue %p wnd %x id %p period %u timerproc %p msg %d\n",
          MessageQueue, Wnd, IDEvent, Period, TimerFunc, Msg);

   Timer = RemoveTimer(MessageQueue, Wnd, IDEvent, Msg);
   if (NULL == Timer)
   {
      Timer = ExAllocateFromPagedLookasideList(&TimerLookasideList);
      if (NULL == Timer)
      {
         DPRINT1("Failed to allocate timer entry\n");
         return FALSE;
      }
      DPRINT("Allocated new timer entry %p\n", Timer);
      Timer->Wnd = Wnd;
      Timer->IDEvent = IDEvent;
      Timer->Msg = Msg;
   }
   else
   {
      DPRINT("Updating existing timer entry %p\n", Timer);
   }

   KeQuerySystemTime(&CurrentTime);
   Timer->ExpiryTime.QuadPart = CurrentTime.QuadPart +
                                (ULONGLONG) Period * (ULONGLONG) 10000;
   Timer->Period = Period;
   Timer->TimerFunc = TimerFunc;
   DPRINT("Insert timer now %I64d expiry %I64d\n", CurrentTime.QuadPart,
          Timer->ExpiryTime.QuadPart);

   InsertTimer(MessageQueue, Timer);

#ifndef NDEBUG

   DumpTimerList(MessageQueue);
#endif /* ! defined(NDEBUG) */

   return TRUE;
}

BOOLEAN FASTCALL
MsqKillTimer(PUSER_MESSAGE_QUEUE MessageQueue, HWND Wnd,
             UINT_PTR IDEvent, UINT Msg)
{
   PTIMER_ENTRY Timer;

   DPRINT("MsqKillTimer queue %p wnd %x id %p msg %d\n",
          MessageQueue, Wnd, IDEvent, Msg);

   Timer = RemoveTimer(MessageQueue, Wnd, IDEvent, Msg);

   if (NULL == Timer)
   {
      DPRINT("Failed to remove timer from list, not found\n");
   }
   else
   {
      ExFreeToPagedLookasideList(&TimerLookasideList, Timer);
   }

#ifndef NDEBUG
   DumpTimerList(MessageQueue);
#endif /* ! defined(NDEBUG) */

   return NULL != Timer;
}

BOOLEAN FASTCALL
MsqGetTimerMessage(PUSER_MESSAGE_QUEUE MessageQueue,
                   PWINDOW_OBJECT WindowFilter, UINT MsgFilterMin, UINT MsgFilterMax,
                   MSG *Msg, BOOLEAN Restart)
{
   PTIMER_ENTRY Timer;
   LARGE_INTEGER CurrentTime;
   LARGE_INTEGER LargeTickCount;
   PLIST_ENTRY EnumEntry;
   BOOLEAN GotMessage;

   DPRINT("MsqGetTimerMessage queue %p msg %p restart %s\n",
          MessageQueue, Msg, Restart ? "TRUE" : "FALSE");

   KeQuerySystemTime(&CurrentTime);
   DPRINT("Current time %I64d\n", CurrentTime.QuadPart);
   EnumEntry = MessageQueue->TimerListHead.Flink;
   GotMessage = FALSE;
   while (EnumEntry != &MessageQueue->TimerListHead)
   {
      Timer = CONTAINING_RECORD(MessageQueue->TimerListHead.Flink,
                                TIMER_ENTRY, ListEntry);
      DPRINT("Checking timer %p wnd %x expiry %I64d\n", Timer, Timer->Wnd,
             Timer->ExpiryTime.QuadPart);
      EnumEntry = EnumEntry->Flink;
      if ((NULL == WindowFilter || Timer->Wnd == WindowFilter->hSelf) &&
            ((MsgFilterMin == 0 && MsgFilterMax == 0) ||
             (MsgFilterMin <= Timer->Msg &&
              Timer->Msg <= MsgFilterMax)))
      {
         if (Timer->ExpiryTime.QuadPart <= CurrentTime.QuadPart)
         {
            DPRINT("Timer is expired\n");
            GotMessage = TRUE;
            break;
         }
         else
         {
            DPRINT("No need to check later timers\n");
            break;
         }
      }
      else
      {
         DPRINT("timer %p (wnd %x msg %d) failed filter wnd %x msgmin %d msgmax %d\n",
                Timer, Timer->Wnd, Timer->Msg, WindowFilter->hSelf, MsgFilterMin, MsgFilterMax);
      }
   }

   if (! GotMessage)
   {
      DPRINT("No timer pending\n");
      return FALSE;
   }

   Msg->hwnd = Timer->Wnd;
   Msg->message = Timer->Msg;
   Msg->wParam = (WPARAM) Timer->IDEvent;
   Msg->lParam = (LPARAM) Timer->TimerFunc;
   KeQueryTickCount(&LargeTickCount);
   Msg->time = MsqCalculateMessageTime(&LargeTickCount);
   Msg->pt = gpsi->ptCursor;

   if (Restart)
   {
      RemoveEntryList(&Timer->ListEntry);
      Timer->ExpiryTime.QuadPart = CurrentTime.QuadPart +
                                   (ULONGLONG) Timer->Period * (ULONGLONG) 10000;
      DPRINT("Restarting timer %p expires %I64d\n", Timer, Timer->ExpiryTime.QuadPart);
      InsertTimer(MessageQueue, Timer);

#ifndef NDEBUG

      DumpTimerList(MessageQueue);
#endif /* ! defined(NDEBUG) */

   }

   DPRINT("Created message wnd %x msg %d wParam %u lParam %u\n", Msg->hwnd, Msg->message,
          Msg->wParam, Msg->lParam);

   return TRUE;
}

VOID FASTCALL
MsqRemoveTimersWindow(PUSER_MESSAGE_QUEUE MessageQueue, HWND Wnd)
{
   PTIMER_ENTRY Timer;
   PLIST_ENTRY EnumEntry;

   DPRINT("MsqRemoveTimersWindow queue %p wnd %x\n", MessageQueue, Wnd);

   EnumEntry = MessageQueue->TimerListHead.Flink;
   while (EnumEntry != &MessageQueue->TimerListHead)
   {
      Timer = CONTAINING_RECORD(EnumEntry, TIMER_ENTRY, ListEntry);
      EnumEntry = EnumEntry->Flink;
      if (Timer->Wnd == Wnd)
      {
         DPRINT("Removing timer %p because its window is going away\n", Timer);
         RemoveEntryList(&Timer->ListEntry);
         ExFreeToPagedLookasideList(&TimerLookasideList, Timer);
      }
   }

#ifndef NDEBUG
   DumpTimerList(MessageQueue);
#endif /* ! defined(NDEBUG) */

}

BOOLEAN FASTCALL
MsqGetFirstTimerExpiry(PUSER_MESSAGE_QUEUE MessageQueue,
                       PWINDOW_OBJECT WndFilter, UINT MsgFilterMin, UINT MsgFilterMax,
                       PLARGE_INTEGER FirstTimerExpiry)
{
   PTIMER_ENTRY Timer;
   PLIST_ENTRY EnumEntry;

   DPRINT("MsqGetFirstTimerExpiry queue %p wndfilter %x msgfiltermin %d msgfiltermax %d expiry %p\n",
          MessageQueue, WndFilter, MsgFilterMin, MsgFilterMax, FirstTimerExpiry);

   EnumEntry = MessageQueue->TimerListHead.Flink;
   while (EnumEntry != &MessageQueue->TimerListHead)
   {
      Timer = CONTAINING_RECORD(MessageQueue->TimerListHead.Flink,
                                TIMER_ENTRY, ListEntry);
      EnumEntry = EnumEntry->Flink;
      if ((NULL == WndFilter || PtrToInt(WndFilter) == 1 || Timer->Wnd == WndFilter->hSelf) &&
            ((MsgFilterMin == 0 && MsgFilterMax == 0) ||
             (MsgFilterMin <= Timer->Msg &&
              Timer->Msg <= MsgFilterMax)))
      {
         *FirstTimerExpiry = Timer->ExpiryTime;
         DPRINT("First timer expires %I64d\n", Timer->ExpiryTime);
         return TRUE;
      }
   }

   return FALSE;
}

/* EOF */
