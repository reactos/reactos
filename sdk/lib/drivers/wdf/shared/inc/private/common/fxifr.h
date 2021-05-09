/*++

Copyright (c) Microsoft Corporation

Module Name:

    FxIFR.h

Abstract:

    This module contains the private IFR tracing definitions
    The structures are common to the In-Flight Recorder (IFR)
    and the IFR-related debug commands in wdfkd.dll.

Environment:

    kernel mode and debug extensions

Revision History:


--*/

#ifndef _FXIFR_H
#define _FXIFR_H

//
// This is maximum number of arguments a single message can capture.
// It reflects WPP_MAX_MOF_FIELDS value of 8 found in WPP trace support.
//
#define WDF_MAX_LOG_FIELDS              (8)

//
// This is the longest allowable message retained in the IFR log.
//
#define WDF_MAX_IFR_MESSAGE_SIZE        (256)


//
// This is the maximum name length for the driver.
//
#define WDF_IFR_HEADER_NAME_LEN         (32)

//
// Access macro for trace GUID BITs.
//
#define TRACE_BIT(a)   WPP_BIT_ ## a

//
// Various GUIDs needed.
//

//
// (Keep synch-ed with "KmdfTraceGuid" in fxtrace.h)
//
DEFINE_GUID (WdfTraceGuid,
  0x544d4c9d, 0x942c, 0x46d5, 0xbf, 0x50, 0xdf, 0x5c, 0xd9, 0x52, 0x4a, 0x50);

//
// GUID used to tag IFR log in crash dump.
//
DEFINE_GUID (WdfDumpGuid,
  0x54c84888, 0x01d1, 0x4c1e, 0xbe, 0xd6, 0x28, 0x2c, 0x98, 0x24, 0x13, 0x03);

//
// GUID used to tag drivers info in crash dump.
//
// {F87E4A4C-C5A1-4d2f-BFF0-D5DE63A5E4C3}
DEFINE_GUID(WdfDumpGuid2,
0xf87e4a4c, 0xc5a1, 0x4d2f, 0xbf, 0xf0, 0xd5, 0xde, 0x63, 0xa5, 0xe4, 0xc3);

//
// This structure hold the current and previous 16-bit offsets into
// the IFR log.  These variable must be access together as a LONG
// via the InterlockedCompareExchange().
//
typedef struct _WDF_IFR_OFFSET {
    union {
        struct {
            USHORT Current;
            USHORT Previous;
        } s;
        LONG  AsLONG;
    } u;
} WDF_IFR_OFFSET, *PWDF_IFR_OFFSET;


#define WDF_IFR_LOG_TAG 'gLxF'     // 'FxLg'

//
// This is the IFR log header structure.  It is immediately followed
// by the log area itself.
//
typedef struct _WDF_IFR_HEADER {

    GUID            Guid;                   // WDF's GUID (WDF_TRACE_GUID)
    PUCHAR          Base;                   // log data area base (not header)
    ULONG           Size;                   // size of the log (1 page by default)
    WDF_IFR_OFFSET  Offset;                 // current/previous offsets
    LONG            Sequence;               // local sequence number
#if (FX_CORE_MODE == FX_CORE_USER_MODE)
    PLONG           SequenceNumberPointer;  // Global IFR Sequence Number
#endif
    CHAR            DriverName[WDF_IFR_HEADER_NAME_LEN];

} WDF_IFR_HEADER, *PWDF_IFR_HEADER;


#define WDF_IFR_RECORD_SIGNATURE 'RL'  // 'LR'

typedef struct _WDF_IFR_RECORD {

    USHORT      Signature;        // 'LR'  Log Record signature
    USHORT      Length;
    LONG        Sequence;
    USHORT      PrevOffset;       // offset to previous record
    USHORT      MessageNumber;    // message number   see <GUID>.tmf
    GUID        MessageGuid;      // message GUID     see <GUID>.tmf

} WDF_IFR_RECORD, *PWDF_IFR_RECORD;


#endif // _FXIFR_H
