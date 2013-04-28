/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/console/console.c
 * PURPOSE:         Win32 server console functions
 * PROGRAMMERS:     James Tabor
 *                  <jimtabor@adsl-64-217-116-74.dsl.hstntx.swbell.net>
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

extern RTL_CRITICAL_SECTION ConsoleLock;
extern BOOL ConsoleInitialized;
extern BOOL WINAPI IsDebuggerPresent(VOID);

/* Console reserved "file" names */
static LPCWSTR BaseConFileName       = CONSOLE_FILE_NAME;
static LPCWSTR BaseConInputFileName  = CONSOLE_INPUT_FILE_NAME;
static LPCWSTR BaseConOutputFileName = CONSOLE_OUTPUT_FILE_NAME;

PHANDLER_ROUTINE InitialHandler[1];
PHANDLER_ROUTINE* CtrlHandlers;
ULONG NrCtrlHandlers;
ULONG NrAllocatedHandlers;

HANDLE InputWaitHandle = INVALID_HANDLE_VALUE;

#define INPUTEXENAME_BUFLEN 256
static WCHAR InputExeName[INPUTEXENAME_BUFLEN];


/* Default Console Control Handler ********************************************/

BOOL
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

        case 3:
            ExitThread(0);
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
                    case CTRL_LOGOFF_EVENT:
                    case CTRL_SHUTDOWN_EVENT:
                    case 3:
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
WINAPI
InitConsoleCtrlHandling(VOID)
{
    /* Initialize Console Ctrl Handler */
    NrAllocatedHandlers = NrCtrlHandlers = 1;
    CtrlHandlers = InitialHandler;
    CtrlHandlers[0] = DefaultConsoleCtrlHandler;
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
 * @unimplemented (Undocumented)
 */
BOOL
WINAPI
ConsoleMenuControl(HANDLE hConsole,
                   DWORD Unknown1,
                   DWORD Unknown2)
{
    DPRINT1("ConsoleMenuControl(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", hConsole, Unknown1, Unknown2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @implemented
 */
HANDLE
WINAPI
DuplicateConsoleHandle(HANDLE hConsole,
                       DWORD dwDesiredAccess,
                       BOOL bInheritHandle,
                       DWORD dwOptions)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_DUPLICATEHANDLE DuplicateHandleRequest = &ApiMessage.Data.DuplicateHandleRequest;

    if ( (dwOptions & ~(DUPLICATE_CLOSE_SOURCE | DUPLICATE_SAME_ACCESS)) ||
         (!(dwOptions & DUPLICATE_SAME_ACCESS) &&
           (dwDesiredAccess & ~(GENERIC_READ | GENERIC_WRITE))) )
    {
        SetLastError (ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    DuplicateHandleRequest->ConsoleHandle = hConsole;
    DuplicateHandleRequest->Access = dwDesiredAccess;
    DuplicateHandleRequest->Inheritable = bInheritHandle;
    DuplicateHandleRequest->Options = dwOptions;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepDuplicateHandle),
                                 sizeof(CONSOLE_DUPLICATEHANDLE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
    }

    return DuplicateHandleRequest->ConsoleHandle;
}


/*
 * @implemented
 */
BOOL
WINAPI
GetConsoleDisplayMode(LPDWORD lpModeFlags)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETDISPLAYMODE GetDisplayModeRequest = &ApiMessage.Data.GetDisplayModeRequest;

    if (lpModeFlags == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    // GetDisplayModeRequest->OutputHandle = hConsoleOutput;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetDisplayMode),
                                 sizeof(CONSOLE_GETDISPLAYMODE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpModeFlags = GetDisplayModeRequest->DisplayMode;
    return TRUE;
}


/*
 * @unimplemented (Undocumented)
 */
DWORD
WINAPI
GetConsoleFontInfo(DWORD Unknown0,
                   DWORD Unknown1,
                   DWORD Unknown2,
                   DWORD Unknown3)
{
    DPRINT1("GetConsoleFontInfo(0x%x, 0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2, Unknown3);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
COORD
WINAPI
GetConsoleFontSize(HANDLE hConsoleOutput,
                   DWORD nFont)
{
    COORD Empty = {0, 0};
    DPRINT1("GetConsoleFontSize(0x%x, 0x%x) UNIMPLEMENTED!\n", hConsoleOutput, nFont);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return Empty;
}


/*
 * @implemented (Undocumented)
 */
BOOL
WINAPI
GetConsoleHardwareState(HANDLE hConsoleOutput,
                        DWORD Flags,
                        PDWORD State)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETHWSTATE HardwareStateRequest = &ApiMessage.Data.HardwareStateRequest;

    DPRINT1("GetConsoleHardwareState(%d, 0x%p) UNIMPLEMENTED!\n", Flags, State);

    if (State == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    HardwareStateRequest->OutputHandle = hConsoleOutput;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetHardwareState),
                                 sizeof(CONSOLE_GETSETHWSTATE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

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
 * @unimplemented
 */
INT
WINAPI
GetCurrentConsoleFont(HANDLE hConsoleOutput,
                      BOOL bMaximumWindow,
                      PCONSOLE_FONT_INFO lpConsoleCurrentFont)
{
    DPRINT1("GetCurrentConsoleFont(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", hConsoleOutput, bMaximumWindow, lpConsoleCurrentFont);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented (Undocumented)
 */
ULONG
WINAPI
GetNumberOfConsoleFonts(VOID)
{
    DPRINT1("GetNumberOfConsoleFonts() UNIMPLEMENTED!\n");
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 1;
}


/*
 * @unimplemented (Undocumented)
 */
DWORD
WINAPI
InvalidateConsoleDIBits(DWORD Unknown0,
                        DWORD Unknown1)
{
    DPRINT1("InvalidateConsoleDIBits(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented (Undocumented)
 */
HANDLE
WINAPI
OpenConsoleW(LPCWSTR wsName,
             DWORD dwDesiredAccess,
             BOOL bInheritHandle,
             DWORD dwShareMode)
{
    NTSTATUS Status = STATUS_SUCCESS;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_OPENCONSOLE OpenConsoleRequest = &ApiMessage.Data.OpenConsoleRequest;
    CONSOLE_HANDLE_TYPE HandleType;

    if (wsName && 0 == _wcsicmp(wsName, BaseConInputFileName))
    {
        HandleType = HANDLE_INPUT;
    }
    else if (wsName && 0 == _wcsicmp(wsName, BaseConOutputFileName))
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

    OpenConsoleRequest->HandleType = HandleType;
    OpenConsoleRequest->Access = dwDesiredAccess;
    OpenConsoleRequest->Inheritable = bInheritHandle;
    OpenConsoleRequest->ShareMode = dwShareMode;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepOpenConsole),
                                 sizeof(CONSOLE_OPENCONSOLE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
    }

    return OpenConsoleRequest->ConsoleHandle;
}


/*
 * @unimplemented (Undocumented)
 */
BOOL
WINAPI
SetConsoleCursor(DWORD Unknown0,
                 DWORD Unknown1)
{
    DPRINT1("SetConsoleCursor(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetConsoleDisplayMode(HANDLE hConsoleOutput,
                      DWORD dwFlags,
                      PCOORD lpNewScreenBufferDimensions)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETDISPLAYMODE SetDisplayModeRequest = &ApiMessage.Data.SetDisplayModeRequest;

    SetDisplayModeRequest->OutputHandle = hConsoleOutput;
    SetDisplayModeRequest->DisplayMode  = dwFlags;
    SetDisplayModeRequest->NewSBDim.X   = 0;
    SetDisplayModeRequest->NewSBDim.Y   = 0;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetDisplayMode),
                                 sizeof(CONSOLE_SETDISPLAYMODE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (lpNewScreenBufferDimensions)
        *lpNewScreenBufferDimensions = SetDisplayModeRequest->NewSBDim;

    return TRUE;
}


/*
 * @unimplemented (Undocumented)
 */
BOOL
WINAPI
SetConsoleFont(DWORD Unknown0,
               DWORD Unknown1)
{
    DPRINT1("SetConsoleFont(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
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
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETHWSTATE HardwareStateRequest = &ApiMessage.Data.HardwareStateRequest;

    DPRINT1("SetConsoleHardwareState(%d, %d) UNIMPLEMENTED!\n", Flags, State);

    HardwareStateRequest->OutputHandle = hConsoleOutput;
    HardwareStateRequest->State = State;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetHardwareState),
                                 sizeof(CONSOLE_GETSETHWSTATE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @unimplemented (Undocumented)
 */
BOOL
WINAPI
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
 * @unimplemented (Undocumented)
 */
BOOL
WINAPI
SetConsoleMaximumWindowSize(DWORD Unknown0,
                            DWORD Unknown1)
{
    DPRINT1("SetConsoleMaximumWindowSize(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented (Undocumented)
 */
BOOL
WINAPI
SetConsoleMenuClose(DWORD Unknown0)
{
    DPRINT1("SetConsoleMenuClose(0x%x) UNIMPLEMENTED!\n", Unknown0);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*
 * @unimplemented (Undocumented)
 */
BOOL
WINAPI
SetConsolePalette(DWORD Unknown0,
                  DWORD Unknown1,
                  DWORD Unknown2)
{
    DPRINT1("SetConsolePalette(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}

/*
 * @unimplemented (Undocumented)
 */
DWORD
WINAPI
ShowConsoleCursor(DWORD Unknown0,
                  DWORD Unknown1)
{
    DPRINT1("ShowConsoleCursor(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * FUNCTION: Checks whether the given handle is a valid console handle.
 *
 * ARGUMENTS:
 *      Handle - Handle to be checked
 *
 * RETURNS:
 *      TRUE: Handle is a valid console handle
 *      FALSE: Handle is not a valid console handle.
 *
 * STATUS: Officially undocumented
 *
 * @implemented
 */
BOOL
WINAPI
VerifyConsoleIoHandle(HANDLE Handle)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    ApiMessage.Data.VerifyHandleRequest.ConsoleHandle = Handle;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepVerifyIoHandle),
                                 sizeof(CONSOLE_VERIFYHANDLE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
WriteConsoleInputVDMA(DWORD Unknown0,
                      DWORD Unknown1,
                      DWORD Unknown2,
                      DWORD Unknown3)
{
    DPRINT1("WriteConsoleInputVDMA(0x%x, 0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2, Unknown3);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @unimplemented
 */
DWORD
WINAPI
WriteConsoleInputVDMW(DWORD Unknown0,
                      DWORD Unknown1,
                      DWORD Unknown2,
                      DWORD Unknown3)
{
    DPRINT1("WriteConsoleInputVDMW(0x%x, 0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2, Unknown3);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return 0;
}


/*
 * @implemented (Undocumented)
 */
BOOL
WINAPI
CloseConsoleHandle(HANDLE Handle)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    ApiMessage.Data.CloseHandleRequest.ConsoleHandle = Handle;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepCloseHandle),
                                 sizeof(CONSOLE_CLOSEHANDLE));
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
HANDLE
WINAPI
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

    switch (nStdHandle)
    {
        case STD_INPUT_HANDLE:
            return Ppb->StandardInput;

        case STD_OUTPUT_HANDLE:
            return Ppb->StandardOutput;

        case STD_ERROR_HANDLE:
            return Ppb->StandardError;
    }

    SetLastError(ERROR_INVALID_HANDLE);
    return INVALID_HANDLE_VALUE;
}


/*
 * @implemented
 */
BOOL
WINAPI
SetStdHandle(DWORD nStdHandle,
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

    /* no need to check if hHandle == INVALID_HANDLE_VALUE */

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

    /* Windows for whatever reason sets the last error to ERROR_INVALID_HANDLE here */
    SetLastError(ERROR_INVALID_HANDLE);
    return FALSE;
}


/*--------------------------------------------------------------
 *    AllocConsole
 *
 * @implemented
 */
BOOL
WINAPI
AllocConsole(VOID)
{
    NTSTATUS Status;
    PRTL_USER_PROCESS_PARAMETERS Parameters = NtCurrentPeb()->ProcessParameters;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_ALLOCCONSOLE AllocConsoleRequest = &ApiMessage.Data.AllocConsoleRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    LPWSTR AppPath = NULL;
    SIZE_T Length = 0;

    if (Parameters->ConsoleHandle)
    {
        DPRINT1("AllocConsole: Allocating a console to a process already having one\n");
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    CaptureBuffer = CsrAllocateCaptureBuffer(1, sizeof(CONSOLE_START_INFO));
    if (CaptureBuffer == NULL)
    {
        DPRINT1("CsrAllocateCaptureBuffer failed!\n");
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    CsrAllocateMessagePointer(CaptureBuffer,
                              sizeof(CONSOLE_START_INFO),
                              (PVOID*)&AllocConsoleRequest->ConsoleStartInfo);

/** Copied from BasepInitConsole **********************************************/
    InitConsoleInfo(AllocConsoleRequest->ConsoleStartInfo);

    AppPath = AllocConsoleRequest->ConsoleStartInfo->AppPath;
    Length = min(MAX_PATH, Parameters->ImagePathName.Length / sizeof(WCHAR));
    wcsncpy(AppPath, Parameters->ImagePathName.Buffer, Length);
    AppPath[Length] = L'\0';
/******************************************************************************/

    AllocConsoleRequest->Console = NULL;
    AllocConsoleRequest->CtrlDispatcher = ConsoleControlDispatcher;
    AllocConsoleRequest->PropDispatcher = PropDialogHandler;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepAlloc),
                                 sizeof(CONSOLE_ALLOCCONSOLE));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    Parameters->ConsoleHandle = AllocConsoleRequest->Console;
    SetStdHandle(STD_INPUT_HANDLE , AllocConsoleRequest->InputHandle );
    SetStdHandle(STD_OUTPUT_HANDLE, AllocConsoleRequest->OutputHandle);
    SetStdHandle(STD_ERROR_HANDLE , AllocConsoleRequest->ErrorHandle );

    /* Initialize Console Ctrl Handler */
    InitConsoleCtrlHandling();

    InputWaitHandle = AllocConsoleRequest->InputWaitHandle;

    return TRUE;
}


/*--------------------------------------------------------------
 *    FreeConsole
 *
 * @implemented
 */
BOOL
WINAPI
FreeConsole(VOID)
{
    // AG: I'm not sure if this is correct (what happens to std handles?)
    // but I just tried to reverse what AllocConsole() does...

    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepFree),
                                 sizeof(CONSOLE_FREECONSOLE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    NtCurrentPeb()->ProcessParameters->ConsoleHandle = NULL;

    CloseHandle(InputWaitHandle);
    InputWaitHandle = INVALID_HANDLE_VALUE;

    return TRUE;
}


/*--------------------------------------------------------------
 *    GetConsoleScreenBufferInfo
 *
 * @implemented
 */
BOOL
WINAPI
GetConsoleScreenBufferInfo(HANDLE hConsoleOutput,
                           PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    if (lpConsoleScreenBufferInfo == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ApiMessage.Data.ScreenBufferInfoRequest.OutputHandle = hConsoleOutput;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetScreenBufferInfo),
                                 sizeof(CONSOLE_GETSCREENBUFFERINFO));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpConsoleScreenBufferInfo = ApiMessage.Data.ScreenBufferInfoRequest.Info;

    return TRUE;
}


/*--------------------------------------------------------------
 *    SetConsoleCursorPosition
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleCursorPosition(HANDLE hConsoleOutput,
                         COORD dwCursorPosition)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    ApiMessage.Data.SetCursorPositionRequest.OutputHandle = hConsoleOutput;
    ApiMessage.Data.SetCursorPositionRequest.Position = dwCursorPosition;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetCursorPosition),
                                 sizeof(CONSOLE_SETCURSORPOSITION));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*--------------------------------------------------------------
 *     GetConsoleMode
 *
 * @implemented
 */
BOOL
WINAPI
GetConsoleMode(HANDLE hConsoleHandle,
               LPDWORD lpMode)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &ApiMessage.Data.ConsoleModeRequest;

    if (lpMode == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ConsoleModeRequest->ConsoleHandle = hConsoleHandle;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetMode),
                                 sizeof(CONSOLE_GETSETCONSOLEMODE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpMode = ConsoleModeRequest->ConsoleMode;

    return TRUE;
}


/*--------------------------------------------------------------
 *     GetNumberOfConsoleInputEvents
 *
 * @implemented
 */
BOOL
WINAPI
GetNumberOfConsoleInputEvents(HANDLE hConsoleInput,
                              LPDWORD lpNumberOfEvents)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETNUMINPUTEVENTS GetNumInputEventsRequest = &ApiMessage.Data.GetNumInputEventsRequest;

    GetNumInputEventsRequest->InputHandle = hConsoleInput;
    GetNumInputEventsRequest->NumInputEvents = 0;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetNumberOfInputEvents),
                                 sizeof(CONSOLE_GETNUMINPUTEVENTS));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    if (lpNumberOfEvents == NULL)
    {
        SetLastError(ERROR_INVALID_ACCESS);
        return FALSE;
    }

    *lpNumberOfEvents = GetNumInputEventsRequest->NumInputEvents;

    return TRUE;
}


/*--------------------------------------------------------------
 *     GetLargestConsoleWindowSize
 *
 * @implemented
 */
COORD
WINAPI
GetLargestConsoleWindowSize(HANDLE hConsoleOutput)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETLARGESTWINDOWSIZE GetLargestWindowSizeRequest = &ApiMessage.Data.GetLargestWindowSizeRequest;

    GetLargestWindowSizeRequest->OutputHandle = hConsoleOutput;
    GetLargestWindowSizeRequest->Size.X = 0;
    GetLargestWindowSizeRequest->Size.Y = 0;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetLargestWindowSize),
                                 sizeof(CONSOLE_GETLARGESTWINDOWSIZE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
    }

    DPRINT1("GetLargestConsoleWindowSize, X = %d, Y = %d\n", GetLargestWindowSizeRequest->Size.X, GetLargestWindowSizeRequest->Size.Y);
    return GetLargestWindowSizeRequest->Size;
}


/*--------------------------------------------------------------
 *    GetConsoleCursorInfo
 *
 * @implemented
 */
BOOL
WINAPI
GetConsoleCursorInfo(HANDLE hConsoleOutput,
                     PCONSOLE_CURSOR_INFO lpConsoleCursorInfo)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    if (!lpConsoleCursorInfo)
    {
        if (!hConsoleOutput)
            SetLastError(ERROR_INVALID_HANDLE);
        else
            SetLastError(ERROR_INVALID_ACCESS);

        return FALSE;
    }

    ApiMessage.Data.CursorInfoRequest.OutputHandle = hConsoleOutput;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetCursorInfo),
                                 sizeof(CONSOLE_GETSETCURSORINFO));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpConsoleCursorInfo = ApiMessage.Data.CursorInfoRequest.Info;

    return TRUE;
}


/*--------------------------------------------------------------
 *     GetNumberOfConsoleMouseButtons
 *
 * @unimplemented
 */
BOOL
WINAPI
GetNumberOfConsoleMouseButtons(LPDWORD lpNumberOfMouseButtons)
{
    DPRINT1("GetNumberOfConsoleMouseButtons(0x%x) UNIMPLEMENTED!\n", lpNumberOfMouseButtons);
    SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
    return FALSE;
}


/*--------------------------------------------------------------
 *     SetConsoleMode
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleMode(HANDLE hConsoleHandle,
               DWORD dwMode)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETCONSOLEMODE ConsoleModeRequest = &ApiMessage.Data.ConsoleModeRequest;

    ConsoleModeRequest->ConsoleHandle = hConsoleHandle;
    ConsoleModeRequest->ConsoleMode   = dwMode;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetMode),
                                 sizeof(CONSOLE_GETSETCONSOLEMODE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*--------------------------------------------------------------
 *     SetConsoleActiveScreenBuffer
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleActiveScreenBuffer(HANDLE hConsoleOutput)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    ApiMessage.Data.SetScreenBufferRequest.OutputHandle = hConsoleOutput;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetActiveScreenBuffer),
                                 sizeof(CONSOLE_SETACTIVESCREENBUFFER));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*--------------------------------------------------------------
 *     FlushConsoleInputBuffer
 *
 * @implemented
 */
BOOL
WINAPI
FlushConsoleInputBuffer(HANDLE hConsoleInput)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    ApiMessage.Data.FlushInputBufferRequest.InputHandle = hConsoleInput;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepFlushInputBuffer),
                                 sizeof(CONSOLE_FLUSHINPUTBUFFER));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*--------------------------------------------------------------
 *     SetConsoleScreenBufferSize
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleScreenBufferSize(HANDLE hConsoleOutput,
                           COORD dwSize)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    ApiMessage.Data.SetScreenBufferSizeRequest.OutputHandle = hConsoleOutput;
    ApiMessage.Data.SetScreenBufferSizeRequest.Size = dwSize;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetScreenBufferSize),
                                 sizeof(CONSOLE_SETSCREENBUFFERSIZE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*--------------------------------------------------------------
 *     SetConsoleCursorInfo
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleCursorInfo(HANDLE hConsoleOutput,
                     CONST CONSOLE_CURSOR_INFO *lpConsoleCursorInfo)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    ApiMessage.Data.CursorInfoRequest.OutputHandle = hConsoleOutput;
    ApiMessage.Data.CursorInfoRequest.Info = *lpConsoleCursorInfo;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetCursorInfo),
                                 sizeof(CONSOLE_GETSETCURSORINFO));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


static
BOOL
IntScrollConsoleScreenBuffer(HANDLE hConsoleOutput,
                             const SMALL_RECT *lpScrollRectangle,
                             const SMALL_RECT *lpClipRectangle,
                             COORD dwDestinationOrigin,
                             const CHAR_INFO *lpFill,
                             BOOL bUnicode)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SCROLLSCREENBUFFER ScrollScreenBufferRequest = &ApiMessage.Data.ScrollScreenBufferRequest;

    ScrollScreenBufferRequest->OutputHandle = hConsoleOutput;
    ScrollScreenBufferRequest->Unicode = bUnicode;
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
    ScrollScreenBufferRequest->Fill = *lpFill;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepScrollScreenBuffer),
                                 sizeof(CONSOLE_SCROLLSCREENBUFFER));

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*--------------------------------------------------------------
 *    ScrollConsoleScreenBufferA
 *
 * @implemented
 */
BOOL
WINAPI
ScrollConsoleScreenBufferA(HANDLE hConsoleOutput,
                           CONST SMALL_RECT *lpScrollRectangle,
                           CONST SMALL_RECT *lpClipRectangle,
                           COORD dwDestinationOrigin,
                           CONST CHAR_INFO *lpFill)
{
    return IntScrollConsoleScreenBuffer(hConsoleOutput,
                                        (PSMALL_RECT)lpScrollRectangle,
                                        (PSMALL_RECT)lpClipRectangle,
                                        dwDestinationOrigin,
                                        (PCHAR_INFO)lpFill,
                                        FALSE);
}


/*--------------------------------------------------------------
 *     ScrollConsoleScreenBufferW
 *
 * @implemented
 */
BOOL
WINAPI
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


/*--------------------------------------------------------------
 *     SetConsoleWindowInfo
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleWindowInfo(HANDLE hConsoleOutput,
                     BOOL bAbsolute,
                     CONST SMALL_RECT *lpConsoleWindow)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_SETWINDOWINFO SetWindowInfoRequest = &ApiMessage.Data.SetWindowInfoRequest;

    if (lpConsoleWindow == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    SetWindowInfoRequest->OutputHandle = hConsoleOutput;
    SetWindowInfoRequest->Absolute = bAbsolute;
    SetWindowInfoRequest->WindowRect = *lpConsoleWindow;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetWindowInfo),
                                 sizeof(CONSOLE_SETWINDOWINFO));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*--------------------------------------------------------------
 *      SetConsoleTextAttribute
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleTextAttribute(HANDLE hConsoleOutput,
                        WORD wAttributes)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    ApiMessage.Data.SetTextAttribRequest.OutputHandle = hConsoleOutput;
    ApiMessage.Data.SetTextAttribRequest.Attrib = wAttributes;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetTextAttribute),
                                 sizeof(CONSOLE_SETTEXTATTRIB));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
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
SetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine,
                      BOOL Add)
{
    BOOL Ret;

    RtlEnterCriticalSection(&BaseDllDirectoryLock);
    if (Add)
    {
        Ret = AddConsoleCtrlHandler(HandlerRoutine);
    }
    else
    {
        Ret = RemoveConsoleCtrlHandler(HandlerRoutine);
    }

    RtlLeaveCriticalSection(&BaseDllDirectoryLock);
    return(Ret);
}


/*--------------------------------------------------------------
 *     GenerateConsoleCtrlEvent
 *
 * @implemented
 */
BOOL
WINAPI
GenerateConsoleCtrlEvent(DWORD dwCtrlEvent,
                         DWORD dwProcessGroupId)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    if (dwCtrlEvent != CTRL_C_EVENT && dwCtrlEvent != CTRL_BREAK_EVENT)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    ApiMessage.Data.GenerateCtrlEventRequest.Event = dwCtrlEvent;
    ApiMessage.Data.GenerateCtrlEventRequest.ProcessGroup = dwProcessGroupId;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGenerateCtrlEvent),
                                 sizeof(CONSOLE_GENERATECTRLEVENT));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


static DWORD
IntGetConsoleTitle(LPVOID lpConsoleTitle, DWORD nSize, BOOL bUnicode)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &ApiMessage.Data.TitleRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    if (nSize == 0) return 0;

    TitleRequest->Length = nSize * (bUnicode ? 1 : sizeof(WCHAR));
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

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetTitle),
                                 sizeof(CONSOLE_GETSETCONSOLETITLE));
    if (!NT_SUCCESS(Status))
    {
        CsrFreeCaptureBuffer(CaptureBuffer);
        BaseSetLastNTError(Status);
        return 0;
    }

    if (bUnicode)
    {
        if (nSize >= sizeof(WCHAR))
            wcscpy((LPWSTR)lpConsoleTitle, TitleRequest->Title);
    }
    else
    {
        if (nSize < TitleRequest->Length / sizeof(WCHAR) ||
            !WideCharToMultiByte(CP_ACP, // ANSI code page
                                 0, // performance and mapping flags
                                 TitleRequest->Title, // address of wide-character string
                                 -1, // number of characters in string
                                 (LPSTR)lpConsoleTitle, // address of buffer for new string
                                 nSize, // size of buffer
                                 NULL, // FAST
                                 NULL))
        {
            /* Yes, if the buffer isn't big enough, it returns 0... Bad API */
            *(LPSTR)lpConsoleTitle = '\0';
            TitleRequest->Length = 0;
        }
    }
    CsrFreeCaptureBuffer(CaptureBuffer);

    return TitleRequest->Length / sizeof(WCHAR);
}


/*--------------------------------------------------------------
 *    GetConsoleTitleW
 *
 * @implemented
 */
DWORD
WINAPI
GetConsoleTitleW(LPWSTR lpConsoleTitle,
                 DWORD nSize)
{
    return IntGetConsoleTitle(lpConsoleTitle, nSize, TRUE);
}


/*--------------------------------------------------------------
 *     GetConsoleTitleA
 *
 * @implemented
 */
DWORD
WINAPI
GetConsoleTitleA(LPSTR lpConsoleTitle,
                 DWORD nSize)
{
    return IntGetConsoleTitle(lpConsoleTitle, nSize, FALSE);
}


/*--------------------------------------------------------------
 *    SetConsoleTitleW
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleTitleW(LPCWSTR lpConsoleTitle)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETSETCONSOLETITLE TitleRequest = &ApiMessage.Data.TitleRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;

    TitleRequest->Length = wcslen(lpConsoleTitle) * sizeof(WCHAR);

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

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetTitle),
                                 sizeof(CONSOLE_GETSETCONSOLETITLE));

    CsrFreeCaptureBuffer(CaptureBuffer);

    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return TRUE;
}


/*--------------------------------------------------------------
 *    SetConsoleTitleA
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleTitleA(LPCSTR lpConsoleTitle)
{
    BOOL   Ret;
    ULONG  Length = strlen(lpConsoleTitle) + 1;
    LPWSTR WideTitle = HeapAlloc(GetProcessHeap(), 0, Length * sizeof(WCHAR));

    if (!WideTitle)
    {
        SetLastError(ERROR_NOT_ENOUGH_MEMORY);
        return FALSE;
    }

    MultiByteToWideChar(CP_ACP, 0, lpConsoleTitle, -1, WideTitle, Length);

    Ret = SetConsoleTitleW(WideTitle);

    HeapFree(GetProcessHeap(), 0, WideTitle);
    return Ret;
}


/*--------------------------------------------------------------
 *    CreateConsoleScreenBuffer
 *
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
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    if ( (dwDesiredAccess & ~(GENERIC_READ | GENERIC_WRITE))    ||
         (dwShareMode & ~(FILE_SHARE_READ | FILE_SHARE_WRITE))  ||
         (dwFlags != CONSOLE_TEXTMODE_BUFFER) )
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return INVALID_HANDLE_VALUE;
    }

    ApiMessage.Data.CreateScreenBufferRequest.Access = dwDesiredAccess;
    ApiMessage.Data.CreateScreenBufferRequest.ShareMode = dwShareMode;
    ApiMessage.Data.CreateScreenBufferRequest.Inheritable =
        (lpSecurityAttributes ? lpSecurityAttributes->bInheritHandle : FALSE);

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepCreateScreenBuffer),
                                 sizeof(CONSOLE_CREATESCREENBUFFER));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return INVALID_HANDLE_VALUE;
    }

    return ApiMessage.Data.CreateScreenBufferRequest.OutputHandle;
}


/*--------------------------------------------------------------
 *    GetConsoleCP
 *
 * @implemented
 */
UINT
WINAPI
GetConsoleCP(VOID)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    /* Get the Input Code Page */
    ApiMessage.Data.ConsoleCPRequest.InputCP = TRUE;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetCP),
                                 sizeof(CONSOLE_GETSETINPUTOUTPUTCP));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return 0;
    }

    return ApiMessage.Data.ConsoleCPRequest.CodePage;
}


/*--------------------------------------------------------------
 *    SetConsoleCP
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleCP(UINT wCodePageID)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    /* Set the Input Code Page */
    ApiMessage.Data.ConsoleCPRequest.InputCP = TRUE;
    ApiMessage.Data.ConsoleCPRequest.CodePage = wCodePageID;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetCP),
                                 sizeof(CONSOLE_GETSETINPUTOUTPUTCP));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
    }

    return NT_SUCCESS(Status);
}


/*--------------------------------------------------------------
 *    GetConsoleOutputCP
 *
 * @implemented
 */
UINT
WINAPI
GetConsoleOutputCP(VOID)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    /* Get the Output Code Page */
    ApiMessage.Data.ConsoleCPRequest.InputCP = FALSE;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetCP),
                                 sizeof(CONSOLE_GETSETINPUTOUTPUTCP));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError (Status);
        return 0;
    }

    return ApiMessage.Data.ConsoleCPRequest.CodePage;
}


/*--------------------------------------------------------------
 *    SetConsoleOutputCP
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleOutputCP(UINT wCodePageID)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    /* Set the Output Code Page */
    ApiMessage.Data.ConsoleCPRequest.InputCP = FALSE;
    ApiMessage.Data.ConsoleCPRequest.CodePage = wCodePageID;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetCP),
                                 sizeof(CONSOLE_GETSETINPUTOUTPUTCP));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
    }

    return NT_SUCCESS(Status);
}


/*--------------------------------------------------------------
 *     GetConsoleProcessList
 *
 * @implemented
 */
DWORD
WINAPI
GetConsoleProcessList(LPDWORD lpdwProcessList,
                      DWORD dwProcessCount)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_GETPROCESSLIST GetProcessListRequest = &ApiMessage.Data.GetProcessListRequest;
    PCSR_CAPTURE_BUFFER CaptureBuffer;
    ULONG nProcesses;

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
        return FALSE;
    }

    GetProcessListRequest->nMaxIds = dwProcessCount;

    CsrAllocateMessagePointer(CaptureBuffer,
                              dwProcessCount * sizeof(DWORD),
                              (PVOID*)&GetProcessListRequest->pProcessIds);

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 CaptureBuffer,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetProcessList),
                                 sizeof(CONSOLE_GETPROCESSLIST));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError (Status);
        nProcesses = 0;
    }
    else
    {
        nProcesses = GetProcessListRequest->nProcessIdsTotal;
        if (dwProcessCount >= nProcesses)
        {
            memcpy(lpdwProcessList, GetProcessListRequest->pProcessIds, nProcesses * sizeof(DWORD));
        }
    }

    CsrFreeCaptureBuffer(CaptureBuffer);
    return nProcesses;
}


/*--------------------------------------------------------------
 *     GetConsoleSelectionInfo
 *
 * @implemented
 */
BOOL
WINAPI
GetConsoleSelectionInfo(PCONSOLE_SELECTION_INFO lpConsoleSelectionInfo)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    if (lpConsoleSelectionInfo == NULL)
    {
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetSelectionInfo),
                                 sizeof(CONSOLE_GETSELECTIONINFO));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    *lpConsoleSelectionInfo = ApiMessage.Data.GetSelectionInfoRequest.Info;
    return TRUE;
}


/*--------------------------------------------------------------
 *     AttachConsole
 *
 * @implemented
 *
 * @note Strongly inspired by AllocConsole.
 */
BOOL
WINAPI
AttachConsole(DWORD dwProcessId)
{
    NTSTATUS Status;
    PRTL_USER_PROCESS_PARAMETERS Parameters = NtCurrentPeb()->ProcessParameters;
    CONSOLE_API_MESSAGE ApiMessage;
    PCONSOLE_ATTACHCONSOLE AttachConsoleRequest = &ApiMessage.Data.AttachConsoleRequest;

    if (Parameters->ConsoleHandle)
    {
        DPRINT1("AttachConsole: Attaching a console to a process already having one\n");
        SetLastError(ERROR_ACCESS_DENIED);
        return FALSE;
    }

    AttachConsoleRequest->ProcessId = dwProcessId;
    AttachConsoleRequest->CtrlDispatcher = ConsoleControlDispatcher;
    AttachConsoleRequest->PropDispatcher = PropDialogHandler;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepAttach),
                                 sizeof(CONSOLE_ATTACHCONSOLE));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    Parameters->ConsoleHandle = AttachConsoleRequest->Console;
    SetStdHandle(STD_INPUT_HANDLE , AttachConsoleRequest->InputHandle );
    SetStdHandle(STD_OUTPUT_HANDLE, AttachConsoleRequest->OutputHandle);
    SetStdHandle(STD_ERROR_HANDLE , AttachConsoleRequest->ErrorHandle );

    /* Initialize Console Ctrl Handler */
    InitConsoleCtrlHandling();

    InputWaitHandle = AttachConsoleRequest->InputWaitHandle;

    return TRUE;
}


/*--------------------------------------------------------------
 *     GetConsoleWindow
 *
 * @implemented
 */
HWND
WINAPI
GetConsoleWindow(VOID)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepGetConsoleWindow),
                                 sizeof(CONSOLE_GETWINDOW));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return (HWND)NULL;
    }

    return ApiMessage.Data.GetWindowRequest.WindowHandle;
}


/*--------------------------------------------------------------
 *     SetConsoleIcon
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleIcon(HICON hicon)
{
    NTSTATUS Status;
    CONSOLE_API_MESSAGE ApiMessage;

    ApiMessage.Data.SetIconRequest.WindowIcon = hicon;

    Status = CsrClientCallServer((PCSR_API_MESSAGE)&ApiMessage,
                                 NULL,
                                 CSR_CREATE_API_NUMBER(CONSRV_SERVERDLL_INDEX, ConsolepSetIcon),
                                 sizeof(CONSOLE_SETICON));
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return NT_SUCCESS(Status);
}


/******************************************************************************
 * \name SetConsoleInputExeNameW
 * \brief Sets the console input file name from a unicode string.
 * \param lpInputExeName Pointer to a unicode string with the name.
 * \return TRUE if successful, FALSE if unsuccsedful.
 * \remarks If lpInputExeName is 0 or the string length is 0 or greater than 255,
 *          the function fails and sets last error to ERROR_INVALID_PARAMETER.
 */
BOOL
WINAPI
SetConsoleInputExeNameW(LPCWSTR lpInputExeName)
{
    int lenName;

    if ( !lpInputExeName                            ||
        (lenName = lstrlenW(lpInputExeName)) == 0   ||
         lenName > INPUTEXENAME_BUFLEN - 1 )
    {
        /* Fail if string is empty or too long */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    RtlEnterCriticalSection(&ConsoleLock);
    _SEH2_TRY
    {
        RtlCopyMemory(InputExeName, lpInputExeName, lenName * sizeof(WCHAR));
        InputExeName[lenName] = L'\0';
    }
    _SEH2_FINALLY
    {
        RtlLeaveCriticalSection(&ConsoleLock);
    }
    _SEH2_END;

    return TRUE;
}


/******************************************************************************
 * \name SetConsoleInputExeNameA
 * \brief Sets the console input file name from an ansi string.
 * \param lpInputExeName Pointer to an ansi string with the name.
 * \return TRUE if successful, FALSE if unsuccsedful.
 * \remarks If lpInputExeName is 0 or the string length is 0 or greater than 255,
 *          the function fails and sets last error to ERROR_INVALID_PARAMETER.
 */
BOOL
WINAPI
SetConsoleInputExeNameA(LPCSTR lpInputExeName)
{
    WCHAR Buffer[INPUTEXENAME_BUFLEN];
    ANSI_STRING InputExeNameA;
    UNICODE_STRING InputExeNameU;
    NTSTATUS Status;

    RtlInitAnsiString(&InputExeNameA, lpInputExeName);

    if ( InputExeNameA.Length == 0 || 
         InputExeNameA.Length > INPUTEXENAME_BUFLEN - 1 )
    {
        /* Fail if string is empty or too long */
        SetLastError(ERROR_INVALID_PARAMETER);
        return FALSE;
    }

    InputExeNameU.Buffer = Buffer;
    InputExeNameU.MaximumLength = sizeof(Buffer);
    InputExeNameU.Length = 0;

    Status = RtlAnsiStringToUnicodeString(&InputExeNameU, &InputExeNameA, FALSE);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    return SetConsoleInputExeNameW(InputExeNameU.Buffer);
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
GetConsoleInputExeNameW(DWORD nBufferLength, LPWSTR lpBuffer)
{
    ULONG lenName = lstrlenW(InputExeName);

    if (nBufferLength == 0)
    {
        /* Buffer size is requested, return it */
        return lenName + 1;
    }

    if (lenName + 1 > nBufferLength)
    {
        /* Buffer is not large enough! */
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return 2;
    }

    RtlEnterCriticalSection(&ConsoleLock);
    _SEH2_TRY
    {
        RtlCopyMemory(lpBuffer, InputExeName, lenName * sizeof(WCHAR));
        lpBuffer[lenName] = '\0';
    }
    _SEH2_FINALLY
    {
        RtlLeaveCriticalSection(&ConsoleLock);
    }
    _SEH2_END;

    /* Success, return 1 */
    return 1;
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
GetConsoleInputExeNameA(DWORD nBufferLength, LPSTR lpBuffer)
{
    WCHAR Buffer[INPUTEXENAME_BUFLEN];
    DWORD Ret;
    UNICODE_STRING BufferU;
    ANSI_STRING BufferA;

    /* Get the unicode name */
    Ret = GetConsoleInputExeNameW(sizeof(Buffer) / sizeof(Buffer[0]), Buffer);

    /* Initialize strings for conversion */
    RtlInitUnicodeString(&BufferU, Buffer);
    BufferA.Length = 0;
    BufferA.MaximumLength = nBufferLength;
    BufferA.Buffer = lpBuffer;

    /* Convert unicode name to ansi, copying as much chars as fit */
    RtlUnicodeStringToAnsiString(&BufferA, &BufferU, FALSE);

    /* Error handling */
    if (nBufferLength <= BufferU.Length / sizeof(WCHAR))
    {
        SetLastError(ERROR_BUFFER_OVERFLOW);
        return 2;
    }

    return Ret;
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
GetConsoleCursorMode(HANDLE hConsole, PBOOL pUnknown1, PBOOL pUnknown2)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
GetConsoleNlsMode(HANDLE hConsole, LPDWORD lpMode)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
SetConsoleCursorMode(HANDLE hConsole, BOOL Unknown1, BOOL Unknown2)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
SetConsoleLocalEUDC(DWORD Unknown1, DWORD Unknown2, DWORD Unknown3, DWORD Unknown4)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
SetConsoleNlsMode(HANDLE hConsole, DWORD dwMode)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
RegisterConsoleIME(HWND hWnd, LPDWORD ThreadId)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
RegisterConsoleOS2(BOOL bUnknown)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
SetConsoleOS2OemFormat(BOOL bUnknown)
{
    STUB;
    return FALSE;
}

BOOL
WINAPI
UnregisterConsoleIME(VOID)
{
    STUB;
    return FALSE;
}


/*
 * @unimplemented
 */
BOOL WINAPI GetConsoleKeyboardLayoutNameA(LPSTR name)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL WINAPI GetConsoleKeyboardLayoutNameW(LPWSTR name)
{
    STUB;
    return 0;
}

/*
 * @unimplemented
 */
BOOL
WINAPI
SetLastConsoleEventActive(VOID)
{
    STUB;
    return FALSE;
}

/* EOF */
