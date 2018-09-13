/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    authXXXX.h

Abstract:

    Decode/Encode RFC XXXX SnmpPrivMsg, SnmpAuthMsg, and SnmpMgmtCom.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

#ifndef authXXXX_h
#define authXXXX_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>
#include <snmputil.h>

//--------------------------- PUBLIC STRUCTS --------------------------------

typedef struct {
    AsnAny authInfo;
    AsnImplicitSequence authData;
} SnmpAuthMsg;

typedef AsnOctetString AsnImplicitOctetString;
typedef struct {
    AsnObjectIdentifier privDst;
    AsnImplicitOctetString privData;
} SnmpPrivMsg;

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------


SNMPAPI SnmpEncodePrivMsg(
           IN AsnObjectIdentifier *DestParty, // Destination party
	   IN OUT BYTE **pBuffer,             // Buffer to accept encoding
	   IN OUT UINT *nLength               // Length of buffer
	   );

SNMPAPI SnmpEncodeAuthMsg(
           IN AsnObjectIdentifier *SrcParty, // To determine how to auth.
	   IN OUT BYTE **pBuffer,            // Buffer to accept encoding
	   IN OUT UINT *nLength              // Length of buffer
	   );

SNMPAPI SnmpEncodeMgmtCom(
           IN SnmpMgmtCom *MgmtCom, // Mgmt Com message to encode
           IN OUT BYTE **pBuffer,   // Buffer to accept encoding
	   IN OUT UINT *nLength     // Length of buffer
	   );

SNMPAPI SnmpDecodePrivMsg(
           IN OUT BYTE **pBuffer,
	   IN OUT UINT *nLength,
           OUT SnmpPrivMsg *PrivMsg
	   );

SNMPAPI SnmpDecodeAuthMsg(
	   IN AsnObjectIdentifier *privDst,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength,
           OUT SnmpAuthMsg *AuthMsg
	   );

SNMPAPI SnmpDecodeMgmtCom(
           IN OUT BYTE **pBuffer,
	   IN OUT UINT *nLength,
           SnmpMgmtCom *MgmtCom
	   );


//------------------------------- END ---------------------------------------

#endif /* authXXXX_h */
