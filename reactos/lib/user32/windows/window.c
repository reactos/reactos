/* $Id: window.c,v 1.29 2003/05/12 19:30:00 jfilby Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/window.c
 * PURPOSE:         Window management
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      06-06-2001  CSH  Created
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include <user32.h>
#include <window.h>
#include <user32/callback.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/
ULONG
   WinHasThickFrameStyle(ULONG Style, ULONG ExStyle)
{
  return((Style & WS_THICKFRAME) &&
	 (!((Style & (WS_DLGFRAME | WS_BORDER)) == WS_DLGFRAME)));
}

NTSTATUS STDCALL
User32SendNCCALCSIZEMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDNCCALCSIZEMESSAGE_CALLBACK_ARGUMENTS CallbackArgs;
  SENDNCCALCSIZEMESSAGE_CALLBACK_RESULT Result;
  WNDPROC Proc;

  DPRINT("User32SendNCCALCSIZEMessageForKernel.\n");
  CallbackArgs = (PSENDNCCALCSIZEMESSAGE_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDNCCALCSIZEMESSAGE_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)GetWindowLongW(CallbackArgs->Wnd, GWL_WNDPROC);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  if (CallbackArgs->Validate)
    {
      Result.Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_NCCALCSIZE, 
				      TRUE, 
				      (LPARAM)&CallbackArgs->Params);
      Result.Params = CallbackArgs->Params;
    }
  else
    {
      Result.Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_NCCALCSIZE,
				      FALSE, (LPARAM)&CallbackArgs->Rect);
      Result.Rect = CallbackArgs->Rect;
    }
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(Result), STATUS_SUCCESS));
}

NTSTATUS STDCALL
User32SendGETMINMAXINFOMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDGETMINMAXINFO_CALLBACK_ARGUMENTS CallbackArgs;
  SENDGETMINMAXINFO_CALLBACK_RESULT Result;
  WNDPROC Proc;

  DPRINT("User32SendGETMINAXINFOMessageForKernel.\n");
  CallbackArgs = (PSENDGETMINMAXINFO_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDGETMINMAXINFO_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)GetWindowLongW(CallbackArgs->Wnd, GWL_WNDPROC);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result.Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_GETMINMAXINFO, 
				  0, (LPARAM)&CallbackArgs->MinMaxInfo);
  Result.MinMaxInfo = CallbackArgs->MinMaxInfo;
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(Result), STATUS_SUCCESS));
}

NTSTATUS STDCALL
User32SendCREATEMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDCREATEMESSAGE_CALLBACK_ARGUMENTS CallbackArgs;
  WNDPROC Proc;
  LRESULT Result;

  DPRINT("User32SendCREATEMessageForKernel.\n");
  CallbackArgs = (PSENDCREATEMESSAGE_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDCREATEMESSAGE_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)GetWindowLongW(CallbackArgs->Wnd, GWL_WNDPROC);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_CREATE, 0, 
			   (LPARAM)&CallbackArgs->CreateStruct);
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}

NTSTATUS STDCALL
User32SendNCCREATEMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDNCCREATEMESSAGE_CALLBACK_ARGUMENTS CallbackArgs;
  WNDPROC Proc;
  LRESULT Result;

  DPRINT("User32SendNCCREATEMessageForKernel.\n");
  CallbackArgs = (PSENDNCCREATEMESSAGE_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDNCCREATEMESSAGE_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)GetWindowLongW(CallbackArgs->Wnd, GWL_WNDPROC);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_NCCREATE, 0, 
			   (LPARAM)&CallbackArgs->CreateStruct);
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}

NTSTATUS STDCALL
User32CallSendAsyncProcForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDASYNCPROC_CALLBACK_ARGUMENTS CallbackArgs;

  DPRINT("User32CallSendAsyncProcKernel()\n");
  CallbackArgs = (PSENDASYNCPROC_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(WINDOWPROC_CALLBACK_ARGUMENTS))
    {
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  CallbackArgs->Callback(CallbackArgs->Wnd, CallbackArgs->Msg,
			 CallbackArgs->Context, CallbackArgs->Result);
  return(STATUS_SUCCESS);
}

NTSTATUS STDCALL
User32CallWindowProcFromKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PWINDOWPROC_CALLBACK_ARGUMENTS CallbackArgs;
  LRESULT Result;

  CallbackArgs = (PWINDOWPROC_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(WINDOWPROC_CALLBACK_ARGUMENTS))
    {
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  if (CallbackArgs->Proc == NULL)
    {
      CallbackArgs->Proc = (WNDPROC)GetWindowLong(CallbackArgs->Wnd, 
						  GWL_WNDPROC);
    }
  Result = CallWindowProcW(CallbackArgs->Proc, CallbackArgs->Wnd, 
			   CallbackArgs->Msg, CallbackArgs->wParam, 
			   CallbackArgs->lParam);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}

static void NC_AdjustRectOuter95 (LPRECT rect, DWORD style, BOOL menu, DWORD exStyle)
{
    int adjust;
    if(style & WS_ICONIC) return;

    if ((exStyle & (WS_EX_STATICEDGE|WS_EX_DLGMODALFRAME)) ==
        WS_EX_STATICEDGE)
    {
        adjust = 1; /* for the outer frame always present */
    }
    else
    {
        adjust = 0;
        if ((exStyle & WS_EX_DLGMODALFRAME) ||
            (style & (WS_THICKFRAME|WS_DLGFRAME))) adjust = 2; /* outer */
    }
    if (style & WS_THICKFRAME)
        adjust +=  ( GetSystemMetrics (SM_CXFRAME)
                   - GetSystemMetrics (SM_CXDLGFRAME)); /* The resize border */
    if ((style & (WS_BORDER|WS_DLGFRAME)) ||
        (exStyle & WS_EX_DLGMODALFRAME))
        adjust++; /* The other border */

    InflateRect (rect, adjust, adjust);

    if ((style & WS_CAPTION) == WS_CAPTION)
    {
        if (exStyle & WS_EX_TOOLWINDOW)
            rect->top -= GetSystemMetrics(SM_CYSMCAPTION);
        else
            rect->top -= GetSystemMetrics(SM_CYCAPTION);
    }
    if (menu) rect->top -= GetSystemMetrics(SM_CYMENU);
}

static void
NC_AdjustRectInner95 (LPRECT rect, DWORD style, DWORD exStyle)
{
    if(style & WS_ICONIC) return;

    if (exStyle & WS_EX_CLIENTEDGE)
        InflateRect(rect, GetSystemMetrics(SM_CXEDGE), GetSystemMetrics(SM_CYEDGE));

    if (style & WS_VSCROLL)
    {
        if((exStyle & WS_EX_LEFTSCROLLBAR) != 0)
            rect->left  -= GetSystemMetrics(SM_CXVSCROLL);
        else
            rect->right += GetSystemMetrics(SM_CXVSCROLL);
    }
    if (style & WS_HSCROLL) rect->bottom += GetSystemMetrics(SM_CYHSCROLL);
}

WINBOOL STDCALL
AdjustWindowRect(LPRECT lpRect,
		 DWORD dwStyle,
		 WINBOOL bMenu)
{
  return(AdjustWindowRectEx(lpRect, dwStyle, bMenu, 0));
}

WINBOOL STDCALL
AdjustWindowRectEx(LPRECT lpRect, 
		   DWORD dwStyle, 
		   WINBOOL bMenu, 
		   DWORD dwExStyle)
{
    dwStyle &= (WS_DLGFRAME | WS_BORDER | WS_THICKFRAME | WS_CHILD);
    dwExStyle &= (WS_EX_DLGMODALFRAME | WS_EX_CLIENTEDGE |
                WS_EX_STATICEDGE | WS_EX_TOOLWINDOW);
    if (dwExStyle & WS_EX_DLGMODALFRAME) dwStyle &= ~WS_THICKFRAME;

    NC_AdjustRectOuter95( lpRect, dwStyle, bMenu, dwExStyle );
    NC_AdjustRectInner95( lpRect, dwStyle, dwExStyle );
    lpRect->right += 2;
    lpRect->bottom += 2;
    return TRUE;
}

WINBOOL STDCALL
AllowSetForegroundWindow(DWORD dwProcessId)
{
  UNIMPLEMENTED;
  return(FALSE);
}

WINBOOL STDCALL
AnimateWindow(HWND hwnd,
	      DWORD dwTime,
	      DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}

UINT STDCALL
ArrangeIconicWindows(HWND hWnd)
{
  UNIMPLEMENTED;
  return 0;
}

HDWP STDCALL
BeginDeferWindowPos(int nNumWindows)
{
  UNIMPLEMENTED;
  return (HDWP)0;
}

WINBOOL STDCALL
BringWindowToTop(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

WORD STDCALL
CascadeWindows(HWND hwndParent,
	       UINT wHow,
	       CONST RECT *lpRect,
	       UINT cKids,
	       const HWND *lpKids)
{
  UNIMPLEMENTED;
  return 0;
}

HWND STDCALL
ChildWindowFromPoint(HWND hWndParent,
		     POINT Point)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

HWND STDCALL
ChildWindowFromPointEx(HWND hwndParent,
		       POINT pt,
		       UINT uFlags)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

WINBOOL STDCALL
CloseWindow(HWND hWnd)
{
    SendMessageA(hWnd, WM_CLOSE, 0, 0);
    SendMessageA(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);

    return (WINBOOL)(hWnd);
}

HWND STDCALL
CreateWindowExA(DWORD dwExStyle,
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
  UNICODE_STRING WindowName;
  UNICODE_STRING ClassName;
  HWND Handle;
  INT sw;

OutputDebugStringA("CreateWindowEx\n");
  if (IS_ATOM(lpClassName))
    {
      RtlInitUnicodeString(&ClassName, NULL);
      ClassName.Buffer = (LPWSTR)lpClassName;
    }
  else
    {
      if (!RtlCreateUnicodeStringFromAsciiz(&(ClassName), (PCSZ)lpClassName))
	{
	  SetLastError(ERROR_OUTOFMEMORY);
	  return (HWND)0;
	}
    }

  if (!RtlCreateUnicodeStringFromAsciiz(&WindowName, (PCSZ)lpWindowName))
    {
      if (!IS_ATOM(lpClassName))
	{
	  RtlFreeUnicodeString(&ClassName);
	}
      SetLastError(ERROR_OUTOFMEMORY);
      return (HWND)0;
    }

  /* Fixup default coordinates. */
  sw = SW_SHOW;
  if (x == (LONG) CW_USEDEFAULT || nWidth == (LONG) CW_USEDEFAULT)
    {
      if (dwStyle & (WS_CHILD | WS_POPUP))
	{
	  if (x == (LONG) CW_USEDEFAULT)
	    {
	      x = y = 0;
	    }
	  if (nWidth == (LONG) CW_USEDEFAULT)
	    {
	      nWidth = nHeight = 0;
	    }
	}
      else
	{
	  STARTUPINFOA info;

	  GetStartupInfoA(&info);

	  if (x == (LONG) CW_USEDEFAULT)
	    {
	      if (y != (LONG) CW_USEDEFAULT)
		{
		  sw = y;
		}
	      x = (info.dwFlags & STARTF_USEPOSITION) ? info.dwX : 0;
	      y = (info.dwFlags & STARTF_USEPOSITION) ? info.dwY : 0;
	    }
	  
	  if (nWidth == (LONG) CW_USEDEFAULT)
	    {
	      if (info.dwFlags & STARTF_USESIZE)
		{
		  nWidth = info.dwXSize;
		  nHeight = info.dwYSize;
		}
	      else
		{
		  RECT r;

		  SystemParametersInfoA(SPI_GETWORKAREA, 0, &r, 0);
		  nWidth = (((r.right - r.left) * 3) / 4) - x;
		  nHeight = (((r.bottom - r.top) * 3) / 4) - y;
		}
	    }
	}
    }

  Handle = NtUserCreateWindowEx(dwExStyle,
				&ClassName,
				&WindowName,
				dwStyle,
				x,
				y,
				nWidth,
				nHeight,
				hWndParent,
				hMenu,
				hInstance,
				lpParam,
				sw);

  RtlFreeUnicodeString(&WindowName);

  if (!IS_ATOM(lpClassName)) 
    {
      RtlFreeUnicodeString(&ClassName);
    }
  
  return Handle;
}

HWND STDCALL
CreateWindowExW(DWORD dwExStyle,
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
  UNICODE_STRING WindowName;
  UNICODE_STRING ClassName;
  HANDLE Handle;
  UINT sw;

  if (IS_ATOM(lpClassName)) 
    {
      RtlInitUnicodeString(&ClassName, NULL);
      ClassName.Buffer = (LPWSTR)lpClassName;
    } 
  else 
    {
      RtlInitUnicodeString(&ClassName, lpClassName);
    }

  RtlInitUnicodeString(&WindowName, lpWindowName);

  /* Fixup default coordinates. */
  sw = SW_SHOW;
  if (x == (LONG) CW_USEDEFAULT || nWidth == (LONG) CW_USEDEFAULT)
    {
      if (dwStyle & (WS_CHILD | WS_POPUP))
	{
	  if (x == (LONG) CW_USEDEFAULT)
	    {
	      x = y = 0;
	    }
	  if (nWidth == (LONG) CW_USEDEFAULT)
	    {
	      nWidth = nHeight = 0;
	    }
	}
      else
	{
	  STARTUPINFOW info;

	  GetStartupInfoW(&info);

	  if (x == (LONG) CW_USEDEFAULT)
	    {
	      if (y != (LONG) CW_USEDEFAULT)
		{
		  sw = y;
		}
	      x = (info.dwFlags & STARTF_USEPOSITION) ? info.dwX : 0;
	      y = (info.dwFlags & STARTF_USEPOSITION) ? info.dwY : 0;
	    }
	  
	  if (nWidth == (LONG) CW_USEDEFAULT)
	    {
	      if (info.dwFlags & STARTF_USESIZE)
		{
		  nWidth = info.dwXSize;
		  nHeight = info.dwYSize;
		}
	      else
		{
		  RECT r;

		  SystemParametersInfoW(SPI_GETWORKAREA, 0, &r, 0);
		  nWidth = (((r.right - r.left) * 3) / 4) - x;
		  nHeight = (((r.bottom - r.top) * 3) / 4) - y;
		}
	    }
	}
    }

  Handle = NtUserCreateWindowEx(dwExStyle,
				&ClassName,
				&WindowName,
				dwStyle,
				x,
				y,
				nWidth,
				nHeight,
				hWndParent,
				hMenu,
				hInstance,
				lpParam,
				0);

  return (HWND)Handle;
}

HDWP STDCALL
DeferWindowPos(HDWP hWinPosInfo,
	       HWND hWnd,
	       HWND hWndInsertAfter,
	       int x,
	       int y,
	       int cx,
	       int cy,
	       UINT uFlags)
{
  UNIMPLEMENTED;
  return (HDWP)0;
}

WINBOOL STDCALL
DestroyWindow(HWND hWnd)
{
  SendMessageW(hWnd, WM_DESTROY, 0, 0);
  SendMessageW(hWnd, WM_NCDESTROY, 0, 0);

  return NtUserDestroyWindow(hWnd);
}

WINBOOL STDCALL
EndDeferWindowPos(HDWP hWinPosInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
EnumChildWindows(HWND hWndParent,
		 ENUMWINDOWSPROC lpEnumFunc,
		 LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
EnumThreadWindows(DWORD dwThreadId,
		  ENUMWINDOWSPROC lpfn,
		  LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
EnumWindows(ENUMWINDOWSPROC lpEnumFunc,
	    LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}

HWND STDCALL
FindWindowA(LPCSTR lpClassName, LPCSTR lpWindowName)
{
  //FIXME: FindWindow does not search children, but FindWindowEx does.
  //       what should we do about this?
  return FindWindowExA (NULL, NULL, lpClassName, lpWindowName);
}

HWND STDCALL
FindWindowExA(HWND hwndParent,
	      HWND hwndChildAfter,
	      LPCSTR lpszClass,
	      LPCSTR lpszWindow)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

HWND STDCALL
FindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName)
{
  //FIXME: FindWindow does not search children, but FindWindowEx does.
  //       what should we do about this?
  return FindWindowExW (NULL, NULL, lpClassName, lpWindowName);
}

HWND STDCALL
FindWindowExW(HWND hwndParent,
	      HWND hwndChildAfter,
	      LPCWSTR lpszClass,
	      LPCWSTR lpszWindow)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

WINBOOL STDCALL
GetAltTabInfo(HWND hwnd,
	      int iItem,
	      PALTTABINFO pati,
	      LPTSTR pszItemText,
	      UINT cchItemText)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
GetAltTabInfoA(HWND hwnd,
	       int iItem,
	       PALTTABINFO pati,
	       LPSTR pszItemText,
	       UINT cchItemText)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
GetAltTabInfoW(HWND hwnd,
	       int iItem,
	       PALTTABINFO pati,
	       LPWSTR pszItemText,
	       UINT cchItemText)
{
  UNIMPLEMENTED;
  return FALSE;
}

HWND STDCALL
GetAncestor(HWND hwnd, UINT gaFlags)
{
  return(NtUserGetAncestor(hwnd, gaFlags));
}

WINBOOL STDCALL
GetClientRect(HWND hWnd, LPRECT lpRect)
{
  return(NtUserGetClientRect(hWnd, lpRect));
}

HWND STDCALL
GetDesktopWindow(VOID)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

HWND STDCALL
GetForegroundWindow(VOID)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

WINBOOL STDCALL
GetGUIThreadInfo(DWORD idThread,
		 LPGUITHREADINFO lpgui)
{
  UNIMPLEMENTED;
  return FALSE;
}

HWND STDCALL
GetLastActivePopup(HWND hWnd)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

HWND STDCALL
GetParent(HWND hWnd)
{
  return NtUserGetAncestor(hWnd, GA_PARENT);
}

WINBOOL STDCALL
GetProcessDefaultLayout(DWORD *pdwDefaultLayout)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
GetTitleBarInfo(HWND hwnd,
		PTITLEBARINFO pti)
{
  UNIMPLEMENTED;
  return FALSE;
}

HWND STDCALL
GetTopWindow(HWND hWnd)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

HWND STDCALL
GetWindow(HWND hWnd,
	  UINT uCmd)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

WINBOOL STDCALL
GetWindowInfo(HWND hwnd,
	      PWINDOWINFO pwi)
{
  UNIMPLEMENTED;
  return FALSE;
}

UINT STDCALL
GetWindowModuleFileName(HWND hwnd,
			LPSTR lpszFileName,
			UINT cchFileNameMax)
{
  UNIMPLEMENTED;
  return 0;
}

UINT STDCALL
GetWindowModuleFileNameA(HWND hwnd,
			 LPSTR lpszFileName,
			 UINT cchFileNameMax)
{
  UNIMPLEMENTED;
  return 0;
}

UINT STDCALL
GetWindowModuleFileNameW(HWND hwnd,
			 LPWSTR lpszFileName,
			 UINT cchFileNameMax)
{
  UNIMPLEMENTED;
  return 0;
}

WINBOOL STDCALL
GetWindowPlacement(HWND hWnd,
		   WINDOWPLACEMENT *lpwndpl)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
GetWindowRect(HWND hWnd,
	      LPRECT lpRect)
{
  return(NtUserGetWindowRect(hWnd, lpRect));
}

int STDCALL
GetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount)
{
  return(SendMessageA(hWnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString));
}

int STDCALL
GetWindowTextLengthA(HWND hWnd)
{
  return(SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0));
}

int STDCALL
GetWindowTextLengthW(HWND hWnd)
{
  UNIMPLEMENTED;
  return 0;
}

int STDCALL
GetWindowTextW(HWND hWnd,
	       LPWSTR lpString,
	       int nMaxCount)
{
  UNIMPLEMENTED;
  return 0;
}

DWORD STDCALL
GetWindowThreadProcessId(HWND hWnd,
			 LPDWORD lpdwProcessId)
{
  UNIMPLEMENTED;
  return 0;
}

WINBOOL STDCALL
IsChild(HWND hWndParent,
	HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
IsIconic(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
IsWindow(HWND hWnd)
{
  DWORD WndProc = NtUserGetWindowLong(hWnd, GWL_WNDPROC);

  return (0 != WndProc || ERROR_INVALID_HANDLE != GetLastError());
}

WINBOOL STDCALL
IsWindowUnicode(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
IsWindowVisible(HWND hWnd)
{
  while (GetWindowLong(hWnd, GWL_STYLE) & WS_CHILD)
    {
      if (!(GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE))
	{
	  return(FALSE);
	}
      hWnd = GetAncestor(hWnd, GA_PARENT);
    }
  return(GetWindowLong(hWnd, GWL_STYLE) & WS_VISIBLE);
}

WINBOOL STDCALL
IsZoomed(HWND hWnd)
{
  ULONG uStyle = GetWindowLong(hWnd, GWL_STYLE);
  
  return (uStyle & WS_MAXIMIZE);
}

WINBOOL STDCALL
LockSetForegroundWindow(UINT uLockCode)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
MoveWindow(HWND hWnd,
	   int X,
	   int Y,
	   int nWidth,
	   int nHeight,
	   WINBOOL bRepaint)
{
  return NtUserMoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);
}

WINBOOL STDCALL
OpenIcon(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

HWND STDCALL
RealChildWindowFromPoint(HWND hwndParent,
			 POINT ptParentClientCoords)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

UINT
RealGetWindowClass(HWND  hwnd,
		   LPTSTR pszType,
		   UINT  cchType)
{
  UNIMPLEMENTED;
  return 0;
}

WINBOOL STDCALL
SetForegroundWindow(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
SetLayeredWindowAttributes(HWND hwnd,
			   COLORREF crKey,
			   BYTE bAlpha,
			   DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}

HWND STDCALL
SetParent(HWND hWndChild,
	  HWND hWndNewParent)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

WINBOOL STDCALL
SetProcessDefaultLayout(DWORD dwDefaultLayout)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
SetWindowPlacement(HWND hWnd,
		   CONST WINDOWPLACEMENT *lpwndpl)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
SetWindowPos(HWND hWnd,
	     HWND hWndInsertAfter,
	     int X,
	     int Y,
	     int cx,
	     int cy,
	     UINT uFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
SetWindowTextA(HWND hWnd,
	       LPCSTR lpString)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
SetWindowTextW(HWND hWnd,
	       LPCWSTR lpString)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
ShowOwnedPopups(HWND hWnd,
		WINBOOL fShow)
{
  UNIMPLEMENTED;
  return FALSE;
}

WINBOOL STDCALL
ShowWindow(HWND hWnd,
	   int nCmdShow)
{
  return NtUserShowWindow(hWnd, nCmdShow);
}

WINBOOL STDCALL
ShowWindowAsync(HWND hWnd,
		int nCmdShow)
{
  UNIMPLEMENTED;
  return FALSE;
}

WORD STDCALL
TileWindows(HWND hwndParent,
	    UINT wHow,
	    CONST RECT *lpRect,
	    UINT cKids,
	    const HWND *lpKids)
{
  UNIMPLEMENTED;
  return 0;
}

WINBOOL STDCALL
UpdateLayeredWindow(HWND hwnd,
		    HDC hdcDst,
		    POINT *pptDst,
		    SIZE *psize,
		    HDC hdcSrc,
		    POINT *pptSrc,
		    COLORREF crKey,
		    BLENDFUNCTION *pblend,
		    DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}

HWND STDCALL
WindowFromPoint(POINT Point)
{
  UNIMPLEMENTED;
  return (HWND)0;
}

int STDCALL
MapWindowPoints(HWND hWndFrom, HWND hWndTo, LPPOINT lpPoints, UINT cPoints)
{
  POINT FromOffset, ToOffset;
  LONG XMove, YMove;
  ULONG i;

  NtUserGetClientOrigin(hWndFrom, &FromOffset);
  NtUserGetClientOrigin(hWndTo, &ToOffset);
  XMove = FromOffset.x - ToOffset.x;
  YMove = FromOffset.y - ToOffset.y;

  for (i = 0; i < cPoints; i++)
    {
      lpPoints[i].x += XMove;
      lpPoints[i].y += YMove;
    }
  return(MAKELONG(LOWORD(XMove), LOWORD(YMove))); 
}


WINBOOL STDCALL 
ScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
  return(MapWindowPoints(NULL, hWnd, lpPoint, 1));
}

/* EOF */
