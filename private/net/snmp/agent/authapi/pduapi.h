/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    pduapi.h

Abstract:

    Contains private definitions, types, and prototypes for the
    encoding/decoding of PDU packets.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

#ifndef pduapi_h
#define pduapi_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>
#include <snmputil.h>

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI SnmpPduEncodePdu(
	   IN BYTE nType,         // Type of RFC 1157 PDU to encode
           IN RFC1157Pdu *pdu,    // RFC 1157 PDU to encode into stream buffer
           IN OUT BYTE **pBuffer, // Stream buffer to accept encoding
	   IN OUT UINT *nLength   // Length of stream buffer
	   );

SNMPAPI SnmpPduDecodePdu(
	   IN BYTE nType,         // Type of RFC 1157 PDU to decode
           OUT RFC1157Pdu *pdu,   // RFC 1157 PDU to accept decoding
           IN OUT BYTE **pBuffer, // Stream buffer to decode
	   IN OUT UINT *nLength   // Length of stream buffer
	   );

SNMPAPI SnmpPduEncodeTrap(
           IN BYTE nType,          // Type of RFC 1157 TRAP to encode
           IN RFC1157TrapPdu *pdu, // RFC 1157 Trap to encode into stream buffer
           IN OUT BYTE **pBuffer,  // Stream buffer to accept encoding
	   IN OUT UINT *nLength    // Length of stream buffer
	   );

SNMPAPI SnmpPduDecodeTrap(
           IN BYTE nType,           // Type of RFC 1157 TRAP to decode
           OUT RFC1157TrapPdu *pdu, // RFC 1157 Trap to accept decoding
           IN OUT BYTE **pBuffer,   // Stream buffer to decode
	   IN OUT UINT *nLength     // Length of stream buffer
	   );

SNMPAPI SnmpPduEncodeAnyPdu(
           RFC1157Pdus *pdu,      // PDU/TRAP to Encode
	   IN OUT BYTE **pBuffer, // Buffer to accept encoding
	   IN OUT UINT *nLength   // Length of buffer
	   );

SNMPAPI SnmpPduDecodeAnyPdu(
	   OUT RFC1157Pdus *pdu,  // Will accept PDU or TRAP as result
           IN OUT BYTE **pBuffer, // Stream buffer to decode
	   IN OUT UINT *nLength   // Length of stream buffer
	   );

SNMPAPI PDU_ReleaseAnyPDU(
           IN OUT RFC1157Pdus *Pdu
	   );

//------------------------------- END ---------------------------------------

#endif /* pduapi_h */
