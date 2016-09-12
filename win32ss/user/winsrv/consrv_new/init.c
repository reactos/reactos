/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/winsrv/consrv_new/init.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "api.h"
#include "procinit.h"
#include "include/conio.h"
#include "include/console.h"
#include "console.h"

#define NDEBUG
#include <debug.h>

/* GLOBALS ********************************************************************/

HINSTANCE ConSrvDllInstance = NULL;

/* Memory */
HANDLE ConSrvHeap = NULL;   // Our own heap.

// Windows Server 2003 table from http://j00ru.vexillium.org/csrss_list/api_list.html#Windows_2k3
// plus a little bit of Windows 7.
PCSR_API_ROUTINE ConsoleServerApiDispatchTable[ConsolepMaxApiNumber - CONSRV_FIRST_API_NUMBER] =
{
    SrvOpenConsole,
    SrvGetConsoleInput,
    SrvWriteConsoleInput,
    SrvReadConsoleOutput,
    SrvWriteConsoleOutput,
    SrvReadConsoleOutputString,
    SrvWriteConsoleOutputString,
    SrvFillConsoleOutput,
    SrvGetConsoleMode,
    // SrvGetConsoleNumberOfFonts,
    SrvGetConsoleNumberOfInputEvents,
    SrvGetConsoleScreenBufferInfo,
    SrvGetConsoleCursorInfo,
    // SrvGetConsoleMouseInfo,
    // SrvGetConsoleFontInfo,
    // SrvGetConsoleFontSize,
    // SrvGetConsoleCurrentFont,
    SrvSetConsoleMode,
    SrvSetConsoleActiveScreenBuffer,
    SrvFlushConsoleInputBuffer,
    SrvGetLargestConsoleWindowSize,
    SrvSetConsoleScreenBufferSize,
    SrvSetConsoleCursorPosition,
    SrvSetConsoleCursorInfo,
    SrvSetConsoleWindowInfo,
    SrvScrollConsoleScreenBuffer,
    SrvSetConsoleTextAttribute,
    // SrvSetConsoleFont,
    SrvSetConsoleIcon,
    SrvReadConsole,
    SrvWriteConsole,
    SrvDuplicateHandle,
    // SrvGetHandleInformation,
    // SrvSetHandleInformation,
    SrvCloseHandle,
    SrvVerifyConsoleIoHandle,
    SrvAllocConsole,
    SrvFreeConsole,
    SrvGetConsoleTitle,
    SrvSetConsoleTitle,
    SrvCreateConsoleScreenBuffer,
    SrvInvalidateBitMapRect,
    // SrvVDMConsoleOperation,
    SrvSetConsoleCursor,
    SrvShowConsoleCursor,
    SrvConsoleMenuControl,
    // SrvSetConsolePalette,
    SrvSetConsoleDisplayMode,
    // SrvRegisterConsoleVDM,
    SrvGetConsoleHardwareState,
    SrvSetConsoleHardwareState,
    SrvGetConsoleDisplayMode,
    SrvAddConsoleAlias,
    SrvGetConsoleAlias,
    SrvGetConsoleAliasesLength,
    SrvGetConsoleAliasExesLength,
    SrvGetConsoleAliases,
    SrvGetConsoleAliasExes,
    SrvExpungeConsoleCommandHistory,
    SrvSetConsoleNumberOfCommands,
    SrvGetConsoleCommandHistoryLength,
    SrvGetConsoleCommandHistory,
    // SrvSetConsoleCommandHistoryMode,
    SrvGetConsoleCP,
    SrvSetConsoleCP,
    // SrvSetConsoleKeyShortcuts,
    SrvSetConsoleMenuClose,
    // SrvConsoleNotifyLastClose,
    SrvGenerateConsoleCtrlEvent,
    // SrvGetConsoleKeyboardLayoutName,
    SrvGetConsoleWindow,
    // SrvGetConsoleCharType,
    // SrvSetConsoleLocalEUDC,
    // SrvSetConsoleCursorMode,
    // SrvGetConsoleCursorMode,
    // SrvRegisterConsoleOS2,
    // SrvSetConsoleOS2OemFormat,
    // SrvGetConsoleNlsMode,
    // SrvSetConsoleNlsMode,
    // SrvRegisterConsoleIME,
    // SrvUnregisterConsoleIME,
    // SrvGetConsoleLangId,
    SrvAttachConsole,
    SrvGetConsoleSelectionInfo,
    SrvGetConsoleProcessList,
    SrvGetConsoleHistory,
    SrvSetConsoleHistory,
};

BOOLEAN ConsoleServerApiServerValidTable[ConsolepMaxApiNumber - CONSRV_FIRST_API_NUMBER] =
{
    FALSE,   // SrvOpenConsole,
    FALSE,   // SrvGetConsoleInput,
    FALSE,   // SrvWriteConsoleInput,
    FALSE,   // SrvReadConsoleOutput,
    FALSE,   // SrvWriteConsoleOutput,
    FALSE,   // SrvReadConsoleOutputString,
    FALSE,   // SrvWriteConsoleOutputString,
    FALSE,   // SrvFillConsoleOutput,
    FALSE,   // SrvGetConsoleMode,
    // FALSE,   // SrvGetConsoleNumberOfFonts,
    FALSE,   // SrvGetConsoleNumberOfInputEvents,
    FALSE,   // SrvGetConsoleScreenBufferInfo,
    FALSE,   // SrvGetConsoleCursorInfo,
    // FALSE,   // SrvGetConsoleMouseInfo,
    // FALSE,   // SrvGetConsoleFontInfo,
    // FALSE,   // SrvGetConsoleFontSize,
    // FALSE,   // SrvGetConsoleCurrentFont,
    FALSE,   // SrvSetConsoleMode,
    FALSE,   // SrvSetConsoleActiveScreenBuffer,
    FALSE,   // SrvFlushConsoleInputBuffer,
    FALSE,   // SrvGetLargestConsoleWindowSize,
    FALSE,   // SrvSetConsoleScreenBufferSize,
    FALSE,   // SrvSetConsoleCursorPosition,
    FALSE,   // SrvSetConsoleCursorInfo,
    FALSE,   // SrvSetConsoleWindowInfo,
    FALSE,   // SrvScrollConsoleScreenBuffer,
    FALSE,   // SrvSetConsoleTextAttribute,
    // FALSE,   // SrvSetConsoleFont,
    FALSE,   // SrvSetConsoleIcon,
    FALSE,   // SrvReadConsole,
    FALSE,   // SrvWriteConsole,
    FALSE,   // SrvDuplicateHandle,
    // FALSE,   // SrvGetHandleInformation,
    // FALSE,   // SrvSetHandleInformation,
    FALSE,   // SrvCloseHandle,
    FALSE,   // SrvVerifyConsoleIoHandle,
    FALSE,   // SrvAllocConsole,
    FALSE,   // SrvFreeConsole,
    FALSE,   // SrvGetConsoleTitle,
    FALSE,   // SrvSetConsoleTitle,
    FALSE,   // SrvCreateConsoleScreenBuffer,
    FALSE,   // SrvInvalidateBitMapRect,
    // FALSE,   // SrvVDMConsoleOperation,
    FALSE,   // SrvSetConsoleCursor,
    FALSE,   // SrvShowConsoleCursor,
    FALSE,   // SrvConsoleMenuControl,
    // FALSE,   // SrvSetConsolePalette,
    FALSE,   // SrvSetConsoleDisplayMode,
    // FALSE,   // SrvRegisterConsoleVDM,
    FALSE,   // SrvGetConsoleHardwareState,
    FALSE,   // SrvSetConsoleHardwareState,
    TRUE,    // SrvGetConsoleDisplayMode,
    FALSE,   // SrvAddConsoleAlias,
    FALSE,   // SrvGetConsoleAlias,
    FALSE,   // SrvGetConsoleAliasesLength,
    FALSE,   // SrvGetConsoleAliasExesLength,
    FALSE,   // SrvGetConsoleAliases,
    FALSE,   // SrvGetConsoleAliasExes,
    FALSE,   // SrvExpungeConsoleCommandHistory,
    FALSE,   // SrvSetConsoleNumberOfCommands,
    FALSE,   // SrvGetConsoleCommandHistoryLength,
    FALSE,   // SrvGetConsoleCommandHistory,
    // FALSE,   // SrvSetConsoleCommandHistoryMode,
    FALSE,   // SrvGetConsoleCP,
    FALSE,   // SrvSetConsoleCP,
    // FALSE,   // SrvSetConsoleKeyShortcuts,
    FALSE,   // SrvSetConsoleMenuClose,
    // FALSE,   // SrvConsoleNotifyLastClose,
    FALSE,   // SrvGenerateConsoleCtrlEvent,
    // FALSE,   // SrvGetConsoleKeyboardLayoutName,
    FALSE,   // SrvGetConsoleWindow,
    // FALSE,   // SrvGetConsoleCharType,
    // FALSE,   // SrvSetConsoleLocalEUDC,
    // FALSE,   // SrvSetConsoleCursorMode,
    // FALSE,   // SrvGetConsoleCursorMode,
    // FALSE,   // SrvRegisterConsoleOS2,
    // FALSE,   // SrvSetConsoleOS2OemFormat,
    // FALSE,   // SrvGetConsoleNlsMode,
    // FALSE,   // SrvSetConsoleNlsMode,
    // FALSE,   // SrvRegisterConsoleIME,
    // FALSE,   // SrvUnregisterConsoleIME,
    // FALSE,   // SrvGetConsoleLangId,
    FALSE,   // SrvAttachConsole,
    FALSE,   // SrvGetConsoleSelectionInfo,
    FALSE,   // SrvGetConsoleProcessList,
    FALSE,   // SrvGetConsoleHistory,
    FALSE,   // SrvSetConsoleHistory
};

PCHAR ConsoleServerApiNameTable[ConsolepMaxApiNumber - CONSRV_FIRST_API_NUMBER] =
{
    "OpenConsole",
    "GetConsoleInput",
    "WriteConsoleInput",
    "ReadConsoleOutput",
    "WriteConsoleOutput",
    "ReadConsoleOutputString",
    "WriteConsoleOutputString",
    "FillConsoleOutput",
    "GetConsoleMode",
    // "GetConsoleNumberOfFonts",
    "GetConsoleNumberOfInputEvents",
    "GetConsoleScreenBufferInfo",
    "GetConsoleCursorInfo",
    // "GetConsoleMouseInfo",
    // "GetConsoleFontInfo",
    // "GetConsoleFontSize",
    // "GetConsoleCurrentFont",
    "SetConsoleMode",
    "SetConsoleActiveScreenBuffer",
    "FlushConsoleInputBuffer",
    "GetLargestConsoleWindowSize",
    "SetConsoleScreenBufferSize",
    "SetConsoleCursorPosition",
    "SetConsoleCursorInfo",
    "SetConsoleWindowInfo",
    "ScrollConsoleScreenBuffer",
    "SetConsoleTextAttribute",
    // "SetConsoleFont",
    "SetConsoleIcon",
    "ReadConsole",
    "WriteConsole",
    "DuplicateHandle",
    // "GetHandleInformation",
    // "SetHandleInformation",
    "CloseHandle",
    "VerifyConsoleIoHandle",
    "AllocConsole",
    "FreeConsole",
    "GetConsoleTitle",
    "SetConsoleTitle",
    "CreateConsoleScreenBuffer",
    "InvalidateBitMapRect",
    // "VDMConsoleOperation",
    "SetConsoleCursor",
    "ShowConsoleCursor",
    "ConsoleMenuControl",
    // "SetConsolePalette",
    "SetConsoleDisplayMode",
    // "RegisterConsoleVDM",
    "GetConsoleHardwareState",
    "SetConsoleHardwareState",
    "GetConsoleDisplayMode",
    "AddConsoleAlias",
    "GetConsoleAlias",
    "GetConsoleAliasesLength",
    "GetConsoleAliasExesLength",
    "GetConsoleAliases",
    "GetConsoleAliasExes",
    "ExpungeConsoleCommandHistory",
    "SetConsoleNumberOfCommands",
    "GetConsoleCommandHistoryLength",
    "GetConsoleCommandHistory",
    // "SetConsoleCommandHistoryMode",
    "GetConsoleCP",
    "SetConsoleCP",
    // "SetConsoleKeyShortcuts",
    "SetConsoleMenuClose",
    // "ConsoleNotifyLastClose",
    "GenerateConsoleCtrlEvent",
    // "GetConsoleKeyboardLayoutName",
    "GetConsoleWindow",
    // "GetConsoleCharType",
    // "SetConsoleLocalEUDC",
    // "SetConsoleCursorMode",
    // "GetConsoleCursorMode",
    // "RegisterConsoleOS2",
    // "SetConsoleOS2OemFormat",
    // "GetConsoleNlsMode",
    // "SetConsoleNlsMode",
    // "RegisterConsoleIME",
    // "UnregisterConsoleIME",
    // "GetConsoleLangId",
    "AttachConsole",
    "GetConsoleSelectionInfo",
    "GetConsoleProcessList",
    "GetConsoleHistory",
    "SetConsoleHistory",
};


/* FUNCTIONS ******************************************************************/

/* See handle.c */
NTSTATUS
ConSrvInheritHandlesTable(IN PCONSOLE_PROCESS_DATA SourceProcessData,
                          IN PCONSOLE_PROCESS_DATA TargetProcessData);

NTSTATUS
NTAPI
ConSrvNewProcess(PCSR_PROCESS SourceProcess,
                 PCSR_PROCESS TargetProcess)
{
    /**************************************************************************
     * This function is called whenever a new process (GUI or CUI) is created.
     *
     * Copy the parent's handles table here if both the parent and the child
     * processes are CUI. If we must actually create our proper console (and
     * thus do not inherit from the console handles of the parent's), then we
     * will clean this table in the next ConSrvConnect call. Why we are doing
     * this? It's because here, we still don't know whether or not we must create
     * a new console instead of inherit it from the parent, and, because in
     * ConSrvConnect we don't have any reference to the parent process anymore.
     **************************************************************************/

    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_PROCESS_DATA TargetProcessData;

    /* An empty target process is invalid */
    if (!TargetProcess) return STATUS_INVALID_PARAMETER;

    TargetProcessData = ConsoleGetPerProcessData(TargetProcess);

    /* Initialize the new (target) process */
    RtlZeroMemory(TargetProcessData, sizeof(*TargetProcessData));
    TargetProcessData->Process = TargetProcess;
    TargetProcessData->ConsoleEvent = NULL;
    TargetProcessData->ConsoleHandle = TargetProcessData->ParentConsoleHandle = NULL;
    TargetProcessData->ConsoleApp = ((TargetProcess->Flags & CsrProcessIsConsoleApp) ? TRUE : FALSE);

    /*
     * The handles table gets initialized either when inheriting from
     * another console process, or when creating a new console.
     */
    TargetProcessData->HandleTableSize = 0;
    TargetProcessData->HandleTable = NULL;

    RtlInitializeCriticalSection(&TargetProcessData->HandleTableLock);

    /* Do nothing if the source process is NULL */
    if (!SourceProcess) return STATUS_SUCCESS;

    // SourceProcessData = ConsoleGetPerProcessData(SourceProcess);

    /*
     * If the child process is a console application and the parent process is
     * either a console application or just has a valid console (with a valid
     * handles table: this can happen if it is a GUI application having called
     * AllocConsole), then try to inherit handles from the parent process.
     */
    if (TargetProcessData->ConsoleApp /* && SourceProcessData->ConsoleApp */)
    {
        PCONSOLE_PROCESS_DATA SourceProcessData = ConsoleGetPerProcessData(SourceProcess);
        PCONSOLE SourceConsole;

        /* Validate and lock the parent's console */
        if (ConDrvValidateConsole(&SourceConsole,
                                  SourceProcessData->ConsoleHandle,
                                  CONSOLE_RUNNING, TRUE))
        {
            /* Inherit the parent's handles table */
            Status = ConSrvInheritHandlesTable(SourceProcessData, TargetProcessData);
            if (NT_SUCCESS(Status))
            {
                /* Temporary save the parent's console too */
                TargetProcessData->ParentConsoleHandle = SourceProcessData->ConsoleHandle;
            }

            /* Unlock the parent's console */
            LeaveCriticalSection(&SourceConsole->Lock);
        }
    }

    return Status;
}

NTSTATUS
NTAPI
ConSrvConnect(IN PCSR_PROCESS CsrProcess,
              IN OUT PVOID ConnectionInfo,
              IN OUT PULONG ConnectionInfoLength)
{
    /**************************************************************************
     * This function is called whenever a CUI new process is created.
     **************************************************************************/

    NTSTATUS Status = STATUS_SUCCESS;
    PCONSOLE_CONNECTION_INFO ConnectInfo = (PCONSOLE_CONNECTION_INFO)ConnectionInfo;
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(CsrProcess);

    if ( ConnectionInfo       == NULL ||
         ConnectionInfoLength == NULL ||
        *ConnectionInfoLength != sizeof(CONSOLE_CONNECTION_INFO) )
    {
        DPRINT1("CONSRV: Connection failed\n");
        return STATUS_UNSUCCESSFUL;
    }

    /* If we don't need a console, then get out of here */
    if (!ConnectInfo->ConsoleNeeded || !ProcessData->ConsoleApp) // In fact, it is for GUI apps.
    {
        return STATUS_SUCCESS;
    }

    /* If we don't have a console, then create a new one... */
    if (!ConnectInfo->ConsoleHandle ||
         ConnectInfo->ConsoleHandle != ProcessData->ParentConsoleHandle)
    {
        DPRINT("ConSrvConnect - Allocate a new console\n");

        /*
         * We are about to create a new console. However when ConSrvNewProcess
         * was called, we didn't know that we wanted to create a new console and
         * therefore, we by default inherited the handles table from our parent
         * process. It's only now that we notice that in fact we do not need
         * them, because we've created a new console and thus we must use it.
         *
         * ConSrvAllocateConsole will free our old handles table
         * and recreate a new valid one.
         */

        /* Initialize a new Console owned by the Console Leader Process */
        Status = ConSrvAllocateConsole(ProcessData,
                                       &ConnectInfo->InputHandle,
                                       &ConnectInfo->OutputHandle,
                                       &ConnectInfo->ErrorHandle,
                                       &ConnectInfo->ConsoleStartInfo);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Console allocation failed\n");
            return Status;
        }
    }
    else /* We inherit it from the parent */
    {
        DPRINT("ConSrvConnect - Reuse current (parent's) console\n");

        /* Reuse our current console */
        Status = ConSrvInheritConsole(ProcessData,
                                      ConnectInfo->ConsoleHandle,
                                      FALSE,
                                      NULL,  // &ConnectInfo->InputHandle,
                                      NULL,  // &ConnectInfo->OutputHandle,
                                      NULL); // &ConnectInfo->ErrorHandle);
        if (!NT_SUCCESS(Status))
        {
            DPRINT1("Console inheritance failed\n");
            return Status;
        }
    }

    /* Return the console handle and the input wait handle to the caller */
    ConnectInfo->ConsoleHandle   = ProcessData->ConsoleHandle;
    ConnectInfo->InputWaitHandle = ProcessData->ConsoleEvent;

    /* Set the Property-Dialog and Control-Dispatcher handlers */
    ProcessData->PropDispatcher = ConnectInfo->PropDispatcher;
    ProcessData->CtrlDispatcher = ConnectInfo->CtrlDispatcher;

    return STATUS_SUCCESS;
}

VOID
NTAPI
ConSrvDisconnect(PCSR_PROCESS Process)
{
    PCONSOLE_PROCESS_DATA ProcessData = ConsoleGetPerProcessData(Process);

    /**************************************************************************
     * This function is called whenever a new process (GUI or CUI) is destroyed.
     **************************************************************************/

    if ( ProcessData->ConsoleHandle != NULL ||
         ProcessData->HandleTable   != NULL )
    {
        DPRINT("ConSrvDisconnect - calling ConSrvRemoveConsole\n");
        ConSrvRemoveConsole(ProcessData);
    }

    RtlDeleteCriticalSection(&ProcessData->HandleTableLock);
}

CSR_SERVER_DLL_INIT(ConServerDllInitialization)
{
    /* Initialize the memory */
    ConSrvHeap = RtlGetProcessHeap();
/*
    // We can use our own heap instead of the CSR heap to investigate heap corruptions :)
    ConSrvHeap = RtlCreateHeap(HEAP_GROWABLE                |
                               HEAP_PROTECTION_ENABLED      |
                               HEAP_FREE_CHECKING_ENABLED   |
                               HEAP_TAIL_CHECKING_ENABLED   |
                               HEAP_VALIDATE_ALL_ENABLED,
                               NULL, 0, 0, NULL, NULL);
    if (!ConSrvHeap) return STATUS_NO_MEMORY;
*/

    ConDrvInitConsoleSupport();

    /* Setup the DLL Object */
    LoadedServerDll->ApiBase = CONSRV_FIRST_API_NUMBER;
    LoadedServerDll->HighestApiSupported = ConsolepMaxApiNumber;
    LoadedServerDll->DispatchTable = ConsoleServerApiDispatchTable;
    LoadedServerDll->ValidTable = ConsoleServerApiServerValidTable;
    LoadedServerDll->NameTable = ConsoleServerApiNameTable;
    LoadedServerDll->SizeOfProcessData = sizeof(CONSOLE_PROCESS_DATA);
    LoadedServerDll->ConnectCallback = ConSrvConnect;
    LoadedServerDll->DisconnectCallback = ConSrvDisconnect;
    LoadedServerDll->NewProcessCallback = ConSrvNewProcess;
    // LoadedServerDll->HardErrorCallback = ConSrvHardError;
    LoadedServerDll->ShutdownProcessCallback = NULL;

    ConSrvDllInstance = LoadedServerDll->ServerHandle;

    /* All done */
    return STATUS_SUCCESS;
}

/* EOF */
