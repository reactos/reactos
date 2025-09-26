/*
 * Toolhelp
 *
 * Copyright 2005 Eric Pouech
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

#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "tlhelp32.h"
#include "wine/test.h"
#include "winuser.h"
#include "winternl.h"

static char     selfname[MAX_PATH];

/* Some functions are only in later versions of kernel32.dll */
static HANDLE (WINAPI *pCreateToolhelp32Snapshot)(DWORD, DWORD);
static BOOL (WINAPI *pModule32First)(HANDLE, LPMODULEENTRY32);
static BOOL (WINAPI *pModule32Next)(HANDLE, LPMODULEENTRY32);
static BOOL (WINAPI *pProcess32First)(HANDLE, LPPROCESSENTRY32);
static BOOL (WINAPI *pProcess32Next)(HANDLE, LPPROCESSENTRY32);
static BOOL (WINAPI *pThread32First)(HANDLE, LPTHREADENTRY32);
static BOOL (WINAPI *pThread32Next)(HANDLE, LPTHREADENTRY32);
static NTSTATUS (WINAPI * pNtQuerySystemInformation)(SYSTEM_INFORMATION_CLASS, void *, ULONG, ULONG *);

/* 1 minute should be more than enough */
#define WAIT_TIME       (60 * 1000)
/* Specify the number of simultaneous threads to test */
#define NUM_THREADS 4

static DWORD WINAPI sub_thread(void* pmt)
{
    DWORD w = WaitForSingleObject(pmt, WAIT_TIME);
    return w;
}

/******************************************************************
 *		init
 *
 * generates basic information like:
 *      selfname:       the way to reinvoke ourselves
 * returns:
 *      -1      on error
 *      0       if parent
 *      doesn't return if child
 */
static int     init(void)
{
    int                 argc;
    char**              argv;
    HANDLE              ev1, ev2, ev3, hThread;
    DWORD               w, tid;

    argc = winetest_get_mainargs( &argv );
    strcpy(selfname, argv[0]);

    switch (argc)
    {
    case 2: /* the test program */
        return 0;
    case 4: /* the sub-process */
        ev1 = (HANDLE)(INT_PTR)atoi(argv[2]);
        ev2 = (HANDLE)(INT_PTR)atoi(argv[3]);
        ev3 = CreateEventW(NULL, FALSE, FALSE, NULL);

        if (ev3 == NULL) ExitProcess(WAIT_ABANDONED);
        hThread = CreateThread(NULL, 0, sub_thread, ev3, 0, &tid);
        if (hThread == NULL) ExitProcess(WAIT_ABANDONED);
        if (!LoadLibraryA("shell32.dll")) ExitProcess(WAIT_ABANDONED);

        /* signal init of sub-process is done */
        SetEvent(ev1);
        /* wait for parent to have done all its queries */
        w = WaitForSingleObject(ev2, WAIT_TIME);
        if (w != WAIT_OBJECT_0) ExitProcess(w);
        /* signal sub-thread to terminate */
        SetEvent(ev3);
        w = WaitForSingleObject(hThread, WAIT_TIME);
        if (w != WAIT_OBJECT_0) ExitProcess(w);
        GetExitCodeThread(hThread, &w);
        ExitProcess(w);
    default:
        return -1;
    }
}

static void test_process(DWORD curr_pid, DWORD sub_pcs_pid)
{
    HANDLE              hSnapshot;
    PROCESSENTRY32      pe;
    MODULEENTRY32       me;
    unsigned            found = 0;
    int                 num = 0;
    int			childpos = -1;

    hSnapshot = pCreateToolhelp32Snapshot( TH32CS_SNAPPROCESS, 0 );
    ok(hSnapshot != NULL, "Cannot create snapshot\n");

    /* check that this current process is enumerated */
    pe.dwSize = sizeof(pe);
    if (pProcess32First( hSnapshot, &pe ))
    {
        do
        {
            if (pe.th32ProcessID == curr_pid) found++;
            if (pe.th32ProcessID == sub_pcs_pid) { childpos = num; found++; }
            trace("PID=%lx %s\n", pe.th32ProcessID, pe.szExeFile);
            num++;
        } while (pProcess32Next( hSnapshot, &pe ));
    }
    ok(found == 2, "couldn't find self and/or sub-process in process list\n");

    /* check that first really resets the enumeration */
    found = 0;
    if (pProcess32First( hSnapshot, &pe ))
    {
        do
        {
            if (pe.th32ProcessID == curr_pid) found++;
            if (pe.th32ProcessID == sub_pcs_pid) found++;
            trace("PID=%lx %s\n", pe.th32ProcessID, pe.szExeFile);
            num--;
        } while (pProcess32Next( hSnapshot, &pe ));
    }
    ok(found == 2, "couldn't find self and/or sub-process in process list\n");
    ok(!num, "mismatch in counting\n");

    /* one broken program does Process32First() and does not expect anything
     * interesting to be there, especially not the just forked off child */
    ok (childpos !=0, "child is not expected to be at position 0.\n");

    me.dwSize = sizeof(me);
    ok(!pModule32First( hSnapshot, &me ), "shouldn't return a module\n");

    CloseHandle(hSnapshot);
    ok(!pProcess32First( hSnapshot, &pe ), "shouldn't return a process\n");
}

static DWORD WINAPI get_id_thread(void* curr_pid)
{
    HANDLE              hSnapshot;
    THREADENTRY32       te;
    HANDLE              ev, threads[NUM_THREADS];
    DWORD               thread_ids[NUM_THREADS];
    DWORD               thread_traversed[NUM_THREADS];
    DWORD               tid, first_tid = 0;
    BOOL                found = FALSE;
    int                 i, matched_idx = -1;
    ULONG               buf_size = 0;
    NTSTATUS            status;
    BYTE*               pcs_buffer = NULL;
    DWORD               pcs_offset = 0;
    SYSTEM_PROCESS_INFORMATION* spi = NULL;

    ev = CreateEventW(NULL, TRUE, FALSE, NULL);
    ok(ev != NULL, "Cannot create event\n");

    for (i = 0; i < NUM_THREADS; i++)
    {
        threads[i] = CreateThread(NULL, 0, sub_thread, ev, 0, &tid);
        ok(threads[i] != NULL, "Cannot create thread\n");
        thread_ids[i] = tid;
    }

    hSnapshot = pCreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, 0);
    ok(hSnapshot != NULL, "Cannot create snapshot\n");

    /* Check that this current process is enumerated */
    te.dwSize = sizeof(te);
    ok(pThread32First(hSnapshot, &te), "Thread cannot traverse\n");
    do
    {
        if (found)
        {
            if (te.th32OwnerProcessID != PtrToUlong(curr_pid)) break;

            if (matched_idx >= 0)
            {
                thread_traversed[matched_idx++] = te.th32ThreadID;
                if (matched_idx >= NUM_THREADS) break;
            }
            else if (thread_ids[0] == te.th32ThreadID)
            {
                matched_idx = 0;
                thread_traversed[matched_idx++] = te.th32ThreadID;
            }
        }
        else if (te.th32OwnerProcessID == PtrToUlong(curr_pid))
        {
            found = TRUE;
            first_tid = te.th32ThreadID;
        }
    }
    while (pThread32Next(hSnapshot, &te));

    ok(found, "Couldn't find self and/or sub-process in process list\n");

    /* Check if the thread order is strictly consistent */
    found = FALSE;
    for (i = 0; i < NUM_THREADS; i++)
    {
        if (thread_traversed[i] != thread_ids[i])
        {
            found = TRUE;
            break;
        }
        /* Reset data */
        thread_traversed[i] = 0;
    }
    ok(found == FALSE, "The thread order is not strictly consistent\n");

    /* Determine the order by NtQuerySystemInformation function */

    while ((status = NtQuerySystemInformation(SystemProcessInformation,
            pcs_buffer, buf_size, &buf_size)) == STATUS_INFO_LENGTH_MISMATCH)
    {
        free(pcs_buffer);
        pcs_buffer = malloc(buf_size);
    }
    ok(status == STATUS_SUCCESS, "got %#lx\n", status);
    found = FALSE;
    matched_idx = -1;

    do
    {
        spi = (SYSTEM_PROCESS_INFORMATION*)&pcs_buffer[pcs_offset];
        if (spi->UniqueProcessId == curr_pid)
        {
            found = TRUE;
            break;
        }
        pcs_offset += spi->NextEntryOffset;
    } while (spi->NextEntryOffset != 0);

    ok(found && spi, "No process found\n");
    for (i = 0; i < spi->dwThreadCount; i++)
    {
        tid = HandleToULong(spi->ti[i].ClientId.UniqueThread);
        if (matched_idx > 0)
        {
            thread_traversed[matched_idx++] = tid;
            if (matched_idx >= NUM_THREADS) break;
        }
        else if (tid == thread_ids[0])
        {
            matched_idx = 0;
            thread_traversed[matched_idx++] = tid;
        }
    }
    free(pcs_buffer);

    ok(matched_idx > 0, "No thread id match found\n");

    found = FALSE;
    for (i = 0; i < NUM_THREADS; i++)
    {
        if (thread_traversed[i] != thread_ids[i])
        {
            found = TRUE;
            break;
        }
    }
    ok(found == FALSE, "wrong order in NtQuerySystemInformation function\n");

    SetEvent(ev);
    WaitForMultipleObjects( NUM_THREADS, threads, TRUE, WAIT_TIME );
    for (i = 0; i < NUM_THREADS; i++)
        CloseHandle(threads[i]);
    CloseHandle(ev);
    CloseHandle(hSnapshot);

    return first_tid;
}

static void test_main_thread(DWORD curr_pid, DWORD main_tid)
{
    HANDLE              thread;
    DWORD               tid = 0;
    int                 error;

    /* Check that the main thread id is first one in this thread. */
    tid = get_id_thread(ULongToPtr(curr_pid));
    ok(tid == main_tid, "The first thread id returned is not the main thread id\n");

    /* Check that the main thread id is first one in other thread. */
    thread = CreateThread(NULL, 0, get_id_thread, ULongToPtr(curr_pid), 0, NULL);
    error = WaitForSingleObject(thread, WAIT_TIME);
    ok(error == WAIT_OBJECT_0, "Thread did not complete within timelimit\n");

    ok(GetExitCodeThread(thread, &tid), "Could not retrieve exit code\n");
    ok(tid == main_tid, "The first thread id returned is not the main thread id\n");
}

static void test_thread(DWORD curr_pid, DWORD sub_pcs_pid)
{
    HANDLE              hSnapshot;
    THREADENTRY32       te;
    MODULEENTRY32       me;
    unsigned            curr_found = 0;
    unsigned            sub_found = 0;

    hSnapshot = pCreateToolhelp32Snapshot( TH32CS_SNAPTHREAD, 0 );
    ok(hSnapshot != NULL, "Cannot create snapshot\n");

    /* check that this current process is enumerated */
    te.dwSize = sizeof(te);
    if (pThread32First( hSnapshot, &te ))
    {
        do
        {
            if (te.th32OwnerProcessID == curr_pid) curr_found++;
            if (te.th32OwnerProcessID == sub_pcs_pid) sub_found++;
            if (winetest_debug > 1)
                trace("PID=%lx TID=%lx %ld\n", te.th32OwnerProcessID, te.th32ThreadID, te.tpBasePri);
        } while (pThread32Next( hSnapshot, &te ));
    }
    ok(curr_found, "couldn't find self in thread list\n");
    ok(sub_found >= 2, "couldn't find sub-process threads in thread list\n");

    /* check that first really resets enumeration */
    curr_found = 0;
    sub_found = 0;
    if (pThread32First( hSnapshot, &te ))
    {
        do
        {
            if (te.th32OwnerProcessID == curr_pid) curr_found++;
            if (te.th32OwnerProcessID == sub_pcs_pid) sub_found++;
            if (winetest_debug > 1)
                trace("PID=%lx TID=%lx %ld\n", te.th32OwnerProcessID, te.th32ThreadID, te.tpBasePri);
        } while (pThread32Next( hSnapshot, &te ));
    }
    ok(curr_found, "couldn't find self in thread list\n");
    ok(sub_found >= 2, "couldn't find sub-process threads in thread list\n");

    me.dwSize = sizeof(me);
    ok(!pModule32First( hSnapshot, &me ), "shouldn't return a module\n");

    CloseHandle(hSnapshot);
    ok(!pThread32First( hSnapshot, &te ), "shouldn't return a thread\n");
}

static const char* curr_expected_modules[] =
{
    "kernel32_test.exe",
    "kernel32.dll",
    "ntdll.dll"
};

static const char* sub_expected_modules[] =
{
    "kernel32_test.exe",
    "kernel32.dll",
    "shell32.dll",
    "ntdll.dll"
};

static void test_module(DWORD pid, const char* expected[], unsigned num_expected)
{
    HANDLE              hSnapshot;
    PROCESSENTRY32      pe;
    THREADENTRY32       te;
    MODULEENTRY32       me;
    unsigned            found[32];
    unsigned            i;
    int                 num = 0;

    ok(ARRAY_SIZE(found) >= num_expected, "Internal: bump found[] size\n");

    hSnapshot = pCreateToolhelp32Snapshot( TH32CS_SNAPMODULE, pid );
    ok(hSnapshot != NULL, "Cannot create snapshot\n");

    for (i = 0; i < num_expected; i++) found[i] = 0;
    me.dwSize = sizeof(me);
    if (pModule32First( hSnapshot, &me ))
    {
        do
        {
            trace("PID=%lx base=%p size=%lx %s %s\n",
                  me.th32ProcessID, me.modBaseAddr, me.modBaseSize, me.szExePath, me.szModule);
            ok(me.th32ProcessID == pid, "wrong returned process id\n");
            for (i = 0; i < num_expected; i++)
                if (!lstrcmpiA(expected[i], me.szModule)) found[i]++;
            num++;
        } while (pModule32Next( hSnapshot, &me ));
    }
    for (i = 0; i < num_expected; i++)
        ok(found[i] == 1, "Module %s is %s\n",
           expected[i], found[i] ? "listed more than once" : "not listed");

    /* check that first really resets the enumeration */
    for (i = 0; i < num_expected; i++) found[i] = 0;
    me.dwSize = sizeof(me);
    if (pModule32First( hSnapshot, &me ))
    {
        do
        {
            trace("PID=%lx base=%p size=%lx %s %s\n",
                  me.th32ProcessID, me.modBaseAddr, me.modBaseSize, me.szExePath, me.szModule);
            for (i = 0; i < num_expected; i++)
                if (!lstrcmpiA(expected[i], me.szModule)) found[i]++;
            num--;
        } while (pModule32Next( hSnapshot, &me ));
    }
    for (i = 0; i < num_expected; i++)
        ok(found[i] == 1, "Module %s is %s\n",
           expected[i], found[i] ? "listed more than once" : "not listed");
    ok(!num, "mismatch in counting\n");

    pe.dwSize = sizeof(pe);
    ok(!pProcess32First( hSnapshot, &pe ), "shouldn't return a process\n");

    te.dwSize = sizeof(te);
    ok(!pThread32First( hSnapshot, &te ), "shouldn't return a thread\n");

    CloseHandle(hSnapshot);
    ok(!pModule32First( hSnapshot, &me ), "shouldn't return a module\n");
}

START_TEST(toolhelp)
{
    DWORD               pid = GetCurrentProcessId();
    int                 r;
    char                *p, module[MAX_PATH];
    char                buffer[MAX_PATH + 21];
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    HANDLE              ev1, ev2;
    DWORD               w;
    HANDLE              hkernel32 = GetModuleHandleA("kernel32");
    HANDLE              hntdll = GetModuleHandleA("ntdll.dll");

    pCreateToolhelp32Snapshot = (VOID *) GetProcAddress(hkernel32, "CreateToolhelp32Snapshot");
    pModule32First = (VOID *) GetProcAddress(hkernel32, "Module32First");
    pModule32Next = (VOID *) GetProcAddress(hkernel32, "Module32Next");
    pProcess32First = (VOID *) GetProcAddress(hkernel32, "Process32First");
    pProcess32Next = (VOID *) GetProcAddress(hkernel32, "Process32Next");
    pThread32First = (VOID *) GetProcAddress(hkernel32, "Thread32First");
    pThread32Next = (VOID *) GetProcAddress(hkernel32, "Thread32Next");
    pNtQuerySystemInformation = (VOID *) GetProcAddress(hntdll, "NtQuerySystemInformation");

    if (!pCreateToolhelp32Snapshot || 
        !pModule32First || !pModule32Next ||
        !pProcess32First || !pProcess32Next ||
        !pThread32First || !pThread32Next ||
        !pNtQuerySystemInformation)
    {
        win_skip("Needed functions are not available, most likely running on Windows NT\n");
        return;
    }

    r = init();
    ok(r == 0, "Basic init of sub-process test\n");
    if (r != 0) return;

    sa.nLength = sizeof(sa);
    sa.lpSecurityDescriptor = NULL;
    sa.bInheritHandle = TRUE;

    ev1 = CreateEventW(&sa, FALSE, FALSE, NULL);
    ev2 = CreateEventW(&sa, FALSE, FALSE, NULL);
    ok (ev1 != NULL && ev2 != NULL, "Couldn't create events\n");
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    sprintf(buffer, "%s toolhelp %Iu %Iu", selfname, (DWORD_PTR)ev1, (DWORD_PTR)ev2);
    ok(CreateProcessA(NULL, buffer, NULL, NULL, TRUE, 0, NULL, NULL, &startup, &info), "CreateProcess\n");
    /* wait for child to be initialized */
    w = WaitForSingleObject(ev1, WAIT_TIME);
    ok(w == WAIT_OBJECT_0, "Failed to wait on sub-process startup\n");

    GetModuleFileNameA( 0, module, sizeof(module) );
    if (!(p = strrchr( module, '\\' ))) p = module;
    else p++;
    curr_expected_modules[0] = p;
    sub_expected_modules[0] = p;

    test_process(pid, info.dwProcessId);
    test_thread(pid, info.dwProcessId);
    test_main_thread(pid, GetCurrentThreadId());
    test_module(pid, curr_expected_modules, ARRAY_SIZE(curr_expected_modules));
    test_module(info.dwProcessId, sub_expected_modules, ARRAY_SIZE(sub_expected_modules));

    SetEvent(ev2);
    wait_child_process( info.hProcess );
}
