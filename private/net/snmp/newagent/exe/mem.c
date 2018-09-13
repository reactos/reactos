/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    mem.c

Abstract:

    Contains memory allocation routines for SNMP master agent.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Include files                                                             //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#include "globals.h"
#include "mem.h"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Private definitions                                                       //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

#define AGENT_HEAP_FLAGS            0
#define AGENT_HEAP_INITIAL_SIZE     0xffff
#define AGENT_HEAP_MAXIMUM_SIZE     0


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

HANDLE g_hAgentHeap = NULL;


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public procudures                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
AgentHeapCreate(
    )

/*++

Routine Description:

    Creates private heap for master agent private structures.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    // create master agent heap
    g_hAgentHeap = HeapCreate(
                        AGENT_HEAP_FLAGS, 
                        AGENT_HEAP_INITIAL_SIZE, 
                        AGENT_HEAP_MAXIMUM_SIZE
                        );

    // validate heap handle
    if (g_hAgentHeap == NULL) {
            
        SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: SVC: error %d creating agent heap.\n",
            GetLastError()
            ));
    }

    // return success if created
    return (g_hAgentHeap != NULL);
}


BOOL
AgentHeapDestroy(
    )

/*++

Routine Description:

    Destroys private heap for master agent private structures.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    // validate handle
    if (g_hAgentHeap != NULL) {

        // release heap handle
        HeapDestroy(g_hAgentHeap);

        // re-initialize
        g_hAgentHeap = NULL;
    }

    return TRUE;
}


LPVOID
AgentMemAlloc(
    UINT nBytes
    )

/*++

Routine Description:

    Allocates memory from master agent's private heap.

Arguments:

    None.

Return Values:

    Returns true if successful.

--*/

{
    // allocate memory from private heap (and initialize)
    return HeapAlloc(g_hAgentHeap, HEAP_ZERO_MEMORY, nBytes);
}


VOID
AgentMemFree(
    LPVOID pMem
    )

/*++

Routine Description:

    Frees memory from master agent's private heap.

Arguments:

    pMem - pointer to memory block to release.

Return Values:

    Returns true if successful.

--*/

{
    // validate pointer
    if (pMem != NULL) {

        // release agent memory
        HeapFree(g_hAgentHeap, 0, pMem);
    }
}

