/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    trapthrd.h

Abstract:

    Contains definitions for trap processing thread.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _TRAPTHRD_H_
#define _TRAPTHRD_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Header files                                                              //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "snmppdus.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
ProcessSubagentEvents(
    );

BOOL
GenerateTrap(
    PSNMP_PDU pPdu
    );

BOOL
GenerateColdStartTrap(
    );

BOOL
GenerateAuthenticationTrap(
    );

#endif // _TRAPTHRD_H_
