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
/* $Id: class.c,v 1.59.4.1 2004/07/07 18:03:01 weiden Exp $
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

#include <w32k.h>

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

BOOL FASTCALL
ClassReferenceClassByAtom(
   PCLASS_OBJECT* Class,
   RTL_ATOM Atom,
   HINSTANCE hInstance)
{
   PCLASS_OBJECT Current, BestMatch = NULL;
   PLIST_ENTRY CurrentEntry;
   PW32PROCESS Process = PsGetWin32Process();
  
   CurrentEntry = Process->ClassListHead.Flink;
   while (CurrentEntry != &Process->ClassListHead)
   {
      Current = CONTAINING_RECORD(CurrentEntry, CLASS_OBJECT, ListEntry);
      
      if (Current->Atom == Atom && (hInstance == NULL || Current->hInstance == hInstance))
      {
         *Class = Current;
         ObmReferenceObject(Current);
         return TRUE;
      }

      if (Current->Atom == Atom && Current->Global)
         BestMatch = Current;

      CurrentEntry = CurrentEntry->Flink;
   }

   if (BestMatch != NULL)
   {
      *Class = BestMatch;
      ObmReferenceObject(BestMatch);
      return TRUE;
   }
  
   return FALSE;
}

BOOL FASTCALL
ClassReferenceClassByName(
   PCLASS_OBJECT *Class,
   LPCWSTR ClassName,
   HINSTANCE hInstance)
{
   PWINSTATION_OBJECT WinStaObject;
   NTSTATUS Status;
   BOOL Found;
   RTL_ATOM ClassAtom;

   if (!ClassName)
      return FALSE;

   Status = IntValidateWindowStationHandle(
      PROCESS_WINDOW_STATION(),
      KernelMode,
      0,
      &WinStaObject);

   if (!NT_SUCCESS(Status))
   {
      DPRINT("Validation of window station handle (0x%X) failed\n",
	     PROCESS_WINDOW_STATION());
      return FALSE;
   }

   Status = RtlLookupAtomInAtomTable(
      WinStaObject->AtomTable,
      (LPWSTR)ClassName,
      &ClassAtom);

   if (!NT_SUCCESS(Status))
   {
      ObDereferenceObject(WinStaObject);  
      return FALSE;
   }

   Found = ClassReferenceClassByAtom(Class, ClassAtom, hInstance);
   ObDereferenceObject(WinStaObject);  

   return Found;
}

BOOL FASTCALL
ClassReferenceClassByNameOrAtom(PCLASS_OBJECT *Class, LPCWSTR ClassNameOrAtom, HINSTANCE hInstance)
{
   BOOL Found;

   if (IS_ATOM(ClassNameOrAtom))
      Found = ClassReferenceClassByAtom(Class, (RTL_ATOM)((ULONG_PTR)ClassNameOrAtom), hInstance);
   else
      Found = ClassReferenceClassByName(Class, ClassNameOrAtom, hInstance);

   return Found;
}

ULONG FASTCALL
IntGetClassName(PWINDOW_OBJECT WindowObject, LPWSTR lpClassName, ULONG nMaxCount)
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
      WindowObject->Class->Atom, NULL, NULL, NULL, &Length);
   Name = ExAllocatePoolWithTag(PagedPool, Length + sizeof(UNICODE_NULL), TAG_STRING);
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

PCLASS_OBJECT FASTCALL
IntCreateClass(
   CONST WNDCLASSEXW *lpwcx,
   DWORD Flags,
   WNDPROC wpExtra,
   PUNICODE_STRING MenuName,
   RTL_ATOM Atom)
{
	PCLASS_OBJECT ClassObject;
	ULONG  objectSize;
	BOOL Global;

	Global = (Flags & REGISTERCLASS_SYSTEM) || (lpwcx->style & CS_GLOBALCLASS) ? TRUE : FALSE;

	/* Check for double registration of the class. */
	if (PsGetWin32Process() != NULL)
	{
		if (ClassReferenceClassByAtom(&ClassObject, Atom, lpwcx->hInstance))
		{
			/*
			 * NOTE: We may also get a global class from
                         * ClassReferenceClassByAtom. This simple check
                         * prevents that we fail valid request.
                         */
			if (ClassObject->hInstance == lpwcx->hInstance)
			{
			        SetLastWin32Error(ERROR_CLASS_ALREADY_EXISTS);
				ObmDereferenceObject(ClassObject);
				return(NULL);
			}
		}	
	}
	
	objectSize = sizeof(CLASS_OBJECT) + lpwcx->cbClsExtra;
	if (!(ClassObject = ExAllocatePoolWithTag(PagedPool, objectSize, TAG_CLASS)))
	{          
		SetLastWin32Error(ERROR_NOT_ENOUGH_MEMORY);
		return(NULL);
	}
	
	ClassObject->RefCount = 1;
	
	ClassObject->cbSize = lpwcx->cbSize;
	ClassObject->style = lpwcx->style;
	ClassObject->cbClsExtra = lpwcx->cbClsExtra;
	ClassObject->cbWndExtra = lpwcx->cbWndExtra;
	ClassObject->hInstance = lpwcx->hInstance;
	ClassObject->hIcon = lpwcx->hIcon;
	ClassObject->hCursor = lpwcx->hCursor;
	ClassObject->hbrBackground = lpwcx->hbrBackground;
	ClassObject->Unicode = !(Flags & REGISTERCLASS_ANSI);
	ClassObject->Global = Global;
	ClassObject->hIconSm = lpwcx->hIconSm;
	ClassObject->Atom = Atom;
	if (wpExtra == NULL) {
		if (Flags & REGISTERCLASS_ANSI)
		{
			ClassObject->lpfnWndProcA = lpwcx->lpfnWndProc;
			ClassObject->lpfnWndProcW = (WNDPROC)IntAddWndProcHandle(lpwcx->lpfnWndProc,FALSE);
		}
		else
		{
			ClassObject->lpfnWndProcW = lpwcx->lpfnWndProc;
			ClassObject->lpfnWndProcA = (WNDPROC)IntAddWndProcHandle(lpwcx->lpfnWndProc,TRUE);
		}
	} else {
		if (Flags & REGISTERCLASS_ANSI)
		{
			ClassObject->lpfnWndProcA = lpwcx->lpfnWndProc;
			ClassObject->lpfnWndProcW = wpExtra;
		}
		else
		{
			ClassObject->lpfnWndProcW = lpwcx->lpfnWndProc;
			ClassObject->lpfnWndProcA = wpExtra;
		}
	}
	if (MenuName->Length == 0)
	{
		ClassObject->lpszMenuName.Length =
		ClassObject->lpszMenuName.MaximumLength = 0;
		ClassObject->lpszMenuName.Buffer = MenuName->Buffer;
	}
	else
	{		
		ClassObject->lpszMenuName.Length =
		ClassObject->lpszMenuName.MaximumLength = MenuName->MaximumLength;
		ClassObject->lpszMenuName.Buffer = ExAllocatePoolWithTag(PagedPool, ClassObject->lpszMenuName.MaximumLength, TAG_STRING);
		RtlCopyUnicodeString(&ClassObject->lpszMenuName, MenuName);
	}
	/* Extra class data */
	if (ClassObject->cbClsExtra != 0)
	{
		ClassObject->ExtraData = (PCHAR)(ClassObject + 1);
		RtlZeroMemory(ClassObject->ExtraData, (ULONG)ClassObject->cbClsExtra);
	}
	else
	{
		ClassObject->ExtraData = NULL;
	}

	InitializeListHead(&ClassObject->ClassWindowsListHead);

	return(ClassObject);
}

PCLASS_OBJECT FASTCALL
IntRegisterClass(CONST WNDCLASSEXW *lpwcx, PUNICODE_STRING ClassName, PUNICODE_STRING MenuName,
                 WNDPROC wpExtra, DWORD Flags)
{
  PWINSTATION_OBJECT WinStaObject;
  RTL_ATOM Atom;
  NTSTATUS Status;
  PCLASS_OBJECT Ret;
  
  WinStaObject = PsGetWin32Process()->WindowStation;
  if(WinStaObject == NULL)
  {
    DPRINT1("Unable to register class process %d, window station is inaccessible\n", PsGetCurrentProcessId());
    return NULL;
  }
  
  if(ClassName->Length)
  {
    DPRINT1("IntRegisterClass(%wZ)\n", ClassName);
    Status = RtlAddAtomToAtomTable(WinStaObject->AtomTable,
                                   ClassName->Buffer,
                                   &Atom);
    if (!NT_SUCCESS(Status))
    {
      DPRINT("Failed adding class name (%wZ) to atom table\n", ClassName);
      SetLastNtError(Status);      
      return((RTL_ATOM)0);
    }
  }
  else
  {
    Atom = (RTL_ATOM)(ULONG)ClassName->Buffer;
  }
  
  Ret = IntCreateClass(lpwcx, Flags, wpExtra, MenuName, Atom);
  if(Ret == NULL)
  {
    if(ClassName->Length)
    {
      RtlDeleteAtomFromAtomTable(WinStaObject->AtomTable, Atom);
    }
    return NULL;
  }
  
  InsertTailList(&PsGetWin32Process()->ClassListHead, &Ret->ListEntry);
  
  return Ret;
}

ULONG FASTCALL
IntGetClassLong(PWINDOW_OBJECT WindowObject, ULONG Offset, BOOL Ansi)
{
  LONG Ret;

  if ((int)Offset >= 0)
    {
      DPRINT("GetClassLong(%x, %d)\n", WindowObject->Handle, Offset);
      if ((Offset + sizeof(LONG)) > WindowObject->Class->cbClsExtra)
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
      Ret = (ULONG)WindowObject->Class->lpszMenuName.Buffer;
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

ULONG FASTCALL
IntSetClassLong(PWINDOW_OBJECT WindowObject, ULONG Offset, LONG dwNewLong, BOOL Ansi)
{
  ULONG Ret = 0;
  
  if ((int)Offset >= 0)
    {
      LONG *Addr;
      DPRINT("SetClassLong(%x, %d, %x)\n", WindowObject->Handle, Offset, dwNewLong);
      if ((Offset + sizeof(LONG)) > WindowObject->Class->cbClsExtra)
	{
	  SetLastWin32Error(ERROR_INVALID_PARAMETER);
	  return 0;
	}
      Addr = ((LONG *)(WindowObject->Class->ExtraData + Offset));
      Ret = *Addr;
      *Addr = dwNewLong;
      return Ret;
    }
  
  switch (Offset)
    {
    case GCL_CBWNDEXTRA:
      Ret = WindowObject->Class->cbWndExtra;
      WindowObject->Class->cbWndExtra = dwNewLong;
      break;
    case GCL_CBCLSEXTRA:
      Ret = WindowObject->Class->cbClsExtra;
      WindowObject->Class->cbClsExtra = dwNewLong;
      break;
    case GCL_HBRBACKGROUND:
      Ret = (ULONG)WindowObject->Class->hbrBackground;
      WindowObject->Class->hbrBackground = (HBRUSH)dwNewLong;
      break;
    case GCL_HCURSOR:
      Ret = (ULONG)WindowObject->Class->hCursor;
      WindowObject->Class->hCursor = (HCURSOR)dwNewLong;
      break;
    case GCL_HICON:
      Ret = (ULONG)WindowObject->Class->hIcon;
      WindowObject->Class->hIcon = (HICON)dwNewLong;
      break;
    case GCL_HICONSM:
      Ret = (ULONG)WindowObject->Class->hIconSm;
      WindowObject->Class->hIconSm = (HICON)dwNewLong;
      break;
    case GCL_HMODULE:
      Ret = (ULONG)WindowObject->Class->hInstance;
      WindowObject->Class->hInstance = (HINSTANCE)dwNewLong;
      break;
    case GCL_MENUNAME:
      /* FIXME - what do we return in this case? */
      if (WindowObject->Class->lpszMenuName.MaximumLength)
        RtlFreeUnicodeString(&WindowObject->Class->lpszMenuName);
      if (!IS_INTRESOURCE(dwNewLong))
      {
        WindowObject->Class->lpszMenuName.Length =
        WindowObject->Class->lpszMenuName.MaximumLength = ((PUNICODE_STRING)dwNewLong)->MaximumLength;
        WindowObject->Class->lpszMenuName.Buffer = ExAllocatePoolWithTag(PagedPool, WindowObject->Class->lpszMenuName.MaximumLength, TAG_STRING);
        RtlCopyUnicodeString(&WindowObject->Class->lpszMenuName, (PUNICODE_STRING)dwNewLong);
      }
      else
      {
        WindowObject->Class->lpszMenuName.Length =
        WindowObject->Class->lpszMenuName.MaximumLength = 0;
        WindowObject->Class->lpszMenuName.Buffer = (LPWSTR)dwNewLong;
      }
      break;
    case GCL_STYLE:
      Ret = WindowObject->Class->style;
      WindowObject->Class->style = dwNewLong;
      break;
    case GCL_WNDPROC:
      /* FIXME - what do we return in this case? */
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
  
  return Ret;
}

