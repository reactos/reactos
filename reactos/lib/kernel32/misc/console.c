/* $Id$
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/console.c
 * PURPOSE:         Win32 server console functions
 * PROGRAMMER:      James Tabor 
 *			<jimtabor@adsl-64-217-116-74.dsl.hstntx.swbell.net>
 * UPDATE HISTORY:
 *	199901?? ??	Created
 *	19990204 EA	SetConsoleTitleA
 *      19990306 EA	Stubs
 */

/* INCLUDES ******************************************************************/

#include <k32.h>

#define NDEBUG
#include "../include/debug.h"

extern BOOL WINAPI DefaultConsoleCtrlHandler(DWORD Event);
extern __declspec(noreturn) VOID CALLBACK ConsoleControlDispatcher(DWORD CodeAndFlag);
extern RTL_CRITICAL_SECTION ConsoleLock;
extern BOOL WINAPI IsDebuggerPresent(VOID);


/* GLOBALS *******************************************************************/

static BOOL IgnoreCtrlEvents = FALSE;

static PHANDLER_ROUTINE* CtrlHandlers = NULL;
static ULONG NrCtrlHandlers = 0;
static WCHAR InputExeName[MAX_PATH + 1] = L"";

/* Default Console Control Handler *******************************************/

BOOL WINAPI DefaultConsoleCtrlHandler(DWORD Event)
{
	switch(Event)
	{
	case CTRL_C_EVENT:
		DPRINT("Ctrl-C Event\n");
		break;
		
	case CTRL_BREAK_EVENT:
		DPRINT("Ctrl-Break Event\n");
		break;

	case CTRL_SHUTDOWN_EVENT:
		DPRINT("Ctrl Shutdown Event\n");
		break;

	case CTRL_CLOSE_EVENT:
		DPRINT("Ctrl Close Event\n");
		break;

	case CTRL_LOGOFF_EVENT:		
		DPRINT("Ctrl Logoff Event\n");
		break;
	}
	ExitProcess(0);
	return TRUE;
}


__declspec(noreturn) VOID CALLBACK ConsoleControlDispatcher(DWORD CodeAndFlag)
{
DWORD nExitCode = 0;
DWORD nCode = CodeAndFlag & MAXLONG;
UINT i;

SetThreadPriority(GetCurrentThread(), THREAD_PRIORITY_HIGHEST);

	switch(nCode)
	{
	case CTRL_C_EVENT:
	case CTRL_BREAK_EVENT:
	{
		if(IsDebuggerPresent())
		{
			EXCEPTION_RECORD erException;
			erException.ExceptionCode = 
			(nCode == CTRL_C_EVENT ? DBG_CONTROL_C : DBG_CONTROL_BREAK);
			erException.ExceptionFlags = 0;
			erException.ExceptionRecord = NULL;
			erException.ExceptionAddress = &DefaultConsoleCtrlHandler;
			erException.NumberParameters = 0;
			RtlRaiseException(&erException);
		}		
		RtlEnterCriticalSection(&ConsoleLock);

		if(!(nCode == CTRL_C_EVENT &&
			NtCurrentPeb()->ProcessParameters->ProcessGroup & 1))
		{
			for(i = NrCtrlHandlers; i > 0; -- i)
				if(CtrlHandlers[i - 1](nCode)) break;
		}
		RtlLeaveCriticalSection(&ConsoleLock);
		ExitThread(0);
	}
	case CTRL_CLOSE_EVENT:
	case CTRL_LOGOFF_EVENT:
	case CTRL_SHUTDOWN_EVENT:
		break;

	default: ExitThread(0);
	}

	RtlEnterCriticalSection(&ConsoleLock);

	if(!(nCode == CTRL_C_EVENT &&
		NtCurrentPeb()->ProcessParameters->ProcessGroup & 1))
	{
	i = NrCtrlHandlers;
	while(i > 0)
		{
		if (i == 1 && (CodeAndFlag & MINLONG) && 
			(nCode == CTRL_LOGOFF_EVENT || nCode == CTRL_SHUTDOWN_EVENT))
				break;

			if(CtrlHandlers[i - 1](nCode))
			{
				switch(nCode)
				{
					case CTRL_CLOSE_EVENT:
					case CTRL_LOGOFF_EVENT:
					case CTRL_SHUTDOWN_EVENT:
						nExitCode = CodeAndFlag;
				}
				break;
			}
			--i;
		}
	}
	RtlLeaveCriticalSection(&ConsoleLock);
	ExitThread(nExitCode);
}


/* FUNCTIONS *****************************************************************/

/*
 * @unimplemented
 */
BOOL STDCALL
AddConsoleAliasA (LPSTR Source,
		  LPSTR Target,
		  LPSTR ExeName)
{
  DPRINT1("AddConsoleAliasA(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Source, Target, ExeName);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
AddConsoleAliasW (LPWSTR Source,
		  LPWSTR Target,
		  LPWSTR ExeName)
{
  DPRINT1("AddConsoleAliasW(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Source, Target, ExeName);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
ConsoleMenuControl (HANDLE	hConsole,
		    DWORD	Unknown1,
		    DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  DPRINT1("ConsoleMenuControl(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", hConsole, Unknown1, Unknown2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @implemented
 */
HANDLE STDCALL
DuplicateConsoleHandle (HANDLE	hConsole,
			DWORD   dwDesiredAccess,
			BOOL	bInheritHandle,
			DWORD	dwOptions)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  if (IsConsoleHandle (hConsole) == FALSE)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return INVALID_HANDLE_VALUE;
    }
  
  Request.Type = CSRSS_DUPLICATE_HANDLE;
  Request.Data.DuplicateHandleRequest.Handle = hConsole;
  Request.Data.DuplicateHandleRequest.ProcessId = GetTeb()->Cid.UniqueProcess;
  Status = CsrClientCallServer(&Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status=Reply.Status))
    {
      SetLastErrorByStatus(Status);
      return INVALID_HANDLE_VALUE;
    }
  return Reply.Data.DuplicateHandleReply.Handle;
}


/*
 * @unimplemented
 */
DWORD STDCALL
ExpungeConsoleCommandHistoryW (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  DPRINT1("ExpungeConsoleCommandHistoryW(0x%x) UNIMPLEMENTED!\n", Unknown0);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
ExpungeConsoleCommandHistoryA (DWORD	Unknown0)
     /*
      * Undocumented
      */
{

  DPRINT1("ExpungeConsoleCommandHistoryW(0x%x) UNIMPLEMENTED!\n", Unknown0);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasW (DWORD	Unknown0,
		  DWORD	Unknown1,
		  DWORD	Unknown2,
		  DWORD	Unknown3)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleAliasW(0x%x, 0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2, Unknown3);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasA (DWORD	Unknown0,
		  DWORD	Unknown1,
		  DWORD	Unknown2,
		  DWORD	Unknown3)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleAliasA(0x%x, 0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2, Unknown3);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasExesW (DWORD	Unknown0,
		      DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleAliasExesW(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasExesA (DWORD	Unknown0,
		      DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleAliasExesA(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasExesLengthA (VOID)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleAliasExesLengthA() UNIMPLEMENTED!\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasExesLengthW (VOID)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleAliasExesLengthW() UNIMPLEMENTED!\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasesW (DWORD	Unknown0,
		    DWORD	Unknown1,
		    DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleAliasesW(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}
 

/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasesA (DWORD	Unknown0,
		    DWORD	Unknown1,
		    DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleAliasesA(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasesLengthW (DWORD Unknown0)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleAliasesLengthW(0x%x) UNIMPLEMENTED!\n", Unknown0);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleAliasesLengthA (DWORD Unknown0)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleAliasesLengthA(0x%x) UNIMPLEMENTED!\n", Unknown0);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleCommandHistoryW (DWORD	Unknown0,
			   DWORD	Unknown1,
			   DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleCommandHistoryW(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleCommandHistoryA (DWORD	Unknown0,
			   DWORD	Unknown1,
			   DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleCommandHistoryA(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleCommandHistoryLengthW (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleCommandHistoryLengthW(0x%x) UNIMPLEMENTED!\n", Unknown0);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleCommandHistoryLengthA (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleCommandHistoryLengthA(0x%x) UNIMPLEMENTED!\n", Unknown0);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleDisplayMode (LPDWORD lpdwMode)
     /*
      * FUNCTION: Get the console display mode
      * ARGUMENTS:
      *      lpdwMode - Address of variable that receives the current value
      *                 of display mode
      * STATUS: Undocumented
      */
{
  DPRINT1("GetConsoleDisplayMode(0x%x) UNIMPLEMENTED!\n", lpdwMode);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleFontInfo (DWORD	Unknown0,
		    DWORD	Unknown1,
		    DWORD	Unknown2,
		    DWORD	Unknown3)
     /*
      * Undocumented
      */
{
  DPRINT1("GetConsoleFontInfo(0x%x, 0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2, Unknown3);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetConsoleFontSize(HANDLE hConsoleOutput,
		   DWORD nFont)
{
  DPRINT1("GetConsoleFontSize(0x%x, 0x%x) UNIMPLEMENTED!\n", hConsoleOutput, nFont);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @implemented
 */
DWORD STDCALL
GetConsoleHardwareState (HANDLE	hConsole,
			 DWORD	Flags,
			 PDWORD	State)
     /*
      * Undocumented
      */
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  NTSTATUS          Status;

  Request.Type = CSRSS_SETGET_CONSOLE_HW_STATE;
  Request.Data.ConsoleHardwareStateRequest.ConsoleHandle = hConsole;
  Request.Data.ConsoleHardwareStateRequest.SetGet = CONSOLE_HARDWARE_STATE_GET;

  Status = CsrClientCallServer(& Request,
			       & Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  *State = Reply.Data.ConsoleHardwareStateReply.State;
  return TRUE;  
}


/*
 * @implemented
 */
DWORD STDCALL
GetConsoleInputWaitHandle (VOID)
     /*
      * Undocumented
      */
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request.Type = CSRSS_GET_INPUT_WAIT_HANDLE;
  Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST),
				sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
    {
      SetLastErrorByStatus(Status);
      return 0;
    }
  return (DWORD) Reply.Data.GetConsoleInputWaitHandle.InputWaitHandle;
}


/*
 * @unimplemented
 */
DWORD STDCALL
GetCurrentConsoleFont(HANDLE hConsoleOutput,
		      BOOL bMaximumWindow,
		      PCONSOLE_FONT_INFO lpConsoleCurrentFont)
{
  DPRINT1("GetCurrentConsoleFont(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", hConsoleOutput, bMaximumWindow, lpConsoleCurrentFont);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
ULONG STDCALL
GetNumberOfConsoleFonts (VOID)
     /*
      * Undocumented
      */
{
  DPRINT1("GetNumberOfConsoleFonts() UNIMPLEMENTED!\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 1; /* FIXME: call csrss.exe */
}


/*
 * @unimplemented
 */
DWORD STDCALL
InvalidateConsoleDIBits (DWORD	Unknown0,
			 DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  DPRINT1("InvalidateConsoleDIBits(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
HANDLE STDCALL
OpenConsoleW (LPWSTR  wsName,
	      DWORD   dwDesiredAccess,
	      BOOL    bInheritHandle,
	      DWORD   dwCreationDistribution)
     /*
      * Undocumented
      */
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  PHANDLE           phConsole = NULL;
  NTSTATUS          Status = STATUS_SUCCESS;
  
  if(0 == _wcsicmp(wsName, L"CONIN$"))
  {
    Request.Type = CSRSS_GET_INPUT_HANDLE;
    phConsole = & Reply.Data.GetInputHandleReply.InputHandle;
  }
  else if (0 == _wcsicmp(wsName, L"CONOUT$"))
  {
    Request.Type = CSRSS_GET_OUTPUT_HANDLE;
    phConsole = & Reply.Data.GetOutputHandleReply.OutputHandle;
  }
  else
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return(INVALID_HANDLE_VALUE);
  }
  if ((GENERIC_READ|GENERIC_WRITE) != dwDesiredAccess)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return(INVALID_HANDLE_VALUE);
  }
  if (OPEN_EXISTING != dwCreationDistribution)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return(INVALID_HANDLE_VALUE);
  }
  Status = CsrClientCallServer(& Request,
			       & Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus(Status);
    return INVALID_HANDLE_VALUE;
  }
  return(*phConsole);
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleCommandHistoryMode (DWORD	dwMode)
     /*
      * Undocumented
      */
{
  DPRINT1("SetConsoleCommandHistoryMode(0x%x) UNIMPLEMENTED!\n", dwMode);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleCursor (DWORD	Unknown0,
		  DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  DPRINT1("SetConsoleCursor(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleDisplayMode (HANDLE hOut,
		       DWORD dwNewMode,
		       LPDWORD lpdwOldMode)
     /*
      * FUNCTION: Set the console display mode.
      * ARGUMENTS:
      *       hOut - Standard output handle.
      *       dwNewMode - New mode.
      *       lpdwOldMode - Address of a variable that receives the old mode.
      */
{
  DPRINT1("SetConsoleDisplayMode(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", hOut, dwNewMode, lpdwOldMode);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleFont (DWORD	Unknown0,
		DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  DPRINT1("SetConsoleFont(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @implemented
 */
BOOL STDCALL
SetConsoleHardwareState (HANDLE	hConsole,
			 DWORD	Flags,
			 DWORD	State)
     /*
      * Undocumented
      */
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  NTSTATUS          Status;

  Request.Type = CSRSS_SETGET_CONSOLE_HW_STATE;
  Request.Data.ConsoleHardwareStateRequest.ConsoleHandle = hConsole;
  Request.Data.ConsoleHardwareStateRequest.SetGet = CONSOLE_HARDWARE_STATE_SET;
  Request.Data.ConsoleHardwareStateRequest.State = State;

  Status = CsrClientCallServer(& Request,
			       & Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  return TRUE;  
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleKeyShortcuts (DWORD	Unknown0,
			DWORD	Unknown1,
			DWORD	Unknown2,
			DWORD	Unknown3)
     /*
      * Undocumented
      */
{
  DPRINT1("SetConsoleKeyShortcuts(0x%x, 0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2, Unknown3);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleMaximumWindowSize (DWORD	Unknown0,
			     DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  DPRINT1("SetConsoleMaximumWindowSize(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleMenuClose (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  DPRINT1("SetConsoleMenuClose(0x%x) UNIMPLEMENTED!\n", Unknown0);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleNumberOfCommandsA (DWORD	Unknown0,
			     DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  DPRINT1("SetConsoleNumberOfCommandsA(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsoleNumberOfCommandsW (DWORD	Unknown0,
			     DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  DPRINT1("SetConsoleNumberOfCommandsW(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetConsolePalette (DWORD	Unknown0,
		   DWORD	Unknown1,
		   DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  DPRINT1("SetConsolePalette(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
BOOL STDCALL
SetLastConsoleEventActive (VOID)
     /*
      * Undocumented
      */
{
  DPRINT1("SetLastConsoleEventActive() UNIMPLEMENTED!\n");
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*
 * @unimplemented
 */
DWORD STDCALL
ShowConsoleCursor (DWORD	Unknown0,
		   DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  DPRINT1("ShowConsoleCursor(0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * FUNCTION: Checks whether the given handle is a valid console handle.
 * ARGUMENTS:
 *      Handle - Handle to be checked
 * RETURNS:
 *      TRUE: Handle is a valid console handle
 *      FALSE: Handle is not a valid console handle.
 * STATUS: Officially undocumented
 *
 * @implemented
 */
BOOL STDCALL
VerifyConsoleIoHandle(HANDLE Handle)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request.Type = CSRSS_VERIFY_HANDLE;
  Request.Data.VerifyHandleRequest.Handle = Handle;
  Status = CsrClientCallServer(&Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return FALSE;
    }

  return (BOOL)NT_SUCCESS(Reply.Status);
}


/*
 * @unimplemented
 */
DWORD STDCALL
WriteConsoleInputVDMA (DWORD	Unknown0,
		       DWORD	Unknown1,
		       DWORD	Unknown2,
		       DWORD	Unknown3)
{
  DPRINT1("WriteConsoleInputVDMA(0x%x, 0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2, Unknown3);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @unimplemented
 */
DWORD STDCALL
WriteConsoleInputVDMW (DWORD	Unknown0,
		       DWORD	Unknown1,
		       DWORD	Unknown2,
		       DWORD	Unknown3)
{
  DPRINT1("WriteConsoleInputVDMW(0x%x, 0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", Unknown0, Unknown1, Unknown2, Unknown3);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


/*
 * @implemented
 */
BOOL STDCALL
CloseConsoleHandle(HANDLE Handle)
     /*
      * Undocumented
      */
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  if (IsConsoleHandle (Handle) == FALSE)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }

  Request.Type = CSRSS_CLOSE_HANDLE;
  Request.Data.CloseHandleRequest.Handle = Handle;
  Status = CsrClientCallServer(&Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST),
			       sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status))
    {
       SetLastErrorByStatus(Status);
       return FALSE;
    }

  return TRUE;
}


/*
 * internal function
 */
BOOL STDCALL
IsConsoleHandle(HANDLE Handle)
{
  if ((((ULONG)Handle) & 0x10000003) == 0x3)
    {
      return(TRUE);
    }
  return(FALSE);
}


/*
 * @implemented
 */
HANDLE STDCALL
GetStdHandle(DWORD nStdHandle)
     /*
      * FUNCTION: Get a handle for the standard input, standard output
      * and a standard error device.
      * ARGUMENTS:
      *       nStdHandle - Specifies the device for which to return the handle.
      * RETURNS: If the function succeeds, the return value is the handle
      * of the specified device. Otherwise the value is INVALID_HANDLE_VALUE.
      */
{
  PRTL_USER_PROCESS_PARAMETERS Ppb;

  Ppb = NtCurrentPeb()->ProcessParameters;
  switch (nStdHandle)
    {
      case STD_INPUT_HANDLE:
	return Ppb->hStdInput;

      case STD_OUTPUT_HANDLE:
	return Ppb->hStdOutput;

      case STD_ERROR_HANDLE:
	return Ppb->hStdError;
    }

  SetLastError (ERROR_INVALID_PARAMETER);
  return INVALID_HANDLE_VALUE;
}


/*
 * @implemented
 */
BOOL WINAPI
SetStdHandle(DWORD nStdHandle,
	     HANDLE hHandle)
     /*
      * FUNCTION: Set the handle for the standard input, standard output or
      * the standard error device.
      * ARGUMENTS:
      *        nStdHandle - Specifies the handle to be set.
      *        hHandle - The handle to set.
      * RETURNS: TRUE if the function succeeds, FALSE otherwise.
      */
{
  PRTL_USER_PROCESS_PARAMETERS Ppb;

  /* no need to check if hHandle == INVALID_HANDLE_VALUE */

  Ppb = NtCurrentPeb()->ProcessParameters;

  switch (nStdHandle)
    {
      case STD_INPUT_HANDLE:
	Ppb->hStdInput = hHandle;
	return TRUE;

      case STD_OUTPUT_HANDLE:
	Ppb->hStdOutput = hHandle;
	return TRUE;

      case STD_ERROR_HANDLE:
	Ppb->hStdError = hHandle;
	return TRUE;
    }

  /* windows for whatever reason sets the last error to ERROR_INVALID_HANDLE here */
  SetLastError (ERROR_INVALID_HANDLE);
  return FALSE;
}


static BOOL
IntWriteConsole(HANDLE hConsoleOutput,
                PVOID lpBuffer,
                DWORD nNumberOfCharsToWrite,
                LPDWORD lpNumberOfCharsWritten,
                LPVOID lpReserved,
                BOOL bUnicode)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  USHORT nChars;
  ULONG MessageSize, BufferSize, SizeBytes, CharSize;
  DWORD Written = 0;
  
  CharSize = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));

  BufferSize = sizeof(CSRSS_API_REQUEST) + min(nNumberOfCharsToWrite * CharSize, CSRSS_MAX_WRITE_CONSOLE_REQUEST);
  Request = RtlAllocateHeap(GetProcessHeap(), 0, BufferSize);
  if(Request == NULL)
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  Request->Type = CSRSS_WRITE_CONSOLE;
  Request->Data.WriteConsoleRequest.ConsoleHandle = hConsoleOutput;
  Request->Data.WriteConsoleRequest.Unicode = bUnicode;

  while(nNumberOfCharsToWrite > 0)
  {
    nChars = min(nNumberOfCharsToWrite, CSRSS_MAX_WRITE_CONSOLE_REQUEST / CharSize);
    Request->Data.WriteConsoleRequest.NrCharactersToWrite = nChars;

    SizeBytes = nChars * CharSize;

    memcpy(Request->Data.WriteConsoleRequest.Buffer, lpBuffer, SizeBytes);

    MessageSize = CSRSS_REQUEST_HEADER_SIZE + sizeof(CSRSS_WRITE_CONSOLE_REQUEST) + SizeBytes;
    Status = CsrClientCallServer(Request,
                                 &Reply,
                                 MessageSize,
                                 sizeof(CSRSS_API_REPLY));

    if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
    {
      RtlFreeHeap(GetProcessHeap(), 0, Request);
      SetLastErrorByStatus(Status);
      return FALSE;
    }

    nNumberOfCharsToWrite -= nChars;
    lpBuffer = (PVOID)((ULONG_PTR)lpBuffer + (ULONG_PTR)SizeBytes);
    Written += Reply.Data.WriteConsoleReply.NrCharactersWritten;
  }

  RtlFreeHeap(GetProcessHeap(), 0, Request);
  
  if(lpNumberOfCharsWritten != NULL)
  {
    *lpNumberOfCharsWritten = Written;
  }

  return TRUE;
}


/*--------------------------------------------------------------
 *	WriteConsoleA
 *
 * @implemented
 */
BOOL STDCALL 
WriteConsoleA(HANDLE hConsoleOutput,
	      CONST VOID *lpBuffer,
	      DWORD nNumberOfCharsToWrite,
	      LPDWORD lpNumberOfCharsWritten,
	      LPVOID lpReserved)
{
  return IntWriteConsole(hConsoleOutput,
                         (PVOID)lpBuffer,
                         nNumberOfCharsToWrite,
                         lpNumberOfCharsWritten,
                         lpReserved,
                         FALSE);
}


/*--------------------------------------------------------------
 *	WriteConsoleW
 *
 * @implemented
 */
BOOL STDCALL
WriteConsoleW(
	HANDLE		 hConsoleOutput,
	CONST VOID	*lpBuffer,
	DWORD		 nNumberOfCharsToWrite,
	LPDWORD		 lpNumberOfCharsWritten,
	LPVOID		 lpReserved
	)
{
  return IntWriteConsole(hConsoleOutput,
                         (PVOID)lpBuffer,
                         nNumberOfCharsToWrite,
                         lpNumberOfCharsWritten,
                         lpReserved,
                         TRUE);
}


static BOOL
IntReadConsole(HANDLE hConsoleInput,
               PVOID lpBuffer,
               DWORD nNumberOfCharsToRead,
               LPDWORD lpNumberOfCharsRead,
               LPVOID lpReserved,
               BOOL bUnicode)
{
  CSRSS_API_REQUEST Request;
  PCSRSS_API_REPLY Reply;
  NTSTATUS Status;
  ULONG BufferSize, CharSize, CharsRead = 0;
  
  CharSize = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
  
  BufferSize = sizeof(CSRSS_API_REQUEST) + min(nNumberOfCharsToRead * CharSize, CSRSS_MAX_READ_CONSOLE_REQUEST);
  Reply = RtlAllocateHeap(GetProcessHeap(), 0, BufferSize);
  if(Reply == NULL)
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  
  Reply->Status = STATUS_SUCCESS;

  do
  {
    if(Reply->Status == STATUS_PENDING)
    {
      Status = NtWaitForSingleObject(Reply->Data.ReadConsoleReply.EventHandle, FALSE, 0);
      if(!NT_SUCCESS(Status))
      {
        DPRINT1("Wait for console input failed!\n");
        break;
      }
    }
    
    Request.Type = CSRSS_READ_CONSOLE;
    Request.Data.ReadConsoleRequest.ConsoleHandle = hConsoleInput;
    Request.Data.ReadConsoleRequest.Unicode = bUnicode;
    Request.Data.ReadConsoleRequest.NrCharactersToRead = min(nNumberOfCharsToRead, CSRSS_MAX_READ_CONSOLE_REQUEST / CharSize);
    Request.Data.ReadConsoleRequest.nCharsCanBeDeleted = CharsRead;
    Status = CsrClientCallServer(&Request,
                                 Reply,
                                 sizeof(CSRSS_API_REQUEST),
                                 sizeof(CSRSS_API_REPLY) + (Request.Data.ReadConsoleRequest.NrCharactersToRead * CharSize));

    if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply->Status))
    {
      DPRINT1("CSR returned error in ReadConsole\n");
      SetLastErrorByStatus(Status);
      RtlFreeHeap(GetProcessHeap(), 0, Reply);
      return FALSE;
    }
    
    nNumberOfCharsToRead -= Reply->Data.ReadConsoleReply.NrCharactersRead;
    memcpy((PVOID)((ULONG_PTR)lpBuffer + (ULONG_PTR)(CharsRead * CharSize)),
           Reply->Data.ReadConsoleReply.Buffer,
           Reply->Data.ReadConsoleReply.NrCharactersRead * CharSize);
    CharsRead += Reply->Data.ReadConsoleReply.NrCharactersRead;
    
    if(Reply->Status == STATUS_NOTIFY_CLEANUP)
    {
      if(CharsRead > 0)
      {
        CharsRead--;
        nNumberOfCharsToRead++;
      }
      Reply->Status = STATUS_PENDING;
    }
  } while(Reply->Status == STATUS_PENDING && nNumberOfCharsToRead > 0);
  
  if(lpNumberOfCharsRead != NULL)
  {
    *lpNumberOfCharsRead = CharsRead;
  }
  
  return TRUE;
}


/*--------------------------------------------------------------
 *	ReadConsoleA
 *
 * @implemented
 */
BOOL STDCALL
ReadConsoleA(HANDLE hConsoleInput,
             LPVOID lpBuffer,
             DWORD nNumberOfCharsToRead,
             LPDWORD lpNumberOfCharsRead,
             LPVOID lpReserved)
{
  return IntReadConsole(hConsoleInput,
                        lpBuffer,
                        nNumberOfCharsToRead,
                        lpNumberOfCharsRead,
                        lpReserved,
                        FALSE);
}


/*--------------------------------------------------------------
 *	ReadConsoleW
 *
 * @implemented
 */
BOOL STDCALL
ReadConsoleW(HANDLE hConsoleInput,
             LPVOID lpBuffer,
             DWORD nNumberOfCharsToRead,
             LPDWORD lpNumberOfCharsRead,
             LPVOID lpReserved)
{
  return IntReadConsole(hConsoleInput,
                        lpBuffer,
                        nNumberOfCharsToRead,
                        lpNumberOfCharsRead,
                        lpReserved,
                        TRUE);
}


/*--------------------------------------------------------------
 *	AllocConsole
 *
 * @implemented
 */
BOOL STDCALL AllocConsole(VOID)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;
   HANDLE hStdError;

   if(NtCurrentPeb()->ProcessParameters->hConsole)
   {
	DPRINT("AllocConsole: Allocate duplicate console to the same Process\n");
	SetLastErrorByStatus (STATUS_OBJECT_NAME_EXISTS); 
	return FALSE;	 
   }

   Request.Data.AllocConsoleRequest.CtrlDispatcher = (PCONTROLDISPATCHER) &ConsoleControlDispatcher;

   Request.Type = CSRSS_ALLOC_CONSOLE;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   NtCurrentPeb()->ProcessParameters->hConsole = Reply.Data.AllocConsoleReply.Console;
   SetStdHandle( STD_INPUT_HANDLE, Reply.Data.AllocConsoleReply.InputHandle );
   SetStdHandle( STD_OUTPUT_HANDLE, Reply.Data.AllocConsoleReply.OutputHandle );
   hStdError = DuplicateConsoleHandle(Reply.Data.AllocConsoleReply.OutputHandle,
                                      0,
				      TRUE,
				      DUPLICATE_SAME_ACCESS);
   SetStdHandle( STD_ERROR_HANDLE, hStdError );
   return TRUE;
}


/*--------------------------------------------------------------
 *	FreeConsole
 *
 * @implemented
 */
BOOL STDCALL FreeConsole(VOID)
{
    // AG: I'm not sure if this is correct (what happens to std handles?)
    // but I just tried to reverse what AllocConsole() does...

   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_FREE_CONSOLE;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }

   return TRUE;
}


/*--------------------------------------------------------------
 *	GetConsoleScreenBufferInfo
 *
 * @implemented
 */
BOOL
STDCALL
GetConsoleScreenBufferInfo(
    HANDLE hConsoleOutput,
    PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo
    )
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_SCREEN_BUFFER_INFO;
   Request.Data.ScreenBufferInfoRequest.ConsoleHandle = hConsoleOutput;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   *lpConsoleScreenBufferInfo = Reply.Data.ScreenBufferInfoReply.Info;
   return TRUE;
}


/*--------------------------------------------------------------
 *	SetConsoleCursorPosition
 *
 * @implemented
 */
BOOL
STDCALL
SetConsoleCursorPosition(
    HANDLE hConsoleOutput,
    COORD dwCursorPosition
    )
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_SET_CURSOR;
   Request.Data.SetCursorRequest.ConsoleHandle = hConsoleOutput;
   Request.Data.SetCursorRequest.Position = dwCursorPosition;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   return TRUE;
}


static BOOL
IntFillConsoleOutputCharacter(HANDLE hConsoleOutput,
                              PVOID cCharacter,
                              DWORD nLength,
                              COORD dwWriteCoord,
                              LPDWORD lpNumberOfCharsWritten,
                              BOOL bUnicode)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request.Type = CSRSS_FILL_OUTPUT;
  Request.Data.FillOutputRequest.ConsoleHandle = hConsoleOutput;
  Request.Data.FillOutputRequest.Unicode = bUnicode;
  if(bUnicode)
    Request.Data.FillOutputRequest.Char.UnicodeChar = *((WCHAR*)cCharacter);
  else
    Request.Data.FillOutputRequest.Char.AsciiChar = *((CHAR*)cCharacter);
  Request.Data.FillOutputRequest.Position = dwWriteCoord;
  Request.Data.FillOutputRequest.Length = nLength;
  Status = CsrClientCallServer(&Request, &Reply,
                               sizeof(CSRSS_API_REQUEST),
                               sizeof(CSRSS_API_REPLY));

  if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  if(lpNumberOfCharsWritten != NULL)
  {
    *lpNumberOfCharsWritten = Reply.Data.FillOutputReply.NrCharactersWritten;
  }
  
  return TRUE;
}

/*--------------------------------------------------------------
 *	FillConsoleOutputCharacterA
 *
 * @implemented
 */
BOOL STDCALL
FillConsoleOutputCharacterA(
	HANDLE		hConsoleOutput,
	CHAR		cCharacter,
	DWORD		nLength,
	COORD		dwWriteCoord,
	LPDWORD		lpNumberOfCharsWritten
	)
{
  return IntFillConsoleOutputCharacter(hConsoleOutput,
                                       &cCharacter,
                                       nLength,
                                       dwWriteCoord,
                                       lpNumberOfCharsWritten,
                                       FALSE);
}


/*--------------------------------------------------------------
 *	FillConsoleOutputCharacterW
 *
 * @implemented
 */
BOOL
STDCALL
FillConsoleOutputCharacterW(
	HANDLE		hConsoleOutput,
	WCHAR		cCharacter,
	DWORD		nLength,
	COORD		dwWriteCoord,
	LPDWORD		lpNumberOfCharsWritten
	)
{
  return IntFillConsoleOutputCharacter(hConsoleOutput,
                                       &cCharacter,
                                       nLength,
                                       dwWriteCoord,
                                       lpNumberOfCharsWritten,
                                       TRUE);
}


static BOOL
IntPeekConsoleInput(HANDLE hConsoleInput,
                    PINPUT_RECORD lpBuffer,
                    DWORD nLength,
                    LPDWORD lpNumberOfEventsRead,
                    BOOL bUnicode)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  PVOID BufferBase;
  PVOID BufferTargetBase;
  ULONG Size;
  
  if(lpBuffer == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Size = nLength * sizeof(INPUT_RECORD);

  Status = CsrCaptureParameterBuffer((PVOID)lpBuffer, Size, &BufferBase, &BufferTargetBase);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  
  Request = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CSRSS_API_REQUEST));
  if(Request == NULL)
  {
    CsrReleaseParameterBuffer(BufferBase);
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }
  
  Request->Type = CSRSS_PEEK_CONSOLE_INPUT;
  Request->Data.PeekConsoleInputRequest.ConsoleHandle = hConsoleInput;
  Request->Data.PeekConsoleInputRequest.Unicode = bUnicode;
  Request->Data.PeekConsoleInputRequest.Length = nLength;
  Request->Data.PeekConsoleInputRequest.InputRecord = (INPUT_RECORD*)BufferTargetBase;
  
  Status = CsrClientCallServer(Request, &Reply,
                               sizeof(CSRSS_API_REQUEST),
                               sizeof(CSRSS_API_REPLY));
  
  if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    RtlFreeHeap(GetProcessHeap(), 0, Request);
    CsrReleaseParameterBuffer(BufferBase);
    return FALSE;
  }

  memcpy(lpBuffer, BufferBase, sizeof(INPUT_RECORD) * Reply.Data.PeekConsoleInputReply.Length);

  RtlFreeHeap(GetProcessHeap(), 0, Request);
  CsrReleaseParameterBuffer(BufferBase);
  
  if(lpNumberOfEventsRead != NULL)
  {
    *lpNumberOfEventsRead = Reply.Data.PeekConsoleInputReply.Length;
  }
  
  return TRUE;
}

/*--------------------------------------------------------------
 * 	PeekConsoleInputA
 *
 * @implemented
 */
BOOL
WINAPI
PeekConsoleInputA(
	HANDLE			hConsoleInput,
	PINPUT_RECORD		lpBuffer,
	DWORD			nLength,
	LPDWORD			lpNumberOfEventsRead
	)
{
  return IntPeekConsoleInput(hConsoleInput, lpBuffer, nLength,
                             lpNumberOfEventsRead, FALSE);
}


/*--------------------------------------------------------------
 * 	PeekConsoleInputW
 *
 * @implemented
 */
BOOL
WINAPI
PeekConsoleInputW(
	HANDLE			hConsoleInput,
	PINPUT_RECORD		lpBuffer,
	DWORD			nLength,
	LPDWORD			lpNumberOfEventsRead
	)    
{
  return IntPeekConsoleInput(hConsoleInput, lpBuffer, nLength,
                             lpNumberOfEventsRead, TRUE);
}


static BOOL
IntReadConsoleInput(HANDLE hConsoleInput,
                    PINPUT_RECORD lpBuffer,
                    DWORD nLength,
                    LPDWORD lpNumberOfEventsRead,
                    BOOL bUnicode)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  ULONG Read;
  NTSTATUS Status;
  
  Request.Type = CSRSS_READ_INPUT;
  Request.Data.ReadInputRequest.ConsoleHandle = hConsoleInput;
  Request.Data.ReadInputRequest.Unicode = bUnicode;

  Read = 0;
  while(nLength > 0)
  {
    Status = CsrClientCallServer(&Request, &Reply,
                                 sizeof(CSRSS_API_REQUEST),
                                 sizeof(CSRSS_API_REPLY));
    if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
    {
      if(Read == 0)
      {
        /* we couldn't read a single record, fail */
        SetLastErrorByStatus(Status);
        return FALSE;
      }
      else
      {
        /* FIXME - fail gracefully in case we already read at least one record? */
        break;
      }
    }
    else if(Status == STATUS_PENDING)
    {
      if(Read == 0)
      {
        Status = NtWaitForSingleObject(Reply.Data.ReadInputReply.Event, FALSE, 0);
        if(!NT_SUCCESS(Status))
        {
          SetLastErrorByStatus(Status);
          break;
        }
      }
      else
      {
        /* nothing more to read (waiting for more input??), let's just bail */
        break;
      }
    }
    else
    {
      lpBuffer[Read++] = Reply.Data.ReadInputReply.Input;
      nLength--;

      if(!Reply.Data.ReadInputReply.MoreEvents)
      {
        /* nothing more to read, bail */
        break;
      }
    }
  }
  
  if(lpNumberOfEventsRead != NULL)
  {
    *lpNumberOfEventsRead = Read;
  }
  
  return (Read > 0);
}


/*--------------------------------------------------------------
 * 	ReadConsoleInputA
 *
 * @implemented
 */
BOOL WINAPI
ReadConsoleInputA(HANDLE hConsoleInput,
		  PINPUT_RECORD	lpBuffer,
		  DWORD	nLength,
		  LPDWORD lpNumberOfEventsRead)
{
  return IntReadConsoleInput(hConsoleInput, lpBuffer, nLength,
                             lpNumberOfEventsRead, FALSE);
}


/*--------------------------------------------------------------
 * 	ReadConsoleInputW
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleInputW(
	HANDLE			hConsoleInput,
	PINPUT_RECORD		lpBuffer,
	DWORD			nLength,
	LPDWORD			lpNumberOfEventsRead
	)
{
  return IntReadConsoleInput(hConsoleInput, lpBuffer, nLength,
                             lpNumberOfEventsRead, TRUE);
}


static BOOL
IntWriteConsoleInput(HANDLE hConsoleInput,
                     PINPUT_RECORD lpBuffer,
                     DWORD nLength,
                     LPDWORD lpNumberOfEventsWritten,
                     BOOL bUnicode)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  PVOID BufferBase, BufferTargetBase;
  NTSTATUS Status;
  DWORD Size;

  if(lpBuffer == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }

  Size = nLength * sizeof(INPUT_RECORD);

  Status = CsrCaptureParameterBuffer((PVOID)lpBuffer, Size, &BufferBase, &BufferTargetBase);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  Request.Type = CSRSS_WRITE_CONSOLE_INPUT;
  Request.Data.WriteConsoleInputRequest.ConsoleHandle = hConsoleInput;
  Request.Data.WriteConsoleInputRequest.Unicode = bUnicode;
  Request.Data.WriteConsoleInputRequest.Length = nLength;
  Request.Data.WriteConsoleInputRequest.InputRecord = (PINPUT_RECORD)BufferTargetBase;

  Status = CsrClientCallServer(&Request, &Reply,
                               sizeof(CSRSS_API_REQUEST),
                               sizeof(CSRSS_API_REPLY));

  CsrReleaseParameterBuffer(BufferBase);
  
  if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  
  if(lpNumberOfEventsWritten != NULL)
  {
    *lpNumberOfEventsWritten = Reply.Data.WriteConsoleInputReply.Length;
  }

  return TRUE;
}


/*--------------------------------------------------------------
 * 	WriteConsoleInputA
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleInputA(
	HANDLE			 hConsoleInput,
	CONST INPUT_RECORD	*lpBuffer,
	DWORD			 nLength,
	LPDWORD			 lpNumberOfEventsWritten
	)
{
  return IntWriteConsoleInput(hConsoleInput,
                              (PINPUT_RECORD)lpBuffer,
                              nLength,
                              lpNumberOfEventsWritten,
                              FALSE);
}


/*--------------------------------------------------------------
 * 	WriteConsoleInputW
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleInputW(
	HANDLE			 hConsoleInput,
	CONST INPUT_RECORD	*lpBuffer,
	DWORD			 nLength,
	LPDWORD			 lpNumberOfEventsWritten
	)
{
  return IntWriteConsoleInput(hConsoleInput,
                              (PINPUT_RECORD)lpBuffer,
                              nLength,
                              lpNumberOfEventsWritten,
                              TRUE);
}


static BOOL
IntReadConsoleOutput(HANDLE hConsoleOutput,
                     PCHAR_INFO lpBuffer,
                     COORD dwBufferSize,
                     COORD dwBufferCoord,
                     PSMALL_RECT lpReadRegion,
                     BOOL bUnicode)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  PVOID BufferBase;
  PVOID BufferTargetBase;
  NTSTATUS Status;
  DWORD Size, SizeX, SizeY;
  
  if(lpBuffer == NULL)
  {
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Size = dwBufferSize.X * dwBufferSize.Y * sizeof(CHAR_INFO);

  Status = CsrCaptureParameterBuffer((PVOID)lpBuffer, Size, &BufferBase, &BufferTargetBase);
  if(!NT_SUCCESS(Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }
  
  Request = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CSRSS_API_REQUEST));
  if(Request == NULL)
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    CsrReleaseParameterBuffer(BufferBase);
    return FALSE;
  }
   
  Request->Type = CSRSS_READ_CONSOLE_OUTPUT;
  Request->Data.ReadConsoleOutputRequest.ConsoleHandle = hConsoleOutput;
  Request->Data.ReadConsoleOutputRequest.Unicode = bUnicode;
  Request->Data.ReadConsoleOutputRequest.BufferSize = dwBufferSize;
  Request->Data.ReadConsoleOutputRequest.BufferCoord = dwBufferCoord;
  Request->Data.ReadConsoleOutputRequest.ReadRegion = *lpReadRegion;
  Request->Data.ReadConsoleOutputRequest.CharInfo = (PCHAR_INFO)BufferTargetBase;
  
  Status = CsrClientCallServer(Request, &Reply,
                               sizeof(CSRSS_API_REQUEST),
                               sizeof(CSRSS_API_REPLY));

  if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus(Status);
    RtlFreeHeap(GetProcessHeap(), 0, Request);
    CsrReleaseParameterBuffer(BufferBase);
    return FALSE;
  }
  
  SizeX = Reply.Data.ReadConsoleOutputReply.ReadRegion.Right - Reply.Data.ReadConsoleOutputReply.ReadRegion.Left + 1;
  SizeY = Reply.Data.ReadConsoleOutputReply.ReadRegion.Bottom - Reply.Data.ReadConsoleOutputReply.ReadRegion.Top + 1;
  
  memcpy(lpBuffer, BufferBase, sizeof(CHAR_INFO) * SizeX * SizeY);
  
  RtlFreeHeap(GetProcessHeap(), 0, Request);
  CsrReleaseParameterBuffer(BufferBase);
  
  *lpReadRegion = Reply.Data.ReadConsoleOutputReply.ReadRegion;
  
  return TRUE;
}

/*--------------------------------------------------------------
 * 	ReadConsoleOutputA
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputA(
	HANDLE		hConsoleOutput,
	PCHAR_INFO	lpBuffer,
	COORD		dwBufferSize,
	COORD		dwBufferCoord,
	PSMALL_RECT	lpReadRegion
	)
{
  return IntReadConsoleOutput(hConsoleOutput, lpBuffer, dwBufferSize,
	                      dwBufferCoord, lpReadRegion, FALSE);
}


/*--------------------------------------------------------------
 * 	ReadConsoleOutputW
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputW(
	HANDLE		hConsoleOutput,
	PCHAR_INFO	lpBuffer,
	COORD		dwBufferSize,
	COORD		dwBufferCoord,
	PSMALL_RECT	lpReadRegion
	)
{
  return IntReadConsoleOutput(hConsoleOutput, lpBuffer, dwBufferSize,
	                      dwBufferCoord, lpReadRegion, TRUE);
}


static BOOL
IntWriteConsoleOutput(HANDLE hConsoleOutput,
                      CONST CHAR_INFO *lpBuffer,
                      COORD dwBufferSize,
                      COORD dwBufferCoord,
                      PSMALL_RECT lpWriteRegion,
                      BOOL bUnicode)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  ULONG Size;
  PVOID BufferBase;
  PVOID BufferTargetBase;

  Size = dwBufferSize.Y * dwBufferSize.X * sizeof(CHAR_INFO);

  Status = CsrCaptureParameterBuffer((PVOID)lpBuffer,
				     Size,
				     &BufferBase,
				     &BufferTargetBase);
  if (!NT_SUCCESS(Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }
  
  Request = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, 
			    sizeof(CSRSS_API_REQUEST));
  if (Request == NULL)
    {
      CsrReleaseParameterBuffer(BufferBase);
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return FALSE;
    }
  Request->Type = CSRSS_WRITE_CONSOLE_OUTPUT;
  Request->Data.WriteConsoleOutputRequest.ConsoleHandle = hConsoleOutput;
  Request->Data.WriteConsoleOutputRequest.Unicode = bUnicode;
  Request->Data.WriteConsoleOutputRequest.BufferSize = dwBufferSize;
  Request->Data.WriteConsoleOutputRequest.BufferCoord = dwBufferCoord;
  Request->Data.WriteConsoleOutputRequest.WriteRegion = *lpWriteRegion;
  Request->Data.WriteConsoleOutputRequest.CharInfo = 
    (CHAR_INFO*)BufferTargetBase;
  
  Status = CsrClientCallServer(Request, &Reply, 
			       sizeof(CSRSS_API_REQUEST), 
			       sizeof(CSRSS_API_REPLY));

  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
    {
      CsrReleaseParameterBuffer(BufferBase);
      RtlFreeHeap(GetProcessHeap(), 0, Request);
      SetLastErrorByStatus(Status);
      return FALSE;
    }
      
  RtlFreeHeap(GetProcessHeap(), 0, Request);
  CsrReleaseParameterBuffer(BufferBase);
  
  *lpWriteRegion = Reply.Data.WriteConsoleOutputReply.WriteRegion;
  
  return(TRUE);
}

/*--------------------------------------------------------------
 * 	WriteConsoleOutputA
 *
 * @implemented
 */
BOOL WINAPI
WriteConsoleOutputA(HANDLE		 hConsoleOutput,
		    CONST CHAR_INFO	*lpBuffer,
		    COORD		 dwBufferSize,
		    COORD		 dwBufferCoord,
		    PSMALL_RECT	 lpWriteRegion)
{
  return IntWriteConsoleOutput(hConsoleOutput, lpBuffer, dwBufferSize,
                               dwBufferCoord, lpWriteRegion, FALSE);
}


/*--------------------------------------------------------------
 * 	WriteConsoleOutputW
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleOutputW(
	HANDLE		 hConsoleOutput,
	CONST CHAR_INFO	*lpBuffer,
	COORD		 dwBufferSize,
	COORD		 dwBufferCoord,
	PSMALL_RECT	 lpWriteRegion
	)
{
  return IntWriteConsoleOutput(hConsoleOutput, lpBuffer, dwBufferSize,
                               dwBufferCoord, lpWriteRegion, TRUE);
}


static BOOL
IntReadConsoleOutputCharacter(HANDLE hConsoleOutput,
                              PVOID lpCharacter,
                              DWORD nLength,
                              COORD dwReadCoord,
                              LPDWORD lpNumberOfCharsRead,
                              BOOL bUnicode)
{
  CSRSS_API_REQUEST Request;
  PCSRSS_API_REPLY Reply;
  NTSTATUS Status;
  ULONG nChars, SizeBytes, CharSize;
  DWORD CharsRead = 0;
  
  CharSize = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
  
  nChars = min(nLength, CSRSS_MAX_READ_CONSOLE_OUTPUT_CHAR / CharSize);
  SizeBytes = nChars * CharSize;

  Reply = RtlAllocateHeap(GetProcessHeap(), 0,
			  sizeof(CSRSS_API_REPLY) + SizeBytes);
  if(Reply == NULL)
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }


  Request.Type = CSRSS_READ_CONSOLE_OUTPUT_CHAR;
  Request.Data.ReadConsoleOutputCharRequest.ConsoleHandle = hConsoleOutput;
  Request.Data.ReadConsoleOutputCharRequest.Unicode = bUnicode;
  Request.Data.ReadConsoleOutputCharRequest.ReadCoord = dwReadCoord;

  while(nLength > 0)
  {
    DWORD BytesRead;
    
    Request.Data.ReadConsoleOutputCharRequest.NumCharsToRead = min(nLength, nChars);

    Status = CsrClientCallServer(&Request,
                                 Reply,
                                 sizeof(CSRSS_API_REQUEST),
                                 sizeof(CSRSS_API_REPLY) + SizeBytes);
    if(!NT_SUCCESS(Status) || !NT_SUCCESS(Reply->Status))
    {
      SetLastErrorByStatus(Status);
      break;
    }

    BytesRead = Reply->Data.ReadConsoleOutputCharReply.CharsRead * CharSize;
    memcpy(lpCharacter, &Reply->Data.ReadConsoleOutputCharReply.String[0], BytesRead);
    lpCharacter = (PVOID)((ULONG_PTR)lpCharacter + (ULONG_PTR)BytesRead);
    CharsRead += Reply->Data.ReadConsoleOutputCharReply.CharsRead;
    nLength -= Reply->Data.ReadConsoleOutputCharReply.CharsRead;
    
    Request.Data.ReadConsoleOutputCharRequest.ReadCoord = Reply->Data.ReadConsoleOutputCharReply.EndCoord;
  }

  RtlFreeHeap(GetProcessHeap(), 0, Reply);
  
  if(lpNumberOfCharsRead != NULL)
  {
    *lpNumberOfCharsRead = CharsRead;
  }

  return TRUE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleOutputCharacterA
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputCharacterA(
	HANDLE		hConsoleOutput,
	LPSTR		lpCharacter,
	DWORD		nLength,
	COORD		dwReadCoord,
	LPDWORD		lpNumberOfCharsRead
	)
{
  return IntReadConsoleOutputCharacter(hConsoleOutput,
                                       (PVOID)lpCharacter,
                                       nLength,
                                       dwReadCoord,
                                       lpNumberOfCharsRead,
                                       FALSE);
}


/*--------------------------------------------------------------
 *      ReadConsoleOutputCharacterW
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputCharacterW(
	HANDLE		hConsoleOutput,
	LPWSTR		lpCharacter,
	DWORD		nLength,
	COORD		dwReadCoord,
	LPDWORD		lpNumberOfCharsRead
	)
{
  return IntReadConsoleOutputCharacter(hConsoleOutput,
                                       (PVOID)lpCharacter,
                                       nLength,
                                       dwReadCoord,
                                       lpNumberOfCharsRead,
                                       TRUE);
}


/*--------------------------------------------------------------
 * 	ReadConsoleOutputAttribute
 *
 * @implemented
 */
BOOL
WINAPI
ReadConsoleOutputAttribute(
	HANDLE		hConsoleOutput,
	LPWORD		lpAttribute,
	DWORD		nLength,
	COORD		dwReadCoord,
	LPDWORD		lpNumberOfAttrsRead
	)
{
  CSRSS_API_REQUEST Request;
  PCSRSS_API_REPLY Reply;
  NTSTATUS Status;
  DWORD Size, i;
  
  Reply = RtlAllocateHeap(GetProcessHeap(), 0,
			  sizeof(CSRSS_API_REPLY) + min(nLength, CSRSS_MAX_READ_CONSOLE_OUTPUT_ATTRIB));
  if (Reply == NULL)
    {
      SetLastError(ERROR_OUTOFMEMORY);
      return(FALSE);
    }

  if (lpNumberOfAttrsRead != NULL)
    *lpNumberOfAttrsRead = nLength;

  Request.Type = CSRSS_READ_CONSOLE_OUTPUT_ATTRIB;
  Request.Data.ReadConsoleOutputAttribRequest.ConsoleHandle = hConsoleOutput;
  Request.Data.ReadConsoleOutputAttribRequest.ReadCoord = dwReadCoord;

  while (nLength != 0)
    {
      if (nLength > CSRSS_MAX_READ_CONSOLE_OUTPUT_ATTRIB)
	Size = CSRSS_MAX_READ_CONSOLE_OUTPUT_ATTRIB;
      else
	Size = nLength;

      Request.Data.ReadConsoleOutputAttribRequest.NumAttrsToRead = Size;

      Status = CsrClientCallServer(&Request,
				   Reply,
				   sizeof(CSRSS_API_REQUEST),
				   sizeof(CSRSS_API_REPLY) + Size);
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Reply->Status))
	{
	  RtlFreeHeap(GetProcessHeap(), 0, Reply);
	  SetLastErrorByStatus(Status);
	  return(FALSE);
	}

      // Convert CHARs to WORDs
      for(i = 0; i < Size; ++i)
        *lpAttribute++ = Reply->Data.ReadConsoleOutputAttribReply.String[i];
      
      nLength -= Size;
      Request.Data.ReadConsoleOutputAttribRequest.ReadCoord = Reply->Data.ReadConsoleOutputAttribReply.EndCoord;
    }

  RtlFreeHeap(GetProcessHeap(), 0, Reply);

  return(TRUE);
}


static BOOL
IntWriteConsoleOutputCharacter(HANDLE hConsoleOutput,
                               PVOID lpCharacter,
                               DWORD nLength,
                               COORD dwWriteCoord,
                               LPDWORD lpNumberOfCharsWritten,
                               BOOL bUnicode)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  ULONG SizeBytes, CharSize, nChars;
  DWORD Written = 0;
  
  CharSize = (bUnicode ? sizeof(WCHAR) : sizeof(CHAR));
  
  nChars = min(nLength, CSRSS_MAX_WRITE_CONSOLE_REQUEST / CharSize);
  SizeBytes = nChars * CharSize;

  Request = RtlAllocateHeap(GetProcessHeap(), 0,
                            sizeof(CSRSS_API_REQUEST) + (nChars * CharSize));
  if(Request == NULL)
  {
    SetLastError(ERROR_NOT_ENOUGH_MEMORY);
    return FALSE;
  }

  Request->Type = CSRSS_WRITE_CONSOLE_OUTPUT_CHAR;
  Request->Data.WriteConsoleOutputCharRequest.ConsoleHandle = hConsoleOutput;
  Request->Data.WriteConsoleOutputCharRequest.Unicode = bUnicode;
  Request->Data.WriteConsoleOutputCharRequest.Coord = dwWriteCoord;

  while(nLength > 0)
  {
    DWORD BytesWrite;
    
    Request->Data.WriteConsoleOutputCharRequest.Length = min(nLength, nChars);
    BytesWrite = Request->Data.WriteConsoleOutputCharRequest.Length * CharSize;

    memcpy(&Request->Data.WriteConsoleOutputCharRequest.String[0], lpCharacter, BytesWrite);

    Status = CsrClientCallServer(Request, &Reply,
                                 sizeof(CSRSS_API_REQUEST) + BytesWrite,
                                 sizeof(CSRSS_API_REPLY));

    if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
    {
      RtlFreeHeap(GetProcessHeap(), 0, Request);
      SetLastErrorByStatus(Status);
      return FALSE;
    }

    nLength -= Reply.Data.WriteConsoleOutputCharReply.NrCharactersWritten;
    lpCharacter = (PVOID)((ULONG_PTR)lpCharacter + (ULONG_PTR)(Reply.Data.WriteConsoleOutputCharReply.NrCharactersWritten * CharSize));
    Written += Reply.Data.WriteConsoleOutputCharReply.NrCharactersWritten;

    Request->Data.WriteConsoleOutputCharRequest.Coord = Reply.Data.WriteConsoleOutputCharReply.EndCoord;
  }

  RtlFreeHeap(GetProcessHeap(), 0, Request);
  
  if(lpNumberOfCharsWritten != NULL)
  {
    *lpNumberOfCharsWritten = Written;
  }

  return TRUE;
}


/*--------------------------------------------------------------
 * 	WriteConsoleOutputCharacterA
 *
 * @implemented
 */
BOOL WINAPI
WriteConsoleOutputCharacterA(HANDLE		hConsoleOutput,
			     LPCSTR		lpCharacter,
			     DWORD		nLength,
			     COORD		dwWriteCoord,
			     LPDWORD		lpNumberOfCharsWritten)
{
  return IntWriteConsoleOutputCharacter(hConsoleOutput,
                                        (PVOID)lpCharacter,
                                        nLength,
                                        dwWriteCoord,
                                        lpNumberOfCharsWritten,
                                        FALSE);
}


/*--------------------------------------------------------------
 * 	WriteConsoleOutputCharacterW
 *
 * @implemented
 */
BOOL WINAPI
WriteConsoleOutputCharacterW(HANDLE		hConsoleOutput,
			     LPCWSTR		lpCharacter,
			     DWORD		nLength,
			     COORD		dwWriteCoord,
			     LPDWORD		lpNumberOfCharsWritten)
{
  return IntWriteConsoleOutputCharacter(hConsoleOutput,
                                        (PVOID)lpCharacter,
                                        nLength,
                                        dwWriteCoord,
                                        lpNumberOfCharsWritten,
                                        TRUE);
}


/*--------------------------------------------------------------
 * 	WriteConsoleOutputAttribute
 *
 * @implemented
 */
BOOL
WINAPI
WriteConsoleOutputAttribute(
	HANDLE		 hConsoleOutput,
	CONST WORD	*lpAttribute,
	DWORD		 nLength,
	COORD		 dwWriteCoord,
	LPDWORD		 lpNumberOfAttrsWritten
	)
{
   PCSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;
   WORD Size;
   int c;

   Request = RtlAllocateHeap(GetProcessHeap(), 0,
		             sizeof(CSRSS_API_REQUEST) +
		             min(nLength, CSRSS_MAX_WRITE_CONSOLE_OUTPUT_ATTRIB));
   if( !Request )
     {
       SetLastError( ERROR_OUTOFMEMORY );
       return FALSE;
     }
   Request->Type = CSRSS_WRITE_CONSOLE_OUTPUT_ATTRIB;
   Request->Data.WriteConsoleOutputAttribRequest.ConsoleHandle = hConsoleOutput;
   Request->Data.WriteConsoleOutputAttribRequest.Coord = dwWriteCoord;
   if( lpNumberOfAttrsWritten )
      *lpNumberOfAttrsWritten = nLength;
   while( nLength )
      {
	 Size = nLength > CSRSS_MAX_WRITE_CONSOLE_OUTPUT_ATTRIB ? CSRSS_MAX_WRITE_CONSOLE_OUTPUT_ATTRIB : nLength;
	 Request->Data.WriteConsoleOutputAttribRequest.Length = Size;
	 for( c = 0; c < Size; c++ )
	   Request->Data.WriteConsoleOutputAttribRequest.String[c] = (char)lpAttribute[c];
	 Status = CsrClientCallServer( Request, &Reply, sizeof( CSRSS_API_REQUEST ) + (Size * 2), sizeof( CSRSS_API_REPLY ) );
	 if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
	    {
	       RtlFreeHeap( GetProcessHeap(), 0, Request );
	       SetLastErrorByStatus ( Status );
	       return FALSE;
	    }
	 nLength -= Size;
	 lpAttribute += Size;
	 Request->Data.WriteConsoleOutputAttribRequest.Coord = Reply.Data.WriteConsoleOutputAttribReply.EndCoord;
      }
   
   RtlFreeHeap( GetProcessHeap(), 0, Request );
   return TRUE;
}


/*--------------------------------------------------------------
 * 	FillConsoleOutputAttribute
 *
 * @implemented
 */
BOOL
WINAPI
FillConsoleOutputAttribute(
	HANDLE		hConsoleOutput,
	WORD		wAttribute,
	DWORD		nLength,
	COORD		dwWriteCoord,
	LPDWORD		lpNumberOfAttrsWritten
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_FILL_OUTPUT_ATTRIB;
   Request.Data.FillOutputAttribRequest.ConsoleHandle = hConsoleOutput;
   Request.Data.FillOutputAttribRequest.Attribute = wAttribute;
   Request.Data.FillOutputAttribRequest.Coord = dwWriteCoord;
   Request.Data.FillOutputAttribRequest.Length = nLength;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   if( lpNumberOfAttrsWritten )
      *lpNumberOfAttrsWritten = nLength;
   return TRUE;
}


/*--------------------------------------------------------------
 * 	GetConsoleMode
 *
 * @implemented
 */
BOOL
WINAPI
GetConsoleMode(
	HANDLE		hConsoleHandle,
	LPDWORD		lpMode
	)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  
  Request.Type = CSRSS_GET_CONSOLE_MODE;
  Request.Data.GetConsoleModeRequest.ConsoleHandle = hConsoleHandle;
  Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
  if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	SetLastErrorByStatus ( Status );
	return FALSE;
      }
  *lpMode = Reply.Data.GetConsoleModeReply.ConsoleMode;
  return TRUE;
}


/*--------------------------------------------------------------
 * 	GetNumberOfConsoleInputEvents
 *
 * @implemented
 */
BOOL
WINAPI
GetNumberOfConsoleInputEvents(
	HANDLE		hConsoleInput,
	LPDWORD		lpNumberOfEvents
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;
 
   if(lpNumberOfEvents == NULL)
   {
      SetLastError(ERROR_INVALID_PARAMETER);
      return FALSE;
   }
   
   Request.Type = CSRSS_GET_NUM_INPUT_EVENTS;
   Request.Data.GetNumInputEventsRequest.ConsoleHandle = hConsoleInput;
   Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST), sizeof(CSRSS_API_REPLY));
   if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
   {
      SetLastErrorByStatus(Status);
      return FALSE;
   }
   
   *lpNumberOfEvents = Reply.Data.GetNumInputEventsReply.NumInputEvents;
   
	return TRUE;
}


/*--------------------------------------------------------------
 * 	GetLargestConsoleWindowSize
 *
 * @unimplemented
 */
COORD
WINAPI
GetLargestConsoleWindowSize(
	HANDLE		hConsoleOutput
	)
{
  COORD Coord = {80,25};
  DPRINT1("GetLargestConsoleWindowSize(0x%x) UNIMPLEMENTED!\n", hConsoleOutput);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return Coord;
}


/*--------------------------------------------------------------
 *	GetConsoleCursorInfo
 *
 * @implemented
 */
BOOL
WINAPI
GetConsoleCursorInfo(
	HANDLE			hConsoleOutput,
	PCONSOLE_CURSOR_INFO	lpConsoleCursorInfo
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_GET_CURSOR_INFO;
   Request.Data.GetCursorInfoRequest.ConsoleHandle = hConsoleOutput;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );

   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   *lpConsoleCursorInfo = Reply.Data.GetCursorInfoReply.Info;
   return TRUE;
}


/*--------------------------------------------------------------
 * 	GetNumberOfConsoleMouseButtons
 *
 * @unimplemented
 */
BOOL
WINAPI
GetNumberOfConsoleMouseButtons(
	LPDWORD		lpNumberOfMouseButtons
	)
{
  DPRINT1("GetNumberOfConsoleMouseButtons(0x%x) UNIMPLEMENTED!\n", lpNumberOfMouseButtons);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*--------------------------------------------------------------
 * 	SetConsoleMode
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleMode(
	HANDLE		hConsoleHandle,
	DWORD		dwMode
	)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  
  Request.Type = CSRSS_SET_CONSOLE_MODE;
  Request.Data.SetConsoleModeRequest.ConsoleHandle = hConsoleHandle;
  Request.Data.SetConsoleModeRequest.Mode = dwMode;
  Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
  if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	SetLastErrorByStatus ( Status );
	return FALSE;
      }
  return TRUE;
}


/*--------------------------------------------------------------
 * 	SetConsoleActiveScreenBuffer
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleActiveScreenBuffer(
	HANDLE		hConsoleOutput
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_SET_SCREEN_BUFFER;
   Request.Data.SetScreenBufferRequest.OutputHandle = hConsoleOutput;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   return TRUE;
}


/*--------------------------------------------------------------
 * 	FlushConsoleInputBuffer
 *
 * @implemented
 */
BOOL
WINAPI
FlushConsoleInputBuffer(
	HANDLE		hConsoleInput
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_FLUSH_INPUT_BUFFER;
   Request.Data.FlushInputBufferRequest.ConsoleInput = hConsoleInput;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   return TRUE;
}


/*--------------------------------------------------------------
 * 	SetConsoleScreenBufferSize
 *
 * @unimplemented
 */
BOOL
WINAPI
SetConsoleScreenBufferSize(
	HANDLE		hConsoleOutput,
	COORD		dwSize
	)
{
  DPRINT1("SetConsoleScreenBufferSize(0x%x, 0x%x) UNIMPLEMENTED!\n", hConsoleOutput, dwSize);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/*--------------------------------------------------------------
 * 	SetConsoleCursorInfo
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleCursorInfo(
	HANDLE				 hConsoleOutput,
	CONST CONSOLE_CURSOR_INFO	*lpConsoleCursorInfo
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_SET_CURSOR_INFO;
   Request.Data.SetCursorInfoRequest.ConsoleHandle = hConsoleOutput;
   Request.Data.SetCursorInfoRequest.Info = *lpConsoleCursorInfo;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );

   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   return TRUE;
}


static BOOL
IntScrollConsoleScreenBuffer(HANDLE hConsoleOutput,
                             PSMALL_RECT lpScrollRectangle,
                             PSMALL_RECT lpClipRectangle,
                             COORD dwDestinationOrigin,
                             PCHAR_INFO lpFill,
                             BOOL bUnicode)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request.Type = CSRSS_SCROLL_CONSOLE_SCREEN_BUFFER;
  Request.Data.ScrollConsoleScreenBufferRequest.ConsoleHandle = hConsoleOutput;
  Request.Data.ScrollConsoleScreenBufferRequest.Unicode = bUnicode;
  Request.Data.ScrollConsoleScreenBufferRequest.ScrollRectangle = *lpScrollRectangle;

  if(lpClipRectangle != NULL)
  {
    Request.Data.ScrollConsoleScreenBufferRequest.UseClipRectangle = TRUE;
    Request.Data.ScrollConsoleScreenBufferRequest.ClipRectangle = *lpClipRectangle;
  }
  else
  {
    Request.Data.ScrollConsoleScreenBufferRequest.UseClipRectangle = FALSE;
  }

  Request.Data.ScrollConsoleScreenBufferRequest.DestinationOrigin = dwDestinationOrigin;
  Request.Data.ScrollConsoleScreenBufferRequest.Fill = *lpFill;
  Status = CsrClientCallServer(&Request, &Reply,
                               sizeof(CSRSS_API_REQUEST),
                               sizeof(CSRSS_API_REPLY));

  if(!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus(Status);
    return FALSE;
  }

  return TRUE;
}


/*--------------------------------------------------------------
 *	ScrollConsoleScreenBufferA
 *
 * @implemented
 */
BOOL
WINAPI
ScrollConsoleScreenBufferA(
	HANDLE			 hConsoleOutput,
	CONST SMALL_RECT	*lpScrollRectangle,
	CONST SMALL_RECT	*lpClipRectangle,
	COORD			 dwDestinationOrigin,
	CONST CHAR_INFO		*lpFill
	)
{
  return IntScrollConsoleScreenBuffer(hConsoleOutput,
                                      (PSMALL_RECT)lpScrollRectangle,
                                      (PSMALL_RECT)lpClipRectangle,
                                      dwDestinationOrigin,
                                      (PCHAR_INFO)lpFill,
                                      FALSE);
}


/*--------------------------------------------------------------
 * 	ScrollConsoleScreenBufferW
 *
 * @implemented
 */
BOOL
WINAPI
ScrollConsoleScreenBufferW(
	HANDLE			 hConsoleOutput,
	CONST SMALL_RECT	*lpScrollRectangle,
	CONST SMALL_RECT	*lpClipRectangle,
	COORD			 dwDestinationOrigin,
	CONST CHAR_INFO		*lpFill
	)
{
  return IntScrollConsoleScreenBuffer(hConsoleOutput,
                                      (PSMALL_RECT)lpScrollRectangle,
                                      (PSMALL_RECT)lpClipRectangle,
                                      dwDestinationOrigin,
                                      (PCHAR_INFO)lpFill,
                                      TRUE);
}


/*--------------------------------------------------------------
 * 	SetConsoleWindowInfo
 *
 * @unimplemented
 */
BOOL
WINAPI
SetConsoleWindowInfo(
	HANDLE			 hConsoleOutput,
	BOOL			 bAbsolute,
	CONST SMALL_RECT	*lpConsoleWindow
	)
{
  DPRINT1("SetConsoleWindowInfo(0x%x, 0x%x, 0x%x) UNIMPLEMENTED!\n", hConsoleOutput, bAbsolute, lpConsoleWindow);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*--------------------------------------------------------------
 *      SetConsoleTextAttribute
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleTextAttribute(
        HANDLE		hConsoleOutput,
        WORD            wAttributes
        )
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_SET_ATTRIB;
   Request.Data.SetAttribRequest.ConsoleHandle = hConsoleOutput;
   Request.Data.SetAttribRequest.Attrib = wAttributes;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   return TRUE;
}


BOOL STATIC
AddConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine)
{
  if (HandlerRoutine == NULL)
    {
      IgnoreCtrlEvents = TRUE;
      return(TRUE);
    }
  else
    {
      NrCtrlHandlers++;
      if (CtrlHandlers == NULL)
        {
          CtrlHandlers = RtlAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                         NrCtrlHandlers * sizeof(PHANDLER_ROUTINE)); 
        }
      else
        {
          CtrlHandlers = RtlReAllocateHeap(RtlGetProcessHeap(), HEAP_ZERO_MEMORY,
                                           (PVOID)CtrlHandlers,
                                           NrCtrlHandlers * sizeof(PHANDLER_ROUTINE)); 
        }
      if (CtrlHandlers == NULL)
	{
	  NrCtrlHandlers = 0;
	  SetLastError(ERROR_NOT_ENOUGH_MEMORY);
	  return(FALSE);
	}
      CtrlHandlers[NrCtrlHandlers - 1] = HandlerRoutine;
      return(TRUE);
    }
}


BOOL STATIC
RemoveConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine)
{
  ULONG i;

  if (HandlerRoutine == NULL)
    {
      IgnoreCtrlEvents = FALSE;
      return(TRUE);
    }
  else
    {
      for (i = 0; i < NrCtrlHandlers; i++)
	{
	  if ( ((void*)(CtrlHandlers[i])) == (void*)HandlerRoutine)
	    {
	      NrCtrlHandlers--;
	      memmove(CtrlHandlers + i, CtrlHandlers + i + 1, 
		      (NrCtrlHandlers - i) * sizeof(PHANDLER_ROUTINE));
	      CtrlHandlers = 
		RtlReAllocateHeap(RtlGetProcessHeap(),
				  HEAP_ZERO_MEMORY,
				  (PVOID)CtrlHandlers,
				  NrCtrlHandlers * sizeof(PHANDLER_ROUTINE));
	      return(TRUE);
	    }
	}
    }
  return(FALSE);
}


/*
 * @implemented
 */
BOOL WINAPI
SetConsoleCtrlHandler(PHANDLER_ROUTINE HandlerRoutine,
		      BOOL Add)
{
  BOOLEAN Ret;

  RtlEnterCriticalSection(&DllLock);
  if (Add)
    {
      Ret = AddConsoleCtrlHandler(HandlerRoutine);
    }
  else
    {
      Ret = RemoveConsoleCtrlHandler(HandlerRoutine);
    }
  RtlLeaveCriticalSection(&DllLock);
  return(Ret);
}


/*--------------------------------------------------------------
 * 	GenerateConsoleCtrlEvent
 *
 * @unimplemented
 */
BOOL WINAPI
GenerateConsoleCtrlEvent(
	DWORD		dwCtrlEvent,
	DWORD		dwProcessGroupId
	)
{
  DPRINT1("GenerateConsoleCtrlEvent(0x%x, 0x%x) UNIMPLEMENTED!\n", dwCtrlEvent, dwProcessGroupId);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}


/*--------------------------------------------------------------
 *	GetConsoleTitleW
 *
 * @implemented
 */
DWORD
WINAPI
GetConsoleTitleW(
	LPWSTR		lpConsoleTitle,
	DWORD		nSize
	)
{
   CSRSS_API_REQUEST Request;
   PCSRSS_API_REPLY Reply;
   NTSTATUS Status;
   HANDLE hConsole;

   hConsole = CreateFileW(L"CONIN$", GENERIC_READ, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
   if (hConsole == INVALID_HANDLE_VALUE)
   {
      return 0;
   }

   Reply = RtlAllocateHeap(GetProcessHeap(), HEAP_ZERO_MEMORY, sizeof(CSRSS_API_REPLY) + CSRSS_MAX_TITLE_LENGTH * sizeof(WCHAR));
   if(Reply == NULL)
   {
      CloseHandle(hConsole);   
      SetLastError(ERROR_OUTOFMEMORY);
      return 0;
   }

   Request.Type = CSRSS_GET_TITLE;
   Request.Data.GetTitleRequest.ConsoleHandle = hConsole;
   
   Status = CsrClientCallServer(&Request, Reply, sizeof(CSRSS_API_REQUEST), sizeof(CSRSS_API_REPLY) + CSRSS_MAX_TITLE_LENGTH * sizeof(WCHAR));
   CloseHandle(hConsole);
   if(!NT_SUCCESS(Status) || !(NT_SUCCESS(Status = Reply->Status)))
   {
      SetLastErrorByStatus(Status);
      RtlFreeHeap(GetProcessHeap(), 0, Reply);
      return 0;
   }
   
   if(nSize * sizeof(WCHAR) < Reply->Data.GetTitleReply.Length)
   {
      wcsncpy(lpConsoleTitle, Reply->Data.GetTitleReply.Title, nSize - 1);
      lpConsoleTitle[nSize--] = L'\0';
   }
   else
   {  
      nSize = Reply->Data.GetTitleReply.Length / sizeof (WCHAR);
      wcscpy(lpConsoleTitle, Reply->Data.GetTitleReply.Title);
      lpConsoleTitle[nSize] = L'\0';
   }
   
   RtlFreeHeap(GetProcessHeap(), 0, Reply);
   return nSize;
}


/*--------------------------------------------------------------
 * 	GetConsoleTitleA
 *
 * 	19990306 EA
 *
 * @implemented
 */
DWORD
WINAPI
GetConsoleTitleA(
	LPSTR		lpConsoleTitle,
	DWORD		nSize
	)
{
	wchar_t	WideTitle [CSRSS_MAX_TITLE_LENGTH];
	DWORD	nWideTitle = sizeof WideTitle;
	DWORD	nWritten;
	
	if (!lpConsoleTitle || !nSize) return 0;
	nWideTitle = GetConsoleTitleW( (LPWSTR) WideTitle, nWideTitle );
	if (!nWideTitle) return 0;

	if ( (nWritten = WideCharToMultiByte(
    		CP_ACP,			// ANSI code page 
		0,			// performance and mapping flags 
		(LPWSTR) WideTitle,	// address of wide-character string 
		nWideTitle,		// number of characters in string 
		lpConsoleTitle,		// address of buffer for new string 
		nSize,			// size of buffer 
		NULL,			// FAST
		NULL	 		// FAST
		)))
	{
		lpConsoleTitle[nWritten] = '\0';
		return nWritten;
	}

	return 0;
}


/*--------------------------------------------------------------
 *	SetConsoleTitleW
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleTitleW(
	LPCWSTR		lpConsoleTitle
	)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  unsigned int c;
  HANDLE hConsole;

  hConsole = CreateFileW(L"CONIN$", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hConsole == INVALID_HANDLE_VALUE)
  {
     return FALSE;
  }
  
  Request = RtlAllocateHeap(GetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    sizeof(CSRSS_API_REQUEST) + CSRSS_MAX_SET_TITLE_REQUEST);
  if (Request == NULL)
    {
      CloseHandle(hConsole);
      SetLastError(ERROR_OUTOFMEMORY);
      return(FALSE);
    }
  
  Request->Type = CSRSS_SET_TITLE;
  Request->Data.SetTitleRequest.Console = hConsole;
  
  for( c = 0; lpConsoleTitle[c] && c < CSRSS_MAX_TITLE_LENGTH; c++ )
    Request->Data.SetTitleRequest.Title[c] = lpConsoleTitle[c];
  // add null
  Request->Data.SetTitleRequest.Title[c] = 0;
  Request->Data.SetTitleRequest.Length = c;  
  Status = CsrClientCallServer(Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST) + 
			       c * sizeof(WCHAR),
			       sizeof(CSRSS_API_REPLY));
  CloseHandle(hConsole);
  if (!NT_SUCCESS(Status) || !NT_SUCCESS( Status = Reply.Status ) )
    {
      RtlFreeHeap( GetProcessHeap(), 0, Request );
      SetLastErrorByStatus (Status);
      return(FALSE);
    }
  RtlFreeHeap( GetProcessHeap(), 0, Request );
  return TRUE;
}


/*--------------------------------------------------------------
 *	SetConsoleTitleA
 *	
 * 	19990204 EA	Added
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleTitleA(
	LPCSTR		lpConsoleTitle
	)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  unsigned int c;
  HANDLE hConsole;

  hConsole = CreateFileW(L"CONIN$", GENERIC_WRITE, 0, NULL, OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
  if (hConsole == INVALID_HANDLE_VALUE)
  {
     return FALSE;
  }
  
  Request = RtlAllocateHeap(GetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    sizeof(CSRSS_API_REQUEST) + CSRSS_MAX_SET_TITLE_REQUEST);
  if (Request == NULL)
    {
      CloseHandle(hConsole);
      SetLastError(ERROR_OUTOFMEMORY);
      return(FALSE);
    }
  
  Request->Type = CSRSS_SET_TITLE;
  Request->Data.SetTitleRequest.Console = hConsole;
  
  for( c = 0; lpConsoleTitle[c] && c < CSRSS_MAX_TITLE_LENGTH; c++ )
    Request->Data.SetTitleRequest.Title[c] = lpConsoleTitle[c];
  // add null
  Request->Data.SetTitleRequest.Title[c] = 0;
  Request->Data.SetTitleRequest.Length = c;
  Status = CsrClientCallServer(Request,
			       &Reply,
			       sizeof(CSRSS_API_REQUEST) + 
			       c * sizeof(WCHAR),
			       sizeof(CSRSS_API_REPLY));
  CloseHandle(hConsole);
  if (!NT_SUCCESS(Status) || !NT_SUCCESS( Status = Reply.Status ) )
    {
      RtlFreeHeap( GetProcessHeap(), 0, Request );
      SetLastErrorByStatus (Status);
      return(FALSE);
    }
  RtlFreeHeap( GetProcessHeap(), 0, Request );
  return TRUE;
}


/*--------------------------------------------------------------
 *	CreateConsoleScreenBuffer
 *
 * @implemented
 */
HANDLE
WINAPI
CreateConsoleScreenBuffer(
	DWORD				 dwDesiredAccess,
	DWORD				 dwShareMode,
	CONST SECURITY_ATTRIBUTES	*lpSecurityAttributes,
	DWORD				 dwFlags,
	LPVOID				 lpScreenBufferData
	)
{
   // FIXME: don't ignore access, share mode, and security
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_CREATE_SCREEN_BUFFER;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   return Reply.Data.CreateScreenBufferReply.OutputHandle;
}


/*--------------------------------------------------------------
 *	GetConsoleCP
 *
 * @implemented
 */
UINT
WINAPI
GetConsoleCP( VOID )
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  NTSTATUS          Status;
   
  Request.Type = CSRSS_GET_CONSOLE_CP;
  Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST),
                               sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus (Status);
    return 0;
  }
  return Reply.Data.GetConsoleCodePage.CodePage;
}


/*--------------------------------------------------------------
 *	SetConsoleCP
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleCP(
	UINT		wCodePageID
	)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  NTSTATUS          Status;
   
  Request.Type = CSRSS_SET_CONSOLE_CP;
  Request.Data.SetConsoleCodePage.CodePage = wCodePageID;
  Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST),
                               sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus (Status);
  }
  return NT_SUCCESS(Status);
}


/*--------------------------------------------------------------
 *	GetConsoleOutputCP
 *
 * @implemented
 */
UINT
WINAPI
GetConsoleOutputCP( VOID )
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  NTSTATUS          Status;
   
  Request.Type = CSRSS_GET_CONSOLE_OUTPUT_CP;
  Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST),
                               sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus (Status);
    return 0;
  }
  return Reply.Data.GetConsoleOutputCodePage.CodePage;
}


/*--------------------------------------------------------------
 *	SetConsoleOutputCP
 *
 * @implemented
 */
BOOL
WINAPI
SetConsoleOutputCP(
	UINT		wCodePageID
	)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  NTSTATUS          Status;
   
  Request.Type = CSRSS_SET_CONSOLE_OUTPUT_CP;
  Request.Data.SetConsoleOutputCodePage.CodePage = wCodePageID;
  Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST),
                               sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus (Status);
  }
  return NT_SUCCESS(Status);
}


/*--------------------------------------------------------------
 * 	GetConsoleProcessList
 *
 * @unimplemented
 */
DWORD STDCALL
GetConsoleProcessList(LPDWORD lpdwProcessList,
                  DWORD dwProcessCount)
{
  DPRINT1("GetConsoleProcessList(0x%x, 0x%x) UNIMPLEMENTED!\n", lpdwProcessList, dwProcessCount);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}



/*--------------------------------------------------------------
 * 	GetConsoleSelectionInfo
 *
 * @unimplemented
 */
BOOL STDCALL
GetConsoleSelectionInfo(PCONSOLE_SELECTION_INFO lpConsoleSelectionInfo)
{
  DPRINT1("GetConsoleSelectionInfo(0x%x) UNIMPLEMENTED!\n", lpConsoleSelectionInfo);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}



/*--------------------------------------------------------------
 * 	AttachConsole
 *
 * @unimplemented
 */
BOOL STDCALL 
AttachConsole(DWORD dwProcessId)
{
  DPRINT1("AttachConsole(0x%x) UNIMPLEMENTED!\n", dwProcessId);
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

/*--------------------------------------------------------------
 * 	GetConsoleWindow
 *
 * @implemented
 */
HWND STDCALL
GetConsoleWindow (VOID)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  NTSTATUS          Status;
   
  Request.Type = CSRSS_GET_CONSOLE_WINDOW;
  Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
  if (!NT_SUCCESS(Status ) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus (Status);
    return (HWND) NULL;
  }
  return Reply.Data.GetConsoleWindowReply.WindowHandle;
}


/*--------------------------------------------------------------
 * 	SetConsoleIcon
 *
 * @implemented
 */
BOOL STDCALL SetConsoleIcon(HICON hicon)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY   Reply;
  NTSTATUS          Status;
  
  Request.Type = CSRSS_SET_CONSOLE_ICON;
  Request.Data.SetConsoleIconRequest.WindowIcon = hicon;
  Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
  {
    SetLastErrorByStatus (Status);
    return FALSE;
  }
  return NT_SUCCESS(Status);
}


/*--------------------------------------------------------------
 * 	SetConsoleInputExeNameW
 *
 * @implemented
 */
BOOL STDCALL
SetConsoleInputExeNameW(LPCWSTR lpInputExeName)
{
  BOOL Ret = FALSE;
  int lenName = lstrlenW(lpInputExeName);

  if(lenName < 1 ||
     lenName > (sizeof(InputExeName) / sizeof(InputExeName[0])) - 1)
  {
    /* Fail if string is empty or too long */
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  RtlEnterCriticalSection(&ConsoleLock);
  /* wrap copying into SEH as we may copy from invalid buffer and in case of an
     exception the console lock would've never been released, which would cause
     further calls (if the exception was handled by the caller) to recursively
     acquire the lock... */
  _SEH_TRY
  {
    RtlCopyMemory(InputExeName, lpInputExeName, lenName * sizeof(WCHAR));
    InputExeName[lenName] = L'\0';
    Ret = TRUE;
  }
  _SEH_HANDLE
  {
    lenName = 0;
    SetLastErrorByStatus(_SEH_GetExceptionCode());
  }
  _SEH_END;
  RtlLeaveCriticalSection(&ConsoleLock);
  
  return Ret;
}


/*--------------------------------------------------------------
 * 	SetConsoleInputExeNameA
 *
 * @implemented
 */
BOOL STDCALL
SetConsoleInputExeNameA(LPCSTR lpInputExeName)
{
  ANSI_STRING InputExeNameA;
  UNICODE_STRING InputExeNameU;
  NTSTATUS Status;
  BOOL Ret;
  
  RtlInitAnsiString(&InputExeNameA, lpInputExeName);
  
  if(InputExeNameA.Length < sizeof(InputExeNameA.Buffer[0]) ||
     InputExeNameA.Length >= (sizeof(InputExeName) / sizeof(InputExeName[0])) - 1)
  {
    /* Fail if string is empty or too long */
    SetLastError(ERROR_INVALID_PARAMETER);
    return FALSE;
  }
  
  Status = RtlAnsiStringToUnicodeString(&InputExeNameU, &InputExeNameA, TRUE);
  if(NT_SUCCESS(Status))
  {
    Ret = SetConsoleInputExeNameW(InputExeNameU.Buffer);
    RtlFreeUnicodeString(&InputExeNameU);
  }
  else
  {
    SetLastErrorByStatus(Status);
    Ret = FALSE;
  }

  return Ret;
}


/*--------------------------------------------------------------
 * 	GetConsoleInputExeNameW
 *
 * @implemented
 */
DWORD STDCALL
GetConsoleInputExeNameW(DWORD nBufferLength, LPWSTR lpBuffer)
{
  int lenName;

  RtlEnterCriticalSection(&ConsoleLock);

  lenName = lstrlenW(InputExeName);
  if(lenName >= nBufferLength)
  {
    /* buffer is not large enough, return the required size */
    RtlLeaveCriticalSection(&ConsoleLock);
    return lenName + 1;
  }

  /* wrap copying into SEH as we may copy to invalid buffer and in case of an
     exception the console lock would've never been released, which would cause
     further calls (if the exception was handled by the caller) to recursively
     acquire the lock... */
  _SEH_TRY
  {
    RtlCopyMemory(lpBuffer, InputExeName, (lenName + 1) * sizeof(WCHAR));
  }
  _SEH_HANDLE
  {
    lenName = 0;
    SetLastErrorByStatus(_SEH_GetExceptionCode());
  }
  _SEH_END;
  
  RtlLeaveCriticalSection(&ConsoleLock);

  return lenName;
}


/*--------------------------------------------------------------
 * 	GetConsoleInputExeNameA
 *
 * @implemented
 */
DWORD STDCALL
GetConsoleInputExeNameA(DWORD nBufferLength, LPSTR lpBuffer)
{
  WCHAR *Buffer;
  DWORD Ret;
  
  if(nBufferLength > 0)
  {
    Buffer = RtlAllocateHeap(RtlGetProcessHeap(), 0, nBufferLength * sizeof(WCHAR));
    if(Buffer == NULL)
    {
      SetLastError(ERROR_NOT_ENOUGH_MEMORY);
      return 0;
    }
  }
  else
  {
    Buffer = NULL;
  }
  
  Ret = GetConsoleInputExeNameW(nBufferLength, Buffer);
  if(nBufferLength > 0)
  {
    if(Ret > 0)
    {
      UNICODE_STRING BufferU;
      ANSI_STRING BufferA;
      
      RtlInitUnicodeString(&BufferU, Buffer);

      BufferA.Length = 0;
      BufferA.MaximumLength = nBufferLength;
      BufferA.Buffer = lpBuffer;
      
      RtlUnicodeStringToAnsiString(&BufferA, &BufferU, FALSE);
    }
    
    RtlFreeHeap(RtlGetProcessHeap(), 0, Buffer);
  }
  
  return Ret;
}

/* EOF */
