/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    globals.c

Abstract:

    Contains global data for SNMP master agent.

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


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Global variables                                                          //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

DWORD  g_dwUpTimeReference = 0;
HANDLE g_hTerminationEvent = NULL;
HANDLE g_hRegistryEvent = NULL;

// notification event for changes in the default parameters registry tree
HANDLE g_hDefaultRegNotifier;
HKEY   g_hDefaultKey;
// notification event for changes in the policy parameters registry tree
HANDLE g_hPolicyRegNotifier;
HKEY   g_hPolicyKey;

LIST_ENTRY g_Subagents          = { NULL };
LIST_ENTRY g_SupportedRegions   = { NULL };
LIST_ENTRY g_ValidCommunities   = { NULL };
LIST_ENTRY g_TrapDestinations   = { NULL };
LIST_ENTRY g_PermittedManagers  = { NULL };
LIST_ENTRY g_IncomingTransports = { NULL };
LIST_ENTRY g_OutgoingTransports = { NULL };

CMD_LINE_ARGUMENTS g_CmdLineArguments;

