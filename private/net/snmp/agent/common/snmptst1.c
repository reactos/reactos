/*++

Copyright (c) 1992-1996  Microsoft Corporation

Module Name:

    snmptst1.c

Abstract:

    Routines to test the functionality of utility functions in COMMON.

Environment:

    User Mode - Win32

Revision History:

    10-May-1996 DonRyan
        Removed banner from Technology Dynamics, Inc.

--*/

//--------------------------- WINDOWS DEPENDENCIES --------------------------

//--------------------------- STANDARD DEPENDENCIES -- #include<xxxxx.h> ----

#include <stdio.h>

//--------------------------- MODULE DEPENDENCIES -- #include"xxxxx.h" ------

#include <snmp.h>
#include <snmputil.h>

//--------------------------- SELF-DEPENDENCY -- ONE #include"module.h" -----

//--------------------------- PUBLIC VARIABLES --(same as in module.h file)--

//--------------------------- PRIVATE CONSTANTS -----------------------------

//--------------------------- PRIVATE STRUCTS -------------------------------

//--------------------------- PRIVATE VARIABLES -----------------------------

//--------------------------- PRIVATE PROTOTYPES ----------------------------

//--------------------------- PRIVATE PROCEDURES ----------------------------

//--------------------------- PUBLIC PROCEDURES -----------------------------

char src[] = "0123456789";
char dst[100];
AsnObjectIdentifier orig;
AsnObjectIdentifier new;

void __cdecl main()

{
   printf( "Buffer reverse test --\n\n" );

      printf( "   Before:  %s\n", src );

      SnmpSvcBufRevInPlace( src, 10 );

      printf( "   After :  %s\n", src );

   printf( "\nBuffer copy reverse test --\n\n" );

      printf( "   Source:  %s\n", src );

      SnmpSvcBufRevAndCpy( dst, src, 10 );

      printf( "   Dest  :  %s\n", dst );

   //
   // Setup for OID tests
   //

   orig.ids = (UINT *)SnmpUtilMemAlloc( 100*sizeof(UINT) );
   orig.ids[0] = 0;
   orig.ids[1] = 1;
   orig.ids[2] = 2;
   orig.ids[3] = 3;
   orig.idLength = 4;
   printf( "\nOID copy test --\n\n" );

      printf( "   Original OID:  " );
      SnmpUtilPrintOid( &orig );
      printf( " --> %d\n", orig.idLength );

   SnmpUtilOidCpy( &new, &orig );

      printf( "   New OID     :  " );
      SnmpUtilPrintOid( &new );
      printf( " --> %d\n", new.idLength );

   printf( "\nOID compare test --\n\n" );

      printf( "   First less than second\n\n" );

      orig.ids[3] = 0;

      printf( "   First OID :  " );
      SnmpUtilPrintOid( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SnmpUtilPrintOid( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SnmpUtilOidCmp(&orig, &new) );

      printf( "\n   First greater than second\n\n" );

      orig.ids[3] = 4;

      printf( "   First OID :  " );
      SnmpUtilPrintOid( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SnmpUtilPrintOid( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SnmpUtilOidCmp(&orig, &new) );

      printf( "\n   First shorter than second\n\n" );

      orig.idLength = 3;

      printf( "   First OID :  " );
      SnmpUtilPrintOid( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SnmpUtilPrintOid( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SnmpUtilOidCmp(&orig, &new) );

      printf( "\n   First longer than second\n\n" );

      orig.idLength = 5;
      orig.ids[3] = 3;
      orig.ids[4] = 4;

      printf( "   First OID :  " );
      SnmpUtilPrintOid( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SnmpUtilPrintOid( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SnmpUtilOidCmp(&orig, &new) );

      printf( "\n   Prefix equal\n\n" );

      printf( "   First OID :  " );
      SnmpUtilPrintOid( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SnmpUtilPrintOid( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SnmpUtilOidNCmp(&orig, &new, new.idLength) );

      printf( "\n   0 length prefix test\n\n" );

      orig.idLength = 0;

      printf( "   First OID :  " );
      SnmpUtilPrintOid( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SnmpUtilPrintOid( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SnmpUtilOidNCmp(&orig, &new, orig.idLength) );

      printf( "\n   Both equal\n\n" );

      orig.idLength = 4;
      orig.ids[3] = 3;
      orig.ids[4] = 4;

      printf( "   First OID :  " );
      SnmpUtilPrintOid( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SnmpUtilPrintOid( &new );
      printf( " --> %d\n", new.idLength );

      printf( "\n   Result:  %d\n", SnmpUtilOidCmp(&orig, &new) );

      printf( "\n   Append second to first\n\n" );

      orig.idLength = 5;
      orig.ids[3] = 10;
      orig.ids[4] = 20;

      printf( "   First OID :  " );
      SnmpUtilPrintOid( &orig );
      printf( " --> %d\n", orig.idLength );

      printf( "   Second OID:  " );
      SnmpUtilPrintOid( &new );
      printf( " --> %d\n", new.idLength );

      SnmpUtilOidAppend( &orig, &new );
      SnmpUtilPrintOid( &orig );
      printf( " --> %d\n", orig.idLength );

   SnmpUtilOidFree( &orig );
   SnmpUtilOidFree( &new );
}

//-------------------------------- END --------------------------------------

