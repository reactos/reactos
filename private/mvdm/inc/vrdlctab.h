/*++

Copyright (c) 1991  Microsoft Corporation

Module Name:

    vrdlctab.h

Abstract:

    Contains structures which are shared between the VDM code and the DOS
    redir code. Separated from VDMREDIR.H

Author:

    Richard L Firth (rfirth) 13-May-1992

Revision History:

--*/

//
// manifests
//

#define DOS_DLC_STATUS_PERM_SLOTS   10
#define DOS_DLC_STATUS_TEMP_SLOTS   5
#define DOS_DLC_MAX_ADAPTERS        2

//
// EXTENDED_STATUS_PARMS - there is one of these per adapter
//

/* XLATOFF */
#pragma pack(1)
/* XLATON */

typedef struct _EXTENDED_STATUS_PARMS { /* */
    BYTE    cbSize;
    BYTE    cbPageFrameSize;
    WORD    wAdapterType;
    WORD    wCurrentFrameSize;
    WORD    wMaxFrameSize;
} EXTENDED_STATUS_PARMS;

typedef EXTENDED_STATUS_PARMS UNALIGNED * PEXTENDED_STATUS_PARMS;

//
// DOS_DLC_STATUS - there is one of these for each of the permanent and temporary
// connections
//

typedef struct _DOS_DLC_STATUS { /* */
    WORD    usStationId;
    WORD    usDlcStatusCode;
    BYTE    uchFrmrData[5];
    BYTE    uchAccessPriority;
    BYTE    uchRemoteNodeAddress[6];
    BYTE    uchRemoteSap;
    BYTE    auchReserved[3];
} DOS_DLC_STATUS;

typedef DOS_DLC_STATUS UNALIGNED * PDOS_DLC_STATUS;

//
// ADAPTER_STATUS_PARMS - In real DOS workstation, this is maintained by the
// adapter software, but is made available to applications through DIR.STATUS.
// Token Ring and Ethernet adapter have different adapter status parameters
//
// Note: some fields prefixed by Tr or Eth because the moronic x86 assembler
// can't handle the same field name in different structures
//

typedef struct _TOKEN_RING_ADAPTER_STATUS_PARMS { /* */
    DWORD   PhysicalAddress;
    BYTE    UpstreamNodeAddress[6];
    DWORD   UpstreamPhysicalAddress;
    BYTE    LastPollAddress[6];
    WORD    AuthorizedEnvironment;
    WORD    TransmitAccessPriority;
    WORD    SourceClassAuthorization;
    WORD    LastAttentionCode;
    BYTE    TrLastSourceAddress[6];
    WORD    LastBeaconType;
    WORD    TrLastMajorVector;
    WORD    TrNetworkStatus;
    WORD    SoftError;
    WORD    FrontEndErrorCount;
    WORD    LocalRingNumber;
    WORD    MonitorErrorCode;
    WORD    BeaconTransmitType;
    WORD    BeaconReceiveType;
    WORD    TrFrameCorrelation;
    BYTE    BeaconingNaun[6];
    DWORD   Reserved;
    DWORD   BeaconingPhysicalAddress;
} TOKEN_RING_ADAPTER_STATUS_PARMS;

typedef TOKEN_RING_ADAPTER_STATUS_PARMS UNALIGNED * PTOKEN_RING_ADAPTER_STATUS_PARMS;

typedef struct _ETHERNET_ADAPTER_STATUS_PARMS { /* */
    BYTE    Reserved1[28];
    BYTE    EthLastSourceAddress[6];
    BYTE    Reserved2[2];
    WORD    EthLastMajorVector;
    WORD    EthNetworkStatus;
    WORD    ErrorReportTimerValue;
    WORD    ErrorReportTimerTickCounter;
    WORD    LocalBusNumber;
    BYTE    Reserved3[6];
    WORD    EthFrameCorrelation;
    BYTE    Reserved4[6];
    WORD    NetworkUtilizationSamples;
    WORD    NetworkBusySamples;
    BYTE    Reserved5[4];
} ETHERNET_ADAPTER_STATUS_PARMS;

typedef ETHERNET_ADAPTER_STATUS_PARMS UNALIGNED * PETHERNET_ADAPTER_STATUS_PARMS;

typedef union _ADAPTER_STATUS_PARMS { /* */
    TOKEN_RING_ADAPTER_STATUS_PARMS TokenRing;
    ETHERNET_ADAPTER_STATUS_PARMS Ethernet;
} ADAPTER_STATUS_PARMS;

typedef ADAPTER_STATUS_PARMS UNALIGNED * PADAPTER_STATUS_PARMS;

//
// VDM_REDIR_DOS_WINDOW - this structure is used by the MVDM DLC code to return
// information to the DOS DLC program via the redir. This is used mainly in
// asynchronous call-backs (aka ANRs, post-routines or DLC appendages). We let
// the redir code know if there is an ANR by setting dwPostRoutine
//

typedef struct _VDM_REDIR_DOS_WINDOW { /* */
    DWORD   dwPostRoutine;
    DWORD   dwDlcTimerTick;
    EXTENDED_STATUS_PARMS aExtendedStatus[DOS_DLC_MAX_ADAPTERS];
    ADAPTER_STATUS_PARMS AdapterStatusParms[DOS_DLC_MAX_ADAPTERS];
    DOS_DLC_STATUS aStatusTables[(DOS_DLC_STATUS_TEMP_SLOTS + DOS_DLC_STATUS_PERM_SLOTS)];
} VDM_REDIR_DOS_WINDOW;

typedef VDM_REDIR_DOS_WINDOW UNALIGNED * LPVDM_REDIR_DOS_WINDOW;

/* XLATOFF */
#pragma pack()
/* XLATON */
