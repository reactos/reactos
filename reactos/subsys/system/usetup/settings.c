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
/* $Id: settings.c,v 1.2 2004/06/02 22:18:06 ekohl Exp $
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/settings.c
 * PURPOSE:         Device settings support functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES *****************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>

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


BOOLEAN
ProcessKeyboardLayoutFiles(PGENERIC_LIST List)
{
  return TRUE;
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

  List = CreateGenericList();
  if (List == NULL)
    return NULL;

  if (!InfFindFirstLine (InfFile, L"Mouse", NULL, &Context))
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

/* EOF */
