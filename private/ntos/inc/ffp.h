/*++

Copyright (c) 1998 Microsoft Corporation

Module Name:

    ffp.h

Abstract:

    Structures used in controlling the
    Fast Forwarding Path (FFP)
    functionality in network drivers.

Author:

    Chaitanya Kodeboyina (chaitk)  30-Sep-1998

Environment:

    Kernel Mode

Revision History:

--*/

#ifndef _FFP_H
#define _FFP_H

//
// CacheEntryTypes for OID_FFP_SEED
//

#define FFP_DISCARD_PACKET        -1 // -ve cache entry  - packet discarded
#define FFP_INDICATE_PACKET        0 // invalid entry - packet passed to xport
#define FFP_FORWARD_PACKET        +1 // +ve cache entry - packet forwarded

//
// Input format for various NDIS OIDs
// used in controlling FFP operation
//

//
// RequestInfo for OID_FFP_SUPPORT query
//
typedef    struct _FFPVersionParams {
    ULONG          NdisProtocolType;
    ULONG          FFPVersion;
}   FFPVersionParams;

//
// RequestInfo for OID_FFP_SUPPORT set
//
typedef    struct _FFPSupportParams {
    ULONG          NdisProtocolType;
    ULONG          FastForwardingCacheSize;
    ULONG          FFPControlFlags;
}   FFPSupportParams;

//
// RequestInfo for OID_FFP_FLUSH set
//
typedef struct _FFPFlushParams {
    ULONG          NdisProtocolType;
}   FFPFlushParams;

//
// RequestInfo for OID_FFP_CONTROL query/set
//
typedef    struct _FFPControlParams {
    ULONG          NdisProtocolType;
    ULONG          FFPControlFlags;
}   FFPControlParams;

//
// RequestInfo for OID_FFP_PARAMS query/set
//
typedef    struct _FFPCacheParams {
    ULONG          NdisProtocolType;
    ULONG          FastForwardingCacheSize;
}   FFPCacheParams;

//
// RequestInfo for OID_FFP_SEED query/set
//
typedef struct _FFPDataParams {
    ULONG          NdisProtocolType;
    LONG           CacheEntryType;
    ULONG          HeaderSize;
    union {
        UCHAR       Header[1];
        struct {
            IPHeader Header;
            ULONG    DwordAfterHeader;
        }           IpHeader;
    };
}   FFPDataParams;


//
// RequestInfo for OID_FFP_IFSTATS query/reset
// [ used to get per adapter FF statistics ]
//

/*
InPacketsForwarded refers to the number of packets
received on this adapter that were forwarded out
on another adapter, 
and
OutPacketsForwarded refer to the number of packets
received on another adapter and forwarded out on
on this adapter.
*/

typedef struct _FFPAdapterStats {
    ULONG          NdisProtocolType; 
    ULONG          InPacketsForwarded;
    ULONG          InOctetsForwarded;
    ULONG          InPacketsDiscarded;
    ULONG          InOctetsDiscarded;
    ULONG          InPacketsIndicated;
    ULONG          InOctetsIndicated;
    ULONG          OutPacketsForwarded;
    ULONG          OutOctetsForwarded;
}   FFPAdapterStats;


//
// RequestInfo for OID_FFP_GLSTATS query/reset
// [ used to get global Fast Forwarding stats ]
//

/*
PacketsForwarded refers to the number of packets
forwarded out in the fast path,
and
PacketsDiscarded refers to the number of packets
discarded on the fast path, 
and
PacketsIndicated refers to the number of packets
that were indicated to transport.
*/

typedef struct _FFPDriverStats {
    ULONG          NdisProtocolType;
    ULONG          PacketsForwarded;
    ULONG          OctetsForwarded;
    ULONG          PacketsDiscarded;
    ULONG          OctetsDiscarded;
    ULONG          PacketsIndicated;
    ULONG          OctetsIndicated;
}   FFPDriverStats;

#endif // _FFP_H
