/* $Id: class.c,v 1.25 2003/08/06 16:47:35 weiden Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/class.c
 * PURPOSE:         Window classes
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */
#include <windows.h>
#include <user32.h>
#include <string.h>
#include <stdlib.h>
#include <debug.h>

/* copied from menu.c */
NTSTATUS
STATIC HEAP_strdupA2W ( HANDLE hHeap, LPWSTR* ppszW, LPCSTR lpszA, UINT* NewLen )
{
  ULONG len;
  NTSTATUS Status;
  *ppszW = NULL;
  if ( !lpszA )
    return STATUS_SUCCESS;
  len = lstrlenA(lpszA);
  *ppszW = RtlAllocateHeap ( hHeap, 0, (len+1) * sizeof(WCHAR) );
  if ( !*ppszW )
    return STATUS_NO_MEMORY;
  Status = RtlMultiByteToUnicodeN ( *ppszW, len*sizeof(WCHAR), NULL, (PCHAR)lpszA, len ); 
  (*ppszW)[len] = L'\0';
  if(NewLen) (*NewLen) = (UINT)len;
  return Status;
}

/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetClassInfoA(
  HINSTANCE hInstance,
  LPCSTR lpClassName,
  LPWNDCLASSA lpWndClass)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetClassInfoExA(
  HINSTANCE hinst,
  LPCSTR lpszClass,
  LPWNDCLASSEXA lpwcx)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetClassInfoExW(
  HINSTANCE hinst,
  LPCWSTR lpszClass,
  LPWNDCLASSEXW lpwcx)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
GetClassInfoW(
  HINSTANCE hInstance,
  LPCWSTR lpClassName,
  LPWNDCLASSW lpWndClass)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @implemented
 */
DWORD STDCALL
GetClassLongA(HWND hWnd, int nIndex)
{
  return(NtUserGetClassLong(hWnd, nIndex, TRUE));
}

/*
 * @implemented
 */
DWORD STDCALL
GetClassLongW(HWND hWnd, int nIndex)
{
  return(NtUserGetClassLong(hWnd, nIndex, FALSE));
}


/*
 * @implemented
 */
int
STDCALL
GetClassNameA(
  HWND hWnd,
  LPSTR lpClassName,
  int nMaxCount)
{
	int result;
	LPWSTR ClassName;
	NTSTATUS Status;
	ClassName = RtlAllocateHeap(RtlGetProcessHeap(),HEAP_ZERO_MEMORY,nMaxCount);
    result = NtUserGetClassName(hWnd, ClassName, nMaxCount);
    Status = RtlUnicodeToMultiByteN (lpClassName,
				   result,
				   NULL,
				   ClassName,
				   result);
    if (!NT_SUCCESS(Status))
	{
		return 0;
	}
	RtlFreeHeap(RtlGetProcessHeap(),0,ClassName);
	return result;
}


/*
 * @implemented
 */
int
STDCALL
GetClassNameW(
  HWND hWnd,
  LPWSTR lpClassName,
  int nMaxCount)
{
	int result;
	LPWSTR ClassName;
	ClassName = RtlAllocateHeap(RtlGetProcessHeap(),HEAP_ZERO_MEMORY,nMaxCount);
    result = NtUserGetClassName(hWnd, ClassName, nMaxCount);
	RtlCopyMemory(ClassName,lpClassName,result);
	RtlFreeHeap(RtlGetProcessHeap(),0,ClassName);
	return result;
}


/*
 * @unimplemented
 */
WORD
STDCALL
GetClassWord(
  HWND hWnd,
  int nIndex)
/*
 * NOTE: Obsoleted in 32-bit windows
 */
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @implemented
 */
LONG STDCALL
GetWindowLongA(HWND hWnd, int nIndex)
{
  return NtUserGetWindowLong(hWnd, nIndex, TRUE);
}


/*
 * @implemented
 */
LONG STDCALL
GetWindowLongW(HWND hWnd, int nIndex)
{
  return NtUserGetWindowLong(hWnd, nIndex, FALSE);
}

/*
 * @implemented
 */
WORD STDCALL
GetWindowWord(HWND hWnd, int nIndex)
{
  return (WORD)NtUserGetWindowLong(hWnd, nIndex,  TRUE);
}

/*
 * @unimplemented
 */
UINT
STDCALL
RealGetWindowClassW(
  HWND  hwnd,
  LPSTR pszType,
  UINT  cchType)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
UINT
STDCALL
RealGetWindowClassA(
  HWND  hwnd,
  LPSTR pszType,
  UINT  cchType)
{
  UNIMPLEMENTED;
  return 0;
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassA(CONST WNDCLASSA *lpWndClass)
{
  WNDCLASSEXA Class;
  
  if(!lpWndClass) return (ATOM)0;

  RtlMoveMemory ( &Class.style, lpWndClass, sizeof(WNDCLASSA));
  Class.cbSize = sizeof(WNDCLASSEXA);
  Class.hIconSm = INVALID_HANDLE_VALUE;
  return RegisterClassExA(&Class);
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassExA(CONST WNDCLASSEXA *lpwcx)
{
  RTL_ATOM Atom;
  WNDCLASSEXW wndclass;
  HANDLE hHeap;
  NTSTATUS Status;
  LPWSTR ClassName = NULL;
  LPWSTR MenuName = NULL;
  
  if(!lpwcx || (lpwcx->cbSize != sizeof(WNDCLASSEXA)))
    return (ATOM)0;
    
  if(!lpwcx->lpszClassName) return (ATOM)0;
  
  hHeap = RtlGetProcessHeap();
  RtlMoveMemory(&wndclass, lpwcx, sizeof(WNDCLASSEXW));
  
  if(HIWORD(lpwcx->lpszClassName))
  {
    Status = HEAP_strdupA2W (hHeap, &ClassName, (LPCSTR)lpwcx->lpszClassName, NULL);
    if (!NT_SUCCESS (Status))
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }
    wndclass.lpszClassName = ClassName;
  }
  
  if(HIWORD(lpwcx->lpszMenuName))
  {
    Status = HEAP_strdupA2W (hHeap, &MenuName, (LPCSTR)lpwcx->lpszMenuName, NULL);
    if (!NT_SUCCESS (Status))
    {
      if(HIWORD(lpwcx->lpszClassName))
      {
        RtlFreeHeap (hHeap, 0, ClassName);
      }
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }
    wndclass.lpszMenuName = MenuName;
  }

  Atom = NtUserRegisterClassExWOW(&wndclass,
				  FALSE,
				  0,
				  0,
				  0);

  /* free strings if neccessary */
  if(MenuName) RtlFreeHeap (hHeap, 0, MenuName);
  if(ClassName) RtlFreeHeap (hHeap, 0, ClassName);  

  return (ATOM)Atom;
}



/*
 * @implemented
 */
ATOM STDCALL
RegisterClassExW(CONST WNDCLASSEXW *lpwcx)
{
  RTL_ATOM Atom;
  HANDLE hHeap;
  WNDCLASSEXW wndclass;
  LPWSTR ClassName = NULL;
  LPWSTR MenuName = NULL;
  ULONG len;
  
  if(!lpwcx || (lpwcx->cbSize != sizeof(WNDCLASSEXA)))
    return (ATOM)0;
    
  if(!lpwcx->lpszClassName) return (ATOM)0;
  
  hHeap = RtlGetProcessHeap();
  RtlMoveMemory(&wndclass, lpwcx, sizeof(WNDCLASSEXW));
  
  /* copy strings if needed */
  
  if(HIWORD(lpwcx->lpszClassName))
  {
    len = lstrlenW(lpwcx->lpszClassName);
    ClassName = RtlAllocateHeap (hHeap, 0, (len + 1) * sizeof(WCHAR));
    if(!ClassName)
    {
      SetLastError(RtlNtStatusToDosError(STATUS_NO_MEMORY));
      return 0;
    }
    memcpy(&ClassName, &lpwcx->lpszClassName, len);
    
    wndclass.lpszClassName = ClassName;
  }
  
  if(HIWORD(lpwcx->lpszMenuName))
  {
    len = lstrlenW(lpwcx->lpszMenuName);
    MenuName = RtlAllocateHeap (hHeap, 0, (len + 1) * sizeof(WCHAR));
    if(!MenuName)
    {
      if(HIWORD(lpwcx->lpszClassName))
      {
        RtlFreeHeap (hHeap, 0, MenuName);
      }
      SetLastError(RtlNtStatusToDosError(STATUS_NO_MEMORY));
      return 0;
    }
    memcpy(&MenuName, &lpwcx->lpszMenuName, len);
    
    wndclass.lpszMenuName = MenuName;
  }

  Atom = NtUserRegisterClassExWOW(&wndclass,
				  TRUE,
				  0,
				  0,
				  0);

  /* free strings if neccessary */
  if(MenuName) RtlFreeHeap (hHeap, 0, MenuName);
  if(ClassName) RtlFreeHeap (hHeap, 0, ClassName);

  return (ATOM)Atom;
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassW(CONST WNDCLASSW *lpWndClass)
{
  WNDCLASSEXW Class;
  
  if(!lpWndClass) return (ATOM)0;

  RtlMoveMemory(&Class.style, lpWndClass, sizeof(WNDCLASSW));
  Class.cbSize = sizeof(WNDCLASSEXW);
  Class.hIconSm = INVALID_HANDLE_VALUE;
  return RegisterClassExW(&Class);
}

/*
 * @mplemented
 */
DWORD
STDCALL
SetClassLongA(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  return(NtUserSetClassLong(hWnd, nIndex, dwNewLong, TRUE));
}


/*
 * @implemented
 */
DWORD
STDCALL
SetClassLongW(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  return(NtUserSetClassLong(hWnd, nIndex, dwNewLong, FALSE));
}


/*
 * @unimplemented
 */
WORD
STDCALL
SetClassWord(
  HWND hWnd,
  int nIndex,
  WORD wNewWord)
/*
 * NOTE: Obsoleted in 32-bit windows
 */
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @implemented
 */
LONG
STDCALL
SetWindowLongA(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  return NtUserSetWindowLong(hWnd, nIndex, dwNewLong, TRUE);
}


/*
 * @implemented
 */
LONG
STDCALL
SetWindowLongW(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  return NtUserSetWindowLong(hWnd, nIndex, dwNewLong, FALSE);
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
UnregisterClassA(
  LPCSTR lpClassName,
  HINSTANCE hInstance)
{
  UNIMPLEMENTED;
  return FALSE;
}


/*
 * @unimplemented
 */
WINBOOL
STDCALL
UnregisterClassW(
  LPCWSTR lpClassName,
  HINSTANCE hInstance)
{
  UNIMPLEMENTED;
  return FALSE;
}

/* EOF */
