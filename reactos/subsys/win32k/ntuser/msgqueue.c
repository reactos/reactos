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
/* $Id: msgqueue.c,v 1.64 2004/01/20 23:35:59 gvg Exp $
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

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/msgqueue.h>
#include <include/callback.h>
#include <include/window.h>
#include <include/winpos.h>
#include <include/winsta.h>
#include <include/desktop.h>
#include <include/class.h>
#include <include/object.h>
#include <include/input.h>
#include <include/cursoricon.h>
#include <include/focus.h>
#include <include/caret.h>

#define NDEBUG
#include <win32k/debug1.h>
#include <debug.h>

/* GLOBALS *******************************************************************/

#define TAG_MSGQ TAG('M', 'S', 'G', 'Q')

#define SYSTEM_MESSAGE_QUEUE_SIZE           (256)

static MSG SystemMessageQueue[SYSTEM_MESSAGE_QUEUE_SIZE];
static ULONG SystemMessageQueueHead = 0;
static ULONG SystemMessageQueueTail = 0;
static ULONG SystemMessageQueueCount = 0;
static ULONG SystemMessageQueueMouseMove = -1;
static KSPIN_LOCK SystemMessageQueueLock;

static ULONG volatile HardwareMessageQueueStamp = 0;
static LIST_ENTRY HardwareMessageQueueHead;
static FAST_MUTEX HardwareMessageQueueLock;

static KEVENT HardwareMessageEvent;

static PAGED_LOOKASIDE_LIST MessageLookasideList;

/* FUNCTIONS *****************************************************************/

/* check the queue status */
inline BOOL MsqIsSignaled( PUSER_MESSAGE_QUEUE Queue )
{
    return ((Queue->WakeBits & Queue->WakeMask) || (Queue->ChangedBits & Queue->ChangedMask));
}

/* set some queue bits */
inline VOID MsqSetQueueBits( PUSER_MESSAGE_QUEUE Queue, WORD Bits )
{
    Queue->WakeBits |= Bits;
    Queue->ChangedBits |= Bits;
    if (MsqIsSignaled( Queue )) KeSetEvent(&Queue->NewMessages, IO_NO_INCREMENT, FALSE);
}

/* clear some queue bits */
inline VOID MsqClearQueueBits( PUSER_MESSAGE_QUEUE Queue, WORD Bits )
{
    Queue->WakeBits &= ~Bits;
    Queue->ChangedBits &= ~Bits;
}

VOID FASTCALL
MsqIncPaintCountQueue(PUSER_MESSAGE_QUEUE Queue)
{
  ExAcquireFastMutex(&Queue->Lock);
  Queue->PaintCount++;
  Queue->PaintPosted = TRUE;
  KeSetEvent(&Queue->NewMessages, IO_NO_INCREMENT, FALSE);
  ExReleaseFastMutex(&Queue->Lock);
}

VOID FASTCALL
MsqDecPaintCountQueue(PUSER_MESSAGE_QUEUE Queue)
{
  ExAcquireFastMutex(&Queue->Lock);
  Queue->PaintCount--;
  if (Queue->PaintCount == 0)
    {
      Queue->PaintPosted = FALSE;
    }
  ExReleaseFastMutex(&Queue->Lock);
}


NTSTATUS FASTCALL
MsqInitializeImpl(VOID)
{
  /*CurrentFocusMessageQueue = NULL;*/
  InitializeListHead(&HardwareMessageQueueHead);
  KeInitializeEvent(&HardwareMessageEvent, NotificationEvent, 0);
  KeInitializeSpinLock(&SystemMessageQueueLock);
  ExInitializeFastMutex(&HardwareMessageQueueLock);

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
MsqInsertSystemMessage(MSG* Msg, BOOL RemMouseMoveMsg)
{
  LARGE_INTEGER LargeTickCount;
  KIRQL OldIrql;
  ULONG mmov = (ULONG)-1;
  
  KeQueryTickCount(&LargeTickCount);
  Msg->time = LargeTickCount.u.LowPart;

  KeAcquireSpinLock(&SystemMessageQueueLock, &OldIrql);

  /* only insert WM_MOUSEMOVE messages if not already in system message queue */
  if((Msg->message == WM_MOUSEMOVE) && RemMouseMoveMsg)
    mmov = SystemMessageQueueMouseMove;

  if(mmov != (ULONG)-1)
  {
    /* insert message at the queue head */
    while (mmov != SystemMessageQueueHead )
    {
      ULONG prev = mmov ? mmov - 1 : SYSTEM_MESSAGE_QUEUE_SIZE - 1;
      ASSERT(mmov >= 0);
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
      KeReleaseSpinLock(&SystemMessageQueueLock, OldIrql);
      return;
    }
    SystemMessageQueue[SystemMessageQueueTail] = *Msg;
    if(Msg->message == WM_MOUSEMOVE)
      SystemMessageQueueMouseMove = SystemMessageQueueTail;
    SystemMessageQueueTail =
      (SystemMessageQueueTail + 1) % SYSTEM_MESSAGE_QUEUE_SIZE;
    SystemMessageQueueCount++;
  }
  KeReleaseSpinLock(&SystemMessageQueueLock, OldIrql);
  KeSetEvent(&HardwareMessageEvent, IO_NO_INCREMENT, FALSE);
}

BOOL STATIC FASTCALL
MsqIsDblClk(PWINDOW_OBJECT Window, PUSER_MESSAGE Message, BOOL Remove)
{
  PWINSTATION_OBJECT WinStaObject;
  PSYSTEM_CURSORINFO CurInfo;
  NTSTATUS Status;
  LONG dX, dY;
  BOOL Res;
  
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
                                          KernelMode,
                                          0,
                                          &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    return FALSE;
  }
  CurInfo = &WinStaObject->SystemCursor;
  Res = (Message->Msg.hwnd == (HWND)CurInfo->LastClkWnd) && 
        ((Message->Msg.time - CurInfo->LastBtnDown) < CurInfo->DblClickSpeed);
  if(Res)
  {
    
    dX = CurInfo->LastBtnDownX - Message->Msg.pt.x;
    dY = CurInfo->LastBtnDownY - Message->Msg.pt.y;
    if(dX < 0) dX = -dX;
    if(dY < 0) dY = -dY;
    
    Res = (dX <= CurInfo->DblClickWidth) &&
          (dY <= CurInfo->DblClickHeight);

    if(Remove)
    {
      CurInfo->LastBtnDown = 0;
      CurInfo->LastBtnDownX = Message->Msg.pt.x;
      CurInfo->LastBtnDownY = Message->Msg.pt.y;
      CurInfo->LastClkWnd = NULL;
    }
  }
  else
  {
    if(Remove)
    {
      CurInfo->LastBtnDownX = Message->Msg.pt.x;
      CurInfo->LastBtnDownY = Message->Msg.pt.y;
      CurInfo->LastClkWnd = (HANDLE)Message->Msg.hwnd;
      CurInfo->LastBtnDown = Message->Msg.time;
    }
  }
  
  ObDereferenceObject(WinStaObject);
  return Res;
}

BOOL STATIC STDCALL
MsqTranslateMouseMessage(HWND hWnd, UINT FilterLow, UINT FilterHigh,
			 PUSER_MESSAGE Message, BOOL Remove, PBOOL Freed,
			 PWINDOW_OBJECT ScopeWin, PUSHORT HitTest,
			 PPOINT ScreenPoint, PBOOL MouseClick, BOOL FromGlobalQueue)
{
  USHORT Msg = Message->Msg.message;
  PWINDOW_OBJECT CaptureWin, Window = NULL;
  HWND Wnd;
  POINT Point;
  LPARAM SpareLParam;
  LRESULT Result;

  if (Msg == WM_LBUTTONDOWN || 
      Msg == WM_MBUTTONDOWN ||
      Msg == WM_RBUTTONDOWN ||
      Msg == WM_XBUTTONDOWN)
  {
    USHORT Hit = WinPosWindowFromPoint(ScopeWin, 
    				   Message->Msg.pt, 
    				   &Window);
    /*
    **Make sure that we have a window that is not already in focus
    */
    if (0 != Window && Window->Self != IntGetFocusWindow())
    {
        SpareLParam = MAKELONG(Hit, Msg);
        
        if(Window && (Hit != (USHORT)HTTRANSPARENT))
        {
          Result = IntSendMessage(Wnd, WM_MOUSEACTIVATE, (WPARAM)NtUserGetParent(Window->Self), (LPARAM)SpareLParam);
          
          switch (Result)
          {
              case MA_NOACTIVATEANDEAT:
                  *Freed = FALSE;
                  IntReleaseWindowObject(Window);
                  return TRUE;
              case MA_NOACTIVATE:
                  break;
              case MA_ACTIVATEANDEAT:
                  IntMouseActivateWindow(Window);
                  IntReleaseWindowObject(Window);
                  *Freed = FALSE;
                  return TRUE;
/*              case MA_ACTIVATE:
              case 0:*/
              default:
                  IntMouseActivateWindow(Window);
                  break;
          }
          IntReleaseWindowObject(Window);
        }
        else
        {
          ExFreePool(Message);
          *Freed = TRUE;
          return(FALSE);
        }
    }

  }
  
  CaptureWin = IntGetCaptureWindow();

  if (CaptureWin == NULL)
  {
    if(Msg == WM_MOUSEWHEEL)
    {
      *HitTest = HTCLIENT;
      Window = IntGetWindowObject(IntGetFocusWindow());
    }
    else
    {
      *HitTest = WinPosWindowFromPoint(ScopeWin, Message->Msg.pt, &Window);
      if(!Window)
      {
        /* change the cursor on desktop background */
        IntLoadDefaultCursors(TRUE);
      }
    }
  }
  else
  {
    Window = IntGetWindowObject(CaptureWin);
    *HitTest = HTCLIENT;
  }

  if (Window == NULL)
  {
    ExFreePool(Message);
    *Freed = TRUE;
    return(FALSE);
  }

  if (Window->MessageQueue != PsGetWin32Thread()->MessageQueue)
  {
    if (! FromGlobalQueue)
    {
      DPRINT("Moving msg between private queues\n");
      /* This message is already queued in a private queue, but we need
       * to move it to a different queue, perhaps because a new window
       * was created which now covers the screen area previously taken
       * by another window. To move it, we need to take it out of the
       * old queue. Note that we're already holding the lock mutes of the
       * old queue */
      RemoveEntryList(&Message->ListEntry);
    }
    ExAcquireFastMutex(&Window->MessageQueue->HardwareLock);
    InsertTailList(&Window->MessageQueue->HardwareMessagesListHead,
                   &Message->ListEntry);
    ExReleaseFastMutex(&Window->MessageQueue->HardwareLock);
    KeSetEvent(&Window->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
    IntReleaseWindowObject(Window);
    *Freed = FALSE;
    return(FALSE);
  }
  
  if (hWnd != NULL && Window->Self != hWnd &&
      !IntIsChildWindow(hWnd, Window->Self))
  {
    ExAcquireFastMutex(&Window->MessageQueue->HardwareLock);
    InsertTailList(&Window->MessageQueue->HardwareMessagesListHead,
                   &Message->ListEntry);
    ExReleaseFastMutex(&Window->MessageQueue->HardwareLock);
    KeSetEvent(&Window->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
    IntReleaseWindowObject(Window);
    *Freed = FALSE;
    return(FALSE);
  }
  
  switch (Msg)
  {
    case WM_LBUTTONDOWN:
    case WM_MBUTTONDOWN:
    case WM_RBUTTONDOWN:
    case WM_XBUTTONDOWN:
    {
      if ((((*HitTest) != HTCLIENT) || 
          (IntGetClassLong(Window, GCL_STYLE, FALSE) & CS_DBLCLKS)) &&
          MsqIsDblClk(Window, Message, Remove))
      {
        Msg += WM_LBUTTONDBLCLK - WM_LBUTTONDOWN;
      }
      break;
    }
  }
  
  *ScreenPoint = Message->Msg.pt;
  Point = Message->Msg.pt;

  if ((*HitTest) != HTCLIENT)
  {
    Msg += WM_NCMOUSEMOVE - WM_MOUSEMOVE;
    if((Msg == WM_NCRBUTTONUP) && 
       (((*HitTest) == HTCAPTION) || ((*HitTest) == HTSYSMENU)))
    {
      Msg = WM_CONTEXTMENU;
      Message->Msg.wParam = (WPARAM)Window->Self;
    }
    else
    {
      Message->Msg.wParam = *HitTest;
    }
  }
  else
  {
    if(Msg != WM_MOUSEWHEEL)
    {
      Point.x -= Window->ClientRect.left;
      Point.y -= Window->ClientRect.top;
    }
  }
  
  /* FIXME: Check message filter. */

  if (Remove)
    {
      Message->Msg.hwnd = Window->Self;
      Message->Msg.message = Msg;
      Message->Msg.lParam = MAKELONG(Point.x, Point.y);
    }
  
  IntReleaseWindowObject(Window);
  *Freed = FALSE;
  return(TRUE);
}

BOOL STDCALL
MsqPeekHardwareMessage(PUSER_MESSAGE_QUEUE MessageQueue, HWND hWnd,
		       UINT FilterLow, UINT FilterHigh, BOOL Remove,
		       PUSER_MESSAGE* Message)
{
  KIRQL OldIrql;
  USHORT HitTest;
  POINT ScreenPoint;
  BOOL Accept, Freed;
  BOOL MouseClick;
  PLIST_ENTRY CurrentEntry;
  PWINDOW_OBJECT DesktopWindow;

  if( !IntGetScreenDC() || 
      PsGetWin32Thread()->MessageQueue == W32kGetPrimitiveMessageQueue() ) 
  {
    return FALSE;
  }

  DesktopWindow = IntGetWindowObject(IntGetDesktopWindow());

  /* Process messages in the message queue itself. */
  ExAcquireFastMutex(&MessageQueue->HardwareLock);
  CurrentEntry = MessageQueue->HardwareMessagesListHead.Flink;
  while (CurrentEntry != &MessageQueue->HardwareMessagesListHead)
    {
      PUSER_MESSAGE Current =
	CONTAINING_RECORD(CurrentEntry, USER_MESSAGE, ListEntry);
      CurrentEntry = CurrentEntry->Flink;
      if (Current->Msg.message >= WM_MOUSEFIRST &&
	  Current->Msg.message <= WM_MOUSELAST)
	{
	  Accept = MsqTranslateMouseMessage(hWnd, FilterLow, FilterHigh,
					    Current, Remove, &Freed,
					    DesktopWindow, &HitTest,
					    &ScreenPoint, &MouseClick, FALSE);
	  if (Accept)
	    {
	      if (Remove)
		{
		  RemoveEntryList(&Current->ListEntry);
		}
	      ExReleaseFastMutex(&MessageQueue->HardwareLock);
	      *Message = Current;
	      IntReleaseWindowObject(DesktopWindow);
	      return(TRUE);
	    }
	  
	  if(Freed)
	    RemoveEntryList(&Current->ListEntry);
	}
    }
  ExReleaseFastMutex(&MessageQueue->HardwareLock);

  /* Now try the global queue. */
  ExAcquireFastMutex(&HardwareMessageQueueLock);
  /* Transfer all messages from the DPC accessible queue to the main queue. */
  KeAcquireSpinLock(&SystemMessageQueueLock, &OldIrql);
  while (SystemMessageQueueCount > 0)
    {
      PUSER_MESSAGE UserMsg;
      MSG Msg;

      ASSERT(SystemMessageQueueHead < SYSTEM_MESSAGE_QUEUE_SIZE);
      Msg = SystemMessageQueue[SystemMessageQueueHead];
      SystemMessageQueueHead =
	(SystemMessageQueueHead + 1) % SYSTEM_MESSAGE_QUEUE_SIZE;
      SystemMessageQueueCount--;
      KeReleaseSpinLock(&SystemMessageQueueLock, OldIrql);
      UserMsg = ExAllocateFromPagedLookasideList(&MessageLookasideList);
      /* What to do if out of memory? For now we just panic a bit in debug */
      ASSERT(UserMsg);
      UserMsg->Msg = Msg;
      InsertTailList(&HardwareMessageQueueHead, &UserMsg->ListEntry);
      KeAcquireSpinLock(&SystemMessageQueueLock, &OldIrql);
    }
  /*
   * we could set this to -1 conditionally if we find one, but
   * this is more efficient and just as effective.
   */
  SystemMessageQueueMouseMove = -1;
  HardwareMessageQueueStamp++;
  KeReleaseSpinLock(&SystemMessageQueueLock, OldIrql);

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
	  ExReleaseFastMutex(&HardwareMessageQueueLock);
	  /* Translate the message. */
	  Accept = MsqTranslateMouseMessage(hWnd, FilterLow, FilterHigh,
					    Current, Remove, &Freed,
					    DesktopWindow, &HitTest,
					    &ScreenPoint, &MouseClick, TRUE);
	  ExAcquireFastMutex(&HardwareMessageQueueLock);
	  if (Accept)
	    {
	      /* Check for no more messages in the system queue. */
	      KeAcquireSpinLock(&SystemMessageQueueLock, &OldIrql);
	      if (SystemMessageQueueCount == 0 &&
		  IsListEmpty(&HardwareMessageQueueHead))
		{
		  KeClearEvent(&HardwareMessageEvent);
		}
	      KeReleaseSpinLock(&SystemMessageQueueLock, OldIrql);

	      /*
		 If we aren't removing the message then add it to the private
		 queue.
	      */
	      if (!Remove)
		{
                  ExAcquireFastMutex(&MessageQueue->HardwareLock);
		  InsertTailList(&MessageQueue->HardwareMessagesListHead,
				 &Current->ListEntry);
                  ExReleaseFastMutex(&MessageQueue->HardwareLock);
		}
	      ExReleaseFastMutex(&HardwareMessageQueueLock);
	      *Message = Current;
	      IntReleaseWindowObject(DesktopWindow);
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
  KeAcquireSpinLock(&SystemMessageQueueLock, &OldIrql);
  if (SystemMessageQueueCount == 0 && IsListEmpty(&HardwareMessageQueueHead))
    {
      KeClearEvent(&HardwareMessageEvent);
    }
  KeReleaseSpinLock(&SystemMessageQueueLock, OldIrql);
  ExReleaseFastMutex(&HardwareMessageQueueLock);

  return(FALSE);
}

VOID STDCALL
MsqPostKeyboardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  PUSER_MESSAGE_QUEUE FocusMessageQueue;
  PUSER_MESSAGE Message;
  MSG Msg;

  DPRINT("MsqPostKeyboardMessage(uMsg 0x%x, wParam 0x%x, lParam 0x%x)\n",
    uMsg, wParam, lParam);

  Msg.hwnd = 0;
  Msg.message = uMsg;
  Msg.wParam = wParam;
  Msg.lParam = lParam;
  /* FIXME: Initialize time and point. */
  
  FocusMessageQueue = IntGetFocusMessageQueue();
  if( !IntGetScreenDC() ) {
    if( W32kGetPrimitiveMessageQueue() ) {
      Message = MsqCreateMessage(&Msg);
      MsqPostMessage(W32kGetPrimitiveMessageQueue(), Message);
    }
  } else {
    if (FocusMessageQueue == NULL)
      {
	DPRINT("No focus message queue\n");
	return;
      }
    
    if (FocusMessageQueue->FocusWindow != (HWND)0)
      {
	Msg.hwnd = FocusMessageQueue->FocusWindow;
        DPRINT("Msg.hwnd = %x\n", Msg.hwnd);
	Message = MsqCreateMessage(&Msg);
	MsqPostMessage(FocusMessageQueue, Message);
      }
    else
      {
	DPRINT("Invalid focus window handle\n");
      }
  }
}

VOID STDCALL
MsqPostHotKeyMessage(PVOID Thread, HWND hWnd, WPARAM wParam, LPARAM lParam)
{
  PWINDOW_OBJECT Window;
  PUSER_MESSAGE Message;
  PW32THREAD Win32Thread;
  PW32PROCESS Win32Process;
  MSG Mesg;
  NTSTATUS Status;

  Status = ObReferenceObjectByPointer (Thread,
				       THREAD_ALL_ACCESS,
				       PsThreadType,
				       KernelMode);
  if (!NT_SUCCESS(Status))
    return;

  Win32Thread = ((PETHREAD)Thread)->Win32Thread;
  if (Win32Thread == NULL || Win32Thread->MessageQueue == NULL)
    {
      ObDereferenceObject ((PETHREAD)Thread);
      return;
    }

  Win32Process = ((PETHREAD)Thread)->ThreadsProcess->Win32Process;
  if (Win32Process == NULL || Win32Process->WindowStation == NULL)
    {
      ObDereferenceObject ((PETHREAD)Thread);
      return;
    }

  Status = ObmReferenceObjectByHandle(Win32Process->WindowStation->HandleTable,
                                      hWnd, otWindow, (PVOID*)&Window);
  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject ((PETHREAD)Thread);
      return;
    }

  Mesg.hwnd = hWnd;
  Mesg.message = WM_HOTKEY;
  Mesg.wParam = wParam;
  Mesg.lParam = lParam;
//      Mesg.pt.x = PsGetWin32Process()->WindowStation->SystemCursor.x;
//      Mesg.pt.y = PsGetWin32Process()->WindowStation->SystemCursor.y;
//      KeQueryTickCount(&LargeTickCount);
//      Mesg.time = LargeTickCount.u.LowPart;
  Message = MsqCreateMessage(&Mesg);
  MsqPostMessage(Window->MessageQueue, Message);
  ObmDereferenceObject(Window);
  ObDereferenceObject (Thread);

//  ExAcquireFastMutex(&pThread->MessageQueue->Lock);
//  InsertHeadList(&pThread->MessageQueue->PostedMessagesListHead,
//		 &Message->ListEntry);
//  KeSetEvent(&pThread->MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
//  ExReleaseFastMutex(&pThread->MessageQueue->Lock);

}

VOID FASTCALL
MsqInitializeMessage(PUSER_MESSAGE Message,
		     LPMSG Msg)
{
  RtlMoveMemory(&Message->Msg, Msg, sizeof(MSG));
}

PUSER_MESSAGE FASTCALL
MsqCreateMessage(LPMSG Msg)
{
  PUSER_MESSAGE Message;

  Message = ExAllocateFromPagedLookasideList(&MessageLookasideList);
  if (!Message)
    {
      return NULL;
    }

  MsqInitializeMessage(Message, Msg);

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
    ExAcquireFastMutex(&MessageQueue->Lock);
    ListEntry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
    Message = CONTAINING_RECORD(ListEntry, USER_SENT_MESSAGE_NOTIFY,
				ListEntry);
    ExReleaseFastMutex(&MessageQueue->Lock);

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
  PUSER_SENT_MESSAGE_NOTIFY NotifyMessage;

  ExAcquireFastMutex(&MessageQueue->Lock);
  if (IsListEmpty(&MessageQueue->SentMessagesListHead))
    {
      ExReleaseFastMutex(&MessageQueue->Lock);
      return(FALSE);
    }
  Entry = RemoveHeadList(&MessageQueue->SentMessagesListHead);
  Message = CONTAINING_RECORD(Entry, USER_SENT_MESSAGE, ListEntry);
  ExReleaseFastMutex(&MessageQueue->Lock);

  /* Call the window procedure. */
  Result = IntSendMessage(Message->Msg.hwnd,
                          Message->Msg.message,
                          Message->Msg.wParam,
                          Message->Msg.lParam);

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
  if (Message->CompletionCallback != NULL)
    {
      NotifyMessage = ExAllocatePool(NonPagedPool,
				     sizeof(USER_SENT_MESSAGE_NOTIFY));
      NotifyMessage->CompletionCallback =
	Message->CompletionCallback;
      NotifyMessage->CompletionCallbackContext =
	Message->CompletionCallbackContext;
      NotifyMessage->Result = Result;
      NotifyMessage->hWnd = Message->Msg.hwnd;
      NotifyMessage->Msg = Message->Msg.message;
      MsqSendNotifyMessage(Message->CompletionQueue, NotifyMessage);
    }

  ExFreePool(Message);
  return(TRUE);
}

VOID FASTCALL
MsqSendNotifyMessage(PUSER_MESSAGE_QUEUE MessageQueue,
		     PUSER_SENT_MESSAGE_NOTIFY NotifyMessage)
{
  ExAcquireFastMutex(&MessageQueue->Lock);
  InsertTailList(&MessageQueue->NotifyMessagesListHead,
		 &NotifyMessage->ListEntry);
  KeSetEvent(&MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
  ExReleaseFastMutex(&MessageQueue->Lock);
}

LRESULT FASTCALL
MsqSendMessage(PUSER_MESSAGE_QUEUE MessageQueue,
	       HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam)
{
  PUSER_SENT_MESSAGE Message;
  KEVENT CompletionEvent;
  PVOID WaitObjects[2];
  NTSTATUS WaitStatus;
  LRESULT Result;
  PUSER_MESSAGE_QUEUE ThreadQueue;

  KeInitializeEvent(&CompletionEvent, NotificationEvent, FALSE);

  Message = ExAllocatePoolWithTag(PagedPool, sizeof(USER_SENT_MESSAGE), TAG_MSGQ);
  Message->Msg.hwnd = Wnd;
  Message->Msg.message = Msg;
  Message->Msg.wParam = wParam;
  Message->Msg.lParam = lParam;
  Message->CompletionEvent = &CompletionEvent;
  Message->Result = &Result;
  Message->CompletionQueue = NULL;
  Message->CompletionCallback = NULL;

  ExAcquireFastMutex(&MessageQueue->Lock);
  InsertTailList(&MessageQueue->SentMessagesListHead, &Message->ListEntry);
  KeSetEvent(&MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
  ExReleaseFastMutex(&MessageQueue->Lock);

  ThreadQueue = PsGetWin32Thread()->MessageQueue;
  WaitObjects[1] = &ThreadQueue->NewMessages;
  WaitObjects[0] = &CompletionEvent;
  do
    {
      WaitStatus = KeWaitForMultipleObjects(2, WaitObjects, WaitAny, UserRequest,
                                            UserMode, TRUE, NULL, NULL);
      while (MsqDispatchOneSentMessage(ThreadQueue))
        {
          ;
        }
    }
  while (NT_SUCCESS(WaitStatus) && STATUS_WAIT_0 != WaitStatus);

  return (STATUS_WAIT_0 == WaitStatus ? Result : -1);
}

VOID FASTCALL
MsqPostMessage(PUSER_MESSAGE_QUEUE MessageQueue, PUSER_MESSAGE Message)
{
  ExAcquireFastMutex(&MessageQueue->Lock);
  InsertTailList(&MessageQueue->PostedMessagesListHead,
		 &Message->ListEntry);
  KeSetEvent(&MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
  ExReleaseFastMutex(&MessageQueue->Lock);
}

VOID FASTCALL
MsqPostQuitMessage(PUSER_MESSAGE_QUEUE MessageQueue, ULONG ExitCode)
{
  ExAcquireFastMutex(&MessageQueue->Lock);
  MessageQueue->QuitPosted = TRUE;
  MessageQueue->QuitExitCode = ExitCode;
  KeSetEvent(&MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
  ExReleaseFastMutex(&MessageQueue->Lock);
}

BOOLEAN STDCALL
MsqFindMessage(IN PUSER_MESSAGE_QUEUE MessageQueue,
	       IN BOOLEAN Hardware,
	       IN BOOLEAN Remove,
	       IN HWND Wnd,
	       IN UINT MsgFilterLow,
	       IN UINT MsgFilterHigh,
	       OUT PUSER_MESSAGE* Message)
{
  PLIST_ENTRY CurrentEntry;
  PUSER_MESSAGE CurrentMessage;
  PLIST_ENTRY ListHead;

  if (Hardware)
    {
      return(MsqPeekHardwareMessage(MessageQueue, Wnd,
				    MsgFilterLow, MsgFilterHigh,
				    Remove, Message));
    }

  ExAcquireFastMutex(&MessageQueue->Lock);
  CurrentEntry = MessageQueue->PostedMessagesListHead.Flink;
  ListHead = &MessageQueue->PostedMessagesListHead;
  while (CurrentEntry != ListHead)
    {
      CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
					 ListEntry);
      if ((Wnd == 0 || Wnd == CurrentMessage->Msg.hwnd) &&
	  ((MsgFilterLow == 0 && MsgFilterHigh == 0) ||
	   (MsgFilterLow <= CurrentMessage->Msg.message &&
	    MsgFilterHigh >= CurrentMessage->Msg.message)))
	{
	  if (Remove)
	    {
	      RemoveEntryList(&CurrentMessage->ListEntry);
	    }
	  ExReleaseFastMutex(&MessageQueue->Lock);
	  *Message = CurrentMessage;
	  return(TRUE);
	}
      CurrentEntry = CurrentEntry->Flink;
    }
  ExReleaseFastMutex(&MessageQueue->Lock);
  return(FALSE);
}

NTSTATUS FASTCALL
MsqWaitForNewMessages(PUSER_MESSAGE_QUEUE MessageQueue)
{
  PVOID WaitObjects[2] = {&MessageQueue->NewMessages, &HardwareMessageEvent};
  return(KeWaitForMultipleObjects(2,
				  WaitObjects,
				  WaitAny,
				  Executive,
				  UserMode,
				  TRUE,
				  NULL,
				  NULL));
}

VOID FASTCALL
MsqInitializeMessageQueue(struct _ETHREAD *Thread, PUSER_MESSAGE_QUEUE MessageQueue)
{
  MessageQueue->Thread = Thread;
  MessageQueue->CaretInfo = (PTHRDCARETINFO)(MessageQueue + 1);
  InitializeListHead(&MessageQueue->PostedMessagesListHead);
  InitializeListHead(&MessageQueue->SentMessagesListHead);
  InitializeListHead(&MessageQueue->HardwareMessagesListHead);
  ExInitializeFastMutex(&MessageQueue->HardwareLock);
  ExInitializeFastMutex(&MessageQueue->Lock);
  MessageQueue->QuitPosted = FALSE;
  MessageQueue->QuitExitCode = 0;
  KeInitializeEvent(&MessageQueue->NewMessages, SynchronizationEvent, FALSE);
  MessageQueue->QueueStatus = 0;
  MessageQueue->FocusWindow = NULL;
  MessageQueue->PaintPosted = FALSE;
  MessageQueue->PaintCount = 0;
}

VOID FASTCALL
MsqFreeMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue)
{
  PLIST_ENTRY CurrentEntry;
  PUSER_MESSAGE CurrentMessage;

  CurrentEntry = MessageQueue->PostedMessagesListHead.Flink;
  while (CurrentEntry != &MessageQueue->PostedMessagesListHead)
    {
      CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE,
					 ListEntry);
      CurrentEntry = CurrentEntry->Flink;
      MsqDestroyMessage(CurrentMessage);
    }
}

PUSER_MESSAGE_QUEUE FASTCALL
MsqCreateMessageQueue(struct _ETHREAD *Thread)
{
  PUSER_MESSAGE_QUEUE MessageQueue;

  MessageQueue = (PUSER_MESSAGE_QUEUE)ExAllocatePool(PagedPool,
				   sizeof(USER_MESSAGE_QUEUE) + sizeof(THRDCARETINFO));
  if (!MessageQueue)
    {
      return NULL;
    }

  MsqInitializeMessageQueue(Thread, MessageQueue);

  return MessageQueue;
}

VOID FASTCALL
MsqDestroyMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue)
{
  MsqFreeMessageQueue(MessageQueue);
  ExFreePool(MessageQueue);
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
  
  MessageQueue = PsGetWin32Thread()->MessageQueue;
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
  
  MessageQueue = PsGetWin32Thread()->MessageQueue;
  if(!MessageQueue)
  {
    return 0;
  }
  
  return MessageQueue->ExtraInfo;
}

/* EOF */
