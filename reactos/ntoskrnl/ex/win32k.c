/*
 *  ReactOS kernel
 *  Copyright (C) 1998, 1999, 2000, 2001 ReactOS Team
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
/*
 * PROJECT:         ReactOS kernel
 * FILE:            kernel/ex/win32k.c
 * PURPOSE:         Executive Win32 subsystem support
 * PROGRAMMER:      Casper S. Hornstrup (chorns@users.sourceforge.net)
 * UPDATE HISTORY:
 *      04-06-2001  CSH  Created
 */

#include <ntoskrnl.h>
#define NDEBUG
#include <internal/debug.h>

/* DATA **********************************************************************/

#define WINSTA_ACCESSCLIPBOARD	(0x4L)
#define WINSTA_ACCESSGLOBALATOMS	(0x20L)
#define WINSTA_CREATEDESKTOP	(0x8L)
#define WINSTA_ENUMDESKTOPS	(0x1L)
#define WINSTA_ENUMERATE	(0x100L)
#define WINSTA_EXITWINDOWS	(0x40L)
#define WINSTA_READATTRIBUTES	(0x2L)
#define WINSTA_READSCREEN	(0x200L)
#define WINSTA_WRITEATTRIBUTES	(0x10L)

#define DF_ALLOWOTHERACCOUNTHOOK	(0x1L)
#define DESKTOP_CREATEMENU	(0x4L)
#define DESKTOP_CREATEWINDOW	(0x2L)
#define DESKTOP_ENUMERATE	(0x40L)
#define DESKTOP_HOOKCONTROL	(0x8L)
#define DESKTOP_JOURNALPLAYBACK	(0x20L)
#define DESKTOP_JOURNALRECORD	(0x10L)
#define DESKTOP_READOBJECTS	(0x1L)
#define DESKTOP_SWITCHDESKTOP	(0x100L)
#define DESKTOP_WRITEOBJECTS	(0x80L)

POBJECT_TYPE EXPORTED ExWindowStationObjectType = NULL;
POBJECT_TYPE EXPORTED ExDesktopObjectType = NULL;

static GENERIC_MAPPING ExpWindowStationMapping = {
  STANDARD_RIGHTS_READ | WINSTA_ENUMDESKTOPS | WINSTA_ENUMERATE |  WINSTA_READATTRIBUTES | WINSTA_READSCREEN,
  STANDARD_RIGHTS_WRITE | WINSTA_ACCESSCLIPBOARD | WINSTA_CREATEDESKTOP | WINSTA_WRITEATTRIBUTES,
  STANDARD_RIGHTS_EXECUTE | WINSTA_ACCESSGLOBALATOMS | WINSTA_EXITWINDOWS,
  STANDARD_RIGHTS_REQUIRED | WINSTA_ACCESSCLIPBOARD | WINSTA_ACCESSGLOBALATOMS | WINSTA_CREATEDESKTOP |
                             WINSTA_ENUMDESKTOPS | WINSTA_ENUMERATE | WINSTA_EXITWINDOWS |
                             WINSTA_READATTRIBUTES | WINSTA_READSCREEN | WINSTA_WRITEATTRIBUTES
};

static GENERIC_MAPPING ExpDesktopMapping = {
  STANDARD_RIGHTS_READ | DESKTOP_ENUMERATE | DESKTOP_READOBJECTS,
  STANDARD_RIGHTS_WRITE | DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_HOOKCONTROL |
                          DESKTOP_JOURNALPLAYBACK | DESKTOP_JOURNALRECORD | DESKTOP_WRITEOBJECTS,
  STANDARD_RIGHTS_EXECUTE | DESKTOP_SWITCHDESKTOP,
  STANDARD_RIGHTS_REQUIRED | DESKTOP_CREATEMENU | DESKTOP_CREATEWINDOW | DESKTOP_ENUMERATE |
                             DESKTOP_HOOKCONTROL | DESKTOP_JOURNALPLAYBACK | DESKTOP_JOURNALRECORD |
                             DESKTOP_READOBJECTS | DESKTOP_SWITCHDESKTOP | DESKTOP_WRITEOBJECTS
};

/* FUNCTIONS ****************************************************************/


NTSTATUS STDCALL
ExpWinStaObjectCreate(PVOID ObjectBody,
		      PVOID Parent,
		      PWSTR RemainingPath,
		      struct _OBJECT_ATTRIBUTES* ObjectAttributes)
{
  PWINSTATION_OBJECT WinSta = (PWINSTATION_OBJECT)ObjectBody;
  UNICODE_STRING UnicodeString;
  NTSTATUS Status;

  if (RemainingPath == NULL)
	{
		return STATUS_SUCCESS;
	}

  if (wcschr((RemainingPath + 1), '\\') != NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

  RtlInitUnicodeString(&UnicodeString, (RemainingPath + 1));

  DPRINT("Creating window station (0x%X)  Name (%wZ)\n", WinSta, &UnicodeString);

  Status = RtlCreateUnicodeString(&WinSta->Name, UnicodeString.Buffer);
  if (!NT_SUCCESS(Status))
  {
    return Status;
  }

  KeInitializeSpinLock(&WinSta->Lock);

  InitializeListHead(&WinSta->DesktopListHead);

#if 1
  WinSta->AtomTable = NULL;
#endif

  Status = RtlCreateAtomTable(37, &WinSta->AtomTable);
  if (!NT_SUCCESS(Status))
  {
    RtlFreeUnicodeString(&WinSta->Name);
    return Status;
  }

  WinSta->SystemMenuTemplate = (HANDLE)0;

  DPRINT("Window station successfully created. Name (%wZ)\n", &WinSta->Name);

  return STATUS_SUCCESS;
}

VOID STDCALL
ExpWinStaObjectDelete(PVOID DeletedObject)
{
  PWINSTATION_OBJECT WinSta = (PWINSTATION_OBJECT)DeletedObject;

  DPRINT("Deleting window station (0x%X)\n", WinSta);

  RtlDestroyAtomTable(WinSta->AtomTable);

  RtlFreeUnicodeString(&WinSta->Name);
}

PVOID
ExpWinStaObjectFind(PWINSTATION_OBJECT WinStaObject,
		    PWSTR Name,
		    ULONG Attributes)
{
  PLIST_ENTRY Current;
  PDESKTOP_OBJECT CurrentObject;

  DPRINT("WinStaObject (0x%X)  Name (%wS)\n", WinStaObject, Name);

  if (Name[0] == 0)
  {
    return NULL;
  }

  Current = WinStaObject->DesktopListHead.Flink;
  while (Current != &WinStaObject->DesktopListHead)
  {
    CurrentObject = CONTAINING_RECORD(Current, DESKTOP_OBJECT, ListEntry);
    DPRINT("Scanning %wZ for %wS\n", &CurrentObject->Name, Name);
    if (Attributes & OBJ_CASE_INSENSITIVE)
    {
      if (_wcsicmp(CurrentObject->Name.Buffer, Name) == 0)
      {
        DPRINT("Found desktop at (0x%X)\n", CurrentObject);
        return CurrentObject;
      }
    }
    else
    {
      if (wcscmp(CurrentObject->Name.Buffer, Name) == 0)
      {
        DPRINT("Found desktop at (0x%X)\n", CurrentObject);
        return CurrentObject;
      }
    }
    Current = Current->Flink;
  }

  DPRINT("Returning NULL\n");

  return NULL;
}

NTSTATUS STDCALL
ExpWinStaObjectParse(PVOID Object,
		     PVOID *NextObject,
		     PUNICODE_STRING FullPath,
		     PWSTR *Path,
		     ULONG Attributes)
{
  PVOID FoundObject;
  NTSTATUS Status;
  PWSTR End;

  DPRINT("Object (0x%X)  Path (0x%X)  *Path (%wS)\n", Object, Path, *Path);

  *NextObject = NULL;

  if ((Path == NULL) || ((*Path) == NULL))
  {
    return STATUS_SUCCESS;
  }

  End = wcschr((*Path) + 1, '\\');
  if (End != NULL)
  {
    DPRINT("Name contains illegal characters\n");
    return STATUS_UNSUCCESSFUL;
  }

  FoundObject = ExpWinStaObjectFind(Object, (*Path) + 1, Attributes);
  if (FoundObject == NULL)
  {
    DPRINT("Name was not found\n");
    return STATUS_UNSUCCESSFUL;
  }

  Status = ObReferenceObjectByPointer(
    FoundObject,
    STANDARD_RIGHTS_REQUIRED,
    NULL,
    UserMode);

  *NextObject = FoundObject;
  *Path = NULL;

  return Status;
}

NTSTATUS STDCALL
ExpDesktopObjectCreate(PVOID ObjectBody,
		       PVOID Parent,
		       PWSTR RemainingPath,
		       struct _OBJECT_ATTRIBUTES* ObjectAttributes)
{
  PDESKTOP_OBJECT Desktop = (PDESKTOP_OBJECT)ObjectBody;
  UNICODE_STRING UnicodeString;

  if (RemainingPath == NULL)
	{
		return STATUS_SUCCESS;
	}

  if (wcschr((RemainingPath + 1), '\\') != NULL)
	{
		return STATUS_UNSUCCESSFUL;
	}

  RtlInitUnicodeString(&UnicodeString, (RemainingPath + 1));

  DPRINT("Creating desktop (0x%X)  Name (%wZ)\n", Desktop, &UnicodeString);

  KeInitializeSpinLock(&Desktop->Lock);

  Desktop->WindowStation = (PWINSTATION_OBJECT)Parent;

  /* Put the desktop on the window station's list of associcated desktops */
  ExInterlockedInsertTailList(
    &Desktop->WindowStation->DesktopListHead,
    &Desktop->ListEntry,
    &Desktop->WindowStation->Lock);

  return RtlCreateUnicodeString(&Desktop->Name, UnicodeString.Buffer);
}

VOID STDCALL
ExpDesktopObjectDelete(PVOID DeletedObject)
{
  PDESKTOP_OBJECT Desktop = (PDESKTOP_OBJECT)DeletedObject;
  KIRQL OldIrql;

  DPRINT("Deleting desktop (0x%X)\n", Desktop);

  /* Remove the desktop from the window station's list of associcated desktops */
  KeAcquireSpinLock(&Desktop->WindowStation->Lock, &OldIrql);
  RemoveEntryList(&Desktop->ListEntry);
  KeReleaseSpinLock(&Desktop->WindowStation->Lock, OldIrql);

  RtlFreeUnicodeString(&Desktop->Name);
}

VOID INIT_FUNCTION
ExpWin32kInit(VOID)
{
  /* Create window station object type */
  ExWindowStationObjectType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  if (ExWindowStationObjectType == NULL)
  {
    CPRINT("Could not create window station object type\n");
    KEBUGCHECK(0);
  }

  ExWindowStationObjectType->Tag = TAG('W', 'I', 'N', 'S');
  ExWindowStationObjectType->TotalObjects = 0;
  ExWindowStationObjectType->TotalHandles = 0;
  ExWindowStationObjectType->MaxObjects = ULONG_MAX;
  ExWindowStationObjectType->MaxHandles = ULONG_MAX;
  ExWindowStationObjectType->PagedPoolCharge = 0;
  ExWindowStationObjectType->NonpagedPoolCharge = sizeof(WINSTATION_OBJECT);
  ExWindowStationObjectType->Mapping = &ExpWindowStationMapping;
  ExWindowStationObjectType->Dump = NULL;
  ExWindowStationObjectType->Open = NULL;
  ExWindowStationObjectType->Close = NULL;
  ExWindowStationObjectType->Delete = ExpWinStaObjectDelete;
  ExWindowStationObjectType->Parse = ExpWinStaObjectParse;
  ExWindowStationObjectType->Security = NULL;
  ExWindowStationObjectType->QueryName = NULL;
  ExWindowStationObjectType->OkayToClose = NULL;
  ExWindowStationObjectType->Create = ExpWinStaObjectCreate;
  ExWindowStationObjectType->DuplicationNotify = NULL;
  RtlRosInitUnicodeStringFromLiteral(&ExWindowStationObjectType->TypeName, L"WindowStation");

  ObpCreateTypeObject(ExWindowStationObjectType);

  /* Create desktop object type */
  ExDesktopObjectType = ExAllocatePool(NonPagedPool, sizeof(OBJECT_TYPE));
  if (ExDesktopObjectType == NULL)
  {
    CPRINT("Could not create desktop object type\n");
    KEBUGCHECK(0);
  }

  ExDesktopObjectType->Tag = TAG('D', 'E', 'S', 'K');
  ExDesktopObjectType->TotalObjects = 0;
  ExDesktopObjectType->TotalHandles = 0;
  ExDesktopObjectType->MaxObjects = ULONG_MAX;
  ExDesktopObjectType->MaxHandles = ULONG_MAX;
  ExDesktopObjectType->PagedPoolCharge = 0;
  ExDesktopObjectType->NonpagedPoolCharge = sizeof(DESKTOP_OBJECT);
  ExDesktopObjectType->Mapping = &ExpDesktopMapping;
  ExDesktopObjectType->Dump = NULL;
  ExDesktopObjectType->Open = NULL;
  ExDesktopObjectType->Close = NULL;
  ExDesktopObjectType->Delete = ExpDesktopObjectDelete;
  ExDesktopObjectType->Parse = NULL;
  ExDesktopObjectType->Security = NULL;
  ExDesktopObjectType->QueryName = NULL;
  ExDesktopObjectType->OkayToClose = NULL;
  ExDesktopObjectType->Create = ExpDesktopObjectCreate;
  ExDesktopObjectType->DuplicationNotify = NULL;
  RtlRosInitUnicodeStringFromLiteral(&ExDesktopObjectType->TypeName, L"Desktop");

  ObpCreateTypeObject(ExDesktopObjectType);
}

/* EOF */
