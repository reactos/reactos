/* $Id: msgqueue.c,v 1.1 2001/06/12 17:50:29 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Message queues
 * FILE:             subsys/win32k/ntuser/msgqueue.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/msgqueue.h>

#define NDEBUG
#include <debug.h>


VOID
MsqInitializeMessage(
  PUSER_MESSAGE Message,
  LPMSG Msg)
{
  RtlMoveMemory(&Message->Msg, Msg, sizeof(MSG));
}

PUSER_MESSAGE
MsqCreateMessage(
  LPMSG Msg)
{
  PUSER_MESSAGE Message;

  Message = (PUSER_MESSAGE)ExAllocatePool(
    PagedPool, sizeof(USER_MESSAGE));
  if (!Message)
  {
    return NULL;
  }

  MsqInitializeMessage(Message, Msg);

  return Message;
}

VOID
MsqDestroyMessage(
  PUSER_MESSAGE Message)
{
  ExFreePool(Message);
}

VOID
MsqPostMessage(
  PUSER_MESSAGE_QUEUE MessageQueue,
  PUSER_MESSAGE Message)
{
  // FIXME: Grab lock
  InsertTailList(&MessageQueue->ListHead, &Message->ListEntry);
}

BOOL
MsqRetrieveMessage(
  PUSER_MESSAGE_QUEUE MessageQueue,
  PUSER_MESSAGE *Message)
{
  PLIST_ENTRY CurrentEntry;

  // FIXME: Grab lock
  if (!IsListEmpty(&MessageQueue->ListHead))
  {
    CurrentEntry = RemoveHeadList(&MessageQueue->ListHead);
    *Message = CONTAINING_RECORD(CurrentEntry, USER_MESSAGE, ListEntry);
    return TRUE;
  }
  else
  {
    return FALSE;
  }
}


VOID
MsqInitializeMessageQueue(
  PUSER_MESSAGE_QUEUE MessageQueue)
{
  InitializeListHead(&MessageQueue->ListHead);
  ExInitializeFastMutex(MessageQueue->ListLock);
}

VOID
MsqFreeMessageQueue(
  PUSER_MESSAGE_QUEUE MessageQueue)
{
  PUSER_MESSAGE Message;

  // FIXME: Grab lock
  while (MsqRetrieveMessage(MessageQueue, &Message))
  {
    ExFreePool(Message);
  }
}

PUSER_MESSAGE_QUEUE
MsqCreateMessageQueue(VOID)
{
  PUSER_MESSAGE_QUEUE MessageQueue;

  MessageQueue = (PUSER_MESSAGE_QUEUE)ExAllocatePool(
    PagedPool, sizeof(USER_MESSAGE_QUEUE));
  if (!MessageQueue)
  {
    return NULL;
  }

  MsqInitializeMessageQueue(MessageQueue);

  return MessageQueue;
}

VOID
MsqDestroyMessageQueue(
  PUSER_MESSAGE_QUEUE MessageQueue)
{
  MsqFreeMessageQueue(MessageQueue);
  ExFreePool(MessageQueue);
}

/* EOF */
