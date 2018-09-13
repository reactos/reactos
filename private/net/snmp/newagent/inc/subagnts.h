/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    subagnts.h

Abstract:

    Contains definitions for manipulating subagent structures.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/
 
#ifndef _SUBAGNTS_H_
#define _SUBAGNTS_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

// flag values to be used in _SUBAGENT_LIST_ENTRY:uchFlags
#define FLG_SLE_KEEP    0x01

typedef struct _SUBAGENT_LIST_ENTRY {
    
    LIST_ENTRY              Link;          
    LIST_ENTRY              SupportedRegions;
    PFNSNMPEXTENSIONINIT    pfnSnmpExtensionInit;
    PFNSNMPEXTENSIONINITEX  pfnSnmpExtensionInitEx;
    PFNSNMPEXTENSIONCLOSE   pfnSnmpExtensionClose;
    PFNSNMPEXTENSIONMONITOR pfnSnmpExtensionMonitor;
    PFNSNMPEXTENSIONQUERY   pfnSnmpExtensionQuery;
    PFNSNMPEXTENSIONQUERYEX pfnSnmpExtensionQueryEx;
    PFNSNMPEXTENSIONTRAP    pfnSnmpExtensionTrap;
    HANDLE                  hSubagentTrapEvent;
    HANDLE                  hSubagentDll;
    UCHAR                   uchFlags;
    LPSTR                   pPathname;

} SUBAGENT_LIST_ENTRY, *PSUBAGENT_LIST_ENTRY;

#define SNMP_EXTENSION_INIT     "SnmpExtensionInit"
#define SNMP_EXTENSION_INIT_EX  "SnmpExtensionInitEx"
#define SNMP_EXTENSION_CLOSE    "SnmpExtensionClose"
#define SNMP_EXTENSION_MONITOR  "SnmpExtensionMonitor"
#define SNMP_EXTENSION_QUERY    "SnmpExtensionQuery"
#define SNMP_EXTENSION_QUERY_EX "SnmpExtensionQueryEx"
#define SNMP_EXTENSION_TRAP     "SnmpExtensionTrap"


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
FindSubagent(
    PSUBAGENT_LIST_ENTRY * ppSLE,
    LPSTR                  pPathname
    );

BOOL
AddSubagentByDll(
    LPSTR pPathname,
    UCHAR uchInitFlags
    );

BOOL
AllocSLE(
    PSUBAGENT_LIST_ENTRY * ppSLE,
    LPSTR                  pPathname,
    UCHAR                  uchInitFlags
    );

BOOL
FreeSLE(
    PSUBAGENT_LIST_ENTRY pSLE
    );

BOOL
LoadSubagents(
    );

BOOL
UnloadSubagents(
    );

#endif // _SUBAGNTS_H_
