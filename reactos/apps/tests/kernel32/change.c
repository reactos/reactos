/*
 * Tests for file change notification functions
 *
 * Copyright (c) 2004 Hans Leidekker
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

#include "wine/test.h"
#include <windef.h>
#include <winbase.h>

static DWORD CALLBACK NotificationThread(LPVOID arg)
{
    HANDLE change = (HANDLE) arg;
    BOOL ret = FALSE;
    DWORD status;

    status = WaitForSingleObject(change, 100);

    if (status == WAIT_OBJECT_0 ) {
        ret = FindNextChangeNotification(change);
    }

    ok(FindCloseChangeNotification(change), "FindCloseChangeNotification error: %ld\n",
       GetLastError());

    ExitThread((DWORD)ret);
}

static HANDLE StartNotificationThread(LPCSTR path, BOOL subtree, DWORD flags)
{
    HANDLE change, thread;
    DWORD threadId;

    change = FindFirstChangeNotificationA(path, subtree, flags);
    ok(change != INVALID_HANDLE_VALUE, "FindFirstChangeNotification error: %ld\n", GetLastError());

    thread = CreateThread(NULL, 0, (LPTHREAD_START_ROUTINE)NotificationThread, (LPVOID)change,
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
    ok(CloseHandle(file), "CloseHandle error: %ld\n", GetLastError());

    /* Try to register notification for a file. win98 and win2k behave differently here */
    change = FindFirstChangeNotificationA(filename1, FALSE, FILE_NOTIFY_CHANGE_FILE_NAME);
    ok(change == INVALID_HANDLE_VALUE && (GetLastError() == ERROR_DIRECTORY ||
                                          GetLastError() == ERROR_FILE_NOT_FOUND),
       "FindFirstChangeNotification error: %ld\n", GetLastError());

    lstrcpyA(dirname1, filename1);
    lstrcatA(dirname1, "dir");

    ret = CreateDirectoryA(dirname1, NULL);
    ok(ret, "CreateDirectoryA error: %ld\n", GetLastError());

    /* What if we remove the directory we registered notification for? */
    thread = StartNotificationThread(dirname1, FALSE, FILE_NOTIFY_CHANGE_DIR_NAME);
    ret = RemoveDirectoryA(dirname1);
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

    lstrcpyA(dirname2, dirname1);
    lstrcatA(dirname2, "new");

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
    ok(CloseHandle(file), "CloseHandle error: %ld\n", GetLastError());
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
    ok(CloseHandle(file), "CloseHandle error: %ld\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* Change file size by truncating a file */
    thread = StartNotificationThread(workdir, FALSE, FILE_NOTIFY_CHANGE_SIZE);
    file = CreateFileA(filename2, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, 
                       FILE_ATTRIBUTE_NORMAL, 0);
    ok(file != INVALID_HANDLE_VALUE, "CreateFileA error: %ld\n", GetLastError());
    ret = WriteFile(file, buffer, sizeof(buffer) / 2, &count, NULL);
    ok(ret && count == sizeof(buffer) / 2, "WriteFileA error: %ld\n", GetLastError());
    ok(CloseHandle(file), "CloseHandle error: %ld\n", GetLastError());
    ok(FinishNotificationThread(thread), "Missed notification\n");

    /* clean up */
    
    ret = DeleteFileA(filename2);
    ok(ret, "DeleteFileA error: %ld\n", GetLastError());

    ret = RemoveDirectoryA(workdir);
    ok(ret, "RemoveDirectoryA error: %ld\n", GetLastError());
}

START_TEST(change)
{
    test_FindFirstChangeNotification();
}
