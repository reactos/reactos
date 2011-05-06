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

#include <windef.h>
#include <winbase.h>

#include "wine/test.h"

static const char szmspath[] = "\\\\.\\mailslot\\wine_mailslot_test";

static int mailslot_test(void)
{
    HANDLE hSlot, hSlot2, hWriter, hWriter2;
    unsigned char buffer[16];
    DWORD count, dwMax, dwNext, dwMsgCount, dwTimeout;
    BOOL ret;

    /* sanity check on GetMailslotInfo */
    dwMax = dwNext = dwMsgCount = dwTimeout = 0;
    ok( !GetMailslotInfo( INVALID_HANDLE_VALUE, &dwMax, &dwNext,
            &dwMsgCount, &dwTimeout ), "getmailslotinfo succeeded\n");

    /* open a mailslot that doesn't exist */
    hWriter = CreateFile(szmspath, GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ok( hWriter == INVALID_HANDLE_VALUE, "nonexistent mailslot\n");

    /* open a mailslot without the right name */
    hSlot = CreateMailslot( "blah", 0, 0, NULL );
    ok( hSlot == INVALID_HANDLE_VALUE,
            "Created mailslot with invalid name\n");
    ok( GetLastError() == ERROR_INVALID_NAME,
            "error should be ERROR_INVALID_NAME\n");

    /* open a mailslot with a null name */
    hSlot = CreateMailslot( NULL, 0, 0, NULL );
    ok( hSlot == INVALID_HANDLE_VALUE || broken(hSlot != INVALID_HANDLE_VALUE), /* win9x */
        "Created mailslot with invalid name\n");
    if (hSlot == INVALID_HANDLE_VALUE)
        ok( GetLastError() == ERROR_PATH_NOT_FOUND,
            "error should be ERROR_PATH_NOT_FOUND\n");
    else  /* succeeds on win9x */
        CloseHandle( hSlot );

    /* valid open, but with wacky parameters ... then check them */
    hSlot = CreateMailslot( szmspath, -1, -1, NULL );
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
    hSlot = CreateMailslot( szmspath, 0, 0, NULL );
    ok( hSlot != INVALID_HANDLE_VALUE , "valid mailslot failed\n");

    /* try and read/write to it */
    count = 0;
    memset(buffer, 0, sizeof buffer);
    ret = ReadFile( hSlot, buffer, sizeof buffer, &count, NULL);
    ok( !ret || broken(ret), /* win9x */ "slot read\n");
    if (!ret) ok( GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %u\n", GetLastError() );
    else ok( count == 0, "wrong count %u\n", count );
    ok( !WriteFile( hSlot, buffer, sizeof buffer, &count, NULL),
            "slot write\n");
    ok( GetLastError() == ERROR_ACCESS_DENIED, "wrong error %u\n", GetLastError() );

    /* now try and open the client, but with the wrong sharing mode */
    hWriter = CreateFile(szmspath, GENERIC_WRITE,
                             0, NULL, OPEN_EXISTING, 0, NULL);
    ok( hWriter != INVALID_HANDLE_VALUE /* vista */ || GetLastError() == ERROR_SHARING_VIOLATION,
        "error should be ERROR_SHARING_VIOLATION got %p / %u\n", hWriter, GetLastError());
    if (hWriter != INVALID_HANDLE_VALUE) CloseHandle( hWriter );

    /* now open the client with the correct sharing mode */
    hWriter = CreateFile(szmspath, GENERIC_READ|GENERIC_WRITE,
                             FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    if (hWriter == INVALID_HANDLE_VALUE)  /* win9x doesn't like GENERIC_READ */
        hWriter = CreateFile(szmspath, GENERIC_WRITE, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    ok( hWriter != INVALID_HANDLE_VALUE, "existing mailslot err %u\n", GetLastError());

    /*
     * opening a client should make no difference to
     * whether we can read or write the mailslot
     */
    ret = ReadFile( hSlot, buffer, sizeof buffer/2, &count, NULL);
    ok( !ret || broken(ret), /* win9x */ "slot read\n");
    if (!ret) ok( GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %u\n", GetLastError() );
    else ok( count == 0, "wrong count %u\n", count );
    ok( !WriteFile( hSlot, buffer, sizeof buffer/2, &count, NULL),
            "slot write\n");
    ok( GetLastError() == ERROR_ACCESS_DENIED, "wrong error %u\n", GetLastError() );

    /*
     * we can't read from this client, 
     * but we should be able to write to it
     */
    ok( !ReadFile( hWriter, buffer, sizeof buffer/2, &count, NULL),
            "can read client\n");
    ok( GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ACCESS_DENIED,
            "wrong error %u\n", GetLastError() );
    ok( WriteFile( hWriter, buffer, sizeof buffer/2, &count, NULL),
            "can't write client\n");
    ok( !ReadFile( hWriter, buffer, sizeof buffer/2, &count, NULL),
            "can read client\n");
    ok( GetLastError() == ERROR_INVALID_PARAMETER || GetLastError() == ERROR_ACCESS_DENIED,
            "wrong error %u\n", GetLastError() );

    /*
     * seeing as there's something in the slot,
     * we should be able to read it once
     */
    ok( ReadFile( hSlot, buffer, sizeof buffer, &count, NULL),
            "slot read\n");
    ok( count == (sizeof buffer/2), "short read\n" );

    /* but not again */
    ret = ReadFile( hSlot, buffer, sizeof buffer, &count, NULL);
    ok( !ret || broken(ret), /* win9x */ "slot read\n");
    if (!ret) ok( GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %u\n", GetLastError() );
    else ok( count == 0, "wrong count %u\n", count );

    /* now try open another writer... should fail */
    hWriter2 = CreateFile(szmspath, GENERIC_READ|GENERIC_WRITE,
                     FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    /* succeeds on vista, don't test */
    if (hWriter2 != INVALID_HANDLE_VALUE) CloseHandle( hWriter2 );

    /* now try open another as a reader ... also fails */
    hWriter2 = CreateFile(szmspath, GENERIC_READ,
                     FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    /* succeeds on vista, don't test */
    if (hWriter2 != INVALID_HANDLE_VALUE) CloseHandle( hWriter2 );

    /* now try open another as a writer ... still fails */
    hWriter2 = CreateFile(szmspath, GENERIC_WRITE,
                     FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    /* succeeds on vista, don't test */
    if (hWriter2 != INVALID_HANDLE_VALUE) CloseHandle( hWriter2 );

    /* now open another one */
    hSlot2 = CreateMailslot( szmspath, 0, 0, NULL );
    ok( hSlot2 == INVALID_HANDLE_VALUE , "opened two mailslots\n");

    /* close the client again */
    ok( CloseHandle( hWriter ), "closing the client\n");

    /*
     * now try reopen it with slightly different permissions ...
     * shared writing
     */
    hWriter = CreateFile(szmspath, GENERIC_WRITE,
              FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, OPEN_EXISTING, 0, NULL);
    ok( hWriter != INVALID_HANDLE_VALUE, "sharing writer\n");

    /*
     * now try open another as a writer ...
     * but don't share with the first ... fail
     */
    hWriter2 = CreateFile(szmspath, GENERIC_WRITE,
                     FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
    /* succeeds on vista, don't test */
    if (hWriter2 != INVALID_HANDLE_VALUE) CloseHandle( hWriter2 );

    /* now try open another as a writer ... and share with the first */
    hWriter2 = CreateFile(szmspath, GENERIC_WRITE,
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
    ok( !ret || broken(ret), /* win9x */ "slot read\n");
    if (!ret) ok( GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %u\n", GetLastError() );
    else ok( count == 0, "wrong count %u\n", count );

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
    todo_wine {
    ok( dwMsgCount == 2, "dwMsgCount incorrect\n");
    }

    /* write a 3rd message with zero size */
    ok( WriteFile( hWriter2, buffer, 0, &count, NULL), "3rd write failed\n");

    /* check the mailslot info */
    dwNext = dwMsgCount = 0;
    ok( GetMailslotInfo( hSlot, NULL, &dwNext, &dwMsgCount, NULL ),
        "getmailslotinfo failed\n");
    ok( dwNext == 1, "dwNext incorrect\n");
    todo_wine
        ok( dwMsgCount == 3 || broken(dwMsgCount == 2), /* win9x */
            "dwMsgCount incorrect %u\n", dwMsgCount);

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
    todo_wine {
        ok( dwMsgCount == 2 || broken(dwMsgCount == 1), /* win9x */
            "dwMsgCount incorrect %u\n", dwMsgCount);
    }

    /* read the second message */
    ok( ReadFile( hSlot, buffer, sizeof buffer, &count, NULL),
        "2nd slot read failed\n");
    ok( count == 2, "failed to get 2nd message\n");
    ok( ( buffer[0] == 'b' ) && ( buffer[1] == 'c' ), "2nd message wrong\n");

    /* check the mailslot info */
    dwNext = dwMsgCount = 0;
    ok( GetMailslotInfo( hSlot, NULL, &dwNext, &dwMsgCount, NULL ),
        "getmailslotinfo failed\n");
    ok( dwNext == 0 || broken(dwNext == ~0u), /* win9x */ "dwNext incorrect %u\n", dwNext);
    todo_wine {
        ok( dwMsgCount == 1 || broken(dwMsgCount == 0), /* win9x */
            "dwMsgCount incorrect %u\n", dwMsgCount);
    }

    /* read the 3rd (zero length) message */
    todo_wine {
    ok( ReadFile( hSlot, buffer, sizeof buffer, &count, NULL),
        "3rd slot read failed\n");
    }
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
    ok( !ret || broken(ret), /* win9x */ "3rd slot read succeeded\n");
    if (!ret) ok( GetLastError() == ERROR_SEM_TIMEOUT, "wrong error %u\n", GetLastError() );
    else ok( count == 0, "wrong count %u\n", count );

    /* finally close the mailslot and its client */
    ok( CloseHandle( hWriter2 ), "closing 2nd client\n");
    ok( CloseHandle( hWriter ), "closing the client\n");
    ok( CloseHandle( hSlot ), "closing the mailslot\n");

    /* test timeouts */
    hSlot = CreateMailslot( szmspath, 0, 1000, NULL );
    ok( hSlot != INVALID_HANDLE_VALUE , "valid mailslot failed\n");
    count = 0;
    memset(buffer, 0, sizeof buffer);
    dwTimeout = GetTickCount();
    ok( !ReadFile( hSlot, buffer, sizeof buffer, &count, NULL), "slot read\n");
    ok( GetLastError() == ERROR_SEM_TIMEOUT || broken(GetLastError() == ERROR_ACCESS_DENIED), /* win9x */
        "wrong error %u\n", GetLastError() );
    dwTimeout = GetTickCount() - dwTimeout;
    ok( dwTimeout >= 990, "timeout too short %u\n", dwTimeout );
    ok( CloseHandle( hSlot ), "closing the mailslot\n");

    return 0;
}

START_TEST(mailslot)
{
    mailslot_test();
}
