/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    berapi.c

Abstract:

    ASN.1 BER Encode/Decode APIs

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

#include "authapi.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

#include "berapi.h"

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

   // BER limits
#define BER_OCTET_LEN         8      // 8 bits in an octet
#define BER_OCTET_SIZE        256    // 8 bits make an octet
#define BER_MAX_INT_OCTETS    4      // 4 octets - 32 bit integer
#define BER_MAX_LEN_OCTETS    2      // 2 octets - 16 bit integer length
#define BER_MAX_STREAM_LEN    0xffff // 16 bit unsigned integer
#define BER_MIN_HEADER_LEN    2      // 2 octets
#define BER_MAX_SIMPLE_LEN    127    // SNMP BER definition

   // Buffer offsets for info.
#define BER_TAG_OFFSET        0
#define BER_LENGTH_OFFSET     1

   // Meaningful bit definitions
#define BER_EXTENDED_TAG      0x1f   // 00011111
#define BER_OCTET_CONT_BIT    0x80   // 10000000

   // BER OBJECTIDENTIFIER limits
#define BER_MAX_FIRST_ELEM    2      // Obj Id's 1st element must be 0-2
#define BER_MAX_SECOND_ELEM   39     // Obj Id's 2nd element must be 0-40
#define BER_SUBOID_BLK_SIZE   20     // Number of Sub OID's to alloc at once.


//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE MACROS --------------------------------

   // This macro requires that a label named 'Exit' exists in the function
   //    and handles any cleanup procedures the function needs to do
   //    before exiting.
   #define BERAPI_ERROR( X ) \
          SetLastError( X ); \
          nResult = SNMPAPI_ERROR; \
          goto Exit \

//--------------------------- PRIVATE PROTOTYPES ----------------------------

SNMPAPI BER_CreateHeader(
           IN BYTE nTag,
           IN UINT nObjLen,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nStart
           );

SNMPAPI BER_ProcessHeader(
           IN BYTE nTag,
           IN BYTE *BufPtr,
           IN UINT BufLen,
           OUT UINT *StrLen,
           OUT UINT *HeadLen
           );

SNMPAPI BER_DecodeAsnInteger(
           IN BYTE nTag,          // Expect type of integer
           IN OUT BYTE **pBuffer, // Buffer to decode
           IN OUT UINT *nLength,  // Length of buffer
           IN OUT AsnAny *pResult // Contains decoded integer and specific type
           );

SNMPAPI BER_DecodeAsnOctetStr(
           IN BYTE nTag,          // Expect type of OctetStr
           IN OUT BYTE **pBuffer, // Buffer to decode
           IN OUT UINT *nLength,  // Length of buffer
           IN OUT AsnAny *pResult // Contains decoded OctetStr and specific type
           );

SNMPAPI BER_DecodeAsnNull(
           IN BYTE nTag,          // Expect type of NULL
           IN OUT BYTE **pBuffer, // Buffer to decode
           IN OUT UINT *nLength,  // Length of buffer
           IN OUT AsnAny *pResult // Contains decoded NULL and specific type
           );

SNMPAPI BER_DecodeAsnObjectId(
           IN BYTE nTag,          // Expect type of ObjectId
           IN OUT BYTE **pBuffer, // Buffer to decode
           IN OUT UINT *nLength,  // Length of buffer
           IN OUT AsnAny *pResult // Contains decoded ObjectId and specific type
           );

SNMPAPI BER_DecodeAsnImplicitSeq(
           IN BYTE nTag,          // Expected type of sequence
           IN OUT BYTE **pBuffer, // Buffer to decode
           IN OUT UINT *nLength,  // Length of buffer
           IN OUT AsnAny *pResult // Contains decoded sequence and specific type
           );

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// SnmpBerDecodeAsnStream
//    Will decode next object in stream, based on expected type.
//
// Notes:
//    If expected type does not match what is in buffer, an error is generated.
//
//    "Extended Tags" (tags requiring more than one octet) are invalid.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_BERAPI_INVALID_TAG
//
SNMPAPI SnmpBerDecodeAsnStream(
           IN BYTE idExpectedAsnType, // Expected type of ASN object
           IN OUT BYTE **pBuffer,     // Buffer to decode
           IN OUT UINT *nLength,      // Length of buffer
           OUT AsnAny *pResult        // Contains decoded object and type
           )

{
SNMPAPI nResult;

    // Call appropriate routine to convert ASN.1 type
    switch ( idExpectedAsnType )
       {
       case ASN_RFC1155_COUNTER:
       case ASN_RFC1155_GAUGE:
       case ASN_RFC1155_TIMETICKS:
       case ASN_INTEGER:
          nResult = BER_DecodeAsnInteger( idExpectedAsnType,
                                          pBuffer, nLength, pResult );
          break;

       case ASN_RFC1155_IPADDRESS:
       case ASN_RFC1155_OPAQUE:
       case ASN_OCTETSTRING:
          nResult = BER_DecodeAsnOctetStr( idExpectedAsnType,
                                           pBuffer, nLength, pResult );
          break;

       case ASN_NULL:
          nResult = BER_DecodeAsnNull( idExpectedAsnType,
                                       pBuffer, nLength, pResult );
          break;

       case ASN_OBJECTIDENTIFIER:
          nResult = BER_DecodeAsnObjectId( idExpectedAsnType,
                                           pBuffer, nLength, pResult );
          break;

       case ASN_SEQUENCE:
       case ASN_RFC1157_GETREQUEST:
       case ASN_RFC1157_GETNEXTREQUEST:
       case ASN_RFC1157_GETRESPONSE:
       case ASN_RFC1157_SETREQUEST:
       case ASN_RFC1157_TRAP:
       case ASN_RFCxxxx_PRIVDATA:
          nResult = BER_DecodeAsnImplicitSeq( idExpectedAsnType,
                                       pBuffer, nLength, pResult );
          break;

       default:
          BERAPI_ERROR( SNMP_BERAPI_INVALID_TAG );
          SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: BER: Invalid tag encountered.\n"
            ));
       }

Exit:
    return nResult;
} // end SnmpBerDecodeAsnStream()



//
// SnmpBerEncodeAsnAny
//    Encodes any ASN type, except sequences, implicit or normal.
//
// Notes:
//    "Extended Tags" are invalid.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_BERAPI_INVALID_TAG
//
SNMPAPI SnmpBerEncodeAsnAny(
           IN AsnAny *pItem,
           IN OUT BYTE   **pBuffer,
           IN OUT UINT   *nLength
           )

{
SNMPAPI nResult;


    // Call appropriate routine to convert ASN.1 type
    switch ( pItem->asnType )
       {
       case ASN_RFC1155_COUNTER:
       case ASN_RFC1155_GAUGE:
       case ASN_RFC1155_TIMETICKS:
       case ASN_INTEGER:
          nResult = SnmpBerEncodeAsnInteger( pItem->asnType,
                                             pItem->asnValue.number,
                                             pBuffer, nLength );
          break;

       case ASN_RFC1155_IPADDRESS:
       case ASN_RFC1155_OPAQUE:
       case ASN_OCTETSTRING:
          nResult = SnmpBerEncodeAsnOctetStr( pItem->asnType,
                                              &pItem->asnValue.string,
                                              pBuffer, nLength );
          break;

       case ASN_NULL:
          nResult = SnmpBerEncodeAsnNull( pItem->asnType, pBuffer, nLength );
          break;

       case ASN_OBJECTIDENTIFIER:
          nResult = SnmpBerEncodeAsnObjectId( pItem->asnType,
                                              &pItem->asnValue.object,
                                              pBuffer, nLength );
          break;


       default:
          BERAPI_ERROR( SNMP_BERAPI_INVALID_TAG );
          SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: BER: Invalid tag encountered.\n"
            ));
       }

Exit:
    return nResult;
} // SnmpBerEncodeAsnAny



//
// BER_CreateHeader
//
// Notes:
//    If an error occurs during this routine, it is the responsibility of
//    the calling routine to free any memory that may or may not have
//    been realloc'ed by BER_CreateHeader.
//
//    The buffer information must be initialized before calling this routine.
//    If this is the first routine to alloc memory for the buffer, it must be
//    initialized to NULL.
//
SNMPAPI BER_CreateHeader(
           IN BYTE nTag,
           IN UINT nObjLen,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nStart
           )

{
UINT     nHeadLen;
UINT     nStrLen;
BYTE     nExtLen;
SNMPAPI  nResult;
UINT     tLen;


   // Check for extended length
   nExtLen = 0;
   if ( nObjLen > BER_MAX_SIMPLE_LEN ) {
      tLen = nObjLen;

      while (tLen > 0) {
          nExtLen++;
          tLen /= BER_OCTET_SIZE;
      }
   }

   // Calculate the length of the header
   nHeadLen = BER_MIN_HEADER_LEN + nExtLen;

   // Re-Alloc the buffer to hold the header
   *pBuffer = SnmpUtilMemReAlloc( *pBuffer, (*nStart + nObjLen + nHeadLen) );
   if ( *pBuffer == NULL )
      {
      BERAPI_ERROR( SNMP_MEM_ALLOC_ERROR );
      SNMPDBG((
        SNMP_LOG_ERROR,
        "SNMP: BER: Out of memory.\n"
        ));
      }

   // Save start position of buffer
   nStrLen = *nStart + nObjLen;

   // Adjust overall length of buffer
   *nStart += nObjLen + nHeadLen;

   // Put Tag at end of buffer
   (*pBuffer)[nStrLen+nHeadLen-BER_TAG_OFFSET-1] = nTag;

   // Place Extended length indicator in stream if needed
   if ( nExtLen > 0 )
      {
      (*pBuffer)[nStrLen+nHeadLen-BER_LENGTH_OFFSET-1] =
                                    (BYTE) (nExtLen | BER_OCTET_CONT_BIT);
      }

   // Put stream length in buffer (reverse order)
   (*pBuffer)[nStrLen] = 0;
   while ( nObjLen > 0 )
      {
      (*pBuffer)[nStrLen++] = (BYTE) (nObjLen % BER_OCTET_SIZE);
      nObjLen /= BER_OCTET_SIZE;
      }

   nResult = SNMPAPI_NOERROR;

Exit:
   // Memory is released if 'realloc' fails, leaving buffer pointing to NULL.

   return nResult;
} // CreateHeader



//
// BER_ProcessHeader:
//     Calculates the length of the ASN.1 object.  Does not change any
//     buffer information.
//
// Notes:
//     If an error occurs, the values of StrLen and HeadLen are
//     invalid.
//
// Return codes:
//     SNMPAPI_NOERROR
//     SNMPAPI_ERROR
//
// Error codes:
//     SNMP_BERAPI_INVALID_TAG
//     SNMP_BERAPI_OVERFLOW
//     SNMP_BERAPI_SHORT_BUFFER
//
//
//
SNMPAPI BER_ProcessHeader(
           IN BYTE nTag,
           IN BYTE *BufPtr,
           IN UINT BufLen,
           OUT UINT *StrLen,
           OUT UINT *HeadLen
           )

{
UINT     ExtLen;
UINT     I;
SNMPAPI  nResult;


    // Check for short buffer
    if ( BufLen < BER_MIN_HEADER_LEN )
       {
       BERAPI_ERROR( SNMP_BERAPI_SHORT_BUFFER );
       SNMPDBG((
         SNMP_LOG_ERROR,
         "SNMP: BER: Short buffer encountered.\n"
         ));
       }

    // Check for correct tag
    if ( BufPtr[BER_TAG_OFFSET] != nTag )
       {
       BERAPI_ERROR( SNMP_BERAPI_INVALID_TAG );
       SNMPDBG((
         SNMP_LOG_ERROR,
         "SNMP: BER: Invalid tag encountered.\n"
         ));
       }

    // Check for extended 'length' octets
    *StrLen = BufPtr[BER_LENGTH_OFFSET] & ~BER_OCTET_CONT_BIT;

    // if extended length, calculate length
    if ( BufPtr[BER_LENGTH_OFFSET] & BER_OCTET_CONT_BIT )
       {
       ExtLen  = *StrLen;
       *StrLen = 0;

       // Check for short buffer
       if ( BufLen < (UINT)BER_MIN_HEADER_LEN+ExtLen )
          {
          BERAPI_ERROR( SNMP_BERAPI_SHORT_BUFFER );
          SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: BER: Short buffer encountered.\n"
            ));
          }

       // Check for string length overflow
       if ( ExtLen > BER_MAX_LEN_OCTETS )
          {
          BERAPI_ERROR( SNMP_BERAPI_OVERFLOW );
          SNMPDBG((
            SNMP_LOG_ERROR,
            "SNMP: BER: Overflow encountered.\n"
            ));
          }

       // Build length
       for ( I=0;I < ExtLen;I++ )
          {
          *StrLen = *StrLen * BER_OCTET_SIZE + BufPtr[I+BER_MIN_HEADER_LEN];
          }
       }
    else
       {
       ExtLen = 0;
       }

    // Calculate header length
    *HeadLen = BER_MIN_HEADER_LEN + ExtLen;

    // Check for short buffer
    if ( (ULONG)BufLen < (ULONG)*HeadLen + (ULONG)*StrLen )
       {
       BERAPI_ERROR( SNMP_BERAPI_SHORT_BUFFER );
       SNMPDBG((
         SNMP_LOG_ERROR,
         "SNMP: BER: Short buffer encountered.\n"
         ));
       }

    // Signal success
    nResult = SNMPAPI_NOERROR;

Exit:
    return nResult;
} // BER_ProcessHeader



//
// SnmpBerEncodeAsnInteger
//
// Notes:
//    If an error occurs during this routine, it is the responsibility of
//    the calling routine to free any memory that may or may not have
//    been realloc'ed by this routine.
//
//    The buffer information must be initialized before calling this routine.
//    If this is the first routine to alloc memory for the buffer, it must be
//    initialized to NULL.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//
SNMPAPI SnmpBerEncodeAsnInteger(
           IN BYTE nTag,
           IN AsnInteger lInteger,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
           )

{
BYTE      *pTemp;
UINT      nIntLen;
ULONG     ULTemp;
UINT      I;
SNMPAPI   nResult;


   ULTemp = (ULONG) lInteger;

   switch ( nTag )
      {
      case ASN_RFC1155_COUNTER:
      case ASN_RFC1155_GAUGE:
      case ASN_RFC1155_TIMETICKS:
         if ( (ULONG)0x80 > (ULONG)lInteger )
            {
            nIntLen = 1;
            }
         else if ( (ULONG)0x8000 > (ULONG)lInteger )
            {
            nIntLen = 2;
            }
         else if ( 0x800000 > (ULONG)lInteger )
            {
            nIntLen = 3;
            }
         else if ( 0x80000000 > (ULONG)lInteger )
            {
            nIntLen = 4;
            }
         else
            {
            nIntLen = 5;
            }
         break;

      default:
         if ( (AsnInteger)0 > lInteger )
            {
            if ( (ULONG)0x80 >= -lInteger )
               {
               nIntLen = 1;
               }
            else if ( (ULONG)0x8000 >= -lInteger )
               {
               nIntLen = 2;
               }
            else if ( (ULONG)0x800000 >= -lInteger )
               {
               nIntLen = 3;
               }
            else
               {
               nIntLen = 4;
               }
            }
         else
            {
            if ( (ULONG)0x80 > lInteger )
               {
               nIntLen = 1;
               }
            else if ( (ULONG)0x8000 > lInteger )
               {
               nIntLen = 2;
               }
            else if ( (ULONG)0x800000 > lInteger )
               {
               nIntLen = 3;
               }
            else
               {
               nIntLen = 4;
               }
            }
         } // case

   // Alloc maximum memory for stream.  BER_CreateHeader will adjust to fit
   *pBuffer = SnmpUtilMemReAlloc( *pBuffer, (*nLength + nIntLen + BER_MIN_HEADER_LEN) );
   if ( *pBuffer == NULL )
      {
      BERAPI_ERROR( SNMP_MEM_ALLOC_ERROR );
      SNMPDBG((
        SNMP_LOG_ERROR,
        "SNMP: BER: Out of memory.\n"
        ));
      }

   // Alias the start position in buffer
   pTemp = *pBuffer + *nLength;

   // Encode the ASN integer
   for ( I=0;I < min(nIntLen, BER_MAX_INT_OCTETS);I++ )
      {
      pTemp[I] = (BYTE) (ULTemp % BER_OCTET_SIZE);
      ULTemp = ULTemp / BER_OCTET_SIZE;
      }

   // Check for the necessary fifth bit on unsigned integers
   if ( nIntLen > BER_MAX_INT_OCTETS )
      {
      pTemp[BER_MAX_INT_OCTETS] = 0;
      }

   // Create header
   nResult = BER_CreateHeader( nTag, nIntLen, pBuffer, nLength );

Exit:
   // Memory is released if 'realloc' fails, leaving buffer pointing to NULL.

   return nResult;
} // SnmpBerEncodeAsnInteger



//
// BER_DecodeAsnInteger:
//     Decodes an integer.  If the integer cannot be decoded, an error message
//     is returned and the buffer information remains unchanged.
//
// Notes:  Considers integers over 32 bits (4 Octets) as an error.
//
// Return codes:
//     SNMPAPI_NOERROR
//     SNMPAPI_ERROR
//
// Error codes:
//     SNMP_BERAPI_INVALID_TAG
//     SNMP_BERAPI_OVERFLOW
//     SNMP_BERAPI_SHORT_BUFFER
//
SNMPAPI BER_DecodeAsnInteger(
           IN BYTE nTag,          // Expect type of integer
           IN OUT BYTE **pBuffer, // Buffer to decode
           IN OUT UINT *nLength,  // Length of buffer
           IN OUT AsnAny *pResult // Contains decoded integer and specific type
           )

{
UINT       StrLen;
UINT       HeadLen;
UINT       I;
AsnInteger IntResult;
SNMPAPI    nResult;


    // Process header of ASN.1 INTEGER
    nResult = BER_ProcessHeader( nTag,  *pBuffer, *nLength, &StrLen, &HeadLen );
    if ( nResult != SNMPAPI_NOERROR )
       {
       goto Exit;
       }

   // Check length based on signed or unsigned expected
   switch( nTag )
      {
      case ASN_RFC1155_COUNTER:
      case ASN_RFC1155_GAUGE:
      case ASN_RFC1155_TIMETICKS:
         if ( StrLen > BER_MAX_INT_OCTETS+1 )
            {
            BERAPI_ERROR( SNMP_BERAPI_OVERFLOW );
            SNMPDBG((
              SNMP_LOG_ERROR,
              "SNMP: BER: Overflow encountered.\n"
              ));
            }
         break;

      default:
         if ( StrLen > BER_MAX_INT_OCTETS )
            {
            BERAPI_ERROR( SNMP_BERAPI_OVERFLOW );
            SNMPDBG((
              SNMP_LOG_ERROR,
              "SNMP: BER: Overflow encountered.\n"
              ));
            }
         break;
      }

    // Save sign information
    if ( (*pBuffer)[HeadLen] & 0x80 )
       {
       IntResult = -1;
       }
    else
       {
       IntResult = 0;
       }

    // Convert integer
    for ( I=0;I < StrLen;I++ )
       {
       IntResult = IntResult << BER_OCTET_LEN | (*pBuffer)[HeadLen+I];
       }

    // Extend sign if necessary
    // Assign completed conversion to return structure
    pResult->asnType          = nTag;
    pResult->asnValue.number  = IntResult;

    // Adjust buffer info
    *nLength -= HeadLen + StrLen;
    *pBuffer += HeadLen + StrLen;

    // Signal successful conversion
    nResult = SNMPAPI_NOERROR;

Exit:
    return nResult;
} // BER_DecodeAsnInteger



//
// SnmpBerEncodeAsnOctetStr
//
// Notes:
//    If an error occurs during this routine, it is the responsibility of
//    the calling routine to free any memory that may or may not have
//    been realloc'ed by this routine.
//
//    The buffer information must be initialized before calling this routine.
//    If this is the first routine to alloc memory for the buffer, it must be
//    initialized to NULL.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//
SNMPAPI SnmpBerEncodeAsnOctetStr(
           IN BYTE nTag,
           IN AsnOctetString *String,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
           )

{
BYTE     *pTemp;
SNMPAPI  nResult;


   // Alloc maximum memory for stream.  BER_CreateHeader will adjust to fit
   *pBuffer = SnmpUtilMemReAlloc( *pBuffer,
                       (*nLength + String->length + BER_MIN_HEADER_LEN) );
   if ( *pBuffer == NULL )
      {
      BERAPI_ERROR( SNMP_MEM_ALLOC_ERROR );
      SNMPDBG((
        SNMP_LOG_ERROR,
        "SNMP: BER: Out of memory.\n"
        ));
      }

   // Alias the start position in buffer
   pTemp = *pBuffer + *nLength;

   // Encode stream (reverse order)
   SnmpSvcBufRevAndCpy( pTemp, String->stream, String->length );

   // Create header
   nResult = BER_CreateHeader( nTag, String->length, pBuffer, nLength );

Exit:
   // Memory is released if 'realloc' fails, leaving buffer pointing to NULL.

   return nResult;
} // SnmpBerEncodeAsnOctetStr



//
// BER_DecodeAsnOctetStr:
//     Decodes an octet string.  If the octet string cannot be decoded, an
//     error message is returned and the buffer information remains unchanged.
//     Returns a pointer into the message buffer which points to the
//     octet string.  This routine does not allocate any memory to hold the
//     octet string.
//
// Notes:  Considers octet strings longer than 65535 as an an error.
//
// Return codes:
//     SNMPAPI_NOERROR
//     SNMPAPI_ERROR
//
// Error codes:
//     SNMP_BERAPI_INVALID_TAG
//     SNMP_BERAPI_OVERFLOW
//     SNMP_BERAPI_SHORT_BUFFER
//
SNMPAPI BER_DecodeAsnOctetStr(
           IN BYTE nTag,          // Expect type of OctetStr
           IN OUT BYTE **pBuffer, // Buffer to decode
           IN OUT UINT *nLength,  // Length of buffer
           IN OUT AsnAny *pResult // Contains decoded OctetStr and specific type
           )

{
UINT     StrLen;
UINT     HeadLen;
SNMPAPI  nResult;


    // Process header of ASN.1 OCTET STRING
    nResult = BER_ProcessHeader( nTag,  *pBuffer, *nLength, &StrLen, &HeadLen );
    if ( nResult == SNMPAPI_ERROR )
       {
       goto Exit;
       }

    // Set result type
    pResult->asnType = nTag;

    // Set return values
    pResult->asnValue.string.length  = StrLen;
    pResult->asnValue.string.stream  = *pBuffer + HeadLen;
    pResult->asnValue.string.dynamic = FALSE;

    // Adjust buffer info
    *nLength -= HeadLen + StrLen;
    *pBuffer += HeadLen + StrLen;

    // Signal successful conversion
    nResult = SNMPAPI_NOERROR;

Exit:
   return nResult;
} // BER_DecodeAsnOctetStr



//
// SnmpBerEncodeAsnNull
//    Encodes an ASN NULL into the buffer.
//
// Notes:
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//
SNMPAPI SnmpBerEncodeAsnNull(
           IN BYTE nTag,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
           )

{
   // Create header
   return BER_CreateHeader( nTag, 0, pBuffer, nLength );
} // SnmpBerEncodeAsnNull



//
// BER_DecodeAsnNull:
//     Decodes an NULL.  If the NULL cannot be decoded, an error message is
//     returned and the buffer information remains unchanged.
//
// Return codes:
//     SNMPAPI_NOERROR
//     SNMPAPI_ERROR
//
// Error codes:
//     SNMP_BERAPI_INVALID_TAG
//     SNMP_BERAPI_OVERFLOW
//     SNMP_BERAPI_SHORT_BUFFER
//     SNMP_BERAPI_INVALID_LENGTH
//
//
//
SNMPAPI BER_DecodeAsnNull(
           IN BYTE nTag,          // Expect type of NULL
           IN OUT BYTE **pBuffer, // Buffer to decode
           IN OUT UINT *nLength,  // Length of buffer
           IN OUT AsnAny *pResult // Contains decoded NULL and specific type
           )

{
UINT    StrLen;
UINT    HeadLen;
SNMPAPI nResult;


    // Get length of ASN.1 NULL
    nResult = BER_ProcessHeader( nTag, *pBuffer, *nLength, &StrLen, &HeadLen );
    if ( nResult == SNMPAPI_ERROR )
       {
       goto Exit;
       }

    // Check for correct length
    if ( StrLen != 0 )
       {
       BERAPI_ERROR( SNMP_BERAPI_INVALID_LENGTH );
       SNMPDBG((
         SNMP_LOG_ERROR,
         "SNMP: BER: Invalid length encountered.\n"
         ));
       }

    // Set return type
    pResult->asnType = nTag;

    // Adjust buffer info
    *nLength -= HeadLen + StrLen;
    *pBuffer += HeadLen + StrLen;

    // Signal successful conversion
    nResult = SNMPAPI_NOERROR;

Exit:
    return nResult;
} // BER_DecodeAsnNull



//
// SnmpBerEncodeAsnObjectId
//
// Notes:
//    If an error occurs during this routine, it is the responsibility of
//    the calling routine to free any memory that may or may not have
//    been realloc'ed by this routine.
//
//    The buffer information must be initialized before calling this routine.
//    If this is the first routine to alloc memory for the buffer, it must be
//    initialized to NULL.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    SNMP_MEM_ALLOC_ERROR
//    SNMP_BERAPI_INVALID_OBJELEM
//
SNMPAPI SnmpBerEncodeAsnObjectId(
           IN BYTE nTag,
           IN AsnObjectIdentifier *ObjectId,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
           )

{
UINT     nObjIdLen;
UINT     nTemp;
BYTE     *pTemp;
UINT     I;
SNMPAPI  nResult;


   // Check for error on number of SUB ID's
   if ( ObjectId->idLength < 2 )
      {
      BERAPI_ERROR( SNMP_BERAPI_INVALID_LENGTH );
      SNMPDBG((
        SNMP_LOG_ERROR,
        "SNMP: BER: Invalid length encountered.\n"
        ));
      }

   // Alloc maximum memory for stream.  BER_CreateHeader will adjust to fit
   *pBuffer = SnmpUtilMemReAlloc( *pBuffer,
                       (*nLength + ObjectId->idLength*3 + BER_MIN_HEADER_LEN) );
   if ( *pBuffer == NULL )
      {
      BERAPI_ERROR( SNMP_MEM_ALLOC_ERROR );
      SNMPDBG((
        SNMP_LOG_ERROR,
        "SNMP: BER: Out of memory.\n"
        ));
      }

   // Alias start position in buffer
   pTemp = *pBuffer + *nLength;

   // Encode stream (reverse order) and calculate length
   nObjIdLen = 0;
   I         = ObjectId->idLength;
   while ( I - 2 > 0 )
      {
      nTemp = ObjectId->ids[--I];
      if ( nTemp )
         {
         pTemp[nObjIdLen] = 0;
         while ( nTemp )
            {
            pTemp[nObjIdLen++] |= nTemp % BER_OCTET_CONT_BIT;
            if ( nTemp /= BER_OCTET_CONT_BIT )
               {
               pTemp[nObjIdLen] = BER_OCTET_CONT_BIT;
               }
            }
         }
      else
         {
         pTemp[nObjIdLen++] = 0;
         }
      }

   // Check for invalid first and second element
   if ( (ObjectId->ids[0] > BER_MAX_FIRST_ELEM) ||
        (ObjectId->idLength > 1 && ObjectId->ids[1] > BER_MAX_SECOND_ELEM) )
      {
      BERAPI_ERROR( SNMP_BERAPI_INVALID_OBJELEM );
      SNMPDBG((
        SNMP_LOG_ERROR,
        "SNMP: BER: Invalid objelem encountered.\n"
        ));
      }

   // Setup the first two elements of the object ID
   pTemp[nObjIdLen++] = (BYTE) (ObjectId->ids[0] * 40 + ObjectId->ids[1]);

   // Create header
   nResult = BER_CreateHeader( nTag, nObjIdLen, pBuffer, nLength );

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      SnmpUtilMemFree( *pBuffer );

      *pBuffer = NULL;
      *nLength = 0;
      }

   return nResult;
} // SnmpBerEncodeAsnObjectId


#define MAX_SNMPOID             0XFFFFFFFF

//
// BER_DecodeAsnObjectId:
//     Decodes an object identifier.  If the object identifier cannot be
//     decoded, an error message is returned and the buffer information
//     remains unchanged.
//
// Notes:  Considers object identifiers that have over 65535 elements
//         as an error.
//
// Return codes:
//     SNMPAPI_NOERROR
//     SNMPAPI_ERROR
//
// Error codes:
//     SNMP_BERAPI_INVALID_TAG
//     SNMP_BERAPI_OVERFLOW
//     SNMP_BERAPI_SHORT_BUFFER
//     SNMP_BERAPI_INVALID_LENGTH
//
SNMPAPI BER_DecodeAsnObjectId(
           IN BYTE nTag,          // Expect type of ObjectId
           IN OUT BYTE **pBuffer, // Buffer to decode
           IN OUT UINT *nLength,  // Length of buffer
           IN OUT AsnAny *pResult // Contains decoded ObjectId and specific type
           )

{
UINT     StrLen;
UINT     HeadLen;
UINT     I;
UINT     *TempObjId;
UINT     ElemTemp;
UINT     nLen;
SNMPAPI  nResult;


    // Initialize
    TempObjId = pResult->asnValue.object.ids = NULL;

    // Get length of ASN.1 OBJECT IDENTIFIER
    nResult = BER_ProcessHeader( nTag, *pBuffer, *nLength, &StrLen, &HeadLen );
    if ( nResult == SNMPAPI_ERROR )
       {
       goto Exit;
       }

    // Check for invalid length
    if ( StrLen < 1 )
       {
       BERAPI_ERROR( SNMP_BERAPI_INVALID_LENGTH );
       SNMPDBG((
         SNMP_LOG_ERROR,
         "SNMP: BER: Invalid length encountered.\n"
         ));
       }

    // Alloc space for at least 2 elements
    if ( NULL == (TempObjId = SnmpUtilMemAlloc((BER_SUBOID_BLK_SIZE*sizeof(UINT)))) )
       {
       BERAPI_ERROR( SNMP_MEM_ALLOC_ERROR );
       SNMPDBG((
         SNMP_LOG_ERROR,
         "SNMP: BER: Out of memory.\n"
         ));
       }

    // Get first two elements of the object identifier
    I            = HeadLen;
    TempObjId[0] = (*pBuffer)[I] / 40;
    TempObjId[1] = (*pBuffer)[I] % 40;
    nLen         = 2;

    // Check first element for error
    if ( TempObjId[0] > BER_MAX_FIRST_ELEM )
       {
       BERAPI_ERROR( SNMP_BERAPI_INVALID_OBJELEM );
       SNMPDBG((
         SNMP_LOG_ERROR,
         "SNMP: BER: Invalid objelem encountered.\n"
         ));
       }

    // Get rest of elements
    I++;
    while ( I < (UINT)HeadLen+StrLen && nLen < SNMP_MAX_OID_LEN )
       {
       ElemTemp = 0;
       do
         {
         ElemTemp = (ElemTemp << 7) | ((*pBuffer)[I] & ~BER_OCTET_CONT_BIT);
         ++I;

         // Check for invalid length
          if ( I > StrLen + HeadLen )
             {
             BERAPI_ERROR( SNMP_BERAPI_INVALID_LENGTH );
             SNMPDBG((   
               SNMP_LOG_ERROR,
               "SNMP: BER: Invalid length encountered.\n"
               ));
             }

         } while((*pBuffer)[I - 1] & BER_OCTET_CONT_BIT);

        // Check for 32 bit overflow
        if ( (ElemTemp > MAX_SNMPOID) )
           {
           BERAPI_ERROR( SNMP_BERAPI_OVERFLOW );
           SNMPDBG((   
             SNMP_LOG_ERROR,
             "SNMP: BER: Overflow encountered.\n"
             ));
           }

#if 0
       ElemTemp = (*pBuffer)[I] & ~BER_OCTET_CONT_BIT;

       // Check for 16 bit element
       if ( (*pBuffer)[I++] & BER_OCTET_CONT_BIT )
          {
          // Check for invalid length
          if ( I == StrLen + HeadLen )
             {
             BERAPI_ERROR( SNMP_BERAPI_INVALID_LENGTH );
             SNMPDBG((   
               SNMP_LOG_ERROR,
               "SNMP: BER: Invalid length encountered.\n"
               ));
             }

          // Check for 16 bit overflow
          if ( (*pBuffer)[I] & BER_OCTET_CONT_BIT )
             {
             BERAPI_ERROR( SNMP_BERAPI_OVERFLOW );
             SNMPDBG((   
               SNMP_LOG_ERROR,
               "SNMP: BER: Overflow encountered.\n"
               ));
             }

          // Build element
          ElemTemp = ElemTemp * BER_OCTET_CONT_BIT + (*pBuffer)[I++];
          }
#endif



       // alloc memory for object ids
       if ( !(nLen % BER_SUBOID_BLK_SIZE) )
          {
          if ( NULL ==
               (TempObjId = SnmpUtilMemReAlloc(TempObjId,
                                    ((UINT)(nLen+BER_SUBOID_BLK_SIZE) *
                                    sizeof(UINT)))) )
             {
             BERAPI_ERROR( SNMP_MEM_ALLOC_ERROR );
             SNMPDBG((   
               SNMP_LOG_ERROR,
               "SNMP: BER: Out of memory.\n"
               ));
             }
          }

       // Assign element to the object id struct
       TempObjId[nLen++] = ElemTemp;
       }

    // Check for object id too long - overflow
    if ( I < (UINT)StrLen+HeadLen )
       {
       BERAPI_ERROR( SNMP_BERAPI_OVERFLOW );
       SNMPDBG((   
         SNMP_LOG_ERROR,
         "SNMP: BER: Overflow encountered.\n"
         ));
       }

    // Set result type
    pResult->asnType = nTag;

    // Set result - Realloc to perfect fit
    pResult->asnValue.object.ids      = SnmpUtilMemReAlloc( TempObjId,
                                                 (nLen * sizeof(UINT)) );
    pResult->asnValue.object.idLength = nLen;

    // Adjust buffer info
    *nLength -= HeadLen + StrLen;
    *pBuffer += HeadLen + StrLen;

    // Signal successful conversion
    nResult = SNMPAPI_NOERROR;

Exit:
   if ( nResult == SNMPAPI_ERROR )
      {
      SnmpUtilMemFree( TempObjId );
      }

   return nResult;
} // BER_DecodeAsnObjectId



//
// SnmpBerEncodeAsnImplicitSeq
//
// Notes:
//    The buffer passed as the encoding buffer is considered as the data to
//    be encoded as a sequence.  The data must already be in reverse order
//    (probably from a previous call to an encoding routine.)  The encoding
//    algorithm will fool the 'BER_CreateHeader' routine into thinking that the
//    information in the buffer has just been encoded, just like the
//    other encoding routines.  The result will be 'BER_CreateHeader' appending
//    the header (in reverse order) onto the buffer referenced by *pBuffer.
//
// Return Codes:
//    SNMPAPI_NOERROR
//    SNMPAPI_ERROR
//
// Error Codes:
//    None.
//
SNMPAPI SnmpBerEncodeAsnImplicitSeq(
           IN BYTE nTag,
           IN UINT nSeqLen,
           IN OUT BYTE **pBuffer,
           IN OUT UINT *nLength
           )

{
   // Fool CreateHeader into thinking that this routine just encoded
   //    the items in the sequence
   *nLength -= nSeqLen;

   // Create header
   return BER_CreateHeader( nTag, nSeqLen, pBuffer, nLength );
} // SnmpBerEncodeAsnImplicitSeq



//
// BER_DecodeAsnImplicitSeq:
//     Decodes a sequence.  Returns a pointer into the message buffer which
//     which points to the first object in the sequence.  NO new memory is
//     allocated to hold the sequence objects.  If an error occurs, one of
//     the following error codes is returned and the buffer information
//     remains unchanged.
//
// Notes:
//     To decode the individual objects, call the main decode routine passing
//     the sequence buffer pointer (stream) and the sequence length.
//
// Return codes:
//     SNMPAPI_NOERROR
//     SNMPAPI_ERROR
//
// Error codes:
//     SNMP_BERAPI_INVALID_TAG
//     SNMP_BERAPI_OVERFLOW
//     SNMP_BERAPI_SHORT_BUFFER
//
SNMPAPI BER_DecodeAsnImplicitSeq(
           IN BYTE nTag,          // Expected type of sequence
           IN OUT BYTE **pBuffer, // Buffer to decode
           IN OUT UINT *nLength,  // Length of buffer
           IN OUT AsnAny *pResult // Contains decoded sequence and specific type
           )

{
UINT       StrLen;
UINT       HeadLen;
SNMPAPI    nResult;


    // Process header of IMPLICIT SEQUENCE
    nResult = BER_ProcessHeader( nTag, *pBuffer, *nLength, &StrLen, &HeadLen );
    if ( nResult == SNMPAPI_ERROR )
       {
       goto Exit;
       }

    // Set result type
    pResult->asnType = nTag;

    // Set return values
    pResult->asnValue.sequence.length = StrLen;
    pResult->asnValue.sequence.stream = *pBuffer + HeadLen;

    // Adjust buffer info
    *nLength -= HeadLen + StrLen;
    *pBuffer += HeadLen + StrLen;

    // Signal successful conversion
    nResult = SNMPAPI_NOERROR;

Exit:
    return nResult;
} // BER_DecodeAsnImplicitSeq



//
// SnmpBerQueryAsnType:
//     Examines the buffer to determine the type of the next object
//     in the buffer.
//
// Notes:
//    If any of the ASN types are equal to SNMPAPI_ERROR or SNMPAPI_NOERROR,
//    then this routine will fail to perform accurately.
//
// Return codes:
//     SNMPAPI_NOERROR
//     SNMPAPI_ERROR
//
// Error codes:
//     SNMP_BERAPI_SHORT_BUFFER
//     SNMP_BERAPI_INVALID_TAG
//
SNMPAPI SnmpBerQueryAsnType( BYTE  *pBuffer, /* in */
                             UINT  nLength /* in */ )

{
SNMPAPI nResult;


    // Check for short buffer
    if ( !nLength )
       {
       BERAPI_ERROR( SNMP_BERAPI_SHORT_BUFFER );
       SNMPDBG((   
         SNMP_LOG_ERROR,
         "SNMP: BER: Short buffer encountered.\n"
         ));
       }

    // Check for extended tag
    if ( (pBuffer[BER_TAG_OFFSET] & BER_EXTENDED_TAG) == BER_EXTENDED_TAG )
       {
       BERAPI_ERROR( SNMP_BERAPI_INVALID_TAG );
       SNMPDBG((   
         SNMP_LOG_ERROR,
         "SNMP: BER: Invalid tag encountered.\n"
         ));
       }

    nResult = pBuffer[BER_TAG_OFFSET];

Exit:
    return nResult;
} // SnmpBerQueryAsnType

//-------------------------------- END --------------------------------------
