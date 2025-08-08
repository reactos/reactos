/*
 * File change notification tests
 *
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

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windows.h>
#include <winnt.h>
#include <winternl.h>
#include <winerror.h>
#include <stdio.h>
#include "wine/test.h"

static NTSTATUS (WINAPI *pNtNotifyChangeDirectoryFile)(
                          HANDLE,HANDLE,PIO_APC_ROUTINE,PVOID,
                          PIO_STATUS_BLOCK,PVOID,ULONG,ULONG,BOOLEAN);

static NTSTATUS (WINAPI *pNtCancelIoFile)(HANDLE,PIO_STATUS_BLOCK);


static void test_ntncdf(void)
{
    NTSTATUS r;
    HANDLE hdir, hEvent;
    char buffer[0x1000];
    DWORD fflags, filter = 0;
    IO_STATUS_BLOCK iosb;
    WCHAR path[MAX_PATH], subdir[MAX_PATH];
    static const WCHAR szBoo[] = { '\\','b','o','o',0 };
    static const WCHAR szHoo[] = { '\\','h','o','o',0 };
    PFILE_NOTIFY_INFORMATION pfni;

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

    r = pNtNotifyChangeDirectoryFile(NULL,NULL,NULL,NULL,NULL,NULL,0,0,0);
    ok(r==STATUS_ACCESS_VIOLATION, "should return access violation\n");

    fflags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
    hdir = CreateFileW(path, GENERIC_READ|SYNCHRONIZE, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, fflags, NULL);
    ok( hdir != INVALID_HANDLE_VALUE, "failed to open directory\n");

    hEvent = CreateEventA( NULL, 0, 0, NULL );

    r = pNtNotifyChangeDirectoryFile(hdir,NULL,NULL,NULL,&iosb,NULL,0,0,0);
    ok(r==STATUS_INVALID_PARAMETER, "should return invalid parameter\n");

    r = pNtNotifyChangeDirectoryFile(hdir,hEvent,NULL,NULL,&iosb,NULL,0,0,0);
    ok(r==STATUS_INVALID_PARAMETER, "should return invalid parameter\n");

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;
    filter |= FILE_NOTIFY_CHANGE_DIR_NAME;
    filter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
    filter |= FILE_NOTIFY_CHANGE_SIZE;
    filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
    filter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
    filter |= FILE_NOTIFY_CHANGE_CREATION;
    filter |= FILE_NOTIFY_CHANGE_SECURITY;

    iosb.Status = 1;
    iosb.Information = 1;
    r = pNtNotifyChangeDirectoryFile(hdir,hEvent,NULL,NULL,&iosb,buffer,sizeof buffer,-1,0);
    ok(r==STATUS_INVALID_PARAMETER, "should return invalid parameter\n");

    ok( iosb.Status == 1, "information wrong\n");
    ok( iosb.Information == 1, "information wrong\n");

    iosb.Status = 1;
    iosb.Information = 0;
    r = pNtNotifyChangeDirectoryFile(hdir,hEvent,NULL,NULL,&iosb,buffer,sizeof buffer,filter,0);
    ok(r==STATUS_PENDING, "should return status pending\n");

    r = WaitForSingleObject( hEvent, 100 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = WaitForSingleObject( hdir, 100 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( hdir, 100 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = WaitForSingleObject( hEvent, 100 );
    ok( r == WAIT_OBJECT_0, "event should be ready\n" );

    ok( iosb.Status == STATUS_SUCCESS, "information wrong\n");
    ok( iosb.Information == 0x12, "information wrong\n");

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_ADDED, "action wrong\n" );
    ok( pfni->FileNameLength == 6, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szHoo[1],6), "name wrong\n" );

    r = pNtNotifyChangeDirectoryFile(hdir,0,NULL,NULL,&iosb,buffer,sizeof buffer,0,0);
    ok(r==STATUS_INVALID_PARAMETER, "should return invalid parameter\n");

    r = pNtNotifyChangeDirectoryFile(hdir,hEvent,NULL,NULL,&iosb,buffer,sizeof buffer,0,0);
    ok(r==STATUS_INVALID_PARAMETER, "should return invalid parameter\n");

    filter = FILE_NOTIFY_CHANGE_SIZE;

    iosb.Status = 1;
    iosb.Information = 1;
    r = pNtNotifyChangeDirectoryFile(hdir,0,NULL,NULL,&iosb,NULL,0,filter,0);
    ok(r==STATUS_PENDING, "should status pending\n");

    ok( iosb.Status == 1, "information wrong\n");
    ok( iosb.Information == 1, "information wrong\n");

    r = WaitForSingleObject( hdir, 0 );
    ok( r == STATUS_TIMEOUT, "should timeout\n" );

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( hdir, 100 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    r = WaitForSingleObject( hdir, 100 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok( iosb.Status == STATUS_NOTIFY_ENUM_DIR, "information wrong\n");
    ok( iosb.Information == 0, "information wrong\n");

    CloseHandle(hdir);
    CloseHandle(hEvent);

    r = RemoveDirectoryW( path );
    ok( r == TRUE, "failed to remove directory\n");
}


static void test_ntncdf_async(void)
{
    NTSTATUS r;
    HANDLE hdir, hEvent;
    char buffer[0x1000];
    DWORD fflags, filter = 0;
    IO_STATUS_BLOCK iosb, iosb2, iosb3;
    WCHAR path[MAX_PATH], subdir[MAX_PATH];
    static const WCHAR szBoo[] = { '\\','b','o','o',0 };
    static const WCHAR szHoo[] = { '\\','h','o','o',0 };
    PFILE_NOTIFY_INFORMATION pfni;

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

    r = pNtNotifyChangeDirectoryFile(NULL,NULL,NULL,NULL,NULL,NULL,0,0,0);
    ok(r==STATUS_ACCESS_VIOLATION, "should return access violation\n");

    fflags = FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED;
    hdir = CreateFileW(path, GENERIC_READ|SYNCHRONIZE, FILE_SHARE_READ, NULL, 
                        OPEN_EXISTING, fflags, NULL);
    ok( hdir != INVALID_HANDLE_VALUE, "failed to open directory\n");

    hEvent = CreateEventA( NULL, 0, 0, NULL );

    filter = FILE_NOTIFY_CHANGE_FILE_NAME;
    filter |= FILE_NOTIFY_CHANGE_DIR_NAME;
    filter |= FILE_NOTIFY_CHANGE_ATTRIBUTES;
    filter |= FILE_NOTIFY_CHANGE_SIZE;
    filter |= FILE_NOTIFY_CHANGE_LAST_WRITE;
    filter |= FILE_NOTIFY_CHANGE_LAST_ACCESS;
    filter |= FILE_NOTIFY_CHANGE_CREATION;
    filter |= FILE_NOTIFY_CHANGE_SECURITY;


    iosb.Status   = 0x01234567;
    iosb.Information = 0x12345678;
    r = pNtNotifyChangeDirectoryFile(hdir,0,NULL,NULL,&iosb,buffer,sizeof buffer,filter,0);
    ok(r==STATUS_PENDING, "should status pending\n");
    ok(iosb.Status == 0x01234567, "status set too soon\n");
    ok(iosb.Information == 0x12345678, "info set too soon\n");

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( hdir, 100 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok(iosb.Status == STATUS_SUCCESS, "status not successful\n");
    ok(iosb.Information == 0x12, "info not set\n");

    pfni = (PFILE_NOTIFY_INFORMATION) buffer;
    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_ADDED, "action wrong\n" );
    ok( pfni->FileNameLength == 6, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szHoo[1],6), "name wrong\n" );

    r = pNtNotifyChangeDirectoryFile(hdir,0,NULL,NULL,&iosb,buffer,sizeof buffer,filter,0);
    ok(r==STATUS_PENDING, "should status pending\n");

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( hdir, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok(iosb.Status == STATUS_SUCCESS, "status not successful\n");
    ok(iosb.Information == 0x12, "info not set\n");

    ok( pfni->NextEntryOffset == 0, "offset wrong\n" );
    ok( pfni->Action == FILE_ACTION_REMOVED, "action wrong\n" );
    ok( pfni->FileNameLength == 6, "len wrong\n" );
    ok( !memcmp(pfni->FileName,&szHoo[1],6), "name wrong\n" );

    /* check APCs */
    iosb.Status = 0;
    iosb.Information = 0;

    r = pNtNotifyChangeDirectoryFile(hdir,0,NULL,NULL,&iosb,NULL,0,filter,0);
    ok(r==STATUS_PENDING, "should status pending\n");

    r = CreateDirectoryW( subdir, NULL );
    ok( r == TRUE, "failed to create directory\n");

    r = WaitForSingleObject( hdir, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok(iosb.Status == STATUS_NOTIFY_ENUM_DIR, "status not successful\n");
    ok(iosb.Information == 0, "info not set\n");

    iosb.Status = 0;
    iosb.Information = 0;

    r = pNtNotifyChangeDirectoryFile(hdir,hEvent,NULL,NULL,&iosb,buffer,sizeof buffer,filter,0);
    ok(r==STATUS_PENDING, "should status pending\n");

    r = RemoveDirectoryW( subdir );
    ok( r == TRUE, "failed to remove directory\n");

    r = WaitForSingleObject( hEvent, 0 );
    ok( r == WAIT_OBJECT_0, "should be ready\n" );

    ok(iosb.Status == STATUS_SUCCESS, "status not successful\n");
    ok(iosb.Information == 0x12, "info not set\n");


    iosb.Status   = 0x01234567;
    iosb.Information = 0x12345678;
    r = pNtNotifyChangeDirectoryFile(hdir,0,NULL,NULL,&iosb,buffer,sizeof buffer,filter,0);
    ok(r==STATUS_PENDING, "should status pending\n");

    iosb2.Status   = 0x01234567;
    iosb2.Information = 0x12345678;
    r = pNtNotifyChangeDirectoryFile(hdir,0,NULL,NULL,&iosb2,buffer,sizeof buffer,filter,0);
    ok(r==STATUS_PENDING, "should status pending\n");

    ok(iosb.Status == 0x01234567, "status set too soon\n");
    ok(iosb.Information == 0x12345678, "info set too soon\n");

    iosb3.Status   = 0x111111;
    iosb3.Information = 0x222222;

    r = pNtCancelIoFile(hdir, &iosb3);
    ok( r == STATUS_SUCCESS, "cancel failed\n");

    CloseHandle(hdir);

    ok(iosb.Status == STATUS_CANCELLED, "status wrong %lx\n",iosb.Status);
    ok(iosb2.Status == STATUS_CANCELLED, "status wrong %lx\n",iosb2.Status);
    ok(iosb3.Status == STATUS_SUCCESS, "status wrong %lx\n",iosb3.Status);

    ok(iosb.Information == 0, "info wrong\n");
    ok(iosb2.Information == 0, "info wrong\n");
    ok(iosb3.Information == 0, "info wrong\n");

    iosb3.Status   = 0x111111;
    iosb3.Information = 0x222222;
    r = pNtCancelIoFile(hdir, &iosb3);
    ok( r == STATUS_INVALID_HANDLE, "cancel failed %lx\n", r);
    ok(iosb3.Status == 0x111111, "status wrong %lx\n",iosb3.Status);
    ok(iosb3.Information == 0x222222, "info wrong\n");

    r = RemoveDirectoryW( path );
    ok( r == TRUE, "failed to remove directory\n");

    CloseHandle(hEvent);
}

START_TEST(change)
{
    HMODULE hntdll = GetModuleHandleA("ntdll");

    pNtNotifyChangeDirectoryFile = (void *)GetProcAddress(hntdll, "NtNotifyChangeDirectoryFile");
    pNtCancelIoFile = (void *)GetProcAddress(hntdll, "NtCancelIoFile");

    if (!pNtNotifyChangeDirectoryFile || !pNtCancelIoFile)
    {
        win_skip("missing functions, skipping test\n");
        return;
    }

    test_ntncdf();
    test_ntncdf_async();
}
