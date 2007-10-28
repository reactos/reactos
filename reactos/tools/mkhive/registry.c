/*
 *  ReactOS kernel
 *  Copyright (C) 2006 ReactOS Team
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
 * PROJECT:         ReactOS hive maker
 * FILE:            tools/mkhive/registry.c
 * PURPOSE:         Registry code
 * PROGRAMMER:      Hervé Poussineau
 */

/*
 * TODO:
 *	- Implement RegDeleteKey()
 *	- Implement RegEnumValue()
 *	- Implement RegQueryValueExA()
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define NDEBUG
#include "mkhive.h"

#define REG_DATA_SIZE_MASK                 0x7FFFFFFF
#define REG_DATA_IN_OFFSET                 0x80000000

static CMHIVE RootHive;
static MEMKEY RootKey;
CMHIVE DefaultHive;  /* \Registry\User\.DEFAULT */
CMHIVE SamHive;      /* \Registry\Machine\SAM */
CMHIVE SecurityHive; /* \Registry\Machine\SECURITY */
CMHIVE SoftwareHive; /* \Registry\Machine\SOFTWARE */
CMHIVE SystemHive;   /* \Registry\Machine\SYSTEM */

static MEMKEY
CreateInMemoryStructure(
	IN PCMHIVE RegistryHive,
	IN HCELL_INDEX KeyCellOffset,
	IN PCUNICODE_STRING KeyName)
{
	MEMKEY Key;

	Key = (MEMKEY) malloc (sizeof(KEY));
	if (!Key)
		return NULL;

	InitializeListHead (&Key->SubKeyList);
	InitializeListHead (&Key->ValueList);
	InitializeListHead (&Key->KeyList);

	Key->SubKeyCount = 0;
	Key->ValueCount = 0;

	Key->NameSize = KeyName->Length;
	Key->Name = malloc (Key->NameSize);
	if (!Key->Name)
		return NULL;
	memcpy(Key->Name, KeyName->Buffer, KeyName->Length);

	Key->DataType = 0;
	Key->DataSize = 0;
	Key->Data = NULL;

	Key->RegistryHive = RegistryHive;
	Key->KeyCellOffset = KeyCellOffset;
	Key->KeyCell = (PCM_KEY_NODE)HvGetCell (&RegistryHive->Hive, Key->KeyCellOffset);
	if (!Key->KeyCell)
	{
		free(Key);
		return NULL;
	}
	Key->LinkedKey = NULL;
	return Key;
}

static LONG
RegpOpenOrCreateKey(
	IN HKEY hParentKey,
	IN PCWSTR KeyName,
	IN BOOL AllowCreation,
	OUT PHKEY Key)
{
	PWSTR LocalKeyName;
	PWSTR End;
	UNICODE_STRING KeyString;
	NTSTATUS Status;
	MEMKEY ParentKey;
	MEMKEY CurrentKey;
	PLIST_ENTRY Ptr;
	PCM_KEY_NODE SubKeyCell;
	HCELL_INDEX BlockOffset;

	DPRINT("RegpCreateOpenKey('%S')\n", KeyName);

	if (*KeyName == L'\\')
	{
		KeyName++;
		ParentKey = RootKey;
	}
	else if (hParentKey == NULL)
	{
		ParentKey = RootKey;
	}
	else
	{
		ParentKey = HKEY_TO_MEMKEY(RootKey);
	}

	LocalKeyName = (PWSTR)KeyName;
	for (;;)
	{
		End = (PWSTR) xwcschr(LocalKeyName, '\\');
		if (End)
		{
			KeyString.Buffer = LocalKeyName;
			KeyString.Length = KeyString.MaximumLength =
				(USHORT)((ULONG_PTR)End - (ULONG_PTR)LocalKeyName);
		}
		else
			RtlInitUnicodeString(&KeyString, LocalKeyName);

		/* Redirect from 'CurrentControlSet' to 'ControlSet001' */
		if (!memcmp(LocalKeyName, L"CurrentControlSet", 34) &&
                    ParentKey->NameSize == 12 &&
                    !memcmp(ParentKey->Name, L"SYSTEM", 12))
			RtlInitUnicodeString(&KeyString, L"ControlSet001");

		/* Check subkey in memory structure */
		Ptr = ParentKey->SubKeyList.Flink;
		while (Ptr != &ParentKey->SubKeyList)
		{
			CurrentKey = CONTAINING_RECORD(Ptr, KEY, KeyList);
			if (CurrentKey->NameSize == KeyString.Length
			 && memcmp(CurrentKey->Name, KeyString.Buffer, KeyString.Length) == 0)
			{
				goto nextsubkey;
			}

			Ptr = Ptr->Flink;
		}

		Status = CmiScanForSubKey(
			ParentKey->RegistryHive,
			ParentKey->KeyCell,
			&KeyString,
			OBJ_CASE_INSENSITIVE,
			&SubKeyCell,
			&BlockOffset);
		if (AllowCreation && Status == STATUS_OBJECT_NAME_NOT_FOUND)
		{
			Status = CmiAddSubKey(
				ParentKey->RegistryHive,
				ParentKey->KeyCell,
				ParentKey->KeyCellOffset,
				&KeyString,
				0,
				&SubKeyCell,
				&BlockOffset);
		}
		if (!NT_SUCCESS(Status))
			return ERROR_UNSUCCESSFUL;

		/* Now, SubKeyCell/BlockOffset are valid */
		CurrentKey = CreateInMemoryStructure(
			ParentKey->RegistryHive,
			BlockOffset,
			&KeyString);
		if (!CurrentKey)
			return ERROR_OUTOFMEMORY;

		/* Add CurrentKey in ParentKey */
		InsertTailList(&ParentKey->SubKeyList, &CurrentKey->KeyList);
		ParentKey->SubKeyCount++;

nextsubkey:
		ParentKey = CurrentKey;
		if (End)
			LocalKeyName = End + 1;
		else
			break;
	}

	*Key = MEMKEY_TO_HKEY(ParentKey);

	return ERROR_SUCCESS;
}

LONG WINAPI
RegCreateKeyW(
	IN HKEY hKey,
	IN LPCWSTR lpSubKey,
	OUT PHKEY phkResult)
{
	return RegpOpenOrCreateKey(hKey, lpSubKey, TRUE, phkResult);
}

static PWSTR
MultiByteToWideChar(
	IN PCSTR MultiByteString)
{
	ANSI_STRING Source;
	UNICODE_STRING Destination;
	NTSTATUS Status;

	RtlInitAnsiString(&Source, MultiByteString);
	Status = RtlAnsiStringToUnicodeString(&Destination, &Source, TRUE);
	if (!NT_SUCCESS(Status))
		return NULL;
	return Destination.Buffer;
}

LONG WINAPI
RegCreateKeyA(
	IN HKEY hKey,
	IN LPCSTR lpSubKey,
	OUT PHKEY phkResult)
{
	PWSTR lpSubKeyW;
	LONG rc;

	lpSubKeyW = MultiByteToWideChar(lpSubKey);
	if (!lpSubKeyW)
		return ERROR_OUTOFMEMORY;

	rc = RegCreateKeyW(hKey, lpSubKeyW, phkResult);
	free(lpSubKeyW);
	return rc;
}

LONG WINAPI
RegDeleteKeyA(HKEY Key,
	     LPCSTR Name)
{
  if (Name != NULL && strchr(Name, '\\') != NULL)
    return(ERROR_INVALID_PARAMETER);

  DPRINT1("FIXME!\n");

  return(ERROR_SUCCESS);
}

LONG WINAPI
RegOpenKeyW(
	IN HKEY hKey,
	IN LPCWSTR lpSubKey,
	OUT PHKEY phkResult)
{
	return RegpOpenOrCreateKey(hKey, lpSubKey, FALSE, phkResult);
}

LONG WINAPI
RegOpenKeyA(
	IN HKEY hKey,
	IN LPCSTR lpSubKey,
	OUT PHKEY phkResult)
{
	PWSTR lpSubKeyW;
	LONG rc;

	lpSubKeyW = MultiByteToWideChar(lpSubKey);
	if (!lpSubKeyW)
		return ERROR_OUTOFMEMORY;

	rc = RegOpenKeyW(hKey, lpSubKeyW, phkResult);
	free(lpSubKeyW);
	return rc;
}

static LONG
RegpOpenOrCreateValue(
	IN HKEY hKey,
	IN LPCWSTR ValueName,
	IN BOOL AllowCreation,
	OUT PCM_KEY_VALUE *ValueCell,
	OUT PHCELL_INDEX ValueCellOffset)
{
	MEMKEY ParentKey;
	UNICODE_STRING ValueString;
	NTSTATUS Status;

	ParentKey = HKEY_TO_MEMKEY(hKey);
	RtlInitUnicodeString(&ValueString, ValueName);

	Status = CmiScanForValueKey(
		ParentKey->RegistryHive,
		ParentKey->KeyCell,
		&ValueString,
		ValueCell,
		ValueCellOffset);
	if (AllowCreation && Status == STATUS_OBJECT_NAME_NOT_FOUND)
	{
		Status = CmiAddValueKey(
			ParentKey->RegistryHive,
			ParentKey->KeyCell,
			ParentKey->KeyCellOffset,
			&ValueString,
			ValueCell,
			ValueCellOffset);
	}
	if (!NT_SUCCESS(Status))
		return ERROR_UNSUCCESSFUL;
	return ERROR_SUCCESS;
}

LONG WINAPI
RegSetValueExW(
	IN HKEY hKey,
	IN LPCWSTR lpValueName OPTIONAL,
	IN ULONG Reserved,
	IN ULONG dwType,
	IN const UCHAR* lpData,
	IN USHORT cbData)
{
	MEMKEY Key, DestKey;
	PHKEY phKey;
	PCM_KEY_VALUE ValueCell;
	HCELL_INDEX ValueCellOffset;
	PVOID DataCell;
	LONG DataCellSize;
	NTSTATUS Status;

	if (dwType == REG_LINK)
	{
		/* Special handling of registry links */
		if (cbData != sizeof(PVOID))
			return STATUS_INVALID_PARAMETER;
		phKey = (PHKEY)lpData;
		Key = HKEY_TO_MEMKEY(hKey);
		DestKey = HKEY_TO_MEMKEY(*phKey);

		/* Create the link in memory */
		Key->DataType = REG_LINK;
		Key->LinkedKey = DestKey;

		/* Create the link in registry hive (if applicable) */
		if (Key->RegistryHive != DestKey->RegistryHive)
			return STATUS_SUCCESS;
		DPRINT1("Save link to registry\n");
		return STATUS_NOT_IMPLEMENTED;
	}

	if ((cbData & REG_DATA_SIZE_MASK) != cbData)
		return STATUS_UNSUCCESSFUL;

	Key = HKEY_TO_MEMKEY(hKey);

	Status = RegpOpenOrCreateValue(hKey, lpValueName, TRUE, &ValueCell, &ValueCellOffset);
	if (!NT_SUCCESS(Status))
		return ERROR_UNSUCCESSFUL;

	/* Get size of the allocated cellule (if any) */
	if (!(ValueCell->DataLength & REG_DATA_IN_OFFSET) &&
		(ValueCell->DataLength & REG_DATA_SIZE_MASK) != 0)
	{
		DataCell = HvGetCell(&Key->RegistryHive->Hive, ValueCell->Data);
		if (!DataCell)
			return ERROR_UNSUCCESSFUL;
		DataCellSize = -HvGetCellSize(&Key->RegistryHive->Hive, DataCell);
	}
	else
	{
		DataCell = NULL;
		DataCellSize = 0;
	}

	if (cbData <= sizeof(HCELL_INDEX))
	{
		/* If data size <= sizeof(HCELL_INDEX) then store data in the data offset */
		DPRINT("ValueCell->DataLength %lu\n", ValueCell->DataLength);
		if (DataCell)
			HvFreeCell(&Key->RegistryHive->Hive, ValueCell->Data);

		RtlCopyMemory(&ValueCell->Data, lpData, cbData);
		ValueCell->DataLength = (ULONG)(cbData | REG_DATA_IN_OFFSET);
		ValueCell->Type = dwType;
		HvMarkCellDirty(&Key->RegistryHive->Hive, ValueCellOffset, FALSE);
	}
	else
	{
		if (cbData > (SIZE_T)DataCellSize)
		{
			/* New data size is larger than the current, destroy current
			 * data block and allocate a new one. */
			HCELL_INDEX NewOffset;

			DPRINT("ValueCell->DataLength %lu\n", ValueCell->DataLength);

			NewOffset = HvAllocateCell(&Key->RegistryHive->Hive, cbData, Stable, HCELL_NIL);
			if (NewOffset == HCELL_NIL)
			{
				DPRINT("HvAllocateCell() failed with status 0x%08lx\n", Status);
				return ERROR_UNSUCCESSFUL;
			}

			if (DataCell)
				HvFreeCell(&Key->RegistryHive->Hive, ValueCell->Data);

			ValueCell->Data = NewOffset;
			DataCell = (PVOID)HvGetCell(&Key->RegistryHive->Hive, NewOffset);
		}

		/* Copy new contents to cellule */
		RtlCopyMemory(DataCell, lpData, cbData);
		ValueCell->DataLength = (ULONG)(cbData & REG_DATA_SIZE_MASK);
		ValueCell->Type = dwType;
		HvMarkCellDirty(&Key->RegistryHive->Hive, ValueCell->Data, FALSE);
		HvMarkCellDirty(&Key->RegistryHive->Hive, ValueCellOffset, FALSE);
	}

	HvMarkCellDirty(&Key->RegistryHive->Hive, Key->KeyCellOffset, FALSE);

	DPRINT("Return status 0x%08lx\n", Status);
	return Status;
}

LONG WINAPI
RegSetValueExA(
	IN HKEY hKey,
	IN LPCSTR lpValueName OPTIONAL,
	IN ULONG Reserved,
	IN ULONG dwType,
	IN const UCHAR* lpData,
	IN ULONG cbData)
{
	LPWSTR lpValueNameW = NULL;
	const UCHAR* lpDataW;
	USHORT cbDataW;
	LONG rc = ERROR_SUCCESS;

	DPRINT("RegSetValueA(%s)\n", lpValueName);
	if (lpValueName)
	{
		lpValueNameW = MultiByteToWideChar(lpValueName);
		if (!lpValueNameW)
			return ERROR_OUTOFMEMORY;
	}

	if ((dwType == REG_SZ || dwType == REG_EXPAND_SZ || dwType == REG_MULTI_SZ)
	 && cbData != 0)
	{
		ANSI_STRING AnsiString;
		UNICODE_STRING Data;

		if (lpData[cbData - 1] != '\0')
			cbData++;
		RtlInitAnsiString(&AnsiString, NULL);
		AnsiString.Buffer = (PSTR)lpData;
		AnsiString.Length = (USHORT)cbData - 1;
		AnsiString.MaximumLength = (USHORT)cbData;
		RtlAnsiStringToUnicodeString (&Data, &AnsiString, TRUE);
		lpDataW = (const UCHAR*)Data.Buffer;
		cbDataW = Data.MaximumLength;
	}
	else
	{
		lpDataW = lpData;
		cbDataW = (USHORT)cbData;
	}

	if (rc == ERROR_SUCCESS)
		rc = RegSetValueExW(hKey, lpValueNameW, 0, dwType, lpDataW, cbDataW);
	if (lpValueNameW)
		free(lpValueNameW);
	if (lpData != lpDataW)
		free((PVOID)lpDataW);
	return rc;
}

LONG WINAPI
RegQueryValueExW(
	IN HKEY hKey,
	IN LPCWSTR lpValueName,
	IN PULONG lpReserved,
	OUT PULONG lpType,
	OUT PUCHAR lpData,
	OUT PSIZE_T lpcbData)
{
	//ParentKey = HKEY_TO_MEMKEY(RootKey);
	PCM_KEY_VALUE ValueCell;
	HCELL_INDEX ValueCellOffset;
	LONG rc;

	rc = RegpOpenOrCreateValue(
			hKey,
			lpValueName,
			FALSE,
			&ValueCell,
			&ValueCellOffset);
	if (rc != ERROR_SUCCESS)
		return rc;

	DPRINT1("RegQueryValueExW(%S) not implemented\n", lpValueName);
	/* ValueCell and ValueCellOffset are valid */

	return ERROR_UNSUCCESSFUL;
}

LONG WINAPI
RegQueryValueExA(
	IN HKEY hKey,
	IN LPCSTR lpValueName,
	IN PULONG lpReserved,
	OUT PULONG lpType,
	OUT PUCHAR lpData,
	OUT PSIZE_T lpcbData)
{
	LPWSTR lpValueNameW = NULL;
	LONG rc;

	if (lpValueName)
	{
		lpValueNameW = MultiByteToWideChar(lpValueName);
		if (!lpValueNameW)
			return ERROR_OUTOFMEMORY;
	}

	rc = RegQueryValueExW(hKey, lpValueNameW, lpReserved, lpType, lpData, lpcbData);
	if (lpValueNameW)
		free(lpValueNameW);
	return ERROR_UNSUCCESSFUL;
}

LONG WINAPI
RegDeleteValueW(
	IN HKEY hKey,
	IN LPCWSTR lpValueName OPTIONAL)
{
	DPRINT1("RegDeleteValueW() unimplemented\n");
	return ERROR_UNSUCCESSFUL;
}

LONG WINAPI
RegDeleteValueA(
	IN HKEY hKey,
	IN LPCSTR lpValueName OPTIONAL)
{
	LPWSTR lpValueNameW;
	LONG rc;

	if (lpValueName)
	{
		lpValueNameW = MultiByteToWideChar(lpValueName);
		if (!lpValueNameW)
			return ERROR_OUTOFMEMORY;
		rc = RegDeleteValueW(hKey, lpValueNameW);
		free(lpValueNameW);
	}
	else
		rc = RegDeleteValueW(hKey, NULL);
	return rc;
}

static BOOL
ConnectRegistry(
	IN HKEY RootKey,
	IN PCMHIVE HiveToConnect,
	IN LPCWSTR Path)
{
	NTSTATUS Status;
	MEMKEY NewKey;
	LONG rc;

	Status = CmiInitializeTempHive(HiveToConnect);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("CmiInitializeTempHive() failed with status 0x%08lx\n", Status);
		return FALSE;
	}

	/* Create key */
	rc = RegCreateKeyW(
		RootKey,
		Path,
		(PHKEY)&NewKey);
	if (rc != ERROR_SUCCESS)
		return FALSE;

	NewKey->RegistryHive = HiveToConnect;
	NewKey->KeyCellOffset = HiveToConnect->Hive.BaseBlock->RootCell;
	NewKey->KeyCell = (PCM_KEY_NODE)HvGetCell (&HiveToConnect->Hive, NewKey->KeyCellOffset);
	return TRUE;
}

static BOOL
MyExportBinaryHive (PCHAR FileName,
		  PCMHIVE RootHive)
{
	FILE *File;
	BOOL ret;

	/* Create new hive file */
	File = fopen (FileName, "w+b");
	if (File == NULL)
	{
		printf("    Error creating/opening file\n");
		return FALSE;
	}

	fseek (File, 0, SEEK_SET);

	RootHive->FileHandles[HFILE_TYPE_PRIMARY] = (HANDLE)File;
	ret = HvWriteHive(&RootHive->Hive);
	fclose (File);
	return ret;
}

LIST_ENTRY CmiHiveListHead;

VOID
RegInitializeRegistry(VOID)
{
	UNICODE_STRING RootKeyName = RTL_CONSTANT_STRING(L"\\");
	NTSTATUS Status;
	HKEY ControlSetKey;

	InitializeListHead(&CmiHiveListHead);

	Status = CmiInitializeTempHive(&RootHive);
	if (!NT_SUCCESS(Status))
	{
		DPRINT1("CmiInitializeTempHive() failed with status 0x%08lx\n", Status);
		return;
	}

	RootKey = CreateInMemoryStructure(
		&RootHive,
		RootHive.Hive.BaseBlock->RootCell,
		&RootKeyName);

	/* Create DEFAULT key */
	ConnectRegistry(
		NULL,
		&DefaultHive,
		L"Registry\\User\\.DEFAULT");

	/* Create SAM key */
	ConnectRegistry(
		NULL,
		&SamHive,
		L"Registry\\Machine\\SAM");

	/* Create SECURITY key */
	ConnectRegistry(
		NULL,
		&SecurityHive,
		L"Registry\\Machine\\SECURITY");

	/* Create SOFTWARE key */
	ConnectRegistry(
		NULL,
		&SoftwareHive,
		L"Registry\\Machine\\SOFTWARE");

	/* Create SYSTEM key */
	ConnectRegistry(
		NULL,
		&SystemHive,
		L"Registry\\Machine\\SYSTEM");

	/* Create 'ControlSet001' key */
	RegCreateKeyW(
		NULL,
		L"Registry\\Machine\\SYSTEM\\ControlSet001",
		&ControlSetKey);
}

/* EOF */
