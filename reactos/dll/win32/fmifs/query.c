/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         File Management IFS Utility functions
 * FILE:            reactos/dll/win32/fmifs/query.c
 * PURPOSE:         Query volume information
 *
 * PROGRAMMERS:     Hervé Poussineau (hpoussin@reactos.org)
 */

#include "precomp.h"

BOOLEAN NTAPI
QueryAvailableFileSystemFormat(
	IN DWORD Index,
	IN OUT PWCHAR FileSystem, /* FIXME: Probably one minimal size is mandatory, but which one? */
	OUT UCHAR* Major,
	OUT UCHAR* Minor,
	OUT BOOLEAN* LastestVersion)
{
	PLIST_ENTRY ListEntry;
	PIFS_PROVIDER Provider;

	if (!FileSystem || !Major ||!Minor ||!LastestVersion)
		return FALSE;

	ListEntry = ProviderListHead.Flink;
	while (TRUE)
	{
		if (ListEntry == &ProviderListHead)
			return FALSE;
		if (Index == 0)
			break;
		ListEntry = ListEntry->Flink;
		Index--;
	}

	Provider = CONTAINING_RECORD(ListEntry, IFS_PROVIDER, ListEntry);
	wcscpy(FileSystem, Provider->Name);
	*Major = 0; /* FIXME */
	*Minor = 0; /* FIXME */
	*LastestVersion = TRUE; /* FIXME */

	return TRUE;
}
