#include <windows.h>
#ifdef WIN95

HKL
STDCALL
ActivateKeyboardLayout(
		       HKL hkl,
		       UINT Flags) { return 0; }
#else
WINBOOL
STDCALL
ActivateKeyboardLayout(
		       HKL hkl,
		       UINT Flags) { return 0; }
#endif /* WIN95 */

int
STDCALL
ToUnicode(
	  UINT wVirtKey,
	  UINT wScanCode,
	  PBYTE lpKeyState,
	  LPWSTR pwszBuff,
	  int cchBuff,
	  UINT wFlags) { return 0; }
 
int
STDCALL
ToUnicodeEx(
	    UINT wVirtKey,
	    UINT wScanCode,
	    PBYTE lpKeyState,
	    LPWSTR pwszBuff,
	    int cchBuff,
	    UINT wFlags,
	    HKL dwhkl) { return 0; }

WINBOOL
STDCALL
OpenIcon(
	 HWND hWnd) { return FALSE; } 

WINBOOL
STDCALL
CloseWindow(
	    HWND hWnd) { return FALSE; }


WINBOOL
STDCALL
RegisterHotKey(
	       HWND hWnd ,
	       int anID,
	       UINT fsModifiers,
	       UINT vk) { return FALSE; }



 
WINBOOL
STDCALL
UnregisterHotKey(
		 HWND hWnd,
		 int anID) { return FALSE; }

WINBOOL
STDCALL
ExitWindowsEx(
	      UINT uFlags,
	      DWORD dwReserved) { return TRUE; }

WORD
STDCALL
GetWindowWord(
	      HWND hWnd,
	      int nIndex) { return 0; }

WINBOOL
STDCALL
LockWindowUpdate(
		 HWND hWndLock) { return FALSE; }

WINBOOL
STDCALL
UnloadKeyboardLayout(
		     HKL hkl) { return 0; }

 
int
STDCALL
GetKeyboardLayoutList(
		      int nBuff,
		      HKL *lpList) { return 0; }



WINBOOL
STDCALL
SetWindowContextHelpId(HWND hWnd, DWORD Id) { return FALSE; }

DWORD
STDCALL
GetWindowContextHelpId(HWND hWnd) { return 0; } 
HKL
STDCALL
GetKeyboardLayout(
		  DWORD dwLayout
		  ) { return 0; }

 


 
WINBOOL
STDCALL
SetUserObjectSecurity(
		      HANDLE hObj,
		      PSECURITY_INFORMATION pSIRequested,
		      PSECURITY_DESCRIPTOR pSID) { return 0; }

 
WINBOOL
STDCALL
GetUserObjectSecurity(
		      HANDLE hObj,
		      PSECURITY_INFORMATION pSIRequested,
		      PSECURITY_DESCRIPTOR pSID,
		      DWORD nLength,
		      LPDWORD lpnLengthNeeded) { return 0; }

 

#if 0
WINBOOL
STDCALL
SetMessageQueue(
		int cMessagesMax) { return 0; }

 

 
WINBOOL
STDCALL
ExitWindowsEx(
	      UINT uFlags,
	      DWORD dwReserved) { return 0; }

#endif

WINBOOL
STDCALL
SwapMouseButton(
		WINBOOL fSwap) { return 0; }

#if 0 
DWORD
STDCALL
GetMessagePos(
	      VOID) { return 0; }

 
LONG
STDCALL
GetMessageTime(
	       VOID) { return 0; }

 
LONG
STDCALL
GetMessageExtraInfo(
		    VOID) { return 0; }

 
LPARAM
STDCALL
SetMessageExtraInfo(
		    LPARAM lParam) { return 0; }

 
long  
STDCALL  
BroadcastSystemMessage(
		       DWORD x, 
		       LPDWORD y, 
		       UINT z, 
		       WPARAM wParam, 
		       LPARAM lParam) { return 0; }

#endif
WINBOOL
STDCALL
AttachThreadInput(
		  DWORD idAttach,
		  DWORD idAttachTo,
		  WINBOOL fAttach) { return 0; }

#if 0
WINBOOL
STDCALL
ReplyMessage(
	     LRESULT lResult) { return 0; }

 
WINBOOL
STDCALL
WaitMessage(
	    VOID) { return 0; }

 
DWORD
STDCALL
WaitForInputIdle(
		 HANDLE hProcess,
		 DWORD dwMilliseconds) { return 0; }


WINBOOL
STDCALL
InSendMessage(
	      VOID) { return 0; }

 
 
WINBOOL
STDCALL
IsMenu(
       HMENU hMenu) { return 0; }

 

#endif
 

 
HDWP
STDCALL
BeginDeferWindowPos(
		    int nNumWindows) { return 0; }

 
HDWP
STDCALL
DeferWindowPos(
	       HDWP hWinPosInfo,
	       HWND hWnd,
	       HWND hWndInsertAfter,
	       int x,
	       int y,
	       int cx,
	       int cy,
	       UINT uFlags) { return 0; }

 
WINBOOL
STDCALL
EndDeferWindowPos(
		  HDWP hWinPosInfo) { return 0; }


 
WINBOOL
STDCALL
AnyPopup(
	 VOID) { return 0; }


WINBOOL
STDCALL
ShowOwnedPopups(
		HWND hWnd,
		WINBOOL fShow)
{
	return FALSE;
}


#if 0

 
WINBOOL
STDCALL
SetDlgItemInt(
	      HWND hDlg,
	      int nIDDlgItem,
	      UINT uValue,
	      WINBOOL bSigned) { return 0; }

 
UINT
STDCALL
GetDlgItemInt(
	      HWND hDlg,
	      int nIDDlgItem,
	      WINBOOL *lpTranslated,
	      WINBOOL bSigned) { return 0; }

 
WINBOOL
STDCALL
CheckDlgButton(
	       HWND hDlg,
	       int nIDButton,
	       UINT uCheck) { return 0; }

 
WINBOOL
STDCALL
CheckRadioButton(
		 HWND hDlg,
		 int nIDFirstButton,
		 int nIDLastButton,
		 int nIDCheckButton) { return 0; }

 
UINT
STDCALL
IsDlgButtonChecked(
		   HWND hDlg,
		   int nIDButton) { return 0; }

 
HWND
STDCALL
GetNextDlgGroupItem(
		    HWND hDlg,
		    HWND hCtl,
		    WINBOOL bPrevious) { return 0; }

 
HWND
STDCALL
GetNextDlgTabItem(
		  HWND hDlg,
		  HWND hCtl,
		  WINBOOL bPrevious) { return 0; }

 
int
STDCALL
GetDlgCtrlID(
	     HWND hWnd) { return 0; }

 
long
STDCALL
GetDialogBaseUnits(VOID) { return 0; }

#endif


 
DWORD
STDCALL
OemKeyScan(
	   WORD wOemChar) { return 0; }

 
VOID
STDCALL
keybd_event(
	    BYTE bVk,
	    BYTE bScan,
	    DWORD dwFlags,
	    DWORD dwExtraInfo) { return; }

 
VOID
STDCALL
mouse_event(
	    DWORD dwFlags,
	    DWORD dx,
	    DWORD dy,
	    DWORD cButtons,
	    DWORD dwExtraInfo) { return; }

#if 0
WINBOOL
STDCALL
GetInputState(
	      VOID) { return 0; }

 
DWORD
STDCALL
GetQueueStatus(
	       UINT flags) { return 0; }

 
HWND
STDCALL
GetCapture(
	   VOID) { return 0; }

 
HWND
STDCALL
SetCapture(
	   HWND hWnd) { return 0; }

 
WINBOOL
STDCALL
ReleaseCapture(
	       VOID) { return 0; }

 
DWORD
STDCALL
MsgWaitForMultipleObjects(
			  DWORD nCount,
			  LPHANDLE pHandles,
			  WINBOOL fWaitAll,
			  DWORD dwMilliseconds,
			  DWORD dwWakeMask) { return 0; }

 
UINT
STDCALL
SetTimer(
	 HWND hWnd ,
	 UINT nIDEvent,
	 UINT uElapse,
	 TIMERPROC lpTimerFunc) { return 0; }

 
WINBOOL
STDCALL
KillTimer(
	  HWND hWnd,
	  UINT uIDEvent) { return 0; }


#endif
WINBOOL
STDCALL
DestroyAcceleratorTable(
			HACCEL hAccel) { return 0; }

#if 0
HMENU
STDCALL
GetMenu(
	HWND hWnd) { return 0; }

 
 
WINBOOL
STDCALL
HiliteMenuItem(
	       HWND hWnd,
	       HMENU hMenu,
	       UINT uIDHiliteItem,
	       UINT uHilite) { return 0; }

 
UINT
STDCALL
GetMenuState(
	     HMENU hMenu,
	     UINT uId,
	     UINT uFlags) { return 0; }

 
WINBOOL
STDCALL
DrawMenuBar(
	    HWND hWnd) { return 0; }

 
HMENU
STDCALL
GetSystemMenu(
	      HWND hWnd,
	      WINBOOL bRevert) { return 0; }

 
HMENU
STDCALL
CreateMenu(
	   VOID) { return 0; }

 
HMENU
STDCALL
CreatePopupMenu(
		VOID) { return 0; }

 
WINBOOL
STDCALL
DestroyMenu(
	    HMENU hMenu) { return 0; }

 
DWORD
STDCALL
CheckMenuItem(
	      HMENU hMenu,
	      UINT uIDCheckItem,
	      UINT uCheck) { return 0; }

 
WINBOOL
STDCALL
EnableMenuItem(
	       HMENU hMenu,
	       UINT uIDEnableItem,
	       UINT uEnable) { return 0; }

 
HMENU
STDCALL
GetSubMenu(
	   HMENU hMenu,
	   int nPos) { return 0; }

 
UINT
STDCALL
GetMenuItemID(
	      HMENU hMenu,
	      int nPos) { return 0; }

 
int
STDCALL
GetMenuItemCount(
		 HMENU hMenu) { return 0; }

WINBOOL
STDCALL RemoveMenu(
		   HMENU hMenu,
		   UINT uPosition,
		   UINT uFlags) { return 0; }

 
WINBOOL
STDCALL
DeleteMenu(
	   HMENU hMenu,
	   UINT uPosition,
	   UINT uFlags) { return 0; }

 
WINBOOL
STDCALL
SetMenuItemBitmaps(
		   HMENU hMenu,
		   UINT uPosition,
		   UINT uFlags,
		   HBITMAP hBitmapUnchecked,
		   HBITMAP hBitmapChecked) { return 0; }

 
LONG
STDCALL
GetMenuCheckMarkDimensions(
			   VOID) { return 0; }

 
WINBOOL
STDCALL
TrackPopupMenu(
	       HMENU hMenu,
	       UINT uFlags,
	       int x,
	       int y,
	       int nReserved,
	       HWND hWnd,
	       CONST RECT *prcRect) { return 0; }

UINT
STDCALL
GetMenuDefaultItem(
		   HMENU hMenu, 
		   UINT fByPos, 
		   UINT gmdiFlags) { return 0; }

WINBOOL
STDCALL
SetMenuDefaultItem(
		   HMENU hMenu, 
		   UINT uItem, 
		   UINT fByPos) { return 0; }

WINBOOL
STDCALL
GetMenuItemRect(HWND hWnd, 
		HMENU hMenu, 
		UINT uItem, 
		LPRECT lprcItem) { return 0; }

#endif

int
STDCALL
MenuItemFromPoint(HWND hWnd, 
		  HMENU hMenu, 
		  POINT ptScreen) { return 0; }

 
DWORD
STDCALL
DragObject(HWND hWndFrom, HWND hWndTo, UINT u, DWORD d, HCURSOR c) { return 0; }

 
WINBOOL
STDCALL
DragDetect(HWND hwnd, 
	   POINT pt) { return 0; }


 
HWND
STDCALL
GetForegroundWindow(
		    VOID) { return 0; }



WINBOOL
STDCALL
PaintDesktop(HDC hdc) { return 0; }

HWND
STDCALL
WindowFromDC(
	     HDC hDC){ return 0; }



#if 0  
HDC
STDCALL
GetDC(
      HWND hWnd){ return 0; }

 
HDC
STDCALL
GetDCEx(
	HWND hWnd ,
	HRGN hrgnClip,
	DWORD flags){ return 0; }



 
int
STDCALL
ReleaseDC(
	  HWND hWnd,
	  HDC hDC){ return 0; }
 

HDC
STDCALL
BeginPaint(
	   HWND hWnd,
	   LPPAINTSTRUCT lpPaint) { return 0; }

 
WINBOOL
STDCALL
EndPaint(
	 HWND hWnd,
	 CONST PAINTSTRUCT *lpPaint) { return 0; }

WINBOOL
STDCALL
SetForegroundWindow(
		    HWND hWnd) { return 0; }

 



 
WINBOOL
STDCALL
GetUpdateRect(
	      HWND hWnd,
	      LPRECT lpRect,
	      WINBOOL bErase) { return 0; }

 
int
STDCALL
GetUpdateRgn(
	     HWND hWnd,
	     HRGN hRgn,
	     WINBOOL bErase) { return 0; }

 
int
STDCALL
SetWindowRgn(
	     HWND hWnd,
	     HRGN hRgn,
	     WINBOOL bRedraw) { return 0; }

 
int
STDCALL
GetWindowRgn(
	     HWND hWnd,
	     HRGN hRgn) { return 0; }

 
int
STDCALL
ExcludeUpdateRgn(
		 HDC hDC,
		 HWND hWnd) { return 0; }

 
WINBOOL
STDCALL
InvalidateRect(
	       HWND hWnd ,
	       CONST RECT *lpRect,
	       WINBOOL bErase) { return 0; }

 
WINBOOL
STDCALL
ValidateRect(
	     HWND hWnd ,
	     CONST RECT *lpRect) { return 0; }

 
WINBOOL
STDCALL
InvalidateRgn(
	      HWND hWnd,
	      HRGN hRgn,
	      WINBOOL bErase) { return 0; }

 
WINBOOL
STDCALL
ValidateRgn(
	    HWND hWnd,
	    HRGN hRgn) { return 0; }

 


 
WINBOOL
STDCALL
LockWindowUpdate(
		 HWND hWndLock) { return 0; }

 
WINBOOL
STDCALL
ScrollWindow(
	     HWND hWnd,
	     int XAmount,
	     int YAmount,
	     CONST RECT *lpRect,
	     CONST RECT *lpClipRect) { return 0; }

 
WINBOOL
STDCALL
ScrollDC(
	 HDC hDC,
	 int dx,
	 int dy,
	 CONST RECT *lprcScroll,
	 CONST RECT *lprcClip ,
	 HRGN hrgnUpdate,
	 LPRECT lprcUpdate) { return 0; }

 
int
STDCALL
ScrollWindowEx(
	       HWND hWnd,
	       int dx,
	       int dy,
	       CONST RECT *prcScroll,
	       CONST RECT *prcClip ,
	       HRGN hrgnUpdate,
	       LPRECT prcUpdate,
	       UINT flags) { return 0; }

 
int
STDCALL
SetScrollPos(
	     HWND hWnd,
	     int nBar,
	     int nPos,
	     WINBOOL bRedraw) { return 0; }

 
int
STDCALL
GetScrollPos(
	     HWND hWnd,
	     int nBar) { return 0; }

 
WINBOOL
STDCALL
SetScrollRange(
	       HWND hWnd,
	       int nBar,
	       int nMinPos,
	       int nMaxPos,
	       WINBOOL bRedraw) { return 0; }

 
WINBOOL
STDCALL
GetScrollRange(
	       HWND hWnd,
	       int nBar,
	       LPINT lpMinPos,
	       LPINT lpMaxPos) { return 0; }

 
WINBOOL
STDCALL
ShowScrollBar(
	      HWND hWnd,
	      int wBar,
	      WINBOOL bShow) { return 0; }

 
WINBOOL
STDCALL
EnableScrollBar(
		HWND hWnd,
		UINT wSBflags,
		UINT wArrows) { return 0; }




 

WINBOOL
STDCALL
SetWindowContextHelpId(HWND hWnd, DWORD dw) { return 0; }

DWORD
STDCALL
GetWindowContextHelpId(HWND hWnd ) { return 0; }

WINBOOL
STDCALL
SetMenuContextHelpId(HMENU hMenu, DWORD dw) { return 0; }

DWORD
STDCALL
GetMenuContextHelpId(HMENU hMenu) { return 0; }

 
#endif

WINBOOL
STDCALL
GetCursorPos(
	     LPPOINT lpPoint) { return 0; }

 
WINBOOL
STDCALL
ClipCursor(
	   CONST RECT *lpRect) { return 0; }

 
WINBOOL
STDCALL
GetClipCursor(
	      LPRECT lpRect) { return 0; }

 
HCURSOR
STDCALL
GetCursor(
	  VOID) { return 0; }

#if 0
 
WINBOOL
STDCALL
ClientToScreen(
	       HWND hWnd,
	       LPPOINT lpPoint) { return 0; }

 
WINBOOL
STDCALL
ScreenToClient(
	       HWND hWnd,
	       LPPOINT lpPoint) { return 0; }

 

 
HWND
STDCALL
WindowFromPoint(
		POINT Point) { return 0; }

 
HWND
STDCALL
ChildWindowFromPoint(
		     HWND hWndParent,
		     POINT Point) { return 0; }

 

 
WORD
STDCALL
GetClassWord(
	     HWND hWnd,
	     int nIndex) { return 0; }

 
WORD
STDCALL
SetClassWord(
	     HWND hWnd,
	     int nIndex,
	     WORD wNewWord) { return 0; }

#endif
 
WINBOOL
STDCALL
EnumChildWindows(
		 HWND hWndParent,
		 ENUMWINDOWSPROC lpEnumFunc,
		 LPARAM lParam) { return 0; }

 
WINBOOL
STDCALL
EnumWindows(
	    ENUMWINDOWSPROC lpEnumFunc,
	    LPARAM lParam) { return 0; }

 
WINBOOL
STDCALL
EnumThreadWindows(
		  DWORD dwThreadId,
		  ENUMWINDOWSPROC lpfn,
		  LPARAM lParam) { return 0; }



HWND
STDCALL
GetLastActivePopup(
		   HWND hWnd) { return 0; }

 
#if 0

WINBOOL
STDCALL
UnhookWindowsHook(
		  int nCode,
		  HOOKPROC pfnFilterProc) { return 0; }

WINBOOL
STDCALL
UnhookWindowsHookEx(
		    HHOOK hhk) { return 0; }

 
LRESULT
STDCALL
CallNextHookEx(
	       HHOOK hhk,
	       int nCode,
	       WPARAM wParam,
	       LPARAM lParam) { return 0; }

 
WINBOOL
STDCALL
CheckMenuRadioItem(HMENU hMenu, UINT i, UINT j, UINT k, UINT l) { return 0; }

#endif
HCURSOR
STDCALL
CreateCursor(
	     HINSTANCE hInst,
	     int xHotSpot,
	     int yHotSpot,
	     int nWidth,
	     int nHeight,
	     CONST VOID *pvANDPlane,
	     CONST VOID *pvXORPlane) { return 0; }

 

 
WINBOOL
STDCALL
SetSystemCursor(
		HCURSOR hcur,
		DWORD   anID) { return 0; }

 


 
int
STDCALL
LookupIconIdFromDirectory(
			  PBYTE presbits,
			  WINBOOL fIcon) { return 0; }

 
int
STDCALL
LookupIconIdFromDirectoryEx(
			    PBYTE presbits,
			    WINBOOL  fIcon,
			    int   cxDesired,
			    int   cyDesired,
			    UINT  Flags) { return 0; }

 


 
HICON
STDCALL
CopyImage(
	  HANDLE hImage,
	  UINT u,
	  int i,
	  int j,
	  UINT k) { return 0; }

 



 


WINBOOL
STDCALL
TranslateMDISysAccel(
		     HWND hWndClient,
		     LPMSG lpMsg) { return 0; }

 
UINT
STDCALL
ArrangeIconicWindows(
		     HWND hWnd) { return 0; }

WORD
STDCALL
TileWindows(HWND hwndParent, UINT wHow, CONST RECT * lpRect, UINT cKids, const HWND *lpKids) { return 0; }

WORD
STDCALL
CascadeWindows(HWND hwndParent, UINT wHow, CONST RECT * lpRect, UINT cKids,  const HWND *lpKids) { return 0; }

 


VOID
STDCALL
SetLastErrorEx(
	       DWORD dwErrCode,
	       DWORD dwType
	       ) { return; }

 
VOID
STDCALL
SetDebugErrorLevel(
		   DWORD dwLevel
		   ) { return; }



WINBOOL
STDCALL
CalcChildScroll(HANDLE hWnd, DWORD x) { return 0; }

void WINAPI CascadeChildWindows( HWND parent, WORD action )
{
   return;
}

#if 0
WINBOOL
STDCALL
DrawEdge(HDC hdc, LPRECT qrc, UINT edge, UINT grfFlags) { return 0; }

#endif



WINBOOL
STDCALL
DrawFrameControl(HDC hDC, LPRECT lpRect, UINT i, UINT j) { return 0; }



WINBOOL
STDCALL
DrawCaption(HWND hWnd, HDC hDC, CONST RECT *r, UINT u) { return 0; }

WINBOOL
STDCALL
DrawAnimatedRects(HWND hwnd, int idAni, CONST RECT * lprcFrom, CONST RECT * lprcTo) { return 0; }

#if 0
WINBOOL
STDCALL
TrackPopupMenuEx(HMENU hMenu, UINT u, int i, int j, HWND hwnd, LPTPMPARAMS l) { return 0; }

HWND
STDCALL
ChildWindowFromPointEx(HWND hWnd, POINT p, UINT u) { return 0; }


#endif

#undef CallMsgFilter
WINBOOL
STDCALL
CallMsgFilter(LPMSG lpMsg, int nCode) { return 0; }

#undef TranslateAccelerator
int
STDCALL
TranslateAccelerator(
    HWND hWnd,
    HACCEL hAccTable,
    LPMSG lpMsg) { return 0; }

typedef struct _TRACKMOUSEEVENT 
{
	DWORD cbSize; 
	DWORD dwFlags; 
	HWND  hwndTrack; 
	DWORD dwHoverTime; 
} TRACKMOUSEEVENT; 
 
WINBOOL WINAPI TrackMouseEvent( TRACKMOUSEEVENT *lpEventTrack ) { return 0;}

WINBOOL  WINAPI DrawFrame(HDC hDC,LPRECT r,UINT u,UINT v) { return 0;}



#undef IsDialogMessage
WINBOOL
STDCALL
IsDialogMessage(
    HWND hDlg,
    LPMSG lpMsg) { return 0; }

HWND STDCALL SetShellWindow(HWND hwndshell) { return 0; }

HWND STDCALL GetShellWindow(void) { return 0; }

WINBOOL STDCALL KillSystemTimer(HWND hwnd, UINT id) { return FALSE; }

UINT STDCALL SetSystemTimer(HWND hwnd, UINT id, UINT timeout, TIMERPROC proc) { return 0; }

DWORD STDCALL MsgWaitForMultipleObjectsEx( 
     DWORD nCount, 
     LPHANDLE pHandles, 
     DWORD dwMilliseconds, 
     DWORD dwWakeMask, 
     DWORD dwFlags 
    ) {
	return 0;
} 
// not correct
WINBOOL 
STDCALL
ScrollChildren
(
        HWND hWnd,
 //       UINT uMsg,
        WPARAM wParam,
        LPARAM lParam
)
{
	return FALSE;
}


// not correct
VOID STDCALL TileChildWindows(HWND hwndParent, DWORD dwUnknown) { return; }



WINBOOL STDCALL
SetDeskWallpaper(LPCSTR WallPaper ) { return FALSE; }
HCONV WINAPI	DdeConnect (DWORD dw, HSZ h1, HSZ h2, CONVCONTEXT *c) { return 0; }
WINBOOL WINAPI	DdeDisconnect (HCONV h) { return 0; }
WINBOOL WINAPI	DdeFreeDataHandle (HDDEDATA h) { return 0; }
DWORD WINAPI	DdeGetData (HDDEDATA h, BYTE *b, DWORD d, DWORD w) { return 0; }
UINT WINAPI	DdeGetLastError (DWORD d) { return 0; }
HDDEDATA WINAPI	DdeNameService (DWORD d, HSZ h1, HSZ h2, UINT u) { return 0; }
WINBOOL WINAPI	DdePostAdvise (DWORD dw, HSZ h1, HSZ h2) { return 0; }
HCONV WINAPI	DdeReconnect (HCONV h) { return 0; }
WINBOOL WINAPI	DdeUninitialize (DWORD dw) { return 0; }
int WINAPI	DdeCmpStringHandles (HSZ h1, HSZ h2) { return 0; }
HDDEDATA WINAPI	DdeCreateDataHandle (DWORD dw, LPBYTE b, DWORD x, DWORD y, HSZ h, 
				UINT i, UINT j) { return 0; }
WINBOOL WINAPI DdeAbandonTransaction(DWORD i, HCONV h, DWORD tr) { return 0; } 


HDDEDATA WINAPI DdeAddData(HDDEDATA hData, LPBYTE pSrc, DWORD cb, DWORD cbOff) { return 0; } 
LPBYTE WINAPI DdeAccessData(HDDEDATA hData, LPDWORD pcbDataSize) { return 0; } 
WINBOOL WINAPI DdeUnaccessData(HDDEDATA hData) { return 0; } 

HDDEDATA WINAPI DdeClientTransaction(LPBYTE pData, DWORD cbData, 
        HCONV hConv, HSZ hszItem, UINT wFmt, UINT wType, 
        DWORD dwTimeout, LPDWORD pdwResult) { return 0; }

HCONVLIST WINAPI DdeConnectList(DWORD idInst, HSZ hszService, HSZ hszTopic, 
        HCONVLIST hConvList, PCONVCONTEXT pCC) { return 0; }
HCONV WINAPI DdeQueryNextServer(HCONVLIST hConvList, HCONV hConvPrev) { return 0; }
WINBOOL WINAPI DdeDisconnectList(HCONVLIST hConvList) { return 0; } 
 
 
WINBOOL WINAPI DdeEnableCallback(DWORD idInst, HCONV hConv, UINT wCmd) { return 0; } 
WINBOOL WINAPI DdeImpersonateClient(HCONV hConv) { return 0; } 
 
WINBOOL WINAPI DdeFreeStringHandle(DWORD idInst, HSZ hsz) { return 0; }
WINBOOL WINAPI DdeKeepStringHandle(DWORD idInst, HSZ hsz) { return 0; } 

WINBOOL WINAPI DdeGetQualityOfService(HCONV h, PVOID SecurityQualityOfService, UINT y) { return 0; }
WINBOOL WINAPI DdeSetQualityOfService(HCONV h, PVOID SecurityQualityOfService, UINT y) { return 0; }

#define PCONVINFO void*
UINT WINAPI DdeQueryConvInfo(HCONV hConv, DWORD idTransaction, PCONVINFO pConvInfo) { return 0; }
WINBOOL WINAPI DdeSetUserHandle(HCONV hConv, DWORD id, DWORD hUser) { return 0; }

WINBOOL STDCALL ImpersonateDdeClientWindow( HWND hWndClient, HWND hWndServer  )
{
	return FALSE;
}

WINBOOL WINAPI FreeDDElParam(    UINT msg, LONG lParam  ) { return 0; } 
LONG WINAPI PackDDElParam(UINT msg, UINT uiLo, UINT uiHi) { return 0; }
WINBOOL WINAPI UnpackDDElParam( UINT msg, LONG lParam, PUINT puiLo, PUINT puiHi  )  { return FALSE; }
LONG WINAPI ReuseDDElParam(LONG lParam, UINT msgIn, UINT msgOut, UINT uiLo, UINT uiHi) { return 0; }
 