#pragma once

#define MSQ_HUNG        5000
#define MSQ_NORMAL      0
#define MSQ_ISHOOK      1
#define MSQ_ISEVENT     2
#define MSQ_INJECTMODULE 3

#define QSIDCOUNTS 6

typedef enum _QS_ROS_TYPES
{
    QSRosKey = 0,
    QSRosMouseMove,
    QSRosMouseButton,
    QSRosPostMessage,
    QSRosSendMessage,
    QSRosHotKey,
}QS_ROS_TYPES,*PQS_ROS_TYPES;

typedef struct _USER_MESSAGE
{
  LIST_ENTRY ListEntry;
  MSG Msg;
  DWORD QS_Flags;
} USER_MESSAGE, *PUSER_MESSAGE;

struct _USER_MESSAGE_QUEUE;

typedef struct _USER_SENT_MESSAGE
{
  LIST_ENTRY ListEntry;
  MSG Msg;
  DWORD QS_Flags;  // Original QS bits used to create this message.
  PKEVENT CompletionEvent;
  LRESULT* Result;
  LRESULT lResult;
  struct _USER_MESSAGE_QUEUE* SenderQueue;
  struct _USER_MESSAGE_QUEUE* CallBackSenderQueue;
  SENDASYNCPROC CompletionCallback;
  ULONG_PTR CompletionCallbackContext;
  /* entry in the dispatching list of the sender's message queue */
  LIST_ENTRY DispatchingListEntry;
  INT HookMessage;
  BOOL HasPackedLParam;
} USER_SENT_MESSAGE, *PUSER_SENT_MESSAGE;

typedef struct _USER_MESSAGE_QUEUE
{
  /* Reference counter, only access this variable with interlocked functions! */
  LONG References;

  /* Owner of the message queue */
  struct _ETHREAD *Thread;
  /* Queue of messages sent to the queue. */
  LIST_ENTRY SentMessagesListHead;
  /* Queue of messages posted to the queue. */
  LIST_ENTRY PostedMessagesListHead;
  /* Queue for hardware messages for the queue. */
  LIST_ENTRY HardwareMessagesListHead;
  /* True if a WM_MOUSEMOVE is pending */
  BOOLEAN MouseMoved;
  /* Current WM_MOUSEMOVE message */
  MSG MouseMoveMsg;
  /* Last click message for translating double clicks */
  MSG msgDblClk;
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
  /* Current capture window for this queue. */
  HWND CaptureWindow;
  PWND spwndCapture;
  /* Current window with focus (ie. receives keyboard input) for this queue. */
  PWND spwndFocus;
  /* Current active window for this queue. */
  PWND spwndActive;
  PWND spwndActivePrev;
  /* Current move/size window for this queue */
  HWND MoveSize;
  /* Current menu owner window for this queue */
  HWND MenuOwner;
  /* Identifes the menu state */
  BYTE MenuState;
  /* Caret information for this queue */
  PTHRDCARETINFO CaretInfo;
  /* Message Queue Flags */
  DWORD QF_flags;
  DWORD cThreads; // Shared message queue counter.

  /* Queue state tracking */
  // Send list QS_SENDMESSAGE
  // Post list QS_POSTMESSAGE|QS_HOTKEY|QS_PAINT|QS_TIMER|QS_KEY
  // Hard list QS_MOUSE|QS_KEY only
  // Accounting of queue bit sets, the rest are flags. QS_TIMER QS_PAINT counts are handled in thread information.
  DWORD nCntsQBits[QSIDCOUNTS]; // QS_KEY QS_MOUSEMOVE QS_MOUSEBUTTON QS_POSTMESSAGE QS_SENDMESSAGE QS_HOTKEY

  /* Extra message information */
  LPARAM ExtraInfo;

  /* State of each key */
  BYTE afKeyRecentDown[256 / 8]; // 1 bit per key
  BYTE afKeyState[256 * 2 / 8]; // 2 bits per key

  /* Showing cursor counter (value>=0 - cursor visible, value<0 - cursor hidden) */
  INT iCursorLevel;
  /* Cursor object */
  PCURICON_OBJECT CursorObject;

  /* Messages that are currently dispatched by other threads */
  LIST_ENTRY DispatchingMessagesHead;
  /* Messages that are currently dispatched by this message queue, required for cleanup */
  LIST_ENTRY LocalDispatchingMessagesHead;

  /* Desktop that the message queue is attached to */
  struct _DESKTOP *Desktop;
} USER_MESSAGE_QUEUE, *PUSER_MESSAGE_QUEUE;

#define QF_UPDATEKEYSTATE         0x00000001
#define QF_FMENUSTATUSBREAK       0x00000004
#define QF_FMENUSTATUS            0x00000008
#define QF_FF10STATUS             0x00000010
#define QF_MOUSEMOVED             0x00000020 // See MouseMoved.
#define QF_ACTIVATIONCHANGE       0x00000040
#define QF_TABSWITCHING           0x00000080
#define QF_KEYSTATERESET          0x00000100
#define QF_INDESTROY              0x00000200
#define QF_LOCKNOREMOVE           0x00000400
#define QF_FOCUSNULLSINCEACTIVE   0x00000800
#define QF_DIALOGACTIVE           0x00004000
#define QF_EVENTDEACTIVATEREMOVED 0x00008000
#define QF_TRACKMOUSELEAVE        0x00020000
#define QF_TRACKMOUSEHOVER        0x00040000
#define QF_TRACKMOUSEFIRING       0x00080000
#define QF_CAPTURELOCKED          0x00100000
#define QF_ACTIVEWNDTRACKING      0x00200000

/* Internal messages codes */
enum internal_event_message
{
    WM_ASYNC_SHOWWINDOW = 0x80000000,
    WM_ASYNC_SETWINDOWPOS,
    WM_ASYNC_SETACTIVEWINDOW
};

BOOL FASTCALL MsqIsHung(PUSER_MESSAGE_QUEUE MessageQueue);
VOID CALLBACK HungAppSysTimerProc(HWND,UINT,UINT_PTR,DWORD);
NTSTATUS FASTCALL co_MsqSendMessage(PUSER_MESSAGE_QUEUE MessageQueue,
           HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam,
           UINT uTimeout, BOOL Block, INT HookMessage, ULONG_PTR *uResult);
PUSER_MESSAGE FASTCALL MsqCreateMessage(LPMSG Msg);
VOID FASTCALL MsqDestroyMessage(PUSER_MESSAGE Message);
VOID FASTCALL MsqPostMessage(PUSER_MESSAGE_QUEUE MessageQueue, MSG* Msg, BOOLEAN HardwareMessage, DWORD MessageBits);
VOID FASTCALL MsqPostQuitMessage(PUSER_MESSAGE_QUEUE MessageQueue, ULONG ExitCode);
BOOLEAN APIENTRY
MsqPeekMessage(IN PUSER_MESSAGE_QUEUE MessageQueue,
	              IN BOOLEAN Remove,
	              IN PWND Window,
	              IN UINT MsgFilterLow,
	              IN UINT MsgFilterHigh,
	              IN UINT QSflags,
	              OUT PMSG Message);
BOOL APIENTRY
co_MsqPeekHardwareMessage(IN PUSER_MESSAGE_QUEUE MessageQueue,
	                      IN BOOL Remove,
	                      IN PWND Window,
	                      IN UINT MsgFilterLow,
	                      IN UINT MsgFilterHigh,
	                      IN UINT QSflags,
	                      OUT MSG* pMsg);
BOOL APIENTRY
co_MsqPeekMouseMove(IN PUSER_MESSAGE_QUEUE MessageQueue,
                    IN BOOL Remove,
                    IN PWND Window,
                    IN UINT MsgFilterLow,
                    IN UINT MsgFilterHigh,
                    OUT MSG* pMsg);
BOOLEAN FASTCALL MsqInitializeMessageQueue(struct _ETHREAD *Thread, PUSER_MESSAGE_QUEUE MessageQueue);
PUSER_MESSAGE_QUEUE FASTCALL MsqCreateMessageQueue(struct _ETHREAD *Thread);
VOID FASTCALL MsqDestroyMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
INIT_FUNCTION NTSTATUS NTAPI MsqInitializeImpl(VOID);
BOOLEAN FASTCALL co_MsqDispatchOneSentMessage(PUSER_MESSAGE_QUEUE MessageQueue);
NTSTATUS FASTCALL
co_MsqWaitForNewMessages(PUSER_MESSAGE_QUEUE MessageQueue, PWND WndFilter,
                      UINT MsgFilterMin, UINT MsgFilterMax);
VOID FASTCALL MsqIncPaintCountQueue(PUSER_MESSAGE_QUEUE Queue);
VOID FASTCALL MsqDecPaintCountQueue(PUSER_MESSAGE_QUEUE Queue);
LRESULT FASTCALL co_IntSendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT FASTCALL co_IntPostOrSendMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);
LRESULT FASTCALL
co_IntSendMessageTimeout(HWND hWnd,
                      UINT Msg,
                      WPARAM wParam,
                      LPARAM lParam,
                      UINT uFlags,
                      UINT uTimeout,
                      ULONG_PTR *uResult);

BOOL FASTCALL UserSendNotifyMessage( HWND hWnd,UINT Msg,WPARAM wParam,LPARAM lParam );
LRESULT FASTCALL co_IntSendMessageNoWait(HWND hWnd,
                        UINT Msg,
                        WPARAM wParam,
                        LPARAM lParam);
LRESULT FASTCALL
co_IntSendMessageWithCallBack(HWND hWnd,
                              UINT Msg,
                              WPARAM wParam,
                              LPARAM lParam,
                              SENDASYNCPROC CompletionCallback,
                              ULONG_PTR CompletionCallbackContext,
                              ULONG_PTR *uResult);
BOOL FASTCALL
co_MsqSendMessageAsync(PTHREADINFO ptiReceiver,
                       HWND hwnd,
                       UINT Msg,
                       WPARAM wParam,
                       LPARAM lParam,
                       SENDASYNCPROC CompletionCallback,
                       ULONG_PTR CompletionCallbackContext,
                       BOOL HasPackedLParam,
                       INT HookMessage);

LRESULT FASTCALL IntDispatchMessage(MSG* Msg);
BOOL FASTCALL IntTranslateKbdMessage(LPMSG lpMsg, UINT flags);
VOID FASTCALL MsqPostHotKeyMessage(PVOID Thread, HWND hWnd, WPARAM wParam, LPARAM lParam);
VOID FASTCALL co_MsqInsertMouseMessage(MSG* Msg, DWORD flags, ULONG_PTR dwExtraInfo, BOOL Hook);
BOOL FASTCALL MsqIsClkLck(LPMSG Msg, BOOL Remove);
BOOL FASTCALL MsqIsDblClk(LPMSG Msg, BOOL Remove);
HWND FASTCALL MsqSetStateWindow(PUSER_MESSAGE_QUEUE MessageQueue, ULONG Type, HWND hWnd);
BOOL APIENTRY IntInitMessagePumpHook(VOID);
BOOL APIENTRY IntUninitMessagePumpHook(VOID);

LPARAM FASTCALL MsqSetMessageExtraInfo(LPARAM lParam);
LPARAM FASTCALL MsqGetMessageExtraInfo(VOID);
VOID APIENTRY MsqRemoveWindowMessagesFromQueue(PVOID pWindow); /* F*(&$ headers, will be gone in the rewrite! */

#define IntReferenceMessageQueue(MsgQueue) \
  InterlockedIncrement(&(MsgQueue)->References)

#define IntDereferenceMessageQueue(MsgQueue) \
  do { \
    if(InterlockedDecrement(&(MsgQueue)->References) == 0) \
    { \
      TRACE("Free message queue 0x%p\n", (MsgQueue)); \
      if ((MsgQueue)->NewMessages != NULL) \
        ObDereferenceObject((MsgQueue)->NewMessages); \
      ExFreePoolWithTag((MsgQueue), USERTAG_Q); \
    } \
  } while(0)

#define IS_BTN_MESSAGE(message,code) \
  ((message) == WM_LBUTTON##code || \
   (message) == WM_MBUTTON##code || \
   (message) == WM_RBUTTON##code || \
   (message) == WM_XBUTTON##code || \
   (message) == WM_NCLBUTTON##code || \
   (message) == WM_NCMBUTTON##code || \
   (message) == WM_NCRBUTTON##code || \
   (message) == WM_NCXBUTTON##code )

#define WM_NCMOUSEFIRST WM_NCMOUSEMOVE
#define WM_NCMOUSELAST  (WM_NCMOUSEFIRST+(WM_MOUSELAST-WM_MOUSEFIRST))

#define IS_MOUSE_MESSAGE(message) \
    ((message >= WM_NCMOUSEFIRST && message <= WM_NCMOUSELAST) || \
            (message >= WM_MOUSEFIRST && message <= WM_MOUSELAST))

#define IS_KBD_MESSAGE(message) \
    (message >= WM_KEYFIRST && message <= WM_KEYLAST)

HANDLE FASTCALL IntMsqSetWakeMask(DWORD WakeMask);
BOOL FASTCALL IntMsqClearWakeMask(VOID);

static __inline LONG
MsqCalculateMessageTime(IN PLARGE_INTEGER TickCount)
{
    return (LONG)(TickCount->QuadPart * (KeQueryTimeIncrement() / 10000));
}

VOID FASTCALL IdlePing(VOID);
VOID FASTCALL IdlePong(VOID);
BOOL FASTCALL co_MsqReplyMessage(LRESULT);
VOID FASTCALL MsqWakeQueue(PUSER_MESSAGE_QUEUE,DWORD,BOOL);
VOID FASTCALL ClearMsgBitsMask(PUSER_MESSAGE_QUEUE,UINT);

int UserShowCursor(BOOL bShow);
PCURICON_OBJECT
FASTCALL
UserSetCursor(PCURICON_OBJECT NewCursor,
              BOOL ForceChange);

DWORD APIENTRY IntGetQueueStatus(DWORD);

UINT lParamMemorySize(UINT Msg, WPARAM wParam, LPARAM lParam);
/* EOF */
