/*
 * notevil.c
 *
 * --------------------------------------------------------------------
 *
 * This software is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This software is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this software; see the file COPYING.LIB. If
 * not, write to the Free Software Foundation, Inc., 675 Mass Ave,
 * Cambridge, MA 02139, USA.
 *
 * --------------------------------------------------------------------
 * ReactOS Coders Console Parade
 *
 * 19990411 EA
 * 19990515 EA
 */

#include <windows.h>
#include <stdio.h>
#include "resource.h"

// #define DISPLAY_COORD

LPCWSTR app_name = L"notevil";

HANDLE  myself;
HANDLE  ScreenBuffer;
CONSOLE_SCREEN_BUFFER_INFO ScreenBufferInfo;
HANDLE  WaitableTimer;

VOID
WriteStringAt(LPWSTR lpString,
              COORD  xy,
              WORD   wColor)
{
    DWORD cWritten = 0;
    DWORD dwLen;

    if (!lpString || *lpString == 0) return;

    dwLen = (DWORD)wcslen(lpString);

    /* Don't bother writing text when erasing */
    if (wColor)
    {
        WriteConsoleOutputCharacterW(ScreenBuffer,
                                     lpString,
                                     dwLen,
                                     xy,
                                     &cWritten);
    }

    FillConsoleOutputAttribute(ScreenBuffer,
                               wColor,
                               dwLen,
                               xy,
                               &cWritten);
}


#ifdef DISPLAY_COORD
VOID
WriteCoord(COORD c)
{
    COORD xy = {0,0};
    WCHAR buf[40];

    wsprintf(buf, L"x=%02d  y=%02d", c.X, c.Y);

    WriteStringAt(buf, xy,
                  BACKGROUND_RED | BACKGROUND_GREEN | BACKGROUND_BLUE);
}
#endif /* def DISPLAY_COORD */


VOID
GetNextString(LPWSTR Buffer,
              INT    BufferSize,
              PDWORD Index)
{
    if (RES_LAST_INDEX == *Index)
        *Index = RES_FIRST_INDEX;
    else
        ++*Index;

    LoadStringW(myself, *Index, Buffer, BufferSize);
}


VOID
DisplayTitle(VOID)
{
    LPWSTR szTitle = L"ReactOS Coders Console Parade";
    COORD  xy;

    xy.X = (ScreenBufferInfo.dwSize.X - (USHORT)wcslen(szTitle)) / 2;
    xy.Y = ScreenBufferInfo.dwSize.Y / 2;

    WriteStringAt(szTitle, xy,
                  FOREGROUND_GREEN | FOREGROUND_INTENSITY);
}


#define RES_DELAY_CHANGE 12
#define RES_BUFFER_SIZE  1024
VOID
MainLoop(VOID)
{
    WCHAR NameString[RES_BUFFER_SIZE];
    DWORD NameIndex  = 0;
    INT   NameLength = 0;
    COORD xy;
    INT   n = RES_DELAY_CHANGE;
    INT   dir_y  = 1;
    INT   dir_x  = 1;
    WORD  wColor = 1;

    xy.X = ScreenBufferInfo.dwSize.X / 2;
    xy.Y = ScreenBufferInfo.dwSize.Y / 2;

    for ( ; 1; ++n )
    {
        if (n == RES_DELAY_CHANGE)
        {
            n = 0;

            GetNextString(NameString,
                          RES_BUFFER_SIZE,
                          &NameIndex);
            NameLength = wcslen(NameString);

            wColor++;
            if ((wColor & 0x000F) == 0)
                wColor = 1;
        }
        if (xy.X == 0)
        {
            if (dir_x == -1)
                dir_x = 1;
        }
        else if (xy.X >= ScreenBufferInfo.dwSize.X - NameLength - 1)
        {
            if (dir_x == 1)
                dir_x = -1;
        }
        xy.X += dir_x;

        if (xy.Y == 0)
        {
            if (dir_y == -1)
                dir_y = 1;
        }
        else if (xy.Y >= ScreenBufferInfo.dwSize.Y - 1)
        {
            if (dir_y == 1)
                dir_y = -1;
        }
        xy.Y += dir_y;

#ifdef DISPLAY_COORD
        WriteCoord(xy);
#endif /* def DISPLAY_COORD */

        DisplayTitle();
        WriteStringAt(NameString, xy, wColor);
        WaitForSingleObject(WaitableTimer, INFINITE);
        WriteStringAt(NameString, xy, 0);
    }
}


int wmain(int argc, WCHAR* argv[])
{
    LARGE_INTEGER lint;
    DWORD Written;
    COORD Coord = { 0, 0 };

    myself = GetModuleHandle(NULL);

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE),
                               &ScreenBufferInfo);
    ScreenBufferInfo.dwSize.X = ScreenBufferInfo.srWindow.Right - ScreenBufferInfo.srWindow.Left + 1;
    ScreenBufferInfo.dwSize.Y = ScreenBufferInfo.srWindow.Bottom - ScreenBufferInfo.srWindow.Top + 1;
    ScreenBuffer = CreateConsoleScreenBuffer(GENERIC_WRITE,
                                             0,
                                             NULL,
                                             CONSOLE_TEXTMODE_BUFFER,
                                             NULL);
    if (ScreenBuffer == INVALID_HANDLE_VALUE)
    {
        wprintf(L"%s: could not create a new screen buffer\n", app_name);
        return EXIT_FAILURE;
    }

    /* Fill buffer with black background */
    FillConsoleOutputAttribute(ScreenBuffer,
                               0,
                               ScreenBufferInfo.dwSize.X * ScreenBufferInfo.dwSize.Y,
                               Coord,
                               &Written);

    WaitableTimer = CreateWaitableTimer(NULL, FALSE, NULL);
    if (WaitableTimer == INVALID_HANDLE_VALUE)
    {
        wprintf(L"CreateWaitabletimer() failed\n");
        return 1;
    }
    lint.QuadPart = -2000000;
    if (!SetWaitableTimer(WaitableTimer, &lint, 200, NULL, NULL, FALSE))
    {
        wprintf(L"SetWaitableTimer() failed: 0x%lx\n", GetLastError());
        return 2;
    }
    SetConsoleActiveScreenBuffer(ScreenBuffer);
    MainLoop();
    CloseHandle(ScreenBuffer);
    return EXIT_SUCCESS;
}

/* EOF */
