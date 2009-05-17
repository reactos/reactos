/*
 * Unit tests for console API
 *
 * Copyright (c) 2003,2004 Eric Pouech
 * Copyright (c) 2007 Kirill K. Smirnov
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
 */

#include "wine/test.h"
#include <windows.h>
#include <stdio.h>

static BOOL (WINAPI *pGetConsoleInputExeNameA)(DWORD, LPSTR);
static BOOL (WINAPI *pSetConsoleInputExeNameA)(LPCSTR);

/* DEFAULT_ATTRIB is used for all initial filling of the console.
 * all modifications are made with TEST_ATTRIB so that we could check
 * what has to be modified or not
 */
#define TEST_ATTRIB    (BACKGROUND_BLUE | FOREGROUND_GREEN)
#define DEFAULT_ATTRIB (FOREGROUND_GREEN | FOREGROUND_BLUE | FOREGROUND_RED)
/* when filling the screen with non-blank chars, this macro defines
 * what character should be at position 'c'
 */
#define CONTENT(c)    ('A' + (((c).Y * 17 + (c).X) % 23))

#define	okCURSOR(hCon, c) do { \
  CONSOLE_SCREEN_BUFFER_INFO __sbi; \
  BOOL expect = GetConsoleScreenBufferInfo((hCon), &__sbi) && \
                __sbi.dwCursorPosition.X == (c).X && __sbi.dwCursorPosition.Y == (c).Y; \
  ok(expect, "Expected cursor at (%d,%d), got (%d,%d)\n", \
     (c).X, (c).Y, __sbi.dwCursorPosition.X, __sbi.dwCursorPosition.Y); \
} while (0)

#define okCHAR(hCon, c, ch, attr) do { \
  char __ch; WORD __attr; DWORD __len; BOOL expect; \
  expect = ReadConsoleOutputCharacter((hCon), &__ch, 1, (c), &__len) == 1 && __len == 1 && __ch == (ch); \
  ok(expect, "At (%d,%d): expecting char '%c'/%02x got '%c'/%02x\n", (c).X, (c).Y, (ch), (ch), __ch, __ch); \
  expect = ReadConsoleOutputAttribute((hCon), &__attr, 1, (c), &__len) == 1 && __len == 1 && __attr == (attr); \
  ok(expect, "At (%d,%d): expecting attr %04x got %04x\n", (c).X, (c).Y, (attr), __attr); \
} while (0)

static void init_function_pointers(void)
{
    HMODULE hKernel32;

#define KERNEL32_GET_PROC(func)                                     \
    p##func = (void *)GetProcAddress(hKernel32, #func);             \
    if(!p##func) trace("GetProcAddress(hKernel32, '%s') failed\n", #func);

    hKernel32 = GetModuleHandleA("kernel32.dll");
    KERNEL32_GET_PROC(GetConsoleInputExeNameA);
    KERNEL32_GET_PROC(SetConsoleInputExeNameA);

#undef KERNEL32_GET_PROC
}

/* FIXME: this could be optimized on a speed point of view */
static void resetContent(HANDLE hCon, COORD sbSize, BOOL content)
{
    COORD       c;
    WORD        attr = DEFAULT_ATTRIB;
    char        ch;
    DWORD       len;

    for (c.X = 0; c.X < sbSize.X; c.X++)
    {
        for (c.Y = 0; c.Y < sbSize.Y; c.Y++)
        {
            ch = (content) ? CONTENT(c) : ' ';
            WriteConsoleOutputAttribute(hCon, &attr, 1, c, &len);
            WriteConsoleOutputCharacterA(hCon, &ch, 1, c, &len);
        }
    }
}

static void testCursor(HANDLE hCon, COORD sbSize)
{
    COORD		c;

    c.X = c.Y = 0;
    ok(SetConsoleCursorPosition(0, c) == 0, "No handle\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_HANDLE, GetLastError());

    c.X = c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left\n");
    okCURSOR(hCon, c);

    c.X = sbSize.X - 1;
    c.Y = sbSize.Y - 1;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in lower-right\n");
    okCURSOR(hCon, c);

    c.X = sbSize.X;
    c.Y = sbSize.Y - 1;
    ok(SetConsoleCursorPosition(hCon, c) == 0, "Cursor is outside\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_PARAMETER, GetLastError());

    c.X = sbSize.X - 1;
    c.Y = sbSize.Y;
    ok(SetConsoleCursorPosition(hCon, c) == 0, "Cursor is outside\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_PARAMETER, GetLastError());

    c.X = -1;
    c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) == 0, "Cursor is outside\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_PARAMETER, GetLastError());

    c.X = 0;
    c.Y = -1;
    ok(SetConsoleCursorPosition(hCon, c) == 0, "Cursor is outside\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_PARAMETER, GetLastError());
}

static void testCursorInfo(HANDLE hCon)
{
    BOOL ret;
    CONSOLE_CURSOR_INFO info;

    SetLastError(0xdeadbeef);
    ret = GetConsoleCursorInfo(NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_HANDLE, GetLastError());

    SetLastError(0xdeadbeef);
    info.dwSize = -1;
    ret = GetConsoleCursorInfo(NULL, &info);
    ok(!ret, "Expected failure\n");
    ok(info.dwSize == -1, "Expected no change for dwSize\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_HANDLE, GetLastError());

    /* Test the correct call first to distinguish between win9x and the rest */
    SetLastError(0xdeadbeef);
    ret = GetConsoleCursorInfo(hCon, &info);
    ok(ret, "Expected success\n");
    ok(info.dwSize == 25 ||
       info.dwSize == 12 /* win9x */,
       "Expected 12 or 25, got %d\n", info.dwSize);
    ok(info.bVisible, "Expected the cursor to be visible\n");
    ok(GetLastError() == 0xdeadbeef, "GetLastError: expecting %u got %u\n",
       0xdeadbeef, GetLastError());

    if (info.dwSize == 12)
    {
        win_skip("NULL CONSOLE_CURSOR_INFO will crash on win9x\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = GetConsoleCursorInfo(hCon, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_ACCESS, "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_ACCESS, GetLastError());
}

static void testEmptyWrite(HANDLE hCon)
{
    COORD		c;
    DWORD		len;
    const char* 	mytest = "";

    c.X = c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left\n");

    len = -1;
    ok(WriteConsole(hCon, NULL, 0, &len, NULL) != 0 && len == 0, "WriteConsole\n");
    okCURSOR(hCon, c);

    /* Passing a NULL lpBuffer with sufficiently large non-zero length succeeds
     * on native Windows and result in memory-like contents being written to
     * the console. Calling WriteConsoleW like this will crash on Wine. */
    if (0)
    {
        len = -1;
        ok(!WriteConsole(hCon, NULL, 16, &len, NULL) && len == -1, "WriteConsole\n");
        okCURSOR(hCon, c);

        /* Cursor advances for this call. */
        len = -1;
        ok(WriteConsole(hCon, NULL, 128, &len, NULL) != 0 && len == 128, "WriteConsole\n");
    }

    len = -1;
    ok(WriteConsole(hCon, mytest, 0, &len, NULL) != 0 && len == 0, "WriteConsole\n");
    okCURSOR(hCon, c);

    /* WriteConsole does not halt on a null terminator and is happy to write
     * memory contents beyond the actual size of the buffer. */
    len = -1;
    ok(WriteConsole(hCon, mytest, 16, &len, NULL) != 0 && len == 16, "WriteConsole\n");
    c.X += 16;
    okCURSOR(hCon, c);
}

static void testWriteSimple(HANDLE hCon)
{
    COORD		c;
    DWORD		len;
    const char*		mytest = "abcdefg";
    const int	mylen = strlen(mytest);

    /* single line write */
    c.X = c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left\n");

    ok(WriteConsole(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
    c.Y = 0;
    for (c.X = 0; c.X < mylen; c.X++)
    {
        okCHAR(hCon, c, mytest[c.X], TEST_ATTRIB);
    }

    okCURSOR(hCon, c);
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);
}

static void testWriteNotWrappedNotProcessed(HANDLE hCon, COORD sbSize)
{
    COORD		c;
    DWORD		len, mode;
    const char*         mytest = "123";
    const int           mylen = strlen(mytest);
    int                 ret;
    int			p;

    ok(GetConsoleMode(hCon, &mode) && SetConsoleMode(hCon, mode & ~(ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT)),
       "clearing wrap at EOL & processed output\n");

    /* write line, wrapping disabled, buffer exceeds sb width */
    c.X = sbSize.X - 3; c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-3\n");

    ret = WriteConsole(hCon, mytest, mylen, &len, NULL);
    ok(ret != 0 && len == mylen, "Couldn't write, ret = %d, len = %d\n", ret, len);
    c.Y = 0;
    for (p = mylen - 3; p < mylen; p++)
    {
        c.X = sbSize.X - 3 + p % 3;
        okCHAR(hCon, c, mytest[p], TEST_ATTRIB);
    }

    c.X = 0; c.Y = 1;
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);

    p = sbSize.X - 3 + mylen % 3;
    c.X = p; c.Y = 0;

    /* write line, wrapping disabled, strings end on end of line */
    c.X = sbSize.X - mylen; c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-3\n");

    ok(WriteConsole(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
}

static void testWriteNotWrappedProcessed(HANDLE hCon, COORD sbSize)
{
    COORD		c;
    DWORD		len, mode;
    const char*		mytest = "abcd\nf\tg";
    const int	mylen = strlen(mytest);
    const int	mylen2 = strchr(mytest, '\n') - mytest;
    int			p;

    ok(GetConsoleMode(hCon, &mode) && SetConsoleMode(hCon, (mode | ENABLE_PROCESSED_OUTPUT) & ~ENABLE_WRAP_AT_EOL_OUTPUT),
       "clearing wrap at EOL & setting processed output\n");

    /* write line, wrapping disabled, buffer exceeds sb width */
    c.X = sbSize.X - 5; c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-5\n");

    ok(WriteConsole(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
    c.Y = 0;
    for (c.X = sbSize.X - 5; c.X < sbSize.X - 1; c.X++)
    {
        okCHAR(hCon, c, mytest[c.X - sbSize.X + 5], TEST_ATTRIB);
    }
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);

    c.X = 0; c.Y++;
    okCHAR(hCon, c, mytest[5], TEST_ATTRIB);
    for (c.X = 1; c.X < 8; c.X++)
        okCHAR(hCon, c, ' ', TEST_ATTRIB);
    okCHAR(hCon, c, mytest[7], TEST_ATTRIB);
    c.X++;
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);

    okCURSOR(hCon, c);

    /* write line, wrapping disabled, strings end on end of line */
    c.X = sbSize.X - 4; c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-4\n");

    ok(WriteConsole(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
    c.Y = 0;
    for (c.X = sbSize.X - 4; c.X < sbSize.X; c.X++)
    {
        okCHAR(hCon, c, mytest[c.X - sbSize.X + 4], TEST_ATTRIB);
    }
    c.X = 0; c.Y++;
    okCHAR(hCon, c, mytest[5], TEST_ATTRIB);
    for (c.X = 1; c.X < 8; c.X++)
        okCHAR(hCon, c, ' ', TEST_ATTRIB);
    okCHAR(hCon, c, mytest[7], TEST_ATTRIB);
    c.X++;
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);

    okCURSOR(hCon, c);

    /* write line, wrapping disabled, strings end after end of line */
    c.X = sbSize.X - 3; c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-4\n");

    ok(WriteConsole(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
    c.Y = 0;
    for (p = mylen2 - 3; p < mylen2; p++)
    {
        c.X = sbSize.X - 3 + p % 3;
        okCHAR(hCon, c, mytest[p], TEST_ATTRIB);
    }
    c.X = 0; c.Y = 1;
    okCHAR(hCon, c, mytest[5], TEST_ATTRIB);
    for (c.X = 1; c.X < 8; c.X++)
        okCHAR(hCon, c, ' ', TEST_ATTRIB);
    okCHAR(hCon, c, mytest[7], TEST_ATTRIB);
    c.X++;
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);

    okCURSOR(hCon, c);
}

static void testWriteWrappedNotProcessed(HANDLE hCon, COORD sbSize)
{
    COORD		c;
    DWORD		len, mode;
    const char*		mytest = "abcd\nf\tg";
    const int	mylen = strlen(mytest);
    int			p;

    ok(GetConsoleMode(hCon, &mode) && SetConsoleMode(hCon,(mode | ENABLE_WRAP_AT_EOL_OUTPUT) & ~(ENABLE_PROCESSED_OUTPUT)),
       "setting wrap at EOL & clearing processed output\n");

    /* write line, wrapping enabled, buffer doesn't exceed sb width */
    c.X = sbSize.X - 9; c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-9\n");

    ok(WriteConsole(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
    c.Y = 0;
    for (p = 0; p < mylen; p++)
    {
        c.X = sbSize.X - 9 + p;
        okCHAR(hCon, c, mytest[p], TEST_ATTRIB);
    }
    c.X = sbSize.X - 9 + mylen;
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);
    c.X = 0; c.Y = 1;
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);

    /* write line, wrapping enabled, buffer does exceed sb width */
    c.X = sbSize.X - 3; c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-3\n");

    c.Y = 1;
    c.X = mylen - 3;
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);
}

static void testWriteWrappedProcessed(HANDLE hCon, COORD sbSize)
{
    COORD		c;
    DWORD		len, mode;
    const char*		mytest = "abcd\nf\tg";
    const int	mylen = strlen(mytest);
    int			p;

    ok(GetConsoleMode(hCon, &mode) && SetConsoleMode(hCon, mode | (ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_PROCESSED_OUTPUT)),
       "setting wrap at EOL & processed output\n");

    /* write line, wrapping enabled, buffer doesn't exceed sb width */
    c.X = sbSize.X - 9; c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-9\n");

    ok(WriteConsole(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
    for (p = 0; p < 4; p++)
    {
        c.X = sbSize.X - 9 + p;
        okCHAR(hCon, c, mytest[p], TEST_ATTRIB);
    }
    c.X = sbSize.X - 9 + p;
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);
    c.X = 0; c.Y++;
    okCHAR(hCon, c, mytest[5], TEST_ATTRIB);
    for (c.X = 1; c.X < 8; c.X++)
        okCHAR(hCon, c, ' ', TEST_ATTRIB);
    okCHAR(hCon, c, mytest[7], TEST_ATTRIB);
    c.X++;
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);
    okCURSOR(hCon, c);

    /* write line, wrapping enabled, buffer does exceed sb width */
    c.X = sbSize.X - 3; c.Y = 2;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-3\n");

    ok(WriteConsole(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
    for (p = 0; p < 3; p++)
    {
        c.X = sbSize.X - 3 + p;
        okCHAR(hCon, c, mytest[p], TEST_ATTRIB);
    }
    c.X = 0; c.Y++;
    okCHAR(hCon, c, mytest[3], TEST_ATTRIB);
    c.X++;
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);

    c.X = 0; c.Y++;
    okCHAR(hCon, c, mytest[5], TEST_ATTRIB);
    for (c.X = 1; c.X < 8; c.X++)
        okCHAR(hCon, c, ' ', TEST_ATTRIB);
    okCHAR(hCon, c, mytest[7], TEST_ATTRIB);
    c.X++;
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);
    okCURSOR(hCon, c);
}

static void testWrite(HANDLE hCon, COORD sbSize)
{
    /* FIXME: should in fact insure that the sb is at least 10 character wide */
    ok(SetConsoleTextAttribute(hCon, TEST_ATTRIB), "Setting default text color\n");
    resetContent(hCon, sbSize, FALSE);
    testEmptyWrite(hCon);
    resetContent(hCon, sbSize, FALSE);
    testWriteSimple(hCon);
    resetContent(hCon, sbSize, FALSE);
    testWriteNotWrappedNotProcessed(hCon, sbSize);
    resetContent(hCon, sbSize, FALSE);
    testWriteNotWrappedProcessed(hCon, sbSize);
    resetContent(hCon, sbSize, FALSE);
    testWriteWrappedNotProcessed(hCon, sbSize);
    resetContent(hCon, sbSize, FALSE);
    testWriteWrappedProcessed(hCon, sbSize);
}

static void testScroll(HANDLE hCon, COORD sbSize)
{
    SMALL_RECT  scroll, clip;
    COORD       dst, c, tc;
    CHAR_INFO   ci;
    BOOL ret;

#define W 11
#define H 7

#define IN_SRECT(r,c) ((r).Left <= (c).X && (c).X <= (r).Right && (r).Top <= (c).Y && (c).Y <= (r).Bottom)
#define IN_SRECT2(r,d,c) ((d).X <= (c).X && (c).X <= (d).X + (r).Right - (r).Left && (d).Y <= (c).Y && (c).Y <= (d).Y + (r).Bottom - (r).Top)

    /* no clipping, src & dst rect don't overlap */
    resetContent(hCon, sbSize, TRUE);

    scroll.Left = 0;
    scroll.Right = W - 1;
    scroll.Top = 0;
    scroll.Bottom = H - 1;
    dst.X = W + 3;
    dst.Y = H + 3;
    ci.Char.UnicodeChar = '#';
    ci.Attributes = TEST_ATTRIB;

    clip.Left = 0;
    clip.Right = sbSize.X - 1;
    clip.Top = 0;
    clip.Bottom = sbSize.Y - 1;

    ok(ScrollConsoleScreenBuffer(hCon, &scroll, NULL, dst, &ci), "Scrolling SB\n");

    for (c.Y = 0; c.Y < sbSize.Y; c.Y++)
    {
        for (c.X = 0; c.X < sbSize.X; c.X++)
        {
            if (IN_SRECT2(scroll, dst, c) && IN_SRECT(clip, c))
            {
                tc.X = c.X - dst.X;
                tc.Y = c.Y - dst.Y;
                okCHAR(hCon, c, CONTENT(tc), DEFAULT_ATTRIB);
            }
            else if (IN_SRECT(scroll, c) && IN_SRECT(clip, c))
                okCHAR(hCon, c, '#', TEST_ATTRIB);
            else okCHAR(hCon, c, CONTENT(c), DEFAULT_ATTRIB);
        }
    }

    /* no clipping, src & dst rect do overlap */
    resetContent(hCon, sbSize, TRUE);

    scroll.Left = 0;
    scroll.Right = W - 1;
    scroll.Top = 0;
    scroll.Bottom = H - 1;
    dst.X = W /2;
    dst.Y = H / 2;
    ci.Char.UnicodeChar = '#';
    ci.Attributes = TEST_ATTRIB;

    clip.Left = 0;
    clip.Right = sbSize.X - 1;
    clip.Top = 0;
    clip.Bottom = sbSize.Y - 1;

    ok(ScrollConsoleScreenBuffer(hCon, &scroll, NULL, dst, &ci), "Scrolling SB\n");

    for (c.Y = 0; c.Y < sbSize.Y; c.Y++)
    {
        for (c.X = 0; c.X < sbSize.X; c.X++)
        {
            if (dst.X <= c.X && c.X < dst.X + W && dst.Y <= c.Y && c.Y < dst.Y + H)
            {
                tc.X = c.X - dst.X;
                tc.Y = c.Y - dst.Y;
                okCHAR(hCon, c, CONTENT(tc), DEFAULT_ATTRIB);
            }
            else if (c.X < W && c.Y < H) okCHAR(hCon, c, '#', TEST_ATTRIB);
            else okCHAR(hCon, c, CONTENT(c), DEFAULT_ATTRIB);
        }
    }

    /* clipping, src & dst rect don't overlap */
    resetContent(hCon, sbSize, TRUE);

    scroll.Left = 0;
    scroll.Right = W - 1;
    scroll.Top = 0;
    scroll.Bottom = H - 1;
    dst.X = W + 3;
    dst.Y = H + 3;
    ci.Char.UnicodeChar = '#';
    ci.Attributes = TEST_ATTRIB;

    clip.Left = W / 2;
    clip.Right = min(W + W / 2, sbSize.X - 1);
    clip.Top = H / 2;
    clip.Bottom = min(H + H / 2, sbSize.Y - 1);

    SetLastError(0xdeadbeef);
    ret = ScrollConsoleScreenBuffer(hCon, &scroll, &clip, dst, &ci);
    if (ret)
    {
        for (c.Y = 0; c.Y < sbSize.Y; c.Y++)
        {
            for (c.X = 0; c.X < sbSize.X; c.X++)
            {
                if (IN_SRECT2(scroll, dst, c) && IN_SRECT(clip, c))
                {
                    tc.X = c.X - dst.X;
                    tc.Y = c.Y - dst.Y;
                    okCHAR(hCon, c, CONTENT(tc), DEFAULT_ATTRIB);
                }
                else if (IN_SRECT(scroll, c) && IN_SRECT(clip, c))
                    okCHAR(hCon, c, '#', TEST_ATTRIB);
                else okCHAR(hCon, c, CONTENT(c), DEFAULT_ATTRIB);
            }
        }
    }
    else
    {
        /* Win9x will fail, Only accept ERROR_NOT_ENOUGH_MEMORY */
        ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY,
            "Expected ERROR_NOT_ENOUGH_MEMORY, got %u\n", GetLastError());
    }

    /* clipping, src & dst rect do overlap */
    resetContent(hCon, sbSize, TRUE);

    scroll.Left = 0;
    scroll.Right = W - 1;
    scroll.Top = 0;
    scroll.Bottom = H - 1;
    dst.X = W / 2 - 3;
    dst.Y = H / 2 - 3;
    ci.Char.UnicodeChar = '#';
    ci.Attributes = TEST_ATTRIB;

    clip.Left = W / 2;
    clip.Right = min(W + W / 2, sbSize.X - 1);
    clip.Top = H / 2;
    clip.Bottom = min(H + H / 2, sbSize.Y - 1);

    ok(ScrollConsoleScreenBuffer(hCon, &scroll, &clip, dst, &ci), "Scrolling SB\n");

    for (c.Y = 0; c.Y < sbSize.Y; c.Y++)
    {
        for (c.X = 0; c.X < sbSize.X; c.X++)
        {
            if (IN_SRECT2(scroll, dst, c) && IN_SRECT(clip, c))
            {
                tc.X = c.X - dst.X;
                tc.Y = c.Y - dst.Y;
                okCHAR(hCon, c, CONTENT(tc), DEFAULT_ATTRIB);
            }
            else if (IN_SRECT(scroll, c) && IN_SRECT(clip, c))
                okCHAR(hCon, c, '#', TEST_ATTRIB);
            else okCHAR(hCon, c, CONTENT(c), DEFAULT_ATTRIB);
        }
    }
}

static int mch_count;
/* we need the event as Wine console event generation isn't synchronous
 * (ie GenerateConsoleCtrlEvent returns before all ctrl-handlers in all
 * processes have been called).
 */
static HANDLE mch_event;
static BOOL WINAPI mch(DWORD event)
{
    mch_count++;
    SetEvent(mch_event);
    return TRUE;
}

static void testCtrlHandler(void)
{
    ok(!SetConsoleCtrlHandler(mch, FALSE), "Shouldn't succeed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Bad error %u\n", GetLastError());
    ok(SetConsoleCtrlHandler(mch, TRUE), "Couldn't set handler\n");
    /* wine requires the event for the test, as we cannot insure, so far, that event
     * are processed synchronously in GenerateConsoleCtrlEvent()
     */
    mch_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    mch_count = 0;
    ok(GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0), "Couldn't send ctrl-c event\n");
    /* FIXME: it isn't synchronous on wine but it can still happen before we test */
    if (0) ok(mch_count == 1, "Event isn't synchronous\n");
    ok(WaitForSingleObject(mch_event, 3000) == WAIT_OBJECT_0, "event sending didn't work\n");
    CloseHandle(mch_event);

    /* Turning off ctrl-c handling doesn't work on win9x such way ... */
    ok(SetConsoleCtrlHandler(NULL, TRUE), "Couldn't turn off ctrl-c handling\n");
    mch_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    mch_count = 0;
    if(!(GetVersion() & 0x80000000))
        /* ... and next line leads to an unhandled exception on 9x.  Avoid it on 9x. */
        ok(GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0), "Couldn't send ctrl-c event\n");
    ok(WaitForSingleObject(mch_event, 3000) == WAIT_TIMEOUT && mch_count == 0, "Event shouldn't have been sent\n");
    CloseHandle(mch_event);
    ok(SetConsoleCtrlHandler(mch, FALSE), "Couldn't remove handler\n");
    ok(!SetConsoleCtrlHandler(mch, FALSE), "Shouldn't succeed\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Bad error %u\n", GetLastError());
}

/*
 * Test console screen buffer:
 * 1) Try to set invalid handle.
 * 2) Try to set non-console handles.
 * 3) Use CONOUT$ file as active SB.
 * 4) Test cursor.
 * 5) Test output codepage to show it is not a property of SB.
 * 6) Test switching to old SB if we close all handles to current SB - works
 * in Windows, TODO in wine.
 *
 * What is not tested but should be:
 * 1) ScreenBufferInfo
 */
static void testScreenBuffer(HANDLE hConOut)
{
    HANDLE hConOutRW, hConOutRO, hConOutWT;
    HANDLE hFileOutRW, hFileOutRO, hFileOutWT;
    HANDLE hConOutNew;
    char test_str1[] = "Test for SB1";
    char test_str2[] = "Test for SB2";
    char test_cp866[] = {0xe2, 0xa5, 0xe1, 0xe2, 0};
    char test_cp1251[] = {0xf2, 0xe5, 0xf1, 0xf2, 0};
    WCHAR test_unicode[] = {0x0442, 0x0435, 0x0441, 0x0442, 0};
    WCHAR str_wbuf[20];
    char str_buf[20];
    DWORD len;
    COORD c;
    BOOL ret;
    DWORD oldcp;

    if (!IsValidCodePage(866))
    {
        skip("Codepage 866 not available\n");
        return;
    }

    /* In the beginning set output codepage to 866 */
    oldcp = GetConsoleOutputCP();
    SetLastError(0xdeadbeef);
    ret = SetConsoleOutputCP(866);
    if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("SetConsoleOutputCP is not implemented\n");
        return;
    }
    ok(ret, "Cannot set output codepage to 866\n");

    hConOutRW = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                         FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         CONSOLE_TEXTMODE_BUFFER, NULL);
    ok(hConOutRW != INVALID_HANDLE_VALUE,
       "Cannot create a new screen buffer for ReadWrite\n");
    hConOutRO = CreateConsoleScreenBuffer(GENERIC_READ,
                         FILE_SHARE_READ, NULL,
                         CONSOLE_TEXTMODE_BUFFER, NULL);
    ok(hConOutRO != INVALID_HANDLE_VALUE,
       "Cannot create a new screen buffer for ReadOnly\n");
    hConOutWT = CreateConsoleScreenBuffer(GENERIC_WRITE,
                         FILE_SHARE_WRITE, NULL,
                         CONSOLE_TEXTMODE_BUFFER, NULL);
    ok(hConOutWT != INVALID_HANDLE_VALUE,
       "Cannot create a new screen buffer for WriteOnly\n");

    hFileOutRW = CreateFileA("NUL", GENERIC_READ | GENERIC_WRITE,
                             FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                             OPEN_EXISTING, 0, NULL);
    ok(hFileOutRW != INVALID_HANDLE_VALUE, "Cannot open NUL for ReadWrite\n");
    hFileOutRO = CreateFileA("NUL", GENERIC_READ, FILE_SHARE_READ,
                             NULL, OPEN_EXISTING, 0, NULL);
    ok(hFileOutRO != INVALID_HANDLE_VALUE, "Cannot open NUL for ReadOnly\n");
    hFileOutWT = CreateFileA("NUL", GENERIC_WRITE, FILE_SHARE_WRITE,
                             NULL, OPEN_EXISTING, 0, NULL);
    ok(hFileOutWT != INVALID_HANDLE_VALUE, "Cannot open NUL for WriteOnly\n");

    /* Trying to set invalid handle */
    SetLastError(0);
    ok(!SetConsoleActiveScreenBuffer(INVALID_HANDLE_VALUE),
       "Shouldn't succeed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_HANDLE, GetLastError());

    /* Trying to set non-console handles */
    SetLastError(0);
    ok(!SetConsoleActiveScreenBuffer(hFileOutRW), "Shouldn't succeed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_HANDLE, GetLastError());

    SetLastError(0);
    ok(!SetConsoleActiveScreenBuffer(hFileOutRO), "Shouldn't succeed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_HANDLE, GetLastError());

    SetLastError(0);
    ok(!SetConsoleActiveScreenBuffer(hFileOutWT), "Shouldn't succeed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_HANDLE, GetLastError());

    CloseHandle(hFileOutRW);
    CloseHandle(hFileOutRO);
    CloseHandle(hFileOutWT);

    /* Trying to set SB handles with various access modes */
    SetLastError(0);
    ok(!SetConsoleActiveScreenBuffer(hConOutRO), "Shouldn't succeed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_HANDLE, GetLastError());

    ok(SetConsoleActiveScreenBuffer(hConOutWT), "Couldn't set new WriteOnly SB\n");

    ok(SetConsoleActiveScreenBuffer(hConOutRW), "Couldn't set new ReadWrite SB\n");

    CloseHandle(hConOutWT);
    CloseHandle(hConOutRO);

    /* Now we have two ReadWrite SB, active must be hConOutRW */
    /* Open current SB via CONOUT$ */
    hConOutNew = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0,
                             NULL, OPEN_EXISTING, 0, 0);
    ok(hConOutNew != INVALID_HANDLE_VALUE, "CONOUT$ is not opened\n");


    /* test cursor */
    c.X = c.Y = 10;
    SetConsoleCursorPosition(hConOut, c);
    c.X = c.Y = 5;
    SetConsoleCursorPosition(hConOutRW, c);
    okCURSOR(hConOutNew, c);
    c.X = c.Y = 10;
    okCURSOR(hConOut, c);


    c.X = c.Y = 0;

    /* Write using hConOutNew... */
    SetConsoleCursorPosition(hConOutNew, c);
    ret = WriteConsoleA(hConOutNew, test_str2, lstrlenA(test_str2), &len, NULL);
    ok (ret && len == lstrlenA(test_str2), "WriteConsoleA failed\n");
    /* ... and read it back via hConOutRW */
    ret = ReadConsoleOutputCharacterA(hConOutRW, str_buf, lstrlenA(test_str2), c, &len);
    ok(ret && len == lstrlenA(test_str2), "ReadConsoleOutputCharacterA failed\n");
    str_buf[lstrlenA(test_str2)] = 0;
    ok(!lstrcmpA(str_buf, test_str2), "got '%s' expected '%s'\n", str_buf, test_str2);


    /* Now test output codepage handling. Current is 866 as we set earlier. */
    SetConsoleCursorPosition(hConOutRW, c);
    ret = WriteConsoleA(hConOutRW, test_cp866, lstrlenA(test_cp866), &len, NULL);
    ok(ret && len == lstrlenA(test_cp866), "WriteConsoleA failed\n");
    ret = ReadConsoleOutputCharacterW(hConOutRW, str_wbuf, lstrlenA(test_cp866), c, &len);
    ok(ret && len == lstrlenA(test_cp866), "ReadConsoleOutputCharacterW failed\n");
    str_wbuf[lstrlenA(test_cp866)] = 0;
    ok(!lstrcmpW(str_wbuf, test_unicode), "string does not match the pattern\n");

    /*
     * cp866 is OK, let's switch to cp1251.
     * We expect that this codepage will be used in every SB - active and not.
     */
    ok(SetConsoleOutputCP(1251), "Cannot set output cp to 1251\n");
    SetConsoleCursorPosition(hConOutRW, c);
    ret = WriteConsoleA(hConOutRW, test_cp1251, lstrlenA(test_cp1251), &len, NULL);
    ok(ret && len == lstrlenA(test_cp1251), "WriteConsoleA failed\n");
    ret = ReadConsoleOutputCharacterW(hConOutRW, str_wbuf, lstrlenA(test_cp1251), c, &len);
    ok(ret && len == lstrlenA(test_cp1251), "ReadConsoleOutputCharacterW failed\n");
    str_wbuf[lstrlenA(test_cp1251)] = 0;
    ok(!lstrcmpW(str_wbuf, test_unicode), "string does not match the pattern\n");

    /* Check what has happened to hConOut. */
    SetConsoleCursorPosition(hConOut, c);
    ret = WriteConsoleA(hConOut, test_cp1251, lstrlenA(test_cp1251), &len, NULL);
    ok(ret && len == lstrlenA(test_cp1251), "WriteConsoleA failed\n");
    ret = ReadConsoleOutputCharacterW(hConOut, str_wbuf, lstrlenA(test_cp1251), c, &len);
    ok(ret && len == lstrlenA(test_cp1251), "ReadConsoleOutputCharacterW failed\n");
    str_wbuf[lstrlenA(test_cp1251)] = 0;
    ok(!lstrcmpW(str_wbuf, test_unicode), "string does not match the pattern\n");

    /* Close all handles of current console SB */
    CloseHandle(hConOutNew);
    CloseHandle(hConOutRW);

    /* Now active SB should be hConOut */
    hConOutNew = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0,
                             NULL, OPEN_EXISTING, 0, 0);
    ok(hConOutNew != INVALID_HANDLE_VALUE, "CONOUT$ is not opened\n");

    /* Write using hConOutNew... */
    SetConsoleCursorPosition(hConOutNew, c);
    ret = WriteConsoleA(hConOutNew, test_str1, lstrlenA(test_str1), &len, NULL);
    ok (ret && len == lstrlenA(test_str1), "WriteConsoleA failed\n");
    /* ... and read it back via hConOut */
    ret = ReadConsoleOutputCharacterA(hConOut, str_buf, lstrlenA(test_str1), c, &len);
    ok(ret && len == lstrlenA(test_str1), "ReadConsoleOutputCharacterA failed\n");
    str_buf[lstrlenA(test_str1)] = 0;
    todo_wine ok(!lstrcmpA(str_buf, test_str1), "got '%s' expected '%s'\n", str_buf, test_str1);
    CloseHandle(hConOutNew);

    /* This is not really needed under Windows */
    SetConsoleActiveScreenBuffer(hConOut);

    /* restore codepage */
    SetConsoleOutputCP(oldcp);
}

static void test_GetSetConsoleInputExeName(void)
{
    BOOL ret;
    DWORD error;
    char buffer[MAX_PATH], module[MAX_PATH], *p;
    static char input_exe[MAX_PATH] = "winetest.exe";

    SetLastError(0xdeadbeef);
    ret = pGetConsoleInputExeNameA(0, NULL);
    error = GetLastError();
    ok(ret, "GetConsoleInputExeNameA failed\n");
    ok(error == ERROR_BUFFER_OVERFLOW, "got %u expected ERROR_BUFFER_OVERFLOW\n", error);

    SetLastError(0xdeadbeef);
    ret = pGetConsoleInputExeNameA(0, buffer);
    error = GetLastError();
    ok(ret, "GetConsoleInputExeNameA failed\n");
    ok(error == ERROR_BUFFER_OVERFLOW, "got %u expected ERROR_BUFFER_OVERFLOW\n", error);

    GetModuleFileNameA(GetModuleHandle(NULL), module, sizeof(module));
    p = strrchr(module, '\\') + 1;

    ret = pGetConsoleInputExeNameA(sizeof(buffer)/sizeof(buffer[0]), buffer);
    ok(ret, "GetConsoleInputExeNameA failed\n");
    todo_wine ok(!lstrcmpA(buffer, p), "got %s expected %s\n", buffer, p);

    SetLastError(0xdeadbeef);
    ret = pSetConsoleInputExeNameA(NULL);
    error = GetLastError();
    ok(!ret, "SetConsoleInputExeNameA failed\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u expected ERROR_INVALID_PARAMETER\n", error);

    SetLastError(0xdeadbeef);
    ret = pSetConsoleInputExeNameA("");
    error = GetLastError();
    ok(!ret, "SetConsoleInputExeNameA failed\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %u expected ERROR_INVALID_PARAMETER\n", error);

    ret = pSetConsoleInputExeNameA(input_exe);
    ok(ret, "SetConsoleInputExeNameA failed\n");

    ret = pGetConsoleInputExeNameA(sizeof(buffer)/sizeof(buffer[0]), buffer);
    ok(ret, "GetConsoleInputExeNameA failed\n");
    ok(!lstrcmpA(buffer, input_exe), "got %s expected %s\n", buffer, input_exe);
}

START_TEST(console)
{
    HANDLE hConIn, hConOut;
    BOOL ret;
    CONSOLE_SCREEN_BUFFER_INFO	sbi;

    init_function_pointers();

    /* be sure we have a clean console (and that's our own)
     * FIXME: this will make the test fail (currently) if we don't run
     * under X11
     * Another solution would be to rerun the test under wineconsole with
     * the curses backend
     */

    /* first, we detach and open a fresh console to play with */
    FreeConsole();
    ok(AllocConsole(), "Couldn't alloc console\n");
    hConIn = CreateFileA("CONIN$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    hConOut = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);

    /* now verify everything's ok */
    ok(hConIn != INVALID_HANDLE_VALUE, "Opening ConIn\n");
    ok(hConOut != INVALID_HANDLE_VALUE, "Opening ConOut\n");

    ret = GetConsoleScreenBufferInfo(hConOut, &sbi);
    ok(ret, "Getting sb info\n");
    if (!ret) return;

    /* Non interactive tests */
    testCursor(hConOut, sbi.dwSize);
    /* test parameters (FIXME: test functionality) */
    testCursorInfo(hConOut);
    /* will test wrapped (on/off) & processed (on/off) strings output */
    testWrite(hConOut, sbi.dwSize);
    /* will test line scrolling at the bottom of the screen */
    /* testBottomScroll(); */
    /* will test all the scrolling operations */
    testScroll(hConOut, sbi.dwSize);
    /* will test sb creation / modification / codepage handling */
    testScreenBuffer(hConOut);
    testCtrlHandler();
    /* still to be done: access rights & access on objects */

    if (!pGetConsoleInputExeNameA || !pSetConsoleInputExeNameA)
    {
        win_skip("GetConsoleInputExeNameA and/or SetConsoleInputExeNameA is not available\n");
        return;
    }
    else
        test_GetSetConsoleInputExeName();
}
