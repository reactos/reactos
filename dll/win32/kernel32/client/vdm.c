/*
 * PROJECT:         ReactOS Win32 Base API
 * LICENSE:         GPL - See COPYING in the top level directory
 * FILE:            dll/win32/kernel32/client/vdm.c
 * PURPOSE:         Virtual DOS Machines (VDM) Support
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>

/* TYPES **********************************************************************/

#define BINARY_UNKNOWN  (0)
#define BINARY_PE_EXE32 (1)
#define BINARY_PE_DLL32 (2)
#define BINARY_PE_EXE64 (3)
#define BINARY_PE_DLL64 (4)
#define BINARY_WIN16    (5)
#define BINARY_OS216    (6)
#define BINARY_DOS      (7)
#define BINARY_UNIX_EXE (8)
#define BINARY_UNIX_LIB (9)


typedef enum _ENV_NAME_TYPE
{
    EnvNameNotAPath = 1,
    EnvNameSinglePath ,
    EnvNameMultiplePath
} ENV_NAME_TYPE;

typedef struct _ENV_INFO
{
    ENV_NAME_TYPE NameType;
    ULONG  NameLength;
    PWCHAR Name;
} ENV_INFO, *PENV_INFO;

/* GLOBALS ********************************************************************/

#define ENV_NAME_ENTRY(type, name)  \
    {(type), _ARRAYSIZE(name) - 1, (name)}

static ENV_INFO BasepEnvNameType[] =
{
    ENV_NAME_ENTRY(EnvNameMultiplePath, L"PATH"),
    ENV_NAME_ENTRY(EnvNameSinglePath  , L"WINDIR"),
    ENV_NAME_ENTRY(EnvNameSinglePath  , L"SYSTEMROOT"),
    ENV_NAME_ENTRY(EnvNameMultiplePath, L"TEMP"),
    ENV_NAME_ENTRY(EnvNameMultiplePath, L"TMP"),
};

static UNICODE_STRING BaseDotComSuffixName = RTL_CONSTANT_STRING(L".com");
static UNICODE_STRING BaseDotPifSuffixName = RTL_CONSTANT_STRING(L".pif");
static UNICODE_STRING BaseDotExeSuffixName = RTL_CONSTANT_STRING(L".exe");

/* FUNCTIONS ******************************************************************/

ULONG
WINAPI
BaseIsDosApplication(IN PUNICODE_STRING PathName,
                     IN NTSTATUS Status)
{
    UNICODE_STRING String;

    /* Is it a .com? */
    String.Length = BaseDotComSuffixName.Length;
    String.Buffer = &PathName->Buffer[(PathName->Length - String.Length) / sizeof(WCHAR)];
    if (RtlEqualUnicodeString(&String, &BaseDotComSuffixName, TRUE)) return BINARY_TYPE_COM;

    /* Is it a .pif? */
    String.Length = BaseDotPifSuffixName.Length;
    String.Buffer = &PathName->Buffer[(PathName->Length - String.Length) / sizeof(WCHAR)];
    if (RtlEqualUnicodeString(&String, &BaseDotPifSuffixName, TRUE)) return BINARY_TYPE_PIF;

    /* Is it an exe? */
    String.Length = BaseDotExeSuffixName.Length;
    String.Buffer = &PathName->Buffer[(PathName->Length - String.Length) / sizeof(WCHAR)];
    if (RtlEqualUnicodeString(&String, &BaseDotExeSuffixName, TRUE)) return BINARY_TYPE_EXE;

    return 0;
}

NTSTATUS
WINAPI
BaseCheckVDM(IN ULONG BinaryType,
             IN PCWCH ApplicationName,
             IN PCWCH CommandLine,
             IN PCWCH CurrentDirectory,
             IN PANSI_STRING AnsiEnvironment,
             IN PBASE_API_MESSAGE ApiMessage,
             IN OUT PULONG iTask,
             IN DWORD CreationFlags,
             IN LPSTARTUPINFOW StartupInfo,
             IN HANDLE hUserToken OPTIONAL)
{
    NTSTATUS Status;
    PBASE_CHECK_VDM CheckVdm = &ApiMessage->Data.CheckVDMRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;
    PWCHAR CurrentDir = NULL;
    PWCHAR ShortAppName = NULL;
    PWCHAR ShortCurrentDir = NULL;
    SIZE_T Length;
    PCHAR AnsiCmdLine = NULL;
    PCHAR AnsiAppName = NULL;
    PCHAR AnsiCurDirectory = NULL;
    PCHAR AnsiDesktop = NULL;
    PCHAR AnsiTitle = NULL;
    PCHAR AnsiReserved = NULL;
    STARTUPINFOA AnsiStartupInfo;
    ULONG NumStrings = 5;

    /* Parameters validation */
    if (ApplicationName == NULL || CommandLine == NULL)
    {
        return STATUS_INVALID_PARAMETER;
    }

    /* Trim leading whitespace from ApplicationName */
    while (*ApplicationName == L' ' || *ApplicationName == L'\t')
        ++ApplicationName;

    /* Calculate the size of the short application name */
    Length = GetShortPathNameW(ApplicationName, NULL, 0);
    if (Length == 0)
    {
        Status = STATUS_OBJECT_PATH_INVALID;
        goto Cleanup;
    }

    /* Allocate memory for the short application name */
    ShortAppName = (PWCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           Length * sizeof(WCHAR));
    if (!ShortAppName)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    /* Get the short application name */
    if (GetShortPathNameW(ApplicationName, ShortAppName, Length) == 0)
    {
        /* Try to determine which error occurred */
        switch (GetLastError())
        {
            case ERROR_NOT_ENOUGH_MEMORY:
            {
                Status = STATUS_NO_MEMORY;
                break;
            }

            case ERROR_INVALID_PARAMETER:
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            default:
            {
                Status = STATUS_OBJECT_PATH_INVALID;
            }
        }

        goto Cleanup;
    }

    /* Trim leading whitespace from CommandLine */
    while (*CommandLine == L' ' || *CommandLine == L'\t')
        ++CommandLine;

    /*
     * CommandLine is usually formatted as: 'ApplicationName param0 ...'.
     * So we want to strip the first token (ApplicationName) from it.
     * Two cases are in fact possible:
     * - either the first token is indeed ApplicationName, so we just skip it;
     * - or the first token is not exactly ApplicationName, because it happened
     *   that somebody else already preprocessed CommandLine. Therefore we
     *   suppose that the first token corresponds to an application name and
     *   we skip it. Care should be taken when quotes are present in this token.
     */
     if (*CommandLine)
     {
        /* The first part of CommandLine should be the ApplicationName... */
        Length = wcslen(ApplicationName);
        if (Length <= wcslen(CommandLine) &&
            _wcsnicmp(ApplicationName, CommandLine, Length) == 0)
        {
            /* Skip it */
            CommandLine += Length;
        }
        /*
         * ... but it is not, however we still have a token. We suppose that
         * it corresponds to some sort of application name, so we skip it too.
         */
        else
        {
            /* Get rid of the first token. We stop when we see whitespace. */
            while (*CommandLine && !(*CommandLine == L' ' || *CommandLine == L'\t'))
            {
                if (*CommandLine == L'\"')
                {
                    /* We enter a quoted part, skip it */
                    ++CommandLine;
                    while (*CommandLine && *CommandLine++ != L'\"') ;
                }
                else
                {
                    /* Go to the next character */
                    ++CommandLine;
                }
            }
        }
    }

    /*
     * Trim remaining whitespace from CommandLine that may be
     * present between the application name and the parameters.
     */
    while (*CommandLine == L' ' || *CommandLine == L'\t')
        ++CommandLine;

    /* Get the current directory */
    if (CurrentDirectory == NULL)
    {
        /* Allocate memory for the current directory path */
        Length = GetCurrentDirectoryW(0, NULL);
        CurrentDir = (PWCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
                                             Length * sizeof(WCHAR));
        if (CurrentDir == NULL)
        {
            Status = STATUS_NO_MEMORY;
            goto Cleanup;
        }

        /* Get the current directory */
        GetCurrentDirectoryW(Length, CurrentDir);
        CurrentDirectory = CurrentDir;
    }

    /* Calculate the size of the short current directory path */
    Length = GetShortPathNameW(CurrentDirectory, NULL, 0);

    /* Allocate memory for the short current directory path */
    ShortCurrentDir = (PWCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
                                              HEAP_ZERO_MEMORY,
                                              Length * sizeof(WCHAR));
    if (!ShortCurrentDir)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    /* Get the short current directory path */
    if (!GetShortPathNameW(CurrentDirectory, ShortCurrentDir, Length))
    {
        /* Try to determine which error occurred */
        switch (GetLastError())
        {
            case ERROR_NOT_ENOUGH_MEMORY:
            {
                Status = STATUS_NO_MEMORY;
                break;
            }

            case ERROR_INVALID_PARAMETER:
            {
                Status = STATUS_INVALID_PARAMETER;
                break;
            }

            default:
            {
                Status = STATUS_OBJECT_PATH_INVALID;
            }
        }
        goto Cleanup;
    }

    /* Make sure that the command line isn't too long */
    Length = wcslen(CommandLine);
    if (Length > UNICODE_STRING_MAX_CHARS - 1)
    {
        Status = STATUS_INVALID_PARAMETER;
        goto Cleanup;
    }

    /* Setup the input parameters */
    CheckVdm->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    CheckVdm->BinaryType = BinaryType;
    CheckVdm->CodePage = CP_ACP;
    CheckVdm->dwCreationFlags = CreationFlags;
    CheckVdm->CurDrive = CurrentDirectory[0] - L'A';
    CheckVdm->CmdLen = (USHORT)Length + 1;
    CheckVdm->AppLen = (USHORT)wcslen(ShortAppName) + 1;
    CheckVdm->PifLen = 0; // TODO: PIF file support!
    CheckVdm->CurDirectoryLen = (USHORT)wcslen(ShortCurrentDir) + 1;
    CheckVdm->EnvLen = AnsiEnvironment->Length;
    CheckVdm->DesktopLen = (StartupInfo->lpDesktop != NULL) ? (wcslen(StartupInfo->lpDesktop) + 1) : 0;
    CheckVdm->TitleLen = (StartupInfo->lpTitle != NULL) ? (wcslen(StartupInfo->lpTitle) + 1) : 0;
    CheckVdm->ReservedLen = (StartupInfo->lpReserved != NULL) ? (wcslen(StartupInfo->lpReserved) + 1) : 0;

    if (StartupInfo->dwFlags & STARTF_USESTDHANDLES)
    {
        /* Set the standard handles */
        CheckVdm->StdIn  = StartupInfo->hStdInput;
        CheckVdm->StdOut = StartupInfo->hStdOutput;
        CheckVdm->StdErr = StartupInfo->hStdError;
    }

    /* Allocate memory for the ANSI strings */
    // We need to add the newline characters '\r\n' to the command line
    AnsiCmdLine = (PCHAR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, CheckVdm->CmdLen + 2);
    AnsiAppName = (PCHAR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, CheckVdm->AppLen);
    AnsiCurDirectory = (PCHAR)RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY, CheckVdm->CurDirectoryLen);
    if (StartupInfo->lpDesktop)
        AnsiDesktop = (PCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
                                             HEAP_ZERO_MEMORY,
                                             CheckVdm->DesktopLen);
    if (StartupInfo->lpTitle)
        AnsiTitle = (PCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
                                           HEAP_ZERO_MEMORY,
                                           CheckVdm->TitleLen);
    if (StartupInfo->lpReserved)
        AnsiReserved = (PCHAR)RtlAllocateHeap(RtlGetProcessHeap(),
                                              HEAP_ZERO_MEMORY,
                                              CheckVdm->ReservedLen);

    if (!AnsiCmdLine
        || !AnsiAppName
        || !AnsiCurDirectory
        || (StartupInfo->lpDesktop && !AnsiDesktop)
        || (StartupInfo->lpTitle && !AnsiTitle)
        || (StartupInfo->lpReserved && !AnsiReserved))
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    /* Convert the command line into an ANSI string */
    WideCharToMultiByte(CP_ACP,
                        0,
                        CommandLine,
                        CheckVdm->CmdLen,
                        AnsiCmdLine,
                        CheckVdm->CmdLen,
                        NULL,
                        NULL);
    /* Add the needed newline and NULL-terminate */
    CheckVdm->CmdLen--; // Rewind back to the NULL character
    AnsiCmdLine[CheckVdm->CmdLen++] = '\r';
    AnsiCmdLine[CheckVdm->CmdLen++] = '\n';
    AnsiCmdLine[CheckVdm->CmdLen++] = 0;

    /* Convert the short application name into an ANSI string */
    WideCharToMultiByte(CP_ACP,
                        0,
                        ShortAppName,
                        CheckVdm->AppLen,
                        AnsiAppName,
                        CheckVdm->AppLen,
                        NULL,
                        NULL);

    /* Convert the short current directory path into an ANSI string */
    WideCharToMultiByte(CP_ACP,
                        0,
                        ShortCurrentDir,
                        CheckVdm->CurDirectoryLen,
                        AnsiCurDirectory,
                        CheckVdm->CurDirectoryLen,
                        NULL,
                        NULL);

    if (StartupInfo->lpDesktop)
    {
        /* Convert the desktop name into an ANSI string */
        WideCharToMultiByte(CP_ACP,
                            0,
                            StartupInfo->lpDesktop,
                            CheckVdm->DesktopLen,
                            AnsiDesktop,
                            CheckVdm->DesktopLen,
                            NULL,
                            NULL);
        NumStrings++;
    }

    if (StartupInfo->lpTitle)
    {
        /* Convert the title into an ANSI string */
        WideCharToMultiByte(CP_ACP,
                            0,
                            StartupInfo->lpTitle,
                            CheckVdm->TitleLen,
                            AnsiTitle,
                            CheckVdm->TitleLen,
                            NULL,
                            NULL);
        NumStrings++;
    }

    if (StartupInfo->lpReserved)
    {
        /* Convert the reserved value into an ANSI string */
        WideCharToMultiByte(CP_ACP,
                            0,
                            StartupInfo->lpReserved,
                            CheckVdm->ReservedLen,
                            AnsiReserved,
                            CheckVdm->ReservedLen,
                            NULL,
                            NULL);
        NumStrings++;
    }

    /* Fill the ANSI startup info structure */
    RtlCopyMemory(&AnsiStartupInfo, StartupInfo, sizeof(AnsiStartupInfo));
    AnsiStartupInfo.lpReserved = AnsiReserved;
    AnsiStartupInfo.lpDesktop = AnsiDesktop;
    AnsiStartupInfo.lpTitle = AnsiTitle;

    /* Allocate the capture buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(NumStrings,
                                             CheckVdm->CmdLen
                                             + CheckVdm->AppLen
                                             + CheckVdm->PifLen
                                             + CheckVdm->CurDirectoryLen
                                             + CheckVdm->DesktopLen
                                             + CheckVdm->TitleLen
                                             + CheckVdm->ReservedLen
                                             + CheckVdm->EnvLen
                                             + sizeof(*CheckVdm->StartupInfo));
    if (CaptureBuffer == NULL)
    {
        Status = STATUS_NO_MEMORY;
        goto Cleanup;
    }

    /* Capture the command line */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            AnsiCmdLine,
                            CheckVdm->CmdLen,
                            (PVOID*)&CheckVdm->CmdLine);

    /* Capture the application name */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            AnsiAppName,
                            CheckVdm->AppLen,
                            (PVOID*)&CheckVdm->AppName);

    CheckVdm->PifFile = NULL; // TODO: PIF file support!

    /* Capture the current directory */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            AnsiCurDirectory,
                            CheckVdm->CurDirectoryLen,
                            (PVOID*)&CheckVdm->CurDirectory);

    /* Capture the environment */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            AnsiEnvironment->Buffer,
                            CheckVdm->EnvLen,
                            (PVOID*)&CheckVdm->Env);

    /* Capture the startup info structure */
    CsrCaptureMessageBuffer(CaptureBuffer,
                            &AnsiStartupInfo,
                            sizeof(*CheckVdm->StartupInfo),
                            (PVOID*)&CheckVdm->StartupInfo);

    if (StartupInfo->lpDesktop)
    {
        /* Capture the desktop name */
        CsrCaptureMessageBuffer(CaptureBuffer,
                                AnsiDesktop,
                                CheckVdm->DesktopLen,
                                (PVOID*)&CheckVdm->Desktop);
    }
    else CheckVdm->Desktop = NULL;

    if (StartupInfo->lpTitle)
    {
        /* Capture the title */
        CsrCaptureMessageBuffer(CaptureBuffer,
                                AnsiTitle,
                                CheckVdm->TitleLen,
                                (PVOID*)&CheckVdm->Title);
    }
    else CheckVdm->Title = NULL;

    if (StartupInfo->lpReserved)
    {
        /* Capture the reserved parameter */
        CsrCaptureMessageBuffer(CaptureBuffer,
                                AnsiReserved,
                                CheckVdm->ReservedLen,
                                (PVOID*)&CheckVdm->Reserved);
    }
    else CheckVdm->Reserved = NULL;

    /* Send the message to CSRSS */
    Status = CsrClientCallServer((PCSR_API_MESSAGE)ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepCheckVDM),
                                 sizeof(*CheckVdm));

    /* Write back the task ID */
    *iTask = CheckVdm->iTask;

Cleanup:

    /* Free the ANSI strings */
    if (AnsiCmdLine) RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiCmdLine);
    if (AnsiAppName) RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiAppName);
    if (AnsiCurDirectory) RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiCurDirectory);
    if (AnsiDesktop) RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiDesktop);
    if (AnsiTitle) RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiTitle);
    if (AnsiReserved) RtlFreeHeap(RtlGetProcessHeap(), 0, AnsiReserved);

    /* Free the capture buffer */
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    /* Free the current directory, if it was allocated here, and its short path */
    if (ShortCurrentDir) RtlFreeHeap(RtlGetProcessHeap(), 0, ShortCurrentDir);
    if (CurrentDir) RtlFreeHeap(RtlGetProcessHeap(), 0, CurrentDir);

    /* Free the short app name */
    if (ShortAppName) RtlFreeHeap(RtlGetProcessHeap(), 0, ShortAppName);

    return Status;
}

BOOL
WINAPI
BaseUpdateVDMEntry(IN ULONG UpdateIndex,
                   IN OUT PHANDLE WaitHandle,
                   IN ULONG IndexInfo,
                   IN ULONG BinaryType)
{
    BASE_API_MESSAGE ApiMessage;
    PBASE_UPDATE_VDM_ENTRY UpdateVdmEntry = &ApiMessage.Data.UpdateVDMEntryRequest;

    /* Check what update is being sent */
    switch (UpdateIndex)
    {
        /* VDM is being undone */
        case VdmEntryUndo:
        {
            /* Tell the server how far we had gotten along */
            UpdateVdmEntry->iTask = HandleToUlong(*WaitHandle);
            UpdateVdmEntry->VDMCreationState = IndexInfo;
            break;
        }

        /* VDM is ready with a new process handle */
        case VdmEntryUpdateProcess:
        {
            /* Send it the process handle */
            UpdateVdmEntry->VDMProcessHandle = *WaitHandle;
            UpdateVdmEntry->iTask = IndexInfo;
            break;
        }
    }

    /* Also check what kind of binary this is for the console handle */
    if (BinaryType == BINARY_TYPE_WOW)
    {
        /* Magic value for 16-bit apps */
        UpdateVdmEntry->ConsoleHandle = (HANDLE)-1;
    }
    else if (UpdateVdmEntry->iTask)
    {
        /* No handle for true VDM */
        UpdateVdmEntry->ConsoleHandle = NULL;
    }
    else
    {
        /* Otherwise, use the regular console handle */
        UpdateVdmEntry->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    }

    /* Finally write the index and binary type */
    UpdateVdmEntry->EntryIndex = UpdateIndex;
    UpdateVdmEntry->BinaryType = BinaryType;

    /* Send the message to CSRSS */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepUpdateVDMEntry),
                        sizeof(*UpdateVdmEntry));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        /* Handle failure */
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    /* If this was an update, CSRSS returns a new wait handle */
    if (UpdateIndex == VdmEntryUpdateProcess)
    {
        /* Return it to the caller */
        *WaitHandle = UpdateVdmEntry->WaitObjectForParent;
    }

    /* We made it */
    return TRUE;
}

BOOL
WINAPI
BaseCheckForVDM(IN HANDLE ProcessHandle,
                OUT LPDWORD ExitCode)
{
    NTSTATUS Status;
    EVENT_BASIC_INFORMATION EventBasicInfo;
    BASE_API_MESSAGE ApiMessage;
    PBASE_GET_VDM_EXIT_CODE GetVdmExitCode = &ApiMessage.Data.GetVDMExitCodeRequest;

    /* It's VDM if the process is actually a wait handle (an event) */
    Status = NtQueryEvent(ProcessHandle,
                          EventBasicInformation,
                          &EventBasicInfo,
                          sizeof(EventBasicInfo),
                          NULL);
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Setup the input parameters */
    GetVdmExitCode->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetVdmExitCode->hParent = ProcessHandle;

    /* Call CSRSS */
    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepGetVDMExitCode),
                                 sizeof(*GetVdmExitCode));
    if (!NT_SUCCESS(Status)) return FALSE;

    /* Get the exit code from the reply */
    *ExitCode = GetVdmExitCode->ExitCode;
    return TRUE;
}

BOOL
WINAPI
BaseGetVdmConfigInfo(IN LPCWSTR CommandLineReserved,
                     IN ULONG DosSeqId,
                     IN ULONG BinaryType,
                     IN PUNICODE_STRING CmdLineString,
                     OUT PULONG VdmSize)
{
    WCHAR Buffer[MAX_PATH];
    WCHAR CommandLine[MAX_PATH * 2];
    ULONG Length;

    /* Clear the buffer in case we fail */
    CmdLineString->Buffer = 0;

    /* Always return the same size: 16 Mb */
    *VdmSize = 0x1000000;

    /* Get the system directory */
    Length = GetSystemDirectoryW(Buffer, MAX_PATH);
    if (!(Length) || (Length >= MAX_PATH))
    {
        /* Eliminate no path or path too big */
        SetLastError(ERROR_INVALID_NAME);
        return FALSE;
    }

    /* Check if this is VDM with a DOS Sequence ID */
    if (DosSeqId)
    {
        /*
         * Build the VDM string for it:
         * -i%lx : Gives the DOS Sequence ID;
         * %s%c  : Nothing if DOS VDM, -w if WoW VDM, -ws if separate WoW VDM.
         */
        _snwprintf(CommandLine,
                   ARRAYSIZE(CommandLine),
                   L"\"%s\\ntvdm.exe\" -i%lx %s%c",
                   Buffer,
                   DosSeqId,
                   (BinaryType == BINARY_TYPE_DOS) ? L" " : L"-w",
                   (BinaryType == BINARY_TYPE_SEPARATE_WOW) ? L's' : L' ');
    }
    else
    {
        /*
         * Build the string for it without the DOS Sequence ID:
         * %s%c  : Nothing if DOS VDM, -w if WoW VDM, -ws if separate WoW VDM.
         */
        _snwprintf(CommandLine,
                   ARRAYSIZE(CommandLine),
                   L"\"%s\\ntvdm.exe\" %s%c",
                   Buffer,
                   (BinaryType == BINARY_TYPE_DOS) ? L" " : L"-w",
                   (BinaryType == BINARY_TYPE_SEPARATE_WOW) ? L's' : L' ');
    }

    /* Create the actual string */
    return RtlCreateUnicodeString(CmdLineString, CommandLine);
}

ENV_NAME_TYPE
WINAPI
BaseGetEnvNameType_U(IN PWCHAR Name,
                     IN ULONG NameLength)
{
    PENV_INFO EnvInfo;
    ENV_NAME_TYPE NameType;
    ULONG i;

    /* Start by assuming the environment variable doesn't describe paths */
    NameType = EnvNameNotAPath;

    /* Loop all the environment names */
    for (i = 0; i < ARRAYSIZE(BasepEnvNameType); i++)
    {
        /* Get this entry */
        EnvInfo = &BasepEnvNameType[i];

        /* Check if it matches the name */
        if ((EnvInfo->NameLength == NameLength) &&
            (_wcsnicmp(EnvInfo->Name, Name, NameLength) == 0))
        {
            /* It does, return the type */
            NameType = EnvInfo->NameType;
            break;
        }
    }

    return NameType;
}

BOOL
NTAPI
BaseCreateVDMEnvironment(IN PWCHAR lpEnvironment,
                         OUT PANSI_STRING AnsiEnv,
                         OUT PUNICODE_STRING UnicodeEnv)
{
#define IS_ALPHA(x)   \
    ( ((x) >= L'A' && (x) <= L'Z') || ((x) >= L'a' && (x) <= L'z') )

// From lib/rtl/path.c :
// Can be put in some .h ??
#define IS_PATH_SEPARATOR(x)    ((x) == L'\\' || (x) == L'/')

    BOOL Success = FALSE;
    NTSTATUS Status;
    ULONG EnvironmentSize = 0;
    SIZE_T RegionSize;
    PWCHAR Environment, NewEnvironment = NULL;
    ENV_NAME_TYPE NameType;
    ULONG NameLength, NumChars, Remaining;
    PWCHAR SourcePtr, DestPtr, StartPtr;

    /* Make sure we have both strings */
    if (!AnsiEnv || !UnicodeEnv)
    {
        /* Fail */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    /* Check if an environment was passed in */
    if (!lpEnvironment)
    {
        /* Nope, create one */
        Status = RtlCreateEnvironment(TRUE, &Environment);
        if (!NT_SUCCESS(Status)) goto Cleanup;
    }
    else
    {
        /* Use the one we got */
        Environment = lpEnvironment;
    }

    /* Do we have something now ? */
    if (!Environment)
    {
        /* Still not, bail out */
        SetLastError(ERROR_BAD_ENVIRONMENT);
        goto Cleanup;
    }

    /*
     * Count how much space the whole environment takes. The environment block is
     * doubly NULL-terminated (NULL from last string and final NULL terminator).
     */
    SourcePtr = Environment;
    while (!(*SourcePtr++ == UNICODE_NULL && *SourcePtr == UNICODE_NULL))
        ++EnvironmentSize;
    EnvironmentSize += 2; // Add the two terminating NULLs

    /*
     * Allocate a new copy large enough to hold all the environment with paths
     * in their short form. Since the short form of a path can be a bit longer
     * than its long form, for example in the case where characters that are
     * invalid in the 8.3 representation are present in the long path name:
     *   'C:\\a+b' --> 'C:\\A_B~1', or:
     *   'C:\\a b' --> 'C:\\AB2761~1' (with checksum inserted),
     * we suppose that the possible total number of extra characters needed to
     * convert the long paths into their short form is at most equal to MAX_PATH.
     */
    RegionSize = (EnvironmentSize + MAX_PATH) * sizeof(WCHAR);
    Status = NtAllocateVirtualMemory(NtCurrentProcess(),
                                     (PVOID*)&NewEnvironment,
                                     0,
                                     &RegionSize,
                                     MEM_COMMIT,
                                     PAGE_READWRITE);
    if (!NT_SUCCESS(Status))
    {
        /* We failed, bail out */
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        NewEnvironment = NULL;
        goto Cleanup;
    }

    /* Parse the environment block */
    Remaining = MAX_PATH - 2; // '-2': remove the last two NULLs. FIXME: is it really needed??
    SourcePtr = Environment;
    DestPtr   = NewEnvironment;

    /* Loop through all the environment strings */
    while (*SourcePtr != UNICODE_NULL)
    {
        /*
         * 1. Check the type of the environment variable and copy its name.
         */

        /* Regular environment variable */
        if (*SourcePtr != L'=')
        {
            StartPtr = SourcePtr;

            /* Copy the environment variable name, including the '=' */
            while (*SourcePtr != UNICODE_NULL)
            {
                *DestPtr++ = *SourcePtr;
                if (*SourcePtr++ == L'=') break;
            }

            /* Guess the type of the environment variable */
            NameType = BaseGetEnvNameType_U(StartPtr, SourcePtr - StartPtr - 1);
        }
        /* 'Current directory' environment variable (i.e. of '=X:=' form) */
        else // if (*SourcePtr == L'=')
        {
            /* First assume we have a possibly malformed environment variable */
            NameType = EnvNameNotAPath;

            /* Check for a valid 'Current directory' environment variable */
            if (IS_ALPHA(SourcePtr[1]) && SourcePtr[2] == L':' && SourcePtr[3] == L'=')
            {
                /*
                 * Small optimization: convert the path to short form only if
                 * the current directory is not the root directory (i.e. not
                 * of the '=X:=Y:\' form), otherwise just do a simple copy.
                 */
                if ( wcslen(SourcePtr) >= ARRAYSIZE("=X:=Y:\\")-1 &&
                     !( IS_ALPHA(SourcePtr[4]) && SourcePtr[5] == L':' &&
                        IS_PATH_SEPARATOR(SourcePtr[6]) && SourcePtr[7] == UNICODE_NULL ) )
                {
                    NameType = EnvNameSinglePath;

                    /* Copy the '=X:=' prefix */
                    *DestPtr++ = SourcePtr[0];
                    *DestPtr++ = SourcePtr[1];
                    *DestPtr++ = SourcePtr[2];
                    *DestPtr++ = SourcePtr[3];
                    SourcePtr += 4;
                }
            }
            else
            {
                /*
                 * Invalid stuff starting with '=', i.e.:
                 * =? (with '?' not being a letter)
                 * =X??? (with '?' not being ":=" and not followed by something longer than 3 characters)
                 * =X:=??? (with '?' not being "X:\\")
                 *
                 * 'NameType' is already set to 'EnvNameNotAPath'.
                 */
            }
        }


        /*
         * 2. Copy the environment value and perform conversions accordingly.
         */

        if (NameType == EnvNameNotAPath)
        {
            /* Copy everything, including the NULL terminator */
            do
            {
                *DestPtr++ = *SourcePtr;
            } while (*SourcePtr++ != UNICODE_NULL);
        }
        else if (NameType == EnvNameSinglePath)
        {
            /* Convert the path to its short form */
            NameLength = wcslen(SourcePtr);
            NumChars = GetShortPathNameW(SourcePtr, DestPtr, NameLength + 1 + Remaining);
            if (NumChars == 0 || NumChars > NameLength + Remaining)
            {
                /* If the conversion failed, just copy the original value */
                RtlCopyMemory(DestPtr, SourcePtr, NameLength * sizeof(WCHAR));
                NumChars = NameLength;
            }
            DestPtr += NumChars;
            if (NumChars > NameLength)
                Remaining -= (NumChars - NameLength);

            SourcePtr += NameLength;

            /* Copy the NULL terminator */
            *DestPtr++ = *SourcePtr++;
        }
        else // if (NameType == EnvNameMultiplePath)
        {
            WCHAR Delimiter;

            /* Loop through the list of paths (delimited by ';') and convert each path to its short form */
            do
            {
                /* Copy any trailing ';' before going to the next path */
                while (*SourcePtr == L';')
                {
                    *DestPtr++ = *SourcePtr++;
                }

                StartPtr = SourcePtr;

                /* Find the next path list delimiter or the NULL terminator */
                while (*SourcePtr != UNICODE_NULL && *SourcePtr != L';')
                {
                    ++SourcePtr;
                }
                Delimiter = *SourcePtr;

                NameLength = SourcePtr - StartPtr;
                if (NameLength)
                {
                    /*
                     * Temporarily replace the possible path list delimiter by NULL.
                     * 'lpEnvironment' must point to a read+write memory buffer!
                     */
                    *SourcePtr = UNICODE_NULL;

                    NumChars = GetShortPathNameW(StartPtr, DestPtr, NameLength + 1 + Remaining);
                    if ( NumChars == 0 ||
                        (Delimiter == L';' ? NumChars > NameLength + Remaining
                                           : NumChars > NameLength /* + Remaining ?? */) )
                    {
                        /* If the conversion failed, just copy the original value */
                        RtlCopyMemory(DestPtr, StartPtr, NameLength * sizeof(WCHAR));
                        NumChars = NameLength;
                    }
                    DestPtr += NumChars;
                    if (NumChars > NameLength)
                        Remaining -= (NumChars - NameLength);

                    /* If removed, restore the path list delimiter in the source environment value and copy it */
                    if (Delimiter != UNICODE_NULL)
                    {
                        *SourcePtr = Delimiter;
                        *DestPtr++ = *SourcePtr++;
                    }
                }
            } while (*SourcePtr != UNICODE_NULL);

            /* Copy the NULL terminator */
            *DestPtr++ = *SourcePtr++;
        }
    }

    /* NULL-terminate the environment block */
    *DestPtr++ = UNICODE_NULL;

    /* Initialize the Unicode string to hold it */
    RtlInitEmptyUnicodeString(UnicodeEnv, NewEnvironment,
                              (DestPtr - NewEnvironment) * sizeof(WCHAR));
    UnicodeEnv->Length = UnicodeEnv->MaximumLength;

    /* Create its ANSI version */
    Status = RtlUnicodeStringToAnsiString(AnsiEnv, UnicodeEnv, TRUE);
    if (!NT_SUCCESS(Status))
    {
        /* Set last error if conversion failure */
        BaseSetLastNTError(Status);
    }
    else
    {
        /* Everything went okay, so return success */
        Success = TRUE;
        NewEnvironment = NULL;
    }

Cleanup:
    /* Cleanup path starts here, start by destroying the environment copy */
    if (!lpEnvironment && Environment) RtlDestroyEnvironment(Environment);

    /* See if we are here due to failure */
    if (NewEnvironment)
    {
        /* Initialize the paths to be empty */
        RtlInitEmptyUnicodeString(UnicodeEnv, NULL, 0);
        RtlInitEmptyAnsiString(AnsiEnv, NULL, 0);

        /* Free the environment copy */
        RegionSize = 0;
        Status = NtFreeVirtualMemory(NtCurrentProcess(),
                                     (PVOID*)&NewEnvironment,
                                     &RegionSize,
                                     MEM_RELEASE);
        ASSERT(NT_SUCCESS(Status));
    }

    /* Return the result */
    return Success;
}

BOOL
NTAPI
BaseDestroyVDMEnvironment(IN PANSI_STRING AnsiEnv,
                          IN PUNICODE_STRING UnicodeEnv)
{
    SIZE_T Dummy = 0;

    /* Clear the ANSI buffer since Rtl creates this for us */
    if (AnsiEnv->Buffer) RtlFreeAnsiString(AnsiEnv);

    /* The Unicode buffer is build by hand, though */
    if (UnicodeEnv->Buffer)
    {
        /* So clear it through the API */
        NtFreeVirtualMemory(NtCurrentProcess(),
                            (PVOID*)&UnicodeEnv->Buffer,
                            &Dummy,
                            MEM_RELEASE);
    }

    /* All done */
    return TRUE;
}


/* Check whether a file is an OS/2 or a very old Windows executable
 * by testing on import of KERNEL.
 *
 * FIXME: is reading the module imports the only way of discerning
 *        old Windows binaries from OS/2 ones ? At least it seems so...
 */
static DWORD WINAPI
InternalIsOS2OrOldWin(HANDLE hFile, IMAGE_DOS_HEADER *mz, IMAGE_OS2_HEADER *ne)
{
  DWORD CurPos;
  LPWORD modtab = NULL;
  LPSTR nametab = NULL;
  DWORD Read, Ret;
  int i;

  Ret = BINARY_OS216;
  CurPos = SetFilePointer(hFile, 0, NULL, FILE_CURRENT);

  /* read modref table */
  if((SetFilePointer(hFile, mz->e_lfanew + ne->ne_modtab, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) ||
     (!(modtab = HeapAlloc(GetProcessHeap(), 0, ne->ne_cmod * sizeof(WORD)))) ||
     (!(ReadFile(hFile, modtab, ne->ne_cmod * sizeof(WORD), &Read, NULL))) ||
     (Read != (DWORD)ne->ne_cmod * sizeof(WORD)))
  {
    goto broken;
  }

  /* read imported names table */
  if((SetFilePointer(hFile, mz->e_lfanew + ne->ne_imptab, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) ||
     (!(nametab = HeapAlloc(GetProcessHeap(), 0, ne->ne_enttab - ne->ne_imptab))) ||
     (!(ReadFile(hFile, nametab, ne->ne_enttab - ne->ne_imptab, &Read, NULL))) ||
     (Read != (DWORD)ne->ne_enttab - ne->ne_imptab))
  {
    goto broken;
  }

  for(i = 0; i < ne->ne_cmod; i++)
  {
    LPSTR module;
    module = &nametab[modtab[i]];
    if(!strncmp(&module[1], "KERNEL", module[0]))
    {
      /* very old windows file */
      Ret = BINARY_WIN16;
      goto done;
    }
  }

  broken:
  DPRINT1("InternalIsOS2OrOldWin(): Binary file seems to be broken\n");

  done:
  HeapFree(GetProcessHeap(), 0, modtab);
  HeapFree(GetProcessHeap(), 0, nametab);
  SetFilePointer(hFile, CurPos, NULL, FILE_BEGIN);
  return Ret;
}

static DWORD WINAPI
InternalGetBinaryType(HANDLE hFile)
{
  union
  {
    struct
    {
      unsigned char magic[4];
      unsigned char ignored[12];
      unsigned short type;
    } elf;
    struct
    {
      unsigned long magic;
      unsigned long cputype;
      unsigned long cpusubtype;
      unsigned long filetype;
    } macho;
    IMAGE_DOS_HEADER mz;
  } Header;
  char magic[4];
  DWORD Read;

  if((SetFilePointer(hFile, 0, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) ||
     (!ReadFile(hFile, &Header, sizeof(Header), &Read, NULL) ||
      (Read != sizeof(Header))))
  {
    return BINARY_UNKNOWN;
  }

  if(!memcmp(Header.elf.magic, "\177ELF", sizeof(Header.elf.magic)))
  {
    /* FIXME: we don't bother to check byte order, architecture, etc. */
    switch(Header.elf.type)
    {
      case 2:
        return BINARY_UNIX_EXE;
      case 3:
        return BINARY_UNIX_LIB;
    }
    return BINARY_UNKNOWN;
  }

  /* Mach-o File with Endian set to Big Endian or Little Endian*/
  if(Header.macho.magic == 0xFEEDFACE ||
     Header.macho.magic == 0xCEFAEDFE)
  {
    switch(Header.macho.filetype)
    {
      case 0x8:
        /* MH_BUNDLE */
        return BINARY_UNIX_LIB;
    }
    return BINARY_UNKNOWN;
  }

  /* Not ELF, try DOS */
  if(Header.mz.e_magic == IMAGE_DOS_SIGNATURE)
  {
    /* We do have a DOS image so we will now try to seek into
     * the file by the amount indicated by the field
     * "Offset to extended header" and read in the
     * "magic" field information at that location.
     * This will tell us if there is more header information
     * to read or not.
     */
    if((SetFilePointer(hFile, Header.mz.e_lfanew, NULL, FILE_BEGIN) == INVALID_SET_FILE_POINTER) ||
       (!ReadFile(hFile, magic, sizeof(magic), &Read, NULL) ||
        (Read != sizeof(magic))))
    {
      return BINARY_DOS;
    }

    /* Reading the magic field succeeded so
     * we will try to determine what type it is.
     */
    if(!memcmp(magic, "PE\0\0", sizeof(magic)))
    {
      IMAGE_FILE_HEADER FileHeader;
      if(!ReadFile(hFile, &FileHeader, sizeof(IMAGE_FILE_HEADER), &Read, NULL) ||
         (Read != sizeof(IMAGE_FILE_HEADER)))
      {
        return BINARY_DOS;
      }

      /* FIXME - detect 32/64 bit */

      if(FileHeader.Characteristics & IMAGE_FILE_DLL)
        return BINARY_PE_DLL32;
      return BINARY_PE_EXE32;
    }

    if(!memcmp(magic, "NE", 2))
    {
      /* This is a Windows executable (NE) header.  This can
       * mean either a 16-bit OS/2 or a 16-bit Windows or even a
       * DOS program (running under a DOS extender).  To decide
       * which, we'll have to read the NE header.
       */
      IMAGE_OS2_HEADER ne;
      if((SetFilePointer(hFile, Header.mz.e_lfanew, NULL, FILE_BEGIN) == 1) ||
         !ReadFile(hFile, &ne, sizeof(IMAGE_OS2_HEADER), &Read, NULL) ||
         (Read != sizeof(IMAGE_OS2_HEADER)))
      {
        /* Couldn't read header, so abort. */
        return BINARY_DOS;
      }

      switch(ne.ne_exetyp)
      {
        case 2:
          return BINARY_WIN16;
        case 5:
          return BINARY_DOS;
        default:
          return InternalIsOS2OrOldWin(hFile, &Header.mz, &ne);
      }
    }
    return BINARY_DOS;
  }
  return BINARY_UNKNOWN;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetBinaryTypeW (
    LPCWSTR lpApplicationName,
    LPDWORD lpBinaryType
    )
{
  HANDLE hFile;
  DWORD BinType;

  if(!lpApplicationName || !lpBinaryType)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  hFile = CreateFileW(lpApplicationName, GENERIC_READ, FILE_SHARE_READ, NULL,
                      OPEN_EXISTING, 0, 0);
  if(hFile == INVALID_HANDLE_VALUE)
  {
    return FALSE;
  }

  BinType = InternalGetBinaryType(hFile);
  CloseHandle(hFile);

  switch(BinType)
  {
    case BINARY_UNKNOWN:
    {
      WCHAR *dot;

      /*
       * guess from filename
       */
      if(!(dot = wcsrchr(lpApplicationName, L'.')))
      {
        return FALSE;
      }
      if(!lstrcmpiW(dot, L".COM"))
      {
        *lpBinaryType = SCS_DOS_BINARY;
        return TRUE;
      }
      if(!lstrcmpiW(dot, L".PIF"))
      {
        *lpBinaryType = SCS_PIF_BINARY;
        return TRUE;
      }
      return FALSE;
    }
    case BINARY_PE_EXE32:
    case BINARY_PE_DLL32:
    {
      *lpBinaryType = SCS_32BIT_BINARY;
      return TRUE;
    }
    case BINARY_PE_EXE64:
    case BINARY_PE_DLL64:
    {
      *lpBinaryType = SCS_64BIT_BINARY;
      return TRUE;
    }
    case BINARY_WIN16:
    {
      *lpBinaryType = SCS_WOW_BINARY;
      return TRUE;
    }
    case BINARY_OS216:
    {
      *lpBinaryType = SCS_OS216_BINARY;
      return TRUE;
    }
    case BINARY_DOS:
    {
      *lpBinaryType = SCS_DOS_BINARY;
      return TRUE;
    }
    case BINARY_UNIX_EXE:
    case BINARY_UNIX_LIB:
    {
      return FALSE;
    }
  }

  DPRINT1("Invalid binary type %lu returned!\n", BinType);
  return FALSE;
}

/*
 * @implemented
 */
BOOL
WINAPI
GetBinaryTypeA(IN LPCSTR lpApplicationName,
               OUT LPDWORD lpBinaryType)
{
    ANSI_STRING ApplicationNameString;
    UNICODE_STRING ApplicationNameW;
    BOOL StringAllocated = FALSE, Result;
    NTSTATUS Status;

    RtlInitAnsiString(&ApplicationNameString, lpApplicationName);

    if (ApplicationNameString.Length * sizeof(WCHAR) >= NtCurrentTeb()->StaticUnicodeString.MaximumLength)
    {
        StringAllocated = TRUE;
        Status = RtlAnsiStringToUnicodeString(&ApplicationNameW, &ApplicationNameString, TRUE);
    }
    else
    {
        Status = RtlAnsiStringToUnicodeString(&(NtCurrentTeb()->StaticUnicodeString), &ApplicationNameString, FALSE);
    }

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (StringAllocated)
    {
        Result = GetBinaryTypeW(ApplicationNameW.Buffer, lpBinaryType);
        RtlFreeUnicodeString(&ApplicationNameW);
    }
    else
    {
        Result = GetBinaryTypeW(NtCurrentTeb()->StaticUnicodeString.Buffer, lpBinaryType);
    }

    return Result;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
CmdBatNotification (
    DWORD   Unknown
    )
{
    STUB;
    return FALSE;
}

/*
 * @implemented
 */
VOID
WINAPI
ExitVDM(BOOL IsWow, ULONG iWowTask)
{
    BASE_API_MESSAGE ApiMessage;
    PBASE_EXIT_VDM ExitVdm = &ApiMessage.Data.ExitVDMRequest;

    /* Setup the input parameters */
    ExitVdm->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    ExitVdm->iWowTask = IsWow ? iWowTask : 0; /* Always zero for DOS tasks */
    ExitVdm->WaitObjectForVDM = NULL;

    /* Call CSRSS */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepExitVDM),
                        sizeof(*ExitVdm));

    /* Close the returned wait object handle, if any */
    if (NT_SUCCESS(ApiMessage.Status) && (ExitVdm->WaitObjectForVDM != NULL))
    {
        CloseHandle(ExitVdm->WaitObjectForVDM);
    }
}

/*
 * @implemented
 */
BOOL
WINAPI
GetNextVDMCommand(PVDM_COMMAND_INFO CommandData)
{
    BOOL Success = FALSE;
    NTSTATUS Status;
    BASE_API_MESSAGE ApiMessage;
    PBASE_GET_NEXT_VDM_COMMAND GetNextVdmCommand = &ApiMessage.Data.GetNextVDMCommandRequest;
    PBASE_IS_FIRST_VDM IsFirstVdm = &ApiMessage.Data.IsFirstVDMRequest;
    PBASE_SET_REENTER_COUNT SetReenterCount = &ApiMessage.Data.SetReenterCountRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;
    ULONG NumStrings = 0;

    /*
     * Special case to test whether the VDM is the first one.
     */
    if (CommandData == NULL)
    {
        /* Call CSRSS */
        CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                            NULL,
                            CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepIsFirstVDM),
                            sizeof(*IsFirstVdm));
        if (!NT_SUCCESS(ApiMessage.Status))
        {
            BaseSetLastNTError(ApiMessage.Status);
            return FALSE;
        }

        /* Return TRUE if this is the first VDM */
        return IsFirstVdm->FirstVDM;
    }

    /* CommandData != NULL */

    /*
     * Special case to increment or decrement the reentrancy count.
     */
    if ((CommandData->VDMState == VDM_INC_REENTER_COUNT) ||
        (CommandData->VDMState == VDM_DEC_REENTER_COUNT))
    {
        /* Setup the input parameters */
        SetReenterCount->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
        SetReenterCount->fIncDec = CommandData->VDMState;

        /* Call CSRSS */
        CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                            NULL,
                            CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepSetReenterCount),
                            sizeof(*SetReenterCount));
        if (!NT_SUCCESS(ApiMessage.Status))
        {
            BaseSetLastNTError(ApiMessage.Status);
            return FALSE;
        }

        return TRUE;
    }

    /*
     * TODO!
     * Special case to retrieve or set WOW information.
     */
    // TODO: if CommandData->VDMState & (VDM_LIST_WOW_PROCESSES | VDM_LIST_WOW_TASKS | VDM_ADD_WOW_TASK)
    // then call BasepGetNextVDMCommand in a simpler way!

    /*
     * Regular case.
     */

    /* Clear the structure */
    RtlZeroMemory(GetNextVdmCommand, sizeof(*GetNextVdmCommand));

    /* Setup the input parameters */
    GetNextVdmCommand->iTask = CommandData->TaskId;
    GetNextVdmCommand->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetNextVdmCommand->CmdLen = CommandData->CmdLen;
    GetNextVdmCommand->AppLen = CommandData->AppLen;
    GetNextVdmCommand->PifLen = CommandData->PifLen;
    GetNextVdmCommand->CurDirectoryLen = CommandData->CurDirectoryLen;
    GetNextVdmCommand->EnvLen = CommandData->EnvLen;
    GetNextVdmCommand->DesktopLen = CommandData->DesktopLen;
    GetNextVdmCommand->TitleLen = CommandData->TitleLen;
    GetNextVdmCommand->ReservedLen = CommandData->ReservedLen;
    GetNextVdmCommand->VDMState = CommandData->VDMState;

    /* Count the number of strings */
    if (CommandData->CmdLen) NumStrings++;
    if (CommandData->AppLen) NumStrings++;
    if (CommandData->PifLen) NumStrings++;
    if (CommandData->CurDirectoryLen) NumStrings++;
    if (CommandData->EnvLen) NumStrings++;
    if (CommandData->DesktopLen) NumStrings++;
    if (CommandData->TitleLen) NumStrings++;
    if (CommandData->ReservedLen) NumStrings++;

    /* Allocate the capture buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(NumStrings + 1,
                                             GetNextVdmCommand->CmdLen
                                             + GetNextVdmCommand->AppLen
                                             + GetNextVdmCommand->PifLen
                                             + GetNextVdmCommand->CurDirectoryLen
                                             + GetNextVdmCommand->EnvLen
                                             + GetNextVdmCommand->DesktopLen
                                             + GetNextVdmCommand->TitleLen
                                             + GetNextVdmCommand->ReservedLen
                                             + sizeof(*GetNextVdmCommand->StartupInfo));
    if (CaptureBuffer == NULL)
    {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        goto Cleanup;
    }

    /* Capture the data */

    CsrAllocateMessagePointer(CaptureBuffer,
                              sizeof(*GetNextVdmCommand->StartupInfo),
                              (PVOID*)&GetNextVdmCommand->StartupInfo);

    if (CommandData->CmdLen)
    {
        CsrAllocateMessagePointer(CaptureBuffer,
                                  CommandData->CmdLen,
                                  (PVOID*)&GetNextVdmCommand->CmdLine);
    }

    if (CommandData->AppLen)
    {
        CsrAllocateMessagePointer(CaptureBuffer,
                                  CommandData->AppLen,
                                  (PVOID*)&GetNextVdmCommand->AppName);
    }

    if (CommandData->PifLen)
    {
        CsrAllocateMessagePointer(CaptureBuffer,
                                  CommandData->PifLen,
                                  (PVOID*)&GetNextVdmCommand->PifFile);
    }

    if (CommandData->CurDirectoryLen)
    {
        CsrAllocateMessagePointer(CaptureBuffer,
                                  CommandData->CurDirectoryLen,
                                  (PVOID*)&GetNextVdmCommand->CurDirectory);
    }

    if (CommandData->EnvLen)
    {
        CsrAllocateMessagePointer(CaptureBuffer,
                                  CommandData->EnvLen,
                                  (PVOID*)&GetNextVdmCommand->Env);
    }

    if (CommandData->DesktopLen)
    {
        CsrAllocateMessagePointer(CaptureBuffer,
                                  CommandData->DesktopLen,
                                  (PVOID*)&GetNextVdmCommand->Desktop);
    }

    if (CommandData->TitleLen)
    {
        CsrAllocateMessagePointer(CaptureBuffer,
                                  CommandData->TitleLen,
                                  (PVOID*)&GetNextVdmCommand->Title);
    }

    if (CommandData->ReservedLen)
    {
        CsrAllocateMessagePointer(CaptureBuffer,
                                  CommandData->ReservedLen,
                                  (PVOID*)&GetNextVdmCommand->Reserved);
    }

    while (TRUE)
    {
        /* Call CSRSS */
        Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                     CaptureBuffer,
                                     CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepGetNextVDMCommand),
                                     sizeof(*GetNextVdmCommand));

        /* Exit the waiting loop if we did not receive any event handle */
        if (GetNextVdmCommand->WaitObjectForVDM == NULL)
            break;

        /* Wait for the event to become signaled and try again */
        Status = NtWaitForSingleObject(GetNextVdmCommand->WaitObjectForVDM,
                                       FALSE, NULL);
        if (Status != STATUS_SUCCESS)
        {
            /* Fail if we timed out, or if some other error happened */
            BaseSetLastNTError(Status);
            goto Cleanup;
        }

        /* Set the retry flag, clear the exit code, and retry a query */
        GetNextVdmCommand->VDMState |= VDM_FLAG_RETRY;
        GetNextVdmCommand->ExitCode = 0;
    }

    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_INVALID_PARAMETER)
        {
            /*
             * One of the buffer lengths was less than required. Store the correct ones.
             * Note that the status code is not STATUS_BUFFER_TOO_SMALL as one would expect,
             * in order to keep compatibility with Windows 2003 BASESRV.DLL.
             */
            CommandData->CmdLen = GetNextVdmCommand->CmdLen;
            CommandData->AppLen = GetNextVdmCommand->AppLen;
            CommandData->PifLen = GetNextVdmCommand->PifLen;
            CommandData->CurDirectoryLen = GetNextVdmCommand->CurDirectoryLen;
            CommandData->EnvLen      = GetNextVdmCommand->EnvLen;
            CommandData->DesktopLen  = GetNextVdmCommand->DesktopLen;
            CommandData->TitleLen    = GetNextVdmCommand->TitleLen;
            CommandData->ReservedLen = GetNextVdmCommand->ReservedLen;
        }
        else
        {
            /* Any other failure */
            CommandData->CmdLen = 0;
            CommandData->AppLen = 0;
            CommandData->PifLen = 0;
            CommandData->CurDirectoryLen = 0;
            CommandData->EnvLen      = 0;
            CommandData->DesktopLen  = 0;
            CommandData->TitleLen    = 0;
            CommandData->ReservedLen = 0;
        }

        BaseSetLastNTError(Status);
        goto Cleanup;
    }

    /* Write back the standard handles */
    CommandData->StdIn  = GetNextVdmCommand->StdIn;
    CommandData->StdOut = GetNextVdmCommand->StdOut;
    CommandData->StdErr = GetNextVdmCommand->StdErr;

    /* Write back the startup info */
    RtlMoveMemory(&CommandData->StartupInfo,
                  GetNextVdmCommand->StartupInfo,
                  sizeof(*GetNextVdmCommand->StartupInfo));

    if (CommandData->CmdLen)
    {
        /* Write back the command line */
        RtlMoveMemory(CommandData->CmdLine,
                      GetNextVdmCommand->CmdLine,
                      GetNextVdmCommand->CmdLen);

        /* Set the actual length */
        CommandData->CmdLen = GetNextVdmCommand->CmdLen;
    }

    if (CommandData->AppLen)
    {
        /* Write back the application name */
        RtlMoveMemory(CommandData->AppName,
                      GetNextVdmCommand->AppName,
                      GetNextVdmCommand->AppLen);

        /* Set the actual length */
        CommandData->AppLen = GetNextVdmCommand->AppLen;
    }

    if (CommandData->PifLen)
    {
        /* Write back the PIF file name */
        RtlMoveMemory(CommandData->PifFile,
                      GetNextVdmCommand->PifFile,
                      GetNextVdmCommand->PifLen);

        /* Set the actual length */
        CommandData->PifLen = GetNextVdmCommand->PifLen;
    }

    if (CommandData->CurDirectoryLen)
    {
        /* Write back the current directory */
        RtlMoveMemory(CommandData->CurDirectory,
                      GetNextVdmCommand->CurDirectory,
                      GetNextVdmCommand->CurDirectoryLen);

        /* Set the actual length */
        CommandData->CurDirectoryLen = GetNextVdmCommand->CurDirectoryLen;
    }

    if (CommandData->EnvLen)
    {
        /* Write back the environment */
        RtlMoveMemory(CommandData->Env,
                      GetNextVdmCommand->Env,
                      GetNextVdmCommand->EnvLen);

        /* Set the actual length */
        CommandData->EnvLen = GetNextVdmCommand->EnvLen;
    }

    if (CommandData->DesktopLen)
    {
        /* Write back the desktop name */
        RtlMoveMemory(CommandData->Desktop,
                      GetNextVdmCommand->Desktop,
                      GetNextVdmCommand->DesktopLen);

        /* Set the actual length */
        CommandData->DesktopLen = GetNextVdmCommand->DesktopLen;
    }

    if (CommandData->TitleLen)
    {
        /* Write back the title */
        RtlMoveMemory(CommandData->Title,
                      GetNextVdmCommand->Title,
                      GetNextVdmCommand->TitleLen);

        /* Set the actual length */
        CommandData->TitleLen = GetNextVdmCommand->TitleLen;
    }

    if (CommandData->ReservedLen)
    {
        /* Write back the reserved parameter */
        RtlMoveMemory(CommandData->Reserved,
                      GetNextVdmCommand->Reserved,
                      GetNextVdmCommand->ReservedLen);

        /* Set the actual length */
        CommandData->ReservedLen = GetNextVdmCommand->ReservedLen;
    }

    /* Write the remaining output parameters */
    CommandData->TaskId        = GetNextVdmCommand->iTask;
    CommandData->CreationFlags = GetNextVdmCommand->dwCreationFlags;
    CommandData->CodePage      = GetNextVdmCommand->CodePage;
    CommandData->ExitCode      = GetNextVdmCommand->ExitCode;
    CommandData->CurrentDrive  = GetNextVdmCommand->CurrentDrive;
    CommandData->VDMState      = GetNextVdmCommand->VDMState;
    CommandData->ComingFromBat = GetNextVdmCommand->fComingFromBat;

    /* It was successful */
    Success = TRUE;

Cleanup:
    if (CaptureBuffer != NULL) CsrFreeCaptureBuffer(CaptureBuffer);
    return Success;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetVDMCurrentDirectories(DWORD cchCurDirs, PCHAR lpszzCurDirs)
{
    BASE_API_MESSAGE ApiMessage;
    PBASE_GETSET_VDM_CURDIRS VDMCurrentDirsRequest = &ApiMessage.Data.VDMCurrentDirsRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    /* Allocate the capture buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(1, cchCurDirs);
    if (CaptureBuffer == NULL)
    {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return 0;
    }

    /* Setup the input parameters */
    VDMCurrentDirsRequest->cchCurDirs = cchCurDirs;
    CsrAllocateMessagePointer(CaptureBuffer,
                              cchCurDirs,
                              (PVOID*)&VDMCurrentDirsRequest->lpszzCurDirs);

    /* Call CSRSS */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepGetVDMCurDirs),
                        sizeof(*VDMCurrentDirsRequest));

    /* Set the last error */
    BaseSetLastNTError(ApiMessage.Status);

    if (NT_SUCCESS(ApiMessage.Status))
    {
        /* Copy the result */
        RtlMoveMemory(lpszzCurDirs, VDMCurrentDirsRequest->lpszzCurDirs, cchCurDirs);
    }

    /* Free the capture buffer */
    CsrFreeCaptureBuffer(CaptureBuffer);

    /* Return the size if it was successful, or if the buffer was too small */
    return (NT_SUCCESS(ApiMessage.Status) || (ApiMessage.Status == STATUS_BUFFER_TOO_SMALL))
           ? VDMCurrentDirsRequest->cchCurDirs : 0;
}


/*
 * @implemented (undocumented)
 */
BOOL
WINAPI
RegisterConsoleVDM(IN DWORD dwRegisterFlags,
                   IN HANDLE hStartHardwareEvent,
                   IN HANDLE hEndHardwareEvent,
                   IN HANDLE hErrorHardwareEvent,
                   IN DWORD dwUnusedVar,
                   OUT LPDWORD lpVideoStateLength,
                   OUT PVOID* lpVideoState, // PVIDEO_HARDWARE_STATE_HEADER*
                   IN PVOID lpUnusedBuffer,
                   IN DWORD dwUnusedBufferLength,
                   IN COORD dwVDMBufferSize,
                   OUT PVOID* lpVDMBuffer)
{
    BOOL Success;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_REGISTERVDM RegisterVDMRequest = &ApiMessage.Data.RegisterVDMRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;

    /* Set up the data to send to the Console Server */
    RegisterVDMRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    RegisterVDMRequest->RegisterFlags = dwRegisterFlags;

    if (dwRegisterFlags != 0)
    {
        RegisterVDMRequest->StartHardwareEvent = hStartHardwareEvent;
        RegisterVDMRequest->EndHardwareEvent   = hEndHardwareEvent;
        RegisterVDMRequest->ErrorHardwareEvent = hErrorHardwareEvent;

        RegisterVDMRequest->VDMBufferSize = dwVDMBufferSize;

#if 0
        RegisterVDMRequest->UnusedBufferLength = dwUnusedBufferLength;

        /* Allocate a Capture Buffer */
        CaptureBuffer = CsrAllocateCaptureBuffer(1, dwUnusedBufferLength);
        if (CaptureBuffer == NULL)
        {
            DPRINT1("CsrAllocateCaptureBuffer failed!\n");
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        /* Capture the buffer to write */
        CsrCaptureMessageBuffer(CaptureBuffer,
                                (PVOID)lpUnusedBuffer,
                                dwUnusedBufferLength,
                                (PVOID*)&RegisterVDMRequest->UnusedBuffer);
#endif
    }
    else
    {
        // CaptureBuffer = NULL;
    }

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepRegisterVDM),
                        sizeof(*RegisterVDMRequest));

    /* Check for success */
    Success = NT_SUCCESS(ApiMessage.Status);

    /* Release the capture buffer if needed */
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    /* Retrieve the results */
    if (Success)
    {
        if (dwRegisterFlags != 0)
        {
            _SEH2_TRY
            {
                *lpVideoStateLength = RegisterVDMRequest->VideoStateLength;
                *lpVideoState       = RegisterVDMRequest->VideoState;
                *lpVDMBuffer        = RegisterVDMRequest->VDMBuffer;
            }
            _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
            {
                SetLastError(ERROR_INVALID_ACCESS);
                Success = FALSE;
            }
            _SEH2_END;
        }
    }
    else
    {
        BaseSetLastNTError(ApiMessage.Status);
    }

    /* Return success status */
    return Success;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
RegisterWowBaseHandlers (
    DWORD   Unknown0
    )
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
RegisterWowExec (
    DWORD   Unknown0
    )
{
    STUB;
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetVDMCurrentDirectories(DWORD cchCurDirs, PCHAR lpszzCurDirs)
{
    BASE_API_MESSAGE ApiMessage;
    PBASE_GETSET_VDM_CURDIRS VDMCurrentDirsRequest = &ApiMessage.Data.VDMCurrentDirsRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    /* Allocate the capture buffer */
    CaptureBuffer = CsrAllocateCaptureBuffer(1, cchCurDirs);
    if (CaptureBuffer == NULL)
    {
        BaseSetLastNTError(STATUS_NO_MEMORY);
        return FALSE;
    }

    /* Setup the input parameters */
    VDMCurrentDirsRequest->cchCurDirs = cchCurDirs;
    CsrCaptureMessageBuffer(CaptureBuffer,
                            lpszzCurDirs,
                            cchCurDirs,
                            (PVOID*)&VDMCurrentDirsRequest->lpszzCurDirs);

    /* Call CSRSS */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(BASESRV_SERVERDLL_INDEX, BasepSetVDMCurDirs),
                        sizeof(*VDMCurrentDirsRequest));

    /* Free the capture buffer */
    CsrFreeCaptureBuffer(CaptureBuffer);

    /* Set the last error */
    BaseSetLastNTError(ApiMessage.Status);

    return NT_SUCCESS(ApiMessage.Status) ? TRUE : FALSE;
}

/*
 * @unimplemented
 */
DWORD
WINAPI
VDMConsoleOperation (
    DWORD   Unknown0,
    DWORD   Unknown1
    )
{
    STUB;
    return 0;
}


/*
 * @unimplemented
 */
BOOL
WINAPI
VDMOperationStarted(IN ULONG Unknown0)
{
    DPRINT1("VDMOperationStarted(%d)\n", Unknown0);

    return BaseUpdateVDMEntry(VdmEntryUpdateControlCHandler,
                              NULL,
                              0,
                              Unknown0);
}
