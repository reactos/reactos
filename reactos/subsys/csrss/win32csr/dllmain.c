/* $Id: dllmain.c,v 1.3 2004/01/11 17:31:16 gvg Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            subsys/csrss/win32csr/dllmain.c
 * PURPOSE:         Initialization 
 */

/* INCLUDES ******************************************************************/

#include <windows.h>
#include "csrplugin.h"
#include "conio.h"
#include "desktopbg.h"
#include "guiconsole.h"

#define NDEBUG
#include <debug.h>

/* Not defined in any header file */
extern VOID STDCALL PrivateCsrssManualGuiCheck(LONG Check);

/* GLOBALS *******************************************************************/

HANDLE Win32CsrApiHeap;
static CSRSS_EXPORTED_FUNCS CsrExports;

static CSRSS_API_DEFINITION Win32CsrApiDefinitions[] =
  {
    CSRSS_DEFINE_API(CSRSS_WRITE_CONSOLE,                CsrWriteConsole),
    CSRSS_DEFINE_API(CSRSS_READ_CONSOLE,                 CsrReadConsole),
    CSRSS_DEFINE_API(CSRSS_ALLOC_CONSOLE,                CsrAllocConsole),
    CSRSS_DEFINE_API(CSRSS_FREE_CONSOLE,                 CsrFreeConsole),
    CSRSS_DEFINE_API(CSRSS_SCREEN_BUFFER_INFO,           CsrGetScreenBufferInfo),
    CSRSS_DEFINE_API(CSRSS_SET_CURSOR,                   CsrSetCursor),
    CSRSS_DEFINE_API(CSRSS_FILL_OUTPUT,                  CsrFillOutputChar),
    CSRSS_DEFINE_API(CSRSS_READ_INPUT,                   CsrReadInputEvent),
    CSRSS_DEFINE_API(CSRSS_WRITE_CONSOLE_OUTPUT_CHAR,    CsrWriteConsoleOutputChar),
    CSRSS_DEFINE_API(CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB,  CsrWriteConsoleOutputAttrib),
    CSRSS_DEFINE_API(CSRSS_FILL_OUTPUT_ATTRIB,           CsrFillOutputAttrib),
    CSRSS_DEFINE_API(CSRSS_GET_CURSOR_INFO,              CsrGetCursorInfo),
    CSRSS_DEFINE_API(CSRSS_SET_CURSOR_INFO,              CsrSetCursorInfo),
    CSRSS_DEFINE_API(CSRSS_SET_ATTRIB,                   CsrSetTextAttrib),
    CSRSS_DEFINE_API(CSRSS_GET_CONSOLE_MODE,             CsrGetConsoleMode),
    CSRSS_DEFINE_API(CSRSS_SET_CONSOLE_MODE,             CsrSetConsoleMode),
    CSRSS_DEFINE_API(CSRSS_CREATE_SCREEN_BUFFER,         CsrCreateScreenBuffer),
    CSRSS_DEFINE_API(CSRSS_SET_SCREEN_BUFFER,            CsrSetScreenBuffer),
    CSRSS_DEFINE_API(CSRSS_SET_TITLE,                    CsrSetTitle),
    CSRSS_DEFINE_API(CSRSS_GET_TITLE,                    CsrGetTitle),
    CSRSS_DEFINE_API(CSRSS_WRITE_CONSOLE_OUTPUT,         CsrWriteConsoleOutput),
    CSRSS_DEFINE_API(CSRSS_FLUSH_INPUT_BUFFER,           CsrFlushInputBuffer),
    CSRSS_DEFINE_API(CSRSS_SCROLL_CONSOLE_SCREEN_BUFFER, CsrScrollConsoleScreenBuffer),
    CSRSS_DEFINE_API(CSRSS_READ_CONSOLE_OUTPUT_CHAR,     CsrReadConsoleOutputChar),
    CSRSS_DEFINE_API(CSRSS_READ_CONSOLE_OUTPUT_ATTRIB,   CsrReadConsoleOutputAttrib),
    CSRSS_DEFINE_API(CSRSS_GET_NUM_INPUT_EVENTS,         CsrGetNumberOfConsoleInputEvents),
    CSRSS_DEFINE_API(CSRSS_PEEK_CONSOLE_INPUT,           CsrPeekConsoleInput),
    CSRSS_DEFINE_API(CSRSS_READ_CONSOLE_OUTPUT,          CsrReadConsoleOutput),
    CSRSS_DEFINE_API(CSRSS_WRITE_CONSOLE_INPUT,          CsrWriteConsoleInput),
    CSRSS_DEFINE_API(CSRSS_SETGET_CONSOLE_HW_STATE,      CsrHardwareStateProperty),
    CSRSS_DEFINE_API(CSRSS_CREATE_DESKTOP,               CsrCreateDesktop),
    CSRSS_DEFINE_API(CSRSS_SHOW_DESKTOP,                 CsrShowDesktop),
    CSRSS_DEFINE_API(CSRSS_HIDE_DESKTOP,                 CsrHideDesktop),
    { 0, 0, 0, NULL }
  };

static CSRSS_OBJECT_DEFINITION Win32CsrObjectDefinitions[] =
  {
    { CONIO_CONSOLE_MAGIC,       ConioDeleteConsole },
    { CONIO_SCREEN_BUFFER_MAGIC, ConioDeleteScreenBuffer },
    { 0,                         NULL }
  };

/* FUNCTIONS *****************************************************************/

BOOL STDCALL
DllMain(HANDLE hDll,
	DWORD dwReason,
	LPVOID lpReserved)
{
  return TRUE;
}

NTSTATUS FASTCALL
Win32CsrInsertObject(PCSRSS_PROCESS_DATA ProcessData,
                     PHANDLE Handle,
                     Object_t *Object)
{
  InitializeCriticalSection(&(Object->Lock));

  return (CsrExports.CsrInsertObjectProc)(ProcessData, Handle, Object);
}

NTSTATUS FASTCALL
Win32CsrGetObject(PCSRSS_PROCESS_DATA ProcessData,
                 HANDLE Handle,
                 Object_t **Object)
{
  return (CsrExports.CsrGetObjectProc)(ProcessData, Handle, Object);
}

NTSTATUS FASTCALL
Win32CsrLockObject(PCSRSS_PROCESS_DATA ProcessData,
                   HANDLE Handle,
                   Object_t **Object,
                   LONG Type)
{
  NTSTATUS Status;

  Status = (CsrExports.CsrGetObjectProc)(ProcessData, Handle, Object);
  if (! NT_SUCCESS(Status))
    {
      return Status;
    }

  if ((*Object)->Type != Type)
    {
      return STATUS_INVALID_HANDLE;
    }

  EnterCriticalSection(&((*Object)->Lock));

  return STATUS_SUCCESS;
}

VOID FASTCALL
Win32CsrUnlockObject(Object_t *Object)
{
  LeaveCriticalSection(&(Object->Lock));
}

NTSTATUS FASTCALL
Win32CsrReleaseObject(PCSRSS_PROCESS_DATA ProcessData,
                     HANDLE Object)
{
  return (CsrExports.CsrReleaseObjectProc)(ProcessData, Object);
}


BOOL STDCALL
Win32CsrInitialization(PCSRSS_API_DEFINITION *ApiDefinitions,
                       PCSRSS_OBJECT_DEFINITION *ObjectDefinitions,
                       PCSRSS_EXPORTED_FUNCS Exports,
                       HANDLE CsrssApiHeap)
{
  HANDLE ThreadHandle;

  CsrExports = *Exports;
  Win32CsrApiHeap = CsrssApiHeap;

  PrivateCsrssManualGuiCheck(0);
  CsrInitConsoleSupport();
  ThreadHandle = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE) Console_Api, NULL, 0, NULL);
  if (NULL == ThreadHandle)
    {
      DPRINT1("CSR: Unable to create console thread\n");
      return FALSE;
    }
  CloseHandle(ThreadHandle);

  *ApiDefinitions = Win32CsrApiDefinitions;
  *ObjectDefinitions = Win32CsrObjectDefinitions;

  return TRUE;
}

/* EOF */
