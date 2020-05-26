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
#include "winioctl.h"
#include "wine/test.h"

#define PIPENAME "\\\\.\\PiPe\\tests_pipe.c"
#define PIPENAME_SPECIAL "\\\\.\\PiPe\\tests->pipe.c"

#define NB_SERVER_LOOPS 8

static HANDLE alarm_event;
static BOOL (WINAPI *pDuplicateTokenEx)(HANDLE,DWORD,LPSECURITY_ATTRIBUTES,
                                        SECURITY_IMPERSONATION_LEVEL,TOKEN_TYPE,PHANDLE);
static DWORD (WINAPI *pQueueUserAPC)(PAPCFUNC pfnAPC, HANDLE hThread, ULONG_PTR dwData);
static BOOL (WINAPI *pCancelIoEx)(HANDLE handle, LPOVERLAPPED lpOverlapped);

static BOOL user_apc_ran;
static void CALLBACK user_apc(ULONG_PTR param)
{
    user_apc_ran = TRUE;
}


enum rpcThreadOp
{
    RPC_READFILE
};

struct rpcThreadArgs
{
    ULONG_PTR returnValue;
    DWORD lastError;
    enum rpcThreadOp op;
    ULONG_PTR args[5];
};

static DWORD CALLBACK rpcThreadMain(LPVOID arg)
{
    struct rpcThreadArgs *rpcargs = (struct rpcThreadArgs *)arg;
    if (winetest_debug > 1) trace("rpcThreadMain starting\n");
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

        default:
            SetLastError( ERROR_CALL_NOT_IMPLEMENTED );
            rpcargs->returnValue = 0;
            break;
    }

    rpcargs->lastError = GetLastError();
    if (winetest_debug > 1) trace("rpcThreadMain returning\n");
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

#define test_not_signaled(h) _test_not_signaled(__LINE__,h)
static void _test_not_signaled(unsigned line, HANDLE handle)
{
    DWORD res = WaitForSingleObject(handle, 0);
    disable_success_count
    ok_(__FILE__,line)(res == WAIT_TIMEOUT, "WaitForSingleObject returned %u (%u)\n", res, GetLastError());
}

#define test_signaled(h) _test_signaled(__LINE__,h)
static void _test_signaled(unsigned line, HANDLE handle)
{
    DWORD res = WaitForSingleObject(handle, 0);
    ok_(__FILE__,line)(res == WAIT_OBJECT_0, "WaitForSingleObject returned %u\n", res);
}

#define test_pipe_info(a,b,c,d,e) _test_pipe_info(__LINE__,a,b,c,d,e)
static void _test_pipe_info(unsigned line, HANDLE pipe, DWORD ex_flags, DWORD ex_out_buf_size, DWORD ex_in_buf_size, DWORD ex_max_instances)
{
    DWORD flags = 0xdeadbeef, out_buf_size = 0xdeadbeef, in_buf_size = 0xdeadbeef, max_instances = 0xdeadbeef;
    BOOL res;

    res = GetNamedPipeInfo(pipe, &flags, &out_buf_size, &in_buf_size, &max_instances);
    ok_(__FILE__,line)(res, "GetNamedPipeInfo failed: %x\n", res);
    ok_(__FILE__,line)(flags == ex_flags, "flags = %x, expected %x\n", flags, ex_flags);
    ok_(__FILE__,line)(out_buf_size == ex_out_buf_size, "out_buf_size = %x, expected %u\n", out_buf_size, ex_out_buf_size);
    ok_(__FILE__,line)(in_buf_size == ex_in_buf_size, "in_buf_size = %x, expected %u\n", in_buf_size, ex_in_buf_size);
    ok_(__FILE__,line)(max_instances == ex_max_instances, "max_instances = %x, expected %u\n", max_instances, ex_max_instances);
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
    DWORD avail;
    DWORD left;
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
    test_signaled(hnp);

    ret = PeekNamedPipe(hnp, NULL, 0, NULL, &readden, NULL);
    todo_wine
    ok(!ret && GetLastError() == ERROR_BAD_PIPE, "PeekNamedPipe returned %x (%u)\n",
       ret, GetLastError());

    ret = WaitNamedPipeA(PIPENAME, 2000);
    ok(ret, "WaitNamedPipe failed (%d)\n", GetLastError());

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile failed (%d)\n", GetLastError());

    ok(!WaitNamedPipeA(PIPENAME, 100), "WaitNamedPipe succeeded\n");

    ok(GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %u\n", GetLastError());

    /* Test ConnectNamedPipe() in both directions */
    ok(!ConnectNamedPipe(hnp, NULL), "ConnectNamedPipe(server) succeeded\n");
    ok(GetLastError() == ERROR_PIPE_CONNECTED, "expected ERROR_PIPE_CONNECTED, got %u\n", GetLastError());
    ok(!ConnectNamedPipe(hFile, NULL), "ConnectNamedPipe(client) succeeded\n");
    ok(GetLastError() == ERROR_INVALID_FUNCTION, "expected ERROR_INVALID_FUNCTION, got %u\n", GetLastError());

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
        ok(PeekNamedPipe(hFile, NULL, 0, NULL, &avail, &left), "Peek\n");
        ok(avail == sizeof(obuf), "peek 1 got %d bytes\n", avail);
        if (pipemode == PIPE_TYPE_BYTE)
            ok(left == 0, "peek 1 got %d bytes left\n", left);
        else
            ok(left == sizeof(obuf), "peek 1 got %d bytes left\n", left);
        ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf), "read 1 got %d bytes\n", readden);
        ok(memcmp(obuf, ibuf, written) == 0, "content 1 check\n");

        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf2, sizeof(obuf2), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf2), "write file len 2\n");
        ok(PeekNamedPipe(hnp, NULL, 0, NULL, &avail, &left), "Peek\n");
        ok(avail == sizeof(obuf2), "peek 2 got %d bytes\n", avail);
        if (pipemode == PIPE_TYPE_BYTE)
            ok(left == 0, "peek 2 got %d bytes left\n", left);
        else
            ok(left == sizeof(obuf2), "peek 2 got %d bytes left\n", left);
        ok(PeekNamedPipe(hnp, (LPVOID)1, 0, NULL, &avail, &left), "Peek\n");
        ok(avail == sizeof(obuf2), "peek 2 got %d bytes\n", avail);
        if (pipemode == PIPE_TYPE_BYTE)
            ok(left == 0, "peek 2 got %d bytes left\n", left);
        else
            ok(left == sizeof(obuf2), "peek 2 got %d bytes left\n", left);
        ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf2), "read 2 got %d bytes\n", readden);
        ok(memcmp(obuf2, ibuf, written) == 0, "content 2 check\n");

        /* Test how ReadFile behaves when the buffer is not big enough for the whole message */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf2, sizeof(obuf2), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf2), "write file len\n");
        ok(PeekNamedPipe(hFile, ibuf, 4, &readden, &avail, &left), "Peek\n");
        ok(readden == 4, "peek got %d bytes\n", readden);
        ok(avail == sizeof(obuf2), "peek got %d bytes available\n", avail);
        if (pipemode == PIPE_TYPE_BYTE)
            ok(left == -4, "peek got %d bytes left\n", left);
        else
            ok(left == sizeof(obuf2)-4, "peek got %d bytes left\n", left);
        ok(ReadFile(hFile, ibuf, 4, &readden, NULL), "ReadFile\n");
        ok(readden == 4, "read got %d bytes\n", readden);
        ok(ReadFile(hFile, ibuf + 4, sizeof(ibuf) - 4, &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf2) - 4, "read got %d bytes\n", readden);
        ok(memcmp(obuf2, ibuf, written) == 0, "content check\n");

        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len\n");
        ok(PeekNamedPipe(hnp, ibuf, 4, &readden, &avail, &left), "Peek\n");
        ok(readden == 4, "peek got %d bytes\n", readden);
        ok(avail == sizeof(obuf), "peek got %d bytes available\n", avail);
        if (pipemode == PIPE_TYPE_BYTE)
        {
            ok(left == -4, "peek got %d bytes left\n", left);
            ok(ReadFile(hnp, ibuf, 4, &readden, NULL), "ReadFile\n");
        }
        else
        {
            ok(left == sizeof(obuf)-4, "peek got %d bytes left\n", left);
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

        /* Test reading of multiple writes */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile3a\n");
        ok(written == sizeof(obuf), "write file len 3a\n");
        ok(WriteFile(hnp, obuf2, sizeof(obuf2), &written, NULL), " WriteFile3b\n");
        ok(written == sizeof(obuf2), "write file len 3b\n");
        ok(PeekNamedPipe(hFile, ibuf, 4, &readden, &avail, &left), "Peek3\n");
        ok(readden == 4, "peek3 got %d bytes\n", readden);
        if (pipemode == PIPE_TYPE_BYTE)
            ok(left == -4, "peek3 got %d bytes left\n", left);
        else
            ok(left == sizeof(obuf)-4, "peek3 got %d bytes left\n", left);
        ok(avail == sizeof(obuf) + sizeof(obuf2), "peek3 got %d bytes available\n", avail);
        ok(PeekNamedPipe(hFile, ibuf, sizeof(ibuf), &readden, &avail, &left), "Peek3\n");
        if (pipemode == PIPE_TYPE_BYTE) {
            ok(readden == sizeof(obuf) + sizeof(obuf2), "peek3 got %d bytes\n", readden);
            ok(left == (DWORD) -(sizeof(obuf) + sizeof(obuf2)), "peek3 got %d bytes left\n", left);
        }
        else
        {
            ok(readden == sizeof(obuf), "peek3 got %d bytes\n", readden);
            ok(left == 0, "peek3 got %d bytes left\n", left);
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
        ok(PeekNamedPipe(hnp, ibuf, 4, &readden, &avail, &left), "Peek3\n");
        ok(readden == 4, "peek3 got %d bytes\n", readden);
        if (pipemode == PIPE_TYPE_BYTE)
            ok(left == -4, "peek3 got %d bytes left\n", left);
        else
            ok(left == sizeof(obuf)-4, "peek3 got %d bytes left\n", left);
        ok(avail == sizeof(obuf) + sizeof(obuf2), "peek3 got %d bytes available\n", avail);
        ok(PeekNamedPipe(hnp, ibuf, sizeof(ibuf), &readden, &avail, &left), "Peek4\n");
        if (pipemode == PIPE_TYPE_BYTE) {
            ok(readden == sizeof(obuf) + sizeof(obuf2), "peek4 got %d bytes\n", readden);
            ok(left == (DWORD) -(sizeof(obuf) + sizeof(obuf2)), "peek4 got %d bytes left\n", left);
        }
        else
        {
            ok(readden == sizeof(obuf), "peek4 got %d bytes\n", readden);
            ok(left == 0, "peek4 got %d bytes left\n", left);
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
            ok(PeekNamedPipe(hFile, ibuf, sizeof(ibuf), &readden, &avail, &left), "Peek5\n");
            ok(readden == sizeof(obuf), "peek5 got %d bytes\n", readden);
            ok(avail == sizeof(obuf) + sizeof(obuf2), "peek5 got %d bytes available\n", avail);
            ok(left == 0, "peek5 got %d bytes left\n", left);
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
            SetLastError(0xdeadbeef);
            ok(!ReadFile(hFile, ibuf, 4, &readden, NULL), "ReadFile 9\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error 9\n");
            ok(readden == 4, "read got %d bytes 9\n", readden);
            SetLastError(0xdeadbeef);
            ret = RpcReadFile(hFile, ibuf + 4, 4, &readden, NULL);
            ok(!ret, "RpcReadFile 9\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error 9\n");
            ok(readden == 4, "read got %d bytes 9\n", readden);
            ret = RpcReadFile(hFile, ibuf + 8, sizeof(ibuf), &readden, NULL);
            ok(ret, "RpcReadFile 9\n");
            ok(readden == sizeof(obuf) - 8, "read got %d bytes 9\n", readden);
            ok(memcmp(obuf, ibuf, sizeof(obuf)) == 0, "content check 9\n");
            if (readden <= sizeof(obuf) - 8) /* blocks forever if second part was already received */
            {
                memset(ibuf, 0, sizeof(ibuf));
                SetLastError(0xdeadbeef);
                ret = RpcReadFile(hFile, ibuf, 4, &readden, NULL);
                ok(!ret, "RpcReadFile 9\n");
                ok(GetLastError() == ERROR_MORE_DATA, "wrong error 9\n");
                ok(readden == 4, "read got %d bytes 9\n", readden);
                SetLastError(0xdeadbeef);
                ok(!ReadFile(hFile, ibuf + 4, 4, &readden, NULL), "ReadFile 9\n");
                ok(GetLastError() == ERROR_MORE_DATA, "wrong error 9\n");
                ok(readden == 4, "read got %d bytes 9\n", readden);
                ret = RpcReadFile(hFile, ibuf + 8, sizeof(ibuf), &readden, NULL);
                ok(ret, "RpcReadFile 9\n");
                ok(readden == sizeof(obuf2) - 8, "read got %d bytes 9\n", readden);
                ok(memcmp(obuf2, ibuf, sizeof(obuf2)) == 0, "content check 9\n");
            }

            /* Now the reverse direction */
            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hFile, obuf2, sizeof(obuf2), &written, NULL), "WriteFile 10\n");
            ok(written == sizeof(obuf2), "write file len 10\n");
            ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile 10\n");
            ok(written == sizeof(obuf), "write file len 10\n");
            SetLastError(0xdeadbeef);
            ok(!ReadFile(hnp, ibuf, 4, &readden, NULL), "ReadFile 10\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error 10\n");
            ok(readden == 4, "read got %d bytes 10\n", readden);
            SetLastError(0xdeadbeef);
            ret = RpcReadFile(hnp, ibuf + 4, 4, &readden, NULL);
            ok(!ret, "RpcReadFile 10\n");
            ok(GetLastError() == ERROR_MORE_DATA, "wrong error 10\n");
            ok(readden == 4, "read got %d bytes 10\n", readden);
            ret = RpcReadFile(hnp, ibuf + 8, sizeof(ibuf), &readden, NULL);
            ok(ret, "RpcReadFile 10\n");
            ok(readden == sizeof(obuf2) - 8, "read got %d bytes 10\n", readden);
            ok(memcmp(obuf2, ibuf, sizeof(obuf2)) == 0, "content check 10\n");
            if (readden <= sizeof(obuf2) - 8) /* blocks forever if second part was already received */
            {
                memset(ibuf, 0, sizeof(ibuf));
                SetLastError(0xdeadbeef);
                ret = RpcReadFile(hnp, ibuf, 4, &readden, NULL);
                ok(!ret, "RpcReadFile 10\n");
                ok(GetLastError() == ERROR_MORE_DATA, "wrong error 10\n");
                ok(readden == 4, "read got %d bytes 10\n", readden);
                SetLastError(0xdeadbeef);
                ok(!ReadFile(hnp, ibuf + 4, 4, &readden, NULL), "ReadFile 10\n");
                ok(GetLastError() == ERROR_MORE_DATA, "wrong error 10\n");
                ok(readden == 4, "read got %d bytes 10\n", readden);
                ret = RpcReadFile(hnp, ibuf + 8, sizeof(ibuf), &readden, NULL);
                ok(ret, "RpcReadFile 10\n");
                ok(readden == sizeof(obuf) - 8, "read got %d bytes 10\n", readden);
                ok(memcmp(obuf, ibuf, sizeof(obuf)) == 0, "content check 10\n");
            }

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

    if (winetest_debug > 1) trace("test_CreateNamedPipe returning\n");
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

static void test_ReadFile(void)
{
    HANDLE server, client;
    OVERLAPPED overlapped;
    DWORD size;
    BOOL res;

    static char buf[512];

    server = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
                              PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                              1, 1024, 1024, NMPWAIT_WAIT_FOREVER, NULL);
    ok(server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed with %u\n", GetLastError());

    client = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                         OPEN_EXISTING, 0, 0);
    ok(client != INVALID_HANDLE_VALUE, "CreateFile failed with %u\n", GetLastError());

    ok(WriteFile(client, buf, sizeof(buf), &size, NULL), "WriteFile\n");

    res = ReadFile(server, buf, 1, &size, NULL);
    ok(!res && GetLastError() == ERROR_MORE_DATA, "ReadFile returned %x(%u)\n", res, GetLastError());
    ok(size == 1, "size = %u\n", size);

    /* pass both overlapped and ret read */
    memset(&overlapped, 0, sizeof(overlapped));
    res = ReadFile(server, buf, 1, &size, &overlapped);
    ok(!res && GetLastError() == ERROR_MORE_DATA, "ReadFile returned %x(%u)\n", res, GetLastError());
    ok(size == 0, "size = %u\n", size);
    ok((NTSTATUS)overlapped.Internal == STATUS_BUFFER_OVERFLOW, "Internal = %lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == 1, "InternalHigh = %lx\n", overlapped.InternalHigh);

    DisconnectNamedPipe(server);

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.InternalHigh = 0xdeadbeef;
    res = ReadFile(server, buf, 1, &size, &overlapped);
    ok(!res && GetLastError() == ERROR_PIPE_NOT_CONNECTED, "ReadFile returned %x(%u)\n", res, GetLastError());
    ok(size == 0, "size = %u\n", size);
    ok(overlapped.Internal == STATUS_PENDING, "Internal = %lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == 0xdeadbeef, "InternalHigh = %lx\n", overlapped.InternalHigh);

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.InternalHigh = 0xdeadbeef;
    res = WriteFile(server, buf, 1, &size, &overlapped);
    ok(!res && GetLastError() == ERROR_PIPE_NOT_CONNECTED, "ReadFile returned %x(%u)\n", res, GetLastError());
    ok(size == 0, "size = %u\n", size);
    ok(overlapped.Internal == STATUS_PENDING, "Internal = %lx\n", overlapped.Internal);
    ok(overlapped.InternalHigh == 0xdeadbeef, "InternalHigh = %lx\n", overlapped.InternalHigh);

    CloseHandle(server);
    CloseHandle(client);
}

/** implementation of alarm() */
static DWORD CALLBACK alarmThreadMain(LPVOID arg)
{
    DWORD_PTR timeout = (DWORD_PTR) arg;
    if (winetest_debug > 1) trace("alarmThreadMain\n");
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

    if (winetest_debug > 1) trace("serverThreadMain1 start\n");
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
        if (winetest_debug > 1) trace("Server calling ConnectNamedPipe...\n");
        ok(ConnectNamedPipe(hnp, NULL)
            || GetLastError() == ERROR_PIPE_CONNECTED, "ConnectNamedPipe\n");
        if (winetest_debug > 1) trace("ConnectNamedPipe returned.\n");

        /* Echo bytes once */
        memset(buf, 0, sizeof(buf));

        if (winetest_debug > 1) trace("Server reading...\n");
        success = ReadFile(hnp, buf, sizeof(buf), &readden, NULL);
        if (winetest_debug > 1) trace("Server done reading.\n");
        ok(success, "ReadFile\n");
        ok(readden, "short read\n");

        if (winetest_debug > 1) trace("Server writing...\n");
        ok(WriteFile(hnp, buf, readden, &written, NULL), "WriteFile\n");
        if (winetest_debug > 1) trace("Server done writing.\n");
        ok(written == readden, "write file len\n");

        /* finish this connection, wait for next one */
        ok(FlushFileBuffers(hnp), "FlushFileBuffers\n");
        if (winetest_debug > 1) trace("Server done flushing.\n");
        ok(DisconnectNamedPipe(hnp), "DisconnectNamedPipe\n");
        if (winetest_debug > 1) trace("Server done disconnecting.\n");
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
            if (winetest_debug > 1) trace("Queueing an user APC\n"); /* verify the pipe is non alerable */
            ret = pQueueUserAPC(&user_apc, GetCurrentThread(), 0);
            ok(ret, "QueueUserAPC failed: %d\n", GetLastError());
        }

        /* Wait for client to connect */
        if (winetest_debug > 1) trace("Server calling ConnectNamedPipe...\n");
        ok(ConnectNamedPipe(hnp, NULL)
            || GetLastError() == ERROR_PIPE_CONNECTED, "ConnectNamedPipe\n");
        if (winetest_debug > 1) trace("ConnectNamedPipe returned.\n");

        /* Echo bytes once */
        memset(buf, 0, sizeof(buf));

        if (winetest_debug > 1) trace("Server reading...\n");
        success = ReadFile(hnp, buf, sizeof(buf), &readden, NULL);
        if (winetest_debug > 1) trace("Server done reading.\n");
        ok(success, "ReadFile\n");

        if (winetest_debug > 1) trace("Server writing...\n");
        ok(WriteFile(hnp, buf, readden, &written, NULL), "WriteFile\n");
        if (winetest_debug > 1) trace("Server done writing.\n");
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

    if (winetest_debug > 1) trace("serverThreadMain3\n");
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
            if (winetest_debug > 1) trace("Server calling non-overlapped ConnectNamedPipe on overlapped pipe...\n");
            success = ConnectNamedPipe(hnp, NULL);
            err = GetLastError();
            ok(success || (err == ERROR_PIPE_CONNECTED), "ConnectNamedPipe failed: %d\n", err);
            if (winetest_debug > 1) trace("ConnectNamedPipe operation complete.\n");
        } else {
            if (winetest_debug > 1) trace("Server calling overlapped ConnectNamedPipe...\n");
            success = ConnectNamedPipe(hnp, &oOverlap);
            err = GetLastError();
            ok(!success && (err == ERROR_IO_PENDING || err == ERROR_PIPE_CONNECTED), "overlapped ConnectNamedPipe\n");
            if (winetest_debug > 1) trace("overlapped ConnectNamedPipe returned.\n");
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
            if (winetest_debug > 1) trace("overlapped ConnectNamedPipe operation complete.\n");
        }

        /* Echo bytes once */
        memset(buf, 0, sizeof(buf));

        if (winetest_debug > 1) trace("Server reading...\n");
        success = ReadFile(hnp, buf, sizeof(buf), &readden, &oOverlap);
        if (winetest_debug > 1) trace("Server ReadFile returned...\n");
        err = GetLastError();
        ok(success || err == ERROR_IO_PENDING, "overlapped ReadFile\n");
        if (winetest_debug > 1) trace("overlapped ReadFile returned.\n");
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
        if (winetest_debug > 1) trace("Server done reading.\n");
        ok(success, "overlapped ReadFile\n");

        if (winetest_debug > 1) trace("Server writing...\n");
        success = WriteFile(hnp, buf, readden, &written, &oOverlap);
        if (winetest_debug > 1) trace("Server WriteFile returned...\n");
        err = GetLastError();
        ok(success || err == ERROR_IO_PENDING, "overlapped WriteFile\n");
        if (winetest_debug > 1) trace("overlapped WriteFile returned.\n");
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
        if (winetest_debug > 1) trace("Server done writing.\n");
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

    if (winetest_debug > 1) trace("serverThreadMain4\n");
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
        if (winetest_debug > 1) trace("Server calling overlapped ConnectNamedPipe...\n");
        success = ConnectNamedPipe(hnp, &oConnect);
        err = GetLastError();
        ok(!success && (err == ERROR_IO_PENDING || err == ERROR_PIPE_CONNECTED),
           "overlapped ConnectNamedPipe got %u err %u\n", success, err );
        if (!success && err == ERROR_IO_PENDING) {
            if (winetest_debug > 1) trace("ConnectNamedPipe GetQueuedCompletionStatus\n");
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
        if (winetest_debug > 1) trace("overlapped ConnectNamedPipe operation complete.\n");

        /* Echo bytes once */
        memset(buf, 0, sizeof(buf));

        if (winetest_debug > 1) trace("Server reading...\n");
        success = ReadFile(hnp, buf, sizeof(buf), &readden, &oRead);
        if (winetest_debug > 1) trace("Server ReadFile returned...\n");
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
        if (winetest_debug > 1) trace("Server done reading.\n");

        if (winetest_debug > 1) trace("Server writing...\n");
        success = WriteFile(hnp, buf, readden, &written, &oWrite);
        if (winetest_debug > 1) trace("Server WriteFile returned...\n");
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
        if (winetest_debug > 1) trace("Server done writing.\n");

        /* Client will finish this connection, the following ops will trigger broken pipe errors. */

        /* Wait for the pipe to break. */
        while (PeekNamedPipe(hnp, NULL, 0, NULL, &written, &written));

        if (winetest_debug > 1) trace("Server writing on disconnected pipe...\n");
        SetLastError(ERROR_SUCCESS);
        success = WriteFile(hnp, buf, readden, &written, &oWrite);
        err = GetLastError();
        todo_wine_if (!success && err == ERROR_PIPE_NOT_CONNECTED) ok(!success && err == ERROR_NO_DATA,
            "overlapped WriteFile on disconnected pipe returned %u, err=%i\n", success, err);

        /* No completion status is queued on immediate error. */
        SetLastError(ERROR_SUCCESS);
        oResult = (OVERLAPPED *)0xdeadbeef;
        success = GetQueuedCompletionStatus(hcompletion, &written, &compkey,
            &oResult, 0);
        err = GetLastError();
        ok(!success && err == WAIT_TIMEOUT && !oResult,
           "WriteFile GetQueuedCompletionStatus returned %u, err=%i, oResult %p\n",
           success, err, oResult);

        if (winetest_debug > 1) trace("Server reading from disconnected pipe...\n");
        SetLastError(ERROR_SUCCESS);
        success = ReadFile(hnp, buf, sizeof(buf), &readden, &oRead);
        if (winetest_debug > 1) trace("Server ReadFile from disconnected pipe returned...\n");
        err = GetLastError();
        ok(!success && err == ERROR_BROKEN_PIPE,
            "overlapped ReadFile on disconnected pipe returned %u, err=%i\n", success, err);

        SetLastError(ERROR_SUCCESS);
        oResult = (OVERLAPPED *)0xdeadbeef;
        success = GetQueuedCompletionStatus(hcompletion, &readden, &compkey,
            &oResult, 0);
        err = GetLastError();
        ok(!success && err == WAIT_TIMEOUT && !oResult,
           "ReadFile GetQueuedCompletionStatus returned %u, err=%i, oResult %p\n",
           success, err, oResult);

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

    if (winetest_debug > 1) trace("serverThreadMain5\n");
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
        if (winetest_debug > 1) trace("Server calling ConnectNamedPipe...\n");
        success = ConnectNamedPipe(hnp, NULL);
        err = GetLastError();
        ok(success || (err == ERROR_PIPE_CONNECTED), "ConnectNamedPipe failed: %d\n", err);
        if (winetest_debug > 1) trace("ConnectNamedPipe operation complete.\n");

        /* Echo bytes once */
        memset(buf, 0, sizeof(buf));

        if (winetest_debug > 1) trace("Server reading...\n");
        completion_called = 0;
        ResetEvent(hEvent);
        success = ReadFileEx(hnp, buf, sizeof(buf), &oOverlap, completion_routine);
        if (winetest_debug > 1) trace("Server ReadFileEx returned...\n");
        ok(success, "ReadFileEx failed, err=%i\n", GetLastError());
        ok(completion_called == 0, "completion routine called before ReadFileEx return\n");
        if (winetest_debug > 1) trace("ReadFileEx returned.\n");
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
        if (winetest_debug > 1) trace("Server done reading.\n");

        if (winetest_debug > 1) trace("Server writing...\n");
        completion_called = 0;
        ResetEvent(hEvent);
        success = WriteFileEx(hnp, buf, readden, &oOverlap, completion_routine);
        if (winetest_debug > 1) trace("Server WriteFileEx returned...\n");
        ok(success, "WriteFileEx failed, err=%i\n", GetLastError());
        ok(completion_called == 0, "completion routine called before ReadFileEx return\n");
        if (winetest_debug > 1) trace("overlapped WriteFile returned.\n");
        if (success) {
            DWORD ret;
            do {
                ret = WaitForSingleObjectEx(hEvent, INFINITE, TRUE);
            } while (ret == WAIT_IO_COMPLETION);
            ok(ret == 0, "wait WriteFileEx returned %x\n", ret);
        }
        if (winetest_debug > 1) trace("Server done writing.\n");
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

    if (winetest_debug > 1) trace("exercizeServer starting\n");
    for (i = 0; i < NB_SERVER_LOOPS; i++) {
        HANDLE hFile=INVALID_HANDLE_VALUE;
        static const char obuf[] = "Bit Bucket";
        char ibuf[32];
        DWORD written;
        DWORD readden;
        int loop;

        for (loop = 0; loop < 3; loop++) {
	    DWORD err;
            if (winetest_debug > 1) trace("Client connecting...\n");
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
            if (winetest_debug > 1) trace("connect failed, retrying\n");
            Sleep(200);
        }
        ok(hFile != INVALID_HANDLE_VALUE, "client opening named pipe\n");

        /* Make sure it can echo */
        memset(ibuf, 0, sizeof(ibuf));
        if (winetest_debug > 1) trace("Client writing...\n");
        ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile to client end of pipe\n");
        ok(written == sizeof(obuf), "write file len\n");
        if (winetest_debug > 1) trace("Client reading...\n");
        ok(ReadFile(hFile, ibuf, sizeof(obuf), &readden, NULL), "ReadFile from client end of pipe\n");
        ok(readden == sizeof(obuf), "read file len\n");
        ok(memcmp(obuf, ibuf, written) == 0, "content check\n");

        if (winetest_debug > 1) trace("Client closing...\n");
        ok(CloseHandle(hFile), "CloseHandle\n");
    }

    ok(WaitForSingleObject(serverThread,INFINITE) == WAIT_OBJECT_0, "WaitForSingleObject\n");
    CloseHandle(hnp);
    if (winetest_debug > 1) trace("exercizeServer returning\n");
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
    if (winetest_debug > 1) trace("test_NamedPipe_2 returning\n");
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

        ret = PeekNamedPipe(hFile, NULL, 0, NULL, &readden, NULL);
        todo_wine
        ok(!ret && GetLastError() == ERROR_PIPE_NOT_CONNECTED, "PeekNamedPipe returned %x (%u)\n",
           ret, GetLastError());
        ret = PeekNamedPipe(hnp, NULL, 0, NULL, &readden, NULL);
        todo_wine
        ok(!ret && GetLastError() == ERROR_BAD_PIPE, "PeekNamedPipe returned %x (%u)\n",
           ret, GetLastError());
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
    test_pipe_info(piperead, FILE_PIPE_SERVER_END, 4096, 4096, 1);
    test_pipe_info(pipewrite, 0, 4096, 4096, 1);

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
    ok(ReadFile(piperead,readbuf,sizeof(readbuf),&read, NULL), "Read from broken pipe with pending data failed\n");
    ok(read == sizeof(PIPENAME), "Read from anonymous pipe got %d bytes\n", read);
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
    ok(ReadFile(piperead, buffer, size, &read, NULL), "Read from broken pipe with pending data failed\n");
    ok(read == size, "Read from anonymous pipe got %d bytes\n", read);
    for (i = 0; i < size; i++) ok( buffer[i] == (BYTE)i, "invalid data %x at %x\n", buffer[i], i );
    /* But now we need to get informed that the pipe is closed */
    ok(ReadFile(piperead,readbuf,sizeof(readbuf),&read, NULL) == 0, "Broken pipe not detected\n");
    ok(CloseHandle(piperead), "CloseHandle for the read pipe failed\n");
    HeapFree(GetProcessHeap(), 0, buffer);

    ok(user_apc_ran == FALSE, "user apc ran, pipe using alertable io mode\n");
    SleepEx(0, TRUE); /* get rid of apc */

    ok(CreatePipe(&piperead, &pipewrite, &pipe_attr, 1) != 0, "CreatePipe failed\n");
    test_pipe_info(piperead, FILE_PIPE_SERVER_END, 1, 1, 1);
    test_pipe_info(pipewrite, 0, 1, 1, 1);
    ok(CloseHandle(pipewrite), "CloseHandle for the Write Pipe failed\n");
    ok(CloseHandle(piperead), "CloseHandle for the read pipe failed\n");
}

static void test_CloseHandle(void)
{
    static const char testdata[] = "Hello World";
    DWORD state, numbytes;
    HANDLE hpipe, hfile;
    char buffer[32];
    BOOL ret;

    hpipe = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
                             PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                             1, 1024, 1024, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(hpipe != INVALID_HANDLE_VALUE, "CreateNamedPipe failed with %u\n", GetLastError());

    hfile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile failed with %u\n", GetLastError());

    numbytes = 0xdeadbeef;
    ret = WriteFile(hpipe, testdata, sizeof(testdata), &numbytes, NULL);
    ok(ret, "WriteFile failed with %u\n", GetLastError());
    ok(numbytes == sizeof(testdata), "expected sizeof(testdata), got %u\n", numbytes);

    numbytes = 0xdeadbeef;
    ret = PeekNamedPipe(hfile, NULL, 0, NULL, &numbytes, NULL);
    ok(ret, "PeekNamedPipe failed with %u\n", GetLastError());
    ok(numbytes == sizeof(testdata), "expected sizeof(testdata), got %u\n", numbytes);

    ret = CloseHandle(hpipe);
    ok(ret, "CloseHandle failed with %u\n", GetLastError());

    numbytes = 0xdeadbeef;
    memset(buffer, 0, sizeof(buffer));
    ret = ReadFile(hfile, buffer, 0, &numbytes, NULL);
    ok(ret, "ReadFile failed with %u\n", GetLastError());
    ok(numbytes == 0, "expected 0, got %u\n", numbytes);

    numbytes = 0xdeadbeef;
    ret = PeekNamedPipe(hfile, NULL, 0, NULL, &numbytes, NULL);
    ok(ret, "PeekNamedPipe failed with %u\n", GetLastError());
    ok(numbytes == sizeof(testdata), "expected sizeof(testdata), got %u\n", numbytes);

    numbytes = 0xdeadbeef;
    memset(buffer, 0, sizeof(buffer));
    ret = ReadFile(hfile, buffer, sizeof(buffer), &numbytes, NULL);
    ok(ret, "ReadFile failed with %u\n", GetLastError());
    ok(numbytes == sizeof(testdata), "expected sizeof(testdata), got %u\n", numbytes);

    ret = GetNamedPipeHandleStateA(hfile, &state, NULL, NULL, NULL, NULL, 0);
    ok(ret, "GetNamedPipeHandleState failed with %u\n", GetLastError());
    state = PIPE_READMODE_MESSAGE | PIPE_WAIT;
    ret = SetNamedPipeHandleState(hfile, &state, NULL, NULL);
    ok(ret, "SetNamedPipeHandleState failed with %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buffer, 0, &numbytes, NULL);
    ok(!ret, "ReadFile unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_BROKEN_PIPE, "expected ERROR_BROKEN_PIPE, got %u\n", GetLastError());

    numbytes = 0xdeadbeef;
    ret = PeekNamedPipe(hfile, NULL, 0, NULL, &numbytes, NULL);
    ok(!ret && GetLastError() == ERROR_BROKEN_PIPE, "PeekNamedPipe returned %x (%u)\n",
       ret, GetLastError());
    ok(numbytes == 0xdeadbeef, "numbytes = %u\n", numbytes);

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, testdata, sizeof(testdata), &numbytes, NULL);
    ok(!ret, "WriteFile unexpectedly succeeded\n");
    todo_wine ok(GetLastError() == ERROR_NO_DATA, "expected ERROR_NO_DATA, got %u\n", GetLastError());

    CloseHandle(hfile);

    hpipe = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
                             PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                             1, 1024, 1024, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(hpipe != INVALID_HANDLE_VALUE, "CreateNamedPipe failed with %u\n", GetLastError());

    hfile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile failed with %u\n", GetLastError());

    numbytes = 0xdeadbeef;
    ret = WriteFile(hpipe, testdata, 0, &numbytes, NULL);
    ok(ret, "WriteFile failed with %u\n", GetLastError());
    ok(numbytes == 0, "expected 0, got %u\n", numbytes);

    ret = CloseHandle(hpipe);
    ok(ret, "CloseHandle failed with %u\n", GetLastError());

    numbytes = 0xdeadbeef;
    memset(buffer, 0, sizeof(buffer));
    ret = ReadFile(hfile, buffer, sizeof(buffer), &numbytes, NULL);
    ok(ret, "ReadFile failed with %u\n", GetLastError());
    ok(numbytes == 0, "expected 0, got %u\n", numbytes);

    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buffer, 0, &numbytes, NULL);
    ok(!ret, "ReadFile unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_BROKEN_PIPE, "expected ERROR_BROKEN_PIPE, got %u\n", GetLastError());

    ret = GetNamedPipeHandleStateA(hfile, &state, NULL, NULL, NULL, NULL, 0);
    ok(ret, "GetNamedPipeHandleState failed with %u\n", GetLastError());
    state = PIPE_READMODE_MESSAGE | PIPE_WAIT;
    ret = SetNamedPipeHandleState(hfile, &state, NULL, NULL);
    ok(ret, "SetNamedPipeHandleState failed with %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadFile(hfile, buffer, 0, &numbytes, NULL);
    ok(!ret, "ReadFile unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_BROKEN_PIPE, "expected ERROR_BROKEN_PIPE, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hfile, testdata, sizeof(testdata), &numbytes, NULL);
    ok(!ret, "WriteFile unexpectedly succeeded\n");
    todo_wine ok(GetLastError() == ERROR_NO_DATA, "expected ERROR_NO_DATA, got %u\n", GetLastError());

    CloseHandle(hfile);

    /* repeat test with hpipe <-> hfile swapped */

    hpipe = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
                             PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                             1, 1024, 1024, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(hpipe != INVALID_HANDLE_VALUE, "CreateNamedPipe failed with %u\n", GetLastError());

    hfile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile failed with %u\n", GetLastError());

    numbytes = 0xdeadbeef;
    ret = WriteFile(hfile, testdata, sizeof(testdata), &numbytes, NULL);
    ok(ret, "WriteFile failed with %u\n", GetLastError());
    ok(numbytes == sizeof(testdata), "expected sizeof(testdata), got %u\n", numbytes);

    numbytes = 0xdeadbeef;
    ret = PeekNamedPipe(hpipe, NULL, 0, NULL, &numbytes, NULL);
    ok(ret, "PeekNamedPipe failed with %u\n", GetLastError());
    ok(numbytes == sizeof(testdata), "expected sizeof(testdata), got %u\n", numbytes);

    ret = CloseHandle(hfile);
    ok(ret, "CloseHandle failed with %u\n", GetLastError());

    numbytes = 0xdeadbeef;
    memset(buffer, 0, sizeof(buffer));
    ret = ReadFile(hpipe, buffer, 0, &numbytes, NULL);
    ok(ret || GetLastError() == ERROR_MORE_DATA /* >= Win 8 */,
                 "ReadFile failed with %u\n", GetLastError());
    ok(numbytes == 0, "expected 0, got %u\n", numbytes);

    numbytes = 0xdeadbeef;
    ret = PeekNamedPipe(hpipe, NULL, 0, NULL, &numbytes, NULL);
    ok(ret, "PeekNamedPipe failed with %u\n", GetLastError());
    ok(numbytes == sizeof(testdata), "expected sizeof(testdata), got %u\n", numbytes);

    numbytes = 0xdeadbeef;
    memset(buffer, 0, sizeof(buffer));
    ret = ReadFile(hpipe, buffer, sizeof(buffer), &numbytes, NULL);
    ok(ret, "ReadFile failed with %u\n", GetLastError());
    ok(numbytes == sizeof(testdata), "expected sizeof(testdata), got %u\n", numbytes);

    ret = GetNamedPipeHandleStateA(hpipe, &state, NULL, NULL, NULL, NULL, 0);
    ok(ret, "GetNamedPipeHandleState failed with %u\n", GetLastError());
    state = PIPE_READMODE_MESSAGE | PIPE_WAIT;
    ret = SetNamedPipeHandleState(hpipe, &state, NULL, NULL);
    ok(ret, "SetNamedPipeHandleState failed with %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadFile(hpipe, buffer, 0, &numbytes, NULL);
    ok(!ret, "ReadFile unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_BROKEN_PIPE, "expected ERROR_BROKEN_PIPE, got %u\n", GetLastError());

    numbytes = 0xdeadbeef;
    ret = PeekNamedPipe(hpipe, NULL, 0, NULL, &numbytes, NULL);
    ok(!ret && GetLastError() == ERROR_BROKEN_PIPE, "PeekNamedPipe returned %x (%u)\n",
       ret, GetLastError());
    ok(numbytes == 0xdeadbeef, "numbytes = %u\n", numbytes);

    SetLastError(0xdeadbeef);
    ret = WriteFile(hpipe, testdata, sizeof(testdata), &numbytes, NULL);
    ok(!ret, "WriteFile unexpectedly succeeded\n");
    todo_wine ok(GetLastError() == ERROR_NO_DATA, "expected ERROR_NO_DATA, got %u\n", GetLastError());

    CloseHandle(hpipe);

    hpipe = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
                             PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                             1, 1024, 1024, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(hpipe != INVALID_HANDLE_VALUE, "CreateNamedPipe failed with %u\n", GetLastError());

    hfile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hfile != INVALID_HANDLE_VALUE, "CreateFile failed with %u\n", GetLastError());

    numbytes = 0xdeadbeef;
    ret = WriteFile(hfile, testdata, 0, &numbytes, NULL);
    ok(ret, "WriteFile failed with %u\n", GetLastError());
    ok(numbytes == 0, "expected 0, got %u\n", numbytes);

    ret = CloseHandle(hfile);
    ok(ret, "CloseHandle failed with %u\n", GetLastError());

    numbytes = 0xdeadbeef;
    memset(buffer, 0, sizeof(buffer));
    ret = ReadFile(hpipe, buffer, sizeof(buffer), &numbytes, NULL);
    ok(ret, "ReadFile failed with %u\n", GetLastError());
    ok(numbytes == 0, "expected 0, got %u\n", numbytes);

    SetLastError(0xdeadbeef);
    ret = ReadFile(hpipe, buffer, 0, &numbytes, NULL);
    ok(!ret, "ReadFile unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_BROKEN_PIPE, "expected ERROR_BROKEN_PIPE, got %u\n", GetLastError());

    ret = GetNamedPipeHandleStateA(hpipe, &state, NULL, NULL, NULL, NULL, 0);
    ok(ret, "GetNamedPipeHandleState failed with %u\n", GetLastError());
    state = PIPE_READMODE_MESSAGE | PIPE_WAIT;
    ret = SetNamedPipeHandleState(hpipe, &state, NULL, NULL);
    ok(ret, "SetNamedPipeHandleState failed with %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadFile(hpipe, buffer, 0, &numbytes, NULL);
    ok(!ret, "ReadFile unexpectedly succeeded\n");
    ok(GetLastError() == ERROR_BROKEN_PIPE, "expected ERROR_BROKEN_PIPE, got %u\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = WriteFile(hpipe, testdata, sizeof(testdata), &numbytes, NULL);
    ok(!ret, "WriteFile unexpectedly succeeded\n");
    todo_wine ok(GetLastError() == ERROR_NO_DATA, "expected ERROR_NO_DATA, got %u\n", GetLastError());

    CloseHandle(hpipe);
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

    ret = ConnectNamedPipe(pipe, &ol);
    err = GetLastError();
    ok(ret == 0, "ret %d\n", ret);
    ok(err == ERROR_IO_PENDING, "gle %d\n", err);
    CancelIo(pipe);
    ret = WaitForSingleObjectEx(ol.hEvent, INFINITE, 1);
    ok(ret == WAIT_OBJECT_0, "ret %x\n", ret);

    ret = GetOverlappedResult(pipe, &ol, &num, 1);
    err = GetLastError();
    ok(ret == 0, "ret %d\n", ret);
    ok(err == ERROR_OPERATION_ABORTED, "gle %d\n", err);

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

static void test_overlapped_error(void)
{
    HANDLE pipe, file, event;
    DWORD err, numbytes;
    OVERLAPPED overlapped;
    BOOL ret;

    event = CreateEventA(NULL, TRUE, FALSE, NULL);
    ok(event != NULL, "CreateEventA failed with %u\n", GetLastError());

    pipe = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                            1, 1024, 1024, NMPWAIT_WAIT_FOREVER, NULL);
    ok(pipe != INVALID_HANDLE_VALUE, "CreateNamedPipe failed with %u\n", GetLastError());

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = event;
    ret = ConnectNamedPipe(pipe, &overlapped);
    err = GetLastError();
    ok(ret == FALSE, "ConnectNamedPipe succeeded\n");
    ok(err == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %u\n", err);

    file = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed with %u\n", GetLastError());

    numbytes = 0xdeadbeef;
    ret = GetOverlappedResult(pipe, &overlapped, &numbytes, TRUE);
    ok(ret == TRUE, "GetOverlappedResult failed\n");
    ok(numbytes == 0, "expected 0, got %u\n", numbytes);
    ok(overlapped.Internal == STATUS_SUCCESS, "expected STATUS_SUCCESS, got %08lx\n", overlapped.Internal);

    CloseHandle(file);
    CloseHandle(pipe);

    pipe = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
                            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
                            1, 1024, 1024, NMPWAIT_WAIT_FOREVER, NULL);
    ok(pipe != INVALID_HANDLE_VALUE, "CreateNamedPipe failed with %u\n", GetLastError());

    file = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                       OPEN_EXISTING, FILE_FLAG_OVERLAPPED, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFile failed with %u\n", GetLastError());

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = event;
    ret = ConnectNamedPipe(pipe, &overlapped);
    err = GetLastError();
    ok(ret == FALSE, "ConnectNamedPipe succeeded\n");
    ok(err == ERROR_PIPE_CONNECTED, "expected ERROR_PIPE_CONNECTED, got %u\n", err);
    ok(overlapped.Internal == STATUS_PENDING, "expected STATUS_PENDING, got %08lx\n", overlapped.Internal);

    CloseHandle(file);
    CloseHandle(pipe);

    CloseHandle(event);
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

static void test_GetNamedPipeInfo(void)
{
    HANDLE server;

    server = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
        /* dwOpenMode */ PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    test_pipe_info(server, PIPE_SERVER_END | PIPE_TYPE_BYTE, 1024, 1024, 1);

    CloseHandle(server);

    server = CreateNamedPipeA(PIPENAME, PIPE_ACCESS_DUPLEX,
        /* dwOpenMode */ PIPE_TYPE_MESSAGE | PIPE_NOWAIT,
        /* nMaxInstances */ 3,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    test_pipe_info(server, PIPE_SERVER_END | PIPE_TYPE_MESSAGE, 1024, 1024, 3);

    CloseHandle(server);

    server = CreateNamedPipeA(PIPENAME, FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX,
        /* dwOpenMode */ PIPE_TYPE_MESSAGE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 0,
        /* nInBufSize */ 0,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    test_pipe_info(server, PIPE_SERVER_END | PIPE_TYPE_MESSAGE, 0, 0, 1);

    CloseHandle(server);

    server = CreateNamedPipeA(PIPENAME, FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX,
        /* dwOpenMode */ PIPE_TYPE_MESSAGE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 0xf000,
        /* nInBufSize */ 0xf000,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    test_pipe_info(server, PIPE_SERVER_END | PIPE_TYPE_MESSAGE, 0xf000, 0xf000, 1);

    CloseHandle(server);
}

static void test_readfileex_pending(void)
{
    HANDLE server, client, event;
    BOOL ret;
    DWORD err, wait, num_bytes;
    OVERLAPPED overlapped;
    char read_buf[1024];
    char write_buf[1024];
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
    for (i=0; i<256; i++)
    {
        ret = ReadFile(client, read_buf, sizeof(read_buf), &num_bytes, NULL);
        ok(ret == TRUE, "ReadFile failed\n");

        ok(completion_called == 0, "completion routine called during ReadFile\n");

        wait = WaitForSingleObjectEx(event, 0, TRUE);
        ok(wait == WAIT_IO_COMPLETION || wait == WAIT_OBJECT_0 || wait == WAIT_TIMEOUT,
           "WaitForSingleObject returned %x\n", wait);
        if (wait != WAIT_TIMEOUT) break;
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
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_IO_PENDING, "expected ERROR_IO_PENDING, got %d\n", GetLastError());
    ok(num_bytes == 0, "bytes %u\n", num_bytes);
    ok((NTSTATUS)overlapped.Internal == STATUS_PENDING, "expected STATUS_PENDING, got %#lx\n", overlapped.Internal);
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
    CloseHandle(event);
}

#define test_peek_pipe(a,b,c,d) _test_peek_pipe(__LINE__,a,b,c,d)
static void _test_peek_pipe(unsigned line, HANDLE pipe, DWORD expected_read, DWORD expected_avail, DWORD expected_message_length)
{
    DWORD bytes_read = 0xdeadbeed, avail = 0xdeadbeef, left = 0xdeadbeed;
    char buf[4000];
    FILE_PIPE_PEEK_BUFFER *peek_buf = (void*)buf;
    IO_STATUS_BLOCK io;
    NTSTATUS status;
    BOOL r;

    r = PeekNamedPipe(pipe, buf, sizeof(buf), &bytes_read, &avail, &left);
    ok_(__FILE__,line)(r, "PeekNamedPipe failed: %u\n", GetLastError());
    ok_(__FILE__,line)(bytes_read == expected_read, "bytes_read = %u, expected %u\n", bytes_read, expected_read);
    ok_(__FILE__,line)(avail == expected_avail, "avail = %u, expected %u\n", avail, expected_avail);
    ok_(__FILE__,line)(left == expected_message_length - expected_read, "left = %d, expected %d\n",
                       left, expected_message_length - expected_read);

    status = NtFsControlFile(pipe, 0, NULL, NULL, &io, FSCTL_PIPE_PEEK, NULL, 0, buf, sizeof(buf));
    ok_(__FILE__,line)(!status || status == STATUS_PENDING, "NtFsControlFile(FSCTL_PIPE_PEEK) failed: %x\n", status);
    ok_(__FILE__,line)(io.Information == FIELD_OFFSET(FILE_PIPE_PEEK_BUFFER, Data[expected_read]),
                       "io.Information = %lu\n", io.Information);
    ok_(__FILE__,line)(peek_buf->ReadDataAvailable == expected_avail, "ReadDataAvailable = %u, expected %u\n",
                       peek_buf->ReadDataAvailable, expected_avail);
    ok_(__FILE__,line)(peek_buf->MessageLength == expected_message_length, "MessageLength = %u, expected %u\n",
                       peek_buf->MessageLength, expected_message_length);

    if (expected_read)
    {
        r = PeekNamedPipe(pipe, buf, 1, &bytes_read, &avail, &left);
        ok_(__FILE__,line)(r, "PeekNamedPipe failed: %u\n", GetLastError());
        ok_(__FILE__,line)(bytes_read == 1, "bytes_read = %u, expected %u\n", bytes_read, expected_read);
        ok_(__FILE__,line)(avail == expected_avail, "avail = %u, expected %u\n", avail, expected_avail);
        ok_(__FILE__,line)(left == expected_message_length-1, "left = %d, expected %d\n", left, expected_message_length-1);
    }
}

#define overlapped_read_sync(a,b,c,d,e) _overlapped_read_sync(__LINE__,a,b,c,d,e)
static void _overlapped_read_sync(unsigned line, HANDLE reader, void *buf, DWORD buf_size, DWORD expected_result, BOOL partial_read)
{
    DWORD read_bytes = 0xdeadbeef;
    OVERLAPPED overlapped;
    BOOL res;

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    res = ReadFile(reader, buf, buf_size, &read_bytes, &overlapped);
    if (partial_read)
        ok_(__FILE__,line)(!res && GetLastError() == ERROR_MORE_DATA, "ReadFile returned: %x (%u)\n", res, GetLastError());
    else
        ok_(__FILE__,line)(res, "ReadFile failed: %u\n", GetLastError());
    if(partial_read)
        ok_(__FILE__,line)(!read_bytes, "read_bytes %u expected 0\n", read_bytes);
    else
        ok_(__FILE__,line)(read_bytes == expected_result, "read_bytes %u expected %u\n", read_bytes, expected_result);

    read_bytes = 0xdeadbeef;
    res = GetOverlappedResult(reader, &overlapped, &read_bytes, FALSE);
    if (partial_read)
        ok_(__FILE__,line)(!res && GetLastError() == ERROR_MORE_DATA,
                           "GetOverlappedResult returned: %x (%u)\n", res, GetLastError());
    else
        ok_(__FILE__,line)(res, "GetOverlappedResult failed: %u\n", GetLastError());
    ok_(__FILE__,line)(read_bytes == expected_result, "read_bytes %u expected %u\n", read_bytes, expected_result);
    CloseHandle(overlapped.hEvent);
}

#define overlapped_read_async(a,b,c,d) _overlapped_read_async(__LINE__,a,b,c,d)
static void _overlapped_read_async(unsigned line, HANDLE reader, void *buf, DWORD buf_size, OVERLAPPED *overlapped)
{
    DWORD read_bytes = 0xdeadbeef;
    BOOL res;

    memset(overlapped, 0, sizeof(*overlapped));
    overlapped->hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    res = ReadFile(reader, buf, buf_size, &read_bytes, overlapped);
    ok_(__FILE__,line)(!res && GetLastError() == ERROR_IO_PENDING, "ReadFile returned %x(%u)\n", res, GetLastError());
    ok_(__FILE__,line)(!read_bytes, "read_bytes %u expected 0\n", read_bytes);

    _test_not_signaled(line, overlapped->hEvent);
}

#define overlapped_write_sync(a,b,c) _overlapped_write_sync(__LINE__,a,b,c)
static void _overlapped_write_sync(unsigned line, HANDLE writer, void *buf, DWORD size)
{
    DWORD written_bytes = 0xdeadbeef;
    OVERLAPPED overlapped;
    BOOL res;

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    res = WriteFile(writer, buf, size, &written_bytes, &overlapped);
    ok_(__FILE__,line)(res, "WriteFile returned %x(%u)\n", res, GetLastError());
    ok_(__FILE__,line)(written_bytes == size, "WriteFile returned written_bytes = %u\n", written_bytes);

    written_bytes = 0xdeadbeef;
    res = GetOverlappedResult(writer, &overlapped, &written_bytes, FALSE);
    ok_(__FILE__,line)(res, "GetOverlappedResult failed: %u\n", GetLastError());
    ok_(__FILE__,line)(written_bytes == size, "GetOverlappedResult returned written_bytes %u expected %u\n", written_bytes, size);

    CloseHandle(overlapped.hEvent);
}

#define overlapped_write_async(a,b,c,d) _overlapped_write_async(__LINE__,a,b,c,d)
static void _overlapped_write_async(unsigned line, HANDLE writer, void *buf, DWORD size, OVERLAPPED *overlapped)
{
    DWORD written_bytes = 0xdeadbeef;
    BOOL res;

    memset(overlapped, 0, sizeof(*overlapped));
    overlapped->hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    res = WriteFile(writer, buf, size, &written_bytes, overlapped);
    disable_success_count
    ok_(__FILE__,line)(!res && GetLastError() == ERROR_IO_PENDING, "WriteFile returned %x(%u)\n", res, GetLastError());
    disable_success_count
    ok_(__FILE__,line)(!written_bytes, "written_bytes = %u\n", written_bytes);

    _test_not_signaled(line, overlapped->hEvent);
}

#define test_flush_sync(a) _test_flush_sync(__LINE__,a)
static void _test_flush_sync(unsigned line, HANDLE pipe)
{
    BOOL res;

    res = FlushFileBuffers(pipe);
    ok_(__FILE__,line)(res, "FlushFileBuffers failed: %u\n", GetLastError());
}

static DWORD expected_flush_error;

static DWORD CALLBACK flush_proc(HANDLE pipe)
{
    BOOL res;

    res = FlushFileBuffers(pipe);
    if (expected_flush_error == ERROR_SUCCESS)
        ok(res, "FlushFileBuffers failed: %u\n", GetLastError());
    else
        todo_wine ok(!res && GetLastError() == expected_flush_error, "FlushFileBuffers failed: %u\n", GetLastError());
    return 0;
}

#define test_flush_async(a,b) _test_flush_async(__LINE__,a,b)
static HANDLE _test_flush_async(unsigned line, HANDLE pipe, DWORD error)
{
    HANDLE thread;
    DWORD tid;

    expected_flush_error = error;
    thread = CreateThread(NULL, 0, flush_proc, pipe, 0, &tid);
    ok_(__FILE__,line)(thread != NULL, "CreateThread failed: %u\n", GetLastError());

    Sleep(50);
    _test_not_signaled(line, thread);
    return thread;
}

#define test_flush_done(a) _test_flush_done(__LINE__,a)
static void _test_flush_done(unsigned line, HANDLE thread)
{
    DWORD res = WaitForSingleObject(thread, 1000);
    ok_(__FILE__,line)(res == WAIT_OBJECT_0, "WaitForSingleObject returned %u (%u)\n", res, GetLastError());
    CloseHandle(thread);
}

#define test_overlapped_result(a,b,c,d) _test_overlapped_result(__LINE__,a,b,c,d)
static void _test_overlapped_result(unsigned line, HANDLE handle, OVERLAPPED *overlapped, DWORD expected_result, BOOL partial_read)
{
    DWORD result = 0xdeadbeef;
    BOOL res;

    _test_signaled(line, overlapped->hEvent);

    res = GetOverlappedResult(handle, overlapped, &result, FALSE);
    if (partial_read)
        ok_(__FILE__,line)(!res && GetLastError() == ERROR_MORE_DATA, "GetOverlappedResult returned: %x (%u)\n", res, GetLastError());
    else
        ok_(__FILE__,line)(res, "GetOverlappedResult failed: %u\n", GetLastError());
    ok_(__FILE__,line)(result == expected_result, "read_bytes = %u, expected %u\n", result, expected_result);
    CloseHandle(overlapped->hEvent);
}

#define test_overlapped_failure(a,b,c) _test_overlapped_failure(__LINE__,a,b,c)
static void _test_overlapped_failure(unsigned line, HANDLE handle, OVERLAPPED *overlapped, DWORD error)
{
    DWORD result;
    BOOL res;

    _test_signaled(line, overlapped->hEvent);

    res = GetOverlappedResult(handle, overlapped, &result, FALSE);
    ok_(__FILE__,line)(!res && GetLastError() == error, "GetOverlappedResult returned: %x (%u), expected error %u\n",
                       res, GetLastError(), error);
    ok_(__FILE__,line)(!result, "result = %u\n", result);
    CloseHandle(overlapped->hEvent);
}

#define cancel_overlapped(a,b) _cancel_overlapped(__LINE__,a,b)
static void _cancel_overlapped(unsigned line, HANDLE handle, OVERLAPPED *overlapped)
{
    BOOL res;

    res = pCancelIoEx(handle, overlapped);
    ok_(__FILE__,line)(res, "CancelIoEx failed: %u\n", GetLastError());

    _test_overlapped_failure(line, handle, overlapped, ERROR_OPERATION_ABORTED);
}

static void test_blocking_rw(HANDLE writer, HANDLE reader, DWORD buf_size, BOOL msg_mode, BOOL msg_read)
{
    OVERLAPPED read_overlapped, read_overlapped2, write_overlapped, write_overlapped2;
    char buf[10000], read_buf[10000];
    HANDLE flush_thread;

    memset(buf, 0xaa, sizeof(buf));

    /* test pending read with overlapped event */
    overlapped_read_async(reader, read_buf, 1000, &read_overlapped);
    test_flush_sync(writer);
    test_peek_pipe(reader, 0, 0, 0);

    /* write more data than needed for read */
    overlapped_write_sync(writer, buf, 4000);
    test_overlapped_result(reader, &read_overlapped, 1000, msg_read);

    /* test pending write with overlapped event */
    overlapped_write_async(writer, buf, buf_size, &write_overlapped);

    /* write one more byte */
    overlapped_write_async(writer, buf, 1, &write_overlapped2);
    flush_thread = test_flush_async(writer, ERROR_SUCCESS);
    test_not_signaled(write_overlapped.hEvent);

    /* empty write will not block */
    overlapped_write_sync(writer, buf, 0);
    test_not_signaled(write_overlapped.hEvent);
    test_not_signaled(write_overlapped2.hEvent);

    /* read remaining data from the first write */
    overlapped_read_sync(reader, read_buf, 3000, 3000, FALSE);
    test_overlapped_result(writer, &write_overlapped, buf_size, FALSE);
    test_not_signaled(write_overlapped2.hEvent);
    test_not_signaled(flush_thread);

    /* read one byte so that the next write fits the buffer */
    overlapped_read_sync(reader, read_buf, 1, 1, msg_read);
    test_overlapped_result(writer, &write_overlapped2, 1, FALSE);

    /* read the whole buffer */
    overlapped_read_sync(reader, read_buf, buf_size, buf_size-msg_read, FALSE);

    if(msg_read)
        overlapped_read_sync(reader, read_buf, 1000, 1, FALSE);

    if(msg_mode) {
        /* we still have an empty message in queue */
        overlapped_read_sync(reader, read_buf, 1000, 0, FALSE);
    }
    test_flush_done(flush_thread);

    /* pipe is empty, the next read will block */
    overlapped_read_async(reader, read_buf, 0, &read_overlapped);
    overlapped_read_async(reader, read_buf, 1000, &read_overlapped2);

    /* write one byte */
    overlapped_write_sync(writer, buf, 1);
    test_overlapped_result(reader, &read_overlapped, 0, msg_read);
    test_overlapped_result(reader, &read_overlapped2, 1, FALSE);

    /* write a message larger than buffer */
    overlapped_write_async(writer, buf, buf_size+2000, &write_overlapped);

    /* read so that pending write is still larger than the buffer */
    overlapped_read_sync(reader, read_buf, 1999, 1999, msg_read);
    test_not_signaled(write_overlapped.hEvent);

    /* read one more byte */
    overlapped_read_sync(reader, read_buf, 1, 1, msg_read);
    test_overlapped_result(writer, &write_overlapped, buf_size+2000, FALSE);

    /* read remaining data */
    overlapped_read_sync(reader, read_buf, buf_size+1, buf_size, FALSE);

    /* simple pass of empty message */
    overlapped_write_sync(writer, buf, 0);
    if(msg_mode)
        overlapped_read_sync(reader, read_buf, 1, 0, FALSE);

    /* pipe is empty, the next read will block */
    test_flush_sync(writer);
    overlapped_read_async(reader, read_buf, 0, &read_overlapped);
    overlapped_read_async(reader, read_buf, 1, &read_overlapped2);

    /* 0 length write wakes one read in msg mode */
    overlapped_write_sync(writer, buf, 0);
    if(msg_mode)
        test_overlapped_result(reader, &read_overlapped, 0, FALSE);
    else
        test_not_signaled(read_overlapped.hEvent);
    test_not_signaled(read_overlapped2.hEvent);
    overlapped_write_sync(writer, buf, 1);
    test_overlapped_result(reader, &read_overlapped2, 1, FALSE);

    overlapped_write_sync(writer, buf, 20);
    test_peek_pipe(reader, 20, 20, msg_mode ? 20 : 0);
    overlapped_write_sync(writer, buf, 15);
    test_peek_pipe(reader, msg_mode ? 20 : 35, 35, msg_mode ? 20 : 0);
    overlapped_read_sync(reader, read_buf, 10, 10, msg_read);
    test_peek_pipe(reader, msg_mode ? 10 : 25, 25, msg_mode ? 10 : 0);
    overlapped_read_sync(reader, read_buf, 10, 10, FALSE);
    test_peek_pipe(reader, 15, 15, msg_mode ? 15 : 0);
    overlapped_read_sync(reader, read_buf, 15, 15, FALSE);

    if(!pCancelIoEx) {
        win_skip("CancelIoEx not available\n");
        return;
    }

    /* add one more pending read, then cancel the first one */
    overlapped_read_async(reader, read_buf, 1, &read_overlapped);
    overlapped_read_async(reader, read_buf, 1, &read_overlapped2);
    cancel_overlapped(reader, &read_overlapped2);
    test_not_signaled(read_overlapped.hEvent);
    overlapped_write_sync(writer, buf, 1);
    test_overlapped_result(reader, &read_overlapped, 1, FALSE);

    /* make two async writes, cancel the first one and make sure that we read from the second one */
    overlapped_write_async(writer, buf, buf_size+2000, &write_overlapped);
    overlapped_write_async(writer, buf, 1, &write_overlapped2);
    cancel_overlapped(writer, &write_overlapped);
    overlapped_read_sync(reader, read_buf, 1000, 1, FALSE);
    test_overlapped_result(writer, &write_overlapped2, 1, FALSE);

    /* same as above, but parially read written data before canceling */
    overlapped_write_async(writer, buf, buf_size+2000, &write_overlapped);
    overlapped_write_async(writer, buf, 1, &write_overlapped2);
    overlapped_read_sync(reader, read_buf, 10, 10, msg_read);
    test_not_signaled(write_overlapped.hEvent);
    cancel_overlapped(writer, &write_overlapped);
    overlapped_read_sync(reader, read_buf, 1000, 1, FALSE);
    test_overlapped_result(writer, &write_overlapped2, 1, FALSE);

    /* empty queue by canceling write and make sure that flush is signaled */
    overlapped_write_async(writer, buf, buf_size+2000, &write_overlapped);
    flush_thread = test_flush_async(writer, ERROR_SUCCESS);
    test_not_signaled(flush_thread);
    cancel_overlapped(writer, &write_overlapped);
    test_flush_done(flush_thread);
}

static void child_process_write_pipe(HANDLE pipe)
{
    OVERLAPPED overlapped;
    char buf[10000];

    memset(buf, 'x', sizeof(buf));
    overlapped_write_async(pipe, buf, sizeof(buf), &overlapped);

    /* sleep until parent process terminates this process */
    Sleep(INFINITE);
}

static HANDLE create_writepipe_process(HANDLE pipe)
{
    STARTUPINFOA si = { sizeof(si) };
    PROCESS_INFORMATION info;
    char **argv, buf[MAX_PATH];
    BOOL res;

    winetest_get_mainargs(&argv);
    sprintf(buf, "\"%s\" pipe writepipe %lx", argv[0], (UINT_PTR)pipe);
    res = CreateProcessA(NULL, buf, NULL, NULL, TRUE, 0L, NULL, NULL, &si, &info);
    ok(res, "CreateProcess failed: %u\n", GetLastError());
    CloseHandle(info.hThread);

    return info.hProcess;
}

static void create_overlapped_pipe(DWORD mode, HANDLE *client, HANDLE *server)
{
    SECURITY_ATTRIBUTES sec_attr = { sizeof(sec_attr), NULL, TRUE };
    DWORD read_mode = mode & (PIPE_READMODE_BYTE | PIPE_READMODE_MESSAGE);
    OVERLAPPED overlapped;
    BOOL res;

    *server = CreateNamedPipeA(PIPENAME, FILE_FLAG_OVERLAPPED | PIPE_ACCESS_DUPLEX,
                               PIPE_WAIT | mode, 1, 5000, 6000, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(&server != INVALID_HANDLE_VALUE, "CreateNamedPipe failed: %u\n", GetLastError());
    test_signaled(*server);

    memset(&overlapped, 0, sizeof(overlapped));
    overlapped.hEvent = CreateEventW(NULL, TRUE, FALSE, NULL);
    res = ConnectNamedPipe(*server, &overlapped);
    ok(!res && GetLastError() == ERROR_IO_PENDING, "WriteFile returned %x(%u)\n", res, GetLastError());
    test_not_signaled(*server);
    test_not_signaled(overlapped.hEvent);

    *client = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, &sec_attr, OPEN_EXISTING, FILE_FLAG_OVERLAPPED, NULL);
    ok(*server != INVALID_HANDLE_VALUE, "CreateFile failed: %u\n", GetLastError());

    res = SetNamedPipeHandleState(*client, &read_mode, NULL, NULL);
    ok(res, "SetNamedPipeHandleState failed: %u\n", GetLastError());

    test_signaled(*client);
    test_not_signaled(*server);
    test_overlapped_result(*server, &overlapped, 0, FALSE);
}

static void test_overlapped_transport(BOOL msg_mode, BOOL msg_read_mode)
{
    OVERLAPPED overlapped, overlapped2;
    HANDLE server, client, flush;
    DWORD read_bytes;
    HANDLE process;
    char buf[60000];
    BOOL res;

    DWORD create_flags =
        (msg_mode ? PIPE_TYPE_MESSAGE : PIPE_TYPE_BYTE) |
        (msg_read_mode ? PIPE_READMODE_MESSAGE : PIPE_READMODE_BYTE);

    create_overlapped_pipe(create_flags, &client, &server);

    trace("testing %s, %s server->client writes...\n",
          msg_mode ? "message mode" : "byte mode", msg_read_mode ? "message read" : "byte read");
    test_blocking_rw(server, client, 5000, msg_mode, msg_read_mode);
    trace("testing %s, %s client->server writes...\n",
          msg_mode ? "message mode" : "byte mode", msg_read_mode ? "message read" : "byte read");
    test_blocking_rw(client, server, 6000, msg_mode, msg_read_mode);

    CloseHandle(client);
    CloseHandle(server);

    /* close client with pending writes */
    create_overlapped_pipe(create_flags, &client, &server);
    overlapped_write_async(server, buf, 7000, &overlapped);
    flush = test_flush_async(server, ERROR_BROKEN_PIPE);
    CloseHandle(client);
    test_overlapped_failure(server, &overlapped, ERROR_BROKEN_PIPE);
    test_flush_done(flush);
    CloseHandle(server);

    /* close server with pending writes */
    create_overlapped_pipe(create_flags, &client, &server);
    overlapped_write_async(client, buf, 7000, &overlapped);
    flush = test_flush_async(client, ERROR_BROKEN_PIPE);
    CloseHandle(server);
    test_overlapped_failure(client, &overlapped, ERROR_BROKEN_PIPE);
    test_flush_done(flush);
    CloseHandle(client);

    /* disconnect with pending writes */
    create_overlapped_pipe(create_flags, &client, &server);
    overlapped_write_async(client, buf, 7000, &overlapped);
    overlapped_write_async(server, buf, 7000, &overlapped2);
    flush = test_flush_async(client, ERROR_PIPE_NOT_CONNECTED);
    res = DisconnectNamedPipe(server);
    ok(res, "DisconnectNamedPipe failed: %u\n", GetLastError());
    test_overlapped_failure(client, &overlapped, ERROR_PIPE_NOT_CONNECTED);
    test_overlapped_failure(client, &overlapped2, ERROR_PIPE_NOT_CONNECTED);
    test_flush_done(flush);
    CloseHandle(server);
    CloseHandle(client);

    /* terminate process with pending write */
    create_overlapped_pipe(create_flags, &client, &server);
    process = create_writepipe_process(client);
    /* successfully read part of write that is pending in child process */
    res = ReadFile(server, buf, 10, &read_bytes, NULL);
    if(!msg_read_mode)
        ok(res, "ReadFile failed: %u\n", GetLastError());
    else
        ok(!res && GetLastError() == ERROR_MORE_DATA, "ReadFile returned: %x %u\n", res, GetLastError());
    ok(read_bytes == 10, "read_bytes = %u\n", read_bytes);
    TerminateProcess(process, 0);
    winetest_wait_child_process(process);
    /* after terminating process, there is no pending write and pipe buffer is empty */
    overlapped_read_async(server, buf, 10, &overlapped);
    overlapped_write_sync(client, buf, 1);
    test_overlapped_result(server, &overlapped, 1, FALSE);
    CloseHandle(process);
    CloseHandle(server);
    CloseHandle(client);
}

START_TEST(pipe)
{
    char **argv;
    int argc;
    HMODULE hmod;

    hmod = GetModuleHandleA("advapi32.dll");
    pDuplicateTokenEx = (void *) GetProcAddress(hmod, "DuplicateTokenEx");
    hmod = GetModuleHandleA("kernel32.dll");
    pQueueUserAPC = (void *) GetProcAddress(hmod, "QueueUserAPC");
    pCancelIoEx = (void *) GetProcAddress(hmod, "CancelIoEx");

    argc = winetest_get_mainargs(&argv);

    if (argc > 3 && !strcmp(argv[2], "writepipe"))
    {
        UINT_PTR handle;
        sscanf(argv[3], "%lx", &handle);
        child_process_write_pipe((HANDLE)handle);
        return;
    }

    if (test_DisconnectNamedPipe())
        return;
    test_CreateNamedPipe_instances_must_match();
    test_NamedPipe_2();
    test_CreateNamedPipe(PIPE_TYPE_BYTE);
    test_CreateNamedPipe(PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE);
    test_CreatePipe();
    test_ReadFile();
    test_CloseHandle();
    test_impersonation();
    test_overlapped();
    test_overlapped_error();
    test_NamedPipeHandleState();
    test_GetNamedPipeInfo();
    test_readfileex_pending();
    test_overlapped_transport(TRUE, FALSE);
    test_overlapped_transport(TRUE, TRUE);
    test_overlapped_transport(FALSE, FALSE);
}
