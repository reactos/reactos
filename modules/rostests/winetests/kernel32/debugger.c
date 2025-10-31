/*
 * Unit tests for the debugger facility
 *
 * Copyright (c) 2007 Francois Gouget for CodeWeavers
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

#include <stdio.h>
#include <assert.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winternl.h>
#include <winreg.h>
#include "wine/test.h"
#include "wine/heap.h"
#include "wine/rbtree.h"

#ifndef STATUS_DEBUGGER_INACTIVE
#define STATUS_DEBUGGER_INACTIVE         ((NTSTATUS) 0xC0000354)
#endif

#define child_ok (winetest_set_location(__FILE__, __LINE__), 0) ? (void)0 : test_child_ok

static int    myARGC;
static char** myARGV;
static BOOL is_wow64;

static BOOL (WINAPI *pCheckRemoteDebuggerPresent)(HANDLE,PBOOL);

static void (WINAPI *pDbgBreakPoint)(void);

static NTSTATUS  (WINAPI *pNtSuspendProcess)(HANDLE process);
static NTSTATUS  (WINAPI *pNtResumeProcess)(HANDLE process);
static NTSTATUS  (WINAPI *pNtCreateDebugObject)(HANDLE *, ACCESS_MASK, OBJECT_ATTRIBUTES *, ULONG);
static NTSTATUS  (WINAPI *pNtSetInformationDebugObject)(HANDLE,DEBUGOBJECTINFOCLASS,void *,ULONG,ULONG*);
static NTSTATUS  (WINAPI *pDbgUiConnectToDbg)(void);
static HANDLE    (WINAPI *pDbgUiGetThreadDebugObject)(void);
static void      (WINAPI *pDbgUiSetThreadDebugObject)(HANDLE);
static DWORD     (WINAPI *pGetMappedFileNameW)(HANDLE,void*,WCHAR*,DWORD);
static BOOL      (WINAPI *pIsWow64Process)(HANDLE,PBOOL);

static LONG child_failures;

static HMODULE ntdll;

static void WINAPIV __WINE_PRINTF_ATTR(2, 3) test_child_ok(int condition, const char *msg, ...)
{
    va_list valist;

    va_start(valist, msg);
    winetest_vok(condition, msg, valist);
    va_end(valist);
    if (!condition) ++child_failures;
}

/* Copied from the process test */
static void get_file_name(char* buf)
{
    char path[MAX_PATH];

    buf[0] = '\0';
    GetTempPathA(sizeof(path), path);
    GetTempFileNameA(path, "wt", 0, buf);
}

typedef struct tag_reg_save_value
{
    const char *name;
    DWORD type;
    BYTE *data;
    DWORD size;
} reg_save_value;

static DWORD save_value(HKEY hkey, const char *value, reg_save_value *saved)
{
    DWORD ret;
    saved->name=value;
    saved->data=0;
    saved->size=0;
    ret=RegQueryValueExA(hkey, value, NULL, &saved->type, NULL, &saved->size);
    if (ret == ERROR_SUCCESS)
    {
        saved->data=HeapAlloc(GetProcessHeap(), 0, saved->size);
        RegQueryValueExA(hkey, value, NULL, &saved->type, saved->data, &saved->size);
    }
    return ret;
}

static void restore_value(HKEY hkey, reg_save_value *saved)
{
    if (saved->data)
    {
        RegSetValueExA(hkey, saved->name, 0, saved->type, saved->data, saved->size);
        HeapFree(GetProcessHeap(), 0, saved->data);
    }
    else
        RegDeleteValueA(hkey, saved->name);
}

static void get_events(const char* name, HANDLE *start_event, HANDLE *done_event)
{
    const char* basename;
    char* event_name;

    basename=strrchr(name, '\\');
    basename=(basename ? basename+1 : name);
    event_name=HeapAlloc(GetProcessHeap(), 0, 6+strlen(basename)+1);

    sprintf(event_name, "start_%s", basename);
    *start_event=CreateEventA(NULL, 0,0, event_name);
    sprintf(event_name, "done_%s", basename);
    *done_event=CreateEventA(NULL, 0,0, event_name);
    HeapFree(GetProcessHeap(), 0, event_name);
}

static void save_blackbox(const char* logfile, void* blackbox, int size, const char *dbgtrace)
{
    HANDLE hFile;
    DWORD written;
    BOOL ret;

    hFile = CreateFileA(logfile, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "Couldn't create %s: %lu\n", logfile, GetLastError());
    if (hFile == INVALID_HANDLE_VALUE)
        return;
    ret = WriteFile(hFile, blackbox, size, &written, NULL);
    ok(ret && written == size, "Error writing\n");
    if (dbgtrace && dbgtrace[0])
    {
        ret = WriteFile(hFile, dbgtrace, strlen(dbgtrace), &written, NULL);
        ok(ret && written == strlen(dbgtrace), "Error writing\n");
    }
    CloseHandle(hFile);
}

#define load_blackbox(a, b, c) _load_blackbox(__LINE__, (a), (b), (c))
static int _load_blackbox(unsigned int line, const char* logfile, void* blackbox, int size)
{
    HANDLE hFile;
    DWORD read;
    BOOL ret;
    char buf[4096];

    hFile = CreateFileA(logfile, GENERIC_READ, FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ok_(__FILE__, line)(0, "unable to open '%s': %#lx\n", logfile, GetLastError());
        return 0;
    }
    SetLastError(0xdeadbeef);
    ret = ReadFile(hFile, blackbox, size, &read, NULL);
    ok(ret, "ReadFile failed: %ld\n", GetLastError());
    ok(read == size, "wrong size for '%s': read=%ld\n", logfile, read);
    ret = ReadFile(hFile, buf, sizeof(buf) - 1, &read, NULL);
    if (ret && read)
    {
        buf[read] = 0;
        trace("debugger traces:>>>\n%s\n<<< Done.\n", buf);
    }
    CloseHandle(hFile);
    return 1;
}

static DWORD WINAPI thread_proc(void *arg)
{
    Sleep(10000);
    trace("exiting\n");
    ExitThread(1);
}

static void run_background_thread(void)
{
    DWORD tid;
    HANDLE thread = CreateThread(NULL, 0, thread_proc, NULL, 0, &tid);
    ok(thread != NULL, "CreateThread failed\n");
    CloseHandle(thread);
}

static void doCrash(void)
{
    volatile char* p;

    /* make sure the exception gets to the debugger */
    SetErrorMode( 0 );
    SetUnhandledExceptionFilter( NULL );

    run_background_thread();

    /* Just crash */
    trace("child: crashing...\n");
    p=NULL;
    *p=0;
}

typedef struct
{
    int argc;
    DWORD pid;
    BOOL debug_rc;
    DWORD debug_err;
    BOOL attach_rc;
    DWORD attach_err;
    BOOL nokill_rc;
    DWORD nokill_err;
    BOOL detach_rc;
    DWORD detach_err;
    DWORD failures;
} debugger_blackbox_t;

struct debugger_context
{
    DWORD pid;
    DEBUG_EVENT ev;
    unsigned process_cnt;
    unsigned dll_cnt;
    void *image_base;
    DWORD thread_tag;
    unsigned thread_cnt;
    struct wine_rb_tree threads;
    struct debuggee_thread *current_thread;
    struct debuggee_thread *main_thread;
};

struct debuggee_thread
{
    DWORD tid;
    DWORD tag;
    HANDLE handle;
    CONTEXT ctx;
    struct wine_rb_entry entry;
};

int debuggee_thread_compare(const void *key, const struct wine_rb_entry *entry)
{
    struct debuggee_thread *thread = WINE_RB_ENTRY_VALUE(entry, struct debuggee_thread, entry);
    return memcmp(key, &thread->tid, sizeof(thread->tid));
}

static void add_thread(struct debugger_context *ctx, DWORD tid)
{
    struct debuggee_thread *thread;
    if (!ctx->thread_cnt++) wine_rb_init(&ctx->threads, debuggee_thread_compare);
    thread = heap_alloc(sizeof(*thread));
    thread->tid = tid;
    thread->tag = ctx->thread_tag;
    thread->handle = NULL;
    wine_rb_put(&ctx->threads, &tid, &thread->entry);
    if (!ctx->main_thread) ctx->main_thread = thread;
}

static struct debuggee_thread *get_debuggee_thread(struct debugger_context *ctx, DWORD tid)
{
    struct wine_rb_entry *entry = wine_rb_get(&ctx->threads, &tid);
    ok(entry != NULL, "unknown thread %lx\n", tid);
    return WINE_RB_ENTRY_VALUE(entry, struct debuggee_thread, entry);
}

static void remove_thread(struct debugger_context *ctx, DWORD tid)
{
    struct debuggee_thread *thread = get_debuggee_thread(ctx, tid);

    wine_rb_remove(&ctx->threads, &thread->entry);
    if (thread->handle) CloseHandle(thread->handle);
    heap_free(thread);
}

static void *get_ip(const CONTEXT *ctx)
{
#ifdef __i386__
    return (void *)ctx->Eip;
#elif defined(__x86_64__)
    return (void *)ctx->Rip;
#else
    return NULL;
#endif
}

static void set_ip(CONTEXT *ctx, void *ip)
{
#ifdef __i386__
    ctx->Eip = (DWORD_PTR)ip;
#elif defined(__x86_64__)
    ctx->Rip = (DWORD_PTR)ip;
#endif
}

#define fetch_thread_context(a) fetch_thread_context_(__LINE__,a)
static void fetch_thread_context_(unsigned line, struct debuggee_thread *thread)
{
    BOOL ret;

    if (!thread->handle)
    {
        thread->handle = OpenThread(THREAD_GET_CONTEXT | THREAD_SET_CONTEXT | THREAD_QUERY_INFORMATION,
                                    FALSE, thread->tid);
        ok_(__FILE__,line)(thread->handle != NULL, "OpenThread failed: %lu\n", GetLastError());
    }

    memset(&thread->ctx, 0xaa, sizeof(thread->ctx));
    thread->ctx.ContextFlags = CONTEXT_FULL;
    ret = GetThreadContext(thread->handle, &thread->ctx);
    ok_(__FILE__,line)(ret, "GetThreadContext failed: %lu\n", GetLastError());
}

#define set_thread_context(a,b) set_thread_context_(__LINE__,a,b)
static void set_thread_context_(unsigned line, struct debugger_context *ctx, struct debuggee_thread *thread)
{
    BOOL ret;
    ret = SetThreadContext(thread->handle, &thread->ctx);
    ok_(__FILE__,line)(ret, "SetThreadContext failed: %lu\n", GetLastError());
}

#define WAIT_EVENT_TIMEOUT 20000
#define POLL_EVENT_TIMEOUT 200

#define next_event(a,b) next_event_(__LINE__,a,b)
static void next_event_(unsigned line, struct debugger_context *ctx, unsigned timeout)
{
    BOOL ret;

    ctx->current_thread = NULL;

    for (;;)
    {
        if (ctx->process_cnt && ctx->ev.dwDebugEventCode != -1)
        {
            ret = ContinueDebugEvent(ctx->ev.dwProcessId, ctx->ev.dwThreadId, DBG_CONTINUE);
            ok_(__FILE__,line)(ret, "ContinueDebugEvent failed, last error %ld.\n", GetLastError());
        }

        ret = WaitForDebugEvent(&ctx->ev, timeout);
        if (!ret)
        {
            ok_(__FILE__,line)(GetLastError() == ERROR_SEM_TIMEOUT,
                               "WaitForDebugEvent failed, last error %ld.\n", GetLastError());
            ctx->ev.dwDebugEventCode = -1;
            return;
        }

        if (ctx->ev.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT)
        {
            if (!ctx->process_cnt) ctx->pid = ctx->ev.dwProcessId;
            ctx->process_cnt++;
        }

        if (ctx->ev.dwDebugEventCode == OUTPUT_DEBUG_STRING_EVENT) continue; /* ignore for now */
        if (ctx->ev.dwProcessId == ctx->pid) break;

        ok_(__FILE__,line)(ctx->process_cnt > 1, "unexpected event pid\n");
    }

    switch (ctx->ev.dwDebugEventCode)
    {
    case CREATE_PROCESS_DEBUG_EVENT:
        add_thread(ctx, ctx->ev.dwThreadId);
        ctx->image_base = ctx->ev.u.CreateProcessInfo.lpBaseOfImage;
        break;
    case EXIT_PROCESS_DEBUG_EVENT:
        remove_thread(ctx, ctx->ev.dwThreadId);
        return;
    case CREATE_THREAD_DEBUG_EVENT:
        add_thread(ctx, ctx->ev.dwThreadId);
        break;
    case EXIT_THREAD_DEBUG_EVENT:
        remove_thread(ctx, ctx->ev.dwThreadId);
        return;
    case LOAD_DLL_DEBUG_EVENT:
        ok(ctx->ev.u.LoadDll.lpBaseOfDll != ctx->image_base, "process image reported as DLL load event\n");
        ctx->dll_cnt++;
        break;
    case UNLOAD_DLL_DEBUG_EVENT:
        ctx->dll_cnt--;
        break;
    }

    ctx->current_thread = get_debuggee_thread(ctx, ctx->ev.dwThreadId);
}

static DWORD event_mask(DWORD ev)
{
    return (ev >= 1 && ev <= 7) ? (1LU << ev) : 0;
}

#define next_event_filter(a, b,c) next_event_filter_(__LINE__, (a), (b), (c))
static void next_event_filter_(unsigned line, struct debugger_context *ctx, DWORD timeout, DWORD mask)
{
    do
    {
        next_event_(line, ctx, timeout);
    } while (event_mask(ctx->ev.dwDebugEventCode) & mask);
}

#define wait_for_breakpoint(a) wait_for_breakpoint_(__LINE__,a)
static void wait_for_breakpoint_(unsigned line, struct debugger_context *ctx)
{
    next_event_filter_(line, ctx, WAIT_EVENT_TIMEOUT,
                       event_mask(LOAD_DLL_DEBUG_EVENT) | event_mask(UNLOAD_DLL_DEBUG_EVENT) | event_mask(CREATE_THREAD_DEBUG_EVENT));

    ok_(__FILE__,line)(ctx->ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx->ev.dwDebugEventCode);
    ok_(__FILE__,line)(ctx->ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT, "ExceptionCode = %lx\n",
                       ctx->ev.u.Exception.ExceptionRecord.ExceptionCode);
}

#define check_thread_running(h)   ok(_check_thread_suspend_count(h) == 0, "Expecting running thread\n")
#define check_thread_suspended(h) ok(_check_thread_suspend_count(h) >  0, "Expecting suspended thread\n")

static LONG _check_thread_suspend_count(HANDLE h)
{
    DWORD suspend_count;

    suspend_count = SuspendThread(h);
    if (suspend_count != (DWORD)-1 && ResumeThread(h) == (DWORD)-1)
        return (DWORD)-2;
    return suspend_count;
}

static void process_attach_events(struct debugger_context *ctx, BOOL pass_exception)
{
    DEBUG_EVENT ev;
    BOOL ret;
    HANDLE prev_thread;

    ctx->ev.dwDebugEventCode = -1;
    next_event(ctx, 0);
    ok(ctx->ev.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx->ev.dwDebugEventCode);

    todo_wine
    check_thread_suspended(ctx->ev.u.CreateProcessInfo.hThread);
    prev_thread = ctx->ev.u.CreateProcessInfo.hThread;
    next_event(ctx, 0);

    if (ctx->ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT) /* Vista+ reports ntdll.dll before reporting threads */
    {
        ok(ctx->ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx->ev.dwDebugEventCode);
        ok(ctx->ev.u.LoadDll.lpBaseOfDll == ntdll, "The first reported DLL is not ntdll.dll\n");
        next_event(ctx, 0);
    }

    while (ctx->ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT)
    {
        todo_wine
        check_thread_suspended(ctx->ev.u.CreateThread.hThread);
        check_thread_running(prev_thread);
        prev_thread = ctx->ev.u.CreateThread.hThread;
        next_event(ctx, 0);
    }

    do
    {
        /* even when there are more pending events, they are not reported until current event is continued */
        ret = WaitForDebugEvent(&ev, 10);
        ok(GetLastError() == ERROR_SEM_TIMEOUT, "WaitForDebugEvent returned %x(%lu)\n", ret, GetLastError());

        next_event(ctx, WAIT_EVENT_TIMEOUT);
        if (ctx->ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT)
            ok(ctx->ev.u.LoadDll.lpBaseOfDll != ntdll, "ntdll.dll reported out of order\n");
    } while (ctx->ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT || ctx->ev.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT);
    ok(ctx->dll_cnt > 2, "dll_cnt = %d\n", ctx->dll_cnt);

    /* a new thread is created and it executes DbgBreakPoint, which causes the exception */
    /* Win11 doesn't generate it at this point (Win <= 10 do) */
    if (ctx->ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT)
    {
        DWORD last_threads[5];
        unsigned thd_idx = 0, i;

        check_thread_running(prev_thread);

        /* sometimes (at least Win10) several thread creations are reported here */
        do
        {
            check_thread_running(ctx->ev.u.CreateThread.hThread);
            if (thd_idx < ARRAY_SIZE(last_threads))
                last_threads[thd_idx++] = ctx->ev.dwThreadId;
            next_event(ctx, WAIT_EVENT_TIMEOUT);
        } while (ctx->ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT);
        ok(thd_idx <= ARRAY_SIZE(last_threads), "too many threads created\n");
        for (i = 0; i < thd_idx; i++)
            if (last_threads[i] == ctx->ev.dwThreadId) break;
        ok(i < thd_idx, "unexpected thread\n");

        ok(ctx->ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx->ev.dwDebugEventCode);
        ok(ctx->ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT, "ExceptionCode = %lx\n",
           ctx->ev.u.Exception.ExceptionRecord.ExceptionCode);
        ok(ctx->ev.u.Exception.ExceptionRecord.ExceptionAddress == pDbgBreakPoint, "ExceptionAddress != DbgBreakPoint\n");

        if (pass_exception)
        {
            ret = ContinueDebugEvent(ctx->ev.dwProcessId, ctx->ev.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
            ok(ret, "ContinueDebugEvent failed, last error %ld.\n", GetLastError());
            ctx->ev.dwDebugEventCode = -1;
        }
    }

    /* flush debug events */
    do next_event(ctx, POLL_EVENT_TIMEOUT);
    while (ctx->ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT || ctx->ev.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT
           || ctx->ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT || ctx->ev.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT);
    ok(ctx->ev.dwDebugEventCode == -1, "dwDebugEventCode = %ld\n", ctx->ev.dwDebugEventCode);
}

static void doDebugger(int argc, char** argv)
{
    const char* logfile;
    debugger_blackbox_t blackbox;
    HANDLE start_event = 0, done_event = 0, debug_event;
    char buf[4096] = "";
    struct debugger_context ctx = { 0 };

    blackbox.argc=argc;
    logfile=(argc >= 4 ? argv[3] : NULL);
    blackbox.pid=(argc >= 5 ? atol(argv[4]) : 0);

    blackbox.attach_err=0;
    if (strstr(myARGV[2], "attach"))
    {
        blackbox.attach_rc=DebugActiveProcess(blackbox.pid);
        if (!blackbox.attach_rc)
            blackbox.attach_err=GetLastError();
    }
    else
        blackbox.attach_rc=TRUE;

    if (strstr(myARGV[2], "process"))
    {
        strcat(buf, "processing debug messages\n");
        process_attach_events(&ctx, FALSE);
    }

    debug_event=(argc >= 6 ? (HANDLE)(INT_PTR)atol(argv[5]) : NULL);
    blackbox.debug_err=0;
    if (debug_event && strstr(myARGV[2], "event"))
    {
        strcat(buf, "setting event\n");
        blackbox.debug_rc=SetEvent(debug_event);
        if (!blackbox.debug_rc)
            blackbox.debug_err=GetLastError();
    }
    else
        blackbox.debug_rc=TRUE;

    if (strstr(myARGV[2], "process"))
    {
        next_event(&ctx, WAIT_EVENT_TIMEOUT);
        ok(ctx.ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
        ok(ctx.ev.u.Exception.ExceptionRecord.ExceptionCode == STATUS_ACCESS_VIOLATION, "ExceptionCode = %lx\n",
           ctx.ev.u.Exception.ExceptionRecord.ExceptionCode);
    }

    if (logfile)
    {
        get_events(logfile, &start_event, &done_event);
    }

    if (strstr(myARGV[2], "order"))
    {
        strcat(buf, "waiting for the start signal...\n");
        WaitForSingleObject(start_event, INFINITE);
    }

    blackbox.nokill_err=0;
    if (strstr(myARGV[2], "nokill"))
    {
        blackbox.nokill_rc = DebugSetProcessKillOnExit(FALSE);
        if (!blackbox.nokill_rc)
            blackbox.nokill_err=GetLastError();
    }
    else
        blackbox.nokill_rc=TRUE;

    blackbox.detach_err=0;
    if (strstr(myARGV[2], "detach"))
    {
        blackbox.detach_rc = DebugActiveProcessStop(blackbox.pid);
        if (!blackbox.detach_rc)
            blackbox.detach_err=GetLastError();
    }
    else
        blackbox.detach_rc=TRUE;

    if (debug_event && strstr(myARGV[2], "late"))
    {
        strcat(buf, "setting event\n");
        blackbox.debug_rc=SetEvent(debug_event);
        if (!blackbox.debug_rc)
            blackbox.debug_err=GetLastError();
    }

    strcat(buf, "done debugging...\n");
    if (logfile)
    {
        blackbox.failures = winetest_get_failures();
        save_blackbox(logfile, &blackbox, sizeof(blackbox), buf);
    }

    SetEvent(done_event);

    /* Just exit with a known value */
    ExitProcess(0xdeadbeef);
}

static void crash_and_debug(HKEY hkey, const char* argv0, const char* dbgtasks)
{
    static BOOL skip_crash_and_debug = FALSE;
    BOOL bRet;
    DWORD ret;
    HANDLE start_event, done_event;
    char* cmd;
    char dbglog[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA startup;
    DWORD exit_code;
    debugger_blackbox_t dbg_blackbox;
    DWORD wait_code;

    if (skip_crash_and_debug)
    {
        win_skip("Skipping crash_and_debug\n");
        return;
    }

    ret=RegSetValueExA(hkey, "auto", 0, REG_SZ, (BYTE*)"1", 2);
    if (ret == ERROR_ACCESS_DENIED)
    {
        skip_crash_and_debug = TRUE;
        skip("No write access to change the debugger\n");
        return;
    }

    ok(ret == ERROR_SUCCESS, "unable to set AeDebug/auto: ret=%ld\n", ret);

    get_file_name(dbglog);
    get_events(dbglog, &start_event, &done_event);
    cmd=HeapAlloc(GetProcessHeap(), 0, strlen(argv0)+10+strlen(dbgtasks)+1+strlen(dbglog)+2+34+1);
    sprintf(cmd, "%s debugger %s \"%s\" %%ld %%ld", argv0, dbgtasks, dbglog);
    ret=RegSetValueExA(hkey, "debugger", 0, REG_SZ, (BYTE*)cmd, strlen(cmd)+1);
    ok(ret == ERROR_SUCCESS, "unable to set AeDebug/debugger: ret=%ld\n", ret);
    HeapFree(GetProcessHeap(), 0, cmd);

    cmd = HeapAlloc(GetProcessHeap(), 0, strlen(argv0) + 16);
    sprintf(cmd, "%s debugger crash", argv0);

    trace("running %s...\n", dbgtasks);
    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    ret=CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess: err=%ld\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, cmd);
    CloseHandle(info.hThread);

    /* The process exits... */
    trace("waiting for child exit...\n");
    wait_code = WaitForSingleObject(info.hProcess, 30000);
#if defined(_WIN64) && defined(__MINGW32__)
    /* Mingw x64 doesn't output proper unwind info */
    skip_crash_and_debug = broken(wait_code == WAIT_TIMEOUT);
    if (skip_crash_and_debug)
    {
        TerminateProcess(info.hProcess, WAIT_TIMEOUT);
        WaitForSingleObject(info.hProcess, 5000);
        CloseHandle(info.hProcess);
        DeleteFileA(dbglog);
        win_skip("Giving up on child process\n");
        return;
    }
#endif
    ok(wait_code == WAIT_OBJECT_0, "Timed out waiting for the child to crash\n");
    bRet = GetExitCodeProcess(info.hProcess, &exit_code);
    ok(bRet, "GetExitCodeProcess failed: err=%ld\n", GetLastError());
    if (strstr(dbgtasks, "code2"))
    {
        /* If, after attaching to the debuggee, the debugger exits without
         * detaching, then the debuggee gets a special exit code.
         */
        ok(exit_code == STATUS_DEBUGGER_INACTIVE ||
           broken(exit_code == STATUS_ACCESS_VIOLATION) || /* Intermittent Vista+ */
           broken(exit_code == WAIT_ABANDONED), /* NT4, W2K */
           "wrong exit code : %08lx\n", exit_code);
    }
    else
        ok(exit_code == STATUS_ACCESS_VIOLATION ||
           broken(exit_code == WAIT_ABANDONED), /* NT4, W2K, W2K3 */
           "wrong exit code : %08lx\n", exit_code);
    CloseHandle(info.hProcess);

    /* ...before the debugger */
    if (strstr(dbgtasks, "order"))
        ok(SetEvent(start_event), "SetEvent(start_event) failed\n");

    trace("waiting for the debugger...\n");
    wait_code = WaitForSingleObject(done_event, 5000);
#if defined(_WIN64) && defined(__MINGW32__)
    /* Mingw x64 doesn't output proper unwind info */
    skip_crash_and_debug = broken(wait_code == WAIT_TIMEOUT);
    if (skip_crash_and_debug)
    {
        DeleteFileA(dbglog);
        win_skip("Giving up on debugger\n");
        return;
    }
#endif
    ok(wait_code == WAIT_OBJECT_0, "Timed out waiting for the debugger\n");

    ok(load_blackbox(dbglog, &dbg_blackbox, sizeof(dbg_blackbox)), "failed to open: %s\n", dbglog);

    ok(dbg_blackbox.argc == 6, "wrong debugger argument count: %d\n", dbg_blackbox.argc);
    ok(dbg_blackbox.pid == info.dwProcessId, "the child and debugged pids don't match: %ld != %ld\n", info.dwProcessId, dbg_blackbox.pid);
    ok(dbg_blackbox.debug_rc, "debugger: SetEvent(debug_event) failed err=%ld\n", dbg_blackbox.debug_err);
    ok(dbg_blackbox.attach_rc, "DebugActiveProcess(%ld) failed err=%ld\n", dbg_blackbox.pid, dbg_blackbox.attach_err);
    ok(dbg_blackbox.nokill_rc, "DebugSetProcessKillOnExit(FALSE) failed err=%ld\n", dbg_blackbox.nokill_err);
    ok(dbg_blackbox.detach_rc, "DebugActiveProcessStop(%ld) failed err=%ld\n", dbg_blackbox.pid, dbg_blackbox.detach_err);
    ok(!dbg_blackbox.failures, "debugger reported %lu failures\n", dbg_blackbox.failures);

    DeleteFileA(dbglog);
}

static void crash_and_winedbg(HKEY hkey, const char* argv0)
{
    BOOL bRet;
    DWORD ret;
    char* cmd;
    PROCESS_INFORMATION	info;
    STARTUPINFOA startup;
    DWORD exit_code;

    ret=RegSetValueExA(hkey, "auto", 0, REG_SZ, (BYTE*)"1", 2);
    ok(ret == ERROR_SUCCESS, "unable to set AeDebug/auto: ret=%ld\n", ret);

    cmd=HeapAlloc(GetProcessHeap(), 0, strlen(argv0)+15+1);
    sprintf(cmd, "%s debugger crash", argv0);

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    ret=CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess: err=%ld\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, cmd);
    CloseHandle(info.hThread);

    trace("waiting for child exit...\n");
    ok(WaitForSingleObject(info.hProcess, 60000) == WAIT_OBJECT_0, "Timed out waiting for the child to crash\n");
    bRet = GetExitCodeProcess(info.hProcess, &exit_code);
    ok(bRet, "GetExitCodeProcess failed: err=%ld\n", GetLastError());
    ok(exit_code == STATUS_ACCESS_VIOLATION, "exit code = %08lx\n", exit_code);
    CloseHandle(info.hProcess);
}

static void test_ExitCode(void)
{
    static const char* AeDebug="Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug";
    static const char* WineDbg="Software\\Wine\\WineDbg";
    char test_exe[MAX_PATH];
    DWORD ret;
    HKEY hkey;
    DWORD disposition;
    reg_save_value auto_value;
    reg_save_value debugger_value;

    GetModuleFileNameA(GetModuleHandleA(NULL), test_exe, sizeof(test_exe));
    if (GetFileAttributesA(test_exe) == INVALID_FILE_ATTRIBUTES)
        strcat(test_exe, ".so");
    if (GetFileAttributesA(test_exe) == INVALID_FILE_ATTRIBUTES)
    {
        ok(0, "could not find the test executable '%s'\n", test_exe);
        return;
    }

    ret=RegCreateKeyExA(HKEY_LOCAL_MACHINE, AeDebug, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &disposition);
    if (ret == ERROR_SUCCESS)
    {
        save_value(hkey, "auto", &auto_value);
        save_value(hkey, "debugger", &debugger_value);
        trace("HKLM\\%s\\debugger is set to '%s'\n", AeDebug, debugger_value.data);
    }
    else if (ret == ERROR_ACCESS_DENIED)
    {
        skip("not enough privileges to change the debugger\n");
        return;
    }
    else if (ret != ERROR_FILE_NOT_FOUND)
    {
        ok(0, "could not open the AeDebug key: %ld\n", ret);
        return;
    }
    else
    {
        auto_value.data = NULL;
        debugger_value.data = NULL;
    }

    if (debugger_value.data && debugger_value.type == REG_SZ &&
        strstr((char*)debugger_value.data, "winedbg --auto"))
    {
        HKEY hkeyWinedbg;
        ret=RegCreateKeyA(HKEY_CURRENT_USER, WineDbg, &hkeyWinedbg);
        if (ret == ERROR_SUCCESS)
        {
            static DWORD zero;
            reg_save_value crash_dlg_value;
            save_value(hkeyWinedbg, "ShowCrashDialog", &crash_dlg_value);
            RegSetValueExA(hkeyWinedbg, "ShowCrashDialog", 0, REG_DWORD, (BYTE *)&zero, sizeof(DWORD));
            ignore_exceptions(TRUE);
            crash_and_winedbg(hkey, test_exe);
            ignore_exceptions(FALSE);
            restore_value(hkeyWinedbg, &crash_dlg_value);
            RegCloseKey(hkeyWinedbg);
        }
        else
            ok(0, "Couldn't access WineDbg Key - error %lu\n", ret);
    }

    if (winetest_interactive)
        /* Since the debugging process never sets the debug event, it isn't recognized
           as a valid debugger and, after the debugger exits, Windows will show a dialog box
           asking the user what to do */
        crash_and_debug(hkey, test_exe, "dbg,none");
    else
        skip("\"none\" debugger test needs user interaction\n");
    ok(disposition == REG_OPENED_EXISTING_KEY, "expected REG_OPENED_EXISTING_KEY, got %ld\n", disposition);
    crash_and_debug(hkey, test_exe, "dbg,event,order");
    crash_and_debug(hkey, test_exe, "dbg,attach,event,code2");
    crash_and_debug(hkey, test_exe, "dbg,attach,event,nokill");
    crash_and_debug(hkey, test_exe, "dbg,attach,event,detach");
    crash_and_debug(hkey, test_exe, "dbg,attach,detach,late");
    crash_and_debug(hkey, test_exe, "dbg,attach,process,event,detach");

    if (disposition == REG_CREATED_NEW_KEY)
    {
        RegCloseKey(hkey);
        RegDeleteKeyA(HKEY_LOCAL_MACHINE, AeDebug);
    }
    else
    {
        restore_value(hkey, &auto_value);
        restore_value(hkey, &debugger_value);
        RegCloseKey(hkey);
    }
}

static void test_RemoteDebugger(void)
{
    BOOL bret, present;
    if(!pCheckRemoteDebuggerPresent)
    {
        win_skip("CheckRemoteDebuggerPresent is not available\n");
        return;
    }
    present = TRUE;
    SetLastError(0xdeadbeef);
    bret = pCheckRemoteDebuggerPresent(GetCurrentProcess(),&present);
    ok(bret , "expected CheckRemoteDebuggerPresent to succeed\n");
    ok(0xdeadbeef == GetLastError(),
       "expected error to be unchanged, got %ld/%lx\n",GetLastError(), GetLastError());

    present = TRUE;
    SetLastError(0xdeadbeef);
    bret = pCheckRemoteDebuggerPresent(NULL,&present);
    ok(!bret , "expected CheckRemoteDebuggerPresent to fail\n");
    ok(present, "expected parameter to be unchanged\n");
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "expected error ERROR_INVALID_PARAMETER, got %ld/%lx\n",GetLastError(), GetLastError());

    SetLastError(0xdeadbeef);
    bret = pCheckRemoteDebuggerPresent(GetCurrentProcess(),NULL);
    ok(!bret , "expected CheckRemoteDebuggerPresent to fail\n");
    ok(ERROR_INVALID_PARAMETER == GetLastError(),
       "expected error ERROR_INVALID_PARAMETER, got %ld/%lx\n",GetLastError(), GetLastError());
}

struct child_blackbox
{
    LONG failures;
};

static void doChild(int argc, char **argv)
{
    struct child_blackbox blackbox;
    const char *blackbox_file;
    WCHAR path[MAX_PATH];
    HMODULE mod;
    HANDLE parent, file, map;
    DWORD ppid;
    BOOL debug;
    BOOL ret;

    blackbox_file = argv[4];
    sscanf(argv[3], "%08lx", &ppid);

    parent = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, ppid);
    child_ok(!!parent, "OpenProcess failed, last error %#lx.\n", GetLastError());

    ret = pCheckRemoteDebuggerPresent(parent, &debug);
    child_ok(ret, "CheckRemoteDebuggerPresent failed, last error %#lx.\n", GetLastError());
    child_ok(!debug, "Expected debug == 0, got %#x.\n", debug);

    ret = DebugActiveProcess(ppid);
    child_ok(ret, "DebugActiveProcess failed, last error %#lx.\n", GetLastError());

    ret = pCheckRemoteDebuggerPresent(parent, &debug);
    child_ok(ret, "CheckRemoteDebuggerPresent failed, last error %#lx.\n", GetLastError());
    child_ok(debug, "Expected debug != 0, got %#x.\n", debug);

    ret = DebugActiveProcessStop(ppid);
    child_ok(ret, "DebugActiveProcessStop failed, last error %#lx.\n", GetLastError());

    ret = pCheckRemoteDebuggerPresent(parent, &debug);
    child_ok(ret, "CheckRemoteDebuggerPresent failed, last error %#lx.\n", GetLastError());
    child_ok(!debug, "Expected debug == 0, got %#x.\n", debug);

    ret = CloseHandle(parent);
    child_ok(ret, "CloseHandle failed, last error %#lx.\n", GetLastError());

    ret = IsDebuggerPresent();
    child_ok(ret, "Expected ret != 0, got %#x.\n", ret);
    ret = pCheckRemoteDebuggerPresent(GetCurrentProcess(), &debug);
    child_ok(ret, "CheckRemoteDebuggerPresent failed, last error %#lx.\n", GetLastError());
    child_ok(debug, "Expected debug != 0, got %#x.\n", debug);

    NtCurrentTeb()->Peb->BeingDebugged = FALSE;

    ret = IsDebuggerPresent();
    child_ok(!ret, "Expected ret != 0, got %#x.\n", ret);
    ret = pCheckRemoteDebuggerPresent(GetCurrentProcess(), &debug);
    child_ok(ret, "CheckRemoteDebuggerPresent failed, last error %#lx.\n", GetLastError());
    child_ok(debug, "Expected debug != 0, got %#x.\n", debug);

    NtCurrentTeb()->Peb->BeingDebugged = TRUE;

    mod = LoadLibraryW( L"ole32.dll" );
    FreeLibrary( mod );

    GetSystemDirectoryW( path, MAX_PATH );
    wcscat( path, L"\\oleaut32.dll" );
    file = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    child_ok( file != INVALID_HANDLE_VALUE, "failed to open %s: %lu\n", debugstr_w(path), GetLastError());
    map = CreateFileMappingW( file, NULL, SEC_IMAGE | PAGE_READONLY, 0, 0, NULL );
    child_ok( map != NULL, "failed to create mapping %s: %lu\n", debugstr_w(path), GetLastError() );
    mod = MapViewOfFile( map, FILE_MAP_READ, 0, 0, 0 );
    child_ok( mod != NULL, "failed to map %s: %lu\n", debugstr_w(path), GetLastError() );
    CloseHandle( file );
    CloseHandle( map );
    UnmapViewOfFile( mod );

    if (sizeof(void *) > sizeof(int))
    {
        GetSystemWow64DirectoryW( path, MAX_PATH );
        wcscat( path, L"\\oleacc.dll" );
    }
    else if (is_wow64)
    {
        wcscpy( path, L"c:\\windows\\sysnative\\oleacc.dll" );
    }
    else goto done;

    file = CreateFileW( path, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, 0 );
    child_ok( file != INVALID_HANDLE_VALUE, "failed to open %s: %lu\n", debugstr_w(path), GetLastError());
    map = CreateFileMappingW( file, NULL, SEC_IMAGE | PAGE_READONLY, 0, 0, NULL );
    child_ok( map != NULL, "failed to create mapping %s: %lu\n", debugstr_w(path), GetLastError() );
    mod = MapViewOfFile( map, FILE_MAP_READ, 0, 0, 0 );
    child_ok( mod != NULL, "failed to map %s: %lu\n", debugstr_w(path), GetLastError() );
    CloseHandle( file );
    CloseHandle( map );
    UnmapViewOfFile( mod );

done:
    blackbox.failures = child_failures;
    save_blackbox(blackbox_file, &blackbox, sizeof(blackbox), NULL);
}

static HMODULE ole32_mod, oleaut32_mod, oleacc_mod;

static void check_dll_event( HANDLE process, DEBUG_EVENT *ev )
{
    WCHAR *p, module[MAX_PATH];

    switch (ev->dwDebugEventCode)
    {
    case CREATE_PROCESS_DEBUG_EVENT:
        break;
    case LOAD_DLL_DEBUG_EVENT:
        if (!pGetMappedFileNameW( process, ev->u.LoadDll.lpBaseOfDll, module, MAX_PATH )) module[0] = 0;
        if ((p = wcsrchr( module, '\\' ))) p++;
        else p = module;
        if (!wcsicmp( p, L"ole32.dll" )) ole32_mod = ev->u.LoadDll.lpBaseOfDll;
        else if (!wcsicmp( p, L"oleaut32.dll" )) oleaut32_mod = ev->u.LoadDll.lpBaseOfDll;
        else if (!wcsicmp( p, L"oleacc.dll" )) oleacc_mod = ev->u.LoadDll.lpBaseOfDll;
        break;
    case UNLOAD_DLL_DEBUG_EVENT:
        if (ev->u.UnloadDll.lpBaseOfDll == ole32_mod) ole32_mod = (HMODULE)1;
        if (ev->u.UnloadDll.lpBaseOfDll == oleaut32_mod) oleaut32_mod = (HMODULE)1;
        if (ev->u.UnloadDll.lpBaseOfDll == oleacc_mod) oleacc_mod = (HMODULE)1;
        break;
    }
}

static void test_debug_loop(int argc, char **argv)
{
    const char *arguments = " debugger child ";
    struct child_blackbox blackbox;
    char blackbox_file[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    BOOL debug;
    DWORD pid;
    char *cmd;
    BOOL ret;

    if (!pCheckRemoteDebuggerPresent)
    {
        win_skip("CheckRemoteDebuggerPresent not available, skipping test.\n");
        return;
    }
    if (sizeof(void *) > sizeof(int))
    {
        WCHAR buffer[MAX_PATH];
        GetSystemWow64DirectoryW( buffer, MAX_PATH );
        wcscat( buffer, L"\\oleacc.dll" );
        if (GetFileAttributesW( buffer ) == INVALID_FILE_ATTRIBUTES)
        {
            skip("Skipping test on 64bit only configuration\n");
            return;
        }
    }

    pid = GetCurrentProcessId();
    ret = DebugActiveProcess(pid);
    ok(!ret, "DebugActiveProcess() succeeded on own process.\n");

    get_file_name(blackbox_file);
    cmd = HeapAlloc(GetProcessHeap(), 0, strlen(argv[0]) + strlen(arguments) + strlen(blackbox_file) + 2 + 10);
    sprintf(cmd, "%s%s%08lx \"%s\"", argv[0], arguments, pid, blackbox_file);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    ret = CreateProcessA(NULL, cmd, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed, last error %#lx.\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, cmd);

    ret = pCheckRemoteDebuggerPresent(pi.hProcess, &debug);
    ok(ret, "CheckRemoteDebuggerPresent failed, last error %#lx.\n", GetLastError());
    ok(debug, "Expected debug != 0, got %#x.\n", debug);

    for (;;)
    {
        DEBUG_EVENT ev;

        ret = WaitForDebugEvent(&ev, INFINITE);
        ok(ret, "WaitForDebugEvent failed, last error %#lx.\n", GetLastError());
        if (!ret) break;

        if (ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) break;
        check_dll_event( pi.hProcess, &ev );
#if defined(__i386__) || defined(__x86_64__)
        if (ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT &&
            ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
        {
            BYTE byte = 0;
            NtReadVirtualMemory(pi.hProcess, ev.u.Exception.ExceptionRecord.ExceptionAddress, &byte, 1, NULL);
            ok(byte == 0xcc, "got %02x\n", byte);
        }
#endif
        ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
        ok(ret, "ContinueDebugEvent failed, last error %#lx.\n", GetLastError());
        if (!ret) break;
    }

    /* sometimes not all unload events are sent on win7 */
    ok( ole32_mod == (HMODULE)1 || broken( ole32_mod != NULL ), "ole32.dll was not reported\n" );
    ok( oleaut32_mod == (HMODULE)1, "oleaut32.dll was not reported\n" );
#ifdef _WIN64
    ok( oleacc_mod == (HMODULE)1, "oleacc.dll was not reported\n" );
#else
    ok( oleacc_mod == NULL, "oleacc.dll was reported\n" );
#endif

    ret = CloseHandle(pi.hThread);
    ok(ret, "CloseHandle failed, last error %#lx.\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "CloseHandle failed, last error %#lx.\n", GetLastError());

    load_blackbox(blackbox_file, &blackbox, sizeof(blackbox));
    ok(!blackbox.failures, "Got %ld failures from child process.\n", blackbox.failures);

    ret = DeleteFileA(blackbox_file);
    ok(ret, "DeleteFileA failed, last error %#lx.\n", GetLastError());
}

struct find_main_window
{
    DWORD       pid;
    unsigned    count;
    HWND        windows[5];
};

static BOOL CALLBACK enum_windows_callback(HWND handle, LPARAM lParam)
{
    struct find_main_window* fmw = (struct find_main_window*)lParam;
    DWORD pid = 0;

    if (GetWindowThreadProcessId(handle, &pid) && fmw->pid == pid &&
        !GetWindow(handle, GW_OWNER))
    {
        ok(fmw->count < ARRAY_SIZE(fmw->windows), "Too many windows\n");
        if (fmw->count < ARRAY_SIZE(fmw->windows))
            fmw->windows[fmw->count++] = handle;
    }
    return TRUE;
}

static void close_main_windows(DWORD pid)
{
    struct find_main_window fmw = {pid, 0};
    unsigned i;

    EnumWindows(enum_windows_callback, (LPARAM)&fmw);
    ok(fmw.count, "no window found\n");
    for (i = 0; i < fmw.count; i++)
        PostMessageA(fmw.windows[i], WM_CLOSE, 0, 0);
}

static void test_debug_loop_wow64(void)
{
    WCHAR buffer[MAX_PATH], *p;
    PROCESS_INFORMATION pi;
    STARTUPINFOW si;
    BOOL ret;
    unsigned order = 0, bp_order = 0, bpwx_order = 0, num_ntdll = 0, num_wow64 = 0;

    /* checking conditions for running this test */
    if (GetSystemWow64DirectoryW( buffer, ARRAY_SIZE(buffer) ) && sizeof(void*) > sizeof(int) && pGetMappedFileNameW)
    {
        wcscat( buffer, L"\\msinfo32.exe" );
        ret = GetFileAttributesW( buffer ) != INVALID_FILE_ATTRIBUTES;
    }
    else ret = FALSE;
    if (!ret)
    {
        skip("Skipping test on incompatible config\n");
        return;
    }
    memset( &si, 0, sizeof(si) );
    si.cb = sizeof(si);
    ret = CreateProcessW( NULL, buffer, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &si, &pi );
    ok(ret, "CreateProcess failed, last error %#lx.\n", GetLastError());

    for (;;)
    {
        DEBUG_EVENT ev;

        ++order;
        ret = WaitForDebugEvent( &ev, 2000 );
        if (!ret) break;

        switch (ev.dwDebugEventCode)
        {
        case CREATE_PROCESS_DEBUG_EVENT:
            break;
        case LOAD_DLL_DEBUG_EVENT:
            if (!pGetMappedFileNameW( pi.hProcess, ev.u.LoadDll.lpBaseOfDll, buffer, ARRAY_SIZE(buffer) )) buffer[0] = L'\0';
            if ((p = wcsrchr( buffer, '\\' ))) p++;
            else p = buffer;
            if (!wcsnicmp( p, L"wow64", 5 ) || !wcsicmp( p, L"xtajit.dll" ))
            {
                /* on Win10, wow64cpu's load dll event is received after first exception */
                ok(bpwx_order == 0, "loaddll for wow64 DLLs should appear before exception\n");
                num_wow64++;
            }
            else if (!wcsicmp( p, L"ntdll.dll" ))
            {
                ok(bp_order == 0 && bpwx_order == 0, "loaddll on ntdll should appear before exception\n");
                num_ntdll++;
            }
            break;
        case EXCEPTION_DEBUG_EVENT:
            if (ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT)
                bp_order = order;
            else if (ev.u.Exception.ExceptionRecord.ExceptionCode == STATUS_WX86_BREAKPOINT)
                bpwx_order = order;
        }
        ret = ContinueDebugEvent(ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE);
        ok(ret, "ContinueDebugEvent failed, last error %#lx.\n", GetLastError());
        if (!ret) break;
    }

    /* gracefully terminates msinfo32 */
    close_main_windows( pi.dwProcessId );

    /* eat up the remaining events... not generating unload dll events in case of process termination */
    for (;;)
    {
        DEBUG_EVENT ev;

        ret = WaitForDebugEvent( &ev, 2000 );
        if (!ret || ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) break;
        switch (ev.dwDebugEventCode)
        {
        default:
            ok(0, "Unexpected event: %lu\n", ev.dwDebugEventCode);
            /* fall through */
        case EXIT_THREAD_DEBUG_EVENT:
            ret = ContinueDebugEvent( ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE );
            ok(ret, "ContinueDebugEvent failed, last error %#lx.\n", GetLastError());
            break;
        }
    }

    ret = WaitForSingleObject( pi.hProcess, 2000 );
    if (ret != WAIT_OBJECT_0)
    {
        DWORD ec;
        ret = GetExitCodeProcess( pi.hProcess, &ec );
        ok(ret, "GetExitCodeProcess failed: %lu\n", GetLastError());
        ok(ec != STILL_ACTIVE, "GetExitCodeProcess still active\n");
    }
    for (;;)
    {
        DEBUG_EVENT ev;

        ret = WaitForDebugEvent( &ev, 2000 );
        if (!ret || ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT) break;
        switch (ev.dwDebugEventCode)
        {
        default:
            ok(0, "Unexpected event: %lu\n", ev.dwDebugEventCode);
            /* fall through */
        case EXIT_THREAD_DEBUG_EVENT:
            ret = ContinueDebugEvent( ev.dwProcessId, ev.dwThreadId, DBG_CONTINUE );
            ok(ret, "ContinueDebugEvent failed, last error %#lx.\n", GetLastError());
            break;
        }
    }
    ret = CloseHandle( pi.hThread );
    ok(ret, "CloseHandle failed, last error %#lx.\n", GetLastError());
    ret = CloseHandle( pi.hProcess );
    ok(ret, "CloseHandle failed, last error %#lx.\n", GetLastError());

    if (strcmp( winetest_platform, "wine" ) || num_wow64) /* windows or new wine wow */
    {
        ok(num_ntdll == 2, "Expecting two ntdll instances\n");
        ok(num_wow64 >= 3, "Expecting more than 3 wow64*.dll\n");
    }
    else /* Wine's old wow, or 32/64 bit only configurations */
    {
        ok(num_ntdll == 1, "Expecting one ntdll instances\n");
        ok(num_wow64 == 0, "Expecting more no wow64*.dll\n");
    }
    ok(bp_order, "Expecting 1 bp exceptions\n");
    todo_wine
    {
        ok(bpwx_order, "Expecting 1 bpwx exceptions\n");
        ok(bp_order < bpwx_order, "Out of order bp exceptions\n");
    }
}

static void doChildren(int argc, char **argv)
{
    const char *arguments = "debugger children last";
    struct child_blackbox blackbox;
    const char *blackbox_file, *p;
    char event_name[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    HANDLE event;
    char *cmd;
    BOOL ret;

    if (!strcmp(argv[3], "last")) return;

    blackbox_file = argv[3];

    run_background_thread();

    p = strrchr(blackbox_file, '\\');
    p = p ? p+1 : blackbox_file;
    strcpy(event_name, p);
    strcat(event_name, "_init");
    event = OpenEventA(EVENT_ALL_ACCESS, FALSE, event_name);
    child_ok(event != NULL, "OpenEvent failed, last error %ld.\n", GetLastError());
    SetEvent(event);
    CloseHandle(event);

    p = strrchr(blackbox_file, '\\');
    p = p ? p+1 : blackbox_file;
    strcpy(event_name, p);
    strcat(event_name, "_attach");
    event = OpenEventA(EVENT_ALL_ACCESS, FALSE, event_name);
    child_ok(event != NULL, "OpenEvent failed, last error %ld.\n", GetLastError());
    WaitForSingleObject(event, INFINITE);
    CloseHandle(event);

    cmd = HeapAlloc(GetProcessHeap(), 0, strlen(argv[0]) + strlen(arguments) + 2);
    sprintf(cmd, "%s %s", argv[0], arguments);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    ret = CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &si, &pi);
    child_ok(ret, "CreateProcess failed, last error %ld.\n", GetLastError());

    child_ok(WaitForSingleObject(pi.hProcess, 10000) == WAIT_OBJECT_0,
            "Timed out waiting for the child to exit\n");

    ret = CloseHandle(pi.hThread);
    child_ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    child_ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());

    blackbox.failures = child_failures;
    save_blackbox(blackbox_file, &blackbox, sizeof(blackbox), NULL);

    HeapFree(GetProcessHeap(), 0, cmd);
}

static void test_debug_children(const char *name, DWORD flag, BOOL debug_child, BOOL pass_exception)
{
    const char *arguments = "debugger children";
    struct child_blackbox blackbox;
    char blackbox_file[MAX_PATH], *p;
    char event_name[MAX_PATH];
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    HANDLE event_init, event_attach;
    char *cmd;
    BOOL debug, ret;
    struct debugger_context ctx = { 0 };

    if (!pCheckRemoteDebuggerPresent)
    {
        win_skip("CheckRemoteDebuggerPresent not available, skipping test.\n");
        return;
    }

    get_file_name(blackbox_file);
    cmd = HeapAlloc(GetProcessHeap(), 0, strlen(name) + strlen(arguments) + strlen(blackbox_file) + 5);
    sprintf(cmd, "%s %s \"%s\"", name, arguments, blackbox_file);

    p = strrchr(blackbox_file, '\\');
    p = p ? p+1 : blackbox_file;
    strcpy(event_name, p);
    strcat(event_name, "_init");
    event_init = CreateEventA(NULL, FALSE, FALSE, event_name);
    ok(event_init != NULL, "OpenEvent failed, last error %ld.\n", GetLastError());

    p = strrchr(blackbox_file, '\\');
    p = p ? p+1 : blackbox_file;
    strcpy(event_name, p);
    strcat(event_name, "_attach");
    event_attach = CreateEventA(NULL, FALSE, flag!=0, event_name);
    ok(event_attach != NULL, "CreateEvent failed, last error %ld.\n", GetLastError());

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);

    ret = CreateProcessA(NULL, cmd, NULL, NULL, FALSE, flag, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed, last error %ld.\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, cmd);
    if (!flag)
    {
        WaitForSingleObject(event_init, INFINITE);
        Sleep(100);
        ret = DebugActiveProcess(pi.dwProcessId);
        ok(ret, "DebugActiveProcess failed, last error %ld.\n", GetLastError());
    }

    ret = pCheckRemoteDebuggerPresent(pi.hProcess, &debug);
    ok(ret, "CheckRemoteDebuggerPresent failed, last error %ld.\n", GetLastError());
    ok(debug, "Expected debug != 0, got %x.\n", debug);

    trace("starting debugger loop\n");

    if (flag)
    {
        DWORD last_thread;

        next_event(&ctx, WAIT_EVENT_TIMEOUT);
        ok(ctx.ev.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
        ok(ctx.pid == pi.dwProcessId, "unexpected dwProcessId %x\n", ctx.ev.dwProcessId == ctx.pid);

        next_event(&ctx, WAIT_EVENT_TIMEOUT);
        ok(ctx.ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
        last_thread = ctx.ev.dwThreadId;

        wait_for_breakpoint(&ctx);
        ok(ctx.dll_cnt > 2, "dll_cnt = %d\n", ctx.dll_cnt);

        ok(ctx.ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
        ok(ctx.ev.dwThreadId == last_thread, "unexpected thread\n");
        ok(ctx.ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT, "ExceptionCode = %lx\n",
           ctx.ev.u.Exception.ExceptionRecord.ExceptionCode);

        /* Except for wxppro and w2008, the initial breakpoint is now somewhere else, possibly within LdrInitShimEngineDynamic,
         * It's also catching exceptions and ContinueDebugEvent(DBG_EXCEPTION_NOT_HANDLED) should not crash the child now */
        ok(ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress != pDbgBreakPoint, "ExceptionAddress == pDbgBreakPoint\n");

        if (pass_exception)
        {
            ret = ContinueDebugEvent(ctx.ev.dwProcessId, ctx.ev.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
            ok(ret, "ContinueDebugEvent failed, last error %ld.\n", GetLastError());
            ctx.ev.dwDebugEventCode = -1;

            next_event(&ctx, WAIT_EVENT_TIMEOUT);
            ok(ctx.ev.dwDebugEventCode != EXCEPTION_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
        }
    }
    else
    {
        DWORD last_thread;

        process_attach_events(&ctx, pass_exception);
        ok(ctx.pid == pi.dwProcessId, "unexpected dwProcessId %lx\n", ctx.pid);

        ret = DebugBreakProcess(pi.hProcess);
        ok(ret, "BreakProcess failed: %lu\n", GetLastError());

        /* a new thread, which executes DbgBreakPoint, is created */
        next_event(&ctx, WAIT_EVENT_TIMEOUT);
        ok(ctx.ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
        last_thread = ctx.ev.dwThreadId;

        if (ctx.ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT)
            next_event(&ctx, WAIT_EVENT_TIMEOUT);

        ok(ctx.ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
        ok(ctx.ev.dwThreadId == last_thread, "unexpected thread\n");
        ok(ctx.ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT, "ExceptionCode = %lx\n",
           ctx.ev.u.Exception.ExceptionRecord.ExceptionCode);
        ok(ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress == pDbgBreakPoint, "ExceptionAddress != DbgBreakPoint\n");

        ret = SetEvent(event_attach);
        ok(ret, "SetEvent failed, last error %ld.\n", GetLastError());

        if (pass_exception)
        {
            ret = ContinueDebugEvent(ctx.ev.dwProcessId, ctx.ev.dwThreadId, DBG_EXCEPTION_NOT_HANDLED);
            ok(ret, "ContinueDebugEvent failed, last error %ld.\n", GetLastError());
            ctx.ev.dwDebugEventCode = -1;
        }
    }

    do next_event(&ctx, WAIT_EVENT_TIMEOUT);
    while (ctx.ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT || ctx.ev.dwDebugEventCode == UNLOAD_DLL_DEBUG_EVENT
           || ctx.ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT || ctx.ev.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT);

    ok(ctx.ev.dwDebugEventCode == EXIT_PROCESS_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
    ret = ContinueDebugEvent(ctx.ev.dwProcessId, ctx.ev.dwThreadId, DBG_CONTINUE);
    ok(ret, "ContinueDebugEvent failed, last error %ld.\n", GetLastError());

    if(debug_child)
        ok(ctx.process_cnt == 2, "didn't get any child events (flag: %lx).\n", flag);
    else
        ok(ctx.process_cnt == 1, "got child event (flag: %lx).\n", flag);
    CloseHandle(event_init);
    CloseHandle(event_attach);

    ret = CloseHandle(pi.hThread);
    ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());

    load_blackbox(blackbox_file, &blackbox, sizeof(blackbox));
    ok(!blackbox.failures, "Got %ld failures from child process.\n", blackbox.failures);

    ret = DeleteFileA(blackbox_file);
    ok(ret, "DeleteFileA failed, last error %ld.\n", GetLastError());
}

static void wait_debugger(HANDLE event, unsigned int cnt)
{
    while (cnt--) WaitForSingleObject(event, INFINITE);
    ExitProcess(0);
}

#define expect_event(a,b) expect_event_(__LINE__,a,b)
static void expect_event_(unsigned line, struct debugger_context *ctx, DWORD event_code)
{
    next_event(ctx, WAIT_EVENT_TIMEOUT);
    ok_(__FILE__,line)(ctx->ev.dwDebugEventCode == event_code, "dwDebugEventCode = %ld expected %ld\n",
                       ctx->ev.dwDebugEventCode, event_code);
}

#define expect_exception(a,b) expect_exception_(__LINE__,a,b)
static void expect_exception_(unsigned line, struct debugger_context *ctx, DWORD exception_code)
{
    expect_event_(line, ctx, EXCEPTION_DEBUG_EVENT);
    ok_(__FILE__,line)(ctx->ev.u.Exception.ExceptionRecord.ExceptionCode == exception_code, "ExceptionCode = %lx expected %lx\n",
                       ctx->ev.u.Exception.ExceptionRecord.ExceptionCode, exception_code);
}

#define check_breakpoint_exception(a,b) expect_breakpoint_exception_(__LINE__,a,b)
static void check_breakpoint_exception_(unsigned line, struct debugger_context *ctx, const void *expect_addr)
{
    struct debuggee_thread *thread;
    if (!expect_addr) return;
    ok_(__FILE__,line)(ctx->ev.u.Exception.ExceptionRecord.ExceptionAddress == expect_addr,
                       "ExceptionAddress = %p expected %p\n", ctx->ev.u.Exception.ExceptionRecord.ExceptionAddress, expect_addr);
    thread = get_debuggee_thread(ctx, ctx->ev.dwThreadId);
    fetch_thread_context(thread);
    ok_(__FILE__,line)(get_ip(&thread->ctx) == (char*)expect_addr + 1, "unexpected instruction pointer %p expected %p\n",
                       get_ip(&thread->ctx), expect_addr);
}

#define expect_breakpoint_exception(a,b) expect_breakpoint_exception_(__LINE__,a,b)
static void expect_breakpoint_exception_(unsigned line, struct debugger_context *ctx, const void *expect_addr)
{
    expect_exception_(line, ctx, EXCEPTION_BREAKPOINT);
    check_breakpoint_exception_(line, ctx, expect_addr);
}

#define single_step(a,b,c) single_step_(__LINE__,a,b,c)
static void single_step_(unsigned line, struct debugger_context *ctx, struct debuggee_thread *thread, void *expect_addr)
{
#if defined(__i386__) || defined(__x86_64__)
    fetch_thread_context(thread);
    thread->ctx.EFlags |= 0x100;
    set_thread_context(ctx, thread);
    expect_exception_(line, ctx, EXCEPTION_SINGLE_STEP);
    ok_(__FILE__,line)(ctx->ev.u.Exception.ExceptionRecord.ExceptionAddress == expect_addr,
                       "ExceptionAddress = %p expected %p\n", ctx->ev.u.Exception.ExceptionRecord.ExceptionAddress, expect_addr);
    fetch_thread_context(thread);
    ok_(__FILE__,line)(get_ip(&thread->ctx) == expect_addr, "unexpected instruction pointer %p expected %p\n",
                       get_ip(&thread->ctx), expect_addr);
    ok_(__FILE__,line)(!(thread->ctx.EFlags & 0x100), "EFlags = %lx\n", thread->ctx.EFlags);
#endif
}

static const BYTE loop_code[] = {
#if defined(__i386__) || defined(__x86_64__)
    0x90,                         /* nop */
    0x90,                         /* nop */
    0x90,                         /* nop */
    0xe9, 0xf8, 0xff, 0xff, 0xff  /* jmp $-8 */
#endif
};

static const BYTE call_debug_service_code[] = {
#ifdef __i386__
    0x53,                         /* pushl %ebx */
    0x57,                         /* pushl %edi */
    0x8b, 0x44, 0x24, 0x0c,       /* movl 12(%esp),%eax */
    0xb9, 0x11, 0x11, 0x11, 0x11, /* movl $0x11111111,%ecx */
    0xba, 0x22, 0x22, 0x22, 0x22, /* movl $0x22222222,%edx */
    0xbb, 0x33, 0x33, 0x33, 0x33, /* movl $0x33333333,%ebx */
    0xbf, 0x44, 0x44, 0x44, 0x44, /* movl $0x44444444,%edi */
    0xcd, 0x2d,                   /* int $0x2d */
    0xeb,                         /* jmp $+17 */
    0x0f, 0x1f, 0x00,             /* nop */
    0x31, 0xc0,                   /* xorl %eax,%eax */
    0xeb, 0x0c,                   /* jmp $+14 */
    0x90, 0x90, 0x90, 0x90,       /* nop */
    0x90, 0x90, 0x90, 0x90,
    0x90,
    0x31, 0xc0,                   /* xorl %eax,%eax */
    0x40,                         /* incl %eax */
    0x5f,                         /* popl %edi */
    0x5b,                         /* popl %ebx */
    0xc3,                         /* ret */
#elif defined(__x86_64__)
    0x53,                         /* push %rbx */
    0x57,                         /* push %rdi */
    0x48, 0x89, 0xc8,             /* movl %rcx,%rax */
    0x48, 0xb9, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, 0x11, /* movabs $0x1111111111111111,%rcx */
    0x48, 0xba, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, 0x22, /* movabs $0x2222222222222222,%rdx */
    0x48, 0xbb, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x33, /* movabs $0x3333333333333333,%rbx */
    0x48, 0xbf, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, 0x44, /* movabs $0x4444444444444444,%rdi */
    0xcd, 0x2d,                   /* int $0x2d */
    0xeb,                         /* jmp $+17 */
    0x0f, 0x1f, 0x00,             /* nop */
    0x48, 0x31, 0xc0,             /* xor %rax,%rax */
    0xeb, 0x0e,                   /* jmp $+16 */
    0x90, 0x90, 0x90, 0x90,       /* nop */
    0x90, 0x90, 0x90, 0x90,
    0x48, 0x31, 0xc0,             /* xor %rax,%rax */
    0x48, 0xff, 0xc0,             /* inc %rax */
    0x5f,                         /* pop %rdi */
    0x5b,                         /* pop %rbx */
    0xc3,                         /* ret */
#endif
};

#if defined(__i386__) || defined(__x86_64__)
#define OP_BP 0xcc
#else
#define OP_BP 0
#endif

static void test_debugger(const char *argv0)
{
    static const char arguments[] = " debugger wait ";
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    struct debugger_context ctx = { 0 };
    PROCESS_INFORMATION pi;
    STARTUPINFOA si;
    NTSTATUS status;
    HANDLE event, thread;
    BYTE *mem, buf[4096], *proc_code, *thread_proc, byte;
    unsigned int i, worker_cnt, exception_cnt, skip_reply_later;
    struct debuggee_thread *debuggee_thread;
    char *cmd;
    BOOL ret;

    event = CreateEventW(&sa, FALSE, FALSE, NULL);
    ok(event != NULL, "CreateEvent failed: %lu\n", GetLastError());

    cmd = heap_alloc(strlen(argv0) + strlen(arguments) + 16);
    sprintf(cmd, "%s%s%lx %u\n", argv0, arguments, (DWORD)(DWORD_PTR)event, OP_BP ? 3 : 1);

    memset(&si, 0, sizeof(si));
    si.cb = sizeof(si);
    ret = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed, last error %#lx.\n", GetLastError());
    heap_free(cmd);

    next_event(&ctx, WAIT_EVENT_TIMEOUT);
    ok(ctx.ev.dwDebugEventCode == CREATE_PROCESS_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);

    if ((skip_reply_later = !ContinueDebugEvent(ctx.ev.dwProcessId, ctx.ev.dwThreadId, DBG_REPLY_LATER)))
        win_skip("Skipping unsupported DBG_REPLY_LATER tests\n");
    else
    {
        DEBUG_EVENT de;

        de = ctx.ev;
        ctx.ev.dwDebugEventCode = -1;
        next_event(&ctx, WAIT_EVENT_TIMEOUT);
        ok(de.dwDebugEventCode == ctx.ev.dwDebugEventCode,
           "dwDebugEventCode differs: %lx (was %lx)\n", ctx.ev.dwDebugEventCode, de.dwDebugEventCode);
        ok(de.dwProcessId == ctx.ev.dwProcessId,
           "dwProcessId differs: %lx (was %lx)\n", ctx.ev.dwProcessId, de.dwProcessId);
        ok(de.dwThreadId == ctx.ev.dwThreadId,
           "dwThreadId differs: %lx (was %lx)\n", ctx.ev.dwThreadId, de.dwThreadId);

        /* Suspending the thread should prevent other attach debug events
         * to be received until it's resumed */
        thread = OpenThread(THREAD_SUSPEND_RESUME, FALSE, ctx.ev.dwThreadId);
        ok(thread != INVALID_HANDLE_VALUE, "OpenThread failed, last error:%lu\n", GetLastError());

        status = NtSuspendThread(thread, NULL);
        ok(!status, "NtSuspendThread failed, last error:%lu\n", GetLastError());

        ret = ContinueDebugEvent(ctx.ev.dwProcessId, ctx.ev.dwThreadId, DBG_REPLY_LATER);
        ok(ret, "ContinueDebugEvent failed, last error:%lu\n", GetLastError());
        ok(!WaitForDebugEvent(&ctx.ev, POLL_EVENT_TIMEOUT), "WaitForDebugEvent succeeded.\n");

        status = NtResumeThread(thread, NULL);
        ok(!status, "NtResumeThread failed, last error:%lu\n", GetLastError());

        ret = CloseHandle(thread);
        ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());

        ok(WaitForDebugEvent(&ctx.ev, POLL_EVENT_TIMEOUT), "WaitForDebugEvent failed.\n");
        ok(de.dwDebugEventCode == ctx.ev.dwDebugEventCode,
           "dwDebugEventCode differs: %lx (was %lx)\n", ctx.ev.dwDebugEventCode, de.dwDebugEventCode);

        next_event(&ctx, WAIT_EVENT_TIMEOUT);
        ok(ctx.ev.dwDebugEventCode == LOAD_DLL_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
        de = ctx.ev;

        ret = ContinueDebugEvent(ctx.ev.dwProcessId, ctx.ev.dwThreadId, DBG_REPLY_LATER);
        ok(ret, "ContinueDebugEvent failed, last error:%lu\n", GetLastError());

        ctx.ev.dwDebugEventCode = -1;
        next_event(&ctx, WAIT_EVENT_TIMEOUT);
        ok(de.dwDebugEventCode == ctx.ev.dwDebugEventCode,
           "dwDebugEventCode differs: %lx (was %lx)\n", ctx.ev.dwDebugEventCode, de.dwDebugEventCode);
        ok(de.dwProcessId == ctx.ev.dwProcessId,
           "dwProcessId differs: %lx (was %lx)\n", ctx.ev.dwProcessId, de.dwProcessId);
        ok(de.dwThreadId == ctx.ev.dwThreadId,
           "dwThreadId differs: %lx (was %lx)\n", ctx.ev.dwThreadId, de.dwThreadId);
    }

    wait_for_breakpoint(&ctx);
    do next_event(&ctx, POLL_EVENT_TIMEOUT);
    while(ctx.ev.dwDebugEventCode != -1);

    mem = VirtualAllocEx(pi.hProcess, NULL, sizeof(buf), MEM_COMMIT, PAGE_EXECUTE_READWRITE);
    ok(mem != NULL, "VirtualAllocEx failed: %lu\n", GetLastError());
    proc_code   = buf + 1024;
    thread_proc = mem + 1024;

    if (sizeof(loop_code) > 1)
    {
        /* test single-step exceptions */
        memset(buf, OP_BP, sizeof(buf));
        memcpy(proc_code, &loop_code, sizeof(loop_code));
        proc_code[0] = OP_BP; /* set a breakpoint */
        ret = WriteProcessMemory(pi.hProcess, mem, buf, sizeof(buf), NULL);
        ok(ret, "WriteProcessMemory failed: %lu\n", GetLastError());

        thread = CreateRemoteThread(pi.hProcess, NULL, 0, (void*)thread_proc, NULL, 0, NULL);
        ok(thread != NULL, "CreateRemoteThread failed: %lu\n", GetLastError());

        expect_event(&ctx, CREATE_THREAD_DEBUG_EVENT);
        debuggee_thread = get_debuggee_thread(&ctx, ctx.ev.dwThreadId);

        wait_for_breakpoint(&ctx);
        fetch_thread_context(debuggee_thread);
        ok(ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress == thread_proc,
           "ExceptionAddress = %p\n", ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress);
        ok(get_ip(&debuggee_thread->ctx) == thread_proc + 1, "unexpected instruction pointer %p\n",
           get_ip(&debuggee_thread->ctx));

        single_step(&ctx, debuggee_thread, thread_proc + 2);
        single_step(&ctx, debuggee_thread, thread_proc + 3);
        single_step(&ctx, debuggee_thread, thread_proc);

        byte = 0xc3; /* ret */
        ret = WriteProcessMemory(pi.hProcess, thread_proc, &byte, 1, NULL);
        ok(ret, "WriteProcessMemory failed: %lu\n", GetLastError());

        expect_event(&ctx, EXIT_THREAD_DEBUG_EVENT);
    }
    else todo_wine win_skip("loop_code not supported on this architecture\n");

    if (sizeof(call_debug_service_code) > 1)
    {
        /* test debug service exceptions */
        memset(buf, OP_BP, sizeof(buf));
        memcpy(proc_code, call_debug_service_code, sizeof(call_debug_service_code));
        ret = WriteProcessMemory(pi.hProcess, mem, buf, sizeof(buf), NULL);
        ok(ret, "WriteProcessMemory failed: %lu\n", GetLastError());

        /* BREAKPOINT_PRINT */
        thread = CreateRemoteThread(pi.hProcess, NULL, 0, (void*)thread_proc, (void*)2, 0, NULL);
        ok(thread != NULL, "CreateRemoteThread failed: %lu\n", GetLastError());
        expect_event(&ctx, CREATE_THREAD_DEBUG_EVENT);
        expect_breakpoint_exception(&ctx, NULL);
        expect_event(&ctx, EXIT_THREAD_DEBUG_EVENT);

        /* BREAKPOINT_PROMPT */
        thread = CreateRemoteThread(pi.hProcess, NULL, 0, (void*)thread_proc, (void*)1, 0, NULL);
        ok(thread != NULL, "CreateRemoteThread failed: %lu\n", GetLastError());
        expect_event(&ctx, CREATE_THREAD_DEBUG_EVENT);
        next_event(&ctx, WAIT_EVENT_TIMEOUT);
        /* some 32-bit Windows versions report exception to the debugger */
        if (sizeof(void *) == 4 && ctx.ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) next_event(&ctx, WAIT_EVENT_TIMEOUT);
        ok(ctx.ev.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT, "unexpected debug event %lu\n", ctx.ev.dwDebugEventCode);
    }
    else todo_wine win_skip("call_debug_service_code not supported on this architecture\n");

    if (skip_reply_later)
        win_skip("Skipping unsupported DBG_REPLY_LATER tests\n");
    else if (sizeof(loop_code) > 1)
    {
        HANDLE thread_a, thread_b;
        DEBUG_EVENT de_a, de_b;

        memset(buf, OP_BP, sizeof(buf));
        memcpy(proc_code, &loop_code, sizeof(loop_code));
        ret = WriteProcessMemory(pi.hProcess, mem, buf, sizeof(buf), NULL);
        ok(ret, "WriteProcessMemory failed: %lu\n", GetLastError());

        byte = OP_BP;
        ret = WriteProcessMemory(pi.hProcess, thread_proc + 1, &byte, 1, NULL);
        ok(ret, "WriteProcessMemory failed: %lu\n", GetLastError());

        thread_a = CreateRemoteThread(pi.hProcess, NULL, 0, (void*)thread_proc, NULL, 0, NULL);
        ok(thread_a != NULL, "CreateRemoteThread failed: %lu\n", GetLastError());
        next_event(&ctx, WAIT_EVENT_TIMEOUT);
        ok(ctx.ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
        de_a = ctx.ev;

        thread_b = CreateRemoteThread(pi.hProcess, NULL, 0, (void*)thread_proc, NULL, 0, NULL);
        ok(thread_b != NULL, "CreateRemoteThread failed: %lu\n", GetLastError());
        do next_event(&ctx, POLL_EVENT_TIMEOUT);
        while(ctx.ev.dwDebugEventCode != CREATE_THREAD_DEBUG_EVENT);
        de_b = ctx.ev;

        status = NtSuspendThread(thread_b, NULL);
        ok(!status, "NtSuspendThread failed, last error:%lu\n", GetLastError());
        ret = ContinueDebugEvent(ctx.ev.dwProcessId, ctx.ev.dwThreadId, DBG_REPLY_LATER);
        ok(ret, "ContinueDebugEvent failed, last error:%lu\n", GetLastError());

        ctx.ev.dwDebugEventCode = -1;
        next_event(&ctx, WAIT_EVENT_TIMEOUT);
        ok(ctx.ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT,
           "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
        ok(de_a.dwProcessId == ctx.ev.dwProcessId,
           "dwProcessId differs: %lx (was %lx)\n", ctx.ev.dwProcessId, de_a.dwProcessId);
        ok(de_a.dwThreadId == ctx.ev.dwThreadId,
           "dwThreadId differs: %lx (was %lx)\n", ctx.ev.dwThreadId, de_a.dwThreadId);
        de_a = ctx.ev;

        byte = 0xc3; /* ret */
        ret = WriteProcessMemory(pi.hProcess, thread_proc + 1, &byte, 1, NULL);
        ok(ret, "WriteProcessMemory failed: %lu\n", GetLastError());

        ok(pNtSuspendProcess != NULL, "NtSuspendProcess not found\n");
        ok(pNtResumeProcess != NULL, "pNtResumeProcess not found\n");
        if (pNtSuspendProcess && pNtResumeProcess)
        {
            DWORD action = DBG_REPLY_LATER;
            status = pNtSuspendProcess(pi.hProcess);
            ok(!status, "NtSuspendProcess failed, last error:%lu\n", GetLastError());
            do
            {
                ret = ContinueDebugEvent(ctx.ev.dwProcessId, ctx.ev.dwThreadId, action);
                ok(ret, "ContinueDebugEvent failed, last error:%lu\n", GetLastError());
                ret = WaitForDebugEvent(&ctx.ev, POLL_EVENT_TIMEOUT);
                ok(!ret || ctx.ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT, "WaitForDebugEvent succeeded.\n");
                if (ret) add_thread(&ctx, ctx.ev.dwThreadId);
                action = DBG_CONTINUE;
            } while (ret);

            status = NtResumeThread(thread_b, NULL);
            ok(!status, "NtResumeThread failed, last error:%lu\n", GetLastError());
            while (WaitForDebugEvent(&ctx.ev, POLL_EVENT_TIMEOUT))
            {
                ok(ctx.ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT, "Unexpected debug event %lx\n", ctx.ev.dwDebugEventCode);
                if (ctx.ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT)
                {
                    add_thread(&ctx, ctx.ev.dwThreadId);
                    ret = ContinueDebugEvent(ctx.ev.dwProcessId, ctx.ev.dwThreadId, DBG_CONTINUE);
                    ok(ret, "ContinueDebugEvent failed, last error:%lu\n", GetLastError());
                }
            }

            status = pNtResumeProcess(pi.hProcess);
            ok(!status, "pNtResumeProcess failed, last error:%lu\n", GetLastError());
        }
        else
        {
            status = NtResumeThread(thread_b, NULL);
            ok(!status, "NtResumeThread failed, last error:%lu\n", GetLastError());
            ok(!WaitForDebugEvent(&ctx.ev, POLL_EVENT_TIMEOUT), "WaitForDebugEvent succeeded.\n");
        }

        /* Testing shows that on windows the debug event order between threads
         * is not guaranteed.
         *
         * Now we expect thread_a to report:
         * - its delayed EXCEPTION_DEBUG_EVENT
         * - EXIT_THREAD_DEBUG_EVENT
         *
         * and thread_b to report:
         * - its delayed CREATE_THREAD_DEBUG_EVENT
         * - EXIT_THREAD_DEBUG_EVENT
         *
         * We should not get EXCEPTION_DEBUG_EVENT from thread_b as we updated
         * its instructions before continuing CREATE_THREAD_DEBUG_EVENT.
         */
        ctx.ev.dwDebugEventCode = -1;
        next_event(&ctx, POLL_EVENT_TIMEOUT);

        if (ctx.ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT)
        {
            ok(de_a.dwDebugEventCode == ctx.ev.dwDebugEventCode,
               "dwDebugEventCode differs: %lx (was %lx)\n", ctx.ev.dwDebugEventCode, de_a.dwDebugEventCode);
            ok(de_a.dwProcessId == ctx.ev.dwProcessId,
               "dwProcessId differs: %lx (was %lx)\n", ctx.ev.dwProcessId, de_a.dwProcessId);
            ok(de_a.dwThreadId == ctx.ev.dwThreadId,
               "dwThreadId differs: %lx (was %lx)\n", ctx.ev.dwThreadId, de_a.dwThreadId);

            next_event(&ctx, POLL_EVENT_TIMEOUT);
            if (ctx.ev.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT)
            {
                ok(de_a.dwProcessId == ctx.ev.dwProcessId,
                   "dwProcessId differs: %lx (was %lx)\n", ctx.ev.dwProcessId, de_a.dwProcessId);
                ok(de_a.dwThreadId == ctx.ev.dwThreadId,
                   "dwThreadId differs: %lx (was %lx)\n", ctx.ev.dwThreadId, de_a.dwThreadId);

                ret = CloseHandle(thread_a);
                ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());
                thread_a = NULL;

                next_event(&ctx, POLL_EVENT_TIMEOUT);
            }

            ok(de_b.dwDebugEventCode == ctx.ev.dwDebugEventCode,
               "dwDebugEventCode differs: %lx (was %lx)\n", ctx.ev.dwDebugEventCode, de_b.dwDebugEventCode);
            ok(de_b.dwProcessId == ctx.ev.dwProcessId,
               "dwProcessId differs: %lx (was %lx)\n", ctx.ev.dwProcessId, de_b.dwProcessId);
            ok(de_b.dwThreadId == ctx.ev.dwThreadId,
               "dwThreadId differs: %lx (was %lx)\n", ctx.ev.dwThreadId, de_b.dwThreadId);
        }
        else
        {
            ok(de_b.dwDebugEventCode == ctx.ev.dwDebugEventCode,
               "dwDebugEventCode differs: %lx (was %lx)\n", ctx.ev.dwDebugEventCode, de_b.dwDebugEventCode);
            ok(de_b.dwProcessId == ctx.ev.dwProcessId,
               "dwProcessId differs: %lx (was %lx)\n", ctx.ev.dwProcessId, de_b.dwProcessId);
            ok(de_b.dwThreadId == ctx.ev.dwThreadId,
               "dwThreadId differs: %lx (was %lx)\n", ctx.ev.dwThreadId, de_b.dwThreadId);

            next_event(&ctx, POLL_EVENT_TIMEOUT);
            if (ctx.ev.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT)
            {
                ok(de_b.dwProcessId == ctx.ev.dwProcessId,
                   "dwProcessId differs: %lx (was %lx)\n", ctx.ev.dwProcessId, de_b.dwProcessId);
                ok(de_b.dwThreadId == ctx.ev.dwThreadId,
                   "dwThreadId differs: %lx (was %lx)\n", ctx.ev.dwThreadId, de_b.dwThreadId);

                ret = CloseHandle(thread_b);
                ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());
                thread_b = NULL;

                next_event(&ctx, POLL_EVENT_TIMEOUT);
            }

            ok(de_a.dwDebugEventCode == ctx.ev.dwDebugEventCode,
               "dwDebugEventCode differs: %lx (was %lx)\n", ctx.ev.dwDebugEventCode, de_a.dwDebugEventCode);
            ok(de_a.dwProcessId == ctx.ev.dwProcessId,
               "dwProcessId differs: %lx (was %lx)\n", ctx.ev.dwProcessId, de_a.dwProcessId);
            ok(de_a.dwThreadId == ctx.ev.dwThreadId,
               "dwThreadId differs: %lx (was %lx)\n", ctx.ev.dwThreadId, de_a.dwThreadId);
        }

        if (thread_a)
        {
            next_event(&ctx, POLL_EVENT_TIMEOUT);
            ok(ctx.ev.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT,
               "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);

            ret = CloseHandle(thread_a);
            ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());
        }


        if (thread_b)
        {
            next_event(&ctx, POLL_EVENT_TIMEOUT);
            ok(ctx.ev.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT,
               "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);

            ret = CloseHandle(thread_b);
            ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());
        }
    }

    if (sizeof(loop_code) > 1)
    {
        unsigned event_order = 0;

        memset(buf, OP_BP, sizeof(buf));
        memcpy(proc_code, &loop_code, sizeof(loop_code));
        ret = WriteProcessMemory(pi.hProcess, mem, buf, sizeof(buf), NULL);
        ok(ret, "WriteProcessMemory failed: %lu\n", GetLastError());

        ctx.thread_tag = 1;

        worker_cnt = 20;
        for (i = 0; i < worker_cnt; i++)
        {
            DWORD tid;
            thread = CreateRemoteThread(pi.hProcess, NULL, 0, (void*)thread_proc, NULL, 0, &tid);
            ok(thread != NULL, "CreateRemoteThread failed: %lu\n", GetLastError());

            do
            {
                next_event(&ctx, WAIT_EVENT_TIMEOUT);
                ok(ctx.ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
            } while (ctx.ev.dwDebugEventCode == CREATE_THREAD_DEBUG_EVENT && ctx.ev.dwThreadId != tid);
            ok(ctx.ev.u.CreateThread.lpStartAddress == (void*)thread_proc, "Unexpected thread's start address\n");

            ret = CloseHandle(thread);
            ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());
        }

        byte = OP_BP;
        ret = WriteProcessMemory(pi.hProcess, thread_proc + 1, &byte, 1, NULL);
        ok(ret, "WriteProcessMemory failed: %lu\n", GetLastError());

        wait_for_breakpoint(&ctx);
        check_breakpoint_exception(&ctx, thread_proc + 1);
        exception_cnt = 1;

        byte = 0xc3; /* ret */
        ret = WriteProcessMemory(pi.hProcess, thread_proc + 1, &byte, 1, NULL);
        ok(ret, "WriteProcessMemory failed: %lu\n", GetLastError());

        /* One would expect that we get all exception debug events (for the worker threads
         * that hit the BP instruction), then the exit thread events for all created threads.
         * It happens that on Windows, the exception & exit thread events can be intertwined.
         * So detect this situation.
         */
        for (;;)
        {
            DEBUG_EVENT ev;

            fetch_thread_context(ctx.current_thread);
            ok(get_ip(&ctx.current_thread->ctx) == thread_proc + 2
               || broken(get_ip(&ctx.current_thread->ctx) == thread_proc), /* sometimes observed on win10 */
               "unexpected instruction pointer2 %p (%p)\n", get_ip(&ctx.current_thread->ctx), thread_proc);
            /* even when there are more pending events, they are not reported until current event is continued */
            ret = WaitForDebugEvent(&ev, 10);
            ok(GetLastError() == ERROR_SEM_TIMEOUT, "WaitForDebugEvent returned %x(%lu)\n", ret, GetLastError());

            for (;;)
            {
                next_event_filter(&ctx, POLL_EVENT_TIMEOUT, event_mask(CREATE_THREAD_DEBUG_EVENT));
                if (ctx.ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) break;
                if (ctx.ev.dwDebugEventCode == EXIT_THREAD_DEBUG_EVENT)
                {
                    if (event_order == 0) event_order = 1; /* first exit thread event */
                    if (!--worker_cnt) break;
                }
            }
            if (!worker_cnt) break;

            ok(ctx.ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
            trace("exception at %p in thread %04lx\n", ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress, ctx.ev.dwThreadId);
            ok(ctx.ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT, "ExceptionCode = %lx\n",
               ctx.ev.u.Exception.ExceptionRecord.ExceptionCode);
            ok(ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress == thread_proc + 1,
               "ExceptionAddress = %p\n", ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress);
            exception_cnt++;
            if (event_order == 1) event_order = 2; /* exception debug event after exit thread event */
        }

        trace("received %u exceptions\n", exception_cnt);
        ok(!worker_cnt, "Missing %u exit thread events\n", worker_cnt);
        ok(event_order == 1 || broken(event_order == 2), "Intertwined exit thread & exception debug events\n");
    }

    if (OP_BP)
    {
        CONTEXT orig_context;
        char instr, *ip;

        /* main thread sleeps inside ntdll waiting for the event. set breakpoint there and make sure
         * ntdll can handle that. */
        SuspendThread(ctx.main_thread->handle);

        fetch_thread_context(ctx.main_thread);
        ret = ReadProcessMemory(pi.hProcess, get_ip(&ctx.main_thread->ctx), &instr, 1, NULL);
        ok(ret, "ReadProcessMemory failed: %lu\n", GetLastError());

        orig_context = ctx.main_thread->ctx;
        ip = get_ip(&ctx.main_thread->ctx);

#if defined(__i386__)
        ctx.main_thread->ctx.Eax = 101;
        ctx.main_thread->ctx.Ebx = 102;
        ctx.main_thread->ctx.Ecx = 103;
        ctx.main_thread->ctx.Edx = 104;
        ctx.main_thread->ctx.Esi = 105;
        ctx.main_thread->ctx.Edi = 106;
#elif defined(__x86_64__)
        ctx.main_thread->ctx.Rax = 101;
        ctx.main_thread->ctx.Rbx = 102;
        ctx.main_thread->ctx.Rcx = 103;
        ctx.main_thread->ctx.Rdx = 104;
        ctx.main_thread->ctx.Rsi = 105;
        ctx.main_thread->ctx.Rdi = 106;
        ctx.main_thread->ctx.R8  = 107;
        ctx.main_thread->ctx.R9  = 108;
        ctx.main_thread->ctx.R10 = 109;
        ctx.main_thread->ctx.R11 = 110;
        ctx.main_thread->ctx.R12 = 111;
        ctx.main_thread->ctx.R13 = 112;
        ctx.main_thread->ctx.R14 = 113;
        ctx.main_thread->ctx.R15 = 114;
#endif
        set_thread_context(&ctx, ctx.main_thread);

        fetch_thread_context(ctx.main_thread);
#if defined(__i386__)
        /* win2k8 do not preserve eax, rcx and edx; newer versions do */
        ok(ctx.main_thread->ctx.Ebx == 102, "Ebx = %lx\n", ctx.main_thread->ctx.Ebx);
        ok(ctx.main_thread->ctx.Esi == 105, "Esi = %lx\n", ctx.main_thread->ctx.Esi);
        ok(ctx.main_thread->ctx.Edi == 106, "Edi = %lx\n", ctx.main_thread->ctx.Edi);
#elif defined(__x86_64__)
        ok(ctx.main_thread->ctx.Rax == 101, "Rax = %I64x\n", ctx.main_thread->ctx.Rax);
        ok(ctx.main_thread->ctx.Rbx == 102, "Rbx = %I64x\n", ctx.main_thread->ctx.Rbx);
        ok(ctx.main_thread->ctx.Rcx == 103, "Rcx = %I64x\n", ctx.main_thread->ctx.Rcx);
        ok(ctx.main_thread->ctx.Rdx == 104, "Rdx = %I64x\n", ctx.main_thread->ctx.Rdx);
        ok(ctx.main_thread->ctx.Rsi == 105, "Rsi = %I64x\n", ctx.main_thread->ctx.Rsi);
        ok(ctx.main_thread->ctx.Rdi == 106, "Rdi = %I64x\n", ctx.main_thread->ctx.Rdi);
        ok(ctx.main_thread->ctx.R8  == 107, "R8 = %I64x\n",  ctx.main_thread->ctx.R8);
        ok(ctx.main_thread->ctx.R9  == 108, "R9 = %I64x\n",  ctx.main_thread->ctx.R9);
        ok(ctx.main_thread->ctx.R10 == 109, "R10 = %I64x\n", ctx.main_thread->ctx.R10);
        ok(ctx.main_thread->ctx.R11 == 110, "R11 = %I64x\n", ctx.main_thread->ctx.R11);
        ok(ctx.main_thread->ctx.R12 == 111, "R12 = %I64x\n", ctx.main_thread->ctx.R12);
        ok(ctx.main_thread->ctx.R13 == 112, "R13 = %I64x\n", ctx.main_thread->ctx.R13);
        ok(ctx.main_thread->ctx.R14 == 113, "R14 = %I64x\n", ctx.main_thread->ctx.R14);
        ok(ctx.main_thread->ctx.R15 == 114, "R15 = %I64x\n", ctx.main_thread->ctx.R15);
#endif

        byte = OP_BP;
        ret = WriteProcessMemory(pi.hProcess, ip, &byte, 1, NULL);
        ok(ret, "WriteProcessMemory failed: %lu\n", GetLastError());

        SetEvent(event);
        ResumeThread(ctx.main_thread->handle);

        next_event_filter(&ctx, 2000, event_mask(CREATE_THREAD_DEBUG_EVENT));
        ok(ctx.ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
        ok(ctx.ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_BREAKPOINT, "ExceptionCode = %lx\n",
           ctx.ev.u.Exception.ExceptionRecord.ExceptionCode);
        ok(ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress == ip,
           "ExceptionAddress = %p\n", ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress);

        fetch_thread_context(ctx.main_thread);
        ok(get_ip(&ctx.main_thread->ctx) == ip + 1, "unexpected instruction pointer %p\n", get_ip(&ctx.main_thread->ctx));

#if defined(__i386__)
        ok(ctx.main_thread->ctx.Eax == 0,   "Eax = %lx\n", ctx.main_thread->ctx.Eax);
        ok(ctx.main_thread->ctx.Ebx == 102, "Ebx = %lx\n", ctx.main_thread->ctx.Ebx);
        ok(ctx.main_thread->ctx.Ecx != 103, "Ecx = %lx\n", ctx.main_thread->ctx.Ecx);
        ok(ctx.main_thread->ctx.Edx != 104, "Edx = %lx\n", ctx.main_thread->ctx.Edx);
        ok(ctx.main_thread->ctx.Esi == 105, "Esi = %lx\n", ctx.main_thread->ctx.Esi);
        ok(ctx.main_thread->ctx.Edi == 106, "Edi = %lx\n", ctx.main_thread->ctx.Edi);
#elif defined(__x86_64__)
        ok(ctx.main_thread->ctx.Rax == 0,   "Rax = %I64x\n", ctx.main_thread->ctx.Rax);
        ok(ctx.main_thread->ctx.Rbx == 102, "Rbx = %I64x\n", ctx.main_thread->ctx.Rbx);
        ok(ctx.main_thread->ctx.Rcx != 103, "Rcx = %I64x\n", ctx.main_thread->ctx.Rcx);
        ok(ctx.main_thread->ctx.Rdx != 104, "Rdx = %I64x\n", ctx.main_thread->ctx.Rdx);
        ok(ctx.main_thread->ctx.Rsi == 105, "Rsi = %I64x\n", ctx.main_thread->ctx.Rsi);
        ok(ctx.main_thread->ctx.Rdi == 106, "Rdi = %I64x\n", ctx.main_thread->ctx.Rdi);
        ok(ctx.main_thread->ctx.R8  != 107, "R8 = %I64x\n",  ctx.main_thread->ctx.R8);
        ok(ctx.main_thread->ctx.R9  != 108, "R9 = %I64x\n",  ctx.main_thread->ctx.R9);
        ok(ctx.main_thread->ctx.R10 != 109, "R10 = %I64x\n", ctx.main_thread->ctx.R10);
        ok(ctx.main_thread->ctx.R11 != 110, "R11 = %I64x\n", ctx.main_thread->ctx.R11);
        ok(ctx.main_thread->ctx.R12 == 111, "R12 = %I64x\n", ctx.main_thread->ctx.R12);
        ok(ctx.main_thread->ctx.R13 == 112, "R13 = %I64x\n", ctx.main_thread->ctx.R13);
        ok(ctx.main_thread->ctx.R14 == 113, "R14 = %I64x\n", ctx.main_thread->ctx.R14);
        ok(ctx.main_thread->ctx.R15 == 114, "R15 = %I64x\n", ctx.main_thread->ctx.R15);
#endif

        ctx.main_thread->ctx = orig_context;
        set_ip(&ctx.main_thread->ctx, ip);
        set_thread_context(&ctx, ctx.main_thread);

        ret = WriteProcessMemory(pi.hProcess, ip, &instr, 1, NULL);
        ok(ret, "WriteProcessMemory failed: %lu\n", GetLastError());

        memset(buf + 10, 0x90, 10); /* nop */
        ret = WriteProcessMemory(pi.hProcess, mem + 10, buf + 10, 10, NULL);
        ok(ret, "WriteProcessMemory failed: %lu\n", GetLastError());

        next_event(&ctx, POLL_EVENT_TIMEOUT);

        /* try single step while debuggee is in a syscall */
        fetch_thread_context(ctx.main_thread);
        orig_context = ctx.main_thread->ctx;
        ip = get_ip(&ctx.main_thread->ctx);

#if defined(__i386__)
        ctx.main_thread->ctx.EFlags |= 0x100;
        ctx.main_thread->ctx.Eip = (ULONG_PTR)mem + 10;
#elif defined(__x86_64__)
        ctx.main_thread->ctx.EFlags |= 0x100;
        ctx.main_thread->ctx.Rip = (ULONG64)mem + 10;
#endif
        set_thread_context(&ctx, ctx.main_thread);

        SetEvent(event);

        next_event(&ctx, WAIT_EVENT_TIMEOUT);
        if (sizeof(void*) != 4 || ctx.ev.u.Exception.ExceptionRecord.ExceptionCode != EXCEPTION_BREAKPOINT)
        {
            ok(ctx.ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT, "dwDebugEventCode = %ld\n", ctx.ev.dwDebugEventCode);
            ok(ctx.ev.u.Exception.ExceptionRecord.ExceptionCode == EXCEPTION_SINGLE_STEP, "ExceptionCode = %lx\n",
               ctx.ev.u.Exception.ExceptionRecord.ExceptionCode);
            ok(ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress == mem + 10 ||
               ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress == mem + 11,
               "ExceptionAddress = %p expected %p\n", ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress, mem + 10);

            fetch_thread_context(ctx.main_thread);
            ok(get_ip(&ctx.main_thread->ctx) == ctx.ev.u.Exception.ExceptionRecord.ExceptionAddress,
               "ip = %p\n", get_ip(&ctx.main_thread->ctx));

        }
        else win_skip("got breakpoint instead of single step exception\n");

        ctx.main_thread->ctx = orig_context;
        set_thread_context(&ctx, ctx.main_thread);
    }

    SetEvent(event);

    do
    {
        next_event(&ctx, WAIT_EVENT_TIMEOUT);
        ok (ctx.ev.dwDebugEventCode != EXCEPTION_DEBUG_EVENT, "got exception\n");
        if (ctx.ev.dwDebugEventCode == EXCEPTION_DEBUG_EVENT) break;
    }
    while (ctx.ev.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT);
    if (ctx.ev.dwDebugEventCode != EXIT_PROCESS_DEBUG_EVENT) TerminateProcess(pi.hProcess, 0);

    ret = CloseHandle(event);
    ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());
    ret = CloseHandle(pi.hThread);
    ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());
    ret = CloseHandle(pi.hProcess);
    ok(ret, "CloseHandle failed, last error %ld.\n", GetLastError());
}

static DWORD run_child_wait( char *cmd, HANDLE event )
{
    PROCESS_INFORMATION pi;
    STARTUPINFOA si = { sizeof(si) };
    BOOL ret;
    DWORD exit_code;

    ret = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed, last error %#lx.\n", GetLastError());
    Sleep(200);
    CloseHandle( pDbgUiGetThreadDebugObject() );
    pDbgUiSetThreadDebugObject( 0 );
    SetEvent( event );
    WaitForSingleObject( pi.hProcess, 1000 );
    ret = GetExitCodeProcess( pi.hProcess, &exit_code );
    ok( ret, "GetExitCodeProcess failed err=%ld\n", GetLastError());
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    return exit_code;
}

static PROCESS_INFORMATION pi;
static char *cmd;

static DWORD WINAPI debug_and_exit(void *arg)
{
    STARTUPINFOA si = { sizeof(si) };
    HANDLE debug;
    ULONG val = 0;
    NTSTATUS status;
    BOOL ret;

    ret = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed, last error %#lx.\n", GetLastError());
    debug = pDbgUiGetThreadDebugObject();
    status = pNtSetInformationDebugObject( debug, DebugObjectKillProcessOnExitInformation,
                                           &val, sizeof(val), NULL );
    ok( !status, "NtSetInformationDebugObject failed %lx\n", status );
    *(HANDLE *)arg = debug;
    Sleep(200);
    ExitThread(0);
}

static DWORD WINAPI debug_and_wait(void *arg)
{
    STARTUPINFOA si = { sizeof(si) };
    HANDLE debug = *(HANDLE *)arg;
    ULONG val = 0;
    NTSTATUS status;
    BOOL ret;

    pDbgUiSetThreadDebugObject( debug );
    ret = CreateProcessA(NULL, cmd, NULL, NULL, TRUE, DEBUG_PROCESS, NULL, NULL, &si, &pi);
    ok(ret, "CreateProcess failed, last error %#lx.\n", GetLastError());
    debug = pDbgUiGetThreadDebugObject();
    status = pNtSetInformationDebugObject( debug, DebugObjectKillProcessOnExitInformation,
                                           &val, sizeof(val), NULL );
    ok( !status, "NtSetInformationDebugObject failed %lx\n", status );
    Sleep(INFINITE);
    ExitThread(0);
}

static DWORD WINAPI create_debug_port(void *arg)
{
    STARTUPINFOA si = { sizeof(si) };
    NTSTATUS status = pDbgUiConnectToDbg();

    ok( !status, "DbgUiConnectToDbg failed %lx\n", status );
    *(HANDLE *)arg = pDbgUiGetThreadDebugObject();
    Sleep( INFINITE );
    ExitThread(0);
}

static void test_kill_on_exit(const char *argv0)
{
    static const char arguments[] = " debugger wait ";
    SECURITY_ATTRIBUTES sa = { sizeof(sa), NULL, TRUE };
    OBJECT_ATTRIBUTES attr = { sizeof(attr) };
    NTSTATUS status;
    HANDLE event, debug, thread;
    DWORD exit_code, tid;
    ULONG val;
    BOOL ret;

    event = CreateEventW(&sa, FALSE, FALSE, NULL);
    ok(event != NULL, "CreateEvent failed: %lu\n", GetLastError());

    cmd = heap_alloc(strlen(argv0) + strlen(arguments) + 16);
    sprintf(cmd, "%s%s%lx\n", argv0, arguments, (DWORD)(DWORD_PTR)event);

    status = pNtCreateDebugObject( &debug, DEBUG_ALL_ACCESS, &attr, 0 );
    ok( !status, "NtCreateDebugObject failed %lx\n", status );
    pDbgUiSetThreadDebugObject( debug );
    exit_code = run_child_wait( cmd, event );
    ok( exit_code == 0, "exit code = %08lx\n", exit_code);

    status = pNtCreateDebugObject( &debug, DEBUG_ALL_ACCESS, &attr, DEBUG_KILL_ON_CLOSE );
    ok( !status, "NtCreateDebugObject failed %lx\n", status );
    pDbgUiSetThreadDebugObject( debug );
    exit_code = run_child_wait( cmd, event );
    ok( exit_code == STATUS_DEBUGGER_INACTIVE, "exit code = %08lx\n", exit_code);

    status = pNtCreateDebugObject( &debug, DEBUG_ALL_ACCESS, &attr, 0xfffe );
    ok( status == STATUS_INVALID_PARAMETER, "NtCreateDebugObject failed %lx\n", status );

    status = pNtCreateDebugObject( &debug, DEBUG_ALL_ACCESS, &attr, 0 );
    ok( !status, "NtCreateDebugObject failed %lx\n", status );
    pDbgUiSetThreadDebugObject( debug );
    val = DEBUG_KILL_ON_CLOSE;
    status = pNtSetInformationDebugObject( debug, DebugObjectKillProcessOnExitInformation,
                                           &val, sizeof(val), NULL );
    ok( !status, "NtSetInformationDebugObject failed %lx\n", status );
    exit_code = run_child_wait( cmd, event );
    ok( exit_code == STATUS_DEBUGGER_INACTIVE, "exit code = %08lx\n", exit_code);

    status = pNtCreateDebugObject( &debug, DEBUG_ALL_ACCESS, &attr, DEBUG_KILL_ON_CLOSE );
    ok( !status, "NtCreateDebugObject failed %lx\n", status );
    pDbgUiSetThreadDebugObject( debug );
    val = 0;
    status = pNtSetInformationDebugObject( debug, DebugObjectKillProcessOnExitInformation,
                                           &val, sizeof(val), NULL );
    ok( !status, "NtSetInformationDebugObject failed %lx\n", status );
    exit_code = run_child_wait( cmd, event );
    ok( exit_code == 0, "exit code = %08lx\n", exit_code);

    status = pDbgUiConnectToDbg();
    ok( !status, "DbgUiConnectToDbg failed %lx\n", status );
    exit_code = run_child_wait( cmd, event );
    ok( exit_code == STATUS_DEBUGGER_INACTIVE, "exit code = %08lx\n", exit_code);

    /* test that threads close the debug port on exit */
    thread = CreateThread(NULL, 0, debug_and_exit, &debug, 0, &tid);
    WaitForSingleObject( thread, 1000 );
    ok( debug != 0, "no debug port\n" );
    val = 0;
    status = pNtSetInformationDebugObject( debug, DebugObjectKillProcessOnExitInformation,
                                           &val, sizeof(val), NULL );
    ok( status == STATUS_INVALID_HANDLE || broken(status == STATUS_SUCCESS),  /* wow64 */
        "NtSetInformationDebugObject failed %lx\n", status );
    SetEvent( event );
    if (!status)
    {
        WaitForSingleObject( pi.hProcess, 100 );
        GetExitCodeProcess( pi.hProcess, &exit_code );
        ok( exit_code == STILL_ACTIVE, "exit code = %08lx\n", exit_code);
        CloseHandle( debug );
    }
    WaitForSingleObject( pi.hProcess, 1000 );
    GetExitCodeProcess( pi.hProcess, &exit_code );
    ok( exit_code == 0, "exit code = %08lx\n", exit_code);
    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    CloseHandle( thread );

    /* checking on forced exit */
    status = pNtCreateDebugObject( &debug, DEBUG_ALL_ACCESS, &attr, DEBUG_KILL_ON_CLOSE );
    ok( !status, "NtCreateDebugObject failed %lx\n", status );
    thread = CreateThread(NULL, 0, debug_and_wait, &debug, 0, &tid);
    Sleep( 100 );
    ok( debug != 0, "no debug port\n" );
    val = DEBUG_KILL_ON_CLOSE;
    status = pNtSetInformationDebugObject( debug, DebugObjectKillProcessOnExitInformation,
                                           &val, sizeof(val), NULL );
    ok( status == STATUS_SUCCESS, "NtSetInformationDebugObject failed %lx\n", status );
    TerminateThread( thread, 0 );

    status = WaitForSingleObject( pi.hProcess, 1500 );
    if (status != WAIT_OBJECT_0)
    {
        todo_wine /* Wine doesn't handle debug port of TerminateThread */
        ok(broken(sizeof(void*) == sizeof(int)), /* happens consistently on 32bit on Win7, 10 & 11 */
           "Terminating thread should terminate debuggee\n");

        ret = TerminateProcess( pi.hProcess, 0 );
        ok(ret, "TerminateProcess failed: %lu\n", GetLastError());
        CloseHandle( debug );
    }
    else
    {
        ok(status == WAIT_OBJECT_0, "debuggee didn't terminate %lx\n", status);
        ret = GetExitCodeProcess( pi.hProcess, &exit_code );
        ok(ret, "No exit code: %lu\n", GetLastError());
        todo_wine
        ok( exit_code == STATUS_DEBUGGER_INACTIVE || broken(exit_code == STILL_ACTIVE), /* wow64 */
            "exit code = %08lx\n", exit_code);
        status = pNtSetInformationDebugObject( debug, DebugObjectKillProcessOnExitInformation,
                                               &val, sizeof(val), NULL );
        todo_wine
        ok( status == STATUS_INVALID_HANDLE, "NtSetInformationDebugObject failed %lx\n", status );
    }

    CloseHandle( pi.hProcess );
    CloseHandle( pi.hThread );
    CloseHandle( thread );

    debug = 0;
    thread = CreateThread(NULL, 0, create_debug_port, &debug, 0, &tid);
    Sleep(100);
    ok( debug != 0, "no debug port\n" );
    val = 0;
    status = pNtSetInformationDebugObject( debug, DebugObjectKillProcessOnExitInformation,
                                           &val, sizeof(val), NULL );
    ok( status == STATUS_SUCCESS, "NtSetInformationDebugObject failed %lx\n", status );
    TerminateThread( thread, 0 );
    Sleep( 200 );
    status = pNtSetInformationDebugObject( debug, DebugObjectKillProcessOnExitInformation,
                                           &val, sizeof(val), NULL );
    todo_wine
    ok( status == STATUS_INVALID_HANDLE  || broken( status == STATUS_SUCCESS ),
        "NtSetInformationDebugObject failed %lx\n", status );
    if (status != STATUS_INVALID_HANDLE) CloseHandle( debug );
    CloseHandle( thread );

    CloseHandle( event );
    heap_free(cmd);
}

START_TEST(debugger)
{
    HMODULE hdll;

    hdll=GetModuleHandleA("kernel32.dll");
    pCheckRemoteDebuggerPresent=(void*)GetProcAddress(hdll, "CheckRemoteDebuggerPresent");
    pIsWow64Process=(void*)GetProcAddress(hdll, "IsWow64Process");
    pGetMappedFileNameW = (void*)GetProcAddress(hdll, "GetMappedFileNameW");
    if (!pGetMappedFileNameW) pGetMappedFileNameW = (void*)GetProcAddress(LoadLibraryA("psapi.dll"),
                                                                          "GetMappedFileNameW");
    ntdll = GetModuleHandleA("ntdll.dll");
    pDbgBreakPoint = (void*)GetProcAddress(ntdll, "DbgBreakPoint");
    pNtSuspendProcess = (void*)GetProcAddress(ntdll, "NtSuspendProcess");
    pNtResumeProcess = (void*)GetProcAddress(ntdll, "NtResumeProcess");
    pNtCreateDebugObject = (void*)GetProcAddress(ntdll, "NtCreateDebugObject");
    pNtSetInformationDebugObject = (void*)GetProcAddress(ntdll, "NtSetInformationDebugObject");
    pDbgUiConnectToDbg = (void*)GetProcAddress(ntdll, "DbgUiConnectToDbg");
    pDbgUiGetThreadDebugObject = (void*)GetProcAddress(ntdll, "DbgUiGetThreadDebugObject");
    pDbgUiSetThreadDebugObject = (void*)GetProcAddress(ntdll, "DbgUiSetThreadDebugObject");

#ifdef __arm__
    /* mask thumb bit for address comparisons */
    pDbgBreakPoint = (void *)((ULONG_PTR)pDbgBreakPoint & ~1);
#endif

    if (pIsWow64Process) pIsWow64Process( GetCurrentProcess(), &is_wow64 );

    myARGC=winetest_get_mainargs(&myARGV);
    if (myARGC >= 3 && strcmp(myARGV[2], "crash") == 0)
    {
        doCrash();
    }
    else if (myARGC >= 3 && strncmp(myARGV[2], "dbg,", 4) == 0)
    {
        doDebugger(myARGC, myARGV);
    }
    else if (myARGC >= 5 && !strcmp(myARGV[2], "child"))
    {
        doChild(myARGC, myARGV);
    }
    else if (myARGC >= 4 && !strcmp(myARGV[2], "children"))
    {
        doChildren(myARGC, myARGV);
    }
    else if (myARGC >= 4 && !strcmp(myARGV[2], "wait"))
    {
        DWORD event, cnt = 1;
        sscanf(myARGV[3], "%lx", &event);
        if (myARGC >= 5) cnt = atoi(myARGV[4]);
        wait_debugger((HANDLE)(DWORD_PTR)event, cnt);
    }
    else
    {
        test_ExitCode();
        test_RemoteDebugger();
        test_debug_loop(myARGC, myARGV);
        test_debug_loop_wow64();
        test_debug_children(myARGV[0], DEBUG_PROCESS, TRUE, FALSE);
        test_debug_children(myARGV[0], DEBUG_ONLY_THIS_PROCESS, FALSE, FALSE);
        test_debug_children(myARGV[0], DEBUG_PROCESS|DEBUG_ONLY_THIS_PROCESS, FALSE, FALSE);
        test_debug_children(myARGV[0], 0, FALSE, FALSE);
        test_debug_children(myARGV[0], 0, FALSE, TRUE);
        test_debug_children(myARGV[0], DEBUG_ONLY_THIS_PROCESS, FALSE, TRUE);
        test_debugger(myARGV[0]);
        test_kill_on_exit(myARGV[0]);
    }
}
