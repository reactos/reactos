/* $Id: class.c,v 1.22 2003/08/05 15:41:03 weiden Exp $
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
 * @unimplemented
 */
UINT
STDCALL
RealGetWindowClass(
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
 * @unimplemented
 */
UINT
STDCALL
RealGetWindowClassW(
  HWND  hwnd,
  LPWSTR pszType,
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

  Atom = NtUserRegisterClassExWOW(0,(WNDCLASSEXA*)lpwcx,
				  FALSE,
				  0,
				  0,
				  0);

  return (ATOM)Atom;
}


/*
 * @implemented
 */
ATOM STDCALL
RegisterClassExW(CONST WNDCLASSEXW *lpwcx)
{
  RTL_ATOM Atom;

  Atom = NtUserRegisterClassExWOW((WNDCLASSEXW*)lpwcx,
				  0,
				  TRUE,
				  0,
				  0,
				  0);

  return (ATOM)Atom;
}


/*
 * @implemented
 */
ATOM STDCALL
RegisterClassW(CONST WNDCLASSW *lpWndClass)
{
  WNDCLASSEXW Class;

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
