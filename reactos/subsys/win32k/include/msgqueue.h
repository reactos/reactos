#ifndef __WIN32K_MSGQUEUE_H
#define __WIN32K_MSGQUEUE_H

#include <windows.h>

typedef struct _USER_MESSAGE
{
  LIST_ENTRY ListEntry;
  MSG Msg;
} USER_MESSAGE, *PUSER_MESSAGE;

typedef struct _USER_MESSAGE_QUEUE
{
   LIST_ENTRY ListHead;
   PFAST_MUTEX ListLock;
} USER_MESSAGE_QUEUE, *PUSER_MESSAGE_QUEUE;


VOID
MsqInitializeMessage(
  PUSER_MESSAGE Message,
  LPMSG Msg);

PUSER_MESSAGE
MsqCreateMessage(
  LPMSG Msg);

VOID
MsqDestroyMessage(
  PUSER_MESSAGE Message);

VOID
MsqPostMessage(
  PUSER_MESSAGE_QUEUE MessageQueue,
  PUSER_MESSAGE Message);

BOOL
MsqRetrieveMessage(
  PUSER_MESSAGE_QUEUE MessageQueue,
  PUSER_MESSAGE *Message);

VOID
MsqInitializeMessageQueue(
  PUSER_MESSAGE_QUEUE MessageQueue);

VOID
MsqFreeMessageQueue(
  PUSER_MESSAGE_QUEUE MessageQueue);

PUSER_MESSAGE_QUEUE
MsqCreateMessageQueue(VOID);

VOID
MsqDestroyMessageQueue(
  PUSER_MESSAGE_QUEUE MessageQueue);

#endif /* __WIN32K_MSGQUEUE_H */

/* EOF */
