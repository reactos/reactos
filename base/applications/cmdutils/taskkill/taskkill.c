/*
 * Task termination utility
 *
 * Copyright 2008 Andrew Riedi
 * Copyright 2010 Andrew Nguyen
 * Copyright 2020 He Yang
 * Copyright 2025 Hermès Bélusca-Maïto
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
    BOOL is_numeric;
}
*process_list;
static unsigned int process_count;

struct pid_close_info
{
    DWORD pid;
    BOOL found;
};

#ifdef __REACTOS__
unsigned int* pkill_list;
DWORD pkill_size;
DWORD self_pid;
#endif

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
        process_list[process_count].is_numeric = FALSE;
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

static void mark_task_process(const WCHAR *str, int *status_code)
{
#ifndef __REACTOS__
    DWORD self_pid = GetCurrentProcessId();
#endif
    const WCHAR *p = str;
    BOOL is_numeric;
    unsigned int i;
    DWORD pid;

    is_numeric = TRUE;
    while (*p)
    {
        if (!iswdigit(*p++))
        {
            is_numeric = FALSE;
            break;
        }
    }

    if (is_numeric)
    {
        pid = wcstol(str, NULL, 10);
        for (i = 0; i < process_count; ++i)
        {
            if (process_list[i].p.th32ProcessID == pid)
                break;
        }
        if (i == process_count || process_list[i].matched)
            goto not_found;
        process_list[i].matched = TRUE;
        process_list[i].is_numeric = TRUE;
        if (pid == self_pid)
        {
            taskkill_message(STRING_SELF_TERMINATION);
            *status_code = 1;
        }
#ifdef __REACTOS__
        else
        {
            pkill_list[pkill_size++] = i;
        }
#endif
        return;
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
            }
#ifdef __REACTOS__
            else
            {
                pkill_list[pkill_size++] = i;
                continue;
            }
#endif
            return;
        }
    }

#ifdef __REACTOS__
    // Cannot find any process matching the PID or name
    if (pkill_size == 0)
    {
#endif
not_found:
    taskkill_message_printfW(STRING_SEARCH_FAILED, str);
    *status_code = 128;
#ifdef __REACTOS__
    }
#endif
}

static void taskkill_message_print_process(int msg, unsigned int index)
{
    WCHAR pid_str[16];

    if (!process_list[index].is_numeric)
    {
        taskkill_message_printfW(msg, process_list[index].p.szExeFile);
        return;
    }
    wsprintfW(pid_str, L"%lu", process_list[index].p.th32ProcessID);
    taskkill_message_printfW(msg, pid_str);
}

#ifndef __REACTOS__
/*
 * Below is the Wine method of terminating child processes.
 * Its problem is that it doesn't terminate them in either a parent-to-children
 * or children-to-parent relationship, but instead in the order in which they
 * appear in the process list. This differs from Windows' (or ReactOS) method.
 * Wine's termination ordering can cause problems in scenarii where e.g. a
 * parent process could re-spawn killed children processes, or, where it is
 * of interest to kill the parent process first and then its children.
 *
 * NOTE: The following two functions implicitly assume that the process list
 * obtained from the system, is such that any child process P[j] of a given
 * parent process P[i] is enumerated *AFTER* its parent (i.e. i < j).
 *
 * Because of these facts, the ReactOS method is employed instead.
 * Note however that the Wine method (below) has been adapted for
 * ease of usage and comparison with that of ReactOS.
 */

static BOOL find_parent(unsigned int process_index, unsigned int *parent_index)
{
    DWORD parent_id = process_list[process_index].p.th32ParentProcessID;
    unsigned int i;

    if (!parent_id)
        return FALSE;

    for (i = 0; i < process_count; ++i)
    {
        if (process_list[i].p.th32ProcessID == parent_id)
        {
            *parent_index = i;
            return TRUE;
        }
    }
    return FALSE;
}

static void mark_child_processes(void)
{
    unsigned int i, parent;

    for (i = 0; i < process_count; ++i)
    {
        if (process_list[i].matched)
            continue;
#ifdef __REACTOS__
        // Prevent self-termination if we are in the process tree
        if (process_list[i].p.th32ProcessID == self_pid)
            continue;
#endif
        parent = i;
        while (find_parent(parent, &parent))
        {
            if (process_list[parent].matched)
            {
                WINE_TRACE("Adding child %04lx.\n", process_list[i].p.th32ProcessID);
                process_list[i].matched = TRUE;
#ifdef __REACTOS__
                pkill_list[pkill_size++] = i;
#endif
                break;
            }
        }
    }
}

#endif // !__REACTOS__

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

#ifdef __REACTOS__

static BOOL get_pid_creation_time(DWORD pid, FILETIME *time)
{
    HANDLE process;
    FILETIME t1, t2, t3;
    BOOL success;

    process = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, pid);
    if (!process)
        return FALSE;

    success = GetProcessTimes(process, time, &t1, &t2, &t3);
    CloseHandle(process);

    return success;
}

static void queue_children(DWORD ppid)
{
    FILETIME parent_creation_time;
    unsigned int i;

    if (!get_pid_creation_time(ppid, &parent_creation_time))
        return;

    for (i = 0; i < process_count; ++i)
    {
        FILETIME child_creation_time;
        DWORD pid;

        // Ignore processes already marked for termination
        if (process_list[i].matched)
            continue;

        // Prevent self-termination if we are in the process tree
        pid = process_list[i].p.th32ProcessID;
        if (pid == self_pid)
            continue;

        if (!get_pid_creation_time(pid, &child_creation_time))
            continue;

        // Compare creation time to avoid PID reuse cases
        if (process_list[i].p.th32ParentProcessID == ppid &&
            CompareFileTime(&parent_creation_time, &child_creation_time) < 0)
        {
            // Process marked for termination
            WINE_TRACE("Adding child %04lx.\n", pid);
            process_list[i].matched = TRUE;
            pkill_list[pkill_size++] = i;
        }
    }
}

/*
 * Based on the root processes to terminate, we perform a level order traversal
 * (Breadth First Search) of the corresponding process trees, building the list
 * of all processes and children to terminate,
 * This allows terminating the processes, starting from parents down to their
 * children. Note that this is in the reverse order than what Windows' taskkill
 * does. The reason why we chose to do the reverse, is because there exist
 * (parent) processes that detect whether their children are terminated, and
 * if so, attempt to restart their terminated children. We want to avoid this
 * scenario in order to ensure no extra processes get started, while the user
 * wanted to terminate them.
 */
static void mark_child_processes(void)
{
    /*
     * The temporary FIFO queue for BFS (starting empty), is embedded
     * inside the result list. The queue resides between the [front, end)
     * indices (if front == end, the queue is empty), and moves down
     * through the result list, generating the sought values in order.
     */
    unsigned int front = 0; // end = 0; given by pkill_size

    /* The root processes have already been
     * enqueued in pkill_list[0..pkill_size] */

    /* Now, find and enqueue the children processes */
    while (pkill_size - front > 0)
    {
        /* Begin search at the next level */
        unsigned int len = pkill_size - front;
        unsigned int i;
        for (i = 0; i < len; ++i)
        {
            /* Standard BFS would pop the element from the front of
             * the queue and push it to the end of the result list.
             * In our case, everything is already correctly positioned
             * so that we can just simply emulate queue front popping. */
            DWORD pid = process_list[pkill_list[front++]].p.th32ProcessID;

            /* Enqueue the children processes */
            queue_children(pid); // Updates pkill_size accordingly.
        }
    }
}

#endif // __REACTOS__

static int send_close_messages(void)
{
    const WCHAR *process_name;
    struct pid_close_info info;
    unsigned int i;
    int status_code = 0;

#ifdef __REACTOS__
    DWORD index;
    for (index = 0; index < pkill_size; ++index)
#else
    for (i = 0; i < process_count; i++)
#endif
    {
#ifdef __REACTOS__
        i = pkill_list[index];
#else
        if (!process_list[i].matched)
            continue;
#endif

        info.pid = process_list[i].p.th32ProcessID;
        process_name = process_list[i].p.szExeFile;
        info.found = FALSE;
        WINE_TRACE("Terminating pid %04lx.\n", info.pid);
        EnumWindows(pid_enum_proc, (LPARAM)&info);
        if (info.found)
        {
            if (kill_child_processes)
                taskkill_message_printfW(STRING_CLOSE_CHILD, info.pid, process_list[i].p.th32ParentProcessID);
            else if (process_list[i].is_numeric)
                taskkill_message_printfW(STRING_CLOSE_PID_SEARCH, info.pid);
            else
                taskkill_message_printfW(STRING_CLOSE_PROC_SRCH, process_name, info.pid);
            continue;
        }
        taskkill_message_print_process(STRING_SEARCH_FAILED, i);
        status_code = 128;
    }

    return status_code;
}

static int terminate_processes(void)
{
    const WCHAR *process_name;
    unsigned int i;
    int status_code = 0;
    HANDLE process;
    DWORD pid;

#ifdef __REACTOS__
    DWORD index;
    for (index = 0; index < pkill_size; ++index)
#else
    for (i = 0; i < process_count; i++)
#endif
    {
#ifdef __REACTOS__
        i = pkill_list[index];
#else
        if (!process_list[i].matched)
            continue;
#endif

        pid = process_list[i].p.th32ProcessID;
        process_name = process_list[i].p.szExeFile;
        process = OpenProcess(PROCESS_TERMINATE, FALSE, pid);
        if (!process)
        {
            taskkill_message_print_process(STRING_SEARCH_FAILED, i);
            status_code = 128;
            continue;
        }
        if (!TerminateProcess(process, 1))
        {
#ifdef __REACTOS__
            if (kill_child_processes)
                taskkill_message_printfW(STRING_TERM_CHILD_FAILED, pid, process_list[i].p.th32ParentProcessID);
            else
#endif
            taskkill_message_print_process(STRING_TERMINATE_FAILED, i);
            status_code = 1;
            CloseHandle(process);
            continue;
        }
        if (kill_child_processes)
            taskkill_message_printfW(STRING_TERM_CHILD, pid, process_list[i].p.th32ParentProcessID);
        else if (process_list[i].is_numeric)
            taskkill_message_printfW(STRING_TERM_PID_SEARCH, pid);
        else
            taskkill_message_printfW(STRING_TERM_PROC_SEARCH, process_name, pid);
        CloseHandle(process);
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
                if (force_termination)
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
                if (has_help)
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
                if (kill_child_processes)
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
    else if (!has_im && !has_pid) // has_help == FALSE
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
    int search_status = 0, terminate_status;
    unsigned int i;

    if (!process_arguments(argc, argv))
        return 1;

    if (!enumerate_processes())
    {
        taskkill_message(STRING_ENUM_FAILED);
        return 1;
    }

#ifdef __REACTOS__
    pkill_list = malloc(process_count * sizeof(unsigned int*));
    if (!pkill_list)
        return 1;
    memset(pkill_list, 0, process_count * sizeof(unsigned int*));
    pkill_size = 0;

    self_pid = GetCurrentProcessId();
#endif

    for (i = 0; i < task_count; ++i)
        mark_task_process(task_list[i], &search_status);
    if (kill_child_processes)
        mark_child_processes();
    if (force_termination)
        terminate_status = terminate_processes();
    else
        terminate_status = send_close_messages();
    return search_status ? search_status : terminate_status;
}
