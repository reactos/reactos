/* $Id: class.c,v 1.8 2002/07/04 19:56:37 dwelch Exp $
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

//#define NDEBUG
#include <debug.h>

/* FUNCTIONS *****************************************************************/

NTSTATUS
InitClassImpl(VOID)
{
  return(STATUS_SUCCESS);
}

NTSTATUS
CleanupClassImpl(VOID)
{
  return(STATUS_SUCCESS);
}


NTSTATUS
ClassReferenceClassByName(PW32PROCESS Process,
			  PWNDCLASS_OBJECT* Class,
			  LPWSTR ClassName)
{
  PWNDCLASS_OBJECT Current;
  PLIST_ENTRY CurrentEntry;
  
  ExAcquireFastMutexUnsafe (&Process->ClassListLock);
  CurrentEntry = Process->ClassListHead.Flink;
  while (CurrentEntry != &Process->ClassListHead)
    {
      Current = CONTAINING_RECORD(CurrentEntry, WNDCLASS_OBJECT, ListEntry);
      
      if (_wcsicmp(ClassName, Current->Class.lpszClassName) == 0)
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

NTSTATUS
ClassReferenceClassByAtom(PWNDCLASS_OBJECT *Class,
			  RTL_ATOM ClassAtom)
{
  PWINSTATION_OBJECT WinStaObject;
  ULONG ClassNameLength;
  WCHAR ClassName[256];
  NTSTATUS Status;

  if (!ClassAtom)
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

  ClassNameLength = sizeof(ClassName);
  Status = RtlQueryAtomInAtomTable(WinStaObject->AtomTable,
				   ClassAtom,
				   NULL,
				   NULL,
				   &ClassName[0],
				   &ClassNameLength);
  
  Status = ClassReferenceClassByName(PsGetWin32Process(),
				     Class,
				     &ClassName[0]);
  
  ObDereferenceObject(WinStaObject);
  
  return(Status);
}

NTSTATUS
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
      Status = ClassReferenceClassByName(PsGetWin32Process(), Class, 
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

DWORD STDCALL
NtUserGetClassName(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2)
{
  UNIMPLEMENTED;
    
  return(0);
}

DWORD STDCALL
NtUserGetWOWClass(DWORD Unknown0,
		  DWORD Unknown1)
{
  UNIMPLEMENTED;
  
  return(0);
}

RTL_ATOM STDCALL
NtUserRegisterClassExWOW(LPWNDCLASSEX lpwcx,
			 BOOL bUnicodeClass,
			 DWORD Unknown2,
			 DWORD Unknown3,
			 DWORD Unknown4,
			 DWORD Unknown5)
/*
 * FUNCTION:
 *   Registers a new class with the window manager
 * ARGUMENTS:
 *   lpcx          = Win32 extended window class structure
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
  WORD  objectSize;
  LPTSTR  namePtr;
  
  W32kGuiCheck();

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

  objectSize = sizeof(WNDCLASS_OBJECT) +
    (lpwcx->lpszMenuName != 0 ? ((wcslen (lpwcx->lpszMenuName) + 1) * 2) : 0) +
    ((wcslen (lpwcx->lpszClassName) + 1) * 2);
  ClassObject = ObmCreateObject(NULL, NULL, otClass, objectSize);
  if (ClassObject == 0)
    {
      RtlDeleteAtomFromAtomTable(WinStaObject->AtomTable, Atom);
      ObDereferenceObject(WinStaObject);
      DPRINT("Failed creating window class object\n");
      SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
      
      return((RTL_ATOM)0);
    }

  ClassObject->Class = *lpwcx;
  ClassObject->Unicode = bUnicodeClass;
  namePtr = (LPTSTR)(((PCHAR)ClassObject) + sizeof (WNDCLASS_OBJECT));
  if (lpwcx->lpszMenuName != 0)
    {
      ClassObject->Class.lpszMenuName = namePtr;
      wcscpy (namePtr, lpwcx->lpszMenuName);
      namePtr += wcslen (lpwcx->lpszMenuName + 1);
    }
  ClassObject->Class.lpszClassName = namePtr;
  wcscpy (namePtr, lpwcx->lpszClassName);
  ExAcquireFastMutex(&PsGetWin32Process()->ClassListLock);
  InsertTailList(&PsGetWin32Process()->ClassListHead, &ClassObject->ListEntry);
  ExReleaseFastMutex(&PsGetWin32Process()->ClassListLock);
  
  ObDereferenceObject(WinStaObject);
  
  return(Atom);
}

DWORD STDCALL
NtUserGetClassLong(HWND hWnd, DWORD Offset)
{
}

DWORD STDCALL
NtUserSetClassLong(DWORD Unknown0,
		   DWORD Unknown1,
		   DWORD Unknown2,
		   DWORD Unknown3)
{
  UNIMPLEMENTED;

  return(0);
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
