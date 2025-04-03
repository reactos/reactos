/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GNU GPLv2 only as published by the Free Software Foundation
 * PURPOSE:         Test for pipe (CORE-17376)
 * PROGRAMMER:      Simone Mario Lombardo
 */

#include "precomp.h"

static const char* g_PipeName = "\\\\.\\pipe\\rostest_pipe";

#define MINBUFFERSIZE 1
#define MAXBUFFERSIZE 255
#define TEST_MESSAGE "Test"
#define TEST_MESSAGE_SIZE 4

static DWORD g_dwReadBufferSize;

void CheckError(BOOL Success, DWORD dwLastError, const char* errorMsg)
{
    ok(Success, errorMsg);
    if (!Success)
    {
        trace("Error: %s. LastError: 0x%lX\n", errorMsg, dwLastError);
    }
}

DWORD WINAPI PipeWriter(_In_ PVOID Param)
{
    HANDLE hPipe = (HANDLE)Param;
    DWORD cbWritten = 0;
    BOOL Success = WriteFile(hPipe, TEST_MESSAGE, TEST_MESSAGE_SIZE, &cbWritten, NULL);
    DWORD dwLastError = GetLastError();

    CheckError(Success, dwLastError, "Pipe's WriteFile failed");
    ok(cbWritten == TEST_MESSAGE_SIZE, "Invalid cbWritten: 0x%lx\n", cbWritten);

    return 0;
}

DWORD WINAPI PipeReader(_In_ PVOID Param)
{
    HANDLE hPipe = (HANDLE)Param;
    CHAR* szOutMsg = (CHAR*)malloc(g_dwReadBufferSize);
    if (!szOutMsg)
    {
        trace("Memory allocation failed for szOutMsg\n");
        return -1;
    }

    DWORD cbRead = 0;
    BOOL Success = ReadFile(hPipe, szOutMsg, g_dwReadBufferSize, &cbRead, NULL);
    DWORD dwLastError = GetLastError();

    if (g_dwReadBufferSize == MINBUFFERSIZE)
    {
        ok(!Success, "Pipe's ReadFile returned TRUE, instead of expected FALSE\n");
    }
    else
    {
        CheckError(Success, dwLastError, "Pipe's ReadFile failed");
    }

    ok(cbRead > 0, "Invalid cbRead: 0x%lx\n", cbRead);

    if (g_dwReadBufferSize == MINBUFFERSIZE)
        ok(dwLastError == ERROR_MORE_DATA, "Pipe's ReadFile last error is unexpected\n");

    free(szOutMsg);
    return 0;
}

VOID StartTestCORE17376(_In_ DWORD adReadBufferSize)
{
    HANDLE hPipeReader, hPipeWriter, hThreadReader, hThreadWriter;
    trace("adReadBufferSize = %ld - START\n", adReadBufferSize);

    g_dwReadBufferSize = adReadBufferSize;

    hPipeReader = CreateNamedPipeA(
        g_PipeName,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE,
        1,
        MAXBUFFERSIZE,
        MAXBUFFERSIZE,
        0,
        NULL);
    ok(hPipeReader != INVALID_HANDLE_VALUE, "CreateNamedPipeA failed\n");

    if (hPipeReader == INVALID_HANDLE_VALUE) return;

    hThreadReader = CreateThread(NULL, 0, PipeReader, hPipeReader, 0, NULL);
    ok(hThreadReader != INVALID_HANDLE_VALUE, "CreateThread failed\n");

    hPipeWriter = CreateFile(
        g_PipeName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hPipeWriter != INVALID_HANDLE_VALUE, "CreateFile failed\n");

    if (hPipeWriter != INVALID_HANDLE_VALUE)
    {
        hThreadWriter = CreateThread(NULL, 0, PipeWriter, hPipeWriter, 0, NULL);
        ok(hThreadWriter != INVALID_HANDLE_VALUE, "CreateThread failed\n");

        if (hThreadWriter != INVALID_HANDLE_VALUE)
        {
            WaitForSingleObject(hThreadWriter, INFINITE);
            CloseHandle(hThreadWriter);
        }
        CloseHandle(hPipeWriter);
    }

    if (hThreadReader != INVALID_HANDLE_VALUE)
    {
        WaitForSingleObject(hThreadReader, INFINITE);
        CloseHandle(hThreadReader);
    }
    CloseHandle(hPipeReader);

    trace("adReadBufferSize = %ld - COMPLETED\n", adReadBufferSize);
}

START_TEST(Pipes)
{
    StartTestCORE17376(MINBUFFERSIZE);
    StartTestCORE17376(MAXBUFFERSIZE);
}

