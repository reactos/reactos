/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    berapi.h

Abstract:

    ASN.1 BER Encode/Decode APIs.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

#ifndef berapi_h
#define berapi_h

//--------------------------- PUBLIC CONSTANTS ------------------------------

#include <snmp.h>
#include <snmputil.h>

#define SnmpBerEncodeAsnSequence( nSeqLen, \
                                  pBuffer, \
                                  nLength ) \
	  SnmpBerEncodeAsnImplicitSeq( ASN_SEQUENCE, nSeqLen, pBuffer, nLength )

//--------------------------- PUBLIC STRUCTS --------------------------------

//--------------------------- PUBLIC VARIABLES --(same as in module.c file)--

//--------------------------- PUBLIC PROTOTYPES -----------------------------

//
// Private Decoding API
//
SNMPAPI SnmpBerDecodeAsnStream(
           IN BYTE idExpectedAsnType, // Expected ASN type in buffer
           IN OUT BYTE **pBuffer,     // Buffer to decode
           IN OUT UINT *nLength,      // Length of buffer
           OUT AsnAny *pResult        // Contains decoded object and type
	   );

//
// Private Encoding API's
//
SNMPAPI SnmpBerEncodeAsnInteger(
           IN BYTE nTag,
           IN AsnInteger lInteger,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
	   );
SNMPAPI SnmpBerEncodeAsnOctetStr(
	   IN BYTE nTag,
           IN AsnOctetString *String,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
	   );
SNMPAPI SnmpBerEncodeAsnObjectId(
	   IN BYTE nTag,
           IN AsnObjectIdentifier *ObjectId,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
	   );
SNMPAPI SnmpBerEncodeAsnNull(
           IN BYTE nTag,
	   IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
	   );
SNMPAPI SnmpBerEncodeAsnImplicitSeq(
           IN BYTE nTag,
           IN UINT nSeqLen,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
	   );
SNMPAPI SnmpBerEncodeAsnAny(
           IN AsnAny *pItem,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
	   );

//
// Private Query API
//
SNMPAPI SnmpBerQueryAsnType(
           IN BYTE *pBuffer,
           IN UINT nLength
           );

//------------------------------- END ---------------------------------------

#endif /* berapi_h */

