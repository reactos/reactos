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
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#include <windef.h>
#include <winbase.h>
#include <winsock.h>
#include <wtypes.h>
#include <winerror.h>

#include "wine/test.h"

#define PIPENAME "\\\\.\\PiPe\\tests_pipe.c"

#define NB_SERVER_LOOPS 8

static HANDLE alarm_event;

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

    ok(WaitNamedPipeA(PIPENAME, 2000), "WaitNamedPipe failed (%08lx)\n", GetLastError());

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile failed (%08lx)\n", GetLastError());

    /* don't try to do i/o if one side couldn't be opened, as it hangs */
    if (hFile != INVALID_HANDLE_VALUE) {
        HANDLE hFile2;

        /* Make sure we can read and write a few bytes in both directions */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len 1\n");
        ok(PeekNamedPipe(hFile, NULL, 0, NULL, &readden, NULL), "Peek\n");
        ok(readden == sizeof(obuf), "peek 1 got %ld bytes\n", readden);
        ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf), "read 1 got %ld bytes\n", readden);
        ok(memcmp(obuf, ibuf, written) == 0, "content 1 check\n");

        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf2, sizeof(obuf2), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf2), "write file len 2\n");
        ok(PeekNamedPipe(hnp, NULL, 0, NULL, &readden, NULL), "Peek\n");
        ok(readden == sizeof(obuf2), "peek 2 got %ld bytes\n", readden);
        ok(PeekNamedPipe(hnp, (LPVOID)1, 0, NULL, &readden, NULL), "Peek\n");
        ok(readden == sizeof(obuf2), "peek 2 got %ld bytes\n", readden);
        ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf2), "read 2 got %ld bytes\n", readden);
        ok(memcmp(obuf2, ibuf, written) == 0, "content 2 check\n");

        /* Test reading of multiple writes */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile3a\n");
        ok(written == sizeof(obuf), "write file len 3a\n");
        ok(WriteFile(hnp, obuf2, sizeof(obuf2), &written, NULL), " WriteFile3b\n");
        ok(written == sizeof(obuf2), "write file len 3b\n");
        ok(PeekNamedPipe(hFile, ibuf, sizeof(ibuf), &readden, &avail, NULL), "Peek3\n");
        if (pipemode == PIPE_TYPE_BYTE) {
            todo_wine {
                /* should return all 23 bytes */
                ok(readden == sizeof(obuf) + sizeof(obuf2), "peek3 got %ld bytes\n", readden);
            }
        }
        else
            ok(readden == sizeof(obuf), "peek3 got %ld bytes\n", readden);
        if (avail != sizeof(obuf)) /* older Linux kernels only return the first write here */
            ok(avail == sizeof(obuf) + sizeof(obuf2), "peek3 got %ld bytes available\n", avail);
        pbuf = ibuf;
        ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "pipe content 3a check\n");
        if (pipemode == PIPE_TYPE_BYTE) {
            todo_wine {
                pbuf += sizeof(obuf);
                ok(memcmp(obuf2, pbuf, sizeof(obuf2)) == 0, "pipe content 3b check\n");
            }
        }
        ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf) + sizeof(obuf2), "read 3 got %ld bytes\n", readden);
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
            todo_wine {
                /* should return all 23 bytes */
                ok(readden == sizeof(obuf) + sizeof(obuf2), "peek4 got %ld bytes\n", readden);
            }
        }
        else
            ok(readden == sizeof(obuf), "peek4 got %ld bytes\n", readden);
        if (avail != sizeof(obuf)) /* older Linux kernels only return the first write here */
            ok(avail == sizeof(obuf) + sizeof(obuf2), "peek4 got %ld bytes available\n", avail);
        pbuf = ibuf;
        ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "pipe content 4a check\n");
        if (pipemode == PIPE_TYPE_BYTE) {
            todo_wine {
                pbuf += sizeof(obuf);
                ok(memcmp(obuf2, pbuf, sizeof(obuf2)) == 0, "pipe content 4b check\n");
            }
        }
        ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
        if (pipemode == PIPE_TYPE_BYTE) {
            ok(readden == sizeof(obuf) + sizeof(obuf2), "read 4 got %ld bytes\n", readden);
        }
        else {
            todo_wine {
                ok(readden == sizeof(obuf), "read 4 got %ld bytes\n", readden);
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
            ok(readden == sizeof(obuf), "peek5 got %ld bytes\n", readden);
            if (avail != sizeof(obuf)) /* older Linux kernels only return the first write here */
                ok(avail == sizeof(obuf) + sizeof(obuf2), "peek5 got %ld bytes available\n", avail);
            pbuf = ibuf;
            ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 5a check\n");
            ok(ReadFile(hFile, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            todo_wine {
                ok(readden == sizeof(obuf), "read 5 got %ld bytes\n", readden);
            }
            pbuf = ibuf;
            ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 5a check\n");
    
            /* Multiple writes in the reverse direction */
            /* the write of obuf2 from write4 should still be in the buffer */
            ok(PeekNamedPipe(hnp, ibuf, sizeof(ibuf), &readden, &avail, NULL), "Peek6a\n");
            todo_wine {
                ok(readden == sizeof(obuf2), "peek6a got %ld bytes\n", readden);
                ok(avail == sizeof(obuf2), "peek6a got %ld bytes available\n", avail);
            }
            if (avail > 0) {
                ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
                ok(readden == sizeof(obuf2), "read 6a got %ld bytes\n", readden);
                pbuf = ibuf;
                ok(memcmp(obuf2, pbuf, sizeof(obuf2)) == 0, "content 6a check\n");
            }
            memset(ibuf, 0, sizeof(ibuf));
            ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile6a\n");
            ok(written == sizeof(obuf), "write file len 6a\n");
            ok(WriteFile(hFile, obuf2, sizeof(obuf2), &written, NULL), " WriteFile6b\n");
            ok(written == sizeof(obuf2), "write file len 6b\n");
            ok(PeekNamedPipe(hnp, ibuf, sizeof(ibuf), &readden, &avail, NULL), "Peek6\n");
            ok(readden == sizeof(obuf), "peek6 got %ld bytes\n", readden);
            if (avail != sizeof(obuf)) /* older Linux kernels only return the first write here */
                ok(avail == sizeof(obuf) + sizeof(obuf2), "peek6b got %ld bytes available\n", avail);
            pbuf = ibuf;
            ok(memcmp(obuf, pbuf, sizeof(obuf)) == 0, "content 6a check\n");
            ok(ReadFile(hnp, ibuf, sizeof(ibuf), &readden, NULL), "ReadFile\n");
            todo_wine {
                ok(readden == sizeof(obuf), "read 6b got %ld bytes\n", readden);
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

    hnp = CreateNamedPipe(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    if (INVALID_HANDLE_VALUE == hnp) {
        trace ("Seems we have no named pipes.\n");
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
    char readbuf[32];

    pipe_attr.nLength = sizeof(SECURITY_ATTRIBUTES); 
    pipe_attr.bInheritHandle = TRUE; 
    pipe_attr.lpSecurityDescriptor = NULL; 
    ok(CreatePipe(&piperead, &pipewrite, &pipe_attr, 0) != 0, "CreatePipe failed\n");
    ok(WriteFile(pipewrite,PIPENAME,sizeof(PIPENAME), &written, NULL), "Write to anonymous pipe failed\n");
    ok(written == sizeof(PIPENAME), "Write to anonymous pipe wrote %ld bytes instead of %d\n", written,sizeof(PIPENAME));
    ok(ReadFile(piperead,readbuf,sizeof(readbuf),&read, NULL), "Read from non empty pipe failed\n");
    ok(read == sizeof(PIPENAME), "Read from  anonymous pipe got %ld bytes instead of %d\n", read, sizeof(PIPENAME));

    /* Now write another chunk*/
    ok(CreatePipe(&piperead, &pipewrite, &pipe_attr, 0) != 0, "CreatePipe failed\n");
    ok(WriteFile(pipewrite,PIPENAME,sizeof(PIPENAME), &written, NULL), "Write to anonymous pipe failed\n");
    ok(written == sizeof(PIPENAME), "Write to anonymous pipe wrote %ld bytes instead of %d\n", written,sizeof(PIPENAME));
    /* and close the write end, read should still succeed*/
    ok(CloseHandle(pipewrite), "CloseHandle for the Write Pipe failed\n");
    ok(ReadFile(piperead,readbuf,sizeof(readbuf),&read, NULL), "Read from broken pipe withe with pending data failed\n");
    ok(read == sizeof(PIPENAME), "Read from  anonymous pipe got %ld bytes instead of %d\n", read, sizeof(PIPENAME));
    /* But now we need to get informed that the pipe is closed */
    ok(ReadFile(piperead,readbuf,sizeof(readbuf),&read, NULL) == 0, "Broken pipe not detected\n");
}

START_TEST(pipe)
{
    trace("test 1 of 6:\n");
    if (test_DisconnectNamedPipe())
        return;
    trace("test 2 of 6:\n");
    test_CreateNamedPipe_instances_must_match();
    trace("test 3 of 6:\n");
    test_NamedPipe_2();
    trace("test 4 of 6:\n");
    test_CreateNamedPipe(PIPE_TYPE_BYTE);
    trace("test 5 of 6\n");
    test_CreateNamedPipe(PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE);
    trace("test 6 of 6\n");
    test_CreatePipe();
    trace("all tests done\n");
}
