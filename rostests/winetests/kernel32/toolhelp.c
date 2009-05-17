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

#include "windef.h"
#include "winbase.h"
#include "tlhelp32.h"
#include "wine/test.h"
#include "winuser.h"

static char     selfname[MAX_PATH];

/* Some functions are only in later versions of kernel32.dll */
static HANDLE (WINAPI *pCreateToolhelp32Snapshot)(DWORD, DWORD);
static BOOL (WINAPI *pModule32First)(HANDLE, LPMODULEENTRY32);
static BOOL (WINAPI *pModule32Next)(HANDLE, LPMODULEENTRY32);
static BOOL (WINAPI *pProcess32First)(HANDLE, LPPROCESSENTRY32);
static BOOL (WINAPI *pProcess32Next)(HANDLE, LPPROCESSENTRY32);
static BOOL (WINAPI *pThread32First)(HANDLE, LPTHREADENTRY32);
static BOOL (WINAPI *pThread32Next)(HANDLE, LPTHREADENTRY32);

/* 1 minute should be more than enough */
#define WAIT_TIME       (60 * 1000)

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
        ev3 = CreateEvent(NULL, FALSE, FALSE, NULL);

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
            trace("PID=%x %s\n", pe.th32ProcessID, pe.szExeFile);
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
            trace("PID=%x %s\n", pe.th32ProcessID, pe.szExeFile);
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

static void test_thread(DWORD curr_pid, DWORD sub_pcs_pid)
{
    HANDLE              hSnapshot;
    THREADENTRY32       te;
    MODULEENTRY32       me;
    int                 num = 0;
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
                trace("PID=%x TID=%x %d\n", te.th32OwnerProcessID, te.th32ThreadID, te.tpBasePri);
            num++;
        } while (pThread32Next( hSnapshot, &te ));
    }
    ok(curr_found == 1, "couldn't find self in thread list\n");
    ok(sub_found == 2, "couldn't find sub-process thread's in thread list\n");

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
                trace("PID=%x TID=%x %d\n", te.th32OwnerProcessID, te.th32ThreadID, te.tpBasePri);
            num--;
        } while (pThread32Next( hSnapshot, &te ));
    }
    ok(curr_found == 1, "couldn't find self in thread list\n");
    ok(sub_found == 2, "couldn't find sub-process thread's in thread list\n");

    me.dwSize = sizeof(me);
    ok(!pModule32First( hSnapshot, &me ), "shouldn't return a module\n");

    CloseHandle(hSnapshot);
    ok(!pThread32First( hSnapshot, &te ), "shouldn't return a thread\n");
}

static const char* curr_expected_modules[] =
{
    "kernel32_test.exe"
    "kernel32.dll",
    /* FIXME: could test for ntdll on NT and Wine */
};
static const char* sub_expected_modules[] =
{
    "kernel32_test.exe",
    "kernel32.dll",
    "shell32.dll"
    /* FIXME: could test for ntdll on NT and Wine */
};
#define NUM_OF(x) (sizeof(x) / sizeof(x[0]))

static void test_module(DWORD pid, const char* expected[], unsigned num_expected)
{
    HANDLE              hSnapshot;
    PROCESSENTRY32      pe;
    THREADENTRY32       te;
    MODULEENTRY32       me;
    unsigned            found[32];
    unsigned            i;
    int                 num = 0;

    ok(NUM_OF(found) >= num_expected, "Internal: bump found[] size\n");

    hSnapshot = pCreateToolhelp32Snapshot( TH32CS_SNAPMODULE, pid );
    ok(hSnapshot != NULL, "Cannot create snapshot\n");

    for (i = 0; i < num_expected; i++) found[i] = 0;
    me.dwSize = sizeof(me);
    if (pModule32First( hSnapshot, &me ))
    {
        do
        {
            trace("PID=%x base=%p size=%x %s %s\n",
                  me.th32ProcessID, me.modBaseAddr, me.modBaseSize, me.szExePath, me.szModule);
            ok(me.th32ProcessID == pid, "wrong returned process id\n");
            for (i = 0; i < num_expected; i++)
                if (!lstrcmpi(expected[i], me.szModule)) found[i]++;
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
            trace("PID=%x base=%p size=%x %s %s\n",
                  me.th32ProcessID, me.modBaseAddr, me.modBaseSize, me.szExePath, me.szModule);
            for (i = 0; i < num_expected; i++)
                if (!lstrcmpi(expected[i], me.szModule)) found[i]++;
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
    char                buffer[MAX_PATH];
    SECURITY_ATTRIBUTES sa;
    PROCESS_INFORMATION	info;
    STARTUPINFOA	startup;
    HANDLE              ev1, ev2;
    DWORD               w;
    HANDLE              hkernel32 = GetModuleHandleA("kernel32");

    pCreateToolhelp32Snapshot = (VOID *) GetProcAddress(hkernel32, "CreateToolhelp32Snapshot");
    pModule32First = (VOID *) GetProcAddress(hkernel32, "Module32First");
    pModule32Next = (VOID *) GetProcAddress(hkernel32, "Module32Next");
    pProcess32First = (VOID *) GetProcAddress(hkernel32, "Process32First");
    pProcess32Next = (VOID *) GetProcAddress(hkernel32, "Process32Next");
    pThread32First = (VOID *) GetProcAddress(hkernel32, "Thread32First");
    pThread32Next = (VOID *) GetProcAddress(hkernel32, "Thread32Next");

    if (!pCreateToolhelp32Snapshot || 
        !pModule32First || !pModule32Next ||
        !pProcess32First || !pProcess32Next ||
        !pThread32First || !pThread32Next)
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

    ev1 = CreateEvent(&sa, FALSE, FALSE, NULL);
    ev2 = CreateEvent(&sa, FALSE, FALSE, NULL);
    ok (ev1 != NULL && ev2 != NULL, "Couldn't create events\n");
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;

    sprintf(buffer, "%s tests/toolhelp.c %lu %lu", selfname, (DWORD_PTR)ev1, (DWORD_PTR)ev2);
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
    test_module(pid, curr_expected_modules, NUM_OF(curr_expected_modules));
    test_module(info.dwProcessId, sub_expected_modules, NUM_OF(sub_expected_modules));

    SetEvent(ev2);
    winetest_wait_child_process( info.hProcess );
}
