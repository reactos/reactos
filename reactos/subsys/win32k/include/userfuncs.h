#ifndef _WIN32K_USERFUNCS_H
#define _WIN32K_USERFUNCS_H

//currently unused
#define ASSERT_REFS(obj) ASSERT(ObmGetReferenceCount(obj) >= 2)

#define UserReferenceWindowObjectCo(o) IntReferenceWindowObject(o)
#define UserDereferenceWindowObjectCo(o) IntReleaseWindowObject(o)

#define UserReferenceAccelObjectCo(o) IntReferenceWindowObject(o)
#define UserDereferenceAccelObjectCo(o) IntReleaseWindowObject(o)

extern PUSER_HANDLE_TABLE gHandleTable;


/*************** WINSTA.C ***************/

HWINSTA FASTCALL UserGetProcessWindowStation(VOID);

/*************** INPUT.C ***************/

NTSTATUS FASTCALL
UserAcquireOrReleaseInputOwnership(BOOLEAN Release);

/*************** WINPOS.C ***************/

BOOL FASTCALL
UserGetClientOrigin(HWND hWnd, LPPOINT Point);

/*************** FOCUS.C ***************/

HWND FASTCALL UserGetActiveWindow();

HWND FASTCALL UserGetForegroundWindow(VOID);

HWND FASTCALL UserSetFocus(HWND hWnd);

/*************** WINDC.C ***************/

INT FASTCALL
UserReleaseDC(PWINDOW_OBJECT Window, HDC hDc);

HDC FASTCALL
UserGetDCEx(PWINDOW_OBJECT Window OPTIONAL, HANDLE ClipRegion, ULONG Flags);

DWORD FASTCALL
UserGetWindowDC(PWINDOW_OBJECT Wnd);

/*************** METRIC.C ***************/

ULONG FASTCALL
UserGetSystemMetrics(ULONG Index);

/*************** KEYBOARD.C ***************/

DWORD FASTCALL UserGetKeyState(DWORD key);

DWORD FASTCALL UserGetKeyboardType(DWORD TypeFlag);

HKL FASTCALL UserGetKeyboardLayout(DWORD dwThreadId);


/*************** MISC.C ***************/

BOOL FASTCALL
UserSystemParametersInfo(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni);
  
/*************** MESSAGE.C ***************/
  
BOOL FASTCALL
UserPostMessage(HWND Wnd,
        UINT Msg,
        WPARAM wParam,
        LPARAM lParam); 



/*************** PAINTING.C ***************/

BOOL FASTCALL UserValidateRgn(HWND hWnd, HRGN hRgn);


/*************** WINDOW.C ***************/

PWINDOW_OBJECT FASTCALL UserGetWindowObject(HWND hWnd);

VOID FASTCALL
co_DestroyThreadWindows(struct _ETHREAD *Thread);

HWND FASTCALL UserGetShellWindow();

HWND FASTCALL UserSetParent(HWND hWndChild, HWND hWndNewParent);

HWND FASTCALL UserGetWindow(HWND hWnd, UINT Relationship);

HDC FASTCALL
UserGetDCEx(PWINDOW_OBJECT Window OPTIONAL, HANDLE ClipRegion, ULONG Flags);

BOOLEAN FASTCALL co_UserDestroyWindow(PWINDOW_OBJECT Wnd);

LONG FASTCALL UserGetWindowLong(HWND hWnd, DWORD Index, BOOL Ansi);

HWND FASTCALL UserGetAncestor(HWND hWnd, UINT Type);

/*************** MENU.C ***************/

HMENU FASTCALL UserCreateMenu(BOOL PopupMenu);

BOOL FASTCALL
UserSetMenuDefaultItem(
  PMENU_OBJECT Menu,
  UINT uItem,
  UINT fByPos);

BOOL FASTCALL UserDestroyMenu(HMENU hMenu);



 
 
/*************** SCROLLBAR.C ***************/
 
DWORD FASTCALL
co_UserShowScrollBar(PWINDOW_OBJECT Window, int wBar, DWORD bShow);

 
#endif /* _WIN32K_USERFUNCS_H */

/* EOF */
