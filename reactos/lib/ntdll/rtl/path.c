/* $Id: path.c,v 1.1 2000/02/05 16:08:49 ekohl Exp $
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
	DWORD Length;
	PCURDIR cd;

	DPRINT ("RtlGetCurrentDirectory %lu %p\n", MaximumLength, Buffer);

	cd = &(NtCurrentPeb ()->ProcessParameters->CurrentDirectory);

	RtlAcquirePebLock();
	Length = cd->DosPath.Length / sizeof(WCHAR);
	if (cd->DosPath.Buffer[Length - 2] != L':')
		Length--;

	DPRINT ("cd->DosPath.Buffer %S\n", cd->DosPath.Buffer);

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
	UNICODE_STRING             full;
	OBJECT_ATTRIBUTES          obj;
	IO_STATUS_BLOCK            iosb;
	PCURDIR cd;
	NTSTATUS Status;
	ULONG                      size;
	HANDLE                     handle = NULL;
	WCHAR                      *wcs,*buf = 0;

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

	obj.Length                   = sizeof(obj);
	obj.RootDirectory            = 0;
	obj.ObjectName               = &full;
	obj.Attributes               = OBJ_CASE_INSENSITIVE | OBJ_INHERIT;
	obj.SecurityDescriptor       = 0;
	obj.SecurityQualityOfService = 0;

	Status = NtOpenFile (&handle,
	                     SYNCHRONIZE | FILE_TRAVERSE,
	                     &obj,
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
	WCHAR *dosname,
	ULONG size,
	WCHAR *buf,
	WCHAR **shortname
	)
{
	WCHAR           *wcs, var[4], drive;
	int             len;
	DWORD           offs, sz, type;
	UNICODE_STRING  usvar, pfx;
	PCURDIR cd;
	NTSTATUS        status;

	DPRINT("RtlGetFullPathName_U %S %ld %p %p\n",
	       dosname, size, buf, shortname);

	if (!dosname || !*dosname)
		return 0;

	len = wcslen (dosname);

	/* strip trailing spaces */
	while (len && dosname[len - 1] == L' ')
		len--;
	if (!len)
		return 0;

	/* strip trailing path separator */
	if (IS_PATH_SEPARATOR(dosname[len - 1]))
		len--;
	if (!len)
		return 0;
	if (shortname)
		*shortname = 0;
	memset (buf, 0, size);

CHECKPOINT;
	/* check for DOS device name */
	sz = RtlIsDosDeviceName_U (dosname);
	if (sz)
	{
		offs = sz >> 17;
		sz &= 0x0000FFFF;
		if (sz + 8 >= size)
			return sz + 10;
		wcscpy (buf, L"\\\\.\\");
		wcsncat (buf, dosname + offs, sz / 2);
		return sz + 8;
	}

CHECKPOINT;
	type = RtlDetermineDosPathNameType_U (dosname);

	RtlAcquirePebLock();

	cd = &(NtCurrentPeb ()->ProcessParameters->CurrentDirectory);
DPRINT("type %ld\n", type);
	switch (type)
	{
		case 1:		/* \\xxx or \\.xxx */
		case 2:		/* x:\xxx  */
		case 6:		/* \\.\xxx */
			break;

		case 3:		/* x:xxx   */
			drive = towupper (*dosname);
			dosname += 2;
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
				usvar.Length        = 2 * swprintf( var, L"=%c:", drive );
				usvar.MaximumLength = 8;
				usvar.Buffer        = var;
				pfx.Length          = 0;
				pfx.MaximumLength   = size;
				pfx.Buffer          = buf;
				status = RtlQueryEnvironmentVariable_U( 0, &usvar, &pfx );
CHECKPOINT;
				if (!NT_SUCCESS(status))
				{
CHECKPOINT;
					if (status == STATUS_BUFFER_TOO_SMALL)
						return pfx.Length + len * 2 + 2;
					swprintf( buf, L"%c:\\", drive );
				}
				else
				{
CHECKPOINT;
					if( pfx.Length > 6 )
					{
CHECKPOINT;
						buf[ pfx.Length / 2 ] = L'\\';
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
			if (*dosname == L'.')
			{
				dosname += 2;
				len     -= 2;
			}
			break;

		case 7:		/* \\.     */
			wcscpy (buf, L"\\\\.\\");
			len = 0;
			break;

		default:
			return 0;
	}

CHECKPOINT;
	/* add dosname to prefix */
	wcsncat (buf, dosname, len);

CHECKPOINT;
	/* replace slashes */
	for (wcs = buf; *wcs; wcs++ )
		if (*wcs == L'/')
			*wcs = L'\\';

	len = wcslen (buf);

	/* find shortname */
	if (shortname)
	{
		for (wcs = buf + len - 1; wcs >= buf; wcs--)
		{
			if (*wcs == L'\\')
			{
				*shortname = wcs + 1;
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
	PWSTR		*shortname,
	PCURDIR		nah
	)
{
 UNICODE_STRING  us;
	PCURDIR cd;
 DWORD           type, size;
 WCHAR           *buf = 0, fullname[517];
 int             offs, len;

	RtlAcquirePebLock();

	RtlInitUnicodeString( &us, dosname );
	if( us.Length > 8 )
	{
		buf = us.Buffer;
		/* check for "\\?\" - allows to use very long filenames ( up to 32k ) */
		if (buf[0] == L'\\' && buf[1] == L'\\' && buf[2] == L'?' && buf[3] == L'\\')
		{
//			if( f_77F68606( &us, ntname, shortname, nah ) )
//				goto out;
			buf = 0;
			goto fail;
		}
	}

	buf = RtlAllocateHeap (RtlGetProcessHeap (),
	                       0,
	                       sizeof( fullname ) + MAX_PFX_SIZE );
	if (!buf)
		goto fail;

	size = RtlGetFullPathName_U( dosname, sizeof( fullname ), fullname, shortname );
	if( !size || size > 0x208 )
		goto fail;

	/* Set NT prefix */
	offs = 0;
	wcscpy (buf, L"\\??\\");

	type = RtlDetermineDosPathNameType_U( fullname );
	switch (type)
	{
		case 1:
			wcscat( buf, L"UNC\\" );
			offs = 2;
			break; /* \\xxx   */

		case 6:
			offs = 4;
			break; /* \\.\xxx */
	}
	wcscat( buf, fullname + offs );
	len = wcslen( buf );

	/* Set NT filename */
	ntname->Length        = len * 2;
	ntname->MaximumLength = sizeof( fullname ) + MAX_PFX_SIZE;
	ntname->Buffer        = buf;

	/* Set shortname if possible */
	if( shortname && *shortname )
		*shortname = buf + len - wcslen( *shortname );

	/* Set name and handle structure if possible */
	if( nah )
	{
		memset( nah, 0, sizeof(CURDIR));
		cd = &(NtCurrentPeb ()->ProcessParameters->CurrentDirectory);
		if (type == 5 && cd->Handle &&
		    !_wcsnicmp (cd->DosPath.Buffer, fullname, cd->DosPath.Length / 2))
		{
			len = (( cd->DosPath.Length / 2 ) - offs ) + ( ( type == 1 ) ? 8 : 4 );
			nah->DosPath.Buffer        = buf + len;
			nah->DosPath.Length        = ntname->Length - ( len * 2 );
			nah->DosPath.MaximumLength = nah->DosPath.Length;
			nah->Handle             = cd->Handle;
		}
	}
/* out:; */
	RtlReleasePebLock();
	return TRUE;

fail:;
	if( buf )
		RtlFreeHeap (RtlGetProcessHeap (), 0, buf );
	RtlReleasePebLock();
	return FALSE;
}


ULONG
STDCALL
RtlDosSearchPath_U (
	WCHAR *sp,
	WCHAR *name,
	WCHAR *ext,
	ULONG buf_sz,
	WCHAR *buffer,
	WCHAR **shortname
	)
{
 DWORD type;
 ULONG len = 0;
 WCHAR *full_name,*wcs,*path;

  type = RtlDetermineDosPathNameType_U( name );

  if( type != 5 )
  {
    if( RtlDoesFileExists_U( name ) )
    {
      len = RtlGetFullPathName_U( name, buf_sz, buffer, shortname );
    }
  }
  else
  {
    len = wcslen( sp );
    len += wcslen( name );
    if( wcschr( name, L'.' ) ) ext = 0;
    if( ext ) len += wcslen( ext );

    full_name = (WCHAR*)RtlAllocateHeap (RtlGetProcessHeap (), 0, ( len + 1 ) * 2 );
    len = 0;
    if( full_name )
    {
      path = sp;
      while( *path )
      {
        wcs = full_name;
        while( *path && *path != L';' ) *wcs++ = *path++;
        if( *path ) path++;
        if( wcs != full_name && *(wcs - 1) != L'\\' ) *wcs++ = L'\\';
        wcscpy( wcs, name );
        if( ext ) wcscat( wcs, ext );
        if( RtlDoesFileExists_U( full_name ) )
        {
          len = RtlGetFullPathName_U( full_name, buf_sz, buffer, shortname );
          break;
        }
      }
      RtlFreeHeap (RtlGetProcessHeap (), 0, full_name );
    }
  }
  return len;
}


BOOLEAN
STDCALL
RtlIsNameLegalDOS8Dot3 (
	PUNICODE_STRING	us,
	PANSI_STRING	as,
	PBOOLEAN	pb
	)
{
 ANSI_STRING *name = as,as1;
 char        buf[12],*str;
	NTSTATUS Status;
 BOOLEAN     have_space = FALSE;
 BOOLEAN have_point = FALSE;
 ULONG       len,i;

	if (us->Length > 24)
		return FALSE; /* name too long */

	if (!name)
	{
		name = &as1;
		name->Length = 0;
		name->MaximumLength = 12;
		name->Buffer = buf;
	}

	Status = RtlUpcaseUnicodeStringToCountedOemString (name,
	                                                   us,
	                                                   FALSE);
	if (!NT_SUCCESS(Status))
		return FALSE;

	len = name->Length;
	str = name->Buffer;

	if (!(len == 1 && *str == '.') &&
	    !(len == 2 && *(short*)(str) == 0x2E2E))
	{
		for (i = 0; i < len; i++, str++)
		{
			switch (*str)
			{
				case ' ':
					have_space = TRUE;
					break;

				case '.':
					if ((have_point) ||	/* two points */
					    (!i) ||		/* point is first char */
					    (i + 1 == len) ||	/* point is last char */
					    (len - i > 4))	/* more than 3 chars of extension */
						return FALSE;
					have_point = TRUE;
					break;
			}
		}
	}

	if (pb)
		*pb = have_space;

	return TRUE;
}


BOOLEAN
STDCALL
RtlDoesFileExists_U (
	PWSTR FileName
	)
{
	UNICODE_STRING NtFileName;
	OBJECT_ATTRIBUTES obj;
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

	obj.Length                   = sizeof(obj);
	obj.RootDirectory            = CurDir.Handle;
	obj.ObjectName               = &NtFileName;
	obj.Attributes               = OBJ_CASE_INSENSITIVE;
	obj.SecurityDescriptor       = 0;
	obj.SecurityQualityOfService = 0;

	Status = NtQueryAttributesFile (&obj, NULL);

	RtlFreeHeap (RtlGetProcessHeap(),
	             0,
	             Buffer);

	if (NT_SUCCESS(Status) ||
	    Status == STATUS_SHARING_VIOLATION ||
	    Status == STATUS_ACCESS_DENIED)
		return TRUE;

	return FALSE;
}

/* EOF */
