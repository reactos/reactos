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
 * FILE:            base/setup/usetup/consup.c
 * PURPOSE:         Console support functions
 * PROGRAMMER:
 */

/* INCLUDES ******************************************************************/

#include <usetup.h>

#define NDEBUG
#include <debug.h>

/* GLOBALS ******************************************************************/

HANDLE StdInput  = NULL;
HANDLE StdOutput = NULL;

SHORT xScreen = 0;
SHORT yScreen = 0;

/* FUNCTIONS *****************************************************************/

BOOLEAN
CONSOLE_Init(VOID)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    /* Allocate a new console */
    if (!AllocConsole())
        return FALSE;

    /* Get the standard handles */
    StdInput  = GetStdHandle(STD_INPUT_HANDLE);
    StdOutput = GetStdHandle(STD_OUTPUT_HANDLE);

    /* Retrieve the size of the console */
    if (!GetConsoleScreenBufferInfo(StdOutput, &csbi))
    {
        FreeConsole();
        return FALSE;
    }
    xScreen = csbi.dwSize.X;
    yScreen = csbi.dwSize.Y;

    return TRUE;
}

VOID
CONSOLE_ConInKey(
    OUT PINPUT_RECORD Buffer)
{
    DWORD Read;

    while (TRUE)
    {
        /* Wait for a key press */
        ReadConsoleInput(StdInput, Buffer, 1, &Read);

        if ((Buffer->EventType == KEY_EVENT) &&
            (Buffer->Event.KeyEvent.bKeyDown != FALSE))
        {
            break;
        }
    }
}

BOOLEAN
CONSOLE_ConInKeyPeek(
    OUT PINPUT_RECORD Buffer)
{
    DWORD Read = 0;

    while (TRUE)
    {
        /* Try to get a key press without blocking */
        if (!PeekConsoleInput(StdInput, Buffer, 1, &Read))
            return FALSE;
        if (Read == 0)
            return FALSE;

        /* Consume it */
        ReadConsoleInput(StdInput, Buffer, 1, &Read);

        if ((Buffer->EventType == KEY_EVENT) &&
            (Buffer->Event.KeyEvent.bKeyDown != FALSE))
        {
            return TRUE;
        }
    }
}

VOID
CONSOLE_ConOutChar(
    IN CHAR c)
{
    DWORD Written;

    WriteConsole(StdOutput,
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

    WriteConsole(StdOutput,
                 szText,
                 (ULONG)strlen(szText),
                 &Written,
                 NULL);
    WriteConsole(StdOutput,
                 "\n",
                 1,
                 &Written,
                 NULL);
}

VOID
CONSOLE_ConOutPrintfV(
    IN LPCSTR szFormat,
    IN va_list args)
{
    CHAR szOut[256];
    DWORD dwWritten;

    vsprintf(szOut, szFormat, args);

    WriteConsole(StdOutput,
                 szOut,
                 (ULONG)strlen(szOut),
                 &dwWritten,
                 NULL);
}

VOID
__cdecl
CONSOLE_ConOutPrintf(
    IN LPCSTR szFormat,
    ...)
{
    va_list arg_ptr;

    va_start(arg_ptr, szFormat);
    CONSOLE_ConOutPrintfV(szFormat, arg_ptr);
    va_end(arg_ptr);
}

BOOL
CONSOLE_Flush(VOID)
{
    return FlushConsoleInputBuffer(StdInput);
}

VOID
CONSOLE_GetCursorXY(
    OUT PSHORT x,
    OUT PSHORT y)
{
    CONSOLE_SCREEN_BUFFER_INFO csbi;

    GetConsoleScreenBufferInfo(StdOutput, &csbi);

    *x = csbi.dwCursorPosition.X;
    *y = csbi.dwCursorPosition.Y;
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

    /*
     * Hide everything under the same foreground & background colors, so that
     * the actual color and text blanking reset does not create a visual "blinking".
     * We do this because we cannot do the screen scrolling trick that would
     * allow to change both the text and the colors at the same time (the
     * function is currently not available in our console "emulation" layer).
     */
    FillConsoleOutputAttribute(StdOutput,
                               FOREGROUND_BLUE | BACKGROUND_BLUE,
                               xScreen * yScreen,
                               coPos,
                               &Written);

    /* Blank the text */
    FillConsoleOutputCharacterA(StdOutput,
                                ' ',
                                xScreen * yScreen,
                                coPos,
                                &Written);

    /* Reset the actual foreground & background colors */
    FillConsoleOutputAttribute(StdOutput,
                               FOREGROUND_WHITE | BACKGROUND_BLUE,
                               xScreen * yScreen,
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

        FillConsoleOutputAttribute(StdOutput,
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

        FillConsoleOutputAttribute(StdOutput,
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

    WriteConsoleOutputCharacterA(StdOutput,
                                 Text,
                                 (ULONG)strlen(Text),
                                 coPos,
                                 &Written);
}

VOID
CONSOLE_ClearTextXY(IN SHORT x,
                    IN SHORT y,
                    IN SHORT Length)
{
    COORD coPos;
    DWORD Written;

    coPos.X = x;
    coPos.Y = y;

    FillConsoleOutputCharacterA(StdOutput,
                                ' ',
                                Length,
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

    FillConsoleOutputAttribute(StdOutput,
                               BACKGROUND_WHITE,
                               len,
                               coPos,
                               &Written);

    WriteConsoleOutputCharacterW(StdOutput,
                                 Text,
                                 (ULONG)Length,
                                 coPos,
                                 &Written);

    coPos.X += Length;
    if (len > Length)
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    ' ',
                                    len - Length,
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

    WriteConsoleOutputCharacterA(StdOutput,
                                 Text,
                                 Length,
                                 coPos,
                                 &Written);

    coPos.Y++;
    FillConsoleOutputCharacterA(StdOutput,
                                CharDoubleHorizontalLine,
                                Length,
                                coPos,
                                &Written);
}

VOID
CONSOLE_SetStatusTextXV(
    IN SHORT x,
    IN LPCSTR fmt,
    IN va_list args)
{
    INT nLength;
    COORD coPos;
    DWORD Written;
    CHAR Buffer[128];

    memset(Buffer, ' ', min(sizeof(Buffer), xScreen));
    nLength = vsprintf(&Buffer[x], fmt, args);
    ASSERT(x + nLength < sizeof(Buffer));
    Buffer[x + nLength] = ' ';

    coPos.X = 0;
    coPos.Y = yScreen - 1;
    FillConsoleOutputAttribute(StdOutput,
                               BACKGROUND_WHITE,
                               xScreen,
                               coPos,
                               &Written);
    WriteConsoleOutputCharacterA(StdOutput,
                                 Buffer,
                                 min(sizeof(Buffer), xScreen),
                                 coPos,
                                 &Written);
}

VOID
__cdecl
CONSOLE_SetStatusTextX(
    IN SHORT x,
    IN LPCSTR fmt,
    ...)
{
    va_list ap;

    va_start(ap, fmt);
    CONSOLE_SetStatusTextXV(x, fmt, ap);
    va_end(ap);
}

VOID
CONSOLE_SetStatusTextV(
    IN LPCSTR fmt,
    IN va_list args)
{
    CONSOLE_SetStatusTextXV(0, fmt, args);
}

VOID
__cdecl
CONSOLE_SetStatusText(
    IN LPCSTR fmt,
    ...)
{
    va_list ap;

    va_start(ap, fmt);
    CONSOLE_SetStatusTextV(fmt, ap);
    va_end(ap);
}

static
VOID
CONSOLE_ClearStatusTextX(
    IN SHORT x,
    IN SHORT Length)
{
    COORD coPos;
    DWORD Written;

    coPos.X = x;
    coPos.Y = yScreen - 1;

    FillConsoleOutputCharacterA(StdOutput,
                                ' ',
                                Length,
                                coPos,
                                &Written);
}

VOID
__cdecl
CONSOLE_SetStatusTextAutoFitX(
    IN SHORT x,
    IN LPCSTR fmt,
    ...)
{
    CHAR Buffer[128];
    DWORD Length;
    va_list ap;

    va_start(ap, fmt);
    vsprintf(Buffer, fmt, ap);
    va_end(ap);

    Length = (ULONG)strlen(Buffer);

    if (Length + x <= 79)
    {
        CONSOLE_SetStatusTextX(x , Buffer);
    }
    else
    {
        CONSOLE_SetStatusTextX(79 - Length , Buffer);
    }
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
__cdecl
CONSOLE_PrintTextXY(
    IN SHORT x,
    IN SHORT y,
    IN LPCSTR fmt,
    ...)
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

    WriteConsoleOutputCharacterA(StdOutput,
                                 buffer,
                                 (ULONG)strlen(buffer),
                                 coPos,
                                 &Written);
}

VOID
__cdecl
CONSOLE_PrintTextXYN(
    IN SHORT x,
    IN SHORT y,
    IN SHORT len,
    IN LPCSTR fmt,
    ...)
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

    WriteConsoleOutputCharacterA(StdOutput,
                                 buffer,
                                 Length,
                                 coPos,
                                 &Written);

    coPos.X += Length;

    if (len > Length)
    {
        FillConsoleOutputCharacterA(StdOutput,
                                    ' ',
                                    len - Length,
                                    coPos,
                                    &Written);
    }
}

VOID
CONSOLE_SetStyledText(
    IN SHORT x,
    IN SHORT y,
    IN INT Flags,
    IN LPCSTR Text)
{
    COORD coPos;

    coPos.X = x;
    coPos.Y = y;

    if (Flags & TEXT_TYPE_STATUS)
    {
        coPos.X = x;
        coPos.Y = yScreen - 1;
    }
    else /* TEXT_TYPE_REGULAR (Default) */
    {
        coPos.X = x;
        coPos.Y = y;
    }

    if (Flags & TEXT_ALIGN_CENTER)
    {
        coPos.X = (xScreen - (SHORT)strlen(Text)) / 2;
    }
    else if(Flags & TEXT_ALIGN_RIGHT)
    {
        coPos.X = coPos.X - (SHORT)strlen(Text);

        if (Flags & TEXT_PADDING_SMALL)
        {
            coPos.X -= 1;
        }
        else if (Flags & TEXT_PADDING_MEDIUM)
        {
            coPos.X -= 2;
        }
        else if (Flags & TEXT_PADDING_BIG)
        {
            coPos.X -= 3;
        }
    }
    else /* TEXT_ALIGN_LEFT (Default) */
    {
        if (Flags & TEXT_PADDING_SMALL)
        {
            coPos.X += 1;
        }
        else if (Flags & TEXT_PADDING_MEDIUM)
        {
            coPos.X += 2;
        }
        else if (Flags & TEXT_PADDING_BIG)
        {
            coPos.X += 3;
        }
    }

    if (Flags & TEXT_TYPE_STATUS)
    {
        CONSOLE_SetStatusTextX(coPos.X, Text);
    }
    else /* TEXT_TYPE_REGULAR (Default) */
    {
        if (Flags & TEXT_STYLE_HIGHLIGHT)
        {
            CONSOLE_SetHighlightedTextXY(coPos.X, coPos.Y, Text);
        }
        else if (Flags & TEXT_STYLE_UNDERLINE)
        {
            CONSOLE_SetUnderlinedTextXY(coPos.X, coPos.Y, Text);
        }
        else /* TEXT_STYLE_NORMAL (Default) */
        {
            CONSOLE_SetTextXY(coPos.X, coPos.Y, Text);
        }
    }
}

VOID
CONSOLE_ClearStyledText(
    IN SHORT x,
    IN SHORT y,
    IN INT Flags,
    IN SHORT Length)
{
    COORD coPos;

    coPos.X = x;
    coPos.Y = y;

    if (Flags & TEXT_TYPE_STATUS)
    {
        coPos.X = x;
        coPos.Y = yScreen - 1;
    }
    else /* TEXT_TYPE_REGULAR (Default) */
    {
        coPos.X = x;
        coPos.Y = y;
    }

    if (Flags & TEXT_ALIGN_CENTER)
    {
        coPos.X = (xScreen - Length) / 2;
    }
    else if(Flags & TEXT_ALIGN_RIGHT)
    {
        coPos.X = coPos.X - Length;

        if (Flags & TEXT_PADDING_SMALL)
        {
            coPos.X -= 1;
        }
        else if (Flags & TEXT_PADDING_MEDIUM)
        {
            coPos.X -= 2;
        }
        else if (Flags & TEXT_PADDING_BIG)
        {
            coPos.X -= 3;
        }
    }
    else /* TEXT_ALIGN_LEFT (Default) */
    {
        if (Flags & TEXT_PADDING_SMALL)
        {
            coPos.X += 1;
        }
        else if (Flags & TEXT_PADDING_MEDIUM)
        {
            coPos.X += 2;
        }
        else if (Flags & TEXT_PADDING_BIG)
        {
            coPos.X += 3;
        }
    }

    if (Flags & TEXT_TYPE_STATUS)
    {
        CONSOLE_ClearStatusTextX(coPos.X, Length);
    }
    else if (Flags & TEXT_STYLE_UNDERLINE)
    {
        CONSOLE_ClearTextXY(coPos.X, coPos.Y, Length);
        CONSOLE_ClearTextXY(coPos.X, coPos.Y + 1, Length);
    }
    else /* TEXT_TYPE_REGULAR (Default) */
    {
        CONSOLE_ClearTextXY(coPos.X, coPos.Y, Length);
    }
}

/* EOF */
