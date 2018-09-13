/*++

Copyright (c) 1995 Microsoft Corporation

Module Name:

    rnr2ops.c

Abstract:

    This module contains support for the DNS RnR2 provider

Author:

    Arnold Miller (ArnoldM)  3-Jan-1996

Revision History:

--*/

#define UNICODE
#define _UNICODE

#include <winsockp.h>
#include <tchar.h>
#include <ws2spi.h>
#include "rnrdefs.h"
#include "svcguid.h"
#include <align.h>


GUID HostnameGuid = SVCID_HOSTNAME;
GUID AddressGuid = SVCID_INET_HOSTADDRBYINETSTRING;
GUID InetHostName = SVCID_INET_HOSTADDRBYNAME;
GUID IANAGuid = SVCID_INET_SERVICEBYNAME;
GUID AtmaGuid = SVCID_DNS_TYPE_ATMA;
GUID Ipv6Guid = SVCID_DNS_TYPE_AAAA;


DWORD
AllocateUnicodeString (
    IN     LPSTR   lpAnsi,
    IN OUT PWCHAR *lppUnicode
)
/*++

Routine Description:

   Allocate a Unicode String intialized with the Ansi one.
   Caller must free with FREE_HEAP().

Arguments:

   lpAnsi     - ANSI string that is used to init the Unicode string

   lppUnicode - address to receive pointer to Unicode string.

Return Value:

   NO_ERROR if successful. Win32 error otherwise.

--*/

{
    LPWSTR         UnicodeString;
    INT            err;
    DWORD          dwAnsiLen;

    *lppUnicode = NULL ;

    //
    // handle the trivial case
    //
    if (!lpAnsi)
    {
        return NO_ERROR ;
    }

    //
    // allocate the memory
    //
    dwAnsiLen = strlen(lpAnsi) + 1;

    UnicodeString = (LPWSTR)ALLOCATE_HEAP(dwAnsiLen * sizeof(WCHAR));

    if (!UnicodeString)
    {
        return ERROR_NOT_ENOUGH_MEMORY ;
    }

    //
    // convert it
    //

    err = MultiByteToWideChar(
                   CP_ACP,         // better by ANSI
                   0,
                   lpAnsi,
                   -1,             // it's NULL terminated
                   UnicodeString,
                   dwAnsiLen );    // # of wide characters available

    if (!err)
    {
        FREE_HEAP(UnicodeString) ;
        return (GetLastError());
    }

    *lppUnicode = UnicodeString;

    return NO_ERROR ;
}

