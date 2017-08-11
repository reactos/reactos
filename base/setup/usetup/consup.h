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
 * FILE:            base/setup/usetup/consup.h
 * PURPOSE:         Console support functions
 * PROGRAMMER:      Eric Kohl
 */

#pragma once

#define FOREGROUND_WHITE (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)
#define FOREGROUND_YELLOW (FOREGROUND_INTENSITY | FOREGROUND_RED | FOREGROUND_GREEN)
#define BACKGROUND_WHITE (BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE)

/* Text style */
#define TEXT_STYLE_NORMAL      0x00000001
#define TEXT_STYLE_HIGHLIGHT   0x00000002
#define TEXT_STYLE_UNDERLINE   0x00000004

/* Text type */
#define TEXT_TYPE_REGULAR      0x00000008
#define TEXT_TYPE_STATUS       0x00000010

/* Text align */
#define TEXT_ALIGN_DEFAULT     0x00000020
#define TEXT_ALIGN_RIGHT       0x00000040
#define TEXT_ALIGN_LEFT        0x00000080
#define TEXT_ALIGN_CENTER      0x00000100

/* Text padding */
#define TEXT_PADDING_SMALL     0x00000200 /* One space */
#define TEXT_PADDING_MEDIUM    0x00000400 /* Two spaces */
#define TEXT_PADDING_BIG       0x00000800 /* Three spaces */

extern HANDLE StdInput, StdOutput;
extern SHORT xScreen, yScreen;

BOOLEAN
CONSOLE_Init(
	VOID);

VOID
CONSOLE_ClearScreen(VOID);

VOID
CONSOLE_ConInKey(
	OUT PINPUT_RECORD Buffer);

VOID
CONSOLE_ConOutChar(
	IN CHAR c);

VOID
CONSOLE_ConOutPrintf(
	IN LPCSTR szFormat, ...);

VOID
CONSOLE_ConOutPuts(
	IN LPCSTR szText);

BOOL
CONSOLE_Flush(VOID);

VOID
CONSOLE_GetCursorXY(
    PSHORT x,
    PSHORT y);

SHORT
CONSOLE_GetCursorX(VOID);

SHORT
CONSOLE_GetCursorY(VOID);

VOID
CONSOLE_InvertTextXY(
	IN SHORT x,
	IN SHORT y,
	IN SHORT col,
	IN SHORT row);

VOID
CONSOLE_NormalTextXY(
	IN SHORT x,
	IN SHORT y,
	IN SHORT col,
	IN SHORT row);

VOID
CONSOLE_PrintTextXY(
	IN SHORT x,
	IN SHORT y,
	IN LPCSTR fmt, ...);

VOID
CONSOLE_PrintTextXYN(
	IN SHORT x,
	IN SHORT y,
	IN SHORT len,
	IN LPCSTR fmt, ...);

VOID
CONSOLE_SetCursorType(
	IN BOOL bInsert,
	IN BOOL bVisible);

VOID
CONSOLE_SetCursorXY(
	IN SHORT x,
	IN SHORT y);

VOID
CONSOLE_SetCursorXY(
	IN SHORT x,
	IN SHORT y);

VOID
CONSOLE_SetHighlightedTextXY(
	IN SHORT x,
	IN SHORT y,
	IN LPCSTR Text);

VOID
CONSOLE_SetInputTextXY(
	IN SHORT x,
	IN SHORT y,
	IN SHORT len,
	IN LPCWSTR Text);

VOID
CONSOLE_SetInvertedTextXY(
	IN SHORT x,
	IN SHORT y,
	IN LPCSTR Text);

VOID
CONSOLE_SetStatusText(
	IN LPCSTR fmt, ...);

VOID
CONSOLE_SetStatusTextX(
    IN SHORT x,
	IN LPCSTR fmt, ...);

VOID
CONSOLE_SetStatusTextAutoFitX(
    IN SHORT x,
	IN LPCSTR fmt, ...);

VOID
CONSOLE_SetTextXY(
	IN SHORT x,
	IN SHORT y,
	IN LPCSTR Text);

VOID
CONSOLE_SetUnderlinedTextXY(
	IN SHORT x,
	IN SHORT y,
	IN LPCSTR Text);

VOID
CONSOLE_SetStyledText(
	IN SHORT x,
	IN SHORT y,
	IN INT Flags,
	IN LPCSTR Text);

VOID
CONSOLE_ClearStyledText(IN SHORT x,
                        IN SHORT y,
                        IN INT Flags,
                        IN SHORT Length);

/* EOF */
