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

HANDLE StdInput  = INVALID_HANDLE_VALUE;
HANDLE StdOutput = INVALID_HANDLE_VALUE;

SHORT xScreen = 0;
SHORT yScreen = 0;

/* FUNCTIONS *****************************************************************/

BOOLEAN
CONSOLE_Init(
	VOID)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;
	if (!HOST_InitConsole())
		return FALSE;

	StdInput = GetStdHandle(STD_INPUT_HANDLE);
	StdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
	if (!GetConsoleScreenBufferInfo(StdOutput, &csbi))
		return FALSE;
	xScreen = csbi.dwSize.X;
	yScreen = 50;//csbi.dwSize.Y;
	return TRUE;
}

VOID
CONSOLE_ConInKey(
	OUT PINPUT_RECORD Buffer)
{
	DWORD Read;

	while (TRUE)
	{
		ReadConsoleInput(StdInput, Buffer, 1, &Read);

		if ((Buffer->EventType == KEY_EVENT)
		 && (Buffer->Event.KeyEvent.bKeyDown == TRUE))
			break;
	}
}

VOID
CONSOLE_ConOutChar(
	IN CHAR c)
{
	DWORD Written;

	WriteConsole(
		StdOutput,
		&c,
		1,
		&Written,
		NULL);
}

VOID
CONSOLE_ConOutPuts(
	IN LPCSTR szText)
{
	DWORD Written;

	WriteConsole(
		StdOutput,
		szText,
		(ULONG)strlen(szText),
		&Written,
		NULL);
	WriteConsole(
		StdOutput,
		"\n",
		1,
		&Written,
		NULL);
}

VOID
CONSOLE_ConOutPrintf(
	IN LPCSTR szFormat, ...)
{
	CHAR szOut[256];
	DWORD dwWritten;
	va_list arg_ptr;

	va_start(arg_ptr, szFormat);
	vsprintf(szOut, szFormat, arg_ptr);
	va_end(arg_ptr);

	WriteConsole(
		StdOutput,
		szOut,
		(ULONG)strlen(szOut),
		&dwWritten,
		NULL);
}

SHORT
CONSOLE_GetCursorX(VOID)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(StdOutput, &csbi);

	return csbi.dwCursorPosition.X;
}

SHORT
CONSOLE_GetCursorY(VOID)
{
	CONSOLE_SCREEN_BUFFER_INFO csbi;

	GetConsoleScreenBufferInfo(StdOutput, &csbi);

	return csbi.dwCursorPosition.Y;
}

VOID
CONSOLE_SetCursorType(
	IN BOOL bInsert,
	IN BOOL bVisible)
{
	CONSOLE_CURSOR_INFO cci;

	cci.dwSize = bInsert ? 10 : 99;
	cci.bVisible = bVisible;

	SetConsoleCursorInfo(StdOutput, &cci);
}

VOID
CONSOLE_SetCursorXY(
	IN SHORT x,
	IN SHORT y)
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
	DWORD Written;

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
CONSOLE_SetStatusText(
	IN LPCSTR fmt, ...)
{
	CHAR Buffer[128];
	va_list ap;
	COORD coPos;
	DWORD Written;

	va_start(ap, fmt);
	vsprintf(Buffer, fmt, ap);
	va_end(ap);

	coPos.X = 0;
	coPos.Y = yScreen - 1;

	FillConsoleOutputAttribute(
		StdOutput,
		BACKGROUND_WHITE,
		xScreen,
		coPos,
		&Written);

	FillConsoleOutputCharacterA(
		StdOutput,
		' ',
		xScreen,
		coPos,
		&Written);

	WriteConsoleOutputCharacterA(
		StdOutput,
		Buffer,
		(ULONG)strlen(Buffer),
		coPos,
		&Written);
}

VOID
CONSOLE_InvertTextXY(
	IN SHORT x,
	IN SHORT y,
	IN SHORT col,
	IN SHORT row)
{
	COORD coPos;
	DWORD Written;

	for (coPos.Y = y; coPos.Y < y + row; coPos.Y++)
	{
		coPos.X = x;

		FillConsoleOutputAttribute(
			StdOutput,
			FOREGROUND_BLUE | BACKGROUND_WHITE,
			col,
			coPos,
			&Written);
	}
}

VOID
CONSOLE_NormalTextXY(
	IN SHORT x,
	IN SHORT y,
	IN SHORT col,
	IN SHORT row)
{
	COORD coPos;
	DWORD Written;

	for (coPos.Y = y; coPos.Y < y + row; coPos.Y++)
	{
		coPos.X = x;

		FillConsoleOutputAttribute(
			StdOutput,
			FOREGROUND_WHITE | BACKGROUND_BLUE,
			col,
			coPos,
			&Written);
	}
}

VOID
CONSOLE_SetTextXY(
	IN SHORT x,
	IN SHORT y,
	IN LPCSTR Text)
{
	COORD coPos;
	DWORD Written;

	coPos.X = x;
	coPos.Y = y;

	WriteConsoleOutputCharacterA(
		StdOutput,
		Text,
		(ULONG)strlen(Text),
		coPos,
		&Written);
}

VOID
CONSOLE_SetInputTextXY(
	IN SHORT x,
	IN SHORT y,
	IN SHORT len,
	IN LPCWSTR Text)
{
	COORD coPos;
	SHORT Length;
	DWORD Written;

	coPos.X = x;
	coPos.Y = y;

	Length = (SHORT)wcslen(Text);
	if (Length > len - 1)
		Length = len - 1;

	FillConsoleOutputAttribute(
		StdOutput,
		BACKGROUND_WHITE,
		len,
		coPos,
		&Written);

	WriteConsoleOutputCharacterW(
		StdOutput,
		Text,
		(ULONG)Length,
		coPos,
		&Written);

	coPos.X += Length;
	FillConsoleOutputCharacterA(
		StdOutput,
		'_',
		1,
		coPos,
		&Written);

	if (len > Length + 1)
	{
		coPos.X++;
		FillConsoleOutputCharacterA(
			StdOutput,
			' ',
			len - Length - 1,
			coPos,
			&Written);
	}
}

VOID
CONSOLE_SetUnderlinedTextXY(
	IN SHORT x,
	IN SHORT y,
	IN LPCSTR Text)
{
	COORD coPos;
	DWORD Length;
	DWORD Written;

	coPos.X = x;
	coPos.Y = y;

	Length = (ULONG)strlen(Text);

	WriteConsoleOutputCharacterA(
		StdOutput,
		Text,
		Length,
		coPos,
		&Written);

	coPos.Y++;
	FillConsoleOutputCharacterA(
		StdOutput,
		0xCD,
		Length,
		coPos,
		&Written);
}

VOID
CONSOLE_SetInvertedTextXY(
	IN SHORT x,
	IN SHORT y,
	IN LPCSTR Text)
{
	COORD coPos;
	DWORD Length;
	DWORD Written;

	coPos.X = x;
	coPos.Y = y;

	Length = (ULONG)strlen(Text);

	FillConsoleOutputAttribute(
		StdOutput,
		FOREGROUND_BLUE | BACKGROUND_WHITE,
		Length,
		coPos,
		&Written);

	WriteConsoleOutputCharacterA(
		StdOutput,
		Text,
		Length,
		coPos,
		&Written);
}

VOID
CONSOLE_SetHighlightedTextXY(
	IN SHORT x,
	IN SHORT y,
	IN LPCSTR Text)
{
	COORD coPos;
	DWORD Length;
	DWORD Written;

	coPos.X = x;
	coPos.Y = y;

	Length = (ULONG)strlen(Text);

	FillConsoleOutputAttribute(
		StdOutput,
		FOREGROUND_WHITE | FOREGROUND_INTENSITY | BACKGROUND_BLUE,
		Length,
		coPos,
		&Written);

	WriteConsoleOutputCharacterA(
		StdOutput,
		Text,
		Length,
		coPos,
		&Written);
}

VOID
CONSOLE_PrintTextXY(
	IN SHORT x,
	IN SHORT y,
	IN LPCSTR fmt, ...)
{
	CHAR buffer[512];
	va_list ap;
	COORD coPos;
	DWORD Written;

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);

	coPos.X = x;
	coPos.Y = y;

	WriteConsoleOutputCharacterA(
		StdOutput,
		buffer,
		(ULONG)strlen(buffer),
		coPos,
		&Written);
}

VOID
CONSOLE_PrintTextXYN(
	IN SHORT x,
	IN SHORT y,
	IN SHORT len,
	IN LPCSTR fmt, ...)
{
	CHAR buffer[512];
	va_list ap;
	COORD coPos;
	SHORT Length;
	DWORD Written;

	va_start(ap, fmt);
	vsprintf(buffer, fmt, ap);
	va_end(ap);

	coPos.X = x;
	coPos.Y = y;

	Length = (SHORT)strlen(buffer);
	if (Length > len - 1)
		Length = len - 1;

	WriteConsoleOutputCharacterA(
		StdOutput,
		buffer,
		Length,
		coPos,
		&Written);

	coPos.X += Length;

	if (len > Length)
	{
		FillConsoleOutputCharacterA(
			StdOutput,
			' ',
			len - Length,
			coPos,
			&Written);
	}
}

/* EOF */
