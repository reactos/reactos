/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/console/console.c
 * PURPOSE:         Win32 server console functions
 * PROGRAMMERS:     James Tabor <jimtabor@adsl-64-217-116-74.dsl.hstntx.swbell.net>
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

extern RTL_CRITICAL_SECTION ConsoleLock;
extern BOOLEAN ConsoleInitialized;

/* Console reserved "file" names */
static LPCWSTR BaseConFileName       = CONSOLE_FILE_NAME;
static LPCWSTR BaseConInputFileName  = CONSOLE_INPUT_FILE_NAME;
static LPCWSTR BaseConOutputFileName = CONSOLE_OUTPUT_FILE_NAME;

/* Console Control handling */
static PHANDLER_ROUTINE InitialHandler[1];
static PHANDLER_ROUTINE* CtrlHandlers;
static ULONG NrCtrlHandlers;
static ULONG NrAllocatedHandlers;
static BOOLEAN LastCloseNotify = FALSE;

extern BOOL WINAPI IsDebuggerPresent(VOID);

/* Console Input facilities */
HANDLE InputWaitHandle = INVALID_HANDLE_VALUE;

#define EXENAME_LENGTH (255 + 1)
static RTL_CRITICAL_SECTION ExeNameLock;
static BOOLEAN ExeNameInitialized;
static WCHAR ExeNameBuffer[EXENAME_LENGTH]; // NULL-terminated
static USHORT ExeNameLength;    // Count in number of characters without NULL
static WCHAR StartDirBuffer[MAX_PATH + 1];  // NULL-terminated
static USHORT StartDirLength;   // Count in number of characters without NULL


/* Default Console Control Handler ********************************************/

static BOOL
WINAPI
DefaultConsoleCtrlHandler(DWORD Event)
{
    DPRINT("Default handler called: %lx\n", Event);
    switch(Event)
    {
        case CTRL_C_EVENT:
            DPRINT("Ctrl-C Event\n");
            break;

        case CTRL_BREAK_EVENT:
            DPRINT("Ctrl-Break Event\n");
            break;

        case CTRL_CLOSE_EVENT:
            DPRINT("Ctrl Close Event\n");
            break;

        case CTRL_LAST_CLOSE_EVENT:
            DPRINT("Ctrl Last Close Event\n");
            break;

        case CTRL_LOGOFF_EVENT:
            DPRINT("Ctrl Logoff Event\n");
            break;

        case CTRL_SHUTDOWN_EVENT:
            DPRINT("Ctrl Shutdown Event\n");
            break;
    }

    ExitProcess(CONTROL_C_EXIT);
    return TRUE;
}

DWORD
WINAPI
ConsoleControlDispatcher(IN LPVOID lpThreadParameter)
{
    DWORD nExitCode = 0;
    DWORD CodeAndFlag = PtrToUlong(lpThreadParameter);
    DWORD nCode = CodeAndFlag & MAXLONG;
    UINT i;
    EXCEPTION_RECORD erException;

    DPRINT1("Console Dispatcher Active: %lx %lx\n", CodeAndFlag, nCode);
    SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

    switch(nCode)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        {
            if (IsDebuggerPresent())
            {
                erException.ExceptionCode = (nCode == CTRL_C_EVENT ?
                                             DBG_CONTROL_C : DBG_CONTROL_BREAK);
                erException.ExceptionFlags = 0;
                erException.ExceptionRecord = NULL;
                erException.ExceptionAddress = DefaultConsoleCtrlHandler;
                erException.NumberParameters = 0;

                _SEH2_TRY
                {
                    RtlRaiseException(&erException);
                }
                _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
                {
                    RtlEnterCriticalSection(&ConsoleLock);

                    if ((nCode != CTRL_C_EVENT) ||
                        (NtCurrentPeb()->ProcessParameters->ConsoleFlags != 1))
                    {
                        for (i = NrCtrlHandlers; i > 0; i--)
                        {
                            if (CtrlHandlers[i - 1](nCode)) break;
                        }
                    }

                    RtlLeaveCriticalSection(&ConsoleLock);
                }
                _SEH2_END;

                ExitThread(0);
            }
            break;
        }

        case CTRL_CLOSE_EVENT:
        case CTRL_LOGOFF_EVENT:
        case CTRL_SHUTDOWN_EVENT:
            break;

        case CTRL_LAST_CLOSE_EVENT:
            /*
             * In case the console app hasn't register for last close notification,
             * just kill this console handler thread. We don't want that such apps
             * get killed for unexpected reasons. On the contrary apps that registered
             * can be killed because they expect to be.
             */
            if (!LastCloseNotify) ExitThread(0);
            break;

        case 4:
            ExitProcess(CONTROL_C_EXIT);
            break;

        default:
            ASSERT(FALSE);
            break;
    }

    ASSERT(ConsoleInitialized);

    RtlEnterCriticalSection(&ConsoleLock);

    nExitCode = 0;
    if ((nCode != CTRL_C_EVENT) || (NtCurrentPeb()->ProcessParameters->ConsoleFlags != 1))
    {
        for (i = NrCtrlHandlers; i > 0; i--)
        {
            if ((i == 1) &&
                (CodeAndFlag & MINLONG) &&
                ((nCode == CTRL_LOGOFF_EVENT) || (nCode == CTRL_SHUTDOWN_EVENT)))
            {
                DPRINT("Skipping system/service apps\n");
                break;
            }

            if (CtrlHandlers[i - 1](nCode))
            {
                switch(nCode)
                {
                    case CTRL_CLOSE_EVENT:
                    case CTRL_LAST_CLOSE_EVENT:
                    case CTRL_LOGOFF_EVENT:
                    case CTRL_SHUTDOWN_EVENT:
                        nExitCode = CodeAndFlag;
                        break;
                }
                break;
            }
        }
    }

    RtlLeaveCriticalSection(&ConsoleLock);

    ExitThread(nExitCode);
    return STATUS_SUCCESS;
}

VOID
InitializeCtrlHandling(VOID)
{
    /* Initialize Console Ctrl Handler */
    NrAllocatedHandlers = NrCtrlHandlers = 1;
    CtrlHandlers = InitialHandler;
    CtrlHandlers[0] = DefaultConsoleCtrlHandler;
}


/* Input EXE Name Support *****************************************************/

VOID
InitExeName(VOID)
{
    NTSTATUS Status;
    PPEB Peb = NtCurrentPeb();
    PCURDIR CurrentDirectory = &Peb->ProcessParameters->CurrentDirectory;
    PLDR_DATA_TABLE_ENTRY ImageEntry;

    if (ExeNameInitialized) return;

    /* Initialize the EXE name lock */
    Status = RtlInitializeCriticalSection(&ExeNameLock);
    if (!NT_SUCCESS(Status)) return;
    ExeNameInitialized = TRUE;

    ImageEntry = CONTAINING_RECORD(Peb->Ldr->InLoadOrderModuleList.Flink,
                                   LDR_DATA_TABLE_ENTRY,
                                   InLoadOrderLinks);

    /* Retrieve the EXE name, NULL-terminate it... */
    ExeNameLength = min(sizeof(ExeNameBuffer)/sizeof(ExeNameBuffer[0]),
                        ImageEntry->BaseDllName.Length / sizeof(WCHAR));
    RtlCopyMemory(ExeNameBuffer,
                  ImageEntry->BaseDllName.Buffer,
                  ImageEntry->BaseDllName.Length);
    ExeNameBuffer[ExeNameLength] = UNICODE_NULL;

    /* ... and retrieve the current directory path and NULL-terminate it. */
    StartDirLength = min(sizeof(StartDirBuffer)/sizeof(StartDirBuffer[0]),
                         CurrentDirectory->DosPath.Length / sizeof(WCHAR));
    RtlCopyMemory(StartDirBuffer,
                  CurrentDirectory->DosPath.Buffer,
                  CurrentDirectory->DosPath.Length);
    StartDirBuffer[StartDirLength] = UNICODE_NULL;
}

/*
 * NOTE:
 * The "LPDWORD Length" parameters point on input to the maximum size of
 * the buffers that can hold data (if != 0), and on output they hold the
 * real size of the data. If "Length" are == 0 on input, then on output
 * they receive the full size of the data.
 * The "LPWSTR* String" parameters have a double meaning:
 * - when "CaptureStrings" is TRUE, data is copied to the buffers pointed
 *   by the pointers (*String).
 * - when "CaptureStrings" is FALSE, "*String" are set to the addresses of
 *   the source data.
 */
VOID
SetUpAppName(IN BOOLEAN CaptureStrings,
             IN OUT LPDWORD CurDirLength,
             IN OUT LPWSTR* CurDir,
             IN OUT LPDWORD AppNameLength,
             IN OUT LPWSTR* AppName)
{
    DWORD Length;

    /* Retrieve the needed buffer size */
    Length = (StartDirLength + 1) * sizeof(WCHAR);
    if (*CurDirLength > 0) Length = min(Length, *CurDirLength);
    *CurDirLength = Length;

    /* Capture the data if needed, or, return a pointer to it */
    if (CaptureStrings)
    {
        /*
         * Length is always >= sizeof(WCHAR). Copy everything but the
         * possible trailing NULL character, and then NULL-terminate.
         */
        Length -= sizeof(WCHAR);
        RtlCopyMemory(*CurDir, StartDirBuffer, Length);
        (*CurDir)[Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
    else
    {
        *CurDir = StartDirBuffer;
    }

    /* Retrieve the needed buffer size */
    Length = (ExeNameLength + 1) * sizeof(WCHAR);
    if (*AppNameLength > 0) Length = min(Length, *AppNameLength);
    *AppNameLength = Length;

    /* Capture the data if needed, or, return a pointer to it */
    if (CaptureStrings)
    {
        /*
         * Length is always >= sizeof(WCHAR). Copy everything but the
         * possible trailing NULL character, and then NULL-terminate.
         */
        Length -= sizeof(WCHAR);
        RtlCopyMemory(*AppName, ExeNameBuffer, Length);
        (*AppName)[Length / sizeof(WCHAR)] = UNICODE_NULL;
    }
    else
    {
        *AppName = ExeNameBuffer;
    }
}

USHORT
GetCurrentExeName(OUT PWCHAR ExeName,
                  IN USHORT BufferSize)
{
    USHORT ExeLength;

    if (ExeNameInitialized)
    {
        RtlEnterCriticalSection(&ExeNameLock);

        if (BufferSize > ExeNameLength * sizeof(WCHAR))
            BufferSize = ExeNameLength * sizeof(WCHAR);

        RtlCopyMemory(ExeName, ExeNameBuffer, BufferSize);

        RtlLeaveCriticalSection(&ExeNameLock);
        ExeLength = BufferSize;
    }
    else
    {
        *ExeName  = UNICODE_NULL;
        ExeLength = 0;
    }

    return ExeLength;
}

/* FUNCTIONS ******************************************************************/

LPCWSTR
IntCheckForConsoleFileName(IN LPCWSTR pszName,
                           IN DWORD dwDesiredAccess)
{
    LPCWSTR ConsoleName = pszName;
    ULONG DeviceNameInfo;

    /*
     * Check whether we deal with a DOS device, and if so,
     * strip the path till the file name.
     * Therefore, things like \\.\CON or C:\some_path\CONIN$
     * are transformed into CON or CONIN$, for example.
     */
    DeviceNameInfo = RtlIsDosDeviceName_U(pszName);
    if (DeviceNameInfo != 0)
    {
        ConsoleName = (LPCWSTR)((ULONG_PTR)ConsoleName + ((DeviceNameInfo >> 16) & 0xFFFF));
    }

    /* Return a standard console "file" name according to what we passed in parameters */
    if (_wcsicmp(ConsoleName, BaseConInputFileName) == 0)
    {
        return BaseConInputFileName;
    }
    else if (_wcsicmp(ConsoleName, BaseConOutputFileName) == 0)
    {
        return BaseConOutputFileName;
    }
    else if (_wcsicmp(ConsoleName, BaseConFileName) == 0)
    {
        if ((dwDesiredAccess & (GENERIC_READ | GENERIC_WRITE)) == GENERIC_READ)
        {
            return BaseConInputFileName;
        }
        else if ((dwDesiredAccess & (GENERIC_READ | GENERIC_WRITE)) == GENERIC_WRITE)
        {
            return BaseConOutputFileName;
        }
    }

    /* If we are there, that means that either the file name or the desired access are wrong */
    return NULL;
}


/*
 * @implemented (Undocumented)
 * @note See http://undoc.airesoft.co.uk/kernel32.dll/ConsoleMenuControl.php
 */
HMENU
WINAPI
DECLSPEC_HOTPATCH
ConsoleMenuControl(HANDLE hConsoleOutput,
                   DWORD dwCmdIdLow,
                   DWORD dwCmdIdHigh)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_MENUCONTROL MenuControlRequest = &ApiMessage.Data.MenuControlRequest;

    MenuControlRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    MenuControlRequest->OutputHandle  = hConsoleOutput;
    MenuControlRequest->CmdIdLow      = dwCmdIdLow;
    MenuControlRequest->CmdIdHigh     = dwCmdIdHigh;
    MenuControlRequest->MenuHandle    = NULL;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepMenuControl),
                        sizeof(*MenuControlRequest));

    return MenuControlRequest->MenuHandle;
}


/*
 * @implemented
 */
HANDLE
WINAPI
DECLSPEC_HOTPATCH
DuplicateConsoleHandle(HANDLE hConsole,
                       DWORD dwDesiredAccess,
                       BOOL bInheritHandle,
                       DWORD dwOptions)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_DUPLICATEHANDLE DuplicateHandleRequest = &ApiMessage.Data.DuplicateHandleRequest;

    if ( (dwOptions & ~(DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS)) ||
         (!(dwOptions & DUPLICATE_SAME_ACCESS) &&
           (dwDesiredAccess & ~(GENERIC_READ | GENERIC_WRITE))) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    DuplicateHandleRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    DuplicateHandleRequest->SourceHandle  = hConsole;
    DuplicateHandleRequest->DesiredAccess = dwDesiredAccess;
    DuplicateHandleRequest->InheritHandle = bInheritHandle;
    DuplicateHandleRequest->Options       = dwOptions;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepDuplicateHandle),
                        sizeof(*DuplicateHandleRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return INVALID_HANDLE_VALUE;
    }

    return DuplicateHandleRequest->TargetHandle;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetConsoleHandleInformation(IN HANDLE hHandle,
                            OUT LPDWORD lpdwFlags)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETHANDLEINFO GetHandleInfoRequest = &ApiMessage.Data.GetHandleInfoRequest;

    GetHandleInfoRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetHandleInfoRequest->Handle        = hHandle;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetHandleInformation),
                        sizeof(*GetHandleInfoRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    *lpdwFlags = GetHandleInfoRequest->Flags;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetConsoleHandleInformation(IN HANDLE hHandle,
                            IN DWORD dwMask,
                            IN DWORD dwFlags)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETHANDLEINFO SetHandleInfoRequest = &ApiMessage.Data.SetHandleInfoRequest;

    SetHandleInfoRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetHandleInfoRequest->Handle        = hHandle;
    SetHandleInfoRequest->Mask          = dwMask;
    SetHandleInfoRequest->Flags         = dwFlags;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetHandleInformation),
                        sizeof(*SetHandleInfoRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetConsoleDisplayMode(LPDWORD lpModeFlags)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETDISPLAYMODE GetDisplayModeRequest = &ApiMessage.Data.GetDisplayModeRequest;

    if (lpModeFlags == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    GetDisplayModeRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetDisplayMode),
                        sizeof(*GetDisplayModeRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    *lpModeFlags = GetDisplayModeRequest->DisplayMode; // ModeFlags

    return TRUE;
}


/*
 * @implemented (Undocumented)
 * @note See http://cboard.cprogramming.com/windows-programming/102187-console-font-size.html
 */
DWORD
WINAPI
GetConsoleFontInfo(IN HANDLE hConsoleOutput,
                   IN BOOL bMaximumWindow,
                   IN DWORD nFontCount,
                   OUT PCONSOLE_FONT_INFO lpConsoleFontInfo)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETFONTINFO GetFontInfoRequest = &ApiMessage.Data.GetFontInfoRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    GetFontInfoRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetFontInfoRequest->OutputHandle  = hConsoleOutput;
    GetFontInfoRequest->MaximumWindow = bMaximumWindow;
    GetFontInfoRequest->NumFonts      = nFontCount;

    CaptureBuffer = CsrAllocateCaptureBuffer(1, nFontCount * sizeof(CONSOLE_FONT_INFO));
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    CsrAllocateMessagePointer(CaptureBuffer,
                              nFontCount * sizeof(CONSOLE_FONT_INFO),
                              (PVOID*)&GetFontInfoRequest->FontInfo);

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetFontInfo),
                        sizeof(*GetFontInfoRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
    }
    else
    {
        RtlCopyMemory(lpConsoleFontInfo,
                      GetFontInfoRequest->FontInfo,
                      GetFontInfoRequest->NumFonts * sizeof(CONSOLE_FONT_INFO));
    }

    CsrFreeCaptureBuffer(CaptureBuffer);
    return GetFontInfoRequest->NumFonts;
}


/*
 * @implemented
 */
COORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleFontSize(IN HANDLE hConsoleOutput,
                   IN DWORD nFont)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETFONTSIZE GetFontSizeRequest = &ApiMessage.Data.GetFontSizeRequest;
    COORD Empty = {0, 0};

    GetFontSizeRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetFontSizeRequest->OutputHandle  = hConsoleOutput;
    GetFontSizeRequest->FontIndex     = nFont;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetFontSize),
                        sizeof(*GetFontSizeRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return Empty;
    }

    return GetFontSizeRequest->FontSize;
}


/*
 * @implemented (Undocumented)
 */
BOOL
WINAPI
GetConsoleHardwareState(HANDLE hConsoleOutput,
                        PDWORD Flags,
                        PDWORD State)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETHWSTATE HardwareStateRequest = &ApiMessage.Data.HardwareStateRequest;

    DPRINT1("GetConsoleHardwareState(%lu, 0x%p) UNIMPLEMENTED!\n", Flags, State);

    if (Flags == NULL || State == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    HardwareStateRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    HardwareStateRequest->OutputHandle  = hConsoleOutput;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetHardwareState),
                        sizeof(*HardwareStateRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    *Flags = HardwareStateRequest->Flags;
    *State = HardwareStateRequest->State;

    return TRUE;
}


/*
 * @implemented (Undocumented)
 */
HANDLE
WINAPI
GetConsoleInputWaitHandle(VOID)
{
    return InputWaitHandle;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetCurrentConsoleFont(IN HANDLE hConsoleOutput,
                      IN BOOL bMaximumWindow,
                      OUT PCONSOLE_FONT_INFO lpConsoleCurrentFont)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETCURRENTFONT GetCurrentFontRequest = &ApiMessage.Data.GetCurrentFontRequest;

    GetCurrentFontRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetCurrentFontRequest->OutputHandle  = hConsoleOutput;
    GetCurrentFontRequest->MaximumWindow = bMaximumWindow;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetCurrentFont),
                        sizeof(*GetCurrentFontRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    lpConsoleCurrentFont->dwFontSize = GetCurrentFontRequest->FontSize;
    lpConsoleCurrentFont->nFont      = GetCurrentFontRequest->FontIndex;

    return TRUE;
}


/*
 * @implemented (Undocumented)
 * @note See http://cboard.cprogramming.com/windows-programming/102187-console-font-size.html
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetNumberOfConsoleFonts(VOID)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETNUMFONTS GetNumFontsRequest = &ApiMessage.Data.GetNumFontsRequest;

    GetNumFontsRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetNumberOfFonts),
                        sizeof(*GetNumFontsRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return 0;
    }

    return GetNumFontsRequest->NumFonts;
}


/*
 * @implemented (Undocumented)
 * @note See http://blog.airesoft.co.uk/2012/10/things-ms-can-do-that-they-dont-tell-you-about-console-graphics/
 */
BOOL
WINAPI
InvalidateConsoleDIBits(IN HANDLE hConsoleOutput,
                        IN PSMALL_RECT lpRect)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_INVALIDATEDIBITS InvalidateDIBitsRequest = &ApiMessage.Data.InvalidateDIBitsRequest;

    if (lpRect == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    InvalidateDIBitsRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    InvalidateDIBitsRequest->OutputHandle  = hConsoleOutput;
    InvalidateDIBitsRequest->Region        = *lpRect;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepInvalidateBitMapRect),
                        sizeof(*InvalidateDIBitsRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented (Undocumented)
 */
HANDLE
WINAPI
OpenConsoleW(LPCWSTR wsName,
             DWORD dwDesiredAccess,
             BOOL bInheritHandle,
             DWORD dwShareMode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_OPENCONSOLE OpenConsoleRequest = &ApiMessage.Data.OpenConsoleRequest;
    CONSOLE_HANDLE_TYPE HandleType;

    if (wsName && (_wcsicmp(wsName, BaseConInputFileName) == 0))
    {
        HandleType = HANDLE_INPUT;
    }
    else if (wsName && (_wcsicmp(wsName, BaseConOutputFileName) == 0))
    {
        HandleType = HANDLE_OUTPUT;
    }
    else
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    if ( (dwDesiredAccess & ~(GENERIC_READ | GENERIC_WRITE)) ||
         (dwShareMode & ~(FILE_SHARE_READ | FILE_SHARE_WRITE)) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    OpenConsoleRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    OpenConsoleRequest->HandleType    = HandleType;
    OpenConsoleRequest->DesiredAccess = dwDesiredAccess;
    OpenConsoleRequest->InheritHandle = bInheritHandle;
    OpenConsoleRequest->ShareMode     = dwShareMode;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepOpenConsole),
                        sizeof(*OpenConsoleRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return INVALID_HANDLE_VALUE;
    }

    return OpenConsoleRequest->Handle;
}


/*
 * @implemented (Undocumented)
 * @note See http://undoc.airesoft.co.uk/kernel32.dll/SetConsoleCursor.php
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleCursor(HANDLE  hConsoleOutput,
                 HCURSOR hCursor)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETCURSOR SetCursorRequest = &ApiMessage.Data.SetCursorRequest;

    SetCursorRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetCursorRequest->OutputHandle  = hConsoleOutput;
    SetCursorRequest->CursorHandle  = hCursor;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetCursor),
                        sizeof(*SetCursorRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetConsoleDisplayMode(HANDLE hConsoleOutput,
                      DWORD  dwFlags, // dwModeFlags
                      PCOORD lpNewScreenBufferDimensions)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETDISPLAYMODE SetDisplayModeRequest = &ApiMessage.Data.SetDisplayModeRequest;

    SetDisplayModeRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetDisplayModeRequest->OutputHandle  = hConsoleOutput;
    SetDisplayModeRequest->DisplayMode   = dwFlags; // ModeFlags ; dwModeFlags
    SetDisplayModeRequest->NewSBDim.X    = 0;
    SetDisplayModeRequest->NewSBDim.Y    = 0;
    /* SetDisplayModeRequest->EventHandle; */

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetDisplayMode),
                        sizeof(*SetDisplayModeRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    if (lpNewScreenBufferDimensions)
        *lpNewScreenBufferDimensions = SetDisplayModeRequest->NewSBDim;

    return TRUE;
}


/*
 * @implemented (Undocumented)
 * @note See http://cboard.cprogramming.com/windows-programming/102187-console-font-size.html
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleFont(IN HANDLE hConsoleOutput,
               IN DWORD nFont)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETFONT SetFontRequest = &ApiMessage.Data.SetFontRequest;

    SetFontRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetFontRequest->OutputHandle  = hConsoleOutput;
    SetFontRequest->FontIndex     = nFont;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetFont),
                        sizeof(*SetFontRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented (Undocumented)
 */
BOOL
WINAPI
SetConsoleHardwareState(HANDLE hConsoleOutput,
                        DWORD Flags,
                        DWORD State)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETHWSTATE HardwareStateRequest = &ApiMessage.Data.HardwareStateRequest;

    DPRINT1("SetConsoleHardwareState(%lu, %lu) UNIMPLEMENTED!\n", Flags, State);

    HardwareStateRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    HardwareStateRequest->OutputHandle  = hConsoleOutput;
    HardwareStateRequest->Flags         = Flags;
    HardwareStateRequest->State         = State;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetHardwareState),
                        sizeof(*HardwareStateRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @unimplemented (Undocumented)
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleKeyShortcuts(DWORD Unknown0,
                       DWORD Unknown1,
                       DWORD Unknown2,
                       DWORD Unknown3)
{
    DPRINT1("SetConsoleKeyShortcuts(0x%x, 0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2, Unknown3);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @implemented (Undocumented)
 * @note See http://undoc.airesoft.co.uk/kernel32.dll/SetConsoleMaximumWindowSize.php
 *       Does nothing, returns TRUE only. Checked on Windows Server 2003.
 */
BOOL
WINAPI
SetConsoleMaximumWindowSize(HANDLE hConsoleOutput,
                            COORD dwMaximumSize)
{
    DPRINT1("SetConsoleMaximumWindowSize(0x%p, {%d, %d}) does nothing\n",
            hConsoleOutput, dwMaximumSize.X, dwMaximumSize.Y);
    return TRUE;
}


/*
 * @implemented (Undocumented)
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleMenuClose(BOOL bEnable)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETMENUCLOSE SetMenuCloseRequest = &ApiMessage.Data.SetMenuCloseRequest;

    SetMenuCloseRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetMenuCloseRequest->Enable        = bEnable;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetMenuClose),
                        sizeof(*SetMenuCloseRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented (Undocumented)
 * @note See http://comments.gmane.org/gmane.comp.lang.harbour.devel/27844
 *       Usage example: https://github.com/harbour/core/commit/d79a1b7b812cbde6ddf718ebfd6939a24f633e52
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsolePalette(HANDLE hConsoleOutput,
                  HPALETTE hPalette,
                  UINT dwUsage)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETPALETTE SetPaletteRequest = &ApiMessage.Data.SetPaletteRequest;

    SetPaletteRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetPaletteRequest->OutputHandle  = hConsoleOutput;
    SetPaletteRequest->PaletteHandle = hPalette;
    SetPaletteRequest->Usage         = dwUsage;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetPalette),
                        sizeof(*SetPaletteRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented (Undocumented)
 * @note See http://undoc.airesoft.co.uk/kernel32.dll/ShowConsoleCursor.php
 */
INT
WINAPI
DECLSPEC_HOTPATCH
ShowConsoleCursor(HANDLE hConsoleOutput,
                  BOOL bShow)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SHOWCURSOR ShowCursorRequest = &ApiMessage.Data.ShowCursorRequest;

    ShowCursorRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    ShowCursorRequest->OutputHandle  = hConsoleOutput;
    ShowCursorRequest->Show          = bShow;
    ShowCursorRequest->RefCount      = 0;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepShowCursor),
                        sizeof(*ShowCursorRequest));

    return ShowCursorRequest->RefCount;
}


/*
 * FUNCTION: Checks whether the given handle is a valid console handle.
 *
 * ARGUMENTS:
 *      hIoHandle - Handle to be checked.
 *
 * RETURNS:
 *      TRUE : Handle is a valid console handle.
 *      FALSE: Handle is not a valid console handle.
 *
 * STATUS: Officially undocumented
 *
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
VerifyConsoleIoHandle(HANDLE hIoHandle)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_VERIFYHANDLE VerifyHandleRequest = &ApiMessage.Data.VerifyHandleRequest;

    VerifyHandleRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    VerifyHandleRequest->Handle        = hIoHandle;
    VerifyHandleRequest->IsValid       = FALSE;

    /* If the process is not attached to a console, return invalid handle */
    if (VerifyHandleRequest->ConsoleHandle == NULL) return FALSE;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepVerifyIoHandle),
                        sizeof(*VerifyHandleRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return VerifyHandleRequest->IsValid;
}


/*
 * @implemented (Undocumented)
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
CloseConsoleHandle(HANDLE hHandle)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_CLOSEHANDLE CloseHandleRequest = &ApiMessage.Data.CloseHandleRequest;

    CloseHandleRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    CloseHandleRequest->Handle        = hHandle;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepCloseHandle),
                        sizeof(*CloseHandleRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
HANDLE
WINAPI
DECLSPEC_HOTPATCH
GetStdHandle(DWORD nStdHandle)
/*
 * FUNCTION: Get a handle for the standard input, standard output
 * and a standard error device.
 *
 * ARGUMENTS:
 *       nStdHandle - Specifies the device for which to return the handle.
 *
 * RETURNS: If the function succeeds, the return value is the handle
 * of the specified device. Otherwise the value is INVALID_HANDLE_VALUE.
 */
{
    PRTL_USER_PROCESS_PARAMETERS Ppb = NtCurrentPeb()->ProcessParameters;
    HANDLE Handle = INVALID_HANDLE_VALUE;

    switch (nStdHandle)
    {
        case STD_INPUT_HANDLE:
            Handle = Ppb->StandardInput;
            break;

        case STD_OUTPUT_HANDLE:
            Handle = Ppb->StandardOutput;
            break;

        case STD_ERROR_HANDLE:
            Handle = Ppb->StandardError;
            break;
    }

    /* If the returned handle is invalid, set last error */
    if (Handle == INVALID_HANDLE_VALUE) SetLastError(ERROR_INVALID_HANDLE);

    return Handle;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetStdHandle(DWORD  nStdHandle,
             HANDLE hHandle)
/*
 * FUNCTION: Set the handle for the standard input, standard output or
 * the standard error device.
 *
 * ARGUMENTS:
 *        nStdHandle - Specifies the handle to be set.
 *        hHandle - The handle to set.
 *
 * RETURNS: TRUE if the function succeeds, FALSE otherwise.
 */
{
    PRTL_USER_PROCESS_PARAMETERS Ppb = NtCurrentPeb()->ProcessParameters;

    /* No need to check if hHandle == INVALID_HANDLE_VALUE */

    switch (nStdHandle)
    {
        case STD_INPUT_HANDLE:
            Ppb->StandardInput = hHandle;
            return TRUE;

        case STD_OUTPUT_HANDLE:
            Ppb->StandardOutput = hHandle;
            return TRUE;

        case STD_ERROR_HANDLE:
            Ppb->StandardError = hHandle;
            return TRUE;
    }

    /* nStdHandle was invalid, bail out */
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}


/*
 * @implemented
 */
static BOOL
IntAllocConsole(LPWSTR Title,
                DWORD TitleLength,
                LPWSTR Desktop,
                DWORD DesktopLength,
                LPWSTR CurDir,
                DWORD CurDirLength,
                LPWSTR AppName,
                DWORD AppNameLength,
                LPTHREAD_START_ROUTINE CtrlRoutine,
                LPTHREAD_START_ROUTINE PropRoutine,
                PCONSOLE_START_INFO ConsoleStartInfo)
{
    BOOL Success = TRUE;
    NTSTATUS Status;

    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_ALLOCCONSOLE AllocConsoleRequest = &ApiMessage.Data.AllocConsoleRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    AllocConsoleRequest->CtrlRoutine = CtrlRoutine;
    AllocConsoleRequest->PropRoutine = PropRoutine;

    CaptureBuffer = CsrAllocateCaptureBuffer(5, TitleLength   +
                                                DesktopLength +
                                                CurDirLength  +
                                                AppNameLength +
                                                sizeof(CONSOLE_START_INFO));
    if (CaptureBuffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        Success = FALSE;
        goto Quit;
    }

    CsrCaptureMessageBuffer(CaptureBuffer,
                            ConsoleStartInfo,
                            sizeof(CONSOLE_START_INFO),
                            (PVOID*)&AllocConsoleRequest->ConsoleStartInfo);

    AllocConsoleRequest->TitleLength = TitleLength;
    CsrCaptureMessageBuffer(CaptureBuffer,
                            Title,
                            TitleLength,
                            (PVOID*)&AllocConsoleRequest->ConsoleTitle);

    AllocConsoleRequest->DesktopLength = DesktopLength;
    CsrCaptureMessageBuffer(CaptureBuffer,
                            Desktop,
                            DesktopLength,
                            (PVOID*)&AllocConsoleRequest->Desktop);

    AllocConsoleRequest->CurDirLength = CurDirLength;
    CsrCaptureMessageBuffer(CaptureBuffer,
                            CurDir,
                            CurDirLength,
                            (PVOID*)&AllocConsoleRequest->CurDir);

    AllocConsoleRequest->AppNameLength = AppNameLength;
    CsrCaptureMessageBuffer(CaptureBuffer,
                            AppName,
                            AppNameLength,
                            (PVOID*)&AllocConsoleRequest->AppName);

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepAlloc),
                        sizeof(*AllocConsoleRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        Success = FALSE;
        goto Quit;
    }

    // Is AllocConsoleRequest->ConsoleStartInfo->InitEvents aligned on handle boundary ????
    Status = NtWaitForMultipleObjects(MAX_INIT_EVENTS,
                                      AllocConsoleRequest->ConsoleStartInfo->InitEvents,
                                      WaitAny, FALSE, NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        Success = FALSE;
        goto Quit;
    }

    NtClose(AllocConsoleRequest->ConsoleStartInfo->InitEvents[INIT_SUCCESS]);
    NtClose(AllocConsoleRequest->ConsoleStartInfo->InitEvents[INIT_FAILURE]);
    if (Status != INIT_SUCCESS)
    {
        NtCurrentPeb()->ProcessParameters->ConsoleHandle = NULL;
        Success = FALSE;
    }
    else
    {
        RtlCopyMemory(ConsoleStartInfo,
                      AllocConsoleRequest->ConsoleStartInfo,
                      sizeof(CONSOLE_START_INFO));
        Success = TRUE;
    }

Quit:
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);
    return Success;
}

BOOL
WINAPI
DECLSPEC_HOTPATCH
AllocConsole(VOID)
{
    BOOL Success;
    CONSOLE_START_INFO ConsoleStartInfo;

    PWCHAR ConsoleTitle;
    PWCHAR Desktop;
    PWCHAR AppName;
    PWCHAR CurDir;

    ULONG TitleLength   = (MAX_PATH + 1) * sizeof(WCHAR);
    ULONG DesktopLength = (MAX_PATH + 1) * sizeof(WCHAR);
    ULONG AppNameLength = 128 * sizeof(WCHAR);
    ULONG CurDirLength  = (MAX_PATH + 1) * sizeof(WCHAR);

    LCID lcid;

    RtlEnterCriticalSection(&ConsoleLock);

    if (NtCurrentPeb()->ProcessParameters->ConsoleHandle)
    {
        DPRINT1("AllocConsole: Allocating a console to a process already having one\n");
        SetLastError(ERROR_ACCESS_DENIED);
        Success = FALSE;
        goto Quit;
    }

    /* Set up the console properties */
    SetUpConsoleInfo(FALSE,
                     &TitleLength,
                     &ConsoleTitle,
                     &DesktopLength,
                     &Desktop,
                     &ConsoleStartInfo);
    DPRINT("ConsoleTitle = '%S' - Desktop = '%S'\n",
           ConsoleTitle, Desktop);

    /* Initialize the Input EXE name */
    InitExeName();
    SetUpAppName(FALSE,
                 &CurDirLength,
                 &CurDir,
                 &AppNameLength,
                 &AppName);
    DPRINT("CurDir = '%S' - AppName = '%S'\n",
           CurDir, AppName);

    Success = IntAllocConsole(ConsoleTitle,
                              TitleLength,
                              Desktop,
                              DesktopLength,
                              CurDir,
                              CurDirLength,
                              AppName,
                              AppNameLength,
                              ConsoleControlDispatcher,
                              PropDialogHandler,
                              &ConsoleStartInfo);
    if (Success)
    {
        /* Set up the handles */
        SetUpHandles(&ConsoleStartInfo);
        InputWaitHandle = ConsoleStartInfo.InputWaitHandle;

        /* Initialize Console Ctrl Handling */
        InitializeCtrlHandling();

        /* Sets the current console locale for this thread */
        SetTEBLangID(lcid);
    }

Quit:
    RtlLeaveCriticalSection(&ConsoleLock);
    return Success;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
FreeConsole(VOID)
{
    BOOL Success = TRUE;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_FREECONSOLE FreeConsoleRequest = &ApiMessage.Data.FreeConsoleRequest;
    HANDLE ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;

    RtlEnterCriticalSection(&ConsoleLock);

    /* We must have a non-trivial handle to close */
    if (ConsoleHandle == NULL) // IsConsoleHandle(ConsoleHandle)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        Success = FALSE;
        goto Quit;
    }

    /* Set up the data to send to the Console Server */
    FreeConsoleRequest->ConsoleHandle = ConsoleHandle;

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepFree),
                        sizeof(*FreeConsoleRequest));

    /* Check for success */
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        Success = FALSE;
        goto Quit;
    }

    /* Reset the console handle */
    NtCurrentPeb()->ProcessParameters->ConsoleHandle = NULL;

    /* Close the associated input handle */
    CloseHandle(InputWaitHandle);
    InputWaitHandle = INVALID_HANDLE_VALUE;

Quit:
    RtlLeaveCriticalSection(&ConsoleLock);
    return Success;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetConsoleScreenBufferInfo(HANDLE hConsoleOutput,
                           PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSCREENBUFFERINFO ScreenBufferInfoRequest = &ApiMessage.Data.ScreenBufferInfoRequest;

    if (lpConsoleScreenBufferInfo == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ScreenBufferInfoRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    ScreenBufferInfoRequest->OutputHandle  = hConsoleOutput;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetScreenBufferInfo),
                        sizeof(*ScreenBufferInfoRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    lpConsoleScreenBufferInfo->dwSize              = ScreenBufferInfoRequest->ScreenBufferSize;
    lpConsoleScreenBufferInfo->dwCursorPosition    = ScreenBufferInfoRequest->CursorPosition;
    lpConsoleScreenBufferInfo->wAttributes         = ScreenBufferInfoRequest->Attributes;
    lpConsoleScreenBufferInfo->srWindow.Left       = ScreenBufferInfoRequest->ViewOrigin.X;
    lpConsoleScreenBufferInfo->srWindow.Top        = ScreenBufferInfoRequest->ViewOrigin.Y;
    lpConsoleScreenBufferInfo->srWindow.Right      = ScreenBufferInfoRequest->ViewOrigin.X + ScreenBufferInfoRequest->ViewSize.X - 1;
    lpConsoleScreenBufferInfo->srWindow.Bottom     = ScreenBufferInfoRequest->ViewOrigin.Y + ScreenBufferInfoRequest->ViewSize.Y - 1;
    lpConsoleScreenBufferInfo->dwMaximumWindowSize = ScreenBufferInfoRequest->MaximumViewSize;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleCursorPosition(HANDLE hConsoleOutput,
                         COORD dwCursorPosition)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETCURSORPOSITION SetCursorPositionRequest = &ApiMessage.Data.SetCursorPositionRequest;

    SetCursorPositionRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetCursorPositionRequest->OutputHandle  = hConsoleOutput;
    SetCursorPositionRequest->Position      = dwCursorPosition;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetCursorPosition),
                        sizeof(*SetCursorPositionRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetConsoleMode(HANDLE hConsoleHandle,
               LPDWORD lpMode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &ApiMessage.Data.ConsoleModeRequest;

    if (lpMode == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ConsoleModeRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    ConsoleModeRequest->Handle        = hConsoleHandle;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetMode),
                        sizeof(*ConsoleModeRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    *lpMode = ConsoleModeRequest->Mode;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleMode(HANDLE hConsoleHandle,
               DWORD dwMode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &ApiMessage.Data.ConsoleModeRequest;

    ConsoleModeRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    ConsoleModeRequest->Handle        = hConsoleHandle;
    ConsoleModeRequest->Mode          = dwMode;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetMode),
                        sizeof(*ConsoleModeRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetNumberOfConsoleInputEvents(HANDLE hConsoleInput,
                              LPDWORD lpNumberOfEvents)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETNUMINPUTEVENTS GetNumInputEventsRequest = &ApiMessage.Data.GetNumInputEventsRequest;

    GetNumInputEventsRequest->ConsoleHandle  = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetNumInputEventsRequest->InputHandle    = hConsoleInput;
    GetNumInputEventsRequest->NumberOfEvents = 0;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetNumberOfInputEvents),
                        sizeof(*GetNumInputEventsRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    if (lpNumberOfEvents == NULL)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

    *lpNumberOfEvents = GetNumInputEventsRequest->NumberOfEvents;

    return TRUE;
}


/*
 * @implemented
 */
COORD
WINAPI
DECLSPEC_HOTPATCH
GetLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETLARGESTWINDOWSIZE GetLargestWindowSizeRequest = &ApiMessage.Data.GetLargestWindowSizeRequest;

    GetLargestWindowSizeRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetLargestWindowSizeRequest->OutputHandle  = hConsoleOutput;
    GetLargestWindowSizeRequest->Size.X = 0;
    GetLargestWindowSizeRequest->Size.Y = 0;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetLargestWindowSize),
                        sizeof(*GetLargestWindowSizeRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
    }

    DPRINT("GetLargestConsoleWindowSize, X = %d, Y = %d\n", GetLargestWindowSizeRequest->Size.X, GetLargestWindowSizeRequest->Size.Y);
    return GetLargestWindowSizeRequest->Size;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetConsoleCursorInfo(HANDLE hConsoleOutput,
                     PCONSOLE_CURSOR_INFO lpConsoleCursorInfo)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETCURSORINFO CursorInfoRequest = &ApiMessage.Data.CursorInfoRequest;

    if (!lpConsoleCursorInfo)
    {
        if (!hConsoleOutput)
            SetLastError(ERROR_INVALID_HANDLE);
        else
            SetLastError(ERROR_INVALID_ACCESS);

        return FALSE;
    }

    CursorInfoRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    CursorInfoRequest->OutputHandle  = hConsoleOutput;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetCursorInfo),
                        sizeof(*CursorInfoRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    *lpConsoleCursorInfo = CursorInfoRequest->Info;

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetConsoleCursorInfo(HANDLE hConsoleOutput,
                     CONST CONSOLE_CURSOR_INFO *lpConsoleCursorInfo)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETCURSORINFO CursorInfoRequest = &ApiMessage.Data.CursorInfoRequest;

    CursorInfoRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    CursorInfoRequest->OutputHandle  = hConsoleOutput;
    CursorInfoRequest->Info          = *lpConsoleCursorInfo;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetCursorInfo),
                        sizeof(*CursorInfoRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetNumberOfConsoleMouseButtons(LPDWORD lpNumberOfMouseButtons)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETMOUSEINFO GetMouseInfoRequest = &ApiMessage.Data.GetMouseInfoRequest;

    GetMouseInfoRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetMouseInfo),
                        sizeof(*GetMouseInfoRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    *lpNumberOfMouseButtons = GetMouseInfoRequest->NumButtons;
    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleActiveScreenBuffer(HANDLE hConsoleOutput)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETACTIVESCREENBUFFER SetScreenBufferRequest = &ApiMessage.Data.SetScreenBufferRequest;

    SetScreenBufferRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetScreenBufferRequest->OutputHandle  = hConsoleOutput;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetActiveScreenBuffer),
                        sizeof(*SetScreenBufferRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
FlushConsoleInputBuffer(HANDLE hConsoleInput)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_FLUSHINPUTBUFFER FlushInputBufferRequest = &ApiMessage.Data.FlushInputBufferRequest;

    FlushInputBufferRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    FlushInputBufferRequest->InputHandle   = hConsoleInput;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepFlushInputBuffer),
                        sizeof(*FlushInputBufferRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleScreenBufferSize(HANDLE hConsoleOutput,
                           COORD dwSize)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETSCREENBUFFERSIZE SetScreenBufferSizeRequest = &ApiMessage.Data.SetScreenBufferSizeRequest;

    SetScreenBufferSizeRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetScreenBufferSizeRequest->OutputHandle  = hConsoleOutput;
    SetScreenBufferSizeRequest->Size          = dwSize;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetScreenBufferSize),
                        sizeof(*SetScreenBufferSizeRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


static
BOOL
IntScrollConsoleScreenBuffer(HANDLE hConsoleOutput,
                             CONST SMALL_RECT* lpScrollRectangle,
                             CONST SMALL_RECT* lpClipRectangle,
                             COORD dwDestinationOrigin,
                             CONST CHAR_INFO* lpFill,
                             BOOL bUnicode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SCROLLSCREENBUFFER ScrollScreenBufferRequest = &ApiMessage.Data.ScrollScreenBufferRequest;

    ScrollScreenBufferRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    ScrollScreenBufferRequest->OutputHandle  = hConsoleOutput;
    ScrollScreenBufferRequest->ScrollRectangle = *lpScrollRectangle;

    if (lpClipRectangle != NULL)
    {
        ScrollScreenBufferRequest->UseClipRectangle = TRUE;
        ScrollScreenBufferRequest->ClipRectangle = *lpClipRectangle;
    }
    else
    {
        ScrollScreenBufferRequest->UseClipRectangle = FALSE;
    }

    ScrollScreenBufferRequest->DestinationOrigin = dwDestinationOrigin;
    ScrollScreenBufferRequest->Fill    = *lpFill;
    ScrollScreenBufferRequest->Unicode = bUnicode;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepScrollScreenBuffer),
                        sizeof(*ScrollScreenBufferRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
ScrollConsoleScreenBufferA(HANDLE hConsoleOutput,
                           CONST SMALL_RECT* lpScrollRectangle,
                           CONST SMALL_RECT* lpClipRectangle,
                           COORD dwDestinationOrigin,
                           CONST CHAR_INFO* lpFill)
{
    return IntScrollConsoleScreenBuffer(hConsoleOutput,
                                        lpScrollRectangle,
                                        lpClipRectangle,
                                        dwDestinationOrigin,
                                        lpFill,
                                        FALSE);
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
ScrollConsoleScreenBufferW(HANDLE hConsoleOutput,
                           CONST SMALL_RECT *lpScrollRectangle,
                           CONST SMALL_RECT *lpClipRectangle,
                           COORD dwDestinationOrigin,
                           CONST CHAR_INFO *lpFill)
{
    return IntScrollConsoleScreenBuffer(hConsoleOutput,
                                        lpScrollRectangle,
                                        lpClipRectangle,
                                        dwDestinationOrigin,
                                        lpFill,
                                        TRUE);
}


/*
 * @implemented
 */
BOOL
WINAPI
SetConsoleWindowInfo(HANDLE hConsoleOutput,
                     BOOL bAbsolute,
                     CONST SMALL_RECT *lpConsoleWindow)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETWINDOWINFO SetWindowInfoRequest = &ApiMessage.Data.SetWindowInfoRequest;

    if (lpConsoleWindow == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SetWindowInfoRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetWindowInfoRequest->OutputHandle  = hConsoleOutput;
    SetWindowInfoRequest->Absolute      = bAbsolute;
    SetWindowInfoRequest->WindowRect    = *lpConsoleWindow;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetWindowInfo),
                        sizeof(*SetWindowInfoRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleTextAttribute(HANDLE hConsoleOutput,
                        WORD wAttributes)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETTEXTATTRIB SetTextAttribRequest = &ApiMessage.Data.SetTextAttribRequest;

    SetTextAttribRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetTextAttribRequest->OutputHandle  = hConsoleOutput;
    SetTextAttribRequest->Attributes    = wAttributes;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetTextAttribute),
                        sizeof(*SetTextAttribRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


static
BOOL
AddConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine)
{
    PHANDLER_ROUTINE* NewCtrlHandlers = NULL;

    if (HandlerRoutine == NULL)
    {
        NtCurrentPeb()->ProcessParameters->ConsoleFlags = TRUE;
        return TRUE;
    }

    if (NrCtrlHandlers == NrAllocatedHandlers)
    {
        NewCtrlHandlers = RtlAllocateHeap(RtlGetProcessHeap(),
                                          0,
                                          (NrCtrlHandlers + 4) * sizeof(PHANDLER_ROUTINE));
        if (NewCtrlHandlers == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return FALSE;
        }

        memmove(NewCtrlHandlers, CtrlHandlers, sizeof(PHANDLER_ROUTINE) * NrCtrlHandlers);

        if (NrAllocatedHandlers > 1) RtlFreeHeap(RtlGetProcessHeap(), 0, CtrlHandlers);

        CtrlHandlers = NewCtrlHandlers;
        NrAllocatedHandlers += 4;
    }

    ASSERT(NrCtrlHandlers < NrAllocatedHandlers);

    CtrlHandlers[NrCtrlHandlers++] = HandlerRoutine;
    return TRUE;
}


static
BOOL
RemoveConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine)
{
    ULONG i;

    if (HandlerRoutine == NULL)
    {
        NtCurrentPeb()->ProcessParameters->ConsoleFlags = FALSE;
        return TRUE;
    }

    for (i = 0; i < NrCtrlHandlers; i++)
    {
        if (CtrlHandlers[i] == HandlerRoutine)
        {
            if (i < (NrCtrlHandlers - 1))
            {
                memmove(&CtrlHandlers[i],
                        &CtrlHandlers[i+1],
                        (NrCtrlHandlers - i + 1) * sizeof(PHANDLER_ROUTINE));
            }

            NrCtrlHandlers--;
            return TRUE;
        }
    }

    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine,
                      BOOL Add)
{
    BOOL Ret;

    RtlEnterCriticalSection(&ConsoleLock);

    if (Add)
        Ret = AddConsoleCtrlHandler(HandlerRoutine);
    else
        Ret = RemoveConsoleCtrlHandler(HandlerRoutine);

    RtlLeaveCriticalSection(&ConsoleLock);
    return Ret;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
GenerateConsoleCtrlEvent(DWORD dwCtrlEvent,
                         DWORD dwProcessGroupId)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GENERATECTRLEVENT GenerateCtrlEventRequest = &ApiMessage.Data.GenerateCtrlEventRequest;

    if (dwCtrlEvent != CTRL_C_EVENT && dwCtrlEvent != CTRL_BREAK_EVENT)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    GenerateCtrlEventRequest->ConsoleHandle  = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GenerateCtrlEventRequest->CtrlEvent      = dwCtrlEvent;
    GenerateCtrlEventRequest->ProcessGroupId = dwProcessGroupId;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGenerateCtrlEvent),
                        sizeof(*GenerateCtrlEventRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


static DWORD
IntGetConsoleTitle(LPVOID lpConsoleTitle, DWORD dwNumChars, BOOLEAN bUnicode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &ApiMessage.Data.TitleRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    if (dwNumChars == 0) return 0;

    TitleRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    TitleRequest->Length        = dwNumChars * (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
    TitleRequest->Unicode       = bUnicode;

    CaptureBuffer = CsrAllocateCaptureBuffer(1, TitleRequest->Length);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    CsrAllocateMessagePointer(CaptureBuffer,
                              TitleRequest->Length,
                              (PVOID*)&TitleRequest->Title);

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetTitle),
                        sizeof(*TitleRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
        BaseSetLastNTError(ApiMessage.Status);
        return 0;
    }

    dwNumChars = TitleRequest->Length / (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));

    if (dwNumChars > 0)
    {
        RtlCopyMemory(lpConsoleTitle, TitleRequest->Title, TitleRequest->Length);

        if (bUnicode)
            ((LPWSTR)lpConsoleTitle)[dwNumChars] = UNICODE_NULL;
        else
            ((LPSTR)lpConsoleTitle)[dwNumChars] = ANSI_NULL;
    }

    CsrFreeCaptureBuffer(CaptureBuffer);

    return dwNumChars;
}


/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleTitleW(LPWSTR lpConsoleTitle,
                 DWORD nSize)
{
    return IntGetConsoleTitle(lpConsoleTitle, nSize, TRUE);
}


/*
 * @implemented
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleTitleA(LPSTR lpConsoleTitle,
                 DWORD nSize)
{
    return IntGetConsoleTitle(lpConsoleTitle, nSize, FALSE);
}


static BOOL
IntSetConsoleTitle(CONST VOID *lpConsoleTitle, BOOLEAN bUnicode)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &ApiMessage.Data.TitleRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    ULONG NumChars = (ULONG)(lpConsoleTitle ? (bUnicode ? wcslen(lpConsoleTitle) : strlen(lpConsoleTitle)) : 0);

    TitleRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    TitleRequest->Length        = NumChars * (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
    TitleRequest->Unicode       = bUnicode;

    CaptureBuffer = CsrAllocateCaptureBuffer(1, TitleRequest->Length);
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrCaptureMessageBuffer(CaptureBuffer,
                            (PVOID)lpConsoleTitle,
                            TitleRequest->Length,
                            (PVOID*)&TitleRequest->Title);

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetTitle),
                        sizeof(*TitleRequest));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}

/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleTitleW(LPCWSTR lpConsoleTitle)
{
    return IntSetConsoleTitle(lpConsoleTitle, TRUE);
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleTitleA(LPCSTR lpConsoleTitle)
{
    return IntSetConsoleTitle(lpConsoleTitle, FALSE);
}


/*
 * @implemented
 */
HANDLE
WINAPI
CreateConsoleScreenBuffer(DWORD dwDesiredAccess,
                          DWORD dwShareMode,
                          CONST SECURITY_ATTRIBUTES *lpSecurityAttributes,
                          DWORD dwFlags,
                          LPVOID lpScreenBufferData)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_CREATESCREENBUFFER CreateScreenBufferRequest = &ApiMessage.Data.CreateScreenBufferRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer = NULL;
    PCONSOLE_GRAPHICS_BUFFER_INFO GraphicsBufferInfo = lpScreenBufferData;

    if ( (dwDesiredAccess & ~(GENERIC_READ | GENERIC_WRITE))    ||
         (dwShareMode & ~(FILE_SHARE_READ | FILE_SHARE_WRITE))  ||
         (dwFlags != CONSOLE_TEXTMODE_BUFFER && dwFlags != CONSOLE_GRAPHICS_BUFFER) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    CreateScreenBufferRequest->ConsoleHandle    = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    CreateScreenBufferRequest->DesiredAccess    = dwDesiredAccess;
    CreateScreenBufferRequest->InheritHandle    =
        (lpSecurityAttributes ? lpSecurityAttributes->bInheritHandle : FALSE);
    CreateScreenBufferRequest->ShareMode        = dwShareMode;
    CreateScreenBufferRequest->ScreenBufferType = dwFlags;

    if (dwFlags == CONSOLE_GRAPHICS_BUFFER)
    {
        if (CreateScreenBufferRequest->InheritHandle || GraphicsBufferInfo == NULL)
        {
            SetLastError(ERROR_INVALID_PARAMETER);
            return INVALID_HANDLE_VALUE;
        }

        CreateScreenBufferRequest->GraphicsBufferInfo = *GraphicsBufferInfo;

        CaptureBuffer = CsrAllocateCaptureBuffer(1, GraphicsBufferInfo->dwBitMapInfoLength);
        if (CaptureBuffer == NULL)
        {
            SetLastError(ERROR_NOT_ENOUGH_MEMORY);
            return INVALID_HANDLE_VALUE;
        }

        CsrCaptureMessageBuffer(CaptureBuffer,
                                (PVOID)GraphicsBufferInfo->lpBitMapInfo,
                                GraphicsBufferInfo->dwBitMapInfoLength,
                                (PVOID*)&CreateScreenBufferRequest->GraphicsBufferInfo.lpBitMapInfo);
    }

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepCreateScreenBuffer),
                        sizeof(*CreateScreenBufferRequest));

    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return INVALID_HANDLE_VALUE;
    }

    if (dwFlags == CONSOLE_GRAPHICS_BUFFER && GraphicsBufferInfo)
    {
        GraphicsBufferInfo->hMutex   = CreateScreenBufferRequest->hMutex  ; // CreateScreenBufferRequest->GraphicsBufferInfo.hMutex  ;
        GraphicsBufferInfo->lpBitMap = CreateScreenBufferRequest->lpBitMap; // CreateScreenBufferRequest->GraphicsBufferInfo.lpBitMap;
    }

    return CreateScreenBufferRequest->OutputHandle;
}


/*
 * @implemented
 */
UINT
WINAPI
DECLSPEC_HOTPATCH
GetConsoleCP(VOID)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETINPUTOUTPUTCP GetConsoleCPRequest = &ApiMessage.Data.GetConsoleCPRequest;

    /* Get the Input Code Page */
    GetConsoleCPRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetConsoleCPRequest->OutputCP      = FALSE;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetCP),
                        sizeof(*GetConsoleCPRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return 0;
    }

    return GetConsoleCPRequest->CodePage;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleCP(UINT wCodePageID)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETINPUTOUTPUTCP SetConsoleCPRequest = &ApiMessage.Data.SetConsoleCPRequest;

    /* Set the Input Code Page */
    SetConsoleCPRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetConsoleCPRequest->CodePage      = wCodePageID;
    SetConsoleCPRequest->OutputCP      = FALSE;
    /* SetConsoleCPRequest->EventHandle; */

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetCP),
                        sizeof(*SetConsoleCPRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
UINT
WINAPI
DECLSPEC_HOTPATCH
GetConsoleOutputCP(VOID)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETINPUTOUTPUTCP GetConsoleCPRequest = &ApiMessage.Data.GetConsoleCPRequest;

    /* Get the Output Code Page */
    GetConsoleCPRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetConsoleCPRequest->OutputCP      = TRUE;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetCP),
                        sizeof(*GetConsoleCPRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return 0;
    }

    return GetConsoleCPRequest->CodePage;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleOutputCP(UINT wCodePageID)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETINPUTOUTPUTCP SetConsoleCPRequest = &ApiMessage.Data.SetConsoleCPRequest;

    /* Set the Output Code Page */
    SetConsoleCPRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetConsoleCPRequest->CodePage      = wCodePageID;
    SetConsoleCPRequest->OutputCP      = TRUE;
    /* SetConsoleCPRequest->EventHandle; */

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetCP),
                        sizeof(*SetConsoleCPRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @implemented
 */
DWORD
WINAPI
GetConsoleProcessList(LPDWORD lpdwProcessList,
                      DWORD dwProcessCount)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETPROCESSLIST GetProcessListRequest = &ApiMessage.Data.GetProcessListRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG nProcesses = 0;

    if (lpdwProcessList == NULL || dwProcessCount == 0)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return 0;
    }

    CaptureBuffer = CsrAllocateCaptureBuffer(1, dwProcessCount * sizeof(DWORD));
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return 0;
    }

    GetProcessListRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetProcessListRequest->ProcessCount  = dwProcessCount;

    CsrAllocateMessagePointer(CaptureBuffer,
                              dwProcessCount * sizeof(DWORD),
                              (PVOID*)&GetProcessListRequest->ProcessIdsList);

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetProcessList),
                        sizeof(*GetProcessListRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
    }
    else
    {
        nProcesses = GetProcessListRequest->ProcessCount;
        if (dwProcessCount >= nProcesses)
        {
            RtlCopyMemory(lpdwProcessList, GetProcessListRequest->ProcessIdsList, nProcesses * sizeof(DWORD));
        }
    }

    CsrFreeCaptureBuffer(CaptureBuffer);
    return nProcesses;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetConsoleSelectionInfo(PCONSOLE_SELECTION_INFO lpConsoleSelectionInfo)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSELECTIONINFO GetSelectionInfoRequest = &ApiMessage.Data.GetSelectionInfoRequest;

    if (lpConsoleSelectionInfo == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    GetSelectionInfoRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetSelectionInfo),
                        sizeof(*GetSelectionInfoRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    *lpConsoleSelectionInfo = GetSelectionInfoRequest->Info;

    return TRUE;
}


/*
 * @implemented
 * @note Strongly inspired by AllocConsole.
 */
static BOOL
IntAttachConsole(DWORD ProcessId,
                 LPTHREAD_START_ROUTINE CtrlRoutine,
                 LPTHREAD_START_ROUTINE PropRoutine,
                 PCONSOLE_START_INFO ConsoleStartInfo)
{
    BOOL Success = TRUE;
    NTSTATUS Status;

    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_ATTACHCONSOLE AttachConsoleRequest = &ApiMessage.Data.AttachConsoleRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    AttachConsoleRequest->ProcessId   = ProcessId;
    AttachConsoleRequest->CtrlRoutine = CtrlRoutine;
    AttachConsoleRequest->PropRoutine = PropRoutine;

    CaptureBuffer = CsrAllocateCaptureBuffer(1, sizeof(CONSOLE_START_INFO));
    if (CaptureBuffer == NULL)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        Success = FALSE;
        goto Quit;
    }

    CsrCaptureMessageBuffer(CaptureBuffer,
                            ConsoleStartInfo,
                            sizeof(CONSOLE_START_INFO),
                            (PVOID*)&AttachConsoleRequest->ConsoleStartInfo);

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        CaptureBuffer,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepAttach),
                        sizeof(*AttachConsoleRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        Success = FALSE;
        goto Quit;
    }

    // Is AttachConsoleRequest->ConsoleStartInfo->InitEvents aligned on handle boundary ????
    Status = NtWaitForMultipleObjects(MAX_INIT_EVENTS,
                                      AttachConsoleRequest->ConsoleStartInfo->InitEvents,
                                      WaitAny, FALSE, NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        Success = FALSE;
        goto Quit;
    }

    NtClose(AttachConsoleRequest->ConsoleStartInfo->InitEvents[INIT_SUCCESS]);
    NtClose(AttachConsoleRequest->ConsoleStartInfo->InitEvents[INIT_FAILURE]);
    if (Status != INIT_SUCCESS)
    {
        NtCurrentPeb()->ProcessParameters->ConsoleHandle = NULL;
        Success = FALSE;
    }
    else
    {
        RtlCopyMemory(ConsoleStartInfo,
                      AttachConsoleRequest->ConsoleStartInfo,
                      sizeof(CONSOLE_START_INFO));
        Success = TRUE;
    }

Quit:
    if (CaptureBuffer) CsrFreeCaptureBuffer(CaptureBuffer);
    return Success;
}

BOOL
WINAPI
AttachConsole(DWORD dwProcessId)
{
    BOOL Success;
    CONSOLE_START_INFO ConsoleStartInfo;

    DWORD dummy;
    LCID lcid;

    RtlEnterCriticalSection(&ConsoleLock);

    if (NtCurrentPeb()->ProcessParameters->ConsoleHandle)
    {
        DPRINT1("AttachConsole: Attaching a console to a process already having one\n");
        SetLastError(ERROR_ACCESS_DENIED);
        Success = FALSE;
        goto Quit;
    }

    /* Set up the console properties */
    SetUpConsoleInfo(FALSE,
                     &dummy,
                     NULL,
                     &dummy,
                     NULL,
                     &ConsoleStartInfo);

    Success = IntAttachConsole(dwProcessId,
                               ConsoleControlDispatcher,
                               PropDialogHandler,
                               &ConsoleStartInfo);
    if (Success)
    {
        /* Set up the handles */
        SetUpHandles(&ConsoleStartInfo);
        InputWaitHandle = ConsoleStartInfo.InputWaitHandle;

        /* Initialize Console Ctrl Handling */
        InitializeCtrlHandling();

        /* Sets the current console locale for this thread */
        SetTEBLangID(lcid);
    }

Quit:
    RtlLeaveCriticalSection(&ConsoleLock);
    return Success;
}


/*
 * @implemented
 */
HWND
WINAPI
DECLSPEC_HOTPATCH
GetConsoleWindow(VOID)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETWINDOW GetWindowRequest = &ApiMessage.Data.GetWindowRequest;

    GetWindowRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetConsoleWindow),
                        sizeof(*GetWindowRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return (HWND)NULL;
    }

    return GetWindowRequest->WindowHandle;
}


/*
 * @implemented
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleIcon(HICON hIcon)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETICON SetIconRequest = &ApiMessage.Data.SetIconRequest;

    SetIconRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    SetIconRequest->IconHandle    = hIcon;

    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetIcon),
                        sizeof(*SetIconRequest));
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    return TRUE;
}


/******************************************************************************
 * \name SetConsoleInputExeNameW
 * \brief Sets the console input file name from a unicode string.
 * \param lpExeName Pointer to a unicode string with the name.
 * \return TRUE if successful, FALSE if unsuccessful.
 * \remarks If lpExeName is 0 or the string length is 0 or greater than 255,
 *          the function fails and sets last error to ERROR_INVALID_PARAMETER.
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleInputExeNameW(IN LPCWSTR lpExeName)
{
    DWORD ExeLength;

    ExeLength = lstrlenW(lpExeName);
    if ((ExeLength == 0) || (ExeLength >= EXENAME_LENGTH))
    {
        /* Fail if string is empty or too long */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlEnterCriticalSection(&ExeNameLock);
    _SEH2_TRY
    {
        /* Set the input EXE name, not NULL terminated */
        RtlCopyMemory(ExeNameBuffer, lpExeName, ExeLength * sizeof(WCHAR));
        ExeNameLength = (USHORT)ExeLength;
    }
    _SEH2_FINALLY
    {
        RtlLeaveCriticalSection(&ExeNameLock);
    }
    _SEH2_END;

    return TRUE;
}


/******************************************************************************
 * \name SetConsoleInputExeNameA
 * \brief Sets the console input file name from an ansi string.
 * \param lpExeName Pointer to an ansi string with the name.
 * \return TRUE if successful, FALSE if unsuccessful.
 * \remarks If lpExeName is 0 or the string length is 0 or greater than 255,
 *          the function fails and sets last error to ERROR_INVALID_PARAMETER.
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleInputExeNameA(IN LPCSTR lpExeName)
{
    NTSTATUS Status;
#ifdef USE_TEB_STATIC_USTR
    PUNICODE_STRING ExeNameU;
#else
    UNICODE_STRING ExeNameU;
#endif
    ANSI_STRING ExeNameA;
#ifndef USE_TEB_STATIC_USTR
    WCHAR Buffer[EXENAME_LENGTH];
#endif

#ifdef USE_TEB_STATIC_USTR
    /*
     * Use the TEB static UNICODE string for storage. It is already
     * initialized at process creation time by the Memory Manager.
     */
    ExeNameU = &NtCurrentTeb()->StaticUnicodeString;
#endif

    /* Initialize string for conversion */
    RtlInitAnsiString(&ExeNameA, lpExeName);

#if 1
    if ((ExeNameA.Length == 0) || (ExeNameA.Length >= EXENAME_LENGTH))
    {
        /* Fail if string is empty or too long */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }
#endif
#ifndef USE_TEB_STATIC_USTR
    ExeNameU.Length = 0;
    ExeNameU.MaximumLength = (USHORT)sizeof(Buffer);
    ExeNameU.Buffer = Buffer;
#endif

#ifdef USE_TEB_STATIC_USTR
    Status = RtlAnsiStringToUnicodeString(ExeNameU, &ExeNameA, FALSE);
#else
    Status = RtlAnsiStringToUnicodeString(&ExeNameU, &ExeNameA, FALSE);
#endif
    if (!NT_SUCCESS(Status))
    {
        /* Fail if string is empty or too long */
        if (Status == STATUS_BUFFER_OVERFLOW)
            SetLastError(ERROR_FILENAME_EXCED_RANGE);
        else
            SetLastError(ERROR_INVALID_PARAMETER);

        return FALSE;
    }

#ifdef USE_TEB_STATIC_USTR
    return SetConsoleInputExeNameW(ExeNameU->Buffer);
#else
    return SetConsoleInputExeNameW(ExeNameU.Buffer);
#endif
}


/******************************************************************************
 * \name GetConsoleInputExeNameW
 * \brief Retrieves the console input file name as unicode string.
 * \param nBufferLength Length of the buffer in WCHARs.
 *        Specify 0 to receive the needed buffer length.
 * \param lpBuffer Pointer to a buffer that receives the string.
 * \return Needed buffer size if \p nBufferLength is 0.
 *         Otherwise 1 if successful, 2 if buffer is too small.
 * \remarks Sets last error value to ERROR_BUFFER_OVERFLOW if the buffer
 *          is not big enough.
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleInputExeNameW(IN DWORD nBufferLength,
                        OUT LPWSTR lpExeName)
{
    if (nBufferLength <= ExeNameLength)
    {
        /* Buffer is not large enough! Return the correct size. */
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return ExeNameLength + 1;
    }

    RtlEnterCriticalSection(&ExeNameLock);
    _SEH2_TRY
    {
        /* Copy the input EXE name and NULL-terminate it */
        RtlCopyMemory(lpExeName, ExeNameBuffer, ExeNameLength * sizeof(WCHAR));
        lpExeName[ExeNameLength] = UNICODE_NULL;
    }
    _SEH2_FINALLY
    {
        RtlLeaveCriticalSection(&ExeNameLock);
    }
    _SEH2_END;

    return TRUE;
}


/******************************************************************************
 * \name GetConsoleInputExeNameA
 * \brief Retrieves the console input file name as ansi string.
 * \param nBufferLength Length of the buffer in CHARs.
 * \param lpBuffer Pointer to a buffer that receives the string.
 * \return 1 if successful, 2 if buffer is too small.
 * \remarks Sets last error value to ERROR_BUFFER_OVERFLOW if the buffer
 *          is not big enough. The buffer receives as much characters as fit.
 */
DWORD
WINAPI
DECLSPEC_HOTPATCH
GetConsoleInputExeNameA(IN DWORD nBufferLength,
                        OUT LPSTR lpExeName)
{
    NTSTATUS Status;
    DWORD ExeLength;
    UNICODE_STRING BufferU;
    ANSI_STRING BufferA;
    WCHAR Buffer[EXENAME_LENGTH];

    /* Get the UNICODE name */
    ExeLength = GetConsoleInputExeNameW(EXENAME_LENGTH, Buffer);

    if ((ExeLength == 0) || (ExeLength >= EXENAME_LENGTH))
        return ExeLength;

    /* Initialize the strings for conversion */
    RtlInitUnicodeString(&BufferU, Buffer);
    BufferA.Length = 0;
    BufferA.MaximumLength = (USHORT)nBufferLength;
    BufferA.Buffer = lpExeName;

    /* Convert UNICODE name to ANSI, copying as much chars as it can fit */
    Status = RtlUnicodeStringToAnsiString(&BufferA, &BufferU, FALSE);
    if (!NT_SUCCESS(Status))
    {
        if (Status == STATUS_BUFFER_OVERFLOW)
        {
            SetLastError(ERROR_BUFFER_OVERFLOW);
            return ExeLength + 1;
        }
        SetLastError(ERROR_INVALID_PARAMETER);
    }

    return ExeLength;
}

BOOL
WINAPI
GetConsoleCharType(HANDLE hConsole, COORD Coord, PDWORD Type)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
DECLSPEC_HOTPATCH
GetConsoleCursorMode(HANDLE hConsole, PBOOL pUnknown1, PBOOL pUnknown2)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleCursorMode(HANDLE hConsole, BOOL Unknown1, BOOL Unknown2)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
DECLSPEC_HOTPATCH
GetConsoleNlsMode(HANDLE hConsole, LPDWORD lpMode)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleNlsMode(HANDLE hConsole, DWORD dwMode)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleLocalEUDC(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
DECLSPEC_HOTPATCH
RegisterConsoleIME(HWND hWnd, LPDWORD ThreadId)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
DECLSPEC_HOTPATCH
RegisterConsoleOS2(BOOL bUnknown)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
DECLSPEC_HOTPATCH
SetConsoleOS2OemFormat(BOOL bUnknown)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
DECLSPEC_HOTPATCH
UnregisterConsoleIME(VOID)
{
    STUB;
    return FALSE;
}


static
BOOL
IntGetConsoleKeyboardLayoutName(OUT PVOID pszLayoutName,
                                IN BOOL bAnsi)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETKBDLAYOUTNAME GetKbdLayoutNameRequest = &ApiMessage.Data.GetKbdLayoutNameRequest;

    /* Set up the data to send to the Console Server */
    GetKbdLayoutNameRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;
    GetKbdLayoutNameRequest->Ansi          = bAnsi;

    /* Call the server */
    CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                        NULL,
                        CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetKeyboardLayoutName),
                        sizeof(*GetKbdLayoutNameRequest));

    /* Check for success */
    if (!NT_SUCCESS(ApiMessage.Status))
    {
        BaseSetLastNTError(ApiMessage.Status);
        return FALSE;
    }

    /* Retrieve the results */
    _SEH2_TRY
    {
        /* Copy only KL_NAMELENGTH == 9 characters, ANSI or UNICODE */
        if (bAnsi)
            strncpy(pszLayoutName, (PCHAR)GetKbdLayoutNameRequest->LayoutBuffer, KL_NAMELENGTH);
        else
            wcsncpy(pszLayoutName, (PWCHAR)GetKbdLayoutNameRequest->LayoutBuffer, KL_NAMELENGTH);
    }
    _SEH2_EXCEPT(EXCEPTION_EXECUTE_HANDLER)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        _SEH2_YIELD(return FALSE);
    }
    _SEH2_END;

    return TRUE;
}

/*
 * @implemented (undocumented)
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
GetConsoleKeyboardLayoutNameA(OUT LPSTR pszLayoutName)
{
    return IntGetConsoleKeyboardLayoutName(pszLayoutName, TRUE);
}

/*
 * @implemented (undocumented)
 */
BOOL
WINAPI
DECLSPEC_HOTPATCH
GetConsoleKeyboardLayoutNameW(OUT LPWSTR pszLayoutName)
{
    return IntGetConsoleKeyboardLayoutName(pszLayoutName, FALSE);
}

/*
 * @implemented
 */
DWORD
WINAPI
SetLastConsoleEventActive(VOID)
{
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_NOTIFYLASTCLOSE NotifyLastCloseRequest = &ApiMessage.Data.NotifyLastCloseRequest;

    /* Set the flag used by the console control dispatcher */
    LastCloseNotify = TRUE;

    /* Set up the input arguments */
    NotifyLastCloseRequest->ConsoleHandle = NtCurrentPeb()->ProcessParameters->ConsoleHandle;

    /* Call CSRSS; just return the NTSTATUS cast to DWORD */
    return CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                               NULL,
                               CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepNotifyLastClose),
                               sizeof(*NotifyLastCloseRequest));
}

/* EOF */
