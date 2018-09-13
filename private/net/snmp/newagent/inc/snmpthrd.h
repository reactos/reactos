/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    snmpthrd.h

Abstract:

    Contains definitions for master agent network thread.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/

#ifndef _SNMPTHRD_H_
#define _SNMPTHRD_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

LPSTR
CommunityOctetsToString(
    AsnOctetString  *pAsnCommunity,
    BOOL            bUnicode
    );

LPSTR
StaticUnicodeToString(
    LPWSTR wszUnicode
    );

DWORD
WINAPI
ProcessSnmpMessages(
    LPVOID lpParam
    );

#endif // _SNMPTHRD_H_

