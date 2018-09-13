/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    startup.h

Abstract:

    Contains definitions for starting SNMP master agent.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _STARTUP_H_
#define _STARTUP_H_

extern HANDLE g_hAgentThread;
extern HANDLE g_hRegistryThread;

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
StartupAgent(
    );

BOOL
ShutdownAgent(
    );

#endif // _STARTUP_H_
