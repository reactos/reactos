/*
 * Unit test suite for process functions
 *
 * Copyright 2002 Eric Pouech
 * Copyright 2006 Dmitry Timoshkov
 * Copyright 2014 Michael Müller
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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winuser.h"
#include "wincon.h"
#include "winnls.h"
#include "winternl.h"
#include "tlhelp32.h"

#include "wine/test.h"

#include "winnt.h"

/* PROCESS_ALL_ACCESS in Vista+ PSDKs is incompatible with older Windows versions */
#define PROCESS_ALL_ACCESS_NT4 (PROCESS_ALL_ACCESS & ~0xf000)
/* THREAD_ALL_ACCESS in Vista+ PSDKs is incompatible with older Windows versions */
#define THREAD_ALL_ACCESS_NT4 (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3ff)

#define expect_eq_d(expected, actual) \
    do { \
      int value = (actual); \
      ok((expected) == value, "Expected " #actual " to be %d (" #expected ") is %d\n", \
          (expected), value); \
    } while (0)
#define expect_eq_s(expected, actual) \
    do { \
      LPCSTR value = (actual); \
      ok(lstrcmpA((expected), value) == 0, "Expected " #actual " to be L\"%s\" (" #expected ") is L\"%s\"\n", \
          expected, value); \
    } while (0)
#define expect_eq_ws_i(expected, actual) \
    do { \
      LPCWSTR value = (actual); \
      ok(lstrcmpiW((expected), value) == 0, "Expected " #actual " to be L\"%s\" (" #expected ") is L\"%s\"\n", \
          wine_dbgstr_w(expected), wine_dbgstr_w(value)); \
    } while (0)

static HINSTANCE hkernel32, hntdll;
static void   (WINAPI *pGetNativeSystemInfo)(LPSYSTEM_INFO);
static BOOL   (WINAPI *pGetSystemRegistryQuota)(PDWORD, PDWORD);
static BOOL   (WINAPI *pIsWow64Process)(HANDLE,PBOOL);
static LPVOID (WINAPI *pVirtualAllocEx)(HANDLE, LPVOID, SIZE_T, DWORD, DWORD);
static BOOL   (WINAPI *pVirtualFreeEx)(HANDLE, LPVOID, SIZE_T, DWORD);
static BOOL   (WINAPI *pQueryFullProcessImageNameA)(HANDLE hProcess, DWORD dwFlags, LPSTR lpExeName, PDWORD lpdwSize);
static BOOL   (WINAPI *pQueryFullProcessImageNameW)(HANDLE hProcess, DWORD dwFlags, LPWSTR lpExeName, PDWORD lpdwSize);
static DWORD  (WINAPI *pK32GetProcessImageFileNameA)(HANDLE,LPSTR,DWORD);
static HANDLE (WINAPI *pCreateJobObjectW)(LPSECURITY_ATTRIBUTES sa, LPCWSTR name);
static BOOL   (WINAPI *pAssignProcessToJobObject)(HANDLE job, HANDLE process);
static BOOL   (WINAPI *pIsProcessInJob)(HANDLE process, HANDLE job, PBOOL result);
static BOOL   (WINAPI *pTerminateJobObject)(HANDLE job, UINT exit_code);
static BOOL   (WINAPI *pQueryInformationJobObject)(HANDLE job, JOBOBJECTINFOCLASS class, LPVOID info, DWORD len, LPDWORD ret_len);
static BOOL   (WINAPI *pSetInformationJobObject)(HANDLE job, JOBOBJECTINFOCLASS class, LPVOID info, DWORD len);
static HANDLE (WINAPI *pCreateIoCompletionPort)(HANDLE file, HANDLE existing_port, ULONG_PTR key, DWORD threads);
static BOOL   (WINAPI *pGetNumaProcessorNode)(UCHAR, PUCHAR);
static NTSTATUS (WINAPI *pNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
static BOOL   (WINAPI *pProcessIdToSessionId)(DWORD,DWORD*);
static DWORD  (WINAPI *pWTSGetActiveConsoleSessionId)(void);
static HANDLE (WINAPI *pCreateToolhelp32Snapshot)(DWORD, DWORD);
static BOOL   (WINAPI *pProcess32First)(HANDLE, PROCESSENTRY32*);
static BOOL   (WINAPI *pProcess32Next)(HANDLE, PROCESSENTRY32*);
static BOOL   (WINAPI *pThread32First)(HANDLE, THREADENTRY32*);
static BOOL   (WINAPI *pThread32Next)(HANDLE, THREADENTRY32*);
static BOOL   (WINAPI *pGetLogicalProcessorInformationEx)(LOGICAL_PROCESSOR_RELATIONSHIP,SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*,DWORD*);
static SIZE_T (WINAPI *pGetLargePageMinimum)(void);
static BOOL   (WINAPI *pInitializeProcThreadAttributeList)(struct _PROC_THREAD_ATTRIBUTE_LIST*, DWORD, DWORD, SIZE_T*);
static BOOL   (WINAPI *pUpdateProcThreadAttribute)(struct _PROC_THREAD_ATTRIBUTE_LIST*, DWORD, DWORD_PTR, void *,SIZE_T,void*,SIZE_T*);
static void   (WINAPI *pDeleteProcThreadAttributeList)(struct _PROC_THREAD_ATTRIBUTE_LIST*);
static DWORD  (WINAPI *pGetActiveProcessorCount)(WORD);

/* ############################### */
static char     base[MAX_PATH];
static char     selfname[MAX_PATH];
static char*    exename;
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

static char     memory[1024*256];
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

/******************************************************************
 *		init
 *
 * generates basic information like:
 *      base:           absolute path to curr dir
 *      selfname:       the way to reinvoke ourselves
 *      exename:        executable without the path
 * function-pointers, which are not implemented in all windows versions
 */
static BOOL init(void)
{
    char *p;

    myARGC = winetest_get_mainargs( &myARGV );
    if (!GetCurrentDirectoryA(sizeof(base), base)) return FALSE;
    strcpy(selfname, myARGV[0]);

    /* Strip the path of selfname */
    if ((p = strrchr(selfname, '\\')) != NULL) exename = p + 1;
    else exename = selfname;

    if ((p = strrchr(exename, '/')) != NULL) exename = p + 1;

    hkernel32 = GetModuleHandleA("kernel32");
    hntdll    = GetModuleHandleA("ntdll.dll");

    pNtQueryInformationProcess = (void *)GetProcAddress(hntdll, "NtQueryInformationProcess");

    pGetNativeSystemInfo = (void *) GetProcAddress(hkernel32, "GetNativeSystemInfo");
    pGetSystemRegistryQuota = (void *) GetProcAddress(hkernel32, "GetSystemRegistryQuota");
    pIsWow64Process = (void *) GetProcAddress(hkernel32, "IsWow64Process");
    pVirtualAllocEx = (void *) GetProcAddress(hkernel32, "VirtualAllocEx");
    pVirtualFreeEx = (void *) GetProcAddress(hkernel32, "VirtualFreeEx");
    pQueryFullProcessImageNameA = (void *) GetProcAddress(hkernel32, "QueryFullProcessImageNameA");
    pQueryFullProcessImageNameW = (void *) GetProcAddress(hkernel32, "QueryFullProcessImageNameW");
    pK32GetProcessImageFileNameA = (void *) GetProcAddress(hkernel32, "K32GetProcessImageFileNameA");
    pCreateJobObjectW = (void *)GetProcAddress(hkernel32, "CreateJobObjectW");
    pAssignProcessToJobObject = (void *)GetProcAddress(hkernel32, "AssignProcessToJobObject");
    pIsProcessInJob = (void *)GetProcAddress(hkernel32, "IsProcessInJob");
    pTerminateJobObject = (void *)GetProcAddress(hkernel32, "TerminateJobObject");
    pQueryInformationJobObject = (void *)GetProcAddress(hkernel32, "QueryInformationJobObject");
    pSetInformationJobObject = (void *)GetProcAddress(hkernel32, "SetInformationJobObject");
    pCreateIoCompletionPort = (void *)GetProcAddress(hkernel32, "CreateIoCompletionPort");
    pGetNumaProcessorNode = (void *)GetProcAddress(hkernel32, "GetNumaProcessorNode");
    pProcessIdToSessionId = (void *)GetProcAddress(hkernel32, "ProcessIdToSessionId");
    pWTSGetActiveConsoleSessionId = (void *)GetProcAddress(hkernel32, "WTSGetActiveConsoleSessionId");
    pCreateToolhelp32Snapshot = (void *)GetProcAddress(hkernel32, "CreateToolhelp32Snapshot");
    pProcess32First = (void *)GetProcAddress(hkernel32, "Process32First");
    pProcess32Next = (void *)GetProcAddress(hkernel32, "Process32Next");
    pThread32First = (void *)GetProcAddress(hkernel32, "Thread32First");
    pThread32Next = (void *)GetProcAddress(hkernel32, "Thread32Next");
    pGetLogicalProcessorInformationEx = (void *)GetProcAddress(hkernel32, "GetLogicalProcessorInformationEx");
    pGetLargePageMinimum = (void *)GetProcAddress(hkernel32, "GetLargePageMinimum");
    pInitializeProcThreadAttributeList = (void *)GetProcAddress(hkernel32, "InitializeProcThreadAttributeList");
    pUpdateProcThreadAttribute = (void *)GetProcAddress(hkernel32, "UpdateProcThreadAttribute");
    pDeleteProcThreadAttributeList = (void *)GetProcAddress(hkernel32, "DeleteProcThreadAttributeList");
    pGetActiveProcessorCount = (void *)GetProcAddress(hkernel32, "GetActiveProcessorCount");

    return TRUE;
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
static void WINETEST_PRINTF_ATTR(2,3) childPrintf(HANDLE h, const char* fmt, ...)
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
    RTL_USER_PROCESS_PARAMETERS *params = NtCurrentTeb()->Peb->ProcessParameters;
    STARTUPINFOA        siA;
    STARTUPINFOW        siW;
    int                 i;
    char                *ptrA, *ptrA_save;
    WCHAR               *ptrW, *ptrW_save;
    char                bufA[MAX_PATH];
    WCHAR               bufW[MAX_PATH];
    HANDLE              hFile = CreateFileA(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    HANDLE              snapshot;
    PROCESSENTRY32      pe;
    BOOL ret;

    if (hFile == INVALID_HANDLE_VALUE) return;

    /* output of startup info (Ansi) */
    GetStartupInfoA(&siA);
    childPrintf(hFile,
                "[StartupInfoA]\ncb=%08u\nlpDesktop=%s\nlpTitle=%s\n"
                "dwX=%u\ndwY=%u\ndwXSize=%u\ndwYSize=%u\n"
                "dwXCountChars=%u\ndwYCountChars=%u\ndwFillAttribute=%u\n"
                "dwFlags=%u\nwShowWindow=%u\n"
                "hStdInput=%lu\nhStdOutput=%lu\nhStdError=%lu\n\n",
                siA.cb, encodeA(siA.lpDesktop), encodeA(siA.lpTitle),
                siA.dwX, siA.dwY, siA.dwXSize, siA.dwYSize,
                siA.dwXCountChars, siA.dwYCountChars, siA.dwFillAttribute,
                siA.dwFlags, siA.wShowWindow,
                (DWORD_PTR)siA.hStdInput, (DWORD_PTR)siA.hStdOutput, (DWORD_PTR)siA.hStdError);

    /* check the console handles in the TEB */
    childPrintf(hFile, "[TEB]\nhStdInput=%lu\nhStdOutput=%lu\nhStdError=%lu\n\n",
                (DWORD_PTR)params->hStdInput, (DWORD_PTR)params->hStdOutput,
                (DWORD_PTR)params->hStdError);

    /* since GetStartupInfoW is only implemented in win2k,
     * zero out before calling so we can notice the difference
     */
    memset(&siW, 0, sizeof(siW));
    GetStartupInfoW(&siW);
    childPrintf(hFile,
                "[StartupInfoW]\ncb=%08u\nlpDesktop=%s\nlpTitle=%s\n"
                "dwX=%u\ndwY=%u\ndwXSize=%u\ndwYSize=%u\n"
                "dwXCountChars=%u\ndwYCountChars=%u\ndwFillAttribute=%u\n"
                "dwFlags=%u\nwShowWindow=%u\n"
                "hStdInput=%lu\nhStdOutput=%lu\nhStdError=%lu\n\n",
                siW.cb, encodeW(siW.lpDesktop), encodeW(siW.lpTitle),
                siW.dwX, siW.dwY, siW.dwXSize, siW.dwYSize,
                siW.dwXCountChars, siW.dwYCountChars, siW.dwFillAttribute,
                siW.dwFlags, siW.wShowWindow,
                (DWORD_PTR)siW.hStdInput, (DWORD_PTR)siW.hStdOutput, (DWORD_PTR)siW.hStdError);

    /* Arguments */
    childPrintf(hFile, "[Arguments]\nargcA=%d\n", myARGC);
    for (i = 0; i < myARGC; i++)
    {
        childPrintf(hFile, "argvA%d=%s\n", i, encodeA(myARGV[i]));
    }
    childPrintf(hFile, "CommandLineA=%s\n", encodeA(GetCommandLineA()));
    childPrintf(hFile, "CommandLineW=%s\n\n", encodeW(GetCommandLineW()));

    /* output toolhelp information */
    snapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    ok(snapshot != INVALID_HANDLE_VALUE, "CreateToolhelp32Snapshot failed %u\n", GetLastError());
    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);
    if (pProcess32First(snapshot, &pe))
    {
        while (pe.th32ProcessID != GetCurrentProcessId())
            if (!pProcess32Next(snapshot, &pe)) break;
    }
    CloseHandle(snapshot);
    ok(pe.th32ProcessID == GetCurrentProcessId(), "failed to find current process in snapshot\n");
    childPrintf(hFile,
                "[Toolhelp]\ncntUsage=%u\nth32DefaultHeapID=%lu\n"
                "th32ModuleID=%u\ncntThreads=%u\nth32ParentProcessID=%u\n"
                "pcPriClassBase=%u\ndwFlags=%u\nszExeFile=%s\n\n",
                pe.cntUsage, pe.th32DefaultHeapID, pe.th32ModuleID,
                pe.cntThreads, pe.th32ParentProcessID, pe.pcPriClassBase,
                pe.dwFlags, encodeA(pe.szExeFile));

    /* output of environment (Ansi) */
    ptrA_save = ptrA = GetEnvironmentStringsA();
    if (ptrA)
    {
        char    env_var[MAX_LISTED_ENV_VAR];

        childPrintf(hFile, "[EnvironmentA]\n");
        i = 0;
        while (*ptrA)
        {
            lstrcpynA(env_var, ptrA, MAX_LISTED_ENV_VAR);
            childPrintf(hFile, "env%d=%s\n", i, encodeA(env_var));
            i++;
            ptrA += strlen(ptrA) + 1;
        }
        childPrintf(hFile, "len=%d\n\n", i);
        FreeEnvironmentStringsA(ptrA_save);
    }

    /* output of environment (Unicode) */
    ptrW_save = ptrW = GetEnvironmentStringsW();
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
        FreeEnvironmentStringsW(ptrW_save);
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
            childPrintf(hFile, "InputMode=%u\n", modeIn);
        if (GetConsoleMode(hConOut, &modeOut))
            childPrintf(hFile, "OutputMode=%u\n", modeOut);

        /* now that we have written all relevant information, let's change it */
        SetLastError(0xdeadbeef);
        ret = SetConsoleCP(1252);
        if (!ret && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        {
            win_skip("Setting the codepage is not implemented\n");
        }
        else
        {
            ok(ret, "Setting CP\n");
            ok(SetConsoleOutputCP(1252), "Setting SB CP\n");
        }

        ret = SetConsoleMode(hConIn, modeIn ^ 1);
        ok( ret, "Setting mode (%d)\n", GetLastError());
        ret = SetConsoleMode(hConOut, modeOut ^ 1);
        ok( ret, "Setting mode (%d)\n", GetLastError());
        sbi.dwCursorPosition.X ^= 1;
        sbi.dwCursorPosition.Y ^= 1;
        ret = SetConsoleCursorPosition(hConOut, sbi.dwCursorPosition);
        ok( ret, "Setting cursor position (%d)\n", GetLastError());
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

static WCHAR* getChildStringW(const char* sect, const char* key)
{
    char        buf[1024+4*MAX_LISTED_ENV_VAR];
    WCHAR*       ret;

    GetPrivateProfileStringA(sect, key, "-", buf, sizeof(buf), resfile);
    if (buf[0] == '\0' || (buf[0] == '-' && buf[1] == '\0')) return NULL;
    assert(!(strlen(buf) & 1));
    ret = decodeW(buf);
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

static void ok_child_string( int line, const char *sect, const char *key,
                             const char *expect, int sensitive )
{
    char* result = getChildString( sect, key );
    ok_(__FILE__, line)( strCmp(result, expect, sensitive) == 0, "%s:%s expected '%s', got '%s'\n",
                         sect, key, expect ? expect : "(null)", result );
}

static void ok_child_stringWA( int line, const char *sect, const char *key,
                             const char *expect, int sensitive )
{
    WCHAR* expectW;
    CHAR* resultA;
    DWORD len;
    WCHAR* result = getChildStringW( sect, key );

    len = MultiByteToWideChar( CP_ACP, 0, expect, -1, NULL, 0);
    expectW = HeapAlloc(GetProcessHeap(),0,len*sizeof(WCHAR));
    MultiByteToWideChar( CP_ACP, 0, expect, -1, expectW, len);

    len = WideCharToMultiByte( CP_ACP, 0, result, -1, NULL, 0, NULL, NULL);
    resultA = HeapAlloc(GetProcessHeap(),0,len*sizeof(CHAR));
    WideCharToMultiByte( CP_ACP, 0, result, -1, resultA, len, NULL, NULL);

    if (sensitive)
        ok_(__FILE__, line)( lstrcmpW(result, expectW) == 0, "%s:%s expected '%s', got '%s'\n",
                         sect, key, expect ? expect : "(null)", resultA );
    else
        ok_(__FILE__, line)( lstrcmpiW(result, expectW) == 0, "%s:%s expected '%s', got '%s'\n",
                         sect, key, expect ? expect : "(null)", resultA );
    HeapFree(GetProcessHeap(),0,expectW);
    HeapFree(GetProcessHeap(),0,resultA);
}

static void ok_child_int( int line, const char *sect, const char *key, UINT expect )
{
    UINT result = GetPrivateProfileIntA( sect, key, !expect, resfile );
    ok_(__FILE__, line)( result == expect, "%s:%s expected %u, but got %u\n", sect, key, expect, result );
}

#define okChildString(sect, key, expect) ok_child_string(__LINE__, (sect), (key), (expect), 1 )
#define okChildIString(sect, key, expect) ok_child_string(__LINE__, (sect), (key), (expect), 0 )
#define okChildStringWA(sect, key, expect) ok_child_stringWA(__LINE__, (sect), (key), (expect), 1 )
#define okChildInt(sect, key, expect) ok_child_int(__LINE__, (sect), (key), (expect))

static void test_Startup(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup,si;
    char *result;
    static CHAR title[]   = "I'm the title string",
                desktop[] = "winsta0\\default",
                empty[]   = "";

    /* let's start simplistic */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

    GetStartupInfoA(&si);
    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", si.lpDesktop);
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
    DeleteFileA(resfile);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = title;
    startup.lpDesktop = desktop;
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

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
    DeleteFileA(resfile);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = title;
    startup.lpDesktop = NULL;
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

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
    DeleteFileA(resfile);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = title;
    startup.lpDesktop = empty;
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

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
    DeleteFileA(resfile);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = NULL;
    startup.lpDesktop = desktop;
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", startup.lpDesktop);
    result = getChildString( "StartupInfoA", "lpTitle" );
    ok( broken(!result) || (result && !strCmp( result, selfname, 0 )),
        "expected '%s' or null, got '%s'\n", selfname, result );
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
    DeleteFileA(resfile);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = empty;
    startup.lpDesktop = desktop;
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

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
    DeleteFileA(resfile);

    /* not so simplistic now */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.lpTitle = empty;
    startup.lpDesktop = empty;
    startup.dwXCountChars = 0x12121212;
    startup.dwYCountChars = 0x23232323;
    startup.dwX = 0x34343434;
    startup.dwY = 0x45454545;
    startup.dwXSize = 0x56565656;
    startup.dwYSize = 0x67676767;
    startup.dwFillAttribute = 0xA55A;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

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
    DeleteFileA(resfile);

    /* TODO: test for A/W and W/A and W/W */
}

static void test_CommandLine(void)
{
    char                buffer[MAX_PATH], fullpath[MAX_PATH], *lpFilePart, *p;
    char                buffer2[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    BOOL                ret;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* the basics */
    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\" \"C:\\Program Files\\my nice app.exe\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

    okChildInt("Arguments", "argcA", 5);
    okChildString("Arguments", "argvA4", "C:\\Program Files\\my nice app.exe");
    okChildString("Arguments", "argvA5", NULL);
    okChildString("Arguments", "CommandLineA", buffer);
    release_memory();
    DeleteFileA(resfile);

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* from François */
    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\" \"a\\\"b\\\\\" c\\\" d", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

    okChildInt("Arguments", "argcA", 7);
    okChildString("Arguments", "argvA4", "a\"b\\");
    okChildString("Arguments", "argvA5", "c\"");
    okChildString("Arguments", "argvA6", "d");
    okChildString("Arguments", "argvA7", NULL);
    okChildString("Arguments", "CommandLineA", buffer);
    release_memory();
    DeleteFileA(resfile);

    /* Test for Bug1330 to show that XP doesn't change '/' to '\\' in argv[0]*/
    get_file_name(resfile);
    /* Use exename to avoid buffer containing things like 'C:' */
    sprintf(buffer, "./%s tests/process.c dump \"%s\" \"a\\\"b\\\\\" c\\\" d", exename, resfile);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess (%s) failed : %d\n", buffer, GetLastError());
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);
    sprintf(buffer, "./%s", exename);
    okChildString("Arguments", "argvA0", buffer);
    release_memory();
    DeleteFileA(resfile);

    get_file_name(resfile);
    /* Use exename to avoid buffer containing things like 'C:' */
    sprintf(buffer, ".\\%s tests/process.c dump \"%s\" \"a\\\"b\\\\\" c\\\" d", exename, resfile);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess (%s) failed : %d\n", buffer, GetLastError());
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);
    sprintf(buffer, ".\\%s", exename);
    okChildString("Arguments", "argvA0", buffer);
    release_memory();
    DeleteFileA(resfile);

    get_file_name(resfile);
    GetFullPathNameA(selfname, MAX_PATH, fullpath, &lpFilePart);
    assert ( lpFilePart != 0);
    *(lpFilePart -1 ) = 0;
    p = strrchr(fullpath, '\\');
    /* Use exename to avoid buffer containing things like 'C:' */
    if (p) sprintf(buffer, "..%s/%s tests/process.c dump \"%s\" \"a\\\"b\\\\\" c\\\" d", p, exename, resfile);
    else sprintf(buffer, "./%s tests/process.c dump \"%s\" \"a\\\"b\\\\\" c\\\" d", exename, resfile);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess (%s) failed : %d\n", buffer, GetLastError());
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);
    if (p) sprintf(buffer, "..%s/%s", p, exename);
    else sprintf(buffer, "./%s", exename);
    okChildString("Arguments", "argvA0", buffer);
    release_memory();
    DeleteFileA(resfile);

    /* Using AppName */
    get_file_name(resfile);
    GetFullPathNameA(selfname, MAX_PATH, fullpath, &lpFilePart);
    assert ( lpFilePart != 0);
    *(lpFilePart -1 ) = 0;
    p = strrchr(fullpath, '\\');
    /* Use exename to avoid buffer containing things like 'C:' */
    if (p) sprintf(buffer, "..%s/%s", p, exename);
    else sprintf(buffer, "./%s", exename);
    sprintf(buffer2, "dummy tests/process.c dump \"%s\" \"a\\\"b\\\\\" c\\\" d", resfile);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(buffer, buffer2, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess (%s) failed : %d\n", buffer, GetLastError());
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);
    sprintf(buffer, "tests/process.c dump %s", resfile);
    okChildString("Arguments", "argvA0", "dummy");
    okChildString("Arguments", "CommandLineA", buffer2);
    okChildStringWA("Arguments", "CommandLineW", buffer2);
    release_memory();
    DeleteFileA(resfile);

    if (0) /* Test crashes on NT-based Windows. */
    {
        /* Test NULL application name and command line parameters. */
        SetLastError(0xdeadbeef);
        ret = CreateProcessA(NULL, NULL, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
        ok(!ret, "CreateProcessA unexpectedly succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    }

    buffer[0] = '\0';

    /* Test empty application name parameter. */
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(buffer, NULL, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_PATH_NOT_FOUND ||
       broken(GetLastError() == ERROR_FILE_NOT_FOUND) /* Win9x/WinME */ ||
       broken(GetLastError() == ERROR_ACCESS_DENIED) /* Win98 */,
       "Expected ERROR_PATH_NOT_FOUND, got %d\n", GetLastError());

    buffer2[0] = '\0';

    /* Test empty application name and command line parameters. */
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(buffer, buffer2, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_PATH_NOT_FOUND ||
       broken(GetLastError() == ERROR_FILE_NOT_FOUND) /* Win9x/WinME */ ||
       broken(GetLastError() == ERROR_ACCESS_DENIED) /* Win98 */,
       "Expected ERROR_PATH_NOT_FOUND, got %d\n", GetLastError());

    /* Test empty command line parameter. */
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, buffer2, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       GetLastError() == ERROR_PATH_NOT_FOUND /* NT4 */ ||
       GetLastError() == ERROR_BAD_PATHNAME /* Win98 */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* Win7 */,
       "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    strcpy(buffer, "doesnotexist.exe");
    strcpy(buffer2, "does not exist.exe");

    /* Test nonexistent application name. */
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(buffer, NULL, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CreateProcessA(buffer2, NULL, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    /* Test nonexistent command line parameter. */
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, buffer2, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());
}

static void test_Directory(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    char windir[MAX_PATH];
    static CHAR cmdline[] = "winver.exe";

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* the basics */
    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    GetWindowsDirectoryA( windir, sizeof(windir) );
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, windir, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

    okChildIString("Misc", "CurrDirA", windir);
    release_memory();
    DeleteFileA(resfile);

    /* search PATH for the exe if directory is NULL */
    ok(CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    ok(TerminateProcess(info.hProcess, 0), "Child process termination\n");
    CloseHandle(info.hThread);
    CloseHandle(info.hProcess);

    /* if any directory is provided, don't search PATH, error on bad directory */
    SetLastError(0xdeadbeef);
    memset(&info, 0, sizeof(info));
    ok(!CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0L,
                       NULL, "non\\existent\\directory", &startup, &info), "CreateProcess\n");
    ok(GetLastError() == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %d\n", GetLastError());
    ok(!TerminateProcess(info.hProcess, 0), "Child process should not exist\n");
}

static void test_Toolhelp(void)
{
    char                buffer[MAX_PATH];
    STARTUPINFOA        startup;
    PROCESS_INFORMATION info;
    HANDLE              process, thread, snapshot;
    PROCESSENTRY32      pe;
    THREADENTRY32       te;
    DWORD               ret;
    int                 i;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess failed\n");
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);

    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    okChildInt("Toolhelp", "cntUsage", 0);
    okChildInt("Toolhelp", "th32DefaultHeapID", 0);
    okChildInt("Toolhelp", "th32ModuleID", 0);
    okChildInt("Toolhelp", "th32ParentProcessID", GetCurrentProcessId());
    /* pcPriClassBase differs between Windows versions (either 6 or 8) */
    okChildInt("Toolhelp", "dwFlags", 0);

    release_memory();
    DeleteFileA(resfile);

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c nested \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess failed\n");
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");

    process = OpenProcess(PROCESS_ALL_ACCESS_NT4, FALSE, info.dwProcessId);
    ok(process != NULL, "OpenProcess failed %u\n", GetLastError());
    CloseHandle(process);

    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);

    for (i = 0; i < 20; i++)
    {
        SetLastError(0xdeadbeef);
        process = OpenProcess(PROCESS_ALL_ACCESS_NT4, FALSE, info.dwProcessId);
        ok(process || GetLastError() == ERROR_INVALID_PARAMETER, "OpenProcess failed %u\n", GetLastError());
        if (!process) break;
        CloseHandle(process);
        Sleep(100);
    }
    /* The following test fails randomly on some Windows versions, but Gothic 2 depends on it */
    ok(i < 20 || broken(i == 20), "process object not released\n");

    snapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    ok(snapshot != INVALID_HANDLE_VALUE, "CreateToolhelp32Snapshot failed %u\n", GetLastError());
    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);
    if (pProcess32First(snapshot, &pe))
    {
        while (pe.th32ParentProcessID != info.dwProcessId)
            if (!pProcess32Next(snapshot, &pe)) break;
    }
    CloseHandle(snapshot);
    ok(pe.th32ParentProcessID == info.dwProcessId, "failed to find nested child process\n");

    process = OpenProcess(PROCESS_ALL_ACCESS_NT4, FALSE, pe.th32ProcessID);
    ok(process != NULL, "OpenProcess failed %u\n", GetLastError());

    snapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    ok(snapshot != INVALID_HANDLE_VALUE, "CreateToolhelp32Snapshot failed %u\n", GetLastError());
    memset(&te, 0, sizeof(te));
    te.dwSize = sizeof(te);
    if (pThread32First(snapshot, &te))
    {
        while (te.th32OwnerProcessID != pe.th32ProcessID)
            if (!pThread32Next(snapshot, &te)) break;
    }
    CloseHandle(snapshot);
    ok(te.th32OwnerProcessID == pe.th32ProcessID, "failed to find suspended thread\n");

    thread = OpenThread(THREAD_ALL_ACCESS_NT4, FALSE, te.th32ThreadID);
    ok(thread != NULL, "OpenThread failed %u\n", GetLastError());
    ret = ResumeThread(thread);
    ok(ret == 1, "expected 1, got %u\n", ret);
    CloseHandle(thread);

    ok(WaitForSingleObject(process, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    CloseHandle(process);

    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    okChildInt("Toolhelp", "cntUsage", 0);
    okChildInt("Toolhelp", "th32DefaultHeapID", 0);
    okChildInt("Toolhelp", "th32ModuleID", 0);
    okChildInt("Toolhelp", "th32ParentProcessID", info.dwProcessId);
    /* pcPriClassBase differs between Windows versions (either 6 or 8) */
    okChildInt("Toolhelp", "dwFlags", 0);

    release_memory();
    DeleteFileA(resfile);
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
    char                *child_env;
    int                 child_env_len;
    char                *ptr;
    char                *ptr2;
    char                *env;
    int                 slen;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* the basics */
    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    env = GetEnvironmentStringsA();
    cmpEnvironment(env);
    release_memory();
    DeleteFileA(resfile);

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* the basics */
    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);

    child_env_len = 0;
    ptr = env;
    while(*ptr)
    {
        slen = strlen(ptr)+1;
        child_env_len += slen;
        ptr += slen;
    }
    /* Add space for additional environment variables */
    child_env_len += 256;
    child_env = HeapAlloc(GetProcessHeap(), 0, child_env_len);

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
    for (ptr2 = env; *ptr2; ptr2 += strlen(ptr2) + 1)
    {
        if (strncmp(ptr2, "PATH=", 5) != 0 &&
            strncmp(ptr2, "WINELOADER=", 11) != 0 &&
            !is_str_env_drive_dir(ptr2))
        {
            strcpy(ptr, ptr2);
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

    HeapFree(GetProcessHeap(), 0, child_env);
    FreeEnvironmentStringsA(env);
    release_memory();
    DeleteFileA(resfile);
}

static  void    test_SuspendFlag(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA       startup, us;
    DWORD               exit_status;
    char *result;

    /* let's start simplistic */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &startup, &info), "CreateProcess\n");

    ok(GetExitCodeThread(info.hThread, &exit_status) && exit_status == STILL_ACTIVE, "thread still running\n");
    Sleep(1000);
    ok(GetExitCodeThread(info.hThread, &exit_status) && exit_status == STILL_ACTIVE, "thread still running\n");
    ok(ResumeThread(info.hThread) == 1, "Resuming thread\n");

    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    GetStartupInfoA(&us);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", us.lpDesktop);
    result = getChildString( "StartupInfoA", "lpTitle" );
    ok( broken(!result) || (result && !strCmp( result, selfname, 0 )),
        "expected '%s' or null, got '%s'\n", selfname, result );
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
    DeleteFileA(resfile);
}

static  void    test_DebuggingFlag(void)
{
    char                buffer[MAX_PATH];
    void               *processbase = NULL;
    PROCESS_INFORMATION	info;
    STARTUPINFOA       startup, us;
    DEBUG_EVENT         de;
    unsigned            dbg = 0;
    char *result;

    /* let's start simplistic */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &startup, &info), "CreateProcess\n");

    /* get all startup events up to the entry point break exception */
    do 
    {
        ok(WaitForDebugEvent(&de, INFINITE), "reading debug event\n");
        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
        if (!dbg)
        {
            ok(de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT,
               "first event: %d\n", de.dwDebugEventCode);
            processbase = de.u.CreateProcessInfo.lpBaseOfImage;
        }
        if (de.dwDebugEventCode != EXCEPTION_DEBUG_EVENT) dbg++;
        ok(de.dwDebugEventCode != LOAD_DLL_DEBUG_EVENT ||
           de.u.LoadDll.lpBaseOfDll != processbase, "got LOAD_DLL for main module\n");
    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    ok(dbg, "I have seen a debug event\n");
    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    GetStartupInfoA(&us);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", us.lpDesktop);
    result = getChildString( "StartupInfoA", "lpTitle" );
    ok( broken(!result) || (result && !strCmp( result, selfname, 0 )),
        "expected '%s' or null, got '%s'\n", selfname, result );
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
    DeleteFileA(resfile);
}

static BOOL is_console(HANDLE h)
{
    return h != INVALID_HANDLE_VALUE && ((ULONG_PTR)h & 3) == 3;
}

static void test_Console(void)
{
    char                buffer[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA       startup, us;
    SECURITY_ATTRIBUTES sa;
    CONSOLE_SCREEN_BUFFER_INFO	sbi, sbiC;
    DWORD               modeIn, modeOut, modeInC, modeOutC;
    DWORD               cpIn, cpOut, cpInC, cpOutC;
    DWORD               w;
    HANDLE              hChildIn, hChildInInh, hChildOut, hChildOutInh, hParentIn, hParentOut;
    const char*         msg = "This is a std-handle inheritance test.";
    unsigned            msg_len;
    BOOL                run_tests = TRUE;
    char *result;

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
    if (!is_console(startup.hStdInput) || !is_console(startup.hStdOutput))
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
    ok(GetConsoleMode(startup.hStdInput, &modeIn), "Getting console in mode\n");
    ok(GetConsoleMode(startup.hStdOutput, &modeOut), "Getting console out mode\n");
    cpIn = GetConsoleCP();
    cpOut = GetConsoleOutputCP();

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\" console", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &info), "CreateProcess\n");

    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    /* now get the modification the child has made, and resets parents expected values */
    ok(GetConsoleScreenBufferInfo(startup.hStdOutput, &sbiC), "Getting sb info\n");
    ok(GetConsoleMode(startup.hStdInput, &modeInC), "Getting console in mode\n");
    ok(GetConsoleMode(startup.hStdOutput, &modeOutC), "Getting console out mode\n");

    SetConsoleMode(startup.hStdInput, modeIn);
    SetConsoleMode(startup.hStdOutput, modeOut);

    cpInC = GetConsoleCP();
    cpOutC = GetConsoleOutputCP();

    /* Try to set invalid CP */
    SetLastError(0xdeadbeef);
    ok(!SetConsoleCP(0), "Shouldn't succeed\n");
    ok(GetLastError()==ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED), /* win9x */
       "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_PARAMETER, GetLastError());
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        run_tests = FALSE;


    SetLastError(0xdeadbeef);
    ok(!SetConsoleOutputCP(0), "Shouldn't succeed\n");
    ok(GetLastError()==ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED), /* win9x */
       "GetLastError: expecting %u got %u\n",
       ERROR_INVALID_PARAMETER, GetLastError());

    SetConsoleCP(cpIn);
    SetConsoleOutputCP(cpOut);

    GetStartupInfoA(&us);

    okChildInt("StartupInfoA", "cb", startup.cb);
    okChildString("StartupInfoA", "lpDesktop", us.lpDesktop);
    result = getChildString( "StartupInfoA", "lpTitle" );
    ok( broken(!result) || (result && !strCmp( result, selfname, 0 )),
        "expected '%s' or null, got '%s'\n", selfname, result );
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
    okChildInt("StartupInfoA", "hStdInput", (DWORD_PTR)startup.hStdInput);
    okChildInt("StartupInfoA", "hStdOutput", (DWORD_PTR)startup.hStdOutput);
    okChildInt("StartupInfoA", "hStdError", (DWORD_PTR)startup.hStdError);
    okChildInt("Console", "SizeX", (DWORD)sbi.dwSize.X);
    okChildInt("Console", "SizeY", (DWORD)sbi.dwSize.Y);
    okChildInt("Console", "CursorX", (DWORD)sbi.dwCursorPosition.X);
    okChildInt("Console", "CursorY", (DWORD)sbi.dwCursorPosition.Y);
    okChildInt("Console", "Attributes", sbi.wAttributes);
    okChildInt("Console", "winLeft", (DWORD)sbi.srWindow.Left);
    okChildInt("Console", "winTop", (DWORD)sbi.srWindow.Top);
    okChildInt("Console", "winRight", (DWORD)sbi.srWindow.Right);
    okChildInt("Console", "winBottom", (DWORD)sbi.srWindow.Bottom);
    okChildInt("Console", "maxWinWidth", (DWORD)sbi.dwMaximumWindowSize.X);
    okChildInt("Console", "maxWinHeight", (DWORD)sbi.dwMaximumWindowSize.Y);
    okChildInt("Console", "InputCP", cpIn);
    okChildInt("Console", "OutputCP", cpOut);
    okChildInt("Console", "InputMode", modeIn);
    okChildInt("Console", "OutputMode", modeOut);

    if (run_tests)
    {
        ok(cpInC == 1252, "Wrong console CP (expected 1252 got %d/%d)\n", cpInC, cpIn);
        ok(cpOutC == 1252, "Wrong console-SB CP (expected 1252 got %d/%d)\n", cpOutC, cpOut);
    }
    else
        win_skip("Setting the codepage is not implemented\n");

    ok(modeInC == (modeIn ^ 1), "Wrong console mode\n");
    ok(modeOutC == (modeOut ^ 1), "Wrong console-SB mode\n");
    trace("cursor position(X): %d/%d\n",sbi.dwCursorPosition.X, sbiC.dwCursorPosition.X);
    ok(sbiC.dwCursorPosition.Y == (sbi.dwCursorPosition.Y ^ 1), "Wrong cursor position\n");

    release_memory();
    DeleteFileA(resfile);

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
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\" stdhandle", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &startup, &info), "CreateProcess\n");
    ok(CloseHandle(hChildInInh), "Closing handle\n");
    ok(CloseHandle(hChildOutInh), "Closing handle\n");

    msg_len = strlen(msg) + 1;
    ok(WriteFile(hParentOut, msg, msg_len, &w, NULL), "Writing to child\n");
    ok(w == msg_len, "Should have written %u bytes, actually wrote %u\n", msg_len, w);
    memset(buffer, 0, sizeof(buffer));
    ok(ReadFile(hParentIn, buffer, sizeof(buffer), &w, NULL), "Reading from child\n");
    ok(strcmp(buffer, msg) == 0, "Should have received '%s'\n", msg);

    /* the child may also send the final "n tests executed" string, so read it to avoid a deadlock */
    ReadFile(hParentIn, buffer, sizeof(buffer), &w, NULL);

    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    okChildString("StdHandle", "msg", msg);

    release_memory();
    DeleteFileA(resfile);
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
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\" exit_code", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info), "CreateProcess\n");

    /* wait for child to terminate */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    /* child process has changed result file, so let profile functions know about it */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    ok(GetExitCodeProcess(info.hProcess, &code), "Getting exit code\n");
    okChildInt("ExitCode", "value", code);

    release_memory();
    DeleteFileA(resfile);
}

static void test_OpenProcess(void)
{
    HANDLE hproc;
    void *addr1;
    MEMORY_BASIC_INFORMATION info;
    SIZE_T dummy, read_bytes;
    BOOL ret;

    /* not exported in all windows versions */
    if ((!pVirtualAllocEx) || (!pVirtualFreeEx)) {
        win_skip("VirtualAllocEx not found\n");
        return;
    }

    /* without PROCESS_VM_OPERATION */
    hproc = OpenProcess(PROCESS_ALL_ACCESS_NT4 & ~PROCESS_VM_OPERATION, FALSE, GetCurrentProcessId());
    ok(hproc != NULL, "OpenProcess error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    addr1 = pVirtualAllocEx(hproc, 0, 0xFFFC, MEM_RESERVE, PAGE_NOACCESS);
    ok(!addr1, "VirtualAllocEx should fail\n");
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {   /* Win9x */
        CloseHandle(hproc);
        win_skip("VirtualAllocEx not implemented\n");
        return;
    }
    ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %d\n", GetLastError());

    read_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadProcessMemory(hproc, test_OpenProcess, &dummy, sizeof(dummy), &read_bytes);
    ok(ret, "ReadProcessMemory error %d\n", GetLastError());
    ok(read_bytes == sizeof(dummy), "wrong read bytes %ld\n", read_bytes);

    CloseHandle(hproc);

    hproc = OpenProcess(PROCESS_VM_OPERATION, FALSE, GetCurrentProcessId());
    ok(hproc != NULL, "OpenProcess error %d\n", GetLastError());

    addr1 = pVirtualAllocEx(hproc, 0, 0xFFFC, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 != NULL, "VirtualAllocEx error %d\n", GetLastError());

    /* without PROCESS_QUERY_INFORMATION */
    SetLastError(0xdeadbeef);
    ok(!VirtualQueryEx(hproc, addr1, &info, sizeof(info)),
       "VirtualQueryEx without PROCESS_QUERY_INFORMATION rights should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %d\n", GetLastError());

    /* without PROCESS_VM_READ */
    read_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ok(!ReadProcessMemory(hproc, addr1, &dummy, sizeof(dummy), &read_bytes),
       "ReadProcessMemory without PROCESS_VM_READ rights should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %d\n", GetLastError());
    ok(read_bytes == 0, "wrong read bytes %ld\n", read_bytes);

    CloseHandle(hproc);

    hproc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());

    memset(&info, 0xcc, sizeof(info));
    read_bytes = VirtualQueryEx(hproc, addr1, &info, sizeof(info));
    ok(read_bytes == sizeof(info), "VirtualQueryEx error %d\n", GetLastError());

    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x10000, "%lx != 0x10000\n", info.RegionSize);
    ok(info.State == MEM_RESERVE, "%x != MEM_RESERVE\n", info.State);
    /* NT reports Protect == 0 for a not committed memory block */
    ok(info.Protect == 0 /* NT */ ||
       info.Protect == PAGE_NOACCESS, /* Win9x */
        "%x != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);

    SetLastError(0xdeadbeef);
    ok(!pVirtualFreeEx(hproc, addr1, 0, MEM_RELEASE),
       "VirtualFreeEx without PROCESS_VM_OPERATION rights should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %d\n", GetLastError());

    CloseHandle(hproc);

    hproc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, GetCurrentProcessId());
    if (hproc)
    {
        SetLastError(0xdeadbeef);
        memset(&info, 0xcc, sizeof(info));
        read_bytes = VirtualQueryEx(hproc, addr1, &info, sizeof(info));
        if (read_bytes) /* win8 */
        {
            ok(read_bytes == sizeof(info), "VirtualQueryEx error %d\n", GetLastError());
            ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
            ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
            ok(info.AllocationProtect == PAGE_NOACCESS, "%x != PAGE_NOACCESS\n", info.AllocationProtect);
            ok(info.RegionSize == 0x10000, "%lx != 0x10000\n", info.RegionSize);
            ok(info.State == MEM_RESERVE, "%x != MEM_RESERVE\n", info.State);
            ok(info.Protect == 0, "%x != PAGE_NOACCESS\n", info.Protect);
            ok(info.Type == MEM_PRIVATE, "%x != MEM_PRIVATE\n", info.Type);
        }
        else /* before win8 */
            ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %d\n", GetLastError());

        SetLastError(0xdeadbeef);
        ok(!pVirtualFreeEx(hproc, addr1, 0, MEM_RELEASE),
           "VirtualFreeEx without PROCESS_VM_OPERATION rights should fail\n");
        ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %d\n", GetLastError());

        CloseHandle(hproc);
    }

    ok(VirtualFree(addr1, 0, MEM_RELEASE), "VirtualFree failed\n");
}

static void test_GetProcessVersion(void)
{
    static char cmdline[] = "winver.exe";
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    DWORD ret;

    SetLastError(0xdeadbeef);
    ret = GetProcessVersion(0);
    ok(ret, "GetProcessVersion error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetProcessVersion(GetCurrentProcessId());
    ok(ret, "GetProcessVersion error %u\n", GetLastError());

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetProcessVersion(pi.dwProcessId);
    ok(ret, "GetProcessVersion error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = TerminateProcess(pi.hProcess, 0);
    ok(ret, "TerminateProcess error %u\n", GetLastError());

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

static void test_GetProcessImageFileNameA(void)
{
    DWORD rc;
    CHAR process[MAX_PATH];
    static const char harddisk[] = "\\Device\\HarddiskVolume";

    if (!pK32GetProcessImageFileNameA)
    {
        win_skip("K32GetProcessImageFileNameA is unavailable\n");
        return;
    }

    /* callers must guess the buffer size */
    SetLastError(0xdeadbeef);
    rc = pK32GetProcessImageFileNameA(GetCurrentProcess(), NULL, 0);
    ok(!rc && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
       "K32GetProcessImageFileNameA(no buffer): returned %u, le=%u\n", rc, GetLastError());

    *process = '\0';
    rc = pK32GetProcessImageFileNameA(GetCurrentProcess(), process, sizeof(process));
    expect_eq_d(rc, lstrlenA(process));
    if (strncmp(process, harddisk, lstrlenA(harddisk)))
    {
        todo_wine win_skip("%s is probably on a network share, skipping tests\n", process);
        return;
    }

    if (!pQueryFullProcessImageNameA)
        win_skip("QueryFullProcessImageNameA unavailable (added in Windows Vista)\n");
    else
    {
        CHAR image[MAX_PATH];
        DWORD length;

        length = sizeof(image);
        expect_eq_d(TRUE, pQueryFullProcessImageNameA(GetCurrentProcess(), PROCESS_NAME_NATIVE, image, &length));
        expect_eq_d(length, lstrlenA(image));
        ok(lstrcmpiA(process, image) == 0, "expected '%s' to be equal to '%s'\n", process, image);
    }
}

static void test_QueryFullProcessImageNameA(void)
{
#define INIT_STR "Just some words"
    DWORD length, size;
    CHAR buf[MAX_PATH], module[MAX_PATH];

    if (!pQueryFullProcessImageNameA)
    {
        win_skip("QueryFullProcessImageNameA unavailable (added in Windows Vista)\n");
        return;
    }

    *module = '\0';
    SetLastError(0); /* old Windows don't reset it on success */
    size = GetModuleFileNameA(NULL, module, sizeof(module));
    ok(size && GetLastError() != ERROR_INSUFFICIENT_BUFFER, "GetModuleFileName failed: %u le=%u\n", size, GetLastError());

    /* get the buffer length without \0 terminator */
    length = sizeof(buf);
    expect_eq_d(TRUE, pQueryFullProcessImageNameA(GetCurrentProcess(), 0, buf, &length));
    expect_eq_d(length, lstrlenA(buf));
    ok((buf[0] == '\\' && buf[1] == '\\') ||
       lstrcmpiA(buf, module) == 0, "expected %s to match %s\n", buf, module);

    /*  when the buffer is too small
     *  - function fail with error ERROR_INSUFFICIENT_BUFFER
     *  - the size variable is not modified
     * tested with the biggest too small size
     */
    size = length;
    sprintf(buf,INIT_STR);
    expect_eq_d(FALSE, pQueryFullProcessImageNameA(GetCurrentProcess(), 0, buf, &size));
    expect_eq_d(ERROR_INSUFFICIENT_BUFFER, GetLastError());
    expect_eq_d(length, size);
    expect_eq_s(INIT_STR, buf);

    /* retest with smaller buffer size
     */
    size = 4;
    sprintf(buf,INIT_STR);
    expect_eq_d(FALSE, pQueryFullProcessImageNameA(GetCurrentProcess(), 0, buf, &size));
    expect_eq_d(ERROR_INSUFFICIENT_BUFFER, GetLastError());
    expect_eq_d(4, size);
    expect_eq_s(INIT_STR, buf);

    /* this is a difference between the ascii and the unicode version
     * the unicode version crashes when the size is big enough to hold
     * the result while the ascii version throws an error
     */
    size = 1024;
    expect_eq_d(FALSE, pQueryFullProcessImageNameA(GetCurrentProcess(), 0, NULL, &size));
    expect_eq_d(1024, size);
    expect_eq_d(ERROR_INVALID_PARAMETER, GetLastError());
}

static void test_QueryFullProcessImageNameW(void)
{
    HANDLE hSelf;
    WCHAR module_name[1024], device[1024];
    WCHAR deviceW[] = {'\\','D', 'e','v','i','c','e',0};
    WCHAR buf[1024];
    DWORD size, len;

    if (!pQueryFullProcessImageNameW)
    {
        win_skip("QueryFullProcessImageNameW unavailable (added in Windows Vista)\n");
        return;
    }

    ok(GetModuleFileNameW(NULL, module_name, 1024), "GetModuleFileNameW(NULL, ...) failed\n");

    /* GetCurrentProcess pseudo-handle */
    size = sizeof(buf) / sizeof(buf[0]);
    expect_eq_d(TRUE, pQueryFullProcessImageNameW(GetCurrentProcess(), 0, buf, &size));
    expect_eq_d(lstrlenW(buf), size);
    expect_eq_ws_i(buf, module_name);

    hSelf = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, GetCurrentProcessId());
    /* Real handle */
    size = sizeof(buf) / sizeof(buf[0]);
    expect_eq_d(TRUE, pQueryFullProcessImageNameW(hSelf, 0, buf, &size));
    expect_eq_d(lstrlenW(buf), size);
    expect_eq_ws_i(buf, module_name);

    /* Buffer too small */
    size = lstrlenW(module_name)/2;
    lstrcpyW(buf, deviceW);
    SetLastError(0xdeadbeef);
    expect_eq_d(FALSE, pQueryFullProcessImageNameW(hSelf, 0, buf, &size));
    expect_eq_d(lstrlenW(module_name)/2, size);  /* size not changed(!) */
    expect_eq_d(ERROR_INSUFFICIENT_BUFFER, GetLastError());
    expect_eq_ws_i(deviceW, buf);  /* buffer not changed */

    /* Too small - not space for NUL terminator */
    size = lstrlenW(module_name);
    SetLastError(0xdeadbeef);
    expect_eq_d(FALSE, pQueryFullProcessImageNameW(hSelf, 0, buf, &size));
    expect_eq_d(lstrlenW(module_name), size);  /* size not changed(!) */
    expect_eq_d(ERROR_INSUFFICIENT_BUFFER, GetLastError());

    /* NULL buffer */
    size = 0;
    expect_eq_d(FALSE, pQueryFullProcessImageNameW(hSelf, 0, NULL, &size));
    expect_eq_d(0, size);
    expect_eq_d(ERROR_INSUFFICIENT_BUFFER, GetLastError());

    /* Buffer too small */
    size = lstrlenW(module_name)/2;
    SetLastError(0xdeadbeef);
    lstrcpyW(buf, module_name);
    expect_eq_d(FALSE, pQueryFullProcessImageNameW(hSelf, 0, buf, &size));
    expect_eq_d(lstrlenW(module_name)/2, size);  /* size not changed(!) */
    expect_eq_d(ERROR_INSUFFICIENT_BUFFER, GetLastError());
    expect_eq_ws_i(module_name, buf);  /* buffer not changed */


    /* native path */
    size = sizeof(buf) / sizeof(buf[0]);
    expect_eq_d(TRUE, pQueryFullProcessImageNameW(hSelf, PROCESS_NAME_NATIVE, buf, &size));
    expect_eq_d(lstrlenW(buf), size);
    ok(buf[0] == '\\', "NT path should begin with '\\'\n");
    ok(memcmp(buf, deviceW, sizeof(WCHAR)*lstrlenW(deviceW)) == 0, "NT path should begin with \\Device\n");

    module_name[2] = '\0';
    *device = '\0';
    size = QueryDosDeviceW(module_name, device, sizeof(device)/sizeof(device[0]));
    ok(size, "QueryDosDeviceW failed: le=%u\n", GetLastError());
    len = lstrlenW(device);
    ok(size >= len+2, "expected %d to be greater than %d+2 = strlen(%s)\n", size, len, wine_dbgstr_w(device));

    if (size >= lstrlenW(buf))
    {
        ok(0, "expected %s\\ to match the start of %s\n", wine_dbgstr_w(device), wine_dbgstr_w(buf));
    }
    else
    {
        ok(buf[len] == '\\', "expected '%c' to be a '\\' in %s\n", buf[len], wine_dbgstr_w(module_name));
        buf[len] = '\0';
        ok(lstrcmpiW(device, buf) == 0, "expected %s to match %s\n", wine_dbgstr_w(device), wine_dbgstr_w(buf));
        ok(lstrcmpiW(module_name+3, buf+len+1) == 0, "expected '%s' to match '%s'\n", wine_dbgstr_w(module_name+3), wine_dbgstr_w(buf+len+1));
    }

    CloseHandle(hSelf);
}

static void test_Handles(void)
{
    HANDLE handle = GetCurrentProcess();
    HANDLE h2, h3;
    BOOL ret;
    DWORD code;

    ok( handle == (HANDLE)~(ULONG_PTR)0 ||
        handle == (HANDLE)(ULONG_PTR)0x7fffffff /* win9x */,
        "invalid current process handle %p\n", handle );
    ret = GetExitCodeProcess( handle, &code );
    ok( ret, "GetExitCodeProcess failed err %u\n", GetLastError() );
#ifdef _WIN64
    /* truncated handle */
    SetLastError( 0xdeadbeef );
    handle = (HANDLE)((ULONG_PTR)handle & ~0u);
    ret = GetExitCodeProcess( handle, &code );
    ok( !ret, "GetExitCodeProcess succeeded for %p\n", handle );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %u\n", GetLastError() );
    /* sign-extended handle */
    SetLastError( 0xdeadbeef );
    handle = (HANDLE)((LONG_PTR)(int)(ULONG_PTR)handle);
    ret = GetExitCodeProcess( handle, &code );
    ok( ret, "GetExitCodeProcess failed err %u\n", GetLastError() );
    /* invalid high-word */
    SetLastError( 0xdeadbeef );
    handle = (HANDLE)(((ULONG_PTR)handle & ~0u) + ((ULONG_PTR)1 << 32));
    ret = GetExitCodeProcess( handle, &code );
    ok( !ret, "GetExitCodeProcess succeeded for %p\n", handle );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %u\n", GetLastError() );
#endif

    handle = GetStdHandle( STD_ERROR_HANDLE );
    ok( handle != 0, "handle %p\n", handle );
    DuplicateHandle( GetCurrentProcess(), handle, GetCurrentProcess(), &h3,
                     0, TRUE, DUPLICATE_SAME_ACCESS );
    SetStdHandle( STD_ERROR_HANDLE, h3 );
    CloseHandle( (HANDLE)STD_ERROR_HANDLE );
    h2 = GetStdHandle( STD_ERROR_HANDLE );
    ok( h2 == 0 ||
        broken( h2 == h3) || /* nt4, w2k */
        broken( h2 == INVALID_HANDLE_VALUE),  /* win9x */
        "wrong handle %p/%p\n", h2, h3 );
    SetStdHandle( STD_ERROR_HANDLE, handle );
}

static void test_IsWow64Process(void)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    DWORD ret;
    BOOL is_wow64;
    static char cmdline[] = "C:\\Program Files\\Internet Explorer\\iexplore.exe";
    static char cmdline_wow64[] = "C:\\Program Files (x86)\\Internet Explorer\\iexplore.exe";

    if (!pIsWow64Process)
    {
        skip("IsWow64Process is not available\n");
        return;
    }

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ret = CreateProcessA(NULL, cmdline_wow64, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (ret)
    {
        trace("Created process %s\n", cmdline_wow64);
        is_wow64 = FALSE;
        ret = pIsWow64Process(pi.hProcess, &is_wow64);
        ok(ret, "IsWow64Process failed.\n");
        ok(is_wow64, "is_wow64 returned FALSE.\n");

        ret = TerminateProcess(pi.hProcess, 0);
        ok(ret, "TerminateProcess error\n");

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    if (ret)
    {
        trace("Created process %s\n", cmdline);
        is_wow64 = TRUE;
        ret = pIsWow64Process(pi.hProcess, &is_wow64);
        ok(ret, "IsWow64Process failed.\n");
        ok(!is_wow64, "is_wow64 returned TRUE.\n");

        ret = TerminateProcess(pi.hProcess, 0);
        ok(ret, "TerminateProcess error\n");

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }
}

static void test_SystemInfo(void)
{
    SYSTEM_INFO si, nsi;
    BOOL is_wow64;

    if (!pGetNativeSystemInfo)
    {
        win_skip("GetNativeSystemInfo is not available\n");
        return;
    }

    if (!pIsWow64Process || !pIsWow64Process( GetCurrentProcess(), &is_wow64 )) is_wow64 = FALSE;

    GetSystemInfo(&si);
    pGetNativeSystemInfo(&nsi);
    if (is_wow64)
    {
        if (S(U(si)).wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        {
            ok(S(U(nsi)).wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64,
               "Expected PROCESSOR_ARCHITECTURE_AMD64, got %d\n",
               S(U(nsi)).wProcessorArchitecture);
            ok(nsi.dwProcessorType == PROCESSOR_AMD_X8664,
               "Expected PROCESSOR_AMD_X8664, got %d\n",
               nsi.dwProcessorType);
        }
    }
    else
    {
        ok(S(U(si)).wProcessorArchitecture == S(U(nsi)).wProcessorArchitecture,
           "Expected no difference for wProcessorArchitecture, got %d and %d\n",
           S(U(si)).wProcessorArchitecture, S(U(nsi)).wProcessorArchitecture);
        ok(si.dwProcessorType == nsi.dwProcessorType,
           "Expected no difference for dwProcessorType, got %d and %d\n",
           si.dwProcessorType, nsi.dwProcessorType);
    }
}

static void test_RegistryQuota(void)
{
    BOOL ret;
    DWORD max_quota, used_quota;

    if (!pGetSystemRegistryQuota)
    {
        win_skip("GetSystemRegistryQuota is not available\n");
        return;
    }

    ret = pGetSystemRegistryQuota(NULL, NULL);
    ok(ret == TRUE,
       "Expected GetSystemRegistryQuota to return TRUE, got %d\n", ret);

    ret = pGetSystemRegistryQuota(&max_quota, NULL);
    ok(ret == TRUE,
       "Expected GetSystemRegistryQuota to return TRUE, got %d\n", ret);

    ret = pGetSystemRegistryQuota(NULL, &used_quota);
    ok(ret == TRUE,
       "Expected GetSystemRegistryQuota to return TRUE, got %d\n", ret);

    ret = pGetSystemRegistryQuota(&max_quota, &used_quota);
    ok(ret == TRUE,
       "Expected GetSystemRegistryQuota to return TRUE, got %d\n", ret);
}

static void test_TerminateProcess(void)
{
    static char cmdline[] = "winver.exe";
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    DWORD ret;
    HANDLE dummy, thread;

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    thread = CreateRemoteThread(pi.hProcess, NULL, 0, (void *)0xdeadbeef, NULL, CREATE_SUSPENDED, &ret);
    ok(thread != 0, "CreateRemoteThread error %d\n", GetLastError());

    /* create a not closed thread handle duplicate in the target process */
    SetLastError(0xdeadbeef);
    ret = DuplicateHandle(GetCurrentProcess(), thread, pi.hProcess, &dummy,
                          0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(ret, "DuplicateHandle error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = TerminateThread(thread, 0);
    ok(ret, "TerminateThread error %u\n", GetLastError());
    CloseHandle(thread);

    SetLastError(0xdeadbeef);
    ret = TerminateProcess(pi.hProcess, 0);
    ok(ret, "TerminateProcess error %u\n", GetLastError());

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

static void test_DuplicateHandle(void)
{
    char path[MAX_PATH], file_name[MAX_PATH];
    HANDLE f, fmin, out;
    DWORD info;
    BOOL r;

    r = DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(),
            GetCurrentProcess(), &out, 0, FALSE,
            DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    ok(r, "DuplicateHandle error %u\n", GetLastError());
    r = GetHandleInformation(out, &info);
    ok(r, "GetHandleInformation error %u\n", GetLastError());
    ok(info == 0, "info = %x\n", info);
    ok(out != GetCurrentProcess(), "out = GetCurrentProcess()\n");
    CloseHandle(out);

    r = DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(),
            GetCurrentProcess(), &out, 0, TRUE,
            DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    ok(r, "DuplicateHandle error %u\n", GetLastError());
    r = GetHandleInformation(out, &info);
    ok(r, "GetHandleInformation error %u\n", GetLastError());
    ok(info == HANDLE_FLAG_INHERIT, "info = %x\n", info);
    ok(out != GetCurrentProcess(), "out = GetCurrentProcess()\n");
    CloseHandle(out);

    GetTempPathA(MAX_PATH, path);
    GetTempFileNameA(path, "wt", 0, file_name);
    f = CreateFileA(file_name, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    if (f == INVALID_HANDLE_VALUE)
    {
        ok(0, "could not create %s\n", file_name);
        return;
    }

    r = DuplicateHandle(GetCurrentProcess(), f, GetCurrentProcess(), &out,
            0, FALSE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    ok(r, "DuplicateHandle error %u\n", GetLastError());
    ok(f == out, "f != out\n");
    r = GetHandleInformation(out, &info);
    ok(r, "GetHandleInformation error %u\n", GetLastError());
    ok(info == 0, "info = %x\n", info);

    r = DuplicateHandle(GetCurrentProcess(), f, GetCurrentProcess(), &out,
            0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    ok(r, "DuplicateHandle error %u\n", GetLastError());
    ok(f == out, "f != out\n");
    r = GetHandleInformation(out, &info);
    ok(r, "GetHandleInformation error %u\n", GetLastError());
    ok(info == HANDLE_FLAG_INHERIT, "info = %x\n", info);

    r = SetHandleInformation(f, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE);
    ok(r, "SetHandleInformation error %u\n", GetLastError());
    r = DuplicateHandle(GetCurrentProcess(), f, GetCurrentProcess(), &out,
                0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    ok(r, "DuplicateHandle error %u\n", GetLastError());
    ok(f != out, "f == out\n");
    r = GetHandleInformation(out, &info);
    ok(r, "GetHandleInformation error %u\n", GetLastError());
    ok(info == HANDLE_FLAG_INHERIT, "info = %x\n", info);
    r = SetHandleInformation(f, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
    ok(r, "SetHandleInformation error %u\n", GetLastError());

    /* Test if DuplicateHandle allocates first free handle */
    if (f > out)
    {
        fmin = out;
    }
    else
    {
        fmin = f;
        f = out;
    }
    CloseHandle(fmin);
    r = DuplicateHandle(GetCurrentProcess(), f, GetCurrentProcess(), &out,
            0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    ok(r, "DuplicateHandle error %u\n", GetLastError());
    ok(f == out, "f != out\n");
    CloseHandle(out);
    DeleteFileA(file_name);

    f = CreateFileA("CONIN$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    if (!is_console(f))
    {
        skip("DuplicateHandle on console handle\n");
        CloseHandle(f);
        return;
    }

    r = DuplicateHandle(GetCurrentProcess(), f, GetCurrentProcess(), &out,
            0, FALSE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    ok(r, "DuplicateHandle error %u\n", GetLastError());
    todo_wine ok(f != out, "f == out\n");
    CloseHandle(out);
}

#define test_completion(a, b, c, d, e) _test_completion(__LINE__, a, b, c, d, e)
static void _test_completion(int line, HANDLE port, DWORD ekey, ULONG_PTR evalue, ULONG_PTR eoverlapped, DWORD wait)
{
    LPOVERLAPPED overlapped;
    ULONG_PTR value;
    DWORD key;
    BOOL ret;

    ret = GetQueuedCompletionStatus(port, &key, &value, &overlapped, wait);

    ok_(__FILE__, line)(ret, "GetQueuedCompletionStatus: %x\n", GetLastError());
    if (ret)
    {
        ok_(__FILE__, line)(key == ekey, "unexpected key %x\n", key);
        ok_(__FILE__, line)(value == evalue, "unexpected value %p\n", (void *)value);
        ok_(__FILE__, line)(overlapped == (LPOVERLAPPED)eoverlapped, "unexpected overlapped %p\n", overlapped);
    }
}

#define create_process(cmd, pi) _create_process(__LINE__, cmd, pi)
static void _create_process(int line, const char *command, LPPROCESS_INFORMATION pi)
{
    BOOL ret;
    char buffer[MAX_PATH];
    STARTUPINFOA si = {0};

    sprintf(buffer, "\"%s\" tests/process.c %s", selfname, command);

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, pi);
    ok_(__FILE__, line)(ret, "CreateProcess error %u\n", GetLastError());
}

#define test_assigned_proc(job, ...) _test_assigned_proc(__LINE__, job, __VA_ARGS__)
static void _test_assigned_proc(int line, HANDLE job, int expected_count, ...)
{
    char buf[sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + sizeof(ULONG_PTR) * 20];
    PJOBOBJECT_BASIC_PROCESS_ID_LIST pid_list = (JOBOBJECT_BASIC_PROCESS_ID_LIST *)buf;
    DWORD ret_len, pid;
    va_list valist;
    int n;
    BOOL ret;

    memset(buf, 0, sizeof(buf));
    ret = pQueryInformationJobObject(job, JobObjectBasicProcessIdList, pid_list, sizeof(buf), &ret_len);
    ok_(__FILE__, line)(ret, "QueryInformationJobObject error %u\n", GetLastError());
    if (ret)
    {
        todo_wine_if(expected_count)
        ok_(__FILE__, line)(expected_count == pid_list->NumberOfAssignedProcesses,
                            "Expected NumberOfAssignedProcesses to be %d (expected_count) is %d\n",
                            expected_count, pid_list->NumberOfAssignedProcesses);
        todo_wine_if(expected_count)
        ok_(__FILE__, line)(expected_count == pid_list->NumberOfProcessIdsInList,
                            "Expected NumberOfProcessIdsInList to be %d (expected_count) is %d\n",
                            expected_count, pid_list->NumberOfProcessIdsInList);

        va_start(valist, expected_count);
        for (n = 0; n < min(expected_count, pid_list->NumberOfProcessIdsInList); ++n)
        {
            pid = va_arg(valist, DWORD);
            ok_(__FILE__, line)(pid == pid_list->ProcessIdList[n],
                                "Expected pid_list->ProcessIdList[%d] to be %x is %lx\n",
                                n, pid, pid_list->ProcessIdList[n]);
        }
        va_end(valist);
    }
}

#define test_accounting(job, total_proc, active_proc, terminated_proc) _test_accounting(__LINE__, job, total_proc, active_proc, terminated_proc)
static void _test_accounting(int line, HANDLE job, int total_proc, int active_proc, int terminated_proc)
{
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION basic_accounting;
    DWORD ret_len;
    BOOL ret;

    memset(&basic_accounting, 0, sizeof(basic_accounting));
    ret = pQueryInformationJobObject(job, JobObjectBasicAccountingInformation, &basic_accounting, sizeof(basic_accounting), &ret_len);
    ok_(__FILE__, line)(ret, "QueryInformationJobObject error %u\n", GetLastError());
    if (ret)
    {
        /* Not going to check process times or page faults */

        todo_wine_if(total_proc)
        ok_(__FILE__, line)(total_proc == basic_accounting.TotalProcesses,
                            "Expected basic_accounting.TotalProcesses to be %d (total_proc) is %d\n",
                            total_proc, basic_accounting.TotalProcesses);
        todo_wine_if(active_proc)
        ok_(__FILE__, line)(active_proc == basic_accounting.ActiveProcesses,
                            "Expected basic_accounting.ActiveProcesses to be %d (active_proc) is %d\n",
                            active_proc, basic_accounting.ActiveProcesses);
        ok_(__FILE__, line)(terminated_proc == basic_accounting.TotalTerminatedProcesses,
                            "Expected basic_accounting.TotalTerminatedProcesses to be %d (terminated_proc) is %d\n",
                            terminated_proc, basic_accounting.TotalTerminatedProcesses);
    }
}

static void test_IsProcessInJob(void)
{
    HANDLE job, job2;
    PROCESS_INFORMATION pi;
    BOOL ret, out;
    DWORD dwret;

    if (!pIsProcessInJob)
    {
        win_skip("IsProcessInJob not available.\n");
        return;
    }

    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %u\n", GetLastError());

    job2 = pCreateJobObjectW(NULL, NULL);
    ok(job2 != NULL, "CreateJobObject error %u\n", GetLastError());

    create_process("wait", &pi);

    out = TRUE;
    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %u\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job, 0);
    test_accounting(job, 0, 0, 0);

    out = TRUE;
    ret = pIsProcessInJob(pi.hProcess, job2, &out);
    ok(ret, "IsProcessInJob error %u\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job2, 0);
    test_accounting(job2, 0, 0, 0);

    out = TRUE;
    ret = pIsProcessInJob(pi.hProcess, NULL, &out);
    ok(ret, "IsProcessInJob error %u\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %u\n", GetLastError());

    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %u\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job, 1, pi.dwProcessId);
    test_accounting(job, 1, 1, 0);

    out = TRUE;
    ret = pIsProcessInJob(pi.hProcess, job2, &out);
    ok(ret, "IsProcessInJob error %u\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job2, 0);
    test_accounting(job2, 0, 0, 0);

    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, NULL, &out);
    ok(ret, "IsProcessInJob error %u\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    TerminateProcess(pi.hProcess, 0);

    dwret = WaitForSingleObject(pi.hProcess, 1000);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", dwret);

    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %u\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job, 0);
    test_accounting(job, 1, 0, 0);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(job);
    CloseHandle(job2);
}

static void test_TerminateJobObject(void)
{
    HANDLE job;
    PROCESS_INFORMATION pi;
    BOOL ret;
    DWORD dwret;

    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %u\n", GetLastError());
    test_assigned_proc(job, 0);
    test_accounting(job, 0, 0, 0);

    create_process("wait", &pi);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %u\n", GetLastError());
    test_assigned_proc(job, 1, pi.dwProcessId);
    test_accounting(job, 1, 1, 0);

    ret = pTerminateJobObject(job, 123);
    ok(ret, "TerminateJobObject error %u\n", GetLastError());

    dwret = WaitForSingleObject(pi.hProcess, 1000);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", dwret);
    if (dwret == WAIT_TIMEOUT) TerminateProcess(pi.hProcess, 0);
    test_assigned_proc(job, 0);
    test_accounting(job, 1, 0, 0);

    ret = GetExitCodeProcess(pi.hProcess, &dwret);
    ok(ret, "GetExitCodeProcess error %u\n", GetLastError());
    ok(dwret == 123 || broken(dwret == 0) || broken(dwret == 259) /* randomly fails on Win 2000 / XP */,
       "wrong exitcode %u\n", dwret);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    /* Test adding an already terminated process to a job object */
    create_process("exit", &pi);

    dwret = WaitForSingleObject(pi.hProcess, 1000);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", dwret);

    SetLastError(0xdeadbeef);
    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(!ret, "AssignProcessToJobObject unexpectedly succeeded\n");
    expect_eq_d(ERROR_ACCESS_DENIED, GetLastError());
    test_assigned_proc(job, 0);
    test_accounting(job, 1, 0, 0);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    CloseHandle(job);
}

static void test_QueryInformationJobObject(void)
{
    char buf[sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + sizeof(ULONG_PTR) * 4];
    PJOBOBJECT_BASIC_PROCESS_ID_LIST pid_list = (JOBOBJECT_BASIC_PROCESS_ID_LIST *)buf;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION ext_limit_info;
    JOBOBJECT_BASIC_LIMIT_INFORMATION *basic_limit_info = &ext_limit_info.BasicLimitInformation;
    DWORD dwret, ret_len;
    PROCESS_INFORMATION pi[2];
    HANDLE job;
    BOOL ret;

    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %u\n", GetLastError());

    /* Only active processes are returned */
    create_process("exit", &pi[0]);
    ret = pAssignProcessToJobObject(job, pi[0].hProcess);
    ok(ret, "AssignProcessToJobObject error %u\n", GetLastError());
    dwret = WaitForSingleObject(pi[0].hProcess, 1000);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", dwret);

    CloseHandle(pi[0].hProcess);
    CloseHandle(pi[0].hThread);

    create_process("wait", &pi[0]);
    ret = pAssignProcessToJobObject(job, pi[0].hProcess);
    ok(ret, "AssignProcessToJobObject error %u\n", GetLastError());

    create_process("wait", &pi[1]);
    ret = pAssignProcessToJobObject(job, pi[1].hProcess);
    ok(ret, "AssignProcessToJobObject error %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = QueryInformationJobObject(job, JobObjectBasicProcessIdList, pid_list,
                                    FIELD_OFFSET(JOBOBJECT_BASIC_PROCESS_ID_LIST, ProcessIdList), &ret_len);
    ok(!ret, "QueryInformationJobObject expected failure\n");
    expect_eq_d(ERROR_BAD_LENGTH, GetLastError());

    SetLastError(0xdeadbeef);
    memset(buf, 0, sizeof(buf));
    pid_list->NumberOfAssignedProcesses = 42;
    pid_list->NumberOfProcessIdsInList  = 42;
    ret = QueryInformationJobObject(job, JobObjectBasicProcessIdList, pid_list,
                                    FIELD_OFFSET(JOBOBJECT_BASIC_PROCESS_ID_LIST, ProcessIdList[1]), &ret_len);
    todo_wine
    ok(!ret, "QueryInformationJobObject expected failure\n");
    todo_wine
    expect_eq_d(ERROR_MORE_DATA, GetLastError());
    if (ret)
    {
        todo_wine
        expect_eq_d(42, pid_list->NumberOfAssignedProcesses);
        todo_wine
        expect_eq_d(42, pid_list->NumberOfProcessIdsInList);
    }

    memset(buf, 0, sizeof(buf));
    ret = pQueryInformationJobObject(job, JobObjectBasicProcessIdList, pid_list, sizeof(buf), &ret_len);
    ok(ret, "QueryInformationJobObject error %u\n", GetLastError());
    if(ret)
    {
        if (pid_list->NumberOfAssignedProcesses == 3) /* Win 8 */
            win_skip("Number of assigned processes broken on Win 8\n");
        else
        {
            ULONG_PTR *list = pid_list->ProcessIdList;

            todo_wine
            ok(ret_len == FIELD_OFFSET(JOBOBJECT_BASIC_PROCESS_ID_LIST, ProcessIdList[2]),
               "QueryInformationJobObject returned ret_len=%u\n", ret_len);

            todo_wine
            expect_eq_d(2, pid_list->NumberOfAssignedProcesses);
            todo_wine
            expect_eq_d(2, pid_list->NumberOfProcessIdsInList);
            todo_wine
            expect_eq_d(pi[0].dwProcessId, list[0]);
            todo_wine
            expect_eq_d(pi[1].dwProcessId, list[1]);
        }
    }

    /* test JobObjectBasicLimitInformation */
    ret = pQueryInformationJobObject(job, JobObjectBasicLimitInformation, basic_limit_info,
                                     sizeof(*basic_limit_info) - 1, &ret_len);
    ok(!ret, "QueryInformationJobObject expected failure\n");
    expect_eq_d(ERROR_BAD_LENGTH, GetLastError());

    ret_len = 0xdeadbeef;
    memset(basic_limit_info, 0x11, sizeof(*basic_limit_info));
    ret = pQueryInformationJobObject(job, JobObjectBasicLimitInformation, basic_limit_info,
                                     sizeof(*basic_limit_info), &ret_len);
    ok(ret, "QueryInformationJobObject error %u\n", GetLastError());
    ok(ret_len == sizeof(*basic_limit_info), "QueryInformationJobObject returned ret_len=%u\n", ret_len);
    expect_eq_d(0, basic_limit_info->LimitFlags);

    /* test JobObjectExtendedLimitInformation */
    ret = pQueryInformationJobObject(job, JobObjectExtendedLimitInformation, &ext_limit_info,
                                     sizeof(ext_limit_info) - 1, &ret_len);
    ok(!ret, "QueryInformationJobObject expected failure\n");
    expect_eq_d(ERROR_BAD_LENGTH, GetLastError());

    ret_len = 0xdeadbeef;
    memset(&ext_limit_info, 0x11, sizeof(ext_limit_info));
    ret = pQueryInformationJobObject(job, JobObjectExtendedLimitInformation, &ext_limit_info,
                                     sizeof(ext_limit_info), &ret_len);
    ok(ret, "QueryInformationJobObject error %u\n", GetLastError());
    ok(ret_len == sizeof(ext_limit_info), "QueryInformationJobObject returned ret_len=%u\n", ret_len);
    expect_eq_d(0, basic_limit_info->LimitFlags);

    TerminateProcess(pi[0].hProcess, 0);
    CloseHandle(pi[0].hProcess);
    CloseHandle(pi[0].hThread);

    TerminateProcess(pi[1].hProcess, 0);
    CloseHandle(pi[1].hProcess);
    CloseHandle(pi[1].hThread);

    CloseHandle(job);
}

static void test_CompletionPort(void)
{
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT port_info;
    PROCESS_INFORMATION pi;
    HANDLE job, port;
    DWORD dwret;
    BOOL ret;

    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %u\n", GetLastError());

    port = pCreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    ok(port != NULL, "CreateIoCompletionPort error %u\n", GetLastError());

    port_info.CompletionKey = job;
    port_info.CompletionPort = port;
    ret = pSetInformationJobObject(job, JobObjectAssociateCompletionPortInformation, &port_info, sizeof(port_info));
    ok(ret, "SetInformationJobObject error %u\n", GetLastError());

    create_process("wait", &pi);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %u\n", GetLastError());

    test_completion(port, JOB_OBJECT_MSG_NEW_PROCESS, (DWORD_PTR)job, pi.dwProcessId, 0);

    TerminateProcess(pi.hProcess, 0);
    dwret = WaitForSingleObject(pi.hProcess, 1000);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", dwret);

    test_completion(port, JOB_OBJECT_MSG_EXIT_PROCESS, (DWORD_PTR)job, pi.dwProcessId, 0);
    test_completion(port, JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO, (DWORD_PTR)job, 0, 100);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(job);
    CloseHandle(port);
}

static void test_KillOnJobClose(void)
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit_info;
    PROCESS_INFORMATION pi;
    DWORD dwret;
    HANDLE job;
    BOOL ret;

    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %u\n", GetLastError());

    limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    ret = pSetInformationJobObject(job, JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info));
    if (!ret && GetLastError() == ERROR_INVALID_PARAMETER)
    {
        win_skip("Kill on job close limit not available\n");
        return;
    }
    ok(ret, "SetInformationJobObject error %u\n", GetLastError());
    test_assigned_proc(job, 0);
    test_accounting(job, 0, 0, 0);

    create_process("wait", &pi);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %u\n", GetLastError());
    test_assigned_proc(job, 1, pi.dwProcessId);
    test_accounting(job, 1, 1, 0);

    CloseHandle(job);

    dwret = WaitForSingleObject(pi.hProcess, 1000);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", dwret);
    if (dwret == WAIT_TIMEOUT) TerminateProcess(pi.hProcess, 0);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

static void test_WaitForJobObject(void)
{
    HANDLE job;
    PROCESS_INFORMATION pi;
    BOOL ret;
    DWORD dwret;

    /* test waiting for a job object when the process is killed */
    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %u\n", GetLastError());

    dwret = WaitForSingleObject(job, 100);
    ok(dwret == WAIT_TIMEOUT, "WaitForSingleObject returned %u\n", dwret);

    create_process("wait", &pi);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %u\n", GetLastError());

    dwret = WaitForSingleObject(job, 100);
    ok(dwret == WAIT_TIMEOUT, "WaitForSingleObject returned %u\n", dwret);

    ret = pTerminateJobObject(job, 123);
    ok(ret, "TerminateJobObject error %u\n", GetLastError());

    dwret = WaitForSingleObject(job, 500);
    ok(dwret == WAIT_OBJECT_0 || broken(dwret == WAIT_TIMEOUT),
       "WaitForSingleObject returned %u\n", dwret);

    if (dwret == WAIT_TIMEOUT) /* Win 2000/XP */
    {
#ifdef __REACTOS__
        if (!ret)
        {
            ok(0, "HACK: Killing process to speed up the test\n");
            TerminateProcess(pi.hProcess, 0);
        }
#endif
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(job);
        win_skip("TerminateJobObject doesn't signal job, skipping tests\n");
        return;
    }

    /* the object is not reset immediately */
    dwret = WaitForSingleObject(job, 100);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", dwret);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    /* creating a new process doesn't reset the signalled state */
    create_process("wait", &pi);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %u\n", GetLastError());

    dwret = WaitForSingleObject(job, 100);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", dwret);

    ret = pTerminateJobObject(job, 123);
    ok(ret, "TerminateJobObject error %u\n", GetLastError());

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    CloseHandle(job);

    /* repeat the test, but this time the process terminates properly */
    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %u\n", GetLastError());

    dwret = WaitForSingleObject(job, 100);
    ok(dwret == WAIT_TIMEOUT, "WaitForSingleObject returned %u\n", dwret);

    create_process("exit", &pi);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %u\n", GetLastError());

    dwret = WaitForSingleObject(job, 100);
    ok(dwret == WAIT_TIMEOUT, "WaitForSingleObject returned %u\n", dwret);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(job);
}

static HANDLE test_AddSelfToJob(void)
{
    HANDLE job;
    BOOL ret;

    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %u\n", GetLastError());

    ret = pAssignProcessToJobObject(job, GetCurrentProcess());
    ok(ret, "AssignProcessToJobObject error %u\n", GetLastError());
    test_assigned_proc(job, 1, GetCurrentProcessId());
    test_accounting(job, 1, 1, 0);

    return job;
}

static void test_jobInheritance(HANDLE job)
{
    char buffer[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {0};
    DWORD dwret;
    BOOL ret, out;

    if (!pIsProcessInJob)
    {
        win_skip("IsProcessInJob not available.\n");
        return;
    }

    sprintf(buffer, "\"%s\" tests/process.c %s", selfname, "exit");

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcessA error %u\n", GetLastError());

    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %u\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job, 2, GetCurrentProcessId(), pi.dwProcessId);
    test_accounting(job, 2, 2, 0);

    dwret = WaitForSingleObject(pi.hProcess, 1000);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", dwret);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

static void test_BreakawayOk(HANDLE job)
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit_info;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {0};
    char buffer[MAX_PATH];
    BOOL ret, out;
    DWORD dwret;

    if (!pIsProcessInJob)
    {
        win_skip("IsProcessInJob not available.\n");
        return;
    }

    sprintf(buffer, "\"%s\" tests/process.c %s", selfname, "exit");

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);
    ok(!ret, "CreateProcessA expected failure\n");
    expect_eq_d(ERROR_ACCESS_DENIED, GetLastError());
    test_assigned_proc(job, 1, GetCurrentProcessId());
    test_accounting(job, 2, 1, 0);

    if (ret)
    {
        TerminateProcess(pi.hProcess, 0);

        dwret = WaitForSingleObject(pi.hProcess, 1000);
        ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", dwret);

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK;
    ret = pSetInformationJobObject(job, JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info));
    ok(ret, "SetInformationJobObject error %u\n", GetLastError());

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcessA error %u\n", GetLastError());

    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %u\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job, 1, GetCurrentProcessId());
    test_accounting(job, 2, 1, 0);

    dwret = WaitForSingleObject(pi.hProcess, 1000);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", dwret);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
    ret = pSetInformationJobObject(job, JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info));
    ok(ret, "SetInformationJobObject error %u\n", GetLastError());

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess error %u\n", GetLastError());

    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %u\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job, 1, GetCurrentProcessId());
    test_accounting(job, 2, 1, 0);

    dwret = WaitForSingleObject(pi.hProcess, 1000);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", dwret);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    /* unset breakaway ok */
    limit_info.BasicLimitInformation.LimitFlags = 0;
    ret = pSetInformationJobObject(job, JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info));
    ok(ret, "SetInformationJobObject error %u\n", GetLastError());
}

static void test_StartupNoConsole(void)
{
#ifndef _WIN64
    char                buffer[MAX_PATH];
    STARTUPINFOA        startup;
    PROCESS_INFORMATION info;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &startup,
                      &info), "CreateProcess\n");
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    okChildInt("StartupInfoA", "hStdInput", (UINT)INVALID_HANDLE_VALUE);
    okChildInt("StartupInfoA", "hStdOutput", (UINT)INVALID_HANDLE_VALUE);
    okChildInt("StartupInfoA", "hStdError", (UINT)INVALID_HANDLE_VALUE);
    okChildInt("TEB", "hStdInput", 0);
    okChildInt("TEB", "hStdOutput", 0);
    okChildInt("TEB", "hStdError", 0);
    release_memory();
    DeleteFileA(resfile);
#endif
}

static void test_DetachConsoleHandles(void)
{
#ifndef _WIN64
    char                buffer[MAX_PATH];
    STARTUPINFOA        startup;
    PROCESS_INFORMATION info;
    UINT                result;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW|STARTF_USESTDHANDLES;
    startup.wShowWindow = SW_SHOWNORMAL;
    startup.hStdInput = GetStdHandle(STD_INPUT_HANDLE);
    startup.hStdOutput = GetStdHandle(STD_OUTPUT_HANDLE);
    startup.hStdError = GetStdHandle(STD_ERROR_HANDLE);
    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &startup,
                      &info), "CreateProcess\n");
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);

    result = GetPrivateProfileIntA("StartupInfoA", "hStdInput", 0, resfile);
    ok(result != 0 && result != (UINT)INVALID_HANDLE_VALUE, "bad handle %x\n", result);
    result = GetPrivateProfileIntA("StartupInfoA", "hStdOutput", 0, resfile);
    ok(result != 0 && result != (UINT)INVALID_HANDLE_VALUE, "bad handle %x\n", result);
    result = GetPrivateProfileIntA("StartupInfoA", "hStdError", 0, resfile);
    ok(result != 0 && result != (UINT)INVALID_HANDLE_VALUE, "bad handle %x\n", result);
    result = GetPrivateProfileIntA("TEB", "hStdInput", 0, resfile);
    ok(result != 0 && result != (UINT)INVALID_HANDLE_VALUE, "bad handle %x\n", result);
    result = GetPrivateProfileIntA("TEB", "hStdOutput", 0, resfile);
    ok(result != 0 && result != (UINT)INVALID_HANDLE_VALUE, "bad handle %x\n", result);
    result = GetPrivateProfileIntA("TEB", "hStdError", 0, resfile);
    ok(result != 0 && result != (UINT)INVALID_HANDLE_VALUE, "bad handle %x\n", result);

    release_memory();
    DeleteFileA(resfile);
#endif
}

#if defined(__i386__) || defined(__x86_64__)
static BOOL read_nt_header(HANDLE process_handle, MEMORY_BASIC_INFORMATION *mbi,
                           IMAGE_NT_HEADERS *nt_header)
{
    IMAGE_DOS_HEADER dos_header;

    if (!ReadProcessMemory(process_handle, mbi->BaseAddress, &dos_header, sizeof(dos_header), NULL))
        return FALSE;

    if ((dos_header.e_magic != IMAGE_DOS_SIGNATURE) ||
        ((ULONG)dos_header.e_lfanew > mbi->RegionSize) ||
        (dos_header.e_lfanew < sizeof(dos_header)))
        return FALSE;

    if (!ReadProcessMemory(process_handle, (char *)mbi->BaseAddress + dos_header.e_lfanew,
                           nt_header, sizeof(*nt_header), NULL))
        return FALSE;

    return (nt_header->Signature == IMAGE_NT_SIGNATURE);
}

static PVOID get_process_exe(HANDLE process_handle, IMAGE_NT_HEADERS *nt_header)
{
    PVOID exe_base, address;
    MEMORY_BASIC_INFORMATION mbi;

    /* Find the EXE base in the new process */
    exe_base = NULL;
    for (address = NULL ;
         VirtualQueryEx(process_handle, address, &mbi, sizeof(mbi)) ;
         address = (char *)mbi.BaseAddress + mbi.RegionSize) {
        if ((mbi.Type == SEC_IMAGE) &&
            read_nt_header(process_handle, &mbi, nt_header) &&
            !(nt_header->FileHeader.Characteristics & IMAGE_FILE_DLL)) {
            exe_base = mbi.BaseAddress;
            break;
        }
    }

    return exe_base;
}

static BOOL are_imports_resolved(HANDLE process_handle, PVOID module_base, IMAGE_NT_HEADERS *nt_header)
{
    BOOL ret;
    IMAGE_IMPORT_DESCRIPTOR iid;
    ULONG_PTR orig_iat_entry_value, iat_entry_value;

    ok(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress, "Import table VA is zero\n");
    ok(nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size, "Import table Size is zero\n");

    if (!nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress ||
        !nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].Size)
        return FALSE;

    /* Read the first IID */
    ret = ReadProcessMemory(process_handle,
                            (char *)module_base + nt_header->OptionalHeader.DataDirectory[IMAGE_DIRECTORY_ENTRY_IMPORT].VirtualAddress,
                            &iid, sizeof(iid), NULL);
    ok(ret, "Failed to read remote module IID (%d)\n", GetLastError());

    /* Validate the IID is present and not a bound import, and that we have
       an OriginalFirstThunk to compare with */
    ok(iid.Name, "Module first IID does not have a Name\n");
    ok(iid.FirstThunk, "Module first IID does not have a FirstThunk\n");
    ok(!iid.TimeDateStamp, "Module first IID is a bound import (UNSUPPORTED for current test)\n");
    ok(iid.OriginalFirstThunk, "Module first IID does not have an OriginalFirstThunk (UNSUPPORTED for current test)\n");

    /* Read a single IAT entry from the FirstThunk */
    ret = ReadProcessMemory(process_handle, (char *)module_base + iid.FirstThunk,
                            &iat_entry_value, sizeof(iat_entry_value), NULL);
    ok(ret, "Failed to read IAT entry from FirstThunk (%d)\n", GetLastError());
    ok(iat_entry_value, "IAT entry in FirstThunk is NULL\n");

    /* Read a single IAT entry from the OriginalFirstThunk */
    ret = ReadProcessMemory(process_handle, (char *)module_base + iid.OriginalFirstThunk,
                            &orig_iat_entry_value, sizeof(orig_iat_entry_value), NULL);
    ok(ret, "Failed to read IAT entry from OriginalFirstThunk (%d)\n", GetLastError());
    ok(orig_iat_entry_value, "IAT entry in OriginalFirstThunk is NULL\n");

    return iat_entry_value != orig_iat_entry_value;
}

static void test_SuspendProcessNewThread(void)
{
    BOOL ret;
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    PVOID exe_base, exit_thread_ptr;
    IMAGE_NT_HEADERS nt_header;
    HANDLE thread_handle = NULL;
    DWORD dret, exit_code = 0;
    CONTEXT ctx;

    exit_thread_ptr = GetProcAddress(hkernel32, "ExitThread");
    ok(exit_thread_ptr != NULL, "GetProcAddress ExitThread failed\n");

    si.cb = sizeof(si);
    ret = CreateProcessA(NULL, selfname, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    ok(ret, "Failed to create process (%d)\n", GetLastError());

    exe_base = get_process_exe(pi.hProcess, &nt_header);
    ok(exe_base != NULL, "Could not find EXE in remote process\n");

    ret = are_imports_resolved(pi.hProcess, exe_base, &nt_header);
    ok(!ret, "IAT entry resolved prematurely\n");

    thread_handle = CreateRemoteThread(pi.hProcess, NULL, 0,
                                       (LPTHREAD_START_ROUTINE)exit_thread_ptr,
                                       (PVOID)(ULONG_PTR)0x1234, CREATE_SUSPENDED, NULL);
    ok(thread_handle != NULL, "Could not create remote thread (%d)\n", GetLastError());

    ret = are_imports_resolved(pi.hProcess, exe_base, &nt_header);
    ok(!ret, "IAT entry resolved prematurely\n");

    ctx.ContextFlags = CONTEXT_ALL;
    ret = GetThreadContext( thread_handle, &ctx );
    ok( ret, "Failed retrieving remote thread context (%d)\n", GetLastError() );
    ok( ctx.ContextFlags == CONTEXT_ALL, "wrong flags %x\n", ctx.ContextFlags );
#ifdef __x86_64__
    ok( !ctx.Rax, "rax is not zero %lx\n", ctx.Rax );
    ok( !ctx.Rbx, "rbx is not zero %lx\n", ctx.Rbx );
    ok( ctx.Rcx == (ULONG_PTR)exit_thread_ptr, "wrong rcx %lx/%p\n", ctx.Rcx, exit_thread_ptr );
    ok( ctx.Rdx == 0x1234, "wrong rdx %lx\n", ctx.Rdx );
    ok( !ctx.Rsi, "rsi is not zero %lx\n", ctx.Rsi );
    ok( !ctx.Rdi, "rdi is not zero %lx\n", ctx.Rdi );
    ok( !ctx.Rbp, "rbp is not zero %lx\n", ctx.Rbp );
    ok( !ctx.R8, "r8 is not zero %lx\n", ctx.R8 );
    ok( !ctx.R9, "r9 is not zero %lx\n", ctx.R9 );
    ok( !ctx.R10, "r10 is not zero %lx\n", ctx.R10 );
    ok( !ctx.R11, "r11 is not zero %lx\n", ctx.R11 );
    ok( !ctx.R12, "r12 is not zero %lx\n", ctx.R12 );
    ok( !ctx.R13, "r13 is not zero %lx\n", ctx.R13 );
    ok( !ctx.R14, "r14 is not zero %lx\n", ctx.R14 );
    ok( !ctx.R15, "r15 is not zero %lx\n", ctx.R15 );
    ok( !((ctx.Rsp + 0x28) & 0xfff), "rsp is not at top of stack page %lx\n", ctx.Rsp );
    ok( ctx.EFlags == 0x200, "wrong flags %08x\n", ctx.EFlags );
    ok( ctx.MxCsr == 0x1f80, "wrong mxcsr %08x\n", ctx.MxCsr );
    ok( ctx.FltSave.ControlWord == 0x27f, "wrong control %08x\n", ctx.FltSave.ControlWord );
#else
    ok( !ctx.Ebp || broken(ctx.Ebp), /* winxp */ "ebp is not zero %08x\n", ctx.Ebp );
    if (!ctx.Ebp)  /* winxp is completely different */
    {
        ok( !ctx.Ecx, "ecx is not zero %08x\n", ctx.Ecx );
        ok( !ctx.Edx, "edx is not zero %08x\n", ctx.Edx );
        ok( !ctx.Esi, "esi is not zero %08x\n", ctx.Esi );
        ok( !ctx.Edi, "edi is not zero %08x\n", ctx.Edi );
    }
    ok( ctx.Eax == (ULONG_PTR)exit_thread_ptr, "wrong eax %08x/%p\n", ctx.Eax, exit_thread_ptr );
    ok( ctx.Ebx == 0x1234, "wrong ebx %08x\n", ctx.Ebx );
    ok( !((ctx.Esp + 0x10) & 0xfff) || broken( !((ctx.Esp + 4) & 0xfff) ), /* winxp, w2k3 */
        "esp is not at top of stack page or properly aligned: %08x\n", ctx.Esp );
    ok( (ctx.EFlags & ~2) == 0x200, "wrong flags %08x\n", ctx.EFlags );
    ok( (WORD)ctx.FloatSave.ControlWord == 0x27f, "wrong control %08x\n", ctx.FloatSave.ControlWord );
    ok( *(WORD *)ctx.ExtendedRegisters == 0x27f, "wrong control %08x\n", *(WORD *)ctx.ExtendedRegisters );
#endif

    ResumeThread( thread_handle );
    dret = WaitForSingleObject(thread_handle, 60000);
    ok(dret == WAIT_OBJECT_0, "Waiting for remote thread failed (%d)\n", GetLastError());
    ret = GetExitCodeThread(thread_handle, &exit_code);
    ok(ret, "Failed to retrieve remote thread exit code (%d)\n", GetLastError());
    ok(exit_code == 0x1234, "Invalid remote thread exit code\n");

    ret = are_imports_resolved(pi.hProcess, exe_base, &nt_header);
    ok(ret, "EXE IAT entry not resolved\n");

    if (thread_handle)
        CloseHandle(thread_handle);

    TerminateProcess(pi.hProcess, 0);
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

static void test_SuspendProcessState(void)
{
    struct pipe_params
    {
        ULONG pipe_write_buf;
        ULONG pipe_read_buf;
        ULONG bytes_returned;
        CHAR pipe_name[MAX_PATH];
    };

#ifdef __x86_64__
    struct remote_rop_chain
    {
        void     *exit_process_ptr;
        ULONG_PTR home_rcx;
        ULONG_PTR home_rdx;
        ULONG_PTR home_r8;
        ULONG_PTR home_r9;
        ULONG_PTR pipe_read_buf_size;
        ULONG_PTR bytes_returned;
        ULONG_PTR timeout;
    };
#else
    struct remote_rop_chain
    {
        void     *exit_process_ptr;
        ULONG_PTR pipe_name;
        ULONG_PTR pipe_write_buf;
        ULONG_PTR pipe_write_buf_size;
        ULONG_PTR pipe_read_buf;
        ULONG_PTR pipe_read_buf_size;
        ULONG_PTR bytes_returned;
        ULONG_PTR timeout;
        void     *unreached_ret;
        ULONG_PTR exit_code;
    };
#endif

    static const char pipe_name[] = "\\\\.\\pipe\\TestPipe";
    static const ULONG pipe_write_magic = 0x454e4957;
    STARTUPINFOA si = {0};
    PROCESS_INFORMATION pi = {0};
    PVOID exe_base, remote_pipe_params, exit_process_ptr,
          call_named_pipe_a;
    IMAGE_NT_HEADERS nt_header;
    struct pipe_params pipe_params;
    struct remote_rop_chain rop_chain;
    CONTEXT ctx;
    HANDLE server_pipe_handle;
    BOOL pipe_connected;
    ULONG pipe_magic, numb;
    BOOL ret;
    void *entry_ptr, *peb_ptr;
    PEB child_peb;

    exit_process_ptr = GetProcAddress(hkernel32, "ExitProcess");
    ok(exit_process_ptr != NULL, "GetProcAddress ExitProcess failed\n");

    call_named_pipe_a = GetProcAddress(hkernel32, "CallNamedPipeA");
    ok(call_named_pipe_a != NULL, "GetProcAddress CallNamedPipeA failed\n");

    si.cb = sizeof(si);
    ret = CreateProcessA(NULL, selfname, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    ok(ret, "Failed to create process (%d)\n", GetLastError());

    exe_base = get_process_exe(pi.hProcess, &nt_header);
    /* Make sure we found the EXE in the new process */
    ok(exe_base != NULL, "Could not find EXE in remote process\n");

    ret = are_imports_resolved(pi.hProcess, exe_base, &nt_header);
    ok(!ret, "IAT entry resolved prematurely\n");

    server_pipe_handle = CreateNamedPipeA(pipe_name, PIPE_ACCESS_DUPLEX | FILE_FLAG_WRITE_THROUGH,
                                        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 0x20000, 0x20000,
                                        0, NULL);
    ok(server_pipe_handle != INVALID_HANDLE_VALUE, "Failed to create communication pipe (%d)\n", GetLastError());

    /* Set up the remote process environment */
    ctx.ContextFlags = CONTEXT_ALL;
    ret = GetThreadContext(pi.hThread, &ctx);
    ok(ret, "Failed retrieving remote thread context (%d)\n", GetLastError());
    ok( ctx.ContextFlags == CONTEXT_ALL, "wrong flags %x\n", ctx.ContextFlags );

    remote_pipe_params = VirtualAllocEx(pi.hProcess, NULL, sizeof(pipe_params), MEM_COMMIT, PAGE_READWRITE);
    ok(remote_pipe_params != NULL, "Failed allocating memory in remote process (%d)\n", GetLastError());

    pipe_params.pipe_write_buf = pipe_write_magic;
    pipe_params.pipe_read_buf = 0;
    pipe_params.bytes_returned = 0;
    strcpy(pipe_params.pipe_name, pipe_name);

    ret = WriteProcessMemory(pi.hProcess, remote_pipe_params,
                             &pipe_params, sizeof(pipe_params), NULL);
    ok(ret, "Failed to write to remote process memory (%d)\n", GetLastError());

#ifdef __x86_64__
    ok( !ctx.Rax, "rax is not zero %lx\n", ctx.Rax );
    ok( !ctx.Rbx, "rbx is not zero %lx\n", ctx.Rbx );
    ok( !ctx.Rsi, "rsi is not zero %lx\n", ctx.Rsi );
    ok( !ctx.Rdi, "rdi is not zero %lx\n", ctx.Rdi );
    ok( !ctx.Rbp, "rbp is not zero %lx\n", ctx.Rbp );
    ok( !ctx.R8, "r8 is not zero %lx\n", ctx.R8 );
    ok( !ctx.R9, "r9 is not zero %lx\n", ctx.R9 );
    ok( !ctx.R10, "r10 is not zero %lx\n", ctx.R10 );
    ok( !ctx.R11, "r11 is not zero %lx\n", ctx.R11 );
    ok( !ctx.R12, "r12 is not zero %lx\n", ctx.R12 );
    ok( !ctx.R13, "r13 is not zero %lx\n", ctx.R13 );
    ok( !ctx.R14, "r14 is not zero %lx\n", ctx.R14 );
    ok( !ctx.R15, "r15 is not zero %lx\n", ctx.R15 );
    ok( !((ctx.Rsp + 0x28) & 0xfff), "rsp is not at top of stack page %lx\n", ctx.Rsp );
    ok( ctx.EFlags == 0x200, "wrong flags %08x\n", ctx.EFlags );
    ok( ctx.MxCsr == 0x1f80, "wrong mxcsr %08x\n", ctx.MxCsr );
    ok( ctx.FltSave.ControlWord == 0x27f, "wrong control %08x\n", ctx.FltSave.ControlWord );
    entry_ptr = (void *)ctx.Rcx;
    peb_ptr = (void *)ctx.Rdx;

    rop_chain.exit_process_ptr = exit_process_ptr;
    ctx.Rcx = (ULONG_PTR)remote_pipe_params + offsetof(struct pipe_params, pipe_name);
    ctx.Rdx = (ULONG_PTR)remote_pipe_params + offsetof(struct pipe_params, pipe_write_buf);
    ctx.R8 = sizeof(pipe_params.pipe_write_buf);
    ctx.R9 = (ULONG_PTR)remote_pipe_params + offsetof(struct pipe_params, pipe_read_buf);
    rop_chain.pipe_read_buf_size = sizeof(pipe_params.pipe_read_buf);
    rop_chain.bytes_returned = (ULONG_PTR)remote_pipe_params + offsetof(struct pipe_params, bytes_returned);
    rop_chain.timeout = 10000;

    ctx.Rip = (ULONG_PTR)call_named_pipe_a;
    ctx.Rsp -= sizeof(rop_chain);
    ret = WriteProcessMemory(pi.hProcess, (void *)ctx.Rsp, &rop_chain, sizeof(rop_chain), NULL);
    ok(ret, "Failed to write to remote process thread stack (%d)\n", GetLastError());
#else
    ok( !ctx.Ebp || broken(ctx.Ebp), /* winxp */ "ebp is not zero %08x\n", ctx.Ebp );
    if (!ctx.Ebp)  /* winxp is completely different */
    {
        ok( !ctx.Ecx, "ecx is not zero %08x\n", ctx.Ecx );
        ok( !ctx.Edx, "edx is not zero %08x\n", ctx.Edx );
        ok( !ctx.Esi, "esi is not zero %08x\n", ctx.Esi );
        ok( !ctx.Edi, "edi is not zero %08x\n", ctx.Edi );
    }
    ok( !((ctx.Esp + 0x10) & 0xfff) || broken( !((ctx.Esp + 4) & 0xfff) ), /* winxp, w2k3 */
        "esp is not at top of stack page or properly aligned: %08x\n", ctx.Esp );
    ok( (ctx.EFlags & ~2) == 0x200, "wrong flags %08x\n", ctx.EFlags );
    ok( (WORD)ctx.FloatSave.ControlWord == 0x27f, "wrong control %08x\n", ctx.FloatSave.ControlWord );
    ok( *(WORD *)ctx.ExtendedRegisters == 0x27f, "wrong control %08x\n", *(WORD *)ctx.ExtendedRegisters );
    entry_ptr = (void *)ctx.Eax;
    peb_ptr = (void *)ctx.Ebx;

    rop_chain.exit_process_ptr = exit_process_ptr;
    rop_chain.pipe_name = (ULONG_PTR)remote_pipe_params + offsetof(struct pipe_params, pipe_name);
    rop_chain.pipe_write_buf = (ULONG_PTR)remote_pipe_params + offsetof(struct pipe_params, pipe_write_buf);
    rop_chain.pipe_write_buf_size = sizeof(pipe_params.pipe_write_buf);
    rop_chain.pipe_read_buf = (ULONG_PTR)remote_pipe_params + offsetof(struct pipe_params, pipe_read_buf);
    rop_chain.pipe_read_buf_size = sizeof(pipe_params.pipe_read_buf);
    rop_chain.bytes_returned = (ULONG_PTR)remote_pipe_params + offsetof(struct pipe_params, bytes_returned);
    rop_chain.timeout = 10000;
    rop_chain.exit_code = 0;

    ctx.Eip = (ULONG_PTR)call_named_pipe_a;
    ctx.Esp -= sizeof(rop_chain);
    ret = WriteProcessMemory(pi.hProcess, (void *)ctx.Esp, &rop_chain, sizeof(rop_chain), NULL);
    ok(ret, "Failed to write to remote process thread stack (%d)\n", GetLastError());
#endif

    ret = ReadProcessMemory( pi.hProcess, peb_ptr, &child_peb, sizeof(child_peb), NULL );
    ros_skip_flaky
    ok( ret, "Failed to read PEB (%u)\n", GetLastError() );
    ros_skip_flaky
    ok( child_peb.ImageBaseAddress == exe_base, "wrong base %p/%p\n",
        child_peb.ImageBaseAddress, exe_base );
    ok( entry_ptr == (char *)exe_base + nt_header.OptionalHeader.AddressOfEntryPoint,
        "wrong entry point %p/%p\n", entry_ptr,
        (char *)exe_base + nt_header.OptionalHeader.AddressOfEntryPoint );

    ret = SetThreadContext(pi.hThread, &ctx);
    ok(ret, "Failed to set remote thread context (%d)\n", GetLastError());

    ResumeThread(pi.hThread);

    pipe_connected = ConnectNamedPipe(server_pipe_handle, NULL) || (GetLastError() == ERROR_PIPE_CONNECTED);
    ok(pipe_connected, "Pipe did not connect\n");

    ret = ReadFile(server_pipe_handle, &pipe_magic, sizeof(pipe_magic), &numb, NULL);
    ok(ret, "Failed to read buffer from pipe (%d)\n", GetLastError());

    ok(pipe_magic == pipe_write_magic, "Did not get the correct magic from the remote process\n");

    /* Validate the Imports, at this point the thread in the new process should have
       initialized the EXE module imports and call each dll DllMain notifying it on
       the new thread in the process. */
    ret = are_imports_resolved(pi.hProcess, exe_base, &nt_header);
    ok(ret, "EXE IAT is not resolved\n");

    ret = WriteFile(server_pipe_handle, &pipe_magic, sizeof(pipe_magic), &numb, NULL);
    ok(ret, "Failed to write the magic back to the pipe (%d)\n", GetLastError());

    CloseHandle(server_pipe_handle);
    TerminateProcess(pi.hProcess, 0);
    WaitForSingleObject(pi.hProcess, 10000);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}
#else
static void test_SuspendProcessNewThread(void)
{
}
static void test_SuspendProcessState(void)
{
}
#endif

static void test_DetachStdHandles(void)
{
#ifndef _WIN64
    char                buffer[MAX_PATH], tempfile[MAX_PATH];
    STARTUPINFOA        startup;
    PROCESS_INFORMATION info;
    HANDLE              hstdin, hstdout, hstderr, htemp;
    BOOL                res;

    hstdin = GetStdHandle(STD_INPUT_HANDLE);
    hstdout = GetStdHandle(STD_OUTPUT_HANDLE);
    hstderr = GetStdHandle(STD_ERROR_HANDLE);

    get_file_name(tempfile);
    htemp = CreateFileA(tempfile, GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(htemp != INVALID_HANDLE_VALUE, "failed opening temporary file\n");

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    get_file_name(resfile);
    sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, resfile);

    SetStdHandle(STD_INPUT_HANDLE, htemp);
    SetStdHandle(STD_OUTPUT_HANDLE, htemp);
    SetStdHandle(STD_ERROR_HANDLE, htemp);

    res = CreateProcessA(NULL, buffer, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &startup,
                      &info);

    SetStdHandle(STD_INPUT_HANDLE, hstdin);
    SetStdHandle(STD_OUTPUT_HANDLE, hstdout);
    SetStdHandle(STD_ERROR_HANDLE, hstderr);

    ok(res, "CreateProcess failed\n");
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
    okChildInt("StartupInfoA", "hStdInput", (UINT)INVALID_HANDLE_VALUE);
    okChildInt("StartupInfoA", "hStdOutput", (UINT)INVALID_HANDLE_VALUE);
    okChildInt("StartupInfoA", "hStdError", (UINT)INVALID_HANDLE_VALUE);
    okChildInt("TEB", "hStdInput", 0);
    okChildInt("TEB", "hStdOutput", 0);
    okChildInt("TEB", "hStdError", 0);
    release_memory();
    DeleteFileA(resfile);

    CloseHandle(htemp);
    DeleteFileA(tempfile);
#endif
}

static void test_GetNumaProcessorNode(void)
{
    SYSTEM_INFO si;
    UCHAR node;
    BOOL ret;
    int i;

    if (!pGetNumaProcessorNode)
    {
        win_skip("GetNumaProcessorNode is missing\n");
        return;
    }

    GetSystemInfo(&si);
    for (i = 0; i < 256; i++)
    {
        SetLastError(0xdeadbeef);
        node = (i < si.dwNumberOfProcessors) ? 0xFF : 0xAA;
        ret = pGetNumaProcessorNode(i, &node);
        if (i < si.dwNumberOfProcessors)
        {
            ok(ret, "GetNumaProcessorNode returned FALSE for processor %d\n", i);
            ok(node != 0xFF, "expected node != 0xFF, but got 0xFF\n");
        }
        else
        {
            ok(!ret, "GetNumaProcessorNode returned TRUE for processor %d\n", i);
            ok(node == 0xFF || broken(node == 0xAA) /* WinXP */, "expected node 0xFF, got %x\n", node);
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
        }
    }
}

static void test_session_info(void)
{
    DWORD session_id, active_session;
    BOOL r;

    if (!pProcessIdToSessionId)
    {
        win_skip("ProcessIdToSessionId is missing\n");
        return;
    }

    r = pProcessIdToSessionId(GetCurrentProcessId(), &session_id);
    ok(r, "ProcessIdToSessionId failed: %u\n", GetLastError());
    trace("session_id = %x\n", session_id);

    active_session = pWTSGetActiveConsoleSessionId();
    trace("active_session = %x\n", active_session);
}

static void test_process_info(void)
{
    char buf[4096];
    static const ULONG info_size[] =
    {
        sizeof(PROCESS_BASIC_INFORMATION) /* ProcessBasicInformation */,
        sizeof(QUOTA_LIMITS) /* ProcessQuotaLimits */,
        sizeof(IO_COUNTERS) /* ProcessIoCounters */,
        sizeof(VM_COUNTERS) /* ProcessVmCounters */,
        sizeof(KERNEL_USER_TIMES) /* ProcessTimes */,
        sizeof(ULONG) /* ProcessBasePriority */,
        sizeof(ULONG) /* ProcessRaisePriority */,
        sizeof(HANDLE) /* ProcessDebugPort */,
        sizeof(HANDLE) /* ProcessExceptionPort */,
        0 /* FIXME: sizeof(PROCESS_ACCESS_TOKEN) ProcessAccessToken */,
        0 /* FIXME: sizeof(PROCESS_LDT_INFORMATION) ProcessLdtInformation */,
        0 /* FIXME: sizeof(PROCESS_LDT_SIZE) ProcessLdtSize */,
        sizeof(ULONG) /* ProcessDefaultHardErrorMode */,
        0 /* ProcessIoPortHandlers: kernel-mode only */,
        0 /* FIXME: sizeof(POOLED_USAGE_AND_LIMITS) ProcessPooledUsageAndLimits */,
        0 /* FIXME: sizeof(PROCESS_WS_WATCH_INFORMATION) ProcessWorkingSetWatch */,
        sizeof(ULONG) /* ProcessUserModeIOPL */,
        sizeof(BOOLEAN) /* ProcessEnableAlignmentFaultFixup */,
        sizeof(PROCESS_PRIORITY_CLASS) /* ProcessPriorityClass */,
        sizeof(ULONG) /* ProcessWx86Information */,
        sizeof(ULONG) /* ProcessHandleCount */,
        sizeof(ULONG_PTR) /* ProcessAffinityMask */,
        sizeof(ULONG) /* ProcessPriorityBoost */,
        0 /* sizeof(PROCESS_DEVICEMAP_INFORMATION) ProcessDeviceMap */,
        0 /* sizeof(PROCESS_SESSION_INFORMATION) ProcessSessionInformation */,
        0 /* sizeof(PROCESS_FOREGROUND_BACKGROUND) ProcessForegroundInformation */,
        sizeof(ULONG_PTR) /* ProcessWow64Information */,
        sizeof(buf) /* ProcessImageFileName */,
        sizeof(ULONG) /* ProcessLUIDDeviceMapsEnabled */,
        sizeof(ULONG) /* ProcessBreakOnTermination */,
        sizeof(HANDLE) /* ProcessDebugObjectHandle */,
        sizeof(ULONG) /* ProcessDebugFlags */,
        sizeof(buf) /* ProcessHandleTracing */,
        sizeof(ULONG) /* ProcessIoPriority */,
        sizeof(ULONG) /* ProcessExecuteFlags */,
        0 /* FIXME: sizeof(?) ProcessTlsInformation */,
        0 /* FIXME: sizeof(?) ProcessCookie */,
        sizeof(SECTION_IMAGE_INFORMATION) /* ProcessImageInformation */,
        0 /* FIXME: sizeof(PROCESS_CYCLE_TIME_INFORMATION) ProcessCycleTime */,
        sizeof(ULONG) /* ProcessPagePriority */,
        40 /* ProcessInstrumentationCallback */,
        0 /* FIXME: sizeof(PROCESS_STACK_ALLOCATION_INFORMATION) ProcessThreadStackAllocation */,
        0 /* FIXME: sizeof(PROCESS_WS_WATCH_INFORMATION_EX[]) ProcessWorkingSetWatchEx */,
        sizeof(buf) /* ProcessImageFileNameWin32 */,
        sizeof(HANDLE) /* ProcessImageFileMapping */,
        0 /* FIXME: sizeof(PROCESS_AFFINITY_UPDATE_MODE) ProcessAffinityUpdateMode */,
        0 /* FIXME: sizeof(PROCESS_MEMORY_ALLOCATION_MODE) ProcessMemoryAllocationMode */,
        sizeof(USHORT) /* ProcessGroupInformation */,
        sizeof(ULONG) /* ProcessTokenVirtualizationEnabled */,
        sizeof(ULONG_PTR) /* ProcessConsoleHostProcess */,
        0 /* FIXME: sizeof(PROCESS_WINDOW_INFORMATION) ProcessWindowInformation */,
#if 0 /* FIXME: Add remaining classes */
        sizeof(PROCESS_HANDLE_SNAPSHOT_INFORMATION) /* ProcessHandleInformation */,
        sizeof(PROCESS_MITIGATION_POLICY_INFORMATION) /* ProcessMitigationPolicy */,
        sizeof(ProcessDynamicFunctionTableInformation) /* ProcessDynamicFunctionTableInformation */,
        sizeof(?) /* ProcessHandleCheckingMode */,
        sizeof(PROCESS_KEEPALIVE_COUNT_INFORMATION) /* ProcessKeepAliveCount */,
        sizeof(PROCESS_REVOKE_FILE_HANDLES_INFORMATION) /* ProcessRevokeFileHandles */,
        sizeof(PROCESS_WORKING_SET_CONTROL) /* ProcessWorkingSetControl */,
        sizeof(?) /* ProcessHandleTable */,
        sizeof(?) /* ProcessCheckStackExtentsMode */,
        sizeof(buf) /* ProcessCommandLineInformation */,
        sizeof(PS_PROTECTION) /* ProcessProtectionInformation */,
        sizeof(PROCESS_MEMORY_EXHAUSTION_INFO) /* ProcessMemoryExhaustion */,
        sizeof(PROCESS_FAULT_INFORMATION) /* ProcessFaultInformation */,
        sizeof(PROCESS_TELEMETRY_ID_INFORMATION) /* ProcessTelemetryIdInformation */,
        sizeof(PROCESS_COMMIT_RELEASE_INFORMATION) /* ProcessCommitReleaseInformation */,
        sizeof(?) /* ProcessDefaultCpuSetsInformation */,
        sizeof(?) /* ProcessAllowedCpuSetsInformation */,
        0 /* ProcessReserved1Information */,
        0 /* ProcessReserved2Information */,
        sizeof(?) /* ProcessSubsystemProcess */,
        sizeof(PROCESS_JOB_MEMORY_INFO) /* ProcessJobMemoryInformation */,
#endif
    };
    HANDLE hproc;
    ULONG i, status, ret_len, size;

    if (!pNtQueryInformationProcess)
    {
        win_skip("NtQueryInformationProcess is not available on this platform\n");
        return;
    }

    hproc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, GetCurrentProcessId());
    if (!hproc)
    {
        win_skip("PROCESS_QUERY_LIMITED_INFORMATION is not supported on this platform\n");
        return;
    }

    for (i = 0; i < MaxProcessInfoClass; i++)
    {
        size = info_size[i];
        if (!size) size = sizeof(buf);
        ret_len = 0;
        status = pNtQueryInformationProcess(hproc, i, buf, info_size[i], &ret_len);
        if (status == STATUS_NOT_IMPLEMENTED) continue;
        if (status == STATUS_INVALID_INFO_CLASS) continue;
        if (status == STATUS_INFO_LENGTH_MISMATCH) continue;

        switch (i)
        {
        case ProcessBasicInformation:
        case ProcessQuotaLimits:
        case ProcessTimes:
        case ProcessPriorityClass:
        case ProcessPriorityBoost:
        case ProcessLUIDDeviceMapsEnabled:
        case 33 /* ProcessIoPriority */:
        case ProcessIoCounters:
        case ProcessVmCounters:
        case ProcessWow64Information:
        case ProcessDefaultHardErrorMode:
        case ProcessHandleCount:
        case ProcessImageFileName:
        case ProcessImageInformation:
        case ProcessPagePriority:
        case ProcessImageFileNameWin32:
            ok(status == STATUS_SUCCESS, "for info %u expected STATUS_SUCCESS, got %08x (ret_len %u)\n", i, status, ret_len);
            break;

        case ProcessAffinityMask:
        case ProcessBreakOnTermination:
        case ProcessGroupInformation:
        case ProcessConsoleHostProcess:
            ok(status == STATUS_ACCESS_DENIED /* before win8 */ || status == STATUS_SUCCESS /* win8 is less strict */,
               "for info %u expected STATUS_SUCCESS, got %08x (ret_len %u)\n", i, status, ret_len);
            break;

        case ProcessDebugObjectHandle:
            ok(status == STATUS_ACCESS_DENIED || status == STATUS_PORT_NOT_SET,
               "for info %u expected STATUS_ACCESS_DENIED, got %08x (ret_len %u)\n", i, status, ret_len);
            break;

        case ProcessExecuteFlags:
        case ProcessDebugPort:
        case ProcessDebugFlags:
        case ProcessCookie:
todo_wine
            ok(status == STATUS_ACCESS_DENIED, "for info %u expected STATUS_ACCESS_DENIED, got %08x (ret_len %u)\n", i, status, ret_len);
            break;

        default:
            ok(status == STATUS_ACCESS_DENIED, "for info %u expected STATUS_ACCESS_DENIED, got %08x (ret_len %u)\n", i, status, ret_len);
            break;
        }
    }

    CloseHandle(hproc);
}

static void test_GetLogicalProcessorInformationEx(void)
{
    SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX *info;
    DWORD len;
    BOOL ret;

    if (!pGetLogicalProcessorInformationEx)
    {
        win_skip("GetLogicalProcessorInformationEx() is not supported\n");
        return;
    }

    ret = pGetLogicalProcessorInformationEx(RelationAll, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %d, error %d\n", ret, GetLastError());

    len = 0;
    ret = pGetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &len);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %d, error %d\n", ret, GetLastError());
    ok(len > 0, "got %u\n", len);

    len = 0;
    ret = pGetLogicalProcessorInformationEx(RelationAll, NULL, &len);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %d, error %d\n", ret, GetLastError());
    ok(len > 0, "got %u\n", len);

    info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = pGetLogicalProcessorInformationEx(RelationAll, info, &len);
    ok(ret, "got %d, error %d\n", ret, GetLastError());
    ok(info->Size > 0, "got %u\n", info->Size);
    HeapFree(GetProcessHeap(), 0, info);
}

static void test_largepages(void)
{
    SIZE_T size;

    if (!pGetLargePageMinimum) {
        skip("No GetLargePageMinimum support.\n");
        return;
    }
    size = pGetLargePageMinimum();

    ok((size == 0) || (size == 2*1024*1024) || (size == 4*1024*1024), "GetLargePageMinimum reports %ld size\n", size);
}

struct proc_thread_attr
{
    DWORD_PTR attr;
    SIZE_T size;
    void *value;
};

struct _PROC_THREAD_ATTRIBUTE_LIST
{
    DWORD mask;  /* bitmask of items in list */
    DWORD size;  /* max number of items in list */
    DWORD count; /* number of items in list */
    DWORD pad;
    DWORD_PTR unk;
    struct proc_thread_attr attrs[10];
};

static void test_ProcThreadAttributeList(void)
{
    BOOL ret;
    SIZE_T size, needed;
    int i;
    struct _PROC_THREAD_ATTRIBUTE_LIST list, expect_list;
    HANDLE handles[4];

    if (!pInitializeProcThreadAttributeList)
    {
        win_skip("No support for ProcThreadAttributeList\n");
        return;
    }

    for (i = 0; i <= 10; i++)
    {
        needed = FIELD_OFFSET(struct _PROC_THREAD_ATTRIBUTE_LIST, attrs[i]);
        ret = pInitializeProcThreadAttributeList(NULL, i, 0, &size);
        ok(!ret, "got %d\n", ret);
        if(i >= 4 && GetLastError() == ERROR_INVALID_PARAMETER) /* Vista only allows a maximium of 3 slots */
            break;
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %d\n", GetLastError());
        ok(size == needed, "%d: got %ld expect %ld\n", i, size, needed);

        memset(&list, 0xcc, sizeof(list));
        ret = pInitializeProcThreadAttributeList(&list, i, 0, &size);
        ok(ret, "got %d\n", ret);
        ok(list.mask == 0, "%d: got %08x\n", i, list.mask);
        ok(list.size == i, "%d: got %08x\n", i, list.size);
        ok(list.count == 0, "%d: got %08x\n", i, list.count);
        ok(list.unk == 0, "%d: got %08lx\n", i, list.unk);
    }

    memset(handles, 0, sizeof(handles));
    memset(&expect_list, 0xcc, sizeof(expect_list));
    expect_list.mask = 0;
    expect_list.size = i - 1;
    expect_list.count = 0;
    expect_list.unk = 0;

    ret = pUpdateProcThreadAttribute(&list, 0, 0xcafe, handles, sizeof(PROCESSOR_NUMBER), NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_NOT_SUPPORTED, "got %d\n", GetLastError());

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, handles, sizeof(handles[0]) / 2, NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_BAD_LENGTH, "got %d\n", GetLastError());

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, handles, sizeof(handles[0]) * 2, NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_BAD_LENGTH, "got %d\n", GetLastError());

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, handles, sizeof(handles[0]), NULL, NULL);
    ok(ret, "got %d\n", ret);

    expect_list.mask |= 1 << ProcThreadAttributeParentProcess;
    expect_list.attrs[0].attr = PROC_THREAD_ATTRIBUTE_PARENT_PROCESS;
    expect_list.attrs[0].size = sizeof(handles[0]);
    expect_list.attrs[0].value = handles;
    expect_list.count++;

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, handles, sizeof(handles[0]), NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_OBJECT_NAME_EXISTS, "got %d\n", GetLastError());

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handles, sizeof(handles) - 1, NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_BAD_LENGTH, "got %d\n", GetLastError());

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handles, sizeof(handles), NULL, NULL);
    ok(ret, "got %d\n", ret);

    expect_list.mask |= 1 << ProcThreadAttributeHandleList;
    expect_list.attrs[1].attr = PROC_THREAD_ATTRIBUTE_HANDLE_LIST;
    expect_list.attrs[1].size = sizeof(handles);
    expect_list.attrs[1].value = handles;
    expect_list.count++;

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handles, sizeof(handles), NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_OBJECT_NAME_EXISTS, "got %d\n", GetLastError());

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR, handles, sizeof(PROCESSOR_NUMBER), NULL, NULL);
    ok(ret || GetLastError() == ERROR_NOT_SUPPORTED, "got %d gle %d\n", ret, GetLastError());

    if (ret)
    {
        expect_list.mask |= 1 << ProcThreadAttributeIdealProcessor;
        expect_list.attrs[2].attr = PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR;
        expect_list.attrs[2].size = sizeof(PROCESSOR_NUMBER);
        expect_list.attrs[2].value = handles;
        expect_list.count++;
    }

    ok(!memcmp(&list, &expect_list, size), "mismatch\n");

    pDeleteProcThreadAttributeList(&list);
}

static void test_GetActiveProcessorCount(void)
{
    DWORD count;

    if (!pGetActiveProcessorCount)
    {
        win_skip("GetActiveProcessorCount not available, skipping test\n");
        return;
    }

    count = pGetActiveProcessorCount(0);
    ok(count, "GetActiveProcessorCount failed, error %u\n", GetLastError());

    /* Test would fail on systems with more than 6400 processors */
    SetLastError(0xdeadbeef);
    count = pGetActiveProcessorCount(101);
    ok(count == 0, "Expeced GetActiveProcessorCount to fail\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %u\n", GetLastError());
}

START_TEST(process)
{
    HANDLE job;
    BOOL b = init();
    ok(b, "Basic init of CreateProcess test\n");
    if (!b) return;

    if (myARGC >= 3)
    {
        if (!strcmp(myARGV[2], "dump") && myARGC >= 4)
        {
            doChild(myARGV[3], (myARGC >= 5) ? myARGV[4] : NULL);
            return;
        }
        else if (!strcmp(myARGV[2], "wait"))
        {
            Sleep(30000);
            ok(0, "Child process not killed\n");
            return;
        }
        else if (!strcmp(myARGV[2], "exit"))
        {
            Sleep(100);
            return;
        }
        else if (!strcmp(myARGV[2], "nested") && myARGC >= 4)
        {
            char                buffer[MAX_PATH];
            STARTUPINFOA        startup;
            PROCESS_INFORMATION info;

            memset(&startup, 0, sizeof(startup));
            startup.cb = sizeof(startup);
            startup.dwFlags = STARTF_USESHOWWINDOW;
            startup.wShowWindow = SW_SHOWNORMAL;

            sprintf(buffer, "\"%s\" tests/process.c dump \"%s\"", selfname, myARGV[3]);
            ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &startup, &info), "CreateProcess failed\n");
            CloseHandle(info.hProcess);
            CloseHandle(info.hThread);
            return;
        }

        ok(0, "Unexpected command %s\n", myARGV[2]);
        return;
    }

    test_process_info();
    test_TerminateProcess();
    test_Startup();
    test_CommandLine();
    test_Directory();
    test_Toolhelp();
    test_Environment();
    test_SuspendFlag();
    test_DebuggingFlag();
    test_Console();
    test_ExitCode();
    test_OpenProcess();
    test_GetProcessVersion();
    test_GetProcessImageFileNameA();
    test_QueryFullProcessImageNameA();
    test_QueryFullProcessImageNameW();
    test_Handles();
    test_IsWow64Process();
    test_SystemInfo();
    test_RegistryQuota();
    test_DuplicateHandle();
    test_StartupNoConsole();
    test_DetachConsoleHandles();
    test_DetachStdHandles();
    test_GetNumaProcessorNode();
    test_session_info();
    test_GetLogicalProcessorInformationEx();
    test_GetActiveProcessorCount();
    test_largepages();
    test_ProcThreadAttributeList();
    test_SuspendProcessState();
    test_SuspendProcessNewThread();

    /* things that can be tested:
     *  lookup:         check the way program to be executed is searched
     *  handles:        check the handle inheritance stuff (+sec options)
     *  console:        check if console creation parameters work
     */

    if (!pCreateJobObjectW)
    {
        win_skip("No job object support\n");
        return;
    }

    test_IsProcessInJob();
    test_TerminateJobObject();
    test_QueryInformationJobObject();
    test_CompletionPort();
    test_KillOnJobClose();
    test_WaitForJobObject();
    job = test_AddSelfToJob();
    test_jobInheritance(job);
    test_BreakawayOk(job);
    CloseHandle(job);
}
