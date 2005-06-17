/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/ntdll/rtl/path.c
 * PURPOSE:         Path and current directory functions
 * UPDATE HISTORY:
 *                  Created 03/02/00
 */

/* INCLUDES ******************************************************************/

#define NDEBUG
#include <ntdll.h>

/* DEFINITONS and MACROS ******************************************************/

#define MAX_PFX_SIZE       16

#define IS_PATH_SEPARATOR(x) (((x)==L'\\')||((x)==L'/'))


/* GLOBALS ********************************************************************/

static const WCHAR DeviceRootW[] = L"\\\\.\\";

static const UNICODE_STRING _condev = RTL_CONSTANT_STRING(L"\\\\.\\CON");

static const UNICODE_STRING _lpt = RTL_CONSTANT_STRING(L"LPT");

static const UNICODE_STRING _com = RTL_CONSTANT_STRING(L"COM");

static const UNICODE_STRING _prn = RTL_CONSTANT_STRING(L"PRN");

static const UNICODE_STRING _aux = RTL_CONSTANT_STRING(L"AUX");

static const UNICODE_STRING _con = RTL_CONSTANT_STRING(L"CON");

static const UNICODE_STRING _nul = RTL_CONSTANT_STRING(L"NUL");

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
 *
 */
ULONG STDCALL
RtlDetermineDosPathNameType_U(PCWSTR Path)
{
   DPRINT("RtlDetermineDosPathNameType_U %S\n", Path);

   if (Path == NULL)
   {
      return INVALID_PATH;
   }

   if (IS_PATH_SEPARATOR(Path[0]))
   {
      if (!IS_PATH_SEPARATOR(Path[1])) return ABSOLUTE_PATH;         /* \xxx   */
      if (Path[2] != L'.') return UNC_PATH;                          /* \\xxx   */
      if (IS_PATH_SEPARATOR(Path[3])) return DEVICE_PATH;            /* \\.\xxx */
      if (Path[3]) return UNC_PATH;                                  /* \\.xxxx */

      return UNC_DOT_PATH;                                           /* \\.     */
   }
   else
   {
      /* FIXME: the Wine version of this line reads:
       * if (!Path[1] || Path[1] != L':')    return RELATIVE_PATH
       * Should we do this too?
       * -Gunnar
       */
      if (Path[1] != L':') return RELATIVE_PATH;                     /* xxx     */
      if (IS_PATH_SEPARATOR(Path[2])) return ABSOLUTE_DRIVE_PATH;    /* x:\xxx  */

      return RELATIVE_DRIVE_PATH;                                    /* x:xxx   */
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
RtlSetCurrentDirectory_U(PUNICODE_STRING dir)
{
   UNICODE_STRING full;
   UNICODE_STRING envvar;
   FILE_FS_DEVICE_INFORMATION device_info;
   OBJECT_ATTRIBUTES Attr;
   IO_STATUS_BLOCK iosb;
   PCURDIR cd;
   NTSTATUS Status;
   ULONG size;
   HANDLE handle = NULL;
   WCHAR var[4];
   PWSTR ptr;

   DPRINT("RtlSetCurrentDirectory %wZ\n", dir);

   RtlAcquirePebLock ();

   cd = (PCURDIR)&NtCurrentPeb ()->ProcessParameters->CurrentDirectoryName;

   if (!RtlDosPathNameToNtPathName_U (dir->Buffer, &full, 0, 0))
   {
      RtlReleasePebLock ();
      return STATUS_OBJECT_NAME_INVALID;
   }

   DPRINT("RtlSetCurrentDirectory: full %wZ\n",&full);

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
      RtlFreeUnicodeString( &full);
      RtlReleasePebLock ();
      return Status;
   }

   /* don't keep the directory handle open on removable media */
   if (NT_SUCCESS(NtQueryVolumeInformationFile( handle, &iosb, &device_info,
                                                sizeof(device_info), FileFsDeviceInformation )) &&
     (device_info.Characteristics & FILE_REMOVABLE_MEDIA))
   {
      DPRINT1("don't keep the directory handle open on removable media\n");
      NtClose( handle );
      handle = 0;
   }


/* What the heck is this all about??? It looks like its getting the long path,
 * and if does, ITS WRONG! If current directory is set with a short path,
 * GetCurrentDir should return a short path.
 * If anyone agrees with me, remove this stuff.
 * -Gunnar
 */
#if 0
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

   /* If it's just "\", we need special handling */
   if (filenameinfo->FileNameLength > sizeof(WCHAR))
     {
   wcs = buf + size / sizeof(WCHAR) - 1;
   if (*wcs == L'\\')
     {
       *(wcs) = 0;
       wcs--;
       size -= sizeof(WCHAR);
     }

   for (Index = 0;
        Index < filenameinfo->FileNameLength / sizeof(WCHAR);
        Index++)
     {
        if (filenameinfo->FileName[Index] == '\\') backslashcount++;
     }

   DPRINT("%d \n",backslashcount);
   for (;backslashcount;wcs--)
     {
        if (*wcs=='\\') backslashcount--;
     }
   wcs++;

   RtlCopyMemory(wcs, filenameinfo->FileName, filenameinfo->FileNameLength);
   wcs[filenameinfo->FileNameLength / sizeof(WCHAR)] = 0;

   size = (wcs - buf) * sizeof(WCHAR) + filenameinfo->FileNameLength;
     }
#endif



   if (cd->Handle)
      NtClose(cd->Handle);
   cd->Handle = handle;

   /* append trailing \ if missing */
   size = full.Length / sizeof(WCHAR);
   ptr = full.Buffer;
   ptr += 4;  /* skip \??\ prefix */
   size -= 4;

   /* This is ok because RtlDosPathNameToNtPathName_U returns a nullterminated string.
    * So the nullterm is replaced with \
    * -Gunnar
    */
   if (size && ptr[size - 1] != '\\') ptr[size++] = '\\';

   memcpy( cd->DosPath.Buffer, ptr, size * sizeof(WCHAR));
   cd->DosPath.Buffer[size] = 0;
   cd->DosPath.Length = size * sizeof(WCHAR);


   /* FIXME: whats this all about??? Wine doesnt have this. -Gunnar */
   if (cd->DosPath.Buffer[1]==':')
   {
      envvar.Length = 2 * swprintf (var, L"=%c:", cd->DosPath.Buffer[0]);
      envvar.MaximumLength = 8;
      envvar.Buffer = var;

      RtlSetEnvironmentVariable(NULL,
                 &envvar,
                 &cd->DosPath);
   }

   RtlFreeUnicodeString( &full);
   RtlReleasePebLock();

   return STATUS_SUCCESS;
}



/******************************************************************
 *    collapse_path
 *
 * Helper for RtlGetFullPathName_U.
 * 1) Convert slashes into backslashes
 * 2) Get rid of duplicate backslashes
 * 3) Get rid of . and .. components in the path.
 */
static inline void collapse_path( WCHAR *path, UINT mark )
{
    WCHAR *p, *next;

    /* convert every / into a \ */
    for (p = path; *p; p++) if (*p == '/') *p = '\\';

    /* collapse duplicate backslashes */
    next = path + max( 1, mark );
    for (p = next; *p; p++) if (*p != '\\' || next[-1] != '\\') *next++ = *p;
    *next = 0;

    p = path + mark;
    while (*p)
    {
        if (*p == '.')
        {
            switch(p[1])
            {
            case '\\': /* .\ component */
                next = p + 2;
                memmove( p, next, (wcslen(next) + 1) * sizeof(WCHAR) );
                continue;
            case 0:  /* final . */
                if (p > path + mark) p--;
                *p = 0;
                continue;
            case '.':
                if (p[2] == '\\')  /* ..\ component */
                {
                    next = p + 3;
                    if (p > path + mark)
                    {
                        p--;
                        while (p > path + mark && p[-1] != '\\') p--;
                    }
                    memmove( p, next, (wcslen(next) + 1) * sizeof(WCHAR) );
                    continue;
                }
                else if (!p[2])  /* final .. */
                {
                    if (p > path + mark)
                    {
                        p--;
                        while (p > path + mark && p[-1] != '\\') p--;
                        if (p > path + mark) p--;
                    }
                    *p = 0;
                    continue;
                }
                break;
            }
        }
        /* skip to the next component */
        while (*p && *p != '\\') p++;
        if (*p == '\\') p++;
    }

    /* remove trailing spaces and dots (yes, Windows really does that, don't ask) */
    while (p > path + mark && (p[-1] == ' ' || p[-1] == '.')) p--;
    *p = 0;
}



/******************************************************************
 *    skip_unc_prefix
 *
 * Skip the \\share\dir\ part of a file name. Helper for RtlGetFullPathName_U.
 */
static const WCHAR *skip_unc_prefix( const WCHAR *ptr )
{
    ptr += 2;
    while (*ptr && !IS_PATH_SEPARATOR(*ptr)) ptr++;  /* share name */
    while (IS_PATH_SEPARATOR(*ptr)) ptr++;
    while (*ptr && !IS_PATH_SEPARATOR(*ptr)) ptr++;  /* dir name */
    while (IS_PATH_SEPARATOR(*ptr)) ptr++;
    return ptr;
}


/******************************************************************
 *    get_full_path_helper
 *
 * Helper for RtlGetFullPathName_U
 * Note: name and buffer are allowed to point to the same memory spot
 */
static ULONG get_full_path_helper(
   LPCWSTR name,
   LPWSTR buffer,
   ULONG size)
{
    ULONG                       reqsize = 0, mark = 0, dep = 0, deplen;
    DOS_PATHNAME_TYPE           type;
    LPWSTR                      ins_str = NULL;
    LPCWSTR                     ptr;
    const UNICODE_STRING*       cd;
    WCHAR                       tmp[4];

    /* return error if name only consists of spaces */
    for (ptr = name; *ptr; ptr++) if (*ptr != ' ') break;
    if (!*ptr) return 0;

    RtlAcquirePebLock();

    cd = &((PCURDIR)&NtCurrentTeb()->Peb->ProcessParameters->CurrentDirectoryName)->DosPath;

    switch (type = RtlDetermineDosPathNameType_U(name))
    {
    case UNC_PATH:              /* \\foo   */
        ptr = skip_unc_prefix( name );
        mark = (ptr - name);
        break;

    case DEVICE_PATH:           /* \\.\foo */
        mark = 4;
        break;

    case ABSOLUTE_DRIVE_PATH:   /* c:\foo  */
        reqsize = sizeof(WCHAR);
        tmp[0] = towupper(name[0]);
        ins_str = tmp;
        dep = 1;
        mark = 3;
        break;

    case RELATIVE_DRIVE_PATH:   /* c:foo   */
        dep = 2;
        if (towupper(name[0]) != towupper(cd->Buffer[0]) || cd->Buffer[1] != ':')
        {
            UNICODE_STRING      var, val;

            tmp[0] = '=';
            tmp[1] = name[0];
            tmp[2] = ':';
            tmp[3] = '\0';
            var.Length = 3 * sizeof(WCHAR);
            var.MaximumLength = 4 * sizeof(WCHAR);
            var.Buffer = tmp;
            val.Length = 0;
            val.MaximumLength = size;
            val.Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, size);

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
                reqsize = val.Length + sizeof(WCHAR); /* append trailing '\\' */
                val.Buffer[val.Length / sizeof(WCHAR)] = '\\';
                ins_str = val.Buffer;
                break;
            case STATUS_VARIABLE_NOT_FOUND:
                reqsize = 3 * sizeof(WCHAR);
                tmp[0] = name[0];
                tmp[1] = ':';
                tmp[2] = '\\';
                ins_str = tmp;
                break;
            default:
                DPRINT1("Unsupported status code\n");
                break;
            }
            mark = 3;
            break;
        }
        /* fall through */

    case RELATIVE_PATH:         /* foo     */
        reqsize = cd->Length;
        ins_str = cd->Buffer;
        if (cd->Buffer[1] != ':')
        {
            ptr = skip_unc_prefix( cd->Buffer );
            mark = ptr - cd->Buffer;
        }
        else mark = 3;
        break;

    case ABSOLUTE_PATH:         /* \xxx    */
#ifdef __WINE__
        if (name[0] == '/')  /* may be a Unix path */
        {
            const WCHAR *ptr = name;
            int drive = find_drive_root( &ptr );
            if (drive != -1)
            {
                reqsize = 3 * sizeof(WCHAR);
                tmp[0] = 'A' + drive;
                tmp[1] = ':';
                tmp[2] = '\\';
                ins_str = tmp;
                mark = 3;
                dep = ptr - name;
                break;
            }
        }
#endif
        if (cd->Buffer[1] == ':')
        {
            reqsize = 2 * sizeof(WCHAR);
            tmp[0] = cd->Buffer[0];
            tmp[1] = ':';
            ins_str = tmp;
            mark = 3;
        }
        else
        {
            ptr = skip_unc_prefix( cd->Buffer );
            reqsize = (ptr - cd->Buffer) * sizeof(WCHAR);
            mark = reqsize / sizeof(WCHAR);
            ins_str = cd->Buffer;
        }
        break;

    case UNC_DOT_PATH:         /* \\.     */
        reqsize = 4 * sizeof(WCHAR);
        dep = 3;
        tmp[0] = '\\';
        tmp[1] = '\\';
        tmp[2] = '.';
        tmp[3] = '\\';
        ins_str = tmp;
        mark = 4;
        break;

    case INVALID_PATH:
        goto done;
    }

    /* enough space ? */
    deplen = wcslen(name + dep) * sizeof(WCHAR);
    if (reqsize + deplen + sizeof(WCHAR) > size)
    {
        /* not enough space, return need size (including terminating '\0') */
        reqsize += deplen + sizeof(WCHAR);
        goto done;
    }

    memmove(buffer + reqsize / sizeof(WCHAR), name + dep, deplen + sizeof(WCHAR));
    if (reqsize) memcpy(buffer, ins_str, reqsize);
    reqsize += deplen;

    if (ins_str && ins_str != tmp && ins_str != cd->Buffer)
        RtlFreeHeap(RtlGetProcessHeap(), 0, ins_str);

    collapse_path( buffer, mark );
    reqsize = wcslen(buffer) * sizeof(WCHAR);

done:
    RtlReleasePebLock();
    return reqsize;
}


/******************************************************************
 *    RtlGetFullPathName_U  (NTDLL.@)
 *
 * Returns the number of bytes written to buffer (not including the
 * terminating NULL) if the function succeeds, or the required number of bytes
 * (including the terminating NULL) if the buffer is too small.
 *
 * file_part will point to the filename part inside buffer (except if we use
 * DOS device name, in which case file_in_buf is NULL)
 *
 * @implemented
 */
DWORD STDCALL RtlGetFullPathName_U(
   const WCHAR* name,
   ULONG size,
   WCHAR* buffer,
   WCHAR** file_part)
{
    WCHAR*      ptr;
    DWORD       dosdev;
    DWORD       reqsize;

    DPRINT("RtlGetFullPathName_U(%S %lu %p %p)\n", name, size, buffer, file_part);

    if (!name || !*name) return 0;

    if (file_part) *file_part = NULL;

    /* check for DOS device name */
    dosdev = RtlIsDosDeviceName_U((WCHAR*)name);
    if (dosdev)
    {
        DWORD   offset = HIWORD(dosdev) / sizeof(WCHAR); /* get it in WCHARs, not bytes */
        DWORD   sz = LOWORD(dosdev); /* in bytes */

        if (8 + sz + 2 > size) return sz + 10;
        wcscpy(buffer, DeviceRootW);
        memmove(buffer + 4, name + offset, sz);
        buffer[4 + sz / sizeof(WCHAR)] = '\0';
        /* file_part isn't set in this case */
        return sz + 8;
    }

    reqsize = get_full_path_helper(name, buffer, size);
    if (!reqsize) return 0;
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
 * @implemented
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
	if (Type == ABSOLUTE_DRIVE_PATH ||
	    Type == RELATIVE_DRIVE_PATH)
	{
	    /* make the drive letter to uppercase */
	    Buffer[tmpLength] = towupper(Buffer[tmpLength]);
	}

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
 * @implemented
 */
BOOLEAN STDCALL
RtlDoesFileExists_U(IN PWSTR FileName)
{
	UNICODE_STRING NtFileName;
	OBJECT_ATTRIBUTES Attr;
   FILE_BASIC_INFORMATION Info;
	NTSTATUS Status;
	CURDIR CurDir;


	if (!RtlDosPathNameToNtPathName_U (FileName,
	                                   &NtFileName,
	                                   NULL,
	                                   &CurDir))
		return FALSE;

	if (CurDir.DosPath.Length)
		NtFileName = CurDir.DosPath;
	else
		CurDir.Handle = 0;

	InitializeObjectAttributes (&Attr,
	                            &NtFileName,
	                            OBJ_CASE_INSENSITIVE,
	                            CurDir.Handle,
	                            NULL);

	Status = NtQueryAttributesFile (&Attr, &Info);

   RtlFreeUnicodeString(&NtFileName);


	if (NT_SUCCESS(Status) ||
	    Status == STATUS_SHARING_VIOLATION ||
	    Status == STATUS_ACCESS_DENIED)
		return TRUE;

	return FALSE;
}

NTSTATUS STDCALL
RtlpEnsureBufferSize(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3)
{
    DPRINT1("RtlpEnsureBufferSize: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS STDCALL
RtlNtPathNameToDosPathName(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3, ULONG Unknown4)
{
    DPRINT1("RtlNtPathNameToDosPathName: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
