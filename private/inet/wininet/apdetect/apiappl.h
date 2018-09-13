//================================================================================
//  Copyright (C) 1997 Microsoft Corporation
//  Author: RameshV
//  Description: these are the exported dhcp client api function definitions
//================================================================================

#ifndef APIAPPL_H_INCLUDED
#define APIAPPL_H_INCLUDED

#ifndef DHCPAPI_PARAMS_DEFINED
#define DHCPAPI_PARAMS_DEFINED
typedef struct _DHCPAPI_PARAMS {                  // use this structure to request params
    ULONG                          Flags;         // for future use
    ULONG                          OptionId;      // what option is this?
    BOOL                           IsVendor;      // is this vendor specific?
    LPBYTE                         Data;          // the actual data
    DWORD                          nBytesData;    // how many bytes of data are there in Data?
} DHCPAPI_PARAMS, *PDHCPAPI_PARAMS, *LPDHCPAPI_PARAMS;
#endif DHCPAPI_PARAMS_DEFINED

DWORD                                             // win32 status
DhcpAcquireParameters(                            // acquire/renew a lease
    IN      LPWSTR                 AdapterName    // adapter to acquire lease on
);

DWORD                                             // win32 status
DhcpReleaseParameters(                            // release an existing lease
    IN      LPWSTR                 AdapterName    // adpater to release lease for
);

DWORD                                             // win32 status
DhcpEnableDynamicConfic(                          // convert from static to dhcp
    IN      LPWSTR                 AdapterName    // convert for this adapter
);

DWORD                                             // win32 status
DhcpDisableDynamicConfig(                         // convert from dhcp to static
    IN      LPWSTR                 AdapterName    // convert this adapter
);

DWORD                                             // win32 status
DhcpStaticRefreshParams(                          // some registry parameters may have changed, refresh them
    IN      LPWSTR                 AdapterName
);

DWORD
APIENTRY // Request client for options.. and get the options.
DhcpRequestOptions(
    LPWSTR             AdapterName,
    BYTE              *pbRequestedOptions,
    DWORD              dwNumberOfOptions,
    BYTE             **ppOptionList,        // out param
    DWORD             *pdwOptionListSize,   // out param
    BYTE             **ppbReturnedOptions,  // out param
    DWORD             *pdwNumberOfAvailableOptions // out param
);

DWORD
APIENTRY // Register with the client to get Event for notification.
DhcpRegisterOptions(
    LPWSTR             AdapterName ,  // Null implies ALL adapters.
    LPBYTE             OptionList  ,  // The list of options to check.
    DWORD              OptionListSz,  // The size of the above list
    HANDLE             *pdwHandle     // the handle of an event to wait for.
);  // returns an event.


DWORD
APIENTRY // Deregister with the client..
DhcpDeRegisterOptions(
    HANDLE             Event          // This MUST be the one returned by above fn.
);

DWORD                                             // win32 status
APIENTRY
DhcpRequestParameters(                            // request parameters of client
    IN      LPWSTR                 AdapterName,   // adapter name to request for
    IN      LPBYTE                 ClassId,       // byte stream of class id to use
    IN      DWORD                  ClassIdLen,    // # of bytes of class id to use
    IN      PDHCPAPI_PARAMS        SendParams,    // parameters to send to server
    IN      DWORD                  nSendParams,   // size of above array
    IN      DWORD                  Flags,         // must be zero, reserved
    IN OUT  PDHCPAPI_PARAMS        RecdParams,    // fill this array with received params
    IN OUT  LPDWORD                pnRecdParamsBytes // i/p: size of above in BYTES, o/p required bytes or filled up # of elements
);  // returns ERROR_MORE_DATA if o/p buffer is of insufficient size, and fills in reqd size in # of bytes

DWORD                                             // win32 status
APIENTRY
DhcpRegisterParameterChangeNofitication(          // notify if a parameter has changed
    IN      LPWSTR                 AdapterName,   // adapter of interest
    IN      LPBYTE                 ClassId,       // byte stream of class id to use
    IN      DWORD                  ClassIdLen,    // # of bytes of class id
    IN      PDHCPAPI_PARAMS        Params,        // params of interest
    IN      DWORD                  nParams,       // # of elts in above array
    IN      DWORD                  Flags,         // must be zero, reserved
    IN OUT  PHANDLE                hEvent         // handle to event that will be SetEvent'ed in case of param change
);

DWORD
APIENTRY
DhcpDeRegisterParameterChangeNofitication(        // undo the registration
    IN      HANDLE                 Event          // handle to event returned by DhcpRegisterParameterChangeNotification, NULL ==> everything
);

DWORD                                             // win32 status
APIENTRY
DhcpPersistentRequestParams(                      // parameters to request persistently
    IN      LPWSTR                 AdapterName,   // adapter name to request for
    IN      LPBYTE                 ClassId,       // byte stream of class id to use
    IN      DWORD                  ClassIdLen,    // # of bytes of class id
    IN      PDHCPAPI_PARAMS        SendParams,    // persistent parameters
    IN      DWORD                  nSendParams,   // size of above array
    IN      DWORD                  Flags,         // must be zero, reserved
    IN      LPWSTR                 AppName        // the name of the app that is to be used for this instance
);

DWORD                                             // win32 status
APIENTRY
DhcpDelPersistentRequestParams(                   // undo the effect of a persistent request -- currently undo from registry
    IN      LPWSTR                 AdapterName,   // the name of the adpater to delete for
    IN      LPWSTR                 AppName        // the name used by the app
);

#endif APIAPPL_H_INCLUDED
