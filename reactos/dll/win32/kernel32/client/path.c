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

UNICODE_STRING NoDefaultCurrentDirectoryInExePath = RTL_CONSTANT_STRING(L"NoDefaultCurrentDirectoryInExePath");

UNICODE_STRING BaseWindowsSystemDirectory, BaseWindowsDirectory;
UNICODE_STRING BaseDefaultPathAppend, BaseDefaultPath, BaseDllDirectory;

PVOID gpTermsrvGetWindowsDirectoryA;
PVOID gpTermsrvGetWindowsDirectoryW;

/* This is bitmask for each illegal filename character */
/* If someone has time, please feel free to use 0b notation */
DWORD IllegalMask[4] =
{
    0xFFFFFFFF, // None allowed (00 to 1F)
    0xFC009C05, // 20, 22, 2A, 2B, 2C, 2F, 3A, 3B, 3C, 3D, 3E, 3F not allowed
    0x38000000, // 5B, 5C, 5D not allowed
    0x10000000  // 7C not allowed
};

BASE_SEARCH_PATH_TYPE BaseDllOrderCurrent[BaseCurrentDirPlacementMax][BaseSearchPathMax] =
{
    {
        BaseSearchPathApp,
        BaseSearchPathCurrent,
        BaseSearchPathDefault,
        BaseSearchPathEnv,
        BaseSearchPathInvalid
    },
    {
        BaseSearchPathApp,
        BaseSearchPathDefault,
        BaseSearchPathCurrent,
        BaseSearchPathEnv,
        BaseSearchPathInvalid
    }
};

BASE_SEARCH_PATH_TYPE BaseProcessOrderNoCurrent[BaseSearchPathMax] =
{
    BaseSearchPathApp,
    BaseSearchPathDefault,
    BaseSearchPathEnv,
    BaseSearchPathInvalid,
    BaseSearchPathInvalid
};

BASE_SEARCH_PATH_TYPE BaseDllOrderNoCurrent[BaseSearchPathMax] =
{
    BaseSearchPathApp,
    BaseSearchPathDll,
    BaseSearchPathDefault,
    BaseSearchPathEnv,
    BaseSearchPathInvalid
};

BASE_SEARCH_PATH_TYPE BaseProcessOrder[BaseSearchPathMax] =
{
    BaseSearchPathApp,
    BaseSearchPathCurrent,
    BaseSearchPathDefault,
    BaseSearchPathEnv,
    BaseSearchPathInvalid
};

BASE_CURRENT_DIR_PLACEMENT BasepDllCurrentDirPlacement = BaseCurrentDirPlacementInvalid;

extern UNICODE_STRING BasePathVariableName;

/* PRIVATE FUNCTIONS **********************************************************/

PWCHAR
WINAPI
BasepEndOfDirName(IN PWCHAR FileName)
{
    PWCHAR FileNameEnd, FileNameSeparator;

    /* Find the first slash */
    FileNameSeparator = wcschr(FileName, OBJ_NAME_PATH_SEPARATOR);
    if (FileNameSeparator)
    {
        /* Find the last one */
        FileNameEnd = wcsrchr(FileNameSeparator, OBJ_NAME_PATH_SEPARATOR);
        ASSERT(FileNameEnd);

        /* Handle the case where they are one and the same */
        if (FileNameEnd == FileNameSeparator) FileNameEnd++;
    }
    else
    {
        /* No directory was specified */
        FileNameEnd = NULL;
    }

    /* Return where the directory ends and the filename starts */
    return FileNameEnd;
}

LPWSTR
WINAPI
BasepComputeProcessPath(IN PBASE_SEARCH_PATH_TYPE PathOrder,
                        IN LPWSTR AppName,
                        IN LPVOID Environment)
{
    PWCHAR PathBuffer, Buffer, AppNameEnd, PathCurrent;
    ULONG PathLengthInBytes;
    NTSTATUS Status;
    UNICODE_STRING EnvPath;
    PBASE_SEARCH_PATH_TYPE Order;

    /* Initialize state */
    AppNameEnd = Buffer = PathBuffer = NULL;
    Status = STATUS_SUCCESS;
    PathLengthInBytes = 0;

    /* Loop the ordering array */
    for (Order = PathOrder; *Order != BaseSearchPathInvalid; Order++) {
    switch (*Order)
    {
        /* Compute the size of the DLL path */
        case BaseSearchPathDll:

            /* This path only gets called if SetDllDirectory was called */
            ASSERT(BaseDllDirectory.Buffer != NULL);

            /* Make sure there's a DLL directory size */
            if (BaseDllDirectory.Length)
            {
                /* Add it, plus the separator */
                PathLengthInBytes += BaseDllDirectory.Length + sizeof(L';');
            }
            break;

        /* Compute the size of the current path */
        case BaseSearchPathCurrent:

            /* Add ".;" */
            PathLengthInBytes += (2 * sizeof(WCHAR));
            break;

        /* Compute the size of the "PATH" environment variable */
        case BaseSearchPathEnv:

            /* Grab PEB lock if one wasn't passed in */
            if (!Environment) RtlAcquirePebLock();

            /* Query the size first */
            EnvPath.MaximumLength = 0;
            Status = RtlQueryEnvironmentVariable_U(Environment,
                                                   &BasePathVariableName,
                                                   &EnvPath);
            if (Status == STATUS_BUFFER_TOO_SMALL)
            {
                /* Compute the size we'll need for the environment */
                EnvPath.MaximumLength = EnvPath.Length + sizeof(WCHAR);
                if ((EnvPath.Length + sizeof(WCHAR)) > UNICODE_STRING_MAX_BYTES)
                {
                    /* Don't let it overflow */
                    EnvPath.MaximumLength = EnvPath.Length;
                }

                /* Allocate the environment buffer */
                Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                                         0,
                                         EnvPath.MaximumLength);
                if (Buffer)
                {
                    /* Now query the PATH environment variable */
                    EnvPath.Buffer = Buffer;
                    Status = RtlQueryEnvironmentVariable_U(Environment,
                                                           &BasePathVariableName,
                                                           &EnvPath);
                }
                else
                {
                    /* Failure case */
                    Status = STATUS_NO_MEMORY;
                }
            }

            /* Release the PEB lock from above */
            if (!Environment) RtlReleasePebLock();

            /* There might not be a PATH */
            if (Status == STATUS_VARIABLE_NOT_FOUND)
            {
                /* In this case, skip this PathOrder */
                EnvPath.Length = EnvPath.MaximumLength = 0;
                Status = STATUS_SUCCESS;
            }
            else if (!NT_SUCCESS(Status))
            {
                /* An early failure, go to exit code */
                goto Quickie;
            }
            else
            {
                /* Add the length of the PATH variable */
                ASSERT(!(EnvPath.Length & 1));
                PathLengthInBytes += (EnvPath.Length + sizeof(L';'));
            }
            break;

        /* Compute the size of the default search path */
        case BaseSearchPathDefault:

            /* Just add it... it already has a ';' at the end */
            ASSERT(!(BaseDefaultPath.Length & 1));
            PathLengthInBytes += BaseDefaultPath.Length;
            break;

        /* Compute the size of the current app directory */
        case BaseSearchPathApp:
            /* Find out where the app name ends, to get only the directory */
            if (AppName) AppNameEnd = BasepEndOfDirName(AppName);

            /* Check if there was no application name passed in */
            if (!(AppName) || !(AppNameEnd))
            {
                /* Do we have a per-thread CURDIR to use? */
                if (NtCurrentTeb()->NtTib.SubSystemTib)
                {
                    /* This means someone added RTL_PERTHREAD_CURDIR */
                    UNIMPLEMENTED_DBGBREAK();
                }

                /* We do not. Do we have the LDR_ENTRY for the executable? */
                if (!BasepExeLdrEntry)
                {
                    /* We do not. Grab it */
                    LdrEnumerateLoadedModules(0,
                                              BasepLocateExeLdrEntry,
                                              NtCurrentPeb()->ImageBaseAddress);
                }

                /* Now do we have it? */
                if (BasepExeLdrEntry)
                {
                    /* Yes, so read the name out of it */
                    AppName = BasepExeLdrEntry->FullDllName.Buffer;
                }

                /* Find out where the app name ends, to get only the directory */
                if (AppName) AppNameEnd = BasepEndOfDirName(AppName);
            }

            /* So, do we have an application name and its directory? */
            if ((AppName) && (AppNameEnd))
            {
                /* Add the size of the app's directory, plus the separator */
                PathLengthInBytes += ((AppNameEnd - AppName) * sizeof(WCHAR)) + sizeof(L';');
            }
            break;

        default:
            break;
        }
    }

    /* Bam, all done, we now have the final path size */
    ASSERT(PathLengthInBytes > 0);
    ASSERT(!(PathLengthInBytes & 1));

    /* Allocate the buffer to hold it */
    PathBuffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathLengthInBytes);
    if (!PathBuffer)
    {
        /* Failure path */
        Status = STATUS_NO_MEMORY;
        goto Quickie;
    }

    /* Now we loop again, this time to copy the data */
    PathCurrent = PathBuffer;
    for (Order = PathOrder; *Order != BaseSearchPathInvalid; Order++) {
    switch (*Order)
    {
        /* Add the DLL path */
        case BaseSearchPathDll:
            if (BaseDllDirectory.Length)
            {
                /* Copy it in the buffer, ASSERT there's enough space */
                ASSERT((((PathCurrent - PathBuffer + 1) * sizeof(WCHAR)) + BaseDllDirectory.Length) <= PathLengthInBytes);
                RtlCopyMemory(PathCurrent,
                              BaseDllDirectory.Buffer,
                              BaseDllDirectory.Length);

                /* Update the current pointer, add a separator */
                PathCurrent += (BaseDllDirectory.Length / sizeof(WCHAR));
                *PathCurrent++ = ';';
            }
            break;

        /* Add the current application path */
        case BaseSearchPathApp:
            if ((AppName) && (AppNameEnd))
            {
                /* Copy it in the buffer, ASSERT there's enough space */
                ASSERT(((PathCurrent - PathBuffer + 1 + (AppNameEnd - AppName)) * sizeof(WCHAR)) <= PathLengthInBytes);
                RtlCopyMemory(PathCurrent,
                              AppName,
                              (AppNameEnd - AppName) * sizeof(WCHAR));

                /* Update the current pointer, add a separator */
                PathCurrent += AppNameEnd - AppName;
                *PathCurrent++ = ';';
            }
            break;

        /* Add the default search path */
        case BaseSearchPathDefault:
            /* Copy it in the buffer, ASSERT there's enough space */
            ASSERT((((PathCurrent - PathBuffer) * sizeof(WCHAR)) + BaseDefaultPath.Length) <= PathLengthInBytes);
            RtlCopyMemory(PathCurrent, BaseDefaultPath.Buffer, BaseDefaultPath.Length);

            /* Update the current pointer. The default path already has a ";" */
            PathCurrent += (BaseDefaultPath.Length / sizeof(WCHAR));
            break;

        /* Add the path in the PATH environment variable */
        case BaseSearchPathEnv:
            if (EnvPath.Length)
            {
                /* Copy it in the buffer, ASSERT there's enough space */
                ASSERT((((PathCurrent - PathBuffer + 1) * sizeof(WCHAR)) + EnvPath.Length) <= PathLengthInBytes);
                RtlCopyMemory(PathCurrent, EnvPath.Buffer, EnvPath.Length);

                /* Update the current pointer, add a separator */
                PathCurrent += (EnvPath.Length / sizeof(WCHAR));
                *PathCurrent++ = ';';
            }
            break;

        /* Add the current dierctory */
        case BaseSearchPathCurrent:

            /* Copy it in the buffer, ASSERT there's enough space */
            ASSERT(((PathCurrent - PathBuffer + 2) * sizeof(WCHAR)) <= PathLengthInBytes);
            *PathCurrent++ = '.';

            /* Add the path separator */
            *PathCurrent++ = ';';
            break;

        default:
            break;
        }
    }

    /* Everything should've perfectly fit in there */
    ASSERT((PathCurrent - PathBuffer) * sizeof(WCHAR) == PathLengthInBytes);
    ASSERT(PathCurrent > PathBuffer);

    /* Terminate the whole thing */
    PathCurrent[-1] = UNICODE_NULL;

Quickie:
    /* Exit path: free our buffers */
    if (Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    if (PathBuffer)
    {
        /* This only gets freed in the failure path, since caller wants it */
        if (!NT_SUCCESS(Status))
        {
            RtlFreeHeap(RtlGetProcessHeap(), 0, PathBuffer);
            PathBuffer = NULL;
        }
    }

    /* Return the path! */
    return PathBuffer;
}

LPWSTR
WINAPI
BaseComputeProcessSearchPath(VOID)
{
    DPRINT("Computing Process Search path\n");

    /* Compute the path using default process order */
    return BasepComputeProcessPath(BaseProcessOrder, NULL, NULL);
}

LPWSTR
WINAPI
BaseComputeProcessExePath(IN LPWSTR FullPath)
{
    PBASE_SEARCH_PATH_TYPE PathOrder;
    DPRINT("Computing EXE path: %S\n", FullPath);

    /* Check if we should use the current directory */
    PathOrder = NeedCurrentDirectoryForExePathW(FullPath) ?
                BaseProcessOrder : BaseProcessOrderNoCurrent;

    /* And now compute the path */
    return BasepComputeProcessPath(PathOrder, NULL, NULL);
}

LPWSTR
WINAPI
BaseComputeProcessDllPath(IN LPWSTR FullPath,
                          IN PVOID Environment)
{
    LPWSTR DllPath = NULL;
    UNICODE_STRING KeyName = RTL_CONSTANT_STRING(L"\\Registry\\MACHINE\\System\\CurrentControlSet\\Control\\Session Manager");
    UNICODE_STRING ValueName = RTL_CONSTANT_STRING(L"SafeDllSearchMode");
    OBJECT_ATTRIBUTES ObjectAttributes = RTL_CONSTANT_OBJECT_ATTRIBUTES(&KeyName, OBJ_CASE_INSENSITIVE);
    KEY_VALUE_PARTIAL_INFORMATION PartialInfo;
    HANDLE KeyHandle;
    NTSTATUS Status;
    ULONG ResultLength;
    BASE_CURRENT_DIR_PLACEMENT CurrentDirPlacement, OldCurrentDirPlacement;

    /* Acquire DLL directory lock */
    RtlEnterCriticalSection(&BaseDllDirectoryLock);

    /* Check if we have a base dll directory */
    if (BaseDllDirectory.Buffer)
    {
        /* Then compute the process path using DLL order (without curdir) */
        DllPath = BasepComputeProcessPath(BaseDllOrderNoCurrent, FullPath, Environment);

        /* Release DLL directory lock */
        RtlLeaveCriticalSection(&BaseDllDirectoryLock);

        /* Return dll path */
        return DllPath;
    }

    /* Release DLL directory lock */
    RtlLeaveCriticalSection(&BaseDllDirectoryLock);

    /* Read the current placement */
    CurrentDirPlacement = BasepDllCurrentDirPlacement;
    if (CurrentDirPlacement == BaseCurrentDirPlacementInvalid)
    {
        /* Open the configuration key */
        Status = NtOpenKey(&KeyHandle, KEY_QUERY_VALUE, &ObjectAttributes);
        if (NT_SUCCESS(Status))
        {
            /* Query if safe search is enabled */
            Status = NtQueryValueKey(KeyHandle,
                                     &ValueName,
                                     KeyValuePartialInformation,
                                     &PartialInfo,
                                     sizeof(PartialInfo),
                                     &ResultLength);
            if (NT_SUCCESS(Status))
            {
                /* Read the value if the size is OK */
                if (ResultLength == sizeof(PartialInfo))
                {
                    CurrentDirPlacement = *(PULONG)PartialInfo.Data;
                }
            }

            /* Close the handle */
            NtClose(KeyHandle);

            /* Validate the registry value */
            if ((CurrentDirPlacement <= BaseCurrentDirPlacementInvalid) ||
                (CurrentDirPlacement >= BaseCurrentDirPlacementMax))
            {
                /* Default to safe search */
                CurrentDirPlacement = BaseCurrentDirPlacementSafe;
            }
        }

        /* Update the placement and read the old one */
        OldCurrentDirPlacement = InterlockedCompareExchange((PLONG)&BasepDllCurrentDirPlacement,
                                                            CurrentDirPlacement,
                                                            BaseCurrentDirPlacementInvalid);
        if (OldCurrentDirPlacement != BaseCurrentDirPlacementInvalid)
        {
            /* If there already was a placement, use it */
            CurrentDirPlacement = OldCurrentDirPlacement;
        }
    }

    /* Check if the placement is invalid or not set */
    if ((CurrentDirPlacement <= BaseCurrentDirPlacementInvalid) ||
        (CurrentDirPlacement >= BaseCurrentDirPlacementMax))
    {
        /* Default to safe search */
        CurrentDirPlacement = BaseCurrentDirPlacementSafe;
    }

    /* Compute the process path using either normal or safe search */
    DllPath = BasepComputeProcessPath(BaseDllOrderCurrent[CurrentDirPlacement],
                                      FullPath,
                                      Environment);

    /* Return dll path */
    return DllPath;
}

BOOLEAN
WINAPI
CheckForSameCurdir(IN PUNICODE_STRING DirName)
{
    PUNICODE_STRING CurDir;
    USHORT CurLength;
    BOOLEAN Result;
    UNICODE_STRING CurDirCopy;

    CurDir = &NtCurrentPeb()->ProcessParameters->CurrentDirectory.DosPath;

    CurLength = CurDir->Length;
    if (CurDir->Length <= 6)
    {
        if (CurLength != DirName->Length) return FALSE;
    }
    else
    {
        if ((CurLength - 2) != DirName->Length) return FALSE;
    }

    RtlAcquirePebLock();

    CurDirCopy = *CurDir;
    if (CurDirCopy.Length > 6) CurDirCopy.Length -= 2;

    Result = 0;

    if (RtlEqualUnicodeString(&CurDirCopy, DirName, TRUE)) Result = TRUE;

    RtlReleasePebLock();

    return Result;
}

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
    UCHAR c;
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

    /* This could be . or .. or something else */
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
    RtlInitEmptyUnicodeString(&UnicodeName, Name, (USHORT)Length * sizeof(WCHAR));
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
        if ((c > 0x7F) || (IllegalMask[c / 32] & (1 << (c % 32))))
        {
            return FALSE;
        }

        /* Check if this is perhaps an extension? */
        if (c == '.')
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
        if (FileName[i] == L'.')
        {
            /* Unlike the short case, we WANT more than one extension, or a long one */
            if ((HasExtension) || (Dots > 3))
            {
                return TRUE;
            }
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
        while ((*Path == L'\\') || (*Path == L'/')) Path++;

        /* Make sure there's something after the slashes too! */
        if (*Path == UNICODE_NULL) break;

        /* Now skip past the file name until we get to the first slash */
        p = Path + 1;
        while ((*p) && ((*p != L'\\') && (*p != L'/'))) p++;

        /* Whatever is in between those two is now the file name length */
        Length = p - Path;

        /*
         * Check if it is valid
         * Note that !IsShortName != IsLongName, these two functions simply help
         * us determine if a conversion is necessary or not.
         * "Found" really means: "Is a conversion necessary?", hence the "!"
         */
        Found = UseShort ? !IsShortName_U(Path, Length) : !IsLongName_U(Path, Length);
        if (Found)
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
    return Found;
}

PWCHAR
WINAPI
SkipPathTypeIndicator_U(IN LPWSTR Path)
{
    PWCHAR ReturnPath;
    ULONG i;

    /* Check what kind of path this is and how many slashes to skip */
    switch (RtlDetermineDosPathNameType_U(Path))
    {
        case RtlPathTypeUncAbsolute:
        case RtlPathTypeLocalDevice:
        {
            /* Keep going until we bypass the path indicators */
            for (ReturnPath = Path + 2, i = 2; (i > 0) && (*ReturnPath); ReturnPath++)
            {
                /* We look for 2 slashes, so keep at it until we find them */
                if ((*ReturnPath == L'\\') || (*ReturnPath == L'/')) i--;
            }

            return ReturnPath;
        }

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

/* PUBLIC FUNCTIONS ***********************************************************/

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

    RtlInitEmptyAnsiString(&AnsiDllDirectory, lpBuffer, (USHORT)nBufferLength);

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
 *
 * NOTE: Many of these A functions may seem to do rather complex A<->W mapping
 * beyond what you would usually expect. There are two main reasons:
 *
 * First, these APIs are subject to the ANSI/OEM File API selection status that
 * the caller has chosen, so we must use the "8BitString" internal Base APIs.
 *
 * Secondly, the Wide APIs (coming from the 9x world) are coded to return the
 * length of the paths in "ANSI" by dividing their internal Wide character count
 * by two... this is usually correct when dealing with pure-ASCII codepages but
 * not necessarily when dealing with MBCS pre-Unicode sets, which NT supports
 * for CJK, for example.
 */
DWORD
WINAPI
GetFullPathNameA(IN LPCSTR lpFileName,
                 IN DWORD nBufferLength,
                 OUT LPSTR lpBuffer,
                 OUT LPSTR *lpFilePart)
{
    NTSTATUS Status;
    PWCHAR Buffer = NULL;
    ULONG PathSize, FilePartSize;
    ANSI_STRING AnsiString;
    UNICODE_STRING FileNameString, UniString;
    PWCHAR LocalFilePart;
    PWCHAR* FilePart;

    /* If the caller wants filepart, use a local wide buffer since this is A */
    FilePart = lpFilePart != NULL ? &LocalFilePart : NULL;

    /* Initialize for Quickie */
    FilePartSize = PathSize = 0;
    FileNameString.Buffer = NULL;

    /* First get our string in Unicode */
    Status = Basep8BitStringToDynamicUnicodeString(&FileNameString, lpFileName);
    if (!NT_SUCCESS(Status)) goto Quickie;

    /* Allocate a buffer to hold teh path name */
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(),
                             0,
                             MAX_PATH * sizeof(WCHAR) + sizeof(UNICODE_NULL));
    if (!Buffer)
    {
        BaseSetLastNTError(STATUS_INSUFFICIENT_RESOURCES);
        goto Quickie;
    }

    /* Call into RTL to get the full Unicode path name */
    PathSize = RtlGetFullPathName_U(FileNameString.Buffer,
                                    MAX_PATH * sizeof(WCHAR),
                                    Buffer,
                                    FilePart);
    if (PathSize <= (MAX_PATH * sizeof(WCHAR)))
    {
        /* The buffer will fit, get the real ANSI string size now */
        Status = RtlUnicodeToMultiByteSize(&PathSize, Buffer, PathSize);
        if (NT_SUCCESS(Status))
        {
            /* Now check if the user wanted file part size as well */
            if ((PathSize) && (lpFilePart) && (LocalFilePart))
            {
                /* Yep, so in this case get the length of the file part too */
                Status = RtlUnicodeToMultiByteSize(&FilePartSize,
                                                   Buffer,
                                                   (LocalFilePart - Buffer) *
                                                   sizeof(WCHAR));
                if (!NT_SUCCESS(Status))
                {
                    /* We failed to do that, so fail the whole call */
                    BaseSetLastNTError(Status);
                    PathSize = 0;
                }
            }
        }
    }
    else
    {
        /* Reset the path size since the buffer is not large enough */
        PathSize = 0;
    }

    /* Either no path, or local buffer was too small, enter failure code */
    if (!PathSize) goto Quickie;

    /* If the *caller's* buffer was too small, fail, but add in space for NULL */
    if (PathSize >= nBufferLength)
    {
        PathSize++;
        goto Quickie;
    }

    /* So far so good, initialize a unicode string to convert back to ANSI/OEM */
    RtlInitUnicodeString(&UniString, Buffer);
    Status = BasepUnicodeStringTo8BitString(&AnsiString, &UniString, TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* Final conversion failed, fail the call */
        BaseSetLastNTError(Status);
        PathSize = 0;
    }
    else
    {
        /* Conversion worked, now copy the ANSI/OEM buffer into the buffer */
        RtlCopyMemory(lpBuffer, AnsiString.Buffer, PathSize + 1);
        RtlFreeAnsiString(&AnsiString);

        /* And finally, did the caller request file part information? */
        if (lpFilePart)
        {
            /* Use the size we computed earlier and add it to the buffer */
            *lpFilePart = LocalFilePart ? &lpBuffer[FilePartSize] : 0;
        }
    }

Quickie:
    /* Cleanup and return the path size */
    if (FileNameString.Buffer) RtlFreeUnicodeString(&FileNameString);
    if (Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    return PathSize;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetFullPathNameW(IN LPCWSTR lpFileName,
                 IN DWORD nBufferLength,
                 OUT LPWSTR lpBuffer,
                 OUT LPWSTR *lpFilePart)
{
    /* Call Rtl to do the work */
    return RtlGetFullPathName_U(lpFileName,
                                nBufferLength * sizeof(WCHAR),
                                lpBuffer,
                                lpFilePart) / sizeof(WCHAR);
}

/*
 * @implemented
 */
DWORD
WINAPI
SearchPathA(IN LPCSTR lpPath,
            IN LPCSTR lpFileName,
            IN LPCSTR lpExtension,
            IN DWORD nBufferLength,
            IN LPSTR lpBuffer,
            OUT LPSTR *lpFilePart)
{
    PUNICODE_STRING FileNameString;
    UNICODE_STRING PathString, ExtensionString;
    NTSTATUS Status;
    ULONG PathSize, FilePartSize, AnsiLength;
    PWCHAR LocalFilePart, Buffer;
    PWCHAR* FilePart;

    /* If the caller wants filepart, use a local wide buffer since this is A */
    FilePart = lpFilePart != NULL ? &LocalFilePart : NULL;

    /* Initialize stuff for Quickie */
    PathSize = 0;
    Buffer = NULL;
    ExtensionString.Buffer = PathString.Buffer = NULL;

    /* Get the UNICODE_STRING file name */
    FileNameString = Basep8BitStringToStaticUnicodeString(lpFileName);
    if (!FileNameString) return 0;

    /* Did the caller specify an extension */
    if (lpExtension)
    {
        /* Yup, convert it into UNICODE_STRING */
        Status = Basep8BitStringToDynamicUnicodeString(&ExtensionString,
                                                       lpExtension);
        if (!NT_SUCCESS(Status)) goto Quickie;
    }

    /* Did the caller specify a path */
    if (lpPath)
    {
        /* Yup, convert it into UNICODE_STRING */
        Status = Basep8BitStringToDynamicUnicodeString(&PathString, lpPath);
        if (!NT_SUCCESS(Status)) goto Quickie;
    }

    /* Allocate our output buffer */
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, nBufferLength * sizeof(WCHAR));
    if (!Buffer)
    {
        /* It failed, bail out */
        BaseSetLastNTError(STATUS_NO_MEMORY);
        goto Quickie;
    }

    /* Now run the Wide search with the input buffer lengths */
    PathSize = SearchPathW(PathString.Buffer,
                           FileNameString->Buffer,
                           ExtensionString.Buffer,
                           nBufferLength,
                           Buffer,
                           FilePart);
    if (PathSize <= nBufferLength)
    {
        /* It fits, but is it empty? If so, bail out */
        if (!PathSize) goto Quickie;

        /* The length above is inexact, we need it in ANSI */
        Status = RtlUnicodeToMultiByteSize(&AnsiLength, Buffer, PathSize * sizeof(WCHAR));
        if (!NT_SUCCESS(Status))
        {
            /* Conversion failed, fail the call */
            PathSize = 0;
            BaseSetLastNTError(Status);
            goto Quickie;
        }

        /* If the correct ANSI size is too big, return requird length plus a NULL */
        if (AnsiLength >= nBufferLength)
        {
            PathSize = AnsiLength + 1;
            goto Quickie;
        }

        /* Now apply the final conversion to ANSI */
        Status = RtlUnicodeToMultiByteN(lpBuffer,
                                        nBufferLength - 1,
                                        &AnsiLength,
                                        Buffer,
                                        PathSize * sizeof(WCHAR));
        if (!NT_SUCCESS(Status))
        {
            /* Conversion failed, fail the whole call */
            PathSize = 0;
            BaseSetLastNTError(STATUS_NO_MEMORY);
            goto Quickie;
        }

        /* NULL-terminate and return the real ANSI length */
        lpBuffer[AnsiLength] = ANSI_NULL;
        PathSize = AnsiLength;

        /* Now check if the user wanted file part size as well */
        if (lpFilePart)
        {
            /* If we didn't get a file part, clear the caller's */
            if (!LocalFilePart)
            {
                *lpFilePart = NULL;
            }
            else
            {
                /* Yep, so in this case get the length of the file part too */
                Status = RtlUnicodeToMultiByteSize(&FilePartSize,
                                                   Buffer,
                                                   (LocalFilePart - Buffer) *
                                                   sizeof(WCHAR));
                if (!NT_SUCCESS(Status))
                {
                    /* We failed to do that, so fail the whole call */
                    BaseSetLastNTError(Status);
                    PathSize = 0;
                }

                /* Return the file part buffer */
                *lpFilePart = lpBuffer + FilePartSize;
            }
        }
    }
    else
    {
        /* Our initial buffer guess was too small, allocate a bigger one */
        RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
        Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, PathSize * sizeof(WCHAR));
        if (!Buffer)
        {
            /* Out of memory, fail everything */
            BaseSetLastNTError(STATUS_NO_MEMORY);
            goto Quickie;
        }

        /* Do the search again -- it will fail, we just want the path size */
        PathSize = SearchPathW(PathString.Buffer,
                               FileNameString->Buffer,
                               ExtensionString.Buffer,
                               PathSize,
                               Buffer,
                               FilePart);
        if (!PathSize) goto Quickie;

        /* Convert it to a correct size */
        Status = RtlUnicodeToMultiByteSize(&PathSize, Buffer, PathSize * sizeof(WCHAR));
        if (NT_SUCCESS(Status))
        {
            /* Make space for the NULL-char */
            PathSize++;
        }
        else
        {
            /* Conversion failed for some reason, fail the call */
            BaseSetLastNTError(Status);
            PathSize = 0;
        }
    }

Quickie:
    /* Cleanup/complete path */
    if (Buffer) RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
    if (ExtensionString.Buffer) RtlFreeUnicodeString(&ExtensionString);
    if (PathString.Buffer) RtlFreeUnicodeString(&PathString);
    return PathSize;
}

/*
 * @implemented
 */
DWORD
WINAPI
SearchPathW(IN LPCWSTR lpPath,
            IN LPCWSTR lpFileName,
            IN LPCWSTR lpExtension,
            IN DWORD nBufferLength,
            IN LPWSTR lpBuffer,
            OUT LPWSTR *lpFilePart)
{
    UNICODE_STRING FileNameString, ExtensionString, PathString, CallerBuffer;
    ULONG Flags, LengthNeeded, FilePartSize;
    NTSTATUS Status;
    DWORD Result = 0;

    /* Default flags for RtlDosSearchPath_Ustr */
    Flags = 6;

    /* Clear file part in case we fail */
    if (lpFilePart) *lpFilePart = NULL;

    /* Initialize path buffer for free later */
    PathString.Buffer = NULL;

    /* Convert filename to a unicode string and eliminate trailing spaces */
    RtlInitUnicodeString(&FileNameString, lpFileName);
    while ((FileNameString.Length >= sizeof(WCHAR)) &&
           (FileNameString.Buffer[(FileNameString.Length / sizeof(WCHAR)) - 1] == L' '))
    {
        FileNameString.Length -= sizeof(WCHAR);
    }

    /* Was it all just spaces? */
    if (!FileNameString.Length)
    {
        /* Fail out */
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        goto Quickie;
    }

    /* Convert extension to a unicode string */
    RtlInitUnicodeString(&ExtensionString, lpExtension);

    /* Check if the user sent a path */
    if (lpPath)
    {
        /* Convert it to a unicode string too */
        Status = RtlInitUnicodeStringEx(&PathString, lpPath);
        if (NT_ERROR(Status))
        {
            /* Fail if it was too long */
            BaseSetLastNTError(Status);
            goto Quickie;
        }
    }
    else
    {
        /* A path wasn't sent, so compute it ourselves */
        PathString.Buffer = BaseComputeProcessSearchPath();
        if (!PathString.Buffer)
        {
            /* Fail if we couldn't compute it */
            BaseSetLastNTError(STATUS_NO_MEMORY);
            goto Quickie;
        }

        /* See how big the computed path is */
        LengthNeeded = lstrlenW(PathString.Buffer);
        if (LengthNeeded > UNICODE_STRING_MAX_CHARS)
        {
            /* Fail if it's too long */
            BaseSetLastNTError(STATUS_NAME_TOO_LONG);
            goto Quickie;
        }

        /* Set the path size now that we have it */
        PathString.MaximumLength = PathString.Length = (USHORT)LengthNeeded * sizeof(WCHAR);

        /* Request SxS isolation from RtlDosSearchPath_Ustr */
        Flags |= 1;
    }

    /* Create the string that describes the output buffer from the caller */
    CallerBuffer.Length = 0;
    CallerBuffer.Buffer = lpBuffer;

    /* How much space does the caller have? */
    if (nBufferLength <= UNICODE_STRING_MAX_CHARS)
    {
        /* Add it into the string */
        CallerBuffer.MaximumLength = (USHORT)nBufferLength * sizeof(WCHAR);
    }
    else
    {
        /* Caller wants too much, limit it to the maximum length of a string */
        CallerBuffer.MaximumLength = UNICODE_STRING_MAX_BYTES;
    }

    /* Call Rtl to do the work */
    Status = RtlDosSearchPath_Ustr(Flags,
                                   &PathString,
                                   &FileNameString,
                                   &ExtensionString,
                                   &CallerBuffer,
                                   NULL,
                                   NULL,
                                   &FilePartSize,
                                   &LengthNeeded);
    if (NT_ERROR(Status))
    {
        /* Check for unusual status codes */
        if ((Status != STATUS_NO_SUCH_FILE) && (Status != STATUS_BUFFER_TOO_SMALL))
        {
            /* Print them out since maybe an app needs fixing */
            DbgPrint("%s on file %wZ failed; NTSTATUS = %08lx\n",
                     __FUNCTION__,
                     &FileNameString,
                     Status);
            DbgPrint("    Path = %wZ\n", &PathString);
        }

        /* Check if the failure was due to a small buffer */
        if (Status == STATUS_BUFFER_TOO_SMALL)
        {
            /* Check if the length was actually too big for Rtl to work with */
            Result = LengthNeeded / sizeof(WCHAR);
            if (Result > 0xFFFFFFFF) BaseSetLastNTError(STATUS_NAME_TOO_LONG);
        }
        else
        {
            /* Some other error, set the error code */
            BaseSetLastNTError(Status);
        }
    }
    else
    {
        /* It worked! Write the file part now */
        if (lpFilePart) *lpFilePart = &lpBuffer[FilePartSize];

        /* Convert the final result length */
        Result = CallerBuffer.Length / sizeof(WCHAR);
    }

Quickie:
    /* Check if there was a dynamic path string to free */
    if ((PathString.Buffer != lpPath) && (PathString.Buffer))
    {
        /* And free it */
        RtlFreeHeap(RtlGetProcessHeap(), 0, PathString.Buffer);
    }

    /* Return the final result length */
    return Result;
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
    BOOLEAN Found = FALSE;
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
    if (GetFileAttributesW(lpszShortPath) == INVALID_FILE_ATTRIBUTES)
    {
        /* It doesn't, so fail */
        ReturnLength = 0;
        goto Quickie;
    }

    /* Now get a pointer to the actual path, skipping indicators */
    Path = SkipPathTypeIndicator_U((LPWSTR)lpszShortPath);

    /* Is there any path or filename in there? */
    if (!(Path) ||
        (*Path == UNICODE_NULL) ||
        !(FindLFNorSFN_U(Path, &First, &Last, FALSE)))
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

    /* Make a copy of it */
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

    ShortPathUni.MaximumLength = (USHORT)PathLength * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    LongPathUni.Buffer = LongPath;
    LongPathUni.Length = (USHORT)PathLength * sizeof(WCHAR);

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
            PathLength = GetShortPathNameW(LongPathUni.Buffer, ShortPath, PathLength);
        }
    }

    if (!PathLength) goto Quickie;

    LongPathUni.MaximumLength = (USHORT)PathLength * sizeof(WCHAR) + sizeof(UNICODE_NULL);
    ShortPathUni.Buffer = ShortPath;
    ShortPathUni.Length = (USHORT)PathLength * sizeof(WCHAR);

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
    BOOLEAN Found = FALSE;
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
    if (GetFileAttributesW(lpszLongPath) == INVALID_FILE_ATTRIBUTES)
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
    Path = SkipPathTypeIndicator_U((LPWSTR)lpszLongPath);

    /* Is there any path or filename in there? */
    if (!(Path) ||
        (*Path == UNICODE_NULL) ||
        !(FindLFNorSFN_U(Path, &First, &Last, TRUE)))
    {
        /* There isn't, so the long path is simply the short path */
        ReturnLength = wcslen(lpszLongPath);

        /* Is there space for it? */
        if ((cchBuffer > ReturnLength) && (lpszShortPath))
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

/*
 * @implemented
 *
 * NOTE: Windows returns a dos/short (8.3) path
 */
DWORD
WINAPI
GetTempPathA(IN DWORD nBufferLength,
             IN LPSTR lpBuffer)
{
   WCHAR BufferW[MAX_PATH];
   DWORD ret;

   ret = GetTempPathW(MAX_PATH, BufferW);

   if (!ret) return 0;

   if (ret > MAX_PATH)
   {
      SetLastError(ERROR_FILENAME_EXCED_RANGE);
      return 0;
   }

   return FilenameW2A_FitOrFail(lpBuffer, nBufferLength, BufferW, ret+1);
}

/*
 * @implemented
 *
 * ripped from wine
 */
DWORD
WINAPI
GetTempPathW(IN DWORD count,
             IN LPWSTR path)
{
    static const WCHAR tmp[]  = { 'T', 'M', 'P', 0 };
    static const WCHAR temp[] = { 'T', 'E', 'M', 'P', 0 };
    static const WCHAR userprofile[] = { 'U','S','E','R','P','R','O','F','I','L','E',0 };
    WCHAR tmp_path[MAX_PATH];
    WCHAR full_tmp_path[MAX_PATH];
    UINT ret;

    DPRINT("%u,%p\n", count, path);

    if (!(ret = GetEnvironmentVariableW( tmp, tmp_path, MAX_PATH )) &&
        !(ret = GetEnvironmentVariableW( temp, tmp_path, MAX_PATH )) &&
        !(ret = GetEnvironmentVariableW( userprofile, tmp_path, MAX_PATH )) &&
        !(ret = GetWindowsDirectoryW( tmp_path, MAX_PATH )))
    {
        return 0;
    }

    if (ret > MAX_PATH)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }

    ret = GetFullPathNameW(tmp_path, MAX_PATH, full_tmp_path, NULL);
    if (!ret) return 0;

    if (ret > MAX_PATH - 2)
    {
        SetLastError(ERROR_FILENAME_EXCED_RANGE);
        return 0;
    }

    if (full_tmp_path[ret-1] != '\\')
    {
        full_tmp_path[ret++] = '\\';
        full_tmp_path[ret]   = '\0';
    }

    ret++; /* add space for terminating 0 */

    if (count >= ret)
    {
        lstrcpynW(path, tmp_path, count);
        /* the remaining buffer must be zeroed up to 32766 bytes in XP or 32767
         * bytes after it, we will assume the > XP behavior for now */
        memset(path + ret, 0, (min(count, 32767) - ret) * sizeof(WCHAR));
        ret--; /* return length without 0 */
    }
    else if (count)
    {
        /* the buffer must be cleared if contents will not fit */
        memset(path, 0, count * sizeof(WCHAR));
    }

    DPRINT("GetTempPathW returning %u, %S\n", ret, path);
    return ret;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetCurrentDirectoryA(IN DWORD nBufferLength,
                     IN LPSTR lpBuffer)
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    PUNICODE_STRING StaticString;
    ULONG MaxLength;

    StaticString = &NtCurrentTeb()->StaticUnicodeString;

    MaxLength = nBufferLength;
    if (nBufferLength >= UNICODE_STRING_MAX_BYTES)
    {
        MaxLength = UNICODE_STRING_MAX_BYTES - 1;
    }

    StaticString->Length = (USHORT)RtlGetCurrentDirectory_U(StaticString->MaximumLength,
                                                            StaticString->Buffer);
    Status = RtlUnicodeToMultiByteSize(&nBufferLength,
                                       StaticString->Buffer,
                                       StaticString->Length);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    if (MaxLength <= nBufferLength)
    {
        return nBufferLength + 1;
    }

    AnsiString.Buffer = lpBuffer;
    AnsiString.MaximumLength = (USHORT)MaxLength;
    Status = BasepUnicodeStringTo8BitString(&AnsiString, StaticString, FALSE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return AnsiString.Length;
}

/*
 * @implemented
 */
DWORD
WINAPI
GetCurrentDirectoryW(IN DWORD nBufferLength,
                     IN LPWSTR lpBuffer)
{
    return RtlGetCurrentDirectory_U(nBufferLength * sizeof(WCHAR), lpBuffer) / sizeof(WCHAR);
}

/*
 * @implemented
 */
BOOL
WINAPI
SetCurrentDirectoryA(IN LPCSTR lpPathName)
{
    PUNICODE_STRING DirName;
    NTSTATUS Status;

    if (!lpPathName)
    {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    DirName = Basep8BitStringToStaticUnicodeString(lpPathName);
    if (!DirName) return FALSE;

    if (CheckForSameCurdir(DirName)) return TRUE;

    Status = RtlSetCurrentDirectory_U(DirName);
    if (NT_SUCCESS(Status)) return TRUE;

    if ((*DirName->Buffer != L'"') || (DirName->Length <= 2))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    DirName = Basep8BitStringToStaticUnicodeString(lpPathName + 1);
    if (!DirName) return FALSE;

    Status = RtlSetCurrentDirectory_U(DirName);
    if (!NT_SUCCESS(Status))
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
SetCurrentDirectoryW(IN LPCWSTR lpPathName)
{
    NTSTATUS Status;
    UNICODE_STRING UnicodeString;

    if (!lpPathName)
    {
        BaseSetLastNTError(STATUS_INVALID_PARAMETER);
        return FALSE;
    }

    Status = RtlInitUnicodeStringEx(&UnicodeString, lpPathName);
    if (NT_SUCCESS(Status))
    {
        if (!CheckForSameCurdir(&UnicodeString))
        {
            Status = RtlSetCurrentDirectory_U(&UnicodeString);
        }
    }

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
UINT
WINAPI
GetSystemDirectoryA(IN LPSTR lpBuffer,
                    IN UINT uSize)
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    ULONG AnsiLength;

    /* Get the correct size of the Unicode Base directory */
    Status = RtlUnicodeToMultiByteSize(&AnsiLength,
                                       BaseWindowsSystemDirectory.Buffer,
                                       BaseWindowsSystemDirectory.MaximumLength);
    if (!NT_SUCCESS(Status)) return 0;

    if (uSize < AnsiLength) return AnsiLength;

    RtlInitEmptyAnsiString(&AnsiString, lpBuffer, uSize);

    Status = BasepUnicodeStringTo8BitString(&AnsiString,
                                            &BaseWindowsSystemDirectory,
                                            FALSE);
    if (!NT_SUCCESS(Status)) return 0;

    return AnsiString.Length;
}

/*
 * @implemented
 */
UINT
WINAPI
GetSystemDirectoryW(IN LPWSTR lpBuffer,
                    IN UINT uSize)
{
    ULONG ReturnLength;

    ReturnLength = BaseWindowsSystemDirectory.MaximumLength;
    if ((uSize * sizeof(WCHAR)) >= ReturnLength)
    {
        RtlCopyMemory(lpBuffer,
                      BaseWindowsSystemDirectory.Buffer,
                      BaseWindowsSystemDirectory.Length);
        lpBuffer[BaseWindowsSystemDirectory.Length / sizeof(WCHAR)] = ANSI_NULL;

        ReturnLength = BaseWindowsSystemDirectory.Length;
    }

    return ReturnLength / sizeof(WCHAR);
}

/*
 * @implemented
 */
UINT
WINAPI
GetWindowsDirectoryA(IN LPSTR lpBuffer,
                     IN UINT uSize)
{
    /* Is this a TS installation? */
    if (gpTermsrvGetWindowsDirectoryA) UNIMPLEMENTED;

    /* Otherwise, call the System API */
    return GetSystemWindowsDirectoryA(lpBuffer, uSize);
}

/*
 * @implemented
 */
UINT
WINAPI
GetWindowsDirectoryW(IN LPWSTR lpBuffer,
                     IN UINT uSize)
{
    /* Is this a TS installation? */
    if (gpTermsrvGetWindowsDirectoryW) UNIMPLEMENTED;

    /* Otherwise, call the System API */
    return GetSystemWindowsDirectoryW(lpBuffer, uSize);
}

/*
 * @implemented
 */
UINT
WINAPI
GetSystemWindowsDirectoryA(IN LPSTR lpBuffer,
                           IN UINT uSize)
{
    ANSI_STRING AnsiString;
    NTSTATUS Status;
    ULONG AnsiLength;

    /* Get the correct size of the Unicode Base directory */
    Status = RtlUnicodeToMultiByteSize(&AnsiLength,
                                       BaseWindowsDirectory.Buffer,
                                       BaseWindowsDirectory.MaximumLength);
    if (!NT_SUCCESS(Status)) return 0;

    if (uSize < AnsiLength) return AnsiLength;

    RtlInitEmptyAnsiString(&AnsiString, lpBuffer, uSize);

    Status = BasepUnicodeStringTo8BitString(&AnsiString,
                                            &BaseWindowsDirectory,
                                            FALSE);
    if (!NT_SUCCESS(Status)) return 0;

    return AnsiString.Length;
}

/*
 * @implemented
 */
UINT
WINAPI
GetSystemWindowsDirectoryW(IN LPWSTR lpBuffer,
                           IN UINT uSize)
{
    ULONG ReturnLength;

    ReturnLength = BaseWindowsDirectory.MaximumLength;
    if ((uSize * sizeof(WCHAR)) >= ReturnLength)
    {
        RtlCopyMemory(lpBuffer,
                      BaseWindowsDirectory.Buffer,
                      BaseWindowsDirectory.Length);
        lpBuffer[BaseWindowsDirectory.Length / sizeof(WCHAR)] = ANSI_NULL;

        ReturnLength = BaseWindowsDirectory.Length;
    }

    return ReturnLength / sizeof(WCHAR);
}

/*
 * @unimplemented
 */
UINT
WINAPI
GetSystemWow64DirectoryW(IN LPWSTR lpBuffer,
                         IN UINT uSize)
{
#ifdef _WIN64
    UNIMPLEMENTED;
    return 0;
#else
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
#endif
}

/*
 * @unimplemented
 */
UINT
WINAPI
GetSystemWow64DirectoryA(IN LPSTR lpBuffer,
                         IN UINT uSize)
{
#ifdef _WIN64
    UNIMPLEMENTED;
    return 0;
#else
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
#endif
}

/* EOF */
