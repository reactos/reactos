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
#include <stdlib.h>
#include <stdio.h>
#include <time.h>

#include <windef.h>
#include <winbase.h>
#include <winsock.h>

#ifndef STANDALONE
#include "wine/test.h"
#else
#include <assert.h>
#define START_TEST(name) main(int argc, char **argv)
#define ok(condition, msg) \
	do { \
		if(!(condition)) \
		{ \
			fprintf(stderr,"failed at %d\n",__LINE__); \
			exit(0); \
		} \
	} while(0)

#define todo_wine
#endif

#include <wtypes.h>
#include <winerror.h>

#define PIPENAME "\\\\.\\PiPe\\tests_" __FILE__

void test_CreateNamedPipe(void)
{
    HANDLE hnp;
    HANDLE hFile;
    const char obuf[] = "Bit Bucket";
    char ibuf[32];
    DWORD written;
    DWORD readden;

    trace("test_CreateNamedPipe starting\n");
    /* Bad parameter checks */
    hnp = CreateNamedPipe("not a named pipe", PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
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
        PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        1, 1024, 1024, NMPWAIT_USE_DEFAULT_WAIT, NULL);
    ok(hnp == INVALID_HANDLE_VALUE && GetLastError() == ERROR_PATH_NOT_FOUND,
        "CreateNamedPipe should fail if name is NULL\n");

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile == INVALID_HANDLE_VALUE
        && GetLastError() == ERROR_FILE_NOT_FOUND,
        "connecting to nonexistent named pipe should fail with ERROR_FILE_NOT_FOUND\n");

    /* Functional checks */

    hnp = CreateNamedPipe(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

    hFile = CreateFileA(PIPENAME, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, 0);
    ok(hFile != INVALID_HANDLE_VALUE, "CreateFile failed\n");

    /* don't try to do i/o if one side couldn't be opened, as it hangs */
    if (hFile != INVALID_HANDLE_VALUE) {
        HANDLE hFile2;

        /* Make sure we can read and write a few bytes in both directions */
        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hnp, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len\n");
        ok(ReadFile(hFile, ibuf, sizeof(obuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf), "read file len\n");
        ok(memcmp(obuf, ibuf, written) == 0, "content check\n");

        memset(ibuf, 0, sizeof(ibuf));
        ok(WriteFile(hFile, obuf, sizeof(obuf), &written, NULL), "WriteFile\n");
        ok(written == sizeof(obuf), "write file len\n");
        ok(ReadFile(hnp, ibuf, sizeof(obuf), &readden, NULL), "ReadFile\n");
        ok(readden == sizeof(obuf), "read file len\n");
        ok(memcmp(obuf, ibuf, written) == 0, "content check\n");

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

void test_CreateNamedPipe_instances_must_match(void)
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
    Sleep(timeout);
    ok(FALSE, "alarm\n");
    ExitProcess(1);
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
    for (i = 0; ; i++) {
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

    for (i = 0; ; i++) {
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

    for (i = 0; ; i++) {
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
}

static void exercizeServer(const char *pipename, HANDLE serverThread)
{
    int i;

    trace("exercizeServer starting\n");
    for (i = 0; i < 8; i++) {
        HANDLE hFile=INVALID_HANDLE_VALUE;
        const char obuf[] = "Bit Bucket";
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

    ok(TerminateThread(serverThread, 0), "TerminateThread\n");
    CloseHandle(hnp);
    trace("exercizeServer returning\n");
}

void test_NamedPipe_2(void)
{
    HANDLE serverThread;
    DWORD serverThreadId;
    HANDLE alarmThread;
    DWORD alarmThreadId;

    trace("test_NamedPipe_2 starting\n");
    /* Set up a ten second timeout */
    alarmThread = CreateThread(NULL, 0, alarmThreadMain, (void *) 10000, 0, &alarmThreadId);

    /* The servers we're about to exercize do try to clean up carefully,
     * but to reduce the change of a test failure due to a pipe handle
     * leak in the test code, we'll use a different pipe name for each server.
     */

    /* Try server #1 */
    serverThread = CreateThread(NULL, 0, serverThreadMain1, 0, 0, &serverThreadId);
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

    ok(TerminateThread(alarmThread, 0), "TerminateThread\n");
    trace("test_NamedPipe_2 returning\n");
}

void test_DisconnectNamedPipe(void)
{
    HANDLE hnp;
    HANDLE hFile;
    const char obuf[] = "Bit Bucket";
    char ibuf[32];
    DWORD written;
    DWORD readden;

    hnp = CreateNamedPipe(PIPENAME, PIPE_ACCESS_DUPLEX, PIPE_TYPE_BYTE | PIPE_WAIT,
        /* nMaxInstances */ 1,
        /* nOutBufSize */ 1024,
        /* nInBufSize */ 1024,
        /* nDefaultWait */ NMPWAIT_USE_DEFAULT_WAIT,
        /* lpSecurityAttrib */ NULL);
    ok(hnp != INVALID_HANDLE_VALUE, "CreateNamedPipe failed\n");

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

}

START_TEST(pipe)
{
    trace("test 1 of 4:\n");
    test_DisconnectNamedPipe();
    trace("test 2 of 4:\n");
    test_CreateNamedPipe_instances_must_match();
    trace("test 3 of 4:\n");
    test_NamedPipe_2();
    trace("test 4 of 4:\n");
    test_CreateNamedPipe();
    trace("all tests done\n");
}
