/*
 * Unit tests for console API
 *
 * Copyright (c) 2003,2004 Eric Pouech
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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "wine/test.h"
#include <windows.h>
#include <stdio.h>

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
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError: expecting %lu got %lu\n",
       (DWORD) ERROR_INVALID_HANDLE, (DWORD) GetLastError());

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
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError: expecting %lu got %lu\n",
       (DWORD) ERROR_INVALID_PARAMETER, (DWORD) GetLastError());

    c.X = sbSize.X - 1;
    c.Y = sbSize.Y;
    ok(SetConsoleCursorPosition(hCon, c) == 0, "Cursor is outside\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError: expecting %lu got %lu\n",
       (DWORD) ERROR_INVALID_PARAMETER, (DWORD) GetLastError());

    c.X = -1;
    c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) == 0, "Cursor is outside\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError: expecting %lu got %lu\n",
       (DWORD) ERROR_INVALID_PARAMETER, (DWORD) GetLastError());

    c.X = 0;
    c.Y = -1;
    ok(SetConsoleCursorPosition(hCon, c) == 0, "Cursor is outside\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError: expecting %lu got %lu\n",
       (DWORD) ERROR_INVALID_PARAMETER, (DWORD) GetLastError());
}

static void testWriteSimple(HANDLE hCon, COORD sbSize)
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
    ok(ret != 0 && len == mylen, "Couldn't write, ret = %d, len = %ld\n", ret, len);
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
    testWriteSimple(hCon, sbSize);
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
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Bad error %lu\n", GetLastError());
    ok(SetConsoleCtrlHandler(mch, TRUE), "Couldn't set handler\n");
    /* wine requires the event for the test, as we cannot insure, so far, that event
     * are processed synchronously in GenerateConsoleCtrlEvent()
     */
    mch_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    mch_count = 0;
    ok(GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0), "Couldn't send ctrl-c event\n");
#if 0  /* FIXME: it isn't synchronous on wine but it can still happen before we test */
    todo_wine ok(mch_count == 1, "Event isn't synchronous\n");
#endif
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
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Bad error %lu\n", GetLastError());
}

START_TEST(console)
{
    HANDLE hConIn, hConOut;
    BOOL ret;
    CONSOLE_SCREEN_BUFFER_INFO	sbi;

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

    ok(ret = GetConsoleScreenBufferInfo(hConOut, &sbi), "Getting sb info\n");
    if (!ret) return;

    /* Non interactive tests */
    testCursor(hConOut, sbi.dwSize);
    /* will test wrapped (on/off) & processed (on/off) strings output */
    testWrite(hConOut, sbi.dwSize);
    /* will test line scrolling at the bottom of the screen */
    /* testBottomScroll(); */
    /* will test all the scrolling operations */
    testScroll(hConOut, sbi.dwSize);
    /* will test sb creation / modification... */
    /* testScreenBuffer() */
    testCtrlHandler();
    /* still to be done: access rights & access on objects */
}
