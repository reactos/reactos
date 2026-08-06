/*
 * PROJECT:     ReactOS API tests
 * LICENSE:     MIT (https://spdx.org/licenses/MIT)
 * PURPOSE:     Named Pipe File System (NPFS) FSCTL_GET_CONNECTION_ATTRIBUTES test
 * COPYRIGHT:   Copyright 2026 Mohammad Amin Mollazadeh
 */

#include <stdio.h>
#include <apitest.h>
#include <apitest_guard.h>
#include <ndk/ntndk.h>
#include <strsafe.h>

static char ClientProcessIdAttributeName[] = "ClientProcessId";
static char ClientSessionIdAttributeName[] = "ClientSessionId";

static char NonExistenceAttributeName[] = "NonExistenceAttribute";
static char CustomAttributeName1[] = "MyOwnAttribute1";
static char CustomAttributeName2[] = "MyOwnAttribute2";
static char CustomAttributeName1Upper[] = "MYOWNATTRIBUTE1";

static int TestValue1 = 0xC001D00D;
static char TestValue2[] = "The Test Value";
static char TestValue3[] = "The Third Test Value";

static void NamedPipeClientChildProcess()
{
    HANDLE hPipe = INVALID_HANDLE_VALUE;
    DWORD BytesRead = 0;
    BYTE Buffer[255];

    hPipe = CreateFileW(L"\\\\.\\pipe\\NpfsApiTestPipe",
                        GENERIC_READ,
                        0,
                        NULL,
                        OPEN_EXISTING,
                        0,
                        NULL);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        skip("Failed to open the named pipe: 0x%08X\n", GetLastError());
        return;
    }

    ReadFile(hPipe, Buffer, sizeof(Buffer), &BytesRead, NULL);

    CloseHandle(hPipe);
}

static BOOL RunChildProcess(PPROCESS_INFORMATION ppi)
{
    char cmdline[MAX_PATH];
    char exe[MAX_PATH];
    STARTUPINFOA si = { 0 };

    GetModuleFileNameA(NULL, exe, sizeof(exe));
    sprintf(cmdline, "\"%s\" %s %s", exe, "NpfsGetConnectionAttributes", "subtest");

    si.cb = sizeof(si);
    return CreateProcessA(exe, cmdline, NULL, NULL, FALSE, 0, NULL, NULL, &si, ppi);
}

START_TEST(NpfsGetConnectionAttributes)
{
    NTSTATUS Status;

    HANDLE hPipe = INVALID_HANDLE_VALUE;

    char Buffer[256];
    IO_STATUS_BLOCK IoStatusBlock = {0};

    PROCESS_INFORMATION ProcessInfo = {0};
    DWORD SessionId;

    char** argv;
    int argc = winetest_get_mainargs(&argv);
    if (argc > 2)
    {
        NamedPipeClientChildProcess();
        return;
    }

    hPipe = CreateNamedPipeW(L"\\\\.\\pipe\\NpfsApiTestPipe",
                             PIPE_ACCESS_DUPLEX,
                             PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT,
                             1,
                             256,
                             256,
                             0,
                             NULL);
    if (hPipe == INVALID_HANDLE_VALUE)
    {
        skip("Failed to create/open the named pipe: 0x%08X\n", GetLastError());
        return;
    }

    /* Query for a non-existance attribute */
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             NonExistenceAttributeName, sizeof(NonExistenceAttributeName),
                             Buffer, sizeof(Buffer));
    ok_eq_hex(Status, STATUS_NOT_FOUND);

    /* Set a custom attribute and query it back */
    RtlCopyMemory(Buffer, CustomAttributeName1, sizeof(CustomAttributeName1));
    RtlCopyMemory(Buffer + sizeof(CustomAttributeName1), &TestValue1, sizeof(TestValue1));
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_SET_CONNECTION_ATTRIBUTE,
                             Buffer, sizeof(CustomAttributeName1) + sizeof(TestValue1),
                             NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(IoStatusBlock.Information, 0);

    *(int*)Buffer = 0xdeadbeef;
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             CustomAttributeName1, sizeof(CustomAttributeName1),
                             Buffer, sizeof(int));
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(IoStatusBlock.Information, sizeof(int));
    ok_eq_hex(*(int*)Buffer, 0xC001D00D);

    /* Set a second custom attribute and query attributes back */
    RtlCopyMemory(Buffer, CustomAttributeName2, sizeof(CustomAttributeName2));
    RtlCopyMemory(Buffer + sizeof(CustomAttributeName2), TestValue2, sizeof(TestValue2));
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_SET_CONNECTION_ATTRIBUTE,
                             Buffer, sizeof(CustomAttributeName2) + sizeof(TestValue2),
                             NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(IoStatusBlock.Information, 0);

    *(int*)Buffer = 0xdeadbeef;
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             CustomAttributeName1, sizeof(CustomAttributeName1),
                             Buffer, sizeof(int));
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(IoStatusBlock.Information, sizeof(int));
    ok_eq_hex(*(int*)Buffer, 0xC001D00D);

    RtlZeroMemory(Buffer, sizeof(Buffer));
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             CustomAttributeName2, sizeof(CustomAttributeName2),
                             Buffer, sizeof(TestValue2));
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(IoStatusBlock.Information, sizeof(TestValue2));
    ok_eq_str(Buffer, TestValue2);

    /* Set the first attribute to a bigger value */
    RtlCopyMemory(Buffer, CustomAttributeName1, sizeof(CustomAttributeName1));
    RtlCopyMemory(Buffer + sizeof(CustomAttributeName1), TestValue3, sizeof(TestValue3));
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_SET_CONNECTION_ATTRIBUTE,
                             Buffer, sizeof(CustomAttributeName1) + sizeof(TestValue3),
                             NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(IoStatusBlock.Information, 0);

    RtlZeroMemory(Buffer, sizeof(TestValue3));
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             CustomAttributeName1, sizeof(CustomAttributeName1),
                             Buffer, sizeof(TestValue3));
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(IoStatusBlock.Information, sizeof(TestValue3));
    ok_eq_str(Buffer, TestValue3);

    /* Query NULL attribute */
    RtlZeroMemory(Buffer, sizeof(Buffer));
    RtlZeroMemory(&IoStatusBlock, sizeof(IoStatusBlock));
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             NULL, 0,
                             Buffer, sizeof(Buffer));
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);
    ok_eq_ulong(IoStatusBlock.Information, 0);

    /* Query with NULL as buffer */
    RtlZeroMemory(Buffer, sizeof(Buffer));
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             CustomAttributeName2, sizeof(CustomAttributeName2),
                             NULL, 0);
    ok_eq_hex(Status, STATUS_BUFFER_TOO_SMALL);
    ok_eq_ulong(IoStatusBlock.Information, 0);

    /* Query with a small buffer */
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             CustomAttributeName2, sizeof(CustomAttributeName2),
                             Buffer, sizeof(char));
    ok_eq_hex(Status, STATUS_BUFFER_TOO_SMALL);
    ok_eq_ulong(IoStatusBlock.Information, 0);

    /* Case-insensitive query */
    RtlZeroMemory(Buffer, sizeof(TestValue1));
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             CustomAttributeName1Upper, sizeof(CustomAttributeName1Upper),
                             Buffer, sizeof(TestValue1));
    ok_eq_hex(Status, STATUS_NOT_FOUND);

    /* Zero-length value: Unset the attribute */
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_SET_CONNECTION_ATTRIBUTE,
                             CustomAttributeName1, sizeof(CustomAttributeName1),
                             NULL, 0);
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(IoStatusBlock.Information, 0);

    RtlZeroMemory(Buffer, sizeof(TestValue1));
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             CustomAttributeName1, sizeof(CustomAttributeName1),
                             Buffer, sizeof(TestValue1));
    ok_eq_hex(Status, STATUS_NOT_FOUND);
    ok_eq_ulong(IoStatusBlock.Information, 0);

    /* Empty name */
    RtlZeroMemory(Buffer, sizeof(Buffer));
    RtlCopyMemory(Buffer + 1, &TestValue1, sizeof(TestValue1));
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_SET_CONNECTION_ATTRIBUTE,
                             Buffer, 1 + sizeof(TestValue1),
                             NULL, 0);
    ok_eq_hex(Status, STATUS_INVALID_PARAMETER);

    RtlZeroMemory(Buffer, sizeof(Buffer));
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             "", sizeof(""),
                             Buffer, sizeof(TestValue1));
    ok_eq_hex(Status, STATUS_NOT_FOUND);

    /* Query for the PID and process session ID attributes.
     * These attributes are set by NpCreateClientEnd. */

    /* ClientProcessId and ClientSessionId should return STATUS_NOT_FOUND
     * when there's no client connected to the pipe. */
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             ClientProcessIdAttributeName, sizeof(ClientProcessIdAttributeName),
                             Buffer, sizeof(ULONG));
    ok_eq_hex(Status, STATUS_NOT_FOUND);

    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             ClientSessionIdAttributeName, sizeof(ClientSessionIdAttributeName),
                             Buffer, sizeof(ULONG));
    ok_eq_hex(Status, STATUS_NOT_FOUND);

    if (!RunChildProcess(&ProcessInfo))
    {
        skip("Failed to create the child process: 0x%08X\n", GetLastError());
        goto Cleanup;
    }
    /* Wait for client connection */
    if (!ConnectNamedPipe(hPipe, NULL))
    {
        skip("Failed to connect the named pipe: 0x%08X\n", GetLastError());
        goto Cleanup;
    }

    *(DWORD*)Buffer = 0xdeadbeef;
    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             ClientProcessIdAttributeName, sizeof(ClientProcessIdAttributeName),
                             Buffer, sizeof(DWORD));
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(IoStatusBlock.Information, sizeof(DWORD));
    ok_eq_ulong(*(DWORD*)Buffer, ProcessInfo.dwProcessId);

    if (!ProcessIdToSessionId(ProcessInfo.dwProcessId, &SessionId))
    {
        skip("Failed to get the session ID of the client process: 0x%08X\n", GetLastError());
        goto Cleanup;
    }

    Status = NtFsControlFile(hPipe, NULL, NULL, NULL,
                             &IoStatusBlock,
                             FSCTL_PIPE_GET_CONNECTION_ATTRIBUTE,
                             ClientSessionIdAttributeName, sizeof(ClientSessionIdAttributeName),
                             Buffer, sizeof(ULONG));
    ok_eq_hex(Status, STATUS_SUCCESS);
    ok_eq_ulong(IoStatusBlock.Information, sizeof(ULONG));
    ok_eq_ulong(*(ULONG*)Buffer, SessionId);

    /* Disconnect the named pipe and wait for the child process to exit */
    DisconnectNamedPipe(hPipe);
    winetest_wait_child_process(ProcessInfo.hProcess);

Cleanup:
    if (ProcessInfo.hThread != NULL)
        CloseHandle(ProcessInfo.hThread);
    if (ProcessInfo.hProcess != NULL)
        CloseHandle(ProcessInfo.hProcess);
    if (hPipe != INVALID_HANDLE_VALUE)
        NtClose(hPipe);
}
