/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    authapi.c

Abstract:

    Communications message decode/encode routines.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/
 
//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdlib.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>

#include "authxxxx.h"
#include "auth1157.h"
#include "pduapi.h"
#include "berapi.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "authapi.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// SnmpSvcEncodeMessage
//    Encodes the specified message and message type (RFC 1157 or Mgmt Com)
//    into a buffer.
//
// Notes:
//    If an error occurs, the buffer is freed and set to NULL.
//
//    The buffer information will be initialized by this routine.
//
//    It will be the responsibility of the calling routine to free the buffer
//    if the encoding is successful.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_AUTHAPI_INVALID_MSG_TYPE
//
SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcEncodeMessage(
           IN UINT snmpAuthType,        // Type of message to encode
           IN SnmpMgmtCom *snmpMgmtCom, // Message to encode
           IN OUT BYTE **pBuffer,       // Buffer to accept encoded message
           IN OUT UINT *nLength         // Length of buffer
           )

{
RFC1157Message message;
SNMPAPI        nResult;


   // Initialize buffer information
   *pBuffer = NULL;
   *nLength = 0;

   // Encode for particular message type
   switch ( snmpAuthType )
      {
      case ASN_RFCxxxx_SNMPMGMTCOM:
         // Encode SnmpMgmtCom's parts
         if ( SNMPAPI_ERROR ==
	      (nResult = SnmpEncodeMgmtCom(snmpMgmtCom, pBuffer, nLength)) )
            {
            goto Exit;
            }

         // Encode SnmpAuthMsg parts
         if ( SNMPAPI_ERROR ==
	      (nResult = SnmpEncodeAuthMsg(&snmpMgmtCom->srcParty,
	                                   pBuffer, nLength)) )
            {
            goto Exit;
            }

         // Encode Priv Msg parts
         if ( SNMPAPI_ERROR ==
              (nResult = SnmpEncodePrivMsg(&snmpMgmtCom->dstParty,
	                                   pBuffer, nLength)) )
            {
            goto Exit;
            }

	 // Reverse the buffer
	 SnmpSvcBufRevInPlace( *pBuffer, *nLength );
         break;

      case ASN_SEQUENCE:
         // convert RFC 1157 Message to a RFC xxxx SnmpMgmtCom
         if ( SNMPAPI_ERROR ==
              (nResult = SnmpMgmtComToRFC1157Message(&message, snmpMgmtCom)) )
            {
            goto Exit;
            }

         // Encode RFC 1157 Message
	 nResult = SnmpEncodeRFC1157Message( &message, pBuffer, nLength );
         break;

      default:
         // Message type unknown - error
	 nResult = SNMPAPI_ERROR;

	 SetLastError( SNMP_AUTHAPI_INVALID_MSG_TYPE );
      }

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      SnmpUtilMemFree( *pBuffer );

      *pBuffer = NULL;
      *nLength = 0;
      }

   return nResult;
} // SnmpSvcEncodeMessage



//
// SnmpSvcDecodeMessage
//    Will determine the type of message to decode (RFC 1157 or Mgmt Com)
//    and then perform the necessary steps to decode it.
//
// Notes:
//    If an error occurs, the data in the 'snmpMgmtCom' structure should not
//    be considered valid
//
//    The data in the stream buffer, 'pBuffer', is left unchanged regardless
//    of the error outcome.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_AUTHAPI_INVALID_MSG_TYPE
//
SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcDecodeMessage(
	   OUT UINT *SnmpAuthType,       // Type of message decoded
           OUT SnmpMgmtCom *snmpMgmtCom, // Result of decoding stream
           IN BYTE *pBuffer,             // Buffer containing stream to decode
           IN UINT nLength,              // Length of buffer
           IN BOOL fAuthMsg              // Authenticate message
	   )

{
SnmpPrivMsg    snmpPrivMsg;
SnmpAuthMsg    snmpAuthMsg;
RFC1157Message message;
SNMPAPI        nResult;


   // Initialize management com message structure
   snmpMgmtCom->pdu.pduValue.pdu.varBinds.list = NULL;
   snmpMgmtCom->pdu.pduValue.pdu.varBinds.len  = 0;

   // Find out message type
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerQueryAsnType(pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Save message type
   *SnmpAuthType = (UINT) nResult;

   // Decode based on message type
   switch ( nResult )
      {
      case ASN_RFCxxxx_SNMPPRIVMSG:
         // Extract Priv Msg parts
         if ( SNMPAPI_ERROR ==
              (nResult = SnmpDecodePrivMsg(&pBuffer, &nLength, &snmpPrivMsg)) )
            {
            goto Exit;
            }

         // Extract SnmpAuthMsg parts
         if ( SNMPAPI_ERROR ==
	      (nResult = SnmpDecodeAuthMsg(&snmpPrivMsg.privDst,
				           &snmpPrivMsg.privData.stream,
	                                   &snmpPrivMsg.privData.length,
				           &snmpAuthMsg)) )
            {
            goto Exit;
            }

         // Extract SnmpMgmtCom's parts
         if ( SNMPAPI_ERROR ==
	      (nResult = SnmpDecodeMgmtCom(&snmpAuthMsg.authData.stream,
                                           &snmpAuthMsg.authData.length,
	                                   snmpMgmtCom)) )
            {
            goto Exit;
            }

         break;

      case ASN_SEQUENCE:
         // process RFC 1157 Message
         if ( SNMPAPI_ERROR ==
	      (nResult = SnmpDecodeRFC1157Message(&message, pBuffer, nLength)) )
            {
            goto Exit;
            }

         // convert RFC 1157 Message to a RFC xxxx SnmpMgmtCom
         nResult = SnmpRFC1157MessageToMgmtCom( &message, snmpMgmtCom, fAuthMsg );
         break;

      default:
         // Unknow message type
	 nResult = SNMPAPI_ERROR;

	 SetLastError( SNMP_AUTHAPI_INVALID_MSG_TYPE );
         break;
        }

Exit:
   // If an error occurs, the memory is freed by a lower level decoding routine.

   return nResult;
} // SnmpSvcDecodeMessage



//
// SnmpSvcReleaseMessage
//    Releases all memory associated with a message.
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
SNMPAPI 
SNMP_FUNC_TYPE
SnmpSvcReleaseMessage(
           IN OUT SnmpMgmtCom *snmpMgmtCom // Message to release
	   )

{
   // Release source and destination OID
   SnmpUtilOidFree( &snmpMgmtCom->dstParty );
   SnmpUtilOidFree( &snmpMgmtCom->srcParty );

   // Free community if dynamic
   if ( snmpMgmtCom->community.dynamic )
      {
      SnmpUtilMemFree( snmpMgmtCom->community.stream );
      }

   // Release PDU
   return PDU_ReleaseAnyPDU( &snmpMgmtCom->pdu );
} // SnmpSvcReleaseMessage

//-------------------------------- END --------------------------------------

