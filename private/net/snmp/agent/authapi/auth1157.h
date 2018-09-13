/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    auth1157.h

Abstract:

    Decode/Encode RFC 1157 Messages.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
#ifndef auth1157_h
#define auth1157_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>
#include <snmputil.h>

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

typedef struct {
    AsnInteger version;
    AsnOctetString community;
    RFC1157Pdus data;
} RFC1157Message;

//--------------------------- PUBLIC PROTOTYPES -----------------------------

SNMPAPI SnmpEncodeRFC1157Message(
           IN RFC1157Message *message, // Message to encode into stream
           IN OUT BYTE **pBuffer,      // Buffer to accept encoded message
           IN OUT UINT *nLength        // Length of buffer
	   );

SNMPAPI SnmpDecodeRFC1157Message(
           OUT RFC1157Message *message, // Decoded message from stream
           IN BYTE *pBuffer,       // Buffer containing stream to decode
           IN UINT nLength         // Length of buffer
	   );

SNMPAPI SnmpRFC1157MessageToMgmtCom(
           IN RFC1157Message *message,   // RFC 1157 Message to convert
           OUT SnmpMgmtCom *snmpMgmtCom, // Resulting Management Com format
           IN BOOL fAuthMsg // whether to perform optional authentication
	   );

SNMPAPI SnmpMgmtComToRFC1157Message(
           OUT RFC1157Message *message, // Resulting 1157 format
           IN SnmpMgmtCom *snmpMgmtCom  // Management Com message to convert
	   );

//------------------------------- END ---------------------------------------

#endif /* auth1157_h */

