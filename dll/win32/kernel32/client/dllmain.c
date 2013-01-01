/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/dllmain.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Ariadne (ariadne@xs4all.nl)
 *                  Aleksey Bragin (aleksey@reactos.org)
 * UPDATE HISTORY:
 *                  Created 01/11/98
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

// #define NDEBUG
#include <debug.h>

/* GLOBALS *******************************************************************/

PBASE_STATIC_SERVER_DATA BaseStaticServerData;

BOOLEAN BaseRunningInServerProcess;

WCHAR BaseDefaultPathBuffer[6140];

HANDLE BaseNamedObjectDirectory;
HMODULE hCurrentModule = NULL;
HMODULE kernel32_handle = NULL;
PPEB Peb;
ULONG SessionId;
BOOL ConsoleInitialized = FALSE;
static BOOL DllInitialized = FALSE;

/* Critical section for various kernel32 data structures */
RTL_CRITICAL_SECTION BaseDllDirectoryLock;
RTL_CRITICAL_SECTION ConsoleLock;

extern DWORD WINAPI ConsoleControlDispatcher(IN LPVOID lpThreadParameter);
extern HANDLE InputWaitHandle;

extern BOOL FASTCALL NlsInit(VOID);
extern VOID FASTCALL NlsUninit(VOID);

#define WIN_OBJ_DIR L"\\Windows"
#define SESSION_DIR L"\\Sessions"

/* FUNCTIONS *****************************************************************/

BOOL
WINAPI
BasepInitConsole(VOID)
{
    NTSTATUS Status;
    PRTL_USER_PROCESS_PARAMETERS Parameters = NtCurrentPeb()->ProcessParameters;
    LPCWSTR ExeName;
    STARTUPINFO si;
    WCHAR SessionDir[256];
    ULONG SessionId = NtCurrentPeb()->SessionId;
    BOOLEAN InServer;

    CONSOLE_CONNECTION_INFO ConnectInfo;
    ULONG ConnectInfoSize = sizeof(ConnectInfo);

    WCHAR lpTest[MAX_PATH];
    GetModuleFileNameW(NULL, lpTest, MAX_PATH);
    DPRINT("BasepInitConsole for : %S\n", lpTest);
    DPRINT("Our current console handles are: %lx, %lx, %lx %lx\n",
           Parameters->ConsoleHandle, Parameters->StandardInput,
           Parameters->StandardOutput, Parameters->StandardError);

    /* Initialize our global console DLL lock */
    Status = RtlInitializeCriticalSection(&ConsoleLock);
    if (!NT_SUCCESS(Status)) return FALSE;
    ConsoleInitialized = TRUE;

    /* We have nothing to do if this isn't a console app... */
    if (RtlImageNtHeader(GetModuleHandle(NULL))->OptionalHeader.Subsystem !=
        IMAGE_SUBSYSTEM_WINDOWS_CUI)
    {
        DPRINT("Image is not a console application\n");
        Parameters->ConsoleHandle = NULL;
        ConnectInfo.ConsoleNeeded = FALSE; // ConsoleNeeded is used for knowing whether or not this is a CUI app.
    }
    else
    {
        /* Assume one is needed */
        GetStartupInfo(&si);
        ConnectInfo.ConsoleNeeded = TRUE;
        ConnectInfo.ShowCmd = si.wShowWindow;

        /* Handle the special flags given to us by BasePushProcessParameters */
        if (Parameters->ConsoleHandle == HANDLE_DETACHED_PROCESS)
        {
            /* No console to create */
            DPRINT("No console to create\n");
            Parameters->ConsoleHandle = NULL;
            ConnectInfo.ConsoleNeeded = FALSE;
        }
        else if (Parameters->ConsoleHandle == HANDLE_CREATE_NEW_CONSOLE)
        {
            /* We'll get the real one soon */
            DPRINT("Creating new console\n");
            Parameters->ConsoleHandle = NULL;
        }
        else if (Parameters->ConsoleHandle == HANDLE_CREATE_NO_WINDOW)
        {
            /* We'll get the real one soon */
            DPRINT("Creating new invisible console\n");
            Parameters->ConsoleHandle = NULL;
            ConnectInfo.ShowCmd = SW_HIDE;
        }
        else
        {
            if (Parameters->ConsoleHandle == INVALID_HANDLE_VALUE)
            {
                Parameters->ConsoleHandle = NULL;
            }
            DPRINT("Using existing console: %x\n", Parameters->ConsoleHandle);
        }
    }

    /* Now use the proper console handle */
    ConnectInfo.Console = Parameters->ConsoleHandle;

    /* Initialize Console Ctrl Handler and input EXE name */
    InitConsoleCtrlHandling();
    ConnectInfo.CtrlDispatcher = ConsoleControlDispatcher;

    ExeName = wcsrchr(Parameters->ImagePathName.Buffer, L'\\');
    if (ExeName)
        SetConsoleInputExeNameW(ExeName + 1);

    /* Setup the right Object Directory path */
    if (!SessionId)
    {
        /* Use the raw path */
        wcscpy(SessionDir, WIN_OBJ_DIR);
    }
    else
    {
        /* Use the session path */
        swprintf(SessionDir,
                 L"%ws\\%ld%ws",
                 SESSION_DIR,
                 SessionId,
                 WIN_OBJ_DIR);
    }

    /* Connect to the Console Server */
    DPRINT("Connecting to the Console Server in BasepInitConsole...\n");
    Status = CsrClientConnectToServer(SessionDir,
                                      CONSRV_SERVERDLL_INDEX,
                                      &ConnectInfo,
                                      &ConnectInfoSize,
                                      &InServer);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to connect to the Console Server (Status %lx)\n", Status);
        return FALSE;
    }

    /* Nothing to do for server-to-server */
    if (InServer) return TRUE;

    /* Nothing to do if not a console app */
    if (!ConnectInfo.ConsoleNeeded) return TRUE;

    /* We got the handles, let's set them */
    if ((Parameters->ConsoleHandle = ConnectInfo.Console))
    {
        /* If we already had some, don't use the new ones */
        if (!Parameters->StandardInput)
        {
            Parameters->StandardInput = ConnectInfo.InputHandle;
        }
        if (!Parameters->StandardOutput)
        {
            Parameters->StandardOutput = ConnectInfo.OutputHandle;
        }
        if (!Parameters->StandardError)
        {
            Parameters->StandardError = ConnectInfo.ErrorHandle;
        }
    }

    InputWaitHandle = ConnectInfo.InputWaitHandle;

    DPRINT("Console setup: %lx, %lx, %lx, %lx\n",
            Parameters->ConsoleHandle,
            Parameters->StandardInput,
            Parameters->StandardOutput,
            Parameters->StandardError);
    return TRUE;
}

NTSTATUS
NTAPI
BaseCreateThreadPoolThread(IN PTHREAD_START_ROUTINE Function,
                           IN PVOID Parameter,
                           OUT PHANDLE ThreadHandle)
{
    NTSTATUS Status;

    /* Create a Win32 thread */
    *ThreadHandle = CreateRemoteThread(NtCurrentProcess(),
                                       NULL,
                                       0,
                                       Function,
                                       Parameter,
                                       CREATE_SUSPENDED,
                                       NULL);
    if (!(*ThreadHandle))
    {
        /* Get the status value if we couldn't get a handle */
        Status = NtCurrentTeb()->LastStatusValue;
        if (NT_SUCCESS(Status)) Status = STATUS_UNSUCCESSFUL;
    }
    else
    {
        /* Set success code */
        Status = STATUS_SUCCESS;
    }

    /* All done */
    return Status;
}

NTSTATUS
NTAPI
BaseExitThreadPoolThread(IN NTSTATUS ExitStatus)
{
    /* Exit the thread */
    ExitThread(ExitStatus);
}

BOOL
WINAPI
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    NTSTATUS Status;
    ULONG Dummy;
    ULONG DummySize = sizeof(Dummy);
    WCHAR SessionDir[256];

    DPRINT("DllMain(hInst %lx, dwReason %lu)\n",
           hDll, dwReason);

    Basep8BitStringToUnicodeString = RtlAnsiStringToUnicodeString;

    /* Cache the PEB and Session ID */
    Peb = NtCurrentPeb();
    SessionId = Peb->SessionId;

    switch (dwReason)
    {
        case DLL_PROCESS_ATTACH:
        
        /* Set no filter intially */
        GlobalTopLevelExceptionFilter = RtlEncodePointer(NULL);
        
        /* Enable the Rtl thread pool and timer queue to use proper Win32 thread */
        RtlSetThreadPoolStartFunc(BaseCreateThreadPoolThread, BaseExitThreadPoolThread);

        /* Don't bother us for each thread */
        LdrDisableThreadCalloutsForDll((PVOID)hDll);

        /* Initialize default path to NULL */
        RtlInitUnicodeString(&BaseDefaultPath, NULL);

        /* Setup the right Object Directory path */
        if (!SessionId)
        {
            /* Use the raw path */
            wcscpy(SessionDir, WIN_OBJ_DIR);
        }
        else
        {
            /* Use the session path */
            swprintf(SessionDir,
                     L"%ws\\%ld%ws",
                     SESSION_DIR,
                     SessionId,
                     WIN_OBJ_DIR);
        }

        /* Connect to the base server */
        DPRINT("Connecting to CSR in DllMain...\n");
        Status = CsrClientConnectToServer(SessionDir,
                                          BASESRV_SERVERDLL_INDEX,
                                          &Dummy,
                                          &DummySize,
                                          &BaseRunningInServerProcess);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Failed to connect to CSR (Status %lx)\n", Status);
            NtTerminateProcess(NtCurrentProcess(), Status);
            return FALSE;
        }
        DPRINT("kernel32 DllMain - OK, connection succeeded\n");

        /* Get the server data */
        ASSERT(Peb->ReadOnlyStaticServerData);
        BaseStaticServerData = Peb->ReadOnlyStaticServerData[BASESRV_SERVERDLL_INDEX];
        ASSERT(BaseStaticServerData);

        /* Check if we are running a CSR Server */
        if (!BaseRunningInServerProcess)
        {
            /* Set the termination port for the thread */
            DPRINT("Creating new thread for CSR\n");
            CsrNewThread();
        }

        /* Initialize heap handle table */
        BaseDllInitializeMemoryManager();

        /* Set HMODULE for our DLL */
        kernel32_handle = hCurrentModule = hDll;

        /* Set the directories */
        BaseWindowsDirectory = BaseStaticServerData->WindowsDirectory;
        BaseWindowsSystemDirectory = BaseStaticServerData->WindowsSystemDirectory;

        /* Construct the default path (using the static buffer) */
        _snwprintf(BaseDefaultPathBuffer, sizeof(BaseDefaultPathBuffer) / sizeof(WCHAR),
            L".;%wZ;%wZ\\system;%wZ;", &BaseWindowsSystemDirectory, &BaseWindowsDirectory, &BaseWindowsDirectory);

        BaseDefaultPath.Buffer = BaseDefaultPathBuffer;
        BaseDefaultPath.Length = wcslen(BaseDefaultPathBuffer) * sizeof(WCHAR);
        BaseDefaultPath.MaximumLength = sizeof(BaseDefaultPathBuffer);

        /* Use remaining part of the default path buffer for the append path */
        BaseDefaultPathAppend.Buffer = (PWSTR)((ULONG_PTR)BaseDefaultPathBuffer + BaseDefaultPath.Length);
        BaseDefaultPathAppend.Length = 0;
        BaseDefaultPathAppend.MaximumLength = BaseDefaultPath.MaximumLength - BaseDefaultPath.Length;

        /* Initialize command line */
        InitCommandLines();

        /* Initialize the DLL critical section */
        RtlInitializeCriticalSection(&BaseDllDirectoryLock);

        /* Initialize the National Language Support routines */
        if (!NlsInit())
        {
            DPRINT1("NLS Init failed\n");
            return FALSE;
        }

        /* Initialize Console Support */
        if (!BasepInitConsole())
        {
            DPRINT1("Failure to set up console\n");
            return FALSE;
        }

        /* Initialize application certification globals */
        InitializeListHead(&BasepAppCertDllsList);
        RtlInitializeCriticalSection(&gcsAppCert);

        /* Insert more dll attach stuff here! */
        DllInitialized = TRUE;
        DPRINT("Initialization complete\n");
        break;

        case DLL_PROCESS_DETACH:

            DPRINT("DLL_PROCESS_DETACH\n");
            if (DllInitialized == TRUE)
            {
                /* Insert more dll detach stuff here! */
                NlsUninit();

                /* Delete DLL critical section */
                if (ConsoleInitialized == TRUE)
                {
                    ConsoleInitialized = FALSE;
                    RtlDeleteCriticalSection(&ConsoleLock);
                }
                RtlDeleteCriticalSection(&BaseDllDirectoryLock);
            }
            break;

        default:
            break;
    }

    return TRUE;
}

/* EOF */
