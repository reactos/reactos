/* $Id: console.c,v 1.39 2001/12/20 03:56:09 dwelch Exp $
 *
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS system libraries
 * FILE:            lib/kernel32/misc/console.c
 * PURPOSE:         Win32 server console functions
 * PROGRAMMER:      ???
 * UPDATE HISTORY:
 *	199901?? ??	Created
 *	19990204 EA	SetConsoleTitleA
 *      19990306 EA	Stubs
 */

/* INCLUDES ******************************************************************/

#include <ddk/ntddk.h>
#include <ddk/ntddblue.h>
#include <windows.h>
#include <assert.h>
#include <wchar.h>

#include <csrss/csrss.h>
#include <ntdll/csr.h>

#define NDEBUG
#include <kernel32/kernel32.h>
#include <kernel32/error.h>

/* GLOBALS *******************************************************************/

static BOOL IgnoreCtrlEvents = FALSE;
static ULONG NrCtrlHandlers = 0;
static PHANDLER_ROUTINE* CtrlHandlers = NULL;

/* FUNCTIONS *****************************************************************/

BOOL STDCALL
AddConsoleAliasA (DWORD a0,
		  DWORD a1,
		  DWORD a2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

BOOL STDCALL
AddConsoleAliasW (DWORD a0,
		  DWORD a1,
		  DWORD a2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

BOOL STDCALL
ConsoleMenuControl (HANDLE	hConsole,
		    DWORD	Unknown1,
		    DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

BOOL STDCALL
DuplicateConsoleHandle (HANDLE	hConsole,
			DWORD	Unknown1,
			DWORD	Unknown2,
			DWORD	Unknown3)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

DWORD STDCALL
ExpungeConsoleCommandHistoryW (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


DWORD STDCALL
ExpungeConsoleCommandHistoryA (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleAliasW (DWORD	Unknown0,
		  DWORD	Unknown1,
		  DWORD	Unknown2,
		  DWORD	Unknown3)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}


DWORD STDCALL
GetConsoleAliasA (DWORD	Unknown0,
		  DWORD	Unknown1,
		  DWORD	Unknown2,
		  DWORD	Unknown3)
     /*
      * Undocumented
      */
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}

DWORD STDCALL
GetConsoleAliasExesW (DWORD	Unknown0,
		      DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}



DWORD STDCALL
GetConsoleAliasExesA (DWORD	Unknown0,
		      DWORD	Unknown1)
     /*
      * Undocumented
      */
{
	SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
	return 0;
}



DWORD STDCALL
GetConsoleAliasExesLengthA (VOID)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleAliasExesLengthW (VOID)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleAliasesW (DWORD	Unknown0,
		    DWORD	Unknown1,
		    DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}
 
DWORD STDCALL
GetConsoleAliasesA (DWORD	Unknown0,
		    DWORD	Unknown1,
		    DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleAliasesLengthW (DWORD Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleAliasesLengthA (DWORD Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleCommandHistoryW (DWORD	Unknown0,
			   DWORD	Unknown1,
			   DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleCommandHistoryA (DWORD	Unknown0,
			   DWORD	Unknown1,
			   DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleCommandHistoryLengthW (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleCommandHistoryLengthA (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

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
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleFontInfo (DWORD	Unknown0,
		    DWORD	Unknown1,
		    DWORD	Unknown2,
		    DWORD	Unknown3)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleFontSize (DWORD	Unknown0,
		    DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleHardwareState (DWORD	Unknown0,
			 DWORD	Unknown1,
			 DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
GetConsoleInputWaitHandle (VOID)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

DWORD STDCALL
GetCurrentConsoleFont (DWORD	Unknown0,
		       DWORD	Unknown1,
		       DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

ULONG STDCALL
GetNumberOfConsoleFonts (VOID)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 1; /* FIXME: call csrss.exe */
}

DWORD STDCALL
InvalidateConsoleDIBits (DWORD	Unknown0,
			 DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
OpenConsoleW (DWORD	Unknown0,
	      DWORD	Unknown1,
	      DWORD	Unknown2,
	      DWORD	Unknown3)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

WINBOOL STDCALL
SetConsoleCommandHistoryMode (DWORD	dwMode)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

WINBOOL STDCALL
SetConsoleCursor (DWORD	Unknown0,
		  DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

WINBOOL STDCALL
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
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

WINBOOL STDCALL
SetConsoleFont (DWORD	Unknown0,
		DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

WINBOOL STDCALL
SetConsoleHardwareState (DWORD	Unknown0,
			 DWORD	Unknown1,
			 DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

WINBOOL STDCALL
SetConsoleKeyShortcuts (DWORD	Unknown0,
			DWORD	Unknown1,
			DWORD	Unknown2,
			DWORD	Unknown3)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

WINBOOL STDCALL
SetConsoleMaximumWindowSize (DWORD	Unknown0,
			     DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

WINBOOL STDCALL
SetConsoleMenuClose (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

WINBOOL STDCALL
SetConsoleNumberOfCommandsA (DWORD	Unknown0,
			     DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

WINBOOL STDCALL
SetConsoleNumberOfCommandsW (DWORD	Unknown0,
			     DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

WINBOOL STDCALL
SetConsolePalette (DWORD	Unknown0,
		   DWORD	Unknown1,
		   DWORD	Unknown2)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

WINBOOL STDCALL
SetLastConsoleEventActive (VOID)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return FALSE;
}

DWORD STDCALL
ShowConsoleCursor (DWORD	Unknown0,
		   DWORD	Unknown1)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
VerifyConsoleIoHandle (DWORD	Unknown0)
     /*
      * Undocumented
      */
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
WriteConsoleInputVDMA (DWORD	Unknown0,
		       DWORD	Unknown1,
		       DWORD	Unknown2,
		       DWORD	Unknown3)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

DWORD STDCALL
WriteConsoleInputVDMW (DWORD	Unknown0,
		       DWORD	Unknown1,
		       DWORD	Unknown2,
		       DWORD	Unknown3)
{
  SetLastError(ERROR_CALL_NOT_IMPLEMENTED);
  return 0;
}

WINBOOL STDCALL 
CloseConsoleHandle(HANDLE Handle)
     /*
      * Undocumented
      */
{
  if (IsConsoleHandle (Handle) == FALSE)
    {
      SetLastError (ERROR_INVALID_PARAMETER);
      return FALSE;
    }
  /* FIXME: call CSRSS */
  return FALSE;
}

BOOLEAN STDCALL 
IsConsoleHandle(HANDLE Handle)
{
  if ((((ULONG)Handle) & 0x10000003) == 0x3)
    {
      return(TRUE);
    }
  return(FALSE);
}

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
    case STD_INPUT_HANDLE:	return Ppb->InputHandle;
    case STD_OUTPUT_HANDLE:	return Ppb->OutputHandle;
    case STD_ERROR_HANDLE:	return Ppb->ErrorHandle;
    }
  SetLastError (ERROR_INVALID_PARAMETER);
  return INVALID_HANDLE_VALUE;
}

WINBASEAPI BOOL WINAPI 
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
   
  Ppb = NtCurrentPeb()->ProcessParameters;
  
  /* More checking needed? */
  if (hHandle == INVALID_HANDLE_VALUE)
    {
      SetLastError (ERROR_INVALID_HANDLE);
      return FALSE;
    }
   
  SetLastError(ERROR_SUCCESS); /* OK */
  switch (nStdHandle)
    {
    case STD_INPUT_HANDLE:
      Ppb->InputHandle = hHandle;
      return TRUE;
    case STD_OUTPUT_HANDLE:
      Ppb->OutputHandle = hHandle;
      return TRUE;
    case STD_ERROR_HANDLE:
      Ppb->ErrorHandle = hHandle;
      return TRUE;
    }
  SetLastError (ERROR_INVALID_PARAMETER);
  return FALSE;
}


/*--------------------------------------------------------------
 *	WriteConsoleA
 */
WINBOOL STDCALL 
WriteConsoleA(HANDLE hConsoleOutput,
	      CONST VOID *lpBuffer,
	      DWORD nNumberOfCharsToWrite,
	      LPDWORD lpNumberOfCharsWritten,
	      LPVOID lpReserved)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  USHORT Size;
  ULONG MessageSize;
  
  Request = RtlAllocateHeap(GetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    sizeof(CSRSS_API_REQUEST) + 
			    CSRSS_MAX_WRITE_CONSOLE_REQUEST);
  if (Request == NULL)
    {
      return(FALSE);
    }
  
  Request->Type = CSRSS_WRITE_CONSOLE;
  Request->Data.WriteConsoleRequest.ConsoleHandle = hConsoleOutput;
  if (lpNumberOfCharsWritten != NULL)
    *lpNumberOfCharsWritten = nNumberOfCharsToWrite;
  while (nNumberOfCharsToWrite)
    {
      if (nNumberOfCharsToWrite > CSRSS_MAX_WRITE_CONSOLE_REQUEST)
	{
	  Size = CSRSS_MAX_WRITE_CONSOLE_REQUEST;
	}
      else
	{
	  Size = nNumberOfCharsToWrite;
	}
      Request->Data.WriteConsoleRequest.NrCharactersToWrite = Size;
      
      memcpy(Request->Data.WriteConsoleRequest.Buffer, lpBuffer, Size);

      MessageSize = CSRSS_REQUEST_HEADER_SIZE + 
	sizeof(CSRSS_WRITE_CONSOLE_REQUEST) + Size;
      Status = CsrClientCallServer(Request,
				   &Reply,
				   MessageSize,
				   sizeof(CSRSS_API_REPLY));
      
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
	{
	  RtlFreeHeap(GetProcessHeap(), 0, Request);
	  SetLastErrorByStatus(Status);
	  return(FALSE);
	}
      nNumberOfCharsToWrite -= Size;
      lpBuffer += Size;
    }
  RtlFreeHeap(GetProcessHeap(), 0, Request);
  return TRUE;
}


/*--------------------------------------------------------------
 *	ReadConsoleA
 */
WINBOOL STDCALL ReadConsoleA(HANDLE hConsoleInput,
			     LPVOID lpBuffer,
			     DWORD nNumberOfCharsToRead,
			     LPDWORD lpNumberOfCharsRead,
			     LPVOID lpReserved)
{
   CSRSS_API_REQUEST Request;
   PCSRSS_API_REPLY Reply;
   NTSTATUS Status;
   ULONG CharsRead = 0;
   
   Reply = RtlAllocateHeap(GetProcessHeap(),
		     HEAP_ZERO_MEMORY,
		     sizeof(CSRSS_API_REPLY) + nNumberOfCharsToRead);
   if (Reply == NULL)
     {
	return(FALSE);
     }
   
   Request.Type = CSRSS_READ_CONSOLE;
   Request.Data.ReadConsoleRequest.ConsoleHandle = hConsoleInput;
   Request.Data.ReadConsoleRequest.NrCharactersToRead = nNumberOfCharsToRead > CSRSS_MAX_READ_CONSOLE_REQUEST ? CSRSS_MAX_READ_CONSOLE_REQUEST : nNumberOfCharsToRead;
   Request.Data.ReadConsoleRequest.nCharsCanBeDeleted = 0;
   Status = CsrClientCallServer(&Request, 
				Reply,
				sizeof(CSRSS_API_REQUEST),
				sizeof(CSRSS_API_REPLY) + 
				Request.Data.ReadConsoleRequest.NrCharactersToRead);
   if (!NT_SUCCESS(Status) || !NT_SUCCESS( Status = Reply->Status ))
     {
	DbgPrint( "CSR returned error in ReadConsole\n" );
	SetLastErrorByStatus ( Status );
	RtlFreeHeap( GetProcessHeap(), 0, Reply );
	return(FALSE);
     }
   if( Reply->Status == STATUS_NOTIFY_CLEANUP )
      Reply->Status = STATUS_PENDING;     // ignore backspace because we have no chars to backspace
   /* There may not be any chars or lines to read yet, so wait */
   while( Reply->Status == STATUS_PENDING )
     {
       /* some chars may have been returned, but not a whole line yet, so recompute buffer and try again */
       nNumberOfCharsToRead -= Reply->Data.ReadConsoleReply.NrCharactersRead;
       /* don't overflow caller's buffer, even if you still don't have a complete line */
       if( !nNumberOfCharsToRead )
	 break;
       Request.Data.ReadConsoleRequest.NrCharactersToRead = nNumberOfCharsToRead > CSRSS_MAX_READ_CONSOLE_REQUEST ? CSRSS_MAX_READ_CONSOLE_REQUEST : nNumberOfCharsToRead;
       /* copy any chars already read to buffer */
       memcpy( lpBuffer + CharsRead, Reply->Data.ReadConsoleReply.Buffer, Reply->Data.ReadConsoleReply.NrCharactersRead );
       CharsRead += Reply->Data.ReadConsoleReply.NrCharactersRead;
       /* wait for csrss to signal there is more data to read, but not if we got STATUS_NOTIFY_CLEANUP for backspace */
       Status = NtWaitForSingleObject( Reply->Data.ReadConsoleReply.EventHandle, FALSE, 0 );
       if( !NT_SUCCESS( Status ) )
	  {
	     DbgPrint( "Wait for console input failed!\n" );
	     RtlFreeHeap( GetProcessHeap(), 0, Reply );
	     return FALSE;
	  }
       Request.Data.ReadConsoleRequest.nCharsCanBeDeleted = CharsRead;
       Status = CsrClientCallServer( &Request, Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) + Request.Data.ReadConsoleRequest.NrCharactersToRead );
       if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply->Status ) )
	 {
	   SetLastErrorByStatus ( Status );
	   RtlFreeHeap( GetProcessHeap(), 0, Reply );
	   return FALSE;
	 }
       if( Reply->Status == STATUS_NOTIFY_CLEANUP )
	  {
	     // delete last char
	     if( CharsRead )
		{
		   CharsRead--;
		   nNumberOfCharsToRead++;
		}
	     Reply->Status = STATUS_PENDING;  // retry
	  }
     }
   /* copy data to buffer, count total returned, and return */
   memcpy( lpBuffer + CharsRead, Reply->Data.ReadConsoleReply.Buffer, Reply->Data.ReadConsoleReply.NrCharactersRead );
   CharsRead += Reply->Data.ReadConsoleReply.NrCharactersRead;
   if (lpNumberOfCharsRead != NULL)
     *lpNumberOfCharsRead = CharsRead;
   RtlFreeHeap(GetProcessHeap(),
	    0,
	    Reply);
   
   return(TRUE);
}


/*--------------------------------------------------------------
 *	AllocConsole
 */
WINBOOL STDCALL AllocConsole(VOID)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_ALLOC_CONSOLE;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   SetStdHandle( STD_INPUT_HANDLE, Reply.Data.AllocConsoleReply.InputHandle );
   SetStdHandle( STD_OUTPUT_HANDLE, Reply.Data.AllocConsoleReply.OutputHandle );
   SetStdHandle( STD_ERROR_HANDLE, Reply.Data.AllocConsoleReply.OutputHandle );
   return TRUE;
}


/*--------------------------------------------------------------
 *	FreeConsole
 */
WINBOOL STDCALL FreeConsole(VOID)
{
   DbgPrint("FreeConsole() is unimplemented");
   return FALSE;
}


/*--------------------------------------------------------------
 *	GetConsoleScreenBufferInfo
 */
WINBOOL
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
 */
WINBOOL
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


/*--------------------------------------------------------------
 *	FillConsoleOutputCharacterA
 */
WINBOOL
STDCALL
FillConsoleOutputCharacterA(
	HANDLE		hConsoleOutput,
	CHAR		cCharacter,
	DWORD		nLength,
	COORD		dwWriteCoord,
	LPDWORD		lpNumberOfCharsWritten
	)
{
   CSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;

   Request.Type = CSRSS_FILL_OUTPUT;
   Request.Data.FillOutputRequest.ConsoleHandle = hConsoleOutput;
   Request.Data.FillOutputRequest.Char = cCharacter;
   Request.Data.FillOutputRequest.Position = dwWriteCoord;
   Request.Data.FillOutputRequest.Length = nLength;
   Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );
   if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
      {
	 SetLastErrorByStatus ( Status );
	 return FALSE;
      }
   *lpNumberOfCharsWritten = nLength;
   return TRUE;
}


/*--------------------------------------------------------------
 *	FillConsoleOutputCharacterW
 */
WINBOOL
STDCALL
FillConsoleOutputCharacterW(
	HANDLE		hConsoleOutput,
	WCHAR		cCharacter,
	DWORD		nLength,
	COORD		dwWriteCoord,
	LPDWORD		lpNumberOfCharsWritten
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	PeekConsoleInputA
 */
WINBASEAPI
BOOL
WINAPI
PeekConsoleInputA(
	HANDLE			hConsoleInput,
	PINPUT_RECORD		lpBuffer,
	DWORD			nLength,
	LPDWORD			lpNumberOfEventsRead
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	PeekConsoleInputW
 */
WINBASEAPI
BOOL
WINAPI
PeekConsoleInputW(
	HANDLE			hConsoleInput,
	PINPUT_RECORD		lpBuffer,
	DWORD			nLength,
	LPDWORD			lpNumberOfEventsRead
	)    
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleInputA
 */
WINBASEAPI BOOL WINAPI
ReadConsoleInputA(HANDLE hConsoleInput,
		  PINPUT_RECORD	lpBuffer,
		  DWORD	nLength,
		  LPDWORD lpNumberOfEventsRead)
{
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  DWORD NumEventsRead;
  NTSTATUS Status;

  Request.Type = CSRSS_READ_INPUT;
  Request.Data.ReadInputRequest.ConsoleHandle = hConsoleInput;
  Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST),
				sizeof(CSRSS_API_REPLY));
  if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
    {
      SetLastErrorByStatus(Status);
      return(FALSE);
    }
  
  while (Status == STATUS_PENDING)
    {
      Status = NtWaitForSingleObject(Reply.Data.ReadInputReply.Event, FALSE, 
				     0);
      if(!NT_SUCCESS(Status))
	{
	  SetLastErrorByStatus(Status);
	  return FALSE;
	}
      
      Request.Type = CSRSS_READ_INPUT;
      Request.Data.ReadInputRequest.ConsoleHandle = hConsoleInput;
      Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST),
				   sizeof(CSRSS_API_REPLY));
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
	{
	  SetLastErrorByStatus(Status);
	  return(FALSE);
	}
    }
  
  NumEventsRead = 0;
  *lpBuffer = Reply.Data.ReadInputReply.Input;
  lpBuffer++;
  NumEventsRead++;
  
  while ((NumEventsRead < nLength) && (Reply.Data.ReadInputReply.MoreEvents))
    {
      Status = CsrClientCallServer(&Request, &Reply, sizeof(CSRSS_API_REQUEST),
				   sizeof(CSRSS_API_REPLY));
      if (!NT_SUCCESS(Status) || !NT_SUCCESS(Status = Reply.Status))
	{
	  SetLastErrorByStatus(Status);
	  return(FALSE);
	}
      
      if (Status == STATUS_PENDING)
	{
	  break;
	}
      
      *lpBuffer = Reply.Data.ReadInputReply.Input;
      lpBuffer++;
      NumEventsRead++;
      
    }
  *lpNumberOfEventsRead = NumEventsRead;
  
  return TRUE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleInputW
 */
WINBASEAPI
BOOL
WINAPI
ReadConsoleInputW(
	HANDLE			hConsoleInput,
	PINPUT_RECORD		lpBuffer,
	DWORD			nLength,
	LPDWORD			lpNumberOfEventsRead
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	WriteConsoleInputA
 */
WINBASEAPI
BOOL
WINAPI
WriteConsoleInputA(
	HANDLE			 hConsoleInput,
	CONST INPUT_RECORD	*lpBuffer,
	DWORD			 nLength,
	LPDWORD			 lpNumberOfEventsWritten
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	WriteConsoleInputW
 */
WINBASEAPI
BOOL
WINAPI
WriteConsoleInputW(
	HANDLE			 hConsoleInput,
	CONST INPUT_RECORD	*lpBuffer,
	DWORD			 nLength,
	LPDWORD			 lpNumberOfEventsWritten
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleOutputA
 */
WINBASEAPI
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
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleOutputW
 */
WINBASEAPI
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
/* TO DO */
	return FALSE;
}

/*--------------------------------------------------------------
 * 	WriteConsoleOutputA
 */
WINBASEAPI BOOL WINAPI
WriteConsoleOutputA(HANDLE		 hConsoleOutput,
		    CONST CHAR_INFO	*lpBuffer,
		    COORD		 dwBufferSize,
		    COORD		 dwBufferCoord,
		    PSMALL_RECT	 lpWriteRegion)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  ULONG Size;
  BOOLEAN Result;
  ULONG i, j;
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
      SetLastError(ERROR_OUTOFMEMORY);
      return FALSE;
    }
  Request->Type = CSRSS_WRITE_CONSOLE_OUTPUT;
  Request->Data.WriteConsoleOutputRequest.ConsoleHandle = hConsoleOutput;
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
      RtlFreeHeap(GetProcessHeap(), 0, Request);
      SetLastErrorByStatus(Status);
      return FALSE;
    }
      
  *lpWriteRegion = Reply.Data.WriteConsoleOutputReply.WriteRegion;
  RtlFreeHeap(GetProcessHeap(), 0, Request);
  CsrReleaseParameterBuffer(BufferBase);
  return(TRUE);
}


/*--------------------------------------------------------------
 * 	WriteConsoleOutputW
 */
WINBASEAPI
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
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleOutputCharacterA
 */
WINBASEAPI
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
   SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
   return FALSE;
}


/*--------------------------------------------------------------
 *      ReadConsoleOutputCharacterW
 */
WINBASEAPI
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
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	ReadConsoleOutputAttribute
 */
WINBASEAPI
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
   SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
   return FALSE;
}


/*--------------------------------------------------------------
 * 	WriteConsoleOutputCharacterA
 */
WINBASEAPI BOOL WINAPI
WriteConsoleOutputCharacterA(HANDLE		hConsoleOutput,
			     LPCSTR		lpCharacter,
			     DWORD		nLength,
			     COORD		dwWriteCoord,
			     LPDWORD		lpNumberOfCharsWritten)
{
  PCSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;
  WORD Size;
  
  Request = RtlAllocateHeap(GetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    sizeof(CSRSS_API_REQUEST) + CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR);
  if( !Request )
    {
      SetLastError( ERROR_OUTOFMEMORY );
      return FALSE;
    }
  Request->Type = CSRSS_WRITE_CONSOLE_OUTPUT_CHAR;
  Request->Data.WriteConsoleOutputCharRequest.ConsoleHandle = hConsoleOutput;
  Request->Data.WriteConsoleOutputCharRequest.Coord = dwWriteCoord;
  if( lpNumberOfCharsWritten )
    *lpNumberOfCharsWritten = nLength;
  while( nLength )
    {
      Size = nLength > CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR ? CSRSS_MAX_WRITE_CONSOLE_OUTPUT_CHAR : nLength;
      Request->Data.WriteConsoleOutputCharRequest.Length = Size;
      memcpy( &Request->Data.WriteConsoleOutputCharRequest.String[0],
	      lpCharacter,
	      Size );
      Status = CsrClientCallServer( Request, &Reply, sizeof( CSRSS_API_REQUEST ) + Size, sizeof( CSRSS_API_REPLY ) );
      if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
	{
	  RtlFreeHeap( GetProcessHeap(), 0, Request );
	  SetLastErrorByStatus ( Status );
	  return FALSE;
	}
      nLength -= Size;
      lpCharacter += Size;
      Request->Data.WriteConsoleOutputCharRequest.Coord = Reply.Data.WriteConsoleOutputCharReply.EndCoord;
    }
  return TRUE;
}


/*--------------------------------------------------------------
 * 	WriteConsoleOutputCharacterW
 */
WINBASEAPI
BOOL
WINAPI
WriteConsoleOutputCharacterW(
	HANDLE		hConsoleOutput,
	LPCWSTR		lpCharacter,
	DWORD		nLength,
	COORD		dwWriteCoord,
	LPDWORD		lpNumberOfCharsWritten
	)
{
/* TO DO */
	return FALSE;
}



/*--------------------------------------------------------------
 * 	WriteConsoleOutputAttribute
 */
WINBASEAPI
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

   Request = RtlAllocateHeap(GetProcessHeap(),
		       HEAP_ZERO_MEMORY,
		       sizeof(CSRSS_API_REQUEST) + CSRSS_MAX_WRITE_CONSOLE_OUTPUT_ATTRIB);
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
	 for( c = 0; c < ( Size * 2 ); c++ )
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
   return TRUE;
}


/*--------------------------------------------------------------
 * 	FillConsoleOutputAttribute
 */
WINBASEAPI
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
 */
WINBASEAPI
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
  
  Request.Type = CSRSS_GET_MODE;
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
 */
WINBASEAPI
BOOL
WINAPI
GetNumberOfConsoleInputEvents(
	HANDLE		hConsoleInput,
	LPDWORD		lpNumberOfEvents
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	GetLargestConsoleWindowSize
 */
WINBASEAPI
COORD
WINAPI
GetLargestConsoleWindowSize(
	HANDLE		hConsoleOutput
	)
{
#if 1	/* FIXME: */
	COORD Coord = {80,25};

/* TO DO */
	return Coord;
#endif
}


/*--------------------------------------------------------------
 *	GetConsoleCursorInfo
 */
WINBASEAPI
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
 */
WINBASEAPI
BOOL
WINAPI
GetNumberOfConsoleMouseButtons(
	LPDWORD		lpNumberOfMouseButtons
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	SetConsoleMode
 */
WINBASEAPI
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
  
  Request.Type = CSRSS_SET_MODE;
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
 */
WINBASEAPI
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
   Request.Data.SetActiveScreenBufferRequest.OutputHandle = hConsoleOutput;
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
 */
WINBASEAPI
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
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleScreenBufferSize(
	HANDLE		hConsoleOutput,
	COORD		dwSize
	)
{
/* TO DO */
	return FALSE;
}

/*--------------------------------------------------------------
 * 	SetConsoleCursorInfo
 */
WINBASEAPI
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


/*--------------------------------------------------------------
 *	ScrollConsoleScreenBufferA
 */
WINBASEAPI
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
  CSRSS_API_REQUEST Request;
  CSRSS_API_REPLY Reply;
  NTSTATUS Status;

  Request.Type = CSRSS_SCROLL_CONSOLE_SCREEN_BUFFER;
  Request.Data.ScrollConsoleScreenBufferRequest.ConsoleHandle = hConsoleOutput;
  Request.Data.ScrollConsoleScreenBufferRequest.ScrollRectangle = *lpScrollRectangle;

  if (lpClipRectangle != NULL)
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
  Status = CsrClientCallServer( &Request, &Reply, sizeof( CSRSS_API_REQUEST ), sizeof( CSRSS_API_REPLY ) );

  if( !NT_SUCCESS( Status ) || !NT_SUCCESS( Status = Reply.Status ) )
    {
  SetLastErrorByStatus ( Status );
	return FALSE;
    }
  return TRUE;
}


/*--------------------------------------------------------------
 * 	ScrollConsoleScreenBufferW
 */
WINBASEAPI
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
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	SetConsoleWindowInfo
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleWindowInfo(
	HANDLE			 hConsoleOutput,
	BOOL			 bAbsolute,
	CONST SMALL_RECT	*lpConsoleWindow
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 *      SetConsoleTextAttribute
 */
WINBASEAPI
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
      CtrlHandlers = 
	RtlReAllocateHeap(RtlGetProcessHeap(),
			   HEAP_ZERO_MEMORY,
			   (PVOID)CtrlHandlers,
			   NrCtrlHandlers * sizeof(PHANDLER_ROUTINE)); 
      if (CtrlHandlers == NULL)
	{
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
	  if (CtrlHandlers[i] == HandlerRoutine)
	    {
	      CtrlHandlers[i] = CtrlHandlers[NrCtrlHandlers - 1];
	      NrCtrlHandlers--;
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

WINBASEAPI BOOL WINAPI
SetConsoleCtrlHandler(PHANDLER_ROUTINE	HandlerRoutine,
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
 */
WINBASEAPI BOOL WINAPI
GenerateConsoleCtrlEvent(
	DWORD		dwCtrlEvent,
	DWORD		dwProcessGroupId
	)
{
  /* TO DO */
  return FALSE;
}


/*--------------------------------------------------------------
 *	GetConsoleTitleW
 */
#define MAX_CONSOLE_TITLE_LENGTH 80

WINBASEAPI
DWORD
WINAPI
GetConsoleTitleW(
	LPWSTR		lpConsoleTitle,
	DWORD		nSize
	)
{
	union {
	CSRSS_API_REQUEST	quest;
	CSRSS_API_REPLY		ply;
	} Re;
	NTSTATUS		Status;

	/* Marshall data */
	Re.quest.Type = CSRSS_GET_TITLE;
	Re.quest.Data.GetTitleRequest.ConsoleHandle =
		GetStdHandle (STD_INPUT_HANDLE);

	/* Call CSRSS */
	Status = CsrClientCallServer (
			& Re.quest,
			& Re.ply,
			(sizeof (CSRSS_GET_TITLE_REQUEST) +
			sizeof (LPC_MESSAGE_HEADER) +
			sizeof (ULONG)),
			sizeof (CSRSS_API_REPLY)
			);
	if (	!NT_SUCCESS(Status)
		|| !NT_SUCCESS (Status = Re.ply.Status)
		)
	{
		SetLastErrorByStatus (Status);
		return (0);
	}

	/* Convert size in characters to size in bytes */
	nSize = sizeof (WCHAR) * nSize;

	/* Unmarshall data */
	if (nSize < Re.ply.Data.GetTitleReply.Length)
	{
		DbgPrint ("%s: ret=%d\n", __FUNCTION__, Re.ply.Data.GetTitleReply.Length);
		nSize /= sizeof (WCHAR);
		if (nSize > 1)
		{
			wcsncpy (
				lpConsoleTitle,
				Re.ply.Data.GetTitleReply.Title,
				(nSize - 1)
				);
			/* Add null */
			lpConsoleTitle [nSize --] = L'\0';
		}
	}
	else
	{
		nSize = Re.ply.Data.GetTitleReply.Length / sizeof (WCHAR);
		wcscpy (lpConsoleTitle, Re.ply.Data.GetTitleReply.Title);
	}

	return nSize;
}


/*--------------------------------------------------------------
 * 	GetConsoleTitleA
 *
 * 	19990306 EA
 */
WINBASEAPI
DWORD
WINAPI
GetConsoleTitleA(
	LPSTR		lpConsoleTitle,
	DWORD		nSize
	)
{
	wchar_t	WideTitle [MAX_CONSOLE_TITLE_LENGTH];
	DWORD	nWideTitle = sizeof WideTitle;
//	DWORD	nWritten;
	
	if (!lpConsoleTitle || !nSize) return 0;
	nWideTitle = GetConsoleTitleW( (LPWSTR) WideTitle, nWideTitle );
	if (!nWideTitle) return 0;
#if 0
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
#endif
	return 0;
}


/*--------------------------------------------------------------
 *	SetConsoleTitleW
 */
WINBASEAPI
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
  
  Request = RtlAllocateHeap(GetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    sizeof(CSRSS_API_REQUEST) + CSRSS_MAX_SET_TITLE_REQUEST);
  if (Request == NULL)
    {
      return(FALSE);
    }
  
  Request->Type = CSRSS_SET_TITLE;
  Request->Data.SetTitleRequest.Console = GetStdHandle( STD_INPUT_HANDLE );
  
  for( c = 0; lpConsoleTitle[c] && c < CSRSS_MAX_TITLE_LENGTH; c++ )
    Request->Data.SetTitleRequest.Title[c] = lpConsoleTitle[c];
  // add null
  Request->Data.SetTitleRequest.Title[c] = 0;
  Request->Data.SetTitleRequest.Length = c;  
  Status = CsrClientCallServer(Request,
			       &Reply,
			       sizeof(CSRSS_SET_TITLE_REQUEST) +
			       c +
			       sizeof( LPC_MESSAGE_HEADER ) +
			       sizeof( ULONG ),
			       sizeof(CSRSS_API_REPLY));
  
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
 */
WINBASEAPI
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
  
  Request = RtlAllocateHeap(GetProcessHeap(),
			    HEAP_ZERO_MEMORY,
			    sizeof(CSRSS_API_REQUEST) + CSRSS_MAX_SET_TITLE_REQUEST);
  if (Request == NULL)
    {
      return(FALSE);
    }
  
  Request->Type = CSRSS_SET_TITLE;
  Request->Data.SetTitleRequest.Console = GetStdHandle( STD_INPUT_HANDLE );
  
  for( c = 0; lpConsoleTitle[c] && c < CSRSS_MAX_TITLE_LENGTH; c++ )
    Request->Data.SetTitleRequest.Title[c] = lpConsoleTitle[c];
  // add null
  Request->Data.SetTitleRequest.Title[c] = 0;
  Request->Data.SetTitleRequest.Length = c;
  Status = CsrClientCallServer(Request,
			       &Reply,
			       sizeof(CSRSS_SET_TITLE_REQUEST) +
			       c +
			       sizeof( LPC_MESSAGE_HEADER ) +
			       sizeof( ULONG ),
			       sizeof(CSRSS_API_REPLY));
  
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
 *	ReadConsoleW
 */
WINBASEAPI
BOOL
WINAPI
ReadConsoleW(
	HANDLE		hConsoleInput,
	LPVOID		lpBuffer,
	DWORD		nNumberOfCharsToRead,
	LPDWORD 	lpNumberOfCharsRead,
	LPVOID		lpReserved
	)
{
/* --- TO DO --- */
	return FALSE;
}


/*--------------------------------------------------------------
 *	WriteConsoleW
 */
WINBASEAPI
BOOL
WINAPI
WriteConsoleW(
	HANDLE		 hConsoleOutput,
	CONST VOID	*lpBuffer,
	DWORD		 nNumberOfCharsToWrite,
	LPDWORD		 lpNumberOfCharsWritten,
	LPVOID		 lpReserved
	)
{
#if 0
   PCSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;
   
   Request = RtlAllocateHeap(GetProcessHeap(),
		       HEAP_ZERO_MEMORY,
		       sizeof(CSRSS_API_REQUEST) + nNumberOfCharsToWrite * sizeof(WCHAR));
   if (Request == NULL)
     {
	return(FALSE);
     }

   Request->Type = CSRSS_WRITE_CONSOLE;
   Request->Data.WriteConsoleRequest.ConsoleHandle = hConsoleOutput;
   Request->Data.WriteConsoleRequest.NrCharactersToWrite =
     nNumberOfCharsToWrite;
//   DbgPrint("nNumberOfCharsToWrite %d\n", nNumberOfCharsToWrite);
//   DbgPrint("Buffer %s\n", Request->Data.WriteConsoleRequest.Buffer);
   memcpy(Request->Data.WriteConsoleRequest.Buffer,
	  lpBuffer,
	  nNumberOfCharsToWrite * sizeof(WCHAR));

   Status = CsrClientCallServer(Request,
				&Reply,
				sizeof(CSRSS_API_REQUEST) +
				nNumberOfCharsToWrite,
				sizeof(CSRSS_API_REPLY));

   RtlFreeHeap(GetProcessHeap(),
	    0,
	    Request);

   if (!NT_SUCCESS(Status))
     {
	return(FALSE);
     }

   if (lpNumberOfCharsWritten != NULL)
     {
	*lpNumberOfCharsWritten = 
	  Reply.Data.WriteConsoleReply.NrCharactersWritten;
     }

   return(TRUE);
#endif
   return FALSE;
}


/*--------------------------------------------------------------
 *	CreateConsoleScreenBuffer
 */
WINBASEAPI
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
 */
WINBASEAPI
UINT
WINAPI
GetConsoleCP( VOID )
{
/* --- TO DO --- */
	return CP_OEMCP; /* FIXME */
}


/*--------------------------------------------------------------
 *	SetConsoleCP
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleCP(
	UINT		wCodePageID
	)
{
/* --- TO DO --- */
	return FALSE;
}


/*--------------------------------------------------------------
 *	GetConsoleOutputCP
 */
WINBASEAPI
UINT
WINAPI
GetConsoleOutputCP( VOID )
{
/* --- TO DO --- */
	return 0; /* FIXME */
}


/*--------------------------------------------------------------
 *	SetConsoleOutputCP
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleOutputCP(
	UINT		wCodePageID
	)
{
/* --- TO DO --- */
	return FALSE;
}


/* EOF */
