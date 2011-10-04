/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/dllmain.c
 * PURPOSE:         Initialization
 * PROGRAMMERS:     Dmitry Philippov (shedon@mail.ru)
 */

/* INCLUDES ******************************************************************/
#define NDEBUG
#include "w32csr.h"
#include "file.h"
#include <debug.h>

/* Not defined in any header file */
extern VOID WINAPI PrivateCsrssManualGuiCheck(LONG Check);
extern LIST_ENTRY DosDeviceHistory;
extern RTL_CRITICAL_SECTION Win32CsrDefineDosDeviceCritSec;

/* GLOBALS *******************************************************************/

HANDLE Win32CsrApiHeap;
HINSTANCE Win32CsrDllHandle = NULL;
static CSRSS_EXPORTED_FUNCS CsrExports;

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
    CSRSS_DEFINE_API(GET_TEMP_FILE,                CsrGetTempFile),
    CSRSS_DEFINE_API(DEFINE_DOS_DEVICE,            CsrDefineDosDevice),
    { 0, 0, NULL }
};

static HHOOK hhk = NULL;

/* FUNCTIONS *****************************************************************/

LRESULT
CALLBACK
KeyboardHookProc(
    int nCode,
    WPARAM wParam,
    LPARAM lParam)
{
   return CallNextHookEx(hhk, nCode, wParam, lParam);
}

BOOL WINAPI
DllMain(HANDLE hDll,
        DWORD dwReason,
        LPVOID lpReserved)
{
    if (DLL_PROCESS_ATTACH == dwReason)
    {
        Win32CsrDllHandle = hDll;
//
// HACK HACK HACK ReactOS to BOOT! Initialization BUG ALERT! See bug 5655.
//
        hhk = SetWindowsHookEx(WH_KEYBOARD_LL, KeyboardHookProc, NULL, 0);
// BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT!
//  BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT!
//   BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT! BUG ALERT!
    }

    if (DLL_PROCESS_DETACH == dwReason)
    {
        CsrCleanupDefineDosDevice();
    }
    return TRUE;
}

/* Ensure that a captured buffer is safe to access */
BOOL FASTCALL
Win32CsrValidateBuffer(PCSRSS_PROCESS_DATA ProcessData, PVOID Buffer,
                       SIZE_T NumElements, SIZE_T ElementSize)
{
    /* Check that the following conditions are true:
     * 1. The start of the buffer is somewhere within the process's
     *    shared memory section view.
     * 2. The remaining space in the view is at least as large as the buffer.
     *    (NB: Please don't try to "optimize" this by using multiplication
     *    instead of division; remember that 2147483648 * 2 = 0.)
     * 3. The buffer is DWORD-aligned.
     */
    ULONG_PTR Offset = (BYTE *)Buffer - (BYTE *)ProcessData->CsrSectionViewBase;
    if (Offset >= ProcessData->CsrSectionViewSize
            || NumElements > (ProcessData->CsrSectionViewSize - Offset) / ElementSize
            || (Offset & (sizeof(DWORD) - 1)) != 0)
    {
        DPRINT1("Invalid buffer %p(%u*%u); section view is %p(%u)\n",
                Buffer, NumElements, ElementSize,
                ProcessData->CsrSectionViewBase, ProcessData->CsrSectionViewSize);
        return FALSE;
    }
    return TRUE;
}

NTSTATUS FASTCALL
Win32CsrEnumProcesses(CSRSS_ENUM_PROCESS_PROC EnumProc,
                      PVOID Context)
{
    return (CsrExports.CsrEnumProcessesProc)(EnumProc, Context);
}

static BOOL WINAPI
Win32CsrInitComplete(void)
{
    return TRUE;
}

VOID
WINAPI
PrivateCsrssManualGuiCheck(LONG Check)
{
  NtUserCallOneParam(Check, ONEPARAM_ROUTINE_CSRSS_GUICHECK);
}

BOOL WINAPI
Win32CsrInitialization(PCSRSS_API_DEFINITION *ApiDefinitions,
                       PCSRPLUGIN_SERVER_PROCS ServerProcs,
                       PCSRSS_EXPORTED_FUNCS Exports,
                       HANDLE CsrssApiHeap)
{
    CsrExports = *Exports;
    Win32CsrApiHeap = CsrssApiHeap;

    NtUserInitialize(0, NULL, NULL);

    PrivateCsrssManualGuiCheck(0);
    CsrInitConsoleSupport();

    *ApiDefinitions = Win32CsrApiDefinitions;
    ServerProcs->InitCompleteProc = Win32CsrInitComplete;
    ServerProcs->HardErrorProc = Win32CsrHardError;
    ServerProcs->ProcessInheritProc = Win32CsrDuplicateHandleTable;
    ServerProcs->ProcessDeletedProc = Win32CsrReleaseConsole;

    RtlInitializeCriticalSection(&Win32CsrDefineDosDeviceCritSec);
    InitializeListHead(&DosDeviceHistory);
    return TRUE;
}

/* EOF */
