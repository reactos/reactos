/* $Id: window.c,v 1.43 2003/07/10 21:04:32 chorns Exp $
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
#include <user32/regcontrol.h>

#define NDEBUG
#include <debug.h>

static BOOL ControlsInitCalled = FALSE;

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
User32SendWINDOWPOSCHANGINGMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDWINDOWPOSCHANGING_CALLBACK_ARGUMENTS CallbackArgs;
  WNDPROC Proc;
  LRESULT Result;

  DPRINT("User32SendWINDOWPOSCHANGINGMessageForKernel.\n");
  CallbackArgs = (PSENDWINDOWPOSCHANGING_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDWINDOWPOSCHANGING_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)GetWindowLongW(CallbackArgs->Wnd, GWL_WNDPROC);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_WINDOWPOSCHANGING, 0, 
			   (LPARAM)&CallbackArgs->WindowPos);
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}


NTSTATUS STDCALL
User32SendWINDOWPOSCHANGEDMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDWINDOWPOSCHANGED_CALLBACK_ARGUMENTS CallbackArgs;
  WNDPROC Proc;
  LRESULT Result;

  DPRINT("User32SendWINDOWPOSCHANGEDMessageForKernel.\n");
  CallbackArgs = (PSENDWINDOWPOSCHANGED_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDWINDOWPOSCHANGED_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)GetWindowLongW(CallbackArgs->Wnd, GWL_WNDPROC);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_WINDOWPOSCHANGED, 0, 
			   (LPARAM)&CallbackArgs->WindowPos);
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}


NTSTATUS STDCALL
User32SendSTYLECHANGINGMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDSTYLECHANGING_CALLBACK_ARGUMENTS CallbackArgs;
  WNDPROC Proc;
  LRESULT Result;

  DPRINT("User32SendSTYLECHANGINGMessageForKernel.\n");
  CallbackArgs = (PSENDSTYLECHANGING_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDSTYLECHANGING_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)GetWindowLongW(CallbackArgs->Wnd, GWL_WNDPROC);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_STYLECHANGING, CallbackArgs->WhichStyle,
			   (LPARAM)&CallbackArgs->Style);
  DPRINT("Returning result %d.\n", Result);
  return(ZwCallbackReturn(&Result, sizeof(LRESULT), STATUS_SUCCESS));
}


NTSTATUS STDCALL
User32SendSTYLECHANGEDMessageForKernel(PVOID Arguments, ULONG ArgumentLength)
{
  PSENDSTYLECHANGED_CALLBACK_ARGUMENTS CallbackArgs;
  WNDPROC Proc;
  LRESULT Result;

  DPRINT("User32SendSTYLECHANGEDGMessageForKernel.\n");
  CallbackArgs = (PSENDSTYLECHANGED_CALLBACK_ARGUMENTS)Arguments;
  if (ArgumentLength != sizeof(SENDSTYLECHANGED_CALLBACK_ARGUMENTS))
    {
      DPRINT("Wrong length.\n");
      return(STATUS_INFO_LENGTH_MISMATCH);
    }
  Proc = (WNDPROC)GetWindowLongW(CallbackArgs->Wnd, GWL_WNDPROC);
  DPRINT("Proc %X\n", Proc);
  /* Call the window procedure; notice kernel messages are always unicode. */
  Result = CallWindowProcW(Proc, CallbackArgs->Wnd, WM_STYLECHANGED, CallbackArgs->WhichStyle,
			   (LPARAM)&CallbackArgs->Style);
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


/*
 * @implemented
 */
WINBOOL STDCALL
AdjustWindowRect(LPRECT lpRect,
		 DWORD dwStyle,
		 WINBOOL bMenu)
{
  return(AdjustWindowRectEx(lpRect, dwStyle, bMenu, 0));
}


/*
 * @implemented
 */
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


/*
 * @unimplemented
 */
WINBOOL STDCALL
AllowSetForegroundWindow(DWORD dwProcessId)
{
  UNIMPLEMENTED;
  return(FALSE);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
AnimateWindow(HWND hwnd,
	      DWORD dwTime,
	      DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
UINT STDCALL
ArrangeIconicWindows(HWND hWnd)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
HDWP STDCALL
BeginDeferWindowPos(int nNumWindows)
{
  UNIMPLEMENTED;
  return (HDWP)0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
BringWindowToTop(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
HWND STDCALL
ChildWindowFromPoint(HWND hWndParent,
		     POINT Point)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
HWND STDCALL
ChildWindowFromPointEx(HWND hwndParent,
		       POINT pt,
		       UINT uFlags)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @implemented
 */
WINBOOL STDCALL
CloseWindow(HWND hWnd)
{
    SendMessageA(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);

    return (WINBOOL)(hWnd);
}

/*
 * @implemented
 */
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

  /* Register built-in controls if not already done */
  if (! ControlsInitCalled)
    {
      ControlsInit();
      ControlsInitCalled = TRUE;
    }

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


/*
 * @implemented
 */
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

  /* Register built-in controls if not already done */
  if (! ControlsInitCalled)
    {
      ControlsInit();
      ControlsInitCalled = TRUE;
    }

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


/*
 * @unimplemented
 */
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


/*
 * @implemented
 */
WINBOOL STDCALL
DestroyWindow(HWND hWnd)
{
  return NtUserDestroyWindow(hWnd);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
EndDeferWindowPos(HDWP hWinPosInfo)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
EnumChildWindows(HWND hWndParent,
		 ENUMWINDOWSPROC lpEnumFunc,
		 LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
EnumThreadWindows(DWORD dwThreadId,
		  ENUMWINDOWSPROC lpfn,
		  LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
EnumWindows(ENUMWINDOWSPROC lpEnumFunc,
	    LPARAM lParam)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
HWND STDCALL
FindWindowA(LPCSTR lpClassName, LPCSTR lpWindowName)
{
  //FIXME: FindWindow does not search children, but FindWindowEx does.
  //       what should we do about this?
  return FindWindowExA (NULL, NULL, lpClassName, lpWindowName);
}


/*
 * @unimplemented
 */
HWND STDCALL
FindWindowExA(HWND hwndParent,
	      HWND hwndChildAfter,
	      LPCSTR lpszClass,
	      LPCSTR lpszWindow)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @implemented
 */
HWND STDCALL
FindWindowW(LPCWSTR lpClassName, LPCWSTR lpWindowName)
{
  /* 
  
  There was a FIXME here earlier, but I think it is just a documentation unclarity.

  FindWindow only searches top level windows. What they mean is that child 
  windows of other windows than the desktop can be searched. 
  FindWindowExW never does a recursive search.
  
	/ Joakim
  */

  return FindWindowExW (NULL, NULL, lpClassName, lpWindowName);
}


/*
 * @implemented
 */
HWND STDCALL
FindWindowExW(HWND hwndParent,
	      HWND hwndChildAfter,
	      LPCWSTR lpszClass,
	      LPCWSTR lpszWindow)
{
	UNICODE_STRING ucClassName;
	UNICODE_STRING ucWindowName;

	if (IS_ATOM(lpszClass)) 
	{
		RtlInitUnicodeString(&ucClassName, NULL);
		ucClassName.Buffer = (LPWSTR)lpszClass;
    } 
	else 
    {
		RtlInitUnicodeString(&ucClassName, lpszClass);
    }

	// Window names can't be atoms, and if lpszWindow = NULL,
	// RtlInitUnicodeString will clear it
	
	RtlInitUnicodeString(&ucWindowName, lpszWindow);


	return NtUserFindWindowEx(hwndParent, hwndChildAfter, &ucClassName, &ucWindowName);
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @implemented
 */
HWND STDCALL
GetAncestor(HWND hwnd, UINT gaFlags)
{
  return(NtUserGetAncestor(hwnd, gaFlags));
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetClientRect(HWND hWnd, LPRECT lpRect)
{
  return(NtUserGetClientRect(hWnd, lpRect));
}


/*
 * @implemented
 */
HWND STDCALL
GetDesktopWindow(VOID)
{
	return NtUserGetDesktopWindow();
}


/*
 * @unimplemented
 */
HWND STDCALL
GetForegroundWindow(VOID)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetGUIThreadInfo(DWORD idThread,
		 LPGUITHREADINFO lpgui)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HWND STDCALL
GetLastActivePopup(HWND hWnd)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @implemented
 */
HWND STDCALL
GetParent(HWND hWnd)
{
  return NtUserGetAncestor(hWnd, GA_PARENT);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetProcessDefaultLayout(DWORD *pdwDefaultLayout)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetTitleBarInfo(HWND hwnd,
		PTITLEBARINFO pti)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HWND STDCALL
GetTopWindow(HWND hWnd)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
HWND STDCALL
GetWindow(HWND hWnd,
	  UINT uCmd)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetWindowInfo(HWND hwnd,
	      PWINDOWINFO pwi)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
UINT STDCALL
GetWindowModuleFileName(HWND hwnd,
			LPSTR lpszFileName,
			UINT cchFileNameMax)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT STDCALL
GetWindowModuleFileNameA(HWND hwnd,
			 LPSTR lpszFileName,
			 UINT cchFileNameMax)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT STDCALL
GetWindowModuleFileNameW(HWND hwnd,
			 LPWSTR lpszFileName,
			 UINT cchFileNameMax)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
GetWindowPlacement(HWND hWnd,
		   WINDOWPLACEMENT *lpwndpl)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
GetWindowRect(HWND hWnd,
	      LPRECT lpRect)
{
  return(NtUserGetWindowRect(hWnd, lpRect));
}


/*
 * @implemented
 */
int STDCALL
GetWindowTextA(HWND hWnd, LPSTR lpString, int nMaxCount)
{
  return(SendMessageA(hWnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString));
}


/*
 * @implemented
 */
int STDCALL
GetWindowTextLengthA(HWND hWnd)
{
  return(SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0));
}


/*
 * @unimplemented
 */
int STDCALL
GetWindowTextLengthW(HWND hWnd)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
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
	if(lpdwProcessId) *lpdwProcessId = NtUserQueryWindow(hWnd, 0);
	return(NtUserQueryWindow(hWnd, 1));
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
IsChild(HWND hWndParent,
	HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
IsIconic(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
IsWindow(HWND hWnd)
{
  DWORD WndProc = NtUserGetWindowLong(hWnd, GWL_WNDPROC);

  return (0 != WndProc || ERROR_INVALID_HANDLE != GetLastError());
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
IsWindowUnicode(HWND hWnd)
{
#ifdef TODO
  UNIMPLEMENTED;
#endif
  return FALSE;
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
WINBOOL STDCALL
IsZoomed(HWND hWnd)
{
  ULONG uStyle = GetWindowLong(hWnd, GWL_STYLE);
  
  return (uStyle & WS_MAXIMIZE);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
LockSetForegroundWindow(UINT uLockCode)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
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


/*
 * @unimplemented
 */
WINBOOL STDCALL
OpenIcon(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HWND STDCALL
RealChildWindowFromPoint(HWND hwndParent,
			 POINT ptParentClientCoords)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
UINT
RealGetWindowClass(HWND  hwnd,
		   LPTSTR pszType,
		   UINT  cchType)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetForegroundWindow(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetLayeredWindowAttributes(HWND hwnd,
			   COLORREF crKey,
			   BYTE bAlpha,
			   DWORD dwFlags)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
HWND STDCALL
SetParent(HWND hWndChild,
	  HWND hWndNewParent)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetProcessDefaultLayout(DWORD dwDefaultLayout)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
SetWindowPlacement(HWND hWnd,
		   CONST WINDOWPLACEMENT *lpwndpl)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetWindowPos(HWND hWnd,
	     HWND hWndInsertAfter,
	     int X,
	     int Y,
	     int cx,
	     int cy,
	     UINT uFlags)
{
  return NtUserSetWindowPos(hWnd,hWndInsertAfter, X, Y, cx, cy, uFlags);
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetWindowTextA(HWND hWnd,
	       LPCSTR lpString)
{
  return SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)lpString);
}


/*
 * @implemented
 */
WINBOOL STDCALL
SetWindowTextW(HWND hWnd,
	       LPCWSTR lpString)
{
  return SendMessageW(hWnd, WM_SETTEXT, 0, (LPARAM)lpString);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
ShowOwnedPopups(HWND hWnd,
		WINBOOL fShow)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
WINBOOL STDCALL
ShowWindow(HWND hWnd,
	   int nCmdShow)
{
  return NtUserShowWindow(hWnd, nCmdShow);
}


/*
 * @unimplemented
 */
WINBOOL STDCALL
ShowWindowAsync(HWND hWnd,
		int nCmdShow)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
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


/*
 * @unimplemented
 */
HWND STDCALL
WindowFromPoint(POINT Point)
{
  UNIMPLEMENTED;
  return (HWND)0;
}


/*
 * @implemented
 */
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


/*
 * @implemented
 */
WINBOOL STDCALL 
ScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
  return(MapWindowPoints(NULL, hWnd, lpPoint, 1));
}

/* EOF */
