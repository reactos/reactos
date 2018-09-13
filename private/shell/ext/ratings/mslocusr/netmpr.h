/*++

Copyright (c) 1991-1995  Microsoft Corporation

Module Name:

    netmpr.h

Abstract:

    DDK WINNET Header File for WIN32

Environment:

    User Mode -Win32

Notes:


Revision History:

    20-Mar-1995     LenS
        Created.

--*/

#ifndef _INC_NETMPR_
#define _INC_NETMPR_


//
//  Authentication and Logon/Logoff.
//

#define LOGON_DONE              0x00000001
#define LOGON_PRIMARY           0x00000002
#define LOGON_MUST_VALIDATE     0x00000004

#define LOGOFF_PENDING  1
#define LOGOFF_COMMIT   2
#define LOGOFF_CANCEL   3


//
//  Password Cache.
//

#ifndef PCE_STRUCT_DEFINED
#define PCE_STRUCT_DEFINED

struct PASSWORD_CACHE_ENTRY {
    WORD cbEntry;               /* size of this entry in bytes, incl. pad */
    WORD cbResource;            /* size of resource name in bytes */
    WORD cbPassword;            /* size of password in bytes */
    BYTE iEntry;                /* index number of this entry, for MRU */
    BYTE nType;                 /* type of entry (see below) */
    char abResource[1];         /* resource name (may not be ASCIIZ at all) */
};

#define PCE_MEMORYONLY          0x01    /* for flags field when adding */

/*
    Typedef for the callback routine passed to the enumeration functions.
    It will be called once for each entry that matches the criteria
    requested.  It returns TRUE if it wants the enumeration to
    continue, FALSE to stop.
*/
typedef BOOL (FAR PASCAL *CACHECALLBACK)( struct PASSWORD_CACHE_ENTRY FAR *pce, DWORD dwRefData );

#endif  /* PCE_STRUCT_DEFINED */

DWORD APIENTRY
WNetCachePassword(
    LPSTR pbResource,
    WORD  cbResource,
    LPSTR pbPassword,
    WORD  cbPassword,
    BYTE  nType,
    UINT  fnFlags
    );

DWORD APIENTRY
WNetGetCachedPassword(
    LPSTR  pbResource,
    WORD   cbResource,
    LPSTR  pbPassword,
    LPWORD pcbPassword,
    BYTE   nType
    );

DWORD APIENTRY
WNetRemoveCachedPassword(
    LPSTR pbResource,
    WORD  cbResource,
    BYTE  nType
    );

DWORD APIENTRY
WNetEnumCachedPasswords(
    LPSTR pbPrefix,
    WORD  cbPrefix,
    BYTE  nType,
    CACHECALLBACK pfnCallback,
    DWORD dwRefData
    );

#endif
