/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/console/init.c
 * PURPOSE:         Console API Client Initialization
 * PROGRAMMERS:     Hermes Belusca - Maito
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

RTL_CRITICAL_SECTION ConsoleLock;
BOOL ConsoleInitialized = FALSE;

extern DWORD WINAPI ConsoleControlDispatcher(IN LPVOID lpThreadParameter);
extern HANDLE InputWaitHandle;

#define WIN_OBJ_DIR L"\\Windows"
#define SESSION_DIR L"\\Sessions"


/* FUNCTIONS ******************************************************************/

VOID
InitConsoleProps(IN OUT PCONSOLE_PROPS ConsoleProps)
{
    STARTUPINFOW si;

    GetStartupInfoW(&si);

    ConsoleProps->dwStartupFlags = si.dwFlags;
    if (si.dwFlags & STARTF_USEFILLATTRIBUTE)
    {
        ConsoleProps->FillAttribute = si.dwFillAttribute;
    }
    if (si.dwFlags & STARTF_USECOUNTCHARS)
    {
        ConsoleProps->ScreenBufferSize.X = (SHORT)(si.dwXCountChars);
        ConsoleProps->ScreenBufferSize.Y = (SHORT)(si.dwYCountChars);
    }
    if (si.dwFlags & STARTF_USESHOWWINDOW)
    {
        ConsoleProps->ShowWindow = si.wShowWindow;
    }
    if (si.dwFlags & STARTF_USEPOSITION)
    {
        ConsoleProps->ConsoleWindowOrigin.x = (LONG)(si.dwX);
        ConsoleProps->ConsoleWindowOrigin.y = (LONG)(si.dwY);
    }
    if (si.dwFlags & STARTF_USESIZE)
    {
        ConsoleProps->ConsoleWindowSize.cx = (LONG)(si.dwXSize);
        ConsoleProps->ConsoleWindowSize.cy = (LONG)(si.dwYSize);
    }

    if (si.lpTitle)
    {
        wcsncpy(ConsoleProps->ConsoleTitle, si.lpTitle, MAX_PATH + 1);
    }
    else
    {
        ConsoleProps->ConsoleTitle[0] = L'\0';
    }
}


BOOL
WINAPI
BasepInitConsole(VOID)
{
    NTSTATUS Status;
    PRTL_USER_PROCESS_PARAMETERS Parameters = NtCurrentPeb()->ProcessParameters;
    WCHAR SessionDir[256];
    ULONG SessionId = NtCurrentPeb()->SessionId;
    BOOLEAN InServer;

    CONSOLE_CONNECTION_INFO ConnectInfo;
    ULONG ConnectInfoSize = sizeof(ConnectInfo);

    DPRINT("BasepInitConsole for : %wZ\n", &Parameters->ImagePathName);
    DPRINT("Our current console handles are: %lx, %lx, %lx %lx\n",
           Parameters->ConsoleHandle, Parameters->StandardInput,
           Parameters->StandardOutput, Parameters->StandardError);

    /* Initialize our global console DLL lock */
    Status = RtlInitializeCriticalSection(&ConsoleLock);
    if (!NT_SUCCESS(Status)) return FALSE;
    ConsoleInitialized = TRUE;

    /* Do nothing if this isn't a console app... */
    if (RtlImageNtHeader(GetModuleHandle(NULL))->OptionalHeader.Subsystem !=
        IMAGE_SUBSYSTEM_WINDOWS_CUI)
    {
        DPRINT("Image is not a console application\n");
        Parameters->ConsoleHandle = NULL;
        ConnectInfo.ConsoleNeeded = FALSE; // ConsoleNeeded is used for knowing whether or not this is a CUI app.

        ConnectInfo.ConsoleProps.ConsoleTitle[0] = L'\0';
        ConnectInfo.AppPath[0] = L'\0';
    }
    else
    {
        SIZE_T Length = 0;
        LPCWSTR ExeName;

        InitConsoleProps(&ConnectInfo.ConsoleProps);

        Length = min(sizeof(ConnectInfo.AppPath) / sizeof(ConnectInfo.AppPath[0]),
                     Parameters->ImagePathName.Length / sizeof(WCHAR));
        wcsncpy(ConnectInfo.AppPath, Parameters->ImagePathName.Buffer, Length);
        ConnectInfo.AppPath[Length] = L'\0';

        /* Initialize Input EXE name */
        ExeName = wcsrchr(Parameters->ImagePathName.Buffer, L'\\');
        if (ExeName) SetConsoleInputExeNameW(ExeName + 1);

        /* Assume one is needed */
        ConnectInfo.ConsoleNeeded = TRUE;

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
            ConnectInfo.ConsoleProps.ShowWindow = SW_HIDE;
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

    /* Initialize Console Ctrl Handler */
    InitConsoleCtrlHandling();
    ConnectInfo.CtrlDispatcher = ConsoleControlDispatcher;

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


VOID
WINAPI
BasepUninitConsole(VOID)
{
    /* Delete our critical section if we were initialized */
    if (ConsoleInitialized == TRUE)
    {
        ConsoleInitialized = FALSE;
        RtlDeleteCriticalSection(&ConsoleLock);
    }
}

/* EOF */
