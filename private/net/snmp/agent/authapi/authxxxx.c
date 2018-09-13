/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    authXXXX.c

Abstract:

    Decode/Encode RFC XXXX SnmpPrivMsg, SnmpAuthMsg, and SnmpMgmtCom.

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

#include "berapi.h"
#include "pduapi.h"
#include "authapi.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "authxxxx.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// SnmpEncodePrivMsg
//    Create the private message header.
//
// Notes:
//    The buffer information must be initialized prior to calling this routine.
//
//    If an error occurs, the buffer is freed and set to NULL.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//
SNMPAPI SnmpEncodePrivMsg(
           IN AsnObjectIdentifier *DestParty, // Destination party
	   IN OUT BYTE **pBuffer,             // Buffer to accept encoding
	   IN OUT UINT *nLength               // Length of buffer
	   )

{
AsnOctetString String;
int            nResult;


   // Allocate memory for temp buffer
   if ( NULL == (String.stream = SnmpUtilMemAlloc((sizeof(BYTE) * *nLength))) )
      {
      SetLastError( SNMP_MEM_ALLOC_ERROR );

      goto Exit;
      }

   //
   // To avoid this copying, the buffer could be made into a string from
   // within this routine, just for this special case.
   //

      // Reverse buffer and make copy because string encode will reverse it
      //    as it gets copied back into the buffer.
      SnmpSvcBufRevAndCpy( String.stream, *pBuffer, *nLength );
      String.length = *nLength;

      // Fool encode octet string into thinking buffer is empty
      *nLength = 0;

   // Encode auth message as implicit octet string
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnOctetStr(ASN_RFCxxxx_PRIVDATA,
                                            &String, pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode the destination
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnObjectId(ASN_OBJECTIDENTIFIER,
                                            DestParty, pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode all as implicit sequence - RFCxxxx Priv message
   nResult = SnmpBerEncodeAsnImplicitSeq( ASN_RFCxxxx_SNMPPRIVMSG, *nLength,
                                          pBuffer, nLength );

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      SnmpUtilMemFree( *pBuffer );

      *pBuffer = NULL;
      *nLength = 0;
      }

   SnmpUtilMemFree( String.stream );

   return nResult;
} // SnmpEncodePrivMsg



//
// SnmpDecodePrivMsg
//    Removes and processes the private message header.
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
SNMPAPI SnmpDecodePrivMsg(
           IN OUT BYTE **pBuffer,
	   IN OUT UINT *nLength,
           OUT SnmpPrivMsg *PrivMsg
	   )

{
AsnAny pResult;
BYTE   *BufPtr;
UINT   BufLen;
int    nResult;


   // Initialize
   PrivMsg->privDst.ids = NULL;

   // Process the sequence
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_RFCxxxx_SNMPPRIVMSG,
	                                  pBuffer, nLength, &pResult)) )
      {
      goto Exit;
      }

   // Make copy of buffer information
   BufPtr = pResult.asnValue.sequence.stream;
   BufLen = pResult.asnValue.sequence.length;

   // Decode private destination
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_OBJECTIDENTIFIER,
	                                  &BufPtr, &BufLen, &pResult)) )
      {
      goto Exit;
      }

   // This is a non-standard copy
   PrivMsg->privDst = pResult.asnValue.object;

   // Decode private Data
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_RFCxxxx_PRIVDATA,
	                                  &BufPtr, &BufLen, &pResult)) )
      {
      goto Exit;
      }

   // This is a non-standard copy
   PrivMsg->privData = pResult.asnValue.string;

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      SnmpUtilOidFree( &PrivMsg->privDst );
      }

   return nResult;
} // SnmpDecodePrivMsg



//
// SnmpEncodeAuthMsg
//    Creates the authentication message header.
//
// Notes:
//    The buffer information must be initialized prior to calling this routine.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpEncodeAuthMsg(
           IN AsnObjectIdentifier *SrcParty, // To determine how to auth.
	   IN OUT BYTE **pBuffer,            // Buffer to accept encoding
	   IN OUT UINT *nLength              // Length of buffer
	   )

{
AsnOctetString String;
int            nResult;



   // OPENISSUE - Should be checking SrcParty to determine AUTH method.
   //
   // Normally, this routine would determine the authentication method
   // and then encode the snmp mgmt com packet accordingly, but for now,
   // a NULL string is encoded to indicate that NO authentication is present.
   //
   // OPENISSUE - Following line removes a warning, remove when above is done.
      if ( SrcParty->idLength );
      String.length = 0;
      String.stream = NULL;

   // Encode authentication
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnOctetStr(ASN_OCTETSTRING,
                                            &String, pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode all as implicit sequence - RFCxxxx Auth message
   nResult = SnmpBerEncodeAsnImplicitSeq( ASN_RFCxxxx_SNMPAUTHMSG, *nLength,
                                          pBuffer, nLength );

Exit:
   // If an error occurs, the memory is freed by a lower level encoding routine.

   // May have to free the space allocated for the octet string above.

   return nResult;
} // SnmpEncodeAuthMsg



//
// SnmpDecodeAuthMsg
//    Removes and processes the authentication message header.
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
SNMPAPI SnmpDecodeAuthMsg(
	   IN AsnObjectIdentifier *privDst,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength,
           OUT SnmpAuthMsg *AuthMsg
	   )

{
AsnAny pResult;
BYTE   *BufPtr;
UINT   BufLen;
int    nResult;


   // Initialize
   AuthMsg->authInfo.asnValue.object.ids = NULL;

   // Extract auth message from sequence
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_RFCxxxx_SNMPAUTHMSG,
                                          pBuffer, nLength, &pResult)) )
      {
      goto Exit;
      }

   BufPtr = pResult.asnValue.sequence.stream;
   BufLen = pResult.asnValue.sequence.length;

   if ( SNMPAPI_ERROR == (nResult = SnmpBerQueryAsnType(BufPtr, BufLen)) )
      {
      goto Exit;
      }

   // Decode authentication info
   if ( SNMPAPI_ERROR ==
      (nResult = SnmpBerDecodeAsnStream((BYTE)nResult, &BufPtr,
                                        &BufLen, &pResult)) )
      {
      goto Exit;
      }

   // This is a non-standard copy
   AuthMsg->authInfo = pResult;

   // Decode authentication data
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_RFCxxxx_SNMPMGMTCOM,
                                          &BufPtr, &BufLen, &pResult)) )
      {
      goto Exit;
      }

   // This is a non-standard copy
   AuthMsg->authData = pResult.asnValue.sequence;

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      if ( AuthMsg->authInfo.asnType == ASN_OBJECTIDENTIFIER )
         {
         SnmpUtilOidFree( &AuthMsg->authInfo.asnValue.object );
         }

      SnmpUtilOidFree( privDst );
      }

   return nResult;
} // SnmpDecodeAuthMsg



//
// SnmpEncodeMgmtCom
//    Encode the management com message.
//
// Notes:
//    The buffer information must be initialized prior to calling this routine.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpEncodeMgmtCom(
           IN SnmpMgmtCom *MgmtCom, // Mgmt Com message to encode
           IN OUT BYTE **pBuffer,   // Buffer to accept encoding
	   IN OUT UINT *nLength     // Length of buffer
	   )

{
int  nResult;


   // Encode PDU/TRAP
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpPduEncodeAnyPdu(&MgmtCom->pdu, pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode source party
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnObjectId(ASN_OBJECTIDENTIFIER,
                                            &MgmtCom->srcParty,
	                                    pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode destination party
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnObjectId(ASN_OBJECTIDENTIFIER,
                                            &MgmtCom->dstParty,
	                                    pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode all as implicit sequence - RFCxxxx Mgmt Com
   nResult = SnmpBerEncodeAsnImplicitSeq( ASN_RFCxxxx_SNMPMGMTCOM, *nLength,
                                          pBuffer, nLength );

Exit:
   // If an error occurs, the memory is freed by a lower level encoding routine.

   return nResult;
} // SnmpEncodeMgmtCom



//
// SnmpDecodeMgmtCom
//    Processes the mgmt com message.
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
SNMPAPI SnmpDecodeMgmtCom(
           IN OUT BYTE **pBuffer,
	   IN OUT UINT *nLength,
           SnmpMgmtCom *MgmtCom
	   )

{
AsnAny pResult;
int    nResult;


   // Initialize
   MgmtCom->dstParty.ids = NULL;
   MgmtCom->srcParty.ids = NULL;

   // Decode destination party
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_OBJECTIDENTIFIER,
	                                  pBuffer, nLength, &pResult)) )
      {
      goto Exit;
      }

   // This is a non-standard copy
   MgmtCom->dstParty = pResult.asnValue.object;

   // Decode source party
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_OBJECTIDENTIFIER,
	                                  pBuffer, nLength, &pResult)) )
      {
      goto Exit;
      }

   // This is a non-standard copy
   MgmtCom->srcParty = pResult.asnValue.object;

   // Decode the PDU type
   nResult = SnmpPduDecodeAnyPdu( &MgmtCom->pdu, pBuffer, nLength );

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      SnmpUtilOidFree( &MgmtCom->dstParty );
      SnmpUtilOidFree( &MgmtCom->srcParty );
      }

   return nResult;
} // SnmpDecodeMgmtCom

//-------------------------------- END --------------------------------------

