/* $Id: stubs.c,v 1.2 2001/05/07 22:03:27 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/misc/stubs.c
 * PURPOSE:         User32.dll stubs
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * NOTES:           If you implement a function, remove it from this file
 * UPDATE HISTORY:
 *      08-05-2001  CSH  Created
 */
#include <windows.h>

HKL
STDCALL
ActivateKeyboardLayout(
  HKL hkl,
  UINT Flags)
{
  return (HKL)0;
}

WINBOOL
STDCALL
AdjustWindowRect(
  LPRECT lpRect,
  DWORD dwStyle,
  WINBOOL bMenu)
{
  return FALSE;
}

WINBOOL
STDCALL
AdjustWindowRectEx( 
  LPRECT lpRect, 
  DWORD dwStyle, 
  WINBOOL bMenu, 
  DWORD dwExStyle)
{
  return FALSE;
}

WINBOOL
STDCALL
AllowSetForegroundWindow(
  DWORD dwProcessId)
{
  return FALSE;
}

WINBOOL
STDCALL
AnimateWindow(
  HWND hwnd,
  DWORD dwTime,
  DWORD dwFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
AnyPopup(VOID)
{
  return FALSE;
}

WINBOOL
STDCALL
AppendMenuA(
  HMENU hMenu,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  return FALSE;
}

WINBOOL
STDCALL
AppendMenuW(
  HMENU hMenu,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  return FALSE;
}

UINT
STDCALL
ArrangeIconicWindows(
  HWND hWnd)
{
  return 0;
}

WINBOOL
STDCALL
AttachThreadInput(
  DWORD idAttach,
  DWORD idAttachTo,
  WINBOOL fAttach)
{
  return FALSE;
}

HDWP
STDCALL
BeginDeferWindowPos(
  int nNumWindows)
{
  return (HDWP)0;
}

HDC
STDCALL
BeginPaint(
  HWND hwnd,
  LPPAINTSTRUCT lpPaint)
{
  return (HDC)0;
}

WINBOOL
STDCALL
BlockInput(
  WINBOOL fBlockIt)
{
  return FALSE;
}

WINBOOL
STDCALL
BringWindowToTop(
  HWND hWnd)
{
  return FALSE;
}

long
STDCALL
BroadcastSystemMessage(
  DWORD dwFlags,
  LPDWORD lpdwRecipients,
  UINT uiMessage,
  WPARAM wParam,
  LPARAM lParam)
{
  return 0;
}

long
STDCALL
BroadcastSystemMessageA(
  DWORD dwFlags,
  LPDWORD lpdwRecipients,
  UINT uiMessage,
  WPARAM wParam,
  LPARAM lParam)
{
  return 0;
}

long
STDCALL
BroadcastSystemMessageW(
  DWORD dwFlags,
  LPDWORD lpdwRecipients,
  UINT uiMessage,
  WPARAM wParam,
  LPARAM lParam)
{
  return 0;
}

#if 0
WINBOOL
STDCALL
CallMsgFilter(
  LPMSG lpMsg,
  int nCode)
{
  return FALSE;
}
#endif

WINBOOL
STDCALL
CallMsgFilterA(
  LPMSG lpMsg,
  int nCode)
{
  return FALSE;
}

WINBOOL
STDCALL
CallMsgFilterW(
  LPMSG lpMsg,
  int nCode)
{
  return FALSE;
}

LRESULT
STDCALL
CallNextHookEx(
  HHOOK hhk,
  int nCode,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
CallWindowProcA(
  WNDPROC lpPrevWndFunc,
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
CallWindowProcW(
  WNDPROC lpPrevWndFunc,
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

WORD
STDCALL
CascadeWindows(
  HWND hwndParent,
  UINT wHow,
  CONST RECT *lpRect,
  UINT cKids,
  const HWND *lpKids)
{
  return 0;
}

WINBOOL
STDCALL
ChangeClipboardChain(
  HWND hWndRemove,
  HWND hWndNewNext)
{
  return FALSE;
}

LONG
STDCALL
ChangeDisplaySettingsA(
  LPDEVMODE lpDevMode,
  DWORD dwflags)
{
  return 0;
}

LONG
STDCALL
ChangeDisplaySettingsExA(
  LPCSTR lpszDeviceName,
  LPDEVMODE lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam)
{
  return 0;
}

LONG
STDCALL
ChangeDisplaySettingsExW(
  LPCWSTR lpszDeviceName,
  LPDEVMODE lpDevMode,
  HWND hwnd,
  DWORD dwflags,
  LPVOID lParam)
{
  return 0;
}

LONG
STDCALL
ChangeDisplaySettingsW(
  LPDEVMODE lpDevMode,
  DWORD dwflags)
{
  return 0;
}

LPSTR
STDCALL
CharLowerA(
  LPSTR lpsz)
{
  return (LPSTR)NULL;
}

DWORD
STDCALL
CharLowerBuffA(
  LPSTR lpsz,
  DWORD cchLength)
{
  return 0;
}

DWORD
STDCALL
CharLowerBuffW(
  LPWSTR lpsz,
  DWORD cchLength)
{
  return 0;
}

LPWSTR
STDCALL
CharLowerW(
  LPWSTR lpsz)
{
  return (LPWSTR)NULL;
}

LPSTR
STDCALL
CharNextA(
  LPCSTR lpsz)
{
  return (LPSTR)NULL;
}

LPSTR
STDCALL
CharNextExA(
  WORD CodePage,
  LPCSTR lpCurrentChar,
  DWORD dwFlags)
{
}

LPWSTR
STDCALL
CharNextW(
  LPCWSTR lpsz)
{
  return (LPWSTR)NULL;
}

LPSTR
STDCALL
CharPrevA(
  LPCSTR lpszStart,
  LPCSTR lpszCurrent)
{
  return (LPSTR)NULL;
}

LPWSTR
STDCALL
CharPrevW(
  LPCWSTR lpszStart,
  LPCWSTR lpszCurrent)
{
  return (LPWSTR)NULL;
}

LPSTR
STDCALL
CharPrevExA(
  WORD CodePage,
  LPCSTR lpStart,
  LPCSTR lpCurrentChar,
  DWORD dwFlags)
{
  return (LPSTR)NULL;
}

WINBOOL
STDCALL
CharToOemA(
  LPCSTR lpszSrc,
  LPSTR lpszDst)
{
  return FALSE;
}

WINBOOL
STDCALL
CharToOemBuffA(
  LPCSTR lpszSrc,
  LPSTR lpszDst,
  DWORD cchDstLength)
{
  return FALSE;
}

WINBOOL
STDCALL
CharToOemBuffW(
  LPCWSTR lpszSrc,
  LPSTR lpszDst,
  DWORD cchDstLength)
{
  return FALSE;
}

WINBOOL
STDCALL
CharToOemW(
  LPCWSTR lpszSrc,
  LPSTR lpszDst)
{
  return FALSE;
}

LPSTR
STDCALL
CharUpperA(
  LPSTR lpsz)
{
  return (LPSTR)NULL;
}

DWORD
STDCALL
CharUpperBuffA(
  LPSTR lpsz,
  DWORD cchLength)
{
  return 0;
}

DWORD
STDCALL
CharUpperBuffW(
  LPWSTR lpsz,
  DWORD cchLength)
{
  return 0;
}

LPWSTR
STDCALL
CharUpperW(
  LPWSTR lpsz)
{
  return (LPWSTR)NULL;
}

WINBOOL
STDCALL
CheckDlgButton(
  HWND hDlg,
  int nIDButton,
  UINT uCheck)
{
  return FALSE;
}

DWORD
STDCALL
CheckMenuItem(
  HMENU hmenu,
  UINT uIDCheckItem,
  UINT uCheck)
{
  return 0;
}

WINBOOL
STDCALL
CheckMenuRadioItem(
  HMENU hmenu,
  UINT idFirst,
  UINT idLast,
  UINT idCheck,
  UINT uFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
CheckRadioButton(
  HWND hDlg,
  int nIDFirstButton,
  int nIDLastButton,
  int nIDCheckButton)
{
  return FALSE;
}

HWND
STDCALL
ChildWindowFromPoint(
  HWND hWndParent,
  POINT Point)
{
  return (HWND)0;
}

HWND
STDCALL
ChildWindowFromPointEx(
  HWND hwndParent,
  POINT pt,
  UINT uFlags)
{
  return (HWND)0;
}

WINBOOL
STDCALL
ClientToScreen(
  HWND hWnd,
  LPPOINT lpPoint)
{
  return FALSE;
}

WINBOOL
STDCALL
ClipCursor(
  CONST RECT *lpRect)
{
  return FALSE;
}

WINBOOL
STDCALL
CloseClipboard(VOID)
{
  return FALSE;
}

WINBOOL
STDCALL
CloseDesktop(
  HDESK hDesktop)
{
  return FALSE;
}

WINBOOL
STDCALL
CloseWindow(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
CloseWindowStation(
  HWINSTA hWinSta)
{
  return FALSE;
}

int
STDCALL
CopyAcceleratorTableA(
  HACCEL hAccelSrc,
  LPACCEL lpAccelDst,
  int cAccelEntries)
{
  return 0;
}

int
STDCALL
CopyAcceleratorTableW(
  HACCEL hAccelSrc,
  LPACCEL lpAccelDst,
  int cAccelEntries)
{
  return 0;
}

HICON
STDCALL
CopyIcon(
  HICON hIcon)
{
  return (HICON)0;
}

HANDLE
STDCALL
CopyImage(
  HANDLE hImage,
  UINT uType,
  int cxDesired,
  int cyDesired,
  UINT fuFlags)
{
  return (HANDLE)0;
}

WINBOOL
STDCALL
CopyRect(
  LPRECT lprcDst,
  CONST RECT *lprcSrc)
{
  return FALSE;
}

int
STDCALL
CountClipboardFormats(VOID)
{
  return 0;
}

HACCEL
STDCALL
CreateAcceleratorTableA(
  LPACCEL lpaccl,
  int cEntries)
{
  return (HACCEL)0;
}

HACCEL
STDCALL
CreateAcceleratorTableW(
  LPACCEL lpaccl,
  int cEntries)
{
  return (HACCEL)0;
}

WINBOOL
STDCALL
CreateCaret(
  HWND hWnd,
  HBITMAP hBitmap,
  int nWidth,
  int nHeight)
{
  return FALSE;
}

HCURSOR
STDCALL
CreateCursor(
  HINSTANCE hInst,
  int xHotSpot,
  int yHotSpot,
  int nWidth,
  int nHeight,
  CONST VOID *pvANDPlane,
  CONST VOID *pvXORPlane)
{
  return (HCURSOR)0;
}

HDESK
STDCALL
CreateDesktopA(
  LPCSTR lpszDesktop,
  LPCSTR lpszDevice,
  LPDEVMODE pDevmode,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpsa)
{
  return (HDESK)0;
}

HDESK
STDCALL
CreateDesktopW(
  LPCWSTR lpszDesktop,
  LPCWSTR lpszDevice,
  LPDEVMODE pDevmode,
  DWORD dwFlags,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpsa)
{
  return (HDESK)0;
}

HWND
STDCALL
CreateDialogIndirectParamA(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
  return (HWND)0;
}

HWND
STDCALL
CreateDialogIndirectParamAorW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
  return (HWND)0;
}

HWND
STDCALL
CreateDialogIndirectParamW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE lpTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM lParamInit)
{
  return (HWND)0;
}

HWND
STDCALL
CreateDialogParamA(
  HINSTANCE hInstance,
  LPCSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (HWND)0;
}

HWND
STDCALL
CreateDialogParamW(
  HINSTANCE hInstance,
  LPCWSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (HWND)0;
}

HICON
STDCALL
CreateIcon(
  HINSTANCE hInstance,
  int nWidth,
  int nHeight,
  BYTE cPlanes,
  BYTE cBitsPixel,
  CONST BYTE *lpbANDbits,
  CONST BYTE *lpbXORbits)
{
  return (HICON)0;
}

HICON
STDCALL
CreateIconFromResource(
  PBYTE presbits,
  DWORD dwResSize,
  WINBOOL fIcon,
  DWORD dwVer)
{
  return (HICON)0;
}

HICON
STDCALL
CreateIconFromResourceEx(
  PBYTE pbIconBits,
  DWORD cbIconBits,
  WINBOOL fIcon,
  DWORD dwVersion,
  int cxDesired,
  int cyDesired,
  UINT uFlags)
{
  return (HICON)0;
}

HICON
STDCALL
CreateIconIndirect(
  PICONINFO piconinfo)
{
  return (HICON)0;
}

HWND
STDCALL
CreateMDIWindowA(
  LPCSTR lpClassName,
  LPCSTR lpWindowName,
  DWORD dwStyle,
  int X,
  int Y,
  int nWidth,
  int nHeight,
  HWND hWndParent,
  HINSTANCE hInstance,
  LPARAM lParam)
{
  return (HWND)0;
}

HWND
STDCALL
CreateMDIWindowW(
  LPCWSTR lpClassName,
  LPCWSTR lpWindowName,
  DWORD dwStyle,
  int X,
  int Y,
  int nWidth,
  int nHeight,
  HWND hWndParent,
  HINSTANCE hInstance,
  LPARAM lParam)
{
  return (HWND)0;
}

HMENU
STDCALL
CreateMenu(VOID)
{
  return (HMENU)0;
}

HMENU
STDCALL
CreatePopupMenu(VOID)
{
  return (HMENU)0;
}

HWND
STDCALL
CreateWindowExA(
  DWORD dwExStyle,
  LPCSTR lpClassName,
  LPCSTR lpWindowName,
  DWORD dwStyle,
  int x,
  int y,
  int nWidth,
  int nHeight,
  HWND hWndParent,
  HMENU hMenu,
  HINSTANCE hInstance,
  LPVOID lpParam)
{
  return (HWND)0;
}

HWND
STDCALL
CreateWindowExW(
  DWORD dwExStyle,
  LPCWSTR lpClassName,
  LPCWSTR lpWindowName,
  DWORD dwStyle,
  int x,
  int y,
  int nWidth,
  int nHeight,
  HWND hWndParent,
  HMENU hMenu,
  HINSTANCE hInstance,
  LPVOID lpParam)
{
  return (HWND)0;
}

HWINSTA
STDCALL
CreateWindowStationA(
  LPSTR lpwinsta,
  DWORD dwReserved,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpsa)
{
  return (HWINSTA)0;
}

HWINSTA
STDCALL
CreateWindowStationW(
  LPWSTR lpwinsta,
  DWORD dwReserved,
  ACCESS_MASK dwDesiredAccess,
  LPSECURITY_ATTRIBUTES lpsa)
{
  return (HWINSTA)0;
}

WINBOOL
STDCALL
DdeAbandonTransaction(
  DWORD idInst,
  HCONV hConv,
  DWORD idTransaction)
{
  return FALSE;
}

LPBYTE
STDCALL
DdeAccessData(
  HDDEDATA hData,
  LPDWORD pcbDataSize)
{
  return (LPBYTE)0;
}

HDDEDATA
STDCALL
DdeAddData(
  HDDEDATA hData,
  LPBYTE pSrc,
  DWORD cb,
  DWORD cbOff)
{
  return (HDDEDATA)0;
}

HDDEDATA
STDCALL
DdeClientTransaction(
  LPBYTE pData,
  DWORD cbData,
  HCONV hConv,
  HSZ hszItem,
  UINT wFmt,
  UINT wType,
  DWORD dwTimeout,
  LPDWORD pdwResult)
{
  return (HDDEDATA)0;
}

int
STDCALL
DdeCmpStringHandles(
  HSZ hsz1,
  HSZ hsz2)
{
  return 0;
}

HCONV
STDCALL
DdeConnect(
  DWORD idInst,
  HSZ hszService,
  HSZ hszTopic,
  PCONVCONTEXT pCC)
{
  return (HCONV)0;
}

HCONVLIST
STDCALL
DdeConnectList(
  DWORD idInst,
  HSZ hszService,
  HSZ hszTopic,
  HCONVLIST hConvList,
  PCONVCONTEXT pCC)
{
  return (HCONVLIST)0;
}

HDDEDATA
STDCALL
DdeCreateDataHandle(
  DWORD idInst,
  LPBYTE pSrc,
  DWORD cb,
  DWORD cbOff,
  HSZ hszItem,
  UINT wFmt,
  UINT afCmd)
{
  return (HDDEDATA)0;
}

HSZ
STDCALL
DdeCreateStringHandleA(
  DWORD idInst,
  LPSTR psz,
  int iCodePage)
{
  return (HSZ)0;
}

HSZ
STDCALL
DdeCreateStringHandleW(
  DWORD idInst,
  LPWSTR psz,
  int iCodePage)
{
  return (HSZ)0;
}

WINBOOL
STDCALL
DdeDisconnect(
  HCONV hConv)
{
  return FALSE;
}

WINBOOL
STDCALL
DdeDisconnectList(
  HCONVLIST hConvList)
{
  return FALSE;
}

WINBOOL
STDCALL
DdeEnableCallback(
  DWORD idInst,
  HCONV hConv,
  UINT wCmd)
{
  return FALSE;
}

WINBOOL
STDCALL
DdeFreeDataHandle(
  HDDEDATA hData)
{
  return FALSE;
}

BOOL
DdeFreeStringHandle(
  DWORD idInst,
  HSZ hsz)
{
  return FALSE;
}

DWORD
STDCALL
DdeGetData(
  HDDEDATA hData,
  LPBYTE pDst,
  DWORD cbMax,
  DWORD cbOff)
{
  return 0;
}

UINT
STDCALL
DdeGetLastError(
  DWORD idInst)
{
  return 0;
}

WINBOOL
STDCALL
DdeImpersonateClient(
  HCONV hConv)
{
  return FALSE;
}

UINT
STDCALL
DdeInitializeA(
  LPDWORD pidInst,
  PFNCALLBACK pfnCallback,
  DWORD afCmd,
  DWORD ulRes)
{
  return 0;
}

UINT
STDCALL
DdeInitializeW(
  LPDWORD pidInst,
  PFNCALLBACK pfnCallback,
  DWORD afCmd,
  DWORD ulRes)
{
  return 0;
}

WINBOOL
STDCALL
DdeKeepStringHandle(
  DWORD idInst,
  HSZ hsz)
{
  return FALSE;
}

HDDEDATA
STDCALL
DdeNameService(
  DWORD idInst,
  HSZ hsz1,
  HSZ hsz2,
  UINT afCmd)
{
  return (HDDEDATA)0;
}

WINBOOL
STDCALL
DdePostAdvise(
  DWORD idInst,
  HSZ hszTopic,
  HSZ hszItem)
{
  return FALSE;
}

UINT
STDCALL
DdeQueryConvInfo(
  HCONV hConv,
  DWORD idTransaction,
  PCONVINFO pConvInfo)
{
  return 0;
}

HCONV
STDCALL
DdeQueryNextServer(
  HCONVLIST hConvList,
  HCONV hConvPrev)
{
  return (HCONV)0;
}

DWORD
STDCALL
DdeQueryStringA(
  DWORD idInst,
  HSZ hsz,
  LPSTR psz,
  DWORD cchMax,
  int iCodePage)
{
  return 0;
}

DWORD
STDCALL
DdeQueryStringW(
  DWORD idInst,
  HSZ hsz,
  LPWSTR psz,
  DWORD cchMax,
  int iCodePage)
{
  return 0;
}

HCONV
STDCALL
DdeReconnect(
  HCONV hConv)
{
  return (HCONV)0;
}

WINBOOL
STDCALL
DdeSetQualityOfService(
  HWND hwndClient,
  CONST SECURITY_QUALITY_OF_SERVICE *pqosNew,
  PSECURITY_QUALITY_OF_SERVICE pqosPrev)
{
  return FALSE;
}

WINBOOL
STDCALL
DdeSetUserHandle(
  HCONV hConv,
  DWORD id,
  DWORD_PTR hUser)
{
  return FALSE;
}

WINBOOL
STDCALL
DdeUnaccessData(
  HDDEDATA hData)
{
  return FALSE;
}

WINBOOL
STDCALL
DdeUninitialize(
  DWORD idInst)
{
  return FALSE;
}

LRESULT
STDCALL
DefDlgProcA(
  HWND hDlg,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
DefDlgProcW(
  HWND hDlg,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
DefFrameProcA(
  HWND hWnd,
  HWND hWndMDIClient,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
DefFrameProcW(
  HWND hWnd,
  HWND hWndMDIClient,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
DefMDIChildProcA(
  HWND hWnd,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
DefMDIChildProcW(
  HWND hWnd,
  UINT uMsg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
DefWindowProcA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
DefWindowProcW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

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
  UINT uFlags)
{
  return (HDWP)0;
}

WINBOOL
STDCALL
DeleteMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
DestroyAcceleratorTable(
  HACCEL hAccel)
{
  return FALSE;
}

WINBOOL
STDCALL
DestroyCaret(VOID)
{
  return FALSE;
}

WINBOOL
STDCALL
DestroyCursor(
  HCURSOR hCursor)
{
  return FALSE;
}

WINBOOL
STDCALL
DestroyIcon(
  HICON hIcon)
{
  return FALSE;
}

WINBOOL
STDCALL
DestroyMenu(
  HMENU hMenu)
{
  return FALSE;
}

WINBOOL
STDCALL
DestroyWindow(
  HWND hWnd)
{
  return FALSE;
}

INT_PTR
STDCALL
DialogBoxIndirectParamA(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (INT_PTR)NULL;
}

INT_PTR
STDCALL
DialogBoxIndirectParamAorW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (INT_PTR)NULL;
}

INT_PTR
STDCALL
DialogBoxIndirectParamW(
  HINSTANCE hInstance,
  LPCDLGTEMPLATE hDialogTemplate,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (INT_PTR)NULL;
}

INT_PTR
STDCALL
DialogBoxParamA(
  HINSTANCE hInstance,
  LPCSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (INT_PTR)0;
}

INT_PTR
STDCALL
DialogBoxParamW(
  HINSTANCE hInstance,
  LPCWSTR lpTemplateName,
  HWND hWndParent,
  DLGPROC lpDialogFunc,
  LPARAM dwInitParam)
{
  return (INT_PTR)0;
}

LRESULT
STDCALL
DispatchMessageA(
  CONST MSG *lpmsg)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
DispatchMessageW(
  CONST MSG *lpmsg)
{
  return (LRESULT)0;
}

int
STDCALL
DlgDirListA(
  HWND hDlg,
  LPSTR lpPathSpec,
  int nIDListBox,
  int nIDStaticPath,
  UINT uFileType)
{
  return 0;
}

int
STDCALL
DlgDirListComboBoxA(
  HWND hDlg,
  LPSTR lpPathSpec,
  int nIDComboBox,
  int nIDStaticPath,
  UINT uFiletype)
{
  return 0;
}

int
STDCALL
DlgDirListComboBoxW(
  HWND hDlg,
  LPWSTR lpPathSpec,
  int nIDComboBox,
  int nIDStaticPath,
  UINT uFiletype)
{
  return 0;
}

int
STDCALL
DlgDirListW(
  HWND hDlg,
  LPWSTR lpPathSpec,
  int nIDListBox,
  int nIDStaticPath,
  UINT uFileType)
{
  return 0;
}

WINBOOL
STDCALL
DlgDirSelectComboBoxExA(
  HWND hDlg,
  LPSTR lpString,
  int nCount,
  int nIDComboBox)
{
  return FALSE;
}

WINBOOL
STDCALL
DlgDirSelectComboBoxExW(
  HWND hDlg,
  LPWSTR lpString,
  int nCount,
  int nIDComboBox)
{
  return FALSE;
}

WINBOOL
STDCALL
DlgDirSelectExA(
  HWND hDlg,
  LPSTR lpString,
  int nCount,
  int nIDListBox)
{
  return FALSE;
}

WINBOOL
STDCALL
DlgDirSelectExW(
  HWND hDlg,
  LPWSTR lpString,
  int nCount,
  int nIDListBox)
{
  return FALSE;
}

WINBOOL
STDCALL
DragDetect(
  HWND hwnd,
  POINT pt)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawAnimatedRects(
  HWND hwnd,
  int idAni,
  CONST RECT *lprcFrom,
  CONST RECT *lprcTo)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawCaption(
  HWND hwnd,
  HDC hdc,
  LPRECT lprc,
  UINT uFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawEdge(
  HDC hdc,
  LPRECT qrc,
  UINT edge,
  UINT grfFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawFocusRect(
  HDC hDC,
  CONST RECT *lprc)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawIcon(
  HDC hDC,
  int X,
  int Y,
  HICON hIcon)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawIconEx(
  HDC hdc,
  int xLeft,
  int yTop,
  HICON hIcon,
  int cxWidth,
  int cyWidth,
  UINT istepIfAniCur,
  HBRUSH hbrFlickerFreeDraw,
  UINT diFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawMenuBar(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawStateA(
  HDC hdc,
  HBRUSH hbr,
  DRAWSTATEPROC lpOutputFunc,
  LPARAM lData,
  WPARAM wData,
  int x,
  int y,
  int cx,
  int cy,
  UINT fuFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
DrawStateW(
  HDC hdc,
  HBRUSH hbr,
  DRAWSTATEPROC lpOutputFunc,
  LPARAM lData,
  WPARAM wData,
  int x,
  int y,
  int cx,
  int cy,
  UINT fuFlags)
{
  return FALSE;
}

int
STDCALL
DrawTextA(
  HDC hDC,
  LPCSTR lpString,
  int nCount,
  LPRECT lpRect,
  UINT uFormat)
{
  return 0;
}

int
STDCALL
DrawTextExA(
  HDC hdc,
  LPSTR lpchText,
  int cchText,
  LPRECT lprc,
  UINT dwDTFormat,
  LPDRAWTEXTPARAMS lpDTParams)
{
  return 0;
}

int
STDCALL
DrawTextExW(
  HDC hdc,
  LPWSTR lpchText,
  int cchText,
  LPRECT lprc,
  UINT dwDTFormat,
  LPDRAWTEXTPARAMS lpDTParams)
{
  return 0;
}

int
STDCALL
DrawTextW(
  HDC hDC,
  LPCWSTR lpString,
  int nCount,
  LPRECT lpRect,
  UINT uFormat)
{
  return 0;
}

WINBOOL
STDCALL
EmptyClipboard(VOID)
{
  return FALSE;
}

WINBOOL
STDCALL
EnableMenuItem(
  HMENU hMenu,
  UINT uIDEnableItem,
  UINT uEnable)
{
  return FALSE;
}

WINBOOL
STDCALL
EnableScrollBar(
  HWND hWnd,
  UINT wSBflags,
  UINT wArrows)
{
  return FALSE;
}

WINBOOL
STDCALL
EnableWindow(
  HWND hWnd,
  WINBOOL bEnable)
{
  return FALSE;
}

WINBOOL
STDCALL
EndDeferWindowPos(
  HDWP hWinPosInfo)
{
  return FALSE;
}

WINBOOL
STDCALL
EndDialog(
  HWND hDlg,
  INT_PTR nResult)
{
  return FALSE;
}

WINBOOL
STDCALL
EndMenu(VOID)
{
  return FALSE;
}

WINBOOL
STDCALL
EndPaint(
  HWND hWnd,
  CONST PAINTSTRUCT *lpPaint)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumChildWindows(
  HWND hWndParent,
  ENUMWINDOWSPROC lpEnumFunc,
  LPARAM lParam)
{
  return FALSE;
}

UINT
STDCALL
EnumClipboardFormats(
  UINT format)
{
  return 0;
}

WINBOOL
STDCALL
EnumDesktopWindows(
  HDESK hDesktop,
  ENUMWINDOWSPROC lpfn,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumDesktopsA(
  HWINSTA hwinsta,
  DESKTOPENUMPROC lpEnumFunc,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumDesktopsW(
  HWINSTA hwinsta,
  DESKTOPENUMPROC lpEnumFunc,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumDisplayDevicesA(
  LPCSTR lpDevice,
  DWORD iDevNum,
  PDISPLAY_DEVICE lpDisplayDevice,
  DWORD dwFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumDisplayDevicesW(
  LPCWSTR lpDevice,
  DWORD iDevNum,
  PDISPLAY_DEVICE lpDisplayDevice,
  DWORD dwFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumDisplayMonitors(
  HDC hdc,
  LPRECT lprcClip,
  MONITORENUMPROC lpfnEnum,
  LPARAM dwData)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumDisplaySettingsA(
  LPCSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODE lpDevMode)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumDisplaySettingsExA(
  LPCSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODE lpDevMode,
  DWORD dwFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumDisplaySettingsExW(
  LPCWSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODE lpDevMode,
  DWORD dwFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumDisplaySettingsW(
  LPCWSTR lpszDeviceName,
  DWORD iModeNum,
  LPDEVMODE lpDevMode)
{
  return FALSE;
}

int
STDCALL
EnumPropsA(
  HWND hWnd,
  PROPENUMPROC lpEnumFunc)
{
  return 0;
}

int
STDCALL
EnumPropsExA(
  HWND hWnd,
  PROPENUMPROCEX lpEnumFunc,
  LPARAM lParam)
{
  return 0;
}

int
STDCALL
EnumPropsExW(
  HWND hWnd,
  PROPENUMPROCEX lpEnumFunc,
  LPARAM lParam)
{
  return 0;
}

int
STDCALL
EnumPropsW(
  HWND hWnd,
  PROPENUMPROC lpEnumFunc)
{
  return 0;
}

WINBOOL
STDCALL
EnumThreadWindows(
  DWORD dwThreadId,
  ENUMWINDOWSPROC lpfn,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumWindowStationsA(
  ENUMWINDOWSTATIONPROC lpEnumFunc,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumWindowStationsW(
  ENUMWINDOWSTATIONPROC lpEnumFunc,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
EnumWindows(
  ENUMWINDOWSPROC lpEnumFunc,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
EqualRect(
  CONST RECT *lprc1,
  CONST RECT *lprc2)
{
  return FALSE;
}

int
STDCALL
ExcludeUpdateRgn(
  HDC hDC,
  HWND hWnd)
{
  return 0;
}

WINBOOL
STDCALL
ExitWindowsEx(
  UINT uFlags,
  DWORD dwReserved)
{
  return FALSE;
}

int
STDCALL
FillRect(
  HDC hDC,
  CONST RECT *lprc,
  HBRUSH hbr)
{
  return 0;
}

HWND
STDCALL
FindWindowA(
  LPCSTR lpClassName,
  LPCSTR lpWindowName)
{
  return (HWND)0;
}

HWND
STDCALL
FindWindowExA(
  HWND hwndParent,
  HWND hwndChildAfter,
  LPCSTR lpszClass,
  LPCSTR lpszWindow)
{
  return (HWND)0;
}

HWND
STDCALL
FindWindowExW(
  HWND hwndParent,
  HWND hwndChildAfter,
  LPCWSTR lpszClass,
  LPCWSTR lpszWindow)
{
  return (HWND)0;
}

HWND
STDCALL
FindWindowW(
  LPCWSTR lpClassName,
  LPCWSTR lpWindowName)
{
  return (HWND)0;
}

WINBOOL
STDCALL
FlashWindow(
  HWND hWnd,
  WINBOOL bInvert)
{
  return FALSE;
}

WINBOOL
STDCALL
FlashWindowEx(
  PFLASHWINFO pfwi)
{
  return FALSE;
}

int
STDCALL
FrameRect(
  HDC hDC,
  CONST RECT *lprc,
  HBRUSH hbr)
{
  return 0;
}

WINBOOL
STDCALL
FreeDDElParam(
  UINT msg,
  LPARAM lParam)
{
  return FALSE;
}

HWND
STDCALL
GetActiveWindow(VOID)
{
  return (HWND)0;
}

WINBOOL
STDCALL
GetAltTabInfo(
  HWND hwnd,
  int iItem,
  PALTTABINFO pati,
  LPTSTR pszItemText,
  UINT cchItemText)
{
  return FALSE;
}

WINBOOL
STDCALL
GetAltTabInfoA(
  HWND hwnd,
  int iItem,
  PALTTABINFO pati,
  LPSTR pszItemText,
  UINT cchItemText)
{
  return FALSE;
}

WINBOOL
STDCALL
GetAltTabInfoW(
  HWND hwnd,
  int iItem,
  PALTTABINFO pati,
  LPWSTR pszItemText,
  UINT cchItemText)
{
  return FALSE;
}

HWND
STDCALL
GetAncestor(
  HWND hwnd,
  UINT gaFlags)
{
  return (HWND)0;
}

SHORT
STDCALL
GetAsyncKeyState(
  int vKey)
{
  return 0;
}

HWND
STDCALL
GetCapture(VOID)
{
  return (HWND)0;
}

UINT
STDCALL
GetCaretBlinkTime(VOID)
{
  return 0;
}

WINBOOL
STDCALL
GetCaretPos(
  LPPOINT lpPoint)
{
  return FALSE;
}

WINBOOL
STDCALL
GetClassInfoA(
  HINSTANCE hInstance,
  LPCSTR lpClassName,
  LPWNDCLASS lpWndClass)
{
  return FALSE;
}

WINBOOL
STDCALL
GetClassInfoExA(
  HINSTANCE hinst,
  LPCSTR lpszClass,
  LPWNDCLASSEX lpwcx)
{
  return FALSE;
}

WINBOOL
STDCALL
GetClassInfoExW(
  HINSTANCE hinst,
  LPCWSTR lpszClass,
  LPWNDCLASSEX lpwcx)
{
  return FALSE;
}

WINBOOL
STDCALL
GetClassInfoW(
  HINSTANCE hInstance,
  LPCWSTR lpClassName,
  LPWNDCLASS lpWndClass)
{
  return FALSE;
}

DWORD
STDCALL
GetClassLongA(
  HWND hWnd,
  int nIndex)
{
  return 0;
}

DWORD
STDCALL
GetClassLongW(
  HWND hWnd,
  int nIndex)
{
  return 0;
}

int
STDCALL
GetClassNameA(
  HWND hWnd,
  LPSTR lpClassName,
  int nMaxCount)
{
  return 0;
}

int
STDCALL
GetClassNameW(
  HWND hWnd,
  LPWSTR lpClassName,
  int nMaxCount)
{
  return 0;
}

WORD
STDCALL
GetClassWord(
  HWND hWnd,
  int nIndex)
{
  return 0;
}

WINBOOL
STDCALL
GetClientRect(
  HWND hWnd,
  LPRECT lpRect)
{
  return FALSE;
}

WINBOOL
STDCALL
GetClipCursor(
  LPRECT lpRect)
{
  return FALSE;
}

HANDLE
STDCALL
GetClipboardData(
  UINT uFormat)
{
  return (HANDLE)0;
}

int
STDCALL
GetClipboardFormatNameA(
  UINT format,
  LPSTR lpszFormatName,
  int cchMaxCount)
{
  return 0;
}

int
STDCALL
GetClipboardFormatNameW(
  UINT format,
  LPWSTR lpszFormatName,
  int cchMaxCount)
{
  return 0;
}

HWND
STDCALL
GetClipboardOwner(VOID)
{
  return (HWND)0;
}

DWORD
STDCALL
GetClipboardSequenceNumber(VOID)
{
  return 0;
}

HWND
STDCALL
GetClipboardViewer(VOID)
{
  return (HWND)0;
}

WINBOOL
STDCALL
GetComboBoxInfo(
  HWND hwndCombo,
  PCOMBOBOXINFO pcbi)
{
  return FALSE;
}

HCURSOR
STDCALL
GetCursor(VOID)
{
  return (HCURSOR)0;
}

WINBOOL
STDCALL
GetCursorInfo(
  PCURSORINFO pci)
{
  return FALSE;
}

WINBOOL
STDCALL
GetCursorPos(
  LPPOINT lpPoint)
{
  return FALSE;
}

HDC
STDCALL
GetDC(
  HWND hWnd)
{
  return (HDC)0;
}

HDC
STDCALL
GetDCEx(
  HWND hWnd,
  HRGN hrgnClip,
  DWORD flags)
{
  return (HDC)0;
}

HWND
STDCALL
GetDesktopWindow(VOID)
{
  return (HWND)0;
}

LONG
STDCALL
GetDialogBaseUnits(VOID)
{
  return 0;
}

int
STDCALL
GetDlgCtrlID(
  HWND hwndCtl)
{
  return 0;
}

HWND
STDCALL
GetDlgItem(
  HWND hDlg,
  int nIDDlgItem)
{
  return (HWND)0;
}

UINT
STDCALL
GetDlgItemInt(
  HWND hDlg,
  int nIDDlgItem,
  WINBOOL *lpTranslated,
  WINBOOL bSigned)
{
  return 0;
}

UINT
STDCALL
GetDlgItemTextA(
  HWND hDlg,
  int nIDDlgItem,
  LPSTR lpString,
  int nMaxCount)
{
  return 0;
}

UINT
STDCALL
GetDlgItemTextW(
  HWND hDlg,
  int nIDDlgItem,
  LPWSTR lpString,
  int nMaxCount)
{
  return 0;
}

UINT
STDCALL
GetDoubleClickTime(VOID)
{
  return 0;
}

HWND
STDCALL
GetFocus(VOID)
{
  return (HWND)0;
}

HWND
STDCALL
GetForegroundWindow(VOID)
{
  return (HWND)0;
}

WINBOOL
STDCALL
GetGUIThreadInfo(
  DWORD idThread,
  LPGUITHREADINFO lpgui)
{
  return FALSE;
}

DWORD
STDCALL
GetGuiResources(
  HANDLE hProcess,
  DWORD uiFlags)
{
  return 0;
}

WINBOOL
STDCALL
GetIconInfo(
  HICON hIcon,
  PICONINFO piconinfo)
{
  return FALSE;
}

WINBOOL
STDCALL
GetInputState(VOID)
{
  return FALSE;
}

UINT
STDCALL
GetKBCodePage(VOID)
{
  return 0;
}

int
STDCALL
GetKeyNameTextA(
  LONG lParam,
  LPSTR lpString,
  int nSize)
{
  return 0;
}

int
STDCALL
GetKeyNameTextW(
  LONG lParam,
  LPWSTR lpString,
  int nSize)
{
  return 0;
}

SHORT
STDCALL
GetKeyState(
  int nVirtKey)
{
  return 0;
}

HKL
STDCALL
GetKeyboardLayout(
  DWORD idThread)
{
  return (HKL)0;
}

UINT
STDCALL
GetKeyboardLayoutList(
  int nBuff,
  HKL FAR *lpList)
{
  return 0;
}

WINBOOL
STDCALL
GetKeyboardLayoutNameA(
  LPSTR pwszKLID)
{
  return FALSE;
}

WINBOOL
STDCALL
GetKeyboardLayoutNameW(
  LPWSTR pwszKLID)
{
  return FALSE;
}

WINBOOL
STDCALL
GetKeyboardState(
  PBYTE lpKeyState)
{
  return FALSE;
}

int
STDCALL
GetKeyboardType(
  int nTypeFlag)
{
  return 0;
}

HWND
STDCALL
GetLastActivePopup(
  HWND hWnd)
{
  return (HWND)0;
}

WINBOOL
STDCALL
GetLastInputInfo(
  PLASTINPUTINFO plii)
{
  return FALSE;
}

DWORD
STDCALL
GetListBoxInfo(
  HWND hwnd)
{
  return 0;
}

HMENU
STDCALL
GetMenu(
  HWND hWnd)
{
  return (HMENU)0;
}

WINBOOL
STDCALL
GetMenuBarInfo(
  HWND hwnd,
  LONG idObject,
  LONG idItem,
  PMENUBARINFO pmbi)
{
  return FALSE;
}

LONG
STDCALL
GetMenuCheckMarkDimensions(VOID)
{
  return 0;
}

UINT
STDCALL
GetMenuDefaultItem(
  HMENU hMenu,
  UINT fByPos,
  UINT gmdiFlags)
{
  return 0;
}

WINBOOL
STDCALL
GetMenuInfo(
  HMENU hmenu,
  LPCMENUINFO lpcmi)
{
  return FALSE;
}

int
STDCALL
GetMenuItemCount(
  HMENU hMenu)
{
  return 0;
}

UINT
STDCALL
GetMenuItemID(
  HMENU hMenu,
  int nPos)
{
  return 0;
}

WINBOOL
STDCALL
GetMenuItemInfoA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii)
{
  return FALSE;
}

WINBOOL
STDCALL
GetMenuItemInfoW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii)
{
  return FALSE;
}

WINBOOL
STDCALL
GetMenuItemRect(
  HWND hWnd,
  HMENU hMenu,
  UINT uItem,
  LPRECT lprcItem)
{
  return FALSE;
}

UINT
STDCALL
GetMenuState(
  HMENU hMenu,
  UINT uId,
  UINT uFlags)
{
  return 0;
}

int
STDCALL
GetMenuStringA(
  HMENU hMenu,
  UINT uIDItem,
  LPSTR lpString,
  int nMaxCount,
  UINT uFlag)
{
  return 0;
}

int
STDCALL
GetMenuStringW(
  HMENU hMenu,
  UINT uIDItem,
  LPWSTR lpString,
  int nMaxCount,
  UINT uFlag)
{
  return 0;
}

WINBOOL
STDCALL
GetMessageA(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax)
{
  return FALSE;
}

LPARAM
STDCALL
GetMessageExtraInfo(VOID)
{
  return (LPARAM)0;
}

DWORD
STDCALL
GetMessagePos(VOID)
{
  return 0;
}

LONG
STDCALL
GetMessageTime(VOID)
{
  return 0;
}

WINBOOL
STDCALL
GetMessageW(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax)
{
  return FALSE;
}

WINBOOL
STDCALL
GetMonitorInfoA(
  HMONITOR hMonitor,
  LPMONITORINFO lpmi)
{
  return FALSE;
}

WINBOOL
STDCALL
GetMonitorInfoW(
  HMONITOR hMonitor,
  LPMONITORINFO lpmi)
{
  return FALSE;
}

int
STDCALL
GetMouseMovePointsEx(
  UINT cbSize,
  LPMOUSEMOVEPOINT lppt,
  LPMOUSEMOVEPOINT lpptBuf,
  int nBufPoints,
  DWORD resolution)
{
  return 0;
}

HWND
STDCALL
GetNextDlgGroupItem(
  HWND hDlg,
  HWND hCtl,
  WINBOOL bPrevious)
{
  return (HWND)0;
}

HWND
STDCALL
GetNextDlgTabItem(
  HWND hDlg,
  HWND hCtl,
  WINBOOL bPrevious)
{
  return (HWND)0;
}

HWND
STDCALL
GetOpenClipboardWindow(VOID)
{
  return (HWND)0;
}

HWND
STDCALL
GetParent(
  HWND hWnd)
{
  return (HWND)0;
}

int
STDCALL
GetPriorityClipboardFormat(
  UINT *paFormatPriorityList,
  int cFormats)
{
  return 0;
}

WINBOOL
STDCALL
GetProcessDefaultLayout(
  DWORD *pdwDefaultLayout)
{
  return FALSE;
}

HWINSTA
STDCALL
GetProcessWindowStation(VOID)
{
  return (HWINSTA)0;
}

HANDLE
STDCALL
GetPropA(
  HWND hWnd,
  LPCSTR lpString)
{
  return (HANDLE)0;
}

HANDLE
STDCALL
GetPropW(
  HWND hWnd,
  LPCWSTR lpString)
{
  return (HANDLE)0;
}

DWORD
STDCALL
GetQueueStatus(
  UINT flags)
{
  return 0;
}

WINBOOL
STDCALL
GetScrollBarInfo(
  HWND hwnd,
  LONG idObject,
  PSCROLLBARINFO psbi)
{
  return FALSE;
}

WINBOOL
STDCALL
GetScrollInfo(
  HWND hwnd,
  int fnBar,
  LPSCROLLINFO lpsi)
{
  return FALSE;
}

int
STDCALL
GetScrollPos(
  HWND hWnd,
  int nBar)
{
  return 0;
}

WINBOOL
STDCALL
GetScrollRange(
  HWND hWnd,
  int nBar,
  LPINT lpMinPos,
  LPINT lpMaxPos)
{
  return FALSE;
}

HMENU
STDCALL
GetSubMenu(
  HMENU hMenu,
  int nPos)
{
  return (HMENU)0;
}

DWORD
STDCALL
GetSysColor(
  int nIndex)
{
  return 0;
}

HBRUSH
STDCALL
GetSysColorBrush(
  int nIndex)
{
  return (HBRUSH)0;
}

HMENU
STDCALL
GetSystemMenu(
  HWND hWnd,
  WINBOOL bRevert)
{
  return (HMENU)0;
}

int
STDCALL
GetSystemMetrics(
  int nIndex)
{
}

DWORD
STDCALL
GetTabbedTextExtentA(
  HDC hDC,
  LPCSTR lpString,
  int nCount,
  int nTabPositions,
  CONST LPINT lpnTabStopPositions)
{
  return 0;
}

DWORD
STDCALL
GetTabbedTextExtentW(
  HDC hDC,
  LPCWSTR lpString,
  int nCount,
  int nTabPositions,
  CONST LPINT lpnTabStopPositions)
{
  return 0;
}

HDESK
STDCALL
GetThreadDesktop(
  DWORD dwThreadId)
{
  return (HDESK)0;
}

WINBOOL
STDCALL
GetTitleBarInfo(
  HWND hwnd,
  PTITLEBARINFO pti)
{
  return FALSE;
}

HWND
STDCALL
GetTopWindow(
  HWND hWnd)
{
  return (HWND)0;
}

WINBOOL
STDCALL
GetUpdateRect(
  HWND hWnd,
  LPRECT lpRect,
  WINBOOL bErase)
{
  return FALSE;
}

int
STDCALL
GetUpdateRgn(
  HWND hWnd,
  HRGN hRgn,
  WINBOOL bErase)
{
  return 0;
}

WINBOOL
STDCALL
GetUserObjectInformationA(
  HANDLE hObj,
  int nIndex,
  PVOID pvInfo,
  DWORD nLength,
  LPDWORD lpnLengthNeeded)
{
  return FALSE;
}

WINBOOL
STDCALL
GetUserObjectInformationW(
  HANDLE hObj,
  int nIndex,
  PVOID pvInfo,
  DWORD nLength,
  LPDWORD lpnLengthNeeded)
{
  return FALSE;
}

HWND
STDCALL
GetWindow(
  HWND hWnd,
  UINT uCmd)
{
  return (HWND)0;
}

HDC
STDCALL
GetWindowDC(
  HWND hWnd)
{
  return (HDC)0;
}

WINBOOL
STDCALL
GetWindowInfo(
  HWND hwnd,
  PWINDOWINFO pwi)
{
  return FALSE;
}

LONG
STDCALL
GetWindowLongA(
  HWND hWnd,
  int nIndex)
{
  return 0;
}

LONG
STDCALL
GetWindowLongW(
  HWND hWnd,
  int nIndex)
{
  return 0;
}

UINT
STDCALL
GetWindowModuleFileName(
  HWND hwnd,
  LPSTR lpszFileName,
  UINT cchFileNameMax)
{
  return 0;
}

UINT
STDCALL
GetWindowModuleFileNameA(
  HWND hwnd,
  LPSTR lpszFileName,
  UINT cchFileNameMax)
{
  return 0;
}

UINT
STDCALL
GetWindowModuleFileNameW(
  HWND hwnd,
  LPWSTR lpszFileName,
  UINT cchFileNameMax)
{
  return 0;
}

WINBOOL
STDCALL
GetWindowPlacement(
  HWND hWnd,
  WINDOWPLACEMENT *lpwndpl)
{
  return FALSE;
}

WINBOOL
STDCALL
GetWindowRect(
  HWND hWnd,
  LPRECT lpRect)
{
  return FALSE;
}

int
STDCALL
GetWindowRgn(
  HWND hWnd,
  HRGN hRgn)
{
  return 0;
}

int
STDCALL
GetWindowTextA(
  HWND hWnd,
  LPSTR lpString,
  int nMaxCount)
{
  return 0;
}

int
STDCALL
GetWindowTextLengthA(
  HWND hWnd)
{
  return 0;
}

int
STDCALL
GetWindowTextLengthW(
  HWND hWnd)
{
  return 0;
}

int
STDCALL
GetWindowTextW(
  HWND hWnd,
  LPWSTR lpString,
  int nMaxCount)
{
  return 0;
}

DWORD
STDCALL
GetWindowThreadProcessId(
  HWND hWnd,
  LPDWORD lpdwProcessId)
{
  return 0;
}

WINBOOL
STDCALL
GrayStringA(
  HDC hDC,
  HBRUSH hBrush,
  GRAYSTRINGPROC lpOutputFunc,
  LPARAM lpData,
  int nCount,
  int X,
  int Y,
  int nWidth,
  int nHeight)
{
  return FALSE;
}

WINBOOL
STDCALL
GrayStringW(
  HDC hDC,
  HBRUSH hBrush,
  GRAYSTRINGPROC lpOutputFunc,
  LPARAM lpData,
  int nCount,
  int X,
  int Y,
  int nWidth,
  int nHeight)
{
  return FALSE;
}

WINBOOL
STDCALL
HideCaret(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
HiliteMenuItem(
  HWND hwnd,
  HMENU hmenu,
  UINT uItemHilite,
  UINT uHilite)
{
  return FALSE;
}

WINBOOL
STDCALL
ImpersonateDdeClientWindow(
  HWND hWndClient,
  HWND hWndServer)
{
  return FALSE;
}

WINBOOL
STDCALL
InSendMessage(VOID)
{
  return FALSE;
}

DWORD
STDCALL
InSendMessageEx(
  LPVOID lpReserved)
{
  return 0;
}

WINBOOL
STDCALL
InflateRect(
  LPRECT lprc,
  int dx,
  int dy)
{
  return FALSE;
}

WINBOOL
STDCALL
InsertMenuA(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  return FALSE;
}

WINBOOL
STDCALL
InsertMenuItemA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPCMENUITEMINFO lpmii)
{
  return FALSE;
}

WINBOOL
STDCALL
InsertMenuItemW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPCMENUITEMINFO lpmii)
{
  return FALSE;
}

WINBOOL
STDCALL
InsertMenuW(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  return FALSE;
}

WINBOOL
STDCALL
IntersectRect(
  LPRECT lprcDst,
  CONST RECT *lprcSrc1,
  CONST RECT *lprcSrc2)
{
  return FALSE;
}

WINBOOL
STDCALL
InvalidateRect(
  HWND hWnd,
  CONST RECT *lpRect,
  WINBOOL bErase)
{
  return FALSE;
}

WINBOOL
STDCALL
InvalidateRgn(
  HWND hWnd,
  HRGN hRgn,
  WINBOOL bErase)
{
  return FALSE;
}

WINBOOL
STDCALL
InvertRect(
  HDC hDC,
  CONST RECT *lprc)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharAlphaA(
  CHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharAlphaNumericA(
  CHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharAlphaNumericW(
  WCHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharAlphaW(
  WCHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharLowerA(
  CHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharLowerW(
  WCHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharUpperA(
  CHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsCharUpperW(
  WCHAR ch)
{
  return FALSE;
}

WINBOOL
STDCALL
IsChild(
  HWND hWndParent,
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
IsClipboardFormatAvailable(
  UINT format)
{
  return FALSE;
}

#if 0
WINBOOL
STDCALL
IsDialogMessage(
  HWND hDlg,
  LPMSG lpMsg)
{
  return FALSE;
}
#endif

WINBOOL
STDCALL
IsDialogMessageA(
  HWND hDlg,
  LPMSG lpMsg)
{
  return FALSE;
}

WINBOOL
STDCALL
IsDialogMessageW(
  HWND hDlg,
  LPMSG lpMsg)
{
  return FALSE;
}

UINT
STDCALL
IsDlgButtonChecked(
  HWND hDlg,
  int nIDButton)
{
  return 0;
}

WINBOOL
STDCALL
IsIconic(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
IsMenu(
  HMENU hMenu)
{
  return FALSE;
}

WINBOOL
STDCALL
IsRectEmpty(
  CONST RECT *lprc)
{
  return FALSE;
}

WINBOOL
STDCALL
IsWindow(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
IsWindowEnabled(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
IsWindowUnicode(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
IsWindowVisible(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
IsZoomed(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
KillTimer(
  HWND hWnd,
  UINT_PTR uIDEvent)
{
  return FALSE;
}

HACCEL
STDCALL
LoadAcceleratorsA(
  HINSTANCE hInstance,
  LPCSTR lpTableName)
{
  return (HACCEL)0;
}

HACCEL
STDCALL
LoadAcceleratorsW(
  HINSTANCE hInstance,
  LPCWSTR lpTableName)
{
  return (HACCEL)0;
}

HBITMAP
STDCALL
LoadBitmapA(
  HINSTANCE hInstance,
  LPCSTR lpBitmapName)
{
  return (HBITMAP)0;
}

HBITMAP
STDCALL
LoadBitmapW(
  HINSTANCE hInstance,
  LPCWSTR lpBitmapName)
{
  return (HBITMAP)0;
}

HCURSOR
STDCALL
LoadCursorA(
  HINSTANCE hInstance,
  LPCSTR lpCursorName)
{
  return (HCURSOR)0;
}

HCURSOR
STDCALL
LoadCursorFromFileA(
  LPCSTR lpFileName)
{
  return (HCURSOR)0;
}

HCURSOR
STDCALL
LoadCursorFromFileW(
  LPCWSTR lpFileName)
{
  return (HCURSOR)0;
}

HCURSOR
STDCALL
LoadCursorW(
  HINSTANCE hInstance,
  LPCWSTR lpCursorName)
{
  return (HCURSOR)0;
}

HICON
STDCALL
LoadIconA(
  HINSTANCE hInstance,
  LPCSTR lpIconName)
{
  return (HICON)0;
}

HICON
STDCALL
LoadIconW(
  HINSTANCE hInstance,
  LPCWSTR lpIconName)
{
  return (HICON)0;
}

HANDLE
STDCALL
LoadImageA(
  HINSTANCE hinst,
  LPCSTR lpszName,
  UINT uType,
  int cxDesired,
  int cyDesired,
  UINT fuLoad)
{
  return (HANDLE)0;
}

HANDLE
STDCALL
LoadImageW(
  HINSTANCE hinst,
  LPCWSTR lpszName,
  UINT uType,
  int cxDesired,
  int cyDesired,
  UINT fuLoad)
{
  return (HANDLE)0;
}

HKL
STDCALL
LoadKeyboardLayoutA(
  LPCSTR pwszKLID,
  UINT Flags)
{
  return (HKL)0;
}

HKL
STDCALL
LoadKeyboardLayoutW(
  LPCWSTR pwszKLID,
  UINT Flags)
{
  return (HKL)0;
}

HMENU
STDCALL
LoadMenuA(
  HINSTANCE hInstance,
  LPCSTR lpMenuName)
{
  return (HMENU)0;
}

HMENU
STDCALL
LoadMenuIndirectA(
  CONST MENUTEMPLATE *lpMenuTemplate)
{
  return (HMENU)0;
}

HMENU
STDCALL
LoadMenuIndirectW(
  CONST MENUTEMPLATE *lpMenuTemplate)
{
  return (HMENU)0;
}

HMENU
STDCALL
LoadMenuW(
  HINSTANCE hInstance,
  LPCWSTR lpMenuName)
{
  return (HMENU)0;
}

int
STDCALL
LoadStringA(
  HINSTANCE hInstance,
  UINT uID,
  LPSTR lpBuffer,
  int nBufferMax)
{
  return 0;
}

int
STDCALL
LoadStringW(
  HINSTANCE hInstance,
  UINT uID,
  LPWSTR lpBuffer,
  int nBufferMax)
{
  return 0;
}

WINBOOL
STDCALL
LockSetForegroundWindow(
  UINT uLockCode)
{
  return FALSE;
}

WINBOOL
STDCALL
LockWindowUpdate(
  HWND hWndLock)
{
  return FALSE;
}

WINBOOL
STDCALL
LockWorkStation(VOID)
{
  return FALSE;
}

int
STDCALL
LookupIconIdFromDirectory(
  PBYTE presbits,
  WINBOOL fIcon)
{
  return 0;
}

int
STDCALL
LookupIconIdFromDirectoryEx(
  PBYTE presbits,
  WINBOOL fIcon,
  int cxDesired,
  int cyDesired,
  UINT Flags)
{
  return 0;
}

WINBOOL
STDCALL
MapDialogRect(
  HWND hDlg,
  LPRECT lpRect)
{
  return FALSE;
}

UINT
STDCALL
MapVirtualKeyA(
  UINT uCode,
  UINT uMapType)
{
  return 0;
}

UINT
STDCALL
MapVirtualKeyExA(
  UINT uCode,
  UINT uMapType,
  HKL dwhkl)
{
  return 0;
}

UINT
STDCALL
MapVirtualKeyExW(
  UINT uCode,
  UINT uMapType,
  HKL dwhkl)
{
  return 0;
}

UINT
STDCALL
MapVirtualKeyW(
  UINT uCode,
  UINT uMapType)
{
  return 0;
}

int
STDCALL
MapWindowPoints(
  HWND hWndFrom,
  HWND hWndTo,
  LPPOINT lpPoints,
  UINT cPoints)
{
  return 0;
}

int
STDCALL
MenuItemFromPoint(
  HWND hWnd,
  HMENU hMenu,
  POINT ptScreen)
{
  return 0;
}

WINBOOL
STDCALL
MessageBeep(
  UINT uType)
{
  return FALSE;
}

int
STDCALL
MessageBoxA(
  HWND hWnd,
  LPCSTR lpText,
  LPCSTR lpCaption,
  UINT uType)
{
  return 0;
}

int
STDCALL
MessageBoxExA(
  HWND hWnd,
  LPCSTR lpText,
  LPCSTR lpCaption,
  UINT uType,
  WORD wLanguageId)
{
  return 0;
}

int
STDCALL
MessageBoxExW(
  HWND hWnd,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT uType,
  WORD wLanguageId)
{
  return 0;
}

int
STDCALL
MessageBoxIndirectA(
  CONST LPMSGBOXPARAMS lpMsgBoxParams)
{
  return 0;
}

int
STDCALL
MessageBoxIndirectW(
  CONST LPMSGBOXPARAMS lpMsgBoxParams)
{
  return 0;
}

int
STDCALL
MessageBoxW(
  HWND hWnd,
  LPCWSTR lpText,
  LPCWSTR lpCaption,
  UINT uType)
{
  return 0;
}

WINBOOL
STDCALL
ModifyMenuA(
  HMENU hMnu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCSTR lpNewItem)
{
  return FALSE;
}

WINBOOL
STDCALL
ModifyMenuW(
  HMENU hMnu,
  UINT uPosition,
  UINT uFlags,
  UINT_PTR uIDNewItem,
  LPCWSTR lpNewItem)
{
  return FALSE;
}

HMONITOR
STDCALL
MonitorFromPoint(
  POINT pt,
  DWORD dwFlags)
{
  return (HMONITOR)0;
}

HMONITOR
STDCALL
MonitorFromRect(
  LPRECT lprc,
  DWORD dwFlags)
{
  return (HMONITOR)0;
}

HMONITOR
STDCALL
MonitorFromWindow(
  HWND hwnd,
  DWORD dwFlags)
{
  return (HMONITOR)0;
}

WINBOOL
STDCALL
MoveWindow(
  HWND hWnd,
  int X,
  int Y,
  int nWidth,
  int nHeight,
  WINBOOL bRepaint)
{
  return FALSE;
}

DWORD
STDCALL
MsgWaitForMultipleObjects(
  DWORD nCount,
  CONST LPHANDLE pHandles,
  WINBOOL fWaitAll,
  DWORD dwMilliseconds,
  DWORD dwWakeMask)
{
  return 0;
}

DWORD
STDCALL
MsgWaitForMultipleObjectsEx(
  DWORD nCount,
  CONST HANDLE pHandles,
  DWORD dwMilliseconds,
  DWORD dwWakeMask,
  DWORD dwFlags)
{
  return 0;
}

DWORD
STDCALL
OemKeyScan(
  WORD wOemChar)
{
  return 0;
}

WINBOOL
STDCALL
OemToCharA(
  LPCSTR lpszSrc,
  LPSTR lpszDst)
{
  return FALSE;
}

WINBOOL
STDCALL
OemToCharBuffA(
  LPCSTR lpszSrc,
  LPSTR lpszDst,
  DWORD cchDstLength)
{
  return FALSE;
}

WINBOOL
STDCALL
OemToCharBuffW(
  LPCSTR lpszSrc,
  LPWSTR lpszDst,
  DWORD cchDstLength)
{
  return FALSE;
}

WINBOOL
STDCALL
OemToCharW(
  LPCSTR lpszSrc,
  LPWSTR lpszDst)
{
  return FALSE;
}

WINBOOL
STDCALL
OffsetRect(
  LPRECT lprc,
  int dx,
  int dy)
{
  return FALSE;
}

WINBOOL
STDCALL
OpenClipboard(
  HWND hWndNewOwner)
{
  return FALSE;
}

HDESK
STDCALL
OpenDesktopA(
  LPSTR lpszDesktop,
  DWORD dwFlags,
  WINBOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  return (HDESK)0;
}

HDESK
STDCALL
OpenDesktopW(
  LPWSTR lpszDesktop,
  DWORD dwFlags,
  WINBOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  return (HDESK)0;
}

WINBOOL
STDCALL
OpenIcon(
  HWND hWnd)
{
  return FALSE;
}

HDESK
STDCALL
OpenInputDesktop(
  DWORD dwFlags,
  WINBOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  return (HDESK)0;
}

HWINSTA
STDCALL
OpenWindowStationA(
  LPSTR lpszWinSta,
  WINBOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  return (HWINSTA)0;
}

HWINSTA
STDCALL
OpenWindowStationW(
  LPWSTR lpszWinSta,
  WINBOOL fInherit,
  ACCESS_MASK dwDesiredAccess)
{
  return (HWINSTA)0;
}

LPARAM
STDCALL
PackDDElParam(
  UINT msg,
  UINT_PTR uiLo,
  UINT_PTR uiHi)
{
  return (LPARAM)0;
}

WINBOOL
STDCALL
PaintDesktop(
  HDC hdc)
{
  return FALSE;
}

WINBOOL
STDCALL
PeekMessageA(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg)
{
  return FALSE;
}

WINBOOL
STDCALL
PeekMessageW(
  LPMSG lpMsg,
  HWND hWnd,
  UINT wMsgFilterMin,
  UINT wMsgFilterMax,
  UINT wRemoveMsg)
{
  return FALSE;
}

WINBOOL
STDCALL
PostMessageA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
PostMessageW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return FALSE;
}

VOID
STDCALL
PostQuitMessage(
  int nExitCode)
{
}

WINBOOL
STDCALL
PostThreadMessageA(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
PostThreadMessageW(
  DWORD idThread,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
PtInRect(
  CONST RECT *lprc,
  POINT pt)
{
  return FALSE;
}

HWND
STDCALL
RealChildWindowFromPoint(
  HWND hwndParent,
  POINT ptParentClientCoords)
{
  return (HWND)0;
}

UINT
STDCALL
RealGetWindowClass(
  HWND  hwnd,
  LPSTR pszType,
  UINT  cchType)
{
  return 0;
}

UINT
STDCALL
RealGetWindowClassA(
  HWND  hwnd,
  LPSTR pszType,
  UINT  cchType)
{
  return 0;
}

UINT
STDCALL
RealGetWindowClassW(
  HWND  hwnd,
  LPWSTR pszType,
  UINT  cchType)
{
  return 0;
}

WINBOOL
STDCALL
RedrawWindow(
  HWND hWnd,
  CONST RECT *lprcUpdate,
  HRGN hrgnUpdate,
  UINT flags)
{
  return FALSE;
}

ATOM
STDCALL
RegisterClassA(
  CONST WNDCLASS *lpWndClass)
{
  return (ATOM)0;
}

ATOM
STDCALL
RegisterClassExA(
  CONST WNDCLASSEX *lpwcx)
{
  return (ATOM)0;
}

ATOM
STDCALL
RegisterClassExW(
  CONST WNDCLASSEX *lpwcx)
{
  return (ATOM)0;
}

ATOM
STDCALL
RegisterClassW(
  CONST WNDCLASS *lpWndClass)
{
  return (ATOM)0;
}

UINT
STDCALL
RegisterClipboardFormatA(
  LPCSTR lpszFormat)
{
  return 0;
}

UINT
STDCALL
RegisterClipboardFormatW(
  LPCWSTR lpszFormat)
{
  return 0;
}
#if 0
HDEVNOTIFY
STDCALL
RegisterDeviceNotificationA(
  HANDLE hRecipient,
  LPVOID NotificationFilter,
  DWORD Flags)
{
  return (HDEVNOTIFY)0;
}

HDEVNOTIFY
STDCALL
RegisterDeviceNotificationW(
  HANDLE hRecipient,
  LPVOID NotificationFilter,
  DWORD Flags)
{
  return (HDEVNOTIFY)0;
}
#endif
WINBOOL
STDCALL
RegisterHotKey(
  HWND hWnd,
  int id,
  UINT fsModifiers,
  UINT vk)
{
  return FALSE;
}

UINT
STDCALL
RegisterWindowMessageA(
  LPCSTR lpString)
{
  return 0;
}

UINT
STDCALL
RegisterWindowMessageW(
  LPCWSTR lpString)
{
  return 0;
}

WINBOOL
STDCALL
ReleaseCapture(VOID)
{
  return FALSE;
}

int
STDCALL
ReleaseDC(
  HWND hWnd,
  HDC hDC)
{
  return 0;
}

WINBOOL
STDCALL
RemoveMenu(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags)
{
  return FALSE;
}

HANDLE
STDCALL
RemovePropA(
  HWND hWnd,
  LPCSTR lpString)
{
  return (HANDLE)0;
}

HANDLE
STDCALL
RemovePropW(
  HWND hWnd,
  LPCWSTR lpString)
{
  return (HANDLE)0;
}

WINBOOL
STDCALL
ReplyMessage(
  LRESULT lResult)
{
  return FALSE;
}

LPARAM
STDCALL
ReuseDDElParam(
  LPARAM lParam,
  UINT msgIn,
  UINT msgOut,
  UINT_PTR uiLo,
  UINT_PTR uiHi)
{
  return (LPARAM)0;
}

WINBOOL
STDCALL
ScrollDC(
  HDC hDC,
  int dx,
  int dy,
  CONST RECT *lprcScroll,
  CONST RECT *lprcClip,
  HRGN hrgnUpdate,
  LPRECT lprcUpdate)
{
  return FALSE;
}

WINBOOL
STDCALL
ScrollWindow(
  HWND hWnd,
  int XAmount,
  int YAmount,
  CONST RECT *lpRect,
  CONST RECT *lpClipRect)
{
  return FALSE;
}

int
STDCALL
ScrollWindowEx(
  HWND hWnd,
  int dx,
  int dy,
  CONST RECT *prcScroll,
  CONST RECT *prcClip,
  HRGN hrgnUpdate,
  LPRECT prcUpdate,
  UINT flags)
{
  return 0;
}

LRESULT
STDCALL
SendDlgItemMessageA(
  HWND hDlg,
  int nIDDlgItem,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
SendDlgItemMessageW(
  HWND hDlg,
  int nIDDlgItem,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

UINT
STDCALL
SendInput(
  UINT nInputs,
  LPINPUT pInputs,
  int cbSize)
{
  return 0;
}

LRESULT
STDCALL
SendMessageA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

WINBOOL
STDCALL
SendMessageCallbackA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData)
{
  return FALSE;
}

WINBOOL
STDCALL
SendMessageCallbackW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  SENDASYNCPROC lpCallBack,
  ULONG_PTR dwData)
{
  return FALSE;
}

LRESULT
STDCALL
SendMessageTimeoutA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  UINT fuFlags,
  UINT uTimeout,
  PDWORD_PTR lpdwResult)
{
  return (LRESULT)0;
}

LRESULT
STDCALL
SendMessageTimeoutW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam,
  UINT fuFlags,
  UINT uTimeout,
  PDWORD_PTR lpdwResult)
{
  return (LRESULT)0;
}


LRESULT
STDCALL
SendMessageW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return (LRESULT)0;
}

WINBOOL
STDCALL
SendNotifyMessageA(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return FALSE;
}

WINBOOL
STDCALL
SendNotifyMessageW(
  HWND hWnd,
  UINT Msg,
  WPARAM wParam,
  LPARAM lParam)
{
  return FALSE;
}

HWND
STDCALL
SetActiveWindow(
  HWND hWnd)
{
  return (HWND)0;
}

HWND
STDCALL
SetCapture(
  HWND hWnd)
{
  return (HWND)0;
}

WINBOOL
STDCALL
SetCaretBlinkTime(
  UINT uMSeconds)
{
  return FALSE;
}

WINBOOL
STDCALL
SetCaretPos(
  int X,
  int Y)
{
  return FALSE;
}

DWORD
STDCALL
SetClassLongA(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  return 0;
}

DWORD
STDCALL
SetClassLongW(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  return 0;
}

WORD
STDCALL
SetClassWord(
  HWND hWnd,
  int nIndex,
  WORD wNewWord)
{
  return 0;
}

HANDLE
STDCALL
SetClipboardData(
  UINT uFormat,
  HANDLE hMem)
{
  return (HANDLE)0;
}

HWND
STDCALL
SetClipboardViewer(
  HWND hWndNewViewer)
{
  return (HWND)0;
}

HCURSOR
STDCALL
SetCursor(
  HCURSOR hCursor)
{
  return (HCURSOR)0;
}

WINBOOL
STDCALL
SetCursorPos(
  int X,
  int Y)
{
  return FALSE;
}

WINBOOL
STDCALL
SetDlgItemInt(
  HWND hDlg,
  int nIDDlgItem,
  UINT uValue,
  WINBOOL bSigned)
{
  return FALSE;
}

WINBOOL
STDCALL
SetDlgItemTextA(
  HWND hDlg,
  int nIDDlgItem,
  LPCSTR lpString)
{
  return FALSE;
}

WINBOOL
STDCALL
SetDlgItemTextW(
  HWND hDlg,
  int nIDDlgItem,
  LPCWSTR lpString)
{
  return FALSE;
}

WINBOOL
STDCALL
SetDoubleClickTime(
  UINT uInterval)
{
  return FALSE;
}

HWND
STDCALL
SetFocus(
  HWND hWnd)
{
  return (HWND)0;
}

WINBOOL
STDCALL
SetForegroundWindow(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
SetKeyboardState(
  LPBYTE lpKeyState)
{
  return FALSE;
}

VOID
STDCALL
SetLastErrorEx(
  DWORD dwErrCode,
  DWORD dwType)
{
}

WINBOOL
STDCALL
SetLayeredWindowAttributes(
  HWND hwnd,
  COLORREF crKey,
  BYTE bAlpha,
  DWORD dwFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
SetMenu(
  HWND hWnd,
  HMENU hMenu)
{
  return FALSE;
}

WINBOOL
STDCALL
SetMenuDefaultItem(
  HMENU hMenu,
  UINT uItem,
  UINT fByPos)
{
  return FALSE;
}

WINBOOL
STDCALL
SetMenuInfo(
  HMENU hmenu,
  LPCMENUINFO lpcmi)
{
  return FALSE;
}

WINBOOL
STDCALL
SetMenuItemBitmaps(
  HMENU hMenu,
  UINT uPosition,
  UINT uFlags,
  HBITMAP hBitmapUnchecked,
  HBITMAP hBitmapChecked)
{
  return FALSE;
}

WINBOOL
STDCALL
SetMenuItemInfoA(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii)
{
  return FALSE;
}

WINBOOL
STDCALL
SetMenuItemInfoW(
  HMENU hMenu,
  UINT uItem,
  WINBOOL fByPosition,
  LPMENUITEMINFO lpmii)
{
  return FALSE;
}

LPARAM
STDCALL
SetMessageExtraInfo(
  LPARAM lParam)
{
  return (LPARAM)0;
}

HWND
STDCALL
SetParent(
  HWND hWndChild,
  HWND hWndNewParent)
{
  return (HWND)0;
}

WINBOOL
STDCALL
SetProcessDefaultLayout(
  DWORD dwDefaultLayout)
{
  return FALSE;
}

WINBOOL
STDCALL
SetProcessWindowStation(
  HWINSTA hWinSta)
{
  return FALSE;
}

WINBOOL
STDCALL
SetPropA(
  HWND hWnd,
  LPCSTR lpString,
  HANDLE hData)
{
  return FALSE;
}

WINBOOL
STDCALL
SetPropW(
  HWND hWnd,
  LPCWSTR lpString,
  HANDLE hData)
{
  return FALSE;
}

WINBOOL
STDCALL
SetRect(
  LPRECT lprc,
  int xLeft,
  int yTop,
  int xRight,
  int yBottom)
{
  return FALSE;
}

WINBOOL
STDCALL
SetRectEmpty(
  LPRECT lprc)
{
  return FALSE;
}

int
STDCALL
SetScrollInfo(
  HWND hwnd,
  int fnBar,
  LPCSCROLLINFO lpsi,
  WINBOOL fRedraw)
{
  return 0;
}

int
STDCALL
SetScrollPos(
  HWND hWnd,
  int nBar,
  int nPos,
  WINBOOL bRedraw)
{
  return 0;
}

WINBOOL
STDCALL
SetScrollRange(
  HWND hWnd,
  int nBar,
  int nMinPos,
  int nMaxPos,
  WINBOOL bRedraw)
{
  return FALSE;
}

WINBOOL
STDCALL
SetSysColors(
  int cElements,
  CONST INT *lpaElements,
  CONST COLORREF *lpaRgbValues)
{
  return FALSE;
}

WINBOOL
STDCALL
SetSystemCursor(
  HCURSOR hcur,
  DWORD id)
{
  return FALSE;
}

WINBOOL
STDCALL
SetThreadDesktop(
  HDESK hDesktop)
{
  return FALSE;
}

UINT_PTR
STDCALL
SetTimer(
  HWND hWnd,
  UINT_PTR nIDEvent,
  UINT uElapse,
  TIMERPROC lpTimerFunc)
{
  return (UINT_PTR)0;
}

WINBOOL
STDCALL
SetUserObjectInformationA(
  HANDLE hObj,
  int nIndex,
  PVOID pvInfo,
  DWORD nLength)
{
  return FALSE;
}

WINBOOL
STDCALL
SetUserObjectInformationW(
  HANDLE hObj,
  int nIndex,
  PVOID pvInfo,
  DWORD nLength)
{
  return FALSE;
}

LONG
STDCALL
SetWindowLongA(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  return 0;
}

LONG
STDCALL
SetWindowLongW(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  return 0;
}

WINBOOL
STDCALL
SetWindowPlacement(
  HWND hWnd,
  CONST WINDOWPLACEMENT *lpwndpl)
{
  return FALSE;
}

WINBOOL
STDCALL
SetWindowPos(
  HWND hWnd,
  HWND hWndInsertAfter,
  int X,
  int Y,
  int cx,
  int cy,
  UINT uFlags)
{
  return FALSE;
}

int
STDCALL
SetWindowRgn(
  HWND hWnd,
  HRGN hRgn,
  WINBOOL bRedraw)
{
  return 0;
}

WINBOOL
STDCALL
SetWindowTextA(
  HWND hWnd,
  LPCSTR lpString)
{
  return FALSE;
}

WINBOOL
STDCALL
SetWindowTextW(
  HWND hWnd,
  LPCWSTR lpString)
{
  return FALSE;
}

WINBOOL
STDCALL
ShowCaret(
  HWND hWnd)
{
  return FALSE;
}

int
STDCALL
ShowCursor(
  WINBOOL bShow)
{
  return 0;
}

WINBOOL
STDCALL
ShowOwnedPopups(
  HWND hWnd,
  WINBOOL fShow)
{
  return FALSE;
}

WINBOOL
STDCALL
ShowScrollBar(
  HWND hWnd,
  int wBar,
  WINBOOL bShow)
{
  return FALSE;
}

WINBOOL
STDCALL
ShowWindow(
  HWND hWnd,
  int nCmdShow)
{
  return FALSE;
}

WINBOOL
STDCALL
ShowWindowAsync(
  HWND hWnd,
  int nCmdShow)
{
  return FALSE;
}

WINBOOL
STDCALL
SubtractRect(
  LPRECT lprcDst,
  CONST RECT *lprcSrc1,
  CONST RECT *lprcSrc2)
{
  return FALSE;
}

WINBOOL
STDCALL
SwapMouseButton(
  WINBOOL fSwap)
{
  return FALSE;
}

WINBOOL
STDCALL
SwitchDesktop(
  HDESK hDesktop)
{
  return FALSE;
}

WINBOOL
STDCALL
SystemParametersInfoA(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni)
{
  return FALSE;
}

WINBOOL
STDCALL
SystemParametersInfoW(
  UINT uiAction,
  UINT uiParam,
  PVOID pvParam,
  UINT fWinIni)
{
  return FALSE;
}

LONG
STDCALL
TabbedTextOutA(
  HDC hDC,
  int X,
  int Y,
  LPCSTR lpString,
  int nCount,
  int nTabPositions,
  CONST LPINT lpnTabStopPositions,
  int nTabOrigin)
{
  return 0;
}

LONG
STDCALL
TabbedTextOutW(
  HDC hDC,
  int X,
  int Y,
  LPCWSTR lpString,
  int nCount,
  int nTabPositions,
  CONST LPINT lpnTabStopPositions,
  int nTabOrigin)
{
  return 0;
}

WORD
STDCALL
TileWindows(
  HWND hwndParent,
  UINT wHow,
  CONST RECT *lpRect,
  UINT cKids,
  const HWND *lpKids)
{
  return 0;
}

int
STDCALL
ToAscii(
  UINT uVirtKey,
  UINT uScanCode,
  CONST PBYTE lpKeyState,
  LPWORD lpChar,
  UINT uFlags)
{
  return 0;
}

int
STDCALL
ToAsciiEx(
  UINT uVirtKey,
  UINT uScanCode,
  CONST PBYTE lpKeyState,
  LPWORD lpChar,
  UINT uFlags,
  HKL dwhkl)
{
  return 0;
}

int
STDCALL
ToUnicode(
  UINT wVirtKey,
  UINT wScanCode,
  CONST PBYTE lpKeyState,
  LPWSTR pwszBuff,
  int cchBuff,
  UINT wFlags)
{
  return 0;
}

int
STDCALL
ToUnicodeEx(
  UINT wVirtKey,
  UINT wScanCode,
  CONST PBYTE lpKeyState,
  LPWSTR pwszBuff,
  int cchBuff,
  UINT wFlags,
  HKL dwhkl)
{
  return 0;
}

WINBOOL
STDCALL
TrackMouseEvent(
  LPTRACKMOUSEEVENT lpEventTrack)
{
  return FALSE;
}

WINBOOL
STDCALL
TrackPopupMenu(
  HMENU hMenu,
  UINT uFlags,
  int x,
  int y,
  int nReserved,
  HWND hWnd,
  CONST RECT *prcRect)
{
  return FALSE;
}

WINBOOL
STDCALL
TrackPopupMenuEx(
  HMENU hmenu,
  UINT fuFlags,
  int x,
  int y,
  HWND hwnd,
  LPTPMPARAMS lptpm)
{
  return FALSE;
}

#if 0
int
STDCALL
TranslateAccelerator(
  HWND hWnd,
  HACCEL hAccTable,
  LPMSG lpMsg)
{
  return 0;
}
#endif

int
STDCALL
TranslateAcceleratorA(
  HWND hWnd,
  HACCEL hAccTable,
  LPMSG lpMsg)
{
  return 0;
}

int
STDCALL
TranslateAcceleratorW(
  HWND hWnd,
  HACCEL hAccTable,
  LPMSG lpMsg)
{
  return 0;
}

WINBOOL
STDCALL
TranslateMDISysAccel(
  HWND hWndClient,
  LPMSG lpMsg)
{
  return FALSE;
}

WINBOOL
STDCALL
TranslateMessage(
  CONST MSG *lpMsg)
{
  return FALSE;
}

WINBOOL
STDCALL
UnhookWindowsHookEx(
  HHOOK hhk)
{
  return FALSE;
}

WINBOOL
STDCALL
UnionRect(
  LPRECT lprcDst,
  CONST RECT *lprcSrc1,
  CONST RECT *lprcSrc2)
{
  return FALSE;
}

WINBOOL
STDCALL
UnloadKeyboardLayout(
  HKL hkl)
{
  return FALSE;
}

WINBOOL
STDCALL
UnpackDDElParam(
  UINT msg,
  LPARAM lParam,
  PUINT_PTR puiLo,
  PUINT_PTR puiHi)
{
  return FALSE;
}

WINBOOL
STDCALL
UnregisterClassA(
  LPCSTR lpClassName,
  HINSTANCE hInstance)
{
  return FALSE;
}

WINBOOL
STDCALL
UnregisterClassW(
  LPCWSTR lpClassName,
  HINSTANCE hInstance)
{
  return FALSE;
}

WINBOOL
STDCALL
UnregisterDeviceNotification(
  HDEVNOTIFY Handle)
{
  return FALSE;
}

WINBOOL
STDCALL
UnregisterHotKey(
  HWND hWnd,
  int id)
{
  return FALSE;
}

WINBOOL
STDCALL
UpdateLayeredWindow(
  HWND hwnd,
  HDC hdcDst,
  POINT *pptDst,
  SIZE *psize,
  HDC hdcSrc,
  POINT *pptSrc,
  COLORREF crKey,
  BLENDFUNCTION *pblend,
  DWORD dwFlags)
{
  return FALSE;
}

WINBOOL
STDCALL
UpdateWindow(
  HWND hWnd)
{
  return FALSE;
}

WINBOOL
STDCALL
UserHandleGrantAccess(
  HANDLE hUserHandle,
  HANDLE hJob,
  WINBOOL bGrant)
{
  return FALSE;
}

WINBOOL
STDCALL
ValidateRect(
  HWND hWnd,
  CONST RECT *lpRect)
{
  return FALSE;
}

WINBOOL
STDCALL
ValidateRgn(
  HWND hWnd,
  HRGN hRgn)
{
  return FALSE;
}

SHORT
STDCALL
VkKeyScanA(
  CHAR ch)
{
  return 0;
}

SHORT
STDCALL
VkKeyScanExA(
  CHAR ch,
  HKL dwhkl)
{
  return 0;
}

SHORT
STDCALL
VkKeyScanExW(
  WCHAR ch,
  HKL dwhkl)
{
  return 0;
}

SHORT
STDCALL
VkKeyScanW(
  WCHAR ch)
{
  return 0;
}

DWORD
STDCALL
WaitForInputIdle(
  HANDLE hProcess,
  DWORD dwMilliseconds)
{
  return 0;
}

WINBOOL
STDCALL
WaitMessage(VOID)
{
  return FALSE;
}

HWND
STDCALL
WindowFromDC(
  HDC hDC)
{
  return (HWND)0;
}

HWND
STDCALL
WindowFromPoint(
  POINT Point)
{
  return (HWND)0;
}

/* EOF */
