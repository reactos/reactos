/* $Id: path.c,v 1.3 2000/03/13 17:54:23 ekohl Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/path.c
 * PURPOSE:         Path and current directory functions
 * UPDATE HISTORY:
 *                  Created 03/02/00
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ntdll/rtl.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>

#define NDEBUG
#include <ntdll/ntdll.h>


/* DEFINITONS and MACROS ******************************************************/

#define MAX_PFX_SIZE       16

#define IS_PATH_SEPARATOR(x) (((x)==L'\\')||((x)==L'/'))


/* FUNCTIONS *****************************************************************/


static
ULONG
RtlpGetDotSequence (PWSTR p)
{
	ULONG Count = 0;

	for (;;)
	{
		if (*p == '.')
			Count++;
		else if ((*p == '\\' || *p == '\0') && Count)
			return Count;
		else
			return 0;
		p++;
	}
	return 0;
}


static
VOID
RtlpEatPath (
	PWSTR Path
	)
{
	PWSTR p, prev;

	p = Path + 2;
	prev = p;

	while ((*p) != 0 || ((*p) == L'\\' && (*(p+1)) == 0))
	{
		ULONG DotLen;

		DotLen = RtlpGetDotSequence (p+1);
		DPRINT("DotSequenceLength %u\n", DotLen);
		DPRINT("prev %S p %S\n",prev,p);

		if (DotLen == 0)
		{
			prev = p;
			do
			{
				p++;
			}
			while ((*p) != 0 && (*p) != L'\\');
		}
		else if (DotLen == 1)
		{
			wcscpy (p, p+2);
		}
		else
		{
			if (DotLen > 2)
			{
				int n = DotLen - 2;

				while (n > 0 && prev > (Path + 2))
				{
					prev--;
					if ((*prev) == L'\\')
						n--;
				}
			}

			if (*(p + DotLen + 1) == 0)
				*(prev + 1) = 0;
			else
				wcscpy (prev, p + DotLen + 1);
			p = prev;
			if (prev > (Path + 2))
			{
				prev--;
				while ((*prev) != L'\\')
				{
					prev--;
				}
			}
		}
	}
}


ULONG
STDCALL
RtlGetLongestNtPathLength (VOID)
{
	return (MAX_PATH + 9);
}


ULONG
STDCALL
RtlDetermineDosPathNameType_U (
	PWSTR Path
	)
{
	DPRINT ("RtlDetermineDosPathNameType_U %S\n", Path);

	if (Path == NULL)
		return 0;

	if (IS_PATH_SEPARATOR(Path[0]))
	{
		if (!IS_PATH_SEPARATOR(Path[1]))
			return 4;			/* \xxx   */

		if (Path[2] != L'.')
			return 1;			/* \\xxx   */

		if (IS_PATH_SEPARATOR(Path[3]))
			return 6;			/* \\.\xxx */

		if (Path[3])
			return 1;			/* \\.xxxx */

		return 7;				/* \\.     */
	}
	else
	{
		if (Path[1] != L':')
			return 5;			/* xxx     */

		if (IS_PATH_SEPARATOR(Path[2]))
			return 2;			/* x:\xxx  */

		return 3;				/* x:xxx   */
	}
}


/* returns 0 if name is not valid DOS device name, or DWORD with
 * offset in bytes to DOS device name from beginning of buffer in high word
 * and size in bytes of DOS device name in low word */
ULONG
STDCALL
RtlIsDosDeviceName_U (
	PWSTR DeviceName
	)
{
	ULONG Type;
	ULONG Length = 0;
	ULONG Offset;
	PWCHAR wc;

	if (DeviceName == NULL)
		return 0;

	while (DeviceName[Length])
		Length++;

	Type = RtlDetermineDosPathNameType_U (DeviceName);
	if (Type <= 1)
		return 0;
	if (Type == 6)
	{
		if (Length == 7 &&
		    !_wcsnicmp (DeviceName, L"\\\\.\\CON", 7))
			return 0x00080006;
		return 0;
	}

	/* name can end with ':' */
	if (Length && DeviceName[Length - 1 ] == L':')
		Length--;

	/* there can be spaces or points at the end of name */
	wc = DeviceName + Length - 1;
	while (Length && (*wc == L'.' || *wc == L' '))
	{
		Length--;
		wc--;
	}

	/* let's find a beginning of name */
	wc = DeviceName + Length - 1;
	while (wc > DeviceName && !IS_PATH_SEPARATOR(*(wc - 1)))
		wc--;
	Offset = wc - DeviceName;
	Length -= Offset;

	/* check for LPTx or COMx */
	if (Length == 4 && wc[3] >= L'0' && wc[3] <= L'9')
	{
		if (wc[3] == L'0')
			return 0;
		if (!_wcsnicmp (wc, L"LPT", 3) ||
		    !_wcsnicmp (wc, L"COM", 3))
		{
			return ((Offset * 2) << 16 ) | 8;
		}
		return 0;
	}

	/* check for PRN,AUX,NUL or CON */
	if (Length == 3 &&
	    (!_wcsnicmp (wc, L"PRN", 3) ||
	     !_wcsnicmp (wc, L"AUX", 3) ||
	     !_wcsnicmp (wc, L"NUL", 3) ||
	     !_wcsnicmp (wc, L"CON", 3)))
	{
		return ((Offset * 2) << 16) | 6;
	}

	return 0;
}


ULONG
STDCALL
RtlGetCurrentDirectory_U (
	ULONG MaximumLength,
	PWSTR Buffer
	)
{
	ULONG Length;
	PCURDIR cd;

	DPRINT ("RtlGetCurrentDirectory %lu %p\n", MaximumLength, Buffer);

	cd = &(NtCurrentPeb ()->ProcessParameters->CurrentDirectory);

	RtlAcquirePebLock();
	Length = cd->DosPath.Length / sizeof(WCHAR);
	if (cd->DosPath.Buffer[Length - 1] == L'\\' &&
	    cd->DosPath.Buffer[Length - 2] != L':')
		Length--;

	DPRINT ("cd->DosPath.Buffer %S Length %d\n",
	        cd->DosPath.Buffer, Length);

	if (MaximumLength / sizeof(WCHAR) > Length)
	{
		memcpy (Buffer,
		        cd->DosPath.Buffer,
		        Length * sizeof(WCHAR));
		Buffer[Length] = 0;
	}
	else
	{
		Length++;
	}

	RtlReleasePebLock ();

	DPRINT ("CurrentDirectory %S\n", Buffer);

	return (Length * sizeof(WCHAR));
}


NTSTATUS
STDCALL
RtlSetCurrentDirectory_U (
	PUNICODE_STRING name
	)
{
	UNICODE_STRING full;
	OBJECT_ATTRIBUTES Attr;
	IO_STATUS_BLOCK iosb;
	PCURDIR cd;
	NTSTATUS Status;
	ULONG size;
	HANDLE handle = NULL;
	PWSTR wcs;
	PWSTR buf = 0;

	DPRINT ("RtlSetCurrentDirectory %wZ\n", name);

	RtlAcquirePebLock ();
	cd = &(NtCurrentPeb ()->ProcessParameters->CurrentDirectory);
	size = cd->DosPath.MaximumLength;

	buf = RtlAllocateHeap (RtlGetProcessHeap(),
	                       0,
	                       size);
	if (buf == NULL)
	{
		RtlReleasePebLock ();
		return STATUS_NO_MEMORY;
	}

	size = RtlGetFullPathName_U (name->Buffer, size, buf, 0);
	if (!size)
	{
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             buf);
		RtlReleasePebLock ();
		return STATUS_OBJECT_NAME_INVALID;
	}

	if (!RtlDosPathNameToNtPathName_U (buf, &full, 0, 0))
	{
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             buf);
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             full.Buffer);
		RtlReleasePebLock ();
		return STATUS_OBJECT_NAME_INVALID;
	}

	InitializeObjectAttributes (&Attr,
	                            &full,
	                            OBJ_CASE_INSENSITIVE | OBJ_INHERIT,
	                            NULL,
	                            NULL);

	Status = NtOpenFile (&handle,
	                     SYNCHRONIZE | FILE_TRAVERSE,
	                     &Attr,
	                     &iosb,
	                     FILE_SHARE_READ | FILE_SHARE_WRITE,
	                     FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT);
	if (!NT_SUCCESS(Status))
	{
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             buf);
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             full.Buffer);
		RtlReleasePebLock ();
		return Status;
	}

	/* append backslash if missing */
	wcs = buf + size / sizeof(WCHAR) - 1;
	if (*wcs != L'\\')
	{
		*(++wcs) = L'\\';
		*(++wcs) = 0;
		size += sizeof(WCHAR);
	}

	memmove (cd->DosPath.Buffer,
	         buf,
	         size + sizeof(WCHAR));
	cd->DosPath.Length = size;

	if (cd->Handle)
		NtClose (cd->Handle);
	cd->Handle = handle;

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             buf);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             full.Buffer);

	RtlReleasePebLock();

	return STATUS_SUCCESS;
}


ULONG
STDCALL
RtlGetFullPathName_U (
	PWSTR DosName,
	ULONG size,
	PWSTR buf,
	PWSTR *FilePart
	)
{
	WCHAR           *wcs, var[4], drive;
	int             len;
	DWORD           offs, sz, type;
	UNICODE_STRING  usvar, pfx;
	PCURDIR cd;
	NTSTATUS Status;

	DPRINT("RtlGetFullPathName_U %S %ld %p %p\n",
	       DosName, size, buf, FilePart);

	if (!DosName || !*DosName)
		return 0;

	len = wcslen (DosName);

	/* strip trailing spaces */
	while (len && DosName[len - 1] == L' ')
		len--;
	if (!len)
		return 0;

	/* strip trailing path separator */
	if (IS_PATH_SEPARATOR(DosName[len - 1]))
		len--;
	if (!len)
		return 0;
	if (FilePart)
		*FilePart = L'\0';
	memset (buf, 0, size);

CHECKPOINT;
	/* check for DOS device name */
	sz = RtlIsDosDeviceName_U (DosName);
	if (sz)
	{
		offs = sz >> 17;
		sz &= 0x0000FFFF;
		if (sz + 8 >= size)
			return sz + 10;
		wcscpy (buf, L"\\\\.\\");
		wcsncat (buf, DosName + offs, sz / sizeof(WCHAR));
		return sz + 8;
	}

CHECKPOINT;
	type = RtlDetermineDosPathNameType_U (DosName);

	RtlAcquirePebLock();

	cd = &(NtCurrentPeb ()->ProcessParameters->CurrentDirectory);
DPRINT("type %ld\n", type);
	switch (type)
	{
		case 1:		/* \\xxx or \\.xxx */
		case 6:		/* \\.\xxx */
			break;

		case 2:		/* x:\xxx  */
			*DosName = towupper (*DosName);
			break;

		case 3:		/* x:xxx   */
			drive = towupper (*DosName);
			DosName += 2;
			len     -= 2;
CHECKPOINT;
			if (drive == towupper (cd->DosPath.Buffer[0]))
			{
CHECKPOINT;
				wcscpy (buf, cd->DosPath.Buffer);
			}
			else
			{
CHECKPOINT;
				usvar.Length = 2 * swprintf (var, L"=%c:", drive);
				usvar.MaximumLength = 8;
				usvar.Buffer = var;
				pfx.Length = 0;
				pfx.MaximumLength = size;
				pfx.Buffer = buf;
				Status = RtlQueryEnvironmentVariable_U (NULL,
				                                        &usvar,
				                                        &pfx);
CHECKPOINT;
				if (!NT_SUCCESS(Status))
				{
CHECKPOINT;
					if (Status == STATUS_BUFFER_TOO_SMALL)
						return pfx.Length + len * 2 + 2;
					swprintf (buf, L"%c:\\", drive);
				}
				else
				{
CHECKPOINT;
					if (pfx.Length > 6)
					{
CHECKPOINT;
						buf[pfx.Length / 2] = L'\\';
						pfx.Length += 2;
					}
				}
			}
			break;

		case 4:		/* \xxx    */
			wcsncpy (buf, cd->DosPath.Buffer, 2);
			break;

		case 5:		/* xxx     */
			wcscpy (buf, cd->DosPath.Buffer);
			break;

		case 7:		/* \\.     */
			wcscpy (buf, L"\\\\.\\");
			len = 0;
			break;

		default:
			return 0;
	}

	DPRINT("buf \'%S\' DosName \'%S\' len %ld\n", buf, DosName, len);
	/* add dosname to prefix */
	wcsncat (buf, DosName, len);

	CHECKPOINT;
	/* replace slashes */
	for (wcs = buf; *wcs; wcs++)
		if (*wcs == L'/')
			*wcs = L'\\';

	len = wcslen (buf);
	if (len < 3 && buf[len-1] == L':')
		wcscat (buf, L"\\");

	DPRINT("buf \'%S\'\n", buf);
	RtlpEatPath (buf);
	DPRINT("buf \'%S\'\n", buf);

	len = wcslen (buf);

	/* find file part */
	if (FilePart)
	{
		for (wcs = buf + len - 1; wcs >= buf; wcs--)
		{
			if (*wcs == L'\\')
			{
				*FilePart = wcs + 1;
				break;
			}
		}
	}

	RtlReleasePebLock();

	return len * sizeof(WCHAR);
}


BOOLEAN
STDCALL
RtlDosPathNameToNtPathName_U (
	PWSTR		dosname,
	PUNICODE_STRING	ntname,
	PWSTR		*FilePart,
	PCURDIR		nah
	)
{
	UNICODE_STRING  us;
	PCURDIR cd;
	ULONG Type;
	ULONG Size;
	ULONG Length;
	ULONG Offset;
	WCHAR fullname[2*MAX_PATH];
	PWSTR Buffer = NULL;

	RtlAcquirePebLock ();

	RtlInitUnicodeString (&us, dosname);
	if (us.Length > 8)
	{
		Buffer = us.Buffer;
		/* check for "\\?\" - allows to use very long filenames ( up to 32k ) */
		if (Buffer[0] == L'\\' && Buffer[1] == L'\\' &&
		    Buffer[2] == L'?' && Buffer[3] == L'\\')
		{
//			if( f_77F68606( &us, ntname, shortname, nah ) )
//			{
//				RtlReleasePebLock ();
//				return TRUE;
//			}
			Buffer = NULL;
			RtlReleasePebLock ();
			return FALSE;
		}
	}

	Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                          0,
	                          sizeof( fullname ) + MAX_PFX_SIZE);
	if (Buffer == NULL)
	{
		RtlReleasePebLock ();
		return FALSE;
	}

	Size = RtlGetFullPathName_U (dosname,
	                             sizeof(fullname),
	                             fullname,
	                             FilePart);
	if (Size == 0 || Size > MAX_PATH * sizeof(WCHAR))
	{
		RtlFreeHeap (RtlGetProcessHeap (),
		             0,
		             Buffer);
		RtlReleasePebLock ();
		return FALSE;
	}

	/* Set NT prefix */
	Offset = 0;
	wcscpy (Buffer, L"\\??\\");

	Type = RtlDetermineDosPathNameType_U (fullname);
	switch (Type)
	{
		case 1:
			wcscat (Buffer, L"UNC\\");
			Offset = 2;
			break; /* \\xxx   */

		case 6:
			Offset = 4;
			break; /* \\.\xxx */
	}
	wcscat (Buffer, fullname + Offset);
	Length = wcslen (Buffer);

	/* set NT filename */
	ntname->Length        = Length * sizeof(WCHAR);
	ntname->MaximumLength = sizeof(fullname) + MAX_PFX_SIZE;
	ntname->Buffer        = Buffer;

	/* set pointer to file part if possible */
	if (FilePart && *FilePart)
		*FilePart = Buffer + Length - wcslen (*FilePart);

	/* Set name and handle structure if possible */
	if (nah)
	{
		memset (nah, 0, sizeof(CURDIR));
		cd = &(NtCurrentPeb ()->ProcessParameters->CurrentDirectory);
		if (Type == 5 && cd->Handle &&
		    !_wcsnicmp (cd->DosPath.Buffer, fullname, cd->DosPath.Length / 2))
		{
			Length = ((cd->DosPath.Length / sizeof(WCHAR)) - Offset) + ((Type == 1) ? 8 : 4);
			nah->DosPath.Buffer = Buffer + Length;
			nah->DosPath.Length = ntname->Length - (Length * sizeof(WCHAR));
			nah->DosPath.MaximumLength = nah->DosPath.Length;
			nah->Handle = cd->Handle;
		}
	}

	RtlReleasePebLock();

	return TRUE;
}


ULONG
STDCALL
RtlDosSearchPath_U (
	WCHAR *sp,
	WCHAR *name,
	WCHAR *ext,
	ULONG buf_sz,
	WCHAR *buffer,
	PWSTR *FilePart
	)
{
	ULONG Type;
	ULONG Length = 0;
	PWSTR full_name;
	PWSTR wcs;
	PWSTR path;

	Type = RtlDetermineDosPathNameType_U (name);

	if (Type == 5)
	{
		Length = wcslen (sp);
		Length += wcslen (name);
		if (wcschr (name, L'.'))
			ext = NULL;
		if (ext != NULL)
			Length += wcslen (ext);

		full_name = (WCHAR*)RtlAllocateHeap (RtlGetProcessHeap (),
		                                     0,
		                                     (Length + 1) * sizeof(WCHAR));
		Length = 0;
		if (full_name != NULL)
		{
			path = sp;
			while (*path)
			{
				wcs = full_name;
				while (*path && *path != L';')
					*wcs++ = *path++;
				if (*path)
					path++;
				if (wcs != full_name && *(wcs - 1) != L'\\')
					*wcs++ = L'\\';
				wcscpy (wcs, name);
				if (ext)
					wcscat (wcs, ext);
				if (RtlDoesFileExists_U (full_name))
				{
					Length = RtlGetFullPathName_U (full_name,
					                               buf_sz,
					                               buffer,
					                               FilePart);
					break;
				}
			}

			RtlFreeHeap (RtlGetProcessHeap (),
			             0,
			             full_name);
		}
	}
	else if (RtlDoesFileExists_U (name))
	{
		Length = RtlGetFullPathName_U (name,
		                               buf_sz,
		                               buffer,
		                               FilePart);
	}

	return Length;
}


BOOLEAN
STDCALL
RtlIsNameLegalDOS8Dot3 (
	PUNICODE_STRING	UnicodeName,
	PANSI_STRING	AnsiName,
	PBOOLEAN	SpacesFound
	)
{
	PANSI_STRING name = AnsiName;
	ANSI_STRING DummyString;
	CHAR Buffer[12];
	char *str;
	ULONG Length;
	ULONG i;
	NTSTATUS Status;
	BOOLEAN HasSpace = FALSE;
	BOOLEAN HasDot = FALSE;

	if (UnicodeName->Length > 24)
		return FALSE; /* name too long */

	if (!name)
	{
		name = &DummyString;
		name->Length = 0;
		name->MaximumLength = 12;
		name->Buffer = Buffer;
	}

	Status = RtlUpcaseUnicodeStringToCountedOemString (name,
	                                                   UnicodeName,
	                                                   FALSE);
	if (!NT_SUCCESS(Status))
		return FALSE;

	Length = name->Length;
	str = name->Buffer;

	if (!(Length == 1 && *str == '.') &&
	    !(Length == 2 && *str == '.' && *(str + 1) == '.'))
	{
		for (i = 0; i < Length; i++, str++)
		{
			switch (*str)
			{
				case ' ':
					HasSpace = TRUE;
					break;

				case '.':
					if ((HasDot) ||		/* two points */
					    (!i) ||		/* point is first char */
					    (i + 1 == Length) ||/* point is last char */
					    (Length - i > 4))	/* more than 3 chars of extension */
						return FALSE;
					HasDot = TRUE;
					break;
			}
		}
	}

	if (SpacesFound)
		*SpacesFound = HasSpace;

	return TRUE;
}


BOOLEAN
STDCALL
RtlDoesFileExists_U (
	IN	PWSTR	FileName
	)
{
	UNICODE_STRING NtFileName;
	OBJECT_ATTRIBUTES Attr;
	NTSTATUS Status;
	CURDIR CurDir;
	PWSTR Buffer;

	if (!RtlDosPathNameToNtPathName_U (FileName,
	                                   &NtFileName,
	                                   NULL,
	                                   &CurDir))
		return FALSE;

	/* don't forget to free it! */
	Buffer = NtFileName.Buffer;

	if (CurDir.DosPath.Length)
		NtFileName = CurDir.DosPath;
	else
		CurDir.Handle = 0;

	InitializeObjectAttributes (&Attr,
	                            &NtFileName,
	                            OBJ_CASE_INSENSITIVE,
	                            CurDir.Handle,
	                            NULL);

	Status = NtQueryAttributesFile (&Attr, NULL);

	RtlFreeHeap (RtlGetProcessHeap (),
	             0,
	             Buffer);

	if (NT_SUCCESS(Status) ||
	    Status == STATUS_SHARING_VIOLATION ||
	    Status == STATUS_ACCESS_DENIED)
		return TRUE;

	return FALSE;
}

/* EOF */
