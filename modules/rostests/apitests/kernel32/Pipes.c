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

static DWORD g_dwReadBufferSize;

DWORD
WINAPI
PipeWriter(_In_ PVOID Param)
{
    DWORD cbWritten;
    HANDLE hPipe = (HANDLE) Param;
    DWORD dwLastError;
    BOOL Success;
    PCSTR pszInMsg = "Test";

        cbWritten = 0xdeadbeef;
        Success = WriteFile(hPipe, &pszInMsg, 4, &cbWritten, NULL);
        dwLastError = GetLastError();

        ok(Success, "Pipe's WriteFile returned FALSE, instead of expected TRUE\n");
        ok(dwLastError == 0, "dwLastError was 0x%lx\n", dwLastError);
            trace("Last Error = 0x%lX\n",dwLastError);
        ok(cbWritten == 4,"Invalid cbWritten: 0x%lx\n", cbWritten);

    return 0;
}

DWORD
WINAPI
PipeReader(
        _In_ PVOID Param)
{
    DWORD cbRead;
    DWORD dwLastError;
    BOOL Success;
    CHAR szOutMsg[MAXBUFFERSIZE];
    HANDLE hPipe = (HANDLE) Param;

            cbRead = 0xdeadbeef;
            Success = ReadFile(hPipe, &szOutMsg, g_dwReadBufferSize, &cbRead, NULL);
            dwLastError = GetLastError();

            if(g_dwReadBufferSize == MINBUFFERSIZE)
            {
                ok(!Success, "Pipe's ReadFile returned TRUE, instead of expected FALSE\n");
            }
            else
            {
                ok(Success, "Pipe's ReadFile returned FALSE, instead of expected TRUE\n");
                ok(dwLastError == 0, "dwLastError was 0x%lx\n", dwLastError);
                    trace("Last Error = 0x%lX\n",dwLastError);
            }

            ok(cbRead > 0,"Invalid cbRead: 0x%lx\n", cbRead);

            if(g_dwReadBufferSize == MINBUFFERSIZE)
                ok(dwLastError == ERROR_MORE_DATA, "Pipe's ReadFile last error is unexpected\n");
    return 0;
}

VOID
StartTestCORE17376(
     _In_ DWORD adReadBufferSize)
{
    HANDLE hPipeReader;
    HANDLE hPipeWriter;
    HANDLE hThreadReader;
    HANDLE hThreadWriter;
    trace("adReadBufferSize = %ld - START\n", adReadBufferSize);

    g_dwReadBufferSize = adReadBufferSize;

    hPipeReader = CreateNamedPipeA( 
        g_PipeName,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE,
        1,
        MAXBUFFERSIZE,
        MAXBUFFERSIZE,
        0,
        NULL
    );
    ok(hPipeReader != INVALID_HANDLE_VALUE, "CreateNamedPipeA failed\n");
    hThreadReader = CreateThread(NULL,0, PipeReader, hPipeReader, 0, NULL);

    if (hPipeReader != INVALID_HANDLE_VALUE)
    {
    ok(hThreadReader != INVALID_HANDLE_VALUE, "CreateThread failed\n");

    hPipeWriter = CreateFile(g_PipeName, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hPipeWriter != INVALID_HANDLE_VALUE, "CreateFile failed, results might not be accurate\n");

    if (hPipeWriter != INVALID_HANDLE_VALUE)
    {
        hThreadWriter = CreateThread(NULL,0, PipeWriter, hPipeWriter, 0, NULL);
        ok(hThreadWriter != INVALID_HANDLE_VALUE, "CreateThread failed\n");
        if (hThreadWriter != INVALID_HANDLE_VALUE)
        {
            WaitForSingleObject(hThreadWriter, INFINITE);
            CloseHandle(hThreadWriter);
            if (hThreadReader != INVALID_HANDLE_VALUE)
            {
                WaitForSingleObject(hThreadReader, INFINITE);
                CloseHandle(hThreadReader);
            }
        }
        CloseHandle(hPipeWriter);
    }
    CloseHandle(hPipeReader);
    }
    trace("adReadBufferSize = %ld - COMPLETED\n", adReadBufferSize);
}

START_TEST(Pipes)
{
    StartTestCORE17376(MINBUFFERSIZE);
    StartTestCORE17376(MAXBUFFERSIZE);
}

