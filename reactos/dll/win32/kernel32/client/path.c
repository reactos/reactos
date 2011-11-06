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

/* FUNCTIONS ******************************************************************/

BOOL
WINAPI
BasepIsCurDirAllowedForPlainExeNames(VOID)
{
    NTSTATUS Status;
    UNICODE_STRING EmptyString;

    RtlInitEmptyUnicodeString(&EmptyString, NULL, 0);
    Status = RtlQueryEnvironmentVariable_U(0,
                                           &NoDefaultCurrentDirectoryInExePath,
                                           &EmptyString);
    return NT_SUCCESS(Status);
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
        if (wcschr(lpPathName, ';'))
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return FALSE;
        }
        if (!RtlCreateUnicodeString(&DllDirectory, lpPathName))
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return 0;
        }
    }
    else
    {
        RtlInitUnicodeString(&DllDirectory, 0);
    }

    RtlEnterCriticalSection(&BaseDllDirectoryLock);

    OldDirectory = BaseDllDirectory;
    BaseDllDirectory = DllDirectory;

    RtlLeaveCriticalSection(&BaseDllDirectoryLock);

    RtlFreeUnicodeString(&OldDirectory);
    return 1;
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
            return 0;
        }
    }
    else
    {
        RtlInitUnicodeString(&DllDirectory, 0);
    }

    RtlEnterCriticalSection(&BaseDllDirectoryLock);

    OldDirectory = BaseDllDirectory;
    BaseDllDirectory = DllDirectory;

    RtlLeaveCriticalSection(&BaseDllDirectoryLock);

    RtlFreeUnicodeString(&OldDirectory);
    return 1;
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
        Length = BaseDllDirectory.Length  / sizeof(WCHAR);
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

    RtlInitEmptyAnsiString(&AnsiDllDirectory, lpBuffer, 0);

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
    if (wcschr(ExeName, '\\')) return TRUE;

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

/***********************************************************************
 * @implemented
 *
 *           GetLongPathNameW   (KERNEL32.@)
 *
 * NOTES
 *  observed (Win2000):
 *  shortpath=NULL: LastError=ERROR_INVALID_PARAMETER, ret=0
 *  shortpath="":   LastError=ERROR_PATH_NOT_FOUND, ret=0
 */
DWORD WINAPI GetLongPathNameW( LPCWSTR shortpath, LPWSTR longpath, DWORD longlen )
{
#define    MAX_PATHNAME_LEN 1024

    WCHAR               tmplongpath[MAX_PATHNAME_LEN];
    LPCWSTR             p;
    DWORD               sp = 0, lp = 0;
    DWORD               tmplen;
    BOOL                unixabsolute;
    WIN32_FIND_DATAW    wfd;
    HANDLE              goit;

    if (!shortpath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!shortpath[0])
    {
        SetLastError(ERROR_PATH_NOT_FOUND);
        return 0;
    }

    DPRINT("GetLongPathNameW(%s,%p,%ld)\n", shortpath, longpath, longlen);

    if (shortpath[0] == '\\' && shortpath[1] == '\\')
    {
        DPRINT1("ERR: UNC pathname %s\n", shortpath);
        lstrcpynW( longpath, shortpath, longlen );
        return wcslen(longpath);
    }
    unixabsolute = (shortpath[0] == '/');
    /* check for drive letter */
    if (!unixabsolute && shortpath[1] == ':' )
    {
        tmplongpath[0] = shortpath[0];
        tmplongpath[1] = ':';
        lp = sp = 2;
    }

    while (shortpath[sp])
    {
        /* check for path delimiters and reproduce them */
        if (shortpath[sp] == '\\' || shortpath[sp] == '/')
        {
            if (!lp || tmplongpath[lp-1] != '\\')
            {
                /* strip double "\\" */
                tmplongpath[lp++] = '\\';
            }
            tmplongpath[lp] = 0; /* terminate string */
            sp++;
            continue;
        }

        p = shortpath + sp;
        if (sp == 0 && p[0] == '.' && (p[1] == '/' || p[1] == '\\'))
        {
            tmplongpath[lp++] = *p++;
            tmplongpath[lp++] = *p++;
        }
        for (; *p && *p != '/' && *p != '\\'; p++);
        tmplen = p - (shortpath + sp);
        lstrcpynW(tmplongpath + lp, shortpath + sp, tmplen + 1);
        /* Check if the file exists and use the existing file name */
        goit = FindFirstFileW(tmplongpath, &wfd);
        if (goit == INVALID_HANDLE_VALUE)
        {
            DPRINT("not found %s!\n", tmplongpath);
            SetLastError ( ERROR_FILE_NOT_FOUND );
            return 0;
        }
        FindClose(goit);
        wcscpy(tmplongpath + lp, wfd.cFileName);
        lp += wcslen(tmplongpath + lp);
        sp += tmplen;
    }
    tmplen = wcslen(shortpath) - 1;
    if ((shortpath[tmplen] == '/' || shortpath[tmplen] == '\\') &&
        (tmplongpath[lp - 1] != '/' && tmplongpath[lp - 1] != '\\'))
        tmplongpath[lp++] = shortpath[tmplen];
    tmplongpath[lp] = 0;

    tmplen = wcslen(tmplongpath) + 1;
    if (tmplen <= longlen)
    {
        wcscpy(longpath, tmplongpath);
        DPRINT("returning %s\n", longpath);
        tmplen--; /* length without 0 */
    }

    return tmplen;
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
 * NOTE: Copied from Wine.
 * @implemented
 */
DWORD
WINAPI
GetShortPathNameW (
        LPCWSTR longpath,
        LPWSTR  shortpath,
        DWORD   shortlen
        )
{
    WCHAR               tmpshortpath[MAX_PATH];
    LPCWSTR             p;
    DWORD               sp = 0, lp = 0;
    DWORD               tmplen;
    WIN32_FIND_DATAW    wfd;
    HANDLE              goit;
    UNICODE_STRING      ustr;
    WCHAR               ustr_buf[8+1+3+1];

   DPRINT("GetShortPathNameW: %S\n",longpath);

    if (!longpath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }
    if (!longpath[0])
    {
        SetLastError(ERROR_BAD_PATHNAME);
        return 0;
    }

    /* check for drive letter */
    if (longpath[0] != '/' && longpath[1] == ':' )
    {
        tmpshortpath[0] = longpath[0];
        tmpshortpath[1] = ':';
        sp = lp = 2;
    }

    ustr.Buffer = ustr_buf;
    ustr.Length = 0;
    ustr.MaximumLength = sizeof(ustr_buf);

    while (longpath[lp])
    {
        /* check for path delimiters and reproduce them */
        if (longpath[lp] == '\\' || longpath[lp] == '/')
        {
            if (!sp || tmpshortpath[sp-1] != '\\')
            {
                /* strip double "\\" */
                tmpshortpath[sp] = '\\';
                sp++;
            }
            tmpshortpath[sp] = 0; /* terminate string */
            lp++;
            continue;
        }

        for (p = longpath + lp; *p && *p != '/' && *p != '\\'; p++);
        tmplen = p - (longpath + lp);
        lstrcpynW(tmpshortpath + sp, longpath + lp, tmplen + 1);
        /* Check, if the current element is a valid dos name */
        if (tmplen <= 8+1+3)
        {
            BOOLEAN spaces;
            memcpy(ustr_buf, longpath + lp, tmplen * sizeof(WCHAR));
            ustr_buf[tmplen] = '\0';
            ustr.Length = (USHORT)tmplen * sizeof(WCHAR);
            if (RtlIsNameLegalDOS8Dot3(&ustr, NULL, &spaces) && !spaces)
            {
                sp += tmplen;
                lp += tmplen;
                continue;
            }
        }

        /* Check if the file exists and use the existing short file name */
        goit = FindFirstFileW(tmpshortpath, &wfd);
        if (goit == INVALID_HANDLE_VALUE) goto notfound;
        FindClose(goit);
        lstrcpyW(tmpshortpath + sp, wfd.cAlternateFileName);
        sp += lstrlenW(tmpshortpath + sp);
        lp += tmplen;
    }
    tmpshortpath[sp] = 0;

    tmplen = lstrlenW(tmpshortpath) + 1;
    if (tmplen <= shortlen)
    {
        lstrcpyW(shortpath, tmpshortpath);
        tmplen--; /* length without 0 */
    }

    return tmplen;

 notfound:
    SetLastError ( ERROR_FILE_NOT_FOUND );
    return 0;
}

/* EOF */
