/* $Id: class.c,v 1.50 2004/07/03 17:40:23 navaraf Exp $
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
#include <user32/regcontrol.h>
#define NTOS_MODE_USER
#include <ntos.h>



static BOOL GetClassInfoExCommon(
    HINSTANCE hInst,
    LPCWSTR lpszClass,
    LPWNDCLASSEXW lpwcx,
    BOOL unicode)
{
  LPWSTR str;
  UNICODE_STRING str2, str3;
  WNDCLASSEXW w;
  BOOL retval;
  NTSTATUS Status;

  if ( !lpszClass || !lpwcx )
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  if(IS_ATOM(lpszClass))
    str = (LPWSTR)lpszClass;
  else
  {
    extern BOOL ControlsInitialized;

    if (unicode)
    {
        str = HEAP_strdupW ( lpszClass, wcslen(lpszClass) );

        if ( !str )
        {
          SetLastError (RtlNtStatusToDosError(STATUS_NO_MEMORY));
          return FALSE;
        }
    }

    else
    {
        Status = HEAP_strdupAtoW(&str, (LPCSTR)lpszClass, NULL);
        
        if (! NT_SUCCESS(Status))
        {
            SetLastError(RtlNtStatusToDosError(Status));
            return FALSE;
        }
    }

    /* Register built-in controls if not already done */
    if ( !ControlsInitialized )
    {
      ControlsInitialized = ControlsInit(str);
    }
  }

  str2.Length = str3.Length = 0;
  str2.MaximumLength = str3.MaximumLength = 255;
  str2.Buffer = (PWSTR)HEAP_alloc ( str2.MaximumLength * sizeof(WCHAR) );
  if ( !str2.Buffer )
  {
    SetLastError (RtlNtStatusToDosError(STATUS_NO_MEMORY));
    if ( !IS_ATOM(str) )
      HEAP_free ( str );
    return FALSE;
  }

  str3.Buffer = (PWSTR)HEAP_alloc ( str3.MaximumLength * sizeof(WCHAR) );
  if ( !str3.Buffer )
  {
    SetLastError (RtlNtStatusToDosError(STATUS_NO_MEMORY));
    if ( !IS_ATOM(str) )
      HEAP_free ( str );
    return FALSE;
  }

  w.lpszMenuName = (LPCWSTR)&str2;
  w.lpszClassName = (LPCWSTR)&str3;
  retval = (BOOL)NtUserGetClassInfo(hInst, str, &w, TRUE, 0);
  if ( !IS_ATOM(str) )
    HEAP_free(str);

  RtlCopyMemory ( lpwcx, &w, sizeof(WNDCLASSEXW) );

  if ( !IS_INTRESOURCE(w.lpszMenuName) && w.lpszMenuName )
  {
    if (unicode)
        lpwcx->lpszMenuName = heap_string_poolW ( str2.Buffer, str2.Length );
    else
        ((LPWNDCLASSEXA) lpwcx)->lpszMenuName = heap_string_poolA ( str2.Buffer, str2.Length );
  }

  if ( !IS_ATOM(w.lpszClassName) && w.lpszClassName )
  {
    if (unicode)
        lpwcx->lpszClassName = heap_string_poolW ( str3.Buffer, str3.Length );
    else
        ((LPWNDCLASSEXA) lpwcx)->lpszClassName = heap_string_poolA ( str3.Buffer, str3.Length );
  }

  HEAP_free ( str2.Buffer );
  HEAP_free ( str3.Buffer );

  return retval;
}


/*
 * @implemented
 */
BOOL
STDCALL
GetClassInfoExA(
  HINSTANCE hinst,
  LPCSTR lpszClass,
  LPWNDCLASSEXA lpwcx)
{
    return GetClassInfoExCommon(hinst, (LPWSTR)lpszClass, (LPWNDCLASSEXW)lpwcx, FALSE);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetClassInfoExW(
  HINSTANCE hinst,
  LPCWSTR lpszClass,
  LPWNDCLASSEXW lpwcx)
{
    return GetClassInfoExCommon(hinst, lpszClass, lpwcx, TRUE);
}


/*
 * @implemented
 */
BOOL
STDCALL
GetClassInfoA(
  HINSTANCE hInstance,
  LPCSTR lpClassName,
  LPWNDCLASSA lpWndClass)
{
  WNDCLASSEXA w;
  BOOL retval;

  if ( !lpClassName || !lpWndClass )
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  retval = GetClassInfoExA(hInstance,lpClassName,&w);
  RtlCopyMemory ( lpWndClass, &w.style, sizeof(WNDCLASSA) );
  return retval;
}

/*
 * @implemented
 */
BOOL
STDCALL
GetClassInfoW(
  HINSTANCE hInstance,
  LPCWSTR lpClassName,
  LPWNDCLASSW lpWndClass)
{
  WNDCLASSEXW w;
  BOOL retval;

  if(!lpClassName || !lpWndClass)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  retval = GetClassInfoExW(hInstance,lpClassName,&w);
  RtlCopyMemory (lpWndClass,&w.style,sizeof(WNDCLASSW));
  return retval;
}


/*
 * @implemented
 */
DWORD STDCALL
GetClassLongA(HWND hWnd, int nIndex)
{
   switch (nIndex)
   {
      case GCL_HBRBACKGROUND:
         {
            DWORD hBrush = NtUserGetClassLong(hWnd, GCL_HBRBACKGROUND, TRUE);
            if (hBrush != 0 && hBrush < 0x4000)
               hBrush = (DWORD)GetSysColorBrush((ULONG)hBrush - 1);
            return hBrush;
         }

      case GCL_MENUNAME:
         {
            PUNICODE_STRING Name;
            Name = (PUNICODE_STRING)NtUserGetClassLong(hWnd, nIndex, TRUE);
            if (IS_INTRESOURCE(Name))
               return (DWORD)Name;
            else
               return (DWORD)heap_string_poolA(Name->Buffer, Name->Length);
         }

      default:
         return NtUserGetClassLong(hWnd, nIndex, TRUE);
   }
}

/*
 * @implemented
 */
DWORD STDCALL
GetClassLongW ( HWND hWnd, int nIndex )
{
   switch (nIndex)
   {
      case GCL_HBRBACKGROUND:
         {
            DWORD hBrush = NtUserGetClassLong(hWnd, GCL_HBRBACKGROUND, TRUE);
            if (hBrush != 0 && hBrush < 0x4000)
               hBrush = (DWORD)GetSysColorBrush((ULONG)hBrush - 1);
            return hBrush;
         }

      case GCL_MENUNAME:
         {
            PUNICODE_STRING Name;
            Name = (PUNICODE_STRING)NtUserGetClassLong(hWnd, nIndex, FALSE);
            if (IS_INTRESOURCE(Name))
               return (DWORD)Name;
            else
               return (DWORD)heap_string_poolW(Name->Buffer, Name->Length);
         }

      default:
         return NtUserGetClassLong(hWnd, nIndex, FALSE);
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
  
  if(!lpClassName)
    return 0;

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
   return NtUserGetClassName(hWnd, lpClassName, nMaxCount);
}


/*
 * @implemented
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
    if ((nIndex < 0) && (nIndex != GCW_ATOM))
        return 0;

    return (WORD) NtUserGetClassLong ( hWnd, nIndex, TRUE );
}


/*
 * @implemented
 */
LONG
STDCALL
GetWindowLongA ( HWND hWnd, int nIndex )
{
  return NtUserGetWindowLong(hWnd, nIndex, TRUE);
}


/*
 * @implemented
 */
LONG
STDCALL
GetWindowLongW(HWND hWnd, int nIndex)
{
  return NtUserGetWindowLong(hWnd, nIndex, FALSE);
}

/*
 * @implemented
 */
WORD
STDCALL
GetWindowWord(HWND hWnd, int nIndex)
{
  return (WORD)NtUserGetWindowLong(hWnd, nIndex,  TRUE);
}

/*
 * @implemented
 */
WORD
STDCALL
SetWindowWord ( HWND hWnd,int nIndex,WORD wNewWord )
{
  return (WORD)NtUserSetWindowLong ( hWnd, nIndex, (LONG)wNewWord, TRUE );
}

/*
 * @implemented
 */
UINT
STDCALL
RealGetWindowClassW(
  HWND  hwnd,
  LPWSTR pszType,
  UINT  cchType)
{
	/* FIXME: Implement correct functionality of RealGetWindowClass */
	return GetClassNameW(hwnd,pszType,cchType);
}


/*
 * @implemented
 */
UINT
STDCALL
RealGetWindowClassA(
  HWND  hwnd,
  LPSTR pszType,
  UINT  cchType)
{
	/* FIXME: Implement correct functionality of RealGetWindowClass */
	return GetClassNameA(hwnd,pszType,cchType);
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassExA(CONST WNDCLASSEXA *lpwcx)
{
   RTL_ATOM Atom;
   WNDCLASSEXA WndClass;
   UNICODE_STRING ClassName;
   UNICODE_STRING MenuName;

   if (lpwcx == NULL || lpwcx->cbSize != sizeof(WNDCLASSEXW) ||
       lpwcx->cbClsExtra < 0 || lpwcx->cbWndExtra < 0 ||
       lpwcx->lpszClassName == NULL)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }

   /*
    * On real Windows this looks more like:
    *    if (lpwcx->hInstance == User32Instance &&
    *        *(PULONG)((ULONG_PTR)NtCurrentTeb() + 0x6D4) & 0x400)
    * But since I have no idea what the magic field in the
    * TEB structure means, I rather decided to omit that.
    * -- Filip Navara
    */
   if (lpwcx->hInstance == User32Instance)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }

   /* Yes, this is correct. We should modify the passed structure. */
   if (lpwcx->hInstance == NULL)
      ((WNDCLASSEXA*)lpwcx)->hInstance = GetModuleHandleW(NULL);
  
   RtlCopyMemory(&WndClass, lpwcx, sizeof(WNDCLASSEXW));

   if (IS_ATOM(lpwcx->lpszMenuName))
   {
      MenuName.Length =
      MenuName.MaximumLength = 0;
      MenuName.Buffer = (LPWSTR)lpwcx->lpszMenuName;
   } else
   {
      RtlCreateUnicodeStringFromAsciiz(&MenuName, lpwcx->lpszMenuName);
   }

   if (IS_ATOM(lpwcx->lpszClassName))
   {
      ClassName.Length =
      ClassName.MaximumLength = 0;
      ClassName.Buffer = (LPWSTR)lpwcx->lpszClassName;
   } else
   {
      RtlCreateUnicodeStringFromAsciiz(&ClassName, lpwcx->lpszClassName);
   }

   Atom = NtUserRegisterClassExWOW(
      (WNDCLASSEXW*)&WndClass,
      &ClassName,
      &ClassName,
      &MenuName,
      NULL,
      REGISTERCLASS_ANSI,
      0);

   RtlFreeUnicodeString(&MenuName);
   RtlFreeUnicodeString(&ClassName);

   return (ATOM)Atom;
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassExW(CONST WNDCLASSEXW *lpwcx)
{
   WNDCLASSEXW WndClass;
   UNICODE_STRING ClassName;
   UNICODE_STRING MenuName;

   if (lpwcx == NULL || lpwcx->cbSize != sizeof(WNDCLASSEXW) ||
       lpwcx->cbClsExtra < 0 || lpwcx->cbWndExtra < 0 ||
       lpwcx->lpszClassName == NULL)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }

   /*
    * On real Windows this looks more like:
    *    if (lpwcx->hInstance == User32Instance &&
    *        *(PULONG)((ULONG_PTR)NtCurrentTeb() + 0x6D4) & 0x400)
    * But since I have no idea what the magic field in the
    * TEB structure means, I rather decided to omit that.
    * -- Filip Navara
    */
   if (lpwcx->hInstance == User32Instance)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return 0;
   }

   /* Yes, this is correct. We should modify the passed structure. */
   if (lpwcx->hInstance == NULL)
      ((WNDCLASSEXW*)lpwcx)->hInstance = GetModuleHandleW(NULL);

   RtlCopyMemory(&WndClass, lpwcx, sizeof(WNDCLASSEXW));

   if (IS_ATOM(lpwcx->lpszMenuName))
   {
      MenuName.Length =
      MenuName.MaximumLength = 0;
      MenuName.Buffer = (LPWSTR)lpwcx->lpszMenuName;
   } else
   {
      RtlInitUnicodeString(&MenuName, lpwcx->lpszMenuName);
   }

   if (IS_ATOM(lpwcx->lpszClassName))
   {
      ClassName.Length =
      ClassName.MaximumLength = 0;
      ClassName.Buffer = (LPWSTR)lpwcx->lpszClassName;
   } else
   {
      RtlInitUnicodeString(&ClassName, lpwcx->lpszClassName);
   }

   return (ATOM)NtUserRegisterClassExWOW(
      &WndClass,
      &ClassName,
      &ClassName,
      &MenuName,
      NULL,
      0,
      0);
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassA(CONST WNDCLASSA *lpWndClass)
{
   WNDCLASSEXA Class;

   if (lpWndClass == NULL)
      return 0;

   RtlCopyMemory(&Class.style, lpWndClass, sizeof(WNDCLASSA));
   Class.cbSize = sizeof(WNDCLASSEXA);
   Class.hIconSm = NULL;

   return RegisterClassExA(&Class);
}

/*
 * @implemented
 */
ATOM STDCALL
RegisterClassW(CONST WNDCLASSW *lpWndClass)
{
   WNDCLASSEXW Class;

   if (lpWndClass == NULL)
      return 0;

   RtlCopyMemory(&Class.style, lpWndClass, sizeof(WNDCLASSW));
   Class.cbSize = sizeof(WNDCLASSEXW);
   Class.hIconSm = NULL;

   return RegisterClassExW(&Class);
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
  UNICODE_STRING str2buf;
  PUNICODE_STRING str;
  PUNICODE_STRING str2 = &str2buf;

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
    RtlCreateUnicodeStringFromAsciiz ( &str2buf,(LPSTR)dwNewLong );
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
  UNICODE_STRING str2buf;
  PUNICODE_STRING str;
  PUNICODE_STRING str2 = &str2buf;

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
    RtlCreateUnicodeString ( &str2buf, (LPWSTR)dwNewLong );
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
 * @implemented
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
    if ((nIndex < 0) && (nIndex != GCW_ATOM))
        return 0;

    return (WORD) NtUserSetClassLong ( hWnd, nIndex, wNewWord, TRUE );
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
 * @implemented
 */
BOOL
STDCALL
UnregisterClassA(
  LPCSTR lpClassName,
  HINSTANCE hInstance)
{
  LPWSTR ClassName;
  NTSTATUS Status;
  BOOL Result;
  
  if(!IS_ATOM(lpClassName))
  {
    Status = HEAP_strdupAtoW(&ClassName, lpClassName, NULL);
    if(!NT_SUCCESS(Status))
    {
      SetLastError(RtlNtStatusToDosError(Status));
      return FALSE;
    }
  }
  else
    ClassName = (LPWSTR)lpClassName;
  
  Result = (BOOL)NtUserUnregisterClass((LPCWSTR)ClassName, hInstance, 0);
  
  if(ClassName && !IS_ATOM(lpClassName)) 
    HEAP_free(ClassName);
  
  return Result;
}


/*
 * @implemented
 */
BOOL
STDCALL
UnregisterClassW(
  LPCWSTR lpClassName,
  HINSTANCE hInstance)
{
  return (BOOL)NtUserUnregisterClass(lpClassName, hInstance, 0);
}

/* EOF */
