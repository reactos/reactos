/* $Id: class.c,v 1.20 2003/07/10 21:04:31 chorns Exp $
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
  switch (nIndex)
    {
    case GCL_WNDPROC:
      UNIMPLEMENTED;
      return(0);
    case GCL_MENUNAME:
      UNIMPLEMENTED;
      return(0);
    default:
      return(GetClassLongW(hWnd, nIndex));
    }
}


/*
 * @implemented
 */
DWORD STDCALL
GetClassLongW(HWND hWnd, int nIndex)
{
  return(NtUserGetClassLong(hWnd, nIndex));
}


/*
 * @unimplemented
 */
int
STDCALL
GetClassNameA(
  HWND hWnd,
  LPSTR lpClassName,
  int nMaxCount)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
int
STDCALL
GetClassNameW(
  HWND hWnd,
  LPWSTR lpClassName,
  int nMaxCount)
{
  UNIMPLEMENTED;
  return 0;
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
  return NtUserGetWindowLong(hWnd, nIndex);
}


/*
 * @implemented
 */
LONG STDCALL
GetWindowLongW(HWND hWnd, int nIndex)
{
  return NtUserGetWindowLong(hWnd, nIndex);
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

  RtlMoveMemory(&Class.style, lpWndClass, sizeof(WNDCLASS));
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
  UNICODE_STRING MenuName;
  UNICODE_STRING ClassName;
  WNDCLASSEXW Class;
  RTL_ATOM Atom;

  if (!RtlCreateUnicodeStringFromAsciiz(&MenuName, (PCSZ)lpwcx->lpszMenuName))
  {
    RtlFreeUnicodeString(&MenuName);
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return (ATOM)0;
  }

  if (!RtlCreateUnicodeStringFromAsciiz(&ClassName, (PCSZ)lpwcx->lpszClassName))
    {
      RtlFreeUnicodeString(&ClassName);
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return (ATOM)0;
    }

  RtlMoveMemory(&Class, lpwcx, sizeof(WNDCLASSEXA));
  Class.lpszMenuName = MenuName.Buffer;
  Class.lpszClassName = ClassName.Buffer;

  Atom = NtUserRegisterClassExWOW(&Class,
				  FALSE,
				  0,
				  0,
				  0,
				  0);

  RtlFreeUnicodeString(&ClassName);
  RtlFreeUnicodeString(&MenuName);

  return (ATOM)Atom;
}


/*
 * @implemented
 */
ATOM STDCALL
RegisterClassExW(CONST WNDCLASSEXW *lpwcx)
{
  RTL_ATOM Atom;

  Atom = NtUserRegisterClassExWOW((WNDCLASSEX*)lpwcx,
				  TRUE,
				  0,
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
 * @unimplemented
 */
DWORD
STDCALL
SetClassLongA(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  UNIMPLEMENTED;
  return 0;
}


/*
 * @unimplemented
 */
DWORD
STDCALL
SetClassLongW(
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  UNIMPLEMENTED;
  return 0;
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
