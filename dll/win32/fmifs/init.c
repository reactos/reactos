/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/init.c
 * PURPOSE:         Initialisation
 *
 * PROGRAMMERS:     Emanuele Aliberti
 *                  Hervé Poussineau (hpoussin@reactos.org)
 */

#include "precomp.h"

static BOOLEAN FmIfsInitialized = FALSE;
LIST_ENTRY ProviderListHead;

PIFS_PROVIDER
GetProvider(
	IN PWCHAR FileSystem)
{
	PLIST_ENTRY ListEntry;
	PIFS_PROVIDER Provider;

	ListEntry = ProviderListHead.Flink;
	while (ListEntry != &ProviderListHead)
	{
		Provider = CONTAINING_RECORD(ListEntry, IFS_PROVIDER, ListEntry);
		if (_wcsicmp(Provider->Name, FileSystem) == 0)
			return Provider;
		ListEntry = ListEntry->Flink;
	}

	/* Provider not found */
	return NULL;
}

static BOOLEAN
AddProvider(
	IN PCUNICODE_STRING FileSystem,
	IN PWCHAR DllFile)
{
	PIFS_PROVIDER Provider = NULL;
	ULONG RequiredSize;
	HMODULE hMod = NULL;
	BOOLEAN ret = FALSE;

	hMod = LoadLibraryW(DllFile);
	if (!hMod)
		goto cleanup;

	RequiredSize = FIELD_OFFSET(IFS_PROVIDER, Name)
		+ FileSystem->Length + sizeof(UNICODE_NULL);
	Provider = (PIFS_PROVIDER)RtlAllocateHeap(
		RtlGetProcessHeap(),
		0,
		RequiredSize);
	if (!Provider)
		goto cleanup;
	RtlZeroMemory(Provider, RequiredSize);

	/* Get function pointers */
	Provider->ChkdskEx = (CHKDSKEX)GetProcAddress(hMod, "ChkdskEx");
	//Provider->Extend = (EXTEND)GetProcAddress(hMod, "Extend");
	Provider->FormatEx = (FORMATEX)GetProcAddress(hMod, "FormatEx");

	RtlCopyMemory(Provider->Name, FileSystem->Buffer, FileSystem->Length);

	InsertTailList(&ProviderListHead, &Provider->ListEntry);
	ret = TRUE;

cleanup:
	if (!ret)
	{
		if (hMod)
			FreeLibrary(hMod);
		if (Provider)
			RtlFreeHeap(RtlGetProcessHeap(), 0, Provider);
	}
	return ret;
}

static BOOLEAN
InitializeFmIfsOnce(void)
{
	OBJECT_ATTRIBUTES ObjectAttributes;
	UNICODE_STRING RegistryPath
		= RTL_CONSTANT_STRING(L"\\REGISTRY\\Machine\\SOFTWARE\\ReactOS\\ReactOS\\CurrentVersion\\IFS");
	HANDLE hKey = NULL;
	PKEY_VALUE_FULL_INFORMATION Buffer;
	ULONG BufferSize = sizeof(KEY_VALUE_FULL_INFORMATION) + MAX_PATH;
	ULONG RequiredSize;
	ULONG i = 0;
	UNICODE_STRING Name;
	UNICODE_STRING Data;
	NTSTATUS Status;

	InitializeListHead(&ProviderListHead);

	/* Read IFS providers from HKLM\SOFTWARE\ReactOS\ReactOS\CurrentVersion\IFS */
	InitializeObjectAttributes(&ObjectAttributes, &RegistryPath, 0, NULL, NULL);
	Status = NtOpenKey(&hKey, KEY_QUERY_VALUE, &ObjectAttributes);
	if (Status == STATUS_OBJECT_NAME_NOT_FOUND)
		return TRUE;
	else if (!NT_SUCCESS(Status))
		return FALSE;

	Buffer = (PKEY_VALUE_FULL_INFORMATION)RtlAllocateHeap(
		RtlGetProcessHeap(),
		0,
		BufferSize);
	if (!Buffer)
	{
		NtClose(hKey);
		return FALSE;
	}
	
	while (TRUE)
	{
		Status = NtEnumerateValueKey(
			hKey,
			i++,
			KeyValueFullInformation,
			Buffer,
			BufferSize,
			&RequiredSize);
		if (Status == STATUS_BUFFER_OVERFLOW)
			continue;
		else if (!NT_SUCCESS(Status))
			break;
		else if (Buffer->Type != REG_SZ)
			continue;

		Name.Length = Name.MaximumLength = Buffer->NameLength;
		Name.Buffer = Buffer->Name;
		Data.Length = Data.MaximumLength = Buffer->DataLength;
		Data.Buffer = (PWCHAR)((ULONG_PTR)Buffer + Buffer->DataOffset);
		if (Data.Length > sizeof(WCHAR) && Data.Buffer[Data.Length / sizeof(WCHAR) - 1] == UNICODE_NULL)
			Data.Length -= sizeof(WCHAR);

		AddProvider(&Name, Data.Buffer);
	}

	NtClose(hKey);
	RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
	return TRUE;
}

/* FMIFS.8 */
BOOLEAN NTAPI
InitializeFmIfs(
	IN PVOID hinstDll,
	IN DWORD dwReason,
	IN PVOID reserved)
{
	switch (dwReason)
	{
		case DLL_PROCESS_ATTACH:
			if (FALSE == FmIfsInitialized)
			{
				if (FALSE == InitializeFmIfsOnce())
				{
						return FALSE;
				}

				FmIfsInitialized = TRUE;
			}
			break;

		case DLL_THREAD_ATTACH:
			break;

		case DLL_THREAD_DETACH:
			break;

		case DLL_PROCESS_DETACH:
			break;
	}

	return TRUE;
}

/* EOF */
