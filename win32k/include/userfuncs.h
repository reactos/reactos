#ifndef _WIN32K_USERFUNCS_H
#define _WIN32K_USERFUNCS_H

/*************** KEYBOARD.C ******************/

DWORD FASTCALL 
UserGetKeyState(DWORD key);

/******************** HANDLE.C ***************/

VOID UserInitHandleTable(PUSER_HANDLE_TABLE ht, PVOID mem, ULONG bytes);
HANDLE UserAllocHandle(PUSER_HANDLE_TABLE ht, PVOID object, USER_OBJECT_TYPE type );
PVOID UserGetObject(PUSER_HANDLE_TABLE ht, HANDLE handle, USER_OBJECT_TYPE type );
PVOID UserFreeHandle(PUSER_HANDLE_TABLE ht, HANDLE handle );
PVOID UserGetNextHandle(PUSER_HANDLE_TABLE ht, HANDLE* handle, USER_OBJECT_TYPE type );


/************* DESKTOP.C *****************/

inline PDESKTOP_OBJECT FASTCALL UserGetCurrentDesktop();




/************************* ACCELERATOR.C ***********************/

inline PACCELERATOR_TABLE FASTCALL UserGetAccelObject(HACCEL hCursor);

PACCELERATOR_TABLE FASTCALL UserCreateAccelObject(HACCEL* h);



/* metric.c */
ULONG FASTCALL 
UserGetSystemMetrics(ULONG Index);

/* input.h */
NTSTATUS FASTCALL 
UserAcquireOrReleaseInputOwnership(BOOLEAN Release);

/******************** FOCUS.C ********************************/

PWINDOW_OBJECT FASTCALL
UserGetForegroundWindow(VOID);

PWINDOW_OBJECT FASTCALL
UserSetFocus(PWINDOW_OBJECT Wnd OPTIONAL);


/* painting.c */
DWORD FASTCALL 
UserInvalidateRect(PWINDOW_OBJECT Wnd, CONST RECT *Rect, BOOL Erase);

DWORD FASTCALL
UserScrollDC(HDC hDC, INT dx, INT dy, const RECT *lprcScroll,
   const RECT *lprcClip, HRGN hrgnUpdate, LPRECT lprcUpdate);

INT FASTCALL
UserGetUpdateRgn(PWINDOW_OBJECT Window, HRGN hRgn, BOOL bErase);

/* message.c */
BOOL FASTCALL 
UserPostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

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

/************************* WINPOS.C ****************************/


BOOL FASTCALL
UserGetClientOrigin(PWINDOW_OBJECT hWnd, LPPOINT Point);

/************************* SCROLLBAR.C ****************************/

DWORD FASTCALL
UserShowScrollBar(PWINDOW_OBJECT Wnd, int wBar, DWORD bShow);

/* timer.c */

inline VOID FASTCALL 
UserFreeTimer(PTIMER_ENTRY Timer);

VOID FASTCALL
UserRemoveTimersWindow(PWINDOW_OBJECT Wnd);

VOID FASTCALL
UserInsertTimer(PTIMER_ENTRY NewTimer);

VOID FASTCALL 
UserSetNextPendingTimer();

VOID FASTCALL
UserRemoveTimersQueue(PUSER_MESSAGE_QUEUE Queue);

/* hook.c*/
PHOOK FASTCALL
HookCreate(HHOOK* hHook);

PHOOK FASTCALL HookGet(HHOOK hHook);

/* class.c */

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


/* monitor.c */

PMONITOR_OBJECT FASTCALL UserCreateMonitorObject(HANDLE* h);

PMONITOR_OBJECT UserGetMonitorObject(HANDLE hCursor);


/* msgqueue.c */

VOID FASTCALL 
MsqRemoveWindowMessagesFromQueue(PWINDOW_OBJECT pWindow);

inline BOOL FASTCALL
UserMessageFilter(UINT Message, UINT FilterMin, UINT FilterMax);

VOID FASTCALL
MsqRemoveTimersWindow(PUSER_MESSAGE_QUEUE MessageQueue, PWINDOW_OBJECT Wnd);

VOID FASTCALL
MsqInsertExpiredTimer(PTIMER_ENTRY Timer);

PTIMER_ENTRY FASTCALL
UserFindExpiredTimer(   
   PUSER_MESSAGE_QUEUE Queue,
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

/* windc.c */

HDC FASTCALL
UserGetDCEx(PWINDOW_OBJECT Window OPTIONAL, HANDLE ClipRegion, ULONG Flags);

INT FASTCALL
UserReleaseDC(PWINDOW_OBJECT Wnd, HDC hDc);

DWORD FASTCALL
UserGetWindowDC(PWINDOW_OBJECT Wnd);

/* div */
#define UserGetCurrentQueue() \
((PUSER_MESSAGE_QUEUE)PsGetWin32Thread()->MessageQueue)



#endif /* _WIN32K_USERFUNCS_H */
