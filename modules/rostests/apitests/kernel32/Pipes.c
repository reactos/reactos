/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GNU GPLv2 only as published by the Free Software Foundation
 * PURPOSE:         Test for pipe (CORE-17376)
 * PROGRAMMER:      Simone Mario Lombardo <me@simonelombardo.com>
 *                  Julen Urizar Compains <julenuri@hotmail.com>
 */

#include "precomp.h"

static const char* g_PipeName = "\\\\.\\pipe\\rostest_pipe";

#define MINBUFFERSIZE 1
#define MAXBUFFERSIZE 255
#define TEST_MESSAGE "Test"
#define TEST_MESSAGE_SIZE 4

static DWORD g_dwReadBufferSize;

DWORD WINAPI PipeWriter(_In_ PVOID Param)
{
    HANDLE hPipe = (HANDLE)Param;
    DWORD cbWritten = 0;
    BOOL Success = WriteFile(hPipe, TEST_MESSAGE, TEST_MESSAGE_SIZE, &cbWritten, NULL);

    ok(Success, "WriteFile() failed, last error = 0x%lx\n", GetLastError());
    ok_int(cbWritten, TEST_MESSAGE_SIZE);

    return 0;
}

DWORD WINAPI PipeReader(_In_ PVOID Param)
{
    CHAR outMsg[MAXBUFFERSIZE];
    HANDLE hPipe=(HANDLE)Param;

    DWORD cbRead = 0;
    BOOL Success = ReadFile(hPipe, outMsg, g_dwReadBufferSize, &cbRead, NULL);
    
    if (g_dwReadBufferSize == MINBUFFERSIZE)
        ok(!Success, "ReadFile() unexpectedly succeeded\n");
    else
        ok(Success, "ReadFile() failed, last error = 0x%lx\n", GetLastError());

    ok(cbRead != 0, "cbRead == 0\n");

    if (g_dwReadBufferSize == MINBUFFERSIZE)
        ok_hex(GetLastError(), ERROR_MORE_DATA);

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

    if (hPipeReader == INVALID_HANDLE_VALUE)
        return;

    hThreadReader = CreateThread(NULL, 0, PipeReader, hPipeReader, 0, NULL);
    ok(hThreadReader != INVALID_HANDLE_VALUE, "CreateThread failed\n");

    hPipeWriter = CreateFile(
        g_PipeName,
        GENERIC_READ | GENERIC_WRITE,
        FILE_SHARE_READ | FILE_SHARE_WRITE,
        NULL,
        OPEN_EXISTING,
        FILE_ATTRIBUTE_NORMAL,
        NULL);
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
