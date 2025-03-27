/*
 * Task termination utility
 *
 * Copyright 2008 Andrew Riedi
 * Copyright 2010 Andrew Nguyen
 * Copyright 2020 He Yang
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

#include <stdlib.h>
#include <windows.h>
#include <tlhelp32.h>
#include <wine/debug.h>

#ifdef __REACTOS__
#define wcsicmp _wcsicmp
#endif

#include "taskkill.h"

WINE_DEFAULT_DEBUG_CHANNEL(taskkill);

static BOOL force_termination = FALSE;
static BOOL kill_child_processes = FALSE;

static WCHAR **task_list;
static unsigned int task_count;

static struct
{
    PROCESSENTRY32W p;
    BOOL matched;
}
*process_list;
static unsigned int process_count;

struct pid_close_info
{
    DWORD pid;
    BOOL found;
};

#ifdef __REACTOS__

static PWCHAR opList[] = {L"f", L"im", L"pid", L"?", L"t"};

#define OP_PARAM_INVALID -1

#define OP_PARAM_FORCE_TERMINATE 0
#define OP_PARAM_IMAGE 1
#define OP_PARAM_PID 2
#define OP_PARAM_HELP 3
#define OP_PARAM_TERMINATE_CHILD 4

#endif // __REACTOS__

static int taskkill_vprintfW(const WCHAR *msg, va_list va_args)
{
    int wlen;
    DWORD count;
    WCHAR msg_buffer[8192];

    wlen = FormatMessageW(FORMAT_MESSAGE_FROM_STRING, msg, 0, 0, msg_buffer,
                          ARRAY_SIZE(msg_buffer), &va_args);

    if (!WriteConsoleW(GetStdHandle(STD_OUTPUT_HANDLE), msg_buffer, wlen, &count, NULL))
    {
        DWORD len;
        char *msgA;

        /* On Windows WriteConsoleW() fails if the output is redirected. So fall
         * back to WriteFile() using OEM code page.
         */
        len = WideCharToMultiByte(GetOEMCP(), 0, msg_buffer, wlen,
            NULL, 0, NULL, NULL);
        msgA = malloc(len);
        if (!msgA)
            return 0;

        WideCharToMultiByte(GetOEMCP(), 0, msg_buffer, wlen, msgA, len, NULL, NULL);
        WriteFile(GetStdHandle(STD_OUTPUT_HANDLE), msgA, len, &count, FALSE);
        free(msgA);
    }

    return count;
}

static int WINAPIV taskkill_printfW(const WCHAR *msg, ...)
{
    va_list va_args;
    int len;

    va_start(va_args, msg);
    len = taskkill_vprintfW(msg, va_args);
    va_end(va_args);

    return len;
}

static int WINAPIV taskkill_message_printfW(int msg, ...)
{
    va_list va_args;
    WCHAR msg_buffer[8192];
    int len;

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));

    va_start(va_args, msg);
    len = taskkill_vprintfW(msg_buffer, va_args);
    va_end(va_args);

    return len;
}

static int taskkill_message(int msg)
{
    WCHAR msg_buffer[8192];

    LoadStringW(GetModuleHandleW(NULL), msg, msg_buffer, ARRAY_SIZE(msg_buffer));

    return taskkill_printfW(L"%1", msg_buffer);
}

/* Post WM_CLOSE to all top-level windows belonging to the process with specified PID. */
static BOOL CALLBACK pid_enum_proc(HWND hwnd, LPARAM lParam)
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

static BOOL enumerate_processes(void)
{
    unsigned int alloc_count = 128;
    void *realloc_list;
    HANDLE snapshot;

    snapshot = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (snapshot == INVALID_HANDLE_VALUE)
        return FALSE;

    process_list = malloc(alloc_count * sizeof(*process_list));
    if (!process_list)
        return FALSE;

    process_list[0].p.dwSize = sizeof(process_list[0].p);
    if (!Process32FirstW(snapshot, &process_list[0].p))
        return FALSE;

    do
    {
        process_list[process_count++].matched = FALSE;
        if (process_count == alloc_count)
        {
            alloc_count *= 2;
            realloc_list = realloc(process_list, alloc_count * sizeof(*process_list));
            if (!realloc_list)
                return FALSE;
            process_list = realloc_list;
        }
        process_list[process_count].p.dwSize = sizeof(process_list[process_count].p);
    } while (Process32NextW(snapshot, &process_list[process_count].p));
    CloseHandle(snapshot);
    return TRUE;
}

#ifdef __REACTOS__
static BOOL get_task_pid(const WCHAR *str, BOOL *is_numeric, WCHAR *process_name, int *status_code,
    DWORD self_pid, unsigned int* pkill_list, DWORD *pkill_size)
#else
static BOOL get_task_pid(const WCHAR *str, BOOL *is_numeric, WCHAR *process_name, int *status_code, DWORD *pid)
#endif
{
#ifndef __REACTOS__
    DWORD self_pid = GetCurrentProcessId();
#endif
    const WCHAR *p = str;
    unsigned int i;

#ifdef __REACTOS__
    *pkill_size = 0;
    memset(pkill_list, 0, process_count * sizeof(unsigned int*));
#endif

    *is_numeric = TRUE;
    while (*p)
    {
        if (!iswdigit(*p++))
        {
            *is_numeric = FALSE;
            break;
        }
    }

    if (*is_numeric)
    {
#ifdef __REACTOS__
        DWORD pid = wcstol(str, NULL, 10);
#else
        *pid = wcstol(str, NULL, 10);
#endif
        for (i = 0; i < process_count; ++i)
        {
#ifdef __REACTOS__
            if (process_list[i].p.th32ProcessID == pid)
#else
            if (process_list[i].p.th32ProcessID == *pid)
#endif
                break;
        }
        if (i == process_count || process_list[i].matched)
            goto not_found;
        process_list[i].matched = TRUE;
#ifdef __REACTOS__
        if (pid == self_pid)
#else
        if (*pid == self_pid)
#endif
        {
            taskkill_message(STRING_SELF_TERMINATION);
            *status_code = 1;
            return FALSE;
        }
#ifdef __REACTOS__
        pkill_list[*pkill_size] = i;
        (*pkill_size)++;
#endif
        return TRUE;
    }

    for (i = 0; i < process_count; ++i)
    {
        if (!wcsicmp(process_list[i].p.szExeFile, str) && !process_list[i].matched)
        {
            process_list[i].matched = TRUE;
            if (process_list[i].p.th32ProcessID == self_pid)
            {
                taskkill_message(STRING_SELF_TERMINATION);
                *status_code = 1;
                return FALSE;
            }
#ifdef __REACTOS__
            pkill_list[*pkill_size] = i;
            (*pkill_size)++;
#else
            *pid = process_list[i].p.th32ProcessID;
            wcscpy(process_name, process_list[i].p.szExeFile);
            return TRUE;
#endif
        }
    }

#ifdef __REACTOS__
    // Cannot find any process matching the PID or name
    if (*pkill_size == 0)
    {
#endif
not_found:
    taskkill_message_printfW(STRING_SEARCH_FAILED, str);
    *status_code = 128;
    return FALSE;
#ifdef __REACTOS__
    }
    return TRUE;
#endif
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

static BOOL get_pid_creation_time(DWORD pid, FILETIME *time)
{
    HANDLE process;
    FILETIME t1 = { 0 }, t2 = { 0 }, t3 = { 0 };

    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!process)
    {
        return FALSE;
    }

    if (!GetProcessTimes(process, time, &t1, &t2, &t3))
    {
        CloseHandle(process);
        return FALSE;
    }

    CloseHandle(process);
    return TRUE;
}

static void send_close_messages_tree(DWORD ppid)
{
    FILETIME parent_creation_time = { 0 };
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe = { 0 };
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (!get_pid_creation_time(ppid, &parent_creation_time) || !h)
    {
        CloseHandle(h);
        return;
    }

    if (Process32FirstW(h, &pe))
    {
        do
        {
            FILETIME child_creation_time = { 0 };
            struct pid_close_info info = { pe.th32ProcessID };

            if (!get_pid_creation_time(pe.th32ProcessID, &child_creation_time))
            {
                continue;
            }

            // Compare creation time to avoid reuse PID, thanks to @ThFabba
            if (pe.th32ParentProcessID == ppid &&
                CompareFileTime(&parent_creation_time, &child_creation_time) < 0)
            {
                // Use recursion to browse all child processes
                send_close_messages_tree(pe.th32ProcessID);
                EnumWindows(pid_enum_proc, (LPARAM)&info);
                if (info.found)
                {
                    taskkill_message_printfW(STRING_CLOSE_CHILD, pe.th32ProcessID, ppid);
                }
            }
        } while (Process32NextW(h, &pe));
    }

    CloseHandle(h);
}

static int send_close_messages(void)
{
    WCHAR process_name[MAX_PATH];
    struct pid_close_info info;
    unsigned int i;
    int status_code = 0;
    BOOL is_numeric;

#ifdef __REACTOS__
    DWORD self_pid = GetCurrentProcessId();
    unsigned int* pkill_list = malloc(process_count * sizeof(unsigned int*));
    if (!pkill_list)
        return 1;
#endif

    for (i = 0; i < task_count; i++)
    {
#ifdef __REACTOS__
        DWORD pkill_size, index;
        if (!get_task_pid(task_list[i], &is_numeric, process_name, &status_code,
                          self_pid, pkill_list, &pkill_size))
#else
        if (!get_task_pid(task_list[i], &is_numeric, process_name, &status_code, &info.pid))
#endif
            continue;

#ifdef __REACTOS__
        // Try to send close messages to process in `pkill_list`
        for (index = 0; index < pkill_size; index++)
        {
            // struct pid_close_info info = { process_list[pkill_list[index]].p.th32ProcessID };
            info.pid = process_list[pkill_list[index]].p.th32ProcessID;
            if (info.pid == self_pid)
            {
                taskkill_message(STRING_SELF_TERMINATION);
                status_code = 1;
                continue;
            }

            // Send close messages to child first
            if (kill_child_processes)
                send_close_messages_tree(info.pid);

            wcscpy(process_name, process_list[pkill_list[index]].p.szExeFile);
#endif
            info.found = FALSE;
            EnumWindows(pid_enum_proc, (LPARAM)&info);
            if (info.found)
            {
                if (is_numeric)
                    taskkill_message_printfW(STRING_CLOSE_PID_SEARCH, info.pid);
                else
                    taskkill_message_printfW(STRING_CLOSE_PROC_SRCH, process_name, info.pid);
                continue;
            }
            taskkill_message_printfW(STRING_SEARCH_FAILED, task_list[i]);
            status_code = 128;
#ifdef __REACTOS__
        }
#endif
    }

    return status_code;
}

static void terminate_process_tree(DWORD ppid)
{
    FILETIME parent_creation_time = { 0 };
    HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    PROCESSENTRY32W pe = { 0 };
    pe.dwSize = sizeof(PROCESSENTRY32W);

    if (!get_pid_creation_time(ppid, &parent_creation_time) || !h)
    {
        CloseHandle(h);
        return;
    }

    if (Process32FirstW(h, &pe))
    {
        do
        {
            FILETIME child_creation_time = { 0 };

            if (!get_pid_creation_time(pe.th32ProcessID, &child_creation_time))
            {
                continue;
            }

            // Compare creation time to avoid reuse PID, thanks to @ThFabba
            if (pe.th32ParentProcessID == ppid &&
                CompareFileTime(&parent_creation_time, &child_creation_time) < 0)
            {
                HANDLE process;

                // Use recursion to browse all child processes
                terminate_process_tree(pe.th32ProcessID);
                process = OpenProcess(PROCESS_TERMINATE, FALSE, pe.th32ProcessID);
                if (!process)
                {
                    continue;
                }

                if (!TerminateProcess(process, 1))
                {
                    taskkill_message_printfW(STRING_TERM_CHILD_FAILED, pe.th32ProcessID, ppid);
                    CloseHandle(process);
                    continue;
                }

                taskkill_message_printfW(STRING_TERM_CHILD, pe.th32ProcessID, ppid);
                CloseHandle(process);
            }
        } while (Process32NextW(h, &pe));
    }

    CloseHandle(h);
}

static int terminate_processes(void)
{
    WCHAR process_name[MAX_PATH];
    unsigned int i;
    int status_code = 0;
    BOOL is_numeric;
    HANDLE process;
    DWORD pid;

#ifdef __REACTOS__
    DWORD self_pid = GetCurrentProcessId();
    unsigned int* pkill_list = malloc(process_count * sizeof(unsigned int*));
    if (!pkill_list)
        return 1;
#endif

    for (i = 0; i < task_count; i++)
    {
#ifdef __REACTOS__
        DWORD pkill_size, index;
        if (!get_task_pid(task_list[i], &is_numeric, process_name, &status_code,
                          self_pid, pkill_list, &pkill_size))
#else
        if (!get_task_pid(task_list[i], &is_numeric, process_name, &status_code, &pid))
#endif
            continue;

#ifdef __REACTOS__
        // Try to terminate to process in `pkill_list`
        for (index = 0; index < pkill_size; index++)
        {
            pid = process_list[pkill_list[index]].p.th32ProcessID;
            if (pid == self_pid)
            {
                taskkill_message(STRING_SELF_TERMINATION);
                status_code = 1;
                continue;
            }

            // Terminate child first
            if (kill_child_processes)
                terminate_process_tree(pid);
#endif
            process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
            if (!process)
            {
                taskkill_message_printfW(STRING_SEARCH_FAILED, task_list[i]);
                status_code = 128;
                continue;
            }
#ifdef __REACTOS__
            wcscpy(process_name, process_list[pkill_list[index]].p.szExeFile);
#endif

            if (!TerminateProcess(process, 1))
            {
#ifdef __REACTOS__
                taskkill_message_printfW(STRING_TERMINATE_FAILED, process_name);
#else
                taskkill_message_printfW(STRING_TERMINATE_FAILED, task_list[i]);
#endif
                status_code = 1;
                CloseHandle(process);
                continue;
            }
            if (is_numeric)
                taskkill_message_printfW(STRING_TERM_PID_SEARCH, pid);
            else
#ifdef __REACTOS__
                taskkill_message_printfW(STRING_TERM_PROC_SEARCH, process_name, pid);
#else
                taskkill_message_printfW(STRING_TERM_PROC_SEARCH, task_list[i], pid);
#endif
            CloseHandle(process);
#ifdef __REACTOS__
        }
#endif
    }
    return status_code;
}

static BOOL add_to_task_list(WCHAR *name)
{
    static unsigned int list_size = 16;

    if (!task_list)
    {
        task_list = malloc(list_size * sizeof(*task_list));
        if (!task_list)
            return FALSE;
    }
    else if (task_count == list_size)
    {
        void *realloc_list;

        list_size *= 2;
        realloc_list = realloc(task_list, list_size * sizeof(*task_list));
        if (!realloc_list)
            return FALSE;

        task_list = realloc_list;
    }

    task_list[task_count++] = name;
    return TRUE;
}

#ifdef __REACTOS__

static int get_argument_type(WCHAR* argument)
{
    int i;

    if (argument[0] != L'/' && argument[0] != L'-')
    {
        return OP_PARAM_INVALID;
    }
    argument++;

    for (i = 0; i < _countof(opList); i++)
    {
        if (!wcsicmp(opList[i], argument))
        {
            return i;
        }
    }
    return OP_PARAM_INVALID;
}

static BOOL process_arguments(int argc, WCHAR* argv[])
{
    BOOL has_im = FALSE, has_pid = FALSE, has_help = FALSE;

    if (argc > 1)
    {
        int i;
        for (i = 1; i < argc; i++)
        {
            int argument = get_argument_type(argv[i]);

            switch (argument)
            {
            case OP_PARAM_FORCE_TERMINATE:
            {
                if (force_termination == TRUE)
                {
                    // -f already specified
                    taskkill_message_printfW(STRING_PARAM_TOO_MUCH, argv[i], 1);
                    taskkill_message(STRING_USAGE);
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
                    taskkill_message_printfW(STRING_MISSING_PARAM, argv[i]);
                    taskkill_message(STRING_USAGE);
                    return FALSE;
                }

                if (argument == OP_PARAM_IMAGE)
                    has_im = TRUE;
                if (argument == OP_PARAM_PID)
                    has_pid = TRUE;

                if (has_im && has_pid)
                {
                    taskkill_message(STRING_MUTUAL_EXCLUSIVE);
                    taskkill_message(STRING_USAGE);
                    return FALSE;
                }

                if (get_argument_type(argv[i + 1]) != OP_PARAM_INVALID)
                {
                    taskkill_message_printfW(STRING_MISSING_PARAM, argv[i]);
                    taskkill_message(STRING_USAGE);
                    return FALSE;
                }

                if (!add_to_task_list(argv[++i])) // add next parameters to task_list
                    return FALSE;

                break;
            }
            case OP_PARAM_HELP:
            {
                if (has_help == TRUE)
                {
                    // -? already specified
                    taskkill_message_printfW(STRING_PARAM_TOO_MUCH, argv[i], 1);
                    taskkill_message(STRING_USAGE);
                    return FALSE;
                }
                has_help = TRUE;
                break;
            }
            case OP_PARAM_TERMINATE_CHILD:
            {
                if (kill_child_processes == TRUE)
                {
                    // -t already specified
                    taskkill_message_printfW(STRING_PARAM_TOO_MUCH, argv[i], 1);
                    taskkill_message(STRING_USAGE);
                    return FALSE;
                }
                kill_child_processes = TRUE;
                break;
            }
            case OP_PARAM_INVALID:
            default:
            {
                taskkill_message(STRING_INVALID_OPTION);
                taskkill_message(STRING_USAGE);
                return FALSE;
            }
            }
        }
    }

    if (has_help)
    {
        if (argc > 2) // any parameters other than -? is specified
        {
            taskkill_message(STRING_INVALID_SYNTAX);
            taskkill_message(STRING_USAGE);
            return FALSE;
        }
        else
        {
            taskkill_message(STRING_USAGE);
            exit(0);
        }
    }
    else if ((!has_im) && (!has_pid)) // has_help == FALSE
    {
        // both has_im and has_pid are missing (maybe -fi option is missing too, if implemented later)
        taskkill_message(STRING_MISSING_OPTION);
        taskkill_message(STRING_USAGE);
        return FALSE;
    }

    return TRUE;
}

#else

/* FIXME Argument processing does not match behavior observed on Windows.
 * Stringent argument counting and processing is performed, and unrecognized
 * options are detected as parameters when placed after options that accept one. */
static BOOL process_arguments(int argc, WCHAR *argv[])
{
    if (argc > 1)
    {
        int i;
        WCHAR *argdata;
        BOOL has_im = FALSE, has_pid = FALSE;

        /* Only the lone help option is recognized. */
        if (argc == 2)
        {
            argdata = argv[1];
            if ((*argdata == '/' || *argdata == '-') && !lstrcmpW(L"?", argdata + 1))
            {
                taskkill_message(STRING_USAGE);
                exit(0);
            }
        }

        for (i = 1; i < argc; i++)
        {
            BOOL got_im = FALSE, got_pid = FALSE;

            argdata = argv[i];
            if (*argdata != '/' && *argdata != '-')
                goto invalid;
            argdata++;

            if (!wcsicmp(L"t", argdata))
                kill_child_processes = TRUE;
            else if (!wcsicmp(L"f", argdata))
                force_termination = TRUE;
            /* Options /IM and /PID appear to behave identically, except for
             * the fact that they cannot be specified at the same time. */
            else if ((got_im = !wcsicmp(L"im", argdata)) ||
                     (got_pid = !wcsicmp(L"pid", argdata)))
            {
                if (!argv[i + 1])
                {
                    taskkill_message_printfW(STRING_MISSING_PARAM, argv[i]);
                    taskkill_message(STRING_USAGE);
                    return FALSE;
                }

                if (got_im) has_im = TRUE;
                if (got_pid) has_pid = TRUE;

                if (has_im && has_pid)
                {
                    taskkill_message(STRING_MUTUAL_EXCLUSIVE);
                    taskkill_message(STRING_USAGE);
                    return FALSE;
                }

                if (!add_to_task_list(argv[i + 1]))
                    return FALSE;
                i++;
            }
            else
            {
                invalid:
                taskkill_message(STRING_INVALID_OPTION);
                taskkill_message(STRING_USAGE);
                return FALSE;
            }
        }
    }
    else
    {
        taskkill_message(STRING_MISSING_OPTION);
        taskkill_message(STRING_USAGE);
        return FALSE;
    }

    return TRUE;
}

#endif // __REACTOS__

int wmain(int argc, WCHAR *argv[])
{
    if (!process_arguments(argc, argv))
        return 1;

    if (!enumerate_processes())
    {
        taskkill_message(STRING_ENUM_FAILED);
        return 1;
    }

    if (force_termination)
        return terminate_processes();
    return send_close_messages();
}
