/*
 *  ReactOS kernel
 *  Copyright (C) 2004 ReactOS Team
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
/* $Id: settings.c,v 1.3 2004/06/07 12:21:36 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/settings.c
 * PURPOSE:         Device settings support functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <ntos/minmax.h>

#include "usetup.h"
#include "infcache.h"
#include "genlist.h"
#include "settings.h"

#define NDEBUG
#include <debug.h>


/* FUNCTIONS ****************************************************************/

PGENERIC_LIST
CreateComputerTypeList(HINF InfFile)
{
  CHAR Buffer[128];
  PGENERIC_LIST List;
  INFCONTEXT Context;
  PWCHAR KeyName;
  PWCHAR KeyValue;
  PWCHAR UserData;

  List = CreateGenericList();
  if (List == NULL)
    return NULL;

  if (!InfFindFirstLine (InfFile, L"Computer", NULL, &Context))
    {
      DestroyGenericList(List, FALSE);
      return NULL;
    }

  do
    {
      if (!InfGetData (&Context, &KeyName, &KeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT("InfGetData() failed\n");
	  break;
	}

      UserData = RtlAllocateHeap(ProcessHeap,
				 0,
				 (wcslen(KeyName) + 1) * sizeof(WCHAR));
      if (UserData == NULL)
	{
	  /* FIXME: Handle error! */
	}

      wcscpy(UserData, KeyName);

      sprintf(Buffer, "%S", KeyValue);
      AppendGenericListEntry(List, Buffer, UserData, FALSE);
    }
  while (InfFindNextLine(&Context, &Context));

  return List;
}


PGENERIC_LIST
CreateDisplayDriverList(HINF InfFile)
{
  CHAR Buffer[128];
  PGENERIC_LIST List;
  INFCONTEXT Context;
  PWCHAR KeyName;
  PWCHAR KeyValue;
  PWCHAR UserData;

  List = CreateGenericList();
  if (List == NULL)
    return NULL;

  if (!InfFindFirstLine (InfFile, L"Display", NULL, &Context))
    {
      DestroyGenericList(List, FALSE);
      return NULL;
    }

  do
    {
      if (!InfGetData (&Context, &KeyName, &KeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT("InfGetData() failed\n");
	  break;
	}

      UserData = RtlAllocateHeap(ProcessHeap,
				 0,
				 (wcslen(KeyName) + 1) * sizeof(WCHAR));
      if (UserData == NULL)
	{
	  /* FIXME: Handle error! */
	}

      wcscpy(UserData, KeyName);

      sprintf(Buffer, "%S", KeyValue);
      AppendGenericListEntry(List, Buffer, UserData, FALSE);
    }
  while (InfFindNextLine(&Context, &Context));

  AppendGenericListEntry(List, "Automatic detection", NULL, TRUE);

  return List;
}


PGENERIC_LIST
CreateKeyboardDriverList(HINF InfFile)
{
  CHAR Buffer[128];
  PGENERIC_LIST List;
  INFCONTEXT Context;
  PWCHAR KeyName;
  PWCHAR KeyValue;
  PWCHAR UserData;

  List = CreateGenericList();
  if (List == NULL)
    return NULL;

  if (!InfFindFirstLine (InfFile, L"Keyboard", NULL, &Context))
    {
      DestroyGenericList(List, FALSE);
      return NULL;
    }

  do
    {
      if (!InfGetData (&Context, &KeyName, &KeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT("InfGetData() failed\n");
	  break;
	}

      UserData = RtlAllocateHeap(ProcessHeap,
				 0,
				 (wcslen(KeyName) + 1) * sizeof(WCHAR));
      if (UserData == NULL)
	{
	  /* FIXME: Handle error! */
	}

      wcscpy(UserData, KeyName);

      sprintf(Buffer, "%S", KeyValue);
      AppendGenericListEntry(List, Buffer, UserData, FALSE);
    }
  while (InfFindNextLine(&Context, &Context));

  return List;
}


PGENERIC_LIST
CreateKeyboardLayoutList(HINF InfFile)
{
  CHAR Buffer[128];
  PGENERIC_LIST List;
  INFCONTEXT Context;
  PWCHAR KeyName;
  PWCHAR KeyValue;
  PWCHAR UserData;
  WCHAR DefaultLayout[20];

  /* Get default layout id */
  if (!InfFindFirstLine (InfFile, L"NLS", L"DefaultLayout", &Context))
    return NULL;

  if (!InfGetData (&Context, NULL, &KeyValue))
    return NULL;

  wcscpy(DefaultLayout, KeyValue);

  List = CreateGenericList();
  if (List == NULL)
    return NULL;

  if (!InfFindFirstLine (InfFile, L"KeyboardLayout", NULL, &Context))
    {
      DestroyGenericList(List, FALSE);
      return NULL;
    }

  do
    {
      if (!InfGetData (&Context, &KeyName, &KeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT("InfGetData() failed\n");
	  break;
	}

      UserData = RtlAllocateHeap(ProcessHeap,
				 0,
				 (wcslen(KeyName) + 1) * sizeof(WCHAR));
      if (UserData == NULL)
	{
	  /* FIXME: Handle error! */
	}

      wcscpy(UserData, KeyName);

      sprintf(Buffer, "%S", KeyValue);
      AppendGenericListEntry(List,
			     Buffer,
			     UserData,
			     _wcsicmp(KeyName, DefaultLayout) ? FALSE : TRUE);
    }
  while (InfFindNextLine(&Context, &Context));

  return List;
}


BOOLEAN
ProcessKeyboardLayoutRegistry(PGENERIC_LIST List)
{
  PGENERIC_LIST_ENTRY Entry;
  PWCHAR LanguageId;
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  UNICODE_STRING ValueName;
  HANDLE KeyHandle;
  NTSTATUS Status;

  Entry = GetGenericListEntry(List);
  if (Entry == NULL)
    return FALSE;

  LanguageId = (PWCHAR)Entry->UserData;
  if (LanguageId == NULL)
    return FALSE;

  /* Open the nls language key */
  RtlInitUnicodeString(&KeyName,
		       L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\Language");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status =  NtOpenKey(&KeyHandle,
		      KEY_ALL_ACCESS,
		      &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
      return FALSE;
    }

  /* Set default language */
  RtlInitUnicodeString(&ValueName,
		       L"Default");
  Status = NtSetValueKey (KeyHandle,
			  &ValueName,
			  0,
			  REG_SZ,
			  (PVOID)(LanguageId + 4),
			  8);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
      NtClose(KeyHandle);
      return FALSE;
    }

  /* Set install language */
  RtlInitUnicodeString(&ValueName,
		       L"InstallLanguage");
  Status = NtSetValueKey (KeyHandle,
			  &ValueName,
			  0,
			  REG_SZ,
			  (PVOID)(LanguageId + 4),
			  8);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
      NtClose(KeyHandle);
      return FALSE;
    }

  NtClose(KeyHandle);

  /* Open the nls locale key */
  RtlInitUnicodeString(&KeyName,
		       L"\\Registry\\Machine\\SYSTEM\\CurrentControlSet\\Control\\NLS\\Locale");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status =  NtOpenKey(&KeyHandle,
		      KEY_ALL_ACCESS,
		      &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtOpenKey() failed (Status %lx)\n", Status);
      return FALSE;
    }

  /* Set default locale */
  RtlInitUnicodeString(&ValueName,
		       L"(Default)");
  Status = NtSetValueKey (KeyHandle,
			  &ValueName,
			  0,
			  REG_SZ,
			  (PVOID)LanguageId,
			  16);
  if (!NT_SUCCESS(Status))
    {
      DPRINT1("NtSetValueKey() failed (Status %lx)\n", Status);
      NtClose(KeyHandle);
      return FALSE;
    }

  NtClose(KeyHandle);

  return TRUE;
}


#if 0
BOOLEAN
ProcessKeyboardLayoutFiles(PGENERIC_LIST List)
{
  return TRUE;
}
#endif


static BOOLEAN
GetMouseIdentifier(PWSTR ControllerType,
		   PWSTR Identifier,
		   ULONG IdentifierLength)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  WCHAR Buffer[32];
  HANDLE BusKey;
  HANDLE BusInstanceKey;
  HANDLE ControllerKey;
  HANDLE ControllerInstanceKey;
  HANDLE PeripheralKey;
  HANDLE PeripheralInstanceKey;
  ULONG BusInstance;
  ULONG ControllerInstance;
  ULONG PeripheralInstance;
  ULONG BufferLength;
  ULONG ReturnedLength;
  PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
  NTSTATUS Status;

  DPRINT("GetMouseIdentifier() called\n");

  /* Open the bus key */
  RtlInitUnicodeString(&KeyName,
		       L"\\Registry\\Machine\\HARDWARE\\Description\\System\\MultifunctionAdapter");
  InitializeObjectAttributes(&ObjectAttributes,
			     &KeyName,
			     OBJ_CASE_INSENSITIVE,
			     NULL,
			     NULL);
  Status = NtOpenKey(&BusKey,
		     KEY_ALL_ACCESS,
		     &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtOpenKey() failed (Status %lx)\n", Status);
      return FALSE;
    }

  BusInstance = 0;
  while (TRUE)
    {
      swprintf(Buffer, L"%lu", BusInstance);
      RtlInitUnicodeString(&KeyName,
			   Buffer);
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyName,
				 OBJ_CASE_INSENSITIVE,
				 BusKey,
				 NULL);
      Status = NtOpenKey(&BusInstanceKey,
			 KEY_ALL_ACCESS,
			 &ObjectAttributes);
      if (!NT_SUCCESS(Status))
	{
	  DPRINT("NtOpenKey() failed (Status %lx)\n", Status);
	  NtClose(BusKey);
	  return FALSE;
	}

      /* Open the controller type key */
      RtlInitUnicodeString(&KeyName,
			   ControllerType);
      InitializeObjectAttributes(&ObjectAttributes,
				 &KeyName,
				 OBJ_CASE_INSENSITIVE,
				 BusInstanceKey,
				 NULL);
      Status = NtOpenKey(&ControllerKey,
			 KEY_ALL_ACCESS,
			 &ObjectAttributes);
      if (NT_SUCCESS(Status))
	{
	  ControllerInstance = 0;
	  while (TRUE)
	    {
	      /* Open the pointer controller instance key */
	      swprintf(Buffer, L"%lu", ControllerInstance);
	      RtlInitUnicodeString(&KeyName,
				   Buffer);
	      InitializeObjectAttributes(&ObjectAttributes,
					 &KeyName,
					 OBJ_CASE_INSENSITIVE,
					 ControllerKey,
					 NULL);
	      Status = NtOpenKey(&ControllerInstanceKey,
				 KEY_ALL_ACCESS,
				 &ObjectAttributes);
	      if (!NT_SUCCESS(Status))
		{
		  DPRINT("NtOpenKey() failed (Status %lx)\n", Status);
		  NtClose(ControllerKey);
		  NtClose(BusInstanceKey);
		  NtClose(BusKey);
		  return FALSE;
		}

	      /* Open the 'PointerPeripheral' key */
	      RtlInitUnicodeString(&KeyName,
				   L"PointerPeripheral");
	      InitializeObjectAttributes(&ObjectAttributes,
					 &KeyName,
					 OBJ_CASE_INSENSITIVE,
					 ControllerInstanceKey,
					 NULL);
	      Status = NtOpenKey(&PeripheralKey,
				 KEY_ALL_ACCESS,
				 &ObjectAttributes);
	      if (NT_SUCCESS(Status))
		{
		  PeripheralInstance = 0;
		  while (TRUE)
		    {
		      /* Open the pointer peripheral instance key */
		      swprintf(Buffer, L"%lu", PeripheralInstance);
		      RtlInitUnicodeString(&KeyName,
					   Buffer);
		      InitializeObjectAttributes(&ObjectAttributes,
						 &KeyName,
						 OBJ_CASE_INSENSITIVE,
						 PeripheralKey,
						 NULL);
		      Status = NtOpenKey(&PeripheralInstanceKey,
					 KEY_ALL_ACCESS,
					 &ObjectAttributes);
		      if (!NT_SUCCESS(Status))
			{
			  DPRINT("NtOpenKey() failed (Status %lx)\n", Status);
			  NtClose(PeripheralKey);
			  NtClose(ControllerInstanceKey);
			  NtClose(ControllerKey);
			  NtClose(BusInstanceKey);
			  NtClose(BusKey);
			  return FALSE;
			}

		      /* Get peripheral identifier */
		      RtlInitUnicodeString(&KeyName,
					   L"Identifier");

		      BufferLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
				     256 * sizeof(WCHAR);
		      ValueInfo = RtlAllocateHeap(RtlGetProcessHeap(),
						  0,
						  BufferLength);
		      if (ValueInfo == NULL)
			{
			  DPRINT("RtlAllocateHeap() failed\n");
			  NtClose(PeripheralInstanceKey);
			  NtClose(PeripheralKey);
			  NtClose(ControllerInstanceKey);
			  NtClose(ControllerKey);
			  NtClose(BusInstanceKey);
			  NtClose(BusKey);
			  return FALSE;
			}

		      Status = NtQueryValueKey(PeripheralInstanceKey,
					       &KeyName,
					       KeyValuePartialInformation,
					       ValueInfo,
					       BufferLength,
					       &ReturnedLength);
		      if (NT_SUCCESS(Status))
			{
			  DPRINT("Identifier: %S\n", (PWSTR)ValueInfo->Data);

			  BufferLength = min(ValueInfo->DataLength / sizeof(WCHAR), IdentifierLength);
			  RtlCopyMemory (Identifier,
					 ValueInfo->Data,
					 BufferLength * sizeof(WCHAR));
			  Identifier[BufferLength] = 0;

			  RtlFreeHeap(RtlGetProcessHeap(),
				      0,
				      ValueInfo);
			  NtClose(PeripheralInstanceKey);
			  NtClose(PeripheralKey);
			  NtClose(ControllerInstanceKey);
			  NtClose(ControllerKey);
			  NtClose(BusInstanceKey);
			  NtClose(BusKey);
			  return TRUE;
			}

		      RtlFreeHeap(RtlGetProcessHeap(),
				  0,
				  ValueInfo);

		      NtClose(PeripheralInstanceKey);

		      PeripheralInstance++;
		    }

		  NtClose(PeripheralKey);
		}

	      NtClose(ControllerInstanceKey);

	      ControllerInstance++;
	    }

	  NtClose(ControllerKey);
	}

      NtClose(BusInstanceKey);

      BusInstance++;
    }

  NtClose(BusKey);

  return FALSE;
}


PGENERIC_LIST
CreateMouseDriverList(HINF InfFile)
{
  CHAR Buffer[128];
  PGENERIC_LIST List;
  INFCONTEXT Context;
  PWCHAR KeyName;
  PWCHAR KeyValue;
  PWCHAR UserData;
  WCHAR MouseIdentifier[128];
  WCHAR MouseKey[32];

  /* Get the mouse identification */
  if (!GetMouseIdentifier(L"SerialController", MouseIdentifier, 128))
    {
      if (!GetMouseIdentifier(L"PointerController", MouseIdentifier, 128))
	{
	  wcscpy (MouseIdentifier, L"NO MOUSE");
	}
    }

  DPRINT("Mouse identifier: '%S'\n", MouseIdentifier);

  /* Search for matching device identifier */
  if (!InfFindFirstLine(InfFile, L"Map.Mouse", NULL, &Context))
    {
      /* FIXME: error message */
      return NULL;
    }

  do
    {
      if (!InfGetDataField(&Context, 1, &KeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT("InfGetDataField() failed\n");
	  return NULL;
	}

      DPRINT("KeyValue: %S\n", KeyValue);
      if (wcsstr(MouseIdentifier, KeyValue))
	{
	  if (!InfGetDataField(&Context, 0, &KeyName))
	    {
	      /* FIXME: Handle error! */
	      DPRINT("InfGetDataField() failed\n");
	      return NULL;
	    }

	  DPRINT("Mouse key: %S\n", KeyName);
	  wcscpy(MouseKey, KeyName);
	}
    }
  while (InfFindNextLine(&Context, &Context));


  List = CreateGenericList();
  if (List == NULL)
    return NULL;

  if (!InfFindFirstLine(InfFile, L"Mouse", NULL, &Context))
    {
      DestroyGenericList(List, FALSE);
      return NULL;
    }

  do
    {
      if (!InfGetDataField(&Context, 0, &KeyName))
	{
	  DPRINT1("InfGetDataField() failed\n");
	  break;
	}

      if (!InfGetDataField(&Context, 1, &KeyValue))
	{
	  DPRINT1("InfGetDataField() failed\n");
	  break;
	}

      UserData = RtlAllocateHeap(ProcessHeap,
				 0,
				 (wcslen(KeyName) + 1) * sizeof(WCHAR));
      if (UserData == NULL)
	{
	  DPRINT1("RtlAllocateHeap() failed\n");
	  DestroyGenericList(List, TRUE);
	  return NULL;
	}

      wcscpy(UserData, KeyName);

      sprintf(Buffer, "%S", KeyValue);
      AppendGenericListEntry(List,
			     Buffer,
			     UserData,
			     _wcsicmp(KeyName, MouseKey) ? FALSE : TRUE);
    }
  while (InfFindNextLine(&Context, &Context));

  return List;
}


BOOLEAN
ProcessMouseRegistry(HINF InfFile, PGENERIC_LIST List)
{
  PGENERIC_LIST_ENTRY Entry;
  INFCONTEXT Context;
  PWCHAR ServiceName;
  ULONG StartValue;
  NTSTATUS Status;

  DPRINT("ProcessMouseRegistry() called\n");

  Entry = GetGenericListEntry(List);
  if (Entry == NULL)
    {
      DPRINT("GetGenericListEntry() failed\n");
      return FALSE;
    }

  if (!InfFindFirstLine(InfFile, L"Mouse", Entry->UserData, &Context))
    {
      DPRINT("InfFindFirstLine() failed\n");
      return FALSE;
    }

  if (!InfGetDataField(&Context, 3, &ServiceName))
    {
      DPRINT("InfGetDataField() failed\n");
      return FALSE;
    }

  DPRINT("Service name: %S\n", ServiceName);

  StartValue = 1;
  Status = RtlWriteRegistryValue(RTL_REGISTRY_SERVICES,
				 ServiceName,
				 L"Start",
				 REG_DWORD,
				 &StartValue,
				 sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RtlWriteRegistryValue() failed (Status %lx)\n", Status);
      return FALSE;
    }

  DPRINT("ProcessMouseRegistry() done\n");

  return TRUE;
}


#if 0
BOOLEAN
ProcessMouseFiles(PGENERIC_LIST List)
{
  return TRUE;
}
#endif

/* EOF */
