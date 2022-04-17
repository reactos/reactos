/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/file/filename.c
 * PURPOSE:         Directory functions
 * PROGRAMMERS:     Ariadne (ariadne@xs4all.nl)
 *                  Pierre Schweitzer (pierre.schweitzer@reactos.org)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES *****************************************************************/

#include <k32.h>
#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

/* FUNCTIONS ****************************************************************/

/***********************************************************************
 *           GetTempFileNameA   (KERNEL32.@)
 */
UINT WINAPI
GetTempFileNameA(IN LPCSTR lpPathName,
                 IN LPCSTR lpPrefixString,
                 IN UINT uUnique,
                 OUT LPSTR lpTempFileName)
{
    UINT ID;
    NTSTATUS Status;
    LPWSTR lpTempFileNameW;
    PUNICODE_STRING lpPathNameW;
    ANSI_STRING TempFileNameStringA;
    UNICODE_STRING lpPrefixStringW, TempFileNameStringW;

    /* Convert strings */
    lpPathNameW = Basep8BitStringToStaticUnicodeString(lpPathName);
    if (!lpPathNameW)
    {
        return 0;
    }

    if (!Basep8BitStringToDynamicUnicodeString(&lpPrefixStringW, lpPrefixString))
    {
        return 0;
    }

    lpTempFileNameW = RtlAllocateHeap(RtlGetProcessHeap(), 0, MAX_PATH * sizeof(WCHAR));
    if (!lpTempFileNameW)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        RtlFreeUnicodeString(&lpPrefixStringW);
        return 0;
    }

    /* Call Unicode */
    ID = GetTempFileNameW(lpPathNameW->Buffer, lpPrefixStringW.Buffer, uUnique, lpTempFileNameW);
    if (ID)
    {
        RtlInitUnicodeString(&TempFileNameStringW, lpTempFileNameW);
        TempFileNameStringA.Buffer = lpTempFileName;
        TempFileNameStringA.MaximumLength = MAX_PATH;

        Status = BasepUnicodeStringTo8BitString(&TempFileNameStringA, &TempFileNameStringW, FALSE);
        if (!NT_SUCCESS(Status))
        {
            BaseSetLastNTError(Status);
            ID = 0;
        }
    }

    /* Cleanup */
    RtlFreeUnicodeString(&lpPrefixStringW);
    RtlFreeHeap(RtlGetProcessHeap(), 0, lpTempFileNameW);
    return ID;
 }

 /***********************************************************************
  *           GetTempFileNameW   (KERNEL32.@)
  */
UINT WINAPI
GetTempFileNameW(IN LPCWSTR lpPathName,
                 IN LPCWSTR lpPrefixString,
                 IN UINT uUnique,
                 OUT LPWSTR lpTempFileName)
{
    PUCHAR Let;
    HANDLE TempFile;
    UINT ID, Num = 0;
    UCHAR IDString[5];
    WCHAR * TempFileName;
    BASE_API_MESSAGE ApiMessage;
    PBASE_GET_TEMP_FILE GetTempFile = &ApiMessage.Data.GetTempFileRequest;
    DWORD FileAttributes, LastError;
    UNICODE_STRING PathNameString, PrefixString;
    static const WCHAR Ext[] = { L'.', 't', 'm', 'p', UNICODE_NULL };

    RtlInitUnicodeString(&PathNameString, lpPathName);
    if (PathNameString.Length == 0 || PathNameString.Buffer[(PathNameString.Length / sizeof(WCHAR)) - 1] != L'\\')
    {
        PathNameString.Length += sizeof(WCHAR);
    }

    /* lpTempFileName must be able to contain: PathName, Prefix (3), number(4), .tmp(4) & \0(1)
     * See: http://msdn.microsoft.com/en-us/library/aa364991%28v=vs.85%29.aspx
     */
    if (PathNameString.Length > (MAX_PATH - 3 - 4 - 4 - 1) * sizeof(WCHAR))
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return 0;
    }

    /* If PathName and TempFileName aren't the same buffer, move PathName to TempFileName */
    if (lpPathName != lpTempFileName)
    {
        memmove(lpTempFileName, PathNameString.Buffer, PathNameString.Length);
    }

    /* PathName MUST BE a path. Check it */
    lpTempFileName[(PathNameString.Length / sizeof(WCHAR)) - 1] = UNICODE_NULL;
    FileAttributes = GetFileAttributesW(lpTempFileName);
    if (FileAttributes == INVALID_FILE_ATTRIBUTES)
    {
        /* Append a '\' if necessary */
        lpTempFileName[(PathNameString.Length / sizeof(WCHAR)) - 1] = L'\\';
        lpTempFileName[PathNameString.Length / sizeof(WCHAR)] = UNICODE_NULL;
        FileAttributes = GetFileAttributesW(lpTempFileName);
        if (FileAttributes == INVALID_FILE_ATTRIBUTES)
        {
            SetLastError(ERROR_DIRECTORY);
            return 0;
        }
    }
    if (!(FileAttributes & FILE_ATTRIBUTE_DIRECTORY))
    {
        SetLastError(ERROR_DIRECTORY);
        return 0;
    }

    /* Make sure not to mix path & prefix */
    lpTempFileName[(PathNameString.Length / sizeof(WCHAR)) - 1] = L'\\';
    RtlInitUnicodeString(&PrefixString, lpPrefixString);
    if (PrefixString.Length > 3 * sizeof(WCHAR))
    {
        PrefixString.Length = 3 * sizeof(WCHAR);
    }

    /* Append prefix to path */
    TempFileName = lpTempFileName + PathNameString.Length / sizeof(WCHAR);
    memmove(TempFileName, PrefixString.Buffer, PrefixString.Length);
    TempFileName += PrefixString.Length / sizeof(WCHAR);

    /* Then, generate filename */
    do
    {
        /* If user didn't gave any ID, ask Csrss to give one */
        if (!uUnique)
        {
            CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                NULL,
                                CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepGetTempFile),
                                sizeof(*GetTempFile));
            if (GetTempFile->UniqueID == 0)
            {
                Num++;
                continue;
            }

            ID = GetTempFile->UniqueID;
        }
        else
        {
            ID = uUnique;
        }

        /* Convert that ID to wchar */
        RtlIntegerToChar(ID, 0x10, sizeof(IDString), (PCHAR)IDString);
        Let = IDString;
        do
        {
            *(TempFileName++) = RtlAnsiCharToUnicodeChar(&Let);
        } while (*Let != 0);

        /* Append extension & UNICODE_NULL */
        memmove(TempFileName, Ext, sizeof(Ext));

        /* If user provided its ID, just return */
        if (uUnique)
        {
            return uUnique;
        }

        /* Then, try to create file */
        if (!RtlIsDosDeviceName_U(lpTempFileName))
        {
            TempFile = CreateFileW(lpTempFileName,
                                   GENERIC_READ,
                                   0,
                                   NULL,
                                   CREATE_NEW,
                                   FILE_ATTRIBUTE_NORMAL,
                                   0);
            if (TempFile != INVALID_HANDLE_VALUE)
            {
                NtClose(TempFile);
                DPRINT("Temp file: %S\n", lpTempFileName);
                return ID;
            }

            LastError = GetLastError();
            /* There is no need to recover from those errors, they would hit next step */
            if (LastError == ERROR_INVALID_PARAMETER || LastError == ERROR_CANNOT_MAKE ||
                LastError == ERROR_WRITE_PROTECT || LastError == ERROR_NETWORK_ACCESS_DENIED ||
                LastError == ERROR_DISK_FULL || LastError == ERROR_INVALID_NAME ||
                LastError == ERROR_BAD_PATHNAME || LastError == ERROR_NO_INHERITANCE ||
                LastError == ERROR_DISK_CORRUPT ||
                (LastError == ERROR_ACCESS_DENIED && NtCurrentTeb()->LastStatusValue != STATUS_FILE_IS_A_DIRECTORY))
            {
                break;
            }
        }
        Num++;
    } while (Num & 0xFFFF);

    return 0;
}

/*
 * @implemented
 */
BOOL
WINAPI
SetFileShortNameW(
    HANDLE hFile,
    LPCWSTR lpShortName)
{
    NTSTATUS Status;
    ULONG NeededSize;
    UNICODE_STRING ShortName;
    IO_STATUS_BLOCK IoStatusBlock;
    PFILE_NAME_INFORMATION FileNameInfo;

    if(IsConsoleHandle(hFile))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(!lpShortName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlInitUnicodeString(&ShortName, lpShortName);

    NeededSize = sizeof(FILE_NAME_INFORMATION) + ShortName.Length + sizeof(WCHAR);
    if(!(FileNameInfo = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, NeededSize)))
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    FileNameInfo->FileNameLength = ShortName.Length;
    RtlCopyMemory(FileNameInfo->FileName, ShortName.Buffer, ShortName.Length);

    Status = NtSetInformationFile(hFile,
                                  &IoStatusBlock, //out
                                  FileNameInfo,
                                  NeededSize,
                                  FileShortNameInformation);

    RtlFreeHeap(RtlGetProcessHeap(), 0, FileNameInfo);
    if(!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetFileShortNameA(
    HANDLE hFile,
    LPCSTR lpShortName)
{
    PWCHAR ShortNameW;

    if(IsConsoleHandle(hFile))
    {
        SetLastError(ERROR_INVALID_HANDLE);
        return FALSE;
    }

    if(!lpShortName)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    if (!(ShortNameW = FilenameA2W(lpShortName, FALSE)))
        return FALSE;

    return SetFileShortNameW(hFile, ShortNameW);
}


/*
 * @implemented
 */
BOOL
WINAPI
CheckNameLegalDOS8Dot3W(
    LPCWSTR lpName,
    LPSTR lpOemName OPTIONAL,
    DWORD OemNameSize OPTIONAL,
    PBOOL pbNameContainsSpaces OPTIONAL,
    PBOOL pbNameLegal
    )
{
    UNICODE_STRING Name;
    ANSI_STRING AnsiName;
    BOOLEAN NameContainsSpaces;

    if(lpName == NULL ||
       (lpOemName == NULL && OemNameSize != 0) ||
       pbNameLegal == NULL)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

    if(lpOemName != NULL)
    {
      AnsiName.Buffer = lpOemName;
      AnsiName.MaximumLength = (USHORT)OemNameSize * sizeof(CHAR);
      AnsiName.Length = 0;
    }

    RtlInitUnicodeString(&Name, lpName);

    *pbNameLegal = RtlIsNameLegalDOS8Dot3(&Name,
                                          (lpOemName ? &AnsiName : NULL),
                                          &NameContainsSpaces);
    if (*pbNameLegal && pbNameContainsSpaces)
        *pbNameContainsSpaces = NameContainsSpaces;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
CheckNameLegalDOS8Dot3A(
    LPCSTR lpName,
    LPSTR lpOemName OPTIONAL,
    DWORD OemNameSize OPTIONAL,
    PBOOL pbNameContainsSpaces OPTIONAL,
    PBOOL pbNameLegal
    )
{
    UNICODE_STRING Name;
    ANSI_STRING AnsiName, AnsiInputName;
    NTSTATUS Status;
    BOOLEAN NameContainsSpaces;

    if(lpName == NULL ||
       (lpOemName == NULL && OemNameSize != 0) ||
       pbNameLegal == NULL)
    {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
    }

    if(lpOemName != NULL)
    {
      AnsiName.Buffer = lpOemName;
      AnsiName.MaximumLength = (USHORT)OemNameSize * sizeof(CHAR);
      AnsiName.Length = 0;
    }

    RtlInitAnsiString(&AnsiInputName, (LPSTR)lpName);
    if(bIsFileApiAnsi)
      Status = RtlAnsiStringToUnicodeString(&Name, &AnsiInputName, TRUE);
    else
      Status = RtlOemStringToUnicodeString(&Name, &AnsiInputName, TRUE);

    if(!NT_SUCCESS(Status))
    {
      BaseSetLastNTError(Status);
      return FALSE;
    }

    *pbNameLegal = RtlIsNameLegalDOS8Dot3(&Name,
                                          (lpOemName ? &AnsiName : NULL),
                                          &NameContainsSpaces);
    if (*pbNameLegal && pbNameContainsSpaces)
        *pbNameContainsSpaces = NameContainsSpaces;

    RtlFreeUnicodeString(&Name);

    return TRUE;
}
