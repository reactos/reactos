/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    pduapi.c

Abstract:

    Performs all functions to encode/decode a RFC 1157 PDU and trap.

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

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "pduapi.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

#define PDU_ERRORSTATUS_LAST   SNMP_ERRORSTATUS_GENERR
#define PDU_GENERICTRAP_LAST   SNMP_GENERICTRAP_ENTERSPECIFIC

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

void PDU_ReleasePDU(
        RFC1157Pdu *pdu // PDU to release
        );

void PDU_ReleaseTrap(
        RFC1157TrapPdu *pdu // Trap to release
	);

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// SnmpPduEncodePdu:
//    Encode a RFC1157 PDU.
//
// Notes:
//    The encoded PDU is left in reverse order so the rest of the message
//    information can be appended to the buffer, and then reversed.
//
//    The buffer information must be initialized prior to calling this routine.
//
//    If an error occurs, the entire buffer is freed, and set to NULL.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_PDUAPI_INVALID_ES
//
SNMPAPI SnmpPduEncodePdu(
	   IN BYTE nType,         // Type of RFC 1157 PDU to encode
           IN RFC1157Pdu *pdu,    // RFC 1157 PDU to encode into stream buffer
           IN OUT BYTE **pBuffer, // Stream buffer to accept encoding
	   IN OUT UINT *nLength   // Length of stream buffer
	   )

{
SNMPAPI  nResult;
UINT     nBufStart;
int      I;


   // Check for valid error status
   if ( pdu->errorStatus > PDU_ERRORSTATUS_LAST )
      {
      SetLastError( SNMP_PDUAPI_INVALID_ES );
      nResult = SNMPAPI_ERROR;

      goto Exit;
      }

   // Save start position of buffer
   nBufStart = *nLength;

   // Encode var binds
   for ( I=pdu->varBinds.len-1;I >= 0;I-- )
      {
      UINT nSeqStart;


      // Save starting point of sequence
      nSeqStart = *nLength;

      // Encode variable value
      if ( SNMPAPI_ERROR ==
           SnmpBerEncodeAsnAny(&pdu->varBinds.list[I].value,
	                       pBuffer, nLength) )
         {
	 goto Exit;
	 }

      // Encode variable name
      if ( SNMPAPI_ERROR ==
           SnmpBerEncodeAsnObjectId(ASN_OBJECTIDENTIFIER,
                                    &pdu->varBinds.list[I].name,
	                            pBuffer, nLength) )
         {
	 goto Exit;
	 }

      // Encode the entire variable info. as a sequence
      if ( SNMPAPI_ERROR ==
           SnmpBerEncodeAsnSequence(*nLength - nSeqStart,
	                            pBuffer, nLength) )
         {
	 goto Exit;
	 }
      }

   // Encode the var bind list as a sequence of 'sequence's
   if ( SNMPAPI_ERROR ==
        SnmpBerEncodeAsnImplicitSeq(ASN_SEQUENCEOF, *nLength - nBufStart,
                                    pBuffer, nLength) )
      {
      goto Exit;
      }

   // Encode error-index
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnInteger(ASN_INTEGER,
	                                   pdu->errorIndex,
                                           pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode error-status
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnInteger(ASN_INTEGER,
	                                   pdu->errorStatus,
                                           pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode request-id
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnInteger(ASN_INTEGER,
	                                   pdu->requestId,
                                           pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode entire pdu as Implicit Sequence
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnImplicitSeq(nType, *nLength - nBufStart,
                                               pBuffer, nLength)) )
      {
      goto Exit;
      }

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      SnmpUtilMemFree( *pBuffer );

      *pBuffer = NULL;
      *nLength = 0;
      }

   return nResult;
} // SnmpPduEncodePdu



//
// SnmpPduDecodePdu
//    Decode a PDU.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//    SNMP_PDUAPI_INVALID_ES
//
SNMPAPI SnmpPduDecodePdu(
           IN BYTE nType,         // Type of RFC 1157 PDU to decode
           OUT RFC1157Pdu *pdu,   // RFC 1157 PDU to accept decoding
           IN OUT BYTE **pBuffer, // Stream buffer to decode
	   IN OUT UINT *nLength   // Length of stream buffer
	   )

{
UINT    BufLen;
BYTE    *BufPtr;
AsnAny  pResult;
SNMPAPI nResult;


    // Initialize variable bindings' list
    pdu->varBinds.list = NULL;
    pdu->varBinds.len  = 0;

    //   process the sequence encapsuling the PDU
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerDecodeAsnStream(nType, pBuffer, nLength, &pResult)) )
       {
       goto Exit;
       }

    // Make copy of sequence information
    BufPtr = pResult.asnValue.sequence.stream;
    BufLen = pResult.asnValue.sequence.length;

    // Decode request-id
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerDecodeAsnStream(ASN_INTEGER, &BufPtr,
                                           &BufLen, &pResult)) )
       {
       goto Exit;
       }

    // Save request-id
    pdu->requestId = pResult.asnValue.number;

    // Decode error-status
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerDecodeAsnStream(ASN_INTEGER, &BufPtr,
                                           &BufLen, &pResult)) )
       {
       goto Exit;
       }

    // Save error-status
    pdu->errorStatus = pResult.asnValue.number;

   // Check for valid error status
   if ( pdu->errorStatus > PDU_ERRORSTATUS_LAST )
      {
      SetLastError( SNMP_PDUAPI_INVALID_ES );
      nResult = SNMPAPI_ERROR;

      goto Exit;
      }

    // Decode error-index
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerDecodeAsnStream(ASN_INTEGER, &BufPtr,
                                           &BufLen, &pResult)) )
       {
       goto Exit;
       }

    // Save error-status
    pdu->errorIndex = pResult.asnValue.number;

    // Decode variable-bindings' sequence
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerDecodeAsnStream(ASN_SEQUENCEOF, &BufPtr,
                                           &BufLen, &pResult)) )
       {
       goto Exit;
       }

    // Make temps of sequence data
    BufLen = pResult.asnValue.sequence.length;
    BufPtr = pResult.asnValue.sequence.stream;

    // Gather variables and save in pdu list
    while ( BufLen )
       {
       BYTE           *VarBufPtr;
       UINT           VarBufLen;


       // Alloc space for variable binding
       pdu->varBinds.list = (RFC1157VarBind *) SnmpUtilMemReAlloc( pdu->varBinds.list,
                            ((pdu->varBinds.len+1)*sizeof(RFC1157VarBind)) );

       // Check for errors on alloc
       if ( pdu->varBinds.list == NULL )
          {
          SetLastError( SNMP_MEM_ALLOC_ERROR );

          nResult = SNMPAPI_ERROR;
          goto Exit;
          }

       // Decode sequence of info
       if ( SNMPAPI_ERROR ==
            (nResult = SnmpBerDecodeAsnStream(ASN_SEQUENCE,
                                              &BufPtr, &BufLen, &pResult)) )
          {
          goto Exit;
          }

       // Setup variable bindings' buffer
       VarBufPtr = pResult.asnValue.sequence.stream;
       VarBufLen = pResult.asnValue.sequence.length;

       // Decode name
       if ( SNMPAPI_ERROR ==
            (nResult = SnmpBerDecodeAsnStream(ASN_OBJECTIDENTIFIER, &VarBufPtr,
                                              &VarBufLen, &pResult)) )
          {
          goto Exit;
          }

       // Save name of variable
       //    This is a non-standard copy - MS C specific
       pdu->varBinds.list[pdu->varBinds.len].name = pResult.asnValue.object;

       // Get ASN Type
       if ( SNMPAPI_ERROR ==
            (nResult = SnmpBerQueryAsnType(VarBufPtr, VarBufLen)) )
	  {
	  goto Exit;
	  }

       // Decode value
       if ( SNMPAPI_ERROR ==
            (nResult = SnmpBerDecodeAsnStream( (BYTE)nResult,
                              &VarBufPtr, &VarBufLen,
	                      &pdu->varBinds.list[pdu->varBinds.len++].value)) )
          {
          goto Exit;
          }
       }

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      PDU_ReleasePDU( pdu );
      }

   return nResult;
} // SnmpPduDecodePdu



//
// SnmpPduEncodeTrap:
//    The encoded TRAP is left in reverse order so the rest of the message
//    information can be appended to the buffer, and then reversed.
//
// Notes:
//    The buffer information must be initialized prior to calling this routine.
//
//    If an error occurs, the entire buffer is freed and set to NULL.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_PDUAPI_INVALID_GT
//
SNMPAPI SnmpPduEncodeTrap(
	   IN BYTE nType,          // Type of RFC 1157 TRAP to encode
           IN RFC1157TrapPdu *pdu, // RFC 1157 Trap to encode into stream buffer
           IN OUT BYTE **pBuffer,  // Stream buffer to accept encoding
	   IN OUT UINT *nLength    // Length of stream buffer
	   )

{
SNMPAPI  nResult;
UINT     nBufStart;
int      I;


   // Check for valid generic trap
   if ( pdu->genericTrap > PDU_GENERICTRAP_LAST )
      {
      SetLastError( SNMP_PDUAPI_INVALID_GT );
      nResult = SNMPAPI_ERROR;

      goto Exit;
      }

   // Save start position of buffer
   nBufStart = *nLength;

   // Encode var binds
   for( I=pdu->varBinds.len-1;I >= 0;I-- )
      {
      UINT nSeqStart;


      // Save starting point of sequence
      nSeqStart = *nLength;

      // Encode variable value
      if ( SNMPAPI_ERROR ==
           SnmpBerEncodeAsnAny(&pdu->varBinds.list[I].value,
	                       pBuffer, nLength) )
         {
	 goto Exit;
	 }

      // Encode variable name
      if ( SNMPAPI_ERROR ==
           SnmpBerEncodeAsnObjectId(ASN_OBJECTIDENTIFIER,
                                    &pdu->varBinds.list[I].name,
	                            pBuffer, nLength) )
         {
	 goto Exit;
	 }

      // Encode the entire variable info. as a sequence
      if ( SNMPAPI_ERROR ==
           SnmpBerEncodeAsnSequence(*nLength - nSeqStart,
	                            pBuffer, nLength) )
         {
	 goto Exit;
	 }
      }

   // Encode the var bind list as a sequence of 'sequence's
      if ( SNMPAPI_ERROR ==
           SnmpBerEncodeAsnImplicitSeq(ASN_SEQUENCEOF, *nLength - nBufStart,
	                               pBuffer, nLength) )
         {
	 goto Exit;
	 }

   // Encode time-stamp
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnInteger(ASN_RFC1155_TIMETICKS,
                                           pdu->timeStamp,
                                           pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode specific-trap
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnInteger(ASN_INTEGER,
                                           pdu->specificTrap,
                                           pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode generic-trap
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnInteger(ASN_INTEGER,
                                           pdu->genericTrap,
                                           pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode agent-addr
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnOctetStr(ASN_RFC1155_IPADDRESS,
                                            &pdu->agentAddr,
                                            pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode enterprise
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnObjectId(ASN_OBJECTIDENTIFIER,
                                            &pdu->enterprise,
                                            pBuffer, nLength)) )
      {
      goto Exit;
      }

   // Encode entire pdu as Implicit Sequence
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerEncodeAsnImplicitSeq(nType,
	                                       *nLength - nBufStart,
                                               pBuffer, nLength)) )
      {
      goto Exit;
      }

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      SnmpUtilMemFree( *pBuffer );

      *pBuffer = NULL;
      *nLength = 0;
      }

   return nResult;
} // SnmpPduEncodeTrap



//
// SnmpPduDecodeTrap:
//   Decode a TRAP.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//    SNMP_PDUAPI_INVALID_GT
//
SNMPAPI SnmpPduDecodeTrap(
	   IN BYTE nType,           // Type of RFC 1157 TRAP to decode
           OUT RFC1157TrapPdu *pdu, // RFC 1157 Trap to accept decoding
           IN OUT BYTE **pBuffer,   // Stream buffer to decode
	   IN OUT UINT *nLength     // Length of stream buffer
	   )

{
UINT    BufLen;
BYTE    *BufPtr;
AsnAny  pResult;
SNMPAPI nResult;


    // Initialize variable bindings' list
    pdu->enterprise.ids = NULL;
    pdu->varBinds.list  = NULL;
    pdu->varBinds.len   = 0;

    //   process the sequence encapsuling the PDU
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerDecodeAsnStream(nType, pBuffer, nLength, &pResult)) )
       {
       goto Exit;
       }

    // Make copy of sequence information
    BufPtr = pResult.asnValue.sequence.stream;
    BufLen = pResult.asnValue.sequence.length;

    // Decode enterprise
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerDecodeAsnStream(ASN_OBJECTIDENTIFIER, &BufPtr,
                                           &BufLen, &pResult)) )
       {
       goto Exit;
       }

   // This is a non-standard structure copy
   pdu->enterprise = pResult.asnValue.object;

    // Decode agent-addr
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerDecodeAsnStream(ASN_RFC1155_IPADDRESS, &BufPtr,
                                           &BufLen, &pResult)) )
       {
       goto Exit;
       }

   // This is a non-standard structure copy
   pdu->agentAddr = pResult.asnValue.string;

   // Decode generic-trap
   if ( SNMPAPI_ERROR ==
        (nResult = SnmpBerDecodeAsnStream(ASN_INTEGER, &BufPtr,
                                          &BufLen, &pResult)) )
      {
      goto Exit;
      }

    pdu->genericTrap = pResult.asnValue.number;

   // Check for valid generic trap
   if ( pdu->genericTrap > PDU_GENERICTRAP_LAST )
      {
      SetLastError( SNMP_PDUAPI_INVALID_GT );
      nResult = SNMPAPI_ERROR;

      goto Exit;
      }

    // Decode specific-trap
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerDecodeAsnStream(ASN_INTEGER, &BufPtr,
                                           &BufLen, &pResult)) )
       {
       goto Exit;
       }

    pdu->specificTrap = pResult.asnValue.number;

    // Decode time-stamp
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerDecodeAsnStream(ASN_RFC1155_TIMETICKS, &BufPtr,
                                           &BufLen, &pResult)) )
       {
       goto Exit;
       }

    pdu->timeStamp = pResult.asnValue.number;

    // Decode variable-bindings' sequence
    if ( SNMPAPI_ERROR ==
         (nResult = SnmpBerDecodeAsnStream(ASN_SEQUENCEOF, &BufPtr,
                                           &BufLen, &pResult)) )
       {
       goto Exit;
       }

    // Make temps of sequence data
    BufLen = pResult.asnValue.sequence.length;
    BufPtr = pResult.asnValue.sequence.stream;

    // Gather variables and save in pdu list
    while ( BufLen )
       {
       BYTE           *VarBufPtr;
       UINT           VarBufLen;


       // Alloc space for variable binding
       pdu->varBinds.list = (RFC1157VarBind *) SnmpUtilMemReAlloc( pdu->varBinds.list,
                            ((pdu->varBinds.len+1)*sizeof(RFC1157VarBind)) );

       // Check for errors on alloc
       if ( pdu->varBinds.list == NULL )
          {
          SetLastError( SNMP_MEM_ALLOC_ERROR );

          nResult = SNMPAPI_ERROR;
          goto Exit;
          }

       // Decode sequence of info
       if ( SNMPAPI_ERROR ==
            (nResult = SnmpBerDecodeAsnStream(ASN_SEQUENCE,
                                              &BufPtr, &BufLen, &pResult)) )
          {
          goto Exit;
          }

       // Setup variable bindings' buffer
       VarBufPtr = pResult.asnValue.sequence.stream;
       VarBufLen = pResult.asnValue.sequence.length;

       // Decode name
       if ( SNMPAPI_ERROR ==
            (nResult = SnmpBerDecodeAsnStream(ASN_OBJECTIDENTIFIER, &VarBufPtr,
                                              &VarBufLen, &pResult)) )
          {
          goto Exit;
          }

       // Save name of variable
       //    This is a non-standard copy - MS C specific
       pdu->varBinds.list[pdu->varBinds.len].name = pResult.asnValue.object;

       // Decode value
       if ( SNMPAPI_ERROR ==
            (nResult = SnmpBerDecodeAsnStream(
                              (BYTE)SnmpBerQueryAsnType(VarBufPtr, VarBufLen),
                              &VarBufPtr, &VarBufLen,
	                      &pdu->varBinds.list[pdu->varBinds.len++].value)) )
          {
          goto Exit;
          }
       }

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      PDU_ReleaseTrap( pdu );
      }

   return nResult;
} // SnmpPduDecodePdu



//
// SnmpPduEncodeAnyPdu
//    Determines the type of PDU to encode (PDU or TRAP) and calls the
//    appropriate routine to do it.
//
// Notes:
//    If the pdu type is unrecognized, this is an error and the buffer info.
//    is freed.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_PDUAPI_UNRECOGNIZED_PDU
//
SNMPAPI SnmpPduEncodeAnyPdu(
           RFC1157Pdus *pdu,      // PDU/TRAP to Encode
	   IN OUT BYTE **pBuffer, // Buffer to accept encoding
	   IN OUT UINT *nLength   // Length of buffer
	   )

{
SNMPAPI nResult;


    // Encode PDU/TRAP
    switch ( pdu->pduType )
        {
	case ASN_RFC1157_GETREQUEST:
	case ASN_RFC1157_GETNEXTREQUEST:
	case ASN_RFC1157_GETRESPONSE:
	case ASN_RFC1157_SETREQUEST:
            nResult = SnmpPduEncodePdu( pdu->pduType, &pdu->pduValue.pdu,
	                                pBuffer, nLength );
            break;

	case ASN_RFC1157_TRAP:
            nResult = SnmpPduEncodeTrap( pdu->pduType, &pdu->pduValue.trap,
	                                 pBuffer, nLength );
            break;

        default:
	    SetLastError( SNMP_PDUAPI_UNRECOGNIZED_PDU );

            nResult = SNMPAPI_ERROR;
        }

   if ( nResult == SNMPAPI_ERROR )
      {
      SnmpUtilMemFree( *pBuffer );

      *pBuffer = NULL;
      *nLength = 0;
      }

   return nResult;
} // SnmpPduEncodeAnyPdu



//
// SnmpPduDecodeAnyPdu
//    Determines and sets the type of PDU to decode (PDU or TRAP) and calls the
//    appropriate routine to do it.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_PDUAPI_UNRECOGNIZED_PDU
//
SNMPAPI SnmpPduDecodeAnyPdu(
	   OUT RFC1157Pdus *pdu,  // Will accept PDU or TRAP as result
           IN OUT BYTE **pBuffer, // Stream buffer to decode
	   IN OUT UINT *nLength   // Length of stream buffer
	   )

{
SNMPAPI nResult;


   // Get pdu type
   if ( SNMPAPI_ERROR ==
        (pdu->pduType = (BYTE)(nResult = SnmpBerQueryAsnType(*pBuffer, *nLength))) )
      {
      goto Exit;
      }

   switch ( nResult )
      {
      case ASN_RFC1157_GETREQUEST:
      case ASN_RFC1157_GETNEXTREQUEST:
      case ASN_RFC1157_GETRESPONSE:
      case ASN_RFC1157_SETREQUEST:
         nResult = SnmpPduDecodePdu( pdu->pduType, &pdu->pduValue.pdu,
	                             pBuffer, nLength );
         break;

      case ASN_RFC1157_TRAP:
         nResult = SnmpPduDecodeTrap( pdu->pduType, &pdu->pduValue.trap,
				      pBuffer, nLength );
         break;

      default:
	 SetLastError( SNMP_PDUAPI_UNRECOGNIZED_PDU );

         nResult = SNMPAPI_ERROR;
      }

Exit:
   return nResult;
} // SnmpPduDecodeAnyPdu



//
// PDU_ReleasePDU
//    Frees all memory associated with a pdu, including a call to
//    SnmpUtilVarBindListFree to free the var binds list.
//
// Notes:
//    The dynamic memory associated with a pdu must be set before calling
//    this routine.
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
void PDU_ReleasePDU(
        RFC1157Pdu *pdu // PDU to release
        )

{
   SnmpUtilVarBindListFree( &pdu->varBinds );
} // PDU_ReleasePDU



//
// PDU_ReleaseTrap
//    Frees all memory associated with a trap, including a call to
//    SnmpUtilVarBindListFree to free the var binds list.
//
// Notes:
//    The dynamic memory associated with a trap pdu must be set before calling
//    this routine.
//
// Return Codes:
//    None.
//
// Error Codes:
//    None.
//
void PDU_ReleaseTrap(
        RFC1157TrapPdu *pdu // Trap to release
	)

{
   SnmpUtilOidFree( &pdu->enterprise );

   // Free network address if dynamic
   if ( pdu->agentAddr.dynamic )
      {
      SnmpUtilMemFree( pdu->agentAddr.stream );
      }

   SnmpUtilVarBindListFree( &pdu->varBinds );
} // PDU_ReleaseTrap



//
// PDU_ReleaseAnyPdu
//    Determines what type of PDU it is and calls the appropriate routine
//    to release it.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_PDUAPI_UNRECOGNIZED_PDU
//
SNMPAPI PDU_ReleaseAnyPDU(
           IN OUT RFC1157Pdus *Pdu
	   )

{
SNMPAPI nResult;


   // Encode PDU/TRAP
   switch ( Pdu->pduType )
      {
      case ASN_RFC1157_GETREQUEST:
      case ASN_RFC1157_GETNEXTREQUEST:
      case ASN_RFC1157_GETRESPONSE:
      case ASN_RFC1157_SETREQUEST:
         PDU_ReleasePDU( &Pdu->pduValue.pdu );
         break;

      case ASN_RFC1157_TRAP:
         PDU_ReleaseTrap( &Pdu->pduValue.trap );
         break;

      default:
	 SetLastError( SNMP_PDUAPI_UNRECOGNIZED_PDU );

         nResult = SNMPAPI_ERROR;
	 goto Exit;
      }

   nResult = SNMPAPI_NOERROR;

Exit:
   return nResult;
} // PDU_ReleaseAnyPDU

//-------------------------------- END --------------------------------------

