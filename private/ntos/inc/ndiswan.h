/*++ BUILD Version: 0000    // Increment this if a change has global effects

Copyright (c) 1994  Microsoft Corporation

Module Name:

    ndiswan.h

Abstract:

    Main header file for the wan wrapper

Author:

    Thomas J. Dimitri (TommyD)  20-Feb-1994

Revision History:

--*/


#ifndef _NDIS_WAN_
#define _NDIS_WAN_

//
// Begin definitions for WANs
//

//
// Bit field set int he Reserved field for
// NdisRegisterMiniport or passed in NdisRegisterSpecial
//

#define NDIS_USE_WAN_WRAPPER            0x00000001

#define NDIS_STATUS_TAPI_INDICATION ((NDIS_STATUS)0x40010080L)


//
// NDIS WAN Framing bits
//
#define RAS_FRAMING                     0x00000001
#define RAS_COMPRESSION                 0x00000002

#define ARAP_V1_FRAMING                 0x00000004
#define ARAP_V2_FRAMING                 0x00000008
#define ARAP_FRAMING                    (ARAP_V1_FRAMING | ARAP_V2_FRAMING)

#define PPP_MULTILINK_FRAMING           0x00000010
#define PPP_SHORT_SEQUENCE_HDR_FORMAT   0x00000020
#define PPP_MC_MULTILINK_FRAMING        0x00000040

#define PPP_FRAMING                     0x00000100
#define PPP_COMPRESS_ADDRESS_CONTROL    0x00000200
#define PPP_COMPRESS_PROTOCOL_FIELD     0x00000400
#define PPP_ACCM_SUPPORTED              0x00000800

#define SLIP_FRAMING                    0x00001000
#define SLIP_VJ_COMPRESSION             0x00002000
#define SLIP_VJ_AUTODETECT              0x00004000

#define MEDIA_NRZ_ENCODING              0x00010000
#define MEDIA_NRZI_ENCODING             0x00020000
#define MEDIA_NLPID                     0x00040000

#define RFC_1356_FRAMING                0x00100000
#define RFC_1483_FRAMING                0x00200000
#define RFC_1490_FRAMING                0x00400000
#define LLC_ENCAPSULATION               0x00800000

#define SHIVA_FRAMING                   0x01000000
#define NBF_PRESERVE_MAC_ADDRESS        0x01000000

#ifndef _WAN50_
#define PASS_THROUGH_MODE               0x10000000
#define RAW_PASS_THROUGH_MODE           0x20000000
#endif

#define TAPI_PROVIDER                   0x80000000

//
// NDIS WAN Information structures used
// by NDIS 3.1 Wan Miniport drivers
//
typedef struct _NDIS_WAN_INFO
{
    OUT ULONG           MaxFrameSize;
    OUT ULONG           MaxTransmit;
    OUT ULONG           HeaderPadding;
    OUT ULONG           TailPadding;
    OUT ULONG           Endpoints;
    OUT UINT            MemoryFlags;
    OUT NDIS_PHYSICAL_ADDRESS HighestAcceptableAddress;
    OUT ULONG           FramingBits;
    OUT ULONG           DesiredACCM;
} NDIS_WAN_INFO, *PNDIS_WAN_INFO;

typedef struct _NDIS_WAN_SET_LINK_INFO
{
    IN  NDIS_HANDLE     NdisLinkHandle;
    IN  ULONG           MaxSendFrameSize;
    IN  ULONG           MaxRecvFrameSize;
        ULONG           HeaderPadding;
        ULONG           TailPadding;
    IN  ULONG           SendFramingBits;
    IN  ULONG           RecvFramingBits;
    IN  ULONG           SendCompressionBits;
    IN  ULONG           RecvCompressionBits;
    IN  ULONG           SendACCM;
    IN  ULONG           RecvACCM;
} NDIS_WAN_SET_LINK_INFO, *PNDIS_WAN_SET_LINK_INFO;

typedef struct _NDIS_WAN_GET_LINK_INFO {
    IN  NDIS_HANDLE     NdisLinkHandle;
    OUT ULONG           MaxSendFrameSize;
    OUT ULONG           MaxRecvFrameSize;
    OUT ULONG           HeaderPadding;
    OUT ULONG           TailPadding;
    OUT ULONG           SendFramingBits;
    OUT ULONG           RecvFramingBits;
    OUT ULONG           SendCompressionBits;
    OUT ULONG           RecvCompressionBits;
    OUT ULONG           SendACCM;
    OUT ULONG           RecvACCM;
} NDIS_WAN_GET_LINK_INFO, *PNDIS_WAN_GET_LINK_INFO;

//
// NDIS WAN Bridging Options
//
#define BRIDGING_FLAG_LANFCS            0x00000001
#define BRIDGING_FLAG_LANID             0x00000002
#define BRIDGING_FLAG_PADDING           0x00000004

//
// NDIS WAN Bridging Capabilities
//
#define BRIDGING_TINYGRAM               0x00000001
#define BRIDGING_LANID                  0x00000002
#define BRIDGING_NO_SPANNING_TREE       0x00000004
#define BRIDGING_8021D_SPANNING_TREE    0x00000008
#define BRIDGING_8021G_SPANNING_TREE    0x00000010
#define BRIDGING_SOURCE_ROUTING         0x00000020
#define BRIDGING_DEC_LANBRIDGE          0x00000040

//
// NDIS WAN Bridging Type
//
#define BRIDGING_TYPE_RESERVED          0x00000001
#define BRIDGING_TYPE_8023_CANON        0x00000002
#define BRIDGING_TYPE_8024_NO_CANON     0x00000004
#define BRIDGING_TYPE_8025_NO_CANON     0x00000008
#define BRIDGING_TYPE_FDDI_NO_CANON     0x00000010
#define BRIDGING_TYPE_8024_CANON        0x00000400
#define BRIDGING_TYPE_8025_CANON        0x00000800
#define BRIDGING_TYPE_FDDI_CANON        0x00001000

typedef struct _NDIS_WAN_GET_BRIDGE_INFO
{
    IN  NDIS_HANDLE     NdisLinkHandle;
    OUT USHORT          LanSegmentNumber;
    OUT UCHAR           BridgeNumber;
    OUT UCHAR           BridgingOptions;
    OUT ULONG           BridgingCapabilities;
    OUT UCHAR           BridgingType;
    OUT UCHAR           MacBytes[6];
} NDIS_WAN_GET_BRIDGE_INFO, *PNDIS_WAN_GET_BRIDGE_INFO;

typedef struct _NDIS_WAN_SET_BRIDGE_INFO
{
    IN  NDIS_HANDLE     NdisLinkHandle;
    IN  USHORT          LanSegmentNumber;
    IN  UCHAR           BridgeNumber;
    IN  UCHAR           BridgingOptions;
    IN  ULONG           BridgingCapabilities;
    IN  UCHAR           BridgingType;
    IN  UCHAR           MacBytes[6];
} NDIS_WAN_SET_BRIDGE_INFO, *PNDIS_WAN_SET_BRIDGE_INFO;

//
// NDIS WAN Compression Information
//

//
// Define MSCompType bit field, 0 disables all
//
#define NDISWAN_COMPRESSION             0x00000001
#define NDISWAN_ENCRYPTION              0x00000010
#define NDISWAN_40_ENCRYPTION           0x00000020
#define NDISWAN_128_ENCRYPTION          0x00000040
#define NDISWAN_56_ENCRYPTION           0x00000080
#define NDISWAN_HISTORY_LESS            0x01000000

//
// Define CompType codes
//
#define COMPTYPE_OUI                    0
#define COMPTYPE_NT31RAS                254
#define COMPTYPE_NONE                   255


typedef struct _NDIS_WAN_COMPRESS_INFO
{
    UCHAR   SessionKey[8];
    ULONG   MSCompType;

    // Fields above indicate NDISWAN capabilities.
    // Fields below indicate MAC-specific capabilities.

    UCHAR   CompType;
    USHORT  CompLength;

    union
    {
        struct
        {
            UCHAR   CompOUI[3];
            UCHAR   CompSubType;
            UCHAR   CompValues[32];
        } Proprietary;

        struct
        {
            UCHAR   CompValues[32];
        } Public;
    };
} NDIS_WAN_COMPRESS_INFO;

typedef NDIS_WAN_COMPRESS_INFO UNALIGNED *PNDIS_WAN_COMPRESS_INFO;

typedef struct _NDIS_WAN_GET_COMP_INFO
{
    IN  NDIS_HANDLE             NdisLinkHandle;
    OUT NDIS_WAN_COMPRESS_INFO  SendCapabilities;
    OUT NDIS_WAN_COMPRESS_INFO  RecvCapabilities;
} NDIS_WAN_GET_COMP_INFO, *PNDIS_WAN_GET_COMP_INFO;

typedef struct _NDIS_WAN_SET_COMP_INFO
{
    IN  NDIS_HANDLE             NdisLinkHandle;
    IN  NDIS_WAN_COMPRESS_INFO  SendCapabilities;
    IN  NDIS_WAN_COMPRESS_INFO  RecvCapabilities;
} NDIS_WAN_SET_COMP_INFO, *PNDIS_WAN_SET_COMP_INFO;

//
// NDIS WAN Statistics Information
//

typedef struct _NDIS_WAN_GET_STATS_INFO
{
    IN  NDIS_HANDLE NdisLinkHandle;
    OUT ULONG       BytesSent;
    OUT ULONG       BytesRcvd;
    OUT ULONG       FramesSent;
    OUT ULONG       FramesRcvd;
    OUT ULONG       CRCErrors;                      // Serial-like info only
    OUT ULONG       TimeoutErrors;                  // Serial-like info only
    OUT ULONG       AlignmentErrors;                // Serial-like info only
    OUT ULONG       SerialOverrunErrors;            // Serial-like info only
    OUT ULONG       FramingErrors;                  // Serial-like info only
    OUT ULONG       BufferOverrunErrors;            // Serial-like info only
    OUT ULONG       BytesTransmittedUncompressed;   // Compression info only
    OUT ULONG       BytesReceivedUncompressed;      // Compression info only
    OUT ULONG       BytesTransmittedCompressed;     // Compression info only
    OUT ULONG       BytesReceivedCompressed;        // Compression info only
} NDIS_WAN_GET_STATS_INFO, *PNDIS_WAN_GET_STATS_INFO;

#define NdisMWanInitializeWrapper(NdisWrapperHandle,                                \
                                  SystemSpecific1,                                  \
                                  SystemSpecific2,                                  \
                                  SystemSpecific3)                                  \
{                                                                                   \
    NdisMInitializeWrapper(NdisWrapperHandle,                                       \
                            SystemSpecific1,                                        \
                            SystemSpecific2,                                        \
                            SystemSpecific3);                                       \
}

typedef struct _NDIS_MAC_LINE_UP
{
    IN  ULONG               LinkSpeed;
    IN  NDIS_WAN_QUALITY    Quality;
    IN  USHORT              SendWindow;
    IN  NDIS_HANDLE      ConnectionWrapperID;
    IN  NDIS_HANDLE      NdisLinkHandle;
    OUT NDIS_HANDLE      NdisLinkContext;
} NDIS_MAC_LINE_UP, *PNDIS_MAC_LINE_UP;


typedef struct _NDIS_MAC_LINE_DOWN
{
    IN  NDIS_HANDLE      NdisLinkContext;
} NDIS_MAC_LINE_DOWN, *PNDIS_MAC_LINE_DOWN;


//
// These are the error values that can be indicated by the driver.
// This bit field is set when calling NdisIndicateStatus.
//
#define WAN_ERROR_CRC               ((ULONG)0x00000001)
#define WAN_ERROR_FRAMING           ((ULONG)0x00000002)
#define WAN_ERROR_HARDWAREOVERRUN   ((ULONG)0x00000004)
#define WAN_ERROR_BUFFEROVERRUN     ((ULONG)0x00000008)
#define WAN_ERROR_TIMEOUT           ((ULONG)0x00000010)
#define WAN_ERROR_ALIGNMENT         ((ULONG)0x00000020)

typedef struct _NDIS_MAC_FRAGMENT
{
    IN  NDIS_HANDLE     NdisLinkContext;
    IN  ULONG           Errors;
} NDIS_MAC_FRAGMENT, *PNDIS_MAC_FRAGMENT;

//
// NDIS WAN Information structures used
// by NDIS 5.0 Miniport drivers
//

//
// Defines for the individual fields are the
// same as for NDIS 3.x/4.x Wan miniports.
//
// See the DDK.
//

//
// Information that applies to all VC's on
// this adapter.
//
// OID: OID_WAN_CO_GET_INFO
//
typedef struct _NDIS_WAN_CO_INFO {
    OUT ULONG           MaxFrameSize;
    OUT ULONG           MaxSendWindow;
    OUT ULONG           FramingBits;
    OUT ULONG           DesiredACCM;
} NDIS_WAN_CO_INFO, *PNDIS_WAN_CO_INFO;

//
// Set VC specific PPP framing information.
//
// OID: OID_WAN_CO_SET_LINK_INFO
//
typedef struct _NDIS_WAN_CO_SET_LINK_INFO {
    IN  ULONG           MaxSendFrameSize;
    IN  ULONG           MaxRecvFrameSize;
    IN  ULONG           SendFramingBits;
    IN  ULONG           RecvFramingBits;
    IN  ULONG           SendCompressionBits;
    IN  ULONG           RecvCompressionBits;
    IN  ULONG           SendACCM;
    IN  ULONG           RecvACCM;
} NDIS_WAN_CO_SET_LINK_INFO, *PNDIS_WAN_CO_SET_LINK_INFO;

//
// Get VC specific PPP framing information.
//
// OID: OID_WAN_CO_GET_LINK_INFO
//
typedef struct _NDIS_WAN_CO_GET_LINK_INFO {
    OUT ULONG           MaxSendFrameSize;
    OUT ULONG           MaxRecvFrameSize;
    OUT ULONG           SendFramingBits;
    OUT ULONG           RecvFramingBits;
    OUT ULONG           SendCompressionBits;
    OUT ULONG           RecvCompressionBits;
    OUT ULONG           SendACCM;
    OUT ULONG           RecvACCM;
} NDIS_WAN_CO_GET_LINK_INFO, *PNDIS_WAN_CO_GET_LINK_INFO;

//
// Get VC specific PPP compression information
//
// OID: OID_WAN_CO_GET_COMP_INFO
//
typedef struct _NDIS_WAN_CO_GET_COMP_INFO {
    OUT NDIS_WAN_COMPRESS_INFO  SendCapabilities;
    OUT NDIS_WAN_COMPRESS_INFO  RecvCapabilities;
} NDIS_WAN_CO_GET_COMP_INFO, *PNDIS_WAN_CO_GET_COMP_INFO;


//
// Set VC specific PPP compression information
//
// OID: OID_WAN_CO_SET_COMP_INFO
//
typedef struct _NDIS_WAN_CO_SET_COMP_INFO {
    IN  NDIS_WAN_COMPRESS_INFO  SendCapabilities;
    IN  NDIS_WAN_COMPRESS_INFO  RecvCapabilities;
} NDIS_WAN_CO_SET_COMP_INFO, *PNDIS_WAN_CO_SET_COMP_INFO;


//
// Get VC specific statistics
//
// OID: OID_WAN_CO_GET_STATS_INFO
//
typedef struct _NDIS_WAN_CO_GET_STATS_INFO {
    OUT ULONG       BytesSent;
    OUT ULONG       BytesRcvd;
    OUT ULONG       FramesSent;
    OUT ULONG       FramesRcvd;
    OUT ULONG       CRCErrors;                      // Serial-like info only
    OUT ULONG       TimeoutErrors;                  // Serial-like info only
    OUT ULONG       AlignmentErrors;                // Serial-like info only
    OUT ULONG       SerialOverrunErrors;            // Serial-like info only
    OUT ULONG       FramingErrors;                  // Serial-like info only
    OUT ULONG       BufferOverrunErrors;            // Serial-like info only
    OUT ULONG       BytesTransmittedUncompressed;   // Compression info only
    OUT ULONG       BytesReceivedUncompressed;      // Compression info only
    OUT ULONG       BytesTransmittedCompressed;     // Compression info only
    OUT ULONG       BytesReceivedCompressed;        // Compression info only
} NDIS_WAN_CO_GET_STATS_INFO, *PNDIS_WAN_CO_GET_STATS_INFO;

//
// Used to notify NdisWan of Errors.  See error
// bit mask in ndiswan.h
//
// NDIS_STATUS: NDIS_STATUS_WAN_CO_FRAGMENT
//
typedef struct _NDIS_WAN_CO_FRAGMENT {
    IN  ULONG           Errors;
} NDIS_WAN_CO_FRAGMENT, *PNDIS_WAN_CO_FRAGMENT;

//
// Used to notify NdisWan of changes in link speed and
// send window.  Can be given at any time.  NdisWan will honor
// any send window (even zero).  NdisWan will default zero
// TransmitSpeed/ReceiveSpeed settings to 28.8Kbs.
//
// NDIS_STATUS: NDIS_STATUS_WAN_CO_LINKPARAMS
//
typedef struct _WAN_CO_LINKPARAMS {
    ULONG   TransmitSpeed;              // Transmit speed of the VC in Bytes/sec
    ULONG   ReceiveSpeed;               // Receive speed of the VC in Bytes/sec
    ULONG   SendWindow;                 // Current send window for the VC
} WAN_CO_LINKPARAMS, *PWAN_CO_LINKPARAMS;

#endif  // _NDIS_WAN
