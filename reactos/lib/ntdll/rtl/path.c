/* $Id: path.c,v 1.22 2003/10/18 20:32:58 navaraf Exp $
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
#include <ddk/obfuncs.h>
#include <ntos/rtl.h>

#define NDEBUG
#include <ntdll/ntdll.h>

/* DEFINITONS and MACROS ******************************************************/

#define MAX_PFX_SIZE       16

#define IS_PATH_SEPARATOR(x) (((x)==L'\\')||((x)==L'/'))

/* GLOBALS ********************************************************************/

static const UNICODE_STRING _condev = 
{
    .Length	    = sizeof(L"\\\\.\\CON") - sizeof(WCHAR),
    .MaximumLength  = sizeof(L"\\\\.\\CON"),
    .Buffer	    = L"\\\\.\\CON"
};

static const UNICODE_STRING _lpt =
{
    .Length	    = sizeof(L"LPT") - sizeof(WCHAR),
    .MaximumLength  = sizeof(L"LPT"),
    .Buffer	    = L"LPT"
};

static const UNICODE_STRING _com =
{
    .Length	    = sizeof(L"COM") - sizeof(WCHAR),
    .MaximumLength  = sizeof(L"COM"),
    .Buffer	    = L"COM"
};

static const UNICODE_STRING _prn =
{
    .Length	    = sizeof(L"PRN") - sizeof(WCHAR),
    .MaximumLength  = sizeof(L"PRN"),
    .Buffer	    = L"PRN"
};

static const UNICODE_STRING _aux =
{
    .Length	    = sizeof(L"AUX") - sizeof(WCHAR),
    .MaximumLength  = sizeof(L"AUX"),
    .Buffer	    = L"AUX"
};

static const UNICODE_STRING _con =
{
    .Length	    = sizeof(L"CON") - sizeof(WCHAR),
    .MaximumLength  = sizeof(L"CON"),
    .Buffer	    = L"CON"
};

static const UNICODE_STRING _nul =
{
    .Length	    = sizeof(L"NUL") - sizeof(WCHAR),
    .MaximumLength  = sizeof(L"NUL"),
    .Buffer	    = L"NUL"
};

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
ULONG STDCALL RtlGetLongestNtPathLength (VOID)
{
   return (MAX_PATH + 9);
}


/*
 * @implemented
 */
ULONG STDCALL
RtlDetermineDosPathNameType_U(PWSTR Path)
{
   DPRINT("RtlDetermineDosPathNameType_U %S\n", Path);

   if (Path == NULL)
     {
	return 0;
     }

   if (IS_PATH_SEPARATOR(Path[0]))
     {
	if (!IS_PATH_SEPARATOR(Path[1]))
	  {
	     return 4;			/* \xxx   */
	  }

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

/*
 * @implemented
 */
ULONG STDCALL
RtlIsDosDeviceName_U(PWSTR DeviceName)
{
   ULONG Type;
   ULONG Length = 0;
   ULONG Offset;
   PWCHAR wc;
   UNICODE_STRING DeviceNameU;

   if (DeviceName == NULL)
     {
	return 0;
     }

   while (DeviceName[Length])
     {
	Length++;
     }

   Type = RtlDetermineDosPathNameType_U(DeviceName);
   if (Type <= 1)
     {
	return 0;
     }

   if (Type == 6)
     {
        DeviceNameU.Length = DeviceNameU.MaximumLength = Length * sizeof(WCHAR);
	DeviceNameU.Buffer = DeviceName;
	if (Length == 7 &&
	    RtlEqualUnicodeString(&DeviceNameU, (PUNICODE_STRING)&_condev, TRUE))
		return 0x00080006;
	return 0;
     }

   /* name can end with ':' */
   if (Length && DeviceName[Length - 1 ] == L':')
     {
	Length--;
     }

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
     {
	wc--;
     }
   Offset = wc - DeviceName;
   Length -= Offset;
   DeviceNameU.Length = DeviceNameU.MaximumLength = 3 * sizeof(WCHAR);
   DeviceNameU.Buffer = wc;
   
   /* check for LPTx or COMx */
   if (Length == 4 && wc[3] >= L'0' && wc[3] <= L'9')
     {
	if (wc[3] == L'0')
	  {
	     return 0;
	  }
   
	if (RtlEqualUnicodeString(&DeviceNameU, (PUNICODE_STRING)&_lpt, TRUE) ||
	    RtlEqualUnicodeString(&DeviceNameU, (PUNICODE_STRING)&_com, TRUE))
	  {
	     return ((Offset * 2) << 16 ) | 8;
	  }
	return 0;
     }
   
   /* check for PRN,AUX,NUL or CON */
   if (Length == 3 &&
       (RtlEqualUnicodeString(&DeviceNameU, (PUNICODE_STRING)&_prn, TRUE) ||
        RtlEqualUnicodeString(&DeviceNameU, (PUNICODE_STRING)&_aux, TRUE) ||
        RtlEqualUnicodeString(&DeviceNameU, (PUNICODE_STRING)&_nul, TRUE) ||
        RtlEqualUnicodeString(&DeviceNameU, (PUNICODE_STRING)&_con, TRUE)))
     {
	return ((Offset * 2) << 16) | 6;
     }
   
   return 0;
}


/*
 * @implemented
 */
ULONG STDCALL
RtlGetCurrentDirectory_U(ULONG MaximumLength,
			 PWSTR Buffer)
{
	ULONG Length;
	PCURDIR cd;

	DPRINT ("RtlGetCurrentDirectory %lu %p\n", MaximumLength, Buffer);

	RtlAcquirePebLock();

	cd = (PCURDIR)&(NtCurrentPeb ()->ProcessParameters->CurrentDirectoryName);
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


/*
 * @implemented
 */
NTSTATUS STDCALL
RtlSetCurrentDirectory_U(PUNICODE_STRING name)
{
   UNICODE_STRING full;
   UNICODE_STRING envvar;
   OBJECT_ATTRIBUTES Attr;
   IO_STATUS_BLOCK iosb;
   PCURDIR cd;
   NTSTATUS Status;
   ULONG size;
   HANDLE handle = NULL;
   PWSTR wcs;
   PWSTR buf = 0;
   PFILE_NAME_INFORMATION filenameinfo;
   ULONG backslashcount = 0;
   PWSTR cntr;
   WCHAR var[4];
   
   DPRINT ("RtlSetCurrentDirectory %wZ\n", name);
   
   RtlAcquirePebLock ();
   cd = (PCURDIR)&NtCurrentPeb ()->ProcessParameters->CurrentDirectoryName;

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
   
   filenameinfo = RtlAllocateHeap(RtlGetProcessHeap(),
				  0,
				  MAX_PATH*sizeof(WCHAR)+sizeof(ULONG));
   
   Status = NtQueryInformationFile(handle,
				   &iosb,
				   filenameinfo,
				   MAX_PATH*sizeof(WCHAR)+sizeof(ULONG),
				   FileNameInformation);
   if (!NT_SUCCESS(Status))
     {
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    filenameinfo);
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    buf);
	RtlFreeHeap(RtlGetProcessHeap(),
		    0,
		    full.Buffer);
	RtlReleasePebLock();
	return(Status);
     }
   
   if (filenameinfo->FileName[1]) // If it's just "\", we need special handling
     {
	wcs = buf + size / sizeof(WCHAR) - 1;
	if (*wcs == L'\\')
	  {
	    *(wcs) = 0;
	    wcs--;
	    size -= sizeof(WCHAR);
	  }

	for (cntr=filenameinfo->FileName;*cntr!=0;cntr++)
	  {
	     if (*cntr=='\\') backslashcount++;
	  }

	DPRINT("%d \n",backslashcount);
	for (;backslashcount;wcs--)
	  {
	     if (*wcs=='\\') backslashcount--;
	  }
	wcs++;

	wcscpy(wcs,filenameinfo->FileName);

	size=((wcs-buf)+wcslen(filenameinfo->FileName))*sizeof(WCHAR);
     }
   
   RtlFreeHeap (RtlGetProcessHeap (),
		0,
		filenameinfo);
   
   /* append backslash if missing */
   wcs = buf + size / sizeof(WCHAR) - 1;
   if (*wcs != L'\\')
     {
	*(++wcs) = L'\\';
	*(++wcs) = 0;
	size += sizeof(WCHAR);
     }
   
   memmove(cd->DosPath.Buffer,
	   buf,
	   size + sizeof(WCHAR));
   cd->DosPath.Length = size;
   
   if (cd->Handle)
     NtClose(cd->Handle);
   cd->Handle = handle;

   if (cd->DosPath.Buffer[1]==':')
     {
	envvar.Length = 2 * swprintf (var, L"=%c:", cd->DosPath.Buffer[0]);
	envvar.MaximumLength = 8;
	envvar.Buffer = var;
   
	RtlSetEnvironmentVariable(NULL,
				  &envvar,
				  &cd->DosPath);
   }
   
   RtlFreeHeap (RtlGetProcessHeap (),
		0,
		buf);
   
   RtlFreeHeap (RtlGetProcessHeap (),
		0,
		full.Buffer);
   
   RtlReleasePebLock();
   
   return STATUS_SUCCESS;
}

/******************************************************************
 *		get_full_path_helper
 *
 * Helper for RtlGetFullPathName_U
 */
static ULONG get_full_path_helper(LPCWSTR name, LPWSTR buffer, ULONG size)
{
    ULONG               reqsize, mark = 0;
    /*DOS_PATHNAME_TYPE*/INT   type;
    LPWSTR              ptr;
    UNICODE_STRING*     cd;

    reqsize = sizeof(WCHAR); /* '\0' at the end */

    RtlAcquirePebLock();
    cd = &NtCurrentPeb ()->ProcessParameters->CurrentDirectoryName;

    switch (type = RtlDetermineDosPathNameType_U((LPWSTR)name))
    {
    case 1 /*UNC_PATH*/:              /* \\foo   */
    case 6 /*DEVICE_PATH*/:           /* \\.\foo */
        if (reqsize <= size) buffer[0] = '\0';
        break;

    case 2 /*ABSOLUTE_DRIVE_PATH*/:   /* c:\foo  */
        reqsize += sizeof(WCHAR);
        if (reqsize <= size)
        {
            buffer[0] = RtlUpcaseUnicodeChar(name[0]);
            buffer[1] = '\0';
        }
        name++;
        break;

    case 3 /*RELATIVE_DRIVE_PATH*/:   /* c:foo   */
        if (RtlUpcaseUnicodeChar(name[0]) != RtlUpcaseUnicodeChar(cd->Buffer[0]) ||
            cd->Buffer[1] != ':')
        {
            WCHAR               drive[4];
            UNICODE_STRING      var, val;

            drive[0] = '=';
            drive[1] = name[0];
            drive[2] = ':';
            drive[3] = '\0';
            var.Length = 6;
            var.MaximumLength = 8;
            var.Buffer = drive;
            val.Length = 0;
            val.MaximumLength = size;
            val.Buffer = buffer;

            name += 2;

            switch (RtlQueryEnvironmentVariable_U(NULL, &var, &val))
            {
            case STATUS_SUCCESS:
                /* FIXME: Win2k seems to check that the environment variable actually points 
                 * to an existing directory. If not, root of the drive is used
                 * (this seems also to be the only spot in RtlGetFullPathName that the 
                 * existence of a part of a path is checked)
                 */
                /* fall thru */
            case STATUS_BUFFER_TOO_SMALL:
                reqsize += val.Length;
                /* append trailing \\ */
                reqsize += sizeof(WCHAR);
                if (reqsize <= size)
                {
                    buffer[reqsize / sizeof(WCHAR) - 2] = '\\';
                    buffer[reqsize / sizeof(WCHAR) - 1] = '\0';
                }
                break;
            case STATUS_VARIABLE_NOT_FOUND:
                reqsize += 3 * sizeof(WCHAR);
                if (reqsize <= size)
                {
                    buffer[0] = drive[1];
                    buffer[1] = ':';
                    buffer[2] = '\\';
                    buffer[3] = '\0';
                }
                break;
            default:
                DPRINT("Unsupported status code\n");
                break;
            }
            break;
        }
        name += 2;
        /* fall through */

    case 5 /*RELATIVE_PATH*/:         /* foo     */
        reqsize += cd->Length;
        if (reqsize <= size)
        {
            memcpy(buffer, cd->Buffer, cd->Length);
            buffer[cd->Length / sizeof(WCHAR)] = 0;
        }
        if (cd->Buffer[1] != ':')
        {
            ptr = wcsrchr(cd->Buffer + 2, '\\');
            if (ptr) ptr = wcsrchr(ptr + 1, '\\');
            if (!ptr) ptr = cd->Buffer + wcslen(cd->Buffer);
            mark = ptr - cd->Buffer;
        }
        break;

    case 4 /*ABSOLUTE_PATH*/:         /* \xxx    */
        if (cd->Buffer[1] == ':')
        {
            reqsize += 2 * sizeof(WCHAR);
            if (reqsize <= size)
            {
                buffer[0] = cd->Buffer[0];
                buffer[1] = ':';
                buffer[2] = '\0';
            }
        }
        else
        {
            unsigned    len;

            ptr = wcsrchr(cd->Buffer + 2, '\\');
            if (ptr) ptr = wcsrchr(ptr + 1, '\\');
            if (!ptr) ptr = cd->Buffer + wcslen(cd->Buffer);
            len = (ptr - cd->Buffer) * sizeof(WCHAR);
            reqsize += len;
            mark = len / sizeof(WCHAR);
            if (reqsize <= size)
            {
                memcpy(buffer, cd->Buffer, len);
                buffer[len / sizeof(WCHAR)] = '\0';
            }
            else
                buffer[0] = '\0';
        }
        break;

    case 7 /*UNC_DOT_PATH*/:         /* \\.     */
        reqsize += 4 * sizeof(WCHAR);
        name += 3;
        if (reqsize <= size)
        {
            buffer[0] = '\\';
            buffer[1] = '\\';
            buffer[2] = '.';
            buffer[3] = '\\';
            buffer[4] = '\0';
        }
        break;

    case 0 /*INVALID_PATH*/:
        reqsize = 0;
        goto done;
    }

    reqsize += wcslen(name) * sizeof(WCHAR);
    if (reqsize > size) goto done;

    wcscat(buffer, name);

    /* convert every / into a \ */
    for (ptr = buffer; ptr < buffer + size / sizeof(WCHAR); ptr++) 
        if (*ptr == '/') *ptr = '\\';

    reqsize -= sizeof(WCHAR); /* don't count trailing \0 */

    /* mark is non NULL for UNC names, so start path collapsing after server & share name
     * otherwise, it's a fully qualified DOS name, so start after the drive designation
     */
    for (ptr = buffer + (mark ? mark : 2); ptr < buffer + reqsize / sizeof(WCHAR); )
    {
        WCHAR* p = wcsrchr(ptr, '\\');
        if (!p) break;

        p++;
        if (p[0] == '.')
        {
            switch (p[1])
            {
            case '.':
                switch (p[2])
                {
                case '\\':
                    {
                        WCHAR*      prev = p - 2;
                        while (prev >= buffer + mark && *prev != '\\') prev--;
                        /* either collapse \foo\.. into \ or \.. into \ */
                        if (prev < buffer + mark) prev = p - 1;
                        reqsize -= (p + 2 - prev) * sizeof(WCHAR);
                        memmove(prev, p + 2, buffer + reqsize - prev + sizeof(WCHAR));
                        p = prev;
                    }
                    break;
                case '\0':
                    reqsize -= 2 * sizeof(WCHAR);
                    *p = 0;
                    break;
                }
                break;
            case '\\':
                reqsize -= 2 * sizeof(WCHAR);
                memmove(ptr, ptr + 2, buffer + reqsize - ptr + sizeof(WCHAR));
                break;
            }
        }
        ptr = p;
    }

done:
    RtlReleasePebLock();
    return reqsize;
}

/*
 * @implemented
 */
DWORD STDCALL RtlGetFullPathName_U(WCHAR* name, ULONG size, WCHAR* buffer,
                                  WCHAR** file_part)
{
    WCHAR*      ptr;
    DWORD       dosdev;
    DWORD       reqsize;

    DPRINT("(%s %lu %p %p)\n", name, size, buffer, file_part);

    if (!name || !*name) return 0;

    if (file_part) *file_part = NULL;

    /* check for DOS device name */
    dosdev = RtlIsDosDeviceName_U((WCHAR *)name);
    if (dosdev)
    {
        DWORD   offset = ((dosdev & 0xffff0000) >> 16) / sizeof(WCHAR); /* get it in WCHARs, not bytes */
        DWORD   sz = dosdev & 0xffff; /* in bytes */

        if (8 + sz + 2 > size) return sz + 10;
        wcscpy(buffer, L"\\\\.\\");
        memmove(buffer + 4, name + offset, sz);
        buffer[4 + sz / sizeof(WCHAR)] = '\0';
        /* file_part isn't set in this case */
        return sz + 8;
    }

    reqsize = get_full_path_helper(name, buffer, size);
    if (reqsize > size)
    {
        LPWSTR tmp = RtlAllocateHeap(RtlGetProcessHeap(), 0, reqsize);
        reqsize = get_full_path_helper(name, tmp, reqsize);
        if (reqsize > size)  /* it may have worked the second time */
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, tmp);
            return reqsize + sizeof(WCHAR);
        }
        memcpy( buffer, tmp, reqsize + sizeof(WCHAR) );
        RtlFreeHeap(RtlGetProcessHeap(), 0, tmp);
    }

    /* find file part */
    if (file_part && (ptr = wcsrchr(buffer, '\\')) != NULL && ptr >= buffer + 2 && *++ptr)
        *file_part = ptr;
    return reqsize;
}

/*
 * @unimplemented
 */
BOOLEAN STDCALL
RtlDosPathNameToNtPathName_U(PWSTR dosname,
			     PUNICODE_STRING ntname,
			     PWSTR *FilePart,
			     PCURDIR nah)
{
	UNICODE_STRING  us;
	PCURDIR cd;
	ULONG Type;
	ULONG Size;
	ULONG Length;
	ULONG tmpLength;
	ULONG Offset;
	WCHAR fullname[MAX_PATH + 1];
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
	memcpy (Buffer, L"\\??\\", 4 * sizeof(WCHAR));
	tmpLength = 4;

	Type = RtlDetermineDosPathNameType_U (fullname);
	switch (Type)
	{
		case 1:
			memcpy (Buffer + tmpLength, L"UNC\\", 4 * sizeof(WCHAR));
			tmpLength += 4;
			Offset = 2;
			break; /* \\xxx   */

		case 6:
			Offset = 4;
			break; /* \\.\xxx */
	}
	Length = wcslen(fullname + Offset);
	memcpy (Buffer + tmpLength, fullname + Offset, (Length + 1) * sizeof(WCHAR));
	Length += tmpLength;

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
		cd = (PCURDIR)&(NtCurrentPeb ()->ProcessParameters->CurrentDirectoryName);
		if (Type == 5 && cd->Handle)
		{
		    RtlInitUnicodeString(&us, fullname);
		    if (RtlEqualUnicodeString(&us, &cd->DosPath, TRUE))
		    {
			Length = ((cd->DosPath.Length / sizeof(WCHAR)) - Offset) + ((Type == 1) ? 8 : 4);
			nah->DosPath.Buffer = Buffer + Length;
			nah->DosPath.Length = ntname->Length - (Length * sizeof(WCHAR));
			nah->DosPath.MaximumLength = nah->DosPath.Length;
			nah->Handle = cd->Handle;
		    }
		}
	}

	RtlReleasePebLock();

	return TRUE;
}


/*
 * @implemented
 */
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


/*
 * @unimplemented
 */
BOOLEAN STDCALL
RtlDoesFileExists_U(IN PWSTR FileName)
{
	UNICODE_STRING NtFileName;
	OBJECT_ATTRIBUTES Attr;
	NTSTATUS Status;
	CURDIR CurDir;
	PWSTR Buffer;

	/* only used by replacement code */
	HANDLE FileHandle;
	IO_STATUS_BLOCK StatusBlock;

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

	/* FIXME: not implemented yet */
//	Status = NtQueryAttributesFile (&Attr, NULL);

	/* REPLACEMENT start */
	Status = NtOpenFile (&FileHandle,
	                     0x10001,
	                     &Attr,
	                     &StatusBlock,
	                     1,
	                     FILE_SYNCHRONOUS_IO_NONALERT);
	if (NT_SUCCESS(Status))
		NtClose (FileHandle);
	/* REPLACEMENT end */

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
