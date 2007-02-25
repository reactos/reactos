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
/* COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/settings.c
 * PURPOSE:         Device settings support functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>

/* FUNCTIONS ****************************************************************/

static BOOLEAN
GetComputerIdentifier(PWSTR Identifier,
                      ULONG IdentifierLength)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  LPCWSTR ComputerIdentifier;
  HANDLE ProcessorsKey;
  PKEY_FULL_INFORMATION pFullInfo;
  ULONG Size, SizeNeeded;
  NTSTATUS Status;

  DPRINT("GetComputerIdentifier() called\n");

  Size = sizeof(KEY_FULL_INFORMATION);
  pFullInfo = (PKEY_FULL_INFORMATION)RtlAllocateHeap(RtlGetProcessHeap(), 0, Size);
  if (!pFullInfo)
    {
      DPRINT("RtlAllocateHeap() failed\n");
      return FALSE;
    }

  /* Open the processors key */
  RtlInitUnicodeString(&KeyName,
                       L"\\Registry\\Machine\\HARDWARE\\DESCRIPTION\\System\\CentralProcessor");
  InitializeObjectAttributes(&ObjectAttributes,
                             &KeyName,
                             OBJ_CASE_INSENSITIVE,
                             NULL,
                             NULL);
  Status = NtOpenKey(&ProcessorsKey,
                     KEY_QUERY_VALUE ,
                     &ObjectAttributes);
  if (!NT_SUCCESS(Status))
    {
      DPRINT("NtOpenKey() failed (Status 0x%lx)\n", Status);
      RtlFreeHeap(RtlGetProcessHeap(), 0, pFullInfo);
      return FALSE;
    }

  /* Get number of subkeys */
  Status = NtQueryKey(
    ProcessorsKey,
    KeyFullInformation,
    pFullInfo,
    Size,
    &Size);
  NtClose(ProcessorsKey);
  if (!NT_SUCCESS(Status) && Status != STATUS_BUFFER_OVERFLOW)
    {
      DPRINT("NtQueryKey() failed (Status 0x%lx)\n", Status);
      RtlFreeHeap(RtlGetProcessHeap(), 0, pFullInfo);
      return FALSE;
    }

  /* Find computer identifier */
  if (pFullInfo->SubKeys == 0)
    {
      /* Something strange happened. No processor detected */
      RtlFreeHeap(RtlGetProcessHeap(), 0, pFullInfo);
      return FALSE;
    }

  if (pFullInfo->SubKeys == 1)
    {
      /* Computer is mono-CPU */
      ComputerIdentifier = L"PC UP";
    }
  else
    {
      /* Computer is multi-CPUs */
      ComputerIdentifier = L"PC MP";
    }
  RtlFreeHeap(RtlGetProcessHeap(), 0, pFullInfo);

  /* Copy computer identifier to return buffer */
  SizeNeeded = (wcslen(ComputerIdentifier) + 1) * sizeof(WCHAR);
  if (SizeNeeded > IdentifierLength)
    return FALSE;
  RtlCopyMemory(Identifier, ComputerIdentifier, SizeNeeded);
  return TRUE;
}


PGENERIC_LIST
CreateComputerTypeList(HINF InfFile)
{
  CHAR Buffer[128];
  PGENERIC_LIST List;
  INFCONTEXT Context;
  PWCHAR KeyName;
  PWCHAR KeyValue;
  PWCHAR UserData;
  WCHAR ComputerIdentifier[128];
  WCHAR ComputerKey[32];

  /* Get the computer identification */
  if (!GetComputerIdentifier(ComputerIdentifier, 128))
    {
      ComputerIdentifier[0] = 0;
    }

  DPRINT("Computer identifier: '%S'\n", ComputerIdentifier);

  /* Search for matching device identifier */
  if (!SetupFindFirstLineW(InfFile, L"Map.Computer", NULL, &Context))
    {
      /* FIXME: error message */
      return NULL;
    }

  do
    {
      if (!INF_GetDataField(&Context, 1, &KeyValue))
        {
          /* FIXME: Handle error! */
          DPRINT("INF_GetDataField() failed\n");
          return NULL;
        }

      DPRINT("KeyValue: %S\n", KeyValue);
      if (wcsstr(ComputerIdentifier, KeyValue))
        {
          if (!INF_GetDataField(&Context, 0, &KeyName))
            {
              /* FIXME: Handle error! */
              DPRINT("INF_GetDataField() failed\n");
              return NULL;
            }

          DPRINT("Computer key: %S\n", KeyName);
          wcscpy(ComputerKey, KeyName);
        }
    }
  while (SetupFindNextLine(&Context, &Context));

  List = CreateGenericList();
  if (List == NULL)
    return NULL;

  if (!SetupFindFirstLineW (InfFile, L"Computer", NULL, &Context))
    {
      DestroyGenericList(List, FALSE);
      return NULL;
    }

  do
    {
      if (!INF_GetData (&Context, &KeyName, &KeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT("INF_GetData() failed\n");
	  break;
	}

      UserData = (WCHAR*) RtlAllocateHeap(ProcessHeap,
				 0,
				 (wcslen(KeyName) + 1) * sizeof(WCHAR));
      if (UserData == NULL)
	{
	  /* FIXME: Handle error! */
	}

      wcscpy(UserData, KeyName);

      sprintf(Buffer, "%S", KeyValue);
      AppendGenericListEntry(List, Buffer, UserData,
                             _wcsicmp(KeyName, ComputerKey) ? FALSE : TRUE);
    }
  while (SetupFindNextLine(&Context, &Context));

  return List;
}


static BOOLEAN
GetDisplayIdentifier(PWSTR Identifier,
		     ULONG IdentifierLength)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  UNICODE_STRING KeyName;
  WCHAR Buffer[32];
  HANDLE BusKey;
  HANDLE BusInstanceKey;
  HANDLE ControllerKey;
  HANDLE ControllerInstanceKey;
  ULONG BusInstance;
  ULONG ControllerInstance;
  ULONG BufferLength;
  ULONG ReturnedLength;
  PKEY_VALUE_PARTIAL_INFORMATION ValueInfo;
  NTSTATUS Status;

  DPRINT("GetDisplayIdentifier() called\n");

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
			   L"DisplayController");
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

	      /* Get controller identifier */
	      RtlInitUnicodeString(&KeyName,
				   L"Identifier");

	      BufferLength = sizeof(KEY_VALUE_PARTIAL_INFORMATION) +
			     256 * sizeof(WCHAR);
	      ValueInfo = (KEY_VALUE_PARTIAL_INFORMATION*) RtlAllocateHeap(RtlGetProcessHeap(),
					  0,
					  BufferLength);
	      if (ValueInfo == NULL)
		{
		  DPRINT("RtlAllocateHeap() failed\n");
		  NtClose(ControllerInstanceKey);
		  NtClose(ControllerKey);
		  NtClose(BusInstanceKey);
		  NtClose(BusKey);
		  return FALSE;
		}

	      Status = NtQueryValueKey(ControllerInstanceKey,
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
		  NtClose(ControllerInstanceKey);
		  NtClose(ControllerKey);
		  NtClose(BusInstanceKey);
		  NtClose(BusKey);
		  return TRUE;
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
CreateDisplayDriverList(HINF InfFile)
{
  CHAR Buffer[128];
  PGENERIC_LIST List;
  INFCONTEXT Context;
  PWCHAR KeyName;
  PWCHAR KeyValue;
  PWCHAR UserData;
  WCHAR DisplayIdentifier[128];
  WCHAR DisplayKey[32];

  /* Get the display identification */
  if (!GetDisplayIdentifier(DisplayIdentifier, 128))
    {
      DisplayIdentifier[0] = 0;
    }

  DPRINT("Display identifier: '%S'\n", DisplayIdentifier);

  /* Search for matching device identifier */
  if (!SetupFindFirstLineW(InfFile, L"Map.Display", NULL, &Context))
    {
      /* FIXME: error message */
      return NULL;
    }

  do
    {
      if (!INF_GetDataField(&Context, 1, &KeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT("INF_GetDataField() failed\n");
	  return NULL;
	}

      DPRINT("KeyValue: %S\n", KeyValue);
      if (wcsstr(DisplayIdentifier, KeyValue))
	{
	  if (!INF_GetDataField(&Context, 0, &KeyName))
	    {
	      /* FIXME: Handle error! */
	      DPRINT("INF_GetDataField() failed\n");
	      return NULL;
	    }

	  DPRINT("Display key: %S\n", KeyName);
	  wcscpy(DisplayKey, KeyName);
	}
    }
  while (SetupFindNextLine(&Context, &Context));


  List = CreateGenericList();
  if (List == NULL)
    return NULL;

  if (!SetupFindFirstLineW (InfFile, L"Display", NULL, &Context))
    {
      DestroyGenericList(List, FALSE);
      return NULL;
    }

  do
    {
      if (!INF_GetDataField(&Context, 0, &KeyName))
	{
	  DPRINT1("INF_GetDataField() failed\n");
	  break;
	}

      if (!INF_GetDataField(&Context, 1, &KeyValue))
	{
	  DPRINT1("INF_GetDataField() failed\n");
	  break;
	}

      UserData = (WCHAR*) RtlAllocateHeap(ProcessHeap,
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
			     _wcsicmp(KeyName, DisplayKey) ? FALSE : TRUE);
    }
  while (SetupFindNextLine(&Context, &Context));

#if 0
  AppendGenericListEntry(List, "Other display driver", NULL, TRUE);
#endif

  return List;
}

BOOLEAN
ProcessComputerFiles(HINF InfFile, PGENERIC_LIST List, PWCHAR* AdditionalSectionName)
{
	PGENERIC_LIST_ENTRY Entry;
	static WCHAR SectionName[128];

	DPRINT("ProcessComputerFiles() called\n");

	Entry = GetGenericListEntry(List);
	if (Entry == NULL)
	{
		DPRINT("GetGenericListEntry() failed\n");
		return FALSE;
	}

	wcscpy(SectionName, L"Files.");
	wcscat(SectionName, (const wchar_t*) Entry->UserData);
	*AdditionalSectionName = SectionName;

	return TRUE;
}


BOOLEAN
ProcessDisplayRegistry(HINF InfFile, PGENERIC_LIST List)
{
  PGENERIC_LIST_ENTRY Entry;
  INFCONTEXT Context;
  PWCHAR ServiceName;
  ULONG StartValue;
  NTSTATUS Status;
  WCHAR RegPath [255];
  PWCHAR Buffer;
  ULONG Width, Hight, Bpp;

  DPRINT("ProcessDisplayRegistry() called\n");

  Entry = GetGenericListEntry(List);
  if (Entry == NULL)
    {
      DPRINT("GetGenericListEntry() failed\n");
      return FALSE;
    }

  if (!SetupFindFirstLineW(InfFile, L"Display", (WCHAR*) Entry->UserData, &Context))
    {
      DPRINT("SetupFindFirstLineW() failed\n");
      return FALSE;
    }

  /* Enable the right driver */
  if (!INF_GetDataField(&Context, 3, &ServiceName))
    {
      DPRINT("INF_GetDataField() failed\n");
      return FALSE;
    }

  ASSERT(wcslen(ServiceName) < 10);
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

  /* Set the resolution */
  swprintf(RegPath, L"\\Registry\\Machine\\System\\CurrentControlSet\\Hardware Profiles\\Current\\System\\CurrentControlSet\\Services\\%s\\Device0", ServiceName);

  if (!INF_GetDataField(&Context, 4, &Buffer))
    {
      DPRINT("INF_GetDataField() failed\n");
      return FALSE;
    }
  Width = wcstoul(Buffer, NULL, 10);
  Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
				 RegPath,
				 L"DefaultSettings.XResolution",
				 REG_DWORD,
				 &Width,
				 sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RtlWriteRegistryValue() failed (Status %lx)\n", Status);
      return FALSE;
    }


  if (!INF_GetDataField(&Context, 5, &Buffer))
    {
      DPRINT("INF_GetDataField() failed\n");
      return FALSE;
    }
  Hight = wcstoul(Buffer, 0, 0);
  Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
				 RegPath,
				 L"DefaultSettings.YResolution",
				 REG_DWORD,
				 &Hight,
				 sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RtlWriteRegistryValue() failed (Status %lx)\n", Status);
      return FALSE;
    }

  if (!INF_GetDataField(&Context, 6, &Buffer))
    {
      DPRINT("INF_GetDataField() failed\n");
      return FALSE;
    }
  Bpp = wcstoul(Buffer, 0, 0);
  Status = RtlWriteRegistryValue(RTL_REGISTRY_ABSOLUTE,
				 RegPath,
				 L"DefaultSettings.BitsPerPel",
				 REG_DWORD,
				 &Bpp,
				 sizeof(ULONG));
  if (!NT_SUCCESS(Status))
    {
      DPRINT("RtlWriteRegistryValue() failed (Status %lx)\n", Status);
      return FALSE;
    }

  DPRINT("ProcessDisplayRegistry() done\n");

  return TRUE;
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

  if (!SetupFindFirstLineW (InfFile, L"Keyboard", NULL, &Context))
    {
      DestroyGenericList(List, FALSE);
      return NULL;
    }

  do
    {
      if (!INF_GetData (&Context, &KeyName, &KeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT("INF_GetData() failed\n");
	  break;
	}

      UserData = (WCHAR*) RtlAllocateHeap(ProcessHeap,
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
  while (SetupFindNextLine(&Context, &Context));

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
  if (!SetupFindFirstLineW (InfFile, L"NLS", L"DefaultLayout", &Context))
    return NULL;

  if (!INF_GetData (&Context, NULL, &KeyValue))
    return NULL;

  wcscpy(DefaultLayout, KeyValue);

  List = CreateGenericList();
  if (List == NULL)
    return NULL;

  if (!SetupFindFirstLineW (InfFile, L"KeyboardLayout", NULL, &Context))
    {
      DestroyGenericList(List, FALSE);
      return NULL;
    }

  do
    {
      if (!INF_GetData (&Context, &KeyName, &KeyValue))
	{
	  /* FIXME: Handle error! */
	  DPRINT("INF_GetData() failed\n");
	  break;
	}

      UserData = (WCHAR*) RtlAllocateHeap(ProcessHeap,
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
  while (SetupFindNextLine(&Context, &Context));

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

  return TRUE;
}


#if 0
BOOLEAN
ProcessKeyboardLayoutFiles(PGENERIC_LIST List)
{
  return TRUE;
}
#endif

/* EOF */
