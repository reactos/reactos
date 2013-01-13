/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS Console Server DLL
 * FILE:            win32ss/user/consrv/init.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Hermes Belusca-Maito (hermes.belusca@sfr.fr)
 */

#include "consrv.h"
#include "conio.h"

#define NDEBUG
#include <debug.h>

HINSTANCE ConSrvDllInstance = NULL;
// HANDLE BaseApiPort = NULL;

/* Memory */
HANDLE ConSrvHeap = NULL;          // Our own heap.
// HANDLE BaseSrvSharedHeap = NULL;    // Shared heap with CSR. (CsrSrvSharedSectionHeap)
// PBASE_STATIC_SERVER_DATA BaseStaticServerData = NULL;   // Data that we can share amongst processes. Initialized inside BaseSrvSharedHeap.

// Windows 2k3 tables, adapted from http://j00ru.vexillium.org/csrss_list/api_list.html#Windows_2k3
// plus a little bit of Windows 7. It is for testing purposes. After that I will add stubs.
// Some names are also deduced from the subsystems/win32/csrss/csrsrv/server.c ones.
PCSR_API_ROUTINE ConsoleServerApiDispatchTable[ConsolepMaxApiNumber] =
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
    SrvSetConsoleHistory
};

BOOLEAN ConsoleServerApiServerValidTable[ConsolepMaxApiNumber] =
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

    // FALSE
};

PCHAR ConsoleServerApiNameTable[ConsolepMaxApiNumber] =
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
    "SetConsoleHistory"

    // NULL
};


/*
PCSR_API_ROUTINE Win32CsrApiDefinitions[] =
{
    CsrGetHandle,
    CsrGetHandle,
    CsrCloseHandle,
    CsrVerifyHandle,
    CsrDuplicateHandle,
    CsrGetInputWaitHandle,
    CsrFillOutputChar,
    CsrReadInputEvent,
    CsrWriteConsoleOutputChar,
    CsrWriteConsoleOutputAttrib,
    CsrFillOutputAttrib,
    CsrSetTextAttrib,
    CsrWriteConsoleOutput,
    CsrFlushInputBuffer,
    CsrReadConsoleOutputChar,
    CsrReadConsoleOutputAttrib,
    CsrExitReactos,
    CsrHardwareStateProperty,
    CsrCreateDesktop,
    CsrShowDesktop,
    CsrHideDesktop,
    CsrSetLogonNotifyWindow,
    CsrRegisterLogonProcess,
    CsrGenerateCtrlEvent,
};

static CSRSS_API_DEFINITION Win32CsrApiDefinitions[] =
{
    CSRSS_DEFINE_API(GET_INPUT_HANDLE,             CsrGetHandle),
    CSRSS_DEFINE_API(GET_OUTPUT_HANDLE,            CsrGetHandle),
    CSRSS_DEFINE_API(CLOSE_HANDLE,                 CsrCloseHandle),
    CSRSS_DEFINE_API(VERIFY_HANDLE,                CsrVerifyHandle),
    CSRSS_DEFINE_API(DUPLICATE_HANDLE,             CsrDuplicateHandle),
    CSRSS_DEFINE_API(GET_INPUT_WAIT_HANDLE,        CsrGetInputWaitHandle),
    CSRSS_DEFINE_API(WRITE_CONSOLE,                CsrWriteConsole),
    CSRSS_DEFINE_API(READ_CONSOLE,                 CsrReadConsole),
    CSRSS_DEFINE_API(ALLOC_CONSOLE,                CsrAllocConsole),
    CSRSS_DEFINE_API(FREE_CONSOLE,                 CsrFreeConsole),
    CSRSS_DEFINE_API(SCREEN_BUFFER_INFO,           CsrGetScreenBufferInfo),
    CSRSS_DEFINE_API(SET_CURSOR,                   CsrSetCursor),
    CSRSS_DEFINE_API(FILL_OUTPUT,                  CsrFillOutputChar),
    CSRSS_DEFINE_API(READ_INPUT,                   CsrReadInputEvent),
    CSRSS_DEFINE_API(WRITE_CONSOLE_OUTPUT_CHAR,    CsrWriteConsoleOutputChar),
    CSRSS_DEFINE_API(WRITE_CONSOLE_OUTPUT_ATTRIB,  CsrWriteConsoleOutputAttrib),
    CSRSS_DEFINE_API(FILL_OUTPUT_ATTRIB,           CsrFillOutputAttrib),
    CSRSS_DEFINE_API(GET_CURSOR_INFO,              CsrGetCursorInfo),
    CSRSS_DEFINE_API(SET_CURSOR_INFO,              CsrSetCursorInfo),
    CSRSS_DEFINE_API(SET_ATTRIB,                   CsrSetTextAttrib),
    CSRSS_DEFINE_API(GET_CONSOLE_MODE,             CsrGetConsoleMode),
    CSRSS_DEFINE_API(SET_CONSOLE_MODE,             CsrSetConsoleMode),
    CSRSS_DEFINE_API(CREATE_SCREEN_BUFFER,         CsrCreateScreenBuffer),
    CSRSS_DEFINE_API(SET_SCREEN_BUFFER,            CsrSetScreenBuffer),
    CSRSS_DEFINE_API(SET_TITLE,                    CsrSetTitle),
    CSRSS_DEFINE_API(GET_TITLE,                    CsrGetTitle),
    CSRSS_DEFINE_API(WRITE_CONSOLE_OUTPUT,         CsrWriteConsoleOutput),
    CSRSS_DEFINE_API(FLUSH_INPUT_BUFFER,           CsrFlushInputBuffer),
    CSRSS_DEFINE_API(SCROLL_CONSOLE_SCREEN_BUFFER, CsrScrollConsoleScreenBuffer),
    CSRSS_DEFINE_API(READ_CONSOLE_OUTPUT_CHAR,     CsrReadConsoleOutputChar),
    CSRSS_DEFINE_API(READ_CONSOLE_OUTPUT_ATTRIB,   CsrReadConsoleOutputAttrib),
    CSRSS_DEFINE_API(GET_NUM_INPUT_EVENTS,         CsrGetNumberOfConsoleInputEvents),
    CSRSS_DEFINE_API(EXIT_REACTOS,                 CsrExitReactos),
    CSRSS_DEFINE_API(PEEK_CONSOLE_INPUT,           CsrPeekConsoleInput),
    CSRSS_DEFINE_API(READ_CONSOLE_OUTPUT,          CsrReadConsoleOutput),
    CSRSS_DEFINE_API(WRITE_CONSOLE_INPUT,          CsrWriteConsoleInput),
    CSRSS_DEFINE_API(SETGET_CONSOLE_HW_STATE,      CsrHardwareStateProperty),
    CSRSS_DEFINE_API(GET_CONSOLE_WINDOW,           CsrGetConsoleWindow),
    CSRSS_DEFINE_API(CREATE_DESKTOP,               CsrCreateDesktop),
    CSRSS_DEFINE_API(SHOW_DESKTOP,                 CsrShowDesktop),
    CSRSS_DEFINE_API(HIDE_DESKTOP,                 CsrHideDesktop),
    CSRSS_DEFINE_API(SET_CONSOLE_ICON,             CsrSetConsoleIcon),
    CSRSS_DEFINE_API(SET_LOGON_NOTIFY_WINDOW,      CsrSetLogonNotifyWindow),
    CSRSS_DEFINE_API(REGISTER_LOGON_PROCESS,       CsrRegisterLogonProcess),
    CSRSS_DEFINE_API(GET_CONSOLE_CP,               CsrGetConsoleCodePage),
    CSRSS_DEFINE_API(SET_CONSOLE_CP,               CsrSetConsoleCodePage),
    CSRSS_DEFINE_API(GET_CONSOLE_OUTPUT_CP,        CsrGetConsoleOutputCodePage),
    CSRSS_DEFINE_API(SET_CONSOLE_OUTPUT_CP,        CsrSetConsoleOutputCodePage),
    CSRSS_DEFINE_API(GET_PROCESS_LIST,             CsrGetProcessList),
    CSRSS_DEFINE_API(ADD_CONSOLE_ALIAS,      CsrAddConsoleAlias),
    CSRSS_DEFINE_API(GET_CONSOLE_ALIAS,      CsrGetConsoleAlias),
    CSRSS_DEFINE_API(GET_ALL_CONSOLE_ALIASES,         CsrGetAllConsoleAliases),
    CSRSS_DEFINE_API(GET_ALL_CONSOLE_ALIASES_LENGTH,  CsrGetAllConsoleAliasesLength),
    CSRSS_DEFINE_API(GET_CONSOLE_ALIASES_EXES,        CsrGetConsoleAliasesExes),
    CSRSS_DEFINE_API(GET_CONSOLE_ALIASES_EXES_LENGTH, CsrGetConsoleAliasesExesLength),
    CSRSS_DEFINE_API(GENERATE_CTRL_EVENT,          CsrGenerateCtrlEvent),
    CSRSS_DEFINE_API(SET_SCREEN_BUFFER_SIZE,       CsrSetScreenBufferSize),
    CSRSS_DEFINE_API(GET_CONSOLE_SELECTION_INFO,   CsrGetConsoleSelectionInfo),
    CSRSS_DEFINE_API(GET_COMMAND_HISTORY_LENGTH,   CsrGetCommandHistoryLength),
    CSRSS_DEFINE_API(GET_COMMAND_HISTORY,          CsrGetCommandHistory),
    CSRSS_DEFINE_API(EXPUNGE_COMMAND_HISTORY,      CsrExpungeCommandHistory),
    CSRSS_DEFINE_API(SET_HISTORY_NUMBER_COMMANDS,  CsrSetHistoryNumberCommands),
    CSRSS_DEFINE_API(GET_HISTORY_INFO,             CsrGetHistoryInfo),
    CSRSS_DEFINE_API(SET_HISTORY_INFO,             CsrSetHistoryInfo),
    { 0, 0, NULL }
};
*/


/* FUNCTIONS ******************************************************************/

/*
VOID WINAPI ConsoleStaticServerThread(PVOID x)
{
    // NTSTATUS Status = STATUS_SUCCESS;
    PPORT_MESSAGE Request = (PPORT_MESSAGE)x;
    PPORT_MESSAGE Reply = NULL;
    ULONG MessageType = 0;

    DPRINT("BASESRV: %s called\n", __FUNCTION__);

    MessageType = Request->u2.s2.Type;
    DPRINT("BASESRV: %s received a message (Type=%d)\n",
           __FUNCTION__, MessageType);
    switch (MessageType)
    {
        default:
            Reply = Request;
            /\* Status = *\/ NtReplyPort(BaseApiPort, Reply);
            break;
    }
}
*/

CSR_SERVER_DLL_INIT(ConServerDllInitialization)
{
/*
    NTSTATUS Status = STATUS_SUCCESS;

    DPRINT("BASSRV: %s(%ld,...) called\n", __FUNCTION__, ArgumentCount);

    // Get the listening port from csrsrv.dll
    BaseApiPort = CsrQueryApiPort();
    if (BaseApiPort == NULL) return STATUS_UNSUCCESSFUL;

    // Register our message dispatcher
    Status = CsrAddStaticServerThread(ConsoleStaticServerThread);
    if (NT_SUCCESS(Status))
    {
        //TODO initialize the BASE server
    }
    return STATUS_SUCCESS;
*/

    /* Initialize memory */
    ConSrvHeap = RtlGetProcessHeap();  // Initialize our own heap.
    // BaseSrvSharedHeap = LoadedServerDll->SharedSection; // Get the CSR shared heap.
    // LoadedServerDll->SharedSection = BaseStaticServerData;

    CsrInitConsoleSupport();

    /* Setup the DLL Object */
    LoadedServerDll->ApiBase = CONSRV_FIRST_API_NUMBER;
    LoadedServerDll->HighestApiSupported = ConsolepMaxApiNumber;
    LoadedServerDll->DispatchTable = ConsoleServerApiDispatchTable;
    LoadedServerDll->ValidTable = ConsoleServerApiServerValidTable;
    LoadedServerDll->NameTable = ConsoleServerApiNameTable;
    LoadedServerDll->SizeOfProcessData = sizeof(CONSOLE_PROCESS_DATA);
    LoadedServerDll->ConnectCallback = ConsoleConnect;
    LoadedServerDll->DisconnectCallback = ConsoleDisconnect;
    LoadedServerDll->NewProcessCallback = ConsoleNewProcess;
    // LoadedServerDll->HardErrorCallback = Win32CsrHardError;

    /* All done */
    return STATUS_SUCCESS;
}

BOOL
WINAPI
DllMain(IN HINSTANCE hInstanceDll,
        IN DWORD dwReason,
        IN LPVOID lpReserved)
{
    UNREFERENCED_PARAMETER(dwReason);
    UNREFERENCED_PARAMETER(lpReserved);

    if (DLL_PROCESS_ATTACH == dwReason)
    {
        ConSrvDllInstance = hInstanceDll;
    }

    return TRUE;
}

/* EOF */
