/*
 * Unit test suite for CreateProcess function.
 *
 * Copyright 2002 Eric Pouech
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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "wine/test.h"
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wincon.h"
#include "winnls.h"

static char     base[MAX_PATH];
static char     selfname[MAX_PATH];
static char     resfile[MAX_PATH];

static int      myARGC;
static char**   myARGV;

/* As some environment variables get very long on Unix, we only test for
 * the first 127 bytes.
 * Note that increasing this value past 256 may exceed the buffer size
 * limitations of the *Profile functions (at least on Wine).
 */
#define MAX_LISTED_ENV_VAR      128

/* ---------------- portable memory allocation thingie */

static char     memory[1024*32];
static char*    memory_index = memory;

static char*    grab_memory(size_t len)
{
    char*       ret = memory_index;
    /* align on dword */
    len = (len + 3) & ~3;
    memory_index += len;
    assert(memory_index <= memory + sizeof(memory));
    return ret;
}

static void     release_memory(void)
{
    memory_index = memory;
}

/* ---------------- simplistic tool to encode/decode strings (to hide \ " ' and such) */

static const char* encodeA(const char* str)
{
    char*       ptr;
    size_t      len,i;

    if (!str) return "";
    len = strlen(str) + 1;
    ptr = grab_memory(len * 2 + 1);
    for (i = 0; i < len; i++)
        sprintf(&ptr[i * 2], "%02x", (unsigned char)str[i]);
    ptr[2 * len] = '\0';
    return ptr;
}

static const char* encodeW(const WCHAR* str)
{
    char*       ptr;
    size_t      len,i;

    if (!str) return "";
    len = lstrlenW(str) + 1;
    ptr = grab_memory(len * 4 + 1);
    assert(ptr);
    for (i = 0; i < len; i++)
        sprintf(&ptr[i * 4], "%04x", (unsigned int)(unsigned short)str[i]);
    ptr[4 * len] = '\0';
    return ptr;
}

static unsigned decode_char(char c)
{
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    assert(c >= 'A' && c <= 'F');
    return c - 'A' + 10;
}

static char*    decodeA(const char* str)
{
    char*       ptr;
    size_t      len,i;

    len = strlen(str) / 2;
    if (!len--) return NULL;
    ptr = grab_memory(len + 1);
    for (i = 0; i < len; i++)
        ptr[i] = (decode_char(str[2 * i]) << 4) | decode_char(str[2 * i + 1]);
    ptr[len] = '\0';
    return ptr;
}

#if 0
/* This will be needed to decode Unicode strings saved by the child process
 * when we test Unicode functions.
 */
static WCHAR*   decodeW(const char* str)
{
    size_t      len;
    WCHAR*      ptr;
    int         i;

    len = strlen(str) / 4;
    if (!len--) return NULL;
    ptr = (WCHAR*)grab_memory(len * 2 + 1);
    for (i = 0; i < len; i++)
        ptr[i] = (decode_char(str[4 * i]) << 12) |
            (decode_char(str[4 * i + 1]) << 8) |
            (decode_char(str[4 * i + 2]) << 4) |
            (decode_char(str[4 * i + 3]) << 0);
    ptr[len] = '\0';
    return ptr;
}
#endif

/******************************************************************
 *		init
 *
 * generates basic information like:
 *      base:           absolute path to curr dir
 *      selfname:       the way to reinvoke ourselves
 */
static int     init(void)
{
    myARGC = winetest_get_mainargs( &myARGV );
    if (!GetCurrentDirectoryA(sizeof(base), base)) return 0;
    strcpy(selfname, myARGV[0]);
    return 1;
}

/******************************************************************
 *		get_file_name
 *
 * generates an absolute file_name for temporary file
 *
 */
static void     get_file_name(char* buf)
{
    char        path[MAX_PATH];

    buf[0] = '\0';
    GetTempPathA(sizeof(path), path);
    GetTempFileNameA(path, "wt", 0, buf);
}

/******************************************************************
 *		static void     childPrintf
 *
 */
static void     childPrintf(HANDLE h, const char* fmt, ...)
{
    va_list     valist;
    char        buffer[1024+4*MAX_LISTED_ENV_VAR];
    DWORD       w;

    va_start(valist, fmt);
    vsprintf(buffer, fmt, valist);
    va_end(valist);
    WriteFile(h, buffer, strlen(buffer), &w, NULL);
}


/******************************************************************
 *		doChild
 *
 * output most of the information in the child process
 */
static void     doChild(const char* file, const char* option)
{
    STARTUPINFOA        siA;
    STARTUPINFOW        siW;
    int                 i;
    char*               ptrA;
    WCHAR*              ptrW;
    char                bufA[MAX_PATH];
    WCHAR               bufW[MAX_PATH];
    HANDLE              hFile = CreateFileA(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);

    if (hFile == INVALID_HANDLE_VALUE) return;

    /* output of startup info (Ansi) */
    GetStartupInfoA(&siA);
    childPrintf(hFile,
                "[StartupInfoA]\ncb=%08ld\nlpDesktop=%s\nlpTitle=%s\n"
                "dwX=%lu\ndwY=%lu\ndwXSize=%lu\ndwYSize=%lu\n"
                "dwXCountChars=%lu\ndwYCountChars=%lu\ndwFillAttribute=%lu\n"
                "dwFlags=%lu\nwShowWindow=%u\n"
                "hStdInput=%lu\nhStdOutput=%lu\nhStdError=%lu\n\n",
                siA.cb, encodeA(siA.lpDesktop), encodeA(siA.lpTitle),
                siA.dwX, siA.dwY, siA.dwXSize, siA.dwYSize,
                siA.dwXCountChars, siA.dwYCountChars, siA.dwFillAttribute,
                siA.dwFlags, siA.wShowWindow,
                (DWORD)siA.hStdInput, (DWORD)siA.hStdOutput, (DWORD)siA.hStdError);

    /* since GetStartupInfoW is only implemented in win2k,
     * zero out before calling so we can notice the difference
     */
    memset(&siW, 0, sizeof(siW));
    GetStartupInfoW(&siW);
    childPrintf(hFile,
                "[StartupInfoW]\ncb=%08ld\nlpDesktop=%s\nlpTitle=%s\n"
                "dwX=%lu\ndwY=%lu\ndwXSize=%lu\ndwYSize=%lu\n"
                "dwXCountChars=%lu\ndwYCountChars=%lu\ndwFillAttribute=%lu\n"
                "dwFlags=%lu\nwShowWindow=%u\n"
                "hStdInput=%lu\nhStdOutput=%lu\nhStdError=%lu\n\n",
                siW.cb, encodeW(siW.lpDesktop), encodeW(siW.lpTitle),
                siW.dwX, siW.dwY, siW.dwXSize, siW.dwYSize,
                siW.dwXCountChars, siW.dwYCountChars, siW.dwFillAttribute,
                siW.dwFlags, siW.wShowWindow,
                (DWORD)siW.hStdInput, (DWORD)siW.hStdOutput, (DWORD)siW.hStdError);

    /* Arguments */
    childPrintf(hFile, "[Arguments]\nargcA=%d\n", myARGC);
    for (i = 0; i < myARGC; i++)
    {
        childPrintf(hFile, "argvA%d=%s\n", i, encodeA(myARGV[i]));
    }
    childPrintf(hFile, "CommandLineA=%s\n", encodeA(GetCommandLineA()));

#if 0
    int                 argcW;
    WCHAR**             argvW;

    /* this is part of shell32... and should be tested there */
    argvW = CommandLineToArgvW(GetCommandLineW(), &argcW);
    for (i = 0; i < argcW; i++)
    {
        childPrintf(hFile, "argvW%d=%s\n", i, encodeW(argvW[i]));
    }
#endif
    childPrintf(hFile, "CommandLineW=%s\n\n", encodeW(GetCommandLineW()));

    /* output of environment (Ansi) */
    ptrA = GetEnvironmentStringsA();
    if (ptrA)
    {
        char    env_var[MAX_LISTED_ENV_VAR];

        childPrintf(hFile, "[EnvironmentA]\n");
        i = 0;
        while (*ptrA)
        {
            strncpy(env_var, ptrA, MAX_LISTED_ENV_VAR - 1);
            env_var[MAX_LISTED_ENV_VAR - 1] = '\0';
            childPrintf(hFile, "env%d=%s\n", i, encodeA(env_var));
            i++;
            ptrA += strlen(ptrA) + 1;
        }
        childPrintf(hFile, "len=%d\n\n", i);
    }

    /* output of environment (Unicode) */
    ptrW = GetEnvironmentStringsW();
    if (ptrW)
    {
        WCHAR   env_var[MAX_LISTED_ENV_VAR];

        childPrintf(hFile, "[EnvironmentW]\n");
        i = 0;
        while (*ptrW)
        {
            lstrcpynW(env_var, ptrW, MAX_LISTED_ENV_VAR - 1);
            env_var[MAX_LISTED_ENV_VAR - 1] = '\0';
            childPrintf(hFile, "env%d=%s\n", i, encodeW(env_var));
            i++;
            ptrW += lstrlenW(ptrW) + 1;
        }
        childPrintf(hFile, "len=%d\n\n", i);
    }

    childPrintf(hFile, "[Misc]\n");
    if (GetCurrentDirectoryA(sizeof(bufA), bufA))
        childPrintf(hFile, "CurrDirA=%s\n", encodeA(bufA));
    if (GetCurrentDirectoryW(sizeof(bufW) / sizeof(bufW[0]), bufW))
        childPrintf(hFile, "CurrDirW=%s\n", encodeW(bufW));
    childPrintf(hFile, "\n");

    if (option && strcmp(option, "console") == 0)
    {
        CONSOLE_SCREEN_BUFFER_INFO	sbi;
        HANDLE hConIn  = GetStdHandle(STD_INPUT_HANDLE);
        HANDLE hConOut = GetStdHandle(STD_OUTPUT_HANDLE);
        DWORD modeIn, modeOut;

        childPrintf(hFile, "[Console]\n");
        if (GetConsoleScreenBufferInfo(hConOut, &sbi))
        {
            childPrintf(hFile, "SizeX=%d\nSizeY=%d\nCursorX=%d\nCursorY=%d\nAttributes=%d\n",
                        sbi.dwSize.X, sbi.dwSize.Y, sbi.dwCursorPosition.X, sbi.dwCursorPosition.Y, sbi.wAttributes);
            childPrintf(hFile, "winLeft=%d\nwinTop=%d\nwinRight=%d\nwinBottom=%d\n",
                        sbi.srWindow.Left, sbi.srWindow.Top, sbi.srWindow.Right, sbi.srWindow.Bottom);
            childPrintf(hFile, "maxWinWidth=%d\nmaxWinHeight=%d\n",
                        sbi.dwMaximumWindowSize.X, sbi.dwMaximumWindowSize.Y);
        }
        childPrintf(hFile, "InputCP=%d\nOutputCP=%d\n",
                    GetConsoleCP(), GetConsoleOutputCP());
        if (GetConsoleMode(hConIn, &modeIn))
            childPrintf(hFile, "InputMode=%ld\n", modeIn);
        if (GetConsoleMode(hConOut, &modeOut))
            childPrintf(hFile, "OutputMode=%ld\n", modeOut);

        /* now that we have written all relevant information, let's change it */
        ok(SetConsoleCP(1252), "Setting CP\n");
        ok(SetConsoleOutputCP(1252), "Setting SB CP\n");
        ok(SetConsoleMode(hConIn, modeIn ^ 1), "Setting mode (%ld)\n", GetLastError());
        ok(SetConsoleMode(hConOut, modeOut ^ 1), "Setting mode (%ld)\n", GetLastError());
        sbi.dwCursorPosition.X ^= 1;
        sbi.dwCursorPosition.Y ^= 1;
        ok(SetConsoleCursorPosition(hConOut, sbi.dwCursorPosition), "Setting cursor position (%ld)\n", GetLastError());
    }
    if (option && strcmp(option, "stdhandle") == 0)
    {
        HANDLE hStdIn  = GetStdHandle(STD_INPUT_HANDLE);
        HANDLE hStdOut = GetStdHandle(STD_OUTPUT_HANDLE);

        if (hStdIn != INVALID_HANDLE_VALUE || hStdOut != INVALID_HANDLE_VALUE)
        {
            char buf[1024];
            DWORD r, w;

            ok(ReadFile(hStdIn, buf, sizeof(buf), &r, NULL) && r > 0, "Reading message from input pipe\n");
            childPrintf(hFile, "[StdHandle]\nmsg=%s\n\n", encodeA(buf));
            ok(WriteFile(hStdOut, buf, r, &w, NULL) && w == r, "Writing message to output pipe\n");
        }
    }

    if (option && strcmp(option, "exit_code") == 0)
    {
        childPrintf(hFile, "[ExitCode]\nvalue=%d\n\n", 123);
        CloseHandle(hFile);
        ExitProcess(123);
    }

    CloseHandle(hFile);
}

static char* getChildString(const char* sect, const char* key)
{
    char        buf[1024+4*MAX_LISTED_ENV_VAR];
    char*       ret;

    GetPrivateProfileStringA(sect, key, "-", buf, sizeof(buf), resfile);
    if (buf[0] == '\0' || (buf[0] == '-' && buf[1] == '\0')) return NULL;
    assert(!(strlen(buf) & 1));
    ret = decodeA(buf);
    return ret;
}

/* FIXME: this may be moved to the wtmain.c file, because it may be needed by
 * others... (windows uses stricmp while Un*x uses strcasecmp...)
 */
static int wtstrcasecmp(const char* p1, const char* p2)
{
    char c1, c2;

    c1 = c2 = '@';
    while (c1 == c2 && c1)
    {
        c1 = *p1++; c2 = *p2++;
        if (c1 != c2)
        {
            c1 = toupper(c1); c2 = toupper(c2);
        }
    }
    return c1 - c2;
}

static int strCmp(const char* s1, const char* s2, BOOL sensitive)
{
    if (!s1 && !s2) return 0;
    if (!s2) return -1;
    if (!s1) return 1;
    return (sensitive) ? strcmp(s1, s2) : wtstrcasecmp(s1, s2);
}

#define okChildString(sect, key, expect) \
    do { \
        char* result = getChildString((sect), (key)); \
        ok(strCmp(result, expect, 1) == 0, "%s:%s expected '%s', got '%s'\n", (sect), (key), (expect)?(expect):"(null)", result); \
    } while (0)

#define okChildIString(sect, key, expect) \
    do { \
        char* result = getChildString(sect, key); \
        ok(strCmp(result, expect, 0) == 0, "%s:%s expected '%s', got '%s'\n", sect, key, expect, result); \
    } while (0)

/* using !expect insures that the test will fail if the sect/key isn't present
 * in result file
 */
#define okChildInt(sect, key, expect) \
    do { \
        UINT result = GetPrivateProfileIntA((sect), (key), !(expect), resfile); \
        ok(result == expect, "%s:%s expected %d, but got %d\n", (sect), (key), (int)(expect), result); \
   } while (0)

static void test_Startup(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup,si;

    /* let's start simplistic */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    GetStartupInfoA(&si);
    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", si.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", si.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = "I'm the title string";
    startup.lpDesktop = "I'm the desktop string";
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = "I'm the title string";
    startup.lpDesktop = NULL;
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", si.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = "I'm the title string";
    startup.lpDesktop = "";
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    todo_wine okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = NULL;
    startup.lpDesktop = "I'm the desktop string";
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", si.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = "";
    startup.lpDesktop = "I'm the desktop string";
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    todo_wine okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = "";
    startup.lpDesktop = "";
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    todo_wine okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    todo_wine okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    /* TODO: test for A/W and W/A and W/W */
}

static void test_CommandLine(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* the basics */
    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s \"C:\\Program Files\\my nice app.exe\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("Arguments", "argcA", 4);
    okChildString("Arguments", "argvA3", "C:\\Program Files\\my nice app.exe");
    okChildString("Arguments", "argvA4", NULL);
    okChildString("Arguments", "CommandLineA", buffer);
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* from Frangois */
    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s \"a\\\"b\\\\\" c\\\" d", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("Arguments", "argcA", 6);
    okChildString("Arguments", "argvA3", "a\"b\\");
    okChildString("Arguments", "argvA4", "c\"");
    okChildString("Arguments", "argvA5", "d");
    okChildString("Arguments", "argvA6", NULL);
    okChildString("Arguments", "CommandLineA", buffer);
    release_memory();
    assert(DeleteFileA(resfile) != 0);
}

static void test_Directory(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    char windir[MAX_PATH];

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* the basics */
    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    GetWindowsDirectoryA( windir, sizeof(windir) );
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, windir, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildIString("Misc", "CurrDirA", windir);
    release_memory();
    assert(DeleteFileA(resfile) != 0);
}

static BOOL is_str_env_drive_dir(const char* str)
{
    return str[0] == '=' && str[1] >= 'A' && str[1] <= 'Z' && str[2] == ':' &&
        str[3] == '=' && str[4] == str[1];
}

/* compared expected child's environment (in gesA) from actual
 * environment our child got
 */
static void cmpEnvironment(const char* gesA)
{
    int                 i, clen;
    const char*         ptrA;
    char*               res;
    char                key[32];
    BOOL                found;

    clen = GetPrivateProfileIntA("EnvironmentA", "len", 0, resfile);
    
    /* now look each parent env in child */
    if ((ptrA = gesA) != NULL)
    {
        while (*ptrA)
        {
            for (i = 0; i < clen; i++)
            {
                sprintf(key, "env%d", i);
                res = getChildString("EnvironmentA", key);
                if (strncmp(ptrA, res, MAX_LISTED_ENV_VAR - 1) == 0)
                    break;
            }
            found = i < clen;
            ok(found, "Parent-env string %s isn't in child process\n", ptrA);
            
            ptrA += strlen(ptrA) + 1;
            release_memory();
        }
    }
    /* and each child env in parent */
    for (i = 0; i < clen; i++)
    {
        sprintf(key, "env%d", i);
        res = getChildString("EnvironmentA", key);
        if ((ptrA = gesA) != NULL)
        {
            while (*ptrA)
            {
                if (strncmp(res, ptrA, MAX_LISTED_ENV_VAR - 1) == 0)
                    break;
                ptrA += strlen(ptrA) + 1;
            }
            if (!*ptrA) ptrA = NULL;
        }

        if (!is_str_env_drive_dir(res))
        {
            found = ptrA != NULL;
            ok(found, "Child-env string %s isn't in parent process\n", res);
        }
        /* else => should also test we get the right per drive default directory here... */
    }
}

static void test_Environment(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    char                child_env[4096];
    char*               ptr;
    char*               env;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* the basics */
    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    
    cmpEnvironment(GetEnvironmentStringsA());
    release_memory();
    assert(DeleteFileA(resfile) != 0);

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* the basics */
    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ptr = child_env;
    sprintf(ptr, "=%c:=%s", 'C', "C:\\FOO\\BAR");
    ptr += strlen(ptr) + 1;
    strcpy(ptr, "PATH=C:\\WINDOWS;C:\\WINDOWS\\SYSTEM;C:\\MY\\OWN\\DIR");
    ptr += strlen(ptr) + 1;
    strcpy(ptr, "FOO=BAR");
    ptr += strlen(ptr) + 1;
    strcpy(ptr, "BAR=FOOBAR");
    ptr += strlen(ptr) + 1;
    /* copy all existing variables except:
     * - WINELOADER
     * - PATH (already set above)
     * - the directory definitions (=[A-Z]:=)
     */
    for (env = GetEnvironmentStringsA(); *env; env += strlen(env) + 1)
    {
        if (strncmp(env, "PATH=", 5) != 0 &&
            strncmp(env, "WINELOADER=", 11) != 0 &&
            !is_str_env_drive_dir(env))
        {
            strcpy(ptr, env);
            ptr += strlen(ptr) + 1;
        }
    }
    *ptr = '\0';
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, child_env, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    
    cmpEnvironment(child_env);

    release_memory();
    assert(DeleteFileA(resfile) != 0);
}

static  void    test_SuspendFlag(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    DWORD               exit_status;

    /* let's start simplistic */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &startup, &info), "CreateProcess\n");

    ok(GetExitCodeThread(info.hThread, &exit_status) && exit_status == STILL_ACTIVE, "thread still running\n");
    Sleep(8000);
    ok(GetExitCodeThread(info.hThread, &exit_status) && exit_status == STILL_ACTIVE, "thread still running\n");
    ok(ResumeThread(info.hThread) == 1, "Resuming thread\n");

    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);
}

static  void    test_DebuggingFlag(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    DEBUG_EVENT         de;
    unsigned            dbg = 0;

    /* let's start simplistic */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &startup, &info), "CreateProcess\n");

    /* get all startup events up to the entry point break exception */
    do 
    {
        ok(WaitForDebugEvent(&de, INFINITE), "reading debug event\n");
        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
        if (de.dwDebugEventCode != EXCEPTION_DEBUG_EVENT) dbg++;
    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    ok(dbg, "I have seen a debug event\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);
    release_memory();
    assert(DeleteFileA(resfile) != 0);
}

static void test_Console(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    SECURITY_ATTRIBUTES sa;
    CONSOLE_SCREEN_BUFFER_INFO	sbi, sbiC;
    DWORD               modeIn, modeOut, modeInC, modeOutC;
    DWORD               cpIn, cpOut, cpInC, cpOutC;
    DWORD               w;
    HANDLE              hChildIn, hChildInInh, hChildOut, hChildOutInh, hParentIn, hParentOut;
    const char*         msg = "This is a std-handle inheritance test.";
    unsigned            msg_len;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;
    startup.wShowWindow = SW_SHOWNORMAL;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    startup.hStdInput = CreateFileA("CONIN$", GENERIC_READ|GENERIC_WRITE, 0, &sa, OPEN_EXISTING, 0, 0);
    startup.hStdOutput = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, &sa, OPEN_EXISTING, 0, 0);

    /* first, we need to be sure we're attached to a console */
    if (startup.hStdInput == INVALID_HANDLE_VALUE || startup.hStdOutput == INVALID_HANDLE_VALUE)
    {
        /* we're not attached to a console, let's do it */
        AllocConsole();
        startup.hStdInput = CreateFileA("CONIN$", GENERIC_READ|GENERIC_WRITE, 0, &sa, OPEN_EXISTING, 0, 0);
        startup.hStdOutput = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, &sa, OPEN_EXISTING, 0, 0);
    }
    /* now verify everything's ok */
    ok(startup.hStdInput != INVALID_HANDLE_VALUE, "Opening ConIn\n");
    ok(startup.hStdOutput != INVALID_HANDLE_VALUE, "Opening ConOut\n");
    startup.hStdError = startup.hStdOutput;

    ok(GetConsoleScreenBufferInfo(startup.hStdOutput, &sbi), "Getting sb info\n");
    ok(GetConsoleMode(startup.hStdInput, &modeIn) && 
       GetConsoleMode(startup.hStdOutput, &modeOut), "Getting console modes\n");
    cpIn = GetConsoleCP();
    cpOut = GetConsoleOutputCP();

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s console", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &info), "CreateProcess\n");

    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    /* now get the modification the child has made, and resets parents expected values */
    ok(GetConsoleScreenBufferInfo(startup.hStdOutput, &sbiC), "Getting sb info\n");
    ok(GetConsoleMode(startup.hStdInput, &modeInC) && 
       GetConsoleMode(startup.hStdOutput, &modeOutC), "Getting console modes\n");

    SetConsoleMode(startup.hStdInput, modeIn);
    SetConsoleMode(startup.hStdOutput, modeOut);

    cpInC = GetConsoleCP();
    cpOutC = GetConsoleOutputCP();
    SetConsoleCP(cpIn);
    SetConsoleOutputCP(cpOut);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    okChildString("StartupInfoA", "lpTitle", startup.lpTitle);
    okChildInt("StartupInfoA", "dwX", startup.dwX);
    okChildInt("StartupInfoA", "dwY", startup.dwY);
    okChildInt("StartupInfoA", "dwXSize", startup.dwXSize);
    okChildInt("StartupInfoA", "dwYSize", startup.dwYSize);
    okChildInt("StartupInfoA", "dwXCountChars", startup.dwXCountChars);
    okChildInt("StartupInfoA", "dwYCountChars", startup.dwYCountChars);
    okChildInt("StartupInfoA", "dwFillAttribute", startup.dwFillAttribute);
    okChildInt("StartupInfoA", "dwFlags", startup.dwFlags);
    okChildInt("StartupInfoA", "wShowWindow", startup.wShowWindow);

    /* check child correctly inherited the console */
    okChildInt("StartupInfoA", "hStdInput", (DWORD)startup.hStdInput);
    okChildInt("StartupInfoA", "hStdOutput", (DWORD)startup.hStdOutput);
    okChildInt("StartupInfoA", "hStdError", (DWORD)startup.hStdError);
    okChildInt("Console", "SizeX", sbi.dwSize.X);
    okChildInt("Console", "SizeY", sbi.dwSize.Y);
    okChildInt("Console", "CursorX", sbi.dwCursorPosition.X);
    okChildInt("Console", "CursorY", sbi.dwCursorPosition.Y);
    okChildInt("Console", "Attributes", sbi.wAttributes);
    okChildInt("Console", "winLeft", sbi.srWindow.Left);
    okChildInt("Console", "winTop", sbi.srWindow.Top);
    okChildInt("Console", "winRight", sbi.srWindow.Right);
    okChildInt("Console", "winBottom", sbi.srWindow.Bottom);
    okChildInt("Console", "maxWinWidth", sbi.dwMaximumWindowSize.X);
    okChildInt("Console", "maxWinHeight", sbi.dwMaximumWindowSize.Y);
    okChildInt("Console", "InputCP", cpIn);
    okChildInt("Console", "OutputCP", cpOut);
    okChildInt("Console", "InputMode", modeIn);
    okChildInt("Console", "OutputMode", modeOut);

    todo_wine ok(cpInC == 1252, "Wrong console CP (expected 1252 got %ld/%ld)\n", cpInC, cpIn);
    todo_wine ok(cpOutC == 1252, "Wrong console-SB CP (expected 1252 got %ld/%ld)\n", cpOutC, cpOut);
    ok(modeInC == (modeIn ^ 1), "Wrong console mode\n");
    ok(modeOutC == (modeOut ^ 1), "Wrong console-SB mode\n");
    ok(sbiC.dwCursorPosition.X == (sbi.dwCursorPosition.X ^ 1), "Wrong cursor position\n");
    ok(sbiC.dwCursorPosition.Y == (sbi.dwCursorPosition.Y ^ 1), "Wrong cursor position\n");

    release_memory();
    assert(DeleteFileA(resfile) != 0);

    ok(CreatePipe(&hParentIn, &hChildOut, NULL, 0), "Creating parent-input pipe\n");
    ok(DuplicateHandle(GetCurrentProcess(), hChildOut, GetCurrentProcess(), 
                       &hChildOutInh, 0, TRUE, DUPLICATE_SAME_ACCESS),
       "Duplicating as inheritable child-output pipe\n");
    CloseHandle(hChildOut);
 
    ok(CreatePipe(&hChildIn, &hParentOut, NULL, 0), "Creating parent-output pipe\n");
    ok(DuplicateHandle(GetCurrentProcess(), hChildIn, GetCurrentProcess(), 
                       &hChildInInh, 0, TRUE, DUPLICATE_SAME_ACCESS),
       "Duplicating as inheritable child-input pipe\n");
    CloseHandle(hChildIn); 
    
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.hStdInput = hChildInInh;
    startup.hStdOutput = hChildOutInh;
    startup.hStdError = hChildOutInh;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s stdhandle", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &startup, &info), "CreateProcess\n");
    ok(CloseHandle(hChildInInh), "Closing handle\n");
    ok(CloseHandle(hChildOutInh), "Closing handle\n");

    msg_len = strlen(msg) + 1;
    ok(WriteFile(hParentOut, msg, msg_len, &w, NULL), "Writing to child\n");
    ok(w == msg_len, "Should have written %u bytes, actually wrote %lu\n", msg_len, w);
    memset(buffer, 0, sizeof(buffer));
    ok(ReadFile(hParentIn, buffer, sizeof(buffer), &w, NULL), "Reading from child\n");
    ok(strcmp(buffer, msg) == 0, "Should have received '%s'\n", msg);

    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildString("StdHandle", "msg", msg);

    release_memory();
    assert(DeleteFileA(resfile) != 0);
}

static  void    test_ExitCode(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    DWORD               code;

    /* let's start simplistic */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    get_file_name(resfile);
    sprintf(buffer, "%s tests/process.c %s exit_code", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info), "CreateProcess\n");

    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    ok(GetExitCodeProcess(info.hProcess, &code), "Getting exit code\n");
    okChildInt("ExitCode", "value", code);

    release_memory();
    assert(DeleteFileA(resfile) != 0);
}

START_TEST(process)
{
    int b = init();
    ok(b, "Basic init of CreateProcess test\n");
    if (!b) return;

    if (myARGC >= 3)
    {
        doChild(myARGV[2], (myARGC == 3) ? NULL : myARGV[3]);
        return;
    }
    test_Startup();
    test_CommandLine();
    test_Directory();
    test_Environment();
    test_SuspendFlag();
    test_DebuggingFlag();
    test_Console();
    test_ExitCode();
    /* things that can be tested:
     *  lookup:         check the way program to be executed is searched
     *  handles:        check the handle inheritance stuff (+sec options)
     *  console:        check if console creation parameters work
     */
}
