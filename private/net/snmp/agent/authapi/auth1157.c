/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    auth1157.c

Abstract:

    Decode/Encode RFC 1157 Messages.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>

#include "berapi.h"
#include "pduapi.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "auth1157.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

BOOL commauth(RFC1157Message *message);

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// SnmpEncodeRFC1157Message:
//    Encodes an RFC 1157 type message or trap.
//
// Notes:
//    Buffer information must be initialized prior to calling this routine.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpEncodeRFC1157Message(
           IN RFC1157Message *message, // Message to encode into stream
           IN OUT BYTE **pBuffer,      // Buffer to accept encoded message
           IN OUT UINT *nLength        // Length of buffer
	   )

{
SNMPAPI nResult;


   // Encode PDU/TRAP
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpPduEncodeAnyPdu(&message->data, pBuffer, nLength)) )
      {
      goto Exit;
      }

    // Encode Community
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerEncodeAsnOctetStr(ASN_OCTETSTRING,
                                             &message->community,
	                                     pBuffer, nLength)) )
       {
       goto Exit;
       }

    // Encode version
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerEncodeAsnInteger(ASN_INTEGER,
                                            message->version,
	                                    pBuffer, nLength)) )
       {
       goto Exit;
       }

    // Encode the entire RFC 1157 Message as a sequence
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerEncodeAsnSequence(*nLength, pBuffer, nLength)) )
       {
       goto Exit;
       }

   // Reverse the buffer
   SnmpSvcBufRevInPlace( *pBuffer, *nLength );

Exit:
   // If an error occurs, the memory is freed by a lower level encoding routine.

   return nResult;
} // SnmpEncodeRFC1157Message



//
// SnmpDecodeRFC1157Message:
//    Decodes an RFC 1157 type message or trap.
//
// Notes:
//    Buffer information must be initialized prior to calling this routine.
//
//    If the version is invalid, no other information is decoded.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_AUTHAPI_INVALID_VERSION
//
SNMPAPI SnmpDecodeRFC1157Message( OUT RFC1157Message *message,
                                  IN BYTE *pBuffer,
                                  IN UINT nLength )

{
AsnAny  result;
BYTE    *BufPtr;
UINT    BufLen;
SNMPAPI nResult;


   // Decode RFC 1157 Message Sequence
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_SEQUENCE, &pBuffer,
                                           &nLength, &result)) )
      {
      goto Exit;
      }

   // Make copy of buffer information in the sequence
   BufPtr = result.asnValue.sequence.stream;
   BufLen = result.asnValue.sequence.length;

   // Decode version
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_INTEGER, &BufPtr,
	                                   &BufLen, &result)) )
      {
      goto Exit;
      }

   // Version must be 0
   if ( (message->version = result.asnValue.number) != 0 )
      {
      SetLastError( SNMP_AUTHAPI_INVALID_VERSION );

      nResult = SNMPAPI_ERROR;
      goto Exit;
      }

   // Decode community
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_OCTETSTRING, &BufPtr,
	                                   &BufLen, &result)) )
      {
      goto Exit;
      }

   // Non standard assignment - MS C specific
   //    This leaves the pointer to the stream pointing into pBuffer.
   //    Take caution not to destroy pBuffer until done with stream.
   message->community = result.asnValue.string;

   // Decode the PDU type
   nResult = SnmpPduDecodeAnyPdu( &message->data, &BufPtr, &BufLen );

Exit:
   // If an error occurs, the memory is freed by a lower level decoding routine.

   return nResult;
} // SnmpDecodeRFC1157Message



//
// SnmpRFC1157MessageToMgmtCom
//    Converts a RFC 1157 type message to a Management Com type message.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpRFC1157MessageToMgmtCom(
           IN RFC1157Message *message,  // RFC 1157 Message to convert
           OUT SnmpMgmtCom *snmpMgmtCom, // Resulting Management Com format
           IN BOOL fAuthMsg // whether to perform optional authentication
	   )

{
#if 0 /* future security functionality */
   // interact with snmp party mib to look for community as a private
   // auth key of a party to identify the parties
#else
   // This is a temporary operation.  In the future, the message community
   //    will be looked up to determine its assigned parties.
   snmpMgmtCom->dstParty.idLength = 0;
   snmpMgmtCom->dstParty.ids = NULL;
   snmpMgmtCom->srcParty.idLength = 0;
   snmpMgmtCom->srcParty.ids = NULL;

   // This is a temp. action that will be replaced in the future
   //    In addition, this is a non-standard copy.
   snmpMgmtCom->community = message->community;

   if (fAuthMsg && !commauth(message))
      {
      SetLastError( SNMP_AUTHAPI_TRIV_AUTH_FAILED );

      return SNMPAPI_ERROR;
      }
#endif

   // This is a non-standard copy of a structure
   snmpMgmtCom->pdu = message->data;

// Exit:
   return SNMPAPI_NOERROR;
} // SnmpRFC1157MessageToMgmtCom



//
// SnmpMgmtComToRFC1157Message
//    Converts a Management Com type message to a RFC 1157 type message.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpMgmtComToRFC1157Message(
           OUT RFC1157Message *message, // Resulting 1157 format
           IN SnmpMgmtCom *snmpMgmtCom  // Management Com message to convert
           )

{
   // This is a temporary operation.  In the future, the message community
   //    will be looked up to determine its assigned parties.
   message->version = 0;

   // This is a temp. action that will be replaced in the future
   //    In addition, this is a non-standard copy.
   message->community = snmpMgmtCom->community;

   // This is a non-standard copy of a structure
   message->data = snmpMgmtCom->pdu;

// Exit:
   return SNMPAPI_NOERROR;
} // SnmpMgmtComToRFC1157Message

//-------------------------------- END --------------------------------------

