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
 * FILE:            subsys/system/usetup/console.h
 * PURPOSE:         Console support functions
 * PROGRAMMER:      Eric Kohl
 */

#ifndef __CONSOLE_H__
#define __CONSOLE_H__

#undef WriteConsole
#undef ReadConsoleInput
#undef FillConsoleOutputCharacter

#define AllocConsole ConAllocConsole
#define FillConsoleOutputAttribute ConFillConsoleOutputAttribute
#define FillConsoleOutputCharacterA ConFillConsoleOutputCharacterA
#define FreeConsole ConFreeConsole
#define GetConsoleScreenBufferInfo ConGetConsoleScreenBufferInfo
#define ReadConsoleInput ConReadConsoleInput
#define SetConsoleCursorInfo ConSetConsoleCursorInfo
#define SetConsoleCursorPosition ConSetConsoleCursorPosition
#define SetConsoleTextAttribute ConSetConsoleTextAttribute
#define WriteConsole ConWriteConsole
#define WriteConsoleOutputCharacterA ConWriteConsoleOutputCharacterA
#define WriteConsoleOutputCharacterW ConWriteConsoleOutputCharacterW

#define FOREGROUND_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define FOREGROUND_YELLOW (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN)
#define BACKGROUND_WHITE (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)

BOOL WINAPI
ConAllocConsole(
	IN DWORD dwProcessId);

BOOL WINAPI
ConFillConsoleOutputAttribute(
	IN HANDLE hConsoleOutput,
	IN WORD wAttribute,
	IN DWORD nLength,
	IN COORD dwWriteCoord,
	OUT LPDWORD lpNumberOfAttrsWritten);

BOOL WINAPI
ConFillConsoleOutputCharacterA(
	IN HANDLE hConsoleOutput,
	IN CHAR cCharacter,
	IN DWORD nLength,
	IN COORD dwWriteCoord,
	OUT LPDWORD lpNumberOfCharsWritten);

BOOL WINAPI
ConFreeConsole(VOID);

BOOL WINAPI
ConGetConsoleScreenBufferInfo(
	IN HANDLE hConsoleOutput,
	OUT PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo);

BOOL WINAPI
ConReadConsoleInput(
	IN HANDLE hConsoleInput,
	OUT PINPUT_RECORD lpBuffer,
	IN DWORD nLength,
	OUT LPDWORD lpNumberOfEventsRead);

BOOL WINAPI
ConSetConsoleCursorInfo(
	IN HANDLE hConsoleOutput,
	IN const CONSOLE_CURSOR_INFO* lpConsoleCursorInfo);

BOOL WINAPI
ConSetConsoleCursorPosition(
	IN HANDLE hConsoleOutput,
	IN COORD dwCursorPosition);

BOOL WINAPI
ConSetConsoleTextAttribute(
	IN HANDLE hConsoleOutput,
	IN WORD wAttributes);

BOOL WINAPI
ConWriteConsole(
	IN HANDLE hConsoleOutput,
	IN const VOID* lpBuffer,
	IN DWORD nNumberOfCharsToWrite,
	OUT LPDWORD lpNumberOfCharsWritten,
	IN LPVOID lpReserved);

BOOL WINAPI
ConWriteConsoleOutputCharacterA(
	HANDLE hConsoleOutput,
	IN LPCSTR lpCharacter,
	IN DWORD nLength,
	IN COORD dwWriteCoord,
	OUT LPDWORD lpNumberOfCharsWritten);

BOOL WINAPI
ConWriteConsoleOutputCharacterA(
	HANDLE hConsoleOutput,
	IN LPCSTR lpCharacter,
	IN DWORD nLength,
	IN COORD dwWriteCoord,
	OUT LPDWORD lpNumberOfCharsWritten);



VOID
CONSOLE_ConInKey(PINPUT_RECORD Buffer);

VOID
CONSOLE_ConOutChar(CHAR c);

VOID
CONSOLE_ConOutPuts(LPSTR szText);

VOID
CONSOLE_ConOutPrintf(LPSTR szFormat, ...);

SHORT
CONSOLE_GetCursorX(VOID);

SHORT
CONSOLE_GetCursorY(VOID);

VOID
CONSOLE_GetScreenSize(SHORT *maxx,
	      SHORT *maxy);

VOID
CONSOLE_SetCursorType(BOOL bInsert,
	      BOOL bVisible);

VOID
CONSOLE_SetCursorXY(SHORT x,
	    SHORT y);

VOID
CONSOLE_ClearScreen(VOID);

VOID
CONSOLE_SetStatusText(char* fmt, ...);

VOID
CONSOLE_InvertTextXY(SHORT x, SHORT y, SHORT col, SHORT row);

VOID
CONSOLE_NormalTextXY(SHORT x, SHORT y, SHORT col, SHORT row);

VOID
CONSOLE_SetTextXY(SHORT x, SHORT y, PCHAR Text);

VOID
CONSOLE_SetInputTextXY(SHORT x, SHORT y, SHORT len, PWCHAR Text);

VOID
CONSOLE_SetUnderlinedTextXY(SHORT x, SHORT y, PCHAR Text);

VOID
CONSOLE_SetInvertedTextXY(SHORT x, SHORT y, PCHAR Text);

VOID
CONSOLE_SetHighlightedTextXY(SHORT x, SHORT y, PCHAR Text);

VOID
CONSOLE_PrintTextXY(SHORT x, SHORT y, char* fmt, ...);

VOID
CONSOLE_PrintTextXYN(SHORT x, SHORT y, SHORT len, char* fmt, ...);

#endif /* __CONSOLE_H__*/

/* EOF */
