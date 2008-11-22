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
#include <windows.h>
#include <winreg.h>
#include <ntndk.h>
#include "wine/test.h"

#ifndef STATUS_DEBUGGER_INACTIVE
#define STATUS_DEBUGGER_INACTIVE         ((NTSTATUS) 0xC0000354)
#endif

static int    myARGC;
static char** myARGV;

static BOOL (WINAPI *pDebugActiveProcessStop)(DWORD);
static BOOL (WINAPI *pDebugSetProcessKillOnExit)(BOOL);

/* Copied from the process test */
static void get_file_name(char* buf)
{
    char path[MAX_PATH];

    buf[0] = '\0';
    GetTempPathA(sizeof(path), path);
    GetTempFileNameA(path, "wt", 0, buf);
}

static void get_events(const char* name, HANDLE *start_event, HANDLE *done_event)
{
    const char* basename;
    char* event_name;

    basename=strrchr(name, '\\');
    basename=(basename ? basename+1 : name);
    event_name=HeapAlloc(GetProcessHeap(), 0, 6+strlen(basename)+1);

    sprintf(event_name, "start_%s", basename);
    *start_event=CreateEvent(NULL, 0,0, event_name);
    sprintf(event_name, "done_%s", basename);
    *done_event=CreateEvent(NULL, 0,0, event_name);
    HeapFree(GetProcessHeap(), 0, event_name);
}

static void save_blackbox(const char* logfile, void* blackbox, int size)
{
    HANDLE hFile;
    DWORD written;

    hFile=CreateFileA(logfile, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE)
        return;
    WriteFile(hFile, blackbox, size, &written, NULL);
    CloseHandle(hFile);
}

static int load_blackbox(const char* logfile, void* blackbox, int size)
{
    HANDLE hFile;
    DWORD read;
    BOOL ret;

    hFile=CreateFileA(logfile, GENERIC_READ, 0, NULL, OPEN_EXISTING, 0, 0);
    if (hFile == INVALID_HANDLE_VALUE)
    {
        ok(0, "unable to open '%s'\n", logfile);
        return 0;
    }
    ret=ReadFile(hFile, blackbox, size, &read, NULL);
    ok(read == size, "wrong size for '%s': read=%d\n", logfile, read);
    CloseHandle(hFile);
    return 1;
}

typedef struct
{
    DWORD pid;
} crash_blackbox_t;

static void doCrash(int argc,  char** argv)
{
    char* p;

    if (argc >= 4)
    {
        crash_blackbox_t blackbox;
        blackbox.pid=GetCurrentProcessId();
        save_blackbox(argv[3], &blackbox, sizeof(blackbox));
    }

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
} debugger_blackbox_t;

static void doDebugger(int argc, char** argv)
{
    const char* logfile;
    debugger_blackbox_t blackbox;
    HANDLE start_event, done_event, debug_event;

    blackbox.argc=argc;
    logfile=(argc >= 4 ? argv[3] : NULL);
    blackbox.pid=(argc >= 5 ? atol(argv[4]) : 0);

    if (strstr(myARGV[2], "attach"))
    {
        blackbox.attach_rc=DebugActiveProcess(blackbox.pid);
        if (!blackbox.attach_rc)
            blackbox.attach_err=GetLastError();
    }
    else
        blackbox.attach_rc=TRUE;

    debug_event=(argc >= 6 ? (HANDLE)atol(argv[5]) : NULL);
    if (debug_event && strstr(myARGV[2], "event"))
    {
        blackbox.debug_rc=SetEvent(debug_event);
        if (!blackbox.debug_rc)
            blackbox.debug_err=GetLastError();
    }
    else
        blackbox.debug_rc=TRUE;

    get_events(logfile, &start_event, &done_event);
    if (strstr(myARGV[2], "order"))
    {
        trace("debugger: waiting for the start signal...\n");
        WaitForSingleObject(start_event, INFINITE);
    }

    if (strstr(myARGV[2], "nokill"))
    {
        blackbox.nokill_rc=pDebugSetProcessKillOnExit(FALSE);
        if (!blackbox.nokill_rc)
            blackbox.nokill_err=GetLastError();
    }
    else
        blackbox.nokill_rc=TRUE;

    if (strstr(myARGV[2], "detach"))
    {
        blackbox.detach_rc=pDebugActiveProcessStop(blackbox.pid);
        if (!blackbox.detach_rc)
            blackbox.detach_err=GetLastError();
    }
    else
        blackbox.detach_rc=TRUE;

    save_blackbox(logfile, &blackbox, sizeof(blackbox));
    trace("debugger: done debugging...\n");
    SetEvent(done_event);

    /* Just exit with a known value */
    ExitProcess(0xdeadbeef);
}

static void crash_and_debug(HKEY hkey, const char* argv0, const char* dbgtasks)
{
    DWORD ret;
    HANDLE start_event, done_event;
    char* cmd;
    char dbglog[MAX_PATH];
    char childlog[MAX_PATH];
    PROCESS_INFORMATION	info;
    STARTUPINFOA startup;
    DWORD exit_code;
    crash_blackbox_t crash_blackbox;
    debugger_blackbox_t dbg_blackbox;

    ret=RegSetValueExA(hkey, "auto", 0, REG_SZ, (BYTE*)"1", 2);
    ok(ret == ERROR_SUCCESS, "unable to set AeDebug/auto: ret=%d\n", ret);

    get_file_name(dbglog);
    get_events(dbglog, &start_event, &done_event);
    cmd=HeapAlloc(GetProcessHeap(), 0, strlen(argv0)+10+strlen(dbgtasks)+1+strlen(dbglog)+34+1);
    sprintf(cmd, "%s debugger %s %s %%ld %%ld", argv0, dbgtasks, dbglog);
    ret=RegSetValueExA(hkey, "debugger", 0, REG_SZ, (BYTE*)cmd, strlen(cmd)+1);
    ok(ret == ERROR_SUCCESS, "unable to set AeDebug/debugger: ret=%d\n", ret);
    HeapFree(GetProcessHeap(), 0, cmd);

    get_file_name(childlog);
    cmd=HeapAlloc(GetProcessHeap(), 0, strlen(argv0)+16+strlen(dbglog)+1);
    sprintf(cmd, "%s debugger crash %s", argv0, childlog);

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    ret=CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess: err=%d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, cmd);
    CloseHandle(info.hThread);

    /* The process exits... */
    trace("waiting for child exit...\n");
    ok(WaitForSingleObject(info.hProcess, 60000) == WAIT_OBJECT_0, "Timed out waiting for the child to crash\n");
    ok(GetExitCodeProcess(info.hProcess, &exit_code), "GetExitCodeProcess failed: err=%d\n", GetLastError());
    if (strstr(dbgtasks, "code2"))
    {
        /* If, after attaching to the debuggee, the debugger exits without
         * detaching, then the debuggee gets a special exit code.
         */
        ok(exit_code == 0xffffffff || /* Win 9x */
           exit_code == 0x80 || /* NT4 */
           exit_code == STATUS_DEBUGGER_INACTIVE, /* Win >= XP */
           "wrong exit code : %08x\n", exit_code);
    }
    else
        ok(exit_code == STATUS_ACCESS_VIOLATION, "exit code = %08x instead of STATUS_ACCESS_VIOLATION\n", exit_code);
    CloseHandle(info.hProcess);

    /* ...before the debugger */
    if (strstr(dbgtasks, "order"))
        ok(SetEvent(start_event), "SetEvent(start_event) failed\n");

    trace("waiting for the debugger...\n");
    ok(WaitForSingleObject(done_event, 60000) == WAIT_OBJECT_0, "Timed out waiting for the debugger\n");

    assert(load_blackbox(childlog, &crash_blackbox, sizeof(crash_blackbox)));
    assert(load_blackbox(dbglog, &dbg_blackbox, sizeof(dbg_blackbox)));

    ok(dbg_blackbox.argc == 6, "wrong debugger argument count: %d\n", dbg_blackbox.argc);
    ok(dbg_blackbox.pid == crash_blackbox.pid, "the child and debugged pids don't match: %d != %d\n", crash_blackbox.pid, dbg_blackbox.pid);
    ok(dbg_blackbox.debug_rc, "debugger: SetEvent(debug_event) failed err=%d\n", dbg_blackbox.debug_err);
    ok(dbg_blackbox.attach_rc, "DebugActiveProcess(%d) failed err=%d\n", dbg_blackbox.pid, dbg_blackbox.attach_err);
    ok(dbg_blackbox.nokill_rc, "DebugSetProcessKillOnExit(FALSE) failed err=%d\n", dbg_blackbox.nokill_err);
    ok(dbg_blackbox.detach_rc, "DebugActiveProcessStop(%d) failed err=%d\n", dbg_blackbox.pid, dbg_blackbox.detach_err);

    assert(DeleteFileA(dbglog) != 0);
    assert(DeleteFileA(childlog) != 0);
}

static void crash_and_winedbg(HKEY hkey, const char* argv0)
{
    DWORD ret;
    char* cmd;
    PROCESS_INFORMATION	info;
    STARTUPINFOA startup;
    DWORD exit_code;

    ret=RegSetValueExA(hkey, "auto", 0, REG_SZ, (BYTE*)"1", 2);
    ok(ret == ERROR_SUCCESS, "unable to set AeDebug/auto: ret=%d\n", ret);

    cmd=HeapAlloc(GetProcessHeap(), 0, strlen(argv0)+15+1);
    sprintf(cmd, "%s debugger crash", argv0);

    memset(&startup, 0, sizeof(startup));
    startup.cb = sizeof(startup);
    startup.dwFlags = STARTF_USESHOWWINDOW;
    startup.wShowWindow = SW_SHOWNORMAL;
    ret=CreateProcessA(NULL, cmd, NULL, NULL, FALSE, 0, NULL, NULL, &startup, &info);
    ok(ret, "CreateProcess: err=%d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, cmd);
    CloseHandle(info.hThread);

    trace("waiting for child exit...\n");
    ok(WaitForSingleObject(info.hProcess, 60000) == WAIT_OBJECT_0, "Timed out waiting for the child to crash\n");
    ok(GetExitCodeProcess(info.hProcess, &exit_code), "GetExitCodeProcess failed: err=%d\n", GetLastError());
    ok(exit_code == STATUS_ACCESS_VIOLATION, "exit code = %08x\n", exit_code);
    CloseHandle(info.hProcess);
}

static void test_ExitCode(void)
{
    static const char* AeDebug="Software\\Microsoft\\Windows NT\\CurrentVersion\\AeDebug";
    char test_exe[MAX_PATH];
    DWORD ret;
    HKEY hkey;
    DWORD disposition;
    LPBYTE auto_val=NULL;
    DWORD auto_size, auto_type;
    LPBYTE debugger_val=NULL;
    DWORD debugger_size, debugger_type;

    GetModuleFileNameA(GetModuleHandle(NULL), test_exe, sizeof(test_exe));
    if (GetFileAttributes(test_exe) == INVALID_FILE_ATTRIBUTES)
        strcat(test_exe, ".so");
    if (GetFileAttributesA(test_exe) == INVALID_FILE_ATTRIBUTES)
    {
        ok(0, "could not find the test executable '%s'\n", test_exe);
        return;
    }

    ret=RegCreateKeyExA(HKEY_LOCAL_MACHINE, AeDebug, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_ALL_ACCESS, NULL, &hkey, &disposition);
    if (ret == ERROR_SUCCESS)
    {
        auto_size=0;
        ret=RegQueryValueExA(hkey, "auto", NULL, &auto_type, NULL, &auto_size);
        if (ret == ERROR_SUCCESS)
        {
            auto_val=HeapAlloc(GetProcessHeap(), 0, auto_size);
            RegQueryValueExA(hkey, "auto", NULL, &auto_type, auto_val, &auto_size);
        }

        debugger_size=0;
        ret=RegQueryValueExA(hkey, "debugger", NULL, &debugger_type, NULL, &debugger_size);
        if (ret == ERROR_SUCCESS)
        {
            debugger_val=HeapAlloc(GetProcessHeap(), 0, debugger_size);
            RegQueryValueExA(hkey, "debugger", NULL, &debugger_type, debugger_val, &debugger_size);
        }
    }
    else if (ret == ERROR_ACCESS_DENIED)
    {
        skip("not enough privileges to change the debugger\n");
        return;
    }
    else if (ret != ERROR_FILE_NOT_FOUND)
    {
        ok(0, "could not open the AeDebug key: %d\n", ret);
        return;
    }

    if (debugger_val && debugger_type == REG_SZ &&
        strstr((char*)debugger_val, "winedbg --auto"))
        crash_and_winedbg(hkey, test_exe);

    crash_and_debug(hkey, test_exe, "dbg,none");
    crash_and_debug(hkey, test_exe, "dbg,event,order");
    crash_and_debug(hkey, test_exe, "dbg,attach,event,code2");
    if (pDebugSetProcessKillOnExit)
        crash_and_debug(hkey, test_exe, "dbg,attach,event,nokill");
    if (pDebugActiveProcessStop)
        crash_and_debug(hkey, test_exe, "dbg,attach,event,detach");

    if (disposition == REG_CREATED_NEW_KEY)
    {
        RegCloseKey(hkey);
        RegDeleteKeyA(HKEY_LOCAL_MACHINE, AeDebug);
    }
    else
    {
        if (auto_val)
        {
            RegSetValueExA(hkey, "auto", 0, auto_type, auto_val, auto_size);
            HeapFree(GetProcessHeap(), 0, auto_val);
        }
        else
            RegDeleteValueA(hkey, "auto");
        if (debugger_val)
        {
            RegSetValueExA(hkey, "debugger", 0, debugger_type, debugger_val, debugger_size);
            HeapFree(GetProcessHeap(), 0, debugger_val);
        }
        else
            RegDeleteValueA(hkey, "debugger");
        RegCloseKey(hkey);
    }
}

START_TEST(debugger)
{
    HMODULE hdll;

    hdll=GetModuleHandle("kernel32.dll");
    pDebugActiveProcessStop=(void*)GetProcAddress(hdll, "DebugActiveProcessStop");
    pDebugSetProcessKillOnExit=(void*)GetProcAddress(hdll, "DebugSetProcessKillOnExit");

    myARGC=winetest_get_mainargs(&myARGV);
    if (myARGC >= 3 && strcmp(myARGV[2], "crash") == 0)
    {
        doCrash(myARGC, myARGV);
    }
    else if (myARGC >= 3 && strncmp(myARGV[2], "dbg,", 4) == 0)
    {
        doDebugger(myARGC, myARGV);
    }
    else
    {
        test_ExitCode();
    }
}
