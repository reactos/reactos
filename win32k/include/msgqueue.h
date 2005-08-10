#ifndef _WIN32K_MSGQUEUE_H
#define _WIN32K_MSGQUEUE_H

#include "caret.h"
#include "hook.h"

#define MSQ_HUNG        5000

typedef struct _USER_MESSAGE
{
  LIST_ENTRY ListEntry;
  BOOLEAN FreeLParam;
  MSG Msg;
} USER_MESSAGE, *PUSER_MESSAGE;

struct _USER_MESSAGE_QUEUE;

typedef struct _USER_SENT_MESSAGE
{
  LIST_ENTRY ListEntry;
  MSG Msg;
  PKEVENT CompletionEvent;
  LRESULT* Result;
  struct _USER_MESSAGE_QUEUE* SenderQueue;
  SENDASYNCPROC CompletionCallback;
  ULONG_PTR CompletionCallbackContext;
  /* entry in the dispatching list of the sender's message queue */
  LIST_ENTRY DispatchingListEntry;
  BOOL HookMessage;
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

typedef struct _USER_THREAD_INPUT
{
#if 0   
       struct desktop        *desktop;       /* desktop that this thread input belongs to */
       user_handle_t          focus;         /* focus window */
       user_handle_t          capture;       /* capture window */
       user_handle_t          active;        /* active window */
       user_handle_t          menu_owner;    /* current menu owner window */
       user_handle_t          move_size;     /* current moving/resizing window */
       user_handle_t          caret;         /* caret window */
       rectangle_t            caret_rect;    /* caret rectangle */
       int                    caret_hide;    /* caret hide count */
       int                    caret_state;   /* caret on/off state */
       struct list            msg_list;      /* list of hardware messages */
       unsigned char          keystate[256]; /* state of each key */
#endif

  /* Current window with focus (ie. receives keyboard input) for this queue. */
  HWND FocusWindow;
  /* Current capture window for this queue. */
  HWND CaptureWindow;
  /* Current active window for this queue. */
  HWND ActiveWindow;
  /* Current menu owner window for this queue */
  HWND MenuOwner;
  /* Current move/size window for this queue */
  HWND MoveSize;
 /* Caret information for this queue */
  THRDCARETINFO CaretInfo;
   
} USER_THREAD_INPUT, *PUSER_THREAD_INPUT;


typedef struct _USER_MESSAGE_QUEUE
{
  /* Queue of messages sent to the queue. */
  LIST_ENTRY SentMessagesListHead;
  /* Queue of messages posted to the queue. */
  LIST_ENTRY PostedMessagesListHead;
  /* Queue of sent-message notifies for the queue. */
  LIST_ENTRY NotifyMessagesListHead;
  /* Queue for hardware messages for the queue. */
  LIST_ENTRY HardwareMessagesListHead;
  /* Number of expired timers for this thread */
  ULONG TimerCount;
  /* Lock for the hardware message list. */
  KMUTEX HardwareLock;
  /* Pointer to the current WM_MOUSEMOVE message */
  PUSER_MESSAGE MouseMoveMsg;
  /* True if a WM_QUIT message is pending. */
  BOOLEAN QuitPosted;
  /* The quit exit code. */
  ULONG QuitExitCode;
  /* Set if there are new messages specified by WakeMask in any of the queues. */
  PKEVENT NewMessages;
  /* Handle for the above event (in the context of the process owning the queue). */
  HANDLE NewMessagesHandle;
  /* Last time PeekMessage() was called. */
  ULONG LastMsgRead;
  /* True if a window needs painting. */
  BOOLEAN PaintPosted;
  /* Count of paints pending. */
  LONG PaintCount;
  /* Identifes the menu state */
  BYTE MenuState;

  /* Window hooks */
  PHOOKTABLE Hooks;

  /* queue state tracking */
  WORD WakeMask;
  WORD WakeBits;
//  WORD QueueBits;
  WORD ChangedBits;
  WORD ChangedMask;

  /* extra message information */
  LPARAM ExtraInfo;
  /* messages that are currently dispatched by other threads */
  LIST_ENTRY DispatchingMessagesHead;
  /* messages that are currently dispatched by this message queue, required for cleanup */
  LIST_ENTRY LocalDispatchingMessagesHead;
  /* Desktop that the message queue is attached to */
  struct _DESKTOP_OBJECT *Desktop;
  
  
} USER_MESSAGE_QUEUE, *PUSER_MESSAGE_QUEUE;








#endif /* _WIN32K_MSGQUEUE_H */

/* EOF */
