/*
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

/* FUNCTIONS *****************************************************************/

WINBOOL STDCALL CloseConsoleHandle(HANDLE Handle)
{   
}

BOOLEAN IsConsoleHandle(HANDLE Handle)
{
   if ((((ULONG)Handle) & 0x10000003) == 0x3)
     {
	return(TRUE);
     }
   return(FALSE);
}


/*--------------------------------------------------------------
 *	GetStdHandle
 */
HANDLE STDCALL GetStdHandle(DWORD nStdHandle)
{
   PRTL_USER_PROCESS_PARAMETERS Ppb;
   
   DPRINT("GetStdHandle(nStdHandle %d)\n",nStdHandle);
   
   SetLastError(ERROR_SUCCESS); /* OK */
   Ppb = NtCurrentPeb()->ProcessParameters;  
   switch (nStdHandle)
     {
      case STD_INPUT_HANDLE:	return Ppb->InputHandle;
      case STD_OUTPUT_HANDLE:	return Ppb->OutputHandle;
      case STD_ERROR_HANDLE:	return Ppb->ErrorHandle;
     }
   SetLastError(0); /* FIXME: What error code? */
   return INVALID_HANDLE_VALUE;
}


/*--------------------------------------------------------------
 *	SetStdHandle
 */
WINBASEAPI BOOL WINAPI SetStdHandle(DWORD nStdHandle,
				    HANDLE hHandle)
{
   PRTL_USER_PROCESS_PARAMETERS Ppb;
   
   Ppb = NtCurrentPeb()->ProcessParameters;
   
   /* More checking needed? */
   if (hHandle == INVALID_HANDLE_VALUE)
     {
	SetLastError(0);	/* FIXME: What error code? */
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
   SetLastError(0); /* FIXME: What error code? */
   return FALSE;
}


/*--------------------------------------------------------------
 *	WriteConsoleA
 */
WINBOOL STDCALL WriteConsoleA(HANDLE hConsoleOutput,
			      CONST VOID *lpBuffer,
			      DWORD nNumberOfCharsToWrite,
			      LPDWORD lpNumberOfCharsWritten,
			      LPVOID lpReserved)
{
   PCSRSS_API_REQUEST Request;
   CSRSS_API_REPLY Reply;
   NTSTATUS Status;
   
   Request = HeapAlloc(GetProcessHeap(),
		       HEAP_ZERO_MEMORY,
		       sizeof(CSRSS_API_REQUEST) + nNumberOfCharsToWrite);
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
	  nNumberOfCharsToWrite);
   
   Status = CsrClientCallServer(Request, 
				&Reply,
				sizeof(CSRSS_API_REQUEST) + 
				nNumberOfCharsToWrite,
				sizeof(CSRSS_API_REPLY));
   if (!NT_SUCCESS(Status))
     {
	return(FALSE);
     }
   
   HeapFree(GetProcessHeap(),
	    0,
	    Request);
	    
   if (lpNumberOfCharsWritten != NULL)
     {
	*lpNumberOfCharsWritten = 
	  Reply.Data.WriteConsoleReply.NrCharactersWritten;
     }
   
   return(TRUE);
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
   
   Reply = HeapAlloc(GetProcessHeap(),
		     HEAP_ZERO_MEMORY,
		     sizeof(CSRSS_API_REPLY) + nNumberOfCharsToRead);
   if (Reply == NULL)
     {
	return(FALSE);
     }
   
   Request.Type = CSRSS_READ_CONSOLE;
   Request.Data.ReadConsoleRequest.ConsoleHandle = hConsoleInput;
   Request.Data.ReadConsoleRequest.NrCharactersToRead =
     nNumberOfCharsToRead;
   
   Status = CsrClientCallServer(&Request, 
				Reply,
				sizeof(CSRSS_API_REQUEST),
				sizeof(CSRSS_API_REPLY) + 
				nNumberOfCharsToRead);
   if (!NT_SUCCESS(Status) || !NT_SUCCESS(Reply->Status))
     {
	return(FALSE);
     }
   
   if (lpNumberOfCharsRead != NULL)
     {
	*lpNumberOfCharsRead =
	  Reply->Data.ReadConsoleReply.NrCharactersRead;
     }
   memcpy(lpBuffer, 
	  Reply->Data.ReadConsoleReply.Buffer,
	  Reply->Data.ReadConsoleReply.NrCharactersRead);
   
   HeapFree(GetProcessHeap(),
	    0,
	    Reply);
   
   return(TRUE);
}


/*--------------------------------------------------------------
 *	AllocConsole
 */
WINBOOL STDCALL AllocConsole(VOID)
{
   DbgPrint("AllocConsole() is unimplemented\n");
}


/*--------------------------------------------------------------
 *	FreeConsole
 */
WINBOOL STDCALL FreeConsole(VOID)
{
   DbgPrint("FreeConsole() is unimplemented");
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
	DWORD	dwBytesReturned;
	
	if ( !DeviceIoControl(
		hConsoleOutput,
                IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO,
		NULL,
		0,
		lpConsoleScreenBufferInfo,
		sizeof(CONSOLE_SCREEN_BUFFER_INFO),
		& dwBytesReturned,
		NULL
		)
	)
	{
		SetLastError(0); /* FIXME: What error code? */
		return FALSE;
	}
	SetLastError(ERROR_SUCCESS); /* OK */
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
	DWORD dwBytesReturned;
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;

	if( !GetConsoleScreenBufferInfo(hConsoleOutput,&ConsoleScreenBufferInfo) )
	{
		SetLastError(0); /* FIXME: What error code? */
		return FALSE;
	}
	ConsoleScreenBufferInfo.dwCursorPosition.X = dwCursorPosition.X;
	ConsoleScreenBufferInfo.dwCursorPosition.Y = dwCursorPosition.Y;
	
	if( !DeviceIoControl(
			hConsoleOutput,
			IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO,
			&ConsoleScreenBufferInfo,
			sizeof(CONSOLE_SCREEN_BUFFER_INFO),
			NULL,
			0,
			&dwBytesReturned,
			NULL ))
		return FALSE;

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
    DWORD dwBytesReturned;
    OUTPUT_CHARACTER Buffer;

    Buffer.cCharacter = cCharacter;
    Buffer.nLength    = nLength;
    Buffer.dwCoord    = dwWriteCoord;

    if (DeviceIoControl (hConsoleOutput,
                         IOCTL_CONSOLE_FILL_OUTPUT_CHARACTER,
                         &Buffer,
                         sizeof(OUTPUT_CHARACTER),
                         &Buffer,
                         sizeof(OUTPUT_CHARACTER),
                         &dwBytesReturned,
                         NULL))
    {
        *lpNumberOfCharsWritten = Buffer.dwTransfered;
        return TRUE;
    }

    *lpNumberOfCharsWritten = 0;
    return FALSE;
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
WINBASEAPI
BOOL
WINAPI
ReadConsoleInputA(
	HANDLE			hConsoleInput,
	PINPUT_RECORD		lpBuffer,
	DWORD			nLength,
	LPDWORD			lpNumberOfEventsRead
	)
{
   BOOL  stat = TRUE;
   DWORD Result;
   int i;
   
   for (i=0; (stat && i < nLength);)     
     {
	stat = ReadFile(hConsoleInput,
                        &lpBuffer[i].Event.KeyEvent,
                        sizeof(KEY_EVENT_RECORD),
			&Result,
			NULL);
        if (stat)
	  {
             lpBuffer[i].EventType = KEY_EVENT;
	     i++;
	  }
     }
   if (lpNumberOfEventsRead != NULL)
     {
        *lpNumberOfEventsRead = i;
     }
   return(stat);
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
WINBASEAPI
BOOL
WINAPI
WriteConsoleOutputA(
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
    DWORD dwBytesReturned;
    OUTPUT_CHARACTER Buffer;

    Buffer.dwCoord    = dwReadCoord;

    if (DeviceIoControl (hConsoleOutput,
                         IOCTL_CONSOLE_READ_OUTPUT_CHARACTER,
                         &Buffer,
                         sizeof(OUTPUT_CHARACTER),
                         lpCharacter,
                         nLength,
                         &dwBytesReturned,
                         NULL))
    {
        *lpNumberOfCharsRead = Buffer.dwTransfered;
        return TRUE;
    }

    *lpNumberOfCharsRead = 0;
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
    DWORD dwBytesReturned;
    OUTPUT_ATTRIBUTE Buffer;

    Buffer.dwCoord = dwReadCoord;

    if (DeviceIoControl (hConsoleOutput,
                         IOCTL_CONSOLE_READ_OUTPUT_ATTRIBUTE,
                         &Buffer,
                         sizeof(OUTPUT_ATTRIBUTE),
                         (PVOID)lpAttribute,
                         nLength,
                         &dwBytesReturned,
                         NULL))
    {
        *lpNumberOfAttrsRead = Buffer.dwTransfered;
        return TRUE;
    }

    *lpNumberOfAttrsRead = 0;

    return FALSE;
}


/*--------------------------------------------------------------
 * 	WriteConsoleOutputCharacterA
 */
WINBASEAPI
BOOL
WINAPI
WriteConsoleOutputCharacterA(
	HANDLE		hConsoleOutput,
	LPCSTR		lpCharacter,
	DWORD		nLength,
	COORD		dwWriteCoord,
	LPDWORD		lpNumberOfCharsWritten
	)
{
    DWORD dwBytesReturned;
    OUTPUT_CHARACTER Buffer;

    Buffer.dwCoord    = dwWriteCoord;

    if (DeviceIoControl (hConsoleOutput,
                         IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
                         &Buffer,
                         sizeof(OUTPUT_CHARACTER),
                         (LPSTR)lpCharacter,
                         nLength,
                         &dwBytesReturned,
                         NULL))
    {
        *lpNumberOfCharsWritten = Buffer.dwTransfered;
        return TRUE;
    }

    *lpNumberOfCharsWritten = 0;
    return FALSE;
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
    DWORD dwBytesReturned;
    OUTPUT_ATTRIBUTE Buffer;

    Buffer.dwCoord = dwWriteCoord;

    if (DeviceIoControl (hConsoleOutput,
                         IOCTL_CONSOLE_WRITE_OUTPUT_ATTRIBUTE,
                         &Buffer,
                         sizeof(OUTPUT_ATTRIBUTE),
                         (PVOID)lpAttribute,
                         nLength,
                         &dwBytesReturned,
                         NULL))
    {
        *lpNumberOfAttrsWritten = Buffer.dwTransfered;
        return TRUE;
    }

    *lpNumberOfAttrsWritten = 0;

    return FALSE;
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
    DWORD dwBytesReturned;
    OUTPUT_ATTRIBUTE Buffer;

    Buffer.wAttribute = wAttribute;
    Buffer.nLength    = nLength;
    Buffer.dwCoord    = dwWriteCoord;

    if (DeviceIoControl (hConsoleOutput,
                         IOCTL_CONSOLE_FILL_OUTPUT_ATTRIBUTE,
                         &Buffer,
                         sizeof(OUTPUT_ATTRIBUTE),
                         &Buffer,
                         sizeof(OUTPUT_ATTRIBUTE),
                         &dwBytesReturned,
                         NULL))
    {
        *lpNumberOfAttrsWritten = Buffer.dwTransfered;
        return TRUE;
    }

    *lpNumberOfAttrsWritten = 0;

    return FALSE;
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
    CONSOLE_MODE Buffer;
    DWORD   dwBytesReturned;
	
    if (DeviceIoControl (hConsoleHandle,
                         IOCTL_CONSOLE_GET_MODE,
                         NULL,
                         0,
                         &Buffer,
                         sizeof(CONSOLE_MODE),
                         &dwBytesReturned,
                         NULL))
    {
        *lpMode = Buffer.dwMode;
        SetLastError (ERROR_SUCCESS);
        return TRUE;
    }

    SetLastError(0); /* FIXME: What error code? */
    return FALSE;
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
    DWORD   dwBytesReturned;
	
    if (DeviceIoControl (hConsoleOutput,
                         IOCTL_CONSOLE_GET_CURSOR_INFO,
                         NULL,
                         0,
                         lpConsoleCursorInfo,
                         sizeof(CONSOLE_CURSOR_INFO),
                         &dwBytesReturned,
                         NULL))
        return TRUE;

    return FALSE;
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
    CONSOLE_MODE Buffer;
    DWORD   dwBytesReturned;

    Buffer.dwMode = dwMode;
	
    if (DeviceIoControl (hConsoleHandle,
                         IOCTL_CONSOLE_SET_MODE,
                         &Buffer,
                         sizeof(CONSOLE_MODE),
                         NULL,
                         0,
                         &dwBytesReturned,
                         NULL))
    {
        SetLastError (ERROR_SUCCESS);
        return TRUE;
    }

    SetLastError(0); /* FIXME: What error code? */
    return FALSE;
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
/* TO DO */
	return FALSE;
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
/* TO DO */
	return FALSE;
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
    DWORD   dwBytesReturned;
	
    if (DeviceIoControl (hConsoleOutput,
                         IOCTL_CONSOLE_SET_CURSOR_INFO,
                         (PCONSOLE_CURSOR_INFO)lpConsoleCursorInfo,
                         sizeof(CONSOLE_CURSOR_INFO),
                         NULL,
                         0,
                         &dwBytesReturned,
                         NULL))
        return TRUE;

    return FALSE;
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
/* TO DO */
	return FALSE;
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
    DWORD dwBytesReturned;

    if (!DeviceIoControl (hConsoleOutput,
                          IOCTL_CONSOLE_SET_TEXT_ATTRIBUTE,
                          &wAttributes,
                          sizeof(WORD),
                          NULL,
                          0,
                          &dwBytesReturned,
                          NULL))
        return FALSE;
    return TRUE;
}


/*--------------------------------------------------------------
 * 	SetConsoleCtrlHandler
 */
WINBASEAPI
BOOL
WINAPI
SetConsoleCtrlHandler(
	PHANDLER_ROUTINE	HandlerRoutine,
	BOOL			Add
	)
{
/* TO DO */
	return FALSE;
}


/*--------------------------------------------------------------
 * 	GenerateConsoleCtrlEvent
 */
WINBASEAPI
BOOL
WINAPI
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
/* TO DO */
	return 0;
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
/* --- TO DO --- */
	return FALSE;
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
	wchar_t	WideTitle [MAX_CONSOLE_TITLE_LENGTH];
	char	AnsiTitle [MAX_CONSOLE_TITLE_LENGTH];
	INT	nWideTitle;
	
	if (!lpConsoleTitle) return FALSE;
	ZeroMemory( WideTitle, sizeof WideTitle );
	nWideTitle = lstrlenA(lpConsoleTitle);
	if (!lstrcpynA(
		AnsiTitle,
		lpConsoleTitle,
		nWideTitle
		)) 
	{
		return FALSE;
	}
	AnsiTitle[nWideTitle] = '\0';
#if 0
	if ( MultiByteToWideChar(
		CP_ACP,			// ANSI code page 
		MB_PRECOMPOSED,		// character-type options 
		AnsiTitle,		// address of string to map 
		nWideTitle,		// number of characters in string 
		(LPWSTR) WideTitle,	// address of wide-character buffer 
		(-1)			// size of buffer: -1=...\0
		))
	{
		return SetConsoleTitleW( (LPWSTR) WideTitle ); 
	}
#endif
	return FALSE;
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
/* --- TO DO --- */
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
/* --- TO DO --- */
	return FALSE;
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

