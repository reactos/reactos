#pragma once

#define MSQ_HUNG        5000
#define MSQ_NORMAL      0
#define MSQ_ISHOOK      1
#define MSQ_INJECTMODULE 2

typedef struct _USER_MESSAGE
{
  LIST_ENTRY ListEntry;
  MSG Msg;
  DWORD QS_Flags;
  LONG_PTR ExtraInfo;
  DWORD dwQEvent;
  PTHREADINFO pti;
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
  PTHREADINFO ptiSender;
  PTHREADINFO ptiReceiver;
  SENDASYNCPROC CompletionCallback;
  PTHREADINFO ptiCallBackSender;
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

  /* Desktop that the message queue is attached to */
  struct _DESKTOP *Desktop;

  PTHREADINFO ptiSysLock;
  ULONG_PTR   idSysLock;
  ULONG_PTR   idSysPeek;
  PTHREADINFO ptiMouse;
  PTHREADINFO ptiKeyboard;

  /* Queue for hardware messages for the queue. */
  LIST_ENTRY HardwareMessagesListHead;
  /* Last click message for translating double clicks */
  MSG msgDblClk;
  /* Current capture window for this queue. */
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
  /* Message Queue Flags */
  DWORD QF_flags;
  DWORD cThreads; // Shared message queue counter.

  /* Extra message information */
  LPARAM ExtraInfo;

  /* State of each key */
  BYTE afKeyRecentDown[256 / 8]; // 1 bit per key
  BYTE afKeyState[256 * 2 / 8]; // 2 bits per key

  /* Showing cursor counter (value>=0 - cursor visible, value<0 - cursor hidden) */
  INT iCursorLevel;
  /* Cursor object */
  PCURICON_OBJECT CursorObject;

  /* Caret information for this queue */
  THRDCARETINFO CaretInfo;
} USER_MESSAGE_QUEUE, *PUSER_MESSAGE_QUEUE;

#define QF_UPDATEKEYSTATE         0x00000001
#define QF_FMENUSTATUSBREAK       0x00000004
#define QF_FMENUSTATUS            0x00000008
#define QF_FF10STATUS             0x00000010
#define QF_MOUSEMOVED             0x00000020
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
    WM_ASYNC_SETACTIVEWINDOW,
    WM_ASYNC_DESTROYWINDOW
};

#define POSTEVENT_NWE 14

BOOL FASTCALL MsqIsHung(PTHREADINFO pti);
VOID CALLBACK HungAppSysTimerProc(HWND,UINT,UINT_PTR,DWORD);
NTSTATUS FASTCALL co_MsqSendMessage(PTHREADINFO ptirec,
           HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam,
           UINT uTimeout, BOOL Block, INT HookMessage, ULONG_PTR *uResult);
PUSER_MESSAGE FASTCALL MsqCreateMessage(LPMSG Msg);
VOID FASTCALL MsqDestroyMessage(PUSER_MESSAGE Message);
VOID FASTCALL MsqPostMessage(PTHREADINFO, MSG*, BOOLEAN, DWORD, DWORD, LONG_PTR);
VOID FASTCALL MsqPostQuitMessage(PTHREADINFO pti, ULONG ExitCode);
BOOLEAN APIENTRY
MsqPeekMessage(IN PTHREADINFO pti,
	              IN BOOLEAN Remove,
	              IN PWND Window,
	              IN UINT MsgFilterLow,
	              IN UINT MsgFilterHigh,
	              IN UINT QSflags,
	              OUT LONG_PTR *ExtraInfo,
	              OUT PMSG Message);
BOOL APIENTRY
co_MsqPeekHardwareMessage(IN PTHREADINFO pti,
	                      IN BOOL Remove,
	                      IN PWND Window,
	                      IN UINT MsgFilterLow,
	                      IN UINT MsgFilterHigh,
	                      IN UINT QSflags,
	                      OUT MSG* pMsg);
BOOLEAN FASTCALL MsqInitializeMessageQueue(PTHREADINFO, PUSER_MESSAGE_QUEUE);
PUSER_MESSAGE_QUEUE FASTCALL MsqCreateMessageQueue(PTHREADINFO);
VOID FASTCALL MsqCleanupThreadMsgs(PTHREADINFO);
VOID FASTCALL MsqDestroyMessageQueue(_In_ PTHREADINFO pti);
INIT_FUNCTION NTSTATUS NTAPI MsqInitializeImpl(VOID);
BOOLEAN FASTCALL co_MsqDispatchOneSentMessage(_In_ PTHREADINFO pti);
NTSTATUS FASTCALL
co_MsqWaitForNewMessages(PTHREADINFO pti, PWND WndFilter,
                      UINT MsgFilterMin, UINT MsgFilterMax);
VOID FASTCALL MsqIncPaintCountQueue(PTHREADINFO);
VOID FASTCALL MsqDecPaintCountQueue(PTHREADINFO);
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

VOID FASTCALL IntCoalesceMouseMove(PTHREADINFO);
LRESULT FASTCALL IntDispatchMessage(MSG* Msg);
BOOL FASTCALL IntTranslateKbdMessage(LPMSG lpMsg, UINT flags);
VOID FASTCALL co_MsqInsertMouseMessage(MSG* Msg, DWORD flags, ULONG_PTR dwExtraInfo, BOOL Hook);
BOOL FASTCALL MsqIsClkLck(LPMSG Msg, BOOL Remove);
BOOL FASTCALL MsqIsDblClk(LPMSG Msg, BOOL Remove);
HWND FASTCALL MsqSetStateWindow(PTHREADINFO pti, ULONG Type, HWND hWnd);
BOOL APIENTRY IntInitMessagePumpHook(VOID);
BOOL APIENTRY IntUninitMessagePumpHook(VOID);

LPARAM FASTCALL MsqSetMessageExtraInfo(LPARAM lParam);
LPARAM FASTCALL MsqGetMessageExtraInfo(VOID);
VOID APIENTRY MsqRemoveWindowMessagesFromQueue(PWND pWindow);

#define IntReferenceMessageQueue(MsgQueue) \
  InterlockedIncrement(&(MsgQueue)->References)

#define IntDereferenceMessageQueue(MsgQueue) \
  do { \
    if(InterlockedDecrement(&(MsgQueue)->References) == 0) \
    { \
      TRACE("Free message queue 0x%p\n", (MsgQueue)); \
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
VOID FASTCALL MsqWakeQueue(PTHREADINFO,DWORD,BOOL);
VOID FASTCALL ClearMsgBitsMask(PTHREADINFO,UINT);
BOOL FASTCALL IntCallMsgFilter(LPMSG,INT);
WPARAM FASTCALL MsqGetDownKeyState(PUSER_MESSAGE_QUEUE);
BOOL FASTCALL IsThreadSuspended(PTHREADINFO);

int UserShowCursor(BOOL bShow);
PCURICON_OBJECT
FASTCALL
UserSetCursor(PCURICON_OBJECT NewCursor,
              BOOL ForceChange);

DWORD APIENTRY IntGetQueueStatus(DWORD);

UINT lParamMemorySize(UINT Msg, WPARAM wParam, LPARAM lParam);

BOOL APIENTRY
co_IntGetPeekMessage( PMSG pMsg,
                      HWND hWnd,
                      UINT MsgFilterMin,
                      UINT MsgFilterMax,
                      UINT RemoveMsg,
                      BOOL bGMSG );
BOOL FASTCALL
UserPostThreadMessage( PTHREADINFO pti,
                       UINT Msg,
                       WPARAM wParam,
                       LPARAM lParam );
BOOL FASTCALL
co_IntWaitMessage( PWND Window,
                   UINT MsgFilterMin,
                   UINT MsgFilterMax );

/* EOF */
