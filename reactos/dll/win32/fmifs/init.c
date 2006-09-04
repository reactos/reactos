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
	while (ListEntry != ProviderListHead.Flink)
	{
		Provider = CONTAINING_RECORD(ListEntry, IFS_PROVIDER, ListEntry);
		if (wcscmp(Provider->Name, FileSystem) == 0)
			return Provider;
		ListEntry = ListEntry->Flink;
	}

	/* Provider not found */
	return NULL;
}

static BOOLEAN
AddProvider(
	IN PWCHAR FileSystem,
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
		+ wcslen(FileSystem) * sizeof(WCHAR) + sizeof(UNICODE_NULL);
	Provider = (PIFS_PROVIDER)RtlAllocateHeap(
		RtlGetProcessHeap(),
		0,
		RequiredSize);
	if (!Provider)
		goto cleanup;
	RtlZeroMemory(Provider, RequiredSize);

	/* Get function pointers */
	//Provider->Chkdsk = (CHKDSK)GetProcAddress(hMod, "Chkdsk");
	//Provider->ChkdskEx = (CHKDSKEX)GetProcAddress(hMod, "ChkdskEx");
	//Provider->Extend = (EXTEND)GetProcAddress(hMod, "Extend");
	//Provider->Format = (FORMAT)GetProcAddress(hMod, "Format");
	Provider->FormatEx = (FORMATEX)GetProcAddress(hMod, "FormatEx");
	//Provider->Recover = (RECOVER)GetProcAddress(hMod, "Recover");

	wcscpy(Provider->Name, FileSystem);

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
	InitializeListHead(&ProviderListHead);

	/* Add default providers */
	AddProvider(L"FAT", L"ufat");
	AddProvider(L"FAT32", L"ufat");

	/* TODO: Check how many IFS are installed in the system */
	/* TODO: and register a descriptor for each one */
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
