/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GPLv2+ - See COPYING in the top level directory
 * PURPOSE:         Test CreateProcess()
 * PROGRAMMERS:     Mark Jansen
 *                  Serge Gautherie <reactos-git_serge_171003@gautherie.fr>
 */

#include "precomp.h"

#include <ndk/rtlfuncs.h>

// Test spoiling of StaticUnicodeString by CreateProcessA().
static
void
test_CreateProcessA_StaticUnicodeString(void)
{
    PUNICODE_STRING StaticString;
    UNICODE_STRING CompareString;
    BOOL Process;
    STARTUPINFO si = {0};
    PROCESS_INFORMATION pi = {0};
    LONG Result;

    StaticString = &NtCurrentTeb()->StaticUnicodeString;
    RtlInitUnicodeString(&CompareString, L"--sentinel--");
    RtlCopyUnicodeString(StaticString, &CompareString);

    si.cb = sizeof(si);
    Process = CreateProcessA("ApplicationName", "CommandLine", NULL, NULL, FALSE, 0, NULL, "CurrentDir", &si, &pi);
    ok_int(Process, 0);

    Result = RtlCompareUnicodeString(StaticString, &CompareString, TRUE);
    ok(!Result, "Expected %s to equal %s\n",
       wine_dbgstr_wn(StaticString->Buffer, StaticString->Length / sizeof(WCHAR)),
       wine_dbgstr_wn(CompareString.Buffer, CompareString.Length / sizeof(WCHAR)));
}

static
void
test_CreateProcessW_CreationFlags(void)
{
    const HANDLE hCurrentProcess = GetCurrentProcess();
    const WCHAR wszCmdLine[] = L"cmd.exe /C rem";

    DWORD dwCurrentProcessPriorityClass, dwPriorityClass;
    WCHAR wszCmdLineBuffer[MAX_PATH];
    STARTUPINFOW si;
    PROCESS_INFORMATION pi;
    BOOL bRet;

    dwCurrentProcessPriorityClass = GetPriorityClass(hCurrentProcess);
    ok(dwCurrentProcessPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());

    // Invalid combinations.

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ok(!CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                       CREATE_NEW_CONSOLE | DETACHED_PROCESS,
                       NULL, NULL, &si, &pi),
       "CreateProcessW succeeded unexpectedly\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "LastError: %lu != ERROR_INVALID_PARAMETER\n", GetLastError());

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ok(!CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                       CREATE_SEPARATE_WOW_VDM | CREATE_SHARED_WOW_VDM,
                       NULL, NULL, &si, &pi),
       "CreateProcessW succeeded unexpectedly\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "LastError: %lu != ERROR_INVALID_PARAMETER\n", GetLastError());

    // Multiple priorities.
    // Allowed! Then, check their relative priority.

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    bRet = CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                          IDLE_PRIORITY_CLASS | BELOW_NORMAL_PRIORITY_CLASS | NORMAL_PRIORITY_CLASS | ABOVE_NORMAL_PRIORITY_CLASS | HIGH_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS,
                          NULL, NULL, &si, &pi);
    ok(bRet, "CreateProcessW failed. LastError: %lu\n", GetLastError());
    dwPriorityClass = GetPriorityClass(pi.hProcess);
    ok(dwPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());
    ok(dwPriorityClass == IDLE_PRIORITY_CLASS, "dwPriorityClass: 0x%08lx != IDLE_PRIORITY_CLASS\n", dwPriorityClass);
    bRet = TerminateProcess(pi.hProcess, 0);
    ok(bRet, "TerminateProcess failed. LastError: %lu\n", GetLastError());
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    bRet = CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                          BELOW_NORMAL_PRIORITY_CLASS | NORMAL_PRIORITY_CLASS | ABOVE_NORMAL_PRIORITY_CLASS | HIGH_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS,
                          NULL, NULL, &si, &pi);
    ok(bRet, "CreateProcessW failed. LastError: %lu\n", GetLastError());
    dwPriorityClass = GetPriorityClass(pi.hProcess);
    ok(dwPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());
    ok(dwPriorityClass == BELOW_NORMAL_PRIORITY_CLASS, "dwPriorityClass: 0x%08lx != BELOW_NORMAL_PRIORITY_CLASS\n", dwPriorityClass);
    bRet = TerminateProcess(pi.hProcess, 0);
    ok(bRet, "TerminateProcess failed. LastError: %lu\n", GetLastError());
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    bRet = CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                          NORMAL_PRIORITY_CLASS | ABOVE_NORMAL_PRIORITY_CLASS | HIGH_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS,
                          NULL, NULL, &si, &pi);
    ok(bRet, "CreateProcessW failed. LastError: %lu\n", GetLastError());
    dwPriorityClass = GetPriorityClass(pi.hProcess);
    ok(dwPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());
    ok(dwPriorityClass == NORMAL_PRIORITY_CLASS, "dwPriorityClass: 0x%08lx != NORMAL_PRIORITY_CLASS\n", dwPriorityClass);
    bRet = TerminateProcess(pi.hProcess, 0);
    ok(bRet, "TerminateProcess failed. LastError: %lu\n", GetLastError());
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    bRet = CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                          ABOVE_NORMAL_PRIORITY_CLASS | HIGH_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS,
                          NULL, NULL, &si, &pi);
    ok(bRet, "CreateProcessW failed. LastError: %lu\n", GetLastError());
    dwPriorityClass = GetPriorityClass(pi.hProcess);
    ok(dwPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());
    ok(dwPriorityClass == ABOVE_NORMAL_PRIORITY_CLASS, "dwPriorityClass: 0x%08lx != ABOVE_NORMAL_PRIORITY_CLASS\n", dwPriorityClass);
    bRet = TerminateProcess(pi.hProcess, 0);
    ok(bRet, "TerminateProcess failed. LastError: %lu\n", GetLastError());
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    bRet = CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                          HIGH_PRIORITY_CLASS | REALTIME_PRIORITY_CLASS,
                          NULL, NULL, &si, &pi);
    ok(bRet, "CreateProcessW failed. LastError: %lu\n", GetLastError());
    dwPriorityClass = GetPriorityClass(pi.hProcess);
    ok(dwPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());
    ok(dwPriorityClass == HIGH_PRIORITY_CLASS, "dwPriorityClass: 0x%08lx != HIGH_PRIORITY_CLASS\n", dwPriorityClass);
    bRet = TerminateProcess(pi.hProcess, 0);
    ok(bRet, "TerminateProcess failed. LastError: %lu\n", GetLastError());
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    bRet = CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                          REALTIME_PRIORITY_CLASS,
                          NULL, NULL, &si, &pi);
    ok(bRet, "CreateProcessW failed. LastError: %lu\n", GetLastError());
    dwPriorityClass = GetPriorityClass(pi.hProcess);
    ok(dwPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());
    ok(dwPriorityClass == REALTIME_PRIORITY_CLASS, "dwPriorityClass: 0x%08lx != REALTIME_PRIORITY_CLASS\n", dwPriorityClass);
    bRet = TerminateProcess(pi.hProcess, 0);
    ok(bRet, "TerminateProcess failed. LastError: %lu\n", GetLastError());
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    // Default priority.
    // It depends on current process priority.

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    bRet = SetPriorityClass(hCurrentProcess, REALTIME_PRIORITY_CLASS);
    ok(bRet, "SetPriorityClass failed. LastError: %lu\n", GetLastError());
    bRet = CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                          0,
                          NULL, NULL, &si, &pi);
    ok(bRet, "CreateProcessW failed. LastError: %lu\n", GetLastError());
    dwPriorityClass = GetPriorityClass(pi.hProcess);
    ok(dwPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());
    ok(dwPriorityClass == NORMAL_PRIORITY_CLASS, "dwPriorityClass: 0x%08lx != NORMAL_PRIORITY_CLASS\n", dwPriorityClass);
    bRet = TerminateProcess(pi.hProcess, 0);
    ok(bRet, "TerminateProcess failed. LastError: %lu\n", GetLastError());
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    bRet = SetPriorityClass(hCurrentProcess, HIGH_PRIORITY_CLASS);
    ok(bRet, "SetPriorityClass failed. LastError: %lu\n", GetLastError());
    bRet = CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                          0,
                          NULL, NULL, &si, &pi);
    ok(bRet, "CreateProcessW failed. LastError: %lu\n", GetLastError());
    dwPriorityClass = GetPriorityClass(pi.hProcess);
    ok(dwPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());
    ok(dwPriorityClass == NORMAL_PRIORITY_CLASS, "dwPriorityClass: 0x%08lx != NORMAL_PRIORITY_CLASS\n", dwPriorityClass);
    bRet = TerminateProcess(pi.hProcess, 0);
    ok(bRet, "TerminateProcess failed. LastError: %lu\n", GetLastError());
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    bRet = SetPriorityClass(hCurrentProcess, ABOVE_NORMAL_PRIORITY_CLASS);
    ok(bRet, "SetPriorityClass failed. LastError: %lu\n", GetLastError());
    bRet = CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                          0,
                          NULL, NULL, &si, &pi);
    ok(bRet, "CreateProcessW failed. LastError: %lu\n", GetLastError());
    dwPriorityClass = GetPriorityClass(pi.hProcess);
    ok(dwPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());
    ok(dwPriorityClass == NORMAL_PRIORITY_CLASS, "dwPriorityClass: 0x%08lx != NORMAL_PRIORITY_CLASS\n", dwPriorityClass);
    bRet = TerminateProcess(pi.hProcess, 0);
    ok(bRet, "TerminateProcess failed. LastError: %lu\n", GetLastError());
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    bRet = SetPriorityClass(hCurrentProcess, NORMAL_PRIORITY_CLASS);
    ok(bRet, "SetPriorityClass failed. LastError: %lu\n", GetLastError());
    bRet = CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                          0,
                          NULL, NULL, &si, &pi);
    ok(bRet, "CreateProcessW failed. LastError: %lu\n", GetLastError());
    dwPriorityClass = GetPriorityClass(pi.hProcess);
    ok(dwPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());
    ok(dwPriorityClass == NORMAL_PRIORITY_CLASS, "dwPriorityClass: 0x%08lx != NORMAL_PRIORITY_CLASS\n", dwPriorityClass);
    bRet = TerminateProcess(pi.hProcess, 0);
    ok(bRet, "TerminateProcess failed. LastError: %lu\n", GetLastError());
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    bRet = SetPriorityClass(hCurrentProcess, BELOW_NORMAL_PRIORITY_CLASS);
    ok(bRet, "SetPriorityClass failed. LastError: %lu\n", GetLastError());
    bRet = CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                          0,
                          NULL, NULL, &si, &pi);
    ok(bRet, "CreateProcessW failed. LastError: %lu\n", GetLastError());
    dwPriorityClass = GetPriorityClass(pi.hProcess);
    ok(dwPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());
    ok(dwPriorityClass == BELOW_NORMAL_PRIORITY_CLASS, "dwPriorityClass: 0x%08lx != BELOW_NORMAL_PRIORITY_CLASS\n", dwPriorityClass);
    bRet = TerminateProcess(pi.hProcess, 0);
    ok(bRet, "TerminateProcess failed. LastError: %lu\n", GetLastError());
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    SetLastError(0xdeadbeef);
    wcscpy(wszCmdLineBuffer, wszCmdLine);
    ZeroMemory(&si, sizeof(si));
    si.cb = sizeof(si);
    ZeroMemory(&pi, sizeof(pi));
    bRet = SetPriorityClass(hCurrentProcess, IDLE_PRIORITY_CLASS);
    ok(bRet, "SetPriorityClass failed. LastError: %lu\n", GetLastError());
    bRet = CreateProcessW(NULL, wszCmdLineBuffer, NULL, NULL, FALSE,
                          0,
                          NULL, NULL, &si, &pi);
    ok(bRet, "CreateProcessW failed. LastError: %lu\n", GetLastError());
    dwPriorityClass = GetPriorityClass(pi.hProcess);
    ok(dwPriorityClass != 0, "GetPriorityClass failed. LastError: %lu\n", GetLastError());
    ok(dwPriorityClass == IDLE_PRIORITY_CLASS, "dwPriorityClass: 0x%08lx != IDLE_PRIORITY_CLASS\n", dwPriorityClass);
    bRet = TerminateProcess(pi.hProcess, 0);
    ok(bRet, "TerminateProcess failed. LastError: %lu\n", GetLastError());
    WaitForSingleObject(pi.hProcess, INFINITE);
    CloseHandle(pi.hThread);
    CloseHandle(pi.hProcess);

    bRet = SetPriorityClass(hCurrentProcess, dwCurrentProcessPriorityClass);
    ok(bRet, "SetPriorityClass failed. LastError: %lu\n", GetLastError());
}

START_TEST(CreateProcess)
{
    test_CreateProcessA_StaticUnicodeString();

    test_CreateProcessW_CreationFlags();
}
