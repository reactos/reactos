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
BOOLEAN ConsoleInitialized = FALSE;

extern HANDLE InputWaitHandle;

static HMODULE ConsoleApplet = NULL;
static BOOL AlreadyDisplayingProps = FALSE;

static const PWSTR DefaultConsoleTitle = L"ReactOS Console";


/* FUNCTIONS ******************************************************************/

DWORD
WINAPI
PropDialogHandler(IN LPVOID lpThreadParameter)
{
    // NOTE: lpThreadParameter corresponds to the client shared section handle.

    NTSTATUS Status = STATUS_SUCCESS;
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
    if (ConsoleApplet == NULL)
    {
        WCHAR szBuffer[MAX_PATH];

        GetSystemDirectoryW(szBuffer, MAX_PATH);
        wcscat(szBuffer, L"\\console.dll");
        ConsoleApplet = LoadLibraryW(szBuffer);
        if (ConsoleApplet == NULL)
        {
            DPRINT1("Failed to load console.dll\n");
            Status = STATUS_UNSUCCESSFUL;
            goto Quit;
        }
    }

    /* Load its main function */
    CPLFunc = (APPLET_PROC)GetProcAddress(ConsoleApplet, "CPlApplet");
    if (CPLFunc == NULL)
    {
        DPRINT1("Error: Console.dll misses CPlApplet export\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Quit;
    }

    if (CPLFunc(NULL, CPL_INIT, 0, 0) == FALSE)
    {
        DPRINT1("Error: failed to initialize console.dll\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Quit;
    }

    if (CPLFunc(NULL, CPL_GETCOUNT, 0, 0) != 1)
    {
        DPRINT1("Error: console.dll returned unexpected CPL count\n");
        Status = STATUS_UNSUCCESSFUL;
        goto Quit;
    }

    CPLFunc(NULL, CPL_DBLCLK, (LPARAM)lpThreadParameter, 0);
    CPLFunc(NULL, CPL_EXIT  , 0, 0);

Quit:
    AlreadyDisplayingProps = FALSE;
    return Status;
}


static INT
ParseShellInfo(LPCWSTR lpszShellInfo,
               LPCWSTR lpszKeyword)
{
    DPRINT1("ParseShellInfo is UNIMPLEMENTED\n");
    return 0;
}


/*
 * NOTE:
 * The "LPDWORD Length" parameters point on input to the maximum size of
 * the buffers that can hold data (if != 0), and on output they hold the
 * real size of the data. If "Length" are == 0 on input, then on output
 * they receive the full size of the data.
 * The "LPWSTR* lpTitle" parameter has a double meaning:
 * - when "CaptureTitle" is TRUE, data is copied to the buffer pointed
 *   by the pointer (*lpTitle).
 * - when "CaptureTitle" is FALSE, "*lpTitle" is set to the address of
 *   the source data.
 */
VOID
SetUpConsoleInfo(IN BOOLEAN CaptureTitle,
                 IN OUT LPDWORD pTitleLength,
                 IN OUT LPWSTR* lpTitle OPTIONAL,
                 IN OUT LPDWORD pDesktopLength,
                 IN OUT LPWSTR* lpDesktop OPTIONAL,
                 IN OUT PCONSOLE_START_INFO ConsoleStartInfo)
{
    PRTL_USER_PROCESS_PARAMETERS Parameters = NtCurrentPeb()->ProcessParameters;
    DWORD Length;

    /* Initialize the fields */

    ConsoleStartInfo->IconIndex = 0;
    ConsoleStartInfo->hIcon   = NULL;
    ConsoleStartInfo->hIconSm = NULL;
    ConsoleStartInfo->dwStartupFlags = Parameters->WindowFlags;
    ConsoleStartInfo->nFont = 0;
    ConsoleStartInfo->nInputBufferSize = 0;
    ConsoleStartInfo->uCodePage = GetOEMCP();

    if (lpTitle)
    {
        LPWSTR Title;

        /* If we don't have any title, use the default one */
        if (Parameters->WindowTitle.Buffer == NULL)
        {
            Title  = DefaultConsoleTitle;
            Length = lstrlenW(DefaultConsoleTitle) * sizeof(WCHAR); // sizeof(DefaultConsoleTitle);
        }
        else
        {
            Title  = Parameters->WindowTitle.Buffer;
            Length = Parameters->WindowTitle.Length;
        }

        /* Retrieve the needed buffer size */
        Length += sizeof(WCHAR);
        if (*pTitleLength > 0) Length = min(Length, *pTitleLength);
        *pTitleLength = Length;

        /* Capture the data if needed, or, return a pointer to it */
        if (CaptureTitle)
        {
            /*
             * Length is always >= sizeof(WCHAR). Copy everything but the
             * possible trailing NULL character, and then NULL-terminate.
             */
            Length -= sizeof(WCHAR);
            RtlCopyMemory(*lpTitle, Title, Length);
            (*lpTitle)[Length / sizeof(WCHAR)] = UNICODE_NULL;
        }
        else
        {
            *lpTitle = Title;
        }
    }
    else
    {
        *pTitleLength = 0;
    }

    if (lpDesktop && Parameters->DesktopInfo.Buffer && *Parameters->DesktopInfo.Buffer)
    {
        /* Retrieve the needed buffer size */
        Length = Parameters->DesktopInfo.Length + sizeof(WCHAR);
        if (*pDesktopLength > 0) Length = min(Length, *pDesktopLength);
        *pDesktopLength = Length;

        /* Return a pointer to the data */
        *lpDesktop = Parameters->DesktopInfo.Buffer;
    }
    else
    {
        *pDesktopLength = 0;
        if (lpDesktop) *lpDesktop = NULL;
    }

    if (Parameters->WindowFlags & STARTF_USEFILLATTRIBUTE)
    {
        ConsoleStartInfo->wFillAttribute = (WORD)Parameters->FillAttribute;
    }
    if (Parameters->WindowFlags & STARTF_USECOUNTCHARS)
    {
        ConsoleStartInfo->dwScreenBufferSize.X = (SHORT)Parameters->CountCharsX;
        ConsoleStartInfo->dwScreenBufferSize.Y = (SHORT)Parameters->CountCharsY;
    }
    if (Parameters->WindowFlags & STARTF_USESHOWWINDOW)
    {
        ConsoleStartInfo->wShowWindow = (WORD)Parameters->ShowWindowFlags;
    }
    if (Parameters->WindowFlags & STARTF_USEPOSITION)
    {
        ConsoleStartInfo->dwWindowOrigin.X = (SHORT)Parameters->StartingX;
        ConsoleStartInfo->dwWindowOrigin.Y = (SHORT)Parameters->StartingY;
    }
    if (Parameters->WindowFlags & STARTF_USESIZE)
    {
        ConsoleStartInfo->dwWindowSize.X = (SHORT)Parameters->CountX;
        ConsoleStartInfo->dwWindowSize.Y = (SHORT)Parameters->CountY;
    }

    /* Get shell information (ShellInfo.Buffer is NULL-terminated) */
    if (Parameters->ShellInfo.Buffer != NULL)
    {
        ConsoleStartInfo->IconIndex = ParseShellInfo(Parameters->ShellInfo.Buffer, L"dde.");

        if ((Parameters->WindowFlags & STARTF_USEHOTKEY) == 0)
            ConsoleStartInfo->dwHotKey = ParseShellInfo(Parameters->ShellInfo.Buffer, L"hotkey.");
        else
            ConsoleStartInfo->dwHotKey = HandleToUlong(Parameters->StandardInput);
    }
}


VOID
SetUpHandles(IN PCONSOLE_START_INFO ConsoleStartInfo)
{
    PRTL_USER_PROCESS_PARAMETERS Parameters = NtCurrentPeb()->ProcessParameters;

    if (ConsoleStartInfo->dwStartupFlags & STARTF_USEHOTKEY)
    {
        Parameters->WindowFlags &= ~STARTF_USEHOTKEY;
    }
    if (ConsoleStartInfo->dwStartupFlags & STARTF_SHELLPRIVATE)
    {
        Parameters->WindowFlags &= ~STARTF_SHELLPRIVATE;
    }

    /* We got the handles, let's set them */
    Parameters->ConsoleHandle = ConsoleStartInfo->ConsoleHandle;

    if ((ConsoleStartInfo->dwStartupFlags & STARTF_USESTDHANDLES) == 0)
    {
        Parameters->StandardInput  = ConsoleStartInfo->InputHandle;
        Parameters->StandardOutput = ConsoleStartInfo->OutputHandle;
        Parameters->StandardError  = ConsoleStartInfo->ErrorHandle;
    }
}


static BOOLEAN
IsConsoleApp(VOID)
{
    PIMAGE_NT_HEADERS ImageNtHeader = RtlImageNtHeader(NtCurrentPeb()->ImageBaseAddress);
    return (ImageNtHeader && (ImageNtHeader->OptionalHeader.Subsystem ==
                              IMAGE_SUBSYSTEM_WINDOWS_CUI));
}


static BOOLEAN
ConnectConsole(IN PWSTR SessionDir,
               IN PCONSRV_API_CONNECTINFO ConnectInfo,
               OUT PBOOLEAN InServerProcess)
{
    NTSTATUS Status;
    ULONG ConnectInfoSize = sizeof(*ConnectInfo);

    ASSERT(SessionDir);

    /* Connect to the Console Server */
    DPRINT("Connecting to the Console Server...\n");
    Status = CsrClientConnectToServer(SessionDir,
                                      CONSRV_SERVERDLL_INDEX,
                                      ConnectInfo,
                                      &ConnectInfoSize,
                                      InServerProcess);
    if (!NT_SUCCESS(Status))
    {
        DPRINT1("Failed to connect to the Console Server (Status %lx)\n", Status);
        return FALSE;
    }

    /* Nothing to do for server-to-server */
    if (*InServerProcess) return TRUE;

    /* Nothing to do if this is not a console app */
    if (!ConnectInfo->IsConsoleApp) return TRUE;

    /* Wait for the connection to finish */
    // Is ConnectInfo->ConsoleStartInfo.InitEvents aligned on handle boundary ????
    Status = NtWaitForMultipleObjects(MAX_INIT_EVENTS,
                                      ConnectInfo->ConsoleStartInfo.InitEvents,
                                      WaitAny, FALSE, NULL);
    if (!NT_SUCCESS(Status))
    {
        BaseSetLastNTError(Status);
        return FALSE;
    }

    NtClose(ConnectInfo->ConsoleStartInfo.InitEvents[INIT_SUCCESS]);
    NtClose(ConnectInfo->ConsoleStartInfo.InitEvents[INIT_FAILURE]);
    if (Status != INIT_SUCCESS)
    {
        NtCurrentPeb()->ProcessParameters->ConsoleHandle = NULL;
        return FALSE;
    }

    return TRUE;
}


BOOLEAN
WINAPI
ConDllInitialize(IN ULONG Reason,
                 IN PWSTR SessionDir)
{
    NTSTATUS Status;
    PRTL_USER_PROCESS_PARAMETERS Parameters = NtCurrentPeb()->ProcessParameters;
    BOOLEAN InServerProcess = FALSE;
    CONSRV_API_CONNECTINFO ConnectInfo;
    LCID lcid;

    if (Reason != DLL_PROCESS_ATTACH)
    {
        if ((Reason == DLL_THREAD_ATTACH) && IsConsoleApp())
        {
            /* Sets the current console locale for the new thread */
            SetTEBLangID(lcid);
        }
        else if (Reason == DLL_PROCESS_DETACH)
        {
            /* Free our resources */
            if (ConsoleInitialized == TRUE)
            {
                if (ConsoleApplet) FreeLibrary(ConsoleApplet);

                ConsoleInitialized = FALSE;
                RtlDeleteCriticalSection(&ConsoleLock);
            }
        }

        return TRUE;
    }

    DPRINT("ConDllInitialize for: %wZ\n"
           "Our current console handles are: 0x%p, 0x%p, 0x%p 0x%p\n",
           &Parameters->ImagePathName,
           Parameters->ConsoleHandle,
           Parameters->StandardInput,
           Parameters->StandardOutput,
           Parameters->StandardError);

    /* Initialize our global console DLL lock */
    Status = RtlInitializeCriticalSection(&ConsoleLock);
    if (!NT_SUCCESS(Status)) return FALSE;
    ConsoleInitialized = TRUE;

    /* Show by default the console window when applicable */
    ConnectInfo.IsWindowVisible = TRUE;
    /* If this is a console app, a console will be created/opened */
    ConnectInfo.IsConsoleApp = IsConsoleApp();

    /* Do nothing if this is not a console app... */
    if (!ConnectInfo.IsConsoleApp)
    {
        DPRINT("Image is not a console application\n");
    }

    /*
     * Handle the special flags given to us by BasePushProcessParameters.
     */
    if (Parameters->ConsoleHandle == HANDLE_DETACHED_PROCESS)
    {
        /* No console to create */
        DPRINT("No console to create\n");
        /*
         * The new process does not inherit its parent's console and cannot
         * attach to the console of its parent. The new process can call the
         * AllocConsole function at a later time to create a console.
         */
        Parameters->ConsoleHandle = NULL;  // Do not inherit the parent's console.
        ConnectInfo.IsConsoleApp  = FALSE; // Do not create any console.
    }
    else if (Parameters->ConsoleHandle == HANDLE_CREATE_NEW_CONSOLE)
    {
        /* We'll get the real one soon */
        DPRINT("Creating a new separate console\n");
        /*
         * The new process has a new console, instead of inheriting
         * its parent's console.
         */
        Parameters->ConsoleHandle = NULL; // Do not inherit the parent's console.
    }
    else if (Parameters->ConsoleHandle == HANDLE_CREATE_NO_WINDOW)
    {
        /* We'll get the real one soon */
        DPRINT("Creating a new invisible console\n");
        /*
         * The process is a console application that is being run
         * without a console window. Therefore, the console handle
         * for the application is not set.
         */
        Parameters->ConsoleHandle   = NULL;  // Do not inherit the parent's console.
        ConnectInfo.IsWindowVisible = FALSE; // A console is created but is not shown to the user.
    }
    else
    {
        DPRINT("Using existing console: 0x%p\n", Parameters->ConsoleHandle);
    }

    /* Do nothing if this is not a console app... */
    if (!ConnectInfo.IsConsoleApp)
    {
        /* Do not inherit the parent's console if we are not a console app */
        Parameters->ConsoleHandle = NULL;
    }

    /* Now use the proper console handle */
    ConnectInfo.ConsoleStartInfo.ConsoleHandle = Parameters->ConsoleHandle;

    /* Initialize the console dispatchers */
    ConnectInfo.CtrlRoutine = ConsoleControlDispatcher;
    ConnectInfo.PropRoutine = PropDialogHandler;
    // ConnectInfo.ImeRoutine  = ImeRoutine;

    /* Set up the console properties */
    if (ConnectInfo.IsConsoleApp && Parameters->ConsoleHandle == NULL)
    {
        /*
         * We can set up the console properties only if we create a new one
         * (we do not inherit it from our parent).
         */

        LPWSTR ConsoleTitle = ConnectInfo.ConsoleTitle;

        ConnectInfo.TitleLength   = sizeof(ConnectInfo.ConsoleTitle);
        ConnectInfo.DesktopLength = 0; // SetUpConsoleInfo will give us the real length.

        SetUpConsoleInfo(TRUE,
                         &ConnectInfo.TitleLength,
                         &ConsoleTitle,
                         &ConnectInfo.DesktopLength,
                         &ConnectInfo.Desktop,
                         &ConnectInfo.ConsoleStartInfo);
        DPRINT("ConsoleTitle = '%S' - Desktop = '%S'\n",
               ConsoleTitle, ConnectInfo.Desktop);
    }
    else
    {
        ConnectInfo.TitleLength   = 0;
        ConnectInfo.DesktopLength = 0;
    }

    /* Initialize the Input EXE name */
    if (ConnectInfo.IsConsoleApp)
    {
        LPWSTR CurDir  = ConnectInfo.CurDir;
        LPWSTR AppName = ConnectInfo.AppName;

        InitExeName();

        ConnectInfo.CurDirLength  = sizeof(ConnectInfo.CurDir);
        ConnectInfo.AppNameLength = sizeof(ConnectInfo.AppName);

        SetUpAppName(TRUE,
                     &ConnectInfo.CurDirLength,
                     &CurDir,
                     &ConnectInfo.AppNameLength,
                     &AppName);
        DPRINT("CurDir = '%S' - AppName = '%S'\n",
               CurDir, AppName);
    }
    else
    {
        ConnectInfo.CurDirLength  = 0;
        ConnectInfo.AppNameLength = 0;
    }

    /*
     * Initialize Console Ctrl Handling, that needs to be supported by
     * all applications, especially because it is used at shutdown.
     */
    InitializeCtrlHandling();

    /* Connect to the Console Server */
    if (!ConnectConsole(SessionDir,
                        &ConnectInfo,
                        &InServerProcess))
    {
        // DPRINT1("Failed to connect to the Console Server (Status %lx)\n", Status);
        return FALSE;
    }

    /* If we are not doing server-to-server init and if this is a console app... */
    if (!InServerProcess && ConnectInfo.IsConsoleApp)
    {
        /* ... set the handles that we got */
        if (Parameters->ConsoleHandle == NULL)
            SetUpHandles(&ConnectInfo.ConsoleStartInfo);

        InputWaitHandle = ConnectInfo.ConsoleStartInfo.InputWaitHandle;

        /* Sets the current console locale for this thread */
        SetTEBLangID(lcid);
    }

    DPRINT("Console setup: 0x%p, 0x%p, 0x%p, 0x%p\n",
           Parameters->ConsoleHandle,
           Parameters->StandardInput,
           Parameters->StandardOutput,
           Parameters->StandardError);

    return TRUE;
}

/* EOF */
