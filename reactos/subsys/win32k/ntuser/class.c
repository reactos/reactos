/* $Id: class.c,v 1.1 2001/06/12 17:50:29 chorns Exp $
 *
 * COPYRIGHT:        See COPYING in the top level directory
 * PROJECT:          ReactOS kernel
 * PURPOSE:          Window classes
 * FILE:             subsys/win32k/ntuser/class.c
 * PROGRAMER:        Casper S. Hornstrup (chorns@users.sourceforge.net)
 * REVISION HISTORY:
 *       06-06-2001  CSH  Created
 */
#include <ddk/ntddk.h>
#include <win32k/win32k.h>
#include <include/class.h>
#include <include/error.h>
#include <include/object.h>
#include <include/winsta.h>

#define NDEBUG
#include <debug.h>


/* List of system classes */
LIST_ENTRY SystemClassListHead;
FAST_MUTEX SystemClassListLock;


NTSTATUS
InitClassImpl(VOID)
{
  InitializeListHead(&SystemClassListHead);
  //ExInitializeFastMutex(&SystemClassListLock);

  return STATUS_SUCCESS;
}

NTSTATUS
CleanupClassImpl(VOID)
{
  //ExReleaseFastMutex(&SystemClassListLock);

  return STATUS_SUCCESS;
}


DWORD
CliFindClassByName(
  PWNDCLASS_OBJECT *Class,
  LPWSTR ClassName,
  PLIST_ENTRY ListHead)
{
  PWNDCLASS_OBJECT Current;
  PLIST_ENTRY CurrentEntry;

  CurrentEntry = ListHead->Flink;
  while (CurrentEntry != ListHead)
  {
    Current = CONTAINING_RECORD(CurrentEntry, WNDCLASS_OBJECT, ListEntry);

    if (_wcsicmp(ClassName, Current->Class.lpszClassName) == 0)
    {
      *Class = Current;
      ObmReferenceObject(Current);
      return STATUS_SUCCESS;
    }

    CurrentEntry = CurrentEntry->Flink;
  }

  return STATUS_NOT_FOUND;
}

NTSTATUS
CliReferenceClassByNameWinSta(
  PWINSTATION_OBJECT WinStaObject,
  PWNDCLASS_OBJECT *Class,
  LPWSTR ClassName)
{
/*
  if (NT_SUCCESS(CliFindClassByName(Class, ClassName, &LocalClassListHead)))
  {
    return STATUS_SUCCESS;
  }

  if (NT_SUCCESS(CliFindClassByName(Class, ClassName, &GlobalClassListHead)))
  {
    return STATUS_SUCCESS;
  }
*/
  return CliFindClassByName(Class, ClassName, &SystemClassListHead);
}

NTSTATUS
ClassReferenceClassByName(
  PWNDCLASS_OBJECT *Class,
  LPWSTR ClassName)
{
  PWINSTATION_OBJECT WinStaObject;
  NTSTATUS Status;

  if (!ClassName)
  {
    return STATUS_INVALID_PARAMETER;
  }

  Status = ValidateWindowStationHandle(
    PROCESS_WINDOW_STATION(),
    KernelMode,
    0,
    &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    return STATUS_UNSUCCESSFUL;
  }

  Status = CliReferenceClassByNameWinSta(
    WinStaObject,
    Class,
    ClassName);

  ObDereferenceObject(WinStaObject);

  return Status;
}

NTSTATUS
ClassReferenceClassByAtom(
  PWNDCLASS_OBJECT *Class,
  RTL_ATOM ClassAtom)
{
  PWINSTATION_OBJECT WinStaObject;
  ULONG ClassNameLength;
  WCHAR ClassName[256];
  NTSTATUS Status;

  if (!ClassAtom)
  {
    return STATUS_INVALID_PARAMETER;
  }

  Status = ValidateWindowStationHandle(
    PROCESS_WINDOW_STATION(),
    KernelMode,
    0,
    &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    return STATUS_UNSUCCESSFUL;
  }

  ClassNameLength = sizeof(ClassName);
  Status = RtlQueryAtomInAtomTable(
    WinStaObject->AtomTable,
    ClassAtom,
    NULL,
    NULL,
    &ClassName[0],
    &ClassNameLength);

  Status = CliReferenceClassByNameWinSta(
    WinStaObject,
    Class,
    &ClassName[0]);

  ObDereferenceObject(WinStaObject);

  return Status;
}

NTSTATUS
ClassReferenceClassByNameOrAtom(
  PWNDCLASS_OBJECT *Class,
  LPWSTR ClassNameOrAtom)
{
  NTSTATUS Status;

  if (IS_ATOM(ClassNameOrAtom))
  {
    Status = ClassReferenceClassByAtom(Class, (RTL_ATOM)((ULONG_PTR)ClassNameOrAtom));
  }
  else
  {
    Status = ClassReferenceClassByName(Class, ClassNameOrAtom);
  }

  if (!NT_SUCCESS(Status))
  {
    SetLastNtError(Status);
  }

  return Status;
}


DWORD
STDCALL
NtUserGetClassInfo(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetClassName(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserGetWOWClass(
  DWORD Unknown0,
  DWORD Unknown1)
{
  UNIMPLEMENTED

  return 0;
}

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
RTL_ATOM
STDCALL
NtUserRegisterClassExWOW(
  LPWNDCLASSEX lpwcx,
  BOOL bUnicodeClass,
  DWORD Unknown2,
  DWORD Unknown3,
  DWORD Unknown4,
  DWORD Unknown5)
{
  PWINSTATION_OBJECT WinStaObject;
  PWNDCLASS_OBJECT ClassObject;
  NTSTATUS Status;
  RTL_ATOM Atom;

  DPRINT("About to open window station handle (0x%X)\n", PROCESS_WINDOW_STATION());

  Status = ValidateWindowStationHandle(
    PROCESS_WINDOW_STATION(),
    KernelMode,
    0,
    &WinStaObject);
  if (!NT_SUCCESS(Status))
  {
    DPRINT("Validation of window station handle (0x%X) failed\n",
      PROCESS_WINDOW_STATION());
    return (RTL_ATOM)0;
  }

  Status = RtlAddAtomToAtomTable(
    WinStaObject->AtomTable,
    (LPWSTR)lpwcx->lpszClassName,
    &Atom);
  if (!NT_SUCCESS(Status))
  {
    ObDereferenceObject(WinStaObject);
    DPRINT("Failed adding class name (%wS) to atom table\n",
      lpwcx->lpszClassName);
    SetLastNtError(Status);
    return (RTL_ATOM)0;
  }

  ClassObject = ObmCreateObject(
    WinStaObject->HandleTable,
    NULL,
    otClass,
    sizeof(WNDCLASS_OBJECT));
  if (!ClassObject)
  {
    RtlDeleteAtomFromAtomTable(WinStaObject->AtomTable, Atom);
    ObDereferenceObject(WinStaObject);
    DPRINT("Failed creating window class object\n");
    SetLastNtError(STATUS_INSUFFICIENT_RESOURCES);
    return (ATOM)0;
  }

  if (ClassObject->Class.style & CS_GLOBALCLASS)
  {
    /* FIXME: Put on global list */
    InsertTailList(&SystemClassListHead, &ClassObject->ListEntry);
  }
  else
  {
    /* FIXME: Put on local list */
    InsertTailList(&SystemClassListHead, &ClassObject->ListEntry);
  }

  ObDereferenceObject(WinStaObject);

  return (RTL_ATOM)0;
}

DWORD
STDCALL
NtUserSetClassLong(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2,
  DWORD Unknown3)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserSetClassWord(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

DWORD
STDCALL
NtUserUnregisterClass(
  DWORD Unknown0,
  DWORD Unknown1,
  DWORD Unknown2)
{
  UNIMPLEMENTED

  return 0;
}

/* EOF */
