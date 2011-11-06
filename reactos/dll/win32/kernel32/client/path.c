/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/path.c
 * PURPOSE:         Handles path APIs
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

UNICODE_STRING BaseDllDirectory;
UNICODE_STRING NoDefaultCurrentDirectoryInExePath = RTL_CONSTANT_STRING(L"NoDefaultCurrentDirectoryInExePath");

/* This is bitmask for each illegal filename character */
/* If someone has time, please feel free to use 0b notation */
DWORD IllegalMask[4] =
{
    0xFFFFFFFF, // None allowed (00 to 1F)
    0xFC009C05, // 20, 22, 2A, 2B, 2C, 2F, 3A, 3B, 3C, 3D, 3E, 3F not allowed
    0x38000000, // 5B, 5C, 5D not allowed
    0x10000000  // 7C not allowed
};

/* FUNCTIONS ******************************************************************/

/*
 * Why not use RtlIsNameLegalDOS8Dot3? In fact the actual algorithm body is
 * identical (other than the Rtl can optionally check for spaces), however the
 * Rtl will always convert to OEM, while kernel32 has two possible file modes
 * (ANSI or OEM). Therefore we must duplicate the algorithm body to get
 * the correct compatible results
 */
BOOL
WINAPI
IsShortName_U(IN PWCHAR Name,
              IN ULONG Length)
{
    BOOLEAN HasExtension;
    WCHAR c;
    NTSTATUS Status;
    UNICODE_STRING UnicodeName;
    ANSI_STRING AnsiName;
    ULONG i, Dots;
    CHAR AnsiBuffer[MAX_PATH];
    ASSERT(Name);

    /* What do you think 8.3 means? */
    if (Length > 12) return FALSE;

    /* Sure, any emtpy name is a short name */
    if (!Length) return TRUE;

    /* This could be . or .. or somethign else */
    if (*Name == L'.')
    {
        /* Which one is it */
        if ((Length == 1) || ((Length == 2) && *(Name + 1) == L'.'))
        {
            /* . or .., this is good */
            return TRUE;
        }

        /* Some other bizare dot-based name, not good */
        return FALSE;
    }

    /* Initialize our two strings */
    RtlInitEmptyAnsiString(&AnsiName, AnsiBuffer, MAX_PATH);
    RtlInitEmptyUnicodeString(&UnicodeName, Name, Length * sizeof(WCHAR));
    UnicodeName.Length = UnicodeName.MaximumLength;

    /* Now do the conversion */
    Status = BasepUnicodeStringTo8BitString(&AnsiName, &UnicodeName, FALSE);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Now we loop the name */
    HasExtension = FALSE;
    for (i = 0, Dots = Length - 1; i < AnsiName.Length; i++, Dots--)
    {
        /* Read the current byte */
        c = AnsiName.Buffer[i];

        /* Is it DBCS? */
        if (IsDBCSLeadByte(c))
        {
            /* If we're near the end of the string, we can't allow a DBCS */
            if ((!(HasExtension) && (i >= 7)) || (i == AnsiName.Length - 1))
            {
                return FALSE;
            }

            /* Otherwise we skip over it */
            continue;
        }

        /* Check for illegal characters */
        if ((c > 0x7F) || (IllegalMask[c / 32] && (1 << (c % 32))))
        {
            return FALSE;
        }

        /* Check if this is perhaps an extension? */
        if (c == L'.')
        {
            /* Unless the extension is too large or there's more than one */
            if ((HasExtension) || (Dots > 3)) return FALSE;

            /* This looks like an extension */
            HasExtension = TRUE;
        }

        /* 8.3 length was validated, but now we must guard against 9.2 or similar */
        if ((i >= 8) && !(HasExtension)) return FALSE;
    }

    /* You survived the loop, this is a good short name */
    return TRUE;
}

BOOL
WINAPI
IsLongName_U(IN PWCHAR FileName,
             IN ULONG Length)
{
    BOOLEAN HasExtension;
    ULONG i, Dots;

    /* More than 8.3, any combination of dots, and NULL names are all long */
    if (!(Length) || (Length > 12) || (*FileName == L'.')) return TRUE;

    /* Otherwise, initialize our scanning loop */
    HasExtension = FALSE;
    for (i = 0, Dots = Length - 1; i < Length; i++, Dots--)
    {
        /* Check if this could be an extension */
        if (*FileName == '.')
        {
            /* Unlike the short case, we WANT more than one extension, or a long one */
            if ((HasExtension) || (Dots > 3)) return TRUE;
            HasExtension = TRUE;
        }

        /* Check if this would violate the "8" in 8.3, ie. 9.2 */
        if ((i >= 8) && (!HasExtension)) return TRUE;
    }

    /* The name *seems* to conform to 8.3 */
    return FALSE;
}

BOOL
WINAPI
FindLFNorSFN_U(IN PWCHAR Path,
               OUT PWCHAR *First,
               OUT PWCHAR *Last,
               IN BOOL UseShort)
{
    PWCHAR p;
    ULONG Length;
    BOOL Found = 0;
    ASSERT(Path);

    /* Loop while there is something in the path */
    while (TRUE)
    {
        /* Loop within the path skipping slashes */
        while ((*Path) && ((*Path == L'\\') || (*Path == L'/'))) Path++;

        /* Make sure there's something after the slashes too! */
        if (*Path == UNICODE_NULL) break;

        /* Now do the same thing with the last marker */
        p = Path + 1;
        while ((*p) && ((*p == L'\\') || (*p == L'/'))) p++;

        /* Whatever is in between those two is now the file name length */
        Length = p - Path;

        /*
         * Check if it is valid
         * Note that !IsShortName != IsLongName, these two functions simply help
         * us determine if a conversion is necessary or not.
         */
        Found = UseShort ? IsShortName_U(Path, Length) : IsLongName_U(Path, Length);

        /* "Found" really means: "Is a conversion necessary?", hence the ! */
        if (!Found)
        {
            /* It is! did the caller request to know the markers? */
            if ((First) && (Last))
            {
                /* Return them */
                *First = Path;
                *Last = p;
            }
            break;
        }

        /* Is there anything else following this sub-path/filename? */
        if (*p == UNICODE_NULL) break;

        /* Yes, keep going */
        Path = p + 1;
    }

    /* Return if anything was found and valid */
    return !Found;
}

PWCHAR
WINAPI
SkipPathTypeIndicator_U(IN PWCHAR Path)
{
    PWCHAR ReturnPath;
    ULONG i;

    /* Check what kind of path this is and how many slashes to skip */
    switch (RtlDetermineDosPathNameType_U(Path))
    {
        case RtlPathTypeDriveAbsolute:
            return Path + 3;

        case RtlPathTypeDriveRelative:
            return Path + 2;

        case RtlPathTypeRooted:
            return Path + 1;

        case RtlPathTypeRelative:
            return Path;

        case RtlPathTypeRootLocalDevice:
        default:
            return NULL;

        case RtlPathTypeUncAbsolute:
        case RtlPathTypeLocalDevice:

            /* Keep going until we bypass the path indicators */
            for (ReturnPath = Path + 2, i = 2; (i > 0) && (*ReturnPath); ReturnPath++)
            {
                /* We look for 2 slashes, so keep at it until we find them */
                if ((*ReturnPath == L'\\') || (*ReturnPath == L'/')) i--;
            }

            return ReturnPath;
    }
}

BOOL
WINAPI
BasepIsCurDirAllowedForPlainExeNames(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING EmptyString;

    RtlInitEmptyUnicodeString(&EmptyString, NULL, 0);
    Status = RtlQueryEnvironmentVariable_U(NULL,
                                           &NoDefaultCurrentDirectoryInExePath,
                                           &EmptyString);
    return !NT_SUCCESS(Status) && Status != STATUS_BUFFER_TOO_SMALL;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetDllDirectoryW(IN LPCWSTR lpPathName)
{
    UNICODE_STRING OldDirectory, DllDirectory;

    if (lpPathName)
    {
        if (wcschr(lpPathName, L';'))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        if (!RtlCreateUnicodeString(&DllDirectory, lpPathName))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }
    }
    else
    {
        RtlInitUnicodeString(&DllDirectory, NULL);
    }

    RtlEnterCriticalSection(&BaseDllDirectoryLock);

    OldDirectory = BaseDllDirectory;
    BaseDllDirectory = DllDirectory;

    RtlLeaveCriticalSection(&BaseDllDirectoryLock);

    RtlFreeUnicodeString(&OldDirectory);
    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetDllDirectoryA(IN LPCSTR lpPathName)
{
    ANSI_STRING AnsiDllDirectory;
    UNICODE_STRING OldDirectory, DllDirectory;
    NTSTATUS Status;

    if (lpPathName)
    {
        if (strchr(lpPathName, ';'))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }

        Status = RtlInitAnsiStringEx(&AnsiDllDirectory, lpPathName);
        if (NT_SUCCESS(Status))
        {
            Status = Basep8BitStringToUnicodeString(&DllDirectory,
                                                    &AnsiDllDirectory,
                                                    TRUE);
        }

        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(Status);
            return FALSE;
        }
    }
    else
    {
        RtlInitUnicodeString(&DllDirectory, NULL);
    }

    RtlEnterCriticalSection(&BaseDllDirectoryLock);

    OldDirectory = BaseDllDirectory;
    BaseDllDirectory = DllDirectory;

    RtlLeaveCriticalSection(&BaseDllDirectoryLock);

    RtlFreeUnicodeString(&OldDirectory);
    return TRUE;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetDllDirectoryW(IN DWORD nBufferLength,
                 OUT LPWSTR lpBuffer)
{
    ULONG Length;

    RtlEnterCriticalSection(&BaseDllDirectoryLock);

    if ((nBufferLength * sizeof(WCHAR)) > BaseDllDirectory.Length)
    {
        RtlCopyMemory(lpBuffer, BaseDllDirectory.Buffer, BaseDllDirectory.Length);
        Length = BaseDllDirectory.Length / sizeof(WCHAR);
        lpBuffer[Length] = UNICODE_NULL;
    }
    else
    {
        Length = (BaseDllDirectory.Length + sizeof(UNICODE_NULL)) / sizeof(WCHAR);
        if (lpBuffer) *lpBuffer = UNICODE_NULL;
    }

    RtlLeaveCriticalSection(&BaseDllDirectoryLock);
    return Length;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetDllDirectoryA(IN DWORD nBufferLength,
                 OUT LPSTR lpBuffer)
{
    NTSTATUS Status;
    ANSI_STRING AnsiDllDirectory;
    ULONG Length;

    RtlInitEmptyAnsiString(&AnsiDllDirectory, lpBuffer, nBufferLength);

    RtlEnterCriticalSection(&BaseDllDirectoryLock);

    Length = BasepUnicodeStringTo8BitSize(&BaseDllDirectory);
    if (Length > nBufferLength)
    {
        Status = STATUS_SUCCESS;
        if (lpBuffer) *lpBuffer = ANSI_NULL;
    }
    else
    {
        --Length;
        Status = BasepUnicodeStringTo8BitString(&AnsiDllDirectory,
                                                &BaseDllDirectory,
                                                FALSE);
    }

    RtlLeaveCriticalSection(&BaseDllDirectoryLock);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        Length = 0;
        if (lpBuffer) *lpBuffer = ANSI_NULL;
    }

    return Length;
}

/*
 * @implemented
 */
BOOL
WINAPI
NeedCurrentDirectoryForExePathW(IN LPCWSTR ExeName)
{
    if (wcschr(ExeName, L'\\')) return TRUE;

    return BasepIsCurDirAllowedForPlainExeNames();
}

/*
 * @implemented
 */
BOOL
WINAPI
NeedCurrentDirectoryForExePathA(IN LPCSTR ExeName)
{
    if (strchr(ExeName, '\\')) return TRUE;

    return BasepIsCurDirAllowedForPlainExeNames();
}

/*
 * @implemented
 */
DWORD
WINAPI
GetFullPathNameA (
        LPCSTR  lpFileName,
        DWORD   nBufferLength,
        LPSTR   lpBuffer,
        LPSTR   *lpFilePart
        )
{
   WCHAR BufferW[MAX_PATH];
   PWCHAR FileNameW;
   DWORD ret;
   LPWSTR FilePartW = NULL;

   DPRINT("GetFullPathNameA(lpFileName %s, nBufferLength %d, lpBuffer %p, "
        "lpFilePart %p)\n",lpFileName,nBufferLength,lpBuffer,lpFilePart);

   if (!(FileNameW = FilenameA2W(lpFileName, FALSE)))
      return 0;

   ret = GetFullPathNameW(FileNameW, MAX_PATH, BufferW, &FilePartW);

   if (!ret)
      return 0;

   if (ret > MAX_PATH)
   {
      SetLastError(ERROR_FILENAME_EXCED_RANGE);
      return 0;
   }

   ret = FilenameW2A_FitOrFail(lpBuffer, nBufferLength, BufferW, ret+1);

   if (ret < nBufferLength && lpFilePart)
   {
      /* if the path closed with '\', FilePart is NULL */
      if (!FilePartW)
         *lpFilePart=NULL;
      else
         *lpFilePart = (FilePartW - BufferW) + lpBuffer;
   }

   DPRINT("GetFullPathNameA ret: lpBuffer %s lpFilePart %s\n",
        lpBuffer, (lpFilePart == NULL) ? "NULL" : *lpFilePart);

   return ret;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetFullPathNameW(IN LPCWSTR lpFileName,
                 IN DWORD nBufferLength,
                 IN LPWSTR lpBuffer,
                 OUT LPWSTR *lpFilePart)
{
    return RtlGetFullPathName_U((LPWSTR)lpFileName,
                                nBufferLength * sizeof(WCHAR),
                                lpBuffer,
                                lpFilePart) / sizeof(WCHAR);
}

/*
 * @implemented
 */
DWORD
WINAPI
SearchPathA (
        LPCSTR  lpPath,
        LPCSTR  lpFileName,
        LPCSTR  lpExtension,
        DWORD   nBufferLength,
        LPSTR   lpBuffer,
        LPSTR   *lpFilePart
        )
{
        UNICODE_STRING PathU      = { 0, 0, NULL };
        UNICODE_STRING FileNameU  = { 0, 0, NULL };
        UNICODE_STRING ExtensionU = { 0, 0, NULL };
        UNICODE_STRING BufferU    = { 0, 0, NULL };
        ANSI_STRING Path;
        ANSI_STRING FileName;
        ANSI_STRING Extension;
        ANSI_STRING Buffer;
        PWCHAR FilePartW;
        DWORD RetValue = 0;
        NTSTATUS Status = STATUS_SUCCESS;

        if (!lpFileName)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return 0;
        }

        RtlInitAnsiString (&Path,
                           (LPSTR)lpPath);
        RtlInitAnsiString (&FileName,
                           (LPSTR)lpFileName);
        RtlInitAnsiString (&Extension,
                           (LPSTR)lpExtension);

        /* convert ansi (or oem) strings to unicode */
        if (bIsFileApiAnsi)
        {
                Status = RtlAnsiStringToUnicodeString (&PathU,
                                                       &Path,
                                                       TRUE);
                if (!NT_SUCCESS(Status))
                    goto Cleanup;

                Status = RtlAnsiStringToUnicodeString (&FileNameU,
                                                       &FileName,
                                                       TRUE);
                if (!NT_SUCCESS(Status))
                    goto Cleanup;

                Status = RtlAnsiStringToUnicodeString (&ExtensionU,
                                                       &Extension,
                                                       TRUE);
                if (!NT_SUCCESS(Status))
                    goto Cleanup;
        }
        else
        {
                Status = RtlOemStringToUnicodeString (&PathU,
                                                      &Path,
                                                      TRUE);
                if (!NT_SUCCESS(Status))
                    goto Cleanup;
                Status = RtlOemStringToUnicodeString (&FileNameU,
                                                      &FileName,
                                                      TRUE);
                if (!NT_SUCCESS(Status))
                    goto Cleanup;

                Status = RtlOemStringToUnicodeString (&ExtensionU,
                                                      &Extension,
                                                      TRUE);
                if (!NT_SUCCESS(Status))
                    goto Cleanup;
        }

        BufferU.MaximumLength = min(nBufferLength * sizeof(WCHAR), USHRT_MAX);
        BufferU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
                                          0,
                                          BufferU.MaximumLength);
        if (BufferU.Buffer == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        Buffer.MaximumLength = min(nBufferLength, USHRT_MAX);
        Buffer.Buffer = lpBuffer;

        RetValue = SearchPathW (NULL == lpPath ? NULL : PathU.Buffer,
                                NULL == lpFileName ? NULL : FileNameU.Buffer,
                                NULL == lpExtension ? NULL : ExtensionU.Buffer,
                                nBufferLength,
                                BufferU.Buffer,
                                &FilePartW);

        if (0 != RetValue)
        {
                BufferU.Length = wcslen(BufferU.Buffer) * sizeof(WCHAR);
                /* convert ansi (or oem) string to unicode */
                if (bIsFileApiAnsi)
                    Status = RtlUnicodeStringToAnsiString(&Buffer,
                                                          &BufferU,
                                                          FALSE);
                else
                    Status = RtlUnicodeStringToOemString(&Buffer,
                                                         &BufferU,
                                                         FALSE);

                if (NT_SUCCESS(Status) && Buffer.Buffer)
                {
                    /* nul-terminate ascii string */
                    Buffer.Buffer[BufferU.Length / sizeof(WCHAR)] = '\0';

                    if (NULL != lpFilePart && BufferU.Length != 0)
                    {
                        *lpFilePart = strrchr (lpBuffer, '\\') + 1;
                    }
                }
        }

Cleanup:
        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     PathU.Buffer);
        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     FileNameU.Buffer);
        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     ExtensionU.Buffer);
        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     BufferU.Buffer);

        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(Status);
            return 0;
        }

        return RetValue;
}


/***********************************************************************
 *           ContainsPath (Wine name: contains_pathW)
 *
 * Check if the file name contains a path; helper for SearchPathW.
 * A relative path is not considered a path unless it starts with ./ or ../
 */
static
BOOL
ContainsPath(LPCWSTR name)
{
    if (RtlDetermineDosPathNameType_U(name) != RtlPathTypeRelative) return TRUE;
    if (name[0] != '.') return FALSE;
    if (name[1] == '/' || name[1] == '\\' || name[1] == '\0') return TRUE;
    return (name[1] == '.' && (name[2] == '/' || name[2] == '\\'));
}


/*
 * @implemented
 */
DWORD
WINAPI
SearchPathW(LPCWSTR lpPath,
            LPCWSTR lpFileName,
            LPCWSTR lpExtension,
            DWORD nBufferLength,
            LPWSTR lpBuffer,
            LPWSTR *lpFilePart)
{
    DWORD ret = 0;

    if (!lpFileName || !lpFileName[0])
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* If the name contains an explicit path, ignore the path */
    if (ContainsPath(lpFileName))
    {
        /* try first without extension */
        if (RtlDoesFileExists_U(lpFileName))
            return GetFullPathNameW(lpFileName, nBufferLength, lpBuffer, lpFilePart);

        if (lpExtension)
        {
            LPCWSTR p = wcsrchr(lpFileName, '.');
            if (p && !strchr((const char *)p, '/') && !wcschr( p, '\\' ))
                lpExtension = NULL;  /* Ignore the specified extension */
        }

        /* Allocate a buffer for the file name and extension */
        if (lpExtension)
        {
            LPWSTR tmp;
            DWORD len = wcslen(lpFileName) + wcslen(lpExtension);

            if (!(tmp = RtlAllocateHeap(RtlGetProcessHeap(), 0, (len + 1) * sizeof(WCHAR))))
            {
                SetLastError(ERROR_OUTOFMEMORY);
                return 0;
            }
            wcscpy(tmp, lpFileName);
            wcscat(tmp, lpExtension);
            if (RtlDoesFileExists_U(tmp))
                ret = GetFullPathNameW(tmp, nBufferLength, lpBuffer, lpFilePart);
            RtlFreeHeap(RtlGetProcessHeap(), 0, tmp);
        }
    }
    else if (lpPath && lpPath[0])  /* search in the specified path */
    {
        ret = RtlDosSearchPath_U(lpPath,
                                 lpFileName,
                                 lpExtension,
                                 nBufferLength * sizeof(WCHAR),
                                 lpBuffer,
                                 lpFilePart) / sizeof(WCHAR);
    }
    else  /* search in the default path */
    {
        WCHAR *DllPath = GetDllLoadPath(NULL);

        if (DllPath)
        {
            ret = RtlDosSearchPath_U(DllPath,
                                     lpFileName,
                                     lpExtension,
                                     nBufferLength * sizeof(WCHAR),
                                     lpBuffer,
                                     lpFilePart) / sizeof(WCHAR);
            RtlFreeHeap(RtlGetProcessHeap(), 0, DllPath);
        }
        else
        {
            SetLastError(ERROR_OUTOFMEMORY);
            return 0;
        }
    }

    if (!ret) SetLastError(ERROR_FILE_NOT_FOUND);

    return ret;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetLongPathNameW(IN LPCWSTR lpszShortPath,
                 IN LPWSTR lpszLongPath,
                 IN DWORD cchBuffer)
{
    PWCHAR Path, Original, First, Last, Buffer, Src, Dst;
    ULONG Length;
    WCHAR LastChar;
    HANDLE FindHandle;
    DWORD ReturnLength;
    ULONG ErrorMode;
    BOOLEAN Found;
    WIN32_FIND_DATAW FindFileData;

    /* Initialize so Quickie knows there's nothing to do */
    Buffer = Original = NULL;
    ReturnLength = 0;

    /* First check if the input path was obviously NULL */
    if (!lpszShortPath)
    {
        /* Fail the request */
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* We will be touching removed, removable drives -- don't warn the user */
    ErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

    /* Do a simple check to see if the path exists */
    if (GetFileAttributesW(lpszShortPath) == 0xFFFFFFF)
    {
        /* It doesn't, so fail */
        ReturnLength = 0;
        goto Quickie;
    }

    /* Now get a pointer to the actual path, skipping indicators */
    Path = SkipPathTypeIndicator_U((PWCHAR)lpszShortPath);

    /* Try to find a file name in there */
    if (Path) Found = FindLFNorSFN_U(Path, &First, &Last, FALSE);

    /* Is there any path or filename in there? */
    if (!(Path) || (*Path == UNICODE_NULL) || !(Found))
    {
        /* There isn't, so the long path is simply the short path */
        ReturnLength = wcslen(lpszShortPath);

        /* Is there space for it? */
        if ((cchBuffer > ReturnLength) && (lpszLongPath))
        {
            /* Make sure the pointers aren't already the same */
            if (lpszLongPath != lpszShortPath)
            {
                /* They're not -- copy the short path into the long path */
                RtlMoveMemory(lpszLongPath,
                              lpszShortPath,
                              ReturnLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
            }
        }
        else
        {
            /* Otherwise, let caller know we need a bigger buffer, include NULL */
            ReturnLength++;
        }
        goto Quickie;
    }

    /* We are still in the game -- compute the current size */
    Length = wcslen(lpszShortPath) + sizeof(ANSI_NULL);
    Original = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length * sizeof(WCHAR));
    if (!Original) goto ErrorQuickie;

    /* Make a copy ofi t */
    RtlMoveMemory(Original, lpszShortPath, Length * sizeof(WCHAR));

    /* Compute the new first and last markers */
    First = &Original[First - lpszShortPath];
    Last = &Original[Last - lpszShortPath];

    /* Set the current destination pointer for a copy */
    Dst = lpszLongPath;

    /*
     * Windows allows the paths to overlap -- we have to be careful with this and
     * see if it's same to do so, and if not, allocate our own internal buffer
     * that we'll return at the end.
     *
     * This is also why we use RtlMoveMemory everywhere. Don't use RtlCopyMemory!
     */
    if ((cchBuffer) && (lpszLongPath) &&
        (((lpszLongPath >= lpszShortPath) && (lpszLongPath < &lpszShortPath[Length])) ||
         ((lpszLongPath < lpszShortPath) && (&lpszLongPath[cchBuffer] >= lpszShortPath))))
    {
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, cchBuffer * sizeof(WCHAR));
        if (!Buffer) goto ErrorQuickie;

        /* New destination */
        Dst = Buffer;
    }

    /* Prepare for the loop */
    Src = Original;
    ReturnLength = 0;
    while (TRUE)
    {
        /* Current delta in the loop */
        Length = First - Src;

        /* Update the return length by it */
        ReturnLength += Length;

        /* Is there a delta? If so, is there space and buffer for it? */
        if ((Length) && (cchBuffer > ReturnLength) && (lpszLongPath))
        {
            RtlMoveMemory(Dst, Src, Length * sizeof(WCHAR));
            Dst += Length;
        }

        /* "Terminate" this portion of the path's substring so we can do a find */
        LastChar = *Last;
        *Last = UNICODE_NULL;
        FindHandle = FindFirstFileW(Original, &FindFileData);
        *Last = LastChar;

        /* This portion wasn't found, so fail */
        if (FindHandle == INVALID_HANDLE_VALUE)
        {
            ReturnLength = 0;
            break;
        }

        /* Close the find handle */
        FindClose(FindHandle);

        /* Now check the length of the long name */
        Length = wcslen(FindFileData.cFileName);
        if (Length)
        {
            /* This is our new first marker */
            First = FindFileData.cFileName;
        }
        else
        {
            /* Otherwise, the name is the delta between our current markers */
            Length = Last - First;
        }

        /* Update the return length with the short name length, if any */
        ReturnLength += Length;

        /* Once again check for appropriate space and buffer */
        if ((cchBuffer > ReturnLength) && (lpszLongPath))
        {
            /* And do the copy if there is */
            RtlMoveMemory(Dst, First, Length * sizeof(WCHAR));
            Dst += Length;
        }

        /* Now update the source pointer */
        Src = Last;
        if (*Src == UNICODE_NULL) break;

        /* Are there more names in there? */
        Found = FindLFNorSFN_U(Src, &First, &Last, FALSE);
        if (!Found) break;
    }

    /* The loop is done, is there anything left? */
    if (ReturnLength)
    {
        /* Get the length of the straggling path */
        Length = wcslen(Src);
        ReturnLength += Length;

        /* Once again check for appropriate space and buffer */
        if ((cchBuffer > ReturnLength) && (lpszLongPath))
        {
            /* And do the copy if there is -- accounting for NULL here */
            RtlMoveMemory(Dst, Src, Length * sizeof(WCHAR) + sizeof(UNICODE_NULL));

            /* What about our buffer? */
            if (Buffer)
            {
                /* Copy it into the caller's long path */
                RtlMoveMemory(lpszLongPath,
                              Buffer,
                              ReturnLength * sizeof(WCHAR)  + sizeof(UNICODE_NULL));
            }
        }
        else
        {
            /* Buffer is too small, let the caller know, making space for NULL */
            ReturnLength++;
        }
    }

    /* We're all done */
    goto Quickie;

ErrorQuickie:
    /* This is the goto for memory failures */
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);

Quickie:
    /* General function end: free memory, restore error mode, return length */
    if (Original) RtlFreeHeap(RtlGetProcessHeap(), 0, Original);
    if (Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    SetErrorMode(ErrorMode);
    return ReturnLength;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetLongPathNameA(IN LPCSTR lpszShortPath,
                 IN LPSTR lpszLongPath,
                 IN DWORD cchBuffer)
{
    ULONG Result, PathLength;
    PWCHAR LongPath;
    NTSTATUS Status;
    UNICODE_STRING LongPathUni, ShortPathUni;
    ANSI_STRING LongPathAnsi;
    WCHAR LongPathBuffer[MAX_PATH];

    LongPath = NULL;
    LongPathAnsi.Buffer = NULL;
    ShortPathUni.Buffer = NULL;
    Result = 0;

    if (!lpszShortPath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    Status = Basep8BitStringToDynamicUnicodeString(&ShortPathUni, lpszShortPath);
    if (!NT_SUCCESS(Status)) goto Quickie;

    LongPath = LongPathBuffer;

    PathLength = GetLongPathNameW(ShortPathUni.Buffer, LongPathBuffer, MAX_PATH);
    if (PathLength >= MAX_PATH)
    {
        LongPath = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathLength * sizeof(WCHAR));
        if (!LongPath)
        {
            PathLength = 0;
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
        else
        {
            PathLength = GetLongPathNameW(ShortPathUni.Buffer, LongPath, PathLength);
        }
    }

    if (!PathLength) goto Quickie;

    ShortPathUni.MaximumLength = PathLength * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    LongPathUni.Buffer = LongPath;
    LongPathUni.Length = PathLength * sizeof(WCHAR);

    Status = BasepUnicodeStringTo8BitString(&LongPathAnsi, &LongPathUni, TRUE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        Result = 0;
    }

    Result = LongPathAnsi.Length;
    if ((lpszLongPath) && (cchBuffer > LongPathAnsi.Length))
    {
        RtlMoveMemory(lpszLongPath, LongPathAnsi.Buffer, LongPathAnsi.Length);
        lpszLongPath[Result] = ANSI_NULL;
    }
    else
    {
        Result = LongPathAnsi.Length + sizeof(ANSI_NULL);
    }

Quickie:
    if (ShortPathUni.Buffer) RtlFreeUnicodeString(&ShortPathUni);
    if (LongPathAnsi.Buffer) RtlFreeAnsiString(&LongPathAnsi);
    if ((LongPath) && (LongPath != LongPathBuffer))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, LongPath);
    }
    return Result;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetShortPathNameA(IN LPCSTR lpszLongPath,
                  IN LPSTR lpszShortPath,
                  IN DWORD cchBuffer)
{
    ULONG Result, PathLength;
    PWCHAR ShortPath;
    NTSTATUS Status;
    UNICODE_STRING LongPathUni, ShortPathUni;
    ANSI_STRING ShortPathAnsi;
    WCHAR ShortPathBuffer[MAX_PATH];

    ShortPath = NULL;
    ShortPathAnsi.Buffer = NULL;
    LongPathUni.Buffer = NULL;
    Result = 0;

    if (!lpszLongPath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    Status = Basep8BitStringToDynamicUnicodeString(&LongPathUni, lpszLongPath);
    if (!NT_SUCCESS(Status)) goto Quickie;

    ShortPath = ShortPathBuffer;

    PathLength = GetShortPathNameW(LongPathUni.Buffer, ShortPathBuffer, MAX_PATH);
    if (PathLength >= MAX_PATH)
    {
        ShortPath = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathLength * sizeof(WCHAR));
        if (!ShortPath)
        {
            PathLength = 0;
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        }
        else
        {
            PathLength = GetLongPathNameW(LongPathUni.Buffer, ShortPath, PathLength);
        }
    }

    if (!PathLength) goto Quickie;

    LongPathUni.MaximumLength = PathLength * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    ShortPathUni.Buffer = ShortPath;
    ShortPathUni.Length = PathLength * sizeof(WCHAR);

    Status = BasepUnicodeStringTo8BitString(&ShortPathAnsi, &ShortPathUni, TRUE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        Result = 0;
    }

    Result = ShortPathAnsi.Length;
    if ((lpszShortPath) && (cchBuffer > ShortPathAnsi.Length))
    {
        RtlMoveMemory(lpszShortPath, ShortPathAnsi.Buffer, ShortPathAnsi.Length);
        lpszShortPath[Result] = ANSI_NULL;
    }
    else
    {
        Result = ShortPathAnsi.Length + sizeof(ANSI_NULL);
    }

Quickie:
    if (LongPathUni.Buffer) RtlFreeUnicodeString(&LongPathUni);
    if (ShortPathAnsi.Buffer) RtlFreeAnsiString(&ShortPathAnsi);
    if ((ShortPath) && (ShortPath != ShortPathBuffer))
    {
        RtlFreeHeap(RtlGetProcessHeap(), 0, ShortPath);
    }
    return Result;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetShortPathNameW(IN LPCWSTR lpszLongPath,
                  IN LPWSTR lpszShortPath,
                  IN DWORD cchBuffer)
{
    PWCHAR Path, Original, First, Last, Buffer, Src, Dst;
    ULONG Length;
    WCHAR LastChar;
    HANDLE FindHandle;
    DWORD ReturnLength;
    ULONG ErrorMode;
    BOOLEAN Found;
    WIN32_FIND_DATAW FindFileData;

    /* Initialize so Quickie knows there's nothing to do */
    Buffer = Original = NULL;
    ReturnLength = 0;

    /* First check if the input path was obviously NULL */
    if (!lpszLongPath)
    {
        /* Fail the request */
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    /* We will be touching removed, removable drives -- don't warn the user */
    ErrorMode = SetErrorMode(SEM_NOOPENFILEERRORBOX | SEM_FAILCRITICALERRORS);

    /* Do a simple check to see if the path exists */
    if (GetFileAttributesW(lpszShortPath) == 0xFFFFFFF)
    {
        /* Windows checks for an application compatibility flag to allow this */
        if (!(NtCurrentPeb()) || !(NtCurrentPeb()->AppCompatFlags.LowPart & 1))
        {
            /* It doesn't, so fail */
            ReturnLength = 0;
            goto Quickie;
        }
    }

    /* Now get a pointer to the actual path, skipping indicators */
    Path = SkipPathTypeIndicator_U((PWCHAR)lpszShortPath);

    /* Try to find a file name in there */
    if (Path) Found = FindLFNorSFN_U(Path, &First, &Last, TRUE);

    /* Is there any path or filename in there? */
    if (!(Path) || (*Path == UNICODE_NULL) || !(Found))
    {
        /* There isn't, so the long path is simply the short path */
        ReturnLength = wcslen(lpszLongPath);

        /* Is there space for it? */
        if ((cchBuffer > ReturnLength) && (lpszLongPath))
        {
            /* Make sure the pointers aren't already the same */
            if (lpszLongPath != lpszShortPath)
            {
                /* They're not -- copy the short path into the long path */
                RtlMoveMemory(lpszShortPath,
                              lpszLongPath,
                              ReturnLength * sizeof(WCHAR) + sizeof(UNICODE_NULL));
            }
        }
        else
        {
            /* Otherwise, let caller know we need a bigger buffer, include NULL */
            ReturnLength++;
        }
        goto Quickie;
    }

    /* We are still in the game -- compute the current size */
    Length = wcslen(lpszLongPath) + sizeof(ANSI_NULL);
    Original = RtlAllocateHeap(RtlGetProcessHeap(), 0, Length * sizeof(WCHAR));
    if (!Original) goto ErrorQuickie;

    /* Make a copy of it */
    wcsncpy(Original, lpszLongPath, Length);

    /* Compute the new first and last markers */
    First = &Original[First - lpszLongPath];
    Last = &Original[Last - lpszLongPath];

    /* Set the current destination pointer for a copy */
    Dst = lpszShortPath;

    /*
     * Windows allows the paths to overlap -- we have to be careful with this and
     * see if it's same to do so, and if not, allocate our own internal buffer
     * that we'll return at the end.
     *
     * This is also why we use RtlMoveMemory everywhere. Don't use RtlCopyMemory!
     */
    if ((cchBuffer) && (lpszShortPath) &&
        (((lpszShortPath >= lpszLongPath) && (lpszShortPath < &lpszLongPath[Length])) ||
         ((lpszShortPath < lpszLongPath) && (&lpszShortPath[cchBuffer] >= lpszLongPath))))
    {
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, cchBuffer * sizeof(WCHAR));
        if (!Buffer) goto ErrorQuickie;

        /* New destination */
        Dst = Buffer;
    }

    /* Prepare for the loop */
    Src = Original;
    ReturnLength = 0;
    while (TRUE)
    {
        /* Current delta in the loop */
        Length = First - Src;

        /* Update the return length by it */
        ReturnLength += Length;

        /* Is there a delta? If so, is there space and buffer for it? */
        if ((Length) && (cchBuffer > ReturnLength) && (lpszShortPath))
        {
            RtlMoveMemory(Dst, Src, Length * sizeof(WCHAR));
            Dst += Length;
        }

        /* "Terminate" this portion of the path's substring so we can do a find */
        LastChar = *Last;
        *Last = UNICODE_NULL;
        FindHandle = FindFirstFileW(Original, &FindFileData);
        *Last = LastChar;

        /* This portion wasn't found, so fail */
        if (FindHandle == INVALID_HANDLE_VALUE)
        {
            ReturnLength = 0;
            break;
        }

        /* Close the find handle */
        FindClose(FindHandle);

        /* Now check the length of the short name */
        Length = wcslen(FindFileData.cAlternateFileName);
        if (Length)
        {
            /* This is our new first marker */
            First = FindFileData.cAlternateFileName;
        }
        else
        {
            /* Otherwise, the name is the delta between our current markers */
            Length = Last - First;
        }

        /* Update the return length with the short name length, if any */
        ReturnLength += Length;

        /* Once again check for appropriate space and buffer */
        if ((cchBuffer > ReturnLength) && (lpszShortPath))
        {
            /* And do the copy if there is */
            RtlMoveMemory(Dst, First, Length * sizeof(WCHAR));
            Dst += Length;
        }

        /* Now update the source pointer */
        Src = Last;
        if (*Src == UNICODE_NULL) break;

        /* Are there more names in there? */
        Found = FindLFNorSFN_U(Src, &First, &Last, TRUE);
        if (!Found) break;
    }

    /* The loop is done, is there anything left? */
    if (ReturnLength)
    {
        /* Get the length of the straggling path */
        Length = wcslen(Src);
        ReturnLength += Length;

        /* Once again check for appropriate space and buffer */
        if ((cchBuffer > ReturnLength) && (lpszShortPath))
        {
            /* And do the copy if there is -- accounting for NULL here */
            RtlMoveMemory(Dst, Src, Length * sizeof(WCHAR) + sizeof(UNICODE_NULL));

            /* What about our buffer? */
            if (Buffer)
            {
                /* Copy it into the caller's long path */
                RtlMoveMemory(lpszShortPath,
                              Buffer,
                              ReturnLength * sizeof(WCHAR)  + sizeof(UNICODE_NULL));
            }
        }
        else
        {
            /* Buffer is too small, let the caller know, making space for NULL */
            ReturnLength++;
        }
    }

    /* We're all done */
    goto Quickie;

ErrorQuickie:
    /* This is the goto for memory failures */
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);

Quickie:
    /* General function end: free memory, restore error mode, return length */
    if (Original) RtlFreeHeap(RtlGetProcessHeap(), 0, Original);
    if (Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    SetErrorMode(ErrorMode);
    return ReturnLength;
}

/* EOF */
