/*
 * Unit tests for Event Logging functions
 *
 * Copyright (c) 2009 Paul Vriens
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

#include "initguid.h"
#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnt.h"
#include "winreg.h"
#include "sddl.h"
#include "wmistr.h"
#include "evntprov.h"
#include "evntrace.h"
#include "netevent.h"

#include "wine/test.h"

static BOOL (WINAPI *pCreateWellKnownSid)(WELL_KNOWN_SID_TYPE,PSID,PSID,DWORD*);
static BOOL (WINAPI *pGetEventLogInformation)(HANDLE,DWORD,LPVOID,DWORD,LPDWORD);
static ULONG (WINAPI *pEventRegister)(const GUID *,PENABLECALLBACK,void *,REGHANDLE *);
static ULONG (WINAPI *pEventUnregister)(REGHANDLE);
static ULONG (WINAPI *pEventWriteString)(REGHANDLE,UCHAR,ULONGLONG,const WCHAR *);

static BOOL (WINAPI *pGetComputerNameExA)(COMPUTER_NAME_FORMAT,LPSTR,LPDWORD);
static BOOL (WINAPI *pWow64DisableWow64FsRedirection)(PVOID *);
static BOOL (WINAPI *pWow64RevertWow64FsRedirection)(PVOID);

static void init_function_pointers(void)
{
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");

    pCreateWellKnownSid = (void*)GetProcAddress(hadvapi32, "CreateWellKnownSid");
    pGetEventLogInformation = (void*)GetProcAddress(hadvapi32, "GetEventLogInformation");
    pEventWriteString = (void*)GetProcAddress(hadvapi32, "EventWriteString");
    pEventRegister = (void*)GetProcAddress(hadvapi32, "EventRegister");
    pEventUnregister = (void*)GetProcAddress(hadvapi32, "EventUnregister");

    pGetComputerNameExA = (void*)GetProcAddress(hkernel32, "GetComputerNameExA");
    pWow64DisableWow64FsRedirection = (void*)GetProcAddress(hkernel32, "Wow64DisableWow64FsRedirection");
    pWow64RevertWow64FsRedirection = (void*)GetProcAddress(hkernel32, "Wow64RevertWow64FsRedirection");
}

static BOOL create_backup(const char *filename)
{
    HANDLE handle;
    DWORD rc, attribs;

    handle = OpenEventLogA(NULL, "Application");
    if (!handle && (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == RPC_S_SERVER_UNAVAILABLE))
    {
        win_skip( "Can't open event log\n" );
        return FALSE;
    }
    ok(handle != NULL, "OpenEventLogA(Application) failed : %ld\n", GetLastError());

    DeleteFileA(filename);
    rc = BackupEventLogA(handle, filename);
    if (!rc && GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
    {
        skip("insufficient privileges to backup the eventlog\n");
        CloseEventLog(handle);
        return FALSE;
    }
    ok(rc, "BackupEventLogA failed, le=%lu\n", GetLastError());
    CloseEventLog(handle);

    attribs = GetFileAttributesA(filename);
    todo_wine
    ok(attribs != INVALID_FILE_ATTRIBUTES, "Expected a backup file attribs=%#lx le=%lu\n", attribs, GetLastError());
    return TRUE;
}

static void test_open_close(void)
{
    HANDLE handle;
    BOOL ret;

    SetLastError(0xdeadbeef);
    ret = CloseEventLog(NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE ||
       GetLastError() == ERROR_NOACCESS, /* W2K */
       "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenEventLogA(NULL, NULL);
    ok(handle == NULL, "OpenEventLogA() succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenEventLogA("IDontExist", NULL);
    ok(handle == NULL, "OpenEventLogA(IDontExist,) succeeded\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenEventLogA("IDontExist", "deadbeef");
    ok(handle == NULL, "OpenEventLogA(IDontExist,deadbeef) succeeded\n");
    ok(GetLastError() == RPC_S_SERVER_UNAVAILABLE ||
       GetLastError() == RPC_S_INVALID_NET_ADDR, /* Some Vista and Win7 */
       "Expected RPC_S_SERVER_UNAVAILABLE, got %ld\n", GetLastError());

    /* This one opens the Application log */
    handle = OpenEventLogA(NULL, "deadbeef");
    ok(handle != NULL, "OpenEventLogA(deadbeef) failed : %ld\n", GetLastError());
    ret = CloseEventLog(handle);
    ok(ret, "Expected success : %ld\n", GetLastError());
    /* Close a second time */
    SetLastError(0xdeadbeef);
    ret = CloseEventLog(handle);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());
    }

    /* Empty servername should be read as local server */
    handle = OpenEventLogA("", "Application");
    ok(handle != NULL, "OpenEventLogA('',Application) failed : %ld\n", GetLastError());
    CloseEventLog(handle);

    handle = OpenEventLogA(NULL, "Application");
    ok(handle != NULL, "OpenEventLogA(Application) failed : %ld\n", GetLastError());
    CloseEventLog(handle);
}

static void test_info(void)
{
    HANDLE handle;
    BOOL ret;
    DWORD needed;
    BYTE buffer[2 * sizeof(EVENTLOG_FULL_INFORMATION)];
    EVENTLOG_FULL_INFORMATION *efi = (void *)buffer;

    if (!pGetEventLogInformation)
    {
        /* NT4 */
        win_skip("GetEventLogInformation is not available\n");
        return;
    }
    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(NULL, 1, NULL, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_LEVEL, "Expected ERROR_INVALID_LEVEL, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(NULL, EVENTLOG_FULL_INFO, NULL, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

    handle = OpenEventLogA(NULL, "Application");
    ok(handle != NULL, "OpenEventLogA(Application) failed : %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, NULL, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "Expected RPC_X_NULL_REF_POINTER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, NULL, 0, &needed);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "Expected RPC_X_NULL_REF_POINTER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, efi, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "Expected RPC_X_NULL_REF_POINTER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    needed = 0xdeadbeef;
    efi->dwFull = 0xdeadbeef;
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, efi, 0, &needed);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());
    ok(needed == sizeof(EVENTLOG_FULL_INFORMATION), "Expected sizeof(EVENTLOG_FULL_INFORMATION), got %ld\n", needed);
    ok(efi->dwFull == 0xdeadbeef, "Expected no change to the dwFull member\n");

    /* Not that we care, but on success last error is set to ERROR_IO_PENDING */
    efi->dwFull = 0xdeadbeef;
    needed = sizeof(buffer);
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, efi, needed, &needed);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(needed == sizeof(EVENTLOG_FULL_INFORMATION), "Expected sizeof(EVENTLOG_FULL_INFORMATION), got %ld\n", needed);
    ok(efi->dwFull == 0 || efi->dwFull == 1, "Expected 0 (not full) or 1 (full), got %ld\n", efi->dwFull);

    CloseEventLog(handle);
}

static void test_count(void)
{
    HANDLE handle;
    BOOL ret;
    DWORD count;
    const char backup[] = "backup.evt";

    SetLastError(0xdeadbeef);
    ret = GetNumberOfEventLogRecords(NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    count = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(NULL, &count);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());
    ok(count == 0xdeadbeef, "Expected count to stay unchanged\n");

    handle = OpenEventLogA(NULL, "Application");
    ok(handle != NULL, "OpenEventLogA(Application) failed : %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetNumberOfEventLogRecords(handle, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    count = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(handle, &count);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(count != 0xdeadbeef, "Expected the number of records\n");

    CloseEventLog(handle);

    /* Make a backup eventlog to work with */
    if (create_backup(backup))
    {
        handle = OpenBackupEventLogA(NULL, backup);
        todo_wine
        ok(handle != NULL, "Expected a handle, le=%ld\n", GetLastError());

        /* Does GetNumberOfEventLogRecords work with backup eventlogs? */
        count = 0xdeadbeef;
        ret = GetNumberOfEventLogRecords(handle, &count);
        todo_wine
        {
        ok(ret, "Expected success : %ld\n", GetLastError());
        ok(count != 0xdeadbeef, "Expected the number of records\n");
        }

        CloseEventLog(handle);
        DeleteFileA(backup);
    }
}

static void test_oldest(void)
{
    HANDLE handle;
    BOOL ret;
    DWORD oldest;
    const char backup[] = "backup.evt";

    SetLastError(0xdeadbeef);
    ret = GetOldestEventLogRecord(NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    oldest = 0xdeadbeef;
    ret = GetOldestEventLogRecord(NULL, &oldest);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());
    ok(oldest == 0xdeadbeef, "Expected oldest to stay unchanged\n");

    handle = OpenEventLogA(NULL, "Application");
    if (!handle && (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == RPC_S_SERVER_UNAVAILABLE))
    {
        win_skip( "Can't open event log\n" );
        return;
    }
    ok(handle != NULL, "OpenEventLogA(Application) failed : %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = GetOldestEventLogRecord(handle, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    oldest = 0xdeadbeef;
    ret = GetOldestEventLogRecord(handle, &oldest);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(oldest != 0xdeadbeef, "Expected the number of the oldest record\n");

    CloseEventLog(handle);

    /* Make a backup eventlog to work with */
    if (create_backup(backup))
    {
        handle = OpenBackupEventLogA(NULL, backup);
        todo_wine
        ok(handle != NULL, "Expected a handle\n");

        /* Does GetOldestEventLogRecord work with backup eventlogs? */
        oldest = 0xdeadbeef;
        ret = GetOldestEventLogRecord(handle, &oldest);
        todo_wine
        {
        ok(ret, "Expected success : %ld\n", GetLastError());
        ok(oldest != 0xdeadbeef, "Expected the number of the oldest record\n");
        }

        CloseEventLog(handle);
        DeleteFileA(backup);
    }
}

static void test_backup(void)
{
    HANDLE handle;
    BOOL ret;
    const char backup[] = "backup.evt";
    const char backup2[] = "backup2.evt";

    SetLastError(0xdeadbeef);
    ret = BackupEventLogA(NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = BackupEventLogA(NULL, backup);
    ok(!ret, "Expected failure\n");
    ok(GetFileAttributesA(backup) == INVALID_FILE_ATTRIBUTES, "Expected no backup file\n");

    handle = OpenEventLogA(NULL, "Application");
    if (!handle && (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == RPC_S_SERVER_UNAVAILABLE))
    {
        win_skip( "Can't open event log\n" );
        return;
    }
    ok(handle != NULL, "OpenEventLogA(Application) failed : %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = BackupEventLogA(handle, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    ret = BackupEventLogA(handle, backup);
    if (!ret && GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
    {
        skip("insufficient privileges for backup tests\n");
        CloseEventLog(handle);
        return;
    }
    ok(ret, "Expected success : %ld\n", GetLastError());
    todo_wine
    ok(GetFileAttributesA(backup) != INVALID_FILE_ATTRIBUTES, "Expected a backup file\n");

    /* Try to overwrite */
    SetLastError(0xdeadbeef);
    ret = BackupEventLogA(handle, backup);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_ALREADY_EXISTS, "Expected ERROR_ALREADY_EXISTS, got %ld\n", GetLastError());
    }

    CloseEventLog(handle);

    /* Can we make a backup of a backup? */
    handle = OpenBackupEventLogA(NULL, backup);
    todo_wine
    ok(handle != NULL, "Expected a handle\n");

    ret = BackupEventLogA(handle, backup2);
    todo_wine
    {
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(GetFileAttributesA(backup2) != INVALID_FILE_ATTRIBUTES, "Expected a backup file\n");
    }

    CloseEventLog(handle);
    DeleteFileA(backup);
    DeleteFileA(backup2);
}

static void test_read(void)
{
    HANDLE handle;
    BOOL ret;
    DWORD count, toread, read, needed;
    void *buf;

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, 0, 0, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    read = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, 0, 0, NULL, 0, &read, NULL);
    ok(!ret, "Expected failure\n");
    ok(read == 0xdeadbeef, "Expected 'read' parameter to remain unchanged\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, 0, 0, NULL, 0, NULL, &needed);
    ok(!ret, "Expected failure\n");
    ok(needed == 0xdeadbeef, "Expected 'needed' parameter to remain unchanged\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    /* 'read' and 'needed' are only filled when the needed buffer size is passed back or when the call succeeds */
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, 0, 0, NULL, 0, &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, NULL, 0, &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    buf = NULL;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    buf = malloc(sizeof(EVENTLOGRECORD));
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());
    free(buf);

    handle = OpenEventLogA(NULL, "Application");
    if (!handle && (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == RPC_S_SERVER_UNAVAILABLE))
    {
        win_skip( "Can't open event log\n" );
        return;
    }
    ok(handle != NULL, "OpenEventLogA(Application) failed : %ld\n", GetLastError());

    /* Show that we need the proper dwFlags with a (for the rest) proper call */
    buf = malloc(sizeof(EVENTLOGRECORD));

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, 0, 0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ, 0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEEK_READ, 0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEEK_READ | EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    free(buf);

    /* First check if there are any records (in practice only on Wine: FIXME) */
    count = 0;
    GetNumberOfEventLogRecords(handle, &count);
    if (!count)
    {
        skip("No records in the 'Application' log\n");
        CloseEventLog(handle);
        return;
    }

    /* Get the buffer size for the first record */
    buf = malloc(sizeof(EVENTLOGRECORD));
    read = needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    ok(read == 0, "Expected no bytes read\n");
    ok(needed > sizeof(EVENTLOGRECORD), "Expected the needed buffersize to be bigger than sizeof(EVENTLOGRECORD)\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Expected ERROR_INSUFFICIENT_BUFFER, got %ld\n", GetLastError());

    /* Read the first record */
    toread = needed;
    buf = realloc(buf, toread);
    read = needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, buf, toread, &read, &needed);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(read == toread ||
       broken(read < toread), /* NT4 wants a buffer size way bigger than just 1 record */
       "Expected the requested size to be read\n");
    ok(needed == 0, "Expected no extra bytes to be read\n");
    free(buf);

    CloseEventLog(handle);
}

static void test_openbackup(void)
{
    HANDLE handle, handle2, file;
    DWORD written;
    const char backup[] = "backup.evt";
    const char text[] = "Just some text";

    SetLastError(0xdeadbeef);
    handle = OpenBackupEventLogA(NULL, NULL);
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenBackupEventLogA(NULL, "idontexist.evt");
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND ||
       GetLastError() == ERROR_ACCESS_DENIED ||
       GetLastError() == RPC_S_SERVER_UNAVAILABLE,
       "got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenBackupEventLogA("IDontExist", NULL);
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenBackupEventLogA("IDontExist", "idontexist.evt");
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == RPC_S_SERVER_UNAVAILABLE ||
       GetLastError() == RPC_S_INVALID_NET_ADDR, /* Some Vista and Win7 */
       "Expected RPC_S_SERVER_UNAVAILABLE, got %ld\n", GetLastError());

    /* Make a backup eventlog to work with */
    if (create_backup(backup))
    {
        /* FIXME: Wine stops here */
        if (GetFileAttributesA(backup) == INVALID_FILE_ATTRIBUTES)
        {
            skip("We don't have a backup eventlog to work with\n");
            return;
        }

        SetLastError(0xdeadbeef);
        handle = OpenBackupEventLogA("IDontExist", backup);
        ok(handle == NULL, "Didn't expect a handle\n");
        ok(GetLastError() == RPC_S_SERVER_UNAVAILABLE ||
           GetLastError() == RPC_S_INVALID_NET_ADDR, /* Some Vista and Win7 */
           "Expected RPC_S_SERVER_UNAVAILABLE, got %ld\n", GetLastError());

        /* Empty servername should be read as local server */
        handle = OpenBackupEventLogA("", backup);
        ok(handle != NULL, "Expected a handle\n");
        CloseEventLog(handle);

        handle = OpenBackupEventLogA(NULL, backup);
        ok(handle != NULL, "Expected a handle\n");

        /* Can we open that same backup eventlog more than once? */
        handle2 = OpenBackupEventLogA(NULL, backup);
        ok(handle2 != NULL, "Expected a handle\n");
        ok(handle2 != handle, "Didn't expect the same handle\n");
        CloseEventLog(handle2);

        CloseEventLog(handle);
        DeleteFileA(backup);
    }

    /* Is there any content checking done? */
    file = CreateFileA(backup, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    CloseHandle(file);
    SetLastError(0xdeadbeef);
    handle = OpenBackupEventLogA(NULL, backup);
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == ERROR_NOT_ENOUGH_MEMORY ||
       GetLastError() == ERROR_ACCESS_DENIED ||
       GetLastError() == RPC_S_SERVER_UNAVAILABLE ||
       GetLastError() == ERROR_EVENTLOG_FILE_CORRUPT, /* Vista and Win7 */
       "got %ld\n", GetLastError());
    CloseEventLog(handle);
    DeleteFileA(backup);

    file = CreateFileA(backup, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    WriteFile(file, text, sizeof(text), &written, NULL);
    CloseHandle(file);
    SetLastError(0xdeadbeef);
    handle = OpenBackupEventLogA(NULL, backup);
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == ERROR_EVENTLOG_FILE_CORRUPT ||
       GetLastError() == ERROR_ACCESS_DENIED ||
       GetLastError() == RPC_S_SERVER_UNAVAILABLE,
       "got %ld\n", GetLastError());
    CloseEventLog(handle);
    DeleteFileA(backup);
}

static void test_clear(void)
{
    HANDLE handle;
    BOOL ret;
    const char backup[] = "backup.evt";
    const char backup2[] = "backup2.evt";

    SetLastError(0xdeadbeef);
    ret = ClearEventLogA(NULL, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

    /* Make a backup eventlog to work with */
    if (!create_backup(backup))
        return;

    SetLastError(0xdeadbeef);
    ret = ClearEventLogA(NULL, backup);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

    handle = OpenBackupEventLogA(NULL, backup);
    todo_wine
    ok(handle != NULL, "Expected a handle\n");

    /* A real eventlog would fail with ERROR_ALREADY_EXISTS */
    SetLastError(0xdeadbeef);
    ret = ClearEventLogA(handle, backup);
    ok(!ret, "Expected failure\n");
    /* The eventlog service runs under an account that doesn't have the necessary
     * permissions on the users home directory on a default Vista+ system.
     */
    ok(GetLastError() == ERROR_INVALID_HANDLE ||
       GetLastError() == ERROR_ACCESS_DENIED, /* Vista+ */
       "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

    /* Show that ClearEventLog only works for real eventlogs. */
    SetLastError(0xdeadbeef);
    ret = ClearEventLogA(handle, backup2);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());
    ok(GetFileAttributesA(backup2) == INVALID_FILE_ATTRIBUTES, "Expected no backup file\n");

    SetLastError(0xdeadbeef);
    ret = ClearEventLogA(handle, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %ld\n", GetLastError());

    CloseEventLog(handle);
    todo_wine
    ok(DeleteFileA(backup), "Could not delete the backup file\n");
}

static const char eventlogsvc[] = "SYSTEM\\CurrentControlSet\\Services\\Eventlog";
static const char eventlogname[] = "Wine";
static const char eventsources[][11] = { "WineSrc", "WineSrc1", "WineSrc20", "WineSrc300" };

static BOOL create_new_eventlog(void)
{
    HKEY key, eventkey;
    BOOL bret = FALSE;
    LONG lret;
    DWORD i;

    /* First create our eventlog */
    lret = RegOpenKeyA(HKEY_LOCAL_MACHINE, eventlogsvc, &key);
    if (lret != ERROR_SUCCESS)
    {
        skip("Could not open the EventLog service registry key\n");
        return FALSE;
    }
    lret = RegCreateKeyA(key, eventlogname, &eventkey);
    if (lret != ERROR_SUCCESS)
    {
        skip("Could not create the eventlog '%s' registry key\n", eventlogname);
        goto cleanup;
    }

    /* Create some event sources, the registry value 'Sources' is updated automatically */
    for (i = 0; i < ARRAY_SIZE(eventsources); i++)
    {
        HKEY srckey;

        lret = RegCreateKeyA(eventkey, eventsources[i], &srckey);
        if (lret != ERROR_SUCCESS)
        {
            skip("Could not create the eventsource '%s' registry key\n", eventsources[i]);
            goto cleanup;
        }
        RegFlushKey(srckey);
        RegCloseKey(srckey);
    }

    bret = TRUE;

    /* The flushing of the registry (here and above) gives us some assurance
     * that we are not to quickly writing events as 'Sources' could still be
     * not updated.
     */
    RegFlushKey(eventkey);
cleanup:
    RegCloseKey(eventkey);
    RegCloseKey(key);

    return bret;
}

static const char *one_string[] = { "First string" };
static const char *two_strings[] = { "First string", "Second string" };
static const struct
{
    const char  *evt_src;
    WORD         evt_type;
    WORD         evt_cat;
    DWORD        evt_id;
    BOOL         evt_sid;
    WORD         evt_numstrings;
    const char **evt_strings;
} read_write [] =
{
    { eventlogname,    EVENTLOG_INFORMATION_TYPE, 1, 1,  FALSE, 1, one_string },
    { eventsources[0], EVENTLOG_WARNING_TYPE,     1, 2,  FALSE, 0, NULL },
    { eventsources[1], EVENTLOG_AUDIT_FAILURE,    1, 3,  FALSE, 2, two_strings },
    { eventsources[2], EVENTLOG_ERROR_TYPE,       1, 4,  FALSE, 0, NULL },
    { eventsources[3], EVENTLOG_WARNING_TYPE,     1, 5,  FALSE, 1, one_string },
    { eventlogname,    EVENTLOG_SUCCESS,          2, 6,  TRUE,  2, two_strings },
    { eventsources[0], EVENTLOG_AUDIT_FAILURE,    2, 7,  TRUE,  0, NULL },
    { eventsources[1], EVENTLOG_AUDIT_SUCCESS,    2, 8,  TRUE,  2, two_strings },
    { eventsources[2], EVENTLOG_WARNING_TYPE,     2, 9,  TRUE,  0, NULL },
    { eventsources[3], EVENTLOG_ERROR_TYPE,       2, 10, TRUE,  1, one_string }
};

static void test_readwrite(void)
{
    HANDLE handle;
    PSID user;
    DWORD sidsize, count;
    BOOL ret, sidavailable;
    BOOL on_vista = FALSE; /* Used to indicate Vista, W2K8 or Win7 */
    DWORD i;
    char *localcomputer = NULL;
    DWORD size;
    void* buf;

    if (pCreateWellKnownSid)
    {
        sidsize = SECURITY_MAX_SID_SIZE;
        user = malloc(sidsize);
        SetLastError(0xdeadbeef);
        pCreateWellKnownSid(WinInteractiveSid, NULL, user, &sidsize);
        sidavailable = TRUE;
    }
    else
    {
        win_skip("Skipping some SID related tests\n");
        sidavailable = FALSE;
        user = NULL;
    }

    /* Write an event with an incorrect event type. This will fail on Windows 7
     * but succeed on all others, hence it's not part of the struct.
     */
    handle = OpenEventLogA(NULL, eventlogname);
    if (!handle)
    {
        /* Intermittently seen on NT4 when tests are run immediately after boot */
        win_skip("Could not get a handle to the eventlog\n");
        goto cleanup;
    }

    count = 0xdeadbeef;
    GetNumberOfEventLogRecords(handle, &count);
    if (count != 0)
    {
        /* Needed for W2K3 without a service pack */
        win_skip("We most likely opened the Application eventlog\n");
        CloseEventLog(handle);
        Sleep(2000);

        handle = OpenEventLogA(NULL, eventlogname);
        ok(handle != NULL, "OpenEventLogA(%s) failed : %ld\n", eventlogname, GetLastError());
        count = 0xdeadbeef;
        GetNumberOfEventLogRecords(handle, &count);
        if (count != 0)
        {
            win_skip("We didn't open our new eventlog\n");
            CloseEventLog(handle);
            goto cleanup;
        }
    }

    SetLastError(0xdeadbeef);
    ret = ReportEventA(handle, 0x20, 0, 0, NULL, 0, 0, NULL, NULL);
    if (!ret && GetLastError() == ERROR_CRC)
    {
        win_skip("Win7 fails when using incorrect event types\n");
        ret = ReportEventA(handle, 0, 0, 0, NULL, 0, 0, NULL, NULL);
        ok(ret, "Expected success : %ld\n", GetLastError());
    }
    else
    {
        void *buf;
        DWORD read, needed = 0;
        EVENTLOGRECORD *record;

        ok(ret, "Expected success : %ld\n", GetLastError());

        /* Needed to catch earlier Vista (with no ServicePack for example) */
        buf = malloc(sizeof(EVENTLOGRECORD));
        if (!(ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                                  0, buf, sizeof(EVENTLOGRECORD), &read, &needed)) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            buf = realloc(buf, needed);
            ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                                0, buf, needed, &read, &needed);
        }
        if (ret)
        {
            record = (EVENTLOGRECORD *)buf;

            /* Vista and W2K8 return EVENTLOG_SUCCESS, Windows versions before return
             * the written eventtype (0x20 in this case).
             */
            if (record->EventType == EVENTLOG_SUCCESS)
                on_vista = TRUE;
        }
        free(buf);
    }

    /* This will clear the eventlog. The record numbering for new
     * events however differs on Vista SP1+. Before Vista the first
     * event would be numbered 1, on Vista SP1+ it's higher as we already
     * had at least one event (more in case of multiple test runs without
     * a reboot).
     */
    ClearEventLogA(handle, NULL);
    CloseEventLog(handle);

    /* Write a bunch of events while using different event sources */
    for (i = 0; i < ARRAY_SIZE(read_write); i++)
    {
        DWORD oldest;
        BOOL run_sidtests = read_write[i].evt_sid & sidavailable;

        winetest_push_context("%lu:%s", i, read_write[i].evt_src);

        /* We don't need to use RegisterEventSource to report events */
        if (i % 2)
            handle = OpenEventLogA(NULL, read_write[i].evt_src);
        else
            handle = RegisterEventSourceA(NULL, read_write[i].evt_src);
        ok(handle != NULL, "Expected a handle\n");

        SetLastError(0xdeadbeef);
        ret = ReportEventA(handle, read_write[i].evt_type, read_write[i].evt_cat,
                           read_write[i].evt_id, run_sidtests ? user : NULL,
                           read_write[i].evt_numstrings, 0, read_write[i].evt_strings, NULL);
        ok(ret, "Expected ReportEvent success : %ld\n", GetLastError());

        count = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = GetNumberOfEventLogRecords(handle, &count);
        ok(ret, "Expected GetNumberOfEventLogRecords success : %ld\n", GetLastError());
        todo_wine
        ok(count == (i + 1), "Expected %ld records, got %ld\n", i + 1, count);

        oldest = 0xdeadbeef;
        ret = GetOldestEventLogRecord(handle, &oldest);
        ok(ret, "Expected GetOldestEventLogRecord success : %ld\n", GetLastError());
        todo_wine
        ok(oldest == 1 ||
           (oldest > 1 && oldest != 0xdeadbeef), /* Vista SP1+, W2K8 and Win7 */
           "Expected oldest to be 1 or higher, got %ld\n", oldest);
        if (oldest > 1 && oldest != 0xdeadbeef)
            on_vista = TRUE;

        SetLastError(0xdeadbeef);
        if (i % 2)
            ret = CloseEventLog(handle);
        else
            ret = DeregisterEventSource(handle);
        ok(ret, "Expected success : %ld\n", GetLastError());
        winetest_pop_context();
    }

    handle = OpenEventLogA(NULL, eventlogname);
    ok(handle != NULL, "OpenEventLogA(%s) failed : %ld\n", eventlogname, GetLastError());
    count = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(handle, &count);
    ok(ret, "Expected success : %ld\n", GetLastError());
    todo_wine
    ok(count == i, "Expected %ld records, got %ld\n", i, count);
    CloseEventLog(handle);

    if (count == 0)
    {
        skip("No events were written to the eventlog\n");
        goto cleanup;
    }

    /* Report only once */
    if (on_vista)
        skip("There is no DWORD alignment enforced for UserSid on Vista, W2K8 or Win7\n");

    if (on_vista && pGetComputerNameExA)
    {
        /* New Vista+ behavior */
        size = 0;
        SetLastError(0xdeadbeef);
        pGetComputerNameExA(ComputerNameDnsFullyQualified, NULL, &size);
        localcomputer = malloc(size);
        pGetComputerNameExA(ComputerNameDnsFullyQualified, localcomputer, &size);
    }
    else
    {
        size = MAX_COMPUTERNAME_LENGTH + 1;
        localcomputer = malloc(size);
        GetComputerNameA(localcomputer, &size);
    }

    /* Read all events from our created eventlog, one by one */
    handle = OpenEventLogA(NULL, eventlogname);
    ok(handle != NULL, "OpenEventLogA(%s) failed : %ld\n", eventlogname, GetLastError());
    i = 0;
    size = sizeof(EVENTLOGRECORD) + 128;
    buf = malloc(size);
    for (;;)
    {
        DWORD read, needed;
        EVENTLOGRECORD *record;
        char *sourcename, *computername;
        int k;
        char *ptr;
        BOOL run_sidtests = read_write[i].evt_sid & sidavailable;

        winetest_push_context("%lu", i);

        SetLastError(0xdeadbeef);
        ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                            0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
        ok(!ret, "Expected failure\n");
        if (!ret && GetLastError() != ERROR_INSUFFICIENT_BUFFER)
        {
            ok(GetLastError() == ERROR_HANDLE_EOF, "record %ld, got %ld\n", i, GetLastError());
            winetest_pop_context();
            break;
        }

        if (needed > size)
        {
             free(buf);
             size = needed;
             buf = malloc(size);
        }
        ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                            0, buf, needed, &read, &needed);
        ok(ret, "Expected success: %ld\n", GetLastError());
        if (!ret)
        {
             winetest_pop_context();
             break;
        }
        record = (EVENTLOGRECORD *)buf;

        ok(record->Length == read,
           "Expected %ld, got %ld\n", read, record->Length);
        ok(record->Reserved == 0x654c664c,
           "Expected 0x654c664c, got %ld\n", record->Reserved);
        ok(record->RecordNumber == i + 1 ||
           (on_vista && (record->RecordNumber > i + 1)),
           "Expected %ld or higher, got %ld\n", i + 1, record->RecordNumber);
        ok(record->EventID == read_write[i].evt_id,
           "Expected %ld, got %ld\n", read_write[i].evt_id, record->EventID);
        ok(record->EventType == read_write[i].evt_type,
           "Expected %d, got %d\n", read_write[i].evt_type, record->EventType);
        ok(record->NumStrings == read_write[i].evt_numstrings,
           "Expected %d, got %d\n", read_write[i].evt_numstrings, record->NumStrings);
        ok(record->EventCategory == read_write[i].evt_cat,
           "Expected %d, got %d\n", read_write[i].evt_cat, record->EventCategory);

        sourcename = (char *)((BYTE *)buf + sizeof(EVENTLOGRECORD));
        ok(!lstrcmpA(sourcename, read_write[i].evt_src), "Expected '%s', got '%s'\n",
           read_write[i].evt_src, sourcename);

        computername = (char *)((BYTE *)buf + sizeof(EVENTLOGRECORD) + lstrlenA(sourcename) + 1);
        ok(!lstrcmpiA(computername, localcomputer), "Expected '%s', got '%s'\n",
           localcomputer, computername);

        /* Before Vista, UserSid was aligned on a DWORD boundary. Next to that if
         * no padding was actually required a 0 DWORD was still used for padding. No
         * application should be relying on the padding as we are working with offsets
         * anyway.
         */

        if (!on_vista)
        {
            DWORD calculated_sidoffset = sizeof(EVENTLOGRECORD) + lstrlenA(sourcename) + 1 + lstrlenA(computername) + 1;

            /* We are already DWORD aligned, there should still be some padding */
            if ((((UINT_PTR)buf + calculated_sidoffset) % sizeof(DWORD)) == 0)
                ok(*(DWORD *)((BYTE *)buf + calculated_sidoffset) == 0, "Expected 0\n");

            ok((((UINT_PTR)buf + record->UserSidOffset) % sizeof(DWORD)) == 0, "Expected DWORD alignment\n");
        }

        if (run_sidtests)
        {
            ok(record->UserSidLength == sidsize, "Expected %ld, got %ld\n", sidsize, record->UserSidLength);
        }
        else
        {
            ok(record->StringOffset == record->UserSidOffset, "Expected offsets to be the same\n");
            ok(record->UserSidLength == 0, "Expected 0, got %ld\n", record->UserSidLength);
        }

        ok(record->DataLength == 0, "Expected 0, got %ld\n", record->DataLength);

        ptr = (char *)((BYTE *)buf + record->StringOffset);
        for (k = 0; k < record->NumStrings; k++)
        {
            ok(!lstrcmpA(ptr, two_strings[k]), "Expected '%s', got '%s'\n", two_strings[k], ptr);
            ptr += lstrlenA(ptr) + 1;
        }

        ok(record->Length == *(DWORD *)((BYTE *)buf + record->Length - sizeof(DWORD)),
           "Expected the closing DWORD to contain the length of the record\n");

        winetest_pop_context();
        i++;
    }
    free(buf);
    CloseEventLog(handle);

    /* Test clearing a real eventlog */
    handle = OpenEventLogA(NULL, eventlogname);
    ok(handle != NULL, "OpenEventLogA(%s) failed : %ld\n", eventlogname, GetLastError());

    SetLastError(0xdeadbeef);
    ret = ClearEventLogA(handle, NULL);
    ok(ret, "Expected success : %ld\n", GetLastError());

    count = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(handle, &count);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(count == 0, "Expected an empty eventlog, got %ld records\n", count);

    CloseEventLog(handle);

cleanup:
    free(localcomputer);
    free(user);
}

/* Before Vista:
 *
 * Creating an eventlog on Windows (via the registry) automatically leads
 * to creation of a REG_MULTI_SZ named 'Sources'. This value lists all the
 * potential event sources for this eventlog. 'Sources' is automatically
 * updated when a new key (aka event source) is created.
 *
 * Although the updating of registry keys is almost instantaneously, we
 * check it after some other tests to assure we are not querying the
 * registry or file system to quickly.
 *
 * NT4 and higher:
 *
 * The eventlog file itself is also automatically created, even before we
 * start writing events.
 */
static char eventlogfile[MAX_PATH];
static void test_autocreation(void)
{
    HKEY key, eventkey;
    DWORD type, size;
    LONG ret;
    int i;
    char *p;
    char sources[sizeof(eventsources)];
    char sysdir[MAX_PATH];
    void *redir = 0;

    RegOpenKeyA(HKEY_LOCAL_MACHINE, eventlogsvc, &key);
    RegOpenKeyA(key, eventlogname, &eventkey);

    size = sizeof(sources);
    sources[0] = 0;
    ret = RegQueryValueExA(eventkey, "Sources", NULL, &type, (LPBYTE)sources, &size);
    if (ret == ERROR_SUCCESS)
    {
        char sources_verify[sizeof(eventsources)];

        ok(type == REG_MULTI_SZ, "Expected a REG_MULTI_SZ, got %ld\n", type);

        /* Build the expected string */
        memset(sources_verify, 0, sizeof(sources_verify));
        p = sources_verify;
        for (i = ARRAY_SIZE(eventsources); i > 0; i--)
        {
            lstrcpyA(p, eventsources[i - 1]);
            p += (lstrlenA(eventsources[i - 1]) + 1);
        }
        lstrcpyA(p, eventlogname);

        ok(!memcmp(sources, sources_verify, size),
           "Expected a correct 'Sources' value (size : %ld)\n", size);
    }

    RegCloseKey(eventkey);
    RegCloseKey(key);

    /* The directory that holds the eventlog files could be redirected */
    if (pWow64DisableWow64FsRedirection)
        pWow64DisableWow64FsRedirection(&redir);

    /* On Windows we also automatically get an eventlog file */
    GetSystemDirectoryA(sysdir, sizeof(sysdir));

    lstrcpyA(eventlogfile, sysdir);
    lstrcatA(eventlogfile, "\\winevt\\Logs\\");
    lstrcatA(eventlogfile, eventlogname);
    lstrcatA(eventlogfile, ".evtx");

    if (pWow64RevertWow64FsRedirection)
        pWow64RevertWow64FsRedirection(redir);
}

static void cleanup_eventlog(void)
{
    BOOL bret;
    LONG lret;
    HKEY key;
    DWORD i;
    char winesvc[MAX_PATH];

    /* Delete the registry tree */
    lstrcpyA(winesvc, eventlogsvc);
    lstrcatA(winesvc, "\\");
    lstrcatA(winesvc, eventlogname);

    RegOpenKeyA(HKEY_LOCAL_MACHINE, winesvc, &key);
    for (i = 0; i < ARRAY_SIZE(eventsources); i++)
        RegDeleteKeyA(key, eventsources[i]);
    RegDeleteValueA(key, "Sources");
    RegCloseKey(key);
    lret = RegDeleteKeyA(HKEY_LOCAL_MACHINE, winesvc);
    ok(lret == ERROR_SUCCESS, "Could not delete the registry tree : %ld\n", lret);

    /* A handle to the eventlog is locked by services.exe. We can only
     * delete the eventlog file after reboot.
     */
    bret = MoveFileExA(eventlogfile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    ok(bret, "Expected MoveFileEx to succeed: %ld\n", GetLastError());
}

static void test_trace_event_params(void)
{
    static const WCHAR emptyW[] = {0};
    static const GUID test_guid = {0x57696E65, 0x0000, 0x0000, {0x00,0x00, 0x00,0x00,0x00,0x00,0x00,0x01}};

    REGHANDLE reg_handle;
    ULONG uret;

    if (!pEventRegister)
    {
        win_skip("advapi32.EventRegister is missing, skipping trace event tests\n");
        return;
    }

    uret = pEventRegister(NULL, NULL, NULL, &reg_handle);
    todo_wine ok(uret == ERROR_INVALID_PARAMETER, "EventRegister gave wrong error: %#lx\n", uret);

    uret = pEventRegister(&test_guid, NULL, NULL, NULL);
    ok(uret == ERROR_INVALID_PARAMETER, "EventRegister gave wrong error: %#lx\n", uret);

    uret = pEventRegister(&test_guid, NULL, NULL, &reg_handle);
    ok(uret == ERROR_SUCCESS, "EventRegister gave wrong error: %#lx\n", uret);

    uret = pEventWriteString(0, 0, 0, emptyW);
    todo_wine ok(uret == ERROR_INVALID_HANDLE, "EventWriteString gave wrong error: %#lx\n", uret);

    uret = pEventWriteString(reg_handle, 0, 0, NULL);
    todo_wine ok(uret == ERROR_INVALID_PARAMETER, "EventWriteString gave wrong error: %#lx\n", uret);

    uret = pEventUnregister(0);
    todo_wine ok(uret == ERROR_INVALID_HANDLE, "EventUnregister gave wrong error: %#lx\n", uret);

    uret = pEventUnregister(reg_handle);
    ok(uret == ERROR_SUCCESS, "EventUnregister gave wrong error: %#lx\n", uret);
}

static void test_start_trace(void)
{
    const char sessionname[] = "wine";
    const char filepath[] = "wine.etl";
    const char filepath2[] = "eniw.etl";
    EVENT_TRACE_PROPERTIES *properties;
    TRACEHANDLE handle;
    LONG buffersize;
    LONG ret;

    buffersize = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(sessionname) + sizeof(filepath);
    properties = calloc(1, buffersize);
    properties->Wnode.BufferSize = buffersize;
    properties->Wnode.Flags = WNODE_FLAG_TRACED_GUID;
    properties->LogFileMode = EVENT_TRACE_FILE_MODE_NONE;
    properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);
    properties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(sessionname);
    strcpy((char *)properties + properties->LogFileNameOffset, filepath);

    properties->Wnode.BufferSize = 0;
    ret = StartTraceA(&handle, sessionname, properties);
    todo_wine
    ok(ret == ERROR_BAD_LENGTH ||
       ret == ERROR_INVALID_PARAMETER, /* XP and 2k3 */
       "Expected ERROR_BAD_LENGTH, got %ld\n", ret);
    properties->Wnode.BufferSize = buffersize;

    ret = StartTraceA(&handle, "this name is too long", properties);
    todo_wine
    ok(ret == ERROR_BAD_LENGTH, "Expected ERROR_BAD_LENGTH, got %ld\n", ret);

    ret = StartTraceA(&handle, sessionname, NULL);
    todo_wine
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);

    ret = StartTraceA(NULL, sessionname, properties);
    todo_wine
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);

    properties->LogFileNameOffset = 1;
    ret = StartTraceA(&handle, sessionname, properties);
    todo_wine
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);
    properties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(sessionname);

    properties->LoggerNameOffset = 1;
    ret = StartTraceA(&handle, sessionname, properties);
    todo_wine
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);
    properties->LoggerNameOffset = sizeof(EVENT_TRACE_PROPERTIES);

    properties->LogFileMode = EVENT_TRACE_FILE_MODE_SEQUENTIAL | EVENT_TRACE_FILE_MODE_CIRCULAR;
    ret = StartTraceA(&handle, sessionname, properties);
    todo_wine
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);
    properties->LogFileMode = EVENT_TRACE_FILE_MODE_NONE;
    /* XP creates a file we can't delete, so change the filepath to something else */
    strcpy((char *)properties + properties->LogFileNameOffset, filepath2);

    properties->Wnode.Guid = SystemTraceControlGuid;
    ret = StartTraceA(&handle, sessionname, properties);
    todo_wine
    ok(ret == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", ret);
    memset(&properties->Wnode.Guid, 0, sizeof(properties->Wnode.Guid));

    properties->LogFileNameOffset = 0;
    ret = StartTraceA(&handle, sessionname, properties);
    todo_wine
    ok(ret == ERROR_BAD_PATHNAME, "Expected ERROR_BAD_PATHNAME, got %ld\n", ret);
    properties->LogFileNameOffset = sizeof(EVENT_TRACE_PROPERTIES) + sizeof(sessionname);

    ret = StartTraceA(&handle, sessionname, properties);
    if (ret == ERROR_ACCESS_DENIED)
    {
        skip("need admin rights\n");
        goto done;
    }
    ok(ret == ERROR_SUCCESS, "Expected success, got %ld\n", ret);

    ret = StartTraceA(&handle, sessionname, properties);
    todo_wine
    ok(ret == ERROR_ALREADY_EXISTS ||
       ret == ERROR_SHARING_VIOLATION, /* 2k3 */
       "Expected ERROR_ALREADY_EXISTS, got %ld\n", ret);

    /* clean up */
    ControlTraceA(handle, sessionname, properties, EVENT_TRACE_CONTROL_STOP);
done:
    free(properties);
    DeleteFileA(filepath);
}

static BOOL read_record(HANDLE handle, DWORD flags, DWORD offset, EVENTLOGRECORD **record, DWORD *size)
{
    DWORD read, needed;
    BOOL ret;

    SetLastError(0xdeadbeef);
    memset(*record, 0, *size);
    needed = 0;
    if (!(ret = ReadEventLogW(handle, flags, offset, *record, *size, &read, &needed)) &&
        GetLastError() == ERROR_INSUFFICIENT_BUFFER)
    {
        free(*record);
        *record = malloc(needed);
        SetLastError(0xdeadbeef);
        memset(*record, 0, needed);
        *size = needed;
        ret = ReadEventLogW(handle, flags, offset, *record, *size, &read, &needed);
    }

    return ret;
}

static void test_eventlog_start(void)
{
    BOOL ret, found;
    HANDLE handle, handle2;
    EVENTLOGRECORD *record, *record2;
    DWORD size, size2, count, count2, read, needed;
    WCHAR *sourcename, *computername, *localcomputer;
    char *sourcenameA, *computernameA, *localcomputerA;

    /* ReadEventLogW */
    handle = OpenEventLogW(0, L"System");
    if (!handle && (GetLastError() == ERROR_ACCESS_DENIED || GetLastError() == RPC_S_SERVER_UNAVAILABLE))
    {
        win_skip( "Can't open System event log\n" );
        return;
    }
    ok(handle != NULL, "OpenEventLogW(System) failed : %ld\n", GetLastError());
    handle2 = OpenEventLogW(0, L"System");
    todo_wine ok(handle != handle2, "Expected different handle\n");
    CloseEventLog(handle2);

    handle2 = OpenEventLogW(0, L"SYSTEM");
    ok(!!handle2, "Expected valid handle\n");
    CloseEventLog(handle2);

    size = MAX_COMPUTERNAME_LENGTH + 1;
    localcomputer = malloc(size * sizeof(WCHAR));
    GetComputerNameW(localcomputer, &size);

    ret = TRUE;
    found = FALSE;
    while (!found && ret)
    {
        read = needed = 0;
        record = malloc(sizeof(EVENTLOGRECORD));
        if (!(ret = ReadEventLogW(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
                                  0, record, sizeof(EVENTLOGRECORD), &read, &needed)) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            record = realloc(record, needed);
            ret = ReadEventLogW(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
                                0, record, needed, &read, &needed);
            ok(needed == 0, "Expected 0, got %ld\n", needed);
        }
        if (ret && record->EventID == EVENT_EventlogStarted)
        {
            ok(record->Length == read, "Expected %ld, got %ld\n", read, record->Length);
            ok(record->Reserved == 0x654c664c, "Expected 0x654c664c, got %ld\n", record->Reserved);
            ok(record->RecordNumber > 0, "Expected 1 or higher, got %ld\n", record->RecordNumber);
            ok(record->TimeGenerated == record->TimeWritten, "Expected time values to be the same\n");
            ok(record->EventType == EVENTLOG_INFORMATION_TYPE,
                "Expected %d, got %d\n", EVENTLOG_INFORMATION_TYPE, record->EventType);
            ok(record->NumStrings == 0, "Expected 0, got %d\n", record->NumStrings);
            ok(record->EventCategory == 0, "Expected 0, got %d\n", record->EventCategory);
            ok(record->ReservedFlags == 0, "Expected 0, got %d\n", record->ReservedFlags);
            ok(record->ClosingRecordNumber == 0, "Expected 0, got %ld\n", record->ClosingRecordNumber);
            ok(record->StringOffset == record->UserSidOffset, "Expected offsets to be the same\n");
            ok(record->UserSidLength == 0, "Expected 0, got %ld\n", record->UserSidLength);
            ok(record->DataLength == 24, "Expected 24, got %ld\n", record->DataLength);
            ok(record->DataOffset == record->UserSidOffset, "Expected offsets to be the same\n");

            sourcename = (WCHAR *)(record + 1);
            ok(!lstrcmpW(sourcename, L"EventLog"),
                "Expected 'EventLog', got '%ls'\n", sourcename);

            computername = sourcename + sizeof("EventLog");
            ok(!lstrcmpiW(computername, localcomputer), "Expected '%ls', got '%ls'\n",
                localcomputer, computername);

            size = sizeof(EVENTLOGRECORD) + sizeof(L"EventLog") +
                (lstrlenW(computername) + 1) * sizeof(WCHAR);
            size = (size + 7) & ~7;
            ok(record->DataOffset == size ||
                broken(record->DataOffset == size - sizeof(WCHAR)), /* win8 */
                "Expected %ld, got %ld\n", size, record->DataOffset);

            found = TRUE;
        }
        free(record);
    }
    todo_wine ok(found, "EventlogStarted event not found\n");
    CloseEventLog(handle);
    free(localcomputer);

    /* ReadEventLogA */
    size = MAX_COMPUTERNAME_LENGTH + 1;
    localcomputerA = malloc(size);
    GetComputerNameA(localcomputerA, &size);

    handle = OpenEventLogA(0, "System");
    handle2 = OpenEventLogA(0, "SYSTEM");
    todo_wine ok(handle != handle2, "Expected different handle\n");
    CloseEventLog(handle2);

    ret = TRUE;
    found = FALSE;
    while (!found && ret)
    {
        read = needed = 0;
        record = malloc(sizeof(EVENTLOGRECORD));
        if (!(ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
                                  0, record, sizeof(EVENTLOGRECORD), &read, &needed)) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            record = realloc(record, needed);
            ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ,
                                0, record, needed, &read, &needed);
            ok(needed == 0, "Expected 0, got %ld\n", needed);
        }
        if (ret && record->EventID == EVENT_EventlogStarted)
        {
            ok(record->Length == read, "Expected %ld, got %ld\n", read, record->Length);
            ok(record->Reserved == 0x654c664c, "Expected 0x654c664c, got %ld\n", record->Reserved);
            ok(record->RecordNumber > 0, "Expected 1 or higher, got %ld\n", record->RecordNumber);
            ok(record->TimeGenerated == record->TimeWritten, "Expected time values to be the same\n");
            ok(record->EventType == EVENTLOG_INFORMATION_TYPE,
                "Expected %d, got %d\n", EVENTLOG_INFORMATION_TYPE, record->EventType);
            ok(record->NumStrings == 0, "Expected 0, got %d\n", record->NumStrings);
            ok(record->EventCategory == 0, "Expected 0, got %d\n", record->EventCategory);
            ok(record->ReservedFlags == 0, "Expected 0, got %d\n", record->ReservedFlags);
            ok(record->ClosingRecordNumber == 0, "Expected 0, got %ld\n", record->ClosingRecordNumber);
            ok(record->StringOffset == record->UserSidOffset, "Expected offsets to be the same\n");
            ok(record->UserSidLength == 0, "Expected 0, got %ld\n", record->UserSidLength);
            ok(record->DataLength == 24, "Expected 24, got %ld\n", record->DataLength);
            ok(record->DataOffset == record->UserSidOffset, "Expected offsets to be the same\n");

            sourcenameA = (char *)(record + 1);
            ok(!strcmp(sourcenameA, "EventLog"),
                "Expected 'EventLog', got '%s'\n", sourcenameA);

            computernameA = sourcenameA + sizeof("EventLog");
            ok(!_stricmp(computernameA, localcomputerA), "Expected '%s', got '%s'\n",
                localcomputerA, computernameA);

            size = sizeof(EVENTLOGRECORD) + sizeof("EventLog") + strlen(computernameA) + 1;
            size = (size + 7) & ~7;
            ok(record->DataOffset == size ||
                broken(record->DataOffset == size - 1), /* win8 */
                "Expected %ld, got %ld\n", size, record->DataOffset);

            found = TRUE;
        }
        free(record);
    }
    todo_wine ok(found, "EventlogStarted event not found\n");
    CloseEventLog(handle);
    free(localcomputerA);

    /* SEQUENTIAL | FORWARDS - dwRecordOffset is ignored */
    handle = OpenEventLogW(0, L"System");
    size = sizeof(EVENTLOGRECORD) + 128;
    record = malloc(size);
    todo_wine {
    ret = read_record(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 100, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 1, "Expected 1, got %lu\n", record->RecordNumber);
    ret = read_record(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 200, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 2, "Expected 2, got %lu\n", record->RecordNumber);

    /* change direction sequentially */
    ret = read_record(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ, 300, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 2, "Expected 2, got %lu\n", record->RecordNumber);
    ret = read_record(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ, 400, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 1, "Expected 1, got %lu\n", record->RecordNumber);
    }

    /* changing how is an error */
    SetLastError(0xdeadbeef);
    ret = read_record(handle, EVENTLOG_SEEK_READ | EVENTLOG_BACKWARDS_READ, 0, &record, &size);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());
    CloseEventLog(handle);

    /* SEQUENTIAL | BACKWARDS - dwRecordOffset is ignored */
    handle = OpenEventLogW(0, L"System");
    count = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(handle, &count);
    ok(ret, "Expected success : %ld\n", GetLastError());
    todo_wine
    ok(count, "Zero records in log\n");

    ret = read_record(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ, 100, &record, &size);
    todo_wine
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == count, "Expected %lu, got %lu\n", count, record->RecordNumber);
    ret = read_record(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ, 100, &record, &size);
    todo_wine {
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == count - 1, "Expected %lu, got %lu\n", count - 1, record->RecordNumber);
    }
    CloseEventLog(handle);

    handle = OpenEventLogW(0, L"System");
    /* SEEK | FORWARDS */
    /* bogus offset */
    ret = read_record(handle, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ, 0, &record, &size);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    count = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(handle, &count);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ret = read_record(handle, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ, count + 1, &record, &size);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    todo_wine {
    ret = read_record(handle, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ, 2, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 2, "Expected 2, got %lu\n", record->RecordNumber);
    /* skip one */
    ret = read_record(handle, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ, 4, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 4, "Expected 4, got %lu\n", record->RecordNumber);
    /* seek an earlier one */
    ret = read_record(handle, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ, 3, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 3, "Expected 3, got %lu\n", record->RecordNumber);
    /* change how */
    ret = read_record(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 100, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 4 || broken(record->RecordNumber == 5) /* some win10 22h2 */,
        "Expected 4, got %lu\n", record->RecordNumber);
    /* change direction */
    ret = read_record(handle, EVENTLOG_SEEK_READ | EVENTLOG_BACKWARDS_READ, 10, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 10, "Expected 10, got %lu\n", record->RecordNumber);
    }
    CloseEventLog(handle);

    /* SEEK | BACKWARDS */
    handle = OpenEventLogW(0, L"system");
    /* bogus offset */
    ret = read_record(handle, EVENTLOG_SEEK_READ | EVENTLOG_BACKWARDS_READ, 0, &record, &size);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %ld\n", GetLastError());

    todo_wine {
    ret = read_record(handle, EVENTLOG_SEEK_READ | EVENTLOG_BACKWARDS_READ, 5, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 5, "Expected 5, got %lu\n", record->RecordNumber);
    /* skip one */
    ret = read_record(handle, EVENTLOG_SEEK_READ | EVENTLOG_BACKWARDS_READ, 3, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 3, "Expected 3, got %lu\n", record->RecordNumber);
    /* seek a later one */
    ret = read_record(handle, EVENTLOG_SEEK_READ | EVENTLOG_BACKWARDS_READ, 4, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 4, "Expected 4, got %lu\n", record->RecordNumber);
    /* change how */
    ret = read_record(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_BACKWARDS_READ, 100, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 3 || broken(record->RecordNumber == 2) /* some win10 22h2 */,
        "Expected 3, got %lu\n", record->RecordNumber);
    /* change direction */
    ret = read_record(handle, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ, 10, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 10, "Expected 10, got %lu\n", record->RecordNumber);
    }
    CloseEventLog(handle);

    /* reading same log with different handles */
    handle = OpenEventLogW(0, L"System");
    handle2 = OpenEventLogW(0, L"SYSTEM");
    todo_wine {
    ret = read_record(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 1, "Expected 1, got %lu\n", record->RecordNumber);
    ret = read_record(handle2, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(record->RecordNumber == 1, "Expected 1, got %lu\n", record->RecordNumber);
    }
    CloseEventLog(handle2);
    CloseEventLog(handle);

    /* using source name */
    size2 = size;
    record2 = malloc(size2);
    handle = OpenEventLogW(0, L"System");
    handle2 = OpenEventLogW(0, L"EventLog");
    todo_wine {
    ret = read_record(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, &record, &size);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ret = read_record(handle2, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, &record2, &size2);
    ok(ret, "Expected success : %ld\n", GetLastError());
    }
    ok(size == size2, "Expected %lu, got %lu\n", size, size2);
    ok(!memcmp(record, record2, min(size, size2)), "Records miscompare\n");
    count = 0xdeadbeef;
    count2 = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(handle, &count);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ret = GetNumberOfEventLogRecords(handle2, &count2);
    ok(ret, "Expected success : %ld\n", GetLastError());
    ok(count == count2, "Expected %lu, got %lu\n", count, count2);
    CloseEventLog(handle2);
    CloseEventLog(handle);

    free(record2);
    free(record);
}

START_TEST(eventlog)
{
    SetLastError(0xdeadbeef);
    CloseEventLog(NULL);
    if (GetLastError() == ERROR_CALL_NOT_IMPLEMENTED)
    {
        win_skip("Event log functions are not implemented\n");
        return;
    }

    init_function_pointers();

    /* Parameters only */
    test_open_close();
    test_info();
    test_count();
    test_oldest();
    test_backup();
    test_openbackup();
    test_read();
    test_clear();
    test_trace_event_params();

    /* Functional tests */
    if (create_new_eventlog())
    {
        test_readwrite();
        test_autocreation();
        cleanup_eventlog();
    }

    /* Trace tests */
    test_start_trace();

    test_eventlog_start();
}
