/* $Id: class.c,v 1.27 2003/08/08 02:57:54 royce Exp $
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
#include <window.h>
#include <strpool.h>

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
GetClassLongA ( HWND hWnd, int nIndex )
{ 
  PUNICODE_STRING str;

  if ( nIndex != GCL_MENUNAME )
  {
    return NtUserGetClassLong ( hWnd, nIndex, TRUE );
  }

  str = (PUNICODE_STRING)NtUserGetClassLong ( hWnd, nIndex, TRUE );
  if ( IS_INTRESOURCE(str) )
  {
    return (DWORD)str;
  }
  else
  {
    return (DWORD)heap_string_poolA ( str->Buffer, str->Length );
  }
}

/*
 * @implemented
 */
DWORD STDCALL
GetClassLongW ( HWND hWnd, int nIndex )
{
  PUNICODE_STRING str;

  if ( nIndex != GCL_MENUNAME )
  {
    return NtUserGetClassLong ( hWnd, nIndex, FALSE );
  }

  str = (PUNICODE_STRING)NtUserGetClassLong(hWnd, nIndex, TRUE);
  if ( IS_INTRESOURCE(str) )
  {
    return (DWORD)str;
  }
  else
  {
    return (DWORD)heap_string_poolW ( str->Buffer, str->Length );
  }
}


/*
 * @implemented
 */
int STDCALL
GetClassNameA(
  HWND hWnd,
  LPSTR lpClassName,
  int nMaxCount)
{
  int result;
  LPWSTR ClassNameW;
  NTSTATUS Status;

  ClassNameW = HEAP_alloc ( (nMaxCount+1)*sizeof(WCHAR) );

  result = NtUserGetClassName ( hWnd, ClassNameW, nMaxCount );

  Status = HEAP_strcpyWtoA ( lpClassName, ClassNameW, result );

  HEAP_free ( ClassNameW );

  if ( !NT_SUCCESS(Status) )
    return 0;

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
  LPWSTR ClassNameW;

  ClassNameW = HEAP_alloc ( (nMaxCount+1) * sizeof(WCHAR) );

  result = NtUserGetClassName ( hWnd, ClassNameW, nMaxCount );

  RtlCopyMemory ( lpClassName, ClassNameW, result );

  HEAP_free ( ClassNameW );

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

  if ( !lpWndClass )
    return 0;

  RtlCopyMemory ( &Class.style, lpWndClass, sizeof(WNDCLASSA) );

  Class.cbSize = sizeof(WNDCLASSEXA);
  Class.hIconSm = INVALID_HANDLE_VALUE;

  return RegisterClassExA ( &Class );
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassExA(CONST WNDCLASSEXA *lpwcx)
{
  RTL_ATOM Atom;
  WNDCLASSEXW wndclass;
  NTSTATUS Status;
  LPWSTR ClassName = NULL;
  LPWSTR MenuName = NULL;

  if ( !lpwcx || (lpwcx->cbSize != sizeof(WNDCLASSEXA)) )
    return 0;

  if ( !lpwcx->lpszClassName )
    return 0;

  RtlCopyMemory ( &wndclass, lpwcx, sizeof(WNDCLASSEXW) );

  if ( !IS_ATOM(lpwcx->lpszClassName) )
  {
    Status = HEAP_strdupAtoW ( &ClassName, (LPCSTR)lpwcx->lpszClassName, NULL );
    if ( !NT_SUCCESS (Status) )
    {
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }
    wndclass.lpszClassName = ClassName;
  }

  if ( !IS_INTRESOURCE(lpwcx->lpszMenuName) )
  {
    Status = HEAP_strdupAtoW ( &MenuName, (LPCSTR)lpwcx->lpszMenuName, NULL );
    if ( !NT_SUCCESS (Status) )
    {
      if ( ClassName )
	HEAP_free ( ClassName );
      SetLastError (RtlNtStatusToDosError(Status));
      return 0;
    }
    wndclass.lpszMenuName = MenuName;
  }

  Atom = NtUserRegisterClassExWOW ( &wndclass, FALSE, 0, 0, 0 );

  /* free strings if neccessary */
  if ( MenuName  ) HEAP_free ( MenuName );
  if ( ClassName ) HEAP_free ( ClassName );

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

  if ( !lpwcx || (lpwcx->cbSize != sizeof(WNDCLASSEXA)) )
    return 0;

  if ( !lpwcx->lpszClassName )
    return 0;

  hHeap = RtlGetProcessHeap();
  RtlCopyMemory ( &wndclass, lpwcx, sizeof(WNDCLASSEXW) );

  /* copy strings if needed */

  if ( !IS_ATOM(lpwcx->lpszClassName) )
  {
    ClassName = HEAP_strdupW ( lpwcx->lpszClassName, lstrlenW(lpwcx->lpszClassName) );
    if ( !ClassName )
    {
      SetLastError(RtlNtStatusToDosError(STATUS_NO_MEMORY));
      return 0;
    }
    wndclass.lpszClassName = ClassName;
  }

  if ( !IS_INTRESOURCE(lpwcx->lpszMenuName) )
  {
    MenuName = HEAP_strdupW ( lpwcx->lpszMenuName, lstrlenW(lpwcx->lpszMenuName) );
    if ( !MenuName )
    {
      if ( ClassName )
	HEAP_free ( MenuName );
      SetLastError(RtlNtStatusToDosError(STATUS_NO_MEMORY));
      return 0;
    }
    wndclass.lpszMenuName = MenuName;
  }

  Atom = NtUserRegisterClassExWOW ( &wndclass, TRUE, 0, 0, 0 );

  /* free strings if neccessary */
  if ( MenuName  ) HEAP_free ( MenuName  );
  if ( ClassName ) HEAP_free ( ClassName );

  return (ATOM)Atom;
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassW(CONST WNDCLASSW *lpWndClass)
{
  WNDCLASSEXW Class;

  if ( !lpWndClass )
    return 0;

  RtlCopyMemory ( &Class.style, lpWndClass, sizeof(WNDCLASSW) );

  Class.cbSize = sizeof(WNDCLASSEXW);
  Class.hIconSm = INVALID_HANDLE_VALUE;

  return RegisterClassExW ( &Class );
}

/*
 * @implemented
 */
DWORD
STDCALL
SetClassLongA (
  HWND hWnd,
  int nIndex,
  LONG dwNewLong)
{
  PUNICODE_STRING str;
  PUNICODE_STRING str2;

  if ( nIndex != GCL_MENUNAME )
  {
    return NtUserSetClassLong ( hWnd, nIndex, dwNewLong, TRUE );
  }
  if ( IS_INTRESOURCE(dwNewLong) )
  {
    str2 = (PUNICODE_STRING)dwNewLong;
  }
  else
  {
    RtlCreateUnicodeString ( str2, (LPWSTR)dwNewLong );
  }

  str = (PUNICODE_STRING)NtUserSetClassLong(hWnd, nIndex, (DWORD)str2, TRUE);

  if ( !IS_INTRESOURCE(dwNewLong) )
  {
    RtlFreeUnicodeString ( str2 );
  }
  if ( IS_INTRESOURCE(str) )
  {
    return (DWORD)str;
  }
  else
  {
    return (DWORD)heap_string_poolA ( str->Buffer, str->Length );
  }
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
  PUNICODE_STRING str;
  PUNICODE_STRING str2;

  if (nIndex != GCL_MENUNAME )
  {
    return NtUserSetClassLong ( hWnd, nIndex, dwNewLong, FALSE );
  }
  if ( IS_INTRESOURCE(dwNewLong) )
  {
    str2 = (PUNICODE_STRING)dwNewLong;
  }
  else
  {
    RtlCreateUnicodeStringFromAsciiz ( str2,(LPSTR)dwNewLong );
  }

  str = (PUNICODE_STRING)NtUserSetClassLong(hWnd, nIndex, (DWORD)str2, TRUE);

  if ( !IS_INTRESOURCE(dwNewLong) )
  {
    RtlFreeUnicodeString(str2);
  }
  if ( IS_INTRESOURCE(str) )
  {
    return (DWORD)str;
  }
  else
  {
    return (DWORD)heap_string_poolW ( str->Buffer, str->Length );
  }
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
