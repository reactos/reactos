/* $Id: objdir.c,v 1.6 2001/05/01 21:43:45 ea Exp $
 *
 * DESCRIPTION: Object Manager Simple Explorer
 * PROGRAMMER:  David Welch
 * REVISIONS
 * 	2000-04-30 (ea)
 * 		Added directory enumeration.
 * 		(tested under nt4sp4/x86)
 * 	2000-08-11 (ea)
 * 		Added symbolic link expansion.
 * 		(tested under nt4sp4/x86)
 * 	2001-05-01 (ea)
 * 		Fixed entries counter. Added more
 * 		error codes check. Removed wprintf,
 * 		because it does not work in .17.
 */

#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

BYTE DirectoryEntry [32768];
POBJDIR_INFORMATION pDirectoryEntry = (POBJDIR_INFORMATION) DirectoryEntry;

static
PCHAR
STDCALL
RawUszAsz (
	PWCHAR	szU,
	PCHAR	szA
	)
{
	register PCHAR a = szA;
	
	while (*szU) {*szA++ = (CHAR) (0x00ff & * szU++);}
	*szA = '\0';
	return a;
}


static
const char *
STDCALL
StatusToName (NTSTATUS Status)
{
	static char RawValue [16];
	
	switch (Status)
	{
		case STATUS_BUFFER_TOO_SMALL:
			return "STATUS_BUFFER_TOO_SMALL";
		case STATUS_INVALID_PARAMETER:
			return "STATUS_INVALID_PARAMETER";
		case STATUS_OBJECT_NAME_INVALID:
			return "STATUS_OBJECT_NAME_INVALID";
		case STATUS_OBJECT_NAME_NOT_FOUND:
			return "STATUS_OBJECT_NAME_NOT_FOUND";
		case STATUS_PATH_SYNTAX_BAD:
			return "STATUS_PATH_SYNTAX_BAD";
		case STATUS_NO_MORE_ENTRIES:
			return "STATUS_NO_MORE_ENTRIES";
		case STATUS_UNSUCCESSFUL:
			return "STATUS_UNSUCCESSFUL";
	}
	sprintf (RawValue, "0x%08lx", Status);
	return (const char *) RawValue;
}


BOOL
STDCALL
ExpandSymbolicLink (
	IN	PUNICODE_STRING	DirectoryName,
	IN	PUNICODE_STRING	SymbolicLinkName,
	IN OUT	PUNICODE_STRING	TargetObjectName
	)
{
	NTSTATUS		Status;
	HANDLE			hSymbolicLink;
	OBJECT_ATTRIBUTES	oa;
	UNICODE_STRING		Path;
	WCHAR			PathBuffer [MAX_PATH];
	ULONG			DataWritten = 0;


	Path.Buffer = PathBuffer;
	Path.Length = 0;
	Path.MaximumLength = sizeof PathBuffer;
	
	RtlCopyUnicodeString (& Path, DirectoryName);
	if (L'\\' != Path.Buffer [(Path.Length / sizeof Path.Buffer[0]) - 1])
	{
		RtlAppendUnicodeToString (& Path, L"\\");
	}
	RtlAppendUnicodeStringToString (& Path, SymbolicLinkName);

	oa.Length			= sizeof (OBJECT_ATTRIBUTES);
	oa.ObjectName			= & Path;
	oa.Attributes			= 0; /* OBJ_CASE_INSENSITIVE; */
	oa.RootDirectory		= NULL;
	oa.SecurityDescriptor		= NULL;
	oa.SecurityQualityOfService	= NULL;

	Status = NtOpenSymbolicLinkObject(
			& hSymbolicLink,
			SYMBOLIC_LINK_QUERY,	/* 0x20001 */
			& oa
			);

	if (!NT_SUCCESS(Status))
	{
		printf (
			"Failed to open SymbolicLink object (Status: %s)\n",
			StatusToName (Status)
			);
		return FALSE;
	}
	TargetObjectName->Length = TargetObjectName->MaximumLength;
	memset (
		TargetObjectName->Buffer,
		0,
		TargetObjectName->MaximumLength
		);
	Status = NtQuerySymbolicLinkObject(
			hSymbolicLink,
			TargetObjectName,
			& DataWritten
			);
	if (!NT_SUCCESS(Status))
	{
		printf (
			"Failed to query SymbolicLink object (Status: %s)\n",
			StatusToName (Status)
			);
		NtClose (hSymbolicLink);
		return FALSE;
	}
	NtClose (hSymbolicLink);
	return TRUE;
}
	


int main(int argc, char* argv[])
{
	UNICODE_STRING		DirectoryNameW;
	ANSI_STRING		DirectoryNameA;
	OBJECT_ATTRIBUTES	ObjectAttributes;
	NTSTATUS		Status;
	HANDLE			DirectoryHandle;
	ULONG			Context = 0;
	ULONG			ReturnLength = 0;
	ULONG			EntryCount = 0;
	
	/* For expanding symbolic links */
	WCHAR			TargetName [2 * MAX_PATH];
	UNICODE_STRING		TargetObjectName = {
					sizeof TargetName,
					sizeof TargetName,
					TargetName
				};

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
	printf ("\n Directory of %s\n\n", argv[1]);
	/*
	 * Enumerate each item in the directory.
	 */
	Status = NtQueryDirectoryObject (
			DirectoryHandle,
			pDirectoryEntry,
			sizeof DirectoryEntry,
			FALSE,/* ReturnSingleEntry */
			TRUE, /* RestartScan */
			& Context,
			& ReturnLength
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
	while (0 != pDirectoryEntry->ObjectTypeName.Length)
	{
		CHAR ObjectNameA [MAX_PATH];
		CHAR TypeNameA [MAX_PATH];
		CHAR TargetNameA [MAX_PATH];
		
		if (0 == wcscmp (L"SymbolicLink", pDirectoryEntry->ObjectTypeName.Buffer))
		{
			if (TRUE == ExpandSymbolicLink (
					& DirectoryNameW,
					& pDirectoryEntry->ObjectName,
					& TargetObjectName
					)
				)
			{
				
				printf (
					"%-16s %s -> %s\n",
					RawUszAsz (pDirectoryEntry->ObjectTypeName.Buffer, TypeNameA),
					RawUszAsz (pDirectoryEntry->ObjectName.Buffer, ObjectNameA),
					RawUszAsz (TargetObjectName.Buffer, TargetNameA)
					);
			}
			else
			{
				printf (
					"%-16s %s -> (error!)\n",
					RawUszAsz (pDirectoryEntry->ObjectTypeName.Buffer, TypeNameA),
					RawUszAsz (pDirectoryEntry->ObjectName.Buffer, ObjectNameA)
					);
			}
		}
		else
		{
			printf (
				"%-16s %s\n",
				RawUszAsz (pDirectoryEntry->ObjectTypeName.Buffer, TypeNameA),
				RawUszAsz (pDirectoryEntry->ObjectName.Buffer, ObjectNameA)
				);
		}
		++ EntryCount;
		++ pDirectoryEntry;
		
		
	}
	printf ("\n\t%d object(s)\n", EntryCount);
	/*
	 * Free any resource.
	 */
	NtClose (DirectoryHandle);

	return EXIT_SUCCESS;
}


/* EOF */
