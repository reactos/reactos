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
#include <debug.h>

/* Not defined in any header file */
extern VOID WINAPI PrivateCsrssManualGuiCheck(LONG Check);
extern VOID WINAPI PrivateCsrssInitialized();
extern VOID WINAPI InitializeAppSwitchHook();

/* GLOBALS *******************************************************************/

HANDLE Win32CsrApiHeap;
HINSTANCE Win32CsrDllHandle = NULL;
static CSRSS_EXPORTED_FUNCS CsrExports;

static CSRSS_API_DEFINITION Win32CsrApiDefinitions[] =
  {
    CSRSS_DEFINE_API(GET_INPUT_HANDLE,             CsrGetInputHandle),
    CSRSS_DEFINE_API(GET_OUTPUT_HANDLE,            CsrGetOutputHandle),
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
    { 0, 0, NULL }
  };

static CSRSS_OBJECT_DEFINITION Win32CsrObjectDefinitions[] =
  {
    { CONIO_CONSOLE_MAGIC,       ConioDeleteConsole },
    { CONIO_SCREEN_BUFFER_MAGIC, ConioDeleteScreenBuffer },
    { 0,                         NULL }
  };

/* FUNCTIONS *****************************************************************/

BOOL WINAPI
DllMain(HANDLE hDll,
	DWORD dwReason,
	LPVOID lpReserved)
{
  if (DLL_PROCESS_ATTACH == dwReason)
    {
      Win32CsrDllHandle = hDll;
      InitializeAppSwitchHook();
    }

  return TRUE;
}

NTSTATUS FASTCALL
Win32CsrInsertObject(PCSRSS_PROCESS_DATA ProcessData,
                      PHANDLE Handle,
                      Object_t *Object,
                      DWORD Access,
                      BOOL Inheritable)
{
  return CsrInsertObject(ProcessData, Handle, Object, Access, Inheritable);
}

NTSTATUS FASTCALL
Win32CsrGetObject(PCSRSS_PROCESS_DATA ProcessData,
                 HANDLE Handle,
                 Object_t **Object,
                 DWORD Access)
{
  return CsrGetObject(ProcessData, Handle, Object, Access);
}

NTSTATUS FASTCALL
Win32CsrLockObject(PCSRSS_PROCESS_DATA ProcessData,
                   HANDLE Handle,
                   Object_t **Object,
                   DWORD Access,
                   LONG Type)
{
  NTSTATUS Status;

  Status = CsrGetObject(ProcessData, Handle, Object, Access);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  if ((*Object)->Type != Type)
    {
      CsrReleaseObjectByPointer(*Object);
      return STATUS_INVALID_HANDLE;
    }

  EnterCriticalSection(&((*Object)->Lock));

  return STATUS_SUCCESS;
}

VOID FASTCALL
Win32CsrUnlockObject(Object_t *Object)
{
  LeaveCriticalSection(&(Object->Lock));
  CsrReleaseObjectByPointer(Object);
}

NTSTATUS FASTCALL
Win32CsrReleaseObjectByPointer(Object_t *Object)
{
  return CsrReleaseObjectByPointer(Object);
}

NTSTATUS FASTCALL
Win32CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
                      HANDLE Object)
{
  return CsrReleaseObject(ProcessData, Object);
}

NTSTATUS FASTCALL
Win32CsrReleaseConsole(PCSRSS_PROCESS_DATA ProcessData)
{
  return CsrReleaseConsole(ProcessData);
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
  PrivateCsrssInitialized();

  return TRUE;
}

BOOL WINAPI
Win32CsrInitialization(PCSRSS_API_DEFINITION *ApiDefinitions,
                       PCSRPLUGIN_SERVER_PROCS ServerProcs,
                       PCSRSS_EXPORTED_FUNCS Exports,
                       HANDLE CsrssApiHeap)
{
  NTSTATUS Status;
  CsrExports = *Exports;
  Win32CsrApiHeap = CsrssApiHeap;

  Status = NtUserInitialize(0 ,NULL, NULL);

  PrivateCsrssManualGuiCheck(0);
  CsrInitConsoleSupport();
  CsrRegisterObjectDefinitions(Win32CsrObjectDefinitions);

  *ApiDefinitions = Win32CsrApiDefinitions;
  ServerProcs->InitCompleteProc = Win32CsrInitComplete;
  ServerProcs->HardErrorProc = Win32CsrHardError;
  ServerProcs->ProcessInheritProc = CsrDuplicateHandleTable;
  ServerProcs->ProcessDeletedProc = CsrReleaseConsole;

  return TRUE;
}

/* EOF */
