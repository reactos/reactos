/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    mem.h

Abstract:

    Contains memory allocation routines for SNMP master agent.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _MEM_H_
#define _MEM_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AgentHeapCreate(
    );

BOOL
AgentHeapDestroy(
    );

LPVOID
AgentMemAlloc(
    UINT nBytes
    );

VOID
AgentMemFree(
    LPVOID pMem
    );

#endif // _MEM_H_
