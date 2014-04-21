/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            dll/win32/kernel32/client/console/init.c
 * PURPOSE:         Console API Client Initialization
 * PROGRAMMERS:     Alex Ionescu (alex.ionescu@reactos.org)
 *                  Aleksey Bragin (aleksey@reactos.org)
 *                  Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include <k32.h>

// For Control Panel Applet
#include <cpl.h>

#define NDEBUG
#include <debug.h>


/* GLOBALS ********************************************************************/

RTL_CRITICAL_SECTION ConsoleLock;
BOOL ConsoleInitialized = FALSE;

extern HANDLE InputWaitHandle;

static HMODULE ConsoleLibrary = NULL;
static BOOL AlreadyDisplayingProps = FALSE;

#define WIN_OBJ_DIR L"\\Windows"
#define SESSION_DIR L"\\Sessions"


/* FUNCTIONS ******************************************************************/

DWORD
WINAPI
PropDialogHandler(IN LPVOID lpThreadParameter)
{
    // NOTE: lpThreadParameter corresponds to the client shared section handle.

    APPLET_PROC CPLFunc;

    /*
     * Do not launch more than once the console property dialog applet,
     * or (albeit less probable), if we are not initialized.
     */
    if (!ConsoleInitialized || AlreadyDisplayingProps)
    {
        /* Close the associated client shared section handle if needed */
        if (lpThreadParameter)
        {
            CloseHandle((HANDLE)lpThreadParameter);
        }
        return STATUS_UNSUCCESSFUL;
    }

    AlreadyDisplayingProps = TRUE;

    /* Load the Control Applet if needed */
    if (ConsoleLibrary == NULL)
    {
        WCHAR szBuffer[MAX_PATH];

        GetSystemDirectoryW(szBuffer, MAX_PATH);
        wcscat(szBuffer, L"\\console.dll");
        ConsoleLibrary = LoadLibraryW(szBuffer);

        if (ConsoleLibrary == NULL)
        {
            DPRINT1("Failed to load console.dll\n");
            AlreadyDisplayingProps = FALSE;
            return STATUS_UNSUCCESSFUL;
        }
    }

    /* Load its main function */
    CPLFunc = (APPLET_PROC)GetProcAddress(ConsoleLibrary, "CPlApplet");
    if (CPLFunc == NULL)
    {
        DPRINT("Error: Console.dll misses CPlApplet export\n");
        AlreadyDisplayingProps = FALSE;
        return STATUS_UNSUCCESSFUL;
    }

    if (CPLFunc(NULL, CPL_INIT, 0, 0) == FALSE)
    {
        DPRINT("Error: failed to initialize console.dll\n");
        AlreadyDisplayingProps = FALSE;
        return STATUS_UNSUCCESSFUL;
    }

    if (CPLFunc(NULL, CPL_GETCOUNT, 0, 0) != 1)
    {
        DPRINT("Error: console.dll returned unexpected CPL count\n");
        AlreadyDisplayingProps = FALSE;
        return STATUS_UNSUCCESSFUL;
    }

    CPLFunc(NULL, CPL_DBLCLK, (LPARAM)lpThreadParameter, 0);
    CPLFunc(NULL, CPL_EXIT  , 0, 0);

    AlreadyDisplayingProps = FALSE;
    return STATUS_SUCCESS;
}


VOID
InitConsoleInfo(IN OUT PCONSOLE_START_INFO ConsoleStartInfo,
                IN PUNICODE_STRING ImagePathName)
{
    STARTUPINFOW si;
    SIZE_T Length;

    /* Get the startup information */
    GetStartupInfoW(&si);

    /* Initialize the fields */
    ConsoleStartInfo->dwStartupFlags = si.dwFlags;
    if (si.dwFlags & STARTF_USEFILLATTRIBUTE)
    {
        ConsoleStartInfo->wFillAttribute = (WORD)si.dwFillAttribute;
    }
    if (si.dwFlags & STARTF_USECOUNTCHARS)
    {
        ConsoleStartInfo->dwScreenBufferSize.X = (SHORT)(si.dwXCountChars);
        ConsoleStartInfo->dwScreenBufferSize.Y = (SHORT)(si.dwYCountChars);
    }
    if (si.dwFlags & STARTF_USESHOWWINDOW)
    {
        ConsoleStartInfo->wShowWindow = si.wShowWindow;
    }
    if (si.dwFlags & STARTF_USEPOSITION)
    {
        ConsoleStartInfo->dwWindowOrigin.X = (SHORT)(si.dwX);
        ConsoleStartInfo->dwWindowOrigin.Y = (SHORT)(si.dwY);
    }
    if (si.dwFlags & STARTF_USESIZE)
    {
        ConsoleStartInfo->dwWindowSize.X = (SHORT)(si.dwXSize);
        ConsoleStartInfo->dwWindowSize.Y = (SHORT)(si.dwYSize);
    }

    /* Set up the title for the console */
    if (si.lpTitle)
    {
        wcsncpy(ConsoleStartInfo->ConsoleTitle, si.lpTitle, MAX_PATH + 1);
    }
    else
    {
        ConsoleStartInfo->ConsoleTitle[0] = L'\0';
    }

    /* Retrieve the application path name */
    Length = min(sizeof(ConsoleStartInfo->AppPath) / sizeof(ConsoleStartInfo->AppPath[0]) - 1,
                 ImagePathName->Length / sizeof(WCHAR));
    wcsncpy(ConsoleStartInfo->AppPath, ImagePathName->Buffer, Length);
    ConsoleStartInfo->AppPath[Length] = L'\0';

    /* The Console Server will use these fields to set up the console icon */
    ConsoleStartInfo->IconPath[0] = L'\0';
    ConsoleStartInfo->IconIndex   = 0;
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

    CONSRV_API_CONNECTINFO ConnectInfo;
    ULONG ConnectInfoSize = sizeof(ConnectInfo);

    DPRINT("BasepInitConsole for : %wZ\n", &Parameters->ImagePathName);
    DPRINT("Our current console handles are: %p, %p, %p %p\n",
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
        ConnectInfo.ConsoleStartInfo.ConsoleNeeded = FALSE; // ConsoleNeeded is used for knowing whether or not this is a CUI app.

        ConnectInfo.ConsoleStartInfo.ConsoleTitle[0] = L'\0';
        ConnectInfo.ConsoleStartInfo.AppPath[0] = L'\0';
    }
    else
    {
        LPCWSTR ExeName;

        InitConsoleInfo(&ConnectInfo.ConsoleStartInfo,
                        &Parameters->ImagePathName);

        /* Initialize Input EXE name */
        ExeName = wcsrchr(Parameters->ImagePathName.Buffer, L'\\');
        if (ExeName) SetConsoleInputExeNameW(ExeName + 1);

        /* Assume one is needed */
        ConnectInfo.ConsoleStartInfo.ConsoleNeeded = TRUE;

        /* Handle the special flags given to us by BasePushProcessParameters */
        if (Parameters->ConsoleHandle == HANDLE_DETACHED_PROCESS)
        {
            /* No console to create */
            DPRINT("No console to create\n");
            Parameters->ConsoleHandle = NULL;
            ConnectInfo.ConsoleStartInfo.ConsoleNeeded = FALSE;
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
            ConnectInfo.ConsoleStartInfo.wShowWindow = SW_HIDE;
        }
        else
        {
            if (Parameters->ConsoleHandle == INVALID_HANDLE_VALUE)
            {
                Parameters->ConsoleHandle = NULL;
            }
            DPRINT("Using existing console: %p\n", Parameters->ConsoleHandle);
        }
    }

    /* Now use the proper console handle */
    ConnectInfo.ConsoleHandle = Parameters->ConsoleHandle;

    /* Initialize the Console Ctrl Handler */
    InitConsoleCtrlHandling();
    ConnectInfo.ConsoleStartInfo.CtrlDispatcher = ConsoleControlDispatcher;

    /* Initialize the Property Dialog Handler */
    ConnectInfo.ConsoleStartInfo.PropDispatcher = PropDialogHandler;

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
    if (!ConnectInfo.ConsoleStartInfo.ConsoleNeeded) return TRUE;

    /* We got the handles, let's set them */
    if ((Parameters->ConsoleHandle = ConnectInfo.ConsoleHandle))
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

    DPRINT("Console setup: %p, %p, %p, %p\n",
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
        if (ConsoleLibrary) FreeLibrary(ConsoleLibrary);

        ConsoleInitialized = FALSE;
        RtlDeleteCriticalSection(&ConsoleLock);
    }
}

/* EOF */
