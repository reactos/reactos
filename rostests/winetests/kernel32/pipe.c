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

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>

#include <windef.h>
#include <winbase.h>
#include <winsock.h>
#include <wtypes.h>
#include <winerror.h>

#include "wine/test.h"

#define PIPENAME "\\\\.\\PiPe\\tests_pipe.c"

#define NB_SERVER_LOOPS 8

static HANDLE alarm_event;
static BOOL (WINAPI *pDuplicateTokenEx)(HANDLE,DWORD,LPSECURITY_ATTRIBUTES,
                                        SECURITY_IMPERSONATION_LEVEL,TOKEN_TYPE,PHANDLE);


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
    DWORD lpmode;

    if (pipemode == PIPE_TYPE_BYTE)
        trace("test_CreateNamedPipe starting in byte mode\n");
    else
        trace("test_CreateNamedPipe starting in message mode\n");
    /* Bad parameter checks */
    hnp = CreateNamedPipe("not a named pipe", PIPE_ACCESS_DUPLEX, pipemode | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);

    if (hnp == INVALID_HANDLE_VALUE && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED) {
        /* Is this the right way to notify user of skipped tests? */
        ok(hnp == INVALID_HANDLE_VALUE && GetLastError() == ERROR_CALL_NOT_IMPLEMENTED,
            "CreateNamedPipe not supported on this platform, skipping tests.\n");
        return;
    }
    ok(hnp == INVALID_HANDLE_VALUE && GetLastError() == ERROR_INVALID_NAME,
        "CreateNamedPipe should fail if name doesn't start with \\\\.\\pipe\n");

    hnp = CreateNamedPipe(NULL,
        PIPE_ACCESS_DUPLEX, pipemode | PIPE_WAIT,
        1, 1024, 1024, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(hnp == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PATH_NOT_FOUND,
        "CreateNamedPipe should fail if name is NULL\n");

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile == INVALID_HANDLE_VALUE
        && GetLastError() == ERROR_FILE_NOT_FOUND,
        "connecting to nonexistent named pipe should fail with ERROR_FILE_NOT_FOUND\n");

    /* Functional checks */

    hnp = CreateNamedPipe(PIPENAME, PIPE_ACCESS_DUPLEX, pipemode | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    ok(WaitNamedPipeA(PIPENAME, 2000), "WaitNamedPipe failed (%d)\n", GetLastError());

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

        /* Test reading of multiple writes */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile3a\n");
        ok(written == sizeof(obuf), "write file len 3a\n");
        ok(WriteFile(hnp, obuf2, sizeof(obuf2), &written, NULL), " WriteFile3b\n");
        ok(written == sizeof(obuf2), "write file len 3b\n");
        ok(PeekNamedPipe(hFile, ibuf, sizeof(ibuf), &readden, &avail, NULL), "Peek3\n");
        if (pipemode == PIPE_TYPE_BYTE) {
            if (readden != sizeof(obuf))  /* Linux only returns the first message */
                ok(readden == sizeof(obuf) + sizeof(obuf2), "peek3 got %d bytes\n", readden);
            else
                todo_wine ok(readden == sizeof(obuf) + sizeof(obuf2), "peek3 got %d bytes\n", readden);
        }
        else
        {
            if (readden != sizeof(obuf) + sizeof(obuf2))  /* MacOS returns both messages */
                ok(readden == sizeof(obuf), "peek3 got %d bytes\n", readden);
            else
                todo_wine ok(readden == sizeof(obuf), "peek3 got %d bytes\n", readden);
        }
        if (avail != sizeof(obuf)) /* older Linux kernels only return the first write here */
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
            if (readden != sizeof(obuf))  /* Linux only returns the first message */
                /* should return all 23 bytes */
                ok(readden == sizeof(obuf) + sizeof(obuf2), "peek4 got %d bytes\n", readden);
            else
                todo_wine ok(readden == sizeof(obuf) + sizeof(obuf2), "peek4 got %d bytes\n", readden);
        }
        else
        {
            if (readden != sizeof(obuf) + sizeof(obuf2))  /* MacOS returns both messages */
                ok(readden == sizeof(obuf), "peek4 got %d bytes\n", readden);
            else
                todo_wine ok(readden == sizeof(obuf), "peek4 got %d bytes\n", readden);
        }
        if (avail != sizeof(obuf)) /* older Linux kernels only return the first write here */
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
            todo_wine {
                ok(readden == sizeof(obuf), "read 4 got %d bytes\n", readden);
            }
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
            todo_wine {
                ok(SetNamedPipeHandleState(hFile, &lpmode, NULL, NULL), "Change mode\n");
            }
        
            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile5a\n");
            ok(written == sizeof(obuf), "write file len 3a\n");
            ok(WriteFile(hnp, obuf2, sizeof(obuf2), &written, NULL), " WriteFile5b\n");
            ok(written == sizeof(obuf2), "write file len 3b\n");
            ok(PeekNamedPipe(hFile, ibuf, sizeof(ibuf), &readden, &avail, NULL), "Peek5\n");
            if (readden != sizeof(obuf) + sizeof(obuf2))  /* MacOS returns both writes */
                ok(readden == sizeof(obuf), "peek5 got %d bytes\n", readden);
            else
                todo_wine ok(readden == sizeof(obuf), "peek5 got %d bytes\n", readden);
            if (avail != sizeof(obuf)) /* older Linux kernels only return the first write here */
                ok(avail == sizeof(obuf) + sizeof(obuf2), "peek5 got %d bytes available\n", avail);
            else
                todo_wine ok(avail == sizeof(obuf) + sizeof(obuf2), "peek5 got %d bytes available\n", avail);
            pbuf = ibuf;
            ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 5a check\n");
            ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            todo_wine {
                ok(readden == sizeof(obuf), "read 5 got %d bytes\n", readden);
            }
            pbuf = ibuf;
            ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 5a check\n");
    
            /* Multiple writes in the reverse direction */
            /* the write of obuf2 from write4 should still be in the buffer */
            ok(PeekNamedPipe(hnp, ibuf, sizeof(ibuf), &readden, &avail, NULL), "Peek6a\n");
            todo_wine {
                ok(readden == sizeof(obuf2), "peek6a got %d bytes\n", readden);
                ok(avail == sizeof(obuf2), "peek6a got %d bytes available\n", avail);
            }
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
            if (readden != sizeof(obuf) + sizeof(obuf2))  /* MacOS returns both writes */
                ok(readden == sizeof(obuf), "peek6 got %d bytes\n", readden);
            else
                todo_wine ok(readden == sizeof(obuf), "peek6 got %d bytes\n", readden);
            if (avail != sizeof(obuf)) /* older Linux kernels only return the first write here */
                ok(avail == sizeof(obuf) + sizeof(obuf2), "peek6b got %d bytes available\n", avail);
            pbuf = ibuf;
            ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 6a check\n");
            ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            todo_wine {
                ok(readden == sizeof(obuf), "read 6b got %d bytes\n", readden);
            }
            pbuf = ibuf;
            ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 6a check\n");
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

    trace("test_CreateNamedPipe returning\n");
}

static void test_CreateNamedPipe_instances_must_match(void)
{
    HANDLE hnp, hnp2;

    /* Check no mismatch */
    hnp = CreateNamedPipe(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 2,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hnp2 = CreateNamedPipe(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 2,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp2 != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    ok(CloseHandle(hnp), "CloseHandle\n");
    ok(CloseHandle(hnp2), "CloseHandle\n");

    /* Check nMaxInstances */
    hnp = CreateNamedPipe(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hnp2 = CreateNamedPipe(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp2 == INVALID_HANDLE_VALUE
        && GetLastError() == ERROR_PIPE_BUSY, "nMaxInstances not obeyed\n");

    ok(CloseHandle(hnp), "CloseHandle\n");

    /* Check PIPE_ACCESS_* */
    hnp = CreateNamedPipe(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 2,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hnp2 = CreateNamedPipe(PIPENAME, PIPE_ACCESS_INBOUND, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp2 == INVALID_HANDLE_VALUE
        && GetLastError() == ERROR_ACCESS_DENIED, "PIPE_ACCESS_* mismatch allowed\n");

    ok(CloseHandle(hnp), "CloseHandle\n");

    /* etc, etc */
}

/** implementation of alarm() */
static DWORD CALLBACK alarmThreadMain(LPVOID arg)
{
    DWORD timeout = (DWORD) arg;
    trace("alarmThreadMain\n");
    if (WaitForSingleObject( alarm_event, timeout ) == WAIT_TIMEOUT)
    {
        ok(FALSE, "alarm\n");
        ExitProcess(1);
    }
    return 1;
}

HANDLE hnp = INVALID_HANDLE_VALUE;

/** Trivial byte echo server - disconnects after each session */
static DWORD CALLBACK serverThreadMain1(LPVOID arg)
{
    int i;

    trace("serverThreadMain1 start\n");
    /* Set up a simple echo server */
    hnp = CreateNamedPipe(PIPENAME "serverThreadMain1", PIPE_ACCESS_DUPLEX,
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
        DWORD success;

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
    hnp = CreateNamedPipe(PIPENAME "serverThreadMain2", PIPE_ACCESS_DUPLEX,
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
        DWORD success;

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

        /* Set up next echo server */
        hnpNext =
            CreateNamedPipe(PIPENAME "serverThreadMain2", PIPE_ACCESS_DUPLEX,
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
    hnp = CreateNamedPipe(PIPENAME "serverThreadMain3", PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hEvent = CreateEvent(NULL,  /* security attribute */
        TRUE,                   /* manual reset event */
        FALSE,                  /* initial state */
        NULL);                  /* name */
    ok(hEvent != NULL, "CreateEvent\n");

    for (i = 0; i < NB_SERVER_LOOPS; i++) {
        char buf[512];
        DWORD written;
        DWORD readden;
        DWORD dummy;
        DWORD success;
        OVERLAPPED oOverlap;
        int letWFSOEwait = (i & 2);
        int letGORwait = (i & 1);
	DWORD err;

        memset(&oOverlap, 0, sizeof(oOverlap));
        oOverlap.hEvent = hEvent;

        /* Wait for client to connect */
        trace("Server calling overlapped ConnectNamedPipe...\n");
        success = ConnectNamedPipe(hnp, &oOverlap);
        err = GetLastError();
        ok(success || err == ERROR_IO_PENDING
            || err == ERROR_PIPE_CONNECTED, "overlapped ConnectNamedPipe\n");
        trace("overlapped ConnectNamedPipe returned.\n");
        if (!success && (err == ERROR_IO_PENDING) && letWFSOEwait)
            ok(WaitForSingleObjectEx(hEvent, INFINITE, TRUE) == 0, "wait ConnectNamedPipe\n");
        success = GetOverlappedResult(hnp, &oOverlap, &dummy, letGORwait);
	if (!letGORwait && !letWFSOEwait && !success) {
	    ok(GetLastError() == ERROR_IO_INCOMPLETE, "GetOverlappedResult\n");
	    success = GetOverlappedResult(hnp, &oOverlap, &dummy, TRUE);
	}
	ok(success, "GetOverlappedResult ConnectNamedPipe\n");
        trace("overlapped ConnectNamedPipe operation complete.\n");

        /* Echo bytes once */
        memset(buf, 0, sizeof(buf));

        trace("Server reading...\n");
        success = ReadFile(hnp, buf, sizeof(buf), NULL, &oOverlap);
        trace("Server ReadFile returned...\n");
        err = GetLastError();
        ok(success || err == ERROR_IO_PENDING, "overlapped ReadFile\n");
        trace("overlapped ReadFile returned.\n");
        if (!success && (err == ERROR_IO_PENDING) && letWFSOEwait)
            ok(WaitForSingleObjectEx(hEvent, INFINITE, TRUE) == 0, "wait ReadFile\n");
        success = GetOverlappedResult(hnp, &oOverlap, &readden, letGORwait);
	if (!letGORwait && !letWFSOEwait && !success) {
	    ok(GetLastError() == ERROR_IO_INCOMPLETE, "GetOverlappedResult\n");
	    success = GetOverlappedResult(hnp, &oOverlap, &readden, TRUE);
	}
        trace("Server done reading.\n");
        ok(success, "overlapped ReadFile\n");

        trace("Server writing...\n");
        success = WriteFile(hnp, buf, readden, NULL, &oOverlap);
        trace("Server WriteFile returned...\n");
        err = GetLastError();
        ok(success || err == ERROR_IO_PENDING, "overlapped WriteFile\n");
        trace("overlapped WriteFile returned.\n");
        if (!success && (err == ERROR_IO_PENDING) && letWFSOEwait)
            ok(WaitForSingleObjectEx(hEvent, INFINITE, TRUE) == 0, "wait WriteFile\n");
        success = GetOverlappedResult(hnp, &oOverlap, &written, letGORwait);
	if (!letGORwait && !letWFSOEwait && !success) {
	    ok(GetLastError() == ERROR_IO_INCOMPLETE, "GetOverlappedResult\n");
	    success = GetOverlappedResult(hnp, &oOverlap, &written, TRUE);
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
    /* Set up a ten second timeout */
    alarm_event = CreateEvent( NULL, TRUE, FALSE, NULL );
    alarmThread = CreateThread(NULL, 0, alarmThreadMain, (void *) 10000, 0, &alarmThreadId);

    /* The servers we're about to exercize do try to clean up carefully,
     * but to reduce the change of a test failure due to a pipe handle
     * leak in the test code, we'll use a different pipe name for each server.
     */

    /* Try server #1 */
    serverThread = CreateThread(NULL, 0, serverThreadMain1, (void *)8, 0, &serverThreadId);
    ok(serverThread != INVALID_HANDLE_VALUE, "CreateThread\n");
    exercizeServer(PIPENAME "serverThreadMain1", serverThread);

    /* Try server #2 */
    serverThread = CreateThread(NULL, 0, serverThreadMain2, 0, 0, &serverThreadId);
    ok(serverThread != INVALID_HANDLE_VALUE, "CreateThread\n");
    exercizeServer(PIPENAME "serverThreadMain2", serverThread);

    if( 0 ) /* overlapped pipe server doesn't work yet - it randomly fails */
    {
    /* Try server #3 */
    serverThread = CreateThread(NULL, 0, serverThreadMain3, 0, 0, &serverThreadId);
    ok(serverThread != INVALID_HANDLE_VALUE, "CreateThread\n");
    exercizeServer(PIPENAME "serverThreadMain3", serverThread);
    }

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

    SetLastError(0xdeadbeef);
    hnp = CreateNamedPipe(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
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
    struct named_pipe_client_params *params = (struct named_pipe_client_params *)p;
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

    pipe = CreateFile(PIPE_NAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, params->security_flags, NULL);
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

    hPipeServer = CreateNamedPipe(PIPE_NAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_READMODE_BYTE | PIPE_WAIT, 1, 100, 100, NMPWAIT_USE_DEFAULT_WAIT, NULL);
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
    struct overlapped_server_args *a = (struct overlapped_server_args*)arg;
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
    int ret;
    struct overlapped_server_args args;

    args.pipe_created = CreateEventA(0, 1, 0, 0);
    thread = CreateThread(NULL, 0, overlapped_server, &args, 0, &tid);

    WaitForSingleObject(args.pipe_created, INFINITE);
    pipe = CreateFileA("\\\\.\\pipe\\my pipe", GENERIC_READ|GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
    ok(pipe != INVALID_HANDLE_VALUE, "cf failed\n");

    /* Sleep to try to get the ReadFile in the server to occur before the following WriteFile */
    Sleep(1);

    ret = WriteFile(pipe, "x", 1, &num, NULL);
    ok(ret == 1, "ret %d\n", ret);

    WaitForSingleObject(thread, INFINITE);
    CloseHandle(pipe);
    CloseHandle(args.pipe_created);
    CloseHandle(thread);
}

START_TEST(pipe)
{
    HMODULE hmod;

    skip("ROS-HACK: Skipping pipe tests -- ros' npfs is in a sorry state\n");
    return;

    hmod = GetModuleHandle("advapi32.dll");
    pDuplicateTokenEx = (void *) GetProcAddress(hmod, "DuplicateTokenEx");

    if (test_DisconnectNamedPipe())
        return;
    test_CreateNamedPipe_instances_must_match();
    test_NamedPipe_2();
    test_CreateNamedPipe(PIPE_TYPE_BYTE);
    test_CreateNamedPipe(PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE);
    test_CreatePipe();
    test_impersonation();
    test_overlapped();
}
