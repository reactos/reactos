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

#include <usetup.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

static HANDLE StdInput  = INVALID_HANDLE_VALUE;
static HANDLE StdOutput = INVALID_HANDLE_VALUE;

static SHORT xScreen = 0;
static SHORT yScreen = 0;

/* FUNCTIONS *****************************************************************/

BOOL WINAPI
ConAllocConsole(
	IN DWORD dwProcessId)
{
	UNICODE_STRING ScreenName = RTL_CONSTANT_STRING(L"\\??\\BlueScreen");
	UNICODE_STRING KeyboardName = RTL_CONSTANT_STRING(L"\\Device\\KeyboardClass0");
	OBJECT_ATTRIBUTES ObjectAttributes;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	/* Open the screen */
	InitializeObjectAttributes(
		&ObjectAttributes,
		&ScreenName,
		0,
		NULL,
		NULL);
	Status = NtOpenFile(
		&StdOutput,
		FILE_ALL_ACCESS,
		&ObjectAttributes,
		&IoStatusBlock,
		0,
		FILE_SYNCHRONOUS_IO_ALERT);
	if (!NT_SUCCESS(Status))
		return FALSE;

	/* Open the keyboard */
	InitializeObjectAttributes(
		&ObjectAttributes,
		&KeyboardName,
		0,
		NULL,
		NULL);
	Status = NtOpenFile(
		&StdInput,
		FILE_ALL_ACCESS,
		&ObjectAttributes,
		&IoStatusBlock,
		0,
		FILE_SYNCHRONOUS_IO_ALERT);
	if (!NT_SUCCESS(Status))
		return FALSE;

	if (!GetConsoleScreenBufferInfo(StdOutput, &csbi))
		return FALSE;

	xScreen = csbi.dwSize.X;
	yScreen = csbi.dwSize.Y;

	return TRUE;
}

BOOL WINAPI
ConFreeConsole(VOID)
{
	DPRINT("FreeConsole() called\n");

	if (StdInput != INVALID_HANDLE_VALUE)
		NtClose(StdInput);

	if (StdOutput != INVALID_HANDLE_VALUE)
		NtClose(StdOutput);

	DPRINT("FreeConsole() done\n");
	return TRUE;
}

BOOL WINAPI
ConWriteConsole(
	IN HANDLE hConsoleOutput,
	IN const VOID* lpBuffer,
	IN DWORD nNumberOfCharsToWrite,
	OUT LPDWORD lpNumberOfCharsWritten,
	IN LPVOID lpReserved)
{
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	Status = NtWriteFile(
		hConsoleOutput,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		(PVOID)lpBuffer,
		nNumberOfCharsToWrite,
		NULL,
		NULL);
	if (!NT_SUCCESS(Status))
		return FALSE;

	*lpNumberOfCharsWritten = IoStatusBlock.Information;
	return TRUE;
}

BOOL WINAPI
ConReadConsoleInput(
	IN HANDLE hConsoleInput,
	OUT PINPUT_RECORD lpBuffer,
	IN DWORD nLength,
	OUT LPDWORD lpNumberOfEventsRead)
{
	IO_STATUS_BLOCK IoStatusBlock;
	KEYBOARD_INPUT_DATA InputData;
	NTSTATUS Status;

	lpBuffer->EventType = KEY_EVENT;
	Status = NtReadFile(
		hConsoleInput,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		&InputData,
		sizeof(KEYBOARD_INPUT_DATA),
		NULL,
		0);
	if (!NT_SUCCESS(Status))
		return FALSE;

	Status = IntTranslateKey(&InputData, &lpBuffer->Event.KeyEvent);
	if (!NT_SUCCESS(Status))
		return FALSE;

	*lpNumberOfEventsRead = 1;
	return TRUE;
}

BOOL WINAPI
ConWriteConsoleOutputCharacterA(
	HANDLE hConsoleOutput,
	IN LPCSTR lpCharacter,
	IN DWORD nLength,
	IN COORD dwWriteCoord,
	OUT LPDWORD lpNumberOfCharsWritten)
{
	IO_STATUS_BLOCK IoStatusBlock;
	PCHAR Buffer;
	COORD *pCoord;
	PCHAR pText;
	NTSTATUS Status;

	Buffer = RtlAllocateHeap(
		ProcessHeap,
		0,
		nLength + sizeof(COORD));
	pCoord = (COORD *)Buffer;
	pText = (PCHAR)(pCoord + 1);

	*pCoord = dwWriteCoord;
	memcpy(pText, lpCharacter, nLength);

	Status = NtDeviceIoControlFile(
		hConsoleOutput,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
		NULL,
		0,
		Buffer,
		nLength + sizeof(COORD));

	RtlFreeHeap(
		ProcessHeap,
		0,
		Buffer);
	if (!NT_SUCCESS(Status))
		return FALSE;

	*lpNumberOfCharsWritten = IoStatusBlock.Information;
	return TRUE;
}

BOOL WINAPI
ConWriteConsoleOutputCharacterW(
	HANDLE hConsoleOutput,
	IN LPCWSTR lpCharacter,
	IN DWORD nLength,
	IN COORD dwWriteCoord,
	OUT LPDWORD lpNumberOfCharsWritten)
{
	IO_STATUS_BLOCK IoStatusBlock;
	PCHAR Buffer;
	COORD *pCoord;
	PCHAR pText;
	NTSTATUS Status;
	ULONG i;

	Buffer = RtlAllocateHeap(
		ProcessHeap,
		0,
		nLength + sizeof(COORD));
	pCoord = (COORD *)Buffer;
	pText = (PCHAR)(pCoord + 1);

	*pCoord = dwWriteCoord;

	/* FIXME: use real unicode->oem conversion */
	for (i = 0; i < nLength; i++)
		pText[i] = (CHAR)lpCharacter[i];
	pText[i] = 0;

	Status = NtDeviceIoControlFile(
		hConsoleOutput,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IOCTL_CONSOLE_WRITE_OUTPUT_CHARACTER,
		NULL,
		0,
		Buffer,
		nLength + sizeof(COORD));

	RtlFreeHeap(
		ProcessHeap,
		0,
		Buffer);
	if (!NT_SUCCESS(Status))
		return FALSE;

	*lpNumberOfCharsWritten = IoStatusBlock.Information;
	return TRUE;
}

BOOL WINAPI
ConFillConsoleOutputAttribute(
	IN HANDLE hConsoleOutput,
	IN WORD wAttribute,
	IN DWORD nLength,
	IN COORD dwWriteCoord,
	OUT LPDWORD lpNumberOfAttrsWritten)
{
	IO_STATUS_BLOCK IoStatusBlock;
	OUTPUT_ATTRIBUTE Buffer;
	NTSTATUS Status;

	Buffer.wAttribute = wAttribute;
	Buffer.nLength    = nLength;
	Buffer.dwCoord    = dwWriteCoord;

	Status = NtDeviceIoControlFile(
		hConsoleOutput,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IOCTL_CONSOLE_FILL_OUTPUT_ATTRIBUTE,
		&Buffer,
		sizeof(OUTPUT_ATTRIBUTE),
		&Buffer,
		sizeof(OUTPUT_ATTRIBUTE));

	if (!NT_SUCCESS(Status))
		return FALSE;

	*lpNumberOfAttrsWritten = Buffer.dwTransfered;
	return TRUE;
}

BOOL WINAPI
ConFillConsoleOutputCharacterA(
	IN HANDLE hConsoleOutput,
	IN CHAR cCharacter,
	IN DWORD nLength,
	IN COORD dwWriteCoord,
	OUT LPDWORD lpNumberOfCharsWritten)
{
	IO_STATUS_BLOCK IoStatusBlock;
	OUTPUT_CHARACTER Buffer;
	NTSTATUS Status;

	Buffer.cCharacter = cCharacter;
	Buffer.nLength = nLength;
	Buffer.dwCoord = dwWriteCoord;

	Status = NtDeviceIoControlFile(
		hConsoleOutput,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IOCTL_CONSOLE_FILL_OUTPUT_CHARACTER,
		&Buffer,
		sizeof(OUTPUT_CHARACTER),
		&Buffer,
		sizeof(OUTPUT_CHARACTER));
	if (!NT_SUCCESS(Status))
		return FALSE;

	*lpNumberOfCharsWritten = Buffer.dwTransfered;
	return TRUE;
}

BOOL WINAPI
ConGetConsoleScreenBufferInfo(
	IN HANDLE hConsoleOutput,
	OUT PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo)
{
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	Status = NtDeviceIoControlFile(
		hConsoleOutput,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IOCTL_CONSOLE_GET_SCREEN_BUFFER_INFO,
		NULL,
		0,
		lpConsoleScreenBufferInfo,
		sizeof(CONSOLE_SCREEN_BUFFER_INFO));
	return NT_SUCCESS(Status);
}

BOOL WINAPI
ConSetConsoleCursorInfo(
	IN HANDLE hConsoleOutput,
	IN const CONSOLE_CURSOR_INFO* lpConsoleCursorInfo)
{
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	Status = NtDeviceIoControlFile(
		hConsoleOutput,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IOCTL_CONSOLE_SET_CURSOR_INFO,
		(PCONSOLE_CURSOR_INFO)lpConsoleCursorInfo,
		sizeof(CONSOLE_CURSOR_INFO),
		NULL,
		0);
	return NT_SUCCESS(Status);
}

BOOL WINAPI
ConSetConsoleCursorPosition(
	IN HANDLE hConsoleOutput,
	IN COORD dwCursorPosition)
{
	CONSOLE_SCREEN_BUFFER_INFO ConsoleScreenBufferInfo;
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	Status = GetConsoleScreenBufferInfo(hConsoleOutput, &ConsoleScreenBufferInfo);
	if (!NT_SUCCESS(Status))
		return FALSE;

	ConsoleScreenBufferInfo.dwCursorPosition.X = dwCursorPosition.X;
	ConsoleScreenBufferInfo.dwCursorPosition.Y = dwCursorPosition.Y;

	Status = NtDeviceIoControlFile(
		hConsoleOutput,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IOCTL_CONSOLE_SET_SCREEN_BUFFER_INFO,
		&ConsoleScreenBufferInfo,
		sizeof(CONSOLE_SCREEN_BUFFER_INFO),
		NULL,
		0);

	return NT_SUCCESS(Status);
}

BOOL WINAPI
ConSetConsoleTextAttribute(
	IN HANDLE hConsoleOutput,
	IN WORD wAttributes)
{
	IO_STATUS_BLOCK IoStatusBlock;
	NTSTATUS Status;

	Status = NtDeviceIoControlFile(
		hConsoleOutput,
		NULL,
		NULL,
		NULL,
		&IoStatusBlock,
		IOCTL_CONSOLE_SET_TEXT_ATTRIBUTE,
		&wAttributes,
		sizeof(USHORT),
		NULL,
		0);
	return NT_SUCCESS(Status);
}




VOID
CONSOLE_ConInKey(PINPUT_RECORD Buffer)
{
	ULONG Read;

  while (TRUE)
    {
      ReadConsoleInput(StdInput, Buffer, 1, &Read);

      if ((Buffer->EventType == KEY_EVENT) &&
	  (Buffer->Event.KeyEvent.bKeyDown == TRUE))
	  break;
    }
}


VOID
CONSOLE_ConOutChar(CHAR c)
{
  ULONG Written;

  WriteConsole(StdOutput,
	          &c,
	          1,
	          &Written,
	          NULL);
}


VOID
CONSOLE_ConOutPuts(LPSTR szText)
{
  ULONG Written;

  WriteConsole(StdOutput,
	          szText,
	          strlen(szText),
	          &Written,
	          NULL);
  WriteConsole(StdOutput,
	          "\n",
	          1,
	          &Written,
	          NULL);
}


VOID
CONSOLE_ConOutPrintf(LPSTR szFormat, ...)
{
  CHAR szOut[256];
  DWORD dwWritten;
  va_list arg_ptr;

  va_start(arg_ptr, szFormat);
  vsprintf(szOut, szFormat, arg_ptr);
  va_end(arg_ptr);

  WriteConsole(StdOutput,
	          szOut,
	          strlen(szOut),
	          &dwWritten,
	          NULL);
}





SHORT
CONSOLE_GetCursorX(VOID)
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  GetConsoleScreenBufferInfo(StdOutput, &csbi);

  return(csbi.dwCursorPosition.X);
}


SHORT
CONSOLE_GetCursorY(VOID)
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  GetConsoleScreenBufferInfo(StdOutput, &csbi);

  return(csbi.dwCursorPosition.Y);
}


VOID
CONSOLE_GetScreenSize(SHORT *maxx,
	      SHORT *maxy)
{
  CONSOLE_SCREEN_BUFFER_INFO csbi;

  GetConsoleScreenBufferInfo(StdOutput, &csbi);

  if (maxx)
    *maxx = csbi.dwSize.X;

  if (maxy)
    *maxy = csbi.dwSize.Y;
}


VOID
CONSOLE_SetCursorType(BOOL bInsert,
	      BOOL bVisible)
{
  CONSOLE_CURSOR_INFO cci;

  cci.dwSize = bInsert ? 10 : 99;
  cci.bVisible = bVisible;

  SetConsoleCursorInfo(StdOutput, &cci);
}


VOID
CONSOLE_SetCursorXY(SHORT x,
	    SHORT y)
{
  COORD coPos;

  coPos.X = x;
  coPos.Y = y;
  SetConsoleCursorPosition(StdOutput, coPos);
}

VOID
CONSOLE_ClearScreen(VOID)
{
	COORD coPos;
	ULONG Written;

	coPos.X = 0;
	coPos.Y = 0;

	FillConsoleOutputAttribute(
		StdOutput,
		FOREGROUND_WHITE | BACKGROUND_BLUE,
		xScreen * yScreen,
		coPos,
		&Written);

	FillConsoleOutputCharacterA(
		StdOutput,
		' ',
		xScreen * yScreen,
		coPos,
		&Written);
}


VOID
CONSOLE_SetStatusText(char* fmt, ...)
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

  FillConsoleOutputAttribute(StdOutput,
			     BACKGROUND_WHITE,
			     xScreen,
			     coPos,
			     &Written);

  FillConsoleOutputCharacterA(StdOutput,
			     ' ',
			     xScreen,
			     coPos,
			     &Written);

  WriteConsoleOutputCharacterA(StdOutput,
			       Buffer,
			       strlen(Buffer),
			       coPos,
			       &Written);
}


VOID
CONSOLE_InvertTextXY(SHORT x, SHORT y, SHORT col, SHORT row)
{
  COORD coPos;
  ULONG Written;

  for (coPos.Y = y; coPos.Y < y + row; coPos.Y++)
    {
      coPos.X = x;

      FillConsoleOutputAttribute(StdOutput,
				 FOREGROUND_BLUE | BACKGROUND_WHITE,
				 col,
				 coPos,
				 &Written);
    }
}


VOID
CONSOLE_NormalTextXY(SHORT x, SHORT y, SHORT col, SHORT row)
{
  COORD coPos;
  ULONG Written;

  for (coPos.Y = y; coPos.Y < y + row; coPos.Y++)
    {
      coPos.X = x;

      FillConsoleOutputAttribute(StdOutput,
				 FOREGROUND_BLUE | BACKGROUND_WHITE,
				 col,
				 coPos,
				 &Written);
    }
}


VOID
CONSOLE_SetTextXY(SHORT x, SHORT y, PCHAR Text)
{
  COORD coPos;
  ULONG Written;

  coPos.X = x;
  coPos.Y = y;

  WriteConsoleOutputCharacterA(StdOutput,
			       Text,
			       strlen(Text),
			       coPos,
			       &Written);
}


VOID
CONSOLE_SetInputTextXY(SHORT x, SHORT y, SHORT len, PWCHAR Text)
{
  COORD coPos;
  ULONG Length;
  ULONG Written;

  coPos.X = x;
  coPos.Y = y;

  Length = wcslen(Text);
  if (Length > (ULONG)len - 1)
    {
      Length = len - 1;
    }

  FillConsoleOutputAttribute(StdOutput,
			        BACKGROUND_WHITE,
			        len,
			        coPos,
			        &Written);

  WriteConsoleOutputCharacterW(StdOutput,
				   Text,
				   Length,
				   coPos,
				   &Written);

  coPos.X += Length;
  FillConsoleOutputCharacterA(StdOutput,
			        '_',
			        1,
			        coPos,
			        &Written);

  if ((ULONG)len > Length + 1)
    {
      coPos.X++;
      FillConsoleOutputCharacterA(StdOutput,
				    ' ',
				    len - Length - 1,
				    coPos,
				    &Written);
    }
}


VOID
CONSOLE_SetUnderlinedTextXY(SHORT x, SHORT y, PCHAR Text)
{
  COORD coPos;
  ULONG Length;
  ULONG Written;

  coPos.X = x;
  coPos.Y = y;

  Length = strlen(Text);

  WriteConsoleOutputCharacterA(StdOutput,
			          Text,
			          Length,
			          coPos,
			          &Written);

  coPos.Y++;
  FillConsoleOutputCharacterA(StdOutput,
			        0xCD,
			        Length,
			        coPos,
			        &Written);
}


VOID
CONSOLE_SetInvertedTextXY(SHORT x, SHORT y, PCHAR Text)
{
  COORD coPos;
  ULONG Length;
  ULONG Written;

  coPos.X = x;
  coPos.Y = y;

  Length = strlen(Text);

  FillConsoleOutputAttribute(StdOutput,
			        FOREGROUND_BLUE | BACKGROUND_WHITE,
			        Length,
			        coPos,
			        &Written);

  WriteConsoleOutputCharacterA(StdOutput,
			          Text,
			          Length,
			          coPos,
			          &Written);
}


VOID
CONSOLE_SetHighlightedTextXY(SHORT x, SHORT y, PCHAR Text)
{
  COORD coPos;
  ULONG Length;
  ULONG Written;

  coPos.X = x;
  coPos.Y = y;

  Length = strlen(Text);

  FillConsoleOutputAttribute(StdOutput,
			        FOREGROUND_WHITE | FOREGROUND_INTENSITY | BACKGROUND_BLUE,
			        Length,
			        coPos,
			        &Written);

  WriteConsoleOutputCharacterA(StdOutput,
			          Text,
			          Length,
			          coPos,
			          &Written);
}


VOID
CONSOLE_PrintTextXY(SHORT x, SHORT y, char* fmt, ...)
{
  char buffer[512];
  va_list ap;
  COORD coPos;
  ULONG Written;

  va_start(ap, fmt);
  vsprintf(buffer, fmt, ap);
  va_end(ap);

  coPos.X = x;
  coPos.Y = y;

  WriteConsoleOutputCharacterA(StdOutput,
			          buffer,
			          strlen(buffer),
			          coPos,
			          &Written);
}


VOID
CONSOLE_PrintTextXYN(SHORT x, SHORT y, SHORT len, char* fmt, ...)
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
  if (Length > (ULONG)len - 1)
    {
      Length = len - 1;
    }

  WriteConsoleOutputCharacterA(StdOutput,
			          buffer,
			          Length,
			          coPos,
			          &Written);

  coPos.X += Length;

  if ((ULONG)len > Length)
    {
      FillConsoleOutputCharacterA(StdOutput,
				    ' ',
				    len - Length,
				    coPos,
				    &Written);
    }
}

/* EOF */
