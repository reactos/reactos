/* $Id: window.c,v 1.115 2004/05/02 17:25:21 weiden Exp $
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
#include <string.h>
#include <strpool.h>
#include <user32/callback.h>
#include <user32/regcontrol.h>
#define NTOS_MODE_USER
#include <ntos.h>

#define NDEBUG
#include <debug.h>

BOOL ControlsInitialized = FALSE;

LRESULT DefWndNCPaint(HWND hWnd, HRGN hRgn);

/* FUNCTIONS *****************************************************************/


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


/*
 * @unimplemented
 */
BOOL STDCALL
AllowSetForegroundWindow(DWORD dwProcessId)
{
  UNIMPLEMENTED;
  return(FALSE);
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
#if 0
  UNIMPLEMENTED;
  return (HDWP)0;
#else
  return (HDWP)1;
#endif
}


/*
 * @unimplemented
 */
BOOL STDCALL
BringWindowToTop(HWND hWnd)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
/*
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
*/


/*
 * @implemented
 */
HWND STDCALL
ChildWindowFromPoint(HWND hWndParent,
		     POINT Point)
{
  return (HWND) NtUserChildWindowFromPointEx(hWndParent, Point.x, Point.y, 0);
}


/*
 * @implemented
 */
HWND STDCALL
ChildWindowFromPointEx(HWND hwndParent,
		       POINT pt,
		       UINT uFlags)
{
  return (HWND) NtUserChildWindowFromPointEx(hwndParent, pt.x, pt.y, uFlags);
}


/*
 * @implemented
 */
BOOL STDCALL
CloseWindow(HWND hWnd)
{
    SendMessageA(hWnd, WM_SYSCOMMAND, SC_CLOSE, 0);

    return (BOOL)(hWnd);
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
  WNDCLASSEXA wce;
  HWND Handle;
  INT sw;

#if 0
  DbgPrint("[window] CreateWindowExA style %d, exstyle %d, parent %d\n", dwStyle, dwExStyle, hWndParent);
#endif

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

  /* Register built-in controls if not already done */
  if (! ControlsInitialized)
    {
      ControlsInitialized = ControlsInit(ClassName.Buffer);
    }

  if (dwExStyle & WS_EX_MDICHILD)
  {
     if (!IS_ATOM(lpClassName))
        RtlFreeUnicodeString(&ClassName);
     return CreateMDIWindowA(lpClassName, lpWindowName, dwStyle, x, y,
        nWidth, nHeight, hWndParent, hInstance, (LPARAM)lpParam);
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
    
  if(!hMenu && (dwStyle & (WS_OVERLAPPEDWINDOW | WS_POPUP)))
  {
    wce.cbSize = sizeof(WNDCLASSEXA);
    if(GetClassInfoExA(hInstance, lpClassName, &wce) && wce.lpszMenuName)
    {
      hMenu = LoadMenuA(hInstance, wce.lpszMenuName);
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
				sw,
				FALSE);

#if 0
  DbgPrint("[window] NtUserCreateWindowEx() == %d\n", Handle);
#endif

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
  WNDCLASSEXW wce;
  HANDLE Handle;
  UINT sw;

  /* Register built-in controls if not already done */
  if (! ControlsInitialized)
    {
      ControlsInitialized = ControlsInit(lpClassName);
    }

  if (dwExStyle & WS_EX_MDICHILD)
     return CreateMDIWindowW(lpClassName, lpWindowName, dwStyle, x, y,
        nWidth, nHeight, hWndParent, hInstance, (LPARAM)lpParam);

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

  if(!hMenu && (dwStyle & (WS_OVERLAPPEDWINDOW | WS_POPUP)))
  {
    wce.cbSize = sizeof(WNDCLASSEXW);
    if(GetClassInfoExW(hInstance, lpClassName, &wce) && wce.lpszMenuName)
    {
      hMenu = LoadMenuW(hInstance, wce.lpszMenuName);
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
				sw,
				TRUE);

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
#if 0
  return NtUserDeferWindowPos(hWinPosInfo, hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
#else
  SetWindowPos(hWnd, hWndInsertAfter, x, y, cx, cy, uFlags);
  return hWinPosInfo;
#endif
}


/*
 * @implemented
 */
BOOL STDCALL
DestroyWindow(HWND hWnd)
{
  return NtUserDestroyWindow(hWnd);
}


/*
 * @unimplemented
 */
BOOL STDCALL
EndDeferWindowPos(HDWP hWinPosInfo)
{
#if 0
  UNIMPLEMENTED;
  return FALSE;
#else
  return TRUE;
#endif
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
   return NtUserGetForegroundWindow();
}


BOOL
STATIC
User32EnumWindows (
	HDESK hDesktop,
	HWND hWndparent,
	ENUMWINDOWSPROC lpfn,
	LPARAM lParam,
	DWORD dwThreadId,
	BOOL bChildren )
{
  DWORD i, dwCount = 0;
  HWND* pHwnd = NULL;
  HANDLE hHeap;

  if ( !lpfn )
    {
      SetLastError ( ERROR_INVALID_PARAMETER );
      return FALSE;
    }

  /* FIXME instead of always making two calls, should we use some
     sort of persistent buffer and only grow it ( requiring a 2nd
     call ) when the buffer wasn't already big enough? */
  /* first get how many window entries there are */
  SetLastError(0);
  dwCount = NtUserBuildHwndList (
    hDesktop, hWndparent, bChildren, dwThreadId, lParam, NULL, 0 );
  if ( !dwCount || GetLastError() )
    return FALSE;

  /* allocate buffer to receive HWND handles */
  hHeap = GetProcessHeap();
  pHwnd = HeapAlloc ( hHeap, 0, sizeof(HWND)*(dwCount+1) );
  if ( !pHwnd )
    {
      SetLastError ( ERROR_NOT_ENOUGH_MEMORY );
      return FALSE;
    }

  /* now call kernel again to fill the buffer this time */
  dwCount = NtUserBuildHwndList (
    hDesktop, hWndparent, bChildren, dwThreadId, lParam, pHwnd, dwCount );
  if ( !dwCount || GetLastError() )
    {
      if ( pHwnd )
	HeapFree ( hHeap, 0, pHwnd );
      return FALSE;
    }

  /* call the user's callback function until we're done or
     they tell us to quit */
  for ( i = 0; i < dwCount; i++ )
  {
    /* FIXME I'm only getting NULLs from Thread Enumeration, and it's
     * probably because I'm not doing it right in NtUserBuildHwndList.
     * Once that's fixed, we shouldn't have to check for a NULL HWND
     * here
     */
    if ( !(ULONG)pHwnd[i] ) /* don't enumerate a NULL HWND */
      continue;
    if ( !(*lpfn)( pHwnd[i], lParam ) )
    {
      HeapFree ( hHeap, 0, pHwnd );
      return FALSE;
    }
  }
  if ( pHwnd )
    HeapFree ( hHeap, 0, pHwnd );
  return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumChildWindows(
	HWND hWndParent,
	ENUMWINDOWSPROC lpEnumFunc,
	LPARAM lParam)
{
  if ( !hWndParent )
    hWndParent = GetDesktopWindow();
  return User32EnumWindows ( NULL, hWndParent, lpEnumFunc, lParam, 0, FALSE );
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumThreadWindows(DWORD dwThreadId,
		  ENUMWINDOWSPROC lpfn,
		  LPARAM lParam)
{
  if ( !dwThreadId )
    dwThreadId = GetCurrentThreadId();
  return User32EnumWindows ( NULL, NULL, lpfn, lParam, dwThreadId, FALSE );
}


/*
 * @implemented
 */
BOOL STDCALL
EnumWindows(ENUMWINDOWSPROC lpEnumFunc,
	    LPARAM lParam)
{
  return User32EnumWindows ( NULL, NULL, lpEnumFunc, lParam, 0, FALSE );
}


/*
 * @implemented
 */
BOOL
STDCALL
EnumDesktopWindows(
	HDESK hDesktop,
	ENUMWINDOWSPROC lpfn,
	LPARAM lParam)
{
  return User32EnumWindows ( hDesktop, NULL, lpfn, lParam, 0, FALSE );
}


/*
 * @implemented
 */
HWND STDCALL
FindWindowExA(HWND hwndParent,
	      HWND hwndChildAfter,
	      LPCSTR lpszClass,
	      LPCSTR lpszWindow)
{
   UNICODE_STRING ucClassName;
   UNICODE_STRING ucWindowName;
   HWND Result;

   if (lpszClass == NULL)
   {
      ucClassName.Buffer = NULL;
      ucClassName.Length = 0;
   }
   else if (IS_ATOM(lpszClass)) 
   {
      ucClassName.Buffer = (LPWSTR)lpszClass;
      ucClassName.Length = 0;
   } 
   else 
   {
      RtlCreateUnicodeStringFromAsciiz(&ucClassName, (LPSTR)lpszClass);
   }

   RtlCreateUnicodeStringFromAsciiz(&ucWindowName, (LPSTR)lpszWindow);

   Result = NtUserFindWindowEx(hwndParent, hwndChildAfter, &ucClassName,
      &ucWindowName);

   if (!IS_ATOM(lpszClass)) 
      RtlFreeUnicodeString(&ucClassName);
   RtlFreeUnicodeString(&ucWindowName);

   return Result;
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

   if (lpszClass == NULL)
   {
      ucClassName.Buffer = NULL;
      ucClassName.Length = 0;
   }
   else if (IS_ATOM(lpszClass)) 
   {
      RtlInitUnicodeString(&ucClassName, NULL);
      ucClassName.Buffer = (LPWSTR)lpszClass;
   } 
   else 
   {
      RtlInitUnicodeString(&ucClassName, lpszClass);
   }
	
   RtlInitUnicodeString(&ucWindowName, lpszWindow);

   return NtUserFindWindowEx(hwndParent, hwndChildAfter, &ucClassName, &ucWindowName);
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
 * @unimplemented
 */
BOOL STDCALL
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
BOOL STDCALL
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
BOOL STDCALL
GetClientRect(HWND hWnd, LPRECT lpRect)
{
  return(NtUserGetClientRect(hWnd, lpRect));
}


/*
 * @implemented
 */
BOOL STDCALL
GetGUIThreadInfo(DWORD idThread,
		 LPGUITHREADINFO lpgui)
{
  return (BOOL)NtUserGetGUIThreadInfo(idThread, lpgui);
}


/*
 * @unimplemented
 */
HWND STDCALL
GetLastActivePopup(HWND hWnd)
{
  return NtUserGetLastActivePopup(hWnd);
}


/*
 * @implemented
 */
HWND STDCALL
GetParent(HWND hWnd)
{
  return NtUserGetParent(hWnd);
}


/*
 * @unimplemented
 */
BOOL STDCALL
GetProcessDefaultLayout(DWORD *pdwDefaultLayout)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
GetTitleBarInfo(HWND hwnd,
		PTITLEBARINFO pti)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
HWND STDCALL
GetWindow(HWND hWnd,
	  UINT uCmd)
{
  return NtUserGetWindow(hWnd, uCmd);
}


/*
 * @implemented
 */
HWND STDCALL
GetTopWindow(HWND hWnd)
{
  if (!hWnd) hWnd = GetDesktopWindow();
  return GetWindow( hWnd, GW_CHILD );
}


/*
 * @implemented
 */
BOOL STDCALL
GetWindowInfo(HWND hwnd,
	      PWINDOWINFO pwi)
{
  return NtUserGetWindowInfo(hwnd, pwi);
}


/*
 * @implemented
 */
UINT STDCALL
GetWindowModuleFileNameA(HWND hwnd,
			 LPSTR lpszFileName,
			 UINT cchFileNameMax)
{
  HINSTANCE hWndInst;
  
  if(!(hWndInst = NtUserGetWindowInstance(hwnd)))
  {
    return 0;
  }
  
  return GetModuleFileNameA(hWndInst, lpszFileName, cchFileNameMax);
}


/*
 * @implemented
 */
UINT STDCALL
GetWindowModuleFileNameW(HWND hwnd,
			 LPWSTR lpszFileName,
			 UINT cchFileNameMax)
{
  HINSTANCE hWndInst;
  
  if(!(hWndInst = NtUserGetWindowInstance(hwnd)))
  {
    return 0;
  }
  
  return GetModuleFileNameW(hWndInst, lpszFileName, cchFileNameMax);
}


/*
 * @implemented
 */
BOOL STDCALL
GetWindowPlacement(HWND hWnd,
		   WINDOWPLACEMENT *lpwndpl)
{
  return (BOOL)NtUserGetWindowPlacement(hWnd, lpwndpl);
}


/*
 * @implemented
 */
BOOL STDCALL
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
  DWORD ProcessId;
  if(!NtUserGetWindowThreadProcessId(hWnd, &ProcessId))
  {
    return 0;
  }
  
  if(ProcessId != GetCurrentProcessId())
  {
    /* do not send WM_GETTEXT messages to other processes */
    LPWSTR Buffer;
    INT Length;
    
    if (nMaxCount > 1)
    {
      *((PWSTR)lpString) = '\0';
    }
    Buffer = HeapAlloc(GetProcessHeap(), 0, nMaxCount * sizeof(WCHAR));
    if (!Buffer)
      return FALSE;
    Length = NtUserInternalGetWindowText(hWnd, Buffer, nMaxCount);
    if (Length > 0 && nMaxCount > 0 &&
        !WideCharToMultiByte(CP_ACP, 0, Buffer, -1,
        lpString, nMaxCount, NULL, NULL))
    {
      lpString[0] = '\0';
    }
    
    HeapFree(GetProcessHeap(), 0, Buffer);
    
    return (LRESULT)Length;
  }
  
  return(SendMessageA(hWnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString));
}


/*
 * @implemented
 */
int STDCALL
GetWindowTextLengthA(HWND hWnd)
{
  DWORD ProcessId;
  if(!NtUserGetWindowThreadProcessId(hWnd, &ProcessId))
  {
    return 0;
  }
  
  if(ProcessId == GetCurrentProcessId())
  {
    return(SendMessageA(hWnd, WM_GETTEXTLENGTH, 0, 0));
  }
  
  /* do not send WM_GETTEXT messages to other processes */
  return (LRESULT)NtUserInternalGetWindowText(hWnd, NULL, 0);
}


/*
 * @implemented
 */
int STDCALL
GetWindowTextLengthW(HWND hWnd)
{
  DWORD ProcessId;
  if(!NtUserGetWindowThreadProcessId(hWnd, &ProcessId))
  {
    return 0;
  }
  
  if(ProcessId == GetCurrentProcessId())
  {
    return(SendMessageW(hWnd, WM_GETTEXTLENGTH, 0, 0));
  }
  
  /* do not send WM_GETTEXT messages to other processes */
  return (LRESULT)NtUserInternalGetWindowText(hWnd, NULL, 0);
}


/*
 * @implemented
 */
int STDCALL
GetWindowTextW(
	HWND hWnd,
	LPWSTR lpString,
	int nMaxCount)
{
  DWORD ProcessId;
  if(!NtUserGetWindowThreadProcessId(hWnd, &ProcessId))
  {
    return 0;
  }
  
  if(ProcessId == GetCurrentProcessId())
  {
    return(SendMessageW(hWnd, WM_GETTEXT, nMaxCount, (LPARAM)lpString));
  }
  
  /* do not send WM_GETTEXT messages to other processes */
  if (nMaxCount > 1)
  {
    *((PWSTR)lpString) = L'\0';
  }
  
  return (LRESULT)NtUserInternalGetWindowText(hWnd, (PWSTR)lpString, nMaxCount);
}

DWORD STDCALL
GetWindowThreadProcessId(HWND hWnd,
			 LPDWORD lpdwProcessId)
{
   return NtUserGetWindowThreadProcessId(hWnd, lpdwProcessId);
}


/*
 * @implemented
 */
BOOL STDCALL
IsChild(HWND hWndParent,
	HWND hWnd)
{
   if (! IsWindow(hWndParent) || ! IsWindow(hWnd))
   {
       return FALSE;
   }

   do 
   {
      hWnd = (HWND)NtUserGetWindowLong(hWnd, GWL_HWNDPARENT, FALSE);
   }
   while (hWnd != NULL && hWnd != hWndParent);
    
   return hWnd == hWndParent;
}


/*
 * @implemented
 */
BOOL STDCALL
IsIconic(HWND hWnd)
{
  return (NtUserGetWindowLong( hWnd, GWL_STYLE, FALSE) & WS_MINIMIZE) != 0;  
}


/*
 * @implemented
 */
BOOL STDCALL
IsWindow(HWND hWnd)
{
  DWORD WndProc = NtUserGetWindowLong(hWnd, GWL_WNDPROC, FALSE);
  return (0 != WndProc || ERROR_INVALID_WINDOW_HANDLE != GetLastError());
}


/*
 * @implemented
 */
BOOL STDCALL
IsWindowUnicode(HWND hWnd)
{
	return NtUserIsWindowUnicode(hWnd);
}


/*
 * @implemented
 */
BOOL STDCALL
IsWindowVisible(HWND hWnd)
{
  while (NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_CHILD)
    {
      if (!(NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_VISIBLE))
	{
	  return(FALSE);
	}
      hWnd = GetAncestor(hWnd, GA_PARENT);
    }
  return(NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_VISIBLE);
}


/*
 * @implemented
 */
BOOL
STDCALL
IsWindowEnabled(
  HWND hWnd)
{
    // AG: I don't know if child windows are affected if the parent is
    // disabled. I think they stop processing messages but stay appearing
    // as enabled.

    return (! (NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_DISABLED));
}


/*
 * @implemented
 */
BOOL STDCALL
IsZoomed(HWND hWnd)
{
  return NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_MAXIMIZE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
LockSetForegroundWindow(UINT uLockCode)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
MoveWindow(HWND hWnd,
	   int X,
	   int Y,
	   int nWidth,
	   int nHeight,
	   BOOL bRepaint)
{
  return NtUserMoveWindow(hWnd, X, Y, nWidth, nHeight, bRepaint);
}


/*
 * @implemented
 */
BOOL STDCALL
AnimateWindow(HWND hwnd,
	      DWORD dwTime,
	      DWORD dwFlags)
{
  /* FIXME Add animation code */

  /* If trying to show/hide and it's already   *
   * shown/hidden or invalid window, fail with *
   * invalid parameter                         */
   
  BOOL visible;
  visible = IsWindowVisible(hwnd);
//  if(!IsWindow(hwnd) ||
//    (visible && !(dwFlags & AW_HIDE)) ||
//    (!visible && (dwFlags & AW_HIDE)))
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

//  ShowWindow(hwnd, (dwFlags & AW_HIDE) ? SW_HIDE : ((dwFlags & AW_ACTIVATE) ? SW_SHOW : SW_SHOWNA));

  return TRUE;
}


/*
 * @implemented
 */
BOOL STDCALL
OpenIcon(HWND hWnd)
{
    if (!(NtUserGetWindowLong(hWnd, GWL_STYLE, FALSE) & WS_MINIMIZE))
    {
        return FALSE;
    }

    ShowWindow(hWnd,SW_RESTORE);
    return TRUE;
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
BOOL STDCALL
SetForegroundWindow(HWND hWnd)
{
   return NtUserCallHwndLock(hWnd, HWNDLOCK_ROUTINE_SETFOREGROUNDWINDOW);
}


/*
 * @unimplemented
 */
BOOL STDCALL
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
  return NtUserSetParent(hWndChild, hWndNewParent);
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetProcessDefaultLayout(DWORD dwDefaultLayout)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetWindowPlacement(HWND hWnd,
		   CONST WINDOWPLACEMENT *lpwndpl)
{
  return (BOOL)NtUserSetWindowPlacement(hWnd, (WINDOWPLACEMENT *)lpwndpl);
}


/*
 * @implemented
 */
BOOL STDCALL
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
BOOL STDCALL
SetWindowTextA(HWND hWnd,
	       LPCSTR lpString)
{
  DWORD ProcessId;
  if(!NtUserGetWindowThreadProcessId(hWnd, &ProcessId))
  {
    return FALSE;
  }
  
  if(ProcessId != GetCurrentProcessId())
  {
    /* do not send WM_GETTEXT messages to other processes */
    ANSI_STRING AnsiString;
    UNICODE_STRING UnicodeString;
    
    if(lpString)
    {
      RtlInitAnsiString(&AnsiString, (LPSTR)lpString);
      RtlAnsiStringToUnicodeString(&UnicodeString, &AnsiString, TRUE);
      NtUserDefSetText(hWnd, &UnicodeString);
      RtlFreeUnicodeString(&UnicodeString);
    }
    else
      NtUserDefSetText(hWnd, NULL);
    
    if ((GetWindowLongW(hWnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION)
    {
      DefWndNCPaint(hWnd, (HRGN)1);
    }
    return TRUE;
  }
  
  return SendMessageA(hWnd, WM_SETTEXT, 0, (LPARAM)lpString);
}


/*
 * @implemented
 */
BOOL STDCALL
SetWindowTextW(HWND hWnd,
	       LPCWSTR lpString)
{
  DWORD ProcessId;
  if(!NtUserGetWindowThreadProcessId(hWnd, &ProcessId))
  {
    return FALSE;
  }
  
  if(ProcessId != GetCurrentProcessId())
  {
    /* do not send WM_GETTEXT messages to other processes */
    UNICODE_STRING UnicodeString;
    
    if(lpString)
      RtlInitUnicodeString(&UnicodeString, (LPWSTR)lpString);
    
    NtUserDefSetText(hWnd, (lpString ? &UnicodeString : NULL));
    
    if ((GetWindowLongW(hWnd, GWL_STYLE) & WS_CAPTION) == WS_CAPTION)
    {
      DefWndNCPaint(hWnd, (HRGN)1);
    }
    return TRUE;
  }
  
  return SendMessageW(hWnd, WM_SETTEXT, 0, (LPARAM)lpString);
}


/*
 * @unimplemented
 */
BOOL STDCALL
ShowOwnedPopups(HWND hWnd,
		BOOL fShow)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
ShowWindow(HWND hWnd,
	   int nCmdShow)
{
  return NtUserShowWindow(hWnd, nCmdShow);
}


/*
 * @unimplemented
 */
BOOL STDCALL
ShowWindowAsync(HWND hWnd,
		int nCmdShow)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
/*
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
*/


/*
 * @unimplemented
 */
BOOL STDCALL
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
 * @implemented
 */
HWND STDCALL
WindowFromPoint(POINT Point)
{
  //TODO: Determine what the actual parameters to 
  // NtUserWindowFromPoint are.
  return NtUserWindowFromPoint(Point.x, Point.y);
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

  if (hWndFrom == NULL)
  {
    FromOffset.x = FromOffset.y = 0;
  } else
  if(!NtUserGetClientOrigin(hWndFrom, &FromOffset))
  {
    return 0;
  }

  if (hWndTo == NULL)
  {
    ToOffset.x = ToOffset.y = 0;
  } else
  if(!NtUserGetClientOrigin(hWndTo, &ToOffset))
  {
    return 0;
  }
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
BOOL STDCALL 
ScreenToClient(HWND hWnd, LPPOINT lpPoint)
{
  return(MapWindowPoints(NULL, hWnd, lpPoint, 1) != 0);
}


/*
 * @implemented
 */
BOOL STDCALL
ClientToScreen(HWND hWnd, LPPOINT lpPoint)
{
    return (MapWindowPoints( hWnd, NULL, lpPoint, 1 ) != 0);
}


/*
 * @implemented
 */
BOOL
STDCALL
SetWindowContextHelpId(HWND hwnd,
          DWORD dwContextHelpId)
{
  return NtUserSetWindowContextHelpId(hwnd, dwContextHelpId);
}


/*
 * @implemented
 */
DWORD
STDCALL
GetWindowContextHelpId(HWND hwnd)
{
  return NtUserGetWindowContextHelpId(hwnd);
}

/*
 * @implemented
 */
int
STDCALL
InternalGetWindowText(HWND hWnd, LPWSTR lpString, int nMaxCount)
{
  return NtUserInternalGetWindowText(hWnd, lpString, nMaxCount);
}

/*
 * @implemented
 */
BOOL
STDCALL
IsHungAppWindow(HWND hwnd)
{
  return (NtUserQueryWindow(hwnd, QUERY_WINDOW_ISHUNG) != 0);
}

/*
 * @implemented
 */
VOID
STDCALL
SetLastErrorEx(DWORD dwErrCode, DWORD dwType)
{
  SetLastError(dwErrCode);
}

/*
 * @implemented
 */
HWND
STDCALL
GetFocus(VOID)
{
  return (HWND)NtUserGetThreadState(0);
}

/*
 * @unimplemented
 */
HWND
STDCALL
SetTaskmanWindow(HWND x)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HWND
STDCALL
SetProgmanWindow(HWND x)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HWND
STDCALL
GetProgmanWindow(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @unimplemented
 */
HWND
STDCALL
GetTaskmanWindow(VOID)
{
  UNIMPLEMENTED;
  return FALSE;
}

/*
 * @implemented
 */
BOOL STDCALL
ScrollWindow(HWND hWnd, int dx, int dy, CONST RECT *lpRect,
   CONST RECT *prcClip)
{
   return NtUserScrollWindowEx(hWnd, dx, dy, lpRect, prcClip, 0, NULL, 
      (lpRect ? 0 : SW_SCROLLCHILDREN) | SW_INVALIDATE) != ERROR;
}


/*
 * @implemented
 */
INT STDCALL
ScrollWindowEx(HWND hWnd, int dx, int dy, CONST RECT *prcScroll,
   CONST RECT *prcClip, HRGN hrgnUpdate, LPRECT prcUpdate, UINT flags)
{
   return NtUserScrollWindowEx(hWnd, dx, dy, prcScroll, prcClip, hrgnUpdate,
      prcUpdate, flags);
}

/*
 * @implemented
 */
BOOL
STDCALL
AnyPopup(VOID)
{
  return NtUserAnyPopup();
}

/*
 * @implemented
 */
BOOL
STDCALL
IsWindowInDestroy(HWND hWnd)
{
  return NtUserIsWindowInDestroy(hWnd);
}

/* EOF */
	 
