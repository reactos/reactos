/* $Id: msgqueue.c,v 1.5 2002/07/17 21:04:57 dwelch Exp $
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

#define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

static PUSER_MESSAGE_QUEUE CurrentFocusMessageQueue;

/* FUNCTIONS *****************************************************************/

VOID
MsqIncPaintCountQueue(PUSER_MESSAGE_QUEUE Queue)
{
  ExAcquireFastMutex(&Queue->Lock);
  Queue->PaintCount++;
  Queue->PaintPosted = TRUE;
  ExReleaseFastMutex(&Queue->Lock);
}

VOID
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


NTSTATUS
MsqInitializeImpl(VOID)
{
  CurrentFocusMessageQueue = NULL;
  return(STATUS_SUCCESS);
}

VOID
MsqPostKeyboardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
  MSG Msg;
  PUSER_MESSAGE Message;

  Msg.hwnd = CurrentFocusMessageQueue->FocusWindow;
  Msg.message = uMsg;
  Msg.wParam = wParam;
  Msg.lParam = lParam;
  /* FIXME: Initialize time and point. */

  Message = MsqCreateMessage(&Msg);
  MsqPostMessage(CurrentFocusMessageQueue, Message, TRUE);
}

VOID
MsqInitializeMessage(PUSER_MESSAGE Message,
		     LPMSG Msg)
{
  RtlMoveMemory(&Message->Msg, Msg, sizeof(MSG));
}

PUSER_MESSAGE
MsqCreateMessage(LPMSG Msg)
{
  PUSER_MESSAGE Message;
  
  Message = (PUSER_MESSAGE)ExAllocatePool(PagedPool, sizeof(USER_MESSAGE));
  if (!Message)
    {
      return NULL;
    }
  
  MsqInitializeMessage(Message, Msg);
  
  return Message;
}

VOID
MsqDestroyMessage(PUSER_MESSAGE Message)
{
  ExFreePool(Message);
}

VOID
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

    W32kCallSentMessageCallback(Message->CompletionCallback,
				Message->hWnd,
				Message->Msg,
				Message->CompletionCallbackContext,
				Message->Result);
  }
}

BOOLEAN
MsqPeekSentMessages(PUSER_MESSAGE_QUEUE MessageQueue)
{
  return(!IsListEmpty(&MessageQueue->SentMessagesListHead));
}

BOOLEAN
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
  Result = W32kCallWindowProc(NULL,
			      Message->Msg.hwnd,
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

VOID
MsqSendNotifyMessage(PUSER_MESSAGE_QUEUE MessageQueue,
		     PUSER_SENT_MESSAGE_NOTIFY NotifyMessage)
{
  ExAcquireFastMutex(&MessageQueue->Lock);
  InsertTailList(&MessageQueue->NotifyMessagesListHead, 
		 &NotifyMessage->ListEntry);
  ExReleaseFastMutex(&MessageQueue->Lock);
}

VOID
MsqSendMessage(PUSER_MESSAGE_QUEUE MessageQueue,
	       PUSER_SENT_MESSAGE Message)
{
  ExAcquireFastMutex(&MessageQueue->Lock);
  InsertTailList(&MessageQueue->SentMessagesListHead, &Message->ListEntry);
  ExReleaseFastMutex(&MessageQueue->Lock);
}

VOID
MsqPostMessage(PUSER_MESSAGE_QUEUE MessageQueue,
	       PUSER_MESSAGE Message,
	       BOOLEAN Hardware)
{
  ExAcquireFastMutex(&MessageQueue->Lock);
  if (Hardware)
    {
      InsertTailList(&MessageQueue->HardwareMessagesListHead, 
		     &Message->ListEntry);
    }
  else
    {
      InsertTailList(&MessageQueue->PostedMessagesListHead,
		     &Message->ListEntry);
    }
  KeSetEvent(&MessageQueue->NewMessages, IO_NO_INCREMENT, FALSE);
  ExReleaseFastMutex(&MessageQueue->Lock);
}

BOOLEAN
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
  
  ExAcquireFastMutex(&MessageQueue->Lock);
  if (Hardware)
    {
      CurrentEntry = MessageQueue->HardwareMessagesListHead.Flink;
      ListHead = &MessageQueue->HardwareMessagesListHead;
    }
  else
    {
      CurrentEntry = MessageQueue->PostedMessagesListHead.Flink;
      ListHead = &MessageQueue->PostedMessagesListHead;
    }
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

NTSTATUS
MsqWaitForNewMessages(PUSER_MESSAGE_QUEUE MessageQueue)
{
  return(KeWaitForSingleObject(&MessageQueue->NewMessages,
			       0,
			       UserMode,
			       TRUE,
			       NULL));
}

VOID
MsqInitializeMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue)
{
  InitializeListHead(&MessageQueue->PostedMessagesListHead);
  InitializeListHead(&MessageQueue->HardwareMessagesListHead);
  InitializeListHead(&MessageQueue->SentMessagesListHead);
  ExInitializeFastMutex(&MessageQueue->Lock);
  MessageQueue->QuitPosted = FALSE;
  MessageQueue->QuitExitCode = 0;
  KeInitializeEvent(&MessageQueue->NewMessages, NotificationEvent, FALSE);
  MessageQueue->QueueStatus = 0;
  MessageQueue->FocusWindow = NULL;
}

VOID
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
      ExFreePool(CurrentMessage);
    }

  CurrentEntry = MessageQueue->HardwareMessagesListHead.Flink;
  while (CurrentEntry != &MessageQueue->HardwareMessagesListHead)
    {
      CurrentMessage = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE, 
					 ListEntry);
      CurrentEntry = CurrentEntry->Flink;
      ExFreePool(CurrentMessage);
    }
}

PUSER_MESSAGE_QUEUE
MsqCreateMessageQueue(VOID)
{
  PUSER_MESSAGE_QUEUE MessageQueue;

  MessageQueue = (PUSER_MESSAGE_QUEUE)ExAllocatePool(PagedPool, 
				   sizeof(USER_MESSAGE_QUEUE));
  if (!MessageQueue)
    {
      return NULL;
    }
  
  MsqInitializeMessageQueue(MessageQueue);
  
  return MessageQueue;
}

VOID
MsqDestroyMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue)
{
  MsqFreeMessageQueue(MessageQueue);
  ExFreePool(MessageQueue);
}

/* EOF */
