/* $Id: objdir.c,v 1.4 2000/08/08 17:42:33 ekohl Exp $
 *
 * DESCRIPTION: Object Manager Simple Explorer
 * PROGRAMMER:  David Welch
 * REVISIONS
 * 	2000-04-30 (ea)
 * 		Added directory enumeration.
 * 		(tested under nt4sp4/x86)
 */

#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_TYPE 64

typedef
struct _DIRECTORY_ENTRY_HEADER
{
	UNICODE_STRING	Name;
	UNICODE_STRING	Type;
	
} DIRECTORY_ENTRY_HEADER;

struct
{
	DIRECTORY_ENTRY_HEADER	Entry;
	WCHAR			Buffer [MAX_PATH + MAX_TYPE + 2];
	
} DirectoryEntry;


static
const char *
STDCALL
StatusToName (NTSTATUS Status)
{
	static char RawValue [16];
	
	switch (Status)
	{
		case STATUS_OBJECT_NAME_INVALID:
			return "STATUS_OBJECT_NAME_INVALID";
		case STATUS_OBJECT_NAME_NOT_FOUND:
			return "STATUS_OBJECT_NAME_NOT_FOUND";
		case STATUS_PATH_SYNTAX_BAD:
			return "STATUS_PATH_SYNTAX_BAD";
	}
	sprintf (RawValue, "0x%08lx", Status);
	return (const char *) RawValue;
}


int main(int argc, char* argv[])
{
	UNICODE_STRING		DirectoryNameW;
	ANSI_STRING		DirectoryNameA;
	OBJECT_ATTRIBUTES	ObjectAttributes;
	NTSTATUS		Status;
	HANDLE			DirectoryHandle;
	ULONG			BytesReturned = 0;
	ULONG			EntryCount = 0;

	/*
	 * Check user arguments.
	 */
	if (2 != argc)
	{
		fprintf (
			stderr,
			"Usage: %s directory\n\n"
			"  directory   a directory name in the system namespace\n",
			argv [0]
			);
		return EXIT_FAILURE;
	}
	/*
	 * Prepare parameters for next call.
	 */
	RtlInitAnsiString (
		& DirectoryNameA,
		argv [1]
		);
	RtlAnsiStringToUnicodeString (
		& DirectoryNameW,
		& DirectoryNameA,
		TRUE
		);
	InitializeObjectAttributes (
		& ObjectAttributes,
		& DirectoryNameW,
		0,
		NULL,
		NULL
		);
	/*
	 * Try opening the directory the
	 * user requested.
	 */
	Status = NtOpenDirectoryObject (
			& DirectoryHandle,
			DIRECTORY_QUERY,
			& ObjectAttributes
			);
	if (!NT_SUCCESS(Status))
	{
		printf (
			"Failed to open directory object (Status: %s)\n",
			StatusToName (Status)
			);
		return EXIT_FAILURE;
	}
	/*
	 * Enumerate each item in the directory.
	 */
	Status = NtQueryDirectoryObject (
			DirectoryHandle,
			(POBJDIR_INFORMATION)& DirectoryEntry,
			sizeof DirectoryEntry,
			TRUE,
			TRUE,
			& BytesReturned,
			& EntryCount
			);
	if (!NT_SUCCESS(Status))
	{
		printf (
			"Failed to query directory object (Status: %s)\n",
			StatusToName (Status)
			);
		NtClose (DirectoryHandle);
		return EXIT_FAILURE;
	}
	wprintf (L"%d entries:\n", EntryCount);
	while (STATUS_NO_MORE_ENTRIES != Status)
	{
		wprintf (
			L"%-16s %s\n",
			DirectoryEntry.Entry.Type.Buffer,
			DirectoryEntry.Entry.Name.Buffer
			);
		Status = NtQueryDirectoryObject (
				DirectoryHandle,
				(POBJDIR_INFORMATION)& DirectoryEntry,
				sizeof DirectoryEntry,
				TRUE,
				FALSE,
				& BytesReturned,
				& EntryCount
				);
	}
	/*
	 * Free any resource.
	 */
	NtClose (DirectoryHandle);

	return EXIT_SUCCESS;
}


/* EOF */
