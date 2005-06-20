/*
 *  ReactOS kernel
 *  Copyright (C) 2002 ReactOS Team
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            subsys/system/usetup/console.c
 * PURPOSE:         Console support functions
 * PROGRAMMER:      Eric Kohl
 */

/* INCLUDES ******************************************************************/

#include "usetup.h"

#define NDEBUG
#include <debug.h>


/* GLOBALS ******************************************************************/

static HANDLE StdInput  = INVALID_HANDLE_VALUE;
static HANDLE StdOutput = INVALID_HANDLE_VALUE;

static SHORT xScreen = 0;
static SHORT yScreen = 0;


NTSTATUS
ConGetConsoleScreenBufferInfo(PCONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo);


/* FUNCTIONS *****************************************************************/



NTSTATUS
ConAllocConsole(VOID)
{
  OBJECT_ATTRIBUTES ObjectAttributes;
  IO_STATUS_BLOCK IoStatusBlock;
  UNICODE_STRING Name;
  NTSTATUS Status;
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  /* Open the screen */
  RtlInitUnicodeString(&Name,
		       L"\\??\\BlueScreen");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     0,
			     NULL,
			     NULL);
  Status = NtOpenFile (&StdOutput,
		       FILE_ALL_ACCESS,
		       &ObjectAttributes,
		       &IoStatusBlock,
		       0,
		       FILE_SYNCHRONOUS_IO_ALERT);
  if (!NT_SUCCESS(Status))
    return(Status);

  /* Open the keyboard */
  RtlInitUnicodeString(&Name,
		       L"\\??\\Keyboard");
  InitializeObjectAttributes(&ObjectAttributes,
			     &Name,
			     0,
			     NULL,
			     NULL);
  Status = NtOpenFile (&StdInput,
		       FILE_ALL_ACCESS,
		       &ObjectAttributes,
		       &IoStatusBlock,
		       0,
		       FILE_SYNCHRONOUS_IO_ALERT);
  if (!NT_SUCCESS(Status))
    return(Status);

  ConGetConsoleScreenBufferInfo(&csbi);

  xScreen = csbi.dwSize.X;
  yScreen = csbi.dwSize.Y;

  return(Status);
}


VOID
ConFreeConsole(VOID)
{
  DPRINT("FreeConsole() called\n");

  if (StdInput != INVALID_HANDLE_VALUE)
    NtClose(StdInput);

  if (StdOutput != INVALID_HANDLE_VALUE)
    NtClose(StdOutput);

  DPRINT("FreeConsole() done\n");
}




NTSTATUS
ConWriteConsole(PCHAR Buffer,
	        ULONG NumberOfCharsToWrite,
	        PULONG NumberOfCharsWritten)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status = STATUS_SUCCESS;

  Status = NtWriteFile(StdOutput,
		       NULL,
		       NULL,
		       NULL,
		       &IoStatusBlock,
		       Buffer,
		       NumberOfCharsToWrite,
		       NULL,
		       NULL);

  if (NT_SUCCESS(Status) && NumberOfCharsWritten != NULL)
    {
      *NumberOfCharsWritten = IoStatusBlock.Information;
    }

  return(Status);
}


#if 0
/*--------------------------------------------------------------
 *	ReadConsoleA
 */
BOOL
STDCALL
ReadConsoleA(HANDLE hConsoleInput,
			     LPVOID lpBuffer,
			     DWORD nNumberOfCharsToRead,
			     LPDWORD lpNumberOfCharsRead,
			     LPVOID lpReserved)
{
  KEY_EVENT_RECORD KeyEventRecord;
  BOOL  stat = TRUE;
  PCHAR Buffer = (PCHAR)lpBuffer;
  DWORD Result;
  int   i;

  for (i=0; (stat && i<nNumberOfCharsToRead);)
    {
      stat = ReadFile(hConsoleInput,
		      &KeyEventRecord,
		      sizeof(KEY_EVENT_RECORD),
		      &Result,
		      NULL);
      if (stat && KeyEventRecord.bKeyDown && KeyEventRecord.uChar.AsciiChar != 0)
	{
	  Buffer[i] = KeyEventRecord.uChar.AsciiChar;
	  i++;
	}
    }
  if (lpNumberOfCharsRead != NULL)
    {
      *lpNumberOfCharsRead = i;
    }
  return(stat);
}
#endif


NTSTATUS
ConReadConsoleInput(PINPUT_RECORD Buffer)
{
  IO_STATUS_BLOCK Iosb;
  NTSTATUS Status;
  KEYBOARD_INPUT_DATA InputData;

  Buffer->EventType = KEY_EVENT;
  Status = NtReadFile(StdInput,
		      NULL,
		      NULL,
		      NULL,
		      &Iosb,
                      &InputData,
//		      &Buffer->Event.KeyEvent,
		      sizeof(KEY_EVENT_RECORD),
		      NULL,
		      0);

  if (NT_SUCCESS(Status))
    {
      Status = IntTranslateKey(&InputData, &Buffer->Event.KeyEvent);
    }

  return(Status);
}


NTSTATUS
ConReadConsoleOutputCharacters(LPSTR lpCharacter,
			       ULONG nLength,
			       COORD dwReadCoord,
			       PULONG lpNumberOfCharsRead)
{
  IO_STATUS_BLOCK IoStatusBlock;
  OUTPUT_CHARACTER Buffer;
  NTSTATUS Status;

  Buffer.dwCoord    = dwReadCoord;

  Status = NtDeviceIoControlFile(StdOutput,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatusBlock,
				 IOCTL_CONSOLE_READ_OUTPUT_CHARACTER,
				 &Buffer,
				 sizeof(OUTPUT_CHARACTER),
				 (PVOID)lpCharacter,
				 nLength);

  if (NT_SUCCESS(Status) && lpNumberOfCharsRead != NULL)
    {
      *lpNumberOfCharsRead = Buffer.dwTransfered;
    }

  return(Status);
}


NTSTATUS
ConReadConsoleOutputAttributes(PUSHORT lpAttribute,
			       ULONG nLength,
			       COORD dwReadCoord,
			       PULONG lpNumberOfAttrsRead)
{
  IO_STATUS_BLOCK IoStatusBlock;
  OUTPUT_ATTRIBUTE Buffer;
  NTSTATUS Status;

  Buffer.dwCoord = dwReadCoord;

  Status = NtDeviceIoControlFile(StdOutput,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatusBlock,
				 IOCTL_CONSOLE_READ_OUTPUT_ATTRIBUTE,
				 &Buffer,
				 sizeof(OUTPUT_ATTRIBUTE),
				 (PVOID)lpAttribute,
				 nLength);

  if (NT_SUCCESS(Status) && lpNumberOfAttrsRead != NULL)
    {
      *lpNumberOfAttrsRead = Buffer.dwTransfered;
    }

  return(Status);
}


NTSTATUS
ConWriteConsoleOutputCharacters(LPCSTR lpCharacter,
			        ULONG nLength,
			        COORD dwWriteCoord)
{
  IO_STATUS_BLOCK IoStatusBlock;
  PCHAR Buffer;
  COORD *pCoord;
  PCHAR pText;
  NTSTATUS Status;

  Buffer = RtlAllocateHeap(ProcessHeap,
			   0,
			   nLength + sizeof(COORD));
  pCoord = (COORD *)Buffer;
  pText = (PCHAR)(pCoord + 1);

  *pCoord = dwWriteCoord;
  memcpy(pText, lpCharacter, nLength);

  Status = NtDeviceIoControlFile(StdOutput,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatusBlock,
				 IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
				 NULL,
				 0,
				 Buffer,
				 nLength + sizeof(COORD));

  RtlFreeHeap(ProcessHeap,
	      0,
	      Buffer);

  return(Status);
}


NTSTATUS
ConWriteConsoleOutputCharactersW(LPCWSTR lpCharacter,
			         ULONG nLength,
			         COORD dwWriteCoord)
{
  IO_STATUS_BLOCK IoStatusBlock;
  PCHAR Buffer;
  COORD *pCoord;
  PCHAR pText;
  NTSTATUS Status;
  ULONG i;

  Buffer = RtlAllocateHeap(ProcessHeap,
			   0,
			   nLength + sizeof(COORD));
  pCoord = (COORD *)Buffer;
  pText = (PCHAR)(pCoord + 1);

  *pCoord = dwWriteCoord;

  /* FIXME: use real unicode->oem conversion */
  for (i = 0; i < nLength; i++)
    pText[i] = (CHAR)lpCharacter[i];
  pText[i] = 0;

  Status = NtDeviceIoControlFile(StdOutput,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatusBlock,
				 IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
				 NULL,
				 0,
				 Buffer,
				 nLength + sizeof(COORD));

  RtlFreeHeap(ProcessHeap,
	      0,
	      Buffer);

  return(Status);
}


NTSTATUS
ConWriteConsoleOutputAttributes(CONST USHORT *lpAttribute,
			        ULONG nLength,
			        COORD dwWriteCoord,
			        PULONG lpNumberOfAttrsWritten)
{
  IO_STATUS_BLOCK IoStatusBlock;
  PUSHORT Buffer;
  COORD *pCoord;
  PUSHORT pAttrib;
  NTSTATUS Status;

  Buffer = RtlAllocateHeap(ProcessHeap,
			   0,
			   nLength * sizeof(USHORT) + sizeof(COORD));
  pCoord = (COORD *)Buffer;
  pAttrib = (PUSHORT)(pCoord + 1);

  *pCoord = dwWriteCoord;
  memcpy(pAttrib, lpAttribute, nLength * sizeof(USHORT));

  Status = NtDeviceIoControlFile(StdOutput,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatusBlock,
				 IOCTL_CONSOLE_WRITE_OUTPUT_ATTRIBUTE,
				 NULL,
				 0,
				 Buffer,
				 nLength * sizeof(USHORT) + sizeof(COORD));

  RtlFreeHeap(ProcessHeap,
	      0,
	      Buffer);

  return(Status);
}


NTSTATUS
ConFillConsoleOutputAttribute(USHORT wAttribute,
			      ULONG nLength,
			      COORD dwWriteCoord,
			      PULONG lpNumberOfAttrsWritten)
{
  IO_STATUS_BLOCK IoStatusBlock;
  OUTPUT_ATTRIBUTE Buffer;
  NTSTATUS Status;

  Buffer.wAttribute = wAttribute;
  Buffer.nLength    = nLength;
  Buffer.dwCoord    = dwWriteCoord;

  Status = NtDeviceIoControlFile(StdOutput,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatusBlock,
				 IOCTL_CONSOLE_FILL_OUTPUT_ATTRIBUTE,
				 &Buffer,
				 sizeof(OUTPUT_ATTRIBUTE),
				 &Buffer,
				 sizeof(OUTPUT_ATTRIBUTE));

  if (NT_SUCCESS(Status))
    {
      *lpNumberOfAttrsWritten = Buffer.dwTransfered;
    }

  return(Status);
}


NTSTATUS
ConFillConsoleOutputCharacter(CHAR Character,
			      ULONG Length,
			      COORD WriteCoord,
			      PULONG NumberOfCharsWritten)
{
  IO_STATUS_BLOCK IoStatusBlock;
  OUTPUT_CHARACTER Buffer;
  NTSTATUS Status;

  Buffer.cCharacter = Character;
  Buffer.nLength = Length;
  Buffer.dwCoord = WriteCoord;

  Status = NtDeviceIoControlFile(StdOutput,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatusBlock,
				 IOCTL_CONSOLE_FILL_OUTPUT_CHARACTER,
				 &Buffer,
				 sizeof(OUTPUT_CHARACTER),
				 &Buffer,
				 sizeof(OUTPUT_CHARACTER));

  if (NT_SUCCESS(Status))
    {
      *NumberOfCharsWritten = Buffer.dwTransfered;
    }

  return(Status);
}


#if 0
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


NTSTATUS
SetConsoleMode(HANDLE hConsoleHandle,
	       ULONG dwMode)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;
  CONSOLE_MODE Buffer;

  Buffer.dwMode = dwMode;

  Status = NtDeviceIoControlFile(hConsoleHandle,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatusBlock,
				 IOCTL_CONSOLE_SET_MODE,
				 &Buffer,
				 sizeof(CONSOLE_MODE),
				 NULL,
				 0);

  return(Status);
}
#endif


NTSTATUS
ConGetConsoleScreenBufferInfo(PCONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;

  Status = NtDeviceIoControlFile(StdOutput,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatusBlock,
				 IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO,
				 NULL,
				 0,
				 ConsoleScreenBufferInfo,
				 sizeof(CONSOLE_SCREEN_BUFFER_INFO));

  return(Status);
}


NTSTATUS
ConSetConsoleCursorInfo(PCONSOLE_CURSOR_INFO lpConsoleCursorInfo)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;

  Status = NtDeviceIoControlFile(StdOutput,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatusBlock,
				 IOCTL_CONSOLE_SET_CURSOR_INFO,
				 (PCONSOLE_CURSOR_INFO)lpConsoleCursorInfo,
				 sizeof(CONSOLE_CURSOR_INFO),
				 NULL,
				 0);

  return(Status);
}


NTSTATUS
ConSetConsoleCursorPosition(COORD dwCursorPosition)
{
  CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;

  Status = ConGetConsoleScreenBufferInfo(&ConsoleScreenBufferInfo);
  if (!NT_SUCCESS(Status))
    return(Status);

  ConsoleScreenBufferInfo.dwCursorPosition.X = dwCursorPosition.X;
  ConsoleScreenBufferInfo.dwCursorPosition.Y = dwCursorPosition.Y;

  Status = NtDeviceIoControlFile(StdOutput,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatusBlock,
				 IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO,
				 &ConsoleScreenBufferInfo,
				 sizeof(CONSOLE_SCREEN_BUFFER_INFO),
				 NULL,
				 0);

  return(Status);
}


NTSTATUS
ConSetConsoleTextAttribute(USHORT wAttributes)
{
  IO_STATUS_BLOCK IoStatusBlock;
  NTSTATUS Status;

  Status = NtDeviceIoControlFile(StdOutput,
				 NULL,
				 NULL,
				 NULL,
				 &IoStatusBlock,
				 IOCTL_CONSOLE_SET_TEXT_ATTRIBUTE,
				 &wAttributes,
				 sizeof(USHORT),
				 NULL,
				 0);

  return(Status);
}




VOID
ConInKey(PINPUT_RECORD Buffer)
{

  while (TRUE)
    {
      ConReadConsoleInput(Buffer);

      if ((Buffer->EventType == KEY_EVENT) &&
	  (Buffer->Event.KeyEvent.bKeyDown == TRUE))
	  break;
    }
}


VOID
ConOutChar(CHAR c)
{
  ULONG Written;

  ConWriteConsole(&c,
	          1,
	          &Written);
}


VOID
ConOutPuts(LPSTR szText)
{
  ULONG Written;

  ConWriteConsole(szText,
	          strlen(szText),
	          &Written);
  ConWriteConsole("\n",
	          1,
	          &Written);
}


VOID
ConOutPrintf(LPSTR szFormat, ...)
{
  CHAR szOut[256];
  DWORD dwWritten;
  va_list arg_ptr;

  va_start(arg_ptr, szFormat);
  vsprintf(szOut, szFormat, arg_ptr);
  va_end(arg_ptr);

  ConWriteConsole(szOut,
	          strlen(szOut),
	          &dwWritten);
}





SHORT
GetCursorX(VOID)
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  ConGetConsoleScreenBufferInfo(&csbi);

  return(csbi.dwCursorPosition.X);
}


SHORT
GetCursorY(VOID)
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  ConGetConsoleScreenBufferInfo(&csbi);

  return(csbi.dwCursorPosition.Y);
}


VOID
GetScreenSize(SHORT *maxx,
	      SHORT *maxy)
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  ConGetConsoleScreenBufferInfo(&csbi);

  if (maxx)
    *maxx = csbi.dwSize.X;

  if (maxy)
    *maxy = csbi.dwSize.Y;
}


VOID
SetCursorType(BOOL bInsert,
	      BOOL bVisible)
{
  CONSOLE_CURSOR_INFO cci;

  cci.dwSize = bInsert ? 10 : 99;
  cci.bVisible = bVisible;

  ConSetConsoleCursorInfo(&cci);
}


VOID
SetCursorXY(SHORT x,
	    SHORT y)
{
  COORD coPos;

  coPos.X = x;
  coPos.Y = y;
  ConSetConsoleCursorPosition(coPos);
}


VOID
ClearScreen(VOID)
{
  COORD coPos;
  ULONG Written;

  coPos.X = 0;
  coPos.Y = 0;

  FillConsoleOutputAttribute(0x17,
			     xScreen * yScreen,
			     coPos,
			     &Written);

  FillConsoleOutputCharacter(' ',
			     xScreen * yScreen,
			     coPos,
			     &Written);
}


VOID
SetStatusText(char* fmt, ...)
{
  char Buffer[128];
  va_list ap;
  COORD coPos;
  ULONG Written;

  va_start(ap, fmt);
  vsprintf(Buffer, fmt, ap);
  va_end(ap);

  coPos.X = 0;
  coPos.Y = yScreen - 1;

  FillConsoleOutputAttribute(0x70,
			     xScreen,
			     coPos,
			     &Written);

  FillConsoleOutputCharacter(' ',
			     xScreen,
			     coPos,
			     &Written);

  WriteConsoleOutputCharacters(Buffer,
			       strlen(Buffer),
			       coPos);
}


VOID
InvertTextXY(SHORT x, SHORT y, SHORT col, SHORT row)
{
  COORD coPos;
  ULONG Written;

  for (coPos.Y = y; coPos.Y < y + row; coPos.Y++)
    {
      coPos.X = x;

      FillConsoleOutputAttribute(0x71,
				 col,
				 coPos,
				 &Written);
    }
}


VOID
NormalTextXY(SHORT x, SHORT y, SHORT col, SHORT row)
{
  COORD coPos;
  ULONG Written;

  for (coPos.Y = y; coPos.Y < y + row; coPos.Y++)
    {
      coPos.X = x;

      FillConsoleOutputAttribute(0x17,
				 col,
				 coPos,
				 &Written);
    }
}


VOID
SetTextXY(SHORT x, SHORT y, PCHAR Text)
{
  COORD coPos;

  coPos.X = x;
  coPos.Y = y;

  WriteConsoleOutputCharacters(Text,
			       strlen(Text),
			       coPos);
}


VOID
SetInputTextXY(SHORT x, SHORT y, SHORT len, PWCHAR Text)
{
  COORD coPos;
  ULONG Length;
  ULONG Written;

  coPos.X = x;
  coPos.Y = y;

  Length = wcslen(Text);
  if (Length > len - 1)
    {
      Length = len - 1;
    }

  ConFillConsoleOutputAttribute(0x70,
			        len,
			        coPos,
			        &Written);

  ConWriteConsoleOutputCharactersW(Text,
				   Length,
				   coPos);

  coPos.X += Length;
  ConFillConsoleOutputCharacter('_',
			        1,
			        coPos,
			        &Written);

  if (len > Length + 1)
    {
      coPos.X++;
      ConFillConsoleOutputCharacter(' ',
				    len - Length - 1,
				    coPos,
				    &Written);
    }
}


VOID
SetUnderlinedTextXY(SHORT x, SHORT y, PCHAR Text)
{
  COORD coPos;
  ULONG Length;
  ULONG Written;

  coPos.X = x;
  coPos.Y = y;

  Length = strlen(Text);

  ConWriteConsoleOutputCharacters(Text,
			          Length,
			          coPos);

  coPos.Y++;
  ConFillConsoleOutputCharacter(0xCD,
			        Length,
			        coPos,
			        &Written);
}


VOID
SetInvertedTextXY(SHORT x, SHORT y, PCHAR Text)
{
  COORD coPos;
  ULONG Length;
  ULONG Written;

  coPos.X = x;
  coPos.Y = y;

  Length = strlen(Text);

  ConFillConsoleOutputAttribute(0x71,
			        Length,
			        coPos,
			        &Written);

  ConWriteConsoleOutputCharacters(Text,
			          Length,
			          coPos);
}


VOID
SetHighlightedTextXY(SHORT x, SHORT y, PCHAR Text)
{
  COORD coPos;
  ULONG Length;
  ULONG Written;

  coPos.X = x;
  coPos.Y = y;

  Length = strlen(Text);

  ConFillConsoleOutputAttribute(0x1F,
			        Length,
			        coPos,
			        &Written);

  ConWriteConsoleOutputCharacters(Text,
			          Length,
			          coPos);
}


VOID
PrintTextXY(SHORT x, SHORT y, char* fmt, ...)
{
  char buffer[512];
  va_list ap;
  COORD coPos;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  coPos.X = x;
  coPos.Y = y;

  ConWriteConsoleOutputCharacters(buffer,
			          strlen(buffer),
			          coPos);
}


VOID
PrintTextXYN(SHORT x, SHORT y, SHORT len, char* fmt, ...)
{
  char buffer[512];
  va_list ap;
  COORD coPos;
  ULONG Length;
  ULONG Written;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  coPos.X = x;
  coPos.Y = y;

  Length = strlen(buffer);
  if (Length > len - 1)
    {
      Length = len - 1;
    }

  ConWriteConsoleOutputCharacters(buffer,
			          Length,
			          coPos);

  coPos.X += Length;

  if (len > Length)
    {
      ConFillConsoleOutputCharacter(' ',
				    len - Length,
				    coPos,
				    &Written);
    }
}


/* EOF */
