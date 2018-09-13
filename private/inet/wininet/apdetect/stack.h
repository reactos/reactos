//================================================================================
// Copyright (C) 1997 Microsoft Corporation
// Author: RameshV
// All dealings with the stack and other non-Dhcp components go through the API
// given here
//================================================================================

#ifndef STACK_H_INCLUDED
#define STACK_H_INCLUDED

//================================================================================
// Exported API's
//================================================================================

DWORD                                             // win32 status
DhcpClearAllStackParameters(                      // undo the effects
    IN      PDHCP_CONTEXT          DhcpContext    // the adapter to undo
);

DWORD                                             // win32 status
DhcpSetAllStackParameters(                        // set all stack details
    IN      PDHCP_CONTEXT          DhcpContext,   // the context to set stuff
    IN      PDHCP_FULL_OPTIONS     DhcpOptions    // pick up the configuration from off here
);

#endif STACK_H_INCLUDED

#ifndef SYSSTACK_H_INCLUDED
#define SYSSTACK_H_INCLUDED
//================================================================================
// imported api's
//================================================================================
DWORD                                             // return interface index or -1
DhcpIpGetIfIndex(                                 // get the IF index for this adapter
    IN      PDHCP_CONTEXT          DhcpContext    // context of adapter to get IfIndex for
);

DWORD                                             // win32 status
DhcpSetRoute(                                     // set a route with the stack
    IN      DWORD                  Dest,          // network order destination
    IN      DWORD                  DestMask,      // network order destination mask
    IN      DWORD                  IfIndex,       // interface index to route
    IN      DWORD                  NextHop,       // next hop n/w order address
    IN      BOOL                   IsLocal,       // is this a local address? (IRE_DIRECT)
    IN      BOOL                   IsDelete       // is this route being deleted?
);

#endif SYSSTACK_H_INCLUDED
