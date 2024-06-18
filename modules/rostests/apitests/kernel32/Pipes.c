/*
 * PROJECT:         ReactOS api tests
 * LICENSE:         GNU GPLv2 only as published by the Free Software Foundation
 * PURPOSE:         Test for pipe (CORE-17376)
 * PROGRAMMER:      Simone Mario Lombardo
 */

#include "precomp.h"

#define LP TEXT("\\\\.\\pipe\\rostest_pipe")

#define MINBUFFERSIZE 1
#define MAXBUFFERSIZE 255

static DWORD dReadBufferSize;

DWORD
WINAPI
PipeWriter(
        _In_ PVOID Param)
{
    DWORD cbWritten;
    HANDLE hPipe;
    DWORD dwLastError;
    BOOL Success;
    PCSTR pszInMsg = "Test";

    hPipe = CreateFile(LP, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE,
            NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    ok(hPipe != INVALID_HANDLE_VALUE, "CreateFile failed, results might not be accurate\n");
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        Sleep(0);
        Success = WriteFile(hPipe, &pszInMsg, 4, &cbWritten, NULL);
        dwLastError = GetLastError();

        ok(Success, "Pipe's WriteFile returned FALSE, instead of expected TRUE\n");
        if(dwLastError != 0)
            trace("Last Error = 0x%lX\n",dwLastError);

        ok(cbWritten > 0,"Pipe's Writefile has lpNumberOfBytesWritten <= 0\n");

        CloseHandle(hPipe);
    }
    return 0;
}

DWORD
WINAPI
PipeReader(
        _In_ PVOID Param)
{
    HANDLE hPipe;
    DWORD cbRead;
    HANDLE hThread;
    DWORD dwLastError;
    BOOL Success;
    CHAR szOutMsg[MAXBUFFERSIZE];

    hPipe = CreateNamedPipeA( 
        LP,
        PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_MESSAGE|PIPE_READMODE_MESSAGE,
        1,
        MAXBUFFERSIZE,
        MAXBUFFERSIZE,
        0,
        NULL
    );
    ok(hPipe != INVALID_HANDLE_VALUE, "CreateNamedPipeA failed\n");
    if (hPipe != INVALID_HANDLE_VALUE)
    {
        hThread = CreateThread(NULL,0, PipeWriter, NULL, 0, NULL);
        ok(hThread != INVALID_HANDLE_VALUE, "CreateThread failed\n");
        if (hThread != INVALID_HANDLE_VALUE)
        {
            Sleep(0);
            Success = ReadFile(hPipe, &szOutMsg, dReadBufferSize, &cbRead, NULL);
            dwLastError = GetLastError();

            if(dReadBufferSize == MINBUFFERSIZE)
                ok(!Success, "Pipe's ReadFile returned TRUE, instead of expected FALSE\n");
            else
            {
                ok(Success, "Pipe's ReadFile returned FALSE, instead of expected TRUE\n");
                if(dwLastError != 0)
                    trace("Last Error = 0x%lX\n",dwLastError);
            }

            ok(cbRead > 0,"Pipe's Readfile has lpNumberOfBytesRead <= 0\n");

            if(dReadBufferSize == MINBUFFERSIZE)
                ok(dwLastError == ERROR_MORE_DATA, "Pipe's ReadFile last error is unexpected\n");

            WaitForSingleObject(hThread, INFINITE);
            CloseHandle(hThread);
        }
        CloseHandle(hPipe);
    }
    return 0;
}

VOID
StartTestCORE17376(
     _In_ DWORD adReadBufferSize)
{
    HANDLE hThread;

    trace("adReadBufferSize = %ld - START\n", adReadBufferSize);

    dReadBufferSize = adReadBufferSize;

    hThread = CreateThread(NULL,0, PipeReader, NULL, 0, NULL);
    ok(hThread != INVALID_HANDLE_VALUE, "CreateThread failed\n");
    if (hThread != INVALID_HANDLE_VALUE)
    {
        WaitForSingleObject(hThread, INFINITE);
        CloseHandle(hThread);
    }

    trace("adReadBufferSize = %ld - COMPLETED\n", adReadBufferSize);
}

START_TEST(Pipes)
{
    StartTestCORE17376(MINBUFFERSIZE);
    StartTestCORE17376(MAXBUFFERSIZE);
}

