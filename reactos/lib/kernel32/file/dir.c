/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/file/dir.c
 * PURPOSE:         Directory functions
 * PROGRAMMER:      Ariadne ( ariadne@xs4all.nl)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/*
 * NOTES: Changed to using ZwCreateFile
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

UNICODE_STRING DllDirectory = {0, 0, NULL};

/* FUNCTIONS *****************************************************************/

/*
 * @implemented
 */
BOOL
STDCALL
CreateDirectoryA (
        LPCSTR                  lpPathName,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes
        )
{
   PWCHAR PathNameW;

   if (!(PathNameW = FilenameA2W(lpPathName, FALSE)))
      return FALSE;

   return CreateDirectoryW (PathNameW,
                            lpSecurityAttributes);
}


/*
 * @implemented
 */
BOOL
STDCALL
CreateDirectoryExA (
        LPCSTR                  lpTemplateDirectory,
        LPCSTR                  lpNewDirectory,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes)
{
   PWCHAR TemplateDirectoryW;
   PWCHAR NewDirectoryW;
   BOOL ret;

   if (!(TemplateDirectoryW = FilenameA2W(lpTemplateDirectory, TRUE)))
      return FALSE;

   if (!(NewDirectoryW = FilenameA2W(lpNewDirectory, FALSE)))
   {
      RtlFreeHeap (RtlGetProcessHeap (),
                   0,
                   TemplateDirectoryW);
      return FALSE;
   }

   ret = CreateDirectoryExW (TemplateDirectoryW,
                             NewDirectoryW,
                             lpSecurityAttributes);

   RtlFreeHeap (RtlGetProcessHeap (),
                0,
                TemplateDirectoryW);

   return ret;
}


/*
 * @implemented
 */
BOOL
STDCALL
CreateDirectoryW (
        LPCWSTR                 lpPathName,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes
        )
{
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK IoStatusBlock;
        UNICODE_STRING NtPathU;
        HANDLE DirectoryHandle;
        NTSTATUS Status;

        DPRINT ("lpPathName %S lpSecurityAttributes %p\n",
                lpPathName, lpSecurityAttributes);

        if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpPathName,
                                           &NtPathU,
                                           NULL,
                                           NULL))
        {
                SetLastError(ERROR_PATH_NOT_FOUND);
                return FALSE;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &NtPathU,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   (lpSecurityAttributes ? lpSecurityAttributes->lpSecurityDescriptor : NULL));

        Status = NtCreateFile (&DirectoryHandle,
                               GENERIC_READ,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               FILE_CREATE,
                               FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                               NULL,
                               0);

        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     NtPathU.Buffer);

        if (!NT_SUCCESS(Status))
        {
                SetLastErrorByStatus(Status);
                return FALSE;
        }

        NtClose (DirectoryHandle);

        return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
CreateDirectoryExW (
        LPCWSTR                 lpTemplateDirectory,
        LPCWSTR                 lpNewDirectory,
        LPSECURITY_ATTRIBUTES   lpSecurityAttributes
        )
{
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK IoStatusBlock;
        UNICODE_STRING NtPathU, NtTemplatePathU;
        HANDLE DirectoryHandle, TemplateHandle;
        FILE_EA_INFORMATION EaInformation;
        NTSTATUS Status;
        PVOID EaBuffer = NULL;
        ULONG EaLength = 0;

        DPRINT ("lpTemplateDirectory %S lpNewDirectory %S lpSecurityAttributes %p\n",
                lpTemplateDirectory, lpNewDirectory, lpSecurityAttributes);

        /*
         * Read the extended attributes from the template directory
         */

        if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpTemplateDirectory,
                                           &NtTemplatePathU,
                                           NULL,
                                           NULL))
        {
                SetLastError(ERROR_PATH_NOT_FOUND);
                return FALSE;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &NtTemplatePathU,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        Status = NtCreateFile (&TemplateHandle,
                               GENERIC_READ,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               FILE_OPEN,
                               FILE_DIRECTORY_FILE,
                               NULL,
                               0);
        if (!NT_SUCCESS(Status))
        {
                RtlFreeHeap (RtlGetProcessHeap (),
                             0,
                             NtTemplatePathU.Buffer);
                SetLastErrorByStatus (Status);
                return FALSE;
        }

        for (;;)
        {
          Status = NtQueryInformationFile(TemplateHandle,
                                          &IoStatusBlock,
                                          &EaInformation,
                                          sizeof(FILE_EA_INFORMATION),
                                          FileEaInformation);
          if (NT_SUCCESS(Status) && (EaInformation.EaSize != 0))
          {
            EaBuffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                       0,
                                       EaInformation.EaSize);
            if (EaBuffer == NULL)
            {
               Status = STATUS_INSUFFICIENT_RESOURCES;
               break;
            }

            Status = NtQueryEaFile(TemplateHandle,
                                   &IoStatusBlock,
                                   EaBuffer,
                                   EaInformation.EaSize,
                                   FALSE,
                                   NULL,
                                   0,
                                   NULL,
                                   TRUE);

            if (NT_SUCCESS(Status))
            {
               /* we successfully read the extended attributes */
               EaLength = EaInformation.EaSize;
               break;
            }
            else
            {
               RtlFreeHeap(RtlGetProcessHeap(),
                           0,
                           EaBuffer);
               EaBuffer = NULL;

               if (Status != STATUS_BUFFER_TOO_SMALL)
               {
                  /* unless we just allocated not enough memory, break the loop
                     and just continue without copying extended attributes */
                  break;
               }
            }
          }
          else
          {
            /* failure or no extended attributes present, break the loop */
            break;
          }
        }

        NtClose(TemplateHandle);

        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     NtTemplatePathU.Buffer);

        if (!NT_SUCCESS(Status))
        {
                /* free the he extended attributes buffer */
                if (EaBuffer != NULL)
                {
                        RtlFreeHeap (RtlGetProcessHeap (),
                                     0,
                                     EaBuffer);
                }

                SetLastErrorByStatus (Status);
                return FALSE;
        }

        /*
         * Create the new directory and copy over the extended attributes if
         * needed
         */

        if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpNewDirectory,
                                           &NtPathU,
                                           NULL,
                                           NULL))
        {
                /* free the he extended attributes buffer */
                if (EaBuffer != NULL)
                {
                        RtlFreeHeap (RtlGetProcessHeap (),
                                     0,
                                     EaBuffer);
                }

                SetLastError(ERROR_PATH_NOT_FOUND);
                return FALSE;
        }

        InitializeObjectAttributes(&ObjectAttributes,
                                   &NtPathU,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   (lpSecurityAttributes ? lpSecurityAttributes->lpSecurityDescriptor : NULL));

        Status = NtCreateFile (&DirectoryHandle,
                               GENERIC_READ,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_NORMAL,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               FILE_CREATE,
                               FILE_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT,
                               EaBuffer,
                               EaLength);

        /* free the he extended attributes buffer */
        if (EaBuffer != NULL)
        {
                RtlFreeHeap (RtlGetProcessHeap (),
                             0,
                             EaBuffer);
        }

        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     NtPathU.Buffer);

        if (!NT_SUCCESS(Status))
        {
                SetLastErrorByStatus(Status);
                return FALSE;
        }

        NtClose (DirectoryHandle);

        return TRUE;
}


/*
 * @implemented
 */
BOOL
STDCALL
RemoveDirectoryA (
        LPCSTR  lpPathName
        )
{
   PWCHAR PathNameW;

   DPRINT("RemoveDirectoryA(%s)\n",lpPathName);

   if (!(PathNameW = FilenameA2W(lpPathName, FALSE)))
       return FALSE;

   return RemoveDirectoryW (PathNameW);
}


/*
 * @implemented
 */
BOOL
STDCALL
RemoveDirectoryW (
        LPCWSTR lpPathName
        )
{
        FILE_DISPOSITION_INFORMATION FileDispInfo;
        OBJECT_ATTRIBUTES ObjectAttributes;
        IO_STATUS_BLOCK IoStatusBlock;
        UNICODE_STRING NtPathU;
        HANDLE DirectoryHandle;
        NTSTATUS Status;

        DPRINT("lpPathName %S\n", lpPathName);

        if (!RtlDosPathNameToNtPathName_U ((LPWSTR)lpPathName,
                                           &NtPathU,
                                           NULL,
                                           NULL))
                return FALSE;

        InitializeObjectAttributes(&ObjectAttributes,
                                   &NtPathU,
                                   OBJ_CASE_INSENSITIVE,
                                   NULL,
                                   NULL);

        DPRINT("NtPathU '%S'\n", NtPathU.Buffer);

        Status = NtCreateFile (&DirectoryHandle,
                               DELETE,
                               &ObjectAttributes,
                               &IoStatusBlock,
                               NULL,
                               FILE_ATTRIBUTE_DIRECTORY, /* 0x7 */
                               0,
                               FILE_OPEN,
                               FILE_DIRECTORY_FILE,      /* 0x204021 */
                               NULL,
                               0);

        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     NtPathU.Buffer);

        if (!NT_SUCCESS(Status))
        {
                CHECKPOINT;
                SetLastErrorByStatus (Status);
                return FALSE;
        }

        FileDispInfo.DeleteFile = TRUE;

        Status = NtSetInformationFile (DirectoryHandle,
                                       &IoStatusBlock,
                                       &FileDispInfo,
                                       sizeof(FILE_DISPOSITION_INFORMATION),
                                       FileDispositionInformation);
        NtClose(DirectoryHandle);

        if (!NT_SUCCESS(Status))
        {
                SetLastErrorByStatus (Status);
                return FALSE;
        }

        return TRUE;
}


/*
 * @implemented
 */
DWORD
STDCALL
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

   if (!lpFileName)
   {
     SetLastError(ERROR_INVALID_PARAMETER);
     return 0;
   }

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
STDCALL
GetFullPathNameW (
        LPCWSTR lpFileName,
        DWORD   nBufferLength,
        LPWSTR  lpBuffer,
        LPWSTR  *lpFilePart
        )
{
    ULONG Length;

    DPRINT("GetFullPathNameW(lpFileName %S, nBufferLength %d, lpBuffer %p, "
           "lpFilePart %p)\n",lpFileName,nBufferLength,lpBuffer,lpFilePart);

    Length = RtlGetFullPathName_U ((LPWSTR)lpFileName,
                                   nBufferLength * sizeof(WCHAR),
                                   lpBuffer,
                                   lpFilePart);

    DPRINT("GetFullPathNameW ret: lpBuffer %S lpFilePart %S Length %ld\n",
           lpBuffer, (lpFilePart == NULL) ? L"NULL" : *lpFilePart, Length / sizeof(WCHAR));

    return Length/sizeof(WCHAR);
}


/*
 * NOTE: Copied from Wine.
 * @implemented
 */
DWORD
STDCALL
GetShortPathNameA (
        LPCSTR  longpath,
        LPSTR   shortpath,
        DWORD   shortlen
        )
{
    PWCHAR LongPathW;
    WCHAR ShortPathW[MAX_PATH];
    DWORD ret;

    if (!longpath)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    if (!(LongPathW = FilenameA2W(longpath, FALSE)))
      return 0;

    ret = GetShortPathNameW(LongPathW, ShortPathW, MAX_PATH);

    if (!ret)
        return 0;

    if (ret > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }

    return FilenameW2A_FitOrFail(shortpath, shortlen, ShortPathW, ret+1);
}


/*
 * NOTE: Copied from Wine.
 * @implemented
 */
DWORD
STDCALL
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
    if (longpath[1] == ':' )
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
        if (tmplen <= 8+1+3+1)
        {
            BOOLEAN spaces;
            memcpy(ustr_buf, longpath + lp, tmplen * sizeof(WCHAR));
            ustr_buf[tmplen] = '\0';
            ustr.Length = tmplen * sizeof(WCHAR);
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


/*
 * @implemented
 */
DWORD
STDCALL
SearchPathA (
        LPCSTR  lpPath,
        LPCSTR  lpFileName,
        LPCSTR  lpExtension,
        DWORD   nBufferLength,
        LPSTR   lpBuffer,
        LPSTR   *lpFilePart
        )
{
        UNICODE_STRING PathU;
        UNICODE_STRING FileNameU;
        UNICODE_STRING ExtensionU;
        UNICODE_STRING BufferU;
        ANSI_STRING Path;
        ANSI_STRING FileName;
        ANSI_STRING Extension;
        ANSI_STRING Buffer;
        PWCHAR FilePartW;
        DWORD RetValue;

        RtlInitAnsiString (&Path,
                           (LPSTR)lpPath);
        RtlInitAnsiString (&FileName,
                           (LPSTR)lpFileName);
        RtlInitAnsiString (&Extension,
                           (LPSTR)lpExtension);

        /* convert ansi (or oem) strings to unicode */
        if (bIsFileApiAnsi)
        {
                RtlAnsiStringToUnicodeString (&PathU,
                                              &Path,
                                              TRUE);
                RtlAnsiStringToUnicodeString (&FileNameU,
                                              &FileName,
                                              TRUE);
                RtlAnsiStringToUnicodeString (&ExtensionU,
                                              &Extension,
                                              TRUE);
        }
        else
        {
                RtlOemStringToUnicodeString (&PathU,
                                             &Path,
                                             TRUE);
                RtlOemStringToUnicodeString (&FileNameU,
                                             &FileName,
                                             TRUE);
                RtlOemStringToUnicodeString (&ExtensionU,
                                             &Extension,
                                             TRUE);
        }

        BufferU.Length = 0;
        BufferU.MaximumLength = nBufferLength * sizeof(WCHAR);
        BufferU.Buffer = RtlAllocateHeap (RtlGetProcessHeap (),
                                          0,
                                          BufferU.MaximumLength);

        Buffer.Length = 0;
        Buffer.MaximumLength = nBufferLength;
        Buffer.Buffer = lpBuffer;

        RetValue = SearchPathW (NULL == lpPath ? NULL : PathU.Buffer,
                                NULL == lpFileName ? NULL : FileNameU.Buffer,
                                NULL == lpExtension ? NULL : ExtensionU.Buffer,
                                nBufferLength,
                                BufferU.Buffer,
                                &FilePartW);

        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     PathU.Buffer);
        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     FileNameU.Buffer);
        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     ExtensionU.Buffer);

        if (0 != RetValue)
        {
                BufferU.Length = wcslen(BufferU.Buffer) * sizeof(WCHAR);
                /* convert ansi (or oem) string to unicode */
                if (bIsFileApiAnsi)
                        RtlUnicodeStringToAnsiString (&Buffer,
                                                      &BufferU,
                                                      FALSE);
                else
                        RtlUnicodeStringToOemString (&Buffer,
                                                     &BufferU,
                                                     FALSE);
                /* nul-terminate ascii string */
                Buffer.Buffer[BufferU.Length / sizeof(WCHAR)] = '\0';
        }

        RtlFreeHeap (RtlGetProcessHeap (),
                     0,
                     BufferU.Buffer);

        if (NULL != lpFilePart)
        {
                *lpFilePart = strrchr (lpBuffer, '\\') + 1;
        }

        return RetValue;
}


/*
 * @implemented
 */
DWORD
STDCALL
SearchPathW (
        LPCWSTR lpPath,
        LPCWSTR lpFileName,
        LPCWSTR lpExtension,
        DWORD   nBufferLength,
        LPWSTR  lpBuffer,
        LPWSTR  *lpFilePart
        )
/*
 * FUNCTION: Searches for the specified file
 * ARGUMENTS:
 *       lpPath = Points to a null-terminated string that specified the
 *                path to be searched. If this parameters is NULL then
 *                the following directories are searched
 *                          The directory from which the application loaded
 *                          The current directory
 *                          The system directory
 *                          The 16-bit system directory
 *                          The windows directory
 *                          The directories listed in the PATH environment
 *                          variable
 *        lpFileName = Specifies the filename to search for
 *        lpExtension = Points to the null-terminated string that specifies
 *                      an extension to be added to the filename when
 *                      searching for the file. The first character of the
 *                      filename extension must be a period (.). The
 *                      extension is only added if the specified filename
 *                      doesn't end with an extension
 *
 *                      If the filename extension is not required or if the
 *                      filename contains an extension, this parameters can be
 *                      NULL
 *        nBufferLength = The length in characters of the buffer for output
 *        lpBuffer = Points to the buffer for the valid path and filename of
 *                   file found
 *        lpFilePart = Points to the last component of the valid path and
 *                     filename
 * RETURNS: On success, the length, in characters, of the string copied to the
 *          buffer
 *          On failure, zero.
 */
{
        DWORD retCode = 0;
        ULONG pos, len;
        PWCHAR EnvironmentBufferW = NULL;
        PWCHAR AppPathW = NULL;
        WCHAR Buffer;
        BOOL HasExtension;
        LPCWSTR p;
        PWCHAR Name;

        DPRINT("SearchPath\n");

        HasExtension = FALSE;
        p = lpFileName + wcslen(lpFileName);
        while (lpFileName < p &&
               L'\\' != *(p - 1) &&
               L'/' != *(p - 1) &&
               L':' != *(p - 1))
        {
                HasExtension = HasExtension || L'.' == *(p - 1);
                p--;
        }
        if (lpFileName < p)
        {
                if (HasExtension || NULL == lpExtension)
                {
                        Name = (PWCHAR) lpFileName;
                }
                else
                {
                        Name = RtlAllocateHeap(GetProcessHeap(),
                                               HEAP_GENERATE_EXCEPTIONS,
                                               (wcslen(lpFileName) + wcslen(lpExtension) + 1)
                                               * sizeof(WCHAR));
                        if (NULL == Name)
                        {
                                SetLastError(ERROR_OUTOFMEMORY);
                                return 0;
                        }
                        wcscat(wcscpy(Name, lpFileName), lpExtension);
                }
	        if (RtlDoesFileExists_U(Name))
                {
                        retCode = RtlGetFullPathName_U (Name,
                                                        nBufferLength * sizeof(WCHAR),
                                                        lpBuffer,
                                                        lpFilePart);
                }
                if (Name != lpFileName)
                {
                        RtlFreeHeap(GetProcessHeap(), 0, Name);
                }
        }
        else
        {
                if (lpPath == NULL)
                {

                        AppPathW = (PWCHAR) RtlAllocateHeap(GetProcessHeap(),
                                                        HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,
                                                        MAX_PATH * sizeof(WCHAR));


                        wcscat (AppPathW, NtCurrentPeb()->ProcessParameters->ImagePathName.Buffer);

                        len = wcslen (AppPathW);

                        while (len && AppPathW[len - 1] != L'\\')
                                len--;

                        if (len) AppPathW[len-1] = L'\0';

                        len = GetEnvironmentVariableW(L"PATH", &Buffer, 0);
                        len += 1 + GetCurrentDirectoryW(0, &Buffer);
                        len += 1 + GetSystemDirectoryW(&Buffer, 0);
                        len += 1 + GetWindowsDirectoryW(&Buffer, 0);
                        len += 1 + wcslen(AppPathW) * sizeof(WCHAR);

                        EnvironmentBufferW = (PWCHAR) RtlAllocateHeap(GetProcessHeap(),
                                                        HEAP_GENERATE_EXCEPTIONS|HEAP_ZERO_MEMORY,
                                                        len * sizeof(WCHAR));
                        if (EnvironmentBufferW == NULL)
                        {
                                SetLastError(ERROR_OUTOFMEMORY);
                                return 0;
                        }

                        pos = GetCurrentDirectoryW(len, EnvironmentBufferW);
                        EnvironmentBufferW[pos++] = L';';
                        EnvironmentBufferW[pos] = 0;
                        pos += GetSystemDirectoryW(&EnvironmentBufferW[pos], len - pos);
                        EnvironmentBufferW[pos++] = L';';
                        EnvironmentBufferW[pos] = 0;
                        pos += GetWindowsDirectoryW(&EnvironmentBufferW[pos], len - pos);
                        EnvironmentBufferW[pos++] = L';';
                        EnvironmentBufferW[pos] = 0;
                        pos += GetEnvironmentVariableW(L"PATH", &EnvironmentBufferW[pos], len - pos);
                        EnvironmentBufferW[pos++] = L';';
                        EnvironmentBufferW[pos] = 0;
                        wcscat (EnvironmentBufferW, AppPathW);

                        RtlFreeHeap (RtlGetProcessHeap (),
                             0,
                             AppPathW);

                        lpPath = EnvironmentBufferW;

                }

                retCode = RtlDosSearchPath_U ((PWCHAR)lpPath, (PWCHAR)lpFileName, (PWCHAR)lpExtension,
                                              nBufferLength * sizeof(WCHAR), lpBuffer, lpFilePart);

                if (EnvironmentBufferW != NULL)
                {
                        RtlFreeHeap(GetProcessHeap(), 0, EnvironmentBufferW);
                }
                if (retCode == 0)
                {
                        SetLastError(ERROR_FILE_NOT_FOUND);
                }
        }
        return retCode / sizeof(WCHAR);
}

/*
 * @implemented
 */
BOOL
STDCALL
SetDllDirectoryW(
    LPCWSTR lpPathName
    )
{
  UNICODE_STRING PathName;

  RtlInitUnicodeString(&PathName, lpPathName);

  RtlEnterCriticalSection(&DllLock);
  if(PathName.Length > 0)
  {
    if(PathName.Length + sizeof(WCHAR) <= DllDirectory.MaximumLength)
    {
      RtlCopyUnicodeString(&DllDirectory, &PathName);
    }
    else
    {
      RtlFreeUnicodeString(&DllDirectory);
      if(!(DllDirectory.Buffer = (PWSTR)RtlAllocateHeap(RtlGetProcessHeap(),
                                                        0,
                                                        PathName.Length + sizeof(WCHAR))))
      {
        RtlLeaveCriticalSection(&DllLock);
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
      }
      DllDirectory.Length = 0;
      DllDirectory.MaximumLength = PathName.Length + sizeof(WCHAR);

      RtlCopyUnicodeString(&DllDirectory, &PathName);
    }
  }
  else
  {
    RtlFreeUnicodeString(&DllDirectory);
  }
  RtlLeaveCriticalSection(&DllLock);

  return TRUE;
}

/*
 * @implemented
 */
BOOL
STDCALL
SetDllDirectoryA(
    LPCSTR lpPathName /* can be NULL */
    )
{
  PWCHAR PathNameW=NULL;

  if(lpPathName)
  {
     if (!(PathNameW = FilenameA2W(lpPathName, FALSE)))
        return FALSE;
  }

  return SetDllDirectoryW(PathNameW);
}

/*
 * @implemented
 */
DWORD
STDCALL
GetDllDirectoryW(
    DWORD nBufferLength,
    LPWSTR lpBuffer
    )
{
  DWORD Ret;

  RtlEnterCriticalSection(&DllLock);
  if(nBufferLength > 0)
  {
    Ret = DllDirectory.Length / sizeof(WCHAR);
    if(Ret > nBufferLength - 1)
    {
      Ret = nBufferLength - 1;
    }

    if(Ret > 0)
    {
      RtlCopyMemory(lpBuffer, DllDirectory.Buffer, Ret * sizeof(WCHAR));
    }
    lpBuffer[Ret] = L'\0';
  }
  else
  {
    /* include termination character, even if the string is empty! */
    Ret = (DllDirectory.Length / sizeof(WCHAR)) + 1;
  }
  RtlLeaveCriticalSection(&DllLock);

  return Ret;
}

/*
 * @implemented
 */
DWORD
STDCALL
GetDllDirectoryA(
    DWORD nBufferLength,
    LPSTR lpBuffer
    )
{
  WCHAR BufferW[MAX_PATH];
  DWORD ret;

  ret = GetDllDirectoryW(MAX_PATH, BufferW);

  if (!ret)
     return 0;

  if (ret > MAX_PATH)
  {
     SetLastError(ERROR_FILENAME_EXCED_RANGE);
     return 0;
  }

  return FilenameW2A_FitOrFail(lpBuffer, nBufferLength, BufferW, ret+1);
}


/*
 * @implemented
 */
BOOL STDCALL
NeedCurrentDirectoryForExePathW(LPCWSTR ExeName)
{
  return (wcschr(ExeName,
                 L'\\') != NULL);
}


/*
 * @implemented
 */
BOOL STDCALL
NeedCurrentDirectoryForExePathA(LPCSTR ExeName)
{
  return (strchr(ExeName,
                 '\\') != NULL);
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
DWORD STDCALL GetLongPathNameW( LPCWSTR shortpath, LPWSTR longpath, DWORD longlen )
{
#define    MAX_PATHNAME_LEN 1024

    WCHAR               tmplongpath[MAX_PATHNAME_LEN];
    LPCWSTR             p;
    DWORD               sp = 0, lp = 0;
    DWORD               tmplen;
    BOOL                unixabsolute = (shortpath[0] == '/');
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



/***********************************************************************
 *           GetLongPathNameA   (KERNEL32.@)
 */
DWORD STDCALL GetLongPathNameA( LPCSTR shortpath, LPSTR longpath, DWORD longlen )
{
    WCHAR *shortpathW;
    WCHAR longpathW[MAX_PATH];
    DWORD ret;

    DPRINT("GetLongPathNameA %s, %i\n",shortpath,longlen );

    if (!(shortpathW = FilenameA2W( shortpath, FALSE )))
      return 0;

    ret = GetLongPathNameW(shortpathW, longpathW, MAX_PATH);

    if (!ret) return 0;
    if (ret > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }

    return FilenameW2A_FitOrFail(longpath, longlen, longpathW,  ret+1 );
}

/* EOF */
