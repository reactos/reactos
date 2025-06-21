/*
 *  Mailslot regression test
 *
 *  Copyright 2003 Mike McCormack
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
#include <stdlib.h>
#include <stdio.h>

#include <ntstatus.h>
#define WIN32_NO_STATUS
#include <windef.h>
#include <winbase.h>
#include <winternl.h>
#include "wine/test.h"
#ifdef __REACTOS__
#define RTL_CONSTANT_STRING(s) { sizeof(s) - sizeof(s[0]), sizeof(s), (void*)s }
#endif

static const char szmspath[] = "\\\\.\\mailslot\\wine_mailslot_test";

static int mailslot_test(void)
{
    UNICODE_STRING nt_path = RTL_CONSTANT_STRING( L"\\??\\MAILSLOT\\wine_mailslot_test" );
    HANDLE hSlot, hSlot2, hWriter, hWriter2;
    unsigned char buffer[16];
    DWORD count, dwMax, dwNext, dwMsgCount, dwTimeout;
    FILE_MAILSLOT_QUERY_INFORMATION info;
    OBJECT_ATTRIBUTES attr;
    IO_STATUS_BLOCK io;
    BOOL ret;

    /* sanity check on GetMailslotInfo */
    dwMax = dwNext = dwMsgCount = dwTimeout = 0;
    ok( !GetMailslotInfo( INVALID_HANDLE_VALUE, &dwMax, &dwNext,
            &dwMsgCount, &dwTimeout ), "getmailslotinfo succeeded\n");

    /* open a mailslot that doesn't exist */
    hWriter = CreateFileA(szmspath, GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ok( hWriter == INVALID_HANDLE_VALUE, "nonexistent mailslot\n");

    /* open a mailslot without the right name */
    hSlot = CreateMailslotA( "blah", 0, 0, NULL );
    ok( hSlot == INVALID_HANDLE_VALUE,
            "Created mailslot with invalid name\n");
    ok( GetLastError() == ERROR_INVALID_NAME,
            "error should be ERROR_INVALID_NAME\n");

    /* open a mailslot with a null name */
    hSlot = CreateMailslotA( NULL, 0, 0, NULL );
    ok( hSlot == INVALID_HANDLE_VALUE, "Created mailslot with invalid name\n");
    ok( GetLastError() == ERROR_PATH_NOT_FOUND, "error should be ERROR_PATH_NOT_FOUND\n");

    /* valid open, but with wacky parameters ... then check them */
    hSlot = CreateMailslotA( szmspath, -1, -1, NULL );
    ok( hSlot != INVALID_HANDLE_VALUE , "mailslot with valid name failed\n");
    dwMax = dwNext = dwMsgCount = dwTimeout = 0;
    ok( GetMailslotInfo( hSlot, &dwMax, &dwNext, &dwMsgCount, &dwTimeout ),
           "getmailslotinfo failed\n");
    ok( dwMax == ~0U, "dwMax incorrect\n");
    ok( dwNext == MAILSLOT_NO_MESSAGE, "dwNext incorrect\n");
    ok( dwMsgCount == 0, "dwMsgCount incorrect\n");
    ok( dwTimeout == ~0U, "dwTimeout incorrect\n");
    ok( GetMailslotInfo( hSlot, NULL, NULL, NULL, NULL ),
            "getmailslotinfo failed\n");
    ok( CloseHandle(hSlot), "failed to close mailslot\n");

    /* now open it for real */
    hSlot = CreateMailslotA( szmspath, 0, 0, NULL );
    ok( hSlot != INVALID_HANDLE_VALUE , "valid mailslot failed\n");

    /* try and read/write to it */
    count = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(INVALID_HANDLE_VALUE, buffer, 0, &count, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_INVALID_HANDLE, "wrong error %lu\n", GetLastError());
    ok(count == 0, "expected 0, got %lu\n", count);

    count = 0xdeadbeef;
    SetLastError(0xdeadbeef);
    ret = ReadFile(hSlot, buffer, 0, &count, NULL);
    ok(!ret, "ReadFile should fail\n");
    ok(GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %lu\n", GetLastError());
    ok(count == 0, "expected 0, got %lu\n", count);

    count = 0;
    memset(buffer, 0, sizeof buffer);
    ret = ReadFile( hSlot, buffer, sizeof buffer, &count, NULL);
    ok( !ret, "slot read\n");
    if (!ret) ok( GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %lu\n", GetLastError() );
    else ok( count == 0, "wrong count %lu\n", count );
    ok( !WriteFile( hSlot, buffer, sizeof buffer, &count, NULL),
            "slot write\n");
    ok( GetLastError() == ERROR_ACCESS_DENIED, "wrong error %lu\n", GetLastError() );

    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    ret = NtReadFile( hSlot, NULL, NULL, NULL, &io, buffer, sizeof(buffer), NULL, NULL );
    ok( ret == STATUS_IO_TIMEOUT, "got %#x\n", ret );
    ok( io.Status == 0xdeadbeef, "got status %#lx\n", io.Status );
    ok( io.Information == 0xdeadbeef, "got size %Iu\n", io.Information );

    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    ret = NtWriteFile( hSlot, NULL, NULL, NULL, &io, buffer, sizeof(buffer), NULL, NULL );
    ok( ret == STATUS_ACCESS_DENIED, "got %#x\n", ret );
    ok( io.Status == 0xdeadbeef, "got status %#lx\n", io.Status );
    ok( io.Information == 0xdeadbeef, "got size %Iu\n", io.Information );

    /* now try and open the client, but with the wrong sharing mode */
    hWriter = CreateFileA(szmspath, GENERIC_WRITE,
                             0, NULL, OPEN_EXISTING, 0, NULL);
    ok( hWriter != INVALID_HANDLE_VALUE /* vista */ || GetLastError() == ERROR_SHARING_VIOLATION,
        "error should be ERROR_SHARING_VIOLATION got %p / %lu\n", hWriter, GetLastError());
    if (hWriter != INVALID_HANDLE_VALUE) CloseHandle( hWriter );

    /* now open the client with the correct sharing mode */
    hWriter = CreateFileA(szmspath, GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ok( hWriter != INVALID_HANDLE_VALUE, "existing mailslot err %lu\n", GetLastError());

    /*
     * opening a client should make no difference to
     * whether we can read or write the mailslot
     */
    ret = ReadFile( hSlot, buffer, sizeof buffer/2, &count, NULL);
    ok( !ret, "slot read\n");
    if (!ret) ok( GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %lu\n", GetLastError() );
    else ok( count == 0, "wrong count %lu\n", count );
    ok( !WriteFile( hSlot, buffer, sizeof buffer/2, &count, NULL),
            "slot write\n");
    ok( GetLastError() == ERROR_ACCESS_DENIED, "wrong error %lu\n", GetLastError() );

    /*
     * we can't read from this client, 
     * but we should be able to write to it
     */
    ok( !ReadFile( hWriter, buffer, sizeof buffer/2, &count, NULL),
            "can read client\n");
    ok( GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ACCESS_DENIED,
            "wrong error %lu\n", GetLastError() );
    ok( WriteFile( hWriter, buffer, sizeof buffer/2, &count, NULL),
            "can't write client\n");
    ok( !ReadFile( hWriter, buffer, sizeof buffer/2, &count, NULL),
            "can read client\n");
    ok( GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ACCESS_DENIED,
            "wrong error %lu\n", GetLastError() );

    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    ret = NtReadFile( hWriter, NULL, NULL, NULL, &io, buffer, sizeof(buffer), NULL, NULL );
    todo_wine ok( ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret );
    ok( io.Status == 0xdeadbeef, "got status %#lx\n", io.Status );
    ok( io.Information == 0xdeadbeef, "got size %Iu\n", io.Information );

    /*
     * seeing as there's something in the slot,
     * we should be able to read it once
     */
    ok( ReadFile( hSlot, buffer, sizeof buffer, &count, NULL),
            "slot read\n");
    ok( count == (sizeof buffer/2), "short read\n" );

    /* but not again */
    ret = ReadFile( hSlot, buffer, sizeof buffer, &count, NULL);
    ok( !ret, "slot read\n");
    if (!ret) ok( GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %lu\n", GetLastError() );
    else ok( count == 0, "wrong count %lu\n", count );

    /* now try open another writer... should fail */
    hWriter2 = CreateFileA(szmspath, GENERIC_READ|GENERIC_WRITE,
                     FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    /* succeeds on vista, don't test */
    if (hWriter2 != INVALID_HANDLE_VALUE) CloseHandle( hWriter2 );

    /* now try open another as a reader ... also fails */
    hWriter2 = CreateFileA(szmspath, GENERIC_READ,
                     FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    /* succeeds on vista, don't test */
    if (hWriter2 != INVALID_HANDLE_VALUE) CloseHandle( hWriter2 );

    /* now try open another as a writer ... still fails */
    hWriter2 = CreateFileA(szmspath, GENERIC_WRITE,
                     FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    /* succeeds on vista, don't test */
    if (hWriter2 != INVALID_HANDLE_VALUE) CloseHandle( hWriter2 );

    /* now open another one */
    hSlot2 = CreateMailslotA( szmspath, 0, 0, NULL );
    ok( hSlot2 == INVALID_HANDLE_VALUE , "opened two mailslots\n");

    /* close the client again */
    ok( CloseHandle( hWriter ), "closing the client\n");

    /*
     * now try reopen it with slightly different permissions ...
     * shared writing
     */
    hWriter = CreateFileA(szmspath, GENERIC_WRITE,
              FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    ok( hWriter != INVALID_HANDLE_VALUE, "sharing writer\n");

    /*
     * now try open another as a writer ...
     * but don't share with the first ... fail
     */
    hWriter2 = CreateFileA(szmspath, GENERIC_WRITE,
                     FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    /* succeeds on vista, don't test */
    if (hWriter2 != INVALID_HANDLE_VALUE) CloseHandle( hWriter2 );

    /* now try open another as a writer ... and share with the first */
    hWriter2 = CreateFileA(szmspath, GENERIC_WRITE,
              FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    ok( hWriter2 != INVALID_HANDLE_VALUE, "2nd sharing writer\n");

    /* check the mailslot info */
    dwMax = dwNext = dwMsgCount = dwTimeout = 0;
    ok( GetMailslotInfo( hSlot, &dwMax, &dwNext, &dwMsgCount, &dwTimeout ),
        "getmailslotinfo failed\n");
    ok( dwNext == MAILSLOT_NO_MESSAGE, "dwNext incorrect\n");
    ok( dwMax == 0, "dwMax incorrect\n");
    ok( dwMsgCount == 0, "dwMsgCount incorrect\n");
    ok( dwTimeout == 0, "dwTimeout incorrect\n");

    /* check there's still no data */
    ret = ReadFile( hSlot, buffer, sizeof buffer, &count, NULL);
    ok( !ret, "slot read\n");
    if (!ret) ok( GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %lu\n", GetLastError() );
    else ok( count == 0, "wrong count %lu\n", count );

    /* write two messages */
    buffer[0] = 'a';
    ok( WriteFile( hWriter, buffer, 1, &count, NULL), "1st write failed\n");

    /* check the mailslot info */
    dwNext = dwMsgCount = 0;
    ok( GetMailslotInfo( hSlot, NULL, &dwNext, &dwMsgCount, NULL ),
        "getmailslotinfo failed\n");
    ok( dwNext == 1, "dwNext incorrect\n");
    ok( dwMsgCount == 1, "dwMsgCount incorrect\n");

    buffer[0] = 'b';
    buffer[1] = 'c';
    ok( WriteFile( hWriter2, buffer, 2, &count, NULL), "2nd write failed\n");

    /* check the mailslot info */
    dwNext = dwMsgCount = 0;
    ok( GetMailslotInfo( hSlot, NULL, &dwNext, &dwMsgCount, NULL ),
        "getmailslotinfo failed\n");
    ok( dwNext == 1, "dwNext incorrect\n");
    ok( dwMsgCount == 2, "dwMsgCount incorrect\n");

    /* write a 3rd message with zero size */
    ok( WriteFile( hWriter2, buffer, 0, &count, NULL), "3rd write failed\n");

    /* check the mailslot info */
    dwNext = dwMsgCount = 0;
    ok( GetMailslotInfo( hSlot, NULL, &dwNext, &dwMsgCount, NULL ),
        "getmailslotinfo failed\n");
    ok( dwNext == 1, "dwNext incorrect\n");
    ok( dwMsgCount == 3, "dwMsgCount incorrect %lu\n", dwMsgCount);

    buffer[0]=buffer[1]=0;

    /*
     * then check that they come out with the correct order and size,
     * then the slot is empty
     */
    ok( ReadFile( hSlot, buffer, sizeof buffer, &count, NULL),
        "1st slot read failed\n");
    ok( count == 1, "failed to get 1st message\n");
    ok( buffer[0] == 'a', "1st message wrong\n");

    /* check the mailslot info */
    dwNext = dwMsgCount = 0;
    ok( GetMailslotInfo( hSlot, NULL, &dwNext, &dwMsgCount, NULL ),
        "getmailslotinfo failed\n");
    ok( dwNext == 2, "dwNext incorrect\n");
    ok( dwMsgCount == 2, "dwMsgCount incorrect %lu\n", dwMsgCount);

    /* read the second message */
    ok( ReadFile( hSlot, buffer, sizeof buffer, &count, NULL),
        "2nd slot read failed\n");
    ok( count == 2, "failed to get 2nd message\n");
    ok( ( buffer[0] == 'b' ) && ( buffer[1] == 'c' ), "2nd message wrong\n");

    /* check the mailslot info */
    dwNext = dwMsgCount = 0;
    ok( GetMailslotInfo( hSlot, NULL, &dwNext, &dwMsgCount, NULL ),
        "getmailslotinfo failed\n");
    ok( dwNext == 0, "dwNext incorrect %lu\n", dwNext);
    ok( dwMsgCount == 1, "dwMsgCount incorrect %lu\n", dwMsgCount);

    /* read the 3rd (zero length) message */
    ok( ReadFile( hSlot, buffer, sizeof buffer, &count, NULL),
        "3rd slot read failed\n");
    ok( count == 0, "failed to get 3rd message\n");

    /*
     * now there should be no more messages
     * check the mailslot info
     */
    dwNext = dwMsgCount = 0;
    ok( GetMailslotInfo( hSlot, NULL, &dwNext, &dwMsgCount, NULL ),
        "getmailslotinfo failed\n");
    ok( dwNext == MAILSLOT_NO_MESSAGE, "dwNext incorrect\n");
    ok( dwMsgCount == 0, "dwMsgCount incorrect\n");

    /* check that reads fail */
    ret = ReadFile( hSlot, buffer, sizeof buffer, &count, NULL);
    ok( !ret, "3rd slot read succeeded\n");
    if (!ret) ok( GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %lu\n", GetLastError() );
    else ok( count == 0, "wrong count %lu\n", count );

    /* Try to perform a partial read. */
    count = 0;
    ret = WriteFile( hWriter, buffer, 2, &count, NULL );
    ok( ret, "got %d\n", ret );
    ok( count == 2, "got count %lu\n", count );

    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    ret = NtReadFile( hSlot, NULL, NULL, NULL, &io, buffer, 1, NULL, NULL );
    ok( ret == STATUS_BUFFER_TOO_SMALL, "got %#x\n", ret );
    ok( io.Status == 0xdeadbeef, "got status %#lx\n", io.Status );
    ok( io.Information == 0xdeadbeef, "got size %Iu\n", io.Information );

    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    memset( &info, 0xcc, sizeof(info) );
    ret = NtQueryInformationFile( hSlot, &io, &info, sizeof(info), FileMailslotQueryInformation );
    ok( ret == STATUS_SUCCESS, "got %#x\n", ret );
    ok( io.Status == STATUS_SUCCESS, "got status %#lx\n", io.Status );
    ok( io.Information == sizeof(info), "got size %Iu\n", io.Information );
    ok( !info.MaximumMessageSize, "got maximum size %lu\n", info.MaximumMessageSize );
    ok( !info.MailslotQuota, "got quota %lu\n", info.MailslotQuota );
    ok( info.NextMessageSize == 2, "got next size %lu\n", info.NextMessageSize );
    ok( info.MessagesAvailable == 1, "got message count %lu\n", info.MessagesAvailable );
    ok( !info.ReadTimeout.QuadPart, "got timeout %I64u\n", info.ReadTimeout.QuadPart );

    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    memset( &info, 0xcc, sizeof(info) );
    ret = NtQueryInformationFile( hWriter, &io, &info, sizeof(info), FileMailslotQueryInformation );
    todo_wine ok( ret == STATUS_SUCCESS || ret == STATUS_INVALID_PARAMETER /* Win < 8 */, "got %#x\n", ret );
    if (ret == STATUS_SUCCESS)
    {
        ok( io.Status == STATUS_SUCCESS, "got status %#lx\n", io.Status );
        ok( io.Information == sizeof(info), "got size %Iu\n", io.Information );
        ok( !info.MaximumMessageSize, "got maximum size %lu\n", info.MaximumMessageSize );
        ok( !info.MailslotQuota, "got quota %lu\n", info.MailslotQuota );
        ok( info.NextMessageSize == 2, "got next size %lu\n", info.NextMessageSize );
        ok( info.MessagesAvailable == 1, "got message count %lu\n", info.MessagesAvailable );
        ok( !info.ReadTimeout.QuadPart, "got timeout %I64u\n", info.ReadTimeout.QuadPart );
    }

    /* finally close the mailslot and its client */
    ok( CloseHandle( hWriter2 ), "closing 2nd client\n");
    ok( CloseHandle( hWriter ), "closing the client\n");
    ok( CloseHandle( hSlot ), "closing the mailslot\n");

    /* test timeouts */
    hSlot = CreateMailslotA( szmspath, 0, 1000, NULL );
    ok( hSlot != INVALID_HANDLE_VALUE , "valid mailslot failed\n");
    count = 0;
    memset(buffer, 0, sizeof buffer);
    dwTimeout = GetTickCount();
    ok( !ReadFile( hSlot, buffer, sizeof buffer, &count, NULL), "slot read\n");
    ok( GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %lu\n", GetLastError() );
    dwTimeout = GetTickCount() - dwTimeout;
    ok( dwTimeout >= 900, "timeout too short %lu\n", dwTimeout );
    ok( CloseHandle( hSlot ), "closing the mailslot\n");

    InitializeObjectAttributes( &attr, &nt_path, 0, NULL, NULL );
    ret = NtCreateMailslotFile( &hSlot, GENERIC_READ | GENERIC_WRITE | SYNCHRONIZE, &attr, &io, 0, 0, 0, NULL );
    ok( !ret, "got %#x\n", ret );

    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    ret = NtWriteFile( hSlot, NULL, NULL, NULL, &io, buffer, sizeof(buffer), NULL, NULL );
    todo_wine ok( ret == STATUS_INVALID_PARAMETER, "got %#x\n", ret );
    ok( io.Status == 0xdeadbeef, "got status %#lx\n", io.Status );
    ok( io.Information == 0xdeadbeef, "got size %Iu\n", io.Information );

    io.Status = 0xdeadbeef;
    io.Information = 0xdeadbeef;
    ret = NtReadFile( hSlot, NULL, NULL, NULL, &io, buffer, sizeof(buffer), NULL, NULL );
    ok( ret == STATUS_PENDING, "got %#x\n", ret );
    ok( io.Status == 0xdeadbeef, "got status %#lx\n", io.Status );
    ok( io.Information == 0xdeadbeef, "got size %Iu\n", io.Information );

    ret = WaitForSingleObject( hSlot, 0 );
    ok( ret == WAIT_TIMEOUT, "got %d\n", ret );

    hWriter = CreateFileA( szmspath, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL );
    ok( hWriter != INVALID_HANDLE_VALUE, "got err %lu\n", GetLastError() );

    ret = WriteFile( hWriter, "data", 4, &count, NULL );
    ok( ret == TRUE, "got error %lu\n", GetLastError() );

    ret = WaitForSingleObject( hSlot, 1000 );
    ok( !ret, "got %d\n", ret );
    ok( !io.Status, "got status %#lx\n", io.Status );
    ok( io.Information == 4, "got size %Iu\n", io.Information );

    CloseHandle( hWriter );
    CloseHandle( hSlot );

    return 0;
}

START_TEST(mailslot)
{
    mailslot_test();
}
