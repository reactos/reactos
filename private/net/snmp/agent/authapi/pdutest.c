/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    pdutest.c

Abstract:

    Provides code to test the functionality and integrity of the PDUAPI's.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>

#include "pduapi.h"

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

//
// Global Var - To keep stack size down
//
BYTE       pBuffer[2000];

int pdutest()

{
UINT           nLength;
BYTE           *pTemp;
RFC1157Pdus    anyPdu;
BYTE           *pEncodeBuf;
UINT           nEncodeLen;
int            status;
RFC1157VarBindList SrcVarBinds;
RFC1157VarBindList DstVarBinds;


   printf( "\n\nPDU api testing\n" );
   printf( "---------------\n\n\n" );

   // Copy the var bind
   printf( "   Var bind copy test --\n\n" );

   // Test var bind copy
   SrcVarBinds.list = SnmpUtilMemAlloc( 3 * sizeof(RFC1157VarBind) );
   SrcVarBinds.len  = 3;

      SrcVarBinds.list[0].name.ids       = SnmpUtilMemAlloc( 3*sizeof(UINT) );
      SrcVarBinds.list[0].name.idLength  = 3;

         SrcVarBinds.list[0].name.ids[0] = 0;
         SrcVarBinds.list[0].name.ids[1] = 0;
         SrcVarBinds.list[0].name.ids[2] = 0;

      SrcVarBinds.list[0].value.asnType = ASN_OBJECTIDENTIFIER;
      SrcVarBinds.list[0].value.asnValue.object.ids = SnmpUtilMemAlloc( 3*sizeof(UINT) );
      SrcVarBinds.list[0].value.asnValue.object.idLength  = 3;

         SrcVarBinds.list[0].value.asnValue.object.ids[0] = 0;
         SrcVarBinds.list[0].value.asnValue.object.ids[1] = 0;
         SrcVarBinds.list[0].value.asnValue.object.ids[2] = 0;

      SrcVarBinds.list[1].name.ids       = SnmpUtilMemAlloc( 3*sizeof(UINT) );
      SrcVarBinds.list[1].name.idLength  = 3;

         SrcVarBinds.list[1].name.ids[0] = 1;
         SrcVarBinds.list[1].name.ids[1] = 1;
         SrcVarBinds.list[1].name.ids[2] = 1;

      SrcVarBinds.list[1].value.asnType = ASN_OBJECTIDENTIFIER;
      SrcVarBinds.list[1].value.asnValue.object.ids = SnmpUtilMemAlloc( 3*sizeof(UINT) );
      SrcVarBinds.list[1].value.asnValue.object.idLength  = 3;

         SrcVarBinds.list[1].value.asnValue.object.ids[0] = 1;
         SrcVarBinds.list[1].value.asnValue.object.ids[1] = 1;
         SrcVarBinds.list[1].value.asnValue.object.ids[2] = 1;

      SrcVarBinds.list[2].name.ids       = SnmpUtilMemAlloc( 3*sizeof(UINT) );
      SrcVarBinds.list[2].name.idLength  = 3;

         SrcVarBinds.list[2].name.ids[0] = 2;
         SrcVarBinds.list[2].name.ids[1] = 2;
         SrcVarBinds.list[2].name.ids[2] = 2;

      SrcVarBinds.list[2].value.asnType = ASN_OBJECTIDENTIFIER;
      SrcVarBinds.list[2].value.asnValue.object.ids = SnmpUtilMemAlloc( 3*sizeof(UINT) );
      SrcVarBinds.list[2].value.asnValue.object.idLength  = 3;

         SrcVarBinds.list[2].value.asnValue.object.ids[0] = 2;
         SrcVarBinds.list[2].value.asnValue.object.ids[1] = 2;
         SrcVarBinds.list[2].value.asnValue.object.ids[2] = 2;

   if ( SNMPAPI_ERROR == SnmpUtilVarBindListCpy(&DstVarBinds, &SrcVarBinds) )
      {
      printf( "   Var bind copy failed:  %d\n", GetLastError() );
      }
   else
      {
      printf( "   Var bind copy succeeded\n" );
      }

   // Free memory
   SnmpUtilVarBindListFree( &SrcVarBinds );
   SnmpUtilVarBindListFree( &DstVarBinds );

   // Setup GetRequest pdu
   pBuffer[0] = 0x01; // fake
   pBuffer[1] = 0x1c;

      // Setup request-id
      pBuffer[2] = ASN_INTEGER;
      pBuffer[3] = 0x04;
      pBuffer[4] = 0x05;
      pBuffer[5] = 0xae;
      pBuffer[6] = 0x56;
      pBuffer[7] = 0x02;

      // Setup error-status
      pBuffer[8] = ASN_INTEGER;
      pBuffer[9] = 0x01;
      pBuffer[10] = 0x08; // invalid

      // Setup error-index
      pBuffer[11] = ASN_INTEGER;
      pBuffer[12] = 0x01;
      pBuffer[13] = 0x00;

      // Setup variable bindings
      pBuffer[14] = ASN_SEQUENCEOF;
      pBuffer[15] = 0x0e;

         // Setup first sequence
         pBuffer[16] = ASN_SEQUENCE;
         pBuffer[17] = 0x0c;

            // Setup variable name
	      pBuffer[18] = ASN_OBJECTIDENTIFIER;
	      pBuffer[19] = 0x08;
	      pBuffer[20] = 0x2b;
	      pBuffer[21] = 0x06;
	      pBuffer[22] = 0x01;
	      pBuffer[23] = 0x02;
	      pBuffer[24] = 0x01;
	      pBuffer[25] = 0x01;
	      pBuffer[26] = 0x01;
	      pBuffer[27] = 0x00;

	      // Setup variable value
	      pBuffer[28] = ASN_NULL;
	      pBuffer[29] = 0x00;

   // Setup for pdu processing
   nLength = 30;
   pTemp = pBuffer;

   //
   // Decode PDU successfully
   //

   printf( "\n   PDU Decoding tests --\n\n" );

   // Test for invalid PDU
      status = SnmpPduDecodeAnyPdu( &anyPdu, &pTemp, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Invalid PDU, recognized:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Successful PDU decoding, shouldn't be\n" );
         }

   // Fix and try again - test for invalid error status
      pBuffer[0] = ASN_RFC1157_GETREQUEST;
      pTemp = pBuffer;
      nLength = 30;
      status = SnmpPduDecodeAnyPdu( &anyPdu, &pTemp, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Invalid Error status, recognized:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Successful PDU decoding, shouldn't be\n" );
         }

   // Fix and try again
      pBuffer[10] = 0x00;
      pTemp = pBuffer;
      nLength = 30;
      status = SnmpPduDecodeAnyPdu( &anyPdu, &pTemp, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Error decoding PDU, shouldn't be:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Successful PDU decoding\n" );
         }

   //
   // Encode PDU successfully
   //

   printf( "\n   PDU Encoding tests --\n\n" );

   // Test for invalid PDU
      pEncodeBuf = NULL;
      nEncodeLen = 0;
      anyPdu.pduType = 0x00;
      status = SnmpPduEncodeAnyPdu( &anyPdu, &pEncodeBuf, &nEncodeLen );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Invalid PDU, recognized:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Successful PDU encoding, shouldn't be\n" );
         }

   // Test for invalid error status
      pEncodeBuf = NULL;
      nEncodeLen = 0;
      anyPdu.pduType = ASN_RFC1157_GETREQUEST;
      anyPdu.pduValue.pdu.errorStatus = 0x08;
      status = SnmpPduEncodeAnyPdu( &anyPdu, &pEncodeBuf, &nEncodeLen );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Invalid error status, recognized:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Successful PDU encoding, shouldn't be\n" );
         }

   // Fix and try again
      SnmpUtilMemFree( pEncodeBuf );
      pEncodeBuf = NULL;
      nEncodeLen = 0;
      anyPdu.pduValue.pdu.errorStatus = 0x00;
      status = SnmpPduEncodeAnyPdu( &anyPdu, &pEncodeBuf, &nEncodeLen );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Error encoding PDU, shouldn't be:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Successful PDU encoding\n" );
         }

   // Reverse PDU buffer
   SnmpSvcBufRevInPlace( pEncodeBuf, nEncodeLen );

   // Compare PDU buffers
   if ( strncmp(pBuffer, pEncodeBuf, nEncodeLen) )
      {
      printf( "\nPDU Buffers DON'T match!!\n\n" );
      }
   else
      {
      printf( "\nPDU Buffers match!!\n\n" );
      }

   // Free memory for testing traps
   SnmpUtilVarBindListFree( &anyPdu.pduValue.pdu.varBinds );
   SnmpUtilMemFree( pEncodeBuf );

   //
   // Trap testing
   //

   // Setup TRAP
   pBuffer[0] = ASN_RFC1157_TRAP;
   pBuffer[1] = 37;

      // Setup enterprise
      pBuffer[2] = ASN_OBJECTIDENTIFIER;
      pBuffer[3] = 0x04;
      pBuffer[4] = 82;
      pBuffer[5] = 0x02;
      pBuffer[6] = 0x02;
      pBuffer[7] = 0x02;

      // Setup agent-addr
      pBuffer[8] = ASN_RFC1155_IPADDRESS;
      pBuffer[9] = 0x04;
      pBuffer[10] = 'T';
      pBuffer[11] = 'o';
      pBuffer[12] = 'd';
      pBuffer[13] = 'd';

      // Setup generic-trap
      pBuffer[14] = ASN_INTEGER;
      pBuffer[15] = 0x01;
      pBuffer[16] = 0x07; // invalid

      // Setup specific-trap
      pBuffer[17] = ASN_INTEGER;
      pBuffer[18] = 0x01;
      pBuffer[19] = 0x01;

      // Setup time-stamp
      pBuffer[20] = ASN_RFC1155_TIMETICKS;
      pBuffer[21] = 0x01;
      pBuffer[22] = 0x02;

      // Setup var-binds
      pBuffer[23] = ASN_SEQUENCEOF;
      pBuffer[24] = 0x0e;

         // Setup first sequence
         pBuffer[25] = ASN_SEQUENCE;
         pBuffer[26] = 0x0c;

            // Setup variable name
	      pBuffer[27] = ASN_OBJECTIDENTIFIER;
	      pBuffer[28] = 0x08;
	      pBuffer[29] = 0x2b;
	      pBuffer[30] = 0x06;
	      pBuffer[31] = 0x01;
	      pBuffer[32] = 0x02;
	      pBuffer[33] = 0x01;
	      pBuffer[34] = 0x01;
	      pBuffer[35] = 0x01;
	      pBuffer[36] = 0x00;

	      // Setup variable value
	      pBuffer[37] = ASN_NULL;
	      pBuffer[38] = 0x00;

   // Setup for trap processing
   nLength = 39;
   pTemp = pBuffer;

   //
   // Decode trap successfully
   //

   printf( "\nTrap Decoding Tests\n\n" );

      status = SnmpPduDecodeAnyPdu( &anyPdu, &pTemp, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Invalid generic trap, recognized:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Successful TRAP decoding, shouldn't be\n" );
         }

      // Fix and try again
      pBuffer[16] = 0x06;
      nLength = 39;
      pTemp = pBuffer;
      status = SnmpPduDecodeAnyPdu( &anyPdu, &pTemp, &nLength );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Error decoding TRAP, shouldn't be:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Successful TRAP decoding\n" );
         }

   //
   // Encode TRAP successfully
   //

   printf( "\nTrap Encoding Tests\n\n" );

      pEncodeBuf = NULL;
      nEncodeLen = 0;
      anyPdu.pduValue.trap.genericTrap = 0x07;
      status = SnmpPduEncodeAnyPdu( &anyPdu, &pEncodeBuf, &nEncodeLen );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Invalid generic trap, recognized:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Successful TRAP encoding, shouldn't be\n" );
         }

      pEncodeBuf = NULL;
      nEncodeLen = 0;
      anyPdu.pduValue.trap.genericTrap = 0x06;
      status = SnmpPduEncodeAnyPdu( &anyPdu, &pEncodeBuf, &nEncodeLen );
      if ( status == SNMPAPI_ERROR )
         {
         printf( "   Error encoding TRAP, shouldn't be:  %d\n", GetLastError() );
         }
      else
         {
         printf( "   Successful TRAP encoding\n" );
         }

   // Reverse the PDU
   SnmpSvcBufRevInPlace( pEncodeBuf, nEncodeLen );

   // Compare two buffers
   if ( strncmp(pBuffer, pEncodeBuf, nEncodeLen) )
      {
      printf( "\nTrap Buffers DON'T match!!\n\n" );
      }
   else
      {
      printf( "\nTrap Buffers match!!\n\n" );
      }

   // Free memory
   SnmpUtilVarBindListFree( &anyPdu.pduValue.trap.varBinds );
   SnmpUtilMemFree( pEncodeBuf );

   return 0;
}

//-------------------------------- END --------------------------------------

