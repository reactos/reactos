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

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winternl.h>
#include <winioctl.h>
#include <stdio.h>

#include "wine/test.h"
#ifdef __REACTOS__
#include <wincon_undoc.h>
typedef void *HPCON;
#define FILE_DEVICE_CONSOLE 80
#endif

static void (WINAPI *pClosePseudoConsole)(HPCON);
static HRESULT (WINAPI *pCreatePseudoConsole)(COORD,HANDLE,HANDLE,DWORD,HPCON*);
static BOOL (WINAPI *pGetConsoleInputExeNameA)(DWORD, LPSTR);
static DWORD (WINAPI *pGetConsoleProcessList)(LPDWORD, DWORD);
static HANDLE (WINAPI *pOpenConsoleW)(LPCWSTR,DWORD,BOOL,DWORD);
static BOOL (WINAPI *pSetConsoleInputExeNameA)(LPCSTR);
static BOOL (WINAPI *pVerifyConsoleIoHandle)(HANDLE handle);

static BOOL skip_nt;

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
  expect = ReadConsoleOutputCharacterA((hCon), &__ch, 1, (c), &__len) == 1 && __len == 1 && __ch == (ch); \
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
    KERNEL32_GET_PROC(ClosePseudoConsole);
    KERNEL32_GET_PROC(CreatePseudoConsole);
    KERNEL32_GET_PROC(GetConsoleInputExeNameA);
    KERNEL32_GET_PROC(GetConsoleProcessList);
    KERNEL32_GET_PROC(OpenConsoleW);
    KERNEL32_GET_PROC(SetConsoleInputExeNameA);
    KERNEL32_GET_PROC(VerifyConsoleIoHandle);

#undef KERNEL32_GET_PROC
}

static HANDLE create_unbound_handle(BOOL output, BOOL test_status)
{
    OBJECT_ATTRIBUTES attr = {sizeof(attr)};
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING name;
    HANDLE handle;
    NTSTATUS status;

    attr.ObjectName = &name;
    attr.Attributes = OBJ_INHERIT;
    RtlInitUnicodeString( &name, output ? L"\\Device\\ConDrv\\Output" : L"\\Device\\ConDrv\\Input" );
    status = NtCreateFile( &handle, FILE_READ_DATA | FILE_WRITE_DATA | SYNCHRONIZE | FILE_READ_ATTRIBUTES |
                           FILE_WRITE_ATTRIBUTES, &attr, &iosb, NULL, FILE_ATTRIBUTE_NORMAL,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE, FILE_CREATE,
                           FILE_NON_DIRECTORY_FILE | FILE_SYNCHRONOUS_IO_NONALERT, NULL, 0 );
    if (test_status) ok(!status, "NtCreateFile failed: %#lx\n", status);
    return status ? NULL : handle;
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

/* dummy console ctrl handler to test reset of ctrl handler's list */
static BOOL WINAPI mydummych(DWORD event)
{
    return TRUE;
}

static void testCursor(HANDLE hCon, COORD sbSize)
{
    COORD		c;

    c.X = c.Y = 0;
    ok(SetConsoleCursorPosition(0, c) == 0, "No handle\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError: expecting %u got %lu\n",
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
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_PARAMETER, GetLastError());

    c.X = sbSize.X - 1;
    c.Y = sbSize.Y;
    ok(SetConsoleCursorPosition(hCon, c) == 0, "Cursor is outside\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_PARAMETER, GetLastError());

    c.X = -1;
    c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) == 0, "Cursor is outside\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_PARAMETER, GetLastError());

    c.X = 0;
    c.Y = -1;
    ok(SetConsoleCursorPosition(hCon, c) == 0, "Cursor is outside\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_PARAMETER, GetLastError());
}

static void testCursorInfo(HANDLE hCon)
{
    BOOL ret;
    CONSOLE_CURSOR_INFO info;
    HANDLE pipe1, pipe2;

    SetLastError(0xdeadbeef);
    ret = GetConsoleCursorInfo(NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_HANDLE, GetLastError());

    SetLastError(0xdeadbeef);
    info.dwSize = -1;
    ret = GetConsoleCursorInfo(NULL, &info);
    ok(!ret, "Expected failure\n");
    ok(info.dwSize == -1, "Expected no change for dwSize\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_HANDLE, GetLastError());

    /* Test the correct call first to distinguish between win9x and the rest */
    SetLastError(0xdeadbeef);
    ret = GetConsoleCursorInfo(hCon, &info);
    ok(ret, "Expected success\n");
    ok(info.dwSize == 25 ||
       info.dwSize == 12 /* win9x */,
       "Expected 12 or 25, got %ld\n", info.dwSize);
    ok(info.bVisible, "Expected the cursor to be visible\n");
    ok(GetLastError() == 0xdeadbeef, "GetLastError: expecting %u got %lu\n",
       0xdeadbeef, GetLastError());

    CreatePipe(&pipe1, &pipe2, NULL, 0);
    info.dwSize = -1;
    ret = GetConsoleCursorInfo(pipe1, &info);
    ok(!ret, "Expected failure\n");
    ok(info.dwSize == -1, "Expected no change for dwSize\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "GetLastError: %lu\n",  GetLastError());
    CloseHandle(pipe1);
    CloseHandle(pipe2);

    /* Don't test NULL CONSOLE_CURSOR_INFO, it crashes on win9x and win7 */
}

static void testEmptyWrite(HANDLE hCon)
{
    static const char	emptybuf[16];
    COORD		c;
    DWORD		len;

    c.X = c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left\n");

    len = -1;
    ok(WriteConsoleA(hCon, NULL, 0, &len, NULL) != 0 && len == 0, "WriteConsole\n");
    okCURSOR(hCon, c);

    /* Passing a NULL lpBuffer with sufficiently large non-zero length succeeds
     * on native Windows and result in memory-like contents being written to
     * the console. Calling WriteConsoleW like this will crash on Wine. */
    if (0)
    {
        len = -1;
        ok(!WriteConsoleA(hCon, NULL, 16, &len, NULL) && len == -1, "WriteConsole\n");
        okCURSOR(hCon, c);

        /* Cursor advances for this call. */
        len = -1;
        ok(WriteConsoleA(hCon, NULL, 128, &len, NULL) != 0 && len == 128, "WriteConsole\n");
    }

    len = -1;
    ok(WriteConsoleA(hCon, emptybuf, 0, &len, NULL) != 0 && len == 0, "WriteConsole\n");
    okCURSOR(hCon, c);

    /* WriteConsole does not halt on a null terminator and is happy to write
     * memory contents beyond the actual size of the buffer. */
    len = -1;
    ok(WriteConsoleA(hCon, emptybuf, 16, &len, NULL) != 0 && len == 16, "WriteConsole\n");
    c.X += 16;
    okCURSOR(hCon, c);
}

static void simple_write_console(HANDLE console, const char *text)
{
    DWORD len;
    COORD c = {0, 0};
    BOOL ret;

    /* single line write */
    c.X = c.Y = 0;
    ok(SetConsoleCursorPosition(console, c) != 0, "Cursor in upper-left\n");

    ret = WriteConsoleA(console, text, strlen(text), &len, NULL);
    ok(ret, "WriteConsoleA failed: %lu\n", GetLastError());
    ok(len == strlen(text), "unexpected len %lu\n", len);
}

static void testWriteSimple(HANDLE hCon)
{
    const char*	mytest = "abcdefg";
    int mylen = strlen(mytest);
    COORD c = {0, 0};
    DWORD len;
    BOOL ret;

    simple_write_console(hCon, mytest);

    for (c.X = 0; c.X < mylen; c.X++)
    {
        okCHAR(hCon, c, mytest[c.X], TEST_ATTRIB);
    }

    okCURSOR(hCon, c);
    okCHAR(hCon, c, ' ', DEFAULT_ATTRIB);

    ret = WriteFile(hCon, mytest, mylen, &len, NULL);
    ok(ret, "WriteFile failed: %lu\n", GetLastError());
    ok(len == mylen, "unexpected len = %lu\n", len);

    for (c.X = 0; c.X < 2 * mylen; c.X++)
    {
        okCHAR(hCon, c, mytest[c.X % mylen], TEST_ATTRIB);
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
    char                ctrl_buf[32];
    int                 ret;
    int			p;

    ok(GetConsoleMode(hCon, &mode) && SetConsoleMode(hCon, mode & ~(ENABLE_PROCESSED_OUTPUT|ENABLE_WRAP_AT_EOL_OUTPUT)),
       "clearing wrap at EOL & processed output\n");

    /* write line, wrapping disabled, buffer exceeds sb width */
    c.X = sbSize.X - 3; c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-3\n");

    ret = WriteConsoleA(hCon, mytest, mylen, &len, NULL);
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

    ok(WriteConsoleA(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");

    /* test how control chars are handled. */
    c.X = c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-3\n");
    for (p = 0; p < 32; p++) ctrl_buf[p] = (char)p;
    ok(WriteConsoleA(hCon, ctrl_buf, 32, &len, NULL) != 0 && len == 32, "WriteConsole\n");
    for (p = 0; p < 32; p++)
    {
        c.X = p; c.Y = 0;
        okCHAR(hCon, c, (char)p, TEST_ATTRIB);
    }
}

static void testWriteNotWrappedProcessed(HANDLE hCon, COORD sbSize)
{
    COORD		c;
    DWORD		len, mode;
    const char*		mytest = "abcd\nf\tg";
    const int	mylen = strlen(mytest);
    const int	mylen2 = strchr(mytest, '\n') - mytest;
    int			p;
    WORD                attr;

    ok(GetConsoleMode(hCon, &mode) && SetConsoleMode(hCon, (mode | ENABLE_PROCESSED_OUTPUT) &
                                                     ~(ENABLE_WRAP_AT_EOL_OUTPUT | ENABLE_VIRTUAL_TERMINAL_PROCESSING)),
       "clearing wrap at EOL & setting processed output\n");

    /* write line, wrapping disabled, buffer exceeds sb width */
    c.X = sbSize.X - 5; c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-5\n");

    ok(WriteConsoleA(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
    c.Y = 0;
    for (c.X = sbSize.X - 5; c.X < sbSize.X - 1; c.X++)
    {
        okCHAR(hCon, c, mytest[c.X - sbSize.X + 5], TEST_ATTRIB);
    }

    ReadConsoleOutputAttribute(hCon, &attr, 1, c, &len);
    /* Win9x and WinMe change the attribs for '\n' up to 'f' */
    if (attr == TEST_ATTRIB)
    {
        win_skip("Win9x/WinMe don't respect ~ENABLE_WRAP_AT_EOL_OUTPUT\n");
        return;
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

    ok(WriteConsoleA(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
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

    ok(WriteConsoleA(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
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

    ok(WriteConsoleA(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
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
    WORD                attr;

    ok(GetConsoleMode(hCon, &mode) && SetConsoleMode(hCon, mode | (ENABLE_WRAP_AT_EOL_OUTPUT|ENABLE_PROCESSED_OUTPUT)),
       "setting wrap at EOL & processed output\n");

    /* write line, wrapping enabled, buffer doesn't exceed sb width */
    c.X = sbSize.X - 9; c.Y = 0;
    ok(SetConsoleCursorPosition(hCon, c) != 0, "Cursor in upper-left-9\n");

    ok(WriteConsoleA(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
    for (p = 0; p < 4; p++)
    {
        c.X = sbSize.X - 9 + p;
        okCHAR(hCon, c, mytest[p], TEST_ATTRIB);
    }
    c.X = sbSize.X - 9 + p;
    ReadConsoleOutputAttribute(hCon, &attr, 1, c, &len);
    if (attr == TEST_ATTRIB)
        win_skip("Win9x/WinMe changes attribs for '\\n' up to 'f'\n");
    else
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

    ok(WriteConsoleA(hCon, mytest, mylen, &len, NULL) != 0 && len == mylen, "WriteConsole\n");
    for (p = 0; p < 3; p++)
    {
        c.X = sbSize.X - 3 + p;
        okCHAR(hCon, c, mytest[p], TEST_ATTRIB);
    }
    c.X = 0; c.Y++;
    okCHAR(hCon, c, mytest[3], TEST_ATTRIB);
    c.X++;
    ReadConsoleOutputAttribute(hCon, &attr, 1, c, &len);
    if (attr == TEST_ATTRIB)
        win_skip("Win9x/WinMe changes attribs for '\\n' up to 'f'\n");
    else
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
    /* FIXME: should in fact ensure that the sb is at least 10 characters wide */
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

    ok(ScrollConsoleScreenBufferA(hCon, &scroll, NULL, dst, &ci), "Scrolling SB\n");

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

    ok(ScrollConsoleScreenBufferA(hCon, &scroll, NULL, dst, &ci), "Scrolling SB\n");

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
    ret = ScrollConsoleScreenBufferA(hCon, &scroll, &clip, dst, &ci);
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
            "Expected ERROR_NOT_ENOUGH_MEMORY, got %lu\n", GetLastError());
    }

    /* no clipping, src & dst rect do overlap */
    scroll.Left = 0;
    scroll.Right = W - 1;
    scroll.Top = 0;
    scroll.Bottom = H - 1;
    dst.X = W / 2 - 3;
    dst.Y = H / 2 - 3;
    ci.Char.UnicodeChar = '#';
    ci.Attributes = TEST_ATTRIB;

    ret = ScrollConsoleScreenBufferA(hCon, &scroll, NULL, dst, &ci);
    ok(ret, "ScrollConsoleScreenBufferA failed: %lu\n", GetLastError());
    /* no win10 1909 error here, only check the result of the clipped case */

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

    /* Windows 10 1909 fails if the destination is not in the clip rect
     * but the result is still ok!
     */
    SetLastError(0xdeadbeef);
    ret = ScrollConsoleScreenBufferA(hCon, &scroll, &clip, dst, &ci);
    ok((ret && GetLastError() == 0xdeadbeef) ||
       broken(!ret && GetLastError() == ERROR_INVALID_PARAMETER),
       "ScrollConsoleScreenBufferA failed: %lu\n", GetLastError());

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
    /* wine requires the event for the test, as we cannot ensure, so far, that
     * events are processed synchronously in GenerateConsoleCtrlEvent()
     */
    mch_event = CreateEventA(NULL, TRUE, FALSE, NULL);
    mch_count = 0;
    ok(GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0), "Couldn't send ctrl-c event\n");
    /* FIXME: it isn't synchronous on wine but it can still happen before we test */
    if (0) ok(mch_count == 1, "Event isn't synchronous\n");
    ok(WaitForSingleObject(mch_event, 3000) == WAIT_OBJECT_0, "event sending didn't work\n");
    CloseHandle(mch_event);

    ok(SetConsoleCtrlHandler(NULL, FALSE), "Couldn't turn on ctrl-c handling\n");
    ok((RtlGetCurrentPeb()->ProcessParameters->ConsoleFlags & 1) == 0,
       "Unexpect ConsoleFlags value %lx\n", RtlGetCurrentPeb()->ProcessParameters->ConsoleFlags);

    /* Turning off ctrl-c handling doesn't work on win9x such way ... */
    ok(SetConsoleCtrlHandler(NULL, TRUE), "Couldn't turn off ctrl-c handling\n");
    ok((RtlGetCurrentPeb()->ProcessParameters->ConsoleFlags & 1) != 0,
       "Unexpect ConsoleFlags value %lx\n", RtlGetCurrentPeb()->ProcessParameters->ConsoleFlags);
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
    DWORD len, error;
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
       "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_HANDLE, GetLastError());

    /* Trying to set non-console handles */
    SetLastError(0);
    ok(!SetConsoleActiveScreenBuffer(hFileOutRW), "Shouldn't succeed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_HANDLE, GetLastError());

    SetLastError(0);
    ok(!SetConsoleActiveScreenBuffer(hFileOutRO), "Shouldn't succeed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_HANDLE, GetLastError());

    SetLastError(0);
    ok(!SetConsoleActiveScreenBuffer(hFileOutWT), "Shouldn't succeed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE,
       "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_HANDLE, GetLastError());

    /* trying to write non-console handle */
    SetLastError(0xdeadbeef);
    ret = WriteConsoleA(hFileOutRW, test_str1, lstrlenA(test_str1), &len, NULL);
    error = GetLastError();
    ok(!ret, "Shouldn't succeed\n");
    ok(error == ERROR_INVALID_HANDLE || error == ERROR_INVALID_FUNCTION,
       "GetLastError: got %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = WriteConsoleA(hFileOutRO, test_str1, lstrlenA(test_str1), &len, NULL);
    error = GetLastError();
    ok(!ret, "Shouldn't succeed\n");
    ok(error == ERROR_INVALID_HANDLE || error == ERROR_INVALID_FUNCTION,
       "GetLastError: got %lu\n", error);

    SetLastError(0xdeadbeef);
    ret = WriteConsoleA(hFileOutWT, test_str1, lstrlenA(test_str1), &len, NULL);
    error = GetLastError();
    ok(!ret, "Shouldn't succeed\n");
    ok(error == ERROR_INVALID_HANDLE || error == ERROR_INVALID_FUNCTION,
       "GetLastError: got %lu\n", error);

    CloseHandle(hFileOutRW);
    CloseHandle(hFileOutRO);
    CloseHandle(hFileOutWT);

    /* Trying to set SB handles with various access modes */
    SetLastError(0);
    ok(!SetConsoleActiveScreenBuffer(hConOutRO), "Shouldn't succeed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE || broken(GetLastError() == ERROR_ACCESS_DENIED) /* win10 1809 */,
       "unexpected last error %lu\n", GetLastError());

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
    ret = ReadConsoleOutputCharacterW(hConOutRW, str_wbuf, lstrlenW(test_unicode), c, &len);
    /* Work around some broken results under Windows with some locale (ja, cn, ko...)
     * Looks like a real bug in Win10 (at least).
     */
    if (ret && broken(len == lstrlenW(test_unicode) / sizeof(WCHAR)))
        ret = ReadConsoleOutputCharacterW(hConOutRW, str_wbuf, lstrlenW(test_unicode) * sizeof(WCHAR), c, &len);
    ok(ret, "ReadConsoleOutputCharacterW failed\n");
    ok(len == lstrlenW(test_unicode), "unexpected len %lu %u\n", len, lstrlenW(test_unicode));
    ok(!memcmp(str_wbuf, test_unicode, lstrlenW(test_unicode) * sizeof(WCHAR)),
       "string does not match the pattern\n");

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

#ifndef __REACTOS__ // Can't link
static void test_new_screen_buffer_properties(HANDLE hConOut)
{
    BOOL ret;
    HANDLE hConOut2;
    CONSOLE_FONT_INFOEX cfi, cfi2;
    CONSOLE_SCREEN_BUFFER_INFO csbi, csbi2;

    /* Font information */
    cfi.cbSize = cfi2.cbSize = sizeof(CONSOLE_FONT_INFOEX);

    ret = GetCurrentConsoleFontEx(hConOut, FALSE, &cfi);
    ok(ret, "GetCurrentConsoleFontEx failed: error %lu\n", GetLastError());

    hConOut2 = CreateConsoleScreenBuffer(GENERIC_READ|GENERIC_WRITE, 0, NULL,
                                         CONSOLE_TEXTMODE_BUFFER, NULL);
    ok(hConOut2 != INVALID_HANDLE_VALUE, "CreateConsoleScreenBuffer failed: error %lu\n", GetLastError());

    ret = GetCurrentConsoleFontEx(hConOut2, FALSE, &cfi2);
    ok(ret, "GetCurrentConsoleFontEx failed: error %lu\n", GetLastError());
    CloseHandle(hConOut2);

    ok(cfi2.nFont == cfi.nFont, "Font index should match: "
       "got %lu, expected %lu\n", cfi2.nFont, cfi.nFont);
    ok(cfi2.dwFontSize.X == cfi.dwFontSize.X, "Font width should match: "
      "got %d, expected %d\n", cfi2.dwFontSize.X, cfi.dwFontSize.X);
    ok(cfi2.dwFontSize.Y == cfi.dwFontSize.Y, "Font height should match: "
       "got %d, expected %d\n", cfi2.dwFontSize.Y, cfi.dwFontSize.Y);
    ok(cfi2.FontFamily == cfi.FontFamily, "Font family should match: "
       "got %u, expected %u\n", cfi2.FontFamily, cfi.FontFamily);
    ok(cfi2.FontWeight == cfi.FontWeight, "Font weight should match: "
       "got %u, expected %u\n", cfi2.FontWeight, cfi.FontWeight);
    ok(!lstrcmpW(cfi2.FaceName, cfi.FaceName), "Font name should match: "
       "got %s, expected %s\n", wine_dbgstr_w(cfi2.FaceName), wine_dbgstr_w(cfi.FaceName));

    /* Display window size */
    ret = GetConsoleScreenBufferInfo(hConOut, &csbi);
    ok(ret, "GetConsoleScreenBufferInfo failed: error %lu\n", GetLastError());

    hConOut2 = CreateConsoleScreenBuffer(GENERIC_READ|GENERIC_WRITE, 0, NULL,
                                         CONSOLE_TEXTMODE_BUFFER, NULL);
    ok(hConOut2 != INVALID_HANDLE_VALUE, "CreateConsoleScreenBuffer failed: error %lu\n", GetLastError());

    ret = GetConsoleScreenBufferInfo(hConOut2, &csbi2);
    ok(ret, "GetConsoleScreenBufferInfo failed: error %lu\n", GetLastError());
    CloseHandle(hConOut2);

    ok(csbi2.srWindow.Left == csbi.srWindow.Left, "Left coordinate should match\n");
    ok(csbi2.srWindow.Top == csbi.srWindow.Top, "Top coordinate should match\n");
    ok(csbi2.srWindow.Right == csbi.srWindow.Right, "Right coordinate should match\n");
    ok(csbi2.srWindow.Bottom == csbi.srWindow.Bottom, "Bottom coordinate should match\n");
}

static void test_new_screen_buffer_color_attributes(HANDLE hConOut)
{
    CONSOLE_SCREEN_BUFFER_INFOEX csbi, csbi2;
    BOOL ret;
    HANDLE hConOut2;
    WORD orig_attr, orig_popup, attr;

    csbi.cbSize = csbi2.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

    ret = GetConsoleScreenBufferInfoEx(hConOut, &csbi);
    ok(ret, "GetConsoleScreenBufferInfoEx failed: error %lu\n", GetLastError());
    orig_attr = csbi.wAttributes;
    orig_popup = csbi.wPopupAttributes;

    hConOut2 = CreateConsoleScreenBuffer(GENERIC_READ|GENERIC_WRITE, 0, NULL,
                                         CONSOLE_TEXTMODE_BUFFER, NULL);
    ok(hConOut2 != INVALID_HANDLE_VALUE, "CreateConsoleScreenBuffer failed: error %lu\n", GetLastError());

    ret = GetConsoleScreenBufferInfoEx(hConOut2, &csbi2);
    ok(ret, "GetConsoleScreenBufferInfoEx failed: error %lu\n", GetLastError());
    CloseHandle(hConOut2);

    ok(csbi2.wAttributes == orig_attr, "Character Attributes should have been copied: "
       "got %#x, expected %#x\n", csbi2.wAttributes, orig_attr);
    ok(csbi2.wPopupAttributes != orig_popup, "Popup Attributes should not match original value\n");
    ok(csbi2.wPopupAttributes == orig_attr, "Popup Attributes should match Character Attributes\n");

    /* Test different Character Attributes */
    attr = FOREGROUND_BLUE|BACKGROUND_GREEN;
    ret = SetConsoleTextAttribute(hConOut, attr);
    ok(ret, "SetConsoleTextAttribute failed: error %lu\n", GetLastError());

    hConOut2 = CreateConsoleScreenBuffer(GENERIC_READ|GENERIC_WRITE, 0, NULL,
                                         CONSOLE_TEXTMODE_BUFFER, NULL);
    ok(hConOut2 != INVALID_HANDLE_VALUE, "CreateConsoleScreenBuffer failed: error %lu\n", GetLastError());

    memset(&csbi2, 0, sizeof(CONSOLE_SCREEN_BUFFER_INFOEX));
    csbi2.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

    ret = GetConsoleScreenBufferInfoEx(hConOut2, &csbi2);
    ok(ret, "GetConsoleScreenBufferInfoEx failed: error %lu\n", GetLastError());
    CloseHandle(hConOut2);

    ok(csbi2.wAttributes == attr, "Character Attributes should have been copied: "
       "got %#x, expected %#x\n", csbi2.wAttributes, attr);
    ok(csbi2.wPopupAttributes != orig_popup, "Popup Attributes should not match original value\n");
    ok(csbi2.wPopupAttributes == attr, "Popup Attributes should match Character Attributes\n");

    ret = SetConsoleTextAttribute(hConOut, orig_attr);
    ok(ret, "SetConsoleTextAttribute failed: error %lu\n", GetLastError());

    /* Test inheritance of different Popup Attributes */
    csbi.wPopupAttributes = attr;
    ret = SetConsoleScreenBufferInfoEx(hConOut, &csbi);
    ok(ret, "SetConsoleScreenBufferInfoEx failed: error %lu\n", GetLastError());

    hConOut2 = CreateConsoleScreenBuffer(GENERIC_READ|GENERIC_WRITE, 0, NULL,
                                         CONSOLE_TEXTMODE_BUFFER, NULL);
    ok(hConOut2 != INVALID_HANDLE_VALUE, "CreateConsoleScreenBuffer failed: error %lu\n", GetLastError());

    memset(&csbi2, 0, sizeof(CONSOLE_SCREEN_BUFFER_INFOEX));
    csbi2.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

    ret = GetConsoleScreenBufferInfoEx(hConOut2, &csbi2);
    ok(ret, "GetConsoleScreenBufferInfoEx failed: error %lu\n", GetLastError());
    CloseHandle(hConOut2);

    ok(csbi2.wAttributes == orig_attr, "Character Attributes should have been copied: "
       "got %#x, expected %#x\n", csbi2.wAttributes, orig_attr);
    ok(csbi2.wPopupAttributes != orig_popup, "Popup Attributes should not match original value\n");
    ok(csbi2.wPopupAttributes == orig_attr, "Popup Attributes should match Character Attributes\n");

    csbi.wPopupAttributes = orig_popup;
    ret = SetConsoleScreenBufferInfoEx(hConOut, &csbi);
    ok(ret, "SetConsoleScreenBufferInfoEx failed: error %lu\n", GetLastError());
}
#endif

static void CALLBACK signaled_function(void *p, BOOLEAN timeout)
{
    HANDLE event = p;
    SetEvent(event);
    ok(!timeout, "wait shouldn't have timed out\n");
}

static void testWaitForConsoleInput(HANDLE input_handle)
{
    HANDLE wait_handle;
    HANDLE complete_event;
    INPUT_RECORD record;
    DWORD events_written;
    DWORD wait_ret;
    BOOL ret;

    complete_event = CreateEventW(NULL, FALSE, FALSE, NULL);

    /* Test success case */
    ret = RegisterWaitForSingleObject(&wait_handle, input_handle, signaled_function, complete_event, INFINITE, WT_EXECUTEONLYONCE);
    ok(ret == TRUE, "Expected RegisterWaitForSingleObject to return TRUE, got %d\n", ret);
    /* give worker thread a chance to start up */
    Sleep(100);
    record.EventType = KEY_EVENT;
    record.Event.KeyEvent.bKeyDown = 1;
    record.Event.KeyEvent.wRepeatCount = 1;
    record.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
    record.Event.KeyEvent.wVirtualScanCode = VK_RETURN;
    record.Event.KeyEvent.uChar.UnicodeChar = '\r';
    record.Event.KeyEvent.dwControlKeyState = 0;
    ret = WriteConsoleInputW(input_handle, &record, 1, &events_written);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    wait_ret = WaitForSingleObject(complete_event, INFINITE);
    ok(wait_ret == WAIT_OBJECT_0, "Expected the handle to be signaled\n");
    ret = UnregisterWait(wait_handle);
    /* If the callback is still running, this fails with ERROR_IO_PENDING, but
       that's ok and expected. */
    ok(ret != 0 || GetLastError() == ERROR_IO_PENDING,
        "UnregisterWait failed with error %ld\n", GetLastError());

    /* Test timeout case */
    FlushConsoleInputBuffer(input_handle);
    ret = RegisterWaitForSingleObject(&wait_handle, input_handle, signaled_function, complete_event, INFINITE, WT_EXECUTEONLYONCE);
    wait_ret = WaitForSingleObject(complete_event, 100);
    ok(wait_ret == WAIT_TIMEOUT, "Expected the wait to time out\n");
    ret = UnregisterWait(wait_handle);
    ok(ret, "UnregisterWait failed with error %ld\n", GetLastError());

    /* Clean up */
    CloseHandle(complete_event);
}

static BOOL filter_spurious_event(HANDLE input)
{
    INPUT_RECORD ir;
    DWORD r;

    if (!ReadConsoleInputW(input, &ir, 1, &r) || r != 1) return FALSE;

    switch (ir.EventType)
    {
    case MOUSE_EVENT:
        if (ir.Event.MouseEvent.dwEventFlags == MOUSE_MOVED) return TRUE;
        ok(0, "Unexcepted mouse message: state=%lx ctrl=%lx flags=%lx (%u, %u)\n",
           ir.Event.MouseEvent.dwButtonState,
           ir.Event.MouseEvent.dwControlKeyState,
           ir.Event.MouseEvent.dwEventFlags,
           ir.Event.MouseEvent.dwMousePosition.X,
           ir.Event.MouseEvent.dwMousePosition.Y);
        break;
    case WINDOW_BUFFER_SIZE_EVENT:
        return TRUE;
    default:
        ok(0, "Unexpected message %u\n", ir.EventType);
    }
    return FALSE;
}

static void test_wait(HANDLE input, HANDLE orig_output)
{
    HANDLE output, unbound_output, unbound_input;
    LARGE_INTEGER zero;
    INPUT_RECORD ir;
    DWORD res, count;
    NTSTATUS status;
    BOOL ret;

    if (skip_nt) return;

    memset(&ir, 0, sizeof(ir));
    ir.EventType = MOUSE_EVENT;
    ir.Event.MouseEvent.dwEventFlags = MOUSE_MOVED;
    zero.QuadPart = 0;

    output = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                       CONSOLE_TEXTMODE_BUFFER, NULL);
    ok(output != INVALID_HANDLE_VALUE, "CreateConsoleScreenBuffer failed: %lu\n", GetLastError());

    ret = SetConsoleActiveScreenBuffer(output);
    ok(ret, "SetConsoleActiveScreenBuffer failed: %lu\n", GetLastError());
    ret = FlushConsoleInputBuffer(input);
    ok(ret, "FlushConsoleInputBuffer failed: %lu\n", GetLastError());

    unbound_output = create_unbound_handle(TRUE, TRUE);
    unbound_input = create_unbound_handle(FALSE, TRUE);

    while ((res = WaitForSingleObject(input, 0)) == WAIT_OBJECT_0)
    {
        if (!filter_spurious_event(input)) break;
    }
    ok(res == WAIT_TIMEOUT, "WaitForSingleObject returned %lx\n", res);
    while ((res = WaitForSingleObject(output, 0)) == WAIT_OBJECT_0)
    {
        if (!filter_spurious_event(input)) break;
    }
    ok(res == WAIT_TIMEOUT, "WaitForSingleObject returned %lx\n", res);
    while ((res = WaitForSingleObject(orig_output, 0)) == WAIT_OBJECT_0)
    {
        if (!filter_spurious_event(input)) break;
    }
    ok(res == WAIT_TIMEOUT, "WaitForSingleObject returned %lx\n", res);
    while ((res = WaitForSingleObject(unbound_output, 0)) == WAIT_OBJECT_0)
    {
        if (!filter_spurious_event(unbound_input)) break;
    }
    ok(res == WAIT_TIMEOUT, "WaitForSingleObject returned %lx\n", res);
    while ((res = WaitForSingleObject(unbound_input, 0)) == WAIT_OBJECT_0)
    {
        if (!filter_spurious_event(unbound_input)) break;
    }
    ok(res == WAIT_TIMEOUT, "WaitForSingleObject returned %lx\n", res);
    while ((status = NtWaitForSingleObject(input, FALSE, &zero)) == STATUS_SUCCESS)
    {
        if (!filter_spurious_event(input)) break;
    }
    ok(status == STATUS_TIMEOUT || broken(status == STATUS_ACCESS_DENIED /* win2k8 */),
       "NtWaitForSingleObject returned %lx\n", status);
    while ((status = NtWaitForSingleObject(output, FALSE, &zero)) == STATUS_SUCCESS)
    {
        if (!filter_spurious_event(input)) break;
    }
    ok(status == STATUS_TIMEOUT || broken(status == STATUS_ACCESS_DENIED /* win2k8 */),
       "NtWaitForSingleObject returned %lx\n", status);

    ret = WriteConsoleInputW(input, &ir, 1, &count);
    ok(ret, "WriteConsoleInputW failed: %lu\n", GetLastError());

    res = WaitForSingleObject(input, 0);
    ok(!res, "WaitForSingleObject returned %lx\n", res);
    res = WaitForSingleObject(output, 0);
    ok(!res, "WaitForSingleObject returned %lx\n", res);
    res = WaitForSingleObject(orig_output, 0);
    ok(!res, "WaitForSingleObject returned %lx\n", res);
    res = WaitForSingleObject(unbound_output, 0);
    ok(!res, "WaitForSingleObject returned %lx\n", res);
    res = WaitForSingleObject(unbound_input, 0);
    ok(!res, "WaitForSingleObject returned %lx\n", res);
    status = NtWaitForSingleObject(input, FALSE, &zero);
    ok(!status || broken(status == STATUS_ACCESS_DENIED /* win2k8 */),
       "NtWaitForSingleObject returned %lx\n", status);
    status = NtWaitForSingleObject(output, FALSE, &zero);
    ok(!status || broken(status == STATUS_ACCESS_DENIED /* win2k8 */),
       "NtWaitForSingleObject returned %lx\n", status);

    ret = SetConsoleActiveScreenBuffer(orig_output);
    ok(ret, "SetConsoleActiveScreenBuffer failed: %lu\n", GetLastError());

    CloseHandle(unbound_input);
    CloseHandle(unbound_output);
    CloseHandle(output);
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
    ok(error == ERROR_BUFFER_OVERFLOW, "got %lu expected ERROR_BUFFER_OVERFLOW\n", error);

    SetLastError(0xdeadbeef);
    ret = pGetConsoleInputExeNameA(0, buffer);
    error = GetLastError();
    ok(ret, "GetConsoleInputExeNameA failed\n");
    ok(error == ERROR_BUFFER_OVERFLOW, "got %lu expected ERROR_BUFFER_OVERFLOW\n", error);

    GetModuleFileNameA(GetModuleHandleA(NULL), module, sizeof(module));
    p = strrchr(module, '\\') + 1;

    ret = pGetConsoleInputExeNameA(ARRAY_SIZE(buffer), buffer);
    ok(ret, "GetConsoleInputExeNameA failed\n");
    todo_wine ok(!lstrcmpA(buffer, p), "got %s expected %s\n", buffer, p);

    SetLastError(0xdeadbeef);
    ret = pSetConsoleInputExeNameA(NULL);
    error = GetLastError();
    ok(!ret, "SetConsoleInputExeNameA failed\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %lu expected ERROR_INVALID_PARAMETER\n", error);

    SetLastError(0xdeadbeef);
    ret = pSetConsoleInputExeNameA("");
    error = GetLastError();
    ok(!ret, "SetConsoleInputExeNameA failed\n");
    ok(error == ERROR_INVALID_PARAMETER, "got %lu expected ERROR_INVALID_PARAMETER\n", error);

    ret = pSetConsoleInputExeNameA(input_exe);
    ok(ret, "SetConsoleInputExeNameA failed\n");

    ret = pGetConsoleInputExeNameA(ARRAY_SIZE(buffer), buffer);
    ok(ret, "GetConsoleInputExeNameA failed\n");
    ok(!lstrcmpA(buffer, input_exe), "got %s expected %s\n", buffer, input_exe);
}

static void test_GetConsoleProcessList(void)
{
    DWORD ret, *list = NULL;

    if (!pGetConsoleProcessList)
    {
        win_skip("GetConsoleProcessList is not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pGetConsoleProcessList(NULL, 0);
    ok(ret == 0, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %ld\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetConsoleProcessList(NULL, 1);
    ok(ret == 0, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %ld\n",
       GetLastError());

    /* We should only have 1 process but only for these specific unit tests as
     * we created our own console. An AttachConsole(ATTACH_PARENT_PROCESS)
     * gives us two processes - see test_AttachConsole.
     */
    list = HeapAlloc(GetProcessHeap(), 0, sizeof(DWORD));

    SetLastError(0xdeadbeef);
    ret = pGetConsoleProcessList(list, 0);
    ok(ret == 0, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER,
       "Expected ERROR_INVALID_PARAMETER, got %ld\n",
       GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetConsoleProcessList(list, 1);
    ok(ret == 1, "Expected 1, got %ld\n", ret);

    HeapFree(GetProcessHeap(), 0, list);

    list = HeapAlloc(GetProcessHeap(), 0, ret * sizeof(DWORD));

    SetLastError(0xdeadbeef);
    ret = pGetConsoleProcessList(list, ret);
    ok(ret == 1, "Expected 1, got %ld\n", ret);

    if (ret == 1)
    {
        DWORD pid = GetCurrentProcessId();
        ok(list[0] == pid, "Expected %ld, got %ld\n", pid, list[0]);
    }

    HeapFree(GetProcessHeap(), 0, list);
}

static void test_OpenCON(void)
{
    static const WCHAR conW[] = {'C','O','N',0};
    static const DWORD accesses[] = {CREATE_NEW, CREATE_ALWAYS, OPEN_EXISTING,
                                     OPEN_ALWAYS, TRUNCATE_EXISTING};
    unsigned            i;
    HANDLE              h;

    for (i = 0; i < ARRAY_SIZE(accesses); i++)
    {
        h = CreateFileW(conW, GENERIC_WRITE, 0, NULL, accesses[i], 0, NULL);
        ok(h != INVALID_HANDLE_VALUE || broken(accesses[i] == TRUNCATE_EXISTING /* Win8 */),
           "Expected to open the CON device on write (%lx)\n", accesses[i]);
        CloseHandle(h);

        h = CreateFileW(conW, GENERIC_READ, 0, NULL, accesses[i], 0, NULL);
        /* Windows versions differ here:
         * MSDN states in CreateFile that TRUNCATE_EXISTING requires GENERIC_WRITE
         * NT, XP, Vista comply, but Win7 doesn't and allows opening CON with TRUNCATE_EXISTING
         * So don't test when disposition is TRUNCATE_EXISTING
         */
        ok(h != INVALID_HANDLE_VALUE || broken(accesses[i] == TRUNCATE_EXISTING /* Win7+ */),
           "Expected to open the CON device on read (%lx)\n", accesses[i]);
        CloseHandle(h);
        h = CreateFileW(conW, GENERIC_READ|GENERIC_WRITE, 0, NULL, accesses[i], 0, NULL);
        ok(h == INVALID_HANDLE_VALUE, "Expected not to open the CON device on read-write (%lx)\n", accesses[i]);
        ok(GetLastError() == ERROR_FILE_NOT_FOUND || GetLastError() == ERROR_INVALID_PARAMETER,
           "Unexpected error %lx\n", GetLastError());
    }
}

static void test_OpenConsoleW(void)
{
    static const WCHAR coninW[] = {'C','O','N','I','N','$',0};
    static const WCHAR conoutW[] = {'C','O','N','O','U','T','$',0};
    static const WCHAR emptyW[] = {0};
    static const WCHAR invalidW[] = {'I','N','V','A','L','I','D',0};
    DWORD gle;

    static const struct
    {
        LPCWSTR name;
        DWORD access;
        BOOL inherit;
        DWORD creation;
        DWORD gle, gle2;
    } invalid_table[] = {
        {NULL,     0,                            FALSE,      0,                 ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {NULL,     0,                            FALSE,      0xdeadbeef,        ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {NULL,     0xdeadbeef,                   FALSE,      0,                 ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {NULL,     0xdeadbeef,                   TRUE,       0xdeadbeef,        ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {NULL,     0,                            FALSE,      OPEN_ALWAYS,       ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {NULL,     GENERIC_READ | GENERIC_WRITE, FALSE,      0,                 ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {NULL,     GENERIC_READ | GENERIC_WRITE, FALSE,      OPEN_ALWAYS,       ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {NULL,     GENERIC_READ | GENERIC_WRITE, FALSE,      OPEN_EXISTING,     ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {emptyW,   0,                            FALSE,      0,                 ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {emptyW,   0,                            FALSE,      0xdeadbeef,        ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {emptyW,   0xdeadbeef,                   FALSE,      0,                 ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {emptyW,   0xdeadbeef,                   TRUE,       0xdeadbeef,        ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {emptyW,   0,                            FALSE,      OPEN_ALWAYS,       ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {emptyW,   GENERIC_READ | GENERIC_WRITE, FALSE,      0,                 ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {emptyW,   GENERIC_READ | GENERIC_WRITE, FALSE,      OPEN_ALWAYS,       ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {emptyW,   GENERIC_READ | GENERIC_WRITE, FALSE,      OPEN_EXISTING,     ERROR_INVALID_PARAMETER, ERROR_PATH_NOT_FOUND},
        {invalidW, 0,                            FALSE,      0,                 ERROR_INVALID_PARAMETER, ERROR_FILE_NOT_FOUND},
        {invalidW, 0,                            FALSE,      0xdeadbeef,        ERROR_INVALID_PARAMETER, 0},
        {invalidW, 0xdeadbeef,                   FALSE,      0,                 ERROR_INVALID_PARAMETER, ERROR_FILE_NOT_FOUND},
        {invalidW, 0xdeadbeef,                   TRUE,       0xdeadbeef,        ERROR_INVALID_PARAMETER, 0},
        {invalidW, 0,                            FALSE,      OPEN_ALWAYS,       ERROR_INVALID_PARAMETER, ERROR_FILE_NOT_FOUND},
        {invalidW, GENERIC_READ | GENERIC_WRITE, FALSE,      0,                 ERROR_INVALID_PARAMETER, ERROR_FILE_NOT_FOUND},
        {invalidW, GENERIC_READ | GENERIC_WRITE, FALSE,      OPEN_ALWAYS,       ERROR_INVALID_PARAMETER, ERROR_FILE_NOT_FOUND},
        {invalidW, GENERIC_READ | GENERIC_WRITE, FALSE,      OPEN_EXISTING,     ERROR_INVALID_PARAMETER, ERROR_FILE_NOT_FOUND},
        {coninW,   0,                            FALSE,      0xdeadbeef,        ERROR_INVALID_PARAMETER, 0},
        {coninW,   0xdeadbeef,                   FALSE,      0,                 ERROR_INVALID_PARAMETER, ERROR_ACCESS_DENIED},
        {coninW,   0xdeadbeef,                   TRUE,       0xdeadbeef,        ERROR_INVALID_PARAMETER, 0},
        {conoutW,  0,                            FALSE,      0xdeadbeef,        ERROR_INVALID_PARAMETER, 0},
        {conoutW,  0xceadbeef,                   FALSE,      0,                 ERROR_INVALID_PARAMETER, ERROR_ACCESS_DENIED},
        {conoutW,  0xdeadbeef,                   TRUE,       0xdeadbeef,        ERROR_INVALID_PARAMETER, 0},
    };
    static const struct
    {
        LPCWSTR name;
        DWORD access;
        BOOL inherit;
        DWORD creation;
    } valid_table[] = {
        {coninW,   0,                            FALSE,      0                },
        {coninW,   0,                            TRUE,       0                },
        {coninW,   GENERIC_EXECUTE,              TRUE,       0                },
        {coninW,   GENERIC_ALL,                  TRUE,       0                },
        {coninW,   0,                            FALSE,      OPEN_ALWAYS      },
        {coninW,   GENERIC_READ | GENERIC_WRITE, FALSE,      0                },
        {coninW,   GENERIC_READ | GENERIC_WRITE, FALSE,      CREATE_NEW       },
        {coninW,   GENERIC_READ | GENERIC_WRITE, FALSE,      CREATE_ALWAYS    },
        {coninW,   GENERIC_READ | GENERIC_WRITE, FALSE,      OPEN_ALWAYS      },
        {coninW,   GENERIC_READ | GENERIC_WRITE, FALSE,      TRUNCATE_EXISTING},
        {conoutW,  0,                            FALSE,      0                },
        {conoutW,  0,                            FALSE,      OPEN_ALWAYS      },
        {conoutW,  GENERIC_READ | GENERIC_WRITE, FALSE,      0                },
        {conoutW,  GENERIC_READ | GENERIC_WRITE, FALSE,      CREATE_NEW,      },
        {conoutW,  GENERIC_READ | GENERIC_WRITE, FALSE,      CREATE_ALWAYS    },
        {conoutW,  GENERIC_READ | GENERIC_WRITE, FALSE,      OPEN_ALWAYS      },
        {conoutW,  GENERIC_READ | GENERIC_WRITE, FALSE,      TRUNCATE_EXISTING},
    };

    int index;
    HANDLE ret;

    if (!pOpenConsoleW)
    {
        win_skip("OpenConsoleW is not available\n");
        return;
    }

    for (index = 0; index < ARRAY_SIZE(invalid_table); index++)
    {
        SetLastError(0xdeadbeef);
        ret = pOpenConsoleW(invalid_table[index].name, invalid_table[index].access,
                            invalid_table[index].inherit, invalid_table[index].creation);
        gle = GetLastError();
        ok(ret == INVALID_HANDLE_VALUE,
           "Expected OpenConsoleW to return INVALID_HANDLE_VALUE for index %d, got %p\n",
           index, ret);
        ok(gle == invalid_table[index].gle || (gle != 0 && gle == invalid_table[index].gle2),
           "Expected GetLastError() to return %lu/%lu for index %d, got %lu\n",
           invalid_table[index].gle, invalid_table[index].gle2, index, gle);
    }

    for (index = 0; index < ARRAY_SIZE(valid_table); index++)
    {
        ret = pOpenConsoleW(valid_table[index].name, valid_table[index].access,
                            valid_table[index].inherit, valid_table[index].creation);
        todo_wine
        ok(ret != INVALID_HANDLE_VALUE || broken(ret == INVALID_HANDLE_VALUE /* until Win7 */),
           "Expected OpenConsoleW to succeed for index %d, got %p\n", index, ret);
        if (ret != INVALID_HANDLE_VALUE)
            CloseHandle(ret);
    }

    ret = pOpenConsoleW(coninW, GENERIC_READ | GENERIC_WRITE, FALSE, OPEN_EXISTING);
    ok(ret != INVALID_HANDLE_VALUE, "Expected OpenConsoleW to return a valid handle\n");
    if (ret != INVALID_HANDLE_VALUE)
        CloseHandle(ret);

    ret = pOpenConsoleW(conoutW, GENERIC_READ | GENERIC_WRITE, FALSE, OPEN_EXISTING);
    ok(ret != INVALID_HANDLE_VALUE, "Expected OpenConsoleW to return a valid handle\n");
    if (ret != INVALID_HANDLE_VALUE)
        CloseHandle(ret);
}

static void test_CreateFileW(void)
{
    static const struct
    {
        BOOL input;
        DWORD access;
        BOOL inherit;
        DWORD creation;
        DWORD gle;
        BOOL is_broken;
    } cf_table[] = {
        {TRUE,   0,                            FALSE,      OPEN_ALWAYS,       0,                              FALSE},
        {TRUE,   GENERIC_READ | GENERIC_WRITE, FALSE,      0,                 ERROR_INVALID_PARAMETER,        TRUE},
        {TRUE,   0,                            FALSE,      0,                 ERROR_INVALID_PARAMETER,        TRUE},
        {TRUE,   GENERIC_READ | GENERIC_WRITE, FALSE,      CREATE_NEW,        0,                              FALSE},
        {TRUE,   GENERIC_READ | GENERIC_WRITE, FALSE,      CREATE_ALWAYS,     0,                              FALSE},
        {TRUE,   GENERIC_READ | GENERIC_WRITE, FALSE,      OPEN_ALWAYS,       0,                              FALSE},
        {FALSE,  0,                            FALSE,      0,                 ERROR_INVALID_PARAMETER,        TRUE},
        {FALSE,  0,                            FALSE,      OPEN_ALWAYS,       0,                              FALSE},
        {FALSE,  GENERIC_READ | GENERIC_WRITE, FALSE,      0,                 ERROR_INVALID_PARAMETER,        TRUE},
        {FALSE,  GENERIC_READ | GENERIC_WRITE, FALSE,      CREATE_NEW,        0,                              FALSE},
        {FALSE,  GENERIC_READ | GENERIC_WRITE, FALSE,      CREATE_ALWAYS,     0,                              FALSE},
        {FALSE,  GENERIC_READ | GENERIC_WRITE, FALSE,      OPEN_ALWAYS,       0,                              FALSE},
        /* TRUNCATE_EXISTING is forbidden starting with Windows 8 */
    };

    static const UINT nt_disposition[5] =
    {
        FILE_CREATE,        /* CREATE_NEW */
        FILE_OVERWRITE_IF,  /* CREATE_ALWAYS */
        FILE_OPEN,          /* OPEN_EXISTING */
        FILE_OPEN_IF,       /* OPEN_ALWAYS */
        FILE_OVERWRITE      /* TRUNCATE_EXISTING */
    };

    int index;
    HANDLE ret;
    SECURITY_ATTRIBUTES sa;
    OBJECT_ATTRIBUTES attr = {sizeof(attr)};
    UNICODE_STRING string;
    IO_STATUS_BLOCK iosb;
    NTSTATUS status;

    for (index = 0; index < ARRAY_SIZE(cf_table); index++)
    {
        SetLastError(0xdeadbeef);

        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = cf_table[index].inherit;

        ret = CreateFileW(cf_table[index].input ? L"CONIN$" : L"CONOUT$", cf_table[index].access,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, &sa,
                          cf_table[index].creation, FILE_ATTRIBUTE_NORMAL, NULL);
        if (ret == INVALID_HANDLE_VALUE)
        {
            ok(cf_table[index].gle,
               "Expected CreateFileW not to return INVALID_HANDLE_VALUE for index %d\n", index);
            ok(GetLastError() == cf_table[index].gle,
                "Expected GetLastError() to return %lu for index %d, got %lu\n",
                cf_table[index].gle, index, GetLastError());
        }
        else
        {
            ok(!cf_table[index].gle || broken(cf_table[index].is_broken) /* Win7 */,
               "Expected CreateFileW to succeed for index %d\n", index);
            CloseHandle(ret);
        }

        if (skip_nt) continue;

        SetLastError(0xdeadbeef);

        sa.nLength = sizeof(sa);
        sa.lpSecurityDescriptor = NULL;
        sa.bInheritHandle = cf_table[index].inherit;

        ret = CreateFileW(cf_table[index].input ? L"\\??\\CONIN$" : L"\\??\\CONOUT$", cf_table[index].access,
                          FILE_SHARE_READ|FILE_SHARE_WRITE, &sa,
                          cf_table[index].creation, FILE_ATTRIBUTE_NORMAL, NULL);
        if (cf_table[index].gle)
            ok(ret == INVALID_HANDLE_VALUE && GetLastError() == cf_table[index].gle,
               "CreateFileW to returned %p %lu for index %d\n", ret, GetLastError(), index);
        else
            ok(ret != INVALID_HANDLE_VALUE && (!cf_table[index].gle || broken(cf_table[index].is_broken) /* Win7 */),
               "CreateFileW to returned %p %lu for index %d\n", ret, GetLastError(), index);
        if (ret != INVALID_HANDLE_VALUE) CloseHandle(ret);

        if (cf_table[index].gle) continue;

        RtlInitUnicodeString(&string, cf_table[index].input
                             ? L"\\Device\\ConDrv\\CurrentIn" : L"\\Device\\ConDrv\\CurrentOut");
        attr.ObjectName = &string;
        status = NtCreateFile(&ret, cf_table[index].access | SYNCHRONIZE | FILE_READ_ATTRIBUTES, &attr, &iosb, NULL,
                              FILE_ATTRIBUTE_NORMAL, 0, nt_disposition[cf_table[index].creation - CREATE_NEW],
                              FILE_NON_DIRECTORY_FILE, NULL, 0);
        ok(!status, "NtCreateFile failed %lx for %u\n", status, index);
        CloseHandle(ret);

        RtlInitUnicodeString(&string, cf_table[index].input ? L"\\??\\CONIN$" : L"\\??\\CONOUT$");
        attr.ObjectName = &string;
        status = NtCreateFile(&ret, cf_table[index].access | SYNCHRONIZE | FILE_READ_ATTRIBUTES, &attr, &iosb, NULL,
                              FILE_ATTRIBUTE_NORMAL, 0, nt_disposition[cf_table[index].creation - CREATE_NEW],
                              FILE_NON_DIRECTORY_FILE, NULL, 0);
        ok(!status, "NtCreateFile failed %lx for %u\n", status, index);
        CloseHandle(ret);
    }
}

static void test_VerifyConsoleIoHandle( HANDLE handle )
{
    BOOL ret;
    DWORD error;

    if (!pVerifyConsoleIoHandle)
    {
        win_skip("VerifyConsoleIoHandle is not available\n");
        return;
    }

    /* invalid handle */
    SetLastError(0xdeadbeef);
    ret = pVerifyConsoleIoHandle((HANDLE)0xdeadbee0);
    error = GetLastError();
    ok(!ret, "expected VerifyConsoleIoHandle to fail\n");
    ok(error == 0xdeadbeef, "wrong GetLastError() %ld\n", error);

    /* invalid handle + 1 */
    SetLastError(0xdeadbeef);
    ret = pVerifyConsoleIoHandle((HANDLE)0xdeadbee1);
    error = GetLastError();
    ok(!ret, "expected VerifyConsoleIoHandle to fail\n");
    ok(error == 0xdeadbeef, "wrong GetLastError() %ld\n", error);

    /* invalid handle + 2 */
    SetLastError(0xdeadbeef);
    ret = pVerifyConsoleIoHandle((HANDLE)0xdeadbee2);
    error = GetLastError();
    ok(!ret, "expected VerifyConsoleIoHandle to fail\n");
    ok(error == 0xdeadbeef, "wrong GetLastError() %ld\n", error);

    /* invalid handle + 3 */
    SetLastError(0xdeadbeef);
    ret = pVerifyConsoleIoHandle((HANDLE)0xdeadbee3);
    error = GetLastError();
    ok(!ret, "expected VerifyConsoleIoHandle to fail\n");
    ok(error == 0xdeadbeef, "wrong GetLastError() %ld\n", error);

    /* valid handle */
    SetLastError(0xdeadbeef);
    ret = pVerifyConsoleIoHandle(handle);
    error = GetLastError();
    ok(ret ||
       broken(!ret), /* Windows 8 and 10 */
       "expected VerifyConsoleIoHandle to succeed\n");
    ok(error == 0xdeadbeef, "wrong GetLastError() %ld\n", error);
}

static void test_GetSetStdHandle(void)
{
    HANDLE handle;
    DWORD error;
    BOOL ret;

    /* get invalid std handle */
    SetLastError(0xdeadbeef);
    handle = GetStdHandle(42);
    error = GetLastError();
    ok(error == ERROR_INVALID_HANDLE || broken(error == ERROR_INVALID_FUNCTION)/* Win9x */,
       "wrong GetLastError() %ld\n", error);
    ok(handle == INVALID_HANDLE_VALUE, "expected INVALID_HANDLE_VALUE\n");

    /* get valid */
    SetLastError(0xdeadbeef);
    handle = GetStdHandle(STD_INPUT_HANDLE);
    error = GetLastError();
    ok(error == 0xdeadbeef, "wrong GetLastError() %ld\n", error);

    /* set invalid std handle */
    SetLastError(0xdeadbeef);
    ret = SetStdHandle(42, handle);
    error = GetLastError();
    ok(!ret, "expected SetStdHandle to fail\n");
    ok(error == ERROR_INVALID_HANDLE || broken(error == ERROR_INVALID_FUNCTION)/* Win9x */,
       "wrong GetLastError() %ld\n", error);

    /* set valid (restore old value) */
    SetLastError(0xdeadbeef);
    ret = SetStdHandle(STD_INPUT_HANDLE, handle);
    error = GetLastError();
    ok(ret, "expected SetStdHandle to succeed\n");
    ok(error == 0xdeadbeef, "wrong GetLastError() %ld\n", error);
}

static void test_DuplicateConsoleHandle(void)
{
    HANDLE handle, event;
    BOOL ret;

    if (skip_nt) return;

    event = CreateEventW(NULL, TRUE, FALSE, NULL);

    /* duplicate an event handle with DuplicateConsoleHandle */
    handle = DuplicateConsoleHandle(event, 0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(handle != NULL, "DuplicateConsoleHandle failed: %lu\n", GetLastError());

    ret = SetEvent(handle);
    ok(ret, "SetEvent failed: %lu\n", GetLastError());

    ret = CloseConsoleHandle(handle);
    ok(ret, "CloseConsoleHandle failed: %lu\n", GetLastError());
    ret = CloseConsoleHandle(event);
    ok(ret, "CloseConsoleHandle failed: %lu\n", GetLastError());

    handle = DuplicateConsoleHandle((HANDLE)0xdeadbeef, 0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(handle == INVALID_HANDLE_VALUE, "DuplicateConsoleHandle failed: %lu\n", GetLastError());
    ok(GetLastError() == ERROR_INVALID_HANDLE, "last error = %lu\n", GetLastError());
}

static void test_GetNumberOfConsoleInputEvents(HANDLE input_handle)
{
    DWORD count;
    BOOL ret;
    int i;

    const struct
    {
        HANDLE handle;
        LPDWORD nrofevents;
        DWORD last_error;
    } invalid_table[] =
    {
        {NULL,                 NULL,   ERROR_INVALID_HANDLE},
        {NULL,                 &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL,   ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, &count, ERROR_INVALID_HANDLE},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_table); i++)
    {
        SetLastError(0xdeadbeef);
        if (invalid_table[i].nrofevents) count = 0xdeadbeef;
        ret = GetNumberOfConsoleInputEvents(invalid_table[i].handle,
                                            invalid_table[i].nrofevents);
        ok(!ret, "[%d] Expected GetNumberOfConsoleInputEvents to return FALSE, got %d\n", i, ret);
        if (invalid_table[i].nrofevents)
        {
            ok(count == 0xdeadbeef,
               "[%d] Expected output count to be unmodified, got %lu\n", i, count);
        }
        ok(GetLastError() == invalid_table[i].last_error,
           "[%d] Expected last error to be %lu, got %lu\n",
           i, invalid_table[i].last_error, GetLastError());
    }

    /* Test crashes on Windows 7. */
    if (0)
    {
        SetLastError(0xdeadbeef);
        ret = GetNumberOfConsoleInputEvents(input_handle, NULL);
        ok(!ret, "Expected GetNumberOfConsoleInputEvents to return FALSE, got %d\n", ret);
        ok(GetLastError() == ERROR_INVALID_ACCESS,
           "Expected last error to be ERROR_INVALID_ACCESS, got %lu\n",
           GetLastError());
    }

    count = 0xdeadbeef;
    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count != 0xdeadbeef, "Expected output count to initialized\n");
}

static void test_WriteConsoleInputA(HANDLE input_handle)
{
    INPUT_RECORD event;
    INPUT_RECORD event_list[5];
    MOUSE_EVENT_RECORD mouse_event = { {0, 0}, 0, 0, MOUSE_MOVED };
    KEY_EVENT_RECORD key_event;
    DWORD count, console_mode, gle;
    BOOL ret;
    int i;

    const struct
    {
        HANDLE handle;
        const INPUT_RECORD *buffer;
        DWORD count;
        LPDWORD written;
        DWORD gle, gle2;
        int win_crash;
    } invalid_table[] =
    {
        {NULL, NULL, 0, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {NULL, NULL, 0, &count,ERROR_INVALID_HANDLE},
        {NULL, NULL, 1, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {NULL, NULL, 1, &count, ERROR_NOACCESS, ERROR_INVALID_ACCESS},
        {NULL, &event, 0, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {NULL, &event, 0, &count, ERROR_INVALID_HANDLE},
        {NULL, &event, 1, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {NULL, &event, 1, &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 0, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {INVALID_HANDLE_VALUE, NULL, 0, &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 1, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {INVALID_HANDLE_VALUE, NULL, 1, &count, ERROR_INVALID_HANDLE, ERROR_INVALID_ACCESS},
        {INVALID_HANDLE_VALUE, &event, 0, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {INVALID_HANDLE_VALUE, &event, 0, &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, &event, 1, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {INVALID_HANDLE_VALUE, &event, 1, &count, ERROR_INVALID_HANDLE},
        {input_handle, NULL, 0, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {input_handle, NULL, 1, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {input_handle, NULL, 1, &count, ERROR_NOACCESS, ERROR_INVALID_ACCESS},
        {input_handle, &event, 0, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {input_handle, &event, 1, NULL, ERROR_INVALID_ACCESS, 0, 1},
    };

    /* Suppress external sources of input events for the duration of the test. */
    ret = GetConsoleMode(input_handle, &console_mode);
    ok(ret == TRUE, "Expected GetConsoleMode to return TRUE, got %d\n", ret);
    if (!ret)
    {
        skip("GetConsoleMode failed with last error %lu\n", GetLastError());
        return;
    }

    ret = SetConsoleMode(input_handle, console_mode & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
    ok(ret == TRUE, "Expected SetConsoleMode to return TRUE, got %d\n", ret);
    if (!ret)
    {
        skip("SetConsoleMode failed with last error %lu\n", GetLastError());
        return;
    }

    /* Discard any events queued before the tests. */
    ret = FlushConsoleInputBuffer(input_handle);
    ok(ret == TRUE, "Expected FlushConsoleInputBuffer to return TRUE, got %d\n", ret);

    event.EventType = MOUSE_EVENT;
    event.Event.MouseEvent = mouse_event;

    for (i = 0; i < ARRAY_SIZE(invalid_table); i++)
    {
        if (invalid_table[i].win_crash)
            continue;

        SetLastError(0xdeadbeef);
        if (invalid_table[i].written) count = 0xdeadbeef;
        ret = WriteConsoleInputA(invalid_table[i].handle,
                                 invalid_table[i].buffer,
                                 invalid_table[i].count,
                                 invalid_table[i].written);
        ok(!ret, "[%d] Expected WriteConsoleInputA to return FALSE, got %d\n", i, ret);
        gle = GetLastError();
        ok(gle == invalid_table[i].gle || (gle != 0 && gle == invalid_table[i].gle2),
           "[%d] Expected last error to be %lu or %lu, got %lu\n",
           i, invalid_table[i].gle, invalid_table[i].gle2, gle);
    }

    count = 0xdeadbeef;
    ret = WriteConsoleInputA(input_handle, NULL, 0, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = WriteConsoleInputA(input_handle, &event, 0, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = WriteConsoleInputA(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = FlushConsoleInputBuffer(input_handle);
    ok(ret == TRUE, "Expected FlushConsoleInputBuffer to return TRUE, got %d\n", ret);

    /* Writing a single mouse event doesn't seem to affect the count if an adjacent mouse event is already queued. */
    event.EventType = MOUSE_EVENT;
    event.Event.MouseEvent = mouse_event;

    ret = WriteConsoleInputA(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = WriteConsoleInputA(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    todo_wine
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = FlushConsoleInputBuffer(input_handle);
    ok(ret == TRUE, "Expected FlushConsoleInputBuffer to return TRUE, got %d\n", ret);

    for (i = 0; i < ARRAY_SIZE(event_list); i++)
    {
        event_list[i].EventType = MOUSE_EVENT;
        event_list[i].Event.MouseEvent = mouse_event;
    }

    /* Writing consecutive chunks of mouse events appears to work. */
    ret = WriteConsoleInputA(input_handle, event_list, ARRAY_SIZE(event_list), &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == ARRAY_SIZE(event_list),
       "Expected count to be event list length, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == ARRAY_SIZE(event_list),
       "Expected count to be event list length, got %lu\n", count);

    ret = WriteConsoleInputA(input_handle, event_list, ARRAY_SIZE(event_list), &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == ARRAY_SIZE(event_list),
       "Expected count to be event list length, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 2*ARRAY_SIZE(event_list),
       "Expected count to be twice event list length, got %lu\n", count);

    /* Again, writing a single mouse event with adjacent mouse events queued doesn't appear to affect the count. */
    ret = WriteConsoleInputA(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    todo_wine
    ok(count == 2*ARRAY_SIZE(event_list),
       "Expected count to be twice event list length, got %lu\n", count);

    ret = FlushConsoleInputBuffer(input_handle);
    ok(ret == TRUE, "Expected FlushConsoleInputBuffer to return TRUE, got %d\n", ret);

    key_event.bKeyDown = FALSE;
    key_event.wRepeatCount = 0;
    key_event.wVirtualKeyCode = VK_SPACE;
    key_event.wVirtualScanCode = VK_SPACE;
    key_event.uChar.AsciiChar = ' ';
    key_event.dwControlKeyState = 0;

    event.EventType = KEY_EVENT;
    event.Event.KeyEvent = key_event;

    /* Key events don't exhibit the same behavior as mouse events. */
    ret = WriteConsoleInputA(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = WriteConsoleInputA(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 2, "Expected count to be 2, got %lu\n", count);

    ret = FlushConsoleInputBuffer(input_handle);
    ok(ret == TRUE, "Expected FlushConsoleInputBuffer to return TRUE, got %d\n", ret);

    /* Try interleaving mouse and key events. */
    event.EventType = MOUSE_EVENT;
    event.Event.MouseEvent = mouse_event;

    ret = WriteConsoleInputA(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    event.EventType = KEY_EVENT;
    event.Event.KeyEvent = key_event;

    ret = WriteConsoleInputA(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 2, "Expected count to be 2, got %lu\n", count);

    event.EventType = MOUSE_EVENT;
    event.Event.MouseEvent = mouse_event;

    ret = WriteConsoleInputA(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputA to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 3, "Expected count to be 3, got %lu\n", count);

    /* Restore the old console mode. */
    ret = SetConsoleMode(input_handle, console_mode);
    ok(ret == TRUE, "Expected SetConsoleMode to return TRUE, got %d\n", ret);
}

static void test_WriteConsoleInputW(HANDLE input_handle)
{
    INPUT_RECORD event;
    INPUT_RECORD event_list[5];
    MOUSE_EVENT_RECORD mouse_event = { {0, 0}, 0, 0, MOUSE_MOVED };
    KEY_EVENT_RECORD key_event;
    DWORD count, console_mode, gle;
    BOOL ret;
    int i;

    const struct
    {
        HANDLE handle;
        const INPUT_RECORD *buffer;
        DWORD count;
        LPDWORD written;
        DWORD gle, gle2;
        int win_crash;
    } invalid_table[] =
    {
        {NULL, NULL, 0, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {NULL, NULL, 0, &count, ERROR_INVALID_HANDLE},
        {NULL, NULL, 1, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {NULL, NULL, 1, &count, ERROR_NOACCESS, ERROR_INVALID_ACCESS},
        {NULL, &event, 0, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {NULL, &event, 0, &count, ERROR_INVALID_HANDLE},
        {NULL, &event, 1, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {NULL, &event, 1, &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 0, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {INVALID_HANDLE_VALUE, NULL, 0, &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 1, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {INVALID_HANDLE_VALUE, NULL, 1, &count, ERROR_INVALID_HANDLE, ERROR_INVALID_ACCESS},
        {INVALID_HANDLE_VALUE, &event, 0, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {INVALID_HANDLE_VALUE, &event, 0, &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, &event, 1, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {INVALID_HANDLE_VALUE, &event, 1, &count, ERROR_INVALID_HANDLE},
        {input_handle, NULL, 0, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {input_handle, NULL, 1, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {input_handle, NULL, 1, &count, ERROR_NOACCESS, ERROR_INVALID_ACCESS},
        {input_handle, &event, 0, NULL, ERROR_INVALID_ACCESS, 0, 1},
        {input_handle, &event, 1, NULL, ERROR_INVALID_ACCESS, 0, 1},
    };

    /* Suppress external sources of input events for the duration of the test. */
    ret = GetConsoleMode(input_handle, &console_mode);
    ok(ret == TRUE, "Expected GetConsoleMode to return TRUE, got %d\n", ret);
    if (!ret)
    {
        skip("GetConsoleMode failed with last error %lu\n", GetLastError());
        return;
    }

    ret = SetConsoleMode(input_handle, console_mode & ~(ENABLE_MOUSE_INPUT | ENABLE_WINDOW_INPUT));
    ok(ret == TRUE, "Expected SetConsoleMode to return TRUE, got %d\n", ret);
    if (!ret)
    {
        skip("SetConsoleMode failed with last error %lu\n", GetLastError());
        return;
    }

    /* Discard any events queued before the tests. */
    ret = FlushConsoleInputBuffer(input_handle);
    ok(ret == TRUE, "Expected FlushConsoleInputBuffer to return TRUE, got %d\n", ret);

    event.EventType = MOUSE_EVENT;
    event.Event.MouseEvent = mouse_event;

    for (i = 0; i < ARRAY_SIZE(invalid_table); i++)
    {
        if (invalid_table[i].win_crash)
            continue;

        SetLastError(0xdeadbeef);
        if (invalid_table[i].written) count = 0xdeadbeef;
        ret = WriteConsoleInputW(invalid_table[i].handle,
                                 invalid_table[i].buffer,
                                 invalid_table[i].count,
                                 invalid_table[i].written);
        ok(!ret, "[%d] Expected WriteConsoleInputW to return FALSE, got %d\n", i, ret);
        gle = GetLastError();
        ok(gle == invalid_table[i].gle || (gle != 0 && gle == invalid_table[i].gle2),
           "[%d] Expected last error to be %lu or %lu, got %lu\n",
           i, invalid_table[i].gle, invalid_table[i].gle2, gle);
    }

    count = 0xdeadbeef;
    ret = WriteConsoleInputW(input_handle, NULL, 0, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = WriteConsoleInputW(input_handle, &event, 0, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = WriteConsoleInputW(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = FlushConsoleInputBuffer(input_handle);
    ok(ret == TRUE, "Expected FlushConsoleInputBuffer to return TRUE, got %d\n", ret);

    /* Writing a single mouse event doesn't seem to affect the count if an adjacent mouse event is already queued. */
    event.EventType = MOUSE_EVENT;
    event.Event.MouseEvent = mouse_event;

    ret = WriteConsoleInputW(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = WriteConsoleInputW(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    todo_wine
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = FlushConsoleInputBuffer(input_handle);
    ok(ret == TRUE, "Expected FlushConsoleInputBuffer to return TRUE, got %d\n", ret);

    for (i = 0; i < ARRAY_SIZE(event_list); i++)
    {
        event_list[i].EventType = MOUSE_EVENT;
        event_list[i].Event.MouseEvent = mouse_event;
    }

    /* Writing consecutive chunks of mouse events appears to work. */
    ret = WriteConsoleInputW(input_handle, event_list, ARRAY_SIZE(event_list), &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == ARRAY_SIZE(event_list),
       "Expected count to be event list length, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == ARRAY_SIZE(event_list),
       "Expected count to be event list length, got %lu\n", count);

    ret = WriteConsoleInputW(input_handle, event_list, ARRAY_SIZE(event_list), &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == ARRAY_SIZE(event_list),
       "Expected count to be event list length, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 2*ARRAY_SIZE(event_list),
       "Expected count to be twice event list length, got %lu\n", count);

    /* Again, writing a single mouse event with adjacent mouse events queued doesn't appear to affect the count. */
    ret = WriteConsoleInputW(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    todo_wine
    ok(count == 2*ARRAY_SIZE(event_list),
       "Expected count to be twice event list length, got %lu\n", count);

    ret = FlushConsoleInputBuffer(input_handle);
    ok(ret == TRUE, "Expected FlushConsoleInputBuffer to return TRUE, got %d\n", ret);

    key_event.bKeyDown = FALSE;
    key_event.wRepeatCount = 0;
    key_event.wVirtualKeyCode = VK_SPACE;
    key_event.wVirtualScanCode = VK_SPACE;
    key_event.uChar.UnicodeChar = ' ';
    key_event.dwControlKeyState = 0;

    event.EventType = KEY_EVENT;
    event.Event.KeyEvent = key_event;

    /* Key events don't exhibit the same behavior as mouse events. */
    ret = WriteConsoleInputW(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = WriteConsoleInputW(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 2, "Expected count to be 2, got %lu\n", count);

    ret = FlushConsoleInputBuffer(input_handle);
    ok(ret == TRUE, "Expected FlushConsoleInputBuffer to return TRUE, got %d\n", ret);

    /* Try interleaving mouse and key events. */
    event.EventType = MOUSE_EVENT;
    event.Event.MouseEvent = mouse_event;

    ret = WriteConsoleInputW(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    event.EventType = KEY_EVENT;
    event.Event.KeyEvent = key_event;

    ret = WriteConsoleInputW(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 2, "Expected count to be 2, got %lu\n", count);

    event.EventType = MOUSE_EVENT;
    event.Event.MouseEvent = mouse_event;

    ret = WriteConsoleInputW(input_handle, &event, 1, &count);
    ok(ret == TRUE, "Expected WriteConsoleInputW to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    ret = GetNumberOfConsoleInputEvents(input_handle, &count);
    ok(ret == TRUE, "Expected GetNumberOfConsoleInputEvents to return TRUE, got %d\n", ret);
    ok(count == 3, "Expected count to be 3, got %lu\n", count);

    /* Restore the old console mode. */
    ret = SetConsoleMode(input_handle, console_mode);
    ok(ret == TRUE, "Expected SetConsoleMode to return TRUE, got %d\n", ret);
}

static void test_FlushConsoleInputBuffer(HANDLE input, HANDLE output)
{
    INPUT_RECORD record;
    DWORD count;
    BOOL ret;

    ret = FlushConsoleInputBuffer(input);
    ok(ret, "FlushConsoleInputBuffer failed: %lu\n", GetLastError());

    ret = GetNumberOfConsoleInputEvents(input, &count);
    ok(ret, "GetNumberOfConsoleInputEvents failed: %lu\n", GetLastError());
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    record.EventType = KEY_EVENT;
    record.Event.KeyEvent.bKeyDown = 1;
    record.Event.KeyEvent.wRepeatCount = 1;
    record.Event.KeyEvent.wVirtualKeyCode = VK_RETURN;
    record.Event.KeyEvent.wVirtualScanCode = VK_RETURN;
    record.Event.KeyEvent.uChar.UnicodeChar = '\r';
    record.Event.KeyEvent.dwControlKeyState = 0;
    ret = WriteConsoleInputW(input, &record, 1, &count);
    ok(ret, "WriteConsoleInputW failed: %lu\n", GetLastError());

    ret = GetNumberOfConsoleInputEvents(input, &count);
    ok(ret, "GetNumberOfConsoleInputEvents failed: %lu\n", GetLastError());
    ok(count == 1, "Expected count to be 0, got %lu\n", count);

    ret = FlushConsoleInputBuffer(input);
    ok(ret, "FlushConsoleInputBuffer failed: %lu\n", GetLastError());

    ret = GetNumberOfConsoleInputEvents(input, &count);
    ok(ret, "GetNumberOfConsoleInputEvents failed: %lu\n", GetLastError());
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    ret = WriteConsoleInputW(input, &record, 1, &count);
    ok(ret, "WriteConsoleInputW failed: %lu\n", GetLastError());

    ret = GetNumberOfConsoleInputEvents(input, &count);
    ok(ret, "GetNumberOfConsoleInputEvents failed: %lu\n", GetLastError());
    ok(count == 1, "Expected count to be 0, got %lu\n", count);

    ret = FlushFileBuffers(input);
    ok(ret, "FlushFileBuffers failed: %lu\n", GetLastError());

    ret = GetNumberOfConsoleInputEvents(input, &count);
    ok(ret, "GetNumberOfConsoleInputEvents failed: %lu\n", GetLastError());
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    ret = FlushConsoleInputBuffer(output);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "FlushConsoleInputBuffer returned: %x(%lu)\n",
       ret, GetLastError());

    ret = FlushFileBuffers(output);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "FlushFileBuffers returned: %x(%lu)\n",
       ret, GetLastError());
}

static void test_WriteConsoleOutputCharacterA(HANDLE output_handle)
{
    static const char output[] = {'a', 0};

    COORD origin = {0, 0};
    DWORD count;
    BOOL ret;
    int i;

    const struct
    {
        HANDLE hConsoleOutput;
        LPCSTR str;
        DWORD length;
        COORD coord;
        LPDWORD lpNumCharsWritten;
        DWORD expected_count;
        DWORD last_error;
        int win7_crash;
    } invalid_table[] =
    {
        {NULL, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, NULL, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {NULL, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, NULL, 1, {0, 0}, &count, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, output, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, output, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {NULL, output, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, output, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, NULL, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, NULL, 1, {0, 0}, &count, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, output, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, output, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, output, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, output, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {output_handle, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 1, {0, 0}, &count, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, output, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, output, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_table); i++)
    {
        if (invalid_table[i].win7_crash)
            continue;

        SetLastError(0xdeadbeef);
        if (invalid_table[i].lpNumCharsWritten) count = 0xdeadbeef;
        ret = WriteConsoleOutputCharacterA(invalid_table[i].hConsoleOutput,
                                           invalid_table[i].str,
                                           invalid_table[i].length,
                                           invalid_table[i].coord,
                                           invalid_table[i].lpNumCharsWritten);
        ok(!ret, "[%d] Expected WriteConsoleOutputCharacterA to return FALSE, got %d\n", i, ret);
        ok(GetLastError() == invalid_table[i].last_error,
           "[%d] Expected last error to be %lu, got %lu\n",
           i, invalid_table[i].last_error, GetLastError());
    }

    count = 0xdeadbeef;
    ret = WriteConsoleOutputCharacterA(output_handle, NULL, 0, origin, &count);
    ok(ret == TRUE, "Expected WriteConsoleOutputCharacterA to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = WriteConsoleOutputCharacterA(output_handle, output, 0, origin, &count);
    ok(ret == TRUE, "Expected WriteConsoleOutputCharacterA to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = WriteConsoleOutputCharacterA(output_handle, output, 1, origin, &count);
    ok(ret == TRUE, "Expected WriteConsoleOutputCharacterA to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    count = 0xdeadbeef;
    origin.X = 200;
    ret = WriteConsoleOutputCharacterA(output_handle, output, 0, origin, &count);
    ok(ret == TRUE, "Expected WriteConsoleOutputCharacterA to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    for (i = 1; i < 32; i++)
    {
        CONSOLE_SCREEN_BUFFER_INFO csbi;
        char ch = (char)i;
        COORD c = {1, 2};

        ret = WriteConsoleOutputCharacterA(output_handle, &ch, 1, c, &count);
        ok(ret == TRUE, "Expected WriteConsoleOutputCharacterA to return TRUE, got %d\n", ret);
        ok(count == 1, "Expected count to be 1, got %lu\n", count);
        okCHAR(output_handle, c, (char)i, 7);
        ret = GetConsoleScreenBufferInfo(output_handle, &csbi);
    }
}

static void test_WriteConsoleOutputCharacterW(HANDLE output_handle)
{
    static const WCHAR outputW[] = {'a',0};

    COORD origin = {0, 0};
    DWORD count;
    BOOL ret;
    int i;

    const struct
    {
        HANDLE hConsoleOutput;
        LPCWSTR str;
        DWORD length;
        COORD coord;
        LPDWORD lpNumCharsWritten;
        DWORD expected_count;
        DWORD last_error;
        int win7_crash;
    } invalid_table[] =
    {
        {NULL, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, NULL, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {NULL, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, NULL, 1, {0, 0}, &count, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, outputW, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, outputW, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {NULL, outputW, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, outputW, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, NULL, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, NULL, 1, {0, 0}, &count, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, outputW, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, outputW, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, outputW, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, outputW, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {output_handle, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 1, {0, 0}, &count, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, outputW, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, outputW, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_table); i++)
    {
        if (invalid_table[i].win7_crash)
            continue;

        SetLastError(0xdeadbeef);
        if (invalid_table[i].lpNumCharsWritten) count = 0xdeadbeef;
        ret = WriteConsoleOutputCharacterW(invalid_table[i].hConsoleOutput,
                                           invalid_table[i].str,
                                           invalid_table[i].length,
                                           invalid_table[i].coord,
                                           invalid_table[i].lpNumCharsWritten);
        ok(!ret, "[%d] Expected WriteConsoleOutputCharacterW to return FALSE, got %d\n", i, ret);
        ok(GetLastError() == invalid_table[i].last_error,
           "[%d] Expected last error to be %lu, got %lu\n",
           i, invalid_table[i].last_error, GetLastError());
    }

    count = 0xdeadbeef;
    ret = WriteConsoleOutputCharacterW(output_handle, NULL, 0, origin, &count);
    ok(ret == TRUE, "Expected WriteConsoleOutputCharacterW to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = WriteConsoleOutputCharacterW(output_handle, outputW, 0, origin, &count);
    ok(ret == TRUE, "Expected WriteConsoleOutputCharacterW to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = WriteConsoleOutputCharacterW(output_handle, outputW, 1, origin, &count);
    ok(ret == TRUE, "Expected WriteConsoleOutputCharacterW to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    count = 0xdeadbeef;
    origin.X = 200;
    ret = WriteConsoleOutputCharacterW(output_handle, outputW, 0, origin, &count);
    ok(ret == TRUE, "Expected WriteConsoleOutputCharacterW to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

}

static void test_WriteConsoleOutputAttribute(HANDLE output_handle)
{
    WORD attr = FOREGROUND_BLUE;
    COORD origin = {0, 0};
    DWORD count;
    BOOL ret;
    int i;

    const struct
    {
        HANDLE hConsoleOutput;
        const WORD *attr;
        DWORD length;
        COORD coord;
        LPDWORD lpNumAttrsWritten;
        DWORD expected_count;
        DWORD last_error;
        int win7_crash;
    } invalid_table[] =
    {
        {NULL, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, NULL, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {NULL, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, NULL, 1, {0, 0}, &count, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, &attr, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, &attr, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {NULL, &attr, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, &attr, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, NULL, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, NULL, 1, {0, 0}, &count, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, &attr, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, &attr, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, &attr, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, &attr, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {output_handle, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 1, {0, 0}, &count, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, &attr, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, &attr, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_table); i++)
    {
        if (invalid_table[i].win7_crash)
            continue;

        SetLastError(0xdeadbeef);
        if (invalid_table[i].lpNumAttrsWritten) count = 0xdeadbeef;
        ret = WriteConsoleOutputAttribute(invalid_table[i].hConsoleOutput,
                                          invalid_table[i].attr,
                                          invalid_table[i].length,
                                          invalid_table[i].coord,
                                          invalid_table[i].lpNumAttrsWritten);
        ok(!ret, "[%d] Expected WriteConsoleOutputAttribute to return FALSE, got %d\n", i, ret);
        ok(GetLastError() == invalid_table[i].last_error,
           "[%d] Expected last error to be %lu, got %lu\n",
           i, invalid_table[i].last_error, GetLastError());
    }

    count = 0xdeadbeef;
    ret = WriteConsoleOutputAttribute(output_handle, NULL, 0, origin, &count);
    ok(ret == TRUE, "Expected WriteConsoleOutputAttribute to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = WriteConsoleOutputAttribute(output_handle, &attr, 0, origin, &count);
    ok(ret == TRUE, "Expected WriteConsoleOutputAttribute to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = WriteConsoleOutputAttribute(output_handle, &attr, 1, origin, &count);
    ok(ret == TRUE, "Expected WriteConsoleOutputAttribute to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    count = 0xdeadbeef;
    origin.X = 200;
    ret = WriteConsoleOutputAttribute(output_handle, &attr, 0, origin, &count);
    ok(ret == TRUE, "Expected WriteConsoleOutputAttribute to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);
}

static void set_region(SMALL_RECT *region, unsigned int left, unsigned int top, unsigned int right, unsigned int bottom)
{
    region->Left   = left;
    region->Top    = top;
    region->Right  = right;
    region->Bottom = bottom;
}

#define check_region(a,b,c,d,e) check_region_(__LINE__,a,b,c,d,e)
static void check_region_(unsigned int line, const SMALL_RECT *region, unsigned int left, unsigned int top, int right, int bottom)
{
    ok_(__FILE__,line)(region->Left == left, "Left = %u, expected %u\n", region->Left, left);
    ok_(__FILE__,line)(region->Top == top, "Top = %u, expected %u\n", region->Top, top);
    /* In multiple places returned region depends on Windows versions: some return right < left, others leave it untouched */
    if (right >= 0)
        ok_(__FILE__,line)(region->Right == right, "Right = %u, expected %u\n", region->Right, right);
    else
        ok_(__FILE__,line)(region->Right == -right || region->Right == region->Left - 1,
                           "Right = %u, expected %d\n", region->Right, right);
    if (bottom > 0)
        ok_(__FILE__,line)(region->Bottom == bottom, "Bottom = %u, expected %u\n", region->Bottom, bottom);
    else if (bottom < 0)
        ok_(__FILE__,line)(region->Bottom == -bottom || region->Bottom == region->Top - 1,
                           "Bottom = %u, expected %d\n", region->Bottom, bottom);
}

static void test_WriteConsoleOutput(HANDLE console)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    CHAR_INFO char_info_buf[2048];
    SMALL_RECT region;
    COORD size, coord;
    unsigned int i;
    BOOL ret;

    for (i = 0; i < ARRAY_SIZE(char_info_buf); i++)
    {
        char_info_buf[i].Char.UnicodeChar = '0' + i % 10;
        char_info_buf[i].Attributes = 0;
    }

    ret = GetConsoleScreenBufferInfo(console, &info);
    ok(ret, "GetConsoleScreenBufferInfo failed: %lu\n", GetLastError());

    size.X = 23;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 15, 11);
    ret = WriteConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(ret, "WriteConsoleOutputW failed: %lu\n", GetLastError());
    check_region(&region, 10, 7, 15, 11);

    size.X = 23;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 200, 7, 15, 211);
    ret = WriteConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "WriteConsoleOutputW returned: %x(%lu)\n", ret, GetLastError());
    check_region(&region, 200, 7, 15, 211);

    size.X = 23;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 200, 7, 211, 8);
    ret = WriteConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(ret, "WriteConsoleOutputW failed: %lu\n", GetLastError());
    check_region(&region, 200, 7, 211, 8);

    size.X = 23;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 9, 11);
    ret = WriteConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "WriteConsoleOutputW returned: %x(%lu)\n", ret, GetLastError());
    check_region(&region, 10, 7, 9, 11);

    size.X = 23;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 11, 6);
    ret = WriteConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "WriteConsoleOutputW returned: %x(%lu)\n", ret, GetLastError());
    check_region(&region, 10, 7, 11, 6);

    size.X = 2;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 15, 11);
    ret = WriteConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "WriteConsoleOutputW returned: %x(%lu)\n", ret, GetLastError());
    check_region(&region, 10, 7, 15, 11);

    size.X = 23;
    size.Y = 3;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 15, 11);
    ret = WriteConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "WriteConsoleOutputW returned: %x(%lu)\n", ret, GetLastError());
    check_region(&region, 10, 7, 15, 11);

    size.X = 6;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 15, 11);
    ret = WriteConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(ret, "WriteConsoleOutputW failed: %lu\n", GetLastError());
    check_region(&region, 10, 7, 13, 11);

    size.X = 6;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 15, 11);
    ret = WriteConsoleOutputW((HANDLE)0xdeadbeef, char_info_buf, size, coord, &region);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "WriteConsoleOutputW returned: %x(%lu)\n", ret, GetLastError());
    if (!skip_nt) check_region(&region, 10, 7, 13, 11);

    size.X = 16;
    size.Y = 7;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 15, 11);
    ret = WriteConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(ret, "WriteConsoleOutputW failed: %lu\n", GetLastError());
    check_region(&region, 10, 7, 15, 10);

    size.X = 16;
    size.Y = 7;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, info.dwSize.X - 2, 7, info.dwSize.X + 2, 7);
    ret = WriteConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(ret, "WriteConsoleOutputW failed: %lu\n", GetLastError());
    check_region(&region, info.dwSize.X - 2, 7, info.dwSize.X - 1, 7);
}

static void test_FillConsoleOutputCharacterA(HANDLE output_handle)
{
    COORD origin = {0, 0};
    DWORD count;
    BOOL ret;
    int i;

    const struct
    {
        HANDLE hConsoleOutput;
        CHAR ch;
        DWORD length;
        COORD coord;
        LPDWORD lpNumCharsWritten;
        DWORD last_error;
        int win7_crash;
    } invalid_table[] =
    {
        {NULL, 'a', 0, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {NULL, 'a', 0, {0, 0}, &count, ERROR_INVALID_HANDLE},
        {NULL, 'a', 1, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {NULL, 'a', 1, {0, 0}, &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, 'a', 0, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, 'a', 0, {0, 0}, &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, 'a', 1, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, 'a', 1, {0, 0}, &count, ERROR_INVALID_HANDLE},
        {output_handle, 'a', 0, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {output_handle, 'a', 1, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_table); i++)
    {
        if (invalid_table[i].win7_crash)
            continue;

        SetLastError(0xdeadbeef);
        if (invalid_table[i].lpNumCharsWritten) count = 0xdeadbeef;
        ret = FillConsoleOutputCharacterA(invalid_table[i].hConsoleOutput,
                                          invalid_table[i].ch,
                                          invalid_table[i].length,
                                          invalid_table[i].coord,
                                          invalid_table[i].lpNumCharsWritten);
        ok(!ret, "[%d] Expected FillConsoleOutputCharacterA to return FALSE, got %d\n", i, ret);
        ok(GetLastError() == invalid_table[i].last_error,
           "[%d] Expected last error to be %lu, got %lu\n",
           i, invalid_table[i].last_error, GetLastError());
    }

    count = 0xdeadbeef;
    ret = FillConsoleOutputCharacterA(output_handle, 'a', 0, origin, &count);
    ok(ret == TRUE, "Expected FillConsoleOutputCharacterA to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = FillConsoleOutputCharacterA(output_handle, 'a', 1, origin, &count);
    ok(ret == TRUE, "Expected FillConsoleOutputCharacterA to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);
}

static void test_FillConsoleOutputCharacterW(HANDLE output_handle)
{
    COORD origin = {0, 0};
    DWORD count;
    BOOL ret;
    int i;

    const struct
    {
        HANDLE hConsoleOutput;
        WCHAR ch;
        DWORD length;
        COORD coord;
        LPDWORD lpNumCharsWritten;
        DWORD last_error;
        int win7_crash;
    } invalid_table[] =
    {
        {NULL, 'a', 0, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {NULL, 'a', 0, {0, 0}, &count, ERROR_INVALID_HANDLE},
        {NULL, 'a', 1, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {NULL, 'a', 1, {0, 0}, &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, 'a', 0, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, 'a', 0, {0, 0}, &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, 'a', 1, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, 'a', 1, {0, 0}, &count, ERROR_INVALID_HANDLE},
        {output_handle, 'a', 0, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {output_handle, 'a', 1, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_table); i++)
    {
        if (invalid_table[i].win7_crash)
            continue;

        SetLastError(0xdeadbeef);
        if (invalid_table[i].lpNumCharsWritten) count = 0xdeadbeef;
        ret = FillConsoleOutputCharacterW(invalid_table[i].hConsoleOutput,
                                          invalid_table[i].ch,
                                          invalid_table[i].length,
                                          invalid_table[i].coord,
                                          invalid_table[i].lpNumCharsWritten);
        ok(!ret, "[%d] Expected FillConsoleOutputCharacterW to return FALSE, got %d\n", i, ret);
        ok(GetLastError() == invalid_table[i].last_error,
           "[%d] Expected last error to be %lu, got %lu\n",
           i, invalid_table[i].last_error, GetLastError());
    }

    count = 0xdeadbeef;
    ret = FillConsoleOutputCharacterW(output_handle, 'a', 0, origin, &count);
    ok(ret == TRUE, "Expected FillConsoleOutputCharacterW to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = FillConsoleOutputCharacterW(output_handle, 'a', 1, origin, &count);
    ok(ret == TRUE, "Expected FillConsoleOutputCharacterW to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);
}

static void test_FillConsoleOutputAttribute(HANDLE output_handle)
{
    COORD origin = {0, 0};
    DWORD count;
    BOOL ret;
    int i;

    const struct
    {
        HANDLE hConsoleOutput;
        WORD attr;
        DWORD length;
        COORD coord;
        LPDWORD lpNumAttrsWritten;
        DWORD last_error;
        int win7_crash;
    } invalid_table[] =
    {
        {NULL, FOREGROUND_BLUE, 0, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {NULL, FOREGROUND_BLUE, 0, {0, 0}, &count, ERROR_INVALID_HANDLE},
        {NULL, FOREGROUND_BLUE, 1, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {NULL, FOREGROUND_BLUE, 1, {0, 0}, &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, FOREGROUND_BLUE, 0, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, FOREGROUND_BLUE, 0, {0, 0}, &count, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, FOREGROUND_BLUE, 1, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, FOREGROUND_BLUE, 1, {0, 0}, &count, ERROR_INVALID_HANDLE},
        {output_handle, FOREGROUND_BLUE, 0, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
        {output_handle, FOREGROUND_BLUE, 1, {0, 0}, NULL, ERROR_INVALID_ACCESS, 1},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_table); i++)
    {
        if (invalid_table[i].win7_crash)
            continue;

        SetLastError(0xdeadbeef);
        if (invalid_table[i].lpNumAttrsWritten) count = 0xdeadbeef;
        ret = FillConsoleOutputAttribute(invalid_table[i].hConsoleOutput,
                                         invalid_table[i].attr,
                                         invalid_table[i].length,
                                         invalid_table[i].coord,
                                         invalid_table[i].lpNumAttrsWritten);
        ok(!ret, "[%d] Expected FillConsoleOutputAttribute to return FALSE, got %d\n", i, ret);
        ok(GetLastError() == invalid_table[i].last_error,
           "[%d] Expected last error to be %lu, got %lu\n",
           i, invalid_table[i].last_error, GetLastError());
    }

    count = 0xdeadbeef;
    ret = FillConsoleOutputAttribute(output_handle, FOREGROUND_BLUE, 0, origin, &count);
    ok(ret == TRUE, "Expected FillConsoleOutputAttribute to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = FillConsoleOutputAttribute(output_handle, FOREGROUND_BLUE, 1, origin, &count);
    ok(ret == TRUE, "Expected FillConsoleOutputAttribute to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    count = 0xdeadbeef;
    ret = FillConsoleOutputAttribute(output_handle, ~0, 1, origin, &count);
    ok(ret == TRUE, "Expected FillConsoleOutputAttribute to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);
}

static void test_ReadConsoleOutputCharacterA(HANDLE output_handle)
{
    CHAR read;
    COORD origin = {0, 0};
    DWORD count;
    BOOL ret;
    int i;

    const struct
    {
        HANDLE hConsoleOutput;
        LPSTR lpstr;
        DWORD length;
        COORD coord;
        LPDWORD read_count;
        DWORD expected_count;
        DWORD last_error;
        int win7_crash;
    } invalid_table[] =
    {
        {NULL, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, NULL, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {NULL, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, NULL, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE, 1},
        {NULL, &read, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, &read, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {NULL, &read, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, &read, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, NULL, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, NULL, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE, 1},
        {INVALID_HANDLE_VALUE, &read, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, &read, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, &read, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, &read, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {output_handle, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 1, {0, 0}, &count, 1, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 10, {0, 0}, &count, 10, ERROR_INVALID_ACCESS, 1},
        {output_handle, &read, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, &read, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_table); i++)
    {
        if (invalid_table[i].win7_crash)
            continue;

        SetLastError(0xdeadbeef);
        if (invalid_table[i].read_count) count = 0xdeadbeef;
        ret = ReadConsoleOutputCharacterA(invalid_table[i].hConsoleOutput,
                                          invalid_table[i].lpstr,
                                          invalid_table[i].length,
                                          invalid_table[i].coord,
                                          invalid_table[i].read_count);
        ok(!ret, "[%d] Expected ReadConsoleOutputCharacterA to return FALSE, got %d\n", i, ret);
        ok(GetLastError() == invalid_table[i].last_error,
           "[%d] Expected last error to be %lu, got %lu\n",
           i, invalid_table[i].last_error, GetLastError());
    }

    count = 0xdeadbeef;
    ret = ReadConsoleOutputCharacterA(output_handle, NULL, 0, origin, &count);
    ok(ret == TRUE, "Expected ReadConsoleOutputCharacterA to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = ReadConsoleOutputCharacterA(output_handle, &read, 0, origin, &count);
    ok(ret == TRUE, "Expected ReadConsoleOutputCharacterA to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = ReadConsoleOutputCharacterA(output_handle, &read, 1, origin, &count);
    ok(ret == TRUE, "Expected ReadConsoleOutputCharacterA to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    count = 0xdeadbeef;
    origin.X = 200;
    ret = ReadConsoleOutputCharacterA(output_handle, &read, 1, origin, &count);
    ok(ret == TRUE, "Expected ReadConsoleOutputCharacterA to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);
}

static void test_ReadConsoleOutputCharacterW(HANDLE output_handle)
{
    WCHAR read;
    COORD origin = {0, 0};
    DWORD count;
    BOOL ret;
    int i;

    const struct
    {
        HANDLE hConsoleOutput;
        LPWSTR buffer;
        DWORD length;
        COORD coord;
        LPDWORD read_count;
        DWORD expected_count;
        DWORD last_error;
        int win7_crash;
    } invalid_table[] =
    {
        {NULL, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, NULL, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {NULL, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, NULL, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE, 1},
        {NULL, &read, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, &read, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {NULL, &read, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, &read, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, NULL, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, NULL, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE, 1},
        {INVALID_HANDLE_VALUE, &read, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, &read, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, &read, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, &read, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {output_handle, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 1, {0, 0}, &count, 1, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 10, {0, 0}, &count, 10, ERROR_INVALID_ACCESS, 1},
        {output_handle, &read, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, &read, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_table); i++)
    {
        if (invalid_table[i].win7_crash)
            continue;

        SetLastError(0xdeadbeef);
        if (invalid_table[i].read_count) count = 0xdeadbeef;
        ret = ReadConsoleOutputCharacterW(invalid_table[i].hConsoleOutput,
                                          invalid_table[i].buffer,
                                          invalid_table[i].length,
                                          invalid_table[i].coord,
                                          invalid_table[i].read_count);
        ok(!ret, "[%d] Expected ReadConsoleOutputCharacterW to return FALSE, got %d\n", i, ret);
        ok(GetLastError() == invalid_table[i].last_error,
           "[%d] Expected last error to be %lu, got %lu\n",
           i, invalid_table[i].last_error, GetLastError());
    }

    count = 0xdeadbeef;
    ret = ReadConsoleOutputCharacterW(output_handle, NULL, 0, origin, &count);
    ok(ret == TRUE, "Expected ReadConsoleOutputCharacterW to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = ReadConsoleOutputCharacterW(output_handle, &read, 0, origin, &count);
    ok(ret == TRUE, "Expected ReadConsoleOutputCharacterW to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = ReadConsoleOutputCharacterW(output_handle, &read, 1, origin, &count);
    ok(ret == TRUE, "Expected ReadConsoleOutputCharacterW to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    count = 0xdeadbeef;
    origin.X = 200;
    ret = ReadConsoleOutputCharacterW(output_handle, &read, 1, origin, &count);
    ok(ret == TRUE, "Expected ReadConsoleOutputCharacterW to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);
}

static void test_ReadConsoleOutputAttribute(HANDLE output_handle)
{
    WORD attr;
    COORD origin = {0, 0};
    DWORD count;
    BOOL ret;
    int i;

    const struct
    {
        HANDLE hConsoleOutput;
        LPWORD lpAttribute;
        DWORD length;
        COORD coord;
        LPDWORD read_count;
        DWORD expected_count;
        DWORD last_error;
        int win7_crash;
    } invalid_table[] =
    {
        {NULL, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, NULL, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {NULL, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, NULL, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE, 1},
        {NULL, &attr, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, &attr, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {NULL, &attr, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {NULL, &attr, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, NULL, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, NULL, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE, 1},
        {INVALID_HANDLE_VALUE, &attr, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, &attr, 0, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {INVALID_HANDLE_VALUE, &attr, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {INVALID_HANDLE_VALUE, &attr, 1, {0, 0}, &count, 0, ERROR_INVALID_HANDLE},
        {output_handle, NULL, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, NULL, 1, {0, 0}, &count, 1, ERROR_INVALID_ACCESS, 1},
        {output_handle, &attr, 0, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
        {output_handle, &attr, 1, {0, 0}, NULL, 0xdeadbeef, ERROR_INVALID_ACCESS, 1},
    };

    for (i = 0; i < ARRAY_SIZE(invalid_table); i++)
    {
        if (invalid_table[i].win7_crash)
            continue;

        SetLastError(0xdeadbeef);
        if (invalid_table[i].read_count) count = 0xdeadbeef;
        ret = ReadConsoleOutputAttribute(invalid_table[i].hConsoleOutput,
                                         invalid_table[i].lpAttribute,
                                         invalid_table[i].length,
                                         invalid_table[i].coord,
                                         invalid_table[i].read_count);
        ok(!ret, "[%d] Expected ReadConsoleOutputAttribute to return FALSE, got %d\n", i, ret);
        ok(GetLastError() == invalid_table[i].last_error,
           "[%d] Expected last error to be %lu, got %lu\n",
           i, invalid_table[i].last_error, GetLastError());
    }

    count = 0xdeadbeef;
    ret = ReadConsoleOutputAttribute(output_handle, NULL, 0, origin, &count);
    ok(ret == TRUE, "Expected ReadConsoleOutputAttribute to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = ReadConsoleOutputAttribute(output_handle, &attr, 0, origin, &count);
    ok(ret == TRUE, "Expected ReadConsoleOutputAttribute to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 0, got %lu\n", count);

    count = 0xdeadbeef;
    ret = ReadConsoleOutputAttribute(output_handle, &attr, 1, origin, &count);
    ok(ret == TRUE, "Expected ReadConsoleOutputAttribute to return TRUE, got %d\n", ret);
    ok(count == 1, "Expected count to be 1, got %lu\n", count);

    count = 0xdeadbeef;
    origin.X = 200;
    ret = ReadConsoleOutputAttribute(output_handle, &attr, 1, origin, &count);
    ok(ret == TRUE, "Expected ReadConsoleOutputAttribute to return TRUE, got %d\n", ret);
    ok(count == 0, "Expected count to be 1, got %lu\n", count);
}

static void test_ReadConsoleOutput(HANDLE console)
{
    CONSOLE_SCREEN_BUFFER_INFO info;
    CHAR_INFO char_info_buf[2048];
    SMALL_RECT region;
    COORD size, coord;
    DWORD count;
    WCHAR ch;
    BOOL ret;

    if (skip_nt) return;

    ret = GetConsoleScreenBufferInfo(console, &info);
    ok(ret, "GetConsoleScreenBufferInfo failed: %lu\n", GetLastError());

    size.X = 23;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 15, 11);
    ret = ReadConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(ret, "ReadConsoleOutputW failed: %lu\n", GetLastError());
    check_region(&region, 10, 7, 15, 11);

    size.X = 23;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 200, 7, 15, 211);
    ret = ReadConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(!ret, "ReadConsoleOutputW returned: %x(%lu)\n", ret, GetLastError());
    check_region(&region, 200, 7, -15, 0);

    size.X = 23;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 200, 7, 211, 8);
    ret = ReadConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok((!ret && (GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_INVALID_FUNCTION)) || broken(ret /* win8 */),
       "ReadConsoleOutputW returned: %x %lu\n", ret, GetLastError());
    if (!ret && GetLastError() == ERROR_INVALID_PARAMETER) check_region(&region, 200, 7, -211, -8);

    size.X = 23;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 9, 11);
    ret = ReadConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok((!ret && (GetLastError() == ERROR_INVALID_FUNCTION || GetLastError() == ERROR_NOT_ENOUGH_MEMORY)) || broken(ret /* win8 */),
       "ReadConsoleOutputW returned: %x(%lu)\n", ret, GetLastError());
    check_region(&region, 10, 7, 9, -11);

    size.X = 23;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 11, 6);
    ret = ReadConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok((!ret && (GetLastError() == ERROR_INVALID_FUNCTION || GetLastError() == ERROR_NOT_ENOUGH_MEMORY)) || broken(ret /* win8 */),
       "ReadConsoleOutputW returned: %x(%lu)\n", ret, GetLastError());
    check_region(&region, 10, 7, -11, 6);

    size.X = 2;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 15, 11);
    ret = ReadConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok((!ret && (GetLastError() == ERROR_INVALID_FUNCTION || GetLastError() == ERROR_NOT_ENOUGH_MEMORY)) || broken(ret /* win8 */),
       "ReadConsoleOutputW returned: %x(%lu)\n", ret, GetLastError());
    check_region(&region, 10, 7, -15, -11);

    size.X = 23;
    size.Y = 3;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 15, 11);
    ret = ReadConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok((!ret && (GetLastError() == ERROR_INVALID_FUNCTION || GetLastError() == ERROR_NOT_ENOUGH_MEMORY)) || broken(ret /* win8 */),
       "ReadConsoleOutputW returned: %x(%lu)\n", ret, GetLastError());
    check_region(&region, 10, 7, -15, 6);

    size.X = 6;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 15, 11);
    ret = ReadConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(ret, "ReadConsoleOutputW failed: %lu\n", GetLastError());
    check_region(&region, 10, 7, 13, 11);

    size.X = 6;
    size.Y = 17;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 15, 11);
    ret = ReadConsoleOutputW((HANDLE)0xdeadbeef, char_info_buf, size, coord, &region);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "ReadConsoleOutputW returned: %x(%lu)\n", ret, GetLastError());
    if (!skip_nt) check_region(&region, 10, 7, 13, 11);

    size.X = 16;
    size.Y = 7;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, 10, 7, 15, 11);
    ret = ReadConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(ret, "ReadConsoleOutputW failed: %lu\n", GetLastError());
    check_region(&region, 10, 7, 15, 10);

    size.X = 16;
    size.Y = 7;
    coord.X = 2;
    coord.Y = 3;
    set_region(&region, info.dwSize.X - 2, 7, info.dwSize.X + 2, 7);
    ret = ReadConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(ret || GetLastError() == ERROR_INVALID_PARAMETER, "ReadConsoleOutputW failed: %lu\n", GetLastError());
    if (ret) check_region(&region, info.dwSize.X - 2, 7, info.dwSize.X - 1, 7);

    coord.X = 2;
    coord.Y = 3;
    ret = WriteConsoleOutputCharacterW(console, L"xyz", 3, coord, &count);
    ok(ret, "WriteConsoleOutputCharacterW failed: %lu\n", GetLastError());
    ok(count == 3, "count = %lu\n", count);

    memset(char_info_buf, 0xc0, sizeof(char_info_buf));
    size.X = 16;
    size.Y = 7;
    coord.X = 5;
    coord.Y = 6;
    set_region(&region, 2, 3, 5, 3);
    ret = ReadConsoleOutputW(console, char_info_buf, size, coord, &region);
    ok(ret, "ReadConsoleOutputW failed: %lu\n", GetLastError());
    check_region(&region, 2, 3, 5, 3);
    ch = char_info_buf[coord.Y * size.X + coord.X].Char.UnicodeChar;
    ok(ch == 'x', "unexpected char %c/%x\n", ch, ch);
    ch = char_info_buf[coord.Y * size.X + coord.X + 1].Char.UnicodeChar;
    ok(ch == 'y', "unexpected char %c/%x\n", ch, ch);
    ch = char_info_buf[coord.Y * size.X + coord.X + 2].Char.UnicodeChar;
    ok(ch == 'z', "unexpected char %c/%x\n", ch, ch);
}

static void test_ReadConsole(HANDLE input)
{
    DWORD ret, bytes;
    char buf[1024];
    HANDLE output;

    SetLastError(0xdeadbeef);
    ret = GetFileSize(input, NULL);
    ok(ret == INVALID_FILE_SIZE || broken(TRUE), /* only Win7 pro64 on 64bit returns a valid file size here */
       "expected INVALID_FILE_SIZE, got %#lx\n", ret);
    if (ret == INVALID_FILE_SIZE)
        ok(GetLastError() == ERROR_INVALID_HANDLE ||
           GetLastError() == ERROR_INVALID_FUNCTION, /* Win 8, 10 */
           "expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(input, buf, -128, &bytes, NULL);
    ok(!ret, "expected 0, got %lu\n", ret);
    ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY ||
       GetLastError() == ERROR_NOACCESS, /* Win 8, 10 */
       "expected ERROR_NOT_ENOUGH_MEMORY, got %ld\n", GetLastError());
    ok(!bytes, "expected 0, got %lu\n", bytes);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadConsoleA(input, buf, -128, &bytes, NULL);
    ok(!ret, "expected 0, got %lu\n", ret);
    ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY ||
       GetLastError() == ERROR_NOACCESS, /* Win 8, 10 */
       "expected ERROR_NOT_ENOUGH_MEMORY, got %ld\n", GetLastError());
    ok(bytes == 0xdeadbeef, "expected 0xdeadbeef, got %#lx\n", bytes);

    bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadConsoleW(input, buf, -128, &bytes, NULL);
    ok(!ret, "expected 0, got %lu\n", ret);
    ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY ||
       GetLastError() == ERROR_NOACCESS, /* Win 8, 10 */
       "expected ERROR_NOT_ENOUGH_MEMORY, got %ld\n", GetLastError());
    ok(bytes == 0xdeadbeef, "expected 0xdeadbeef, got %#lx\n", bytes);

    output = CreateFileA("CONOUT$", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(output != INVALID_HANDLE_VALUE, "Could not open console\n");

    ret = ReadConsoleW(output, buf, sizeof(buf) / sizeof(WCHAR), &bytes, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
       "ReadConsoleW returned %lx(%lu)\n", ret, GetLastError());

    ret = ReadConsoleA(output, buf, sizeof(buf), &bytes, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
       "ReadConsoleA returned %lx(%lu)\n", ret, GetLastError());

    ret = ReadFile(output, buf, sizeof(buf), &bytes, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
       "ReadFile returned %lx(%lu)\n", ret, GetLastError());

    CloseHandle(output);
}

static void test_GetCurrentConsoleFont(HANDLE std_output)
{
    BOOL ret;
    CONSOLE_FONT_INFO cfi;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    short int width, height;
    HANDLE pipe1, pipe2;
    COORD c;

    memset(&cfi, 0, sizeof(CONSOLE_FONT_INFO));
    SetLastError(0xdeadbeef);
    ret = GetCurrentConsoleFont(NULL, FALSE, &cfi);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    ok(!cfi.dwFontSize.X, "got %d, expected 0\n", cfi.dwFontSize.X);
    ok(!cfi.dwFontSize.Y, "got %d, expected 0\n", cfi.dwFontSize.Y);

    memset(&cfi, 0, sizeof(CONSOLE_FONT_INFO));
    SetLastError(0xdeadbeef);
    ret = GetCurrentConsoleFont(NULL, TRUE, &cfi);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    ok(!cfi.dwFontSize.X, "got %d, expected 0\n", cfi.dwFontSize.X);
    ok(!cfi.dwFontSize.Y, "got %d, expected 0\n", cfi.dwFontSize.Y);

    memset(&cfi, 0, sizeof(CONSOLE_FONT_INFO));
    SetLastError(0xdeadbeef);
    ret = GetCurrentConsoleFont(GetStdHandle(STD_INPUT_HANDLE), FALSE, &cfi);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    ok(!cfi.dwFontSize.X, "got %d, expected 0\n", cfi.dwFontSize.X);
    ok(!cfi.dwFontSize.Y, "got %d, expected 0\n", cfi.dwFontSize.Y);

    memset(&cfi, 0, sizeof(CONSOLE_FONT_INFO));
    SetLastError(0xdeadbeef);
    ret = GetCurrentConsoleFont(GetStdHandle(STD_INPUT_HANDLE), TRUE, &cfi);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    ok(!cfi.dwFontSize.X, "got %d, expected 0\n", cfi.dwFontSize.X);
    ok(!cfi.dwFontSize.Y, "got %d, expected 0\n", cfi.dwFontSize.Y);

    CreatePipe(&pipe1, &pipe2, NULL, 0);
    memset(&cfi, 0, sizeof(CONSOLE_FONT_INFO));
    SetLastError(0xdeadbeef);
    ret = GetCurrentConsoleFont(pipe1, TRUE, &cfi);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    ok(!cfi.dwFontSize.X, "got %d, expected 0\n", cfi.dwFontSize.X);
    ok(!cfi.dwFontSize.Y, "got %d, expected 0\n", cfi.dwFontSize.Y);
    CloseHandle(pipe1);
    CloseHandle(pipe2);

    memset(&cfi, 0, sizeof(CONSOLE_FONT_INFO));
    SetLastError(0xdeadbeef);
    ret = GetCurrentConsoleFont(std_output, FALSE, &cfi);
    ok(ret, "got %d, expected non-zero\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());
    GetConsoleScreenBufferInfo(std_output, &csbi);
    width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;
    c = GetConsoleFontSize(std_output, cfi.nFont);
    ok(cfi.dwFontSize.X == width || cfi.dwFontSize.X == c.X /* Vista and higher */,
        "got %d, expected %d\n", cfi.dwFontSize.X, width);
    ok(cfi.dwFontSize.Y == height || cfi.dwFontSize.Y == c.Y /* Vista and higher */,
        "got %d, expected %d\n", cfi.dwFontSize.Y, height);

    memset(&cfi, 0, sizeof(CONSOLE_FONT_INFO));
    SetLastError(0xdeadbeef);
    ret = GetCurrentConsoleFont(std_output, TRUE, &cfi);
    ok(ret, "got %d, expected non-zero\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());
    ok(cfi.dwFontSize.X == csbi.dwMaximumWindowSize.X,
       "got %d, expected %d\n", cfi.dwFontSize.X, csbi.dwMaximumWindowSize.X);
    ok(cfi.dwFontSize.Y == csbi.dwMaximumWindowSize.Y,
       "got %d, expected %d\n", cfi.dwFontSize.Y, csbi.dwMaximumWindowSize.Y);
}

#ifndef __REACTOS__ // Can't link
static void test_GetCurrentConsoleFontEx(HANDLE std_output)
{
    HANDLE hmod;
    BOOL (WINAPI *pGetCurrentConsoleFontEx)(HANDLE, BOOL, CONSOLE_FONT_INFOEX *);
    CONSOLE_FONT_INFO cfi;
    CONSOLE_FONT_INFOEX cfix;
    BOOL ret;
    HANDLE std_input = GetStdHandle(STD_INPUT_HANDLE);
    HANDLE pipe1, pipe2;
    COORD c;

    hmod = GetModuleHandleA("kernel32.dll");
    pGetCurrentConsoleFontEx = (void *)GetProcAddress(hmod, "GetCurrentConsoleFontEx");
    if (!pGetCurrentConsoleFontEx)
    {
        win_skip("GetCurrentConsoleFontEx is not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(NULL, FALSE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(NULL, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(std_input, FALSE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(std_input, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(std_output, FALSE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(std_output, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    cfix.cbSize = sizeof(CONSOLE_FONT_INFOEX);

    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(NULL, FALSE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(NULL, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(std_input, FALSE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(std_input, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    CreatePipe(&pipe1, &pipe2, NULL, 0);
    memset(&cfi, 0, sizeof(CONSOLE_FONT_INFO));
    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(pipe1, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    CloseHandle(pipe1);
    CloseHandle(pipe2);

    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(std_output, FALSE, &cfix);
    ok(ret, "got %d, expected non-zero\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());

    memset(&cfi, 0, sizeof(CONSOLE_FONT_INFO));
    SetLastError(0xdeadbeef);
    ret = GetCurrentConsoleFont(std_output, FALSE, &cfi);
    ok(ret, "got %d, expected non-zero\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());

    ok(cfix.dwFontSize.X == cfi.dwFontSize.X, "expected values to match\n");
    ok(cfix.dwFontSize.Y == cfi.dwFontSize.Y, "expected values to match\n");

    SetLastError(0xdeadbeef);
    c = GetConsoleFontSize(std_output, cfix.nFont);
    ok(c.X && c.Y, "GetConsoleFontSize failed; err = %lu\n", GetLastError());
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());

    ok(cfix.dwFontSize.X == c.X, "Font width doesn't match; got %u, expected %u\n",
       cfix.dwFontSize.X, c.X);
    ok(cfix.dwFontSize.Y == c.Y, "Font height doesn't match; got %u, expected %u\n",
       cfix.dwFontSize.Y, c.Y);

    ok(cfi.dwFontSize.X == c.X, "Font width doesn't match; got %u, expected %u\n",
       cfi.dwFontSize.X, c.X);
    ok(cfi.dwFontSize.Y == c.Y, "Font height doesn't match; got %u, expected %u\n",
       cfi.dwFontSize.Y, c.Y);

    SetLastError(0xdeadbeef);
    ret = pGetCurrentConsoleFontEx(std_output, TRUE, &cfix);
    ok(ret, "got %d, expected non-zero\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());

    memset(&cfi, 0, sizeof(CONSOLE_FONT_INFO));
    SetLastError(0xdeadbeef);
    ret = GetCurrentConsoleFont(std_output, TRUE, &cfi);
    ok(ret, "got %d, expected non-zero\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());

    ok(cfix.dwFontSize.X == cfi.dwFontSize.X, "expected values to match\n");
    ok(cfix.dwFontSize.Y == cfi.dwFontSize.Y, "expected values to match\n");
}

static void test_SetCurrentConsoleFontEx(HANDLE std_output)
{
    CONSOLE_FONT_INFOEX orig_cfix, cfix;
    BOOL ret;
    HANDLE pipe1, pipe2;
    HANDLE std_input = GetStdHandle(STD_INPUT_HANDLE);

    orig_cfix.cbSize = sizeof(CONSOLE_FONT_INFOEX);

    ret = GetCurrentConsoleFontEx(std_output, FALSE, &orig_cfix);
    ok(ret, "got %d, expected non-zero\n", ret);

    cfix = orig_cfix;
    cfix.cbSize = 0;

    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(NULL, FALSE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(NULL, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    CreatePipe(&pipe1, &pipe2, NULL, 0);
    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(pipe1, FALSE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());
    CloseHandle(pipe1);
    CloseHandle(pipe2);

    CreatePipe(&pipe1, &pipe2, NULL, 0);
    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(pipe1, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());
    CloseHandle(pipe1);
    CloseHandle(pipe2);

    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(std_input, FALSE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(std_input, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(std_output, FALSE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(std_output, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    cfix = orig_cfix;

    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(NULL, FALSE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(NULL, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    CreatePipe(&pipe1, &pipe2, NULL, 0);
    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(pipe1, FALSE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    CloseHandle(pipe1);
    CloseHandle(pipe2);

    CreatePipe(&pipe1, &pipe2, NULL, 0);
    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(pipe1, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    CloseHandle(pipe1);
    CloseHandle(pipe2);

    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(std_input, FALSE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(std_input, TRUE, &cfix);
    ok(!ret, "got %d, expected 0\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(std_output, FALSE, &cfix);
    ok(ret, "got %d, expected non-zero\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(std_output, TRUE, &cfix);
    ok(ret, "got %d, expected non-zero\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());

    /* Restore original console font parameters */
    SetLastError(0xdeadbeef);
    ret = SetCurrentConsoleFontEx(std_output, FALSE, &orig_cfix);
    ok(ret, "got %d, expected non-zero\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());
}
#endif

static void test_GetConsoleFontSize(HANDLE std_output)
{
    COORD c;
    DWORD index = 0;
    CONSOLE_FONT_INFO cfi;
    RECT r;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    LONG font_width, font_height;
    HMODULE hmod;
    DWORD (WINAPI *pGetNumberOfConsoleFonts)(void);
    HANDLE pipe1, pipe2;

    memset(&c, 10, sizeof(COORD));
    SetLastError(0xdeadbeef);
    c = GetConsoleFontSize(NULL, index);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    ok(!c.X, "got %d, expected 0\n", c.X);
    ok(!c.Y, "got %d, expected 0\n", c.Y);

    memset(&c, 10, sizeof(COORD));
    SetLastError(0xdeadbeef);
    c = GetConsoleFontSize(GetStdHandle(STD_INPUT_HANDLE), index);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    ok(!c.X, "got %d, expected 0\n", c.X);
    ok(!c.Y, "got %d, expected 0\n", c.Y);

    CreatePipe(&pipe1, &pipe2, NULL, 0);
    memset(&c, 10, sizeof(COORD));
    SetLastError(0xdeadbeef);
    c = GetConsoleFontSize(pipe1, index);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    ok(!c.X, "got %d, expected 0\n", c.X);
    ok(!c.Y, "got %d, expected 0\n", c.Y);
    CloseHandle(pipe1);
    CloseHandle(pipe2);

    GetCurrentConsoleFont(std_output, FALSE, &cfi);
    memset(&c, 10, sizeof(COORD));
    SetLastError(0xdeadbeef);
    c = GetConsoleFontSize(std_output, cfi.nFont);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());
    GetClientRect(GetConsoleWindow(), &r);
    GetConsoleScreenBufferInfo(std_output, &csbi);
    font_width = (r.right - r.left) / (csbi.srWindow.Right - csbi.srWindow.Left + 1);
    font_height = (r.bottom - r.top) / (csbi.srWindow.Bottom - csbi.srWindow.Top + 1);
    ok(c.X == font_width, "got %d, expected %ld\n", c.X, font_width);
    ok(c.Y == font_height, "got %d, expected %ld\n", c.Y, font_height);

    hmod = GetModuleHandleA("kernel32.dll");
    pGetNumberOfConsoleFonts = (void *)GetProcAddress(hmod, "GetNumberOfConsoleFonts");
    if (!pGetNumberOfConsoleFonts)
    {
        win_skip("GetNumberOfConsoleFonts is not available\n");
        return;
    }
    index = pGetNumberOfConsoleFonts();

    memset(&c, 10, sizeof(COORD));
    SetLastError(0xdeadbeef);
    c = GetConsoleFontSize(std_output, index);
    ok(GetLastError() == ERROR_INVALID_PARAMETER || broken(GetLastError() == 0xdeadbeef) /* win10 1809 */,
        "unexpected last error %lu\n", GetLastError());
    if (GetLastError() == ERROR_INVALID_PARAMETER)
    {
        ok(!c.X, "got %d, expected 0\n", c.X);
        ok(!c.Y, "got %d, expected 0\n", c.Y);
    }
}

static void test_GetLargestConsoleWindowSize(HANDLE std_output)
{
    COORD c, font;
    RECT r;
    LONG workarea_w, workarea_h, maxcon_w, maxcon_h;
    CONSOLE_SCREEN_BUFFER_INFO sbi;
    CONSOLE_FONT_INFO cfi;
    HANDLE pipe1, pipe2;
    DWORD index, i;
    HMODULE hmod;
    BOOL ret;
    DWORD (WINAPI *pGetNumberOfConsoleFonts)(void);
    BOOL (WINAPI *pSetConsoleFont)(HANDLE, DWORD);

    memset(&c, 10, sizeof(COORD));
    SetLastError(0xdeadbeef);
    c = GetLargestConsoleWindowSize(NULL);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    ok(!c.X, "got %d, expected 0\n", c.X);
    ok(!c.Y, "got %d, expected 0\n", c.Y);

    memset(&c, 10, sizeof(COORD));
    SetLastError(0xdeadbeef);
    c = GetLargestConsoleWindowSize(GetStdHandle(STD_INPUT_HANDLE));
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    ok(!c.X, "got %d, expected 0\n", c.X);
    ok(!c.Y, "got %d, expected 0\n", c.Y);

    CreatePipe(&pipe1, &pipe2, NULL, 0);
    memset(&c, 10, sizeof(COORD));
    SetLastError(0xdeadbeef);
    c = GetLargestConsoleWindowSize(pipe1);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    ok(!c.X, "got %d, expected 0\n", c.X);
    ok(!c.Y, "got %d, expected 0\n", c.Y);
    CloseHandle(pipe1);
    CloseHandle(pipe2);

    SystemParametersInfoW(SPI_GETWORKAREA, 0, &r, 0);
    workarea_w = r.right - r.left;
    workarea_h = r.bottom - r.top - GetSystemMetrics(SM_CYCAPTION);

    GetCurrentConsoleFont(std_output, FALSE, &cfi);
    index = cfi.nFont; /* save current font index */

    hmod = GetModuleHandleA("kernel32.dll");
    pGetNumberOfConsoleFonts = (void *)GetProcAddress(hmod, "GetNumberOfConsoleFonts");
    if (!pGetNumberOfConsoleFonts)
    {
        win_skip("GetNumberOfConsoleFonts is not available\n");
        return;
    }
    pSetConsoleFont = (void *)GetProcAddress(hmod, "SetConsoleFont");
    if (!pSetConsoleFont)
    {
        win_skip("SetConsoleFont is not available\n");
        return;
    }

    for (i = 0; i < pGetNumberOfConsoleFonts(); i++)
    {
        pSetConsoleFont(std_output, i);
        memset(&c, 10, sizeof(COORD));
        SetLastError(0xdeadbeef);
        c = GetLargestConsoleWindowSize(std_output);
        ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());
        GetCurrentConsoleFont(std_output, FALSE, &cfi);
        font = GetConsoleFontSize(std_output, cfi.nFont);
        maxcon_w = workarea_w / font.X;
        maxcon_h = workarea_h / font.Y;
        ok(c.X == maxcon_w || c.X == maxcon_w - 1 /* Win10 */, "got %d, expected %ld\n", c.X, maxcon_w);
        ok(c.Y == maxcon_h || c.Y == maxcon_h - 1 /* Win10 */, "got %d, expected %ld\n", c.Y, maxcon_h);

        ret = GetConsoleScreenBufferInfo(std_output, &sbi);
        ok(ret, "GetConsoleScreenBufferInfo failed %lu\n", GetLastError());
        ok(sbi.dwMaximumWindowSize.X == min(c.X, sbi.dwSize.X), "got %d, expected %d\n",
           sbi.dwMaximumWindowSize.X, min(c.X, sbi.dwSize.X));
        ok(sbi.dwMaximumWindowSize.Y == min(c.Y, sbi.dwSize.Y), "got %d, expected %d\n",
           sbi.dwMaximumWindowSize.Y, min(c.Y, sbi.dwSize.Y));
    }
    pSetConsoleFont(std_output, index); /* restore original font size */
}

static void test_GetConsoleFontInfo(HANDLE std_output)
{
    HANDLE hmod;
    BOOL (WINAPI *pGetConsoleFontInfo)(HANDLE, BOOL, DWORD, CONSOLE_FONT_INFO *);
    DWORD (WINAPI *pGetNumberOfConsoleFonts)(void);
    DWORD num_fonts, index, i;
    int memsize, win_width, win_height, tmp_w, tmp_h;
    CONSOLE_FONT_INFO *cfi;
    BOOL ret;
    CONSOLE_SCREEN_BUFFER_INFO csbi;
    COORD orig_sb_size, tmp_sb_size, orig_font, tmp_font;

    hmod = GetModuleHandleA("kernel32.dll");
    pGetConsoleFontInfo = (void *)GetProcAddress(hmod, "GetConsoleFontInfo");
    if (!pGetConsoleFontInfo)
    {
        win_skip("GetConsoleFontInfo is not available\n");
        return;
    }

    pGetNumberOfConsoleFonts = (void *)GetProcAddress(hmod, "GetNumberOfConsoleFonts");
    if (!pGetNumberOfConsoleFonts)
    {
        win_skip("GetNumberOfConsoleFonts is not available\n");
        return;
    }

    num_fonts = pGetNumberOfConsoleFonts();
    memsize = num_fonts * sizeof(CONSOLE_FONT_INFO);
    cfi = HeapAlloc(GetProcessHeap(), 0, memsize);
    memset(cfi, 0, memsize);

    GetConsoleScreenBufferInfo(std_output, &csbi);
    orig_sb_size = csbi.dwSize;
    tmp_sb_size.X = csbi.dwSize.X + 3;
    tmp_sb_size.Y = csbi.dwSize.Y + 5;
    SetConsoleScreenBufferSize(std_output, tmp_sb_size);

    SetLastError(0xdeadbeef);
    ret = pGetConsoleFontInfo(NULL, FALSE, 0, cfi);
    ok(!ret, "got %d, expected zero\n", ret);
    if (GetLastError() == LOWORD(E_NOTIMPL) /* win10 1709+ */ ||
        broken(GetLastError() == ERROR_GEN_FAILURE) /* win10 1607 */)
    {
        skip("GetConsoleFontInfo is not implemented\n");
        SetConsoleScreenBufferSize(std_output, orig_sb_size);
        HeapFree(GetProcessHeap(), 0, cfi);
        return;
    }
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetConsoleFontInfo(GetStdHandle(STD_INPUT_HANDLE), FALSE, 0, cfi);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetConsoleFontInfo(std_output, FALSE, 0, cfi);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());

    GetConsoleScreenBufferInfo(std_output, &csbi);
    win_width = csbi.srWindow.Right - csbi.srWindow.Left + 1;
    win_height = csbi.srWindow.Bottom - csbi.srWindow.Top + 1;

    GetCurrentConsoleFont(std_output, FALSE, &cfi[0]);
    index = cfi[0].nFont;
    orig_font = GetConsoleFontSize(std_output, index);

    memset(cfi, 0, memsize);
    ret = pGetConsoleFontInfo(std_output, FALSE, num_fonts, cfi);
    ok(ret, "got %d, expected non-zero\n", ret);
    ok(cfi[index].dwFontSize.X == win_width, "got %d, expected %d\n",
       cfi[index].dwFontSize.X, win_width);
    ok(cfi[index].dwFontSize.Y == win_height, "got %d, expected %d\n",
       cfi[index].dwFontSize.Y, win_height);

    for (i = 0; i < num_fonts; i++)
    {
        ok(cfi[i].nFont == i, "element out of order, got nFont %ld, expected %ld\n", cfi[i].nFont, i);
        tmp_font = GetConsoleFontSize(std_output, cfi[i].nFont);
        tmp_w = (double)orig_font.X / tmp_font.X * win_width;
        tmp_h = (double)orig_font.Y / tmp_font.Y * win_height;
        ok(cfi[i].dwFontSize.X == tmp_w, "got %d, expected %d\n", cfi[i].dwFontSize.X, tmp_w);
        ok(cfi[i].dwFontSize.Y == tmp_h, "got %d, expected %d\n", cfi[i].dwFontSize.Y, tmp_h);
    }

    SetLastError(0xdeadbeef);
    ret = pGetConsoleFontInfo(NULL, TRUE, 0, cfi);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetConsoleFontInfo(GetStdHandle(STD_INPUT_HANDLE), TRUE, 0, cfi);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetConsoleFontInfo(std_output, TRUE, 0, cfi);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());

    memset(cfi, 0, memsize);
    ret = pGetConsoleFontInfo(std_output, TRUE, num_fonts, cfi);
    ok(ret, "got %d, expected non-zero\n", ret);
    ok(cfi[index].dwFontSize.X == csbi.dwMaximumWindowSize.X, "got %d, expected %d\n",
       cfi[index].dwFontSize.X, csbi.dwMaximumWindowSize.X);
    ok(cfi[index].dwFontSize.Y == csbi.dwMaximumWindowSize.Y, "got %d, expected %d\n",
       cfi[index].dwFontSize.Y, csbi.dwMaximumWindowSize.Y);

    for (i = 0; i < num_fonts; i++)
    {
        ok(cfi[i].nFont == i, "element out of order, got nFont %ld, expected %ld\n", cfi[i].nFont, i);
        tmp_font = GetConsoleFontSize(std_output, cfi[i].nFont);
        tmp_w = (double)orig_font.X / tmp_font.X * csbi.dwMaximumWindowSize.X;
        tmp_h = (double)orig_font.Y / tmp_font.Y * csbi.dwMaximumWindowSize.Y;
        ok(cfi[i].dwFontSize.X == tmp_w, "got %d, expected %d\n", cfi[i].dwFontSize.X, tmp_w);
        ok(cfi[i].dwFontSize.Y == tmp_h, "got %d, expected %d\n", cfi[i].dwFontSize.Y, tmp_h);
     }

    HeapFree(GetProcessHeap(), 0, cfi);
    SetConsoleScreenBufferSize(std_output, orig_sb_size);
}

static void test_SetConsoleFont(HANDLE std_output)
{
    HANDLE hmod;
    BOOL (WINAPI *pSetConsoleFont)(HANDLE, DWORD);
    BOOL ret;
    DWORD (WINAPI *pGetNumberOfConsoleFonts)(void);
    DWORD num_fonts;

    hmod = GetModuleHandleA("kernel32.dll");
    pSetConsoleFont = (void *)GetProcAddress(hmod, "SetConsoleFont");
    if (!pSetConsoleFont)
    {
        win_skip("SetConsoleFont is not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pSetConsoleFont(NULL, 0);
    ok(!ret, "got %d, expected zero\n", ret);
    if (GetLastError() == LOWORD(E_NOTIMPL) /* win10 1709+ */ ||
        broken(GetLastError() == ERROR_GEN_FAILURE) /* win10 1607 */)
    {
        skip("SetConsoleFont is not implemented\n");
        return;
    }
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pSetConsoleFont(GetStdHandle(STD_INPUT_HANDLE), 0);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    pGetNumberOfConsoleFonts = (void *)GetProcAddress(hmod, "GetNumberOfConsoleFonts");
    if (!pGetNumberOfConsoleFonts)
    {
        win_skip("GetNumberOfConsoleFonts is not available\n");
        return;
    }

    num_fonts = pGetNumberOfConsoleFonts();

    SetLastError(0xdeadbeef);
    ret = pSetConsoleFont(std_output, num_fonts);
    ok(!ret, "got %d, expected zero\n", ret);
    todo_wine ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());
}

static void test_GetConsoleScreenBufferInfoEx(HANDLE std_output)
{
    HANDLE hmod;
    BOOL (WINAPI *pGetConsoleScreenBufferInfoEx)(HANDLE, CONSOLE_SCREEN_BUFFER_INFOEX *);
    CONSOLE_SCREEN_BUFFER_INFOEX csbix;
    HANDLE pipe1, pipe2;
    BOOL ret;
    HANDLE std_input = GetStdHandle(STD_INPUT_HANDLE);

    hmod = GetModuleHandleA("kernel32.dll");
    pGetConsoleScreenBufferInfoEx = (void *)GetProcAddress(hmod, "GetConsoleScreenBufferInfoEx");
    if (!pGetConsoleScreenBufferInfoEx)
    {
        win_skip("GetConsoleScreenBufferInfoEx is not available\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pGetConsoleScreenBufferInfoEx(NULL, &csbix);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetConsoleScreenBufferInfoEx(std_input, &csbix);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetConsoleScreenBufferInfoEx(std_output, &csbix);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    csbix.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);

    SetLastError(0xdeadbeef);
    ret = pGetConsoleScreenBufferInfoEx(NULL, &csbix);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetConsoleScreenBufferInfoEx(std_input, &csbix);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    CreatePipe(&pipe1, &pipe2, NULL, 0);
    SetLastError(0xdeadbeef);
    ret = pGetConsoleScreenBufferInfoEx(std_input, &csbix);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());
    CloseHandle(pipe1);
    CloseHandle(pipe2);

    SetLastError(0xdeadbeef);
    ret = pGetConsoleScreenBufferInfoEx(std_output, &csbix);
    ok(ret, "got %d, expected non-zero\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());
}

static void test_FreeConsole(void)
{
    HANDLE handle, unbound_output = NULL, unbound_input = NULL;
    DWORD size, mode, type;
    WCHAR title[16];
    char buf[32];
    HWND hwnd;
    UINT cp;
    BOOL ret;

    ok(RtlGetCurrentPeb()->ProcessParameters->ConsoleHandle != NULL, "ConsoleHandle is NULL\n");
    ok(!SetConsoleCtrlHandler(mydummych, FALSE), "dummy ctrl handler shouldn't be set\n");
    ret = SetConsoleCtrlHandler(mydummych, TRUE);
    ok(ret, "SetConsoleCtrlHandler failed: %lu\n", GetLastError());
    if (!skip_nt)
    {
        unbound_input  = create_unbound_handle(FALSE, TRUE);
        unbound_output = create_unbound_handle(TRUE, TRUE);
    }

    ret = FreeConsole();
    ok(ret, "FreeConsole failed: %lu\n", GetLastError());

    ok(RtlGetCurrentPeb()->ProcessParameters->ConsoleHandle == NULL, "ConsoleHandle = %p\n",
       RtlGetCurrentPeb()->ProcessParameters->ConsoleHandle);

    handle = CreateFileA("CONOUT$", GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(handle == INVALID_HANDLE_VALUE &&
       (GetLastError() == ERROR_INVALID_HANDLE || broken(GetLastError() == ERROR_ACCESS_DENIED /* winxp */)),
       "CreateFileA failed: %lu\n", GetLastError());

    handle = CreateFileA("CONIN$", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(handle == INVALID_HANDLE_VALUE &&
       (GetLastError() == ERROR_INVALID_HANDLE || broken(GetLastError() == ERROR_ACCESS_DENIED /* winxp */)),
       "CreateFileA failed: %lu\n", GetLastError());

    handle = CreateFileA("CON", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(handle == INVALID_HANDLE_VALUE &&
       (GetLastError() == ERROR_INVALID_HANDLE || broken(GetLastError() == ERROR_ACCESS_DENIED /* winxp */)),
       "CreateFileA failed: %lu\n", GetLastError());

    handle = CreateFileA("CON", GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(handle == INVALID_HANDLE_VALUE &&
       (GetLastError() == ERROR_INVALID_HANDLE || broken(GetLastError() == ERROR_FILE_NOT_FOUND /* winxp */)),
       "CreateFileA failed: %lu\n", GetLastError());

    handle = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE,
                                       FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                       CONSOLE_TEXTMODE_BUFFER, NULL);
    ok(handle == INVALID_HANDLE_VALUE && GetLastError() == ERROR_INVALID_HANDLE,
       "CreateConsoleScreenBuffer returned: %p (%lu)\n", handle, GetLastError());

    SetLastError(0xdeadbeef);
    cp = GetConsoleCP();
    ok(!cp, "cp = %x\n", cp);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "last error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    cp = GetConsoleOutputCP();
    ok(!cp, "cp = %x\n", cp);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "last error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetConsoleCP(GetOEMCP());
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "SetConsoleCP returned %x(%lu)\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetConsoleOutputCP(GetOEMCP());
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "SetConsoleCP returned %x(%lu)\n", ret, GetLastError());

    if (skip_nt) return;

    SetLastError(0xdeadbeef);
    memset( title, 0xc0, sizeof(title) );
    size = GetConsoleTitleW( title, ARRAY_SIZE(title) );
    ok(!size, "GetConsoleTitleW returned %lu\n", size);
    ok(title[0] == 0xc0c0, "title byffer changed\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "last error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = SetConsoleTitleW( L"test" );
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "SetConsoleTitleW returned %x(%lu)\n", ret, GetLastError());

    SetLastError(0xdeadbeef);
    hwnd = GetConsoleWindow();
    ok(!hwnd, "hwnd = %p\n", hwnd);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "last error %lu\n", GetLastError());

    ret = GenerateConsoleCtrlEvent(CTRL_C_EVENT, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "GenerateConsoleCtrlEvent returned %x(%lu)\n",
       ret, GetLastError());

    SetStdHandle( STD_INPUT_HANDLE, (HANDLE)0xdeadbeef );
    handle = GetConsoleInputWaitHandle();
    ok(handle == (HANDLE)0xdeadbeef, "GetConsoleInputWaitHandle returned %p\n", handle);
    SetStdHandle( STD_INPUT_HANDLE, NULL );
    handle = GetConsoleInputWaitHandle();
    ok(!handle, "GetConsoleInputWaitHandle returned %p\n", handle);

    ret = ReadFile(unbound_input, buf, sizeof(buf), &size, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
       "ReadFile returned %x %lu\n", ret, GetLastError());

    ret = FlushFileBuffers(unbound_input);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
       "ReadFile returned %x %lu\n", ret, GetLastError());

    ret = WriteFile(unbound_input, "test", 4, &size, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
       "ReadFile returned %x %lu\n", ret, GetLastError());

    ret = GetConsoleMode(unbound_input, &mode);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
       "GetConsoleMode returned %x %lu\n", ret, GetLastError());
    ret = GetConsoleMode(unbound_output, &mode);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE,
       "GetConsoleMode returned %x %lu\n", ret, GetLastError());

    type = GetFileType(unbound_input);
    ok(type == FILE_TYPE_CHAR, "GetFileType returned %lu\n", type);
    type = GetFileType(unbound_output);
    ok(type == FILE_TYPE_CHAR, "GetFileType returned %lu\n", type);

    todo_wine
    ok(!SetConsoleCtrlHandler(mydummych, FALSE), "FreeConsole() should have reset ctrl handlers' list\n");

    CloseHandle(unbound_input);
    CloseHandle(unbound_output);
}

#ifndef __REACTOS__ // Can't link
static void test_SetConsoleScreenBufferInfoEx(HANDLE std_output)
{
    BOOL ret;
    HANDLE hmod;
    HANDLE std_input = CreateFileA("CONIN$", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    BOOL (WINAPI *pSetConsoleScreenBufferInfoEx)(HANDLE, CONSOLE_SCREEN_BUFFER_INFOEX *);
    BOOL (WINAPI *pGetConsoleScreenBufferInfoEx)(HANDLE, CONSOLE_SCREEN_BUFFER_INFOEX *);
    CONSOLE_SCREEN_BUFFER_INFOEX info;

    hmod = GetModuleHandleA("kernel32.dll");
    pSetConsoleScreenBufferInfoEx = (void *)GetProcAddress(hmod, "SetConsoleScreenBufferInfoEx");
    pGetConsoleScreenBufferInfoEx = (void *)GetProcAddress(hmod, "GetConsoleScreenBufferInfoEx");
    if (!pSetConsoleScreenBufferInfoEx || !pGetConsoleScreenBufferInfoEx)
    {
        win_skip("SetConsoleScreenBufferInfoEx is not available\n");
        return;
    }

    memset(&info, 0, sizeof(CONSOLE_SCREEN_BUFFER_INFOEX));
    info.cbSize = sizeof(CONSOLE_SCREEN_BUFFER_INFOEX);
    pGetConsoleScreenBufferInfoEx(std_output, &info);

    SetLastError(0xdeadbeef);
    ret = pSetConsoleScreenBufferInfoEx(NULL, &info);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE, "got %lu, expected 6\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pSetConsoleScreenBufferInfoEx(std_output, &info);
    ok(ret, "got %d, expected one\n", ret);
    ok(GetLastError() == 0xdeadbeef, "got %lu, expected 0xdeadbeef\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pSetConsoleScreenBufferInfoEx(std_input, &info);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_HANDLE || GetLastError() == ERROR_ACCESS_DENIED,
            "got %lu, expected 5 or 6\n", GetLastError());

    info.cbSize = 0;
    SetLastError(0xdeadbeef);
    ret = pSetConsoleScreenBufferInfoEx(std_output, &info);
    ok(!ret, "got %d, expected zero\n", ret);
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "got %lu, expected 87\n", GetLastError());

    CloseHandle(std_input);
}

static void test_GetConsoleOriginalTitleA(void)
{
    char title[] = "Original Console Title";
    char buf[64];
    DWORD ret, title_len = strlen(title);

    ret = GetConsoleOriginalTitleA(NULL, 0);
    ok(!ret, "Unexpected string length; error %lu\n", GetLastError());

    ret = GetConsoleOriginalTitleA(buf, 0);
    ok(!ret, "Unexpected string length; error %lu\n", GetLastError());

    ret = GetConsoleOriginalTitleA(buf, ARRAY_SIZE(buf));
    ok(ret, "GetConsoleOriginalTitleA failed: %lu\n", GetLastError());
    ok(!strcmp(buf, title), "got %s, expected %s\n", wine_dbgstr_a(buf), wine_dbgstr_a(title));
    ok(ret == title_len, "got %lu, expected %lu\n", ret, title_len);

    ret = SetConsoleTitleA("test");
    ok(ret, "SetConsoleTitleA failed: %lu\n", GetLastError());

    ret = GetConsoleOriginalTitleA(buf, ARRAY_SIZE(buf));
    ok(ret, "GetConsoleOriginalTitleA failed: %lu\n", GetLastError());
    ok(!strcmp(buf, title), "got %s, expected %s\n", wine_dbgstr_a(buf), wine_dbgstr_a(title));
    ok(ret == title_len, "got %lu, expected %lu\n", ret, title_len);
}

static void test_GetConsoleOriginalTitleW(void)
{
    WCHAR title[] = L"Original Console Title";
    WCHAR buf[64];
    DWORD ret, title_len = lstrlenW(title);

    ret = GetConsoleOriginalTitleW(NULL, 0);
    ok(!ret, "Unexpected string length; error %lu\n", GetLastError());

    ret = GetConsoleOriginalTitleW(buf, 0);
    ok(!ret, "Unexpected string length; error %lu\n", GetLastError());

    ret = GetConsoleOriginalTitleW(buf, ARRAY_SIZE(buf));
    ok(ret, "GetConsoleOriginalTitleW failed: %lu\n", GetLastError());
    buf[ret] = 0;
    ok(!wcscmp(buf, title), "got %s, expected %s\n", wine_dbgstr_w(buf), wine_dbgstr_w(title));
    ok(ret == title_len, "got %lu, expected %lu\n", ret, title_len);

    ret = SetConsoleTitleW(L"test");
    ok(ret, "SetConsoleTitleW failed: %lu\n", GetLastError());

    ret = GetConsoleOriginalTitleW(buf, ARRAY_SIZE(buf));
    ok(ret, "GetConsoleOriginalTitleW failed: %lu\n", GetLastError());
    ok(!wcscmp(buf, title), "got %s, expected %s\n", wine_dbgstr_w(buf), wine_dbgstr_w(title));
    ok(ret == title_len, "got %lu, expected %lu\n", ret, title_len);

    ret = GetConsoleOriginalTitleW(buf, 5);
    ok(ret, "GetConsoleOriginalTitleW failed: %lu\n", GetLastError());
    ok(!wcscmp(buf, L"Orig"), "got %s, expected 'Orig'\n", wine_dbgstr_w(buf));
    ok(ret == title_len, "got %lu, expected %lu\n", ret, title_len);
}
#endif

static void test_GetConsoleOriginalTitleW_empty(void)
{
    WCHAR buf[64];
    DWORD ret;

    ret = GetConsoleOriginalTitleW(buf, ARRAY_SIZE(buf));
    ok(!ret, "GetConsoleOriginalTitleW failed: %lu\n", GetLastError());
}

static void test_GetConsoleOriginalTitle(void)
{
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION info;
    char **argv, buf[MAX_PATH];
    char title[] = "Original Console Title";
    BOOL ret;

    winetest_get_mainargs(&argv);
    sprintf(buf, "\"%s\" console title_test", argv[0]);
    si.lpTitle = title;
    ret = CreateProcessA(NULL, buf, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &info);
    ok(ret, "CreateProcess failed: %lu\n", GetLastError());
    CloseHandle(info.hThread);
    wait_child_process(info.hProcess);
    CloseHandle(info.hProcess);

    strcat(buf, " empty");
    title[0] = 0;
    ret = CreateProcessA(NULL, buf, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &info);
    ok(ret, "CreateProcess failed: %lu\n", GetLastError());
    CloseHandle(info.hThread);
    wait_child_process(info.hProcess);
    CloseHandle(info.hProcess);
}

static void test_GetConsoleTitleA(void)
{
    char buf[64], str[] = "test";
    DWORD ret;

    ret = SetConsoleTitleA(str);
    ok(ret, "SetConsoleTitleA failed: %lu\n", GetLastError());

    ret = GetConsoleTitleA(NULL, 0);
    ok(!ret, "Unexpected string length; error %lu\n", GetLastError());

    ret = GetConsoleTitleA(buf, 0);
    ok(!ret, "Unexpected string length; error %lu\n", GetLastError());

    ret = GetConsoleTitleA(buf, ARRAY_SIZE(buf));
    ok(ret, "GetConsoleTitleW failed: %lu\n", GetLastError());
    ok(ret == strlen(str), "Got string length %lu, expected %Iu\n", ret, strlen(str));
    ok(!strcmp(buf, str), "Title = %s\n", wine_dbgstr_a(buf));

    ret = SetConsoleTitleA("");
    ok(ret, "SetConsoleTitleA failed: %lu\n", GetLastError());

    ret = GetConsoleTitleA(buf, ARRAY_SIZE(buf));
    ok(!ret, "Unexpected string length; error %lu\n", GetLastError());
}

static void test_GetConsoleTitleW(void)
{
    WCHAR buf[64], str[] = L"test";
    DWORD ret;

    ret = SetConsoleTitleW(str);
    ok(ret, "SetConsoleTitleW failed: %lu\n", GetLastError());

    ret = GetConsoleTitleW(NULL, 0);
    ok(!ret, "Unexpected string length; error %lu\n", GetLastError());

    ret = GetConsoleTitleW(buf, 0);
    ok(!ret, "Unexpected string length; error %lu\n", GetLastError());

    ret = GetConsoleTitleW(buf, ARRAY_SIZE(buf));
    ok(ret, "GetConsoleTitleW failed: %lu\n", GetLastError());
    ok(ret == wcslen(str), "Got string length %lu, expected %Iu\n", ret, wcslen(str));
    ok(!wcscmp(buf, str), "Title = %s\n", wine_dbgstr_w(buf));

    ret = GetConsoleTitleW(buf, 2);
    ok(ret, "GetConsoleTitleW failed: %lu\n", GetLastError());
    ok(ret == wcslen(str), "Got string length %lu, expected %Iu\n", ret, wcslen(str));
    if (!skip_nt) ok(!wcscmp(buf, L"t"), "Title = %s\n", wine_dbgstr_w(buf));

    ret = GetConsoleTitleW(buf, 4);
    ok(ret, "GetConsoleTitleW failed: %lu\n", GetLastError());
    ok(ret == wcslen(str), "Got string length %lu, expected %Iu\n", ret, wcslen(str));
    if (!skip_nt) ok(!wcscmp(buf, L"tes"), "Title = %s\n", wine_dbgstr_w(buf));

    ret = SetConsoleTitleW(L"");
    ok(ret, "SetConsoleTitleW failed: %lu\n", GetLastError());

    ret = GetConsoleTitleW(buf, ARRAY_SIZE(buf));
    ok(!ret, "Unexpected string length; error %lu\n", GetLastError());
}

static void test_file_info(HANDLE input, HANDLE output)
{
    FILE_STANDARD_INFORMATION std_info;
    FILE_FS_DEVICE_INFORMATION fs_info;
    LARGE_INTEGER size;
    IO_STATUS_BLOCK io;
    DWORD type;
    NTSTATUS status;
    BOOL ret;

    if (skip_nt) return;

    status = NtQueryInformationFile(input, &io, &std_info, sizeof(std_info), FileStandardInformation);
    ok(status == STATUS_INVALID_DEVICE_REQUEST, "NtQueryInformationFile returned: %#lx\n", status);

    status = NtQueryInformationFile(output, &io, &std_info, sizeof(std_info), FileStandardInformation);
    ok(status == STATUS_INVALID_DEVICE_REQUEST, "NtQueryInformationFile returned: %#lx\n", status);

    ret = GetFileSizeEx(input, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_FUNCTION,
       "GetFileSizeEx returned %x(%lu)\n", ret, GetLastError());

    ret = GetFileSizeEx(output, &size);
    ok(!ret && GetLastError() == ERROR_INVALID_FUNCTION,
       "GetFileSizeEx returned %x(%lu)\n", ret, GetLastError());

    status = NtQueryVolumeInformationFile(input, &io, &fs_info, sizeof(fs_info), FileFsDeviceInformation);
    ok(!status, "NtQueryVolumeInformationFile failed: %#lx\n", status);
    ok(fs_info.DeviceType == FILE_DEVICE_CONSOLE, "DeviceType = %lu\n", fs_info.DeviceType);
    ok(fs_info.Characteristics == FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL,
       "Characteristics = %lx\n", fs_info.Characteristics);

    status = NtQueryVolumeInformationFile(output, &io, &fs_info, sizeof(fs_info), FileFsDeviceInformation);
    ok(!status, "NtQueryVolumeInformationFile failed: %#lx\n", status);
    ok(fs_info.DeviceType == FILE_DEVICE_CONSOLE, "DeviceType = %lu\n", fs_info.DeviceType);
    ok(fs_info.Characteristics == FILE_DEVICE_ALLOW_APPCONTAINER_TRAVERSAL,
       "Characteristics = %lx\n", fs_info.Characteristics);

    type = GetFileType(input);
    ok(type == FILE_TYPE_CHAR, "GetFileType returned %lu\n", type);
    type = GetFileType(output);
    ok(type == FILE_TYPE_CHAR, "GetFileType returned %lu\n", type);
}

static void test_console_as_root_directory(void)
{
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING name;
    HANDLE handle, h2;
    NTSTATUS status;

    handle = CreateFileA( "CON", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0 );
    ok( handle != INVALID_HANDLE_VALUE, "CreateFileA error %lu\n", GetLastError() );

    RtlInitUnicodeString( &name, L"" );
    InitializeObjectAttributes( &attr, &name, 0, handle, NULL );
    status = NtCreateFile( &h2, SYNCHRONIZE, &attr, &iosb, NULL, 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN, 0, NULL, 0 );
    ok( status == STATUS_NOT_FOUND || broken( status == STATUS_OBJECT_TYPE_MISMATCH ) /* Win7 */,
        "NtCreateFile returned %#lx\n", status );

    CloseHandle( handle );
}

static void test_condrv_server_as_root_directory(void)
{
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK iosb;
    UNICODE_STRING name;
    HANDLE handle, h2;
    NTSTATUS status;

    FreeConsole();

    RtlInitUnicodeString( &name, L"\\Device\\ConDrv\\Server" );
    InitializeObjectAttributes( &attr, &name, 0, NULL, NULL );
    status = NtCreateFile( &handle, SYNCHRONIZE, &attr, &iosb, NULL, 0,
                           FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                           FILE_OPEN, 0, NULL, 0 );
    ok( !status || broken( status == STATUS_OBJECT_PATH_NOT_FOUND ) /* Win7 */,
        "NtCreateFile returned %#lx\n", status );

    if (status)
    {
        win_skip( "cannot open \\Device\\ConDrv\\Server, skipping RootDirectory test" );
    }
    else
    {
        RtlInitUnicodeString( &name, L"" );
        InitializeObjectAttributes( &attr, &name, 0, handle, NULL );
        status = NtCreateFile( &h2, SYNCHRONIZE, &attr, &iosb, NULL, 0,
                               FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
                               FILE_OPEN, 0, NULL, 0 );
        ok( status == STATUS_NOT_FOUND, "NtCreateFile returned %#lx\n", status );

        CloseHandle( handle );
    }
}

static void test_AttachConsole_child(DWORD console_pid)
{
    HANDLE pipe_in, pipe_out;
    COORD c = {0,0};
    HANDLE console;
    char buf[32];
    DWORD len;
    BOOL res;

    res = CreatePipe(&pipe_in, &pipe_out, NULL, 0);
    ok(res, "CreatePipe failed: %lu\n", GetLastError());

    res = AttachConsole(console_pid);
    ok(!res && GetLastError() == ERROR_ACCESS_DENIED,
       "AttachConsole returned: %x(%lu)\n", res, GetLastError());

    ok(RtlGetCurrentPeb()->ProcessParameters->ConsoleHandle != NULL, "ConsoleHandle is NULL\n");
    res = FreeConsole();
    ok(res, "FreeConsole failed: %lu\n", GetLastError());
    ok(RtlGetCurrentPeb()->ProcessParameters->ConsoleHandle == NULL, "ConsoleHandle = %p\n",
       RtlGetCurrentPeb()->ProcessParameters->ConsoleHandle);

    SetStdHandle(STD_ERROR_HANDLE, pipe_out);

    ok(!SetConsoleCtrlHandler(mydummych, FALSE), "dummy ctrl handler shouldn't be set\n");
    res = SetConsoleCtrlHandler(mydummych, TRUE);
    ok(res, "SetConsoleCtrlHandler failed: %lu\n", GetLastError());

    res = AttachConsole(console_pid);
    ok(res, "AttachConsole failed: %lu\n", GetLastError());

    ok(pipe_out != GetStdHandle(STD_ERROR_HANDLE), "std handle not set to console\n");
    ok(RtlGetCurrentPeb()->ProcessParameters->ConsoleHandle != NULL, "ConsoleHandle is NULL\n");

    console = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(console != INVALID_HANDLE_VALUE, "Could not open console\n");

    res = ReadConsoleOutputCharacterA(console, buf, 6, c, &len);
    ok(res, "ReadConsoleOutputCharacterA failed: %lu\n", GetLastError());
    ok(len == 6, "len = %lu\n", len);
    ok(!memcmp(buf, "Parent", 6), "Unexpected console output\n");

    todo_wine
    ok(!SetConsoleCtrlHandler(mydummych, FALSE), "AttachConsole() should have reset ctrl handlers' list\n");

    res = FreeConsole();
    ok(res, "FreeConsole failed: %lu\n", GetLastError());

    SetStdHandle(STD_INPUT_HANDLE, pipe_in);
    SetStdHandle(STD_OUTPUT_HANDLE, pipe_out);

    res = AttachConsole(ATTACH_PARENT_PROCESS);
    ok(res, "AttachConsole failed: %lu\n", GetLastError());

    if (pGetConsoleProcessList)
    {
        DWORD list[2] = { 0xbabebabe };
        DWORD pid = GetCurrentProcessId();

        SetLastError(0xdeadbeef);
        len = pGetConsoleProcessList(list, 1);
        ok(len == 2, "Expected 2 processes, got %ld\n", len);
        ok(list[0] == 0xbabebabe, "Unexpected value in list %lu\n", list[0]);

        len = pGetConsoleProcessList(list, 2);
        ok(len == 2, "Expected 2 processes, got %ld\n", len);
        ok(list[0] == console_pid || list[1] == console_pid, "Parent PID not in list\n");
        ok(list[0] == pid || list[1] == pid, "PID not in list\n");
        ok(GetLastError() == 0xdeadbeef, "Unexpected last error: %lu\n", GetLastError());
    }

    ok(pipe_in != GetStdHandle(STD_INPUT_HANDLE), "std handle not set to console\n");
    ok(pipe_out != GetStdHandle(STD_OUTPUT_HANDLE), "std handle not set to console\n");

    console = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(console != INVALID_HANDLE_VALUE, "Could not open console\n");

    res = ReadConsoleOutputCharacterA(console, buf, 6, c, &len);
    ok(res, "ReadConsoleOutputCharacterA failed: %lu\n", GetLastError());
    ok(len == 6, "len = %lu\n", len);
    ok(!memcmp(buf, "Parent", 6), "Unexpected console output\n");

    simple_write_console(console, "Child");
    CloseHandle(console);

    res = FreeConsole();
    ok(res, "FreeConsole failed: %lu\n", GetLastError());

    res = CloseHandle(pipe_in);
    ok(res, "pipe_in is no longer valid\n");
    res = CloseHandle(pipe_out);
    ok(res, "pipe_out is no longer valid\n");
}

static void test_AttachConsole(HANDLE console)
{
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION info;
    char **argv, buf[MAX_PATH];
    COORD c = {0,0};
    DWORD len;
    BOOL res;

    simple_write_console(console, "Parent console");

    winetest_get_mainargs(&argv);
    sprintf(buf, "\"%s\" console attach_console %lx", argv[0], GetCurrentProcessId());
    res = CreateProcessA(NULL, buf, NULL, NULL, TRUE, 0, NULL, NULL, &si, &info);
    ok(res, "CreateProcess failed: %lu\n", GetLastError());
    CloseHandle(info.hThread);

    wait_child_process(info.hProcess);
    CloseHandle(info.hProcess);

    res = ReadConsoleOutputCharacterA(console, buf, 5, c, &len);
    ok(res, "ReadConsoleOutputCharacterA failed: %lu\n", GetLastError());
    ok(len == 5, "len = %lu\n", len);
    ok(!memcmp(buf, "Child", 5), "Unexpected console output\n");
}

static void test_AllocConsole_child(void)
{
    HANDLE unbound_output;
    HANDLE prev_output, prev_error;
    STARTUPINFOW si;
    DWORD mode;
    BOOL res;

    GetStartupInfoW(&si);

    prev_output = GetStdHandle(STD_OUTPUT_HANDLE);
    res = DuplicateHandle(GetCurrentProcess(), prev_output, GetCurrentProcess(), &unbound_output,
                          0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(res, "DuplicateHandle failed: %lu\n", GetLastError());

    res = GetConsoleMode(unbound_output, &mode);
    ok(res, "GetConsoleMode failed: %lu\n", GetLastError());

    prev_error = GetStdHandle(STD_ERROR_HANDLE);
    if (si.dwFlags & STARTF_USESTDHANDLES)
    {
        res = GetConsoleMode(prev_error, &mode);
        ok(!res && GetLastError() == ERROR_INVALID_HANDLE, "GetConsoleMode failed: %lu\n", GetLastError());
    }

    FreeConsole();

    ok(GetStdHandle(STD_OUTPUT_HANDLE) == prev_output, "GetStdHandle(STD_OUTPUT_HANDLE) = %p\n", GetStdHandle(STD_OUTPUT_HANDLE));
    ok(GetStdHandle(STD_ERROR_HANDLE) == prev_error, "GetStdHandle(STD_ERROR_HANDLE) = %p\n", GetStdHandle(STD_ERROR_HANDLE));
    res = GetConsoleMode(unbound_output, &mode);
    ok(!res && GetLastError() == ERROR_INVALID_HANDLE, "GetConsoleMode failed: %lu\n", GetLastError());

    ok(!SetConsoleCtrlHandler(mydummych, FALSE), "dummy ctrl handler shouldn't be set\n");
    res = SetConsoleCtrlHandler(mydummych, TRUE);
    ok(res, "SetConsoleCtrlHandler failed: %lu\n", GetLastError());
    res = AllocConsole();
    ok(res, "AllocConsole failed: %lu\n", GetLastError());

    if (si.dwFlags & STARTF_USESTDHANDLES)
    {
        ok(GetStdHandle(STD_OUTPUT_HANDLE) == prev_output, "GetStdHandle(STD_OUTPUT_HANDLE) = %p\n", GetStdHandle(STD_OUTPUT_HANDLE));
        ok(GetStdHandle(STD_ERROR_HANDLE) == prev_error, "GetStdHandle(STD_ERROR_HANDLE) = %p\n", GetStdHandle(STD_ERROR_HANDLE));
    }

    res = GetConsoleMode(unbound_output, &mode);
    ok(res, "GetConsoleMode failed: %lu\n", GetLastError());

    todo_wine
    ok(!SetConsoleCtrlHandler(mydummych, FALSE), "AllocConsole() should have reset ctrl handlers' list\n");

    FreeConsole();
    SetStdHandle(STD_OUTPUT_HANDLE, NULL);
    SetStdHandle(STD_ERROR_HANDLE, NULL);
    res = AllocConsole();
    ok(res, "AllocConsole failed: %lu\n", GetLastError());

    ok(GetStdHandle(STD_OUTPUT_HANDLE) != NULL, "GetStdHandle(STD_OUTPUT_HANDLE) = %p\n", GetStdHandle(STD_OUTPUT_HANDLE));
    ok(GetStdHandle(STD_ERROR_HANDLE) != NULL, "GetStdHandle(STD_ERROR_HANDLE) = %p\n", GetStdHandle(STD_ERROR_HANDLE));

    res = GetConsoleMode(unbound_output, &mode);
    ok(res, "GetConsoleMode failed: %lu\n", GetLastError());
    res = GetConsoleMode(GetStdHandle(STD_OUTPUT_HANDLE), &mode);
    ok(res, "GetConsoleMode failed: %lu\n", GetLastError());
    res = GetConsoleMode(GetStdHandle(STD_ERROR_HANDLE), &mode);
    ok(res, "GetConsoleMode failed: %lu\n", GetLastError());

    res = CloseHandle(unbound_output);
    ok(res, "CloseHandle failed: %lu\n", GetLastError());
}

static void test_AllocConsole(void)
{
    SECURITY_ATTRIBUTES inheritable_attr = { sizeof(inheritable_attr), NULL, TRUE };
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION info;
    char **argv, buf[MAX_PATH];
    HANDLE pipe_read, pipe_write;
    BOOL res;

    if (skip_nt) return;

    winetest_get_mainargs(&argv);
    sprintf(buf, "\"%s\" console alloc_console", argv[0]);
    res = CreateProcessA(NULL, buf, NULL, NULL, FALSE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &info);
    ok(res, "CreateProcess failed: %lu\n", GetLastError());
    CloseHandle(info.hThread);
    wait_child_process(info.hProcess);
    CloseHandle(info.hProcess);

    res = CreatePipe(&pipe_read, &pipe_write, &inheritable_attr, 0);
    ok(res, "CreatePipe failed: %lu\n", GetLastError());

    si.dwFlags = STARTF_USESTDHANDLES;
    si.hStdError = pipe_write;
    res = CreateProcessA(NULL, buf, NULL, NULL, TRUE, CREATE_NEW_CONSOLE, NULL, NULL, &si, &info);
    ok(res, "CreateProcess failed: %lu\n", GetLastError());
    CloseHandle(info.hThread);
    wait_child_process(info.hProcess);
    CloseHandle(info.hProcess);

    CloseHandle(pipe_read);
    CloseHandle(pipe_write);
}

static void test_pseudo_console_child(HANDLE input, HANDLE output)
{
    CONSOLE_SCREEN_BUFFER_INFO sb_info;
    CONSOLE_CURSOR_INFO cursor_info;
    DWORD mode;
    HWND hwnd;
    BOOL ret;

    ret = GetConsoleMode(input, &mode);
    ok(ret, "GetConsoleMode failed: %lu\n", GetLastError());
    ok(mode == (ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT |
                ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS | ENABLE_AUTO_POSITION),
       "mode = %lx\n", mode);

    ret = SetConsoleMode(input, mode & ~ENABLE_AUTO_POSITION);
    ok(ret, "SetConsoleMode failed: %lu\n", GetLastError());

    ret = GetConsoleMode(input, &mode);
    ok(ret, "GetConsoleMode failed: %lu\n", GetLastError());
    ok(mode == (ENABLE_PROCESSED_INPUT | ENABLE_LINE_INPUT | ENABLE_ECHO_INPUT | ENABLE_MOUSE_INPUT |
                ENABLE_INSERT_MODE | ENABLE_QUICK_EDIT_MODE | ENABLE_EXTENDED_FLAGS), "mode = %lx\n", mode);

    ret = SetConsoleMode(input, mode | ENABLE_AUTO_POSITION);
    ok(ret, "SetConsoleMode failed: %lu\n", GetLastError());

    ret = GetConsoleMode(output, &mode);
    ok(ret, "GetConsoleMode failed: %lu\n", GetLastError());
    mode &= ~ENABLE_VIRTUAL_TERMINAL_PROCESSING;
    ok(mode == (ENABLE_PROCESSED_OUTPUT | ENABLE_WRAP_AT_EOL_OUTPUT), "mode = %lx\n", mode);

    ret = SetConsoleMode(output, mode & ~ENABLE_WRAP_AT_EOL_OUTPUT);
    ok(ret, "SetConsoleMode failed: %lu\n", GetLastError());

    ret = GetConsoleMode(output, &mode);
    ok(ret, "GetConsoleMode failed: %lu\n", GetLastError());
    ok(mode == ENABLE_PROCESSED_OUTPUT, "mode = %lx\n", mode);

    ret = SetConsoleMode(output, mode | ENABLE_WRAP_AT_EOL_OUTPUT);
    ok(ret, "SetConsoleMode failed: %lu\n", GetLastError());

    ret = GetConsoleScreenBufferInfo(output, &sb_info);
    ok(ret, "GetConsoleScreenBufferInfo failed: %lu\n", GetLastError());
    ok(sb_info.dwSize.X == 40, "dwSize.X = %u\n", sb_info.dwSize.X);
    ok(sb_info.dwSize.Y == 30, "dwSize.Y = %u\n", sb_info.dwSize.Y);
    ok(sb_info.dwCursorPosition.X == 0, "dwCursorPosition.X = %u\n", sb_info.dwCursorPosition.X);
    ok(sb_info.dwCursorPosition.Y == 0, "dwCursorPosition.Y = %u\n", sb_info.dwCursorPosition.Y);
    ok(sb_info.wAttributes == 7, "wAttributes = %x\n", sb_info.wAttributes);
    ok(sb_info.srWindow.Left == 0, "srWindow.Left = %u\n", sb_info.srWindow.Left);
    ok(sb_info.srWindow.Top == 0, "srWindow.Top = %u\n", sb_info.srWindow.Top);
    ok(sb_info.srWindow.Right == 39, "srWindow.Right = %u\n", sb_info.srWindow.Right);
    ok(sb_info.srWindow.Bottom == 29, "srWindow.Bottom = %u\n", sb_info.srWindow.Bottom);
    ok(sb_info.dwMaximumWindowSize.X == 40, "dwMaximumWindowSize.X = %u\n", sb_info.dwMaximumWindowSize.X);
    ok(sb_info.dwMaximumWindowSize.Y == 30, "dwMaximumWindowSize.Y = %u\n", sb_info.dwMaximumWindowSize.Y);

    ret = GetConsoleCursorInfo(output, &cursor_info);
    ok(ret, "GetConsoleCursorInfo failed: %lu\n", GetLastError());
    ok(cursor_info.dwSize == 25, "dwSize = %lu\n", cursor_info.dwSize);
    ok(cursor_info.bVisible == TRUE, "bVisible = %x\n", cursor_info.bVisible);

    hwnd = GetConsoleWindow();
    ok(IsWindow(hwnd), "no console window\n");

    test_GetConsoleTitleA();
    test_GetConsoleTitleW();
    test_WriteConsoleInputW(input);
}

static DWORD WINAPI read_pipe_proc( void *handle )
{
    char buf[64];
    DWORD size;
    while (ReadFile(handle, buf, sizeof(buf), &size, NULL));
    ok(GetLastError() == ERROR_BROKEN_PIPE, "ReadFile returned %lu\n", GetLastError());
    CloseHandle(handle);
    return 0;
}

static void test_pseudo_console(void)
{
    STARTUPINFOEXA startup = {{ sizeof(startup) }};
    HANDLE console_pipe, console_pipe2, thread;
    char **argv, cmdline[MAX_PATH];
    PROCESS_INFORMATION info;
    HPCON pseudo_console;
    SIZE_T attr_size;
    COORD size;
    BOOL ret;
    HRESULT hres;

    if (!pCreatePseudoConsole)
    {
        win_skip("CreatePseudoConsole not available\n");
        return;
    }

    console_pipe = CreateNamedPipeW(L"\\\\.\\pipe\\pseudoconsoleconn", PIPE_ACCESS_DUPLEX,
                                    PIPE_WAIT | PIPE_TYPE_BYTE, 1, 4096, 4096, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(console_pipe != INVALID_HANDLE_VALUE, "CreateNamedPipeW failed: %lu\n", GetLastError());

    console_pipe2 = CreateFileW(L"\\\\.\\pipe\\pseudoconsoleconn", GENERIC_READ | GENERIC_WRITE, 0, NULL,
                                OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    ok(console_pipe2 != INVALID_HANDLE_VALUE, "CreateFile failed: %lu\n", GetLastError());

    thread = CreateThread( NULL, 0, read_pipe_proc, console_pipe, 0, NULL );
    CloseHandle(thread);

    size.X = 0;
    size.Y = 30;
    hres = pCreatePseudoConsole(size, console_pipe2, console_pipe2, 0, &pseudo_console);
    ok(hres == E_INVALIDARG, "CreatePseudoConsole failed: %08lx\n", hres);

    size.X = 40;
    size.Y = 0;
    hres = pCreatePseudoConsole(size, console_pipe2, console_pipe2, 0, &pseudo_console);
    ok(hres == E_INVALIDARG, "CreatePseudoConsole failed: %08lx\n", hres);

    size.X = 40;
    size.Y = 30;
    hres = pCreatePseudoConsole(size, console_pipe2, console_pipe2, 0, &pseudo_console);
    ok(hres == S_OK, "CreatePseudoConsole failed: %08lx\n", hres);
    CloseHandle(console_pipe2);

#ifndef __REACTOS__
    InitializeProcThreadAttributeList(NULL, 1, 0, &attr_size);
    startup.lpAttributeList = HeapAlloc(GetProcessHeap(), 0, attr_size);
    InitializeProcThreadAttributeList(startup.lpAttributeList, 1, 0, &attr_size);
    UpdateProcThreadAttribute(startup.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, pseudo_console,
                              sizeof(pseudo_console), NULL, NULL);
#endif

    winetest_get_mainargs(&argv);
    sprintf(cmdline, "\"%s\" %s --pseudo-console", argv[0], argv[1]);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL, &startup.StartupInfo, &info);
    ok(ret, "CreateProcessW failed: %lu\n", GetLastError());

    CloseHandle(info.hThread);
    HeapFree(GetProcessHeap(), 0, startup.lpAttributeList);
    wait_child_process(info.hProcess);
    CloseHandle(info.hProcess);

    pClosePseudoConsole(pseudo_console);
}

/* copy an executable, but changing its subsystem */
static void copy_change_subsystem(const char* in, const char* out, DWORD subsyst)
{
    BOOL ret;
    HANDLE hFile, hMap;
    void* mapping;
    IMAGE_NT_HEADERS *nthdr;

    ret = CopyFileA(in, out, FALSE);
    ok(ret, "Failed to copy executable %s in %s (%lu)\n", in, out, GetLastError());

    hFile = CreateFileA(out, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                         OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hFile != INVALID_HANDLE_VALUE, "Couldn't open file %s (%lu)\n", out, GetLastError());
    hMap = CreateFileMappingW(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
    ok(hMap != NULL, "Couldn't create map (%lu)\n", GetLastError());
    mapping = MapViewOfFile(hMap, FILE_MAP_ALL_ACCESS, 0, 0, 0);
    ok(mapping != NULL, "Couldn't map (%lu)\n", GetLastError());
    nthdr = RtlImageNtHeader(mapping);
    ok(nthdr != NULL, "Cannot get NT headers out of %s\n", out);
    if (nthdr) nthdr->OptionalHeader.Subsystem = subsyst;
    ret = UnmapViewOfFile(mapping);
    ok(ret, "Couldn't unmap (%lu)\n", GetLastError());
    CloseHandle(hMap);
    CloseHandle(hFile);
}

enum inheritance_model {NULL_STD, CONSOLE_STD, STARTUPINFO_STD};

static DWORD check_child_console_bits(const char* exec, DWORD flags, enum inheritance_model inherit)
{
    SECURITY_ATTRIBUTES sa = {0, NULL, TRUE};
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION info;
    char buf[MAX_PATH];
    HANDLE handle;
    DWORD exit_code;
    BOOL res;
    DWORD ret;
    BOOL inherit_handles = FALSE;

    sprintf(buf, "\"%s\" console check_console", exec);
    switch (inherit)
    {
    case NULL_STD:
        SetStdHandle(STD_INPUT_HANDLE, NULL);
        SetStdHandle(STD_OUTPUT_HANDLE, NULL);
        SetStdHandle(STD_ERROR_HANDLE, NULL);
        break;
    case CONSOLE_STD:
        handle = CreateFileA("CONIN$", GENERIC_READ, 0, &sa, OPEN_EXISTING, 0, 0);
        ok(handle != INVALID_HANDLE_VALUE, "Couldn't create input to console\n");
        SetStdHandle(STD_INPUT_HANDLE, handle);
        handle = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, &sa, OPEN_EXISTING, 0, 0);
        ok(handle != INVALID_HANDLE_VALUE, "Couldn't create input to console\n");
        SetStdHandle(STD_OUTPUT_HANDLE, handle);
        SetStdHandle(STD_ERROR_HANDLE, handle);
        break;
    case STARTUPINFO_STD:
        si.dwFlags |= STARTF_USESTDHANDLES;
        si.hStdInput = CreateFileA("CONIN$", GENERIC_READ, 0, &sa, OPEN_EXISTING, 0, 0);
        ok(si.hStdInput != INVALID_HANDLE_VALUE, "Couldn't create input to console\n");
        si.hStdOutput = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, &sa, OPEN_EXISTING, 0, 0);
        ok(si.hStdInput != INVALID_HANDLE_VALUE, "Couldn't create output to console\n");
        si.hStdError = INVALID_HANDLE_VALUE;
        inherit_handles = TRUE;
        break;
    }
    res = CreateProcessA(NULL, buf, NULL, NULL, inherit_handles, flags, NULL, NULL, &si, &info);
    ok(res, "CreateProcess failed: %lu %s\n", GetLastError(), buf);
    CloseHandle(info.hThread);
    ret = WaitForSingleObject(info.hProcess, 30000);
    ok(ret == WAIT_OBJECT_0, "Could not wait for the child process: %ld le=%lu\n",
        ret, GetLastError());
    res = GetExitCodeProcess(info.hProcess, &exit_code);
    ok(res && exit_code <= 255, "Couldn't get exit_code\n");
    CloseHandle(info.hProcess);
    switch (inherit)
    {
    case NULL_STD:
        break;
    case CONSOLE_STD:
        CloseHandle(GetStdHandle(STD_INPUT_HANDLE));
        CloseHandle(GetStdHandle(STD_OUTPUT_HANDLE));
        break;
    case STARTUPINFO_STD:
        CloseHandle(si.hStdInput);
        CloseHandle(si.hStdOutput);
        break;
    }
    return exit_code;
}

#define CP_WITH_CONSOLE  0x01   /* attached to a console */
#define CP_WITH_HANDLE   0x02   /* child has a console handle */
#define CP_WITH_WINDOW   0x04   /* child has a console window */
#define CP_ALONE         0x08   /* whether child is the single process attached to console */
#define CP_GROUP_LEADER  0x10   /* whether the child is the process group leader */
#define CP_INPUT_VALID   0x20   /* whether StdHandle(INPUT) is a valid console handle */
#define CP_OUTPUT_VALID  0x40   /* whether StdHandle(OUTPUT) is a valid console handle */
#define CP_ENABLED_CTRLC 0x80   /* whether the ctrl-c handling isn't blocked */

#define CP_OWN_CONSOLE (CP_WITH_CONSOLE | CP_WITH_HANDLE | CP_INPUT_VALID | CP_OUTPUT_VALID | CP_ALONE)
#define CP_INH_CONSOLE (CP_WITH_CONSOLE | CP_WITH_HANDLE | CP_INPUT_VALID | CP_OUTPUT_VALID)

static void test_CreateProcessCUI(void)
{
    HANDLE hstd[3];
    static char guiexec[MAX_PATH];
    static char cuiexec[MAX_PATH];
    char **argv;
    BOOL res;
    int i;
    BOOL saved_console_flags;

    static struct
    {
        BOOL use_cui;
        DWORD cp_flags;
        enum inheritance_model inherit;
        DWORD expected;
        DWORD is_broken;
    }
    no_console_tests[] =
    {
/* 0*/  {FALSE, 0,                                     NULL_STD,        0},
        {FALSE, DETACHED_PROCESS,                      NULL_STD,        0},
        {FALSE, CREATE_NEW_CONSOLE,                    NULL_STD,        0},
        {FALSE, CREATE_NO_WINDOW,                      NULL_STD,        0},
        {FALSE, DETACHED_PROCESS | CREATE_NO_WINDOW,   NULL_STD,        0},
/* 5*/  {FALSE, CREATE_NEW_CONSOLE | CREATE_NO_WINDOW, NULL_STD,        0},

        {TRUE,  0,                                     NULL_STD,        CP_OWN_CONSOLE | CP_WITH_WINDOW},
        {TRUE,  DETACHED_PROCESS,                      NULL_STD,        0},
        {TRUE,  CREATE_NEW_CONSOLE,                    NULL_STD,        CP_OWN_CONSOLE | CP_WITH_WINDOW},
        {TRUE,  CREATE_NO_WINDOW,                      NULL_STD,        CP_OWN_CONSOLE},
/*10*/  {TRUE,  DETACHED_PROCESS | CREATE_NO_WINDOW,   NULL_STD,        0},
        {TRUE,  CREATE_NEW_CONSOLE | CREATE_NO_WINDOW, NULL_STD,        CP_OWN_CONSOLE | CP_WITH_WINDOW},
    },
    with_console_tests[] =
    {
/* 0*/  {FALSE, 0,                                     NULL_STD,        0},
        {FALSE, DETACHED_PROCESS,                      NULL_STD,        0},
        {FALSE, CREATE_NEW_CONSOLE,                    NULL_STD,        0},
        {FALSE, CREATE_NO_WINDOW,                      NULL_STD,        0},
        {FALSE, DETACHED_PROCESS | CREATE_NO_WINDOW,   NULL_STD,        0},
/* 5*/  {FALSE, CREATE_NEW_CONSOLE | CREATE_NO_WINDOW, NULL_STD,        0},

        {FALSE, 0,                                     CONSOLE_STD,     0},
        {FALSE, DETACHED_PROCESS,                      CONSOLE_STD,     0},
        {FALSE, CREATE_NEW_CONSOLE,                    CONSOLE_STD,     0},
        {FALSE, CREATE_NO_WINDOW,                      CONSOLE_STD,     0},
/*10*/  {FALSE, DETACHED_PROCESS | CREATE_NO_WINDOW,   CONSOLE_STD,     0},
        {FALSE, CREATE_NEW_CONSOLE | CREATE_NO_WINDOW, CONSOLE_STD,     0},

        {FALSE, 0,                                     STARTUPINFO_STD, 0},
        {FALSE, DETACHED_PROCESS,                      STARTUPINFO_STD, 0},
        {FALSE, CREATE_NEW_CONSOLE,                    STARTUPINFO_STD, 0},
/*15*/  {FALSE, CREATE_NO_WINDOW,                      STARTUPINFO_STD, 0},
        {FALSE, DETACHED_PROCESS | CREATE_NO_WINDOW,   STARTUPINFO_STD, 0},
        {FALSE, CREATE_NEW_CONSOLE | CREATE_NO_WINDOW, STARTUPINFO_STD, 0},

        {TRUE,  0,                                     NULL_STD,        CP_WITH_CONSOLE | CP_WITH_HANDLE | CP_WITH_WINDOW},
        {TRUE,  DETACHED_PROCESS,                      NULL_STD,        0},
/*20*/  {TRUE,  CREATE_NEW_CONSOLE,                    NULL_STD,        CP_OWN_CONSOLE | CP_WITH_WINDOW},
        {TRUE,  CREATE_NO_WINDOW,                      NULL_STD,        CP_OWN_CONSOLE},
        {TRUE,  DETACHED_PROCESS | CREATE_NO_WINDOW,   NULL_STD,        0},
        {TRUE,  CREATE_NEW_CONSOLE | CREATE_NO_WINDOW, NULL_STD,        CP_OWN_CONSOLE | CP_WITH_WINDOW},

        {TRUE,  0,                                     CONSOLE_STD,     CP_INH_CONSOLE | CP_WITH_WINDOW},
/*25*/  {TRUE,  DETACHED_PROCESS,                      CONSOLE_STD,     0},
        {TRUE,  CREATE_NEW_CONSOLE,                    CONSOLE_STD,     CP_OWN_CONSOLE | CP_WITH_WINDOW},
        {TRUE,  CREATE_NO_WINDOW,                      CONSOLE_STD,     CP_OWN_CONSOLE},
        {TRUE,  DETACHED_PROCESS | CREATE_NO_WINDOW,   CONSOLE_STD,     0},
        {TRUE,  CREATE_NEW_CONSOLE | CREATE_NO_WINDOW, CONSOLE_STD,     CP_OWN_CONSOLE | CP_WITH_WINDOW},

/*30*/  {TRUE,  0,                                     STARTUPINFO_STD, CP_INH_CONSOLE | CP_WITH_WINDOW},
        {TRUE,  DETACHED_PROCESS,                      STARTUPINFO_STD, CP_INPUT_VALID | CP_OUTPUT_VALID, .is_broken = 0x100},
        {TRUE,  CREATE_NEW_CONSOLE,                    STARTUPINFO_STD, CP_OWN_CONSOLE | CP_WITH_WINDOW,  .is_broken = CP_WITH_CONSOLE | CP_WITH_HANDLE | CP_WITH_WINDOW | CP_ALONE},
        {TRUE,  CREATE_NO_WINDOW,                      STARTUPINFO_STD, CP_OWN_CONSOLE,                   .is_broken = CP_WITH_CONSOLE | CP_WITH_HANDLE | CP_ALONE},
        {TRUE,  DETACHED_PROCESS | CREATE_NO_WINDOW,   STARTUPINFO_STD, CP_INPUT_VALID | CP_OUTPUT_VALID, .is_broken = 0x100},
/*35*/  {TRUE,  CREATE_NEW_CONSOLE | CREATE_NO_WINDOW, STARTUPINFO_STD, CP_OWN_CONSOLE | CP_WITH_WINDOW,  .is_broken = CP_WITH_CONSOLE | CP_WITH_HANDLE | CP_WITH_WINDOW | CP_ALONE},
    };
    static struct group_flags_tests
    {
        /* input */
        BOOL use_cui;
        DWORD cp_flags;
        enum inheritance_model inherit;
        BOOL noctrl_flag;
        /* output */
        DWORD expected;
    }
    group_flags_tests[] =
    {
/*  0 */ {TRUE,  0,                        CONSOLE_STD,     TRUE,   CP_INH_CONSOLE | CP_WITH_WINDOW},
         {TRUE,  CREATE_NEW_PROCESS_GROUP, CONSOLE_STD,     TRUE,   CP_INH_CONSOLE | CP_WITH_WINDOW | CP_GROUP_LEADER},
         {TRUE,  0,                        CONSOLE_STD,     FALSE,  CP_INH_CONSOLE | CP_WITH_WINDOW | CP_ENABLED_CTRLC},
         {TRUE,  CREATE_NEW_PROCESS_GROUP, CONSOLE_STD,     FALSE,  CP_INH_CONSOLE | CP_WITH_WINDOW | CP_GROUP_LEADER},
         {TRUE,  0,                        STARTUPINFO_STD, TRUE,   CP_INH_CONSOLE | CP_WITH_WINDOW},
/*  5 */ {TRUE,  CREATE_NEW_PROCESS_GROUP, STARTUPINFO_STD, TRUE,   CP_INH_CONSOLE | CP_WITH_WINDOW | CP_GROUP_LEADER},
         {TRUE,  0,                        STARTUPINFO_STD, FALSE,  CP_INH_CONSOLE | CP_WITH_WINDOW | CP_ENABLED_CTRLC},
         {TRUE,  CREATE_NEW_PROCESS_GROUP, STARTUPINFO_STD, FALSE,  CP_INH_CONSOLE | CP_WITH_WINDOW | CP_GROUP_LEADER},
         {FALSE, 0,                        CONSOLE_STD,     TRUE,   0},
         {FALSE, CREATE_NEW_PROCESS_GROUP, CONSOLE_STD,     TRUE,   CP_GROUP_LEADER},
/* 10 */ {FALSE, 0,                        CONSOLE_STD,     FALSE,  CP_ENABLED_CTRLC},
         {FALSE, CREATE_NEW_PROCESS_GROUP, CONSOLE_STD,     FALSE,  CP_GROUP_LEADER},
         {FALSE, 0,                        STARTUPINFO_STD, TRUE,   0},
         {FALSE, CREATE_NEW_PROCESS_GROUP, STARTUPINFO_STD, TRUE,   CP_GROUP_LEADER},
         {FALSE, 0,                        STARTUPINFO_STD, FALSE,  CP_ENABLED_CTRLC},
/* 15 */ {FALSE, CREATE_NEW_PROCESS_GROUP, STARTUPINFO_STD, FALSE,  CP_GROUP_LEADER},
         {TRUE,  CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE, CONSOLE_STD, TRUE,   CP_INH_CONSOLE | CP_WITH_WINDOW | CP_GROUP_LEADER | CP_ALONE},
         {FALSE, CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE, CONSOLE_STD, TRUE,   CP_GROUP_LEADER},
         {TRUE,  CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE, CONSOLE_STD, FALSE,  CP_INH_CONSOLE | CP_WITH_WINDOW | CP_GROUP_LEADER | CP_ALONE | CP_ENABLED_CTRLC},
         {FALSE, CREATE_NEW_PROCESS_GROUP | CREATE_NEW_CONSOLE, CONSOLE_STD, FALSE,  CP_GROUP_LEADER | CP_ENABLED_CTRLC},
/* 20 */ {TRUE,  CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS,   CONSOLE_STD, TRUE,   CP_GROUP_LEADER},
         {FALSE, CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS,   CONSOLE_STD, TRUE,   CP_GROUP_LEADER},
         {TRUE,  CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS,   CONSOLE_STD, FALSE,  CP_GROUP_LEADER},
         {FALSE, CREATE_NEW_PROCESS_GROUP | DETACHED_PROCESS,   CONSOLE_STD, FALSE,  CP_GROUP_LEADER},
    };

    hstd[0] = GetStdHandle(STD_INPUT_HANDLE);
    hstd[1] = GetStdHandle(STD_OUTPUT_HANDLE);
    hstd[2] = GetStdHandle(STD_ERROR_HANDLE);

    winetest_get_mainargs(&argv);
    GetTempPathA(ARRAY_SIZE(guiexec), guiexec);
    strcat(guiexec, "console_gui.exe");
    copy_change_subsystem(argv[0], guiexec, IMAGE_SUBSYSTEM_WINDOWS_GUI);
    GetTempPathA(ARRAY_SIZE(cuiexec), cuiexec);
    strcat(cuiexec, "console_cui.exe");
    copy_change_subsystem(argv[0], cuiexec, IMAGE_SUBSYSTEM_WINDOWS_CUI);

    FreeConsole();

    for (i = 0; i < ARRAY_SIZE(no_console_tests); i++)
    {
        res = check_child_console_bits(no_console_tests[i].use_cui ? cuiexec : guiexec,
                                       no_console_tests[i].cp_flags,
                                       no_console_tests[i].inherit);
        ok(res == no_console_tests[i].expected, "[%d] Unexpected result %x (%lx)\n",
           i, res, no_console_tests[i].expected);
    }

    AllocConsole();

    for (i = 0; i < ARRAY_SIZE(with_console_tests); i++)
    {
        res = check_child_console_bits(with_console_tests[i].use_cui ? cuiexec : guiexec,
                                       with_console_tests[i].cp_flags,
                                       with_console_tests[i].inherit);
        ok(res == with_console_tests[i].expected ||
           broken(with_console_tests[i].is_broken && res == (with_console_tests[i].is_broken & 0xff)),
           "[%d] Unexpected result %x (%lx)\n",
           i, res, with_console_tests[i].expected);
    }

    saved_console_flags = RtlGetCurrentPeb()->ProcessParameters->ConsoleFlags;

    for (i = 0; i < ARRAY_SIZE(group_flags_tests); i++)
    {
        res = SetConsoleCtrlHandler(NULL, group_flags_tests[i].noctrl_flag);
        ok(res, "Couldn't set ctrl handler\n");
        res = check_child_console_bits(group_flags_tests[i].use_cui ? cuiexec : guiexec,
                                       group_flags_tests[i].cp_flags,
                                       group_flags_tests[i].inherit);
        ok(res == group_flags_tests[i].expected ||
           /* Win7 doesn't report group id */
           broken(res == (group_flags_tests[i].expected & ~CP_GROUP_LEADER)),
           "[%d] Unexpected result %x (%lx)\n",
           i, res, group_flags_tests[i].expected);
    }

    RtlGetCurrentPeb()->ProcessParameters->ConsoleFlags = saved_console_flags;

    DeleteFileA(guiexec);
    DeleteFileA(cuiexec);

    SetStdHandle(STD_INPUT_HANDLE, hstd[0]);
    SetStdHandle(STD_OUTPUT_HANDLE, hstd[1]);
    SetStdHandle(STD_ERROR_HANDLE, hstd[2]);
}

#define NO_EVENT 0xfe

static HANDLE mch_child_kill_event;
static DWORD  mch_child_event = NO_EVENT;
static BOOL WINAPI mch_child(DWORD event)
{
    mch_child_event = event;
    SetEvent(mch_child_kill_event);
    return TRUE;
}

static void test_CtrlHandlerSubsystem(void)
{
    static char guiexec[MAX_PATH];
    static char cuiexec[MAX_PATH];

    static struct
    {
        /* input */
        BOOL use_cui;
        DWORD cp_flags;
        enum pgid {PGID_PARENT, PGID_ZERO, PGID_CHILD} pgid_kind;
        /* output */
        unsigned child_event;
    }
    tests[] =
    {
/*  0 */ {FALSE, 0,                             PGID_PARENT,      NO_EVENT},
         {FALSE, 0,                             PGID_ZERO,        NO_EVENT},
         {FALSE, CREATE_NEW_PROCESS_GROUP,      PGID_CHILD,       NO_EVENT},
         {FALSE, CREATE_NEW_PROCESS_GROUP,      PGID_PARENT,      NO_EVENT},
         {FALSE, CREATE_NEW_PROCESS_GROUP,      PGID_ZERO,        NO_EVENT},
/*  5 */ {TRUE,  0,                             PGID_PARENT,      CTRL_C_EVENT},
         {TRUE,  0,                             PGID_ZERO,        CTRL_C_EVENT},
         {TRUE,  CREATE_NEW_PROCESS_GROUP,      PGID_CHILD,       NO_EVENT},
         {TRUE,  CREATE_NEW_PROCESS_GROUP,      PGID_PARENT,      NO_EVENT},
         {TRUE,  CREATE_NEW_PROCESS_GROUP,      PGID_ZERO,        NO_EVENT},
/* 10 */ {TRUE,  CREATE_NEW_CONSOLE,            PGID_PARENT,      NO_EVENT},
         {TRUE,  CREATE_NEW_CONSOLE,            PGID_ZERO,        NO_EVENT},
         {TRUE,  DETACHED_PROCESS,              PGID_PARENT,      NO_EVENT},
         {TRUE,  DETACHED_PROCESS,              PGID_ZERO,        NO_EVENT},
    };
    SECURITY_ATTRIBUTES inheritable_attr = { sizeof(inheritable_attr), NULL, TRUE };
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION info;
    char buf[MAX_PATH];
    DWORD exit_code;
    HANDLE event_child;
    char **argv;
    DWORD saved_console_flags;
    DWORD pgid;
    BOOL ret;
    DWORD res;
    int i;

    winetest_get_mainargs(&argv);
    GetTempPathA(ARRAY_SIZE(guiexec), guiexec);
    strcat(guiexec, "console_gui.exe");
    copy_change_subsystem(argv[0], guiexec, IMAGE_SUBSYSTEM_WINDOWS_GUI);
    GetTempPathA(ARRAY_SIZE(cuiexec), cuiexec);
    strcat(cuiexec, "console_cui.exe");
    copy_change_subsystem(argv[0], cuiexec, IMAGE_SUBSYSTEM_WINDOWS_CUI);

    event_child = CreateEventA(&inheritable_attr, FALSE, FALSE, NULL);
    ok(event_child != NULL, "Couldn't create event\n");

    saved_console_flags = RtlGetCurrentPeb()->ProcessParameters->ConsoleFlags;

    /* protect self against ctrl-c, but don't mask it on child */
    ret = SetConsoleCtrlHandler(NULL, FALSE);
    ret = SetConsoleCtrlHandler(mydummych, TRUE);
    ok(ret, "Couldn't set ctrl-c handler flag\n");

    for (i = 0; i < ARRAY_SIZE(tests); i++)
    {
        winetest_push_context("test #%u", i);

        res = snprintf(buf, ARRAY_SIZE(buf), "\"%s\" console ctrl_handler %p", tests[i].use_cui ? cuiexec : guiexec, event_child);
        ok((LONG)res >= 0 && res < ARRAY_SIZE(buf), "Truncated string %s (%lu)\n", buf, res);

        ret = CreateProcessA(NULL, buf, NULL, NULL, TRUE, tests[i].cp_flags,
                             NULL, NULL, &si, &info);
        ok(ret, "CreateProcess failed: %lu %s\n", GetLastError(), tests[i].use_cui ? cuiexec : guiexec);

        res = WaitForSingleObject(event_child, 5000);
        ok(res == WAIT_OBJECT_0, "Child didn't init %lu %p\n", res, event_child);

        switch (tests[i].pgid_kind)
        {
        case PGID_PARENT:
            pgid = RtlGetCurrentPeb()->ProcessParameters->ProcessGroupId;
            break;
        case PGID_CHILD:
            ok((tests[i].cp_flags & CREATE_NEW_PROCESS_GROUP) != 0,
               "PGID should only be used with new process groupw\n");
            pgid = info.dwProcessId;
            break;
        case PGID_ZERO:
            pgid = 0;
            break;
        default:
            ok(0, "Unexpected pgid kind %u\n", tests[i].pgid_kind);
            pgid = 0;
        }

        ret = GenerateConsoleCtrlEvent(CTRL_C_EVENT, pgid);
        ok(ret || broken(GetLastError() == ERROR_INVALID_PARAMETER) /* Win7 */,
           "GenerateConsoleCtrlEvent failed: %lu\n", GetLastError());

        res = WaitForSingleObject(info.hProcess, 2000);
        ok(res == WAIT_OBJECT_0, "Expecting child to be terminated\n");

        if (ret)
        {
            ret = GetExitCodeProcess(info.hProcess, &exit_code);
            ok(ret, "Couldn't get exit code\n");

            ok(tests[i].child_event == exit_code, "Unexpected exit code %#lx, instead of %#x\n",
               exit_code, tests[i].child_event);
        }

        CloseHandle(info.hProcess);
        CloseHandle(info.hThread);
        winetest_pop_context();
    }

    /* test default handlers return code */
    res = snprintf(buf, ARRAY_SIZE(buf), "\"%s\" console no_ctrl_handler %p", cuiexec, event_child);
    ok((LONG)res >= 0 && res < ARRAY_SIZE(buf), "Truncated string %s (%lu)\n", buf, res);

    ret = CreateProcessA(NULL, buf, NULL, NULL, TRUE, 0, NULL, NULL, &si, &info);
    ok(ret, "CreateProcess failed: %lu %s\n", GetLastError(), cuiexec);

    res = WaitForSingleObject(event_child, 5000);
    ok(res == WAIT_OBJECT_0, "Child didn't init %lu\n", res);

    pgid = RtlGetCurrentPeb()->ProcessParameters->ProcessGroupId;
    ret = GenerateConsoleCtrlEvent(CTRL_C_EVENT, pgid);
    if (!ret && broken(GetLastError() == ERROR_INVALID_PARAMETER) /* Win7 */)
    {
        win_skip("Skip test on Win7\n");
        TerminateProcess(info.hProcess, 0);
    }
    else
    {
        ok(ret, "GenerateConsoleCtrlEvent failed: %lu\n", GetLastError());

        res = WaitForSingleObject(info.hProcess, 2000);
        ok(res == WAIT_OBJECT_0, "Expecting child to be terminated\n");

        if (ret)
        {
            ret = GetExitCodeProcess(info.hProcess, &exit_code);
            ok(ret, "Couldn't get exit code\n");

            ok(exit_code == STATUS_CONTROL_C_EXIT, "Unexpected exit code %#lx, instead of %#lx\n",
               exit_code, STATUS_CONTROL_C_EXIT);
        }
    }

    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);

    CloseHandle(event_child);

    RtlGetCurrentPeb()->ProcessParameters->ConsoleFlags = saved_console_flags;
    ret = SetConsoleCtrlHandler(mydummych, FALSE);
    ok(ret, "Couldn't remove ctrl-c handler flag\n");

    DeleteFileA(guiexec);
    DeleteFileA(cuiexec);
}

START_TEST(console)
{
    HANDLE hConIn, hConOut, revert_output = NULL, unbound_output;
    BOOL ret, test_current;
    CONSOLE_SCREEN_BUFFER_INFO	sbi;
    BOOL using_pseudo_console;
    DWORD size;
    char **argv;
    int argc;

    init_function_pointers();

    argc = winetest_get_mainargs(&argv);

    if (argc > 3 && !strcmp(argv[2], "attach_console"))
    {
        DWORD parent_pid;
        sscanf(argv[3], "%lx", &parent_pid);
        test_AttachConsole_child(parent_pid);
        return;
    }

    if (argc == 3 && !strcmp(argv[2], "alloc_console"))
    {
        test_AllocConsole_child();
        return;
    }

    if (argc == 4 && !strcmp(argv[2], "ctrl_handler"))
    {
        HANDLE event;

        SetConsoleCtrlHandler(mch_child, TRUE);
        mch_child_kill_event = CreateEventA(NULL, FALSE, FALSE, NULL);
        ok(mch_child_kill_event != NULL, "Couldn't create event\n");
        sscanf(argv[3], "%p", &event);
        ret = SetEvent(event);
        ok(ret, "SetEvent failed\n");

        WaitForSingleObject(mch_child_kill_event, 1000); /* enough for all events to be distributed? */
        ExitProcess(mch_child_event);
    }

    if (argc == 4 && !strcmp(argv[2], "no_ctrl_handler"))
    {
        HANDLE event;

        SetConsoleCtrlHandler(NULL, FALSE);
        sscanf(argv[3], "%p", &event);
        ret = SetEvent(event);
        ok(ret, "SetEvent failed\n");

        event = CreateEventA(NULL, FALSE, FALSE, NULL);
        ok(event != NULL, "Couldn't create event\n");

        /* wait for parent to kill us */
        WaitForSingleObject(event, INFINITE);
        ok(0, "Shouldn't happen\n");
        ExitProcess(0xff);
    }

    if (argc == 3 && !strcmp(argv[2], "check_console"))
    {
        DWORD exit_code = 0, pcslist;
        if (GetConsoleCP() != 0) exit_code |= CP_WITH_CONSOLE;
        if (RtlGetCurrentPeb()->ProcessParameters->ConsoleHandle) exit_code |= CP_WITH_HANDLE;
        if (IsWindow(GetConsoleWindow())) exit_code |= CP_WITH_WINDOW;
        if (pGetConsoleProcessList && GetConsoleProcessList(&pcslist, 1) == 1)
            exit_code |= CP_ALONE;
        if (RtlGetCurrentPeb()->ProcessParameters->Size >=
            offsetof(RTL_USER_PROCESS_PARAMETERS, ProcessGroupId) +
            sizeof(RtlGetCurrentPeb()->ProcessParameters->ProcessGroupId) &&
            RtlGetCurrentPeb()->ProcessParameters->ProcessGroupId == GetCurrentProcessId())
            exit_code |= CP_GROUP_LEADER;
        if (GetFileType(GetStdHandle(STD_INPUT_HANDLE)) == FILE_TYPE_CHAR)
            exit_code |= CP_INPUT_VALID;
        if (GetFileType(GetStdHandle(STD_OUTPUT_HANDLE)) == FILE_TYPE_CHAR)
            exit_code |= CP_OUTPUT_VALID;
        if (!(RtlGetCurrentPeb()->ProcessParameters->ConsoleFlags & 1))
            exit_code |= CP_ENABLED_CTRLC;
        ExitProcess(exit_code);
    }

#ifndef __REACTOS__ // Can't link
    if (argc >= 3 && !strcmp(argv[2], "title_test"))
    {
        if (argc == 3)
        {
            test_GetConsoleOriginalTitleA();
            test_GetConsoleOriginalTitleW();
        }
        else
            test_GetConsoleOriginalTitleW_empty();
        return;
    }
#endif

    test_current = argc >= 3 && !strcmp(argv[2], "--current");
    using_pseudo_console = argc >= 3 && !strcmp(argv[2], "--pseudo-console");

    if (!test_current && !using_pseudo_console)
    {
        static const char font_name[] = "Lucida Console";
        HKEY console_key;
        char old_font[LF_FACESIZE];
        BOOL delete = FALSE;
        LONG err;

        /* ReadConsoleOutputW doesn't retrieve characters from the output buffer
         * correctly for characters that don't have a glyph in the console font. So,
         * we first set the console font to Lucida Console (which has a wider
         * selection of glyphs available than the default raster fonts). We want
         * to be able to restore the original font afterwards, so don't change
         * if we can't read the original font.
         */
        err = RegOpenKeyExA(HKEY_CURRENT_USER, "Console", 0,
                            KEY_QUERY_VALUE | KEY_SET_VALUE, &console_key);
        if (err == ERROR_SUCCESS)
        {
            size = sizeof(old_font);
            err = RegQueryValueExA(console_key, "FaceName", NULL, NULL,
                                   (LPBYTE) old_font, &size);
            if (err == ERROR_SUCCESS || err == ERROR_FILE_NOT_FOUND)
            {
                delete = (err == ERROR_FILE_NOT_FOUND);
                err = RegSetValueExA(console_key, "FaceName", 0, REG_SZ,
                                     (const BYTE *) font_name, sizeof(font_name));
                if (err != ERROR_SUCCESS)
                    trace("Unable to change default console font, error %ld\n", err);
            }
            else
            {
                trace("Unable to query default console font, error %ld\n", err);
                RegCloseKey(console_key);
                console_key = NULL;
            }
        }
        else
        {
            trace("Unable to open HKCU\\Console, error %ld\n", err);
            console_key = NULL;
        }

        /* Now detach and open a fresh console to play with */
        FreeConsole();
        ok(AllocConsole(), "Couldn't alloc console\n");

        /* Restore default console font if needed */
        if (console_key != NULL)
        {
            if (delete)
                err = RegDeleteValueA(console_key, "FaceName");
            else
                err = RegSetValueExA(console_key, "FaceName", 0, REG_SZ,
                                     (const BYTE *) old_font, strlen(old_font) + 1);
            ok(err == ERROR_SUCCESS, "Unable to restore default console font, error %ld\n", err);
        }
    }

    unbound_output = create_unbound_handle(TRUE, FALSE);
    if (!unbound_output)
    {
        win_skip("Skipping NT path tests, not supported on this Windows version\n");
        skip_nt = TRUE;
    }

    if (test_current)
    {
        HANDLE sb;
        revert_output = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        sb = CreateConsoleScreenBuffer(GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
                                       CONSOLE_TEXTMODE_BUFFER, NULL);
        ok(sb != INVALID_HANDLE_VALUE, "Could not allocate screen buffer: %lu\n", GetLastError());
        SetConsoleActiveScreenBuffer(sb);
    }

    hConIn = CreateFileA("CONIN$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    hConOut = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);

    /* now verify everything's ok */
    ok(hConIn != INVALID_HANDLE_VALUE, "Opening ConIn\n");
    ok(hConOut != INVALID_HANDLE_VALUE, "Opening ConOut\n");

    if (using_pseudo_console)
    {
        test_pseudo_console_child(hConIn, hConOut);
        return;
    }

    ret = GetConsoleScreenBufferInfo(hConOut, &sbi);
    ok(ret, "Getting sb info\n");
    if (!ret) return;

    /* Reduce the size of the buffer to the visible area plus 3 lines to speed
     * up the tests.
     */
    trace("Visible area: %dx%d - %dx%d Buffer size: %dx%d\n", sbi.srWindow.Left, sbi.srWindow.Top, sbi.srWindow.Right, sbi.srWindow.Bottom, sbi.dwSize.X, sbi.dwSize.Y);
    sbi.dwSize.Y = size = (sbi.srWindow.Bottom + 1) + 3;
    ret = SetConsoleScreenBufferSize(hConOut, sbi.dwSize);
    ok(ret, "Setting sb info\n");
    ret = GetConsoleScreenBufferInfo(hConOut, &sbi);
    ok(ret, "Getting sb info\n");
    ok(sbi.dwSize.Y == size, "Unexpected buffer size: %d instead of %ld\n", sbi.dwSize.Y, size);
    if (!ret) return;

    test_ReadConsole(hConIn);
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
    if (!test_current) testScreenBuffer(hConOut);
#ifndef __REACTOS__ // Can't link
    test_new_screen_buffer_properties(hConOut);
    test_new_screen_buffer_color_attributes(hConOut);
#endif
    /* Test waiting for a console handle */
    testWaitForConsoleInput(hConIn);
    test_wait(hConIn, hConOut);

    if (!test_current)
    {
        /* clear duplicated console font table */
        CloseHandle(hConIn);
        CloseHandle(hConOut);
        FreeConsole();
        ok(AllocConsole(), "Couldn't alloc console\n");
        hConIn = CreateFileA("CONIN$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        hConOut = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        ok(hConIn != INVALID_HANDLE_VALUE, "Opening ConIn\n");
        ok(hConOut != INVALID_HANDLE_VALUE, "Opening ConOut\n");
    }

    testCtrlHandler();
    /* still to be done: access rights & access on objects */

    if (!pGetConsoleInputExeNameA || !pSetConsoleInputExeNameA)
        win_skip("GetConsoleInputExeNameA and/or SetConsoleInputExeNameA is not available\n");
    else
        test_GetSetConsoleInputExeName();

    if (!test_current) test_GetConsoleProcessList();
    test_OpenConsoleW();
    test_CreateFileW();
    test_OpenCON();
    test_VerifyConsoleIoHandle(hConOut);
    test_GetSetStdHandle();
    test_DuplicateConsoleHandle();
    test_GetNumberOfConsoleInputEvents(hConIn);
    test_WriteConsoleInputA(hConIn);
    test_WriteConsoleInputW(hConIn);
    test_FlushConsoleInputBuffer(hConIn, hConOut);
    test_WriteConsoleOutputCharacterA(hConOut);
    test_WriteConsoleOutputCharacterW(hConOut);
    test_WriteConsoleOutputAttribute(hConOut);
    test_WriteConsoleOutput(hConOut);
    test_FillConsoleOutputCharacterA(hConOut);
    test_FillConsoleOutputCharacterW(hConOut);
    test_FillConsoleOutputAttribute(hConOut);
    test_ReadConsoleOutputCharacterA(hConOut);
    test_ReadConsoleOutputCharacterW(hConOut);
    test_ReadConsoleOutputAttribute(hConOut);
    test_ReadConsoleOutput(hConOut);
    if (!test_current)
    {
        test_GetCurrentConsoleFont(hConOut);
#ifndef __REACTOS__ // Can't link
        test_GetCurrentConsoleFontEx(hConOut);
        test_SetCurrentConsoleFontEx(hConOut);
#endif
        test_GetConsoleFontSize(hConOut);
        test_GetLargestConsoleWindowSize(hConOut);
        test_GetConsoleFontInfo(hConOut);
        test_SetConsoleFont(hConOut);
    }
    test_GetConsoleScreenBufferInfoEx(hConOut);
#ifndef __REACTOS__ // Can't link
    test_SetConsoleScreenBufferInfoEx(hConOut);
#endif
    test_file_info(hConIn, hConOut);
    test_GetConsoleOriginalTitle();
    test_GetConsoleTitleA();
    test_GetConsoleTitleW();
    test_console_as_root_directory();
    if (!test_current)
    {
        test_pseudo_console();
        test_AttachConsole(hConOut);
        test_AllocConsole();
        test_FreeConsole();
        test_condrv_server_as_root_directory();
        test_CreateProcessCUI();
        test_CtrlHandlerSubsystem();
    }
    else if (revert_output) SetConsoleActiveScreenBuffer(revert_output);

    CloseHandle(unbound_output);
}
