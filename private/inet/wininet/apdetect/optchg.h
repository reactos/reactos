//================================================================================
// Copyright (c) 1997 Microsoft Corporation
// Author: RameshV
// Description: handles the noticiations and other mechanisms for parameter
//      changes (options )
//================================================================================

#ifndef OPTCHG_H_INCLUDED
#define OPTCHG_H_INCLUDED

//================================================================================
// exported APIS
//================================================================================
DWORD                                             // win32 status
DhcpAddParamChangeRequest(                        // add a new param change notification request
    IN      LPWSTR                 AdapterName,   // for this adapter, can be NULL
    IN      LPBYTE                 ClassId,       // what class id does this belong to?
    IN      DWORD                  ClassIdLength, // how big is this class id?
    IN      LPBYTE                 OptList,       // this is the list of options of interest
    IN      DWORD                  OptListSize,   // this is the # of bytes of above
    IN      BOOL                   IsVendor,      // is this vendor specific?
    IN      DWORD                  ProcId,        // which is the calling process?
    IN      DWORD                  Descriptor,    // what is the unique descriptor in this process?
    IN      HANDLE                 Handle         // what is the handle in the calling process space?
);

DWORD                                             // win32 status
DhcpDelParamChangeRequest(                        // delete a particular request
    IN      DWORD                  ProcId,        // the process id of the caller
    IN      HANDLE                 Handle         // the handle as used by the calling process
);

DWORD                                             // win32 status
DhcpMarkParamChangeRequests(                      // find all params that are affected and mark then as pending
    IN      LPTSTR                 AdapterName,   // adapter of relevance
    IN      BYTE                   OptionId,      // the option id itself
    IN      BOOL                   IsVendor,      // is this vendor specific
    IN      LPBYTE                 ClassId        // which class --> this must be something that has been ADD-CLASSED
);

typedef DWORD (*DHCP_NOTIFY_FUNC)(                // this is the type of the fucntion that actually notifies clients of option change
    IN      DWORD                  ProcId,        // <ProcId + Descriptor> make a unique key used for finding the event
    IN      DWORD                  Descriptor     // --- on Win98, only Descriptor is really needed.
);                                                // if return value is NOT error success, we delete this request

DWORD                                             // win32 status
DhcpNotifyMarkedParamChangeRequests(              // notify pending param change requests
    IN      DHCP_NOTIFY_FUNC       NotifyHandler  // call this function for each unique id that is present
);


DWORD                                             // win32 status
DhcpNotifyClientOnParamChange(                    // notify clients
    IN      DWORD                  ProcId,        // which process called this
    IN      DWORD                  Descriptor     // unique descriptor for that process
);

DWORD                                             // win32 status
DhcpInitializeParamChangeRequests(                // initialize everything in this file
    VOID
);

VOID
DhcpCleanupParamChangeRequests(                   // unwind this module
    VOID
);

DWORD                                             // win32 status
DhcpAddParamRequestChangeRequestList(             // add to the request list the list of params registered for notifications
    IN      LPWSTR                 AdapterName,   // which adatper is this request list being requested for?
    IN      LPBYTE                 Buffer,        // buffer to add options to
    IN OUT  LPDWORD                Size,          // in: existing filled up size, out: total size filled up
    IN      LPBYTE                 ClassName,     // ClassId
    IN      DWORD                  ClassLen       // size of ClassId in bytes
);

#endif OPTCHG_H_INCLUDED
