/*
 * Task termination utility
 *
 * Copyright 2008 Andrew Riedi
 * Copyright 2010 Andrew Nguyen
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

#include <psapi.h>
#include <stdlib.h>
#include <windows.h>
#include <wine/debug.h>
#include <wine/unicode.h>

#include "taskkill.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskkill);

static BOOL force_termination = FALSE;

static WCHAR **ToKillTaskList;
static unsigned int ToKillTaskCount;

static WCHAR opForceTerminate[] = L"f";
static WCHAR opImage[] = L"im";
static WCHAR opPID[] = L"pid";
static WCHAR opHelp[] = L"?";
static WCHAR opTerminateChildren[] = L"t";

static PWCHAR opList[] = {opForceTerminate, opImage, opPID, opHelp, opTerminateChildren};

#define OP_PARAM_INVALID -1

#define OP_PARAM_FORCE_TERMINATE 0
#define OP_PARAM_IMAGE 1
#define OP_PARAM_PID 2
#define OP_PARAM_HELP 3
#define OP_PARAM_TERMITANE_CHILD 4

struct pid_close_info
{
    DWORD pid;
    BOOL found;
};

static int
TaskkillVPrintfW(const WCHAR *msg, __ms_va_list va_args)
{
    int wlen;
    DWORD count, ret;
    WCHAR msg_buffer[8192];

    wlen = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, msg, 0, 0, msg_buffer, ARRAY_SIZE(msg_buffer), &va_args);

    ret = WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), msg_buffer, wlen, &count, NULL);
    if (!ret)
    {
        DWORD len;
        char *msgA;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile(), assuming the console encoding is still the right
         * one in that case.
         */
        len = WideCharToMultiByte(GetConsoleOutputCP(), 0, msg_buffer, wlen, NULL, 0, NULL, NULL);
        msgA = HeapAlloc(GetProcessHeap(), 0, len);
        if (!msgA)
            return 0;

        WideCharToMultiByte(GetConsoleOutputCP(), 0, msg_buffer, wlen, msgA, len, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        HeapFree(GetProcessHeap(), 0, msgA);
    }

    return count;
}

static int WINAPIV
TaskkillPrintfW(const WCHAR *msg, ...)
{
    __ms_va_list va_args;
    int len;

    __ms_va_start(va_args, msg);
    len = TaskkillVPrintfW(msg, va_args);
    __ms_va_end(va_args);

    return len;
}

static int WINAPIV
TaskkillMessagePrintfW(int msg, ...)
{
    __ms_va_list va_args;
    WCHAR msg_buffer[8192];
    int len;

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));

    __ms_va_start(va_args, msg);
    len = TaskkillVPrintfW(msg_buffer, va_args);
    __ms_va_end(va_args);

    return len;
}

static int
TaskkillMessage(int msg)
{
    static const WCHAR formatW[] = {'%', '1', 0};
    WCHAR msg_buffer[8192];

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));

    return TaskkillPrintfW(formatW, msg_buffer);
}

/* Post WM_CLOSE to all top-level windows belonging to the process with specified PID. */
static BOOL CALLBACK
PIDEnumProc(HWND hwnd, LPARAM lParam)
{
    struct pid_close_info *info = (struct pid_close_info *)lParam;
    DWORD hwnd_pid;

    GetWindowThreadProcessId(hwnd, &hwnd_pid);

    if (hwnd_pid == info->pid)
    {
        PostMessageW(hwnd, WM_CLOSE, 0, 0);
        info->found = TRUE;
    }

    return TRUE;
}

static DWORD *
EnumerateProcesses(DWORD *list_count)
{
    DWORD *pid_list, alloc_bytes = 1024 * sizeof(*pid_list), needed_bytes;

    pid_list = HeapAlloc(GetProcessHeap(), 0, alloc_bytes);
    if (!pid_list)
        return NULL;

    for (;;)
    {
        DWORD *realloc_list;

        if (!EnumProcesses(pid_list, alloc_bytes, &needed_bytes))
        {
            HeapFree(GetProcessHeap(), 0, pid_list);
            return NULL;
        }

        /* EnumProcesses can't signal an insufficient buffer condition, so the
         * only way to possibly determine whether a larger buffer is required
         * is to see whether the written number of bytes is the same as the
         * buffer size. If so, the buffer will be reallocated to twice the
         * size. */
        if (alloc_bytes != needed_bytes)
            break;

        alloc_bytes *= 2;
        realloc_list = HeapReAlloc(GetProcessHeap(), 0, pid_list, alloc_bytes);
        if (!realloc_list)
        {
            HeapFree(GetProcessHeap(), 0, pid_list);
            return NULL;
        }
        pid_list = realloc_list;
    }

    *list_count = needed_bytes / sizeof(*pid_list);
    return pid_list;
}

static BOOL
GetProcessNameFromPID(DWORD pid, WCHAR *buf, DWORD chars)
{
    HANDLE process;
    HMODULE module;
    DWORD required_size;

    process = OpenProcess(PROCESS_QUERY_INFORMATION | PROCESS_VM_READ, FALSE, pid);
    if (!process)
        return FALSE;

    if (!EnumProcessModules(process, &module, sizeof(module), &required_size))
    {
        CloseHandle(process);
        return FALSE;
    }

    if (!GetModuleBaseNameW(process, module, buf, chars))
    {
        CloseHandle(process);
        return FALSE;
    }

    CloseHandle(process);
    return TRUE;
}

/* The implemented task enumeration and termination behavior does not
 * exactly match native behavior. On Windows:
 *
 * In the case of terminating by process name, specifying a particular
 * process name more times than the number of running instances causes
 * all instances to be terminated, but termination failure messages to
 * be printed as many times as the difference between the specification
 * quantity and the number of running instances.
 *
 * Successful terminations are all listed first in order, with failing
 * terminations being listed at the end.
 *
 * A PID of zero causes taskkill to warn about the inability to terminate
 * system processes. */
static int
SendCloseMessages(void)
{
    DWORD *pid_list, pid_list_size;
    DWORD self_pid = GetCurrentProcessId();
    unsigned int i;
    int status_code = 0;

    pid_list = EnumerateProcesses(&pid_list_size);
    if (!pid_list)
    {
        TaskkillMessage(STRING_ENUM_FAILED);
        return 1;
    }

    for (i = 0; i < ToKillTaskCount; i++)
    {
        WCHAR *p = ToKillTaskList[i];
        BOOL is_numeric = TRUE;

        /* Determine whether the string is not numeric. */
        while (*p)
        {
            if (!isdigitW(*p++))
            {
                is_numeric = FALSE;
                break;
            }
        }

        if (is_numeric)
        {
            DWORD pid = atoiW(ToKillTaskList[i]);
            struct pid_close_info info = {pid};

            if (pid == self_pid)
            {
                TaskkillMessage(STRING_SELF_TERMINATION);
                status_code = 1;
                continue;
            }

            EnumWindows(PIDEnumProc, (LPARAM)&info);
            if (info.found)
                TaskkillMessagePrintfW(STRING_CLOSE_PID_SEARCH, pid);
            else
            {
                TaskkillMessagePrintfW(STRING_SEARCH_FAILED, ToKillTaskList[i]);
                status_code = 128;
            }
        }
        else
        {
            DWORD index;
            BOOL found_process = FALSE;

            for (index = 0; index < pid_list_size; index++)
            {
                WCHAR process_name[MAX_PATH];

                if (GetProcessNameFromPID(pid_list[index], process_name, MAX_PATH) &&
                    !strcmpiW(process_name, ToKillTaskList[i]))
                {
                    struct pid_close_info info = {pid_list[index]};

                    found_process = TRUE;
                    if (pid_list[index] == self_pid)
                    {
                        TaskkillMessage(STRING_SELF_TERMINATION);
                        status_code = 1;
                        continue;
                    }

                    EnumWindows(PIDEnumProc, (LPARAM)&info);
                    TaskkillMessagePrintfW(STRING_CLOSE_PROC_SRCH, process_name, pid_list[index]);
                }
            }

            if (!found_process)
            {
                TaskkillMessagePrintfW(STRING_SEARCH_FAILED, ToKillTaskList[i]);
                status_code = 128;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, pid_list);
    return status_code;
}

static int
TerminateToKillProcesses(void)
{
    DWORD *pid_list, pid_list_size;
    DWORD self_pid = GetCurrentProcessId();
    unsigned int i;
    int status_code = 0;

    pid_list = EnumerateProcesses(&pid_list_size);
    if (!pid_list)
    {
        TaskkillMessage(STRING_ENUM_FAILED);
        return 1;
    }

    for (i = 0; i < ToKillTaskCount; i++)
    {
        WCHAR *p = ToKillTaskList[i];
        BOOL is_numeric = TRUE;

        /* Determine whether the string is not numeric. */
        while (*p)
        {
            if (!isdigitW(*p++))
            {
                is_numeric = FALSE;
                break;
            }
        }

        if (is_numeric)
        {
            DWORD pid = atoiW(ToKillTaskList[i]);
            HANDLE process;

            if (pid == self_pid)
            {
                TaskkillMessage(STRING_SELF_TERMINATION);
                status_code = 1;
                continue;
            }

            process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (!process)
            {
                TaskkillMessagePrintfW(STRING_SEARCH_FAILED, ToKillTaskList[i]);
                status_code = 128;
                continue;
            }

            if (!TerminateProcess(process, 0))
            {
                TaskkillMessagePrintfW(STRING_TERMINATE_FAILED, ToKillTaskList[i]);
                status_code = 1;
                CloseHandle(process);
                continue;
            }

            TaskkillMessagePrintfW(STRING_TERM_PID_SEARCH, pid);
            CloseHandle(process);
        }
        else
        {
            DWORD index;
            BOOL found_process = FALSE;

            for (index = 0; index < pid_list_size; index++)
            {
                WCHAR process_name[MAX_PATH];

                if (GetProcessNameFromPID(pid_list[index], process_name, MAX_PATH) &&
                    !strcmpiW(process_name, ToKillTaskList[i]))
                {
                    HANDLE process;

                    if (pid_list[index] == self_pid)
                    {
                        TaskkillMessage(STRING_SELF_TERMINATION);
                        status_code = 1;
                        continue;
                    }

                    process = OpenProcess(PROCESS_TERMINATE, FALSE, pid_list[index]);
                    if (!process)
                    {
                        TaskkillMessagePrintfW(STRING_SEARCH_FAILED, ToKillTaskList[i]);
                        status_code = 128;
                        continue;
                    }

                    if (!TerminateProcess(process, 0))
                    {
                        TaskkillMessagePrintfW(STRING_TERMINATE_FAILED, ToKillTaskList[i]);
                        status_code = 1;
                        CloseHandle(process);
                        continue;
                    }

                    found_process = TRUE;
                    TaskkillMessagePrintfW(STRING_TERM_PROC_SEARCH, ToKillTaskList[i], pid_list[index]);
                    CloseHandle(process);
                }
            }

            if (!found_process)
            {
                TaskkillMessagePrintfW(STRING_SEARCH_FAILED, ToKillTaskList[i]);
                status_code = 128;
            }
        }
    }

    HeapFree(GetProcessHeap(), 0, pid_list);
    return status_code;
}

static BOOL
ToKillTaskListAdd(WCHAR *name)
{
    static unsigned int list_size = 16;

    if (!ToKillTaskList)
    {
        ToKillTaskList = HeapAlloc(GetProcessHeap(), 0, list_size * sizeof(*ToKillTaskList));
        if (!ToKillTaskList)
            return FALSE;
    }
    else if (ToKillTaskCount == list_size)
    {
        void *realloc_list;

        list_size *= 2;
        realloc_list = HeapReAlloc(GetProcessHeap(), 0, ToKillTaskList, list_size * sizeof(*ToKillTaskList));
        if (!realloc_list)
            return FALSE;

        ToKillTaskList = realloc_list;
    }

    ToKillTaskList[ToKillTaskCount++] = name;
    return TRUE;
}

static int
GetArgumentType(WCHAR *argument)
{
    int i;

    if (argument[0] != L'/' && argument[0] != L'-')
    {
        return OP_PARAM_INVALID;
    }
    argument++;

    for (i = 0; i < _countof(opList); i++)
    {
        if (!strcmpiW(opList[i], argument))
        {
            return i;
        }
    }
    return OP_PARAM_INVALID;
}

/* FIXME
Argument T not supported

*/
static BOOL
ProcessArguments(int argc, WCHAR *argv[])
{
    BOOL has_im = FALSE, has_pid = FALSE, has_help = FALSE;

    if (argc > 1)
    {
        int i;
        for (i = 1; i < argc; i++)
        {
            int Argument = GetArgumentType(argv[i]);

            switch (Argument)
            {
                case OP_PARAM_FORCE_TERMINATE:
                {
                    if (force_termination == TRUE)
                    {
                        // -f already specified
                        TaskkillMessagePrintfW(STRING_PARAM_TOO_MUCH, argv[i], 1);
                        TaskkillMessage(STRING_USAGE);
                        return FALSE;
                    }
                    force_termination = TRUE;
                    break;
                }
                case OP_PARAM_IMAGE:
                case OP_PARAM_PID:
                {
                    if (!argv[i + 1])
                    {
                        TaskkillMessagePrintfW(STRING_MISSING_PARAM, argv[i]);
                        TaskkillMessage(STRING_USAGE);
                        return FALSE;
                    }

                    if (Argument == OP_PARAM_IMAGE)
                        has_im = TRUE;
                    if (Argument == OP_PARAM_PID)
                        has_pid = TRUE;

                    if (has_im && has_pid)
                    {
                        TaskkillMessage(STRING_MUTUAL_EXCLUSIVE);
                        TaskkillMessage(STRING_USAGE);
                        return FALSE;
                    }

                    if (GetArgumentType(argv[i + 1]) != OP_PARAM_INVALID)
                    {
                        TaskkillMessagePrintfW(STRING_MISSING_PARAM, argv[i]);
                        TaskkillMessage(STRING_USAGE);
                        return FALSE;
                    }

                    if (!ToKillTaskListAdd(argv[++i])) // add next parameters to ToKillTaskList
                        return FALSE;

                    break;
                }
                case OP_PARAM_HELP:
                {
                    if (has_help == TRUE)
                    {
                        // -? already specified
                        TaskkillMessagePrintfW(STRING_PARAM_TOO_MUCH, argv[i], 1);
                        TaskkillMessage(STRING_USAGE);
                        return FALSE;
                    }
                    has_help = TRUE;
                    break;
                }
                case OP_PARAM_TERMITANE_CHILD:
                {
                    WINE_FIXME("argument T not supported\n");
                    break;
                }
                case OP_PARAM_INVALID:
                default:
                {
                    TaskkillMessage(STRING_INVALID_OPTION);
                    TaskkillMessage(STRING_USAGE);
                    return FALSE;
                }
            }
        }
    }

    if (has_help)
    {
        if (argc > 2) // any parameters other than -? is specified
        {
            TaskkillMessage(STRING_INVALID_SYNTAX);
            TaskkillMessage(STRING_USAGE);
            return FALSE;
        }
        else
        {
            TaskkillMessage(STRING_USAGE);
            exit(0);
        }
    }
    else if ((!has_im) && (!has_pid)) // has_help == FALSE
    {
        // both has_im and has_pid are missing (maybe -fi option is missing too, if implemented later)
        TaskkillMessage(STRING_MISSING_OPTION);
        TaskkillMessage(STRING_USAGE);
        return FALSE;
    }

    return TRUE;
}

int
wmain(int argc, WCHAR *argv[])
{
    int StatusCode = 0;

    if (!ProcessArguments(argc, argv))
    {
        HeapFree(GetProcessHeap(), 0, ToKillTaskList);
        return 1;
    }

    if (force_termination)
        StatusCode = TerminateToKillProcesses();
    else
        StatusCode = SendCloseMessages();

    HeapFree(GetProcessHeap(), 0, ToKillTaskList);
    return StatusCode;
}
