/* $Id: class.c,v 1.15 2002/09/07 15:12:45 chorns Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS user32.dll
 * FILE:            lib/user32/windows/class.c
 * PURPOSE:         Window classes
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      09-05-2001  CSH  Created
 */

#include <user32.h>


BOOL
STDCALL
GetClassInfoA(
  HINSTANCE hInstance,
  LPCSTR lpClassName,
  LPWNDCLASSA lpWndClass)
{
  return FALSE;
}

BOOL
STDCALL
GetClassInfoExA(
  HINSTANCE hinst,
  LPCSTR lpszClass,
  LPWNDCLASSEXA lpwcx)
{
  return FALSE;
}

BOOL
STDCALL
GetClassInfoExW(
  HINSTANCE hinst,
  LPCWSTR lpszClass,
  LPWNDCLASSEXW lpwcx)
{
  return FALSE;
}

BOOL
STDCALL
GetClassInfoW(
  HINSTANCE hInstance,
  LPCWSTR lpClassName,
  LPWNDCLASSW lpWndClass)
{
  return FALSE;
}

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

DWORD STDCALL
GetClassLongW(HWND hWnd, int nIndex)
{
  return(NtUserGetClassLong(hWnd, nIndex));
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
/*
 * NOTE: Obsoleted in 32-bit windows
 */
{
  return 0;
}

LONG STDCALL
GetWindowLongA(HWND hWnd, int nIndex)
{
  return 0;
}

LONG STDCALL
GetWindowLongW(HWND hWnd, int nIndex)
{
  return(NtUserGetWindowLong(hWnd, nIndex));
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

ATOM STDCALL
RegisterClassA(CONST WNDCLASSA *lpWndClass)
{
  WNDCLASSEXA Class;

  RtlMoveMemory(&Class.style, lpWndClass, sizeof(WNDCLASS));
  Class.cbSize = sizeof(WNDCLASSEX);
  Class.hIconSm = INVALID_HANDLE_VALUE;
  return RegisterClassExA(&Class);
}

ATOM STDCALL
RegisterClassExA(CONST WNDCLASSEXA *lpwcx)
{
  UNICODE_STRING MenuName;
  UNICODE_STRING ClassName;
  WNDCLASSEX Class;
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
  
  RtlMoveMemory(&Class, lpwcx, sizeof(WNDCLASSEX));
  Class.lpszMenuName = (LPCTSTR)MenuName.Buffer;
  Class.lpszClassName = (LPCTSTR)ClassName.Buffer;

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

ATOM STDCALL
RegisterClassW(CONST WNDCLASSW *lpWndClass)
{
  WNDCLASSEX Class;

  RtlMoveMemory(&Class.style, lpWndClass, sizeof(WNDCLASS));
  Class.cbSize = sizeof(WNDCLASSEX);
  Class.hIconSm = INVALID_HANDLE_VALUE;
  return RegisterClassExW(&Class);
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
/*
 * NOTE: Obsoleted in 32-bit windows
 */
{
  return 0;
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

/* EOF */
