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
/* $Id: class.c,v 1.22 2003/08/05 15:41:03 weiden Exp $
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
  
  ExAcquireFastMutexUnsafe (&Process->ClassListLock);
  CurrentEntry = Process->ClassListHead.Flink;
  while (CurrentEntry != &Process->ClassListHead)
    {
    Current = CONTAINING_RECORD(CurrentEntry, WNDCLASS_OBJECT, ListEntry);
      
	if (Current->ClassW.lpszClassName == (LPWSTR)(ULONG)Atom)
	{
	*Class = Current;
	ObmReferenceObject(Current);
	ExReleaseFastMutexUnsafe (&Process->ClassListLock);
	return(STATUS_SUCCESS);
	}

	CurrentEntry = CurrentEntry->Flink;
    }
  ExReleaseFastMutexUnsafe (&Process->ClassListLock);
  
  return(STATUS_NOT_FOUND);
}

NTSTATUS STDCALL
ClassReferenceClassByName(PWNDCLASS_OBJECT *Class,
			  PWSTR ClassName)
{
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;
  RTL_ATOM ClassAtom;

  if (!ClassName)
    {
      return(STATUS_INVALID_PARAMETER);
    }

  Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
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
				     ClassName,
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
				LPWSTR ClassNameOrAtom)
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
NtUserGetClassInfo(IN LPWSTR ClassName,
		   IN ULONG InfoClass,
		   OUT PVOID Info,
		   IN ULONG InfoLength,
		   OUT PULONG ReturnedLength)
{
  UNIMPLEMENTED;
    
  return(0);
}

ULONG FASTCALL
W32kGetClassName(struct _WINDOW_OBJECT *WindowObject,
		   LPWSTR lpClassName,
		   int nMaxCount)
{
	int length;
	length = wcslen(WindowObject->Class->ClassW.lpszClassName);
	if (length > nMaxCount)
	{
		length = nMaxCount;
	}
	wcsncpy(lpClassName,WindowObject->Class->ClassW.lpszClassName,length+1);
	return length;
}

DWORD STDCALL
NtUserGetClassName(HWND hWnd,
		   LPWSTR lpClassName,
		   int nMaxCount)
{
  PWINDOW_OBJECT WindowObject;
  LONG Ret;

  WindowObject = W32kGetWindowObject(hWnd);
  if (WindowObject == NULL)
    {
      return(0);
    }
  Ret = W32kGetClassName(WindowObject, lpClassName, nMaxCount);
  W32kReleaseWindowObject(WindowObject);
  return(Ret);
}

DWORD STDCALL
NtUserGetWOWClass(DWORD Unknown0,
		  DWORD Unknown1)
{
  UNIMPLEMENTED;
  
  return(0);
}

PWNDCLASS_OBJECT FASTCALL
W32kCreateClass(CONST WNDCLASSEXW *lpwcxw,
                CONST WNDCLASSEXA *lpwcxa,
                BOOL bUnicodeClass,
                RTL_ATOM Atom)
{
  PWNDCLASS_OBJECT ClassObject;
  WORD  objectSize;
  LPWSTR  namePtrW;
  LPSTR   namePtrA;
  ULONG menulenA=1, menulenW=sizeof(WCHAR);

  /* FIXME - how to handle INT resources? */
  /* FIXME - lpszClassName = Atom? is that right */
  if ( bUnicodeClass )
    {
      if ( lpwcxw->lpszMenuName )
        {
          menulenW = (wcslen(lpwcxw->lpszMenuName) + 1) * sizeof(WCHAR);
          RtlUnicodeToMultiByteSize ( &menulenA, lpwcxw->lpszMenuName, menulenW );
        }
    }
  else
    {
      if ( lpwcxa->lpszMenuName )
        {
          menulenA = strlen(lpwcxa->lpszMenuName) + 1;
          RtlMultiByteToUnicodeSize ( &menulenW, lpwcxa->lpszMenuName, menulenA );
        }
    }
  objectSize = sizeof(WNDCLASS_OBJECT);
  ClassObject = ObmCreateObject(NULL, NULL, otClass, objectSize);
  if (ClassObject == 0)
    {          
      return(NULL);
    }

  if ( bUnicodeClass )
    {
      memmove ( &ClassObject->ClassW, lpwcxw, sizeof(ClassObject->ClassW) );
      memmove ( &ClassObject->ClassA, &ClassObject->ClassW, sizeof(ClassObject->ClassA) );
	  ClassObject->ClassA.lpfnWndProc = 0xCCCCCCCC; /*FIXME: figure out what the correct strange value is and what to do with it */
      namePtrW = ExAllocatePool(PagedPool, menulenW);
	  if ( lpwcxw->lpszMenuName )
        memmove ( namePtrW, lpwcxw->lpszMenuName, menulenW );
      else
        *namePtrW = L'\0';
      ClassObject->ClassW.lpszMenuName = namePtrW;
      namePtrA = ExAllocatePool(PagedPool, menulenA);
      if ( *namePtrW )
        RtlUnicodeToMultiByteN ( namePtrA, menulenA, NULL, namePtrW, menulenW );
      else
        *namePtrA = '\0';
      ClassObject->ClassA.lpszMenuName = namePtrA;
    }
  else
    {
      memmove ( &ClassObject->ClassA, lpwcxa, sizeof(ClassObject->ClassA) );
      memmove ( &ClassObject->ClassW, &ClassObject->ClassA, sizeof(ClassObject->ClassW) );
	  ClassObject->ClassW.lpfnWndProc = 0xCCCCCCCC; /* FIXME: figure out what the correct strange value is and what to do with it */
      namePtrA = ExAllocatePool(PagedPool, menulenA);
      if ( lpwcxa->lpszMenuName )
        memmove ( namePtrA, lpwcxa->lpszMenuName, menulenA );
      else
        *namePtrA = '\0';
      ClassObject->ClassA.lpszMenuName = namePtrA;
      namePtrW = ExAllocatePool(PagedPool, menulenW);
      if ( *namePtrA )
        RtlMultiByteToUnicodeN ( namePtrW, menulenW, NULL, namePtrA, menulenA );
      else
        *namePtrW = L'\0';
      ClassObject->ClassW.lpszMenuName = namePtrW;
    }
  ClassObject->Unicode = bUnicodeClass;
  ClassObject->ClassW.lpszClassName = (LPWSTR)(ULONG)Atom;
  ClassObject->ClassA.lpszClassName = (LPSTR)(ULONG)Atom;
  return(ClassObject);
}

RTL_ATOM STDCALL
NtUserRegisterClassExWOW(CONST WNDCLASSEXW *lpwcxw,
			 CONST WNDCLASSEXA *lpwcxa,
			 BOOL bUnicodeClass,
			 DWORD Unknown3,
			 DWORD Unknown4,
			 DWORD Unknown5)
/*
 * FUNCTION:
 *   Registers a new class with the window manager
 * ARGUMENTS:
 *   lpcxw          = Win32 extended window class structure (unicode)
 *   lpcxa          = Win32 extended window class structure (ascii)
 *   bUnicodeClass = Wether to send ANSI or unicode strings
 *                   to window procedures
 * RETURNS:
 *   Atom identifying the new class
 */
{
  PWINSTATION_OBJECT WinStaObject;
  PWNDCLASS_OBJECT ClassObject;
  NTSTATUS Status;
  RTL_ATOM Atom;
  LPWSTR classname;
  int len;

  DPRINT("About to open window station handle (0x%X)\n", 
	 PROCESS_WINDOW_STATION());

  Status = ValidateWindowStationHandle(PROCESS_WINDOW_STATION(),
				       KernelMode,
				       0,
				       &WinStaObject);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("Validation of window station handle (0x%X) failed\n",
	     PROCESS_WINDOW_STATION());
      return((RTL_ATOM)0);
    }
  if (bUnicodeClass)
  {
	if (!IS_ATOM(lpwcxw->lpszClassName))
	    {
		Status = RtlAddAtomToAtomTable(WinStaObject->AtomTable,
						(LPWSTR)lpwcxw->lpszClassName,
						&Atom);
		if (!NT_SUCCESS(Status))
		{
		ObDereferenceObject(WinStaObject);
		DPRINT("Failed adding class name (%wS) to atom table\n",
			lpwcxw->lpszClassName);
		SetLastNtError(Status);      
		return((RTL_ATOM)0);
		}
		}
	else
	    {
		Atom = (RTL_ATOM)(ULONG)lpwcxw->lpszClassName;
		}
	ClassObject = W32kCreateClass(lpwcxw, NULL, bUnicodeClass, Atom);
	if (ClassObject == NULL)
    {
		if (!IS_ATOM(lpwcxw->lpszClassName))
		{
		RtlDeleteAtomFromAtomTable(WinStaObject->AtomTable, Atom);
		}
		ObDereferenceObject(WinStaObject);
		DPRINT("Failed creating window class object\n");
		SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
		return((RTL_ATOM)0);
	}
  }
  else
  {
	if (!IS_ATOM(lpwcxa->lpszClassName))
	    {
		len = strlen(lpwcxa->lpszClassName);
        classname = ExAllocatePool(PagedPool, ((len + 1) * sizeof(WCHAR)));
		Status = RtlMultiByteToUnicodeN (classname,
				   ((len + 1) * sizeof(WCHAR)),
				   NULL,
				   lpwcxa->lpszClassName,
				   len);
		if (!NT_SUCCESS(Status))
		{
			return 0;
		}
		Status = RtlAddAtomToAtomTable(WinStaObject->AtomTable,
						(LPWSTR)classname,
						&Atom);
		if (!NT_SUCCESS(Status))
		{
		ObDereferenceObject(WinStaObject);
		DPRINT("Failed adding class name (%wS) to atom table\n",
			lpwcxa->lpszClassName);
		SetLastNtError(Status);      
		return((RTL_ATOM)0);
		}
		}
	else
	    {
		Atom = (RTL_ATOM)(ULONG)lpwcxa->lpszClassName;
		}
	ClassObject = W32kCreateClass(NULL, lpwcxa, bUnicodeClass, Atom);
	if (ClassObject == NULL)
    {
		if (!IS_ATOM(lpwcxa->lpszClassName))
		{
		RtlDeleteAtomFromAtomTable(WinStaObject->AtomTable, Atom);
		}
		ObDereferenceObject(WinStaObject);
		DPRINT("Failed creating window class object\n");
		SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
		return((RTL_ATOM)0);
	}
  }
  ExAcquireFastMutex(&PsGetWin32Process()->ClassListLock);
  InsertTailList(&PsGetWin32Process()->ClassListHead, &ClassObject->ListEntry);
  ExReleaseFastMutex(&PsGetWin32Process()->ClassListLock);
  
  ObDereferenceObject(WinStaObject);
  
  return(Atom);
}

ULONG FASTCALL
W32kGetClassLong(struct _WINDOW_OBJECT *WindowObject, ULONG Offset, BOOL Ansi)
{
  LONG Ret;
  switch (Offset)
    {
    case GCL_CBWNDEXTRA:
      Ret = WindowObject->Class->ClassW.cbWndExtra;
      break;
    case GCL_CBCLSEXTRA:
      Ret = WindowObject->Class->ClassW.cbClsExtra;
      break;
    case GCL_HBRBACKGROUND:
      Ret = (ULONG)WindowObject->Class->ClassW.hbrBackground;
      break;
    case GCL_HCURSOR:
      Ret = (ULONG)WindowObject->Class->ClassW.hCursor;
      break;
    case GCL_HICON:
      Ret = (ULONG)WindowObject->Class->ClassW.hIcon;
      break;
    case GCL_HICONSM:
      Ret = (ULONG)WindowObject->Class->ClassW.hIconSm;
      break;
    case GCL_HMODULE:
      Ret = (ULONG)WindowObject->Class->ClassW.hInstance;
      break;
    case GCL_MENUNAME:
	  if (Ansi)
	  {
		Ret = (ULONG)WindowObject->Class->ClassA.lpszMenuName;
	  }
	  else
	  {
		Ret = (ULONG)WindowObject->Class->ClassW.lpszMenuName;
	  }
      break;
    case GCL_STYLE:
      Ret = WindowObject->Class->ClassW.style;
      break;
    case GCL_WNDPROC:
	  if (WindowObject->Unicode)
	  {
		Ret = (ULONG)WindowObject->Class->ClassW.lpfnWndProc;
	  }
	  else
	  {
		Ret = (ULONG)WindowObject->Class->ClassA.lpfnWndProc;
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

  WindowObject = W32kGetWindowObject(hWnd);
  if (WindowObject == NULL)
    {
      return(0);
    }
  Ret = W32kGetClassLong(WindowObject, Offset, Ansi);
  W32kReleaseWindowObject(WindowObject);
  return(Ret);
}

void FASTCALL
W32kSetClassLong(PWINDOW_OBJECT WindowObject, ULONG Offset, LONG dwNewLong, BOOL Ansi)
{
  switch (Offset)
    {
    case GCL_CBWNDEXTRA:
      WindowObject->Class->ClassW.cbWndExtra = dwNewLong;
      WindowObject->Class->ClassA.cbWndExtra = dwNewLong;
      break;
    case GCL_CBCLSEXTRA:
      WindowObject->Class->ClassW.cbClsExtra = dwNewLong;
      WindowObject->Class->ClassA.cbClsExtra = dwNewLong;
      break;
    case GCL_HBRBACKGROUND:
      WindowObject->Class->ClassW.hbrBackground = (HBRUSH)dwNewLong;
      WindowObject->Class->ClassA.hbrBackground = (HBRUSH)dwNewLong;
      break;
    case GCL_HCURSOR:
      WindowObject->Class->ClassW.hCursor = (HCURSOR)dwNewLong;
      WindowObject->Class->ClassA.hCursor = (HCURSOR)dwNewLong;
      break;
    case GCL_HICON:
      WindowObject->Class->ClassW.hIcon = (HICON)dwNewLong;
      WindowObject->Class->ClassA.hIcon = (HICON)dwNewLong;
      break;
    case GCL_HICONSM:
      WindowObject->Class->ClassW.hIconSm = (HICON)dwNewLong;
      WindowObject->Class->ClassA.hIconSm = (HICON)dwNewLong;
      break;
    case GCL_HMODULE:
      WindowObject->Class->ClassW.hInstance = (HINSTANCE)dwNewLong;
      WindowObject->Class->ClassA.hInstance = (HINSTANCE)dwNewLong;
      break;
    /*case GCL_MENUNAME:
      WindowObject->Class->Class.lpszMenuName = (LPCWSTR)dwNewLong;
      break;*/
    case GCL_STYLE:
      WindowObject->Class->ClassW.style = dwNewLong;
      WindowObject->Class->ClassA.style = dwNewLong;
      break;
    /*case GCL_WNDPROC:
      WindowObject->Class->Class.lpfnWndProc = (WNDPROC)dwNewLong;
      break;*/
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

  WindowObject = W32kGetWindowObject(hWnd);
  if (WindowObject == NULL)
    {
      return(0);
    }
  Ret = W32kGetClassLong(WindowObject, Offset, Ansi);
  W32kSetClassLong(WindowObject, Offset, dwNewLong, Ansi);
  W32kReleaseWindowObject(WindowObject);
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

DWORD STDCALL
NtUserUnregisterClass(DWORD Unknown0,
		      DWORD Unknown1,
		      DWORD Unknown2)
{
  UNIMPLEMENTED;

  return(0);
}

/* EOF */
