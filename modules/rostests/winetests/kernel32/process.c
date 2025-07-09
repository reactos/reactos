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
#include "wine/heap.h"
#ifdef __REACTOS__
// #include <wincon_undoc.h>
typedef void *HPCON;
typedef struct _SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION {
    DWORD Machine : 16;
    DWORD KernelMode : 1;
    DWORD UserMode : 1;
    DWORD Native : 1;
    DWORD Process : 1;
    DWORD WoW64Container : 1;
    DWORD ReservedZero0 : 11;
} SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION;

typedef enum _MACHINE_ATTRIBUTES
{
    UserEnabled    = 0x00000001,
    KernelEnabled  = 0x00000002,
    Wow64Container = 0x00000004,
} MACHINE_ATTRIBUTES;

typedef struct _PROCESS_MACHINE_INFORMATION {
    USHORT ProcessMachine;
    USHORT Res0;
    MACHINE_ATTRIBUTES MachineAttributes;
} PROCESS_MACHINE_INFORMATION;
#endif

/* PROCESS_ALL_ACCESS in Vista+ PSDKs is incompatible with older Windows versions */
#define PROCESS_ALL_ACCESS_NT4 (PROCESS_ALL_ACCESS & ~0xf000)
/* THREAD_ALL_ACCESS in Vista+ PSDKs is incompatible with older Windows versions */
#define THREAD_ALL_ACCESS_NT4 (STANDARD_RIGHTS_REQUIRED | SYNCHRONIZE | 0x3ff)

#define expect_eq_d(expected, actual) \
    do { \
      int value = (actual); \
      ok((expected) == value, "Expected " #actual " to be %d (" #expected ") is %d\n", \
         (int)(expected), value); \
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
static BOOL   (WINAPI *pIsWow64Process2)(HANDLE, USHORT *, USHORT *);
static BOOL   (WINAPI *pQueryFullProcessImageNameA)(HANDLE hProcess, DWORD dwFlags, LPSTR lpExeName, PDWORD lpdwSize);
static BOOL   (WINAPI *pQueryFullProcessImageNameW)(HANDLE hProcess, DWORD dwFlags, LPWSTR lpExeName, PDWORD lpdwSize);
static DWORD  (WINAPI *pK32GetProcessImageFileNameA)(HANDLE,LPSTR,DWORD);
static HANDLE (WINAPI *pCreateJobObjectW)(LPSECURITY_ATTRIBUTES sa, LPCWSTR name);
static HANDLE (WINAPI *pOpenJobObjectA)(DWORD access, BOOL inherit, LPCSTR name);
static BOOL   (WINAPI *pAssignProcessToJobObject)(HANDLE job, HANDLE process);
static BOOL   (WINAPI *pIsProcessInJob)(HANDLE process, HANDLE job, PBOOL result);
static BOOL   (WINAPI *pTerminateJobObject)(HANDLE job, UINT exit_code);
static BOOL   (WINAPI *pQueryInformationJobObject)(HANDLE job, JOBOBJECTINFOCLASS class, LPVOID info, DWORD len, LPDWORD ret_len);
static BOOL   (WINAPI *pSetInformationJobObject)(HANDLE job, JOBOBJECTINFOCLASS class, LPVOID info, DWORD len);
static HANDLE (WINAPI *pCreateIoCompletionPort)(HANDLE file, HANDLE existing_port, ULONG_PTR key, DWORD threads);
static BOOL   (WINAPI *pGetNumaProcessorNode)(UCHAR, PUCHAR);
static NTSTATUS (WINAPI *pNtQueryInformationProcess)(HANDLE, PROCESSINFOCLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI *pNtQueryInformationThread)(HANDLE, THREADINFOCLASS, PVOID, ULONG, PULONG);
static NTSTATUS (WINAPI *pNtQuerySystemInformationEx)(SYSTEM_INFORMATION_CLASS, void*, ULONG, void*, ULONG, ULONG*);
static DWORD  (WINAPI *pWTSGetActiveConsoleSessionId)(void);
static HANDLE (WINAPI *pCreateToolhelp32Snapshot)(DWORD, DWORD);
static BOOL   (WINAPI *pProcess32First)(HANDLE, PROCESSENTRY32*);
static BOOL   (WINAPI *pProcess32Next)(HANDLE, PROCESSENTRY32*);
static BOOL   (WINAPI *pThread32First)(HANDLE, THREADENTRY32*);
static BOOL   (WINAPI *pThread32Next)(HANDLE, THREADENTRY32*);
static BOOL   (WINAPI *pGetLogicalProcessorInformationEx)(LOGICAL_PROCESSOR_RELATIONSHIP,SYSTEM_LOGICAL_PROCESSOR_INFORMATION_EX*,DWORD*);
static SIZE_T (WINAPI *pGetLargePageMinimum)(void);
#ifndef __REACTOS__
static BOOL   (WINAPI *pGetSystemCpuSetInformation)(SYSTEM_CPU_SET_INFORMATION*,ULONG,ULONG*,HANDLE,ULONG);
#endif
static BOOL   (WINAPI *pInitializeProcThreadAttributeList)(struct _PROC_THREAD_ATTRIBUTE_LIST*, DWORD, DWORD, SIZE_T*);
static BOOL   (WINAPI *pUpdateProcThreadAttribute)(struct _PROC_THREAD_ATTRIBUTE_LIST*, DWORD, DWORD_PTR, void *,SIZE_T,void*,SIZE_T*);
static void   (WINAPI *pDeleteProcThreadAttributeList)(struct _PROC_THREAD_ATTRIBUTE_LIST*);
static DWORD  (WINAPI *pGetActiveProcessorCount)(WORD);
static DWORD  (WINAPI *pGetMaximumProcessorCount)(WORD);
static BOOL   (WINAPI *pGetProcessInformation)(HANDLE,PROCESS_INFORMATION_CLASS,void*,DWORD);

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

static void wait_and_close_child_process(PROCESS_INFORMATION *pi)
{
    wait_child_process(pi->hProcess);
    CloseHandle(pi->hThread);
    CloseHandle(pi->hProcess);
}

static void reload_child_info(const char* resfile)
{
    /* This forces the profile functions to reload the resource file
     * after the child process has modified it.
     */
    WritePrivateProfileStringA(NULL, NULL, NULL, resfile);
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
    GetModuleFileNameA( 0, selfname, sizeof(selfname) );

    /* Strip the path of selfname */
    if ((p = strrchr(selfname, '\\')) != NULL) exename = p + 1;
    else exename = selfname;

    if ((p = strrchr(exename, '/')) != NULL) exename = p + 1;

    hkernel32 = GetModuleHandleA("kernel32");
    hntdll    = GetModuleHandleA("ntdll.dll");

    pNtQueryInformationProcess = (void *)GetProcAddress(hntdll, "NtQueryInformationProcess");
    pNtQueryInformationThread = (void *)GetProcAddress(hntdll, "NtQueryInformationThread");
    pNtQuerySystemInformationEx = (void *)GetProcAddress(hntdll, "NtQuerySystemInformationEx");

    pGetNativeSystemInfo = (void *) GetProcAddress(hkernel32, "GetNativeSystemInfo");
    pGetSystemRegistryQuota = (void *) GetProcAddress(hkernel32, "GetSystemRegistryQuota");
    pIsWow64Process = (void *) GetProcAddress(hkernel32, "IsWow64Process");
    pIsWow64Process2 = (void *) GetProcAddress(hkernel32, "IsWow64Process2");
    pQueryFullProcessImageNameA = (void *) GetProcAddress(hkernel32, "QueryFullProcessImageNameA");
    pQueryFullProcessImageNameW = (void *) GetProcAddress(hkernel32, "QueryFullProcessImageNameW");
    pK32GetProcessImageFileNameA = (void *) GetProcAddress(hkernel32, "K32GetProcessImageFileNameA");
    pCreateJobObjectW = (void *)GetProcAddress(hkernel32, "CreateJobObjectW");
    pOpenJobObjectA = (void *)GetProcAddress(hkernel32, "OpenJobObjectA");
    pAssignProcessToJobObject = (void *)GetProcAddress(hkernel32, "AssignProcessToJobObject");
    pIsProcessInJob = (void *)GetProcAddress(hkernel32, "IsProcessInJob");
    pTerminateJobObject = (void *)GetProcAddress(hkernel32, "TerminateJobObject");
    pQueryInformationJobObject = (void *)GetProcAddress(hkernel32, "QueryInformationJobObject");
    pSetInformationJobObject = (void *)GetProcAddress(hkernel32, "SetInformationJobObject");
    pCreateIoCompletionPort = (void *)GetProcAddress(hkernel32, "CreateIoCompletionPort");
    pGetNumaProcessorNode = (void *)GetProcAddress(hkernel32, "GetNumaProcessorNode");
    pWTSGetActiveConsoleSessionId = (void *)GetProcAddress(hkernel32, "WTSGetActiveConsoleSessionId");
    pCreateToolhelp32Snapshot = (void *)GetProcAddress(hkernel32, "CreateToolhelp32Snapshot");
    pProcess32First = (void *)GetProcAddress(hkernel32, "Process32First");
    pProcess32Next = (void *)GetProcAddress(hkernel32, "Process32Next");
    pThread32First = (void *)GetProcAddress(hkernel32, "Thread32First");
    pThread32Next = (void *)GetProcAddress(hkernel32, "Thread32Next");
    pGetLogicalProcessorInformationEx = (void *)GetProcAddress(hkernel32, "GetLogicalProcessorInformationEx");
    pGetLargePageMinimum = (void *)GetProcAddress(hkernel32, "GetLargePageMinimum");
#ifndef __REACTOS__
    pGetSystemCpuSetInformation = (void *)GetProcAddress(hkernel32, "GetSystemCpuSetInformation");
#endif
    pInitializeProcThreadAttributeList = (void *)GetProcAddress(hkernel32, "InitializeProcThreadAttributeList");
    pUpdateProcThreadAttribute = (void *)GetProcAddress(hkernel32, "UpdateProcThreadAttribute");
    pDeleteProcThreadAttributeList = (void *)GetProcAddress(hkernel32, "DeleteProcThreadAttributeList");
    pGetActiveProcessorCount = (void *)GetProcAddress(hkernel32, "GetActiveProcessorCount");
    pGetMaximumProcessorCount = (void *)GetProcAddress(hkernel32, "GetMaximumProcessorCount");
    pGetProcessInformation = (void *)GetProcAddress(hkernel32, "GetProcessInformation");

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
#ifdef __REACTOS__
static void WINAPIV WINETEST_PRINTF_ATTR(2,3) childPrintf(HANDLE h, const char* fmt, ...)
#else
static void WINAPIV __WINE_PRINTF_ATTR(2,3) childPrintf(HANDLE h, const char* fmt, ...)
#endif
{
    va_list valist;
    char        buffer[1024+4*MAX_LISTED_ENV_VAR];
    DWORD       w;

    va_start(valist, fmt);
    vsprintf(buffer, fmt, valist);
    va_end(valist);
    WriteFile(h, buffer, strlen(buffer), &w, NULL);
}

/* bits 0..1 contains FILE_TYPE_{UNKNOWN, CHAR, PIPE, DISK} */
#define HATTR_NULL      0x08               /* NULL handle value */
#define HATTR_INVALID   0x04               /* INVALID_HANDLE_VALUE */
#define HATTR_TYPE      0x0c               /* valid handle, with type set */
#define HATTR_UNTOUCHED 0x10               /* Identify fields untouched by GetStartupInfoW */
#define HATTR_INHERIT   0x20               /* inheritance flag set */
#define HATTR_PROTECT   0x40               /* protect from close flag set */
#define HATTR_DANGLING  0x80               /* a pseudo value to show that the handle value has been copied but not inherited */

#define HANDLE_UNTOUCHEDW (HANDLE)(DWORD_PTR)(0x5050505050505050ull)

static unsigned encode_handle_attributes(HANDLE h)
{
    DWORD dw;
    unsigned result;

    if (h == NULL)
        result = HATTR_NULL;
    else if (h == INVALID_HANDLE_VALUE)
        result = HATTR_INVALID;
    else if (h == HANDLE_UNTOUCHEDW)
        result = HATTR_UNTOUCHED;
    else
    {
        result = HATTR_TYPE;
        dw = GetFileType(h);
        if (dw == FILE_TYPE_CHAR || dw == FILE_TYPE_DISK || dw == FILE_TYPE_PIPE)
        {
            DWORD info;
            if (GetHandleInformation(h, &info))
            {
                if (info & HANDLE_FLAG_INHERIT)
                    result |= HATTR_INHERIT;
                if (info & HANDLE_FLAG_PROTECT_FROM_CLOSE)
                    result |= HATTR_PROTECT;
            }
        }
        else
            dw = FILE_TYPE_UNKNOWN;
        result |= dw;
    }
    return result;
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
    memset(&siA, 0xA0, sizeof(siA));
    GetStartupInfoA(&siA);
    childPrintf(hFile,
                "[StartupInfoA]\ncb=%08lu\nlpDesktop=%s\nlpTitle=%s\n"
                "dwX=%lu\ndwY=%lu\ndwXSize=%lu\ndwYSize=%lu\n"
                "dwXCountChars=%lu\ndwYCountChars=%lu\ndwFillAttribute=%lu\n"
                "dwFlags=%lu\nwShowWindow=%u\n"
                "hStdInput=%Iu\nhStdOutput=%Iu\nhStdError=%Iu\n"
                "hStdInputEncode=%u\nhStdOutputEncode=%u\nhStdErrorEncode=%u\n\n",
                siA.cb, encodeA(siA.lpDesktop), encodeA(siA.lpTitle),
                siA.dwX, siA.dwY, siA.dwXSize, siA.dwYSize,
                siA.dwXCountChars, siA.dwYCountChars, siA.dwFillAttribute,
                siA.dwFlags, siA.wShowWindow,
                (DWORD_PTR)siA.hStdInput, (DWORD_PTR)siA.hStdOutput, (DWORD_PTR)siA.hStdError,
                encode_handle_attributes(siA.hStdInput), encode_handle_attributes(siA.hStdOutput),
                encode_handle_attributes(siA.hStdError));

    /* check the console handles in the TEB */
    childPrintf(hFile,
                "[TEB]\nhStdInput=%Iu\nhStdOutput=%Iu\nhStdError=%Iu\n"
                "hStdInputEncode=%u\nhStdOutputEncode=%u\nhStdErrorEncode=%u\n\n",
                (DWORD_PTR)params->hStdInput, (DWORD_PTR)params->hStdOutput,
                (DWORD_PTR)params->hStdError,
                encode_handle_attributes(params->hStdInput), encode_handle_attributes(params->hStdOutput),
                encode_handle_attributes(params->hStdError));

    memset(&siW, 0x50, sizeof(siW));
    GetStartupInfoW(&siW);
    childPrintf(hFile,
                "[StartupInfoW]\ncb=%08lu\nlpDesktop=%s\nlpTitle=%s\n"
                "dwX=%lu\ndwY=%lu\ndwXSize=%lu\ndwYSize=%lu\n"
                "dwXCountChars=%lu\ndwYCountChars=%lu\ndwFillAttribute=%lu\n"
                "dwFlags=%lu\nwShowWindow=%u\n"
                "hStdInput=%Iu\nhStdOutput=%Iu\nhStdError=%Iu\n"
                "hStdInputEncode=%u\nhStdOutputEncode=%u\nhStdErrorEncode=%u\n\n",
                siW.cb, encodeW(siW.lpDesktop), encodeW(siW.lpTitle),
                siW.dwX, siW.dwY, siW.dwXSize, siW.dwYSize,
                siW.dwXCountChars, siW.dwYCountChars, siW.dwFillAttribute,
                siW.dwFlags, siW.wShowWindow,
                (DWORD_PTR)siW.hStdInput, (DWORD_PTR)siW.hStdOutput, (DWORD_PTR)siW.hStdError,
                encode_handle_attributes(siW.hStdInput), encode_handle_attributes(siW.hStdOutput),
                encode_handle_attributes(siW.hStdError));

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
    ok(snapshot != INVALID_HANDLE_VALUE, "CreateToolhelp32Snapshot failed %lu\n", GetLastError());
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
                "[Toolhelp]\ncntUsage=%lu\nth32DefaultHeapID=%Iu\n"
                "th32ModuleID=%lu\ncntThreads=%lu\nth32ParentProcessID=%lu\n"
                "pcPriClassBase=%lu\ndwFlags=%lu\nszExeFile=%s\n\n",
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
    if (GetCurrentDirectoryW(ARRAY_SIZE(bufW), bufW))
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
            childPrintf(hFile, "InputMode=%lu\n", modeIn);
        if (GetConsoleMode(hConOut, &modeOut))
            childPrintf(hFile, "OutputMode=%lu\n", modeOut);

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
        ok( ret, "Setting mode (%ld)\n", GetLastError());
        ret = SetConsoleMode(hConOut, modeOut ^ 1);
        ok( ret, "Setting mode (%ld)\n", GetLastError());
        sbi.dwCursorPosition.X = !sbi.dwCursorPosition.X;
        sbi.dwCursorPosition.Y = !sbi.dwCursorPosition.Y;
        ret = SetConsoleCursorPosition(hConOut, sbi.dwCursorPosition);
        ok( ret, "Setting cursor position (%ld)\n", GetLastError());
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

static int strCmp(const char* s1, const char* s2, BOOL sensitive)
{
    if (!s1 && !s2) return 0;
    if (!s2) return -1;
    if (!s1) return 1;
    return (sensitive) ? strcmp(s1, s2) : strcasecmp(s1, s2);
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

static void ok_child_hexint( int line, const char *sect, const char *key, UINT expect, UINT is_broken )
{
    UINT result = GetPrivateProfileIntA( sect, key, !expect, resfile );
    ok_(__FILE__, line)( result == expect || broken( is_broken && result == is_broken ), "%s:%s expected %#x, but got %#x\n", sect, key, expect, result );
}

#define okChildString(sect, key, expect) ok_child_string(__LINE__, (sect), (key), (expect), 1 )
#define okChildIString(sect, key, expect) ok_child_string(__LINE__, (sect), (key), (expect), 0 )
#define okChildStringWA(sect, key, expect) ok_child_stringWA(__LINE__, (sect), (key), (expect), 1 )
#define okChildInt(sect, key, expect) ok_child_int(__LINE__, (sect), (key), (expect))
#define okChildHexInt(sect, key, expect, is_broken) ok_child_hexint(__LINE__, (sect), (key), (expect), (is_broken))

static void test_Startup(void)
{
    char                buffer[2 * MAX_PATH + 25];
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
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
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
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
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
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
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
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
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
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
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
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
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
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
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
    char                buffer[2 * MAX_PATH + 65], fullpath[MAX_PATH], *lpFilePart, *p;
    char                buffer2[MAX_PATH + 44];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    BOOL                ret;
    LPWSTR              cmdline, cmdline_backup;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    /* failure case */
    strcpy(buffer, "\"t:\\NotADir\\NotAFile.exe\"");
    memset(&info, 0xa, sizeof(info));
    ok(!CreateProcessA(buffer, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess unexpectedly succeeded\n");
    /* Check that the effective STARTUPINFOA parameters are not modified */
    ok(startup.cb == sizeof(startup), "unexpected cb %ld\n", startup.cb);
    ok(startup.lpDesktop == NULL, "lpDesktop is not NULL\n");
    ok(startup.lpTitle == NULL, "lpTitle is not NULL\n");
    ok(startup.dwFlags == STARTF_USESHOWWINDOW, "unexpected dwFlags %04lx\n", startup.dwFlags);
    ok(startup.wShowWindow == SW_SHOWNORMAL, "unexpected wShowWindow %d\n", startup.wShowWindow);
    ok(!info.hProcess, "unexpected hProcess %p\n", info.hProcess);
    ok(!info.hThread, "unexpected hThread %p\n", info.hThread);
    ok(!info.dwProcessId, "unexpected dwProcessId %04lx\n", info.dwProcessId);
    ok(!info.dwThreadId, "unexpected dwThreadId %04lx\n", info.dwThreadId);

    /* the basics; not getting confused by the leading and trailing " */
    get_file_name(resfile);
    sprintf(buffer, "\"%s\" process dump \"%s\" \"C:\\Program Files\\my nice app.exe\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* Check that the effective STARTUPINFOA parameters are not modified */
    ok(startup.cb == sizeof(startup), "unexpected cb %ld\n", startup.cb);
    ok(startup.lpDesktop == NULL, "lpDesktop is not NULL\n");
    ok(startup.lpTitle == NULL, "lpTitle is not NULL\n");
    ok(startup.dwFlags == STARTF_USESHOWWINDOW, "unexpected dwFlags %04lx\n", startup.dwFlags);
    ok(startup.wShowWindow == SW_SHOWNORMAL, "unexpected wShowWindow %d\n", startup.wShowWindow);
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
    okChildInt("Arguments", "argcA", 5);
    okChildString("Arguments", "argvA4", "C:\\Program Files\\my nice app.exe");
    okChildString("Arguments", "argvA5", NULL);
    okChildString("Arguments", "CommandLineA", buffer);
    release_memory();
    DeleteFileA(resfile);

    /* test main()'s quotes handling */
    get_file_name(resfile);
    sprintf(buffer, "\"%s\" process dump \"%s\" \"a\\\"b\\\\\" c\\\" d", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
    okChildInt("Arguments", "argcA", 7);
    okChildString("Arguments", "argvA4", "a\"b\\");
    okChildString("Arguments", "argvA5", "c\"");
    okChildString("Arguments", "argvA6", "d");
    okChildString("Arguments", "argvA7", NULL);
    okChildString("Arguments", "CommandLineA", buffer);
    release_memory();
    DeleteFileA(resfile);

    GetFullPathNameA(selfname, MAX_PATH, fullpath, &lpFilePart);
    assert ( lpFilePart != 0);
    *(lpFilePart -1 ) = 0;
    SetCurrentDirectoryA( fullpath );

    /* Test for Bug1330 to show that XP doesn't change '/' to '\\' in argv[0]
     * and " escaping.
     */
    get_file_name(resfile);
    /* Use exename to avoid buffer containing things like 'C:' */
    sprintf(buffer, "./%s process dump \"%s\" \"\"\"\"", exename, resfile);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess (%s) failed : %ld\n", buffer, GetLastError());
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
    sprintf(buffer, "./%s", exename);
    okChildInt("Arguments", "argcA", 5);
    okChildString("Arguments", "argvA0", buffer);
    okChildString("Arguments", "argvA4", "\"");
    okChildString("Arguments", "argvA5", NULL);
    release_memory();
    DeleteFileA(resfile);

    get_file_name(resfile);
    /* Use exename to avoid buffer containing things like 'C:' */
    sprintf(buffer, ".\\%s process dump \"%s\"", exename, resfile);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess (%s) failed : %ld\n", buffer, GetLastError());
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
    sprintf(buffer, ".\\%s", exename);
    okChildString("Arguments", "argvA0", buffer);
    release_memory();
    DeleteFileA(resfile);

    get_file_name(resfile);
    p = strrchr(fullpath, '\\');
    /* Use exename to avoid buffer containing things like 'C:' */
    if (p) sprintf(buffer, "..%s/%s process dump \"%s\"", p, exename, resfile);
    else sprintf(buffer, "./%s process dump \"%s\"", exename, resfile);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess (%s) failed : %ld\n", buffer, GetLastError());
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
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
    sprintf(buffer2, "dummy process dump \"%s\"", resfile);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(buffer, buffer2, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess (%s) failed : %ld\n", buffer, GetLastError());
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
    okChildString("Arguments", "argvA0", "dummy");
    okChildString("Arguments", "CommandLineA", buffer2);
    okChildStringWA("Arguments", "CommandLineW", buffer2);
    release_memory();
    DeleteFileA(resfile);
    SetCurrentDirectoryA( base );

    if (0) /* Test crashes on NT-based Windows. */
    {
        /* Test NULL application name and command line parameters. */
        SetLastError(0xdeadbeef);
        ret = CreateProcessA(NULL, NULL, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
        ok(!ret, "CreateProcessA unexpectedly succeeded\n");
        ok(GetLastError() == ERROR_INVALID_PARAMETER,
           "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    }

    buffer[0] = '\0';

    /* Test empty application name parameter. */
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(buffer, NULL, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_PATH_NOT_FOUND ||
       broken(GetLastError() == ERROR_FILE_NOT_FOUND) /* Win9x/WinME */ ||
       broken(GetLastError() == ERROR_ACCESS_DENIED) /* Win98 */,
       "Expected ERROR_PATH_NOT_FOUND, got %ld\n", GetLastError());

    buffer2[0] = '\0';

    /* Test empty application name and command line parameters. */
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(buffer, buffer2, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_PATH_NOT_FOUND ||
       broken(GetLastError() == ERROR_FILE_NOT_FOUND) /* Win9x/WinME */ ||
       broken(GetLastError() == ERROR_ACCESS_DENIED) /* Win98 */,
       "Expected ERROR_PATH_NOT_FOUND, got %ld\n", GetLastError());

    /* Test empty command line parameter. */
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, buffer2, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       GetLastError() == ERROR_PATH_NOT_FOUND /* NT4 */ ||
       GetLastError() == ERROR_BAD_PATHNAME /* Win98 */ ||
       GetLastError() == ERROR_INVALID_PARAMETER /* Win7 */,
       "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());

    strcpy(buffer, "doesnotexist.exe");
    strcpy(buffer2, "does not exist.exe");

    /* Test nonexistent application name. */
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(buffer, NULL, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CreateProcessA(buffer2, NULL, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());

    /* Test nonexistent command line parameter. */
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, buffer2, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info);
    ok(!ret, "CreateProcessA unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %ld\n", GetLastError());

    /* Test whether GetCommandLineW reads directly from TEB or from a cached address */
    cmdline = GetCommandLineW();
    ok(cmdline == NtCurrentTeb()->Peb->ProcessParameters->CommandLine.Buffer, "Expected address from TEB, got %p\n", cmdline);

    cmdline_backup = cmdline;
    NtCurrentTeb()->Peb->ProcessParameters->CommandLine.Buffer = NULL;
    cmdline = GetCommandLineW();
    ok(cmdline == cmdline_backup, "Expected cached address from TEB, got %p\n", cmdline);
    NtCurrentTeb()->Peb->ProcessParameters->CommandLine.Buffer = cmdline_backup;
}

static void test_Directory(void)
{
    char                buffer[2 * MAX_PATH + 25];
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
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);
    GetWindowsDirectoryA( windir, sizeof(windir) );
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, windir, &startup, &info), "CreateProcess\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
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
    ok(GetLastError() == ERROR_DIRECTORY, "Expected ERROR_DIRECTORY, got %ld\n", GetLastError());
    ok(!TerminateProcess(info.hProcess, 0), "Child process should not exist\n");
}

static void test_Toolhelp(void)
{
    char                buffer[2 * MAX_PATH + 27];
    STARTUPINFOA        startup;
    PROCESS_INFORMATION info;
    HANDLE              process, thread, snapshot;
    DWORD               nested_pid;
    PROCESSENTRY32      pe;
    THREADENTRY32       te;
    DWORD               ret;
    int                 i;

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess failed\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
    okChildInt("Toolhelp", "cntUsage", 0);
    okChildInt("Toolhelp", "th32DefaultHeapID", 0);
    okChildInt("Toolhelp", "th32ModuleID", 0);
    okChildInt("Toolhelp", "th32ParentProcessID", GetCurrentProcessId());
    /* pcPriClassBase differs between Windows versions (either 6 or 8) */
    okChildInt("Toolhelp", "dwFlags", 0);

    release_memory();
    DeleteFileA(resfile);

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" process nested \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess failed\n");
    wait_child_process(info.hProcess);

    process = OpenProcess(PROCESS_ALL_ACCESS_NT4, FALSE, info.dwProcessId);
    ok(process != NULL, "OpenProcess failed %lu\n", GetLastError());
    CloseHandle(process);

    CloseHandle(info.hProcess);
    CloseHandle(info.hThread);

    for (i = 0; i < 20; i++)
    {
        SetLastError(0xdeadbeef);
        process = OpenProcess(PROCESS_ALL_ACCESS_NT4, FALSE, info.dwProcessId);
        ok(process || GetLastError() == ERROR_INVALID_PARAMETER, "OpenProcess failed %lu\n", GetLastError());
        if (!process) break;
        CloseHandle(process);
        Sleep(100);
    }
    /* The following test fails randomly on some Windows versions, but Gothic 2 depends on it */
    ok(i < 20 || broken(i == 20), "process object not released\n");

    /* Look for the nested process by pid */
    reload_child_info(resfile);
    nested_pid = GetPrivateProfileIntA("Nested", "Pid", 0, resfile);
    DeleteFileA(resfile);

    snapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    ok(snapshot != INVALID_HANDLE_VALUE, "CreateToolhelp32Snapshot failed %lu\n", GetLastError());
    memset(&pe, 0, sizeof(pe));
    pe.dwSize = sizeof(pe);
    if (pProcess32First(snapshot, &pe))
    {
        while (pe.th32ProcessID != nested_pid)
            if (!pProcess32Next(snapshot, &pe)) break;
    }
    CloseHandle(snapshot);
    ok(pe.th32ProcessID == nested_pid, "failed to find nested child process\n");
    ok(pe.th32ParentProcessID == info.dwProcessId, "nested child process has parent %lu instead of %lu\n", pe.th32ParentProcessID, info.dwProcessId);
    ok(stricmp(pe.szExeFile, exename) == 0, "nested executable is %s instead of %s\n", pe.szExeFile, exename);

    process = OpenProcess(PROCESS_ALL_ACCESS_NT4, FALSE, pe.th32ProcessID);
    ok(process != NULL, "OpenProcess failed %lu\n", GetLastError());

    snapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    ok(snapshot != INVALID_HANDLE_VALUE, "CreateToolhelp32Snapshot failed %lu\n", GetLastError());
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
    ok(thread != NULL, "OpenThread failed %lu\n", GetLastError());
    ret = ResumeThread(thread);
    ok(ret == 1, "expected 1, got %lu\n", ret);
    CloseHandle(thread);

    wait_child_process(process);
    CloseHandle(process);

    reload_child_info(resfile);
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
    char                buffer[2 * MAX_PATH + 25];
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
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, NULL, NULL, &startup, &info), "CreateProcess\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
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
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);

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
     * - PATH (already set above)
     * - the directory definitions (=[A-Z]:=)
     */
    for (ptr2 = env; *ptr2; ptr2 += strlen(ptr2) + 1)
    {
        if (strncmp(ptr2, "PATH=", 5) != 0 &&
            !is_str_env_drive_dir(ptr2))
        {
            strcpy(ptr, ptr2);
            ptr += strlen(ptr) + 1;
        }
    }
    *ptr = '\0';
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0L, child_env, NULL, &startup, &info), "CreateProcess\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
    cmpEnvironment(child_env);

    HeapFree(GetProcessHeap(), 0, child_env);
    FreeEnvironmentStringsA(env);
    release_memory();
    DeleteFileA(resfile);
}

static  void    test_SuspendFlag(void)
{
    char                buffer[2 * MAX_PATH + 25];
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
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &startup, &info), "CreateProcess\n");

    ok(GetExitCodeThread(info.hThread, &exit_status) && exit_status == STILL_ACTIVE, "thread still running\n");
    Sleep(100);
    ok(GetExitCodeThread(info.hThread, &exit_status) && exit_status == STILL_ACTIVE, "thread still running\n");
    ok(ResumeThread(info.hThread) == 1, "Resuming thread\n");

    wait_and_close_child_process(&info);

    GetStartupInfoA(&us);

    reload_child_info(resfile);
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
    char                buffer[2 * MAX_PATH + 25];
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
    sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &startup, &info), "CreateProcess\n");

    /* get all startup events up to the entry point break exception */
    do 
    {
        ok(WaitForDebugEvent(&de, INFINITE), "reading debug event\n");
        ContinueDebugEvent(de.dwProcessId, de.dwThreadId, DBG_CONTINUE);
        if (!dbg)
        {
            ok(de.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT,
               "first event: %ld\n", de.dwDebugEventCode);
            processbase = de.u.CreateProcessInfo.lpBaseOfImage;
        }
        if (de.dwDebugEventCode != EXCEPTION_DEBUG_EVENT) dbg++;
        ok(de.dwDebugEventCode != LOAD_DLL_DEBUG_EVENT ||
           de.u.LoadDll.lpBaseOfDll != processbase, "got LOAD_DLL for main module\n");
    } while (de.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);

    ok(dbg, "I have seen a debug event\n");
    wait_and_close_child_process(&info);

    GetStartupInfoA(&us);

    reload_child_info(resfile);
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

static void test_Console(void)
{
    char                buffer[2 * MAX_PATH + 35];
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
    if (startup.hStdInput == INVALID_HANDLE_VALUE || startup.hStdOutput == INVALID_HANDLE_VALUE)
    {
        /* this fails either when this test process is run detached from console
         * (unlikely, as this very process must be explicitly created with detached flag),
         * or is attached to a Wine's shell-no-window kind of console (if the later, detach from it)
         */
        FreeConsole();
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
    sprintf(buffer, "\"%s\" process dump \"%s\" console", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &info), "CreateProcess\n");
    wait_and_close_child_process(&info);

    reload_child_info(resfile);
    /* now get the modification the child has made, and resets parents expected values */
    ok(GetConsoleScreenBufferInfo(startup.hStdOutput, &sbiC), "Getting sb info\n");
    ok(GetConsoleMode(startup.hStdInput, &modeInC), "Getting console in mode\n");
    ok(GetConsoleMode(startup.hStdOutput, &modeOutC), "Getting console out mode\n");

    SetConsoleMode(startup.hStdInput, modeIn);
    SetConsoleMode(startup.hStdOutput, modeOut);

    /* don't test flag that is changed at startup if WINETEST_COLOR is set */
    modeOut = (modeOut & ~ENABLE_VIRTUAL_TERMINAL_PROCESSING) |
              (modeOutC & ENABLE_VIRTUAL_TERMINAL_PROCESSING);

    cpInC = GetConsoleCP();
    cpOutC = GetConsoleOutputCP();

    /* Try to set invalid CP */
    SetLastError(0xdeadbeef);
    ok(!SetConsoleCP(0), "Shouldn't succeed\n");
    ok(GetLastError()==ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED), /* win9x */
       "GetLastError: expecting %u got %lu\n",
       ERROR_INVALID_PARAMETER, GetLastError());
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
        run_tests = FALSE;


    SetLastError(0xdeadbeef);
    ok(!SetConsoleOutputCP(0), "Shouldn't succeed\n");
    ok(GetLastError()==ERROR_INVALID_PARAMETER ||
       broken(GetLastError() == ERROR_CALL_NOT_IMPLEMENTED), /* win9x */
       "GetLastError: expecting %u got %lu\n",
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
        ok(cpInC == 1252, "Wrong console CP (expected 1252 got %ld/%ld)\n", cpInC, cpIn);
        ok(cpOutC == 1252, "Wrong console-SB CP (expected 1252 got %ld/%ld)\n", cpOutC, cpOut);
    }
    else
        win_skip("Setting the codepage is not implemented\n");

    ok(modeInC == (modeIn ^ 1), "Wrong console mode\n");
    ok(modeOutC == (modeOut ^ 1), "Wrong console-SB mode\n");

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
    sprintf(buffer, "\"%s\" process dump \"%s\" stdhandle", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, TRUE, DETACHED_PROCESS, NULL, NULL, &startup, &info), "CreateProcess\n");
    ok(CloseHandle(hChildInInh), "Closing handle\n");
    ok(CloseHandle(hChildOutInh), "Closing handle\n");

    msg_len = strlen(msg) + 1;
    ok(WriteFile(hParentOut, msg, msg_len, &w, NULL), "Writing to child\n");
    ok(w == msg_len, "Should have written %u bytes, actually wrote %lu\n", msg_len, w);
    memset(buffer, 0, sizeof(buffer));
    ok(ReadFile(hParentIn, buffer, sizeof(buffer), &w, NULL), "Reading from child\n");
    ok(strcmp(buffer, msg) == 0, "Should have received '%s'\n", msg);

    /* the child may also send the final "n tests executed" string, so read it to avoid a deadlock */
    ReadFile(hParentIn, buffer, sizeof(buffer), &w, NULL);

    wait_and_close_child_process(&info);

    reload_child_info(resfile);
    okChildString("StdHandle", "msg", msg);

    release_memory();
    DeleteFileA(resfile);
}

static  void    test_ExitCode(void)
{
    char                buffer[2 * MAX_PATH + 35];
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    DWORD               code;

    /* let's start simplistic */
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" process dump \"%s\" exit_code", selfname, resfile);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info), "CreateProcess\n");

    /* not wait_child_process() because of the exit code */
    ok(WaitForSingleObject(info.hProcess, 30000) == WAIT_OBJECT_0, "Child process termination\n");

    reload_child_info(resfile);
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

    /* without PROCESS_VM_OPERATION */
    hproc = OpenProcess(PROCESS_ALL_ACCESS_NT4 & ~PROCESS_VM_OPERATION, FALSE, GetCurrentProcessId());
    ok(hproc != NULL, "OpenProcess error %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    addr1 = VirtualAllocEx(hproc, 0, 0xFFFC, MEM_RESERVE, PAGE_NOACCESS);
    ok(!addr1, "VirtualAllocEx should fail\n");
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {   /* Win9x */
        CloseHandle(hproc);
        win_skip("VirtualAllocEx not implemented\n");
        return;
    }
    ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %ld\n", GetLastError());

    read_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadProcessMemory(hproc, test_OpenProcess, &dummy, sizeof(dummy), &read_bytes);
    ok(ret, "ReadProcessMemory error %ld\n", GetLastError());
    ok(read_bytes == sizeof(dummy), "wrong read bytes %Id\n", read_bytes);

    CloseHandle(hproc);

    hproc = OpenProcess(PROCESS_VM_OPERATION, FALSE, GetCurrentProcessId());
    ok(hproc != NULL, "OpenProcess error %ld\n", GetLastError());

    addr1 = VirtualAllocEx(hproc, 0, 0xFFFC, MEM_RESERVE, PAGE_NOACCESS);
    ok(addr1 != NULL, "VirtualAllocEx error %ld\n", GetLastError());

    /* without PROCESS_QUERY_INFORMATION */
    SetLastError(0xdeadbeef);
    ok(!VirtualQueryEx(hproc, addr1, &info, sizeof(info)),
       "VirtualQueryEx without PROCESS_QUERY_INFORMATION rights should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %ld\n", GetLastError());

    /* without PROCESS_VM_READ */
    read_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ok(!ReadProcessMemory(hproc, addr1, &dummy, sizeof(dummy), &read_bytes),
       "ReadProcessMemory without PROCESS_VM_READ rights should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %ld\n", GetLastError());
    ok(read_bytes == 0, "wrong read bytes %Id\n", read_bytes);

    CloseHandle(hproc);

    hproc = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, GetCurrentProcessId());

    memset(&info, 0xcc, sizeof(info));
    read_bytes = VirtualQueryEx(hproc, addr1, &info, sizeof(info));
    ok(read_bytes == sizeof(info), "VirtualQueryEx error %ld\n", GetLastError());

    ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
    ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
    ok(info.AllocationProtect == PAGE_NOACCESS, "%lx != PAGE_NOACCESS\n", info.AllocationProtect);
    ok(info.RegionSize == 0x10000, "%Ix != 0x10000\n", info.RegionSize);
    ok(info.State == MEM_RESERVE, "%lx != MEM_RESERVE\n", info.State);
    /* NT reports Protect == 0 for a not committed memory block */
    ok(info.Protect == 0 /* NT */ ||
       info.Protect == PAGE_NOACCESS, /* Win9x */
        "%lx != PAGE_NOACCESS\n", info.Protect);
    ok(info.Type == MEM_PRIVATE, "%lx != MEM_PRIVATE\n", info.Type);

    SetLastError(0xdeadbeef);
    ok(!VirtualFreeEx(hproc, addr1, 0, MEM_RELEASE),
       "VirtualFreeEx without PROCESS_VM_OPERATION rights should fail\n");
    ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %ld\n", GetLastError());

    CloseHandle(hproc);

    hproc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, GetCurrentProcessId());
    if (hproc)
    {
        SetLastError(0xdeadbeef);
        memset(&info, 0xcc, sizeof(info));
        read_bytes = VirtualQueryEx(hproc, addr1, &info, sizeof(info));
        if (read_bytes) /* win8 */
        {
            ok(read_bytes == sizeof(info), "VirtualQueryEx error %ld\n", GetLastError());
            ok(info.BaseAddress == addr1, "%p != %p\n", info.BaseAddress, addr1);
            ok(info.AllocationBase == addr1, "%p != %p\n", info.AllocationBase, addr1);
            ok(info.AllocationProtect == PAGE_NOACCESS, "%lx != PAGE_NOACCESS\n", info.AllocationProtect);
            ok(info.RegionSize == 0x10000, "%Ix != 0x10000\n", info.RegionSize);
            ok(info.State == MEM_RESERVE, "%lx != MEM_RESERVE\n", info.State);
            ok(info.Protect == 0, "%lx != PAGE_NOACCESS\n", info.Protect);
            ok(info.Type == MEM_PRIVATE, "%lx != MEM_PRIVATE\n", info.Type);
        }
        else /* before win8 */
            ok(broken(GetLastError() == ERROR_ACCESS_DENIED), "wrong error %ld\n", GetLastError());

        SetLastError(0xdeadbeef);
        ok(!VirtualFreeEx(hproc, addr1, 0, MEM_RELEASE),
           "VirtualFreeEx without PROCESS_VM_OPERATION rights should fail\n");
        ok(GetLastError() == ERROR_ACCESS_DENIED, "wrong error %ld\n", GetLastError());

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
    ok(ret, "GetProcessVersion error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetProcessVersion(GetCurrentProcessId());
    ok(ret, "GetProcessVersion error %lu\n", GetLastError());

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    si.dwFlags = STARTF_USESHOWWINDOW;
    si.wShowWindow = SW_HIDE;
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(NULL, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetProcessVersion(pi.dwProcessId);
    ok(ret, "GetProcessVersion error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = TerminateProcess(pi.hProcess, 0);
    ok(ret, "TerminateProcess error %lu\n", GetLastError());

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
       "K32GetProcessImageFileNameA(no buffer): returned %lu, le=%lu\n", rc, GetLastError());

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
    ok(size && GetLastError() != ERROR_INSUFFICIENT_BUFFER, "GetModuleFileName failed: %lu le=%lu\n", size, GetLastError());

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

    /* this is a difference between the ansi and the unicode version
     * the unicode version crashes when the size is big enough to hold
     * the result while the ansi version throws an error
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
    DWORD flags;

    if (!pQueryFullProcessImageNameW)
    {
        win_skip("QueryFullProcessImageNameW unavailable (added in Windows Vista)\n");
        return;
    }

    ok(GetModuleFileNameW(NULL, module_name, 1024), "GetModuleFileNameW(NULL, ...) failed\n");

    /* GetCurrentProcess pseudo-handle */
    size = ARRAY_SIZE(buf);
    expect_eq_d(TRUE, pQueryFullProcessImageNameW(GetCurrentProcess(), 0, buf, &size));
    expect_eq_d(lstrlenW(buf), size);
    expect_eq_ws_i(buf, module_name);

    hSelf = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, GetCurrentProcessId());
    /* Real handle */
    size = ARRAY_SIZE(buf);
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

    /* Invalid flags - a few arbitrary values only */
    for (flags = 2; flags <= 15; ++flags)
    {
        size = ARRAY_SIZE(buf);
        SetLastError(0xdeadbeef);
        *(DWORD*)buf = 0x13579acf;
        todo_wine
        {
        expect_eq_d(FALSE, pQueryFullProcessImageNameW(hSelf, flags, buf, &size));
        expect_eq_d((DWORD)ARRAY_SIZE(buf), size);  /* size not changed */
        expect_eq_d(ERROR_INVALID_PARAMETER, GetLastError());
        expect_eq_d(0x13579acf, *(DWORD*)buf);  /* buffer not changed */
        }
    }
    for (flags = 16; flags != 0; flags <<= 1)
    {
        size = ARRAY_SIZE(buf);
        SetLastError(0xdeadbeef);
        *(DWORD*)buf = 0x13579acf;
        todo_wine
        {
        expect_eq_d(FALSE, pQueryFullProcessImageNameW(hSelf, flags, buf, &size));
        expect_eq_d((DWORD)ARRAY_SIZE(buf), size);  /* size not changed */
        expect_eq_d(ERROR_INVALID_PARAMETER, GetLastError());
        expect_eq_d(0x13579acf, *(DWORD*)buf);  /* buffer not changed */
        }
    }

    /* native path */
    size = ARRAY_SIZE(buf);
    expect_eq_d(TRUE, pQueryFullProcessImageNameW(hSelf, PROCESS_NAME_NATIVE, buf, &size));
    expect_eq_d(lstrlenW(buf), size);
    ok(buf[0] == '\\', "NT path should begin with '\\'\n");
    ok(memcmp(buf, deviceW, sizeof(WCHAR)*lstrlenW(deviceW)) == 0, "NT path should begin with \\Device\n");

    module_name[2] = '\0';
    *device = '\0';
    size = QueryDosDeviceW(module_name, device, ARRAY_SIZE(device));
    ok(size, "QueryDosDeviceW failed: le=%lu\n", GetLastError());
    len = lstrlenW(device);
    ok(size >= len+2, "expected %ld to be greater than %ld+2 = strlen(%s)\n", size, len, wine_dbgstr_w(device));

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
    ok( ret, "GetExitCodeProcess failed err %lu\n", GetLastError() );
#ifdef _WIN64
    /* truncated handle */
    SetLastError( 0xdeadbeef );
    handle = (HANDLE)((ULONG_PTR)handle & ~0u);
    ret = GetExitCodeProcess( handle, &code );
    ok( !ret, "GetExitCodeProcess succeeded for %p\n", handle );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %lu\n", GetLastError() );
    /* sign-extended handle */
    SetLastError( 0xdeadbeef );
    handle = (HANDLE)((LONG_PTR)(int)(ULONG_PTR)handle);
    ret = GetExitCodeProcess( handle, &code );
    ok( ret, "GetExitCodeProcess failed err %lu\n", GetLastError() );
    /* invalid high-word */
    SetLastError( 0xdeadbeef );
    handle = (HANDLE)(((ULONG_PTR)handle & ~0u) + ((ULONG_PTR)1 << 32));
    ret = GetExitCodeProcess( handle, &code );
    ok( !ret, "GetExitCodeProcess succeeded for %p\n", handle );
    ok( GetLastError() == ERROR_INVALID_HANDLE, "wrong error %lu\n", GetLastError() );
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
        win_skip("IsWow64Process is not available\n");
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

static void test_IsWow64Process2(void)
{
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    BOOL ret, is_wow64;
    USHORT machine, native_machine;
    static char cmdline[] = "C:\\Program Files\\Internet Explorer\\iexplore.exe";
    static char cmdline_wow64[] = "C:\\Program Files (x86)\\Internet Explorer\\iexplore.exe";
#ifdef __i386__
    USHORT expect_native = IMAGE_FILE_MACHINE_I386;
#elif defined __x86_64__
    USHORT expect_native = IMAGE_FILE_MACHINE_AMD64;
#elif defined __arm__
    USHORT expect_native = IMAGE_FILE_MACHINE_ARMNT;
#elif defined __aarch64__
    USHORT expect_native = IMAGE_FILE_MACHINE_ARM64;
#else
    USHORT expect_native = 0;
#endif

    if (!pIsWow64Process2)
    {
        win_skip("IsWow64Process2 is not available\n");
        return;
    }

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(cmdline_wow64, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    if (ret)
    {
        SetLastError(0xdeadbeef);
        machine = native_machine = 0xdead;
        ret = pIsWow64Process2(pi.hProcess, &machine, &native_machine);
        ok(ret, "IsWow64Process2 error %lu\n", GetLastError());

#if defined(__i386__) || defined(__x86_64__)
        ok(machine == IMAGE_FILE_MACHINE_I386, "got %#x\n", machine);
        ok( native_machine == IMAGE_FILE_MACHINE_AMD64 ||
            native_machine == IMAGE_FILE_MACHINE_ARM64, "got %#x\n", native_machine);
        expect_native = native_machine;
#else
        skip("not supported architecture\n");
#endif
        ret = TerminateProcess(pi.hProcess, 0);
        ok(ret, "TerminateProcess error\n");

        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
    }

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    SetLastError(0xdeadbeef);
    ret = CreateProcessA(cmdline, NULL, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pIsWow64Process(pi.hProcess, &is_wow64);
    ok(ret, "IsWow64Process error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    machine = native_machine = 0xdead;
    ret = pIsWow64Process2(pi.hProcess, &machine, &native_machine);
    ok(ret, "IsWow64Process2 error %lu\n", GetLastError());

    ok(machine == IMAGE_FILE_MACHINE_UNKNOWN, "got %#x\n", machine);
    ok(native_machine == expect_native, "got %#x\n", native_machine);

    SetLastError(0xdeadbeef);
    machine = 0xdead;
    ret = pIsWow64Process2(pi.hProcess, &machine, NULL);
    ok(ret, "IsWow64Process2 error %lu\n", GetLastError());
    ok(machine == IMAGE_FILE_MACHINE_UNKNOWN, "got %#x\n", machine);

    ret = TerminateProcess(pi.hProcess, 0);
    ok(ret, "TerminateProcess error\n");

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    SetLastError(0xdeadbeef);
    ret = pIsWow64Process(GetCurrentProcess(), &is_wow64);
    ok(ret, "IsWow64Process error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    machine = native_machine = 0xdead;
    ret = pIsWow64Process2(GetCurrentProcess(), &machine, &native_machine);
    ok(ret, "IsWow64Process2 error %lu\n", GetLastError());

    if (is_wow64)
    {
        ok(machine == IMAGE_FILE_MACHINE_I386, "got %#x\n", machine);
        ok(native_machine == expect_native, "got %#x\n", native_machine);
    }
    else
    {
        ok(machine == IMAGE_FILE_MACHINE_UNKNOWN, "got %#x\n", machine);
        ok(native_machine == expect_native, "got %#x\n", native_machine);
    }

    SetLastError(0xdeadbeef);
    machine = 0xdead;
    ret = pIsWow64Process2(GetCurrentProcess(), &machine, NULL);
    ok(ret, "IsWow64Process2 error %lu\n", GetLastError());
    if (is_wow64)
        ok(machine == IMAGE_FILE_MACHINE_I386, "got %#x\n", machine);
    else
        ok(machine == IMAGE_FILE_MACHINE_UNKNOWN, "got %#x\n", machine);
}

static void test_SystemInfo(void)
{
    SYSTEM_INFO si, nsi;
    BOOL is_wow64;
    USHORT machine, native_machine;

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
        if (si.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_INTEL)
        {
            ok(nsi.wProcessorArchitecture == PROCESSOR_ARCHITECTURE_AMD64,
               "Expected PROCESSOR_ARCHITECTURE_AMD64, got %d\n",
               nsi.wProcessorArchitecture);
            if (pIsWow64Process2 && pIsWow64Process2(GetCurrentProcess(), &machine, &native_machine) &&
                native_machine == IMAGE_FILE_MACHINE_ARM64)
            {
                ok(nsi.dwProcessorType == PROCESSOR_INTEL_PENTIUM, "got %ld\n", nsi.dwProcessorType);
                ok(nsi.wProcessorLevel == 15, "got %d\n", nsi.wProcessorLevel);
                ok(nsi.wProcessorRevision == 0x40a, "got %d\n", nsi.wProcessorRevision);
            }
            else ok(nsi.dwProcessorType == PROCESSOR_AMD_X8664, "got %ld\n", nsi.dwProcessorType);
        }
    }
    else
    {
        ok(si.wProcessorArchitecture == nsi.wProcessorArchitecture,
           "Expected no difference for wProcessorArchitecture, got %d and %d\n",
           si.wProcessorArchitecture, nsi.wProcessorArchitecture);
        ok(si.dwProcessorType == nsi.dwProcessorType,
           "Expected no difference for dwProcessorType, got %ld and %ld\n",
           si.dwProcessorType, nsi.dwProcessorType);
    }
}

static void test_ProcessorCount(void)
{
    DWORD active, maximum;

    if (!pGetActiveProcessorCount || !pGetMaximumProcessorCount)
    {
        win_skip("GetActiveProcessorCount or GetMaximumProcessorCount is not available\n");
        return;
    }

    active = pGetActiveProcessorCount(ALL_PROCESSOR_GROUPS);
    maximum = pGetMaximumProcessorCount(ALL_PROCESSOR_GROUPS);
    ok(active <= maximum,
       "Number of active processors %li is greater than maximum number of processors %li\n",
       active, maximum);
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
    ok(ret, "CreateProcess error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    thread = CreateRemoteThread(pi.hProcess, NULL, 0, (void *)0xdeadbeef, NULL, CREATE_SUSPENDED, &ret);
    ok(thread != 0, "CreateRemoteThread error %ld\n", GetLastError());

    /* create a not closed thread handle duplicate in the target process */
    SetLastError(0xdeadbeef);
    ret = DuplicateHandle(GetCurrentProcess(), thread, pi.hProcess, &dummy,
                          0, FALSE, DUPLICATE_SAME_ACCESS);
    ok(ret, "DuplicateHandle error %lu\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = TerminateThread(thread, 0);
    ok(ret, "TerminateThread error %lu\n", GetLastError());
    CloseHandle(thread);

    SetLastError(0xdeadbeef);
    ret = TerminateProcess(pi.hProcess, 0);
    ok(ret, "TerminateProcess error %lu\n", GetLastError());

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
    ok(r, "DuplicateHandle error %lu\n", GetLastError());
    r = GetHandleInformation(out, &info);
    ok(r, "GetHandleInformation error %lu\n", GetLastError());
    ok(info == 0, "info = %lx\n", info);
    ok(out != GetCurrentProcess(), "out = GetCurrentProcess()\n");
    CloseHandle(out);

    r = DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(),
            GetCurrentProcess(), &out, 0, TRUE,
            DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    ok(r, "DuplicateHandle error %lu\n", GetLastError());
    r = GetHandleInformation(out, &info);
    ok(r, "GetHandleInformation error %lu\n", GetLastError());
    ok(info == HANDLE_FLAG_INHERIT, "info = %lx\n", info);
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
    ok(r, "DuplicateHandle error %lu\n", GetLastError());
    ok(f == out, "f != out\n");
    r = GetHandleInformation(out, &info);
    ok(r, "GetHandleInformation error %lu\n", GetLastError());
    ok(info == 0, "info = %lx\n", info);

    r = DuplicateHandle(GetCurrentProcess(), f, GetCurrentProcess(), &out,
            0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    ok(r, "DuplicateHandle error %lu\n", GetLastError());
    ok(f == out, "f != out\n");
    r = GetHandleInformation(out, &info);
    ok(r, "GetHandleInformation error %lu\n", GetLastError());
    ok(info == HANDLE_FLAG_INHERIT, "info = %lx\n", info);

    r = SetHandleInformation(f, HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE);
    ok(r, "SetHandleInformation error %lu\n", GetLastError());
    r = DuplicateHandle(GetCurrentProcess(), f, GetCurrentProcess(), &out,
                0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    ok(r, "DuplicateHandle error %lu\n", GetLastError());
    ok(f != out, "f == out\n");
    r = GetHandleInformation(out, &info);
    ok(r, "GetHandleInformation error %lu\n", GetLastError());
    ok(info == HANDLE_FLAG_INHERIT, "info = %lx\n", info);
    r = SetHandleInformation(f, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
    ok(r, "SetHandleInformation error %lu\n", GetLastError());

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
    ok(r, "DuplicateHandle error %lu\n", GetLastError());
    ok(f == out, "f != out\n");
    CloseHandle(out);
    DeleteFileA(file_name);

    f = CreateFileA("CONIN$", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(f != INVALID_HANDLE_VALUE, "Failed to open CONIN$ %lu\n", GetLastError());
    r = DuplicateHandle(GetCurrentProcess(), f, GetCurrentProcess(), &out,
            0, FALSE, DUPLICATE_SAME_ACCESS | DUPLICATE_CLOSE_SOURCE);
    ok(r, "DuplicateHandle error %lu\n", GetLastError());
    ok(f == out || broken(/* Win7 */ (((ULONG_PTR)f & 3) == 3) && (f != out)), "f != out\n");
    CloseHandle(out);

    /* Test DUPLICATE_SAME_ATTRIBUTES */
    f = CreateFileA("NUL", GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(f != INVALID_HANDLE_VALUE, "Failed to open NUL %lu\n", GetLastError());
    r = GetHandleInformation(f, &info);
    ok(r && info == 0, "Unexpected info %lx\n", info);

    r = DuplicateHandle(GetCurrentProcess(), f, GetCurrentProcess(), &out,
                        0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES);
    ok(r, "DuplicateHandle error %lu\n", GetLastError());
    r = GetHandleInformation(out, &info);
    ok(r && info == 0, "Unexpected info %lx\n", info);
    CloseHandle(out);

    r = SetHandleInformation(f, HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE,
                             HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE);
    ok(r, "SetHandleInformation error %lu\n", GetLastError());
    info = 0xdeabeef;
    r = GetHandleInformation(f, &info);
    ok(r && info == (HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE), "Unexpected info %lx\n", info);
    ok(r, "SetHandleInformation error %lu\n", GetLastError());
    r = DuplicateHandle(GetCurrentProcess(), f, GetCurrentProcess(), &out,
                        0, FALSE, DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES);
    ok(r, "DuplicateHandle error %lu\n", GetLastError());
    info = 0xdeabeef;
    r = GetHandleInformation(out, &info);
    ok(r && info == (HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE), "Unexpected info %lx\n", info);
    r = SetHandleInformation(out, HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
    ok(r, "SetHandleInformation error %lu\n", GetLastError());
    CloseHandle(out);
    r = SetHandleInformation(f, HANDLE_FLAG_INHERIT | HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
    ok(r, "SetHandleInformation error %lu\n", GetLastError());
    CloseHandle(f);

    r = DuplicateHandle(GetCurrentProcess(), GetCurrentProcess(), GetCurrentProcess(), &out,
                        0, TRUE, DUPLICATE_SAME_ACCESS | DUPLICATE_SAME_ATTRIBUTES);
    ok(r, "DuplicateHandle error %lu\n", GetLastError());
    info = 0xdeabeef;
    r = GetHandleInformation(out, &info);
    ok(r && info == 0, "Unexpected info %lx\n", info);
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

    ok_(__FILE__, line)(ret, "GetQueuedCompletionStatus: %lx\n", GetLastError());
    if (ret)
    {
        ok_(__FILE__, line)(key == ekey, "unexpected key %lx\n", key);
        ok_(__FILE__, line)(value == evalue, "unexpected value %p\n", (void *)value);
        ok_(__FILE__, line)(overlapped == (LPOVERLAPPED)eoverlapped, "unexpected overlapped %p\n", overlapped);
    }
}

#define create_process(cmd, pi) _create_process(__LINE__, cmd, pi)
static void _create_process(int line, const char *command, LPPROCESS_INFORMATION pi)
{
    BOOL ret;
    char buffer[MAX_PATH + 19];
    STARTUPINFOA si = {0};

    sprintf(buffer, "\"%s\" process %s", selfname, command);

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, pi);
    ok_(__FILE__, line)(ret, "CreateProcess error %lu\n", GetLastError());
}

#define test_assigned_proc(job, ...) _test_assigned_proc(__LINE__, job, __VA_ARGS__)
static void _test_assigned_proc(int line, HANDLE job, unsigned int count, ...)
{
    char buf[sizeof(JOBOBJECT_BASIC_PROCESS_ID_LIST) + sizeof(ULONG_PTR) * 20];
    JOBOBJECT_BASIC_PROCESS_ID_LIST *list = (JOBOBJECT_BASIC_PROCESS_ID_LIST *)buf;
    unsigned int i, pid;
    va_list valist;
    DWORD size;
    BOOL ret;

    memset(buf, 0, sizeof(buf));
    ret = pQueryInformationJobObject(job, JobObjectBasicProcessIdList, list, sizeof(buf), &size);
    ok_(__FILE__, line)(ret, "failed to get process id list, error %lu\n", GetLastError());

    ok_(__FILE__, line)(list->NumberOfAssignedProcesses == count,
                        "expected %u assigned processes, got %lu\n", count, list->NumberOfAssignedProcesses);
    ok_(__FILE__, line)(list->NumberOfProcessIdsInList == count,
                        "expected %u process IDs, got %lu\n", count, list->NumberOfProcessIdsInList);

    va_start(valist, count);
    for (i = 0; i < min(count, list->NumberOfProcessIdsInList); ++i)
    {
        pid = va_arg(valist, unsigned int);
        ok_(__FILE__, line)(pid == list->ProcessIdList[i],
                            "wrong pid %u: expected %#04x, got %#04Ix\n", i, pid, list->ProcessIdList[i]);
    }
    va_end(valist);
}

#define test_accounting(a, b, c, d) _test_accounting(__LINE__, a, b, c, d)
static void _test_accounting(int line, HANDLE job, unsigned int total, unsigned int active, unsigned int terminated)
{
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION info;
    DWORD size;
    BOOL ret;

    memset(&info, 0, sizeof(info));
    ret = pQueryInformationJobObject(job, JobObjectBasicAccountingInformation, &info, sizeof(info), &size);
    ok_(__FILE__, line)(ret, "failed to get accounting information, error %lu\n", GetLastError());

    ok_(__FILE__, line)(info.TotalProcesses == total,
                        "expected %u total processes, got %lu\n", total, info.TotalProcesses);
    ok_(__FILE__, line)(info.ActiveProcesses == active,
                        "expected %u active processes, got %lu\n", active, info.ActiveProcesses);
    ok_(__FILE__, line)(info.TotalTerminatedProcesses == terminated,
                        "expected %u terminated processes, got %lu\n", terminated, info.TotalTerminatedProcesses);
}

static void test_IsProcessInJob(void)
{
    HANDLE job, job2;
    PROCESS_INFORMATION pi;
    BOOL ret, out;

    if (!pIsProcessInJob)
    {
        win_skip("IsProcessInJob not available.\n");
        return;
    }

    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %lu\n", GetLastError());

    job2 = pCreateJobObjectW(NULL, NULL);
    ok(job2 != NULL, "CreateJobObject error %lu\n", GetLastError());

    create_process("wait", &pi);

    out = TRUE;
    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job, 0);
    test_accounting(job, 0, 0, 0);

    out = TRUE;
    ret = pIsProcessInJob(pi.hProcess, job2, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job2, 0);
    test_accounting(job2, 0, 0, 0);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job, 1, pi.dwProcessId);
    test_accounting(job, 1, 1, 0);

    out = TRUE;
    ret = pIsProcessInJob(pi.hProcess, job2, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job2, 0);
    test_accounting(job2, 0, 0, 0);

    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, NULL, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    TerminateProcess(pi.hProcess, 0);
    wait_child_process(pi.hProcess);

    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
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
    ok(job != NULL, "CreateJobObject error %lu\n", GetLastError());
    test_assigned_proc(job, 0);
    test_accounting(job, 0, 0, 0);

    create_process("wait", &pi);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());
    test_assigned_proc(job, 1, pi.dwProcessId);
    test_accounting(job, 1, 1, 0);

    ret = pTerminateJobObject(job, 123);
    ok(ret, "TerminateJobObject error %lu\n", GetLastError());

    /* not wait_child_process() because of the exit code */
    dwret = WaitForSingleObject(pi.hProcess, 1000);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", dwret);
    if (dwret == WAIT_TIMEOUT) TerminateProcess(pi.hProcess, 0);
    test_assigned_proc(job, 0);
    test_accounting(job, 1, 0, 0);

    ret = GetExitCodeProcess(pi.hProcess, &dwret);
    ok(ret, "GetExitCodeProcess error %lu\n", GetLastError());
    ok(dwret == 123 || broken(dwret == 0) /* randomly fails on Win 2000 / XP */,
       "wrong exitcode %lu\n", dwret);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    /* Test adding an already terminated process to a job object */
    create_process("exit", &pi);
    wait_child_process(pi.hProcess);

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
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION basic_accounting_info;
    DWORD ret_len;
    PROCESS_INFORMATION pi[2];
    char buffer[50];
    HANDLE job, sem;
    BOOL ret;

    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %lu\n", GetLastError());

    /* Only active processes are returned */
    sprintf(buffer, "sync kernel32-process-%lx", GetCurrentProcessId());
    sem = CreateSemaphoreA(NULL, 0, 1, buffer + 5);
    ok(sem != NULL, "CreateSemaphoreA failed le=%lu\n", GetLastError());
    create_process(buffer, &pi[0]);

    ret = pAssignProcessToJobObject(job, pi[0].hProcess);
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

    ReleaseSemaphore(sem, 1, NULL);
    wait_and_close_child_process(&pi[0]);

    create_process("wait", &pi[0]);
    ret = pAssignProcessToJobObject(job, pi[0].hProcess);
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

    create_process("wait", &pi[1]);
    ret = pAssignProcessToJobObject(job, pi[1].hProcess);
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

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
    ok(!ret, "QueryInformationJobObject expected failure\n");
    expect_eq_d(ERROR_MORE_DATA, GetLastError());
    if (ret)
    {
        expect_eq_d(42, pid_list->NumberOfAssignedProcesses);
        expect_eq_d(42, pid_list->NumberOfProcessIdsInList);
    }

    memset(buf, 0, sizeof(buf));
    ret = pQueryInformationJobObject(job, JobObjectBasicProcessIdList, pid_list, sizeof(buf), &ret_len);
    ok(ret, "QueryInformationJobObject error %lu\n", GetLastError());
    if(ret)
    {
        if (pid_list->NumberOfAssignedProcesses == 3) /* Win 8 */
            win_skip("Number of assigned processes broken on Win 8\n");
        else
        {
            ULONG_PTR *list = pid_list->ProcessIdList;

            ok(ret_len == FIELD_OFFSET(JOBOBJECT_BASIC_PROCESS_ID_LIST, ProcessIdList[2]),
               "QueryInformationJobObject returned ret_len=%lu\n", ret_len);

            expect_eq_d(2, pid_list->NumberOfAssignedProcesses);
            expect_eq_d(2, pid_list->NumberOfProcessIdsInList);
            expect_eq_d(pi[0].dwProcessId, list[0]);
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
    ok(ret, "QueryInformationJobObject error %lu\n", GetLastError());
    ok(ret_len == sizeof(*basic_limit_info), "QueryInformationJobObject returned ret_len=%lu\n", ret_len);
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
    ok(ret, "QueryInformationJobObject error %lu\n", GetLastError());
    ok(ret_len == sizeof(ext_limit_info), "QueryInformationJobObject returned ret_len=%lu\n", ret_len);
    expect_eq_d(0, basic_limit_info->LimitFlags);

    /* test JobObjectBasicAccountingInformation */
    ret = pQueryInformationJobObject(job, JobObjectBasicAccountingInformation, &basic_accounting_info,
                                     sizeof(basic_accounting_info), &ret_len);
    ok(ret, "QueryInformationJobObject error %lu\n", GetLastError());
    ok(ret_len == sizeof(basic_accounting_info), "QueryInformationJobObject returned ret_len=%lu\n", ret_len);
    expect_eq_d(3, basic_accounting_info.TotalProcesses);
    expect_eq_d(2, basic_accounting_info.ActiveProcesses);

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
    PROCESS_INFORMATION pi, pi2;
    HANDLE job, port;
    BOOL ret;

    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %lu\n", GetLastError());

    create_process("wait", &pi2);
    ret = pAssignProcessToJobObject(job, pi2.hProcess);
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

    port = pCreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    ok(port != NULL, "CreateIoCompletionPort error %lu\n", GetLastError());

    port_info.CompletionKey = job;
    port_info.CompletionPort = port;
    ret = pSetInformationJobObject(job, JobObjectAssociateCompletionPortInformation, &port_info, sizeof(port_info));
    ok(ret, "SetInformationJobObject error %lu\n", GetLastError());

    create_process("wait", &pi);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

    test_completion(port, JOB_OBJECT_MSG_NEW_PROCESS, (DWORD_PTR)job, pi2.dwProcessId, 0);
    test_completion(port, JOB_OBJECT_MSG_NEW_PROCESS, (DWORD_PTR)job, pi.dwProcessId, 0);

    TerminateProcess(pi.hProcess, 0);
    wait_child_process(pi.hProcess);

    test_completion(port, JOB_OBJECT_MSG_EXIT_PROCESS, (DWORD_PTR)job, pi.dwProcessId, 0);
    TerminateProcess(pi2.hProcess, 0);
    wait_child_process(pi2.hProcess);
    CloseHandle(pi2.hProcess);
    CloseHandle(pi2.hThread);

    test_completion(port, JOB_OBJECT_MSG_EXIT_PROCESS, (DWORD_PTR)job, pi2.dwProcessId, 0);
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
    ok(job != NULL, "CreateJobObject error %lu\n", GetLastError());

    limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_KILL_ON_JOB_CLOSE;
    ret = pSetInformationJobObject(job, JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info));
    if (!ret && GetLastError() == ERROR_INVALID_PARAMETER)
    {
        win_skip("Kill on job close limit not available\n");
        return;
    }
    ok(ret, "SetInformationJobObject error %lu\n", GetLastError());
    test_assigned_proc(job, 0);
    test_accounting(job, 0, 0, 0);

    create_process("wait", &pi);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());
    test_assigned_proc(job, 1, pi.dwProcessId);
    test_accounting(job, 1, 1, 0);

    CloseHandle(job);

    /* not wait_child_process() for the kill */
    dwret = WaitForSingleObject(pi.hProcess, 1000);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", dwret);
    if (dwret == WAIT_TIMEOUT) TerminateProcess(pi.hProcess, 0);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

static void test_WaitForJobObject(void)
{
    HANDLE job, sem;
    char buffer[50];
    PROCESS_INFORMATION pi;
    BOOL ret;
    DWORD dwret;

    /* test waiting for a job object when the process is killed */
    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %lu\n", GetLastError());

    dwret = WaitForSingleObject(job, 100);
    ok(dwret == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", dwret);

    create_process("wait", &pi);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

    dwret = WaitForSingleObject(job, 100);
    ok(dwret == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", dwret);

    ret = pTerminateJobObject(job, 123);
    ok(ret, "TerminateJobObject error %lu\n", GetLastError());

    dwret = WaitForSingleObject(job, 500);
    ok(dwret == WAIT_OBJECT_0 || broken(dwret == WAIT_TIMEOUT),
       "WaitForSingleObject returned %lu\n", dwret);

    if (dwret == WAIT_TIMEOUT) /* Win 2000/XP */
    {
        CloseHandle(pi.hProcess);
        CloseHandle(pi.hThread);
        CloseHandle(job);
        win_skip("TerminateJobObject doesn't signal job, skipping tests\n");
        return;
    }

    /* the object is not reset immediately */
    dwret = WaitForSingleObject(job, 100);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", dwret);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    /* creating a new process doesn't reset the signalled state */
    create_process("wait", &pi);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

    dwret = WaitForSingleObject(job, 100);
    ok(dwret == WAIT_OBJECT_0, "WaitForSingleObject returned %lu\n", dwret);

    ret = pTerminateJobObject(job, 123);
    ok(ret, "TerminateJobObject error %lu\n", GetLastError());

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    CloseHandle(job);

    /* repeat the test, but this time the process terminates properly */
    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %lu\n", GetLastError());

    dwret = WaitForSingleObject(job, 100);
    ok(dwret == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", dwret);

    sprintf(buffer, "sync kernel32-process-%lx", GetCurrentProcessId());
    sem = CreateSemaphoreA(NULL, 0, 1, buffer + 5);
    ok(sem != NULL, "CreateSemaphoreA failed le=%lu\n", GetLastError());
    create_process(buffer, &pi);

    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());
    ReleaseSemaphore(sem, 1, NULL);

    dwret = WaitForSingleObject(job, 100);
    ok(dwret == WAIT_TIMEOUT, "WaitForSingleObject returned %lu\n", dwret);

    wait_and_close_child_process(&pi);
    CloseHandle(job);
    CloseHandle(sem);
}

static HANDLE test_AddSelfToJob(void)
{
    HANDLE job;
    BOOL ret;

    job = pCreateJobObjectW(NULL, NULL);
    ok(job != NULL, "CreateJobObject error %lu\n", GetLastError());

    ret = pAssignProcessToJobObject(job, GetCurrentProcess());
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());
    test_assigned_proc(job, 1, GetCurrentProcessId());
    test_accounting(job, 1, 1, 0);

    return job;
}

static void test_jobInheritance(HANDLE job)
{
    PROCESS_INFORMATION pi;
    BOOL ret, out;

    if (!pIsProcessInJob)
    {
        win_skip("IsProcessInJob not available.\n");
        return;
    }

    create_process("exit", &pi);

    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);
    test_assigned_proc(job, 2, GetCurrentProcessId(), pi.dwProcessId);
    test_accounting(job, 2, 2, 0);

    wait_and_close_child_process(&pi);
}

static void test_BreakawayOk(HANDLE parent_job)
{
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit_info;
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = {0};
    char buffer[MAX_PATH + 23];
    BOOL ret, out, nested_jobs;
    HANDLE job;

    if (!pIsProcessInJob)
    {
        win_skip("IsProcessInJob not available.\n");
        return;
    }

    job = pCreateJobObjectW(NULL, NULL);
    ok(!!job, "CreateJobObjectW error %lu\n", GetLastError());

    ret = pAssignProcessToJobObject(job, GetCurrentProcess());
    ok(ret || broken(!ret && GetLastError() == ERROR_ACCESS_DENIED) /* before Win 8. */,
            "AssignProcessToJobObject error %lu\n", GetLastError());
    nested_jobs = ret;
    if (!ret)
        win_skip("Nested jobs are not supported.\n");

    sprintf(buffer, "\"%s\" process exit", selfname);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);
    ok(!ret, "CreateProcessA expected failure\n");
    expect_eq_d(ERROR_ACCESS_DENIED, GetLastError());

    if (ret)
    {
        TerminateProcess(pi.hProcess, 0);
        wait_and_close_child_process(&pi);
    }

    if (nested_jobs)
    {
        test_assigned_proc(job, 1, GetCurrentProcessId());
        test_accounting(job, 1, 1, 0);

        limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK;
        ret = pSetInformationJobObject(job, JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info));
        ok(ret, "SetInformationJobObject error %lu\n", GetLastError());

        sprintf(buffer, "\"%s\" process exit", selfname);
        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);
        ok(ret, "CreateProcessA error %lu\n", GetLastError());

        ret = pIsProcessInJob(pi.hProcess, job, &out);
        ok(ret, "IsProcessInJob error %lu\n", GetLastError());
        ok(!out, "IsProcessInJob returned out=%u\n", out);

        ret = pIsProcessInJob(pi.hProcess, parent_job, &out);
        ok(ret, "IsProcessInJob error %lu\n", GetLastError());
        ok(out, "IsProcessInJob returned out=%u\n", out);

        TerminateProcess(pi.hProcess, 0);
        wait_and_close_child_process(&pi);
    }

    limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK;
    ret = pSetInformationJobObject(parent_job, JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info));
    ok(ret, "SetInformationJobObject error %lu\n", GetLastError());

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcessA error %lu\n", GetLastError());

    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);
    if (nested_jobs)
    {
        test_assigned_proc(job, 1, GetCurrentProcessId());
        test_accounting(job, 1, 1, 0);
    }

    ret = pIsProcessInJob(pi.hProcess, parent_job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);

    wait_and_close_child_process(&pi);

    limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
    ret = pSetInformationJobObject(job, JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info));
    ok(ret, "SetInformationJobObject error %lu\n", GetLastError());

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess error %lu\n", GetLastError());

    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);
    if (nested_jobs)
    {
        test_assigned_proc(job, 1, GetCurrentProcessId());
        test_accounting(job, 1, 1, 0);
    }

    wait_and_close_child_process(&pi);

    /* unset breakaway ok */
    limit_info.BasicLimitInformation.LimitFlags = 0;
    ret = pSetInformationJobObject(job, JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info));
    ok(ret, "SetInformationJobObject error %lu\n", GetLastError());
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

#define H_CONSOLE  0
#define H_DISK     1
#define H_CHAR     2
#define H_PIPE     3
#define H_NULL     4
#define H_INVALID  5
#define H_DEVIL    6 /* unassigned handle */

#define ARG_STD                 0x80000000
#define ARG_STARTUPINFO         0x00000000
#define ARG_CP_INHERIT          0x40000000
#define ARG_HANDLE_INHERIT      0x20000000
#define ARG_HANDLE_PROTECT      0x10000000
#define ARG_HANDLE_MASK         (~0xff000000)

static  BOOL check_run_child(const char *exec, DWORD flags, BOOL cp_inherit,
                             STARTUPINFOA *si)
{
    PROCESS_INFORMATION info;
    char buffer[2 * MAX_PATH + 64];
    DWORD exit_code;
    BOOL res;
    DWORD ret;

    get_file_name(resfile);
    sprintf(buffer, "\"%s\" process dump \"%s\"", exec, resfile);

    res = CreateProcessA(NULL, buffer, NULL, NULL, cp_inherit, flags, NULL, NULL, si, &info);
    ok(res, "CreateProcess failed: %lu %s\n", GetLastError(), buffer);
    CloseHandle(info.hThread);
    ret = WaitForSingleObject(info.hProcess, 30000);
    ok(ret == WAIT_OBJECT_0, "Could not wait for the child process: %ld le=%lu\n",
        ret, GetLastError());
    res = GetExitCodeProcess(info.hProcess, &exit_code);
    ok(res && exit_code == 0, "Couldn't get exit_code\n");
    CloseHandle(info.hProcess);
    return res;
}

static char std_handle_file[MAX_PATH];

static BOOL build_startupinfo( STARTUPINFOA *startup, unsigned args, HANDLE hstd[2] )
{
    SECURITY_ATTRIBUTES inherit_sa = { sizeof(inherit_sa), NULL, TRUE };
    SECURITY_ATTRIBUTES *psa;
    BOOL ret, needs_close = FALSE;

    psa = (args & ARG_HANDLE_INHERIT) ? &inherit_sa : NULL;

    memset(startup, 0, sizeof(*startup));
    startup->cb = sizeof(*startup);

    switch (args & ARG_HANDLE_MASK)
    {
    case H_CONSOLE:
        hstd[0] = CreateFileA("CONIN$", GENERIC_READ, 0, psa, OPEN_EXISTING, 0, 0);
        ok(hstd[0] != INVALID_HANDLE_VALUE, "Couldn't create input to console\n");
        hstd[1] = CreateFileA("CONOUT$", GENERIC_READ|GENERIC_WRITE, 0, psa, OPEN_EXISTING, 0, 0);
        ok(hstd[1] != INVALID_HANDLE_VALUE, "Couldn't create input to console\n");
        needs_close = TRUE;
        break;
    case H_DISK:
        hstd[0] = CreateFileA(std_handle_file, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, psa, OPEN_EXISTING, 0, 0);
        ok(hstd[0] != INVALID_HANDLE_VALUE, "Couldn't create input to file %s\n", std_handle_file);
        hstd[1] = CreateFileA(std_handle_file, GENERIC_READ|GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, psa, OPEN_EXISTING, 0, 0);
        ok(hstd[1] != INVALID_HANDLE_VALUE, "Couldn't create input to file %s\n", std_handle_file);
        needs_close = TRUE;
        break;
    case H_CHAR:
        hstd[0] = CreateFileA("NUL", GENERIC_READ, 0, psa, OPEN_EXISTING, 0, 0);
        ok(hstd[0] != INVALID_HANDLE_VALUE, "Couldn't create input to NUL\n");
        hstd[1] = CreateFileA("NUL", GENERIC_READ|GENERIC_WRITE, 0, psa, OPEN_EXISTING, 0, 0);
        ok(hstd[1] != INVALID_HANDLE_VALUE, "Couldn't create input to NUL\n");
        needs_close = TRUE;
        break;
    case H_PIPE:
        ret = CreatePipe(&hstd[0], &hstd[1], psa, 0);
        ok(ret, "Couldn't create anon pipe\n");
        needs_close = TRUE;
        break;
    case H_NULL:
        hstd[0] = hstd[1] = NULL;
        break;
    case H_INVALID:
        hstd[0] = hstd[1] = INVALID_HANDLE_VALUE;
        break;
    case H_DEVIL:
        hstd[0] = (HANDLE)(ULONG_PTR)0x066600;
        hstd[1] = (HANDLE)(ULONG_PTR)0x066610;
        break;
    default:
        ok(0, "Unsupported handle type %x\n", args & ARG_HANDLE_MASK);
        return FALSE;
    }
    if ((args & ARG_HANDLE_PROTECT) && needs_close)
    {
        ret = SetHandleInformation(hstd[0], HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE);
        ok(ret, "Couldn't set inherit flag to hstd[0]\n");
        ret = SetHandleInformation(hstd[1], HANDLE_FLAG_PROTECT_FROM_CLOSE, HANDLE_FLAG_PROTECT_FROM_CLOSE);
        ok(ret, "Couldn't set inherit flag to hstd[1]\n");
    }

    if (args & ARG_STD)
    {
        SetStdHandle(STD_INPUT_HANDLE,  hstd[0]);
        SetStdHandle(STD_OUTPUT_HANDLE, hstd[1]);
    }
    else /* through startup info */
    {
        startup->dwFlags |= STARTF_USESTDHANDLES;
        startup->hStdInput  = hstd[0];
        startup->hStdOutput = hstd[1];
    }
    return needs_close;
}

struct std_handle_test
{
    /* input */
    unsigned args;
    /* output */
    DWORD expected;
    DWORD is_broken; /* Win7 broken file types */
};

static void test_StdHandleInheritance(void)
{
    HANDLE hsavestd[3];
    static char guiexec[MAX_PATH];
    static char cuiexec[MAX_PATH];
    char **argv;
    BOOL ret;
    int i, j;

    static const struct std_handle_test
    nothing_cui[] =
    {
        /* all others handles type behave as H_DISK */
/* 0*/  {ARG_STARTUPINFO | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_DISK,      HATTR_TYPE | HATTR_INHERIT | FILE_TYPE_DISK},
        {ARG_STD         | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_DISK,      HATTR_TYPE | HATTR_INHERIT | FILE_TYPE_DISK},

        /* all others handles type behave as H_DISK */
        {ARG_STARTUPINFO |                  ARG_HANDLE_INHERIT | H_DISK,      HATTR_NULL, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},
        {ARG_STD         |                  ARG_HANDLE_INHERIT | H_DISK,      HATTR_TYPE | HATTR_INHERIT | FILE_TYPE_DISK},

        /* all others handles type behave as H_DISK */
        {ARG_STARTUPINFO |                                       H_DISK,      HATTR_NULL, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},
/* 5*/  {ARG_STD         |                                       H_DISK,      HATTR_TYPE | FILE_TYPE_DISK},

        /* all others handles type behave as H_DISK */
        {ARG_STARTUPINFO |                  ARG_HANDLE_PROTECT | H_DISK,      HATTR_NULL, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},
        {ARG_STD         |                  ARG_HANDLE_PROTECT | H_DISK,      HATTR_TYPE | HATTR_PROTECT | FILE_TYPE_DISK},

        /* all others handles type behave as H_DISK */
        {ARG_STARTUPINFO | ARG_CP_INHERIT |                      H_DISK,      HATTR_DANGLING, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},
        {ARG_STD         | ARG_CP_INHERIT |                      H_DISK,      HATTR_DANGLING},

        /* all others handles type behave as H_DISK */
/*10*/  {ARG_STARTUPINFO |                                       H_DEVIL,     HATTR_NULL, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},
        {ARG_STD         |                                       H_DEVIL,     HATTR_NULL},
        {ARG_STARTUPINFO |                                       H_INVALID,   HATTR_NULL, .is_broken = HATTR_INVALID},
        {ARG_STD         |                                       H_INVALID,   HATTR_NULL, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},
        {ARG_STARTUPINFO |                                       H_NULL,      HATTR_NULL, .is_broken = HATTR_INVALID},
/*15*/  {ARG_STD         |                                       H_NULL,      HATTR_NULL, .is_broken = HATTR_INVALID},
    },
    nothing_gui[] =
    {
        /* testing all types because of discrepancies */
/* 0*/  {ARG_STARTUPINFO | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_DISK,      HATTR_TYPE | HATTR_INHERIT | FILE_TYPE_DISK},
        {ARG_STD         | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_DISK,      HATTR_TYPE | HATTR_INHERIT | FILE_TYPE_DISK},
        {ARG_STARTUPINFO | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_PIPE,      HATTR_TYPE | HATTR_INHERIT | FILE_TYPE_PIPE},
        {ARG_STD         | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_PIPE,      HATTR_TYPE | HATTR_INHERIT | FILE_TYPE_PIPE},
        {ARG_STARTUPINFO | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_CHAR,      HATTR_TYPE | HATTR_INHERIT | FILE_TYPE_CHAR},
/* 5*/  {ARG_STD         | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_CHAR,      HATTR_TYPE | HATTR_INHERIT | FILE_TYPE_CHAR},
        {ARG_STARTUPINFO | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_CONSOLE,   HATTR_NULL, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},
        {ARG_STD         | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_CONSOLE,   HATTR_NULL, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},

        /* all others handles type behave as H_DISK */
        {ARG_STARTUPINFO |                  ARG_HANDLE_INHERIT | H_DISK,      HATTR_NULL, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},
        {ARG_STD         |                  ARG_HANDLE_INHERIT | H_DISK,      HATTR_NULL},

        /* all others handles type behave as H_DISK */
/*10*/  {ARG_STARTUPINFO | ARG_CP_INHERIT |                      H_DISK,      HATTR_DANGLING},
        {ARG_STD         | ARG_CP_INHERIT |                      H_DISK,      HATTR_DANGLING},

        /* all others handles type behave as H_DISK */
        {ARG_STARTUPINFO |                                       H_DISK,      HATTR_NULL, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},
        {ARG_STD         |                                       H_DISK,      HATTR_NULL},

        {ARG_STARTUPINFO |                                       H_DEVIL,     HATTR_NULL, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},
/*15*/  {ARG_STD         |                                       H_DEVIL,     HATTR_NULL},
        {ARG_STARTUPINFO |                                       H_INVALID,   HATTR_NULL, .is_broken = HATTR_INVALID},
        {ARG_STD         |                                       H_INVALID,   HATTR_NULL},
        {ARG_STARTUPINFO |                                       H_NULL,      HATTR_NULL},
        {ARG_STD         |                                       H_NULL,      HATTR_NULL},
    },
    detached_cui[] =
    {
        {ARG_STD         | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_CONSOLE,  HATTR_NULL},
        {ARG_STARTUPINFO | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_CONSOLE,  HATTR_TYPE | HATTR_INHERIT | FILE_TYPE_CHAR, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},
        /* all others handles type behave as H_DISK */
        {ARG_STD         | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_DISK,     HATTR_NULL},
        {ARG_STARTUPINFO | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_DISK,     HATTR_TYPE | HATTR_INHERIT | FILE_TYPE_DISK},
    },
    detached_gui[] =
    {
        {ARG_STD         | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_CONSOLE,  HATTR_NULL},
        {ARG_STARTUPINFO | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_CONSOLE,  HATTR_NULL, .is_broken = HATTR_TYPE | FILE_TYPE_UNKNOWN},
        /* all others handles type behave as H_DISK */
        {ARG_STD         | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_DISK,     HATTR_NULL},
        {ARG_STARTUPINFO | ARG_CP_INHERIT | ARG_HANDLE_INHERIT | H_DISK,     HATTR_TYPE | HATTR_INHERIT | FILE_TYPE_DISK},
    };
    static const struct
    {
        DWORD cp_flags;
        BOOL use_cui;
        const struct std_handle_test* tests;
        size_t count;
        const char* descr;
    }
    tests[] =
    {
#define X(d, cg, s) {(d), (cg), s, ARRAY_SIZE(s), #s}
        X(0,                TRUE,  nothing_cui),
        X(0,                FALSE, nothing_gui),
        X(DETACHED_PROCESS, TRUE,  detached_cui),
        X(DETACHED_PROCESS, FALSE, detached_gui),
#undef X
    };

    hsavestd[0] = GetStdHandle(STD_INPUT_HANDLE);
    hsavestd[1] = GetStdHandle(STD_OUTPUT_HANDLE);
    hsavestd[2] = GetStdHandle(STD_ERROR_HANDLE);

    winetest_get_mainargs(&argv);

    GetTempPathA(ARRAY_SIZE(guiexec), guiexec);
    strcat(guiexec, "process_gui.exe");
    copy_change_subsystem(argv[0], guiexec, IMAGE_SUBSYSTEM_WINDOWS_GUI);
    GetTempPathA(ARRAY_SIZE(cuiexec), cuiexec);
    strcat(cuiexec, "process_cui.exe");
    copy_change_subsystem(argv[0], cuiexec, IMAGE_SUBSYSTEM_WINDOWS_CUI);
    get_file_name(std_handle_file);

    for (j = 0; j < ARRAY_SIZE(tests); j++)
    {
        const struct std_handle_test* std_tests = tests[j].tests;

        for (i = 0; i < tests[j].count; i++)
        {
            STARTUPINFOA startup;
#if defined(__REACTOS__) && _MSC_VER < 1930
            HANDLE hstd[2] = {0};
#else
            HANDLE hstd[2] = {};
#endif
            BOOL needs_close;

            winetest_push_context("%s[%u] ", tests[j].descr, i);
            needs_close = build_startupinfo( &startup, std_tests[i].args, hstd );

            ret = check_run_child(tests[j].use_cui ? cuiexec : guiexec,
                                  tests[j].cp_flags, !!(std_tests[i].args & ARG_CP_INHERIT),
                                  &startup);
            ok(ret, "Couldn't run child\n");
            reload_child_info(resfile);

            if (std_tests[i].expected & HATTR_DANGLING)
            {
                /* The value of the handle (in parent) has been copied in STARTUPINFO fields (in child),
                 * but the object hasn't been inherited from parent to child.
                 * There's no reliable way to test that the object hasn't been inherited, as the
                 * entry in the child's handle table is free and could have been reused before
                 * this test occurs.
                 * So simply test that the value is passed untouched.
                 */
                okChildHexInt("StartupInfoA", "hStdInput", (DWORD_PTR)((std_tests[i].args & ARG_STD) ? INVALID_HANDLE_VALUE : hstd[0]), std_tests[i].is_broken);
                okChildHexInt("StartupInfoA", "hStdOutput", (DWORD_PTR)((std_tests[i].args & ARG_STD) ? INVALID_HANDLE_VALUE : hstd[1]), std_tests[i].is_broken);
                if (!(std_tests[i].args & ARG_STD))
                {
                    okChildHexInt("StartupInfoW", "hStdInput", (DWORD_PTR)hstd[0], std_tests[i].is_broken);
                    okChildHexInt("StartupInfoW", "hStdOutput", (DWORD_PTR)hstd[1], std_tests[i].is_broken);
                }

                okChildHexInt("TEB", "hStdInput", (DWORD_PTR)hstd[0], std_tests[i].is_broken);
                okChildHexInt("TEB", "hStdOutput", (DWORD_PTR)hstd[1], std_tests[i].is_broken);
            }
            else
            {
                unsigned startup_expected = (std_tests[i].args & ARG_STD) ? HATTR_INVALID : std_tests[i].expected;

                okChildHexInt("StartupInfoA", "hStdInputEncode", startup_expected, std_tests[i].is_broken);
                okChildHexInt("StartupInfoA", "hStdOutputEncode", startup_expected, std_tests[i].is_broken);

                startup_expected = (std_tests[i].args & ARG_STD) ? HATTR_UNTOUCHED : std_tests[i].expected;

                okChildHexInt("StartupInfoW", "hStdInputEncode", startup_expected, std_tests[i].is_broken);
                okChildHexInt("StartupInfoW", "hStdOutputEncode", startup_expected, std_tests[i].is_broken);

                okChildHexInt("TEB", "hStdInputEncode", std_tests[i].expected, std_tests[i].is_broken);
                okChildHexInt("TEB", "hStdOutputEncode", std_tests[i].expected, std_tests[i].is_broken);
            }

            release_memory();
            DeleteFileA(resfile);
            if (needs_close)
            {
                SetHandleInformation(hstd[0], HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
                CloseHandle(hstd[0]);
                SetHandleInformation(hstd[1], HANDLE_FLAG_PROTECT_FROM_CLOSE, 0);
                CloseHandle(hstd[1]);
            }
            winetest_pop_context();
        }
    }

    DeleteFileA(guiexec);
    DeleteFileA(cuiexec);
    DeleteFileA(std_handle_file);

    SetStdHandle(STD_INPUT_HANDLE,  hsavestd[0]);
    SetStdHandle(STD_OUTPUT_HANDLE, hsavestd[1]);
    SetStdHandle(STD_ERROR_HANDLE,  hsavestd[2]);
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
    ok(ret, "Failed to read remote module IID (%ld)\n", GetLastError());

    /* Validate the IID is present and not a bound import, and that we have
       an OriginalFirstThunk to compare with */
    ok(iid.Name, "Module first IID does not have a Name\n");
    ok(iid.FirstThunk, "Module first IID does not have a FirstThunk\n");
    ok(!iid.TimeDateStamp, "Module first IID is a bound import (UNSUPPORTED for current test)\n");
    ok(iid.OriginalFirstThunk, "Module first IID does not have an OriginalFirstThunk (UNSUPPORTED for current test)\n");

    /* Read a single IAT entry from the FirstThunk */
    ret = ReadProcessMemory(process_handle, (char *)module_base + iid.FirstThunk,
                            &iat_entry_value, sizeof(iat_entry_value), NULL);
    ok(ret, "Failed to read IAT entry from FirstThunk (%ld)\n", GetLastError());
    ok(iat_entry_value, "IAT entry in FirstThunk is NULL\n");

    /* Read a single IAT entry from the OriginalFirstThunk */
    ret = ReadProcessMemory(process_handle, (char *)module_base + iid.OriginalFirstThunk,
                            &orig_iat_entry_value, sizeof(orig_iat_entry_value), NULL);
    ok(ret, "Failed to read IAT entry from OriginalFirstThunk (%ld)\n", GetLastError());
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
    ok(ret, "Failed to create process (%ld)\n", GetLastError());

    exe_base = get_process_exe(pi.hProcess, &nt_header);
    ok(exe_base != NULL, "Could not find EXE in remote process\n");

    ret = are_imports_resolved(pi.hProcess, exe_base, &nt_header);
    ok(!ret, "IAT entry resolved prematurely\n");

    thread_handle = CreateRemoteThread(pi.hProcess, NULL, 0,
                                       (LPTHREAD_START_ROUTINE)exit_thread_ptr,
                                       (PVOID)(ULONG_PTR)0x1234, CREATE_SUSPENDED, NULL);
    ok(thread_handle != NULL, "Could not create remote thread (%ld)\n", GetLastError());

    ret = are_imports_resolved(pi.hProcess, exe_base, &nt_header);
    ok(!ret, "IAT entry resolved prematurely\n");

    ctx.ContextFlags = CONTEXT_ALL;
    ret = GetThreadContext( thread_handle, &ctx );
    ok( ret, "Failed retrieving remote thread context (%ld)\n", GetLastError() );
    ok( ctx.ContextFlags == CONTEXT_ALL, "wrong flags %lx\n", ctx.ContextFlags );
#ifdef __x86_64__
    ok( !ctx.Rax, "rax is not zero %Ix\n", ctx.Rax );
    ok( !ctx.Rbx, "rbx is not zero %Ix\n", ctx.Rbx );
    ok( ctx.Rcx == (ULONG_PTR)exit_thread_ptr, "wrong rcx %Ix/%p\n", ctx.Rcx, exit_thread_ptr );
    ok( ctx.Rdx == 0x1234, "wrong rdx %Ix\n", ctx.Rdx );
    ok( !ctx.Rsi, "rsi is not zero %Ix\n", ctx.Rsi );
    ok( !ctx.Rdi, "rdi is not zero %Ix\n", ctx.Rdi );
    ok( !ctx.Rbp, "rbp is not zero %Ix\n", ctx.Rbp );
    ok( !ctx.R8, "r8 is not zero %Ix\n", ctx.R8 );
    ok( !ctx.R9, "r9 is not zero %Ix\n", ctx.R9 );
    ok( !ctx.R10, "r10 is not zero %Ix\n", ctx.R10 );
    ok( !ctx.R11, "r11 is not zero %Ix\n", ctx.R11 );
    ok( !ctx.R12, "r12 is not zero %Ix\n", ctx.R12 );
    ok( !ctx.R13, "r13 is not zero %Ix\n", ctx.R13 );
    ok( !ctx.R14, "r14 is not zero %Ix\n", ctx.R14 );
    ok( !ctx.R15, "r15 is not zero %Ix\n", ctx.R15 );
    ok( ctx.EFlags == 0x200, "wrong flags %08lx\n", ctx.EFlags );
    ok( ctx.MxCsr == 0x1f80, "wrong mxcsr %08lx\n", ctx.MxCsr );
    ok( ctx.FltSave.ControlWord == 0x27f, "wrong control %08x\n", ctx.FltSave.ControlWord );
#else
    ok( !ctx.Ebp || broken(ctx.Ebp), /* winxp */ "ebp is not zero %08lx\n", ctx.Ebp );
    if (!ctx.Ebp)  /* winxp is completely different */
    {
        ok( !ctx.Ecx, "ecx is not zero %08lx\n", ctx.Ecx );
        ok( !ctx.Edx, "edx is not zero %08lx\n", ctx.Edx );
        ok( !ctx.Esi, "esi is not zero %08lx\n", ctx.Esi );
        ok( !ctx.Edi, "edi is not zero %08lx\n", ctx.Edi );
    }
    ok( ctx.Eax == (ULONG_PTR)exit_thread_ptr, "wrong eax %08lx/%p\n", ctx.Eax, exit_thread_ptr );
    ok( ctx.Ebx == 0x1234, "wrong ebx %08lx\n", ctx.Ebx );
    ok( (ctx.EFlags & ~2) == 0x200, "wrong flags %08lx\n", ctx.EFlags );
    ok( (WORD)ctx.FloatSave.ControlWord == 0x27f, "wrong control %08lx\n", ctx.FloatSave.ControlWord );
    ok( *(WORD *)ctx.ExtendedRegisters == 0x27f, "wrong control %08x\n", *(WORD *)ctx.ExtendedRegisters );
#endif

    ResumeThread( thread_handle );
    dret = WaitForSingleObject(thread_handle, 60000);
    ok(dret == WAIT_OBJECT_0, "Waiting for remote thread failed (%ld)\n", GetLastError());
    ret = GetExitCodeThread(thread_handle, &exit_code);
    ok(ret, "Failed to retrieve remote thread exit code (%ld)\n", GetLastError());
    ok(exit_code == 0x1234, "Invalid remote thread exit code\n");

    ret = are_imports_resolved(pi.hProcess, exe_base, &nt_header);
    ok(ret, "EXE IAT entry not resolved\n");

    if (thread_handle)
        CloseHandle(thread_handle);

    /* Note that the child's main thread is still suspended so the exit code
     * is set by the TerminateProcess() call.
     */
    TerminateProcess(pi.hProcess, 0);
    wait_and_close_child_process(&pi);
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
    void *user_thread_start, *start_ptr, *entry_ptr, *peb_ptr;
    PEB child_peb, *peb = NtCurrentTeb()->Peb;

    exit_process_ptr = GetProcAddress(hkernel32, "ExitProcess");
    ok(exit_process_ptr != NULL, "GetProcAddress ExitProcess failed\n");

    call_named_pipe_a = GetProcAddress(hkernel32, "CallNamedPipeA");
    ok(call_named_pipe_a != NULL, "GetProcAddress CallNamedPipeA failed\n");

    si.cb = sizeof(si);
    ret = CreateProcessA(NULL, selfname, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &si, &pi);
    ok(ret, "Failed to create process (%ld)\n", GetLastError());

    exe_base = get_process_exe(pi.hProcess, &nt_header);
    /* Make sure we found the EXE in the new process */
    ok(exe_base != NULL, "Could not find EXE in remote process\n");

    ret = are_imports_resolved(pi.hProcess, exe_base, &nt_header);
    ok(!ret, "IAT entry resolved prematurely\n");

    server_pipe_handle = CreateNamedPipeA(pipe_name, PIPE_ACCESS_DUPLEX | FILE_FLAG_WRITE_THROUGH,
                                        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 0x20000, 0x20000,
                                        0, NULL);
    ok(server_pipe_handle != INVALID_HANDLE_VALUE, "Failed to create communication pipe (%ld)\n", GetLastError());

    /* Set up the remote process environment */
    ctx.ContextFlags = CONTEXT_ALL;
    ret = GetThreadContext(pi.hThread, &ctx);
    ok(ret, "Failed retrieving remote thread context (%ld)\n", GetLastError());
    ok( ctx.ContextFlags == CONTEXT_ALL, "wrong flags %lx\n", ctx.ContextFlags );

    remote_pipe_params = VirtualAllocEx(pi.hProcess, NULL, sizeof(pipe_params), MEM_COMMIT, PAGE_READWRITE);
    ok(remote_pipe_params != NULL, "Failed allocating memory in remote process (%ld)\n", GetLastError());

    pipe_params.pipe_write_buf = pipe_write_magic;
    pipe_params.pipe_read_buf = 0;
    pipe_params.bytes_returned = 0;
    strcpy(pipe_params.pipe_name, pipe_name);

    ret = WriteProcessMemory(pi.hProcess, remote_pipe_params,
                             &pipe_params, sizeof(pipe_params), NULL);
    ok(ret, "Failed to write to remote process memory (%ld)\n", GetLastError());

#ifdef __x86_64__
    ok( !ctx.Rax, "rax is not zero %Ix\n", ctx.Rax );
    ok( !ctx.Rbx, "rbx is not zero %Ix\n", ctx.Rbx );
    ok( !ctx.Rsi, "rsi is not zero %Ix\n", ctx.Rsi );
    ok( !ctx.Rdi, "rdi is not zero %Ix\n", ctx.Rdi );
    ok( !ctx.Rbp, "rbp is not zero %Ix\n", ctx.Rbp );
    ok( !ctx.R8, "r8 is not zero %Ix\n", ctx.R8 );
    ok( !ctx.R9, "r9 is not zero %Ix\n", ctx.R9 );
    ok( !ctx.R10, "r10 is not zero %Ix\n", ctx.R10 );
    ok( !ctx.R11, "r11 is not zero %Ix\n", ctx.R11 );
    ok( !ctx.R12, "r12 is not zero %Ix\n", ctx.R12 );
    ok( !ctx.R13, "r13 is not zero %Ix\n", ctx.R13 );
    ok( !ctx.R14, "r14 is not zero %Ix\n", ctx.R14 );
    ok( !ctx.R15, "r15 is not zero %Ix\n", ctx.R15 );
    ok( ctx.EFlags == 0x200, "wrong flags %08lx\n", ctx.EFlags );
    ok( ctx.MxCsr == 0x1f80, "wrong mxcsr %08lx\n", ctx.MxCsr );
    ok( ctx.FltSave.ControlWord == 0x27f, "wrong control %08x\n", ctx.FltSave.ControlWord );
    start_ptr = (void *)ctx.Rip;
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
    ok(ret, "Failed to write to remote process thread stack (%ld)\n", GetLastError());
#else
    ok( !ctx.Ebp || broken(ctx.Ebp), /* winxp */ "ebp is not zero %08lx\n", ctx.Ebp );
    if (!ctx.Ebp)  /* winxp is completely different */
    {
        ok( !ctx.Ecx, "ecx is not zero %08lx\n", ctx.Ecx );
        ok( !ctx.Edx, "edx is not zero %08lx\n", ctx.Edx );
        ok( !ctx.Esi, "esi is not zero %08lx\n", ctx.Esi );
        ok( !ctx.Edi, "edi is not zero %08lx\n", ctx.Edi );
    }
    ok( (ctx.EFlags & ~2) == 0x200, "wrong flags %08lx\n", ctx.EFlags );
    ok( (WORD)ctx.FloatSave.ControlWord == 0x27f, "wrong control %08lx\n", ctx.FloatSave.ControlWord );
    ok( *(WORD *)ctx.ExtendedRegisters == 0x27f, "wrong control %08x\n", *(WORD *)ctx.ExtendedRegisters );
    start_ptr = (void *)ctx.Eip;
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
    ok(ret, "Failed to write to remote process thread stack (%ld)\n", GetLastError());
#endif

    ret = ReadProcessMemory( pi.hProcess, peb_ptr, &child_peb, sizeof(child_peb), NULL );
    ok( ret, "Failed to read PEB (%lu)\n", GetLastError() );
    ok( child_peb.ImageBaseAddress == exe_base, "wrong base %p/%p\n",
        child_peb.ImageBaseAddress, exe_base );
    user_thread_start = GetProcAddress( GetModuleHandleA("ntdll.dll"), "RtlUserThreadStart" );
    if (user_thread_start)
        ok( start_ptr == user_thread_start,
            "wrong start addr %p / %p\n", start_ptr, user_thread_start );
    ok( entry_ptr == (char *)exe_base + nt_header.OptionalHeader.AddressOfEntryPoint,
        "wrong entry point %p/%p\n", entry_ptr,
        (char *)exe_base + nt_header.OptionalHeader.AddressOfEntryPoint );

    ok( !child_peb.LdrData, "LdrData set %p\n", child_peb.LdrData );
    ok( !child_peb.FastPebLock, "FastPebLock set %p\n", child_peb.FastPebLock );
    ok( !child_peb.TlsBitmap, "TlsBitmap set %p\n", child_peb.TlsBitmap );
    ok( !child_peb.TlsExpansionBitmap, "TlsExpansionBitmap set %p\n", child_peb.TlsExpansionBitmap );
    ok( !child_peb.LoaderLock, "LoaderLock set %p\n", child_peb.LoaderLock );
    ok( !child_peb.ProcessHeap, "ProcessHeap set %p\n", child_peb.ProcessHeap );
    ok( !child_peb.CSDVersion.Buffer, "CSDVersion set %s\n", debugstr_w(child_peb.CSDVersion.Buffer) );

    ok( child_peb.OSMajorVersion == peb->OSMajorVersion, "OSMajorVersion not set %lu\n", child_peb.OSMajorVersion );
    ok( child_peb.OSPlatformId == peb->OSPlatformId, "OSPlatformId not set %lu\n", child_peb.OSPlatformId );
    ok( child_peb.SessionId == peb->SessionId, "SessionId not set %lu\n", child_peb.SessionId );
    ok( child_peb.CriticalSectionTimeout.QuadPart, "CriticalSectionTimeout not set %s\n",
        wine_dbgstr_longlong(child_peb.CriticalSectionTimeout.QuadPart) );
    ok( child_peb.HeapSegmentReserve == peb->HeapSegmentReserve,
        "HeapSegmentReserve not set %Iu\n", child_peb.HeapSegmentReserve );
    ok( child_peb.HeapSegmentCommit == peb->HeapSegmentCommit,
        "HeapSegmentCommit not set %Iu\n", child_peb.HeapSegmentCommit );
    ok( child_peb.HeapDeCommitTotalFreeThreshold == peb->HeapDeCommitTotalFreeThreshold,
        "HeapDeCommitTotalFreeThreshold not set %Iu\n", child_peb.HeapDeCommitTotalFreeThreshold );
    ok( child_peb.HeapDeCommitFreeBlockThreshold == peb->HeapDeCommitFreeBlockThreshold,
        "HeapDeCommitFreeBlockThreshold not set %Iu\n", child_peb.HeapDeCommitFreeBlockThreshold );

    if (pNtQueryInformationThread)
    {
        TEB child_teb;
        THREAD_BASIC_INFORMATION info;
        NTSTATUS status = pNtQueryInformationThread( pi.hThread, ThreadBasicInformation,
                                                     &info, sizeof(info), NULL );
        ok( !status, "NtQueryInformationProcess failed %lx\n", status );
        ret = ReadProcessMemory( pi.hProcess, info.TebBaseAddress, &child_teb, sizeof(child_teb), NULL );
        ok( ret, "Failed to read TEB (%lu)\n", GetLastError() );

        ok( child_teb.Peb == peb_ptr, "wrong Peb %p / %p\n", child_teb.Peb, peb_ptr );
        ok( PtrToUlong(child_teb.ClientId.UniqueProcess) == pi.dwProcessId, "wrong pid %lx / %lx\n",
            PtrToUlong(child_teb.ClientId.UniqueProcess), pi.dwProcessId );
        ok( PtrToUlong(child_teb.ClientId.UniqueThread) == pi.dwThreadId, "wrong tid %lx / %lx\n",
            PtrToUlong(child_teb.ClientId.UniqueThread), pi.dwThreadId );
        ok( PtrToUlong(child_teb.RealClientId.UniqueProcess) == pi.dwProcessId, "wrong real pid %lx / %lx\n",
            PtrToUlong(child_teb.RealClientId.UniqueProcess), pi.dwProcessId );
        ok( PtrToUlong(child_teb.RealClientId.UniqueThread) == pi.dwThreadId, "wrong real tid %lx / %lx\n",
            PtrToUlong(child_teb.RealClientId.UniqueThread), pi.dwThreadId );
        ok( child_teb.StaticUnicodeString.MaximumLength == sizeof(child_teb.StaticUnicodeBuffer),
            "StaticUnicodeString.MaximumLength wrong %x\n", child_teb.StaticUnicodeString.MaximumLength );
        ok( (char *)child_teb.StaticUnicodeString.Buffer == (char *)info.TebBaseAddress + offsetof(TEB, StaticUnicodeBuffer),
            "StaticUnicodeString.Buffer wrong %p\n", child_teb.StaticUnicodeString.Buffer );

        ok( !child_teb.CurrentLocale, "CurrentLocale set %lx\n", child_teb.CurrentLocale );
        ok( !child_teb.TlsLinks.Flink, "TlsLinks.Flink set %p\n", child_teb.TlsLinks.Flink );
        ok( !child_teb.TlsLinks.Blink, "TlsLinks.Blink set %p\n", child_teb.TlsLinks.Blink );
        ok( !child_teb.TlsExpansionSlots, "TlsExpansionSlots set %p\n", child_teb.TlsExpansionSlots );
        ok( !child_teb.FlsSlots, "FlsSlots set %p\n", child_teb.FlsSlots );
    }

    ret = SetThreadContext(pi.hThread, &ctx);
    ok(ret, "Failed to set remote thread context (%ld)\n", GetLastError());

    ResumeThread(pi.hThread);

    pipe_connected = ConnectNamedPipe(server_pipe_handle, NULL) || (GetLastError() == ERROR_PIPE_CONNECTED);
    ok(pipe_connected, "Pipe did not connect\n");

    ret = ReadFile(server_pipe_handle, &pipe_magic, sizeof(pipe_magic), &numb, NULL);
    ok(ret, "Failed to read buffer from pipe (%ld)\n", GetLastError());

    ok(pipe_magic == pipe_write_magic, "Did not get the correct magic from the remote process\n");

    /* Validate the imports: at this point the thread in the new process
     * should have initialized the EXE module imports and called each dll's
     * DllMain(), notifying it of the new thread in the process.
     */
    ret = are_imports_resolved(pi.hProcess, exe_base, &nt_header);
    ok(ret, "EXE IAT is not resolved\n");

    ret = WriteFile(server_pipe_handle, &pipe_magic, sizeof(pipe_magic), &numb, NULL);
    ok(ret, "Failed to write the magic back to the pipe (%ld)\n", GetLastError());
    CloseHandle(server_pipe_handle);

    /* Avoid wait_child_process() because the exit code results from a race
     * between the TerminateProcess() call and the child's ExitProcess() call
     * which uses a random value in the 64 bit case.
     */
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
            ok(GetLastError() == ERROR_INVALID_PARAMETER, "expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
        }
    }
}

static void test_session_info(void)
{
    DWORD session_id, active_session;
    BOOL r;

    r = ProcessIdToSessionId(GetCurrentProcessId(), &session_id);
    ok(r, "ProcessIdToSessionId failed: %lu\n", GetLastError());
    trace("session_id = %lx\n", session_id);

    active_session = pWTSGetActiveConsoleSessionId();
    trace("active_session = %lx\n", active_session);
}

static void test_process_info(HANDLE hproc)
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
        sizeof(ULONG) /* ProcessCookie */,
        sizeof(SECTION_IMAGE_INFORMATION) /* ProcessImageInformation */,
        sizeof(PROCESS_CYCLE_TIME_INFORMATION) /* ProcessCycleTime */,
        sizeof(ULONG) /* ProcessPagePriority */,
        40 /* ProcessInstrumentationCallback */,
        0 /* FIXME: sizeof(PROCESS_STACK_ALLOCATION_INFORMATION) ProcessThreadStackAllocation */,
        0 /* FIXME: sizeof(PROCESS_WS_WATCH_INFORMATION_EX[]) ProcessWorkingSetWatchEx */,
        sizeof(buf) /* ProcessImageFileNameWin32 */,
#if 0 /* FIXME: Add remaining classes */
        sizeof(HANDLE) /* ProcessImageFileMapping */,
        sizeof(PROCESS_AFFINITY_UPDATE_MODE) /* ProcessAffinityUpdateMode */,
        sizeof(PROCESS_MEMORY_ALLOCATION_MODE) /* ProcessMemoryAllocationMode */,
        sizeof(USHORT[]) /* ProcessGroupInformation */,
        sizeof(ULONG) /* ProcessTokenVirtualizationEnabled */,
        sizeof(ULONG_PTR) /* ProcessConsoleHostProcess */,
        sizeof(PROCESS_WINDOW_INFORMATION) /* ProcessWindowInformation */,
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
    ULONG i, status, ret_len;
    BOOL is_current = hproc == GetCurrentProcess();

    if (!pNtQueryInformationProcess)
    {
        win_skip("NtQueryInformationProcess is not available on this platform\n");
        return;
    }

    for (i = 0; i < ARRAY_SIZE(info_size); i++)
    {
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
        case ProcessIoPriority:
        case ProcessIoCounters:
        case ProcessVmCounters:
        case ProcessWow64Information:
        case ProcessDefaultHardErrorMode:
        case ProcessHandleCount:
        case ProcessImageFileName:
        case ProcessImageInformation:
        case ProcessCycleTime:
        case ProcessPagePriority:
        case ProcessImageFileNameWin32:
            ok(status == STATUS_SUCCESS, "for info %lu expected STATUS_SUCCESS, got %08lx (ret_len %lu)\n", i, status, ret_len);
            break;

        case ProcessAffinityMask:
        case ProcessBreakOnTermination:
            ok(status == STATUS_ACCESS_DENIED /* before win8 */ || status == STATUS_SUCCESS /* win8 is less strict */,
               "for info %lu expected STATUS_SUCCESS, got %08lx (ret_len %lu)\n", i, status, ret_len);
            break;

        case ProcessDebugObjectHandle:
            ok(status == STATUS_ACCESS_DENIED || status == STATUS_PORT_NOT_SET,
               "for info %lu expected STATUS_ACCESS_DENIED, got %08lx (ret_len %lu)\n", i, status, ret_len);
            break;
        case ProcessCookie:
            if (is_current)
                ok(status == STATUS_SUCCESS || status == STATUS_INVALID_PARAMETER /* before win8 */,
                   "for info %lu got %08lx (ret_len %lu)\n", i, status, ret_len);
            else
                ok(status == STATUS_INVALID_PARAMETER /* before win8 */ || status == STATUS_ACCESS_DENIED,
                   "for info %lu got %08lx (ret_len %lu)\n", i, status, ret_len);
            break;
        case ProcessExecuteFlags:
        case ProcessDebugPort:
        case ProcessDebugFlags:
            if (is_current)
                ok(status == STATUS_SUCCESS || status == STATUS_INVALID_PARAMETER,
                    "for info %lu, got %08lx (ret_len %lu)\n", i, status, ret_len);
            else
                todo_wine
                ok(status == STATUS_ACCESS_DENIED,
                    "for info %lu expected STATUS_ACCESS_DENIED, got %08lx (ret_len %lu)\n", i, status, ret_len);
            break;

        default:
            if (is_current)
                ok(status == STATUS_SUCCESS || status == STATUS_UNSUCCESSFUL || status == STATUS_INVALID_PARAMETER,
                    "for info %lu, got %08lx (ret_len %lu)\n", i, status, ret_len);
            else
                ok(status == STATUS_ACCESS_DENIED,
                    "for info %lu expected STATUS_ACCESS_DENIED, got %08lx (ret_len %lu)\n", i, status, ret_len);
            break;
        }
    }
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
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER, "got %d, error %ld\n", ret, GetLastError());

    len = 0;
    ret = pGetLogicalProcessorInformationEx(RelationProcessorCore, NULL, &len);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %d, error %ld\n", ret, GetLastError());
    ok(len > 0, "got %lu\n", len);

    len = 0;
    ret = pGetLogicalProcessorInformationEx(RelationAll, NULL, &len);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %d, error %ld\n", ret, GetLastError());
    ok(len > 0, "got %lu\n", len);

    info = HeapAlloc(GetProcessHeap(), HEAP_ZERO_MEMORY, len);
    ret = pGetLogicalProcessorInformationEx(RelationAll, info, &len);
    ok(ret, "got %d, error %ld\n", ret, GetLastError());
    ok(info->Size > 0, "got %lu\n", info->Size);
    HeapFree(GetProcessHeap(), 0, info);
}

#ifndef __REACTOS__
static void test_GetSystemCpuSetInformation(void)
{
    SYSTEM_CPU_SET_INFORMATION *info, *info_nt;
    HANDLE process = GetCurrentProcess();
    ULONG size, expected_size;
    NTSTATUS status;
    SYSTEM_INFO si;
    BOOL ret;

    if (!pGetSystemCpuSetInformation)
    {
        win_skip("GetSystemCpuSetInformation() is not supported.\n");
        return;
    }

    GetSystemInfo(&si);

    expected_size = sizeof(*info) * si.dwNumberOfProcessors;

    if (0)
    {
        /* Crashes on Windows with NULL return length. */
        pGetSystemCpuSetInformation(NULL, 0, NULL, process, 0);
    }

    size = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pGetSystemCpuSetInformation(NULL, size, &size, process, 0);
    ok(!ret && GetLastError() == ERROR_NOACCESS, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ok(!size, "Got unexpected size %lu.\n", size);

    size = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pGetSystemCpuSetInformation(NULL, 0, &size, (HANDLE)0xdeadbeef, 0);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ok(!size, "Got unexpected size %lu.\n", size);

    size = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pGetSystemCpuSetInformation(NULL, 0, &size, process, 0);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ok(size == expected_size, "Got unexpected size %lu.\n", size);

    info = heap_alloc(size);
    info_nt = heap_alloc(size);

    status = pNtQuerySystemInformationEx(SystemCpuSetInformation, &process, sizeof(process), info_nt, expected_size, NULL);
    ok(!status, "Got unexpected status %#lx.\n", status);

    size = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = pGetSystemCpuSetInformation(info, expected_size, &size, process, 0);
    ok(ret && GetLastError() == 0xdeadbeef, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ok(size == expected_size, "Got unexpected size %lu.\n", size);

    ok(!memcmp(info, info_nt, expected_size), "Info does not match NtQuerySystemInformationEx().\n");

    heap_free(info_nt);
    heap_free(info);
}
#endif

static void test_largepages(void)
{
    SIZE_T size;

    if (!pGetLargePageMinimum) {
        win_skip("No GetLargePageMinimum support.\n");
        return;
    }
    size = pGetLargePageMinimum();

    ok((size == 0) || (size == 2*1024*1024) || (size == 4*1024*1024), "GetLargePageMinimum reports %Id size\n", size);
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
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "got %ld\n", GetLastError());
        ok(size == needed, "%d: got %Id expect %Id\n", i, size, needed);

        memset(&list, 0xcc, sizeof(list));
        ret = pInitializeProcThreadAttributeList(&list, i, 0, &size);
        ok(ret, "got %d\n", ret);
        ok(list.mask == 0, "%d: got %08lx\n", i, list.mask);
        ok(list.size == i, "%d: got %08lx\n", i, list.size);
        ok(list.count == 0, "%d: got %08lx\n", i, list.count);
        ok(list.unk == 0, "%d: got %08Ix\n", i, list.unk);
    }

    memset(handles, 0, sizeof(handles));
    memset(&expect_list, 0xcc, sizeof(expect_list));
    expect_list.mask = 0;
    expect_list.size = i - 1;
    expect_list.count = 0;
    expect_list.unk = 0;

    ret = pUpdateProcThreadAttribute(&list, 0, 0xcafe, handles, sizeof(PROCESSOR_NUMBER), NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_NOT_SUPPORTED, "got %ld\n", GetLastError());

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, handles, sizeof(handles[0]) / 2, NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_BAD_LENGTH, "got %ld\n", GetLastError());

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, handles, sizeof(handles[0]) * 2, NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_BAD_LENGTH, "got %ld\n", GetLastError());

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, handles, sizeof(handles[0]), NULL, NULL);
    ok(ret, "got %d\n", ret);

    expect_list.mask |= 1 << ProcThreadAttributeParentProcess;
    expect_list.attrs[0].attr = PROC_THREAD_ATTRIBUTE_PARENT_PROCESS;
    expect_list.attrs[0].size = sizeof(handles[0]);
    expect_list.attrs[0].value = handles;
    expect_list.count++;

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS, handles, sizeof(handles[0]), NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_OBJECT_NAME_EXISTS, "got %ld\n", GetLastError());

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handles, sizeof(handles) - 1, NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_BAD_LENGTH, "got %ld\n", GetLastError());

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handles, sizeof(handles), NULL, NULL);
    ok(ret, "got %d\n", ret);

    expect_list.mask |= 1 << ProcThreadAttributeHandleList;
    expect_list.attrs[1].attr = PROC_THREAD_ATTRIBUTE_HANDLE_LIST;
    expect_list.attrs[1].size = sizeof(handles);
    expect_list.attrs[1].value = handles;
    expect_list.count++;

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, handles, sizeof(handles), NULL, NULL);
    ok(!ret, "got %d\n", ret);
    ok(GetLastError() == ERROR_OBJECT_NAME_EXISTS, "got %ld\n", GetLastError());

    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR, handles, sizeof(PROCESSOR_NUMBER), NULL, NULL);
    ok(ret || GetLastError() == ERROR_NOT_SUPPORTED, "got %d gle %ld\n", ret, GetLastError());

    if (ret)
    {
        expect_list.mask |= 1 << ProcThreadAttributeIdealProcessor;
        expect_list.attrs[2].attr = PROC_THREAD_ATTRIBUTE_IDEAL_PROCESSOR;
        expect_list.attrs[2].size = sizeof(PROCESSOR_NUMBER);
        expect_list.attrs[2].value = handles;
        expect_list.count++;
    }

#ifndef __REACTOS__
    ret = pUpdateProcThreadAttribute(&list, 0, PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE, handles, sizeof(handles[0]), NULL, NULL);
    ok(ret || broken(GetLastError() == ERROR_NOT_SUPPORTED), "got %d gle %ld\n", ret, GetLastError());

    if (ret)
    {
        unsigned int i = expect_list.count++;
        expect_list.mask |= 1 << ProcThreadAttributePseudoConsole;
        expect_list.attrs[i].attr = PROC_THREAD_ATTRIBUTE_PSEUDOCONSOLE;
        expect_list.attrs[i].size = sizeof(HPCON);
        expect_list.attrs[i].value = handles;
    }
#endif

    ok(!memcmp(&list, &expect_list, size), "mismatch\n");

    pDeleteProcThreadAttributeList(&list);
}

/* level 0: Main test process
 * level 1: Process created by level 0 process without handle inheritance
 * level 2: Process created by level 1 process with handle inheritance and level 0
 *          process parent substitute.
 * level 255: Process created by level 1 process during invalid parent handles testing. */
static void test_parent_process_attribute(unsigned int level, HANDLE read_pipe)
{
    PROCESS_BASIC_INFORMATION pbi;
    char buffer[MAX_PATH + 64];
    HANDLE write_pipe = NULL;
    PROCESS_INFORMATION info;
    SECURITY_ATTRIBUTES sa;
    STARTUPINFOEXA si;
    DWORD parent_id;
    NTSTATUS status;
    ULONG pbi_size;
    HANDLE parent;
    DWORD size;
    BOOL ret;

    struct
    {
        HANDLE parent;
        DWORD parent_id;
    }
    parent_data;

    if (level == 255)
        return;

    if (!pInitializeProcThreadAttributeList)
    {
        win_skip("No support for ProcThreadAttributeList.\n");
        return;
    }

    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    if (!level)
    {
        ret = CreatePipe(&read_pipe, &write_pipe, &sa, 0);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

        parent_data.parent = OpenProcess(PROCESS_CREATE_PROCESS | PROCESS_QUERY_INFORMATION, TRUE, GetCurrentProcessId());
        parent_data.parent_id = GetCurrentProcessId();
    }
    else
    {
        status = pNtQueryInformationProcess(GetCurrentProcess(), ProcessBasicInformation, &pbi, sizeof(pbi), &pbi_size);
        ok(status == STATUS_SUCCESS, "Got unexpected status %#lx.\n", status);
        parent_id = pbi.InheritedFromUniqueProcessId;

        memset(&parent_data, 0, sizeof(parent_data));
        ret = ReadFile(read_pipe, &parent_data, sizeof(parent_data), &size, NULL);
        ok((level == 2 && ret) || (level == 1 && !ret && GetLastError() == ERROR_INVALID_HANDLE),
                "Got unexpected ret %#x, level %u, GetLastError() %lu.\n",
                ret, level, GetLastError());
    }

    if (level == 2)
    {
        ok(parent_id == parent_data.parent_id, "Got parent id %lu, parent_data.parent_id %lu.\n",
                parent_id, parent_data.parent_id);
        return;
    }

    memset(&si, 0, sizeof(si));
    si.StartupInfo.cb = sizeof(si);

    if (level)
    {
        HANDLE handle;
        SIZE_T size;

        ret = pInitializeProcThreadAttributeList(NULL, 1, 0, &size);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
                "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

        sprintf(buffer, "\"%s\" process parent %u %p", selfname, 255, read_pipe);

#if 0
        /* Crashes on some Windows installations, otherwise successfully creates process. */
        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT,
                NULL, NULL, (STARTUPINFOA *)&si, &info);
        ok(ret, "Got unexpected ret %#x, GetLastError() %u.\n", ret, GetLastError());
        wait_and_close_child_process(&info);
#endif
        si.lpAttributeList = heap_alloc(size);
        ret = pInitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &size);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
        handle = OpenProcess(PROCESS_CREATE_PROCESS, TRUE, GetCurrentProcessId());
        ret = pUpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
                &handle, sizeof(handle), NULL, NULL);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
        ret = CreateProcessA(NULL, buffer, NULL, NULL, TRUE, EXTENDED_STARTUPINFO_PRESENT,
                NULL, NULL, (STARTUPINFOA *)&si, &info);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
        wait_and_close_child_process(&info);
        CloseHandle(handle);
        pDeleteProcThreadAttributeList(si.lpAttributeList);
        heap_free(si.lpAttributeList);

        si.lpAttributeList = heap_alloc(size);
        ret = pInitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &size);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
        handle = (HANDLE)0xdeadbeef;
        ret = pUpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
                &handle, sizeof(handle), NULL, NULL);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
        ret = CreateProcessA(NULL, buffer, NULL, NULL, TRUE, EXTENDED_STARTUPINFO_PRESENT,
                NULL, NULL, (STARTUPINFOA *)&si, &info);
        ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected ret %#x, GetLastError() %lu.\n",
                ret, GetLastError());
        pDeleteProcThreadAttributeList(si.lpAttributeList);
        heap_free(si.lpAttributeList);

        si.lpAttributeList = heap_alloc(size);
        ret = pInitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &size);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
        handle = NULL;
        ret = pUpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
                &handle, sizeof(handle), NULL, NULL);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
        ret = CreateProcessA(NULL, buffer, NULL, NULL, TRUE, EXTENDED_STARTUPINFO_PRESENT,
                NULL, NULL, (STARTUPINFOA *)&si, &info);
        ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected ret %#x, GetLastError() %lu.\n",
                ret, GetLastError());
        pDeleteProcThreadAttributeList(si.lpAttributeList);
        heap_free(si.lpAttributeList);

        si.lpAttributeList = heap_alloc(size);
        ret = pInitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &size);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
        handle = GetCurrentProcess();
        ret = pUpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
                &handle, sizeof(handle), NULL, NULL);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
        ret = CreateProcessA(NULL, buffer, NULL, NULL, TRUE, EXTENDED_STARTUPINFO_PRESENT,
                NULL, NULL, (STARTUPINFOA *)&si, &info);
        /* Broken on Vista / w7 / w10. */
        ok(ret || broken(!ret && GetLastError() == ERROR_INVALID_HANDLE),
                "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
        if (ret)
            wait_and_close_child_process(&info);
        pDeleteProcThreadAttributeList(si.lpAttributeList);
        heap_free(si.lpAttributeList);

        si.lpAttributeList = heap_alloc(size);
        ret = pInitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &size);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

        parent = OpenProcess(PROCESS_CREATE_PROCESS, FALSE, parent_id);

        ret = pUpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_PARENT_PROCESS,
                &parent, sizeof(parent), NULL, NULL);
        ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    }

    sprintf(buffer, "\"%s\" process parent %u %p", selfname, level + 1, read_pipe);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, level == 1, level == 1 ? EXTENDED_STARTUPINFO_PRESENT : 0,
            NULL, NULL, (STARTUPINFOA *)&si, &info);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    if (level)
    {
        pDeleteProcThreadAttributeList(si.lpAttributeList);
        heap_free(si.lpAttributeList);
        CloseHandle(parent);
    }
    else
    {
        ret = WriteFile(write_pipe, &parent_data, sizeof(parent_data), &size, NULL);
    }

    wait_and_close_child_process(&info);

    if (!level)
    {
        CloseHandle(read_pipe);
        CloseHandle(write_pipe);
        CloseHandle(parent_data.parent);
    }
}

static void test_handle_list_attribute(BOOL child, HANDLE handle1, HANDLE handle2)
{
    char buffer[MAX_PATH + 64];
    HANDLE pipe[2];
    PROCESS_INFORMATION info;
    STARTUPINFOEXA si;
    SIZE_T size;
    BOOL ret;
    SECURITY_ATTRIBUTES sa;

    if (child)
    {
        char name1[256], name2[256];
        DWORD flags;

        flags = 0;
        ret = GetHandleInformation(handle1, &flags);
        ok(ret, "Failed to get handle info, error %ld.\n", GetLastError());
        ok(flags == HANDLE_FLAG_INHERIT, "Unexpected flags %#lx.\n", flags);
#ifndef __REACTOS__ // Can't link
        ret = GetFileInformationByHandleEx(handle1, FileNameInfo, name1, sizeof(name1));
        ok(ret, "Failed to get pipe name, error %ld\n", GetLastError());
#endif
        CloseHandle(handle1);
        flags = 0;
        ret = GetHandleInformation(handle2, &flags);
        if (ret)
        {
            ok(!(flags & HANDLE_FLAG_INHERIT), "Parent's handle shouldn't have been inherited\n");
#ifndef __REACTOS__ // Can't link
            ret = GetFileInformationByHandleEx(handle2, FileNameInfo, name2, sizeof(name2));
            ok(!ret || strcmp(name1, name2), "Parent's handle shouldn't have been inherited\n");
#endif
        }
        else
            ok(GetLastError() == ERROR_INVALID_HANDLE, "Unexpected return value, error %ld.\n", GetLastError());

        return;
    }

    ret = pInitializeProcThreadAttributeList(NULL, 1, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
            "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    memset(&si, 0, sizeof(si));
    si.StartupInfo.cb = sizeof(si);
    si.lpAttributeList = heap_alloc(size);
    ret = pInitializeProcThreadAttributeList(si.lpAttributeList, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    memset(&sa, 0, sizeof(sa));
    sa.nLength = sizeof(sa);
    sa.bInheritHandle = TRUE;

    ret = CreatePipe(&pipe[0], &pipe[1], &sa, 1024);
    ok(ret, "Failed to create a pipe.\n");

    ret = pUpdateProcThreadAttribute(si.lpAttributeList, 0, PROC_THREAD_ATTRIBUTE_HANDLE_LIST, &pipe[0],
            sizeof(pipe[0]), NULL, NULL);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    sprintf(buffer, "\"%s\" process handlelist %p %p", selfname, pipe[0], pipe[1]);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, TRUE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
            (STARTUPINFOA *)&si, &info);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    wait_and_close_child_process(&info);

    CloseHandle(pipe[0]);
    CloseHandle(pipe[1]);
}

static void test_dead_process(void)
{
    DWORD_PTR data[256];
    PROCESS_BASIC_INFORMATION basic;
    SYSTEM_PROCESS_INFORMATION *spi;
    SECTION_IMAGE_INFORMATION image;
    PROCESS_INFORMATION pi;
    PROCESS_PRIORITY_CLASS *prio = (PROCESS_PRIORITY_CLASS *)data;
    BYTE *buffer = NULL;
    BOOL found;
    ULONG size = 0;
    DWORD offset = 0;
    NTSTATUS status;

    create_process("exit", &pi);
    wait_child_process(pi.hProcess);
    Sleep(100);

    memset( data, 0, sizeof(data) );
    status = NtQueryInformationProcess( pi.hProcess, ProcessImageFileName, data, sizeof(data), NULL);
    ok( !status, "ProcessImageFileName failed %lx\n", status );
    ok( ((UNICODE_STRING *)data)->Length, "ProcessImageFileName not set\n" );
    ok( ((UNICODE_STRING *)data)->Buffer[0] == '\\', "ProcessImageFileName not set\n" );

    memset( prio, 0xcc, sizeof(*prio) );
    status = NtQueryInformationProcess( pi.hProcess, ProcessPriorityClass, prio, sizeof(*prio), NULL);
    ok( !status, "ProcessPriorityClass failed %lx\n", status );
    ok( prio->PriorityClass != 0xcc, "ProcessPriorityClass not set\n" );

    memset( &basic, 0xcc, sizeof(basic) );
    status = NtQueryInformationProcess( pi.hProcess, ProcessBasicInformation, &basic, sizeof(basic), NULL);
    ok( !status, "ProcessBasicInformation failed %lx\n", status );
    ok( basic.ExitStatus == 0, "ProcessBasicInformation info modified\n" );

    memset( &image, 0xcc, sizeof(image) );
    status = NtQueryInformationProcess( pi.hProcess, ProcessImageInformation, &image, sizeof(image), NULL);
    ok( status == STATUS_PROCESS_IS_TERMINATING, "ProcessImageInformation wrong error %lx\n", status );
    ok( image.Machine == 0xcccc, "ProcessImageInformation info modified\n" );

    while ((status = NtQuerySystemInformation(SystemProcessInformation, buffer, size, &size)) == STATUS_INFO_LENGTH_MISMATCH)
    {
        free(buffer);
        buffer = malloc(size);
    }
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    found = FALSE;
    do
    {
        spi = (SYSTEM_PROCESS_INFORMATION *)(buffer + offset);
        if (spi->UniqueProcessId == ULongToHandle(pi.dwProcessId))
        {
            found = TRUE;
            break;
        }
        offset += spi->NextEntryOffset;
    } while (spi->NextEntryOffset);
    ok( !found, "process still enumerated\n" );
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
}

static void test_nested_jobs_child(unsigned int index)
{
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT port_info;
    HANDLE job, job_parent, job_other, port;
    PROCESS_INFORMATION pi;
    OVERLAPPED *overlapped;
    char job_name[32];
    ULONG_PTR value;
    DWORD dead_pid;
    BOOL ret, out;
    DWORD key;

    sprintf(job_name, "test_nested_jobs_%u", index);
    job = pOpenJobObjectA(JOB_OBJECT_ASSIGN_PROCESS | JOB_OBJECT_SET_ATTRIBUTES | JOB_OBJECT_QUERY
            | JOB_OBJECT_TERMINATE, FALSE, job_name);
    ok(!!job, "OpenJobObjectA error %lu\n", GetLastError());

    sprintf(job_name, "test_nested_jobs_%u", !index);
    job_other = pOpenJobObjectA(JOB_OBJECT_ASSIGN_PROCESS | JOB_OBJECT_SET_ATTRIBUTES | JOB_OBJECT_QUERY
            | JOB_OBJECT_TERMINATE, FALSE, job_name);
    ok(!!job_other, "OpenJobObjectA error %lu\n", GetLastError());

    job_parent = pCreateJobObjectW(NULL, NULL);
    ok(!!job_parent, "CreateJobObjectA error %lu\n", GetLastError());

    ret = pAssignProcessToJobObject(job_parent, GetCurrentProcess());
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

    create_process("wait", &pi);

    ret = pAssignProcessToJobObject(job_parent, pi.hProcess);
    ok(ret || broken(!ret && GetLastError() == ERROR_ACCESS_DENIED) /* Supported since Windows 8. */,
            "AssignProcessToJobObject error %lu\n", GetLastError());
    if (!ret)
    {
        win_skip("Nested jobs are not supported.\n");
        goto done;
    }
    ret = pAssignProcessToJobObject(job, pi.hProcess);
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, NULL, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    out = TRUE;
    ret = pIsProcessInJob(GetCurrentProcess(), job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);

    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    ret = pAssignProcessToJobObject(job, GetCurrentProcess());
    ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

    TerminateProcess(pi.hProcess, 0);
    wait_child_process(pi.hProcess);
    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);

    dead_pid = pi.dwProcessId;

    port = pCreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    ok(!!port, "CreateIoCompletionPort error %lu\n", GetLastError());

    port_info.CompletionPort = port;
    port_info.CompletionKey = job;
    ret = pSetInformationJobObject(job, JobObjectAssociateCompletionPortInformation, &port_info, sizeof(port_info));
    ok(ret, "SetInformationJobObject error %lu\n", GetLastError());
    port_info.CompletionKey = job_parent;
    ret = pSetInformationJobObject(job_parent, JobObjectAssociateCompletionPortInformation,
            &port_info, sizeof(port_info));
    ok(ret, "SetInformationJobObject error %lu\n", GetLastError());

    create_process("wait", &pi);
    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    out = FALSE;
    ret = pIsProcessInJob(pi.hProcess, job_parent, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    /* The first already dead child process still shows up randomly. */
    do
    {
        ret = GetQueuedCompletionStatus(port, &key, &value, &overlapped, 0);
    } while (ret && (ULONG_PTR)overlapped == dead_pid);

    ok(ret, "GetQueuedCompletionStatus: %lx\n", GetLastError());
    ok(key == JOB_OBJECT_MSG_NEW_PROCESS, "unexpected key %lx\n", key);
    ok((HANDLE)value == job, "unexpected value %p\n", (void *)value);
    ok((ULONG_PTR)overlapped == GetCurrentProcessId(), "unexpected pid %#lx\n", (DWORD)(DWORD_PTR)overlapped);

    do
    {
        ret = GetQueuedCompletionStatus(port, &key, &value, &overlapped, 0);
    } while (ret && (ULONG_PTR)overlapped == dead_pid);

    ok(ret, "GetQueuedCompletionStatus: %lx\n", GetLastError());
    ok(key == JOB_OBJECT_MSG_NEW_PROCESS, "unexpected key %lx\n", key);
    ok((HANDLE)value == job_parent, "unexpected value %p\n", (void *)value);
    ok((ULONG_PTR)overlapped == GetCurrentProcessId(), "unexpected pid %#lx\n", (DWORD)(DWORD_PTR)overlapped);

    test_completion(port, JOB_OBJECT_MSG_NEW_PROCESS, (DWORD_PTR)job, pi.dwProcessId, 0);
    test_completion(port, JOB_OBJECT_MSG_NEW_PROCESS, (DWORD_PTR)job_parent, pi.dwProcessId, 0);

    ret = GetQueuedCompletionStatus(port, &key, &value, &overlapped, 0);
    ok(!ret, "GetQueuedCompletionStatus succeeded.\n");

    if (index)
    {
        ret = pAssignProcessToJobObject(job_other, GetCurrentProcess());
        ok(!ret, "AssignProcessToJobObject succeeded\n");
        ok(GetLastError() == ERROR_ACCESS_DENIED, "Got unexpected error %lu.\n", GetLastError());
    }

    CloseHandle(port);

done:
    TerminateProcess(pi.hProcess, 0);
    wait_child_process(pi.hProcess);

    CloseHandle(pi.hProcess);
    CloseHandle(pi.hThread);
    CloseHandle(job_parent);
    CloseHandle(job);
    CloseHandle(job_other);
}

static void test_nested_jobs(void)
{
    BOOL ret, already_in_job = TRUE, create_succeeded = FALSE;
    PROCESS_INFORMATION info[2];
    char buffer[MAX_PATH + 26];
    STARTUPINFOA si = {0};
    HANDLE job1, job2;
    unsigned int i;

    if (!pIsProcessInJob)
    {
        win_skip("IsProcessInJob not available.\n");
        return;
    }

    job1 = pCreateJobObjectW(NULL, NULL);
    ok(!!job1, "CreateJobObjectW failed, error %lu.\n", GetLastError());
    job2 = pCreateJobObjectW(NULL, NULL);
    ok(!!job2, "CreateJobObjectW failed, error %lu.\n", GetLastError());

    create_succeeded = TRUE;
    sprintf(buffer, "\"%s\" process wait", selfname);
    for (i = 0; i < 2; ++i)
    {
        ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, &si, &info[i]);
        if (!ret && GetLastError() == ERROR_ACCESS_DENIED)
        {
            create_succeeded = FALSE;
            break;
        }
        ok(ret, "CreateProcessA error %lu\n", GetLastError());
    }

    if (create_succeeded)
    {
        ret = pIsProcessInJob(info[0].hProcess, NULL, &already_in_job);
        ok(ret, "IsProcessInJob error %lu\n", GetLastError());

        if (!already_in_job)
        {
            ret = pAssignProcessToJobObject(job2, info[1].hProcess);
            ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

            ret = pAssignProcessToJobObject(job1, info[0].hProcess);
            ok(ret, "AssignProcessToJobObject error %lu\n", GetLastError());

            ret = pAssignProcessToJobObject(job2, info[0].hProcess);
            ok(!ret, "AssignProcessToJobObject succeeded\n");
            ok(GetLastError() == ERROR_ACCESS_DENIED, "Got unexpected error %lu.\n", GetLastError());

            TerminateProcess(info[1].hProcess, 0);
            wait_child_process(info[1].hProcess);
            CloseHandle(info[1].hProcess);
            CloseHandle(info[1].hThread);

            ret = pAssignProcessToJobObject(job2, info[0].hProcess);
            ok(!ret, "AssignProcessToJobObject succeeded\n");
            ok(GetLastError() == ERROR_ACCESS_DENIED, "Got unexpected error %lu.\n", GetLastError());
        }

        TerminateProcess(info[0].hProcess, 0);
        wait_child_process(info[0].hProcess);
        CloseHandle(info[0].hProcess);
        CloseHandle(info[0].hThread);
    }

    if (already_in_job)
    {
        win_skip("Test process is already in job, can't test parenting non-empty job.\n");
    }

    CloseHandle(job1);
    CloseHandle(job2);

    job1 = pCreateJobObjectW(NULL, L"test_nested_jobs_0");
    ok(!!job1, "CreateJobObjectW failed, error %lu.\n", GetLastError());
    job2 = pCreateJobObjectW(NULL, L"test_nested_jobs_1");
    ok(!!job2, "CreateJobObjectW failed, error %lu.\n", GetLastError());

    sprintf(buffer, "\"%s\" process nested_jobs 0", selfname);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &info[0]),
            "CreateProcess failed\n");
    wait_child_process(info[0].hProcess);
    sprintf(buffer, "\"%s\" process nested_jobs 1", selfname);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, 0, NULL, NULL, &si, &info[1]),
        "CreateProcess failed\n");
    wait_child_process(info[1].hProcess);
    for (i = 0; i < 2; ++i)
    {
        CloseHandle(info[i].hProcess);
        CloseHandle(info[i].hThread);
    }

    CloseHandle(job1);
    CloseHandle(job2);
}

static void test_job_list_attribute(HANDLE parent_job)
{
    JOBOBJECT_BASIC_ACCOUNTING_INFORMATION job_info;
    JOBOBJECT_EXTENDED_LIMIT_INFORMATION limit_info;
    JOBOBJECT_ASSOCIATE_COMPLETION_PORT port_info;
    PPROC_THREAD_ATTRIBUTE_LIST attrs;
    char buffer[MAX_PATH + 19];
    PROCESS_INFORMATION pi;
    OVERLAPPED *overlapped;
    HANDLE jobs[2], port;
    STARTUPINFOEXA si;
    ULONG_PTR value;
    BOOL ret, out;
    HANDLE tmp;
    SIZE_T size;
    DWORD key;

    if (!pInitializeProcThreadAttributeList)
    {
        win_skip("No support for ProcThreadAttributeList\n");
        return;
    }

    ret = pInitializeProcThreadAttributeList(NULL, 1, 0, &size);
    ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER,
            "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    attrs = heap_alloc(size);


    jobs[0] = (HANDLE)0xdeadbeef;
    jobs[1] = NULL;

    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, jobs,
            sizeof(*jobs), NULL, NULL);
    if (!ret && GetLastError() == ERROR_NOT_SUPPORTED)
    {
        /* Supported since Win10. */
        win_skip("PROC_THREAD_ATTRIBUTE_JOB_LIST is not supported.\n");
        pDeleteProcThreadAttributeList(attrs);
        heap_free(attrs);
        return;
    }
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, jobs,
            3, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_BAD_LENGTH, "Got unexpected ret %#x, GetLastError() %lu.\n",
            ret, GetLastError());

    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, jobs,
            sizeof(*jobs) * 2, NULL, NULL);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, jobs,
            sizeof(*jobs), NULL, NULL);

    memset(&si, 0, sizeof(si));
    si.StartupInfo.cb = sizeof(si);
    si.lpAttributeList = attrs;
    sprintf(buffer, "\"%s\" process wait", selfname);

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
            (STARTUPINFOA *)&si, &pi);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected ret %#x, GetLastError() %lu.\n",
            ret, GetLastError());

    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, jobs + 1,
            sizeof(*jobs), NULL, NULL);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
            (STARTUPINFOA *)&si, &pi);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "Got unexpected ret %#x, GetLastError() %lu.\n",
            ret, GetLastError());

    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, &parent_job,
            sizeof(parent_job), NULL, NULL);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
            (STARTUPINFOA *)&si, &pi);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    ret = pIsProcessInJob(pi.hProcess, parent_job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    TerminateProcess(pi.hProcess, 0);
    wait_and_close_child_process(&pi);

    jobs[0] = pCreateJobObjectW(NULL, NULL);
    ok(!!jobs[0], "CreateJobObjectA error %lu\n", GetLastError());
    jobs[1] = pCreateJobObjectW(NULL, NULL);
    ok(!!jobs[1], "CreateJobObjectA error %lu\n", GetLastError());

    /* Breakaway works for the inherited job only. */
    limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK;
    ret = pSetInformationJobObject(parent_job, JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info));
    ok(ret, "SetInformationJobObject error %lu\n", GetLastError());
    limit_info.BasicLimitInformation.LimitFlags = JOB_OBJECT_LIMIT_BREAKAWAY_OK
            | JOB_OBJECT_LIMIT_SILENT_BREAKAWAY_OK;
    ret = pSetInformationJobObject(jobs[1], JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info));
    ok(ret, "SetInformationJobObject error %lu\n", GetLastError());

    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, jobs + 1,
            sizeof(*jobs), NULL, NULL);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT
            | CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, (STARTUPINFOA *)&si, &pi);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    ret = pIsProcessInJob(pi.hProcess, parent_job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);

    ret = pIsProcessInJob(pi.hProcess, jobs[1], &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    ret = pIsProcessInJob(pi.hProcess, jobs[0], &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);

    TerminateProcess(pi.hProcess, 0);
    wait_and_close_child_process(&pi);

    CloseHandle(jobs[1]);
    jobs[1] = pCreateJobObjectW(NULL, NULL);
    ok(!!jobs[1], "CreateJobObjectA error %lu\n", GetLastError());

    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, jobs + 1,
            sizeof(*jobs), NULL, NULL);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT,
            NULL, NULL, (STARTUPINFOA *)&si, &pi);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    ret = pIsProcessInJob(pi.hProcess, parent_job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    ret = pIsProcessInJob(pi.hProcess, jobs[1], &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    ret = pIsProcessInJob(pi.hProcess, jobs[0], &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(!out, "IsProcessInJob returned out=%u\n", out);

    TerminateProcess(pi.hProcess, 0);
    wait_and_close_child_process(&pi);

    ret = pQueryInformationJobObject(jobs[0], JobObjectBasicAccountingInformation, &job_info,
            sizeof(job_info), NULL);
    ok(ret, "QueryInformationJobObject error %lu\n", GetLastError());
    ok(!job_info.TotalProcesses, "Got unexpected TotalProcesses %lu.\n", job_info.TotalProcesses);
    ok(!job_info.ActiveProcesses, "Got unexpected ActiveProcesses %lu.\n", job_info.ActiveProcesses);

    ret = pQueryInformationJobObject(jobs[1], JobObjectBasicAccountingInformation, &job_info,
            sizeof(job_info), NULL);
    ok(ret, "QueryInformationJobObject error %lu\n", GetLastError());
    ok(job_info.TotalProcesses == 1, "Got unexpected TotalProcesses %lu.\n", job_info.TotalProcesses);
    ok(!job_info.ActiveProcesses || job_info.ActiveProcesses == 1, "Got unexpected ActiveProcesses %lu.\n",
            job_info.ActiveProcesses);

    /* Fails due to the second job already has the parent other than the first job in the list. */
    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, jobs,
            2 * sizeof(*jobs), NULL, NULL);

    port = pCreateIoCompletionPort(INVALID_HANDLE_VALUE, NULL, 0, 1);
    ok(!!port, "CreateIoCompletionPort error %lu\n", GetLastError());

    port_info.CompletionPort = port;
    port_info.CompletionKey = jobs[0];
    ret = pSetInformationJobObject(jobs[0], JobObjectAssociateCompletionPortInformation, &port_info, sizeof(port_info));
    ok(ret, "SetInformationJobObject error %lu\n", GetLastError());

    ret = GetQueuedCompletionStatus(port, &key, &value, &overlapped, 0);
    ok(!ret, "GetQueuedCompletionStatus succeeded.\n");

    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
            (STARTUPINFOA *)&si, &pi);
    ok(!ret && GetLastError() == ERROR_ACCESS_DENIED, "Got unexpected ret %#x, GetLastError() %lu.\n",
            ret, GetLastError());

    ret = GetQueuedCompletionStatus(port, &key, &value, &overlapped, 100);
    ok(ret, "GetQueuedCompletionStatus: %lx\n", GetLastError());
    ok(key == JOB_OBJECT_MSG_NEW_PROCESS, "unexpected key %lx\n", key);
    ok((HANDLE)value == jobs[0], "unexpected value %p\n", (void *)value);
    ok(!!overlapped, "Got zero pid.\n");

    ret = GetQueuedCompletionStatus(port, &key, &value, &overlapped, 100);
    ok(ret, "GetQueuedCompletionStatus: %lx\n", GetLastError());
    ok(key == JOB_OBJECT_MSG_EXIT_PROCESS, "unexpected key %lx\n", key);
    ok((HANDLE)value == jobs[0], "unexpected value %p\n", (void *)value);
    ok(!!overlapped, "Got zero pid.\n");

    ret = GetQueuedCompletionStatus(port, &key, &value, &overlapped, 100);
    ok(ret, "GetQueuedCompletionStatus: %lx\n", GetLastError());
    ok(key == JOB_OBJECT_MSG_ACTIVE_PROCESS_ZERO, "unexpected key %lx\n", key);
    ok((HANDLE)value == jobs[0], "unexpected value %p\n", (void *)value);
    ok(!overlapped, "Got unexpected overlapped %p.\n", overlapped);

    ret = GetQueuedCompletionStatus(port, &key, &value, &overlapped, 0);
    ok(!ret, "GetQueuedCompletionStatus succeeded.\n");

    CloseHandle(port);

    /* The first job got updated even though the process creation failed. */
    ret = pQueryInformationJobObject(jobs[0], JobObjectBasicAccountingInformation, &job_info,
            sizeof(job_info), NULL);
    ok(ret, "QueryInformationJobObject error %lu\n", GetLastError());
    ok(job_info.TotalProcesses == 1, "Got unexpected TotalProcesses %lu.\n", job_info.TotalProcesses);
    ok(!job_info.ActiveProcesses, "Got unexpected ActiveProcesses %lu.\n", job_info.ActiveProcesses);

    ret = pQueryInformationJobObject(jobs[1], JobObjectBasicAccountingInformation, &job_info,
            sizeof(job_info), NULL);
    ok(ret, "QueryInformationJobObject error %lu\n", GetLastError());
    ok(job_info.TotalProcesses == 1, "Got unexpected TotalProcesses %lu.\n", job_info.TotalProcesses);
    ok(!job_info.ActiveProcesses, "Got unexpected ActiveProcesses %lu.\n", job_info.ActiveProcesses);

    /* Check that the first job actually got the job_parent as parent. */
    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, jobs,
            sizeof(*jobs), NULL, NULL);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT
            | CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, (STARTUPINFOA *)&si, &pi);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    ret = pIsProcessInJob(pi.hProcess, parent_job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    ret = pIsProcessInJob(pi.hProcess, jobs[0], &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    TerminateProcess(pi.hProcess, 0);
    wait_and_close_child_process(&pi);

    tmp = jobs[0];
    jobs[0] = jobs[1];
    jobs[1] = tmp;

    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, jobs,
            2 * sizeof(*jobs), NULL, NULL);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT, NULL, NULL,
            (STARTUPINFOA *)&si, &pi);
    ok(!ret && GetLastError() == ERROR_ACCESS_DENIED, "Got unexpected ret %#x, GetLastError() %lu.\n",
            ret, GetLastError());

    CloseHandle(jobs[0]);
    CloseHandle(jobs[1]);

    jobs[0] = pCreateJobObjectW(NULL, NULL);
    ok(!!jobs[0], "CreateJobObjectA error %lu\n", GetLastError());
    jobs[1] = pCreateJobObjectW(NULL, NULL);
    ok(!!jobs[1], "CreateJobObjectA error %lu\n", GetLastError());

    /* Create the job chain successfully and check the job chain. */
    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, jobs,
            2 * sizeof(*jobs), NULL, NULL);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT,
            NULL, NULL, (STARTUPINFOA *)&si, &pi);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    ret = pIsProcessInJob(pi.hProcess, parent_job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    ret = pIsProcessInJob(pi.hProcess, jobs[0], &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    ret = pIsProcessInJob(pi.hProcess, jobs[1], &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    TerminateProcess(pi.hProcess, 0);
    wait_and_close_child_process(&pi);

    ret = pInitializeProcThreadAttributeList(attrs, 1, 0, &size);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());
    ret = pUpdateProcThreadAttribute(attrs, 0, PROC_THREAD_ATTRIBUTE_JOB_LIST, jobs + 1,
            sizeof(*jobs), NULL, NULL);
    ret = CreateProcessA(NULL, buffer, NULL, NULL, FALSE, EXTENDED_STARTUPINFO_PRESENT
            | CREATE_BREAKAWAY_FROM_JOB, NULL, NULL, (STARTUPINFOA *)&si, &pi);
    ok(ret, "Got unexpected ret %#x, GetLastError() %lu.\n", ret, GetLastError());

    ret = pIsProcessInJob(pi.hProcess, parent_job, &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    ret = pIsProcessInJob(pi.hProcess, jobs[0], &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    ret = pIsProcessInJob(pi.hProcess, jobs[1], &out);
    ok(ret, "IsProcessInJob error %lu\n", GetLastError());
    ok(out, "IsProcessInJob returned out=%u\n", out);

    TerminateProcess(pi.hProcess, 0);
    wait_and_close_child_process(&pi);

    CloseHandle(jobs[0]);
    CloseHandle(jobs[1]);

    pDeleteProcThreadAttributeList(attrs);
    heap_free(attrs);

    limit_info.BasicLimitInformation.LimitFlags = 0;
    ret = pSetInformationJobObject(parent_job, JobObjectExtendedLimitInformation, &limit_info, sizeof(limit_info));
    ok(ret, "SetInformationJobObject error %lu\n", GetLastError());
}

static void test_services_exe(void)
{
    NTSTATUS status;
    ULONG size, offset, try;
    char *buf;
    SYSTEM_PROCESS_INFORMATION *spi;
    ULONG services_pid = 0, services_session_id = ~0;

    /* Check that passing a zero size returns a size suitable for the next call,
     * taking into account that in rare cases processes may start between the
     * two NtQuerySystemInformation() calls. So this may require a few tries.
     */
    for (try = 0; try < 3; try++)
    {
        status = NtQuerySystemInformation(SystemProcessInformation, NULL, 0, &size);
        ok(status == STATUS_INFO_LENGTH_MISMATCH, "got %#lx\n", status);

        buf = malloc(size);
        status = NtQuerySystemInformation(SystemProcessInformation, buf, size, &size);
        if (status != STATUS_INFO_LENGTH_MISMATCH) break;
        free(buf);
    }
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);

    spi = (SYSTEM_PROCESS_INFORMATION *)buf;
    offset = 0;

    do
    {
        spi = (SYSTEM_PROCESS_INFORMATION *)(buf + offset);
        if (!wcsnicmp(spi->ProcessName.Buffer, L"services.exe", spi->ProcessName.Length/sizeof(WCHAR)))
        {
            services_pid = HandleToUlong(spi->UniqueProcessId);
            services_session_id = spi->SessionId;
        }
        offset += spi->NextEntryOffset;
    } while (spi->NextEntryOffset != 0);

    ok(services_pid != 0, "services.exe not found\n");
    todo_wine
    ok(services_session_id == 0, "got services.exe SessionId %lu\n", services_session_id);
}

static void test_startupinfo( void )
{
    STARTUPINFOA startup_beforeA, startup_afterA;
    STARTUPINFOW startup_beforeW, startup_afterW;
    RTL_USER_PROCESS_PARAMETERS *params;

    params = RtlGetCurrentPeb()->ProcessParameters;

    startup_beforeA.hStdInput = (HANDLE)0x56780000;
    GetStartupInfoA(&startup_beforeA);

    startup_beforeW.hStdInput = (HANDLE)0x12340000;
    GetStartupInfoW(&startup_beforeW);

    /* change a couple of fields in PEB */
    params->dwX = ~params->dwX;
    params->hStdInput = (HANDLE)~(DWORD_PTR)params->hStdInput;

    startup_afterA.hStdInput = (HANDLE)0x87650000;
    GetStartupInfoA(&startup_afterA);

    /* wharf... ansi version is cached... */
    ok(startup_beforeA.dwX == startup_afterA.dwX, "Unexpected field value\n");
    ok(startup_beforeA.dwFlags == startup_afterA.dwFlags, "Unexpected field value\n");
    ok(startup_beforeA.hStdInput == startup_afterA.hStdInput, "Unexpected field value\n");

    if (startup_beforeW.dwFlags & STARTF_USESTDHANDLES)
    {
        ok(startup_beforeA.hStdInput != NULL && startup_beforeA.hStdInput != INVALID_HANDLE_VALUE,
           "Unexpected field value\n");
        ok(startup_afterA.hStdInput != NULL && startup_afterA.hStdInput != INVALID_HANDLE_VALUE,
           "Unexpected field value\n");
    }
    else
    {
        ok(startup_beforeA.hStdInput == INVALID_HANDLE_VALUE, "Unexpected field value %p\n", startup_beforeA.hStdInput);
        ok(startup_afterA.hStdInput == INVALID_HANDLE_VALUE, "Unexpected field value %p\n", startup_afterA.hStdInput);
    }

    /* ... while unicode is not */
    startup_afterW.hStdInput = (HANDLE)0x43210000;
    GetStartupInfoW(&startup_afterW);

    ok(~startup_beforeW.dwX == startup_afterW.dwX, "Unexpected field value\n");
    if (startup_beforeW.dwFlags & STARTF_USESTDHANDLES)
    {
        ok(params->hStdInput == startup_afterW.hStdInput, "Unexpected field value\n");
        ok((HANDLE)~(DWORD_PTR)startup_beforeW.hStdInput == startup_afterW.hStdInput, "Unexpected field value\n");
    }
    else
    {
        ok(startup_beforeW.hStdInput == (HANDLE)0x12340000, "Unexpected field value\n");
        ok(startup_afterW.hStdInput == (HANDLE)0x43210000, "Unexpected field value\n");
    }

    /* check impact of STARTF_USESTDHANDLES bit */
    params->dwFlags ^= STARTF_USESTDHANDLES;

    startup_afterW.hStdInput = (HANDLE)0x43210000;
    GetStartupInfoW(&startup_afterW);

    ok((startup_beforeW.dwFlags ^ STARTF_USESTDHANDLES) == startup_afterW.dwFlags, "Unexpected field value\n");
    if (startup_afterW.dwFlags & STARTF_USESTDHANDLES)
    {
        ok(params->hStdInput == startup_afterW.hStdInput, "Unexpected field value\n");
        ok(startup_afterW.hStdInput != (HANDLE)0x43210000, "Unexpected field value\n");
    }
    else
    {
        ok(startup_afterW.hStdInput == (HANDLE)0x43210000, "Unexpected field value\n");
    }

    /* FIXME add more tests to check whether the dwFlags controls the returned
     * values (as done for STARTF_USESTDHANDLES) in unicode case.
     */

    /* reset the modified fields in PEB */
    params->dwX = ~params->dwX;
    params->hStdInput = (HANDLE)~(DWORD_PTR)params->hStdInput;
    params->dwFlags ^= STARTF_USESTDHANDLES;
}

static void test_GetProcessInformation(void)
{
    SYSTEM_SUPPORTED_PROCESSOR_ARCHITECTURES_INFORMATION machines[8];
    PROCESS_MACHINE_INFORMATION mi;
    NTSTATUS status;
    HANDLE process;
    unsigned int i;
    BOOL ret;

    if (!pGetProcessInformation)
    {
        win_skip("GetProcessInformation() is not available.\n");
        return;
    }

    SetLastError(0xdeadbeef);
    ret = pGetProcessInformation(GetCurrentProcess(), ProcessMachineTypeInfo, NULL, 0);
    if (!ret && GetLastError() == ERROR_INVALID_PARAMETER)
    {
        win_skip("GetProcessInformation(ProcessMachineTypeInfo) is not supported.\n"); /* < win11 */
        return;
    }
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_BAD_LENGTH, "Unexpected error %ld.\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pGetProcessInformation(GetCurrentProcess(), ProcessMachineTypeInfo, &mi, 0);
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_BAD_LENGTH, "Unexpected error %ld.\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pGetProcessInformation(GetCurrentProcess(), ProcessMachineTypeInfo, &mi, sizeof(mi) - 1);
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_BAD_LENGTH, "Unexpected error %ld.\n", GetLastError());
    SetLastError(0xdeadbeef);
    ret = pGetProcessInformation(GetCurrentProcess(), ProcessMachineTypeInfo, &mi, sizeof(mi) + 1);
    ok(!ret, "Unexpected return value %d.\n", ret);
    ok(GetLastError() == ERROR_BAD_LENGTH, "Unexpected error %ld.\n", GetLastError());

    ret = pGetProcessInformation(GetCurrentProcess(), ProcessMachineTypeInfo, &mi, sizeof(mi));
    ok(ret, "Unexpected return value %d.\n", ret);

#ifndef __REACTOS__ // Can't link
    process = GetCurrentProcess();
    status = NtQuerySystemInformationEx( SystemSupportedProcessorArchitectures, &process, sizeof(process),
            machines, sizeof(machines), NULL );
    ok(!status, "Failed to get architectures information.\n");
    for (i = 0; machines[i].Machine; i++)
    {
        if (machines[i].Process)
        {
            ok(mi.ProcessMachine == machines[i].Machine, "Unexpected process machine %#x.\n", mi.ProcessMachine);
            ok(!!(mi.MachineAttributes & UserEnabled) == machines[i].UserMode, "Unexpected attributes %#x.\n",
                    mi.MachineAttributes);
            ok(!!(mi.MachineAttributes & KernelEnabled) == machines[i].KernelMode, "Unexpected attributes %#x.\n",
                    mi.MachineAttributes);
            ok(!!(mi.MachineAttributes & Wow64Container) == machines[i].WoW64Container, "Unexpected attributes %#x.\n",
                    mi.MachineAttributes);
            ok(!(mi.MachineAttributes & ~(UserEnabled | KernelEnabled | Wow64Container)), "Unexpected attributes %#x.\n",
                    mi.MachineAttributes);
            break;
        }
    }
#endif
}

START_TEST(process)
{
    HANDLE job, hproc, h, h2;
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
        else if (!strcmp(myARGV[2], "sync") && myARGC >= 4)
        {
            HANDLE sem = OpenSemaphoreA(SYNCHRONIZE, FALSE, myARGV[3]);
            ok(sem != 0, "OpenSemaphoreA(%s) failed le=%lu\n", myARGV[3], GetLastError());
            if (sem)
            {
                DWORD ret = WaitForSingleObject(sem, 30000);
                ok(ret == WAIT_OBJECT_0, "WaitForSingleObject(%s) returned %lu\n", myARGV[3], ret);
                CloseHandle(sem);
            }
            return;
        }
        else if (!strcmp(myARGV[2], "exit"))
        {
            return;
        }
        else if (!strcmp(myARGV[2], "nested") && myARGC >= 4)
        {
            char                buffer[MAX_PATH + 26];
            STARTUPINFOA        startup;
            PROCESS_INFORMATION info;
            HANDLE hFile;

            memset(&startup, 0, sizeof(startup));
            startup.cb = sizeof(startup);
            startup.dwFlags = STARTF_USESHOWWINDOW;
            startup.wShowWindow = SW_SHOWNORMAL;

            sprintf(buffer, "\"%s\" process dump \"%s\"", selfname, myARGV[3]);
            ok(CreateProcessA(NULL, buffer, NULL, NULL, FALSE, CREATE_SUSPENDED, NULL, NULL, &startup, &info), "CreateProcess failed\n");
            CloseHandle(info.hProcess);
            CloseHandle(info.hThread);

            /* The nested process is suspended so we can use the same resource
             * file and it's up to the parent to read it before resuming the
             * nested process.
             */
            hFile = CreateFileA(myARGV[3], GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
            childPrintf(hFile, "[Nested]\nPid=%08lu\n", info.dwProcessId);
            CloseHandle(hFile);
            return;
        }
        else if (!strcmp(myARGV[2], "parent") && myARGC >= 5)
        {
            sscanf(myARGV[4], "%p", &h);
            test_parent_process_attribute(atoi(myARGV[3]), h);
            return;
        }
        else if (!strcmp(myARGV[2], "handlelist") && myARGC >= 5)
        {
            sscanf(myARGV[3], "%p", &h);
            sscanf(myARGV[4], "%p", &h2);
            test_handle_list_attribute(TRUE, h, h2);
            return;
        }
        else if (!strcmp(myARGV[2], "nested_jobs") && myARGC >= 4)
        {
            test_nested_jobs_child(atoi(myARGV[3]));
            return;
        }

        ok(0, "Unexpected command %s\n", myARGV[2]);
        return;
    }
    hproc = OpenProcess(PROCESS_QUERY_LIMITED_INFORMATION, FALSE, GetCurrentProcessId());
    if (hproc)
    {
        test_process_info(hproc);
        CloseHandle(hproc);
    }
    else
        win_skip("PROCESS_QUERY_LIMITED_INFORMATION is not supported on this platform\n");
    test_process_info(GetCurrentProcess());
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
    test_IsWow64Process2();
    test_SystemInfo();
    test_ProcessorCount();
    test_RegistryQuota();
    test_DuplicateHandle();
    test_StdHandleInheritance();
    test_GetNumaProcessorNode();
    test_session_info();
    test_GetLogicalProcessorInformationEx();
#ifndef __REACTOS__
    test_GetSystemCpuSetInformation();
#endif
    test_largepages();
    test_ProcThreadAttributeList();
    test_SuspendProcessState();
    test_SuspendProcessNewThread();
    test_parent_process_attribute(0, NULL);
    test_handle_list_attribute(FALSE, NULL, NULL);
    test_dead_process();
    test_services_exe();
    test_startupinfo();
    test_GetProcessInformation();

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
    test_nested_jobs();
    job = test_AddSelfToJob();
    test_jobInheritance(job);
    test_job_list_attribute(job);
    test_BreakawayOk(job);
    CloseHandle(job);
}
