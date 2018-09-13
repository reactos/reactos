/*++

Copyright (c) 1992-1997  Microsoft Corporation

Module Name:

    snmppdus.h

Abstract:

    Contains definitions for manipulating SNMP PDUs.

Environment:

    User Mode - Win32

Revision History:

    10-Feb-1997 DonRyan
        Rewrote to implement SNMPv2 support.

--*/

#ifndef _SNMPPDUS_H_
#define _SNMPPDUS_H_

///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public definitions                                                        //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

typedef struct _NORMAL_PDU {

    AsnInteger32 nRequestId;
    AsnInteger32 nErrorStatus;
    AsnInteger32 nErrorIndex;

} NORMAL_PDU, *PNORMAL_PDU;

typedef struct _BULK_PDU {

    AsnInteger32 nRequestId;
    AsnInteger32 nErrorStatus;
    AsnInteger32 nErrorIndex;
    AsnInteger32 nNonRepeaters;
    AsnInteger32 nMaxRepetitions;

} BULK_PDU, *PBULK_PDU;

typedef struct _TRAP_PDU {

    AsnObjectIdentifier EnterpriseOid;
    AsnIPAddress        AgentAddr;
    AsnInteger32        nGenericTrap;
    AsnInteger32        nSpecificTrap;
    AsnTimeticks        nTimeticks;

} TRAP_PDU, *PTRAP_PDU;

typedef struct _SNMP_PDU {

    UINT            nType;
    SnmpVarBindList Vbl;
    union {
        TRAP_PDU   TrapPdu;
        BULK_PDU   BulkPdu;
        NORMAL_PDU NormPdu;
    } Pdu;

} SNMP_PDU, *PSNMP_PDU;

#define SNMP_VERSION_1  0
#define SNMP_VERSION_2C 1


///////////////////////////////////////////////////////////////////////////////
//                                                                           //
// Public prototypes                                                         //
//                                                                           //
///////////////////////////////////////////////////////////////////////////////

BOOL
BuildMessage(
    AsnInteger32      nVersion,
    AsnOctetString *  pCommunity,
    PSNMP_PDU         pPdu,
    PBYTE             pMessage,
    PDWORD            pMessageSize
    );

BOOL
ParseMessage(
    AsnInteger32 *   pVersion,
    AsnOctetString * pCommunity,
    PSNMP_PDU        pPdu,
    PBYTE            pMessage,
    DWORD            dwMessageSize
    );

#endif // _SNMPPDUS_H_
