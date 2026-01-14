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
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA
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
    HANDLE change = arg;
    BOOL notified = FALSE;
    BOOL ret = FALSE;
    DWORD status;

    status = WaitForSingleObject(change, 100);

    if (status == WAIT_OBJECT_0 ) {
        notified = TRUE;
        FindNextChangeNotification(change);
    }

    ret = FindCloseChangeNotification(change);
    ok( ret, "FindCloseChangeNotification error: %ld\n",
       GetLastError());

    ExitThread((DWORD)notified);
}

static HANDLE StartNotificationThread(LPCSTR path, BOOL subtree, DWORD flags)
{
    HANDLE change, thread;
    DWORD threadId;

    change = FindFirstChangeNotificationA(path, subtree, flags);
    ok(change != INVALID_HANDLE_VALUE, "FindFirstChangeNotification error: %ld\n", GetLastError());

    thread = CreateThread(NULL, 0, NotificationThread, change, 0, &threadId);
    ok(thread != NULL, "CreateThread error: %ld\n", GetLastError());

    return thread;
}

static DWORD FinishNotificationThread(HANDLE thread)
{
    DWORD status, exitcode;

    status = WaitForSingleObject(thread, 5000);
    ok(status == WAIT_OBJECT_0, "WaitForSingleObject status %ld error %ld\n", status, GetLastError());

    ok(GetExitCodeThread(thread, &exitcode), "Could not retrieve thread exit code\n");
    CloseHandle(thread);

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
    ok(change == INVALID_HANDLE_VALUE, "Expected INVALID_HANDLE_VALUE, got %p\n", change);
    ok(GetLastError() == ERROR_FILE_NOT_FOUND,
       "FindFirstChangeNotification error: %ld\n", GetLastError());

    change = FindFirstChangeNotificationA(NULL, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    ok(change == INVALID_HANDLE_VALUE || broken(change == NULL) /* < win7 */,
       "Expected INVALID_HANDLE_VALUE, got %p\n", change);
    ok(GetLastError() == ERROR_PATH_NOT_FOUND,
       "FindFirstChangeNotification error: %lu\n", GetLastError());

    ret = FindNextChangeNotification(NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "FindNextChangeNotification error: %ld\n",
       GetLastError());

    ret = FindCloseChangeNotification(NULL);
    ok(!ret && GetLastError() == ERROR_INVALID_HANDLE, "FindCloseChangeNotification error: %ld\n",
       GetLastError());

    ret = GetTempPathA(MAX_PATH, dirname1);
    ok(ret, "GetTempPathA error: %ld\n", GetLastError());

    ret = GetTempFileNameA(dirname1, "ffc", 0, workdir);
    ok(ret, "GetTempFileNameA error: %ld\n", GetLastError());
    DeleteFileA( workdir );

    ret = CreateDirectoryA(workdir, NULL);
    ok(ret, "CreateDirectoryA error: %ld\n", GetLastError());

    ret = GetTempFileNameA(workdir, prefix, 0, filename1);
    ok(ret, "GetTempFileNameA error: %ld\n", GetLastError());

    file = CreateFileA(filename1, GENERIC_WRITE|GENERIC_READ, 0, NULL, CREATE_ALWAYS,
                       FILE_ATTRIBUTE_NORMAL, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA error: %ld\n", GetLastError());
    ret = CloseHandle(file);
    ok( ret, "CloseHandle error: %ld\n", GetLastError());

    /* Try to register notification for a file */
    change = FindFirstChangeNotificationA(filename1, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    ok(change == INVALID_HANDLE_VALUE, "Expected INVALID_HANDLE_VALUE, got %p\n", change);
    ok(GetLastError() == ERROR_DIRECTORY,
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
    ok(!FinishNotificationThread(thread), "Got notification\n");

    /* What if we remove the directory we registered notification for? */
    thread = StartNotificationThread(dirname2, FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    ret = RemoveDirectoryA(dirname2);
    ok(ret, "RemoveDirectoryA error: %ld\n", GetLastError());
    ret = FinishNotificationThread(thread);
    todo_wine ok(ret, "Missed notification\n");

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
    memset(buffer, 0, sizeof(buffer));
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
    HANDLE handle, file;
    LONG r;
    WCHAR path[MAX_PATH], subdir[MAX_PATH], filename[MAX_PATH];
    static const WCHAR szBoo[] = { '\\','b','o','o',0 };
    static const WCHAR szHoo[] = { '\\','h','o','o',0 };
    static const WCHAR szZoo[] = { '\\','z','o','o',0 };

    r = GetTempPathW( MAX_PATH, path );
    ok( r != 0, "temp path failed\n");

    lstrcatW( path, szBoo );
    lstrcpyW( subdir, path );
    lstrcatW( subdir, szHoo );

    lstrcpyW( filename, path );
    lstrcatW( filename, szZoo );

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

    file = CreateFileW( filename, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, 0 );
    ok( file != INVALID_HANDLE_VALUE, "CreateFile error %lu\n", GetLastError() );
    CloseHandle(file);

    r = WaitForSingleObject( handle, 1000 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = FindNextChangeNotification(handle);
    ok( r == TRUE, "find next failed\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == STATUS_TIMEOUT, "should time out\n");

    r = DeleteFileW( filename );
    ok( r == TRUE, "failed to remove file\n");

    r = WaitForSingleObject( handle, 1000 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = FindNextChangeNotification(handle);
    ok( r == TRUE, "find next failed\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == STATUS_TIMEOUT, "should time out\n");

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create subdir\n");

    r = WaitForSingleObject( handle, 1000 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n");

    r = FindNextChangeNotification(handle);
    ok( r == TRUE, "find next failed\n");

    r = WaitForSingleObject( handle, 0 );
    ok( r == STATUS_TIMEOUT, "should time out\n");

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove subdir\n");

    r = WaitForSingleObject( handle, 1000 );
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

/* this test concentrates on the wait behavior when multiple threads are
 * waiting on a change notification handle. */
static void test_ffcnMultipleThreads(void)
{
    LONG r;
    DWORD filter, threadId, status, exitcode;
    HANDLE handles[2];
    char tmp[MAX_PATH], path[MAX_PATH];

    r = GetTempPathA(MAX_PATH, tmp);
    ok(r, "GetTempPathA error: %ld\n", GetLastError());

    r = GetTempFileNameA(tmp, "ffc", 0, path);
    ok(r, "GetTempFileNameA error: %ld\n", GetLastError());
    DeleteFileA( path );

    r = CreateDirectoryA(path, NULL);
    ok(r, "CreateDirectoryA error: %ld\n", GetLastError());

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;
    filter |= FILE_NOTIFY_CHANGE_DIR_NAME;

    handles[0] = FindFirstChangeNotificationA(path, FALSE, filter);
    ok(handles[0] != INVALID_HANDLE_VALUE, "FindFirstChangeNotification error: %ld\n", GetLastError());

    /* Test behavior if a waiting thread holds the last reference to a change
     * directory object with an empty wine user APC queue for this thread (bug #7286) */

    /* Create our notification thread */
    handles[1] = CreateThread(NULL, 0, NotificationThread, handles[0], 0,
                              &threadId);
    ok(handles[1] != NULL, "CreateThread error: %ld\n", GetLastError());

    status = WaitForMultipleObjects(2, handles, FALSE, 5000);
    ok(status == WAIT_OBJECT_0 || status == WAIT_OBJECT_0+1, "WaitForMultipleObjects status %ld error %ld\n", status, GetLastError());
    ok(GetExitCodeThread(handles[1], &exitcode), "Could not retrieve thread exit code\n");

    /* Clean up */
    r = RemoveDirectoryA( path );
    ok( r == TRUE, "failed to remove dir\n");
}

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
    BOOL got_subdir_change = FALSE;

    r = GetTempPathW( MAX_PATH, path );
    ok( r != 0, "temp path failed\n");

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
    r = ReadDirectoryChangesW(NULL,NULL,0,FALSE,0,NULL,NULL,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    fflags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
    hdir = CreateFileW(path, GENERIC_READ|SYNCHRONIZE|FILE_LIST_DIRECTORY, 
                        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, 
                        OPEN_EXISTING, fflags, NULL);
    ok( hdir != INVALID_HANDLE_VALUE, "failed to open directory\n");

    ov.hEvent = CreateEventW( NULL, 1, 0, NULL );

    SetLastError(0xd0b00b00);
    r = ReadDirectoryChangesW(hdir,NULL,0,FALSE,0,NULL,NULL,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    SetLastError(0xd0b00b00);
    r = ReadDirectoryChangesW(hdir,NULL,0,FALSE,0,NULL,&ov,NULL);
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

    r = ReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,-1,NULL,&ov,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    r = ReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,0,NULL,&ov,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    r = ReadDirectoryChangesW(hdir,buffer,sizeof buffer,TRUE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = WaitForSingleObject( ov.hEvent, 10 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "event should be ready\n" );

    ok( (NTSTATUS)ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0x12, "ov.InternalHigh wrong\n");

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_ADDED, "action wrong\n" );
    ok( pfni->FileNameLength == 6, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szHoo[1],6), "name wrong\n" );

    ResetEvent(ov.hEvent);
    SetLastError(0xd0b00b00);
    r = ReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,0,NULL,NULL,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    r = ReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,0,NULL,&ov,NULL);
    ok(GetLastError()==ERROR_INVALID_PARAMETER,"last error wrong\n");
    ok(r==FALSE, "should return false\n");

    filter = FILE_NOTIFY_CHANGE_SIZE;

    SetEvent(ov.hEvent);
    ov.Internal = 1;
    ov.InternalHigh = 1;
    ov.Offset = 0;
    ov.OffsetHigh = 0;
    memset( buffer, 0, sizeof buffer );
    r = ReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    ok( (NTSTATUS)ov.Internal == STATUS_PENDING, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 1, "ov.InternalHigh wrong\n");

    r = WaitForSingleObject( ov.hEvent, 0 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok( (NTSTATUS)ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0x12, "ov.InternalHigh wrong\n");

    if ((NTSTATUS)ov.Internal == STATUS_SUCCESS)
    {
        r = GetOverlappedResult( hdir, &ov, &dwCount, TRUE );
        ok( r == TRUE, "getoverlappedresult failed\n");
        ok( dwCount == 0x12, "count wrong\n");
    }

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_REMOVED, "action wrong\n" );
    ok( pfni->FileNameLength == 6, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szHoo[1],6), "name wrong\n" );

    /* what happens if the buffer is too small? */
    r = ReadDirectoryChangesW(hdir,buffer,0x10,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok( (NTSTATUS)ov.Internal == STATUS_NOTIFY_ENUM_DIR, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0, "ov.InternalHigh wrong\n");

    /* test the recursive watch */
    r = ReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = CreateDirectoryW( subsubdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    while (1)
    {
        r = WaitForSingleObject( ov.hEvent, 1000 );
        ok(r == WAIT_OBJECT_0, "should be ready\n" );
        if (r == WAIT_TIMEOUT) break;

        ok((NTSTATUS) ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");

        pfni = (PFILE_NOTIFY_INFORMATION) buffer;
        while (1)
        {
            /* We might get one or more modified events on the parent dir */
            if (pfni->Action == FILE_ACTION_MODIFIED)
            {
                ok(pfni->FileNameLength == 3 * sizeof(WCHAR), "len wrong\n" );
                ok(!memcmp(pfni->FileName, &szGa[1], 3 * sizeof(WCHAR)), "name wrong\n");
            }
            else
            {
                ok(pfni->Action == FILE_ACTION_ADDED, "action wrong\n");
                ok(pfni->FileNameLength == 6 * sizeof(WCHAR), "len wrong\n" );
                ok(!memcmp(pfni->FileName, &szGa[1], 6 * sizeof(WCHAR)), "name wrong\n");
                got_subdir_change = TRUE;
            }
            if (!pfni->NextEntryOffset) break;
            pfni = (PFILE_NOTIFY_INFORMATION)((char *)pfni + pfni->NextEntryOffset);
        }

        if (got_subdir_change) break;

        r = ReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
        ok(r==TRUE, "should return true\n");
    }
    ok(got_subdir_change, "didn't get subdir change\n");

    r = RemoveDirectoryW( subsubdir );
    ok( r == TRUE, "failed to remove directory\n");

    ov.Internal = 1;
    ov.InternalHigh = 1;
    r = ReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    /* we may get a notification for the parent dir too */
    if (pfni->Action == FILE_ACTION_MODIFIED && pfni->NextEntryOffset)
    {
        ok( pfni->FileNameLength == 3*sizeof(WCHAR), "len wrong %lu\n", pfni->FileNameLength );
        ok( !memcmp(pfni->FileName,&szGa[1],3*sizeof(WCHAR)), "name wrong\n" );
        pfni = (PFILE_NOTIFY_INFORMATION)((char *)pfni + pfni->NextEntryOffset);
    }
    ok( pfni->NextEntryOffset == 0, "offset wrong %lu\n", pfni->NextEntryOffset );
    ok( pfni->Action == FILE_ACTION_REMOVED, "action wrong %lu\n", pfni->Action );
    ok( pfni->FileNameLength == 6*sizeof(WCHAR), "len wrong %lu\n", pfni->FileNameLength );
    ok( !memcmp(pfni->FileName,&szGa[1],6*sizeof(WCHAR)), "name wrong\n" );

    ok( (NTSTATUS)ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
    dwCount = (char *)&pfni->FileName[pfni->FileNameLength/sizeof(WCHAR)] - buffer;
    ok( ov.InternalHigh == dwCount, "ov.InternalHigh wrong %Iu/%lu\n",ov.InternalHigh, dwCount );

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

    r = GetTempPathW( MAX_PATH, path );
    ok( r != 0, "temp path failed\n");

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

    ov.hEvent = CreateEventW( NULL, 1, 0, NULL );

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;
    filter |= FILE_NOTIFY_CHANGE_DIR_NAME;

    SetLastError(0xd0b00b00);
    ov.Internal = 0;
    ov.InternalHigh = 0;
    memset( buffer, 0, sizeof buffer );

    r = ReadDirectoryChangesW(hdir,NULL,0,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = WaitForSingleObject( ov.hEvent, 0 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( ov.hEvent, 0 );
    ok( r == WAIT_OBJECT_0, "event should be ready\n" );

    ok( (NTSTATUS)ov.Internal == STATUS_NOTIFY_ENUM_DIR, "ov.Internal wrong\n");
    ok( ov.InternalHigh == 0, "ov.InternalHigh wrong\n");

    ov.Internal = 0;
    ov.InternalHigh = 0;
    ov.Offset = 0;
    ov.OffsetHigh = 0;
    memset( buffer, 0, sizeof buffer );

    r = ReadDirectoryChangesW(hdir,buffer,sizeof buffer,FALSE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = WaitForSingleObject( ov.hEvent, 0 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok( (NTSTATUS)ov.Internal == STATUS_NOTIFY_ENUM_DIR, "ov.Internal wrong\n");
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

    ov.hEvent = CreateEventW( NULL, 0, 0, NULL );

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;

    r = ReadDirectoryChangesW(hdir,buffer,sizeof buffer,TRUE,filter,NULL,&ov,NULL);
    ok(r==TRUE, "should return true\n");

    r = WaitForSingleObject( ov.hEvent, 10 );
    ok( r == WAIT_TIMEOUT, "should timeout\n" );

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    hfile = CreateFileW( file, GENERIC_READ|GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL );
    ok( hfile != INVALID_HANDLE_VALUE, "failed to create file\n");
    ok( CloseHandle(hfile), "failed to close file\n");

    r = WaitForSingleObject( ov.hEvent, 1000 );
    ok( r == WAIT_OBJECT_0, "event should be ready\n" );

    ok( (NTSTATUS)ov.Internal == STATUS_SUCCESS, "ov.Internal wrong\n");
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

static void CALLBACK readdirectorychanges_cr(DWORD error, DWORD len, LPOVERLAPPED ov)
{
    ok(error == 0, "ReadDirectoryChangesW error %ld\n", error);
    ok(ov->hEvent == (void*)0xdeadbeef, "hEvent should not have changed\n");
}

static void test_readdirectorychanges_cr(void)
{
    static const WCHAR szBoo[] = { '\\','b','o','o','\\',0 };
    static const WCHAR szDir[] = { 'd','i','r',0 };
    static const WCHAR szFile[] = { 'f','i','l','e',0 };
    static const WCHAR szBackslash[] = { '\\',0 };

    WCHAR path[MAX_PATH], file[MAX_PATH], dir[MAX_PATH], sub_file[MAX_PATH];
    FILE_NOTIFY_INFORMATION fni[1024], *fni_next;
    OVERLAPPED ov;
    HANDLE hdir, hfile;
    NTSTATUS r;

    r = GetTempPathW(MAX_PATH, path);
    ok(r != 0, "temp path failed\n");

    lstrcatW(path, szBoo);
    lstrcpyW(dir, path);
    lstrcatW(dir, szDir);
    lstrcpyW(file, path);
    lstrcatW(file, szFile);
    lstrcpyW(sub_file, dir);
    lstrcatW(sub_file, szBackslash);
    lstrcatW(sub_file, szFile);

    DeleteFileW(file);
    RemoveDirectoryW(dir);
    RemoveDirectoryW(path);

    r = CreateDirectoryW(path, NULL);
    ok(r == TRUE, "failed to create directory\n");

    hdir = CreateFileW(path, GENERIC_READ,
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    ok(hdir != INVALID_HANDLE_VALUE, "failed to open directory\n");

    memset(&ov, 0, sizeof(ov));
    ov.hEvent = (void*)0xdeadbeef;
    r = ReadDirectoryChangesW(hdir, fni, sizeof(fni), FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME, NULL, &ov, readdirectorychanges_cr);
    ok(r == TRUE, "ReadDirectoryChangesW failed\n");

    hfile = CreateFileW(file, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 0, NULL);
    ok(hfile != INVALID_HANDLE_VALUE, "failed to create file\n");
    CloseHandle(hfile);

    r = SleepEx(1000, TRUE);
    ok(r != 0, "failed to receive file creation event\n");
    ok(fni->NextEntryOffset == 0, "there should be no more events in buffer\n");
    ok(fni->Action == FILE_ACTION_ADDED, "Action = %ld\n", fni->Action);
    ok(fni->FileNameLength == lstrlenW(szFile)*sizeof(WCHAR),
            "FileNameLength = %ld\n", fni->FileNameLength);
    ok(!memcmp(fni->FileName, szFile, lstrlenW(szFile)*sizeof(WCHAR)),
            "FileName = %s\n", wine_dbgstr_wn(fni->FileName, fni->FileNameLength/sizeof(WCHAR)));

    r = ReadDirectoryChangesW(hdir, fni, sizeof(fni), FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME, NULL, &ov, readdirectorychanges_cr);
    ok(r == TRUE, "ReadDirectoryChangesW failed\n");

    /* This event will not be reported */
    r = CreateDirectoryW(dir, NULL);
    ok(r == TRUE, "failed to create directory\n");

    r = MoveFileW(file, sub_file);
    ok(r == TRUE, "failed to move file\n");

    r = SleepEx(1000, TRUE);
    ok(r != 0, "failed to receive file move event\n");
    ok(fni->NextEntryOffset == 0, "there should be no more events in buffer\n");
    ok(fni->Action == FILE_ACTION_REMOVED, "Action = %ld\n", fni->Action);
    ok(fni->FileNameLength == lstrlenW(szFile)*sizeof(WCHAR),
            "FileNameLength = %ld\n", fni->FileNameLength);
    ok(!memcmp(fni->FileName, szFile, lstrlenW(szFile)*sizeof(WCHAR)),
            "FileName = %s\n", wine_dbgstr_wn(fni->FileName, fni->FileNameLength/sizeof(WCHAR)));

    r = ReadDirectoryChangesW(hdir, fni, sizeof(fni), FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME, NULL, &ov, readdirectorychanges_cr);
    ok(r == TRUE, "ReadDirectoryChangesW failed\n");

    r = MoveFileW(sub_file, file);
    ok(r == TRUE, "failed to move file\n");

    r = SleepEx(1000, TRUE);
    ok(r != 0, "failed to receive file move event\n");
    ok(fni->NextEntryOffset == 0, "there should be no more events in buffer\n");
    ok(fni->Action == FILE_ACTION_ADDED, "Action = %ld\n", fni->Action);
    ok(fni->FileNameLength == lstrlenW(szFile)*sizeof(WCHAR),
            "FileNameLength = %ld\n", fni->FileNameLength);
    ok(!memcmp(fni->FileName, szFile, lstrlenW(szFile)*sizeof(WCHAR)),
            "FileName = %s\n", wine_dbgstr_wn(fni->FileName, fni->FileNameLength/sizeof(WCHAR)));

    r = ReadDirectoryChangesW(hdir, fni, sizeof(fni), FALSE,
            FILE_NOTIFY_CHANGE_FILE_NAME, NULL, &ov, readdirectorychanges_cr);
    ok(r == TRUE, "ReadDirectoryChangesW failed\n");

    r = DeleteFileW(file);
    ok(r == TRUE, "failed to delete file\n");

    r = SleepEx(1000, TRUE);
    ok(r != 0, "failed to receive file removal event\n");
    ok(fni->NextEntryOffset == 0, "there should be no more events in buffer\n");
    ok(fni->Action == FILE_ACTION_REMOVED, "Action = %ld\n", fni->Action);
    ok(fni->FileNameLength == lstrlenW(szFile)*sizeof(WCHAR),
            "FileNameLength = %ld\n", fni->FileNameLength);
    ok(!memcmp(fni->FileName, szFile, lstrlenW(szFile)*sizeof(WCHAR)),
            "FileName = %s\n", wine_dbgstr_wn(fni->FileName, fni->FileNameLength/sizeof(WCHAR)));

    CloseHandle(hdir);

    hdir = CreateFileW(path, GENERIC_READ,
            FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, NULL, OPEN_EXISTING,
            FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED, NULL);
    ok(hdir != INVALID_HANDLE_VALUE, "failed to open directory\n");

    r = ReadDirectoryChangesW(hdir, fni, sizeof(fni), FALSE,
            FILE_NOTIFY_CHANGE_DIR_NAME, NULL, &ov, readdirectorychanges_cr);
    ok(r == TRUE, "ReadDirectoryChangesW failed\n");

    r = MoveFileW(dir, file);
    ok(r == TRUE, "failed to move directory\n");

    r = SleepEx(1000, TRUE);
    ok(r != 0, "failed to receive directory move event\n");
    if (fni->Action == FILE_ACTION_RENAMED_OLD_NAME)
    {
        ok(fni->Action == FILE_ACTION_RENAMED_OLD_NAME, "Action = %ld\n", fni->Action);
        ok(fni->FileNameLength == lstrlenW(szDir)*sizeof(WCHAR),
                "FileNameLength = %ld\n", fni->FileNameLength);
        ok(!memcmp(fni->FileName, szDir, lstrlenW(szDir)*sizeof(WCHAR)),
                "FileName = %s\n", wine_dbgstr_wn(fni->FileName, fni->FileNameLength/sizeof(WCHAR)));
        ok(fni->NextEntryOffset != 0, "no next entry in movement event\n");
        fni_next = (FILE_NOTIFY_INFORMATION*)((char*)fni+fni->NextEntryOffset);
        ok(fni_next->NextEntryOffset == 0, "there should be no more events in buffer\n");
        ok(fni_next->Action == FILE_ACTION_RENAMED_NEW_NAME, "Action = %ld\n", fni_next->Action);
        ok(fni_next->FileNameLength == lstrlenW(szFile)*sizeof(WCHAR),
                "FileNameLength = %ld\n", fni_next->FileNameLength);
        ok(!memcmp(fni_next->FileName, szFile, lstrlenW(szFile)*sizeof(WCHAR)),
                "FileName = %s\n", wine_dbgstr_wn(fni_next->FileName, fni_next->FileNameLength/sizeof(WCHAR)));
    }
    else
    {
        todo_wine ok(0, "Expected rename event\n");

        if (fni->NextEntryOffset == 0)
        {
            r = ReadDirectoryChangesW(hdir, fni, sizeof(fni), FALSE,
                    FILE_NOTIFY_CHANGE_DIR_NAME, NULL, &ov, readdirectorychanges_cr);
            ok(r == TRUE, "ReadDirectoryChangesW failed\n");

            r = SleepEx(1000, TRUE);
            ok(r != 0, "failed to receive directory move event\n");
        }
    }

    r = CreateDirectoryW(dir, NULL);
    ok(r == TRUE, "failed to create directory\n");

    r = RemoveDirectoryW(dir);
    ok(r == TRUE, "failed to remove directory\n");

    r = ReadDirectoryChangesW(hdir, fni, sizeof(fni), FALSE,
            FILE_NOTIFY_CHANGE_DIR_NAME, NULL, &ov, readdirectorychanges_cr);
    ok(r == TRUE, "ReadDirectoryChangesW failed\n");

    r = SleepEx(1000, TRUE);
    ok(r != 0, "failed to receive directory creation event\n");
    ok(fni->Action == FILE_ACTION_ADDED, "Action = %ld\n", fni->Action);
    ok(fni->FileNameLength == lstrlenW(szDir)*sizeof(WCHAR),
            "FileNameLength = %ld\n", fni->FileNameLength);
    ok(!memcmp(fni->FileName, szDir, lstrlenW(szDir)*sizeof(WCHAR)),
            "FileName = %s\n", wine_dbgstr_wn(fni->FileName, fni->FileNameLength/sizeof(WCHAR)));
    if (fni->NextEntryOffset)
        fni_next = (FILE_NOTIFY_INFORMATION*)((char*)fni+fni->NextEntryOffset);
    else
    {
        r = ReadDirectoryChangesW(hdir, fni, sizeof(fni), FALSE,
                FILE_NOTIFY_CHANGE_DIR_NAME, NULL, &ov, readdirectorychanges_cr);
        ok(r == TRUE, "ReadDirectoryChangesW failed\n");

        r = SleepEx(1000, TRUE);
        ok(r != 0, "failed to receive directory removal event\n");
        fni_next = fni;
    }
    ok(fni_next->NextEntryOffset == 0, "there should be no more events in buffer\n");
    ok(fni_next->Action == FILE_ACTION_REMOVED, "Action = %ld\n", fni_next->Action);
    ok(fni_next->FileNameLength == lstrlenW(szDir)*sizeof(WCHAR),
            "FileNameLength = %ld\n", fni_next->FileNameLength);
    ok(!memcmp(fni_next->FileName, szDir, lstrlenW(szDir)*sizeof(WCHAR)),
            "FileName = %s\n", wine_dbgstr_wn(fni_next->FileName, fni_next->FileNameLength/sizeof(WCHAR)));

    CloseHandle(hdir);
    RemoveDirectoryW(file);
    RemoveDirectoryW(path);
}

static void test_ffcn_directory_overlap(void)
{
    HANDLE parent_watch, child_watch, parent_thread, child_thread;
    char workdir[MAX_PATH], parentdir[MAX_PATH], childdir[MAX_PATH];
    char tempfile[MAX_PATH];
    DWORD threadId;
    BOOL ret;

    /* Setup directory hierarchy */
    ret = GetTempPathA(MAX_PATH, workdir);
    ok((ret > 0) && (ret <= MAX_PATH),
       "GetTempPathA error: %ld\n", GetLastError());

    ret = GetTempFileNameA(workdir, "fcn", 0, tempfile);
    ok(ret, "GetTempFileNameA error: %ld\n", GetLastError());
    ret = DeleteFileA(tempfile);
    ok(ret, "DeleteFileA error: %ld\n", GetLastError());

    lstrcpyA(parentdir, tempfile);
    ret = CreateDirectoryA(parentdir, NULL);
    ok(ret, "CreateDirectoryA error: %ld\n", GetLastError());

    lstrcpyA(childdir, parentdir);
    lstrcatA(childdir, "\\c");
    ret = CreateDirectoryA(childdir, NULL);
    ok(ret, "CreateDirectoryA error: %ld\n", GetLastError());


    /* When recursively watching overlapping directories, changes in child
     * should trigger notifications for both child and parent */
    parent_thread = StartNotificationThread(parentdir, TRUE,
                                            FILE_NOTIFY_CHANGE_FILE_NAME);
    child_thread = StartNotificationThread(childdir, TRUE,
                                            FILE_NOTIFY_CHANGE_FILE_NAME);

    /* Create a file in child */
    ret = GetTempFileNameA(childdir, "fcn", 0, tempfile);
    ok(ret, "GetTempFileNameA error: %ld\n", GetLastError());

    /* Both watches should trigger */
    ret = FinishNotificationThread(parent_thread);
    ok(ret, "Missed parent notification\n");
    ret = FinishNotificationThread(child_thread);
    ok(ret, "Missed child notification\n");

    ret = DeleteFileA(tempfile);
    ok(ret, "DeleteFileA error: %ld\n", GetLastError());


    /* Removing a recursive parent watch should not affect child watches. Doing
     * so used to crash wineserver. */
    parent_watch = FindFirstChangeNotificationA(parentdir, TRUE,
                                                FILE_NOTIFY_CHANGE_FILE_NAME);
    ok(parent_watch != INVALID_HANDLE_VALUE,
       "FindFirstChangeNotification error: %ld\n", GetLastError());
    child_watch = FindFirstChangeNotificationA(childdir, TRUE,
                                               FILE_NOTIFY_CHANGE_FILE_NAME);
    ok(child_watch != INVALID_HANDLE_VALUE,
       "FindFirstChangeNotification error: %ld\n", GetLastError());

    ret = FindCloseChangeNotification(parent_watch);
    ok(ret, "FindCloseChangeNotification error: %ld\n", GetLastError());

    child_thread = CreateThread(NULL, 0, NotificationThread, child_watch, 0,
                                &threadId);
    ok(child_thread != NULL, "CreateThread error: %ld\n", GetLastError());

    /* Create a file in child */
    ret = GetTempFileNameA(childdir, "fcn", 0, tempfile);
    ok(ret, "GetTempFileNameA error: %ld\n", GetLastError());

    /* Child watch should trigger */
    ret = FinishNotificationThread(child_thread);
    ok(ret, "Missed child notification\n");

    /* clean up */
    ret = DeleteFileA(tempfile);
    ok(ret, "DeleteFileA error: %ld\n", GetLastError());

    ret = RemoveDirectoryA(childdir);
    ok(ret, "RemoveDirectoryA error: %ld\n", GetLastError());

    ret = RemoveDirectoryA(parentdir);
    ok(ret, "RemoveDirectoryA error: %ld\n", GetLastError());
}

START_TEST(change)
{
    test_ffcnMultipleThreads();
    /* The above function runs a test that must occur before FindCloseChangeNotification is run in the
       current thread to preserve the emptiness of the wine user APC queue. To ensure this it should be
       placed first. */
    test_FindFirstChangeNotification();
    test_ffcn();
    test_readdirectorychanges();
    test_readdirectorychanges_null();
    test_readdirectorychanges_filedir();
    test_readdirectorychanges_cr();
    test_ffcn_directory_overlap();
}
