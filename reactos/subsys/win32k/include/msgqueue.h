#ifndef _WIN32K_MSGQUEUE_H
#define _WIN32K_MSGQUEUE_H

#include <internal/ex.h>
#include <windows.h>
#include "caret.h"
#include "hook.h"

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
  /* Owner of the message queue */
  struct _ETHREAD *Thread;
  /* Queue of messages sent to the queue. */
  LIST_ENTRY SentMessagesListHead;
  /* Queue of messages posted to the queue. */
  LIST_ENTRY PostedMessagesListHead;
  /* Queue of sent-message notifies for the queue. */
  LIST_ENTRY NotifyMessagesListHead;
  /* Queue for hardware messages for the queue. */
  LIST_ENTRY HardwareMessagesListHead;
  /* Lock for the hardware message list. */
  FAST_MUTEX HardwareLock;
  /* Lock for the queue. */
  FAST_MUTEX Lock;
  /* True if a WM_QUIT message is pending. */
  BOOLEAN QuitPosted;
  /* The quit exit code. */
  ULONG QuitExitCode;
  /* Set if there are new messages in any of the queues. */
  KEVENT NewMessages;  
  /* FIXME: Unknown. */
  ULONG QueueStatus;
  /* Current window with focus (ie. receives keyboard input) for this queue. */
  HWND FocusWindow;
  /* True if a window needs painting. */
  BOOLEAN PaintPosted;
  /* Count of paints pending. */
  ULONG PaintCount;
  /* Current active window for this queue. */
  HWND ActiveWindow;
  /* Current capture window for this queue. */
  HWND CaptureWindow;
  /* Current move/size window for this queue */
  HWND MoveSize;
  /* Current menu owner window for this queue */
  HWND MenuOwner;
  /* Identifes the menu state */
  BYTE MenuState;
  /* Caret information for this queue */
  PTHRDCARETINFO CaretInfo;
  
  /* Window hooks */
  PHOOKTABLE Hooks;

  /* queue state tracking */
  WORD WakeBits;
  WORD WakeMask;
  WORD ChangedBits;
  WORD ChangedMask;
  
  /* extra message information */
  LPARAM ExtraInfo;

} USER_MESSAGE_QUEUE, *PUSER_MESSAGE_QUEUE;

LRESULT FASTCALL
MsqSendMessage(PUSER_MESSAGE_QUEUE MessageQueue,
	       HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam);
VOID FASTCALL
MsqInitializeMessage(PUSER_MESSAGE Message,
		     LPMSG Msg);
PUSER_MESSAGE FASTCALL
MsqCreateMessage(LPMSG Msg);
VOID FASTCALL
MsqDestroyMessage(PUSER_MESSAGE Message);
VOID FASTCALL
MsqPostMessage(PUSER_MESSAGE_QUEUE MessageQueue,
	       PUSER_MESSAGE Message);
VOID FASTCALL
MsqPostQuitMessage(PUSER_MESSAGE_QUEUE MessageQueue, ULONG ExitCode);
BOOLEAN STDCALL
MsqFindMessage(IN PUSER_MESSAGE_QUEUE MessageQueue,
	       IN BOOLEAN Hardware,
	       IN BOOLEAN Remove,
	       IN HWND Wnd,
	       IN UINT MsgFilterLow,
	       IN UINT MsgFilterHigh,
	       OUT PUSER_MESSAGE* Message);
VOID FASTCALL
MsqInitializeMessageQueue(struct _ETHREAD *Thread, PUSER_MESSAGE_QUEUE MessageQueue);
VOID FASTCALL
MsqFreeMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
PUSER_MESSAGE_QUEUE FASTCALL
MsqCreateMessageQueue(struct _ETHREAD *Thread);
VOID FASTCALL
MsqDestroyMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
PUSER_MESSAGE_QUEUE FASTCALL
MsqGetHardwareMessageQueue(VOID);
NTSTATUS FASTCALL
MsqWaitForNewMessage(PUSER_MESSAGE_QUEUE MessageQueue);
NTSTATUS FASTCALL
MsqInitializeImpl(VOID);
BOOLEAN FASTCALL
MsqDispatchOneSentMessage(PUSER_MESSAGE_QUEUE MessageQueue);
NTSTATUS FASTCALL
MsqWaitForNewMessages(PUSER_MESSAGE_QUEUE MessageQueue);
VOID FASTCALL
MsqSendNotifyMessage(PUSER_MESSAGE_QUEUE MessageQueue,
		     PUSER_SENT_MESSAGE_NOTIFY NotifyMessage);
VOID FASTCALL
MsqIncPaintCountQueue(PUSER_MESSAGE_QUEUE Queue);
VOID FASTCALL
MsqDecPaintCountQueue(PUSER_MESSAGE_QUEUE Queue);
LRESULT STDCALL
IntSendMessage(HWND hWnd,
		UINT Msg,
		WPARAM wParam,
		LPARAM lParam);
LRESULT FASTCALL
IntDispatchMessage(MSG* Msg);
BOOL FASTCALL
IntTranslateKbdMessage(LPMSG lpMsg, HKL dwhkl);

VOID STDCALL
MsqPostKeyboardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID STDCALL
MsqPostHotKeyMessage(PVOID Thread, HWND hWnd, WPARAM wParam, LPARAM lParam);
VOID FASTCALL
MsqInsertSystemMessage(MSG* Msg, BOOL RemMouseMoveMsg);

inline BOOL MsqIsSignaled( PUSER_MESSAGE_QUEUE queue );
inline VOID MsqSetQueueBits( PUSER_MESSAGE_QUEUE queue, WORD bits );
inline VOID MsqClearQueueBits( PUSER_MESSAGE_QUEUE queue, WORD bits );
BOOL IntInitMessagePumpHook();
BOOL IntUninitMessagePumpHook();
#define MAKE_LONG(x, y) ((((y) & 0xFFFF) << 16) | ((x) & 0xFFFF))

PHOOKTABLE FASTCALL MsqGetHooks(PUSER_MESSAGE_QUEUE Queue);
VOID FASTCALL MsqSetHooks(PUSER_MESSAGE_QUEUE Queue, PHOOKTABLE Hooks);

LPARAM FASTCALL MsqSetMessageExtraInfo(LPARAM lParam);
LPARAM FASTCALL MsqGetMessageExtraInfo(VOID);

#define IntLockMessageQueue(MsgQueue) \
  ExAcquireFastMutex(&MsgQueue->Lock)

#define IntUnLockMessageQueue(MsgQueue) \
  ExReleaseFastMutex(&MsgQueue->Lock)

#define IntLockHardwareMessageQueue(MsgQueue) \
  ExAcquireFastMutex(&MsgQueue->HardwareLock)

#define IntUnLockHardwareMessageQueue(MsgQueue) \
  ExReleaseFastMutex(&MsgQueue->HardwareLock)

/* check the queue status */
#define MsqIsSignaled(MsgQueue) \
  ((MsgQueue->WakeBits & MsgQueue->WakeMask) || (MsgQueue->ChangedBits & MsgQueue->ChangedMask))

#endif /* _WIN32K_MSGQUEUE_H */

/* EOF */
