#ifndef __WIN32K_MSGQUEUE_H
#define __WIN32K_MSGQUEUE_H

#include <windows.h>

typedef struct _USER_MESSAGE
{
  LIST_ENTRY ListEntry;
  MSG Msg;
} USER_MESSAGE, *PUSER_MESSAGE;

struct _USER_MESSAGE_QUEUE;

typedef struct _USER_SENT_MESSAGE
{
  LIST_ENTRY ListEntry;
  MSG Msg;
  PKEVENT CompletionEvent;
  LRESULT* Result;
  struct _USER_MESSAGE_QUEUE* CompletionQueue;
  SENDASYNCPROC CompletionCallback;
  ULONG_PTR CompletionCallbackContext;
} USER_SENT_MESSAGE, *PUSER_SENT_MESSAGE;

typedef struct _USER_SENT_MESSAGE_NOTIFY
{
  SENDASYNCPROC CompletionCallback;
  ULONG_PTR CompletionCallbackContext;
  LRESULT Result;
  HWND hWnd;
  UINT Msg;
  LIST_ENTRY ListEntry;
} USER_SENT_MESSAGE_NOTIFY, *PUSER_SENT_MESSAGE_NOTIFY;

typedef struct _USER_MESSAGE_QUEUE
{
  LIST_ENTRY SentMessagesListHead;
  LIST_ENTRY PostedMessagesListHead;
  LIST_ENTRY HardwareMessagesListHead;
  LIST_ENTRY NotifyMessagesListHead;
  FAST_MUTEX Lock;
  BOOLEAN QuitPosted;
  ULONG QuitExitCode;
  KEVENT NewMessages;  
  ULONG QueueStatus;
  HWND FocusWindow;
} USER_MESSAGE_QUEUE, *PUSER_MESSAGE_QUEUE;

VOID
MsqSendMessage(PUSER_MESSAGE_QUEUE MessageQueue,
	       PUSER_SENT_MESSAGE Message);
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

#define MAKE_LONG(x, y) ((((y) & 0xFFFF) << 16) | ((x) & 0xFFFF))

#endif /* __WIN32K_MSGQUEUE_H */

/* EOF */
