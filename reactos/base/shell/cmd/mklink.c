/*
 *  MKLINK.C - mklink internal command.
 */

#include <precomp.h>

#ifdef INCLUDE_CMD_MKLINK

/* There is no API for creating junctions, so we must do it the hard way */
static BOOL CreateJunction(LPCTSTR LinkName, LPCTSTR TargetName)
{
	/* Structure for reparse point daya when ReparseTag is one of
	 * the tags defined by Microsoft. Copied from MinGW winnt.h */
	typedef struct _REPARSE_DATA_BUFFER {
		DWORD  ReparseTag;
		WORD   ReparseDataLength;
		WORD   Reserved;
		_ANONYMOUS_UNION union {
			struct {
				WORD   SubstituteNameOffset;
				WORD   SubstituteNameLength;
				WORD   PrintNameOffset;
				WORD   PrintNameLength;
				ULONG  Flags;
				WCHAR PathBuffer[1];
			} SymbolicLinkReparseBuffer;
			struct {
				WORD   SubstituteNameOffset;
				WORD   SubstituteNameLength;
				WORD   PrintNameOffset;
				WORD   PrintNameLength;
				WCHAR PathBuffer[1];
			} MountPointReparseBuffer;
			struct {
				BYTE   DataBuffer[1];
			} GenericReparseBuffer;
		} DUMMYUNIONNAME;
	} REPARSE_DATA_BUFFER, *PREPARSE_DATA_BUFFER;

	HMODULE hNTDLL = GetModuleHandle(_T("NTDLL"));
	BOOLEAN (WINAPI *RtlDosPathNameToNtPathName_U)(PCWSTR, PUNICODE_STRING, PCWSTR *, CURDIR *)
		= (BOOLEAN (WINAPI *)())GetProcAddress(hNTDLL, "RtlDosPathNameToNtPathName_U");
	VOID (WINAPI *RtlFreeUnicodeString)(PUNICODE_STRING)
		= (VOID (WINAPI *)())GetProcAddress(hNTDLL, "RtlFreeUnicodeString");

	TCHAR TargetFullPath[MAX_PATH];
#ifdef UNICODE
#define TargetFullPathW TargetFullPath
#else
	WCHAR TargetFullPathW[MAX_PATH];
#endif
	UNICODE_STRING TargetNTPath;
	HANDLE hJunction;

	/* The data for this kind of reparse point has two strings:
	 * The first ("SubstituteName") is the full target path in NT format,
	 * the second ("PrintName") is the full target path in Win32 format.
	 * Both of these must be wide-character strings. */
	if (!RtlDosPathNameToNtPathName_U ||
	    !RtlFreeUnicodeString ||
	    !GetFullPathName(TargetName, MAX_PATH, TargetFullPath, NULL) ||
#ifndef UNICODE
	    !MultiByteToWideChar(CP_ACP, 0, TargetFullPath, -1, TargetFullPathW, -1) ||
#endif
	    !RtlDosPathNameToNtPathName_U(TargetFullPathW, &TargetNTPath, NULL, NULL))
	{
		return FALSE;
	}

	/* We have both the names we need, so time to create the junction.
	 * Start with an empty directory */
	if (!CreateDirectory(LinkName, NULL))
	{
		RtlFreeUnicodeString(&TargetNTPath);
		return FALSE;
	}

	/* Open the directory we just created */
	hJunction = CreateFile(LinkName, GENERIC_WRITE, 0, NULL,
		OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS, NULL);
	if (hJunction != INVALID_HANDLE_VALUE)
	{
		/* Allocate a buffer large enough to hold both strings, including trailing NULs */
		DWORD TargetLen = wcslen(TargetFullPathW) * sizeof(WCHAR);
		DWORD DataSize = FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer.PathBuffer)
		                 + TargetNTPath.Length + sizeof(WCHAR)
		                 + TargetLen           + sizeof(WCHAR);
		PREPARSE_DATA_BUFFER Data = _alloca(DataSize);

		/* Fill it out and use it to turn the directory into a reparse point */
		Data->ReparseTag = IO_REPARSE_TAG_MOUNT_POINT;
		Data->ReparseDataLength = DataSize - FIELD_OFFSET(REPARSE_DATA_BUFFER, MountPointReparseBuffer);
		Data->Reserved = 0;
		Data->MountPointReparseBuffer.SubstituteNameOffset = 0;
		Data->MountPointReparseBuffer.SubstituteNameLength = TargetNTPath.Length;
		wcscpy(Data->MountPointReparseBuffer.PathBuffer,
		       TargetNTPath.Buffer);
		Data->MountPointReparseBuffer.PrintNameOffset = TargetNTPath.Length + sizeof(WCHAR);
		Data->MountPointReparseBuffer.PrintNameLength = TargetLen;
		wcscpy((WCHAR *)((BYTE *)Data->MountPointReparseBuffer.PathBuffer
		                 + Data->MountPointReparseBuffer.PrintNameOffset),
		       TargetFullPathW);
		if (DeviceIoControl(hJunction, FSCTL_SET_REPARSE_POINT,
		                    Data, DataSize, NULL, 0, &DataSize, NULL))
		{
			/* Success */
			CloseHandle(hJunction);
			RtlFreeUnicodeString(&TargetNTPath);
			return TRUE;
		}
		CloseHandle(hJunction);
	}
	RemoveDirectory(LinkName);
	RtlFreeUnicodeString(&TargetNTPath);
	return FALSE;
}

INT
cmd_mklink(LPTSTR param)
{
	HMODULE hKernel32 = GetModuleHandle(_T("KERNEL32"));
	DWORD Flags = 0;
	enum { SYMBOLIC, HARD, JUNCTION } LinkType = SYMBOLIC;
	INT NumFiles = 0;
	LPTSTR Name[2];
	INT argc, i;
	LPTSTR *arg;

	if (!_tcsncmp(param, _T("/?"), 2))
	{
		ConOutResPuts(STRING_MKLINK_HELP);
		return 0;
	}

	arg = split(param, &argc, FALSE);
	for (i = 0; i < argc; i++)
	{
		if (arg[i][0] == _T('/'))
		{
			if (!_tcsicmp(arg[i], _T("/D")))
				Flags |= 1; /* SYMBOLIC_LINK_FLAG_DIRECTORY */
			else if (!_tcsicmp(arg[i], _T("/H")))
				LinkType = HARD;
			else if (!_tcsicmp(arg[i], _T("/J")))
				LinkType = JUNCTION;
			else
			{
				error_invalid_switch(arg[i][1]);
				freep(arg);
				return 1;
			}
		}
		else
		{
			if (NumFiles == 2)
			{
				error_too_many_parameters(arg[i]);
				freep(arg);
				return 1;
			}
			Name[NumFiles++] = arg[i];
		}
	}
	freep(arg);

	if (NumFiles != 2)
	{
		error_req_param_missing();
		return 1;
	}

	nErrorLevel = 0;

	if (LinkType == SYMBOLIC)
	{
		/* CreateSymbolicLink doesn't exist in old versions of Windows,
		 * so load dynamically */
		BOOL (WINAPI *CreateSymbolicLink)(LPCTSTR, LPCTSTR, DWORD)
#ifdef UNICODE
			= (BOOL (WINAPI *)())GetProcAddress(hKernel32, "CreateSymbolicLinkW");
#else
			= (BOOL (WINAPI *)())GetProcAddress(hKernel32, "CreateSymbolicLinkA");
#endif
		if (CreateSymbolicLink && CreateSymbolicLink(Name[0], Name[1], Flags))
		{
			ConOutResPrintf(STRING_MKLINK_CREATED_SYMBOLIC, Name[0], Name[1]);
			return 0;
		}
	}
	else if (LinkType == HARD)
	{
		/* CreateHardLink doesn't exist in old versions of Windows,
		 * so load dynamically */
		BOOL (WINAPI *CreateHardLink)(LPCTSTR, LPCTSTR, LPSECURITY_ATTRIBUTES)
#ifdef UNICODE
			= (BOOL (WINAPI *)())GetProcAddress(hKernel32, "CreateHardLinkW");
#else
			= (BOOL (WINAPI *)())GetProcAddress(hKernel32, "CreateHardLinkA");
#endif
		if (CreateHardLink && CreateHardLink(Name[0], Name[1], NULL))
		{
			ConOutResPrintf(STRING_MKLINK_CREATED_HARD, Name[0], Name[1]);
			return 0;
		}
	}
	else
	{
		if (CreateJunction(Name[0], Name[1]))
		{
			ConOutResPrintf(STRING_MKLINK_CREATED_JUNCTION, Name[0], Name[1]);
			return 0;
		}
	}

	ErrorMessage(GetLastError(), _T("MKLINK"));
	return 1;
}

#endif /* INCLUDE_CMD_MKLINK */
