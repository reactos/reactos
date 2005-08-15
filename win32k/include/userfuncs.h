#ifndef _WIN32K_USERFUNCS_H
#define _WIN32K_USERFUNCS_H


#define QUEUE_2_WTHREAD(queue) CONTAINING_RECORD(queue, W32THREAD, Queue)

/*************** KEYBOARD.C ******************/

DWORD FASTCALL 
UserGetKeyState(DWORD key);

BOOL FASTCALL
IntTranslateKbdMessage(LPMSG lpMsg, HKL dwhkl);


/******************** HANDLE.C ***************/

VOID UserInitHandleTable(PUSER_HANDLE_TABLE ht, PVOID mem, ULONG bytes);
HANDLE UserAllocHandle(PUSER_HANDLE_TABLE ht, PVOID object, USER_OBJECT_TYPE type );
PVOID UserGetObject(PUSER_HANDLE_TABLE ht, HANDLE handle, USER_OBJECT_TYPE type );
PVOID UserFreeHandle(PUSER_HANDLE_TABLE ht, HANDLE handle );
PVOID UserGetNextHandle(PUSER_HANDLE_TABLE ht, HANDLE* handle, USER_OBJECT_TYPE type );

/************* PROP.C *****************/

BOOL FASTCALL
IntSetProp(PWINDOW_OBJECT Wnd, ATOM Atom, HANDLE Data);

PPROPERTY FASTCALL
IntGetProp(PWINDOW_OBJECT WindowObject, ATOM Atom);


/************* DESKTOP.C *****************/

inline PDESKTOP_OBJECT FASTCALL UserGetCurrentDesktop();

NTSTATUS FASTCALL
InitDesktopImpl(VOID);

NTSTATUS FASTCALL
CleanupDesktopImpl(VOID);
                       
NTSTATUS STDCALL
IntDesktopObjectCreate(PVOID ObjectBody,
             PVOID Parent,
             PWSTR RemainingPath,
             struct _OBJECT_ATTRIBUTES* ObjectAttributes);

VOID STDCALL
IntDesktopObjectDelete(PVOID DeletedObject);

VOID FASTCALL
IntGetDesktopWorkArea(PDESKTOP_OBJECT Desktop, PRECT Rect);

LRESULT CALLBACK
IntDesktopWindowProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

HDC FASTCALL
IntGetScreenDC(VOID);

PWINDOW_OBJECT FASTCALL
UserGetDesktopWindow (VOID);

HWND FASTCALL
IntGetCurrentThreadDesktopWindow(VOID);

PUSER_MESSAGE_QUEUE FASTCALL
UserGetFocusQueue(VOID);

VOID FASTCALL
IntSetFocusQueue(PUSER_MESSAGE_QUEUE Thread);

PDESKTOP_OBJECT FASTCALL
UserGetActiveDesktop(VOID);

NTSTATUS FASTCALL
UserShowDesktop(PDESKTOP_OBJECT Desktop, ULONG Width, ULONG Height);

NTSTATUS FASTCALL
IntHideDesktop(PDESKTOP_OBJECT Desktop);

HDESK FASTCALL
IntGetDesktopObjectHandle(PDESKTOP_OBJECT DesktopObject);

NTSTATUS FASTCALL
IntValidateDesktopHandle(
   HDESK Desktop,
   KPROCESSOR_MODE AccessMode,
   ACCESS_MASK DesiredAccess,
   PDESKTOP_OBJECT *Object);

NTSTATUS FASTCALL
IntParseDesktopPath(PEPROCESS Process,
                    PUNICODE_STRING DesktopPath,
                    HWINSTA *hWinSta,
                    HDESK *hDesktop);

BOOL FASTCALL
IntDesktopUpdatePerUserSettings(BOOL bEnable);

BOOL IntRegisterShellHookWindow(HWND hWnd);
BOOL IntDeRegisterShellHookWindow(HWND hWnd);

VOID IntShellHookNotify(WPARAM Message, LPARAM lParam);

#define IntIsActiveDesktop(Desktop) \
  ((Desktop)->WindowStation->ActiveDesktop == (Desktop))



/************************* ACCELERATOR.C ***********************/

inline PACCELERATOR_TABLE FASTCALL UserGetAccelObject(HACCEL hCursor);

PACCELERATOR_TABLE FASTCALL UserCreateAccelObject(HACCEL* h);



/* metric.c */
ULONG FASTCALL 
UserGetSystemMetrics(ULONG Index);

/******************** INPUT.C ********************************/

#include <internal/kbd.h>

BOOLEAN attach_thread_input( PW32THREAD thread_from, PW32THREAD thread_to );
PUSER_THREAD_INPUT create_thread_input( PW32THREAD thread );
inline VOID FASTCALL UserDereferenceInput(PUSER_THREAD_INPUT Input);

NTSTATUS FASTCALL
InitInputImpl(VOID);
NTSTATUS FASTCALL
InitKeyboardImpl(VOID);
PUSER_MESSAGE_QUEUE W32kGetPrimitiveQueue(VOID);
VOID W32kUnregisterPrimitiveQueue(VOID);
PKBDTABLES W32kGetDefaultKeyLayout(VOID);
VOID FASTCALL W32kKeyProcessMessage(LPMSG Msg, PKBDTABLES KeyLayout, BYTE Prefix);
BOOL FASTCALL IntBlockInput(PW32THREAD W32Thread, BOOL BlockIt);
BOOL FASTCALL IntMouseInput(MOUSEINPUT *mi);
BOOL FASTCALL IntKeyboardInput(KEYBDINPUT *ki);

#define ThreadHasInputAccess(W32Thread) \
  (TRUE)

NTSTATUS FASTCALL 
UserAcquireOrReleaseInputOwnership(BOOLEAN Release);

/******************** FOCUS.C ********************************/

/*
 * These functions take the window handles from current message queue.
 */
PWINDOW_OBJECT FASTCALL
UserGetCaptureWindow();
PWINDOW_OBJECT FASTCALL
UserGetFocusWindow();

/*
 * These functions take the window handles from current thread queue.
 */
PWINDOW_OBJECT FASTCALL
UserGetThreadFocusWindow();

BOOL FASTCALL
IntMouseActivateWindow(PWINDOW_OBJECT Window);
BOOL FASTCALL
IntSetForegroundWindow(PWINDOW_OBJECT Window);
PWINDOW_OBJECT FASTCALL
IntSetActiveWindow(PWINDOW_OBJECT Window);

PWINDOW_OBJECT FASTCALL
UserGetForegroundWindow(VOID);

PWINDOW_OBJECT FASTCALL
UserSetFocus(PWINDOW_OBJECT Wnd OPTIONAL);


/******************** PAINTING.C ********************************/

VOID FASTCALL
IntValidateParent(PWINDOW_OBJECT Child, HRGN ValidRegion);
BOOL FASTCALL
UserRedrawWindow(PWINDOW_OBJECT Wnd, const RECT* UpdateRect, HRGN UpdateRgn, ULONG Flags);
BOOL FASTCALL
IntGetPaintMessage(HWND hWnd, UINT MsgFilterMin, UINT MsgFilterMax, PW32THREAD Thread,
                   MSG *Message, BOOL Remove);

BOOL FASTCALL UserValidateRgn(PWINDOW_OBJECT hWnd, HRGN hRgn);


#define IntLockWindowUpdate(Window) \
  ExAcquireFastMutex(&Window->UpdateLock)

#define IntUnLockWindowUpdate(Window) \
  ExReleaseFastMutex(&Window->UpdateLock)

DWORD FASTCALL 
UserInvalidateRect(PWINDOW_OBJECT Wnd, CONST RECT *Rect, BOOL Erase);

DWORD FASTCALL
UserScrollDC(HDC hDC, INT dx, INT dy, const RECT *lprcScroll,
   const RECT *lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate);

INT FASTCALL
UserGetUpdateRgn(PWINDOW_OBJECT Window, HRGN hRgn, BOOL bErase);

/******************** MESSAGE.C ********************************/

BOOL FASTCALL 
UserPostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

LRESULT FASTCALL
IntSendMessage(HWND hWnd,
      UINT Msg,
      WPARAM wParam,
      LPARAM lParam);
      
LRESULT FASTCALL
IntPostOrSendMessage(HWND hWnd,
           UINT Msg,
           WPARAM wParam,
           LPARAM lParam);
           
LRESULT FASTCALL
IntSendMessageTimeout(HWND hWnd,
                      UINT Msg,
                      WPARAM wParam,
                      LPARAM lParam,
                      UINT uFlags,
                      UINT uTimeout,
                      ULONG_PTR *uResult);
                      
LRESULT FASTCALL
IntDispatchMessage(MSG* Msg);



/************************ WINDOW.C *****************************/

inline VOID FASTCALL UserFreeWindowObject(PWINDOW_OBJECT Wnd);

PWINDOW_OBJECT FASTCALL IntGetWindowObject(HWND hWnd);

PWINDOW_OBJECT FASTCALL UserCreateWindowObject(HWND* h, ULONG bytes);

PWINDOW_OBJECT FASTCALL 
UserGetWindow(PWINDOW_OBJECT Wnd, UINT Relationship);

LONG FASTCALL 
UserGetWindowLong(PWINDOW_OBJECT Wnd, DWORD Index, BOOL Ansi);

LONG FASTCALL
UserSetWindowLong(PWINDOW_OBJECT Wnd, DWORD Index, LONG NewValue, BOOL Ansi);

BOOLEAN FASTCALL
UserDestroyWindow(PWINDOW_OBJECT Wnd);

HWND FASTCALL
GetHwnd(PWINDOW_OBJECT Wnd);

/************************* WINSTA.C ****************************/

inline PWINSTATION_OBJECT FASTCALL UserGetCurrentWinSta();

/************************* MENU.C ****************************/

inline PMENU_OBJECT FASTCALL UserGetMenuObject(HMENU hMenu);

HMENU FASTCALL
UserCreateMenu(BOOL PopupMenu);

PMENU_OBJECT FASTCALL 
UserCreateMenuObject(HANDLE* h);


/************************* CARET.C ****************************/

BOOL FASTCALL
UserShowCaret(PWINDOW_OBJECT Wnd);

BOOL FASTCALL
UserSetCaretPos(int X, int Y);

BOOL FASTCALL
UserHideCaret(PWINDOW_OBJECT Wnd);

BOOL FASTCALL
UserSwitchCaretShowing(PTHRDCARETINFO Info);

BOOL FASTCALL
UserDestroyCaret(PW32THREAD Win32Thread);

BOOL FASTCALL
UserSetCaretBlinkTime(UINT uMSeconds);

VOID FASTCALL
UserDrawCaret(HWND hWnd);

/************************* WINPOS.C ****************************/


BOOL FASTCALL
UserGetClientOrigin(PWINDOW_OBJECT hWnd, LPPOINT Point);

/************************* SCROLLBAR.C ****************************/

DWORD FASTCALL
UserShowScrollBar(PWINDOW_OBJECT Wnd, int wBar, DWORD bShow);


/************************* TIMER.C ****************************/

inline VOID FASTCALL 
UserFreeTimer(PTIMER_ENTRY Timer);

VOID FASTCALL
UserRemoveTimersWindow(PWINDOW_OBJECT Wnd);

VOID FASTCALL
UserInsertTimer(PTIMER_ENTRY NewTimer);

VOID FASTCALL 
UserSetNextPendingTimer();

VOID FASTCALL
UserRemoveTimersThread(PW32THREAD W32Thread);


/********************* HOOK.C *****************/

PHOOK FASTCALL
HookCreate(HHOOK* hHook);

PHOOK FASTCALL HookGet(HHOOK hHook);


/********************* CLASS.C *****************/


VOID FASTCALL
ClassReferenceClass(PWNDCLASS_OBJECT Class);

VOID FASTCALL
UserDereferenceClass(PWNDCLASS_OBJECT Class);

PWNDCLASS_OBJECT FASTCALL 
ClassCreateClass(DWORD bytes);


/********************* CURSORIICON.C *****************/

inline PCURICON_OBJECT FASTCALL 
UserGetCursorObject(HCURSOR hCursor);

VOID FASTCALL
CurIconReferenceCurIcon(PCURICON_OBJECT CurIcon);

PCURICON_OBJECT UserAllocCursorIcon(HCURSOR* h);
PCURICON_OBJECT UserGetCursorIcon(HCURSOR hCursor);

VOID FASTCALL
UserDereferenceCurIcon(PCURICON_OBJECT CurIcon);

VOID FASTCALL 
CursorDereference(PCURICON_OBJECT Cursor);

PCURICON_OBJECT FASTCALL 
UserCreateCursorObject(HCURSOR* hCursor, ULONG extraBytes);

PCURICON_OBJECT FASTCALL 
CursorGet(HCURSOR hCursor);


/******************* MONITOR.C ********************/

PMONITOR_OBJECT FASTCALL UserCreateMonitorObject(HANDLE* h);

PMONITOR_OBJECT UserGetMonitorObject(HANDLE hCursor);


/******************* MSGQUEUE.C ********************/

VOID FASTCALL
UserDereferenceQueue(PUSER_MESSAGE_QUEUE MsgQueue);

VOID FASTCALL
MsqDestroyMessageQueue(PW32THREAD WThread);

HWND FASTCALL
MsqSetStateWindow(PUSER_THREAD_INPUT Input, ULONG Type, HWND hWnd);


VOID FASTCALL 
MsqCleanupWindow(PWINDOW_OBJECT pWindow);

inline BOOL FASTCALL
UserMessageFilter(UINT Message, UINT FilterMin, UINT FilterMax);

VOID FASTCALL
MsqRemoveTimersWindow(PUSER_MESSAGE_QUEUE MessageQueue, PWINDOW_OBJECT Wnd);

VOID FASTCALL
MsqInsertExpiredTimer(PTIMER_ENTRY Timer);

PTIMER_ENTRY FASTCALL
UserFindExpiredTimer(   
   PUSER_MESSAGE_QUEUE MessageQueue,
   PWINDOW_OBJECT Wnd OPTIONAL, 
   UINT MsgFilterMin, 
   UINT MsgFilterMax,
   BOOL Remove
   );

PTIMER_ENTRY FASTCALL
MsqRemoveTimer(
   PWINDOW_OBJECT Wnd, 
   UINT_PTR IDEvent, 
   UINT Message
   );


BOOL FASTCALL
MsqIsHung(PUSER_MESSAGE_QUEUE MessageQueue);
NTSTATUS FASTCALL
MsqSendMessage(PUSER_MESSAGE_QUEUE MessageQueue,
          HWND Wnd, UINT Msg, WPARAM wParam, LPARAM lParam,
               UINT uTimeout, BOOL Block, BOOL HookMessage,
               ULONG_PTR *uResult);
PUSER_MESSAGE FASTCALL
MsqCreateMessage(LPMSG Msg, BOOLEAN FreeLParam);
VOID FASTCALL
MsqDestroyMessage(PUSER_MESSAGE Message);
VOID FASTCALL
MsqPostMessage(PUSER_MESSAGE_QUEUE MessageQueue,
          MSG* Msg, BOOLEAN FreeLParam, DWORD MessageBits);
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
BOOLEAN FASTCALL
MsqInitializeMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
VOID FASTCALL
MsqCleanupMessageQueue(PUSER_MESSAGE_QUEUE MessageQueue);
BOOLEAN FASTCALL
MsqCreateMessageQueue(PW32THREAD WThread);
PUSER_MESSAGE_QUEUE FASTCALL
MsqGetHardwareMessageQueue(VOID);
NTSTATUS FASTCALL
MsqInitializeImpl(VOID);
BOOLEAN FASTCALL
MsqDispatchOneSentMessage(PUSER_MESSAGE_QUEUE MessageQueue);
NTSTATUS FASTCALL
MsqWaitForNewMessages(PUSER_MESSAGE_QUEUE MessageQueue, HWND WndFilter,
                      UINT MsgFilterMin, UINT MsgFilterMax);
VOID FASTCALL
MsqSendNotifyMessage(PUSER_MESSAGE_QUEUE MessageQueue,
           PUSER_SENT_MESSAGE_NOTIFY NotifyMessage);
VOID FASTCALL
MsqIncPaintCountQueue(PUSER_MESSAGE_QUEUE Queue);
VOID FASTCALL
MsqDecPaintCountQueue(PUSER_MESSAGE_QUEUE Queue);

void cp(char* f, int l);
//#define IntSendMessage(a,b,c,d) (cp(__FILE__,__LINE__), IIntSendMessage(a,b,c,d)) 



VOID FASTCALL
MsqPostKeyboardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
VOID FASTCALL
MsqPostHotKeyMessage(PVOID Thread, HWND hWnd, WPARAM wParam, LPARAM lParam);
VOID FASTCALL
MsqInsertSystemMessage(MSG* Msg);
BOOL FASTCALL
MsqIsDblClk(LPMSG Msg, BOOL Remove);

inline BOOL MsqIsSignaled( PUSER_MESSAGE_QUEUE queue );
inline VOID MsqSetQueueBits( PUSER_MESSAGE_QUEUE queue, WORD bits );
inline VOID MsqClearQueueBits( PUSER_MESSAGE_QUEUE queue, WORD bits );

BOOL STDCALL IntInitMessagePumpHook();
BOOL STDCALL IntUninitMessagePumpHook();

#define MAKE_LONG(x, y) ((((y) & 0xFFFF) << 16) | ((x) & 0xFFFF))

PHOOKTABLE FASTCALL MsqGetHooks(PUSER_MESSAGE_QUEUE Queue);
VOID FASTCALL MsqSetHooks(PUSER_MESSAGE_QUEUE Queue, PHOOKTABLE Hooks);

LPARAM FASTCALL MsqSetMessageExtraInfo(LPARAM lParam);
LPARAM FASTCALL MsqGetMessageExtraInfo(VOID);

#define IntLockHardwareMessageQueue(MsgQueue) \
  KeWaitForMutexObject(&(MsgQueue)->HardwareLock, UserRequest, KernelMode, FALSE, NULL)

#define IntUnLockHardwareMessageQueue(MsgQueue) \
  KeReleaseMutex(&(MsgQueue)->HardwareLock, FALSE)

#define IntReferenceMessageQueue(MsgQueue) \
  InterlockedIncrement(&(MsgQueue)->References)



#define IS_BTN_MESSAGE(message,code) \
  ((message) == WM_LBUTTON##code || \
   (message) == WM_MBUTTON##code || \
   (message) == WM_RBUTTON##code || \
   (message) == WM_XBUTTON##code || \
   (message) == WM_NCLBUTTON##code || \
   (message) == WM_NCMBUTTON##code || \
   (message) == WM_NCRBUTTON##code || \
   (message) == WM_NCXBUTTON##code )

HANDLE FASTCALL
IntMsqSetWakeMask(DWORD WakeMask);

BOOL FASTCALL
IntMsqClearWakeMask(VOID);

BOOLEAN FASTCALL
MsqSetTimer(PUSER_MESSAGE_QUEUE MessageQueue, HWND Wnd,
            UINT_PTR IDEvent, UINT Period, TIMERPROC TimerFunc,
            UINT Msg);
BOOLEAN FASTCALL
MsqKillTimer(PUSER_MESSAGE_QUEUE MessageQueue, HWND Wnd,
             UINT_PTR IDEvent, UINT Msg);
BOOLEAN FASTCALL
MsqGetTimerMessage(PUSER_MESSAGE_QUEUE Queue,
                   HWND WndFilter, UINT MsgFilterMin, UINT MsgFilterMax,
                   MSG *Msg, BOOLEAN Restart);
BOOLEAN FASTCALL
MsqGetFirstTimerExpiry(PUSER_MESSAGE_QUEUE MessageQueue,
                       HWND WndFilter, UINT MsgFilterMin, UINT MsgFilterMax,
                       PLARGE_INTEGER FirstTimerExpiry);



/**************************** VIS.C ************************/

HRGN FASTCALL
VIS_ComputeVisibleRegion(PWINDOW_OBJECT Window, BOOLEAN ClientArea,
   BOOLEAN ClipChildren, BOOLEAN ClipSiblings);

VOID FASTCALL
VIS_WindowLayoutChanged(PWINDOW_OBJECT Window, HRGN UncoveredRgn);


/**************************** NTUSER.C ************************/

inline PVOID FASTCALL UserAlloc(SIZE_T bytes);
inline PVOID FASTCALL UserAllocTag(SIZE_T bytes, ULONG tag);
inline PVOID FASTCALL UserAllocZero(SIZE_T bytes);
inline PVOID FASTCALL UserAllocZeroTag(SIZE_T bytes, ULONG tag);
inline VOID FASTCALL UserFree(PVOID mem);

VOID FASTCALL
DestroyThreadWindows(PW32THREAD W32Thread);


/* windc.c */

HDC FASTCALL
UserGetDCEx(PWINDOW_OBJECT Window OPTIONAL, HANDLE ClipRegion, ULONG Flags);

INT FASTCALL
UserReleaseDC(PWINDOW_OBJECT Wnd, HDC hDc);

DWORD FASTCALL
UserGetWindowDC(PWINDOW_OBJECT Wnd);

/* div */
#define UserGetCurrentQueue() (&PsGetWin32Thread()->Queue)



#endif /* _WIN32K_USERFUNCS_H */
