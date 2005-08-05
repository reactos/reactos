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
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id$
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Message queues
 * FILE:             subsys/win32k/ntuser/msgqueue.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <w32k.h>

//#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

#define SYSTEM_MESSAGE_QUEUE_SIZE           (256)

static MSG SystemMessageQueue[SYSTEM_MESSAGE_QUEUE_SIZE];
static ULONG SystemMessageQueueHead = 0;
static ULONG SystemMessageQueueTail = 0;
static ULONG SystemMessageQueueCount = 0;
static ULONG SystemMessageQueueMouseMove = -1;
static KSPIN_LOCK SystemMessageQueueLock;

static ULONG volatile HardwareMessageQueueStamp = 0;
static LIST_ENTRY HardwareMessageQueueHead;
static KMUTEX HardwareMessageQueueLock;

static KEVENT HardwareMessageEvent;

static PAGED_LOOKASIDE_LIST MessageLookasideList;


#define IntLockSystemMessageQueue(OldIrql) \
  KeAcquireSpinLock(&SystemMessageQueueLock, &OldIrql)

#define IntUnLockSystemMessageQueue(OldIrql) \
  KeReleaseSpinLock(&SystemMessageQueueLock, OldIrql)

#define IntUnLockSystemHardwareMessageQueueLock(Wait) \
  KeReleaseMutex(&HardwareMessageQueueLock, Wait)

/* FUNCTIONS *****************************************************************/

void cp(char* f, int l)
{
   DPRINT1("%s, %i\n",f,l);
}

HANDLE FASTCALL
IntMsqSetWakeMask(DWORD WakeMask) //FIXME: take queue as param?
{
   PUSER_MESSAGE_QUEUE MessageQueue;
   HANDLE MessageEventHandle;

   MessageQueue = UserGetCurrentQueue();

   MessageQueue->WakeMask = WakeMask;
   MessageEventHandle = MessageQueue->NewMessagesHandle;

   return MessageEventHandle;
}

BOOL FASTCALL
IntMsqClearWakeMask(VOID) //FIXME: take queue as param?
{
   PUSER_MESSAGE_QUEUE MessageQueue;

   MessageQueue = UserGetCurrentQueue();
   MessageQueue->WakeMask = ~0;

   return TRUE;
}

/* check the queue status */
inline BOOL
MsqIsQueueSignaled(PUSER_MESSAGE_QUEUE Queue)
{
   return ((Queue->WakeBits & Queue->WakeMask) || (Queue->ChangedBits & Queue->ChangedMask));
}

/* set some queue bits */
inline VOID
MsqSetQueueBits(PUSER_MESSAGE_QUEUE Queue, WORD Bits )
{
   ASSERT(Queue);
   ASSERT(Queue->NewMessages);
   
   DPRINT1("MsqSetQueueBits, queue %x\n", Queue);
   
   Queue->WakeBits |= Bits;
   Queue->ChangedBits |= Bits;
   
   if (MsqIsQueueSignaled(Queue))
      KeSetEvent(Queue->NewMessages, IO_NO_INCREMENT, FALSE);
}

/* clear some queue bits */
inline VOID
MsqClearQueueBits(PUSER_MESSAGE_QUEUE Queue, WORD Bits )
{
   Queue->WakeBits &= ~Bits;
   Queue->ChangedBits &= ~Bits;
}

VOID FASTCALL
MsqIncPaintCountQueue(PUSER_MESSAGE_QUEUE Queue)
{
   Queue->PaintCount++;
   //FIXME: PaintPosted  ust go away. QS_PAINT is used instead!
   Queue->PaintPosted = TRUE;
   MsqSetQueueBits(Queue, QS_PAINT);
}

VOID FASTCALL
MsqDecPaintCountQueue(PUSER_MESSAGE_QUEUE Queue)
{
   Queue->PaintCount--;

   /* wine checks for this... */
   if (Queue->PaintCount < 0)
      Queue->PaintCount = 0;

   if (Queue->PaintCount == 0)
   {
      //FIXME: PaintPosted  ust go away. QS_PAINT is used instead!
      Queue->PaintPosted = FALSE;
      MsqClearQueueBits(Queue, QS_PAINT);
   }
}


NTSTATUS FASTCALL
MsqInitializeImpl(VOID)
{
   /*CurrentFocusMessageQueue = NULL;*/
   InitializeListHead(&HardwareMessageQueueHead);
   KeInitializeEvent(&HardwareMessageEvent, NotificationEvent, 0);
   KeInitializeSpinLock(&SystemMessageQueueLock);
   KeInitializeMutex(&HardwareMessageQueueLock, 0);

   ExInitializePagedLookasideList(&MessageLookasideList,
                                  NULL,
                                  NULL,
                                  0,
                                  sizeof(USER_MESSAGE),
                                  0,
                                  256);

   return(STATUS_SUCCESS);
}

VOID FASTCALL
MsqInsertSystemMessage(MSG* Msg)
{
   LARGE_INTEGER LargeTickCount;
   KIRQL OldIrql;
   ULONG mmov = (ULONG)-1;

   KeQueryTickCount(&LargeTickCount);
   Msg->time = LargeTickCount.u.LowPart;

   IntLockSystemMessageQueue(OldIrql);

   /* only insert WM_MOUSEMOVE messages if not already in system message queue */
   if(Msg->message == WM_MOUSEMOVE)
      mmov = SystemMessageQueueMouseMove;

   if(mmov != (ULONG)-1)
   {
      /* insert message at the queue head */
      while (mmov != SystemMessageQueueHead )
      {
         ULONG prev = mmov ? mmov - 1 : SYSTEM_MESSAGE_QUEUE_SIZE - 1;
         ASSERT((LONG) mmov >= 0);
         ASSERT(mmov < SYSTEM_MESSAGE_QUEUE_SIZE);
         SystemMessageQueue[mmov] = SystemMessageQueue[prev];
         mmov = prev;
      }
      SystemMessageQueue[SystemMessageQueueHead] = *Msg;
   }
   else
   {
      if (SystemMessageQueueCount == SYSTEM_MESSAGE_QUEUE_SIZE)
      {
         IntUnLockSystemMessageQueue(OldIrql);
         return;
      }
      SystemMessageQueue[SystemMessageQueueTail] = *Msg;
      if(Msg->message == WM_MOUSEMOVE)
         SystemMessageQueueMouseMove = SystemMessageQueueTail;
      SystemMessageQueueTail =
         (SystemMessageQueueTail + 1) % SYSTEM_MESSAGE_QUEUE_SIZE;
      SystemMessageQueueCount++;
   }
   IntUnLockSystemMessageQueue(OldIrql);

   /* wakes all waiters  FIXME: wake all? why??*/
   KeSetEvent(&HardwareMessageEvent, IO_NO_INCREMENT, FALSE);
}

BOOL FASTCALL
MsqIsDblClk(LPMSG Msg, BOOL Remove)
{
   PWINSTATION_OBJECT WinStaObject;
   PSYSTEM_CURSORINFO CurInfo;
   LONG dX, dY;
   BOOL Res;

   if (PsGetWin32Thread()->Desktop == NULL)
   {
      return FALSE;
   }

   WinStaObject = UserGetCurrentWinSta();

   CurInfo = UserGetSysCursorInfo(WinStaObject);
   Res = (Msg->hwnd == (HWND)CurInfo->LastClkWnd) &&
         ((Msg->time - CurInfo->LastBtnDown) < CurInfo->DblClickSpeed);
   if(Res)
   {

      dX = CurInfo->LastBtnDownX - Msg->pt.x;
      dY = CurInfo->LastBtnDownY - Msg->pt.y;
      if(dX < 0)
         dX = -dX;
      if(dY < 0)
         dY = -dY;

      Res = (dX <= CurInfo->DblClickWidth) &&
            (dY <= CurInfo->DblClickHeight);
   }

   if(Remove)
   {
      if (Res)
      {
         CurInfo->LastBtnDown = 0;
         CurInfo->LastBtnDownX = Msg->pt.x;
         CurInfo->LastBtnDownY = Msg->pt.y;
         CurInfo->LastClkWnd = NULL;
      }
      else
      {
         CurInfo->LastBtnDownX = Msg->pt.x;
         CurInfo->LastBtnDownY = Msg->pt.y;
         CurInfo->LastClkWnd = (HANDLE)Msg->hwnd;
         CurInfo->LastBtnDown = Msg->time;
      }
   }

   return Res;
}

BOOL STATIC STDCALL
MsqTranslateMouseMessage(PUSER_MESSAGE_QUEUE MessageQueue, HWND hWnd, UINT FilterLow, UINT FilterHigh,
                         PUSER_MESSAGE Message, BOOL Remove, PBOOL Freed,
                         PWINDOW_OBJECT ScopeWin, PPOINT ScreenPoint, BOOL FromGlobalQueue)
{
   USHORT Msg = Message->Msg.message;
   PWINDOW_OBJECT Window = NULL;
   //  HWND CaptureWin;

   Window = UserGetCaptureWindow();
   if (Window == NULL)
   {
      if(Msg == WM_MOUSEWHEEL)
      {
         Window = UserGetFocusWindow();
      }
      else
      {
         WinPosWindowFromPoint(ScopeWin, NULL, &Message->Msg.pt, &Window);
         if(Window == NULL)
         {
            Window = ScopeWin;
         }
      }
   }
   //  else
   //7  {
   /* FIXME - window messages should go to the right window if no buttons are
              pressed */
   //    Window = IntGetWindowObject(CaptureWin);
   //  }

   if (Window == NULL)
   {
      if(!FromGlobalQueue)
      {
         RemoveEntryList(&Message->ListEntry);
         if(MessageQueue->MouseMoveMsg == Message)
         {
            MessageQueue->MouseMoveMsg = NULL;
         }
      }
      ExFreePool(Message);
      *Freed = TRUE;
      return(FALSE);
   }

   if (Window->MessageQueue != MessageQueue)
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
      IntLockHardwareMessageQueue(Window->MessageQueue);
      InsertTailList(&Window->MessageQueue->HardwareMessagesListHead,
                     &Message->ListEntry);
      if(Message->Msg.message == WM_MOUSEMOVE)
      {
         if(Window->MessageQueue->MouseMoveMsg)
         {
            /* remove the old WM_MOUSEMOVE message, we're processing a more recent
               one */
            RemoveEntryList(&Window->MessageQueue->MouseMoveMsg->ListEntry);
            ExFreePool(Window->MessageQueue->MouseMoveMsg);
         }
         /* save the pointer to the WM_MOUSEMOVE message in the new queue */
         Window->MessageQueue->MouseMoveMsg = Message;

         MsqSetQueueBits(Window->MessageQueue, QS_MOUSEMOVE);
      }
      else
      {
         MsqSetQueueBits(Window->MessageQueue, QS_MOUSEBUTTON);
      }
      IntUnLockHardwareMessageQueue(Window->MessageQueue);

      *Freed = FALSE;
      return(FALSE);
   }

   /* From here on, we're in the same message queue as the caller! */

   *ScreenPoint = Message->Msg.pt;

   if((hWnd != NULL && Window->Self != hWnd) ||
         ((FilterLow != 0 || FilterLow != 0) && (Msg < FilterLow || Msg > FilterHigh)))
   {
      /* Reject the message because it doesn't match the filter */

      if(FromGlobalQueue)
      {
         /* Lock the message queue so no other thread can mess with it.
            Our own message queue is not locked while fetching from the global
            queue, so we have to make sure nothing interferes! */
         IntLockHardwareMessageQueue(Window->MessageQueue);
         /* if we're from the global queue, we need to add our message to our
            private queue so we don't loose it! */
         InsertTailList(&Window->MessageQueue->HardwareMessagesListHead,
                        &Message->ListEntry);
      }

      if (Message->Msg.message == WM_MOUSEMOVE)
      {
         if(Window->MessageQueue->MouseMoveMsg &&
               (Window->MessageQueue->MouseMoveMsg != Message))
         {
            /* delete the old message */
            RemoveEntryList(&Window->MessageQueue->MouseMoveMsg->ListEntry);
            ExFreePool(Window->MessageQueue->MouseMoveMsg);
         }
         /* always save a pointer to this WM_MOUSEMOVE message here because we're
            sure that the message is in the private queue */
         Window->MessageQueue->MouseMoveMsg = Message;
      }
      if(FromGlobalQueue)
      {
         IntUnLockHardwareMessageQueue(Window->MessageQueue);
      }

      *Freed = FALSE;
      return(FALSE);
   }

   /* FIXME - only assign if removing? */
   Message->Msg.hwnd = Window->Self;
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
         IntLockHardwareMessageQueue(Window->MessageQueue);
         if(Window->MessageQueue->MouseMoveMsg)
         {
            /* delete the WM_(NC)MOUSEMOVE message in the private queue, we're dealing
               with one that's been sent later */
            RemoveEntryList(&Window->MessageQueue->MouseMoveMsg->ListEntry);
            ExFreePool(Window->MessageQueue->MouseMoveMsg);
            /* our message is not in the private queue so we can remove the pointer
               instead of setting it to the current message we're processing */
            Window->MessageQueue->MouseMoveMsg = NULL;
         }
         IntUnLockHardwareMessageQueue(Window->MessageQueue);
      }
      else if(Window->MessageQueue->MouseMoveMsg == Message)
      {
         Window->MessageQueue->MouseMoveMsg = NULL;
      }
   }

   *Freed = FALSE;
   return(TRUE);
}

BOOL STDCALL
MsqPeekHardwareMessage(
   PUSER_MESSAGE_QUEUE MessageQueue,
   HWND hWnd,
   UINT FilterLow,
   UINT FilterHigh,
   BOOL Remove,
   PUSER_MESSAGE* Message
)
{
   KIRQL OldIrql;
   POINT ScreenPoint;
   BOOL Accept, Freed;
   PLIST_ENTRY CurrentEntry;
   PWINDOW_OBJECT DesktopWindow;
   PVOID WaitObjects[2];
   NTSTATUS WaitStatus;
   PUSER_MESSAGE Current;

   ASSERT(MessageQueue);
   ASSERT(Message);


   if( !IntGetScreenDC() || UserGetCurrentQueue() == W32kGetPrimitiveMessageQueue() )
   {
      return FALSE;
   }
   WaitObjects[1] = MessageQueue->NewMessages;
   WaitObjects[0] = &HardwareMessageQueueLock;
   do
   {
      UserLeave();

      WaitStatus = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, UserRequest,
                                            UserMode, FALSE, NULL, NULL);

      UserEnterExclusive();

      while (MsqDispatchOneSentMessage(MessageQueue))
      {
      }
   }
   while (NT_SUCCESS(WaitStatus) && STATUS_WAIT_0 != WaitStatus);

   DesktopWindow = UserGetDesktopWindow();

   /* Process messages in the message queue itself. */
   IntLockHardwareMessageQueue(MessageQueue);
   
   LIST_FOR_EACH_SAFE(CurrentEntry, &MessageQueue->HardwareMessagesListHead, Current, USER_MESSAGE, ListEntry)
   {
      if (Current->Msg.message >= WM_MOUSEFIRST &&
            Current->Msg.message <= WM_MOUSELAST)
      {
         Accept = MsqTranslateMouseMessage(MessageQueue, hWnd, FilterLow, FilterHigh,
                                           Current, Remove, &Freed,
                                           DesktopWindow, &ScreenPoint, FALSE);
         if (Accept)
         {
            if (Remove)
            {
               RemoveEntryList(&Current->ListEntry);
            }
            IntUnLockHardwareMessageQueue(MessageQueue);
            IntUnLockSystemHardwareMessageQueueLock(FALSE);
            *Message = Current;

            return(TRUE);
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
      BOOL ProcessMessage;



      ASSERT(SystemMessageQueueHead < SYSTEM_MESSAGE_QUEUE_SIZE);
      Msg = SystemMessageQueue[SystemMessageQueueHead];
      SystemMessageQueueHead =
         (SystemMessageQueueHead + 1) % SYSTEM_MESSAGE_QUEUE_SIZE;
      SystemMessageQueueCount--;
      IntUnLockSystemMessageQueue(OldIrql);
      if (WM_MOUSEFIRST <= Msg.message && Msg.message <= WM_MOUSELAST)
      {
         MSLLHOOKSTRUCT MouseHookData;

         MouseHookData.pt.x = LOWORD(Msg.lParam);
         MouseHookData.pt.y = HIWORD(Msg.lParam);
         switch(Msg.message)
         {
            case WM_MOUSEWHEEL:
               MouseHookData.mouseData = MAKELONG(0, GET_WHEEL_DELTA_WPARAM(Msg.wParam));
               break;
            case WM_XBUTTONDOWN:
            case WM_XBUTTONUP:
            case WM_XBUTTONDBLCLK:
            case WM_NCXBUTTONDOWN:
            case WM_NCXBUTTONUP:
            case WM_NCXBUTTONDBLCLK:
               MouseHookData.mouseData = MAKELONG(0, HIWORD(Msg.wParam));
               break;
            default:
               MouseHookData.mouseData = 0;
               break;
         }
         MouseHookData.flags = 0;
         MouseHookData.time = Msg.time;
         MouseHookData.dwExtraInfo = 0;
         ProcessMessage = (0 == HOOK_CallHooks(WH_MOUSE_LL, HC_ACTION,
                                               Msg.message, (LPARAM) &MouseHookData));
      }
      else
      {
         ProcessMessage = TRUE;
      }
      if (ProcessMessage)
      {
         UserMsg = ExAllocateFromPagedLookasideList(&MessageLookasideList);
         /* What to do if out of memory? For now we just panic a bit in debug */
         ASSERT(UserMsg);
         UserMsg->FreeLParam = FALSE;
         UserMsg->Msg = Msg;
         InsertTailList(&HardwareMessageQueueHead, &UserMsg->ListEntry);
      }
      IntLockSystemMessageQueue(OldIrql);
   }
   /*
    * we could set this to -1 conditionally if we find one, but
    * this is more efficient and just as effective.
    */
   SystemMessageQueueMouseMove = -1;
   HardwareMessageQueueStamp++;
   IntUnLockSystemMessageQueue(OldIrql);

   /* Process messages in the queue until we find one to return. */
   CurrentEntry = HardwareMessageQueueHead.Flink;
   while (CurrentEntry != &HardwareMessageQueueHead)
   {
      PUSER_MESSAGE Current =
         CONTAINING_RECORD(CurrentEntry, USER_MESSAGE, ListEntry);
      CurrentEntry = CurrentEntry->Flink;
      
      //FIXME: unconditionally remove ALL messages???????????????
      RemoveEntryList(&Current->ListEntry);
      
      
      HardwareMessageQueueStamp++;
      if (Current->Msg.message >= WM_MOUSEFIRST &&
            Current->Msg.message <= WM_MOUSELAST)
      {
         const ULONG ActiveStamp = HardwareMessageQueueStamp;
         /* Translate the message. */
         Accept = MsqTranslateMouseMessage(MessageQueue, hWnd, FilterLow, FilterHigh,
                                           Current, Remove, &Freed,
                                           DesktopWindow, &ScreenPoint, TRUE);
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

            return(TRUE);
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

   return(FALSE);
}

VOID FASTCALL
MsqPostKeyboardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
   PUSER_MESSAGE_QUEUE FocusMessageQueue;
   MSG Msg;
   LARGE_INTEGER LargeTickCount;
   KBDLLHOOKSTRUCT KbdHookData;

   DPRINT("MsqPostKeyboardMessage(uMsg 0x%x, wParam 0x%x, lParam 0x%x)\n",
          uMsg, wParam, lParam);

   Msg.hwnd = 0;
   Msg.message = uMsg;
   Msg.wParam = wParam;
   Msg.lParam = lParam;

   KeQueryTickCount(&LargeTickCount);
   Msg.time = LargeTickCount.u.LowPart;
   /* We can't get the Msg.pt point here since we don't know thread
      (and thus the window station) the message will end up in yet. */

   KbdHookData.vkCode = Msg.wParam;
   KbdHookData.scanCode = (Msg.lParam >> 16) & 0xff;
   KbdHookData.flags = (0 == (Msg.lParam & 0x01000000) ? 0 : LLKHF_EXTENDED) |
                       (0 == (Msg.lParam & 0x20000000) ? 0 : LLKHF_ALTDOWN) |
                       (0 == (Msg.lParam & 0x80000000) ? 0 : LLKHF_UP);
   KbdHookData.time = Msg.time;
   KbdHookData.dwExtraInfo = 0;
   if (HOOK_CallHooks(WH_KEYBOARD_LL, HC_ACTION, Msg.message, (LPARAM) &KbdHookData))
   {
      DPRINT("Kbd msg %d wParam %d lParam 0x%08x dropped by WH_KEYBOARD_LL hook\n",
             Msg.message, Msg.wParam, Msg.lParam);
      return;
   }

   FocusMessageQueue = UserGetFocusMessageQueue();

   /*
    * FIXME: whats the point of this call???? -- Gunnar
    *
    * There's a dedicated thread in CSRSS that processes input messages for
    * consoles and it's message queue is marked as "primitive message queue".
    * We can assume that if there is no screen DC then we're in console mode
    * and the keyboard messages should go to this queue.
    *
    * This behaviour should eventually be removed.
    *
    * -- Filip
    */
   if( !IntGetScreenDC() )
   {
      /* FIXME: What to do about Msg.pt here? */
      if( W32kGetPrimitiveMessageQueue() )
      {
         MsqPostMessage(W32kGetPrimitiveMessageQueue(), &Msg, FALSE, QS_KEY);
      }
   }
   else
   {
      if (FocusMessageQueue == NULL)
      {
         DPRINT("No focus message queue\n");
         return;
      }

      if (FocusMessageQueue->Input->FocusWindow != (HWND)0)
      {
         Msg.hwnd = FocusMessageQueue->Input->FocusWindow;
//         DPRINT("Msg.hwnd = %x\n", Msg.hwnd);
//         DPRINT("FocusMessageQueue %x\n", FocusMessageQueue);
//         DPRINT("FocusMessageQueue->Desktop %x\n", FocusMessageQueue->Desktop);
//         DPRINT("FocusMessageQueue->Desktop->WindowStation %x\n", FocusMessageQueue->Desktop->WindowStation);
         //FIXME: fikk crash her , inval mem 0x0000001c
         UserGetCursorLocation(FocusMessageQueue->Desktop->WindowStation, &Msg.pt);
         MsqPostMessage(FocusMessageQueue, &Msg, FALSE, QS_KEY);
      }
      else
      {
         DPRINT("Invalid focus window handle\n");
      }
   }
}

VOID FASTCALL
MsqPostHotKeyMessage(PVOID Thread, HWND hWnd, WPARAM wParam, LPARAM lParam)
{
   PWINDOW_OBJECT Window;
   PW32THREAD Win32Thread;
   PWINSTATION_OBJECT WinSta;
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

//   WinSta = Win32Thread->Desktop->WindowStation;
//   Status = ObmReferenceObjectByHandle(WinSta->HandleTable,
//                                       hWnd, otWindow, (PVOID*)&Window);


//   if (!NT_SUCCESS(Status))
//   {
//      ObDereferenceObject ((PETHREAD)Thread);
//      return;
//   }

   if (!(Window = IntGetWindowObject(hWnd)))
   {
      ObDereferenceObject ((PETHREAD)Thread);
      return;
   }

   Mesg.hwnd = hWnd;
   Mesg.message = WM_HOTKEY;
   Mesg.wParam = wParam;
   Mesg.lParam = lParam;
   KeQueryTickCount(&LargeTickCount);
   Mesg.time = LargeTickCount.u.LowPart;
   UserGetCursorLocation(WinSta, &Mesg.pt);
   
   MsqPostMessage(Window->MessageQueue, &Mesg, FALSE, QS_HOTKEY);

   ObDereferenceObject (Thread);

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
MsqDispatchSentNotifyMessages(PUSER_MESSAGE_QUEUE MessageQueue)
{
   PLIST_ENTRY ListEntry;
   PUSER_SENT_MESSAGE_NOTIFY Message;

   while (!IsListEmpty(&MessageQueue->SentMessagesListHead))
   {
      ListEntry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
      Message = CONTAINING_RECORD(ListEntry, USER_SENT_MESSAGE_NOTIFY,
                                  ListEntry);

      IntCallSentMessageCallback(Message->CompletionCallback,
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
MsqDispatchOneSentMessage(PUSER_MESSAGE_QUEUE MessageQueue)
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
   
   DPRINT1("Queue 0x%x\n",MessageQueue);

   /* remove it from the list of pending messages */
   Entry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
   Message = CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry);

   /* insert it to the list of messages that are currently dispatched by this
      message queue */
   InsertTailList(&MessageQueue->LocalDispatchingMessagesHead,
                  &Message->ListEntry);

   if (Message->HookMessage)
   {
      Result = HOOK_CallHooks(Message->Msg.message,
                              (INT) Message->Msg.hwnd,
                              Message->Msg.wParam,
                              Message->Msg.lParam);
   }
   else
   {
      /* Call the window procedure. */
      Result = IntSendMessage(Message->Msg.hwnd,
                              Message->Msg.message,
                              Message->Msg.wParam,
                              Message->Msg.lParam);
   }

   /* remove the message from the local dispatching list, because it doesn't need
      to be cleaned up on thread termination anymore */
   RemoveEntryList(&Message->ListEntry);

   /* remove the message from the dispatching list, so lock the sender's message queue */

   SenderReturned = (Message->DispatchingListEntry.Flink == NULL);
   if(!SenderReturned)
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
      NotifyMessage->CompletionCallback = Message->CompletionCallback;
      NotifyMessage->CompletionCallbackContext = Message->CompletionCallbackContext;
      NotifyMessage->Result = Result;
      NotifyMessage->hWnd = Message->Msg.hwnd;
      NotifyMessage->Msg = Message->Msg.message;
      MsqSendNotifyMessage(Message->SenderQueue, NotifyMessage);
   }

Notified:

   /* dereference both sender and our queue */
//   IntDereferenceMessageQueue(MessageQueue);
//   IntDereferenceMessageQueue(Message->SenderQueue);

   /* free the message */
   ExFreePool(Message);
   return(TRUE);
}


VOID FASTCALL
MsqCleanupWindow(PWINDOW_OBJECT Window)
{
   PUSER_SENT_MESSAGE SentMessage;
   PUSER_MESSAGE PostedMessage;
   PUSER_MESSAGE_QUEUE MessageQueue;
   PLIST_ENTRY TmpEntry;
//   PWINDOW_OBJECT Window = pWindow;

   ASSERT(Window);

   MessageQueue = Window->MessageQueue;
   ASSERT(MessageQueue);
   
   
   /* before or after queue cleanup ? */
   UserRemoveTimersWindow(Window);


   /* remove the posted messages for this window */
   LIST_FOR_EACH_SAFE(TmpEntry, &MessageQueue->PostedMessagesListHead, PostedMessage, USER_MESSAGE, ListEntry)
   {
      if (PostedMessage->Msg.hwnd == Window->Self)
      {
         RemoveEntryList(&PostedMessage->ListEntry);
         MsqDestroyMessage(PostedMessage);
      }
   }

   /* remove the sent messages for this window */
   LIST_FOR_EACH_SAFE(TmpEntry, &MessageQueue->SentMessagesListHead, SentMessage, USER_SENT_MESSAGE, ListEntry)
   {
      if(SentMessage->Msg.hwnd == Window->Self)
      {
         DPRINT("Notify the sender and remove a message from the queue that had not been dispatched\n");

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

         /* dereference our and the sender's message queue */
//         IntDereferenceMessageQueue(MessageQueue);
//         IntDereferenceMessageQueue(SentMessage->SenderQueue);

         /* free the message */
         ExFreePool(SentMessage);
      }
   }
}


VOID FASTCALL
MsqSendNotifyMessage(PUSER_MESSAGE_QUEUE MessageQueue,
                     PUSER_SENT_MESSAGE_NOTIFY NotifyMessage)
{
   InsertTailList(&MessageQueue->NotifyMessagesListHead,
                  &NotifyMessage->ListEntry);

   MsqSetQueueBits(MessageQueue, QS_SENDMESSAGE);
}


NTSTATUS FASTCALL
MsqSendMessage(
   PUSER_MESSAGE_QUEUE MessageQueue, //target queue?
   //FIXME: take a MSG instead of all these params?
   HWND Wnd, 
   UINT Msg, 
   WPARAM wParam, 
   LPARAM lParam,
   UINT uTimeout, 
   BOOL Block, //Block = should NOT process/dispath sent messages while waiting for answer
   BOOL HookMessage,
   ULONG_PTR *uResult)
{
   PUSER_SENT_MESSAGE Message;
   KEVENT CompletionEvent;
   NTSTATUS WaitStatus;
   LRESULT Result;
   PUSER_MESSAGE_QUEUE ThreadQueue;
   LARGE_INTEGER Timeout;
   PUSER_SENT_MESSAGE SentMsg;
   PLIST_ENTRY Entry;

   if(!(Message = ExAllocatePoolWithTag(PagedPool, sizeof(USER_SENT_MESSAGE), TAG_USRMSG)))
   {
      DPRINT1("MsqSendMessage(): Not enough memory to allocate a message");
      return STATUS_INSUFFICIENT_RESOURCES;
   }

   //init event here? overkill! KeClearEvent etc.. sounds more correct
   KeInitializeEvent(&CompletionEvent, NotificationEvent, FALSE);

   ThreadQueue = UserGetCurrentQueue();
   
   ASSERT(ThreadQueue != MessageQueue);

   Timeout.QuadPart = (LONGLONG) uTimeout * (LONGLONG) -10000;

   /* FIXME - increase reference counter of sender's message queue here */

   /* shouldnt need referencing here. the message is known to both sender and reciever queue,
      and the message will be cleaned up when either of them die */
      
   Result = 0;
   Message->Msg.hwnd = Wnd;
   Message->Msg.message = Msg;
   Message->Msg.wParam = wParam;
   Message->Msg.lParam = lParam;
   Message->CompletionEvent = &CompletionEvent;
   Message->Result = &Result;
   Message->SenderQueue = ThreadQueue;
//   IntReferenceMessageQueue(ThreadQueue);
   Message->CompletionCallback = NULL;
   Message->HookMessage = HookMessage;

//   IntReferenceMessageQueue(MessageQueue);

   /* add it to the list of pending messages */
   InsertTailList(&ThreadQueue->DispatchingMessagesHead, &Message->DispatchingListEntry);

   /* queue it in the destination's message queue */
   InsertTailList(&MessageQueue->SentMessagesListHead, &Message->ListEntry);

   MsqSetQueueBits(MessageQueue, QS_SENDMESSAGE);

   /* we can't access the Message anymore since it could have already been deleted! */

   if(Block)
   {

      UserLeave();

      /* don't process messages sent to the thread */
      WaitStatus = KeWaitForSingleObject(&CompletionEvent, UserRequest, UserMode,
                                         FALSE, (uTimeout ? &Timeout : NULL));

      UserEnterExclusive();

      if(WaitStatus == STATUS_TIMEOUT)
      {
         /* look up if the message has not yet dispatched, if so
            make sure it can't pass a result and it must not set the completion event anymore */
         
         
         
         LIST_FOR_EACH_SAFE(Entry, &MessageQueue->SentMessagesListHead, SentMsg, USER_SENT_MESSAGE, ListEntry)
         {
            if (SentMsg == Message)
            {
               /* we can access Message here, it's secure because the message queue is locked
                  and the message is still hasn't been dispatched */
               Message->CompletionEvent = NULL;
               Message->Result = NULL;
               break;
            }
         }

         /* remove from the local dispatching list so the other thread knows,
            it can't pass a result and it must not set the completion event anymore */
         LIST_FOR_EACH_SAFE(Entry, &ThreadQueue->DispatchingMessagesHead, SentMsg, USER_SENT_MESSAGE, ListEntry)
         {
            if (SentMsg == Message)
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
         }

         DPRINT("MsqSendMessage (blocked) timed out\n");
      }
      while (MsqDispatchOneSentMessage(ThreadQueue));
   }
   else
   {
      PVOID WaitObjects[2];

      WaitObjects[0] = &CompletionEvent;
      WaitObjects[1] = ThreadQueue->NewMessages;
      do
      {

         UserLeave();

         //FIXME: wait smarter (wake only if sent messages in queue)
         WaitStatus = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, UserRequest,
                                               UserMode, FALSE, (uTimeout ? &Timeout : NULL), NULL);

         UserEnterExclusive();

         if(WaitStatus == STATUS_TIMEOUT)
         {
            /* look up if the message has not yet been dispatched, if so
               make sure it can't pass a result and it must not set the completion event anymore */
            LIST_FOR_EACH_SAFE(Entry, &MessageQueue->SentMessagesListHead, SentMsg, USER_SENT_MESSAGE, ListEntry)
            {
               if (SentMsg == Message)
               {
                  /* we can access Message here, it's secure because the message queue is locked
                     and the message is still hasn't been dispatched */
                  Message->CompletionEvent = NULL;
                  Message->Result = NULL;
                  break;
               }
            }

            /* remove from the local dispatching list so the other thread knows,
               it can't pass a result and it must not set the completion event anymore */
            LIST_FOR_EACH_SAFE(Entry, &ThreadQueue->DispatchingMessagesHead, SentMsg, USER_SENT_MESSAGE, ListEntry)               
            {
               if (SentMsg == Message)
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
            }

            DPRINT("MsqSendMessage timed out\n");
            break;
         }
         
         while (MsqDispatchOneSentMessage(ThreadQueue));
      }
      while (NT_SUCCESS(WaitStatus) && STATUS_WAIT_0 != WaitStatus);
   }

   if(WaitStatus != STATUS_TIMEOUT)
      *uResult = (STATUS_WAIT_0 == WaitStatus ? Result : -1);

   return WaitStatus;
}

VOID FASTCALL
MsqPostMessage(
   PUSER_MESSAGE_QUEUE TargetQueue,
   MSG* Msg,
   BOOLEAN FreeLParam,
   DWORD MessageBits
)
{
   PUSER_MESSAGE Message;

   if(!(Message = MsqCreateMessage(Msg, FreeLParam)))
   {
      return;
   }
   
   InsertTailList(&TargetQueue->PostedMessagesListHead, &Message->ListEntry);
   MsqSetQueueBits(TargetQueue, MessageBits);
   
   //FIXME: reference queue. no. we just give the message away and it has no link to us
}


VOID FASTCALL
MsqPostQuitMessage(PUSER_MESSAGE_QUEUE MessageQueue, ULONG ExitCode)
{
   MessageQueue->QuitPosted = TRUE;
   MessageQueue->QuitExitCode = ExitCode;

   MsqSetQueueBits(MessageQueue, QS_POSTMESSAGE);
}

BOOLEAN STDCALL
MsqFindMessage(
   IN PUSER_MESSAGE_QUEUE MessageQueue,
   IN BOOLEAN Hardware,
   IN BOOLEAN Remove,
   IN HWND Wnd,
   IN UINT MsgFilterLow,
   IN UINT MsgFilterHigh,
   OUT PUSER_MESSAGE* Message)
{
   PLIST_ENTRY EnumEntry;
   PUSER_MESSAGE CurrentMessage;

   ASSERT(MessageQueue);
   ASSERT(Message);


   if (Hardware)
   {
      return(MsqPeekHardwareMessage(MessageQueue, Wnd,
                                    MsgFilterLow, MsgFilterHigh,
                                    Remove, Message));
   }

   LIST_FOR_EACH_SAFE(EnumEntry, &MessageQueue->PostedMessagesListHead, CurrentMessage, USER_MESSAGE, ListEntry)
   {
      if ((Wnd == 0 || Wnd == CurrentMessage->Msg.hwnd) && 
       UserMessageFilter(CurrentMessage->Msg.message, MsgFilterLow, MsgFilterHigh))
      {
         if (Remove)
         {
            RemoveEntryList(&CurrentMessage->ListEntry);
         }
         *Message = CurrentMessage;
         return(TRUE);
      }
   }
   
   return(FALSE);
}

NTSTATUS FASTCALL
MsqWaitForNewMessages(
   PUSER_MESSAGE_QUEUE MessageQueue,
   HWND WndFilter,
   UINT MsgFilterMin,
   UINT MsgFilterMax
)
{
   PVOID WaitObjects[2] = {MessageQueue->NewMessages, &HardwareMessageEvent};
   NTSTATUS Status;


   UserLeave();

   Status = KeWaitForMultipleObjects(2,
                                     WaitObjects,
                                     WaitAny,
                                     Executive,
                                     UserMode,
                                     FALSE,
                                     NULL,//Timeout,
                                     NULL);
   DPRINT1("MsqWaitForNewMessages woke, ret=%i\n", Status);
   UserEnterExclusive();

   return Status;
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
   /* uhm. whats the point of this? why not embedd it in the struct itself? */
//   MessageQueue->CaretInfo = (PTHRDCARETINFO)(MessageQueue + 1);
   MessageQueue->Input = (PUSER_THREAD_INPUT)(MessageQueue + 1);

   InitializeListHead(&MessageQueue->PostedMessagesListHead);
   InitializeListHead(&MessageQueue->SentMessagesListHead);
   InitializeListHead(&MessageQueue->HardwareMessagesListHead);
   InitializeListHead(&MessageQueue->DispatchingMessagesHead);
   InitializeListHead(&MessageQueue->LocalDispatchingMessagesHead);
   KeInitializeMutex(&MessageQueue->HardwareLock, 0);
   MessageQueue->QuitPosted = FALSE;
   MessageQueue->QuitExitCode = 0;
   KeQueryTickCount(&LargeTickCount);
   MessageQueue->LastMsgRead = LargeTickCount.u.LowPart;
//   MessageQueue->FocusWindow = NULL;
   MessageQueue->PaintPosted = FALSE;
   MessageQueue->PaintCount = 0;
   MessageQueue->WakeMask = ~0; //FIXME!
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
   
   //FIXME: event now has 2 references. missing ZwClose()?

   return TRUE;
}

VOID FASTCALL
MsqCleanupMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue)
{
   PLIST_ENTRY CurrentEntry;
   PUSER_MESSAGE CurrentMessage;
//   PTIMER_ENTRY CurrentTimer;
   PUSER_SENT_MESSAGE CurrentSentMessage;

   /* cleanup posted messages */
   while (!IsListEmpty(&MessageQueue->PostedMessagesListHead))
   {
      CurrentEntry = RemoveHeadList(&MessageQueue->PostedMessagesListHead);
      CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE, ListEntry);
      MsqDestroyMessage(CurrentMessage);
   }

   /* remove the messages that have not yet been dispatched */
   while (!IsListEmpty(&MessageQueue->SentMessagesListHead))
   {
      CurrentEntry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE, ListEntry);

      DPRINT("Notify the sender and remove a message from the queue that had not been dispatched\n");

      /* remove the message from the dispatching list */
//      if(CurrentSentMessage->DispatchingListEntry.Flink != NULL)
//7      {
//      RemoveEntryList(&CurrentSentMessage->DispatchingListEntry);
//      }

      /* remove from sender. it might not be linked into the sender queue bcause sender
         might be dead.
       */
      if (CurrentSentMessage->SenderQueue)
      {
         RemoveEntryList(&CurrentSentMessage->DispatchingListEntry);
      }

      /* wake the sender's thread. */
      if (CurrentSentMessage->CompletionEvent != NULL)
      {
         KeSetEvent(CurrentSentMessage->CompletionEvent, IO_NO_INCREMENT, FALSE);
      }

      /* dereference our and the sender's message queue */
      /* shoudnt be necesary */
//      IntDereferenceMessageQueue(MessageQueue);
//      IntDereferenceMessageQueue(CurrentSentMessage->SenderQueue);

      /* free the message */
      ExFreePool(CurrentSentMessage);
   }

   /* cleanup timers */
//   while (! IsListEmpty(&MessageQueue->ExpiredTimersList))
//   {
//      CurrentEntry = RemoveHeadList(&MessageQueue->ExpiredTimersList);
//      CurrentTimer = CONTAINING_RECORD(CurrentEntry, TIMER_ENTRY, ListEntry);
//      UserFreeTimer(CurrentTimer);
//   }
   
   UserRemoveTimersQueue(MessageQueue);
   ASSERT(MessageQueue->TimerCount == 0);   

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

      /* dereference our and the sender's message queue */
      /* shoudnt be necesary */
//      IntDereferenceMessageQueue(MessageQueue);
//      IntDereferenceMessageQueue(CurrentSentMessage->SenderQueue);

      /* free the message */
      ExFreePool(CurrentSentMessage);
   }

   /* tell other threads not to bother returning any info to us */
   while (! IsListEmpty(&MessageQueue->DispatchingMessagesHead))
   {
      CurrentEntry = RemoveHeadList(&MessageQueue->DispatchingMessagesHead);
      CurrentSentMessage = CONTAINING_RECORD(CurrentEntry, USER_SENT_MESSAGE, DispatchingListEntry);
      
      CurrentSentMessage->CompletionEvent = NULL;
      CurrentSentMessage->SenderQueue = NULL;
      CurrentSentMessage->Result = NULL;

      /* do NOT dereference our message queue as it might get attempted to be
         locked later */
   }

}

PUSER_MESSAGE_QUEUE FASTCALL
MsqCreateMessageQueue(struct _ETHREAD *Thread)
{
   PUSER_MESSAGE_QUEUE Queue;

   Queue = UserAllocZeroTag(sizeof(USER_MESSAGE_QUEUE) + sizeof(USER_THREAD_INPUT),// + sizeof(THRDCARETINFO),
                  TAG_MSGQ);

   if (!Queue)
      return NULL;
   
   /* initialize the queue */
   if (!MsqInitializeMessageQueue(Thread, Queue))
   {
      UserFree(Queue);
      return NULL;
   }

   return Queue;
}




#if 0
    struct process *process = thread->process;
    struct thread_input *input;

    remove_thread_hooks( thread );
    if (!thread->queue) return;
    if (process->queue == thread->queue)  /* is it the process main queue? */
    {
        release_object( process->queue );
        process->queue = NULL;
        if (process->idle_event)
        {
            set_event( process->idle_event );
            release_object( process->idle_event );
            process->idle_event = NULL;
        }
    }
    input = thread->queue->input;
    release_object( thread->queue );
    thread->queue = NULL;
#endif


/* called when the thread is destroyed */
VOID FASTCALL
MsqDestroyMessageQueue(PW32THREAD W32Thread)
{
   //FIXME: PW32THREAD er en win32k private struct og kan fint inneholde PUSER_MESSAGE_QUEUE!
   //PsSetWin32Thread etc..
   PUSER_MESSAGE_QUEUE Queue = (PUSER_MESSAGE_QUEUE)W32Thread->MessageQueue;

   DPRINT1("Free message queue 0x%x\n", Queue);
   //DPRINT1("MsqDestroyMessageQueue 0x%x\n", Queue);

   /* remove the message queue from any desktops */
   //FIXME: queue->desktop? what about thread->hDesktop?? why both?
   if (Queue->Desktop)
   {
      Queue->Desktop->ActiveMessageQueue = NULL;
      Queue->Desktop = NULL;
   }


   //   if ((desk = (PDESKTOP_OBJECT)InterlockedExchange((LONG*)&MessageQueue->Desktop, 0)))
   //   {
   //      ASSERT(MessageQueue == desk->ActiveMessageQueue);

   //      InterlockedExchange((LONG*)&desk->ActiveMessageQueue, 0);
   //HUH??? what about desk->ActiveMessageQueue?? shouldnt it be derefed?
   //      IntDereferenceMessageQueue(MessageQueue);
   //   }

   /* if this is the primitive message queue, deregister it */
   if (Queue == W32kGetPrimitiveMessageQueue())
   W32kUnregisterPrimitiveMessageQueue();

   /* cleanup timers */
   UserRemoveTimersQueue(Queue);

   /* clean it up */
   MsqCleanupMessageQueue(Queue);

   //can some externals have reference to these two?
   //arent they only used internally?
   if (Queue->NewMessages != NULL)
   ObDereferenceObject(Queue->NewMessages);

   if (Queue->NewMessagesHandle != NULL)
   ZwClose(Queue->NewMessagesHandle);

   ExFreePool(Queue);

   W32Thread->MessageQueue = NULL;
   
#if 0

      cleanup_results( queue );
      for (i = 0; i < NB_MSG_KINDS; i++) empty_msg_list( &queue->msg_list[i] );

      while ((ptr = list_head( &queue->pending_timers )))
      {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        list_remove( &timer->entry );
        free( timer );
      }
      while ((ptr = list_head( &queue->expired_timers )))
      {
        struct timer *timer = LIST_ENTRY( ptr, struct timer, entry );
        list_remove( &timer->entry );
        free( timer );
      }
      if (queue->timeout) remove_timeout_user( queue->timeout );
      if (queue->input) release_object( queue->input );
      if (queue->hooks) release_object( queue->hooks );
    
#endif
   
}


inline PUSER_THREAD_INPUT FASTCALL UserGetCurrentInput()
{
   return UserGetCurrentQueue()->Input;
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
   PUSER_MESSAGE_QUEUE MessageQueue;

   MessageQueue = UserGetCurrentQueue();
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
   PUSER_MESSAGE_QUEUE MessageQueue;

   MessageQueue = UserGetCurrentQueue();
   if(!MessageQueue)
   {
      return 0;
   }

   return MessageQueue->ExtraInfo;
}

HWND FASTCALL
MsqSetStateWindow(PUSER_THREAD_INPUT Input, ULONG Type, HWND hWnd)
{
   HWND Prev;

   switch(Type)
   {
      case MSQ_STATE_CAPTURE:
         Prev = Input->CaptureWindow;
         Input->CaptureWindow = hWnd;
         return Prev;
      case MSQ_STATE_ACTIVE:
         Prev = Input->ActiveWindow;
         Input->ActiveWindow = hWnd;
         return Prev;
      case MSQ_STATE_FOCUS:
         Prev = Input->FocusWindow;
         Input->FocusWindow = hWnd;
         return Prev;
      case MSQ_STATE_MENUOWNER:
         Prev = Input->MenuOwner;
         Input->MenuOwner = hWnd;
         return Prev;
      case MSQ_STATE_MOVESIZE:
         Prev = Input->MoveSize;
         Input->MoveSize = hWnd;
         return Prev;
      case MSQ_STATE_CARET:
         //ASSERT(Input->CaretInfo);
         Prev = Input->CaretInfo.hWnd;
         Input->CaretInfo.hWnd = hWnd;
         return Prev;
   }

   return NULL;
}

inline BOOL FASTCALL
UserMessageFilter(UINT Message, UINT FilterMin, UINT FilterMax)
{
   if (!FilterMin && !FilterMax) return TRUE;
   
   return (Message >= FilterMin && Message <= FilterMax); 
}



BOOLEAN FASTCALL
MsqGetTimerMessage(
   PUSER_MESSAGE_QUEUE Queue,
   HWND hWndFilter, //FIXME: NULL, INVALID_HANDLE_VALUE
   UINT MsgFilterMin, 
   UINT MsgFilterMax,
   MSG *Msg, 
   BOOLEAN Restart
   )
{
   PTIMER_ENTRY Timer;
   
   Timer = UserFindExpiredTimer(
      Queue, 
      GetWnd(hWndFilter), 
      MsgFilterMin, 
      MsgFilterMax,
      Restart
      );
      
   if (Timer)
   {
      Msg->hwnd = GetHwnd(Timer->Wnd);
      Msg->message = Timer->Message;
      Msg->wParam = (WPARAM) Timer->IDEvent;
      Msg->lParam = (LPARAM) Timer->TimerFunc;
   }
   
   return (BOOL)Timer;
}


/* EOF */
