/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/rtl/path.c
 * PURPOSE:         Path and current directory functions
 * PROGRAMMERS:
 */

/* INCLUDES *****************************************************************/

#include <rtl.h>

#define NDEBUG
#include <debug.h>

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
ULONG NTAPI RtlGetLongestNtPathLength (VOID)
{
   return (MAX_PATH + 9);
}


/*
 * @implemented
 *
 */
ULONG NTAPI
RtlDetermineDosPathNameType_U(PCWSTR Path)
{
   DPRINT("RtlDetermineDosPathNameType_U %S\n", Path);

   if (Path == NULL)
   {
      return RtlPathTypeUnknown;
   }

   if (IS_PATH_SEPARATOR(Path[0]))
   {
      if (!IS_PATH_SEPARATOR(Path[1])) return RtlPathTypeRooted;         /* \xxx   */
      if (Path[2] != L'.') return RtlPathTypeUncAbsolute;                          /* \\xxx   */
      if (IS_PATH_SEPARATOR(Path[3])) return RtlPathTypeLocalDevice;            /* \\.\xxx */
      if (Path[3]) return RtlPathTypeUncAbsolute;                                  /* \\.xxxx */

      return RtlPathTypeRootLocalDevice;                                           /* \\.     */
   }
   else
   {
      if (!Path[0] || Path[1] != L':') return RtlPathTypeRelative;       /* xxx     */
      if (IS_PATH_SEPARATOR(Path[2])) return RtlPathTypeDriveAbsolute;    /* x:\xxx  */

      return RtlPathTypeDriveRelative;                                    /* x:xxx   */
   }
}


/* returns 0 if name is not valid DOS device name, or DWORD with
 * offset in bytes to DOS device name from beginning of buffer in high word
 * and size in bytes of DOS device name in low word */

/*
 * @implemented
 */
ULONG NTAPI
RtlIsDosDeviceName_U(PWSTR dos_name)
{
    static const WCHAR consoleW[] = {'\\','\\','.','\\','C','O','N',0};
    static const WCHAR auxW[3] = {'A','U','X'};
    static const WCHAR comW[3] = {'C','O','M'};
    static const WCHAR conW[3] = {'C','O','N'};
    static const WCHAR lptW[3] = {'L','P','T'};
    static const WCHAR nulW[3] = {'N','U','L'};
    static const WCHAR prnW[3] = {'P','R','N'};

    const WCHAR *start, *end, *p;

    switch(RtlDetermineDosPathNameType_U( dos_name ))
    {
    case RtlPathTypeUnknown:
    case RtlPathTypeUncAbsolute:
        return 0;
    case RtlPathTypeLocalDevice:
        if (!_wcsicmp( dos_name, consoleW ))
            return MAKELONG( sizeof(conW), 4 * sizeof(WCHAR) );  /* 4 is length of \\.\ prefix */
        return 0;
    default:
        break;
    }

    end = dos_name + wcslen(dos_name) - 1;
    while (end >= dos_name && *end == ':') end--;  /* remove all trailing ':' */

    /* find start of file name */
    for (start = end; start >= dos_name; start--)
    {
        if (IS_PATH_SEPARATOR(start[0])) break;
        /* check for ':' but ignore if before extension (for things like NUL:.txt) */
        if (start[0] == ':' && start[1] != '.') break;
    }
    start++;

    /* remove extension */
    if ((p = wcschr( start, '.' )))
    {
        end = p - 1;
        if (end >= dos_name && *end == ':') end--;  /* remove trailing ':' before extension */
    }
    /* remove trailing spaces */
    while (end >= dos_name && *end == ' ') end--;

    /* now we have a potential device name between start and end, check it */
    switch(end - start + 1)
    {
    case 3:
        if (_wcsnicmp( start, auxW, 3 ) &&
            _wcsnicmp( start, conW, 3 ) &&
            _wcsnicmp( start, nulW, 3 ) &&
            _wcsnicmp( start, prnW, 3 )) break;
        return MAKELONG( 3 * sizeof(WCHAR), (start - dos_name) * sizeof(WCHAR) );
    case 4:
        if (_wcsnicmp( start, comW, 3 ) && _wcsnicmp( start, lptW, 3 )) break;
        if (*end <= '0' || *end > '9') break;
        return MAKELONG( 4 * sizeof(WCHAR), (start - dos_name) * sizeof(WCHAR) );
    default:  /* can't match anything */
        break;
    }
    return 0;
}


/*
 * @implemented
 */
ULONG NTAPI
RtlGetCurrentDirectory_U(ULONG MaximumLength,
			 PWSTR Buffer)
{
	ULONG Length;
	PCURDIR cd;

	DPRINT ("RtlGetCurrentDirectory %lu %p\n", MaximumLength, Buffer);

	RtlAcquirePebLock();

	cd = (PCURDIR)&(NtCurrentPeb ()->ProcessParameters->CurrentDirectory.DosPath);
	Length = cd->DosPath.Length / sizeof(WCHAR);
	if (cd->DosPath.Buffer[Length - 1] == L'\\' &&
	    cd->DosPath.Buffer[Length - 2] != L':')
		Length--;

	DPRINT ("cd->DosPath.Buffer %S Length %lu\n",
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
NTSTATUS NTAPI
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

   cd = (PCURDIR)&NtCurrentPeb ()->ProcessParameters->CurrentDirectory.DosPath;

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

   Status = ZwOpenFile (&handle,
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
   if (NT_SUCCESS(ZwQueryVolumeInformationFile( handle, &iosb, &device_info,
                                                sizeof(device_info), FileFsDeviceInformation )) &&
     (device_info.Characteristics & FILE_REMOVABLE_MEDIA))
   {
      DPRINT1("don't keep the directory handle open on removable media\n");
      ZwClose( handle );
      handle = 0;
   }

   if (cd->Handle)
      ZwClose(cd->Handle);
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
static __inline void collapse_path( WCHAR *path, UINT mark )
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
        if (*p == '\\')
        {
            /* remove last dot in previous dir name */
            if (p > path + mark && p[-1] == '.') memmove( p-1, p, (wcslen(p) + 1) * sizeof(WCHAR) );
            else p++;
        }
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
    RTL_PATH_TYPE               type;
    LPWSTR                      ins_str = NULL;
    LPCWSTR                     ptr;
    const UNICODE_STRING*       cd;
    WCHAR                       tmp[4];

    /* return error if name only consists of spaces */
    for (ptr = name; *ptr; ptr++) if (*ptr != ' ') break;
    if (!*ptr) return 0;

    RtlAcquirePebLock();

    //cd = &((PCURDIR)&NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->CurrentDirectory.DosPath)->DosPath;
    cd = &NtCurrentTeb()->ProcessEnvironmentBlock->ProcessParameters->CurrentDirectory.DosPath;

    switch (type = RtlDetermineDosPathNameType_U(name))
    {
    case RtlPathTypeUncAbsolute:              /* \\foo   */
        ptr = skip_unc_prefix( name );
        mark = (ptr - name);
        break;

    case RtlPathTypeLocalDevice:           /* \\.\foo */
        mark = 4;
        break;

    case RtlPathTypeDriveAbsolute:   /* c:\foo  */
        reqsize = sizeof(WCHAR);
        tmp[0] = towupper(name[0]);
        ins_str = tmp;
        dep = 1;
        mark = 3;
        break;

    case RtlPathTypeDriveRelative:   /* c:foo   */
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
            if (val.Buffer == NULL)
            {
                reqsize = 0;
                goto done;
            }

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

    case RtlPathTypeRelative:         /* foo     */
        reqsize = cd->Length;
        ins_str = cd->Buffer;
        if (cd->Buffer[1] != ':')
        {
            ptr = skip_unc_prefix( cd->Buffer );
            mark = ptr - cd->Buffer;
        }
        else mark = 3;
        break;

    case RtlPathTypeRooted:         /* \xxx    */
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

    case RtlPathTypeRootLocalDevice:         /* \\.     */
        reqsize = 4 * sizeof(WCHAR);
        dep = 3;
        tmp[0] = '\\';
        tmp[1] = '\\';
        tmp[2] = '.';
        tmp[3] = '\\';
        ins_str = tmp;
        mark = 4;
        break;

    case RtlPathTypeUnknown:
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
ULONG NTAPI RtlGetFullPathName_U(
   const WCHAR* name,
   ULONG size,
   WCHAR* buffer,
   WCHAR** file_part)
{
    WCHAR*      ptr;
    ULONG       dosdev;
    ULONG       reqsize;

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
        if (tmp == NULL)
            return 0;
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
BOOLEAN NTAPI
RtlDosPathNameToNtPathName_U(IN PCWSTR DosPathName,
			     OUT PUNICODE_STRING NtPathName,
			     OUT PCWSTR *NtFileNamePart,
			     OUT CURDIR *DirectoryInfo)
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

	RtlInitUnicodeString (&us, DosPathName);
	if (us.Length > 8)
	{
		Buffer = us.Buffer;
		/* check for "\\?\" - allows to use very long filenames ( up to 32k ) */
		if (Buffer[0] == L'\\' && Buffer[1] == L'\\' &&
		    Buffer[2] == L'?' && Buffer[3] == L'\\')
                {
			/* allocate the new string and simply copy it */
			NtPathName->Length = us.Length;
			NtPathName->MaximumLength = us.Length + sizeof(WCHAR);
			NtPathName->Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
			                                     0,
			                                     NtPathName->MaximumLength);
			if (NtPathName->Buffer == NULL)
			{
				return FALSE;
			}

			/* copy the string */
			RtlCopyMemory(NtPathName->Buffer,
			              us.Buffer,
			              NtPathName->Length);
			NtPathName->Buffer[us.Length / sizeof(WCHAR)] = L'\0';

			/* change the \\?\ prefix to \??\ */
			NtPathName->Buffer[1] = L'?';

			if (NtFileNamePart != NULL)
			{
				PWSTR FilePart = NULL;
				PWSTR s;

				/* try to find the last separator */
				s = NtPathName->Buffer + (NtPathName->Length / sizeof(WCHAR));
				while (s != NtPathName->Buffer)
				{
					if (*s == L'\\')
					{
						FilePart = s + 1;
						break;
					}
					s--;
				}

				*NtFileNamePart = FilePart;
			}

			if (DirectoryInfo != NULL)
			{
				DirectoryInfo->DosPath.Length = 0;
				DirectoryInfo->DosPath.MaximumLength = 0;
				DirectoryInfo->DosPath.Buffer = NULL;
				DirectoryInfo->Handle = NULL;
			}

			return TRUE;
		}
	}

	Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
	                          0,
	                          sizeof( fullname ) + MAX_PFX_SIZE);
	if (Buffer == NULL)
	{
		return FALSE;
	}

	RtlAcquirePebLock ();

	Size = RtlGetFullPathName_U (DosPathName,
	                             sizeof(fullname),
	                             fullname,
	                             (PWSTR*)NtFileNamePart);
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
	if (Type == RtlPathTypeDriveAbsolute ||
	    Type == RtlPathTypeDriveRelative)
	{
	    /* make the drive letter to uppercase */
	    Buffer[tmpLength] = towupper(Buffer[tmpLength]);
	}

	/* set NT filename */
	NtPathName->Length        = Length * sizeof(WCHAR);
	NtPathName->MaximumLength = sizeof(fullname) + MAX_PFX_SIZE;
	NtPathName->Buffer        = Buffer;

	/* set pointer to file part if possible */
	if (NtFileNamePart && *NtFileNamePart)
		*NtFileNamePart = Buffer + Length - wcslen (*NtFileNamePart);

	/* Set name and handle structure if possible */
	if (DirectoryInfo)
	{
		memset (DirectoryInfo, 0, sizeof(CURDIR));
		cd = (PCURDIR)&(NtCurrentPeb ()->ProcessParameters->CurrentDirectory.DosPath);
		if (Type == 5 && cd->Handle)
		{
		    RtlInitUnicodeString(&us, fullname);
		    if (RtlEqualUnicodeString(&us, &cd->DosPath, TRUE))
		    {
			Length = ((cd->DosPath.Length / sizeof(WCHAR)) - Offset) + ((Type == 1) ? 8 : 4);
			DirectoryInfo->DosPath.Buffer = Buffer + Length;
			DirectoryInfo->DosPath.Length = NtPathName->Length - (Length * sizeof(WCHAR));
			DirectoryInfo->DosPath.MaximumLength = DirectoryInfo->DosPath.Length;
			DirectoryInfo->Handle = cd->Handle;
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
NTAPI
RtlDosSearchPath_U (
	PCWSTR sp,
	PCWSTR name,
	PCWSTR ext,
	ULONG buf_sz,
	WCHAR *buffer,
	PWSTR *FilePart
	)
{
	ULONG Type;
	ULONG Length = 0;
	PWSTR full_name;
	PWSTR wcs;
	PCWSTR path;

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
BOOLEAN NTAPI
RtlDoesFileExists_U(IN PCWSTR FileName)
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

	Status = ZwQueryAttributesFile (&Attr, &Info);

   RtlFreeUnicodeString(&NtFileName);


	if (NT_SUCCESS(Status) ||
	    Status == STATUS_SHARING_VIOLATION ||
	    Status == STATUS_ACCESS_DENIED)
		return TRUE;

	return FALSE;
}


/*
 * @unimplemented
 */
BOOLEAN NTAPI
RtlDosPathNameToRelativeNtPathName_U(PVOID Unknown1,
                                     PVOID Unknown2,
                                     PVOID Unknown3,
                                     PVOID Unknown4)
{
    DPRINT1("RtlDosPathNameToRelativeNtPathName_U(0x%p, 0x%p, 0x%p, 0x%p) UNIMPLEMENTED!\n",
            Unknown1, Unknown2, Unknown3, Unknown4);
    return FALSE;
}


/*
 * @unimplemented
 */
VOID NTAPI
RtlReleaseRelativeName(PVOID Unknown)
{
    DPRINT1("RtlReleaseRelativeName(0x%p) UNIMPLEMENTED\n", Unknown);
}

NTSTATUS NTAPI
RtlpEnsureBufferSize(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3)
{
    DPRINT1("RtlpEnsureBufferSize: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

NTSTATUS NTAPI
RtlNtPathNameToDosPathName(ULONG Unknown1, ULONG Unknown2, ULONG Unknown3, ULONG Unknown4)
{
    DPRINT1("RtlNtPathNameToDosPathName: stub\n");
    return STATUS_NOT_IMPLEMENTED;
}

/* EOF */
