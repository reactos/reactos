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
/* $Id: class.c,v 1.59.8.6 2004/09/26 22:44:35 weiden Exp $
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

NTSTATUS INTERNAL_CALL
InitClassImpl(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS INTERNAL_CALL
CleanupClassImpl(VOID)
{
  return(STATUS_SUCCESS);
}

BOOL INTERNAL_CALL
IntReferenceClassByAtom(PCLASS_OBJECT* Class,
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
         ClassReferenceObject(Current);
         return TRUE;
      }

      if (Current->Atom == Atom && Current->Global)
         BestMatch = Current;

      CurrentEntry = CurrentEntry->Flink;
   }

   if (BestMatch != NULL)
   {
      *Class = BestMatch;
      ClassReferenceObject(BestMatch);
      return TRUE;
   }
  
   return FALSE;
}

BOOL INTERNAL_CALL
IntReferenceClassByName(PCLASS_OBJECT *Class,
                        PUNICODE_STRING ClassName,
                        HINSTANCE hInstance)
{
   RTL_ATOM ClassAtom;
   NTSTATUS Status;
   PWINSTATION_OBJECT WinStaObject = PsGetWin32Process()->WindowStation;

   if (!ClassName || !WinStaObject)
      return FALSE;

   Status = RtlLookupAtomInAtomTable(
      WinStaObject->AtomTable,
      (LPWSTR)ClassName->Buffer,
      &ClassAtom);

   if (!NT_SUCCESS(Status))
   {
      DbgPrint("RtlLookupAtomInAtomTable failed for %wZ\n", ClassName);
      return FALSE;
   }

   return IntReferenceClassByAtom(Class, ClassAtom, hInstance);
}

BOOL INTERNAL_CALL
IntReferenceClassByNameOrAtom(PCLASS_OBJECT *Class,
                              PUNICODE_STRING ClassNameOrAtom,
			      HINSTANCE hInstance)
{
   if (IS_ATOM(ClassNameOrAtom->Buffer))
      return IntReferenceClassByAtom(Class, (RTL_ATOM)((ULONG_PTR)ClassNameOrAtom->Buffer), hInstance);
   
   return IntReferenceClassByName(Class, ClassNameOrAtom, hInstance);
}

BOOL INTERNAL_CALL
IntGetClassName(PWINDOW_OBJECT WindowObject, PUNICODE_STRING ClassName, ULONG nMaxCount)
{
  ULONG Length;
  NTSTATUS Status;
  PWINSTATION_OBJECT WinStaObject = PsGetWin32Process()->WindowStation;

  if (!WinStaObject)
  {
    return 0;
  }
  
  ClassName->Length = 0;
  Status = RtlQueryAtomInAtomTable(WinStaObject->AtomTable,
                                   WindowObject->Class->Atom,
                                   NULL,
                                   NULL,
                                   NULL,
                                   &Length);
  if(!NT_SUCCESS(Status))
  {
    return FALSE;
  }
  
  ClassName->Length = (USHORT)Length;
  ClassName->MaximumLength = ClassName->Length + sizeof(WCHAR);
  ClassName->Buffer = ExAllocatePoolWithTag(PagedPool, ClassName->MaximumLength, TAG_STRING);
  
  if(!ClassName->Buffer)
  {
    DPRINT1("IntGetClassName: Not enough memory to allocate memory for the class name!\n");
    return FALSE;
  }
  
  Status = RtlQueryAtomInAtomTable(WinStaObject->AtomTable,
                                   WindowObject->Class->Atom,
                                   NULL,
                                   NULL,
                                   ClassName->Buffer,
                                   &Length);
  if(!NT_SUCCESS(Status))
  {
    DPRINT1("IntGetClassName: RtlQueryAtomInAtomTable failed\n");
    RtlFreeUnicodeString(ClassName);
    return FALSE;
  }
  
  ClassName->Length = (USHORT)Length;
  ClassName->Buffer[Length / sizeof(WCHAR)] = L'\0';
  
  return TRUE;
}

PCLASS_OBJECT INTERNAL_CALL
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
		if (IntReferenceClassByAtom(&ClassObject, Atom, lpwcx->hInstance))
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

PCLASS_OBJECT INTERNAL_CALL
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
    if(IS_ATOM(ClassName->Buffer))
    {
      DPRINT1("Unable to register class 0x%x (process %d), window station is inaccessible\n", ClassName->Buffer, PsGetCurrentProcessId());
    }
    else
    {
      DPRINT1("Unable to register class %wZ (process %d), window station is inaccessible\n", ClassName, PsGetCurrentProcessId());
    }
    return NULL;
  }
  
  /* FIXME - check the rights of the thread's desktop if we're allowed to register a class? */

  DPRINT("IntRegisterClass(%wZ)\n", ClassName);
  Status = RtlAddAtomToAtomTable(WinStaObject->AtomTable,
                                 ClassName->Buffer,
                                 &Atom);
  if (!NT_SUCCESS(Status))
  {
    DPRINT1("Failed adding class name (%wZ) to atom table\n", ClassName);
    SetLastNtError(Status);
    return NULL;
  }

  Ret = IntCreateClass(lpwcx, Flags, wpExtra, MenuName, Atom);
  if(Ret == NULL)
  {
    DPRINT1("Failed creating a class object (%wZ)\n", ClassName);
    RtlDeleteAtomFromAtomTable(WinStaObject->AtomTable, Atom);
    return NULL;
  }
  
  /* FIXME - reference the window station? */
  InsertTailList(&PsGetWin32Process()->ClassListHead, &Ret->ListEntry);
  
  return Ret;
}

ULONG INTERNAL_CALL
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

ULONG INTERNAL_CALL
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

BOOL INTERNAL_CALL
IntGetClassInfo(HINSTANCE hInstance, PUNICODE_STRING ClassName, LPWNDCLASSEXW lpWndClassEx, BOOL Ansi)
{
  PCLASS_OBJECT Class;
  
  if(IntReferenceClassByNameOrAtom(&Class, ClassName, hInstance))
  {
    RTL_ATOM Atom;
    
    lpWndClassEx->cbSize = sizeof(LPWNDCLASSEXW);
    lpWndClassEx->style = Class->style;
    lpWndClassEx->lpfnWndProc = (Ansi ? Class->lpfnWndProcA : Class->lpfnWndProcW);
    lpWndClassEx->cbClsExtra = Class->cbClsExtra;
    lpWndClassEx->cbWndExtra = Class->cbWndExtra;
    /* This is not typo, we're really not going to use Class->hInstance here. */
    lpWndClassEx->hInstance = hInstance;
    lpWndClassEx->hIcon = Class->hIcon;
    lpWndClassEx->hCursor = Class->hCursor;
    lpWndClassEx->hbrBackground = Class->hbrBackground;
    if (Class->lpszMenuName.MaximumLength)
      RtlCopyUnicodeString((PUNICODE_STRING)lpWndClassEx->lpszMenuName, &Class->lpszMenuName);
    else
      lpWndClassEx->lpszMenuName = Class->lpszMenuName.Buffer;
    /* ClassName->Buffer points to a buffer that is readable from umode because it was passed to kmode and we just probed it */
    lpWndClassEx->lpszClassName = ClassName->Buffer;
    lpWndClassEx->hIconSm = Class->hIconSm;
    Atom = Class->Atom;

    ObmDereferenceObject(Class);
    return Atom;
  }
  
  SetLastWin32Error(ERROR_CLASS_DOES_NOT_EXIST);
  return 0;
}

