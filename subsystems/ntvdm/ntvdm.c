/*
 * COPYRIGHT:       GPL - See COPYING in the top level directory
 * PROJECT:         ReactOS Virtual DOS Machine
 * FILE:            ntvdm.c
 * PURPOSE:         Virtual DOS Machine
 * PROGRAMMERS:     Aleksandar Andrejevic <theflash AT sdf DOT lonestar DOT org>
 */

/* INCLUDES *******************************************************************/

#define NDEBUG

#include "ntvdm.h"
#include "emulator.h"

#include "clock.h"
#include "hardware/ps2.h"
#include "hardware/vga.h"
#include "bios/bios.h"
#include "dos/dem.h"

#include "resource.h"

/* VARIABLES ******************************************************************/

static HANDLE ConsoleInput  = INVALID_HANDLE_VALUE;
static HANDLE ConsoleOutput = INVALID_HANDLE_VALUE;
static DWORD  OrgConsoleInputMode, OrgConsoleOutputMode;
static BOOLEAN AcceptCommands = TRUE;
static HANDLE CommandThread = NULL;

static HMENU hConsoleMenu  = NULL;
static INT   VdmMenuPos    = -1;
static BOOLEAN ShowPointer = FALSE;

#ifndef STANDALONE
ULONG SessionId = 0;
#endif

HANDLE VdmTaskEvent = NULL;

/*
 * Those menu helpers were taken from the GUI frontend in winsrv.dll
 */
typedef struct _VDM_MENUITEM
{
    UINT uID;
    const struct _VDM_MENUITEM *SubMenu;
    WORD wCmdID;
} VDM_MENUITEM, *PVDM_MENUITEM;

static const VDM_MENUITEM VdmMenuItems[] =
{
    { IDS_VDM_DUMPMEM, NULL, ID_VDM_DUMPMEM },
    { IDS_VDM_QUIT   , NULL, ID_VDM_QUIT    },

    { 0, NULL, 0 }      /* End of list */
};

static const VDM_MENUITEM VdmMainMenuItems[] =
{
    { -1, NULL, 0 },    /* Separator */
    { IDS_HIDE_MOUSE,   NULL, ID_SHOWHIDE_MOUSE },  /* Hide mouse; can be renamed to Show mouse */
    { IDS_VDM_MENU  ,   VdmMenuItems,         0 },  /* ReactOS VDM Menu */

    { 0, NULL, 0 }      /* End of list */
};

static VOID
AppendMenuItems(HMENU hMenu,
                const VDM_MENUITEM *Items)
{
    UINT i = 0;
    WCHAR szMenuString[255];
    HMENU hSubMenu;

    do
    {
        if (Items[i].uID != (UINT)-1)
        {
            if (LoadStringW(GetModuleHandle(NULL),
                            Items[i].uID,
                            szMenuString,
                            sizeof(szMenuString) / sizeof(szMenuString[0])) > 0)
            {
                if (Items[i].SubMenu != NULL)
                {
                    hSubMenu = CreatePopupMenu();
                    if (hSubMenu != NULL)
                    {
                        AppendMenuItems(hSubMenu, Items[i].SubMenu);

                        if (!AppendMenuW(hMenu,
                                         MF_STRING | MF_POPUP,
                                         (UINT_PTR)hSubMenu,
                                         szMenuString))
                        {
                            DestroyMenu(hSubMenu);
                        }
                    }
                }
                else
                {
                    AppendMenuW(hMenu,
                                MF_STRING,
                                Items[i].wCmdID,
                                szMenuString);
                }
            }
        }
        else
        {
            AppendMenuW(hMenu,
                        MF_SEPARATOR,
                        0,
                        NULL);
        }
        i++;
    } while (!(Items[i].uID == 0 && Items[i].SubMenu == NULL && Items[i].wCmdID == 0));
}

static VOID
CreateVdmMenu(HANDLE ConOutHandle)
{
    hConsoleMenu = ConsoleMenuControl(ConOutHandle,
                                      ID_SHOWHIDE_MOUSE,
                                      ID_VDM_QUIT);
    if (hConsoleMenu == NULL) return;

    VdmMenuPos = GetMenuItemCount(hConsoleMenu);
    AppendMenuItems(hConsoleMenu, VdmMainMenuItems);
    DrawMenuBar(GetConsoleWindow());
}

static VOID
DestroyVdmMenu(VOID)
{
    UINT i = 0;
    const VDM_MENUITEM *Items = VdmMainMenuItems;

    do
    {
        DeleteMenu(hConsoleMenu, VdmMenuPos, MF_BYPOSITION);
        i++;
    } while (!(Items[i].uID == 0 && Items[i].SubMenu == NULL && Items[i].wCmdID == 0));

    DrawMenuBar(GetConsoleWindow());
}

static VOID ShowHideMousePointer(HANDLE ConOutHandle, BOOLEAN ShowPtr)
{
    WCHAR szMenuString[255] = L"";

    if (ShowPtr)
    {
        /* Be sure the cursor will be shown */
        while (ShowConsoleCursor(ConOutHandle, TRUE) < 0) ;
    }
    else
    {
        /* Be sure the cursor will be hidden */
        while (ShowConsoleCursor(ConOutHandle, FALSE) >= 0) ;
    }

    if (LoadStringW(GetModuleHandle(NULL),
                    (!ShowPtr ? IDS_SHOW_MOUSE : IDS_HIDE_MOUSE),
                    szMenuString,
                    sizeof(szMenuString) / sizeof(szMenuString[0])) > 0)
    {
        ModifyMenu(hConsoleMenu, ID_SHOWHIDE_MOUSE,
                   MF_BYCOMMAND, ID_SHOWHIDE_MOUSE, szMenuString);
    }
}

/* PUBLIC FUNCTIONS ***********************************************************/

VOID
DisplayMessage(LPCWSTR Format, ...)
{
    WCHAR Buffer[256];
    va_list Parameters;

    va_start(Parameters, Format);
    _vsnwprintf(Buffer, 256, Format, Parameters);
    DPRINT1("\n\nNTVDM Subsystem\n%S\n\n", Buffer);
    MessageBoxW(NULL, Buffer, L"NTVDM Subsystem", MB_OK);
    va_end(Parameters);
}

static BOOL
WINAPI
ConsoleCtrlHandler(DWORD ControlType)
{
    switch (ControlType)
    {
        case CTRL_C_EVENT:
        case CTRL_BREAK_EVENT:
        {
            /* Call INT 23h */
            EmulatorInterrupt(0x23);
            break;
        }
        case CTRL_LAST_CLOSE_EVENT:
        {
            if (WaitForSingleObject(VdmTaskEvent, 0) == WAIT_TIMEOUT)
            {
                /* Exit immediately */
                if (CommandThread) TerminateThread(CommandThread, 0);
                EmulatorTerminate();
            }
            else
            {
                /* Stop accepting new commands */
                AcceptCommands = FALSE;
            }

            break;
        }
        default:
        {
            /* Stop the VDM if the user logs out or closes the console */
            EmulatorTerminate();
        }
    }
    return TRUE;
}

static VOID
ConsoleInitUI(VOID)
{
    CreateVdmMenu(ConsoleOutput);
}

static VOID
ConsoleCleanupUI(VOID)
{
    /* Display again properly the mouse pointer */
    if (ShowPointer) ShowHideMousePointer(ConsoleOutput, ShowPointer);

    DestroyVdmMenu();
}

static BOOL
ConsoleAttach(VOID)
{
    /* Save the original input and output console modes */
    if (!GetConsoleMode(ConsoleInput , &OrgConsoleInputMode ) ||
        !GetConsoleMode(ConsoleOutput, &OrgConsoleOutputMode))
    {
        CloseHandle(ConsoleOutput);
        CloseHandle(ConsoleInput);
        wprintf(L"FATAL: Cannot save console in/out modes\n");
        // return FALSE;
    }

    /* Initialize the UI */
    ConsoleInitUI();

    return TRUE;
}

static VOID
ConsoleDetach(VOID)
{
    /* Restore the original input and output console modes */
    SetConsoleMode(ConsoleOutput, OrgConsoleOutputMode);
    SetConsoleMode(ConsoleInput , OrgConsoleInputMode );

    /* Cleanup the UI */
    ConsoleCleanupUI();
}

static BOOL
ConsoleInit(VOID)
{
    /* Set the handler routine */
    SetConsoleCtrlHandler(ConsoleCtrlHandler, TRUE);

    /* Enable the CTRL_LAST_CLOSE_EVENT */
    SetLastConsoleEventActive();

    /*
     * NOTE: The CONIN$ and CONOUT$ "virtual" files
     * always point to non-redirected console handles.
     */

    /* Get the input handle to the real console, and check for success */
    ConsoleInput = CreateFileW(L"CONIN$",
                               GENERIC_READ | GENERIC_WRITE,
                               FILE_SHARE_READ | FILE_SHARE_WRITE,
                               NULL,
                               OPEN_EXISTING,
                               0,
                               NULL);
    if (ConsoleInput == INVALID_HANDLE_VALUE)
    {
        wprintf(L"FATAL: Cannot retrieve a handle to the console input\n");
        return FALSE;
    }

    /* Get the output handle to the real console, and check for success */
    ConsoleOutput = CreateFileW(L"CONOUT$",
                                GENERIC_READ | GENERIC_WRITE,
                                FILE_SHARE_READ | FILE_SHARE_WRITE,
                                NULL,
                                OPEN_EXISTING,
                                0,
                                NULL);
    if (ConsoleOutput == INVALID_HANDLE_VALUE)
    {
        CloseHandle(ConsoleInput);
        wprintf(L"FATAL: Cannot retrieve a handle to the console output\n");
        return FALSE;
    }

    /* Effectively attach to the console */
    return ConsoleAttach();
}

static VOID
ConsoleCleanup(VOID)
{
    /* Detach from the console */
    ConsoleDetach();

    /* Close the console handles */
    if (ConsoleOutput != INVALID_HANDLE_VALUE) CloseHandle(ConsoleOutput);
    if (ConsoleInput  != INVALID_HANDLE_VALUE) CloseHandle(ConsoleInput);
}

DWORD
WINAPI
PumpConsoleInput(LPVOID Parameter)
{
    HANDLE ConsoleInput = (HANDLE)Parameter;
    INPUT_RECORD InputRecord;
    DWORD Count;

    while (VdmRunning)
    {
        /* Make sure the task event is signaled */
        WaitForSingleObject(VdmTaskEvent, INFINITE);

        /* Wait for an input record */
        if (!ReadConsoleInput(ConsoleInput, &InputRecord, 1, &Count))
        {
            DWORD LastError = GetLastError();
            DPRINT1("Error reading console input (0x%p, %lu) - Error %lu\n", ConsoleInput, Count, LastError);
            return LastError;
        }

        ASSERT(Count != 0);

        /* Check the event type */
        switch (InputRecord.EventType)
        {
            case KEY_EVENT:
            case MOUSE_EVENT:
                /* Send it to the PS/2 controller */
                PS2Dispatch(&InputRecord);
                break;

            case MENU_EVENT:
            {
                switch (InputRecord.Event.MenuEvent.dwCommandId)
                {
                    case ID_SHOWHIDE_MOUSE:
                        ShowHideMousePointer(ConsoleOutput, ShowPointer);
                        ShowPointer = !ShowPointer;
                        break;

                    case ID_VDM_DUMPMEM:
                        DumpMemory();
                        break;

                    case ID_VDM_QUIT:
                        /* Stop the VDM */
                        EmulatorTerminate();
                        break;

                    default:
                        break;
                }

                break;
            }

            default:
                break;
        }
    }

    return 0;
}

#ifndef STANDALONE
static DWORD
WINAPI
CommandThreadProc(LPVOID Parameter)
{
    BOOLEAN First = TRUE;
    DWORD Result;
    VDM_COMMAND_INFO CommandInfo;
    CHAR CmdLine[MAX_PATH];
    CHAR AppName[MAX_PATH];
    CHAR PifFile[MAX_PATH];
    CHAR Desktop[MAX_PATH];
    CHAR Title[MAX_PATH];
    ULONG EnvSize = 256;
    PVOID Env = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, EnvSize);

    UNREFERENCED_PARAMETER(Parameter);
    ASSERT(Env != NULL);

    do
    {
        /* Clear the structure */
        ZeroMemory(&CommandInfo, sizeof(CommandInfo));

        /* Initialize the structure members */
        CommandInfo.TaskId = SessionId;
        CommandInfo.VDMState = VDM_FLAG_DOS;
        CommandInfo.CmdLine = CmdLine;
        CommandInfo.CmdLen = sizeof(CmdLine);
        CommandInfo.AppName = AppName;
        CommandInfo.AppLen = sizeof(AppName);
        CommandInfo.PifFile = PifFile;
        CommandInfo.PifLen = sizeof(PifFile);
        CommandInfo.Desktop = Desktop;
        CommandInfo.DesktopLen = sizeof(Desktop);
        CommandInfo.Title = Title;
        CommandInfo.TitleLen = sizeof(Title);
        CommandInfo.Env = Env;
        CommandInfo.EnvLen = EnvSize;

        if (First) CommandInfo.VDMState |= VDM_FLAG_FIRST_TASK;

Command:
        if (!GetNextVDMCommand(&CommandInfo))
        {
            if (CommandInfo.EnvLen > EnvSize)
            {
                /* Expand the environment size */
                EnvSize = CommandInfo.EnvLen;
                Env = HeapReAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, Env, EnvSize);

                /* Repeat the request */
                goto Command;
            }

            break;
        }

        /* Start the process from the command line */
        DPRINT1("Starting '%s' ('%s')...\n", AppName, CmdLine);
        Result = DosStartProcess(AppName, CmdLine, Env);
        if (Result != ERROR_SUCCESS)
        {
            DisplayMessage(L"Could not start '%S'. Error: %u", AppName, Result);
            // break;
            continue;
        }

        First = FALSE;
    }
    while (AcceptCommands);

    HeapFree(GetProcessHeap(), 0, Env);
    return 0;
}
#endif

INT
wmain(INT argc, WCHAR *argv[])
{
#ifdef STANDALONE

    DWORD Result;
    CHAR ApplicationName[MAX_PATH];
    CHAR CommandLine[DOS_CMDLINE_LENGTH];

    if (argc >= 2)
    {
        WideCharToMultiByte(CP_ACP, 0, argv[1], -1, ApplicationName, sizeof(ApplicationName), NULL, NULL);

        if (argc >= 3) WideCharToMultiByte(CP_ACP, 0, argv[2], -1, CommandLine, sizeof(CommandLine), NULL, NULL);
        else strcpy(CommandLine, "");
    }
    else
    {
        wprintf(L"\nReactOS Virtual DOS Machine\n\n"
                L"Usage: NTVDM <executable> [<parameters>]\n");
        return 0;
    }

#else

    INT i;
    WCHAR *endptr;

    /* Parse the command line arguments */
    for (i = 1; i < argc; i++)
    {
        if (wcsncmp(argv[i], L"-i", 2) == 0)
        {
            /* This is the session ID */
            SessionId = wcstoul(argv[i] + 2, &endptr, 10);

            /* The VDM hasn't been started from a console, so quit when the task is done */
            AcceptCommands = FALSE;
        }
    }

#endif

    DPRINT1("\n\n\nNTVDM - Starting...\n\n\n");

    /* Create the task event */
    VdmTaskEvent = CreateEvent(NULL, TRUE, FALSE, NULL);
    ASSERT(VdmTaskEvent != NULL);

    /* Initialize the console */
    if (!ConsoleInit())
    {
        wprintf(L"FATAL: A problem occurred when trying to initialize the console\n");
        goto Cleanup;
    }

    /* Initialize the emulator */
    if (!EmulatorInitialize(ConsoleInput, ConsoleOutput))
    {
        wprintf(L"FATAL: Failed to initialize the emulator\n");
        goto Cleanup;
    }

    /* Initialize the system BIOS */
    if (!BiosInitialize(NULL))
    {
        wprintf(L"FATAL: Failed to initialize the VDM BIOS.\n");
        goto Cleanup;
    }

    /* Initialize the VDM DOS kernel */
    if (!DosInitialize(NULL))
    {
        wprintf(L"FATAL: Failed to initialize the VDM DOS kernel.\n");
        goto Cleanup;
    }

#ifndef STANDALONE

    /* Create the GetNextVDMCommand thread */
    CommandThread = CreateThread(NULL, 0, &CommandThreadProc, NULL, 0, NULL);
    if (CommandThread == NULL)
    {
        wprintf(L"FATAL: Failed to create the command processing thread: %d\n", GetLastError());
        goto Cleanup;
    }

    /* Wait for the command thread to exit */
    WaitForSingleObject(CommandThread, INFINITE);

    /* Close the thread handle */
    CloseHandle(CommandThread);

#else

    /* Start the process from the command line */
    DPRINT1("Starting '%s' ('%s')...\n", ApplicationName, CommandLine);
    Result = DosStartProcess(ApplicationName,
                             CommandLine,
                             GetEnvironmentStrings());
    if (Result != ERROR_SUCCESS)
    {
        DisplayMessage(L"Could not start '%S'. Error: %u", ApplicationName, Result);
        goto Cleanup;
    }

#endif

Cleanup:
    BiosCleanup();
    EmulatorCleanup();
    ConsoleCleanup();

#ifndef STANDALONE
    ExitVDM(FALSE, 0);
#endif

    /* Quit the VDM */
    DPRINT1("\n\n\nNTVDM - Exiting...\n\n\n");

    return 0;
}

/* EOF */
