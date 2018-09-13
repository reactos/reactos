/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    bertest.c

Abstract:

    Test code for ASN.1 BER Encode/Decode APIs.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>

#include "berapi.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

// Will stack overflow if this declaration is in MAIN
BYTE   pBuffer[0x10000];

int bertest()

{
BYTE  *pOutBuf;
BYTE  *pTemp;
UINT   nLength;
UINT   nOutLen;
AsnAny pResult;
int    status;


   pOutBuf = NULL;
   nOutLen = 0;

   printf( "\n\nBER api testing\n" );
   printf( "---------------\n\n\n" );

   //
   // Test general buffer info errors
   //

   printf( "General buffer testing\n\n" );

   // Buffer too short - length 0
      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 1;
      pBuffer[2] = 0xff;
      nLength = 0;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   Short buffer test - length 0:  Error:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Short buffer test succeeded, shouldn't have:  %lu\n",
	         pResult.asnValue.number );
         }

   // Buffer too short - length 1
      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 1;
      pBuffer[2] = 0xff;
      nLength = 1;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   Short buffer test - length 1:  Error:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Short buffer test succeeded, shouldn't have:  %lu\n",
	         pResult.asnValue.number );
         }

   // Buffer too short - length 2
      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 1;
      pBuffer[2] = 0xff;
      nLength = 2;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   Short buffer test - length 2:  Error:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Short buffer test succeeded, shouldn't have:  %lu\n",
	         pResult.asnValue.number );
         }

   // Invalid tag - tags don't match
      pBuffer[0] = ASN_OCTETSTRING;
      pBuffer[1] = 0x01;
      pBuffer[2] = 0xff;
      nLength = 3;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   Invalid tag test - Don't match:  Error:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Invalid tag test succeeded, shouldn't have:  %lu\n",
	         pResult.asnValue.number );
         }

   // long tag - we don't support tags requiring more than 1 octet
      pBuffer[0] = ASN_INTEGER | 0x1f;
      pBuffer[2] = 0x01;
      pBuffer[1] = 1;
      pBuffer[3] = 0xff;
      nLength = 4;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER|0x1f, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   Invalid tag test - Extended tag:  Error:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Invalid tag test succeeded, shouldn't have:  %lu\n",
	         pResult.asnValue.number );
         }

   // Max Buffer length test
      pBuffer[0] = ASN_OCTETSTRING;
      pBuffer[1] = 0x82;
      pBuffer[2] = 0xff;
      pBuffer[3] = 0xfb;
      nLength = 0xffff;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_OCTETSTRING, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   Max length buffer, should not fail:  Error:  %d\n",
	          GetLastError() );
         }
      else
         {
         printf( "   Max length buffer succeeded, correct\n" );
         }

   //
   // ASN Integer testing
   //

   printf( "\nInteger testing\n\n" );

   printf( "   Decode Integer -128:  " );

      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 1;
      pBuffer[2] = 0x80;
      nLength = 3;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
         printf( "%ld\n", pResult.asnValue.number );
         }

   printf( "   Decode Integer 128:  " );

      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 2;
      pBuffer[2] = 0x00;
      pBuffer[3] = 0x80;
      nLength = 4;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
         printf( "%ld\n", pResult.asnValue.number );
         }

   printf( "   Decode Integer -32,768:  " );

      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 2;
      pBuffer[2] = 0x80;
      pBuffer[3] = 0x00;
      nLength = 4;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
         printf( "%ld\n", pResult.asnValue.number );
         }

   printf( "   Decode Integer 32,768:  " );

      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 3;
      pBuffer[2] = 0x00;
      pBuffer[3] = 0x80;
      pBuffer[4] = 0x00;
      nLength = 5;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
         printf( "%ld\n", pResult.asnValue.number );
         }

   printf( "   Decode Integer -8,388,608:  " );

      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 3;
      pBuffer[2] = 0x80;
      pBuffer[3] = 0x00;
      pBuffer[4] = 0x00;
      nLength = 5;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
         printf( "%ld\n", pResult.asnValue.number );
         }

   printf( "   Decode Integer 8,388,608:  " );

      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 4;
      pBuffer[2] = 0x00;
      pBuffer[3] = 0x80;
      pBuffer[4] = 0x00;
      pBuffer[5] = 0x00;
      nLength = 6;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
         printf( "%ld\n", pResult.asnValue.number );
         }

   printf( "   Decode (Smallest) Integer -2,147,483,648:  " );

      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 4;
      pBuffer[2] = 0x80;
      pBuffer[3] = 0x00;
      pBuffer[4] = 0x00;
      pBuffer[5] = 0x00;
      nLength = 6;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
         printf( "%ld\n", pResult.asnValue.number );
         }

   printf( "   Decode (Largest) Integer 2,147,483,647:  " );

      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 4;
      pBuffer[2] = 0x7f;
      pBuffer[3] = 0xff;
      pBuffer[4] = 0xff;
      pBuffer[5] = 0xff;
      nLength = 6;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
         printf( "%ld\n", pResult.asnValue.number );
         }

   printf( "   Decode Integer 0:  " );

      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 1;
      pBuffer[2] = 0x00;
      nLength = 3;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
         printf( "%ld\n", pResult.asnValue.number );
         }

   printf( "   Decode Integer -1:  " );

      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 4;
      pBuffer[2] = 0xff;
      pBuffer[3] = 0xff;
      pBuffer[4] = 0xff;
      pBuffer[5] = 0xff;
      nLength = 6;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
         printf( "%ld\n", pResult.asnValue.number );
         }

   printf( "   Decode (Counter) Integer 4,294,967,295:  " );

      pBuffer[0] = ASN_RFC1155_COUNTER;
      pBuffer[1] = 5;
      pBuffer[2] = 0x00;
      pBuffer[3] = 0xff;
      pBuffer[4] = 0xff;
      pBuffer[5] = 0xff;
      pBuffer[6] = 0xff;
      nLength = 7;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_RFC1155_COUNTER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
         printf( "%lu\n", pResult.asnValue.number );
         }

   printf( "   Decode (Too large) 4,294,967,296:  " );

      pBuffer[0] = ASN_INTEGER;
      pBuffer[1] = 5;
      pBuffer[2] = 0x01;
      pBuffer[3] = 0x00;
      pBuffer[4] = 0x00;
      pBuffer[5] = 0x00;
      pBuffer[6] = 0x00;
      nLength = 7;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
         printf( "%ld\n", pResult.asnValue.number );
         }

   printf( "   Encode -128:  " );

      {
      BYTE       *pOut   = NULL;
      UINT       nLength = 0;
      AsnInteger Number  = 0xffffff80;

      status = SnmpBerEncodeAsnInteger( ASN_INTEGER, Number, &pOut, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
	 SnmpSvcBufRevInPlace( pOut, nLength );

         pTemp = pOut;

         status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                          &pResult );
	 printf( "%ld\n", pResult.asnValue.number );
         }

      SnmpUtilMemFree( pOut );
      }

   printf( "   Encode 128:  " );

      {
      BYTE       *pOut   = NULL;
      UINT       nLength = 0;
      AsnInteger Number  = 0x00000080;

      status = SnmpBerEncodeAsnInteger( ASN_INTEGER, Number, &pOut, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
	 SnmpSvcBufRevInPlace( pOut, nLength );

         pTemp = pOut;

         status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                          &pResult );
	 printf( "%ld\n", pResult.asnValue.number );
         }

      SnmpUtilMemFree( pOut );
      }

   printf( "   Encode -32,768:  " );

      {
      BYTE       *pOut   = NULL;
      UINT       nLength = 0;
      AsnInteger Number  = 0xffff8000;

      status = SnmpBerEncodeAsnInteger( ASN_INTEGER, Number, &pOut, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
	 SnmpSvcBufRevInPlace( pOut, nLength );

         pTemp = pOut;

         status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                          &pResult );
	 printf( "%ld\n", pResult.asnValue.number );
         }

      SnmpUtilMemFree( pOut );
      }

   printf( "   Encode 32,768:  " );

      {
      BYTE       *pOut   = NULL;
      UINT       nLength = 0;
      AsnInteger Number  = 0x00008000;

      status = SnmpBerEncodeAsnInteger( ASN_INTEGER, Number, &pOut, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
	 SnmpSvcBufRevInPlace( pOut, nLength );

         pTemp = pOut;

         status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                          &pResult );
	 printf( "%ld\n", pResult.asnValue.number );
         }

      SnmpUtilMemFree( pOut );
      }

   printf( "   Encode -8,388,608:  " );

      {
      BYTE       *pOut   = NULL;
      UINT       nLength = 0;
      AsnInteger Number  = 0xff800000;

      status = SnmpBerEncodeAsnInteger( ASN_INTEGER, Number, &pOut, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
	 SnmpSvcBufRevInPlace( pOut, nLength );

         pTemp = pOut;

         status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                          &pResult );
	 printf( "%ld\n", pResult.asnValue.number );
         }

      SnmpUtilMemFree( pOut );
      }

   printf( "   Encode 8,388,608:  " );

      {
      BYTE       *pOut   = NULL;
      UINT       nLength = 0;
      AsnInteger Number  = 0x00800000;

      status = SnmpBerEncodeAsnInteger( ASN_INTEGER, Number, &pOut, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
	 SnmpSvcBufRevInPlace( pOut, nLength );

         pTemp = pOut;

         status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                          &pResult );
	 printf( "%ld\n", pResult.asnValue.number );
         }

      SnmpUtilMemFree( pOut );
      }

   printf( "   Encode (Smallest) -2,147,483,648:  " );

      {
      BYTE       *pOut   = NULL;
      UINT       nLength = 0;
      AsnInteger Number  = 0x80000000;

      status = SnmpBerEncodeAsnInteger( ASN_INTEGER, Number, &pOut, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
	 SnmpSvcBufRevInPlace( pOut, nLength );

         pTemp = pOut;

         status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                          &pResult );
	 printf( "%ld\n", pResult.asnValue.number );
         }

      SnmpUtilMemFree( pOut );
      }

   printf( "   Encode (Largest) 2,147,483,647:  " );

      {
      BYTE       *pOut   = NULL;
      UINT       nLength = 0;
      AsnInteger Number  = 0x7fffffff;

      status = SnmpBerEncodeAsnInteger( ASN_INTEGER, Number, &pOut, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
	 SnmpSvcBufRevInPlace( pOut, nLength );

         pTemp = pOut;

         status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                          &pResult );
	 printf( "%ld\n", pResult.asnValue.number );
         }

      SnmpUtilMemFree( pOut );
      }

   printf( "   Encode 0:  " );

      {
      BYTE       *pOut   = NULL;
      UINT       nLength = 0;
      AsnInteger Number  = 0;

      status = SnmpBerEncodeAsnInteger( ASN_INTEGER, Number, &pOut, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
	 SnmpSvcBufRevInPlace( pOut, nLength );

         pTemp = pOut;

         status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                          &pResult );
	 printf( "%ld\n", pResult.asnValue.number );
         }

      SnmpUtilMemFree( pOut );
      }

   printf( "   Encode -1:  " );

      {
      BYTE       *pOut   = NULL;
      UINT       nLength = 0;
      AsnInteger Number  = -1;

      status = SnmpBerEncodeAsnInteger( ASN_INTEGER, Number, &pOut, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
	 SnmpSvcBufRevInPlace( pOut, nLength );

         pTemp = pOut;

         status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                          &pResult );
	 printf( "%ld\n", pResult.asnValue.number );
         }

      SnmpUtilMemFree( pOut );
      }

   printf( "   Encode (Counter) 4,294,967,295:  " );

      {
      BYTE       *pOut   = NULL;
      UINT       nLength = 0;
      ULONG      Number  = 4294967295;

      status = SnmpBerEncodeAsnInteger( ASN_RFC1155_COUNTER, Number,
                                        &pOut, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "Error = %d\n", GetLastError() );
         }
      else
         {
	 SnmpSvcBufRevInPlace( pOut, nLength );

         pTemp = pOut;

         status = SnmpBerDecodeAsnStream( ASN_INTEGER, &pTemp, &nLength,
                                          &pResult );
	 printf( "%lu\n", pResult.asnValue.number );
         }

      SnmpUtilMemFree( pOut );
      }

   //
   // Octet string testing
   //

   printf( "\nOctet String Testing\n\n" );

   // Test NULL string
      pBuffer[0] = ASN_OCTETSTRING;
      pBuffer[1] = 0;
      nLength = 2;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_OCTETSTRING, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   NULL string Test shouldn't fail:  Error:  %d\n",
	          GetLastError() );
         }
      else
         {
         printf( "   NULL string test successful\n" );
         }

   //
   // NULL testing
   //

   printf( "\nNULL testing\n\n" );

   // Test invalid length
      pBuffer[0] = ASN_NULL;
      pBuffer[1] = 1;
      nLength = 3;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_NULL, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   NULL Test for invalid length:  Error:  %d\n",
	          GetLastError() );
         }
      else
         {
         printf( "   NULL test successful, shouldn't be\n" );
         }

   //
   // Object Identifier testing
   //

   printf( "\nObject Identifier Testing\n\n" );

   printf( "   Decoding tests --\n\n" );

   // Test invalid length - Obj ID too short
      pBuffer[0] = ASN_OBJECTIDENTIFIER;
      pBuffer[1] = 0;
      nLength = 2;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_OBJECTIDENTIFIER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   Object ID too short Test:  Error:  %d\n",
	          GetLastError() );
         }
      else
         {
         printf( "   Object ID too short test successful, shouldn't be\n" );
         }

   // Test invalid length - Obj ID too long
      pBuffer[0] = ASN_OBJECTIDENTIFIER;
      pBuffer[1] = 0x82;
      pBuffer[2] = 0x7f;
      pBuffer[3] = 0x00;
      pBuffer[4] = 41;
      memset( &pBuffer[5], 1, 0x7efe );
      nLength = 0x7f04;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_OBJECTIDENTIFIER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   Object ID length Overflow Test:  Error:  %d\n",
	          GetLastError() );
         }
      else
         {
         printf( "   Object ID length overflow test successful, shouldn't be\n" );
         }

   // Test invalid first element
      pBuffer[0] = ASN_OBJECTIDENTIFIER;
      pBuffer[1] = 3;
      pBuffer[2] = 123;
      pBuffer[3] = 2;
      pBuffer[4] = 3;
      nLength = 5;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_OBJECTIDENTIFIER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   Object ID 1st element too large:  Error:  %d\n",
	          GetLastError() );
         }
      else
         {
         printf( "   Object ID 1st element too large, successful, shouldn't be\n" );
         }

   // Test overflow condition on elements
      pBuffer[0] = ASN_OBJECTIDENTIFIER;
      pBuffer[1] = 4;
      pBuffer[2] = 0x00;
      pBuffer[3] = 0x81;
      pBuffer[4] = 0x82;
      pBuffer[5] = 0x83;
      nLength = 6;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_OBJECTIDENTIFIER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   Object ID element OverFlow:  Error:  %d\n",
	          GetLastError() );
         }
      else
         {
         printf( "   Object ID element OverFlow, successful, shouldn't be\n" );
         }

   // Test shortest length
      pBuffer[0] = ASN_OBJECTIDENTIFIER;
      pBuffer[1] = 1;
      pBuffer[2] = 40;
      nLength = 3;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_OBJECTIDENTIFIER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   Object ID short Test, shouldn't be here:  Error:  %d\n",
	          GetLastError() );
         }
      else
         {
         printf( "   Object ID shortest length test successful\n" );
         }

   // Test maximum length
      pBuffer[0] = ASN_OBJECTIDENTIFIER;
      pBuffer[1] = 0x82;
      pBuffer[2] = 0x7e;
      pBuffer[3] = 0xff;
      pBuffer[4] = 41;
      memset( &pBuffer[5], 1, 0x7efd );
      nLength = 0x7f03;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_OBJECTIDENTIFIER, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   Object maximum length Test, shouldn't be here:  Error:  %d\n",
	          GetLastError() );
         }
      else
         {
         printf( "   Object ID maximum length test successful\n" );
         }

   printf( "\n   Encoding tests --\n\n" );

   // Test encoding of invalid 1st element
      pResult.asnValue.object.idLength = 2;
      pResult.asnValue.object.ids = SnmpUtilMemAlloc( sizeof(UINT) );
      pResult.asnValue.object.ids[0] = 3;
      status = SnmpBerEncodeAsnObjectId( ASN_OBJECTIDENTIFIER,
                                         &pResult.asnValue.object, &pOutBuf, &nOutLen );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   1st element too large:  Error:  %d\n",
	          GetLastError() );

	 SnmpUtilMemFree( pOutBuf );
         pOutBuf = NULL;
         nOutLen = 0;
         }
      else
         {
         printf( "   1st element too large, successful, shouldn't be\n" );
         }

   // Test encoding of invalid 2nd element
      pResult.asnValue.object.idLength = 2;
      pResult.asnValue.object.ids[0] = 0;
      pResult.asnValue.object.ids[1] = 40;
      status = SnmpBerEncodeAsnObjectId( ASN_OBJECTIDENTIFIER,
                                         &pResult.asnValue.object, &pOutBuf, &nOutLen );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   2nd element too large:  Error:  %d\n",
	          GetLastError() );

	 SnmpUtilMemFree( pOutBuf );
         pOutBuf = NULL;
         nOutLen = 0;
         }
      else
         {
         printf( "   2nd element too large, successful, shouldn't be\n" );
         }

   // Test encoding with idLength of 1
      pResult.asnValue.object.idLength = 1;
      pResult.asnValue.object.ids[0] = 0;
      pResult.asnValue.object.ids[1] = 0;
      status = SnmpBerEncodeAsnObjectId( ASN_OBJECTIDENTIFIER, 
                                         &pResult.asnValue.object, &pOutBuf,
                                         &nOutLen );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   idLength == 1 failed. Error: %d\n",
                   GetLastError() );
         SnmpUtilMemFree ( pOutBuf );
         pOutBuf = NULL;
         nOutLen = 0;
         }
      else
         {
         printf(" idLength == 1 succeeded.\n");
         }
   //
   // Sequence Testing
   //

   printf( "\nSequence Testing\n\n" );

   // Null sequence test
      pBuffer[0] = ASN_SEQUENCE;
      pBuffer[1] = 0;
      nLength = 65;

      pTemp = pBuffer;

      status = SnmpBerDecodeAsnStream( ASN_SEQUENCE, &pTemp, &nLength,
                                       &pResult );
      if ( status == SNMPAPI_ERROR )
         {
         printf ( "   NULL sequence Test shouldn't fail:  Error:  %d\n",
	          GetLastError() );
         }
      else
         {
         printf( "   NULL sequence test successful\n" );
         }

    //
    // General testing
    //

   printf( "\nGeneral Testing\n\n" );

    // Set first type - Integer
    pBuffer[0] = ASN_INTEGER;
    pBuffer[1] = 3;
    pBuffer[2] = 0x0f;
    pBuffer[3] = 0x42;
    pBuffer[4] = 0x40;

    // Set second type - Octet String
    pBuffer[5] = ASN_OCTETSTRING;
    pBuffer[6] = 0x82;
    pBuffer[7] = 0x01;
    pBuffer[8] = 0x40;
    strcpy( &pBuffer[9], "adfasdfasdfasdfasdfaasdfasdfasdfasdfasdfadfadfadsfadfadfadsfasdfadsfasdfasdfasdfadfasdfasdfasdfasdfaasdfasdfasdfasdfasdfadfadfadsfadfadfadsfasdfadsfasdfasdfasdfadfasdfasdfasdfasdfaasdfasdfasdfasdfasdfadfadfadsfadfadfadsfasdfadsfasdfasdfasdfadfasdfasdfasdfasdfaasdfasdfasdfasdfasdfadfadfadsfadfadfadsfasdfadsfasdfasdfasdf" );

    // Set third type - NULL
    pBuffer[329] = ASN_NULL;
    pBuffer[330] = 0;

    // Set fourth type - Object Identifier
    pBuffer[331] = ASN_OBJECTIDENTIFIER;
    pBuffer[332] = 5;
    pBuffer[333] = 0x28;
    pBuffer[334] = 0xc2;
    pBuffer[335] = 0x7a;
    pBuffer[336] = 0x05;
    pBuffer[337] = 0x01;

    // Set fifth type - Sequence
    pBuffer[338] = ASN_RFC1157_SETREQUEST;
    pBuffer[339] = 12;

       pBuffer[340] = ASN_OBJECTIDENTIFIER;
       pBuffer[341] = 5;
       pBuffer[342] = 0x28;
       pBuffer[343] = 0xc2;
       pBuffer[344] = 0x7a;
       pBuffer[345] = 0x05;
       pBuffer[346] = 0x01;

       pBuffer[347] = ASN_INTEGER;
       pBuffer[348] = 3;
       pBuffer[349] = 0x0f;
       pBuffer[350] = 0x42;
       pBuffer[351] = 0x40;

    // Set length of buffer
    nLength = 352;

    // Set temporary variable
    pTemp = pBuffer;

    // Init output buffer
    pOutBuf = NULL;
    nOutLen = 0;

    // Expected type - Integer
    status = SnmpBerDecodeAsnStream(ASN_INTEGER, &pTemp, &nLength, 
                                    &pResult);
    if ( status == SNMPAPI_ERROR )
       {
       printf( "   Error\n" );
       }

       status = SnmpBerEncodeAsnInteger( ASN_INTEGER, pResult.asnValue.number, &pOutBuf, &nOutLen );
       if ( status == SNMPAPI_ERROR )
          {
          printf( "   Error\n" );
          }

       status = _msize( pOutBuf );

    // Expected type - Octet String
    status = SnmpBerDecodeAsnStream(ASN_OCTETSTRING, &pTemp, &nLength, 
                                    &pResult);
    if ( status == SNMPAPI_ERROR )
       {
       printf( "   Error\n" );
       }

       status = SnmpBerEncodeAsnOctetStr( ASN_OCTETSTRING,
                                          &pResult.asnValue.string, &pOutBuf, &nOutLen );
       if ( status == SNMPAPI_ERROR )
          {
          printf( "   Error\n" );
          }

       status = _msize( pOutBuf );

    // Expected type - NULL
    status = SnmpBerDecodeAsnStream(ASN_NULL, &pTemp, &nLength, 
                                    &pResult);
    if ( status == SNMPAPI_ERROR )
       {
       printf( "   Error\n" );
       }

       status = SnmpBerEncodeAsnNull( ASN_NULL, &pOutBuf, &nOutLen );
       if ( status == SNMPAPI_ERROR )
          {
          printf( "   Error\n" );
          }

       status = _msize( pOutBuf );

    // Expected type - Object Identifier
    status = SnmpBerDecodeAsnStream(ASN_OBJECTIDENTIFIER, &pTemp, &nLength, 
                                    &pResult);
    if ( status == SNMPAPI_ERROR )
       {
       printf( "   Error\n" );
       }

       status = SnmpBerEncodeAsnObjectId( ASN_OBJECTIDENTIFIER, &pResult.asnValue.object, &pOutBuf, &nOutLen );
       if ( status == SNMPAPI_ERROR )
          {
          printf( "   Error\n" );
          }

       status = _msize( pOutBuf );

    // Expected type - Sequence
    status = SnmpBerDecodeAsnStream(ASN_RFC1157_SETREQUEST, &pTemp, &nLength, 
                                    &pResult);
    if ( status == SNMPAPI_ERROR )
       {
       printf( "   Error\n" );
       }

       {
       char *BufPtr;
       UINT BufLen;
       AsnObjectIdentifier ObjectId;
       AsnInteger nInt;
       UINT nSeqLen;


       BufPtr = pResult.asnValue.sequence.stream;
       BufLen = nSeqLen = pResult.asnValue.sequence.length;

       // Expected type - ObjectId
       status = SnmpBerDecodeAsnStream(ASN_OBJECTIDENTIFIER, &BufPtr, &BufLen, 
                                       &pResult);
       if ( status == SNMPAPI_ERROR )
          {
          printf( "   Error\n" );
          }

       // Save object id
       ObjectId = pResult.asnValue.object;

       // Expected type - Integer
       status = SnmpBerDecodeAsnStream(ASN_INTEGER, &BufPtr, &BufLen, 
                                       &pResult);
       if ( status == SNMPAPI_ERROR )
          {
          printf( "   Error\n" );
          }

       // Save integer
       nInt = pResult.asnValue.number;

       status = SnmpBerEncodeAsnInteger( ASN_INTEGER, nInt, &pOutBuf, &nOutLen );
       if ( status == SNMPAPI_ERROR )
          {
          printf( "   Error\n" );
          }
       status = SnmpBerEncodeAsnObjectId( ASN_OBJECTIDENTIFIER, &ObjectId, &pOutBuf, &nOutLen );
       if ( status == SNMPAPI_ERROR )
          {
          printf( "   Error\n" );
          }
       status = SnmpBerEncodeAsnSequence( nSeqLen, &pOutBuf, &nOutLen );
       if ( status == SNMPAPI_ERROR )
          {
          printf( "   Error\n" );
          }
       }

       status = _msize( pOutBuf );

    SnmpUtilMemFree( pOutBuf );

    return 0;
} // main

//-------------------------------- END --------------------------------------

