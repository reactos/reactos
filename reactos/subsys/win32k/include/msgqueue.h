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
  LIST_ENTRY SentMessagesListHead;
  LIST_ENTRY PostedMessagesListHead;
  LIST_ENTRY HardwareMessagesListHead;
  FAST_MUTEX Lock;
  BOOLEAN QuitPosted;
  ULONG QuitExitCode;
  KEVENT NewMessages;  
  ULONG QueueStatus;
  HWND FocusWindow;
} USER_MESSAGE_QUEUE, *PUSER_MESSAGE_QUEUE;


VOID
MsqInitializeMessage(PUSER_MESSAGE Message,
		     LPMSG Msg);
PUSER_MESSAGE
MsqCreateMessage(LPMSG Msg);
VOID
MsqDestroyMessage(PUSER_MESSAGE Message);
VOID
MsqPostMessage(PUSER_MESSAGE_QUEUE MessageQueue,
	       PUSER_MESSAGE Message,
	       BOOLEAN Hardware);
BOOLEAN
MsqFindMessage(IN PUSER_MESSAGE_QUEUE MessageQueue,
	       IN BOOLEAN Hardware,
	       IN BOOLEAN Remove,
	       IN HWND Wnd,
	       IN UINT MsgFilterLow,
	       IN UINT MsgFilterHigh,
	       OUT PUSER_MESSAGE* Message);
VOID
MsqInitializeMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
VOID
MsqFreeMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
PUSER_MESSAGE_QUEUE
MsqCreateMessageQueue(VOID);
VOID
MsqDestroyMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
PUSER_MESSAGE_QUEUE
MsqGetHardwareMessageQueue(VOID);
NTSTATUS
MsqWaitForNewMessage(PUSER_MESSAGE_QUEUE MessageQueue);
NTSTATUS
MsqInitializeImpl(VOID);

#endif /* __WIN32K_MSGQUEUE_H */

/* EOF */
