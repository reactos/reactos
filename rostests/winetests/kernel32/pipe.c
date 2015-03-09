/*
 * Unit tests for named pipe functions in Wine
 *
 * Copyright (c) 2002 Dan Kegel
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
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "windef.h"
#include "winbase.h"
#include "winternl.h"
#include "wine/test.h"

#define PIPENAME "\\\\.\\PiPe\\tests_pipe.c"
#define PIPENAME_SPECIAL "\\\\.\\PiPe\\tests->pipe.c"

#define NB_SERVER_LOOPS 8

static HANDLE alarm_event;
static BOOL (WINAPI *pDuplicateTokenEx)(HANDLE,DWORD,LPSECURITY_ATTRIBUTES,
                                        SECURITY_IMPERSONATION_LEVEL,TOKEN_TYPE,PHANDLE);
static DWORD (WINAPI *pQueueUserAPC)(PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData);

static BOOL user_apc_ran;
static void CALLBACK user_apc(ULONG_PTR param)
{
    user_apc_ran = TRUE;
}


enum rpcThreadOp
{
    RPC_READFILE,
    RPC_WRITEFILE,
    RPC_PEEKNAMEDPIPE
};

struct rpcThreadArgs
{
    ULONG_PTR returnValue;
    DWORD lastError;
    enum rpcThreadOp op;
    ULONG_PTR args[6];
};

static DWORD CALLBACK rpcThreadMain(LPVOID arg)
{
    struct rpcThreadArgs *rpcargs = (struct rpcThreadArgs *)arg;
    trace("rpcThreadMain starting\n");
    SetLastError( rpcargs->lastError );

    switch (rpcargs->op)
    {
        case RPC_READFILE:
            rpcargs->returnValue = (ULONG_PTR)ReadFile( (HANDLE)rpcargs->args[0],         /* hFile */
                                                        (LPVOID)rpcargs->args[1],         /* buffer */
                                                        (DWORD)rpcargs->args[2],          /* bytesToRead */
                                                        (LPDWORD)rpcargs->args[3],        /* bytesRead */
                                                        (LPOVERLAPPED)rpcargs->args[4] ); /* overlapped */
            break;

        case RPC_WRITEFILE:
            rpcargs->returnValue = (ULONG_PTR)WriteFile( (HANDLE)rpcargs->args[0],        /* hFile */
                                                         (LPCVOID)rpcargs->args[1],       /* buffer */
                                                         (DWORD)rpcargs->args[2],         /* bytesToWrite */
                                                         (LPDWORD)rpcargs->args[3],       /* bytesWritten */
                                                         (LPOVERLAPPED)rpcargs->args[4] ); /* overlapped */
            break;

        case RPC_PEEKNAMEDPIPE:
            rpcargs->returnValue = (ULONG_PTR)PeekNamedPipe( (HANDLE)rpcargs->args[0],    /* hPipe */
                                                             (LPVOID)rpcargs->args[1],    /* lpvBuffer */
                                                             (DWORD)rpcargs->args[2],     /* cbBuffer */
                                                             (LPDWORD)rpcargs->args[3],   /* lpcbRead */
                                                             (LPDWORD)rpcargs->args[4],   /* lpcbAvail */
                                                             (LPDWORD)rpcargs->args[5] ); /* lpcbMessage */
            break;

        default:
            SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
            rpcargs->returnValue = 0;
            break;
    }

    rpcargs->lastError = GetLastError();
    trace("rpcThreadMain returning\n");
    return 0;
}

/* Runs ReadFile(...) from a different thread */
static BOOL RpcReadFile(HANDLE hFile, LPVOID buffer, DWORD bytesToRead, LPDWORD bytesRead, LPOVERLAPPED overlapped)
{
    struct rpcThreadArgs rpcargs;
    HANDLE thread;
    DWORD threadId, ret;

    rpcargs.returnValue = 0;
    rpcargs.lastError = GetLastError();
    rpcargs.op = RPC_READFILE;
    rpcargs.args[0] = (ULONG_PTR)hFile;
    rpcargs.args[1] = (ULONG_PTR)buffer;
    rpcargs.args[2] = (ULONG_PTR)bytesToRead;
    rpcargs.args[3] = (ULONG_PTR)bytesRead;
    rpcargs.args[4] = (ULONG_PTR)overlapped;

    thread = CreateThread(NULL, 0, rpcThreadMain, (void *)&rpcargs, 0, &threadId);
    ok(thread != NULL, "CreateThread failed. %d\n", GetLastError());
    ret = WaitForSingleObject(thread, INFINITE);
    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject failed with %d.\n", GetLastError());
    CloseHandle(thread);

    SetLastError(rpcargs.lastError);
    return (BOOL)rpcargs.returnValue;
}

/* Runs PeekNamedPipe(...) from a different thread */
static BOOL RpcPeekNamedPipe(HANDLE hPipe, LPVOID lpvBuffer, DWORD cbBuffer,
                             LPDWORD lpcbRead, LPDWORD lpcbAvail, LPDWORD lpcbMessage)
{
    struct rpcThreadArgs rpcargs;
    HANDLE thread;
    DWORD threadId;

    rpcargs.returnValue = 0;
    rpcargs.lastError = GetLastError();
    rpcargs.op = RPC_PEEKNAMEDPIPE;
    rpcargs.args[0] = (ULONG_PTR)hPipe;
    rpcargs.args[1] = (ULONG_PTR)lpvBuffer;
    rpcargs.args[2] = (ULONG_PTR)cbBuffer;
    rpcargs.args[3] = (ULONG_PTR)lpcbRead;
    rpcargs.args[4] = (ULONG_PTR)lpcbAvail;
    rpcargs.args[5] = (ULONG_PTR)lpcbMessage;

    thread = CreateThread(NULL, 0, rpcThreadMain, (void *)&rpcargs, 0, &threadId);
    ok(thread != NULL, "CreateThread failed. %d\n", GetLastError());
    ok(WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0,"WaitForSingleObject failed with %d.\n", GetLastError());
    CloseHandle(thread);

    SetLastError(rpcargs.lastError);
    return (BOOL)rpcargs.returnValue;
}

static void test_CreateNamedPipe(int pipemode)
{
    HANDLE hnp;
    HANDLE hFile;
    static const char obuf[] = "Bit Bucket";
    static const char obuf2[] = "More bits";
    char ibuf[32], *pbuf;
    DWORD written;
    DWORD readden;
    DWORD leftmsg;
    DWORD avail;
    DWORD lpmode;
    BOOL ret;

    if (pipemode == PIPE_TYPE_BYTE)
        trace("test_CreateNamedPipe starting in byte mode\n");
    else
        trace("test_CreateNamedPipe starting in message mode\n");

    /* Wait for nonexistent pipe */
    ret = WaitNamedPipeA(PIPENAME, 2000);
    ok(ret == 0, "WaitNamedPipe returned %d for nonexistent pipe\n", ret);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "wrong error %u\n", GetLastError());

    /* Bad parameter checks */
    hnp = CreateNamedPipeA("not a named pipe", PIPE_ACCESS_DUPLEX, pipemode | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp == INVALID_HANDLE_VALUE && GetLastError() == ERROR_INVALID_NAME,
        "CreateNamedPipe should fail if name doesn't start with \\\\.\\pipe\n");

    if (pipemode == PIPE_TYPE_BYTE)
    {
        /* Bad parameter checks */
        hnp = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_MESSAGE,
            /* nMaxInstances */ 1,
            /* nOutBufSize */ 1024,
            /* nInBufSize */ 1024,
            /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
            /* lpSecurityAttrib */ NULL);
        ok(hnp == INVALID_HANDLE_VALUE && GetLastError() == ERROR_INVALID_PARAMETER,
            "CreateNamedPipe should fail with PIPE_TYPE_BYTE | PIPE_READMODE_MESSAGE\n");
    }

    hnp = CreateNamedPipeA(NULL,
        PIPE_ACCESS_DUPLEX, pipemode | PIPE_WAIT,
        1, 1024, 1024, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(hnp == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PATH_NOT_FOUND,
        "CreateNamedPipe should fail if name is NULL\n");

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile == INVALID_HANDLE_VALUE
        && GetLastError() == ERROR_FILE_NOT_FOUND,
        "connecting to nonexistent named pipe should fail with ERROR_FILE_NOT_FOUND\n");

    /* Functional checks */

    hnp = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX, pipemode | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    ret = WaitNamedPipeA(PIPENAME, 2000);
    ok(ret, "WaitNamedPipe failed (%d)\n", GetLastError());

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError());

    ok(!WaitNamedPipeA(PIPENAME, 1000), "WaitNamedPipe succeeded\n");

    ok(GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %u\n", GetLastError());

    /* don't try to do i/o if one side couldn't be opened, as it hangs */
    if (hFile != INVALID_HANDLE_VALUE) {
        HANDLE hFile2;

        /* Make sure we can read and write a few bytes in both directions */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len\n");
        ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf), "read got %d bytes\n", readden);
        ok(memcmp(obuf, ibuf, written) == 0, "content check\n");

        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf2, sizeof(obuf2), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf2), "write file len\n");
        ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf2), "read got %d bytes\n", readden);
        ok(memcmp(obuf2, ibuf, written) == 0, "content check\n");

        /* Now the same again, but with an additional call to PeekNamedPipe */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len 1\n");
        ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, NULL), "Peek\n");
        ok(readden == sizeof(obuf), "peek 1 got %d bytes\n", readden);
        ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf), "read 1 got %d bytes\n", readden);
        ok(memcmp(obuf, ibuf, written) == 0, "content 1 check\n");

        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf2, sizeof(obuf2), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf2), "write file len 2\n");
        ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, NULL), "Peek\n");
        ok(readden == sizeof(obuf2), "peek 2 got %d bytes\n", readden);
        ok(PeekNamedPipe(hnp, (LPVOID)1, 0, NULL, &readden, NULL), "Peek\n");
        ok(readden == sizeof(obuf2), "peek 2 got %d bytes\n", readden);
        ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf2), "read 2 got %d bytes\n", readden);
        ok(memcmp(obuf2, ibuf, written) == 0, "content 2 check\n");

        /* Test how ReadFile behaves when the buffer is not big enough for the whole message */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf2, sizeof(obuf2), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf2), "write file len\n");
        ok(ReadFile(hFile, ibuf, 4, &readden, NULL), "ReadFile\n");
        ok(readden == 4, "read got %d bytes\n", readden);
        readden = leftmsg = -1;
        ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe\n");
        ok(readden == sizeof(obuf2) - 4, "peek got %d bytes total\n", readden);
        if (pipemode == PIPE_TYPE_BYTE)
            ok(leftmsg == 0, "peek got %d bytes left in message\n", leftmsg);
        else
            ok(leftmsg == sizeof(obuf2) - 4, "peek got %d bytes left in message\n", leftmsg);
        ok(ReadFile(hFile, ibuf + 4, sizeof(ibuf) - 4, &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf2) - 4, "read got %d bytes\n", readden);
        ok(memcmp(obuf2, ibuf, written) == 0, "content check\n");
        readden = leftmsg = -1;
        ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe\n");
        ok(readden == 0, "peek got %d bytes total\n", readden);
        ok(leftmsg == 0, "peek got %d bytes left in message\n", leftmsg);

        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len\n");
        if (pipemode == PIPE_TYPE_BYTE)
        {
            ok(ReadFile(hnp, ibuf, 4, &readden, NULL), "ReadFile\n");
        }
        else
        {
            SetLastError(0xdeadbeef);
            ok(!ReadFile(hnp, ibuf, 4, &readden, NULL), "ReadFile\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error\n");
        }
        ok(readden == 4, "read got %d bytes\n", readden);
        ok(ReadFile(hnp, ibuf + 4, sizeof(ibuf) - 4, &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf) - 4, "read got %d bytes\n", readden);
        ok(memcmp(obuf, ibuf, written) == 0, "content check\n");

        /* Similar to above, but use a read buffer size small enough to read in three parts */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf2, sizeof(obuf2), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf2), "write file len\n");
        if (pipemode == PIPE_TYPE_BYTE)
        {
            ok(ReadFile(hnp, ibuf, 4, &readden, NULL), "ReadFile\n");
            ok(readden == 4, "read got %d bytes\n", readden);
            ok(ReadFile(hnp, ibuf + 4, 4, &readden, NULL), "ReadFile\n");
        }
        else
        {
            SetLastError(0xdeadbeef);
            ok(!ReadFile(hnp, ibuf, 4, &readden, NULL), "ReadFile\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error\n");
            ok(readden == 4, "read got %d bytes\n", readden);
            SetLastError(0xdeadbeef);
            ok(!ReadFile(hnp, ibuf + 4, 4, &readden, NULL), "ReadFile\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error\n");
        }
        ok(readden == 4, "read got %d bytes\n", readden);
        ok(ReadFile(hnp, ibuf + 8, sizeof(ibuf) - 8, &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf2) - 8, "read got %d bytes\n", readden);
        ok(memcmp(obuf2, ibuf, written) == 0, "content check\n");

        /* Tests for sending empty messages */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf, 0, &written, NULL), "WriteFile\n");
        ok(written == 0, "write file len\n");
        if (pipemode != PIPE_TYPE_BYTE)
        {
            ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            ok(readden == 0, "read got %d bytes\n", readden);
        }

        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf, 0, &written, NULL), "WriteFile\n");
        ok(written == 0, "write file len\n");
        if (pipemode != PIPE_TYPE_BYTE)
        {
            ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            ok(readden == 0, "read got %d bytes\n", readden);
        }

        /* similar to above, but with an additional call to PeekNamedPipe inbetween */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf, 0, &written, NULL), "WriteFile\n");
        ok(written == 0, "write file len\n");
        ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, NULL), "Peek\n");
        ok(readden == 0, "peek got %d bytes\n", readden);
        if (pipemode != PIPE_TYPE_BYTE)
        {
            struct rpcThreadArgs rpcargs;
            HANDLE thread;
            DWORD threadId;

            rpcargs.returnValue = 0;
            rpcargs.lastError = GetLastError();
            rpcargs.op = RPC_READFILE;
            rpcargs.args[0] = (ULONG_PTR)hFile;
            rpcargs.args[1] = (ULONG_PTR)ibuf;
            rpcargs.args[2] = (ULONG_PTR)sizeof(ibuf);
            rpcargs.args[3] = (ULONG_PTR)&readden;
            rpcargs.args[4] = (ULONG_PTR)NULL;

            thread = CreateThread(NULL, 0, rpcThreadMain, (void *)&rpcargs, 0, &threadId);
            ok(thread != NULL, "CreateThread failed. %d\n", GetLastError());
            ret = WaitForSingleObject(thread, 200);
            todo_wine
            ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %d instead of %d.\n", ret, WAIT_OBJECT_0);
            if (ret == WAIT_TIMEOUT)
            {
                ok(WriteFile(hnp, obuf, 0, &written, NULL), "WriteFile\n");
                ok(written == 0, "write file len\n");
                ret = WaitForSingleObject(thread, 200);
                ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %d instead of %d.\n", ret, WAIT_OBJECT_0);
            }
            CloseHandle(thread);
            ok((BOOL)rpcargs.returnValue, "ReadFile\n");
            ok(readden == 0, "read got %d bytes\n", readden);
        }

        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf, 0, &written, NULL), "WriteFile\n");
        ok(written == 0, "write file len\n");
        ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, NULL), "Peek\n");
        ok(readden == 0, "peek got %d bytes\n", readden);
        if (pipemode != PIPE_TYPE_BYTE)
        {
            struct rpcThreadArgs rpcargs;
            HANDLE thread;
            DWORD threadId;

            rpcargs.returnValue = 0;
            rpcargs.lastError = GetLastError();
            rpcargs.op = RPC_READFILE;
            rpcargs.args[0] = (ULONG_PTR)hnp;
            rpcargs.args[1] = (ULONG_PTR)ibuf;
            rpcargs.args[2] = (ULONG_PTR)sizeof(ibuf);
            rpcargs.args[3] = (ULONG_PTR)&readden;
            rpcargs.args[4] = (ULONG_PTR)NULL;

            thread = CreateThread(NULL, 0, rpcThreadMain, (void *)&rpcargs, 0, &threadId);
            ok(thread != NULL, "CreateThread failed. %d\n", GetLastError());
            ret = WaitForSingleObject(thread, 200);
            todo_wine
            ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %d instead of %d.\n", ret, WAIT_OBJECT_0);
            if (ret == WAIT_TIMEOUT)
            {
                ok(WriteFile(hFile, obuf, 0, &written, NULL), "WriteFile\n");
                ok(written == 0, "write file len\n");
                ret = WaitForSingleObject(thread, 200);
                ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %d instead of %d.\n", ret, WAIT_OBJECT_0);
            }
            CloseHandle(thread);
            ok((BOOL)rpcargs.returnValue, "ReadFile\n");
            ok(readden == 0, "read got %d bytes\n", readden);
        }

        /* similar to above, but now with PeekNamedPipe and multiple messages */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf, 0, &written, NULL), "WriteFile\n");
        ok(written == 0, "write file len\n");
        ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len\n");
        ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "Peek\n");
        ok(readden == sizeof(obuf), "peek got %d bytes\n", readden);
        ok(leftmsg == 0, "peek got %d bytes left in msg\n", leftmsg);
        ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "Peek\n");
        ok(readden == sizeof(obuf), "peek got %d bytes\n", readden);
        if (pipemode != PIPE_TYPE_BYTE)
            todo_wine
            ok(leftmsg == 0, "peek got %d bytes left in msg\n", leftmsg);
        else
            ok(leftmsg == 0, "peek got %d bytes left in msg\n", leftmsg);
        ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf), "read got %d bytes\n", readden);
        ok(memcmp(obuf, ibuf, sizeof(obuf)) == 0, "content check\n");

        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf2, 0, &written, NULL), "WriteFile\n");
        ok(written == 0, "write file len\n");
        ok(WriteFile(hFile, obuf2, sizeof(obuf2), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf2), "write file len\n");
        ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "Peek\n");
        ok(readden == sizeof(obuf2), "peek got %d bytes\n", readden);
        ok(leftmsg == 0, "peek got %d bytes left in msg\n", leftmsg);
        ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "Peek\n");
        ok(readden == sizeof(obuf2), "peek got %d bytes\n", readden);
        if (pipemode != PIPE_TYPE_BYTE)
            todo_wine
            ok(leftmsg == 0, "peek got %d bytes left in msg\n", leftmsg);
        else
            ok(leftmsg == 0, "peek got %d bytes left in msg\n", leftmsg);
        ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        if (pipemode != PIPE_TYPE_BYTE)
        {
            todo_wine
            ok(readden == 0, "read got %d bytes\n", readden);
            if (readden == 0)
                ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        }
        ok(readden == sizeof(obuf2), "read got %d bytes\n", readden);
        ok(memcmp(obuf2, ibuf, sizeof(obuf2)) == 0, "content check\n");

        /* Test reading of multiple writes */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile3a\n");
        ok(written == sizeof(obuf), "write file len 3a\n");
        ok(WriteFile(hnp, obuf2, sizeof(obuf2), &written, NULL), " WriteFile3b\n");
        ok(written == sizeof(obuf2), "write file len 3b\n");
        ok(PeekNamedPipe(hFile, ibuf, sizeof(ibuf), &readden, &avail, NULL), "Peek3\n");
        if (pipemode == PIPE_TYPE_BYTE) {
            todo_wine ok(readden == sizeof(obuf) + sizeof(obuf2), "peek3 got %d bytes\n", readden);
        }
        else
        {
            ok(readden == sizeof(obuf), "peek3 got %d bytes\n", readden);
        }
        ok(avail == sizeof(obuf) + sizeof(obuf2), "peek3 got %d bytes available\n", avail);
        pbuf = ibuf;
        ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "pipe content 3a check\n");
        if (pipemode == PIPE_TYPE_BYTE && readden >= sizeof(obuf)+sizeof(obuf2)) {
            pbuf += sizeof(obuf);
            ok(memcmp(obuf2, pbuf, sizeof(obuf2)) == 0, "pipe content 3b check\n");
        }
        ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf) + sizeof(obuf2), "read 3 got %d bytes\n", readden);
        pbuf = ibuf;
        ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 3a check\n");
        pbuf += sizeof(obuf);
        ok(memcmp(obuf2, pbuf, sizeof(obuf2)) == 0, "content 3b check\n");

        /* Multiple writes in the reverse direction */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile4a\n");
        ok(written == sizeof(obuf), "write file len 4a\n");
        ok(WriteFile(hFile, obuf2, sizeof(obuf2), &written, NULL), " WriteFile4b\n");
        ok(written == sizeof(obuf2), "write file len 4b\n");
        ok(PeekNamedPipe(hnp, ibuf, sizeof(ibuf), &readden, &avail, NULL), "Peek4\n");
        if (pipemode == PIPE_TYPE_BYTE) {
            todo_wine ok(readden == sizeof(obuf) + sizeof(obuf2), "peek4 got %d bytes\n", readden);
        }
        else
        {
            ok(readden == sizeof(obuf), "peek4 got %d bytes\n", readden);
        }
        ok(avail == sizeof(obuf) + sizeof(obuf2), "peek4 got %d bytes available\n", avail);
        pbuf = ibuf;
        ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "pipe content 4a check\n");
        if (pipemode == PIPE_TYPE_BYTE && readden >= sizeof(obuf)+sizeof(obuf2)) {
            pbuf += sizeof(obuf);
            ok(memcmp(obuf2, pbuf, sizeof(obuf2)) == 0, "pipe content 4b check\n");
        }
        ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        if (pipemode == PIPE_TYPE_BYTE) {
            ok(readden == sizeof(obuf) + sizeof(obuf2), "read 4 got %d bytes\n", readden);
        }
        else {
            ok(readden == sizeof(obuf), "read 4 got %d bytes\n", readden);
        }
        pbuf = ibuf;
        ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 4a check\n");
        if (pipemode == PIPE_TYPE_BYTE) {
            pbuf += sizeof(obuf);
            ok(memcmp(obuf2, pbuf, sizeof(obuf2)) == 0, "content 4b check\n");
        }

        /* Test reading of multiple writes after a mode change
          (CreateFile always creates a byte mode pipe) */
        lpmode = PIPE_READMODE_MESSAGE;
        if (pipemode == PIPE_TYPE_BYTE) {
            /* trying to change the client end of a byte pipe to message mode should fail */
            ok(!SetNamedPipeHandleState(hFile, &lpmode, NULL, NULL), "Change mode\n");
        }
        else {
            ok(SetNamedPipeHandleState(hFile, &lpmode, NULL, NULL), "Change mode\n");
        
            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile5a\n");
            ok(written == sizeof(obuf), "write file len 3a\n");
            ok(WriteFile(hnp, obuf2, sizeof(obuf2), &written, NULL), " WriteFile5b\n");
            ok(written == sizeof(obuf2), "write file len 3b\n");
            ok(PeekNamedPipe(hFile, ibuf, sizeof(ibuf), &readden, &avail, NULL), "Peek5\n");
            ok(readden == sizeof(obuf), "peek5 got %d bytes\n", readden);
            ok(avail == sizeof(obuf) + sizeof(obuf2), "peek5 got %d bytes available\n", avail);
            pbuf = ibuf;
            ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 5a check\n");
            ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            ok(readden == sizeof(obuf), "read 5 got %d bytes\n", readden);
            pbuf = ibuf;
            ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 5a check\n");
            if (readden <= sizeof(obuf))
                ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");

            /* Multiple writes in the reverse direction */
            /* the write of obuf2 from write4 should still be in the buffer */
            ok(PeekNamedPipe(hnp, ibuf, sizeof(ibuf), &readden, &avail, NULL), "Peek6a\n");
            ok(readden == sizeof(obuf2), "peek6a got %d bytes\n", readden);
            ok(avail == sizeof(obuf2), "peek6a got %d bytes available\n", avail);
            if (avail > 0) {
                ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
                ok(readden == sizeof(obuf2), "read 6a got %d bytes\n", readden);
                pbuf = ibuf;
                ok(memcmp(obuf2, pbuf, sizeof(obuf2)) == 0, "content 6a check\n");
            }
            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile6a\n");
            ok(written == sizeof(obuf), "write file len 6a\n");
            ok(WriteFile(hFile, obuf2, sizeof(obuf2), &written, NULL), " WriteFile6b\n");
            ok(written == sizeof(obuf2), "write file len 6b\n");
            ok(PeekNamedPipe(hnp, ibuf, sizeof(ibuf), &readden, &avail, NULL), "Peek6\n");
            ok(readden == sizeof(obuf), "peek6 got %d bytes\n", readden);
            ok(avail == sizeof(obuf) + sizeof(obuf2), "peek6b got %d bytes available\n", avail);
            pbuf = ibuf;
            ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 6a check\n");
            ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            ok(readden == sizeof(obuf), "read 6b got %d bytes\n", readden);
            pbuf = ibuf;
            ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 6a check\n");
            if (readden <= sizeof(obuf))
                ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");

            /* Tests for sending empty messages */
            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hnp, obuf, 0, &written, NULL), "WriteFile\n");
            ok(written == 0, "write file len\n");
            ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            ok(readden == 0, "read got %d bytes\n", readden);

            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hFile, obuf, 0, &written, NULL), "WriteFile\n");
            ok(written == 0, "write file len\n");
            ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            ok(readden == 0, "read got %d bytes\n", readden);

            /* similar to above, but with an additional call to PeekNamedPipe inbetween */
            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hnp, obuf, 0, &written, NULL), "WriteFile\n");
            ok(written == 0, "write file len\n");
            ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, NULL), "Peek\n");
            ok(readden == 0, "peek got %d bytes\n", readden);
            {
                struct rpcThreadArgs rpcargs;
                HANDLE thread;
                DWORD threadId;

                rpcargs.returnValue = 0;
                rpcargs.lastError = GetLastError();
                rpcargs.op = RPC_READFILE;
                rpcargs.args[0] = (ULONG_PTR)hFile;
                rpcargs.args[1] = (ULONG_PTR)ibuf;
                rpcargs.args[2] = (ULONG_PTR)sizeof(ibuf);
                rpcargs.args[3] = (ULONG_PTR)&readden;
                rpcargs.args[4] = (ULONG_PTR)NULL;

                thread = CreateThread(NULL, 0, rpcThreadMain, (void *)&rpcargs, 0, &threadId);
                ok(thread != NULL, "CreateThread failed. %d\n", GetLastError());
                ret = WaitForSingleObject(thread, 200);
                todo_wine
                ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %d instead of %d.\n", ret, WAIT_OBJECT_0);
                if (ret == WAIT_TIMEOUT)
                {
                    ok(WriteFile(hnp, obuf, 0, &written, NULL), "WriteFile\n");
                    ok(written == 0, "write file len\n");
                    ret = WaitForSingleObject(thread, 200);
                    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %d instead of %d.\n", ret, WAIT_OBJECT_0);
                }
                CloseHandle(thread);
                ok((BOOL)rpcargs.returnValue, "ReadFile\n");
                ok(readden == 0, "read got %d bytes\n", readden);
            }

            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hFile, obuf, 0, &written, NULL), "WriteFile\n");
            ok(written == 0, "write file len\n");
            ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, NULL), "Peek\n");
            ok(readden == 0, "peek got %d bytes\n", readden);
            {
                struct rpcThreadArgs rpcargs;
                HANDLE thread;
                DWORD threadId;

                rpcargs.returnValue = 0;
                rpcargs.lastError = GetLastError();
                rpcargs.op = RPC_READFILE;
                rpcargs.args[0] = (ULONG_PTR)hnp;
                rpcargs.args[1] = (ULONG_PTR)ibuf;
                rpcargs.args[2] = (ULONG_PTR)sizeof(ibuf);
                rpcargs.args[3] = (ULONG_PTR)&readden;
                rpcargs.args[4] = (ULONG_PTR)NULL;

                thread = CreateThread(NULL, 0, rpcThreadMain, (void *)&rpcargs, 0, &threadId);
                ok(thread != NULL, "CreateThread failed. %d\n", GetLastError());
                ret = WaitForSingleObject(thread, 200);
                todo_wine
                ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %d instead of %d.\n", ret, WAIT_OBJECT_0);
                if (ret == WAIT_TIMEOUT)
                {
                    ok(WriteFile(hFile, obuf, 0, &written, NULL), "WriteFile\n");
                    ok(written == 0, "write file len\n");
                    ret = WaitForSingleObject(thread, 200);
                    ok(ret == WAIT_OBJECT_0, "WaitForSingleObject returned %d instead of %d.\n", ret, WAIT_OBJECT_0);
                }
                CloseHandle(thread);
                ok((BOOL)rpcargs.returnValue, "ReadFile\n");
                ok(readden == 0, "read got %d bytes\n", readden);
            }

            /* similar to above, but now with PeekNamedPipe and multiple messages */
            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hnp, obuf, 0, &written, NULL), "WriteFile\n");
            ok(written == 0, "write file len\n");
            ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
            ok(written == sizeof(obuf), "write file len\n");
            ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "Peek\n");
            ok(readden == sizeof(obuf), "peek got %d bytes\n", readden);
            ok(leftmsg == 0, "peek got %d bytes left in msg\n", leftmsg);
            ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "Peek\n");
            ok(readden == sizeof(obuf), "peek got %d bytes\n", readden);
            todo_wine
            ok(leftmsg == 0, "peek got %d bytes left in msg\n", leftmsg);
            ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            todo_wine
            ok(readden == 0, "read got %d bytes\n", readden);
            if (readden == 0)
                ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            ok(readden == sizeof(obuf), "read got %d bytes\n", readden);
            ok(memcmp(obuf, ibuf, sizeof(obuf)) == 0, "content check\n");

            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hFile, obuf2, 0, &written, NULL), "WriteFile\n");
            ok(written == 0, "write file len\n");
            ok(WriteFile(hFile, obuf2, sizeof(obuf2), &written, NULL), "WriteFile\n");
            ok(written == sizeof(obuf2), "write file len\n");
            ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "Peek\n");
            ok(readden == sizeof(obuf2), "peek got %d bytes\n", readden);
            ok(leftmsg == 0, "peek got %d bytes left in msg\n", leftmsg);
            ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "Peek\n");
            ok(readden == sizeof(obuf2), "peek got %d bytes\n", readden);
            todo_wine
            ok(leftmsg == 0, "peek got %d bytes left in msg\n", leftmsg);
            ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            todo_wine
            ok(readden == 0, "read got %d bytes\n", readden);
            if (readden == 0)
                ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            ok(readden == sizeof(obuf2), "read got %d bytes\n", readden);
            ok(memcmp(obuf2, ibuf, sizeof(obuf2)) == 0, "content check\n");

            /* Test how ReadFile behaves when the buffer is not big enough for the whole message */
            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hnp, obuf2, sizeof(obuf2), &written, NULL), "WriteFile 7\n");
            ok(written == sizeof(obuf2), "write file len 7\n");
            SetLastError(0xdeadbeef);
            ok(!ReadFile(hFile, ibuf, 4, &readden, NULL), "ReadFile 7\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error 7\n");
            ok(readden == 4, "read got %d bytes 7\n", readden);
            ok(ReadFile(hFile, ibuf + 4, sizeof(ibuf) - 4, &readden, NULL), "ReadFile 7\n");
            ok(readden == sizeof(obuf2) - 4, "read got %d bytes 7\n", readden);
            ok(memcmp(obuf2, ibuf, written) == 0, "content check 7\n");

            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile 8\n");
            ok(written == sizeof(obuf), "write file len 8\n");
            SetLastError(0xdeadbeef);
            ok(!ReadFile(hnp, ibuf, 4, &readden, NULL), "ReadFile 8\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error 8\n");
            ok(readden == 4, "read got %d bytes 8\n", readden);
            ok(ReadFile(hnp, ibuf + 4, sizeof(ibuf) - 4, &readden, NULL), "ReadFile 8\n");
            ok(readden == sizeof(obuf) - 4, "read got %d bytes 8\n", readden);
            ok(memcmp(obuf, ibuf, written) == 0, "content check 8\n");

            /* The following test shows that when doing a partial read of a message, the rest
             * is still in the pipe, and can be received from a second thread. This shows
             * especially that the content is _not_ stored in thread-local-storage until it is
             * completely transmitted. The same method works even across multiple processes. */
            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile 9\n");
            ok(written == sizeof(obuf), "write file len 9\n");
            ok(WriteFile(hnp, obuf2, sizeof(obuf2), &written, NULL), "WriteFile 9\n");
            ok(written == sizeof(obuf2), "write file len 9\n");
            readden = leftmsg = -1;
            ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe 9\n");
            ok(readden == sizeof(obuf) + sizeof(obuf2), "peek got %d bytes total 9\n", readden);
            ok(leftmsg == sizeof(obuf), "peek got %d bytes left in message 9\n", leftmsg);
            readden = leftmsg = -1;
            ok(RpcPeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "RpcPeekNamedPipe 9\n");
            ok(readden == sizeof(obuf) + sizeof(obuf2), "peek got %d bytes total 9\n", readden);
            ok(leftmsg == sizeof(obuf), "peek got %d bytes left in message 9\n", leftmsg);
            SetLastError(0xdeadbeef);
            ok(!ReadFile(hFile, ibuf, 4, &readden, NULL), "ReadFile 9\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error 9\n");
            ok(readden == 4, "read got %d bytes 9\n", readden);
            SetLastError(0xdeadbeef);
            ret = RpcReadFile(hFile, ibuf + 4, 4, &readden, NULL);
            ok(!ret, "RpcReadFile 9\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error 9\n");
            ok(readden == 4, "read got %d bytes 9\n", readden);
            readden = leftmsg = -1;
            ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe 9\n");
            ok(readden == sizeof(obuf) - 8 + sizeof(obuf2), "peek got %d bytes total 9\n", readden);
            ok(leftmsg == sizeof(obuf) - 8, "peek got %d bytes left in message 9\n", leftmsg);
            readden = leftmsg = -1;
            ok(RpcPeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "RpcPeekNamedPipe 9\n");
            ok(readden == sizeof(obuf) - 8 + sizeof(obuf2), "peek got %d bytes total 9\n", readden);
            ok(leftmsg == sizeof(obuf) - 8, "peek got %d bytes left in message 9\n", leftmsg);
            ret = RpcReadFile(hFile, ibuf + 8, sizeof(ibuf), &readden, NULL);
            ok(ret, "RpcReadFile 9\n");
            ok(readden == sizeof(obuf) - 8, "read got %d bytes 9\n", readden);
            ok(memcmp(obuf, ibuf, sizeof(obuf)) == 0, "content check 9\n");
            if (readden <= sizeof(obuf) - 8) /* blocks forever if second part was already received */
            {
                memset(ibuf, 0, sizeof(ibuf));
                readden = leftmsg = -1;
                ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe 9\n");
                ok(readden == sizeof(obuf2), "peek got %d bytes total 9\n", readden);
                ok(leftmsg == sizeof(obuf2), "peek got %d bytes left in message 9\n", leftmsg);
                readden = leftmsg = -1;
                ok(RpcPeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "RpcPeekNamedPipe 9\n");
                ok(readden == sizeof(obuf2), "peek got %d bytes total 9\n", readden);
                ok(leftmsg == sizeof(obuf2), "peek got %d bytes left in message 9\n", leftmsg);
                SetLastError(0xdeadbeef);
                ret = RpcReadFile(hFile, ibuf, 4, &readden, NULL);
                ok(!ret, "RpcReadFile 9\n");
                ok(GetLastError() == ERROR_MORE_DATA, "wrong error 9\n");
                ok(readden == 4, "read got %d bytes 9\n", readden);
                SetLastError(0xdeadbeef);
                ok(!ReadFile(hFile, ibuf + 4, 4, &readden, NULL), "ReadFile 9\n");
                ok(GetLastError() == ERROR_MORE_DATA, "wrong error 9\n");
                ok(readden == 4, "read got %d bytes 9\n", readden);
                readden = leftmsg = -1;
                ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe 9\n");
                ok(readden == sizeof(obuf2) - 8, "peek got %d bytes total 9\n", readden);
                ok(leftmsg == sizeof(obuf2) - 8, "peek got %d bytes left in message 9\n", leftmsg);
                readden = leftmsg = -1;
                ok(RpcPeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "RpcPeekNamedPipe 9\n");
                ok(readden == sizeof(obuf2) - 8, "peek got %d bytes total 9\n", readden);
                ok(leftmsg == sizeof(obuf2) - 8, "peek got %d bytes left in message 9\n", leftmsg);
                ret = RpcReadFile(hFile, ibuf + 8, sizeof(ibuf), &readden, NULL);
                ok(ret, "RpcReadFile 9\n");
                ok(readden == sizeof(obuf2) - 8, "read got %d bytes 9\n", readden);
                ok(memcmp(obuf2, ibuf, sizeof(obuf2)) == 0, "content check 9\n");
            }
            readden = leftmsg = -1;
            ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe 9\n");
            ok(readden == 0, "peek got %d bytes total 9\n", readden);
            ok(leftmsg == 0, "peek got %d bytes left in message 9\n", leftmsg);
            readden = leftmsg = -1;
            ok(RpcPeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "RpcPeekNamedPipe 9\n");
            ok(readden == 0, "peek got %d bytes total 9\n", readden);
            ok(leftmsg == 0, "peek got %d bytes left in message 9\n", leftmsg);

            /* Now the reverse direction */
            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hFile, obuf2, sizeof(obuf2), &written, NULL), "WriteFile 10\n");
            ok(written == sizeof(obuf2), "write file len 10\n");
            ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile 10\n");
            ok(written == sizeof(obuf), "write file len 10\n");
            readden = leftmsg = -1;
            ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe 10\n");
            ok(readden == sizeof(obuf) + sizeof(obuf2), "peek got %d bytes total 10\n", readden);
            ok(leftmsg == sizeof(obuf2), "peek got %d bytes left in message 10\n", leftmsg);
            readden = leftmsg = -1;
            ok(RpcPeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "RpcPeekNamedPipe 10\n");
            ok(readden == sizeof(obuf) + sizeof(obuf2), "peek got %d bytes total 10\n", readden);
            ok(leftmsg == sizeof(obuf2), "peek got %d bytes left in message 10\n", leftmsg);
            SetLastError(0xdeadbeef);
            ok(!ReadFile(hnp, ibuf, 4, &readden, NULL), "ReadFile 10\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error 10\n");
            ok(readden == 4, "read got %d bytes 10\n", readden);
            SetLastError(0xdeadbeef);
            ret = RpcReadFile(hnp, ibuf + 4, 4, &readden, NULL);
            ok(!ret, "RpcReadFile 10\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error 10\n");
            ok(readden == 4, "read got %d bytes 10\n", readden);
            readden = leftmsg = -1;
            ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe 10\n");
            ok(readden == sizeof(obuf2) - 8 + sizeof(obuf), "peek got %d bytes total 10\n", readden);
            ok(leftmsg == sizeof(obuf2) - 8, "peek got %d bytes left in message 10\n", leftmsg);
            readden = leftmsg = -1;
            ok(RpcPeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "RpcPeekNamedPipe 10\n");
            ok(readden == sizeof(obuf2) - 8 + sizeof(obuf), "peek got %d bytes total 10\n", readden);
            ok(leftmsg == sizeof(obuf2) - 8, "peek got %d bytes left in message 10\n", leftmsg);
            ret = RpcReadFile(hnp, ibuf + 8, sizeof(ibuf), &readden, NULL);
            ok(ret, "RpcReadFile 10\n");
            ok(readden == sizeof(obuf2) - 8, "read got %d bytes 10\n", readden);
            ok(memcmp(obuf2, ibuf, sizeof(obuf2)) == 0, "content check 10\n");
            if (readden <= sizeof(obuf2) - 8) /* blocks forever if second part was already received */
            {
                memset(ibuf, 0, sizeof(ibuf));
                readden = leftmsg = -1;
                ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe 10\n");
                ok(readden == sizeof(obuf), "peek got %d bytes total 10\n", readden);
                ok(leftmsg == sizeof(obuf), "peek got %d bytes left in message 10\n", leftmsg);
                readden = leftmsg = -1;
                ok(RpcPeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "RpcPeekNamedPipe 10\n");
                ok(readden == sizeof(obuf), "peek got %d bytes total 10\n", readden);
                ok(leftmsg == sizeof(obuf), "peek got %d bytes left in message 10\n", leftmsg);
                SetLastError(0xdeadbeef);
                ret = RpcReadFile(hnp, ibuf, 4, &readden, NULL);
                ok(!ret, "RpcReadFile 10\n");
                ok(GetLastError() == ERROR_MORE_DATA, "wrong error 10\n");
                ok(readden == 4, "read got %d bytes 10\n", readden);
                SetLastError(0xdeadbeef);
                ok(!ReadFile(hnp, ibuf + 4, 4, &readden, NULL), "ReadFile 10\n");
                ok(GetLastError() == ERROR_MORE_DATA, "wrong error 10\n");
                ok(readden == 4, "read got %d bytes 10\n", readden);
                readden = leftmsg = -1;
                ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe 10\n");
                ok(readden == sizeof(obuf) - 8, "peek got %d bytes total 10\n", readden);
                ok(leftmsg == sizeof(obuf) - 8, "peek got %d bytes left in message 10\n", leftmsg);
                readden = leftmsg = -1;
                ok(RpcPeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "RpcPeekNamedPipe 10\n");
                ok(readden == sizeof(obuf) - 8, "peek got %d bytes total 10\n", readden);
                ok(leftmsg == sizeof(obuf) - 8, "peek got %d bytes left in message 10\n", leftmsg);
                ret = RpcReadFile(hnp, ibuf + 8, sizeof(ibuf), &readden, NULL);
                ok(ret, "RpcReadFile 10\n");
                ok(readden == sizeof(obuf) - 8, "read got %d bytes 10\n", readden);
                ok(memcmp(obuf, ibuf, sizeof(obuf)) == 0, "content check 10\n");
            }
            readden = leftmsg = -1;
            ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe 10\n");
            ok(readden == 0, "peek got %d bytes total 10\n", readden);
            ok(leftmsg == 0, "peek got %d bytes left in message 10\n", leftmsg);
            readden = leftmsg = -1;
            ok(RpcPeekNamedPipe(hnp, NULL, 0, NULL, &readden, &leftmsg), "RpcPeekNamedPipe 10\n");
            ok(readden == 0, "peek got %d bytes total 10\n", readden);
            ok(leftmsg == 0, "peek got %d bytes left in message 10\n", leftmsg);

        }

        /* Test behaviour for very huge messages (which don't fit completely in the buffer) */
        {
            static char big_obuf[512 * 1024];
            static char big_ibuf[512 * 1024];
            struct rpcThreadArgs rpcargs;
            HANDLE thread;
            DWORD threadId;
            memset(big_obuf, 0xAA, sizeof(big_obuf));

            /* Ensure that both pipes are empty before we continue with the next test */
            while (PeekNamedPipe(hFile, NULL, 0, NULL, &readden, NULL) && readden > 0)
                ok(ReadFile(hFile, big_ibuf, sizeof(big_ibuf), &readden, NULL) ||
                   GetLastError() == ERROR_MORE_DATA, "ReadFile\n");

            while (PeekNamedPipe(hnp, NULL, 0, NULL, &readden, NULL) && readden > 0)
                ok(ReadFile(hnp, big_ibuf, sizeof(big_ibuf), &readden, NULL) ||
                   GetLastError() == ERROR_MORE_DATA, "ReadFile\n");

            readden = leftmsg = -1;
            ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe\n");
            ok(readden == 0, "peek got %d bytes total\n", readden);
            ok(leftmsg == 0, "peek got %d bytes left in message\n", leftmsg);

            /* transmit big message, receive with buffer of equal size */
            memset(big_ibuf, 0, sizeof(big_ibuf));
            rpcargs.returnValue = 0;
            rpcargs.lastError = GetLastError();
            rpcargs.op = RPC_WRITEFILE;
            rpcargs.args[0] = (ULONG_PTR)hnp;
            rpcargs.args[1] = (ULONG_PTR)big_obuf;
            rpcargs.args[2] = (ULONG_PTR)sizeof(big_obuf);
            rpcargs.args[3] = (ULONG_PTR)&written;
            rpcargs.args[4] = (ULONG_PTR)NULL;

            thread = CreateThread(NULL, 0, rpcThreadMain, (void *)&rpcargs, 0, &threadId);
            ok(thread != NULL, "CreateThread failed. %d\n", GetLastError());
            ret = WaitForSingleObject(thread, 200);
            ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned %d instead of %d.\n", ret, WAIT_TIMEOUT);
            ok(ReadFile(hFile, big_ibuf, sizeof(big_ibuf), &readden, NULL), "ReadFile\n");
            todo_wine
            ok(readden == sizeof(big_obuf), "read got %d bytes\n", readden);
            todo_wine
            ok(memcmp(big_ibuf, big_obuf, sizeof(big_obuf)) == 0, "content check\n");
            do
            {
                ret = WaitForSingleObject(thread, 1);
                while (PeekNamedPipe(hFile, NULL, 0, NULL, &readden, NULL) && readden > 0)
                    ok(ReadFile(hFile, big_ibuf, sizeof(big_ibuf), &readden, NULL) ||
                       GetLastError() == ERROR_MORE_DATA, "ReadFile\n");
            }
            while (ret == WAIT_TIMEOUT);
            ok(WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed with %d.\n", GetLastError());
            ok((BOOL)rpcargs.returnValue, "WriteFile\n");
            ok(written == sizeof(big_obuf), "write file len\n");
            CloseHandle(thread);

            readden = leftmsg = -1;
            ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe\n");
            ok(readden == 0, "peek got %d bytes total\n", readden);
            ok(leftmsg == 0, "peek got %d bytes left in message\n", leftmsg);

            /* same as above, but receive as multiple parts */
            memset(big_ibuf, 0, sizeof(big_ibuf));
            rpcargs.returnValue = 0;
            rpcargs.lastError = GetLastError();

            thread = CreateThread(NULL, 0, rpcThreadMain, (void *)&rpcargs, 0, &threadId);
            ok(thread != NULL, "CreateThread failed. %d\n", GetLastError());
            ret = WaitForSingleObject(thread, 200);
            ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned %d instead of %d.\n", ret, WAIT_TIMEOUT);
            if (pipemode == PIPE_TYPE_BYTE)
            {
                ok(ReadFile(hFile, big_ibuf, 32, &readden, NULL), "ReadFile\n");
                ok(readden == 32, "read got %d bytes\n", readden);
                ok(ReadFile(hFile, big_ibuf + 32, 32, &readden, NULL), "ReadFile\n");
            }
            else
            {
                SetLastError(0xdeadbeef);
                ok(!ReadFile(hFile, big_ibuf, 32, &readden, NULL), "ReadFile\n");
                ok(GetLastError() == ERROR_MORE_DATA, "wrong error\n");
                ok(readden == 32, "read got %d bytes\n", readden);
                SetLastError(0xdeadbeef);
                ok(!ReadFile(hFile, big_ibuf + 32, 32, &readden, NULL), "ReadFile\n");
                ok(GetLastError() == ERROR_MORE_DATA, "wrong error\n");
            }
            ok(readden == 32, "read got %d bytes\n", readden);
            ok(ReadFile(hFile, big_ibuf + 64, sizeof(big_ibuf) - 64, &readden, NULL), "ReadFile\n");
            todo_wine
            ok(readden == sizeof(big_obuf) - 64, "read got %d bytes\n", readden);
            todo_wine
            ok(memcmp(big_ibuf, big_obuf, sizeof(big_obuf)) == 0, "content check\n");
            do
            {
                ret = WaitForSingleObject(thread, 1);
                while (PeekNamedPipe(hFile, NULL, 0, NULL, &readden, NULL) && readden > 0)
                    ok(ReadFile(hFile, big_ibuf, sizeof(big_ibuf), &readden, NULL) ||
                       GetLastError() == ERROR_MORE_DATA, "ReadFile\n");
            }
            while (ret == WAIT_TIMEOUT);
            ok(WaitForSingleObject(thread, INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject failed with %d.\n", GetLastError());
            ok((BOOL)rpcargs.returnValue, "WriteFile\n");
            ok(written == sizeof(big_obuf), "write file len\n");
            CloseHandle(thread);

            readden = leftmsg = -1;
            ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, &leftmsg), "PeekNamedPipe\n");
            ok(readden == 0, "peek got %d bytes total\n", readden);
            ok(leftmsg == 0, "peek got %d bytes left in message\n", leftmsg);
        }

        /* Picky conformance tests */

        /* Verify that you can't connect to pipe again
         * until server calls DisconnectNamedPipe+ConnectNamedPipe
         * or creates a new pipe
         * case 1: other client not yet closed
         */
        hFile2 = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        ok(hFile2 == INVALID_HANDLE_VALUE,
            "connecting to named pipe after other client closes but before DisconnectNamedPipe should fail\n");
        ok(GetLastError() == ERROR_PIPE_BUSY,
            "connecting to named pipe before other client closes should fail with ERROR_PIPE_BUSY\n");

        ok(CloseHandle(hFile), "CloseHandle\n");

        /* case 2: other client already closed */
        hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        ok(hFile == INVALID_HANDLE_VALUE,
            "connecting to named pipe after other client closes but before DisconnectNamedPipe should fail\n");
        ok(GetLastError() == ERROR_PIPE_BUSY,
            "connecting to named pipe after other client closes but before DisconnectNamedPipe should fail with ERROR_PIPE_BUSY\n");

        ok(DisconnectNamedPipe(hnp), "DisconnectNamedPipe\n");

        /* case 3: server has called DisconnectNamedPipe but not ConnectNamed Pipe */
        hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
        ok(hFile == INVALID_HANDLE_VALUE,
            "connecting to named pipe after other client closes but before DisconnectNamedPipe should fail\n");
        ok(GetLastError() == ERROR_PIPE_BUSY,
            "connecting to named pipe after other client closes but before ConnectNamedPipe should fail with ERROR_PIPE_BUSY\n");

        /* to be complete, we'd call ConnectNamedPipe here and loop,
         * but by default that's blocking, so we'd either have
         * to turn on the uncommon nonblocking mode, or
         * use another thread.
         */
    }

    ok(CloseHandle(hnp), "CloseHandle\n");

    hnp = CreateNamedPipeA(PIPENAME_SPECIAL, PIPE_ACCESS_DUPLEX, pipemode | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe with special characters failed\n");
    ok(CloseHandle(hnp), "CloseHandle\n");

    trace("test_CreateNamedPipe returning\n");
}

static void test_CreateNamedPipe_instances_must_match(void)
{
    HANDLE hnp, hnp2;

    /* Check no mismatch */
    hnp = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 2,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hnp2 = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 2,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp2 != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    ok(CloseHandle(hnp), "CloseHandle\n");
    ok(CloseHandle(hnp2), "CloseHandle\n");

    /* Check nMaxInstances */
    hnp = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hnp2 = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp2 == INVALID_HANDLE_VALUE
        && GetLastError() == ERROR_PIPE_BUSY, "nMaxInstances not obeyed\n");

    ok(CloseHandle(hnp), "CloseHandle\n");

    /* Check PIPE_ACCESS_* */
    hnp = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 2,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hnp2 = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 2,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp2 == INVALID_HANDLE_VALUE
        && GetLastError() == ERROR_ACCESS_DENIED, "PIPE_ACCESS_* mismatch allowed\n");

    ok(CloseHandle(hnp), "CloseHandle\n");

    /* check everything else */
    hnp = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 4,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hnp2 = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE,
        /* nMaxInstances */ 3,
        /* nOutBufSize */ 102,
        /* nInBufSize */ 24,
        /* nDefaultWait */ 1234,
        /* lpSecurityAttrib */ NULL);
    ok(hnp2 != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    ok(CloseHandle(hnp), "CloseHandle\n");
    ok(CloseHandle(hnp2), "CloseHandle\n");
}

static void test_CloseNamedPipe(void)
{
    HANDLE hnp;
    HANDLE hFile;
    static const char obuf[] = "Bit Bucket";
    char ibuf[32];
    DWORD written;
    DWORD readden;

    hnp = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
                           PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                           /* nMaxInstances */ 1,
                           /* nOutBufSize */ 1024,
                           /* nInBufSize */ 1024,
                           /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
                           /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError());

    /* don't try to do i/o if one side couldn't be opened, as it hangs */
    if (hFile != INVALID_HANDLE_VALUE)
    {
        /* Make sure we can read and write a few bytes in both directions */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len 1\n");
        ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, NULL), "Peek\n");
        ok(readden == sizeof(obuf), "got %d bytes\n", readden);

        /* close server end without disconnecting */
        ok(CloseHandle(hnp), "CloseHandle() failed: %08x\n", GetLastError());

        ok(ReadFile(hFile, ibuf, 0, &readden, NULL), "ReadFile() failed: %08x\n", GetLastError());
        ok(readden == 0, "got %d bytes\n", readden);

        memset(ibuf, 0, sizeof(ibuf));
        ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() failed: %08x\n", GetLastError());
        ok(readden == sizeof(obuf), "got %d bytes\n", readden);
        /* pipe is empty now */

        SetLastError(0xdeadbeef);
        ok(!ReadFile(hFile, ibuf, 0, &readden, NULL), "ReadFile() succeeded\n");
        ok(GetLastError() == ERROR_BROKEN_PIPE, "GetLastError() returned %08x, expected ERROR_BROKEN_PIPE\n", GetLastError());
        SetLastError(0);

        CloseHandle(hFile);
    }

    hnp = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
                           PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                           /* nMaxInstances */ 1,
                           /* nOutBufSize */ 1024,
                           /* nInBufSize */ 1024,
                           /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
                           /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError());

    /* don't try to do i/o if one side couldn't be opened, as it hangs */
    if (hFile != INVALID_HANDLE_VALUE)
    {
        ok(WriteFile(hnp, obuf, 0, &written, NULL), "WriteFile\n");
        ok(written == 0, "write file len 1\n");

        /* close server end without disconnecting */
        ok(CloseHandle(hnp), "CloseHandle() failed: %08x\n", GetLastError());

        todo_wine
        ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() failed: %08x\n", GetLastError());
        ok(readden == 0, "got %d bytes\n", readden);
        /* pipe is empty now */

        SetLastError(0xdeadbeef);
        ok(!ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() succeeded\n");
        todo_wine
        ok(GetLastError() == ERROR_BROKEN_PIPE, "GetLastError() returned %08x, expected ERROR_BROKEN_PIPE\n", GetLastError());
        SetLastError(0);

        CloseHandle(hFile);
    }

    hnp = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
                           PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                           /* nMaxInstances */ 1,
                           /* nOutBufSize */ 1024,
                           /* nInBufSize */ 1024,
                           /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
                           /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError());

    if (hFile != INVALID_HANDLE_VALUE)
    {
        /* Make sure we can read and write a few bytes in both directions */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len 1\n");
        ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, NULL), "Peek\n");
        ok(readden == sizeof(obuf), "got %d bytes\n", readden);

        /* close client end without disconnecting */
        ok(CloseHandle(hFile), "CloseHandle() failed: %08x\n", GetLastError());

        /* you'd think ERROR_MORE_DATA, but no */
        ok(ReadFile(hnp, ibuf, 0, &readden, NULL), "ReadFile() failed: %08x\n", GetLastError());
        ok(readden == 0, "got %d bytes\n", readden);

        memset(ibuf, 0, sizeof(ibuf));
        ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() failed: %08x\n", GetLastError());
        ok(readden == sizeof(obuf), "got %d bytes\n", readden);
        /* pipe is empty now */

        SetLastError(0xdeadbeef);
        ok(!ReadFile(hnp, ibuf, 0, &readden, NULL), "ReadFile() succeeded\n");
        ok(GetLastError() == ERROR_BROKEN_PIPE, "GetLastError() returned %08x, expected ERROR_BROKEN_PIPE\n", GetLastError());
        SetLastError(0);

        CloseHandle(hnp);
    }

    hnp = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
                           PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                           /* nMaxInstances */ 1,
                           /* nOutBufSize */ 1024,
                           /* nInBufSize */ 1024,
                           /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
                           /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError());

    /* don't try to do i/o if one side couldn't be opened, as it hangs */
    if (hFile != INVALID_HANDLE_VALUE)
    {
        ok(WriteFile(hFile, obuf, 0, &written, NULL), "WriteFile\n");
        ok(written == 0, "write file len 1\n");

        /* close server end without disconnecting */
        ok(CloseHandle(hFile), "CloseHandle() failed: %08x\n", GetLastError());

        todo_wine
        ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() failed: %08x\n", GetLastError());
        ok(readden == 0, "got %d bytes\n", readden);
        /* pipe is empty now */

        SetLastError(0xdeadbeef);
        ok(!ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() succeeded\n");
        ok(GetLastError() == ERROR_BROKEN_PIPE, "GetLastError() returned %08x, expected ERROR_BROKEN_PIPE\n", GetLastError());
        SetLastError(0);

        CloseHandle(hnp);
    }
}

/** implementation of alarm() */
static DWORD CALLBACK alarmThreadMain(LPVOID arg)
{
    DWORD_PTR timeout = (DWORD_PTR) arg;
    trace("alarmThreadMain\n");
    if (WaitForSingleObject( alarm_event, timeout ) == WAIT_TIMEOUT)
    {
        ok(FALSE, "alarm\n");
        ExitProcess(1);
    }
    return 1;
}

static HANDLE hnp = INVALID_HANDLE_VALUE;

/** Trivial byte echo server - disconnects after each session */
static DWORD CALLBACK serverThreadMain1(LPVOID arg)
{
    int i;

    trace("serverThreadMain1 start\n");
    /* Set up a simple echo server */
    hnp = CreateNamedPipeA(PIPENAME "serverThreadMain1", PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);

    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");
    for (i = 0; i < NB_SERVER_LOOPS; i++) {
        char buf[512];
        DWORD written;
        DWORD readden;
        BOOL success;

        /* Wait for client to connect */
        trace("Server calling ConnectNamedPipe...\n");
        ok(ConnectNamedPipe(hnp, NULL)
            || GetLastError() == ERROR_PIPE_CONNECTED, "ConnectNamedPipe\n");
        trace("ConnectNamedPipe returned.\n");

        /* Echo bytes once */
        memset(buf, 0, sizeof(buf));

        trace("Server reading...\n");
        success = ReadFile(hnp, buf, sizeof(buf), &readden, NULL);
        trace("Server done reading.\n");
        ok(success, "ReadFile\n");
        ok(readden, "short read\n");

        trace("Server writing...\n");
        ok(WriteFile(hnp, buf, readden, &written, NULL), "WriteFile\n");
        trace("Server done writing.\n");
        ok(written == readden, "write file len\n");

        /* finish this connection, wait for next one */
        ok(FlushFileBuffers(hnp), "FlushFileBuffers\n");
        trace("Server done flushing.\n");
        ok(DisconnectNamedPipe(hnp), "DisconnectNamedPipe\n");
        trace("Server done disconnecting.\n");
    }
    return 0;
}

/** Trivial byte echo server - closes after each connection */
static DWORD CALLBACK serverThreadMain2(LPVOID arg)
{
    int i;
    HANDLE hnpNext = 0;

    trace("serverThreadMain2\n");
    /* Set up a simple echo server */
    hnp = CreateNamedPipeA(PIPENAME "serverThreadMain2", PIPE_ACCESS_DUPLEX,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 2,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    for (i = 0; i < NB_SERVER_LOOPS; i++) {
        char buf[512];
        DWORD written;
        DWORD readden;
        DWORD ret;
        BOOL success;


        user_apc_ran = FALSE;
        if (i == 0 && pQueueUserAPC) {
            trace("Queueing an user APC\n"); /* verify the pipe is non alerable */
            ret = pQueueUserAPC(&user_apc, GetCurrentThread(), 0);
            ok(ret, "QueueUserAPC failed: %d\n", GetLastError());
        }

        /* Wait for client to connect */
        trace("Server calling ConnectNamedPipe...\n");
        ok(ConnectNamedPipe(hnp, NULL)
            || GetLastError() == ERROR_PIPE_CONNECTED, "ConnectNamedPipe\n");
        trace("ConnectNamedPipe returned.\n");

        /* Echo bytes once */
        memset(buf, 0, sizeof(buf));

        trace("Server reading...\n");
        success = ReadFile(hnp, buf, sizeof(buf), &readden, NULL);
        trace("Server done reading.\n");
        ok(success, "ReadFile\n");

        trace("Server writing...\n");
        ok(WriteFile(hnp, buf, readden, &written, NULL), "WriteFile\n");
        trace("Server done writing.\n");
        ok(written == readden, "write file len\n");

        /* finish this connection, wait for next one */
        ok(FlushFileBuffers(hnp), "FlushFileBuffers\n");
        ok(DisconnectNamedPipe(hnp), "DisconnectNamedPipe\n");

        ok(user_apc_ran == FALSE, "UserAPC ran, pipe using alertable io mode\n");

        if (i == 0 && pQueueUserAPC)
            SleepEx(0, TRUE); /* get rid of apc */

        /* Set up next echo server */
        hnpNext =
            CreateNamedPipeA(PIPENAME "serverThreadMain2", PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_BYTE | PIPE_WAIT,
            /* nMaxInstances */ 2,
            /* nOutBufSize */ 1024,
            /* nInBufSize */ 1024,
            /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
            /* lpSecurityAttrib */ NULL);

        ok(hnpNext != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

        ok(CloseHandle(hnp), "CloseHandle\n");
        hnp = hnpNext;
    }
    return 0;
}

/** Trivial byte echo server - uses overlapped named pipe calls */
static DWORD CALLBACK serverThreadMain3(LPVOID arg)
{
    int i;
    HANDLE hEvent;

    trace("serverThreadMain3\n");
    /* Set up a simple echo server */
    hnp = CreateNamedPipeA(PIPENAME "serverThreadMain3", PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hEvent = CreateEventW(NULL,  /* security attribute */
        TRUE,                   /* manual reset event */
        FALSE,                  /* initial state */
        NULL);                  /* name */
    ok(hEvent != NULL, "CreateEvent\n");

    for (i = 0; i < NB_SERVER_LOOPS; i++) {
        char buf[512];
        DWORD written;
        DWORD readden;
        DWORD dummy;
        BOOL success;
        OVERLAPPED oOverlap;
        int letWFSOEwait = (i & 2);
        int letGORwait = (i & 1);
        DWORD err;

        memset(&oOverlap, 0, sizeof(oOverlap));
        oOverlap.hEvent = hEvent;

        /* Wait for client to connect */
        if (i == 0) {
            trace("Server calling non-overlapped ConnectNamedPipe on overlapped pipe...\n");
            success = ConnectNamedPipe(hnp, NULL);
            err = GetLastError();
            ok(success || (err == ERROR_PIPE_CONNECTED), "ConnectNamedPipe failed: %d\n", err);
            trace("ConnectNamedPipe operation complete.\n");
        } else {
            trace("Server calling overlapped ConnectNamedPipe...\n");
            success = ConnectNamedPipe(hnp, &oOverlap);
            err = GetLastError();
            ok(!success && (err == ERROR_IO_PENDING || err == ERROR_PIPE_CONNECTED), "overlapped ConnectNamedPipe\n");
            trace("overlapped ConnectNamedPipe returned.\n");
            if (!success && (err == ERROR_IO_PENDING)) {
                if (letWFSOEwait)
                {
                    DWORD ret;
                    do {
                        ret = WaitForSingleObjectEx(hEvent, INFINITE, TRUE);
                    } while (ret == WAIT_IO_COMPLETION);
                    ok(ret == 0, "wait ConnectNamedPipe returned %x\n", ret);
                }
                success = GetOverlappedResult(hnp, &oOverlap, &dummy, letGORwait);
                if (!letGORwait && !letWFSOEwait && !success) {
                    ok(GetLastError() == ERROR_IO_INCOMPLETE, "GetOverlappedResult\n");
                    success = GetOverlappedResult(hnp, &oOverlap, &dummy, TRUE);
                }
            }
            ok(success || (err == ERROR_PIPE_CONNECTED), "GetOverlappedResult ConnectNamedPipe\n");
            trace("overlapped ConnectNamedPipe operation complete.\n");
        }

        /* Echo bytes once */
        memset(buf, 0, sizeof(buf));

        trace("Server reading...\n");
        success = ReadFile(hnp, buf, sizeof(buf), &readden, &oOverlap);
        trace("Server ReadFile returned...\n");
        err = GetLastError();
        ok(success || err == ERROR_IO_PENDING, "overlapped ReadFile\n");
        trace("overlapped ReadFile returned.\n");
        if (!success && (err == ERROR_IO_PENDING)) {
            if (letWFSOEwait)
            {
                DWORD ret;
                do {
                    ret = WaitForSingleObjectEx(hEvent, INFINITE, TRUE);
                } while (ret == WAIT_IO_COMPLETION);
                ok(ret == 0, "wait ReadFile returned %x\n", ret);
            }
            success = GetOverlappedResult(hnp, &oOverlap, &readden, letGORwait);
            if (!letGORwait && !letWFSOEwait && !success) {
                ok(GetLastError() == ERROR_IO_INCOMPLETE, "GetOverlappedResult\n");
                success = GetOverlappedResult(hnp, &oOverlap, &readden, TRUE);
            }
        }
        trace("Server done reading.\n");
        ok(success, "overlapped ReadFile\n");

        trace("Server writing...\n");
        success = WriteFile(hnp, buf, readden, &written, &oOverlap);
        trace("Server WriteFile returned...\n");
        err = GetLastError();
        ok(success || err == ERROR_IO_PENDING, "overlapped WriteFile\n");
        trace("overlapped WriteFile returned.\n");
        if (!success && (err == ERROR_IO_PENDING)) {
            if (letWFSOEwait)
            {
                DWORD ret;
                do {
                    ret = WaitForSingleObjectEx(hEvent, INFINITE, TRUE);
                } while (ret == WAIT_IO_COMPLETION);
                ok(ret == 0, "wait WriteFile returned %x\n", ret);
            }
            success = GetOverlappedResult(hnp, &oOverlap, &written, letGORwait);
            if (!letGORwait && !letWFSOEwait && !success) {
                ok(GetLastError() == ERROR_IO_INCOMPLETE, "GetOverlappedResult\n");
                success = GetOverlappedResult(hnp, &oOverlap, &written, TRUE);
            }
        }
        trace("Server done writing.\n");
        ok(success, "overlapped WriteFile\n");
        ok(written == readden, "write file len\n");

        /* finish this connection, wait for next one */
        ok(FlushFileBuffers(hnp), "FlushFileBuffers\n");
        ok(DisconnectNamedPipe(hnp), "DisconnectNamedPipe\n");
    }
    return 0;
}

/** Trivial byte echo server - uses i/o completion ports */
static DWORD CALLBACK serverThreadMain4(LPVOID arg)
{
    int i;
    HANDLE hcompletion;
    BOOL ret;

    trace("serverThreadMain4\n");
    /* Set up a simple echo server */
    hnp = CreateNamedPipeA(PIPENAME "serverThreadMain4", PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hcompletion = CreateIoCompletionPort(hnp, NULL, 12345, 1);
    ok(hcompletion != NULL, "CreateIoCompletionPort failed, error=%i\n", GetLastError());

    for (i = 0; i < NB_SERVER_LOOPS; i++) {
        char buf[512];
        DWORD written;
        DWORD readden;
        DWORD dummy;
        BOOL success;
        OVERLAPPED oConnect;
        OVERLAPPED oRead;
        OVERLAPPED oWrite;
        OVERLAPPED *oResult;
        DWORD err;
        ULONG_PTR compkey;

        memset(&oConnect, 0, sizeof(oConnect));
        memset(&oRead, 0, sizeof(oRead));
        memset(&oWrite, 0, sizeof(oWrite));

        /* Wait for client to connect */
        trace("Server calling overlapped ConnectNamedPipe...\n");
        success = ConnectNamedPipe(hnp, &oConnect);
        err = GetLastError();
        ok(!success && (err == ERROR_IO_PENDING || err == ERROR_PIPE_CONNECTED),
           "overlapped ConnectNamedPipe got %u err %u\n", success, err );
        if (!success && err == ERROR_IO_PENDING) {
            trace("ConnectNamedPipe GetQueuedCompletionStatus\n");
            success = GetQueuedCompletionStatus(hcompletion, &dummy, &compkey, &oResult, 0);
            if (!success)
            {
                ok( GetLastError() == WAIT_TIMEOUT,
                    "ConnectNamedPipe GetQueuedCompletionStatus wrong error %u\n", GetLastError());
                success = GetQueuedCompletionStatus(hcompletion, &dummy, &compkey, &oResult, 10000);
            }
            ok(success, "ConnectNamedPipe GetQueuedCompletionStatus failed, errno=%i\n", GetLastError());
            if (success)
            {
                ok(compkey == 12345, "got completion key %i instead of 12345\n", (int)compkey);
                ok(oResult == &oConnect, "got overlapped pointer %p instead of %p\n", oResult, &oConnect);
            }
        }
        trace("overlapped ConnectNamedPipe operation complete.\n");

        /* Echo bytes once */
        memset(buf, 0, sizeof(buf));

        trace("Server reading...\n");
        success = ReadFile(hnp, buf, sizeof(buf), &readden, &oRead);
        trace("Server ReadFile returned...\n");
        err = GetLastError();
        ok(success || err == ERROR_IO_PENDING, "overlapped ReadFile, err=%i\n", err);
        success = GetQueuedCompletionStatus(hcompletion, &readden, &compkey,
            &oResult, 10000);
        ok(success, "ReadFile GetQueuedCompletionStatus failed, errno=%i\n", GetLastError());
        if (success)
        {
            ok(compkey == 12345, "got completion key %i instead of 12345\n", (int)compkey);
            ok(oResult == &oRead, "got overlapped pointer %p instead of %p\n", oResult, &oRead);
        }
        trace("Server done reading.\n");

        trace("Server writing...\n");
        success = WriteFile(hnp, buf, readden, &written, &oWrite);
        trace("Server WriteFile returned...\n");
        err = GetLastError();
        ok(success || err == ERROR_IO_PENDING, "overlapped WriteFile failed, err=%u\n", err);
        success = GetQueuedCompletionStatus(hcompletion, &written, &compkey,
            &oResult, 10000);
        ok(success, "WriteFile GetQueuedCompletionStatus failed, errno=%i\n", GetLastError());
        if (success)
        {
            ok(compkey == 12345, "got completion key %i instead of 12345\n", (int)compkey);
            ok(oResult == &oWrite, "got overlapped pointer %p instead of %p\n", oResult, &oWrite);
            ok(written == readden, "write file len\n");
        }
        trace("Server done writing.\n");

        /* finish this connection, wait for next one */
        ok(FlushFileBuffers(hnp), "FlushFileBuffers\n");
        success = DisconnectNamedPipe(hnp);
        ok(success, "DisconnectNamedPipe failed, err %u\n", GetLastError());
    }

    ret = CloseHandle(hnp);
    ok(ret, "CloseHandle named pipe failed, err=%i\n", GetLastError());
    ret = CloseHandle(hcompletion);
    ok(ret, "CloseHandle completion failed, err=%i\n", GetLastError());

    return 0;
}

static int completion_called;
static DWORD completion_errorcode;
static DWORD completion_num_bytes;
static LPOVERLAPPED completion_lpoverlapped;

static VOID WINAPI completion_routine(DWORD errorcode, DWORD num_bytes, LPOVERLAPPED lpoverlapped)
{
    completion_called++;
    completion_errorcode = errorcode;
    completion_num_bytes = num_bytes;
    completion_lpoverlapped = lpoverlapped;
    SetEvent(lpoverlapped->hEvent);
}

/** Trivial byte echo server - uses ReadFileEx/WriteFileEx */
static DWORD CALLBACK serverThreadMain5(LPVOID arg)
{
    int i;
    HANDLE hEvent;

    trace("serverThreadMain5\n");
    /* Set up a simple echo server */
    hnp = CreateNamedPipeA(PIPENAME "serverThreadMain5", PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hEvent = CreateEventW(NULL,  /* security attribute */
        TRUE,                   /* manual reset event */
        FALSE,                  /* initial state */
        NULL);                  /* name */
    ok(hEvent != NULL, "CreateEvent\n");

    for (i = 0; i < NB_SERVER_LOOPS; i++) {
        char buf[512];
        DWORD readden;
        BOOL success;
        OVERLAPPED oOverlap;
        DWORD err;

        memset(&oOverlap, 0, sizeof(oOverlap));
        oOverlap.hEvent = hEvent;

        /* Wait for client to connect */
        trace("Server calling ConnectNamedPipe...\n");
        success = ConnectNamedPipe(hnp, NULL);
        err = GetLastError();
        ok(success || (err == ERROR_PIPE_CONNECTED), "ConnectNamedPipe failed: %d\n", err);
        trace("ConnectNamedPipe operation complete.\n");

        /* Echo bytes once */
        memset(buf, 0, sizeof(buf));

        trace("Server reading...\n");
        completion_called = 0;
        ResetEvent(hEvent);
        success = ReadFileEx(hnp, buf, sizeof(buf), &oOverlap, completion_routine);
        trace("Server ReadFileEx returned...\n");
        ok(success, "ReadFileEx failed, err=%i\n", GetLastError());
        ok(completion_called == 0, "completion routine called before ReadFileEx return\n");
        trace("ReadFileEx returned.\n");
        if (success) {
            DWORD ret;
            do {
                ret = WaitForSingleObjectEx(hEvent, INFINITE, TRUE);
            } while (ret == WAIT_IO_COMPLETION);
            ok(ret == 0, "wait ReadFileEx returned %x\n", ret);
        }
        ok(completion_called == 1, "completion routine called %i times\n", completion_called);
        ok(completion_errorcode == ERROR_SUCCESS, "completion routine got error %d\n", completion_errorcode);
        ok(completion_num_bytes != 0, "read 0 bytes\n");
        ok(completion_lpoverlapped == &oOverlap, "got wrong overlapped pointer %p\n", completion_lpoverlapped);
        readden = completion_num_bytes;
        trace("Server done reading.\n");

        trace("Server writing...\n");
        completion_called = 0;
        ResetEvent(hEvent);
        success = WriteFileEx(hnp, buf, readden, &oOverlap, completion_routine);
        trace("Server WriteFileEx returned...\n");
        ok(success, "WriteFileEx failed, err=%i\n", GetLastError());
        ok(completion_called == 0, "completion routine called before ReadFileEx return\n");
        trace("overlapped WriteFile returned.\n");
        if (success) {
            DWORD ret;
            do {
                ret = WaitForSingleObjectEx(hEvent, INFINITE, TRUE);
            } while (ret == WAIT_IO_COMPLETION);
            ok(ret == 0, "wait WriteFileEx returned %x\n", ret);
        }
        trace("Server done writing.\n");
        ok(completion_called == 1, "completion routine called %i times\n", completion_called);
        ok(completion_errorcode == ERROR_SUCCESS, "completion routine got error %d\n", completion_errorcode);
        ok(completion_num_bytes == readden, "read %i bytes wrote %i\n", readden, completion_num_bytes);
        ok(completion_lpoverlapped == &oOverlap, "got wrong overlapped pointer %p\n", completion_lpoverlapped);

        /* finish this connection, wait for next one */
        ok(FlushFileBuffers(hnp), "FlushFileBuffers\n");
        ok(DisconnectNamedPipe(hnp), "DisconnectNamedPipe\n");
    }
    return 0;
}

static void exercizeServer(const char *pipename, HANDLE serverThread)
{
    int i;

    trace("exercizeServer starting\n");
    for (i = 0; i < NB_SERVER_LOOPS; i++) {
        HANDLE hFile=INVALID_HANDLE_VALUE;
        static const char obuf[] = "Bit Bucket";
        char ibuf[32];
        DWORD written;
        DWORD readden;
        int loop;

        for (loop = 0; loop < 3; loop++) {
	    DWORD err;
            trace("Client connecting...\n");
            /* Connect to the server */
            hFile = CreateFileA(pipename, GENERIC_READ | GENERIC_WRITE, 0,
                NULL, OPEN_EXISTING, 0, 0);
            if (hFile != INVALID_HANDLE_VALUE)
                break;
	    err = GetLastError();
	    if (loop == 0)
	        ok(err == ERROR_PIPE_BUSY || err == ERROR_FILE_NOT_FOUND, "connecting to pipe\n");
	    else
	        ok(err == ERROR_PIPE_BUSY, "connecting to pipe\n");
            trace("connect failed, retrying\n");
            Sleep(200);
        }
        ok(hFile != INVALID_HANDLE_VALUE, "client opening named pipe\n");

        /* Make sure it can echo */
        memset(ibuf, 0, sizeof(ibuf));
        trace("Client writing...\n");
        ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile to client end of pipe\n");
        ok(written == sizeof(obuf), "write file len\n");
        trace("Client reading...\n");
        ok(ReadFile(hFile, ibuf, sizeof(obuf), &readden, NULL), "ReadFile from client end of pipe\n");
        ok(readden == sizeof(obuf), "read file len\n");
        ok(memcmp(obuf, ibuf, written) == 0, "content check\n");

        trace("Client closing...\n");
        ok(CloseHandle(hFile), "CloseHandle\n");
    }

    ok(WaitForSingleObject(serverThread,INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject\n");
    CloseHandle(hnp);
    trace("exercizeServer returning\n");
}

static void test_NamedPipe_2(void)
{
    HANDLE serverThread;
    DWORD serverThreadId;
    HANDLE alarmThread;
    DWORD alarmThreadId;

    trace("test_NamedPipe_2 starting\n");
    /* Set up a twenty second timeout */
    alarm_event = CreateEventW( NULL, TRUE, FALSE, NULL );
    SetLastError(0xdeadbeef);
    alarmThread = CreateThread(NULL, 0, alarmThreadMain, (void *) 20000, 0, &alarmThreadId);
    ok(alarmThread != NULL, "CreateThread failed: %d\n", GetLastError());

    /* The servers we're about to exercise do try to clean up carefully,
     * but to reduce the chance of a test failure due to a pipe handle
     * leak in the test code, we'll use a different pipe name for each server.
     */

    /* Try server #1 */
    SetLastError(0xdeadbeef);
    serverThread = CreateThread(NULL, 0, serverThreadMain1, (void *)8, 0, &serverThreadId);
    ok(serverThread != NULL, "CreateThread failed: %d\n", GetLastError());
    exercizeServer(PIPENAME "serverThreadMain1", serverThread);

    /* Try server #2 */
    SetLastError(0xdeadbeef);
    serverThread = CreateThread(NULL, 0, serverThreadMain2, 0, 0, &serverThreadId);
    ok(serverThread != NULL, "CreateThread failed: %d\n", GetLastError());
    exercizeServer(PIPENAME "serverThreadMain2", serverThread);

    /* Try server #3 */
    SetLastError(0xdeadbeef);
    serverThread = CreateThread(NULL, 0, serverThreadMain3, 0, 0, &serverThreadId);
    ok(serverThread != NULL, "CreateThread failed: %d\n", GetLastError());
    exercizeServer(PIPENAME "serverThreadMain3", serverThread);

    /* Try server #4 */
    SetLastError(0xdeadbeef);
    serverThread = CreateThread(NULL, 0, serverThreadMain4, 0, 0, &serverThreadId);
    ok(serverThread != NULL, "CreateThread failed: %d\n", GetLastError());
    exercizeServer(PIPENAME "serverThreadMain4", serverThread);

    /* Try server #5 */
    SetLastError(0xdeadbeef);
    serverThread = CreateThread(NULL, 0, serverThreadMain5, 0, 0, &serverThreadId);
    ok(serverThread != NULL, "CreateThread failed: %d\n", GetLastError());
    exercizeServer(PIPENAME "serverThreadMain5", serverThread);

    ok(SetEvent( alarm_event ), "SetEvent\n");
    CloseHandle( alarm_event );
    trace("test_NamedPipe_2 returning\n");
}

static int test_DisconnectNamedPipe(void)
{
    HANDLE hnp;
    HANDLE hFile;
    static const char obuf[] = "Bit Bucket";
    char ibuf[32];
    DWORD written;
    DWORD readden;
    DWORD ret;

    SetLastError(0xdeadbeef);
    hnp = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    if ((hnp == INVALID_HANDLE_VALUE /* Win98 */ || !hnp /* Win95 */)
        && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {

        win_skip("Named pipes are not implemented\n");
        return 1;
    }

    ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL) == 0
        && GetLastError() == ERROR_PIPE_LISTENING, "WriteFile to not-yet-connected pipe\n");
    ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL) == 0
        && GetLastError() == ERROR_PIPE_LISTENING, "ReadFile from not-yet-connected pipe\n");

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile failed\n");

    /* don't try to do i/o if one side couldn't be opened, as it hangs */
    if (hFile != INVALID_HANDLE_VALUE) {

        /* see what happens if server calls DisconnectNamedPipe
         * when there are bytes in the pipe
         */

        ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len\n");
        ok(DisconnectNamedPipe(hnp), "DisconnectNamedPipe while messages waiting\n");
        ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL) == 0
            && GetLastError() == ERROR_PIPE_NOT_CONNECTED, "WriteFile to disconnected pipe\n");
        ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL) == 0
            && GetLastError() == ERROR_PIPE_NOT_CONNECTED,
            "ReadFile from disconnected pipe with bytes waiting\n");
        ok(!DisconnectNamedPipe(hnp) && GetLastError() == ERROR_PIPE_NOT_CONNECTED,
           "DisconnectNamedPipe worked twice\n");
        ret = WaitForSingleObject(hFile, 0);
        ok(ret == WAIT_TIMEOUT, "WaitForSingleObject returned %X\n", ret);
        ok(CloseHandle(hFile), "CloseHandle\n");
    }

    ok(CloseHandle(hnp), "CloseHandle\n");

    return 0;
}
static void test_CreatePipe(void)
{
    SECURITY_ATTRIBUTES pipe_attr;
    HANDLE piperead, pipewrite;
    DWORD written;
    DWORD read;
    DWORD i, size;
    BYTE *buffer;
    char readbuf[32];

    user_apc_ran = FALSE;
    if (pQueueUserAPC)
        ok(pQueueUserAPC(user_apc, GetCurrentThread(), 0), "couldn't create user apc\n");

    pipe_attr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    pipe_attr.bInheritHandle = TRUE; 
    pipe_attr.lpSecurityDescriptor = NULL;
    ok(CreatePipe(&piperead, &pipewrite, &pipe_attr, 0) != 0, "CreatePipe failed\n");
    ok(WriteFile(pipewrite,PIPENAME,sizeof(PIPENAME), &written, NULL), "Write to anonymous pipe failed\n");
    ok(written == sizeof(PIPENAME), "Write to anonymous pipe wrote %d bytes\n", written);
    ok(ReadFile(piperead,readbuf,sizeof(readbuf),&read, NULL), "Read from non empty pipe failed\n");
    ok(read == sizeof(PIPENAME), "Read from  anonymous pipe got %d bytes\n", read);
    ok(CloseHandle(pipewrite), "CloseHandle for the write pipe failed\n");
    ok(CloseHandle(piperead), "CloseHandle for the read pipe failed\n");

    /* Now write another chunk*/
    ok(CreatePipe(&piperead, &pipewrite, &pipe_attr, 0) != 0, "CreatePipe failed\n");
    ok(WriteFile(pipewrite,PIPENAME,sizeof(PIPENAME), &written, NULL), "Write to anonymous pipe failed\n");
    ok(written == sizeof(PIPENAME), "Write to anonymous pipe wrote %d bytes\n", written);
    /* and close the write end, read should still succeed*/
    ok(CloseHandle(pipewrite), "CloseHandle for the Write Pipe failed\n");
    ok(ReadFile(piperead,readbuf,sizeof(readbuf),&read, NULL), "Read from broken pipe withe with pending data failed\n");
    ok(read == sizeof(PIPENAME), "Read from  anonymous pipe got %d bytes\n", read);
    /* But now we need to get informed that the pipe is closed */
    ok(ReadFile(piperead,readbuf,sizeof(readbuf),&read, NULL) == 0, "Broken pipe not detected\n");
    ok(CloseHandle(piperead), "CloseHandle for the read pipe failed\n");

    /* Try bigger chunks */
    size = 32768;
    buffer = HeapAlloc( GetProcessHeap(), 0, size );
    for (i = 0; i < size; i++) buffer[i] = i;
    ok(CreatePipe(&piperead, &pipewrite, &pipe_attr, (size + 24)) != 0, "CreatePipe failed\n");
    ok(WriteFile(pipewrite, buffer, size, &written, NULL), "Write to anonymous pipe failed\n");
    ok(written == size, "Write to anonymous pipe wrote %d bytes\n", written);
    /* and close the write end, read should still succeed*/
    ok(CloseHandle(pipewrite), "CloseHandle for the Write Pipe failed\n");
    memset( buffer, 0, size );
    ok(ReadFile(piperead, buffer, size, &read, NULL), "Read from broken pipe withe with pending data failed\n");
    ok(read == size, "Read from  anonymous pipe got %d bytes\n", read);
    for (i = 0; i < size; i++) ok( buffer[i] == (BYTE)i, "invalid data %x at %x\n", buffer[i], i );
    /* But now we need to get informed that the pipe is closed */
    ok(ReadFile(piperead,readbuf,sizeof(readbuf),&read, NULL) == 0, "Broken pipe not detected\n");
    ok(CloseHandle(piperead), "CloseHandle for the read pipe failed\n");
    HeapFree(GetProcessHeap(), 0, buffer);

    ok(user_apc_ran == FALSE, "user apc ran, pipe using alertable io mode\n");
    SleepEx(0, TRUE); /* get rid of apc */
}

struct named_pipe_client_params
{
    DWORD security_flags;
    HANDLE token;
    BOOL revert;
};

#define PIPE_NAME "\\\\.\\pipe\\named_pipe_test"

static DWORD CALLBACK named_pipe_client_func(LPVOID p)
{
    struct named_pipe_client_params *params = p;
    HANDLE pipe;
    BOOL ret;
    const char message[] = "Test";
    DWORD bytes_read, bytes_written;
    char dummy;
    TOKEN_PRIVILEGES *Privileges = NULL;

    if (params->token)
    {
        if (params->revert)
        {
            /* modify the token so we can tell if the pipe impersonation
             * token reverts to the process token */
            ret = AdjustTokenPrivileges(params->token, TRUE, NULL, 0, NULL, NULL);
            ok(ret, "AdjustTokenPrivileges failed with error %d\n", GetLastError());
        }
        ret = SetThreadToken(NULL, params->token);
        ok(ret, "SetThreadToken failed with error %d\n", GetLastError());
    }
    else
    {
        DWORD Size = 0;
        HANDLE process_token;

        ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY|TOKEN_ADJUST_PRIVILEGES, &process_token);
        ok(ret, "OpenProcessToken failed with error %d\n", GetLastError());

        ret = GetTokenInformation(process_token, TokenPrivileges, NULL, 0, &Size);
        ok(!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER, "GetTokenInformation(TokenPrivileges) failed with %d\n", GetLastError());
        Privileges = HeapAlloc(GetProcessHeap(), 0, Size);
        ret = GetTokenInformation(process_token, TokenPrivileges, Privileges, Size, &Size);
        ok(ret, "GetTokenInformation(TokenPrivileges) failed with %d\n", GetLastError());

        ret = AdjustTokenPrivileges(process_token, TRUE, NULL, 0, NULL, NULL);
        ok(ret, "AdjustTokenPrivileges failed with error %d\n", GetLastError());

        CloseHandle(process_token);
    }

    pipe = CreateFileA(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, params->security_flags, NULL);
    ok(pipe != INVALID_HANDLE_VALUE, "CreateFile for pipe failed with error %d\n", GetLastError());

    ret = WriteFile(pipe, message, sizeof(message), &bytes_written, NULL);
    ok(ret, "WriteFile failed with error %d\n", GetLastError());

    ret = ReadFile(pipe, &dummy, sizeof(dummy), &bytes_read, NULL);
    ok(ret, "ReadFile failed with error %d\n", GetLastError());

    if (params->token)
    {
        if (params->revert)
        {
            ret = RevertToSelf();
            ok(ret, "RevertToSelf failed with error %d\n", GetLastError());
        }
        else
        {
            ret = AdjustTokenPrivileges(params->token, TRUE, NULL, 0, NULL, NULL);
            ok(ret, "AdjustTokenPrivileges failed with error %d\n", GetLastError());
        }
    }
    else
    {
        HANDLE process_token;

        ret = OpenProcessToken(GetCurrentProcess(), TOKEN_ADJUST_PRIVILEGES, &process_token);
        ok(ret, "OpenProcessToken failed with error %d\n", GetLastError());

        ret = AdjustTokenPrivileges(process_token, FALSE, Privileges, 0, NULL, NULL);
        ok(ret, "AdjustTokenPrivileges failed with error %d\n", GetLastError());

        HeapFree(GetProcessHeap(), 0, Privileges);

        CloseHandle(process_token);
    }

    ret = WriteFile(pipe, message, sizeof(message), &bytes_written, NULL);
    ok(ret, "WriteFile failed with error %d\n", GetLastError());

    ret = ReadFile(pipe, &dummy, sizeof(dummy), &bytes_read, NULL);
    ok(ret, "ReadFile failed with error %d\n", GetLastError());

    CloseHandle(pipe);

    return 0;
}

static HANDLE make_impersonation_token(DWORD Access, SECURITY_IMPERSONATION_LEVEL ImpersonationLevel)
{
    HANDLE ProcessToken;
    HANDLE Token = NULL;
    BOOL ret;

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_DUPLICATE, &ProcessToken);
    ok(ret, "OpenProcessToken failed with error %d\n", GetLastError());

    ret = pDuplicateTokenEx(ProcessToken, Access, NULL, ImpersonationLevel, TokenImpersonation, &Token);
    ok(ret, "DuplicateToken failed with error %d\n", GetLastError());

    CloseHandle(ProcessToken);

    return Token;
}

static void test_ImpersonateNamedPipeClient(HANDLE hClientToken, DWORD security_flags, BOOL revert, void (*test_func)(int, HANDLE))
{
    HANDLE hPipeServer;
    BOOL ret;
    DWORD dwTid;
    HANDLE hThread;
    char buffer[256];
    DWORD dwBytesRead;
    DWORD error;
    struct named_pipe_client_params params;
    char dummy = 0;
    DWORD dwBytesWritten;
    HANDLE hToken = NULL;
    SECURITY_IMPERSONATION_LEVEL ImpersonationLevel;
    DWORD size;

    hPipeServer = CreateNamedPipeA(PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 100, 100, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(hPipeServer != INVALID_HANDLE_VALUE, "CreateNamedPipe failed with error %d\n", GetLastError());

    params.security_flags = security_flags;
    params.token = hClientToken;
    params.revert = revert;
    hThread = CreateThread(NULL, 0, named_pipe_client_func, &params, 0, &dwTid);
    ok(hThread != NULL, "CreateThread failed with error %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ImpersonateNamedPipeClient(hPipeServer);
    error = GetLastError();
    ok(ret /* win2k3 */ || (error == ERROR_CANNOT_IMPERSONATE),
       "ImpersonateNamedPipeClient should have failed with ERROR_CANNOT_IMPERSONATE instead of %d\n", GetLastError());

    ret = ConnectNamedPipe(hPipeServer, NULL);
    ok(ret || (GetLastError() == ERROR_PIPE_CONNECTED), "ConnectNamedPipe failed with error %d\n", GetLastError());

    ret = ReadFile(hPipeServer, buffer, sizeof(buffer), &dwBytesRead, NULL);
    ok(ret, "ReadFile failed with error %d\n", GetLastError());

    ret = ImpersonateNamedPipeClient(hPipeServer);
    ok(ret, "ImpersonateNamedPipeClient failed with error %d\n", GetLastError());

    ret = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken);
    ok(ret, "OpenThreadToken failed with error %d\n", GetLastError());

    (*test_func)(0, hToken);

    ImpersonationLevel = 0xdeadbeef; /* to avoid false positives */
    ret = GetTokenInformation(hToken, TokenImpersonationLevel, &ImpersonationLevel, sizeof(ImpersonationLevel), &size);
    ok(ret, "GetTokenInformation(TokenImpersonationLevel) failed with error %d\n", GetLastError());
    ok(ImpersonationLevel == SecurityImpersonation, "ImpersonationLevel should have been SecurityImpersonation(%d) instead of %d\n", SecurityImpersonation, ImpersonationLevel);

    CloseHandle(hToken);

    RevertToSelf();

    ret = WriteFile(hPipeServer, &dummy, sizeof(dummy), &dwBytesWritten, NULL);
    ok(ret, "WriteFile failed with error %d\n", GetLastError());

    ret = ReadFile(hPipeServer, buffer, sizeof(buffer), &dwBytesRead, NULL);
    ok(ret, "ReadFile failed with error %d\n", GetLastError());

    ret = ImpersonateNamedPipeClient(hPipeServer);
    ok(ret, "ImpersonateNamedPipeClient failed with error %d\n", GetLastError());

    ret = OpenThreadToken(GetCurrentThread(), TOKEN_QUERY, FALSE, &hToken);
    ok(ret, "OpenThreadToken failed with error %d\n", GetLastError());

    (*test_func)(1, hToken);

    CloseHandle(hToken);

    RevertToSelf();

    ret = WriteFile(hPipeServer, &dummy, sizeof(dummy), &dwBytesWritten, NULL);
    ok(ret, "WriteFile failed with error %d\n", GetLastError());

    WaitForSingleObject(hThread, INFINITE);

    ret = ImpersonateNamedPipeClient(hPipeServer);
    ok(ret, "ImpersonateNamedPipeClient failed with error %d\n", GetLastError());

    RevertToSelf();

    CloseHandle(hThread);
    CloseHandle(hPipeServer);
}

static BOOL are_all_privileges_disabled(HANDLE hToken)
{
    BOOL ret;
    TOKEN_PRIVILEGES *Privileges = NULL;
    DWORD Size = 0;
    BOOL all_privs_disabled = TRUE;
    DWORD i;

    ret = GetTokenInformation(hToken, TokenPrivileges, NULL, 0, &Size);
    if (!ret && GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        Privileges = HeapAlloc(GetProcessHeap(), 0, Size);
        ret = GetTokenInformation(hToken, TokenPrivileges, Privileges, Size, &Size);
        if (!ret)
        {
            HeapFree(GetProcessHeap(), 0, Privileges);
            return FALSE;
        }
    }
    else
        return FALSE;

    for (i = 0; i < Privileges->PrivilegeCount; i++)
    {
        if (Privileges->Privileges[i].Attributes & SE_PRIVILEGE_ENABLED)
        {
            all_privs_disabled = FALSE;
            break;
        }
    }

    HeapFree(GetProcessHeap(), 0, Privileges);

    return all_privs_disabled;
}

static DWORD get_privilege_count(HANDLE hToken)
{
    TOKEN_STATISTICS Statistics;
    DWORD Size = sizeof(Statistics);
    BOOL ret;

    ret = GetTokenInformation(hToken, TokenStatistics, &Statistics, Size, &Size);
    ok(ret, "GetTokenInformation(TokenStatistics)\n");
    if (!ret) return -1;

    return Statistics.PrivilegeCount;
}

static void test_no_sqos_no_token(int call_index, HANDLE hToken)
{
    DWORD priv_count;

    switch (call_index)
    {
    case 0:
        priv_count = get_privilege_count(hToken);
        todo_wine
        ok(priv_count == 0, "privilege count should have been 0 instead of %d\n", priv_count);
        break;
    case 1:
        priv_count = get_privilege_count(hToken);
        ok(priv_count > 0, "privilege count should now be > 0 instead of 0\n");
        ok(!are_all_privileges_disabled(hToken), "impersonated token should not have been modified\n");
        break;
    default:
        ok(0, "shouldn't happen\n");
    }
}

static void test_no_sqos(int call_index, HANDLE hToken)
{
    switch (call_index)
    {
    case 0:
        ok(!are_all_privileges_disabled(hToken), "token should be a copy of the process one\n");
        break;
    case 1:
        todo_wine
        ok(are_all_privileges_disabled(hToken), "impersonated token should have been modified\n");
        break;
    default:
        ok(0, "shouldn't happen\n");
    }
}

static void test_static_context(int call_index, HANDLE hToken)
{
    switch (call_index)
    {
    case 0:
        ok(!are_all_privileges_disabled(hToken), "token should be a copy of the process one\n");
        break;
    case 1:
        ok(!are_all_privileges_disabled(hToken), "impersonated token should not have been modified\n");
        break;
    default:
        ok(0, "shouldn't happen\n");
    }
}

static void test_dynamic_context(int call_index, HANDLE hToken)
{
    switch (call_index)
    {
    case 0:
        ok(!are_all_privileges_disabled(hToken), "token should be a copy of the process one\n");
        break;
    case 1:
        todo_wine
        ok(are_all_privileges_disabled(hToken), "impersonated token should have been modified\n");
        break;
    default:
        ok(0, "shouldn't happen\n");
    }
}

static void test_dynamic_context_no_token(int call_index, HANDLE hToken)
{
    switch (call_index)
    {
    case 0:
        ok(are_all_privileges_disabled(hToken), "token should be a copy of the process one\n");
        break;
    case 1:
        ok(!are_all_privileges_disabled(hToken), "process token modification should have been detected and impersonation token updated\n");
        break;
    default:
        ok(0, "shouldn't happen\n");
    }
}

static void test_no_sqos_revert(int call_index, HANDLE hToken)
{
    DWORD priv_count;
    switch (call_index)
    {
    case 0:
        priv_count = get_privilege_count(hToken);
        todo_wine
        ok(priv_count == 0, "privilege count should have been 0 instead of %d\n", priv_count);
        break;
    case 1:
        priv_count = get_privilege_count(hToken);
        ok(priv_count > 0, "privilege count should now be > 0 instead of 0\n");
        ok(!are_all_privileges_disabled(hToken), "impersonated token should not have been modified\n");
        break;
    default:
        ok(0, "shouldn't happen\n");
    }
}

static void test_static_context_revert(int call_index, HANDLE hToken)
{
    switch (call_index)
    {
    case 0:
        todo_wine
        ok(are_all_privileges_disabled(hToken), "privileges should have been disabled\n");
        break;
    case 1:
        todo_wine
        ok(are_all_privileges_disabled(hToken), "impersonated token should not have been modified\n");
        break;
    default:
        ok(0, "shouldn't happen\n");
    }
}

static void test_dynamic_context_revert(int call_index, HANDLE hToken)
{
    switch (call_index)
    {
    case 0:
        todo_wine
        ok(are_all_privileges_disabled(hToken), "privileges should have been disabled\n");
        break;
    case 1:
        ok(!are_all_privileges_disabled(hToken), "impersonated token should now be process token\n");
        break;
    default:
        ok(0, "shouldn't happen\n");
    }
}

static void test_impersonation(void)
{
    HANDLE hClientToken;
    HANDLE hProcessToken;
    BOOL ret;

    if( !pDuplicateTokenEx ) {
        skip("DuplicateTokenEx not found\n");
        return;
    }

    ret = OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hProcessToken);
    if (!ret)
    {
        skip("couldn't open process token, skipping impersonation tests\n");
        return;
    }

    if (!get_privilege_count(hProcessToken) || are_all_privileges_disabled(hProcessToken))
    {
        skip("token didn't have any privileges or they were all disabled. token not suitable for impersonation tests\n");
        CloseHandle(hProcessToken);
        return;
    }
    CloseHandle(hProcessToken);

    test_ImpersonateNamedPipeClient(NULL, 0, FALSE, test_no_sqos_no_token);
    hClientToken = make_impersonation_token(TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, SecurityImpersonation);
    test_ImpersonateNamedPipeClient(hClientToken, 0, FALSE, test_no_sqos);
    CloseHandle(hClientToken);
    hClientToken = make_impersonation_token(TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, SecurityImpersonation);
    test_ImpersonateNamedPipeClient(hClientToken,
        SECURITY_SQOS_PRESENT | SECURITY_IMPERSONATION, FALSE,
        test_static_context);
    CloseHandle(hClientToken);
    hClientToken = make_impersonation_token(TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, SecurityImpersonation);
    test_ImpersonateNamedPipeClient(hClientToken,
        SECURITY_SQOS_PRESENT | SECURITY_CONTEXT_TRACKING | SECURITY_IMPERSONATION,
        FALSE, test_dynamic_context);
    CloseHandle(hClientToken);
    test_ImpersonateNamedPipeClient(NULL,
        SECURITY_SQOS_PRESENT | SECURITY_CONTEXT_TRACKING | SECURITY_IMPERSONATION,
        FALSE, test_dynamic_context_no_token);

    hClientToken = make_impersonation_token(TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, SecurityImpersonation);
    test_ImpersonateNamedPipeClient(hClientToken, 0, TRUE, test_no_sqos_revert);
    CloseHandle(hClientToken);
    hClientToken = make_impersonation_token(TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, SecurityImpersonation);
    test_ImpersonateNamedPipeClient(hClientToken,
        SECURITY_SQOS_PRESENT | SECURITY_IMPERSONATION, TRUE,
        test_static_context_revert);
    CloseHandle(hClientToken);
    hClientToken = make_impersonation_token(TOKEN_IMPERSONATE | TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, SecurityImpersonation);
    test_ImpersonateNamedPipeClient(hClientToken,
        SECURITY_SQOS_PRESENT | SECURITY_CONTEXT_TRACKING | SECURITY_IMPERSONATION,
        TRUE, test_dynamic_context_revert);
    CloseHandle(hClientToken);
}

struct overlapped_server_args
{
    HANDLE pipe_created;
};

static DWORD CALLBACK overlapped_server(LPVOID arg)
{
    OVERLAPPED ol;
    HANDLE pipe;
    int ret, err;
    struct overlapped_server_args *a = arg;
    DWORD num;
    char buf[100];

    pipe = CreateNamedPipeA("\\\\.\\pipe\\my pipe", FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX, PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE, 1, 0, 0, 100000, NULL);
    ok(pipe != NULL, "pipe NULL\n");

    ol.hEvent = CreateEventA(0, 1, 0, 0);
    ok(ol.hEvent != NULL, "event NULL\n");
    ret = ConnectNamedPipe(pipe, &ol);
    err = GetLastError();
    ok(ret == 0, "ret %d\n", ret);
    ok(err == ERROR_IO_PENDING, "gle %d\n", err);
    SetEvent(a->pipe_created);

    ret = WaitForSingleObjectEx(ol.hEvent, INFINITE, 1);
    ok(ret == WAIT_OBJECT_0, "ret %x\n", ret);

    ret = GetOverlappedResult(pipe, &ol, &num, 1);
    ok(ret == 1, "ret %d\n", ret);

    /* This should block */
    ret = ReadFile(pipe, buf, sizeof(buf), &num, NULL);
    ok(ret == 1, "ret %d\n", ret);

    DisconnectNamedPipe(pipe);
    CloseHandle(ol.hEvent);
    CloseHandle(pipe);
    return 1;
}

static void test_overlapped(void)
{
    DWORD tid, num;
    HANDLE thread, pipe;
    BOOL ret;
    struct overlapped_server_args args;

    args.pipe_created = CreateEventA(0, 1, 0, 0);
    thread = CreateThread(NULL, 0, overlapped_server, &args, 0, &tid);

    WaitForSingleObject(args.pipe_created, INFINITE);
    pipe = CreateFileA("\\\\.\\pipe\\my pipe", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(pipe != INVALID_HANDLE_VALUE, "cf failed\n");

    /* Sleep to try to get the ReadFile in the server to occur before the following WriteFile */
    Sleep(1);

    ret = WriteFile(pipe, "x", 1, &num, NULL);
    ok(ret, "WriteFile failed with error %d\n", GetLastError());

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(pipe);
    CloseHandle(args.pipe_created);
    CloseHandle(thread);
}

static void test_nowait(int pipemode)
{
    HANDLE hnp;
    HANDLE hFile;
    static const char obuf[] = "Bit Bucket";
    char ibuf[32];
    DWORD written;
    DWORD readden;
    DWORD lpmode;

    hnp = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
                           pipemode | PIPE_NOWAIT,
                           /* nMaxInstances */ 1,
                           /* nOutBufSize */ 1024,
                           /* nInBufSize */ 1024,
                           /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
                           /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError());

    /* don't try to do i/o if one side couldn't be opened, as it hangs */
    if (hFile != INVALID_HANDLE_VALUE)
    {
        /* send message from client to server */
        ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len\n");
        ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, NULL), "Peek\n");
        ok(readden == sizeof(obuf), "got %d bytes\n", readden);

        memset(ibuf, 0, sizeof(ibuf));
        ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() failed: %08x\n", GetLastError());
        ok(readden == sizeof(obuf), "got %d bytes\n", readden);
        ok(memcmp(obuf, ibuf, sizeof(obuf)) == 0, "content check\n");

        readden = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ok(!ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() succeeded\n");
        ok(readden == 0, "got %d bytes\n", readden);
        ok(GetLastError() == ERROR_NO_DATA, "GetLastError() returned %08x, expected ERROR_NO_DATA\n", GetLastError());

        lpmode = (pipemode & PIPE_READMODE_MESSAGE) | PIPE_NOWAIT;
        ok(SetNamedPipeHandleState(hFile, &lpmode, NULL, NULL), "Change mode\n");

        /* send message from server to client */
        ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len\n");
        ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, NULL), "Peek\n");
        ok(readden == sizeof(obuf), "got %d bytes\n", readden);

        memset(ibuf, 0, sizeof(ibuf));
        ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() failed: %08x\n", GetLastError());
        ok(readden == sizeof(obuf), "got %d bytes\n", readden);
        ok(memcmp(obuf, ibuf, sizeof(obuf)) == 0, "content check\n");

        readden = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ok(!ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() succeeded\n");
        ok(readden == 0, "got %d bytes\n", readden);
        ok(GetLastError() == ERROR_NO_DATA, "GetLastError() returned %08x, expected ERROR_NO_DATA\n", GetLastError());

        /* now again the bad zero byte message test */
        ok(WriteFile(hFile, obuf, 0, &written, NULL), "WriteFile\n");
        ok(written == 0, "write file len\n");

        if (pipemode != PIPE_TYPE_BYTE)
        {
            ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() failed: %08x\n", GetLastError());
            ok(readden == 0, "got %d bytes\n", readden);
        }
        else
        {
            SetLastError(0xdeadbeef);
            ok(!ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() succeeded\n");
            ok(readden == 0, "got %d bytes\n", readden);
            ok(GetLastError() == ERROR_NO_DATA, "GetLastError() returned %08x, expected ERROR_NO_DATA\n", GetLastError());
        }

        readden = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ok(!ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() succeeded\n");
        ok(readden == 0, "got %d bytes\n", readden);
        ok(GetLastError() == ERROR_NO_DATA, "GetLastError() returned %08x, expected ERROR_NO_DATA\n", GetLastError());

        /* and the same for the reverse direction */
        ok(WriteFile(hnp, obuf, 0, &written, NULL), "WriteFile\n");
        ok(written == 0, "write file len\n");

        if (pipemode != PIPE_TYPE_BYTE)
        {
            ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() failed: %08x\n", GetLastError());
            ok(readden == 0, "got %d bytes\n", readden);
        }
        else
        {
            SetLastError(0xdeadbeef);
            ok(!ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() succeeded\n");
            ok(readden == 0, "got %d bytes\n", readden);
            ok(GetLastError() == ERROR_NO_DATA, "GetLastError() returned %08x, expected ERROR_NO_DATA\n", GetLastError());
        }

        readden = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ok(!ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile() succeeded\n");
        ok(readden == 0, "got %d bytes\n", readden);
        ok(GetLastError() == ERROR_NO_DATA, "GetLastError() returned %08x, expected ERROR_NO_DATA\n", GetLastError());

        ok(CloseHandle(hFile), "CloseHandle\n");
    }

    ok(CloseHandle(hnp), "CloseHandle\n");

}

static void test_NamedPipeHandleState(void)
{
    HANDLE server, client;
    BOOL ret;
    DWORD state, instances, maxCollectionCount, collectDataTimeout;
    char userName[MAX_PATH];

    server = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
        /* dwOpenMode */ PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(server != INVALID_HANDLE_VALUE, "cf failed\n");
    ret = GetNamedPipeHandleStateA(server, NULL, NULL, NULL, NULL, NULL, 0);
    ok(ret, "GetNamedPipeHandleState failed: %d\n", GetLastError());
    ret = GetNamedPipeHandleStateA(server, &state, &instances, NULL, NULL, NULL,
        0);
    ok(ret, "GetNamedPipeHandleState failed: %d\n", GetLastError());
    if (ret)
    {
        ok(state == 0, "unexpected state %08x\n", state);
        ok(instances == 1, "expected 1 instances, got %d\n", instances);
    }
    /* Some parameters have no meaning, and therefore can't be retrieved,
     * on a local pipe.
     */
    SetLastError(0xdeadbeef);
    ret = GetNamedPipeHandleStateA(server, &state, &instances,
        &maxCollectionCount, &collectDataTimeout, userName,
        sizeof(userName) / sizeof(userName[0]));
    todo_wine
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());
    /* A byte-mode pipe server can't be changed to message mode. */
    state = PIPE_READMODE_MESSAGE;
    SetLastError(0xdeadbeef);
    ret = SetNamedPipeHandleState(server, &state, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    client = CreateFileA(PIPENAME, GENERIC_READ|GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, 0, NULL);
    ok(client != INVALID_HANDLE_VALUE, "cf failed\n");

    state = PIPE_READMODE_BYTE;
    ret = SetNamedPipeHandleState(client, &state, NULL, NULL);
    ok(ret, "SetNamedPipeHandleState failed: %d\n", GetLastError());
    /* A byte-mode pipe client can't be changed to message mode, either. */
    state = PIPE_READMODE_MESSAGE;
    SetLastError(0xdeadbeef);
    ret = SetNamedPipeHandleState(server, &state, NULL, NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_PARAMETER,
       "expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    CloseHandle(client);
    CloseHandle(server);

    server = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
        /* dwOpenMode */ PIPE_TYPE_MESSAGE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(server != INVALID_HANDLE_VALUE, "cf failed\n");
    ret = GetNamedPipeHandleStateA(server, NULL, NULL, NULL, NULL, NULL, 0);
    ok(ret, "GetNamedPipeHandleState failed: %d\n", GetLastError());
    ret = GetNamedPipeHandleStateA(server, &state, &instances, NULL, NULL, NULL,
        0);
    ok(ret, "GetNamedPipeHandleState failed: %d\n", GetLastError());
    if (ret)
    {
        ok(state == 0, "unexpected state %08x\n", state);
        ok(instances == 1, "expected 1 instances, got %d\n", instances);
    }
    /* In contrast to byte-mode pipes, a message-mode pipe server can be
     * changed to byte mode.
     */
    state = PIPE_READMODE_BYTE;
    ret = SetNamedPipeHandleState(server, &state, NULL, NULL);
    ok(ret, "SetNamedPipeHandleState failed: %d\n", GetLastError());

    client = CreateFileA(PIPENAME, GENERIC_READ|GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, 0, NULL);
    ok(client != INVALID_HANDLE_VALUE, "cf failed\n");

    state = PIPE_READMODE_MESSAGE;
    ret = SetNamedPipeHandleState(client, &state, NULL, NULL);
    ok(ret, "SetNamedPipeHandleState failed: %d\n", GetLastError());
    /* A message-mode pipe client can also be changed to byte mode.
     */
    state = PIPE_READMODE_BYTE;
    ret = SetNamedPipeHandleState(client, &state, NULL, NULL);
    ok(ret, "SetNamedPipeHandleState failed: %d\n", GetLastError());

    CloseHandle(client);
    CloseHandle(server);
}

static void test_readfileex_pending(void)
{
    HANDLE server, client, event;
    BOOL ret;
    DWORD err, wait, num_bytes, lpmode;
    OVERLAPPED overlapped;
    char read_buf[1024];
    char write_buf[1024];
    const char long_test_string[] = "12test3456ab";
    const char test_string[] = "test";
    int i;

    server = CreateNamedPipeA(PIPENAME, FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX,
        /* dwOpenMode */ PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(server != INVALID_HANDLE_VALUE, "cf failed\n");

    event = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(event != NULL, "CreateEventA failed\n");

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = event;

    ret = ConnectNamedPipe(server, &overlapped);
    err = GetLastError();
    ok(ret == FALSE, "ConnectNamedPipe succeeded\n");
    ok(err == ERROR_IO_PENDING, "ConnectNamedPipe set error %i\n", err);

    wait = WaitForSingleObject(event, 0);
    ok(wait == WAIT_TIMEOUT, "WaitForSingleObject returned %x\n", wait);

    client = CreateFileA(PIPENAME, GENERIC_READ|GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, 0, NULL);
    ok(client != INVALID_HANDLE_VALUE, "cf failed\n");

    wait = WaitForSingleObject(event, 0);
    ok(wait == WAIT_OBJECT_0, "WaitForSingleObject returned %x\n", wait);

    /* Start a read that can't complete immediately. */
    completion_called = 0;
    ResetEvent(event);
    ret = ReadFileEx(server, read_buf, sizeof(read_buf), &overlapped, completion_routine);
    ok(ret == TRUE, "ReadFileEx failed, err=%i\n", GetLastError());
    ok(completion_called == 0, "completion routine called before ReadFileEx returned\n");
    wait = WaitForSingleObject(event, 100);
    ok(wait == WAIT_TIMEOUT, "WaitForSingleObjectEx returned %x\n", wait);
    ok(completion_called == 0, "completion routine called before WriteFile started\n");

    ret = WriteFile(client, test_string, strlen(test_string), &num_bytes, NULL);
    ok(ret == TRUE, "WriteFile failed\n");
    ok(num_bytes == strlen(test_string), "only %i bytes written\n", num_bytes);

    ok(completion_called == 0, "completion routine called during WriteFile\n");

    wait = WaitForSingleObjectEx(event, 0, TRUE);
    ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObjectEx returned %x\n", wait);

    ok(completion_called == 1, "completion not called after writing pipe\n");
    ok(completion_errorcode == 0, "completion called with error %x\n", completion_errorcode);
    ok(completion_num_bytes == strlen(test_string), "ReadFileEx returned only %d bytes\n", completion_num_bytes);
    ok(completion_lpoverlapped == &overlapped, "completion called with wrong overlapped pointer\n");
    ok(!memcmp(test_string, read_buf, strlen(test_string)), "ReadFileEx read wrong bytes\n");

    /* Make writes until the pipe is full and the write fails */
    memset(write_buf, 0xaa, sizeof(write_buf));
    for (i=0; i<256; i++)
    {
        completion_called = 0;
        ResetEvent(event);
        ret = WriteFileEx(server, write_buf, sizeof(write_buf), &overlapped, completion_routine);
        err = GetLastError();

        ok(completion_called == 0, "completion routine called during WriteFileEx\n");

        wait = WaitForSingleObjectEx(event, 0, TRUE);

        if (wait == WAIT_TIMEOUT)
            /* write couldn't complete immediately, presumably the pipe is full */
            break;

        ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObject returned %x\n", wait);

        ok(ret == TRUE, "WriteFileEx failed, err=%i\n", err);
        ok(completion_errorcode == 0, "completion called with error %x\n", completion_errorcode);
        ok(completion_lpoverlapped == &overlapped, "completion called with wrong overlapped pointer\n");
    }

    ok(ret == TRUE, "WriteFileEx failed, err=%i\n", err);
    ok(completion_called == 0, "completion routine called but wait timed out\n");
    ok(completion_errorcode == 0, "completion called with error %x\n", completion_errorcode);
    ok(completion_lpoverlapped == &overlapped, "completion called with wrong overlapped pointer\n");

    /* free up some space in the pipe */
    ret = ReadFile(client, read_buf, sizeof(read_buf), &num_bytes, NULL);
    ok(ret == TRUE, "ReadFile failed\n");

    ok(completion_called == 0, "completion routine called during ReadFile\n");

    wait = WaitForSingleObjectEx(event, 0, TRUE);
    ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObject returned %x\n", wait);
    if (wait == WAIT_TIMEOUT)
    {
        ret = ReadFile(client, read_buf, sizeof(read_buf), &num_bytes, NULL);
        ok(ret == TRUE, "ReadFile failed\n");
        ok(completion_called == 0, "completion routine called during ReadFile\n");
        wait = WaitForSingleObjectEx(event, 0, TRUE);
        ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObject returned %x\n", wait);
    }

    ok(completion_called == 1, "completion routine not called\n");
    ok(completion_errorcode == 0, "completion called with error %x\n", completion_errorcode);
    ok(completion_lpoverlapped == &overlapped, "completion called with wrong overlapped pointer\n");

    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(INVALID_HANDLE_VALUE, read_buf, 0, &num_bytes, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "wrong error %u\n", GetLastError());
    ok(num_bytes == 0, "expected 0, got %u\n", num_bytes);

    S(U(overlapped)).Offset = 0;
    S(U(overlapped)).OffsetHigh = 0;
    overlapped.Internal = -1;
    overlapped.InternalHigh = -1;
    overlapped.hEvent = event;
    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(server, read_buf, 0, &num_bytes, &overlapped);
    ok(GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %d\n", GetLastError());
    ok(num_bytes == 0, "bytes %u\n", num_bytes);
    ok((NTSTATUS)overlapped.Internal == STATUS_PENDING, "expected STATUS_PENDING, got %#lx\n", overlapped.Internal);
todo_wine
    ok(overlapped.InternalHigh == -1, "expected -1, got %lu\n", overlapped.InternalHigh);

    wait = WaitForSingleObject(event, 100);
    ok(wait == WAIT_TIMEOUT, "WaitForSingleObject returned %x\n", wait);

    num_bytes = 0xdeadbeef;
    ret = WriteFile(client, test_string, 1, &num_bytes, NULL);
    ok(ret, "WriteFile failed\n");
    ok(num_bytes == 1, "bytes %u\n", num_bytes);

    wait = WaitForSingleObject(event, 100);
    ok(wait == WAIT_OBJECT_0, "WaitForSingleObject returned %x\n", wait);

    ok(num_bytes == 1, "bytes %u\n", num_bytes);
    ok((NTSTATUS)overlapped.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == 0, "expected 0, got %lu\n", overlapped.InternalHigh);

    /* read the pending byte and clear the pipe */
    num_bytes = 0xdeadbeef;
    ret = ReadFile(server, read_buf, 1, &num_bytes, &overlapped);
    ok(ret, "ReadFile failed\n");
    ok(num_bytes == 1, "bytes %u\n", num_bytes);

    CloseHandle(client);
    CloseHandle(server);

    /* On Windows versions > 2000 it is not possible to add PIPE_NOWAIT to a byte-mode
     * PIPE after creating. Create a new pipe for the following tests. */
    server = CreateNamedPipeA(PIPENAME, FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX,
        /* dwOpenMode */ PIPE_TYPE_BYTE | PIPE_NOWAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(server != INVALID_HANDLE_VALUE, "cf failed\n");

    client = CreateFileA(PIPENAME, GENERIC_READ|GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, 0, NULL);
    ok(client != INVALID_HANDLE_VALUE, "cf failed\n");

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = event;

    /* Initial check with empty pipe */
    memset(read_buf, 0, sizeof(read_buf));
    completion_called = 0;
    ResetEvent(event);
    ret = ReadFileEx(server, read_buf, 4, &overlapped, completion_routine);
    ok(ret == FALSE, "ReadFileEx succeded\n");
    ok(completion_called == 0, "completion routine called before ReadFileEx returned\n");
    ok(GetLastError() == ERROR_NO_DATA, "expected ERROR_NO_DATA, got %d\n", GetLastError());
    wait = WaitForSingleObjectEx(event, 0, TRUE);
    todo_wine
    ok(wait == WAIT_TIMEOUT, "WaitForSingleObjectEx returned %x\n", wait);
    ok(completion_called == 0, "completion routine called before writing to file\n");

    /* Call ReadFileEx after writing content to the pipe */
    num_bytes = 0xdeadbeef;
    ret = WriteFile(client, long_test_string, strlen(long_test_string), &num_bytes, NULL);
    ok(ret, "WriteFile failed, err=%i\n", GetLastError());
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);

    memset(read_buf, 0, sizeof(read_buf));
    completion_called = 0;
    ResetEvent(event);
    ret = ReadFileEx(server, read_buf, 4, &overlapped, completion_routine);
    ok(ret == TRUE, "ReadFileEx failed, err=%i\n", GetLastError());
    ok(completion_called == 0, "completion routine called before ReadFileEx returned\n");
    wait = WaitForSingleObjectEx(event, 0, TRUE);
    ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObjectEx returned %x\n", wait);
    ok(completion_called == 1, "completion not called after writing pipe\n");
    ok(completion_errorcode == 0, "completion called with error %x\n", completion_errorcode);
    ok(completion_num_bytes == 4, "ReadFileEx returned only %d bytes\n", completion_num_bytes);
    ok(completion_lpoverlapped == &overlapped, "completion called with wrong overlapped pointer\n");

    ret = ReadFile(server, read_buf + 4, sizeof(read_buf) - 4, &num_bytes, NULL);
    ok(ret == TRUE, "ReadFile succeeded\n");
    ok(num_bytes == strlen(long_test_string)-4, "ReadFile returned only %d bytes\n", num_bytes);
    ok(!memcmp(long_test_string, read_buf, strlen(long_test_string)), "ReadFile read wrong bytes\n");

    /* Same again, but read as a single part */
    num_bytes = 0xdeadbeef;
    ret = WriteFile(client, long_test_string, strlen(long_test_string), &num_bytes, NULL);
    ok(ret, "WriteFile failed, err=%i\n", GetLastError());
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);

    memset(read_buf, 0, sizeof(read_buf));
    completion_called = 0;
    ResetEvent(event);
    ret = ReadFileEx(server, read_buf, sizeof(read_buf), &overlapped, completion_routine);
    ok(ret == TRUE, "ReadFileEx failed, err=%i\n", GetLastError());
    ok(completion_called == 0, "completion routine called before ReadFileEx returned\n");
    wait = WaitForSingleObjectEx(event, 0, TRUE);
    ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObjectEx returned %x\n", wait);
    ok(completion_called == 1, "completion not called after writing pipe\n");
    ok(completion_errorcode == 0, "completion called with error %x\n", completion_errorcode);
    ok(completion_num_bytes == strlen(long_test_string), "ReadFileEx returned only %d bytes\n", completion_num_bytes);
    ok(completion_lpoverlapped == &overlapped, "completion called with wrong overlapped pointer\n");
    ok(!memcmp(long_test_string, read_buf, strlen(long_test_string)), "ReadFile read wrong bytes\n");

    /* Check content of overlapped structure */
    memset(read_buf, 0, sizeof(read_buf));
    S(U(overlapped)).Offset = 0;
    S(U(overlapped)).OffsetHigh = 0;
    overlapped.Internal = -1;
    overlapped.InternalHigh = -1;
    overlapped.hEvent = event;
    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(server, read_buf, 4, &num_bytes, &overlapped);
    ok(ret == FALSE, "ReadFile succeeded\n");
    ok(GetLastError() == ERROR_NO_DATA, "expected ERROR_NO_DATA, got %d\n", GetLastError());
    ok(num_bytes == 0, "bytes %u\n", num_bytes);
    ok((NTSTATUS)overlapped.Internal == STATUS_PENDING, "expected STATUS_PENDING, got %#lx\n", overlapped.Internal);
    todo_wine
    ok(overlapped.InternalHigh == -1, "expected -1, got %lu\n", overlapped.InternalHigh);
    wait = WaitForSingleObject(event, 100);
    ok(wait == WAIT_TIMEOUT, "WaitForSingleObjectEx returned %x\n", wait);
    ok((NTSTATUS)overlapped.Internal == STATUS_PENDING, "expected STATUS_PENDING, got %#lx\n", overlapped.Internal);
    todo_wine
    ok(overlapped.InternalHigh == -1, "expected -1, got %lu\n", overlapped.InternalHigh);

    /* Call ReadFile after writing to the pipe */
    num_bytes = 0xdeadbeef;
    ret = WriteFile(client, long_test_string, strlen(long_test_string), &num_bytes, NULL);
    ok(ret, "WriteFile failed, err=%i\n", GetLastError());
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);

    memset(read_buf, 0, sizeof(read_buf));
    S(U(overlapped)).Offset = 0;
    S(U(overlapped)).OffsetHigh = 0;
    overlapped.Internal = -1;
    overlapped.InternalHigh = -1;
    overlapped.hEvent = event;
    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(server, read_buf, 4, &num_bytes, &overlapped);
    ok(ret == TRUE, "ReadFile failed, err=%i\n", GetLastError());
    ok(num_bytes == 4, "bytes %u\n", num_bytes);
    ok((NTSTATUS)overlapped.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == 4, "expected 4, got %lu\n", overlapped.InternalHigh);
    wait = WaitForSingleObject(event, 100);
    ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObject returned %x\n", wait);
    ok((NTSTATUS)overlapped.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %#lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == 4, "expected 4, got %lu\n", overlapped.InternalHigh);

    ret = ReadFile(server, read_buf + 4, sizeof(read_buf) - 4, &num_bytes, NULL);
    ok(ret == TRUE, "ReadFile failed\n");
    ok(num_bytes == strlen(long_test_string)-4, "ReadFile returned only %d bytes\n", num_bytes);
    ok(!memcmp(long_test_string, read_buf, strlen(long_test_string)), "ReadFile read wrong bytes\n");

    /* Same again, but read as a single part */
    num_bytes = 0xdeadbeef;
    ret = WriteFile(client, long_test_string, strlen(long_test_string), &num_bytes, NULL);
    ok(ret, "WriteFile failed, err=%i\n", GetLastError());
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);

    memset(read_buf, 0, sizeof(read_buf));
    S(U(overlapped)).Offset = 0;
    S(U(overlapped)).OffsetHigh = 0;
    overlapped.Internal = -1;
    overlapped.InternalHigh = -1;
    overlapped.hEvent = event;
    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(server, read_buf, sizeof(read_buf), &num_bytes, &overlapped);
    ok(ret == TRUE, "ReadFile failed, err=%i\n", GetLastError());
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);
    ok((NTSTATUS)overlapped.Internal == 0, "expected 0, got %#lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == strlen(long_test_string), "expected %u, got %lu\n", (DWORD)strlen(long_test_string), overlapped.InternalHigh);
    wait = WaitForSingleObject(event, 100);
    ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObject returned %x\n", wait);
    ok((NTSTATUS)overlapped.Internal == 0, "expected 0, got %#lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == strlen(long_test_string), "expected %u, got %lu\n", (DWORD)strlen(long_test_string), overlapped.InternalHigh);
    ok(!memcmp(long_test_string, read_buf, strlen(long_test_string)), "ReadFile read wrong bytes\n");

    CloseHandle(client);
    CloseHandle(server);

    server = CreateNamedPipeA(PIPENAME, FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX,
        /* dwOpenMode */ PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(server != INVALID_HANDLE_VALUE, "cf failed\n");

    client = CreateFileA(PIPENAME, GENERIC_READ|GENERIC_WRITE, 0, NULL,
        OPEN_EXISTING, 0, NULL);
    ok(client != INVALID_HANDLE_VALUE, "cf failed\n");

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = event;

    /* Start a call to ReadFileEx which cannot complete immediately */
    memset(read_buf, 0, sizeof(read_buf));
    completion_called = 0;
    ResetEvent(event);
    ret = ReadFileEx(server, read_buf, 4, &overlapped, completion_routine);
    ok(ret == TRUE, "ReadFileEx failed, err=%i\n", GetLastError());
    ok(completion_called == 0, "completion routine called before ReadFileEx returned\n");
    wait = WaitForSingleObject(event, 100);
    ok(wait == WAIT_TIMEOUT, "WaitForSingleObject returned %x\n", wait);
    ok(completion_called == 0, "completion routine called before WriteFile started\n");

    ret = WriteFile(client, long_test_string, strlen(long_test_string), &num_bytes, NULL);
    ok(ret == TRUE, "WriteFile failed\n");
    ok(num_bytes == strlen(long_test_string), "only %i bytes written\n", num_bytes);
    ok(completion_called == 0, "completion routine called during WriteFile\n");
    wait = WaitForSingleObjectEx(event, 0, TRUE);
    ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObjectEx returned %x\n", wait);

    ok(completion_called == 1, "completion not called after writing pipe\n");
    ok(completion_errorcode == 0, "completion called with error %x\n", completion_errorcode);
    ok(completion_num_bytes == 4, "ReadFileEx returned only %d bytes\n", completion_num_bytes);
    ok(completion_lpoverlapped == &overlapped, "completion called with wrong overlapped pointer\n");
    ok(!memcmp(long_test_string, read_buf, 4), "ReadFileEx read wrong bytes\n");

    ret = ReadFile(server, read_buf + 4, 4, &num_bytes, NULL);
    ok(ret == FALSE, "ReadFile succeeded\n");
    ok(num_bytes == 4, "ReadFile returned only %d bytes\n", num_bytes);
    ok(GetLastError() == ERROR_MORE_DATA, "wrong error\n");
    ret = ReadFile(server, read_buf + 8, sizeof(read_buf) - 8, &num_bytes, NULL);
    ok(ret == TRUE, "ReadFile failed\n");
    ok(num_bytes == strlen(long_test_string)-8, "ReadFile returned only %d bytes\n", num_bytes);
    ok(!memcmp(long_test_string, read_buf, strlen(long_test_string)), "ReadFile read wrong bytes\n");

    /* Call ReadFileEx when there is already some content in the pipe */
    ret = WriteFile(client, long_test_string, strlen(long_test_string), &num_bytes, NULL);
    ok(ret == TRUE, "WriteFile failed\n");
    ok(num_bytes == strlen(long_test_string), "only %i bytes written\n", num_bytes);

    memset(read_buf, 0, sizeof(read_buf));
    completion_called = 0;
    ResetEvent(event);
    ret = ReadFileEx(server, read_buf, 4, &overlapped, completion_routine);
    ok(ret == TRUE, "ReadFileEx failed, err=%i\n", GetLastError());
    ok(completion_called == 0, "completion routine called before ReadFileEx returned\n");
    wait = WaitForSingleObjectEx(event, 0, TRUE);
    ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObjectEx returned %x\n", wait);
    ok(completion_called == 1, "completion not called after writing pipe\n");
    ok(completion_errorcode == 0, "completion called with error %x\n", completion_errorcode);
    ok(completion_num_bytes == 4, "ReadFileEx returned only %d bytes\n", completion_num_bytes);
    ok(completion_lpoverlapped == &overlapped, "completion called with wrong overlapped pointer\n");
    ok(!memcmp(long_test_string, read_buf, 4), "ReadFileEx read wrong bytes\n");

    ret = ReadFile(server, read_buf + 4, sizeof(read_buf) - 4, &num_bytes, NULL);
    ok(ret == TRUE, "ReadFile failed\n");
    ok(num_bytes == strlen(long_test_string)-4, "ReadFile returned only %d bytes\n", num_bytes);
    ok(!memcmp(long_test_string, read_buf, strlen(long_test_string)), "ReadFile read wrong bytes\n");

    /* Check content of overlapped structure */
    memset(read_buf, 0, sizeof(read_buf));
    S(U(overlapped)).Offset = 0;
    S(U(overlapped)).OffsetHigh = 0;
    overlapped.Internal = -1;
    overlapped.InternalHigh = -1;
    overlapped.hEvent = event;
    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(server, read_buf, 4, &num_bytes, &overlapped);
    ok(GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %d\n", GetLastError());
    ok(num_bytes == 0, "bytes %u\n", num_bytes);
    ok((NTSTATUS)overlapped.Internal == STATUS_PENDING, "expected STATUS_PENDING, got %#lx\n", overlapped.Internal);
    todo_wine
    ok(overlapped.InternalHigh == -1, "expected -1, got %lu\n", overlapped.InternalHigh);
    wait = WaitForSingleObject(event, 100);
    ok(wait == WAIT_TIMEOUT, "WaitForSingleObject returned %x\n", wait);
    ok((NTSTATUS)overlapped.Internal == STATUS_PENDING, "expected STATUS_PENDING, got %#lx\n", overlapped.Internal);

    num_bytes = 0xdeadbeef;
    ret = WriteFile(client, long_test_string, strlen(long_test_string), &num_bytes, NULL);
    ok(ret, "WriteFile failed\n");
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);
    wait = WaitForSingleObject(event, 100);
    ok(wait == WAIT_OBJECT_0, "WaitForSingleObject returned %x\n", wait);
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);
    ok((NTSTATUS)overlapped.Internal == STATUS_BUFFER_OVERFLOW, "expected STATUS_BUFFER_OVERFLOW, got %#lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == 4, "expected 4, got %lu\n", overlapped.InternalHigh);

    ret = ReadFile(server, read_buf + 4, sizeof(read_buf) - 4, &num_bytes, NULL);
    ok(ret == TRUE, "ReadFile failed\n");
    ok(num_bytes == strlen(long_test_string)-4, "ReadFile returned only %d bytes\n", num_bytes);
    ok(!memcmp(long_test_string, read_buf, strlen(long_test_string)), "ReadFile read wrong bytes\n");

    /* Call ReadFile when there is already some content in the pipe */
    num_bytes = 0xdeadbeef;
    ret = WriteFile(client, long_test_string, strlen(long_test_string), &num_bytes, NULL);
    ok(ret, "WriteFile failed\n");
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);

    memset(read_buf, 0, sizeof(read_buf));
    S(U(overlapped)).Offset = 0;
    S(U(overlapped)).OffsetHigh = 0;
    overlapped.Internal = -1;
    overlapped.InternalHigh = -1;
    overlapped.hEvent = event;
    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(server, read_buf, 4, &num_bytes, &overlapped);
    ok(ret == FALSE, "ReadFile succeeded\n");
    ok(GetLastError() == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %d\n", GetLastError());
    todo_wine
    ok(num_bytes == 0, "ReadFile returned %d bytes\n", num_bytes);
    wait = WaitForSingleObjectEx(event, 0, TRUE);
    ok(wait == WAIT_OBJECT_0, "WaitForSingleObjectEx returned %x\n", wait);
    todo_wine
    ok(num_bytes == 0, "bytes %u\n", num_bytes);
    ok((NTSTATUS)overlapped.Internal == STATUS_BUFFER_OVERFLOW, "expected STATUS_BUFFER_OVERFLOW, got %#lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == 4, "expected 4, got %lu\n", overlapped.InternalHigh);

    ret = ReadFile(server, read_buf + 4, sizeof(read_buf) - 4, &num_bytes, NULL);
    ok(ret == TRUE, "ReadFile failed\n");
    ok(num_bytes == strlen(long_test_string)-4, "ReadFile returned only %d bytes\n", num_bytes);
    ok(!memcmp(long_test_string, read_buf, strlen(long_test_string)), "ReadFile read wrong bytes\n");

    /* Tests for PIPE_NOWAIT in message mode */
    lpmode = PIPE_READMODE_MESSAGE | PIPE_NOWAIT;
    ok(SetNamedPipeHandleState(server, &lpmode, NULL, NULL), "Change mode\n");

    /* Initial check with empty pipe */
    memset(read_buf, 0, sizeof(read_buf));
    completion_called = 0;
    ResetEvent(event);
    ret = ReadFileEx(server, read_buf, 4, &overlapped, completion_routine);
    ok(ret == FALSE, "ReadFileEx succeded\n");
    ok(completion_called == 0, "completion routine called before ReadFileEx returned\n");
    ok(GetLastError() == ERROR_NO_DATA, "expected ERROR_NO_DATA, got %d\n", GetLastError());
    wait = WaitForSingleObjectEx(event, 0, TRUE);
    todo_wine
    ok(wait == WAIT_TIMEOUT, "WaitForSingleObjectEx returned %x\n", wait);
    ok(completion_called == 0, "completion routine called before writing to file\n");

    /* Call ReadFileEx after writing content to the pipe */
    num_bytes = 0xdeadbeef;
    ret = WriteFile(client, long_test_string, strlen(long_test_string), &num_bytes, NULL);
    ok(ret, "WriteFile failed, err=%i\n", GetLastError());
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);

    memset(read_buf, 0, sizeof(read_buf));
    completion_called = 0;
    ResetEvent(event);
    ret = ReadFileEx(server, read_buf, 4, &overlapped, completion_routine);
    ok(ret == TRUE, "ReadFileEx failed, err=%i\n", GetLastError());
    ok(completion_called == 0, "completion routine called before ReadFileEx returned\n");
    wait = WaitForSingleObjectEx(event, 0, TRUE);
    ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObjectEx returned %x\n", wait);
    ok(completion_called == 1, "completion not called after writing pipe\n");
    ok(completion_errorcode == 0, "completion called with error %x\n", completion_errorcode);
    ok(completion_num_bytes == 4, "ReadFileEx returned only %d bytes\n", completion_num_bytes);
    ok(completion_lpoverlapped == &overlapped, "completion called with wrong overlapped pointer\n");

    ret = ReadFile(server, read_buf + 4, sizeof(read_buf) - 4, &num_bytes, NULL);
    ok(ret == TRUE, "ReadFile succeeded\n");
    ok(num_bytes == strlen(long_test_string)-4, "ReadFile returned only %d bytes\n", num_bytes);
    ok(!memcmp(long_test_string, read_buf, strlen(long_test_string)), "ReadFile read wrong bytes\n");

    /* Same again, but read as a single part */
    num_bytes = 0xdeadbeef;
    ret = WriteFile(client, long_test_string, strlen(long_test_string), &num_bytes, NULL);
    ok(ret, "WriteFile failed, err=%i\n", GetLastError());
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);

    memset(read_buf, 0, sizeof(read_buf));
    completion_called = 0;
    ResetEvent(event);
    ret = ReadFileEx(server, read_buf, sizeof(read_buf), &overlapped, completion_routine);
    ok(ret == TRUE, "ReadFileEx failed, err=%i\n", GetLastError());
    ok(completion_called == 0, "completion routine called before ReadFileEx returned\n");
    wait = WaitForSingleObjectEx(event, 0, TRUE);
    ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObjectEx returned %x\n", wait);
    ok(completion_called == 1, "completion not called after writing pipe\n");
    ok(completion_errorcode == 0, "completion called with error %x\n", completion_errorcode);
    ok(completion_num_bytes == strlen(long_test_string), "ReadFileEx returned only %d bytes\n", completion_num_bytes);
    ok(completion_lpoverlapped == &overlapped, "completion called with wrong overlapped pointer\n");
    ok(!memcmp(long_test_string, read_buf, strlen(long_test_string)), "ReadFile read wrong bytes\n");

    /* Check content of overlapped structure */
    memset(read_buf, 0, sizeof(read_buf));
    S(U(overlapped)).Offset = 0;
    S(U(overlapped)).OffsetHigh = 0;
    overlapped.Internal = -1;
    overlapped.InternalHigh = -1;
    overlapped.hEvent = event;
    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(server, read_buf, 4, &num_bytes, &overlapped);
    ok(ret == FALSE, "ReadFile succeeded\n");
    ok(GetLastError() == ERROR_NO_DATA, "expected ERROR_NO_DATA, got %d\n", GetLastError());
    ok(num_bytes == 0, "bytes %u\n", num_bytes);
    ok((NTSTATUS)overlapped.Internal == STATUS_PENDING, "expected STATUS_PENDING, got %#lx\n", overlapped.Internal);
    todo_wine
    ok(overlapped.InternalHigh == -1, "expected -1, got %lu\n", overlapped.InternalHigh);
    wait = WaitForSingleObject(event, 100);
    ok(wait == WAIT_TIMEOUT, "WaitForSingleObjectEx returned %x\n", wait);
    ok((NTSTATUS)overlapped.Internal == STATUS_PENDING, "expected STATUS_PENDING, got %#lx\n", overlapped.Internal);
    todo_wine
    ok(overlapped.InternalHigh == -1, "expected -1, got %lu\n", overlapped.InternalHigh);

    /* Call ReadFile after writing to the pipe */
    num_bytes = 0xdeadbeef;
    ret = WriteFile(client, long_test_string, strlen(long_test_string), &num_bytes, NULL);
    ok(ret, "WriteFile failed, err=%i\n", GetLastError());
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);

    memset(read_buf, 0, sizeof(read_buf));
    S(U(overlapped)).Offset = 0;
    S(U(overlapped)).OffsetHigh = 0;
    overlapped.Internal = -1;
    overlapped.InternalHigh = -1;
    overlapped.hEvent = event;
    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(server, read_buf, 4, &num_bytes, &overlapped);
    ok(ret == FALSE, "ReadFile succeeded\n");
    ok(GetLastError() == ERROR_MORE_DATA, "expected ERROR_MORE_DATA, got %d\n", GetLastError());
    todo_wine
    ok(num_bytes == 0, "bytes %u\n", num_bytes);
    ok((NTSTATUS)overlapped.Internal == STATUS_BUFFER_OVERFLOW, "expected STATUS_BUFFER_OVERFLOW, got %#lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == 4, "expected 4, got %lu\n", overlapped.InternalHigh);
    wait = WaitForSingleObject(event, 100);
    ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObject returned %x\n", wait);
    ok((NTSTATUS)overlapped.Internal == STATUS_BUFFER_OVERFLOW, "expected STATUS_BUFFER_OVERFLOW, got %#lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == 4, "expected 4, got %lu\n", overlapped.InternalHigh);

    ret = ReadFile(server, read_buf + 4, sizeof(read_buf) - 4, &num_bytes, NULL);
    ok(ret == TRUE, "ReadFile failed\n");
    ok(num_bytes == strlen(long_test_string)-4, "ReadFile returned only %d bytes\n", num_bytes);
    ok(!memcmp(long_test_string, read_buf, strlen(long_test_string)), "ReadFile read wrong bytes\n");

    /* Same again, but read as a single part */
    num_bytes = 0xdeadbeef;
    ret = WriteFile(client, long_test_string, strlen(long_test_string), &num_bytes, NULL);
    ok(ret, "WriteFile failed, err=%i\n", GetLastError());
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);

    memset(read_buf, 0, sizeof(read_buf));
    S(U(overlapped)).Offset = 0;
    S(U(overlapped)).OffsetHigh = 0;
    overlapped.Internal = -1;
    overlapped.InternalHigh = -1;
    overlapped.hEvent = event;
    num_bytes = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(server, read_buf, sizeof(read_buf), &num_bytes, &overlapped);
    ok(ret == TRUE, "ReadFile failed, err=%i\n", GetLastError());
    ok(num_bytes == strlen(long_test_string), "bytes %u\n", num_bytes);
    ok((NTSTATUS)overlapped.Internal == 0, "expected 0, got %#lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == strlen(long_test_string), "expected %u, got %lu\n", (DWORD)strlen(long_test_string), overlapped.InternalHigh);
    wait = WaitForSingleObject(event, 100);
    ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0, "WaitForSingleObject returned %x\n", wait);
    ok((NTSTATUS)overlapped.Internal == 0, "expected 0, got %#lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == strlen(long_test_string), "expected %u, got %lu\n", (DWORD)strlen(long_test_string), overlapped.InternalHigh);
    ok(!memcmp(long_test_string, read_buf, strlen(long_test_string)), "ReadFile read wrong bytes\n");

    CloseHandle(client);
    CloseHandle(server);
    CloseHandle(event);
}

START_TEST(pipe)
{
    HMODULE hmod;

    hmod = GetModuleHandleA("advapi32.dll");
    pDuplicateTokenEx = (void *) GetProcAddress(hmod, "DuplicateTokenEx");
    hmod = GetModuleHandleA("kernel32.dll");
    pQueueUserAPC = (void *) GetProcAddress(hmod, "QueueUserAPC");

    if (test_DisconnectNamedPipe())
        return;
    test_CreateNamedPipe_instances_must_match();
    test_NamedPipe_2();
    test_CreateNamedPipe(PIPE_TYPE_BYTE);
    test_CreateNamedPipe(PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE);
    test_CloseNamedPipe();
    test_CreatePipe();
    test_impersonation();
    test_overlapped();
    test_nowait(PIPE_TYPE_BYTE);
    test_nowait(PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE);
    test_NamedPipeHandleState();
    test_readfileex_pending();
}
