/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/init.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

/* INCLUDES *******************************************************************/

#include "consrv.h"
#include "conio.h"
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
    // SrvGetLargestConsoleWindowSize,
    SrvSetConsoleScreenBufferSize,
    SrvSetConsoleCursorPosition,
    SrvSetConsoleCursorInfo,
    // SrvSetConsoleWindowInfo,
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
    // SrvInvalidateBitMapRect,
    // SrvVDMConsoleOperation,
    // SrvSetConsoleCursor,
    // SrvShowConsoleCursor,
    // SrvConsoleMenuControl,
    // SrvSetConsolePalette,
    // SrvSetConsoleDisplayMode,
    // SrvRegisterConsoleVDM,
    SrvGetConsoleHardwareState,
    SrvSetConsoleHardwareState,
    // SrvGetConsoleDisplayMode,
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
    // SrvSetConsoleMenuClose,
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
    // FALSE,   // SrvGetLargestConsoleWindowSize,
    FALSE,   // SrvSetConsoleScreenBufferSize,
    FALSE,   // SrvSetConsoleCursorPosition,
    FALSE,   // SrvSetConsoleCursorInfo,
    // FALSE,   // SrvSetConsoleWindowInfo,
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
    // FALSE,   // SrvInvalidateBitMapRect,
    // FALSE,   // SrvVDMConsoleOperation,
    // FALSE,   // SrvSetConsoleCursor,
    // FALSE,   // SrvShowConsoleCursor,
    // FALSE,   // SrvConsoleMenuControl,
    // FALSE,   // SrvSetConsolePalette,
    // FALSE,   // SrvSetConsoleDisplayMode,
    // FALSE,   // SrvRegisterConsoleVDM,
    FALSE,   // SrvGetConsoleHardwareState,
    FALSE,   // SrvSetConsoleHardwareState,
    // TRUE,    // SrvGetConsoleDisplayMode,
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
    // FALSE,   // SrvSetConsoleMenuClose,
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
    // "GetLargestConsoleWindowSize",
    "SetConsoleScreenBufferSize",
    "SetConsoleCursorPosition",
    "SetConsoleCursorInfo",
    // "SetConsoleWindowInfo",
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
    // "InvalidateBitMapRect",
    // "VDMConsoleOperation",
    // "SetConsoleCursor",
    // "ShowConsoleCursor",
    // "ConsoleMenuControl",
    // "SetConsolePalette",
    // "SetConsoleDisplayMode",
    // "RegisterConsoleVDM",
    "GetConsoleHardwareState",
    "SetConsoleHardwareState",
    // "GetConsoleDisplayMode",
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
    // "SetConsoleMenuClose",
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

CSR_SERVER_DLL_INIT(ConServerDllInitialization)
{
    /* Initialize the memory */
    ConSrvHeap = RtlGetProcessHeap();

    ConSrvInitConsoleSupport();

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

BOOL
WINAPI
DllMain(IN HINSTANCE hInstanceDll,
        IN DWORD dwReason,
        IN LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(hInstanceDll);
    UNREFERENCED_PARAMETER(dwReason);
    UNREFERENCED_PARAMETER(lpReserved);
    return TRUE;
}

/* EOF */
