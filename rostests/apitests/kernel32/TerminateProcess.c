/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         LGPLv2.1+ - See COPYING.LIB in the top level directory
 * PURPOSE:         Test for TerminateProcess
 * PROGRAMMER:      Thomas Faber <thomas.faber@reactos.org>
 */

#include <apitest.h>

#include <ndk/obfuncs.h>
#include <strsafe.h>

static
HANDLE
StartChild(
    _In_ PCWSTR Argument,
    _In_ DWORD Flags,
    _Out_opt_ PDWORD ProcessId)
{
    BOOL Success;
    WCHAR FileName[MAX_PATH];
    WCHAR CommandLine[MAX_PATH];
    STARTUPINFOW StartupInfo;
    PROCESS_INFORMATION ProcessInfo;

    GetModuleFileNameW(NULL, FileName, _countof(FileName));
    StringCbPrintfW(CommandLine,
                    sizeof(CommandLine),
                    L"\"%ls\" TerminateProcess %ls",
                    FileName,
                    Argument);

    RtlZeroMemory(&StartupInfo, sizeof(StartupInfo));
    StartupInfo.cb = sizeof(StartupInfo);
    /* HACK: running the test under rosautotest seems to keep another reference
     * to the child process around until the test finishes (on both ROS and
     * Windows)... I'm too lazy to investigate very much so let's just redirect
     * the child std handles to nowhere. ok() is useless in half the child
     * processes anyway.
     */
    StartupInfo.dwFlags = STARTF_USESTDHANDLES;

    Success = CreateProcessW(FileName,
                             CommandLine,
                             NULL,
                             NULL,
                             FALSE,
                             Flags,
                             NULL,
                             NULL,
                             &StartupInfo,
                             &ProcessInfo);
    if (!Success)
    {
        skip("CreateProcess failed with %lu\n", GetLastError());
        if (ProcessId)
            *ProcessId = 0;
        return NULL;
    }
    CloseHandle(ProcessInfo.hThread);
    if (ProcessId)
        *ProcessId = ProcessInfo.dwProcessId;
    return ProcessInfo.hProcess;
}

static
VOID
TraceHandleCount_(
    _In_ HANDLE hObject,
    _In_ PCSTR File,
    _In_ INT Line)
{
    NTSTATUS Status;
    OBJECT_BASIC_INFORMATION BasicInfo;

    Status = NtQueryObject(hObject,
                           ObjectBasicInformation,
                           &BasicInfo,
                           sizeof(BasicInfo),
                           NULL);
    if (!NT_SUCCESS(Status))
    {
        ok_(File, Line)(0, "NtQueryObject failed with status 0x%lx\n", Status);
        return;
    }
    ok_(File, Line)(0, "Handle %p still has %lu open handles, %lu references\n", hObject, BasicInfo.HandleCount, BasicInfo.PointerCount);
}

#define WaitExpectSuccess(h, ms) WaitExpect_(h, ms, WAIT_OBJECT_0, __FILE__, __LINE__)
#define WaitExpectTimeout(h, ms) WaitExpect_(h, ms, WAIT_TIMEOUT, __FILE__, __LINE__)
static
VOID
WaitExpect_(
    _In_ HANDLE hWait,
    _In_ DWORD Milliseconds,
    _In_ DWORD ExpectedError,
    _In_ PCSTR File,
    _In_ INT Line)
{
    DWORD Error;

    Error = WaitForSingleObject(hWait, Milliseconds);
    ok_(File, Line)(Error == ExpectedError, "Wait for %p return %lu\n", hWait, Error);
}

#define CloseProcessAndVerify(hp, pid, code) CloseProcessAndVerify_(hp, pid, code, __FILE__, __LINE__)
static
VOID
CloseProcessAndVerify_(
    _In_ HANDLE hProcess,
    _In_ DWORD ProcessId,
    _In_ UINT ExpectedExitCode,
    _In_ PCSTR File,
    _In_ INT Line)
{
    int i = 0;
    DWORD Error;
    DWORD ExitCode;
    BOOL Success;

    WaitExpect_(hProcess, 0, WAIT_OBJECT_0, File, Line);
    Success = GetExitCodeProcess(hProcess, &ExitCode);
    ok_(File, Line)(Success, "GetExitCodeProcess failed with %lu\n", GetLastError());
    CloseHandle(hProcess);
    while ((hProcess = OpenProcess(PROCESS_QUERY_INFORMATION, FALSE, ProcessId)) != NULL)
    {
        if (++i >= 100)
        {
            TraceHandleCount_(hProcess, File, Line);
            CloseHandle(hProcess);
            break;
        }
        CloseHandle(hProcess);
        Sleep(100);
    }
    Error = GetLastError();
    ok_(File, Line)(hProcess == NULL, "OpenProcess succeeded unexpectedly for pid 0x%lx\n", ProcessId);
    ok_(File, Line)(Error == ERROR_INVALID_PARAMETER, "Error = %lu\n", Error);
    ok_(File, Line)(ExitCode == ExpectedExitCode, "Exit code is %lu but expected %lu\n", ExitCode, ExpectedExitCode);
}

static
VOID
TestTerminateProcess(
    _In_ HANDLE hEvent)
{
    HANDLE hProcess;
    DWORD ProcessId;

    /* Regular child process that returns from the test function */
    /* HACK: These two tests don't work if stdout is a pipe. See StartChild */
    ResetEvent(hEvent);
    hProcess = StartChild(L"child", 0, &ProcessId);
    WaitExpectSuccess(hEvent, 5000);
    WaitExpectSuccess(hProcess, 5000);
    CloseProcessAndVerify(hProcess, ProcessId, 0);

    ResetEvent(hEvent);
    hProcess = StartChild(L"child", 0, &ProcessId);
    WaitExpectSuccess(hProcess, 5000);
    WaitExpectSuccess(hEvent, 0);
    CloseProcessAndVerify(hProcess, ProcessId, 0);

    /* Suspended process -- never gets a chance to initialize */
    ResetEvent(hEvent);
    hProcess = StartChild(L"child", CREATE_SUSPENDED, &ProcessId);
    WaitExpectTimeout(hEvent, 100);
    WaitExpectTimeout(hProcess, 100);
    TerminateProcess(hProcess, 123);
    WaitExpectSuccess(hProcess, 5000);
    CloseProcessAndVerify(hProcess, ProcessId, 123);

    /* Waiting process -- we have to terminate it */
    ResetEvent(hEvent);
    hProcess = StartChild(L"wait", 0, &ProcessId);
    WaitExpectTimeout(hProcess, 100);
    TerminateProcess(hProcess, 123);
    WaitExpectSuccess(hProcess, 5000);
    CloseProcessAndVerify(hProcess, ProcessId, 123);

    /* Process calls ExitProcess */
    ResetEvent(hEvent);
    hProcess = StartChild(L"child exit 456", 0, &ProcessId);
    WaitExpectSuccess(hEvent, 5000);
    WaitExpectSuccess(hProcess, 5000);
    CloseProcessAndVerify(hProcess, ProcessId, 456);

    /* Process calls TerminateProcess with GetCurrentProcess */
    ResetEvent(hEvent);
    hProcess = StartChild(L"child terminate 456", 0, &ProcessId);
    WaitExpectSuccess(hEvent, 5000);
    WaitExpectSuccess(hProcess, 5000);
    CloseProcessAndVerify(hProcess, ProcessId, 456);

    /* Process calls TerminateProcess with real handle to itself */
    ResetEvent(hEvent);
    hProcess = StartChild(L"child terminate2 456", 0, &ProcessId);
    WaitExpectSuccess(hEvent, 5000);
    WaitExpectSuccess(hProcess, 5000);
    CloseProcessAndVerify(hProcess, ProcessId, 456);
}

START_TEST(TerminateProcess)
{
    HANDLE hEvent;
    BOOL Success;
    DWORD Error;
    int argc;
    char **argv;

    hEvent = CreateEventW(NULL, TRUE, FALSE, L"kernel32_apitest_TerminateProcess_event");
    Error = GetLastError();
    if (!hEvent)
    {
        skip("CreateEvent failed with error %lu\n", Error);
        return;
    }
    argc = winetest_get_mainargs(&argv);
    if (argc >= 3)
    {
        ok(Error == ERROR_ALREADY_EXISTS, "Error = %lu\n", Error);
        if (!strcmp(argv[2], "wait"))
        {
            WaitExpectSuccess(hEvent, 30000);
        }
        else
        {
            Success = SetEvent(hEvent);
            ok(Success, "SetEvent failed with return %d, error %lu\n", Success, GetLastError());
        }
    }
    else
    {
        ok(Error == NO_ERROR, "Error = %lu\n", Error);
        TestTerminateProcess(hEvent);
    }
    CloseHandle(hEvent);
    if (argc >= 5)
    {
        UINT ExitCode = strtol(argv[4], NULL, 10);

        fflush(stdout);
        if (!strcmp(argv[3], "exit"))
            ExitProcess(ExitCode);
        else if (!strcmp(argv[3], "terminate"))
            TerminateProcess(GetCurrentProcess(), ExitCode);
        else if (!strcmp(argv[3], "terminate2"))
        {
            HANDLE hProcess;
            hProcess = OpenProcess(PROCESS_TERMINATE, FALSE, GetCurrentProcessId());
            TerminateProcess(hProcess, ExitCode);
        }
        ok(0, "Should have terminated\n");
    }
}
