#ifndef _WIN32K_USERFUNCS_H
#define _WIN32K_USERFUNCS_H

extern PWINSTATION_OBJECT gInteractiveWinSta;

#define QUEUE_2_WTHREAD(queue) CONTAINING_RECORD(queue, W32THREAD, Queue)



#define UserGetInputDesktop() (gInputDesktop)

#define UserGetCurrentQueue() (&PsGetWin32Thread()->Queue)

#define UserGetForegroundWThread() (QUEUE_2_WTHREAD(gInputDesktop->ActiveQueue))
#define UserGetForegroundQueue() (gInputDesktop ? gInputDesktop->ActiveQueue : NULL)
#define UserSetForegroundQueue(q) (gInputDesktop->ActiveQueue = (q))


/*************** USERATOM.C ******************/

RTL_ATOM FASTCALL
IntAddAtom(LPWSTR AtomName);
ULONG FASTCALL
IntGetAtomName(RTL_ATOM nAtom, LPWSTR lpBuffer, ULONG nSize);

/*************** KEYBOARD.C ******************/

DWORD FASTCALL 
UserGetKeyState(DWORD key);

BOOL FASTCALL
IntTranslateKbdMessage(LPMSG lpMsg, HKL dwhkl);

/******************** OBJECT.C ***************/

/******************** HANDLE.C ***************/

extern USER_HANDLE_TABLE gHandleTable;

PUSER_HANDLE_ENTRY handle_to_entry(PUSER_HANDLE_TABLE ht, HANDLE handle );

#define VOLATILE_OBJECTS_LIST SINGLE_LIST_ENTRY __object_list__ = {NULL};
   
   
#define UserLinkVolatileObject(obj) UserLinkVolatileObject2(&obj->hdr, &__object_list__, (PVOLATILE_OBJECT_ENTRY)_alloca(sizeof(VOLATILE_OBJECT_ENTRY)))
#define UserValidateVolatileObjects()  UserValidateVolatileObjects2(&__object_list__)

VOID FASTCALL UserLinkVolatileObject2(PUSER_OBJECT_HDR hdr, PSINGLE_LIST_ENTRY list, PVOLATILE_OBJECT_ENTRY e);
BOOLEAN FASTCALL UserValidateVolatileObjects2(PSINGLE_LIST_ENTRY list);

#define UserGetEntry(obj) UserGetEntry2(&(obj)->hdr)
#define UserValidateEntry(obj) UserValidateEntry2(&(obj))


inline OBJECT_ENTRY FASTCALL UserGetEntry2(PUSER_OBJECT_HDR hdr);
inline BOOLEAN FASTCALL UserValidateEntry2(POBJECT_ENTRY e);

   
VOID UserInitHandleTable(PUSER_HANDLE_TABLE ht, PVOID mem, ULONG bytes);
HANDLE UserAllocHandle(PUSER_HANDLE_TABLE ht, PVOID object, USER_OBJECT_TYPE type );
PVOID UserGetObject(PUSER_HANDLE_TABLE ht, HANDLE handle, USER_OBJECT_TYPE type );
PVOID UserFreeHandle(PUSER_HANDLE_TABLE ht, HANDLE handle );
PVOID UserGetNextHandle(PUSER_HANDLE_TABLE ht, HANDLE* handle, USER_OBJECT_TYPE type );

/************* PROP.C *****************/

BOOL FASTCALL
UserSetProp(PWINDOW_OBJECT Wnd, ATOM Atom, HANDLE Data);

PPROPERTY FASTCALL
UserGetProp(PWINDOW_OBJECT WindowObject, ATOM Atom);


/************* CALLBACK.C *****************/

LRESULT STDCALL
co_UserCallWindowProc(WNDPROC Proc,
                  BOOLEAN IsAnsiProc,
                  HWND Wnd,
                  UINT Message,
                  WPARAM wParam,
                  LPARAM lParam,
                  INT lParamBufferSize);

VOID STDCALL
co_UserCallSentMessageCallback(SENDASYNCPROC CompletionCallback,
             HWND hWnd,
             UINT Msg,
             ULONG_PTR CompletionCallbackContext,
             LRESULT Result);


HMENU STDCALL
co_UserLoadSysMenuTemplate();

BOOL STDCALL
co_UserLoadDefaultCursors(VOID);

LRESULT STDCALL
co_UserCallHookProc(INT HookId,
                INT Code,
                WPARAM wParam,
                LPARAM lParam,
                HOOKPROC Proc,
                BOOLEAN Ansi,
                PUNICODE_STRING ModuleName);

VOID FASTCALL
IntCleanupThreadCallbacks(PW32THREAD W32Thread);

PVOID FASTCALL
IntCbAllocateMemory(ULONG Size);

VOID FASTCALL
IntCbFreeMemory(PVOID Data);

/************* DESKTOP.C *****************/

extern PDESKTOP_OBJECT gInputDesktop;

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

//PUSER_MESSAGE_QUEUE FASTCALL
//UserGetFocusQueue(VOID);

//VOID FASTCALL
//IntSetFocusQueue(PUSER_MESSAGE_QUEUE Thread);

PDESKTOP_OBJECT FASTCALL
UserGetActiveDesktop(VOID);

NTSTATUS FASTCALL
co_UserShowDesktop(PDESKTOP_OBJECT Desktop, ULONG Width, ULONG Height);

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

VOID co_UserShellHookNotify(WPARAM Message, LPARAM lParam);

#define IntIsActiveDesktop(Desktop) ((Desktop) == (gInputDesktop))



/************************* ACCELERATOR.C ***********************/

BOOLEAN FASTCALL UserDestroyAccel(PACCELERATOR_TABLE Accel);

inline PACCELERATOR_TABLE FASTCALL UserGetAccelObject(HACCEL hCursor);

PACCELERATOR_TABLE FASTCALL UserCreateAccelObject();

NTSTATUS FASTCALL
InitAcceleratorImpl();

NTSTATUS FASTCALL
CleanupAcceleratorImpl();

VOID
RegisterThreadAcceleratorTable(struct _ETHREAD *Thread);

/************************* METRIC.C ***********************/


ULONG FASTCALL 
UserGetSystemMetrics(ULONG Index);

/******************** INPUT.C ********************************/

#include <internal/kbd.h>

BOOLEAN attach_thread_input( PW32THREAD thread_from, PW32THREAD thread_to );
PUSER_THREAD_INPUT create_thread_input( PW32THREAD thread );
inline VOID FASTCALL UserDereferenceInput(PUSER_THREAD_INPUT Input);

inline PW32THREAD FASTCALL UserWThreadFromTid(DWORD tid);

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


//inline VOID FASTCALL UserSetForegroundQueue(PUSER_MESSAGE_QUEUE Queue);


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
co_UserMouseActivateWindow(PWINDOW_OBJECT Window);
BOOL FASTCALL
co_UserSetForegroundWindow(PWINDOW_OBJECT Window);
PWINDOW_OBJECT FASTCALL
co_UserSetActiveWindow(PWINDOW_OBJECT Window);

PWINDOW_OBJECT FASTCALL
UserGetForegroundWindow(VOID);

PWINDOW_OBJECT FASTCALL
co_UserSetFocus(PWINDOW_OBJECT Wnd OPTIONAL);


/******************** PAINTING.C ********************************/

VOID FASTCALL
IntValidateParent(PWINDOW_OBJECT Child, HRGN ValidRegion);
BOOL FASTCALL
co_UserRedrawWindow(PWINDOW_OBJECT Wnd, const RECT* UpdateRect, HRGN UpdateRgn, ULONG Flags);
BOOL FASTCALL
IntGetPaintMessage(HWND hWnd, UINT MsgFilterMin, UINT MsgFilterMax, PW32THREAD Thread,
                   MSG *Message, BOOL Remove);

BOOL FASTCALL co_UserValidateRgn(PWINDOW_OBJECT hWnd, HRGN hRgn);


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
co_UserGetUpdateRgn(PWINDOW_OBJECT Window, HRGN hRgn, BOOL bErase);

/******************** MESSAGE.C ********************************/

BOOL FASTCALL 
UserPostMessage(HWND hWnd, UINT Msg, WPARAM wParam, LPARAM lParam);

LRESULT FASTCALL
co_UserSendMessage(HWND hWnd,
      UINT Msg,
      WPARAM wParam,
      LPARAM lParam);
      

           
LRESULT FASTCALL
co_UserSendMessageTimeout(HWND hWnd,
                      UINT Msg,
                      WPARAM wParam,
                      LPARAM lParam,
                      UINT uFlags,
                      UINT uTimeout,
                      ULONG_PTR *uResult);
                      
LRESULT FASTCALL
IntDispatchMessage(MSG* Msg);



/************************ WINDOW.C *****************************/

PWINDOW_OBJECT FASTCALL UserGetWindowObject_UpdateHandle2(HWND* h);
#define UserGetWindowObject_UpdateHandle(handle) UserGetWindowObject_UpdateHandle2(&(handle))

BOOL FASTCALL
UserSetSystemMenu(PWINDOW_OBJECT WindowObject, PMENU_OBJECT MenuObject);

inline VOID FASTCALL UserFreeWindowObject(PWINDOW_OBJECT Wnd);

inline PWINDOW_OBJECT FASTCALL UserGetWindowObject(HWND hWnd);

PWINDOW_OBJECT FASTCALL UserCreateWindowObject(ULONG bytes);

PWINDOW_OBJECT FASTCALL 
UserGetWindow(PWINDOW_OBJECT Wnd, UINT Relationship);

LONG FASTCALL 
UserGetWindowLong(PWINDOW_OBJECT Wnd, DWORD Index, BOOL Ansi);

LONG FASTCALL
co_UserSetWindowLong(PWINDOW_OBJECT Wnd, DWORD Index, LONG NewValue, BOOL Ansi);

BOOLEAN FASTCALL
co_UserDestroyWindow(PWINDOW_OBJECT Wnd);

HWND FASTCALL
GetHwndSafe(PWINDOW_OBJECT Wnd);

#define HAS_DLGFRAME(Style, ExStyle) \
            (((ExStyle) & WS_EX_DLGMODALFRAME) || \
            (((Style) & WS_DLGFRAME) && (!((Style) & WS_THICKFRAME))))

#define HAS_THICKFRAME(Style, ExStyle) \
            (((Style) & WS_THICKFRAME) && \
            (!(((Style) & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)))

#define HAS_THINFRAME(Style, ExStyle) \
            (((Style) & WS_BORDER) || (!((Style) & (WS_CHILD | WS_POPUP))))

#define IntIsDesktopWindow(WndObj) \
  (WndObj->ParentWnd == NULL)

#define IntIsBroadcastHwnd(hWnd) \
  (hWnd == HWND_BROADCAST || hWnd == HWND_TOPMOST)

#if 0
#define IntWndBelongsToThread(WndObj, W32Thread) \
  (((WndObj->W32Thread->Thread && WndObj->W32Thread->Thread->Tcb.Win32Thread)) && \
   (WndObj->W32Thread->Thread->Tcb.Win32Thread == W32Thread))
#endif

#define IntWndBelongsToThread(WndObj, _W32Thread) (QUEUE_2_WTHREAD((WndObj)->Queue) == (_W32Thread))


#define IntGetWndThreadId(WndObj) \
  WndObj->WThread->Thread->Cid.UniqueThread

#define IntGetWndProcessId(WndObj) \
  WndObj->WThread->Thread->ThreadsProcess->UniqueProcessId

PWINDOW_OBJECT FASTCALL
IntGetProcessWindowObject(PW32THREAD Thread, HWND hWnd);

BOOL FASTCALL
IntIsWindow(HWND hWnd);

PWINDOW_OBJECT* FASTCALL
UserListChildWnd(PWINDOW_OBJECT Window);

HWND* FASTCALL 
IntWinListChildren(PWINDOW_OBJECT Window);

NTSTATUS FASTCALL
InitWindowImpl (VOID);

NTSTATUS FASTCALL
CleanupWindowImpl (VOID);

VOID FASTCALL
IntGetClientRect (PWINDOW_OBJECT WindowObject, PRECT Rect);

PWINDOW_OBJECT FASTCALL 
UserGetActiveWindow(VOID);

BOOL FASTCALL
UserIsWindowVisible (PWINDOW_OBJECT hWnd);

BOOL FASTCALL
UserIsChildWindow (PWINDOW_OBJECT Parent, PWINDOW_OBJECT Child);

VOID FASTCALL
IntUnlinkWindow(PWINDOW_OBJECT Wnd);

VOID FASTCALL
IntLinkWindow(PWINDOW_OBJECT Wnd, PWINDOW_OBJECT WndParent, PWINDOW_OBJECT WndPrevSibling);

PWINDOW_OBJECT FASTCALL
UserGetAncestor(PWINDOW_OBJECT Wnd, UINT Type);

PWINDOW_OBJECT FASTCALL
UserGetParent(PWINDOW_OBJECT Wnd);

inline PWINDOW_OBJECT FASTCALL
UserGetOwner(PWINDOW_OBJECT Wnd);

INT FASTCALL
IntGetWindowRgn(HWND hWnd, HRGN hRgn);

INT FASTCALL
IntGetWindowRgnBox(HWND hWnd, RECT *Rect);

PWINDOW_OBJECT FASTCALL
UserGetShellWindow();

BOOL FASTCALL
IntGetWindowInfo(PWINDOW_OBJECT WindowObject, PWINDOWINFO pwi);

VOID FASTCALL
IntGetWindowBorderMeasures(PWINDOW_OBJECT WindowObject, UINT *cx, UINT *cy);

BOOL FASTCALL
IntAnyPopup(VOID);

BOOL FASTCALL
IntIsWindowInDestroy(PWINDOW_OBJECT Window);

DWORD IntRemoveWndProcHandle(WNDPROC Handle);
DWORD IntRemoveProcessWndProcHandles(HANDLE ProcessID);
DWORD IntAddWndProcHandle(WNDPROC WindowProc, BOOL IsUnicode);

BOOL FASTCALL
co_UserShowOwnedPopups( HWND owner, BOOL fShow );

/************************* WINSTA.C ****************************/

inline PWINSTATION_OBJECT FASTCALL UserGetCurrentWinSta();

extern WINSTATION_OBJECT *InputWindowStation;
extern PW32PROCESS LogonProcess;

NTSTATUS FASTCALL
InitWindowStationImpl(VOID);

NTSTATUS FASTCALL
CleanupWindowStationImpl(VOID);

NTSTATUS
STDCALL
IntWinStaObjectOpen(OB_OPEN_REASON Reason,
                    PVOID ObjectBody,
                    PEPROCESS Process,
                    ULONG HandleCount,
                    ACCESS_MASK GrantedAccess);

VOID STDCALL
IntWinStaObjectDelete(PVOID DeletedObject);

PVOID STDCALL
IntWinStaObjectFind(PVOID Object,
                    PWSTR Name,
                    ULONG Attributes);

NTSTATUS
STDCALL
IntWinStaObjectParse(PVOID Object,
                     PVOID *NextObject,
                     PUNICODE_STRING FullPath,
                     PWSTR *Path,
                     ULONG Attributes);

NTSTATUS FASTCALL
IntValidateWindowStationHandle(
   HWINSTA WindowStation,
   KPROCESSOR_MODE AccessMode,
   ACCESS_MASK DesiredAccess,
   PWINSTATION_OBJECT *Object);

BOOL FASTCALL
IntGetWindowStationObject(PWINSTATION_OBJECT Object);

BOOL FASTCALL
coUserInitializeDesktopGraphics(VOID);

VOID FASTCALL
IntEndDesktopGraphics(VOID);

BOOL FASTCALL
IntGetFullWindowStationName(
   OUT PUNICODE_STRING FullName,
   IN PUNICODE_STRING WinStaName,
   IN OPTIONAL PUNICODE_STRING DesktopName);

PWINSTATION_OBJECT FASTCALL IntGetWinStaObj(VOID);


/************************* MENU.C ****************************/

PMENU_OBJECT FASTCALL
co_UserGetSystemMenu(PWINDOW_OBJECT WindowObject, BOOL bRevert, BOOL RetMenu);

inline PMENU_OBJECT FASTCALL UserGetMenuObject(HMENU hMenu);

VOID FASTCALL
UserDestroyMenu(PMENU_OBJECT MenuObject, BOOL bRecurse);

BOOL FASTCALL
IntIsMenu(HMENU hMenu);


BOOL FASTCALL
UserMenuItemInfo(
   PMENU_OBJECT Menu,
   UINT Item,
   BOOL ByPosition,
   PROSMENUITEMINFO ItemInfo,
   BOOL Set
   );


BOOL FASTCALL
UserSetMenu(
   PWINDOW_OBJECT WindowObject,
   HMENU Menu,
   BOOL *Changed);
   
BOOL FASTCALL
IntSetMenuItemRect(PMENU_OBJECT MenuObject, UINT Item, BOOL fByPos, RECT *rcRect);

BOOL FASTCALL
IntCleanupMenus(PW32PROCESS Win32Process);

NTSTATUS FASTCALL
InitMenuImpl(VOID);

NTSTATUS FASTCALL
CleanupMenuImpl(VOID);

/************************* CARET.C ****************************/

BOOL FASTCALL
co_UserShowCaret(PWINDOW_OBJECT Wnd);

BOOL FASTCALL
co_UserSetCaretPos(int X, int Y);

BOOL FASTCALL
co_UserHideCaret(PWINDOW_OBJECT Wnd);

BOOL FASTCALL
UserSwitchCaretShowing(PTHRDCARETINFO Info);

BOOL FASTCALL
co_UserDestroyCaret(PW32THREAD Win32Thread);

BOOL FASTCALL
UserSetCaretBlinkTime(UINT uMSeconds);

VOID FASTCALL
UserDrawCaret(HWND hWnd);

/************************* WINPOS.C ****************************/

UINT
FASTCALL co_WinPosArrangeIconicWindows(PWINDOW_OBJECT parent);
LRESULT FASTCALL
co_WinPosGetNonClientSize(PWINDOW_OBJECT Wnd, RECT* WindowRect, RECT* ClientRect);
UINT FASTCALL
co_WinPosGetMinMaxInfo(PWINDOW_OBJECT Window, POINT* MaxSize, POINT* MaxPos,
          POINT* MinTrack, POINT* MaxTrack);
UINT FASTCALL
co_WinPosMinMaximize(PWINDOW_OBJECT WindowObject, UINT ShowFlag, RECT* NewPos);
BOOLEAN FASTCALL
co_WinPosSetWindowPos(HWND Wnd, HWND WndInsertAfter, INT x, INT y, INT cx,
         INT cy, UINT flags);
BOOLEAN FASTCALL
co_WinPosShowWindow(PWINDOW_OBJECT Wnd, INT Cmd);
USHORT FASTCALL
co_WinPosWindowFromPoint(PWINDOW_OBJECT ScopeWin, PUSER_MESSAGE_QUEUE OnlyHitTests, POINT *WinPoint,
            PWINDOW_OBJECT* Window);
VOID FASTCALL co_WinPosActivateOtherWindow(PWINDOW_OBJECT Window);

PINTERNALPOS FASTCALL WinPosInitInternalPos(PWINDOW_OBJECT WindowObject,
                                            POINT *pt, PRECT RestoreRect);

BOOL FASTCALL
UserGetClientOrigin(PWINDOW_OBJECT hWnd, LPPOINT Point);

/************************* SCROLLBAR.C ****************************/

DWORD FASTCALL
co_UserShowScrollBar(PWINDOW_OBJECT Wnd, int wBar, DWORD bShow);

#define IntGetScrollbarInfoFromWindow(Window, i) \
  ((PSCROLLBARINFO)(&((Window)->Scroll + i)->ScrollBarInfo))

#define IntGetScrollInfoFromWindow(Window, i) \
  ((LPSCROLLINFO)(&((Window)->Scroll + i)->ScrollInfo))

#define SBOBJ_TO_SBID(Obj) ((Obj) - OBJID_HSCROLL)
#define SBID_IS_VALID(id)  (id == SB_HORZ || id == SB_VERT || id == SB_CTL)

BOOL FASTCALL co_IntCreateScrollBars(PWINDOW_OBJECT Window);
BOOL FASTCALL IntDestroyScrollBars(PWINDOW_OBJECT Window);


/************************* TIMER.C ****************************/

extern KTIMER         gTimerWorkTimer;

VOID FASTCALL TimerWork();

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

PHOOK FASTCALL UserCreateHookObject();

PHOOK FASTCALL UserGetHookObject(HHOOK hHook);
LRESULT FASTCALL co_HOOK_CallHooks(INT HookId, INT Code, WPARAM wParam, LPARAM lParam);
VOID FASTCALL HOOK_DestroyThreadHooks(PETHREAD Thread);


/********************* CLASS.C *****************/


VOID FASTCALL
ClassReferenceClass(PWNDCLASS_OBJECT Class);

VOID FASTCALL
UserDereferenceClass(PWNDCLASS_OBJECT Class);

PWNDCLASS_OBJECT FASTCALL 
ClassCreateClass(DWORD bytes);



/********************* GUICHECK.C *****************/

BOOL FASTCALL IntCreatePrimarySurface();
VOID FASTCALL IntDestroyPrimarySurface();

NTSTATUS FASTCALL InitGuiCheckImpl (VOID);

BOOL FASTCALL
UserGraphicsCheck(BOOL Create);

/********************* CURSORIICON.C *****************/

HCURSOR FASTCALL UserSetCursor(PCURICON_OBJECT NewCursor, BOOL ForceChange);
BOOL FASTCALL UserSetupCurIconHandles(PWINSTATION_OBJECT WinStaObject);
PCURICON_OBJECT FASTCALL UserCreateCurIconHandle(PWINSTATION_OBJECT WinStaObject);
VOID FASTCALL UserCleanupCurIcons(struct _EPROCESS *Process, PW32PROCESS Win32Process);

BOOL FASTCALL 
UserGetCursorLocation(PWINSTATION_OBJECT WinStaObject, POINT *loc);

#define UserGetSysCursorInfo(WinStaObj) \
  (PSYSTEM_CURSORINFO)((WinStaObj)->SystemCursor)

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

PMONITOR_OBJECT FASTCALL UserCreateMonitorObject();
VOID FASTCALL UserDestroyMonitorObject(IN PMONITOR_OBJECT pMonitor);

PMONITOR_OBJECT UserGetMonitorObject(HMONITOR hMon);

NTSTATUS InitMonitorImpl();
NTSTATUS CleanupMonitorImpl();

NTSTATUS UserAttachMonitor(GDIDEVICE *pGdiDevice, ULONG DisplayNumber);
NTSTATUS UserDetachMonitor(GDIDEVICE *pGdiDevice);


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
co_MsqSendMessage(PUSER_MESSAGE_QUEUE MessageQueue,
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
co_MsqFindMessage(IN PUSER_MESSAGE_QUEUE MessageQueue,
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
co_MsqDispatchOneSentMessage(PUSER_MESSAGE_QUEUE MessageQueue);
NTSTATUS FASTCALL
co_MsqWaitForNewMessages(PUSER_MESSAGE_QUEUE MessageQueue, HWND WndFilter,
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
co_MsqPostKeyboardMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
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


/**************************** CLIPBOARD.C ************************/

UINT FASTCALL
IntEnumClipboardFormats(UINT format);


/**************************** VIS.C ************************/

HRGN FASTCALL
VIS_ComputeVisibleRegion(PWINDOW_OBJECT Window, BOOLEAN ClientArea,
   BOOLEAN ClipChildren, BOOLEAN ClipSiblings);

VOID FASTCALL
co_VIS_WindowLayoutChanged(PWINDOW_OBJECT Window, HRGN UncoveredRgn);


/**************************** NTUSER.C ************************/

inline PVOID FASTCALL UserAlloc(SIZE_T bytes);
inline PVOID FASTCALL UserAllocTag(SIZE_T bytes, ULONG tag);
inline PVOID FASTCALL UserAllocZero(SIZE_T bytes);
inline PVOID FASTCALL UserAllocZeroTag(SIZE_T bytes, ULONG tag);
inline VOID FASTCALL UserFree(PVOID mem);

VOID FASTCALL
DestroyThreadWindows(PW32THREAD W32Thread);

/**************************** WINDC.C ************************/

HDC FASTCALL
UserGetDCEx(PWINDOW_OBJECT Window OPTIONAL, HANDLE ClipRegion, ULONG Flags);

INT FASTCALL
UserReleaseDC(PWINDOW_OBJECT Wnd, HDC hDc);

DWORD FASTCALL
UserGetWindowDC(PWINDOW_OBJECT Wnd);


/**************************** HOTKEY.C ************************/

NTSTATUS FASTCALL
InitHotKeys(PWINSTATION_OBJECT WinStaObject);

NTSTATUS FASTCALL
CleanupHotKeys(PWINSTATION_OBJECT WinStaObject);

BOOL
GetHotKey (PWINSTATION_OBJECT WinStaObject,
       UINT fsModifiers,
      UINT vk,
      struct _ETHREAD **Thread,
      HWND *hWnd,
      int *id);

VOID
UnregisterWindowHotKeys(PWINDOW_OBJECT Window);

VOID
UnregisterThreadHotKeys(struct _ETHREAD *Thread);

#define IntLockHotKeys(WinStaObject) \
  ExAcquireFastMutex(&WinStaObject->HotKeyListLock)

#define IntUnLockHotKeys(WinStaObject) \
  ExReleaseFastMutex(&WinStaObject->HotKeyListLock)




#endif /* _WIN32K_USERFUNCS_H */
