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

#include "windef.h"
#include "winbase.h"
#include "winerror.h"
#include "winnt.h"
#include "winreg.h"
#include "sddl.h"

#include "wine/test.h"

static BOOL (WINAPI *pCreateWellKnownSid)(WELL_KNOWN_SID_TYPE,PSID,PSID,DWORD*);
static BOOL (WINAPI *pGetEventLogInformation)(HANDLE,DWORD,LPVOID,DWORD,LPDWORD);

static BOOL (WINAPI *pGetComputerNameExA)(COMPUTER_NAME_FORMAT,LPSTR,LPDWORD);
static BOOL (WINAPI *pWow64DisableWow64FsRedirection)(PVOID *);
static BOOL (WINAPI *pWow64RevertWow64FsRedirection)(PVOID);

static void init_function_pointers(void)
{
    HMODULE hadvapi32 = GetModuleHandleA("advapi32.dll");
    HMODULE hkernel32 = GetModuleHandleA("kernel32.dll");

    pCreateWellKnownSid = (void*)GetProcAddress(hadvapi32, "CreateWellKnownSid");
    pGetEventLogInformation = (void*)GetProcAddress(hadvapi32, "GetEventLogInformation");

    pGetComputerNameExA = (void*)GetProcAddress(hkernel32, "GetComputerNameExA");
    pWow64DisableWow64FsRedirection = (void*)GetProcAddress(hkernel32, "Wow64DisableWow64FsRedirection");
    pWow64RevertWow64FsRedirection = (void*)GetProcAddress(hkernel32, "Wow64RevertWow64FsRedirection");
}

static BOOL create_backup(const char *filename)
{
    HANDLE handle;
    DWORD rc, attribs;

    DeleteFileA(filename);
    handle = OpenEventLogA(NULL, "Application");
    rc = BackupEventLogA(handle, filename);
    if (!rc && GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
    {
        skip("insufficient privileges to backup the eventlog\n");
        CloseEventLog(handle);
        return FALSE;
    }
    ok(rc, "BackupEventLogA failed, le=%u\n", GetLastError());
    CloseEventLog(handle);

    attribs = GetFileAttributesA(filename);
    todo_wine
    ok(attribs != INVALID_FILE_ATTRIBUTES, "Expected a backup file attribs=%#x le=%u\n", attribs, GetLastError());
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
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenEventLogA(NULL, NULL);
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenEventLogA("IDontExist", NULL);
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenEventLogA("IDontExist", "deadbeef");
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == RPC_S_SERVER_UNAVAILABLE ||
       GetLastError() == RPC_S_INVALID_NET_ADDR, /* Some Vista and Win7 */
       "Expected RPC_S_SERVER_UNAVAILABLE, got %d\n", GetLastError());

    /* This one opens the Application log */
    handle = OpenEventLogA(NULL, "deadbeef");
    ok(handle != NULL, "Expected a handle\n");
    ret = CloseEventLog(handle);
    ok(ret, "Expected success\n");
    /* Close a second time */
    SetLastError(0xdeadbeef);
    ret = CloseEventLog(handle);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    }

    /* Empty servername should be read as local server */
    handle = OpenEventLogA("", "Application");
    ok(handle != NULL, "Expected a handle\n");
    CloseEventLog(handle);

    handle = OpenEventLogA(NULL, "Application");
    ok(handle != NULL, "Expected a handle\n");
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
    ok(GetLastError() == ERROR_INVALID_LEVEL, "Expected ERROR_INVALID_LEVEL, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(NULL, EVENTLOG_FULL_INFO, NULL, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    handle = OpenEventLogA(NULL, "Application");

    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, NULL, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "Expected RPC_X_NULL_REF_POINTER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, NULL, 0, &needed);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "Expected RPC_X_NULL_REF_POINTER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, efi, 0, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == RPC_X_NULL_REF_POINTER, "Expected RPC_X_NULL_REF_POINTER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    needed = 0xdeadbeef;
    efi->dwFull = 0xdeadbeef;
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, efi, 0, &needed);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Expected ERROR_INSUFFICIENT_BUFFER, got %d\n", GetLastError());
    ok(needed == sizeof(EVENTLOG_FULL_INFORMATION), "Expected sizeof(EVENTLOG_FULL_INFORMATION), got %d\n", needed);
    ok(efi->dwFull == 0xdeadbeef, "Expected no change to the dwFull member\n");

    /* Not that we care, but on success last error is set to ERROR_IO_PENDING */
    efi->dwFull = 0xdeadbeef;
    needed = sizeof(buffer);
    ret = pGetEventLogInformation(handle, EVENTLOG_FULL_INFO, efi, needed, &needed);
    ok(ret, "Expected success\n");
    ok(needed == sizeof(EVENTLOG_FULL_INFORMATION), "Expected sizeof(EVENTLOG_FULL_INFORMATION), got %d\n", needed);
    ok(efi->dwFull == 0 || efi->dwFull == 1, "Expected 0 (not full) or 1 (full), got %d\n", efi->dwFull);

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
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    count = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(NULL, &count);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    ok(count == 0xdeadbeef, "Expected count to stay unchanged\n");

    handle = OpenEventLogA(NULL, "Application");

    SetLastError(0xdeadbeef);
    ret = GetNumberOfEventLogRecords(handle, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    count = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(handle, &count);
    ok(ret, "Expected success\n");
    ok(count != 0xdeadbeef, "Expected the number of records\n");

    CloseEventLog(handle);

    /* Make a backup eventlog to work with */
    if (create_backup(backup))
    {
        handle = OpenBackupEventLogA(NULL, backup);
        todo_wine
        ok(handle != NULL, "Expected a handle, le=%d\n", GetLastError());

        /* Does GetNumberOfEventLogRecords work with backup eventlogs? */
        count = 0xdeadbeef;
        ret = GetNumberOfEventLogRecords(handle, &count);
        todo_wine
        {
        ok(ret, "Expected success\n");
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
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    oldest = 0xdeadbeef;
    ret = GetOldestEventLogRecord(NULL, &oldest);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    ok(oldest == 0xdeadbeef, "Expected oldest to stay unchanged\n");

    handle = OpenEventLogA(NULL, "Application");

    SetLastError(0xdeadbeef);
    ret = GetOldestEventLogRecord(handle, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    oldest = 0xdeadbeef;
    ret = GetOldestEventLogRecord(handle, &oldest);
    ok(ret, "Expected success\n");
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
        ok(ret, "Expected success\n");
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
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = BackupEventLogA(NULL, backup);
    ok(!ret, "Expected failure\n");
    ok(GetFileAttributesA(backup) == INVALID_FILE_ATTRIBUTES, "Expected no backup file\n");

    handle = OpenEventLogA(NULL, "Application");

    SetLastError(0xdeadbeef);
    ret = BackupEventLogA(handle, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    ret = BackupEventLogA(handle, backup);
    if (!ret && GetLastError() == ERROR_PRIVILEGE_NOT_HELD)
    {
        skip("insufficient privileges for backup tests\n");
        CloseEventLog(handle);
        return;
    }
    ok(ret, "Expected success\n");
    todo_wine
    ok(GetFileAttributesA(backup) != INVALID_FILE_ATTRIBUTES, "Expected a backup file\n");

    /* Try to overwrite */
    SetLastError(0xdeadbeef);
    ret = BackupEventLogA(handle, backup);
    todo_wine
    {
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_ALREADY_EXISTS, "Expected ERROR_ALREADY_EXISTS, got %d\n", GetLastError());
    }

    CloseEventLog(handle);

    /* Can we make a backup of a backup? */
    handle = OpenBackupEventLogA(NULL, backup);
    todo_wine
    ok(handle != NULL, "Expected a handle\n");

    ret = BackupEventLogA(handle, backup2);
    todo_wine
    {
    ok(ret, "Expected success\n");
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
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    read = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, 0, 0, NULL, 0, &read, NULL);
    ok(!ret, "Expected failure\n");
    ok(read == 0xdeadbeef, "Expected 'read' parameter to remain unchanged\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, 0, 0, NULL, 0, NULL, &needed);
    ok(!ret, "Expected failure\n");
    ok(needed == 0xdeadbeef, "Expected 'needed' parameter to remain unchanged\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* 'read' and 'needed' are only filled when the needed buffer size is passed back or when the call succeeds */
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, 0, 0, NULL, 0, &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, NULL, 0, NULL, NULL);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, NULL, 0, &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    buf = NULL;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(EVENTLOGRECORD));
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(NULL, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    HeapFree(GetProcessHeap(), 0, buf);

    handle = OpenEventLogA(NULL, "Application");

    /* Show that we need the proper dwFlags with a (for the rest) proper call */
    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(EVENTLOGRECORD));

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, 0, 0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ, 0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEEK_READ, 0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEEK_READ | EVENTLOG_FORWARDS_READ | EVENTLOG_BACKWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEEK_READ | EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    todo_wine
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    HeapFree(GetProcessHeap(), 0, buf);

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
    buf = HeapAlloc(GetProcessHeap(), 0, sizeof(EVENTLOGRECORD));
    read = needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                        0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
    ok(!ret, "Expected failure\n");
    ok(read == 0, "Expected no bytes read\n");
    ok(needed > sizeof(EVENTLOGRECORD), "Expected the needed buffersize to be bigger than sizeof(EVENTLOGRECORD)\n");
    ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    /* Read the first record */
    toread = needed;
    buf = HeapReAlloc(GetProcessHeap(), 0, buf, toread);
    read = needed = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ, 0, buf, toread, &read, &needed);
    ok(ret, "Expected success\n");
    ok(read == toread ||
       broken(read < toread), /* NT4 wants a buffer size way bigger than just 1 record */
       "Expected the requested size to be read\n");
    ok(needed == 0, "Expected no extra bytes to be read\n");
    HeapFree(GetProcessHeap(), 0, buf);

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
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenBackupEventLogA(NULL, "idontexist.evt");
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == ERROR_FILE_NOT_FOUND, "Expected ERROR_FILE_NOT_FOUND, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenBackupEventLogA("IDontExist", NULL);
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == ERROR_INVALID_PARAMETER, "Expected ERROR_INVALID_PARAMETER, got %d\n", GetLastError());

    SetLastError(0xdeadbeef);
    handle = OpenBackupEventLogA("IDontExist", "idontexist.evt");
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == RPC_S_SERVER_UNAVAILABLE ||
       GetLastError() == RPC_S_INVALID_NET_ADDR, /* Some Vista and Win7 */
       "Expected RPC_S_SERVER_UNAVAILABLE, got %d\n", GetLastError());

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
           "Expected RPC_S_SERVER_UNAVAILABLE, got %d\n", GetLastError());

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
       GetLastError() == ERROR_EVENTLOG_FILE_CORRUPT, /* Vista and Win7 */
       "Expected ERROR_NOT_ENOUGH_MEMORY, got %d\n", GetLastError());
    CloseEventLog(handle);
    DeleteFileA(backup);

    file = CreateFileA(backup, GENERIC_WRITE, 0, NULL, CREATE_NEW, 0, NULL);
    WriteFile(file, text, sizeof(text), &written, NULL);
    CloseHandle(file);
    SetLastError(0xdeadbeef);
    handle = OpenBackupEventLogA(NULL, backup);
    ok(handle == NULL, "Didn't expect a handle\n");
    ok(GetLastError() == ERROR_EVENTLOG_FILE_CORRUPT, "Expected ERROR_EVENTLOG_FILE_CORRUPT, got %d\n", GetLastError());
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
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* Make a backup eventlog to work with */
    if (!create_backup(backup))
        return;

    SetLastError(0xdeadbeef);
    ret = ClearEventLogA(NULL, backup);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

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
       "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

    /* Show that ClearEventLog only works for real eventlogs. */
    SetLastError(0xdeadbeef);
    ret = ClearEventLogA(handle, backup2);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());
    ok(GetFileAttributesA(backup2) == INVALID_FILE_ATTRIBUTES, "Expected no backup file\n");

    SetLastError(0xdeadbeef);
    ret = ClearEventLogA(handle, NULL);
    ok(!ret, "Expected failure\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "Expected ERROR_INVALID_HANDLE, got %d\n", GetLastError());

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
    for (i = 0; i < sizeof(eventsources)/sizeof(eventsources[0]); i++)
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

    if (pCreateWellKnownSid)
    {
        sidsize = SECURITY_MAX_SID_SIZE;
        user = HeapAlloc(GetProcessHeap(), 0, sidsize);
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
        ok(ret, "Expected success : %d\n", GetLastError());
    }
    else
    {
        void *buf;
        DWORD read, needed = 0;
        EVENTLOGRECORD *record;

        ok(ret, "Expected success : %d\n", GetLastError());

        /* Needed to catch earlier Vista (with no ServicePack for example) */
        buf = HeapAlloc(GetProcessHeap(), 0, sizeof(EVENTLOGRECORD));
        if (!(ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                                  0, buf, sizeof(EVENTLOGRECORD), &read, &needed)) &&
            GetLastError() == ERROR_INSUFFICIENT_BUFFER)
        {
            buf = HeapReAlloc(GetProcessHeap(), 0, buf, needed);
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
        HeapFree(GetProcessHeap(), 0, buf);
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
    for (i = 0; i < sizeof(read_write)/sizeof(read_write[0]); i++)
    {
        DWORD oldest;
        BOOL run_sidtests = read_write[i].evt_sid & sidavailable;

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
        ok(ret, "Expected ReportEvent success : %d\n", GetLastError());

        count = 0xdeadbeef;
        SetLastError(0xdeadbeef);
        ret = GetNumberOfEventLogRecords(handle, &count);
        ok(ret, "Expected GetNumberOfEventLogRecords success : %d\n", GetLastError());
        todo_wine
        ok(count == (i + 1), "Expected %d records, got %d\n", i + 1, count);

        oldest = 0xdeadbeef;
        ret = GetOldestEventLogRecord(handle, &oldest);
        ok(ret, "Expected GetOldestEventLogRecord success : %d\n", GetLastError());
        todo_wine
        ok(oldest == 1 ||
           (oldest > 1 && oldest != 0xdeadbeef), /* Vista SP1+, W2K8 and Win7 */
           "Expected oldest to be 1 or higher, got %d\n", oldest);
        if (oldest > 1 && oldest != 0xdeadbeef)
            on_vista = TRUE;

        SetLastError(0xdeadbeef);
        if (i % 2)
            ret = CloseEventLog(handle);
        else
            ret = DeregisterEventSource(handle);
        ok(ret, "Expected success : %d\n", GetLastError());
    }

    handle = OpenEventLogA(NULL, eventlogname);
    count = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(handle, &count);
    ok(ret, "Expected success\n");
    todo_wine
    ok(count == i, "Expected %d records, got %d\n", i, count);
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
        localcomputer = HeapAlloc(GetProcessHeap(), 0, size);
        pGetComputerNameExA(ComputerNameDnsFullyQualified, localcomputer, &size);
    }
    else
    {
        size = MAX_COMPUTERNAME_LENGTH + 1;
        localcomputer = HeapAlloc(GetProcessHeap(), 0, size);
        GetComputerNameA(localcomputer, &size);
    }

    /* Read all events from our created eventlog, one by one */
    handle = OpenEventLogA(NULL, eventlogname);
    i = 0;
    for (;;)
    {
        void *buf;
        DWORD read, needed;
        EVENTLOGRECORD *record;
        char *sourcename, *computername;
        int k;
        char *ptr;
        BOOL run_sidtests = read_write[i].evt_sid & sidavailable;

        buf = HeapAlloc(GetProcessHeap(), 0, sizeof(EVENTLOGRECORD));
        SetLastError(0xdeadbeef);
        ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                            0, buf, sizeof(EVENTLOGRECORD), &read, &needed);
        if (!ret && GetLastError() == ERROR_HANDLE_EOF)
        {
            HeapFree(GetProcessHeap(), 0, buf);
            break;
        }
        ok(!ret, "Expected failure\n");
        ok(GetLastError() == ERROR_INSUFFICIENT_BUFFER,
           "Expected ERROR_INVALID_PARAMETER, got %d\n",GetLastError());

        buf = HeapReAlloc(GetProcessHeap(), 0, buf, needed);
        ret = ReadEventLogA(handle, EVENTLOG_SEQUENTIAL_READ | EVENTLOG_FORWARDS_READ,
                            0, buf, needed, &read, &needed);
        ok(ret, "Expected success: %d\n", GetLastError());

        record = (EVENTLOGRECORD *)buf;

        ok(record->Length == read,
           "Expected %d, got %d\n", read, record->Length);
        ok(record->Reserved == 0x654c664c,
           "Expected 0x654c664c, got %d\n", record->Reserved);
        ok(record->RecordNumber == i + 1 ||
           (on_vista && (record->RecordNumber > i + 1)),
           "Expected %d or higher, got %d\n", i + 1, record->RecordNumber);
        ok(record->EventID == read_write[i].evt_id,
           "Expected %d, got %d\n", read_write[i].evt_id, record->EventID);
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
            ok(record->UserSidLength == sidsize, "Expected %d, got %d\n", sidsize, record->UserSidLength);
        }
        else
        {
            ok(record->StringOffset == record->UserSidOffset, "Expected offsets to be the same\n");
            ok(record->UserSidLength == 0, "Expected 0, got %d\n", record->UserSidLength);
        }

        ok(record->DataLength == 0, "Expected 0, got %d\n", record->DataLength);

        ptr = (char *)((BYTE *)buf + record->StringOffset);
        for (k = 0; k < record->NumStrings; k++)
        {
            ok(!lstrcmpA(ptr, two_strings[k]), "Expected '%s', got '%s'\n", two_strings[k], ptr);
            ptr += lstrlenA(ptr) + 1;
        }

        ok(record->Length == *(DWORD *)((BYTE *)buf + record->Length - sizeof(DWORD)),
           "Expected the closing DWORD to contain the length of the record\n");

        HeapFree(GetProcessHeap(), 0, buf);
        i++;
    }
    CloseEventLog(handle);

    /* Test clearing a real eventlog */
    handle = OpenEventLogA(NULL, eventlogname);

    SetLastError(0xdeadbeef);
    ret = ClearEventLogA(handle, NULL);
    ok(ret, "Expected success\n");

    count = 0xdeadbeef;
    ret = GetNumberOfEventLogRecords(handle, &count);
    ok(ret, "Expected success\n");
    ok(count == 0, "Expected an empty eventlog, got %d records\n", count);

    CloseEventLog(handle);

cleanup:
    HeapFree(GetProcessHeap(), 0, localcomputer);
    HeapFree(GetProcessHeap(), 0, user);
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

        ok(type == REG_MULTI_SZ, "Expected a REG_MULTI_SZ, got %d\n", type);

        /* Build the expected string */
        memset(sources_verify, 0, sizeof(sources_verify));
        p = sources_verify;
        for (i = sizeof(eventsources)/sizeof(eventsources[0]); i > 0; i--)
        {
            lstrcpyA(p, eventsources[i - 1]);
            p += (lstrlenA(eventsources[i - 1]) + 1);
        }
        lstrcpyA(p, eventlogname);

        ok(!memcmp(sources, sources_verify, size),
           "Expected a correct 'Sources' value (size : %d)\n", size);
    }

    RegCloseKey(eventkey);
    RegCloseKey(key);

    /* The directory that holds the eventlog files could be redirected */
    if (pWow64DisableWow64FsRedirection)
        pWow64DisableWow64FsRedirection(&redir);

    /* On Windows we also automatically get an eventlog file */
    GetSystemDirectoryA(sysdir, sizeof(sysdir));

    /* NT4 - W2K3 */
    lstrcpyA(eventlogfile, sysdir);
    lstrcatA(eventlogfile, "\\config\\");
    lstrcatA(eventlogfile, eventlogname);
    lstrcatA(eventlogfile, ".evt");

    if (GetFileAttributesA(eventlogfile) == INVALID_FILE_ATTRIBUTES)
    {
        /* Vista+ */
        lstrcpyA(eventlogfile, sysdir);
        lstrcatA(eventlogfile, "\\winevt\\Logs\\");
        lstrcatA(eventlogfile, eventlogname);
        lstrcatA(eventlogfile, ".evtx");
    }

    todo_wine
    ok(GetFileAttributesA(eventlogfile) != INVALID_FILE_ATTRIBUTES,
       "Expected an eventlog file\n");

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
    for (i = 0; i < sizeof(eventsources)/sizeof(eventsources[0]); i++)
        RegDeleteKeyA(key, eventsources[i]);
    RegDeleteValueA(key, "Sources");
    RegCloseKey(key);
    lret = RegDeleteKeyA(HKEY_LOCAL_MACHINE, winesvc);
    ok(lret == ERROR_SUCCESS, "Could not delete the registry tree : %d\n", lret);

    /* A handle to the eventlog is locked by services.exe. We can only
     * delete the eventlog file after reboot.
     */
    bret = MoveFileExA(eventlogfile, NULL, MOVEFILE_DELAY_UNTIL_REBOOT);
    ok(bret, "Expected MoveFileEx to succeed: %d\n", GetLastError());
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

    /* Functional tests */
    if (create_new_eventlog())
    {
        test_readwrite();
        test_autocreation();
        cleanup_eventlog();
    }
}
