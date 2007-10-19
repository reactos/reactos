/*
 * Tests for file change notification functions
 *
 * Copyright (c) 2004 Hans Leidekker
 * Copyright 2006 Mike McCormack for CodeWeavers
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

/* TODO: - security attribute changes
 *       - compound filter and multiple notifications
 *       - subtree notifications
 *       - non-documented flags FILE_NOTIFY_CHANGE_LAST_ACCESS and
 *         FILE_NOTIFY_CHANGE_CREATION
 */

#include <stdarg.h>
#include <stdio.h>

#include "ntstatus.h"
#define WIN32_NO_STATUS
#include "wine/test.h"
#include <windef.h>
#include <winbase.h>
#include <winternl.h>

static DWORD CALLBACK NotificationThread(LPVOID arg)
{
    HANDLE change = (HANDLE) arg;
    BOOL ret = FALSE;
    DWORD status;

    status = WaitForSingleObject(change, 100);

    if (status == WAIT_OBJECT_0 ) {
        ret = FindNextChangeNotification(change);
    }

    ret = FindCloseChangeNotification(change);
    ok( ret, "FindCloseChangeNotification error: %ld\n",
       GetLastError());

    ExitThread((DWORD)ret);
}

static HANDLE StartNotificationThread(LPCSTR path, BOOL subtree, DWORD flags)
{
    HANDLE change, thread;
    DWORD threadId;

    change = FindFirstChangeNotificationA(path, subtree, flags);
    ok(change != INVALID_HANDLE_VALUE, "FindFirstChangeNotification error: %ld\n", GetLastError());

    thread = CreateThread(NULL, 0, NotificationThread, (LPVOID)change,
                          0, &threadId);
    ok(thread != INVALID_HANDLE_VALUE, "CreateThread error: %ld\n", GetLastError());

    return thread;
}

static DWORD FinishNotificationThread(HANDLE thread)
{
    DWORD status, exitcode;

    status = WaitForSingleObject(thread, 5000);
    ok(status == WAIT_OBJECT_0, "WaitForSingleObject status %ld error %ld\n", status, GetLastError());

    ok(GetExitCodeThread(thread, &exitcode), "Could not retrieve thread exit code\n");

    return exitcode;
}

static void test_FindFirstChangeNotification(void)
{
    HANDLE change, file, thread;
    DWORD attributes, count;
    BOOL ret;

    char workdir[MAX_PATH], dirname1[MAX_PATH], dirname2[MAX_PATH];
    char filename1[MAX_PATH], filename2[MAX_PATH];
    static const char prefix[] = "FCN";
    char buffer[2048];

    /* pathetic checks */

    change = FindFirstChangeNotificationA("not-a-file", FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    ok(change == INVALID_HANDLE_VALUE && GetLastError() == ERROR_FILE_NOT_FOUND,
       "FindFirstChangeNotification error: %ld\n", GetLastError());

    if (0) /* This documents win2k behavior. It crashes on win98. */
    {
        change = FindFirstChangeNotificationA(NULL, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
        ok(change == NULL && GetLastError() == ERROR_PATH_NOT_FOUND,
        "FindFirstChangeNotification error: %ld\n", GetLastError());
    }

    ret = FindNextChangeNotification(NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "FindNextChangeNotification error: %ld\n",
       GetLastError());

    ret = FindCloseChangeNotification(NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "FindCloseChangeNotification error: %ld\n",
       GetLastError());

    ret = GetTempPathA(MAX_PATH, workdir);
    ok(ret, "GetTempPathA error: %ld\n", GetLastError());

    lstrcatA(workdir, "testFileChangeNotification");

    ret = CreateDirectoryA(workdir, NULL);
    ok(ret, "CreateDirectoryA error: %ld\n", GetLastError());

    ret = GetTempFileNameA(workdir, prefix, 0, filename1);
    ok(ret, "GetTempFileNameA error: %ld\n", GetLastError());

    file = CreateFileA(filename1, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA error: %ld\n", GetLastError());
    ret = CloseHandle(file);
    ok( ret, "CloseHandle error: %ld\n", GetLastError());

    /* Try to register notification for a file. win98 and win2k behave differently here */
    change = FindFirstChangeNotificationA(filename1, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    ok(change == INVALID_HANDLE_VALUE && (GetLastError() == ERROR_DIRECTORY ||
                                          GetLastError() == ERROR_FILE_NOT_FOUND),
       "FindFirstChangeNotification error: %ld\n", GetLastError());

    lstrcpyA(dirname1, filename1);
    lstrcatA(dirname1, "dir");

    lstrcpyA(dirname2, dirname1);
    lstrcatA(dirname2, "new");

    ret = CreateDirectoryA(dirname1, NULL);
    ok(ret, "CreateDirectoryA error: %ld\n", GetLastError());

    /* What if we move the directory we registered notification for? */
    thread = StartNotificationThread(dirname1, FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    ret = MoveFileA(dirname1, dirname2);
    ok(ret, "MoveFileA error: %ld\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* What if we remove the directory we registered notification for? */
    thread = StartNotificationThread(dirname2, FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    ret = RemoveDirectoryA(dirname2);
    ok(ret, "RemoveDirectoryA error: %ld\n", GetLastError());

    /* win98 and win2k behave differently here */
    ret = FinishNotificationThread(thread);
    ok(ret || !ret, "You'll never read this\n");

    /* functional checks */

    /* Create a directory */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    ret = CreateDirectoryA(dirname1, NULL);
    ok(ret, "CreateDirectoryA error: %ld\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Rename a directory */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    ret = MoveFileA(dirname1, dirname2);
    ok(ret, "MoveFileA error: %ld\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Delete a directory */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    ret = RemoveDirectoryA(dirname2);
    ok(ret, "RemoveDirectoryA error: %ld\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    lstrcpyA(filename2, filename1);
    lstrcatA(filename2, "new");

    /* Rename a file */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    ret = MoveFileA(filename1, filename2);
    ok(ret, "MoveFileA error: %ld\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Delete a file */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    ret = DeleteFileA(filename2);
    ok(ret, "DeleteFileA error: %ld\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Create a file */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    file = CreateFileA(filename2, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA error: %ld\n", GetLastError());
    ret = CloseHandle(file);
    ok( ret, "CloseHandle error: %ld\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    attributes = GetFileAttributesA(filename2);
    ok(attributes != INVALID_FILE_ATTRIBUTES, "GetFileAttributesA error: %ld\n", GetLastError());
    attributes &= FILE_ATTRIBUTE_READONLY;

    /* Change file attributes */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_ATTRIBUTES);
    ret = SetFileAttributesA(filename2, attributes);
    ok(ret, "SetFileAttributesA error: %ld\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Change last write time by writing to a file */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_LAST_WRITE);
    file = CreateFileA(filename2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA error: %ld\n", GetLastError());
    ret = WriteFile(file, buffer, sizeof(buffer), &count, NULL);
    ok(ret && count == sizeof(buffer), "WriteFile error: %ld\n", GetLastError());
    ret = CloseHandle(file);
    ok( ret, "CloseHandle error: %ld\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Change file size by truncating a file */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_SIZE);
    file = CreateFileA(filename2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA error: %ld\n", GetLastError());
    ret = WriteFile(file, buffer, sizeof(buffer) / 2, &count, NULL);
    ok(ret && count == sizeof(buffer) / 2, "WriteFileA error: %ld\n", GetLastError());
    ret = CloseHandle(file);
    ok( ret, "CloseHandle error: %ld\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* clean up */

    ret = DeleteFileA(filename2);
    ok(ret, "DeleteFileA error: %ld\n", GetLastError());

    ret = RemoveDirectoryA(workdir);
    ok(ret, "RemoveDirectoryA error: %ld\n", GetLastError());
}

/* this test concentrates more on the wait behaviour of the handle */
static void test_ffcn(void)
{
    DWORD filter;
    HANDLE handle;
    LONG r;
    WCHAR path[MAX_PATH], subdir[MAX_PATH];
    static const WCHAR szBoo[] = { '\\','b','o','o',0 };
    static const WCHAR szHoo[] = { '\\','h','o','o',0 };

    r = GetTempPathW( MAX_PATH, path );
    ok( r != 0, "temp path failed\n");
    if (!r)
        return;

    lstrcatW( path, szBoo );
    lstrcpyW( subdir, path );
    lstrcatW( subdir, szHoo );

    RemoveDirectoryW( subdir );
    RemoveDirectoryW( path );

    r = CreateDirectoryW(path, NULL);
    ok( r == TRUE, "failed to create directory\n");

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;
    filter |= FILE_NOTIFY_CHANGE_DIR_NAME;

    handle = FindFirstChangeNotificationW( path, 1, filter);
    ok( handle != INVALID_HANDLE_VALUE, "invalid handle\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == STATUS_TIMEOUT, "should time out\n");

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create subdir\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = FindNextChangeNotification(handle);
    ok( r == TRUE, "find next failed\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == STATUS_TIMEOUT, "should time out\n");

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove subdir\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = FindNextChangeNotification(handle);
    ok( r == TRUE, "find next failed\n");

    r = FindNextChangeNotification(handle);
    ok( r == TRUE, "find next failed\n");

    r = FindCloseChangeNotification(handle);
    ok( r == TRUE, "should succeed\n");

    r = RemoveDirectoryW( path );
    ok( r == TRUE, "failed to remove dir\n");
}

typedef BOOL (WINAPI *fnReadDirectoryChangesW)(HANDLE,LPVOID,DWORD,BOOL,DWORD,
                         LPDWORD,LPOVERLAPPED,LPOVERLAPPED_COMPLETION_ROUTINE);
fnReadDirectoryChangesW pReadDirectoryChangesW;

static void test_readdirectorychanges(void)
{
    HANDLE hdir;
    char buffer[0x1000];
    DWORD fflags, filter = 0, r, dwCount;
    OVERLAPPED ov;
    WCHAR path[MAX_PATH], subdir[MAX_PATH], subsubdir[MAX_PATH];
    static const WCHAR szBoo[] = { '\\','b','o','o',0 };
    static const WCHAR szHoo[] = { '\\','h','o','o',0 };
    static const WCHAR szGa[] = { '\\','h','o','o','\\','g','a',0 };
    PFILE_NOTIFY_INFORMATION pfni;

    if (!pReadDirectoryChangesW)
        return;

    r = GetTempPathW( MAX_PATH, path );
    ok( r != 0, "temp path failed\n");
    if (!r)
        return;

    lstrcatW( path, szBoo );
    lstrcpyW( subdir, path );
    lstrcatW( subdir, szHoo );

    lstrcpyW( subsubdir, path );
    lstrcatW( subsubdir, szGa );

    RemoveDirectoryW( subsubdir );
    RemoveDirectoryW( subdir );
    RemoveDirectoryW( path );

    r = CreateDirectoryW(path, NULL);
    ok( r == TRUE, "failed to create directory\n");

    SetLastError(0xd0b00b00);
    r = pReadDirectoryChangesW(NULL,NULL,0,FALSE,0,NULL,NULL,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    fflags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
    hdir = CreateFileW(path, GENERIC_READ|SYNCHRONIZE|FILE_LIST_DIRECTORY,
                        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, fflags, NULL);
    ok( hdir != INVALID_HANDLE_VALUE, "failed to open directory\n");

    ov.hEvent = CreateEvent( NULL, 1, 0, NULL );

    SetLastError(0xd0b00b00);
    r = pReadDirectoryChangesW(hdir,NULL,0,FALSE,0,NULL,NULL,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    SetLastError(0xd0b00b00);
    r = pReadDirectoryChangesW(hdir,NULL,0,FALSE,0,NULL,&ov,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;
    filter |= FILE_NOTIFY_CHANGE_DIR_NAME;
    filter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
    filter |= FILE_NOTIFY_CHANGE_SIZE;
    filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
    filter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
    filter |= FILE_NOTIFY_CHANGE_CREATION;
    filter |= FILE_NOTIFY_CHANGE_SECURITY;

    SetLastError(0xd0b00b00);
    ov.Internal = 0;
    ov.InternalHigh = 0;
    memset( buffer, 0, sizeof buffer );

    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,-1,NULL,&ov,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,0,NULL,&ov,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,TRUE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = WaitForSingleObject( ov.hEvent, 10 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( ov.hEvent, INFINITE );
    ok( r == WAIT_OBJECT_0, "event should be ready\n" );

    ok( ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0x12, "ov.InternalHigh wrong\n");

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_ADDED, "action wrong\n" );
    ok( pfni->FileNameLength == 6, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szHoo[1],6), "name wrong\n" );

    ResetEvent(ov.hEvent);
    SetLastError(0xd0b00b00);
    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,0,NULL,NULL,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,0,NULL,&ov,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    filter = FILE_NOTIFY_CHANGE_SIZE;

    SetEvent(ov.hEvent);
    ov.Internal = 1;
    ov.InternalHigh = 1;
    S(U(ov)).Offset = 0;
    S(U(ov)).OffsetHigh = 0;
    memset( buffer, 0, sizeof buffer );
    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    ok( ov.Internal == STATUS_PENDING, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 1, "ov.InternalHigh wrong\n");

    r = WaitForSingleObject( ov.hEvent, 0 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( ov.hEvent, INFINITE );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok( ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0x12, "ov.InternalHigh wrong\n");

    r = GetOverlappedResult( hdir, &ov, &dwCount, TRUE );
    ok( r == TRUE, "getoverlappedresult failed\n");
    ok( dwCount == 0x12, "count wrong\n");

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_REMOVED, "action wrong\n" );
    ok( pfni->FileNameLength == 6, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szHoo[1],6), "name wrong\n" );

    /* what happens if the buffer is too small? */
    r = pReadDirectoryChangesW(hdir,buffer,0x10,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( ov.hEvent, INFINITE );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok( ov.Internal == STATUS_NOTIFY_ENUM_DIR, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0, "ov.InternalHigh wrong\n");

    /* test the recursive watch */
    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = CreateDirectoryW( subsubdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( ov.hEvent, INFINITE );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok( ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0x18, "ov.InternalHigh wrong\n");

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_ADDED, "action wrong\n" );
    ok( pfni->FileNameLength == 0x0c, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szGa[1],6), "name wrong\n" );

    r = RemoveDirectoryW( subsubdir );
    ok( r == TRUE, "failed to remove directory\n");

    ov.Internal = 1;
    ov.InternalHigh = 1;
    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( ov.hEvent, INFINITE );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_REMOVED, "action wrong\n" );
    ok( pfni->FileNameLength == 0x0c, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szGa[1],6), "name wrong\n" );

    ok( ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0x18, "ov.InternalHigh wrong\n");

    CloseHandle(hdir);

    r = RemoveDirectoryW( path );
    ok( r == TRUE, "failed to remove directory\n");
}

/* show the behaviour when a null buffer is passed */
static void test_readdirectorychanges_null(void)
{
    NTSTATUS r;
    HANDLE hdir;
    char buffer[0x1000];
    DWORD fflags, filter = 0;
    OVERLAPPED ov;
    WCHAR path[MAX_PATH], subdir[MAX_PATH];
    static const WCHAR szBoo[] = { '\\','b','o','o',0 };
    static const WCHAR szHoo[] = { '\\','h','o','o',0 };
    PFILE_NOTIFY_INFORMATION pfni;

    if (!pReadDirectoryChangesW)
        return;

    r = GetTempPathW( MAX_PATH, path );
    ok( r != 0, "temp path failed\n");
    if (!r)
        return;

    lstrcatW( path, szBoo );
    lstrcpyW( subdir, path );
    lstrcatW( subdir, szHoo );

    RemoveDirectoryW( subdir );
    RemoveDirectoryW( path );

    r = CreateDirectoryW(path, NULL);
    ok( r == TRUE, "failed to create directory\n");

    fflags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
    hdir = CreateFileW(path, GENERIC_READ|SYNCHRONIZE|FILE_LIST_DIRECTORY,
                        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, fflags, NULL);
    ok( hdir != INVALID_HANDLE_VALUE, "failed to open directory\n");

    ov.hEvent = CreateEvent( NULL, 1, 0, NULL );

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;
    filter |= FILE_NOTIFY_CHANGE_DIR_NAME;

    SetLastError(0xd0b00b00);
    ov.Internal = 0;
    ov.InternalHigh = 0;
    memset( buffer, 0, sizeof buffer );

    r = pReadDirectoryChangesW(hdir,NULL,0,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = WaitForSingleObject( ov.hEvent, 0 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( ov.hEvent, 0 );
    ok( r == WAIT_OBJECT_0, "event should be ready\n" );

    ok( ov.Internal == STATUS_NOTIFY_ENUM_DIR, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0, "ov.InternalHigh wrong\n");

    ov.Internal = 0;
    ov.InternalHigh = 0;
    S(U(ov)).Offset = 0;
    S(U(ov)).OffsetHigh = 0;
    memset( buffer, 0, sizeof buffer );

    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = WaitForSingleObject( ov.hEvent, 0 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( ov.hEvent, INFINITE );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok( ov.Internal == STATUS_NOTIFY_ENUM_DIR, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0, "ov.InternalHigh wrong\n");

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );

    CloseHandle(hdir);

    r = RemoveDirectoryW( path );
    ok( r == TRUE, "failed to remove directory\n");
}

static void test_readdirectorychanges_filedir(void)
{
    NTSTATUS r;
    HANDLE hdir, hfile;
    char buffer[0x1000];
    DWORD fflags, filter = 0;
    OVERLAPPED ov;
    WCHAR path[MAX_PATH], subdir[MAX_PATH], file[MAX_PATH];
    static const WCHAR szBoo[] = { '\\','b','o','o',0 };
    static const WCHAR szHoo[] = { '\\','h','o','o',0 };
    static const WCHAR szFoo[] = { '\\','f','o','o',0 };
    PFILE_NOTIFY_INFORMATION pfni;

    r = GetTempPathW( MAX_PATH, path );
    ok( r != 0, "temp path failed\n");
    if (!r)
        return;

    lstrcatW( path, szBoo );
    lstrcpyW( subdir, path );
    lstrcatW( subdir, szHoo );

    lstrcpyW( file, path );
    lstrcatW( file, szFoo );

    DeleteFileW( file );
    RemoveDirectoryW( subdir );
    RemoveDirectoryW( path );

    r = CreateDirectoryW(path, NULL);
    ok( r == TRUE, "failed to create directory\n");

    fflags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
    hdir = CreateFileW(path, GENERIC_READ|SYNCHRONIZE|FILE_LIST_DIRECTORY,
                        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL,
                        OPEN_EXISTING, fflags, NULL);
    ok( hdir != INVALID_HANDLE_VALUE, "failed to open directory\n");

    ov.hEvent = CreateEvent( NULL, 0, 0, NULL );

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;

    r = pReadDirectoryChangesW(hdir,buffer,sizeof buffer,TRUE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = WaitForSingleObject( ov.hEvent, 10 );
    ok( r == WAIT_TIMEOUT, "should timeout\n" );

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    hfile = CreateFileW( file, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    ok( hfile != INVALID_HANDLE_VALUE, "failed to create file\n");
    ok( CloseHandle(hfile), "failed toc lose file\n");

    r = WaitForSingleObject( ov.hEvent, INFINITE );
    ok( r == WAIT_OBJECT_0, "event should be ready\n" );

    ok( ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0x12, "ov.InternalHigh wrong\n");

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_ADDED, "action wrong\n" );
    ok( pfni->FileNameLength == 6, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szFoo[1],6), "name wrong\n" );

    r = DeleteFileW( file );
    ok( r == TRUE, "failed to delete file\n");

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    CloseHandle(hdir);

    r = RemoveDirectoryW( path );
    ok( r == TRUE, "failed to remove directory\n");
}

START_TEST(change)
{
    HMODULE hkernel32 = GetModuleHandle("kernel32");
    pReadDirectoryChangesW = (fnReadDirectoryChangesW)
        GetProcAddress(hkernel32, "ReadDirectoryChangesW");

    test_FindFirstChangeNotification();
    test_ffcn();
    test_readdirectorychanges();
    test_readdirectorychanges_null();
    test_readdirectorychanges_filedir();
}
