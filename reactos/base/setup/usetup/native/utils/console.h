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
 *  You should have received a copy of the GNU General Public License along
 *  with this program; if not, write to the Free Software Foundation, Inc.,
 *  51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */
/*
 * COPYRIGHT:       See COPYING in the top level directory
 * PROJECT:         ReactOS text-mode setup
 * FILE:            base/setup/usetup/native/utils/console.h
 * PURPOSE:         Console support functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

BOOL
WINAPI
AllocConsole(VOID);

BOOL
WINAPI
AttachConsole(
    IN DWORD dwProcessId);

BOOL
WINAPI
FillConsoleOutputAttribute(
    IN HANDLE hConsoleOutput,
    IN WORD wAttribute,
    IN DWORD nLength,
    IN COORD dwWriteCoord,
    OUT LPDWORD lpNumberOfAttrsWritten);

BOOL
WINAPI
FillConsoleOutputCharacterA(
    IN HANDLE hConsoleOutput,
    IN CHAR cCharacter,
    IN DWORD nLength,
    IN COORD dwWriteCoord,
    OUT LPDWORD lpNumberOfCharsWritten);

BOOL
WINAPI
FreeConsole(VOID);

BOOL
WINAPI
GetConsoleScreenBufferInfo(
    IN HANDLE hConsoleOutput,
    OUT PCONSOLE_SCREEN_BUFFER_INFO lpConsoleScreenBufferInfo);

HANDLE
WINAPI
GetStdHandle(
    IN DWORD nStdHandle);

BOOL
WINAPI
ReadConsoleInput(
    IN HANDLE hConsoleInput,
    OUT PINPUT_RECORD lpBuffer,
    IN DWORD nLength,
    OUT LPDWORD lpNumberOfEventsRead);

BOOL
WINAPI
SetConsoleCursorInfo(
    IN HANDLE hConsoleOutput,
    IN const CONSOLE_CURSOR_INFO *lpConsoleCursorInfo);

BOOL
WINAPI
SetConsoleCursorPosition(
    IN HANDLE hConsoleOutput,
    IN COORD dwCursorPosition);

BOOL
WINAPI
SetConsoleTextAttribute(
    IN HANDLE hConsoleOutput,
    IN WORD wAttributes);

BOOL
WINAPI
WriteConsole(
    IN HANDLE hConsoleOutput,
    IN const VOID *lpBuffer,
    IN DWORD nNumberOfCharsToWrite,
    OUT LPDWORD lpNumberOfCharsWritten,
    IN LPVOID lpReserved);

BOOL
WINAPI
WriteConsoleOutputCharacterA(
    HANDLE hConsoleOutput,
    IN LPCSTR lpCharacter,
    IN DWORD nLength,
    IN COORD dwWriteCoord,
    OUT LPDWORD lpNumberOfCharsWritten);

BOOL
WINAPI
WriteConsoleOutputCharacterW(
    HANDLE hConsoleOutput,
    IN LPCSTR lpCharacter,
    IN DWORD nLength,
    IN COORD dwWriteCoord,
    OUT LPDWORD lpNumberOfCharsWritten);

BOOL
WINAPI
SetConsoleOutputCP(
    IN UINT wCodePageID
);

/* EOF */
