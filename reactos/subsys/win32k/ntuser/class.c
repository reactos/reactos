/*
 *  ReactOS W32 Subsystem
 *  Copyright (C) 1998, 1999, 2000, 2001, 2002, 2003 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/* $Id: class.c,v 1.48 2004/02/24 13:27:03 weiden Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <napi/win32.h>
#include <include/class.h>
#include <include/error.h>
#include <include/winsta.h>
#include <include/object.h>
#include <include/guicheck.h>
#include <include/window.h>
#include <include/color.h>
#include <include/tags.h>

#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS FASTCALL
InitClassImpl(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS FASTCALL
CleanupClassImpl(VOID)
{
  return(STATUS_SUCCESS);
}


NTSTATUS FASTCALL
ClassReferenceClassByAtom(PWNDCLASS_OBJECT* Class,
			  RTL_ATOM Atom)
{
  PWNDCLASS_OBJECT Current;
  PLIST_ENTRY CurrentEntry;
  PW32PROCESS Process = PsGetWin32Process();
  
  IntLockProcessClasses(Process);
  CurrentEntry = Process->ClassListHead.Flink;
  while (CurrentEntry != &Process->ClassListHead)
    {
    Current = CONTAINING_RECORD(CurrentEntry, WNDCLASS_OBJECT, ListEntry);
      
	if (Current->Atom == Atom)
	{
	*Class = Current;
	ObmReferenceObject(Current);
	IntUnLockProcessClasses(Process);
	return(STATUS_SUCCESS);
	}

	CurrentEntry = CurrentEntry->Flink;
    }
  IntUnLockProcessClasses(Process);
  
  return(STATUS_NOT_FOUND);
}

NTSTATUS STDCALL
ClassReferenceClassByName(PWNDCLASS_OBJECT *Class,
			  LPCWSTR ClassName)
{
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  RTL_ATOM ClassAtom;

  if (!ClassName)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
					  KernelMode,
					  0,
					  &WinStaObject);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Validation of window station handle (0x%X) failed\n",
	     PROCESS_WINDOW_STATION());
      return(STATUS_UNSUCCESSFUL);
    }

  Status  = RtlLookupAtomInAtomTable(WinStaObject->AtomTable,
				     (LPWSTR)ClassName,
				     &ClassAtom);

  if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(WinStaObject);  
      return(Status);
    }
  Status = ClassReferenceClassByAtom(Class,
				     ClassAtom);
  
  ObDereferenceObject(WinStaObject);  
  return(Status);
}

NTSTATUS FASTCALL
ClassReferenceClassByNameOrAtom(PWNDCLASS_OBJECT *Class,
				LPCWSTR ClassNameOrAtom)
{
  NTSTATUS Status;

  if (IS_ATOM(ClassNameOrAtom))
    {
      Status = ClassReferenceClassByAtom(Class, 
				  (RTL_ATOM)((ULONG_PTR)ClassNameOrAtom));
  }
  else
    {
      Status = ClassReferenceClassByName(Class, 
					 ClassNameOrAtom);
    }

  if (!NT_SUCCESS(Status))
    {
      SetLastNtError(Status);
    }
  
  return(Status);
}

DWORD STDCALL
NtUserGetClassInfo(HINSTANCE hInstance, LPCWSTR lpClassName,
   LPWNDCLASSEXW lpWndClassEx, BOOL Ansi, DWORD unknown3)
{
   PWNDCLASS_OBJECT Class;
   NTSTATUS Status;
   RTL_ATOM Atom;

   Status = ClassReferenceClassByNameOrAtom(&Class, lpClassName);
   if (!NT_SUCCESS(Status))
   {
      SetLastNtError(Status);
      return 0;
   }
   lpWndClassEx->cbSize = sizeof(LPWNDCLASSEXW);
   lpWndClassEx->style = Class->style;
   if (Ansi)
      lpWndClassEx->lpfnWndProc = Class->lpfnWndProcA;
   else
      lpWndClassEx->lpfnWndProc = Class->lpfnWndProcW;
   lpWndClassEx->cbClsExtra = Class->cbClsExtra;
   lpWndClassEx->cbWndExtra = Class->cbWndExtra;
   lpWndClassEx->hInstance = Class->hInstance;
   lpWndClassEx->hIcon = Class->hIcon;
   lpWndClassEx->hCursor = Class->hCursor;
   lpWndClassEx->hbrBackground = Class->hbrBackground;
   if (Class->lpszMenuName)
   {
      if (!IS_INTRESOURCE((LPCWSTR)Class->lpszMenuName))
         RtlCopyUnicodeString((PUNICODE_STRING)lpWndClassEx->lpszMenuName, Class->lpszMenuName);
      else
         lpWndClassEx->lpszMenuName = (LPCWSTR)Class->lpszMenuName;
   }
   else
      lpWndClassEx->lpszMenuName = (LPCWSTR)NULL;
   lpWndClassEx->lpszClassName = lpClassName;
   lpWndClassEx->hIconSm = Class->hIconSm;
   Atom = Class->Atom;
   
   ObmDereferenceObject(Class);

   return Atom;
}

ULONG FASTCALL
IntGetClassName(struct _WINDOW_OBJECT *WindowObject, LPWSTR lpClassName,
   ULONG nMaxCount)
{
   ULONG Length;
   LPWSTR Name;
   PWINSTATION_OBJECT WinStaObject;
   NTSTATUS Status;

   Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
      KernelMode, 0, &WinStaObject);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("Validation of window station handle (0x%X) failed\n",
         PROCESS_WINDOW_STATION());
      return 0;
   }
   Length = 0;
   Status = RtlQueryAtomInAtomTable(WinStaObject->AtomTable,
      WindowObject->Class->Atom, NULL, NULL, Name, &Length);
   Name = ExAllocatePoolWithTag(NonPagedPool, Length + sizeof(UNICODE_NULL), TAG_STRING);
   Status = RtlQueryAtomInAtomTable(WinStaObject->AtomTable,
      WindowObject->Class->Atom, NULL, NULL, Name, &Length);
   if (!NT_SUCCESS(Status))
   {
      DPRINT("IntGetClassName: RtlQueryAtomInAtomTable failed\n");
      return 0;
   }
   Length /= sizeof(WCHAR);
   if (Length > nMaxCount)
   {
      Length = nMaxCount;
   }
   wcsncpy(lpClassName, Name, Length);
   /* FIXME: Check buffer size before doing this! */
   *(lpClassName + Length) = 0;
   ExFreePool(Name);
   ObDereferenceObject(WinStaObject);

   return Length;
}

DWORD STDCALL
NtUserGetClassName (
  HWND hWnd,
  LPWSTR lpClassName,
  ULONG nMaxCount)
{
   PWINDOW_OBJECT WindowObject;
   LONG Length;

   WindowObject = IntGetWindowObject(hWnd);
   if (WindowObject == NULL)
   {
      SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
      return 0;
   }
   Length = IntGetClassName(WindowObject, lpClassName, nMaxCount);
   IntReleaseWindowObject(WindowObject);
   return Length;
}

DWORD STDCALL
NtUserGetWOWClass(DWORD Unknown0,
		  DWORD Unknown1)
{
  UNIMPLEMENTED;
  return(0);
}

PWNDCLASS_OBJECT FASTCALL
IntCreateClass(CONST WNDCLASSEXW *lpwcx,
                BOOL bUnicodeClass,
                WNDPROC wpExtra,
                RTL_ATOM Atom)
{
	PWNDCLASS_OBJECT ClassObject;
	WORD  objectSize;
	NTSTATUS Status;

	/* Check for double registration of the class. */
	if (PsGetWin32Process() != NULL)
	{
		Status = ClassReferenceClassByAtom(&ClassObject, Atom);
		if (NT_SUCCESS(Status))
		{
			ObmDereferenceObject(ClassObject);
			SetLastWin32Error(ERROR_CLASS_ALREADY_EXISTS);
			return(NULL);
		}	
	}
	
	objectSize = sizeof(WNDCLASS_OBJECT) + lpwcx->cbClsExtra;
	ClassObject = ObmCreateObject(NULL, NULL, otClass, objectSize);
	if (ClassObject == 0)
	{          
		SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
		return(NULL);
	}
	
	ClassObject->cbSize = lpwcx->cbSize;
	ClassObject->style = lpwcx->style;
	ClassObject->cbClsExtra = lpwcx->cbClsExtra;
	ClassObject->cbWndExtra = lpwcx->cbWndExtra;
	ClassObject->hInstance = lpwcx->hInstance;
	ClassObject->hIcon = lpwcx->hIcon;
	ClassObject->hCursor = lpwcx->hCursor;
	ClassObject->hbrBackground = lpwcx->hbrBackground;
	ClassObject->Unicode = bUnicodeClass;
	ClassObject->hIconSm = lpwcx->hIconSm;
	ClassObject->Atom = Atom;
	if (wpExtra == 0) {
	if (bUnicodeClass)
	{
		ClassObject->lpfnWndProcW = lpwcx->lpfnWndProc;
			ClassObject->lpfnWndProcA = (WNDPROC)IntAddWndProcHandle(lpwcx->lpfnWndProc,TRUE);
	}
	else
	{
		ClassObject->lpfnWndProcA = lpwcx->lpfnWndProc;
			ClassObject->lpfnWndProcW = (WNDPROC)IntAddWndProcHandle(lpwcx->lpfnWndProc,FALSE);
		}
	} else {
		if (bUnicodeClass)
		{
			ClassObject->lpfnWndProcW = lpwcx->lpfnWndProc;
			ClassObject->lpfnWndProcA = wpExtra;
		}
		else
		{
			ClassObject->lpfnWndProcA = lpwcx->lpfnWndProc;
			ClassObject->lpfnWndProcW = wpExtra;
		}
	}
	if (IS_INTRESOURCE(lpwcx->lpszMenuName))
	{
		ClassObject->lpszMenuName = (PUNICODE_STRING)lpwcx->lpszMenuName;
	}
	else
	{		
		ClassObject->lpszMenuName = ExAllocatePoolWithTag(NonPagedPool,sizeof(UNICODE_STRING), TAG_STRING);
		RtlCreateUnicodeString(ClassObject->lpszMenuName,(LPWSTR)lpwcx->lpszMenuName);
	}
	/* Extra class data */
	if (ClassObject->cbClsExtra != 0)
	{
		ClassObject->ExtraData = (PCHAR)(ClassObject + 1);
		RtlZeroMemory(ClassObject->ExtraData, ClassObject->cbClsExtra);
	}
	else
	{
		ClassObject->ExtraData = NULL;
	}

	return(ClassObject);
}

RTL_ATOM STDCALL
NtUserRegisterClassExWOW(
	CONST WNDCLASSEXW *lpwcx,
	BOOL bUnicodeClass,
	WNDPROC wpExtra,
	DWORD Unknown4,
	DWORD Unknown5)

/*
 * FUNCTION:
 *   Registers a new class with the window manager
 * ARGUMENTS:
 *   lpwcx          = Win32 extended window class structure
 *   bUnicodeClass = Whether to send ANSI or unicode strings
 *                   to window procedures
 *   wpExtra       = Extra window procedure, if this is not null, its used for the second window procedure for standard controls.
 * RETURNS:
 *   Atom identifying the new class
 */
{
  PWINSTATION_OBJECT WinStaObject;
  PWNDCLASS_OBJECT ClassObject;
  NTSTATUS Status;
  RTL_ATOM Atom;
  DPRINT("About to open window station handle (0x%X)\n", 
    PROCESS_WINDOW_STATION());
  Status = IntValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
    KernelMode,
    0,
    &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    return((RTL_ATOM)0);
  }
  if (!IS_ATOM(lpwcx->lpszClassName))
  {
    Status = RtlAddAtomToAtomTable(WinStaObject->AtomTable,
      (LPWSTR)lpwcx->lpszClassName,
      &Atom);
    if (!NT_SUCCESS(Status))
    {
      ObDereferenceObject(WinStaObject);
      DPRINT("Failed adding class name (%wS) to atom table\n",
	lpwcx->lpszClassName);
      SetLastNtError(Status);      
      return((RTL_ATOM)0);
    }
  }
  else
  {
    Atom = (RTL_ATOM)(ULONG)lpwcx->lpszClassName;
  }
  ClassObject = IntCreateClass(lpwcx, bUnicodeClass, wpExtra, Atom);
  if (ClassObject == NULL)
  {
    if (!IS_ATOM(lpwcx->lpszClassName))
    {
      RtlDeleteAtomFromAtomTable(WinStaObject->AtomTable, Atom);
    }
    ObDereferenceObject(WinStaObject);
    DPRINT("Failed creating window class object\n");
    return((RTL_ATOM)0);
  }
  IntLockProcessClasses(PsGetWin32Process());
  InsertTailList(&PsGetWin32Process()->ClassListHead, &ClassObject->ListEntry);
  IntUnLockProcessClasses(PsGetWin32Process());
  ObDereferenceObject(WinStaObject);
  return(Atom);
}

ULONG FASTCALL
IntGetClassLong(struct _WINDOW_OBJECT *WindowObject, ULONG Offset, BOOL Ansi)
{
  LONG Ret;

  if ((int)Offset >= 0)
    {
      DPRINT("GetClassLong(%x, %d)\n", WindowObject->Self, Offset);
      if (Offset > WindowObject->Class->cbClsExtra - sizeof(LONG))
	{
	  SetLastWin32Error(ERROR_INVALID_PARAMETER);
	  return 0;
	}
      Ret = *((LONG *)(WindowObject->Class->ExtraData + Offset));
      DPRINT("Result: %x\n", Ret);
      return Ret;
    }

  switch (Offset)
    {
    case GCL_CBWNDEXTRA:
      Ret = WindowObject->Class->cbWndExtra;
      break;
    case GCL_CBCLSEXTRA:
      Ret = WindowObject->Class->cbClsExtra;
      break;
    case GCL_HBRBACKGROUND:
      Ret = (ULONG)WindowObject->Class->hbrBackground;
      break;
    case GCL_HCURSOR:
      Ret = (ULONG)WindowObject->Class->hCursor;
      break;
    case GCL_HICON:
      Ret = (ULONG)WindowObject->Class->hIcon;
      break;
    case GCL_HICONSM:
      Ret = (ULONG)WindowObject->Class->hIconSm;
      break;
    case GCL_HMODULE:
      Ret = (ULONG)WindowObject->Class->hInstance;
      break;
    case GCL_MENUNAME:
      Ret = (ULONG)WindowObject->Class->lpszMenuName;
      break;
    case GCL_STYLE:
      Ret = WindowObject->Class->style;
      break;
    case GCL_WNDPROC:
	  if (Ansi)
	  {
		Ret = (ULONG)WindowObject->Class->lpfnWndProcA;
	  }
	  else
	  {
		Ret = (ULONG)WindowObject->Class->lpfnWndProcW;
	  }
      break;
    default:
      Ret = 0;
      break;
    }
  return(Ret);
}

DWORD STDCALL
NtUserGetClassLong(HWND hWnd, DWORD Offset, BOOL Ansi)
{
  PWINDOW_OBJECT WindowObject;
  LONG Ret;

  WindowObject = IntGetWindowObject(hWnd);
  if (WindowObject == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
  }
  Ret = IntGetClassLong(WindowObject, Offset, Ansi);
  IntReleaseWindowObject(WindowObject);
  return(Ret);
}

void FASTCALL
IntSetClassLong(PWINDOW_OBJECT WindowObject, ULONG Offset, LONG dwNewLong, BOOL Ansi)
{
  PUNICODE_STRING str;

  if ((int)Offset >= 0)
    {
      DPRINT("SetClassLong(%x, %d, %x)\n", WindowObject->Self, Offset, dwNewLong);
      if (Offset > WindowObject->Class->cbClsExtra - sizeof(LONG))
	{
	  SetLastWin32Error(ERROR_INVALID_PARAMETER);
	  return;
	}
      *((LONG *)(WindowObject->Class->ExtraData + Offset)) = dwNewLong;
      return;
    }
  
  switch (Offset)
    {
    case GCL_CBWNDEXTRA:
      WindowObject->Class->cbWndExtra = dwNewLong;
      break;
    case GCL_CBCLSEXTRA:
      WindowObject->Class->cbClsExtra = dwNewLong;
      break;
    case GCL_HBRBACKGROUND:
      WindowObject->Class->hbrBackground = (HBRUSH)dwNewLong;
      break;
    case GCL_HCURSOR:
      WindowObject->Class->hCursor = (HCURSOR)dwNewLong;
      break;
    case GCL_HICON:
      WindowObject->Class->hIcon = (HICON)dwNewLong;
      break;
    case GCL_HICONSM:
      WindowObject->Class->hIconSm = (HICON)dwNewLong;
      break;
    case GCL_HMODULE:
      WindowObject->Class->hInstance = (HINSTANCE)dwNewLong;
      break;
    case GCL_MENUNAME:
	  if (!IS_INTRESOURCE(dwNewLong))
	  {
	    str = ExAllocatePoolWithTag(PagedPool,sizeof(UNICODE_STRING)+((PUNICODE_STRING)dwNewLong)->Length, TAG_STRING);
	    memcpy(str,(PUNICODE_STRING)dwNewLong,sizeof(UNICODE_STRING)+((PUNICODE_STRING)dwNewLong)->Length);
        WindowObject->Class->lpszMenuName = str;
	  }
	  else
	  {
		WindowObject->Class->lpszMenuName = (PUNICODE_STRING)dwNewLong;
	  }
      break;
    case GCL_STYLE:
      WindowObject->Class->style = dwNewLong;
      break;
    case GCL_WNDPROC:
	  if (Ansi)
	  {
		WindowObject->Class->lpfnWndProcA = (WNDPROC)dwNewLong;
        WindowObject->Class->lpfnWndProcW = (WNDPROC) IntAddWndProcHandle((WNDPROC)dwNewLong,FALSE);
		WindowObject->Class->Unicode = FALSE;
	  }
	  else
	  {
		WindowObject->Class->lpfnWndProcW = (WNDPROC)dwNewLong;
        WindowObject->Class->lpfnWndProcA = (WNDPROC) IntAddWndProcHandle((WNDPROC)dwNewLong,TRUE);
		WindowObject->Class->Unicode = TRUE;
	  }
      break;
    }
}

DWORD STDCALL
NtUserSetClassLong(HWND hWnd,
		   DWORD Offset,
		   LONG dwNewLong,
		   BOOL Ansi)
{
  PWINDOW_OBJECT WindowObject;
  LONG Ret;

  WindowObject = IntGetWindowObject(hWnd);
  if (WindowObject == NULL)
  {
    SetLastWin32Error(ERROR_INVALID_WINDOW_HANDLE);
    return 0;
  }
  Ret = IntGetClassLong(WindowObject, Offset, Ansi);
  IntSetClassLong(WindowObject, Offset, dwNewLong, Ansi);
  IntReleaseWindowObject(WindowObject);
  return(Ret);
}

DWORD STDCALL
NtUserSetClassWord(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2)
{
  UNIMPLEMENTED;
  return(0);
}

BOOL STDCALL
NtUserUnregisterClass(
   LPCWSTR ClassNameOrAtom,
	 HINSTANCE hInstance,
	 DWORD Unknown)
{
   NTSTATUS Status;
   PWNDCLASS_OBJECT Class;
   PWINSTATION_OBJECT WinStaObject;
  
   if (!ClassNameOrAtom)
   {
      SetLastWin32Error(ERROR_INVALID_PARAMETER);
      return FALSE;
   }
  
   Status = IntValidateWindowStationHandle(
      PROCESS_WINDOW_STATION(),
      KernelMode,
      0,
      &WinStaObject);
   if (!NT_SUCCESS(Status))
   {
      SetLastWin32Error(ERROR_INVALID_HANDLE);
      return FALSE;
   }

   Status = ClassReferenceClassByNameOrAtom(&Class, ClassNameOrAtom);
   if (!NT_SUCCESS(Status))
   {
      SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
      return FALSE;
   }
  
   if (Class->hInstance && Class->hInstance != hInstance)
   {
      ObmDereferenceObject(Class);
      SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
      return FALSE;
   }
  
   if (ObmGetReferenceCount(Class) > 2)
   {
      ObmDereferenceObject(Class);
      SetLastWin32Error(ERROR_CLASS_HAS_WINDOWS);
      return FALSE;
   }
  
   /* Dereference the ClassReferenceClassByNameOrAtom() call */
   ObmDereferenceObject(Class);
  
   RemoveEntryList(&Class->ListEntry);

   RtlDeleteAtomFromAtomTable(WinStaObject->AtomTable, Class->Atom);
   ObDereferenceObject(WinStaObject);
  
   /* Free the object */
   ObmDereferenceObject(Class);
  
   return TRUE;
}

/* EOF */
