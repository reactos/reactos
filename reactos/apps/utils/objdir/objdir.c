/* $Id: objdir.c,v 1.11 2003/06/07 16:16:38 chorns Exp $
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
 * 	2001-05-02 (ea)
 * 		Added -r option.
 */

#include <ddk/ntddk.h>
#include <stdarg.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#define MAX_DIR_ENTRY 256


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
PWCHAR
STDCALL
RawAszUsz (
	PCHAR	szA,
	PWCHAR	szW
	)
{
	register PWCHAR w = szW;
	
	while (*szA) {*szW++ = (WCHAR) *szA++;}
	*szW = L'\0';
	return w;
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
		case STATUS_OBJECT_PATH_SYNTAX_BAD:
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


BOOL
STDCALL
ListDirectory (
	IN	PUNICODE_STRING	DirectoryNameW,
	IN	BOOL		Recurse
	)
{
	CHAR			DirectoryNameA [MAX_PATH];
	OBJECT_ATTRIBUTES	ObjectAttributes;
	NTSTATUS		Status;
	HANDLE			DirectoryHandle;
	BYTE			DirectoryEntry [MAX_DIR_ENTRY * sizeof(OBJDIR_INFORMATION)];
	POBJDIR_INFORMATION	pDirectoryEntry = (POBJDIR_INFORMATION) DirectoryEntry;
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

	/* Convert to ANSI the directory's name */
	RawUszAsz (DirectoryNameW->Buffer, DirectoryNameA);
	/*
	 * Prepare parameters for next call.
	 */
	InitializeObjectAttributes (
		& ObjectAttributes,
		DirectoryNameW,
		0,
		NULL,
		NULL
		);
	/*
	 * Try opening the directory.
	 */
	Status = NtOpenDirectoryObject (
			& DirectoryHandle,
			DIRECTORY_QUERY,
			& ObjectAttributes
			);
	if (!NT_SUCCESS(Status))
	{
		printf (
			"Failed to open directory object \"%s\" (Status: %s)\n",
			DirectoryNameA,
			StatusToName (Status)
			);
		return (FALSE);
	}
	printf ("\n Directory of %s\n\n", DirectoryNameA);
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
		if (STATUS_NO_MORE_ENTRIES == Status)
		{
			NtClose (DirectoryHandle);
			return TRUE;
		}
		printf (
			"Failed to query directory object (Status: %s)\n",
			StatusToName (Status)
			);
		NtClose (DirectoryHandle);
		return (FALSE);
	}
	while (0 != pDirectoryEntry->ObjectTypeName.Length)
	{
		CHAR ObjectNameA [MAX_PATH];
		CHAR TypeNameA [MAX_PATH];
		CHAR TargetNameA [MAX_PATH];
		
		if (0 == wcscmp (L"SymbolicLink", pDirectoryEntry->ObjectTypeName.Buffer))
		{
			if (TRUE == ExpandSymbolicLink (
					DirectoryNameW,
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
	/*
	 * Recurse into, if required so.
	 */
	if (FALSE != Recurse)
	{
		pDirectoryEntry = (POBJDIR_INFORMATION) DirectoryEntry;
		while (0 != pDirectoryEntry->ObjectTypeName.Length)
		{
			if (0 == wcscmp (L"Directory", pDirectoryEntry->ObjectTypeName.Buffer))
			{
				WCHAR		CurrentName [MAX_PATH];
				UNICODE_STRING	CurrentDirectory;

				CurrentName [0] = L'\0';
				wcscpy (CurrentName, DirectoryNameW->Buffer);
				if (wcslen (CurrentName) > 1)
				{
					wcscat (CurrentName, L"\\");
				}
				wcscat (CurrentName, pDirectoryEntry->ObjectName.Buffer);
				RtlInitUnicodeString (& CurrentDirectory, CurrentName);
				ListDirectory (& CurrentDirectory, Recurse);
			}
			++ pDirectoryEntry;
		}
	}
	return (TRUE);
}


int main(int argc, char* argv[])
{
	WCHAR		DirectoryNameW [MAX_PATH];
	UNICODE_STRING	DirectoryName;
	BOOL		Recurse = FALSE;

	/*
	 * Check user arguments.
	 */
	switch (argc)
	{
	case 2:
		RawAszUsz (argv[1], DirectoryNameW);
		break;
	case 3:
		if (strcmp (argv[1], "-r"))
		{
			fprintf (
				stderr,
				"%s: unknown option '%s'.\n",
				argv [0], argv[1]
				);
			return EXIT_FAILURE;
		}
		RawAszUsz (argv[2], DirectoryNameW);
		Recurse = TRUE;	
		break;
	default:
		fprintf (
			stderr,
			"\nUsage: %s [-r] directory\n\n"
			"  -r          recurse\n"
			"  directory   a directory name in the system namespace\n\n",
			argv [0]
			);
		return EXIT_FAILURE;
	}
	/*
	 * List the directory.
	 */
	RtlInitUnicodeString (& DirectoryName, DirectoryNameW);
	return (FALSE == ListDirectory (& DirectoryName, Recurse))
		? EXIT_FAILURE
		: EXIT_SUCCESS;
}


/* EOF */
